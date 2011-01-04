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
 * support of memory above 16 Megs for DMA limited to memory
 * below 16 Megs. Copies high memory lo low memory before DMA
 * write operations and does the reverse at completion time for
 * DMA read operations
 */

#include <cpus.h>
#include <platforms.h>
#include <kern/lock.h>
#include <mach/vm_param.h>
#include <vm/vm_page.h>
#include <i386/AT386/himem.h>
#include <kern/kalloc.h>
#include <kern/spl.h>
#include <device/device_types.h>
#include <mach/boolean.h>
#include <kern/misc_protos.h>
#include <i386/AT386/misc_protos.h>

hil_t		hil_head;
decl_simple_lock_data(,hil_lock)

#if	HIMEM_STATS
int himem_request;	/* number of requests */
int himem_used;		/* number of times used */
#endif	/* HIMEM_STATS */

void
himem_init(
	void)
{
	simple_lock_init(&hil_lock, ETAP_VM_HIMEM);
}

/* 
 * Called by drivers, this indicates himem that this driver might need
 * to allocate as many as npages pages in a single I/O DMA transfer
 */

void
himem_reserve(
	int		npages)
{
	register		i = 0;
	vm_page_t		free_head = VM_PAGE_NULL;
	vm_page_t		low;
	hil_t			hil;
	spl_t			ipl;
	extern vm_offset_t	avail_end;

	if (avail_end <= HIGH_MEM)
		return;
	hil = (hil_t)kalloc(npages*sizeof(struct himem_link));
	if (hil == (hil_t)0) 
		panic("himem_reserve: kalloc failed\n");

	for (i=0; i < npages-1; i++)
		(hil+i)->next = hil+i+1;

	/*
	 * This is the only way of getting low physical pages 
	 * wtithout changing VM internals
	 */
	for (i=0; i != npages;) {
		if ((low = vm_page_grab()) == VM_PAGE_NULL)
			panic("No low memory pages for himem\n");
		vm_page_gobble(low); /* mark as consumed internally */
		if (_high_mem_page(low->phys_addr)) {
			low->pageq.next = (queue_entry_t)free_head;
			free_head = low;
		} else {
			(hil+i)->low_page = low->phys_addr;
			i++;
		}
	}

	for (low = free_head; low; low = free_head) {
		free_head = (vm_page_t) low->pageq.next;
		VM_PAGE_FREE(low);
        }

	ipl = splhi();
	simple_lock(&hil_lock);
	(hil+npages-1)->next = hil_head;
	hil_head = hil;
	simple_unlock(&hil_lock);
	splx(ipl);
}

/*
 * Called by driver at DMA initialization time. Converts a high memory
 * physical page to a low memory one. If operation is a write, 
 * [phys_addr, phys_addr+length-1] is copied to new page. Caller must
 * provide a pointer to a pointer to a himem_list. This is used to store
 * all the conversions and is use at completion time to revert the pages.
 * This pointer must point to a null hil_t value for the call on the first
 * page of a DMA transfer.
 */

vm_offset_t
himem_convert(
	vm_offset_t	phys_addr,
	vm_size_t	length,
	int		io_op,
	hil_t		*hil)
{
	hil_t		h;
	spl_t		ipl;
	vm_offset_t	offset = phys_addr & (I386_PGBYTES - 1);

	assert (offset + length <= I386_PGBYTES);

	ipl = splhi();
	simple_lock(&hil_lock);
	while (!(h = hil_head)) { 
		printf("WARNING: out of HIMEM pages\n");
		thread_sleep_simple_lock((event_t)&hil_head,
					simple_lock_addr(hil_lock), FALSE);
		simple_lock (&hil_lock);
	}
	hil_head = hil_head->next;
	simple_unlock(&hil_lock);
	splx(ipl);
	
	h->high_addr = phys_addr;

	if (io_op == D_WRITE) {
	  bcopy((char *)phystokv(phys_addr), (char *)phystokv(h->low_page + offset),
		length);
	  h->length = 0;
	} else {
	  h->length = length;
	}
	h->offset = offset;

	assert(!*hil || (*hil)->high_addr);

	h->next = *hil;
	*hil = h;
	return(h->low_page + offset);
}

/*
 * Called by driver at DMA completion time. Converts a list of low memory
 * physical page to the original high memory one. If operation was read, 
 * [phys_addr, phys_addr+lenght-1] is copied to original page
 */

void
himem_revert(
	hil_t		hil)
{
	hil_t		next;
	boolean_t	wakeup = FALSE;
	spl_t		ipl;

	while(hil) {
		if (hil->length) {
			bcopy((char *)phystokv(hil->low_page + hil->offset),
				(char *)phystokv(hil->high_addr),
			      hil->length);
		}
		hil->high_addr = 0;
		hil->length = 0;
		hil->offset = 0;
		next = hil->next;
		ipl = splhi();
		simple_lock(&hil_lock);
		if (!(hil->next = hil_head))
			wakeup = TRUE;
		hil_head = hil;
		simple_unlock(&hil_lock);
		splx(ipl);
		hil = next;
	}
	if (wakeup)
		thread_wakeup((event_t)&hil_head);
}
