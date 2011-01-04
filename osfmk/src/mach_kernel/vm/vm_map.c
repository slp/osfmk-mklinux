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
 * Revision 2.32.3.7  92/09/15  17:36:26  jeffreyh
 * 	Remove unneeded calls to vm_object_pmap_remove.  They became
 * 	unneeded when sharing maps were removed.  From bruel@gr.osf.org.
 * 	[92/09/03            dlb]
 * 
 * 	Change vm_map_fork to handle unreadable regions in
 * 	VM_INHERIT_COPY.  This includes inheriting unreadable
 * 	memory that may become readable in the future.
 * 	[92/08/31            dlb]
 * 	Fix so file would compile with NORMA_IPC turned off
 * 
 * 	05-Aug-92  Mike Hibler (mike@cs.utah.edu)
 * 	Cleaned up boot path wiring of pages.
 * 	[92/08/25            jeffreyh]
 * 
 * 	Made changes for new vm_fault_page interface to return an error code.
 * 	[92/07/28            sjs]
 * 
 * 	Fix vm_map_convert_to_page_list to check for a null object.
 * 	Use page_loose boolean in page list map copy instead of
 * 	checking whether first page is tabled.
 * 	[92/06/30            dlb]
 * 
 * 	Use pageable copy objects for large NORMA messages,
 * 	which also requires improved technology for converting
 * 	copy objects from object flavor to page list flavor.
 * 	Added vm_map_object_to_entry_list for converting object
 * 	flavor to entry list flavor.  As part of these changes,
 * 	vm_map_copyout was fixed to handle unaligned copy
 * 	objects correctly (dlb@osf.org).  Unrelated small changes:
 * 		- vm_map_convert_from_page_list must hold
 * 		appropriate locks while using vm_page_insert
 * 		and vm_page_activate
 * 		- Removed obsolete entry and hdr link
 * 		initialization code from vm_map_copyin_object
 * 	[92/06/02            alanl]
 * 
 * Revision 2.32.3.6  92/04/09  14:03:01  jeffreyh
 * 	Mark copied-out pages dirty in vm_map_copyout_page_list.
 * 	[92/04/05            dlb]
 * 
 * Revision 2.32.3.5  92/03/28  10:16:40  jeffreyh
 * 	Change object->temporary to object->internal in most places since
 * 	temporary external objects now exist.
 * 	[92/03/27            dlb]
 * 
 * 	Fix vm_map_convert_{to,from}_page_list to cope with continuations.
 * 	[92/03/26            dlb]
 * 	Always need vm_map_copy_discard_cont (not just NORMA_IPC).
 * 	Add continuation recognition optimization to vm_map_copy_discard
 * 	to avoid tail recursion when vm_map_copy_discard_cont is used.
 * 	[92/03/20  14:14:00  dlb]
 * 
 * Revision 2.32.3.4  92/03/03  16:26:18  jeffreyh
 * 	Clip map entry to end of region covered by first page
 * 	list when making a continuation in vm_map_copyin_page_list.
 * 	[92/03/02            dlb]
 * 
 * 	Do pmap_protect in VM_INHERIT_SHARE case of vm_map_fork
 * 	if creating a shadow object for reasons other than
 * 	copy on write.
 * 	[92/02/28            dlb]
 * 	Tweak VM_INHERIT_SHARE code in vm_map_fork() in an
 * 	attempt to cut down on the amount of temporary memory
 * 	that uses asynchronous copy on write.
 * 	[92/02/28  13:14:21  jeffreyh]
 * 
 * 	Remove all keep_wired logic.  wiring_required logic in
 * 	vm_map_copyin_page_list is the replacement.
 * 	[92/02/21  10:15:26  dlb]
 * 
 * 	Change wiring_allowed to wiring_required.  Pay attention to
 * 	it in vm_map_copyout_page list.  This is only for the default
 * 	pager -- to make it fully general, vm_map_copyout has to
 * 	be modified as well - see XXX comment at bottom of routine.
 * 	[92/02/20  15:18:13  dlb]
 * 
 * 	Use object->shadowed and object->use_shared copy to
 * 	detect sharing instead of the reference count.
 * 	[92/02/19  17:39:10  dlb]
 * 
 * 	Use is_shared field in map entries to detect sharing.
 * 	Some minor bug fixes to the code that eliminates
 * 	sharing maps.
 * 	[92/02/19  14:25:30  dlb]
 * 
 * 	Use object->use_shared_copy instead of new copy strategy.
 * 	Removed all sharing map logic.  Rewrote comments to reflect this.
 * 	vm_map_verify_done is now a macro in vm_map.h as a result.
 * 
 * 	First cut (commented out) at vm_map_copy_overwrite optimization
 * 	to insert entries from copy into target, still needs work.
 * 
 * 	Removed (bogus) single_use argument from vm_map_lookup().
 * 
 * 	Pick up dbg bug fix to vm_map_copyout{,page_list} to allow
 * 	insertion of new region between two old ones that fit exactly.
 * 	[92/01/07  11:11:52  dlb]
 * 
 * 	Replace share map logic in vm_map_fork with asynchronous
 * 	copy-on-write.  Check for null object in vm_map_entry_delete.
 * 	[92/01/06  16:22:17  dlb]
 * 	Merge more changes from dlb
 * 	[92/02/26  12:34:55  jeffreyh]
 * 
 * 	Updates from TRUNK
 * 	[92/02/25  17:08:42  jeffreyh]
 * 
 * Revision 2.32.3.3.3.1  92/02/25  16:59:30  jeffreyh
 * 	[David L. Black 92/02/22  17:06:22  dlb]
 * 	   Move inheritance arg check out of vm_map_inherit.
 * 	   Its callers must do this.
 * 
 * Revision 2.33  92/01/03  20:34:57  dbg
 * 	Fix erroneous boundary condition in search for free space in
 * 	vm_map_copyout(), vm_map_copyout_page_list().  The 'end' of the
 * 	entry is the last used address + 1, and therefore can be <= the
 * 	start of the next entry.  See vm_map_enter(), where it was done
 * 	correctly.
 * 	[91/12/23            dbg]
 * 
 * 	Add vm_map_copy_copy.  It is needed by kernel interface
 * 	routines that may still return errors after copying a
 * 	copy-object into kernel address space.
 * 	[91/12/18            dbg]
 *
 * Revision 2.32.3.3  92/02/18  19:22:03  jeffreyh
 * 	Initialize steal_pages field in coninuations made 
 *	by vm_map_copy_in_page_list
 * 	[92/02/18            dlb]
 * 	Add missing vm_map_lock to error case of fault in
 * 	vm_map_copyin_page_list.
 * 	[92/02/14            dlb]
 * 
 * Revision 2.32.3.2  92/01/09  18:46:21  jsb
 * 	Fixed offset bug in vm_map_copyout_page_list.
 * 	Fixed format errors in vm_map_copy_print.
 * 	[92/01/09  15:32:44  jsb]
 * 
 * 	Added vm_map_copy_print.
 * 	[92/01/08  10:10:53  jsb]
 * 
 * 	From dlb: set continuation size correctly in vm_map_copyin_page_list.
 * 	[92/01/04  18:28:28  jsb]
 * 
 * Revision 2.32.3.1  92/01/03  16:39:12  jsb
 * 	Corrected log.
 * 	[91/12/24  14:35:16  jsb]
 * 
 *
 * 
 * Revision 2.32  91/12/13  13:49:41  jsb
 * 	Removed NORMA_ETHER always_steal hack.
 * 
 * Revision 2.30  91/12/10  13:26:34  jsb
 * 	5-Dec-91 David L. Black (dlb) at Open Software Foundation
 * 	Simplify page list continuation abort logic.
 * 	[91/12/10  12:56:06  jsb]
 * 
 * Revision 2.29  91/11/14  17:08:56  rpd
 * 	Add ifdef's to always steal code. Not needed (or wanted) except
 * 	to workaround a norma ether bug. UGH!
 * 	[91/11/14            jeffreyh]
 * 
 * 	Add consume on success logic to vm_map_copyout_page_list.
 * 	[91/11/12            dlb]
 * 
 * Revision 2.28  91/11/14  16:56:51  rpd
 * 	Picked up mysterious norma changes.
 * 	[91/11/14            rpd]
 * 
 * Revision 2.27  91/10/09  16:20:05  af
 * 	Fixed vm_map_copy_page_discard to lock before activating.
 * 	Fixed vm_map_copyout_page_list to clear just the busy bit (from dlb).
 * 	Fixed vm_map_copy_steal_pages to activate if necessary (from dlb).
 * 	[91/10/07            rpd]
 * 
 * 	Fixed vm_map_copyout_page_list to clear busy and dirty bits.
 * 	[91/10/06            rpd]
 * 
 * 	Picked up dlb fix for stealing wired pages in vm_map_copyin_page_list.
 * 	[91/09/27            rpd]
 * 
 * Revision 2.26  91/08/28  11:18:10  jsb
 * 	Changed vm_map_copyout to discard the copy object only upon success.
 * 	[91/08/03            rpd]
 * 	Initialize copy->cpy_cont and copy->cpy_cont args in
 * 	vm_map_convert_to_page_list and
 * 	vm_map_convert_to_page_list_from_object.
 * 	[91/08/16  10:34:53  jsb]
 * 
 * 	Optimize stealing wired pages case of vm_map_copyin_page_list.
 * 	[91/08/12  17:36:57  dlb]
 * 
 * 	Move vm_map_copy_steal pages in this file.  Improve comments,
 * 	and related cleanup.
 * 	[91/08/06  17:22:43  dlb]
 * 
 * 	Split page release logic for page lists into separate
 * 	routine, vm_map_copy_page_discard.  Minor continuation
 * 	bug fix.
 * 	[91/08/05  17:48:23  dlb]
 * 
 * 	Move logic that steals pages by making a new copy into a separate
 * 	routine since both vm_map_{copyin,copyout}_page_list may need it.
 * 	Also: Previous merge included logic to be a little more careful
 * 	about what gets copied when a map entry is duplicated.
 * 	[91/07/31  15:15:19  dlb]
 * 
 * 	Implement vm_map_copy continuation for page lists.  
 * 	Implement in transition map entries needed by the above.
 * 	[91/07/30  14:16:40  dlb]
 * 
 * 	New and improved version of vm_map_copyin_page_list:
 * 	    Clean up error handling (especially vm_fault_page returns).
 * 	    Avoid holding map locks across faults and page copies.
 * 	    Move page stealing code to separate loop to deal with
 * 		pagein errors from vm_fault_page.
 * 	    Add support for stealing wired pages (allows page stealing on
 * 		pagein from default pager).
 * 	[91/07/03  14:15:39  dlb]
 * 	Restored definition of vm_map_convert_from_page_list.
 * 	Added definition of vm_map_convert_to_page_list_from_object.
 * 	Added call to vm_map_convert_from_page_list to vm_map_copy_overwrite.
 * 	Added include of kern/assert.h.
 * 	[91/08/15  13:20:13  jsb]
 * 
 * Revision 2.25  91/08/03  18:19:58  jsb
 * 	Removed vm_map_convert_from_page_list.
 * 	Temporarily make vm_map_copyin_page_list always steal pages.
 * 	[91/08/01  22:49:17  jsb]
 * 
 * 	NORMA_IPC: Added vm_map_convert_{to,from}_page_list functions.
 * 	These will be removed when all kernel interfaces
 * 	understand page_list copy objects.
 * 	[91/07/04  14:00:24  jsb]
 * 
 * 	Removed obsoleted NORMA_IPC functions:
 * 		ipc_clport_copyin_object
 * 		vm_map_copyout_page
 * 		ipc_clport_copyin_pagelist
 * 	[91/07/04  13:20:09  jsb]
 * 
 * Revision 2.24  91/07/01  08:27:22  jsb
 * 	20-Jun-91 David L. Black (dlb) at Open Software Foundation
 * 	Add support for page list format map copies.  NORMA/CLPORT code
 * 	will be cut over later.
 * 
 * 	18-Jun-91 David L. Black (dlb) at Open Software Foundation
 * 	Convert to use multiple format map copies.
 * 	[91/06/29  16:37:03  jsb]
 * 
 * Revision 2.23  91/06/25  10:33:33  rpd
 * 	Changed mach_port_t to ipc_port_t where appropriate.
 * 	[91/05/28            rpd]
 * 
 * Revision 2.22  91/06/17  15:49:02  jsb
 * 	Renamed NORMA conditionals.
 * 	[91/06/17  11:11:13  jsb]
 * 
 * Revision 2.21  91/06/06  17:08:22  jsb
 * 	NORMA_IPC support (first cut):
 * 	Work with page lists instead of copy objects.
 * 	Make coalescing more useful.
 * 	[91/05/14  09:34:41  jsb]
 * 
 * Revision 2.20  91/05/18  14:40:53  rpd
 * 	Restored mask argument to vm_map_find_entry.
 * 	[91/05/02            rpd]
 * 	Removed ZALLOC and ZFREE.
 * 	[91/03/31            rpd]
 * 
 * 	Revised vm_map_find_entry to allow coalescing of entries.
 * 	[91/03/28            rpd]
 * 	Removed map_data.  Moved kentry_data here.
 * 	[91/03/22            rpd]
 * 
 * Revision 2.19  91/05/14  17:49:38  mrt
 * 	Correcting copyright
 * 
 * Revision 2.18  91/03/16  15:05:42  rpd
 * 	Fixed vm_map_pmap_enter to activate loose pages after PMAP_ENTER.
 * 	[91/03/11            rpd]
 * 	Removed vm_map_find.
 * 	[91/03/03            rpd]
 * 	Fixed vm_map_entry_delete's use of vm_object_page_remove,
 * 	following dlb's report.
 * 	[91/01/26            rpd]
 * 	Picked up dlb's fix for vm_map_fork/VM_INHERIT_COPY of wired entries.
 * 	[91/01/12            rpd]
 * 
 * Revision 2.17  91/02/05  17:58:43  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:32:45  mrt]
 * 
 * Revision 2.16  91/01/08  16:45:08  rpd
 * 	Added continuation argument to thread_block.
 * 	[90/12/08            rpd]
 * 
 * Revision 2.15  90/11/05  14:34:26  rpd
 * 	Removed vm_region_old_behavior.
 * 	[90/11/02            rpd]
 * 
 * Revision 2.14  90/10/25  14:50:18  rwd
 * 	Fixed bug in vm_map_enter that was introduced in 2.13.
 * 	[90/10/21            rpd]
 * 
 * Revision 2.13  90/10/12  13:05:48  rpd
 * 	Removed copy_on_write field.
 * 	[90/10/08            rpd]
 * 
 * Revision 2.12  90/08/06  15:08:31  rwd
 * 	Fixed several bugs in the overwriting-permanent-memory case of
 * 	vm_map_copy_overwrite, including an object reference leak.
 * 	[90/07/26            rpd]
 * 
 * Revision 2.11  90/06/19  23:02:09  rpd
 * 	Picked up vm_submap_object, vm_map_fork share-map revisions,
 * 	including Bohman's bug fix.
 * 	[90/06/08            rpd]
 * 
 * 	Fixed vm_region so that it doesn't treat sub-map entries (only
 * 	found in the kernel map) as regular entries.  Instead, it just
 * 	ignores them and doesn't try to send back an object_name reference.
 * 	[90/03/23            gk]
 * 
 * Revision 2.10  90/06/02  15:10:57  rpd
 * 	Moved vm_mapped_pages_info to vm/vm_debug.c.
 * 	[90/05/31            rpd]
 * 
 * 	In vm_map_copyin, if length is zero allow any source address.
 * 	[90/04/23            rpd]
 * 
 * 	Correct share/sub map confusion in vm_map_copy_overwrite.
 * 	[90/04/22            rpd]
 * 
 * 	In vm_map_copyout, make the current protection be VM_PROT_DEFAULT
 * 	and the inheritance be VM_INHERIT_DEFAULT.
 * 	[90/04/18            rpd]
 * 
 * 	Removed some extraneous code from vm_map_copyin/vm_map_copyout.
 * 	[90/03/28            rpd]
 * 	Updated to new vm_map_pageable, with user_wired_count.
 * 	Several bug fixes for vm_map_copy_overwrite.
 * 	Added vm_map_copyin_object.
 * 	[90/03/26  23:14:56  rpd]
 * 
 * Revision 2.9  90/05/29  18:38:46  rwd
 * 	Add flag to turn off forced pmap_enters in vm_map call.
 * 	[90/05/12            rwd]
 * 	Bug fix from rpd for OOL data to VM_PROT_DEFAULT.  New
 * 	vm_map_pmap_enter from rfr to preemtively enter pages on vm_map
 * 	calls.
 * 	[90/04/20            rwd]
 * 
 * Revision 2.8  90/05/03  15:52:42  dbg
 * 	Fix vm_map_copyout to set current protection of new entries to
 * 	VM_PROT_DEFAULT, to match vm_allocate.
 * 	[90/04/12            dbg]
 * 
 * 	Add vm_mapped_pages_info under switch MACH_DEBUG.
 * 	[90/04/06            dbg]
 * 
 * Revision 2.7  90/02/22  20:05:52  dbg
 * 	Combine fields in vm_map and vm_map_copy into a vm_map_header
 * 	structure.  Fix macros dealing with vm_map_t and vm_map_copy_t
 * 	to operate on the header, so that most of the code they use can
 * 	move back into the associated functions (to reduce space).
 * 	[90/01/29            dbg]
 * 
 * 	Add missing code to copy map entries from pageable to
 * 	non-pageable zone in vm_map_copyout.  Try to avoid
 * 	vm_object_copy in vm_map_copyin if source will be
 * 	destroyed.  Fix vm_map_copy_overwrite to correctly
 * 	check for gaps in destination when destination is
 * 	temporary.
 * 	[90/01/26            dbg]
 * 
 * 	Add keep_wired parameter to vm_map_copyin.
 * 	Remove vm_map_check_protection and vm_map_insert (not used).
 * 	Rewrite vm_map_find to call vm_map_enter - should fix all
 * 	callers instead.
 * 	[90/01/25            dbg]
 * 
 * 	Add changes from mainline:
 * 
 * 	Fixed syntax errors in vm_map_print.
 * 	Fixed use of vm_object_copy_slowly in vm_map_copyin.
 * 	Restored similar fix to vm_map_copy_entry.
 * 	[89/12/01  13:56:30  rpd]
 * 	Make sure object lock is held before calling
 * 	vm_object_copy_slowly.  Release old destination object in wired
 * 	case of vm_map_copy_entry.  Fixes from rpd.
 * 	[89/12/15            dlb]
 * 
 * 	Modify vm_map_pageable to create new objects BEFORE clipping
 * 	map entries to avoid object proliferation.
 * 	[88/11/30            dlb]
 * 			
 * 	Check for holes when wiring memory in vm_map_pageable.
 * 	Pass requested access type to vm_map_pageable and check it.
 * 	[88/11/21            dlb]
 * 
 * 	Handle overwriting permanent objects in vm_map_copy_overwrite().
 * 
 * 	Put optimized copy path in vm_map_fork().
 * 	[89/10/01  23:24:32  mwyoung]
 * 
 * 	Integrate the "wait for space" option for kernel maps
 * 	into this module.
 * 
 * 	Add vm_map_copyin(), vm_map_copyout(), vm_map_copy_discard() to
 * 	perform map copies.
 * 
 * 	Convert vm_map_entry_create(), vm_map_clip_{start,end} so that
 * 	they may be used with either a vm_map_t or a vm_map_copy_t.
 * 
 * 	Use vme_next, vme_prev, vme_start, vme_end, vm_map_to_entry.
 * 	[89/08/31  21:12:23  rpd]
 * 
 * 	Picked up NeXT change to vm_region:  now if you give it an
 * 	address in the middle of an entry, it will use the start of
 * 	the entry.
 * 	[89/08/20  23:19:39  rpd]
 * 
 * 	A bug fix from NeXT:  vm_map_protect wasn't unlocking in the
 * 	is_sub_map case.  Also, fixed vm_map_copy_entry to not take
 * 	the address of needs_copy, because it is a bit-field now.
 * 	[89/08/19  23:43:55  rpd]
 * 
 * Revision 2.6  90/01/22  23:09:20  af
 * 	Added vm_map_machine_attributes().
 * 	[90/01/20  17:27:12  af]
 * 
 * Revision 2.5  90/01/19  14:36:05  rwd
 * 	Enter wired pages in destination pmap in vm_move_entry_range, to
 * 	correctly implement wiring semantics.
 * 	[90/01/16            dbg]
 * 
 * Revision 2.4  89/11/29  14:18:19  af
 * 	Redefine VM_PROT_DEFAULT locally for mips.
 * 
 * Revision 2.3  89/09/08  11:28:29  dbg
 * 	Add hack to avoid deadlocking while wiring kernel memory.
 * 	[89/08/31            dbg]
 * 
 * 	Merged with [UNDOCUMENTED!] changes from rfr.
 * 	[89/08/15            dbg]
 * 
 * 	Clip source map entry in vm_move_entry_range, per RFR.  Marking
 * 	the entire data section copy-on-write is costing more than the
 * 	clips (or not being able to collapse the object) ever would.
 * 	[89/07/24            dbg]
 * 
 * 	Add keep_wired parameter to vm_map_move, to wire destination if
 * 	source is wired.
 * 	[89/07/14            dbg]
 * 
 * Revision 2.2  89/08/11  17:57:01  rwd
 * 	Changes for MACH_KERNEL:
 * 	. Break out the inner loop of vm_map_enter, so that
 * 	  kmem_alloc can use it.
 * 	. Add vm_map_move as special case of vm_allocate/vm_map_copy.
 * 	[89/04/28            dbg]
 * 
 * Revision 2.11  89/04/18  21:25:58  mwyoung
 * 	Recent history [mwyoung]:
 * 		Add vm_map_simplify() to keep kernel maps more compact.
 * 	Condensed history:
 * 		Add vm_map_enter(). [mwyoung]
 * 		Return a "version" from vm_map_lookup() to simplify
 * 		 locking. [mwyoung]
 * 		Get pageability changes right.  [dbg, dlb]
 * 		Original implementation.  [avie, mwyoung, dbg]
 * 
 * Revision 2.29.2.1  91/12/09  19:23:39  rpd
 * 	Fixed vm_map_copyout_page_list a la vm_object_coalesce.
 * 	Fixed vm_map_copy_overwrite to check for misalignment.
 * 	Fixed some log infelicities.
 * 	[91/12/09            rpd]
 * 
 * Revision 2.29.1.1  91/12/05  10:56:52  dlb
 * 	4-Dec-91 David L. Black (dlb) at Open Software Foundation
 * 	Rewrite bogus (abort || src_destroy_only) logic in
 * 	vm_map_copyin_page_list_cont.
 * 	Change vm_map_convert_to_page_list routines to avoid
 * 	leaking object references.  Includes a new version of
 * 	vm_map_copy_discard for use as a page list continuation.
 * 	Hold a map reference in vm_map_copyin_page_list continuations.
 * 	Add some checks in vm_map_convert_from_page_list.
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
 *	File:	vm/vm_map.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Virtual memory mapping module.
 */

#include <cpus.h>
#include <norma_vm.h>
#include <task_swapper.h>
#include <mach_assert.h>
#include <dipc.h>

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <mach/vm_behavior.h>
#include <kern/assert.h>
#include <kern/counters.h>
#include <kern/zalloc.h>
#include <vm/vm_init.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>
#include <ipc/ipc_port.h>
#include <kern/misc_protos.h>
#include <mach/mach_server.h>
#include <mach/mach_host_server.h>
#include <ddb/tr.h>
#include <kern/xpr.h>

#if	NORMA_VM
#include <xmm/xmm_server_rename.h>
#include <kern/norma_protos.h>	/* for vm_remap_remote() */
#endif	/* NORMA_VM */

/* Internal prototypes
 */
extern boolean_t vm_map_range_check(
				vm_map_t	map,
				vm_offset_t	start,
				vm_offset_t	end,
				vm_map_entry_t	*entry);

extern vm_map_entry_t	_vm_map_entry_create(
				struct vm_map_header	*map_header);

extern void		_vm_map_entry_dispose(
				struct vm_map_header	*map_header,
				vm_map_entry_t		entry);

extern void		vm_map_pmap_enter(
				vm_map_t	map,
				vm_offset_t 	addr,
				vm_offset_t	end_addr,
				vm_object_t 	object,
				vm_offset_t	offset,
				vm_prot_t	protection);

extern void		_vm_map_clip_end(
				struct vm_map_header	*map_header,
				vm_map_entry_t		entry,
				vm_offset_t		end);

extern void		vm_map_entry_delete(
				vm_map_t	map,
				vm_map_entry_t	entry);

extern kern_return_t	vm_map_delete(
				vm_map_t	map,
				vm_offset_t	start,
				vm_offset_t	end,
				int		flags);

extern void		vm_map_copy_steal_pages(
				vm_map_copy_t	copy);

extern kern_return_t	vm_map_copy_overwrite_unaligned(
				vm_map_t	dst_map,
				vm_map_entry_t	entry,
				vm_map_copy_t	copy,
				vm_offset_t	start);

extern kern_return_t	vm_map_copy_overwrite_aligned(
				vm_map_t	dst_map,
				vm_map_entry_t	tmp_entry,
				vm_map_copy_t	copy,
				vm_offset_t	start);

extern kern_return_t	vm_map_copyout_kernel_buffer(
				vm_map_t	map,
				vm_offset_t	*addr,	/* IN/OUT */
				vm_map_copy_t	copy,
				boolean_t	overwrite);

extern kern_return_t	vm_map_copyin_page_list_cont(
				vm_map_copyin_args_t	cont_args,
				vm_map_copy_t		*copy_result); /* OUT */

extern void		vm_map_fork_share(
				vm_map_t	old_map,
				vm_map_entry_t	old_entry,
				vm_map_t	new_map);

extern boolean_t	vm_map_fork_copy(
				vm_map_t	old_map,
				vm_map_entry_t	*old_entry_p,
				vm_map_t	new_map);

extern kern_return_t	vm_remap_range_allocate(
				vm_map_t	map,
				vm_offset_t	*address,	/* IN/OUT */
				vm_size_t	size,
				vm_offset_t	mask,
				boolean_t	anywhere,
				vm_map_entry_t	*map_entry);	/* OUT */

#if	!NORMA_IPC || !NORMA_VM
extern void		_vm_map_clip_start(
				struct vm_map_header	*map_header,
				vm_map_entry_t		entry,
				vm_offset_t		start);
#endif	/* !NORMA_IPC || !NORMA_VM */

/*
 * Macros to copy a vm_map_entry. We must be careful to correctly
 * manage the wired page count. vm_map_entry_copy() creates a new
 * map entry to the same memory - the wired count in the new entry
 * must be set to zero. vm_map_entry_copy_full() creates a new
 * entry that is identical to the old entry.  This preserves the
 * wire count; it's used for map splitting and zone changing in
 * vm_map_copyout.
 */
#define vm_map_entry_copy(NEW,OLD) \
MACRO_BEGIN                                     \
                *(NEW) = *(OLD);                \
                (NEW)->is_shared = FALSE;	\
                (NEW)->needs_wakeup = FALSE;    \
                (NEW)->in_transition = FALSE;   \
                (NEW)->wired_count = 0;         \
                (NEW)->user_wired_count = 0;    \
MACRO_END

#define vm_map_entry_copy_full(NEW,OLD)        (*(NEW) = *(OLD))

/*
 *	Virtual memory maps provide for the mapping, protection,
 *	and sharing of virtual memory objects.  In addition,
 *	this module provides for an efficient virtual copy of
 *	memory from one map to another.
 *
 *	Synchronization is required prior to most operations.
 *
 *	Maps consist of an ordered doubly-linked list of simple
 *	entries; a single hint is used to speed up lookups.
 *
 *	Sharing maps have been deleted from this version of Mach.
 *	All shared objects are now mapped directly into the respective
 *	maps.  This requires a change in the copy on write strategy;
 *	the asymmetric (delayed) strategy is used for shared temporary
 *	objects instead of the symmetric (shadow) strategy.  All maps
 *	are now "top level" maps (either task map, kernel map or submap
 *	of the kernel map).  
 *
 *	Since portions of maps are specified by start/end addreses,
 *	which may not align with existing map entries, all
 *	routines merely "clip" entries to these start/end values.
 *	[That is, an entry is split into two, bordering at a
 *	start or end value.]  Note that these clippings may not
 *	always be necessary (as the two resulting entries are then
 *	not changed); however, the clipping is done for convenience.
 *	No attempt is currently made to "glue back together" two
 *	abutting entries.
 *
 *	The symmetric (shadow) copy strategy implements virtual copy
 *	by copying VM object references from one map to
 *	another, and then marking both regions as copy-on-write.
 *	It is important to note that only one writeable reference
 *	to a VM object region exists in any map when this strategy
 *	is used -- this means that shadow object creation can be
 *	delayed until a write operation occurs.  The symmetric (delayed)
 *	strategy allows multiple maps to have writeable references to
 *	the same region of a vm object, and hence cannot delay creating
 *	its copy objects.  See vm_object_copy_quickly() in vm_object.c.
 *	Copying of permanent objects is completely different; see
 *	vm_object_copy_strategically() in vm_object.c.
 */

zone_t		vm_map_zone;		/* zone for vm_map structures */
zone_t		vm_map_entry_zone;	/* zone for vm_map_entry structures */
zone_t		vm_map_kentry_zone;	/* zone for kernel entry structures */
zone_t		vm_map_copy_zone;	/* zone for vm_map_copy structures */


/*
 *	Placeholder object for submap operations.  This object is dropped
 *	into the range by a call to vm_map_find, and removed when
 *	vm_map_submap creates the submap.
 */

vm_object_t	vm_submap_object;

/*
 *	vm_map_init:
 *
 *	Initialize the vm_map module.  Must be called before
 *	any other vm_map routines.
 *
 *	Map and entry structures are allocated from zones -- we must
 *	initialize those zones.
 *
 *	There are three zones of interest:
 *
 *	vm_map_zone:		used to allocate maps.
 *	vm_map_entry_zone:	used to allocate map entries.
 *	vm_map_kentry_zone:	used to allocate map entries for the kernel.
 *
 *	The kernel allocates map entries from a special zone that is initially
 *	"crammed" with memory.  It would be difficult (perhaps impossible) for
 *	the kernel to allocate more memory to a entry zone when it became
 *	empty since the very act of allocating memory implies the creation
 *	of a new entry.
 */

vm_offset_t	map_data;
vm_size_t	map_data_size;
vm_offset_t	kentry_data;
vm_size_t	kentry_data_size;
int		kentry_count = 2048;		/* to init kentry_data_size */

/*
 *	Threshold for aggressive (eager) page map entering for vm copyout
 *	operations.  Any copyout larger will NOT be aggressively entered.
 */
vm_size_t vm_map_aggressive_enter_max;		/* set by bootstrap */

void
vm_map_init(
	void)
{
	vm_map_zone = zinit((vm_size_t) sizeof(struct vm_map), 40*1024,
					PAGE_SIZE, "maps");
	vm_map_entry_zone = zinit((vm_size_t) sizeof(struct vm_map_entry),
					1024*1024, PAGE_SIZE*5,
					"non-kernel map entries");
	vm_map_kentry_zone = zinit((vm_size_t) sizeof(struct vm_map_entry),
					kentry_data_size, kentry_data_size,
					"kernel map entries");

	vm_map_copy_zone = zinit((vm_size_t) sizeof(struct vm_map_copy),
					16*1024, PAGE_SIZE, "map copies");

	/*
	 *	Cram the map and kentry zones with initial data.
	 *	Set kentry_zone non-collectible to aid zone_gc().
	 */
	zone_change(vm_map_zone, Z_COLLECT, FALSE);
	zone_change(vm_map_kentry_zone, Z_COLLECT, FALSE);
	zone_change(vm_map_kentry_zone, Z_EXPAND, FALSE);
	zcram(vm_map_zone, map_data, map_data_size);
	zcram(vm_map_kentry_zone, kentry_data, kentry_data_size);
}

void
vm_map_steal_memory(
	void)
{
	map_data_size = round_page(10 * sizeof(struct vm_map));
	map_data = pmap_steal_memory(map_data_size);

	/*
	 * Limiting worst case: vm_map_kentry_zone needs to map each "available"
	 * physical page (i.e. that beyond the kernel image and page tables) 
	 * individually; we fudge this slightly and guess at most one entry per
	 * two pages. This works out to roughly .4 of 1% of physical memory,
	 * or roughly 1800 entries (56K) for a 16M machine with 4K pages.
	 */

	kentry_count = pmap_free_pages() / 2;

	kentry_data_size =
		round_page(kentry_count * sizeof(struct vm_map_entry));
	kentry_data = pmap_steal_memory(kentry_data_size);
}

/*
 *	vm_map_create:
 *
 *	Creates and returns a new empty VM map with
 *	the given physical map structure, and having
 *	the given lower and upper address bounds.
 */
vm_map_t
vm_map_create(
	pmap_t		pmap,
	vm_offset_t	min,
	vm_offset_t	max,
	boolean_t	pageable)
{
	register vm_map_t	result;

	result = (vm_map_t) zalloc(vm_map_zone);
	if (result == VM_MAP_NULL)
		panic("vm_map_create");

	vm_map_first_entry(result) = vm_map_to_entry(result);
	vm_map_last_entry(result)  = vm_map_to_entry(result);
	result->hdr.nentries = 0;
	result->hdr.entries_pageable = pageable;

	result->size = 0;
	result->ref_count = 1;
#if	TASK_SWAPPER
	result->res_count = 1;
	result->sw_state = MAP_SW_IN;
#endif	/* TASK_SWAPPER */
	result->pmap = pmap;
	result->min_offset = min;
	result->max_offset = max;
	result->wiring_required = FALSE;
	result->no_zero_fill = FALSE;
	result->wait_for_space = FALSE;
	result->first_free = vm_map_to_entry(result);
	result->hint = vm_map_to_entry(result);
	vm_map_lock_init(result);
	mutex_init(&result->s_lock, ETAP_VM_RESULT);

	return(result);
}

/*
 *	vm_map_entry_create:	[ internal use only ]
 *
 *	Allocates a VM map entry for insertion in the
 *	given map (or map copy).  No fields are filled.
 */
#define	vm_map_entry_create(map) \
	    _vm_map_entry_create(&(map)->hdr)

#define	vm_map_copy_entry_create(copy) \
	    _vm_map_entry_create(&(copy)->cpy_hdr)

vm_map_entry_t
_vm_map_entry_create(
	register struct vm_map_header	*map_header)
{
	register zone_t	zone;
	register vm_map_entry_t	entry;

	if (map_header->entries_pageable)
	    zone = vm_map_entry_zone;
	else
	    zone = vm_map_kentry_zone;

	entry = (vm_map_entry_t) zalloc(zone);
	if (entry == VM_MAP_ENTRY_NULL)
		panic("vm_map_entry_create");

	return(entry);
}

/*
 *	vm_map_entry_dispose:	[ internal use only ]
 *
 *	Inverse of vm_map_entry_create.
 */
#define	vm_map_entry_dispose(map, entry)		\
MACRO_BEGIN						\
	assert((entry) != (map)->first_free &&		\
	       (entry) != (map)->hint);			\
	_vm_map_entry_dispose(&(map)->hdr, (entry));	\
MACRO_END

#define	vm_map_copy_entry_dispose(map, entry) \
	_vm_map_entry_dispose(&(copy)->cpy_hdr, (entry))

void
_vm_map_entry_dispose(
	register struct vm_map_header	*map_header,
	register vm_map_entry_t		entry)
{
	register zone_t		zone;

	if (map_header->entries_pageable)
	    zone = vm_map_entry_zone;
	else
	    zone = vm_map_kentry_zone;

	zfree(zone, (vm_offset_t) entry);
}

boolean_t first_free_is_valid(vm_map_t map);	/* forward */
boolean_t first_free_check = FALSE;
boolean_t
first_free_is_valid(
	vm_map_t	map)
{
	vm_map_entry_t	entry, next;

	if (!first_free_check)
		return TRUE;
		
	entry = vm_map_to_entry(map);
	next = entry->vme_next;
	while (trunc_page(next->vme_start) == trunc_page(entry->vme_end) ||
	       (trunc_page(next->vme_start) == trunc_page(entry->vme_start) &&
		next != vm_map_to_entry(map))) {
		entry = next;
		next = entry->vme_next;
		if (entry == vm_map_to_entry(map))
			break;
	}
	if (map->first_free != entry) {
		printf("Bad first_free for map 0x%x: 0x%x should be 0x%x\n",
		       map, map->first_free, entry);
		return FALSE;
	}
	return TRUE;
}

/*
 *	UPDATE_FIRST_FREE:
 *
 *	Updates the map->first_free pointer to the
 *	entry immediately before the first hole in the map.
 * 	The map should be locked.
 */
#define UPDATE_FIRST_FREE(map, new_first_free) 				\
MACRO_BEGIN 								\
	vm_map_t	UFF_map; 					\
	vm_map_entry_t	UFF_first_free; 				\
	vm_map_entry_t	UFF_next_entry; 				\
	UFF_map = (map); 						\
	UFF_first_free = (new_first_free);				\
	UFF_next_entry = UFF_first_free->vme_next; 			\
	while (trunc_page(UFF_next_entry->vme_start) == 		\
	       trunc_page(UFF_first_free->vme_end) || 			\
	       (trunc_page(UFF_next_entry->vme_start) == 		\
		trunc_page(UFF_first_free->vme_start) &&		\
		UFF_next_entry != vm_map_to_entry(UFF_map))) { 		\
		UFF_first_free = UFF_next_entry; 			\
		UFF_next_entry = UFF_first_free->vme_next; 		\
		if (UFF_first_free == vm_map_to_entry(UFF_map)) 	\
			break; 						\
	} 								\
	UFF_map->first_free = UFF_first_free; 				\
	assert(first_free_is_valid(UFF_map));				\
MACRO_END

/*
 *	vm_map_entry_{un,}link:
 *
 *	Insert/remove entries from maps (or map copies).
 */
#define vm_map_entry_link(map, after_where, entry)			\
MACRO_BEGIN 								\
	vm_map_t VMEL_map; 						\
	vm_map_entry_t VMEL_entry; 					\
	VMEL_map = (map);						\
	VMEL_entry = (entry); 						\
	_vm_map_entry_link(&VMEL_map->hdr, after_where, VMEL_entry); 	\
	UPDATE_FIRST_FREE(VMEL_map, VMEL_map->first_free); 		\
MACRO_END


#define vm_map_copy_entry_link(copy, after_where, entry)		\
	_vm_map_entry_link(&(copy)->cpy_hdr, after_where, (entry))

#define _vm_map_entry_link(hdr, after_where, entry)			\
	MACRO_BEGIN							\
	(hdr)->nentries++;						\
	(entry)->vme_prev = (after_where);				\
	(entry)->vme_next = (after_where)->vme_next;			\
	(entry)->vme_prev->vme_next = (entry)->vme_next->vme_prev = (entry); \
	MACRO_END

#define vm_map_entry_unlink(map, entry)					\
MACRO_BEGIN 								\
	vm_map_t VMEU_map; 						\
	vm_map_entry_t VMEU_entry; 					\
	vm_map_entry_t VMEU_first_free;					\
	VMEU_map = (map); 						\
	VMEU_entry = (entry); 						\
	if (VMEU_entry->vme_start <= VMEU_map->first_free->vme_start)	\
		VMEU_first_free = VMEU_entry->vme_prev;			\
	else								\
		VMEU_first_free = VMEU_map->first_free;			\
	_vm_map_entry_unlink(&VMEU_map->hdr, VMEU_entry); 		\
	UPDATE_FIRST_FREE(VMEU_map, VMEU_first_free);			\
MACRO_END

#define vm_map_copy_entry_unlink(copy, entry)				\
	_vm_map_entry_unlink(&(copy)->cpy_hdr, (entry))

#define _vm_map_entry_unlink(hdr, entry)				\
	MACRO_BEGIN							\
	(hdr)->nentries--;						\
	(entry)->vme_next->vme_prev = (entry)->vme_prev; 		\
	(entry)->vme_prev->vme_next = (entry)->vme_next; 		\
	MACRO_END

#if	MACH_ASSERT && TASK_SWAPPER
/*
 *	vm_map_reference:
 *
 *	Adds valid reference and residence counts to the given map.
 * 	The map must be in memory (i.e. non-zero residence count).
 *
 */
void
vm_map_reference(
	register vm_map_t	map)
{
	if (map == VM_MAP_NULL)
		return;

	mutex_lock(&map->s_lock);
	assert(map->res_count > 0);
	assert(map->ref_count >= map->res_count);
	map->ref_count++;
	map->res_count++;
	mutex_unlock(&map->s_lock);
}

/*
 *	vm_map_res_reference:
 *
 *	Adds another valid residence count to the given map.
 *
 *	Map is locked so this function can be called from
 *	vm_map_swapin.
 *
 */
void vm_map_res_reference(register vm_map_t map)
{
	/* assert map is locked */
	assert(map->res_count >= 0);
	assert(map->ref_count >= map->res_count);
	if (map->res_count == 0) {
		mutex_unlock(&map->s_lock);
		vm_map_lock(map);
		vm_map_swapin(map);
		mutex_lock(&map->s_lock);
		++map->res_count;
		vm_map_unlock(map);
	} else
		++map->res_count;
}

/*
 *	vm_map_reference_swap:
 *
 *	Adds valid reference and residence counts to the given map.
 *
 *	The map may not be in memory (i.e. zero residence count).
 *
 */
void vm_map_reference_swap(register vm_map_t map)
{
	assert(map != VM_MAP_NULL);
	mutex_lock(&map->s_lock);
	assert(map->res_count >= 0);
	assert(map->ref_count >= map->res_count);
	map->ref_count++;
	vm_map_res_reference(map);
	mutex_unlock(&map->s_lock);
}

/*
 *	vm_map_res_deallocate:
 *
 *	Decrement residence count on a map; possibly causing swapout.
 *
 *	The map must be in memory (i.e. non-zero residence count).
 *
 *	The map is locked, so this function is callable from vm_map_deallocate.
 *
 */
void vm_map_res_deallocate(register vm_map_t map)
{
	assert(map->res_count > 0);
	if (--map->res_count == 0) {
		mutex_unlock(&map->s_lock);
		vm_map_lock(map);
		vm_map_swapout(map);
		vm_map_unlock(map);
		mutex_lock(&map->s_lock);
	}
	assert(map->ref_count >= map->res_count);
}
#endif	/* MACH_ASSERT && TASK_SWAPPER */

/*
 *	vm_map_deallocate:
 *
 *	Removes a reference from the specified map,
 *	destroying it if no references remain.
 *	The map should not be locked.
 */
void
vm_map_deallocate(
	register vm_map_t	map)
{
	register int		c;

	if (map == VM_MAP_NULL)
		return;

	mutex_lock(&map->s_lock);
	if (--map->ref_count > 0) {
		vm_map_res_deallocate(map);
		mutex_unlock(&map->s_lock);
		return;
	}
	assert(map->ref_count == 0);
	mutex_unlock(&map->s_lock);

#if	TASK_SWAPPER
	/*
	 * The map residence count isn't decremented here because
	 * the vm_map_delete below will traverse the entire map, 
	 * deleting entries, and the residence counts on objects
	 * and sharing maps will go away then.
	 */
#endif

	vm_map_destroy(map);
}

/*
 *	vm_map_destroy:
 *
 *	Actually destroy a map.
 */
void
vm_map_destroy(
	register vm_map_t	map)
{
	vm_map_lock(map);
	(void) vm_map_delete(map, map->min_offset,
			     map->max_offset, VM_MAP_NO_FLAGS);
	vm_map_unlock(map);

	pmap_destroy(map->pmap);

	zfree(vm_map_zone, (vm_offset_t) map);
}

#if	TASK_SWAPPER
/*
 * vm_map_swapin/vm_map_swapout
 *
 * Swap a map in and out, either referencing or releasing its resources.  
 * These functions are internal use only; however, they must be exported
 * because they may be called from macros, which are exported.
 *
 * In the case of swapout, there could be races on the residence count, 
 * so if the residence count is up, we return, assuming that a 
 * vm_map_deallocate() call in the near future will bring us back.
 *
 * Locking:
 *	-- We use the map write lock for synchronization among races.
 *	-- The map write lock, and not the simple s_lock, protects the
 *	   swap state of the map.
 *	-- If a map entry is a share map, then we hold both locks, in
 *	   hierarchical order.
 *
 * Synchronization Notes:
 *	1) If a vm_map_swapin() call happens while swapout in progress, it
 *	will block on the map lock and proceed when swapout is through.
 *	2) A vm_map_reference() call at this time is illegal, and will
 *	cause a panic.  vm_map_reference() is only allowed on resident
 *	maps, since it refuses to block.
 *	3) A vm_map_swapin() call during a swapin will block, and 
 *	proceeed when the first swapin is done, turning into a nop.
 *	This is the reason the res_count is not incremented until
 *	after the swapin is complete.
 *	4) There is a timing hole after the checks of the res_count, before
 *	the map lock is taken, during which a swapin may get the lock
 *	before a swapout about to happen.  If this happens, the swapin
 *	will detect the state and increment the reference count, causing
 *	the swapout to be a nop, thereby delaying it until a later 
 *	vm_map_deallocate.  If the swapout gets the lock first, then 
 *	the swapin will simply block until the swapout is done, and 
 *	then proceed.
 *
 * Because vm_map_swapin() is potentially an expensive operation, it
 * should be used with caution.
 *
 * Invariants:
 *	1) A map with a residence count of zero is either swapped, or
 *	   being swapped.
 *	2) A map with a non-zero residence count is either resident,
 *	   or being swapped in.
 */

int vm_map_swap_enable = 1;

void vm_map_swapin (vm_map_t map)
{
	register vm_map_entry_t entry;
	
	if (!vm_map_swap_enable)	/* debug */
		return;

	/*
	 * Map is locked
	 * First deal with various races.
	 */
	if (map->sw_state == MAP_SW_IN)
		/* 
		 * we raced with swapout and won.  Returning will incr.
		 * the res_count, turning the swapout into a nop.
		 */
		return;

	/*
	 * The residence count must be zero.  If we raced with another
	 * swapin, the state would have been IN; if we raced with a
	 * swapout (after another competing swapin), we must have lost
	 * the race to get here (see above comment), in which case
	 * res_count is still 0.
	 */
	assert(map->res_count == 0);

	/*
	 * There are no intermediate states of a map going out or
	 * coming in, since the map is locked during the transition.
	 */
	assert(map->sw_state == MAP_SW_OUT);

	/*
	 * We now operate upon each map entry.  If the entry is a sub- 
	 * or share-map, we call vm_map_res_reference upon it.
	 * If the entry is an object, we call vm_object_res_reference
	 * (this may iterate through the shadow chain).
	 * Note that we hold the map locked the entire time,
	 * even if we get back here via a recursive call in
	 * vm_map_res_reference.
	 */
	entry = vm_map_first_entry(map);

	while (entry != vm_map_to_entry(map)) {
		if (entry->object.vm_object != VM_OBJECT_NULL) {
			assert(!entry->is_sub_map);
#if 0
			if (entry->is_a_map) {
				vm_map_t lmap = entry->object.share_map;
				mutex_lock(&lmap->s_lock);
				vm_map_res_reference(lmap);
				mutex_unlock(&lmap->s_lock);
			} else
#endif
			{
				vm_object_t object = entry->object.vm_object;
				vm_object_lock(object);
				/*
				 * This call may iterate through the
				 * shadow chain.
				 */
				vm_object_res_reference(object);
				vm_object_unlock(object);
			}
		}
		entry = entry->vme_next;
	}
	assert(map->sw_state == MAP_SW_OUT);
	map->sw_state = MAP_SW_IN;
}

void vm_map_swapout(vm_map_t map)
{
	register vm_map_entry_t entry;
	
	/*
	 * Map is locked
	 * First deal with various races.
	 * If we raced with a swapin and lost, the residence count
	 * will have been incremented to 1, and we simply return.
	 */
	mutex_lock(&map->s_lock);
	if (map->res_count != 0) {
		mutex_unlock(&map->s_lock);
		return;
	}
	mutex_unlock(&map->s_lock);

	/*
	 * There are no intermediate states of a map going out or
	 * coming in, since the map is locked during the transition.
	 */
	assert(map->sw_state == MAP_SW_IN);

	if (!vm_map_swap_enable)
		return;

	/*
	 * We now operate upon each map entry.  If the entry is a sub- 
	 * or share-map, we call vm_map_res_deallocate upon it.
	 * If the entry is an object, we call vm_object_res_deallocate
	 * (this may iterate through the shadow chain).
	 * Note that we hold the map locked the entire time,
	 * even if we get back here via a recursive call in
	 * vm_map_res_deallocate.
	 */
	entry = vm_map_first_entry(map);

	while (entry != vm_map_to_entry(map)) {
		if (entry->object.vm_object != VM_OBJECT_NULL) {
			assert(!entry->is_sub_map);
#if 0
			if (entry->is_a_map) {
				vm_map_t lmap = entry->object.share_map;
				mutex_lock(&lmap->s_lock);
				vm_map_res_deallocate(lmap);
				mutex_unlock(&lmap->s_lock);
			} else
#endif
			{
				vm_object_t object = entry->object.vm_object;
				vm_object_lock(object);
				/*
				 * This call may take a long time, 
				 * since it could actively push 
				 * out pages (if we implement it 
				 * that way).
				 */
				vm_object_res_deallocate(object);
				vm_object_unlock(object);
			}
		}
		entry = entry->vme_next;
	}
	assert(map->sw_state == MAP_SW_IN);
	map->sw_state = MAP_SW_OUT;
}

#endif	/* TASK_SWAPPER */


/*
 *	SAVE_HINT:
 *
 *	Saves the specified entry as the hint for
 *	future lookups.  Performs necessary interlocks.
 */
#define	SAVE_HINT(map,value) \
		mutex_lock(&(map)->s_lock); \
		(map)->hint = (value); \
		mutex_unlock(&(map)->s_lock);

/*
 *	vm_map_lookup_entry:	[ internal use only ]
 *
 *	Finds the map entry containing (or
 *	immediately preceding) the specified address
 *	in the given map; the entry is returned
 *	in the "entry" parameter.  The boolean
 *	result indicates whether the address is
 *	actually contained in the map.
 */
boolean_t
vm_map_lookup_entry(
	register vm_map_t	map,
	register vm_offset_t	address,
	vm_map_entry_t		*entry)		/* OUT */
{
	register vm_map_entry_t		cur;
	register vm_map_entry_t		last;

	/*
	 *	Start looking either from the head of the
	 *	list, or from the hint.
	 */

	mutex_lock(&map->s_lock);
	cur = map->hint;
	mutex_unlock(&map->s_lock);

	if (cur == vm_map_to_entry(map))
		cur = cur->vme_next;

	if (address >= cur->vme_start) {
	    	/*
		 *	Go from hint to end of list.
		 *
		 *	But first, make a quick check to see if
		 *	we are already looking at the entry we
		 *	want (which is usually the case).
		 *	Note also that we don't need to save the hint
		 *	here... it is the same hint (unless we are
		 *	at the header, in which case the hint didn't
		 *	buy us anything anyway).
		 */
		last = vm_map_to_entry(map);
		if ((cur != last) && (cur->vme_end > address)) {
			*entry = cur;
			return(TRUE);
		}
	}
	else {
	    	/*
		 *	Go from start to hint, *inclusively*
		 */
		last = cur->vme_next;
		cur = vm_map_first_entry(map);
	}

	/*
	 *	Search linearly
	 */

	while (cur != last) {
		if (cur->vme_end > address) {
			if (address >= cur->vme_start) {
			    	/*
				 *	Save this lookup for future
				 *	hints, and return
				 */

				*entry = cur;
				SAVE_HINT(map, cur);
				return(TRUE);
			}
			break;
		}
		cur = cur->vme_next;
	}
	*entry = cur->vme_prev;
	SAVE_HINT(map, *entry);
	return(FALSE);
}

/*
 *	Routine:	vm_map_find_space
 *	Purpose:
 *		Allocate a range in the specified virtual address map,
 *		returning the entry allocated for that range.
 *		Used by kmem_alloc, etc.
 *
 *		The map must be NOT be locked. It will be returned locked
 *		on KERN_SUCCESS, unlocked on failure.
 *
 *		If an entry is allocated, the object/offset fields
 *		are initialized to zero.
 */
kern_return_t
vm_map_find_space(
	register vm_map_t	map,
	vm_offset_t		*address,	/* OUT */
	vm_size_t		size,
	vm_offset_t		mask,
	vm_map_entry_t		*o_entry)	/* OUT */
{
	register vm_map_entry_t	entry, new_entry;
	register vm_offset_t	start;
	register vm_offset_t	end;

	new_entry = vm_map_entry_create(map);

	/*
	 *	Look for the first possible address; if there's already
	 *	something at this address, we have to start after it.
	 */

	vm_map_lock(map);

	assert(first_free_is_valid(map));
	if ((entry = map->first_free) == vm_map_to_entry(map))
		start = map->min_offset;
	else
		start = entry->vme_end;

	/*
	 *	In any case, the "entry" always precedes
	 *	the proposed new region throughout the loop:
	 */

	while (TRUE) {
		register vm_map_entry_t	next;

		/*
		 *	Find the end of the proposed new region.
		 *	Be sure we didn't go beyond the end, or
		 *	wrap around the address.
		 */

		end = ((start + mask) & ~mask);
		if (end < start) {
			vm_map_entry_dispose(map, new_entry);
			vm_map_unlock(map);
			return(KERN_NO_SPACE);
		}
		start = end;
		end += size;

		if ((end > map->max_offset) || (end < start)) {
			vm_map_entry_dispose(map, new_entry);
			vm_map_unlock(map);
			return(KERN_NO_SPACE);
		}

		/*
		 *	If there are no more entries, we must win.
		 */

		next = entry->vme_next;
		if (next == vm_map_to_entry(map))
			break;

		/*
		 *	If there is another entry, it must be
		 *	after the end of the potential new region.
		 */

		if (next->vme_start >= end)
			break;

		/*
		 *	Didn't fit -- move to the next entry.
		 */

		entry = next;
		start = entry->vme_end;
	}

	/*
	 *	At this point,
	 *		"start" and "end" should define the endpoints of the
	 *			available new range, and
	 *		"entry" should refer to the region before the new
	 *			range, and
	 *
	 *		the map should be locked.
	 */

	*address = start;

	new_entry->vme_start = start;
	new_entry->vme_end = end;
	assert(page_aligned(new_entry->vme_start));
	assert(page_aligned(new_entry->vme_end));

	new_entry->is_shared = FALSE;
	new_entry->is_sub_map = FALSE;
	new_entry->object.vm_object = VM_OBJECT_NULL;
	new_entry->offset = (vm_offset_t) 0;

	new_entry->needs_copy = FALSE;

	new_entry->inheritance = VM_INHERIT_DEFAULT;
	new_entry->protection = VM_PROT_DEFAULT;
	new_entry->max_protection = VM_PROT_ALL;
	new_entry->behavior = VM_BEHAVIOR_DEFAULT;
	new_entry->wired_count = 0;
	new_entry->user_wired_count = 0;

	new_entry->in_transition = FALSE;
	new_entry->needs_wakeup = FALSE;

	/*
	 *	Insert the new entry into the list
	 */

	vm_map_entry_link(map, entry, new_entry);

	map->size += size;

	/*
	 *	Update the lookup hint
	 */
	SAVE_HINT(map, new_entry);

	*o_entry = new_entry;
	return(KERN_SUCCESS);
}

int vm_map_pmap_enter_print = FALSE;
int vm_map_pmap_enter_enable = FALSE;

/*
 *	Routine:	vm_map_pmap_enter
 *
 *	Description:
 *		Force pages from the specified object to be entered into
 *		the pmap at the specified address if they are present.
 *		As soon as a page not found in the object the scan ends.
 *
 *	Returns:
 *		Nothing.  
 *
 *	In/out conditions:
 *		The source map should not be locked on entry.
 */
void
vm_map_pmap_enter(
	vm_map_t		map,
	register vm_offset_t 	addr,
	register vm_offset_t	end_addr,
	register vm_object_t 	object,
	vm_offset_t		offset,
	vm_prot_t		protection)
{
	while (addr < end_addr) {
		register vm_page_t	m;

		vm_object_lock(object);
		vm_object_paging_begin(object);

		m = vm_page_lookup(object, offset);
		if (m == VM_PAGE_NULL || m->busy ||
		    (m->unusual && ( m->error || m->restart || m->absent ||
				    protection & m->page_lock))) {

			vm_object_paging_end(object);
			vm_object_unlock(object);
			return;
		}

		assert(!m->fictitious);	/* XXX is this possible ??? */

		if (vm_map_pmap_enter_print) {
			printf("vm_map_pmap_enter:");
			printf("map: %x, addr: %x, object: %x, offset: %x\n",
				map, addr, object, offset);
		}

		m->busy = TRUE;
		vm_object_unlock(object);

		PMAP_ENTER(map->pmap, addr, m,
			   protection, FALSE);

		vm_object_lock(object);
		PAGE_WAKEUP_DONE(m);
		vm_page_lock_queues();
		if (!m->active && !m->inactive)
		    vm_page_activate(m);
		vm_page_unlock_queues();
		vm_object_paging_end(object);
		vm_object_unlock(object);

		offset += PAGE_SIZE;
		addr += PAGE_SIZE;
	}
}

/*
 *	Routine:	vm_map_enter
 *
 *	Description:
 *		Allocate a range in the specified virtual address map.
 *		The resulting range will refer to memory defined by
 *		the given memory object and offset into that object.
 *
 *		Arguments are as defined in the vm_map call.
 */
kern_return_t
vm_map_enter(
	register vm_map_t	map,
	vm_offset_t		*address,	/* IN/OUT */
	vm_size_t		size,
	vm_offset_t		mask,
	boolean_t		anywhere,
	vm_object_t		object,
	vm_offset_t		offset,
	boolean_t		needs_copy,
	vm_prot_t		cur_protection,
	vm_prot_t		max_protection,
	vm_inherit_t		inheritance)
{
	vm_map_entry_t		entry;
	register vm_offset_t	start;
	register vm_offset_t	end;
	kern_return_t		result = KERN_SUCCESS;

#define	RETURN(value)	{ result = value; goto BailOut; }

	assert(page_aligned(*address));
	assert(page_aligned(size));
 StartAgain: ;

	start = *address;

	if (anywhere) {
		vm_map_lock(map);

		/*
		 *	Calculate the first possible address.
		 */

		if (start < map->min_offset)
			start = map->min_offset;
		if (start > map->max_offset)
			RETURN(KERN_NO_SPACE);

		/*
		 *	Look for the first possible address;
		 *	if there's already something at this
		 *	address, we have to start after it.
		 */

		assert(first_free_is_valid(map));
		if (start == map->min_offset) {
			if ((entry = map->first_free) != vm_map_to_entry(map))
				start = entry->vme_end;
		} else {
			vm_map_entry_t	tmp_entry;
			if (vm_map_lookup_entry(map, start, &tmp_entry))
				start = tmp_entry->vme_end;
			entry = tmp_entry;
		}

		/*
		 *	In any case, the "entry" always precedes
		 *	the proposed new region throughout the
		 *	loop:
		 */

		while (TRUE) {
			register vm_map_entry_t	next;

		    	/*
			 *	Find the end of the proposed new region.
			 *	Be sure we didn't go beyond the end, or
			 *	wrap around the address.
			 */

			end = ((start + mask) & ~mask);
			if (end < start)
				return(KERN_NO_SPACE);
			start = end;
			end += size;

			if ((end > map->max_offset) || (end < start)) {
				if (map->wait_for_space) {
					if (size <= (map->max_offset -
						     map->min_offset)) {
						assert_wait((event_t)map,TRUE);
						vm_map_unlock(map);
						thread_block((void (*)(void))0);
						goto StartAgain;
					}
				}
				RETURN(KERN_NO_SPACE);
			}

			/*
			 *	If there are no more entries, we must win.
			 */

			next = entry->vme_next;
			if (next == vm_map_to_entry(map))
				break;

			/*
			 *	If there is another entry, it must be
			 *	after the end of the potential new region.
			 */

			if (next->vme_start >= end)
				break;

			/*
			 *	Didn't fit -- move to the next entry.
			 */

			entry = next;
			start = entry->vme_end;
		}
		*address = start;
	} else {
		vm_map_entry_t		temp_entry;

		/*
		 *	Verify that:
		 *		the address doesn't itself violate
		 *		the mask requirement.
		 */

		vm_map_lock(map);
		if ((start & mask) != 0)
			RETURN(KERN_NO_SPACE);

		/*
		 *	...	the address is within bounds
		 */

		end = start + size;

		if ((start < map->min_offset) ||
		    (end > map->max_offset) ||
		    (start >= end)) {
			RETURN(KERN_INVALID_ADDRESS);
		}

		/*
		 *	...	the starting address isn't allocated
		 */

		if (vm_map_lookup_entry(map, start, &temp_entry))
			RETURN(KERN_NO_SPACE);

		entry = temp_entry;

		/*
		 *	...	the next region doesn't overlap the
		 *		end point.
		 */

		if ((entry->vme_next != vm_map_to_entry(map)) &&
		    (entry->vme_next->vme_start < end))
			RETURN(KERN_NO_SPACE);
	}

	/*
	 *	At this point,
	 *		"start" and "end" should define the endpoints of the
	 *			available new range, and
	 *		"entry" should refer to the region before the new
	 *			range, and
	 *
	 *		the map should be locked.
	 */

	/*
	 *	See whether we can avoid creating a new entry (and object) by
	 *	extending one of our neighbors.  [So far, we only attempt to
	 *	extend from below.]
	 */

	if ((object == VM_OBJECT_NULL) &&
	    (entry != vm_map_to_entry(map)) &&
	    (entry->vme_end == start) &&
	    (!entry->is_shared) &&
	    (!entry->is_sub_map) &&
	    (entry->inheritance == inheritance) &&
	    (entry->protection == cur_protection) &&
	    (entry->max_protection == max_protection) &&
	    (entry->behavior == VM_BEHAVIOR_DEFAULT) &&
	    (entry->in_transition == 0) &&
	    (entry->wired_count == 0)) { /* implies user_wired_count == 0 */
		if (vm_object_coalesce(entry->object.vm_object,
				VM_OBJECT_NULL,
				entry->offset,
				(vm_offset_t) 0,
				(vm_size_t)(entry->vme_end - entry->vme_start),
				(vm_size_t)(end - entry->vme_end))) {

			/*
			 *	Coalesced the two objects - can extend
			 *	the previous map entry to include the
			 *	new range.
			 */
			map->size += (end - entry->vme_end);
			entry->vme_end = end;
			UPDATE_FIRST_FREE(map, map->first_free);
			RETURN(KERN_SUCCESS);
		}
	}

	/*
	 *	Create a new entry
	 */

	{ /**/
	register vm_map_entry_t	new_entry;

	new_entry = vm_map_entry_insert(map, entry, start, end, object,
					offset, needs_copy, FALSE, FALSE,
					cur_protection, max_protection,
					VM_BEHAVIOR_DEFAULT, inheritance, 0);
	vm_map_unlock(map);

	/*	Wire down the new entry if the user
	 *	requested all new map entries be wired.
	 */
	if (map->wiring_required) {
		result = vm_map_wire(map, start, end,
				    new_entry->protection, TRUE);
		return(result);
	}

	if ((object != VM_OBJECT_NULL) &&
	    (vm_map_pmap_enter_enable) &&
	    (!anywhere)	 &&
	    (!needs_copy) && 
	    (size < (128*1024))) {
		vm_map_pmap_enter(map, start, end, 
				  object, offset, cur_protection);
	}

	return(result);
	} /**/

 BailOut: ;
	vm_map_unlock(map);
	return(result);

#undef	RETURN
}

/*
 *	vm_map_clip_start:	[ internal use only ]
 *
 *	Asserts that the given entry begins at or after
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
#define vm_map_clip_start(map, entry, startaddr) 			\
MACRO_BEGIN 								\
	vm_map_t VMCS_map;						\
	vm_map_entry_t VMCS_entry;					\
	vm_offset_t VMCS_startaddr;					\
	VMCS_map = (map);						\
	VMCS_entry = (entry);						\
	VMCS_startaddr = (startaddr);					\
	if (VMCS_startaddr > VMCS_entry->vme_start) 			\
		_vm_map_clip_start(&VMCS_map->hdr,VMCS_entry,VMCS_startaddr);\
	UPDATE_FIRST_FREE(VMCS_map, VMCS_map->first_free);		\
MACRO_END

#define vm_map_copy_clip_start(copy, entry, startaddr) \
	MACRO_BEGIN \
	if ((startaddr) > (entry)->vme_start) \
		_vm_map_clip_start(&(copy)->cpy_hdr,(entry),(startaddr)); \
	MACRO_END

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */
void
_vm_map_clip_start(
	register struct vm_map_header	*map_header,
	register vm_map_entry_t		entry,
	register vm_offset_t		start)
{
	register vm_map_entry_t	new_entry;

	/*
	 *	Split off the front portion --
	 *	note that we must insert the new
	 *	entry BEFORE this one, so that
	 *	this entry has the specified starting
	 *	address.
	 */

	new_entry = _vm_map_entry_create(map_header);
	vm_map_entry_copy_full(new_entry, entry);

	new_entry->vme_end = start;
	entry->offset += (start - entry->vme_start);
	entry->vme_start = start;

	_vm_map_entry_link(map_header, entry->vme_prev, new_entry);

	if (entry->is_sub_map)
	 	vm_map_reference(new_entry->object.sub_map);
	else
		vm_object_reference(new_entry->object.vm_object);
}

/*
 *	vm_map_clip_end:	[ internal use only ]
 *
 *	Asserts that the given entry ends at or before
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
#define vm_map_clip_end(map, entry, endaddr) 				\
MACRO_BEGIN 								\
	vm_map_t VMCE_map;						\
	vm_map_entry_t VMCE_entry;					\
	vm_offset_t VMCE_endaddr;					\
	VMCE_map = (map);						\
	VMCE_entry = (entry);						\
	VMCE_endaddr = (endaddr);					\
	if (VMCE_endaddr < VMCE_entry->vme_end) 			\
		_vm_map_clip_end(&VMCE_map->hdr,VMCE_entry,VMCE_endaddr); \
	UPDATE_FIRST_FREE(VMCE_map, VMCE_map->first_free);		\
MACRO_END

#define vm_map_copy_clip_end(copy, entry, endaddr) \
	MACRO_BEGIN \
	if ((endaddr) < (entry)->vme_end) \
		_vm_map_clip_end(&(copy)->cpy_hdr,(entry),(endaddr)); \
	MACRO_END

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */
void
_vm_map_clip_end(
	register struct vm_map_header	*map_header,
	register vm_map_entry_t		entry,
	register vm_offset_t		end)
{
	register vm_map_entry_t	new_entry;

	/*
	 *	Create a new entry and insert it
	 *	AFTER the specified entry
	 */

	new_entry = _vm_map_entry_create(map_header);
	vm_map_entry_copy_full(new_entry, entry);

	new_entry->vme_start = entry->vme_end = end;
	new_entry->offset += (end - entry->vme_start);

	_vm_map_entry_link(map_header, entry, new_entry);

	if (entry->is_sub_map)
	 	vm_map_reference(new_entry->object.sub_map);
	else
		vm_object_reference(new_entry->object.vm_object);
}


/*
 *	VM_MAP_RANGE_CHECK:	[ internal use only ]
 *
 *	Asserts that the starting and ending region
 *	addresses fall within the valid range of the map.
 */
#define	VM_MAP_RANGE_CHECK(map, start, end)		\
		{					\
		if (start < vm_map_min(map))		\
			start = vm_map_min(map);	\
		if (end > vm_map_max(map))		\
			end = vm_map_max(map);		\
		if (start > end)			\
			start = end;			\
		}

/*
 *	vm_map_range_check:	[ internal use only ]
 *	
 *	Check that the region defined by the specified start and
 *	end addresses are wholly contained within a single map
 *	entry or set of adjacent map entries of the spacified map,
 *	i.e. the specified region contains no unmapped space.
 *	If any or all of the region is unmapped, FALSE is returned.
 *	Otherwise, TRUE is returned and if the output argument 'entry'
 *	is not NULL it points to the map entry containing the start
 *	of the region.
 *
 *	The map is locked for reading on entry and is left locked.
 */
boolean_t
vm_map_range_check(
	register vm_map_t	map,
	register vm_offset_t	start,
	register vm_offset_t	end,
	vm_map_entry_t		*entry)
{
	vm_map_entry_t		cur;
	register vm_offset_t	prev;

	/*
	 * 	Basic sanity checks first
	 */
	if (start < vm_map_min(map) || end > vm_map_max(map) || start > end)
		return (FALSE);

	/*
	 * 	Check first if the region starts within a valid
	 *	mapping for the map.
	 */
	if (!vm_map_lookup_entry(map, start, &cur))
		return (FALSE);

	/*
	 *	Optimize for the case that the region is contained 
	 *	in a single map entry.
	 */
	if (entry != (vm_map_entry_t *) NULL)
		*entry = cur;
	if (end <= cur->vme_end)
		return (TRUE);

	/*
	 * 	If the region is not wholly contained within a
	 * 	single entry, walk the entries looking for holes.
	 */
	prev = cur->vme_end;
	cur = cur->vme_next;
	while ((cur != vm_map_to_entry(map)) && (prev == cur->vme_start)) {
		if (end <= cur->vme_end)
			return (TRUE);
		prev = cur->vme_end;
		cur = cur->vme_next;
	}
	return (FALSE);
}

/*
 *	vm_map_submap:		[ kernel use only ]
 *
 *	Mark the given range as handled by a subordinate map.
 *
 *	This range must have been created with vm_map_find using
 *	the vm_submap_object, and no other operations may have been
 *	performed on this range prior to calling vm_map_submap.
 *
 *	Only a limited number of operations can be performed
 *	within this rage after calling vm_map_submap:
 *		vm_fault
 *	[Don't try vm_map_copyin!]
 *
 *	To remove a submapping, one must first remove the
 *	range from the superior map, and then destroy the
 *	submap (if desired).  [Better yet, don't try it.]
 */
kern_return_t
vm_map_submap(
	register vm_map_t	map,
	register vm_offset_t	start,
	register vm_offset_t	end,
	vm_map_t		submap)
{
	vm_map_entry_t		entry;
	register kern_return_t	result = KERN_INVALID_ARGUMENT;
	register vm_object_t	object;

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &entry)) {
		vm_map_clip_start(map, entry, start);
	}
	 else
		entry = entry->vme_next;

	vm_map_clip_end(map, entry, end);

	if ((entry->vme_start == start) && (entry->vme_end == end) &&
	    (!entry->is_sub_map) &&
	    ((object = entry->object.vm_object) == vm_submap_object) &&
	    (object->resident_page_count == 0) &&
#if	!NORMA_VM
	    (object->copy == VM_OBJECT_NULL) &&
#endif	/* !NORMA_VM */
	    (object->shadow == VM_OBJECT_NULL) &&
	    (!object->pager_created)) {
		entry->object.vm_object = VM_OBJECT_NULL;
		vm_object_deallocate(object);
		entry->is_sub_map = TRUE;
		vm_map_reference(entry->object.sub_map = submap);
		result = KERN_SUCCESS;
	}
	vm_map_unlock(map);

	return(result);
}

/*
 *	vm_map_protect:
 *
 *	Sets the protection of the specified address
 *	region in the target map.  If "set_max" is
 *	specified, the maximum protection is to be set;
 *	otherwise, only the current protection is affected.
 */
kern_return_t
vm_map_protect(
	register vm_map_t	map,
	register vm_offset_t	start,
	register vm_offset_t	end,
	register vm_prot_t	new_prot,
	register boolean_t	set_max)
{
	register vm_map_entry_t		current;
	register vm_offset_t		prev;
	vm_map_entry_t			entry;
	boolean_t			clip;

	XPR(XPR_VM_MAP,
		"vm_map_protect, 0x%X start 0x%X end 0x%X, new 0x%X %d",
		(integer_t)map, start, end, new_prot, set_max);

	vm_map_lock(map);

	/*
	 * 	Lookup the entry.  If it doesn't start in a valid
	 *	entry, return an error.  Remember if we need to
	 *	clip the entry.  We don't do it here because we don't
	 *	want to make any changes until we've scanned the 
	 *	entire range below for address and protection
	 *	violations.
	 */
	if (!(clip = vm_map_lookup_entry(map, start, &entry))) {
		vm_map_unlock(map);
		return(KERN_INVALID_ADDRESS);
	}

	/*
	 *	Make a first pass to check for protection and address
	 *	violations.
	 */

	current = entry;
	prev = current->vme_start;
	while ((current != vm_map_to_entry(map)) &&
	       (current->vme_start < end)) {

		if (current->is_sub_map) {
			vm_map_unlock(map);
			return(KERN_INVALID_ARGUMENT);
		}

		/*
		 * If there is a hole, return an error.
		 */
		if (current->vme_start != prev) {
			vm_map_unlock(map);
			return(KERN_INVALID_ADDRESS);
		}

		if ((new_prot & current->max_protection) != new_prot) {
			vm_map_unlock(map);
			return(KERN_PROTECTION_FAILURE);
		}

		prev = current->vme_end;
		current = current->vme_next;
	}
	if (end > prev) {
		vm_map_unlock(map);
		return(KERN_INVALID_ADDRESS);
	}

	/*
	 *	Go back and fix up protections.
	 *	Clip to start here if the range starts within
	 *	the entry.
	 */

	current = entry;
	if (clip) {
		vm_map_clip_start(map, entry, start);
	}
	while ((current != vm_map_to_entry(map)) &&
	       (current->vme_start < end)) {

		vm_prot_t	old_prot;

		vm_map_clip_end(map, current, end);

		old_prot = current->protection;
		if (set_max)
			current->protection =
				(current->max_protection = new_prot) &
					old_prot;
		else
			current->protection = new_prot;

		/*
		 *	Update physical map if necessary.
		 */

		if (current->protection != old_prot) {
			pmap_protect(map->pmap, current->vme_start,
					current->vme_end,
					current->protection);
		}
		current = current->vme_next;
	}

	vm_map_unlock(map);
	return(KERN_SUCCESS);
}


/*
 *	vm_map_inherit:
 *
 *	Sets the inheritance of the specified address
 *	range in the target map.  Inheritance
 *	affects how the map will be shared with
 *	child maps at the time of vm_map_fork.
 */
kern_return_t
vm_map_inherit(
	register vm_map_t	map,
	register vm_offset_t	start,
	register vm_offset_t	end,
	register vm_inherit_t	new_inheritance)
{
	register vm_map_entry_t	entry;
	vm_map_entry_t	temp_entry;

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &temp_entry)) {
		entry = temp_entry;
		vm_map_clip_start(map, entry, start);
	}
	else
		entry = temp_entry->vme_next;

	while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {
		vm_map_clip_end(map, entry, end);

		entry->inheritance = new_inheritance;

		entry = entry->vme_next;
	}

	vm_map_unlock(map);
	return(KERN_SUCCESS);
}

/*
 *	vm_map_wire:
 *
 *	Sets the pageability of the specified address range in the
 *	target map as wired.  Regions specified as not pageable require
 *	locked-down physical memory and physical page maps.  The
 *	access_type variable indicates types of accesses that must not
 *	generate page faults.  This is checked against protection of
 *	memory being locked-down.
 *
 *	The map must not be locked, but a reference must remain to the
 *	map throughout the call.
 */
kern_return_t
vm_map_wire(
	register vm_map_t	map,
	register vm_offset_t	start,
	register vm_offset_t	end,
	register vm_prot_t	access_type,
	boolean_t		user_wire)
{
	register vm_map_entry_t	entry;
	struct vm_map_entry	*first_entry, tmp_entry;
	register vm_offset_t	s, e;
	kern_return_t		rc;
	boolean_t		need_wakeup;
	unsigned int		last_timestamp;
	vm_size_t		size;

	vm_map_lock(map);
	last_timestamp = map->timestamp;

	VM_MAP_RANGE_CHECK(map, start, end);
	assert(page_aligned(start));
	assert(page_aligned(end));

	if (vm_map_lookup_entry(map, start, &first_entry)) {
		entry = first_entry;
		/* vm_map_clip_start will be done later. */
	} else {
		/* Start address is not in map */
		vm_map_unlock(map);
		return(KERN_INVALID_ADDRESS);
	}

	need_wakeup = FALSE;
	while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {
		assert(!entry->is_sub_map);

		/*
		 * If another thread is wiring/unwiring this entry then
		 * block after informing other thread to wake us up.
		 */
		if (entry->in_transition) {
			/*
			 * We have not clipped the entry.  Make sure that
			 * the start address is in range so that the lookup
			 * below will succeed.
			 */
			s = entry->vme_start < start? start: entry->vme_start;

			entry->needs_wakeup = TRUE;

			/*
			 * wake up anybody waiting on entries that we have
			 * already wired.
			 */
			if (need_wakeup) {
				vm_map_entry_wakeup(map);
				need_wakeup = FALSE;
			}
			/*
			 * User wiring is interruptible
			 */
			vm_map_entry_wait(map, user_wire);
			if (user_wire && current_thread()->wait_result ==
							THREAD_INTERRUPTED) {
				/*
				 * undo the wirings we have done so far
				 * We do not clear the needs_wakeup flag,
				 * because we cannot tell if we were the
				 * only one waiting.
				 */
				vm_map_unwire(map, start, s, user_wire);
				return(KERN_FAILURE);
			}

			vm_map_lock(map);
			/*
			 * Cannot avoid a lookup here. reset timestamp.
			 */
			last_timestamp = map->timestamp;

			/*
			 * The entry could have been clipped, look it up again.
			 * Worse that can happen is, it may not exist anymore.
			 */
			if (!vm_map_lookup_entry(map, s, &first_entry)) {
				if (!user_wire)
					panic("vm_map_wire: re-lookup failed");

				/*
				 * User: undo everything upto the previous
				 * entry.  let vm_map_unwire worry about
				 * checking the validity of the range.
				 */
				vm_map_unlock(map);
				vm_map_unwire(map, start, s, user_wire);
				return(KERN_FAILURE);
			}
			entry = first_entry;
			continue;
		}
		
		/*
		 * If this entry is already wired then increment
		 * the appropriate wire reference count.
		 */
		if (entry->wired_count) {
			/* sanity check: wired_count is a short */
			if (entry->wired_count >= MAX_WIRE_COUNT)
				panic("vm_map_wire: too many wirings");

			if (user_wire &&
			    entry->user_wired_count >= MAX_WIRE_COUNT) {
				vm_map_unlock(map);
				vm_map_unwire(map, start,
						entry->vme_start, user_wire);
				return(KERN_FAILURE);
			}
			/*
			 * entry is already wired down, get our reference
			 * after clipping to our range.
			 */
			vm_map_clip_start(map, entry, start);
			vm_map_clip_end(map, entry, end);
			if (!user_wire || (entry->user_wired_count++ == 0))
				entry->wired_count++;

			entry = entry->vme_next;
			continue;
		}

		/*
		 * Unwired entry
		 */


		/*
		 * Perform actions of vm_map_lookup that need the write
		 * lock on the map: create a shadow object for a
		 * copy-on-write region, or an object for a zero-fill
		 * region.
		 */
		size = entry->vme_end - entry->vme_start;
		/*
		 * If wiring a copy-on-write page, we need to copy it now
		 * even if we're only (currently) requesting read access.
		 * This is aggressive, but once it's wired we can't move it.
		 */
		if (entry->needs_copy) {
			vm_object_shadow(&entry->object.vm_object,
					 &entry->offset, size);
			entry->needs_copy = FALSE;
		} else if (entry->object.vm_object == VM_OBJECT_NULL) {
			entry->object.vm_object = vm_object_allocate(size);
			entry->offset = (vm_offset_t)0;
		}

		vm_map_clip_start(map, entry, start);
		vm_map_clip_end(map, entry, end);

		s = entry->vme_start;
		e = entry->vme_end;

		/*
		 * Check for holes and protection mismatch.
		 * Holes: Next entry should be contiguous unless this
		 *	  is the end of the region.
		 * Protection: Access requested must be allowed, unless
		 *	wiring is by protection class
		 */
		if ((((entry->vme_end < end) &&
		     ((entry->vme_next == vm_map_to_entry(map)) ||
		      (entry->vme_next->vme_start > entry->vme_end))) ||
		     ((entry->protection & access_type) != access_type))) {
			/*
			 * Found a hole or protection problem.
			 * Unwire the region we wired so far.
			 */
			if (start != entry->vme_start) {
				vm_map_unlock(map);
				vm_map_unwire(map, start, s, user_wire);
			} else {
				vm_map_unlock(map);
			}
			return((entry->protection&access_type) != access_type?
				KERN_PROTECTION_FAILURE: KERN_INVALID_ADDRESS);
		}

		assert(entry->wired_count == 0 && entry->user_wired_count == 0);

		if (user_wire)
			entry->user_wired_count++;
		entry->wired_count++;

		entry->in_transition = TRUE;

		/*
		 * This entry might get split once we unlock the map.
		 * In vm_fault_wire(), we need the current range as
		 * defined by this entry.  In order for this to work
		 * along with a simultaneous clip operation, we make a
		 * temporary copy of this entry and use that for the
		 * wiring.  Note that the underlying objects do not
		 * change during a clip.
		 */
		tmp_entry = *entry;

		/*
		 * The in_transition state guarentees that the entry
		 * (or entries for this range, if split occured) will be
		 * there when the map lock is acquired for the second time.
		 */
		vm_map_unlock(map);
		rc = vm_fault_wire(map, &tmp_entry);
		vm_map_lock(map);

		if (last_timestamp+1 != map->timestamp) {
			/*
			 * Find the entry again.  It could have been clipped
			 * after we unlocked the map.
			 */
			if (!vm_map_lookup_entry(map, tmp_entry.vme_start,
								&first_entry))
				panic("vm_map_wire: re-lookup failed");

			entry = first_entry;
		}

		last_timestamp = map->timestamp;

		while ((entry != vm_map_to_entry(map)) &&
		       (entry->vme_start < tmp_entry.vme_end)) {
			assert(entry->in_transition);
			entry->in_transition = FALSE;
			if (entry->needs_wakeup) {
				entry->needs_wakeup = FALSE;
				need_wakeup = TRUE;
			}
			if (rc != KERN_SUCCESS) {	/* from vm_*_wire */
				if (user_wire)
					entry->user_wired_count--;
				entry->wired_count--;
			}
			entry = entry->vme_next;
		}

		if (rc != KERN_SUCCESS) {		/* from vm_*_wire */
			vm_map_unlock(map);
			if (need_wakeup)
				vm_map_entry_wakeup(map);
			/*
			 * undo everything upto the previous entry.
			 */
			(void)vm_map_unwire(map, start, s, user_wire);
			return rc;
		}
	} /* end while loop through map entries */
	vm_map_unlock(map);

	/*
	 * wake up anybody waiting on entries we wired.
	 */
	if (need_wakeup)
		vm_map_entry_wakeup(map);

	return(KERN_SUCCESS);

}


/*
 *	vm_map_unwire:
 *
 *	Sets the pageability of the specified address range in the target
 *	as pageable.  Regions specified must have been wired previously.
 *
 *	The map must not be locked, but a reference must remain to the map
 *	throughout the call.
 *
 *	Kernel will panic on failures.  User unwire ignores holes and
 *	unwired and intransition entries to avoid losing memory by leaving
 *	it unwired.
 */
kern_return_t
vm_map_unwire(
	register vm_map_t	map,
	register vm_offset_t	start,
	register vm_offset_t	end,
	boolean_t		user_wire)
{
	register vm_map_entry_t	entry;
	struct vm_map_entry	*first_entry, tmp_entry;
	boolean_t		need_wakeup;
	unsigned int		last_timestamp;

	vm_map_lock(map);
	last_timestamp = map->timestamp;

	VM_MAP_RANGE_CHECK(map, start, end);
	assert(page_aligned(start));
	assert(page_aligned(end));

	if (vm_map_lookup_entry(map, start, &first_entry)) {
		entry = first_entry;
		/*	vm_map_clip_start will be done later. */
	}
	else {
		/*	Start address is not in map. */
		vm_map_unlock(map);
		return(KERN_INVALID_ADDRESS);
	}

	need_wakeup = FALSE;
	while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {
		assert(!entry->is_sub_map);

		if (entry->in_transition) {
			/*
			 * 1)
			 * Another thread is wiring down this entry. Note
			 * that if it is not for the other thread we would
			 * be unwiring an unwired entry.  This is not
			 * permitted.  If we wait, we will be unwiring memory
			 * we did not wire.
			 *
			 * 2)
			 * Another thread is unwiring this entry.  We did not
			 * have a reference to it, because if we did, this
			 * entry will not be getting unwired now.
			 */
			if (!user_wire)
				panic("vm_map_unwire: in_transition entry");

			entry = entry->vme_next;
			continue;
		}

		if (entry->wired_count == 0 ||
		   (user_wire && entry->user_wired_count == 0)) {
			if (!user_wire)
				panic("vm_map_unwire: entry is unwired");

			entry = entry->vme_next;
			continue;
		}
		
		assert(entry->wired_count > 0 &&
			(!user_wire || entry->user_wired_count > 0));

		vm_map_clip_start(map, entry, start);
		vm_map_clip_end(map, entry, end);

		/*
		 * Check for holes
		 * Holes: Next entry should be contiguous unless
		 *	  this is the end of the region.
		 */
		if (((entry->vme_end < end) && 
		    ((entry->vme_next == vm_map_to_entry(map)) ||
		     (entry->vme_next->vme_start > entry->vme_end)))) {

			if (!user_wire)
				panic("vm_map_unwire: non-contiguous region");
			entry = entry->vme_next;
			continue;
		}

		if (!user_wire || (--entry->user_wired_count == 0))
			entry->wired_count--;

		if (entry->wired_count != 0) {
			entry = entry->vme_next;
			continue;
		}

		entry->in_transition = TRUE;
		tmp_entry = *entry;	/* see comment in vm_map_wire() */

		/*
		 * We can unlock the map now. The in_transition state
		 * guarantees existance of the entry.
		 */
		vm_map_unlock(map);
		vm_fault_unwire(map, &tmp_entry, FALSE);
		vm_map_lock(map);

		if (last_timestamp+1 != map->timestamp) {
			/*
			 * Find the entry again.  It could have been clipped
			 * or deleted after we unlocked the map.
			 */
			if (!vm_map_lookup_entry(map, tmp_entry.vme_start,
								&first_entry)) {
				if (!user_wire)
				       panic("vm_map_unwire: re-lookup failed");
				entry = first_entry->vme_next;
			} else
				entry = first_entry;
		}
		last_timestamp = map->timestamp;

		/*
		 * clear transition bit for all constituent entries that
		 * were in the original entry (saved in tmp_entry).  Also
		 * check for waiters.
		 */
		while ((entry != vm_map_to_entry(map)) &&
		       (entry->vme_start < tmp_entry.vme_end)) {
			assert(entry->in_transition);
			entry->in_transition = FALSE;
			if (entry->needs_wakeup) {
				entry->needs_wakeup = FALSE;
				need_wakeup = TRUE;
			}
			entry = entry->vme_next;
		}
	}
	vm_map_unlock(map);
	/*
	 * wake up anybody waiting on entries that we have unwired.
	 */
	if (need_wakeup)
		vm_map_entry_wakeup(map);
	return(KERN_SUCCESS);

}

/*
 *	vm_map_entry_delete:	[ internal use only ]
 *
 *	Deallocate the given entry from the target map.
 */		
void
vm_map_entry_delete(
	register vm_map_t	map,
	register vm_map_entry_t	entry)
{
	register vm_offset_t	s, e;
	register vm_object_t	object;
	extern vm_object_t	kernel_object;

	s = entry->vme_start;
	e = entry->vme_end;
	assert(page_aligned(s));
	assert(page_aligned(e));
	assert(entry->wired_count == 0);
	assert(entry->user_wired_count == 0);


	/*
	 *	Deallocate the object only after removing all
	 *	pmap entries pointing to its pages.
	 */
	if (entry->is_sub_map)
		vm_map_deallocate(entry->object.sub_map);
	else
	 	vm_object_deallocate(entry->object.vm_object);

	vm_map_entry_unlink(map, entry);
	map->size -= e - s;

	vm_map_entry_dispose(map, entry);
}


/*
 *	vm_map_delete:	[ internal use only ]
 *
 *	Deallocates the given address range from the target map.
 *	Removes all user wirings. Unwires one kernel wiring if
 *	VM_MAP_REMOVE_KUNWIRE is set.  Waits for kernel wirings to go
 *	away if VM_MAP_REMOVE_WAIT_FOR_KWIRE is set.  Sleeps
 *	interruptibly if VM_MAP_REMOVE_INTERRUPTIBLE is set.
 *
 *	This routine is called with map locked and leaves map locked.
 */
kern_return_t
vm_map_delete(
	register vm_map_t	map,
	vm_offset_t		start,
	register vm_offset_t	end,
	int			flags)
{
	register vm_map_entry_t	entry, next;
	struct	 vm_map_entry	*first_entry, tmp_entry;
	register vm_offset_t	s, e;
	register vm_object_t	object;
	boolean_t		need_wakeup;
	unsigned int		last_timestamp = ~0; /* unlikely value */
	boolean_t		interruptible;
	extern vm_map_t		kernel_map;

	interruptible = (flags & VM_MAP_REMOVE_INTERRUPTIBLE);

	/*
	 *	Find the start of the region, and clip it
	 */
	if (vm_map_lookup_entry(map, start, &first_entry)) {
		entry = first_entry;
		vm_map_clip_start(map, entry, start);

		/*
		 *	Fix the lookup hint now, rather than each
		 *	time through the loop.
		 */
		SAVE_HINT(map, entry->vme_prev);
	} else {
		entry = first_entry->vme_next;
	}

	need_wakeup = FALSE;
	/*
	 *	Step through all entries in this region
	 */
	while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {

		vm_map_clip_end(map, entry, end);
retry_entry:
		if (entry->in_transition) {
			/*
			 * Another thread is wiring/unwiring this entry.
			 * Let the other thread know we are waiting.
			 */
			s = entry->vme_start;
			entry->needs_wakeup = TRUE;

			/*
			 * wake up anybody waiting on entries that we have
			 * already unwired/deleted.
			 */
			if (need_wakeup) {
				vm_map_entry_wakeup(map);
				need_wakeup = FALSE;
			}

			vm_map_entry_wait(map, interruptible);

			if (interruptible &&
			   current_thread()->wait_result == THREAD_INTERRUPTED)
				/*
				 * We do not clear the needs_wakeup flag,
				 * since we cannot tell if we were the only one.
				 */
				return KERN_ABORTED;

			vm_map_lock(map);
			/*
			 * Cannot avoid a lookup here. reset timestamp.
			 */
			last_timestamp = map->timestamp;

			/*
			 * The entry could have been clipped or it
			 * may not exist anymore.  Look it up again.
			 */
			if (!vm_map_lookup_entry(map, s, &first_entry)) {
				assert((map != kernel_map) && 
				       (!entry->is_sub_map));
				/*
				 * User: use the next entry
				 */
				entry = first_entry->vme_next;
			} else {
				entry = first_entry;
				SAVE_HINT(map, entry->vme_prev);
			}
			goto retry_entry;
		} /* end in_transition */

		if (entry->wired_count) {
			/*
			 * 	Remove a kernel wiring if requested or if
			 *	there are user wirings.
			 */
			if ((flags & VM_MAP_REMOVE_KUNWIRE) || 
			   (entry->user_wired_count > 0))
				entry->wired_count--;

			/* remove all user wire references */
			entry->user_wired_count = 0;

			if (entry->wired_count != 0) {
				assert((map != kernel_map) && 
				       (!entry->is_sub_map));
				/*
				 * Cannot continue.  Typical case is when
				 * a user thread has physical io pending on
				 * on this page.  Either wait for the
				 * kernel wiring to go away or return an
				 * error.
				 */
				if (flags & VM_MAP_REMOVE_WAIT_FOR_KWIRE) {

					s = entry->vme_start;
					entry->needs_wakeup = TRUE;
					vm_map_entry_wait(map, interruptible);

					if (interruptible &&
			   		    current_thread()->wait_result == 
							THREAD_INTERRUPTED)
						/*
				 	 	 * We do not clear the 
						 * needs_wakeup flag, since we 
						 * cannot tell if we were the 
						 * only one.
				 	 	 */
						return KERN_ABORTED;

					vm_map_lock(map);
					/*
			 	 	 * Cannot avoid a lookup here. reset 
					 * timestamp.
			 	 	 */
					last_timestamp = map->timestamp;

					/*
			 		 * The entry could have been clipped or
					 * it may not exist anymore.  Look it
					 * up again.
			 		 */
					if (!vm_map_lookup_entry(map, s, 
								&first_entry)) {
						assert((map != kernel_map) && 
				       		(!entry->is_sub_map));
						/*
				 		 * User: use the next entry
				 		 */
						entry = first_entry->vme_next;
					} else {
						entry = first_entry;
						SAVE_HINT(map, entry->vme_prev);
					}
					goto retry_entry;
				}
				else {
					return KERN_FAILURE;
				}
			}

			entry->in_transition = TRUE;
			/*
			 * copy current entry.  see comment in vm_map_wire()
			 */
			tmp_entry = *entry;
			s = entry->vme_start;
			e = entry->vme_end;

			/*
			 * We can unlock the map now. The in_transition
			 * state guarentees existance of the entry.
			 */
			vm_map_unlock(map);
			vm_fault_unwire(map, &tmp_entry,
				tmp_entry.object.vm_object == kernel_object);
			vm_map_lock(map);

			if (last_timestamp+1 != map->timestamp) {
				/*
				 * Find the entry again.  It could have
				 * been clipped after we unlocked the map.
				 */
				if (!vm_map_lookup_entry(map, s, &first_entry)){
					assert((map != kernel_map) && 
				       	       (!entry->is_sub_map));
					first_entry = first_entry->vme_next;
				} else {
					SAVE_HINT(map, entry->vme_prev);
				}
			} else {
				SAVE_HINT(map, entry->vme_prev);
				first_entry = entry;
			}

			last_timestamp = map->timestamp;

			entry = first_entry;
			while ((entry != vm_map_to_entry(map)) &&
			       (entry->vme_start < tmp_entry.vme_end)) {
				assert(entry->in_transition);
				entry->in_transition = FALSE;
				if (entry->needs_wakeup) {
					entry->needs_wakeup = FALSE;
					need_wakeup = TRUE;
				}
				entry = entry->vme_next;
			}
			/*
			 * We have unwired the entry(s).  Go back and
			 * delete them.
			 */
			entry = first_entry;
			goto retry_entry;
		}

		/* entry is unwired */
		assert(entry->wired_count == 0);
		assert(entry->user_wired_count == 0);

		if (!entry->is_sub_map &&
		    entry->object.vm_object != kernel_object) {
			pmap_remove(map->pmap,
				    entry->vme_start, entry->vme_end);
		}

		next = entry->vme_next;
		vm_map_entry_delete(map, entry);
		entry = next;
	}

	if (map->wait_for_space)
		thread_wakeup((event_t) map);
	/*
	 * wake up anybody waiting on entries that we have already deleted.
	 */
	if (need_wakeup)
		vm_map_entry_wakeup(map);

	return KERN_SUCCESS;
}

/*
 *	vm_map_remove:
 *
 *	Remove the given address range from the target map.
 *	This is the exported form of vm_map_delete.
 */
kern_return_t
vm_map_remove(
	register vm_map_t	map,
	register vm_offset_t	start,
	register vm_offset_t	end,
	register boolean_t	flags)
{
	register kern_return_t	result;

	vm_map_lock(map);
	VM_MAP_RANGE_CHECK(map, start, end);
	result = vm_map_delete(map, start, end, flags);
	vm_map_unlock(map);

	return(result);
}


/*
 *	vm_map_copy_steal_pages:
 *
 *	Steal all the pages from a vm_map_copy page_list by copying ones
 *	that have not already been stolen.
 */
void
vm_map_copy_steal_pages(
	vm_map_copy_t	copy)
{
	register vm_page_t	m, new_m;
	register int		i;
	vm_object_t		object;

	assert(copy->type == VM_MAP_COPY_PAGE_LIST);
	for (i = 0; i < copy->cpy_npages; i++) {

		/*
		 *	If the page is not tabled, then it's already stolen.
		 */
		m = copy->cpy_page_list[i];
		if (!m->tabled)
			continue;

		/*
		 *	Page was not stolen,  get a new
		 *	one and do the copy now.
		 */
		while ((new_m = vm_page_grab()) == VM_PAGE_NULL) {
			VM_PAGE_WAIT();
		}

		vm_page_gobble(new_m); /* mark as consumed internally */
		vm_page_copy(m, new_m);

		object = m->object;
		vm_object_lock(object);
		vm_page_lock_queues();
		if (!m->active && !m->inactive)
			vm_page_activate(m);
		vm_page_unlock_queues();
		PAGE_WAKEUP_DONE(m);
		vm_object_paging_end(object);
		vm_object_unlock(object);

		copy->cpy_page_list[i] = new_m;
	}
	copy->cpy_page_loose = TRUE;
}

/*
 *	vm_map_copy_page_discard:
 *
 *	Get rid of the pages in a page_list copy.  If the pages are
 *	stolen, they are freed.  If the pages are not stolen, they
 *	are unbusied, and associated state is cleaned up.
 */
void
vm_map_copy_page_discard(
	vm_map_copy_t	copy)
{
	assert(copy->type == VM_MAP_COPY_PAGE_LIST);
	while (copy->cpy_npages > 0) {
		vm_page_t	m;

		if ((m = copy->cpy_page_list[--(copy->cpy_npages)]) !=
		    VM_PAGE_NULL) {

			/*
			 *	If it's not in the table, then it's
			 *	a stolen page that goes back
			 *	to the free list.  Else it belongs
			 *	to some object, and we hold a
			 *	paging reference on that object.
			 */
			if (!m->tabled) {
				VM_PAGE_FREE(m);
			}
			else {
				vm_object_t	object;

				object = m->object;

				vm_object_lock(object);
				vm_page_lock_queues();
				if (!m->active && !m->inactive)
					vm_page_activate(m);
				vm_page_unlock_queues();

				if ((!m->busy)) {
			    	    spl_t s = splvm();
				    kern_return_t kr;
				    kr = vm_page_unpin(m);
				    assert(kr == KERN_SUCCESS);
				    splx(s);	
				} else {
			    	    PAGE_WAKEUP_DONE(m);
				}
				vm_object_paging_end(object);
				vm_object_unlock(object);
			}
		}
	}
}

/*
 *	Routine:	vm_map_copy_discard
 *
 *	Description:
 *		Dispose of a map copy object (returned by
 *		vm_map_copyin).
 */
void
vm_map_copy_discard(
	vm_map_copy_t	copy)
{
	TR_DECL("vm_map_copy_discard");

/*	tr3("enter: copy 0x%x type %d", copy, copy->type);*/
free_next_copy:
	if (copy == VM_MAP_COPY_NULL)
		return;

	switch (copy->type) {
	case VM_MAP_COPY_ENTRY_LIST:
		while (vm_map_copy_first_entry(copy) !=
					vm_map_copy_to_entry(copy)) {
			vm_map_entry_t	entry = vm_map_copy_first_entry(copy);

			vm_map_copy_entry_unlink(copy, entry);
			vm_object_deallocate(entry->object.vm_object);
			vm_map_copy_entry_dispose(copy, entry);
		}
		break;
        case VM_MAP_COPY_OBJECT:
		vm_object_deallocate(copy->cpy_object);
		break;
	case VM_MAP_COPY_PAGE_LIST:

		/*
		 *	To clean this up, we have to unbusy all the pages
		 *	and release the paging references in their objects.
		 */
		if (copy->cpy_npages > 0)
			vm_map_copy_page_discard(copy);

		/*
		 *	If there's a continuation, abort it.  The
		 *	abort routine releases any storage.
		 */
		if (vm_map_copy_has_cont(copy)) {

			assert(vm_map_copy_cont_is_valid(copy));
			/*
			 *	Special case: recognize
			 *	vm_map_copy_discard_cont and optimize
			 *	here to avoid tail recursion.
			 */
			if (copy->cpy_cont == vm_map_copy_discard_cont) {
				register vm_map_copy_t	new_copy;

				new_copy = (vm_map_copy_t) copy->cpy_cont_args;
				zfree(vm_map_copy_zone, (vm_offset_t) copy);
				copy = new_copy;
				goto free_next_copy;
			} else {
				vm_map_copy_abort_cont(copy);
			}
		}

		break;

	case VM_MAP_COPY_KERNEL_BUFFER:

		/*
		 * The vm_map_copy_t and possibly the data buffer were
		 * allocated by a single call to kalloc(), i.e. the
		 * vm_map_copy_t was not allocated out of the zone.
		 */
		kfree((vm_offset_t) copy, copy->cpy_kalloc_size);
		return;
	}
	zfree(vm_map_copy_zone, (vm_offset_t) copy);
}

/*
 *	Routine:	vm_map_copy_copy
 *
 *	Description:
 *			Move the information in a map copy object to
 *			a new map copy object, leaving the old one
 *			empty.
 *
 *			This is used by kernel routines that need
 *			to look at out-of-line data (in copyin form)
 *			before deciding whether to return SUCCESS.
 *			If the routine returns FAILURE, the original
 *			copy object will be deallocated; therefore,
 *			these routines must make a copy of the copy
 *			object and leave the original empty so that
 *			deallocation will not fail.
 */
vm_map_copy_t
vm_map_copy_copy(
	vm_map_copy_t	copy)
{
	vm_map_copy_t	new_copy;

	if (copy == VM_MAP_COPY_NULL)
		return VM_MAP_COPY_NULL;

	/*
	 * Allocate a new copy object, and copy the information
	 * from the old one into it.
	 */

	new_copy = (vm_map_copy_t) zalloc(vm_map_copy_zone);
	*new_copy = *copy;

	if (copy->type == VM_MAP_COPY_ENTRY_LIST) {
		/*
		 * The links in the entry chain must be
		 * changed to point to the new copy object.
		 */
		vm_map_copy_first_entry(copy)->vme_prev
			= vm_map_copy_to_entry(new_copy);
		vm_map_copy_last_entry(copy)->vme_next
			= vm_map_copy_to_entry(new_copy);
	}

	/*
	 * Change the old copy object into one that contains
	 * nothing to be deallocated.
	 */
	copy->type = VM_MAP_COPY_OBJECT;
	copy->cpy_object = VM_OBJECT_NULL;

	/*
	 * Return the new object.
	 */
	return new_copy;
}

/*
 *	Routine:	vm_map_copy_discard_cont
 *
 *	Description:
 *		A version of vm_map_copy_discard that can be called
 *		as a continuation from a vm_map_copy page list.
 */
kern_return_t
vm_map_copy_discard_cont(
	vm_map_copyin_args_t	cont_args,
	vm_map_copy_t		*copy_result)	/* OUT */
{
	vm_map_copy_discard((vm_map_copy_t) cont_args);
	if (copy_result != (vm_map_copy_t *)0)
		*copy_result = VM_MAP_COPY_NULL;
	return(KERN_SUCCESS);
}

/*
 *	Routine:	vm_map_copy_overwrite
 *
 *	Description:
 *		Copy the memory described by the map copy
 *		object (copy; returned by vm_map_copyin) onto
 *		the specified destination region (dst_map, dst_addr).
 *		The destination must be writeable.
 *
 *		Unlike vm_map_copyout, this routine actually
 *		writes over previously-mapped memory.  If the
 *		previous mapping was to a permanent (user-supplied)
 *		memory object, it is preserved.
 *
 *		The attributes (protection and inheritance) of the
 *		destination region are preserved.
 *
 *		If successful, consumes the copy object.
 *		Otherwise, the caller is responsible for it.
 *
 *	Implementation notes:
 *		To overwrite aligned temporary virtual memory, it is
 *		sufficient to remove the previous mapping and insert
 *		the new copy.  This replacement is done either on
 *		the whole region (if no permanent virtual memory
 *		objects are embedded in the destination region) or
 *		in individual map entries.
 *
 *		To overwrite permanent virtual memory , it is necessary
 *		to copy each page, as the external memory management
 *		interface currently does not provide any optimizations.
 *
 *		Unaligned memory also has to be copied.  It is possible
 *		to use 'vm_trickery' to copy the aligned data.  This is
 *		not done but not hard to implement.
 *
 *		Once a page of permanent memory has been overwritten,
 *		it is impossible to interrupt this function; otherwise,
 *		the call would be neither atomic nor location-independent.
 *		The kernel-state portion of a user thread must be
 *		interruptible.
 *
 *		It may be expensive to forward all requests that might
 *		overwrite permanent memory (vm_write, vm_copy) to
 *		uninterruptible kernel threads.  This routine may be
 *		called by interruptible threads; however, success is
 *		not guaranteed -- if the request cannot be performed
 *		atomically and interruptibly, an error indication is
 *		returned.
 */

kern_return_t
vm_map_copy_overwrite(
	vm_map_t	dst_map,
	vm_offset_t	dst_addr,
	vm_map_copy_t	copy,
	boolean_t	interruptible)
{
	vm_offset_t	dst_end;
	vm_map_entry_t	tmp_entry;
	vm_map_entry_t	entry;
	kern_return_t	kr;
	boolean_t	aligned = TRUE;
	boolean_t	contains_permanent_objects = FALSE;

	interruptible = FALSE;	/* XXX */

	/*
	 *	Check for null copy object.
	 */

	if (copy == VM_MAP_COPY_NULL)
		return(KERN_SUCCESS);

	/*
	 *	Check for special kernel buffer allocated
	 *	by new_ipc_kmsg_copyin.
	 */

	if (copy->type == VM_MAP_COPY_KERNEL_BUFFER) {
		return(vm_map_copyout_kernel_buffer(dst_map, &dst_addr, 
						    copy, TRUE));
	}

	/*
	 *      Only works for entry lists at the moment.  Will
	 *	support page lists later.
	 */

	assert(copy->type == VM_MAP_COPY_ENTRY_LIST);

	if (copy->size == 0) {
		vm_map_copy_discard(copy);
		return(KERN_SUCCESS);
	}

	/*
	 *	Verify that the destination is all writeable
	 *	initially.  We have to trunc the destination
	 *	address and round the copy size or we'll end up
	 *	splitting entries in strange ways.
	 */

	if (!page_aligned(copy->size) ||
		!page_aligned (copy->offset) ||
		!page_aligned (dst_addr))
	{
		aligned = FALSE;
		dst_end = round_page(dst_addr + copy->size);
	} else {
		dst_end = dst_addr + copy->size;
	}

start_pass_1:
	vm_map_lock(dst_map);
	if (!vm_map_lookup_entry(dst_map, dst_addr, &tmp_entry)) {
		vm_map_unlock(dst_map);
		return(KERN_INVALID_ADDRESS);
	}
	if (aligned) {
		vm_map_clip_start(dst_map, tmp_entry, dst_addr);
	}
	for (entry = tmp_entry;;) {
		vm_map_entry_t	next = entry->vme_next;

		if ( ! (entry->protection & VM_PROT_WRITE)) {
			vm_map_unlock(dst_map);
			return(KERN_PROTECTION_FAILURE);
		}

		/*
		 *	If the entry is in transition, we must wait
		 *	for it to exit that state.  Anything could happen
		 *	when we unlock the map, so start over.
		 */
                if (entry->in_transition) {

                        /*
                         * Say that we are waiting, and wait for entry.
                         */
                        entry->needs_wakeup = TRUE;
                        vm_map_entry_wait(dst_map, FALSE);

			goto start_pass_1;
		}

/*
 *		our range is contained completely within this map entry
 */
		if (dst_end <= entry->vme_end)
			break;
/*
 *		check that range specified is contiguous region
 */
		if ((next == vm_map_to_entry(dst_map)) ||
		    (next->vme_start != entry->vme_end)) {
			vm_map_unlock(dst_map);
			return(KERN_INVALID_ADDRESS);
		}


		/*
		 *	Check for permanent objects in the destination.
		 */
		if ((entry->object.vm_object != VM_OBJECT_NULL) &&
			   !entry->object.vm_object->internal)
			contains_permanent_objects = TRUE;

		entry = next;
	}/* for */

	/*
	 *	If there are permanent objects in the destination, then
	 *	the copy cannot be interrupted.
	 */

	if (interruptible && contains_permanent_objects)
		return(KERN_FAILURE);	/* XXX */

	/*
 	 *
	 *	Make a second pass, overwriting the data
	 *	At the beginning of each loop iteration,
	 *	the next entry to be overwritten is "tmp_entry"
	 *	(initially, the value returned from the lookup above),
	 *	and the starting address expected in that entry
	 *	is "start".
	 */


	if (aligned) {
		if ((kr =  vm_map_copy_overwrite_aligned( dst_map, tmp_entry,
			copy, dst_addr)) != KERN_SUCCESS)
		{
			return kr;
		}
		vm_map_unlock(dst_map);
	} else {
		/*
		 * Performance gain:
		 *
		 * if the copy and dst address are misaligned but the same
		 * offset within the page we can copy_not_aligned the
		 * misaligned parts and copy aligned the rest.  If they are
		 * aligned but len is unaligned we simply need to copy
		 * the end bit unaligned.  We'll need to split the misaligned
		 * bits of the region in this case !
		 */
	/* ALWAYS UNLOCKS THE dst_map MAP */
		if ((kr =  vm_map_copy_overwrite_unaligned( dst_map,
			tmp_entry, copy, dst_addr)) != KERN_SUCCESS)
		{
			return kr;
		}
	}

	/*
	 *	Throw away the vm_map_copy object
	 */
	vm_map_copy_discard(copy);

	return(KERN_SUCCESS);
}/* vm_map_copy_overwrite */

/*
 *	Routine: vm_map_copy_overwrite_unaligned
 *
 *	Decription:
 *	Physically copy unaligned data
 *
 *	Implementation:
 *	Unaligned parts of pages have to be physically copied.  We use
 *	a modified form of vm_fault_copy (which understands none-aligned
 *	page offsets and sizes) to do the copy.  We attempt to copy as
 *	much memory in one go as possibly, however vm_fault_copy copies
 *	within 1 memory object so we have to find the smaller of "amount left"
 *	"source object data size" and "target object data size".  With
 *	unaligned data we don't need to split regions, therefore the source
 *	(copy) object should be one map entry, the target range may be split
 *	over multiple map entries however.  In any event we are pessimistic
 *	about these assumptions.
 *
 *	Assumptions:
 *	dst_map is locked on entry and is return locked on success,
 *	unlocked on error.
 */

kern_return_t
vm_map_copy_overwrite_unaligned(
	vm_map_t	dst_map,
	vm_map_entry_t	entry,
	vm_map_copy_t	copy,
	vm_offset_t	start)
{
	vm_map_entry_t		copy_entry = vm_map_copy_first_entry(copy);
	vm_map_version_t	version;
	vm_object_t		dst_object;
	vm_offset_t		dst_offset;
	vm_offset_t		src_offset;
	vm_offset_t		entry_offset;
	vm_offset_t		entry_end;
	vm_size_t		src_size,
				dst_size,
				copy_size,
				amount_left;
	kern_return_t		kr = KERN_SUCCESS;

	vm_map_lock_write_to_read(dst_map);

	src_offset = copy->offset - trunc_page(copy->offset);
	amount_left = copy->size;
/*
 *	unaligned so we never clipped this entry, we need the offset into
 *	the vm_object not just the data.
 */	
	while (amount_left > 0) {

		/* "start" must be within the current map entry */
		assert ((start>=entry->vme_start) && (start<entry->vme_end));

		dst_offset = start - entry->vme_start;

		dst_size = entry->vme_end - start;

		src_size = copy_entry->vme_end -
			(copy_entry->vme_start + src_offset);

		if (dst_size < src_size) {
/*
 *			we can only copy dst_size bytes before
 *			we have to get the next destination entry
 */
			copy_size = dst_size;
		} else {
/*
 *			we can only copy src_size bytes before
 *			we have to get the next source copy entry
 */
			copy_size = src_size;
		}

		if (copy_size > amount_left) {
			copy_size = amount_left;
		}
/*
 *		Entry needs copy, create a shadow shadow object for
 *		copy on write region.
 */
		if (entry->needs_copy &&
			 ((entry->protection & VM_PROT_WRITE) != 0))
		{
			if (vm_map_lock_read_to_write(dst_map)) {
				vm_map_lock_read(dst_map);
				goto RetryLookup;
			}
			vm_object_shadow(&entry->object.vm_object,
					&entry->offset,
					(vm_size_t)(entry->vme_end
						- entry->vme_start));
			entry->needs_copy = FALSE;
			vm_map_lock_write_to_read(dst_map);
		}
		dst_object = entry->object.vm_object;
/*
 *		unlike with the virtual (aligned) copy we're going
 *		to fault on it therefore we need a target object.
 */
                if (dst_object == VM_OBJECT_NULL) {
			if (vm_map_lock_read_to_write(dst_map)) {
				vm_map_lock_read(dst_map);
				goto RetryLookup;
			}
			dst_object = vm_object_allocate((vm_size_t)
					entry->vme_end - entry->vme_start);
			entry->object.vm_object = dst_object;
			entry->offset = 0;
			vm_map_lock_write_to_read(dst_map);
		}
/*
 *		Take an object reference and unlock map. The "entry" may
 *		disappear or change when the map is unlocked.
 */
		vm_object_reference(dst_object);
		version.main_timestamp = dst_map->timestamp;
		entry_offset = entry->offset;
		entry_end = entry->vme_end;
		vm_map_unlock_read(dst_map);
/*
 *		Copy as much as possible in one pass
 */
		kr = vm_fault_copy(
			copy_entry->object.vm_object,
			copy_entry->offset + src_offset,
			&copy_size,
			dst_object,
			entry_offset + dst_offset,
			dst_map,
			&version,
			FALSE /* XXX interruptible */ );

		start += copy_size;
		src_offset += copy_size;
		amount_left -= copy_size;
/*
 *		Release the object reference
 */
		vm_object_deallocate(dst_object);
/*
 *		If a hard error occurred, return it now
 */
		if (kr != KERN_SUCCESS)
			return kr;

		if ((copy_entry->vme_start + src_offset) == copy_entry->vme_end
			|| amount_left == 0)
		{
/*
 *			all done with this copy entry, dispose.
 */
			vm_map_copy_entry_unlink(copy, copy_entry);
			vm_object_deallocate(copy_entry->object.vm_object);
			vm_map_copy_entry_dispose(copy, copy_entry);

			if ((copy_entry = vm_map_copy_first_entry(copy))
				== vm_map_copy_to_entry(copy) && amount_left) {
/*
 *				not finished copying but run out of source
 */
				return KERN_INVALID_ADDRESS;
			}
			src_offset = 0;
		}

		if (amount_left == 0)
			return KERN_SUCCESS;

		vm_map_lock_read(dst_map);
		if (version.main_timestamp == dst_map->timestamp) {
			if (start == entry_end) {
/*
 *				destination region is split.  Use the version
 *				information to avoid a lookup in the normal
 *				case.
 */
				entry = entry->vme_next;
/*
 *				should be contiguous. Fail if we encounter
 *				a hole in the destination.
 */
				if (start != entry->vme_start) {
					vm_map_unlock_read(dst_map);
					return KERN_INVALID_ADDRESS ;
				}
			}
		} else {
/*
 *			Map version check failed.
 *			we must lookup the entry because somebody
 *			might have changed the map behind our backs.
 */
RetryLookup:
			if (!vm_map_lookup_entry(dst_map, start, &entry))
			{
				vm_map_unlock_read(dst_map);
				return KERN_INVALID_ADDRESS ;
			}
		}
	}/* while */

	/* NOTREACHED ?? */
	vm_map_unlock_read(dst_map);

	return KERN_SUCCESS;
}/* vm_map_copy_overwrite_unaligned */

/*
 *	Routine:	vm_map_copy_overwrite_aligned
 *
 *	Description:
 *	Does all the vm_trickery possible for whole pages.
 *
 *	Implementation:
 *
 *	If there are no permanent objects in the destination,
 *	and the source and destination map entry zones match,
 *	and the destination map entry is not shared,
 *	then the map entries can be deleted and replaced
 *	with those from the copy.  The following code is the
 *	basic idea of what to do, but there are lots of annoying
 *	little details about getting protection and inheritance
 *	right.  Should add protection, inheritance, and sharing checks
 *	to the above pass and make sure that no wiring is involved.
 */

kern_return_t
vm_map_copy_overwrite_aligned(
	vm_map_t	dst_map,
	vm_map_entry_t	tmp_entry,
	vm_map_copy_t	copy,
	vm_offset_t	start)
{
	vm_object_t	object;
	vm_map_entry_t	copy_entry;
	vm_size_t	copy_size;
	vm_size_t	size;
	vm_map_entry_t	entry;
		
	while ((copy_entry = vm_map_copy_first_entry(copy))
		!= vm_map_copy_to_entry(copy))
	{
		copy_size = (copy_entry->vme_end - copy_entry->vme_start);
		
		entry = tmp_entry;
		size = (entry->vme_end - entry->vme_start);
		/*
		 *	Make sure that no holes popped up in the
		 *	address map, and that the protection is
		 *	still valid, in case the map was unlocked
		 *	earlier.
		 */

		if (entry->vme_start != start) {
			vm_map_unlock(dst_map);
			return(KERN_INVALID_ADDRESS);
		}
		assert(entry != vm_map_to_entry(dst_map));

		/*
		 *	Check protection again
		 */

		if ( ! (entry->protection & VM_PROT_WRITE)) {
			vm_map_unlock(dst_map);
			return(KERN_PROTECTION_FAILURE);
		}

		/*
		 *	Adjust to source size first
		 */

		if (copy_size < size) {
			vm_map_clip_end(dst_map, entry, entry->vme_start + copy_size);
			size = copy_size;
		}

		/*
		 *	Adjust to destination size
		 */

		if (size < copy_size) {
			vm_map_copy_clip_end(copy, copy_entry,
				copy_entry->vme_start + size);
			copy_size = size;
		}

		assert((entry->vme_end - entry->vme_start) == size);
		assert((tmp_entry->vme_end - tmp_entry->vme_start) == size);
		assert((copy_entry->vme_end - copy_entry->vme_start) == size);

		/*
		 *	If the destination contains temporary unshared memory,
		 *	we can perform the copy by throwing it away and
		 *	installing the source data.
		 */

		object = entry->object.vm_object;
		if ((!entry->is_shared &&
		    ((object == VM_OBJECT_NULL) || object->internal)) ||
		    entry->needs_copy) {
			vm_object_t	old_object = entry->object.vm_object;
			vm_offset_t	old_offset = entry->offset;
			vm_offset_t	offset;

			/*
			 * Ensure that the source and destination aren't
			 * identical
			 */
			if (old_object == copy_entry->object.vm_object &&
			    old_offset == copy_entry->offset) {
				vm_map_copy_entry_unlink(copy, copy_entry);
				vm_map_copy_entry_dispose(copy, copy_entry);

				if (old_object != VM_OBJECT_NULL)
					vm_object_deallocate(old_object);

				start = tmp_entry->vme_end;
				tmp_entry = tmp_entry->vme_next;
				continue;
			}

			entry->object = copy_entry->object;
			object = entry->object.vm_object;
			offset = entry->offset = copy_entry->offset;
			entry->needs_copy = copy_entry->needs_copy;
			entry->wired_count = 0;
			entry->user_wired_count = 0;

			vm_map_copy_entry_unlink(copy, copy_entry);
			vm_map_copy_entry_dispose(copy, copy_entry);

			if (old_object != VM_OBJECT_NULL) {
				vm_object_pmap_protect(
					old_object,
					old_offset,
					size,
					dst_map->pmap,
					tmp_entry->vme_start,
					VM_PROT_NONE);

				vm_object_deallocate(old_object);
			}

			/*
			 * Try to aggressively enter physical mappings
			 * (but avoid uninstantiated objects)
			 */
			if (object != VM_OBJECT_NULL) {
			    vm_offset_t	va = entry->vme_start;

			    while (va < entry->vme_end) {
				register vm_page_t	m;
				vm_prot_t		prot;

				/*
				 * Look for the page in the top object
				 */
				prot = entry->protection;
				vm_object_lock(object);
				vm_object_paging_begin(object);

				if ((m = vm_page_lookup(object,offset)) !=
				    VM_PAGE_NULL && !m->busy && 
				    !m->fictitious &&
				    (!m->unusual || (!m->error &&
					!m->restart && !m->absent &&
					 (prot & m->page_lock) == 0))) {
					
					m->busy = TRUE;
					vm_object_unlock(object);
					
					/* 
					 * Honor COW obligations
					 */
					if (entry->needs_copy)
						prot &= ~VM_PROT_WRITE;

					PMAP_ENTER(dst_map->pmap, va, m,
						   prot, FALSE);
		
					vm_object_lock(object);
					vm_page_lock_queues();
					if (!m->active && !m->inactive)
						vm_page_activate(m);
					vm_page_unlock_queues();
					 PAGE_WAKEUP_DONE(m);
				}
				vm_object_paging_end(object);
				vm_object_unlock(object);

				offset += PAGE_SIZE;
				va += PAGE_SIZE;
			    } /* end while (va < entry->vme_end) */
			} /* end if (object) */

			/*
			 *	Set up for the next iteration.  The map
			 *	has not been unlocked, so the next
			 *	address should be at the end of this
			 *	entry, and the next map entry should be
			 *	the one following it.
			 */

			start = tmp_entry->vme_end;
			tmp_entry = tmp_entry->vme_next;
		} else {
			vm_map_version_t	version;
			vm_object_t		dst_object = entry->object.vm_object;
			vm_offset_t		dst_offset = entry->offset;
			kern_return_t		r;

			/*
			 *	Take an object reference, and record
			 *	the map version information so that the
			 *	map can be safely unlocked.
			 */

			vm_object_reference(dst_object);

			version.main_timestamp = dst_map->timestamp;

			vm_map_unlock(dst_map);

			/*
			 *	Copy as much as possible in one pass
			 */

			copy_size = size;
			r = vm_fault_copy(
					copy_entry->object.vm_object,
					copy_entry->offset,
					&copy_size,
					dst_object,
					dst_offset,
					dst_map,
					&version,
					FALSE /* XXX interruptible */ );

			/*
			 *	Release the object reference
			 */

			vm_object_deallocate(dst_object);

			/*
			 *	If a hard error occurred, return it now
			 */

			if (r != KERN_SUCCESS)
				return(r);

			if (copy_size != 0) {
				/*
				 *	Dispose of the copied region
				 */

				vm_map_copy_clip_end(copy, copy_entry,
					copy_entry->vme_start + copy_size);
				vm_map_copy_entry_unlink(copy, copy_entry);
				vm_object_deallocate(copy_entry->object.vm_object);
				vm_map_copy_entry_dispose(copy, copy_entry);
			}

			/*
			 *	Pick up in the destination map where we left off.
			 *
			 *	Use the version information to avoid a lookup
			 *	in the normal case.
			 */

			start += copy_size;
			vm_map_lock(dst_map);
			if ((version.main_timestamp + 1) == dst_map->timestamp) {
				/* We can safely use saved tmp_entry value */

				vm_map_clip_end(dst_map, tmp_entry, start);
				tmp_entry = tmp_entry->vme_next;
			} else {
				/* Must do lookup of tmp_entry */

				if (!vm_map_lookup_entry(dst_map, start, &tmp_entry)) {
					vm_map_unlock(dst_map);
					return(KERN_INVALID_ADDRESS);
				}
				vm_map_clip_start(dst_map, tmp_entry, start);
			}
		}
	}/* while */

	return(KERN_SUCCESS);
}/* vm_map_copy_overwrite_aligned */

#if	DIPC
#include <dipc/dipc_counters.h>
dcntr_decl(unsigned int c_dipc_overwrite_opt = 0;)
dcntr_decl(unsigned int c_dipc_overwrite_nonopt = 0;)
dcntr_decl(unsigned int c_dipc_overwrite_recv_done = 0;)
#else	/* DIPC */
#define	dstat_decl(foo)
#define	dstat(foo)
#define	dcntr_decl(foo)
#define	dcntr(foo)
#endif	/* DIPC */

#if	DIPC
vm_map_copy_t
vm_map_copy_overwrite_recv(
	vm_map_t	map,
	vm_offset_t	addr,
	vm_size_t	size)
{
	vm_offset_t	end_addr;
	vm_map_copy_t	copy = VM_MAP_COPY_NULL;
	vm_map_entry_t	entry, new_entry;
	kern_return_t	kr;

	if (size == 0)
		return VM_MAP_COPY_NULL;

	if (!page_aligned(size) || !page_aligned(addr)) {
		end_addr = round_page(addr + size);
	} else {
		end_addr = addr + size;
	}

	/* Do allocations before taking map lock. */

	/* build an ENTRY_LIST vm_map_copy_t */
	copy = (vm_map_copy_t) zalloc(vm_map_copy_zone);
	vm_map_copy_first_entry(copy) =
	 vm_map_copy_last_entry(copy) = vm_map_copy_to_entry(copy);
	copy->type = VM_MAP_COPY_ENTRY_LIST;
	copy->offset = addr;
	copy->size = size;
	copy->cpy_hdr.nentries = 0;
	copy->cpy_hdr.entries_pageable = TRUE;
	new_entry = vm_map_copy_entry_create(copy);

	vm_map_lock(map);
	if (!vm_map_lookup_entry(map, addr, &entry)) {
		vm_map_unlock(map);
		vm_map_copy_entry_dispose(copy, new_entry);
		vm_map_copy_discard(copy);
		return VM_MAP_COPY_NULL;
	}

	/*
	 * for now, only do this if the range is contained completely
	 * within a single entry.  Just give up if entry is in_transition:
	 * it could be another thread wiring/unwiring, but it could be another
	 * part of this message that set it, in which case we'd deadlock if
	 * we waited for it to clear.  If the entry is not writeable, give up
	 * and let the ipc_kmsg_copyout code handle the error later.
	 */
	if ((end_addr > entry->vme_end) ||
	    entry->in_transition ||
	    !(entry->protection & VM_PROT_WRITE)) {
		vm_map_unlock(map);
		dcntr(++c_dipc_overwrite_nonopt);
		vm_map_copy_entry_dispose(copy, new_entry);
		vm_map_copy_discard(copy);
		return VM_MAP_COPY_NULL;
	}

	vm_map_clip_start(map, entry, trunc_page(addr));

	/* handle COW obligations */
	if (entry->needs_copy) {
		vm_object_shadow(&entry->object.vm_object, &entry->offset,
			(vm_size_t)(entry->vme_end - entry->vme_start));
		entry->needs_copy = FALSE;
	}

	/* make sure there's a target object to put pages into */
	if (entry->object.vm_object == VM_OBJECT_NULL) {
		entry->object.vm_object = vm_object_allocate((vm_size_t)
					(entry->vme_end - entry->vme_start));
		entry->offset = 0;
	}

	/*
	 * set in_transition to keep the entry in the map from moving around
	 * while DIPC fills the object in with data.  This must be cleared
	 * later on by calling vm_map_copy_overwrite_recv_done (below).
	 */
	entry->in_transition = TRUE;

	/* make a copy of the entry */
	vm_map_entry_copy(new_entry, entry);
	vm_object_reference(new_entry->object.vm_object);

	vm_map_unlock(map);

	/* link in the new entry */
	vm_map_copy_entry_link(copy, vm_map_copy_last_entry(copy), new_entry);

	dcntr(++c_dipc_overwrite_opt);
	return copy;
}

/*
 * Find the map entry that corresponds to the copy entry, then clear
 * in_transition on it.  Discard the copy.
 */
kern_return_t
vm_map_copy_overwrite_recv_done(
	vm_map_t	map,
	vm_map_copy_t	copy)
{
	vm_map_entry_t	entry, map_entry;
	boolean_t	need_wakeup = FALSE;

	assert(copy != VM_MAP_COPY_NULL);
	assert(copy->type == VM_MAP_COPY_ENTRY_LIST);
	dcntr(++c_dipc_overwrite_recv_done);

	entry = vm_map_copy_first_entry(copy);
	assert(entry != VM_MAP_ENTRY_NULL);

	vm_map_lock(map);
	
	if (!vm_map_lookup_entry(map, entry->vme_start, &map_entry)) {
		vm_map_unlock(map);
		return KERN_INVALID_ADDRESS;
	}
	while ((map_entry != vm_map_to_entry(map)) &&
	    (map_entry->vme_end <= entry->vme_end)) {
		assert(map_entry->in_transition);
		map_entry->in_transition = FALSE;
		if (map_entry->needs_wakeup) {
			map_entry->needs_wakeup = FALSE;
			need_wakeup = TRUE;
		}
		map_entry = map_entry->vme_next;
	}
	vm_map_unlock(map);
	if (need_wakeup)
		vm_map_entry_wakeup(map);
	vm_map_copy_discard(copy);

	return KERN_SUCCESS;
}

#endif	/* DIPC */

/*
 *	Routine:	vm_map_copyout_kernel_buffer
 *
 *	Description:
 *		Copy out data from a kernel buffer into space in the
 *		destination map. The space may be otpionally dynamically
 *		allocated.
 *
 *		If successful, consumes the copy object.
 *		Otherwise, the caller is responsible for it.
 */
kern_return_t
vm_map_copyout_kernel_buffer(
	vm_map_t	map,
	vm_offset_t	*addr,	/* IN/OUT */
	vm_map_copy_t	copy,
	boolean_t	overwrite)
{
	kern_return_t kr = KERN_SUCCESS;
	thread_act_t thr_act = current_act();

	if (!overwrite) {

		/*
		 * Allocate space in the target map for the data
		 */
		*addr = 0;
		kr = vm_map_enter(map, 
				  addr, 
				  round_page(copy->size),
				  (vm_offset_t) 0, 
				  TRUE,
				  VM_OBJECT_NULL, 
				  (vm_offset_t) 0, 
				  FALSE,
				  VM_PROT_DEFAULT, 
				  VM_PROT_ALL,
				  VM_INHERIT_DEFAULT);
		if (kr != KERN_SUCCESS)
			return(kr);
	}

	/*
	 * Copyout the data from the kernel buffer to the target map.
	 */	
	if (thr_act->map == map) {
	
		/*
		 * If the target map is the current map, just do
		 * the copy.
		 */
		if (copyout((char *)copy->cpy_kdata, (char *)*addr,
				copy->size)) {
			kr = KERN_INVALID_ADDRESS;
		}
	}
	else {
		vm_map_t oldmap;

		/*
		 * If the target map is another map, assume the
		 * target's address space identity for the duration
		 * of the copy.
		 */
		vm_map_reference(map);
		oldmap = vm_map_switch(map);

		if (copyout((char *)copy->cpy_kdata, (char *)*addr,
				copy->size)) {
			kr = KERN_INVALID_ADDRESS;
		}
	
		(void) vm_map_switch(oldmap);
		vm_map_deallocate(map);
	}

	kfree((vm_offset_t)copy, copy->cpy_kalloc_size);

	return(kr);
}
		
/*
 *	Macro:		vm_map_copy_insert
 *	
 *	Description:
 *		Link a copy chain ("copy") into a map at the
 *		specified location (after "where").
 *	Side effects:
 *		The copy chain is destroyed.
 *	Warning:
 *		The arguments are evaluated multiple times.
 */
#define	vm_map_copy_insert(map, where, copy)				\
MACRO_BEGIN								\
	vm_map_t VMCI_map;						\
	vm_map_entry_t VMCI_where;					\
	vm_map_copy_t VMCI_copy;					\
	VMCI_map = (map);						\
	VMCI_where = (where);						\
	VMCI_copy = (copy);						\
	((VMCI_where->vme_next)->vme_prev = vm_map_copy_last_entry(VMCI_copy))\
		->vme_next = (VMCI_where->vme_next);			\
	((VMCI_where)->vme_next = vm_map_copy_first_entry(VMCI_copy))	\
		->vme_prev = VMCI_where;				\
	VMCI_map->hdr.nentries += VMCI_copy->cpy_hdr.nentries;		\
	UPDATE_FIRST_FREE(VMCI_map, VMCI_map->first_free);		\
	zfree(vm_map_copy_zone, (vm_offset_t) VMCI_copy);		\
MACRO_END

/*
 *	Routine:	vm_map_copyout
 *
 *	Description:
 *		Copy out a copy chain ("copy") into newly-allocated
 *		space in the destination map.
 *
 *		If successful, consumes the copy object.
 *		Otherwise, the caller is responsible for it.
 */
kern_return_t
vm_map_copyout(
	register vm_map_t	dst_map,
	vm_offset_t		*dst_addr,	/* OUT */
	register vm_map_copy_t	copy)
{
	vm_size_t	size;
	vm_size_t	adjustment;
	vm_offset_t	start;
	vm_offset_t	vm_copy_start;
	vm_map_entry_t	last;
	register
	vm_map_entry_t	entry;

	/*
	 *	Check for null copy object.
	 */

	if (copy == VM_MAP_COPY_NULL) {
		*dst_addr = 0;
		return(KERN_SUCCESS);
	}

	/*
	 *	Check for special copy object, created
	 *	by vm_map_copyin_object.
	 */

	if (copy->type == VM_MAP_COPY_OBJECT) {
		vm_object_t object = copy->cpy_object;
		kern_return_t kr;
		vm_size_t offset;

		offset = trunc_page(copy->offset);
		size = round_page(copy->size + copy->offset - offset);
		*dst_addr = 0;
		kr = vm_map_enter(dst_map, dst_addr, size,
				  (vm_offset_t) 0, TRUE,
				  object, offset, FALSE,
				  VM_PROT_DEFAULT, VM_PROT_ALL,
				  VM_INHERIT_DEFAULT);
		if (kr != KERN_SUCCESS)
			return(kr);
		/* Account for non-pagealigned copy object */
		*dst_addr += copy->offset - offset;
		zfree(vm_map_copy_zone, (vm_offset_t) copy);
		return(KERN_SUCCESS);
	}

	/*
	 *	Check for special kernel buffer allocated
	 *	by new_ipc_kmsg_copyin.
	 */

	if (copy->type == VM_MAP_COPY_KERNEL_BUFFER) {
		return(vm_map_copyout_kernel_buffer(dst_map, dst_addr, 
						    copy, FALSE));
	}

	if (copy->type == VM_MAP_COPY_PAGE_LIST)
		return(vm_map_copyout_page_list(dst_map, dst_addr, copy));

	/*
	 *	Find space for the data
	 */

	vm_copy_start = trunc_page(copy->offset);
	size =	round_page(copy->offset + copy->size) - vm_copy_start;

 StartAgain: ;

	vm_map_lock(dst_map);
	assert(first_free_is_valid(dst_map));
	start = ((last = dst_map->first_free) == vm_map_to_entry(dst_map)) ?
		vm_map_min(dst_map) : last->vme_end;

	while (TRUE) {
		vm_map_entry_t	next = last->vme_next;
		vm_offset_t	end = start + size;

		if ((end > dst_map->max_offset) || (end < start)) {
			if (dst_map->wait_for_space) {
				if (size <= (dst_map->max_offset - dst_map->min_offset)) {
					assert_wait((event_t) dst_map, TRUE);
					vm_map_unlock(dst_map);
					thread_block((void (*)(void))0);
					goto StartAgain;
				}
			}
			vm_map_unlock(dst_map);
			return(KERN_NO_SPACE);
		}

		if ((next == vm_map_to_entry(dst_map)) ||
		    (next->vme_start >= end))
			break;

		last = next;
		start = last->vme_end;
	}

	/*
	 *	Since we're going to just drop the map
	 *	entries from the copy into the destination
	 *	map, they must come from the same pool.
	 */

	if (copy->cpy_hdr.entries_pageable != dst_map->hdr.entries_pageable) {
	    /*
	     * Mismatches occur when dealing with the default
	     * pager.
	     */
	    zone_t		old_zone;
	    vm_map_entry_t	next, new;

	    /*
	     * Find the zone that the copies were allocated from
	     */
	    old_zone = (copy->cpy_hdr.entries_pageable)
			? vm_map_entry_zone
			: vm_map_kentry_zone;
	    entry = vm_map_copy_first_entry(copy);

	    /*
	     * Reinitialize the copy so that vm_map_copy_entry_link
	     * will work.
	     */
	    copy->cpy_hdr.nentries = 0;
	    copy->cpy_hdr.entries_pageable = dst_map->hdr.entries_pageable;
	    vm_map_copy_first_entry(copy) =
	     vm_map_copy_last_entry(copy) =
		vm_map_copy_to_entry(copy);

	    /*
	     * Copy each entry.
	     */
	    while (entry != vm_map_copy_to_entry(copy)) {
		new = vm_map_copy_entry_create(copy);
		vm_map_entry_copy_full(new, entry);
		vm_map_copy_entry_link(copy,
				vm_map_copy_last_entry(copy),
				new);
		next = entry->vme_next;
		zfree(old_zone, (vm_offset_t) entry);
		entry = next;
	    }
	}

	/*
	 *	Adjust the addresses in the copy chain, and
	 *	reset the region attributes.
	 */

	adjustment = start - vm_copy_start;
	for (entry = vm_map_copy_first_entry(copy);
	     entry != vm_map_copy_to_entry(copy);
	     entry = entry->vme_next) {
		entry->vme_start += adjustment;
		entry->vme_end += adjustment;

		entry->inheritance = VM_INHERIT_DEFAULT;
		entry->protection = VM_PROT_DEFAULT;
		entry->max_protection = VM_PROT_ALL;
		entry->behavior = VM_BEHAVIOR_DEFAULT;

		/*
		 * If the entry is now wired,
		 * map the pages into the destination map.
		 */
		if (entry->wired_count != 0) {
		    register vm_offset_t va;
		    vm_offset_t		 offset;
		    register vm_object_t object;

		    object = entry->object.vm_object;
		    offset = entry->offset;
		    va = entry->vme_start;

		    pmap_pageable(dst_map->pmap,
				  entry->vme_start,
				  entry->vme_end,
				  TRUE);

		    while (va < entry->vme_end) {
			register vm_page_t	m;

			/*
			 * Look up the page in the object.
			 * Assert that the page will be found in the
			 * top object:
			 * either
			 *	the object was newly created by
			 *	vm_object_copy_slowly, and has
			 *	copies of all of the pages from
			 *	the source object
			 * or
			 *	the object was moved from the old
			 *	map entry; because the old map
			 *	entry was wired, all of the pages
			 *	were in the top-level object.
			 *	(XXX not true if we wire pages for
			 *	 reading)
			 */
			vm_object_lock(object);
			vm_object_paging_begin(object);

			m = vm_page_lookup(object, offset);
			if (m == VM_PAGE_NULL || m->wire_count == 0 ||
			    m->absent)
			    panic("vm_map_copyout: wiring 0x%x", m);

			m->busy = TRUE;
			vm_object_unlock(object);

			PMAP_ENTER(dst_map->pmap, va, m,
				   entry->protection, TRUE);

			vm_object_lock(object);
			PAGE_WAKEUP_DONE(m);
			/* the page is wired, so we don't have to activate */
			vm_object_paging_end(object);
			vm_object_unlock(object);

			offset += PAGE_SIZE;
			va += PAGE_SIZE;
		    }
		}
		else if (size <= vm_map_aggressive_enter_max) {

			register vm_offset_t	va;
			vm_offset_t		offset;
			register vm_object_t	object;
			vm_prot_t		prot;

			object = entry->object.vm_object;
			if (object != VM_OBJECT_NULL) {

				offset = entry->offset;
				va = entry->vme_start;
				while (va < entry->vme_end) {
					register vm_page_t	m;
				    
					/*
					 * Look up the page in the object.
					 * Assert that the page will be found
					 * in the top object if at all...
					 */
					vm_object_lock(object);
					vm_object_paging_begin(object);

					if (((m = vm_page_lookup(object,
								 offset))
					     != VM_PAGE_NULL) &&
					    !m->busy && !m->fictitious &&
					    !m->absent && !m->error) {
						m->busy = TRUE;
						vm_object_unlock(object);

						/* honor cow obligations */
						prot = entry->protection;
						if (entry->needs_copy)
							prot &= ~VM_PROT_WRITE;

						PMAP_ENTER(dst_map->pmap, va, 
							   m, prot, FALSE);

						vm_object_lock(object);
						vm_page_lock_queues();
						if (!m->active && !m->inactive)
							vm_page_activate(m);
						vm_page_unlock_queues();
						PAGE_WAKEUP_DONE(m);
					}
					vm_object_paging_end(object);
					vm_object_unlock(object);

					offset += PAGE_SIZE;
					va += PAGE_SIZE;
				}
			}
		}
	}

	/*
	 *	Correct the page alignment for the result
	 */

	*dst_addr = start + (copy->offset - vm_copy_start);

	/*
	 *	Update the hints and the map size
	 */

	SAVE_HINT(dst_map, vm_map_copy_last_entry(copy));

	dst_map->size += size;

	/*
	 *	Link in the copy
	 */

	vm_map_copy_insert(dst_map, last, copy);

	vm_map_unlock(dst_map);

	/*
	 * XXX	If wiring_required, call vm_map_pageable
	 */

	return(KERN_SUCCESS);
}

boolean_t       vm_map_aggressive_enter;        /* not used yet */

/*
 *
 *	vm_map_copyout_page_list:
 *
 *	Version of vm_map_copyout() for page list vm map copies.
 *
 */
kern_return_t
vm_map_copyout_page_list(
	register vm_map_t	dst_map,
	vm_offset_t		*dst_addr,	/* OUT */
	register vm_map_copy_t	copy)
{
	vm_size_t	size;
	vm_offset_t	start;
	vm_offset_t	end;
	vm_offset_t	offset;
	vm_map_entry_t	last;
	register
	vm_object_t	object;
	vm_page_t	*page_list, m;
	vm_map_entry_t	entry;
	vm_offset_t	old_last_offset;
	boolean_t	cont_invoked, needs_wakeup;
	kern_return_t	result = KERN_SUCCESS;
	vm_map_copy_t	orig_copy;
	vm_offset_t	dst_offset;
	boolean_t	must_wire;
	boolean_t	aggressive_enter;

	/*
	 *	Check for null copy object.
	 */

	if (copy == VM_MAP_COPY_NULL) {
		*dst_addr = 0;
		return(KERN_SUCCESS);
	}

	assert(copy->type == VM_MAP_COPY_PAGE_LIST);

	/*
	 *	Make sure the pages are stolen, because we are
	 *	going to put them in a new object.  Assume that
	 *	all pages are identical to first in this regard.
	 */

	page_list = &copy->cpy_page_list[0];
	if (!copy->cpy_page_loose)
		vm_map_copy_steal_pages(copy);

	/*
	 *	Find space for the data
	 */

	size =	round_page(copy->offset + copy->size) -
		trunc_page(copy->offset);
StartAgain:
	vm_map_lock(dst_map);
	must_wire = dst_map->wiring_required;

	assert(first_free_is_valid(dst_map));
	last = dst_map->first_free;
	if (last == vm_map_to_entry(dst_map)) {
		start = vm_map_min(dst_map);
	} else {
		start = last->vme_end;
	}

	while (TRUE) {
		vm_map_entry_t next = last->vme_next;
		end = start + size;

		if ((end > dst_map->max_offset) || (end < start)) {
			if (dst_map->wait_for_space) {
				if (size <= (dst_map->max_offset -
					     dst_map->min_offset)) {
					assert_wait((event_t) dst_map, TRUE);
					vm_map_unlock(dst_map);
					thread_block((void (*)(void))0);
					goto StartAgain;
				}
			}
			vm_map_unlock(dst_map);
			return(KERN_NO_SPACE);
		}

		if ((next == vm_map_to_entry(dst_map)) ||
		    (next->vme_start >= end)) {
			break;
		}

		last = next;
		start = last->vme_end;
	}

	/*
	 *	See whether we can avoid creating a new entry (and object) by
	 *	extending one of our neighbors.  [So far, we only attempt to
	 *	extend from below.]
	 *
	 *	The code path below here is a bit twisted.  If any of the
	 *	extension checks fails, we branch to create_object.  If
	 *	it all works, we fall out the bottom and goto insert_pages.
	 */
	if (last == vm_map_to_entry(dst_map) ||
	    last->vme_end != start ||
	    last->is_shared != FALSE ||
	    last->is_sub_map != FALSE ||
	    last->inheritance != VM_INHERIT_DEFAULT ||
	    last->protection != VM_PROT_DEFAULT ||
	    last->max_protection != VM_PROT_ALL ||
	    last->behavior != VM_BEHAVIOR_DEFAULT ||
	    last->in_transition ||
	    (must_wire ? (last->wired_count != 1 ||
		    last->user_wired_count != 0) :
		(last->wired_count != 0))) {
		    goto create_object;
	}
	
	/*
	 * If this entry needs an object, make one.
	 */
	if (last->object.vm_object == VM_OBJECT_NULL) {
		object = vm_object_allocate(
			(vm_size_t)(last->vme_end - last->vme_start + size));
		last->object.vm_object = object;
		last->offset = 0;
	}
	else {
	    vm_offset_t	prev_offset = last->offset;
	    vm_size_t	prev_size = start - last->vme_start;
	    vm_size_t	new_size;

	    /*
	     *	This is basically vm_object_coalesce.
	     */

	    object = last->object.vm_object;
	    vm_object_lock(object);

	    /*
	     *	Try to collapse the object first
	     */
	    vm_object_collapse(object);

	    /*
	     *	Can't coalesce if pages not mapped to
	     *	last may be in use anyway:
	     *	. more than one reference
	     *	. paged out
	     *	. shadows another object
	     *	. has a copy elsewhere
	     *	. paging references (pages might be in page-list)
	     */

	    if ((object->ref_count > 1) ||
		object->pager_created ||
		(object->shadow != VM_OBJECT_NULL) ||
#if	!NORMA_VM
		(object->copy != VM_OBJECT_NULL) ||
#endif	/* !NORMA_VM */
		(object->paging_in_progress != 0)) {
		    vm_object_unlock(object);
		    goto create_object;
	    }

	    /*
	     *	Extend the object if necessary.  Don't have to call
	     *  vm_object_page_remove because the pages aren't mapped,
	     *	and vm_page_replace will free up any old ones it encounters.
	     */
	    new_size = prev_offset + prev_size + size;
	    if (new_size > object->size) {
#if	MACH_PAGEMAP
		    /*
		     *	We cannot extend an object that has existence info,
		     *	since the existence info might then fail to cover
		     *	the entire object.
		     *
		     *	This assertion must be true because the object
		     *	has no pager, and we only create existence info
		     *	for objects with pagers.
		     */
		    assert(object->existence_map == VM_EXTERNAL_NULL);
#endif	/* MACH_PAGEMAP */
		    object->size = new_size;
	    }
	    vm_object_unlock(object);
        }

	/*
	 *	Coalesced the two objects - can extend
	 *	the previous map entry to include the
	 *	new range.
	 */
	dst_map->size += size;
	last->vme_end = end;
	UPDATE_FIRST_FREE(dst_map, dst_map->first_free);

	SAVE_HINT(dst_map, last);

	goto insert_pages;

create_object:

	/*
	 *	Create object
	 */
	object = vm_object_allocate(size);

	/*
	 *	Create entry
	 */
	last = vm_map_entry_insert(dst_map, last, start, start + size,
				   object, 0, FALSE, FALSE, TRUE,
				   VM_PROT_DEFAULT, VM_PROT_ALL,
				   VM_BEHAVIOR_DEFAULT,
				   VM_INHERIT_DEFAULT, (must_wire ? 1 : 0));

	/*
	 *	Transfer pages into new object.  
	 *	Scan page list in vm_map_copy.
	 */
insert_pages:
	dst_offset = copy->offset & PAGE_MASK;
	cont_invoked = FALSE;
	orig_copy = copy;
	last->in_transition = TRUE;
	old_last_offset = last->offset
	    + (start - last->vme_start);

	aggressive_enter = (size <= vm_map_aggressive_enter_max);

	for (offset = 0; offset < size; offset += PAGE_SIZE) {
		m = *page_list;
		assert(m && !m->tabled);

		/*
		 *	Must clear busy bit in page before inserting it.
		 *	Ok to skip wakeup logic because nobody else
		 *	can possibly know about this page.  Also set
		 *	dirty bit on the assumption that the page is
		 *	not a page of zeros.
		 */

		m->busy = FALSE;
		m->dirty = TRUE;
		vm_object_lock(object);
		vm_page_lock_queues();
		vm_page_replace(m, object, old_last_offset + offset);
		if (must_wire) {
			vm_page_wire(m);
		} else if (aggressive_enter) {
			vm_page_activate(m);
		}
		vm_page_unlock_queues();
		vm_object_unlock(object);

		if (aggressive_enter || must_wire) {
			PMAP_ENTER(dst_map->pmap,
 				   last->vme_start + m->offset - last->offset,
 				   m, last->protection, must_wire);
		}

		*page_list++ = VM_PAGE_NULL;
		assert(copy != VM_MAP_COPY_NULL);
		assert(copy->type == VM_MAP_COPY_PAGE_LIST);
		if (--(copy->cpy_npages) == 0 &&
		    vm_map_copy_has_cont(copy)) {
			vm_map_copy_t	new_copy;

			/*
			 *	Ok to unlock map because entry is
			 *	marked in_transition.
			 */
			cont_invoked = TRUE;
			vm_map_unlock(dst_map);
			vm_map_copy_invoke_cont(copy, &new_copy, &result);

			if (result == KERN_SUCCESS) {

				/*
				 *	If we got back a copy with real pages,
				 *	steal them now.  Either all of the
				 *	pages in the list are tabled or none
				 *	of them are; mixtures are not possible.
				 *
				 *	Save original copy for consume on
				 *	success logic at end of routine.
				 */
				if (copy != orig_copy)
					vm_map_copy_discard(copy);

				if ((copy = new_copy) != VM_MAP_COPY_NULL) {
					page_list = &copy->cpy_page_list[0];
					if (!copy->cpy_page_loose)
				    		vm_map_copy_steal_pages(copy);
				}
			}
			else {
				/*
				 *	Continuation failed.
				 */
				vm_map_lock(dst_map);
				goto error;
			}

			vm_map_lock(dst_map);
		}
	}

	*dst_addr = start + dst_offset;
	
	/*
	 *	Clear the in transition bits.  This is easy if we
	 *	didn't have a continuation.
	 */
error:
	needs_wakeup = FALSE;
	if (!cont_invoked) {
		/*
		 *	We didn't unlock the map, so nobody could
		 *	be waiting.
		 */
		last->in_transition = FALSE;
		assert(!last->needs_wakeup);
	}
	else {
		if (!vm_map_lookup_entry(dst_map, start, &entry))
			panic("vm_map_copyout_page_list: missing entry");

                /*
                 * Clear transition bit for all constituent entries that
                 * were in the original entry.  Also check for waiters.
                 */
                while ((entry != vm_map_to_entry(dst_map)) &&
                       (entry->vme_start < end)) {
                        assert(entry->in_transition);
                        entry->in_transition = FALSE;
                        if (entry->needs_wakeup) {
                                entry->needs_wakeup = FALSE;
                                needs_wakeup = TRUE;
                        }
                        entry = entry->vme_next;
                }
	}
	
	if (result != KERN_SUCCESS)
		(void) vm_map_delete(dst_map, start, end, VM_MAP_NO_FLAGS);

	vm_map_unlock(dst_map);

	if (needs_wakeup)
		vm_map_entry_wakeup(dst_map);

	/*
	 *	Consume on success logic.
	 */
	if (copy != VM_MAP_COPY_NULL && copy != orig_copy) {
		zfree(vm_map_copy_zone, (vm_offset_t) copy);
	}
	if (result == KERN_SUCCESS) {
		assert(orig_copy != VM_MAP_COPY_NULL);
		assert(orig_copy->type == VM_MAP_COPY_PAGE_LIST);
		zfree(vm_map_copy_zone, (vm_offset_t) orig_copy);
	}
	
	return(result);
}

/*
 *	Routine:	vm_map_copyin
 *
 *	Description:
 *		Copy the specified region (src_addr, len) from the
 *		source address space (src_map), possibly removing
 *		the region from the source address space (src_destroy).
 *
 *	Returns:
 *		A vm_map_copy_t object (copy_result), suitable for
 *		insertion into another address space (using vm_map_copyout),
 *		copying over another address space region (using
 *		vm_map_copy_overwrite).  If the copy is unused, it
 *		should be destroyed (using vm_map_copy_discard).
 *
 *	In/out conditions:
 *		The source map should not be locked on entry.
 */
#if	DIPC
dstat_decl(unsigned int c_vmcc_volatile = 0;)
dstat_decl(unsigned int c_vmcc_null_entry = 0;)
dstat_decl(unsigned int c_vmcc_null_entry_continue = 0;)
dstat_decl(unsigned int c_vmcc_src_destroy_opt = 0;)
dstat_decl(unsigned int c_vmcc_wasnt_wired = 0;)
dstat_decl(unsigned int c_vmcc_vocq = 0;)
dstat_decl(unsigned int c_vmcc_vopp = 0;)
dstat_decl(unsigned int c_vmcc_voept = 0;)
dstat_decl(unsigned int c_vmcc_vocs = 0;)
dstat_decl(unsigned int c_vmcc_vocstrat = 0;)
dstat_decl(unsigned int c_vmcc_vmle = 0;)
dstat_decl(unsigned int c_vmcc_reloop = 0;)
dstat_decl(unsigned int c_vmcc_vmd = 0;)
#endif	/* DIPC */

kern_return_t
vm_map_copyin_common(
	vm_map_t	src_map,
	vm_offset_t	src_addr,
	vm_size_t	len,
	boolean_t	src_destroy,
	boolean_t	src_volatile,
	vm_map_copy_t	*copy_result,	/* OUT */
	boolean_t	use_maxprot)
{
	vm_map_entry_t	tmp_entry;	/* Result of last map lookup --
					 * in multi-level lookup, this
					 * entry contains the actual
					 * vm_object/offset.
					 */
	register
	vm_map_entry_t	new_entry = VM_MAP_ENTRY_NULL;	/* Map entry for copy */

	vm_offset_t	src_start;	/* Start of current entry --
					 * where copy is taking place now
					 */
	vm_offset_t	src_end;	/* End of entire region to be
					 * copied */

	register
	vm_map_copy_t	copy;		/* Resulting copy */

	/*
	 *	Check for copies of zero bytes.
	 */

	if (len == 0) {
		*copy_result = VM_MAP_COPY_NULL;
		return(KERN_SUCCESS);
	}

	/*
	 *	Compute start and end of region
	 */

	src_start = trunc_page(src_addr);
	src_end = round_page(src_addr + len);

#if	DIPC
	XPR(XPR_VM_MAP,
	    "vm_map_copyin_common map 0x%x addr 0x%x len 0x%x dest %d volatile %d\n",
	    (natural_t)src_map, src_addr, len, src_destroy, src_volatile);
#else	/* DIPC */
	XPR(XPR_VM_MAP, "vm_map_copyin_common map 0x%x addr 0x%x len 0x%x dest %d\n",
	    (natural_t)src_map, src_addr, len, src_destroy, 0);
#endif	/* DIPC */

	/*
	 *	Check that the end address doesn't overflow
	 */

	if (src_end <= src_start)
		if ((src_end < src_start) || (src_start != 0))
			return(KERN_INVALID_ADDRESS);

	/*
	 *	Allocate a header element for the list.
	 *
	 *	Use the start and end in the header to 
	 *	remember the endpoints prior to rounding.
	 */

	copy = (vm_map_copy_t) zalloc(vm_map_copy_zone);
	vm_map_copy_first_entry(copy) =
	 vm_map_copy_last_entry(copy) = vm_map_copy_to_entry(copy);
	copy->type = VM_MAP_COPY_ENTRY_LIST;
	copy->cpy_hdr.nentries = 0;
	copy->cpy_hdr.entries_pageable = TRUE;

	copy->offset = src_addr;
	copy->size = len;
	
	new_entry = vm_map_copy_entry_create(copy);

#define	RETURN(x)						\
	MACRO_BEGIN						\
	vm_map_unlock(src_map);					\
	if (new_entry != VM_MAP_ENTRY_NULL)			\
		vm_map_copy_entry_dispose(copy,new_entry);	\
	vm_map_copy_discard(copy);				\
	MACRO_RETURN(x);					\
	MACRO_END

	/*
	 *	Find the beginning of the region.
	 */

 	vm_map_lock(src_map);

	if (!vm_map_lookup_entry(src_map, src_start, &tmp_entry))
		RETURN(KERN_INVALID_ADDRESS);
	vm_map_clip_start(src_map, tmp_entry, src_start);

	/*
	 *	Go through entries until we get to the end.
	 */

	while (TRUE) {
		register
		vm_map_entry_t	src_entry = tmp_entry;	/* Top-level entry */
		vm_size_t	src_size;		/* Size of source
							 * map entry (in both
							 * maps)
							 */

		register
		vm_object_t	src_object;		/* Object to copy */
		vm_offset_t	src_offset;

		boolean_t	src_needs_copy;		/* Should source map
							 * be made read-only
							 * for copy-on-write?
							 */

		boolean_t	new_entry_needs_copy;	/* Will new entry be COW? */

		boolean_t	was_wired;		/* Was source wired? */
		vm_map_version_t version;		/* Version before locks
							 * dropped to make copy
							 */
		kern_return_t	result;			/* Return value from
							 * copy_strategically.
							 */
		/*
		 *	Create a new address map entry to hold the result. 
		 *	Fill in the fields from the appropriate source entries.
		 *	We must unlock the source map to do this if we need
		 *	to allocate a map entry.
		 */
		if (new_entry == VM_MAP_ENTRY_NULL) {
		    version.main_timestamp = src_map->timestamp;
		    vm_map_unlock(src_map);

		    dstat(++c_vmcc_null_entry);
		    new_entry = vm_map_copy_entry_create(copy);

		    vm_map_lock(src_map);
		    if ((version.main_timestamp + 1) != src_map->timestamp) {
			if (!vm_map_lookup_entry(src_map, src_start,
					&tmp_entry)) {
				RETURN(KERN_INVALID_ADDRESS);
			}
			vm_map_clip_start(src_map, tmp_entry, src_start);
			dstat(++c_vmcc_null_entry_continue);
			continue; /* restart w/ new tmp_entry */
		    }
		}

		/*
		 *	Verify that the region can be read.
		 */
		if (((src_entry->protection & VM_PROT_READ) == VM_PROT_NONE &&
			!use_maxprot) ||
		    (src_entry->max_protection & VM_PROT_READ) == 0)
			RETURN(KERN_PROTECTION_FAILURE);

		/*
		 *	Clip against the endpoints of the entire region.
		 */

		vm_map_clip_end(src_map, src_entry, src_end);

		src_size = src_entry->vme_end - src_start;
		src_object = src_entry->object.vm_object;
		src_offset = src_entry->offset;
		was_wired = (src_entry->wired_count != 0);

		vm_map_entry_copy(new_entry, src_entry);

		/*
		 *	Attempt non-blocking copy-on-write optimizations.
		 */

		if (src_destroy &&
		    (src_object == VM_OBJECT_NULL || src_object->internal)) {
		    /*
		     * If we are destroying the source, and the object
		     * is internal, we can move the object reference
		     * from the source to the copy.  The copy is
		     * copy-on-write only if the source is.
		     * We make another reference to the object, because
		     * destroying the source entry will deallocate it.
		     */
		    vm_object_reference(src_object);

		    /*
		     * Copy is always unwired.  vm_map_copy_entry
		     * set its wired count to zero.
		     */

		    dstat(++c_vmcc_src_destroy_opt);
		    goto CopySuccessful;
		}

#if	DIPC
		/*
		 *	If the caller promises not to modify the data,
		 *	we don't have to apply copy-on-write processing
		 *	to it.  This works best in the distributed case.
		 *	In the local case, we can wind up with fully
		 *	shared data between sender and receiver -- a
		 *	behavior that we aren't entirely sure we want
		 *	at this point.  So for now, we'll only do this
		 *	on remote data transfers.
		 */
		if (src_volatile == TRUE) {
		    XPR(XPR_VM_MAP,
			"vmcc src_obj 0x%x ent 0x%x obj 0x%x VOLATILE\n",
			src_object, new_entry, new_entry->object.vm_object,
			0, 0);
		    assert(src_destroy == FALSE);
		    dstat(++c_vmcc_volatile);
		    vm_object_reference(src_object);
		    goto CopySuccessful;
		}
#endif	/* DIPC */

RestartCopy:
		XPR(XPR_VM_MAP, "vm_map_copyin_common src_obj 0x%x ent 0x%x obj 0x%x was_wired %d\n",
		    src_object, new_entry, new_entry->object.vm_object,
		    was_wired, 0);
		dstat(!was_wired ? ++c_vmcc_wasnt_wired : 0);
		if (!was_wired &&
		    vm_object_copy_quickly(
				&new_entry->object.vm_object,
				src_offset,
				src_size,
				&src_needs_copy,
				&new_entry_needs_copy)) {

			dstat(++c_vmcc_vocq);
			new_entry->needs_copy = new_entry_needs_copy;

			/*
			 *	Handle copy-on-write obligations
			 */

			if (src_needs_copy && !tmp_entry->needs_copy) {
				dstat(++c_vmcc_vopp);
				vm_object_pmap_protect(
					src_object,
					src_offset,
					src_size,
			      		(src_entry->is_shared ? PMAP_NULL
						: src_map->pmap),
					src_entry->vme_start,
					src_entry->protection &
						~VM_PROT_WRITE);

				tmp_entry->needs_copy = TRUE;
			}
#if	NORMA_VM
			if (src_needs_copy && src_object->internal) {
				dstat(++c_vmcc_voept);
				vm_object_export_pagemap_try(src_object,
							     src_size);
			}
#endif	/* NORMA_VM */

			/*
			 *	The map has never been unlocked, so it's safe
			 *	to move to the next entry rather than doing
			 *	another lookup.
			 */

			goto CopySuccessful;
		}

		new_entry->needs_copy = FALSE;

		/*
		 *	Take an object reference, so that we may
		 *	release the map lock(s).
		 */

		assert(src_object != VM_OBJECT_NULL);
		vm_object_reference(src_object);

		/*
		 *	Record the timestamp for later verification.
		 *	Unlock the map.
		 */

		version.main_timestamp = src_map->timestamp;
		vm_map_unlock(src_map);

		/*
		 *	Perform the copy
		 */

		if (was_wired) {
			dstat(++c_vmcc_vocs);
			vm_object_lock(src_object);
			result = vm_object_copy_slowly(
					src_object,
					src_offset,
					src_size,
					FALSE,
					&new_entry->object.vm_object);
			new_entry->offset = 0;
			new_entry->needs_copy = FALSE;
		} else {
			dstat(++c_vmcc_vocstrat);
			result = vm_object_copy_strategically(src_object,
				src_offset,
				src_size,
				&new_entry->object.vm_object,
				&new_entry->offset,
				&new_entry_needs_copy);

			new_entry->needs_copy = new_entry_needs_copy;
			
		}

		if (result != KERN_SUCCESS &&
		    result != KERN_MEMORY_RESTART_COPY) {
			vm_map_lock(src_map);
			RETURN(result);
		}

		/*
		 *	Throw away the extra reference
		 */

		vm_object_deallocate(src_object);

		/*
		 *	Verify that the map has not substantially
		 *	changed while the copy was being made.
		 */

		vm_map_lock(src_map);	/* Increments timestamp once! */

		if ((version.main_timestamp + 1) == src_map->timestamp)
			goto VerificationSuccessful;

		/*
		 *	Simple version comparison failed.
		 *
		 *	Retry the lookup and verify that the
		 *	same object/offset are still present.
		 *
		 *	[Note: a memory manager that colludes with
		 *	the calling task can detect that we have
		 *	cheated.  While the map was unlocked, the
		 *	mapping could have been changed and restored.]
		 */

		dstat(++c_vmcc_vmle);
		if (!vm_map_lookup_entry(src_map, src_start, &tmp_entry)) {
			RETURN(KERN_INVALID_ADDRESS);
		}

		src_entry = tmp_entry;
		vm_map_clip_start(src_map, src_entry, src_start);

		if ((src_entry->protection & VM_PROT_READ == VM_PROT_NONE &&
			!use_maxprot) ||
		    src_entry->max_protection & VM_PROT_READ == 0)
			goto VerificationFailed;

		if (src_entry->vme_end < new_entry->vme_end)
			src_size = (new_entry->vme_end = src_entry->vme_end) - src_start;

		if ((src_entry->object.vm_object != src_object) ||
		    (src_entry->offset != src_offset) ) {

			/*
			 *	Verification failed.
			 *
			 *	Start over with this top-level entry.
			 */

		 VerificationFailed: ;

			vm_object_deallocate(new_entry->object.vm_object);
			tmp_entry = src_entry;
			continue;
		}

		/*
		 *	Verification succeeded.
		 */

	 VerificationSuccessful: ;

		if (result == KERN_MEMORY_RESTART_COPY)
			goto RestartCopy;

		/*
		 *	Copy succeeded.
		 */

	 CopySuccessful: ;

		/*
		 *	Link in the new copy entry.
		 */

		vm_map_copy_entry_link(copy, vm_map_copy_last_entry(copy),
				       new_entry);
		
		/*
		 *	Determine whether the entire region
		 *	has been copied.
		 */
		src_start = new_entry->vme_end;
		new_entry = VM_MAP_ENTRY_NULL;
		if ((src_start >= src_end) && (src_end != 0))
			break;

		dstat(++c_vmcc_reloop);
		/*
		 *	Verify that there are no gaps in the region
		 */

		tmp_entry = src_entry->vme_next;
		if (tmp_entry->vme_start != src_start)
			RETURN(KERN_INVALID_ADDRESS);
	}

	/*
	 * If the source should be destroyed, do it now, since the
	 * copy was successful. 
	 */
	if (src_destroy) {
		dstat(++c_vmcc_vmd);
		(void) vm_map_delete(src_map,
				     trunc_page(src_addr),
				     src_end,
				     (src_map == kernel_map) ?
					VM_MAP_REMOVE_KUNWIRE :
					VM_MAP_NO_FLAGS);
	}

	vm_map_unlock(src_map);

	*copy_result = copy;
	return(KERN_SUCCESS);

#undef	RETURN
}

/*
 *	vm_map_copyin_object:
 *
 *	Create a copy object from an object.
 *	Our caller donates an object reference.
 */

kern_return_t
vm_map_copyin_object(
	vm_object_t	object,
	vm_offset_t	offset,		/* offset of region in object */
	vm_size_t	size,		/* size of region in object */
	vm_map_copy_t	*copy_result)	/* OUT */
{
	vm_map_copy_t	copy;		/* Resulting copy */

	/*
	 *	We drop the object into a special copy object
	 *	that contains the object directly.
	 */

	copy = (vm_map_copy_t) zalloc(vm_map_copy_zone);
	copy->type = VM_MAP_COPY_OBJECT;
	copy->cpy_object = object;
	copy->cpy_index = 0;
	copy->offset = offset;
	copy->size = size;

	*copy_result = copy;
	return(KERN_SUCCESS);
}

/*
 *	vm_map_copyin_page_list_cont:
 *
 *	Continuation routine for vm_map_copyin_page_list.
 *	
 *	If vm_map_copyin_page_list can't fit the entire vm range
 *	into a single page list object, it creates a continuation.
 *	When the target of the operation has used the pages in the
 *	initial page list, it invokes the continuation, which calls
 *	this routine.  If an error happens, the continuation is aborted
 *	(abort arg to this routine is TRUE).  To avoid deadlocks, the
 *	pages are discarded from the initial page list before invoking
 *	the continuation.
 *
 *	NOTE: This is not the same sort of continuation used by
 *	the scheduler.
 */

kern_return_t
vm_map_copyin_page_list_cont(
	vm_map_copyin_args_t	cont_args,
	vm_map_copy_t		*copy_result)	/* OUT */
{
	kern_return_t	result = KERN_SUCCESS;
	register boolean_t	abort, src_destroy, src_destroy_only;

	/*
	 *	Check for cases that only require memory destruction.
	 */
	abort = (copy_result == (vm_map_copy_t *) 0);
	src_destroy = (cont_args->destroy_len != (vm_size_t) 0);
	src_destroy_only = (cont_args->src_len == (vm_size_t) 0);

	if (abort || src_destroy_only) {
		if (src_destroy)
			result = vm_map_remove(cont_args->map,
			    cont_args->destroy_addr,
			    cont_args->destroy_addr + cont_args->destroy_len,
			    VM_MAP_NO_FLAGS);
		if (!abort)
			*copy_result = VM_MAP_COPY_NULL;
	}
	else {
		result = vm_map_copyin_page_list(cont_args->map,
			cont_args->src_addr, cont_args->src_len,
			cont_args->options, copy_result, TRUE);

		if (src_destroy &&
		    (cont_args->options & VM_MAP_COPYIN_OPT_STEAL_PAGES) &&
		    vm_map_copy_has_cont(*copy_result)) {
			    vm_map_copyin_args_t	new_args;
		    	    /*
			     *	Transfer old destroy info.
			     */
			    new_args = (vm_map_copyin_args_t)
			    		(*copy_result)->cpy_cont_args;
		            new_args->destroy_addr = cont_args->destroy_addr;
		            new_args->destroy_len = cont_args->destroy_len;
		}
	}
	
	vm_map_deallocate(cont_args->map);
	kfree((vm_offset_t)cont_args, sizeof(vm_map_copyin_args_data_t));

	return(result);
}

/*
 *	vm_map_copyin_page_list:
 *
 *	This is a variant of vm_map_copyin that copies in a list of pages.
 *	If steal_pages is TRUE, the pages are only in the returned list.
 *	If steal_pages is FALSE, the pages are busy and still in their
 *	objects.  A continuation may be returned if not all the pages fit:
 *	the recipient of this copy_result must be prepared to deal with it.
 */

kern_return_t
vm_map_copyin_page_list(
    vm_map_t		src_map,
    vm_offset_t		src_addr,
    vm_size_t		len,
    int			options,
    vm_map_copy_t	*copy_result,	/* OUT */
    boolean_t		is_cont)
{
    vm_map_entry_t		src_entry;
    vm_page_t 			m;
    vm_offset_t			src_start;
    vm_offset_t			src_end;
    vm_size_t			src_size;
    register vm_object_t	src_object;
    register vm_offset_t	src_offset;
    vm_offset_t			src_last_offset;
    register vm_map_copy_t	copy;		/* Resulting copy */
    kern_return_t		result = KERN_SUCCESS;
    boolean_t			need_map_lookup;
    vm_map_copyin_args_t	cont_args;
    kern_return_t		error_code;
    vm_prot_t			prot;
    boolean_t			wired;
    boolean_t			no_zero_fill;

    prot = (options & VM_MAP_COPYIN_OPT_VM_PROT);
    no_zero_fill = (options & VM_MAP_COPYIN_OPT_NO_ZERO_FILL);
    
    /*
     * 	If steal_pages is FALSE, this leaves busy pages in
     *	the object.  A continuation must be used if src_destroy
     *	is true in this case (!steal_pages && src_destroy).
     *
     * XXX	Still have a more general problem of what happens
     * XXX	if the same page occurs twice in a list.  Deadlock
     * XXX	can happen if vm_fault_page was called.  A
     * XXX	possible solution is to use a continuation if vm_fault_page
     * XXX	is called and we cross a map entry boundary.
     */

    /*
     *	Check for copies of zero bytes.
     */

    if (len == 0) {
        *copy_result = VM_MAP_COPY_NULL;
	return(KERN_SUCCESS);
    }

    /*
     *	Compute start and end of region
     */

    src_start = trunc_page(src_addr);
    src_end = round_page(src_addr + len);

    /*
     * If the region is not page aligned, override the no_zero_fill
     * argument.
     */

    if (options & VM_MAP_COPYIN_OPT_NO_ZERO_FILL) {
        if (!page_aligned(src_addr) || !page_aligned(src_addr +len))
	    options &= ~VM_MAP_COPYIN_OPT_NO_ZERO_FILL;
    }

    /*
     *	Check that the end address doesn't overflow
     */

    if (src_end <= src_start && (src_end < src_start || src_start != 0)) {
        return KERN_INVALID_ADDRESS;
    }

    /*
     *	Allocate a header element for the page list.
     *
     *	Record original offset and size, as caller may not
     *      be page-aligned.
     */

    copy = (vm_map_copy_t) zalloc(vm_map_copy_zone);
    copy->type = VM_MAP_COPY_PAGE_LIST;
    copy->cpy_npages = 0;
    copy->cpy_page_loose = FALSE;
    copy->offset = src_addr;
    copy->size = len;
    copy->cpy_cont = VM_MAP_COPY_CONT_NULL;
    copy->cpy_cont_args = VM_MAP_COPYIN_ARGS_NULL;
	
    /*
     *	Find the beginning of the region.
     */

do_map_lookup:

    vm_map_lock(src_map);

    if (!vm_map_lookup_entry(src_map, src_start, &src_entry)) {
        result = KERN_INVALID_ADDRESS;
	goto error;
    }
    need_map_lookup = FALSE;

    /*
     *	Go through entries until we get to the end.
     */

    while (TRUE) {
        if ((src_entry->protection & prot) != prot) {
	    result = KERN_PROTECTION_FAILURE;
	    goto error;
	}
	wired = (src_entry->wired_count != 0);

	if (src_end > src_entry->vme_end)
	    src_size = src_entry->vme_end - src_start;
	else
	    src_size = src_end - src_start;

	src_object = src_entry->object.vm_object;

	/*
	 *	If src_object is NULL, allocate it now;
	 *	we're going to fault on it shortly.
	 */
	if (src_object == VM_OBJECT_NULL) {
	    src_object = vm_object_allocate((vm_size_t)
					    src_entry->vme_end -
					    src_entry->vme_start);
	    src_entry->object.vm_object = src_object;
	}
	else if (src_entry->needs_copy && (prot & VM_PROT_WRITE)) {
	    vm_object_shadow(
			     &src_entry->object.vm_object,
			     &src_entry->offset,
			     (vm_size_t) (src_entry->vme_end -
					  src_entry->vme_start));

	    src_entry->needs_copy = FALSE;

	    /* reset src_object */
	    src_object = src_entry->object.vm_object;
	}

	/*
	 * calculate src_offset now, since vm_object_shadow
	 * may have changed src_entry->offset.
	 */
	src_offset = src_entry->offset + (src_start - src_entry->vme_start);

	/*
	 * Iterate over pages.  Fault in ones that aren't present.
	 */
	src_last_offset = src_offset + src_size;
	for (; (src_offset < src_last_offset && !need_map_lookup);
	     src_offset += PAGE_SIZE, src_start += PAGE_SIZE) {

	    if (copy->cpy_npages == VM_MAP_COPY_PAGE_LIST_MAX) {
make_continuation:
	        /*
		 * At this point we have the max number of
		 * pages busy for this thread that we're
		 * willing to allow.  Stop here and record
		 * arguments for the remainder.  Note:
		 * this means that this routine isn't atomic,
		 * but that's the breaks.  Note that only
		 * the first vm_map_copy_t that comes back
		 * from this routine has the right offset
		 * and size; those from continuations are
		 * page rounded, and short by the amount
		 * already done.
		 *
		 * Reset src_end so the src_destroy
		 * code at the bottom doesn't do
		 * something stupid.
		 */

	        cont_args = (vm_map_copyin_args_t) 
		  	    kalloc(sizeof(vm_map_copyin_args_data_t));
		cont_args->map = src_map;
		vm_map_reference(src_map);
		cont_args->src_addr = src_start;
		cont_args->src_len = len - (src_start - src_addr);
		if (options & VM_MAP_COPYIN_OPT_SRC_DESTROY) {
		    cont_args->destroy_addr = cont_args->src_addr;
		    cont_args->destroy_len = cont_args->src_len;
		} else {
		    cont_args->destroy_addr = (vm_offset_t) 0;
		    cont_args->destroy_len = (vm_offset_t) 0;
		}
		cont_args->options = options;
		
		copy->cpy_cont_args = cont_args;
		copy->cpy_cont = vm_map_copyin_page_list_cont;
		
		src_end = src_start;
		break;
	    }

	    /*
	     *	Try to find the page of data.  Have to
	     *	fault it in if there's no page, or something
	     *	going on with the page, or the object has
	     *	a copy object.
	     */
	    vm_object_lock(src_object);
	    vm_object_paging_begin(src_object);
	    if (((m = vm_page_lookup(src_object, src_offset)) !=
		 VM_PAGE_NULL) && 
		!m->busy && !m->fictitious && !m->unusual &&
		((prot & VM_PROT_WRITE) == 0 ||
		 (m->object->copy == VM_OBJECT_NULL))) {
	      
		if (!m->absent &&
		    !(options & VM_MAP_COPYIN_OPT_STEAL_PAGES)) {

		  	/* 
			 * The page is present and will not be
			 * replaced, prep it. Thus allowing
			 * mutiple access on this page 
			 */
			kern_return_t kr;
			spl_t	      s;

			kr = vm_page_prep(m);
			assert(kr == KERN_SUCCESS);
			s = splvm();
			kr = vm_page_pin(m);
			assert(kr == KERN_SUCCESS);
			splx(s);
		} else {
	    		/*
			 *	This is the page.  Mark it busy
			 *	and keep the paging reference on
			 *	the object whilst we do our thing.
			 */
	
		      	m->busy = TRUE;
		}
	    } else {
	        vm_prot_t 	result_prot;
		vm_page_t 	top_page;
		kern_return_t 	kr;
		boolean_t 	data_supply;
				
		/*
		 *	Have to fault the page in; must
		 *	unlock the map to do so.  While
		 *	the map is unlocked, anything
		 *	can happen, we must lookup the
		 *	map entry before continuing.
		 */
		vm_map_unlock(src_map);
		need_map_lookup = TRUE;
		data_supply = src_object->silent_overwrite &&
		  (prot & VM_PROT_WRITE) &&
		    src_start >= src_addr &&
		      src_start + PAGE_SIZE <=
			src_addr + len;

retry:
		result_prot = prot;
				
		XPR(XPR_VM_FAULT,
		    "vm_map_copyin_page_list -> vm_fault_page\n",
		    0,0,0,0,0);
		kr = vm_fault_page(src_object, src_offset,
				   prot, FALSE, FALSE,
				   src_entry->offset,
				   src_entry->offset +
				   (src_entry->vme_end -
				    src_entry->vme_start),
				   VM_BEHAVIOR_SEQUENTIAL,
				   &result_prot, &m, &top_page,
				   &error_code,
				   options & VM_MAP_COPYIN_OPT_NO_ZERO_FILL,
				   data_supply);
		/*
		 *	Cope with what happened.
		 */
		switch (kr) {
		case VM_FAULT_SUCCESS:

		    /*
		     *	If we lost write access,
		     *	try again.
		     */
		    if ((prot & VM_PROT_WRITE) &&
			!(result_prot & VM_PROT_WRITE)) {
		        vm_object_lock(src_object);
			vm_object_paging_begin(src_object);
			goto retry;
		    }
		    break;
		case VM_FAULT_INTERRUPTED: /* ??? */
		case VM_FAULT_RETRY:
		    vm_object_lock(src_object);
		    vm_object_paging_begin(src_object);
		    goto retry;
		case VM_FAULT_MEMORY_SHORTAGE:
		    VM_PAGE_WAIT();
		    vm_object_lock(src_object);
		    vm_object_paging_begin(src_object);
		    goto retry;
		case VM_FAULT_FICTITIOUS_SHORTAGE:
		    vm_page_more_fictitious();
		    vm_object_lock(src_object);
		    vm_object_paging_begin(src_object);
		    goto retry;
		case VM_FAULT_MEMORY_ERROR:
		    /*
		     * Something broke.  If this
		     * is a continuation, return
		     * a partial result if possible,
		     * else fail the whole thing.
		     * In the continuation case, the
		     * next continuation call will
		     * get this error if it persists.
		     */
		    vm_map_lock(src_map);
		    if (is_cont &&
			copy->cpy_npages != 0)
		        goto make_continuation;

		    result = error_code ? error_code : KERN_MEMORY_ERROR;
		    goto error;
		}
				
		if (top_page != VM_PAGE_NULL) {
		    vm_object_lock(src_object);
		    VM_PAGE_FREE(top_page);
		    vm_object_paging_end(src_object);
		    vm_object_unlock(src_object);
		}

	    }

	    /*
	     * The page is busy, its object is locked, and
	     * we have a paging reference on it.  Either
	     * the map is locked, or need_map_lookup is
	     * TRUE.
	     */

	    /*
	     * Put the page in the page list.
	     */
	    copy->cpy_page_list[copy->cpy_npages++] = m;
	    vm_object_unlock(m->object);

	    /*
	     * Pmap enter support.  Only used for
	     * device I/O for colocated server.
	     *
	     * WARNING:  This code assumes that this
	     * option is only used for well behaved
	     * memory.  If the mapping has changed,
	     * the following code will make mistakes.
	     *
	     * XXXO probably ought to do pmap_extract first,
	     * XXXO to avoid needless pmap_enter, but this
	     * XXXO can't detect protection mismatch??
	     */

	    if (options & VM_MAP_COPYIN_OPT_PMAP_ENTER) {
	        /*
		 * XXX  Only used on kernel map.
		 * XXX	Must not remove VM_PROT_WRITE on
		 * XXX	an I/O only requiring VM_PROT_READ
		 * XXX  as another I/O may be active on same page
		 * XXX  assume that if mapping exists, it must
		 * XXX	have the equivalent of at least VM_PROT_READ,
		 * XXX  but don't assume it has VM_PROT_WRITE as the
		 * XXX  pmap might not all the rights of the object
		 */
	        assert(vm_map_pmap(src_map) == kernel_pmap);
	      
		if ((prot & VM_PROT_WRITE) ||
		    (pmap_extract(vm_map_pmap(src_map),
				  src_start) != m->phys_addr))

		    PMAP_ENTER(vm_map_pmap(src_map), src_start,
			       m, prot, wired);
	    }
	}
			
	/*
	 *	DETERMINE whether the entire region
	 *	has been copied.
	 */

	if (src_start >= src_end && src_end != 0) {
	    if (need_map_lookup)
	        vm_map_lock(src_map);
	    break;
	}

	/*
	 * If need_map_lookup is TRUE, have to start over with
	 * another map lookup.  Note that we dropped the map
	 * lock (to call vm_fault_page) above only in this case.
	 */

	if (need_map_lookup)
	    goto do_map_lookup;

	/*
	 *	Verify that there are no gaps in the region
	 */

	src_start = src_entry->vme_end;
	src_entry = src_entry->vme_next;
	if (src_entry->vme_start != src_start) {
	    result = KERN_INVALID_ADDRESS;
	    goto error;
	}
    }

    /*
     * If steal_pages is true, make sure all
     * pages in the copy are not in any object
     * We try to remove them from the original
     * object, but we may have to copy them.
     *
     * At this point every page in the list is busy
     * and holds a paging reference to its object.
     * When we're done stealing, every page is busy,
     * and in no object (m->tabled == FALSE).
     */
    src_start = trunc_page(src_addr);
    if (options & VM_MAP_COPYIN_OPT_STEAL_PAGES) {
        register int 	i;
	vm_offset_t	unwire_end;

	unwire_end = src_start;
	for (i = 0; i < copy->cpy_npages; i++) {
	  
	    /*
	     * Remove the page from its object if it
	     * can be stolen.  It can be stolen if:
	     *
	     * (1) The source is being destroyed, 
	     *       the object is internal (hence
	     *       temporary), and not shared.
	     * (2) The page is not precious.
	     *
	     * The not shared check consists of two
	     * parts:  (a) there are no objects that
	     * shadow this object.  (b) it is not the
	     * object in any shared map entries (i.e.,
	     * use_shared_copy is not set).
	     *
	     * The first check (a) means that we can't
	     * steal pages from objects that are not
	     * at the top of their shadow chains.  This
	     * should not be a frequent occurrence.
	     *
	     * Stealing wired pages requires telling the
	     * pmap module to let go of them.
	     * 
	     * NOTE: stealing clean pages from objects
	     *  	whose mappings survive requires a call to
	     * the pmap module.  Maybe later.
	     */
	    m = copy->cpy_page_list[i];
	    src_object = m->object;
	    vm_object_lock(src_object);

	    if ((options & VM_MAP_COPYIN_OPT_SRC_DESTROY) &&
		src_object->internal &&
		(!src_object->shadowed) &&
		(src_object->copy_strategy ==
		 MEMORY_OBJECT_COPY_SYMMETRIC) &&
		!m->precious) {
	        vm_offset_t	page_vaddr;
		
		page_vaddr = src_start + (i * PAGE_SIZE);
		if (m->wire_count > 0) {

		    assert(m->wire_count == 1);
		    /*
		     * In order to steal a wired
		     * page, we have to unwire it
		     * first.  We do this inline
		     * here because we have the page.
		     *
		     * Step 1: Unwire the map entry.
		     *	Also tell the pmap module
		     * 	that this piece of the
		     * 	pmap is pageable.
		     */
		    vm_object_unlock(src_object);
		    if (page_vaddr >= unwire_end) {
		        if (!vm_map_lookup_entry(src_map,
						 page_vaddr, &src_entry))
			    panic("vm_map_copyin_page_list: missing wired map entry");

			vm_map_clip_start(src_map, src_entry,
					  page_vaddr);
			vm_map_clip_end(src_map, src_entry,
					src_start + src_size);

			assert(src_entry->wired_count > 0);
			src_entry->wired_count = 0;
			src_entry->user_wired_count = 0;
			unwire_end = src_entry->vme_end;
			pmap_pageable(vm_map_pmap(src_map),
				      page_vaddr, unwire_end, TRUE);
		    }

		    /*
		     * Step 2: Unwire the page.
		     * pmap_remove handles this for us.
		     */
		    vm_object_lock(src_object);
		}

		/*
		 * Don't need to remove the mapping;
		 * vm_map_delete will handle it.
		 * 
		 * Steal the page.  Setting the wire count
		 * to zero is vm_page_unwire without
		 * activating the page.
		 */
		vm_page_lock_queues();
		vm_page_remove(m);
		if (m->wire_count > 0) {
		    m->wire_count = 0;
		    vm_page_wire_count--;
		} else {
		    VM_PAGE_QUEUES_REMOVE(m);
		}
		vm_page_unlock_queues();
	    } else {
	        /*
		 * Have to copy this page.  Have to
		 * unlock the map while copying,
		 * hence no further page stealing.
		 * Hence just copy all the pages.
		 * Unlock the map while copying;
		 * This means no further page stealing.
		 */
	        vm_object_unlock(src_object);
		vm_map_unlock(src_map);
		vm_map_copy_steal_pages(copy);
		vm_map_lock(src_map);
		break;
	    }

	    vm_object_paging_end(src_object);
	    vm_object_unlock(src_object);
	}

	copy->cpy_page_loose = TRUE;

	/*
	 * If the source should be destroyed, do it now, since the
	 * copy was successful.
	 */

	if (options & VM_MAP_COPYIN_OPT_SRC_DESTROY) {
	    (void) vm_map_delete(src_map, src_start,
				 src_end, VM_MAP_NO_FLAGS);
	}
    } else {
        /*
	 * Not stealing pages leaves busy or prepped pages in the map.
	 * This will cause source destruction to hang.  Use
	 * a continuation to prevent this.
	 */
        if ((options & VM_MAP_COPYIN_OPT_SRC_DESTROY) &&
	    !vm_map_copy_has_cont(copy)) {
	    cont_args = (vm_map_copyin_args_t) 
	                kalloc(sizeof(vm_map_copyin_args_data_t));
	    vm_map_reference(src_map);
	    cont_args->map = src_map;
	    cont_args->src_addr = (vm_offset_t) 0;
	    cont_args->src_len = (vm_size_t) 0;
	    cont_args->destroy_addr = src_start;
	    cont_args->destroy_len = src_end - src_start;
	    cont_args->options = options;

	    copy->cpy_cont_args = cont_args;
	    copy->cpy_cont = vm_map_copyin_page_list_cont;
	}
    }

    vm_map_unlock(src_map);

    *copy_result = copy;
    return(result);

error:
    vm_map_unlock(src_map);
    vm_map_copy_discard(copy);
    return(result);
}

void
vm_map_fork_share(
	vm_map_t	old_map,
	vm_map_entry_t	old_entry,
	vm_map_t	new_map)
{
	vm_object_t object;
	vm_map_entry_t new_entry;

	/*
	 *	New sharing code.  New map entry
	 *	references original object.  Internal
	 *	objects use asynchronous copy algorithm for
	 *	future copies.  First make sure we have
	 *	the right object.  If we need a shadow,
	 *	or someone else already has one, then
	 *	make a new shadow and share it.
	 */
	
	object = old_entry->object.vm_object;
	if (object == VM_OBJECT_NULL) {
		object = vm_object_allocate((vm_size_t)(old_entry->vme_end -
							old_entry->vme_start));
		old_entry->offset = 0;
		old_entry->object.vm_object = object;
		assert(!old_entry->needs_copy);
	}
	else if (object->copy_strategy !=
		 MEMORY_OBJECT_COPY_SYMMETRIC) {
		
		/*
		 *	We are already using an asymmetric
		 *	copy, and therefore we already have
		 *	the right object.
		 */
		
#if	!NORMA_VM		
		assert(! old_entry->needs_copy);
#else	/* !NORMA_VM */
		/* COPY_CALL, copy might have been deallocated since */
		assert((!old_entry->needs_copy) ||
		       (object->copy_strategy == MEMORY_OBJECT_COPY_CALL &&
			object->ref_count == 1));
#endif	/* !NORMA_VM */
	}
	else if (old_entry->needs_copy ||	/* case 1 */
		 object->shadowed ||		/* case 2 */
		 (! old_entry->is_shared &&	/* case 3 */
		  object->size >
		  (vm_size_t)(old_entry->vme_end -
			      old_entry->vme_start))) {
		
		/*
		 *	We need to create a shadow.
		 *	There are three cases here.
		 *	In the first case, we need to
		 *	complete a deferred symmetrical
		 *	copy that we participated in.
		 *	In the second and third cases,
		 *	we need to create the shadow so
		 *	that changes that we make to the
		 *	object do not interfere with
		 *	any symmetrical copies which
		 *	have occured (case 2) or which
		 *	might occur (case 3).
		 *
		 *	The first case is when we had
		 *	deferred shadow object creation
		 *	via the entry->needs_copy mechanism.
		 *	This mechanism only works when
		 *	only one entry points to the source
		 *	object, and we are about to create
		 *	a second entry pointing to the
		 *	same object. The problem is that
		 *	there is no way of mapping from
		 *	an object to the entries pointing
		 *	to it. (Deferred shadow creation
		 *	works with one entry because occurs
		 *	at fault time, and we walk from the
		 *	entry to the object when handling
		 *	the fault.)
		 *
		 *	The second case is when the object
		 *	to be shared has already been copied
		 *	with a symmetric copy, but we point
		 *	directly to the object without
		 *	needs_copy set in our entry. (This
		 *	can happen because different ranges
		 *	of an object can be pointed to by
		 *	different entries. In particular,
		 *	a single entry pointing to an object
		 *	can be split by a call to vm_inherit,
		 *	which, combined with task_create, can
		 *	result in the different entries
		 *	having different needs_copy values.)
		 *	The shadowed flag in the object allows
		 *	us to detect this case. The problem
		 *	with this case is that if this object
		 *	has or will have shadows, then we
		 *	must not perform an asymmetric copy
		 *	of this object, since such a copy
		 *	allows the object to be changed, which
		 *	will break the previous symmetrical
		 *	copies (which rely upon the object
		 *	not changing). In a sense, the shadowed
		 *	flag says "don't change this object".
		 *	We fix this by creating a shadow
		 *	object for this object, and sharing
		 *	that. This works because we are free
		 *	to change the shadow object (and thus
		 *	to use an asymmetric copy strategy);
		 *	this is also semantically correct,
		 *	since this object is temporary, and
		 *	therefore a copy of the object is
		 *	as good as the object itself. (This
		 *	is not true for permanent objects,
		 *	since the pager needs to see changes,
		 *	which won't happen if the changes
		 *	are made to a copy.)
		 *
		 *	The third case is when the object
		 *	to be shared has parts sticking
		 *	outside of the entry we're working
		 *	with, and thus may in the future
		 *	be subject to a symmetrical copy.
		 *	(This is a preemptive version of
		 *	case 2.)
		 */
		
		assert(!(object->shadowed && old_entry->is_shared));
		vm_object_shadow(&old_entry->object.vm_object,
				 &old_entry->offset,
				 (vm_size_t) (old_entry->vme_end -
					      old_entry->vme_start));
		
		/*
		 *	If we're making a shadow for other than
		 *	copy on write reasons, then we have
		 *	to remove write permission.
		 */
		
		if (!old_entry->needs_copy &&
		    (old_entry->protection & VM_PROT_WRITE)) {
			pmap_protect(vm_map_pmap(old_map),
				     old_entry->vme_start,
				     old_entry->vme_end,
				     old_entry->protection & ~VM_PROT_WRITE);
		}
		old_entry->needs_copy = FALSE;
		object = old_entry->object.vm_object;
	}
	
	/*
	 *	If object was using a symmetric copy strategy,
	 *	change its copy strategy to the default
	 *	asymmetric copy strategy, which is copy_delay
	 *	in the non-norma case and copy_call in the
	 *	norma case. Bump the reference count for the
	 *	new entry.
	 */
	
	vm_object_lock(object);
	object->ref_count++;
	vm_object_res_reference(object);
#if	NORMA_VM
	if (object->internal) {
		/*
		 * If there is no pager, or if a pager is currently
		 * being created, then wait until it has been created.
		 */
		if (! object->pager_initialized) {
			vm_object_pager_create(object);
		}
		assert(object->pager_initialized);
	}

	if (object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
		object->copy_strategy = MEMORY_OBJECT_COPY_CALL;

		/*
		 *	Notify xmm about change in copy strategy,
		 *	so that it will give correct copy strategy
		 *	to other kernels that map this object.
		 */
		vm_object_unlock(object);
		memory_object_share(object->pager, object->pager_request);
	}
	else { 
		vm_object_unlock(object);
	}
#else	/* NORMA_VM */
	if (object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
		object->copy_strategy = MEMORY_OBJECT_COPY_DELAY;
	}
	vm_object_unlock(object);
#endif	/* NORMA_VM */
	
	/*
	 *	Clone the entry, using object ref from above.
	 *	Mark both entries as shared.
	 */
	
	new_entry = vm_map_entry_create(new_map);
	vm_map_entry_copy(new_entry, old_entry);
	old_entry->is_shared = TRUE;
	new_entry->is_shared = TRUE;
	
	/*
	 *	Insert the entry into the new map -- we
	 *	know we're inserting at the end of the new
	 *	map.
	 */
	
	vm_map_entry_link(new_map, vm_map_last_entry(new_map), new_entry);
	
	/*
	 *	Update the physical map
	 */
	
	pmap_copy(new_map->pmap, old_map->pmap, new_entry->vme_start,
		  old_entry->vme_end - old_entry->vme_start,
		  old_entry->vme_start);
}

boolean_t
vm_map_fork_copy(
	vm_map_t	old_map,
	vm_map_entry_t	*old_entry_p,
	vm_map_t	new_map)
{
	vm_map_entry_t old_entry = *old_entry_p;
	vm_size_t entry_size = old_entry->vme_end - old_entry->vme_start;
	vm_offset_t start = old_entry->vme_start;
	vm_map_copy_t copy;
	vm_map_entry_t last = vm_map_last_entry(new_map);

	vm_map_unlock(old_map);
	/*
	 *	Use maxprot version of copyin because we
	 *	care about whether this memory can ever
	 *	be accessed, not just whether it's accessible
	 *	right now.
	 */
	if (vm_map_copyin_maxprot(old_map, start, entry_size, FALSE, &copy)
	    != KERN_SUCCESS) {
		/*
		 *	The map might have changed while it
		 *	was unlocked, check it again.  Skip
		 *	any blank space or permanently
		 *	unreadable region.
		 */
		vm_map_lock(old_map);
		if (!vm_map_lookup_entry(old_map, start, &last) ||
		    last->max_protection & VM_PROT_READ ==
					 VM_PROT_NONE) {
			last = last->vme_next;
		}
		*old_entry_p = last;

		/*
		 * XXX	For some error returns, want to
		 * XXX	skip to the next element.  Note
		 *	that INVALID_ADDRESS and
		 *	PROTECTION_FAILURE are handled above.
		 */
		
		return FALSE;
	}
	
	/*
	 *	Insert the copy into the new map
	 */
	
	vm_map_copy_insert(new_map, last, copy);
	
	/*
	 *	Pick up the traversal at the end of
	 *	the copied region.
	 */
	
	vm_map_lock(old_map);
	start += entry_size;
	if (! vm_map_lookup_entry(old_map, start, &last)) {
		last = last->vme_next;
	} else {
		vm_map_clip_start(old_map, last, start);
	}
	*old_entry_p = last;

	return TRUE;
}

/*
 *	vm_map_fork:
 *
 *	Create and return a new map based on the old
 *	map, according to the inheritance values on the
 *	regions in that map.
 *
 *	The source map must not be locked.
 */
vm_map_t
vm_map_fork(
	vm_map_t	old_map)
{
	pmap_t		new_pmap = pmap_create((vm_size_t) 0);
	vm_map_t	new_map;
	vm_map_entry_t	old_entry;
	vm_size_t	new_size = 0, entry_size;
	vm_map_entry_t	new_entry;
	boolean_t	src_needs_copy;
	boolean_t	new_entry_needs_copy;

	vm_map_reference_swap(old_map);
	vm_map_lock(old_map);

	new_map = vm_map_create(new_pmap,
			old_map->min_offset,
			old_map->max_offset,
			old_map->hdr.entries_pageable);

	for (
	    old_entry = vm_map_first_entry(old_map);
	    old_entry != vm_map_to_entry(old_map);
	    ) {
		if (old_entry->is_sub_map)
			panic("vm_map_fork: encountered a submap");

		entry_size = old_entry->vme_end - old_entry->vme_start;

		switch (old_entry->inheritance) {
		case VM_INHERIT_NONE:
			break;

		case VM_INHERIT_SHARE:
			vm_map_fork_share(old_map, old_entry, new_map);
			new_size += entry_size;
			break;

		case VM_INHERIT_COPY:

			/*
			 *	Inline the copy_quickly case;
			 *	upon failure, fall back on call
			 *	to vm_map_fork_copy.
			 */

			if (old_entry->wired_count != 0) {
				goto slow_vm_map_fork_copy;
			}

			new_entry = vm_map_entry_create(new_map);
			vm_map_entry_copy(new_entry, old_entry);

			if (! vm_object_copy_quickly(
						&new_entry->object.vm_object,
						old_entry->offset,
						(old_entry->vme_end -
							old_entry->vme_start),
						&src_needs_copy,
						&new_entry_needs_copy)) {
				vm_map_entry_dispose(new_map, new_entry);
				goto slow_vm_map_fork_copy;
			}

			/*
			 *	Handle copy-on-write obligations
			 */
			
			if (src_needs_copy && !old_entry->needs_copy) {
				vm_object_pmap_protect(
					old_entry->object.vm_object,
					old_entry->offset,
					(old_entry->vme_end -
							old_entry->vme_start),
					(old_entry->is_shared ? PMAP_NULL :
							old_map->pmap),
					old_entry->vme_start,
					old_entry->protection & ~VM_PROT_WRITE);

				old_entry->needs_copy = TRUE;
			}
#if	NORMA_VM
			if (src_needs_copy && 
			    old_entry->object.vm_object->internal)
				vm_object_export_pagemap_try(
						old_entry->object.vm_object,
						(old_entry->vme_end -
						 old_entry->vme_start));
#endif	/* NORMA_VM */
			new_entry->needs_copy = new_entry_needs_copy;
			
			/*
			 *	Insert the entry at the end
			 *	of the map.
			 */
			
			vm_map_entry_link(new_map, vm_map_last_entry(new_map),
					  new_entry);
			new_size += entry_size;
			break;

		slow_vm_map_fork_copy:
			if (vm_map_fork_copy(old_map, &old_entry, new_map)) {
				new_size += entry_size;
			}
			continue;
		}
		old_entry = old_entry->vme_next;
	}

	new_map->size = new_size;
	vm_map_unlock(old_map);
	vm_map_deallocate(old_map);

	return(new_map);
}

/*
 *	vm_map_lookup_locked:
 *
 *	Finds the VM object, offset, and
 *	protection for a given virtual address in the
 *	specified map, assuming a page fault of the
 *	type specified.
 *
 *	Returns the (object, offset, protection) for
 *	this address, whether it is wired down, and whether
 *	this map has the only reference to the data in question.
 *	In order to later verify this lookup, a "version"
 *	is returned.
 *
 *	The map MUST be locked by the caller and WILL be
 *	locked on exit.  In order to guarantee the
 *	existence of the returned object, it is returned
 *	locked.
 *
 *	If a lookup is requested with "write protection"
 *	specified, the map may be changed to perform virtual
 *	copying operations, although the data referenced will
 *	remain the same.
 */
kern_return_t
vm_map_lookup_locked(
	vm_map_t		*var_map,	/* IN/OUT */
	register vm_offset_t	vaddr,
	register vm_prot_t	fault_type,
	vm_map_version_t	*out_version,	/* OUT */
	vm_object_t		*object,	/* OUT */
	vm_offset_t		*offset,	/* OUT */
	vm_prot_t		*out_prot,	/* OUT */
	boolean_t		*wired,		/* OUT */
	int			*behavior,	/* OUT */
	vm_offset_t		*lo_offset,	/* OUT */
	vm_offset_t		*hi_offset)	/* OUT */
{
	register vm_map_entry_t		entry;
	register vm_map_t		map = *var_map;
	register vm_prot_t		prot;

	RetryLookup: ;

	/*
	 *	If the map has an interesting hint, try it before calling
	 *	full blown lookup routine.
	 */

	mutex_lock(&map->s_lock);
	entry = map->hint;
	mutex_unlock(&map->s_lock);

	if ((entry == vm_map_to_entry(map)) ||
	    (vaddr < entry->vme_start) || (vaddr >= entry->vme_end)) {
		vm_map_entry_t	tmp_entry;

		/*
		 *	Entry was either not a valid hint, or the vaddr
		 *	was not contained in the entry, so do a full lookup.
		 */
		if (!vm_map_lookup_entry(map, vaddr, &tmp_entry))
			return KERN_INVALID_ADDRESS;

		entry = tmp_entry;
	}

	/*
	 *	Handle submaps.  Drop lock on upper map, submap is
	 *	returned locked.
	 */

	if (entry->is_sub_map) {
		register vm_map_t	old_map = map;

		*var_map = map = entry->object.sub_map;
		vm_map_lock_read(map);
		vm_map_unlock_read(old_map);
		goto RetryLookup;
	}
		
	/*
	 *	Check whether this task is allowed to have
	 *	this page.
	 */

	prot = entry->protection;
	if ((fault_type & (prot)) != fault_type)
		return KERN_PROTECTION_FAILURE;

	/*
	 *	If this page is not pageable, we have to get
	 *	it for all possible accesses.
	 */

	if (*wired = (entry->wired_count != 0))
		prot = fault_type = entry->protection;

	/*
	 *	If the entry was copy-on-write, we either ...
	 */

	if (entry->needs_copy) {
	    	/*
		 *	If we want to write the page, we may as well
		 *	handle that now since we've got the map locked.
		 *
		 *	If we don't need to write the page, we just
		 *	demote the permissions allowed.
		 */

		if (fault_type & VM_PROT_WRITE || *wired) {
			/*
			 *	Make a new object, and place it in the
			 *	object chain.  Note that no new references
			 *	have appeared -- one just moved from the
			 *	map to the new object.
			 */

			if (vm_map_lock_read_to_write(map)) {
				vm_map_lock_read(map);
				goto RetryLookup;
			}
			vm_object_shadow(&entry->object.vm_object,
					 &entry->offset,
					 (vm_size_t) (entry->vme_end -
						      entry->vme_start));

			entry->needs_copy = FALSE;
			
			vm_map_lock_write_to_read(map);
		}
		else {
			/*
			 *	We're attempting to read a copy-on-write
			 *	page -- don't allow writes.
			 */

			prot &= (~VM_PROT_WRITE);
		}
	}

	/*
	 *	Create an object if necessary.
	 */
	if (entry->object.vm_object == VM_OBJECT_NULL) {

		if (vm_map_lock_read_to_write(map)) {
			vm_map_lock_read(map);
			goto RetryLookup;
		}

		entry->object.vm_object = vm_object_allocate(
			(vm_size_t)(entry->vme_end - entry->vme_start));
		entry->offset = 0;
		vm_map_lock_write_to_read(map);
	}

	/*
	 *	Return the object/offset from this entry.  If the entry
	 *	was copy-on-write or empty, it has been fixed up.  Also
	 *	return the protection.
	 */

        *offset = (vaddr - entry->vme_start) + entry->offset;
        *object = entry->object.vm_object;
	*out_prot = prot;
	*behavior = entry->behavior;
	*lo_offset = entry->offset;
	*hi_offset = (entry->vme_end - entry->vme_start) + entry->offset;

	/*
	 *	Lock the object to prevent it from disappearing
	 */

	vm_object_lock(*object);

	/*
	 *	Save the version number
	 */

	out_version->main_timestamp = map->timestamp;

	return KERN_SUCCESS;
}


/*
 *	vm_map_verify:
 *
 *	Verifies that the map in question has not changed
 *	since the given version.  If successful, the map
 *	will not change until vm_map_verify_done() is called.
 */
boolean_t
vm_map_verify(
	register vm_map_t		map,
	register vm_map_version_t	*version)	/* REF */
{
	boolean_t	result;

	vm_map_lock_read(map);
	result = (map->timestamp == version->main_timestamp);

	if (!result)
		vm_map_unlock_read(map);

	return(result);
}

/*
 *	vm_map_verify_done:
 *
 *	Releases locks acquired by a vm_map_verify.
 *
 *	This is now a macro in vm/vm_map.h.  It does a
 *	vm_map_unlock_read on the map.
 */


/*
 *	vm_region:
 *
 *	User call to obtain information about a region in
 *	a task's address map. Currently, only one flavor is
 *	supported.
 *
 *	XXX The reserved and behavior fields cannot be filled
 *	    in until the vm merge from the IK is completed, and
 *	    vm_reserve is implemented.
 *
 *	XXX Dependency: syscall_vm_region() also supports only one flavor.
 */

kern_return_t
vm_region(
	vm_map_t		 map,
	vm_offset_t	        *address,		/* IN/OUT */
	vm_size_t		*size,			/* OUT */
	vm_region_flavor_t	 flavor,		/* IN */
	vm_region_info_t	 info,			/* OUT */
	mach_msg_type_number_t	*count,			/* IN/OUT */
	ipc_port_t		*object_name)		/* OUT */
{
	vm_map_entry_t		tmp_entry;
	register
	vm_map_entry_t		entry;
	register
	vm_offset_t		start;
	vm_region_basic_info_t	basic;
	unsigned int		last_stamp;

	if (map == VM_MAP_NULL) 
		return(KERN_INVALID_ARGUMENT);

	if (flavor != VM_REGION_BASIC_INFO || 
	    *count  < VM_REGION_BASIC_INFO_COUNT)
		return(KERN_INVALID_ARGUMENT);

	basic = (vm_region_basic_info_t) info;
	*count = VM_REGION_BASIC_INFO_COUNT;

	vm_map_lock_read(map);

	start = *address;
	if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
		if ((entry = tmp_entry->vme_next) == vm_map_to_entry(map)) {
			vm_map_unlock_read(map);
		   	return(KERN_INVALID_ADDRESS);
		}
	} else {
		entry = tmp_entry;
	}

	start = entry->vme_start;

	basic->offset = entry->offset;
	basic->protection = entry->protection;
	basic->inheritance = entry->inheritance;
	basic->max_protection = entry->max_protection;
	basic->behavior = entry->behavior;
	basic->user_wired_count = entry->user_wired_count;
	basic->reserved = FALSE;		/* XXX when vm_reserve fini */
	*address = start;
	*size = (entry->vme_end - start);

	if (object_name) *object_name = IP_NULL;
	if (entry->is_sub_map) {
		basic->shared = FALSE;
	} else {
		basic->shared = entry->is_shared;
	}

	vm_map_unlock_read(map);

	return(KERN_SUCCESS);
}

/*
 *	Routine:	vm_map_simplify
 *
 *	Description:
 *		Attempt to simplify the map representation in
 *		the vicinity of the given starting address.
 *	Note:
 *		This routine is intended primarily to keep the
 *		kernel maps more compact -- they generally don't
 *		benefit from the "expand a map entry" technology
 *		at allocation time because the adjacent entry
 *		is often wired down.
 */
void
vm_map_simplify(
	vm_map_t	map,
	vm_offset_t	start)
{
	vm_map_entry_t	this_entry;
	vm_map_entry_t	prev_entry;
	vm_map_entry_t	next_entry;

	vm_map_lock(map);
	if (
		(vm_map_lookup_entry(map, start, &this_entry)) &&
		((prev_entry = this_entry->vme_prev) != vm_map_to_entry(map)) &&

		(prev_entry->vme_end == this_entry->vme_start) &&

		(prev_entry->is_shared == FALSE) &&
		(prev_entry->is_sub_map == FALSE) &&

		(this_entry->is_shared == FALSE) &&
		(this_entry->is_sub_map == FALSE) &&

		(prev_entry->inheritance == this_entry->inheritance) &&
		(prev_entry->protection == this_entry->protection) &&
		(prev_entry->max_protection == this_entry->max_protection) &&
		(prev_entry->behavior == this_entry->behavior) &&
		(prev_entry->wired_count == this_entry->wired_count) &&
		(prev_entry->user_wired_count == this_entry->user_wired_count)&&
		(prev_entry->in_transition == FALSE) &&
		(this_entry->in_transition == FALSE) &&

		(prev_entry->needs_copy == this_entry->needs_copy) &&

		(prev_entry->object.vm_object == this_entry->object.vm_object)&&
		((prev_entry->offset +
		 (prev_entry->vme_end - prev_entry->vme_start))
		     == this_entry->offset)
	) {
		SAVE_HINT(map, prev_entry);
		vm_map_entry_unlink(map, this_entry);
		prev_entry->vme_end = this_entry->vme_end;
		UPDATE_FIRST_FREE(map, map->first_free);
	 	vm_object_deallocate(this_entry->object.vm_object);
		vm_map_entry_dispose(map, this_entry);
		counter(c_vm_map_simplified_lower++);
	}
	if (
		(vm_map_lookup_entry(map, start, &this_entry)) &&
		((next_entry = this_entry->vme_next) != vm_map_to_entry(map)) &&

		(next_entry->vme_start == this_entry->vme_end) &&

		(next_entry->is_shared == FALSE) &&
		(next_entry->is_sub_map == FALSE) &&

		(next_entry->is_shared == FALSE) &&
		(next_entry->is_sub_map == FALSE) &&

		(next_entry->inheritance == this_entry->inheritance) &&
		(next_entry->protection == this_entry->protection) &&
		(next_entry->max_protection == this_entry->max_protection) &&
		(next_entry->behavior == this_entry->behavior) &&
		(next_entry->wired_count == this_entry->wired_count) &&
		(next_entry->user_wired_count == this_entry->user_wired_count)&&
		(this_entry->in_transition == FALSE) &&
		(next_entry->in_transition == FALSE) &&

		(next_entry->needs_copy == this_entry->needs_copy) &&

		(next_entry->object.vm_object == this_entry->object.vm_object)&&
		((this_entry->offset +
		 (this_entry->vme_end - this_entry->vme_start))
		     == next_entry->offset)
	) {
		vm_map_entry_unlink(map, next_entry);
		this_entry->vme_end = next_entry->vme_end;
		UPDATE_FIRST_FREE(map, map->first_free);
	 	vm_object_deallocate(next_entry->object.vm_object);
		vm_map_entry_dispose(map, next_entry);
		counter(c_vm_map_simplified_upper++);
	}
	counter(c_vm_map_simplify_called++);
	vm_map_unlock(map);
}


/*
 *	Routine:	vm_map_machine_attribute
 *	Purpose:
 *		Provide machine-specific attributes to mappings,
 *		such as cachability etc. for machines that provide
 *		them.  NUMA architectures and machines with big/strange
 *		caches will use this.
 *	Note:
 *		Responsibilities for locking and checking are handled here,
 *		everything else in the pmap module. If any non-volatile
 *		information must be kept, the pmap module should handle
 *		it itself. [This assumes that attributes do not
 *		need to be inherited, which seems ok to me]
 */
kern_return_t
vm_map_machine_attribute(
	vm_map_t	map,
	vm_offset_t	address,
	vm_size_t	size,
	vm_machine_attribute_t	attribute,
	vm_machine_attribute_val_t* value)		/* IN/OUT */
{
	kern_return_t	ret;

	if (address < vm_map_min(map) ||
	    (address + size) > vm_map_max(map))
		return KERN_INVALID_ADDRESS;

	vm_map_lock(map);

	ret = pmap_attribute(map->pmap, address, size, attribute, value);

	vm_map_unlock(map);

	return ret;
}

/*
 *	vm_map_behavior_set:
 *
 *	Sets the paging reference behavior of the specified address
 *	range in the target map.  Paging reference behavior affects
 *	how pagein operations resulting from faults on the map will be 
 *	clustered.
 */
kern_return_t 
vm_map_behavior_set(
	vm_map_t	map,
	vm_offset_t	start,
	vm_offset_t	end,
	vm_behavior_t	new_behavior)
{
	register vm_map_entry_t	entry;
	vm_map_entry_t	temp_entry;

	XPR(XPR_VM_MAP,
		"vm_map_behavior_set, 0x%X start 0x%X end 0x%X behavior %d",
		(integer_t)map, start, end, new_behavior, 0);

	switch (new_behavior) {
	case VM_BEHAVIOR_DEFAULT:
	case VM_BEHAVIOR_RANDOM:
	case VM_BEHAVIOR_SEQUENTIAL:
	case VM_BEHAVIOR_RSEQNTL:
		break;
	default:
		return(KERN_INVALID_ARGUMENT);
	}

	vm_map_lock(map);

	/*
	 *	The entire address range must be valid for the map.
	 * 	Note that vm_map_range_check() does a 
	 *	vm_map_lookup_entry() internally and returns the
	 *	entry containing the start of the address range if
	 *	the entire range is valid.
	 */
	if (vm_map_range_check(map, start, end, &temp_entry)) {
		entry = temp_entry;
		vm_map_clip_start(map, entry, start);
	}
	else {
		vm_map_unlock(map);
		return(KERN_INVALID_ADDRESS);
	}

	while ((entry != vm_map_to_entry(map)) && (entry->vme_start < end)) {
		vm_map_clip_end(map, entry, end);

		entry->behavior = new_behavior;

		entry = entry->vme_next;
	}

	vm_map_unlock(map);
	return(KERN_SUCCESS);
}

#if	DIPC
/*
 * This should one day be eliminated;
 * we should always construct the right flavor of copy object
 * the first time. Troublesome areas include vm_read, where vm_map_copyin
 * is called without knowing whom the copy object is for.
 * There are also situations where we do want a lazy data structure
 * even if we are sending to a remote port...
 */

extern kern_return_t vm_map_object_to_page_list(
						vm_map_copy_t	*caller_copy);

/*
 *	Convert a copy to a page list.  The copy argument is in/out
 *	because we probably have to allocate a new vm_map_copy structure.
 *	We take responsibility for discarding the old structure and
 *	use a continuation to do so.  Postponing this discard ensures
 *	that the objects containing the pages we've marked busy will stick
 *	around.  
 *
 *	N.B.  For the entry list case, be warned that this routine steals
 *	the pages from the entry list's objects!
 */
kern_return_t
vm_map_convert_to_page_list(
	vm_map_copy_t	*caller_copy)
{
	vm_map_entry_t	entry;
	vm_offset_t	va;
	vm_offset_t	offset;
	vm_object_t	object;
	vm_map_copy_t	copy, new_copy;
	vm_size_t	vm_copy_size;
	vm_prot_t	result_prot;
	vm_page_t	m, top_page;
	kern_return_t	result;
	kern_return_t	kr;

	copy = *caller_copy;

	/*

	 * We may not have to do anything,
	 * or may not be able to do anything.
	 */
	if (copy == VM_MAP_COPY_NULL || copy->type == VM_MAP_COPY_PAGE_LIST) {
		return KERN_SUCCESS;
	}
	if (copy->type == VM_MAP_COPY_OBJECT) {
		return vm_map_object_to_page_list(caller_copy);
	}
	assert(copy->type == VM_MAP_COPY_ENTRY_LIST);

	/*
	 *	Check size.  If this is really big, copy it out
	 *	to the kernel map, and copy in using src_destroy
	 */
	vm_copy_size = round_page(copy->offset + copy->size) -
	  		trunc_page(copy->offset);

	if (vm_copy_size > (VM_MAP_COPY_PAGE_LIST_MAX * PAGE_SIZE)) {
		vm_size_t	copy_size;

		offset = copy->offset;
		copy_size = copy->size;
		result = vm_map_copyout(kernel_map, &va, copy);
		assert(result == KERN_SUCCESS);
		va += offset - trunc_page(offset);
		result = vm_map_copyin_page_list(kernel_map, va, copy_size,
				(VM_MAP_COPYIN_OPT_SRC_DESTROY|
				 VM_MAP_COPYIN_OPT_STEAL_PAGES|
				 VM_PROT_READ),
				caller_copy, FALSE);
		return(result);
	}

	/*
	 *	Allocate the new copy.  Set its continuation to
	 *	discard the old one.
	 */
	new_copy = (vm_map_copy_t) zalloc(vm_map_copy_zone);
	new_copy->type = VM_MAP_COPY_PAGE_LIST;
	new_copy->cpy_npages = 0;
	new_copy->cpy_page_loose = FALSE;
	new_copy->offset = copy->offset;
	new_copy->size = copy->size;
	new_copy->cpy_cont = vm_map_copy_discard_cont;
	new_copy->cpy_cont_args = (vm_map_copyin_args_t) copy;

	/*
	 * Iterate over entries.
	 */
	for (entry = vm_map_copy_first_entry(copy);
	     entry != vm_map_copy_to_entry(copy);
	     entry = entry->vme_next) {

		object = entry->object.vm_object;
		offset = entry->offset;
		/*
		 * Iterate over pages.
		 */
		for (va = entry->vme_start;
		     va < entry->vme_end;
		     va += PAGE_SIZE, offset += PAGE_SIZE) {

			assert(new_copy->cpy_npages !=
			       VM_MAP_COPY_PAGE_LIST_MAX);

			/*
			 *	If the object is null, this is
			 *	zero fill data.
			 */
			if (object == VM_OBJECT_NULL) {
				while ((m = vm_page_grab()) == VM_PAGE_NULL) {
					VM_PAGE_WAIT();
				}
				vm_page_zero_fill(m);
				vm_page_gobble(m);
				new_copy->cpy_page_list[
						new_copy->cpy_npages++] = m;
				continue;
			}

			/*
			 *	Try to find the page of data.
			 */

			vm_object_lock(object);
			if (((m = vm_page_lookup(object, offset)) !=
			     VM_PAGE_NULL) && !m->busy && !m->fictitious &&
			    !m->unusual) {

				/*
				 *	This is the page.  Mark it busy.
				 *	Remove it from its old object.
				 */
				m->busy = TRUE;
				vm_page_lock_queues();
				VM_PAGE_QUEUES_REMOVE(m);
				vm_page_remove(m);
				vm_page_unlock_queues();
				vm_object_unlock(object);
				new_copy->cpy_page_list[new_copy->cpy_npages++]
					= m;
				continue;
			}

retry:
			result_prot = VM_PROT_READ;
			vm_object_paging_begin(object);
			XPR(XPR_VM_FAULT,
			    "vm_map_convert_to_page_list -> vm_fault_page\n",
				0,0,0,0,0);
			kr = vm_fault_page(object, offset,
					   VM_PROT_READ, FALSE, FALSE,
					   entry->offset,
					   entry->offset + 
					   (entry->vme_end -
					    entry->vme_start),
					   VM_BEHAVIOR_SEQUENTIAL,
					   &result_prot, &m, &top_page,
					   FALSE, FALSE, FALSE);
			if (kr == VM_FAULT_MEMORY_SHORTAGE) {
				VM_PAGE_WAIT();
				vm_object_lock(object);
				goto retry;
			}
			if (kr == VM_FAULT_FICTITIOUS_SHORTAGE) {
				vm_page_more_fictitious();
				vm_object_lock(object);
				goto retry;
			}
			if (kr != VM_FAULT_SUCCESS) {
				/* XXX what about data_error? */
				vm_object_lock(object);
				goto retry;
			}

			assert(m != VM_PAGE_NULL);
			m->busy = TRUE;
			vm_page_lock_queues();
			VM_PAGE_QUEUES_REMOVE(m);
			vm_page_remove(m);
			vm_page_unlock_queues();
			vm_object_paging_end(object);
			vm_object_unlock(object);
			new_copy->cpy_page_list[new_copy->cpy_npages++] = m;

			if (top_page != VM_PAGE_NULL) {
				assert(top_page->object == object);
				vm_object_lock(object);
				VM_PAGE_FREE(top_page);
				vm_object_paging_end(object);
				vm_object_unlock(object);
			}
		}
	}

	*caller_copy = new_copy;
	return KERN_SUCCESS;
}


/*
 *	Continue converting pages from an object-flavor
 *	copy object into a page-list flavor copy object.
 *	The parameters in the copy object were updated
 *	by the last invocation of vm_map_object_to_page_list
 *	so there's no work to be done other than calling
 *	back into that routine.  Note that the contents
 *	of the underlying object itself never change
 *	during the conversion process.
 */
kern_return_t
vm_map_object_to_page_list_cont(
	vm_map_copy_t	cont_args,
	vm_map_copy_t	*copy_result)	/* OUT */
{
	boolean_t	abort;

	abort = (copy_result == (vm_map_copy_t *) 0);
	if (abort) {
		printf("vm_map_object_to_page_list_cont:  abort\n");
		vm_map_copy_discard(cont_args);
	}

	assert(cont_args->type == VM_MAP_COPY_OBJECT);
	*copy_result = cont_args;
	return vm_map_object_to_page_list(copy_result);
}


/*
 *	Convert a copy object from object flavor to
 *	page list flavor.  For objects containing more
 *	than the allowable maximum number of pages, we
 *	perform the conversion a piece at a time.
 *
 *	The object flavor copy object contains a vm_object_t
 *	that has all the pages; the type, offset and size
 *	fields always present in a vm_map_copy_t; and an
 *	index field unique to the object flavor.  This index
 *	field MUST be zero when first calling this routine.
 *	It is used to record progress converting pages from
 *	the object to the page list.
 *
 *	Assumptions:
 *		- This routine always returns KERN_SUCCESS.
 */
kern_return_t
vm_map_object_to_page_list(
	vm_map_copy_t	*caller_copy)
{
	vm_object_t	object;
	vm_offset_t	offset;
	vm_map_copy_t	copy, new_copy;
	vm_size_t	index;
	vm_size_t	copy_size;
	vm_page_t	m;
	vm_prot_t	result_prot;
	vm_page_t 	top_page;
	kern_return_t 	kr;

	assert(caller_copy != (vm_map_copy_t *) 0);
	assert(*caller_copy != VM_MAP_COPY_NULL);
	assert((*caller_copy)->type == VM_MAP_COPY_OBJECT);

	copy = *caller_copy;
	object = copy->cpy_object;

	/*
	 *	Allocate the new copy.  Set its continuation to
	 *	discard the old one.
	 */
	new_copy = (vm_map_copy_t) zalloc(vm_map_copy_zone);
	new_copy->type = VM_MAP_COPY_PAGE_LIST;
	new_copy->cpy_npages = 0;
	new_copy->cpy_page_loose = FALSE;
	new_copy->offset = copy->offset - trunc_page(copy->offset);
	new_copy->size = copy->size;
	assert((long)new_copy->size >= 0);
	if (new_copy->size == 0)
		printf("vm_object_to_page_list:  zero size\n");
	new_copy->cpy_cont = vm_map_copy_discard_cont;
	new_copy->cpy_cont_args = (vm_map_copyin_args_t) copy;

	/*
	 *	Compute range of object to extract.
	 */
	index = (vm_size_t)copy->cpy_index; /* XXX!XXX! */
	assert(copy->offset < page_size);
	copy_size = round_page(copy->offset + copy->size);
	assert(page_aligned(copy_size));

	/*
	 *	Detect an object larger than the maximum permitted
	 *	for an individual page list block.  Save the rest
	 *	for later.
	 */
	if (copy_size > VM_MAP_COPY_PAGE_LIST_MAX_SIZE) {
		/*
		 *	Only the first copy object in the chain
		 *	has the unaligned offset and total size.
		 *	Each succeeding copy object has an offset
		 *	of zero and a size decreased by the amount
		 *	contained in the previous copy object.
		 *	These values will be used when creating the
		 *	next page list copy object.
		 *
		 *	Reset the continuation because there are
		 *	more pages left to extract.
		 */
		assert(trunc_page(copy->offset) == 0);
		copy->size = copy->size -
			(VM_MAP_COPY_PAGE_LIST_MAX_SIZE - copy->offset);
		copy->offset = 0;
		copy_size = VM_MAP_COPY_PAGE_LIST_MAX_SIZE;
		new_copy->cpy_cont = (vm_map_copy_cont_t)
			vm_map_object_to_page_list_cont;
		assert(vm_map_copy_cont_is_valid(new_copy));
	}

	/*
	 *	Fault in all pages that aren't present.
	 *	Modelled on vm_map_copyin_page_list.
	 */
	for (offset = index; offset < index + copy_size; offset += PAGE_SIZE) {
		vm_object_lock(object);
		vm_object_paging_begin(object);

		m = vm_page_lookup(object, offset);
		if ((m != VM_PAGE_NULL) && !m->busy && !m->fictitious &&
		    !m->unusual) {
			m->busy = TRUE;
		} else {
retry:
			result_prot = VM_PROT_READ;
			XPR(XPR_VM_FAULT,
				"vm_object_to_page_list -> vm_fault_page\n",
				0,0,0,0,0);
			kr = vm_fault_page(object, offset,
					   VM_PROT_READ, FALSE, FALSE,
					   offset, index + copy_size,/* XXX ? */
					   VM_BEHAVIOR_SEQUENTIAL,
					   &result_prot, &m, &top_page,
					   0, FALSE, FALSE);
			switch (kr) {
			    case VM_FAULT_SUCCESS:
				break;
			    case VM_FAULT_MEMORY_SHORTAGE:
				VM_PAGE_WAIT();
				vm_object_lock(object);
				vm_object_paging_begin(object);
				goto retry;
			    case VM_FAULT_FICTITIOUS_SHORTAGE:
				vm_page_more_fictitious();
				/* fall through... */
			    case VM_FAULT_INTERRUPTED:
			    case VM_FAULT_RETRY:
				vm_object_lock(object);
				vm_object_paging_begin(object);
				goto retry;
			    case VM_FAULT_MEMORY_ERROR:
				panic("vm_map_object_to_page_list");
				break;
			}

			if (top_page != VM_PAGE_NULL) {
				VM_PAGE_FREE(top_page);
				vm_object_paging_end(object);
			}
		}

		assert(m);
		assert(m->busy);
		/* assert(m->wire_count == 0); XXX */

		/*
		 *	Got the page.  Save it in the page list
		 *	and rip it away from the object.
		 */
		new_copy->cpy_page_list[new_copy->cpy_npages++] = m;
		vm_page_lock_queues();
		VM_PAGE_QUEUES_REMOVE(m);
		vm_page_remove(m);
		vm_page_unlock_queues();

		vm_object_paging_end(object);
		vm_object_unlock(object);
	}

	/*
	 *	Update index for next pass through the object.
	 */
	copy->cpy_index += copy_size;

	*caller_copy = new_copy;
	assert(vm_map_copy_cont_is_valid(new_copy));
	return (KERN_SUCCESS);
}

/*
 *	When allocating a new entry, vm_map_entry_list_from_object must
 *	use the pageable v. non-pageable entry zone based on the
 *	attribute of the map into which the entry will be pasted.
 *	This information must be supplied by the caller, as there is
 *	no way to obtain it from the object.
 *
 *	N.B.  Caller donates a reference.
 */
vm_map_copy_t
vm_map_entry_list_from_object(
	vm_object_t		object,
	vm_offset_t		offset,
	vm_size_t		size,
	boolean_t		pageable)
{
	vm_map_entry_t	new_entry;
	vm_map_copy_t	copy;

	assert(object != VM_OBJECT_NULL);
	assert(size != 0);

	copy = (vm_map_copy_t) zalloc(vm_map_copy_zone);
	assert(copy != VM_MAP_COPY_NULL);

	copy->type = VM_MAP_COPY_ENTRY_LIST;
	vm_map_copy_first_entry(copy) =
		vm_map_copy_last_entry(copy) = vm_map_copy_to_entry(copy);
	copy->cpy_hdr.nentries = 0;
	copy->cpy_hdr.entries_pageable = pageable;
	copy->offset = offset;
	copy->size = size;

	/*
	 *	Allocate and initialize an entry for the object.
	 */
	new_entry = vm_map_copy_entry_create(copy);
	new_entry->vme_start = trunc_page(copy->offset);
	new_entry->vme_end = round_page(copy->offset + copy->size);
	new_entry->object.vm_object = object;
	new_entry->offset = offset;
	new_entry->is_shared = FALSE;
	new_entry->is_sub_map = FALSE;
	new_entry->needs_copy = FALSE;
	new_entry->protection = VM_PROT_DEFAULT;
	new_entry->max_protection = VM_PROT_ALL;
	new_entry->inheritance = VM_INHERIT_DEFAULT;
	new_entry->wired_count = 0;
	new_entry->user_wired_count = 0;
	new_entry->in_transition = FALSE;

	/*
	 *	Insert entry into copy object, and return.
	 */
	vm_map_copy_entry_link(copy, vm_map_copy_last_entry(copy), new_entry);
	assert(copy->type == VM_MAP_COPY_ENTRY_LIST);
	return (copy);
}

/*
 *	Convert a page list copy object to an entry list
 *	flavor copy object.
 */
kern_return_t
vm_map_convert_to_entry_list(
	vm_map_copy_t	copy,
	boolean_t	pageable)
{
	vm_object_t object;
	int i;
	vm_map_entry_t	new_entry;
	vm_page_t	*page_list;
	vm_map_copy_t	cur_copy, new_copy;
	vm_offset_t	offset;
	kern_return_t	result;

	/*
	 * Check type of copy object.
	 */
	if (copy->type != VM_MAP_COPY_PAGE_LIST) {
		panic("vm_map_convert_to_entry_list 0x%x %d", copy, copy->type);
	}

	/*
	 *	Insert all the pages into a new object.
	 */
	object = vm_object_allocate(round_page(copy->offset + copy->size) -
				    trunc_page(copy->offset));
	offset = (vm_offset_t) 0;
	cur_copy = copy;

	while (cur_copy) {

		/*
		 *	Make sure the pages are loose.  This may be
		 *	a "Can't Happen", but just to be safe ...
		 */
		page_list = &cur_copy->cpy_page_list[0];
		if (!copy->cpy_page_loose)
			vm_map_copy_steal_pages(cur_copy);

		/*
		 * Stuff this set of pages into the object, removing
		 * them from the page list.
		 */
		vm_object_lock(object);
		vm_page_lock_queues();
		for (i = 0; i < cur_copy->cpy_npages;
		    i++, offset += PAGE_SIZE) {
			register vm_page_t m = *page_list;

			vm_page_insert(m, object, offset);
			m->busy = FALSE;
			m->dirty = TRUE;
			vm_page_activate(m);
			*page_list++ = VM_PAGE_NULL;
		}
		vm_page_unlock_queues();
		vm_object_unlock(object);

		/*
		 *	Invoke continuation if present.
		 */
		if (vm_map_copy_has_cont(cur_copy)) {
			vm_map_copy_invoke_cont(cur_copy, &new_copy, &result);

			if (result != KERN_SUCCESS)
				panic("%s:  %s",
				      "vm_map_convert_to_entry_list",
				      "continuation failure");
		} else {
			new_copy = VM_MAP_COPY_NULL;
		}		
		if (cur_copy != copy)
			vm_map_copy_discard(cur_copy);

		cur_copy = new_copy;
	}

	/*
	 * Change type of copy object
	 */
	vm_map_copy_first_entry(copy) =
	    vm_map_copy_last_entry(copy) = vm_map_copy_to_entry(copy);
	copy->type = VM_MAP_COPY_ENTRY_LIST;
	copy->cpy_hdr.nentries = 0;
	copy->cpy_hdr.entries_pageable = pageable;

	/*
	 * Allocate and initialize an entry for object
	 */
	new_entry = vm_map_copy_entry_create(copy);
	new_entry->vme_start = trunc_page(copy->offset);
	new_entry->vme_end = round_page(copy->offset + copy->size);
	new_entry->object.vm_object = object;
	new_entry->offset = 0;
	new_entry->is_shared = FALSE;
	new_entry->is_sub_map = FALSE;
	new_entry->needs_copy = FALSE;
	new_entry->protection = VM_PROT_DEFAULT;
	new_entry->max_protection = VM_PROT_ALL;
	new_entry->behavior = VM_BEHAVIOR_DEFAULT;
	new_entry->inheritance = VM_INHERIT_DEFAULT;
	new_entry->wired_count = 0;
	new_entry->user_wired_count = 0;

	/*
	 * Insert entry into copy object, and return.
	 */
	vm_map_copy_entry_link(copy, vm_map_copy_last_entry(copy), new_entry);
	return(KERN_SUCCESS);
}

#endif	/* DIPC */

int
vm_map_copy_cont_is_valid(
	vm_map_copy_t	copy)
{
	vm_map_copy_cont_t	cont;

	assert(copy->type == VM_MAP_COPY_PAGE_LIST);
	cont = copy->cpy_cont;
	if (
	    cont != vm_map_copy_discard_cont &&
	    cont != vm_map_copyin_page_list_cont ) {
		printf("vm_map_copy_cont_is_valid:  bogus cont 0x%x\n", cont);
		assert((integer_t) cont == 0xdeadbeef);
	}
	return 1;
}

#include <mach_kdb.h>
#if	MACH_KDB
#include <ddb/db_output.h>
#include <vm/vm_print.h>

#define	printf	db_printf

/*
 * Forward declarations for internal functions.
 */
extern void vm_map_links_print(
		struct vm_map_links	*links);

extern void vm_map_header_print(
		struct vm_map_header	*header);

extern void vm_map_entry_print(
		vm_map_entry_t		entry);

extern void vm_follow_entry(
		vm_map_entry_t		entry);

extern void vm_follow_map(
		vm_map_t		map);

/*
 *	vm_map_links_print:	[ debug ]
 */
void
vm_map_links_print(
	struct vm_map_links	*links)
{
	iprintf("prev=0x%x, next=0x%x, start=0x%x, end=0x%x\n",
		links->prev,
		links->next,
		links->start,
		links->end);
}

/*
 *	vm_map_header_print:	[ debug ]
 */
void
vm_map_header_print(
	struct vm_map_header	*header)
{
	vm_map_links_print(&header->links);
	iprintf("nentries=0x%x, %sentries_pageable\n",
		header->nentries,
		(header->entries_pageable ? "" : "!"));
}

/*
 *	vm_follow_entry:	[ debug ]
 */
void
vm_follow_entry(
	vm_map_entry_t entry)
{
	extern int indent;
	int shadows;

	iprintf("map entry 0x%x:\n", entry);

	indent += 2;

	shadows = vm_follow_object(entry->object.vm_object);
	iprintf("Total objects : %d\n",shadows);

	indent -= 2;
}

/*
 *	vm_map_entry_print:	[ debug ]
 */
void
vm_map_entry_print(
	register vm_map_entry_t	entry)
{
	extern int indent;
	static char *inheritance_name[4] = { "share", "copy", "none", "?"};
	static char *behavior_name[4] = { "dflt", "rand", "seqtl", "rseqntl" };
	
	iprintf("map entry 0x%x:\n", entry);

	indent += 2;

	vm_map_links_print(&entry->links);

	iprintf("start=0x%x, end=0x%x, prot=%x/%x/%s\n",
		entry->vme_start,
		entry->vme_end,
		entry->protection,
		entry->max_protection,
		inheritance_name[(entry->inheritance & 0x3)]);

	iprintf("behavior=%s, wired_count=%d, user_wired_count=%d\n",
		behavior_name[(entry->behavior & 0x3)],
		entry->wired_count,
		entry->user_wired_count);
	iprintf("%sin_transition, %sneeds_wakeup\n",
		(entry->in_transition ? "" : "!"),
		(entry->needs_wakeup ? "" : "!"));

	if (entry->is_sub_map) {
		iprintf("submap=0x%x, offset=0x%x\n",
		       entry->object.sub_map,
		       entry->offset);
	} else {
		iprintf("object=0x%x, offset=0x%x, ",
			entry->object.vm_object,
			entry->offset);
		printf("%sis_shared, %sneeds_copy\n",
		       (entry->is_shared ? "" : "!"),
		       (entry->needs_copy ? "" : "!"));
	}

	indent -= 2;
}

/*
 *	vm_follow_map:	[ debug ]
 */
void
vm_follow_map(
	vm_map_t map)
{
	register vm_map_entry_t	entry;
	extern int indent;

	iprintf("task map 0x%x:\n", map);

	indent += 2;

	for (entry = vm_map_first_entry(map);
	     entry && entry != vm_map_to_entry(map);
	     entry = entry->vme_next) {
	    vm_follow_entry(entry);
	}

	indent -= 2;
}

/*
 *	vm_map_print:	[ debug ]
 */
void
vm_map_print(
	register vm_map_t	map)
{
	register vm_map_entry_t	entry;
	extern int indent;
	char *swstate;

	iprintf("task map 0x%x:\n", map);

	indent += 2;

	vm_map_header_print(&map->hdr);

	iprintf("pmap=0x%x, size=%d, ref=%d, hint=0x%x, first_free=0x%x\n",
		map->pmap,
		map->size,
		map->ref_count,
		map->hint,
		map->first_free);

	iprintf("%swait_for_space, %swiring_required, timestamp=%d\n",
		(map->wait_for_space ? "" : "!"),
		(map->wiring_required ? "" : "!"),
		map->timestamp);

#if	TASK_SWAPPER
	switch (map->sw_state) {
	    case MAP_SW_IN:
		swstate = "SW_IN";
		break;
	    case MAP_SW_OUT:
		swstate = "SW_OUT";
		break;
	    default:
		swstate = "????";
		break;
	}
	iprintf("res=%d, sw_state=%s\n", map->res_count, swstate);
#endif	/* TASK_SWAPPER */

	for (entry = vm_map_first_entry(map);
	     entry && entry != vm_map_to_entry(map);
	     entry = entry->vme_next) {
		vm_map_entry_print(entry);
	}

	indent -= 2;
}

/*
 *	Routine:	vm_map_copy_print
 *	Purpose:
 *		Pretty-print a copy object for ddb.
 */

void
vm_map_copy_print(
	vm_map_copy_t	copy)
{
	extern int indent;
	int i, npages;
	vm_map_entry_t entry;

	printf("copy object 0x%x\n", copy);

	indent += 2;

	iprintf("type=%d", copy->type);
	switch (copy->type) {
		case VM_MAP_COPY_ENTRY_LIST:
		printf("[entry_list]");
		break;
		
		case VM_MAP_COPY_OBJECT:
		printf("[object]");
		break;
		
		case VM_MAP_COPY_PAGE_LIST:
		printf("[page_list]");
		break;
		
		case VM_MAP_COPY_KERNEL_BUFFER:
		printf("[kernel_buffer]");
		break;

		default:
		printf("[bad type]");
		break;
	}
	printf(", offset=0x%x", copy->offset);
	printf(", size=0x%x\n", copy->size);

	switch (copy->type) {
		case VM_MAP_COPY_ENTRY_LIST:
		vm_map_header_print(&copy->cpy_hdr);
		for (entry = vm_map_copy_first_entry(copy);
		     entry && entry != vm_map_copy_to_entry(copy);
		     entry = entry->vme_next) {
			vm_map_entry_print(entry);
		}
		break;

		case VM_MAP_COPY_OBJECT:
		iprintf("object=0x%x\n", copy->cpy_object);
		break;

		case VM_MAP_COPY_KERNEL_BUFFER:
		iprintf("kernel buffer=0x%x", copy->cpy_kdata);
		printf(", kalloc_size=0x%x\n", copy->cpy_kalloc_size);
		break;

		case VM_MAP_COPY_PAGE_LIST:
		iprintf("npages=%d", copy->cpy_npages);
		printf(", cont=%x", copy->cpy_cont);
		printf(", cont_args=%x\n", copy->cpy_cont_args);
		if (copy->cpy_npages < 0) {
			npages = 0;
		} else if (copy->cpy_npages > VM_MAP_COPY_PAGE_LIST_MAX) {
			npages = VM_MAP_COPY_PAGE_LIST_MAX;
		} else {
			npages = copy->cpy_npages;
		}
		iprintf("copy->cpy_page_list[0..%d] = {", npages);
		for (i = 0; i < npages - 1; i++) {
			printf("0x%x, ", copy->cpy_page_list[i]);
		}
		if (npages > 0) {
			printf("0x%x", copy->cpy_page_list[npages - 1]);
		}
		printf("}\n");
		break;
	}

	indent -=2;
}

/*
 *	db_vm_map_total_size(map)	[ debug ]
 *
 *	return the total virtual size (in bytes) of the map
 */
vm_size_t
db_vm_map_total_size(
	vm_map_t	map)
{
	vm_map_entry_t	entry;
	vm_size_t	total;

	total = 0;
	for (entry = vm_map_first_entry(map);
	     entry != vm_map_to_entry(map);
	     entry = entry->vme_next) {
		total += entry->vme_end - entry->vme_start;
	}

	return total;
}

#endif	/* MACH_KDB */

#if	NORMA_VM
/*
 *	Routine:	vm_map_entry_reset
 *
 *	Descritpion:	This routine deletes all vm_entries of a map_header.
 */
void
vm_map_entry_reset(
	struct vm_map_header	*map_header)
{
	vm_map_entry_t		entry;
	vm_map_entry_t		next_entry;

	for (entry = map_header->links.next;
	     entry != (struct vm_map_entry *)&map_header->links;
	     entry = next_entry) {
		next_entry = entry->vme_next;
		_vm_map_entry_unlink(map_header, entry);
		_vm_map_entry_dispose(map_header, entry);
	}
}
#endif	/* NORMA_VM */

/*
 *	Routine:	vm_map_entry_insert
 *
 *	Descritpion:	This routine inserts a new vm_entry in a locked map.
 */
vm_map_entry_t
vm_map_entry_insert(
	vm_map_t	map,
	vm_map_entry_t	insp_entry,
	vm_offset_t	start,
	vm_offset_t	end,
	vm_object_t	object,
	vm_offset_t	offset,
	boolean_t	needs_copy,
	boolean_t	is_shared,
	boolean_t	in_transition,
	vm_prot_t	cur_protection,
	vm_prot_t	max_protection,
	vm_behavior_t	behavior,
	vm_inherit_t	inheritance,
	unsigned	wired_count)
{
	vm_map_entry_t	new_entry;

	assert(insp_entry != (vm_map_entry_t)0);

	new_entry = vm_map_entry_create(map);

	new_entry->vme_start = start;
	new_entry->vme_end = end;
	assert(page_aligned(new_entry->vme_start));
	assert(page_aligned(new_entry->vme_end));

	new_entry->object.vm_object = object;
	new_entry->offset = offset;
	new_entry->is_shared = is_shared;
	new_entry->is_sub_map = FALSE;
	new_entry->needs_copy = needs_copy;
	new_entry->in_transition = in_transition;
	new_entry->needs_wakeup = FALSE;
	new_entry->inheritance = inheritance;
	new_entry->protection = cur_protection;
	new_entry->max_protection = max_protection;
	new_entry->behavior = behavior;
	new_entry->wired_count = wired_count;
	new_entry->user_wired_count = 0;

	/*
	 *	Insert the new entry into the list.
	 */

	vm_map_entry_link(map, insp_entry, new_entry);
	map->size += end - start;

	/*
	 *	Update the free space hint and the lookup hint.
	 */

	SAVE_HINT(map, new_entry);
	return new_entry;
}

/*
 *	Routine:	vm_remap_extract
 *
 *	Descritpion:	This routine returns a vm_entry list from a map.
 */
kern_return_t
vm_remap_extract(
	vm_map_t		map,
	vm_offset_t		addr,
	vm_size_t		size,
	boolean_t		copy,
	struct vm_map_header	*map_header,
	vm_prot_t		*cur_protection,
	vm_prot_t		*max_protection,
	/* What, no behavior? */
	vm_inherit_t		inheritance,
	boolean_t		pageable)
{
	kern_return_t		result;
	vm_size_t		mapped_size;
	vm_size_t		tmp_size;
	vm_map_entry_t		src_entry;     /* result of last map lookup */
	vm_map_entry_t		new_entry;
	vm_offset_t		offset;
	vm_offset_t		map_address;
	vm_offset_t		src_start;     /* start of entry to map */
	vm_offset_t		src_end;       /* end of region to be mapped */
	vm_object_t		object;    
	vm_map_version_t	version;
	boolean_t		src_needs_copy;
	boolean_t		new_entry_needs_copy;

	assert(map != VM_MAP_NULL);
	assert(size != 0 && size == round_page(size));
	assert(inheritance == VM_INHERIT_NONE ||
	       inheritance == VM_INHERIT_COPY ||
	       inheritance == VM_INHERIT_SHARE);

	/*
	 *	Compute start and end of region.
	 */
	src_start = trunc_page(addr);
	src_end = round_page(src_start + size);

	/*
	 *	Initialize map_header.
	 */
	map_header->links.next = (struct vm_map_entry *)&map_header->links;
	map_header->links.prev = (struct vm_map_entry *)&map_header->links;
	map_header->nentries = 0;
	map_header->entries_pageable = pageable;

	*cur_protection = VM_PROT_ALL;
	*max_protection = VM_PROT_ALL;

	map_address = 0;
	mapped_size = 0;
	result = KERN_SUCCESS;

	/*  
	 *	The specified source virtual space might correspond to
	 *	multiple map entries, need to loop on them.
	 */
	vm_map_lock(map);
	while (mapped_size != size) {
		vm_size_t	entry_size;

		/*
		 *	Find the beginning of the region.
		 */ 
		if (! vm_map_lookup_entry(map, src_start, &src_entry)) {
			result = KERN_INVALID_ADDRESS;
			break;
		}

		if (src_start < src_entry->vme_start ||
		    (mapped_size && src_start != src_entry->vme_start)) {
			result = KERN_INVALID_ADDRESS;
			break;
		}

		tmp_size = size - mapped_size;
		if (src_end > src_entry->vme_end)
			tmp_size -= (src_end - src_entry->vme_end);

		object = src_entry->object.vm_object;

		entry_size = (vm_size_t)(src_entry->vme_end -
					 src_entry->vme_start);
		if (object == VM_OBJECT_NULL) {
			object = vm_object_allocate(entry_size);
			src_entry->offset = 0;
			src_entry->object.vm_object = object;
		} else if (object->copy_strategy !=
			   MEMORY_OBJECT_COPY_SYMMETRIC) {
			/*
			 *	We are already using an asymmetric
			 *	copy, and therefore we already have
			 *	the right object.
			 */
#if	!NORMA_VM		
			assert(!src_entry->needs_copy);
#else	/* !NORMA_VM */
			/* COPY_CALL, copy might have been deallocated since */
			assert((!src_entry->needs_copy) ||
			       (object->copy_strategy ==
				MEMORY_OBJECT_COPY_CALL &&
				object->ref_count == 1));
#endif	/* !NORMA_VM */
		} else if (src_entry->needs_copy || object->shadowed ||
			   (object->internal && !src_entry->is_shared &&
			    object->size > entry_size)) {

			vm_object_shadow(&src_entry->object.vm_object,
					 &src_entry->offset,
					 entry_size);

			if (!src_entry->needs_copy &&
			    (src_entry->protection & VM_PROT_WRITE)) {
				pmap_protect(vm_map_pmap(map),
					     src_entry->vme_start,
					     src_entry->vme_end,
					     src_entry->protection &
						     ~VM_PROT_WRITE);
			}

			object = src_entry->object.vm_object;
			src_entry->needs_copy = FALSE;
		}

		offset = src_entry->offset + (src_start - src_entry->vme_start);

		vm_object_lock(object);
		object->ref_count++;	/* object ref. for the new entry */
		VM_OBJ_RES_INCR(object);
#if	NORMA_VM
		if (object->internal) {
			/*
			 * If there is no pager, or if a pager is currently
			 * being created, then wait until it has been created.
			 */
			if (! object->pager_initialized) {
				vm_object_pager_create(object);
			}
			assert(object->pager_initialized);
		}
#if	MACH_PAGEMAP
		/*
		 * Since this object will be shared, any existence
		 * info will become fatally stale.
		 */
		if (object->existence_map != VM_EXTERNAL_NULL) {
			vm_external_destroy(object->existence_map,
							object->size);
		}
#endif	/* MACH_PAGEMAP */
		if (object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
			object->copy_strategy = MEMORY_OBJECT_COPY_CALL;

			/*
			 *	Notify xmm about change in copy strategy,
			 *	so that it will give correct copy strategy
			 *	to other kernels that map this object.
			 */
			vm_object_unlock(object);
			memory_object_share(object->pager,
					    object->pager_request);
		}
		else { 
			vm_object_unlock(object);
		}
#else	/* NORMA_VM */
		if (object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
			object->copy_strategy = MEMORY_OBJECT_COPY_DELAY;
		}
		vm_object_unlock(object);
#endif	/* NORMA_VM */

		new_entry = _vm_map_entry_create(map_header);
		vm_map_entry_copy(new_entry, src_entry);

		new_entry->vme_start = map_address;
		new_entry->vme_end = map_address + tmp_size;
		new_entry->inheritance = inheritance;
		new_entry->offset = offset;

		/*
		 * The new region has to be copied now if required.
		 */
	RestartCopy:
		if (!copy) {
			src_entry->is_shared = TRUE;
			new_entry->is_shared = TRUE;
			new_entry->needs_copy = FALSE;

		} else if (src_entry->wired_count == 0 &&
			 vm_object_copy_quickly(&new_entry->object.vm_object,
						new_entry->offset,
						(new_entry->vme_end -
						    new_entry->vme_start),
						&src_needs_copy,
						&new_entry_needs_copy)) {

			new_entry->needs_copy = new_entry_needs_copy;
			new_entry->is_shared = FALSE;

			/*
			 * Handle copy_on_write semantics.
			 */
			if (src_needs_copy && !src_entry->needs_copy) {
				vm_object_pmap_protect(object,
						       offset,
						       entry_size,
						       (src_entry->is_shared ?
							PMAP_NULL : map->pmap),
						       src_entry->vme_start,
						       src_entry->protection &
						       ~VM_PROT_WRITE);

				src_entry->needs_copy = TRUE;
			}
#if	NORMA_VM
			if (src_needs_copy && object->internal)
				vm_object_export_pagemap_try(object,
							     entry_size);
#endif	/* NORMA_VM */
			/*
			 * Throw away the old object reference of the new entry.
			 */
			vm_object_deallocate(object);

		} else {
			new_entry->is_shared = FALSE;

			/*
			 * The map can be safely unlocked since we
			 * already hold a reference on the object.
			 *
			 * Record the timestamp of the map for later
			 * verification, and unlock the map.
			 */
			version.main_timestamp = map->timestamp;
			vm_map_unlock(map);

			/*
			 * Perform the copy.
			 */
			if (src_entry->wired_count > 0) {
				vm_object_lock(object);
				result = vm_object_copy_slowly(
						object,
						offset,
						entry_size,
						FALSE,
						&new_entry->object.vm_object);

				new_entry->offset = 0;
				new_entry->needs_copy = FALSE;
			} else {
				result = vm_object_copy_strategically(
						object,
						offset,
						entry_size,
						&new_entry->object.vm_object,
						&new_entry->offset,
						&new_entry_needs_copy);

				new_entry->needs_copy = new_entry_needs_copy;
			}

			/*
			 * Throw away the old object reference of the new entry.
			 */
			vm_object_deallocate(object);

			if (result != KERN_SUCCESS &&
			    result != KERN_MEMORY_RESTART_COPY) {
				_vm_map_entry_dispose(map_header, new_entry);
				break;
			}

			/*
			 * Verify that the map has not substantially
			 * changed while the copy was being made.
			 */

			vm_map_lock(map);	/* Increments timestamp once! */
			if (version.main_timestamp + 1 != map->timestamp) {
				/*
				 * Simple version comparison failed.
				 *
				 * Retry the lookup and verify that the
				 * same object/offset are still present.
				 */
				vm_object_deallocate(new_entry->
						     object.vm_object);
				_vm_map_entry_dispose(map_header, new_entry);
				if (result == KERN_MEMORY_RESTART_COPY)
					result = KERN_SUCCESS;
				continue;
			}

			if (result == KERN_MEMORY_RESTART_COPY) {
				vm_object_reference(object);
				goto RestartCopy;
			}
		}

		_vm_map_entry_link(map_header,
				   map_header->links.prev, new_entry);

		*cur_protection &= src_entry->protection;
		*max_protection &= src_entry->max_protection;

		map_address += tmp_size;
		mapped_size += tmp_size;
		src_start += tmp_size;

	} /* end while */

	vm_map_unlock(map);
	if (result != KERN_SUCCESS) {
		/*
		 * Free all allocated elements.
		 */
		for (src_entry = map_header->links.next;
		     src_entry != (struct vm_map_entry *)&map_header->links;
		     src_entry = new_entry) {
			new_entry = src_entry->vme_next;
			_vm_map_entry_unlink(map_header, src_entry);
			assert(!src_entry->is_sub_map);
			vm_object_deallocate(src_entry->object.vm_object);
			_vm_map_entry_dispose(map_header, src_entry);
		}
	}
	return result;
}

/*
 *	Routine:	vm_remap
 *
 *			Map portion of a task's address space.
 *			Mapped region must not overlap more than
 *			one vm memory object. Protections and
 *			inheritance attributes remain the same
 *			as in the original task and are	out parameters.
 *			Source and Target task can be identical
 *			Other attributes are identical as for vm_map()
 */
kern_return_t
vm_remap(
	ipc_port_t		target_task,
	vm_offset_t		*address,
	vm_size_t		size,
	vm_offset_t		mask,
	boolean_t		anywhere,
	ipc_port_t		src_task,
	vm_offset_t		memory_address,
	boolean_t		copy,
	vm_prot_t		*cur_protection,
	vm_prot_t		*max_protection,
	vm_inherit_t		inheritance)
{
	kern_return_t		result;
	vm_map_entry_t		entry;
	vm_map_entry_t		insp_entry;
	vm_map_entry_t		new_entry;
	vm_map_t		src_map;
	vm_map_t		target_map;
	struct vm_map_header	map_header;

	if (src_task == MACH_PORT_NULL || !IP_VALID(src_task))
		return KERN_INVALID_ARGUMENT;

	switch (inheritance) {
	    case VM_INHERIT_NONE:
	    case VM_INHERIT_COPY:
	    case VM_INHERIT_SHARE:
		if (size != 0 && target_task != MACH_PORT_NULL &&
		    (target_map = convert_port_to_map(target_task)))
			break;
		/*FALL THRU*/
	    default:
		return KERN_INVALID_ARGUMENT;
	}

	size = round_page(size);

	ip_lock(src_task);
	if (!ip_active(src_task)) {
		ip_unlock(src_task);
		vm_map_deallocate(target_map);
		return KERN_INVALID_ARGUMENT;
	}

#if	NORMA_VM
	if (IP_IS_REMOTE(src_task)) {
		/*
		 * Perform vm_remap operation on the node
		 * holding the source task map.
		 */
		ip_unlock(src_task);
		result = vm_remap_remote(target_task, address, size, mask,
					 anywhere, src_task, memory_address,
					 copy, cur_protection, max_protection,
					 inheritance);
		if (result == KERN_SUCCESS || result == MIG_NO_REPLY)
			ipc_port_release_send(src_task);
	} else
#endif	/* NORMA_VM */
	{

		/* inlined convert_port_to_map() w/lock already held */
		if (ip_kotype(src_task) != IKOT_TASK) {
			ip_unlock(src_task);
			vm_map_deallocate(target_map);
			return KERN_INVALID_ARGUMENT;
		}

		src_map = ((task_t)src_task->ip_kobject)->map;
		if (src_map == VM_MAP_NULL) {
			ip_unlock(src_task);
			vm_map_deallocate(target_map);
			return(KERN_INVALID_ARGUMENT);
		}

		vm_map_reference_swap(src_map);
		ip_unlock(src_task);

		result = vm_remap_extract(src_map, memory_address,
					  size, copy, &map_header,
					  cur_protection,
					  max_protection,
					  inheritance,
					  target_map->hdr.
					  entries_pageable);
		vm_map_deallocate(src_map);

		if (result != KERN_SUCCESS) {
			vm_map_deallocate(target_map);
			return result;
		}

		/*
		 * Allocate/check a range of free virtual address
		 * space for the target
		 */
		*address = trunc_page(*address);
		vm_map_lock(target_map);
		result = vm_remap_range_allocate(target_map, address, size,
						 mask, anywhere, &insp_entry);

		for (entry = map_header.links.next;
		     entry != (struct vm_map_entry *)&map_header.links;
		     entry = new_entry) {
			new_entry = entry->vme_next;
			_vm_map_entry_unlink(&map_header, entry);
			assert(!entry->is_sub_map);
			if (result == KERN_SUCCESS) {
				entry->vme_start += *address;
				entry->vme_end += *address;
				vm_map_entry_link(target_map,
						  insp_entry, entry);
				insp_entry = entry;
			} else {
				vm_object_deallocate(entry->object.vm_object);
				_vm_map_entry_dispose(&map_header, entry);
			}
		}

		if (result == KERN_SUCCESS) {
			target_map->size += size;
			SAVE_HINT(target_map, insp_entry);
		}
		vm_map_unlock(target_map);
	}

	if (result == KERN_SUCCESS && target_map->wiring_required)
		result = vm_map_wire(target_map, *address,
				     *address + size, *cur_protection, TRUE);

	vm_map_deallocate(target_map);
	return result;
}

/*
 *	Routine:	vm_remap_range_allocate
 *
 *	Description:
 *		Allocate a range in the specified virtual address map.
 *		returns the address and the map entry just before the allocated
 *		range
 *
 *	Map must be locked.
 */

kern_return_t
vm_remap_range_allocate(
	vm_map_t	map,
	vm_offset_t	*address,	/* IN/OUT */
	vm_size_t	size,
	vm_offset_t	mask,
	boolean_t	anywhere,
	vm_map_entry_t	*map_entry)	/* OUT */
{
	register vm_map_entry_t	entry;
	register vm_offset_t	start;
	register vm_offset_t	end;
	kern_return_t		result = KERN_SUCCESS;

 StartAgain: ;

    start = *address;

    if (anywhere)
    {
	/*
	 *	Calculate the first possible address.
	 */

	if (start < map->min_offset)
	    start = map->min_offset;
	if (start > map->max_offset)
	    return(KERN_NO_SPACE);
		
	/*
	 *	Look for the first possible address;
	 *	if there's already something at this
	 *	address, we have to start after it.
	 */

	assert(first_free_is_valid(map));
	if (start == map->min_offset) {
	    if ((entry = map->first_free) != vm_map_to_entry(map))
		start = entry->vme_end;
	} else {
	    vm_map_entry_t	tmp_entry;
	    if (vm_map_lookup_entry(map, start, &tmp_entry))
		start = tmp_entry->vme_end;
	    entry = tmp_entry;
	}
		
	/*
	 *	In any case, the "entry" always precedes
	 *	the proposed new region throughout the
	 *	loop:
	 */

	while (TRUE) {
	    register vm_map_entry_t	next;

	    /*
	     *	Find the end of the proposed new region.
	     *	Be sure we didn't go beyond the end, or
	     *	wrap around the address.
	     */

	    end = ((start + mask) & ~mask);
	    if (end < start)
		    return(KERN_NO_SPACE);
	    start = end;
	    end += size;

	    if ((end > map->max_offset) || (end < start)) {
		if (map->wait_for_space) {
		    if (size <= (map->max_offset -
				 map->min_offset)) {
			assert_wait((event_t) map, TRUE);
			vm_map_unlock(map);
			thread_block((void (*)(void))0);
			vm_map_lock(map);
			goto StartAgain;
		    }
		}
		
		return(KERN_NO_SPACE);
	    }

	    /*
	     *	If there are no more entries, we must win.
	     */

	    next = entry->vme_next;
	    if (next == vm_map_to_entry(map))
		break;

	    /*
	     *	If there is another entry, it must be
	     *	after the end of the potential new region.
	     */

	    if (next->vme_start >= end)
		break;

	    /*
	     *	Didn't fit -- move to the next entry.
	     */

	    entry = next;
	    start = entry->vme_end;
	}
	*address = start;
    } else {
	vm_map_entry_t		temp_entry;
	
	/*
	 *	Verify that:
	 *		the address doesn't itself violate
	 *		the mask requirement.
	 */

	if ((start & mask) != 0)
	    return(KERN_NO_SPACE);


	/*
	 *	...	the address is within bounds
	 */

	end = start + size;

	if ((start < map->min_offset) ||
	    (end > map->max_offset) ||
	    (start >= end)) {
	    return(KERN_INVALID_ADDRESS);
	}

	/*
	 *	...	the starting address isn't allocated
	 */

	if (vm_map_lookup_entry(map, start, &temp_entry))
	    return(KERN_NO_SPACE);

	entry = temp_entry;

	/*
	 *	...	the next region doesn't overlap the
	 *		end point.
	 */

	if ((entry->vme_next != vm_map_to_entry(map)) &&
	    (entry->vme_next->vme_start < end))
	    return(KERN_NO_SPACE);
    }
    *map_entry = entry;
    return(KERN_SUCCESS);
}

/*
 *	vm_map_switch:
 *
 *	Set the address map for the current thr_act to the specified map
 */

vm_map_t
vm_map_switch(
	vm_map_t	map)
{
	int		mycpu;
	thread_act_t	thr_act = current_act();
	vm_map_t	oldmap = thr_act->map;

	mp_disable_preemption();
	mycpu = cpu_number();

	/*
	 *	Deactivate the current map and activate the requested map
	 */
	PMAP_SWITCH_USER(thr_act, map, mycpu);

	mp_enable_preemption();
	return(oldmap);
}
