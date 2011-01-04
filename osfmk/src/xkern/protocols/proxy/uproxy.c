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
 * uproxy.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.32
 * Date: 1993/11/10 17:42:53
 */


/* 
 * Uproxy provides proxy protocols for protocols which live in other
 * protection domains and are configured above protocols in this
 * domain.  It effectively sits at the top of the protocol graph in
 * the domain where it is configured.
 *
 * Uproxy should only be instantiated once.  The actual instantiated
 * protocol object doesn't take part in many UPI operations, but it
 * creates other proxy protocol objects as necessary.
 *
 * Uproxy does not create its own sessions.  Lower sessions for which
 * proxy objects exist in other protection domains are kept in Uproxy
 * maps with reference counts representing the number of such foreign
 * proxy objects.  Uproxy and Lproxy cooperate to maintain these
 * counts. 
 */

#ifdef MACH_KERNEL
#  include <vm/vm_kern.h>
#  include <device/device_port.h>
#else
#  include <mach.h>
#  include <servers/netname.h>
#endif /* MACH_KERNEL */

#include <xkern/include/xkernel.h>
#include <xkern/framework/msg_internal.h>
#include <xkern/protocols/proxy/xk_mig_t.h>
#include <xkern/protocols/proxy/proxy_util.h>

#ifdef MACH_KERNEL
#include <uk_xkern/xk_lproxy.h>
#include <uk_xkern/xk_uproxy_server.h>
#else
#include <xkern/protocols/proxy/uproxy.h>
#include "xk_lproxy.h"
#include "xk_uproxy_server.h"
#endif /* MACH_KERNEL */
int	traceuproxyp;


xkern_return_t	uproxy_init( XObj );

static xkern_return_t	bindSession( XObj, XObj );
static XObj		findProxyXObj( PORT_TYPE, xobj_ext_id_t );
static void * 		startReader( void * );
static void		uproxyAbort( PORT_TYPE, PORT_TYPE );
static xkern_return_t	uproxyCallDemux( XObj, XObj, Msg, Msg );
static xkern_return_t	uproxyDemux( XObj, XObj, Msg );
static xkern_return_t	uproxyOpenDone( XObj, XObj, XObj, XObj, Path );
static void		watchLproxyPorts( Event, void * );


/* 
 * A collection of send rights to protection domains which have
 * references to local objects.  If one of these send rights goes bad,
 * we need to take action.
 */
typedef struct {
    PORT_TYPE	domainPort;
    Map		pathMap;	/* path ID -> send right */
} DomainInfo_s;
static	Map		foreignDomainMap;
#define DOMAIN_MAP_SZ		5
#define DOMAIN_PATH_MAP_SZ	25


/* 
 * This is the state structure for each of the proxy protocol objects
 * representing foreign protocols.  The instantiated Uproxy protocol
 * maintains its state in static variables and doesn't have one of
 * these structures attached.
 */
typedef struct {
    DomainInfo_s	*di;
    xobj_ext_id_t	peerId;
    /* 
     * mySessns is a bag of lower sessions 
     * for which this protocol is the hlpRcv.  Used to determine which
     * sessions to close when this proxy protocol is shut down.
     */
    Map			mySessns;
    /* 
     * llpMap is a bag of all llp's which this protl has openenabled. 
     */
    Map			llpMap;
} PState;

#define SESSN_MAP_SZ	11
#define LLP_MAP_SZ	1


typedef struct {
    PORT_TYPE		port;	/* The lproxy domain port */
    xobj_ext_id_t	xobjRef;
} ProxyMapKey;


/* 
 * map of external object ID's to local proxy objects
 */
static	Map		proxyMap;
#define PROXY_MAP_SZ	113

/* 
 * Map of local proxy objects.  This is used to determine whether 
 * a local protocol object is one of our proxy protocols.
 */
static	Map		revProxyMap;
#define PROXY_MAP_SZ	113


#define WATCH_PORTS_INTERVAL	(5 * 1000 * 1000)
#define UPROXY_NUM_INPUT_THREADS	5

#define UPROXY_STACK_SZ 100

static	PORT_TYPE 	rcvPort;
static	boolean_t	instantiated = FALSE;


#ifndef MACH_KERNEL

static void *
startReader(
    void	*arg)
{
    mach_msg_return_t	mr;
    ProxyThreadInfo	*info = arg;
    LingeringMsg	*lMsg = &info->lm;
    Allocator		msgAlloc;
    u_int		size = XK_MAX_MIG_MSG_LEN;

    xk_thread_checkin();
    msgAlloc = pathGetAlloc(info->path, MSG_ALLOC);
    lMsg->valid = 0;
    cthread_set_data(cthread_self(), lMsg);
    if ( allocReserve(msgAlloc, 2, &size) != XK_SUCCESS ) {
	xTrace1(uproxyp, TR_ERRORS,
		"uproxy input thread couldn't reserve buffers from path %d",
		info->path->id);
    }
    xk_thread_checkout(FALSE);
    /* 
     * This call should read request messages forever and never
     * return. 
     */
    mr = mach_msg_server(xk_uproxy_server, XK_MAX_MIG_MSG_LEN, rcvPort,
			 MACH_MSG_OPTION_NONE);
    xError("mach_msg_server(uproxy) returns!");
    if ( mr != MACH_MSG_SUCCESS ) {
	sprintf(errBuf, "mach_msg_server error: %s\n", mach_error_string(mr));
	xError(errBuf);
    }
    xk_master_lock();
    size = XK_MAX_MIG_MSG_LEN;
    allocReserve(msgAlloc, -2, &size);
    xk_master_unlock();
    return 0;
}

#endif /* ! MACH_KERNEL */


#if defined(MACH_KERNEL) && XK_DEBUG

static void
vmStatsEvent(
    Event	ev,
    void	*arg)
{
    xIfTrace(uproxyp, TR_MAJOR_EVENTS) xkDumpVmStats();
    evSchedule(ev, vmStatsEvent, arg, (u_int)arg);
}
#endif


xkern_return_t
uproxy_init(
    XObj	self)
{
    kern_return_t	kr;
    int			i;
    Event		ev;

    if ( instantiated ) {
	xError("uproxy can not be instantiated more than once!");
	return XK_FAILURE;
    } else {
	instantiated = TRUE;
    }
    xTraceP0(self, TR_GROSS_EVENTS, "init called");
#if defined(MACH_KERNEL) && XK_DEBUG
    {
	if ( ! (ev = evAlloc(self->path)) ) {
	    return XK_FAILURE;
	}
	evDetach(evSchedule(ev, vmStatsEvent, (void *)(30 * 1000 * 1000),
			    30	* 1000 * 1000));
    }
#endif
    if ( ! (ev = evAlloc(self->path)) ) {
	return XK_FAILURE;
    }
    proxyMap = mapCreate( PROXY_MAP_SZ, sizeof(ProxyMapKey), self->path );
    revProxyMap = mapCreate( PROXY_MAP_SZ, sizeof(XObj), self->path );
    foreignDomainMap = mapCreate( DOMAIN_MAP_SZ, sizeof(PORT_TYPE),
				  self->path );
    if ( ! proxyMap || ! revProxyMap || ! foreignDomainMap ) {
	return XK_FAILURE;
    }
#ifndef MACH_KERNEL
    kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
			    &rcvPort);
    if ( kr != KERN_SUCCESS ) {
	xTraceP1(self, TR_ERRORS, "mach_port_allocate error: %s\n",
		 mach_error_string(kr));
	exit(1);
    }
    kr = netname_check_in(name_server_port, self->instName, mach_task_self(),
			  rcvPort);
    if ( kr != KERN_SUCCESS ) {
	sprintf(errBuf, "upProxyReg fails (code %s)!", mach_error_string(kr));
	xError(errBuf);
	return XK_FAILURE;
    }
    xTraceP0(self, TR_EVENTS, "upProxyReg succeeds");

    xkIncreaseCthreadLimit(UPROXY_NUM_INPUT_THREADS);
    for ( i=0; i < UPROXY_NUM_INPUT_THREADS; i++ ) {
	ProxyThreadInfo	*info;

	if ( ! (info = pathAlloc(self->path, sizeof(ProxyThreadInfo))) ) {
	    return XK_NO_MEMORY;
	}
	info->path = self->path;
	cthread_fork( startReader, info );
    }

#else  /* MACH_KERNEL */
    rcvPort = ipc_port_make_send(master_device_port);
#endif /* ! MACH_KERNEL */

    evDetach(evSchedule(ev, watchLproxyPorts, 0, WATCH_PORTS_INTERVAL));
    return XK_SUCCESS;
}


kern_return_t
do_uproxy_establishPath(
    PORT_TYPE	reqPort,
    PORT_TYPE	lproxyPort,
    PORT_TYPE	pathPort,
    xk_path_t	pathId)
{
    Path		path;
    DomainInfo_s	*di;

    xk_thread_checkin();
    xTrace3(uproxyp, TR_DETAILED,
	    "establishPath called, domain %d, path %d, port %d",
	    lproxyPort, pathId, pathPort);
    if ( ! (path = getPathById(pathId)) ) {
	path = xkDefaultPath;
    }
    if ( mapResolve(foreignDomainMap, &lproxyPort, &di) != XK_SUCCESS ) {
	xTrace1(uproxyp, TR_EVENTS, "establishPath -- adding info for domain %d",
		lproxyPort);
	if ( ! (di = pathAlloc(path, sizeof(DomainInfo_s))) ) {
	    xk_thread_checkout(FALSE);
	    return KERN_FAILURE;
	}
	di->pathMap = mapCreate(DOMAIN_PATH_MAP_SZ, sizeof(xk_u_int32), path);
	if ( di->pathMap == 0 ) {
	    allocFree(di);
	    xk_thread_checkout(FALSE);
	    return KERN_FAILURE;
	}
	di->domainPort = lproxyPort;
	if ( mapBind(foreignDomainMap, &lproxyPort, di, 0) != XK_SUCCESS ) {
	    mapClose(di->pathMap);
	    allocFree(di);
	    xk_thread_checkout(FALSE);
	    return KERN_FAILURE;
	}
    }
    if ( mapBind(di->pathMap, &pathId, pathPort, 0) != XK_SUCCESS ) {
	xTrace0(uproxyp, TR_ERRORS, "uproxy couldn't bind path port");
	xk_thread_checkout(FALSE);
	return KERN_FAILURE;
    }
    xk_thread_checkout(FALSE);
    return KERN_SUCCESS;
}


/* 
 * findProxyXObj -- 
 *
 * Find the local XObject which is the proxy
 * for the indicated external XObj reference.
 */
static XObj
findProxyXObj(
    PORT_TYPE		lproxyPort,
    xobj_ext_id_t	extXObj)
{
    ProxyMapKey	key;
    XObj	localObj;

    xTrace2(uproxyp, TR_DETAILED,
	    "uproxy findProxyXObj, port == %x, extXObj == %x",
	    (int)lproxyPort, (int)extXObj);
    bzero((char *)&key, sizeof(key));
    key.port = lproxyPort;
    key.xobjRef = extXObj;
    xAssert(proxyMap);
    if ( mapResolve(proxyMap, &key, &localObj) == XK_SUCCESS ) {
	xTrace1(uproxyp, TR_MORE_EVENTS,
		"findProxyXObj found existing object %x", localObj);
	return localObj;
    }
    return ERR_XOBJ;
}      



/* 
 * Invoked when this protocol's peer's protection domain goes away. 
 */
static xkern_return_t
uproxyProtlClose(
    XObj	p)
{
    PState *ps = (PState *)p->state;

    mapClose(ps->llpMap);
    mapClose(ps->mySessns);
    xDestroy(p);
    return XK_SUCCESS;
}


static int
doClose(
    void	*key,
    void	*val,
    void	*arg)
{
    XObj	lls = *(XObj *)key;

    xTrace1(uproxyp, TR_MORE_EVENTS, "closing lls %x", (int)lls);
    xAssert(xIsValidXObj(lls));
    xClose(lls);
    return TRUE;
}


static int
doDisableAll(
    void	*key,
    void	*val,
    void	*arg)
{
    XObj	llp = (XObj)val;
    XObj	self = (XObj)arg;

    xAssert(xIsProtocol(self));
    xAssert(xIsProtocol(llp));
    xTraceP1(self, TR_EVENTS, "doing disableAll on llp %s", llp->fullName);
    xOpenDisableAll(self, llp);
    return TRUE;
}


static xkern_return_t
noop( void )
{
    xTrace0(uproxyp, TR_SOFT_ERRORS, "uproxy noop function invoked");
    return XK_FAILURE;
}


/* 
 * This is called externally, not through normal upi operations. 
 */
static void
closeProtl(
    XObj	p)
{
    PState 		*ps = (PState *)p->state;
    xkern_return_t	xkr;

    xTraceP0(p, TR_MAJOR_EVENTS, "closeProtl");
    mapForEach(ps->llpMap, doDisableAll, p);
    xTraceP1(p, TR_EVENTS, "closing sessions in sessn map %x",
	     ps->mySessns);
    mapForEach(ps->mySessns, doClose, 0);
    if ( p->rcnt > 1 ) {
	xTraceP1(p, TR_ERRORS, "rcnt (%d) > 1", p->rcnt);
    }
    xkr = mapRemoveBinding(proxyMap, p->binding);
    xAssert( xkr == XK_SUCCESS );
    xkr = mapUnbind(revProxyMap, &p);
    xAssert( xkr == XK_SUCCESS );
    ps->di = 0;
    /* 
     * This protocol object won't really go away until all sessions
     * with references to it go away.  We'll disable its functions in
     * case this doesn't happen right away.
     */
    p->calldemux = (t_calldemux)noop;
    p->demux = (t_demux)noop;
    p->opendone = (t_opendone)noop;
    xClose(p);
}


static xkern_return_t
protlInit(
    XObj	p)
{
    PState	*ps;

    if ( ! (p->state = pathAlloc(p->path, sizeof(PState))) ) {
	return XK_NO_MEMORY;
    }
    ps = (PState *)p->state;
    ps->mySessns = mapCreate(SESSN_MAP_SZ, sizeof(XObj), p->path);
    if ( ! ps->mySessns ) {
	return XK_NO_MEMORY;
    }
    ps->llpMap = mapCreate(LLP_MAP_SZ, sizeof(XObj), p->path);
    if ( ! ps->llpMap ) {
	mapClose(ps->mySessns);
	return XK_NO_MEMORY;
    }
    p->calldemux = uproxyCallDemux;
    p->demux = uproxyDemux;
    p->close = uproxyProtlClose;
    p->opendone = uproxyOpenDone;
    return XK_SUCCESS;
}
    

static xkern_return_t
addProxyXObj(
    XObj		obj,
    PORT_TYPE		port,
    xobj_ext_id_t	extXObj)
{
    ProxyMapKey	key;

    bzero((char *)&key, sizeof(key));
    key.port = port;
    key.xobjRef = extXObj;
    xAssert(proxyMap);
    if ( mapBind(proxyMap, &key, obj, &obj->binding) != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    xAssert(revProxyMap);
    if ( mapBind(revProxyMap, &obj, obj, 0) != XK_SUCCESS ) {
	mapUnbind(proxyMap, &key);
	return XK_FAILURE;
    }
    xTrace1(uproxyp, TR_EVENTS, "bound obj %x in revProxyMap",
	    (int)obj);
    return XK_SUCCESS;
}


static XObj
findProxyProtl(
    PORT_TYPE		lproxyPort,
    xobj_ext_id_t	extXObj)
{
    XObj		localObj;
    char		instName[100];
    xk_xobj_dump_t	objDump;
    kern_return_t	kr;
    xkern_return_t	xkr;
    PState		*ps;
    DomainInfo_s	*di;
    Path		path;

    xTrace2(uproxyp, TR_DETAILED,
	    "uproxy findProxyProtl, port == %x, extXObj == %x",
	    (int)lproxyPort, (int)extXObj);
    if ( (localObj = findProxyXObj(lproxyPort, extXObj)) != ERR_XOBJ ) {
	return localObj;
    }
    if ( mapResolve(foreignDomainMap, &lproxyPort, &di) != XK_SUCCESS ) {
	xTrace1(uproxyp, TR_ERRORS, "remote domain %d not registered",
		lproxyPort);
	return ERR_XOBJ;
    }
    /* 
     * Create a new proxy protocol
     */
    xTrace0(uproxyp, TR_GROSS_EVENTS, "Creating new proxy protocol object");
    xk_master_unlock();
    kr = call_lproxy_dumpXObj(lproxyPort, &xkr, extXObj, &objDump);
    xk_master_lock();
    if ( kr != KERN_SUCCESS ) {
	xTrace1(uproxyp, TR_ERRORS, "mach_msg error %d for call_dumpXObj", kr);
	return ERR_XOBJ;
    }
    if ( xkr == XK_FAILURE ) {
	xTrace1(uproxyp, TR_ERRORS, "call_dumpXObj(%x) fails", extXObj);
	return ERR_XOBJ;
    }
    /* 
     * The object might have been added while we had the lock released
     * above.  We'll check again.
     */
    if ( (localObj = findProxyXObj(lproxyPort, extXObj)) != ERR_XOBJ ) {
	return localObj;
    }
    sprintf(instName, "%s%s%s", objDump.instName,
	    (*objDump.instName == 0) ? "" : "_", "proxy");
    xTrace2(uproxyp, TR_GROSS_EVENTS, "new proxy protocol name is %s/%s",
	    objDump.name, instName);
    if ( ! (path = getPathById(objDump.pathId)) ) {
	xTrace1(uproxyp, TR_SOFT_ERRORS,
		"no corresponding path for remote path %d", objDump.pathId);
	path = xkDefaultPath;
    }
    localObj = xCreateProtl(protlInit, objDump.name, instName, &traceuproxyp,
			    0, 0, path);
    if ( localObj == ERR_XOBJ ) {
	return ERR_XOBJ;
    }
    xTrace2(uproxyp, TR_EVENTS, "binding proxy (%x) to ext protl (%x)",
	    (int)localObj, (int)extXObj);
    if ( addProxyXObj(localObj, lproxyPort, extXObj) != XK_SUCCESS ) {
	xDestroy(localObj);
	return ERR_XOBJ;
    }
    ps = (PState *)localObj->state;
    ps->peerId = extXObj;
    ps->di = di;
    return localObj;
}


/* 
 * uproxy_xDuplicate is called to explicitly indicate that an
 * additional foreign-domain proxy object has been created for the
 * indicated session.  We have to be told about the hlp because it's
 * possible for us to remove this session from our maps and then have
 * the foreign Lproxy decide to duplicate the proxy session.
 *
 * lls is a session in this domain.  hlp is an upper protocol in the
 * foreign domain.  
 */
kern_return_t
do_uproxy_xDuplicate(
    PORT_TYPE		reqPort,
    PORT_TYPE		lproxyPort,
    xkern_return_t	*retVal,
    xobj_ext_id_t	extHlp,
    xobj_ext_id_t	extLls)
{
    XObj	lls = (XObj)extLls;
    XObj	hlp;

    xTrace1(uproxyp, TR_EVENTS, "do_xDuplicate (%x) called", (int)lls);
    xk_thread_checkin();
    hlp = findProxyXObj(lproxyPort, extHlp);
    if ( xIsValidXObj(lls) && xIsValidXObj(hlp) ) {
	*retVal = bindSession(lls, hlp);
    } else {
	if ( ! xIsValidXObj(lls) ) {
	    xTrace1(uproxyp, TR_SOFT_ERRORS,
		    "uproxy_xDuplicate: lls %x is invalid", (int)lls);
	}
	if ( ! xIsValidXObj(hlp) ) {
	    xTrace2(uproxyp, TR_SOFT_ERRORS,
		    "uproxy_xDuplicate: hlp %x (proxy of %x) is invalid",
		    (int)hlp, (int)extHlp);
	}
	*retVal = XK_FAILURE;
    }
    xk_thread_checkout(FALSE);
    return KERN_SUCCESS;
}



#define RETURN_FAILURE	do { xk_thread_checkout(FALSE);	\
			  *retVal = XK_FAILURE;		\
			  return KERN_SUCCESS; } while(0)


kern_return_t
do_uproxy_xOpenEnable(
    PORT_TYPE		reqPort,
    PORT_TYPE		lproxyPort,
    xkern_return_t	*retVal,
    xobj_ext_id_t	extHlpRcv,
    xobj_ext_id_t	extHlpType,
    xobj_ext_id_t	llp,
    xk_part_t		extPart)
{
    Part_s		*p, emptyPart;
    int			numParts;
    XObj		hlpRcv, hlpType;
    PState		*ps;
    xkern_return_t	xkr;

    xTrace0(uproxyp, TR_EVENTS, "do_openEnable called");

    xk_thread_checkin();

    xTrace1(uproxyp, TR_EVENTS, "do_openEnable, reply port %x",
	    (int)lproxyPort);
    if ( (hlpRcv = findProxyProtl(lproxyPort, extHlpRcv)) == ERR_XOBJ ) {
	RETURN_FAILURE;
    }
    ps = (PState *)hlpRcv->state;
    if ( (hlpType = findProxyProtl(lproxyPort, extHlpType)) == ERR_XOBJ ) {
	RETURN_FAILURE;
    }
    if ( ! xIsValidXObj((XObj)llp) ) {
	xTrace1(uproxyp, TR_ERRORS,
		"do_xOpenEnable -- llp object %lx is unsafe", (long)llp);
	RETURN_FAILURE;
    }
    xkr = mapBind(ps->llpMap, &llp, llp, 0);
    if ( xkr != XK_SUCCESS && xkr != XK_ALREADY_BOUND ) {
	RETURN_FAILURE;
    }
    numParts = *(int *)extPart;
    if ( numParts != 0 ) {
        if ( ! (p = pathAlloc(hlpRcv->path, numParts * sizeof(Part_s))) ) {
	    RETURN_FAILURE;
	}
	partInternalize(p, extPart);
	*retVal = xOpenEnable(hlpRcv, hlpType, (XObj)llp, p);
        allocFree((char *)p);
    }
    else {
        partInit(&emptyPart, 0);
	*retVal = xOpenEnable(hlpRcv, hlpType, (XObj)llp, &emptyPart);
    }
    
    xk_thread_checkout(FALSE);

    xTrace0(uproxyp, TR_FUNCTIONAL_TRACE, "do_xOpenEnable returns");
    return KERN_SUCCESS;
}


kern_return_t
do_uproxy_xOpenDisable(
    PORT_TYPE		reqPort,
    PORT_TYPE		lproxyPort,
    xkern_return_t	*retVal,
    xobj_ext_id_t	extHlpRcv,
    xobj_ext_id_t	extHlpType,
    xobj_ext_id_t	llp,
    xk_part_t		extPart)
{
    Part_s	*p;
    int		numParts;
    XObj	hlpRcv, hlpType;

    xTrace0(uproxyp, TR_EVENTS, "do_openDisable called");

    xk_thread_checkin();

    if ( (hlpRcv = findProxyProtl(lproxyPort, extHlpRcv)) == ERR_XOBJ ) {
	RETURN_FAILURE;
    }
    if ( (hlpType = findProxyProtl(lproxyPort, extHlpType)) == ERR_XOBJ ) {
	RETURN_FAILURE;
    }
    if ( ! xIsValidXObj((XObj)llp) ) {
	xTrace1(uproxyp, TR_ERRORS,
		"do_xOpenDisable -- llp object %lx is unsafe", (long)llp);
	RETURN_FAILURE;
    }
    numParts = *(int *)extPart;
    if ( ! (p = pathAlloc(hlpRcv->path, numParts * sizeof(Part_s))) ) {
	RETURN_FAILURE;
    }
    partInternalize(p, extPart);
    *retVal = xOpenDisable(hlpRcv, hlpType, (XObj)llp, p);
    allocFree((char *)p);

    xk_thread_checkout(FALSE);

    xTrace0(uproxyp, TR_FUNCTIONAL_TRACE, "do_xOpenDisable returns");
    return KERN_SUCCESS;
}


#undef RETURN_FAILURE
#define RETURN_FAILURE {					\
    			  xk_thread_checkout(FALSE);			\
			  *retObj = (xobj_ext_id_t)ERR_XOBJ;	\
			  return KERN_SUCCESS;			\
		       }

kern_return_t
do_uproxy_xOpen(
    PORT_TYPE		reqPort,
    PORT_TYPE		lproxyPort,
    xobj_ext_id_t	*retObj,
    xobj_ext_id_t	extHlpRcv,
    xobj_ext_id_t	extHlpType,
    xobj_ext_id_t	llp,
    xk_part_t		extPart,
    xk_path_t		pathId)
{
    Part_s		*p;
    XObj		hlpRcv, hlpType;
    xkern_return_t	xkr;
    Path		path;

    xTrace0(uproxyp, TR_EVENTS, "do_open called");

    xk_thread_checkin();
    if ( ! (path = getPathById(pathId)) ) {
	path = xkDefaultPath;
    }
    if ( (hlpRcv = findProxyProtl(lproxyPort, extHlpRcv)) == ERR_XOBJ ) {
	RETURN_FAILURE;
    }
    if ( (hlpType = findProxyProtl(lproxyPort, extHlpType)) == ERR_XOBJ ) {
	RETURN_FAILURE;
    }
    if ( ! xIsValidXObj((XObj)llp) ) {
	xTrace1(uproxyp, TR_ERRORS,
		"do_xOpen -- llp object %lx is unsafe", (long)llp);
	RETURN_FAILURE;
    }
    p = pathAlloc(path, partExtLen(extPart) * sizeof(Part_s));
    if ( ! p ) {
	RETURN_FAILURE;
    }
    partInternalize(p, extPart);
    *retObj = (xobj_ext_id_t)xOpen(hlpRcv, hlpType, (XObj)llp, p, path);
    allocFree((char *)p);
    if ( xIsXObj((XObj)*retObj) ) {
	xTrace1(uproxyp, TR_FUNCTIONAL_TRACE,
		"do_xOpen got session with rcnt == %d",
		((XObj)*retObj)->rcnt);
	xkr = bindSession((XObj)*retObj, hlpRcv);
	/* 
	 * We secure our tie to this session in 'bindSession' (if it
	 * succeeded), so we don't need the extra ref count given to
	 * us automatically as a result of the xOpen.  If bindSession
	 * failed, we want to close anyway.
	 */
	xClose((XObj)*retObj);
	if ( xkr != XK_SUCCESS ) {
	    *retObj = (xobj_ext_id_t)ERR_XOBJ;
	}
    }
    xk_thread_checkout(FALSE);

    xTrace2(uproxyp, TR_FUNCTIONAL_TRACE, "do_xOpen returns %lx, rcnt == %d",
	    (long)*retObj,
	    (XObj)*retObj != ERR_XOBJ ? ((XObj)*retObj)->rcnt : 0);
    return KERN_SUCCESS;
}


/* 
 * Bind a session into our map so we can shut it down later if
 * the proxy dies.  The object is bound to a reference count representing
 * the number of foreign references not accounted for by input
 * threads.  We only actually increment the reference count of the
 * object in our address space once, indicating that it is in our map
 * and that >= 1 such foreign references exist.
 */
static xkern_return_t
bindSession(
    XObj	s,
    XObj	hlpRcv)
{
    PState		*ps = (PState *)hlpRcv->state;
    void		*countPtr;
    
    xTraceP2(hlpRcv, TR_DETAILED, "binding session (%lx) to map (%lx)",
	     (long)s, (long)ps->mySessns);
    xTraceP1(hlpRcv, TR_DETAILED, "refcnt == %d", s->rcnt);
    if ( mapResolve(ps->mySessns, &s, &countPtr) == XK_SUCCESS ) {
	xTraceP0(hlpRcv, TR_MAJOR_EVENTS,
		 "session already bound, incrementing count");
	(*(int *)countPtr)++;
	return XK_SUCCESS;
    } 
    if ( ! (countPtr = pathAlloc(hlpRcv->path, sizeof(int))) ) {
	return XK_NO_MEMORY;
    }
    *(int *)countPtr = 1;
    xDuplicate(s);
    if ( mapBind(ps->mySessns, &s, countPtr, 0) != XK_SUCCESS ) {
	allocFree(countPtr);
	return XK_FAILURE;
    }
    xTraceP1(hlpRcv, TR_DETAILED, "session (%lx) bound", (long)s);
    return XK_SUCCESS;
}


/* 
 * Decrements the bind count of lls in the protocol map.  If the count
 * is zero, the lower session is removed from the map and closed.
 */
static xkern_return_t
unbindSessn(
    XObj	lls)
{
    PState		*ps;
    void		*countPtr;
    xkern_return_t	xkr;
    
    xAssert(xIsProtocol(lls->up));
    ps = (PState *)lls->up->state;
    if ( mapResolve(ps->mySessns, &lls, &countPtr) == XK_FAILURE ) {
	xTrace1(uproxyp, TR_ERRORS,
		"failure resolving lls %lx protl map in uproxy unbindSessn",
		(long)lls);
	return XK_FAILURE;
    }
    if ( -- *(int *)countPtr == 0 ) {
	xTrace0(uproxyp, TR_EVENTS, "bind count == 0, removing");
	xTrace1(uproxyp, TR_EVENTS, "(lls ref count == %d)", lls->rcnt);
	mapUnbind(ps->mySessns, &lls);
	xkr = xClose(lls);
    }
    return XK_SUCCESS;
}



kern_return_t
do_uproxy_xGetProtlByName(
    PORT_TYPE		reqPort,
    xk_string_t		name,
    xobj_ext_id_t	*objId)
{
    XObj	obj;

    xTrace1(uproxyp, TR_EVENTS, "do_xGetProtlByName(%s) called", name);

    xk_thread_checkin();

    obj = xGetProtlByName(name);
    if ( xIsXObj(obj) ) {
	*objId = (xobj_ext_id_t)obj;
    } else {
	*objId = 0;
    }
    xTrace1(uproxyp, TR_DETAILED, "do_xGetProtlByName returns %x",
	    (int)*objId);

    xk_thread_checkout(FALSE);

    return KERN_SUCCESS;
}


static xobj_ext_id_t
peerProtl(
    XObj	p)
{
    if ( ! xIsXObj(p) || ! xIsValidXObj(p) || ! xIsProtocol(p) ) {
	return (xobj_ext_id_t)ERR_XOBJ;
    }
    xAssert(revProxyMap);
    if ( mapResolve(revProxyMap, &p, 0) == XK_FAILURE ) {
	return (xobj_ext_id_t)ERR_XOBJ;
    }
    return ((PState *)p->state)->peerId;
}
    


kern_return_t
do_uproxy_dumpXObj(
    PORT_TYPE		reqPort,
    xkern_return_t	*xkr,
    xobj_ext_id_t	extObj,
    xk_xobj_dump_t	*dump)
{
    xTrace0(uproxyp, TR_DETAILED, "do_uproxy_dumpXObj called");
    xk_thread_checkin();
    if ( ! xIsValidXObj((XObj)extObj) ) {
	xTrace1(uproxyp, TR_ERRORS, "do_dumpXObj -- xobj %x is unsafe",
		(int)extObj);
	*xkr = XK_FAILURE;
    } else {
	XObj obj = (XObj)extObj;
	xIfTrace(uproxyp, TR_DETAILED) {
	    xPrintXObj(obj);
	}
	dump->type = obj->type;
	dump->myprotl = (xobj_ext_id_t)obj->myprotl;
	if ( xIsProtocol(obj) ) {
	    dump->hlpRcv = (xobj_ext_id_t)ERR_XOBJ;
	    dump->hlpType = (xobj_ext_id_t)ERR_XOBJ;
	} else {
	    dump->hlpRcv = peerProtl(obj->up);
	    dump->hlpType = peerProtl(xHlpType(obj));
	}
	dump->pathId = obj->path->id;
	strcpy(dump->name, obj->name);
	strcpy(dump->instName, obj->instName);
	*xkr = XK_SUCCESS;
    } 
    xk_thread_checkout(FALSE);
    return KERN_SUCCESS;
}


#ifdef XK_PROXY_MSG_HACK

static void
dummyFree(
    char	*p)
{
}

#endif



kern_return_t
do_uproxy_xPush(
    PORT_TYPE 			reqPort,
    xmsg_handle_t		*handle,
    xobj_ext_id_t 		extLls,
    char	 		*msgData,
    mach_msg_type_number_t	msgDataCnt,
    char *			msgOol,
    mach_msg_type_number_t	msgOolCnt,
    xk_msg_attr_t		attr,
    mach_msg_type_number_t	attrLen,
    xk_path_t			pathId)
{
    Msg_s		msg;
    char		*msgBuf;
    XObj		lls = (XObj)extLls;
    xkern_return_t	xkr;
    Path		path;
#ifdef XK_PROXY_MSG_HACK
    MNode	        node;
#endif

    xTrace0(uproxyp, TR_EVENTS, "do_uproxy_xPush called");

    xk_thread_checkin();
    if ( ! (path = getPathById(pathId)) ) {
	path = xkDefaultPath;
    }
    if ( ! xIsValidXObj(lls) ) {
	xTrace1(uproxyp, TR_ERRORS, "do_dumpXObj -- xobj %lx is unsafe",
		(long)lls);
	*handle = XMSG_ERR_HANDLE;
    } else {

	if ( !msgOolCnt ) {
	    xTrace1(uproxyp, TR_DETAILED, "xPush constructing msg of %d bytes",
		    msgDataCnt);
#ifdef XK_PROXY_MSG_HACK
	    /* 
	     * Try to avoid copying the data by forming a message with the
	     * stack data, pushing it, and copying it only if someone else
	     * saves a reference to it during the push sequence.  We
	     * do need to pre-allocate the buffer in case of failure, though.
	     */
	    xTrace0(uproxyp, TR_DETAILED, "Doing constructInPlace hack");
	    msgBuf = allocGet(pathGetAlloc(lls->path, MSG_ALLOC), msgDataCnt);
	    if ( ! msgBuf ) {
		goto bailout;
	    }
	    xkr = msgConstructInPlace(&msg, path, msgDataCnt, 0, msgData,
				      (MsgCIPFreeFunc)dummyFree, (void *)0);
	    if ( xkr != XK_SUCCESS ) {
		allocFree(msgBuf);
		goto bailout;
	    }
	    node = msg.stack;
#else
	    xkr = msgConstructContig(&msg, path, msgDataCnt,
				     UPROXY_STACK_SZ);
	    if ( xkr != XK_SUCCESS ) {
		goto bailout;
	    }
	    msgForEach(&msg, msgDataPtr, (void *)&msgBuf);
	    bcopy(msgData, msgBuf, msgDataCnt);
#endif /* XK_PROXY_MSG_HACK */
	} else {
	    xTrace1(uproxyp, TR_DETAILED, "xPush message of %d bytes (OOL)",
		    msgOolCnt);
	    if ( oolToMsg(msgOol, path, msgOolCnt, &msg) != XK_SUCCESS ) {
		goto bailout;
	    }
	}
	if ( attrLen ) {
	    xkern_return_t	xrv;

	    xTraceP1(lls, TR_EVENTS, "attaching message attribute of len %d",
		     attrLen);
	    xrv = msgSetAttr(&msg, 0, attr, attrLen);
	    xAssert( xrv == XK_SUCCESS );
	}
	*handle = xPush(lls, &msg);
#ifdef XK_PROXY_MSG_HACK
	if ( !msgOolCnt ) {
	    if ( node->refCnt > 1 ) {
		xTrace0(uproxyp, TR_DETAILED, "Multiple stack refs, "
			"backing out of uproxy inPlace hack");
		xAssert(node->type = t_MNodeUsrPage);
		bcopy(msgData, msgBuf, msgDataCnt);
		node->b.usrpage.data = msgBuf;
		node->b.usrpage.bFree = (MsgCIPFreeFunc) allocFree;
	    } else {
		xTrace0(uproxyp, TR_DETAILED, "constructInPlace hack works");
		allocFree(msgBuf);
	    }
	}
#endif /* XK_PROXY_MSG_HACK */
	msgDestroy(&msg);
    }

    xk_thread_checkout(FALSE);
    return KERN_SUCCESS;

bailout:
    *handle = XMSG_ERR_HANDLE;
    xk_thread_checkout(FALSE);
    return KERN_SUCCESS;
}



kern_return_t
do_uproxy_xCall(
    PORT_TYPE 			reqPort,
    xkern_return_t *		xkrRet,
    xobj_ext_id_t 		llsExt,
    char *	 		reqMsgData,
    mach_msg_type_number_t 	reqMsgDataCnt,
    char *			reqOol,
    mach_msg_type_number_t	reqOolCnt,
    char *	 		repMsgData,
    mach_msg_type_number_t *	repMsgDataCnt,
    char **	 		repOol,
    mach_msg_type_number_t *	repOolCnt,
    xk_msg_attr_t		reqAttr,
    mach_msg_type_number_t	reqAttrLen,
    xk_path_t			reqPathId,
    xk_path_t *			repPathId)
{
    XObj		lls = (XObj)llsExt;
    Msg_s		reqMsg;
    Msg_s		repMsg;
    char *		reqMsgBuf;
    xkern_return_t	xkr;
#ifdef XK_PROXY_MSG_HACK
    MNode	        node;
#endif
    Path		reqPath, repPath;

    xTrace0(uproxyp, TR_EVENTS, "do_uproxy_xCall called");

    xk_thread_checkin();
    lingeringMsgClear();
    if ( ! xIsValidXObj(lls) ) {
	xTrace1(uproxyp, TR_ERRORS, "do_dumpXObj -- xobj %lx is unsafe",
		(long)lls);
	goto bailout;
    } 
    if ( ! (reqPath = getPathById(reqPathId)) ) {
	reqPath = xkDefaultPath;
    }
    if ( *repPathId == reqPathId ) {
	repPath = reqPath;
    } else {
	if ( ! (repPath = getPathById(*repPathId)) ) {
	    repPath = xkDefaultPath;
	}
    }
    if ( reqMsgDataCnt ) {

#ifdef XK_PROXY_MSG_HACK
	/* 
	 * Try to avoid copying the data by forming a message with the
	 * stack data, calling with it, and copying it only if someone else
	 * saves a reference to it during the call sequence.  We
	 * do need to pre-allocate the buffer in case of failure,
	 * though.
	 */
	xTrace0(uproxyp, TR_DETAILED, "Doing constructInPlace hack");
	reqMsgBuf = allocGet(pathGetAlloc(reqPath, MSG_ALLOC),
			     reqMsgDataCnt);
	if ( ! reqMsgBuf ) {
	    goto bailout;
	}
	xkr = msgConstructInPlace(&reqMsg, reqPath, reqMsgDataCnt, 0,
				  reqMsgData, (MsgCIPFreeFunc)dummyFree, 0);
	if ( xkr != XK_SUCCESS ) {
	    allocFree(reqMsgBuf);
	    goto bailout;
	}
	node = reqMsg.stack;

#else  /* XK_PROXY_MSG_HACK */

	xkr = msgConstructContig(&reqMsg, reqPath, reqMsgDataCnt,
				 UPROXY_STACK_SZ);
	if ( xkr != XK_SUCCESS ) {
	    goto bailout;
	}
	msgForEach(&reqMsg, msgDataPtr, (void *)&reqMsgBuf);
	bcopy(reqMsgData, reqMsgBuf, reqMsgDataCnt);
#endif /* XK_PROXY_MSG_HACK */

    } else {
	xTrace1(uproxyp, TR_DETAILED, "xCall request of %d bytes (OOL)",
		reqOolCnt);
	if ( oolToMsg(reqOol, reqPath, reqOolCnt, &reqMsg) != XK_SUCCESS ) {
	    goto bailout;
	}
    }
    if ( reqAttrLen ) {
	xkern_return_t	xrv;

	xTraceP1(lls, TR_EVENTS, "attaching message attribute of len %d",
		 reqAttrLen);
	xrv = msgSetAttr(&reqMsg, 0, reqAttr, reqAttrLen);
	xAssert( xrv == XK_SUCCESS );
    }
    msgConstructContig(&repMsg, repPath, 0, 0);
    xIfTrace(uproxyp, TR_DETAILED) {
	xTrace0(uproxyp, TR_ALWAYS, "request:");
	msgShow(&reqMsg);
    }
    *xkrRet = xCall(lls, &reqMsg, &repMsg);
    xTraceP2(lls, TR_DETAILED,
	     "xCall returns to proxy stub, xkr == %d, %d bytes",
	     *xkrRet, msgLen(&repMsg));
    xIfTrace(uproxyp, TR_DETAILED) {
	xTrace0(uproxyp, TR_ALWAYS, "reply:");
	msgShow(&repMsg);
    }

#ifdef XK_PROXY_MSG_HACK
    /* 
     * Copy the in-line request data if necessary
     */
    if ( reqMsgDataCnt ) {
	if ( node->refCnt > 1 ) {
	    xTrace0(uproxyp, TR_DETAILED,
		    "Multiple stack refs, backing out of uproxy inPlace hack");
	    xAssert(node->type = t_MNodeUsrPage);
	    bcopy(reqMsgData, reqMsgBuf, reqMsgDataCnt);
	    node->b.usrpage.data = reqMsgBuf;
	    node->b.usrpage.bFree = (MsgCIPFreeFunc) allocFree;
	} else {
	    xTrace0(uproxyp, TR_DETAILED, "constructInPlace hack works");
	    allocFree(reqMsgBuf);
	}
    }
#endif /* XK_PROXY_MSG_HACK */

    msgDestroy(&reqMsg);
    if ( msgLen(&repMsg) <= XK_INLINE_THRESHOLD ) {
	xAssert( msgLen(&repMsg) <= *repMsgDataCnt );
	xTrace0(uproxyp, TR_EVENTS, "passing reply in-line");
	msg2buf(&repMsg, repMsgData);
	*repMsgDataCnt = msgLen(&repMsg);
	*repOolCnt = 0;
    } else {
	DeallocFunc	func;
	void	*arg;

	xTrace0(uproxyp, TR_EVENTS, "passing reply out-of-line");
	*repMsgDataCnt = 0;
	if ( msgToOol(&repMsg, repOol, &func, &arg) == XK_FAILURE ) {
	    *repOolCnt = 0;
	    return XK_FAILURE;
	} else {
	    *repOolCnt = msgLen(&repMsg);
	    lingeringMsgSave(func, arg, *repOolCnt);
	}
    }
    *repPathId = repMsg.path->id;
    msgDestroy(&repMsg);
    xk_thread_checkout(FALSE);
    return KERN_SUCCESS;

bailout:    
    *xkrRet = XK_FAILURE;
    xk_thread_checkout(FALSE);
    return KERN_SUCCESS;
}


kern_return_t
do_uproxy_xControl(
    PORT_TYPE 		reqPort,
    xobj_ext_id_t 	extObj,
    int			op,
    char		*buf,
    int			*len)
{
    XObj	obj = (XObj)extObj;

    xTrace1(uproxyp, TR_ERRORS, "do_uproxy_xControl called, in length == %d",
	    *len);

    xk_thread_checkin();

    if ( ! xIsValidXObj(obj) ) {
	xTrace1(uproxyp, TR_ERRORS, "do_xControl -- xobj %lx is unsafe",
		(long)obj);
	*len = -1;
    } else {
	*len = xControl(obj, op, buf, *len);
    }
    xTrace1(uproxyp, TR_ERRORS, "do_xControl returns buffer of length %d",
	    *len);

    xk_thread_checkout(FALSE);

    return KERN_SUCCESS;
}
    


kern_return_t
do_uproxy_xClose(
    PORT_TYPE 		reqPort,
    xkern_return_t	*xkr,
    xobj_ext_id_t 	extLls)
{
    XObj	lls = (XObj)extLls;

    xTrace0(uproxyp, TR_DETAILED, "do_uproxy_xClose called");

    xk_thread_checkin();

    if ( ! xIsValidXObj(lls) ) {
	xTrace1(uproxyp, TR_ERRORS, "do_xClose -- xobj %x is unsafe", (int)lls);
	*xkr = XK_FAILURE;
    } else {
	*xkr = unbindSessn(lls);
    }

    xk_thread_checkout(FALSE);

    return KERN_SUCCESS;
}



/* 
 * Normal UPI operations (called by objects from within my own
 * protection domain)
 */


#define checkKernRes( _kr, _s, _str, _res )			\
    if ( (_kr) != KERN_SUCCESS ) {				\
	xTraceP2((_s), TR_ERRORS, "mach msg error %s in %s",	\
		 mach_error_string(_kr), (_str));		\
	return (_res);						\
    }								\


static __inline__ PORT_TYPE
findPathPort(
    Path	path,
    PState	*ps)
{
    PORT_TYPE	port;
    xk_u_int32	zero;

    if ( mapResolve(ps->di->pathMap, &path->id, &port) == XK_SUCCESS ) {
	return port;
    }
    zero = 0;
    if ( mapResolve(ps->di->pathMap, &zero, &port ) == XK_SUCCESS ) {
	return port;
    }
    xTrace1(uproxyp, TR_ERRORS,
	    "uproxy doesn't have path binding for path %d or default",
	    path->id);
    return ps->di->domainPort;
}


static xkern_return_t
uproxyOpenDone(
    XObj	self,
    XObj	lls,
    XObj	llp,
    XObj	hlpType,
    Path	path )
{
    xkern_return_t	xkr;
    kern_return_t	kr;
    PState		*ps = (PState *)self->state;

    xTraceP1(self, TR_MAJOR_EVENTS, "openDone called, lls %x", (int)lls);
    if ( bindSession(lls, self) != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    xk_thread_checkout(TRUE);
    kr = call_lproxy_xOpenDone(findPathPort(path, ps),
			       rcvPort,
			       &xkr,
			       (xobj_ext_id_t)lls,
			       (xobj_ext_id_t)llp,
			       ((PState *)self->state)->peerId,
			       ((PState *)hlpType->state)->peerId,
			       path->id);
    xk_thread_checkin();
    checkKernRes(kr, self, "opendone", XK_FAILURE);
    return xkr;
}



static xkern_return_t
uproxyDemux(
    XObj	self,
    XObj	lls,
    Msg		msg)
{
    PState		*ps = (PState *)self->state;
    xkern_return_t	xkr;
    kern_return_t	kr;
    xk_and_mach_msg_t	xmsg;
    char		*oolBuf;
    int			inlineLen, oolLen;
    DeallocFunc		oolDeallocFunc = 0;
    void		*arg;
    int			deallocMachMsg = 1;

    xTraceP0(self, TR_EVENTS, "demux called");
    xTraceP3(self, TR_FULL_TRACE,
	     "calling lproxyDemux, lls == %lx, msg == %lx, len == %d",
	     (long)lls, (long)msg, msgLen(msg));
    /* 
     * Note: The MIG stub expects us to have a MIG-sized buffer
     * allocated in xmsg.machMsg 
     */
    xmsg.machMsg = 0;
    if ( msgLen(msg) > XK_MAX_MSG_INLINE_LEN ) {
	xTraceP0(self, TR_EVENTS, "sending message out-of-line"); 
	xmsg.xkMsg = 0;
	xmsg.machMsg = allocGet(pathGetAlloc(msgGetPath(msg), MSG_ALLOC),
				XK_MAX_MIG_MSG_LEN);
	if ( ! xmsg.machMsg ) {
	    return XK_FAILURE;
	}
	if ( msgToOol(msg, &oolBuf, &oolDeallocFunc, &arg) == XK_FAILURE ) {
	    allocFree(xmsg.machMsg);
	    return XK_FAILURE;
	}
	xAssert(oolBuf);
	oolLen = msgLen(msg);
	inlineLen = 0;
    } else {
	if ( msgIsOkToMangle(msg, (char **)&xmsg.machMsg,
			     XK_DEMUX_REQ_OFFSET) ) {
	    deallocMachMsg = 0;
	    xmsg.xkMsg = 0;
	} else {
	    xmsg.xkMsg = msg;
	    /* 
	     * We have to dynamically allocate the message because the
	     * in-kernel stack size is quite small and putting the MIG message
	     * on the stack causes overflow problems.
	     */
	    xmsg.machMsg = allocGet(pathGetAlloc(msgGetPath(msg), MSG_ALLOC),
				    XK_MAX_MIG_MSG_LEN);
	    if ( ! xmsg.machMsg ) {
		return XK_FAILURE;
	    }
	}
	oolBuf = 0;
	oolLen = 0;
	inlineLen = msgLen(msg);
    }
    xAssert(&xmsg.machMsg);
    xk_thread_checkout(TRUE);
    kr = call_lproxy_xDemux(
		findPathPort(msgGetPath(msg), ps),
		rcvPort,
		&xkr,
		(xobj_ext_id_t)lls,
		(char *)&xmsg,
		inlineLen,
		0,
		oolBuf,
		oolLen,
		msgGetAttr(msg, 0),
		msg->attrLen);
    xk_thread_checkin();
    if ( deallocMachMsg ) {
	allocFree(xmsg.machMsg);
    }
    if ( oolDeallocFunc ) {
	oolDeallocFunc(arg, oolLen);
    }
    checkKernRes(kr, self, "demux", XK_FAILURE);
    xTraceP1(self, TR_MORE_EVENTS, "demux returns %x", xkr);

    return xkr;
}



static xkern_return_t
uproxyCallDemux(
    XObj	self,
    XObj	lls,
    Msg		reqMsg,
    Msg		repMsg)
{
    PState		*ps = (PState *)self->state;
    xkern_return_t	xkr = XK_FAILURE;
    kern_return_t	kr;
    xk_and_mach_msg_t	req;
    Msg_s		tmpMsg;
    mach_msg_type_number_t	repMsgLen, reqLen;
    char		*buf;
    char		*reqOol = 0; /* the actual memory object to send */
    				     /* (same as reqOolBuf if out-of-kernel) */
    DeallocFunc		oolDeallocFunc;
    void		*arg;
    char		*repOol;
    mach_msg_type_number_t	repOolLen;
    
    xTraceP0(self, TR_EVENTS, "demux called");
    xTraceP3(self, TR_FULL_TRACE,
	     "calling lproxyDemux, lls == %lx, msg == %lx, len == %d",
	     (long)lls, (long)reqMsg, msgLen(reqMsg));
    xIfTraceP(self, TR_DETAILED) msgShow(reqMsg);
    /* 
     * If req.xkMsg is non-zero, the stub will copy the data from the
     * x-kernel message into the mach message.  If req.xkMsg is null,
     * the copy doesn't happen (because we have built the mach message
     * around the existing x-kernel message)
     */
    repMsgLen = XK_MAX_MSG_INLINE_LEN;
    reqLen = msgLen(reqMsg);
    if ( reqLen > XK_INLINE_THRESHOLD ) {
	xTraceP0(self, TR_EVENTS, "sending request out-of-line"); 
	req.xkMsg = 0;
	if ( msgToOol(reqMsg, &reqOol, &oolDeallocFunc, &arg) == XK_FAILURE ) {
	    return XK_FAILURE;
	}
	xAssert(reqOol);
    } else {
	xTraceP0(self, TR_EVENTS, "sending request in-line"); 
	req.xkMsg = reqMsg;
	xAssert(reqOol == 0);
    }
    /* 
     * We have to dynamically allocate the message because the
     * in-kernel stack size is quite small and putting the MIG message
     * on the stack causes overflow problems.
     */
    xkr = msgConstructContig(&tmpMsg, msgGetPath(reqMsg),
			     XK_MAX_MIG_MSG_LEN, 0);
    if ( xkr != XK_SUCCESS ) {
	return XK_FAILURE;
    }
    msgForEach(&tmpMsg, msgDataPtr, (void *)&buf);
    xAssert(buf);
    req.machMsg = buf;

    xk_thread_checkout(TRUE);
    kr = call_lproxy_xCallDemux(
		findPathPort(msgGetPath(reqMsg), ps),
		rcvPort,
		&xkr,
		(xobj_ext_id_t)lls,
		(char *)&req,
		reqOol ? 0 : reqLen,
		0,
		reqOol,
		reqOol ? reqLen : 0,
		0,
		&repMsgLen,
		&repOol,
		&repOolLen,
		msgGetAttr(reqMsg, 0),
		reqMsg->attrLen);
    xk_thread_checkin();
    if ( reqLen > XK_INLINE_THRESHOLD ) {
	if ( oolDeallocFunc ) {
	    oolDeallocFunc(arg, reqLen);
	}
    }
    checkKernRes(kr, self, "calldemux", XK_FAILURE);
    xTraceP1(self, TR_MORE_EVENTS, "calldemux returns %d", xkr);
    if ( xkr == XK_SUCCESS ) {
	if ( repOolLen > 0 ) {
	    Path	p = msgGetPath(repMsg);

	    xTraceP1(self, TR_MORE_EVENTS, "out-of-line callDemux reply, len %d",
		     repOolLen);
	    msgDestroy(repMsg);
	    if ( oolToMsg(repOol, p, repOolLen, repMsg) != XK_SUCCESS ) {
		msgConstructContig(repMsg, p, 0, 0);
		xkr = XK_FAILURE;
	    }
	} else {
	    xTraceP1(self, TR_MORE_EVENTS, "inline reply msg len == %d",
		     repMsgLen);
	    xAssert(msgLen(&tmpMsg) >= repMsgLen);
	    msgPopDiscard(&tmpMsg, XK_CALL_REP_OFFSET); 
	    msgTruncate(&tmpMsg, repMsgLen);
	    msgAssign(repMsg, &tmpMsg);
	}
    }
    xIfTraceP(self, TR_DETAILED) {
	if ( xkr == XK_SUCCESS ) {
	    xTraceP0(self, TR_ALWAYS, "Reply: ");
	    msgShow(repMsg); 
	}
    }
    msgDestroy(&tmpMsg);
    return xkr;
}




#ifdef MACH_KERNEL


static int
portActive(
    PORT_TYPE	port)
{
    return ip_active(port);
}

#else /* ! MACH_KERNEL */

/* 
 * We should really do port-death notification ...  But this does let us
 * simulate the same sort of thing that the kernel does.  Though that
 * can probably be improved, too.
 */
static int
portActive(
    PORT_TYPE	port)
{
    kern_return_t	kr;

    kr = call_lproxy_ping(port);
    return kr == XK_SUCCESS;
}

#endif /* MACH_KERNEL */


static int
checkRights(
    void	*key,
    void	*val,
    void	*arg)
{
    PORT_TYPE		port = *(PORT_TYPE *)key;

    xTrace1(uproxyp, TR_DETAILED, "checking validity of port %x", port);
    if ( portActive(port) ) {
	xTrace0(uproxyp, TR_FULL_TRACE, "port is active");
    } else {
	xTrace0(uproxyp, TR_EVENTS, "port is inactive");
	uproxyAbort(0, port);
    }
    return MFE_CONTINUE;
}



static void
watchLproxyPorts(
    Event	ev,
    void	*arg)
{
    xTrace0(uproxyp, TR_EVENTS, "watchLproxyPorts runs");
    mapForEach(foreignDomainMap, checkRights, 0);
    evSchedule(ev, watchLproxyPorts, 0, WATCH_PORTS_INTERVAL);
}



static int
closeProxyProtls(
    void	*mapKey,
    void	*val,
    void	*arg)
{
    PORT_TYPE	dyingPort = *(PORT_TYPE *)arg;
    ProxyMapKey	*key = (ProxyMapKey *)mapKey;

    if ( dyingPort == key->port ) {
	xTrace1(uproxyp, TR_EVENTS, "closing proxy protl %x", (int)val);
	xAssert(xIsXObj((XObj)val));
	closeProtl((XObj)val);
    } else {
	xTrace2(uproxyp, TR_DETAILED,
		"[%s] port mismatch with proxy protl %x, not closing",
		((XObj)val)->fullName, (int)val);
	xTrace2(uproxyp, TR_DETAILED,
		"(dying port == %x, saved port == %x)", dyingPort, key->port);
    }
    return TRUE;
}



static void
uproxyAbort(
    PORT_TYPE	reqPort,
    PORT_TYPE	lproxyPort)
{
    xkern_return_t	xkr;
    DomainInfo_s	*di;
    
    xTrace0(uproxyp, TR_MAJOR_EVENTS, "uproxyAbort called");
    if ( mapResolve(foreignDomainMap, &lproxyPort, &di) == XK_FAILURE ) {
	xTrace1(uproxyp, TR_MAJOR_EVENTS,
		"uproxyAbort already called with port %x",
		(int)lproxyPort);
	return;
    } 
    xkr = mapUnbind(foreignDomainMap, &lproxyPort);
    xAssert( xkr == XK_SUCCESS );
    mapForEach(proxyMap, closeProxyProtls, &lproxyPort);
    mapClose(di->pathMap);
    allocFree(di);
}


kern_return_t
do_uproxy_abort(
    PORT_TYPE	reqPort,
    PORT_TYPE	lproxyPort)
{
    xk_thread_checkin();
    uproxyAbort(reqPort, lproxyPort);
    xk_thread_checkout(FALSE);
    return KERN_SUCCESS;
}


kern_return_t
do_uproxy_ping(
    PORT_TYPE		reqPort)
{
    xTrace0(uproxyp, TR_EVENTS, "uproxy Ping");
    return KERN_SUCCESS;
}



kern_return_t
do_uproxy_pingtest(
    PORT_TYPE		reqPort,
    PORT_TYPE		lproxyPort,
    int			n)
{
    kern_return_t	kr;
    int			i;

    for ( i=0; i < n; i++ ) {
	kr = call_lproxy_ping(lproxyPort);
	xAssert( kr == KERN_SUCCESS ) ;
    }
    return KERN_SUCCESS;
}


