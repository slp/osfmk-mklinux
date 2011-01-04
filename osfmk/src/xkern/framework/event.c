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
 * event.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * event.c,v
 * Revision 1.38.1.3.1.2  1994/09/07  04:18:13  menze
 * OSF modifications
 *
 * Revision 1.38.1.3  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.38.1.2  1994/09/01  04:19:04  menze
 * Events are now allocated with evAlloc and may be scheduled an
 * arbitrary number of times with evSchedule
 *
 * Added support for Delay (user-space) via evDelay
 *
 * Revision 1.38.1.1  1994/08/08  23:20:56  menze
 * Uses allocators for direct allocations
 *
 * Revision 1.38  1994/02/05  00:11:36  menze
 *   [ 1994/01/28          menze ]
 *   xk_assert.h renamed to assert.h
 *
 */

/*
 * Event library for Mach 3.0 platform [mats]
 */

#ifdef	MACH_KERNEL
#include <cpus.h>
#endif  /* MACH_KERNEL */
#include <xkern/include/domain.h>
#include <mach/message.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/upi.h>
#include <xkern/include/platform.h>
#include <xkern/include/assert.h>
#include <xkern/include/xk_path.h>
#include <xkern/include/event.h>
#include <xkern/include/event_i.h>

struct event	evhead[BIG_N + 1];

int tickmod;
int tracetick;
int traceevent;

#ifdef XK_THREAD_TRACE

/* 
 * Keeps track of all x-kernel events
 */
Map		localEventMap;

/* 
 * Maps native threads to x-kernel events.  Binding to this map may
 * fail in the face of resource shortages, so not all x-kernel events
 * may be found in this map.  
 */
Map		threadEventMap;

#endif

static void	schedule_i( Event e, unsigned time );
static Event	evSelf(void);

/*
 *  e_insque  -- insert an event into an event bucket, ordered by
 *               increasing timeout value
 */

static void
e_insque(
	int q,
	Event e)
{
    Event a;

    xTrace0(event,TR_FULL_TRACE,"e_insque enter");
    EVENT_LOCK();
    for (a = (Event)evhead[q].evnext; a != &evhead[q]; a = (Event)a->evnext) {
        if (a->deltat < e->deltat) {
            /* E goes after a */
            e->deltat -= a->deltat;
            continue;
	} else {
            /* E goes just before a */
	    a->deltat -= e->deltat;
	    insque((queue_entry_t) e, (queue_entry_t) a->evprev);
	    EVENT_UNLOCK();
	    return;
	}
    }
    /* E goes at the end */
    insque((queue_entry_t) e, (queue_entry_t) evhead[q].evprev);
    EVENT_UNLOCK();
}

/*
 * e_remque  --- remove an event from an event bucket
 *
 * Access to the event queues is synchronized through EVENT_LOCK.
 * One must hold the lock in order to invoke e_remque outside of the
 * evClock handler.
 */

static void
e_remque(
	Event e)
{
    xTrace0(event,TR_FULL_TRACE,"e_remque enter");
    xAssert(e);
    if (((Event)(e->evnext))->func != THIS_IS_THE_HEADER) {
	((Event)(e->evnext))->deltat += e->deltat;
    }
    remque((queue_entry_t) e);
}


#ifdef XK_THREAD_TRACE

/* 
 * It is safe to call unbindEvent multiple times with the same event.
 * Only the first call will have any effect.
 */
#define unbindEvent(_e)						\
do {								\
    xAssert(_e);						\
    if ((_e)->bind) {						\
	mapRemoveBinding(threadEventMap, (_e)->bind);		\
	(_e)->bind = 0;						\
    }								\
} while(0)

#define freeEvent(_e)					\
do {							\
    xAssert(_e);					\
    unbindEvent(_e);					\
    mapUnbind(localEventMap, &(_e));			\
    mapReserve(threadEventMap, -1);			\
    procFreeState(_e);					\
    allocFree(_e);					\
} while (0)


#else /* ! XK_THREAD_TRACE */

#define unbindEvent(_e) do { } while (0)
#define freeEvent(_e)	{procFreeState(_e); allocFree((_e)); }

#endif /* XK_THREAD_TRACE */


static Event
evSelf()
{
#ifdef XK_THREAD_TRACE
    Event	ev;
    THREAD_T	thSelf;

    thSelf = THREAD_SELF();
    if ( mapResolve(threadEventMap, &thSelf, &ev) == XK_SUCCESS ) {
	return ev;
    } else {
	xTrace0(event, TR_ERRORS, "evSelf ... couldn't find event structure");
	return 0;
    }
#else
    return 0;
#endif
}


#ifdef XK_THREAD_TRACE
void
evMarkBlocked(
    	xkSemaphore *s)
{
    Event	ev;

    ev = evSelf();
    if ( ev ) {
	ev->state = E_BLOCKED;
	xGetTime(&ev->stopTime);
    }
}

void
evMarkRunning(
	void)
{
    Event	ev;

    ev = evSelf();
    if ( ev ) {
	ev->state = E_RUNNING;
    }
}
#endif /* XK_THREAD_TRACE */


/*
 *  xk_event_stub -- all scheduled events start here
 *
 *  CreateProcess will invoke stub, and stub will apply the function
 *  pointer to the event object and the argument
 *
 */
void
xk_event_stub(
	Event e)
{
  xTrace0(event,TR_FULL_TRACE,"event stub enter");
  if ( e->flags.rescheduled ) {
      xTrace1(event, TR_EVENTS, "Rescheduling event %x", (long)e);
      e->flags.rescheduled = 0;
      schedule_i(e, e->deltat);
      return;
  }
  xAssert( e->state == E_SCHEDULED );
#ifdef XK_THREAD_TRACE
    xAssert( mapResolve(localEventMap, &e, 0) == XK_SUCCESS );
#endif
  if ( e->flags.cancelled ) {
      xTrace1(event, TR_MORE_EVENTS,
	      "event stub not starting cancelled event %x", e);
      if ( e->flags.detached ) {
	  freeEvent(e);
      } else {
	  e->state = E_INIT;
	  e->flags.cancelled = 0;
      }
      return;
  }
#ifdef XK_THREAD_TRACE
    {
	THREAD_T		self;
	xkern_return_t		xkr;
	
	self = THREAD_SELF();
	/* 
	 * We reserved space, so this shouldn't fail.
	 */
	xkr = mapBind(threadEventMap, &self, e, &e->bind);
	xAssert( xkr == XK_SUCCESS ) ;
	e->earlyStack = &self;
	xTrace1(event, TR_MORE_EVENTS, "initial event stack == %x",
		(int)e->earlyStack);
	xGetTime(&e->startTime);
    }
#endif
  e->state = E_RUNNING;
  xTrace1(event,TR_FULL_TRACE, "starting event at addr %x", e->func);
  e->func(e, e->arg);
  e->state = E_FINISHED;
  if ( e->flags.rescheduled ) {
      /* 
       * Reschedule the event using this event structure.  
       */
      xTrace1(event, TR_EVENTS, "Rescheduling event %x", (long)e);
      xAssert(! e->flags.cancelled);
      e->flags.rescheduled = 0;
      EVENT_LOCK();
      unbindEvent(e);
      EVENT_UNLOCK();
      schedule_i(e, e->deltat);
  } else {
      EVENT_LOCK();
      if ( e->flags.detached ) {
	  freeEvent(e);
      } else {
	  e->flags.cancelled = 0;
	  unbindEvent(e);
#ifdef XK_THREAD_TRACE
	  xGetTime(&e->stopTime);
#endif
      }
      EVENT_UNLOCK();
  }
}


Event
evAlloc( Path path )
{
    Event 	e;
    Allocator	a = pathGetAlloc(path, MD_ALLOC);
    
    if ( ! (e = (Event)allocGet(a, sizeof(struct event))) ) {
	xTrace0(event, TR_SOFT_ERRORS, "event allocation failure");
	return 0;
    }
    bzero((char *)e, sizeof(struct event));
    if ( procAllocState(e, a) != XK_SUCCESS ) {
	xTrace0(event, TR_SOFT_ERRORS, "event process state alloc failure");
	allocFree(e);
	return 0;
    }
    e->state = E_INIT;

#ifdef XK_THREAD_TRACE
    {
	xTrace1(event, TR_MORE_EVENTS, "evSchedule adds event %x to maps", e);
	xAssert(localEventMap);
	/* 
	 * XXX -- it would be nice to allocate this binding from the
	 * allocator we were passed if the system allocator is hosed ...
	 */
	if ( mapBind(localEventMap, &e, e, 0) != XK_SUCCESS ) {
	    xTrace0(event, TR_SOFT_ERRORS, "event map bind failure");
	    allocFree(e);
	    return 0;
	}
	if ( mapReserve(threadEventMap, 1) != XK_SUCCESS ) {
	    xTrace0(event, TR_SOFT_ERRORS, "event map bind failure");
	    mapUnbind(localEventMap, &e);
	    allocFree(e);
	    return 0;
	}
    }
#endif
    return e;
}


/* 
 * Schedule this event -- this function must not fail.
 */
static void
schedule_i( Event e, unsigned time ) /* Time in us */
{
    int delt;
    
#ifdef XK_THREAD_TRACE
    {
	XTime	offset;
      
	offset.sec = time / (1000 * 1000);
	offset.usec = time % (unsigned)(1000 * 1000);
	e->bind = 0;	/* filled in in 'stub' once a native thread is assigned */
	xGetTime(&e->startTime);
	xTrace2(event, TR_DETAILED, "event start time: %d %d",
		e->startTime.sec, e->startTime.usec);
	xAddTime(&e->startTime, e->startTime, offset);
    }
#endif

    delt = (time+500*event_granularity)/(event_granularity*1000);
    /* time in us, delt in ticks, event_granularity in ms/tick */
    if (delt == 0 && time != 0) {
	delt = 1;
    }
    e->deltat = delt/BIG_N; /* If BIG_N is a power of 2, this could be a shift*/
    xAssert( ! e->flags.rescheduled && ! e->flags.cancelled );
    if (delt == 0) {
	e->state = E_SCHEDULED;
	/*    cthread_detach(cthread_fork(stub, e)); */
	/* Disallowed, use sledgehammer instead. */
	xTrace0(event,TR_EVENTS,"evSchedule starting event");
	CreateProcess((gen_func) xk_event_stub, e, 1, e);
    } else {
	/* e_insque will take care of locking */
	e->state = E_PENDING;
	e_insque((tickmod+delt)%BIG_N,e); /* Mod could be AND if BIG_N power of 2*/
    }
    xTrace0(event,TR_EVENTS,"evSchedule exit");
}



/*
 * evDetach -- free the storage used by event, or mark it
 *             as detachable after the associate thread finishes
 *
 */

void evDetach(
	Event e)
{
    xTrace0(event, TR_FULL_TRACE, "evDetach");
    if ( ! e ) {
	xTrace0(event, TR_SOFT_ERRORS, "evDetach passed null event");
	return;
    }
    EVENT_LOCK();
    if ( e->state == E_FINISHED || e->state == E_INIT ) {
	xTrace1(event, 5, "evDetach frees event %x", e);
	freeEvent(e);
    } else {
	xTrace1(event, 5, "evDetach marks event %x", e);
	e->flags.detached = 1;
    }
    EVENT_UNLOCK();
    xTrace0(event, TR_EVENTS, "evDetach exits");
}


/* 
 * See description in event.h
 */
EvCancelReturn evCancel(
	Event	e)
{
    int		ans;

    xTrace1(event, TR_EVENTS, "evCancel: event = %x", e);
    xAssert(e);
    EVENT_LOCK();
#if defined(XK_DEBUG) && defined(XK_THREAD_TRACE)
    {
	xkern_return_t	xkr;

	xkr = mapResolve(localEventMap, &e, 0);
	if ( xkr != XK_SUCCESS ) {
	    Kabort("Attempt to cancel nonexistent event");
	}
    }
#endif /* XK_DEBUG && XK_THREAD_TRACE */
    
    switch ( e->state ) {

    case E_SCHEDULED:
	/* 
	 * The stub routine will catch this event before
	 * it gets a chance to run
	 */
	xTrace2(event, TR_MORE_EVENTS,
		"cancelled event %x in SCHEDULED state, flags %x",
		e, e->flags);
	e->flags.cancelled = 1;
	e->flags.rescheduled = 0;
	ans = EVENT_CANCELLED;
	break;
	
    case E_BLOCKED:
    case E_RUNNING:
	e->flags.cancelled = 1;
	e->flags.rescheduled = 0;
	ans = EVENT_RUNNING;
	break;
	
    case E_FINISHED:
	ans = EVENT_FINISHED;
	break;
	
    case E_PENDING:
	e_remque(e);
	e->state = E_INIT;
	if ( e->flags.detached ) {
	    freeEvent(e);
	}
	/* fall-through */
	
    case E_INIT:
	ans = EVENT_CANCELLED;
	break;
	
    default:
	ans = EVENT_CANCELLED;  /* -wall */
	xError("broken event state in evCancel");
    }
    EVENT_UNLOCK();
    return ans;
}


Event
evSchedule( Event e, EvFunc f, void *a, unsigned time )
{
    xTrace1(event, TR_EVENTS, "evReschedule: event = %x", e);
    if ( ! e ) return 0;
    EVENT_LOCK();
#if defined(XK_DEBUG) && defined(XK_THREAD_TRACE)
    {
	xkern_return_t	xkr;

	xkr = mapResolve(localEventMap, &e, 0);
	if ( xkr != XK_SUCCESS ) {
	    xTrace0(event, TR_ERRORS,
		    "Attempt to reschedule nonexistent event");
	    EVENT_UNLOCK();
	    return 0;
	}
    }
#endif /* XK_DEBUG && XK_THREAD_TRACE */
    e->func = f;
    e->arg = a;
    switch ( e->state ) {
	
      case E_RUNNING:
      case E_SCHEDULED:
      case E_BLOCKED:
	/* 
	 * The stub routine will reschedule these events
	 */
	e->flags.rescheduled = 1;
	e->flags.cancelled = 0;
        e->deltat = time;	/* We'll borrow this field */
	EVENT_UNLOCK();
	return e;
	
      case E_PENDING:
	e_remque(e);
	/* fall-through */

      case E_FINISHED:
      case E_INIT:
	EVENT_UNLOCK();
	schedule_i(e, time);
	return e;

      default:
	xTrace0(event, TR_ERRORS, "broken event state in evSchedule");
	EVENT_UNLOCK();
	return 0;
    }
}


int
evIsCancelled(
	Event	e)
{
    int	res;

    xAssert(e);
    EVENT_LOCK();
    res = e->flags.cancelled;
    EVENT_UNLOCK();
    return res;
}

#ifdef XK_THREAD_TRACE
/* 
 * These are appropriate for in-kernel threads for the Decstations.
 * ... what's an appropriate limit for cthread stacks?  For other
 * platforms? 
 */
int	eventStackWarnLimit = 2000;
int	eventStackErrorLimit = 13750;

void
evCheckStack(name)
    char	*name;
{
    Event	ev;
    int		diff;

    ev = evSelf();
    if ( ev ) {
	diff = (char *)&ev - (char *)ev->earlyStack;
	if ( diff < 0 ) diff = -diff;
	if ( diff > eventStackWarnLimit ) {
	    if ( traceevent >= TR_SOFT_ERRORS || diff > eventStackErrorLimit ) {
		xError("x-kernel stack warning");
		xTrace2(event, TR_ALWAYS,
			"x-kernel stack warning [%s], event %x", name, ev);
		xTrace3(event, TR_ALWAYS,
			"cur = %x, beg = %x, size = %d",
			(int)&ev, (int)ev->earlyStack, diff);
	    }
	}
    }
}
#endif

#ifndef XKMACHKERNEL

/* 
 * This is just a temporary kludge for an allocation-free delay
 * function.  
 */
void
evDelay( int time )
{
    xkSemaphore s;
    struct event ev;
    
    semInit(&s, 0);
    ev.extra = &s;
    ev.deltat = (time + (event_granularity >> 1))/event_granularity;
    e_insque(DELAY_BUCKET, &ev);
    semWait(&s);
}

#endif /* ! XKMACHKERNEL */

