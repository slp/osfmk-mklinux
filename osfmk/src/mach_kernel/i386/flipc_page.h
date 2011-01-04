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

#include <mach/machine/vm_param.h>

/*
 * Log from original position (flipc/i386/flipc_page.h):
 *
 * Revision 1.1.2.3  1995/02/21  17:22:54  randys
 * 	Re-indented code to four space indentation
 * 	[1995/02/21  16:27:56  randys]
 *
 * Revision 1.1.2.2  1995/01/12  21:18:58  randys
 * 	Include files defining i386 page macros
 * 	[1995/01/12  20:50:19  randys]
 * 
 * Revision 1.1.2.1  1995/01/12  03:43:51  randys
 * 	Modify flipc_usermsg.c to be device indpedent.  The device
 * 	dependent material (page sizes and rounding) is included from
 * 	flipc_page.h
 * 	[1995/01/06  20:19:31  randys]
 */

/* 
 * Machine specific defines to allow flipc to work with pages.
 * Included from flipc_usermsg.c only.
 */
#define FLIPC_PAGESIZE I386_PGBYTES
#define FLIPC_PAGERND_FN i386_round_page
#define FLIPC_BTOP i386_btop
