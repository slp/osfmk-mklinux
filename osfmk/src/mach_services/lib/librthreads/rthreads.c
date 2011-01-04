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
 * MkLinux
 */
/*
 * File: rthreads.c
 *
 * Implementation of spawn, exit, etc.
 *
 * Derived from the C Threads Package (version 2).
 *
 */

#include <rthreads.h>
#include <rthread_filter.h>
#include "rthread_internals.h"
#include "trace.h"
#include <mach/mach_traps.h>
#include <mach/mach_host.h>
#include <mach/sync_policy.h>
#include <stdarg.h>

/*
 * 	Forward Internal Declarations
 */

private void 		rthread_body(rthread_t);
private rthread_t 	rthread_alloc();
private void 		rthread_kernel_bind(rthread_t, boolean_t);
private rthread_t	rthread_create(rthread_fn_t, void *, int);

/*
 *	Rthread Control Block
 */

struct rthread_control_struct rthread_control = {
	MACH_PORT_NULL,
	RTHREADS_VERSION,
	0,
	0,
	0,
	RTHREAD_NULL,
	0,
	0,
	RTHREAD_NO_POLICY,
	RTHREAD_NO_PRIORITY,
	RTHREAD_NO_PRIORITY,
	RTHREAD_NO_QUANTUM,
	FALSE,
	MACH_PORT_NULL,
	QUEUE_INITIALIZER
};


/*
 *  ROUTINE:	rthread_init
 *
 *  FUNCTION:	Called by crt0 to initialize the rthread package state,
 *		as well as, the state of the first rthread.
 */

int
rthread_init()
{
	static int 		rthreads_started = FALSE;
	register rthread_t 	parent;
	vm_offset_t		stack;
	kern_return_t		r;

	if (rthreads_started)
		return (0);

	rthread_control_create_locks();

	parent = rthread_alloc();
	rthread_control.rthread_list    = parent;
	rthread_control.active_rthreads = 1;
	rthread_control.alloc_rthreads  = 1;

	MACH_CALL(semaphore_create(mach_task_self(),
				   &rthread_control.main_wait,
				   SYNC_POLICY_FIFO, 0), r);

	parent->status |= T_MAIN;
	parent->wired = mach_thread_self();
	rthread_set_name(parent, "main");

	/*
	 * Initialize default policy
	 */
	rthread_policy_info(parent, TRUE);
	rthread_set_default_policy_real(RTHREAD_DEFAULT_POLICY, TRUE);
	rthread_set_default_attributes(parent);

	rthreads_started = TRUE;
    
	/*
	 * We pass back the new stack which should be switched to
	 * by crt0.  This guarantess correct size and alignment.
	 */
	(void) stack_init(parent, &stack);

	mig_init(parent);	/* enable multi-threaded mig interfaces */

	return (stack);
}

/*
 *  Used for automatic initialization by crt0.
 *  Cast needed since too many C compilers choke on the type void (*)().
 */

int (*_thread_init_routine)() = (int (*)()) rthread_init;

/*
 * Function: rthread_stack_base - Return the base of the thread's stack.
 *
 * Arguments:
 *	p - The thread.
 *	offset - The offset into the stack to return.
 *
 * Return Value:
 * 	The base + offset into the stack is returned.
 */

vm_offset_t
rthread_stack_base(rthread, offset)
	register rthread_t rthread;
	register int offset;
{
#ifdef	STACK_GROWTH_UP
	return (rthread->stack_base + offset);
#else	/*STACK_GROWTH_UP*/
	return (rthread->stack_base + rthread->stack_size - offset);
#endif	/*STACK_GROWTH_UP*/

}

/*
 *  ROUTINE:	rthread_alloc		[internal]
 *
 *  FUNCTION:	Allocates space for an rthread structure and initializes it.
 *
 *  NOTE: 	Rthread control block lock is held when called.
 */

private rthread_t
rthread_alloc()
{
	register rthread_t th;

	TR_DECL("rthread_alloc");
	tr1("enter");

	th = (rthread_t) rthreads_malloc(sizeof(struct rthread));

	th->reply_port	= MACH_PORT_NULL;
	th->wired	= MACH_PORT_NULL;
	th->status 	= 0;
	th->list   	= rthread_control.rthread_list;
	rthread_control.rthread_list = th;
	rthread_control.alloc_rthreads++;

	return (th);
}

/*
 *  ROUTINE:	rthread_create		[internal]
 *
 *  FUNCTION:	Creates an rthread and allocates a stack.
 */

private rthread_t
rthread_create(rthread_fn_t func, void *arg, int type)
{
	kern_return_t		r;	
	register rthread_t 	child;

	TR_DECL("rthread_create");
	tr3("func = %x arg = %x",(int)func, (int)arg);

	rthread_control_lock();

	/* Check free list for cached rthreads */
	rthread_queue_deq(&rthread_control.free_list, rthread_t, child);
 
	if (!child) {
		child = rthread_alloc();
		alloc_stack(child);
	}

	if (type == T_CHILD)
		rthread_control.active_rthreads++;
	else 	/* T_ACT */
		rthread_control.activation_count++;

	rthread_control_unlock();

	if (child->wired != MACH_PORT_NULL) {
		/* Drop port reference to allow port resource cleanup.
		 * The port reference was left behind from the last rthread
		 * that used this structure.
		 */	
               if (thread_terminate(child->wired) == KERN_SUCCESS)
                       mach_port_deallocate(mach_task_self(), child->wired);
	}

	child->func   	= func;
	child->arg    	= arg;
	child->status 	= type;
	child->priority = RTHREAD_NO_PRIORITY;
	child->quantum  = RTHREAD_NO_QUANTUM;
	child->policy   = RTHREAD_NO_POLICY;

	tr1("exit");
	return(child);
}

/*
 *  ROUTINE:	rthread_kernel_bind	[internal]
 *
 *  FUNCTION:	Binds an rthread to a kernel thread.  If the resume argument is
 *		TRUE, the kernel thread will be scheduled to run, otherwise,
 *		the thread will remain suspended.
 */

private void
rthread_kernel_bind(rthread_t th, boolean_t resume)
{
	register kern_return_t r;
	thread_port_t kernel_thread;

	TR_DECL("rthread_kernel_bind");
	tr3("th = %x resume = %d",th, resume);

	MACH_CALL(thread_create(mach_task_self(), &kernel_thread), r);
	th->wired = kernel_thread;
	rthread_setup(th, kernel_thread, (rthread_fn_t) rthread_body);
	if (resume)
		MACH_CALL(thread_resume(kernel_thread), r);
}

/*
 *  ROUTINE:	rthread_spawn
 *
 *  FUNCTION:	Creates a new rthread, binds it to a kernel thread and
 *		schedules the thread to run.  The new threads scheduling
 *		attibutes are inherited from the calling thread.
 */
	
rthread_t
rthread_spawn(rthread_fn_t func, void *arg)
{
	rthread_t child;

	TR_DECL("rthread_spawn");
	tr3("func = %x arg = %x", (int)func, (int)arg);

	child = rthread_create(func, arg, T_CHILD);

	rthread_set_default_attributes(child);
	rthread_kernel_bind(child, TRUE);

	return (child);
}

/*
 *  ROUTINE:	rthread_spawn_priority
 *
 *  FUNCTION:	Creates a new rthread and binds it to a kernel thread. The new
 *		threads priority is set to that specified by the priority
 *		argument.  Once the priority is set, the new thread is
 *		scheduled to run.
 */

rthread_t
rthread_spawn_priority(rthread_fn_t func, void *arg, int priority)
{
	register rthread_t 	child;
	kern_return_t		r;

	TR_DECL("rthread_spawn_priority");
	tr4("func = %x arg = %x pri = %d", (int)func, (int)arg, priority);

	child = rthread_create(func, arg, T_CHILD);

	rthread_set_default_attributes(child);
	rthread_kernel_bind(child, FALSE);
	rthread_set_priority(child, priority);
	MACH_CALL(thread_resume(child->wired), r);

	return (child);
}

/*
 *  ROUTINE:	rthread_activation_create
 *
 *  FUNCTION:	Creates an empty rthread.  Empty rthreads consist of a
 *		kernel activation and a user-level stack.  The rthread
 *		handle is returned.
 */
 
rthread_t
rthread_activation_create(mach_port_t subsystem)
{ 
	rthread_t  	rthread_act;
	thread_act_t	kernel_act;
	vm_offset_t	base;
	kern_return_t	r;

	TR_DECL("rthread_activation_create");
	tr1("enter");

	rthread_act = rthread_create(0, 0, T_ACT);

	base = rthread_stack_base(rthread_act, RTHREAD_STACK_OFFSET);

	MACH_CALL(thread_activation_create(mach_task_self(),
					   subsystem,
					   base,
					   rthread_act->stack_size,
					   &kernel_act), r);
	rthread_act->wired = kernel_act;

	return(rthread_act);
}

/*
 *  ROUTINE:	rthread_exit
 *
 *  FUNCTION:	Terminates the calling rthread.  If the rthread is the
 *		main/parent rthread, it will wait for its children to
 *		complete before terminating.
 */

void
rthread_exit(int result)
{
	rthread_t th = _rthread_self();

	TR_DECL("rthread_exit");
	tr3("thread = %x result = %x", th, result);

	th->result = result;

	rthread_terminate(th);

	/* NEVER reached */
}

/*
 *  ROUTINE:	rthread_terminate
 *
 *  FUNCTION:	Terminates the specified rthread.  If the specified rthread is
 *		the main/parent rthread, it will wait for its children to
 *		complete before terminating.
 */

void
rthread_terminate(rthread_t th)
{
	kern_return_t r;
	thread_port_t kernel_thread;


	TR_DECL("rthread_terminate");
	tr3("thread = %x result = %x", th, th->result);

	rthread_control_lock();

	if (th->status & T_MAIN) {

		tr1("main exiting");
		if (rthread_control.active_rthreads > 1) {
			rthread_control.main_waiting = TRUE;
			rthread_control_unlock();
			do {
				r = semaphore_wait(rthread_control.main_wait);
			} while (r != KERN_SUCCESS);	
		}

		MACH_CALL(semaphore_destroy(mach_task_self(),
					    rthread_control.main_wait), r);

		rthread_control_destroy_locks();

		exit(th->result);
	}

	/* rewind stack */
	th->context = rthread_stack_base(th, RTHREAD_STACK_OFFSET);
	/* cache rthread */
	rthread_queue_enq(&rthread_control.free_list, th);

	kernel_thread = th->wired;

	if (th->status & T_CHILD) {
		if (--rthread_control.active_rthreads == 1 &&
		      rthread_control.main_waiting) {
			rthread_control_unlock();
			MACH_CALL(semaphore_signal(
				  rthread_control.main_wait), r);
		} else
			rthread_control_unlock();
	} else	{
		/* T_ACT */
		rthread_control.activation_count--;
		rthread_control_unlock();
	}

	MACH_CALL(thread_terminate(kernel_thread), r);
}

/*
 *  Used for automatic finalization by crt0.  Cast needed since too many C
 *  compilers choke on the type void (*)().
 */

int (*_thread_exit_routine)() = (int (*)()) rthread_exit;

/*
 *  ROUTINE:	rthread_body		[internal]
 *
 *  FUNCTION:	The procedure invoked at the base of each rthread.
 */

void
rthread_body(rthread_t self)
{
	kern_return_t r;

	TR_DECL("rthread_body");
	tr4("thread= %x func= %x status= %x", self, self->func, self->status);

	if (self->func != (rthread_fn_t)0) {
		rthread_fn_t func = self->func;
		self->func 	  = (rthread_fn_t)0;
		self->result      = (int) ((*func)(self->arg));
	}

	rthread_terminate(self);
	/* never return */
}

/*
 *  ROUTINE:	rthread_wait
 *
 *  FUNCTION:	Causes the parent rthread to block until its children have
 *		terminated.  The rthread_wait routine can only be called by 
 *		the parent/main rthread.
 */

int
rthread_wait()
{
	rthread_t th = _rthread_self();
	kern_return_t r;

	TR_DECL("rthread_wait");
	tr3("thread = %x status = %x", th, (int)th->status);

	if (!(th->status & T_MAIN))
		return (-1);

	rthread_control_lock();
	if (rthread_control.active_rthreads > 1) {
		tr1("main waiting");
		rthread_control.main_waiting = TRUE;
		rthread_control_unlock();

		r = semaphore_wait(rthread_control.main_wait);

		if (r == KERN_SUCCESS) {
			/*
			 * Don't need lock here, since main is the
			 * only thread running in the task.
			 */
			rthread_control.main_waiting = FALSE;
		} else {
			rthread_control_lock();
			rthread_control.main_waiting = FALSE;
			rthread_control_unlock();
		}
	} else
		rthread_control_unlock();

	return (r);
}

/*
 *  ROUTINE:	rthread_set_name
 *
 *  FUNCTION:	Assigns a text name to the specified rthread.  This is 
 *		primarily used for debugging purposes.
 */

void
rthread_set_name(rthread_t th, const char *name)
{
	th->name = name;
}

/*
 *  ROUTINE:	rthread_get_name
 *
 *  FUNCTION:	Returns the text name associated with the specified rthread.
 *		This is primarily used for debugging purposes.
 */

const char *
rthread_get_name(rthread_t th)
{
	return (th == RTHREAD_NULL
		? "idle"
		: (th->name == 0 ? "?" : th->name));
}

/*
 *  ROUTINE:	rthread_kernel_port
 *
 *  FUNCTION:	Returns the port of the rthread's wired kernel thread.
 */
 	
mach_port_t
rthread_kernel_port(rthread_t th)
{
	return(th->wired);
}

/*
 *  ROUTINE:	rthread_count
 *
 *  FUNCTION:	Returns the number of currently active rthreads (allocated and
 *		tied/wired to a kernel thread).
 *
 *		Parent + number of active child rthreads
 * 
 *  NOTE:	Rthread activations are not considered to be children.  Hence,
 *		rthread activations are not tallied in with the rthread count. 
 */
	
int
rthread_count()
{
	return(rthread_control.active_rthreads);
}

/*
 *  ROUTINE:	rthread_activation_count
 *
 *  FUNCTION:	Returns the number of currently allocated rthread
 *		activations.
 */
	
int
rthread_activation_count()
{
	return (rthread_control.activation_count);
}

/*
 *  ROUTINE:	rthread_ptr
 *
 *  FUNCTION:	Returns the rthread pointer of the of the rthread 
 *		that owns the specified stack.
 */
	
rthread_t
rthread_ptr(int sp)
{
	return _rthread_ptr(sp);
}

/*
 *  ROUTINE: 	rthread_fork_prepare
 *
 *  FUNCTION: 	Called BEFORE a heavy weight fork operation (i.e., UNIX fork()),
 *		to prepare the rthread state for copying.
 */
	
void 
rthread_fork_prepare()
{
	kern_return_t		r;
	register rthread_t 	th = _rthread_self();

	rthread_control_lock();
	rthread_control_policy_lock();

	vm_inherit(mach_task_self(),
		   th->stack_base,
		   th->stack_size,
		   VM_INHERIT_COPY);
}

/*
 *  ROUTINE: 	rthread_fork_parent
 *
 *  FUNCTION: 	Called AFTER a heavy weight fork operation (i.e., UNIX fork()),
 *		by the PARENT task, returning the rthread state to normal. 
 *
 *		The rthread_fork_parent() routine is always preceeded by the
 *		rthread_fork_prepare() routine.
 */

void 
rthread_fork_parent()
{
	kern_return_t		r;
	register rthread_t 	th = _rthread_self();
	
	rthread_control_unlock();
	rthread_control_policy_unlock();

	vm_inherit(mach_task_self(),
		   th->stack_base,
		   th->stack_size,
		   VM_INHERIT_NONE);
}

/*
 *  ROUTINE: 	rthread_fork_child
 *
 *  FUNCTION: 	Called AFTER a heavy weight fork operation (i.e., UNIX fork()),
 *		by the CHILD task, initializing the new task's rthread
 *		state. 
 *
 *		The rthread_fork_child() routine is always preceeded by the
 *		rthread_fork_prepare() routine.
 */

void 
rthread_fork_child()
{
	kern_return_t 	r;
	rthread_t 	th, l, m;

	stack_fork_child();

	th = _rthread_self();

	rthread_control.active_rthreads  = 1;
	rthread_control.alloc_rthreads   = 1;
	rthread_control.activation_count = 0;

	th->status |= T_MAIN;
	th->wired = mach_thread_self();
	rthread_set_name(th, "main");

	MACH_CALL(semaphore_create(mach_task_self(),
				   &rthread_control.main_wait,
				   SYNC_POLICY_FIFO, 0), r);

	rthread_control_create_locks();

	rthread_set_default_attributes(th);
	rthread_set_policy(th, rthread_control.default_policy);

	vm_inherit(mach_task_self(),
		   th->stack_base,
		   th->stack_size,
		   VM_INHERIT_NONE);

	for(l=rthread_control.rthread_list;l!=RTHREAD_NULL;l=m) {
		m=l->next;
		if (l!=th)
			rthreads_free((void *)l);
	}
    
	rthread_control.rthread_list = th;
	th->next = RTHREAD_NULL;
    
	rthread_queue_init(&rthread_control.free_list);

	mig_init(th);		/* enable multi-threaded mig interfaces */
}

/*
 *  ROUTINE:	rthread_set_data
 *
 *  FUNCTION:	Assigns a data value to the specified rthread.
 */

void 
rthread_set_data(rthread_t th, void *x)
{
	th->data = x;
}

/*
 *  ROUTINE:	rthread_get_data
 *
 *  FUNCTION:	Returns the data associated with the specified rthread.
 */

void *
rthread_get_data(rthread_t th)
{
	void *data;
	data = th->data;

	return (data);
}

/*
 *  ROUTINE:	rthread_self
 *
 *  FUNCTION:	returns a pointer to the rthread structure of the calling
 *		rthread.
 */
	
rthread_t 
rthread_self()
{
	return (rthread_t)_rthread_self();
}

#define PRINT_BUFFER_SIZE 256
#define PRINT_LOCK_ID 0

static int print_buffer_index;
static char print_buffer[PRINT_BUFFER_SIZE];
static mach_port_t print_lock;
static mach_port_t printer_ready;
static mach_port_t printer_waiting;

#define isdigit(d) ((d) >= '0' && (d) <= '9')
#define Ctod(c) ((c) - '0')

#define MAXBUF (sizeof(long int) * 8)		 /* enough for binary */

static
printnum(u, base, putc, putc_arg)
	register unsigned int	u;	/* number to print */
	register int		base;
	void			(*putc)();
	char			*putc_arg;
{
	char	buf[MAXBUF];	/* build number here */
	register char *	p = &buf[MAXBUF-1];
	static char digs[] = "0123456789abcdef";

	do {
	    *p-- = digs[u % base];
	    u /= base;
	} while (u != 0);

	while (++p != &buf[MAXBUF])
	    (*putc)(putc_arg, *p);
}

rthread_doprnt(fmt, args, radix, putc, putc_arg)
	register char	*fmt;
	va_list		args;
	int		radix;		/* default radix - for '%r' */
 	void		(*putc)();	/* character output */
	char		*putc_arg;	/* argument for putc */
{
	int		length;
	int		prec;
	boolean_t	ladjust;
	char		padc;
	int		n;
	unsigned int	u;
	int		plus_sign;
	int		sign_char;
	boolean_t	altfmt;
	int		base;
	char		c;

	while (*fmt != '\0') {
	    if (*fmt != '%') {
		(*putc)(putc_arg, *fmt++);
		continue;
	    }

	    fmt++;

	    length = 0;
	    prec = -1;
	    ladjust = FALSE;
	    padc = ' ';
	    plus_sign = 0;
	    sign_char = 0;
	    altfmt = FALSE;

	    while (TRUE) {
		if (*fmt == '#') {
		    altfmt = TRUE;
		    fmt++;
		}
		else if (*fmt == '-') {
		    ladjust = TRUE;
		    fmt++;
		}
		else if (*fmt == '+') {
		    plus_sign = '+';
		    fmt++;
		}
		else if (*fmt == ' ') {
		    if (plus_sign == 0)
			plus_sign = ' ';
		    fmt++;
		}
		else
		    break;
	    }

	    if (*fmt == '0') {
		padc = '0';
		fmt++;
	    }

	    if (isdigit(*fmt)) {
		while(isdigit(*fmt))
		    length = 10 * length + Ctod(*fmt++);
	    }
	    else if (*fmt == '*') {
		length = va_arg(args, int);
		fmt++;
		if (length < 0) {
		    ladjust = !ladjust;
		    length = -length;
		}
	    }

	    if (*fmt == '.') {
		fmt++;
		if (isdigit(*fmt)) {
		    prec = 0;
		    while(isdigit(*fmt))
			prec = 10 * prec + Ctod(*fmt++);
		}
		else if (*fmt == '*') {
		    prec = va_arg(args, int);
		    fmt++;
		}
	    }

	    if (*fmt == 'l')
		fmt++;	/* need it if sizeof(int) < sizeof(long) */

	    switch(*fmt) {
		case 'b':
		case 'B':
		{
		    register char *p;
		    boolean_t	  any;
		    register int  i;

		    u = va_arg(args, unsigned int);
		    p = va_arg(args, char *);
		    base = *p++;
		    printnum(u, base, putc, putc_arg);

		    if (u == 0)
			break;

		    any = FALSE;
		    while (i = *p++) {
			if (*p <= 32) {
			    /*
			     * Bit field
			     */
			    register int j;
			    if (any)
				(*putc)(putc_arg, ',');
			    else {
				(*putc)(putc_arg, '<');
				any = TRUE;
			    }
			    j = *p++;
			    for (; (c = *p) > 32; p++)
				(*putc)(putc_arg, c);
			    printnum((unsigned)( (u>>(j-1)) & ((2<<(i-j))-1)),
					base, putc, putc_arg);
			}
			else if (u & (1<<(i-1))) {
			    if (any)
				(*putc)(putc_arg, ',');
			    else {
				(*putc)(putc_arg, '<');
				any = TRUE;
			    }
			    for (; (c = *p) > 32; p++)
				(*putc)(putc_arg, c);
			}
			else {
			    for (; *p > 32; p++)
				continue;
			}
		    }
		    if (any)
			(*putc)(putc_arg, '>');
		    break;
		}

		case 'c':
		    c = va_arg(args, int);
		    (*putc)(putc_arg, c);
		    break;

		case 's':
		{
		    register char *p;
		    register char *p2;

		    if (prec == -1)
			prec = 0x7fffffff;	/* MAXINT */

		    p = va_arg(args, char *);

		    if (p == (char *)0)
			p = "";

		    if (length > 0 && !ladjust) {
			n = 0;
			p2 = p;

			for (; *p != '\0' && n < prec; p++)
			    n++;

			p = p2;

			while (n < length) {
			    (*putc)(putc_arg, ' ');
			    n++;
			}
		    }

		    n = 0;

		    while (*p != '\0') {
			if (++n > prec)
			    break;

			(*putc)(putc_arg, *p++);
		    }

		    if (n < length && ladjust) {
			while (n < length) {
			    (*putc)(putc_arg, ' ');
			    n++;
			}
		    }

		    break;
		}

		case 'o':
		case 'O':
		    base = 8;
		    goto print_unsigned;

		case 'd':
		case 'D':
		    base = 10;
		    goto print_signed;

		case 'u':
		case 'U':
		    base = 10;
		    goto print_unsigned;

		case 'x':
		case 'X':
		    base = 16;
		    goto print_unsigned;

		case 'z':
		case 'Z':
		    base = 16;
		    goto print_signed;

		case 'r':
		case 'R':
		    base = radix;
		    goto print_signed;

		case 'n':
		    base = radix;
		    goto print_unsigned;

		print_signed:
		    n = va_arg(args, int);
		    if (n >= 0) {
			u = n;
			sign_char = plus_sign;
		    }
		    else {
			u = -n;
			sign_char = '-';
		    }
		    goto print_num;

		print_unsigned:
		    u = va_arg(args, unsigned int);
		    goto print_num;

		print_num:
		{
		    char	buf[MAXBUF];	/* build number here */
		    register char *	p = &buf[MAXBUF-1];
		    static char digits[] = "0123456789abcdef";
		    char *prefix = 0;

		    if (u != 0 && altfmt) {
			if (base == 8)
			    prefix = "0";
			else if (base == 16)
			    prefix = "0x";
		    }

		    do {
			*p-- = digits[u % base];
			u /= base;
		    } while (u != 0);

		    length -= (&buf[MAXBUF-1] - p);
		    if (sign_char)
			length--;
		    if (prefix)
			length -= strlen(prefix);

		    if (padc == ' ' && !ladjust) {
			/* blank padding goes before prefix */
			while (--length >= 0)
			    (*putc)(putc_arg, ' ');
		    }
		    if (sign_char)
			(*putc)(putc_arg, sign_char);
		    if (prefix)
			while (*prefix)
			    (*putc)(putc_arg, *prefix++);
		    if (padc == '0') {
			/* zero padding goes after sign and prefix */
			while (--length >= 0)
			    (*putc)(putc_arg, '0');
		    }
		    while (++p != &buf[MAXBUF])
			(*putc)(putc_arg, *p);

		    if (ladjust) {
			while (--length >= 0)
			    (*putc)(putc_arg, ' ');
		    }
		    break;
		}

		case '\0':
		    fmt--;
		    break;

		default:
		    (*putc)(putc_arg, *fmt);
	    }
	fmt++;
	}
}

/*
 *  ROUTINE:	init_printing
 *
 *  FUNCTION:	initializes the locks and semaphores controlling printing.
 */
	
void
init_rthread_printf()
{
    kern_return_t kr;

    print_buffer_index = 0;
    print_buffer[print_buffer_index] = 0;

    MACH_CALL(lock_set_create(mach_task_self(),
			      &print_lock, 1, SYNC_POLICY_FIFO), kr);
    MACH_CALL(semaphore_create(mach_task_self(),
			       &printer_ready, SYNC_POLICY_FIFO, 0), kr);
    MACH_CALL(semaphore_create(mach_task_self(),
			       &printer_waiting, SYNC_POLICY_FIFO, 0), kr);
}

void sendchar(char *s, int c)
{
    kern_return_t kr;
    if (print_buffer_index == (PRINT_BUFFER_SIZE-1))
    {
	MACH_CALL(semaphore_signal(printer_waiting) , kr);
	MACH_CALL(semaphore_wait(printer_ready), kr);
    }
    print_buffer[print_buffer_index] = c;
    print_buffer_index++;
}

int vrprintf(char *fmt, va_list args)
{
    kern_return_t kr;

    MACH_CALL(lock_acquire(print_lock, PRINT_LOCK_ID), kr);
    MACH_CALL(semaphore_wait(printer_ready), kr);
    rthread_doprnt(fmt, args, 0, (void (*)(char *, int)) sendchar, (char *) 0);
    MACH_CALL(semaphore_signal(printer_waiting) , kr);
    MACH_CALL(lock_release(print_lock, PRINT_LOCK_ID), kr);
}

int
rprintf(char *fmt, ...)
{
    va_list	args;
    va_start(args, fmt);
    if (_rthread_self()->status & T_MAIN)
	vprintf(fmt, args);
    else
	vrprintf(fmt, args);
    va_end(args);    
}

void
process_printf()
{
    kern_return_t kr;

    MACH_CALL(semaphore_signal(printer_ready), kr);
    MACH_CALL(semaphore_wait(printer_waiting), kr);
    print_buffer[print_buffer_index] = 0;
    printf("%s", print_buffer);
    print_buffer_index = 0;
}

int *
__mach_errno_addr(void)
{
    return &_rthread_self()->err_no;
}
