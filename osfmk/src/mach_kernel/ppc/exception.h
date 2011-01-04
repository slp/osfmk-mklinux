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

/* Miscellaneous constants and structures used by the exception
 * handlers
 */

#ifndef _PPC_EXCEPTION_H_
#define _PPC_EXCEPTION_H_

#ifndef ASSEMBLER

#include <cpus.h>
#include <mach_kgdb.h>

#include <mach/machine/vm_types.h>

/* When an exception is taken, this info is accessed via sprg0 */
/* We should always have this one on a cache line boundary */
struct per_proc_info {
	unsigned short	cpu_number;
	unsigned short	cpu_flags;	/* Various low-level flags */
	vm_offset_t  	istackptr;
	vm_offset_t  	intstack_top_ss;
#if	MACH_KGDB
	vm_offset_t  	gdbstackptr;
	vm_offset_t  	gdbstack_top_ss;
#else 
	unsigned int	ppigas1[2];	/* Take up some space if no KGDB */
#endif	/* MACH_KGDB */

	unsigned int	tempwork1;	/* Temp work area - monitor use carefully */
	unsigned int	tempwork2;	/* Temp work area - monitor use carefully */
	unsigned int	save_sr0;	/* Temp work area - monitor use carefully */

	/* PPC cache line boundary here */

	unsigned int save_exception_type;
	unsigned int phys_exception_handlers;
	unsigned int virt_per_proc_info;/* virt addr for our CPU */
	unsigned int active_kloaded;	/* pointer to active_kloaded[CPU_NO] */
	unsigned int cpu_data;		/* pointer to cpu_data[CPU_NO] */
	unsigned int active_stacks;	/* pointer to active_stacks[CPU_NO] */
	unsigned int need_ast;		/* pointer to need_ast[CPU_NO] */
	unsigned int fpu_pcb;		/* pcb owning the fpu on this cpu */

	/* PPC cache line boundary here */

	unsigned int save_cr;		/* Register save area from here on */
	unsigned int save_srr0;
	unsigned int save_srr1;
	unsigned int save_dar;
	unsigned int save_dsisr;
	unsigned int save_lr;
	unsigned int save_ctr;
	unsigned int save_xer;
	
	unsigned int save_r0;
	unsigned int save_r1;
	unsigned int save_r2;
	unsigned int save_r3;
	unsigned int save_r4;
	unsigned int save_r5;
	unsigned int save_r6;
	unsigned int save_r7;

	unsigned int save_r8;
	unsigned int save_r9;
	unsigned int save_r10;
	unsigned int save_r11;
	unsigned int save_r12;
	unsigned int save_r13;
	unsigned int save_r14;
	unsigned int save_r15;

	unsigned int save_r16;
	unsigned int save_r17;
	unsigned int save_r18;
	unsigned int save_r19;
	unsigned int save_r20;
	unsigned int save_r21;
	unsigned int save_r22;
	unsigned int save_r23;

	unsigned int save_r24;
	unsigned int save_r25;
	unsigned int save_r26;	
	unsigned int save_r27;
	unsigned int save_r28;
	unsigned int save_r29;
	unsigned int save_r30;
	unsigned int save_r31;
};

extern struct per_proc_info *per_proc_info;
/*	extern struct per_proc_info per_proc_info[NCPUS]; */

extern char *trap_type[];

static __inline__ struct per_proc_info * current_proc_info(void)
{
	register struct per_proc_info *ppi;
							   
	__asm__ volatile ("mr %0, 2" : "=r" (ppi));
	return ppi;
}

#endif /* ndef ASSEMBLER */

#define T_VECTOR_SIZE		4		/* function pointer size */
#define SIGPactive		0x8000

/* Hardware exceptions */

#define T_IN_VAIN		(0x00 * T_VECTOR_SIZE)
#define T_RESET			(0x01 * T_VECTOR_SIZE)
#define T_MACHINE_CHECK		(0x02 * T_VECTOR_SIZE)
#define T_DATA_ACCESS		(0x03 * T_VECTOR_SIZE)
#define T_INSTRUCTION_ACCESS	(0x04 * T_VECTOR_SIZE)
#define T_INTERRUPT		(0x05 * T_VECTOR_SIZE)
#define T_ALIGNMENT		(0x06 * T_VECTOR_SIZE)
#define T_PROGRAM		(0x07 * T_VECTOR_SIZE)
#define T_FP_UNAVAILABLE	(0x08 * T_VECTOR_SIZE)
#define T_DECREMENTER		(0x09 * T_VECTOR_SIZE)
#define T_IO_ERROR		(0x0a * T_VECTOR_SIZE)
#define T_RESERVED		(0x0b * T_VECTOR_SIZE)
#define T_SYSTEM_CALL		(0x0c * T_VECTOR_SIZE)
#define T_TRACE			(0x0d * T_VECTOR_SIZE)
#define T_FP_ASSIST		(0x0e * T_VECTOR_SIZE)
#define T_INSTRUCTION_BKPT	(0x13 * T_VECTOR_SIZE)
#define T_SYSTEM_MANAGEMENT	(0x14 * T_VECTOR_SIZE)

#define T_RUNMODE_TRACE		(0x20 * T_VECTOR_SIZE) /* 601 only */

#define T_SIGP		(0x21 * T_VECTOR_SIZE) /* MP only */

/* software exceptions */

#define T_AST			(0x100 * T_VECTOR_SIZE) 
#define T_MAX			T_SIGP		 /* Maximum exception no */

#endif /* _PPC_EXCEPTION_H_ */
