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
 * port_mgr.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * port_mgr.h,v
 * Revision 1.10.2.2.1.2  1994/09/07  04:18:40  menze
 * OSF modifications
 *
 * Revision 1.10.2.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 */

/* 
 * This file defines the prototypes for the port management functions.
 * A header file with these prototypes can be formed by defining the macro
 *	NAME -- token to prepend to routine names
 * and then including this file.
 */

#include <xkern/include/xtype.h>

#define XPASTE(X,Y) X##Y
#define PASTE(X,Y) XPASTE(X,Y)

/* 
 * Initializes the port map.  Mallocs port state structure.
 *  Must be called before any other routines. 
 */
xkern_return_t	PASTE(NAME, PortMapInit) ( void *, Path );

/* 
 * Closes the port map, frees the port state structure
 */
void		PASTE(NAME, PortMapClose) ( void * );

/* 
 * Attempts to get a free port >= FIRST_USER_PORT, placing the
 * new port in *port.  
 */
xkern_return_t	PASTE(NAME, GetFreePort) ( void *, long * );

/* 
 * Increases the reference count of the port.  The port does not have
 * to have been previously acquired.
 */
xkern_return_t	PASTE(NAME, DuplicatePort) ( void *, long );

/* 
 * Decreases the reference count on a port previously acquired through
 * DuplicatePort() or GetFreePort().
 */
void		PASTE(NAME, ReleasePort) ( void *, long );

