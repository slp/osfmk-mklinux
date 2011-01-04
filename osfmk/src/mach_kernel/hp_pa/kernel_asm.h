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

#ifndef _HP_PA_KERNEL_ASM_H_
#define _HP_PA_KERNEL_ASM_H_ 1

#include <machine/asm.h>

/* Minimal frame size for procedures that call other procedures. */
#define KF_SIZE roundup(FM_SIZE + KS_SIZE + ARG_SIZE, FR_ROUND)

#define comma(a,b) a,b	/* Needed to pass arguments with commas to macros. */

/* Check for ASTs and deliver them if there any.  Equivalent to:
     if (need_ast)
	ast_taken(FALSE, AST_ALL, splsched());
   A call frame must have been established.  Can destroy caller-saves
   registers, except ret0 if ret0save and ret0restore are defined to
   save and restore it.  */
#define CHECK_AST(label, ret0save, ret0restore) \
	.import ast_taken		! \
	ldil	L%need_ast,t1           ! \
	ldw	R%need_ast(t1),t1       ! \
	combt,=,n r0,t1,label		! \
	ret0save			! \
	ldi	0,arg0  		! \
	ldi 	-1,arg1  		! \
	rsm	PSW_I,r0		! \
	mfctl	eiem,arg2		! \
	ldi	SPLHIGH,t2		! \
	.call  				! \
	bl	ast_taken,rp		! \
	mtctl	t2,eiem			! \
	ret0restore

#endif	/* _HP_PA_KERNEL_ASM_H_ */
