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
 * ip_i.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * ip_i.h,v
 * Revision 1.38.1.3.1.2  1994/09/07  04:18:30  menze
 * OSF modifications
 *
 * Revision 1.38.1.3  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 * Revision 1.38.1.2  1994/07/22  20:08:14  menze
 * Uses Path-based message interface and UPI
 *
 * Revision 1.38.1.1  1994/04/23  00:43:38  menze
 * Added allocator call-backs
 *
 * Revision 1.38  1994/01/27  17:07:01  menze
 *   [ 1994/01/13          menze ]
 *   Now uses library routines for rom options
 *
 */

#include <xkern/protocols/ip/route.h>

#ifndef ip_i_h
#define ip_i_h

#include <xkern/include/prot/ip.h>
#include <xkern/include/prot/vnet.h>

/* 
 * Length in bytes of the standard IP header (no options)
 */
#define	IPHLEN	20

#define IPVERS	(4 << 4)
#define VERS_MASK 0xf0
#define HLEN_MASK 0x0f
#define GET_HLEN(h) ((h)->vers_hlen & HLEN_MASK)
#define GET_VERS(h) ( ((h)->vers_hlen & VERS_MASK) >> 4 )

#define IPMAXINTERFACES  10

/* 
 * Default MTU.  This will normally be overridden by querying lower
 * protocols. 
 */
#define IPOPTPACKET 512
/* 
 * MAXPACKET -- how many octets of user data will fit in an IP datagram.  
 * Maximum length field - maximum IP header.
 */
#define IPMAXPACKET (0xffff - (15 << 2))
#define IPDEFAULTDGTTL  30
#define IP_GC_INTERVAL 30 * 1000 * 1000		/* 30 seconds */

/* fragment stuff */
#define DONTFRAGMENT  0x4000
#define MOREFRAGMENTS  0x2000
#define FRAGOFFMASK   0x1fff
#define FRAGOFFSET(fragflag)  ((fragflag) & FRAGOFFMASK)
#define COMPLETEPACKET(hdr) (!((hdr).frag & (FRAGOFFMASK | MOREFRAGMENTS)))
#define INFINITE_OFFSET      0xffff

#define IP_ACTIVE_MAP_SZ	101
#define IP_FORWARD_MAP_SZ	101
#define IP_PASSIVE_MAP_SZ	23
#define IP_PASSIVE_SPEC_MAP_SZ	23
#define IP_FRAG_MAP_SZ		23

typedef struct ipheader {
  	u_char 	vers_hlen;	/* high 4 bits are version, low 4 are hlen */
	u_char  type;
	u_short	dlen;
	u_short ident;
	u_short frag;
	u_char  time;
	u_char  prot;
	u_short checksum;
  	IPhost	source;		/* source address */
  	IPhost	dest;		/* destination address */
} IPheader; 



typedef struct pstate {
    XObj	self;
    Map 	activeMap;
    Map		fwdMap;
    Map  	passiveMap;
    Map  	passiveSpecMap;
    Map		fragMap;
    int		numIfc;
    RouteTable	rtTbl;
} PState;

typedef struct sstate {
    IPheader	hdr;
    int		mtu;	/* maximum transmission unit on intface */
    Event	ev;
} SState;

/*
 * The active map is keyed on the local and remote hosts rather than
 * the lls because the lls may change due to routing while the hosts
 * in the IP header will not.
 */
typedef struct {
    long	protNum;
    IPhost	remote;	/* remote host  */
    IPhost	local;	/* local host	*/
}	ActiveId;

typedef IPhost	FwdId;

typedef long	PassiveId;

typedef struct {
    long	prot;
    IPhost	host;
} PassiveSpecId;


/*
 * fragmentation structures
 */

typedef struct {
    IPhost source, dest;
    u_char prot;
    u_char pad;
    u_short seqid;
} FragId;


typedef struct hole {
    u_int	first, last;
} Hole;

#define RHOLE  1
#define RFRAG  2

typedef struct fragif {
    u_char type;
    union {
	Hole	hole;
	Msg_s	frag;
    } u;
    struct fragif *next, *prev;
} FragInfo;


/* 
 * FragId's map into this structure, a list of fragments/holes 
 */
typedef struct FragList {
    u_short  	nholes;
    FragInfo	head;	/* dummy header node */	
    Bind     	binding;
    boolean_t	gcMark;
} FragList;

#define ERR_FRAG ((Fragtable *)-1)


void		ipAllocCallBack( Allocator, void * );
int		ipControlProtl( XObj, int, char *, int );
int		ipControlSessn( XObj, int, char *, int );
XObj		ipCreatePassiveSessn( XObj, XObj, ActiveId *, FwdId *, Path );
xkern_return_t	ipDemux( XObj, XObj, Msg );
void 		ipDumpHdr( IPheader * );
void 		ipDumpHdr( IPheader * );
Enable *	ipFindEnable( XObj, int, IPhost * );
xkern_return_t 	ipForwardPop( XObj, XObj, Msg, void * );
void		ipFreeFragList( FragList * );
xkern_return_t 	ipFwdBcastPop( XObj, XObj, Msg, void * );
int		ipGetHdr( Msg, IPheader *, char * );
void		ipHdrStore( void *, char *, long, void * );
int		ipHostOnLocalNet( PState *, IPhost *);
int		ipIsMyAddr( XObj, IPhost * );
xkern_return_t	ipMsgComplete( XObj, XObj, Msg, void * );
void		ipProcessRomFile( XObj );
xkern_return_t	ipReassemble( XObj, XObj, Msg, IPheader * );
int		ipRemoteNet( PState *, IPhost *, route *);
void		ipRouteChanged( PState *, route *,
			       int (*)(PState *, IPhost *, route *) );
int		ipSameNet( PState *, IPhost *, route *);
xmsg_handle_t	ipSend( XObj s, XObj lls, Msg msg, IPheader *hdr );
xkern_return_t	ipStdPop( XObj, XObj, Msg, void * );
Event		scheduleIpFragCollector( PState *, Path );

xkern_return_t	ipGroupJoin ( IPhost *, XObj );
xkern_return_t	ipGroupLeave( IPhost *, XObj );
xkern_return_t	ipGroupQuery( IPhost *, XObj );

extern int 	traceipp;
extern IPhost	ipSiteGateway;

#endif /* ip_i_h */
