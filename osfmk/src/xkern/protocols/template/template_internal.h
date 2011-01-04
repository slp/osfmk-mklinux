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

xkern_return_t template_init( XObj );

typedef struct {
    u_bit32_t	  Prot;
    u_bit32_t     GenNumber;
    IPhost        Sender;
} templateHdr;

/*
 * Control Operations
 */
#define TEMPLATE_START_PULSE    ( TEMPLATE_CTL * MAXOPS + 0 )
#define TEMPLATE_STOP_PULSE     ( TEMPLATE_CTL * MAXOPS + 0 )

typedef struct {
    XObj      lls;
    u_bit32_t hlpNum;
} ActiveKey;

typedef u_bit32_t PassiveKey;

#define TEMPLATE_ACTIVE_MAP_SIZE	10
#define TEMPLATE_PASSIVE_MAP_SIZE	10
#define DEFAULT_PULSE_RATE 1000 

/* forward */

static xkern_return_t
templateClose(
	       XObj	self);

static XObj
templateCreateSessn(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     ActiveKey	*key,
		     Path	path);

xkern_return_t
templateDemux(
	       XObj 	self,
	       XObj	lls,
	       Msg 	msg);

static long
templateHdrLoad(
		 void	*hdr,
		 char 	*src,
		 long	len,
		 void	*arg);

static void
templateHdrStore(
		  void	*hdr,
		  char 	*dst,
		  long	len,
		  void	*arg);

static XObj
templateOpen(
	      XObj	self,
	      XObj	hlpRcv,
	      XObj	hlpType,
	      Part	p,
	      Path	path);

static xkern_return_t
templateOpenDisable(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     Part	p);

static xkern_return_t
templateOpenDisableAll(
			XObj	self,
			XObj	hlpRcv);

xkern_return_t
templateOpenDone(
		   XObj self,
		   XObj	lls,
		   XObj	llp,
		   XObj	hlpType,
		   Path	path);

static xkern_return_t
templateOpenEnable(
		    XObj	self,
		    XObj	hlpRcv,
		    XObj	hlpType,
		    Part	p);

static xkern_return_t
templatePop(
	     XObj	self,
	     XObj	lls,
	     Msg	m,
	     void	*inHdr);

static int
templateProtControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len);

static void
templatePulse(
	       Event	ev,
	       void 	*arg);

static xmsg_handle_t
templatePush(
	      XObj	self,
	      Msg	m);

static int
templateSessControl(
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



