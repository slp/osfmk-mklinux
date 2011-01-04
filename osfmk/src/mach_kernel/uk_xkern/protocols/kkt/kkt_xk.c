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

#include <uk_xkern/protocols/kkt/kkt_i.h>
#include <xkern/include/prot/mship.h>

#if XK_DEBUG
int	tracekktp;
#endif

#define MAX_LENGTH_DEFAULT (1400 * 16)
#define MAX_LENGTH_ABSOLUTE (0x10000)

XObj thisprotocol = ERR_XOBJ;
Map NodeNameToNapMap;
Map NapToNodeNameMap;

node_name _node_self = NODE_NAME_NULL;

xkern_return_t
kkt_init(
	XObj self );	 

#if DONT_USE_IPHOST 

#define MSHIP_PROTOCOL 1

xkern_return_t
node_name_to_nap(
		 node_name in,
		 IPhost    *out)
{
    MshipIface_s ifmship;
    xkern_return_t	xkr;

    ifmship.node = in;
    if (xControl(xGetDown(thisprotocol, MSHIP_PROTOCOL),
		 RESOLVE,
		 (char *)&ifmship,
		 sizeof(MshipIface_s)) < 0) {
	return XK_FAILURE;
    }

    *out = ifmship.nap;
    xTrace1(kktp, TR_DETAILED, " node_name <%d> ", in);
    xTrace5(kktp, TR_DETAILED, " --> nap <%d.%d.%d.%d> returns <%d>\n",
	    (unsigned) (*out).a, (unsigned) (*out).b, 
	    (unsigned) (*out).c, (unsigned) (*out).d, xkr);
    return XK_SUCCESS;
}

xkern_return_t
nap_to_node_name(
		 IPhost    in,
		 node_name *out)
{
    MshipIface_s ifmship;
    xkern_return_t	xkr;

    ifmship.nap = in;
    if (xControl(xGetDown(thisprotocol, MSHIP_PROTOCOL),
		 RRESOLVE,
		 (char *)&ifmship,
		 sizeof(MshipIface_s)) < 0) {
	return XK_FAILURE;
    }
    *out = ifmship.node;
    xTrace4(kktp, TR_DETAILED, "nap <%d.%d.%d.%d> --> ",
	    (unsigned) (in).a, (unsigned) (in).b, 
	    (unsigned) (in).c, (unsigned) (in).d);
    xTrace2(kktp, TR_DETAILED, " node_name <%d> returns <%d>\n", *out, xkr);
    return XK_SUCCESS;
}
#else  /* DONT_USE_IPHOST */

xkern_return_t
node_name_to_nap(
		 node_name in,
		 IPhost    *out)
{
    *(node_name *)out = in;
    return XK_SUCCESS;
}

xkern_return_t
nap_to_node_name(
		 IPhost    in,
		 node_name *out)
{
    *(IPhost *)out = in;
    return XK_SUCCESS;
}

#endif /* DONT_USE_IPHOST */

static xkern_return_t
serverCallDemux(
		XObj self, 
		XObj lls, 
		Msg dg, 
		Msg rmsg)
{
    xTrace0(kktp, TR_MAJOR_EVENTS,  "KKT serverCallDemux()");
    
    kkt_server_upcall(lls, dg, rmsg);
    return XK_SUCCESS;
}

static xkern_return_t
clientDemux(
	    XObj self,
	    XObj lls,
	    Msg  rmsg)
{
    xTrace0(kktp, TR_MAJOR_EVENTS,  "KKT clientDemux()");
    
    kkt_client_upcall(rmsg);
    return XK_SUCCESS;
}

static xkern_return_t
saveServerSessn(
		XObj self, 
		XObj lls, 
		XObj llp, 
		XObj hlpType,
		Path path)
{
    xTrace0(kktp, TR_MAJOR_EVENTS,  "KKT duplicates lower server session");
    xDuplicate(lls);
    return XK_SUCCESS;
}

xkern_return_t
kkt_init(
	 XObj self)
{
    XObj	llp;
    Part_s	p;
    IPhost      myHost;
    PSTATE      *ps;

    xTrace0(kktp, TR_FULL_TRACE, "init called");

    if (thisprotocol != ERR_XOBJ)
	panic("Only one instance of KKT is allowed");

    if ( ! (self->state = ps = pathAlloc(self->path, sizeof(PSTATE))) ) {
        xTrace0(kktp, TR_ERRORS, "allocation errors in init");
        return XK_FAILURE;
    }
    
    llp = xGetDown(self, 0);
    if (!xIsProtocol(llp)) {
	panic("KKT has no lower protocol");
    }
    /* self->opendone = saveServerSessn; */
    self->calldemux = serverCallDemux;
    self->demux = clientDemux;

    partInit(&p, 1);
    partPush(p, ANY_HOST, 0);
 
    if (xOpenEnable(self, self, llp, &p) == XK_FAILURE) {
	panic("KKT can't openenable lower protocol");
    } else {
	xTrace0(kktp, TR_FULL_TRACE, "KKT xopenenable(%d) OK");
    }

    if (xControl(xGetDown(self, 0), GETMYHOST, (char *)&myHost, sizeof(IPhost)) 
	                                         < (int)sizeof(IPhost))	{
	panic("KKT couldn't get local address");
    }
	    
    xTrace4(kktp, TR_FULL_TRACE, "local IP address: %u %u %u %u",
	    (unsigned) myHost.a, (unsigned) myHost.b, 
	    (unsigned) myHost.c, (unsigned) myHost.d);

    /*
     * Determine some parameters which are typically session dependent.
     * To do so, I open the loopback session, with the assumption that
     * it's no different from a real node-to-node session (I don't 
     * care about IP being out of the loop: it means that something else
     * does provide fragmentation).
     */
    {
	    Part_s	participants[2], *p;
	    unsigned int attr = 1;  /* random */
	    XObj lls;

	    p = &participants[0];
	    partInit(p, 2);
	    partPush(*p, &attr, sizeof(attr));
	    partPush(*p, &myHost, sizeof(myHost));
	    lls = xOpen(self, self, xGetDown(self, 0), p, self->path);	    
	    if (lls == ERR_XOBJ) {
		xTrace0(kktp, TR_ERRORS,  "KKT can't open lower protocol");
		return XK_FAILURE;
	    }
	    if (xControl(lls, GETMAXPACKET, (char *)&ps->max_length, sizeof(ps->max_length)) < sizeof(ps->max_length)) {
		xTrace0(kktp, TR_ERRORS,  "KKT can't get maximum length. Using default");
		ps->max_length = MAX_LENGTH_DEFAULT;
	    }
	    xClose(lls);
    }
    ps->max_length -= KKT_STACK_SIZE;
    ps->max_length = MIN(MAX_LENGTH_ABSOLUTE, ps->max_length);
    /*
     * Unlock threads waiting for initialization to finish.
     */
    thisprotocol = self;

    if (nap_to_node_name(myHost, &_node_self) != XK_SUCCESS) {
	panic("KKT could not get node name resolution for local address");
    }
    assert(_node_self != NODE_NAME_NULL);
    thread_wakeup((event_t)&thisprotocol);

    return XK_SUCCESS;
}






