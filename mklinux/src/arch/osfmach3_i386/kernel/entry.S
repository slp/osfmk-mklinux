/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *  linux/arch/osfmach3_i386/entry.S
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/autoconf.h>

#include <linux/sys.h>
#include <linux/linkage.h>

.globl _sys_call_table

.data
ENTRY(sys_call_table)
	.long SYMBOL_NAME(sys_setup)		/* 0 */
	.long SYMBOL_NAME(sys_exit)
	.long SYMBOL_NAME(sys_fork)
	.long SYMBOL_NAME(sys_read)
	.long SYMBOL_NAME(sys_write)
	.long SYMBOL_NAME(sys_open)		/* 5 */
	.long SYMBOL_NAME(sys_close)
	.long SYMBOL_NAME(sys_waitpid)
	.long SYMBOL_NAME(sys_creat)
	.long SYMBOL_NAME(sys_link)
	.long SYMBOL_NAME(sys_unlink)		/* 10 */
	.long SYMBOL_NAME(sys_execve)
	.long SYMBOL_NAME(sys_chdir)
	.long SYMBOL_NAME(sys_time)
	.long SYMBOL_NAME(sys_mknod)
	.long SYMBOL_NAME(sys_chmod)		/* 15 */
	.long SYMBOL_NAME(sys_chown)
	.long SYMBOL_NAME(sys_break)
	.long SYMBOL_NAME(sys_stat)
	.long SYMBOL_NAME(sys_lseek)
	.long SYMBOL_NAME(sys_getpid)		/* 20 */
	.long SYMBOL_NAME(sys_mount)
	.long SYMBOL_NAME(sys_umount)
	.long SYMBOL_NAME(sys_setuid)
	.long SYMBOL_NAME(sys_getuid)
	.long SYMBOL_NAME(sys_stime)		/* 25 */
	.long SYMBOL_NAME(sys_ptrace)
	.long SYMBOL_NAME(sys_alarm)
	.long SYMBOL_NAME(sys_fstat)
	.long SYMBOL_NAME(sys_pause)
	.long SYMBOL_NAME(sys_utime)		/* 30 */
	.long SYMBOL_NAME(sys_stty)
	.long SYMBOL_NAME(sys_gtty)
	.long SYMBOL_NAME(sys_access)
	.long SYMBOL_NAME(sys_nice)
	.long SYMBOL_NAME(sys_ftime)		/* 35 */
	.long SYMBOL_NAME(sys_sync)
	.long SYMBOL_NAME(sys_kill)
	.long SYMBOL_NAME(sys_rename)
	.long SYMBOL_NAME(sys_mkdir)
	.long SYMBOL_NAME(sys_rmdir)		/* 40 */
	.long SYMBOL_NAME(sys_dup)
	.long SYMBOL_NAME(sys_pipe)
	.long SYMBOL_NAME(sys_times)
	.long SYMBOL_NAME(sys_prof)
	.long SYMBOL_NAME(sys_brk)			/* 45 */
	.long SYMBOL_NAME(sys_setgid)
	.long SYMBOL_NAME(sys_getgid)
	.long SYMBOL_NAME(sys_signal)
	.long SYMBOL_NAME(sys_geteuid)
	.long SYMBOL_NAME(sys_getegid)		/* 50 */
	.long SYMBOL_NAME(sys_acct)
	.long SYMBOL_NAME(sys_phys)
	.long SYMBOL_NAME(sys_lock)
	.long SYMBOL_NAME(sys_ioctl)
	.long SYMBOL_NAME(sys_fcntl)		/* 55 */
	.long SYMBOL_NAME(sys_mpx)
	.long SYMBOL_NAME(sys_setpgid)
	.long SYMBOL_NAME(sys_ulimit)
	.long SYMBOL_NAME(sys_olduname)
	.long SYMBOL_NAME(sys_umask)		/* 60 */
	.long SYMBOL_NAME(sys_chroot)
	.long SYMBOL_NAME(sys_ustat)
	.long SYMBOL_NAME(sys_dup2)
	.long SYMBOL_NAME(sys_getppid)
	.long SYMBOL_NAME(sys_getpgrp)		/* 65 */
	.long SYMBOL_NAME(sys_setsid)
	.long SYMBOL_NAME(sys_sigaction)
	.long SYMBOL_NAME(sys_sgetmask)
	.long SYMBOL_NAME(sys_ssetmask)
	.long SYMBOL_NAME(sys_setreuid)		/* 70 */
	.long SYMBOL_NAME(sys_setregid)
	.long SYMBOL_NAME(sys_sigsuspend)
	.long SYMBOL_NAME(sys_sigpending)
	.long SYMBOL_NAME(sys_sethostname)
	.long SYMBOL_NAME(sys_setrlimit)	/* 75 */
	.long SYMBOL_NAME(sys_getrlimit)
	.long SYMBOL_NAME(sys_getrusage)
	.long SYMBOL_NAME(sys_gettimeofday)
	.long SYMBOL_NAME(sys_settimeofday)
	.long SYMBOL_NAME(sys_getgroups)	/* 80 */
	.long SYMBOL_NAME(sys_setgroups)
	.long SYMBOL_NAME(old_select)
	.long SYMBOL_NAME(sys_symlink)
	.long SYMBOL_NAME(sys_lstat)
	.long SYMBOL_NAME(sys_readlink)		/* 85 */
	.long SYMBOL_NAME(sys_uselib)
	.long SYMBOL_NAME(sys_swapon)
	.long SYMBOL_NAME(sys_reboot)
	.long SYMBOL_NAME(old_readdir)
	.long SYMBOL_NAME(old_mmap)		/* 90 */
	.long SYMBOL_NAME(sys_munmap)
	.long SYMBOL_NAME(sys_truncate)
	.long SYMBOL_NAME(sys_ftruncate)
	.long SYMBOL_NAME(sys_fchmod)
	.long SYMBOL_NAME(sys_fchown)		/* 95 */
	.long SYMBOL_NAME(sys_getpriority)
	.long SYMBOL_NAME(sys_setpriority)
	.long SYMBOL_NAME(sys_profil)
	.long SYMBOL_NAME(sys_statfs)
	.long SYMBOL_NAME(sys_fstatfs)		/* 100 */
	.long SYMBOL_NAME(sys_ioperm)
	.long SYMBOL_NAME(sys_socketcall)
	.long SYMBOL_NAME(sys_syslog)
	.long SYMBOL_NAME(sys_setitimer)
	.long SYMBOL_NAME(sys_getitimer)	/* 105 */
	.long SYMBOL_NAME(sys_newstat)
	.long SYMBOL_NAME(sys_newlstat)
	.long SYMBOL_NAME(sys_newfstat)
	.long SYMBOL_NAME(sys_uname)
	.long SYMBOL_NAME(sys_iopl)		/* 110 */
	.long SYMBOL_NAME(sys_vhangup)
	.long SYMBOL_NAME(sys_idle)
	.long SYMBOL_NAME(sys_vm86old)
	.long SYMBOL_NAME(sys_wait4)
	.long SYMBOL_NAME(sys_swapoff)		/* 115 */
	.long SYMBOL_NAME(sys_sysinfo)
	.long SYMBOL_NAME(sys_ipc)
	.long SYMBOL_NAME(sys_fsync)
	.long SYMBOL_NAME(sys_sigreturn)
	.long SYMBOL_NAME(sys_clone)		/* 120 */
	.long SYMBOL_NAME(sys_setdomainname)
	.long SYMBOL_NAME(sys_newuname)
	.long SYMBOL_NAME(sys_modify_ldt)
	.long SYMBOL_NAME(sys_adjtimex)
	.long SYMBOL_NAME(sys_mprotect)		/* 125 */
	.long SYMBOL_NAME(sys_sigprocmask)
	.long SYMBOL_NAME(sys_create_module)
	.long SYMBOL_NAME(sys_init_module)
	.long SYMBOL_NAME(sys_delete_module)
	.long SYMBOL_NAME(sys_get_kernel_syms)	/* 130 */
	.long SYMBOL_NAME(sys_quotactl)
	.long SYMBOL_NAME(sys_getpgid)
	.long SYMBOL_NAME(sys_fchdir)
	.long SYMBOL_NAME(sys_bdflush)
	.long SYMBOL_NAME(sys_sysfs)		/* 135 */
	.long SYMBOL_NAME(sys_personality)
	.long 0				/* for afs_syscall */
	.long SYMBOL_NAME(sys_setfsuid)
	.long SYMBOL_NAME(sys_setfsgid)
	.long SYMBOL_NAME(sys_llseek)		/* 140 */
	.long SYMBOL_NAME(sys_getdents)
	.long SYMBOL_NAME(sys_select)
	.long SYMBOL_NAME(sys_flock)
	.long SYMBOL_NAME(sys_msync)
	.long SYMBOL_NAME(sys_readv)		/* 145 */
	.long SYMBOL_NAME(sys_writev)
	.long SYMBOL_NAME(sys_getsid)
	.long SYMBOL_NAME(sys_fdatasync)
	.long SYMBOL_NAME(sys_sysctl)
	.long SYMBOL_NAME(sys_mlock)		/* 150 */
	.long SYMBOL_NAME(sys_munlock)
	.long SYMBOL_NAME(sys_mlockall)
	.long SYMBOL_NAME(sys_munlockall)
	.long SYMBOL_NAME(sys_sched_setparam)
	.long SYMBOL_NAME(sys_sched_getparam)   /* 155 */
	.long SYMBOL_NAME(sys_sched_setscheduler)
	.long SYMBOL_NAME(sys_sched_getscheduler)
	.long SYMBOL_NAME(sys_sched_yield)
	.long SYMBOL_NAME(sys_sched_get_priority_max)
	.long SYMBOL_NAME(sys_sched_get_priority_min)  /* 160 */
	.long SYMBOL_NAME(sys_sched_rr_get_interval)
	.long SYMBOL_NAME(sys_nanosleep)
	.long SYMBOL_NAME(sys_mremap)
	.long 0,0
	.long SYMBOL_NAME(sys_vm86)
	.space (NR_syscalls-166)*4


/*
 * XXX
 * These are offsets in a "i386_saved_state" micro-kernel structure.
 * Update them if the structure changes.
 */
#define STATE_GS	0x00
#define STATE_FS	0x04
#define STATE_ES	0x08
#define STATE_DS	0x0c
#define STATE_EDI	0x10
#define STATE_ESI	0x14
#define STATE_EBP	0x18
#define STATE_ESP	0x1c
#define STATE_EBX	0x20
#define STATE_EDX	0x24
#define STATE_ECX	0x28
#define STATE_EAX	0x2c
#define STATE_TRAPNO	0x30
#define STATE_ERR	0x34
#define STATE_EIP	0x38
#define STATE_CS	0x3c
#define STATE_EFL	0x40
#define STATE_UESP	0x44
#define STATE_SS	0x48

/*
 * System call wrapper:
 * The system call routines expect the registers to be on the stack, like
 * arguments.
 * Push the registers like expected, call the routine and pop the modified
 * user regs back to the right place (the Mach thread state structure).
 */
#ifdef	CONFIG_OSFMACH3_DEBUG
#define ARG1	8(%ebp)
#define ARG2	12(%ebp)
#define ARG3	16(%ebp)
#define ARG4	20(%ebp)
#else	/* CONFIG_OSFMACH3_DEBUG */
#define ARG1	16(%esp)
#define ARG2	20(%esp)
#define ARG3	24(%esp)
#define ARG4	28(%esp)
#endif	/* CONFIG_OSFMACH3_DEBUG */
.text
ENTRY(syscall_wrapper)
#ifdef	CONFIG_OSFMACH3_DEBUG
	pushl	%ebp
	movl	%esp,%ebp
#endif	/* CONFIG_OSFMACH3_DEBUG */
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	movl	ARG1,%esi		/* function to be called */
	movl	ARG2,%edi		/* pointer to i386_saved_state struct*/
	movl	ARG3,%ebx		/* pointer to ptr to user regs */
	movl	ARG4,%ecx		/* pointer to orig_eax */

	movl	0(%ebx),%eax		/* current pointer to user regs */
	pushl	%eax

	movl	STATE_SS(%edi),%eax		/* regs.ss */
	pushl	%eax
	movl	STATE_UESP(%edi),%eax		/* regs.esp */
	pushl	%eax
	movl	STATE_EFL(%edi),%eax		/* regs.eflags */
	pushl	%eax
	movl	STATE_CS(%edi),%eax		/* regs.cs */
	pushl	%eax
	movl	STATE_EIP(%edi),%eax		/* regs.eip */
	pushl	%eax
	movl	0(%ecx),%eax			/* regs.orig_eax */
	pushl	%eax
	movl	STATE_GS(%edi),%eax		/* regs.gs */
	pushl	%eax
	movl	STATE_FS(%edi),%eax		/* regs.fs */
	pushl	%eax
	movl	STATE_ES(%edi),%eax		/* regs.es */
	pushl	%eax
	movl	STATE_DS(%edi),%eax		/* regs.ds */
	pushl	%eax
	movl	STATE_EAX(%edi),%eax		/* regs.eax */
	pushl	%eax
	movl	STATE_EBP(%edi),%eax		/* regs.ebp */
	pushl	%eax
	movl	STATE_EDI(%edi),%eax		/* regs.edi */
	pushl	%eax
	movl	STATE_ESI(%edi),%eax		/* regs.esi */
	pushl	%eax
	movl	STATE_EDX(%edi),%eax		/* regs.edx */
	pushl	%eax
	movl	STATE_ECX(%edi),%eax		/* regs.ecx */
	pushl	%eax
	movl	STATE_EBX(%edi),%eax		/* regs.ebx */
	pushl	%eax

	movl	%esp,0(%ebx)		/* update current->tss.regs_ptr to */
					/* reflect the new location of regs */

	call	*%esi			/* call sys_?? routine */
	movl	%eax,STATE_EAX(%edi)		/* regs.eax = return value */

	popl	%eax
	movl	%eax,STATE_EBX(%edi)		/* regs.ebx */
	popl	%eax
	movl	%eax,STATE_ECX(%edi)		/* regs.ecx */
	popl	%eax
	movl	%eax,STATE_EDX(%edi)		/* regs.edx */
	popl	%eax
	movl	%eax,STATE_ESI(%edi)		/* regs.esi */
	popl	%eax
	movl	%eax,STATE_EDI(%edi)		/* regs.edi */
	popl	%eax
	movl	%eax,STATE_EBP(%edi)		/* regs.ebp */
	popl	%eax				/* regs.eax */
	popl	%eax				/* resg.ds */
	popl	%eax				/* regs.es */
	popl	%eax				/* regs.fs */
	popl	%eax				/* regs.gs */
	popl	%eax
	movl	%eax,%ecx			/* regs.orig_eax */
	popl	%eax
	movl	%eax,STATE_EIP(%edi)		/* regs.eip */
	popl	%eax				/* regs.cs */
	popl	%eax
	movl	%eax,STATE_EFL(%edi)		/* regs.eflags */
	popl	%eax
	movl	%eax,STATE_UESP(%edi)		/* regs.esp */
	popl	%eax				/* regs.ss */

	popl	%eax
	movl	%eax,0(%ebx)		/* restore pointer to user regs */

	movl	ARG4,%eax			/* &orig_eax */
	movl	%ecx,0(%eax)			/* regs.orig_eax */

	popl	%ebx
	popl	%edi
	popl	%esi
#ifdef	CONFIG_OSFMACH3_DEBUG
	movl	%ebp,%esp
	popl	%ebp
#endif	/* CONFIG_OSFMACH3_DEBUG */
	ret
