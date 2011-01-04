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

#ifndef	_PPC_ASM_H_
#define	_PPC_ASM_H_

#define __NO_UNDERSCORES__ 1

#ifdef ASSEMBLER
#define r0 0
#define r1 1
#define r2 2
#define r3 3
#define r4 4
#define r5 5
#define r6 6
#define r7 7
#define r8 8
#define r9 9
#define r10 10
#define r11 11
#define r12 12
#define r13 13
#define r14 14
#define r15 15
#define r16 16
#define r17 17
#define r18 18
#define r19 19
#define r20 20
#define r21 21
#define r22 22
#define r23 23
#define r24 24
#define r25 25
#define r26 26
#define r27 27
#define r28 28
#define r29 29
#define r30 30
#define r31 31

#define f0 0
#define f1 1
#define f2 2
#define f3 3
#define f4 4
#define f5 5
#define f6 6
#define f7 7
#define f8 8
#define f9 9
#define f10 10
#define f11 11
#define f12 12
#define f13 13
#define f14 14
#define f15 15
#define f16 16
#define f17 17
#define f18 18
#define f19 19
#define f20 20
#define f21 21
#define f22 22
#define f23 23
#define f24 24
#define f25 25
#define f26 26
#define f27 27
#define f28 28
#define f29 29
#define f30 30
#define f31 31

#define ARG0 r3
#define ARG1 r4
#define ARG2 r5
#define ARG3 r6
#define ARG4 r7
#define ARG5 r8
#define ARG6 r9
#define ARG7 r10

#define tmp0	0	/* Temporary GPR remapping (603e specific) */
#define tmp1	1
#define tmp2	2
#define tmp3	3

/* SPR registers */

#define dmiss	976		/* ea that missed */
#define dcmp	977		/* compare value for the va that missed */
#define hash1	978		/* pointer to first hash pteg */
#define	hash2	979		/* pointer to second hash pteg */
#define imiss	980		/* ea that missed */
#define icmp	981		/* compare value for the va that missed */
#define rpa	982		/* required physical address register */

#define iabr	1010		/* instruction address breakpoint register */
#define pir	1023		/* Processor ID Register */

/* MQ register on the 601 */
#define mq	0		/* spr number for mq register on 601 */

#define IBAT0U	528
#define IBAT0L	529
#define IBAT1U	530
#define IBAT1L	531
#define IBAT2U	532
#define IBAT2L	533
#define IBAT3U	534
#define IBAT3L	535
#define ibat0u	528
#define ibat0l	529
#define ibat1u	530
#define ibat1l	531
#define ibat2u	532
#define ibat2l	533
#define ibat3u	534
#define ibat3l	535

#define DBAT0U	536
#define DBAT0L	537
#define DBAT1U	538
#define DBAT1L	539
#define DBAT2U	540
#define DBAT2L	541
#define DBAT3U	542
#define DBAT3L	543
#define dbat0u	536
#define dbat0l	537
#define dbat1u	538
#define dbat1l	539
#define dbat2u	540
#define dbat2l	541
#define dbat3u	542
#define dbat3l	543

#define HID0	1008
#define hid0	1008
#define HID1	1009
#define hid1	1009
#define SDR1	25
#define sprg0	272
#define sprg1	273
#define sprg2	274
#define sprg3	275
#define ppcDAR	19
#define ppcdar	19
#define srr0	26
#define srr1	27

#define CR0 0
#define CR1 1
#define CR2 2
#define CR3 3
#define CR4 4
#define CR5 5
#define CR6 6
#define CR7 7

#define cr0 0
#define cr1 1
#define cr2 2
#define cr3 3
#define cr4 4
#define cr5 5
#define cr6 6
#define cr7 7

#define cr0_lt	0
#define cr0_gt	1
#define cr0_eq	2
#define cr0_so	3
#define cr0_un	3

#endif	/* ASSEMBLER */

/* Tags are placed before Immediately Following Code (IFC) for the debugger
 * to be able to deduce where to find various registers when backtracing
 * 
 * We only define the values as we use them, see SVR4 ABI PowerPc Supplement
 * for more details (defined in ELF spec).
 */

#define TAG_NO_FRAME_USED 0x00000000

/* (should use genassym to get these offsets) */

#define FM_BACKPTR 0
#define FM_LR_SAVE 4  /* gcc 2.7.1 is now following eabi spec correctly */
/* TODO NMGS FM_SIZE 8 is ok according to EABI specs, but gcc uses 16 */
#define FM_SIZE    16   /* minimum frame contents, backptr and LR save */
#define FM_ARG0	   8
/* redzone is the area under the stack pointer which must be preserved
 * when taking a trap, interrupt etc. This is no longer needed as gcc
 * (2.7.2 and above) now follows ELF spec correctly and never loads/stores
 * below the frame pointer
 */
#define FM_REDZONE 0				/* was ((32-14+1)*4) */

#define COPYIN_ARG0_OFFSET FM_ARG0

#ifdef	MACH_KERNEL
#include <mach_kdb.h>
#else	/* MACH_KERNEL */
#define MACH_KDB 0
#endif	/* MACH_KERNEL */

#define BREAKPOINT_TRAP twge	2,2

/* There is another definition of ALIGN for .c sources */
#ifndef __LANGUAGE_ASSEMBLY
#define ALIGN 4
#endif /* __LANGUAGE_ASSEMBLY */

#ifndef FALIGN
#define FALIGN 4 /* Align functions on words for now. Cachelines is better */
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

#define String	.asciz
#define Value	.word
#define Times(a,b) (a*b)
#define Divide(a,b) (a/b)

#define data16	.byte 0x66
#define addr16	.byte 0x67

#define lo16(x) (((x&0x0000FFFF)/0x8000)*0xFFFF0000)|x
#define hi16(x) (((x>>16)/0x8000)*0xFFFF0000)|(x>>16)

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

#define	Entry(x,tag)	.globl EXT(x); ELF_FUNC(EXT(x)); .long tag;.align FALIGN; LEXT(x)
#define	ENTRY(x,tag)	Entry(x,tag) MCOUNT
#define	ENTRY2(x,y,tag)	.globl EXT(x); .globl EXT(y); \
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

/* These defines are here for .c files that wish to reference global symbols
 * within __asm__ statements. 
 */
#ifndef __NO_UNDERSCORES__
#define CC_SYM_PREFIX "_"
#else
#define CC_SYM_PREFIX ""
#endif /* __NO_UNDERSCORES__ */

#endif /* _PPC_ASM_H_ */
