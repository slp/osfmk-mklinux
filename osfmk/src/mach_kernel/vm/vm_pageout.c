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
/* CMU_HIST */
/*
 * Revision 2.21.5.3  92/09/15  17:37:26  jeffreyh
 * 	Add a few more pages to the reserved pool.
 * 	[92/08/28            jeffreyh]
 * 
 * 	Discard error pages if encountered (just like absent).
 * 	[92/08/21            dlb]
 * 
 * 	With Jeffreyh:  Surgery on the suspend/resume protocol demands
 * 	that we eliminate older wakeups of the kserver_pageout_support
 * 	thread.
 * 	[92/08/20            alanl]
 * 
 * Revision 2.21.5.2  92/06/24  18:04:11  jeffreyh
 * 	Wakeup the pagout_support_thread when there is enough memory
 * 	available to continue receiving kmsgs without filtering.
 * 	[92/06/10            jeffreyh]
 * 
 * 	Added netipc_able_continue_mgs() under NORMA_VM conditional. It
 * 	 is used to judge if a suspended norma message can be restarted.
 * 	[92/06/03            jeffreyh]
 * 
 * Revision 2.21.5.1  92/05/29  11:49:15  jeffreyh
 * 	NORMA only:  drastically increase the size
 * 	of the reserved VM pool to account for
 * 	deficiencies in the NORMA pageout path.
 * 	[92/05/07            alanl]
 * 
 * Revision 2.21  91/12/11  08:44:16  jsb
 * 	Added vm_pageout_active, vm_pageout_inactive,
 * 	and other measurement counters.  Fixed the log.
 * 	[91/11/24            rpd]
 * 
 * Revision 2.20  91/10/09  16:20:36  af
 * 	Added vm_pageout_pause_count, vm_pageout_pause_max technology
 * 	so that vm_pageout_burst_wait can decrease as well as increase.
 * 	[91/10/04            rpd]
 * 
 * Revision 2.19  91/08/28  11:18:54  jsb
 * 	Fixed vm_pageout_scan to send pages to the default pager
 * 	when memory gets very tight.  This is the same idea as the old
 * 	vm_pageout_reserved_external and vm_pageout_reserved_internal,
 * 	but with a different implementation that forcibly double-pages.
 * 	[91/08/07            rpd]
 * 	Precious page support: return precious pages on pageout, use
 * 	memory_object_data_return instead of memory_object_data_write
 * 	when appropriate,
 * 	[91/07/03  14:20:57  dlb]
 * 
 * Revision 2.18  91/07/01  08:28:13  jsb
 * 	Add missing includes of vm/vm_map.h and kern/thread.h.
 * 	[91/06/29  16:53:36  jsb]
 * 
 * Revision 2.17  91/06/17  15:49:37  jsb
 * 	NORMA_VM: declare definitions for memory_object_data_{initialize,write}
 * 	since they won't be provided by interposed-on memory_object_user.h.
 * 	[91/06/17  11:13:22  jsb]
 * 
 * Revision 2.16  91/05/18  14:41:49  rpd
 * 	Fixed vm_pageout_continue to always call vm_pageout_scan.
 * 	Revamped vm_pageout_scan.  Now it recalculates vm_page_inactive_target,
 * 	gradually moves pages from the active list to the inactive list,
 * 	looks at vm_page_free_wanted, handles absent and fictitious pages,
 * 	and blocks with a continuation (vm_pageout_scan_continue), which
 * 	uses vm_page_laundry_count to adjust vm_pageout_burst_wait.
 * 	[91/04/06            rpd]
 * 
 * 	Changed vm_page_free_wanted to unsigned int.
 * 	[91/04/05            rpd]
 * 	Added vm_page_grab_fictitious.
 * 	[91/03/29            rpd]
 * 	Changed vm_page_init.
 * 	[91/03/24            rpd]
 * 
 * Revision 2.15  91/05/14  17:50:59  mrt
 * 	Correcting copyright
 * 
 * Revision 2.14  91/03/16  15:06:50  rpd
 * 	Modified vm_pageout_scan for further vm_page_deactivate changes.
 * 	Also changed it to ignore pages in dead objects.
 * 	[91/03/11            rpd]
 * 	Added vm_pageout_continue.
 * 	[91/01/20            rpd]
 * 
 * Revision 2.13  91/02/05  17:59:57  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:34:17  mrt]
 * 
 * Revision 2.12  91/01/08  16:45:57  rpd
 * 	Added net_kmsg_collect.
 * 	[91/01/05            rpd]
 * 	Added consider_task_collect, consider_thread_collect.
 * 	[91/01/03            rpd]
 * 
 * 	Added stack_collect.
 * 	[90/12/31            rpd]
 * 	Added continuation argument to thread_block.
 * 	[90/12/08            rpd]
 * 
 * 	Ensure that vm_page_free_target is at least five pages
 * 	larger than vm_page_free_min, to avoid vm_page_wait deadlock.
 * 	[90/11/19            rpd]
 * 
 * 	Replaced swapout_threads with consider_zone_gc.
 * 	[90/11/11            rpd]
 * 
 * Revision 2.11  90/11/05  14:35:03  rpd
 * 	Modified vm_pageout_scan for new vm_page_deactivate protocol.
 * 	[90/11/04            rpd]
 * 
 * Revision 2.10  90/10/12  13:06:53  rpd
 * 	Fixed vm_pageout_page to take busy pages.
 * 	[90/10/09            rpd]
 * 
 * 	In vm_pageout_scan, check for new software reference bit
 * 	in addition to using pmap_is_referenced.  Remove busy pages
 * 	found on the active and inactive queues.
 * 	[90/10/08            rpd]
 * 
 * Revision 2.9  90/08/27  22:16:02  dbg
 * 	Fix error in initial assumptions: vm_pageout_setup must take a
 * 	BUSY page, to prevent the page from being scrambled by pagein.
 * 	[90/07/26            dbg]
 * 
 * Revision 2.8  90/06/19  23:03:22  rpd
 * 	Locking fix for vm_pageout_page from dlb/dbg.
 * 	[90/06/11            rpd]
 * 
 * 	Correct initial condition in vm_pageout_page (page is NOT busy).
 * 	Fix documentation for vm_pageout_page and vm_pageout_setup.
 * 	[90/06/05            dbg]
 * 
 * 	Fixed vm_object_unlock type in vm_pageout_page.
 * 	[90/06/04            rpd]
 * 
 * Revision 2.7  90/06/02  15:11:56  rpd
 * 	Removed pageout_task and references to kernel_vm_space.
 * 	[90/04/29            rpd]
 * 
 * 	Made vm_pageout_burst_max, vm_pageout_burst_wait tunable.
 * 	[90/04/18            rpd]
 * 	Converted to new IPC and vm_map_copyin_object.
 * 	[90/03/26  23:18:10  rpd]
 * 
 * Revision 2.6  90/05/29  18:39:52  rwd
 * 	Picked up new vm_pageout_page from dbg.
 * 	[90/05/17            rwd]
 * 	Rfr change to send multiple pages to pager at once.
 * 	[90/04/12  13:49:13  rwd]
 * 
 * Revision 2.5  90/05/03  15:53:21  dbg
 * 	vm_pageout_page flushes page only if asked; otherwise, it copies
 * 	the page.
 * 	[90/03/28            dbg]
 * 
 * 	If an object's pager is not initialized, don't page out to it.
 * 	[90/03/21            dbg]
 * 
 * Revision 2.4  90/02/22  20:06:48  dbg
 * 		PAGE_WAKEUP --> PAGE_WAKEUP_DONE to reflect the fact that it
 * 		clears the busy flag.
 * 		[89/12/13            dlb]
 * 
 * Revision 2.3  90/01/11  11:48:27  dbg
 * 	Pick up recent changes from mainline:
 * 
 * 		Eliminate page lock when writing back a page.
 * 		[89/11/09            mwyoung]
 * 
 * 		Account for paging_offset when setting external page state.
 * 		[89/10/16  15:29:08  af]
 * 
 * 		Improve reserve tuning... it was a little too low.
 * 
 * 		Remove laundry count computations, as the count is never used.
 * 		[89/10/10            mwyoung]
 * 
 * 		Only attempt to collapse if a memory object has not
 * 		been initialized.  Don't bother to PAGE_WAKEUP in
 * 		vm_pageout_scan() before writing back a page -- it
 * 		gets done in vm_pageout_page().
 * 		[89/10/10            mwyoung]
 * 
 * 		Don't reactivate a page when creating a new memory
 * 		object... continue on to page it out immediately.
 * 		[89/09/20            mwyoung]
 * 
 * 		Reverse the sensing of the desperately-short-on-pages tests.
 * 		[89/09/19            mwyoung]
 * 		Check for absent pages before busy pages in vm_pageout_page().
 * 		[89/07/10  00:01:22  mwyoung]
 * 
 * 		Allow dirty pages to be reactivated.
 * 		[89/04/22            mwyoung]
 * 
 * 		Don't clean pages that are absent, in error, or not dirty in
 * 		vm_pageout_page().  These checks were previously issued
 * 		elsewhere.
 * 		[89/04/22            mwyoung]
 * 
 * Revision 2.2  89/09/08  11:28:55  dbg
 * 	Reverse test for internal_only pages.  Old sense left pageout
 * 	daemon spinning.
 * 	[89/08/15            dbg]
 * 
 * 18-Jul-89  David Golub (dbg) at Carnegie-Mellon University
 *	Changes for MACH_KERNEL:
 *	. Removed non-XP code.
 *	Count page wiring when sending page to default pager.
 *	Increase reserved page count.
 *	Make 'internal-only' count LARGER than reserved page count.
 *
 * Revision 2.18  89/06/12  14:53:05  jsb
 * 	Picked up bug fix (missing splimps) from Sequent via dlb.
 * 	[89/06/12  14:39:28  jsb]
 * 
 * Revision 2.17  89/04/18  21:27:08  mwyoung
 * 	Recent history [mwyoung]:
 * 		Keep hint when pages are written out (call
 * 		 vm_external_state_set).
 * 		Use wired-down fictitious page data structure for "holding_page".
 * 	History condensation:
 * 		Avoid flooding memory managers by using timing [mwyoung].
 * 		New pageout daemon for external memory management
 * 		 system [mwyoung].
 * 	[89/04/18            mwyoung]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *	File:	vm/vm_pageout.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	The proverbial page-out daemon.
 */

#include <mach_pagemap.h>
#include <norma_vm.h>
#include <mach_cluster_stats.h>
#include <mach_kdb.h>
#include <dipc.h>
#include <advisory_pageout.h>

#include <mach/mach_types.h>
#include <mach/memory_object.h>
#include <mach/memory_object_default.h>
#include <mach/memory_object_user.h>
#include <mach/mach_host_server.h>
#include <mach/vm_param.h>
#include <mach/vm_statistics.h>
#include <kern/host_statistics.h>
#include <kern/counters.h>
#include <kern/ipc_sched.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/xpr.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <machine/vm_tuning.h>
#if	NORMA_VM
#include <kern/ipc_sched.h>		/* for convert_ipc_timeout_to_ticks */
#include <xmm/xmm_svm.h>		/* for NORMA_VM_PRECIOUS_RETURN_HACK */
#include <xmm/xmm_server_rename.h>	/* for memory_object_data_return */
#endif	/* NORMA_VM */
#include <kern/misc_protos.h>
#include <device/net_io.h>

#ifndef	VM_PAGE_LAUNDRY_MAX
#define	VM_PAGE_LAUNDRY_MAX	10	/* outstanding DMM page cleans */
#endif	/* VM_PAGEOUT_LAUNDRY_MAX */

#ifndef	VM_PAGEOUT_BURST_MAX
#define	VM_PAGEOUT_BURST_MAX	10	/* simultaneous EMM page cleans */
#endif	/* VM_PAGEOUT_BURST_MAX */

#ifndef	VM_PAGEOUT_DISCARD_MAX
#define	VM_PAGEOUT_DISCARD_MAX	68	/* simultaneous EMM page cleans */
#endif	/* VM_PAGEOUT_DISCARD_MAX */

#ifndef	VM_PAGEOUT_BURST_WAIT
#define	VM_PAGEOUT_BURST_WAIT	30	/* milliseconds per page */
#endif	/* VM_PAGEOUT_BURST_WAIT */

#ifndef	VM_PAGEOUT_EMPTY_WAIT
#define VM_PAGEOUT_EMPTY_WAIT	200	/* milliseconds */
#endif	/* VM_PAGEOUT_EMPTY_WAIT */

/*
 *	To obtain a reasonable LRU approximation, the inactive queue
 *	needs to be large enough to give pages on it a chance to be
 *	referenced a second time.  This macro defines the fraction
 *	of active+inactive pages that should be inactive.
 *	The pageout daemon uses it to update vm_page_inactive_target.
 *
 *	If vm_page_free_count falls below vm_page_free_target and
 *	vm_page_inactive_count is below vm_page_inactive_target,
 *	then the pageout daemon starts running.
 */

#ifndef	VM_PAGE_INACTIVE_TARGET
#define	VM_PAGE_INACTIVE_TARGET(avail)	((avail) * 2 / 3)
#endif	/* VM_PAGE_INACTIVE_TARGET */

/*
 *	Once the pageout daemon starts running, it keeps going
 *	until vm_page_free_count meets or exceeds vm_page_free_target.
 */

#ifndef	VM_PAGE_FREE_TARGET
#define	VM_PAGE_FREE_TARGET(free)	(15 + (free) / 80)
#endif	/* VM_PAGE_FREE_TARGET */

/*
 *	The pageout daemon always starts running once vm_page_free_count
 *	falls below vm_page_free_min.
 */

#ifndef	VM_PAGE_FREE_MIN
#define	VM_PAGE_FREE_MIN(free)	(10 + (free) / 100)
#endif	/* VM_PAGE_FREE_MIN */

/*
 *	When vm_page_free_count falls below vm_page_free_reserved,
 *	only vm-privileged threads can allocate pages.  vm-privilege
 *	allows the pageout daemon and default pager (and any other
 *	associated threads needed for default pageout) to continue
 *	operation by dipping into the reserved pool of pages.
 */

#ifndef	VM_PAGE_FREE_RESERVED
#if	NORMA_VM
/*
 *	In the current implementation of NORMA_VM on top of
 *	DIPC, pageouts are freed as soon as the messages
 *	containing them flow off-node.  Threads critical to
 *	the pageout sequence have wired stacks.  It is not
 *	at all clear how much space must be reserved in the
 *	reserved pool.  For now, we revert to using the
 *	non-NORMA_VM formula.
 */
/*XXX double since we ran out with AD2.  MUST FIX XXX*/
#define	VM_PAGE_FREE_RESERVED	(30 + NCPUS)
#else	/* NORMA_VM */
#define	VM_PAGE_FREE_RESERVED	\
	((2 * VM_PAGE_LAUNDRY_MAX) + NCPUS)
#endif	/* NORMA_VM */
#endif	/* VM_PAGE_FREE_RESERVED */


/*
 * Forward declarations for internal routines.
 */
extern void vm_pageout_continue(void);
extern void vm_pageout_scan(void);
extern void vm_pageout_throttle(vm_page_t m);
extern vm_page_t vm_pageout_cluster_page(
			vm_object_t	object,
			vm_offset_t	offset,
			boolean_t	precious_clean);

unsigned int vm_pageout_reserved_internal = 0;
unsigned int vm_pageout_reserved_really = 0;

unsigned int vm_page_laundry_max = 0;		/* # of clusters outstanding */
unsigned int vm_page_laundry_min = 0;
unsigned int vm_pageout_burst_max = 0;
unsigned int vm_pageout_burst_wait = 0;		/* milliseconds per page */
unsigned int vm_pageout_empty_wait = 0;		/* milliseconds */
unsigned int vm_pageout_burst_min = 0;
unsigned int vm_pageout_pause_count = 0;
unsigned int vm_pageout_pause_max = 0;

/*
 *	These variables record the pageout daemon's actions:
 *	how many pages it looks at and what happens to those pages.
 *	No locking needed because only one thread modifies the variables.
 */

unsigned int vm_pageout_active = 0;		/* debugging */
unsigned int vm_pageout_inactive = 0;		/* debugging */
unsigned int vm_pageout_inactive_throttled = 0;	/* debugging */
unsigned int vm_pageout_inactive_forced = 0;	/* debugging */
unsigned int vm_pageout_inactive_nolock = 0;	/* debugging */
unsigned int vm_pageout_inactive_avoid = 0;	/* debugging */
unsigned int vm_pageout_inactive_busy = 0;	/* debugging */
unsigned int vm_pageout_inactive_absent = 0;	/* debugging */
unsigned int vm_pageout_inactive_used = 0;	/* debugging */
unsigned int vm_pageout_inactive_clean = 0;	/* debugging */
unsigned int vm_pageout_inactive_dirty = 0;	/* debugging */
unsigned int vm_pageout_dirty_no_pager = 0;	/* debugging */
unsigned int vm_pageout_inactive_pinned = 0;	/* debugging */
unsigned int vm_pageout_inactive_limbo = 0;	/* debugging */
unsigned int vm_pageout_setup_limbo = 0;	/* debugging */
unsigned int vm_pageout_setup_unprepped = 0;	/* debugging */
unsigned int vm_stat_discard = 0;		/* debugging */
unsigned int vm_stat_discard_sent = 0;		/* debugging */
unsigned int vm_stat_discard_failure = 0;	/* debugging */
unsigned int vm_stat_discard_throttle = 0;	/* debugging */
unsigned int vm_pageout_scan_active_emm_throttle = 0;		/* debugging */
unsigned int vm_pageout_scan_active_emm_throttle_success = 0;	/* debugging */
unsigned int vm_pageout_scan_active_emm_throttle_failure = 0;	/* debugging */
unsigned int vm_pageout_scan_inactive_emm_throttle = 0;		/* debugging */
unsigned int vm_pageout_scan_inactive_emm_throttle_success = 0;	/* debugging */
unsigned int vm_pageout_scan_inactive_emm_throttle_failure = 0;	/* debugging */


unsigned int vm_pageout_out_of_line  = 0;
unsigned int vm_pageout_in_place  = 0;
/*
 *	Routine:	vm_pageout_object_allocate
 *	Purpose:
 *		Allocate an object for use as out-of-line memory in a
 *		data_return/data_initialize message.
 *		The page must be in an unlocked object.
 *
 *		If the page belongs to a trusted pager, cleaning in place
 *		will be used, which utilizes a special "pageout object"
 *		containing private alias pages for the real page frames.
 *		Untrusted pagers use normal out-of-line memory.
 */
vm_object_t
vm_pageout_object_allocate(
	vm_page_t	m,
	vm_size_t	size,
	vm_offset_t	offset)
{
	vm_object_t	object = m->object;
	vm_object_t 	new_object;

	assert(object->pager_ready);

	if (object->pager_trusted || object->internal)
		vm_pageout_throttle(m);

	new_object = vm_object_allocate(size);

	if (object->pager_trusted) {
		assert (offset < object->size);

		vm_object_lock(new_object);
		new_object->pageout = TRUE;
		new_object->shadow = object;
		new_object->can_persist = FALSE;
		new_object->copy_strategy = MEMORY_OBJECT_COPY_NONE;
		new_object->shadow_offset = offset;
		vm_object_unlock(new_object);

		/*
		 * Take a paging reference on the object. This will be dropped
		 * in vm_pageout_object_terminate()
		 */
		vm_object_lock(object);
		vm_object_paging_begin(object);
		vm_object_unlock(object);

		vm_pageout_in_place++;
	} else
		vm_pageout_out_of_line++;
	return(new_object);
}

#if MACH_CLUSTER_STATS
unsigned long vm_pageout_cluster_dirtied = 0;
unsigned long vm_pageout_cluster_cleaned = 0;
unsigned long vm_pageout_cluster_collisions = 0;
unsigned long vm_pageout_cluster_clusters = 0;
unsigned long vm_pageout_cluster_conversions = 0;
unsigned long vm_pageout_target_collisions = 0;
unsigned long vm_pageout_target_page_dirtied = 0;
unsigned long vm_pageout_target_page_freed = 0;
unsigned long vm_pageout_target_page_pinned = 0;
unsigned long vm_pageout_target_page_limbo = 0;
#define CLUSTER_STAT(clause)	clause
#else	/* MACH_CLUSTER_STATS */
#define CLUSTER_STAT(clause)
#endif	/* MACH_CLUSTER_STATS */

/* 
 *	Routine:	vm_pageout_object_terminate
 *	Purpose:
 *		Destroy the pageout_object allocated by
 *		vm_pageout_object_allocate(), and perform all of the
 *		required cleanup actions.
 * 
 *	In/Out conditions:
 *		The object must be locked, and will be returned locked.
 */
void
vm_pageout_object_terminate(
	vm_object_t	object)
{
	vm_object_t	shadow_object;

	/*
	 * Deal with the deallocation (last reference) of a pageout object
	 * (used for cleaning-in-place) by dropping the paging references/
	 * freeing pages in the original object.
	 */

	assert(object->pageout);
	shadow_object = object->shadow;
	vm_object_lock(shadow_object);

	while (!queue_empty(&object->memq)) {
		vm_page_t p, m;
		vm_offset_t offset;

		p = (vm_page_t) queue_first(&object->memq);

		assert(p->private);
		assert(p->pageout);
		p->pageout = FALSE;
		assert(!p->cleaning);

		offset = p->offset;
		VM_PAGE_FREE(p);
		p = VM_PAGE_NULL;

		m = vm_page_lookup(shadow_object,
			offset + object->shadow_offset);

		assert(m != VM_PAGE_NULL);
		assert(m->cleaning);
#if	NORMA_VM_PRECIOUS_RETURN_HACK
		assert(!m->precious || !m->pageout);
#else	/* NORMA_VM_PRECIOUS_RETURN_HACK */
		assert(!m->precious);
#endif	/* NORMA_VM_PRECIOUS_RETURN_HACK */
		m->cleaning = FALSE;

		/*
		 * Account for the paging reference taken when
		 * m->cleaning was set on this page.
		 */
		vm_object_paging_end(shadow_object);
		assert(m->dirty);

		/*
		 * Handle the trusted pager throttle.
		 */
		vm_page_lock_queues();
		if (m->laundry) {
		    vm_page_laundry_count--;
		    m->laundry = FALSE;
		    if (vm_page_laundry_count < vm_page_laundry_min) {
			vm_page_laundry_min = 0;
			thread_wakeup((event_t) &vm_page_laundry_count);
		    }
		}

		/*
		 * Handle the "target" page(s). These pages are to be freed if
		 * successfully cleaned. Target pages are always busy, and are
		 * wired exactly once. The initial target pages are not mapped,
		 * (so cannot be referenced or modified) but converted target
		 * pages may have been modified between the selection as an
		 * adjacent page and conversion to a target.
		 */
		if (m->pageout) {
			assert(m->busy);
			assert(m->wire_count == 1);
			m->pageout = FALSE;
#if MACH_CLUSTER_STATS
			if (m->wanted) vm_pageout_target_collisions++;
#endif
			/*
			 * Revoke all access to the page. Since the object is
			 * locked, and the page is busy, this prevents the page
			 * from being dirtied after the pmap_is_modified() call
			 * returns.
			 */
			pmap_page_protect(m->phys_addr, VM_PROT_NONE);

			/*
			 * Since the page is left "dirty" but "not modifed", we
			 * can detect whether the page was redirtied during
			 * pageout by checking the modify state.
			 */
			m->dirty = pmap_is_modified(m->phys_addr);

			if (m->dirty) {
#if	NORMA_VM_PRECIOUS_RETURN_HACK
				/*
				 *	This situation only occurs if a
				 *	page was converted to be a target
				 *	during the pageout, something which
				 *	should not occur with the precious
				 *	return hack.
				 */
				panic("vm_pageout_object_terminate:  ouch!");
#endif	/* NORMA_VM_PRECIOUS_RETURN_HACK */
				CLUSTER_STAT(vm_pageout_target_page_dirtied++;)
				vm_page_unwire(m);/* reactivates */
				VM_STAT(reactivations++);
				PAGE_WAKEUP_DONE(m);
			} else if (m->prep_pin_count != 0) {
				int s;

#if	NORMA_VM_PRECIOUS_RETURN_HACK
				/*
				 *	This situation only occurs if a
				 *	page was converted to be a target
				 *	during the pageout, something which
				 *	should not occur with the precious
				 *	return hack.
				 */
				panic("vm_pageout_object_terminate:  ouch2!");
#endif	/* NORMA_VM_PRECIOUS_RETURN_HACK */
				s = splvm();
				vm_page_pin_lock();
				if (m->pin_count != 0) {
					/* page is pinned; reactivate */
					CLUSTER_STAT(
					    vm_pageout_target_page_pinned++;)
					vm_page_unwire(m);/* reactivates */
					VM_STAT(reactivations++);
					PAGE_WAKEUP_DONE(m);
				} else {
					/*
					 * page is prepped but not pinned; send
					 * it into limbo.  Note that
					 * vm_page_free (which will be called
					 * after releasing the pin lock) knows
					 * how to handle a page with limbo set.
					 */
					m->limbo = TRUE;
					CLUSTER_STAT(
					    vm_pageout_target_page_limbo++;)
				}
				vm_page_pin_unlock();
				splx(s);
				if (m->limbo)
					vm_page_free(m);
			} else {
				CLUSTER_STAT(vm_pageout_target_page_freed++;)
				vm_page_free(m);/* clears busy, etc. */
			}
			vm_page_unlock_queues();
			continue;
		}
		/*
		 * Handle the "adjacent" pages. These pages were cleaned in
		 * place, and should be left alone.
		 * If prep_pin_count is nonzero, then someone is using the
		 * page, so make it active.
		 */
		if (!m->active && !m->inactive) {
			if (m->reference || m->prep_pin_count != 0)
				vm_page_activate(m);
			else
				vm_page_deactivate(m);
		}
		/*
		 * Set the dirty state according to whether or not the page was
		 * modified during the pageout. Note that we purposefully do
		 * NOT call pmap_clear_modify since the page is still mapped.
		 * If the page were to be dirtied between the 2 calls, this
		 * this fact would be lost. This code is only necessary to
		 * maintain statistics, since the pmap module is always
		 * consulted if m->dirty is false.
		 */
#if MACH_CLUSTER_STATS
		m->dirty = pmap_is_modified(m->phys_addr);

		if (m->dirty)	vm_pageout_cluster_dirtied++;
		else		vm_pageout_cluster_cleaned++;
		if (m->wanted)	vm_pageout_cluster_collisions++;
#else
		m->dirty = 0;
#endif

		/*
		 * Wakeup any thread waiting for the page to be un-cleaning.
		 */
		PAGE_WAKEUP(m);
		vm_page_unlock_queues();
	}
	/*
	 * Account for the paging reference taken in vm_paging_object_allocate.
	 */
	vm_object_paging_end(shadow_object);
	vm_object_unlock(shadow_object);

	assert(object->ref_count == 0);
	assert(object->paging_in_progress == 0);
	assert(object->resident_page_count == 0);
	return;
}

/*
 *	Routine:	vm_pageout_setup
 *	Purpose:
 *		Set up a page for pageout (clean & flush).
 *
 *		Move the page to a new object, as part of which it will be
 *		sent to its memory manager in a memory_object_data_write or
 *		memory_object_initialize message.
 *
 *		The "new_object" and "new_offset" arguments
 *		indicate where the page should be moved.
 *
 *	In/Out conditions:
 *		The page in question must not be on any pageout queues,
 *		and must be busy.  The object to which it belongs
 *		must be unlocked, and the caller must hold a paging
 *		reference to it.  The new_object must not be locked.
 *
 *		This routine returns a pointer to a place-holder page,
 *		inserted at the same offset, to block out-of-order
 *		requests for the page.  The place-holder page must
 *		be freed after the data_write or initialize message
 *		has been sent.
 *
 *		The original page is put on a paging queue and marked
 *		not busy on exit.
 */
vm_page_t
vm_pageout_setup(
	register vm_page_t	m,
	register vm_object_t	new_object,
	vm_offset_t		new_offset)
{
	register vm_object_t	old_object = m->object;
	vm_offset_t		paging_offset;
	vm_offset_t		offset;
	register vm_page_t	holding_page;
	register vm_page_t	new_m;
	register vm_page_t	new_page;
	boolean_t		need_to_wire = FALSE;


        XPR(XPR_VM_PAGEOUT,
     "vm_pageout_setup, obj 0x%X off 0x%X page 0x%X new obj 0x%X offset 0x%X\n",
                (integer_t)m->object, (integer_t)m->offset, 
		(integer_t)m, (integer_t)new_object, 
		(integer_t)new_offset);
	assert(m && m->busy && !m->absent && !m->fictitious && !m->error &&
		!m->restart);

	assert(m->dirty || m->precious);

	/*
	 *	Create a place-holder page where the old one was, to prevent
	 *	attempted pageins of this page while we're unlocked.
	 *	If the pageout daemon put this page in limbo and we're not
	 *	going to clean in place, get another fictitious page to
	 *	exchange for it now.
	 */
	VM_PAGE_GRAB_FICTITIOUS(holding_page);

	if (m->limbo)
		VM_PAGE_GRAB_FICTITIOUS(new_page);

	vm_object_lock(old_object);

	offset = m->offset;
	paging_offset = offset + old_object->paging_offset;

	if (old_object->pager_trusted) {
		/*
		 * This pager is trusted, so we can clean this page
		 * in place. Leave it in the old object, and mark it
		 * cleaning & pageout.
		 */
		new_m = holding_page;
		holding_page = VM_PAGE_NULL;

		/*
		 * If the pageout daemon put this page in limbo, exchange the
		 * identities of the limbo page and the new fictitious page,
		 * and continue with the new page, unless the prep count has
		 * gone to zero in the meantime (which means no one is
		 * interested in the page any more).  In that case, just clear
		 * the limbo bit and free the extra fictitious page.
		 */
		if (m->limbo) {
			if (m->prep_pin_count == 0) {
				/* page doesn't have to be in limbo any more */
				m->limbo = FALSE;
				vm_page_free(new_page);
				vm_pageout_setup_unprepped++;
			} else {
				vm_page_lock_queues();
				VM_PAGE_QUEUES_REMOVE(m);
				vm_page_remove(m);
				vm_page_limbo_exchange(m, new_page);
				vm_pageout_setup_limbo++;
				vm_page_release_limbo(m);
				m = new_page;
				vm_page_insert(m, old_object, offset);
				vm_page_unlock_queues();
			}
		}

		/*
		 * Set up new page to be private shadow of real page.
		 */
		new_m->phys_addr = m->phys_addr;
		new_m->fictitious = FALSE;
		new_m->private = TRUE;
		new_m->pageout = TRUE;

		/*
		 * Mark real page as cleaning (indicating that we hold a
		 * paging reference to be released via m_o_d_r_c) and
		 * pageout (indicating that the page should be freed
		 * when the pageout completes).
		 */
		pmap_clear_modify(m->phys_addr);
		vm_page_lock_queues();
		vm_page_wire(new_m);
		m->cleaning = TRUE;
		m->pageout = TRUE;

		vm_page_wire(m);
		assert(m->wire_count == 1);
		vm_page_unlock_queues();

		m->dirty = TRUE;
		m->precious = FALSE;
		m->page_lock = VM_PROT_NONE;
		m->unusual = FALSE;
		m->unlock_request = VM_PROT_NONE;
	} else {
		/*
		 * Cannot clean in place, so rip the old page out of the
		 * object, and stick the holding page in. Set new_m to the
		 * page in the new object.
		 */
		vm_page_lock_queues();
		VM_PAGE_QUEUES_REMOVE(m);
		vm_page_remove(m);

		/*
		 * If the pageout daemon put this page in limbo, exchange the
		 * identities of the limbo page and the new fictitious page,
		 * and continue with the new page, unless the prep count has
		 * gone to zero in the meantime (which means no one is
		 * interested in the page any more).  In that case, just clear
		 * the limbo bit and free the extra fictitious page.
		 */
		if (m->limbo) {
			if (m->prep_pin_count == 0) {
				/* page doesn't have to be in limbo any more */
				m->limbo = FALSE;
				vm_page_free(new_page);
				vm_pageout_setup_unprepped++;
			} else {
				vm_page_limbo_exchange(m, new_page);
				vm_pageout_setup_limbo++;
				vm_page_release_limbo(m);
				m = new_page;
			}
		}
		
		vm_page_insert(holding_page, old_object, offset);
		vm_page_unlock_queues();

		m->dirty = TRUE;
		m->precious = FALSE;
		new_m = m;
		new_m->page_lock = VM_PROT_NONE;
		new_m->unlock_request = VM_PROT_NONE;

		if (old_object->internal)
			need_to_wire = TRUE;
	}
	/*
	 *	Record that this page has been written out
	 */
#if	MACH_PAGEMAP
	vm_external_state_set(old_object->existence_map, offset);
#endif	/* MACH_PAGEMAP */

	vm_object_unlock(old_object);

	vm_object_lock(new_object);

	/*
	 *	Put the page into the new object. If it is a not wired
	 *	(if it's the real page) it will be activated.
	 */

	vm_page_lock_queues();
	vm_page_insert(new_m, new_object, new_offset);
	if (need_to_wire)
		vm_page_wire(new_m);
	else
		vm_page_activate(new_m);
	PAGE_WAKEUP_DONE(new_m);
	vm_page_unlock_queues();

	vm_object_unlock(new_object);

	/*
	 *	Return the placeholder page to simplify cleanup.
	 */
	return (holding_page);
}

/*
 * Routine:	vm_pageclean_setup
 *
 * Purpose:	setup a page to be cleaned (made non-dirty), but not
 *		necessarily flushed from the VM page cache.
 *		This is accomplished by cleaning in place.
 *
 *		The page must not be busy, and the object and page
 *		queues must be locked.
 *		
 */
void
vm_pageclean_setup(
	vm_page_t	m,
	vm_page_t	new_m,
	vm_object_t	new_object,
	vm_offset_t	new_offset)
{
	vm_object_t old_object = m->object;
	assert(!m->busy);
	assert(!m->cleaning);

	XPR(XPR_VM_PAGEOUT,
    "vm_pageclean_setup, obj 0x%X off 0x%X page 0x%X new 0x%X new_off 0x%X\n",
		(integer_t)old_object, m->offset, (integer_t)m, 
		(integer_t)new_m, new_offset);

	pmap_clear_modify(m->phys_addr);
	vm_object_paging_begin(old_object);

	/*
	 *	Record that this page has been written out
	 */
#if	MACH_PAGEMAP
	vm_external_state_set(old_object->existence_map, m->offset);
#endif	/*MACH_PAGEMAP*/

	/*
	 * Mark original page as cleaning in place.
	 */
	m->cleaning = TRUE;
	m->dirty = TRUE;
#if	NORMA_VM_PRECIOUS_RETURN_HACK
	/*
	 *	Mark the page precious to preserve the xmm svm
	 *	invariant that if a page has been dirtied, then
	 *	all kernels that have copies of the page have
	 *	precious copies of the page.
	 */
	m->precious = TRUE;
#else	/* NORMA_VM_PRECIOUS_RETURN_HACK */
	m->precious = FALSE;
#endif	/* NORMA_VM_PRECIOUS_RETURN_HACK */
	assert(old_object->pager_trusted);

	/*
	 * Convert the fictitious page to a private shadow of
	 * the real page.
	 */
	assert(new_m->fictitious);
	new_m->fictitious = FALSE;
	new_m->private = TRUE;
	new_m->pageout = TRUE;
	new_m->phys_addr = m->phys_addr;
	vm_page_wire(new_m);

	vm_page_insert(new_m, new_object, new_offset);
	assert(!new_m->wanted);
	new_m->busy = FALSE;
}

void
vm_pageclean_copy(
	vm_page_t	m,
	vm_page_t	new_m,
	vm_object_t	new_object,
	vm_offset_t	new_offset)
{
	XPR(XPR_VM_PAGEOUT,
	"vm_pageclean_copy, page 0x%X new_m 0x%X new_obj 0x%X offset 0x%X\n",
		m, new_m, new_object, new_offset, 0);

	assert((!m->busy) && (!m->cleaning));

	assert(!new_m->private && !new_m->fictitious);

	pmap_clear_modify(m->phys_addr);

	m->busy = TRUE;
	vm_object_paging_begin(m->object);
	vm_page_unlock_queues();
	vm_object_unlock(m->object);

	/*
	 * Copy the original page to the new page.
	 */
	vm_page_copy(m, new_m);

	/*
	 * Mark the old page as clean. A request to pmap_is_modified
	 * will get the right answer.
	 */
	vm_object_lock(m->object);
	m->dirty = FALSE;
#if	NORMA_VM_PRECIOUS_RETURN_HACK
	/*
	 *	Mark the page precious to preserve the xmm svm
	 *	invariant that if a page has been dirtied, then
	 *	all kernels that have copies of the page have
	 *	precious copies of the page.
	 */
	m->precious = TRUE;
#endif	/* NORMA_VM_PRECIOUS_RETURN_HACK */

	vm_object_paging_end(m->object);

	vm_page_lock_queues();
	if (!m->active && !m->inactive)
		vm_page_activate(m);
	PAGE_WAKEUP_DONE(m);

	vm_page_insert(new_m, new_object, new_offset);
	vm_page_activate(new_m);
	new_m->busy = FALSE;	/* No other thread can be waiting */
}


/*
 *	Routine:	vm_pageout_initialize_page
 *	Purpose:
 *		Causes the specified page to be initialized in
 *		the appropriate memory object. This routine is used to push
 *		pages into a copy-object when they are modified in the
 *		permanent object.
 *
 *		The page is moved to a temporary object and paged out.
 *
 *	In/out conditions:
 *		The page in question must not be on any pageout queues.
 *		The object to which it belongs must be locked.
 *		The page must be busy, but not hold a paging reference.
 *
 *	Implementation:
 *		Move this page to a completely new object.
 */
void	
vm_pageout_initialize_page(
	vm_page_t	m)
{
	vm_map_copy_t	copy;
	vm_object_t	new_object;
	vm_object_t	object;
	vm_offset_t	paging_offset;
	vm_page_t	holding_page;


	XPR(XPR_VM_PAGEOUT,
		"vm_pageout_initialize_page, page 0x%X\n",
		(integer_t)m, 0, 0, 0, 0);
	assert(m->busy);

	/*
	 *	Verify that we really want to clean this page
	 */
	assert(!m->absent);
	assert(!m->error);
	assert(m->dirty);

	/*
	 *	Create a paging reference to let us play with the object.
	 */
	object = m->object;
	paging_offset = m->offset + object->paging_offset;
	vm_object_paging_begin(object);
	vm_object_unlock(object);
	if (m->absent || m->error || m->restart ||
	    (!m->dirty && !m->precious)) {
		VM_PAGE_FREE(m);
		panic("reservation without pageout?"); /* alan */
		return;
	}

	/*
	 *	Allocate a new object into which we can put the page.
	 */
	new_object = vm_pageout_object_allocate(m, PAGE_SIZE, m->offset);

	/*
	 *	Move or shadow the page into a new object.
	 */
	holding_page = vm_pageout_setup(m, new_object, 0);

	/* Initialize occurs only to default pager */
	assert(holding_page == VM_PAGE_NULL);

	vm_map_copyin_object(new_object, 0, PAGE_SIZE, &copy);

	VM_STAT(pageouts++);
	/* VM_STAT(pages_pagedout++); */

	/*
	 *	Write the data to its pager.
	 *	Note that the data is passed by naming the new object,
	 *	not a virtual address; the pager interface has been
	 *	manipulated to use the "internal memory" data type.
	 *	[The object reference from its allocation is donated
	 *	to the eventual recipient.]
	 */
	memory_object_data_initialize(object->pager,
					object->pager_request,
					paging_offset,
					POINTER_T(copy),
					PAGE_SIZE);

	vm_object_lock(object);
}

#if	MACH_CLUSTER_STATS
#define MAXCLUSTERPAGES	16
struct {
	unsigned long pages_in_cluster;
	unsigned long pages_at_higher_offsets;
	unsigned long pages_at_lower_offsets;
} cluster_stats[MAXCLUSTERPAGES];
#endif	/* MACH_CLUSTER_STATS */

boolean_t allow_clustered_pageouts = TRUE;

/*
 * vm_pageout_cluster:
 *
 * Given a page, page it out, and attempt to clean adjacent pages
 * in the same operation.
 *
 * The page must be busy, and the object unlocked w/ paging reference
 * to prevent deallocation or collapse. The page must not be on any
 * pageout queue.
 */
void
vm_pageout_cluster(
	vm_page_t m)
{
	vm_object_t	object = m->object;
	vm_offset_t	offset = m->offset;	/* from vm_object start */
	vm_offset_t	paging_offset = m->offset + object->paging_offset;
	vm_object_t	new_object;
	vm_offset_t	new_offset;
	vm_size_t	cluster_size;
	vm_offset_t	cluster_offset;		/* from memory_object start */
	vm_offset_t	cluster_lower_bound;	/* from vm_object_start */
	vm_offset_t	cluster_upper_bound;	/* from vm_object_start */
	vm_offset_t	cluster_start, cluster_end; /* from vm_object start */
	vm_offset_t	offset_within_cluster;
	vm_size_t	length_of_data;
	vm_page_t	friend, holding_page;
	vm_page_t	private = VM_PAGE_NULL;
	vm_map_copy_t	copy;
	kern_return_t	rc;
	boolean_t	precious_clean = FALSE;
	int		pages_in_cluster;

	CLUSTER_STAT(int pages_at_higher_offsets = 0;)
	CLUSTER_STAT(int pages_at_lower_offsets = 0;)

	XPR(XPR_VM_PAGEOUT,
		"vm_pageout_cluster, object 0x%X offset 0x%X page 0x%X\n",
		(integer_t)object, offset, (integer_t)m, 0, 0);

	CLUSTER_STAT(vm_pageout_cluster_clusters++;)
	/*
	 * Only a certain kind of page is appreciated here.
	 */
	assert(m->busy && (m->dirty || m->precious) && (m->wire_count == 0));
	assert(!m->cleaning && !m->pageout && !m->inactive && !m->active);

	vm_object_lock(object);
	cluster_size = object->cluster_size;

	assert(cluster_size >= PAGE_SIZE);
	if (cluster_size < PAGE_SIZE) cluster_size = PAGE_SIZE;
	assert(object->pager_created && object->pager_initialized);
	assert(object->internal || object->pager_ready);

	if (m->precious && !m->dirty)
		precious_clean = TRUE;

	if (!object->pager_trusted || !allow_clustered_pageouts)
		cluster_size = PAGE_SIZE;
	vm_object_unlock(object);

	cluster_offset = paging_offset & (cluster_size - 1);
			/* bytes from beginning of cluster */
	/* 
	 * Due to unaligned mappings, we have to be careful
	 * of negative offsets into the VM object. Clip the cluster 
	 * boundary to the VM object, not the memory object.
	 */
	if (offset > cluster_offset) {
		cluster_lower_bound = offset - cluster_offset;
						/* from vm_object */
	} else {
		cluster_lower_bound = 0;
	}
	cluster_upper_bound = (offset - cluster_offset) + cluster_size;

	new_object = vm_pageout_object_allocate(m, cluster_size,
			paging_offset - cluster_offset - object->paging_offset);

	holding_page = vm_pageout_setup(m, new_object, cluster_offset);

	assert(holding_page || (m->object == object) || (m->limbo));
	assert(object->pager_trusted || holding_page);
	assert(!(object->pager_trusted && holding_page));

	/*
	 * Search backward for adjacent eligible pages to clean in 
	 * this operation.
	 */

	cluster_start = offset;
	if (offset) {	/* avoid wrap-around at zero */
	    for (cluster_start = offset - PAGE_SIZE;
		cluster_start >= cluster_lower_bound;
		cluster_start -= PAGE_SIZE) {
		assert(cluster_size > PAGE_SIZE);

		if (private == VM_PAGE_NULL) VM_PAGE_GRAB_FICTITIOUS(private);

		vm_object_lock(object);
		vm_page_lock_queues();

		if ((friend = vm_pageout_cluster_page(object, cluster_start,
				precious_clean)) == VM_PAGE_NULL) {
			vm_page_unlock_queues();
			vm_object_unlock(object);
			break;
		}
		new_offset = (cluster_start + object->paging_offset)
				& (cluster_size - 1);

		assert(new_offset < cluster_offset);
		assert(private != VM_PAGE_NULL);
		vm_pageclean_setup(friend, private, new_object, new_offset);

		vm_page_unlock_queues();
		vm_object_unlock(object);
		CLUSTER_STAT(pages_at_lower_offsets++;)

		private = VM_PAGE_NULL;
	    }
	    cluster_start += PAGE_SIZE;
	}
	assert(cluster_start >= cluster_lower_bound);
	assert(cluster_start <= offset);
	/*
	 * Search forward for adjacent eligible pages to clean in 
	 * this operation.
	 */
	for (cluster_end = offset + PAGE_SIZE;
		cluster_end < cluster_upper_bound;
		cluster_end += PAGE_SIZE) {
		assert(cluster_size > PAGE_SIZE);

		if (private == VM_PAGE_NULL) VM_PAGE_GRAB_FICTITIOUS(private);

		vm_object_lock(object);
		vm_page_lock_queues();

		if ((friend = vm_pageout_cluster_page(object, cluster_end,
				precious_clean)) == VM_PAGE_NULL) {
			vm_page_unlock_queues();
			vm_object_unlock(object);
			break;
		}
		new_offset = (cluster_end + object->paging_offset)
				& (cluster_size - 1);

		assert(new_offset < cluster_size);
		vm_pageclean_setup(friend, private, new_object, new_offset);

		vm_page_unlock_queues();
		vm_object_unlock(object);
		CLUSTER_STAT(pages_at_higher_offsets++;)

		private = VM_PAGE_NULL;
	}
	assert(cluster_end <= cluster_upper_bound);
	assert(cluster_end >= offset + PAGE_SIZE);

	if (private != VM_PAGE_NULL) {
		private->fictitious = TRUE;
		private->private = FALSE;
		vm_page_release_fictitious(private);
	}

	/*
	 * (offset - cluster_offset) is beginning of cluster_object
	 * relative to vm_object start.
	 */
	offset_within_cluster = cluster_start - (offset - cluster_offset);
	length_of_data = cluster_end - cluster_start;

	assert(offset_within_cluster < cluster_size);
	assert((offset_within_cluster + length_of_data) <= cluster_size);
	rc = vm_map_copyin_object(new_object,
			offset_within_cluster,
			length_of_data,
			&copy);

	assert(rc == KERN_SUCCESS);

	pages_in_cluster = length_of_data/PAGE_SIZE;
	VM_STAT(pageouts++);
	/* VM_STAT(pages_pagedout += pages_in_cluster); */

#if	MACH_CLUSTER_STATS
	(cluster_stats[pages_at_lower_offsets].pages_at_lower_offsets)++;
	(cluster_stats[pages_at_higher_offsets].pages_at_higher_offsets)++;
	(cluster_stats[pages_in_cluster].pages_in_cluster)++;
#endif	/* MACH_CLUSTER_STATS */

	/*
	 * Send the data to the pager.
	 */
	paging_offset = cluster_start + object->paging_offset;
	rc = memory_object_data_return(object->pager,
				       object->pager_request,
				       paging_offset,
				       POINTER_T(copy),
				       length_of_data,
				       !precious_clean,
				       FALSE);

	if (rc != KERN_SUCCESS)
		vm_map_copy_discard(copy);

	if (holding_page) {
		assert(!object->pager_trusted);
		vm_object_lock(object);
		VM_PAGE_FREE(holding_page);
		vm_object_paging_end(object);
		vm_object_unlock(object);
	}
}

/*
 *	Trusted pager throttle.
 *	Object must be unlocked, page queues must be unlocked.
 */
void
vm_pageout_throttle(
	register vm_page_t m)
{
	vm_page_lock_queues();
	assert(!m->laundry);
	m->laundry = TRUE;
	while (vm_page_laundry_count >= vm_page_laundry_max) {
		/*
		 * Set the threshold for when vm_page_free()
		 * should wake us up.
		 */
		vm_page_laundry_min = vm_page_laundry_max/2;
		assert_wait((event_t) &vm_page_laundry_count, FALSE);
		vm_page_unlock_queues();

		/*
		 * Pause to let the default pager catch up.
		 */
		thread_block((void (*)(void)) 0);
		vm_page_lock_queues();
	}
	vm_page_laundry_count++;
	vm_page_unlock_queues();
}

/*
 * The global variable vm_pageout_clean_active_pages controls whether
 * active pages are considered valid to be cleaned in place during a
 * clustered pageout. Performance measurements are necessary to determine
 * the best policy.
 */
int vm_pageout_clean_active_pages = 1;
/*
 * vm_pageout_cluster_page: [Internal]
 *
 * return a vm_page_t to the page at (object,offset) if it is appropriate
 * to clean in place. Pages that are non-existent, busy, absent, already
 * cleaning, or not dirty are not eligible to be cleaned as an adjacent
 * page in a cluster.
 *
 * The object must be locked on entry, and remains locked throughout
 * this call.
 */

vm_page_t
vm_pageout_cluster_page(
	vm_object_t object,
	vm_offset_t offset,
	boolean_t precious_clean)
{
	vm_page_t m;

	XPR(XPR_VM_PAGEOUT,
		"vm_pageout_cluster_page, object 0x%X offset 0x%X\n",
		(integer_t)object, offset, 0, 0, 0);

	if ((m = vm_page_lookup(object, offset)) == VM_PAGE_NULL)
		return(VM_PAGE_NULL);

	if (m->busy || m->absent || m->cleaning || 
	    m->prep_pin_count != 0 ||
	    (m->wire_count != 0) || m->error)
		return(VM_PAGE_NULL);

	if (vm_pageout_clean_active_pages) {
		if (!m->active && !m->inactive) return(VM_PAGE_NULL);
	} else {
		if (!m->inactive) return(VM_PAGE_NULL);
	}

	assert(!m->private);
	assert(!m->fictitious);

	if (!m->dirty) m->dirty = pmap_is_modified(m->phys_addr);

	if (precious_clean) {
		if (!m->precious || m->dirty)
			return(VM_PAGE_NULL);
	} else {
		if (!m->dirty)
			return(VM_PAGE_NULL);
	}
	return(m);
}

/*
 *	vm_pageout_scan does the dirty work for the pageout daemon.
 *	It returns with vm_page_queue_free_lock held and
 *	vm_page_free_wanted == 0.
 */
extern void vm_pageout_scan_continue(void);	/* forward; */

void
vm_pageout_scan(void)
{
	unsigned int burst_count;
	boolean_t now = FALSE;
	unsigned int laundry_pages;
	boolean_t need_more_inactive_pages;

        XPR(XPR_VM_PAGEOUT, "vm_pageout_scan\n", 0, 0, 0, 0, 0);

/*???*/	/*
	 *	We want to gradually dribble pages from the active queue
	 *	to the inactive queue.  If we let the inactive queue get
	 *	very small, and then suddenly dump many pages into it,
	 *	those pages won't get a sufficient chance to be referenced
	 *	before we start taking them from the inactive queue.
	 *
	 *	We must limit the rate at which we send pages to the pagers.
	 *	data_write messages consume memory, for message buffers and
	 *	for map-copy objects.  If we get too far ahead of the pagers,
	 *	we can potentially run out of memory.
	 *
	 *	We can use the laundry count to limit directly the number
	 *	of pages outstanding to the default pager.  A similar
	 *	strategy for external pagers doesn't work, because
	 *	external pagers don't have to deallocate the pages sent them,
	 *	and because we might have to send pages to external pagers
	 *	even if they aren't processing writes.  So we also
	 *	use a burst count to limit writes to external pagers.
	 *
	 *	When memory is very tight, we can't rely on external pagers to
	 *	clean pages.  They probably aren't running, because they
	 *	aren't vm-privileged.  If we kept sending dirty pages to them,
	 *	we could exhaust the free list.  However, we can't just ignore
	 *	pages belonging to external objects, because there might be no
	 *	pages belonging to internal objects.  Hence, we get the page
	 *	into an internal object and then immediately double-page it,
	 *	sending it to the default pager.
	 *
	 *	consider_zone_gc should be last, because the other operations
	 *	might return memory to zones.
	 */

    Restart:

	mutex_lock(&vm_page_queue_free_lock);
	now = (vm_page_free_count < vm_page_free_min);
	mutex_unlock(&vm_page_queue_free_lock);
#if	THREAD_SWAPPER
	swapout_threads(now);
#endif	/* THREAD_SWAPPER */

	stack_collect();
	net_kmsg_collect();
	consider_task_collect();
	consider_thread_collect();
	cleanup_limbo_queue();
	consider_zone_gc();
	consider_machine_collect();

	if (vm_page_free_count <= vm_page_free_reserved) {
		need_more_inactive_pages = TRUE;
	} else {
		need_more_inactive_pages = FALSE;
	}
	for (burst_count = 0;;) {
		register vm_page_t m;
		register vm_object_t object;
		unsigned int free_count;

		/*
		 *	Recalculate vm_page_inactivate_target.
		 */

		vm_page_lock_queues();
		vm_page_inactive_target =
			VM_PAGE_INACTIVE_TARGET(vm_page_active_count +
						vm_page_inactive_count);

		/*
		 *	Move pages from active to inactive.
		 */

		while ((vm_page_inactive_count < vm_page_inactive_target ||
			need_more_inactive_pages) &&
		       !queue_empty(&vm_page_queue_active)) {
			register vm_object_t object;

			vm_pageout_active++;
			m = (vm_page_t) queue_first(&vm_page_queue_active);

			/*
			 * If we're getting really low on memory,
			 * try selecting a page that will go 
			 * directly to the default_pager.
			 * If there are no such pages, we have to
			 * page out a page backed by an EMM,
			 * so that the default_pager can recover
			 * it eventually.
			 */
			if (need_more_inactive_pages) {
				vm_pageout_scan_active_emm_throttle++;
				do {
					assert(m->active && !m->inactive);
					object = m->object;

					if (vm_object_lock_try(object)) {
						if (object->pager_trusted ||
						    object->internal) {
							/* found one ! */
							vm_pageout_scan_active_emm_throttle_success++;
							goto object_locked_active;
						}
						vm_object_unlock(object);
					}
					m = (vm_page_t) queue_next(&m->pageq);
				} while (!queue_end(&vm_page_queue_active,
						    (queue_entry_t) m));
				if (queue_end(&vm_page_queue_active,
					      (queue_entry_t) m)) {
					vm_pageout_scan_active_emm_throttle_failure++;
					m = (vm_page_t)
						queue_first(&vm_page_queue_active);
				}
			}

			assert(m->active && !m->inactive);

			object = m->object;
			if (!vm_object_lock_try(object)) {
				/*
				 *	Move page to end and continue.
				 */

				queue_remove(&vm_page_queue_active, m,
					     vm_page_t, pageq);
				queue_enter(&vm_page_queue_active, m,
					    vm_page_t, pageq);
				vm_page_unlock_queues();
				mutex_pause();
				vm_page_lock_queues();
				continue;
			}

		    object_locked_active:
			/*
			 *	If the page is busy, then we pull it
			 *	off the active queue and leave it alone.
			 */

			if (m->busy) {
				vm_object_unlock(object);
				queue_remove(&vm_page_queue_active, m,
					     vm_page_t, pageq);
				m->active = FALSE;
				if (!m->fictitious)
					vm_page_active_count--;
				continue;
			}

			/*
			 *	Deactivate the page while holding the object
			 *	locked, so we know the page is still not busy.
			 *	This should prevent races between pmap_enter
			 *	and pmap_clear_reference.  The page might be
			 *	absent or fictitious, but vm_page_deactivate
			 *	can handle that.
			 */

			vm_page_deactivate(m);
			vm_object_unlock(object);
		}

		/*
		 *	We are done if we have met our target *and*
		 *	nobody is still waiting for a page.
		 */

		mutex_lock(&vm_page_queue_free_lock);
		free_count = vm_page_free_count;
		if ((free_count >= vm_page_free_target) &&
		    (vm_page_free_wanted == 0)) {
			vm_page_unlock_queues();
			break;
		}
		mutex_unlock(&vm_page_queue_free_lock);

		/*
		 * Sometimes we have to pause:
		 *	1) No inactive pages - nothing to do.
		 *	2) Flow control - wait for untrusted pagers to catch up.
		 */

		if (queue_empty(&vm_page_queue_inactive) ||
		    (burst_count >= vm_pageout_burst_max)) {
			unsigned int pages, msecs;

			/*
			 *	vm_pageout_burst_wait is msecs/page.
			 *	If there is nothing for us to do, we wait
			 *	at least vm_pageout_empty_wait msecs.
			 */
			pages = burst_count;

			msecs = burst_count * vm_pageout_burst_wait;

			if (queue_empty(&vm_page_queue_inactive) &&
			    (msecs < vm_pageout_empty_wait))
				msecs = vm_pageout_empty_wait;
			vm_page_unlock_queues();
			thread_will_wait_with_timeout(current_thread(), msecs);
			counter(c_vm_pageout_scan_block++);

			/*
			 *	Unfortunately, we don't have call_continuation
			 *	so we can't rely on tail-recursion.
			 */
			thread_block((void (*)(void)) 0);
			reset_timeout_check(&current_thread()->timer);
			vm_pageout_scan_continue();
			goto Restart;
			/*NOTREACHED*/
		}

		vm_pageout_inactive++;
		m = (vm_page_t) queue_first(&vm_page_queue_inactive);

		if (vm_page_free_count <= vm_page_free_reserved) {
			/*
			 * We're really low on memory. Try to select a page that
			 * would go directly to the default_pager.
			 * If there are no such pages, we have to page out a 
			 * page backed by an EMM, so that the default_pager
			 * can recover it eventually.
			 */
			vm_pageout_scan_inactive_emm_throttle++;
			do {
				assert(!m->active && m->inactive);
				object = m->object;

				if (vm_object_lock_try(object)) {
					if (object->pager_trusted ||
					    object->internal) {
						/* found one ! */
						vm_pageout_scan_inactive_emm_throttle_success++;
						goto object_locked_inactive;
					}
					vm_object_unlock(object);
				}
				m = (vm_page_t) queue_next(&m->pageq);
			} while (!queue_end(&vm_page_queue_inactive,
					    (queue_entry_t) m));
			if (queue_end(&vm_page_queue_inactive,
				      (queue_entry_t) m)) {
				vm_pageout_scan_inactive_emm_throttle_failure++;
				/*
				 * We should check the "active" queue
				 * for good candidates to page out.
				 */
				need_more_inactive_pages = TRUE;

				m = (vm_page_t)
					queue_first(&vm_page_queue_inactive);
			}
		}

		assert(!m->active && m->inactive);
		object = m->object;

		/*
		 *	Try to lock object; since we've got the
		 *	page queues lock, we can only try for this one.
		 */

		if (!vm_object_lock_try(object)) {
			/*
			 *	Move page to end and continue.
			 */
			queue_remove(&vm_page_queue_inactive, m,
				     vm_page_t, pageq);
			queue_enter(&vm_page_queue_inactive, m,
				    vm_page_t, pageq);
			vm_page_unlock_queues();
			mutex_pause();
			vm_pageout_inactive_nolock++;
			continue;
		}

	    object_locked_inactive:
		/*
		 *	Paging out pages of objects which pager is being
		 *	created by another thread must be avoided, because
		 *	this thread may claim for memory, thus leading to a
		 *	possible dead lock between it and the pageout thread
		 *	which will wait for pager creation, if such pages are
		 *	finally chosen. The remaining assumption is that there
		 *	will finally be enough available pages in the inactive
		 *	pool to page out in order to satisfy all memory claimed
		 *	by the thread which concurrently creates the pager.
		 */

		if (!object->pager_initialized && object->pager_created) {
			/*
			 *	Move page to end and continue, hoping that
			 *	there will be enough other inactive pages to
			 *	page out so that the thread which currently
			 *	initializes the pager will succeed.
			 */
			queue_remove(&vm_page_queue_inactive, m,
				     vm_page_t, pageq);
			queue_enter(&vm_page_queue_inactive, m,
				    vm_page_t, pageq);
			vm_page_unlock_queues();
			vm_object_unlock(object);
			vm_pageout_inactive_avoid++;
			continue;
		}

		/*
		 *	Remove the page from the inactive list.
		 */

		queue_remove(&vm_page_queue_inactive, m, vm_page_t, pageq);
		m->inactive = FALSE;
		if (!m->fictitious)
			vm_page_inactive_count--;

		if (m->busy || !object->alive) {
			/*
			 *	Somebody is already playing with this page.
			 *	Leave it off the pageout queues.
			 */

			vm_page_unlock_queues();
			vm_object_unlock(object);
			vm_pageout_inactive_busy++;
			continue;
		}

		/*
		 *	If it's absent or in error, we can reclaim the page.
		 */

		if (m->absent || m->error) {
			vm_pageout_inactive_absent++;
		    reclaim_page:
			vm_page_free(m);
			vm_page_unlock_queues();
			vm_object_unlock(object);
			continue;
		}

		assert(!m->private);
		assert(!m->fictitious);

		/*
		 *	If already cleaning this page in place, convert from
		 *	"adjacent" to "target". We can leave the page mapped,
		 *	and vm_pageout_object_terminate will determine whether
		 *	to free or reactivate.
		 */

		if (m->cleaning) {
#if	NORMA_VM_PRECIOUS_RETURN_HACK
			/*
			 * There's no point in converting this page since
			 * it's precious and it needs to be cleaned again,
			 * (whether or not it's actually dirty).
			 * Just skip it for now, leaving it off of the
			 * queues.
			 */
			assert(m->precious);
#else	/* NORMA_VM_PRECIOUS_RETURN_HACK */
#if	MACH_CLUSTER_STATS
			vm_pageout_cluster_conversions++;
#endif
			if (m->prep_pin_count == 0) {
				m->busy = TRUE;
				m->pageout = TRUE;
				vm_page_wire(m);
			}
#endif	/* NORMA_VM_PRECIOUS_RETURN_HACK */
			vm_object_unlock(object);
			vm_page_unlock_queues();
			continue;
		}

		/*
		 *	If it's being used, reactivate.
		 *	(Fictitious pages are either busy or absent.)
		 */

		if (m->reference || pmap_is_referenced(m->phys_addr)) {
			vm_pageout_inactive_used++;
		    reactivate_page:
#if	ADVISORY_PAGEOUT
			if (m->discard_request) {
				m->discard_request = FALSE;
			}
#endif	/* ADVISORY_PAGEOUT */
			vm_object_unlock(object);
			vm_page_activate(m);
			VM_STAT(reactivations++);
			vm_page_unlock_queues();
			continue;
		}

		if (m->prep_pin_count != 0) {
			int s;
			boolean_t pinned = FALSE;

			s = splvm();
			vm_page_pin_lock();
			if (m->pin_count != 0) {
				/* skip and reactivate pinned page */
				pinned = TRUE;
				vm_pageout_inactive_pinned++;
			} else {
				/* page is prepped; send it into limbo */
				m->limbo = TRUE;
				vm_pageout_inactive_limbo++;
			}
			vm_page_pin_unlock();
			splx(s);
			if (pinned)
				goto reactivate_page;
		}

#if	ADVISORY_PAGEOUT
		if (object->advisory_pageout) {
			boolean_t	do_throttle;
			ipc_port_t	port;
			vm_offset_t	discard_offset;

			if (m->discard_request) {
				vm_stat_discard_failure++;
				goto mandatory_pageout;
			}

			assert(object->pager_initialized);
			m->discard_request = TRUE;
			port = object->pager;

			/* system-wide throttle */
			do_throttle = (vm_page_free_count <=
				       vm_page_free_reserved);
			if (!do_throttle) {
				/* throttle on this pager */
				/* XXX lock ordering ? */
				ip_lock(port);
				do_throttle = (port->ip_msgcount >
					       port->ip_qlimit);
				ip_unlock(port);
			}
			if (do_throttle) {
				vm_stat_discard_throttle++;
#if 0
				/* ignore this page and skip to next */
				vm_page_unlock_queues();
				vm_object_unlock(object);
				continue;
#else
				/* force mandatory pageout */
				goto mandatory_pageout;
#endif
			}

			/* proceed with discard_request */
			vm_page_activate(m);
			vm_stat_discard++;
			VM_STAT(reactivations++);
			discard_offset = m->offset + object->paging_offset;
			vm_stat_discard_sent++;
			vm_page_unlock_queues();
			vm_object_unlock(object);
			memory_object_discard_request(object->pager,
						      object->pager_request,
						      discard_offset,
						      PAGE_SIZE);
			continue;
		}
	mandatory_pageout:
#endif	/* ADVISORY_PAGEOUT */
			
                XPR(XPR_VM_PAGEOUT,
                "vm_pageout_scan, replace object 0x%X offset 0x%X page 0x%X\n",
                (integer_t)object, (integer_t)m->offset, (integer_t)m, 0,0);

		/*
		 *	Eliminate all mappings.
		 */

		m->busy = TRUE;
		pmap_page_protect(m->phys_addr, VM_PROT_NONE);
		if (!m->dirty)
			m->dirty = pmap_is_modified(m->phys_addr);

		/*
		 *	If it's clean and not precious, we can free the page.
		 */

		if (!m->dirty && !m->precious) {
			vm_pageout_inactive_clean++;
			goto reclaim_page;
		}
		vm_page_unlock_queues();

		/*
		 *	If there is no memory object for the page, create
		 *	one and hand it to the default pager.
		 *	[First try to collapse, so we don't create
		 *	one unnecessarily.]
		 */

		if (!object->pager_initialized)
			vm_object_collapse(object);
		if (!object->pager_initialized)
			vm_object_pager_create(object);
		if (!object->pager_initialized) {
			/*
			 *	Still no pager for the object.
			 *	Reactivate the page.
			 *
			 *	Should only happen if there is no
			 *	default pager.
			 */
			vm_page_lock_queues();
			vm_page_activate(m);
			vm_page_unlock_queues();

			/*
			 *	And we are done with it.
			 */
			PAGE_WAKEUP_DONE(m);
			vm_object_unlock(object);

			/*
			 * break here to get back to the preemption
			 * point in the outer loop so that we don't
			 * spin forever if there is no default pager.
			 */
			vm_pageout_dirty_no_pager++;
			/*
			 * We haven't met the other criteria our caller
			 * expects, but can at least hold the lock it
			 * wants us to.
			 */
			mutex_lock(&vm_page_queue_free_lock);
			break;
		}

		if (object->pager_initialized && object->pager == IP_NULL) {
			/*
			 * This pager has been destroyed by either
			 * memory_object_destroy or vm_object_destroy, and
			 * so there is nowhere for the page to go.
			 * Just free the page.
			 */
			VM_PAGE_FREE(m);
			vm_object_unlock(object);
			continue;
		}

		vm_pageout_inactive_dirty++;
		if (!object->internal)
			burst_count++;
		vm_object_paging_begin(object);
		vm_object_unlock(object);
		vm_pageout_cluster(m);	/* flush it */
	}
}

counter(unsigned int	c_vm_pageout_scan_continue = 0;)

void
vm_pageout_scan_continue(void)
{
	/*
	 *	We just paused to let the pagers catch up.
	 *	If vm_page_laundry_count is still high,
	 *	then we aren't waiting long enough.
	 *	If we have paused some vm_pageout_pause_max times without
	 *	adjusting vm_pageout_burst_wait, it might be too big,
	 *	so we decrease it.
	 */

	vm_page_lock_queues();
	counter(++c_vm_pageout_scan_continue);
	if (vm_page_laundry_count > vm_pageout_burst_min) {
		vm_pageout_burst_wait++;
		vm_pageout_pause_count = 0;
	} else if (++vm_pageout_pause_count > vm_pageout_pause_max) {
		vm_pageout_burst_wait = (vm_pageout_burst_wait * 3) / 4;
		if (vm_pageout_burst_wait < 1)
			vm_pageout_burst_wait = 1;
		vm_pageout_pause_count = 0;
	}
	vm_page_unlock_queues();
}

void vm_page_free_reserve(int pages);
int vm_page_free_count_init;

void
vm_page_free_reserve(
	int pages)
{
	int		free_after_reserve;

	vm_page_free_reserved += pages;

	free_after_reserve = vm_page_free_count_init - vm_page_free_reserved;

	vm_page_free_min = vm_page_free_reserved +
		VM_PAGE_FREE_MIN(free_after_reserve);

	vm_page_free_target = vm_page_free_reserved +
		VM_PAGE_FREE_TARGET(free_after_reserve);

	if (vm_page_free_target < vm_page_free_min + 5)
		vm_page_free_target = vm_page_free_min + 5;
}

/*
 *	vm_pageout is the high level pageout daemon.
 */


void
vm_pageout(void)
{
	thread_t	thread;
	processor_set_t	pset;
        kern_return_t                   ret;
        policy_base_t                   base;
        policy_limit_t                  limit;
        policy_fifo_base_data_t         fifo_base;
        policy_fifo_limit_data_t        fifo_limit;

        /*
         * Set thread privileges.
         */
	thread = current_thread();
        thread->vm_privilege = TRUE;
	vm_page_free_reserve(5);	/* XXX */
        stack_privilege(thread);
	thread_swappable(current_act(), FALSE);

	/*
	 * Set thread scheduling priority and policy.
	 */
	pset = thread->processor_set;
        base = (policy_base_t) &fifo_base;
        limit = (policy_limit_t) &fifo_limit;
        fifo_base.base_priority = BASEPRI_SYSTEM;
        fifo_limit.max_priority = BASEPRI_SYSTEM;
        ret = thread_set_policy(thread->top_act, pset, POLICY_FIFO, 
				base, POLICY_TIMESHARE_BASE_COUNT, 
				limit, POLICY_TIMESHARE_LIMIT_COUNT);
        if (ret != KERN_SUCCESS)
                printf("WARNING: vm_pageout_thread is being TIMESHARED!\n");

	/*
	 *	Initialize some paging parameters.
	 */

	if (vm_page_laundry_max == 0)
		vm_page_laundry_max = VM_PAGE_LAUNDRY_MAX;

	if (vm_pageout_burst_max == 0)
		vm_pageout_burst_max = VM_PAGEOUT_BURST_MAX;

	if (vm_pageout_burst_wait == 0)
		vm_pageout_burst_wait = VM_PAGEOUT_BURST_WAIT;

	if (vm_pageout_empty_wait == 0)
		vm_pageout_empty_wait = VM_PAGEOUT_EMPTY_WAIT;

	vm_page_free_count_init = vm_page_free_count;
	if (vm_page_free_reserved == 0)
		vm_page_free_reserve(VM_PAGE_FREE_RESERVED);

	/*
	 *	vm_pageout_scan will set vm_page_inactive_target.
	 *
	 *	The pageout daemon is never done, so loop forever.
	 *	We should call vm_pageout_scan at least once each
	 *	time we are woken, even if vm_page_free_wanted is
	 *	zero, to check vm_page_free_target and
	 *	vm_page_inactive_target.
	 */
	for (;;) {
		vm_pageout_scan();
		/* we hold vm_page_queue_free_lock now */
		assert(vm_page_free_wanted == 0);
		assert_wait((event_t) &vm_page_free_wanted, FALSE);
		mutex_unlock(&vm_page_queue_free_lock);
		counter(c_vm_pageout_block++);
		thread_block((void (*)(void)) 0);
	}
	/*NOTREACHED*/
}


#if	MACH_KDB
#include <ddb/db_output.h>
#include <ddb/db_print.h>
#include <vm/vm_print.h>

#define	printf	kdbprintf
extern int	indent;
void		db_pageout(void);

void
db_vm(void)
{
	extern int vm_page_gobble_count;
	extern int vm_page_limbo_count, vm_page_limbo_real_count;
	extern int vm_page_pin_count;

	iprintf("VM Statistics:\n");
	indent += 2;
	iprintf("pages:\n");
	indent += 2;
	iprintf("activ %5d  inact %5d  free  %5d",
		vm_page_active_count, vm_page_inactive_count,
		vm_page_free_count);
	printf("   wire  %5d  gobbl %5d\n",
	       vm_page_wire_count, vm_page_gobble_count);
	iprintf("laund %5d  limbo %5d  lim_r %5d   pin   %5d\n",
		vm_page_laundry_count, vm_page_limbo_count,
		vm_page_limbo_real_count, vm_page_pin_count);
	indent -= 2;
	iprintf("target:\n");
	indent += 2;
	iprintf("min   %5d  inact %5d  free  %5d",
		vm_page_free_min, vm_page_inactive_target,
		vm_page_free_target);
	printf("   resrv %5d\n", vm_page_free_reserved);
	indent -= 2;

	iprintf("burst:\n");
	indent += 2;
	iprintf("max   %5d  min   %5d  wait  %5d   empty %5d\n",
		  vm_pageout_burst_max, vm_pageout_burst_min,
		  vm_pageout_burst_wait, vm_pageout_empty_wait);
	indent -= 2;
	iprintf("pause:\n");
	indent += 2;
	iprintf("count %5d  max   %5d\n",
		vm_pageout_pause_count, vm_pageout_pause_max);
#if	MACH_COUNTERS
	iprintf("scan_continue called %8d\n", c_vm_pageout_scan_continue);
#endif	/* MACH_COUNTERS */
	indent -= 2;
	db_pageout();
	indent -= 2;
}

void
db_pageout(void)
{
	extern int c_limbo_page_free;
	extern int c_limbo_convert;
#if	MACH_COUNTERS
	extern int c_laundry_pages_freed;
#endif	/* MACH_COUNTERS */

	iprintf("Pageout Statistics:\n");
	indent += 2;
	iprintf("active %5d  inactv %5d\n",
		vm_pageout_active, vm_pageout_inactive);
	iprintf("nolock %5d  avoid  %5d  busy   %5d  absent %5d\n",
		vm_pageout_inactive_nolock, vm_pageout_inactive_avoid,
		vm_pageout_inactive_busy, vm_pageout_inactive_absent);
	iprintf("used   %5d  clean  %5d  dirty  %5d\n",
		vm_pageout_inactive_used, vm_pageout_inactive_clean,
		vm_pageout_inactive_dirty);
	iprintf("pinned %5d  limbo  %5d  setup_limbo %5d  setup_unprep %5d\n",
		vm_pageout_inactive_pinned, vm_pageout_inactive_limbo,
		vm_pageout_setup_limbo, vm_pageout_setup_unprepped);
	iprintf("limbo_page_free  %5d   limbo_convert  %5d\n",
		c_limbo_page_free, c_limbo_convert);
#if	MACH_COUNTERS
	iprintf("laundry_pages_freed %d\n", c_laundry_pages_freed);
#endif	/* MACH_COUNTERS */
#if	MACH_CLUSTER_STATS
	iprintf("Cluster Statistics:\n");
	indent += 2;
	iprintf("dirtied   %5d   cleaned  %5d   collisions  %5d\n",
		vm_pageout_cluster_dirtied, vm_pageout_cluster_cleaned,
		vm_pageout_cluster_collisions);
	iprintf("clusters  %5d   conversions  %5d\n",
		vm_pageout_cluster_clusters, vm_pageout_cluster_conversions);
	indent -= 2;
	iprintf("Target Statistics:\n");
	indent += 2;
	iprintf("collisions   %5d   page_dirtied  %5d   page_freed  %5d\n",
		vm_pageout_target_collisions, vm_pageout_target_page_dirtied,
		vm_pageout_target_page_freed);
	iprintf("page_pinned  %5d   page_limbo    %5d\n",
		vm_pageout_target_page_pinned, vm_pageout_target_page_limbo);
	indent -= 2;
#endif	/* MACH_CLUSTER_STATS */
	indent -= 2;
}

#if MACH_CLUSTER_STATS
unsigned long vm_pageout_cluster_dirtied = 0;
unsigned long vm_pageout_cluster_cleaned = 0;
unsigned long vm_pageout_cluster_collisions = 0;
unsigned long vm_pageout_cluster_clusters = 0;
unsigned long vm_pageout_cluster_conversions = 0;
unsigned long vm_pageout_target_collisions = 0;
unsigned long vm_pageout_target_page_dirtied = 0;
unsigned long vm_pageout_target_page_freed = 0;
unsigned long vm_pageout_target_page_pinned = 0;
unsigned long vm_pageout_target_page_limbo = 0;
#define CLUSTER_STAT(clause)	clause
#else	/* MACH_CLUSTER_STATS */
#define CLUSTER_STAT(clause)
#endif	/* MACH_CLUSTER_STATS */

#endif	/* MACH_KDB */
