/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

/*
 * Generic module to manage structures accessible in physical mode. The
 * module auto-collect pages without any referenced element.
 *
 * XXX: There should be another list per # of free elements in order to
 *	always alloc a new element in the most populated page
 */

#include <kern/kalloc.h>
#include <kern/lock.h>
#include <kern/queue.h>
#include <mach/etap.h>
#include <mach/vm_param.h>
#include <mach/vm_types.h>
#include <vm/vm_page.h>
#include <hp_pa/physmem.h>
#include <hp_pa/thread.h>

#define	ROUND_ALIGN(i)	(((i) + 3) & ~3)

queue_head_t	physmem_queue;			/* Queue of all physmem */
vm_page_t       physmem_queue_free;		/* Queue of free pages */
decl_simple_lock_data(,	physmem_queue_lock)	/* Lock for queues */

/*
 * Physmem module initialization
 */
void
physmem_init()
{
    queue_init(&physmem_queue);
    simple_lock_init(&physmem_queue_lock, ETAP_MISC_Q);
}

/*
 * Add a new queue of element to manage
 */
void
physmem_add(
    physmem_queue_t pq,
    vm_size_t length,
    etap_event_t event)
{
    unsigned int i;

    queue_init(&pq->pq_fpages);
    queue_init(&pq->pq_nfpages);
    pq->pq_used = 0;
    pq->pq_busy = 0;
    pq->pq_length = ROUND_ALIGN(length);
    pq->pq_num = PAGE_SIZE / pq->pq_length;
    while (sizeof (struct physmem_header) + ROUND_ALIGN(pq->pq_num) +
	   pq->pq_num * pq->pq_length > PAGE_SIZE)
	pq->pq_num--;
    simple_lock_init(&pq->pq_lock, event);

    simple_lock(&physmem_queue_lock);
    queue_enter(&physmem_queue, pq, physmem_queue_t, pq_chain);
    simple_unlock(&physmem_queue_lock);
}

/*
 * Allocate an element in a given queue
 */
vm_offset_t
physmem_alloc(
    physmem_queue_t pq)
{
    vm_page_t m;
    physmem_header_t ph;
    unsigned char *p;
    unsigned int i;

    m = VM_PAGE_NULL;
    simple_lock(&pq->pq_lock);
    while (pq->pq_busy * pq->pq_num == pq->pq_used) {
	if (m) {
	    /*
	     * Initialize physmem header at the end of element array
	     */
	    ph = (physmem_header_t)(m->phys_addr + pq->pq_num * pq->pq_length);
	    ph->ph_page = m;
	    ph->ph_used = 1;
	    p = (unsigned char *)(ph + 1);
	    p[0] = 0;
	    for (i = 1; i < pq->pq_num; i++)
		p[i] = 1;

	    queue_enter(&pq->pq_nfpages, ph, physmem_header_t, ph_pages);
	    pq->pq_busy++;
	    pq->pq_used++;
	    simple_unlock(&pq->pq_lock);

	    /*
	     * Return first element of newly allocated page
	     */
	    return (m->phys_addr);
	}

	/*
	 * No element available ==> allocate a new page
	 */
	simple_unlock(&pq->pq_lock);
	simple_lock(&physmem_queue_lock);
	if (physmem_queue_free != VM_PAGE_NULL) {
	    m = physmem_queue_free;
	    physmem_queue_free = (vm_page_t)m->pageq.next;
	    simple_unlock(&physmem_queue_lock);
	} else {
	    simple_unlock(&physmem_queue_lock);
	    while ((m = vm_page_grab()) == VM_PAGE_NULL)
		VM_PAGE_WAIT();
	    vm_page_gobble(m);
	    pmap_enter(kernel_pmap, m->phys_addr, m->phys_addr,
		       VM_PROT_READ|VM_PROT_WRITE, TRUE);
	}
	simple_lock(&pq->pq_lock);
    }

    /*
     * Find into non full page list to allocate a new element
     */
    assert(!queue_empty(&pq->pq_nfpages));
    ph = (physmem_header_t)queue_first(&pq->pq_nfpages);
    assert(ph->ph_used < pq->pq_num);
    p = (unsigned char *)(ph + 1);
    for (i = 0; i < pq->pq_num; i++)
	if (p[i] == 1)
	    break;
    assert(i < pq->pq_num);

    p[i] = 0;
    pq->pq_used++;
    if (++ph->ph_used == pq->pq_num) {
	queue_remove(&pq->pq_nfpages, ph, physmem_header_t, ph_pages);
	queue_enter(&pq->pq_fpages, ph, physmem_header_t, ph_pages);
    }
    simple_unlock(&pq->pq_lock);

    /*
     * Free allocated page if not used. The page can be freed immediately
     * since physmem_alloc() is called without any lock held.
     */
    if (m) {
	pmap_remove(kernel_pmap, m->phys_addr, m->phys_addr + PAGE_SIZE);
	VM_PAGE_CHECK(m);
	VM_PAGE_FREE(m);
    }

    return (ph->ph_page->phys_addr + i * pq->pq_length);
}

/*
 * Release an element of a given queue
 */
void
physmem_free(
    physmem_queue_t pq,
    vm_offset_t addr)
{
    physmem_header_t ph;
    unsigned int i;
    unsigned char *p;

    ph = (physmem_header_t)(trunc_page(addr) + pq->pq_num * pq->pq_length);
    assert(ph->ph_page->phys_addr == trunc_page(addr));

    i = (addr - trunc_page(addr)) / pq->pq_length;
    p = (unsigned char *)(ph + 1);

    simple_lock(&pq->pq_lock);
    assert(p[i] == 0);

    assert(pq->pq_used > 0);
    pq->pq_used--;

    assert(ph->ph_used > 0);
    if (ph->ph_used-- == pq->pq_num) {
	queue_remove(&pq->pq_fpages, ph, physmem_header_t, ph_pages);
	queue_enter(&pq->pq_nfpages, ph, physmem_header_t, ph_pages);
    }
    if (ph->ph_used > 0) {
	p[i] = 1;
	simple_unlock(&pq->pq_lock);
	return;
    }

#if MACH_ASSERT
    p[i] = 1;
    for (i = 0; i < pq->pq_num; i++)
	assert(p[i] == 1);
#endif /* MACH_ASSERT */

    /*
     * Remove page from the queue
     */
    assert(pq->pq_busy > 0);
    pq->pq_busy--;
    queue_remove(&pq->pq_nfpages, ph, physmem_header_t, ph_pages);
    simple_unlock(&pq->pq_lock);

    /*
     * Free allocated page
     */
    simple_lock(&physmem_queue_lock);
    ph->ph_page->pageq.next = (queue_entry_t)physmem_queue_free;
    physmem_queue_free = ph->ph_page;
    simple_unlock(&physmem_queue_lock);
}

/*
 * Collect and free unused pages
 */
void
physmem_collect_scan()
{
    vm_page_t m;

    simple_lock(&physmem_queue_lock);
    while (physmem_queue_free != VM_PAGE_NULL) {
	m = physmem_queue_free;
	physmem_queue_free = (vm_page_t)m->pageq.next;
	simple_unlock(&physmem_queue_lock);

	/*
	 * Release mapping and page
	 */
	pmap_remove(kernel_pmap, m->phys_addr, m->phys_addr + PAGE_SIZE);
	VM_PAGE_CHECK(m);
	VM_PAGE_FREE(m);

	simple_lock(&physmem_queue_lock);
    }
    simple_unlock(&physmem_queue_lock);
}
