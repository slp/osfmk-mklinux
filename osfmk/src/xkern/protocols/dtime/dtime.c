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

/* This is a more unified implementation of the time protocols.  The
 * master and client protocols are control only. 
 */	

#if MACH_KERNEL
#include <ipc/ipc_port.h>
#include <kern/host.h>
#include <kern/clock.h>
#include <kern/ipc_host.h>
#include <mach/clock_server.h>
#else
#include <mach/clock.h>
#endif

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/dtime.h>
#include <xkern/protocols/dtime/dtime_i.h>

#if XK_DEBUG
int tracetimep;
#endif 

static void
retrieve_write 
(retrieve_t	*retrieve,
 long		code,
 long		data)
{	

  long		*p = (long*)(retrieve->addr);

  *p = code;
  retrieve->addr += (sizeof(long));
  retrieve->prod += (sizeof(long));
  
  if (retrieve->prod >= RETRIEVE_AREA_SIZE) {
    printf ("retrieve wraps\n");
    retrieve->prod = 0;
    retrieve->addr -= RETRIEVE_AREA_SIZE;
  }

  p = (long*)(retrieve->addr);

  *p = data;
  retrieve->addr += (sizeof(long));
  retrieve->prod += (sizeof(long));
  
  if (retrieve->prod >= RETRIEVE_AREA_SIZE) {
    printf ("retrieve wraps\n");
    retrieve->prod = 0;
    retrieve->addr -= RETRIEVE_AREA_SIZE;
  }
}

/* not called */
static xmsg_handle_t
timePush
(XObj		s,
 Msg		msg)
{
  /* The time protcol has no headers to add */
  xTraceP0(self, TR_FULL_TRACE, "timePush");
  xAssert(xIsSession(s));
  return xPush(xGetDown(s, 0), msg);
}
 
static void 
send_request (Event ev, void *arg)
{
  XObj 		self = (XObj)arg;
  Msg_s		msg;
  time_msg_t	*msg_buf;
  kern_return_t	kr;
  tvalspec_t	id_time;
  xmsg_handle_t	msg_hnd;
  PState	*pstate = (PState *)xMyProtl(self)->state;
  SState 	*sstate = (SState *)(self->state);

  if (evIsCancelled(ev)) {
    xTrace0(timep, TR_FULL_TRACE, "dtime: Warning -- sync event cancelled");
    return;
  } 

  xTraceP0(self, TR_FULL_TRACE, "send_request");
  xAssert(xIsSession(self));

  /* Have made too many attempts, give up. */
  if (sstate->attempt_count >= sstate->alg_consts.attempt_max_k) {
    printf ("Time service fails on local clock %d.\n", 
	    sstate->ref_clock_id);
    /* Abort the session. */
    xClose(self);
    return;
  }

  /* only one request/response at a time per session */
  semWait(&sstate->sessionSem);

  /* insure that the session still exists */
  if (evIsCancelled(ev)) {
    xTrace0(timep, TR_FULL_TRACE, "dtime: Warning -- sync event cancelled");
    return;
  } 

  /* Construct and send a time request.  */
  msgConstructContig(&msg, self->path, (long)sizeof(time_msg_t), 0);
  msgForEach(&msg, msgDataPtr, &msg_buf); 
  
  msg_buf->version = TIME_VERSION;
  msg_buf->hlp_num = sstate->hlp_num;
  msg_buf->ref_clock_id = sstate->ref_clock_id;
  msg_buf->local_clock_id = sstate->local_clock_id;
  msg_buf->type = TIME_REQ;

  /* don't worry about blocking here -- part of network delay? */
  kr = clock_get_time (pstate->local_rt, &id_time);	
  if (kr != KERN_SUCCESS) {
    xTraceP1(self, TR_ERRORS,
	     "clock_get_time: %s", mach_error_string(kr));
    /* how to cleanly abort the whole protocol ?  */
  }
  
  sstate->attempt_count++;
  sstate->current_id_time = id_time;
  msg_buf->msg.req.id_time = id_time;

  /* Schedule the next send_request.  Ideally, you want to know how
   * "late".  Perhaps one can use the id_time.  Or is it just better
   * to schedule early?  
   */

  evSchedule(sstate->sync_event, send_request,(void *)self,
	     sstate->alg_consts.attempt_ival_W * 1000);

  msg_hnd = xPush(xGetDown(self, 0), &msg);
  if (msg_hnd == XMSG_ERR_HANDLE ||
      msg_hnd == XMSG_ERR_WOULDBLOCK) {
    xTraceP0(self, TR_ERRORS, 
	     "xPush");
  }

  msgDestroy(&msg);

  semSignal(&sstate->sessionSem);

  xTraceP0(self, TR_FULL_TRACE, "exit send_request");  
}	


static void 
send_response
(XObj	self,
 XObj	lls,
 Msg	empty_msg,
 void	*inhdr)
{
  time_msg_t	*req_buf = (time_msg_t*) inhdr;
  Msg_s		msg;
  time_msg_t	*resp_buf;
  xmsg_handle_t	msg_hnd;
  kern_return_t	kr;
  PState	*pstate = (PState *)xMyProtl(self)->state;
  SState 	*sstate = (SState *)(self->state);


  xAssert(xIsSession(self));

  semWait(&sstate->sessionSem);
  msgConstructContig(&msg, self->path, (long)sizeof(time_msg_t), 0);
  msgForEach(&msg, msgDataPtr, &resp_buf); 
  
  /* these could also come from msg */
  resp_buf->version = TIME_VERSION;
  resp_buf->hlp_num = sstate->hlp_num;
  resp_buf->ref_clock_id = sstate->ref_clock_id;
  resp_buf->local_clock_id = sstate->local_clock_id;
  resp_buf->type = TIME_RESP;

  resp_buf->msg.resp.id_time = req_buf->msg.req.id_time;

  /* don't care about losing control here -- part of network delay*/	
  kr = clock_get_time (pstate->local_rt, &(resp_buf->msg.resp.master_time));
  if (kr != KERN_SUCCESS) {
    xTraceP1(self, TR_ERRORS,
	     "clock_get_time: %s", mach_error_string(kr));
    /* cleanly abort protocol ? */
    return;
  }

  msg_hnd = xPush(xGetDown(self, 0), &msg);
  if (msg_hnd == XMSG_ERR_HANDLE ||
      msg_hnd == XMSG_ERR_WOULDBLOCK) {
    xTraceP0(self, TR_ERRORS,
	     "xPush");
  }

  msgDestroy(&msg);

  semSignal(&sstate->sessionSem);
}


static void 
validate_response
(XObj	self,
 XObj	lls,
 Msg	empty_msg,
 void	*inhdr)
{

  time_msg_t	*msg_buf = (time_msg_t *) inhdr;
  PState	*pstate = (PState *)xMyProtl(self)->state;
  SState 	*sstate = (SState *)(self->state);

  kern_return_t	kr;
  
  tvalspec_t	rcv_time;
  u_bit32_t	D, DNS;


  xAssert(xIsSession(self));

  /* only process one reply to this session at a time */
  semWait(&sstate->sessionSem);

  /* check ID here */
  if (CMP_TVALSPEC(&(sstate->current_id_time), &(msg_buf->msg.resp.id_time))) {
    xTraceP0(self, TR_MAJOR_EVENTS, "out of sequence packet rejected");
    semSignal(&pstate->serviceSem);
    return;
  }
  
  /* we care a lot about losing control between here... */

  kr = clock_get_time (pstate->local_rt, &rcv_time);	
  if (kr != KERN_SUCCESS) {
    xTraceP1(self, TR_ERRORS,
	     "clock_get_time: %s", mach_error_string(kr));
    /* abort session */
    return;
  }

  /* since we have the session lock, we don't need to make other checks */

  /* If valid, set local distributed clock and schedule next attempt */

  SUB_TVALSPEC (&rcv_time, &(msg_buf->msg.resp.id_time));

  if (CMP_TVALSPEC(&rcv_time, &sstate->alg_consts.max_delay_2U) < 0) {
    
    /* Estimate the time on the master as master_time + (rcv_time /2),
     * note that nsec can't overflow.
     */
    rcv_time.tv_nsec = rcv_time.tv_nsec / 2;
    rcv_time.tv_nsec += ((rcv_time.tv_sec % 2) * (NSEC_PER_SEC / 2));
    rcv_time.tv_sec = rcv_time.tv_sec / 2;
    ADD_TVALSPEC(&(msg_buf->msg.resp.master_time), &rcv_time);

    /* Send master time to the kernel. */
    kr = clock_set_time (sstate->local_dclock_ctl, msg_buf->msg.resp.master_time);
    if (kr != KERN_SUCCESS) {
      xTraceP1(self, TR_ERRORS, 
	       "clock_set_time: %s", mach_error_string(kr));
      retrieve_write(&(pstate->retrieve), 
		     RT_MASTER_MS * 10000 + sstate->local_clock_id * 100 +
		     sstate->ref_clock_id, 
		     msg_buf->msg.resp.master_time.tv_sec * 1000 + 
		     msg_buf->msg.resp.master_time.tv_nsec / 1000000);
      return;
    }

    /* Calculate delay until next synchronization.  The clock has been set to 
     * a certain accuracy (+- (D - min)) and will drift at rate p until it
     * has potentially violated the bound.  Allow time to make k attempts 
     * spaced W apart before this happens.
     * Want to allow for the delay in scheduling the event when
     * finding DNS.
     */

    /* DNS = (p * (bound - ((rt_time / 2) - min))) - kW */

    D = (rcv_time.tv_sec * USEC_PER_SEC) + (rcv_time.tv_nsec / NSEC_PER_USEC);
    D += 1; /*compensate for roundoff error */

    DNS = (sstate->alg_consts.drift_const * (sstate->alg_consts.bound_ms - 
					     D +
					     sstate->alg_consts.min_delay_min))
      - sstate->alg_consts.sync_time;

    /* Cancel pending attempt from last synchronization and schedule
     * next synchronization.  Cancel will succeed becuase of
     * sessionSem (?). 
     */

    evCancel(sstate->sync_event);
    evSchedule(sstate->sync_event, send_request, (void *)self, DNS);

    /* ...and here */

    /* reset current id and attempt count.  Session won't be reused
     * until lock is released, so there is no race.
     */

    sstate->attempt_count = 0;
    sstate->current_id_time.tv_sec = 0;
    sstate->current_id_time.tv_nsec = 0;
  
    retrieve_write(&(pstate->retrieve), 
		   RT_DELAY_CODE * 10000 + sstate->local_clock_id * 100 +
		   sstate->ref_clock_id, D);
    retrieve_write(&(pstate->retrieve), 
		   RT_DNS_CODE * 10000 + sstate->local_clock_id * 100 +
		   sstate->ref_clock_id, DNS);
  }
  /* Response was not timely. */
  else {
    retrieve_write(&(pstate->retrieve), 
		   RT_TOO_LONG_CODE * 10000 + sstate->local_clock_id * 100 +
		   sstate->ref_clock_id, D);
  }

  semSignal(&sstate->sessionSem);
}


static xkern_return_t
timePop
(XObj	self,
 XObj	lls,
 Msg	empty_msg,	/* all data in header */
 void	*inhdr)
{	
  time_msg_t	*mdata = (time_msg_t *)inhdr;

    xTraceP0(self, TR_ERRORS,
	     "timePop");
  xAssert(xIsSession(self));

  switch (mdata->type) {
  case (TIME_REQ):
    {
      IPhost	req;
      xControl(lls, GETPEERHOST, (char *)&req, sizeof (IPhost));
      xTraceP3(self, TR_EVENTS, 
	       "time request: for ref_clock %d from %s, local clock %d\n",
	      mdata->ref_clock_id, ipHostStr(&req), mdata->local_clock_id);
      send_response (self, lls, empty_msg, inhdr);
    }
    break;
  case (TIME_RESP):
    {
      IPhost	resp;
      xControl(lls, GETPEERHOST, (char *)&resp, sizeof (IPhost));
      xTraceP3(self, TR_EVENTS,
	       "time response: for local clock %d from %s, ref_clock %d\n",
	       mdata->local_clock_id, ipHostStr(&resp), mdata->ref_clock_id );
      validate_response (self, lls, empty_msg, inhdr);
    }
    break;
  default: 
    {
    xTraceP0(self, TR_ERRORS,
	     "unknown msg type");
      return XK_FAILURE;
    }
    break;
  }
  return XK_SUCCESS;
}

#define MIN(x,y) ((x) < (y) ? (x) : (y))

static 	int
timeControlProt
(XObj 		self,
 int 		opcode,
 char 		*buf,
 int		len)
{
  PState	*pstate = (PState *)(self->state);
  retrieve_t	*retrieve = &(pstate->retrieve);

  xTraceP0(self, TR_FULL_TRACE, "timeControlProt");
  switch (opcode) {

  case TIME_TEST:
    {
      return 0;
    }
  case (10103<<16):	/* hard coded for retreive */
    {
      int size;
      
      if (retrieve->prod > retrieve->cons) {
	size = MIN(len, (retrieve->prod - retrieve->cons));
      } else if (retrieve->prod < retrieve->cons) {
	size = MIN(len, (RETRIEVE_AREA_SIZE - retrieve->cons));
      } else
	return 0;

      size &= ~3;
      memcpy(buf, retrieve->addr - retrieve->prod + retrieve->cons, size);
      retrieve->cons += size;
      retrieve->cons %= RETRIEVE_AREA_SIZE;
      return size;
    }

  default:
    return xControl(xGetDown(self, 0), opcode, buf, len);	
  }
}


static 	int
timeControlSessn
(XObj 		self,
 int 		opcode,
 char 		*buf,
 int		len)
{

  PState	*pstate = (PState *)xMyProtl(self)->state;
  SState 	*sstate = (SState*)(self->state);
  retrieve_t	*retrieve = &(pstate->retrieve);

  xTraceP0(self, TR_FULL_TRACE, "timeControlSessn");
  xAssert(xIsSession(self));

  switch (opcode) {
  case TIME_USE_PARAMS:
    {
      kern_return_t	kr;
      mach_msg_type_number_t	count = 1;

      xTraceP0(self, TR_EVENTS, "TIME_USE_PARAMS");      
      checkLen(len, sizeof (alg_consts_t));

      /* Set parameters in session state */
      bcopy(buf, (char *) &(sstate->alg_consts), sizeof (alg_consts_t));
      
      /* and pass bound and drift values to the kernel clock. */
      kr = clock_set_attributes (sstate->local_dclock_ctl, 
				 DCLOCK_PARAM_BOUND, 
				 (clock_attr_t) &(sstate->alg_consts.bound_ms),
				 count);
      if (kr != KERN_SUCCESS) {
	xTraceP1(self, TR_ERRORS, 
		 "clock_set_attributes (bound) : %s", mach_error_string(kr));
	return -1;
      }
      kr = clock_set_attributes (sstate->local_dclock_ctl, 
		 		 DCLOCK_PARAM_DRIFT_CONST, 
				 (clock_attr_t) &(sstate->alg_consts.drift_const), 
				 count);
      if (kr != KERN_SUCCESS) {
	xTraceP1(self, TR_ERRORS, 
		 "clock_set_attributes (drift) : %s", mach_error_string(kr));
	return -1;
      }
      return len;
      break;
    }
  case TIME_SYNC_START:
    {	
      /* There may be a delay before we actually start, but that's OK */
      xTraceP0(self, TR_EVENTS, "TIME_SYNC_START");
      evSchedule(sstate->sync_event, send_request, (void *)self, 0); 
      return 0;
      break;
    }
  case (10103<<16):	/* hard coded for retreive */
    {
      int size;
      
      if (retrieve->prod > retrieve->cons) {
	size = MIN(len, (retrieve->prod - retrieve->cons));
      } else if (retrieve->prod < retrieve->cons) {
	size = MIN(len, (RETRIEVE_AREA_SIZE - retrieve->cons));
      } else
	return 0;

      size &= ~3;
      memcpy(buf, retrieve->addr - retrieve->prod + retrieve->cons, size);
      retrieve->cons += size;
      retrieve->cons %= RETRIEVE_AREA_SIZE;
      return size;
    }
  default:
    return xControl(xGetDown(self, 0), opcode, buf, len);	
  }
}

extern xkern_return_t timeClose (XObj self);

xkern_return_t
timeClose (XObj self)
{
  PState	*pstate = (PState *)((xMyProtl(self))->state);
  SState	*sstate = (SState *)(self->state);
  
  xTraceP0(self, TR_FULL_TRACE, "timeClose");
  xAssert(xIsSession(self));

   xTraceP2(self, TR_MAJOR_EVENTS, 
	   "closing session: local = %d, ref_clock = %d\n",
	   sstate->local_clock_id, sstate->ref_clock_id);

  /* inform the kernel clock object */
  {
    kern_return_t		kr;
    int				fail = 3; 
    mach_msg_type_number_t	count = 1;

    kr = clock_set_attributes (sstate->local_dclock_ctl, DCLOCK_STATE, 
			       (clock_attr_t) &fail, count);
    if (kr != KERN_SUCCESS) {
      xTraceP1(self, TR_ERRORS, 
	       "clock_set_attributes (FAILED) : %s", mach_error_string(kr));
      printf("Warning: Could not fail dclock %s.\n", sstate->local_clock_id);
      return XK_FAILURE;
    }
  }

  /* pending synchronization */
  evCancel(sstate->sync_event);
  evDetach(sstate->sync_event);

  /* inform the master */
  
  /* map */
  if ( mapRemoveBinding(pstate->active_map, self->binding) == XK_FAILURE ) {
    xTraceP0(self, TR_ERRORS,
	     "mapRemoveBinding");
    return XK_FAILURE;
  }

  xClose(xGetDown(self,0));
  xDestroy(self);

  return XK_SUCCESS;  
}

static xkern_return_t
time_sessn (XObj s) 
{ 
  s->push = timePush; 
  s->pop = timePop;
  s->close = timeClose;
  s->control = timeControlSessn;
  return XK_SUCCESS;
}

static XObj
timeCreateSessn
(XObj		self,
 XObj		hlp_Rcv,
 XObj		hlp_Type,
 active_key_t	*key,
 Path		path)
{

  XObj		p_session;
  Allocator	mda = pathGetAlloc(self->path, MD_ALLOC);
  PState	*pstate = self->state;
  SState	*sstate;

  kern_return_t			kr;
  long				dclock_state;
  mach_msg_type_number_t	count = 1;

  xTraceP0(self, TR_FULL_TRACE, "timeCreateSessn");

  semWait(&pstate->serviceSem);
  
  /* was session created during semWait? */
  if (mapResolve(pstate->active_map, key, &p_session) != XK_FAILURE) {
    xTraceP4(self, TR_MAJOR_EVENTS, 
	     "race session %x: ref = %d, loc = %d, lls = %x\n",
 	     (unsigned long)p_session, key->ref_clock_id,key->local_clock_id,
	     (unsigned long)key->lls);
    return p_session;
  }
  
  if ((p_session = xCreateSessn(time_sessn, hlp_Rcv, hlp_Type, self, 1, &(key->lls), path)) == ERR_XOBJ) {
    xTraceP0(self,TR_ERRORS,
	     "xCreateSessn");
    return ERR_XOBJ;
  }

  xTraceP4(self, TR_MAJOR_EVENTS, 
	   "new session %x: ref = %d, loc = %d, lls = %x\n", 
	   (unsigned long)p_session, key->ref_clock_id, key->local_clock_id,
	   (unsigned long)key->lls);

  if (mapBind(pstate->active_map, key, p_session, &p_session->binding) 
      != XK_SUCCESS){
    xTraceP0(self, TR_ERRORS,
	     "mapBind");
    xDestroy(p_session); 
    return ERR_XOBJ;
  }

  semSignal(&pstate->serviceSem);

  if (!(sstate = pathAlloc(path, sizeof(SState)))) {
    xTraceP0(self, TR_ERRORS,
	     "allocation error");
    xDestroy(p_session); 
    return ERR_XOBJ;
  }

  p_session->state = sstate;

  semInit(&sstate->sessionSem, 1);

  if (!(sstate->sync_event = evAlloc(path))) {	
    xTraceP0(self, TR_ERRORS,
	     "sync event allocation failed");
    xDestroy(p_session); 
    return ERR_XOBJ;
  }

  sstate->hlp_num = relProtNum(hlp_Type,self);
  sstate->ref_clock_id = key->ref_clock_id;
  sstate->local_clock_id = key->local_clock_id; 
  
  kr = host_get_clock_control(pstate->host, DISTRIBUTED_CLOCK, 
			      &(sstate->local_dclock_ctl));
  if (kr != KERN_SUCCESS) {
    xTraceP1(self, TR_ERRORS,
	     "host_get_clock_service: %s", mach_error_string(kr));
    xDestroy(p_session);
    return ERR_XOBJ;	
  }	

  /* If the clock has previously failed, tell kernel to UNINITIALIZE. */
  kr = clock_get_attributes (sstate->local_dclock_ctl, DCLOCK_STATE, 
			     (clock_attr_t) &dclock_state, &count);
  if (kr != KERN_SUCCESS) {
    xTraceP1(self, TR_ERRORS,
	     "clock_get_attributes: %s", mach_error_string(kr));
    xDestroy(p_session);
    return ERR_XOBJ;	
  }	

  if (dclock_state == FAILED) {
    dclock_state = UNINITIALIZED;
    kr = clock_set_attributes (sstate->local_dclock_ctl, DCLOCK_STATE, 
			       (clock_attr_t) &dclock_state, count);
    if (kr != KERN_SUCCESS) {
    xTraceP1(self, TR_ERRORS,
	     "clock_set_attributes: %s", mach_error_string(kr));
      xDestroy(p_session);
      return ERR_XOBJ;	
    }	
  }  
  
  /* There is a race condition. */
  if (dclock_state != UNITIALIZED) {
    printf ("Distributed clock is already being managed.\n");
    xDestroy(p_session);
    return ERR_XOBJ;	
  }
    
  sstate->attempt_count = 0;
  sstate->current_id_time.tv_sec = 0;
  sstate->current_id_time.tv_nsec = 0;

  xTraceP0(self, TR_FULL_TRACE, "exit timeCreateSessn");
  return p_session;
} 

static XObj
timeOpen
(XObj		self,
 XObj		hlp_Rcv,
 XObj		hlp_Type,
 Part		part,
 Path		path)
{

  Map		active_map = ((PState *)(self->state))->active_map;
  active_key_t	key;
  u_bit32_t	*master_ref_clock;
  clock_id_t	*client_dest_clock;
  XObj		lls, session;

  xTraceP0(self, TR_FULL_TRACE, "timeOpen");
  xAssert(xIsProtocol(xGetDown(self, 0)));

  key.ref_clock_id = *(u_bit32_t *) partPop(part[0]);     /* remote */	
  key.local_clock_id  = *(clock_id_t *) partPop(part[1]); /* local */

  if ((lls = xOpen(self, self, xGetDown(self, 0), part, path)) 
      == ERR_XOBJ) {
    xTraceP0(self, TR_ERRORS,
	     "xOpen == ERR_XOBJ");
    return ERR_XOBJ;
  }
  
  key.lls = lls;

  if (mapResolve(active_map, &key, &session) == XK_FAILURE) {
    session = timeCreateSessn(self, hlp_Rcv, hlp_Type, &key, path);
    if (session == ERR_XOBJ) {
    xTraceP0(self, TR_ERRORS,
	     "CreateSessn == ERR_XOBJ");
      xClose(lls);
      return ERR_XOBJ;
    }
    else {
      xTraceP0(self, TR_FULL_TRACE, "exit");
      /* in this case, we don't want to duplicate the session, otherwise we
	 can't close it ? */
      xDuplicate (lls);
      return session;
    }
  }
  else {
    xTraceP0(self, TR_FULL_TRACE, "Session exists");
    xClose(lls);
    return ERR_XOBJ;
  }    
}

static xkern_return_t
timeOpenEnable 
(XObj 		self,
 XObj 		hlpRcv,
 XObj 		hlpType,
 Part		part)
{

  passive_key_t	key;
  PState	*ps = self->state;

  xTraceP0(self, TR_FULL_TRACE, "timeOpenEnable");

  /* actually openEnable for own map and for llp */
  key.hlp_num = relProtNum(hlpType,self);

  /* get our participant */
  key.ref_clock_id = *((u_bit32_t *) partPop(part[0])); 
  if (defaultOpenEnable (ps -> passive_map, hlpRcv, hlpType, &key) 
      != XK_SUCCESS) {
    xTraceP0(self, TR_ERRORS,
	     "defaultOpenEnable");
    return XK_FAILURE;
  }
  
  /* pass udp information down */
  if (xOpenEnable (self, self, xGetDown(self, 0), part) == XK_FAILURE) {
    xTraceP0(self, TR_ERRORS,
	     "xOpenEnable == XK_FAILURE");
    return XK_FAILURE;
  }
  
  return XK_SUCCESS;
}

static xkern_return_t
timeOpenDone
(XObj self,
 XObj	lls,
 XObj	llp,
 XObj	hlpType,
 Path	path)
{	
  xTraceP0(self, TR_FULL_TRACE, "timeOpenDone");
  return XK_SUCCESS;
}

extern xkern_return_t timeOpenDisable (XObj self, XObj hlpRcv, XObj
hlpType, Part part);

xkern_return_t
timeOpenDisable 
(XObj 		self,
 XObj 		hlpRcv,
 XObj 		hlpType,
 Part		part)
{
  passive_key_t	key;
  PState	*ps = self->state;

  xTraceP0(self, TR_FULL_TRACE, 
	   "timeOpenDisable");

  key.hlp_num = relProtNum(hlpType,self);
  key.ref_clock_id = *((u_bit32_t *) partPop(part[0]));
  return defaultOpenDisable (ps->passive_map, hlpRcv, hlpType, &key);
}

extern xkern_return_t timeOpenDisableAll (XObj self, XObj hlpRcv);

xkern_return_t
timeOpenDisableAll
(XObj 		self,
 XObj 		hlpRcv)
{
  PState	*ps = self->state;

  xTraceP0(self, TR_FULL_TRACE, "timeOpenDisableAll");
  return defaultOpenDisableAll(ps->passive_map, hlpRcv, 0); 
  return XK_SUCCESS;
}

static long
timeMsgLoad
(void *to, 
 char *from,
 long len,
 void *ignore)
{
  xAssert(len == sizeof(time_msg_t));
  bcopy(from, to, len); 
  return len;	
}

static void
timeMsgStore
  (void *to,
   char *from,
   long len,
   void* ignore)
{
  xAssert(len == sizeof(time_msg_t));   
  bcopy(to, (char*)from, len);
  return;
}

extern xkern_return_t timeDemux (XObj self, XObj ds, Msg msg);

xkern_return_t
timeDemux 
(XObj		self,
 XObj		ds,
 Msg 		msg)
{
  Map		active_map = ((PState *)(self->state))->active_map;  
  Map		passive_map = ((PState *)(self->state))->passive_map;  
  XObj		session;
  active_key_t	active_key;
  passive_key_t	passive_key;
  Enable	*enable;
  time_msg_t	data;

  xTraceP0(self, TR_FULL_TRACE, "timeDemux");

  xAssert(xIsProtocol(self));

  if (msgPop(msg, timeMsgLoad, (void*)&data, sizeof(time_msg_t), NULL) 
      != XK_SUCCESS){
    xTraceP0(self, TR_ERRORS,
	     "msgPop");
    return XK_FAILURE;
  }
  
  if (data.version != TIME_VERSION) {
    xError("timeDemux: did not recognize message version");
    return XK_SUCCESS;
  }
  
  active_key.lls = ds;
  active_key.ref_clock_id = data.ref_clock_id;
  active_key.local_clock_id = data.local_clock_id;

  xTraceP3(self, TR_EVENTS,"mapping: ref = %d, local = %d, lls = %x\n",
	 data.ref_clock_id, data.local_clock_id, (unsigned long)ds);

  if (mapResolve (active_map, &active_key, &session) != XK_SUCCESS) {
    passive_key.hlp_num = data.hlp_num;
    passive_key.ref_clock_id = data.ref_clock_id;
    if (mapResolve (passive_map, &passive_key, &enable) != XK_SUCCESS) {
      xTraceP0(self, TR_ERRORS,
	       "unknown hlp");
      return XK_FAILURE;
    }

    session = timeCreateSessn(self, enable->hlpRcv, enable->hlpType, &active_key, msgGetPath(msg));
    if (session == ERR_XOBJ) {
    xTraceP0(self, TR_ERRORS,
	     "new session");
      return XK_FAILURE;
    }

    /* xDuplicate() needed ? session or ds */
    xDuplicate (session);
    xDuplicate (ds);

    if (xOpenDone(enable->hlpRcv, session, self, msgGetPath(msg)) != XK_SUCCESS) {
    xTraceP0(self, TR_ERRORS,
	     "Opendone");
      return XK_FAILURE;
    }
  }
  else {
    xTraceP1(self, TR_EVENTS, "existing session: %x\n",
	     (unsigned long)session);
  }

  /* having found or created session, xPop to it */
  return xPop (session, ds, msg, &data);
}

extern xkern_return_t	time_init (XObj	self);

xkern_return_t
time_init (XObj self)
{

  XObj			llp;
  PState		*pstate;
  Allocator		mda = pathGetAlloc(self->path, MD_ALLOC);
  Part_s		part[1];
  long			time_port = LOCAL_TIME_PORT;
  Map			active_map, passive_map;
  kern_return_t		kr;

  xTraceP0 (self, TR_FULL_TRACE, "time_init");

  llp = xGetDown(self, 0);
  if (!xIsProtocol(llp)) {
    xTraceP0(self, TR_ERRORS,
	     "no lower protocol");
    return XK_FAILURE;
  }

  self->open = timeOpen;
  self->demux = timeDemux;
  self->opendone = timeOpenDone;
  self->control = timeControlProt;
  self->openenable = timeOpenEnable;
  self->opendisable = timeOpenDisable;
  self->opendisableall = timeOpenDisableAll;
  self->hlpType = self;

  if (!(pstate = pathAlloc(self->path, sizeof(PState)))) {
    xTraceP0(self, TR_ERRORS,
	     "allocation error");
    return XK_NO_MEMORY;
  }

  /* Initialize semaphor */
 
  semInit(&pstate->serviceSem, 1);

  /* Create maps. */

  pstate->active_map = mapCreate (TIME_ACTIVE_MAP_SIZE, sizeof (active_key_t), self->path);
  
  pstate->passive_map = mapCreate (TIME_PASSIVE_MAP_SIZE, sizeof (passive_key_t), self->path);

  if (!passive_map || !active_map) {
    xTraceP0(self, TR_ERRORS,
	     "allocation error");
    pathFree(pstate);
    return XK_NO_MEMORY;
  }

  /* Get local reference clock, i.e. the Mach REALTIME_CLOCK kernel
   * clock object (different types in kernel and user space).  
   */

  xTraceP0(self, TR_FULL_TRACE, "mach clock");

#if MACH_KERNEL
  {
    ipc_port_t		sright;
    extern host_data_t	realhost;

    sright = ipc_port_make_send(realhost.host_self);
    pstate->host = convert_port_to_host(sright);
  }
#else  
  {
    mach_port_t		task, boot, devpriv, junk;

    task = mach_task_self();

    kr = task_get_bootstrap_port (task, &boot);
    if (kr != KERN_SUCCESS) {
      printf ("task_get_bootstrap_port: %s\n", mach_error_string(kr));
      return ;
    }
    
    kr = bootstrap_ports (boot, &(pstate->host), &devpriv, &junk, &junk, 
			  &junk);
    if (kr != KERN_SUCCESS) {
      printf ("bootstrap_privileged_ports: %s\n", mach_error_string(kr));
      return ;
    }
  }
#endif 

  xAssert(pstate->host != HOST_NULL);
  
  kr = host_get_clock_service(pstate->host, REALTIME_CLOCK, &(pstate->local_rt));
  if (kr != KERN_SUCCESS) {
    xTraceP1(self, TR_ERRORS,
	     "host_get_clock_service: %s", mach_error_string(kr));
    pathFree(pstate);
    return XK_FAILURE;	
  }	
  
  /* initialize the data store for the retrieve protocol (ifdef?) */

  pstate->retrieve.prod = 0;
  pstate->retrieve.cons = 0;
#if MACH_KERNEL
  (void)kmem_alloc(kernel_map, (vm_offset_t *)(&pstate->retrieve.addr), 
		   RETRIEVE_AREA_SIZE);
#else   
  pstate->retrieve.addr = (char *)pathAlloc(self->path, RETRIEVE_AREA_SIZE);
#endif  
  
  if (!(pstate->retrieve.addr)) {
    xTraceP0(self, TR_ERRORS,
	     "allocation error");
    return XK_NO_MEMORY;
  }

  self->state = pstate;

  xTraceP0 (self, TR_FULL_TRACE, "exit");
  return XK_SUCCESS;
}


