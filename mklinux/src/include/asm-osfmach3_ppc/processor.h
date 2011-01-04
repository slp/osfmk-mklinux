/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_PPC_PROCESSOR_H
#define _ASM_OSFMACH3_PPC_PROCESSOR_H

/* Bit encodings for Machine State Register (MSR) */
#define MSR_POW		(1<<18)		/* Enable Power Management */
#define MSR_TGPR	(1<<17)		/* TLB Update registers in use */
#define MSR_ILE		(1<<16)		/* Interrupt Little-Endian enable */
#define MSR_EE		(1<<15)		/* External Interrupt enable */
#define MSR_PR		(1<<14)		/* Supervisor/User privilege */
#define MSR_FP		(1<<13)		/* Floating Point enable */
#define MSR_ME		(1<<12)		/* Machine Check enable */
#define MSR_FE0		(1<<11)		/* Floating Exception mode 0 */
#define MSR_SE		(1<<10)		/* Single Step */
#define MSR_BE		(1<<9)		/* Branch Trace */
#define MSR_FE1		(1<<8)		/* Floating Exception mode 1 */
#define MSR_IP		(1<<6)		/* Exception prefix 0x000/0xFFF */
#define MSR_IR		(1<<5)		/* Instruction MMU enable */
#define MSR_DR		(1<<4)		/* Data MMU enable */
#define MSR_RI		(1<<1)		/* Recoverable Exception */
#define MSR_LE		(1<<0)		/* Little-Endian enable */

#define MSR_		MSR_FP|MSR_FE0|MSR_FE1|MSR_ME
#define MSR_USER	MSR_|MSR_PR|MSR_EE|MSR_IR|MSR_DR

/* Bit encodings for Hardware Implementation Register (HID0) */
#define HID0_EMCP	(1<<31)		/* Enable Machine Check pin */
#define HID0_EBA	(1<<29)		/* Enable Bus Address Parity */
#define HID0_EBD	(1<<28)		/* Enable Bus Data Parity */
#define HID0_SBCLK	(1<<27)
#define HID0_EICE	(1<<26)
#define HID0_ECLK	(1<<25)
#define HID0_PAR	(1<<24)
#define HID0_DOZE	(1<<23)
#define HID0_NAP	(1<<22)
#define HID0_SLEEP	(1<<21)
#define HID0_DPM	(1<<20)
#define HID0_ICE	(1<<15)		/* Instruction Cache Enable */
#define HID0_DCE	(1<<14)		/* Data Cache Enable */
#define HID0_ILOCK	(1<<13)		/* Instruction Cache Lock */
#define HID0_DLOCK	(1<<12)		/* Data Cache Lock */
#define HID0_ICFI	(1<<11)		/* Instruction Cache Flash Invalidate */
#define HID0_DCI	(1<<10)		/* Data Cache Invalidate */
 
#include <mach/machine/vm_param.h>

/*
 * User space process size: 2GB. This is hardcoded into a few places,
 * so don't change it unless you know what you are doing.
 *
 * "this is gonna have to change to 1gig for the sparc" - David S. Miller
 */
#define TASK_SIZE	(0x80000000UL)

static __inline__ void
osfmach3_state_to_ptregs(
	struct ppc_thread_state	*state,
	struct pt_regs		*regs,
	struct pt_regs		*extra_regs,
	unsigned long		orig_gpr3)
{
	if (regs != (struct pt_regs *) state) {
		regs->gpr[0] = state->r0;
		regs->gpr[1] = state->r1;
		regs->gpr[2] = state->r2;
		regs->gpr[3] = state->r3;
		regs->gpr[4] = state->r4;
		regs->gpr[5] = state->r5;
		regs->gpr[6] = state->r6;
		regs->gpr[7] = state->r7;
		regs->gpr[8] = state->r8;
		regs->gpr[9] = state->r9;
		regs->gpr[10] = state->r10;
		regs->gpr[11] = state->r11;
		regs->gpr[12] = state->r12;
		regs->gpr[13] = state->r13;
		regs->gpr[14] = state->r14;
		regs->gpr[15] = state->r15;
		regs->gpr[16] = state->r16;
		regs->gpr[17] = state->r17;
		regs->gpr[18] = state->r18;
		regs->gpr[19] = state->r19;
		regs->gpr[20] = state->r20;
		regs->gpr[21] = state->r21;
		regs->gpr[22] = state->r22;
		regs->gpr[23] = state->r23;
		regs->gpr[24] = state->r24;
		regs->gpr[25] = state->r25;
		regs->gpr[26] = state->r26;
		regs->gpr[27] = state->r27;
		regs->gpr[28] = state->r28;
		regs->gpr[29] = state->r29;
		regs->gpr[30] = state->r30;
		regs->gpr[31] = state->r31;
		regs->nip = state->srr0;
		regs->msr = state->srr1;
		regs->ctr = state->ctr;
		regs->link = state->lr;
		regs->ccr = state->cr;
		regs->xer = state->xer;
	}
	regs->pt_extra_regs = (unsigned long) extra_regs;
	if (extra_regs) {
		extra_regs->orig_gpr3 = orig_gpr3;
		extra_regs->result = 0;
#if 0
		extra_regs->result = ;
		extra_regs->trap = ;
		/* ... */
#endif
	}
}

static __inline__ void
osfmach3_ptregs_to_state(
	struct pt_regs		*regs,
	struct ppc_thread_state	*state,
	unsigned long		*orig_gpr3_ptr)
{
	if ((struct pt_regs *) state != regs) {
		state->r0 = regs->gpr[0];
		state->r1 = regs->gpr[1];
		state->r2 = regs->gpr[2];
		state->r3 = regs->gpr[3];
		state->r4 = regs->gpr[4];
		state->r5 = regs->gpr[5];
		state->r6 = regs->gpr[6];
		state->r7 = regs->gpr[7];
		state->r8 = regs->gpr[8];
		state->r9 = regs->gpr[9];
		state->r10 = regs->gpr[10];
		state->r11 = regs->gpr[11];
		state->r12 = regs->gpr[12];
		state->r13 = regs->gpr[13];
		state->r14 = regs->gpr[14];
		state->r15 = regs->gpr[15];
		state->r16 = regs->gpr[16];
		state->r17 = regs->gpr[17];
		state->r18 = regs->gpr[18];
		state->r19 = regs->gpr[19];
		state->r20 = regs->gpr[20];
		state->r21 = regs->gpr[21];
		state->r22 = regs->gpr[22];
		state->r23 = regs->gpr[23];
		state->r24 = regs->gpr[24];
		state->r25 = regs->gpr[25];
		state->r26 = regs->gpr[26];
		state->r27 = regs->gpr[27];
		state->r28 = regs->gpr[28];
		state->r29 = regs->gpr[29];
		state->r30 = regs->gpr[30];
		state->r31 = regs->gpr[31];
		state->srr0 = regs->nip;
		state->srr1 = regs->msr;
		state->ctr = regs->ctr;
		state->lr = regs->link;
		state->cr = regs->ccr;
		state->xer = regs->xer;
	}
	if (orig_gpr3_ptr != NULL) {
		struct pt_regs *extra_regs;

		extra_regs = (struct pt_regs *) regs->pt_extra_regs;
		*orig_gpr3_ptr = extra_regs->orig_gpr3;
	}
}

#include <osfmach3/processor.h>

#endif	/* _ASM_OSFMACH3_PPC_PROCESSOR_H */
