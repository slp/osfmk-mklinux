/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_PPC_ELF_H
#define _ASM_OSFMACH3_PPC_ELF_H

/* Dump registers in the same order as specified in PT_ macros */
/* floating point isn't dumped here... */

#define ELF_CORE_COPY_REGS(pr_reg, regs)				    \
	do {								    \
		memcpy((char*)(&pr_reg[PT_R0]),				    \
		       (char*)(&regs->gpr[0]),				    \
			32*sizeof(long));				    \
		pr_reg[PT_NIP] = regs->nip;				    \
		pr_reg[PT_MSR] = regs->msr;				    \
		pr_reg[PT_CCR] = regs->ccr;				    \
		pr_reg[PT_XER] = regs->xer;				    \
		pr_reg[PT_LNK] = regs->link;				    \
		pr_reg[PT_CTR] = regs->ctr;				    \
		pr_reg[PT_MQ]  = regs->mq;				    \
		pr_reg[PT_TRAP] =					    \
			((struct pt_regs *)regs->pt_extra_regs)->trap;	    \
		pr_reg[PT_ORIG_R3] =					    \
			((struct pt_regs *)regs->pt_extra_regs)->orig_gpr3; \
		pr_reg[PT_DAR] =					    \
			((struct pt_regs *)regs->pt_extra_regs)->dar;	    \
		pr_reg[PT_DSISR] =					    \
			((struct pt_regs *)regs->pt_extra_regs)->dsisr;	    \
		pr_reg[PT_RESULT] =					    \
			((struct pt_regs *)regs->pt_extra_regs)->result;    \
	} while (0);

/* Recover registers from such a dump */

#define ELF_CORE_RECOVER_REGS(pr_reg, regs)				\
	do {								\
		memcpy((char*)(&regs->gpr[0]),				\
		       (char*)(&pr_reg[PT_R0]),				\
			32*sizeof(long));				\
		regs->nip = pr_reg[PT_NIP];				\
		regs->msr = pr_reg[PT_MSR];				\
		regs->ccr = pr_reg[PT_CCR];				\
		regs->xer = pr_reg[PT_XER];				\
		regs->link = pr_reg[PT_LNK];				\
		regs->ctr = pr_reg[PT_CTR];				\
		regs->mq = pr_reg[PT_MQ] ;				\
		((struct pt_regs *)regs->pt_extra_regs)->trap =		\
			pr_reg[PT_TRAP];				\
		((struct pt_regs *)regs->pt_extra_regs)->orig_gpr3 =	\
			pr_reg[PT_ORIG_R3];				\
		((struct pt_regs *)regs->pt_extra_regs)->dar =		\
			pr_reg[PT_DAR];					\
		((struct pt_regs *)regs->pt_extra_regs)->dsisr =	\
			pr_reg[PT_DSISR];				\
		((struct pt_regs *)regs->pt_extra_regs)->result =	\
			pr_reg[PT_RESULT];				\
	} while (0);

#include <asm-ppc/elf.h>

#endif
