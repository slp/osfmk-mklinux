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
 * flipc/flipc.h
 *
 * Contains information (entry points, variable declarations) that needs
 * to be shared within the kernel implementation of flipc (device driver,
 * message engine, and auxilary kernel routines) but not outside of it.
 */

#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <mach/machine.h>
#include <vm/vm_page.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <kern/misc_protos.h>
#include <kern/sync_lock.h>
#include <kern/ipc_sync.h>
#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <flipc_buslock.h>
#include <flipc_perf.h>
#include <flipc_kkt.h>
#include <flipc_ucheck.h>

/* Return codes from flipc routines.  */
typedef enum {
  flipc_Success = 0,
  flipc_Bad_Kkt_Buffer_Size,
  flipc_No_Space,
  flipc_Buffer_Initialized,
  flipc_Buffer_Not_Initialized,
  flipc_Buffer_Corrupted,
  flipc_Send_Endpoint_Disabled,
  flipc_Send_Endpoint_Badtype,
  flipc_Send_Nobuffer,
  flipc_Bad_Node,
  flipc_Bad_Arguments,
  flipc_Bad_Thread,
  flipc_Bad_Epgroup
} flipc_return_t;

/*
 * Definitions for flipc "gateway" locks.
 *
 * These are locks which block threads attempting to acquire them when
 * they are already taken and automatically wakeup the blocked threads
 * when the lock is available.
 */
#include <kern/lock.h>

typedef unsigned int uint;

typedef struct flipc_gateway_lock {
    decl_simple_lock_data(,gateway_protect_lock)
    boolean_t threads_waiting_p;
    boolean_t gatelock_held;
    thread_t thread_holding;
} *flipc_gateway_lock_t;

/* The following macros expect an argument of type struct
   flipc_gateway_lock.  We keep track of the thread holding the lock
   just in case we are taking the lock on behalf of the user and want
   to arrange for some extra protection.  */

/* These locks should work even under the real-time kernel (in
   which kernel sections are pre-emptible).  This is because holding
   a simple lock blocks preemption (I'm told), and the sleep and wakeup
   code should work even if a thread holding the lock is pre-empted
   in the middle of its work.  */

#define FLIPC_INIT_GATELOCK(gatelock)				\
MACRO_BEGIN							\
    simple_lock_init(&(gatelock).gateway_protect_lock, ETAP_MISC_FLIPC);\
    (gatelock).threads_waiting_p = FALSE;			\
    (gatelock).gatelock_held = FALSE;				\
    (gatelock).thread_holding = THREAD_NULL;			\
MACRO_END

#define FLIPC_ACQUIRE_GATELOCK(gatelock)			\
MACRO_BEGIN							\
    do {							\
	simple_lock(&(gatelock).gateway_protect_lock);		\
	if ((gatelock).gatelock_held) {				\
	    (gatelock).threads_waiting_p = TRUE;		\
	    thread_sleep_simple_lock(				\
				     (event_t) &(gatelock),	\
				     simple_lock_addr((gatelock).gateway_protect_lock),\
				     FALSE);			\
	} else {						\
	    (gatelock).gatelock_held = TRUE;			\
	    (gatelock).thread_holding = current_thread();	\
	    simple_unlock(&(gatelock).gateway_protect_lock);	\
	    break;						\
	}							\
    } while (1);						\
MACRO_END

#define FLIPC_RELEASE_GATELOCK(gatelock)			\
MACRO_BEGIN							\
    int wakeup_needed;						\
								\
    simple_lock(&(gatelock).gateway_protect_lock);		\
    wakeup_needed = (gatelock).threads_waiting_p;		\
    (gatelock).thread_holding = THREAD_NULL;			\
    (gatelock).threads_waiting_p = FALSE;			\
    (gatelock).gatelock_held = FALSE;				\
    simple_unlock(&(gatelock).gateway_protect_lock);		\
    if (wakeup_needed)						\
	thread_wakeup((event_t) &(gatelock));			\
MACRO_END

/* Just as an accessor; no race protection.  This may be used without
   race protection to determine if the lock is held by the current
   thread, since if it is, we hold the lock and it's safe to read it,
   and if it isn't, whatever value is returned will not indicate the
   current thread.  */
#define FLIPC_SELF_HOLDS_GATELOCK_P(gatelock) \
  ((gatelock).thread_holding == current_thread())

/*
 * Definition for space for bitvector; initialized in flipcme_system_init.
 * Used in flipc_debug.[ch]
 */
#if MACH_KDB || MACH_KGDB
u_long *flipc_debug_buffer_bitvec;
#endif

/*
 * Definition of the macro that indicates where the message engine is
 * (in the kernel or in a dedicated co-processor).  This should really
 * be set in some flipc implementation specific file, but we haven't
 * set such things up yet.  Should be defined to 1 if the message engine
 * is in the kernel, and 0 if not.
 */
#define KERNEL_MESSAGE_ENGINE FLIPC_KKT

/*
 * Definition of the macro that indicates whether or not we may use
 * the real time primitives (specifically, locks) in our coding.
 */
#define REAL_TIME_PRIMITIVES 1

/*
 * Naming:
 *
 * Anything that directly interface with device driver routines will
 * have the prefix "usermsg".  Anything that is part of the message
 * engine will have the prefix flipcme.  Anything that is part of
 * the KFR will have the prefix flipckfr (with the exception of the
 * system intialization routine, which is just "flipc_"). */

/*
 * Routines that need to be visible to device driver, kfr, and
 * message engine.
 */

/* Called by flipc_system_init upon system initialization.  */
void flipcme_system_init(int *message_buffer_size,
			 int *transport_error_size);
void usermsg_init(void);

/* Called when a user requests initialization.
   This routine takes care of all synchronization with the ME.  */
extern flipc_return_t
flipckfr_cb_init(int max_endpoints, int max_epgroups,
		 int max_buffers, int max_buffers_per_endpoint,
		 int allocations_lock_policy);

/* Called when the ME needs to set the buffer active.  */
void flipcme_make_buffer_active(int endpoint_start, int endpoints,
				int epgroup_start, int epgroups,
				int bufferlist_start, int bufferlist_elements,
				int data_buffer_start, int data_buffers);

/* Called by KFR to find out if the ME thinks the buffer is active.  */
int flipcme_buffer_active(void);

/* Called by the KFR when teardown is required.  */
extern void flipcme_cb_teardown(void);

/* Called by usermsg_set_status when the duty loop needs to be
   kicked.  Also called internally to the ME on buffer
   teardown to shove everything waiting off into the void.  */
extern void flipcme_duty_loop(int ignore_buffer_active);

/* Called by the message engine when it needs to deliver a wakeup
   on an endpoint group index.  */
void flipckfr_wakeup_epgroup(int epgroup_idx);

/* Called by the device driver code on a close to teardown the
   communications buffer.  */
flipc_return_t flipckfr_teardown(void);

/* Called by the device driver code to get the init status of the
   communications buffer.  */
int flipckfr_cb_init_p(void);

/* Called by the device driver code to get the semaphore associated with a
   particular endpoint group.  */
flipc_return_t flipckfr_get_semaphore_port(int epgroup_idex,
					   ipc_port_t *semaphore);

#if REAL_TIME_PRIMITIVES
/*
 * Called by device driver routines to get the lock set
 * containing the allocations lock.  This set will only have a
 * single lock, at index 0.
 */
flipc_return_t flipckfr_allocations_lock(lock_set_t *lock);
#endif	/* REAL_TIME_PRIMITIVES */

/* Called by the device driver code when the user wants to associate a
   semaphore with an endpoint group.  */
flipc_return_t flipckfr_epgroup_set_semaphore(int epgroup_idx,
					      ipc_port_t semaphore);
