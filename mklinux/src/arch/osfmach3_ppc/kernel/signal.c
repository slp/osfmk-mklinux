/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *  linux/arch/ppc/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Adapted for PowerPC by Gary Thomas
 */

#include <linux/autoconf.h>

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>
#include <asm/segment.h>
#include <asm/elf.h>

#ifdef	CONFIG_OSFMACH3
#include <mach/mach_interface.h>
#include <mach/mach_port.h>
#include <mach/mach_host.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/uniproc.h>

#include <asm/pgtable.h>
#endif	/* CONFIG_OSFMACH3 */

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

#define DEBUG_SIGNALS
#undef  DEBUG_SIGNALS

asmlinkage int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options);
#ifdef	CONFIG_OSFMACH3
asmlinkage int do_signal(unsigned long oldmask, struct pt_regs * regs);
#endif	/* CONFIG_OSFMACH3 */

/*
 * atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int sys_sigsuspend(unsigned long set, int p2, int p3, int p4, int p6, int p7, struct pt_regs *regs)
{
	unsigned long mask;

	mask = current->blocked;
	current->blocked = set & _BLOCKABLE;
	regs->gpr[3] = -EINTR;
	regs->result = -EINTR;
#ifdef DEBUG_SIGNALS
printk("Task: '%s'(%p)[%d] - SIGSUSPEND at %lx, Mask: %lx\n", current->comm, current, current->pid, regs->nip, set);	
#endif
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (do_signal(mask,regs)) {
			/*
			 * If a signal handler needs to be called,
			 * do_signal() has set R3 to the signal number (the
			 * first argument of the signal handler), so don't
			 * overwrite that with EINTR !
			 * In the other cases, do_signal() doesn't touch 
			 * R3, so it's still set to -EINTR (see above).
			 */
			return regs->gpr[3];
		}
#ifdef DEBUG_SIGNALS
printk("Task: '%s'(%p)[%d] - loop around signal at %lx, Mask: %lx\n", current->comm, current, current->pid, regs->nip, set);	
#endif
	}
}

/* 
 * These are the flags in the MSR that the user is allowed to change
 * by modifying the saved value of the MSR on the stack.  SE and BE
 * should not be in this list since gdb may want to change these.  I.e,
 * you should be able to step out of a signal handler to see what
 * instruction executes next after the signal handler completes.
 * Alternately, if you stepped into a signal handler, you should be
 * able to continue 'til the next breakpoint from within the signal
 * handler, even if the handler returns.
 */
#define MSR_USERCHANGE	(MSR_FE0 | MSR_FE1)

/*
 * This sets regs->esp even though we don't actually use sigstacks yet..
 */
asmlinkage int sys_sigreturn(struct pt_regs *regs)
{
	struct sigcontext_struct *sc, sigctx;
	int ret;
	elf_gregset_t saved_regs;

#ifdef	CONFIG_OSFMACH3
	struct pt_regs *extra_regs = (struct pt_regs *) regs->pt_extra_regs;
	kern_return_t	kr;
	struct ppc_float_state fpregs;
	mach_port_t		child_thread;
	
#endif	/* CONFIG_OSFMACH3 */

	sc = (struct sigcontext_struct *)(regs->gpr[1] + __SIGNAL_FRAMESIZE);
	memcpy_fromfs(&sigctx, sc, sizeof(sigctx));

	current->blocked = sigctx.oldmask & _BLOCKABLE;
	sc++;			/* Pop signal 'context' */
#ifdef DEBUG_SIGNALS
printk("Sig return - Regs: %p, sc: %p, sig: %d\n", sigctx.regs, sc, sigctx.signal);
#endif
	if (sc == (struct sigcontext_struct *)(sigctx.regs)) {
  		/* Last stacked signal - restore registers */
#ifndef	CONFIG_OSFMACH3
		if (fpu_tss == &current->tss)
			giveup_fpu();
#endif	/* CONFIG_OSFMACH3 */

		memcpy_fromfs(saved_regs, sigctx.regs,
			      sizeof(saved_regs));

		saved_regs[PT_MSR] = (regs->msr & ~MSR_USERCHANGE)
			| (saved_regs[PT_MSR] & MSR_USERCHANGE);

#ifdef ELF_CORE_RECOVER_REGS
		ELF_CORE_RECOVER_REGS(saved_regs, regs);
#else
		if (sizeof(elf_gregset_t) != sizeof(struct pt_regs)) {
			printk("sizeof(elf_gregset_t) (%d) != sizeof(struct pt_regs) (%d)\n",
				 sizeof(elf_gregset_t), sizeof(struct pt_regs));
		} else {
			memcpy(regs, saved_regs, sizeof(saved_regs));
		}
#endif
#ifdef	CONFIG_OSFMACH3
		memcpy_fromfs((unsigned char*)&fpregs,
			      (unsigned char *)sigctx.regs +
			      sizeof(elf_gregset_t),
			      sizeof(elf_fpregset_t));
		child_thread = current->osfmach3.thread->mach_thread_port;
		server_thread_blocking(FALSE);
		kr = thread_set_state(child_thread,
				      PPC_FLOAT_STATE,
				      (thread_state_t)&fpregs,
				      PPC_FLOAT_STATE_COUNT);
		server_thread_unblocking(FALSE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("sys_sigreturn: "
				     "thread_set_state(0x%x)",
				     child_thread));
		}
		if (extra_regs->trap == 0x0C00 /* System Call! */ &&
		    ((int)extra_regs->result == -ERESTARTNOHAND ||
		     (int)extra_regs->result == -ERESTARTSYS ||
		     (int)extra_regs->result == -ERESTARTNOINTR)) {
			regs->gpr[3] = extra_regs->orig_gpr3;
			regs->nip -= 4; /* Back up & retry system call */
			extra_regs->result = 0;
		}
		ret = extra_regs->result;
#else	/* CONFIG_OSFMACH3 */
		memcpy_fromfs(current->tss.fpr,
			      (unsigned char *)sigctx.regs +
			      sizeof(elf_gregset_t),
			      sizeof(elf_fpregset_t));
		if (regs->trap == 0x0C00 /* System Call! */ &&
		    ((int)regs->result == -ERESTARTNOHAND ||
		     (int)regs->result == -ERESTARTSYS ||
		     (int)regs->result == -ERESTARTNOINTR)) {
			regs->gpr[3] = regs->orig_gpr3;
			regs->nip -= 4; /* Back up & retry system call */
			regs->result = 0;
		}
		ret = regs->result;
#endif	/* CONFIG_OSFMACH3 */
#ifdef PAUSE_AFTER_SIGNALS
		cnpause();
#endif
	} else {
		/* More signals to go */
		regs->gpr[1] = (unsigned long)sc - __SIGNAL_FRAMESIZE;
		memcpy_fromfs(&sigctx, sc, sizeof(sigctx));

		regs->gpr[3] = ret = sigctx.signal;
		regs->gpr[4] = (unsigned long) sc;
		/* Pointer to trampoline */
		regs->link = (unsigned long)sigctx.regs +
			sizeof(elf_gregset_t) +
			sizeof(elf_fpregset_t);
		regs->nip = sigctx.handler;
	}
	return ret;
}


/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals that
 * the kernel can handle, and then we build all the user-level signal handling
 * stack-frames in one go after that.
 */
asmlinkage int do_signal(unsigned long oldmask, struct pt_regs * regs)
{
	unsigned long mask;
	unsigned long handler_signal = 0;
	unsigned long *frame = NULL;
	unsigned long *trampoline;
	unsigned long *regs_ptr;
	elf_gregset_t saved_regs;
	double *fpregs_ptr;
	unsigned long nip = 0;
	unsigned long signr;
	struct sigcontext_struct *sc, sigctx;
	struct sigaction * sa;
	int bitno;
#ifdef	CONFIG_OSFMACH3
	struct pt_regs	*extra_regs = (struct pt_regs *) regs->pt_extra_regs;
	struct ppc_float_state fpregs;

	kern_return_t		kr;
	mach_msg_type_number_t	count;
	mach_port_t		child_thread;
	
#else	/* CONFIG_OSFMACH3 */
	int s = _disable_interrupts();
#endif	/* CONFIG_OSFMACH3 */

	mask = ~current->blocked;
	while ((signr = current->signal & mask)) {
#if 0
		signr = ffz(~signr);  /* Compute bit # */
#else
		for (bitno = 0;  bitno < 32;  bitno++)
			if (signr & (1<<bitno))
				break;
		signr = bitno;
#endif
		current->signal &= ~(1<<signr);  /* Clear bit */
		sa = current->sig->action + signr;
		signr++;
		if ((current->flags & PF_PTRACED) && signr != SIGKILL) {
			current->exit_code = signr;
			current->state = TASK_STOPPED;
			notify_parent(current, SIGCHLD);
			schedule();
			if (!(signr = current->exit_code))
				continue;
			current->exit_code = 0;
			if (signr == SIGSTOP)
				continue;
			if (_S(signr) & current->blocked) {
				current->signal |= _S(signr);
				continue;
			}
			sa = current->sig->action + signr - 1;
		}
		if (sa->sa_handler == SIG_IGN) {
			if (signr != SIGCHLD)
				continue;
			/* check for SIGCHLD: it's special */
			while (sys_waitpid(-1,NULL,WNOHANG) > 0)
				/* nothing */;
			continue;
		}
		if (sa->sa_handler == SIG_DFL) {
			if (current->pid == 1)
				continue;
			switch (signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;

			case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (is_orphaned_pgrp(current->pgrp))
					continue;
			case SIGSTOP:
				if (current->flags & PF_PTRACED)
					continue;
				current->state = TASK_STOPPED;
				current->exit_code = signr;
				if (!(current->p_pptr->sig->action[SIGCHLD-1].sa_flags &
						SA_NOCLDSTOP))
					notify_parent(current, SIGCHLD);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGIOT: case SIGFPE: case SIGSEGV:
				if (current->binfmt && current->binfmt->core_dump) {
					if (current->binfmt->core_dump(signr, regs))
						signr |= 0x80;
				}
				/* fall through */
			default:
				current->signal |= _S(signr & 0x7f);
				current->flags |= PF_SIGNALED;
				do_exit(signr);
			}
		}
		/*
		 * OK, we're invoking a handler
		 */
#ifdef	CONFIG_OSFMACH3
		if (extra_regs->trap == 0x0c00)
#else	/* CONFIG_OSFMACH3 */
		if (regs->trap == 0x0c00)
#endif	/* CONFIG_OSFMACH3 */
		{
#ifdef	CONFIG_OSFMACH3
			if ((int)extra_regs->result == -ERESTARTNOHAND ||
			   ((int)extra_regs->result == -ERESTARTSYS &&
			    !(sa->sa_flags & SA_RESTART)))
				(int)extra_regs->result = -EINTR;
#else	/* CONFIG_OSFMACH3 */
			if ((int)regs->result == -ERESTARTNOHAND ||
			   ((int)regs->result == -ERESTARTSYS &&
			    !(sa->sa_flags & SA_RESTART)))
				(int)regs->result = -EINTR;
#endif	/* CONFIG_OSFMACH3 */
		}
		handler_signal |= 1 << (signr-1);
		mask &= ~sa->sa_mask;
	}

#ifdef	CONFIG_OSFMACH3
	if (extra_regs->trap == 0x0c00 &&
	    ((int)extra_regs->result == -ERESTARTNOHAND ||
	     (int)extra_regs->result == -ERESTARTSYS ||
	     (int)extra_regs->result == -ERESTARTNOINTR)) {
		regs->gpr[3] = extra_regs->orig_gpr3;
		regs->nip -= 4; /* Back up & retry system call */
		extra_regs->result = 0;
	}
#else	/* CONFIG_OSFMACH3 */
	if ((int)regs->trap == 0x0c00 &&
	    ((int)regs->result == -ERESTARTNOHAND ||
	     (int)regs->result == -ERESTARTSYS ||
	     (int)regs->result == -ERESTARTNOINTR)) {
		regs->gpr[3] = regs->orig_gpr3;
		regs->nip -= 4; /* Back up & retry system call */
		regs->result = 0;
	}
#endif	/* CONFIG_OSFMACH3 */

	if (!handler_signal)		/* no handler will be called - return 0 */
	{
#ifndef	CONFIG_OSFMACH3
		_enable_interrupts(s);
#endif	/* CONFIG_OSFMACH3 */
		return 0;
	}
	nip = regs->nip;
	frame = (unsigned long *) regs->gpr[1];
	frame = (unsigned long *)((unsigned long)frame & 0xFFFFFFE0);
	/*
	 * Build trampoline code on stack, and save gp and fp regs.
	 * The 56 word hole is because programs using the rs6000/xcoff
	 * style calling sequence can save up to 19 gp regs and 18 fp regs
	 * on the stack before decrementing sp.
	 */
	frame -= 2 + 56;
	trampoline = frame;
	frame -= sizeof(elf_fpregset_t) / sizeof(unsigned long);
	fpregs_ptr = (double *) frame;
	frame -= sizeof(elf_gregset_t) / sizeof(unsigned long);
	regs_ptr = frame;
	/* verify stack is valid for writing to */
	if (verify_area(VERIFY_WRITE, frame,
			((2 + 56) * sizeof(long)) +
			sizeof(elf_gregset_t) +
			sizeof(elf_fpregset_t))) {
		do_exit(SIGSEGV);
	}
#ifndef	CONFIG_OSFMACH3
	if (fpu_tss == &current->tss)
		giveup_fpu();
#else	/* CONFIG_OSFMACH3 */
	count = PPC_FLOAT_STATE_COUNT;
	child_thread = current->osfmach3.thread->mach_thread_port;

	server_thread_blocking(FALSE);
	kr = thread_get_state(child_thread,
			      PPC_FLOAT_STATE,
			      (thread_state_t)&fpregs,
			      &count);
	server_thread_unblocking(FALSE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("do_signal: "
			     "thread_get_state(0x%x)",
			     child_thread));
	}
#endif	/* CONFIG_OSFMACH3 */

	/* Translate to PT_ layout */
#ifdef ELF_CORE_COPY_REGS
	ELF_CORE_COPY_REGS(saved_regs, regs);
#else
	if (sizeof(elf_gregset_t) != sizeof(struct pt_regs)) {
		printk("sizeof(elf_gregset_t) (%d) != sizeof(struct pt_regs) (%d)\n",
		       sizeof(elf_gregset_t), sizeof(struct pt_regs));
	} else {
		memcpy(saved_regs, regs, sizeof(saved_regs));
	}
#endif
	memcpy_tofs(regs_ptr, saved_regs, sizeof(saved_regs));
#ifdef	CONFIG_OSFMACH3
	memcpy_tofs(fpregs_ptr, &fpregs, sizeof(elf_fpregset_t));
#else	/* CONFIG_OSFMACH3 */
	memcpy_tofs(fpregs_ptr, current->tss.fpr, sizeof(elf_fpregset_t));
#endif	/* CONFIG_OSFMACH3 */
	put_fs_long(0x38007777UL, trampoline);      /* li r0,0x7777 */
	put_fs_long(0x44000002UL, trampoline+1);    /* sc           */

	signr = 1;
	sa = current->sig->action;
#ifdef	CONFIG_OSFMACH3
	flush_instruction_cache_range(current->mm,
				      (unsigned long) trampoline,
				      (unsigned long) (trampoline + 2));

	sc = 0;	/* Avoid compiler warning */
#endif	/* CONFIG_OSFMACH3 */

	for (mask = 1 ; mask ; sa++,signr++,mask += mask) {
		if (mask > handler_signal)
			break;
		if (!(mask & handler_signal))
			continue;

		frame -= sizeof(struct sigcontext_struct) / sizeof(long);
		if (verify_area(VERIFY_WRITE, frame,
				sizeof(struct sigcontext_struct))) {
			do_exit(SIGSEGV);
		}
#ifdef DEBUG_SIGNALS
printk("Task: '%s'(%p)[%d] Send signal: %ld, frame: %p\n", current->comm, current, current->pid, signr, frame);
#endif
		sc = (struct sigcontext_struct *)frame;
		nip = (unsigned long) sa->sa_handler;
		if (sa->sa_flags & SA_ONESHOT)
			sa->sa_handler = NULL;
		sigctx.handler = nip;
		sigctx.oldmask = oldmask;
		sigctx.regs    = (struct pt_regs *)regs_ptr;
		sigctx.signal  = signr;
		memcpy_tofs(sc, &sigctx, sizeof(sigctx));
		current->blocked |= sa->sa_mask;
		regs->gpr[3] = signr;
		regs->gpr[4] = (unsigned long) sc;
	}

	frame -= __SIGNAL_FRAMESIZE / sizeof(unsigned long);

	if (verify_area(VERIFY_WRITE, frame, __SIGNAL_FRAMESIZE)) {
		do_exit(SIGSEGV);
	}
	put_fs_long(regs->gpr[1], frame);

	regs->link = (unsigned long)trampoline;
	regs->nip = nip;
	regs->gpr[1] = (unsigned long) frame;
if ((unsigned long)frame & 0x0F) printk("Task: '%s'(%p)[%d] Signal - PC: %lx, SP: %lx, SC: %p\n", current->comm, current, current->pid, regs->nip, regs->gpr[1], sc);

#ifdef DEBUG_SIGNALS
printk("Task: '%s'(%p)[%d] Signal - PC: %lx, SP: %lx, SC: %p\n", current->comm, current, current->pid, regs->nip, regs->gpr[1], sc);
#endif

	/* The DATA cache must be flushed here to insure coherency */
	/* between the DATA & INSTRUCTION caches.  Since we just */
	/* created an instruction stream using the DATA [cache] space */
	/* and since the instruction cache will not look in the DATA */
	/* cache for new data, we have to force the data to go on to */
	/* memory and flush the instruction cache to force it to look */
	/* there.  The following function performs this magic */
	flush_instruction_cache();
#ifndef	CONFIG_OSFMACH3
	_enable_interrupts(s);
#endif	/* CONFIG_OSFMACH3 */
	return 1;
}
