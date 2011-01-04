/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _MACHINE_PTRACE_H
#define _MACHINE_PTRACE_H

#include <mach/thread_status.h>

struct pt_regs {
	struct hp700_thread_state state;
	struct hp700_float_state  fstate;
};

#define INIT_PTREGS { { 0 }, { 0 } }

#define LOWER_PRIV(pc)	((pc) | 3)

#ifdef __KERNEL__
extern void show_regs(struct pt_regs *);
#endif

/* Offsets used by 'ptrace' system call interface */
#define PT_R0	0
#define PT_R1	1
#define PT_R2	2
#define PT_R3	3
#define PT_R4	4
#define PT_R5	5
#define PT_R6	6
#define PT_R7	7
#define PT_R8	8
#define PT_R9	9
#define PT_R10	10
#define PT_R11	11
#define PT_R12	12
#define PT_R13	13
#define PT_R14	14
#define PT_R15	15
#define PT_R16	16
#define PT_R17	17
#define PT_R18	18
#define PT_R19	19
#define PT_R20	20
#define PT_R21	21
#define PT_R22	22
#define PT_R23	23
#define PT_R24	24
#define PT_R25	25
#define PT_R26	26
#define PT_R27	27
#define PT_R28	28
#define PT_R29	29
#define PT_R30	30
#define PT_R31	31

#define PT_SAR	32

#define PT_HOQ	33
#define PT_HSQ  34
#define PT_TOQ	35
#define PT_TSQ	36

#define PT_FPU	59

#define PT_FP0	60
#define PT_FP31	122

#endif /* _MACHINE_PTRACE_H */
