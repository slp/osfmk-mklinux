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
 * ip_control.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.25
 * Date: 1993/08/04 00:43:16
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip.h>
#include <xkern/protocols/ip/ip_i.h>
#include <xkern/protocols/ip/route.h>


#define IPHOSTLEN	sizeof(IPhost)


/*
 * ip_controlsessn
 */
int
ipControlSessn(s, opcode, buf, len)
    XObj s;
    int opcode;
    char *buf;
    int len;
{
    SState        *sstate;
    PState        *pstate;
    IPheader      *hdr;
    
    xAssert(xIsSession(s));
    
    sstate = (SState *)s->state;
    pstate = (PState *)s->myprotl->state;
    
    hdr = &(sstate->hdr);
    switch (opcode) {
	
      case GETMYHOST :
	checkLen(len, IPHOSTLEN);
	*(IPhost *)buf = sstate->hdr.source;
	return IPHOSTLEN;
	
      case GETPEERHOST :
	checkLen(len, IPHOSTLEN);
	*(IPhost *)buf = sstate->hdr.dest;  
	return IPHOSTLEN;
	
      case GETMYHOSTCOUNT:
      case GETPEERHOSTCOUNT:
	checkLen(len, sizeof(int));
	*(int *)buf = 1;
	return sizeof(int);

      case GETMYPROTO :
      case GETPEERPROTO :
	checkLen(len, sizeof(long));
	*(long *)buf = sstate->hdr.prot;
	return sizeof(long);
	
      case GETMAXPACKET :
	checkLen(len, sizeof(int));
	*(int *)buf = IPMAXPACKET;
	return sizeof(int);
	
      case GETOPTPACKET :
	checkLen(len, sizeof(int));
	*(int *)buf = sstate->mtu - IPHLEN;
	return sizeof(int);
	
      case IP_REDIRECT:
	return ipControlProtl(s->myprotl, opcode, buf, len);

      case IP_PSEUDOHDR:
	return 0;
	
      case GETPARTICIPANTS:
	{
	    Part_s	p[2];
	    int		retLen;

	    retLen = xControl(xGetDown(s, 0), opcode, buf, len);
	    if ( retLen < 0 || partExtLen(buf) < 0 || partExtLen(buf) > 2 ) {
		return -1;
	    }
	    partInternalize(p, buf);
	    /* 
	     * The current contents of the participant buffer may
	     * refer to a gateway instead of the end host, or they may
	     * not even be IP hosts.  We must rewrite them.
	     */
	    if ( partPop(p[0]) == 0 ) {
		return -1;
	    }
	    partPush(p[0], &sstate->hdr.dest, sizeof(IPhost));
	    if ( partExtLen(buf) == 2 ) {
		if ( partPop(p[1]) == 0 ) {
		    return -1;
		}
		partPush(p[1], &sstate->hdr.source, sizeof(IPhost));
	    }
	    return (partExternalize(p, buf, &len) == XK_FAILURE) ? -1 : len;
	}

      case IP_GETPSEUDOHDR:
	{
	    IPpseudoHdr	*phdr = (IPpseudoHdr *)buf;

	    checkLen(len, sizeof(IPpseudoHdr));
	    phdr->src = sstate->hdr.source;
	    phdr->dst = sstate->hdr.dest;
	    phdr->zero = 0;
	    phdr->len = 0;
	    phdr->prot = sstate->hdr.prot;
	    return sizeof(IPpseudoHdr);
	}

      default : 
	xTrace0(ipp, 3, "Unhandled opcode -- forwarding");
	return xControl(xGetDown(s, 0), opcode, buf, len);
    }
}



/*
 * ip_controlprotl
 */
int
ipControlProtl(self, opcode, buf, len)
    XObj self;
    int opcode;
    char *buf;
    int len;
{
    PState	*pstate;
    IPhost 	net, mask, gw, dest;
    
    xAssert(xIsProtocol(self));
    pstate = (PState *) self->state;
    
    switch (opcode) {
	
      case IP_REDIRECT :
	{
	    checkLen(len, 2*IPHOSTLEN);
	    net = *(IPhost *)buf;
	    netMaskFind(&mask, &net);
	    gw = *(IPhost *)(buf + IPHOSTLEN);
	    xTrace3(ipp, 4, "IP_REDIRECT : net = %s, mask = %s, gw = %s",
		    ipHostStr(&net), ipHostStr(&mask), ipHostStr(&gw));
	    /*
	     * find which interface reaches the gateway
	     */
	    rt_add(pstate, &net, &mask, &gw, -1, RTDEFAULTTTL);
	    return 0;
	}

      case GETMAXPACKET :
	checkLen(len, sizeof(int));
	*(int *)buf = IPMAXPACKET;
	return sizeof(int);
	
      case GETOPTPACKET:
	/* 
	 * A somewhat meaningless question to be asking the protocol.
	 * It makes more sense to ask an individual session that knows
	 * about the MTU.
	 */
	checkLen(len, sizeof(int));
	*(int *)buf = IPOPTPACKET - IPHLEN;
	return sizeof(int);
	
	/* test control ops - remove later */
      case IP_GETRTINFO :
	/* get route info for a given dest address :
	   in : IP host address 
	   out : route structure for this address
	   */
	{
	    xkern_return_t	xkr;
	    
	    checkLen(len, sizeof(route));
	    dest = *(IPhost *)buf;
	    xkr = rt_get(&pstate->rtTbl, &dest, (route *)buf);
	    return (xkr == XK_SUCCESS) ? sizeof(route) : -1;
	}

      case IP_PSEUDOHDR:
	return 0;

      case ADDMULTI:
	{
		XObj down;
		/*
		 * Join the group.
		 * If not already a member, propagate the request.
		 *
		 * NOT COMPLETE -- we join a group on a particular interface.
		 */
		xTrace0(ipp, 3, "ip ADDMULTI op");
		checkLen(len, sizeof(IPhost));
		down = xGetDown(self, 0);
		switch ( ipGroupJoin((IPhost *)buf, down) ) {

		    case XK_SUCCESS:
			return xControl(down, opcode, buf, len);

		    case XK_ALREADY_BOUND:
			return 0;
		}
		return -1;
	}

      case DELMULTI:
	{
		XObj down;
		/*
		 * Leave the group.
		 * If we are the last to leave,
		 * propagate the request downward.
		 */
		xTrace0(ipp, 3, "ip DELMULTI op");
		checkLen(len, sizeof(IPhost));
		down = xGetDown(self, 0);
		switch ( ipGroupLeave((IPhost *)buf, down) ) {

		    case XK_SUCCESS:
			return xControl(down, opcode, buf, len);

		    case XK_ALREADY_BOUND:
			return 0;
		}
		return -1;
	}

      default:
	xTrace0(ipp,3,"Unrecognized opcode");
	return xControl(xGetDown(self, 0), opcode, buf, len);
    }
}

/*
 * The ipGroupMap remembers reference counts for a multicast address.
 *
 * ptr = pathAlloc(path, size);
 */

static Map ipGroupMap;

struct ipGroup {
	int refcnt;
	Bind binding;
};

struct ipGroupKey {
	IPhost addr;
	XObj down;
};

/*
 * It seems unfair to allocate the map against a session.
 * The entries, however, are allocated against the down session,
 * which may still have its drawbacks.
 *
 * IGMP hooks will be here eventually.
 *
 * MP locking of ipGroupMap will be interesting...
 */

static void
ipGroupInit(void) {
	ipGroupMap = mapCreate(100, sizeof (struct ipGroup), xkDefaultPath);
	xAssert( ipGroupMap != NULL );
}

xkern_return_t
ipGroupJoin (IPhost *h, XObj down) {
	struct ipGroupKey k;
	struct ipGroup *g;

	if (ipGroupMap == NULL)
		ipGroupInit();

	k.addr = *h;
	k.down = down;
	if ( mapResolve(ipGroupMap, (void *)&k, (void *)&g) == XK_SUCCESS ) {
		g->refcnt++;
		return XK_ALREADY_BOUND;
	}
	g = pathAlloc(down->path, sizeof *g);
	xAssert( g );
	g->refcnt = 1;
	switch ( mapBind(ipGroupMap, (void *)&k, (void *)g, &g->binding) ) {

	case XK_SUCCESS:
		break;

	case XK_ALREADY_BOUND:
		/*
		 * Indicates a race -- shouldn't happen...
		 */
		pathFree(g);
		return ipGroupJoin(h, down);

	default:
		pathFree(g);
		return XK_FAILURE;

	}
	/* IGMP group join hook here */
	return XK_SUCCESS;
}

xkern_return_t
ipGroupLeave(IPhost *h, XObj down) {
	struct ipGroupKey k;
	struct ipGroup *g;

	if (ipGroupMap == NULL)
		ipGroupInit();

	k.addr = *h;
	k.down = down;
	if ( mapResolve(ipGroupMap, (void *)&k, (void *)&g) == XK_FAILURE )
		return XK_FAILURE;

	if (--g->refcnt > 0)
		return XK_SUCCESS;

	/* IGMP group exit hook here */

	(void)mapRemoveBinding(ipGroupMap, g->binding);

	pathFree(g);

	return XK_SUCCESS;
}

xkern_return_t
ipGroupQuery(IPhost *h, XObj down) {
	struct ipGroupKey k;

	if (ipGroupMap == NULL)
		ipGroupInit();

	k.addr = *h;
	k.down = down;

	return mapResolve(ipGroupMap, (void *)&k, (void *)NULL);
}
