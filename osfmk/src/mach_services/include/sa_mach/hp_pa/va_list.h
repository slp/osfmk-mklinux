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
 * Four possible situations:
 * 	- We are being included by {var,std}args.h (or anyone) before stdio.h.
 * 	  define real type.
 *
 * 	- We are being included by stdio.h before {var,std}args.h.
 * 	  define hidden type for prototypes in stdio, don't pollute namespace.
 * 
 * 	- We are being included by {var,std}args.h after stdio.h.
 * 	  define real type to match hidden type.  no longer use hidden type.
 * 
 * 	- We are being included again after defining the real va_list.
 * 	  do nothing.
 * 
 */

#ifndef _MACHINE_VALIST_H
#define _MACHINE_VALIST_H

#if	!defined(_HIDDEN_VA_LIST) && !defined(_VA_LIST)

/* Define __gnuc_va_list. */

#ifndef __GNUC_VA_LIST
/*
 * If this is for internal libc use, don't define
 * anything but __gnuc_va_list.
 */
#define __GNUC_VA_LIST
typedef char *__gnuc_va_list;

#endif /* not __GNUC_VA_LIST */

#define _VA_LIST
typedef char *va_list;

#elif	defined(_HIDDEN_VA_LIST) && !defined(_VA_LIST)

#define _VA_LIST
typedef char *__va_list;

#elif	defined(_HIDDEN_VA_LIST) && defined(_VA_LIST)

#undef _HIDDEN_VA_LIST
typedef __va_list va_list;

#endif

#endif /* _MACHINE_VALIST_H */
