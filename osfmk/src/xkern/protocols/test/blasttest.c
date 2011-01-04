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
 * blasttest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.13
 * Date: 1993/05/10 22:54:31
 */

/*
 * Ping-pong test of BLAST
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/blast.h>

/*
 * These definitions describe the lower protocol
 */
#if NORMA_ETHER

#define HOST_TYPE IPhost

static HOST_TYPE ServerAddr = { 0, 0, 0, 0 };
static HOST_TYPE ClientAddr = { 0, 0, 0, 0 };

#else  /* NORMA_ETHER */

#define HOST_TYPE long

static HOST_TYPE ServerAddr;
static HOST_TYPE ClientAddr;

#define STR2HOST str2addr

static void
str2addr(
    long	*h,
    char	*s)
{
    *h = atoi(s);
}
#endif /* NORMA_ETHER */

#define INIT_FUNC	blasttest_init
#define TRACE_VAR	traceblasttestp
#define PROT_STRING	"blast"
/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
#define TRIPS		100
#define TIMES		1
#define DELAY		3
/*
 * Define to do timing calculations
 */
#define TIME
#define SUPER_CLOCK	0


static int lens[] = { 
 8192, 16384, 32768, 65536, 131072, 262144
/*
  16384,
  16384,
  16384,
  16384,
  16384,
  16384,
  16384,
  16384,
  16384,
  16384,
  16384,
  16384,
  16384
*/
};


#define USE_KILL_TICKET
/*
#define ALL_TOGETHER
#define SIMPLE_ACKS
#define REPLY_SIZE 4
*/

#ifdef USE_KILL_TICKET
#define CUSTOM_CLIENT_DEMUX
#define CUSTOM_SERVER_DEMUX

void	customClientDemux( XObj, XObj, Msg, int );
void	customServerDemux( XObj, XObj, Msg, int );

#endif /* USE_KILL_TICKET */


#define SAVE_SERVER_SESSN


#include <xkern/protocols/test/common_test.c>

#ifndef	MACH_KERNEL


void
customClientDemux( self, lls, dg, which)
    XObj        self, lls;
    Msg         dg;
    int	 	which;
{
    PState	*ps = (PState *)self->state;

    if ( ps->clientPushResult[which] == 0 ) return;
    if (xControl(ps->lls, FREERESOURCES,
                 (char *)&ps->clientPushResult[which], sizeof(int))) {
        printf("FREERESOURCES on client session failed\n");
    }
}


void
customServerDemux( self, lls, dg, which )
    XObj        self, lls;
    Msg         dg;
    int	 	which;
{
    PState	*ps = (PState *)self->state;

    if ( ps->serverPushResult[which] == 0 ) return;
    if (xControl(lls, FREERESOURCES,
                 (char *)&ps->serverPushResult[which], sizeof(int))) {
        printf("FREERESOURCES on server session failed\n");
    }
}

static void
testInit( XObj self )
{
}

#else	/* MACH_KERNEL */

typedef struct {
    queue_chain_t       link;
    XObj		lls;
    int 		num;
} ticket_t;

#define TICKET_NUMBER	100
ticket_t tickets[TICKET_NUMBER];
queue_head_t        busy_tickets;
queue_head_t        free_tickets;
thread_t	    blast_thread;
int 		    how_many_tickets;

#ifdef USE_KILL_TICKET

static void
kill_ticket(
	    XObj lls,
	    int num )
{
    ticket_t	*t;

    queue_remove_first(&free_tickets, t, ticket_t *, link);
    assert(t);
    t->lls = lls;
    t->num = num;
    queue_enter(&busy_tickets, t, ticket_t *, link);
    how_many_tickets++;
    clear_wait(blast_thread,THREAD_AWAKENED,FALSE);
}

void
customClientDemux( self, lls, dg, which )
    XObj	self, lls;
    Msg		dg;
    int 	which;
{
    PState	*ps = (PState *)self->state;

    if (ps->clientPushResult[which] == XMSG_NULL_HANDLE) 
	return;
    kill_ticket(lls, ps->clientPushResult[which]);
}

void
customServerDemux(self, lls, dg, which)
    XObj	self, lls;
    Msg		dg;
    int 	which;
{
    PState	*ps = (PState *)self->state;

    if (ps->serverPushResult[which] == XMSG_NULL_HANDLE) 
	return;
    kill_ticket(lls, ps->serverPushResult[which]);
}

#endif /* USE_KILL_TICKET */

static void
blast_loop( 
	   Event evSelf,
	   void *arg )
{
    register ticket_t *t;
     
    blast_thread = current_thread();

    while (1) {
	while (queue_empty(&busy_tickets)) {
           /*
            * Wait for something to do.
            */
           assert_wait(0, FALSE);
           xk_master_unlock();
           thread_block(0);   /* block ourselves */
           xk_master_lock();
	}
        queue_remove_first(&busy_tickets, t, ticket_t *, link);
	(void)xControl(t->lls, FREERESOURCES, (char *)&t->num, sizeof(int));
        how_many_tickets--;
        queue_enter(&free_tickets, t, ticket_t *, link);
    }
}


static void
testInit( XObj self )
{
    register int i;
    Event ev;

    queue_init(&busy_tickets);
    queue_init(&free_tickets);
    for (i = 0; i < TICKET_NUMBER; i++) {
	queue_enter(&free_tickets, &tickets[i], ticket_t *, link);
    }
    if ( ! (ev = pathAlloc(self->path, sizeof(Event))) ) {
	xError("blasttest can't allocate event");
	return;
    }
    evDetach(evSchedule(ev, blast_loop, 0, 0));
}

#endif  /* MACH_KERNEL */


