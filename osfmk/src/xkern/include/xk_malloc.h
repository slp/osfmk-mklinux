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
 * xk_malloc.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:27:40 $
 */


/* the memory throttle only works with the in-Mach-kernel version */

#ifdef XK_MEMORY_THROTTLE

#define MEG 1024*1024
#define XK_MEMORY_LIMIT 10*MEG

#ifndef IN_MALLOC_FILE
extern int memory_unused;
#endif /* IN_MALLOC_FILE */

#define XK_MEMORY_REPORT_INTERVAL 128
static int report_count = 0;

#define XK_INCOMING_MEMORY_MARK ((XK_MEMORY_LIMIT) / 8)

#endif /* XK_MEMORY_THROTTLE */

