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
/* CMU_HIST */
/*
 * Revision 2.5.3.3  92/06/24  17:58:17  jeffreyh
 * 	Fixed MCOUNT macro for __STDC__ [condict]
 * 
 * Revision 2.5.3.2  92/03/28  10:05:11  jeffreyh
 * 	Changes from MK71
 * 	[92/03/20  12:11:56  jeffreyh]
 * 
 * Revision 2.7  92/02/29  15:33:41  rpd
 * 	Added ENTRY2.
 * 	[92/02/28            rpd]
 * 
 * Revision 2.6  92/02/19  15:07:52  elf
 * 	Changed #if __STDC__ to #ifdef __STDC__
 * 	[92/01/16            jvh]
 * 
 * Revision 2.5  91/05/14  16:02:45  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:10:42  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:30:29  mrt]
 * 
 * Revision 2.3  90/12/20  16:35:27  jeffreyh
 * 	changes for __STDC__
 * 	[90/12/06            jeffreyh]
 * 
 * Revision 2.2  90/05/03  15:24:12  dbg
 * 	First checkin.
 * 
 *
 * 	Typo on ENTRY if gprof
 * 	[90/03/29            rvb]
 * 
 * 	fix SVC for "ifdef wheeze" [kupfer]
 * 	Fix the GPROF definitions.
 * 	ENTRY(x) gets profiled iffdef GPROF.
 * 	Entry(x) (and DATA(x)) is NEVER profiled.
 * 	MCOUNT can be used by asm that intends to build a frame,
 * 	after the frame is built.
 *	[90/02/26            rvb]
 *
 * 	Add #define addr16 .byte 0x67
 * 	[90/02/09            rvb]
 * 	Added LBi, SVC and ENTRY
 * 	[89/11/10  09:51:33  rvb]
 * 
 * 	New a.out and coff compatible .s files.
 * 	[89/10/16            rvb]
 */
/* CMU_ENDHIST */
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

#ifndef	_I386_ASM_H_
#define	_I386_ASM_H_

#ifdef _KERNEL
#include <gprof.h>
#endif	/* _KERNEL */

#ifdef MACH_KERNEL
#include <mach_kdb.h>
#else	/* !MACH_KERNEL */
#define	MACH_KDB 0
#endif	/* !MACH_KERNEL */


#if	defined(MACH_KERNEL) || defined(_KERNEL)
#include <gprof.h>
#endif	/* MACH_KERNEL || _KERNEL */


#define S_ARG0	 4(%esp)
#define S_ARG1	 8(%esp)
#define S_ARG2	12(%esp)
#define S_ARG3	16(%esp)

#define FRAME	pushl %ebp; movl %esp, %ebp
#define EMARF	leave

#define B_ARG0	 8(%ebp)
#define B_ARG1	12(%ebp)
#define B_ARG2	16(%ebp)
#define B_ARG3	20(%ebp)

/* There is another definition of ALIGN for .c sources */
#ifdef ASSEMBLER
#define ALIGN 2
#endif /* ASSEMBLER */

#ifndef FALIGN
#define FALIGN ALIGN
#endif

#define LB(x,n) n
#if	__STDC__
#ifndef __NO_UNDERSCORES__
#define	LCL(x)	L ## x
#define EXT(x) _ ## x
#define LEXT(x) _ ## x ## :
#else
#define	LCL(x)	.L ## x
#define EXT(x) x
#define LEXT(x) x ## :
#endif
#define LBc(x,n) n ## :
#define LBb(x,n) n ## b
#define LBf(x,n) n ## f
#else /* __STDC__ */
#ifndef __NO_UNDERSCORES__
#define LCL(x) L/**/x
#define EXT(x) _/**/x
#define LEXT(x) _/**/x/**/:
#else /* __NO_UNDERSCORES__ */
#define	LCL(x)	.L/**/x
#define EXT(x) x
#define LEXT(x) x/**/:
#endif /* __NO_UNDERSCORES__ */
#define LBc(x,n) n/**/:
#define LBb(x,n) n/**/b
#define LBf(x,n) n/**/f
#endif /* __STDC__ */

#define SVC .byte 0x9a; .long 0; .word 0x7

#define RPC_SVC .byte 0x9a; .long 0; .word 0xf

#define String	.asciz
#define Value	.word
#define Times(a,b) (a*b)
#define Divide(a,b) (a/b)

#define INB	inb	%dx, %al
#define OUTB	outb	%al, %dx
#define INL	inl	%dx, %eax
#define OUTL	outl	%eax, %dx

#define data16	.byte 0x66
#define addr16	.byte 0x67

#if !GPROF
#define MCOUNT

#elif defined(__SHARED__)
#define MCOUNT		; .data;\
			.align ALIGN;\
			LBc(x, 8) .long 0;\
			.text;\
			Gpush;\
			Gload;\
			leal Gotoff(LBb(x,8)),%edx;\
			Egaddr(%eax,_mcount_ptr);\
			Gpop;\
			call *(%eax);

#else	/* !GPROF, !__SHARED__ */
#define MCOUNT		; .data;\
			.align ALIGN;\
			LBc(x, 8) .long 0;\
			.text;\
			movl $LBb(x,8),%edx;\
			call *EXT(_mcount_ptr);

#endif /* GPROF */

#ifdef __ELF__
#define ELF_FUNC(x)	.type x,@function
#define ELF_DATA(x)	.type x,@object
#define ELF_SIZE(x,s)	.size x,s
#else
#define ELF_FUNC(x)
#define ELF_DATA(x)
#define ELF_SIZE(x,s)
#endif

#define	Entry(x)	.globl EXT(x); ELF_FUNC(EXT(x)); .align FALIGN; LEXT(x)
#define	ENTRY(x)	Entry(x) MCOUNT
#define	ENTRY2(x,y)	.globl EXT(x); .globl EXT(y); \
			ELF_FUNC(EXT(x)); ELF_FUNC(EXT(y)); \
			.align FALIGN; LEXT(x); LEXT(y) \
			MCOUNT
#if __STDC__
#define	ASENTRY(x) 	.globl x; .align FALIGN; x ## : ELF_FUNC(x) MCOUNT
#else
#define	ASENTRY(x) 	.globl x; .align FALIGN; x: ELF_FUNC(x) MCOUNT
#endif /* __STDC__ */

#define	DATA(x)		.globl EXT(x); ELF_DATA(EXT(x)); .align ALIGN; LEXT(x)

#define End(x)		ELF_SIZE(x,.-x)
#define END(x)		End(EXT(x))
#define ENDDATA(x)	END(x)
#define Enddata(x)	End(x)

/*
 * ELF shared library accessor macros.
 * Gpush saves the %ebx register used for the GOT address
 * Gpop pops %ebx if we need a GOT
 * Gload loads %ebx with the GOT address if shared libraries are used
 * Gcall calls an external function.
 * Gotoff allows you to reference local labels.
 * Gotoff2 allows you to reference local labels with an index reg.
 * Gotoff3 allows you to reference local labels with an index reg & size.
 * Gaddr loads up a register with an address of an external item.
 * Gstack is the number of bytes that Gpush pushes on the stack.
 *
 * Varients of the above with E or L prefixes do EXT(name) or LCL(name)
 * respectively.
 */

#ifndef __SHARED__
#define Gpush
#define Gpop
#define Gload
#define Gcall(func)		call func
#define Gotoff(lab)		lab
#define Gotoff2(l,r)		l(r)
#define Gotoff3(l,r,s)		l(,r,s)
#define Gaddr(to,lab)		movl $lab,to
#define Gcmp(lab,reg)		cmpl $lab,reg
#define Gmemload(lab,reg)	movl lab,reg
#define Gmemstore(reg,lab,tmp)	movl reg,lab
#define Gstack			0

#else
#ifdef __ELF__			/* ELF shared libraries */
#define Gpush			pushl %ebx
#define Gpop			popl %ebx
#define Gload			call 9f; 9: popl %ebx; addl $_GLOBAL_OFFSET_TABLE_+[.-9b],%ebx
#define Gcall(func)		call EXT(func)@PLT
#define Gotoff(lab)		lab@GOTOFF(%ebx)
#define Gotoff2(l,r)		l@GOTOFF(%ebx,r)
#define Gotoff3(l,r,s)		l@GOTOFF(%ebx,r,s)
#define Gaddr(to,lab)		movl lab@GOT(%ebx),to
#define Gcmp(lab,reg)		cmpl reg,lab@GOT(%ebx)
#define Gmemload(lab,reg)	movl lab@GOT(%ebx),reg; movl (reg),reg
#define Gmemstore(reg,lab,tmp)	movl lab@GOT(%ebx),tmp; movl reg,(tmp)
#define Gstack			4

#else				/* ROSE shared libraries */
#define Gpush
#define Gpop
#define Gload
#define Gcall(func)		call *9f; .data; .align ALIGN; 9: .long func; .text
#define Gotoff(lab)		lab
#define Gotoff2(l,r)		l(r)
#define Gotoff3(l,r,s)		l(,r,s)
#define Gaddr(to,lab)		movl 9f,to; .data; .align ALIGN; 9: .long lab; .text
#define Gcmp(lab,reg)		cmpl reg,9f; .data; .align ALIGN; 9: .long lab; .text
#define Gmemload(lab,reg)	movl 9f,reg; movl (reg),reg; .data; .align ALIGN; 9: .long lab; .text
#define Gmemstore(reg,lab,tmp)	movl 9f,tmp; movl reg,(tmp); .data; .align ALIGN; 9: .long lab; .text
#define Gstack			0
#endif	/* __ELF__ */
#endif	/* __SHARED__ */

/* Egotoff is not provided, since external symbols should not use @GOTOFF
   relocations.  */
#define Egcall(func)		Gcall(EXT(func))
#define Egaddr(to,lab)		Gaddr(to,EXT(lab))
#define Egcmp(lab,reg)		Gcmp(EXT(lab),reg)
#define Egmemload(lab,reg)	Gmemload(EXT(lab),reg)
#define Egmemstore(reg,lab,tmp)	Gmemstore(reg,EXT(lab),tmp)

#define Lgotoff(lab)		Gotoff(LCL(lab))
#define Lgotoff2(l,r)		Gotoff2(LCL(l),r)
#define Lgotoff3(l,r,s)		Gotoff3(LCL(l),r,s)
#define Lgcmp(lab,reg)		Gcmp(LCL(lab),reg)
#define Lgmemload(lab,reg)	movl Lgotoff(lab),reg
#define Lgmemstore(reg,lab,tmp)	movl reg,Lgotoff(lab)

#ifdef ASSEMBLER
#if	MACH_KDB
#include <ddb/stab.h>
/*
 * This pseudo-assembler line is added so that there will be at least
 *	one N_SO entry in the symbol stable to define the current file name.
 */
.stabs ,N_SO,0,0,Ltext0
Ltext0:
#endif	/* MACH_KDB */

#else /* NOT ASSEMBLER */

/* These defines are here for .c files that wish to reference global symbols
 * within __asm__ statements. 
 */
#ifndef __NO_UNDERSCORES__
#define CC_SYM_PREFIX "_"
#else
#define CC_SYM_PREFIX ""
#endif /* __NO_UNDERSCORES__ */
#endif /* ASSEMBLER */

#endif /* _I386_ASM_H_ */
