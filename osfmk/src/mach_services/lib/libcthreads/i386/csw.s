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

#include <i386/asm.h>

/*
 * Cthread Context Functions
 *
 * Overall Description
 *    This collection of function is used to save and restore a cthread's user
 *    and internal state.  This is done by constructing a context on the
 *    current stack frame.  The created context has the following form:
 *
 *	|-----------------|  \
 *	|    saved ebx    |   \
 *	|-----------------|    \
 *	|    saved esi    |     \
 *	|-----------------|      > User context
 *	|    saved edi    |     /
 *	|-----------------|    /
 *	|    saved ebp    |   /
 *	|-----------------|  <
 *	| return function |   \
 *	|-----------------|    \ Internal context
 *	| return argument |    /
 *	|-----------------|   /		<---- Context pointer
 *
 *    We save the user context when we are about to do a potentially blocking
 *    cthread operation.  We save the internal context when we actually block.
 *    When we restart a cthread, we "return" to the internal context, which
 *    will then return to the saved used context.
 *
 * Functions
 *    USER_BUILD - Build the user context.
 *    INTERNAL_BUILD - Build the internal context and switch to
 *	  a different internal context.
 *    USER_INVOKE - Invoke the saved user context.
 *    PREPARE - Build the internal context, but don't switch to
 *	  a different internal context.
 *    GET_STATE - Return the current context state.
 *    SET_STATE - Set the current context state.
 *    INVOKE_SELF - Added to avoid indefinite recursion in mutex_lock_real
 *		    with wired threads.
 */

#ifdef GPROF
#undef GPROF
#define GFUDGE 8
#define FRAMEP 12
#define RETURN 8
#else /* GPROF */
#define GFUDGE 0
#endif /* GPROF */

#define B_ARG4	24(%ebp)
#define B_ARG5	28(%ebp)
#define ROUTINE 4
#define ARGUMENT 0


/* This value must be updated if adding extra words to the context.  */
#define CONTEXT_SIZE 16
	.globl EXT(cthread_context_size)
	.align 2
LEXT(cthread_context_size)
	.long CONTEXT_SIZE

filter_switch:
	.long 0f	
	.long 1f	
	.long 2f	
	.long 3f	
	.long 4f	
	.long 5f	
	.long 6f	
	.long 7f	


/*
 * cthread_filter(con, type, a1, a2, a3, a4)
 */

ENTRY(cthread_filter)
	movl	8(%esp),%ecx	/* move type to ecx */

	jmp	*filter_switch(,%ecx,4)

/* USER_BUILD 
 *
 * Arguments:
 *	context - Pointer to the cthread's context pointer.
 *	type	- USE_BUILD
 *	func    - The real function to invoke.
 *	arg1    - The first argument to the real function.
 *	arg2    - The second argument to the real function.
 */

#if GFUDGE > 0
0:	movl	(%esp),%eax	
	pushl	%ebx		
#else
0:	pushl	%ebx		
#endif
	pushl	%esi
	pushl	%edi
	pushl	%ebp
#if GFUDGE > 0
	pushl	%ebp
	pushl	%eax
#endif
	movl	20+GFUDGE(%esp),%ecx	/* get address of the cthread context */
	subl	$8,%esp			/* reserve space for the internal continuation */
	movl	%esp,(%ecx)		/* store the continuation address */
	movl	44+GFUDGE(%esp),%ecx	/* get the 2nd argument to the real function */
	pushl	%ecx			/* push it on the stack */
	movl	44+GFUDGE(%esp),%ecx	/* get the 1st argument to the real function */
	pushl	%ecx                    /* push it on the stack */
#if GFUDGE > 0
	pushl	%eax		/* Pretend return address */
	jmp	*48+GFUDGE(%esp)
#else
	call	*44(%esp)
#endif


/* INTERNAL_BUILD 
 * Arguments:
 *	context - Pointer to the cthread's context pointer.
 *	type	- INTERNAL_BUILD
 *	func    - The internal function to invoke when this context is restarted.
 *	arg1	- The argument to the internal function.
 *	lock	- The lock to unlock before we invoke the new context
 *	new_context - The new context to invoke.
 */

1:	movl	4(%esp),%ebx	/* Get the context pointer */
	movl	(%ebx),%ebx	/* Get the context */
	movl	16(%esp),%eax	/* Get the continuation function argument */
	movl	%eax,ARGUMENT(%ebx) /* Put it in the continuation */
	movl	12(%esp),%eax	/* Get the continuation function */
	movl	%eax,ROUTINE(%ebx)  /* Put it in the continuation */
#if GFUDGE > 0
	movl	%ebp,FRAMEP(%ebx)
#endif
	movl	20(%esp),%edx	/* Get the lock address */
	movl	24(%esp),%ecx	/* Get the new context's continuation */
	movl	(%ecx),%esp     /* Change the stack pointer to the new context */
	xorl	%eax,%eax       /* Zero the register */
	xchg	%eax,(%edx)     /* Unlock the lock */
#if GFUDGE > 0
	movl	FRAMEP(%esp),%ebp
#endif
	movl	ARGUMENT(%esp),%esi /* Get the argument */
	pushl	%esi		    /* Push the argument */
#if GFUDGE > 0
	movl	RETURN+4(%esp),%eax
	pushl	%eax		    /* Pretend return address  */
	jmp	*ROUTINE+8(%esp)
#else
	call	*ROUTINE+4(%esp)    /* Invoke the new context's function */
#endif


/* INTERNAL_INVOKE - not used */
2:	ret


/* USER_INVOKE 
 *
 * Arguments:
 *	context - Pointer to the cthread's context pointer.
 *	type	- USER_INVOKE
 *	result  - The result code to return to the invoked user context.
 */

3:	movl	4(%esp),%ecx	/* Get the context pointer */
	movl	8(%esp),%eax	/* Get the return result */
	movl	(%ecx),%esp	/* Switch the stack to the user context */
	addl	$8,%esp		/* Pop the internal continuation */
#if GFUDGE > 0
	popl	%edi		/* Not relevant  */
	popl	%edi
#endif
	popl	%ebp		/* Pop the user registers... */
	popl	%edi
	popl	%esi
	popl	%ebx
	ret			/* Return to the user */

/* PREPARE 
 *
 * Arguments:
 *	context - Pointer to the cthread's context pointer.
 *	type	- PREPARE
 *	func    - The internal cthread function to invoke when resumed.
 *	arg1    - The argument to the internal function
 */

4:	pushl	%ebp		/* Save the ebp register */
	movl	%esp, %ebp	/* Store the stack into ebp */

	movl	B_ARG0,%ecx	/* Get the context pointer */
	movl	B_ARG2,%edx	/* Get the continuation function */
	movl	B_ARG3,%eax	/* Get the continuation argument */
	movl	(%ecx),%ecx	/* Get the context */
#if GFUDGE > 0
	movl	$0,RETURN(%ecx)
	movl	$0,FRAMEP(%ecx)
#endif
	movl	%edx,ROUTINE(%ecx)  /* Store the continuation function */
	movl	%eax,ARGUMENT(%ecx) /* Store the continuation argument */
	leave			    /* Return to the caller... */
	ret

/* GET_STATE 
 *
 * Arguments:
 *	context - Pointer to the cthread's context pointer.
 *	type - GET_STATE
 *	thread_state - Pointer to the thread state to get.
 *	ret_context  - Pointer to the return context.
 */
5:	
/*
 * This whole get_state/set_state thing on blocked threads is REAL
 * fuzzy.  Do we care beyond eip, ebp, and esp?  For now deal with
 * only these.  It might make sense to save efl across internal
 * context switching to allow single stepping to be specified
 * for blocked threads.  
 */

	movl	B_ARG0,%ecx	/* Get the context pointer */
	movl	(%ecx),%ecx	
	movl	B_ARG2,%edx     /* Save the state pointer */
	movl	B_ARG5,%eax	/* Save the return context pointer */
	pushl	%ebx		/* save ebx */
	movl	ROUTINE(%ecx),%ebx  /* Get the continuation function */
	movl	%ebx,48(%edx)   
	movl	%eax,60(%edx)	/* Set the return context */
	addl	$20,%eax	/* Set the state's continuation function */
	movl	%eax,24(%edx)	/* point ebp to saved ebp  */
	popl	%ebx		/* Restore ebx */
	leave
	ret

/* SET_STATE 
 *
 * Arguments:
 *	context - Pointer to the cthread's context pointer.
 *	type - SET_STATE
 *	thread_state - Pointer to the thread state to get.
 *	ret_context  - Pointer to the return context.
 */
6:	movl	B_ARG0,%ecx	/* Get context pointer */
	movl	(%ecx),%ecx	/* Get the context */
	movl	B_ARG2,%edx	/* Get the state to set */
	movl	48(%edx),%eax	/* Set the state */
	movl	%eax, ROUTINE(%ecx)  /* eip will be the continuation  */

/*
 * I don't see any way to do other registers here that makes any sense
 */
	leave
	ret

/* INVOKE_SELF */
7:	movl	16(%esp),%esi	/* ARG3 (argument)  */
	movl	12(%esp),%ebx	/* ARG2 (routine)  */
	movl	4(%esp),%eax	/* ARG0 (&context)  */
	movl	(%eax),%esp
	pushl	%esi
#if GFUDGE > 0
	pushl	$0		/* Pretend return address */
	jmp	*%ebx
#else
	call	*%ebx
#endif

