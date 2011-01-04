/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _OSFMACH3_SCHED_H
#define _OSFMACH3_SCHED_H

#include <mach/port.h>
#include <mach/exception.h>
#include <cthreads.h>
typedef struct task_struct *task_struct_t;
typedef struct osfmach3_mach_task_struct *osfmach3_mach_task_struct_t;
#include <osfmach3/user_memory.h>
#include <osfmach3/segment.h>

#include <linux/time.h>

#include <asm/ptrace.h>

struct osfmach3_mach_task_struct {
	int		mach_task_count;	/* ref count for clone() */
	mach_port_t	mach_task_port;		/* handle on the Mach task */
	boolean_t	mach_aware;		/* task uses Mach services */
	struct user_memory *user_memory;	/* user memory map */
	struct user_memory *um_hint1;
	struct user_memory *um_hint2;
};
#define INIT_OSFMACH3_TASK						      \
{									      \
	1,					/* mach_task_count */	      \
	MACH_PORT_NULL,				/* mach_task_port */	      \
	FALSE,					/* mach_aware */	      \
	NULL,					/* user_memory */	      \
	NULL,					/* um_hint1 */		      \
	NULL,					/* um_hint2 */		      \
}

struct osfmach3_mach_thread_struct {
	int		mach_thread_count;	/* ref count for async ops */
	mach_port_t	mach_thread_port;	/* handle on the Mach thread */
	mach_port_t	mach_trap_port;		/* exception port */
	unsigned long	mach_trap_port_srights;	/* # send rights on trap port */
	struct pt_regs	*regs_ptr;		/* pointer to user regs */
	struct pt_regs	regs;			/* user regs (sometimes) */
	unsigned long	trap_no;
	unsigned long	error_code;
	unsigned long	fault_address;
	unsigned long	reg_fs;			/* dummy fs segment register */
	cthread_t	active_on_cthread;	/* cthread running this task */
	boolean_t	under_server_control;
	boolean_t	in_interrupt_list;
	int		fake_interrupt_count;
	boolean_t	in_fake_interrupt;	/* currently dealing with one */
	queue_chain_t	interrupt_list;
	boolean_t	exception_completed;	/* exc processed externally */
	struct condition mach_wait_channel;	/* to block the Mach thread */
};
#define INIT_OSFMACH3_THREAD						      \
{									      \
	1,					/* count */		      \
	MACH_PORT_NULL,				/* mach_thread_port */	      \
	MACH_PORT_NULL,				/* mach_trap_port */	      \
	0,					/* mach_trap_port_srights */  \
	&init_osfmach3_thread.regs,		/* regs_ptr */		      \
	INIT_PTREGS,				/* regs */		      \
	0,					/* trap_no */		      \
	0,					/* error_code */	      \
	0,					/* fault_address */	      \
	USER_DS,				/* reg_fs */		      \
	CTHREAD_NULL,				/* active_on_cthread */	      \
	FALSE,					/* under_server_control */    \
	FALSE,					/* in_interrupt_list */	      \
	0,					/* fake_interrupt_count */    \
	FALSE,					/* in_fake_interrupt */	      \
	{ &init_osfmach3_thread.interrupt_list,			 	      \
	  &init_osfmach3_thread.interrupt_list }, /* interrupt_list */        \
	FALSE,					/* exception_completed */     \
	CONDITION_INITIALIZER			/* mach_wait_channel */	      \
}

struct osfmach3_task_struct {
	struct osfmach3_mach_task_struct	*task;
	struct osfmach3_mach_thread_struct	*thread;
};

#define INIT_OSFMACH3 \
{ \
	&init_osfmach3_task, \
	&init_osfmach3_thread \
},

extern void osfmach3_setrun(struct task_struct *tsk);
extern void osfmach3_yield(void);

extern int osfmach3_do_fork(struct task_struct *parent,
			    unsigned long clone_flags,
			    unsigned long usp,
			    struct pt_regs *regs);
extern void osfmach3_synchronize_exit(struct task_struct *tsk);
extern void osfmach3_terminate_task(struct task_struct *tsk);
extern void osfmach3_update_thread_info(struct task_struct *tsk);
extern void osfmach3_update_task_info(struct task_struct *tsk);
extern void osfmach3_fork_resume(struct task_struct *p,
				 unsigned long clone_flags);
extern void osfmach3_fork_cleanup(struct task_struct *p,
				  unsigned long clone_flags);
extern void osfmach3_set_priority(struct task_struct *tsk);

extern void osfmach3_trap_init(exception_behavior_t behavior,
			       thread_state_flavor_t flavor);
extern void osfmach3_trap_terminate_task(struct task_struct *tsk);
extern void osfmach3_trap_setup_task(struct task_struct *task,
				     exception_behavior_t behavior,
				     thread_state_flavor_t flavor);
extern void osfmach3_trap_mach_aware(struct task_struct *task,
				     boolean_t mach_aware,
				     exception_behavior_t behavior,
				     thread_state_flavor_t flavor);
extern void osfmach3_trap_forward(exception_type_t exc_type,
				  exception_data_t code,
				  mach_msg_type_number_t code_count,
				  int *flavor,
				  thread_state_t old_state,
				  mach_msg_type_number_t *icnt,
				  thread_state_t new_state,
				  mach_msg_type_number_t *ocnt);
extern kern_return_t osfmach3_trap_unwind(mach_port_t trap_port,
					  exception_type_t exc_type,
					  exception_data_t code,
					  mach_msg_type_number_t code_count,
					  int *flavor,
					  thread_state_t old_state,
					  mach_msg_type_number_t icnt,
					  thread_state_t new_state,
					  mach_msg_type_number_t *ocnt);
extern void osfmach3_notify_register(struct task_struct *tsk);
extern void osfmach3_notify_deregister(struct task_struct *tsk);

extern void osfmach3_update_vm_info(void);
extern void osfmach3_update_load_info(void);
extern void osfmach3_update_cpu_load_info(void);
extern void osfmach3_get_time(struct timeval *xtimep);
extern void osfmach3_set_time(struct timeval *xtimep);

#endif	/* _OSFMACH3_SCHED_H */
