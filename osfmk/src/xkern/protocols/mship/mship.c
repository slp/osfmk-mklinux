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
/*
 * This protocol is the seed for a true membership service.
 * It was born as a trivial subset of the ARP protocol
 * functionalities. It is meant to grow up to a real
 * membership protocol to support the notion of node groups
 * (as requested by the fault isolation/partition work).
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/part.h>
#include <xkern/include/prot/eth.h>
#include <xkern/include/prot/ip_host.h>
#include <xkern/include/prot/mship.h>
#include <xkern/include/prot/bidctl.h>

#define MAX_RESOLUTION_ENTRIES 100
#define MSHIP_STACK_SIZE       100
#define MSHIP_RETRY_INTERVAL   1000000
#define NODE_INVALID           ((u_bit32_t)-1)
#define MSHIP_MAX_RETRIES      30
#define MSHIP_CACHE            32
#define MSHIP_VERSION          3

static IPhost   ipNull = { 0, 0, 0, 0 };

#if XK_DEBUG
int tracemshipp;
#endif /* XK_DEBUG */

typedef struct {
    Map         NodeNameToNapMap;
    Map         NapToNodeNameMap;
    IPhost      FastNodeName2Nap[MSHIP_CACHE];
    ClusterId   myCluster;
    IPhost      myHost;   
    u_bit32_t   myNode;
    boolean_t   verbose;
} PSTATE;    

typedef enum {
    MSHIP_RESOLVE = 1, 
    MSHIP_RESOLVE_REPLY,
    MSHIP_RRESOLVE,
    MSHIP_RRESOLVE_REPLY,
    MSHIP_ERROR
} mship_mode;

typedef struct {
    u_int        version;
    mship_mode   type;
    ClusterId    cluster;
    struct mship_pcb_s  *pcb;
    MshipIface_s whoami;
    MshipIface_s mif;
} MSHIP_HDR;

typedef struct mship_pcb_s {
    XObj lls;
    xkSemaphore sem;
    Event       event;
    Msg         message;
    MSHIP_HDR   rhdr;
    unsigned int retries;
} *mship_pcb;

static xkern_return_t 
mshipLoadEntry( XObj, char **, int, int, void * );

static xkern_return_t 
mshipLoadVerbose( XObj, char **, int, int, void * );

xkern_return_t
str2node(
	 u_bit32_t *n,
	 char *s);

static XObjRomOpt	mshipOpts[] =
{
    { "node_name", 4, mshipLoadEntry },
    { "verbose", 2, mshipLoadVerbose }
};

static xkern_return_t
mshipSendWaitTimeout(
		     XObj self, 
		     MSHIP_HDR *hdr, 
		     Part p);

void
mshipTimeout(
	    Event	ev,
	    void 	*arg);

static xkern_return_t
mshipLookup(
	    XObj self,
	    MshipIface mif);


static xkern_return_t
mshipReverseLookup(
		   XObj self,
		   MshipIface mif);

static int
mshipControlProtl(
		  XObj self, 
		  int  op, 
		  char *buf, 
		  int  len);

xkern_return_t
mship_init(
	   XObj self );	 

void
mshipAddEntry(
	      XObj self,
	      MshipIface mif);
	      
static xkern_return_t
mshipDemux(
	   XObj self,
	   XObj lls,
	   Msg  message);

void
mship_fillinfunc( 
	       void *hdr, 
	       char *des, 
	       long len, 
	       void *arg);

long
mship_pophdr(
	   void *hdr,
	   char *netHdr,
	   long int len,
	   void *arg);

xkern_return_t
str2node(
	u_bit32_t *n,
	char *s)
{
    *n = atoi(s);
    return XK_SUCCESS;
}

static xkern_return_t
mshipLoadEntry(
	  XObj	self,
	  char	**str,
	  int		nFields,
	  int		line,
	  void	*arg)
{
    PSTATE	*ps = (PSTATE *)self->state;
    int         internal;
    Bind        bd;
    MshipIface_s mifs;
    xkern_return_t xkr;

    if ( str2ipHost(&mifs.nap, str[2]) == XK_FAILURE )
    	return XK_FAILURE;
    mifs.node = NODE_INVALID;
    if (str2node(&mifs.node, str[3]) != XK_SUCCESS || mifs.node == NODE_INVALID)
    	return XK_FAILURE;

    mshipAddEntry(self, &mifs);
    return XK_SUCCESS;
}

static xkern_return_t
mshipLoadVerbose(
		 XObj	self,
		 char	**str,
		 int	nFields,
		 int	line,
		 void	*arg)
{
    PSTATE	*ps = (PSTATE *)self->state;

    ps->verbose = TRUE;
    return XK_SUCCESS;
}

static xkern_return_t
mshipSendWaitTimeout(
		     XObj self, 
		     MSHIP_HDR *hdr, 
		     Part p)
{
    XObj lls, llp = xGetDown(self, 0);
    PSTATE	*ps = (PSTATE *)self->state;
    Msg request;
    Msg_s lcopy;
    xkern_return_t xkr;
    struct mship_pcb_s pcb_s, *pcb;

    pcb = &pcb_s;
    memset((char *)pcb, 0, sizeof(struct mship_pcb_s));
    if ( ! (pcb->event = evAlloc(self->path))) {
	return XK_FAILURE;
    }
    pcb->message = request = pathAlloc(self->path, sizeof(Msg_s));
    if (request == (Msg)0) {
	evDetach(pcb->event);
	return XK_FAILURE;
    }
    semInit(&pcb->sem, 0);
    pcb->retries = 0;

    pcb->lls = lls = xOpen(self, self, llp, p, self->path);
    if (lls == ERR_XOBJ) {
        xTrace0(mshipp, TR_ERRORS,  "MSHIP can't open lower protocol");
	evDetach(pcb->event);
	pathFree(pcb->message);
	return XK_FAILURE;
    }

    if (msgConstructContig(request, self->path, 0, MSHIP_STACK_SIZE) != XK_SUCCESS) {
	xTrace0(mshipp, TR_ERRORS, "MSHIP Unable to create request message\n");
	evDetach(pcb->event);
	pathFree(pcb->message);
	return XK_FAILURE;
    }
    hdr->version = MSHIP_VERSION;
    hdr->pcb = pcb;
    hdr->cluster = ps->myCluster;
    xkr = msgPush(request, mship_fillinfunc, hdr, sizeof (*hdr), (void *)0);
    xAssert(xkr == XK_SUCCESS);

    xTrace0(mshipp, TR_FULL_TRACE, "Sending a request \n");

    msgConstructCopy(&lcopy, request);
    xkr = xPush(lls, &lcopy);
    msgDestroy(&lcopy);

    if (xkr != XK_SUCCESS) {
	evDetach(pcb->event);
	msgDestroy(pcb->message);
	pathFree(pcb->message);
	return xkr;
    }
    
    evSchedule(pcb->event, mshipTimeout, pcb, MSHIP_RETRY_INTERVAL);
    semWait(&pcb->sem);
    
    xClose(lls);
    evCancel(pcb->event);
    evDetach(pcb->event);
    msgDestroy(pcb->message);
    pathFree(pcb->message);

    *hdr = pcb->rhdr;
    if (pcb->rhdr.type == MSHIP_ERROR) 
	return XK_FAILURE;

    return XK_SUCCESS;
}

void
mshipTimeout(
	    Event	ev,
	    void 	*arg)
{
    mship_pcb pcb = (mship_pcb)arg;
    Msg_s lcopy;

    if (pcb->retries++ > MSHIP_MAX_RETRIES) {
	pcb->rhdr.type = MSHIP_ERROR;
	semSignal(&pcb->sem);
	return;
    }

    xTrace0(mshipp, TR_FULL_TRACE, "Sending a request (timeout) \n");

    msgConstructCopy(&lcopy, pcb->message);
    (void)xPush(pcb->lls, &lcopy);
    msgDestroy(&lcopy);

    if ( evIsCancelled(ev) ) {
	xTrace0(mshipp, TR_EVENTS, "mshipTimeout cancelled");
	return;
    }
    evSchedule(ev, mshipTimeout, arg, MSHIP_RETRY_INTERVAL);
}

static xkern_return_t
mshipLookup(
	    XObj self,
	    MshipIface mif)
{
    PSTATE	*ps = (PSTATE *)self->state;
    Part_s	p[2];
    IPhost	ipBcast;
    MSHIP_HDR   hdr;
    xkern_return_t	xkr;
    unsigned int unused;
    /*
     * We must hit the wire and broadcast a request
     */
    ipBcast = IP_LOCAL_BCAST;
    partInit(&p[0], 2);
    partPush(*p, &unused, sizeof(unused));
    partPush(*p, &ipBcast, sizeof(IPhost));
    
    hdr.type = MSHIP_RESOLVE;
    hdr.whoami.node = ps->myNode;
    hdr.whoami.nap  = ps->myHost;
    hdr.mif  = *mif;    

    xkr = mshipSendWaitTimeout(self, &hdr, &p[0]);
    if (xkr != XK_SUCCESS)
	return XK_FAILURE;

    xAssert(hdr.type == MSHIP_RESOLVE_REPLY);
    xAssert(hdr.mif.node == mif->node);

    *mif = hdr.mif;
    return XK_SUCCESS;
}

static xkern_return_t
mshipReverseLookup(
		   XObj self,
		   MshipIface mif)
{
    PSTATE	*ps = (PSTATE *)self->state;
    Part_s	p[2];
    IPhost	ipAddr;
    MSHIP_HDR   hdr;
    xkern_return_t	xkr;
    unsigned int unused;

    /*
     * We must hit the wire and send a request to the IP address we have
     */
    ipAddr = mif->nap;
    partInit(&p[0], 2);
    partPush(*p, &unused, sizeof(unused));
    partPush(*p, &ipAddr, sizeof(IPhost));
    
    hdr.type = MSHIP_RRESOLVE;
    hdr.whoami.node = ps->myNode;
    hdr.whoami.nap  = ps->myHost;
    hdr.mif  = *mif;    

    xkr = mshipSendWaitTimeout(self, &hdr, &p[0]);
    if (xkr != XK_SUCCESS)
	return XK_FAILURE;

    xAssert(hdr.type == MSHIP_RRESOLVE_REPLY);
    xAssert(IP_EQUAL(hdr.mif.nap, mif->nap));

    *mif = hdr.mif;
    return XK_SUCCESS;
}

static int
mshipControlProtl(
		  XObj self, 
		  int  op, 
		  char *buf, 
		  int  len)
{
    PSTATE	*ps = (PSTATE *)self->state;
    MshipIface  mif = (MshipIface) buf;
    int         reply;
    xkern_return_t xkr;

    xAssert(xIsProtocol(self));
    switch (op) {
    case RESOLVE:
	{
	    /*
	     * It turns a node_name into a IPhost 
	     */
	    u_bit32_t in = mif->node;
	    IPhost    *out = &mif->nap;
	    boolean_t found = FALSE;

	    if (in < MSHIP_CACHE) {
		*out = ps->FastNodeName2Nap[in];
	        if (!IP_EQUAL(*out, ipNull))
		    found = TRUE;
	    } else {
		if (mapResolve(ps->NodeNameToNapMap, (void *) &in, (void **) out) == XK_SUCCESS)
		    found = TRUE;
	    }
	    if (found == TRUE || mshipLookup(self, mif) == XK_SUCCESS) {
		reply = 0;
	    } else {
		reply = -1;
	    }
	    break;
	}
    case RRESOLVE:
	{
	    /*
	     * It turns a IPhost into a node_name
	     */
	    IPhost    in = mif->nap;
	    u_bit32_t *out = &mif->node;
	    
	    xkr = mapResolve(ps->NapToNodeNameMap, (void *) &in, (void **) out);
	    if (xkr == XK_SUCCESS || mshipReverseLookup(self, mif) == XK_SUCCESS) {
		reply = 0;
	    } else {
		reply = -1;
	    }
	    break;
	}
    default:
	reply = -1;
    }

    return reply;
}

void
mshipAddEntry(
	      XObj self,
	      MshipIface mif)
{
    PSTATE	*ps = (PSTATE *)self->state;
    int internal;
    Bind bd;

    if (ps->verbose) {
	printf("Node %s joins cluster <%d> with node name %d\n", ipHostStr(&mif->nap), ps->myCluster, mif->node);
    }
    if (mif->node < MSHIP_CACHE) {
	xTrace2(mshipp, TR_FULL_TRACE, "adding node value (%d) to cache; previous value was %s\n", 
		mif->node, ipHostStr(&ps->FastNodeName2Nap[mif->node]));
	if (!IP_EQUAL(ps->FastNodeName2Nap[mif->node], ipNull) &&
	    !IP_EQUAL(ps->FastNodeName2Nap[mif->node], mif->nap) &&
	    ps->verbose) {
	    printf("WARNING: Node %s was previously known with node name %d\n", 
		   ipHostStr(&ps->FastNodeName2Nap[mif->node]), mif->node);
	}
	ps->FastNodeName2Nap[mif->node] = mif->nap;
    } else {
	xTrace1(mshipp, TR_FULL_TRACE, "adding node value (%d) outside cache\n", mif->node);
	if (mapResolve(ps->NapToNodeNameMap, (void *) &mif->nap, (void **) &internal) == XK_SUCCESS &&
	    ps->verbose) {
	    printf("WARNING: Node %s was previously known with node name %d\n",
		   ipHostStr(&mif->nap), internal);
	}
	internal = *(int *)&mif->nap;
	(void)mapBind(ps->NodeNameToNapMap, (void *) &mif->node, internal, &bd);
    }
    xTrace1(mshipp, TR_FULL_TRACE, "adding nap value (%s)\n", ipHostStr(&mif->nap));
    internal = (int)mif->node;
    (void)mapBind(ps->NapToNodeNameMap, (void *) &mif->nap, internal, &bd);
}

static xkern_return_t
mshipDemux(
	   XObj self,
	   XObj lls,
	   Msg  message)
{
    MSHIP_HDR hdr;
    PSTATE	*ps = (PSTATE *)self->state;
    mship_pcb pcb;
    xkern_return_t xkr;
    
    xAssert(msgLen(message) >= sizeof(MSHIP_HDR));
    
    xkr = msgPop(message, mship_pophdr, (void *)&hdr, sizeof(hdr), NULL);
    xAssert(xkr == XK_SUCCESS);

    xTrace0(mshipp, TR_DETAILED, "mship demux");

    if (hdr.version != MSHIP_VERSION || hdr.cluster != ps->myCluster) {
	/*
	 * Both Requests and Replies cannot span a cluster.
	 */
	return XK_SUCCESS;
    }

    pcb = hdr.pcb;
    switch (hdr.type) {
    case MSHIP_RESOLVE_REPLY:
    case MSHIP_RRESOLVE_REPLY:
	{
	    xTrace0(mshipp, TR_FULL_TRACE, "Received reply. Adding a new entry\n");
	    mshipAddEntry(self, &hdr.mif);
	    pcb->rhdr = hdr;
	    semSignal(&pcb->sem);
	}
	break;
    case MSHIP_RESOLVE:
	{
	    if (hdr.whoami.node == ps->myNode) 
		break;
	    xTrace0(mshipp, TR_FULL_TRACE, "Received node->nap request. Adding a new entry\n");
	    mshipAddEntry(self, &hdr.whoami);
	    if (hdr.mif.node == ps->myNode) {
		/*
		 * This is on us!
		 */
		Msg_s reply;
		MSHIP_HDR rhdr;
		
		xTrace0(mshipp, TR_FULL_TRACE, "It's us. We ought to reply\n");
		if (msgConstructContig(&reply, self->path, 0, MSHIP_STACK_SIZE) != XK_SUCCESS) {
		    xTrace0(mshipp, TR_ERRORS, "MSHIP Unable to create request message\n");
		}
		
		rhdr.version = MSHIP_VERSION;
		rhdr.type = MSHIP_RESOLVE_REPLY;
		rhdr.cluster = ps->myCluster;
		rhdr.pcb = hdr.pcb;
		rhdr.mif.node = ps->myNode;
		rhdr.mif.nap = ps->myHost;
		
		xkr = msgPush(&reply, mship_fillinfunc, &rhdr, sizeof (rhdr), (void *)0);
		xAssert(xkr == XK_SUCCESS);
		
		(void)xPush(lls, &reply);
		msgDestroy(&reply);
	    }
	}
	break;
    case MSHIP_RRESOLVE:
	{
	    if (hdr.whoami.node == ps->myNode) 
		break;
	    xTrace0(mshipp, TR_FULL_TRACE, "Received nap->node request. Adding a new entry\n");
	    mshipAddEntry(self, &hdr.whoami);
	    if (IP_EQUAL(hdr.mif.nap, ps->myHost)) {
		/*
		 * This is on us!
		 */
		Msg_s reply;
		MSHIP_HDR rhdr;
		
		xTrace0(mshipp, TR_FULL_TRACE, "It's us. We ought to reply\n");
		if (msgConstructContig(&reply, self->path, 0, MSHIP_STACK_SIZE) != XK_SUCCESS) {
		    xTrace0(mshipp, TR_ERRORS, "MSHIP Unable to create request message\n");
		}
		
		rhdr.version = MSHIP_VERSION;
		rhdr.type = MSHIP_RRESOLVE_REPLY;
		rhdr.cluster = ps->myCluster;
		rhdr.pcb = hdr.pcb;
		rhdr.mif.node = ps->myNode;
		rhdr.mif.nap = ps->myHost;
		
		xkr = msgPush(&reply, mship_fillinfunc, &rhdr, sizeof (rhdr), (void *)0);
		xAssert(xkr == XK_SUCCESS);
		
		(void)xPush(lls, &reply);
		msgDestroy(&reply);
	    }
	}
	break;
    default:
	xAssert(0);
	break;
    }
    return XK_SUCCESS;
}

xkern_return_t
mship_init(
	   XObj self)
{
    XObj	llp;
    Part_s	p;
    PSTATE      *ps;
    int         i;

    xTrace0(mshipp, TR_FULL_TRACE, "init called");

    llp = xGetDown(self, 0);
    if (!xIsProtocol(llp)) {
	xTrace0(mshipp, TR_ERRORS, "mship has no lower protocol");
	return XK_FAILURE;
    }

    if (!(self->state = ps = pathAlloc(self->path, sizeof(PSTATE)))) {
	xTrace0(mshipp, TR_ERRORS, "allocation errors in init");
	return XK_FAILURE;
    }
    
    /* ext: node_name; int: nap */
    ps->NodeNameToNapMap = mapCreate(MAX_RESOLUTION_ENTRIES, sizeof(u_bit32_t),
				     self->path);
    
    /* ext: nap;       int: node_name */
    ps->NapToNodeNameMap = mapCreate(MAX_RESOLUTION_ENTRIES, sizeof(IPhost),
				     self->path);
    
    if (!ps->NodeNameToNapMap || !ps->NapToNodeNameMap) {
	return XK_NO_MEMORY;
    }

    for (i = 0; i < MSHIP_CACHE; i++) {
	ps->FastNodeName2Nap[i] = ipNull;
    }
    
    self->control = mshipControlProtl;
    self->demux = mshipDemux;

    partInit(&p, 1);
    partPush(p, ANY_HOST, 0);
    if (xOpenEnable(self, self, llp, &p) == XK_FAILURE) {
	xTrace0(mshipp, TR_ERRORS, "xopenenable errors in init");
	return XK_FAILURE;
    } 
    xTrace0(mshipp, TR_FULL_TRACE, "mship xopenenable(%d) OK");

    if (xControl(xGetDown(self, 0), GETMYHOST, (char *)&ps->myHost, sizeof(IPhost)) 
	                                         < (int)sizeof(IPhost))	{
	xTrace0(mshipp, TR_ERRORS, "could not determine local IP address");
	return XK_FAILURE;
    }

    if ( xControl(xGetDown(self, 1), BIDCTL_GET_CLUSTERID,
                  (char *)&ps->myCluster, sizeof(ClusterId)) < (int)sizeof(ClusterId) ) {
        xTrace0(mshipp, TR_ERRORS, "mship could not get any clusterid -- aborting init");
        return XK_FAILURE;
    }
    ps->verbose = FALSE;
    findXObjRomOpts(self, mshipOpts, sizeof(mshipOpts)/sizeof(XObjRomOpt), 0);

    ps->myNode = NODE_INVALID;
    if (mapResolve(ps->NapToNodeNameMap, (void *) &ps->myHost, (void **) &ps->myNode) != XK_SUCCESS) {
	xTrace0(mshipp, TR_ERRORS, "could not determine local node name");
	return XK_FAILURE;
    }	
    if (ps->myNode == NODE_INVALID) {
	xTrace0(mshipp, TR_ERRORS, "local node name isn't valid");
	return XK_FAILURE;
    }

    return XK_SUCCESS;
}

void
mship_fillinfunc( 
		 void *hdr, 
		 char *des, 
		 long len, 
		 void *arg)
{
    /*
     * Used by the msgPush() utility.
     */
    bcopy(hdr, des, len);
}

long
mship_pophdr(
	     void *hdr,
	     char *netHdr,
	     long int len,
	     void *arg)
{
    bcopy(netHdr, hdr, len);
    return len;
}


