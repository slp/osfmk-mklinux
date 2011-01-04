/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_PPC_PTRACE_H
#define _ASM_OSFMACH3_PPC_PTRACE_H

#ifdef	__KERNEL__
#define KERNEL
#endif

#include <asm-ppc/ptrace.h>

#define pt_extra_regs	_pad

#ifdef	PPC_DEBUG
#define INIT_PTREGS_DEBUG \
	{ 0.0, 0.0, 0.0, 0.0 },			/* fpr[4] */ \
	0.0,					/* fpcsr */ \
	0, 0,					/* hash1, hash2 */ \
	0, 0,					/* imiss, dmiss */ \
	0, 0,					/* icmp, dcmp */ \
	0,					/* marker */ \
	0,					/* edx */
#else	/* PPC_DEBUG */
#define INIT_PTREGS_DEBUG
#endif	/* PPC_DEBUG */

#define INIT_PTREGS { \
	0,					/* nip */ \
	0,					/* msr */ \
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	  0 , 0 },				/* gpr[32] */ \
	0,					/* ccr */ \
	0,					/* xer */ \
	0,					/* link */ \
	0,					/* ctr */ \
	0,					/* mq */ \
	0,					/* _pad */ \
	/* start of "extra registers" */ \
	0,					/* dar */ \
	0,					/* dsisr */ \
	0,					/* orig_gpr3 */ \
	0,					/* result */ \
	0,					/* trap */ \
	INIT_PTREGS_DEBUG \
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	  0, 0, 0, 0, 0, 0 }			/* _underhead[19] */ \
}

#endif	/* _ASM_OSFMACH3_PPC_PTRACE_H */
