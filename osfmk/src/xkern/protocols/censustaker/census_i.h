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

#define DEFAULT_MAX_NODES       100
#define DEFAULT_MAX_TOLERANCE   1000 /* ms */

#define CENSUSTAKER_DELAY       1000 /* ms */
#define CENSUSTAKER_STACK_SIZE  100  /* bytes */

typedef struct {
    IPhost       master;
    IPhost       thishost;
    u_int        max_nodes;
    u_int        tolerance;
    u_bit32_t    logical_clock;
    XObj         lls[2];
    Event        ev;
    struct dlist token_list;
    Map          address_map;
    boolean_t    
                 force_check:1,
                 unstable:1;
} PState;    

typedef struct {
    struct dlist dl;
    IPhost       address;
    u_bit32_t    last_update;
    boolean_t    is_new;
    u_long       timestamp;
} NodeState;

/*
 * Protocols beneath the census taker
 */
#define HEARTBEAT  0
#define MEMBERSHIP 1

xkern_return_t
censustakerControl(
		   XObj	self,
		   int	op,
		   char *buf,
		   int	len);

xkern_return_t
censustakerDemux(
		 XObj 	self,
		 XObj	lls,
		 Msg 	msg);

void
censustakerMasterChange(
			XObj   self,
			IPhost master);

xkern_return_t
censustakerOpenDone(
		    XObj self,
		    XObj lls,
		    XObj llp,
		    XObj hlpType,
		    Path path);

static void
censustakerTimeout(
		   Event	ev,
		   void 	*arg);

static void
censustakerTrigger(
		   XObj self);

void
censustakerSendUpdate(
		      XObj      self,
		      boolean_t full_format);

void
censustakerUpdateState(
		       XObj self);

static xkern_return_t   
LoadMaxNodes(
	     XObj	self,
	     char	**str,
	     int	nFields,
	     int	line,
	     void	*arg);

static xkern_return_t   
LoadTolerance(
	      XObj	self,
	      char	**str,
	      int	nFields,
	      int	line,
	      void	*arg);
