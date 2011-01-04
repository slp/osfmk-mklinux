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
 * alloc_bs.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * alloc_bs.c,v
 * Revision 1.1.1.3.1.2  1994/09/07  04:18:23  menze
 * OSF modifications
 *
 * Revision 1.1.1.3  1994/09/05  18:27:22  menze
 * No statistics reported for this allocator
 *
 * Revision 1.1.1.2  1994/09/01  04:28:01  menze
 * Uses the standard allocator header
 *
 * Revision 1.1  1994/07/21  23:44:18  menze
 * Initial revision
 *
 */

#include <xkern/include/xk_debug.h>
#include <xkern/include/xk_alloc.h>
#include <xkern/include/intern_alloc.h>


/* 
 * Boot-strap allocator, used for allocations before the allocator
 * module is fully initialized.  This may need to be tied more closely
 * to the host-system memory allocation.
 *
 * Note: this is called very early in initialization ... (e.g, before
 * trace statement initialization.)
 */

static AllocMethods methods;


static void *
get( Allocator self, u_int *length )
{
    AllocHdrStd	*hdr;

    hdr = (AllocHdrStd *)malloc(*length + ALLOC_HDR_SIZE_STD);
    hdr->alloc = self;
    return hdr->buf;
}


static void
put( Allocator self, void *b )
{
    free(b);
}


xkern_return_t
bootStrapAlloc_init( Allocator self )
{
    methods.u.e.get = get;
    methods.u.e.put = put;
    self->class = ALLOC_EXTERIOR;
    self->methods = &methods;
    return XK_SUCCESS;
}

