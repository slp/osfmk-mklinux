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

#ifndef sequencer_h
#define sequencer_h

typedef u_bit32_t  seqno;

typedef enum {
    SEQ_DATA,
    SEQ_NOTIFICATION
} seqtype;

typedef struct {
    seqtype        type;           /* what? */
    xkern_return_t xkr;            /* error status */
    seqno          seq;            /* the message has been installed with this sequence number */
} sequencerXchangeHdr;

#define INVALID_SEQUENCE_NUMBER  ((seqno) -1)

#define SEQUENCER_STACK_SIZE     100

/*
 * Control Operations
 */

#define SEQUENCER_SETPOPULATION      ( SEQUENCER_CTL * MAXOPS + 2 )
#define SEQUENCER_SETGROUPMAP        ( SEQUENCER_CTL * MAXOPS + 3 )
#define SEQUENCER_SETNOTIFICATION    ( SEQUENCER_CTL * MAXOPS + 4 )

#endif /* sequencer_h */




