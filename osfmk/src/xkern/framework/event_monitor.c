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
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * event_monitor.c,v
 * Revision 1.11.1.2.1.2  1994/09/07  04:18:18  menze
 * OSF modifications
 *
 * Revision 1.11.1.2  1994/09/01  04:32:32  menze
 * Meta-data allocations now use Allocators and Paths
 * Event flag format changed
 *
 * Revision 1.11  1994/04/15  21:18:42  davidm
 * (sortFunc): Don't define/declare it if XKMACHKERNEL is defined.
 *
 * Revision 1.10  1994/02/05  00:08:56  menze
 *   [ 1994/01/31          menze ]
 *   assert.h renamed xk_assert.h
 *
 * Revision 1.9  1994/01/25  05:58:15  davidm
 * evDump: if the localEventMap was empty, a core dump was produced to to
 * attempting to free (sort) a null pointer.
 *
 * dispEvent: pointers are now printed as longs.
 *
 * Revision 1.8  1993/12/13  19:07:32  menze
 * Fixed #endifs
 * Modified prototypes
 *
 */

/*
 * Code supporting ps-like tracing for x-kernel events.  Some of this
 * support is in the platform-specific event code.
 */

#include <xkern/include/domain.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/event.h>
#include <xkern/include/event_i.h>
#include <xkern/include/assert.h>
#include <xkern/include/xk_alloc.h>


#ifdef XK_THREAD_TRACE

static char *	evStateStr( EvState );
static int	xTime2sec( XTime );
static void	dispEvent( Event, XTime );
static int	findEvents( void *, void *, void *);
static int	sortFunc( void *, void * );



static char *
evStateStr( s )
    EvState	s;
{
    switch ( s ) {
      case E_PENDING: 	return "PENDING";
      case E_SCHEDULED:	return "SCHED";
      case E_RUNNING: 	return "RUNNING";
      case E_FINISHED: 	return "FINISH";
      case E_BLOCKED: 	return "BLOCKED";
      case E_INIT: 	return "INIT";
      default:		return "UNKNOWN";
    }
}



static int
xTime2sec( t )
    XTime	t;
{
    t.usec += 500 * 1000;
    t.sec += t.usec / (1000 * 1000);
    return t.sec;
}


static void
dispEvent( ev, now )
    Event	ev;
    XTime	now;
{
    XTime	diff;

    xAssert(ev);
    printf("%8lx  ", (u_long)ev->func);
    printf("%8lx\t", (u_long)ev->arg);
    printf("%s\t", evStateStr(ev->state));
    switch ( ev->state ) {
      case E_PENDING:
	xSubTime(&diff, ev->startTime, now);
	printf("%8d", xTime2sec(diff));
	break;
	
      case E_SCHEDULED:
      case E_RUNNING:
	xSubTime(&diff, now, ev->startTime);
	printf("%8d", xTime2sec(diff));
	break;

      case E_FINISHED:
      case E_BLOCKED:
	xSubTime(&diff, now, ev->stopTime);
	printf("%8d", xTime2sec(diff));
	break;
	
      case E_INIT:
	printf("%8d", 0);
	break;
    }
    printf("\t");
    if ( ev->flags.detached ) {
	printf("%c", 'D');
    }
    if ( ev->flags.cancelled ) {
	printf("%c", 'C');
    }
    if ( ev->flags.rescheduled ) {
	printf("%c", 'R');
    }
    printf("\n");
}


#define MAX_EVENTS	128	/* just a suggestion */


typedef struct {
    Event	*arr;
    int		i;
    int		max;
} EvArray;


static int
findEvents( key, val, arg )
    void	*key, *val, *arg;
{
    EvArray	*eva = (EvArray *)arg;
    Event	*newArray;
    int		newSize;

    if ( eva->i >= eva->max ) {
	newSize = eva->max ? eva->max * 2 : MAX_EVENTS;
	newArray = (Event *)xSysAlloc(sizeof(Event) * newSize);
	if ( newArray == 0 ) {
	    xTrace0(event, TR_SOFT_ERRORS,
		    "Couldn't allocate event-tracing array");
	    return 0;
	}
	bzero((char *)newArray, sizeof(Event) * newSize);
	bcopy((char *)eva->arr, (char *)newArray, sizeof(Event) * eva->max);
	if ( eva->arr ) {
	    allocFree((char *)eva->arr);
	}
	eva->arr = newArray;
	eva->max = newSize;
    }
    eva->arr[eva->i++] = (Event)val;
    return MFE_CONTINUE;
}


#ifndef XKMACHKERNEL
static int
sortFunc( void *p1, void *p2 )
{
    Event	*e1 = p1, *e2 = p2;

    if ( (*e1)->state != (*e2)->state ) {
	return (u_int)(*e1)->state - (u_int)((*e2)->state);
    }
    if ( (*e1)->func != (*e2)->func ) {
	return (u_int)(*e1)->func - (u_int)(*e2)->func;
    }
    if ( (*e1)->arg != (*e2)->arg ) {
	return (u_int)(*e1)->arg - (u_int)(*e2)->arg;
    }
    return 0;
}
#endif /* XKMACHKERNEL */


void
evDump()
{
    XTime	now;
    EvArray	evArray;
    int		i;

    xGetTime(&now);
    evArray.arr = 0;
    evArray.max = 0;
    evArray.i = 0;
    mapForEach(localEventMap, findEvents, &evArray);
#ifndef	MACH_KERNEL
    if (evArray.arr) {
	qsort((char *)evArray.arr, evArray.i, sizeof(Event),
	      (int (*)(const void *, const void *))sortFunc);
    }
#endif  /* MACH_KERNEL */
    printf("   Func      arg\tstate\t  seconds\tflags\n\n");
    for ( i=0; i < evArray.i; i++ ) {
	dispEvent(evArray.arr[i], now);
    }
    if (evArray.arr) {
	allocFree(evArray.arr);
    } /* if */
}

#else	/* ! XK_THREAD_TRACE */

void
evDump()
{
}

#endif /* XK_THREAD_TRACE */
