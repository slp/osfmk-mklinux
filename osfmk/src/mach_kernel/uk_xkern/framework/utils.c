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
 * utils.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 */

#include <string.h>
#include <xkern/include/xkernel.h>
#include <xkern/include/domain.h>
#include <xkern/include/assert.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/prot/ip.h>
#include <xkern/include/prot/eth.h>
#include <kern/host.h>
#include <mach/time_value.h>

char *
strchr(
        const char *str,
        int chr)
{
        char *p = (char *)str;
        char tgt = chr;

        while (1) {
                if (*p == tgt)
                        return (p);
                if (*p == '\0')
                        return (0);
                ++p;
        }
}

void
putchar(
	char c)
{
	cnputc(c);
}

char *
mach_error_string(
	kern_return_t	kr)
{
    static char		buf[80];

    sprintf(buf, "%d", kr);
    return buf;
}

