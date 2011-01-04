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
 * MkLinux
 */
/*
 * Revision 2.11.2.1  92/06/22  11:49:51  rwd
 * 	Added debugger_data field to struct cthread to be used by
 * 	call.c routines.
 * 	[92/05/28            rwd]
 * 
 * 	Define mutex_clear to be mutex_init.  This will lose the reference
 * 	to any threads waiting for it.  No better ideas...
 * 	[92/04/24            rwd]
 *
 * Revision 1.3.4.3  1993/08/04  19:55:29  gm
 * 	CR9339: Added full ANSI-C prototype support.  Replace statements
 * 	using "dangling else" clauses with "do { ... } while (0)".
 * 	CR9612: Remove includes of mach header files.
 * 	[1993/08/03  14:47:11  gm]
 *
 * Revision 1.3.4.2  1993/06/09  03:01:56  gm
 * 	Corrected include of <mach/mach.h> to <mach.h>
 * 	[1993/05/05  20:53:46  jeffc]
 * 
 * Revision 1.3  1993/04/19  17:53:12  devrcs
 * 	Prefixed the malloc routines with "cthreads_" to make sure that the
 * 	library uses its own version of the malloc package.
 * 	[93/02/25            barbou]
 * 
 * 	Include boolean.h
 * 	[1993/02/17  16:45:04  bruel]
 * 
 * 	Remove declarations for spin_unlock and spin_try_lock, which are now
 * 	in machine dependent cthread.h
 * 	[1993/02/17  12:32:39  sp]
 * 
 * 	ansi C conformance changes
 * 	[1993/02/02  17:58:30  david]
 * 
 * Revision 1.2  1992/05/12  14:55:13  devrcs
 * 	Created for OSF/1 MK
 * 	[1992/05/05  01:27:03  condict]
 * 
 * 	Changes for single internal lock version of cproc.c
 * 	[92/01/09            rwd]
 * 
 * Revision 2.11  91/08/03  18:20:15  jsb
 * 	Removed the infamous line 122.
 * 	[91/08/01  22:40:24  jsb]
 * 
 * Revision 2.10  91/07/31  18:35:42  dbg
 * 	Fix the standard-C conditional: it's __STDC__.
 * 
 * 	Allow for macro-redefinition of cthread_sp, spin_try_lock,
 * 	spin_unlock (from machine/cthreads.h).
 * 	[91/07/30  17:34:28  dbg]
 * 
 * Revision 2.9  91/05/14  17:56:42  mrt
 * 	Correcting copyright
 * 
 * Revision 2.8  91/02/14  14:19:52  mrt
 * 	Added new Mach copyright
 * 	[91/02/13  12:41:15  mrt]
 * 
 * Revision 2.7  90/11/05  14:37:12  rpd
 * 	Include machine/cthreads.h.  Added spin_lock_t.
 * 	[90/10/31            rwd]
 * 
 * Revision 2.6  90/10/12  13:07:24  rpd
 * 	Channge to allow for positive stack growth.
 * 	[90/10/10            rwd]
 * 
 * Revision 2.5  90/09/09  14:34:56  rpd
 * 	Remove mutex_special and debug_mutex.
 * 	[90/08/24            rwd]
 * 
 * Revision 2.4  90/08/07  14:31:14  rpd
 * 	Removed RCS keyword nonsense.
 * 
 * Revision 2.3  90/01/19  14:37:18  rwd
 * 	Add back pointer to cthread structure.
 * 	[90/01/03            rwd]
 * 	Change definition of cthread_init and change ur_cthread_self macro
 * 	to reflect movement of self pointer on stack.
 * 	[89/12/18  19:18:34  rwd]
 * 
 * Revision 2.2  89/12/08  19:53:49  rwd
 * 	Change spin_try_lock to int.
 * 	[89/11/30            rwd]
 * 	Changed mutex macros to deal with special mutexs
 * 	[89/11/26            rwd]
 * 	Make mutex_{set,clear}_special routines instead of macros.
 * 	[89/11/25            rwd]
 * 	Added mutex_special to specify a need to context switch on this
 * 	mutex.
 * 	[89/11/21            rwd]
 * 
 * 	Made mutex_lock a macro trying to grab the spin_lock first.
 * 	[89/11/13            rwd]
 * 	Removed conditionals.  Mutexes are more like conditions now.
 * 	Changed for limited kernel thread version.
 * 	[89/10/23            rwd]
 * 
 * Revision 2.1  89/08/03  17:09:40  rwd
 * 	Created.
 * 
 *
 * 28-Oct-88  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Implemented spin_lock() as test and test-and-set logic
 * 	(using mutex_try_lock()) in sync.c.  Changed ((char *) 0)
 * 	to 0, at Mike Jones's suggestion, and turned on ANSI-style
 * 	declarations in either C++ or _STDC_.
 * 
 * 29-Sep-88  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Changed NULL to ((char *) 0) to avoid dependency on <stdio.h>,
 * 	at Alessandro Forin's suggestion.
 * 
 * 08-Sep-88  Alessandro Forin (af) at Carnegie Mellon University
 * 	Changed queue_t to cthread_queue_t and string_t to char *
 * 	to avoid conflicts.
 * 
 * 01-Apr-88  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Changed compound statement macros to use the
 * 	do { ... } while (0) trick, so that they work
 * 	in all statement contexts.
 * 
 * 19-Feb-88  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Made spin_unlock() and mutex_unlock() into procedure calls
 * 	rather than macros, so that even smart compilers can't reorder
 * 	the clearing of the lock.  Suggested by Jeff Eppinger.
 * 	Removed the now empty <machine>/cthreads.h.
 * 
 * 01-Dec-87  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Changed cthread_self() to mask the current SP to find
 * 	the self pointer stored at the base of the stack.
 * 
 * 22-Jul-87  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Fixed bugs in mutex_set_name and condition_set_name
 * 	due to bad choice of macro formal parameter name.
 * 
 * 21-Jul-87  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Moved #include <machine/cthreads.h> to avoid referring
 * 	to types before they are declared (required by C++).
 * 
 *  9-Jul-87  Michael Jones (mbj) at Carnegie Mellon University
 * 	Added conditional type declarations for C++.
 * 	Added _cthread_init_routine and _cthread_exit_routine variables
 * 	for automatic initialization and finalization by crt0.
 */
/*
 * 	File: 	cthread.h
 *	Author: Eric Cooper, Carnegie Mellon University
 *	Date:	Jul, 1987
 *
 * 	Definitions for the C Threads package.
 *
 */


#ifndef	_CTHREADS_
#define	_CTHREADS_ 1

#include <mach/boolean.h>
#include <mach/port.h>
#include <mach/message.h>
#include <mach/machine/vm_types.h>
#include <mach/std_types.h>

typedef void *(*cthread_fn_t)(void *arg);
#define CTHREAD_NOFUNC ((cthread_fn_t)(-1))

/*
 * Typedef above necessary for function prototypes.
 */

#include <machine/cthreads.h>

/*
 * Queues.
 */
typedef struct cthread_queue {
	struct cthread_queue_item *head;
	struct cthread_queue_item *tail;
} *cthread_queue_t;

typedef struct cthread_queue_item {
	struct cthread_queue_item *next;
} *cthread_queue_item_t;

#define	NO_QUEUE_ITEM	((cthread_queue_item_t) 0)

#define	QUEUE_INITIALIZER	{ NO_QUEUE_ITEM, NO_QUEUE_ITEM }

/*
 * Spin locks.
 */
extern void spin_lock_solid(spin_lock_t *);
#ifndef	spin_unlock
extern void spin_unlock(spin_lock_t *);
#endif
#ifndef	spin_try_lock
extern boolean_t spin_try_lock(spin_lock_t *);
#endif

#define spin_lock(p) do { if (!spin_try_lock(p)) spin_lock_solid(p); }while (0)

/*
 * Mutex objects.
 */
typedef struct mutex {
    const char *name;
    struct cthread_queue queue;
    spin_lock_t held;
    int trigger;
} *mutex_t;

#define	MUTEX_INITIALIZER	{ 0, QUEUE_INITIALIZER, SPIN_LOCK_INITIALIZER, 0}
extern mutex_t mutex_alloc(void);
extern void mutex_init(mutex_t);
extern void mutex_set_name(mutex_t, const char *);
extern const char *mutex_name(mutex_t);
extern void mutex_clear(mutex_t);
extern void mutex_free(mutex_t);
extern void mutex_lock_solid(mutex_t);
extern void mutex_unlock_solid(mutex_t);

#define mutex_try_lock(m) spin_try_lock(&(m)->held)
#define mutex_lock(m) do {if (!spin_try_lock(&(m)->held)) mutex_lock_solid((m));} while (0)
#define mutex_unlock(m) do {if (spin_unlock(&(m)->held),(m)->trigger)mutex_unlock_solid(m);} while (0)

/*
 * Condition variables.
 */
typedef struct condition {
    struct cthread_queue queue;
    const char *name;
} *condition_t;

extern condition_t condition_alloc(void);
extern void condition_init(condition_t);
extern void condition_set_name(condition_t, const char *);
extern const char *condition_name(condition_t);
extern void condition_clear(condition_t);
extern void condition_free(condition_t);
extern void condition_signal(condition_t);
extern void condition_broadcast(condition_t);
extern void condition_wait(condition_t, mutex_t);

#define	CONDITION_INITIALIZER		{ QUEUE_INITIALIZER, 0 }

/*
 * Threads.
 */
typedef struct cthread *cthread_t;

#define	CTHREAD_NULL	((cthread_t) 0)

extern cthread_t cthread_fork(cthread_fn_t, void *);
extern cthread_t cthread_create_handle(vm_offset_t *);
extern void cthread_detach(cthread_t);
extern void *cthread_join(cthread_t);
extern void cthread_yield(void);
extern void cthread_exit(void *);
extern void cthread_wire(void);
extern void cthread_wire_other(cthread_t, mach_port_t);
extern void cthread_unwire(void);
extern int cthread_kernel_limit(void);
extern void cthread_set_kernel_limit(int);
extern cthread_t cthread_self(void);
extern vm_offset_t cthread_stack_base(cthread_t, vm_size_t);
extern void cthread_set_name(cthread_t, const char *);
extern const char *cthread_name(cthread_t);
extern int cthread_count(void);
extern void cthread_set_limit(int);
extern int cthread_limit(void);
extern void cthread_set_data(cthread_t, void *);
extern void *cthread_data(cthread_t);
extern void cthread_fork_prepare(void);
extern void cthread_fork_child(void);
extern void cthread_fork_parent(void);
extern cthread_t cthread_ptr(vm_offset_t sp);

#ifndef	cthread_sp
extern void *cthread_sp(void);
#endif

extern vm_size_t cthread_stack_size;
extern vm_size_t cthread_wait_stack_size;
extern vm_size_t cthread_red_zone_size; /* When set, this will cause a
					 * red_zone of at least this size
					 * to go in between stacks */

struct cthread_mach_msg_struct {
    mach_msg_header_t *header;
    mach_msg_option_t option;
    mach_msg_size_t send_size;
    mach_msg_size_t rcv_size;
    mach_port_t rcv_name;
    mach_msg_timeout_t timeout;
    mach_port_t notify;
    int min;
    int max;
};

typedef struct cthread_info_struct {
    vm_address_t remote_cthread;
    char *name;
    boolean_t running;
    boolean_t runnable;
    boolean_t waiter;
} cthread_info_t;
    
extern void cthread_msg_busy(mach_port_t, int, int);
extern void cthread_msg_active(mach_port_t, int, int);
extern kern_return_t cthread_mach_msg(struct cthread_mach_msg_struct *);

/*
 * Because of the mach3_task_t and mach3_thread_t rename
 * that occurs for reasons I fail to understand, we must
 * specify thread_t and task_t in these prototypes as
 * mach_port_t
 */
extern kern_return_t cthread_init_thread_calls(mach_port_t, vm_address_t);
extern kern_return_t cthread_get_state(cthread_t, int, thread_state *, int *);
extern kern_return_t cthread_set_state(cthread_t, int, thread_state *, int);
extern kern_return_t cthrad_info(cthread_t, cthread_info_t *);
extern kern_return_t cthread_abort(cthread_t);
extern kern_return_t cthread_resume(cthread_t);
extern kern_return_t cthread_suspend(cthread_t);
extern kern_return_t cthread_threads(mach_port_t, cthread_t **, int *);
extern cthread_t cthread_thread_to_cthread(mach_port_t);
extern void cthread_thread_calls_start(void);
extern void cthread_thread_calls_end(void);

/* Malloc functions (in malloc.c). */
extern void *cthread_malloc(vm_size_t);
extern void cthread_free(void *);
extern void *cthread_realloc(void *, vm_size_t);
extern void cthread_malloc_fork_prepare(void);
extern void cthread_malloc_fork_parent(void);
extern void cthread_malloc_fork_child(void);
extern void cthread_malloc_init(void);

#endif	/*_CTHREADS_*/
