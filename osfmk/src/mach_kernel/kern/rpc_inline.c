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
 *	Custom Inline Functions for RPC
 *
 *	Inline version of functions used by RPC for performance.
 */


#if defined(__GNUC__)


#include <cpus.h>
#include <mach_debug.h>
#include <mach_ldebug.h>

#include <cputypes.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/thread_status.h>
#include <mach/vm_param.h>
#include <mach/port.h>
#include <mach/message.h>
#include <mach/alert.h>

#include <kern/assert.h>
#include <kern/misc_protos.h>
#include <kern/counters.h>
#include <kern/ipc_subsystem.h>
#include <kern/ipc_mig.h>
#include <kern/ipc_tt.h>
#include <kern/mach_param.h>
#include <kern/machine.h>
#include <kern/spl.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread_act.h>
#include <kern/thread_pool.h>
#include <kern/thread.h>
#include <kern/zalloc.h>
#include <kern/misc_protos.h>
#include <kern/sched_prim.h>
#include <kern/syscall_subr.h>
#include <kern/sync_lock.h>
#include <kern/profile.h>

#include <ipc/port.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_hash.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_right.h>
#include <ipc/ipc_notify.h>
#include <ipc/ipc_table.h>
#include <ipc/mach_msg.h>

#include <vm/vm_kern.h>
#include <vm/pmap.h>

#include <device/device_typedefs.h>

/*
 *	Change inlined function names so they will be unique.
 */

#define act_attach              act_attach_inline
#define act_detach              act_detach_inline
#define ipc_right_copyin	ipc_right_copyin_inline
#define ipc_right_copyout	ipc_right_copyout_inline
#define ipc_object_copyin       ipc_object_copyin_inline
#define ipc_object_copyout      ipc_object_copyout_inline
#define thread_pool_get_act     thread_pool_get_act_inline
#ifndef hp_pa
#define act_machine_switch_pcb	act_machine_switch_pcb_inline
#endif

extern void            	act_detach(thread_act_t);
extern void             act_attach(thread_act_t, thread_t, unsigned);
extern void            	act_machine_switch_pcb(thread_act_t);
extern thread_act_t 	thread_pool_get_act( ipc_port_t );
extern kern_return_t ipc_right_copyin(
        ipc_space_t             space,
        mach_port_t             name,
        ipc_entry_t             entry,
        mach_msg_type_name_t    msgt_name,
        boolean_t               deadok,
        ipc_object_t            *objectp,
        ipc_port_t              *sorightp);
extern kern_return_t ipc_right_copyout(
        ipc_space_t             space,
        mach_port_t             name,
        ipc_entry_t             entry,
        mach_msg_type_name_t    msgt_name,
        boolean_t               overflow,
        ipc_object_t            object);
extern kern_return_t ipc_object_copyin(
        ipc_space_t             space,
        mach_port_t             name,
        mach_msg_type_name_t    msgt_name,
        ipc_object_t            *objectp);
extern kern_return_t ipc_object_copyout(
        ipc_space_t             space,
        ipc_object_t            object,
        mach_msg_type_name_t    msgt_name,
        boolean_t               overflow,
        mach_port_t             *namep);




/* from ipc/ipc_right.c */
__inline__
kern_return_t
ipc_right_copyin(
	ipc_space_t		space,
	mach_port_t		name,
	ipc_entry_t		entry,
	mach_msg_type_name_t	msgt_name,
	boolean_t		deadok,
	ipc_object_t		*objectp,
	ipc_port_t		*sorightp)
{
	ipc_entry_bits_t bits = entry->ie_bits;

	assert(space->is_active);

	switch (msgt_name) {
	    case MACH_MSG_TYPE_MAKE_SEND: {
		ipc_port_t port;

		if ((bits & MACH_PORT_TYPE_RECEIVE) == 0)
			goto invalid_right;

		port = (ipc_port_t) entry->ie_object;
		assert(port != IP_NULL);

		ip_lock(port);
		assert(ip_active(port));
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);

		port->ip_mscount++;
		port->ip_srights++;
		ip_reference(port);
		ip_unlock(port);

		*objectp = (ipc_object_t) port;
		*sorightp = IP_NULL;
		break;
	    }

	    case MACH_MSG_TYPE_MAKE_SEND_ONCE: {
		ipc_port_t port;

		if ((bits & MACH_PORT_TYPE_RECEIVE) == 0)
			goto invalid_right;

		port = (ipc_port_t) entry->ie_object;
		assert(port != IP_NULL);

		ip_lock(port);
		assert(ip_active(port));
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);

		port->ip_sorights++;
		ip_reference(port);
		ip_unlock(port);

		*objectp = (ipc_object_t) port;
		*sorightp = IP_NULL;
		break;
	    }

	    case MACH_MSG_TYPE_MOVE_RECEIVE: {
		ipc_port_t port;
		ipc_port_t dnrequest = IP_NULL;

		if ((bits & MACH_PORT_TYPE_RECEIVE) == 0)
			goto invalid_right;

		port = (ipc_port_t) entry->ie_object;
		assert(port != IP_NULL);

		ip_lock(port);
		assert(ip_active(port));
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);

		if (bits & MACH_PORT_TYPE_SEND) {
			assert(IE_BITS_TYPE(bits) ==
					MACH_PORT_TYPE_SEND_RECEIVE);
			assert(IE_BITS_UREFS(bits) > 0);
			assert(port->ip_srights > 0);

			ipc_hash_insert(space, (ipc_object_t) port,
					name, entry);

			ip_reference(port);
		} else {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_RECEIVE);
			assert(IE_BITS_UREFS(bits) == 0);

			dnrequest = ipc_right_dncancel_macro(space, port,
							     name, entry);

			entry->ie_object = IO_NULL;
		}
		entry->ie_bits = bits &~ MACH_PORT_TYPE_RECEIVE;

		ipc_port_clear_receiver(port);

		port->ip_receiver_name = MACH_PORT_NULL;
		port->ip_destination = IP_NULL;
		ip_unlock(port);

		*objectp = (ipc_object_t) port;
		*sorightp = dnrequest;
		break;
	    }

	    case MACH_MSG_TYPE_COPY_SEND: {
		ipc_port_t port;

		if (bits & MACH_PORT_TYPE_DEAD_NAME)
			goto copy_dead;

		/* allow for dead send-once rights */

		if ((bits & MACH_PORT_TYPE_SEND_RIGHTS) == 0)
			goto invalid_right;

		assert(IE_BITS_UREFS(bits) > 0);

		port = (ipc_port_t) entry->ie_object;
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry)) {
			bits = entry->ie_bits;
			goto copy_dead;
		}
		/* port is locked and active */

		if ((bits & MACH_PORT_TYPE_SEND) == 0) {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_ONCE);
			assert(port->ip_sorights > 0);

			ip_unlock(port);
			goto invalid_right;
		}

		assert(port->ip_srights > 0);

		port->ip_srights++;
		ip_reference(port);
		ip_unlock(port);

		*objectp = (ipc_object_t) port;
		*sorightp = IP_NULL;
		break;
	    }

	    case MACH_MSG_TYPE_MOVE_SEND: {
		ipc_port_t port;
		ipc_port_t dnrequest = IP_NULL;

		if (bits & MACH_PORT_TYPE_DEAD_NAME)
			goto move_dead;

		/* allow for dead send-once rights */

		if ((bits & MACH_PORT_TYPE_SEND_RIGHTS) == 0)
			goto invalid_right;

		assert(IE_BITS_UREFS(bits) > 0);

		port = (ipc_port_t) entry->ie_object;
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry)) {
			bits = entry->ie_bits;
			goto move_dead;
		}
		/* port is locked and active */

		if ((bits & MACH_PORT_TYPE_SEND) == 0) {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_ONCE);
			assert(port->ip_sorights > 0);

			ip_unlock(port);
			goto invalid_right;
		}

		assert(port->ip_srights > 0);

		if (IE_BITS_UREFS(bits) == 1) {
			if (bits & MACH_PORT_TYPE_RECEIVE) {
				assert(port->ip_receiver_name == name);
				assert(port->ip_receiver == space);
				assert(IE_BITS_TYPE(bits) ==
						MACH_PORT_TYPE_SEND_RECEIVE);

				ip_reference(port);
			} else {
				assert(IE_BITS_TYPE(bits) ==
						MACH_PORT_TYPE_SEND);

				dnrequest = ipc_right_dncancel_macro(
						space, port, name, entry);

				ipc_hash_delete(space, (ipc_object_t) port,
						name, entry);

				entry->ie_object = IO_NULL;
			}
			entry->ie_bits = bits &~
				(IE_BITS_UREFS_MASK|MACH_PORT_TYPE_SEND);
		} else {
			port->ip_srights++;
			ip_reference(port);
			entry->ie_bits = bits-1; /* decrement urefs */
		}

		ip_unlock(port);

		*objectp = (ipc_object_t) port;
		*sorightp = dnrequest;
		break;
	    }

	    case MACH_MSG_TYPE_MOVE_SEND_ONCE: {
		ipc_port_t port;
		ipc_port_t dnrequest;

		if (bits & MACH_PORT_TYPE_DEAD_NAME)
			goto move_dead;

		/* allow for dead send rights */

		if ((bits & MACH_PORT_TYPE_SEND_RIGHTS) == 0)
			goto invalid_right;

		assert(IE_BITS_UREFS(bits) > 0);

		port = (ipc_port_t) entry->ie_object;
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry)) {
			bits = entry->ie_bits;
			goto move_dead;
		}
		/* port is locked and active */

		if ((bits & MACH_PORT_TYPE_SEND_ONCE) == 0) {
			assert(bits & MACH_PORT_TYPE_SEND);
			assert(port->ip_srights > 0);

			ip_unlock(port);
			goto invalid_right;
		}

		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_ONCE);
		assert(IE_BITS_UREFS(bits) == 1);
		assert(port->ip_sorights > 0);

		dnrequest = ipc_right_dncancel_macro(space, port, name, entry);
		ip_unlock(port);

		entry->ie_object = IO_NULL;
		entry->ie_bits = bits &~ MACH_PORT_TYPE_SEND_ONCE;

		*objectp = (ipc_object_t) port;
		*sorightp = dnrequest;
		break;
	    }

	    default:
		panic("ipc_right_copyin: strange rights");
	}

	return KERN_SUCCESS;

    copy_dead:
	assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_DEAD_NAME);
	assert(IE_BITS_UREFS(bits) > 0);
	assert(entry->ie_request == 0);
	assert(entry->ie_object == 0);

	if (!deadok)
		goto invalid_right;

	*objectp = IO_DEAD;
	*sorightp = IP_NULL;
	return KERN_SUCCESS;

    move_dead:
	assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_DEAD_NAME);
	assert(IE_BITS_UREFS(bits) > 0);
	assert(entry->ie_request == 0);
	assert(entry->ie_object == 0);

	if (!deadok)
		goto invalid_right;

	if (IE_BITS_UREFS(bits) == 1)
		entry->ie_bits = bits &~ MACH_PORT_TYPE_DEAD_NAME;
	else
		entry->ie_bits = bits-1; /* decrement urefs */

	*objectp = IO_DEAD;
	*sorightp = IP_NULL;
	return KERN_SUCCESS;

    invalid_right:
	return KERN_INVALID_RIGHT;
}


/* from ipc/ipc_right.c */
__inline__
kern_return_t
ipc_right_copyout(
	ipc_space_t		space,
	mach_port_t		name,
	ipc_entry_t		entry,
	mach_msg_type_name_t	msgt_name,
	boolean_t		overflow,
	ipc_object_t		object)
{
	ipc_entry_bits_t bits = entry->ie_bits;
	ipc_port_t port;

	assert(IO_VALID(object));
	assert(io_otype(object) == IOT_PORT);
	assert(io_active(object));
	assert(entry->ie_object == object);

	port = (ipc_port_t) object;

	switch (msgt_name) {
	    case MACH_MSG_TYPE_PORT_SEND_ONCE:
		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_NONE);
		assert(port->ip_sorights > 0);

		/* transfer send-once right and ref to entry */
		ip_unlock(port);

		entry->ie_bits = bits | (MACH_PORT_TYPE_SEND_ONCE | 1);
		break;

	    case MACH_MSG_TYPE_PORT_SEND:
		assert(port->ip_srights > 0);

		if (bits & MACH_PORT_TYPE_SEND) {
			mach_port_urefs_t urefs = IE_BITS_UREFS(bits);

			assert(port->ip_srights > 1);
			assert(urefs > 0);
			assert(urefs < MACH_PORT_UREFS_MAX);

			if (urefs+1 == MACH_PORT_UREFS_MAX) {
				if (overflow) {
					/* leave urefs pegged to maximum */

					port->ip_srights--;
					ip_release(port);
					ip_unlock(port);
					return KERN_SUCCESS;
				}

				ip_unlock(port);
				return KERN_UREFS_OVERFLOW;
			}

			port->ip_srights--;
			ip_release(port);
			ip_unlock(port);
		} else if (bits & MACH_PORT_TYPE_RECEIVE) {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_RECEIVE);
			assert(IE_BITS_UREFS(bits) == 0);

			/* transfer send right to entry */
			ip_release(port);
			ip_unlock(port);
		} else {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_NONE);
			assert(IE_BITS_UREFS(bits) == 0);

			/* transfer send right and ref to entry */
			ip_unlock(port);

			/* entry is locked holding ref, so can use port */

			ipc_hash_insert(space, (ipc_object_t) port,
					name, entry);
		}

		entry->ie_bits = (bits | MACH_PORT_TYPE_SEND) + 1;
		break;

	    case MACH_MSG_TYPE_PORT_RECEIVE: {
		ipc_port_t dest;

		assert(port->ip_mscount == 0);
		assert(port->ip_receiver_name == MACH_PORT_NULL);
		dest = port->ip_destination;

		port->ip_receiver_name = name;
		port->ip_receiver = space;

		assert((bits & MACH_PORT_TYPE_RECEIVE) == 0);

		if (bits & MACH_PORT_TYPE_SEND) {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND);
			assert(IE_BITS_UREFS(bits) > 0);
			assert(port->ip_srights > 0);

			ip_release(port);
			ip_unlock(port);

			/* entry is locked holding ref, so can use port */

			ipc_hash_delete(space, (ipc_object_t) port,
					name, entry);
		} else {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_NONE);
			assert(IE_BITS_UREFS(bits) == 0);

			/* transfer ref to entry */
			ip_unlock(port);
		}

		entry->ie_bits = bits | MACH_PORT_TYPE_RECEIVE;

		if (dest != IP_NULL)
			ipc_port_release(dest);
		break;
	    }

	    default:
		panic("ipc_right_copyout: strange rights");
	}

	return KERN_SUCCESS;
}


/* from ipc/ipc_object.c */
__inline__ 
kern_return_t
ipc_object_copyin(
	ipc_space_t		space,
	mach_port_t		name,
	mach_msg_type_name_t	msgt_name,
	ipc_object_t		*objectp)
{
	ipc_entry_t entry;
	ipc_port_t soright;
	kern_return_t kr;

	/*
	 *	Could first try a read lock when doing
	 *	MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND,
	 *	and MACH_MSG_TYPE_MAKE_SEND_ONCE.
	 */

        /*                                                              
         *      Inline expansion of:                                    
         *      ipc_right_lookup_write(space, name, &entry);       
         */                                                             
        assert(space != IS_NULL);                                       
                                                                        
        is_write_lock(space);                                           
                                                                        
        if (!space->is_active) {                                        
                is_write_unlock(space);                                 
                return KERN_INVALID_TASK;                               
        }                                                               
                                                                        
        if ((entry = ipc_entry_lookup(space, name)) == IE_NULL) {  
                is_write_unlock(space);                                 
                return KERN_INVALID_NAME;                               
        }                                                               
        /* end of inline expansion of ipc_right_lookup_write */         
                               
	/* space is write-locked and active */

	kr = ipc_right_copyin(space, name, entry,
			      msgt_name, TRUE,
			      objectp, &soright);
	if (IE_BITS_TYPE(entry->ie_bits) == MACH_PORT_TYPE_NONE)
		ipc_entry_dealloc(space, name, entry);
	is_write_unlock(space);

	if ((kr == KERN_SUCCESS) && (soright != IP_NULL))
		ipc_notify_port_deleted(soright, name);

	return kr;
}


/* from ipc/ipc_object.c */
__inline__
kern_return_t
ipc_object_copyout(
	ipc_space_t		space,
	ipc_object_t		object,
	mach_msg_type_name_t	msgt_name,
	boolean_t		overflow,
	mach_port_t		*namep)
{
	mach_port_t name;
	ipc_entry_t entry;
	kern_return_t kr;

	assert(IO_VALID(object));
	assert(io_otype(object) == IOT_PORT);

	is_write_lock(space);

	for (;;) {
		if (!space->is_active) {
			is_write_unlock(space);
			return KERN_INVALID_TASK;
		}

		if ((msgt_name != MACH_MSG_TYPE_PORT_SEND_ONCE) &&
		    ipc_right_reverse(space, object, &name, &entry)) {
			/* object is locked and active */

			assert(entry->ie_bits & MACH_PORT_TYPE_SEND_RECEIVE);
			break;
		}

		name = (mach_port_t)object;
		kr = ipc_entry_get(space, 
			msgt_name == MACH_MSG_TYPE_PORT_SEND_ONCE,
			&name, &entry);
		if (kr != KERN_SUCCESS) {
			/* unlocks/locks space, so must start again */

			kr = ipc_entry_grow_table(space, ITS_SIZE_NONE);
			if (kr != KERN_SUCCESS)
				return kr; /* space is unlocked */

			continue;
		}

		assert(IE_BITS_TYPE(entry->ie_bits) == MACH_PORT_TYPE_NONE);
		assert(entry->ie_object == IO_NULL);

		io_lock(object);
		if (!io_active(object)) {
			io_unlock(object);
			ipc_entry_dealloc(space, name, entry);
			is_write_unlock(space);
			return KERN_INVALID_CAPABILITY;
		}

		entry->ie_object = object;
		break;
	}

	/* space is write-locked and active, object is locked and active */

	kr = ipc_right_copyout(space, name, entry,
			       msgt_name, overflow, object);
	/* object is unlocked */
	is_write_unlock(space);

	if (kr == KERN_SUCCESS)
		*namep = name;
	return kr;
}


/* from kern/thread_pool.c */
__inline__
thread_act_t
thread_pool_get_act(ipc_port_t pool_port)
{
	thread_pool_t thread_pool = &pool_port->ip_thread_pool;
	thread_act_t thr_act;

#if	MACH_ASSERT
	assert(thread_pool != THREAD_POOL_NULL);
	if (watchacts & WA_ACT_LNK)
		printf("thread_pool_block: %x, waiting=%d\n",
		       thread_pool, thread_pool->waiting);
#endif

	while ((thr_act = thread_pool->thr_acts) == THR_ACT_NULL) {
		if (!ip_active(pool_port))
			return THR_ACT_NULL;
		thread_pool->waiting = 1;
		assert_wait((event_t)thread_pool, FALSE);
		ip_unlock(pool_port);
		thread_block((void (*)(void)) 0);       /* block self */
		ip_lock(pool_port);
	}
	assert(thr_act->thread == THREAD_NULL);
	assert(thr_act->suspend_count == 0);
	thread_pool->thr_acts = thr_act->thread_pool_next;
	act_lock(thr_act);
        thr_act->thread_pool_next = 0;

#if	MACH_ASSERT
	if (watchacts & WA_ACT_LNK)
		printf("thread_pool_block: return %x, next=%x\n",
		       thr_act, thread_pool->thr_acts);
#endif
	return thr_act;
}

/* from kern/thread_act.c */
__inline__
void 
act_attach(
	thread_act_t	thr_act,
	thread_t	thread,
	unsigned	init_alert_mask)
{
	thread_act_t	lower;

#if	MACH_ASSERT
	assert(thread == current_thread() || thread->top_act == THR_ACT_NULL);
	if (watchacts & WA_ACT_LNK)
		printf("act_attach(thr_act %x(%d) thread %x(%d) mask %d)\n",
		       thr_act, thr_act->ref_count, thread, thread->ref_count,
		       init_alert_mask);
#endif	/* MACH_ASSERT */

        /*
         *      Chain the thr_act onto the thread's thr_act stack.
         *      Set mask and auto-propagate alerts from below.
         */
	thr_act->thread = thread;
	thr_act->higher = THR_ACT_NULL;  /*safety*/
	thr_act->alerts = 0;
	thr_act->alert_mask = 0; /* init_alert_mask */
	lower = thr_act->lower = thread->top_act;

	if (lower != THR_ACT_NULL) {
		lower->higher = thr_act;
		thr_act->alerts = (lower->alerts & init_alert_mask);
	}
	
	thread->top_act = thr_act;
}


/* from kern/thread_act.c */
__inline__
void 
act_detach(
	thread_act_t	cur_act)
{
        thread_t        cur_thread = cur_act->thread;

#if     MACH_ASSERT
        if (watchacts & (WA_EXIT|WA_ACT_LNK))
                printf("act_detach: thr_act %x(%d), thrd %x(%d) task=%x(%d)\n",
                       cur_act, cur_act->ref_count,
                       cur_thread, cur_thread->ref_count,
                       cur_act->task,
                       cur_act->task ? cur_act->task->ref_count : 0);
#endif  /* MACH_ASSERT */

        /* Unlink the thr_act from the thread's thr_act stack */
        cur_thread->top_act = cur_act->lower;
        cur_act->thread = 0;

        thread_pool_put_act(cur_act);

#if     MACH_ASSERT
        cur_act->lower = cur_act->higher = THR_ACT_NULL;
        if (cur_thread->top_act)
                cur_thread->top_act->higher = THR_ACT_NULL;
#endif  /* MACH_ASSERT */

        return;
}


#endif	/* __GNUC__ */
