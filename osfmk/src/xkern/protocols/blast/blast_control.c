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
 * blast_control.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.18
 * Date: 1993/07/20 19:33:05
 */

#include <xkern/include/xkernel.h>
#include <xkern/protocols/blast/blast_internal.h>

int
blastControlProtl(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
    PState	*pstate;
    int 	new_size;
    int		diff;
    
    xTrace1(blastp, TR_EVENTS, "blast_controlprotl, opcode: %d", opcode);
    pstate = (PState *)self->state;
    
    switch (opcode) {
      /* free storage associated with ticket returned by push */
      case FREERESOURCES:
	checkLen(len, sizeof(int));
	xTrace1(blastp, TR_MORE_EVENTS, "blast killticket called with id %d",
		*(int *)buf);
        return blast_freeSendSeq(pstate, *(int *)buf);
      case CHAN_RETRANSMIT:
	checkLen(len, sizeof(int));
	xTrace1(blastp, TR_MORE_EVENTS, "blast killticket called with id %d",
		*(int *)buf);
        return blast_Retransmit(pstate, *(int *)buf);
      case BLAST_SETOUTSTANDINGMSGS:
	/* Set the number of outstanding messages to be the integer passed
         * in buf.
         */
	checkLen(len, sizeof(int));
	new_size = *(int *)buf;
	xTrace1(blastp, TR_MORE_EVENTS,
		"set outstanding messages to %d", new_size);
	if (new_size <= 0)
	    return -1;
	diff = new_size - pstate->max_outstanding_messages;
	for (; diff < 0; diff++) 
	    semWait(&pstate->outstanding_messages);
	for (; diff > 0; diff--) 
	    semSignal(&pstate->outstanding_messages);
	pstate->max_outstanding_messages = new_size;
	return 0;
	
      case BLAST_GETOUTSTANDINGMSGS:
	/* Return the current number of allowed outstanding messages */
	checkLen(len, sizeof(int));
	*(int *)buf = pstate->max_outstanding_messages;
	return 0;
	
      default:
        return xControl(xGetDown(self, 0), opcode, buf, len);
    } 
}
  

int
blastControlSessn(s, opcode, buf, len)
    XObj s;
    int opcode;
    char *buf;
    int len;
{
    SState	*state;
    PState	*pstate;
    
    xTrace1(blastp, TR_EVENTS, "in blast_control with session=%x", s); 
    state = (SState *) s->state;
    pstate = (PState *) s->myprotl->state;
    switch (opcode) {
	
	/* free storage associated with ticket returned by push */
      case FREERESOURCES:
	checkLen(len, sizeof(int));
	xTrace1(blastp, TR_MORE_EVENTS, "blast killticket called with id %d",
		*(int *)buf);
        return blast_freeSendSeq(pstate, *(int *)buf);

      case CHAN_RETRANSMIT:
	xTrace1(blastp, TR_MORE_EVENTS, "blast killticket called with id %d",
		*(int *)buf);
        return blast_Retransmit(pstate, *(int *)buf);
	
      case GETMYPROTO:
      case GETPEERPROTO:
	checkLen(len, sizeof(long));
	*(long *)buf = state->prot_id;
	return sizeof(long);
	
      case GETOPTPACKET:
	checkLen(len, sizeof(int));
	*(int *)buf = state->fragmentSize * BLAST_MAX_FRAGS;
	return sizeof(int);

      case GETMAXPACKET:
	checkLen(len, sizeof(int));
	*(int *)buf = state->fragmentSize * BLAST_MAX_FRAGS;
	return sizeof(int);

      default:
        return xControl(xGetDown(s, 0), opcode, buf, len);
    }
}


