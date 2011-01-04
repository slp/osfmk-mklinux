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
 * 	File: 	rthread.h
 *	Author: Eric Cooper, Carnegie Mellon University
 *	Date:	Jul, 1987
 *
 * 	Definitions for the Real-Time Threads package.
 *
 */

#ifndef	_RTHREADS_H_
#define	_RTHREADS_H_

#define RTHREADS 1

#include <machine/rthreads.h>
#include <mach/port.h>
#include <mach/message.h>
#include <mach/machine/vm_types.h>
#include <mach/std_types.h>
#include <mach/policy.h>
#include <mach/thread_switch.h>

typedef void *(*rthread_fn_t)(void *arg);

#define RTHREAD_NOFUNC ((rthread_fn_t)(-1))

#define RTHREAD_NO_PRIORITY		-1
#define RTHREAD_NO_POLICY		-1
#define RTHREAD_NO_QUANTUM		-1
#define RTHREAD_INHERIT_PRIORITY	-1

#define RSWITCH_OPTION_NONE		SWITCH_OPTION_NONE
#define RSWITCH_OPTION_WAIT		SWITCH_OPTION_WAIT

/*
 * Real-Time Threads package initialization.
 */

extern int rthread_init(void);

/*
 * Threads.
 */

typedef struct rthread *rthread_t;
typedef void *any_t;

#define	RTHREAD_NULL	((rthread_t) 0)

extern rthread_t	rthread_spawn(rthread_fn_t, void *);
extern rthread_t	rthread_spawn_priority(rthread_fn_t, void *, int);
extern rthread_t	rthread_activation_create(mach_port_t);
extern void 		rthread_fork_prepare(void);
extern void 		rthread_fork_child(void);
extern void 		rthread_fork_parent(void);
extern void		rthread_exit(int);
extern void		rthread_terminate(rthread_t);
extern int		rthread_wait(void);
extern rthread_t	rthread_ptr(int sp);

extern kern_return_t	rthread_set_default_policy(policy_t);
extern policy_t		rthread_get_default_policy(void);
extern kern_return_t	rthread_set_policy(rthread_t, policy_t);
extern policy_t		rthread_get_policy(rthread_t);
extern kern_return_t 	rthread_set_priority(rthread_t, int);
extern int		rthread_get_priority(rthread_t);
extern void		rthread_set_default_quantum(int);
extern int		rthread_get_default_quantum(void);
extern int		rthread_get_max_priority(void);

#ifndef	rthread_sp
extern int rthread_sp(void);
#endif

extern int	 	rthread_stack_mask;
extern vm_size_t 	rthread_stack_size;
extern vm_size_t 	rthread_wait_stack_size;

extern rthread_t 	rthread_self(void);
extern vm_offset_t	rthread_stack_base(rthread_t, int);
extern void		rthread_set_name(rthread_t, const char *);
extern const char	*rthread_get_name(rthread_t);
extern int 		rthread_count(void);
extern int		rthread_activation_count(void);
extern mach_port_t	rthread_kernel_port(rthread_t);
extern void 		rthread_set_data(rthread_t, void *);
extern void 		*rthread_get_data(rthread_t);

extern kern_return_t 	rthread_abort(rthread_t);
extern kern_return_t 	rthread_resume(rthread_t);
extern kern_return_t 	rthread_suspend(rthread_t);
extern kern_return_t	rthread_switch(rthread_t, int, int);
extern kern_return_t	rthread_trace(rthread_t, boolean_t);

#define rthread_yield()	rthread_switch(RTHREAD_NULL, RSWITCH_OPTION_NONE, 0)

#endif	/*_RTHREADS_H_*/
