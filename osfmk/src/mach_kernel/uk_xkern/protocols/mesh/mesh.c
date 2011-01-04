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
#include <xkern/include/xkernel.h>
#include <xkern/include/prot/vnet.h>
#include <norma/ipc_net.h>
#include <uk_xkern/protocols/mesh/mesh_i.h>
#include <norma/ipc_xk.h>

#if !NORMA_ETHER

#define SUPER_CLOCK 0

#if SUPER_CLOCK
int	nets_mesh;
int	netr_mesh;
int	netr1_mesh;
extern void fc_get();
#endif /* SUPER_CLOCK */

typedef mesh_header_t SState;

typedef struct {
    node_t		mynode;
    Map         	actMap;
    Map         	pasMap;
    unsigned long	mtu;
} PState;

typedef struct {
    node_t		dest_node;
    long		protNum;
} ActiveId;

typedef unsigned long	PassiveId;

int tracemeshp;

#define MESH_ACTIVE_MAP_SZ 128
#define MESH_PASSIVE_MAP_SZ 128

long 
xkcopy(dst, src, len, arg)
void *dst, *src;
long len;
void *arg;
{
    (void)bcopy(src, dst, len);
    return len;
}

static long
getRelProtNum(hlp, llp, s)
XObj        hlp, llp;
char        *s;
{
    long        n;

    n = relProtNum(hlp, llp);
    if (n == -1) {
        xTrace3(meshp, TR_SOFT_ERROR,
               "%s: couldn't get prot num of %s relative to %s",
               s, hlp->name, llp->name);
        return XK_FAILURE;
    }
    return n;
}

static xmsg_handle_t
meshLoopPush(s, msg)
XObj        s;
Msg         *msg;
{
    xTrace0(meshp, TR_EVENTS, "meshLoopPush: who's calling us???");
    return XMSG_NULL_HANDLE;
}

static xmsg_handle_t
meshPush(s, msg)
XObj        s;
Msg         *msg;
{
#if SUPER_CLOCK
    int starts_mesh[2], ends_mesh[2];

    fc_get(starts_mesh);
#endif
    xTrace0(meshp, TR_EVENTS, "mesh_push");
    msgSetAttr(msg, 0, s->state, sizeof(SState));
    xPush(xGetDown(s, 0), msg);
#if SUPER_CLOCK
    fc_get(ends_mesh);
    nets_mesh = ends_mesh[0] - starts_mesh[0];
#endif
    return XMSG_NULL_HANDLE;
}

static xkern_return_t
meshPop(s, llp, m, h)
XObj        s, llp;
Msg         *m;
void        *h;
{
    return xDemux(s, m);
}

static xkern_return_t
meshClose(s)
XObj        s;
{
    PState      *ps = (PState *)xMyProtl(s)->state;

    xTrace1(meshp, TR_MAJOR_EVENTS, "mesh closing session %x", s);
    xAssert(xIsSession(s));
    xAssert(s->rcnt <= 0);
    mapRemoveBinding(ps->actMap, s->binding);
    xDestroy(s);
    return XK_SUCCESS;
}

static int
meshControlProtl( self, op, buf, len )
XObj        self;
int         op, len;
char        *buf;
{
    PState      *ps = (PState *)self->state;

    xAssert(xIsProtocol(self));

    switch (op) {
      case GETMAXPACKET:
      case GETOPTPACKET:
        checkLen(len, sizeof(int));
        *(unsigned long *)buf = ps->mtu;
        return (sizeof(int));

      case GETMYHOST:
        checkLen(len, sizeof(node_t));
        *(node_t *)buf = ps->mynode;
        return (sizeof(node_t));

      case VNET_ISMYADDR:
        checkLen(len, sizeof(node_t));
	if (*(node_t *)buf == ps->mynode)
	    return sizeof(node_t);
	else 
	    return 0;

      default:
        return xControl(xGetDown(self, 0), op, buf, len);
    } 
}

static int
meshControlSessn(s, op, buf, len)
XObj        s;
int         op, len;
char        *buf;
{
    SState      *ss = (SState *)s->state;

    xAssert(xIsSession(s));
    switch (op) {

      case GETMYHOST:
      case GETMAXPACKET:
      case GETOPTPACKET:
        return meshControlProtl(xMyProtl(s), op, buf, len);

      case GETPEERHOST:
        checkLen(len, sizeof(node_t));
        *(node_t *)buf = ss->destination;
        return (sizeof(node_t));

      case GETMYHOSTCOUNT:
      case GETPEERHOSTCOUNT:
        checkLen(len, sizeof(int));
        *(int *)buf = 1;
        return sizeof(int);

      case GETMYPROTO:
      case GETPEERPROTO:
        checkLen(len, sizeof(long));
        *(unsigned long *) buf = ss->type;
        return sizeof(long);

      case GETPARTICIPANTS:
        {
            Part        p;

            partInit(&p, 1);
            /*
             * Remote host
             */
            partPush(p, &ss->destination, sizeof(node_t));
            return (partExternalize(&p, buf, &len) == XK_FAILURE) ? -1 : len;
        }

      default:
        return -1;
    }
}

static void
meshSessnInit(s)
XObj s;
{
    s->push = meshPush;
    s->pop = meshPop;
    s->close = meshClose;
    s->control = meshControlSessn;
}

static XObj
meshCreateSessn( self, hlpRcv, hlpType, key )
    XObj        self, hlpRcv, hlpType;
    ActiveId    *key;
{
    XObj        s;
    XObj        llp = xGetDown(self, 0);
    SState      *ss;
    PState      *ps = (PState *)self->state;

    s = xCreateSessn(meshSessnInit, hlpRcv, hlpType, self, 1, &llp);
    if (key->dest_node == ps->mynode) {
        xTrace0(meshp, TR_MAJOR_EVENTS,
                "ethMeshSessn -- creating loopback session");
        s->push = meshLoopPush;
    }
    s->binding = (Bind) mapBind(ps->actMap, (char *)key, (int)s);
    if (s->binding == ERR_BIND) {
        xTrace0(meshp, TR_ERRORS, "error binding in meshCreateSessn");
        xDestroy(s);
        return ERR_XOBJ;
    }
    ss = X_NEW(SState);
    ss->type = key->protNum;
    ss->remote = ps->mynode;
    ss->destination = key->dest_node;
    s->state = (void *)ss;
    return s;
}

static XObj
meshOpen(self, hlpRcv, hlpType, part)
XObj        self, hlpRcv, hlpType;
Part        *part;
{
    PState      *ps = (PState *)self->state;
    ActiveId    key;
    XObj        meshSessn;
    node_t      *remote_node;
    long        protNum;

    if (part == 0 || partLen(part) < 1) {
        xTrace0(meshp, TR_ERRORS, "meshOpen -- bad participants");
        return ERR_XOBJ;
    }
    remote_node = (node_t *)partPop(*part);
    xAssert(remote_node);
    key.dest_node = *remote_node;
    if ((protNum = getRelProtNum(hlpType, self, "open")) == XK_FAILURE) {
        return ERR_XOBJ;
    }
    key.protNum = protNum;
    xTrace2(meshp, TR_MAJOR_EVENTS, "mesh_open: dest_node = %d, prot = 0x%x\n",
            key.dest_node, key.protNum);
    if (mapResolve(ps->actMap, &key, &meshSessn) == XK_FAILURE) {
        meshSessn = meshCreateSessn(self, hlpRcv, hlpType, &key);
    }
    xTrace1(meshp, TR_MAJOR_EVENTS, "mesh_open: returning %X", meshSessn);
    return (meshSessn);
}

static xkern_return_t
meshOpenEnable(self, hlpRcv, hlpType, part)
XObj self, hlpRcv, hlpType;
Part *part;
{
    PState      *ps = (PState *)self->state;
    PassiveId   key;
    long        protNum;

    if ((protNum = getRelProtNum(hlpType, self, "openEnable")) == XK_FAILURE) {
        return XK_FAILURE;
    }
    xTrace2(meshp, TR_GROSS_EVENTS, "mesh_openenable: hlp=%x, protlNum=%x",
            hlpRcv, protNum);
    key = protNum;
    return defaultOpenEnable(ps->pasMap, hlpRcv, hlpType, (void *)&key);
}

static xkern_return_t
meshOpenDisable(self, hlpRcv, hlpType, part)
XObj self, hlpRcv, hlpType;
Part *part;
{
    PState      *ps = (PState *)self->state;
    long        protNum;
    PassiveId   key;

    if ((protNum = getRelProtNum(hlpType, self, "opendisable")) == XK_FAILURE) {
        return XK_FAILURE;
    }
    xTrace2(meshp, TR_GROSS_EVENTS, "mesh_openenable: hlp=%x, protlNum=%x",
            hlpRcv, protNum);
    key = protNum;
    return defaultOpenDisable(ps->pasMap, hlpRcv, hlpType, (void *)&key);
}

static int
dispActiveMap(key, val, arg)
void        *key, *val, *arg;
{
    XObj        s = (XObj)val;
    xPrintXObj(s);
    return MFE_CONTINUE;
}


static int
dispPassiveMap(key, val, arg)
void        *key, *val, *arg;
{
#if XK_DEBUG
    Enable      *e = (Enable *)val;
#endif /* XK_DEBUG */
    xTrace2(meshp, TR_ALWAYS, "Enable object, hlpRcv == %s, hlpType = %s",
            e->hlpRcv->fullName, e->hlpType->fullName);
    return MFE_CONTINUE;
}

static xkern_return_t
meshOpenDisableAll( self, hlpRcv )
    XObj        self, hlpRcv;
{
    xkern_return_t      xkr;
    PState              *ps = (PState *)self->state;

    xTrace0(meshp, TR_MAJOR_EVENTS, "mesh openDisableAll called");

    xTrace0(meshp, TR_ALWAYS, "before passive map contents:");
    mapForEach(ps->pasMap, dispPassiveMap, 0);
    xkr = defaultOpenDisableAll(((PState *)self->state)->pasMap, hlpRcv, 0);
    xTrace0(meshp, TR_ALWAYS, "after passive map contents:");
    mapForEach(ps->pasMap, dispPassiveMap, 0);
    xTrace0(meshp, TR_ALWAYS, "active map contents:");
    mapForEach(ps->actMap, dispActiveMap, 0);
    return XK_SUCCESS;
}

static xkern_return_t
meshDemux(self, llp, msg)
XObj        self, llp;
Msg         *msg;
{
    PState      *ps = (PState *)self->state;
    ActiveId    actKey;
    PassiveId   pasKey;
    XObj        s = 0;
    Enable      *e;
    SState      sstate, *ss = &sstate;
#if SUPER_CLOCK
    int startr_mesh[2], endr_mesh[2], inter[2];

    fc_get(startr_mesh);
#endif

    msgPop(msg, (MLoadFun) xkcopy, ss, sizeof(SState), NULL);
    xTrace0(meshp, TR_EVENTS, "mesh_demux");
    xTrace1(meshp, TR_FUNCTIONAL_TRACE, "mesh type: 0x%x", ss->type);
    xTrace2(meshp, TR_FUNCTIONAL_TRACE, "src: %d  dst: %d",
            ss->remote, ss->destination);
    xIfTrace(meshp, TR_DETAILED) 
	msgShow(msg);
#if XK_DEBUG
    /*
     * verify that msg is for this host
     */
    if (ss->destination != node_self()) {
        xError("mesh_demux: msg is not for this host");
        return XK_FAILURE;
    }
#endif /* XK_DEBUG */
    actKey.dest_node = ss->remote;
    actKey.protNum = ss->type;
    if (mapResolve(ps->actMap, &actKey, &s) == XK_FAILURE) {
        pasKey = actKey.protNum;
        if (mapResolve(ps->pasMap, &pasKey, &e) == XK_SUCCESS) {
            xTrace1(meshp, TR_EVENTS,
                    "mesh_demux: openenable exists for msg type %x",
                    pasKey);
            xAssert(pasKey == relProtNum(e->hlpType, self));
            s = meshCreateSessn(self, e->hlpRcv, e->hlpType, &actKey);
            if (s != ERR_XOBJ) {
                xOpenDone(e->hlpRcv, s, self);
                xTrace0(meshp, TR_EVENTS,
                        "mesh_demux: sending message to new session");
            }
        } else {
            xTrace1(meshp, TR_EVENTS,
                    "mesh_demux: openenable does not exist for msg type %x",
                    pasKey);
        }
    }
    if (xIsSession(s)) {
        xPop(s, llp, msg, 0);
    }
#if SUPER_CLOCK
    fc_get(inter);
    netr1_mesh = inter[0] - startr_mesh[0];
#endif
    msgDestroy(msg);
#if SUPER_CLOCK
    fc_get(endr_mesh);
    netr_mesh = endr_mesh[0] - startr_mesh[0];
#endif
    return XK_SUCCESS;
}

void
mesh_init( self )
XObj self;
{
    PState      *ps;
    XObj        llp;

    xTrace0(meshp, TR_EVENTS, "mesh_init");
    if (!xIsProtocol(llp = xGetDown(self, 0))) {
        xError("mesh can not get driver protocol object");
        return;
    }
    if ( xOpenEnable(self, self, llp, 0) == XK_FAILURE ) {
        xError("mesh can not openenable driver protocol");
        return;
    }
    ps = X_NEW(PState);
    self->state = (void *)ps;
    ps->actMap = mapCreate(MESH_ACTIVE_MAP_SZ, sizeof(ActiveId));
    ps->pasMap = mapCreate(MESH_PASSIVE_MAP_SZ, sizeof(PassiveId));
    ps->mynode = node_self();
    ps->mtu = MESH_DEFAULT_MTU;

    self->control = meshControlProtl;
    self->open = meshOpen;
    self->openenable = meshOpenEnable;
    self->opendisable = meshOpenDisable;
    self->demux = meshDemux;
    self->opendisableall = meshOpenDisableAll;
}

#endif /* !NORMA_ETHER */
