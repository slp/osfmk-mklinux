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

#ifndef	_MACH_PPC_THREAD_STATUS_H_
#define _MACH_PPC_THREAD_STATUS_H_

/*
 * ppc_thread_state is the structure that is exported to user threads for 
 * use in status/mutate calls.  This structure should never change.
 *
 * TODO : it'd be nice to provide 64 bit compatibility in this structure
 *        - need to make sure that it's understood by gdb though?
 */

#define PPC_THREAD_STATE        1
#define PPC_FLOAT_STATE         2
#define PPC_EXCEPTION_STATE	3
#define THREAD_STATE_NONE	7
	       
struct ppc_thread_state {
	unsigned int srr0;      /* Instruction address register (PC) */
	unsigned int srr1;	/* Machine state register (supervisor) */
				/* srr1 may contain SRR_SYSCALL_VAL */
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int r13;
	unsigned int r14;
	unsigned int r15;
	unsigned int r16;
	unsigned int r17;
	unsigned int r18;
	unsigned int r19;
	unsigned int r20;
	unsigned int r21;
	unsigned int r22;
	unsigned int r23;
	unsigned int r24;
	unsigned int r25;
	unsigned int r26;
	unsigned int r27;
	unsigned int r28;
	unsigned int r29;
	unsigned int r30;
	unsigned int r31;

	unsigned int cr;        /* Condition register */
	unsigned int xer;	/* User's integer exception register */
	unsigned int lr;	/* Link register */
	unsigned int ctr;	/* Count register */
	unsigned int mq;	/* MQ register (601 only) */

	unsigned int pad;      /* structure TODO - check these! */
};

/* This structure should be double-word aligned for performance */

struct ppc_float_state {
	double  fpregs[32];

	unsigned int fpscr_pad; /* fpscr is 64 bits, 32 bits of rubbish */
	unsigned int fpscr;	/* floating point status register */
};

/*
 * saved state structure
 *
 * This structure corresponds to the state of the user registers as saved
 * on the stack upon kernel entry (saved in pcb). On interrupts and exceptions
 * we save all registers. On system calls we only save the registers not
 * saved by the caller.
 *
 * NB: If you change this structure you must also change the KGDB stub as well.
 * 
 * TODO : not sure about the save/restore regs. Does this need to be
 *        different to ppc_thread_state?
 */

struct ppc_saved_state {
	unsigned int srr0;      /* Instruction address register (PC) */
	unsigned int srr1;	/* Machine state register (supervisor) */
				/* srr1 may contain SRR_SYSCALL_VAL */
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int r13;
	unsigned int r14;
	unsigned int r15;
	unsigned int r16;
	unsigned int r17;
	unsigned int r18;
	unsigned int r19;
	unsigned int r20;
	unsigned int r21;
	unsigned int r22;
	unsigned int r23;
	unsigned int r24;
	unsigned int r25;
	unsigned int r26;
	unsigned int r27;
	unsigned int r28;
	unsigned int r29;
	unsigned int r30;
	unsigned int r31;

	unsigned int cr;        /* Condition register */
	unsigned int xer;	/* User's integer exception register */
	unsigned int lr;	/* Link register */
	unsigned int ctr;	/* Count register */
	unsigned int mq;	/* MQ register (601 only) */
	unsigned int pad;	/* To mirror pcb_thread_state in pcb*/

	unsigned int sr_copyin; /* SR_COPYIN is used for remapping */
	unsigned int pad2;	/* struct alignment */
	unsigned int pad3;
	unsigned int pad4;
};

/*
 * ppc_exception_state
 *
 * This structure corresponds to some additional state of the user
 * registers as saved in the PCB upon kernel entry. They are only
 * available if an exception is passed out of the kernel, and even
 * then not all are guaranteed to be updated.
 *
 * Some padding is included in this structure which allows space for
 * servers to store temporary values if need be, to maintain binary
 * compatiblity.
 */

struct ppc_exception_state {
	unsigned long dar;	/* Fault registers for coredump */
	unsigned long dsisr;
	unsigned long exception;/* number of powerpc exception taken */
	unsigned long pad0;	/* align to 16 bytes */

	unsigned long pad1[4];	/* space in PCB "just in case" */
};

/*
 * Save State Flags
 */

#define PPC_THREAD_STATE_COUNT \
   (sizeof(struct ppc_thread_state) / sizeof(int))

#define PPC_EXCEPTION_STATE_COUNT \
   (sizeof(struct ppc_exception_state) / sizeof(int))

#define PPC_FLOAT_STATE_COUNT \
   (sizeof(struct ppc_float_state) / sizeof(int))

/*
 * Machine-independent way for servers and Mach's exception mechanism to
 * choose the most efficient state flavor for exception RPC's:
 */
#define MACHINE_THREAD_STATE		PPC_THREAD_STATE
#define MACHINE_THREAD_STATE_COUNT	PPC_THREAD_STATE_COUNT

/*
 * Largest state on this machine:
 */
#define THREAD_MACHINE_STATE_MAX	PPC_FLOAT_STATE_COUNT

#endif /* _MACH_PPC_THREAD_STATUS_H_ */
