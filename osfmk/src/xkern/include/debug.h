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
 * debug.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:23:49 $
 */

#include "trace.h"
#ifndef debug_h
extern int
	tracebuserror,
	tracecustom,
  	traceethdrv,
 	traceether,
	traceevent,
	tracefixme,
	traceidle,
	traceie,
	traceinit,  
	tracememoryinit,
	tracemsg,  
	traceprocesscreation,
	traceprocessswitch,
	traceprotocol,
	traceptbl,
  	traceprottest,
	tracetick,
	tracetrap,
  	tracelock,
  	tracespl,
	tracesessngc,
	traceuser;

extern void trace();


#if !defined(NDEBUG) || defined(lint)

#define XPASTE(X,Y) X##Y
#define PASTE(X,Y) XPASTE(X,Y)

#   define xIfTrace(t, l) \
	if (PASTE(trace,t) >= l)
#   define xTrace0(t, l, f) \
	if (PASTE(trace,t) >= l) trace(l, f, 0)
#   define xTrace1(t, l, f, arg1) \
	if (PASTE(trace,t) >= l) trace(l, f, 1, arg1)
#   define xTrace2(t, l, f, arg1, arg2) \
	if (PASTE(trace,t) >= l) trace(l, f, 2, arg1, arg2)
#   define xTrace3(t, l, f, arg1, arg2, arg3) \
	if (PASTE(trace,t) >= l) trace(l, f, 3, arg1, arg2, arg3)
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4) \
	if (PASTE(trace,t) >= l) trace(l, f, 4, arg1, arg2, arg3, arg4)
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5) \
	if (PASTE(trace,t) >= l) trace(l, f, 5, arg1, arg2, arg3, arg4, arg5)

#else

#   define xIfTrace(t, l) if (0)
#   define xTrace0(t, l, f)
#   define xTrace1(t, l, f, arg1)
#   define xTrace2(t, l, f, arg1, arg2)
#   define xTrace3(t, l, f, arg1, arg2, arg3)
#   define xTrace4(t, l, f, arg1, arg2, arg3, arg4)
#   define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5)

#endif	/* ! NDEBUG */

extern void	xError(char *);

#endif	/* debug_h */
