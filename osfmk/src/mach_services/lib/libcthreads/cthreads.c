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
/* 
 * Mach Operating System
 * Copyright (c) 1993,1992,1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * 	File:	cthreads.c
 *	Author:	Eric Cooper, Carnegie Mellon University
 *	Date:	July, 1987
 *
 * 	Implementation of fork, join, exit, etc.
 */
/*
 * File: cthreads.c
 *
 * This module is the core of the cthreads implementation.
 */

#include <cthreads.h>
#include <cthread_filter.h>
#include "cthread_internals.h"
#include <mach/mach_traps.h>
#include <mach/mach_host.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef THREAD_NULL
#define THREAD_NULL MACH_PORT_NULL
#endif /*THREAD_NULL*/

vm_size_t cthread_wait_stack_size = 0;

/* Static data
 * 
 *    cthreads_started - Has cthread_init been called?
 */
private	boolean_t		cthreads_started = FALSE;

/*
 * Prototypes for local functions.
 */
staticf vm_offset_t	cthread_init_newstack(void);
staticf void		cthread_init(vm_offset_t *);
staticf cthread_t	cthread_create(cthread_fn_t, void *);
staticf cthread_t	cthread_alloc(void);
staticf void		cthread_body(cthread_t);
staticf void		cthread_wakeup(cthread_t);  
staticf void		cthread_block(cthread_t, cthread_fn_t, void *,
				      cthread_t);
staticf void 		cthread_join_real(cthread_t);
staticf	void		cthread_yield_real(void);
staticf void		cthread_return(cthread_t);
staticf void		cthread_idle(cthread_t);
staticf int		cthread_cpus(void);
staticf	void		cond_wait(condition_t, mutex_t);
staticf	void		mutex_lock_real(mutex_t);
staticf void		cthread_idle_remove(cthread_t);
staticf void		cthread_idle_enter(cthread_t);
staticf cthread_t	cthread_idle_first(void);
staticf void 		cthread_block_wired(cthread_t, cthread_fn_t,
					    void *, cthread_t);

#define	cthread_idle_init()	(cthread_status.waiting_dlq.next = cthread_status.waiting_dlq.prev = &cthread_status.waiting_dlq)

staticf cthread_t
cthread_idle_first(void)
{
	cthread_dlq_entry_t next = cthread_status.waiting_dlq.next;

	if (next == &cthread_status.waiting_dlq) {
		return (CTHREAD_NULL);
	} else {
		cthread_idle_remove((cthread_t)next);
		return (cthread_t)(next);
	}
}

staticf void
cthread_idle_enter(cthread_t p)
{
	register cthread_dlq_entry_t prev, head;

	head = &cthread_status.waiting_dlq;
	prev = head->prev;
	if (head == prev) {
		head->next = (cthread_dlq_entry_t) p;
	} else {
		((cthread_t)prev)->dlq.next = (cthread_dlq_entry_t)p;
	}
	p->dlq.prev = prev;
	p->dlq.next = head;
	head->prev = (cthread_dlq_entry_t)p;
}

staticf void
cthread_idle_remove(cthread_t p)
{
	register cthread_dlq_entry_t	next, prev, head;

	head = &cthread_status.waiting_dlq;
	next = p->dlq.next;
	prev = p->dlq.prev;

	if (head == next)
	    head->prev = prev;
	else
	    ((cthread_t)next)->dlq.prev = prev;

	if (head == prev)
	    head->next = next;
	else
	    ((cthread_t)prev)->dlq.next = next;
	p->dlq.next = (cthread_dlq_entry_t)0;
}

/*
 * Used for automatic initialization and termination by crt0.
 */
vm_offset_t	(*_thread_init_routine)(void) = cthread_init_newstack;
vm_offset_t	(*_threadlib_init_routine)(void) = cthread_init_newstack;
void		(*_thread_exit_routine)(void *) = cthread_exit;

#ifdef STATISTICS
extern struct cthread_statistics_struct cthread_stats;
#endif /*STATISTICS*/

#ifdef TRLOG
#define	TRACE_MAX	(4 * 1024)
#define	TRACE_WINDOW	40

typedef struct cthreads_trace_event {
	const char	*funcname;
	const char	*file;
	unsigned int	lineno;
	const char	*fmt;
	unsigned int	tag1;
	unsigned int	tag2;
	unsigned int	tag3;
	unsigned int	tag4;
	void		*sp;
} cthreads_trace_event;

struct cthreads_tr_struct {
    cthreads_trace_event	trace_buffer[TRACE_MAX];
    unsigned long	trace_index;
} cthreads_tr_data;

unsigned int cthreads_tr_print_now = 0;
#define CLMS 80*4096
char cthreads_log[CLMS];
static int cthreads_log_ptr=0;

static void
cthreads_print_tr(
      cthreads_trace_event *cte, 
      int ti, 
      unsigned long show_extra)
{
    const char *filename, *cp;
    if (cte->file == (char *) 0 || cte->funcname == (char *) 0 ||
	cte->lineno == 0 || cte->fmt == 0) {
	printf("[%04x]\n", ti);
	return;
    }
    filename = cte->file;
    for (cp = cte->file; *cp; ++cp)
	if (*cp == '/')
	    filename = cp + 1;
    printf("[%8x][%04x] %s", (unsigned int) cte->sp,ti, cte->funcname);
    if (show_extra) {
	printf("(%s:%05d):\n\t", filename, cte->lineno);
    } else
	printf(":  ");
    printf(cte->fmt, cte->tag1, cte->tag2, cte->tag3, cte->tag4);
    printf("\n");
}

static void
cthreads_sprint_tr(
      cthreads_trace_event *cte, 
      int ti, 
      unsigned long show_extra)
{
    const char *filename, *cp;
    static spin_lock_t  lock = SPIN_LOCK_INITIALIZER;
    spin_lock(&lock);
    if (cte->file == (char *) 0 || cte->funcname == (char *) 0 ||
	cte->lineno == 0 || cte->fmt == 0) {
	sprintf(&cthreads_log[cthreads_log_ptr],"[%04x]\n", ti);
	while(cthreads_log[cthreads_log_ptr]!='\0')
	    cthreads_log_ptr++;
	spin_unlock(&lock);
	return;
    }
    filename = cte->file;
    for (cp = cte->file; *cp; ++cp)
	if (*cp == '/')
	    filename = cp + 1;
    sprintf(&cthreads_log[cthreads_log_ptr],
	    "[%8x][%04x] %s", (unsigned int) cte->sp,ti, cte->funcname);
    while(cthreads_log[cthreads_log_ptr]!='\0')
	cthreads_log_ptr++;
    if (show_extra)
	sprintf(&cthreads_log[cthreads_log_ptr],
		"(%s:%05d):\n\t", filename, cte->lineno);
    else
	sprintf(&cthreads_log[cthreads_log_ptr], ":  ");
    while(cthreads_log[cthreads_log_ptr]!='\0')
	cthreads_log_ptr++;
    sprintf(&cthreads_log[cthreads_log_ptr],
	    cte->fmt, cte->tag1, cte->tag2, cte->tag3, cte->tag4);
    while(cthreads_log[cthreads_log_ptr]!='\0')
	cthreads_log_ptr++;
    sprintf(&cthreads_log[cthreads_log_ptr],
	    "\n");
    while(cthreads_log[cthreads_log_ptr]!='\0')
	cthreads_log_ptr++;
    if (cthreads_log_ptr >= CLMS-200)
	cthreads_log_ptr = 0;
    spin_unlock(&lock);
}

static void 
tr(const char *funcname, const char *file, unsigned int lineno,
   const char *fmt, unsigned int tag1, unsigned int tag2,
   unsigned int tag3, unsigned int tag4)
{
    static spin_lock_t tr_lock = SPIN_LOCK_INITIALIZER;
    struct cthreads_tr_struct *ctd = &cthreads_tr_data;
    if (ctd->trace_index >= TRACE_MAX) {
	ctd->trace_index = 0;
    }
    ctd->trace_buffer[ctd->trace_index].funcname = funcname;
    ctd->trace_buffer[ctd->trace_index].file = file;
    ctd->trace_buffer[ctd->trace_index].lineno = lineno;
    ctd->trace_buffer[ctd->trace_index].fmt = fmt;
    ctd->trace_buffer[ctd->trace_index].tag1 = tag1;
    ctd->trace_buffer[ctd->trace_index].tag2 = tag2;
    ctd->trace_buffer[ctd->trace_index].tag3 = tag3;
    ctd->trace_buffer[ctd->trace_index].tag4 = tag4;
    ctd->trace_buffer[ctd->trace_index].sp = cthread_sp();
    cthreads_sprint_tr(&(ctd->trace_buffer[ctd->trace_index]),
		      ctd->trace_index, 0);
    if (cthreads_tr_print_now) {
	if (spin_try_lock(&tr_lock)) {
	    cthreads_print_tr(&(ctd->trace_buffer[ctd->trace_index]),
			      ctd->trace_index, 0);
	    spin_unlock(&tr_lock);
	}
    }
    ++ctd->trace_index;
}

void cthreads_show_tr(struct cthreads_tr_struct *ctd, unsigned long index,
		 unsigned long range, unsigned long show_extra);

void
cthreads_show_tr(struct cthreads_tr_struct *ctd,
		 unsigned long index,
		 unsigned long range,
		 unsigned long show_extra)
{
	int		i;

	if (ctd == 0) {
	        ctd = &cthreads_tr_data;
		index = ctd->trace_index - (TRACE_WINDOW-4);
		range = TRACE_WINDOW;
		show_extra = 0;
	}
	if (index + range > TRACE_MAX)
		range = TRACE_MAX - index;
	for (i = index; i < index + range; ++i) {
	    cthreads_print_tr(&(ctd->trace_buffer[i]),i, show_extra);
	}
}
#define	TR_DECL(funcname)	const char *__ntr_func_name__ = funcname
#define	tr1(msg)							\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),0,0,0,0)
#define	tr2(msg,tag1)							\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),		\
	   (unsigned int) (tag1),0,0,0)
#define	tr3(msg,tag1,tag2)						\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),		\
	   (unsigned int) (tag1),(unsigned int) (tag2),0,0)
#define	tr4(msg,tag1,tag2,tag3)						\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),		\
	   (unsigned int) (tag1),(unsigned int) (tag2),(unsigned int) (tag3),0)
#define	tr5(msg,tag1,tag2,tag3,tag4)					\
	tr(__ntr_func_name__, __FILE__, __LINE__, (msg),		\
	   (unsigned int) (tag1),(unsigned int) (tag2),			\
	   (unsigned int) (tag3),(unsigned int) (tag4))
#else /*TRLOG*/
#define	TR_DECL(funcname)
#define tr1(msg)
#define tr2(msg, tag1)
#define tr3(msg, tag1, tag2)
#define tr4(msg, tag1, tag2, tag3)
#define tr5(msg, tag1, tag2, tag3, tag4)
#endif /*TRLOG*/

/*
 * Function: cthread_alloc - Allocate a cthread structure.
 *
 * Description:
 *	Allocate and initialize a cthread.
 *      Allocate space for cthread strucure and initialize.
 *      Ready_lock is held when called
 *
 * Arguments:
 *	None.
 *
 * Return Value:
 *	The allocated cthread is returned.  If there was not enough space to
 *	allocate one then CTHREAD_NULL is returned.
 *
 * Comments:
 *	Does not handle allocation failures from cthread_malloc().
 */

staticf cthread_t
cthread_alloc()
{
    cthread_t p = (cthread_t) cthread_malloc(sizeof(struct cthread));

    TR_DECL("cthread_alloc");
    tr1("enter");

    /* Initialize the new cthread structure. */
    memset(p, 0, sizeof(struct cthread));

    p->reply_port = MACH_PORT_NULL;
    p->wired = MACH_PORT_NULL;
    p->state = CTHREAD_RUNNING;
    p->list = cthread_status.cthread_list;

    cthread_status.cthread_list = p;
    cthread_status.alloc_cthreads++;
    p->cthread_status = &cthread_status;
    return p;
}


/*
 * Function: cthread_create
 *
 * Description:
 *	Create a new cthread.
 *
 * Arguments:
 *	func - The cthread's initial function to run.
 *	arg  - The argument to the initial function.
 *
 * Return Value:
 *	The created cthread is returned.
 *
 * Comments:
 *	The cthread lock is held on entry and released before exit.
 *	Does not handle allocation failures.
 */

staticf cthread_t
cthread_create(cthread_fn_t func, void *arg)
{
    cthread_t child;
    kern_return_t r;
    thread_port_t n;
    
    TR_DECL("cthread_create");
    tr3("func = %x arg = %x",(int)func, (int)arg);
    child = cthread_alloc();
    child->func = func;
    child->arg = arg;

    /*
     * Allocate the cthread stack.  The cthread lock is released because the
     * stack allocation does a kernel call.
     */
    cthread_unlock();
    alloc_stack(child);
    cthread_lock();

    /* Can we allocate another Mach thread?  If yes do so. */
    if ((cthread_status.max_kernel_threads == 0 ||
	cthread_status.kernel_threads < cthread_status.max_kernel_threads) &&
	(func != CTHREAD_NOFUNC)) {

	tr1("Creating new kernel thread");
	cthread_status.kernel_threads++;
	cthread_unlock();
	MACH_CALL(thread_create(mach_task_self(), &n), r);
	child->context = cthread_stack_base(child, CTHREAD_STACK_OFFSET);
	cthread_setup(child, n, (cthread_fn_t)cthread_body);
	MACH_CALL(thread_resume(n), r);
    } else {
        /*
         * No new kernel thread should be allocated, so see if there is a waiting,
         * idle thread.  If there is wake it up to run this cthread.
         */
	cthread_t waiter = CTHREAD_NULL;

	cthread_unlock();
	if (func == CTHREAD_NOFUNC) {
	    tr1("returning only handle");
	} else {
	    child->state = CTHREAD_BLOCKED|CTHREAD_RUNNABLE;
	    child->context = cthread_stack_base(child, CTHREAD_STACK_OFFSET);
	    CTHREAD_FILTER(child, CTHREAD_FILTER_PREPARE,
			   cthread_body, child, 0, 0);
	    cthread_lock();
	    cthread_queue_enq(&cthread_status.run_queue, child);
	    waiter = cthread_idle_first();
	    cthread_unlock();
	    cthread_wakeup(waiter);
	}
    }
    tr1("exit");
    return child;
}

/*
 * Function: cthread_idle
 *
 * Description:
 *	This is where idle Mach threads go to sleep waiting for cthreads to
 * become ready to run. Threads will be run until there is likely to be work
 * to do. 
 *
 * Arguments:
 *	p - The thread that will go idle.
 *
 * Return Value:
 *	None.
 *
 * Comments:
 *	This is only run on a partial "waiting" stack
 */

staticf void
cthread_idle(cthread_t p)
{
    cthread_t new = CTHREAD_NULL;
    int count;
    kern_return_t r;
    
    TR_DECL("cthread_idle");
    tr2("enter 0x%x", (int)p);
#ifdef STATISTICS
    stats_lock(&cthread_stats.lock);
    cthread_stats.waiters++;
    stats_unlock(&cthread_stats.lock);
#endif /*STATISTICS*/
    
    
    /*
     * While there is no work to do de-schedule the current cthread and wait.
     */
    do {
	if (cthread_queue_head(&cthread_status.run_queue, cthread_t) != 
	    CTHREAD_NULL)
	    goto got_one;
#ifdef STATISTICS
	stats_lock(&cthread_stats.lock);
	cthread_stats.idle_swtchs++;
	stats_unlock(&cthread_stats.lock);
#endif /*STATISTICS*/
	/* Make the current Mach thread idle. */
	p->flags |= CTHREAD_DEPRESS;
	if (!p->undepress) {
	    r = thread_switch(THREAD_NULL, SWITCH_OPTION_IDLE, 0);
	    if (r == KERN_SUCCESS) {
		while((cthread_queue_head(&cthread_status.run_queue,
		    cthread_t) == CTHREAD_NULL) && !(p->undepress));
		MACH_CALL(thread_depress_abort(p->wired),r);
	    }
	}
	p->flags &= ~CTHREAD_DEPRESS;
	p->undepress = FALSE;

	if (cthread_queue_head(&cthread_status.run_queue, cthread_t)
	    != CTHREAD_NULL) {
#ifdef SPIN_POLL_IDLE
	  got_one:
	    count = 0;
	    do {
		if (cthread_try_lock()) {
		    cthread_queue_deq(&cthread_status.run_queue,
				      cthread_t, new);
		    if (!new)
			cthread_unlock();
		}
	    } while  (!new && count++<cthread_status.lock_spin_count &&
		      cthread_queue_head(&cthread_status.run_queue, cthread_t)
		      != CTHREAD_NULL);
#ifdef STATISTICS
	    if (!new && count<cthread_status.lock_spin_count) {
		stats_lock(&cthread_stats.lock);
		cthread_stats.idle_lost++;
		stats_unlock(&cthread_stats.lock);
	    }
#endif /*STATISTICS*/
#else /*SPIN_POLL_IDLE*/

	    /*
	     * Attempt to run a cthread.  If there a no runnable cthreads,
	     * we will stay in the idle loop.
	     */
	  got_one:
	    cthread_lock();
	    cthread_queue_deq(&cthread_status.run_queue, cthread_t, new);

	    if (!new) {
		    if (p->dlq.next == (cthread_dlq_entry_t)0)
			cthread_idle_enter(p);
		    cthread_unlock();
	    }

#endif /*SPIN_POLL_IDLE*/
	}
	if ((!new) && p->dlq.next == (cthread_dlq_entry_t)0) {
		cthread_lock();
		cthread_idle_enter(p);
		cthread_unlock();
	}
    } while (!new);

    tr3("waiter 0x%x got %8x", (int)p, (int)new);
#ifdef STATISTICS
    cthread_stats.idle_exit++;
#endif /*STATISTICS*/


    /* cthread lock is held */

    /*
     * Put the waiting cthread (our current context) back on the waiter list
     * for future use.
     */
    p->wired = MACH_PORT_NULL;
    if (p->dlq.next)
	cthread_idle_remove(p);
    cthread_queue_enq(&cthread_status.waiters, p);
    p->state = CTHREAD_BLOCKED;

    /* Switch to the new cthread's context. */
    new->state = CTHREAD_RUNNING;
    CTHREAD_FILTER(p, CTHREAD_FILTER_INTERNAL_BUILD, cthread_idle, p, (void *)&cthread_status.run_lock, &new->context);
}

/*
 * Function: cthread_wakeup - Wake up a waiting cthread.
 *
 * Description:
 *	Attempt to wake up a waiting cthread.  If the cthread does not have an
 * associated Mach thread or is not waiting, then this function does nothing.
 * The waiting cthread has idle'd itself in the kernel.  In this case we abort
 * the idle and wait until we confirm that the waiter has continued.  We have
 * to wait because if we miss starting the waiter, it may never get started.
 *
 * Arguments:
 *	waiter - The cthread to wake up.
 *
 * Return Value:
 *	None.
 *
 * Comments:
 *	No locks are held on entry.  This means that race conditions need to
 *	be carefully avoided.
 */

staticf void
cthread_wakeup(cthread_t waiter)
{
    mach_port_t waiter_thread_port;
    kern_return_t r;

    TR_DECL("cthread_wakeup");

    /* If there is no one to wakeup, just return. */
    if (waiter == CTHREAD_NULL)
	return;

    /* If the waiter does not have a Mach thread, just return. */
    if ((waiter_thread_port = waiter->wired) == MACH_PORT_NULL) {
	    tr2("!waiter->wired 0x%x", (int)waiter);
	    return;
    }
    
    /*
     * If the waiter's priority is not depressed, just return.  This can
     * happen if a race occurred and someone else woke up the cthread.
     */
    if (waiter->undepress) {
	    tr2("waiter->undepress 0x%x", (int)waiter);
	    return;
    }

    /*
     * We have a waiting thread, with a depressed priority.  Abort the
     * depression.
     */
    waiter->undepress = TRUE;
    while (waiter->flags & CTHREAD_DEPRESS && waiter->undepress) {

	/* Attempt to re-start the waiter. */
        r = thread_depress_abort(waiter_thread_port);
	tr3("thread_depress_abort %x %x",waiter,    waiter_thread_port);
	if (r == KERN_SUCCESS){
	    tr1("tda: success");
	    return;
        }

	/*
	 * For some reason we couldn't wakeup the thread (possibly it has not
	 * yet gone idle) - switch out for a while and try again later.
	 */
	MACH_CALL(thread_switch(THREAD_NULL, SWITCH_OPTION_DEPRESS, 10), r);
    }
    tr2("!waiter->flags & CTHREAD_DEPRESS 0x%x", (int)waiter);
}

staticf void 
cthread_block_wired(cthread_t p, cthread_fn_t f, void *a, cthread_t wakeup)
{
	kern_return_t r;

	p->undepress = FALSE;
	cthread_unlock();
	cthread_wakeup(wakeup);
	while (p->state & CTHREAD_BLOCKED) {
	    if (p->undepress && (p->state & CTHREAD_BLOCKED))
		p->undepress = FALSE;
	    p->flags |= CTHREAD_DEPRESS;
	    if (!p->undepress) {
		r = thread_switch(THREAD_NULL, SWITCH_OPTION_IDLE, 0);
		if (r == KERN_SUCCESS) {
		    while((p->state & CTHREAD_BLOCKED) && !(p->undepress));
		    MACH_CALL(thread_depress_abort(p->wired),r);
		}
	    }
	    p->flags &= ~CTHREAD_DEPRESS;
	}
	CTHREAD_FILTER(p, CTHREAD_FILTER_INVOKE_SELF, f, a, 0, 0);

}

/*
 * Function: cthread_block - Block the current cthread.
 *
 * Description:
 *	Current cthread is blocked so switch to any ready cthreads, or, if
 *	none, go into the wait state.
 *
 * Arguments:
 *
 * Return Value:
 *
 * Comments:
 *	The cthread lock is held on entry.
 */

void 
cthread_block(cthread_t p, cthread_fn_t f, void *a, cthread_t wakeup)
{
    cthread_t waiter, new;
    static const char *wait_name = "Waiter";
#ifndef TRLOG
    kern_return_t r;
#endif
    
    TR_DECL("cthread_block");
    tr4("thread = %x func = %x arg = %x", (int)p, (int)f, (int)a);
#ifdef STATISTICS
    cthread_stats.blocks++;
#endif /*STATISTICS*/

    /* Indicate that this cthread is blocked. */
    p->state = CTHREAD_BLOCKED;

    /* If this is a wired thread, then we must block the Mach thread. */
    if (p->wired != MACH_PORT_NULL) {
	cthread_block_wired(p, (cthread_fn_t) f, a, wakeup);

	/* Never reached ... */
	return;
    } 

    /*
     * This is not a wired cthread, so we can attempt to schedule a waiting
     * cthread.  If there are no waiting cthreads, then we will need to idle
     * the Mach thread.
     */

    /*
     * Wakeup the requested cthread (actually thread context).  If there is a
     * thread context to run, then switch to that context and run it.
     */

    else {
	cthread_wakeup(wakeup);/*XXX EVIL.  LOCK HELD XXX*/
	cthread_queue_deq(&cthread_status.run_queue, cthread_t, new);
	if (new) {
	    new->state = CTHREAD_RUNNING;
	    CTHREAD_FILTER(p, CTHREAD_FILTER_INTERNAL_BUILD, f, a, (void *)&cthread_status.run_lock, &new->context);
	    return; /* never reached */
	} 
       /*
	* Nothing new to run.  We have to idle the Mach thread.  To do this we
	* allocate a dummy cthread (CTHREAD_WAITER) to switch to.  If there are no
	* unused waiter cthreads, we have to allocate one.
	*/
	else {
	    cthread_queue_deq(&cthread_status.waiters, cthread_t, waiter);
	    if (waiter == CTHREAD_NULL) {
#ifndef TRLOG
		vm_address_t base;
		vm_size_t size;
#endif /*TRLOG*/
#ifdef STATISTICS
		cthread_stats.wait_stacks++;
#endif /*STATISTICS*/
		waiter = cthread_alloc();
#ifdef TRLOG
		alloc_stack(waiter);
#else /*TRLOG*/
		size = cthread_status.wait_stack_size;
		MACH_CALL(vm_allocate(mach_task_self(), &base, size, TRUE), r);
		if (r == KERN_SUCCESS && base == 0) {
		    /* Allocating at address 0 could cause problems. */
		    MACH_CALL(vm_allocate(mach_task_self(), &base, size, TRUE),
			      r);
		    if (r == KERN_SUCCESS) {
			MACH_CALL(vm_deallocate(mach_task_self(),
						(vm_address_t) 0, size), r);
		    }
		}
#ifdef	WIRE_IN_KERNEL_STACKS
		/*
		 * If we're running in a kernel-loaded application, wire the stack
		 * we just allocated.
		 */
		if (in_kernel)
			stack_wire(base, size);
#endif	/* WIRE_IN_KERNEL_STACKS */

		waiter->stack_base = base;
		waiter->stack_size = cthread_status.wait_stack_size;
#endif /*TRLOG*/
		waiter->context = 
		    cthread_stack_base(waiter, CTHREAD_STACK_OFFSET);
		waiter->name = wait_name;
		CTHREAD_FILTER(waiter, CTHREAD_FILTER_PREPARE,
			       cthread_idle, waiter, 0, 0); 
	    }
	    waiter->wired = mach_thread_self();
	    waiter->flags = CTHREAD_EWIRED|CTHREAD_WAITER;
	    waiter->state = CTHREAD_RUNNING;

	    cthread_idle_enter(waiter);
	    CTHREAD_FILTER(p, CTHREAD_FILTER_INTERNAL_BUILD, f, a,
			   (void *)&cthread_status.run_lock,
			   &waiter->context);

	    return; /* never reached */
	}
    }
}

staticf int
cthread_cpus()
{
    mach_port_t host;
    mach_port_t default_pset;
    natural_t count;
    struct processor_set_basic_info info;

    count = PROCESSOR_SET_BASIC_INFO_COUNT;
    host = mach_host_self();
    processor_set_default(host, &default_pset);
    processor_set_info(default_pset, PROCESSOR_SET_BASIC_INFO,
		       &host, (host_info_t)&info, &count);
    return info.processor_count;
}


/*
 * Function: cthread_stack_base - Return the base of the thread's stack.
 *
 * Arguments:
 *	p - The cthread.
 *	offset - The offset into the stack to return.
 *
 * Return Value:
 * 	The base + offset into the stack is returned.
 */

vm_offset_t
cthread_stack_base(cthread, offset)
	register cthread_t cthread;
	register vm_size_t offset;
{
#ifdef	STACK_GROWTH_UP
	return (cthread->stack_base + offset);
#else	/*STACK_GROWTH_UP*/
	return (cthread->stack_base + cthread->stack_size - offset);
#endif	/*STACK_GROWTH_UP*/

}

/*
 * Internal C-Thread Functions
 */

/*
 * Function: cthread_init_newstack, cthread_init
 *
 * Description:
 *	Allocate the initial cthread and stack and startup the multithreaded
 * MIG interfaces.  The cthread_init_oldstack() interface is used when we want
 * to the existing stack for the initial cthread.  cthread_init_newstack will
 * allocate a new stack for the initial cthread.  cthread_init is the common
 * initialization function.
 *
 * Arguments:
 *	None.
 *
 * Return Value:
 *	The stack allocated to the initial cthread is returned.
 *
 * Comments:
 */
staticf vm_offset_t
cthread_init_newstack(void)
{
    vm_offset_t newstack;

    cthread_init(&newstack);
    return (vm_offset_t) newstack;
}


staticf void
cthread_init(vm_offset_t *newstack)
{

    cthread_t p;

    if (cthreads_started)
        return;

    /* Allocate the initial cthread and initialize the cthread library data. */
    cthread_malloc_init();
    p = cthread_alloc();
    cthread_status.cthread_list = p;
    cthread_status.cthread_cthreads = 1;
    cthread_status.alloc_cthreads = 1;
    cthread_status.kernel_threads = 1;

    cthread_idle_init();
    cthread_status.processors = cthread_cpus();
    if (cthread_status.processors > 1) {
#if CTHREAD_WAITER_SPIN_COUNT
	if (!cthread_status.waiter_spin_count)
	    cthread_status.waiter_spin_count = CTHREAD_WAITER_SPIN_COUNT;
#endif /*CTHREAD_WAITER_SPIN_COUNT*/
#if MUTEX_SPIN_COUNT
	if (!cthread_status.mutex_spin_count)
	    cthread_status.mutex_spin_count = MUTEX_SPIN_COUNT;
#endif /*MUTEX_SPIN_COUNT*/
#if LOCK_SPIN_COUNT
	if (!cthread_status.lock_spin_count)
	    cthread_status.lock_spin_count = LOCK_SPIN_COUNT;
#endif /*LOCK_SPIN_COUNT*/
    }

    p->status |= T_MAIN;
    if (cthread_status.max_kernel_threads == 0)
	p->wired = mach_thread_self();
    cthread_set_name((cthread_t)p, "main");

    cthreads_started = TRUE;
    
    stack_init(p, newstack);

#ifdef TRLOG
    /*
     * if TRLOG then we need full size wait stacks so we can
     * do printfs from idle threads
     */
    cthread_status.wait_stack_size = cthread_status.stack_size;
#else /*TRLOG*/
    if (cthread_wait_stack_size != 0)
	cthread_status.wait_stack_size = cthread_wait_stack_size;
#endif /*TRLOG*/

    mig_init(p);		/* enable multi-threaded mig interfaces */
}

/*
 * Function: cthread_body - Procedure invoked at the base of each cthread.
 *
 * Description:
 *
 * Arguments:
 *
 * Return Value:
 *
 * Comments:
 */

staticf void
cthread_body(cthread_t self)
{
    cthread_t wakeup = CTHREAD_NULL;

    TR_DECL("cthread_body");
    tr4("thread = %x func = %x status = %x", self, self->func, self->status);

    /*
     * If this cthread is supposed to run a function, do so...
     */
    if (self->func != (cthread_fn_t)0) {
	cthread_fn_t func = self->func;
	if (cthread_status.max_kernel_threads == 0)
	    self->wired = mach_thread_self();
	self->func = (cthread_fn_t)0;
	self->result = (*func)(self->arg);
    }

    /* Thread has exit'ed its function: anything else to do... */

    /*
     * If we are the last thread and there is an exit thread then make the
     * exit thread runnable.
     */
    cthread_lock();
    if (--cthread_status.cthread_cthreads == 1 &&
	cthread_status.exit_thread) {
	    cthread_queue_enq(&cthread_status.run_queue,
			  cthread_status.exit_thread);
	    if (cthread_status.exit_thread->wired != MACH_PORT_NULL) {
	        wakeup = cthread_status.exit_thread;
	        cthread_status.exit_thread->state = CTHREAD_RUNNING;
	    } else
	        cthread_status.exit_thread->state |= CTHREAD_RUNNABLE;
	    cthread_status.exit_thread = CTHREAD_NULL;
    }

    /* Remove our kernel thread association. */
    self->wired = MACH_PORT_NULL;
    self->flags &= ~CTHREAD_EWIRED;
    self->context = cthread_stack_base(self, CTHREAD_STACK_OFFSET);

    if (self->status & T_DETACHED) {
	cthread_queue_enq(&cthread_status.cthreads, self);
		} else {
	self->status |= T_RETURNED;
	if (self->joiner) {
	    cthread_t joiner = self->joiner;
	    tr2("joiner found %x", self->joiner);
	    self->joiner = CTHREAD_NULL;
	    if (joiner->wired != MACH_PORT_NULL) {
		joiner->state = CTHREAD_RUNNING;
		wakeup = joiner;
	    } else {
		cthread_queue_enq(&cthread_status.run_queue, joiner);
		joiner->state |= CTHREAD_RUNNABLE;
		if (self->wired != MACH_PORT_NULL) {
			wakeup = cthread_idle_first();
		}
	    }
	}
    }
    /* Block this thread and possibly wakeup a waiter (the wakeup thread). */
    cthread_block(self, (cthread_fn_t) cthread_body, self, wakeup);

    /* Never reached... */
    return; /* never reached */
}

cthread_t
cthread_create_handle(vm_offset_t *stack)
{
    cthread_t p;
    
    cthread_lock();
    cthread_status.cthread_cthreads++;
    cthread_queue_deq(&cthread_status.cthreads, cthread_t, p);
    if (p) {
	cthread_unlock();
    } else {
	p = cthread_create(CTHREAD_NOFUNC, (void *)0);
	}
    p->status = 0;
    p->state = CTHREAD_RUNNING;
    *stack = cthread_stack_base(p, CTHREAD_STACK_OFFSET);
    return (cthread_t)p;
}

/*
 * Function: cthread_fork
 *
 * Description:
 *	Create a child cthread and start it running at function func with
 *	argument arg.
 *
 * Arguments:
 *	func - The function the child should run.
 *	arg - The argument for func.
 *
 * Return Value:
 *	The new cthread is returned.
 */


cthread_t
cthread_fork(cthread_fn_t func, void *arg)
{
    cthread_t child;

    TR_DECL("cthread_fork");
    tr3("func = %x arg = %x", (int)func, (int)arg);

    /*
     * Attempt to find a free cthread.  If there is one, then initialize its
     * function and argument and make it runnable.  If there is also an idle
     * Mach thread, wakeup the Mach thread to run the cthread.
     */
    cthread_lock();
    cthread_status.cthread_cthreads++;
    cthread_queue_deq(&cthread_status.cthreads, cthread_t, child);
    if (child) {
	cthread_t waiter;
	child->func = func;
	child->arg = arg;
	child->status = 0;
	cthread_queue_enq(&cthread_status.run_queue, child);
	child->state |= CTHREAD_RUNNABLE;
	waiter = cthread_idle_first();
	cthread_unlock();
	tr3("child = 0x%x waiter = 0x%x", (int)child, (int)waiter);
	cthread_wakeup(waiter);
    } else {
	child = cthread_create(func, arg);
	}
    return (cthread_t)child;
}

/*
 * Function: cthread_detach
 *
 * Description:
 *	This function is used to indicate that a cthread is about to exit, and
 *	should be detached from the cthread pool.
 *
 * Arguments:
 *	p - The cthread that should be detached.
 *
 * Return Value:
 *	None.
 */

void
cthread_detach(cthread_t t)
{
    cthread_t p = (cthread_t)t;

    TR_DECL("cthread_detach");
    tr2("thread = %x",(int)t);
    cthread_lock();
    if (p->status & T_RETURNED) {      /* If we returned we are all done... */
	cthread_queue_enq(&cthread_status.cthreads, p);
	} else {                       /* Tell the exit code we detached... */
	p->status |= T_DETACHED;
	}
    cthread_unlock();
}

/*
 * Function: cthread_join, cthread_join_real
 *
 * Description:
 *	Wait for the specified cthread to exit.
 *
 * Arguments:
 *	t - The cthread to wait for
 *
 * Return Value:
 *
 * Comments:
 *	Because the join operation can block, the cthread's context is saved
 *      before calling the cthread_join_real function.  The join_real function
 *	performs the actual join and then returns to us.
 */

void *
cthread_join(cthread_t t)
{

    return ((void*) CTHREAD_FILTER(_cthread_self(), CTHREAD_FILTER_USER_BUILD,
		   cthread_join_real, t, 0, 0));
}

staticf void 
cthread_join_real(cthread_t p)
{
    void *result;
    cthread_t self = _cthread_self();

    TR_DECL("cthread_join_real");
    tr2("thread = %x",(int)p);
    cthread_lock();
    if (! (p->status & T_RETURNED)) {
#ifdef STATISTICS
	cthread_stats.join_miss++;
#endif /*STATISTIC*/
	p->joiner = self;
	tr1("blocking");
	cthread_block(self, (cthread_fn_t) cthread_join_real, p, CTHREAD_NULL);
	return; /* never reached */
    }
    result = p->result;
    cthread_queue_enq(&cthread_status.cthreads, p);
    cthread_unlock();
    CTHREAD_FILTER(self, CTHREAD_FILTER_USER_INVOKE, result, 0, 0, 0);
}


/*
 * Function: cthread_exit
 *
 * Description:
 *	A cthread is exiting.
 *
 * Arguments:
 *
 * Return Value:
 *
 * Comments:
 *	Comment from rwd -- XXX Fix T_MAIN case XXX
 */

void
cthread_exit(void *result)
{
    cthread_t p = _cthread_self();

    TR_DECL("cthread_exit");
    tr3("thread = %x result = %x",(int)p, (int)result);
    cthread_lock();


    /*
     * If this is the main cthread, and there are still other active
     * cthread's, then block waiting for them to exit.  When the last one,
     * other than us, exits we will be restarted.  At that point we can exit.
     */
    if (p->status & T_MAIN) {
	tr1("main exiting");

	if (cthread_status.cthread_cthreads > 1) {
	    p->context = cthread_stack_base(p, CTHREAD_STACK_OFFSET);
	    cthread_status.exit_thread = p;
	    cthread_block(p, (cthread_fn_t) cthread_exit, result, CTHREAD_NULL);
	}
		exit((int) result);
    } 

    /*
     * If we are not the main cthread, then rewind the stack back to the top,
     * add ourself to the cthread_run_q and wait for more work to do.
     */
    else {
	p->result = result;
	/* rewind stack */
	p->context = cthread_stack_base(p, CTHREAD_STACK_OFFSET);
	p->wired = MACH_PORT_NULL;
	p->flags &= ~CTHREAD_EWIRED;
	cthread_queue_enq(&cthread_status.run_queue, p);
	cthread_block(p, (cthread_fn_t) cthread_body, p, CTHREAD_NULL);
    }
}

/*
 * Miscellaneous functions:
 *	cthread_set_name - Set the cthread's name.
 * 	cthread_name     - Return the cthread's name.
 *	cthread_count    - Return the number of active cthreads.
 *	cthread_set_data - Set the cthread's data pointer.
 *	cthread_data     - Return the cthread's data pointer.
 */

void
cthread_set_name(cthread_t t, const char *name)
{
    ((cthread_t)t)->name = name;
}

const char *
cthread_name(cthread_t t)
{
    cthread_t p = (cthread_t)t;

    return (p == CTHREAD_NULL
		? "idle"
	    : (p->name == 0 ? "?" : p->name));
}


int
cthread_count()
{
    return cthread_status.cthread_cthreads;
}


void 
cthread_set_data(cthread_t t, void *x) {
    ((cthread_t)t)->data = x;
}

void *
cthread_data(cthread_t t) {
    return (t->data);
}


/*
 * Functions:
 *	cthread_kernel_limit - Return the maximum number of kernel threads.
 *	cthread_set_kernel_limit - Set the maximum number of kernel threads.
 *
 * Description:
 *	Set/return the maximum number of kernel threads.  This is the maximum
 *	number of Mach threads that will be created to run the cthreads.  A
 *	value of 0 indicates that an unlimited number of Mach threads can be
 *	created.  If this is the case, one Mach thread will be created for
 *	each cthread, and the cthread will be wired to the Mach thread.
 *
 * Comments:
 *	If the maximum number of kernel threads is reduced, then existing
 *	kernel threads over the limit will not be terminated.
 *
 *	If we remove an existing limit on the number of Mach threads, we
 *	do not wired the existing cthreads.
 */

int 
cthread_kernel_limit()
{

    /* Locking is not necessary here... */
    return cthread_status.max_kernel_threads;
}

/*
 * Set max number of kernel threads
 * Note:	This will not currently terminate existing threads
 * 		over maximum.
 */

void 
cthread_set_kernel_limit(int n)
{
    TR_DECL("cthread_set_kernel_limit");
    tr3("old = %d new = %d", cthread_status.max_kernel_threads, n);

    cthread_lock();

    /*
     * If we are removing the limit (i.e. allowing all cthreads to have Mach
     * threads) we should wire the existing cthreads but we don't.
     */

    /*
     * If we are imposing a limit where there was none, then unwire the
     * current cthread.  We should probably unwire all other existing
     * cthreads, but we don't.
     */
    if (n && cthread_status.max_kernel_threads == 0) {
	cthread_t p = _cthread_self();
	if (!(p->flags & CTHREAD_EWIRED))
	    p->wired = MACH_PORT_NULL;
    }

    /* Set the new limit. */
    cthread_status.max_kernel_threads = n;
    cthread_unlock();
}

int
cthread_limit()
{
    return cthread_kernel_limit();
}

void
cthread_set_limit(int n)
{
    cthread_set_kernel_limit(n);
}

cthread_t
cthread_ptr(vm_offset_t sp)
{
    return _cthread_ptr(sp);
}

void 
cthread_fork_prepare(void)
{
    cthread_t p = _cthread_self();

    cthread_malloc_fork_prepare();
    
    vm_inherit(mach_task_self(),p->stack_base, p->stack_size, VM_INHERIT_COPY);

    cthread_lock();
}

void 
cthread_fork_parent(void)
{
    cthread_t p = _cthread_self();
    
    cthread_malloc_fork_parent();

    cthread_unlock();
    vm_inherit(mach_task_self(),p->stack_base, p->stack_size, VM_INHERIT_NONE);
}

void 
cthread_fork_child(void)
{
    cthread_t p, l, m;

    cthread_malloc_fork_child();

    stack_fork_child();

    p = _cthread_self();

    cthread_status.cthread_cthreads = 1;
    cthread_status.alloc_cthreads = 1;
    p->status |= T_MAIN;
    cthread_set_name(p, "main");

    vm_inherit(mach_task_self(),p->stack_base, p->stack_size, VM_INHERIT_NONE);

    spin_lock_init(&cthread_status.run_lock);

    cthread_status.kernel_threads=0;

    for(l=cthread_status.cthread_list;l!=CTHREAD_NULL;l=m) {
	m=l->next;
	if (l!=p)
	    cthread_free((void *)l);
    }
    
    cthread_status.cthread_list = p;
    p->next = CTHREAD_NULL;
    
    cthread_queue_init(&cthread_status.waiters);
    cthread_queue_init(&cthread_status.cthreads);
    cthread_queue_init(&cthread_status.run_queue);
    cthread_idle_init();

    mig_init(p);		/* enable multi-threaded mig interfaces */

#ifdef STATISTICS
    cthread_stats.mutex_block = 0;
    cthread_stats.mutex_miss=0;
    cthread_stats.waiters=0;
    cthread_stats.unlock_new=0;
    cthread_stats.blocks=0;
    cthread_stats.notrigger=0;
    cthread_stats.wired=0;
    cthread_stats.wait_stacks = 0;
    cthread_stats.rnone = 0;
    cthread_stats.spin_count = 0;
    cthread_stats.mutex_caught_spin = 0;
    cthread_stats.idle_exit = 0;
    cthread_stats.mutex_cwl = 0;
    cthread_stats.idle_swtchs = 0;
    cthread_stats.idle_lost = 0;
    cthread_stats.umutex_enter = 0;
    cthread_stats.umutex_noheld = 0;
    cthread_stats.join_miss = 0;
#endif /*STATISTICS*/
    
}

/*
 * Mutual Exclusion Locks.
 */

mutex_t 
mutex_alloc() {
    return ((mutex_t) cthread_malloc(sizeof(struct mutex)));
}

void 
mutex_init(mutex_t m)
{
    TR_DECL("mutex_init");
    tr2("mutex = %x", (int)m);

    cthread_queue_init(&(m)->queue);
    spin_lock_init(&(m)->held);
    (m)->trigger=0;
    (m)->name=(char *)0;
}

void 
mutex_set_name(mutex_t m, const char *x)
{
    (m)->name = (x);
}

const char *
mutex_name(mutex_t m)
{
    return ((m)->name != 0 ? (m)->name : "?");
}

void 
mutex_clear(mutex_t m)
{
    mutex_init(m);
}

void 
mutex_free(mutex_t m)
{
    cthread_free((void *) (m));
}


/*
 * Condition functions.
 */

condition_t 
condition_alloc()
{
    return ((condition_t) cthread_malloc(sizeof(struct condition)));
}

void 
condition_init(condition_t c)
{
    cthread_queue_init(&(c)->queue);
}

void 
condition_set_name(condition_t c, const char *x)
{
	(c)->name = (x);
}

const char *
condition_name(condition_t c)
{
    return ((c)->name != 0 ? (c)->name : "?");
}

void 
condition_clear(condition_t c)
{
    condition_broadcast(c);
}

void 
condition_free(condition_t c)
{
    condition_clear(c);
    cthread_free((void *) (c));
}

/*
 * Function: condition_wait
 *
 * Description:
 *	Wait for the specified condition to occur.
 *
 * Arguments:
 *	c - The condition.
 *	m - The mutex used to synchronize condition waits and signals.
 *
 * Return Value:
 *	None.
 *
 * Comments:
 *	- condition_wait is just a stub that builds a user context, then calls
 *	  cond_wait.
 *	- The mutex_t m is locked on entry.  This mutex is used to synchronize
 *	  with condition_signal calls.  The mutex will be relocked before
 *	  returning to the caller.
 */

void 
condition_wait(condition_t c, mutex_t m) {
    CTHREAD_FILTER(_cthread_self(), CTHREAD_FILTER_USER_BUILD,
		   cond_wait, (c), (m), 0);
}


/*
 * Function: mutex_lock_solid, mutex_lock_real
 *
 * Description:
 *	These functions implement the reschedule wait on a mutex.  
 *
 * Arguments:
 *
 * Return Value:
 *
 * Comments:
 */

void 
mutex_lock_solid(mutex_t m) {
    CTHREAD_FILTER(_cthread_self(), CTHREAD_FILTER_USER_BUILD,
		   mutex_lock_real, (m), 0, 0);
}


staticf void 
mutex_lock_real(mutex_t m)
{
    kern_return_t r;
    int count = cthread_status.mutex_spin_count;
    cthread_t p = _cthread_self();
    
    TR_DECL("mutex_lock_real");
    tr2("mutex = %x", (int)m);

#ifdef STATISTICS
    cthread_stats.mutex_miss++;
#endif /*STATISTICS*/
#ifdef BUSY_SPINNING
    /* Spin attempting to get the lock. */
    while (count>=0) {
#endif /*BUSY_SPINNING*/

	/* If we can get the lock, we are done.  Return to the user context.  */
	if (spin_try_lock(&m->held)) {
#ifdef STATISTICS
	    stats_lock(&cthread_stats.lock);
	    cthread_stats.mutex_caught_spin++;
	    stats_unlock(&cthread_stats.lock);
#endif /*STATISTICS*/
	    CTHREAD_FILTER(p, CTHREAD_FILTER_USER_INVOKE,0, 0, 0, 0);
	    return; /* never reached */
	}
#ifdef BUSY_SPINNING
	count--;
    }
#endif /*BUSY_SPINNING*/
    count = 0;

#ifdef SPIN_POLL_MUTEX
    while(!cthread_try_lock()) {
	if (spin_try_lock(&m->held)) {
#ifdef STATISTICS
	    stats_lock(&cthread_stats.lock);
	    cthread_stats.mutex_cwl++;
	    stats_unlock(&cthread_stats.lock);
#endif /*STATISTICS*/
	    CTHREAD_FILTER(p, CTHREAD_FILTER_USER_INVOKE,0, 0, 0, 0);
	    return; /* never reached */
        } else if (count++>cthread_status.lock_spin_count) {
	    count = 0;
	    yield();
        }
    }

   /*
    * Could not get the mutex lock and our spin count has been exceeded.  At
    * this point we will give up and block the cthread.  After we lock the
    * cthread lock we try one last time to get the mutex lock.  If this
    * succeeds, we have save the cthread_block.  If this fails, we have no
    * choice but to block.
    */
#else /*SPIN_POLL_MUTEX*/
    cthread_lock();
#endif /*SPIN_POLL_MUTEX*/
    m->trigger = TRUE;
    if (spin_try_lock(&m->held)) {
	m->trigger = (cthread_queue_head(&m->queue, cthread_t) != CTHREAD_NULL);
	cthread_unlock();
	CTHREAD_FILTER(p, CTHREAD_FILTER_USER_INVOKE, 0, 0, 0, 0);
	return; /* never reached */
    } else {
	cthread_queue_enq(&m->queue, p);
    }
#ifdef STATISTICS
    cthread_stats.mutex_block++;
#endif /*STATISTICS*/
#ifdef WAIT_DEBUG
    p->waiting_for = (int)m;
#endif /*WAIT_DEBUG*/
    cthread_block(p, (cthread_fn_t) mutex_lock_real, m, CTHREAD_NULL);
    return; /* never reached */
}

void 
mutex_unlock_solid(mutex_t m)
{
    cthread_t new, waiter = CTHREAD_NULL;
    
    TR_DECL("mutex_unlock_solid");
    tr2("mutex = %x", (int)m);

#ifdef STATISTICS
    cthread_stats.umutex_enter++;
#endif /*STATISTICS*/
    if (!spin_try_lock(&m->held)) {
#ifdef STATISTICS
	cthread_stats.umutex_noheld++;
#endif /*STATISTICS*/
	return;
    }

    cthread_lock();
    if (m->trigger) {
	cthread_queue_deq(&m->queue, cthread_t, new);
    } else {
#ifdef STATISTICS
	cthread_stats.notrigger++;
#endif /*STATISTICS*/
	new = CTHREAD_NULL;
    }

    spin_unlock(&m->held);
    if (new) {
#ifdef STATISTICS
	cthread_stats.unlock_new++;
#endif /*STATISTICS*/
	m->trigger = (cthread_queue_head(&m->queue, cthread_t) != CTHREAD_NULL);
	if (new->wired != MACH_PORT_NULL) {
	    new->state = CTHREAD_RUNNING;
	    waiter = new;
	} else {
	    cthread_queue_enq(&cthread_status.run_queue, new);
	    new->state |= CTHREAD_RUNNABLE;
	    waiter = cthread_idle_first();
	}
    }
    cthread_unlock();
    cthread_wakeup(waiter);
}


/*
 * Function: condition_signal
 *
 * Description:
 *	Signal that the conditition has occurred and start one waiter.
 *
 * Arguments:
 *	c - The condition.
 *
 * Return Value:
 *	None.
 *
 * Comments:
 */
void 
condition_signal(condition_t c)
{
    cthread_t p, waiter = CTHREAD_NULL;
    
    TR_DECL("condition_signal");
    tr2("cond = %x", (int)c);

    /* If there are no waiters, then simply return. */
    if(cthread_queue_head(&c->queue, cthread_t) == CTHREAD_NULL)
	return;

    /*
     * Lock the cthread queues and recheck for a waiter.  If there is still a
     * waiter, remove if from the condition queue and restart it.
     */
    cthread_lock();
    cthread_queue_deq(&c->queue, cthread_t, p);
    if (p) {
	mutex_t m = p->cond_mutex;
	tr2("waking up %x",(int)p);
	m->trigger = TRUE;
	if (spin_lock_locked(&m->held)) {
#ifdef WAIT_DEBUG
	    p->waiting_for = (int)m;
#endif /*WAIT_DEBUG*/
	    cthread_queue_enq(&m->queue, p);
	} else {
	    m->trigger = (cthread_queue_head(&m->queue, cthread_t) != CTHREAD_NULL);
	    if (p->wired != MACH_PORT_NULL) {
		p->state = CTHREAD_RUNNING;
		waiter = p;
	    } else {
		cthread_queue_enq(&cthread_status.run_queue, p);
		p->state |= CTHREAD_RUNNABLE;
		waiter = cthread_idle_first();
	    }
	}
    }

    /* Unlock the cthread lock and start the waiter. */
    cthread_unlock();
    cthread_wakeup(waiter);
}

/*
 * Function: condition_broadcast
 *
 * Description:
 *	Signal that the conditition has occurred and start all waiters.
 *
 * Arguments:
 *	c - The condition.
 *
 * Return Value:
 *	None.
 *
 * Comments:
 */
void 
condition_broadcast(condition_t c)
{
    cthread_t p, waiter = CTHREAD_NULL;
    
    TR_DECL("condition_broadcastl");
    tr2("cond = %x", (int)c);

    /* If there are no waiters, simply return. */
    if(cthread_queue_head(&c->queue, cthread_t) == CTHREAD_NULL)
	return;

    /*
     * Lock the cthread queues and recheck for a waiter.  If there is still a
     * waiter, remove if from the condition queue and restart it.
     */
    cthread_lock();
    while (cthread_queue_head(&c->queue, cthread_t) != CTHREAD_NULL) {
	mutex_t m;

    if (waiter) {
        cthread_wakeup(waiter);    /*XXX LOCK HELD XXX*/
        waiter = CTHREAD_NULL;
    }

	cthread_queue_deq(&c->queue, cthread_t, p);
	m = p->cond_mutex;
	m->trigger = TRUE;
	if (spin_lock_locked(&m->held)) {
#ifdef WAIT_DEBUG
	    p->waiting_for = (int)m;
#endif /*WAIT_DEBUG*/
	    cthread_queue_enq(&m->queue, p);
	} else {
	    m->trigger = (cthread_queue_head(&m->queue, cthread_t) != CTHREAD_NULL);
	    if (p->wired != MACH_PORT_NULL) {
		p->state = CTHREAD_RUNNING;
		waiter = p;
	    } else {
		cthread_queue_enq(&cthread_status.run_queue, p);
		p->state |= CTHREAD_RUNNABLE;
		waiter = cthread_idle_first();
	    }
	}
    }
    cthread_unlock();
    cthread_wakeup(waiter);
}

staticf void 
cond_wait(condition_t c, mutex_t m)
{
    cthread_t p = _cthread_self();
    cthread_t new, wakeup = CTHREAD_NULL;
    
    TR_DECL("condition_wait");
    tr3("cond = %x mutex = %x", (int)c, (int)m);

    /*
     * Lock the cthread data structures, and queue the current cthread on the
     * condition.
     */
    cthread_lock();
    cthread_queue_enq(&c->queue, p);
    p->cond_mutex = m;
    
    /*
     * Remove the first waiter from the mutex (if there is one), and unlock
     * the mutex.
     */
    cthread_queue_deq(&m->queue, cthread_t, new);
    spin_unlock(&m->held);
    
    /*
     * If there was a cthread waiting on the mutex, start it.
     */
    if (new) {
	m->trigger = (cthread_queue_head(&m->queue, cthread_t) != CTHREAD_NULL);
	if (new->wired != MACH_PORT_NULL) {
	    new->state = CTHREAD_RUNNING;
	    wakeup = new;
	} else {
	    cthread_queue_enq(&cthread_status.run_queue, new);
	    new->state |= CTHREAD_RUNNABLE;
	    if (p->wired != MACH_PORT_NULL) {
		    wakeup = cthread_idle_first();
	    }
	}
    }
#ifdef WAIT_DEBUG
    p->waiting_for = (int)c;
#endif /*WAIT_DEBUG*/
    cthread_block(p, (cthread_fn_t) mutex_lock_real, m, wakeup);
}

staticf void 
cthread_return(cthread_t p)
{
	CTHREAD_FILTER(p, CTHREAD_FILTER_USER_INVOKE, 0, 0, 0, 0);
}

/*
 * Function: cthread_yield, cthread_yield_real
 *
 * Description:
 *	Yield our kernel thread or the processor.  If the cthread is wired then
 * we yield the processor to higher priority Mach threads.  If the cthread is
 * not wired then we attempt to run a different cthread on the current Mach
 * thread.  If there are no waiting runnable cthreads, we release the processor
 * so that another Mach thread (and hence another cthread) can run.
 *
 *	This function is called when it is determined that the current cthread
 * can not make further progress without another thread first running (e.g.
 * when waiting for a spin lock).
 *
 * Arguments:
 * 	None.
 *
 * Return Value:
 * 	None.
 */

void 
cthread_yield() {
    CTHREAD_FILTER(_cthread_self(), CTHREAD_FILTER_USER_BUILD,
		   cthread_yield_real, 0, 0, 0);
}


staticf void 
cthread_yield_real()
{
    cthread_t new, p = _cthread_self();
    kern_return_t r;

    /*
     * If we are a wired cthread, yield to give the Mach thread a chance to
     * context switch.  If there is anything higher priority than us it will
     * run first.  When we are running again, return to the caller's user
     * context.
     */
    if (p->wired != MACH_PORT_NULL) {
	yield();
	CTHREAD_FILTER(p, CTHREAD_FILTER_USER_INVOKE, 0, 0, 0, 0);
	/* Never reached ... */
    }

    /*
     * If we are not a wired thread, look to see if there is a waiting
     * cthread.  If there is, remove if from the run queue and switch to it's
     * context.  If there are no waiting cthread's then do a Mach thread
     * switch. 
     */
    cthread_lock();
    cthread_queue_deq(&cthread_status.run_queue, cthread_t, new);
    if (new) {
	cthread_queue_enq(&cthread_status.run_queue, p);
	p->state = CTHREAD_BLOCKED|CTHREAD_RUNNABLE;
	new->state = CTHREAD_RUNNING;
	CTHREAD_FILTER(p, CTHREAD_FILTER_INTERNAL_BUILD, cthread_return,
		       p, (void *)&cthread_status.run_lock, &new->context);
	/* Never reached... */
    } else {
	cthread_unlock();
	yield();
	CTHREAD_FILTER(p, CTHREAD_FILTER_USER_INVOKE, 0, 0, 0, 0);
	/* Never reached ... */
    }
}

/*
 * Function: cthread_self - Return the current cthread's cthread ID.
 *
 * Description:
 *	The cthread ID is equivalent to the cthread stack address minus the
 *	size of a cthread pointer.  All of this magic is done by the
 *	_cthread_self macro.
 */

cthread_t 
cthread_self() {
    return (cthread_t)_cthread_self();
}


/*
 * Function: cthread_wire, cthread_unwire, cthread_wire_other
 *
 * Description:
 *	Wire/unwire cthreads to Mach threads.
 *
 * Arguments:
 *
 * Return Value:
 *
 * Comments:
 */

void 
cthread_wire()
{
    cthread_wire_other(_cthread_self(), mach_thread_self());
}

void 
cthread_wire_other(cthread_t p, mach_port_t port)
{
#ifdef STATISTICS
    boolean_t already_wired = p->wired;
#endif /*STATISTICS*/

    p->wired = port;
    p->flags |= CTHREAD_EWIRED;
#ifdef STATISTICS
    if (!already_wired) {
	stats_lock(&cthread_stats.lock);
	cthread_stats.wired++;
	stats_unlock(&cthread_stats.lock);
    }
#endif /*STATISTICS*/
}

/*
 * Unwire a cthread.
 */

void 
cthread_unwire()
{
    cthread_t p = _cthread_self();

    if (p->wired != MACH_PORT_NULL) {
	p->wired = MACH_PORT_NULL;
	p->flags &= ~CTHREAD_EWIRED;
#ifdef STATISTICS
	stats_lock(&cthread_stats.lock);
	cthread_stats.wired--;
	stats_unlock(&cthread_stats.lock);
#endif /*STATISTICS*/
    }    
}

void 
cthread_pstats(int file)
{
#ifdef STATISTICS
    fprintf(file,"mutex_miss        %d\n",cthread_stats.mutex_miss); 
    fprintf(file,"mutex_caught_spin %d\n",cthread_stats.mutex_caught_spin);
    fprintf(file,"mutex_cwl         %d\n",cthread_stats.mutex_cwl);
    fprintf(file,"mutex_block       %d\n",cthread_stats.mutex_block);
    fprintf(file,"wait_stacks       %d\n",cthread_stats.wait_stacks);
    fprintf(file,"waiters           %d\n",cthread_stats.waiters);
    fprintf(file,"idle_swtchs       %d\n",cthread_stats.idle_swtchs);
    fprintf(file,"idle_lost         %d\n",cthread_stats.idle_lost);
    fprintf(file,"idle_exit         %d\n",cthread_stats.idle_exit);
    fprintf(file,"unlock_new        %d\n",cthread_stats.unlock_new);
    fprintf(file,"blocks            %d\n",cthread_stats.blocks);
    fprintf(file,"notrigger         %d\n",cthread_stats.notrigger);
    fprintf(file,"wired             %d\n",cthread_stats.wired);
    fprintf(file,"rnone             %d\n",cthread_stats.rnone);
    fprintf(file,"spin_count        %d\n",cthread_stats.spin_count);
    fprintf(file,"umutex_enter      %d\n",cthread_stats.umutex_enter);
    fprintf(file,"umutex_noheld     %d\n",cthread_stats.umutex_noheld);
    fprintf(file,"join_miss         %d\n",cthread_stats.join_miss);
#endif /*STATISTICS*/
}

int *
__mach_errno_addr()
{
    return &_cthread_self()->err_no;
}
