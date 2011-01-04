/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *  linux/arch/ppc/kernel/traps.c
 *
 *  Copyright (C) 1995  Gary Thomas
 *  Adapted for PowerPC by Gary Thomas
 */

/*
 * This file handles the architecture-dependent parts of hardware exceptions
 */

#include <linux/autoconf.h>

#ifdef	CONFIG_OSFMACH3
#include <osfmach3/mach3_debug.h>
#endif	/* CONFIG_OSFMACH3 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/ldt.h>
#include <linux/user.h>
#include <linux/a.out.h>

#include <asm/pgtable.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <asm/io.h>

#include "ppc_machine.h"

#ifdef	CONFIG_OSFMACH3
void console_verbose(void)
{
	extern int console_loglevel;
	console_loglevel = 15;
}
#endif	/* CONFIG_OSFMACH3 */

/* #define printk _printk */

/*
 * Trap & Exception support
 */

void
trap_init(void)
{
}

void
_exception(int signr, struct pt_regs *regs)
{
#ifndef	CONFIG_OSFMACH3
	if (!(current->flags & PF_PTRACED))
		dump_regs(regs);
#endif	/* CONFIG_OSFMACH3 */
	force_sig(signr, current);
#ifdef	CONFIG_OSFMACH3
	if (current == FIRST_TASK) {
		printk("Failure in kernel at PC: %lx, MSR: %lx\n",
		       regs->nip, regs->msr);
		panic("** SERVER EXCEPTION **");
	}

#else	/* CONFIG_OSFMACH3 */
	if (!user_mode(regs))
	{
		printk("Failure in kernel at PC: %x, MSR: %x\n", regs->nip, regs->msr);
		while (1) ;
	}
#endif	/* CONFIG_OSFMACH3 */
}

#ifdef	CONFIG_OSFMACH3
void
#endif	/* CONFIG_OSFMACH3 */
MachineCheckException(struct pt_regs *regs)
{
#ifndef	CONFIG_OSFMACH3
	printk("Machine check at PC: %x[%x], SR: %x\n", regs->nip, va_to_phys(regs->nip), regs->msr);
#endif	/* CONFIG_OSFMACH3 */
	_exception(SIGSEGV, regs);	
}

#ifdef	CONFIG_OSFMACH3
void
#endif	/* CONFIG_OSFMACH3 */
ProgramCheckException(struct pt_regs *regs)
{
	if (current->flags & PF_PTRACED)
	{
#if 0
printk("Program check at PC: %x[%x], SR: %x\n", regs->nip, va_to_phys(regs->nip), regs->msr);
dump_buf(_get_SP(), 32);
#endif
		_exception(SIGTRAP, regs);
	} else
	{
#ifndef	CONFIG_OSFMACH3
		printk("Program check at PC: %x[%x], SR: %x\n", regs->nip, va_to_phys(regs->nip), regs->msr);
		dump_buf(regs->nip-32, 64);
#endif	/* CONFIG_OSFMACH3 */
		_exception(SIGILL, regs);
	}
}

#ifdef	CONFIG_OSFMACH3
void
#endif	/* CONFIG_OSFMACH3 */
SingleStepException(struct pt_regs *regs)
{
#if 0
  printk("Single step at PC: %x[%x], SR: %x\n", regs->nip, va_to_phys(regs->nip), regs->msr);
  dump_buf(_get_SP(), 32);
#endif
	regs->msr &= ~MSR_SE;  /* Turn off 'trace' bit */
	_exception(SIGTRAP, regs);	
}

#ifdef	CONFIG_OSFMACH3
void
#endif	/* CONFIG_OSFMACH3 */
FloatingPointCheckException(struct pt_regs *regs)
{
#ifndef	CONFIG_OSFMACH3
	printk("Floating point check at PC: %x[%x], SR: %x\n", regs->nip, va_to_phys(regs->nip), regs->msr);
#endif	/* CONFIG_OSFMACH3 */
	_exception(SIGFPE, regs);	
}

#ifdef	CONFIG_OSFMACH3
void
#endif	/* CONFIG_OSFMACH3 */
IOErrorException(struct pt_regs *regs)
{
#ifndef	CONFIG_OSFMACH3
	printk("IO Controller error at PC: %x[%x], SR: %x\n", regs->nip, va_to_phys(regs->nip), regs->msr);
#endif	/* CONFIG_OSFMACH3 */
	_exception(SIGIOT, regs);	
}

#ifndef	CONFIG_OSFMACH3
bad_stack(struct pt_regs *regs)
{
	printk("Kernel stack overflow at PC: %x[%x], SR: %x\n", regs->nip, va_to_phys(regs->nip), regs->msr);
	dump_regs(regs);
	while (1) ;
}
#endif	/* CONFIG_OSFMACH3 */

#ifdef	CONFIG_OSFMACH3
void
#endif	/* CONFIG_OSFMACH3 */
dump_regs(struct pt_regs *regs)
{
	int i;
#ifdef	CONFIG_OSFMACH3
	printk("NIP: %08lX, MSR: %08lX, XER: %08lX, LR: %08lX, FRAME: %p\n", regs->nip, regs->msr, regs->xer, regs->link, regs);
	printk("TASK = %p[%d] '%s'\n", current, current->pid, current->comm);
#else	/* CONFIG_OSFMACH3 */
	printk("NIP: %08X, MSR: %08X, XER: %08X, LR: %08X, FRAME: %08X\n", regs->nip, regs->msr, regs->xer, regs->link, regs);
#if 0	
	printk("HASH = %08X/%08X, MISS = %08X/%08X, CMP = %08X/%08X\n", regs->hash1, regs->hash2, regs->imiss, regs->dmiss, regs->icmp, regs->dcmp);
#endif	
	printk("TASK = %x[%d] '%s'\n", current, current->pid, current->comm);
#endif	/* CONFIG_OSFMACH3 */
	for (i = 0;  i < 32;  i++)
	{
		if ((i % 8) == 0)
		{
			printk("GPR%02d: ", i);
		}
#ifdef	CONFIG_OSFMACH3
		printk("%08lX ", regs->gpr[i]);
#else	/* CONFIG_OSFMACH3 */
		printk("%08X ", regs->gpr[i]);
#endif	/* CONFIG_OSFMACH3 */
		if ((i % 8) == 7)
		{
			printk("\n");
		}
	}
#if 0	
	if (regs->nip >= 0x1000)
		dump_buf(regs->nip-32, 64);
	dump_buf((regs->nip&0x0FFFFFFF)|KERNELBASE, 32);
#endif
}

int do_trace_syscalls;

#ifdef	CONFIG_OSFMACH3
void
#endif	/* CONFIG_OSFMACH3 */
trace_syscall(struct pt_regs *regs)
{
#ifdef	CONFIG_OSFMACH3
	if (!do_trace_syscalls) return;
	if ((do_trace_syscalls > 0) && (do_trace_syscalls != current->pid)) return;
	printk("Task: %p(%d), PC: %08lX/%08lX, Syscall: %3ld, Result: %s%ld\n", current, current->pid, regs->nip, regs->link, regs->gpr[0], regs->ccr&0x10000000?"Error=":"", regs->gpr[3]);
#else	/* CONFIG_OSFMACH3 */
	static int count;
	if (!do_trace_syscalls) return;
	if ((do_trace_syscalls > 0) && (do_trace_syscalls != current->pid)) return;
	printk("Task: %08X(%d), PC: %08X/%08X, Syscall: %3d, Result: %s%d\n", current, current->pid, regs->nip, regs->link, regs->gpr[0], regs->ccr&0x10000000?"Error=":"", regs->gpr[3]);
	if (++count == 20)
	{
		count = 0;
		cnpause();
	}
#endif	/* CONFIG_OSFMACH3 */
}

#ifdef	CONFIG_OSFMACH3
int kstack_depth_to_print = 24;

void die_if_kernel(char * str, struct pt_regs * regs, long err)
{
	int			i;
	unsigned long		*stack;
	unsigned long		addr;
	extern char		_stext, _etext;
	struct task_struct	*tsk;

	if (current != FIRST_TASK) {
		/* it's a user process */
		extern boolean_t server_exception_in_progress;
		if (!server_exception_in_progress) {
			return;
		}
		/* no, no, it's a server exception ! */
	}

	console_verbose();
	printk("%s: %04lx\n", str, err & 0xffff);
	show_regs(regs);
	tsk = current;
	if (tsk != NULL) {
		printk("Process %s (pid: %d, stackpage=%08lx)\nStack: ",
		       tsk->comm, tsk->pid, regs->gpr[1]);
	}
	stack = (unsigned long *) regs->gpr[1];
	for (i = 0; i < kstack_depth_to_print; i++) {
		if (((long) stack & 4095) == 0)
			break;
		if (i && ((i % 8) == 0))
			printk("\n	");
		printk("%08lx ", *stack++);
	}
	printk("\nCall Trace: ");
	stack = (unsigned long *) regs->gpr[1];
	i = 1;
	while (((long) stack & 4095) != 0) {
		addr = *stack++;
		if (((addr >= (unsigned long) &_stext) &&
		     (addr <= (unsigned long) &_etext)) ) {
			if (i && ((i % 8) == 0))
				printk("\n	");
			printk("%08lx ", addr);
			i++;
		}
	}
	printk("\nCode: ");
	for (i = 0; i < 20; i++)
		printk("%02x ", 0xff & *((unsigned char *) regs->nip + i));
	printk("\n");
	Debugger("die_if_kernel");
	return;
	panic("die_if_kernel");
	do_exit(SIGSEGV);
}
#endif	/* CONFIG_OSFMACH3 */
