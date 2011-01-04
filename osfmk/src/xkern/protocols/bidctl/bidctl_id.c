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
 * bidctl_id.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:29:06 $
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/bidctl/bidctl_i.h>

/* 
 * Determining local boot id's and query tags
 */


BootId
bidctlNewId()
{
    XTime	t;

    xGetTime(&t);
    return t.sec;
}


/* 
 * bidctlReqTag -- this should be unique for every request and should
 * overflow slowly enough to be sure it won't run into duplicates.  We
 * munge the time value in such away that the id changes every 256
 * usec and has an overflow time > 18 hours.
 */
BootId
bidctlReqTag()
{
    XTime	t;

    xGetTime(&t);
    return t.sec << 16 | ( (t.usec & 0x00ffff00) >> 8);
}
