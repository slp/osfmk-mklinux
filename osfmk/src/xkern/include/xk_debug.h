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
 * xk_debug.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * xk_debug.h,v
 * Revision 1.31.1.4.1.2  1994/09/07  04:18:03  menze
 * OSF modifications
 *
 * Revision 1.31.1.4  1994/09/01  04:12:48  menze
 * Subsystem initialization functions can fail
 *
 * Revision 1.31.1.3  1994/07/21  23:26:23  menze
 * Added tracepath
 *
 * Revision 1.31.1.2  1994/04/01  16:44:24  menze
 * Added tracealloc
 *
 * Revision 1.31.1.1  1994/03/21  16:44:28  menze
 * Size of errBuf is now a manifest constant
 *
 * Revision 1.31  1993/12/11  00:23:28  menze
 * fixed #endif comments
 *
 * Revision 1.30  1993/12/11  00:07:02  menze
 * Added traceendpoint and tracemaccommon for Irix
 *
 */

#ifndef xk_debug_h
#define xk_debug_h

#include <xkern/include/domain.h>
#include <xkern/include/platform.h>
#include <xkern/include/trace.h>
#include <xkern/include/assert.h>

extern int
	tracealloc,
	tracebuserror,
	tracecustom,
        traceendpoint,
  	traceethdrv,
 	traceether,
	traceevent,
	tracefixme,
	traceidle,
  	traceidmap,
	traceie,
	traceinit,  
        tracemaccommon,
	tracememoryinit,
	tracemsg,  
	tracenetmask,
	tracepath,
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

#define GLOBAL_ERRBUF_SIZE      250
extern	char errBuf[GLOBAL_ERRBUF_SIZE];


#if !XK_DEBUG
#define NDEBUG
#endif /* XK_DEBUG */   


void	xTraceLock( void );
void	xTraceUnlock( void );
xkern_return_t	xTraceInit( void );

#if XK_DEBUG || defined(lint)

#define PRETRACE(l) { int __i=l; xTraceLock();  while(__i--) putchar(' '); }
#define POSTTRACE { putchar('\n'); xTraceUnlock(); }

#define XPASTE(X,Y) X##Y
#define PASTE(X,Y) XPASTE(X,Y)


#define xIfTrace(t, l) \
	if (PASTE(trace,t) >= l)
#define xTrace0(t, l, f) 		\
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f); 		\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#define xTrace1(t, l, f, arg1) 	\
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1);	\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#define xTrace2(t, l, f, arg1, arg2) \
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2);	\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#define xTrace3(t, l, f, arg1, arg2, arg3) \
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2, arg3);	\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#define xTrace4(t, l, f, arg1, arg2, arg3, arg4) \
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2, arg3, arg4);	\
	    	POSTTRACE;		\
	    }				\
	} while (0)
#define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5) \
	do {				\
	    if (PASTE(trace,t) >= l) {	\
		PRETRACE(l);		\
	    	printf(f, arg1, arg2, arg3, arg4, arg5); \
	    	POSTTRACE;		\
	    }				\
	} while (0)

#define xIfTraceP( _obj, _lev ) if ( *(_obj)->traceVar >= (_lev) )

#define xTraceP0( _obj, _lev, _fmt )					\
	do {								\
	    xAssert((_obj)->traceVar);					\
	    if ( *(_obj)->traceVar >= (_lev) ) {			\
		sprintf(errBuf, "[%s]: %s", (_obj)->fullName, (_fmt));	\
		PRETRACE(_lev);						\
		printf(errBuf);						\
		POSTTRACE;						\
	    }								\
	} while (0)

#define xTraceP1( _obj, _lev, _fmt, _arg1 )				\
	do {								\
	    xAssert((_obj)->traceVar);					\
	    if ( *(_obj)->traceVar >= (_lev) ) {			\
		sprintf(errBuf, "[%s]: %s", (_obj)->fullName, (_fmt));	\
		PRETRACE(_lev);						\
		printf(errBuf, _arg1);					\
		POSTTRACE;						\
	    }								\
	} while (0)

#define xTraceP2( _obj, _lev, _fmt, _arg1, _arg2 )			\
	do {								\
	    xAssert((_obj)->traceVar);					\
	    if ( *(_obj)->traceVar >= (_lev) ) {			\
		sprintf(errBuf, "[%s]: %s", (_obj)->fullName, (_fmt));	\
		PRETRACE(_lev);						\
		printf(errBuf, _arg1, _arg2);				\
		POSTTRACE;						\
	    }								\
	} while (0)

#define xTraceP3( _obj, _lev, _fmt, _arg1, _arg2, _arg3 )		\
	do {								\
	    xAssert((_obj)->traceVar);					\
	    if ( *(_obj)->traceVar >= (_lev) ) {			\
		sprintf(errBuf, "[%s]: %s", (_obj)->fullName, (_fmt));	\
		PRETRACE(_lev);						\
		printf(errBuf, _arg1, _arg2, _arg3);			\
		POSTTRACE;						\
	    }								\
	} while (0)

#define xTraceP4( _obj, _lev, _fmt, _arg1, _arg2, _arg3, _arg4 )	\
	do {								\
	    xAssert((_obj)->traceVar);					\
	    if ( *(_obj)->traceVar >= (_lev) ) {			\
		sprintf(errBuf, "[%s]: %s", (_obj)->fullName, (_fmt));	\
		PRETRACE(_lev);						\
		printf(errBuf, _arg1, _arg2, _arg3, _arg4);		\
		POSTTRACE;						\
	    }								\
	} while (0)

#define xTraceP5( _obj, _lev, _fmt, _arg1, _arg2, _arg3, _arg4, _arg5 )	\
	do {								\
	    xAssert((_obj)->traceVar);					\
	    if ( *(_obj)->traceVar >= (_lev) ) {			\
		sprintf(errBuf, "[%s]: %s", (_obj)->fullName, (_fmt));	\
		PRETRACE(_lev);						\
		printf(errBuf, _arg1, _arg2, _arg3, _arg4, _arg5);	\
		POSTTRACE;						\
	    }								\
	} while (0)

#else

#define xIfTrace(t, l) if (0)
#define xTrace0(t, l, f)
#define xTrace1(t, l, f, arg1)
#define xTrace2(t, l, f, arg1, arg2)
#define xTrace3(t, l, f, arg1, arg2, arg3)
#define xTrace4(t, l, f, arg1, arg2, arg3, arg4)
#define xTrace5(t, l, f, arg1, arg2, arg3, arg4, arg5)

#define xIfTraceP(t, l) if (0)
#define xTraceP0(t, l, f)
#define xTraceP1(t, l, f, arg1)
#define xTraceP2(t, l, f, arg1, arg2)
#define xTraceP3(t, l, f, arg1, arg2, arg3)
#define xTraceP4(t, l, f, arg1, arg2, arg3, arg4)
#define xTraceP5(t, l, f, arg1, arg2, arg3, arg4, arg5)

#endif	/* XK_DEBUG */

extern void	xError(char *);

extern xkern_return_t	xk_clock_init( void );
#ifdef	XK_PROFILE
extern void	(*xk_clock_trace)(char * caller_id);
#define CLOCK_TRACE(s)	xk_clock_trace(s)
#else	/* XK_PROFILE */
#define CLOCK_TRACE(s)
#endif	/* XK_PROFILE */

#endif	/* xk_debug_h */
