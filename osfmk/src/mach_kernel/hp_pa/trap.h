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
 *  (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: trap.c 1.49 94/12/14$
 */

#ifndef _HP_PA_TRAP_H_
#define _HP_PA_TRAP_H_

/*
 * Trap type values
 */

#define I_NONEXIST	0
#define I_HPMACH_CHK    1
#define I_POWER_FAIL    2
#define I_RECOV_CTR     3
#define I_EXT_INTP      4
#define I_LPMACH_CHK    5
#define I_IPGFT         6
#define I_IMEM_PROT     7
#define I_UNIMPL_INST   8
#define I_BRK_INST      9
#define I_PRIV_OP       10
#define I_PRIV_REG      11
#define I_OVFLO         12
#define I_COND          13
#define I_EXCEP         14
#define I_DPGFT         15
#define I_IPGFT_NA      16
#define I_DPGFT_NA      17
#define I_DMEM_PROT     18
#define I_DMEM_BREAK    19
#define I_TLB_DIRTY     20
#define I_PAGE_REF      21
#define I_EMULAT        22
#define I_HPRIV_XFR     23
#define I_LPRIV_XFR     24
#define I_TAKEN_BR      25
#define	I_DMEM_ACC	26	/* T-chip only - replaces trap 18 */
#define	I_DMEM_PID	27	/* T-chip only - replaces trap 18 */
#define	I_UNALIGN	28	/* T-chip only - replaces trap 18 */

/* 
 * Software generated traps
 */

#define I_IS_OVFL	29
#define I_KS_OVFL	30
#define I_IS_LOST	31

#define TRAP_NAMES \
	"Non-existent trap", \
	"High priority machine check", \
	"Power failure trap", \
	"Recover counter overflow trap", \
	"External interrupt", \
	"Low priority machine check", \
	"Instruction page fault", \
	"Instruction memory protection fault", \
	"Illegal instruction trap", \
	"Break instruction trap", \
	"Privileged operation trap", \
	"Privileged register trap", \
	"Overflow", \
	"Conditional trap", \
	"Assist exception trap", \
	"Data page fault", \
	"Non-access instruction TLB miss", \
	"Non-access data TLB miss", \
	"Data memory protection fault", \
	"Data memory break", \
	"TLB dirty", \
	"Page reference trap", \
	"Assist Emulation trap", \
	"Higher privilege transfer", \
	"Lower privilege transfer", \
	"Taken branch trap", \
	"Data memory access fault", \
	"Data memory protection ID fault", \
	"Data alignment fault", \
	"Interrupt stack overflow", \
	"Kernel stack overflow", \
        "Interrupt stack lost"

/*
 * various opcode recognition macros
 */

#define OPCODE(x)	(((unsigned) (x)) >> 26)

#define OP_LDB     	(0x10)	/* Load byte */
#define OP_LDH     	(0x11)	/* Load halfword */
#define OP_LDW     	(0x12)	/* Load word */
#define OP_LDWM    	(0x13)	/* Load word modify */
#define OP_STB     	(0x18)	/* Store byte */
#define OP_STH     	(0x19)	/* Store halfword */
#define OP_STW     	(0x1a)	/* Store word */
#define OP_STWM    	(0x1b)	/* Store word modify */
#define OP_MEM     	(0x1)	/* Memory management instructions */
#define OP_LDCW    	(0x3)	/* Both load and clear word instructions */
#define OP_LDSTX   	(0x3)	/* Index short load/store */
#define OP_COWD         (0x9)   /* Coproc word load/store */
#define OP_CODWD        (0xb)   /* Coproc doubleword load/store */

#define OPCODE_IS_STORE(opcode) \
	((((opcode) & 0xf0000000) == 0x60000000) || \
	 (((opcode) & 0xf4000200) == 0x24000200) || \
	 (((opcode) & 0xfc000200) == 0x0c000200))

#define OPCODE_IS_LOAD(opcode) \
	(((((opcode) & 0xf0000000) == 0x40000000) || \
	  (((opcode) & 0xf4000200) == 0x24000000) || \
	  (((opcode) & 0xfc000200) == 0x0c000000)) && \
	 (((opcode) & 0xfc001fc0) != 0x0c0011c0))


#define OPCODE_L_IMASK			(0x1fff)
#define OPCODE_S_IMASK         		(0xf)
#define OPCODE_M_UPDATE(opcode)     	((opcode >> 5) & 0x1)
#define OPCODE_U_UPDATE(opcode)     	((opcode >> 13) & 0x1)
#define OPCODE_A_UPDATE(opcode)     	((opcode >> 13) & 0x1)

#define OPCODE_LMEM_TARGET(opcode)  	((opcode >> 16) & 0x1f)
#define OPCODE_LMEM_SOURCE(opcode)  	((opcode >> 16) & 0x1f)
#define OPCODE_LMEM_BASE(opcode)    	((opcode >> 21) & 0x1f)
#define OPCODE_LMEM_IMM(opcode)     	(opcode & 0x3fff)

#define OPCODE_SLMEM_TARGET(opcode) 	(opcode & 0x1f)
#define OPCODE_SLMEM_BASE(opcode)   	((opcode >> 21) & 0x1f)
#define OPCODE_SLMEM_IMM(opcode)    	((opcode >> 16) & 0x1f)

#define OPCODE_SSMEM_SOURCE(opcode) 	((opcode >> 16) & 0x1f)
#define OPCODE_SSMEM_BASE(opcode)   	((opcode >> 21) & 0x1f)
#define OPCODE_SSMEM_IMM(opcode)    	(opcode & 0x1f)

#define OPCODE_IMEM_TARGET(opcode)  	(opcode & 0x1f)
#define OPCODE_IMEM_BASE(opcode)    	((opcode >> 21) & 0x1f)
#define OPCODE_IMEM_X(opcode)       	((opcode >> 16) & 0x1f)

#define OPCODE_CMEM_TARGET(opcode)      (opcode & 0x1f)
#define OPCODE_CMEM_SOURCE(opcode)      (opcode & 0x1f)
#define OPCODE_CMEM_BASE(opcode)        ((opcode >> 21) & 0x1f)
#define OPCODE_CMEM_X(opcode)           ((opcode >> 16) & 0x1f)

#define OPCODE_CSMEM_TARGET(opcode)     (opcode & 0x1f)
#define OPCODE_CSMEM_SOURCE(opcode)     (opcode & 0x1f)
#define OPCODE_CSMEM_BASE(opcode)       ((opcode >> 21) & 0x1f)
#define OPCODE_CSMEM_IMM(opcode)        ((opcode >> 16) & 0x1f)

/*
 * These masks pull out the releavant size bits for a particular
 * opcode family.
 */
#define SIZE_OP_LDSTX(opcode)		(((opcode) & 0xc0) >> 6)            
#define SIZE_OP_LDST(opcode)		(((opcode) & 0x0c000000) >> 26)
#define SIZE_OP_COPROC(opcode)		(((opcode) & 0x08000000) >> 27)

#define BIT_19(opcode)         		((opcode) & 0x1000)

/*
 * This mess rips the qualifier out of the load instructions (4 bits).
 */
#define OP_EXT4_TYPE(opcode)		(((opcode) >> 6) & 0xf)

/*
 * Create psuedo minor opcode for coprocessor memory operations.
 */
#define OP_EXT_COPROC(z) (((z >> 9) & 0x1) | ((z >> 11) & 0x2))

/*
 * Coprocessor UID
 */
#define OP_COP_UID(z) ((z >> 6) & 0x7)

/*
 * Ext4 bits for indexed load/short instructions
 */
#define OP_EXT4_LDBX  		(0x0)	/* Load byte indexed */
#define OP_EXT4_LDHX  		(0x1)	/* Load halfword indexed */
#define OP_EXT4_LDWX  		(0x2)	/* Load word indexed */
#define OP_EXT4_LDBS  		(0x0)	/* Load byte short */
#define OP_EXT4_LDHS  		(0x1)	/* Load halfword short */
#define OP_EXT4_LDWS  		(0x2)	/* Load word short */
#define OP_EXT4_LDWAX 		(0x6)	/* Load word absolute indexed */
#define OP_EXT4_LDWAS 		(0x6)	/* Load word absolute short */
#define OP_EXT4_LDCW  		(0x7)	/* General ldcw op */
#define OP_EXT4_LDCWX 		(0x7)	/* Load and clear word indexed */
#define OP_EXT4_LDCWS 		(0x7)	/* Load and clear word short */
#define OP_EXT4_STBS  		(0x8)	/* Store byte short */
#define OP_EXT4_STHS  		(0x9)	/* Store halfword short */
#define OP_EXT4_STWS  		(0xa)	/* Store word short */
#define OP_EXT4_STBYS 		(0xc)	/* Store bytes short */
#define OP_EXT4_STWAS 		(0xe)	/* Store word absolute short */

/*
 * Coprocessor psuedo load/store minor opcodes.
 */
#define POP_CLDWX     (0x0)         /* Coprocessor load word indexed */
#define POP_CLDDX     (0x0)         /* Coprocessor load doubleword indexed */
#define POP_CSTWX     (0x1)         /* Coprocessor store word indexed */
#define POP_CSTDX     (0x1)         /* Coprocessor store doubleword indexed */
#define POP_CLDWS     (0x2)         /* Coprocessor load word short */
#define POP_CLDDS     (0x2)         /* Coprocessor load doubleword short */
#define POP_CSTWS     (0x3)         /* Coprocessor store word short */
#define POP_CSTDS     (0x3)         /* Coprocessor store doubleword short */

/*
 * Coprocessor UIDs (just to map register files).
 */
#define COP_UID_FRL   (0x0)         /* Left side */
#define COP_UID_FRR   (0x1)         /* Right side */

#ifdef MACH_KERNEL
#ifndef ASSEMBLER

/*
 * prototypes
 */
extern void trap(int, struct hp700_saved_state *ssp);
extern spl_t hp_pa_astcheck(void);
extern void interrupt(int type, struct hp700_saved_state *ssp);

#ifdef HPOSFCOMPAT
extern boolean_t alignment_fault(struct hp700_saved_state *ssp);
extern int handle_alignment_fault(struct hp700_saved_state *ssp);
#endif

extern int syscall_error(int, int, int, struct hp700_saved_state *);
extern int procitab(unsigned, void (*)(int), int);
extern int aspitab(unsigned, unsigned, void (*)(int), int);
extern int waxitab(unsigned, unsigned, void (*)(int), int);

extern void aspintsvc(int);
extern void waxintsvc(int);

extern void hardclock(int);
extern void powerfail(int);
extern void viperintr      (int);
extern int rtclock_intr   (void);
extern void aspintr        (int);
extern void waxintr        (int);
extern void pc586intr      (int);
extern void hilintr        (int);
extern void dcaintr        (int);
extern void int_illegal    (int);
extern void ignore_int     (int);

#endif /* ASSEMBLER */
#endif /* MACH_KERNEL */
#endif /* _HP_PA_TRAP_H_ */
