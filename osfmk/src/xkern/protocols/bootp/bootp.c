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
 * This implementation of the BOOTP protocol aims to obsolete any need
 * for hardwiring addresses of any sort into executables. BOOTP  is
 * deemed to be a more powerful choice than RARP. The presence of BOOTP
 * does not introduce any requirement on the shape of the protocol graph:
 * BOOTP mimics a trivial UDP/IP stack on its own.
 *
 * The BOOTP protocol is supposed to run on top of the eth protocol.
 * Most of its business is done in the bootp_init routine. Should the
 * handshake with a BOOTPD fail, bootp_init fails (XK_FAILURE).
 * 
 * Using the features of basic BOOTP and several other RFCs that came
 * along at different times, this protocol is able to retrieve the 
 * following data:
 *    	- the IP address assigned to the eth interface
 *      - an IP gateway
 *      - the IP address of a time server
 *      - a 32 bits identifier (node number within a computing domain, T200).
 *      - a string (the computing domain identifier, T201).
 *      - a 32 bits identifier (the computing domain id, T202).
 * 
 * The corresponding bootpd protocol must be 2.4.0 or later (rfc1533).
 *
 * Today, the bootp protocol interfaces with the other x-kernel protocols
 * in two ways:
 * 	1. With a set of xControls.
 *      2. Through the rom database. The bootp protocol fills a set of
 *         rom entries, reserved beyond ROM_MAX_LINES. This is not pretty.
 *	   However, it gives us some time to transition to the new interface
 * 	   while we continue to support existing protocols unchanged.
 *
 * ToDo: 
 *      1. The xControl interface
 *
 * Author: Franco Travostino at Open Software Foundation.
 */
#include <xkern/include/xkernel.h>
#include <xkern/include/part.h>
#include <xkern/include/prot/eth.h>
#include <xkern/include/prot/ip_host.h>

#include <xkern/protocols/bootp/bootp.h>
#include <xkern/protocols/bootp/bootp_internal.h>
#include <xkern/include/prot/bootp.h>

XObj thisprotocol;
u_bit32_t bootp_xid = 0xbeefface;
u_bit16_t ip_id;

IPhost IPSOURCE = {0x0, 0x0, 0x0, 0x0};
IPhost IPDESTINATION = {0xff, 0xff, 0xff, 0xff};
static ETHhost	ethBcastHost = BCAST_ETH_AD;

#define DEFAULT_ATTEMPTS 20
#define DEFAULT_MIN_INTERVAL (100 * 1000)         /* 100 msec */                
#define DEFAULT_MAX_INTERVAL (30 * 1000 * 1000)   /* 30  sec  */

#define MIN(a,b) (a < b ? a : b)

#if XK_DEBUG
int tracebootpp;
#endif /* XK_DEBUG */

/*
 * Hacked up versions of UDP and IP headers, to support BOOTP
 */
struct udp {
    u_bit16_t     sport;  /* source port */
    u_bit16_t     dport;  /* destination port */
    u_bit16_t     ulen;   /* udp length */
    u_bit16_t     sum;    /* udp checksum */
};

struct ip {
    u_bit8_t      vers_hlen;      /* high 4 bits are version, low 4 are hlen */
    u_bit8_t      type;
    u_bit16_t     dlen;
    u_bit16_t     ident;
    u_bit16_t     frag;
    u_bit8_t      time;
    u_bit8_t      prot;
    u_bit16_t     checksum;
    IPhost        source;         /* source address */
    IPhost        dest;           /* destination address */
};

struct ps_ip {
    IPhost	  src;		/* Source address */
    IPhost	  dst;		/* Destination address */
    u_bit8_t	  zero;	        /* Zero padding */
    u_bit8_t	  protocol;	/* Next level protocol */
    u_bit16_t	  len;		/* Next level length */
};


/* forward */

xkern_return_t
bootp_demux(
	    XObj 	self,
	    XObj	lls,
	    Msg 	msg);

xkern_return_t
bootp_opendone(
	       XObj 	self,
	       XObj	lls,
	       XObj	llp,
	       XObj	hlpType,
	       Path	path);

void
bootpTimeout(
	     Event  ev,
	     void   *arg);

void
bootp_fillinfunc( 
		 void *hdr, 
		 char *des, 
		 long len, 
		 void *arg);

static xkern_return_t
readAttempts(
	     XObj self, 
	     char **str, 
	     int nFields, 
	     int line, 
	     void *arg);

static xkern_return_t
readMinInterval(
		XObj self, 
		char **str, 
		int nFields, 
		int line, 
		void *arg);

static xkern_return_t
readMaxInterval(
		XObj self, 
		char **str, 
		int nFields, 
		int line, 
		void *arg);

static int
bootp_control(
	      XObj	self,
	      int	op,
	      char 	*buf,
	      int	len);

static XObjRomOpt	bootpOpts[] = {
    { "attempts", 3, readAttempts },
    { "min_interval", 3, readMinInterval },
    { "max_interval", 3, readMaxInterval },
};


static xkern_return_t
readAttempts(
	     XObj self, 
	     char **str, 
	     int nFields, 
	     int line, 
	     void *arg)
{
    PSTATE *ps = (PSTATE *)self->state;

    ps->max_attempts = atoi(str[2]);
    return XK_SUCCESS;
}

static xkern_return_t
readMinInterval(
	     XObj self, 
	     char **str, 
	     int nFields, 
	     int line, 
	     void *arg)
{
    PSTATE *ps = (PSTATE *)self->state;

    ps->min_wait = atoi(str[2]);
    return XK_SUCCESS;
}

static xkern_return_t
readMaxInterval(
	     XObj self, 
	     char **str, 
	     int nFields, 
	     int line, 
	     void *arg)
{
    PSTATE *ps = (PSTATE *)self->state;

    ps->max_wait = atoi(str[2]);
    return XK_SUCCESS;
}
  
void
bootp_fillinfunc( 
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

static long
bootp_pophdr(
	     void *hdr,
	     char *netHdr,
	     long int len,
	     void *arg)
{
    bcopy(netHdr, hdr, len);
    return len;
}

static xkern_return_t
ps2rom(
       XObj self)
{
    char buf[ROM_OPT_MAX_LEN];
    PSTATE *ps = (PSTATE *)self->state;

    /*
     * My own ARP entry
     */
    sprintf(buf, "arp %s %s lock",
	    ipHostStr(&ps->ipaddr), ethHostStr(&ps->ethaddr));
    if ( addRomOpt(buf) != XK_SUCCESS ) return XK_FAILURE;
    /*
     * My own IP gateway entry
     */
    sprintf(buf, "ip gateway %s", ipHostStr(&ps->gwipaddr));
    if ( addRomOpt(buf) != XK_SUCCESS ) return XK_FAILURE;
    /*
     * Cluster id entry
     */
    sprintf(buf, "bidctl clusterid %d", ps->clusterid);
    if ( addRomOpt(buf) != XK_SUCCESS ) return XK_FAILURE;
    /*
     * node_name entry
     */
    sprintf(buf, "mship node_name %s %d", ipHostStr(&ps->ipaddr), ps->nodename);
    if ( addRomOpt(buf) != XK_SUCCESS ) return XK_FAILURE;

    /*
     * time server entry
     */
    sprintf(buf, "time_client config %s", ipHostStr(&ps->tsipaddr));
    if ( addRomOpt(buf) != XK_SUCCESS ) return XK_FAILURE;

    /*
     * multicast address entry
     */
    sprintf(buf, "sequencer mcast_address %s", ipHostStr(&ps->mcipaddr));
    if ( addRomOpt(buf) != XK_SUCCESS ) return XK_FAILURE;

    return XK_SUCCESS;
}

static int
bootp_control(
	      XObj	self,
	      int	op,
	      char 	*buf,
	      int	len)
{
    PSTATE *ps = (PSTATE *)self->state;

    switch (op) {
    case BOOTP_GET_CLUSTERID:
	checkLen(len, sizeof(u_bit32_t));
	*(u_bit32_t *)buf = ps->clusterid;
	return sizeof(u_bit32_t);

    case BOOTP_GET_IPADDR:
	checkLen(len, sizeof(IPhost));
	*(IPhost *)buf = ps->ipaddr;
	return sizeof(IPhost);

    case BOOTP_GET_GWIPADDR:
	checkLen(len, sizeof(IPhost));
	*(IPhost *)buf = ps->gwipaddr;
	return sizeof(IPhost);

    case BOOTP_GET_NODENAME:
	checkLen(len, sizeof(u_bit32_t));
	*(u_bit32_t *)buf = ps->nodename;
	return sizeof(u_bit32_t);

    case BOOTP_GET_TIMEMASTER:
	checkLen(len, sizeof(IPhost));
	*(IPhost *)buf = ps->tsipaddr;
	return sizeof(IPhost);

    case BOOTP_GET_MCADDR:
	checkLen(len, sizeof(IPhost));
	*(IPhost *)buf = ps->mcipaddr;
	return sizeof(IPhost);

    default:
	return xControl(xGetDown(self, 0), op, buf, len);	
    }
}

static xkern_return_t
bootpComposeRequest(
		    XObj self)
{
    PSTATE *ps = (PSTATE *)self->state;
    xkern_return_t xkr;
    struct bootp  *bootp_header;
    struct udp    *udp_header;
    struct ip     *ip_header;
    struct ps_ip  *pseudo_header;
    char *data;
    int i;
    u_bit8_t      vendor_tag[4] = OUR_VENDOR_TAG;
    
    xkr =  msgConstructContig(&ps->request, self->path, 0, BOOTP_STACK_SIZE); 
    if (xkr != XK_SUCCESS)
	return xkr;

    data = (char *)pathAlloc(self->path, BOOTP_STACK_SIZE);
    if (!data)  
	return XK_NO_MEMORY;

    ip_header = (struct ip *)data;
    pseudo_header = (struct ps_ip *)(data + sizeof(struct ip) - sizeof(struct ps_ip));
    udp_header = (struct udp *)(data + sizeof(struct ip));
    bootp_header = (struct bootp *)(data + sizeof(struct ip) + sizeof (struct udp));
    
    bootp_header->bp_op = BOOTREQUEST;
    bootp_header->bp_htype = 1;
    bootp_header->bp_hlen = 6;
    bootp_header->bp_hops = 0;
    bootp_header->bp_xid = htonl(bootp_xid);
    bootp_header->bp_secs = 0;
    bootp_header->bp_unused = 0;
    bootp_header->bp_ciaddr = IPSOURCE;
    bootp_header->bp_yiaddr = IPSOURCE;
    bootp_header->bp_siaddr = IPSOURCE;
    bootp_header->bp_giaddr = IPSOURCE;
    memcpy((char *) &bootp_header->bp_chaddr[0], (const char *)&ps->ethaddr, sizeof(ETHhost));
    for (i = 0; i < 64; i++)
	bootp_header->bp_sname[i] = '\0';
    for (i = 0; i < 128; i++)
	bootp_header->bp_file[i] = '\0';
    memcpy((char *)&bootp_header->bp_vend[0], (const char *)&vendor_tag[0], sizeof vendor_tag);

    pseudo_header->src = IPSOURCE;
    pseudo_header->dst = IPDESTINATION;
    pseudo_header->zero = 0;
    pseudo_header->protocol = 17;
    pseudo_header->len = htons(sizeof(struct bootp) + sizeof (struct udp));

    udp_header->sport = htons(IPPORT_BOOTPC);
    udp_header->dport = htons(IPPORT_BOOTPS);
    udp_header->ulen = htons(sizeof(struct bootp) + sizeof (struct udp));
    udp_header->sum = 0; 
    udp_header->sum = ~ocsum((u_short *)pseudo_header, 
			     (sizeof (struct ps_ip) + sizeof (struct udp) + sizeof(struct bootp)) / 2);

    if (!udp_header->sum)
	udp_header->sum = 0xffff;

    ip_header->vers_hlen = (4 << 4);
    ip_header->vers_hlen |= 5;
    ip_header->type = 0;
    ip_header->dlen = htons(sizeof (struct bootp) + sizeof (struct udp) + sizeof (struct ip));
    ip_header->ident  = htons(ip_id++);
    ip_header->frag = 0;
    ip_header->time = 0xFF;
    ip_header->prot = 17;
    ip_header->source = IPSOURCE;
    ip_header->dest = IPDESTINATION;
    ip_header->checksum = 0;
    ip_header->checksum = ~ocsum((u_short *)ip_header, sizeof(struct ip) / 2);

    xkr = msgPush(&ps->request, bootp_fillinfunc, data,
		  sizeof (struct bootp) + sizeof (struct udp) + sizeof (struct ip),
		  (void *) 0);
    pathFree(data);
    return xkr;
}

xkern_return_t
bootp_demux(
	    XObj 	self,
	    XObj	lls,
	    Msg 	msg)
{
    PSTATE *ps = (PSTATE *)self->state;
    xkern_return_t xkr;
    struct ip     ip_header;
    struct ps_ip  *pseudo_header;
    struct udp    *udp_header;
    struct bootp  *bootp_header;
    u_bit16_t     sum, len;
    char          *data;
    u_bit8_t      *dp, vendor_tag[4] = OUR_VENDOR_TAG;
    static boolean_t gotit;

    if (gotit)
	return XK_SUCCESS;

    xTrace1(bootpp, TR_DETAILED, "bootp receives %d bytes", msgLen(msg));

    if (msgPop(msg, bootp_pophdr, (void *)&ip_header, sizeof(struct ip), NULL) != XK_SUCCESS) {
	xTrace0(bootpp, TR_ERRORS, "bootp msgPop of IP header fails on reply");
	return XK_SUCCESS;
    }
    if ((~ocsum((u_short *)&ip_header, sizeof(struct ip) / 2)) & 0xffff) {
	xTrace0(bootpp, TR_ERRORS, "bootp IP checksum error");
	return XK_SUCCESS;
    }
    len = ntohs(ip_header.dlen);
    if (len < (sizeof (struct bootp) + sizeof (struct udp) + sizeof (struct ip))) {
	xTrace0(bootpp, TR_DETAILED, "bootp IP fragment isn't big enough");
	return XK_SUCCESS;
    }
    len -= sizeof (struct ip);
    if (len > msgLen(msg)) {
	xTrace0(bootpp, TR_ERRORS, "bootp message isn't big enough");
	return XK_SUCCESS;
    }
    if (ip_header.prot != 17) {
	xTrace0(bootpp, TR_DETAILED, "bootp IP: up protocol number doesn't match");
	return XK_SUCCESS;
    }
    
    data = (char *)pathAlloc(self->path, len + sizeof(struct ps_ip));
    if (!data)  
	return XK_SUCCESS;
    msg2buf(msg, data + sizeof(struct ps_ip));

    pseudo_header = (struct ps_ip *)data;
    pseudo_header->src = ip_header.source;
    pseudo_header->dst = ip_header.dest;
    pseudo_header->zero = 0;
    pseudo_header->protocol = 17;
    pseudo_header->len = htons(len);

    udp_header = (struct udp *)(data + sizeof(struct ps_ip));

    if (udp_header->sum) {
	if ((~ocsum((u_short *)pseudo_header, (len + sizeof(struct ps_ip)) / 2)) & 0xffff) {
	    xTrace0(bootpp, TR_ERRORS, "bootp UDP checksum failure");
	    pathFree(data);
	    return XK_SUCCESS;
	}
    }

    if (udp_header->sport != htons(IPPORT_BOOTPS) ||
	udp_header->dport != htons(IPPORT_BOOTPC)) {
	xTrace0(bootpp, TR_DETAILED, "bootp UDP port numbers do not match");
	pathFree(data);
	return XK_SUCCESS;
    }
    if (len != ntohs(udp_header->ulen)) {
	xTrace0(bootpp, TR_DETAILED, "bootp UDP length does not match");
	pathFree(data);
	return XK_SUCCESS;
    }
    len -= sizeof(struct udp);
    
    bootp_header = (struct bootp *)(data + sizeof(struct ps_ip) + sizeof(struct udp));
    if (bootp_header->bp_op != BOOTREPLY)  {
 	xTrace0(bootpp, TR_DETAILED, "bootp header does not seem to be a reply");
	pathFree(data);
	return XK_SUCCESS;
    }
    if (strncmp((const char *)bootp_header->bp_chaddr,
		(const char *)&ps->ethaddr,
		sizeof(ETHhost))) {
 	xTrace0(bootpp, TR_ERRORS, "bootp header carries a different ETHhost");
	pathFree(data);
	return XK_SUCCESS;
    }

    dp = &bootp_header->bp_vend[0];
    if (strncmp((const char *)dp,
		(const char *)vendor_tag, 
		sizeof(vendor_tag))) {
 	xTrace0(bootpp, TR_ERRORS, "bootp header does not carry a rfc1048 style reply");
	pathFree(data);
	return XK_SUCCESS;
    }
		
    xTrace0(bootpp, TR_DETAILED, "a valid bootp reply has been received");

    ps->ipaddr = bootp_header->bp_yiaddr;
    xTrace1(bootpp, TR_DETAILED, "my address %s", ipHostStr(&bootp_header->bp_yiaddr));

    dp += sizeof(vendor_tag);
    while (*dp != TAG_END) {
	switch (*dp++) {
	case TAG_GATEWAY:
	    memcpy((char *)&ps->gwipaddr, (const char *)(dp + 1), sizeof(IPhost));
	    xTrace1(bootpp, TR_DETAILED, "Gateway address %s", ipHostStr(&ps->gwipaddr));
	    break;
	case TAG_TIME_SERVER:
	    memcpy((char *)&ps->tsipaddr, (const char *)(dp + 1), sizeof(IPhost));
	    xTrace1(bootpp, TR_DETAILED, "Time server address %s", ipHostStr(&ps->tsipaddr));
	    break;
	case TAG_HOST_NAME:
	    len = MIN(*dp, MAXSTRING);
	    memcpy((char *)ps->hostname, (const char *)(dp + 1), len);
	    ps->hostname[len+1] = 0;
	    xTrace1(bootpp, TR_DETAILED, "Host name is %s", ps->hostname);
	    break;
	    /*
	     * non-standard tags:
	     */
	case 200:
	    ps->nodename = (u_bit32_t)atoi((const char *)(dp + 1));
	    xTrace1(bootpp, TR_DETAILED, "node name is %d", ps->nodename);
	    break;
	case 201:
	    len = MIN(*dp, MAXSTRING);
	    memcpy((char *)ps->clustername, (const char *)(dp + 1), len);
	    ps->clustername[len+1] = 0;
	    xTrace1(bootpp, TR_DETAILED, "Cluster name is %s", ps->clustername);
	    break;
	case 202:
	    ps->clusterid = (u_bit32_t)atoi((const char *)(dp + 1));
	    xTrace1(bootpp, TR_DETAILED, "Cluster id is %d", ps->clusterid);
	    break;
	case 203:
	    if (str2ipHost(&ps->mcipaddr, (char *)(dp + 1)) != XK_SUCCESS) {
		ps->mcipaddr = IPSOURCE;
	    } else {
		xTrace1(bootpp, TR_DETAILED, "Cluster multicast address %s", ipHostStr(&ps->mcipaddr));
	    }
	    break;
	default:
	    xTrace1(bootpp, TR_DETAILED, "Tag is %d", *(dp-1));
	}
	len = *dp++;
	dp += len;
    }

    /*
     * Among non-standard tags, we must have recv'd at least a
     * nodename and a clusterid!
     */
    if (ps->nodename == BOOTP_INVALID_ID || 
	ps->clusterid == BOOTP_INVALID_ID) {
	 xTrace2(bootpp, TR_ERRORS, "Nodename (%x) and/or Cluster (%x) id aren't specified", 
		 ps->nodename, ps->clusterid);
	 pathFree(data);
	 /*
	  * Next
	  */
	 return XK_SUCCESS;
     }

    semSignal(&ps->sem);
    ps->reply_value = XK_SUCCESS;

    gotit = TRUE;

    printf("\nReceived BOOTP reply from %s\n", ipHostStr(&ip_header.source));

    pathFree(data);
    return XK_SUCCESS;
}

void
bootpTimeout(
	     Event  ev,
	     void   *arg)
{
    XObj self  = (XObj)arg;
    PSTATE *ps = self->state;
    boolean_t  must_wakeup = FALSE;
    
    xTrace0(bootpp, TR_FULL_TRACE, "bootptimeout called\n");

    printf(".");

    if (ps->max_attempts) 
	if (xPush(ps->lls, &ps->request) < 0) {
	    xTrace1(bootpp, TR_ERRORS, "%d xPush fails\n", ps->max_attempts);
	    must_wakeup = TRUE;
	}
	 
    if (!--ps->max_attempts || must_wakeup) {
	ps->reply_value = XK_FAILURE;
	semSignal(&ps->sem);
    } else {
	ps->wait = MIN(2 * ps->wait, ps->max_wait);
	evSchedule(ev, bootpTimeout, self, ps->wait);
	if (ps->wait == ps->max_wait)
	    ps->wait = ps->min_wait;
    }
}

xkern_return_t
bootp_opendone(
		XObj 	self,
		XObj		lls,
		XObj		llp,
		XObj		hlpType,
		Path	        path)
{
   xTrace0(bootpp, TR_FULL_TRACE, "bootp_opendone");
   return XK_SUCCESS;
}


xkern_return_t
bootp_init(
	   XObj self)
{
    Part_s part;
    XObj llp;
    PSTATE *ps;
    xkern_return_t ret = XK_SUCCESS;
    Path path = self->path;

    llp = xGetDown(self, 0);
    self->demux = bootp_demux;
    self->opendone = bootp_opendone;
    self->control = bootp_control;
    self->hlpType = self;
    
 try_again:
    partInit(&part, 1);
    partPush(part, ANY_HOST, 0);
    if (xOpenEnable(self, self, llp, &part)) 
	return XK_FAILURE;

    if ( ! (self->state = ps = pathAlloc(path, sizeof(PSTATE))) ) {
	xTrace0(bootpp, TR_ERRORS, "allocation errors in init");
	return XK_FAILURE;
    }
    bzero((char *)ps, sizeof(PSTATE));
    if (xControl(xGetDown(self, 0), GETMYHOST,  
		 (char *) &ps->ethaddr, sizeof(ETHhost)) < sizeof(ETHhost)) {
	pathFree(ps);
	return XK_FAILURE;
    }
    if ( ! (ps->event = evAlloc(path))) {
	pathFree(ps);
	return XK_FAILURE;
    }
    ps->max_attempts = DEFAULT_ATTEMPTS;
    ps->min_wait = DEFAULT_MIN_INTERVAL;
    ps->max_wait = DEFAULT_MAX_INTERVAL;
    ps->nodename = BOOTP_INVALID_ID;
    ps->clusterid = BOOTP_INVALID_ID;

    if (findXObjRomOpts(self, bootpOpts,
		sizeof(bootpOpts)/sizeof(XObjRomOpt), ps) == XK_FAILURE ) {
	xTrace0(bootpp, TR_ERRORS, "bootp: romfile config errors, not running");
	pathFree(ps);
	return XK_FAILURE;
    }
    ps->wait = ps->min_wait;
    xAssert(ps->max_attempts);
    xAssert(ps->wait);
    /*
     * Open a session
     */
    partInit(&part, 1);
    partPush(part, &ethBcastHost, sizeof(ethBcastHost));
    if ((ps->lls = xOpen(self, self, llp, &part, xkDefaultPath)) == ERR_XOBJ) {
	xTrace0(bootpp, TR_ERRORS, "bootp: client cannot open");
	pathFree(ps);
	return XK_FAILURE;
    }

    /*
     * Compose a bootp request.
     */
    if (ret = bootpComposeRequest(self)) /* = rather than == */
	goto shutdown;

    /*
     * Send it.
     */
    semInit(&ps->sem, 0);

    printf("Try to reach a BOOTP server: .");

    ps->max_attempts--;
    if (xPush(ps->lls, &ps->request) < 0) {
	xTrace0(bootpp, TR_ERRORS, "bootp: can't send message");
	ret = XK_FAILURE;
	goto shutdown;
    }
    evSchedule(ps->event, bootpTimeout, self, ps->wait);
    semWait(&ps->sem);

    printf("\n");

    xTrace1(bootpp, TR_MORE_EVENTS, 
	    "bootp request released from block, with value %d", ps->reply_value);

    if (ps->reply_value) {
	/*
	 * Something went wrong
	 */
	ret = XK_FAILURE;
	goto shutdown;
    } else {
	if ( ps2rom(self) != XK_SUCCESS ) {
	    xTraceP0(self, TR_ERRORS, "Error adding rom options");
	    ret = XK_FAILURE;
	    goto shutdown;
	}
    }

    /*
     * Unplug ourselves!
     */
    if (xOpenDisable(self, self, llp, &part)) 
	ret = XK_FAILURE;

 shutdown:
    evCancel(ps->event);
    evDetach(ps->event);
    msgDestroy(&ps->request);
    xClose(ps->lls);

    if (ret != XK_SUCCESS) {
	printf("************************************************\n");
	printf("No valid reply from a bootpd server was received\n");
	printf("Please contact your system administrator\n");
	printf("************************************************\n");
	goto try_again;
    }
    /*
     * Note: if ret is not XK_SUCCESS, ps will 
     * be destroied by the xDestroyXObj() utility
     */
    return ret;
}



