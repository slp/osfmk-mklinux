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
 * arp_table.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * arp_table.c,v
 * Revision 1.20.4.2.1.2  1994/09/07  04:18:25  menze
 * OSF modifications
 *
 * Revision 1.20.4.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 */


#include <xkern/include/xkernel.h>
#include <xkern/include/prot/eth.h>
#include <xkern/include/prot/ip.h>
#include <xkern/include/prot/arp.h>
#include <xkern/protocols/arp/arp_i.h>
#include <xkern/protocols/arp/arp_table.h>

#define	ARP_TAB   100			/* arp table size */

/*
 * Number of msec between age ticks
 */
#define AGE_INTERVAL 60 * 1000		/* 1 minute   */

/*
 * Default number of ticks
 */
#define TTL_DEF 10		/* 10 minutes */

typedef struct arpent {
    ArpStatus	status;
    ArpWait	wait;
    ETHhost	arp_Ead;
    IPhost 	arp_Iad;
    boolean_t	locked;
    int		ttl;
} ArpEnt;

static void 	ageTimer( Event, void * );
static int	getPos( ArpTbl );	
static int	clearIndex( ArpTbl, int );
static char *	dispEntry( ArpTbl, int );
static void	dispTable( ArpTbl );
static int	ethToIndex( ArpTbl, ETHhost *h );
static int	ipToIndex( ArpTbl, IPhost *h );
static void	signalAll( ArpTbl, int );
static int	waitForAnswer( ArpTbl, ArpEnt * );
static void	writeEntry( ArpTbl, int, IPhost *, ETHhost * );
static int	reverseLookup( XObj, IPhost *, ETHhost *, boolean_t );


static void
freeTbl( ArpTbl tbl )
{
    int	i;

    for ( i=0; i < ARP_TAB && tbl[i]; i++ ) {
	allocFree(tbl[i]);
    }
    allocFree(tbl);
}


/* 
 * See description in arp_table.h
 */
ArpTbl
arpTableInit( Path p )
{
    ArpTbl	tbl;
    ArpEnt	*e;
    int 	i;
    Event	ev;

    if ( ! (tbl = (ArpEnt **)pathAlloc(p, ARP_TAB * sizeof(ArpEnt *))) ) {
	return 0;
    }
    for (i=0; i < ARP_TAB; i++) {
	if ( ! (tbl[i] = pathAlloc(p, sizeof(ArpEnt))) ) {
	    freeTbl(tbl);
	    return 0;
	}
	bzero((char *)tbl[i], sizeof(ArpEnt));
	tbl[i]->status = ARP_FREE;
    }
    if ( ! (ev = evAlloc(p)) ) {
	xTrace0(arpp, TR_ERRORS, "arpTableInit couldn't allocate event");
	freeTbl(tbl);
	return 0;
    }
    evSchedule(ev, ageTimer, tbl, 0);
    return tbl;
}


/* 
 * See description in arp_table.h
 */
int
arpLookup(self, ipHost, ethHost)
    XObj	self;
    IPhost 	*ipHost;
    ETHhost 	*ethHost; 
{
    ArpTbl	tbl = ((PState *)self->state)->tbl;
    int		pos, reply;
    ArpEnt	*ent;
    
    xTrace1(arpp, 3, "arpLookup looking up %s", ipHostStr(ipHost));
    if ( (pos = ipToIndex(tbl, ipHost)) == -1 ) {
	pos = getPos(tbl);
	ent = tbl[pos];
	newArpWait( &ent->wait, self, ipHost, &ent->status );
	ent->status = ARP_ALLOC;
	ent->arp_Iad = *ipHost;
	if ( arpSendRequest(&ent->wait) == XK_FAILURE ) {
	    ent->status = ARP_FREE;
	    return -1;
	}
	/*
	 * We'll wait for the reply in the ARP_ALLOC branch of the switch
	 */
    } else {
	ent = tbl[pos];
    }
    switch ( ent->status ) {

      case ARP_RSLVD:
	xTrace0(arpp, 4, "Had it");
	*ethHost = ent->arp_Ead;
	xTrace1(arpp, TR_MORE_EVENTS, "eth host is %s", ethHostStr(ethHost));
	reply = 0;
	break;

      case ARP_ALLOC:
	/* someone else must have requested it */
	xTrace0(arpp, 4, "Request has been sent -- waiting for reply");
	reply = waitForAnswer(tbl, ent);
	*ethHost = ent->arp_Ead;
	break;

      default:
	xTrace1(arpp, 3, "arp_lookup_ip returns bizarre code %d",
		ent->status);
	xAssert(0);
	reply = 0;
    }
    return reply;
}


/* 
 * reverseLookup -- Find the IP host equivalent of the given ETH host.
 * If the value is not in the table, network RARP requests will be sent if
 * useNet is true.
 * Returns 0 if the lookup was successful and -1 if it was not.
 */
static int
reverseLookup( self, ipHost, ethHost, useNet )
    XObj	self;
    IPhost 	*ipHost;
    ETHhost 	*ethHost;
    boolean_t 	useNet; 
{
    ArpTbl	tbl = ((PState *)self->state)->tbl;
    int		pos, reply;
    ArpEnt	*ent;
    
    xTrace1(arpp, 3, "arpRevLookup looking up %s", ethHostStr(ethHost));
    if ( (pos = ethToIndex(tbl, ethHost)) == -1 ) {
	/*
	 * Entry was not in the table
	 */
	if ( ! useNet ) {
	    return -1;
	}
	pos = getPos(tbl);
	ent = tbl[pos];
	ent->status = ARP_ALLOC;
	ent->arp_Ead = *ethHost;
	if ( (newRarpWait(&ent->wait, self, ethHost, &ent->status)
	    	!= XK_SUCCESS) ||
	     (arpSendRequest(&ent->wait) == XK_FAILURE) ) {
	    ent->status = ARP_FREE;
	    return -1;
	}
	/*
	 * We'll wait for the reply in the ARP_ALLOC branch of the switch
	 */
    } else {
	ent = tbl[pos];
    }
    switch ( ent->status ) {

      case ARP_RSLVD:
	xTrace0(arpp, 4, "Had it");
	*ipHost = ent->arp_Iad;
	reply = 0;
	break;

      case ARP_ALLOC:
	/* someone else must have requested it */
	if ( useNet ) {
	    xTrace0(arpp, 4, "Request has been sent -- waiting for reply");
	    reply = waitForAnswer(tbl, ent);
	    *ipHost = ent->arp_Iad;
	} else {
	    reply = -1;
	}
	break;

      default:
	xTrace1(arpp, 3, "arp_lookup_eth returns bizarre code %d",
		ent->status);
	xAssert(0);
	reply = -1;
    }
    return reply;
}


/* 
 * See description in arp_table.h
 */
int
arpRevLookup( self, ipHost, ethHost )
    XObj	self;
    IPhost 	*ipHost;
    ETHhost 	*ethHost;
{
    return reverseLookup( self, ipHost, ethHost, TRUE );
}


/* 
 * See description in arp_table.h
 */
int
arpRevLookupTable( self, ipHost, ethHost )
    XObj	self;
    IPhost 	*ipHost;
    ETHhost 	*ethHost;
{
    return reverseLookup( self, ipHost, ethHost, FALSE );
}


static int
waitForAnswer( tbl, ent )
    ArpTbl	tbl;
    ArpEnt	*ent;
{
    if ( ent->status != ARP_RSLVD ) {
	ent->wait.numBlocked++;
	semWait(&ent->wait.s);
	ent->wait.numBlocked--;
    }
    if ( ent->status == ARP_RSLVD ) {
	return 0;
    } else {
	xTrace1(arpp, TR_SOFT_ERRORS,
		"ARP returning error (status %d)", ent->status);
	return -1;
    }
}


/* 
 * See description in arp_table.h
 */
void
arpLock( tbl, h )
    ArpTbl	tbl;
    IPhost 	*h;
{
    int i;
    
    if ( (i = ipToIndex(tbl, h)) >= 0 ) {
	xAssert( tbl[i]->status != ARP_FREE );
	tbl[i]->locked = 1;
    }
}


/* 
 * See description in arp_table.h
 */
void
arpSaveBinding( tbl, ip, eth )
    ArpTbl	tbl;
    IPhost 	*ip;
    ETHhost 	*eth;		
{
    int 	ipPos, ethPos;
    
    if ( ip == NULL ) {
	clearIndex(tbl, ethToIndex(tbl, eth));
	return;
    }
    if ( eth == NULL ) {
	clearIndex(tbl, ipToIndex(tbl, ip));
	return;
    }
    xTrace2(arpp, 4, "arp saving %s -> %s", ethHostStr(eth), ipHostStr(ip));
    /*
     * Find a table position for this binding, clearing out old entries
     * with these host values.  This may, in rare cases,  result in a
     * duplicate table entry.  This just wastes a space until the
     * entries time out.
     */
    ipPos = ipToIndex(tbl, ip);
    if ( ipPos >= 0 ) {
	/*
	 * This ip address is already bound to something
	 */
	ArpEnt	*t = tbl[ipPos];

	if ( bcmp((char *)&t->arp_Ead, (char *)eth, sizeof(ETHhost)) == 0 ) {
	    if ( t->status == ARP_RSLVD ) {
		xTrace0(arpp, 3, "arpSaveBinding: already had this binding");
		return;
	    }
	}
    }
    ethPos = ethToIndex(tbl, eth);
    if ( ipPos < 0 && ethPos < 0 ) {
	/*
	 * Neither 'eth' nor 'ip' were bound to anything.  
	 */
	ipPos = getPos(tbl);
    }
    if ( ipPos >= 0 ) {
	writeEntry(tbl, ipPos, ip, eth);
    }
    if ( ethPos >= 0 ) {
	writeEntry(tbl, ethPos, ip, eth);
    }
}


static void
writeEntry( tbl, pos, ip, eth )
    ArpTbl	tbl;
    int		pos;
    IPhost	*ip;
    ETHhost	*eth;
{
    ArpEnt	*t;

    if ( pos < 0 || pos >= ARP_TAB ) {
	sprintf(errBuf, "arp table indexing error (i == %d)", pos);
	xError(errBuf);
	return;
    }
    t = tbl[pos];
    if ( t->locked ) {
	xTrace1(arpp, 3,
		"saveBinding can't override lock: \n%s", dispEntry(tbl, pos));
    } else {
	t->arp_Iad = *ip;
	t->arp_Ead = *eth;
	t->status = ARP_RSLVD;
	t->locked = 0;
	t->ttl = TTL_DEF;
	signalAll(tbl, pos);
    }
}


static void
ageTimer( ev, arg )
    Event	ev;
    void	*arg;
{
    ArpTbl	tbl = (ArpTbl)arg;
    int 	i;
    ArpEnt	*t;

    xTrace0(arpp, TR_EVENTS, "ARP ageTimer runs");
    xIfTrace(arpp, TR_FULL_TRACE) dispTable(tbl);
    for ( i=0; i < ARP_TAB; i++ ) {
	t = tbl[i];
	if ( t->locked ) continue;
	if ( t->status == ARP_RSLVD && t->ttl-- <= 0 ) {
	    xTrace1(arpp, 3, "ageTimer kicks out %s", dispEntry(tbl, i));
	    clearIndex(tbl, i);
	}
    }
    evSchedule(ev, ageTimer, arg, AGE_INTERVAL * 1000);
    xTrace0(arpp, 3, "ARP ageTimer completes");
    xIfTrace(arpp, 9) dispTable(tbl);
}


void
arpTblPurge( tbl, myiph )
    ArpTbl	tbl;
    IPhost	*myiph;
{
    int		i;
    ArpEnt	*t;
    
    xTrace1(arpp, 3, "ARP tblPurge (%s)", ipHostStr(myiph));
    xIfTrace(arpp, 9) dispTable(tbl);
    for ( i=0; i < ARP_TAB; i++ ) {
	t = tbl[i];
	if ( t->status == ARP_RSLVD &&
	      ! netMaskSubnetsEqual(&t->arp_Iad, myiph) ) {
	    xTrace1(arpp, 3, "arpPurge kicks out %s", dispEntry(tbl, i));
	    clearIndex(tbl, i);
	}
    }
    xIfTrace(arpp, 9) dispTable(tbl);
}


/*
 * ipToIndex -- return the table index containing the entry for the
 * specified ip host if it exists, -1 if it does not.
 */
static int
ipToIndex( tbl, h )
    ArpTbl	tbl;
    IPhost 	*h;
{
    int	i;

    for (i=0; i<ARP_TAB; i++) {
	if (tbl[i]->status != ARP_FREE &&
	    ! bcmp((char *)h, (char *)&tbl[i]->arp_Iad, sizeof(IPhost))) {
	    return(i);
	}
    }
    return(-1);
}
  

/*
 * ethToIndex -- return the table index containing the entry for the
 * specified eth host if it exists, -1 if it does not.
 */
static int
ethToIndex( tbl, h )
    ArpTbl	tbl;
    ETHhost 	*h;			
{
    int	i;
    
    for (i=0; i<ARP_TAB; i++) {
	if (tbl[i]->status != ARP_FREE &&
	    ! bcmp((char *)h, (char *)&tbl[i]->arp_Ead,
		   sizeof(ETHhost))) {
	    return(i);
	}
    }
    return(-1);
}


/*
 * getPos -- return a new position for binding
 */
static int
getPos( tbl )
    ArpTbl	tbl;
{
    int		i;
    int		ouster;
    ArpEnt	*t;
    
    ouster = -1;
    for (i=0; i<ARP_TAB; i++) {
	t = tbl[i];
	switch( t->status ) {

	  case ARP_FREE:
	    if ( t->wait.numBlocked > 0 ) {
		continue;
	    }
	    t->status = ARP_ALLOC;
	    return i;
	    
	  case ARP_RSLVD:
	    if ( ouster == -1 ||
		( ! t->locked && tbl[ouster]->ttl > t->ttl ) ) {
		ouster = i;
	    }
	    break;
	    
	  case ARP_ALLOC:
	    break;
	    
	  default:
	    sprintf(errBuf,
		    "getPos finds bizarre table status:\n%s",
		    dispEntry(tbl, i));
	    xError(errBuf);
	    break;
	}
    }
    /*
     * Need to kick someone appropriate out
     */
    if ( ouster < 0 ) {
	dispTable(tbl);
	Kabort("arp couldn't free any table entries");
    }
    clearIndex(tbl, ouster);
    return ouster;
}


/*
 * signalAll -- release all threads waiting on this table entry and
 * free the event state associated with this entry
 */
static void
signalAll( tbl, indx )
    ArpTbl	tbl;
    int 	indx;
{
    int		i;
    ArpWait	*w;
    
    w = &tbl[indx]->wait;
    for (i=0; i < w->numBlocked; i++) {
	semSignal(&w->s);
    }
    if ( w->event ) {
	evCancel(w->event);
	evDetach(w->event);
	w->event = 0;
    }
    if ( xIsSession(w->lls) ) {
	xClose(w->lls);
	w->lls = 0;
    }
    xIfTrace(arpp, 9) dispTable(tbl);
}


/*
 * Clear the indexed table entry.  Returns -1 only if the entry was locked.
 */
static int
clearIndex( tbl, indx )
    ArpTbl	tbl;
    int 	indx; 
{
    ArpEnt	*t;

    if ( indx < 0 || indx >= ARP_TAB ) {
	return 0;
    }
    t = tbl[indx];
#if 0
    if ( t->status == ARP_RSLVD && t->locked ) {
	return -1;
    }
#endif
    signalAll(tbl, indx);
    t->status = ARP_FREE;
    t->locked = 0;
    return 0;
}


static char *
dispEntry( tbl, i )
    ArpTbl	tbl;
    int 	i;
{
    ArpEnt	*t;
    static char	buf[160];
    char	tmpBuf[80];
    
    t = tbl[i];
    sprintf(buf, "i=%d  ", i);
    switch ( t->status ) {
	
      case ARP_FREE:
	(void)strcat(buf, "FREE    ");
	break;
	
      case ARP_ALLOC:
	{
	    strcat(buf, "ALLOC    ");
	    switch ( t->wait.reqMsg.arp_op ) {
		
	      case ARP_REQ:
		sprintf(tmpBuf, "ARP     %s  %d waiters   ",
			ipHostStr(&t->wait.reqMsg.arp_tpa),
			t->wait.numBlocked);
		strcat(buf, tmpBuf);
		break;
		
	      case ARP_RREQ:
		sprintf(tmpBuf, "RARP     %s  %d waiters   ",
			ethHostStr(&t->wait.reqMsg.arp_tha),
			t->wait.numBlocked);
		strcat(buf, tmpBuf);
		break;
		
	      default:
		strcat(buf, "ALLOC -- type unknown");
		break;
	    }
	}
	break;
	
      case ARP_RSLVD:
	sprintf(tmpBuf, "RSLVD   %s <-> %s   ttl = %d   ",
		ipHostStr(&t->arp_Iad), ethHostStr(&t->arp_Ead),
		t->ttl);
	strcat(buf, tmpBuf);
	break;
	
      default:
	sprintf(tmpBuf, "Unknown ArpStatus %d   ", t->status);
	strcat(buf, tmpBuf);
    }
    if ( t->locked ) {
	strcat(buf, "LOCKED  ");
    }
    return buf;
}


static void
dispTable( tbl )
    ArpTbl	tbl;
{
    int i;

    xTrace0(arpp, 0, "-----------");
    for( i=0; i < ARP_TAB; i++ ) {
	if ( tbl[i]->status == ARP_FREE ) continue;
	xTrace0(arpp, 0, dispEntry(tbl, i));
    }
}


void
arpForEach( tbl, afe )
    ArpTbl	tbl;
    ArpForEach	*afe;
{
    int		i;
    ArpBinding	b;

    for ( i=0; i < ARP_TAB; i++ ) {
	if ( tbl[i]->status == ARP_RSLVD ) {
	    b.hw = tbl[i]->arp_Ead;
	    b.ip = tbl[i]->arp_Iad;
	    afe->f(&b, afe->v);
	}
    }
}
