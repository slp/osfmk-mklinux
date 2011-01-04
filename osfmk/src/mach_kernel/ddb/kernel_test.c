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
 * 
 */
/*
 * MkLinux
 */

/*
 * In-kernel test initialization and interrupt framework
 *
 * This framework is intended to facilitate in-kernel testing by providing
 * initialization and a means to generate interrupt-level processing.  A new
 * test supplies an initialization function which can be called at startup
 * time, or can be triggered at some later time by setting a flag.  The
 * initialization typically will start up a kernel thread that handles the
 * testing.  If interrupt-level processing is needed, the test can supply an
 * interrupt routine which will be called from the clock interrupt whenever
 * the test's interrupt control word is non-zero.
 *
 * This framework is a kernel configuration option (KERNEL_TEST).
 *
 * A kernel test thread is created at startup time.  This thread will call
 * the initialization routines for any tests that are enabled at the time.
 * It also will vm_allocate a large array used to generate a paging load.
 * It will then block until it is awakened either to touch the memory to
 * cause paging, or to check for new initializations.
 *
 * A new test is added by defining an array index and corresponding bit
 * position (e.g., VM_TEST and VM_TEST_BIT) in kern_test_defs.h, and by
 * extending the kern_test_init_func and kern_test_intr_func arrays in
 * this file.  By setting the corresponding bit in kern_test_enable in this
 * file, the test will be initialized at startup.  If the bit isn't set at
 * startup, the test can be enabled later by setting the bit from the
 * debugger.
 */

#include <xpr_debug.h>
#include <cpus.h>
#include <dipc.h>
#include <mach_host.h>
#include <kernel_test.h>
#include <norma_scsi.h>

#include <mach/boolean.h>
#include <mach/machine.h>
#include <mach/task_special_ports.h>
#include <mach/vm_param.h>
#include <mach/mach_server.h>
#include <ipc/ipc_init.h>
#include <kern/cpu_number.h>
#include <kern/processor.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/time_out.h>
#include <kern/timer.h>
#include <kern/zalloc.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <machine/machparam.h>
#include <machine/pmap.h>
#include <sys/version.h>

#include <ddb/kern_test_defs.h>

/*
 * This is the master interrupt enabling switch.  If zero, the kernel_test_intr
 * routine will not be called from hertz_tick() (in kern/mach_clock.c).
 * If any test that is enabled at boot time requires interrupt processing,
 * then this must be non-zero at boot time.
 */
unsigned int kern_test_intr_enable = TRUE;

/*
 * These are individual interrupt controls for each class of tests.  If
 * non-zero, then the corresponding interrupt routine in the array below
 * will be called.
 */
unsigned int kern_test_intr_control[MAX_TEST + 1];

/*
 * Each class of test may optionally supply an interrupt routine.
 */
extern void vm_test_intr(void);
#if	DIPC
extern void dipc_test_intr(void);
#endif	/* DIPC */

void kern_test_init_paging(void);
void kern_test_wakeup(void);
void kernel_test_continue(void);

void (*(kern_test_intr_func[MAX_TEST + 1]))(void) = {
		0,
		vm_test_intr,
		0,
		0,
#if	DIPC
		dipc_test_intr,
#else	/* DIPC */
		0,
#endif	/* DIPC */
		0
	};

/*
 * Each class of test must supply an initialization routine.
 */
extern void vm_test_init(boolean_t);
#if	NORMA_SCSI
extern void scsit_test_init(boolean_t);
#endif	/* NORMA_SCSI */
#if	DIPC
extern void kkt_test_bootstrap(boolean_t);
extern void dipc_test_init(boolean_t);
extern void msg_test_init(boolean_t);
#endif	/* DIPC */

void (*(kern_test_init_func[MAX_TEST + 1]))(boolean_t) = {
		0,
		vm_test_init,
#if	NORMA_SCSI
		scsit_test_init,
#else	/* NORMA_SCSI */
		0,
#endif	/* NORMA_SCSI */
#if	DIPC
		kkt_test_bootstrap,
		msg_test_init,
		dipc_test_init,
#else	/* DIPC */
		0,
		0,
		0,
#endif	/* DIPC */
	};

/*
 *	The following flags are used to effect the resumption of suspended
 *	threads so that the in-kernel tests are serialized and synchronized.
 */

boolean_t	start_kernel_threads_blocked = FALSE;
boolean_t	kernel_test_thread_sync_done = FALSE;
boolean_t	kernel_test_thread_blocked = FALSE;
boolean_t	unit_test_thread_sync_done = FALSE;
boolean_t	unit_test_thread_blocked = FALSE;
boolean_t	unit_test_done = FALSE;

decl_simple_lock_data(, kernel_test_lock)
extern	void 		start_kernel_threads(void);


/*
 * The kern_test_enable bit array is used to turn on and off tests.  If
 * the test is enabled at boot time, then the initialization routine will
 * be called with a TRUE argument.  If the test is enabled later, the
 * initialization routine will be called with a FALSE argument.  In either
 * case, the corresponding bit will be set in kern_test_init_done so that
 * the initialization routine is only called once.
 */
unsigned int kern_test_enable = PAGING_LOAD_BIT | VM_TEST_BIT
#if	NORMA_SCSI
	| SCSIT_TEST_BIT
#endif	/* NORMA_SCSI */
#if	DIPC
	| KKT_TEST_BIT
	| MSG_TEST_BIT
	| DIPC_TEST_BIT
#endif	/* DIPC */
	;

unsigned int kern_test_init_done = 0;

/*
 * The kern_test_mask defines those initialization routines that must be
 * invoked regardless of the node type.
 */
unsigned int kern_test_mask = 0
#if	DIPC
	| KKT_TEST_BIT
	| DIPC_TEST_BIT
#endif	/* DIPC */
	;

/*
 * What follows is a brief sketch of the automated in-kernel testing scheme.
 * The "startup" thread is suspended in startup.c to allow the in-kernel
 * tests (via kernel_test_thread) to run without interference from other
 * boot time activities. The kernel test thread creates a series of threads
 * in turn to run the specified unit tests (e.g., vm, scsi, msg, kkt and dipc)
 * blocking after it creates each thread to allow the thread to initialize or
 * in some cases run a unit test. The kernel test thread is woken up by each
 * unit test thread in turn after that test has been initialized or run.
 * After the last unit test has been run, the startup thread is resumed to
 * complete the boot process. It is worth pointing out that if automated 
 * testing is enabled and the message tests as well as the dipc tests are
 * also enabled, then the message tests first get run as an intra-node test
 * (as part of the dipc loopback test suite) and then again as an inter-node
 * test as part of the dipc multinode tests. It is thus necessary to
 * synchronize the execution of the dipc tests (loopback and multinode)
 * relative to the execution of the startup thread and also synchronize the 
 * execution of the message tests relative to the other dipc loopback and
 * multinode tests. This  results in a triple-tiered synchronization scheme 
 * (in places) requiring the use of three pairs of synchronization flags.
 * The actual  synchronization scheme used is comprehensive as it not only
 * takes into account the possibility that the new thread might complete its
 * assigned task before the calling thread has had a chance to get to the code
 * that would  cause it to wait, but it also optimizes out the pointless wakeup
 * event. It also ensures that the calling thread is not resumed prematurely
 * by spurious wakeups as could happen, for example in implementations where
 * the sizeof(event_t) < sizeof (variable used to post wakeup event). 
 */

/*
 * The paging load test will simply run through a vm_allocated array,
 * touching pages.  To be effective, the region should be close to the
 * size of physical memory, so the KERNEL_TEST_PAGES constant below should
 * be set accordingly.
 */
#define KERNEL_TEST_PAGES	4096
int kern_test_paging_load_pages = KERNEL_TEST_PAGES;
int kern_test_paging_load_size;
vm_offset_t kern_test_paging_load_addr;

/* initialize the paging load test */
void
kern_test_init_paging(void)
{
	/* allocate some pageable memory */
	kern_test_paging_load_size = kern_test_paging_load_pages * page_size;
	vm_allocate(kernel_map, &kern_test_paging_load_addr,
					kern_test_paging_load_size, TRUE);
}

/*
 * Start up the paging load.  The kernel_test_thread is responsible for
 * touching the pages and thereby generating the paging load.  This routine
 * may be called from the debugger or from other kernel tests.
 */
void
kern_test_start_paging(void)
{
	kern_test_enable |= PAGING_LOAD_BIT;
	kern_test_intr_control[PAGING_LOAD] = 1;
	thread_wakeup((event_t)&kern_test_enable);
}

/*
 * Stop the paging load.  This routine may be called from the debugger or
 * from other kernel tests.
 */
void
kern_test_stop_paging(void)
{
	kern_test_intr_control[PAGING_LOAD] = 0;
}

/*
 * Wake up the kernel test thread.  Primarily intended for calling from the
 * debugger.
 */
void
kern_test_wakeup(void)
{
	thread_wakeup((event_t)&kern_test_enable);
}

/*
 * This is the kernel test interrupt routine.  It is called from hertz_tick()
 * if kern_test_intr_enable is non-zero.  It checks the kern_test_intr_control
 * array and calls the interrupt routine for any tests which have interrupts
 * enabled.
 */
int kern_test_misses = 0;

void
kernel_test_intr(void)
{
	int i;

	for (i = 1; i <= MAX_TEST; i++) {
		if (kern_test_intr_control[i]) {
			if (!kern_test_intr_func[i]) {
				kern_test_misses++;
				kern_test_intr_control[i] = 0;
			} else
				(*(kern_test_intr_func[i]))();
		}
	}
}

/*
 * kernel_test_continue is the main loop of the kernel test thread.  It
 * will call the initialization routine for any newly enabled test, and it
 * will generate a paging load if the paging load has been started.  While
 * generating a paging load, it will check for newly enabled tests; otherwise,
 * a kern_test_wakeup() must be issued to cause a new test to be initialized.
 */
void
kernel_test_continue(void)
{
	int i;

	while (1) {
initialize:
		if ((kern_test_enable & PAGING_LOAD_BIT) &&
		    !(kern_test_init_done & PAGING_LOAD_BIT)) {
			kern_test_init_paging();
			kern_test_init_done |= PAGING_LOAD_BIT;
		}
		for (i = 1; i <= MAX_TEST; i++) {
			if ((kern_test_enable & (1<<i)) &&
			    !(kern_test_init_done & (1<<i)) &&
			    kern_test_init_func[i]) {
				(*(kern_test_init_func[i]))(FALSE);
				kern_test_init_done |= 1 << i;
			}
		}
		while (kern_test_intr_control[PAGING_LOAD]) {
			/* touch all the pages */
			for (i = 0; i < kern_test_paging_load_size; i += page_size) {
				*(int *)(kern_test_paging_load_addr + i) = i + 1;
				if ((kern_test_init_done ^ kern_test_enable) &
				    kern_test_enable)
					goto initialize;
				if (!kern_test_intr_control[PAGING_LOAD])
					break;
			}
		}
		assert_wait((event_t)&kern_test_enable, FALSE);
		thread_block((void (*)(void)) 0);
	}
}

/*
 * The kernel_test_thread is created by startup().  It has two jobs: to
 * initialize threads for individual kernel tests, and to create a paging
 * load if and when asked.
 */
void
kernel_test_thread(void)
{
	int		i;
	spl_t		s;
	boolean_t	boot_node;

	boot_node = (boolean_t)current_thread()->ith_other;

	/* this is boot-time initialization (func gets TRUE arg) */
	for (i = 1; i <= MAX_TEST; i++) {
		if ((kern_test_enable & (1<<i)) && kern_test_init_func[i]) {
			(*(kern_test_init_func[i]))(TRUE);
			kern_test_init_done |= 1 << i;
		}
	}

	/*
	 *	Resume the kernel thread suspended in start_kernel_threads()
	 *	to continue the boot up process.
	 */

	s = splsched();
	simple_lock(&kernel_test_lock);
	kernel_test_thread_sync_done = TRUE;
	if(start_kernel_threads_blocked)
		thread_wakeup((event_t)&start_kernel_threads);
	simple_unlock(&kernel_test_lock);
	splx(s);

	/*
	 * If we are not on the boot node, just sleep unless someone
	 * wants to set things running.
	 */
	if (!boot_node) {
		assert_wait((event_t)&kern_test_enable, FALSE);
		thread_block((void (*)(void)) 0);
	}
	kernel_test_continue();
}
