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
 * lproxy.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.26
 * Date: 1993/11/09 17:18:38
 */

/* 
 * Implementation of x-kernel boundary crossing routines received by
 * a user task from the kernel.
 */

#include <stdio.h>
#include <mach.h> 
#include <mach_error.h> 
#include <servers/netname.h>
#include <xkern/include/xkernel.h>
#include <xkern/include/upi.h>
#include <xkern/protocols/proxy/xk_mig_t.h>
#include <xkern/protocols/proxy/lproxy.h>
#include <xkern/protocols/proxy/proxy_util.h>
#include <xkern/protocols/proxy/proxy_offset.h>
#include "xk_lproxy_server.h"
#include "xk_uproxy.h"


typedef struct {
    mach_port_t		sendPort;
    xobj_ext_id_t	peerId;
} PState;

typedef struct {
    xobj_ext_id_t	peerId;
} SState;


int	tracelproxyp;

boolean_t		xk_lproxy_server();
mach_msg_return_t	xk_mach_msg_server();

static	XObj		firstInstance;

/* 
 * Variables shared by all instantiations of the LPROXY protocol.
 */
static 	mach_port_t	domainPort;	/* Identifies this proxy domain */


/* 
 * map of xobj_ext_id_t's for real objects in other domains to their
 * proxy objects in this domain
 */
static  Map		proxyMap;
#define PROXY_MAP_SZ	113

/* 
 * Map of pool id's to the receive right for that pool.
 */
static	Map		poolMap;
#define POOL_MAP_SZ	30

#define LPROXY_NUM_INPUT_THREADS	10
#define LPROXY_NUM_CONTROL_THREADS	1


xkern_return_t	lproxy_init( XObj );
xkern_return_t	lproxyPingDownTest( XObj, int );
xkern_return_t	lproxyPingUpTest( XObj, int );

static XObj		createPassiveSessn( PORT_TYPE, xobj_ext_id_t, Path );
static XObj		createProxySessn( mach_port_t, xobj_ext_id_t,
					  XObj, XObj, Path ); 
static xkern_return_t	lproxyCall( XObj, Msg, Msg );
static xkern_return_t 	lproxyCallPop( XObj, XObj, Msg, void *, Msg );
static xkern_return_t	lproxyClose( XObj );
static int		lproxyControlProtl( XObj, int, char *, int );
static int		lproxyControlSessn( XObj, int, char *, int );
static xkern_return_t	lproxyOpenDisable( XObj, XObj, XObj, Part );
static XObj		lproxyOpen( XObj, XObj, XObj, Part, Path );
static xkern_return_t	lproxyOpenEnable( XObj, XObj, XObj, Part );
static xkern_return_t 	lproxyPop( XObj, XObj, Msg, void * );
static xmsg_handle_t	lproxyPush( XObj, Msg );

static xkern_return_t
lproxyPeerOpt(
	      XObj self,
	      char **str,
	      int numFields,
	      int line,
	      void *arg);


static XObjRomOpt lproxyOpts[] = {
    { "uproxy", 3, lproxyPeerOpt }
};


static xkern_return_t
findSendPort(
    XObj	self,
    char	*xkPeer)
{
    mach_port_t		bootstrapPort = MACH_PORT_NULL;
    mach_port_t		privilegedPort = MACH_PORT_NULL;
    mach_port_t		masterPort = MACH_PORT_NULL;
    mach_port_t 	ledger_wired;
    mach_port_t		ledger_paged;
    mach_port_t		security_port;

    PState		*ps = (PState *)self->state;
    kern_return_t	kr;

    kr = task_get_bootstrap_port(mach_task_self(), &bootstrapPort);
    if (kr != KERN_SUCCESS) {
	xTraceP2( self, TR_ERRORS,
	    "findSendPort task_get_bootstrap_port 0x%x %s",
	    kr, mach_error_string(kr));
    } else {
	xTraceP1( self, TR_DETAILED,
	    "findSendPort bootstrap port 0x%x", bootstrapPort);
    }

    kr = bootstrap_ports
      (bootstrapPort, &privilegedPort, &masterPort, 
       &ledger_wired, &ledger_paged, &security_port);

    if (kr != KERN_SUCCESS) {
	xTraceP2( self, TR_ERRORS,
	    "findSendPort bootstrap_privileged_ports 0x%x %s",
	    kr, mach_error_string(kr));
    } else {
	xTraceP1( self, TR_DETAILED,
	    "findSendPort device port 0x%x", masterPort);
    }

    xAssert(xkPeer);
    if ( *xkPeer == 0 ) {
	xTraceP0(self, TR_MAJOR_EVENTS, "Talking to in-kernel proxy peer");
	ps->sendPort = masterPort;
    } else {
	xTraceP1(self, TR_EVENTS, "master port == %x\n", masterPort);
	kr = netname_look_up(name_server_port, "", xkPeer, &ps->sendPort);
	if ( kr == KERN_SUCCESS ) {
	    xTrace1(lproxyp, TR_EVENTS,
		    "upProxyFind succeeds, kernPort == %x\n", ps->sendPort);
	} else {
	    sprintf(errBuf, "upProxyFind fails (%s)!", mach_error_string(kr));
	    xError(errBuf);
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}


static xkern_return_t
lproxyPeerOpt(
	      XObj self,
	      char **str,
	      int numFields,
	      int line,
	      void *arg)
{
    *(char **)arg = str[2];
    return XK_SUCCESS;
}


#ifdef XK_PROXY_MSG_HACK
#  define MSG_SERVER	xk_mach_msg_server
#else
#  define MSG_SERVER	mach_msg_server
#endif

static void *
startReader(
    void	*arg)
{
    mach_msg_return_t	mr;
    ProxyThreadInfo	*info = arg;
    Allocator		msgAlloc;
    u_int		size;

    xk_master_lock();
    msgAlloc = pathGetAlloc(info->path, MSG_ALLOC);
    size = XK_MAX_MIG_MSG_LEN;
    if ( allocReserve(msgAlloc, 2, &size) != XK_SUCCESS ) {
	xTrace1(lproxyp, TR_ERRORS,
		"lproxy input thread couldn't reserve buffers from path %d",
		info->path->id);
    }
#if XK_DEBUG
    xTrace2(lproxyp, TR_EVENTS,
	    "lproxy reading thread (port %d, path %d) starts",
	    info->port, info->path->id);
#endif
    xk_master_unlock();
    info->lm.valid = 0;
    cthread_set_data(cthread_self(), info);
    mr = MSG_SERVER(xk_lproxy_server, XK_MAX_MIG_MSG_LEN, info->port,
		    MACH_MSG_OPTION_NONE);
    /* 
     * Should never return
     */
    xError("mach_msg_server(lproxy) returns!");
    if ( mr != MACH_MSG_SUCCESS ) {
	sprintf(errBuf, "mach_msg_server error: %s\n", mach_error_string(mr));
	Kabort(errBuf);
    }
    xk_master_lock();
    size = XK_MAX_MIG_MSG_LEN;
    allocReserve(msgAlloc, -2, &size);
    xk_master_unlock();
    return 0;
}



static xkern_return_t
lproxyPop(
    XObj	s,
    XObj	lls,
    Msg		msg,
    void	*arg)
{
    xTraceP0(s, TR_EVENTS, "lproxyPop");
    return xDemux(s, msg);
}


static xkern_return_t
lproxyCallPop(
    XObj	s,
    XObj	lls,
    Msg		msg,
    void	*arg,
    Msg		rmsg)
{
    xTraceP0(s, TR_EVENTS, "lproxyPop");
    return xCallDemux(s, msg, rmsg);
}


static XObj
createPassiveSessn(
    PORT_TYPE		uproxyPort,
    xobj_ext_id_t	extLls,
    Path		path )
{
    PState		*ps;
    XObj		lproxyProtl = 0;
    XObj		lls;
    kern_return_t	kr;
    xkern_return_t	xkr;

    lls = createProxySessn(uproxyPort, extLls, 0, 0, path);
    if ( ! xIsXObj(lls) ) {
	xTrace0(lproxyp, TR_ERRORS, "Could not create passive session");
	return ERR_XOBJ;
    }
    /* 
     * Other messages may come through and use our newly
     * created session ... that should be OK, but we'll
     * artificially increase the ref count to make sure it
     * doesn't go away before our message has a chance to 
     * increase the ref count in xPop
     */
    xDuplicate(lls);
    xAssert(xIsXObj(lls->up));
    xk_master_unlock();
    kr = call_uproxy_xDuplicate(uproxyPort, domainPort, &xkr,
				(xobj_ext_id_t)lls->up, extLls);
    xk_master_lock();
    xAssert(lls->rcnt > 0);
    lls->rcnt--;
    if ( kr != KERN_SUCCESS && xkr != XK_SUCCESS ) {
	xTraceP0(lproxyProtl, TR_ERRORS,
		 "couldn't duplicate remote lls");
	xClose(lls);
	lls = ERR_XOBJ;
    }
    return lls;
}


static __inline__ Path
myPath()
{
    ProxyThreadInfo	*ti;

    ti = cthread_data(cthread_self());
    xAssert(ti);
    return ti->path;
}


kern_return_t
do_lproxy_xDemux(
    PORT_TYPE 			reqPort,
    PORT_TYPE			uproxyPort,
    xkern_return_t		*xkrReturn,
    xobj_ext_id_t 		extLls,		/* remote object */
    char	 		*msgData,
    mach_msg_type_number_t 	msgDataCnt,
    int				machMsgStart,
    xk_large_msg_data_t		msgOol,
    mach_msg_type_number_t	msgOolLen,
    xk_msg_attr_t		attr,
    mach_msg_type_number_t	attrLen)
{
    XObj		lls;
    Msg_s		msg;
    xkern_return_t	xkr;
    Path		path;

    xTrace0(lproxyp, TR_EVENTS, "do_lproxy_xDemux called");
    CLOCK_TRACE("do_lproxy_xDemux: entry");

    xk_master_lock();
    path = myPath();
    xTrace2(lproxyp, TR_MORE_EVENTS, "input thread %d, path %d",
	    cthread_self(), path->id);
    if ( mapResolve(proxyMap, &extLls, &lls) == XK_FAILURE ) {
	xTrace1(lproxyp, TR_SOFT_ERRORS, "proxyMap lookup fails for extLls %x",
		(int)extLls);
	lls = createPassiveSessn(uproxyPort, extLls, path);

    }
    if ( xIsXObj(lls) ) {
	if ( msgDataCnt ) {
#ifdef XK_PROXY_MSG_HACK
	    int	offset;
	    boolean_t	res;
	    
	    /* 
	     * The message has been dynamically allocated in the MIG server
	     * and we have implicitly been given the reference to it.
	     * We'll free it whenever this x-kernel msg is destroyed.
	     * We thus avoid an extra bcopy.
	     */
	    xAssert(machMsgStart);
	    offset = (int)(msgData - machMsgStart);
	    xAssert(offset > 0);
	    xTrace2(lproxyp, TR_MORE_EVENTS, "msglen %d, data offset is %d",
		    msgDataCnt, offset);
	    /* 
	     * We need to originally construct the message with the full
	     * buffer length (and then trim it down) to be able to take
	     * advantage of other message optimizations.
	     */
	    xkr = msgConstructInPlace(&msg, path, XK_MAX_MIG_MSG_LEN, 0,
				      (char *)machMsgStart,
				      (MsgCIPFreeFunc)allocFree, (void *)0);
	    if ( xkr == XK_FAILURE ) {
		xTraceP0(lls, TR_SOFT_ERRORS,
			 "Couldn't allocate message, dropping packet");
		*xkrReturn = XK_FAILURE;
		goto bailout;
	    }
	    res = msgPopDiscard(&msg, offset);
	    xAssert( res == TRUE );
	    msgTruncate(&msg, msgDataCnt);
	    xAssert( msgLen(&msg) == msgDataCnt );
#else
	    char		*msgBuf;
	    
	    xkr = msgConstructContig(&msg, path, msgDataCnt, 0);
	    if ( xkr == XK_FAILURE ) {
		xTraceP0(lls, TR_SOFT_ERRORS,
			 "Couldn't allocate message, dropping packet");
		*xkrReturn = XK_FAILURE;
		goto bailout;
	    }
	    msgForEach(&msg, msgDataPtr, (void *)&msgBuf);
	    bcopy(msgData, msgBuf, msgDataCnt);
#endif /* XK_PROXY_MSG_HACK */
	} else {
#ifdef XK_PROXY_MSG_HACK
	    allocFree((char *)machMsgStart);
#endif	    
	    xTrace0(lproxyp, TR_MORE_EVENTS, "demux message is out-of-line");
	    xAssert(msgOol);
	    xkr = msgConstructInPlace(&msg, path, msgOolLen, 0, msgOol,
				      oolFree, (void *)0);
	    if ( xkr == XK_FAILURE ) {
		xTraceP0(lls, TR_SOFT_ERRORS,
			 "Couldn't allocate message, dropping packet");
		*xkrReturn = XK_FAILURE;
		goto bailout;
	    }
	}
	if ( attrLen ) {
	    xkern_return_t	xrv;

	    xTraceP1(lls, TR_EVENTS, "attaching message attribute of len %d", attrLen);
	    xrv = msgSetAttr(&msg, 0, attr, attrLen);
	    xAssert( xrv == XK_SUCCESS );
	}
	/* 
	 * Second argument is just a dummy arg.
	 */
	*xkrReturn = xPop(lls, lls, &msg, 0);
	msgDestroy(&msg);
    } else {
	*xkrReturn = XK_FAILURE;
    }
bailout:    
    xk_master_unlock();

    CLOCK_TRACE("do_lproxy_xDemux: exit");
    return KERN_SUCCESS;
}


kern_return_t
do_lproxy_xCallDemux(
    PORT_TYPE 			reqPort,
    PORT_TYPE 			uproxyPort,
    xkern_return_t		*xkrReturn,
    xobj_ext_id_t 		extLls,		/* remote object */
    char	 		*reqMsgData,
    mach_msg_type_number_t 	reqMsgLen,
    int				machMsgStart,	/* address of request msg */
    xk_large_msg_data_t		reqOol,
    mach_msg_type_number_t	reqOolLen,
    char	 		*repMsgData,
    mach_msg_type_number_t 	*repMsgLen,
    xk_large_msg_data_t		*repOol,
    mach_msg_type_number_t	*repOolLen,
    xk_msg_attr_t		reqAttr,
    mach_msg_type_number_t	reqAttrLen)
{
    XObj		lls;
    Msg_s		reqMsg, repMsg;
    xkern_return_t	xkr;
    Path		path;

    CLOCK_TRACE("do_lproxy_xCallDemux: entry");
    xTrace0(lproxyp, TR_EVENTS, "do_lproxy_xCallDemux called");

    xk_master_lock();
    lingeringMsgClear();
    path = myPath();
    xTrace2(lproxyp, TR_MORE_EVENTS, "input thread %d, path %d",
	    cthread_self(), path->id);
    if ( mapResolve(proxyMap, &extLls, &lls) == XK_FAILURE ) {
	xTrace1(lproxyp, TR_SOFT_ERRORS, "proxyMap lookup fails for extLls %x",
		(int)extLls);
	lls = createPassiveSessn(uproxyPort, extLls, path);
    }
    if ( xIsXObj(lls) ) {
	if ( reqMsgLen ) {

#ifdef XK_PROXY_MSG_HACK
	    int	offset;
	    boolean_t	res;
	    
	    /* 
	     * The message has been dynamically allocated in the MIG server
	     * and we have implicitly been given the reference to it.
	     * We'll free it whenever this x-kernel msg is destroyed.
	     * We thus avoid an extra bcopy.
	     */
	    xAssert(machMsgStart);
	    offset = (int)(reqMsgData - machMsgStart);
	    xAssert(offset > 0);
	    xTrace1(lproxyp, TR_MORE_EVENTS, "xkernel data offset is %d", offset);
	    xkr = msgConstructInPlace(&reqMsg, path,
				      XK_MAX_MIG_MSG_LEN, 0,
				      (char *)machMsgStart,
				      (MsgCIPFreeFunc)allocFree, (void *)0);
	    if ( xkr == XK_FAILURE ) {
		xTraceP0(lls, TR_SOFT_ERRORS,
			 "Couldn't allocate message, dropping packet");
		*xkrReturn = XK_FAILURE;
		goto bailout;
	    }
	    res = msgPopDiscard(&reqMsg, offset);
	    xAssert( res == TRUE );
	    msgTruncate(&reqMsg, reqMsgLen);
	    xAssert( msgLen(&reqMsg) == reqMsgLen );
#else
	    char	*msgBuf;
	    
	    /* 
	     * We have to copy the message because someone above us could
	     * save a reference to it.
	     */
	    xkr = msgConstructContig(&reqMsg, path, reqMsgLen, 0);
	    if ( xkr == XK_FAILURE ) {
		xTraceP0(lls, TR_SOFT_ERRORS,
			 "Couldn't allocate message, dropping packet");
		*xkrReturn = XK_FAILURE;
		goto bailout;
	    }
	    msgForEach(&reqMsg, msgDataPtr, (void *)&msgBuf);
	    bcopy(reqMsgData, msgBuf, reqMsgLen);
#endif
	} else {
#ifdef XK_PROXY_MSG_HACK
	    allocFree((char *)machMsgStart);
#endif	    
	    xTrace0(lproxyp, TR_MORE_EVENTS,
		    "callDemux request is out-of-line");
	    xAssert(reqOol);
	    xkr= msgConstructInPlace(&reqMsg, path, reqOolLen, 0,
				     reqOol, oolFree, (void *)0);
	    if ( xkr == XK_FAILURE ) {
		xTraceP0(lls, TR_SOFT_ERRORS,
			 "Couldn't allocate message, dropping packet");
		*xkrReturn = XK_FAILURE;
		goto bailout;
	    }
	}
	if ( reqAttrLen ) {

	    xTraceP1(lls, TR_EVENTS,
		     "attaching message attribute of len %d", reqAttrLen);
	    xkr = msgSetAttr(&reqMsg, 0, reqAttr, reqAttrLen);
	    xAssert( xkr == XK_SUCCESS );
	}
	msgConstructContig(&repMsg, path, 0, 0);
	xIfTrace(lproxyp, TR_DETAILED) {
	    xTrace0(lproxyp, TR_ALWAYS, "request:");
	    msgShow(&reqMsg);
	}
	/* 
	 * Second argument is just a dummy arg.
	 */
	*xkrReturn = xCallPop(lls, lls, &reqMsg, 0, &repMsg);
	*repOolLen = 0;
	if ( *xkrReturn == XK_SUCCESS ) {
	    xTrace1(lproxyp, TR_EVENTS, "xCallPop returns success, %d bytes",
		    msgLen(&repMsg));
	    xIfTrace(lproxyp, TR_DETAILED) {
		xTrace0(lproxyp, TR_ALWAYS, "reply:");
		msgShow(&repMsg);
	    }
	    if ( msgLen(&repMsg) > XK_MAX_MSG_INLINE_LEN ) {
		DeallocFunc	dFunc;
		void		*dArg;

		msgToOol(&repMsg, repOol, &dFunc, &dArg);
		*repOolLen = msgLen(&repMsg);
		lingeringMsgSave(dFunc, dArg, *repOolLen);
	    } else {
		xAssert(msgLen(&repMsg) <= *repMsgLen);
		msg2buf(&repMsg, repMsgData);
		*repMsgLen = msgLen(&repMsg);
	    }
	} else {
	    xTrace0(lproxyp, TR_EVENTS, "xCallPop returns failure");
	}
	msgDestroy(&reqMsg);
	msgDestroy(&repMsg);
    } else {
	*xkrReturn = XK_FAILURE;
    }
bailout:    
    xk_master_unlock();

    CLOCK_TRACE("do_lproxy_xCallDemux: exit");
    return KERN_SUCCESS;
}




kern_return_t
do_lproxy_xOpenDone(
    mach_port_t		reqPort,
    mach_port_t		uproxyPort,
    xkern_return_t	*xkr,
    xobj_ext_id_t	extLls,
    xobj_ext_id_t	extLlp,
    xobj_ext_id_t	hlpRcv,
    xobj_ext_id_t	hlpType,
    xk_path_t		pathId)
{
    XObj	s, llp;
    PState	*ps;
    Path	path;

    xTrace1(lproxyp, TR_EVENTS, "do_lproxy_xOpenDone called, extLls == %x",
	    (int)extLls);

    xk_master_lock();
    if ( ! (path = getPathById(pathId)) ) {
	xTrace1(lproxyp, TR_SOFT_ERRORS, "no path corresponding to %d", pathId);
	path = xkDefaultPath;
    }
    /* 
     * We should have pre-existing XObjects corresponding to llp,
     * hlpRcv and hlpType.  We will need to create a new proxy session
     * corresponding to lls.
     */
    if ( ! xIsValidXObj((XObj)hlpRcv) ) {
	xTrace1(lproxyp, TR_ERRORS, "do_openDone -- xobj %x is unsafe", hlpRcv);
	*xkr = XK_FAILURE;
    } else if ( ! xIsValidXObj((XObj)hlpType) ) {
	xTrace1(lproxyp, TR_ERRORS, "do_openDone -- xobj %x is unsafe",
		hlpType);
	*xkr = XK_FAILURE;
    } else if ( mapResolve(proxyMap, &extLlp, &llp) == XK_FAILURE ) {
	xTrace1(lproxyp, TR_ERRORS, "do_openDone -- no proxy object for llp %x",
		(int)extLlp);
	*xkr = XK_FAILURE;
    } else if ( mapResolve(proxyMap, &extLls, 0) == XK_SUCCESS ) {
	xTraceP1(llp, TR_SOFT_ERRORS,
		 "proxy object for %lx already exists", (long)extLls);
	*xkr = XK_SUCCESS;
    } else {
	ps = (PState *)llp->state;
	s = createProxySessn(uproxyPort, extLls, (XObj)hlpRcv, (XObj)hlpType,
			     path);
	if ( s == ERR_XOBJ ) {
	    *xkr = XK_FAILURE;
	} else {
	    *xkr = xOpenDone((XObj)hlpRcv, s, llp, path);
	}
    }
    xk_master_unlock();

    return KERN_SUCCESS;
}


kern_return_t
do_lproxy_dumpXObj(
    mach_port_t		reqPort,
    xkern_return_t	*xkr,
    xobj_ext_id_t	extObj,
    xk_xobj_dump_t	*dump)
{
    xTrace0(lproxyp, TR_DETAILED, "do_lproxy_dumpXObj called");
    xk_master_lock();
    if ( ! xIsValidXObj((XObj)extObj) ) {
	xTrace1(lproxyp, TR_ERRORS, "do_dumpXObj -- xobj %x is unsafe", extObj);
	*xkr = XK_FAILURE;
    } else {
	XObj obj = (XObj)extObj;
	xIfTrace(lproxyp, TR_DETAILED) {
	    xPrintXObj(obj);
	}
	dump->type = obj->type;
	dump->myprotl = (xobj_ext_id_t)obj->myprotl;
/*	if ( xIsProtocol(obj) ) { */
	    dump->hlpRcv = (xobj_ext_id_t)ERR_XOBJ;
	    dump->hlpType = (xobj_ext_id_t)ERR_XOBJ;
#if 0
	} else {
	    dump->hlpRcv = peerProtl(obj->up);
	    dump->hlpType = peerProtl(xHlpType(obj));
	}
#endif
    	dump->pathId = obj->path->id;
	strcpy(dump->name, obj->name);
	strcpy(dump->instName, obj->instName);
	*xkr = XK_SUCCESS;
	xTrace2(lproxyp, TR_FULL_TRACE,
		"dumpXObj: sending name: %s  instName: %s",
		dump->name, dump->instName);
    } 
    xk_master_unlock();
    return KERN_SUCCESS;
}


kern_return_t
do_lproxy_ping(
    mach_port_t		reqPort)
{
    xTrace0(lproxyp, TR_EVENTS, "lproxy Ping");
    return KERN_SUCCESS;
}




/* 
 * Normal UPI operations (called by objects from within my own
 * protection domain)
 */


static xkern_return_t
lproxyOpenDisable(
    XObj self,
    XObj hlpRcv,
    XObj hlpType,
    Part p)
{
    xk_part_t		extPart;
    xkern_return_t	xkr;
    kern_return_t	kr;
    PState		*ps = (PState *)self->state;
    int			bufLen;

    xTraceP0(self, TR_DETAILED, "openDisable");

    bufLen = sizeof(extPart);    
    if ( partExternalize(p, &extPart, &bufLen) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "could not externalize participant");
	return XK_FAILURE;
    }
    xk_master_unlock();
    kr = call_uproxy_xOpenDisable(ps->sendPort, domainPort, &xkr,
				 (xobj_ext_id_t)hlpRcv, (xobj_ext_id_t)hlpType,
				 ps->peerId, extPart);
    if ( kr != KERN_SUCCESS ) {
	xTraceP1(self, TR_ERRORS, "mach msg error %s in call_xOpenDisable",
		 mach_error_string(kr));
	xkr = XK_FAILURE;
    } else {
	if ( xkr == XK_FAILURE ) {
	    xTraceP0(self, TR_SOFT_ERRORS, "call_xOpenDisable fails");
	}
    }
    xk_master_lock();
    return xkr;
}


static xkern_return_t
lproxyOpenEnable(
    XObj self,
    XObj hlpRcv,
    XObj hlpType,
    Part p)
{
    xk_part_t		extPart;
    xkern_return_t	xkr;
    kern_return_t	kr;
    PState		*ps = (PState *)self->state;
    int			bufLen;

    xTraceP3(self, TR_DETAILED,
	"openEnable(,hlpRcv=%x, hlpType=%x, part=%x", hlpRcv, hlpType, p);

    bufLen = sizeof(extPart);    
    if ( partExternalize(p, &extPart, &bufLen) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "could not externalize participant");
	return XK_FAILURE;
    }
    xk_master_unlock();
    kr = call_uproxy_xOpenEnable(
		ps->sendPort, domainPort, &xkr,
		(xobj_ext_id_t)hlpRcv, (xobj_ext_id_t)hlpType,
		ps->peerId, extPart);
    if ( kr != KERN_SUCCESS ) {
	xTraceP1(self, TR_ERRORS, "mach msg error %s in call_xOpenEnable",
		 mach_error_string(kr));
	xkr = XK_FAILURE;
    } else {
	if ( xkr == XK_FAILURE ) {
	    xTraceP0(self, TR_SOFT_ERRORS, "call_xOpenEnable fails");
	}
    }
    xk_master_lock();
    return xkr;
}


#define CHECK_KERN_RESULT( _res, _routine, _retVal )			\
{ 									\
   if ( (_res) != KERN_SUCCESS ) {					\
	xTraceP2(s, TR_ERRORS, "mach msg error %s in call_uproxy_%s",	\
		(_routine), mach_error_string(_res));			\
		return (_retVal);					\
       }								\
}


static int
callControl(
    PORT_TYPE		port,
    XObj		obj,
    xobj_ext_id_t	extObj,
    int			op,
    char		*buf,
    int			len)
{
    kern_return_t	kr;
    int			bufLen;
    char                *ctlBuf;

    xTraceP0(obj, TR_EVENTS, "callControl called");

    xTraceP1(obj, TR_EVENTS, "callControl: send port is %x\n", port);

    xk_master_unlock();
    ctlBuf = (char *)pathAlloc(obj->path, XK_MAX_CTL_BUF_LEN);
    if (!ctlBuf) {
	xTraceP1(obj, TR_ERRORS, "allocation failure for (%d) bytes in call_xControl",
		 XK_MAX_CTL_BUF_LEN);
	return -1;
    }
    bcopy(buf, ctlBuf, len);
    bufLen = len;
    kr = call_uproxy_xControl(port, extObj, op, ctlBuf, &bufLen);
    xk_master_lock();
    if ( kr != KERN_SUCCESS ) {
	xTraceP1(obj, TR_ERRORS, "mach msg error %s in call_xControl",
		 mach_error_string(kr));
	pathFree(ctlBuf);
	return -1;
    }
    if ( bufLen > len ) {
	xError("buffer size grew in lproxyControl");
	xTraceP2(obj, TR_ERRORS, "was %d, now %d", len, bufLen);
	bufLen = -1;
    } else {
	if ( bufLen > 0 ) {
	    bcopy(ctlBuf, buf, bufLen);
	}
    }
    pathFree(ctlBuf);
    return bufLen;
}


static int
lproxyControlProtl(
    XObj	s,
    int		op,
    char	*buf,
    int		len)
{
    PState		*ps = (PState *)s->state;

    xTraceP0(s, TR_EVENTS, "lproxyControlProtl called");

    return callControl(ps->sendPort, s, ps->peerId, op, buf, len);
}


static int
lproxyControlSessn(
    XObj	s,
    int		op,
    char	*buf,
    int		len)
{
    SState		*ss = (SState *)s->state;
    PState		*ps = (PState *)xMyProtl(s)->state;

    xTraceP0(s, TR_EVENTS, "lproxyControlSessn called");

    return callControl(ps->sendPort, s, ss->peerId, op, buf, len);
}


static xkern_return_t
sessnInit(
    XObj	s)
{
    s->push = lproxyPush;
    s->close = lproxyClose;
    s->call = lproxyCall;
    s->control = lproxyControlSessn;
    s->pop = lproxyPop;
    s->callpop = lproxyCallPop;
    return XK_SUCCESS;
}


static xkern_return_t
checkHlpValidity(
    xobj_ext_id_t	hlp,
    XObj		expectedHlp)
{
    if ( ! xIsValidXObj((XObj)hlp) ) {
	xTrace1(lproxyp, TR_ERRORS,
		"lproxyFindProxySessn: lls->hlp %x is not valid XObj",
		hlp);
	return XK_FAILURE;
    }
    if ( expectedHlp && expectedHlp != (XObj)hlp ) {
	xTrace2(lproxyp, TR_ERRORS, "lproxyFindProxySessn: hlp mismatch "
		"(expected: %x, internal: %x)", (int)expectedHlp, (int)hlp);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


/* 
 * Find (or create) an appropriate proxy session.  expHlpRcv and
 * expHlpType will only be used for consistency checks and may be
 * null.  We assume that the other proxy is keeping a reference count
 * representing this new session we are creating.  Thus if we don't
 * actually create a session, we will execute a close on the lower
 * proxy. 
 */
static XObj
createProxySessn(
    mach_port_t		uproxyPort,
    xobj_ext_id_t	extLls,
    XObj		expHlpRcv,
    XObj		expHlpType,
    Path		path )
{
    XObj		lproxyProtl;
    XObj		lls;
    kern_return_t	kr;
    xkern_return_t	xkr;
    xk_xobj_dump_t	objDump;
    SState		*ss;
    
    xAssert(proxyMap);
    if ( mapResolve(proxyMap, &extLls, &lls) == XK_FAILURE ) {
	/* 
	 * Get the poop on the real session for which we are
	 * creating the proxy  
	 */
	kr = call_uproxy_dumpXObj(uproxyPort, &xkr, extLls, &objDump);
	if ( kr != KERN_SUCCESS || xkr == XK_FAILURE ) {
	    xTrace1(lproxyp, TR_ERRORS, "mach_msg error for call_dumpXObj (%s)",
		    mach_error_string(kr));
	    return ERR_XOBJ;
	}
	if ( xkr == XK_FAILURE ) {
	    xTrace1(lproxyp, TR_ERRORS, "call_dumpXObj(%x) fails", extLls);
	    return ERR_XOBJ;
	}
	/* 
	 * Find the local proxy protocol which should be used to
	 * create this session.  This is unnecessary for xOpens but
	 * provides a sanity check.
	 */
	if ( mapResolve(proxyMap, &objDump.myprotl, &lproxyProtl)
	    		== XK_FAILURE ) {
	    xTrace2(lproxyp, TR_ERRORS,
		    "No proxy protl corresponds to external lls->myprotl %s/%s",
		    objDump.name, objDump.instName);
	    return ERR_XOBJ;
	}
	if ( checkHlpValidity(objDump.hlpRcv, expHlpRcv) == XK_FAILURE ||
	     checkHlpValidity(objDump.hlpType, expHlpType) == XK_FAILURE ) {
	    return ERR_XOBJ;
	}
	lls = xCreateSessn(sessnInit, (XObj)objDump.hlpRcv,
			   (XObj)objDump.hlpType, lproxyProtl, 0, 0, path);
	if ( lls == ERR_XOBJ ) {
	    xTraceP0(lproxyProtl, TR_SOFT_ERRORS, "xCreateSessn fails!");
	    return ERR_XOBJ;
	}
	if ( ! (ss = pathAlloc(path, sizeof(SState))) ) {
	    xDestroy(lls);
	    xTraceP0(lproxyProtl, TR_SOFT_ERRORS,
		     "failure allocating session state");
	    return ERR_XOBJ;
	}
	if ( mapBind(proxyMap, &extLls, lls, &lls->binding) != XK_SUCCESS ) {
	    allocFree(ss);
	    xDestroy(lls);
	    xTraceP0(lproxyProtl, TR_ERRORS, "failure binding session");
	    return ERR_XOBJ;
	}
	ss->peerId = extLls;
	lls->state = ss;
    } else {
	xTrace1(lproxyp, TR_MORE_EVENTS, "lproxy session %x already exists",
		lls);
	kr = call_uproxy_xClose(uproxyPort, &xkr, extLls);
	if ( kr != KERN_SUCCESS ) {
	    xTrace1(lproxyp, TR_ERRORS, "mach msg error %s in call_xClose",
		     mach_error_string(kr));
	}
    }
    return lls;
}


static XObj
lproxyOpen(
    XObj self,
    XObj hlpRcv,
    XObj hlpType,
    Part p,
    Path path )
{
    xk_part_t		extPart;
    kern_return_t	kr;
    xobj_ext_id_t	extLls;
    XObj		lls;
    PState		*ps = (PState *)self->state;
    int			bufLen;
    
    xTraceP0(self, TR_DETAILED, "open");

    bufLen = sizeof(extPart);    
    if ( partExternalize(p, &extPart, &bufLen) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "could not externalize participant");
	return ERR_XOBJ;
    }
    xk_master_unlock();
    kr = call_uproxy_xOpen(ps->sendPort, domainPort, &extLls,
			   (xobj_ext_id_t)hlpRcv, (xobj_ext_id_t)hlpType,
			   ps->peerId, extPart, path->id);
    xk_master_lock();
    if ( kr != KERN_SUCCESS ) {
	xTraceP1(self, TR_ERRORS, "mach msg error %s in call_xOpen",
		 mach_error_string(kr));
	lls = ERR_XOBJ;
    } else {
	xTraceP0(self, TR_DETAILED, "call_xOpen returns success");
	if ( xIsXObj((XObj)extLls) ) {
	    lls = createProxySessn(ps->sendPort, extLls, hlpRcv, hlpType, path);
	} else {
	    xTraceP0(self, TR_SOFT_ERRORS, "call_xOpen fails");
	    lls = ERR_XOBJ;
	}
    }
    return lls;
}


static xmsg_handle_t
lproxyPush(
    XObj	s,
    Msg		msg)
{
    PState		*ps = (PState *)xMyProtl(s)->state;
    SState		*ss = (SState *)s->state;
    xmsg_handle_t	handle;
    kern_return_t	kr;
    xkern_return_t	xkr;
    xk_and_mach_msg_t	xmsg;
    char		*oolBuf;
    int			inlineLen, oolLen;
    DeallocFunc		dFunc = 0;
    void		*dArg;

    xTraceP0(s, TR_EVENTS, "push called");

    if ( msgLen(msg) <= XK_MAX_MSG_INLINE_LEN ) {
	if ( msgIsOkToMangle(msg, (char **)&xmsg.machMsg,
			     XK_PUSH_REQ_OFFSET) ) {
	    xTraceP0(s, TR_MORE_EVENTS, "push -- msg hack works");
	    xAssert(&xmsg.machMsg);
	    xmsg.xkMsg = 0;
	} else {
	    xmsg.machMsg = 0;
	    xmsg.xkMsg = msg;
	}
	inlineLen = msgLen(msg);
	oolLen = 0;
	oolBuf = 0;
    } else {
	xmsg.machMsg = 0;
	xmsg.xkMsg = 0;
	inlineLen = 0;
	oolLen = msgLen(msg);
	xkr = msgToOol(msg, &oolBuf, &dFunc, &dArg);
	xAssert( xkr == XK_SUCCESS ) ;
    }
    xk_master_unlock();

    CLOCK_TRACE("lproxyPush: C call_uproxy_xPush");
    kr = call_uproxy_xPush(ps->sendPort, &handle, ss->peerId, (char *)&xmsg,
			   inlineLen, oolBuf, oolLen,
			   msgGetAttr(msg, 0), msg->attrLen,
			   msgGetPath(msg)->id);
    CLOCK_TRACE("lproxyPush: R call_uproxy_xPush");
    if ( kr != KERN_SUCCESS ) {
	xTraceP1(s, TR_ERRORS, "mach msg error %s in call_xPush",
		 mach_error_string(kr));
	handle = XMSG_ERR_HANDLE;
    }
    xTraceP1(s, TR_MORE_EVENTS, "push returns %x", (int)handle);

    xk_master_lock();
    if ( dFunc ) {
	dFunc(dArg, oolLen);
    }
    return handle;
}



static xkern_return_t
lproxyCall(
    XObj	s,
    Msg		msg,
    Msg		rmsg)
{
    PState		*ps = (PState *)xMyProtl(s)->state;
    SState		*ss = (SState *)s->state;
    kern_return_t	kr;
    xkern_return_t	xkr;
    mach_msg_type_number_t	rmsgLen;
    Msg_s			tmpMsg;
    char		*machMsgBuf;
    mach_msg_type_number_t	repoolCnt = 0;
    char		*reqool, *repool;
    xk_u_int32		repPathId;
    Path		repPath;

    xTraceP0(s, TR_EVENTS, "call called");

    xIfTraceP(s, TR_DETAILED) {
	xTraceP0(s, TR_ALWAYS, "request:");
	msgShow(msg);
    }
    repPathId = msgGetPath(rmsg)->id;
    xkr = msgConstructContig(&tmpMsg, msgGetPath(rmsg),
			     XK_MAX_MIG_MSG_LEN, 0);
    if ( xkr != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    msgForEach(&tmpMsg, msgDataPtr, (void *)&machMsgBuf);
    rmsgLen = XK_MAX_MSG_INLINE_LEN;
    msgPopDiscard(&tmpMsg, XK_CALL_REP_OFFSET);
    if ( msgLen(msg) <= XK_MAX_MSG_INLINE_LEN ) {
	xk_master_unlock();
	CLOCK_TRACE("lproxyCall: C call_uproxy_xCall: Inline req");
	kr = call_uproxy_xCall(ps->sendPort, &xkr, ss->peerId,
			       (char *)msg, msgLen(msg), (char *)0, 0,
			       machMsgBuf, &rmsgLen,
			       &repool, &repoolCnt,
			       msgGetAttr(msg, 0), msg->attrLen,
			       msgGetPath(msg)->id, &repPathId);
	CLOCK_TRACE("lproxyCall: R call_uproxy_xCall: Inline req");
    } else {
	DeallocFunc	dFunc = 0;
	void		*dArg;

	xTraceP0(s, TR_EVENTS, "call passing request out-of-line");
	xkr = msgToOol(msg, &reqool, &dFunc, &dArg);
	xAssert( xkr == XK_SUCCESS );
	xk_master_unlock();
	CLOCK_TRACE("lproxyCall: C call_uproxy_xCall: OOL req");
	kr = call_uproxy_xCall(ps->sendPort, &xkr, ss->peerId,
			       (char *)0, 0, (char *)reqool, msgLen(msg),
			       machMsgBuf, &rmsgLen,
			       &repool, &repoolCnt,
			       msgGetAttr(msg, 0), msg->attrLen,
			       msgGetPath(msg)->id, &repPathId);
	CLOCK_TRACE("lproxyCall: R call_uproxy_xCall: OOL req");
	if ( dFunc ) {
	    dFunc(dArg, msgLen(msg));
	}
    }

    xk_master_lock();
    
    CHECK_KERN_RESULT(kr, "xCall", XK_FAILURE);
    if ( repPathId != msgGetPath(rmsg)->id ) {
	xTrace2(lproxyp, TR_MORE_EVENTS,
		"reply message path changed from %d to %d",
		msgGetPath(rmsg)->id, repPathId);
	if ( ! (repPath = getPathById(repPathId)) ) {
	    repPath = xkDefaultPath;
	}
    } else {
	repPath = msgGetPath(rmsg);
    }
    if ( xkr == XK_SUCCESS ) {
	if ( repoolCnt > 0 ) {
	    xTraceP0(s, TR_MORE_EVENTS, "reply msg came to us out-of-line");
	    xAssert(repool);
	    msgDestroy(&tmpMsg);
	    if ( msgConstructInPlace(&tmpMsg, repPath, repoolCnt, 0, repool,
				     oolFree, (void *)0) != XK_SUCCESS ) {
		return XK_FAILURE;
	    }
	} else {
	    xTraceP1(s, TR_MORE_EVENTS, "call truncates reply msg to %d",
		     rmsgLen);
	    xAssert( msgLen(&tmpMsg) >= rmsgLen );
	    msgTruncate(&tmpMsg, rmsgLen);
	    if ( tmpMsg.path->id != repPath->id ) {
		/* 
		 * Copy the buffer to a new path.  This should
		 * eventually be a path transfer operation.
		 */
		Msg_s	msgCopy;
		char	*copyBuf;
		
		xTraceP0(s, TR_EVENTS, "transferring buffers");
		if ( msgConstructContig(&msgCopy, repPath, rmsgLen, 0)
		    	== XK_SUCCESS ) {
		    msgForEach(&msgCopy, msgDataPtr, &copyBuf);
		    bcopy(machMsgBuf + XK_CALL_REP_OFFSET, copyBuf, rmsgLen);
		    msgAssign(&tmpMsg, &msgCopy);
		    msgDestroy(&msgCopy);
		}
	    }
	}
	msgAssign(rmsg, &tmpMsg);
    }
    msgDestroy(&tmpMsg);
    xIfTraceP(s, TR_DETAILED) {
	xTraceP0(s, TR_ALWAYS, "reply:");
	msgShow(rmsg);
    }
    xTraceP1(s, TR_MORE_EVENTS, "call returns %d", xkr);
    return xkr;
}


static xkern_return_t
lproxyClose(
    XObj	s)
{
    PState		*ps = (PState *)xMyProtl(s)->state;
    SState		*ss = (SState *)s->state;
    kern_return_t	kr;
    xkern_return_t	xkr;

    xTraceP0(s, TR_EVENTS, "close called");

    if ( mapUnbind(proxyMap, &ss->peerId) == XK_FAILURE ) {
	xTraceP1(s, TR_ERRORS, "map unbind failure (%x) in close",
		 (int)ss->peerId);
    }
    xk_master_unlock();
    kr = call_uproxy_xClose(ps->sendPort, &xkr, ss->peerId);
    xk_master_lock();
    if ( kr != KERN_SUCCESS ) {
	xTraceP1(s, TR_ERRORS, "mach msg error %s in call_xClose",
		 mach_error_string(kr));
	xkr = XK_FAILURE;
    }
    xTraceP1(s, TR_MORE_EVENTS, "lproxyClose returns %d", xkr);
    xDestroy(s);
    return xkr;
}


static xkern_return_t
lproxyShutDown(
    XObj	p)
{
    PState	*ps = (PState *)p->state;

    xTraceP0(p, TR_MAJOR_EVENTS, "shutdown");
    call_uproxy_abort(ps->sendPort, domainPort);
    return XK_SUCCESS;
}


#define do_call(str, call) 						\
    do {								\
        kern_return_t kr;						\
        kr = call;							\
	if ( kr != KERN_SUCCESS ) {					\
		sprintf(errBuf, "call %s from proxy returns %s", 	\
			str, mach_error_string(kr));			\
		xError(errBuf);						\
		return XK_FAILURE; 					\
	}								\
    } while (0)


#define do_msg_call(str, call) 						\
    do {								\
        kern_return_t kr;						\
        kr = call;							\
	if ( kr != KERN_SUCCESS ) {					\
		sprintf(errBuf, "mach message error  %s from %s", 	\
			mach_error_string(kr), str);			\
		xError(errBuf);						\
		return XK_FAILURE;					\
	}								\
    } while (0)


static xkern_return_t
poolInfo(
    XObj		self,
    Path		path,
    u_int		bufs,
    u_int		threads,
    XkThreadPolicy	policy)
{
    ProxyThreadInfo	*info;
    PORT_TYPE		pathPort;
    int			i;
    PState		*ps = self->state;

    if ( mapResolve(poolMap, &path->id, &pathPort) == XK_FAILURE ) {
	xTraceP3(self, TR_EVENTS,
		 "creating input pool for path %d, %d bufs, %d threads",
		 path->id, bufs, threads);
	do_call("rcv port allocate",
		mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				   &pathPort));
	if ( mapBind(poolMap, &path->id, pathPort, 0) != XK_SUCCESS ) {
	    return XK_FAILURE;
	}
	if ( ! (info = pathAlloc(self->path, sizeof(ProxyThreadInfo))) ) {
	    return XK_NO_MEMORY;
	}
	info->path = path;
	info->port = pathPort;
	if ( threads == 0 ) {
	    threads = LPROXY_NUM_INPUT_THREADS;
	}
	for ( i=0; i < threads; i++ ) {
	    cthread_fork( startReader, info );
	}
    } else {
	xTraceP1(self, TR_EVENTS, "input pool for path %d already exists",
		 path->id);
    }
    do_msg_call("establishPath",
		call_uproxy_establishPath(ps->sendPort, domainPort,
					  pathPort, path->id));
    return XK_SUCCESS;
}


static xkern_return_t
inputInfo(
    XObj	self,
    Path	path,
    u_int	msgSize,
    int		numMsgs)
{
    u_int	size = XK_MAX_MIG_MSG_LEN;

    if ( firstInstance != self ) {
	/* 
	 * Only the first lproxy instance reserves input buffers
	 * (although they are shared by all instances)
	 */
	return XK_SUCCESS;
    }
    if ( msgSize > XK_MAX_MSG_INLINE_LEN ) {
	/* 
	 * We probably should establish an allocator for out-of-line
	 * memory.  
	 */
	return XK_SUCCESS;
    }
    return allocReserve(pathGetAlloc(path, MSG_ALLOC), numMsgs, &size);
}


static xkern_return_t
initDomain(
    Path	path)
{
    ProxyThreadInfo	*info;
    int			i;

    if ( domainPort == 0 ) {
	do_call("rcv port allocate",
		mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				   &domainPort));
	if ( ! (info = pathAlloc(path, sizeof(ProxyThreadInfo))) ) {
	    return XK_NO_MEMORY;
	}
	info->path = path;
	info->port = domainPort;
	for ( i=0; i < LPROXY_NUM_CONTROL_THREADS; i++ ) {
	    cthread_fork( startReader, info );
	} 
	poolMap = mapCreate(POOL_MAP_SZ, sizeof(xk_u_int32), path);
	proxyMap = mapCreate(PROXY_MAP_SZ, sizeof(xobj_ext_id_t), path);
	if ( proxyMap == 0 || poolMap == 0 ) return XK_NO_MEMORY;
    }
    return XK_SUCCESS;
}


xkern_return_t
lproxy_init(
    XObj	self)
{
    kern_return_t	kr;
    char		*uproxy = "";
    PState		*ps;
    int			i;

    xTraceP0(self, TR_GROSS_EVENTS, "init called");
    if ( firstInstance == 0 ) {
	firstInstance = self;
    }
    if ( ! (ps = pathAlloc(self->path, sizeof(PState))) ) {
	return XK_NO_MEMORY;
    }
    self->state = ps;
    findXObjRomOpts(self, lproxyOpts,
		    sizeof(lproxyOpts)/sizeof(XObjRomOpt), &uproxy);
    if ( findSendPort(self, uproxy) != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    if ( initDomain(self->path) != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    pathRegisterDriver(self, poolInfo, inputInfo);
    /* 
     * Find the external ID for the protocol for which this instantiation
     * is a proxy
     */
    xTraceP0(self, TR_EVENTS, "looking up id of real object");
    kr = call_uproxy_xGetProtlByName(ps->sendPort, self->instName, &ps->peerId);
    if ( kr == KERN_SUCCESS && ps->peerId ) {
	xTraceP1(self, TR_EVENTS,
		"call_uproxy_xGetProtlByName result: obj %x\n", (int)ps->peerId);
    } else {
	xTraceP1(self, TR_ERRORS, 
		"call_uproxy_xGetProtlByName failed: %s",
		 (kr == KERN_SUCCESS) ? "no such object" :
		 mach_error_string(kr));
	return XK_FAILURE;
    }
    if ( mapBind(proxyMap, &ps->peerId, self, 0) != XK_SUCCESS ) {
	xTraceP0(self, TR_ERRORS, "Can't bind peer ID in init");
	return XK_FAILURE;
    }
    self->shutdown = lproxyShutDown;
    self->openenable = lproxyOpenEnable;
    self->opendisable = lproxyOpenDisable;
    self->open = lproxyOpen;
    self->control = lproxyControlProtl;

/*     evSchedule(callShutDown, 0, 5 * 1000 * 1000); */


#if 0
    /* 
     * Just a test ...
     */
    kr = call_uproxy_xOpen(sendPort);
    xTrace1(lproxyp, TR_ERRORS, "call_uproxy_xOpen result: %s\n",
	    mach_error_string(kr));
    kr = call_uproxy_xGetProtlByName(sendPort, "foo", &objId);
    xTrace2(lproxyp, TR_ERRORS,
	    "call_uproxy_xGetProtlByName result: %s, obj %x\n",
	    mach_error_string(kr), (int)objId);
#endif
    
    return XK_SUCCESS;
}



xkern_return_t
lproxyPingDownTest(
    XObj	self,
    int		n)
{
    PState		*ps = (PState *)self->state;
    int			i;
    kern_return_t	kr;

    xTraceP1(self, TR_EVENTS, "run ping down test: %d times", n);
    for ( i=0; i < n; i++ ) {
	kr = call_uproxy_ping(ps->sendPort);
	xAssert( kr == KERN_SUCCESS );
    }
    return XK_SUCCESS;
}


xkern_return_t
lproxyPingUpTest(
    XObj	self,
    int		n)
{
    PState		*ps = (PState *)self->state;
    kern_return_t	kr;

    xTraceP1(self, TR_EVENTS, "run ping up test: %d times", n);
    kr = call_uproxy_pingtest(ps->sendPort, domainPort, n);
    return kr == KERN_SUCCESS ? XK_SUCCESS : XK_FAILURE;
}


