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
 * upi.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * upi.h,v
 * Revision 1.83.1.4.1.2  1994/09/07  04:18:00  menze
 * OSF modifications
 *
 * Revision 1.83.1.4  1994/09/01  04:11:22  menze
 * Subsystem initialization functions can fail
 * XObj initialization functions return xkern_return_t
 * Removed X_NEW and FREE
 *
 * Revision 1.83.1.3  1994/07/21  23:24:41  menze
 * XObjects now store Paths rather than Allocators
 * xOpen and xOpenDone take Path arguments
 *
 * Revision 1.83.1.2  1994/04/14  21:26:34  menze
 * CreateProtl takes an allocator argument
 * xMyAllocator becomes xMyAlloc
 *
 * Revision 1.83.1.1  1994/04/01  16:49:50  menze
 * Added allocator field to XObject
 *
 * Revision 1.83  1993/12/16  23:21:10  menze
 * Added MAC_CTL and FDDI_CTL
 *
 * Revision 1.82  1993/12/11  00:23:25  menze
 * fixed #endif comments
 *
 * Revision 1.81  1993/11/15  20:49:48  menze
 * Added SIM_CTL
 *
 */

#ifndef upi_h
#define upi_h

#include <xkern/include/xtype.h>
#include <xkern/include/idmap.h>
#include <xkern/include/msg_s.h>
#include <xkern/include/part.h>

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE  0
#endif


typedef enum {Protocol,Session} XObjType;

/* default number of down protocols or sessions */
#define STD_DOWN 8

typedef struct xobj *  (* t_open)( XObj, XObj, XObj, Part, Path );
typedef xkern_return_t (* t_close)( XObj );
typedef xkern_return_t (* t_closedone)( XObj );
typedef xkern_return_t (* t_openenable)( XObj, XObj, XObj, Part );
typedef xkern_return_t (* t_opendisable)( XObj, XObj, XObj, Part );
typedef xkern_return_t (* t_opendisableall)( XObj, XObj );
typedef xkern_return_t (* t_opendone)( XObj, XObj, XObj, XObj, Path );
typedef xkern_return_t (* t_demux)( XObj, XObj, Msg );
typedef xkern_return_t (* t_calldemux)( XObj, XObj, Msg, Msg );
typedef xkern_return_t (* t_pop)( XObj, XObj, Msg, void * );
typedef xkern_return_t (* t_callpop)( XObj, XObj, Msg, void *, Msg );
typedef xmsg_handle_t  (* t_push)( XObj, Msg );
typedef xkern_return_t (* t_call)( XObj, Msg, Msg );
typedef int            (* t_control)( XObj, int, char *, int);
typedef xkern_return_t (* t_duplicate)( XObj );
typedef xkern_return_t (* t_shutdown)( XObj );


typedef struct xobj {
    XObjType	type;
    char	*name;
    char	*instName;
    char	*fullName;
    void	*state;
    Bind	binding;
    int		rcnt;
    int		id;
    int		*traceVar;
    t_open		open;
    t_close		close;
    t_closedone		closedone;
    t_openenable	openenable;
    t_opendisable 	opendisable;
    t_opendisableall 	opendisableall;
    t_opendone 		opendone;
    t_demux		demux;
    t_calldemux		calldemux;
    t_pop 		pop;
    t_callpop     	callpop;
    t_push		push;
    t_call		call;
    t_control 		control;
    t_duplicate		duplicate;
    t_shutdown 		shutdown;
    int 	numdown;
    int		downlistsz;
    unsigned char	idle;
    struct xobj	*down[STD_DOWN];
    struct xobj	**downlist;
    struct xobj	*myprotl;
    struct xobj	*up;
    struct xobj	*hlpType;
    struct path	*path;
} XObj_s;


typedef struct xenable {
    XObj	hlpRcv;
    XObj	hlpType;
    Bind	binding;
    int		rcnt;
}   Enable;

typedef XObj	Sessn;
typedef XObj	Protl;

extern int	globalArgc;
extern char	**globalArgv;

#include <xkern/include/msg.h>


/* error stuff */

#define ERR_XK_MSG	((xmsg_handle_t) XK_FAILURE)
#define	ERR_XOBJ	((XObj) XK_FAILURE)
#define	ERR_SESSN	((Sessn) XK_FAILURE)
#define	ERR_PROTL	((Protl) XK_FAILURE)
#define ERR_ENABLE	((Enable *) XK_FAILURE)
#define ERR_XMALLOC	0

/* protocol and session operations */

extern XObj xOpen(XObj hlpRcv, XObj hlpType, XObj llp, Part p, Path);

extern xkern_return_t
	xOpenEnable(XObj hlpRcv, XObj hlpType, XObj llp, Part p);

extern xkern_return_t
	xOpenDisable(XObj hlpRcv, XObj hlpType, XObj llp, Part p);

extern xkern_return_t	xOpenDisableAll(XObj hlpRcv, XObj llp);

extern xkern_return_t	xOpenDone(XObj hlp, XObj s, XObj llp, Path);

extern xkern_return_t	xCloseDone(XObj s);

extern xkern_return_t	xClose(XObj s);

extern xkern_return_t	xDemux(XObj s, Msg msg);

extern xkern_return_t	xCallDemux(XObj s, Msg msg, Msg returnmsg);

extern xmsg_handle_t	xPush(XObj s, Msg msg);

extern xkern_return_t	xCall(XObj s, Msg msg, Msg returnmsg);

extern xkern_return_t	xShutDown(XObj);

/* 
 * xPop, xCallPop and xGetDown prototypes are in upi_inline.h
 */

extern int  		xControl(XObj s, int opcode, char *buf, int len);

extern xkern_return_t	xDuplicate(XObj s );

typedef void	(* DisableAllFunc)(void *, Enable *);

xkern_return_t	defaultOpenDisable(Map, XObj, XObj, void *);

xkern_return_t	defaultOpenDisableAll(Map, XObj, DisableAllFunc);

xkern_return_t	defaultOpenEnable(Map, XObj, XObj, void * );

xkern_return_t	defaultVirtualOpenDisable(XObj, Map, XObj, XObj, XObj *, Part);

xkern_return_t	defaultVirtualOpenDisableAll(XObj, Map, XObj, XObj *);

xkern_return_t	defaultVirtualOpenEnable(XObj, Map, XObj, XObj, XObj *, Part);

/* initialization operations */

typedef xkern_return_t	(XObjInitFunc)( XObj );

extern XObj
	xCreateSessn(XObjInitFunc f, XObj hlpRcv, XObj hlpType, XObj llp,
		     int downc, XObj *downv, Path);

extern XObj
	xCreateProtl(XObjInitFunc f, char *nm, char *inst, int *trace,
		     int downc, XObj *downv, Path);

extern xkern_return_t	xDestroy(XObj s);

extern xkern_return_t	upiInit(void);

/* utility routines */

extern	XObj 	xGetProtlByName(char *name);
extern  boolean_t	xIsValidXObj(XObj);
extern	xkern_return_t  xSetDown(XObj self, int i, XObj object);
extern	void	xPrintXObj(XObj);
extern  void	xRapture(void);

/* object macros */

#define xSetUp(s, hlp)	((s)->up = (hlp))
#define xGetUp(s)	((s)->up)
#define xMyProtl(s)	((s)->myprotl)
#define xIsXObj(s)	((s) && (s) != ERR_XOBJ)
#define xIsSession(s)	((s) && (s) != ERR_XOBJ && (s)->type == Session)
#define xIsProtocol(s)	((s) && (s) != ERR_XOBJ && (s)->type == Protocol)
#define xHlpType(s)	((s)->hlpType)
#define xMyPath(s) ((s)->path)


/*
 * control operation definitions
 *
 * NOTE: if you change the standard control ops, make the
 * corresponding change to the controlops string array in upi.c
 */
enum {
    GETMYHOST = 0,	/* standard control operations        */
    GETMYHOSTCOUNT,	/* common to all protocols            */
    GETPEERHOST,
    GETPEERHOSTCOUNT,
    GETBCASTHOST,
    GETMAXPACKET,
    GETOPTPACKET,
    GETMYPROTO,
    GETPEERPROTO,
    RESOLVE,
    RRESOLVE,
    FREERESOURCES,
    GETPARTICIPANTS,
    SETNONBLOCKINGIO,
    GETRCVHOST,
    ADDMULTI,
    DELMULTI,
    GETMASTER,
    GETGROUPID,
    SETINCARNATION,
    CHECKINCARNATION
};

#define	ARP_CTL		1	/* like a protocol number; used to    */
#define	BLAST_CTL	2	/* partition opcode space	      */
#define	ETH_CTL		3
#define	IP_CTL		4
#define	SCR_CTL		5
#define	VCHAN_CTL	6
#define	PSYNC_CTL	7
#define	SS_CTL		8
#define	SUNRPC_CTL	9
#define	NFS_CTL		10
#define	TCP_CTL		11
#define UDP_CTL		12
#define ICMP_CTL	13
#define VNET_CTL	14
#define BIDCTL_CTL	15
#define CHAN_CTL	16
#define VDROP_CTL       17
#define KM_CTL          18
#define VTAP_CTL        19
#define JOIN_CTL        20
#define CRYPT_CTL       21
#define BIND_CTL        22
#define SIMETH_CTL      23
#define HASH_CTL        24
#define SIM_CTL         25      /* Common ops for the simulator class */
#define MAC_CTL		26
#define FDDI_CTL	27
#define BOOTP_CTL	28
#define PANNING_CTL     29
#define HEARTBEAT_CTL   30
#define CENSUSTAKER_CTL 31
#define MEMBERSHIP_CTL  32
#define SEQUENCER_CTL   33
#define IGMP_CTL        34


#define	TMP0_CTL	100	/* for use by new/tmp protocols until */
#define	TMP1_CTL	101	/* a standard CTL number is assigned  */
#define	TMP2_CTL	102
#define	TMP3_CTL	103
#define	TMP4_CTL	104

#define	MAXOPS		100	/* maximum number of ops per protocol */

/* Check the length of a control argument */
#define checkLen(A, B) {	\
    if ((A) < (B)) {		\
	return -1;		\
    }				\
}

#include <xkern/include/prot/ip_host.h>
#include <xkern/include/prot/eth_host.h>

extern xkern_return_t	str2ipHost( IPhost *, char * );
extern xkern_return_t	str2ethHost( ETHhost *, char * );
extern char *	ipHostStr( IPhost * );
extern char *	ethHostStr( ETHhost * );

extern XObj	xNullProtl;

/*
 * Optimize xDemux, xCallDemux, xPush and xCall as macros.  The other critical
 * path upi functions (xPop, xCallPop and xGetDown) are defined as inline 
 * functions in upi_inline.h
 */
#if !XK_DEBUG || defined(XK_UPI_MACROS)

#define xDemux(DS, M)	((*((DS)->up->demux))((DS)->up, (DS), (M)))
#define xCallDemux(DS, M, RM)	((*((DS)->up->calldemux))((DS)->up, (DS), (M), (RM)))
#define xPush(S, M)	((*((S)->push))((S), (M)))
#define xCall(S, M, RM)	((*((S)->call))((S), (M), (RM)))

#endif /* !XK_DEBUG || defined(XK_UPI_MACROS) */   

#endif

