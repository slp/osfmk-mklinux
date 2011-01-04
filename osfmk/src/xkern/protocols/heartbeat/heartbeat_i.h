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

typedef struct {
    Map         activeMap;
    Map         passiveMap;
    u_int       def_rate_msecs;
    IPhost      thishost;
    xkSemaphore	serviceSem;
} PState;    

typedef struct {
    u_bit32_t   hlpNum;
    u_int       rate_msecs;
    u_bit32_t   GenNumber;
    Event       ev;
    Msg_s       msg_save;
} SState;    

typedef struct {
    XObj      lls;
    u_bit32_t hlpNum;
} ActiveKey;

typedef u_bit32_t PassiveKey;

#define HEARTBEAT_ACTIVE_MAP_SIZE	10
#define HEARTBEAT_PASSIVE_MAP_SIZE	10
#define DEFAULT_PULSE_RATE 1000 

/* forward */

static xkern_return_t
heartbeatClose(
	       XObj	self);

static XObj
heartbeatCreateSessn(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     ActiveKey	*key,
		     Path	path);

xkern_return_t
heartbeatDemux(
	       XObj 	self,
	       XObj	lls,
	       Msg 	msg);

static long
heartbeatHdrLoad(
		 void	*hdr,
		 char 	*src,
		 long	len,
		 void	*arg);

static void
heartbeatHdrStore(
		  void	*hdr,
		  char 	*dst,
		  long	len,
		  void	*arg);

static XObj
heartbeatOpen(
	      XObj	self,
	      XObj	hlpRcv,
	      XObj	hlpType,
	      Part	p,
	      Path	path);

static xkern_return_t
heartbeatOpenDisable(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     Part	p);

static xkern_return_t
heartbeatOpenDisableAll(
			XObj	self,
			XObj	hlpRcv);

xkern_return_t
heartbeatOpenDone(
		   XObj self,
		   XObj	lls,
		   XObj	llp,
		   XObj	hlpType,
		   Path	path);

static xkern_return_t
heartbeatOpenEnable(
		    XObj	self,
		    XObj	hlpRcv,
		    XObj	hlpType,
		    Part	p);

static xkern_return_t
heartbeatPop(
	     XObj	self,
	     XObj	lls,
	     Msg	m,
	     void	*inHdr);

static int
heartbeatProtControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len);

static void
heartbeatPulse(
	       Event	ev,
	       void 	*arg);

static xmsg_handle_t
heartbeatPush(
	      XObj	self,
	      Msg	m);

static int
heartbeatSessControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len);

static xkern_return_t   
LoadRate(
	 XObj	self,
	 char	**str,
	 int	nFields,
	 int	line,
	 void	*arg);



