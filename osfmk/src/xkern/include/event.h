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
 * event.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * event.h,v
 * Revision 1.22.1.2.1.2  1994/09/07  04:18:04  menze
 * OSF modifications
 *
 * Revision 1.22.1.2  1994/09/01  04:07:56  menze
 * Added evAlloc, changed interface and semantics of evSchedule
 *
 * Revision 1.22  1994/01/10  17:52:33  menze
 *   [ 1994/01/04          menze ]
 *   Added an additional field to struct Event for per-platform state
 *
 */

#ifndef event_h
#define event_h

#include <xkern/include/upi.h>
#include <xkern/include/xtime.h>
#include <xkern/include/domain.h>

/*
 *   Return values for evCancel
 */
typedef enum {
    EVENT_FINISHED = -1,
    EVENT_RUNNING = 0,
    EVENT_CANCELLED = 1
} EvCancelReturn;

/*
 * This structure is to be opaque.  We have to define it here, but protocol
 * implementors are not allowed to know what it contains.
 */
typedef enum {
    E_INIT,
    E_PENDING,
    E_SCHEDULED,
    E_RUNNING,
    E_BLOCKED,
    E_FINISHED
} EvState;


#if XK_DEBUG
#  define XK_THREAD_TRACE
#endif /* XK_DEBUG */   

#ifdef XK_THREAD_TRACE
#  define EV_CHECK_STACK(_str)  evCheckStack(_str)
#else
#  define EV_CHECK_STACK(_str)  do {} while(0)
#endif

typedef	void	(* EvFunc)(Event, void *);

typedef struct event {
    queue_chain_t link;
#define evnext	link.next
#define evprev	link.prev
    unsigned 	deltat;
    EvFunc	func;
    void	*arg;
    EvState	state;
    struct {
	unsigned detached: 1;
	unsigned cancelled: 1;
	unsigned rescheduled: 1;
    } flags;
#ifdef XK_THREAD_TRACE
    XTime	startTime;	/* Time started */
    XTime	stopTime;	/* Time stopped or blocked */
    Bind	bind;
    void        *earlyStack;
#endif
    void	*extra;  /* any platform-specific state */
} Event_s;





/* 
 * Allocates resources for a new event, returning the event handle if
 * successful or 0 if there were allocation failures.  The event must
 * be scheduled with evSchedule to actually run.
 */
Event
evAlloc( Path );


/*
 * Schedule (or reschedule) an event that executes f w/ argument a
 * after delay t usec; t may equal 0, which implies createprocess.
 * If the event is currently running, it will be allowed to finish
 * before the rescheduling takes effect.
 *
 * Returns 0 if an invalid event is passed in, otherwise returns the
 * event itself. 
 */
Event
evSchedule( Event, EvFunc f, void *a, unsigned t );


/*
 * releases a handle to an event; as soon f completes, the resources
 * associated with the event are freed
 */
void evDetach(Event e);

/* cancel event e:
 *  returns EVENT_FINISHED if it knows that the event has already executed
 *     to completion
 *  returns EVENT_RUNNING if the event is currently running
 *  returns EVENT_CANCELLED if the event has been successfully cancelled
 */
EvCancelReturn evCancel(Event e);


/* 
 * returns true (non-zero) if an 'evCancel' has been performed on the event
 */
int evIsCancelled(Event e);


/* 
 * Displays a 'ps'-style listing of x-kernel threads
 */
void evDump(void);


/*
 * Platform-specific check to see if this event is in danger of
 * overrunning the stack, triggering warning/error messages if so.
 */
void evCheckStack(char *);


#endif
