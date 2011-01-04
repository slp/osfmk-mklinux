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

#define DEFAULT_MAX_GROUPS   10
#define DEFAULT_MAX_NODES   100

/*
 * Protocol State
 */
typedef struct {
    Map          activeMap;
    Map          passiveMap;
    Map          groupMap;
    Map          node2groups;  
    long         max_groups;
    long         max_nodes;
    IPhost       thishost;
    xkSemaphore	 serviceSem;
    u_long       timestamp;
} PState;    

/*
 * Group State. 
 * Note that GState->node_map of ALL_NODES_GROUP points
 * to PState->node2groups. That is, for ALL_NODES_GROUP
 * we need to maintain a per node linked list of all
 * other groups which refer to that node.
 */
typedef struct {
    group_id              this_gid;
    boolean_t             initiator;
    u_bit32_t             token_inprogress;
    membershipXchangeHdr  hdr_inprogress;
    Map                   node_map;
    u_long                gcount;
} GState;

/*
 * Session State
 */
typedef struct {
    u_long                timestamp;
    u_bit32_t             hlpNum;
    GState                *shared;   /* among all sessions with same group_id */
} SState;    

xkern_return_t membership_init( XObj );

typedef struct {
    XObj      lls;
    u_bit32_t hlpNum;
} ActiveKey;

typedef group_id PassiveKey;

#define MEMBERSHIP_ACTIVE_MAP_SIZE	10
#define MEMBERSHIP_PASSIVE_MAP_SIZE	10

/* forward */

static xkern_return_t
membershipClose(
		XObj	self);

static GState *
membershipCreateGState(
		       XObj     self,
		       group_id gid);

static XObj
membershipCreateSessn(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     ActiveKey	*key,
		     Path	path);

xkern_return_t
membershipDemux(
		XObj 	self,
		XObj	lls,
		Msg 	msg);

static long
membershipHdrLoad(
		  void	*hdr,
		  char 	*src,
		  long	len,
		  void	*arg);

static void
membershipHdrStore(
		   void	*hdr,
		   char *dst,
		   long	len,
		   void	*arg);

static XObj
membershipOpen(
	       XObj	self,
	       XObj	hlpRcv,
	       XObj	hlpType,
	       Part	part,
	       Path	path);

static xkern_return_t
membershipOpenDisable(
		      XObj	self,
		      XObj	hlpRcv,
		      XObj	hlpType,
		      Part	p);

static xkern_return_t
membershipOpenDisableAll(
			 XObj	self,
			 XObj	hlpRcv);

xkern_return_t
membershipOpenDone(
		   XObj self,
		   XObj	lls,
		   XObj	llp,
		   XObj	hlpType,
		   Path	path);

static xkern_return_t
membershipOpenEnable(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     Part	p);

static xkern_return_t
membershipPop(
	      XObj	self,
	      XObj	lls,
	      Msg	message,
	      void	*hdr);

static int
membershipProtControl(
		      XObj	self,
		      int	op,
		      char 	*buf,
		      int	len);

static xmsg_handle_t
membershipPush(
	       XObj	self,
	       Msg	message);

static int
membershipSessControl(
		      XObj	self,
		      int	op,
		      char 	*buf,
		      int	len);

node_element *
membershipUpdateAddition(
			 XObj   self,
			 IPhost node);

xkern_return_t
membershipUpdateDeletion(
			 XObj   self,
			 IPhost node);

static boolean_t
membershipUpdateInternal(
			 char *buf,
			 long int len,
			 void *foo);

void
membershipUpdateState(
		      XObj self,
		      Msg  message);

static int
membershipZapEntries(
		     void	*key,
		     void	*val,
		     void	*arg);

static xkern_return_t   
LoadMaxGroups(
	      XObj	self,
	      char	**str,
	      int	nFields,
	      int	line,
	      void	*arg);

static xkern_return_t   
LoadMaxNodes(
	     XObj	self,
	     char	**str,
	     int	nFields,
	     int	line,
	     void	*arg);




