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
 * OLD HISTORY
 * Revision 1.1.7.3  1995/01/10  08:08:06  devrcs
 * 	mk6 CR801 - copyright marker not _FREE
 * 	[1994/12/01  21:35:43  dwm]
 * 
 * Revision 1.1.7.1  1994/06/18  18:44:57  bolinger
 * 	Import NMK17.2 sources into MK6.
 * 	[1994/06/18  18:35:43  bolinger]
 * 
 * Revision 1.1.2.1  1994/02/22  12:41:49  bernadat
 * 	Initial Revision
 * 	[94/02/21            bernadat]
 * 
 */

#include <i386/asm.h>

/*
 * Just use mach_setjmp & mach_longjmp entries.
 * Cant use jsr, just branch.
 */

ENTRY(setjmp)
        jmp     EXT(mach_setjmp)

ENTRY(longjmp)
        jmp     EXT(mach_longjmp)
