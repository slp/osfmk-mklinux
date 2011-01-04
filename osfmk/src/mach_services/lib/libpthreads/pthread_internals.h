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
 * POSIX Threads - IEEE 1003.1c
 */

#ifndef _POSIX_PTHREAD_INTERNALS_H
#define _POSIX_PTHREAD_INTERNALS_H

#include <mach/port.h>
#include <mach/message.h>
#include <mach/machine/vm_types.h>
#include <mach/std_types.h>
#include <mach/policy.h>
#include <mach/sync.h>
#include <mach/sync_policy.h>
#include <mach/mach_traps.h>
#include <mach/thread_switch.h>
#include <mach/mach_host.h>
#include <mach.h>			/* For generic MACH support */
#include "posix_sched.h"		/* For POSIX scheduling policy & parameter */
#include "pthread_machdep.h"		/* Machine-dependent definitions. */

/*
 * Compiled-in limits
 */

#define PTHREAD_DESTRUCTOR_ITERATIONS 4
#define _POSIX_THREAD_KEYS_MAX	      16

/*
 * Threads
 */
typedef struct _pthread
{
	long	       sig;	      /* Unique signature for this structure */
	pthread_lock_t lock;	      /* Used for internal mutex on structure */
	int	       detached;
	int	       inherit;
	int	       policy;
	struct sched_param param;
	struct _pthread_mutex *mutexes;
	mach_port_t    joiners;	      /* pthread_join() uses this to wait for death's call */
	int	       num_joiners;
	void	       *exit_value;
	mach_port_t    death;	      /* pthread_exit() uses this to wait for the hearse */
	mach_port_t    kernel_thread; /* kernel thread this thread is bound to */
	void	       *(*fun)(void*);/* Argment for child routine */
	void	       *arg;	      /* Argment for child routine */
	int	       cancel_state;  /* Whether thread can be cancelled */
	struct _pthread_handler_rec *cleanup_stack;
	int		err_no;		/* thread-local errno */
	void	       *tsd[_POSIX_THREAD_KEYS_MAX];  /* Thread specific data */
	int I_dont_know_what_else_goes_here_yet;
} *pthread_t;

/*
 * Thread attributes
 */
typedef struct 
{
	long	       sig;	      /* Unique signature for this structure */
	pthread_lock_t lock;	      /* Used for internal mutex on structure */
	int	       detached;
	int	       inherit;
	int	       policy;
	struct sched_param param;
	int I_dont_know_what_else_goes_here_yet;
} pthread_attr_t;

/*
 * Mutex attributes
 */
typedef struct 
{
	long sig;		     /* Unique signature for this structure */
	int prioceiling;
	int protocol;
} pthread_mutexattr_t;

/*
 * Mutex variables
 */
typedef struct _pthread_mutex
{
	long	       sig;	      /* Unique signature for this structure */
	pthread_lock_t lock;	      /* Used for internal mutex on structure */
	int	       prioceiling;
	int	       priority;      /* Priority to restore when mutex unlocked */
	int	       protocol;
	pthread_t      owner;	      /* Which thread has this mutex locked */
	struct _pthread_mutex *next, *prev;  /* List of other mutexes he owns */
	struct _pthread_cond *busy;   /* List of condition variables using this mutex */
	int	       waiters;	      /* Count of threads waiting for this mutex */
	mach_port_t    sem;	      /* Semaphore used for waiting */
} pthread_mutex_t;

/*
 * Condition variable attributes
 */
typedef struct 
{
	long	       sig;	     /* Unique signature for this structure */
	int unsupported;
} pthread_condattr_t;

/*
 * Condition variables
 */
typedef struct _pthread_cond
{
	long	       sig;	     /* Unique signature for this structure */
	pthread_lock_t lock;	     /* Used for internal mutex on structure */
	mach_port_t    sem;	     /* Kernel semaphore */
	struct _pthread_cond *next, *prev;  /* List of condition variables using mutex */
	struct _pthread_mutex *busy; /* mutex associated with variable */
	int	       waiters;	     /* Number of threads waiting */
} pthread_cond_t;

/*
 * Initialization control (once) variables
 */
typedef struct 
{
	long	       sig;	      /* Unique signature for this structure */
	pthread_lock_t lock;	      /* Used for internal mutex on structure */
} pthread_once_t;

#include "pthread.h"

#define _PTHREAD_DEFAULT_INHERITSCHED	PTHREAD_INHERIT_SCHED
#define _PTHREAD_DEFAULT_PROTOCOL	PTHREAD_PRIO_NONE
#define _PTHREAD_DEFAULT_PRIOCEILING	0
#define _PTHREAD_DEFAULT_POLICY		SCHED_FIFO
#define _PTHREAD_DEFAULT_STACKSIZE	0x10000	  /* 64K */

#define _PTHREAD_NO_SIG			0x00000000
#define _PTHREAD_MUTEX_ATTR_SIG		0x4D545841  /* 'MTXA' */
#define _PTHREAD_MUTEX_SIG		0x4D555458  /* 'MUTX' */
#define _PTHREAD_MUTEX_SIG_init		0x32AAABA7  /* [almost] ~'MUTX' */
#define _PTHREAD_COND_SIG		0x434F4E44  /* 'COND' */
#define _PTHREAD_COND_SIG_init		0x3CB0B1BB  /* [almost] ~'COND' */
#define _PTHREAD_ATTR_SIG		0x54484441  /* 'THDA' */
#define _PTHREAD_ONCE_SIG		0x4F4E4345  /* 'ONCE' */
#define _PTHREAD_ONCE_SIG_init		0x30B1BCBA  /* [almost] ~'ONCE' */
#define _PTHREAD_SIG			0x54485244  /* 'THRD' */

#define _PTHREAD_EXITED		     3
#define _PTHREAD_CREATE_PARENT	     4

#define _PTHREAD_CANCEL_STATE_MASK   0xFE
#define _PTHREAD_CANCEL_TYPE_MASK    0xFD
#define _PTHREAD_CANCEL_PENDING	     0x10  /* pthread_cancel() has been called for this thread */

/* Internal mutex locks for data structures */
#define LOCK(v)	  _spin_lock((void *)&v)
#define UNLOCK(v) _spin_unlock((void *)&v)
/* Number of times to spin when the lock is unavailable and we are on a
   multiprocessor.  On a uniprocessor we yield the processor immediately.  */
#define SPIN_TRIES 10
extern int _spin_tries;

/* Convenience symbols */
#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif
#ifndef NULL
#define NULL  0
#endif

#ifndef MACH_CALL
#define MACH_CALL(expr, ret) (ret) = (expr)
#endif

/* Prototypes. */

/* Functions defined in machine-dependent files. */
extern vm_address_t _sp(void);
extern vm_address_t _adjust_sp(vm_address_t sp);
extern void _spin_lock(pthread_lock_t *lockp);
extern void _spin_unlock(pthread_lock_t *lockp);
extern void _pthread_setup(pthread_t th, void (*f)(pthread_t), vm_address_t sp);

extern void _pthread_tsd_cleanup(pthread_t self);

#endif /* _POSIX_PTHREAD_INTERNALS_H */
