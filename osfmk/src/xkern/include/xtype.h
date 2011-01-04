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
 * xtype.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * xtype.h,v
 * Revision 1.14.1.2.1.2  1994/09/07  04:18:02  menze
 * OSF modifications
 *
 * Revision 1.14.1.2  1994/09/01  04:07:13  menze
 * Added XK_NO_MEMORY and XK_ALREADY_BOUND to xkern_return_t
 *
 * Revision 1.14.1.1  1994/07/21  23:28:19  menze
 * Added forward declarations for XObj, Allocator and Path
 *
 */

#ifndef xtype_h
#define xtype_h

#include <types.h>
#include <mach/boolean.h>

/*
 * upi internal error codes
 */
typedef enum xkret {
    XK_FAILURE = -1,
    XK_SUCCESS = 0,
    XK_NO_MEMORY,
    XK_ALREADY_BOUND
} xkern_return_t;

typedef int	xmsg_handle_t;
#define XMSG_NULL_HANDLE	0
#define XMSG_ERR_HANDLE		-1
#define XMSG_ERR_WOULDBLOCK	-2

typedef struct allocator *Allocator;
typedef struct event 	 *Event;
typedef struct map	 *Map;
typedef struct msg	 *Msg;
typedef struct part	 *Part;
typedef struct path 	 *Path;
typedef struct xobj	 *XObj;

#define XK_MAX_HOST_LEN		6

/*
 * non-ANSI compilers gripe about the _n##U usage while GCC gives
 * warning messages about the u_long cast.
 */
#define UNSIGNED(_n)    _n##U

#endif /* ! xtype_h */
