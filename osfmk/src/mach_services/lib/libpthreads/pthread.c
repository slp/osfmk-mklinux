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
 * 
 */
/*
 * MkLinux
 */

/*
 * POSIX Pthread Library
 */

#include "pthread_internals.h"
#include <stdio.h>	/* For printf(). */
#include <errno.h>	/* For __mach_errno_addr() prototype. */

/*
 * [Internal] stack support
 */

int _pthread_stack_size;
/* This 'shadow' stack size is used to prevent the user from changing the fundamental  
 * stack size after some threads have been created.  Since we use the stack for thread 
 * data structures, the stack size must be well known at all times and cannot vary from
 * thread to thread.
 */
static int __pthread_stack_size, __pthread_stack_mask;
static vm_address_t lowest_stack;
static vm_address_t *free_stacks;

#define STACK_LOWEST(sp)	((sp) & ~__pthread_stack_mask)
#define STACK_RESERVED		(sizeof (struct _pthread))

#ifdef STACK_GROWS_UP

/* The stack grows towards higher addresses:
   |struct _pthread|user stack---------------->|
   ^STACK_BASE     ^STACK_START
   ^STACK_SELF
   ^STACK_LOWEST  */
#define STACK_BASE(sp)		STACK_LOWEST(sp)
#define STACK_START(stack_low)	(STACK_BASE(stack_low) + STACK_RESERVED)
#define STACK_SELF(sp)		STACK_BASE(sp)

#else

/* The stack grows towards lower addresses:
   |<----------------user stack|struct _pthread|
   ^STACK_LOWEST               ^STACK_START    ^STACK_BASE
			       ^STACK_SELF  */

#define STACK_BASE(sp)		(((sp) | __pthread_stack_mask) + 1)
#define STACK_START(stack_low)	(STACK_BASE(stack_low) - STACK_RESERVED)
#define STACK_SELF(sp)		STACK_START(sp)

#endif


static vm_address_t
_pthread_allocate_stack(void)
{
	vm_address_t cur_stack;
	kern_return_t kr;
	if (free_stacks == 0)
	{
	    /* Allocating guard pages is done by doubling
	     * the actual stack size, since STACK_BASE() needs
	     * to have stacks aligned on stack_size. Allocating just 
	     * one page takes as much memory as allocating more pages
	     * since it will remain one entry in the vm map.
	     * Besides, allocating more than one page allows tracking the
	     * overflow pattern when the overflow is bigger than one page.
	     */
#ifndef	NO_GUARD_PAGES
# define	GUARD_SIZE(a)	(2*(a))
# define	GUARD_MASK(a)	(((a)<<1) | 1)
#else
# define	GUARD_SIZE(a)	(a)
# define	GUARD_MASK(a)	(a)
#endif
		while (lowest_stack > GUARD_SIZE(__pthread_stack_size))
		{
			lowest_stack -= GUARD_SIZE(__pthread_stack_size);
			/* Ensure stack is there */
			kr = vm_allocate(mach_task_self(),
					 &lowest_stack,
					 GUARD_SIZE(__pthread_stack_size),
					 FALSE);
#ifndef	NO_GUARD_PAGES
			if (kr == KERN_SUCCESS) {
# ifdef	STACK_GROWS_UP
			    kr = vm_protect(mach_task_self(),
					    lowest_stack+__pthread_stack_size,
					    __pthread_stack_size,
					    FALSE, VM_PROT_NONE);
# else	/* STACK_GROWS_UP */
			    kr = vm_protect(mach_task_self(),
					    lowest_stack,
					    __pthread_stack_size,
					    FALSE, VM_PROT_NONE);
			    lowest_stack += __pthread_stack_size;
# endif	/* STACK_GROWS_UP */
			    if (kr == KERN_SUCCESS)
				break;
			}
#else
			if (kr == KERN_SUCCESS)
			    break;
#endif
		}
		if (lowest_stack > 0)
			free_stacks = (vm_address_t *)lowest_stack;
		else
		{
			/* Too bad.  We'll just have to take what comes.
			   Use vm_map instead of vm_allocate so we can
			   specify alignment.  */
			kr = vm_map(mach_task_self(), &lowest_stack,
				    GUARD_SIZE(__pthread_stack_size),
				    GUARD_MASK(__pthread_stack_mask),
				    TRUE /* anywhere */, MEMORY_OBJECT_NULL,
				    0, FALSE, VM_PROT_DEFAULT, VM_PROT_ALL,
				    VM_INHERIT_DEFAULT);
			/* This really shouldn't fail and if it does I don't
			   know what to do.  */
#ifndef	NO_GUARD_PAGES
			if (kr == KERN_SUCCESS) {
# ifdef	STACK_GROWS_UP
			    kr = vm_protect(mach_task_self(),
					    lowest_stack+__pthread_stack_size,
					    __pthread_stack_size,
					    FALSE, VM_PROT_NONE);
# else	/* STACK_GROWS_UP */
			    kr = vm_protect(mach_task_self(),
					    lowest_stack,
					    __pthread_stack_size,
					    FALSE, VM_PROT_NONE);
			    lowest_stack += __pthread_stack_size;
# endif	/* STACK_GROWS_UP */
			}
#endif
			free_stacks = (vm_address_t *)lowest_stack;
			lowest_stack = 0;
		}
		*free_stacks = 0; /* No other free stacks */
	}
	cur_stack = STACK_START((vm_address_t) free_stacks);
	free_stacks = (vm_address_t *)*free_stacks;
	cur_stack = _adjust_sp(cur_stack); /* Machine dependent stack fudging */
	return (cur_stack);
}

static void
_pthread_free_stack(pthread_t self)
{
	vm_address_t *base = (vm_address_t *) STACK_LOWEST((vm_address_t)self);
	*base = (vm_address_t) free_stacks;
	free_stacks = base;
}

/*
 * Destroy a thread attribute structure
 */
int       
pthread_attr_destroy(pthread_attr_t *attr)
{
	if (attr->sig == _PTHREAD_ATTR_SIG)
	{
		return (ESUCCESS);
	} else
	{
		return (EINVAL); /* Not an attribute structure! */
	}
}

/*
 * Get the 'detach' state from a thread attribute structure.
 * Note: written as a helper function for info hiding
 */
int       
pthread_attr_getdetachstate(const pthread_attr_t *attr, 
			    int *detachstate)
{
	if (attr->sig == _PTHREAD_ATTR_SIG)
	{
		*detachstate = attr->detached;
		return (ESUCCESS);
	} else
	{
		return (EINVAL); /* Not an attribute structure! */
	}
}

/*
 * Get the 'inherit scheduling' info from a thread attribute structure.
 * Note: written as a helper function for info hiding
 */
int       
pthread_attr_getinheritsched(const pthread_attr_t *attr, 
			     int *inheritsched)
{
	if (attr->sig == _PTHREAD_ATTR_SIG)
	{
		*inheritsched = attr->inherit;
		return (ESUCCESS);
	} else
	{
		return (EINVAL); /* Not an attribute structure! */
	}
}

/*
 * Get the scheduling parameters from a thread attribute structure.
 * Note: written as a helper function for info hiding
 */
int       
pthread_attr_getschedparam(const pthread_attr_t *attr, 
			   struct sched_param *param)
{
	if (attr->sig == _PTHREAD_ATTR_SIG)
	{
		*param = attr->param;
		return (ESUCCESS);
	} else
	{
		return (EINVAL); /* Not an attribute structure! */
	}
}

/*
 * Get the scheduling policy from a thread attribute structure.
 * Note: written as a helper function for info hiding
 */
int       
pthread_attr_getschedpolicy(const pthread_attr_t *attr, 
			    int *policy)
{
	if (attr->sig == _PTHREAD_ATTR_SIG)
	{
		*policy = attr->policy;
		return (ESUCCESS);
	} else
	{
		return (EINVAL); /* Not an attribute structure! */
	}
}

/*
 * Initialize a thread attribute structure to default values.
 */
int       
pthread_attr_init(pthread_attr_t *attr)
{
	attr->sig = _PTHREAD_ATTR_SIG;
	attr->policy = _PTHREAD_DEFAULT_POLICY;
	attr->inherit = _PTHREAD_DEFAULT_INHERITSCHED;
	attr->detached = PTHREAD_CREATE_JOINABLE;
	return (ESUCCESS);
}

/*
 * Set the 'detach' state in a thread attribute structure.
 * Note: written as a helper function for info hiding
 */
int       
pthread_attr_setdetachstate(pthread_attr_t *attr, 
			    int detachstate)
{
	if (attr->sig == _PTHREAD_ATTR_SIG)
	{
		if ((detachstate == PTHREAD_CREATE_JOINABLE) ||
		    (detachstate == PTHREAD_CREATE_DETACHED))
		{
			attr->detached = detachstate;
			return (ESUCCESS);
		} else
		{
			return (EINVAL);
		}
	} else
	{
		return (EINVAL); /* Not an attribute structure! */
	}
}

/*
 * Set the 'inherit scheduling' state in a thread attribute structure.
 * Note: written as a helper function for info hiding
 */
int       
pthread_attr_setinheritsched(pthread_attr_t *attr, 
			     int inheritsched)
{
	if (attr->sig == _PTHREAD_ATTR_SIG)
	{
		if ((inheritsched == PTHREAD_INHERIT_SCHED) ||
		    (inheritsched == PTHREAD_EXPLICIT_SCHED))
		{
			attr->inherit = inheritsched;
			return (ESUCCESS);
		} else
		{
			return (EINVAL);
		}
	} else
	{
		return (EINVAL); /* Not an attribute structure! */
	}
}

/*
 * Set the scheduling paramters in a thread attribute structure.
 * Note: written as a helper function for info hiding
 */
int       
pthread_attr_setschedparam(pthread_attr_t *attr, 
			   const struct sched_param *param)
{
	if (attr->sig == _PTHREAD_ATTR_SIG)
	{
		/* TODO: Validate sched_param fields */
		attr->param = *param;
		return (ESUCCESS);
	} else
	{
		return (EINVAL); /* Not an attribute structure! */
	}
}

/*
 * Set the scheduling policy in a thread attribute structure.
 * Note: written as a helper function for info hiding
 */
int       
pthread_attr_setschedpolicy(pthread_attr_t *attr, 
			    int policy)
{
	if (attr->sig == _PTHREAD_ATTR_SIG)
	{
		if ((policy == SCHED_OTHER) ||
		    (policy == SCHED_RR) ||
		    (policy == SCHED_FIFO))
		{
			attr->policy = policy;
			return (ESUCCESS);
		} else
		{
			return (EINVAL);
		}
	} else
	{
		return (EINVAL); /* Not an attribute structure! */
	}
}

/*
 * Create and start execution of a new thread.
 */

static void
_pthread_body(pthread_t self)
{
	pthread_exit((self->fun)(self->arg));
}

static int
_pthread_create(pthread_t t,
		const pthread_attr_t *attrs,
		const thread_port_t kernel_thread)
{
	int res;
	kern_return_t kern_res;
	res = ESUCCESS;
	do
	{
		memset(t, 0, sizeof(*t));
		t->kernel_thread = kernel_thread;
		t->detached = attrs->detached;
		t->inherit = attrs->inherit;
		t->policy = attrs->policy;
		t->param = attrs->param;
		t->mutexes = (struct _pthread_mutex *)NULL;
		t->sig = _PTHREAD_SIG;
		LOCK_INIT(t->lock);
		t->cancel_state = PTHREAD_CANCEL_ENABLE | PTHREAD_CANCEL_DEFERRED;
		t->cleanup_stack = (struct _pthread_handler_rec *)NULL;
		/* Create control semaphores */
		if (t->detached == PTHREAD_CREATE_JOINABLE)
		{
			MACH_CALL(semaphore_create(mach_task_self(), 
						   &t->death, 
						   SYNC_POLICY_FIFO, 
						   0), kern_res);
			if (kern_res != KERN_SUCCESS)
			{
				printf("Can't create 'death' semaphore: %d\n", kern_res);
				res = EINVAL; /* Need better error here? */
				break;
			}
			MACH_CALL(semaphore_create(mach_task_self(), 
						   &t->joiners, 
						   SYNC_POLICY_FIFO, 
						   0), kern_res);
			if (kern_res != KERN_SUCCESS)
			{
				printf("Can't create 'joiners' semaphore: %d\n", kern_res);
				res = EINVAL; /* Need better error here? */
				break;
			}
			t->num_joiners = 0;
		} else
		{
			t->death = MACH_PORT_NULL;
		}
	} while (0);
	return (res);
}

int       
pthread_create(pthread_t *thread, 
	       const pthread_attr_t *attr,
	       void *(*start_routine)(void *), 
	       void *arg)
{
	pthread_attr_t _attr, *attrs;
	vm_address_t stack;
	int res;
	pthread_t t;
	kern_return_t kern_res;
	thread_port_t kernel_thread;
	if ((attrs = (pthread_attr_t *)attr) == (pthread_attr_t *)NULL)
	{			/* Set up default paramters */
		attrs = &_attr;
		pthread_attr_init(attrs);
	}
	res = ESUCCESS;
	do
	{
		/* Allocate a stack for the thread */
		stack = _pthread_allocate_stack();
		/* Thread structure lives at base of stack */
		t = (pthread_t)STACK_SELF(stack);
		*thread = t;
		/* Create the Mach thread for this thread */
		MACH_CALL(thread_create(mach_task_self(), &kernel_thread), kern_res);
		if (kern_res != KERN_SUCCESS)
		{
			printf("Can't create thread: %d\n", kern_res);
			res = EINVAL; /* Need better error here? */
			break;
		}
		if ((res = _pthread_create(t, attrs, kernel_thread)) != 0)
		{
			break;
		}
		t->arg = arg;
		t->fun = start_routine;
		/* Now set it up to execute */
		_pthread_setup(t, _pthread_body, stack);
		/* Send it on it's way */
		MACH_CALL(thread_resume(kernel_thread), kern_res);
		if (kern_res != KERN_SUCCESS)
		{
			printf("Can't resume thread: %d\n", kern_res);
			res = EINVAL; /* Need better error here? */
			break;
		}
	} while (0);
	return (res);
}

/*
 * Make a thread 'undetached' - no longer 'joinable' with other threads.
 */
int       
pthread_detach(pthread_t thread)
{
	kern_return_t kern_res;
	int num_joiners;
	mach_port_t death;
	if (thread->sig == _PTHREAD_SIG)
	{
		LOCK(thread->lock);
		if (thread->detached == PTHREAD_CREATE_JOINABLE)
		{
			thread->detached = PTHREAD_CREATE_DETACHED;
			num_joiners = thread->num_joiners;
			death = thread->death;
			thread->death = MACH_PORT_NULL;
			UNLOCK(thread->lock);
			if (num_joiners > 0)
			{ /* Have to tell these guys this thread can't be joined with */
				MACH_CALL(semaphore_signal_all(thread->joiners), kern_res);
			}
			/* Destroy 'control' semaphores */
			MACH_CALL(semaphore_destroy(mach_task_self(),
						    thread->joiners), kern_res);
			MACH_CALL(semaphore_destroy(mach_task_self(),
						    death), kern_res);
			return (ESUCCESS);
		} else
		{
			UNLOCK(thread->lock);
			return (EINVAL);
		}
	} else
	{
		return (ESRCH); /* Not a valid thread */
	}
}

/*
 * Terminate a thread.
 */
void 
pthread_exit(void *value_ptr)
{
	pthread_t self = pthread_self();
	struct _pthread_handler_rec *handler;
	kern_return_t kern_res;
	int num_joiners;
	while ((handler = self->cleanup_stack) != 0)
	{
		(handler->routine)(handler->arg);
		self->cleanup_stack = handler->next;
	}
	_pthread_tsd_cleanup(self);
	LOCK(self->lock);
	if (self->detached == PTHREAD_CREATE_JOINABLE)
	{
		self->detached = _PTHREAD_EXITED;
		self->exit_value = value_ptr;
		num_joiners = self->num_joiners;
		UNLOCK(self->lock);
		if (num_joiners > 0)
		{
			MACH_CALL(semaphore_signal_all(self->joiners), kern_res);
		}
		MACH_CALL(semaphore_wait(self->death), kern_res);
	} else
		UNLOCK(self->lock);
	/* Destroy thread & reclaim resources */
	if (self->death)
	{
		MACH_CALL(semaphore_destroy(mach_task_self(), self->joiners), kern_res);
		MACH_CALL(semaphore_destroy(mach_task_self(), self->death), kern_res);
	}
	_pthread_free_stack(self);
	MACH_CALL(thread_terminate(mach_thread_self()), kern_res);
}

/*
 * Wait for a thread to terminate and obtain its exit value.
 */
int       
pthread_join(pthread_t thread, 
	     void **value_ptr)
{
	kern_return_t kern_res;
	if (thread->sig == _PTHREAD_SIG)
	{
		LOCK(thread->lock);
		if (thread->detached == PTHREAD_CREATE_JOINABLE)
		{
			thread->num_joiners++;
			UNLOCK(thread->lock);
			MACH_CALL(semaphore_wait(thread->joiners), kern_res);
			LOCK(thread->lock);
			thread->num_joiners--;
		}
		if (thread->detached == _PTHREAD_EXITED)
		{
			if (thread->num_joiners == 0)
			{	/* Give the result to this thread */
				if (value_ptr)
				{
					*value_ptr = thread->exit_value;
				}
				UNLOCK(thread->lock);
				MACH_CALL(semaphore_signal(thread->death), kern_res);
				return (ESUCCESS);
			} else
			{	/* This 'joiner' missed the catch! */
				UNLOCK(thread->lock);
				return (ESRCH);
			}
		} else
		{		/* The thread has become anti-social! */
			UNLOCK(thread->lock);
			return (EINVAL);
		}
	} else
	{
		return (ESRCH); /* Not a valid thread */
	}
}

/*
 * Get the scheduling policy and scheduling paramters for a thread.
 */
int       
pthread_getschedparam(pthread_t thread, 
		      int *policy,
		      struct sched_param *param)
{
	if (thread->sig == _PTHREAD_SIG)
	{
		*policy = thread->policy;
		switch (*policy)
		{
		case SCHED_OTHER:
			break;
		case SCHED_FIFO:
			break;
		case SCHED_RR:
			break;
		default:
			return (EINVAL);
		}
		return (ESUCCESS);
	} else
	{
		return (ESRCH);  /* Not a valid thread structure */
	}
}

/*
 * Set the scheduling policy and scheudling paramters for a thread.
 */
int       
pthread_setschedparam(pthread_t thread, 
		      int policy,
		      const struct sched_param *param)
{
	if (thread->sig == _PTHREAD_SIG)
	{
		switch (policy)
		{
		case SCHED_OTHER:
			break;
		case SCHED_FIFO:
			break;
		case SCHED_RR:
			break;
		default:
			return (EINVAL);
		}
		return (ESUCCESS);
	} else
	{
		return (ESRCH);  /* Not a valid thread structure */
	}
}

/*
 * Determine if two thread identifiers represent the same thread.
 */
int       
pthread_equal(pthread_t t1, 
	      pthread_t t2)
{
	return (t1 == t2);
}

/*
 * Return the thread identifier for the current thread.
 */
pthread_t 
pthread_self(void)
{
	return ((pthread_t)STACK_SELF(_sp()));
}

/*
 * Execute a function exactly one time in a thread-safe fashion.
 */
int       
pthread_once(pthread_once_t *once_control, 
	     void (*init_routine)(void))
{
	LOCK(once_control->lock);
	if (once_control->sig == _PTHREAD_ONCE_SIG_init)
	{
		(*init_routine)();
		once_control->sig = _PTHREAD_ONCE_SIG;
	}
	UNLOCK(once_control->lock);
	return (ESUCCESS);  /* Spec defines no possible errors! */
}

/*
 * Cancel a thread
 */
int
pthread_cancel(pthread_t thread)
{
	if (thread->sig == _PTHREAD_SIG)
	{
		thread->cancel_state |= _PTHREAD_CANCEL_PENDING;
		return (ESUCCESS);
	} else
	{
		return (ESRCH);
	}
}

/*
 * Insert a cancellation point in a thread.
 */
static void
_pthread_testcancel(pthread_t thread)
{
	LOCK(thread->lock);
	if ((thread->cancel_state & (PTHREAD_CANCEL_ENABLE|_PTHREAD_CANCEL_PENDING)) == 
	    (PTHREAD_CANCEL_ENABLE|_PTHREAD_CANCEL_PENDING))
	{
		UNLOCK(thread->lock);
		pthread_exit(0);
	}
	UNLOCK(thread->lock);
}

void
pthread_testcancel(void)
{
	pthread_t self = pthread_self();
	_pthread_testcancel(self);
}

/*
 * Query/update the cancelability 'state' of a thread
 */
int
pthread_setcancelstate(int state, int *oldstate)
{
	pthread_t self = pthread_self();
	int err = ESUCCESS;
	LOCK(self->lock);
	*oldstate = self->cancel_state & _PTHREAD_CANCEL_STATE_MASK;
	if ((state == PTHREAD_CANCEL_ENABLE) || (state == PTHREAD_CANCEL_DISABLE))
	{
		self->cancel_state = (self->cancel_state & _PTHREAD_CANCEL_STATE_MASK) | state;
	} else
	{
		err = EINVAL;
	}
	UNLOCK(self->lock);
	_pthread_testcancel(self);  /* See if we need to 'die' now... */
	return (err);
}

/*
 * Query/update the cancelability 'type' of a thread
 */
int
pthread_setcanceltype(int type, int *oldtype)
{
	pthread_t self = pthread_self();
	int err = ESUCCESS;
	LOCK(self->lock);
	*oldtype = self->cancel_state & _PTHREAD_CANCEL_TYPE_MASK;
	if ((type == PTHREAD_CANCEL_DEFERRED) || (type == PTHREAD_CANCEL_ASYNCHRONOUS))
	{
		self->cancel_state = (self->cancel_state & _PTHREAD_CANCEL_TYPE_MASK) | type;
	} else
	{
		err = EINVAL;
	}
	UNLOCK(self->lock);
	_pthread_testcancel(self);  /* See if we need to 'die' now... */
	return (err);
}

/*
 * Perform package initialization - called automatically when application starts
 */

static int
pthread_init(void)
{
	vm_address_t new_stack;
	pthread_attr_t _attr, *attrs;
	pthread_t thread;
	kern_return_t kr;
	host_basic_info_data_t info;
	mach_msg_type_number_t count;

	/* See if we're on a multiprocessor and set _spin_tries if so.  */
	count = HOST_BASIC_INFO_COUNT;
	kr = host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t) &info,
		       &count);
	if (kr != KERN_SUCCESS)
		printf("host_info failed (%d); probably need privilege.\n", kr);
	else if (info.avail_cpus > 1)
		_spin_tries = SPIN_TRIES;

	if (_pthread_stack_size == 0)
	{
		_pthread_stack_size = _PTHREAD_DEFAULT_STACKSIZE;
	} else
	{ /* Validate user-supplied stack size */
		if (_pthread_stack_size & (_pthread_stack_size-1))
		{  /* Not a power of two!  Can't use it! */
			_pthread_stack_size = _PTHREAD_DEFAULT_STACKSIZE;
		}
	}
	__pthread_stack_size = _pthread_stack_size;
	__pthread_stack_mask = __pthread_stack_size - 1;
	lowest_stack = STACK_LOWEST(_sp());
	attrs = &_attr;
	pthread_attr_init(attrs);
	new_stack = _pthread_allocate_stack();
	thread = (pthread_t)STACK_SELF(new_stack);
	_pthread_create(thread, attrs, mach_thread_self());
	thread->detached = _PTHREAD_CREATE_PARENT;
	return (new_stack);
}

/*
 * Thread-local errno
 */
int *
__mach_errno_addr(void)
{
	return &pthread_self()->err_no;
}

/* This is the "magic" that gets the initialization routine called when the application starts */
int (*_thread_init_routine)(void) = pthread_init;
