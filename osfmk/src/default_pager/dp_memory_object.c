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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
 *	Default Pager.
 *		Memory Object Management.
 */

#include "default_pager_internal.h"

/*
 * List of all vstructs.  A specific vstruct is
 * found directly via its port, this list is
 * only used for monitoring purposes by the
 * default_pager_object* calls
 */
struct vstruct_list_head	vstruct_list;

void vstruct_list_insert(vstruct_t vs);	/* forward */

void
vstruct_list_insert(
	vstruct_t vs)
{
	VSL_LOCK();
	queue_enter(&vstruct_list.vsl_queue, vs, vstruct_t, vs_links);
	vstruct_list.vsl_count++;
	VSL_UNLOCK();
}

void vstruct_list_delete(vstruct_t vs);	/* forward */

void
vstruct_list_delete(
	vstruct_t vs)
{
	VSL_LOCK();
	queue_remove(&vstruct_list.vsl_queue, vs, vstruct_t, vs_links);
	vstruct_list.vsl_count--;
	VSL_UNLOCK();
}

/*
 * We use the sequence numbers on requests to regulate
 * our parallelism.  In general, we allow multiple reads and writes
 * to proceed in parallel, with the exception that reads must
 * wait for previous writes to finish.  (Because the kernel might
 * generate a data-request for a page on the heels of a data-write
 * for the same page, and we must avoid returning stale data.)
 * terminate requests wait for proceeding reads and writes to finish.
 */

unsigned int	default_pager_total = 0;		/* debugging */
unsigned int	default_pager_wait_seqno = 0;		/* debugging */
unsigned int	default_pager_wait_read = 0;		/* debugging */
unsigned int	default_pager_wait_write = 0;		/* debugging */
unsigned int	default_pager_wait_refs = 0;		/* debugging */

void vs_async_wait(vstruct_t);	/* forward */

void
vs_async_wait(
	vstruct_t	vs)
{
	static char here[] = "vs_async_wait";

	ASSERT(vs->vs_async_pending >= 0);
	while (vs->vs_async_pending > 0) {
		condition_wait(&vs->vs_waiting_async, &vs->vs_lock);
	}
	ASSERT(vs->vs_async_pending == 0);
}

#if	PARALLEL
void vs_lock(vstruct_t, mach_port_seqno_t);
void vs_unlock(vstruct_t);
void vs_start_read(vstruct_t);
void vs_wait_for_readers(vstruct_t);
void vs_finish_read(vstruct_t);
void vs_start_write(vstruct_t);
void vs_wait_for_writers(vstruct_t);
void vs_finish_write(vstruct_t);
void vs_wait_for_refs(vstruct_t);
void vs_finish_refs(vstruct_t);

/* 
 * Waits for correct sequence number.  Leaves pager locked.
 */
void
vs_lock(
	vstruct_t		vs,
	mach_port_seqno_t	seqno)
{
	default_pager_total++;
	VS_LOCK(vs);

	while (vs->vs_seqno != seqno) {
		default_pager_wait_seqno++;
		condition_wait(&vs->vs_waiting_seqno, &vs->vs_lock);
	}
}

/*
 * Increments sequence number and unlocks pager.
 */
void
vs_unlock(vstruct_t vs)
{
	vs->vs_seqno++;
	VS_UNLOCK(vs);
	condition_broadcast(&vs->vs_waiting_seqno);
}

/* 
 * Start a read - one more reader.  Pager must be locked.
 */
void
vs_start_read(
	vstruct_t vs)
{
	vs->vs_readers++;
}

/*
 * Wait for readers.  Unlocks and relocks pager if wait needed.
 */
void
vs_wait_for_readers(
	vstruct_t vs)
{
	while (vs->vs_readers != 0) {
		default_pager_wait_read++;
		condition_wait(&vs->vs_waiting_read, &vs->vs_lock);
	}
}

/*
 * Finish a read.  Pager is unlocked and returns unlocked.
 */
void
vs_finish_read(
	vstruct_t vs)
{
	VS_LOCK(vs);
	if (--vs->vs_readers == 0) {
		VS_UNLOCK(vs);
		condition_broadcast(&vs->vs_waiting_read);
	} else
		VS_UNLOCK(vs);
}

/*
 * Start a write - one more writer.  Pager must be locked.
 */
void
vs_start_write(
	vstruct_t vs)
{
	vs->vs_writers++;
}

/* 
 * Wait for writers.  Unlocks and relocks pager if wait needed.
 */
void
vs_wait_for_writers(
	vstruct_t vs)
{
	while (vs->vs_writers != 0) {
		default_pager_wait_write++;
		condition_wait(&vs->vs_waiting_write, &vs->vs_lock);
	}
	vs_async_wait(vs);
}

/*
 * Finish a write.  Pager is unlocked and returns unlocked.
 */
void
vs_finish_write(
	vstruct_t vs)
{
	VS_LOCK(vs);
	if (--vs->vs_writers == 0) {
		VS_UNLOCK(vs);
		condition_broadcast(&vs->vs_waiting_write);
	} else
		VS_UNLOCK(vs);
}

/*
 * Wait for concurrent default_pager_objects.
 * Unlocks and relocks pager if wait needed.
 */
void
vs_wait_for_refs(
	vstruct_t vs)
{
	while (vs->vs_name_refs == 0) {
		default_pager_wait_refs++;
		condition_wait(&vs->vs_waiting_refs, &vs->vs_lock);
	}
}

/*
 * Finished creating name refs - wake up waiters.
 */
void
vs_finish_refs(
	vstruct_t vs)
{
	condition_broadcast(&vs->vs_waiting_refs);
}

#else	/* PARALLEL */

#define	vs_lock(vs,seqno)
#define	vs_unlock(vs)
#define	vs_start_read(vs)
#define	vs_wait_for_readers(vs)
#define	vs_finish_read(vs)
#define	vs_start_write(vs)
#define	vs_wait_for_writers(vs)
#define	vs_finish_write(vs)
#define vs_wait_for_refs(vs)
#define vs_finish_refs(vs)

#endif	/* PARALLEL */

vstruct_t vs_object_create(vm_size_t);	/* forward */

vstruct_t
vs_object_create(
	vm_size_t size)
{
	vstruct_t	vs;
	static char here[] = "vs_object_create";

	/*
	 * Allocate a vstruct. If there are any problems, then report them
	 * to the console.
	 */
	vs = ps_vstruct_create(size);
	if (vs == VSTRUCT_NULL) {
		dprintf(("vs_object_create: unable to allocate %s\n",
			 "-- either run swapon command or reboot"));
		return VSTRUCT_NULL;
	}

	return vs;
}

mach_port_urefs_t default_pager_max_urefs = 10000;

/*
 * Check user reference count on memory object control port.
 * Vstruct must be locked.
 * Unlocks and re-locks vstruct if needs to call kernel.
 */
void vs_check_request(vstruct_t, mach_port_t);	/* forward */

void
vs_check_request(
	vstruct_t	vs,
	mach_port_t	control_port)
{
	mach_port_delta_t delta;
	kern_return_t	kr;
	static char	here[] = "vs_check_request";

	if (++vs->vs_control_refs > default_pager_max_urefs) {
		delta = 1 - vs->vs_control_refs;
		vs->vs_control_refs = 1;

		VS_UNLOCK(vs);

		/*
		 * Deallocate excess user references.
		 */

		kr = mach_port_mod_refs(default_pager_self, control_port,
					MACH_PORT_RIGHT_SEND, delta);
		if (kr != KERN_SUCCESS)
			Panic("dealloc request send");

		VS_LOCK(vs);
	}
}

void default_pager_add(vstruct_t, boolean_t);	/* forward */

void
default_pager_add(
	vstruct_t vs,
	boolean_t internal)
{
	mach_port_t		mem_obj = vs->vs_mem_obj_port;
	mach_port_t		pset;
	mach_port_mscount_t 	sync;
	mach_port_t		previous;
	kern_return_t		kr;
	static char		here[] = "default_pager_add";

	/*
	 * The port currently has a make-send count of zero,
	 * because either we just created the port or we just
	 * received the port in a memory_object_create request.
	 */

	if (internal) {
		/* possibly generate an immediate no-senders notification */
		sync = 0;
		pset = default_pager_internal_set;
	} else {
		/* delay notification till send right is created */
		sync = 1;
		pset = default_pager_external_set;
	}

	kr = mach_port_request_notification(default_pager_self, mem_obj,
					    MACH_NOTIFY_NO_SENDERS, sync,
					    mem_obj,
					    MACH_MSG_TYPE_MAKE_SEND_ONCE,
					    &previous);
	if ((kr != KERN_SUCCESS) || (previous != MACH_PORT_NULL))
		Panic("request notify");

	kr = mach_port_move_member(default_pager_self, mem_obj, pset);
	if (kr != KERN_SUCCESS)
		Panic("add to set");
}

/*
 * Routine:	memory_object_create
 * Purpose:
 * 	Handle requests for memory objects from the
 * 	kernel.
 * Notes:
 * 	Because we only give out the default memory
 * 	manager port to the kernel, we don't have to
 * 	be so paranoid about the contents.
 */
kern_return_t
seqnos_memory_object_create(
	mach_port_t		pager,
	mach_port_seqno_t	seqno,
	mach_port_t		new_mem_obj,
	vm_size_t		new_size,
	mach_port_t		new_control_port,
	vm_size_t		new_page_size)
{
	vstruct_t		vs;
	kern_return_t		kr;
	static char		here[] = "memory_object_create";

	assert(pager == default_pager_default_port);
	assert(MACH_PORT_VALID(new_control_port));
	assert(new_page_size == vm_page_size);

	vs = vs_object_create(new_size);
	if (vs == VSTRUCT_NULL)
		return KERN_RESOURCE_SHORTAGE;

    rename_it:
	kr = mach_port_rename(default_pager_self,
			      new_mem_obj, (mach_port_t)vs_to_port(vs));
	if (kr != KERN_SUCCESS) {
		vstruct_t	vs1;

		if (kr != KERN_NAME_EXISTS)
			Panic("rename");
		vs1 = (vstruct_t) kalloc(sizeof (struct vstruct));
		*vs1 = *vs;

		VSL_LOCK();
		queue_enter(&vstruct_list.vsl_leak_queue, vs, vstruct_t,
			    vs_links);
		VSL_UNLOCK();

		vs = vs1;
		goto rename_it;
	}

	new_mem_obj = (mach_port_t) vs_to_port(vs);

	/*
	 * Set up associations between these ports
	 * and this default_pager structure
	 */

	vs->vs_mem_obj_port = new_mem_obj;
	vs->vs_control_port = new_control_port;
	vs->vs_control_refs = 1;
	vs->vs_object_name = MACH_PORT_NULL;
	vs->vs_name_refs = 1;

	/*
	 * After this, other threads might receive requests
	 * for this memory object or find it in the port list.
	 */

	vstruct_list_insert(vs);
	default_pager_add(vs, TRUE);

	return KERN_SUCCESS;
}

memory_object_copy_strategy_t
default_pager_copy_strategy = MEMORY_OBJECT_COPY_DELAY;

kern_return_t
seqnos_memory_object_init(
	mach_port_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_t		control_port,
	vm_size_t		pager_page_size)
{
	vstruct_t		vs;
	kern_return_t		kr;
	static char		here[] = "memory_object_init";
	memory_object_attr_info_data_t	attributes;

	assert(pager_page_size == vm_page_size);

	vs_lookup(mem_obj, vs);
	vs_lock(vs, seqno);

	if (vs->vs_control_port != MACH_PORT_NULL)
		Panic("bad request");

	vs->vs_control_port = control_port;
	vs->vs_control_refs = 1;
	vs->vs_object_name = MACH_PORT_NULL;
	vs->vs_name_refs = 1;

	/*
	 * Even if the kernel immediately terminates the object,
	 * the control port won't be destroyed until
	 * we process the terminate request, which won't happen
	 * until we unlock the object.
	 */

	attributes.copy_strategy = default_pager_copy_strategy;
	attributes.cluster_size = (1 << (vs->vs_clshift + vm_page_shift));
	attributes.may_cache_object = FALSE;
	attributes.temporary = TRUE;
	kr = memory_object_change_attributes(
				control_port,
				MEMORY_OBJECT_ATTRIBUTE_INFO,
				(memory_object_info_t) &attributes,
				MEMORY_OBJECT_ATTR_INFO_COUNT,
				MACH_PORT_NULL);

	if (kr != KERN_SUCCESS) {
		dprintf(("m_o_change_attributes returned 0x%x \"%s\"\n",
			 kr, mach_error_string(kr)));
		Panic("change_attributes");
	}

	vs_unlock(vs);

	return KERN_SUCCESS;
}

kern_return_t
seqnos_memory_object_synchronize(
	mach_port_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_t		control_port,
	vm_offset_t		offset,
	vm_offset_t		length,
	vm_sync_t		flags)
{
	vstruct_t	vs;
	static char	here[] = "memory_object_synchronize";

	vs_lookup(mem_obj, vs);
	vs_lock(vs, seqno);
	vs_check_request(vs, control_port);
	vs_unlock(vs);

	memory_object_synchronize_completed(control_port, offset, length);

	return KERN_SUCCESS;
}

kern_return_t
seqnos_memory_object_terminate(
	mach_port_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_t		control_port)
{
	vstruct_t		vs;
	mach_port_urefs_t	request_refs;
	kern_return_t		kr;
	static char		here[] = "memory_object_terminate";

	/* 
	 * control port is a receive right, not a send right.
	 */

	vs_lookup(mem_obj, vs);
	vs_lock(vs, seqno);

	/*
	 * Wait for read and write requests to terminate.
	 */

	vs_wait_for_readers(vs);
	vs_wait_for_writers(vs);

	/*
	 * After memory_object_terminate both memory_object_init
	 * and a no-senders notification are possible, so we need
	 * to clean up the request and name ports but leave
	 * the mem_obj port.
	 *
	 * A concurrent default_pager_objects might be allocating
	 * more references for the name port.  In this case,
	 * we must first wait for it to finish.
	 */

	vs_wait_for_refs(vs);

	vs->vs_control_port = MACH_PORT_NULL;
	request_refs = vs->vs_control_refs;
	vs->vs_control_refs = 0;

	assert(!MACH_PORT_VALID(vs->vs_object_name));
	vs->vs_object_name = MACH_PORT_NULL;

	assert(vs->vs_name_refs != 0);
	vs->vs_name_refs = 0;

	vs_unlock(vs);

	/*
	 * Now we deallocate our various port rights.
	 */

	kr = mach_port_mod_refs(default_pager_self, control_port,
				MACH_PORT_RIGHT_SEND, -request_refs);
	if (kr != KERN_SUCCESS)
		Panic("dealloc request send");

	kr = mach_port_mod_refs(default_pager_self, control_port,
				MACH_PORT_RIGHT_RECEIVE, -1);
	if (kr != KERN_SUCCESS)
		Panic("dealloc request receive");

	return KERN_SUCCESS;
}

void
default_pager_no_senders(
	memory_object_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_mscount_t	mscount)
{
	vstruct_t		vs;
	static char		here[] = "default_pager_no_senders";

	/*
	 * Because we don't give out multiple send rights
	 * for a memory object, there can't be a race
	 * between getting a no-senders notification
	 * and creating a new send right for the object.
	 * Hence we don't keep track of mscount.
	 */

	vs_lookup(mem_obj, vs);
	vs_lock(vs, seqno);
	vs_async_wait(vs);	/* wait for pending async IO */

	/*
	 * We shouldn't get a no-senders notification
	 * when the kernel has the object cached.
	 */
	if (vs->vs_control_port != MACH_PORT_NULL)
		Panic("bad request");

	/*
	 * Unlock the pager (though there should be no one
	 * waiting for it).
	 */
	VS_UNLOCK(vs);

	/*
	 * Remove the memory object port association, and then
	 * the destroy the port itself.  We must remove the object
	 * from the port list before deallocating the pager,
	 * because of default_pager_objects.
	 */
	vstruct_list_delete(vs);
	ps_vstruct_dealloc(vs);

	/*
	 * Recover memory that we might have wasted because
	 * of name conflicts
	 */
	VSL_LOCK();
	while (!queue_empty(&vstruct_list.vsl_leak_queue)) {
		vs = (vstruct_t) queue_first(&vstruct_list.vsl_leak_queue);
		queue_remove_first(&vstruct_list.vsl_leak_queue, vs, vstruct_t,
				   vs_links);
		kfree((char *) vs, sizeof *vs);
	}
	VSL_UNLOCK();
}

kern_return_t
seqnos_memory_object_data_request(
	memory_object_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_t		reply_to,
	vm_offset_t		offset,
	vm_size_t		length,
	vm_prot_t		protection_required)
{
	vstruct_t		vs;
	static char		here[] = "memory_object_data_request";

	GSTAT(global_stats.gs_pagein_calls++);

	vs_lookup(mem_obj, vs);
	vs_lock(vs, seqno);
	vs_check_request(vs, reply_to);
	vs_wait_for_writers(vs);
	vs_start_read(vs);
	vs_unlock(vs);

	/*
	 * Request must be on a page boundary and a multiple of pages.
	 */
	if ((offset & vm_page_mask) != 0 || (length & vm_page_mask) != 0)
		Panic("bad alignment");

	vs_cluster_read(vs, offset, length);

	vs_finish_read(vs);

	return KERN_SUCCESS;
}

/*
 * memory_object_data_initialize: check whether we already have each page, and
 * write it if we do not.  The implementation is far from optimized, and
 * also assumes that the default_pager is single-threaded.
 */
kern_return_t
seqnos_memory_object_data_initialize(
	memory_object_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_t		control_port,
	vm_offset_t		offset,
	pointer_t		addr,
	vm_size_t		data_cnt)
{
	vm_offset_t	dealloc_cnt = data_cnt;
	pointer_t	dealloc_addr = addr;
	vstruct_t	vs;
	struct clmap	clmap;
	static char	here[] = "memory_object_data_initialize";

#ifdef	lint
	control_port++;
#endif	/* lint */

	DEBUG(DEBUG_MO_EXTERNAL,
	      ("mem_obj=0x%x,offset=0x%x,cnt=0x%x\n",
	       (int)mem_obj, (int)offset, (int)data_cnt));
	GSTAT(global_stats.gs_pages_init += atop(data_cnt));

	vs_lookup(mem_obj, vs);
	vs_lock(vs, seqno);
	vs_check_request(vs, control_port);
	vs_start_write(vs);
	vs_unlock(vs);

	while (data_cnt > 0) {
		if (ps_clmap(vs, offset, &clmap, CL_FIND, 0, 0)
		    != (vm_offset_t) -1) {
			int			i;

			/*
			 * Cluster found, only write out pages
			 * that aren't there.
			 */
			for (i = 0; i < CLMAP_NPGS(clmap); i++) {
				if (! CLMAP_ISSET(clmap, i)) {
					GSTAT(global_stats.gs_pages_init_writes++);
					vs_cluster_write(vs, offset,
							 addr, vm_page_size);
				}
				offset += vm_page_size;
				addr += vm_page_size;
				data_cnt -= vm_page_size;
			}
		} else if (clmap.cl_error) {
			/*
			 * Previous write error in this cluster, punt silently.
			 * XXX - We could be more aggressive here by
			 * skipping this cluster and continuing with the
			 * next cluster in the object.
			 */
			break;
		} else {
			/*
			 * Allocate and write a cluster, then loop
			 * for more data, if present.
			 */
			if (ps_clmap(vs, offset, &clmap, CL_ALLOC, data_cnt, 0)
			    != (vm_offset_t) -1) {
				vm_size_t count;

				GSTAT(global_stats.gs_pages_init_writes += CLMAP_NPGS(clmap));
				count = CLMAP_NPGS(clmap) * vm_page_size;

				vs_cluster_write(vs, offset, addr, count);
				data_cnt -= count;
				offset += count;
				addr += count;
			} else {
				/*
				 * Paging file space exhausted, punt silently.
				 */
				break;
			}
		}
	}

	vs_finish_write(vs);

	if (vm_deallocate(default_pager_self, dealloc_addr, dealloc_cnt)
	    != KERN_SUCCESS)
		Panic("dealloc data");

	return KERN_SUCCESS;
}

kern_return_t
seqnos_memory_object_lock_completed(
	memory_object_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_t		control_port,
	vm_offset_t		offset,
	vm_size_t		length)
{
	static char	here[] = "memory_object_lock_completed";

#ifdef	lint
	mem_obj++; 
	seqno++; 
	control_port++; 
	offset++; 
	length++;
#endif	/* lint */

	Panic("illegal");
	return KERN_FAILURE;
}

kern_return_t
seqnos_memory_object_data_unlock(
	memory_object_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_t		control_port,
	vm_offset_t		offset,
	vm_size_t		data_cnt,
	vm_prot_t		desired_access)
{
	static char	here[] = "memory_object_data_unlock";

	Panic("illegal");
	return KERN_FAILURE;
}


kern_return_t
seqnos_memory_object_supply_completed(
	memory_object_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_t		control_port,
	vm_offset_t		offset,
	vm_size_t		length,
	kern_return_t		result,
	vm_offset_t		error_offset)
{
	static char	here[] = "memory_object_supply_completed";

	Panic("illegal");
	return KERN_FAILURE;
}

kern_return_t
seqnos_memory_object_data_return(
	memory_object_t		mem_obj,
	mach_port_seqno_t	seqno,
	mach_port_t		control_port,
	vm_offset_t		offset,
	pointer_t		addr,
	vm_size_t		data_cnt,
	boolean_t		dirty,
	boolean_t		kernel_copy)
{
	vstruct_t	vs;
	static char	here[] = "memory_object_data_return";

#ifdef	lint
	control_port++;
	dirty++;
	kernel_copy++;
#endif	/* lint */

	DEBUG(DEBUG_MO_EXTERNAL,
	      ("mem_obj=0x%x,offset=0x%x,addr=0x%xcnt=0x%x\n",
	       (int)mem_obj, (int)offset, (int)addr, (int)data_cnt));
	GSTAT(global_stats.gs_pageout_calls++);

	if ((data_cnt % vm_page_size) != 0)
		Panic("bad alignment");

	vs_lookup(mem_obj, vs);
	vs_lock(vs, seqno);
	vs_check_request(vs, control_port);
	vs_start_write(vs);
	vs_unlock(vs);

	/*
	 * Write the data via clustered writes. vs_cluster_write will
	 * loop if the address range specified crosses cluster
	 * boundaries.
	 */
	vs_cluster_write(vs, offset, addr, data_cnt);

	vs_finish_write(vs);

	if (vm_deallocate(default_pager_self, addr, data_cnt)
	    != KERN_SUCCESS)
		Panic("dealloc data");

	return KERN_SUCCESS;
}

kern_return_t
seqnos_memory_object_discard_request(
	memory_object_t		mem_obj, 
	mach_port_seqno_t	seqno,
	memory_object_control_t	memory_control,
	vm_offset_t		offset, 
	vm_size_t		length)
{
	panic("illegal");
	return KERN_FAILURE;
}

kern_return_t
seqnos_memory_object_change_completed(
	memory_object_t		mem_obj,
	mach_port_seqno_t	seqno,
	memory_object_control_t	memory_control,
	memory_object_flavor_t	flavor)
{
	static char	here[] = "memory_object_change_completed";

	Panic("illegal");
	return KERN_FAILURE;
}

/*
 * Create an external object.
 */
kern_return_t
default_pager_object_create(
	mach_port_t	pager,
	mach_port_t	*mem_obj,
	vm_size_t	size)
{
	vstruct_t	vs;
	mach_port_t	port;
	kern_return_t	result;

	if (pager != default_pager_default_port)
		return KERN_INVALID_ARGUMENT;

	vs = vs_object_create(size);

    rename_it:
	port = (mach_port_t) vs_to_port(vs);
	result = mach_port_allocate_name(default_pager_self,
					 MACH_PORT_RIGHT_RECEIVE, port);
	if (result != KERN_SUCCESS) {
		vstruct_t	vs1;

		if (result != KERN_NAME_EXISTS) 
			return result;

		vs1 = (vstruct_t) kalloc(sizeof (struct vstruct));
		*vs1 = *vs;
		VSL_LOCK();
		queue_enter(&vstruct_list.vsl_leak_queue, vs, vstruct_t,
			    vs_links);
		VSL_UNLOCK();
		vs = vs1;
		goto rename_it;
	}

	/*
	 * Set up associations between these ports
	 * and this vstruct structure
	 */

	vs->vs_mem_obj_port = port;
	vstruct_list_insert(vs);
	default_pager_add(vs, FALSE);

	*mem_obj = port;

	return KERN_SUCCESS;
}

kern_return_t
default_pager_objects(
	mach_port_t			pager,
	default_pager_object_array_t	*objectsp,
	mach_msg_type_number_t		*ocountp,
	mach_port_array_t		*portsp,
	mach_msg_type_number_t		*pcountp)
{
	vm_offset_t		oaddr = 0;	/* memory for objects */
	vm_size_t		osize = 0;	/* current size */
	default_pager_object_t	* objects;
	unsigned int		opotential;

	vm_offset_t		paddr = 0;	/* memory for ports */
	vm_size_t		psize = 0;	/* current size */
	mach_port_t		 * ports;
	unsigned int		ppotential;

	unsigned int		actual;
	unsigned int		num_objects;
	kern_return_t		kr;
	vstruct_t		entry;
	static char		here[] = "default_pager_objects";

	if (pager != default_pager_default_port)
		return KERN_INVALID_ARGUMENT;

	/* start with the inline memory */

	num_objects = 0;

	objects = *objectsp;
	opotential = *ocountp;

	ports = *portsp;
	ppotential = *pcountp;

	VSL_LOCK();

	/*
	 * We will send no more than this many
	 */
	actual = vstruct_list.vsl_count;
	VSL_UNLOCK();

	if (opotential < actual) {
		vm_offset_t	newaddr;
		vm_size_t	newsize;

		newsize = 2 * round_page(actual * sizeof * objects);

		kr = vm_allocate(default_pager_self, &newaddr, newsize, TRUE);
		if (kr != KERN_SUCCESS)
			goto nomemory;

		oaddr = newaddr;
		osize = newsize;
		opotential = osize / sizeof * objects;
		objects = (default_pager_object_t *)oaddr;
	}

	if (ppotential < actual) {
		vm_offset_t	newaddr;
		vm_size_t	newsize;

		newsize = 2 * round_page(actual * sizeof * ports);

		kr = vm_allocate(default_pager_self, &newaddr, newsize, TRUE);
		if (kr != KERN_SUCCESS)
			goto nomemory;

		paddr = newaddr;
		psize = newsize;
		ppotential = psize / sizeof * ports;
		ports = (mach_port_t *)paddr;
	}

	/*
	 * Now scan the list.
	 */

	VSL_LOCK();

	num_objects = 0;
	queue_iterate(&vstruct_list.vsl_queue, entry, vstruct_t, vs_links) {

		mach_port_t		port;
		vm_size_t		size;

		if ((num_objects >= opotential) ||
		    (num_objects >= ppotential)) {

			/*
			 * This should be rare.  In any case,
			 * we will only miss recent objects,
			 * because they are added at the end.
			 */
			break;
		}

		/*
		 * Avoid interfering with normal operations
		 */
		if (!VS_MAP_TRY_LOCK(entry))
			goto not_this_one;
		size = ps_vstruct_allocated_size(entry);
		VS_MAP_UNLOCK(entry);

		VS_LOCK(entry);

		port = entry->vs_object_name;
		if (port == MACH_PORT_NULL) {

			/*
			 * The object is waiting for no-senders
			 * or memory_object_init.
			 */
			VS_UNLOCK(entry);
			goto not_this_one;
		}

		/*
		 * We need a reference for the reply message.
		 * While we are unlocked, the bucket queue
		 * can change and the object might be terminated.
		 * memory_object_terminate will wait for us,
		 * preventing deallocation of the entry.
		 */

		if (--entry->vs_name_refs == 0) {
			VS_UNLOCK(entry);

			/* keep the list locked, wont take long */

			kr = mach_port_mod_refs(default_pager_self,
						port, MACH_PORT_RIGHT_SEND,
						default_pager_max_urefs);
			if (kr != KERN_SUCCESS)
				Panic("mod refs");

			VS_LOCK(entry);

			entry->vs_name_refs += default_pager_max_urefs;
			vs_finish_refs(entry);
		}
		VS_UNLOCK(entry);

		/* the arrays are wired, so no deadlock worries */

		objects[num_objects].dpo_object = (vm_offset_t) entry;
		objects[num_objects].dpo_size = size;
		ports  [num_objects++] = port;
		continue;

	    not_this_one:
		/*
		 * Do not return garbage
		 */
		objects[num_objects].dpo_object = (vm_offset_t) 0;
		objects[num_objects].dpo_size = 0;
		ports  [num_objects++] = MACH_PORT_NULL;

	}

	VSL_UNLOCK();

	/*
	 * Deallocate and clear unused memory.
	 * (Returned memory will automagically become pageable.)
	 */

	if (objects == *objectsp) {

		/*
		 * Our returned information fit inline.
		 * Nothing to deallocate.
		 */
		*ocountp = num_objects;
	} else if (actual == 0) {
		(void) vm_deallocate(default_pager_self, oaddr, osize);

		/* return zero items inline */
		*ocountp = 0;
	} else {
		vm_offset_t used;

		used = round_page(actual * sizeof * objects);

		if (used != osize)
			(void) vm_deallocate(default_pager_self,
					     oaddr + used, osize - used);

		*objectsp = objects;
		*ocountp = num_objects;
	}

	if (ports == *portsp) {

		/*
		 * Our returned information fit inline.
		 * Nothing to deallocate.
		 */

		*pcountp = num_objects;
	} else if (actual == 0) {
		(void) vm_deallocate(default_pager_self, paddr, psize);

		/* return zero items inline */
		*pcountp = 0;
	} else {
		vm_offset_t used;

		used = round_page(actual * sizeof * ports);

		if (used != psize)
			(void) vm_deallocate(default_pager_self,
					     paddr + used, psize - used);

		*portsp = ports;
		*pcountp = num_objects;
	}

	return KERN_SUCCESS;

    nomemory:
	{
		register int	i;
		for (i = 0; i < num_objects; i++)
			(void) mach_port_deallocate(default_pager_self,
						    ports[i]);
	}

	if (objects != *objectsp)
		(void) vm_deallocate(default_pager_self, oaddr, osize);

	if (ports != *portsp)
		(void) vm_deallocate(default_pager_self, paddr, psize);

	return KERN_RESOURCE_SHORTAGE;
}

kern_return_t
default_pager_object_pages(
	mach_port_t			pager,
	mach_port_t			object,
	default_pager_page_array_t	*pagesp,
	mach_msg_type_number_t		*countp)
{
	vm_offset_t			addr;	/* memory for page offsets */
	vm_size_t			size = 0; /* current memory size */
	default_pager_page_t		* pages;
	unsigned int			potential, actual;
	kern_return_t			kr;

	if (pager != default_pager_default_port)
		return KERN_INVALID_ARGUMENT;

	/* we start with the inline space */

	pages = *pagesp;
	potential = *countp;

	for (;;) {
		vstruct_t	entry;

		VSL_LOCK();
		queue_iterate(&vstruct_list.vsl_queue, entry, vstruct_t,
			      vs_links) {
			VS_LOCK(entry);
			if (entry->vs_object_name == object) {
				VSL_UNLOCK();
				goto found_object;
			}
			VS_UNLOCK(entry);
		}
		VSL_UNLOCK();

		/* did not find the object */

		if (pages != *pagesp)
			(void) vm_deallocate(default_pager_self, addr, size);
		return KERN_INVALID_ARGUMENT;

	    found_object:

		if (!VS_MAP_TRY_LOCK(entry)) {
			/* oh well bad luck */

			VS_UNLOCK(entry);

			/* yield the processor */
			(void) thread_switch(MACH_PORT_NULL,
					     SWITCH_OPTION_NONE,
					     0);
			continue;
		}

		actual = ps_vstruct_allocated_pages(entry, pages, potential);
		VS_MAP_UNLOCK(entry);
		VS_UNLOCK(entry);

		if (actual <= potential)
			break;

		/* allocate more memory */

		if (pages != *pagesp)
			(void) vm_deallocate(default_pager_self, addr, size);
		size = round_page(actual * sizeof * pages);
		kr = vm_allocate(default_pager_self, &addr, size, TRUE);
		if (kr != KERN_SUCCESS)
			return kr;
		pages = (default_pager_page_t *)addr;
		potential = size / sizeof * pages;
	}

	/*
	 * Deallocate and clear unused memory.
	 * (Returned memory will automagically become pageable.)
	 */

	if (pages == *pagesp) {

		/*
		 * Our returned information fit inline.
		 * Nothing to deallocate.
		 */

		*countp = actual;
	} else if (actual == 0) {
		(void) vm_deallocate(default_pager_self, addr, size);

		/* return zero items inline */
		*countp = 0;
	} else {
		vm_offset_t used;

		used = round_page(actual * sizeof * pages);

		if (used != size)
			(void) vm_deallocate(default_pager_self,
					     addr + used, size - used);

		*pagesp = pages;
		*countp = actual;
	}
	return KERN_SUCCESS;
}
