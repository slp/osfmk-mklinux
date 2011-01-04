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

#define PANNING_TRANSPORT 0
#define PANNING_BOOTP     1

typedef struct {
    u_bit32_t   h_hlpNum;
    u_bit32_t   h_cluster;
    u_bit32_t   h_gincarnation;
} panningHdr;

#define PANNING_ACTIVE_MAP_SIZE	       101
#define PANNING_PASSIVE_MAP_SIZE	31

typedef struct {
    u_bit32_t   myCluster;
    u_bit32_t   myGroupIncarnation;
    Map		activeMap;	
    Map		passiveMap;	
    xkSemaphore	sessnCreationSem;
} PState;

typedef struct {
    panningHdr  s_hdr;
    boolean_t   s_check_incarnation;
} SState;

typedef struct {
    long	hlpNum;
    XObj        lls;
} ActiveKey;

typedef long	PassiveKey;

/* forwards */

static void
getProtlFuncs(
	      XObj	p);

static xkern_return_t
getSessnFuncs(
	      XObj	s);

static xkern_return_t
panningClose(
	     XObj	self);

static int
panningControlProtl(
		    XObj	self,
		    int		op,
		    char 	*buf,
		    int		len);

static int
panningControlSessn(
		    XObj	self,
		    int		op,
		    char 	*buf,
		    int		len);

static xkern_return_t
panningDemux(
	     XObj	self,
	     XObj	lls,
	     Msg	m);

static XObj
panningCreateSessn(
		   XObj	self,
		   XObj	hlpRcv,
		   XObj	hlpType,
		   ActiveKey	*key,
		   Path	path);

static void
panningHdrStore(
		void	*h,
		char 	*dst,
		long	len,
		void	*arg);

static long
panningHdrLoad(
	       void	*hdr,
	       char 	*src,
	       long	len,
	       void	*arg);

static XObj
panningNewSessn(
		XObj	self,
		XObj	hlpRcv,
		XObj	hlpType,
		ActiveKey	*key,
		Path	path);

static XObj
panningOpen(
	    XObj	self,
	    XObj	hlpRcv,
	    XObj	hlpType,
	    Part	p,
	    Path	path);

static xkern_return_t
panningOpenEnable(
		  XObj	self,
		  XObj	hlpRcv,
		  XObj	hlpType,
		  Part	p);

static xkern_return_t
panningOpenDisable(
		   XObj	self,
		   XObj	hlpRcv,
		   XObj	hlpType,
		   Part	p);

static xkern_return_t
panningOpenDisableAll(
		      XObj	self,
		      XObj	hlpRcv);

static xkern_return_t
panningPop(
	   XObj	self,
	   XObj	lls,
	   Msg	m,
	   void	*inHdr);

static xmsg_handle_t
panningPush(
	    XObj	self,
	    Msg		m);

xkern_return_t
panning_init(
	     XObj	self);


	    


