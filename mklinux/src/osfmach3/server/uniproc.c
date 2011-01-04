/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <osfmach3/uniproc.h>
#include <osfmach3/assert.h>
#include <osfmach3/server_thread.h>

#include <linux/sched.h>

#define UNIPROC_PREEMPTION 0
#if	UNIPROC_PREEMPTION
int			uniproc_allow_preemption = FALSE;
#endif	/* UNIPROC_PREEMPTION */

boolean_t		use_antechamber_mutex = TRUE;
struct mutex		uniproc_antechamber_mutex = MUTEX_INITIALIZER;
struct mutex		uniproc_mutex = MUTEX_INITIALIZER;
cthread_t		uniproc_holder_cthread = NULL;
#if	UNIPROC_PREEMPTION
struct mutex		uniproc_preemption_mutex = MUTEX_INITIALIZER;
#endif	/* UNIPROC_PREEMPTION */

#if	CONFIG_OSFMACH3_DEBUG

struct task_struct	*uniproc_holder = NULL;

unsigned long	uniproc_preemptibles = 0;	/* #times became preemptible */
unsigned long	uniproc_preemptions = 0;	/* #times preemption occured */

#if defined(__STDC__)
#define UNIPROC_ASSERT(ex)						\
MACRO_BEGIN								\
	if (!(ex)) {							\
		printk("Assertion failed: file: \"%s\", line: %d test: %s\n", \
		       __FILE__, __LINE__, # ex );			\
		Debugger("assertion failure");				\
	}								\
MACRO_END
#else	/* __STDC__ */
#define UNIPROC_ASSERT(ex)						\
MACRO_BEGIN								\
	if (!(ex)) {							\
		printk("Assertion failed: file: \"%s\", line: %d test: %s\n", \
		       __FILE__, __LINE__, "ex");			\
		Debugger("assertion failure");				\
	}								\
MACRO_END
#endif	/* __STDC__ */

struct task_struct *
get_current_task(void)
{
	struct server_thread_priv_data	*priv_datap;
	struct task_struct		*current_task;

	priv_datap = server_thread_get_priv_data(cthread_self());
	current_task = priv_datap->current_task;
	if (current_task->osfmach3.thread != init_task.osfmach3.thread) {
		ASSERT(current_task->osfmach3.thread->active_on_cthread
		       == cthread_self());
	}
	return current_task;
}

void
set_current_task(
	struct task_struct *current_task)
{
	struct server_thread_priv_data	*priv_datap;
	struct task_struct		*old_current_task;

	priv_datap = server_thread_get_priv_data(cthread_self());

	old_current_task = priv_datap->current_task;
	if (old_current_task->osfmach3.thread != init_task.osfmach3.thread) {
		ASSERT(old_current_task->osfmach3.thread->active_on_cthread
		       == cthread_self());
		old_current_task->osfmach3.thread->active_on_cthread = 
			CTHREAD_NULL;
	}
	ASSERT(current_task->osfmach3.thread->active_on_cthread == CTHREAD_NULL);
	if (current_task->osfmach3.thread != init_task.osfmach3.thread) {
		current_task->osfmach3.thread->active_on_cthread = cthread_self();
	}

	priv_datap->current_task = current_task;
}

void
uniproc_has_entered(void)
{
	UNIPROC_ASSERT(uniproc_holder == NULL);
	UNIPROC_ASSERT(uniproc_holder_cthread == NULL);
	current_set[smp_processor_id()] = get_current_task();
#if	CONFIG_OSFMACH3_DEBUG
	uniproc_holder = current;
#endif	/* CONFIG_OSFMACH3_DEBUG */
	uniproc_holder_cthread = cthread_self();
}

void
uniproc_enter(void)
{
	struct server_thread_priv_data	*priv_datap;

#if	UNIPROC_PREEMPTION
	if (!mutex_try_lock(&uniproc_mutex)) {
		priv_datap = server_thread_get_priv_data(cthread_self());
		if (priv_datap->preemptive && uniproc_allow_preemption) {
			/* prioritary thread: try to preempt */
			while (!uniproc_try_enter() &&
			       !uniproc_preempt()) {
				server_thread_yield(1);	/* yield for 1 ms */
			}
			/* we're all set */
			return;
		} else {
			/* just block and wait for our turn */
			mutex_lock(&uniproc_mutex);
		}
	}
#else	/* UNIPROC_PREEMPTION */
	if (use_antechamber_mutex) {
		priv_datap = server_thread_get_priv_data(cthread_self());
		if (priv_datap->current_task &&
		    priv_datap->current_task->mm == &init_mm) {
			/*
			 * We're working for a system thread: carry on.
			 */
			mutex_lock(&uniproc_mutex);
		} else {
			/*
			 * We're working for a user thread: we don't want
			 * loads of user threads contending with system
			 * threads on the uniproc mutex, so take the
			 * uniproc_antechamber_mutex first to ensure that
			 * only one user thread will be contending with
			 * system threads at a given time.
			 */
			mutex_lock(&uniproc_antechamber_mutex);
			mutex_lock(&uniproc_mutex);
			mutex_unlock(&uniproc_antechamber_mutex);
		}
	} else {
		mutex_lock(&uniproc_mutex);
	}
#endif	/* UNIPROC_PREEMPTION */
	uniproc_has_entered();
}

boolean_t
uniproc_try_enter(void)
{
	if (mutex_try_lock(&uniproc_mutex)) {
		uniproc_has_entered();
		return TRUE;
	}
	return FALSE;
}

void
uniproc_will_exit(void)
{
	UNIPROC_ASSERT(uniproc_holder == current);
	UNIPROC_ASSERT(uniproc_holder_cthread == cthread_self());
	set_current_task(current);
	current_set[smp_processor_id()] = (struct task_struct *) NULL;
#if	CONFIG_OSFMACH3_DEBUG
	uniproc_holder = NULL;
#endif	/* CONFIG_OSFMACH3_DEBUG */
	uniproc_holder_cthread = NULL;
}

void
uniproc_exit(void)
{
	uniproc_will_exit();
	mutex_unlock(&uniproc_mutex);
}

static __inline__ void
uniproc_change_current(
	struct task_struct *old_task,
	struct task_struct *new_task)
{
	UNIPROC_ASSERT(uniproc_holder != NULL);
	UNIPROC_ASSERT(uniproc_holder == old_task);
	ASSERT(current == old_task);
	current_set[smp_processor_id()] = new_task;
	set_current_task(current);
#if	CONFIG_OSFMACH3_DEBUG
	uniproc_holder = current;
#endif	/* CONFIG_OSFMACH3_DEBUG */
	uniproc_holder_cthread = cthread_self();
}
	
void
uniproc_switch_to(
	struct task_struct *old_task,
	struct task_struct *new_task)
{
	UNIPROC_ASSERT(uniproc_holder_cthread == cthread_self());
	ASSERT(old_task == FIRST_TASK || new_task == FIRST_TASK);
	uniproc_change_current(old_task, new_task);
}

#if	UNIPROC_PREEMPTION
boolean_t
uniproc_preempt(void)
{
	if (mutex_try_lock(&uniproc_preemption_mutex)) {
		uniproc_change_current(current, get_current_task());
#if	CONFIG_OSFMACH3_DEBUG
		{
			struct server_thread_priv_data	*priv_datap;

			priv_datap =
				server_thread_get_priv_data(cthread_self());
			if (priv_datap->preemptive) {
				/*
				 * We actually preempted another thread...
				 * account for this glorious deed !
				 */
				uniproc_preemptions++;
			} else {
				/*
				 * It's just the preemptible thread
				 * preempting itself
				 */
			}
		}
#endif	/* CONFIG_OSFMACH3_DEBUG */
		return TRUE;
	}
	return FALSE;
}
#endif	/* UNIPROC_PREEMPTION */

void
uniproc_preemptible(void)
{
#if	UNIPROC_PREEMPTION
	if (uniproc_allow_preemption) {
		UNIPROC_ASSERT(uniproc_holder == current);
		UNIPROC_ASSERT(uniproc_holder_cthread == cthread_self());
#if	CONFIG_OSFMACH3_DEBUG
		uniproc_preemptibles++;
#endif	/* CONFIG_OSFMACH3_DEBUG */
		mutex_unlock(&uniproc_preemption_mutex);
	} else {
		uniproc_exit();
	}
#else	/* UNIPROC_PREEMPTION */
	uniproc_exit();
#endif	/* UNIPROC_PREEMPTION */
}

void
uniproc_unpreemptible(void)
{
#if	UNIPROC_PREEMPTION
	if (uniproc_allow_preemption && uniproc_preempt()) {
		/*
		 * Nobody preempted us or we preempted someone else.
		 * Anyway, the uniproc is held now, so all is fine...
		 */
	} else {
		/*
		 * We got preempted and we can't get back on stage
		 * right now...
		 */
		uniproc_enter();
	}
#else	/* UNIPROC_PREEMPTION */
	uniproc_enter();
#endif	/* UNIPROC_PREEMPTION */
}

#else	/* CONFIG_OSFMACH3_DEBUG */

#define UNIPROC_ASSERT(ex)

#endif	/* CONFIG_OSFMACH3_DEBUG */



void
uniproc_preemption_init(void)
{
#if	UNIPROC_PREEMPTION
	mutex_lock(&uniproc_preemption_mutex);
#endif	/* UNIPROC_PREEMPTION */
}

boolean_t
holding_uniproc(void)
{
#ifdef	CONFIG_OSFMACH3_DEBUG
	if (uniproc_holder_cthread != cthread_self())
		return FALSE;
	else
#endif	/* CONFIG_OSFMACH3_DEBUG */
		return TRUE;
}
