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
 * upi.c,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * upi.c,v
 * Revision 1.63.1.5.1.2  1994/09/07  04:18:17  menze
 * OSF modifications
 *
 * Revision 1.63.1.5  1994/09/02  21:47:07  menze
 * mapBind and mapVarBind return xkern_return_t
 *
 * Revision 1.63.1.4  1994/09/01  18:49:59  menze
 * Meta-data allocations now use Allocators and Paths
 * Subsystem initialization functions can fail
 *
 * Revision 1.63.1.3  1994/07/22  19:41:52  menze
 * XObjects store Paths rather than Allocators
 * xOpen and xOpenDone take Path arguments
 *
 * Revision 1.63.1.2  1994/04/14  21:28:48  menze
 * xCreateProtl now takes an allocator argument
 *
 * Revision 1.63.1.1  1994/04/01  17:00:24  menze
 * XObject allocator is initialized to "default".  This is temporary.
 *
 * Revision 1.63  1994/02/05  00:08:03  menze
 *   [ 1994/01/30          menze ]
 *   assert.h renamed to xk_assert.h
 *   No longer includes varargs.h directly
 *
 * Revision 1.62  1994/01/25  03:19:29  davidm
 * Removed #include "process.h" (it's included via platform.h already).
 *
 * Revision 1.61  1993/12/13  20:24:19  menze
 * Modifications from UMass:
 *
 *   [ 93/12/13          menze ]
 *   Previous trace deadlock modifications weren't getting reply lengths
 *   right.
 *
 *   [ 93/06/21          nahum ]
 *   Fixed trace deadlock by removing msgLen() calls from within xTrace macros.
 *
 *   [ 93/06/09          nahum ]
 *   Fixed some IRIX build bugs.
 *
 */

#include <xkern/include/domain.h>
#include <xkern/include/upi.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/assert.h>
#include <xkern/include/prottbl.h>
#include <xkern/include/platform.h>
#include <xkern/include/event.h>
#include <xkern/include/xk_alloc.h>
#include <xkern/include/xk_path.h>

#define UPI_PROTL_MAP_SZ	 64
#define UPI_SAFEOBJ_MAP_SZ	512

XObj		xNullProtl;
static Map	protlMap;
static Map	safeObjMap;


#ifdef XK_DEBUG
static char *controlops[] = {
  "getmyhost",
  "getmyhostcount",
  "getpeerhost",
  "getpeerhostcount",
  "getbcasthost",
  "getmaxpacket",
  "getoptpacket",
  "getmyproto",
  "getpeerproto",
  "resolve",
  "rresolve",
  "freeresources",
  "getparticipants",
  "setnonblockingio",
  "getrcvhost",
  "addmulti",
  "delmulti",
  "getmaster",
  "getgroupid",
  "setincarnation",
  "checkincarnation"
};

#define CONTROLMSG(n) ((n) < sizeof controlops / sizeof(char *) ? \
	controlops[n] : "non-standard")
#else
#define CONTROLMSG(n) ""
#endif /* XK_DEBUG */   

static xkern_return_t	xDestroyXObj(XObj s);
static XObj		defaultOpen( XObj, XObj, XObj, Part, Path );
static int		findProtl( void *, void *, void * );
static int		noop( void );
static xkern_return_t	returnFailure( void );
static int		shutdownProtls( void *, void *, void * );
static XObj 		xCreateXObj(int downc, XObj *downv, Path);
static xkern_return_t 	xDestroyXObj(XObj s);

/* 
 * If inline functions are not being used, the upi_inline functions
 * will be instantiated here.
 */

#define UPI_INLINE_INSTANTIATE

#include <xkern/include/upi_inline.h>


/*************************************************
 * Uniform Protocol Interface Operations
 *************************************************/

XObj
xOpen(
    XObj	hlpRcv,
    XObj	hlpType,
    XObj	llp,
    Part	participants,
    Path	path  )
{
    XObj s;
    
    xAssert(xIsProtocol(llp));
    xAssert(xIsProtocol(hlpType));
    xAssert(xIsProtocol(hlpRcv));
    xTrace3(protocol, TR_MAJOR_EVENTS,
	"Calling open[%s] by (%s,%s)",
	llp->fullName, hlpRcv->fullName, hlpType->fullName);
    s = (XObj)(*(llp->open))(llp, hlpRcv, hlpType, participants, path);
    if (xIsXObj(s)) {
	s->rcnt++;
    }
    xTrace5(protocol, TR_MAJOR_EVENTS,
	"Open[%s] by (%s,%s) returns %x (rcnt == %d)",
	llp->fullName, hlpRcv->fullName, hlpType->fullName, s,
	xIsXObj(s) ? s->rcnt : 0);
    return s;
}


xkern_return_t
xOpenEnable(
    XObj	hlpRcv,
    XObj	hlpType,
    XObj	llp,
    Part	p)
{
    xAssert(xIsProtocol(llp));
    xAssert(xIsXObj(hlpType));
    xAssert(xIsXObj(hlpRcv));
    xTrace3(protocol, TR_MAJOR_EVENTS,
	"Calling openenable[%s] by (%s,%s)",
	llp->fullName, hlpRcv->fullName, hlpType->fullName);
    return (*(llp->openenable))(llp, hlpRcv, hlpType, p);
}


xkern_return_t
xOpenDone(
    XObj	hlp,
    XObj	s,
    XObj	llp,
    Path	path)
{
    register t_opendone	od;
    
    xAssert(xIsXObj(s));
    xAssert(xIsXObj(hlp));
    xAssert(xIsXObj(llp));
    xIfTrace(protocol, TR_MAJOR_EVENTS) {
	if ( s->up != hlp ) {
	    xTrace4(protocol, TR_MAJOR_EVENTS,
		"hlpRcv of session %x(%s) becomes %x(%s)",
		s, s->fullName, hlp, hlp->fullName);
	  }
    }
    s->up = hlp;
    if (!(od = hlp->opendone)) return XK_SUCCESS;
    xTrace2(protocol, TR_MAJOR_EVENTS,
	"Calling opendone[%s] by %s", hlp->fullName, llp->fullName);
    xTrace4(protocol, TR_FUNCTIONAL_TRACE,
	"hlp == %x, lls == %x, llp == %x, hlpType == %x",
	(int)hlp, (int)s, (int)llp, (int)s->hlpType);
    return (*od)(hlp, s, llp, s->hlpType, path);
}


xkern_return_t
xCloseDone(
    XObj	s)
{
    register t_closedone	cd;

    xAssert(xIsXObj(s));
    if (!(cd = s->up->closedone)) return XK_SUCCESS;
    xTrace2(protocol, TR_MAJOR_EVENTS,
	"Calling closedone[%s] by %s", s->up->fullName, s->myprotl->fullName);
    return (*cd)(s);
}


xkern_return_t
xOpenDisable(
    XObj	hlpRcv,
    XObj	hlpType,
    XObj	llp,
    Part	participants)
{
    xAssert(xIsProtocol(llp));
    xAssert(xIsXObj(hlpRcv));
    xAssert(xIsXObj(hlpType));
    xTrace3(protocol, TR_MAJOR_EVENTS, "Calling opendisable[%s] by (%s,%s)",
	    llp->fullName, hlpRcv->fullName, hlpType->fullName);
    return (*(llp->opendisable))(llp, hlpRcv, hlpType, participants);
}


xkern_return_t
xOpenDisableAll(
    XObj	hlpRcv,
    XObj	llp)
{
    xAssert(xIsProtocol(llp));
    xAssert(xIsXObj(hlpRcv));
    xTrace2(protocol, TR_MAJOR_EVENTS, "Calling openDisableAll[%s] by (%s)",
	    llp->fullName, hlpRcv->fullName);
    return (*(llp->opendisableall))(llp, hlpRcv);
}


xkern_return_t
xClose(
    XObj	s)
{
    xTrace0(protocol, TR_EVENTS, "xClose: entered");
    /*
     * DEC_REF_COUNT_UNCOND comes from upi_inline.h
     */
    DEC_REF_COUNT_UNCOND(s, "xClose");
    xTrace0(protocol, TR_FULL_TRACE, "xClose returning");
    return XK_SUCCESS;
}


xkern_return_t
xDuplicate(
    XObj	s)
{
    xTrace1(protocol, TR_EVENTS, "calling duplicate[%s]", s->myprotl->fullName);
    return (*s->duplicate)(s);
}


static xkern_return_t
defaultDuplicate(
    XObj	s)
{
    s->rcnt++;
    return XK_SUCCESS;
}


static XObj
defaultOpen(
    XObj	hlpRcv,
    XObj	hlpType,
    XObj	llp,
    Part	p,
    Path	path)
{
    return ERR_XOBJ;
}


static xkern_return_t
returnFailure()
{
    xTrace0(protocol, TR_SOFT_ERRORS, "default UPI operation is invoked");
    return XK_FAILURE;
}

/*
 * xDemux, xCallDemux, xPush and xCall are defined as macros in upi.h when
 * optimized
 */

#if	XK_DEBUG && !defined(XK_UPI_MACROS)

xkern_return_t
xCall(
    XObj	s,
    Msg		msg,
    Msg		replyMsg)
{
    xkern_return_t	retVal;
    
    xAssert(xIsXObj(s));
    xTrace3(protocol, TR_EVENTS,
	"Calling call[%s] by %s, %d bytes",
	s->myprotl->fullName, s->up->fullName, msgLen(msg));
    xIfTrace(protocol, TR_FUNCTIONAL_TRACE) {
	xTrace0(protocol, TR_ALWAYS, "       Message:");
	msgShow(msg);
    }
    retVal = (*s->call)(s, msg, replyMsg);
    xTrace3(protocol, TR_EVENTS,
	"call[%s] returns %d bytes in reply to %s",
	s->myprotl->fullName, msgLen(replyMsg), s->up->fullName);
    return retVal;
}


xmsg_handle_t
xPush(
    XObj	s,
    Msg		msg)
{
    int		retVal;
    
    EV_CHECK_STACK("xPush");
    xAssert(xIsXObj(s));
    xTrace3(protocol, TR_EVENTS,
	"Calling push[%s] by %s, %d bytes",
	s->myprotl->fullName, s->up->fullName, msgLen(msg));
    xIfTrace(protocol, TR_FUNCTIONAL_TRACE) {
	xTrace0(protocol, TR_ALWAYS, "       Message:");
	msgShow(msg);
    }
    retVal = (*s->push)(s, msg);
    xTrace3(protocol, TR_EVENTS,
	"push[%s] by %s returns %d",
	s->myprotl->fullName, s->up->fullName, retVal);
    return retVal;
}


xkern_return_t
xDemux(
    XObj	s,
    Msg		msg)
{
    register t_demux	demux;

    xAssert(xIsXObj(s));
    xAssert(xIsXObj(s->up));
    xTrace4(protocol, TR_EVENTS,
	"Calling demux[%s(%x)] by %s, %d bytes",
	s->up->fullName, s->up, s->myprotl->fullName, msgLen(msg));
    xIfTrace(protocol, TR_FUNCTIONAL_TRACE) {
	xTrace0(protocol, TR_ALWAYS, "       Message:");
	msgShow(msg);
    }
    if (!(demux = s->up->demux)) return XK_SUCCESS;
    return (*demux)(s->up, s, msg);
}


xkern_return_t
xCallDemux(
    XObj	s,
    Msg		msg,
    Msg		replyMsg)
{
    register t_calldemux calldemux;
    xkern_return_t	retVal;

    xAssert(xIsXObj(s));
    xTrace3(protocol, TR_EVENTS,
	"Calling calldemux[%s] by %s, %d bytes",
	s->up->fullName, s->myprotl->fullName, msgLen(msg));
    xIfTrace(protocol, TR_FUNCTIONAL_TRACE) {
	xTrace0(protocol, TR_ALWAYS, "       Message:");
	msgShow(msg);
    }
    if (!(calldemux = s->up->calldemux)) return XK_SUCCESS;
    retVal = (*calldemux)(s->up, s, msg, replyMsg);
    xTrace2(protocol, TR_EVENTS,
	"calldemux[%s] returns %d bytes", s->up->fullName, msgLen(replyMsg));
    return retVal;
}

#endif /* XK_DEBUG && !defined(XK_UPI_MACROS) */   


int
xControl(
    XObj	s,
    int		opcode,
    char	*buf,
    int		len)
{
  register t_control c;

  int res;
  if (! (c = s->control)) {
    return 0;
  }
  xTrace3(protocol, TR_EVENTS, "Calling control[%s] op %s (%d)",
	 s->myprotl->fullName, CONTROLMSG(opcode), opcode);
  res = (*c)(s, opcode, buf, len);
  xTrace4(protocol, TR_EVENTS, "Control[%s] op %s (%d) returns %d",
	s->myprotl->fullName, CONTROLMSG(opcode), opcode, res);
  return res;
}


xkern_return_t
xShutDown(
    XObj	obj)
{
    xAssert(xIsXObj(obj));
    xTrace2(protocol, TR_MAJOR_EVENTS, "Calling shutdown[%s(%x)]",
	    obj->fullName, obj);
    return obj->shutdown(obj);
}


/*************************************************
 * Create and Destroy XObj's Sessions and Protocols 
 *************************************************/

static int
noop()
{
    return 0;
}


XObj
xCreateSessn(
    XObjInitFunc f,
    XObj 	hlpRcv,
    XObj 	hlpType,
    XObj 	llp,
    int		downc,
    XObj	*downv,
    Path	path)
{
    XObj s;
    
    xTrace3(protocol, TR_MAJOR_EVENTS, "xCreateSession:[%s] by [%s,%s]",
	    llp->fullName, hlpRcv->fullName, hlpType->fullName);
    if ((s = xCreateXObj(downc, downv, path)) == ERR_XOBJ) {
	xTrace0(protocol, TR_ERRORS, "CreateSessn failed at CreateXObj");
	return s;
    }
    s->type = Session;
    s->rcnt = 0;
    s->name = llp->name;
    s->instName = llp->instName;
    s->fullName = llp->fullName;
    s->myprotl = llp;
    s->id = 0;			/* only relevant to protocols */
    s->up = hlpRcv;
    s->hlpType = hlpType;
    s->traceVar = llp->traceVar;
    s->path = path ? path : xkDefaultPath;
    llp->rcnt++;
    /*
     * hlpRcv->rcnt++; 
     */
    if (f) {
	if ( (*f)(s) == XK_FAILURE ) {
	    xTrace1(protocol, TR_SOFT_ERRORS,
		    "%s sessn initialization function returns failure",
		    llp->fullName);
	    xDestroyXObj(s);
	    return ERR_XOBJ;
	}
    }
    xTrace1(protocol, TR_EVENTS, "xCreateSession returns %x", s);
    return s;
}


		     

XObj
xCreateProtl(
    XObjInitFunc f,
    char	*name,
    char	*instName,
    int		*tracePtr,
    int		downc,
    XObj	*downv,
    Path	path)
{
  XObj s;
  int  id;

  xTrace1(protocol, TR_MAJOR_EVENTS, "xCreateProtocol:[%s]", name);
  if ( (id = protTblGetId(name)) == -1 ) {
    xTrace1(protocol, TR_ERRORS, "CreateProtl could not find protocol id for %s", name);
      return ERR_XOBJ;
  }
  if ( ! path ) {
      xTrace1(protocol, TR_ERRORS, "bad path specified for %s", name);
      return ERR_XOBJ;
  }
  if ((s = xCreateXObj(downc, downv, path)) == ERR_XOBJ) {
    xTrace0(protocol, TR_ERRORS, "CreateProtl failed at CreateXObj");
    return ERR_XOBJ;
  }
  s->close = (t_close)noop;
  s->rcnt = 1;
  s->traceVar = tracePtr;
  if ( ! (s->name = pathAlloc(path, strlen(name) + 1)) ) {
      xDestroyXObj(s);
      return ERR_XOBJ;
  }
  strcpy(s->name, name);
  if ( ! (s->instName = pathAlloc(path, strlen(instName) + 1)) ) {
      pathFree(s->name);
      xDestroyXObj(s);
      return ERR_XOBJ;
  }
  strcpy(s->instName, instName);
  if ( ! (s->fullName = pathAlloc(path,
				   strlen(name) + strlen(instName) + 2)) ) {
      pathFree(s->name);
      pathFree(s->instName);
      xDestroyXObj(s);
      return ERR_XOBJ;
  }
  sprintf(s->fullName, "%s%c%s", name, (*instName ? '/' : '\0'), instName);
  s->myprotl = s;
  s->id = id;
  s->up = ERR_XOBJ;		/* only relevant to sessions */
  s->path = (path ? path : xkDefaultPath);
  if ( mapBind(protlMap, &s, s, 0) != XK_SUCCESS ) {
      xTrace0(protocol, TR_SOFT_ERRORS, "error binding in protocol map");
      pathFree(s->name);
      pathFree(s->instName);
      xDestroyXObj(s);
      return ERR_XOBJ;
  }
  s->type = Protocol;
  if (f) {
      if ( (*f)(s) == XK_FAILURE ) {
	  xTrace1(protocol, TR_SOFT_ERRORS,
		  "%s protl initialization function returns failure", name);
	  pathFree(s->name);
	  pathFree(s->instName);
	  xDestroyXObj(s);
	  return ERR_XOBJ;
      }
  }
  return s;
}

static XObj
xCreateXObj(
    int		downc,
    XObj	*downv,
    Path	path)
{
  XObj s;
  XObj *dv;
  int  i;

  xAssert(path);
  s = (struct xobj *) pathAlloc(path, sizeof(struct xobj));
  if (! s) {
    xTrace0(protocol, TR_ERRORS, "xCreateObj alloc failure(1)");
    return ERR_XOBJ;
  }
  bzero((char *)s, sizeof(struct xobj));
  s->numdown = downc;
  if (downc > STD_DOWN) {
    s->downlistsz = (((downc - STD_DOWN) / STD_DOWN) + 1) * STD_DOWN;
    s->downlist = (XObj *) pathAlloc(path, s->downlistsz * sizeof(XObj));
    if (! s->downlist) {
      pathFree((char *)s);
      xTrace0(protocol, TR_ERRORS, "xCreateObj alloc failure(2)");
      return ERR_XOBJ;
    }
  }
  dv = s->down;
  for (i=0; i < downc; i++, dv++) {
    if (i == STD_DOWN) {
      dv = s->downlist;
    }
    *dv = downv[i];
  }
  s->idle = FALSE;
  s->open = defaultOpen;
  s->close = (t_close)returnFailure;
  s->closedone = (t_closedone)noop;
  s->openenable = (t_openenable)returnFailure;
  s->opendisable = (t_opendisable)returnFailure;
  s->opendisableall = (t_opendisableall)noop;
  s->opendone = (t_opendone)noop;
  s->demux = (t_demux)returnFailure;
  s->calldemux = (t_calldemux)returnFailure;
  s->callpop = (t_callpop)returnFailure;
  s->pop = (t_pop)returnFailure;
  s->push = (t_push)returnFailure;
  s->call = (t_call)returnFailure;
  s->control = (t_control)returnFailure;
  s->shutdown = (t_shutdown)noop;
  s->duplicate = (t_duplicate)defaultDuplicate;
  if ( mapBind(safeObjMap, &s, s, 0) != XK_SUCCESS ) {
      xTrace0(protocol, TR_SOFT_ERRORS, "Error binding in safe object map");
      if ( s->downlist ) {
	  pathFree(s->downlist);
      }
      pathFree(s);
      return ERR_XOBJ;
  }
  return s;
}


static xkern_return_t
xDestroyXObj(
    XObj	s)
{
    xkern_return_t	xkr;

    xkr = mapUnbind(safeObjMap, &s);
    xAssert(xkr == XK_SUCCESS);
    if (s->type == Protocol) {
	xkr = mapUnbind(protlMap, &s);
	xAssert(xkr == XK_SUCCESS);
    }
    if (s->state) {
	pathFree(s->state);
    }
    if ( s->downlist ) {
	pathFree(s->downlist);
    }
    pathFree(s);
    return XK_SUCCESS;
}


xkern_return_t
xDestroy(
    XObj	s)
{
    xTrace2(protocol, TR_EVENTS, "xDestroy[%s(%x)]", s->fullName, s);
    if ( s->type == Session ) {
	xClose(s->myprotl);
    } 
    return xDestroyXObj(s);
}



/*************************************************
 * Utility Routines
 *************************************************/

typedef struct {
    char	*name;
    XObj	protl;
} FindProtlArg;


static int
findProtl(
    void	*key,
    void	*val,
    void	*arg)
{
    XObj		p = (XObj)val;
    FindProtlArg	*a = (FindProtlArg *)arg;

    /*
     * getProtlByName -- return a protocol matching the
     * indicated protocol class (e.g., "tcp", "udp") rather than
     * looking at the instance name at all.  If there are multiple
     * instances a protocol will be returned at random.
     */
    xAssert( xIsProtocol(p) );
    if ( ! strcmp(p->name, a->name) ) {
	a->protl = p;
	return 0;
    } else {
	return MFE_CONTINUE;
    }
}


XObj
xGetProtlByName(
    char	*name)
{
    FindProtlArg	arg;

    arg.name = name;
    arg.protl = ERR_XOBJ;
    mapForEach(protlMap, findProtl, &arg);
    return arg.protl;
}


xkern_return_t
xSetDown(
    XObj	s,
    int		i,
    XObj	obj)
{
    XObj *newdl;
    int  newsz, n;
    
    if (i < STD_DOWN) {
	s->down[i] = obj;
    } else {
	n = i - STD_DOWN;
	if (n >= s->downlistsz) {
	    /* 
	     * Make newsz the smallest sufficient multiple of STD_DOWN
	     */
	    newsz = ((n / STD_DOWN) + 1) * STD_DOWN;
	    newdl = (XObj *) pathAlloc(obj->path, newsz * sizeof(XObj));
	    if ( newdl == 0 ) {
		return XK_FAILURE;
	    }
	    if ( s->downlist ) {
		bcopy((char *)s->downlist, (char *)newdl,
		      s->downlistsz * sizeof(XObj));
		pathFree(s->downlist);
	    }
	    s->downlist = newdl;
	    s->downlistsz = newsz;
	}
	s->downlist[n] = obj;
    }
    if (i + 1 > s->numdown) {
	s->numdown = i + 1;
    }
    return XK_SUCCESS;
}

/*************************************************
 * Miscellaneous Routines
 *************************************************/

boolean_t
xIsValidXObj(
    XObj	obj)
{
    return (mapResolve(safeObjMap, &obj, 0) == XK_SUCCESS);
}


void
xPrintXObj(
    XObj	p)
{
    int 	i;
    
    if ( !p ) {
	xTrace0(protocol, TR_ALWAYS, "XOBJ NULL");
	return;
    }
    xTrace2(protocol, TR_ALWAYS, "XOBJ %x %s", p, p->fullName);
    xTrace1(protocol, TR_ALWAYS, "type = %s",
	    p->type == Protocol ? "Protocol" : "Session");
    xTrace4(protocol, TR_ALWAYS, "myprotocol = %x %s %s %s",
	    p->myprotl, p->myprotl->name, p->myprotl->instName,
	    p->myprotl->fullName);
    xTrace1(protocol, TR_ALWAYS, "id = %d", p->id);
    xTrace1(protocol, TR_ALWAYS, "rcnt = %d", p->rcnt);
    xTrace1(protocol, TR_ALWAYS, "up = %x ", p->up);
    if ( xIsXObj(p->up) ) {
	xTrace1(protocol, TR_ALWAYS, "up->type = %s ",
		p->type == Protocol ? "Protocol" : "Session");
	xTrace1(protocol, TR_ALWAYS, "up->fullName = %s\n", p->up->fullName);
    }
    xTrace1(protocol, TR_ALWAYS, "numdown = %d ", p->numdown);
    for ( i=0; i < p->numdown; i++ ) {
	xTrace2(protocol, TR_ALWAYS, "down[%d] = %x", i, xGetDown(p, i));
    }
}



xkern_return_t
upiInit()
{
    xTrace0(init, TR_EVENTS, "upi init");
    protlMap = mapCreate(UPI_PROTL_MAP_SZ, sizeof(XObj), xkSystemPath);
    safeObjMap = mapCreate(UPI_SAFEOBJ_MAP_SZ, sizeof(XObj), xkSystemPath);
    if ( ! protlMap || ! safeObjMap ) {
	return XK_FAILURE;
    }
    xNullProtl = xCreateProtl((XObjInitFunc *)noop, "null", "",
#if	XK_DEBUG
			      &traceprotocol,
#else
			      0,
#endif
			      0, 0, xkDefaultPath);
    if ( xNullProtl == ERR_XOBJ ) {
	xTrace0(protocol, TR_ERRORS, "Couldn't create null protocol");
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


#if 0

static int
closeProtls(
    void	*key,
    void	*val,
    void	*arg)
{
    if ( xIsProtocol((XObj)val) ) {
	xClose((XObj)val);
    }
    return TRUE;
}

#endif


static int
shutdownProtls(
    void	*key,
    void	*val,
    void	*arg)
{
    if (xIsProtocol((XObj)val)) {
	xShutDown((XObj)val);
    }
    return TRUE;
}


void
xRapture()
{
/*    mapForEach(safeObjMap, closeProtls, 0); */
    mapForEach(safeObjMap, shutdownProtls, 0);
}
