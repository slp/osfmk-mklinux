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
 * File: i386_lock.s
 */
	
#include <i386/asm.h>

/* spin_tries is the number of times we will try to get the lock before
   yielding the processor with thread_switch().  On a uniprocessor, we
   might as well yield immediately because we're not going to get the
   lock until the thread that holds it is scheduled.  But on a
   multiprocessor there's a fair chance that if we spin a little we'll
   get the lock without incurring the overhead of a kernel call.  So
   pthread_init() will set _spin_tries to >1 in that case.  */
DATA(_spin_tries)
	.long	1
ENDDATA(_spin_tries)

/*
 * void _spin_lock(void *m)
 */
ENTRY(_spin_lock)
.Lrespin:
	movl	4(%esp),%ecx		/ point at mutex
	movl	$1,%eax			/ set locked value in acc
	xchg	%eax,(%ecx)		/ swap with mutex
					/ xchg with memory is automatically
					/ locked
	testl	%eax,%eax		/ was it locked?
	jne	.Llocked
.Lret:	ret
.Llocked:
	movl	EXT(_spin_tries),%edx
.Ltry:	movl	$1,%eax
	xchg	%eax,(%ecx)
	testl	%eax,%eax
	je	.Lret
	decl	%edx
	jne	.Ltry
.Lswitch:
	pushl	%edx			/ timeout = 0
	pushl	%edx			/ option = 0
	pushl	%edx			/ new_thread = 0
	call	thread_switch
	addl	$12,%esp		/ pop off parameters
	jmp	.Lrespin

/*
 * void _spin_unlock(void *m)
 */
ENTRY(_spin_unlock)
	movl	4(%esp),%ecx		/ point at mutex
	xorl	%eax,%eax		/ set unlocked value in acc
	xchg	%eax,(%ecx)		/ swap with mutex
					/ xchg with memory is automatically
					/ locked
	ret
