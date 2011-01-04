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
 * xk_flags.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.1
 * Date: 1993/05/12 23:38:02
 */

#include <xkern/include/romopt.h>
#include <xkern/include/platform.h>
#include <xkern/include/xk_debug.h>

static int	count;
static char	xkRomFlags[10][80];


/* 
 * Called for every boot parameter which assigns an x-kernel
 * romfile entry.
 * 
 * This routine is called very early.  Don't rely on any x-kernel
 * stuff (including trace routines) being properly initialized. 
 */
unsigned
xk_boot_flag_rom(
    unsigned char	*addr,
    char		*flag)
{
    int	n;

    if ( count + 1 < sizeof(xkRomFlags) ) {
	n = strlen(flag);
	if ( n < sizeof(xkRomFlags[0]) ) {
	    strcpy(xkRomFlags[count], flag);
	    count++;
	}
    }
    return 0;
}


/* 
 * Add rom entries from the boot parameters to the actual rom array. 
 */
xkern_return_t
addRomFlags()
{
    int	j;

    for ( j=0; j < count; j++ ) {
	if ( addRomOpt(&xkRomFlags[j][0]) != XK_SUCCESS ) {
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}
