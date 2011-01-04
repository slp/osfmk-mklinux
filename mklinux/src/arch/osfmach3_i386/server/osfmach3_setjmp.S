/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#define	__LANGUAGE_ASSEMBLY
#define __NO_UNDERSCORES__
#define ASSEMBLER

#include <machine/asm.h>

	ENTRY(osfmach3_setjmp)
	movl	4(%esp),%ecx	/* fetch buffer */
	movl	%ebx,0(%ecx)
	movl	%esi,4(%ecx)
	movl	%edi,8(%ecx)
	movl	%ebp,12(%ecx)	/* save frame pointer of caller */
	popl	%edx
	movl	%esp,16(%ecx)	/* save stack pointer of caller */
	movl	%edx,20(%ecx)	/* save pc of caller */
	xorl	%eax,%eax
	jmp	*%edx

	ENTRY(osfmach3_longjmp)
	movl	8(%esp),%eax	/* return(v) */
	movl	4(%esp),%ecx	/* fetch buffer */
	movl	0(%ecx),%ebx
	movl	4(%ecx),%esi
	movl	8(%ecx),%edi
	movl	12(%ecx),%ebp
	movl	16(%ecx),%esp
	jmp	*20(%ecx)
