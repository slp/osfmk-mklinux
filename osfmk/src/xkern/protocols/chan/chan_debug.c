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
 * chan_debug.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.9.1 $
 * $Date: 1995/02/23 15:31:06 $
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/chan/chan_internal.h>
#ifdef BOOTID
#include <xkern/include/prot/bidctl.h>
#endif /* BOOTID */

static char *	flagStr( u_int );


/*
 * chanStatusStr - Print sequence status
 */
char *
chanStatusStr(stat)
    SEQ_STAT stat;
{
    switch (stat) {
      case old:		return "old";
      case current:	return "current";
      case new: 	return "new";
      default: 		return "UNKNOWN!!";
    }
}


/*
 * chanStateStr - Print state of server/client
 */
char *
chanStateStr(state)
    int state;
{
    switch (state) {
      case SVC_EXECUTE:	return "SVC_EXECUTE";
      case SVC_WAIT: 	return "SVC_WAIT";
      case SVC_IDLE: 	return "SVC_IDLE";
      case CLNT_FREE: 	return "CLNT_FREE";
      case CLNT_WAIT: 	return "CLNT_WAIT";
      case DISABLED: 	return "DISABLED";
      default:		return "UNKNOWN!!";
    }
}


#if XK_DEBUG

static char *
flagStr( f )
    u_int	f;
{
    static char	s[80];

    s[0] = 0;
    if ( f & FROM_CLIENT ) {
	(void)strcat(s, "FROM_CLIENT ");
    } else {
	(void)strcat(s, "FROM_SERVER ");
    }
    if ( f & USER_MSG ) {
	(void)strcat(s, "USER_MSG ");
    } 
    if ( f & ACK_REQUESTED ) {
	(void)strcat(s, "ACK_REQUESTED ");
    }
    if ( f & NEGATIVE_ACK ) {
	(void)strcat(s, "NAK ");
    }
    return s;
}

#endif /* XK_DEBUG */   


/*
 * pChanHdr
 */
void
pChanHdr(hdr)
    CHAN_HDR *hdr;
{
    xTrace1(chanp, TR_ALWAYS, "\t| CHAN header for channel %d:", hdr->chan);
    xTrace1(chanp, TR_ALWAYS, "\t|      flags:    %s", flagStr(hdr->flags));
    xTrace1(chanp, TR_ALWAYS, "\t|      seq: %d", hdr->seq);
    xTrace1(chanp, TR_ALWAYS, "\t|      prot_id: %d", hdr->prot_id);
    xTrace1(chanp, TR_ALWAYS, "\t|      len: %d", hdr->len);
}


void
chanDispKey( key )
    ActiveID	*key;
{
    xTrace3(chanp, TR_ALWAYS, "chan == %d, lls = %x, prot = %d",
	    key->chan, key->lls, key->prot_id);
}



