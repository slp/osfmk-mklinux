/*
 *  linux/kernel/fork.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also system_call.s).
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
 */

#include <linux/autoconf.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/ldt.h>
#include <linux/smp.h>

#include <asm/segment.h>
#include <asm/system.h>
#include <asm/pgtable.h>

#ifdef	CONFIG_OSFMACH3
#include <osfmach3/assert.h>
#include <osfmach3/inode_pager.h>
#endif	/* CONFIG_OSFMACH3 */

int nr_tasks=1;
int nr_running=1;
unsigned long int total_forks=0;	/* Handle normal Linux uptimes. */
int last_pid=0;

#ifdef	CONFIG_OSFMACH3
static inline int find_empty_process(struct task_struct *parent,
				     int system_task)
#else	/* CONFIG_OSFMACH3 */
static inline int find_empty_process(void)
#endif	/* CONFIG_OSFMACH3 */
{
	int i;

#ifdef	CONFIG_OSFMACH3
	if (! system_task) {
		if (nr_tasks >= REAL_NR_TASKS - MIN_TASKS_LEFT_FOR_ROOT) {
			if (parent->uid || nr_tasks >= REAL_NR_TASKS)
				return -EAGAIN;
		}
	} else {
		/* forking a kernel thread or task */
		if (nr_tasks >= NR_TASKS) {
			return -EAGAIN;
		}
	}
#else	/* CONFIG_OSFMACH3 */
	if (nr_tasks >= NR_TASKS - MIN_TASKS_LEFT_FOR_ROOT) {
		if (current->uid)
			return -EAGAIN;
	}
#endif	/* CONFIG_OSFMACH3 */
#ifdef	CONFIG_OSFMACH3
	if (parent->uid) {
		long max_tasks = parent->rlim[RLIMIT_NPROC].rlim_cur;
#else	/* CONFIG_OSFMACH3 */
	if (current->uid) {
		long max_tasks = current->rlim[RLIMIT_NPROC].rlim_cur;
#endif	/* CONFIG_OSFMACH3 */

		max_tasks--;	/* count the new process.. */
		if (max_tasks < nr_tasks) {
			struct task_struct *p;
			for_each_task (p) {
#ifdef	CONFIG_OSFMACH3
				if (p->uid == parent->uid)
#else	/* CONFIG_OSFMACH3 */
				if (p->uid == current->uid)
#endif	/* CONFIG_OSFMACH3 */
					if (--max_tasks < 0)
						return -EAGAIN;
			}
		}
	}
	for (i = 0 ; i < NR_TASKS ; i++) {
		if (!task[i])
			return i;
	}
	return -EAGAIN;
}

#ifdef	CONFIG_OSFMACH3
static int get_pid(struct task_struct *parent, unsigned long flags)
#else	/* CONFIG_OSFMACH3 */
static int get_pid(unsigned long flags)
#endif	/* CONFIG_OSFMACH3 */
{
	struct task_struct *p;

	if (flags & CLONE_PID)
#ifdef	CONFIG_OSFMACH3
		return parent->pid;
#else	/* CONFIG_OSFMACH3 */
		return current->pid;
#endif	/* CONFIG_OSFMACH3 */
repeat:
	if ((++last_pid) & 0xffff8000)
		last_pid=1;
	for_each_task (p) {
		if (p->pid == last_pid ||
		    p->pgrp == last_pid ||
		    p->session == last_pid)
			goto repeat;
	}
	return last_pid;
}

#ifdef	CONFIG_OSFMACH3
static inline int dup_mmap(struct task_struct *parent, struct mm_struct * mm)
#else	/* CONFIG_OSFMACH3 */
static inline int dup_mmap(struct mm_struct * mm)
#endif	/* CONFIG_OSFMACH3 */
{
	struct vm_area_struct * mpnt, **p, *tmp;

	mm->mmap = NULL;
	p = &mm->mmap;
#ifdef	CONFIG_OSFMACH3
	for (mpnt = parent->mm->mmap ; mpnt ; mpnt = mpnt->vm_next)
#else	/* CONFIG_OSFMACH3 */
	for (mpnt = current->mm->mmap ; mpnt ; mpnt = mpnt->vm_next)
#endif	/* CONFIG_OSFMACH3 */
	{
		tmp = (struct vm_area_struct *) kmalloc(sizeof(struct vm_area_struct), GFP_KERNEL);
		if (!tmp) {
			/* exit_mmap is called by the caller */
			return -ENOMEM;
		}
		*tmp = *mpnt;
		tmp->vm_flags &= ~VM_LOCKED;
		tmp->vm_mm = mm;
		tmp->vm_next = NULL;
		if (tmp->vm_inode) {
			tmp->vm_inode->i_count++;
#ifdef	CONFIG_OSFMACH3
			ASSERT(tmp->vm_inode->i_mem_object);
			(void) inode_pager_setup(tmp->vm_inode);
#endif	/* CONFIG_OSFMACH3 */
			/* insert tmp into the share list, just after mpnt */
			tmp->vm_next_share->vm_prev_share = tmp;
			mpnt->vm_next_share = tmp;
			tmp->vm_prev_share = mpnt;
		}
#ifndef	CONFIG_OSFMACH3
#ifdef	CONFIG_OSFMACH3
		if (copy_page_range(mm, parent->mm, tmp))
#else	/* CONFIG_OSFMACH3 */
		if (copy_page_range(mm, current->mm, tmp))
#endif	/* CONFIG_OSFMACH3 */
		{
			/* link into the linked list for exit_mmap */
			*p = tmp;  
			p = &tmp->vm_next;
			/* exit_mmap is called by the caller */
			return -ENOMEM;
		}
#endif	/* CONFIG_OSFMACH3 */
		if (tmp->vm_ops && tmp->vm_ops->open)
			tmp->vm_ops->open(tmp);
		*p = tmp;
		p = &tmp->vm_next;
	}
	build_mmap_avl(mm);
#ifdef	CONFIG_OSFMACH3
	flush_tlb_mm(parent->mm);
#else	/* CONFIG_OSFMACH3 */
	flush_tlb_mm(current->mm);
#endif	/* CONFIG_OSFMACH3 */
	return 0;
}

static inline int copy_mm(unsigned long clone_flags, struct task_struct * tsk)
{
#ifdef	CONFIG_OSFMACH3
	struct task_struct *parent = tsk->p_opptr;
#endif	/* CONFIG_OSFMACH3 */

	if (!(clone_flags & CLONE_VM)) {
		struct mm_struct * mm = kmalloc(sizeof(*tsk->mm), GFP_KERNEL);
		if (!mm)
			return -ENOMEM;
#ifdef	CONFIG_OSFMACH3
		*mm = *parent->mm;
#else	/* CONFIG_OSFMACH3 */
		*mm = *current->mm;
#endif	/* CONFIG_OSFMACH3 */
		mm->count = 1;
		mm->def_flags = 0;
		mm->mmap_sem = MUTEX;
		tsk->mm = mm;
		tsk->min_flt = tsk->maj_flt = 0;
		tsk->cmin_flt = tsk->cmaj_flt = 0;
		tsk->nswap = tsk->cnswap = 0;
#ifdef	CONFIG_OSFMACH3
		/* this will be set in copy_thread() */
		tsk->mm->mm_mach_task = NULL;
#else	/* CONFIG_OSFMACH3 */
		if (new_page_tables(tsk)) {
			tsk->mm = NULL;
			exit_mmap(mm);
			goto free_mm;
		}
#endif	/* CONFIG_OSFMACH3 */
#ifdef	CONFIG_OSFMACH3
		if (dup_mmap(parent, mm))
#else	/* CONFIG_OSFMACH3 */
		if (dup_mmap(mm))
#endif	/* CONFIG_OSFMACH3 */
		{
			tsk->mm = NULL;
			exit_mmap(mm);
#ifndef	CONFIG_OSFMACH3
			free_page_tables(mm);
free_mm:
#endif	/* CONFIG_OSFMACH3 */
			kfree(mm);
			return -ENOMEM;
		}
		return 0;
	}
#ifndef	CONFIG_OSFMACH3
#ifdef	CONFIG_OSFMACH3
	SET_PAGE_DIR(tsk, parent->mm->pgd);
#else	/* CONFIG_OSFMACH3 */
	SET_PAGE_DIR(tsk, current->mm->pgd);
#endif	/* CONFIG_OSFMACH3 */
#endif	/* CONFIG_OSFMACH3 */
#ifdef	CONFIG_OSFMACH3
	parent->mm->count++;
#else	/* CONFIG_OSFMACH3 */
	current->mm->count++;
#endif	/* CONFIG_OSFMACH3 */
	return 0;
}

static inline int copy_fs(unsigned long clone_flags, struct task_struct * tsk)
{
#ifdef	CONFIG_OSFMACH3
	struct task_struct *parent = tsk->p_opptr;
#endif	/* CONFIG_OSFMACH3 */

	if (clone_flags & CLONE_FS) {
#ifdef	CONFIG_OSFMACH3
		parent->fs->count++;
#else	/* CONFIG_OSFMACH3 */
		current->fs->count++;
#endif	/* CONFIG_OSFMACH3 */
		return 0;
	}
	tsk->fs = kmalloc(sizeof(*tsk->fs), GFP_KERNEL);
	if (!tsk->fs)
		return -1;
	tsk->fs->count = 1;
#ifdef	CONFIG_OSFMACH3
	tsk->fs->umask = parent->fs->umask;
	if ((tsk->fs->root = parent->fs->root))
		tsk->fs->root->i_count++;
	if ((tsk->fs->pwd = parent->fs->pwd))
		tsk->fs->pwd->i_count++;
#else	/* CONFIG_OSFMACH3 */
	tsk->fs->umask = current->fs->umask;
	if ((tsk->fs->root = current->fs->root))
		tsk->fs->root->i_count++;
	if ((tsk->fs->pwd = current->fs->pwd))
		tsk->fs->pwd->i_count++;
#endif	/* CONFIG_OSFMACH3 */
	return 0;
}

static inline int copy_files(unsigned long clone_flags, struct task_struct * tsk)
{
	int i;
	struct files_struct *oldf, *newf;
	struct file **old_fds, **new_fds;
#ifdef	CONFIG_OSFMACH3
	struct task_struct *parent = tsk->p_opptr;
#endif	/* CONFIG_OSFMACH3 */

#ifdef	CONFIG_OSFMACH3
	oldf = parent->files;
#else	/* CONFIG_OSFMACH3 */
	oldf = current->files;
#endif	/* CONFIG_OSFMACH3 */
	if (clone_flags & CLONE_FILES) {
		oldf->count++;
		return 0;
	}

	newf = kmalloc(sizeof(*newf), GFP_KERNEL);
	tsk->files = newf;
	if (!newf)
		return -1;
			
	newf->count = 1;
	newf->close_on_exec = oldf->close_on_exec;
	newf->open_fds = oldf->open_fds;

	old_fds = oldf->fd;
	new_fds = newf->fd;
	for (i = NR_OPEN; i != 0; i--) {
		struct file * f = *old_fds;
		old_fds++;
		*new_fds = f;
		new_fds++;
		if (f)
			f->f_count++;
	}
	return 0;
}

static inline int copy_sighand(unsigned long clone_flags, struct task_struct * tsk)
{
#ifdef	CONFIG_OSFMACH3
	struct task_struct *parent = tsk->p_opptr;
#endif	/* CONFIG_OSFMACH3 */

	if (clone_flags & CLONE_SIGHAND) {
#ifdef	CONFIG_OSFMACH3
		parent->sig->count++;
#else	/* CONFIG_OSFMACH3 */
		current->sig->count++;
#endif	/* CONFIG_OSFMACH3 */
		return 0;
	}
	tsk->sig = kmalloc(sizeof(*tsk->sig), GFP_KERNEL);
	if (!tsk->sig)
		return -1;
	tsk->sig->count = 1;
#ifdef	CONFIG_OSFMACH3
	memcpy(tsk->sig->action, parent->sig->action, sizeof(tsk->sig->action));
#else	/* CONFIG_OFSMACH3 */
	memcpy(tsk->sig->action, current->sig->action, sizeof(tsk->sig->action));
#endif	/* CONFIG_OSFMACH3 */
	return 0;
}

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in its entirety.
 */
#ifdef	CONFIG_OSFMACH3
int do_fork(unsigned long clone_flags, unsigned long usp, struct pt_regs *regs)
{
	return osfmach3_do_fork(current, clone_flags, usp, regs);
}

int osfmach3_do_fork(
	struct task_struct *parent,
	unsigned long clone_flags,
	unsigned long usp,
	struct pt_regs *regs)
#else	/* CONFIG_OSFMACH3 */
int do_fork(unsigned long clone_flags, unsigned long usp, struct pt_regs *regs)
#endif	/* CONFIG_OSFMACH3 */
{
	int nr;
	int error = -ENOMEM;
	unsigned long new_stack;
	struct task_struct *p;

#ifdef	CONFIG_OSFMACH3
	/*
	 * Either it's a synchronous fork (from the "current" task) or
	 * we have an extra reference on the Linux task to make sure
	 * it won't exit during the fork.
	 */
	ASSERT(parent == current ||
	       parent->osfmach3.thread->mach_thread_count > 1);
#endif	/* CONFIG_OSFMACH3 */

	p = (struct task_struct *) kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		goto bad_fork;
#ifdef	CONFIG_OSFMACH3
	/* we don't need a kernel stack, since we're not a kernel ! */
	new_stack = 0;
#else	/* CONFIG_OSFMACH3 */
	new_stack = alloc_kernel_stack();
	if (!new_stack)
		goto bad_fork_free_p;
#endif	/* CONFIG_OSFMACH3 */
	error = -EAGAIN;
#ifdef	CONFIG_OSFMACH3
	nr = find_empty_process(parent, clone_flags & CLONING_KERNEL);
#else	/* CONFIG_OSFMACH3 */
	nr = find_empty_process();
#endif	/* CONFIG_OSFMACH3 */
	if (nr < 0)
		goto bad_fork_free_stack;

#ifdef	CONFIG_OSFMACH3
	*p = *parent;
#else	/* CONFIG_OSFMACH3 */
	*p = *current;
#endif	/* CONFIG_OSFMACH3 */

	if (p->exec_domain && p->exec_domain->use_count)
		(*p->exec_domain->use_count)++;
	if (p->binfmt && p->binfmt->use_count)
		(*p->binfmt->use_count)++;

	p->did_exec = 0;
	p->swappable = 0;
	p->kernel_stack_page = new_stack;
#ifndef	CONFIG_OSFMACH3
	*(unsigned long *) p->kernel_stack_page = STACK_MAGIC;
#endif	/* CONFIG_OSFMACH3 */
	p->state = TASK_UNINTERRUPTIBLE;
	p->flags &= ~(PF_PTRACED|PF_TRACESYS|PF_SUPERPRIV);
	p->flags |= PF_FORKNOEXEC;
#ifdef	CONFIG_OSFMACH3
	p->pid = get_pid(parent, clone_flags);
#else	/* CONFIG_OSFMACH3 */
	p->pid = get_pid(clone_flags);
#endif	/* CONFIG_OSFMACH3 */
	p->next_run = NULL;
	p->prev_run = NULL;
#ifdef	CONFIG_OSFMACH3
	p->p_pptr = p->p_opptr = parent;
#else	/* CONFIG_OSFMACH3 */
	p->p_pptr = p->p_opptr = current;
#endif	/* CONFIG_OSFMACH3 */
	p->p_cptr = NULL;
	init_waitqueue(&p->wait_chldexit);
	p->signal = 0;
	p->it_real_value = p->it_virt_value = p->it_prof_value = 0;
	p->it_real_incr = p->it_virt_incr = p->it_prof_incr = 0;
	init_timer(&p->real_timer);
	p->real_timer.data = (unsigned long) p;
	p->leader = 0;		/* session leadership doesn't inherit */
	p->tty_old_pgrp = 0;
	p->utime = p->stime = 0;
	p->cutime = p->cstime = 0;
#ifdef __SMP__
	p->processor = NO_PROC_ID;
	p->lock_depth = 1;
#endif
	p->start_time = jiffies;
	task[nr] = p;
	SET_LINKS(p);
	nr_tasks++;

	error = -ENOMEM;
	/* copy all the process information */
	if (copy_files(clone_flags, p))
		goto bad_fork_cleanup;
	if (copy_fs(clone_flags, p))
		goto bad_fork_cleanup_files;
	if (copy_sighand(clone_flags, p))
		goto bad_fork_cleanup_fs;
	if (copy_mm(clone_flags, p))
		goto bad_fork_cleanup_sighand;
	copy_thread(nr, clone_flags, usp, p, regs);
	p->semundo = NULL;

	/* ok, now we should be set up.. */
	p->swappable = 1;
	p->exit_signal = clone_flags & CSIGNAL;
#ifdef	CONFIG_OSFMACH3
	p->counter = (parent->counter >>= 1);
#else	/* CONFIG_OSFMACH3 */
	p->counter = (current->counter >>= 1);
#endif	/* CONFIG_OSFMACH3 */
	wake_up_process(p);			/* do this last, just in case */
#ifdef	CONFIG_OSFMACH3
	osfmach3_fork_resume(p, clone_flags);	/* let the user thread run */
#endif	/* CONFIG_OSFMACH3 */
	++total_forks;
	return p->pid;

bad_fork_cleanup_sighand:
	exit_sighand(p);
bad_fork_cleanup_fs:
	exit_fs(p);
bad_fork_cleanup_files:
	exit_files(p);
bad_fork_cleanup:
	if (p->exec_domain && p->exec_domain->use_count)
		(*p->exec_domain->use_count)--;
	if (p->binfmt && p->binfmt->use_count)
		(*p->binfmt->use_count)--;
#ifdef	CONFIG_OSFMACH3
	osfmach3_fork_cleanup(p, clone_flags);	/* cleanup Mach resources */
#endif	/* CONFIG_OSFMACH3 */
	task[nr] = NULL;
	REMOVE_LINKS(p);
	nr_tasks--;
bad_fork_free_stack:
#ifndef	CONFIG_OSFMACH3
	free_kernel_stack(new_stack);
bad_fork_free_p:
#endif	/* CONFIG_OSFMACH3 */
	kfree(p);
bad_fork:
	return error;
}
