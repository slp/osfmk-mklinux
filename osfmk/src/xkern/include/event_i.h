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
 * event_i.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * event_i.h,v
 * Revision 1.2.1.2.1.2  1994/09/07  04:18:13  menze
 * OSF modifications
 *
 * Revision 1.2.1.2  1994/09/01  04:16:41  menze
 * Added evDelay, removed flag definitions
 *
 */

/* 
 * Event definitions for modules which work very closely with the
 * event subsystem.  
 */

#ifndef event_i
#define event_i


#define THIS_IS_THE_HEADER ((EvFunc)-42)

/* BIG_N should be a function of the estimated number of scheduled events,
 * such that queues don't get too long, but with as little overhead as
 * possible. 128 is probably good for up to some 500-1000 events. [mats]
 */
#define BIG_N 256
#define DELAY_BUCKET BIG_N


extern struct event    evhead[];
extern int event_granularity;
extern int tickmod;
extern int tracetick;
extern int traceevent;


#ifdef XK_THREAD_TRACE

extern Map              localEventMap;
extern Map      	threadEventMap;

/* 
 * evMarkBlocked -- mark the current thread as blocked on the given
 * semaphore.   This is only used for thread monitoring.  The pointer
 * will just be treated as an integer (i.e., it doesn't have to be
 * valid.) 
 */
void	evMarkBlocked( xkSemaphore * );

/* 
 * evMarkRunning -- mark the current thread as running.
 * This is only used for thread monitoring.  
 */
void	evMarkRunning( void );

/* 
 * localEventMap -- contains all x-kernel event structures.  Event
 * pointer is both the key and the bound object
 */
extern Map	localEventMap;


#endif


void	evDelay( int );

void    evClock_setup(void); 
void    evClock(int); 
void    xk_event_stub(Event e);

#ifdef  MACH_KERNEL

#define THREAD_T        thread_t
#define THREAD_SELF()   current_thread()

#if NCPUS > 1
decl_simple_lock_data(, eLock)
#define EVENT_LOCK_INIT() simple_lock_init(&eLock, ETAP_XKERNEL_EVENT);
#define EVENT_LOCK()     simple_lock(&eLock)
#define EVENT_UNLOCK()   simple_unlock(&eLock)
#else  /* NCPUS == 1 */
#define EVENT_LOCK_INIT()
#define EVENT_LOCK()
#define EVENT_UNLOCK()
#endif /* NCPUS == 1 */

#else /* !MACH_KERNEL */

#define THREAD_T        cthread_t
#define THREAD_SELF()   cthread_self()
mutex_t                  event_mutex;
#define EVENT_LOCK()     mutex_lock(event_mutex)
#define EVENT_UNLOCK()   mutex_unlock(event_mutex)

#endif  /* MACH_KERNEL */

extern int event_granularity;
extern int tickmod;
extern int tracetick;
extern int traceevent;



#endif /* endif_i.h */

