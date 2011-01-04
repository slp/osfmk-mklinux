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

#define SEQ_TRANSPORT 0
#define SEQ_BOOTP     1
/*
 * Real Header
 */
typedef enum {
    BROADCAST_REQUEST,
    SEQUENCER_BROADCAST,
    ACK_TO_SEQUENCER,
    SEQUENCER_ACCEPT,
    SEQUENCER_SYNC_REQUEST,
    SEQUENCER_SYNC_REPLY,
    SEQUENCER_MASTER_ALIVE
} GOper;

typedef struct {
    u_bit32_t   h_hlpNum;
    group_id    h_gid;
    u_bit32_t   h_gincarnation;
    IPhost      h_name;
    IPhost      h_name_initiator;   /* who's started all this? */
    IPhost      h_master;
    u_bit32_t   h_headcount;
    GOper       h_type;
    seqno       h_seqno;
    u_bit32_t   h_messageid;
    u_bit32_t   h_expect;
} sequencerHdr;

/*
 * Pseudo Header:  sequencerXchangeHdr. See include/prot/sequencer.h
 */

/*
 * Protocol State
 */
typedef struct {
    Map         activeMap;
    Map         passiveMap;
    Map         groupMap;
    Map         node2groups;
    u_int       retrans_max;
    u_int       retrans_msecs;
    long        max_groups;
    long        max_history_entries;
    IPhost      thishost;
    IPhost      master;
    IPhost      mcaddr;
    xkSemaphore	serviceSem;
} PState;    

/*
 * Group State
 */
typedef enum {
    EMPTY = 0,
    STABLE,
    UNSTABLE,
    WAITING
} GMode;

typedef enum {
    MASTER_OR_SLAVE_IDLE = 0,
    MASTER_NEW_BROAD_SENT,
    MASTER_ACKS_RCVD_OK,
    MASTER_ACCEPT_SENT,
    MASTER_ERROR,
    SLAVE_NEW_BROAD_RCVD,
    SLAVE_ACK_SENT,
    SLAVE_ACCEPT_RCVD_DELIVER,
    SLAVE_ERROR
} GMachine;

typedef struct {
    Msg_s        i_message;
    sequencerHdr i_hdr;
    GMode        i_mode;
    u_long       i_retrial;
    u_long       i_refcount;     /* nodes that prevent me from removing the entry from list */
    u_long       i_ackcount;     /* nodes that haven't ack'd yet and thus prevent me from being stable */
    Event        i_ev;
} GHistory;

typedef struct {
    group_id    this_gid;
    IPhost      address;
    u_bit32_t   group_incarnation;
    u_bit32_t   group_seqno;          
    XObj        group_lls;
    XObj        master_lls;
    GMachine    state;
    Map         node_map;
    u_long      num_members;
    u_long      delay;
    u_long      watchdog;
    u_long      idle_ticks;
    Event       gc_ev;
    Event       live_ev;
    GHistory    *cbuffer;
    u_long      consumer, producer, waitroom;
    u_long      group_history_entries;
} GState;

/*
 * Member State
 */
typedef struct {
    u_bit32_t   m_messageid;
    u_bit32_t   m_expect;
} MState;

/*
 * Session State
 */
typedef struct {
    u_bit32_t   hlpNum;
    GState      *shared;
    GHistory    *his;
    xkSemaphore	sessionSem;
    boolean_t   asynnot;
    Msg         asynmsg;
} SState;    

typedef struct {
    u_bit32_t   hlpNum;
    group_id    group;
} ActiveKey;

typedef u_bit32_t PassiveKey;

typedef struct {
    IPhost old;
    IPhost new;
} Election;

#define SEQUENCER_ACTIVE_MAP_SIZE	            10
#define SEQUENCER_PASSIVE_MAP_SIZE	            10
#define DEFAULT_RETRANS_MAX                         10
#define DEFAULT_RETRANS_MSECS                      500 
#define DEFAULT_MAX_GROUPS                          10
#define DEFAULT_MAX_HISTORY_ENTRIES                 50
#define DEFAULT_DELAY                              500  /* 500 msec */
#define DEFAULT_WATCHDOG                         10000  /* 10  sec  */
#define MASTER_DEFAULT_WATCHDOG     (DEFAULT_WATCHDOG)
#define SLAVE_DEFAULT_WATCHDOG  (3 * DEFAULT_WATCHDOG)
#define INCARNATION_WILD_CARD          ((u_bit32_t)-1)
#define SEQUENCER_SWEEP_HISTORY       DEFAULT_WATCHDOG       

/* forward */

xkern_return_t sequencer_init( XObj );

static MState *
getms(
      Map    map,
      IPhost node);

static int
getnewmaster_entries(
		     void	*key,
		     void	*val,
		     void	*arg);

static IPhost
getnewmaster(
	     Map    map,
	     IPhost oldmaster);

static xkern_return_t   
LoadMaster(
	   XObj	self,
	   char	**str,
	   int	nFields,
	   int	line,
	   void	*arg);

static xkern_return_t   
LoadMcastAddress(
		 XObj	self,
		 char	**str,
		 int	nFields,
		 int	line,
		 void	*arg);

static xkern_return_t   
LoadMaxGroups(
	      XObj	self,
	      char	**str,
	      int	nFields,
	      int	line,
	      void	*arg);

static xkern_return_t   
LoadMaxHistoryEntries(
		      XObj	self,
		      char	**str,
		      int	nFields,
		      int	line,
		      void	*arg);

static xkern_return_t   
LoadRetransMax(
	       XObj	self,
	       char	**str,
	       int	nFields,
	       int	line,
	       void	*arg);

static xkern_return_t   
LoadRetransMsecs(
		 XObj	self,
		 char	**str,
		 int	nFields,
		 int	line,
		 void	*arg);

static void
sequencerAlert(
	       Event	ev,
	       void 	*arg);

static int
sequencerAsynNotif(
		   void	*key,
		   void	*val,
		   void	*arg);

static xkern_return_t
sequencerClose(
	       XObj	self);

GState *
sequencerCreateGState(
		      XObj     self,
		      group_id gid);

static XObj
sequencerCreateSessn(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     ActiveKey	*key,
		     Path	path);

static xkern_return_t
sequencerDemux(
	       XObj 	self,
	       XObj	lls,
	       Msg 	msg);

static xkern_return_t
sequencerDemuxBootstrap(
			XObj 	self,
			XObj	lls,
			Msg 	msg);

static void
sequencerError(
	       XObj      self,
	       GHistory  *his,
	       boolean_t istimeout);

static void
sequencerErrorMaster(
		     XObj      self,
		     GHistory  *his,
		     boolean_t istimeout);

static void
sequencerEventReturn(
		     Event     ev,
		     void      *arg);

static IPhost
sequencerGroupAddress(
		      XObj     self,
		      group_id gid);

static long
sequencerHdrLoad(
		 void	*hdr,
		 char 	*src,
		 long	len,
		 void	*arg);

static void
sequencerHdrStore(
		  void	*hdr,
		  char 	*dst,
		  long	len,
		  void	*arg);

static void
sequencerMasterAlive(
		     Event	ev,
		     void 	*arg);

static XObj
sequencerOpen(
	      XObj	self,
	      XObj	hlpRcv,
	      XObj	hlpType,
	      Part	p,
	      Path	path);

static xkern_return_t
sequencerOpenDisable(
		     XObj	self,
		     XObj	hlpRcv,
		     XObj	hlpType,
		     Part	p);

static xkern_return_t
sequencerOpenDisableAll(
			XObj	self,
			XObj	hlpRcv);

static xkern_return_t
sequencerOpenDone(
		  XObj  self,
		  XObj	lls,
		  XObj	llp,
		  XObj	hlpType,
		  Path	path);

static xkern_return_t
sequencerOpenEnable(
		    XObj	self,
		    XObj	hlpRcv,
		    XObj	hlpType,
		    Part	p);

static xmsg_handle_t
sequencerOutput(
		XObj	self,
		Msg	message,
		IPhost  *node);

static xmsg_handle_t
sequencerOutputControl(
		       XObj      self,
		       GOper     op,
		       seqno     number,
		       boolean_t toslave);

static xkern_return_t
sequencerPop(
	     XObj	self,
	     XObj	lls,
	     Msg	m,
	     void	*inHdr);

static int
sequencerProtControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len);

static void
sequencerPulse(
	       Event	ev,
	       void 	*arg);

static xmsg_handle_t
sequencerPush(
	      XObj	self,
	      Msg	message);

static void
sequencerPruneHistory(
		      GState       *gg,
		      u_bit32_t    from,
		      u_bit32_t    to,
		      boolean_t    release_ref,
		      boolean_t    unconditioned);

static void
sequencerSendSync(
		  Event	ev,
		  void 	*arg);

static int
sequencerSessControl(
		     XObj	self,
		     int	op,
		     char 	*buf,
		     int	len);

static void
sequencerSweepHistory(
		      Event	ev,
		      void 	*arg);

static void
sequencerTimeout(
		 Event	ev,
		 void 	*arg);

static void
sequencerTimeoutMaster(
		       Event	ev,
		       void 	*arg);








