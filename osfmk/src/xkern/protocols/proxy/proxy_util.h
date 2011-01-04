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
 * proxy_util.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.3
 * Date: 1993/07/16 00:47:18
 */

#ifndef proxy_util_h
#define proxy_util_h


typedef 	void	(* DeallocFunc)( void *, int);

typedef struct {
    boolean_t		valid;
    DeallocFunc		func;
    void		*arg;
    int			len;
} LingeringMsg;


void		lingeringMsgClear( void );
void		lingeringMsgSave( DeallocFunc, void *, int );
boolean_t	msgIsOkToMangle( Msg m, char **machMsg, int offset );
boolean_t	msgIsContiguous( Msg m );
xkern_return_t	msgToOol( Msg, char **, DeallocFunc *, void **arg );
xkern_return_t	oolToMsg( char *, Path, unsigned int, Msg );

#  ifdef MACH_KERNEL

#define	XK_PROXY_MSG_HACK

void	oolFreeIncoming( void *, int, void * );
void	oolFreeOutgoing( void *, int, void * );
void	xkDumpVmStats( void );

#  else

/* 
 * Deallocate the virtual memory region containing 'p'.  
 */
void	oolFree( void *, int, void * );

typedef struct {
    LingeringMsg	lm;
    Path		path;
    mach_port_t		port;
} ProxyThreadInfo;

#  endif  /* MACH_KERNEL */


#endif /* ! proxy_util_h */
