/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_I386_PROCESSOR_H
#define _ASM_OSFMACH3_I386_PROCESSOR_H

#include <asm/vm86.h>
#include <asm/math_emu.h>

#include <mach/machine/vm_param.h>

#define TASK_SIZE	VM_MAX_ADDRESS

/*
 * Size of io_bitmap in longwords: 32 is ports 0-0x3ff.
 */
#define IO_BITMAP_SIZE	32

static __inline__ void
osfmach3_state_to_ptregs(
	struct i386_saved_state	*state,
	struct pt_regs		*regs,
	unsigned long		orig_eax)
{
	regs->ebx = state->ebx;
	regs->ecx = state->ecx;
	regs->edx = state->edx;
	regs->esi = state->esi;
	regs->edi = state->edi;
	regs->ebp = state->ebp;
	regs->eax = state->eax;
	regs->ds = state->ds;
	regs->es = state->es;
	regs->fs = state->fs;
	regs->gs = state->gs;
	regs->orig_eax = orig_eax;
	regs->eip = state->eip;
	regs->cs = state->cs;
	regs->eflags = state->efl;
	regs->esp = state->uesp;
	regs->ss = state->ss;
}

static __inline__ void
osfmach3_ptregs_to_state(
	struct pt_regs		*regs,
	struct i386_saved_state	*state,
	unsigned long		*orig_eax_ptr)
{
	state->edi = regs->edi;
	state->esi = regs->esi;
	state->ebp = regs->ebp;
	state->ebx = regs->ebx;
	state->edx = regs->edx;
	state->ecx = regs->ecx;
	state->eax = regs->eax;
	state->eip = regs->eip;
	state->efl = regs->eflags;
	state->uesp = regs->esp;
	if (orig_eax_ptr != NULL)
		*orig_eax_ptr = regs->orig_eax;
}

#include <osfmach3/processor.h>

#endif	/* _ASM_OSFMACH3_I386_PROCESSOR_H */
