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
 * blast_hdr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:29:39 $
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/blast/blast_internal.h>
 

#define HDR	((BLAST_HDR *)hdr)
long
blastHdrLoad(hdr, src, len, arg)
    void *hdr;
    char *src;
    long int len;
    void *arg;
{
    xAssert( len == sizeof(BLAST_HDR) );
    bcopy( src, hdr, len );
    HDR->prot_id = ntohl(HDR->prot_id);
    HDR->seq = ntohl(HDR->seq);
    HDR->num_frag = ntohs(HDR->num_frag);
    BLAST_MASK_NTOH(HDR->mask, HDR->mask);
    HDR->len = ntohl(HDR->len);
    return len;
}


void
blastHdrStore(hdr, dst, len, arg)
    void *hdr;
    char *dst;
    long int len;
    void *arg;
{
    BLAST_HDR	h;

    xAssert( len == sizeof(BLAST_HDR) );
    h = *(BLAST_HDR *)hdr;
    h.prot_id = htonl(h.prot_id);
    h.seq = htonl(h.seq);
    h.num_frag = htons(h.num_frag);
    BLAST_MASK_HTON(h.mask, h.mask);
    h.len = htonl(h.len);
    bcopy( (char *)&h, dst, len );
}
