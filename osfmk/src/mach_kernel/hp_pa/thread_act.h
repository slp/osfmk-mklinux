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

#ifndef	_HP_PA_THREAD_ACT_H_
#define	_HP_PA_THREAD_ACT_H_

#include <kgdb.h>
#include <mach/boolean.h>
#include <mach/vm_param.h>
#include <mach/vm_types.h>
#include <mach/thread_status.h>
#include <kern/lock.h>
#include <machine/vm_tuning.h>

/*
 * HP700 process control block
 *
 * In the continuation model, the PCB holds the user context. It is used
 * on entry to the kernel from user mode, either by system call or trap,
 * to store the necessary user registers and other state.
 */
struct pcb
{
	struct hp700_saved_state ss;
	struct hp700_float_state sf;
	vm_offset_t ksp;		/* kernel stack pointer */
#if	KGDB
	unsigned kgdb_trace;		/* hacks for kgdb single stepping */
	unsigned saved_insn;
	unsigned saved_oqt;
	unsigned saved_sqt;
#endif
};

typedef struct pcb *pcb_t;


/*
 * Kernel state structure
 *
 * This holds the kernel state that is saved and restored across context
 * switches. This is kept at the base of the kernel stack.
 *
 */

struct hp700_kernel_state
{
	unsigned rp;		/* Return pointer, r2 */
	unsigned r3;		/* Callee save general registers */
	unsigned r4;
	unsigned r5;
	unsigned r6;
	unsigned r7;
	unsigned r8;
	unsigned r9;
	unsigned r10;
	unsigned r11;
	unsigned r12;
	unsigned r13;
	unsigned r14;
	unsigned r15;
	unsigned r16;
	unsigned r17;
	unsigned r18;
	unsigned sp;		/* Stack pointer, r30 */
};

struct hp700_trap_state {
	unsigned t2;            /* r21 */
	unsigned t1;            /* r22 */
	unsigned arg0;          /* r26 */
	unsigned sp;            /* r30 */
	unsigned iioqh;
	unsigned iioqt;
	unsigned iisqh;
	unsigned iisqt;
	unsigned eiem;          /* cr15 */
	unsigned iir;           /* cr19 */
	unsigned isr;           /* cr20 */
	unsigned ior;           /* cr21 */
	unsigned ipsw;          /* cr22 */
};

typedef struct MachineThrAct {
	/*
	 * pointer to process control block
	 */
	pcb_t pcb;

} MachineThrAct, *MachineThrAct_t;

/*
 * Maps state flavor to number of words in the state:
 */
extern unsigned int state_count[];

#define KS_SIZE	                sizeof(struct hp700_kernel_state)

#define USER_REGS(ThrAct)	(&(ThrAct)->mact.pcb->ss)
#define USER_FREGS(ThrAct)	(&(ThrAct)->mact.pcb->sf)

#define	user_pc(ThrAct)		(USER_REGS(ThrAct)->iioq_head)

#define act_machine_state_ptr(ThrAct)	(thread_state_t)USER_REGS(ThrAct)

#endif	/* _HP_PA_THREAD_ACT_H_ */
