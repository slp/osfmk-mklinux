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
 *	vm_test.c	in-kernel tests for the vm system
 */

#include <kernel_test.h>

#if	KERNEL_TEST

#include <ddb/kern_test_defs.h>
#include <kern/thread.h>
#include <mach/kern_return.h>
#include <mach/machine/vm_types.h>
#include <mach/mach_server.h>
#include <mach/memory_object.h>
#include <mach/vm_prot.h>
#include <mach/vm_inherit.h>
#include <mach/vm_param.h>
#include <mach_debug/vm_info.h>
#include <mach_debug/page_info.h>
#include <mach_debug/hash_info.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_object.h>
#include <kern/task.h>
#include <kern/host.h>
#include <kern/misc_protos.h>
#include <ipc/ipc_port.h>


#define	VMT_PAGES_DEFAULT	10
vm_offset_t	vmt_addr;
vm_offset_t	vmt_offset;
vm_size_t	vmt_size;
vm_page_t	vmt_pages[VMT_PAGES_DEFAULT];
vm_object_t	vmt_object;
decl_simple_lock_data(, vmt_lock)

#define		vmt_intr_control	kern_test_intr_control[VM_TEST]
int		vmt_thr_control;
int		vmt_delay_ticks = 0;

/*
 *	The following are used to serialize and
 *	synchronize the kernel tests. 
 */

extern	boolean_t	kernel_test_thread_blocked;
extern	boolean_t	unit_test_thread_sync_done;
decl_simple_lock_data(extern, kernel_test_lock)

/* for vmt_intr_control and vmt_thr_control: */
#define TEST_THR_WAKEUP		1
#define BASIC_INTR		3
#define UNPREP			4
#define PRINT			5
#define LIMBO_TEST		6
#define DELAY_PIN		7
#define PIN_TEST		8
#define DELAY_UNPIN		9

int vmt_unprep_count = 0;
int vmt_limbo_count = 0;

extern int	vm_page_limbo_count;

void vm_test_basic_preppin(void);
void vm_test_prep_region(void);
void vm_test_intr(void);
void vm_test_continue(void);
void vm_test_thread(void);
void vm_test_init(boolean_t startup);
void vm_test_run(int testnum);

void
vm_test_basic_preppin(void)
{
	vm_page_t	m, fict_m;
	vm_map_entry_t	entry;
	kern_return_t	kr;
	spl_t		s;

	/* get the first page in the test region */
	vm_map_lock(kernel_map);
	if (!vm_map_lookup_entry(kernel_map, vmt_addr, &entry))
		printf("vm_test_basic_preppin: lookup_entry failed\n");
	vmt_object = entry->object.vm_object;
	vmt_offset = entry->offset + (vmt_addr - entry->vme_start);
	vm_object_reference(vmt_object);
	vm_map_unlock(kernel_map);

	vm_object_lock(vmt_object);
	m = vm_page_lookup(vmt_object, vmt_offset);
	assert(m != VM_PAGE_NULL);
	assert(m->prep_pin_count == 0);

	kr = vm_page_prep(m);
	assert(kr == KERN_SUCCESS && m->prep_count == 1);
	s = splvm();
	kr = vm_page_pin(m);
	assert(kr == KERN_SUCCESS && m->pin_count == 1);
	kr = vm_page_unpin(m);
	assert(kr == KERN_SUCCESS && m->prep_pin_count == 0);
	splx(s);

	kr = vm_page_prep(m);
	assert(kr == KERN_SUCCESS && m->prep_count == 1);
	kr = vm_page_unprep(m);
	assert(kr == KERN_SUCCESS && m->prep_pin_count == 0);

	s = splvm();
	kr = vm_page_pin(m);
	splx(s);
	assert(kr != KERN_SUCCESS && m->prep_pin_count == 0);

	vm_object_unlock(vmt_object);
	printf("vm_test_basic_preppin: PASSED\n");
}

void
vm_test_prep_region(void)
{
	int i;
	kern_return_t kr;
	vm_page_t m;

	/* fault all the pages in */
	for (i = 0; i < vmt_size; i += page_size) {
		*(int *)(vmt_addr + i) = i + 1;
	}

	/* get the pages */
	vm_object_lock(vmt_object);
	for (i = 0; i < VMT_PAGES_DEFAULT; i++) {
		m = vm_page_lookup(vmt_object, (i*page_size) + vmt_offset);
		assert(m != VM_PAGE_NULL);
		vmt_pages[i] = m;
		assert(m->prep_pin_count == 0);
		kr = vm_page_prep(m);
		assert(kr == KERN_SUCCESS && m->prep_count == 1);
	}
/*
	assert(vmt_object->resident_page_count == VMT_PAGES_DEFAULT);
*/
	vm_object_unlock(vmt_object);
}

/*
 * Called at splclock ==> no need for splvm around vm_page[un]pin()
 */
void
vm_test_intr(void)
{
	int		i;
	kern_return_t	kr;
	spl_t 		s;

	simple_lock(&vmt_lock);
	switch (vmt_intr_control) {
	case TEST_THR_WAKEUP:
		vmt_intr_control = 0;
		if (vmt_thr_control)
			thread_wakeup((event_t)&vmt_thr_control);
		break;
	case DELAY_PIN:
		if (--vmt_delay_ticks == 0) {
			vmt_intr_control = 0;
			vmt_limbo_count = 0;
			simple_unlock(&vmt_lock);
			for (i = 0; i < VMT_PAGES_DEFAULT; i++) {
				assert(vmt_pages[i] != VM_PAGE_NULL);
				kr = vm_page_pin(vmt_pages[i]);
				if (kr == KERN_SUCCESS) {
					assert(vmt_pages[i]->prep_count == 1);
					assert(vmt_pages[i]->pin_count == 1);
					assert(!vmt_pages[i]->limbo);
					assert(!vmt_pages[i]->fictitious);
					kr = vm_page_unpin(vmt_pages[i]);
					assert(kr == KERN_SUCCESS);
					assert(vmt_pages[i]->prep_pin_count == 0);
					vmt_pages[i] = VM_PAGE_NULL;
				} else {
					assert(vmt_pages[i]->prep_count == 1);
					assert(vmt_pages[i]->pin_count == 0);
					assert(vmt_pages[i]->limbo);
					vmt_limbo_count++;
				}
			}
			assert(vm_page_limbo_count >= vmt_limbo_count);
			simple_lock(&vmt_lock);
			vmt_thr_control = UNPREP;
			thread_wakeup((event_t)&vmt_thr_control);
		}
		break;
	case DELAY_UNPIN:
		if (--vmt_delay_ticks == 0) {
			vmt_intr_control = 0;
			simple_unlock(&vmt_lock);
			for (i = 0; i < VMT_PAGES_DEFAULT; i++) {
				assert(vmt_pages[i] != VM_PAGE_NULL);
				assert(!vmt_pages[i]->limbo);
				assert(!vmt_pages[i]->fictitious);
				kr = vm_page_unpin(vmt_pages[i]);
				assert(kr == KERN_SUCCESS);
				assert(vmt_pages[i]->prep_pin_count == 0);
				vmt_pages[i] = VM_PAGE_NULL;
			}
			simple_lock(&vmt_lock);
			kern_test_stop_paging();
			vmt_thr_control = PRINT;
			thread_wakeup((event_t)&vmt_thr_control);
		}
		break;
	case BASIC_INTR:
		vmt_intr_control = 0;
		simple_unlock(&vmt_lock);
		for (i = 0; i < VMT_PAGES_DEFAULT; i++) {
			assert(vmt_pages[i] != VM_PAGE_NULL);
			kr = vm_page_pin(vmt_pages[i]);
			assert(kr == KERN_SUCCESS);
			assert(vmt_pages[i]->prep_count == 1);
			assert(vmt_pages[i]->pin_count == 1);
		}
		for (i = 0; i < VMT_PAGES_DEFAULT; i++) {
			kr = vm_page_unpin(vmt_pages[i]);
			assert(kr == KERN_SUCCESS);
			assert(vmt_pages[i]->prep_pin_count == 0);
			vmt_pages[i] = VM_PAGE_NULL;
		}
		simple_lock(&vmt_lock);
		vmt_thr_control = PRINT;
		thread_wakeup((event_t)&vmt_thr_control);
		break;
	}
	simple_unlock(&vmt_lock);
}

void
vm_test_continue(void)
{
	int		i;
	spl_t		s;
	kern_return_t 	kr;

	for (;;) {
		s = splclock();
		simple_lock(&vmt_lock);
		switch (vmt_thr_control) {
		case BASIC_INTR:
			simple_unlock(&vmt_lock);
			splx(s);
			vm_test_prep_region();
			s = splclock();
			simple_lock(&vmt_lock);
			vmt_intr_control = BASIC_INTR;
			vmt_thr_control = 0;
			break;
		case LIMBO_TEST:
			simple_unlock(&vmt_lock);
			splx(s);
			vm_test_prep_region();
			s = splclock();
			simple_lock(&vmt_lock);
			vmt_delay_ticks = 10000;
			vmt_intr_control = DELAY_PIN;
			vmt_thr_control = 0;
			kern_test_start_paging();
			break;
		case PIN_TEST:
			simple_unlock(&vmt_lock);
			splx(s);
			vm_test_prep_region();
			for (i = 0; i < VMT_PAGES_DEFAULT; i++) {
				assert(vmt_pages[i] != VM_PAGE_NULL);
				s = splvm();
				kr = vm_page_pin(vmt_pages[i]);
				splx(s);
				assert(kr == KERN_SUCCESS);
				assert(vmt_pages[i]->prep_count == 1);
				assert(vmt_pages[i]->pin_count == 1);
			}
			s = splclock();
			simple_lock(&vmt_lock);
			vmt_delay_ticks = 10000;
			vmt_intr_control = DELAY_UNPIN;
			vmt_thr_control = 0;
			kern_test_start_paging();
			break;
		case UNPREP:
			simple_unlock(&vmt_lock);
			splx(s);
			vmt_unprep_count = 0;
			for (i = 0; i < VMT_PAGES_DEFAULT; i++) {
				if (vmt_pages[i] != VM_PAGE_NULL) {
					kr = vm_page_unprep(vmt_pages[i]);
					assert(kr == KERN_SUCCESS);
					assert(vmt_pages[i]->prep_pin_count == 0);
					vmt_pages[i] = VM_PAGE_NULL;
					vmt_unprep_count++;
				}
			}
			s = splclock();
			simple_lock(&vmt_lock);
			if (vmt_limbo_count)
				assert(vmt_unprep_count == vmt_limbo_count);
			printf("vm_test_thread: unprep %d, limbo %d\n",
					vmt_unprep_count, vmt_limbo_count);
			vmt_unprep_count = vmt_limbo_count = 0;
			kern_test_stop_paging();
			/* fall through */
		case PRINT:
			printf("vm_test_thread: test done\n");
			vmt_thr_control = 0;
			/*
			 *	Wake up the thread we suspended in
			 *	vm_test_init()
			 */
		
			s = splsched();
			simple_lock(&kernel_test_lock);
			unit_test_thread_sync_done = TRUE;
			if(kernel_test_thread_blocked)
				thread_wakeup((event_t)&kernel_test_thread);
			simple_unlock(&kernel_test_lock);
			splx(s);
			break;
		}
		assert_wait((event_t)&vmt_thr_control, FALSE);
		simple_unlock(&vmt_lock);
		splx(s);
		thread_block(vm_test_continue);
	}
}

void
vm_test_thread(void)
{
	kern_return_t kr;
	int i;

	simple_lock_init(&vmt_lock, ETAP_VM_TEST);

	/* get some memory */
	vmt_size = page_size * VMT_PAGES_DEFAULT;
	kr = vm_allocate(kernel_map, &vmt_addr, vmt_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("vm_test_thread: cannot vm_allocate, kr=%d, size=0x%x\n",
			kr, vmt_size);
		thread_terminate_self();
	}

	/* prevent paging for now */
	kr = vm_map_wire(kernel_map, vmt_addr, vmt_addr + vmt_size,
					VM_PROT_READ|VM_PROT_WRITE, FALSE);
	assert(kr == KERN_SUCCESS);

	/* fault all the pages in */
	for (i = 0; i < vmt_size; i += page_size) {
		*(int *)(vmt_addr + i) = i + 1;
	}

	kern_test_intr_enable++;	/* make sure test interrupts are on */

	vm_test_basic_preppin();

	/* make the test region pageable */
	kr = vm_map_unwire(kernel_map, vmt_addr, vmt_addr + vmt_size, FALSE);

#if	PARAGON860
	{
		char		*s;
		unsigned int	firstnode;

		/*
		 *	Only start up loopback tests on boot node.
		 */
		if ((s = (char *) getbootenv("BOOT_FIRST_NODE")) == 0)
			panic("vm_test_thread");
		firstnode = atoi(s);
		if (dipc_node_self() == (node_name) firstnode)
			vmt_thr_control = BASIC_INTR;
		else
			vmt_thr_control = PRINT;
	}
#else	/* PARAGON860 */
	vmt_thr_control = BASIC_INTR;
#endif	/* PARAGON860 */
	vmt_intr_control = 0;
	vm_test_continue();
}

void
vm_test_init(boolean_t startup)
{
	spl_t s;

	/* the initial part of this test needs to be called at startup */
	assert(startup == TRUE);

	(void) kernel_thread(kernel_task, vm_test_thread, (char *) 0);

	/*
	 *	Suspend this thread until "vm_test_thread" finishes.
	 *	The synchronization scheme uses a simple lock, two
	 *	booleans and the wakeup event.The wakeup event will
	 *	be posted by vm_test_continue().
	 */
	s = splsched();
	simple_lock(&kernel_test_lock);
	while(!unit_test_thread_sync_done){
		assert_wait((event_t) &kernel_test_thread, FALSE);
		kernel_test_thread_blocked = TRUE;
		simple_unlock(&kernel_test_lock);
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splsched();
		simple_lock(&kernel_test_lock);
		kernel_test_thread_blocked = FALSE;
	}
	unit_test_thread_sync_done = FALSE; /* Reset for next use */
	simple_unlock(&kernel_test_lock);
	splx(s);
}

#define	RUN_LIMBO_TEST	1
#define RUN_PIN_TEST	2

/* debugger (user) interface */
void
vm_test_run(int testnum)
{
	switch (testnum) {
	  case RUN_LIMBO_TEST:
		vmt_thr_control = LIMBO_TEST;
		vmt_intr_control = TEST_THR_WAKEUP;
		break;
	  case RUN_PIN_TEST:
		vmt_thr_control = PIN_TEST;
		vmt_intr_control = TEST_THR_WAKEUP;
		break;
	  default:
		printf("vm_test_run: invalid test.  Try:\n");
		printf("        LIMBO_TEST\t%d\n", RUN_LIMBO_TEST);
		printf("        PIN_TEST\t%d\n", RUN_PIN_TEST);
		break;
	}
}

#endif	/* KERNEL_TEST */
