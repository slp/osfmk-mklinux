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
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

#ifndef _MACHINE_CTHREADS_H_
#define _MACHINE_CTHREADS_H_

typedef volatile int spin_lock_t;
#define SPIN_LOCK_INITIALIZER	0
#define spin_lock_init(s)	(*(s) = 0)
#define spin_lock_locked(s)	(*(s) != 0)
#define CTHREAD_STACK_OFFSET 128
#define CTHREAD_WAITER_SPIN_COUNT 500
#define MUTEX_SPIN_COUNT 200
#define LOCK_SPIN_COUNT 200

extern void spin_unlock(spin_lock_t *);
extern int spin_try_lock(spin_lock_t *);
extern void *cthread_sp(void);

#if	__GNUC__ && !_NO_INLINE_SPINLOCKS && !defined(lint)

/* We don't inline spinlocks yet, TODO NMGS */

#endif	/* __GNUC__ */

#define STATE_FLAVOR MACHINE_THREAD_STATE
#define STATE_COUNT MACHINE_THREAD_STATE_COUNT
typedef struct ppc_thread_state thread_state;
#define STATE_STACK(state) ((state)->r1)

typedef vm_offset_t cthread_context_t;

#define cthread_filter(contextp, action, arg1, arg2, arg3, arg4) \
	CTHREAD_FILTER_CALL_ ## action(contextp, arg1, arg2, arg3, arg4)
/*
 * Unfortunately cpp expands the `action' to an integer, when we would
 * prefer to leave it as CTHREAD_FILTER_USER_BUILD or whatever.  This is
 * a consequence of going through more than one level of macros.  But
 * hardwiring the constants here is no worse than the hardwired numbers
 * in AT386/csw.s.
 *
 * The machine-independent CTHREAD_FILTER macro wants to force everything
 * into `void *', which is vexing because we have to cast some of it back,
 * and we lose typechecking.
 */

extern void *cthread_user_build(cthread_context_t *ctxp,
				void (*f)(int),
				void *a1,
				void *a2);
#define CTHREAD_FILTER_CALL_0(ctxp, arg1, arg2, arg3, arg4) \
	cthread_user_build(ctxp, (void (*)(int))arg1, arg2, arg3)

extern void cthread_internal_build(cthread_context_t ctx,
				   void (*f)(int),
				   void *a1,
				   spin_lock_t *lockp,
				   int newctx);
#define CTHREAD_FILTER_CALL_1(ctxp, arg1, arg2, arg3, arg4) \
	cthread_internal_build(*ctxp, (void (*)(int))arg1, arg2, arg3, \
			       *(int *)arg4)

extern void cthread_user_invoke(cthread_context_t ctx, void* result);
#define CTHREAD_FILTER_CALL_3(ctxp, arg1, arg2, arg3, arg4) \
	cthread_user_invoke(*ctxp, (void*)arg1)

extern void cthread_prepare(cthread_context_t ctx, void (*f)(int), void *a1);
#define CTHREAD_FILTER_CALL_4(ctxp, arg1, arg2, arg3, arg4) \
	cthread_prepare(*ctxp, (void (*)(int))arg1, arg2)

extern void cthread_invoke_self(cthread_context_t ctx,
				void (*f)(int),
				void *a1);
#define CTHREAD_FILTER_CALL_7(ctxp, arg1, arg2, arg3, arg4) \
	cthread_invoke_self(*ctxp, (void (*)(int))arg1, arg2)

#endif /* _MACHINE_CTHREADS_H_ */
