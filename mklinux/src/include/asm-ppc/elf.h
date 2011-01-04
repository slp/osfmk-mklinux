#ifndef __PPC_ELF_H
#define __PPC_ELF_H

/*
 * ELF register definitions..
 */

#define ELF_NGREG	48	/* includes nip, msr, lr, etc. */
#define ELF_NFPREG	33	/* includes fpscr */

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) ((x) == EM_PPC)

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_ARCH	EM_PPC
#define ELF_CLASS	ELFCLASS32
#define ELF_DATA	ELFDATA2MSB;

#define USE_ELF_CORE_DUMP
#define ELF_EXEC_PAGESIZE	4096

typedef unsigned long elf_greg_t;
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

typedef double elf_fpreg_t;
typedef elf_fpreg_t elf_fpregset_t[ELF_NFPREG];

/* Dump registers in the same order as specified in PT_ macros */
/* floating point isn't dumped here... */

#ifndef ELF_CORE_COPY_REGS
#define ELF_CORE_COPY_REGS(pr_reg, regs)		\
	do {						\
		memcpy((char*)(&pr_reg[PT_R0]),		\
		       (char*)(&regs->gpr[0]),		\
			32*sizeof(long));		\
		pr_reg[PT_NIP] = regs->nip;		\
		pr_reg[PT_MSR] = regs->msr;		\
		pr_reg[PT_CCR] = regs->ccr;		\
		pr_reg[PT_XER] = regs->xer;		\
		pr_reg[PT_LNK] = regs->link;		\
		pr_reg[PT_CTR] = regs->ctr;		\
		pr_reg[PT_MQ]  = regs->mq;		\
		pr_reg[PT_ORIG_R3] = regs->orig_gpr3;	\
		pr_reg[PT_TRAP] = regs->trap;		\
		pr_reg[PT_DAR] = regs->dar;		\
		pr_reg[PT_DSISR] = regs->dsisr;		\
		pr_reg[PT_RESULT] = regs->result;	\
	} while (0);
#endif	/* ELF_CORE_COPY_REGS */

/* Recover registers from such a dump */

#ifndef	ELF_CORE_RECOVER_REGS
#define ELF_CORE_RECOVER_REGS(pr_reg, regs)		\
	do {						\
		memcpy((char*)(&regs->gpr[0]),		\
		       (char*)(&pr_reg[PT_R0]),		\
			32*sizeof(long));		\
		regs->nip = pr_reg[PT_NIP];		\
		regs->msr = pr_reg[PT_MSR];		\
		regs->ccr = pr_reg[PT_CCR];		\
		regs->xer = pr_reg[PT_XER];		\
		regs->link = pr_reg[PT_LNK];		\
		regs->ctr = pr_reg[PT_CTR];		\
		regs->mq = pr_reg[PT_MQ] ;		\
		regs->orig_gpr3 = pr_reg[PT_ORIG_R3];	\
		regs->trap = pr_reg[PT_TRAP];		\
		regs->dar = pr_reg[PT_DAR];		\
		regs->dsisr = pr_reg[PT_DSISR];		\
		regs->result = pr_reg[PT_RESULT];	\
	} while (0);

#endif /* ELF_CORE_RECOVER_REGS */

#endif
