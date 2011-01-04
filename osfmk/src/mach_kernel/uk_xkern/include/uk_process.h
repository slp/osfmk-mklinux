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
 * Adapted from process.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * process.h,v
 * Revision 1.20.1.2  1994/09/01  04:20:51  menze
 * Subsystem initialization functions can fail
 */

#ifndef uk_process_h
#define uk_process_h

#include <cpus.h>
#include <kern/lock.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <mach/message.h>
#include <xkern/include/xtype.h>
#include <xkern/include/xk_thread.h>

/* define standard priority; higher numbers have less priority */
#define STD_PRIO	BASEPRI_SYSTEM
/*
 * define priorities; higher numbers have less priority
 */

#define XK_THREAD_PRIORITY_MIN		BASEPRI_KERNEL
#define XK_THREAD_PRIORITY_MAX		BASEPRI_SYSTEM

#define XK_INPUT_THREAD_PRIORITY_MIN	XK_THREAD_PRIORITY_MIN
#define XK_INPUT_THREAD_PRIORITY_MAX	XK_THREAD_PRIORITY_MAX
#define	XK_INPUT_THREAD_PRIORITY	XK_THREAD_PRIORITY_MAX
#define	XK_CLOCK_THREAD_PRIORITY	XK_THREAD_PRIORITY_MIN
#define	XK_EVENT_THREAD_PRIORITY	XK_THREAD_PRIORITY_MAX


typedef struct sxkSemaphore {
	int			count;
} xkSemaphore;

#define semWait(S) { if (--(S)->count < 0) realP(S); }
#define semSignal(S) { if (++(S)->count <= 0) realV(S); }

extern void semInit(xkSemaphore *, unsigned int);
extern void realP(xkSemaphore *);
extern void realV(xkSemaphore *);

/*
 * Main synchronization hook (the very coarsest locking!).
 */
decl_simple_lock_data(extern, xkMaster_lock)

#if XK_DEBUG

/* The -DXK_DEBUG=1 switch provides for counting the locking depth, and
   complaining if it isn't 1.  It has a bug, in that the increment & decrement
   instructions aren't atomic in RISC architectures.  Presumably, the occasions
   where this fails are rare.  */

extern int xklockdepthreq;
extern int xklockdepth;
extern int tracexklock;

#define	xk_master_lock()						      \
{xklockdepthreq++;							      \
 xTrace1(xklock,TR_EVENTS,"requesting xklock, depthreq %d",xklockdepthreq);   \
 simple_lock( &xkMaster_lock );						      \
 xklockdepth++;								      \
 if (xklockdepth!=1)							      \
 {xTrace1(xklock,TR_ERRORS,"got xklock, wrong depth %d",xklockdepth); };      \
 xTrace2(xklock,TR_EVENTS,"got xklock, depth %d, depthreq %d",		      \
	 xklockdepth,xklockdepthreq); }
#define	xk_master_unlock()						      \
{if (xklockdepth!=1)							      \
 {xTrace1(xklock,TR_ERRORS,"giving up xklock, wrong depth %d",xklockdepth); };\
 xTrace2(xklock,TR_EVENTS,"giving up xklock, depth %d, depthreq %d",	      \
	 xklockdepth,xklockdepthreq);					      \
 xklockdepth--;								      \
 simple_unlock( &xkMaster_lock );					      \
 xTrace1(xklock,TR_EVENTS,"gave up xklock, depthreq %d",xklockdepthreq);      \
 xklockdepthreq--; }

#else /* XK_DEBUG */

#define	xk_master_lock()	simple_lock( &xkMaster_lock )  
#define	xk_master_unlock()	simple_unlock( &xkMaster_lock )
#define	xk_master_trylock()	simple_lock_try( &xkMaster_lock ) 

#endif /* XK_DEBUG */

/* 
 * Types of x-kernel thread
 */
#define XK_INPUT_THREAD    1

#define set_xk_input_thread(thread)\
                thread->xk_type = XK_INPUT_THREAD;

#define is_xk_input_thread(thread)\
		(thread->xk_type == XK_INPUT_THREAD)

/*
 * We are going to make this procedure much smarter 
 * (taking in account SMP and preemption, we can understand
 * whether it is better to queue or to spawn a new thread)
 */
#define more_input_thread(pool)\
                (pool->threads_out == pool->threads_max)

#define XK_SERVICE_THREAD  2
#define MAX_XK_SERVICE_THREADS 4

#define	set_xk_service_thread(thread)\
		thread->xk_type = XK_SERVICE_THREAD;

#define is_xk_service_thread(thread)\
		(thread->xk_type == XK_SERVICE_THREAD)

extern int xk_service_thread_out;
extern queue_head_t	xkInfoQ;

/*
 * We are going to make this procedure much smarter 
 * (taking in account SMP and preemption, we can understand
 * whether it is better to queue or to spawn a new thread)
 */
#define more_service_thread()      (xk_service_thread_out)

/*
 * The following utilities are used when a shepherd thread
 * exits or enters the x-kernel world.
 */
void
xk_thread_checkout(
		   boolean_t checkqueue);

void
xk_thread_checkin(
		  void);


typedef void (*gen_func) (void *, ...);

xkern_return_t	CreateProcess( gen_func, Event ev, int numArgs, ... );
xkern_return_t	threadInit( void );
void		Delay( int );
xkern_return_t	procAllocState( Event, Allocator );
void		procFreeState( Event );

XkThreadPolicyFunc	xkPolicyFixedRR;
XkThreadPolicyFunc	xkPolicyFixedFifo;

#endif  /* uk_process_h */
