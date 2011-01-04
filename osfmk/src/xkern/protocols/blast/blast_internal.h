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
 * blast_internal.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * blast_internal.h,v
 * Revision 1.38.1.3  1994/07/22  20:08:06  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.38.1.2  1994/04/23  00:15:26  menze
 * Added allocator call-back handlers
 * changed field name spelling
 *
 * Revision 1.38.1.1  1994/04/14  21:35:11  menze
 * Uses allocator-based message interface
 *
 */

#ifndef blast_internal_h
#define blast_internal_h
 
#include <xkern/include/xkernel.h>
#include <xkern/include/prot/blast.h>
#include <xkern/include/prot/chan.h>
#include <xkern/protocols/blast/blast_mask64.h>
#include <xkern/protocols/blast/blast_stack.h>

#define BLASTHLEN  sizeof(BLAST_HDR) 

/*
 * Trace levels
 */
#define REXMIT_T TR_MAJOR_EVENTS

/* 
 * opcodes in the message header
 */
#define BLAST_SEND 1
#define BLAST_NACK 2
#define BLAST_RETRANSMIT 3

#define BLAST_OK 2
#define BLAST_FAIL 2
 
typedef unsigned int	BlastSeq;
#define BLAST_MAX_SEQ	0xffffffff

typedef struct  {
    u_int 	prot_id;
    BlastSeq 	seq;
    u_int 	len;
    u_short 	num_frag;
    u_char 	op;
    char	pad[BLAST_PADDING];
    BlastMask 	mask;
} BLAST_HDR;

typedef struct {
    BLAST_HDR 	hdr;
    Msg_s 	frags[BLAST_MAX_FRAGS+1];
    BlastMask 	mask;
    BlastMask 	old_mask;
    u_int 	wait;
    Bind 	binding;
    struct blast_state *state;
    Event 	event;
    int 	nacks_sent;
    int 	nacks_received;
    boolean_t	canChangeBackoff;
    int		rcnt;
} MSG_STATE;

typedef struct blast_state {
    u_int	ircnt;    
    long	prot_id;
    BLAST_HDR 	short_hdr;
    BLAST_HDR 	cur_hdr;
    XObj 	self;
    int 	fragmentSize;
    int		per_packet;
    Map 	rec_map;
} SState;

typedef struct {
    Map 	active_map;
    xkSemaphore	createSem;
    Map 	passive_map;
    BlastSeq	max_seq;
    xkSemaphore 	outstanding_messages;
    int 	max_outstanding_messages;
    Stack	mstateStack;
    Map 	send_map;
} PState;

typedef struct {
        long	prot;
	XObj	lls;
} ActiveID;

typedef int PassiveID;
	
/*
 * timeout calculations:
 */
#define PER_PACKET	10 * 1000
#define BACKOFF 	2
#define MAX_PER_PACKET	(INT_MAX/BACKOFF)
#define SEND_CONST 	1000 * 1000
#define MAX_WAIT	1000 * 1000
#define NACK_LIMIT	3


/* Default number of concurrent outstanding messages allowed
 * (per protocol instantiation)
 */
#define OUTSTANDING_MESSAGES 64

#define BLAST_ACTIVE_MAP_SZ	101
#define BLAST_SEND_MAP_SZ	101
#define BLAST_REC_MAP_SZ	101
#define BLAST_PASSIVE_MAP_SZ	13
#define BLAST_MSTATE_STACK_SZ	(2 * OUTSTANDING_MESSAGES)

#define BLAST_STACK_LEN	100

/* If BLAST_SIM_DROPS is defined, BLAST will occasionally drop a fragment
 * before it gets to the appropriate BLAST session.
 */
/* #define BLAST_SIM_DROPS */


extern BlastMask	blastFullMask[];
extern int 		traceblastp;

void		blastAllocCallBack( Allocator, void * );
xmsg_handle_t	blastPush( XObj, Msg );
int		blastControlSessn( XObj, int, char *, int );
int		blastControlProtl( XObj, int, char *, int );
void		blastDecIrc( XObj );
MSG_STATE *	blastNewMstate( XObj );
xkern_return_t	blastPop( XObj, XObj, Msg, void * );
xkern_return_t	blastDemux( XObj, XObj, Msg );
XObj		blastCreateSessn( XObj, XObj, XObj, ActiveID *, Path );
void		blast_mapFlush( Map );
int		blast_freeSendSeq( PState *, int );
int		blast_Retransmit( PState *, int );
long		blastHdrLoad( void *, char *, long, void * );
void		blastHdrStore( void *, char *, long, void * );
xkern_return_t	blastSenderPop( XObj, Msg, BLAST_HDR * );
int		blast_mask_to_i( BLAST_MASK_PROTOTYPE );
int		blastRecvCallBack( void *key, void *val, void *arg );
void		blastSendCallBack( XObj self, Allocator a );

void		blast_phdr( BLAST_HDR * );
char *		blastOpStr( int );
void		blastShowActiveKey( ActiveID *key, char *message );
void		blastShowMstate( MSG_STATE *m, char *message );
char *		blastShowMask( BLAST_MASK_PROTOTYPE );

#endif

