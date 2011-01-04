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
 * dgram.c
 *
 * Copyright (c) 1994  Open Software Foundation
 *
 * Code modeled after protocols/vsize.c, which is
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * $Revision: 1.1.7.1 $
 * $Date: 1995/02/23 15:31:31 $
 */
/*
 * A positive-ACK datagram layer to overlay bytestream protocols (like TCP).
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/romopt.h>
#include <xkern/protocols/dgram/dgram_i.h>
#include <xkern/include/prot/dgram.h>

static void		protlFuncInit( XObj );
static xkern_return_t	sessnInit( XObj );
static XObj		dgramCreateSessn( XObj, XObj, XObj, XObj, Path );
static xkern_return_t	dgramOpenDone( XObj, XObj, XObj, XObj, Path );
static int		dgramControlProtl( XObj, int, char *, int );
static int		dgramControlSessn( XObj, int, char *, int );
static xkern_return_t	dgramClose( XObj );


#if XK_DEBUG
int tracedgramp;
#endif /* XK_DEBUG */


xkern_return_t
dgram_init(self)
	XObj self;
{
	PSTATE	*pstate;
	int	i;
	Path	path = self->path;

	xTrace0(dgramp, TR_GROSS_EVENTS, "dgram init");
	xAssert(xIsProtocol(self));
	xAssert(self->numdown == 1);
	if (!xIsProtocol(xGetDown(self, 0))) {
		xError("dgram down vector is misconfigured");
		return XK_FAILURE;
	}
	if ( ! (pstate = pathAlloc(path, sizeof(PSTATE))) ) {
		return XK_NO_MEMORY;
	}

	self->state = (void *)pstate;
#if 0
	findXObjRomOpts(self, dgramOpts, sizeof(dgramOpts)/sizeof(XObjRomOpt), 0);
#endif
	protlFuncInit(self);
	pstate->activeMap = mapCreate(11, sizeof(XObj), path);
	pstate->passiveMap = mapCreate(11, sizeof(PassiveId), path);
	if ( ! pstate->activeMap || ! pstate->passiveMap ) {
	    return XK_FAILURE;
	}
	return XK_SUCCESS;
}


static XObj
dgramOpen(
	XObj self,
    	XObj hlpRcv,
    	XObj hlpType,
	Part p,
	Path path )
{
	XObj	s, lls;
	PSTATE	*pstate;

	xTrace0(dgramp, TR_MAJOR_EVENTS, "dgram open");
	pstate = (PSTATE *)self->state;
	if (!p || partLen(p) < 1 || partLen(p) > 3) {
		xTrace0(dgramp, TR_ERRORS, "dgram open -- bad participants");
		return ERR_XOBJ;
	}
	lls = xOpen(self, hlpType, xGetDown(self, 0), p, path);
	if (lls == ERR_XOBJ) {
		xTrace0(dgramp, TR_ERRORS, "dgram_open: could not open session");
		return ERR_XOBJ;
	}
	if (mapResolve(pstate->activeMap, lls, &s) == XK_SUCCESS) {
		xTrace0(dgramp, TR_MAJOR_EVENTS, "found an existing one");
		xClose(lls);
	} else {
		xTrace0(dgramp, TR_MAJOR_EVENTS, "creating a new one");
		s = dgramCreateSessn(self, hlpRcv, hlpType, lls, path);
		if (s == ERR_XOBJ)
			xClose(lls);
	}
	return s;
}


/*
 * dgramCreateSessn --
 * Create and initialize a new dgram session using lls.
 * Assumes no session already exists corresponding to lls.
 */
static XObj
dgramCreateSessn(self, hlpRcv, hlpType, lls, path)
	XObj self, hlpRcv, hlpType;
	XObj lls;
	Path path;
{
	XObj	s;
	SSTATE	*sstate;
	PSTATE	*pstate = (PSTATE *)self->state;
	int 	i;

	if ((s = xCreateSessn(sessnInit, hlpRcv, hlpType, self, 1, &lls, path))
			== ERR_XOBJ ) {
		xTrace0(dgramp, TR_ERRORS, "create sessn fails in dgramOpen");
		return ERR_XOBJ;
	}
	if ( ! (sstate = pathAlloc(path, sizeof(SSTATE))) ) {
	    xDestroy(s);
	    return ERR_XOBJ;
	}
	s->state = (void *)sstate;
	if ( mapBind(pstate->activeMap, lls, s, &s->binding) != XK_SUCCESS ) {
		xTrace0(dgramp, TR_ERRORS, "mapBind fails in dgramCreateSessn");
		xClose(s);
		return ERR_XOBJ;
	}
	/*
	 * Initialize reassembly message.
	 */
	msgConstructContig(&sstate->reass, s->path, 0, 0);
	/*
	 * The lower sessions' up fields are made to point to this
	 * dgram session (not the protocol)
	 */
	xSetUp(lls, s);

	xTrace1(dgramp, TR_MAJOR_EVENTS, "dgram open returns %x", s);
	return s;
}


static int
dgramControlSessn(s, opcode, buf, len)
	XObj s;
	int opcode;
	char *buf;
	int len;
{
	xTrace0(dgramp, TR_EVENTS, "dgramControlSessn");

	/*
	 * All opcodes are forwarded.
	 */
	return xControl(xGetDown(s, 0), opcode, buf, len);
}


static int
dgramControlProtl(self, opcode, buf, len)
	XObj self;
	int opcode;
	char *buf;
	int len;
{
	xTrace0(dgramp, TR_EVENTS, "dgramControlProtl");
	/*
	 * All opcodes are forwarded.
	 */
	return xControl(xGetDown(self, 0), opcode, buf, len);
}


static xkern_return_t
dgramOpenEnable(
	XObj self,
    	XObj hlpRcv,
    	XObj hlpType,
	Part p )
{
	PSTATE	*ps = (PSTATE *)self->state;

	xTrace0(dgramp, TR_MAJOR_EVENTS, "dgram open enable");
	return defaultVirtualOpenEnable(self, ps->passiveMap, hlpRcv, hlpType,
					self->down, p);
}


static xkern_return_t
dgramOpenDisable(
	XObj self,
    	XObj hlpRcv,
    	XObj hlpType,
	Part p )
{
	PSTATE	*ps = (PSTATE *)self->state;

	return defaultVirtualOpenDisable(self, ps->passiveMap, hlpRcv, hlpType,
					 self->down, p);
}


static xkern_return_t
dgramClose(s)
	XObj s;
{
	PSTATE		*pstate;
	xkern_return_t	res;
	int i;

	xTrace1(dgramp, TR_MAJOR_EVENTS, "dgram close of session %x", s);
	xAssert(s->rcnt <= 0);
	pstate = (PSTATE *)s->myprotl->state;
	if ( s->binding ) {
		res = mapRemoveBinding(pstate->activeMap, s->binding);
		xAssert( res != XK_FAILURE );
	}
	xSetUp(xGetDown(s, 0), xMyProtl(s));
	xClose(xGetDown(s, 0));

	xDestroy(s);
	return XK_SUCCESS;
}

static int dgram_pattern = (int)DG_MAGIC;

/*
 * We should probably worry about byte order here.
 */

static void
dgramHdrStore(void *hdr, char *where, long hdrLen, void *arg) {
	bcopy((char *)&arg, where, hdrLen);
}

/*
 * Called when trailer is freed.
 * Presumably when the message has been successfully transmitted.
 */

static void
dgramFreeFunc(void *where, int length, void *arg) {
	Msg_s this;

	xTrace3(dgramp, TR_EVENTS, "dgramFreeFunc(%x, %d, %x)", where, length, arg);
	allocFree(where);
	/*
	 * Construct "callback" message and send up.
	 *
	 * This is pretty hokey.  Ideas?
	 */
	msgConstructContig(&this, ((XObj)arg)->path, 0, 0);
	xDemux((XObj)arg, &this);
}

/*
 * Wrap msg in a header giving the length,
 * and a footer with a magic pattern.
 * The free of the footer is used to upcall the positive ACK.
 */

static xmsg_handle_t
dgramPush(
	XObj s,
	Msg msg )
{
	int *pattern_buffer;	/* I always wanted to use that name! */
	Msg_s pattern;
	Allocator mda;

	xTrace0(dgramp, TR_EVENTS, "in dgram push");

	if (msgPush(msg, dgramHdrStore, NULL, (long)sizeof (int),
	    (void *)msgLen(msg)) == XK_FAILURE) {
		xTrace0(dgramp, TR_ERRORS, "dgramPush: msgPush failed");
		return XK_FAILURE;
	}
	/*
	 * Construct a buffer and message to allow us a positive ACK.
	 * This may still be misled by an in-line data copy.
	 */
	mda = pathGetAlloc(msgGetPath(msg), MSG_ALLOC);
	pattern_buffer = allocGet(mda, sizeof *pattern_buffer);
	if (pattern_buffer == NULL) {
		xTrace0(dgramp, TR_ERRORS, "dgramPush: allocGet failed");
		return XK_FAILURE;
	}
	*pattern_buffer = DG_MAGIC;
	
	if (msgConstructInPlace(&pattern, msgGetPath(msg), 
	    (u_int)sizeof *pattern_buffer, 0, (char*)pattern_buffer, 
	    dgramFreeFunc, s) == XK_FAILURE) {
		xTrace0(dgramp, TR_ERRORS, "dgramPush: msgConstructInPlace failed");
		allocFree(pattern_buffer);
		return XK_FAILURE;
	}
	/*
	 * dgramFreeFunc now has responsibility for *pattern_buffer.
	 */

	/*
	 * Append the pattern to the message.
	 */
	if (msgJoin(msg, msg, &pattern) != XK_SUCCESS) {
		xTrace0(dgramp, TR_ERRORS, "dgramPush: msgJoin failed");
		msgDestroy(&pattern);
		return XK_FAILURE;
	}
	xTrace0(dgramp, TR_MORE_EVENTS, "dgram_push");
	xAssert(xIsSession(xGetDown(s, 0)));
	return xPush(xGetDown(s, 0), msg);
}


static xkern_return_t
dgramOpenDone(self, lls, llp, hlpType, path)
	XObj self, lls, llp, hlpType;
	Path path;
{
	XObj	s;
	Part 	p[2];
	char	partBuf[100];
	PSTATE	*pstate;
	Enable	*e;
	int		i,j;
	int		llsIndex;

	xTrace0(dgramp, TR_MAJOR_EVENTS, "In dgram openDone");
	if (self == hlpType) {
		xTrace0(dgramp, TR_ERRORS, "self == hlpType in dgramOpenDone");
		return XK_FAILURE;
	}
	pstate = (PSTATE *)self->state;
	/*
	 * check for open enables
	 */
	if (mapResolve(pstate->passiveMap, &hlpType, &e) == XK_FAILURE) {
		/*
		 * This shouldn't happen
		 */
		xTrace0(dgramp, TR_ERRORS,
			"dgramOpenDone: Couldn't find hlp for incoming session");
		return XK_FAILURE;
	}

	xDuplicate(lls);

	s = dgramCreateSessn(self, e->hlpRcv, e->hlpType, lls, path);
	if (s == ERR_XOBJ) {
		xClose(lls);
		return XK_FAILURE;
	}
	xTrace0(dgramp, TR_EVENTS,
		"dgram Passively opened session successfully created");
	return xOpenDone(e->hlpRcv, s, self, path);
}


static xkern_return_t
dgramProtlDemux( 
	XObj	self,
    	XObj	lls,
	Msg	m )
{
	xTrace0(dgramp, TR_ERRORS, "dgramProtlDemux called!!");
	return XK_SUCCESS;
}


/*
 * dgramPop and dgramSessnDemux must be used (i.e., they can't be
 * bypassed) for the UPI reference count mechanism to work properly. 
 *
 * Strip header and footer, reassembling a datagram at a time.
 */
static long
dgramGetLen(void *hdr, char *src, long len, void *arg) {
	/*
	 * We should probably worry about byte order here.
	 */
	bcopy(src, hdr, len);
	return len;
}

static xkern_return_t
dgramPop(
	XObj self,
	XObj lls,
	Msg msg,
	void *h )
{
	SSTATE *s = (SSTATE *)self->state;
	Msg_s datagram;
	u_int pattern;

	xTrace0(dgramp, TR_EVENTS, "dgram Pop");

	if (msgLen(&s->reass) == 0) {
		if (msgPop(msg, dgramGetLen, &s->length, (long)sizeof s->length, 0)
		    == XK_FAILURE) {
			xTrace0(dgramp, TR_ERRORS, "dgramPop: msgPop failed, lost synch");
			return XK_FAILURE;
		}
	}
	if (msgJoin(&s->reass, &s->reass, msg) == XK_FAILURE) {
		xTrace0(dgramp, TR_ERRORS, "dgramPop: msgjoin failed, lost synch");
		return XK_FAILURE;
	}
	if (msgLen(&s->reass) < s->length + sizeof dgram_pattern) {
		/*
		 * We do not have a whole message.
		 */
		xTrace0(dgramp, TR_EVENTS, "dgramPush: unfulfilled message");
		return XK_SUCCESS;
	}
	/*
	 * Construct a new message containing the completed datagram.
	 */
	msgConstructContig(&datagram, self->path, 0, 0);
	msgChopOff(msg, &datagram, s->length);
	msgPop(&s->reass, dgramGetLen, &pattern, (long)sizeof pattern, 0);
	if (pattern != dgram_pattern)
		xTrace1(dgramp, TR_ERRORS, "dgramPop: pattern mismatch: 0x%x", pattern);
	return xDemux(self, &datagram);
}


static xkern_return_t
dgramSessnDemux(
	XObj self,
	XObj lls,
	Msg msg )
{
	xTrace0(dgramp, TR_EVENTS, "dgram Session Demux");
	return xPop(self, lls, msg, 0);
}


static xkern_return_t
sessnInit(s)
	XObj s;
{
	xAssert(xIsSession(s));

	s->push = dgramPush;
	s->pop = dgramPop;
	s->close = dgramClose;
	s->control = dgramControlSessn;
	/*
	 * dgram sessions will look like a protocol to lower sessions, so we
	 * need a demux function
	 */
	s->demux = dgramSessnDemux;
	return XK_SUCCESS;
}

static void
protlFuncInit(p)
	XObj p;
{
	xAssert(xIsProtocol(p));

	p->control = dgramControlProtl;
	p->open = dgramOpen;
	p->openenable = dgramOpenEnable;
	p->opendisable = dgramOpenDisable;
	p->demux = dgramProtlDemux;
	p->opendone = dgramOpenDone;
}
