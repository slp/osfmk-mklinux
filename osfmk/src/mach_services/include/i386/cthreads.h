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

extern __inline__ void
spin_unlock(spin_lock_t *p)
{
	register int _u__ ;

	__asm__ volatile("xorl %0, %0; xchgl %0, %1"
			  : "=&r" (_u__), "=m" (*p) );
}

extern __inline__ int
spin_try_lock(spin_lock_t *p)
{
	boolean_t _r__;

	__asm__ volatile("movl $1, %0; xchgl %0, %1"
			: "=&r" (_r__), "=m" (*p) );
	return(!_r__);
}

extern __inline__ void *
cthread_sp(void)
{
	void	*_sp__;

	__asm__ ("movl %%esp, %0" :	"=r" (_sp__) );
	return(_sp__);
}

#endif	/* __GNUC__ */

#define STATE_FLAVOR i386_THREAD_STATE
#define STATE_COUNT i386_THREAD_STATE_COUNT
typedef struct i386_thread_state thread_state;
#define STATE_STACK(state) ((state)->uesp)

typedef vm_offset_t cthread_context_t;

#endif /* _MACHINE_CTHREADS_H_ */
