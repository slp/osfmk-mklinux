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
 * blast_util.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * blast_util.c,v
 * Revision 1.9.1.2.1.2  1994/09/07  04:18:28  menze
 * OSF modifications
 *
 * Revision 1.9.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.9  1993/12/16  01:25:27  menze
 * Fixed mask references to compile with strict ANSI restrictions
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/blast/blast_internal.h>
 
/* 
 * Returns the index of the first set bit in the mask
 */
int
blast_mask_to_i(mask)
    BlastMask	mask;
{
    int i;
    xTrace1(blastp, TR_FUNCTIONAL_TRACE, "mask_to_i: called mask =  %s",
	    blastShowMask(mask));
    
    for (i=1; i <= BLAST_MAX_FRAGS; i++) {
	if ( BLAST_MASK_IS_BIT_SET(&mask, 1) ) {
	    xTrace1(blastp, TR_DETAILED, "mask_to_i: returns i =  %d", i);
	    return i;
	}
	BLAST_MASK_SHIFT_RIGHT(mask, 1);
    }
    xTrace0(blastp, TR_DETAILED, "mask_to_i: returns 0");
    return 0;
}
    

