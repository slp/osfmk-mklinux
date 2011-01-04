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
 * allocmon.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * allocmon.c,v
 * Revision 1.1.1.3.1.2  1994/09/07  04:18:37  menze
 * OSF modifications
 *
 * Revision 1.1.1.3  1994/09/06  22:58:01  menze
 * 'interval' rom option
 *
 * Revision 1.1.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.1  1994/08/22  21:12:17  menze
 * Initial revision
 *
 */

/* 
 * Not really a protocol; just call allocDumpStats periodically. 
 */

#include <xkern/include/xkernel.h>

XObjInitFunc	allocmon_init;

int	traceallocmonp;


/* 
 * instName interval int
 */
static xkern_return_t
readInterval( XObj self, char **str, int nFields, int line, void *arg )
{
    *(int *)arg = atoi(str[2]);
    return XK_SUCCESS;
}

static XObjRomOpt	rOpts[] = {
    { "interval", 3, readInterval }
};



static void
foo( Event ev, void *arg )
{
    allocDumpStatsAll();
    evDetach(evSchedule(ev, foo, arg, (*(int *)arg) * 1000 * 1000));
}



xkern_return_t
allocmon_init( XObj self )
{
    static int	interval = 30;
    Event	ev;

    if ( findXObjRomOpts(self, rOpts, sizeof(rOpts)/sizeof(XObjRomOpt),
			 &interval)
		== XK_FAILURE ) {
	xError("allocmon: romfile config errors");
	return XK_FAILURE;
    }
    if ( ! (ev = evAlloc(self->path)) ) {
	xTraceP0(self, TR_ERRORS, "allocation failure");
	return XK_NO_MEMORY;
    }
    evDetach(evSchedule(ev, foo, &interval, 0));
    return XK_SUCCESS;
}
