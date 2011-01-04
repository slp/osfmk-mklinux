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
#include <xkern/include/prot/bootp.h>
#include <xkern/include/prot/dtime.h>

#include <mach_services/servers/s_xkern/protocols/dtime_mgmt/dtime_mgmt_i.h>

static xkern_return_t read_config (XObj self, char** array, int n_fields, 
				   int line, void *arg);

static xkern_return_t read_provide (XObj self, char** array, int n_fields, 
				   int line, void *arg);

static xkern_return_t read_local (XObj self, char** array, int n_fields, 
				   int line, void *arg);

/* different sets of rom are active at various times */
static XObjRomOpt rom_opts[] = {
  {"config", 3, read_config},
  {"provide", 3, read_provide},
  {"local", 10, read_local},
};

static xkern_return_t read_config 
(XObj 		self, 
 char** 	array, 
 int		n_fields, 
 int 		line, 
 void 		*arg)
{

  PState  	*pstate = (PState*)(self->state);

  /* format: 1st element ip host */
  if ( str2ipHost(&(pstate->dft_ipaddr), array[2]) == XK_SUCCESS ) {

    /* optional udp port */
    if ((n_fields == 4) && 
	(sscanf (array[3], "%d", &(pstate->dft_udp_port)) < 1))
      return XK_FAILURE;
  }
  /* format: 1st element ref_clock */
  else {
    int	i = pstate->num_ref_clocks;

    if (sscanf (array[2], "%d", 
		&(pstate->ref_clocks[i].ref_clock_id)) < 1)
      return XK_FAILURE;

    if ( str2ipHost(&(pstate->ref_clocks[i].ip_addr), array[3]) 
	!= XK_SUCCESS) 
      return XK_FAILURE;	

    /* optional udp port */
    if (n_fields == 5) {
      if (sscanf (array[4], "%d", &(pstate->ref_clocks[i].udp_port)) < 1)
	return XK_FAILURE;
    }
    else 
      pstate->ref_clocks[i].udp_port = 0;

    (pstate->num_ref_clocks)++;
  }
  return XK_SUCCESS;
}
	  
static xkern_return_t read_provide
(XObj 		self, 
 char** 	array, 
 int		n_fields, 
 int 		line, 
 void 		*arg)
{
  PState  	*pstate = (PState*)(self->state);
  int 		i = pstate->num_pub_clocks;

  if (sscanf (array[2], "%d", 
	      &(pstate->pub_clocks[i].ref_clock_id)) < 1)
    return XK_FAILURE;
  
  /* optional udp port */
  if (n_fields == 4) {
    if (sscanf (array[3], "%d", &(pstate->pub_clocks[i].udp_port)) < 1)
      return XK_FAILURE;
  }
  else 
    pstate->pub_clocks[i].udp_port = 0;

  pstate->num_pub_clocks ++;
  return XK_SUCCESS;
}

static xkern_return_t read_local
(XObj 		self, 
 char** 	array, 
 int		n_fields, 
 int 		line, 
 void 		*arg)
{

  PState  	*pstate = (PState*)(self->state);
  int 		i = pstate->num_local_clocks;
  alg_consts_t	*alg_consts = &(pstate->local_clocks[i].alg_consts);
  unsigned long	tmp_max_delay_U;

  if (sscanf (array[2], "%d", 
	      &(pstate->local_clocks[i].local_clock_id)) < 1)
    return XK_FAILURE;

  if (sscanf (array[3], "%d", 
	      &(pstate->local_clocks[i].ref_clock_id)) < 1)
    return XK_FAILURE;

  if (sscanf (array[4], "%d", 
	      &(pstate->local_clocks[i].alg_consts.drift_const)) < 1)
        return XK_FAILURE;

  if (sscanf (array[5], "%d", 
	      &(pstate->local_clocks[i].alg_consts.min_delay_min)) < 1)
    return XK_FAILURE;
  
  if (sscanf (array[6], "%d", &tmp_max_delay_U) < 1)
    return XK_FAILURE;
  else { 
    pstate->local_clocks[i].alg_consts.max_delay_2U.tv_sec = 
      (tmp_max_delay_U * 2) / USEC_PER_SEC;

    pstate->local_clocks[i].alg_consts.max_delay_2U.tv_nsec = 
      ((tmp_max_delay_U * 2) % USEC_PER_SEC) * NSEC_PER_USEC;
  }

  if (sscanf (array[7], "%d", 
	      &(pstate->local_clocks[i].alg_consts.attempt_max_k)) < 1)
    return XK_FAILURE;
  
  if (sscanf (array[8], "%d", 
	      &(pstate->local_clocks[i].alg_consts.attempt_ival_W)) < 1)
    return XK_FAILURE;

  if (sscanf (array[9], "%d", 
	      &(pstate->local_clocks[i].alg_consts.bound_ms)) < 1)
    return XK_FAILURE;

  /* set k*W*1000 (MSEC_PER_USEC), so it doesn't need to be recalulated */
  pstate->local_clocks[i].alg_consts.sync_time = 
    pstate->local_clocks[i].alg_consts.attempt_ival_W *
      pstate->local_clocks[i].alg_consts.attempt_max_k * 1000;

  /* verify that bound is sane == is time to next attempt > 0  */
  if ((alg_consts->drift_const * (alg_consts->bound_ms + 
				  alg_consts->min_delay_min - 
				  tmp_max_delay_U))
      <= alg_consts->sync_time)
    {
      printf ("Inconsistent parameters for local clock %d.\n", i);
      return XK_FAILURE;
    }

  pstate->num_local_clocks ++; 

  return XK_SUCCESS;
}
 
xkern_return_t
time_client_init (XObj self)
{

  XObj 		llp, lls;
  PState	*pstate;
  Allocator	mda = pathGetAlloc(self->path, MD_ALLOC);
  Part_s	part[2];
  u_bit32_t	master_ref_clock;
  clock_id_t	local_dest_clock;
  IPhost	master_ipaddr = {0, 0, 0, 0};
  long		master_port, any_port = ANY_PORT;
  int 		i,j, unused;

  xTraceP0 (self, TR_FULL_TRACE, "time_client_init");
  
  llp = xGetDown(self, 0);
  if (!xIsProtocol(llp)) {
    xTraceP0(self, TR_ERRORS, "no lower protocol");
    return XK_FAILURE;
  }

  self->hlpType	= self;
  
  if (!(pstate = allocGet(mda, sizeof(PState)))) {
    xTraceP0(self, TR_ERRORS, "allocation error");
    return XK_NO_MEMORY;
  }
  
  self->state = pstate;	

  /* initialize w/ defaults */
  pstate->dft_ipaddr = master_ipaddr; /* hack == 0 */
  pstate->dft_udp_port = REMOTE_TIME_PORT;  
  pstate->num_ref_clocks = 0;
  pstate->num_pub_clocks = 0;
  pstate->num_local_clocks = 0;

  /* Protocol state has list of all configuration information */

  if (findXObjRomOpts (self, rom_opts, sizeof(rom_opts)/sizeof(XObjRomOpt), NULL) != XK_SUCCESS) {
    xTraceP0(self, TR_ERRORS, "RomOpts");
    return XK_FAILURE;
  }

  /* No bootp support yet. Otherwise, add additional information into
     the pstate. */


  /* OpenEnable llp in order to act as a time master, do this for each 
   * reference clock that is 'provided' for the kernel.  The local rt 
   * clock is enabled by default (NOTDEF).
   */

#ifdef NOTDEF
  if ( pstate->num_pub_clocks == 0) {

    partInit(part,1);
  
    master_ref_clock = TIME_REF_CLOCK_LOCAL;
    master_port = pstate->dft_udp_port;

    partPush(part[0], &master_port, sizeof(long));
    partPush(part[0], &master_ref_clock, sizeof (u_bit32_t)); 

    if (xOpenEnable (self, self, xGetDown(self, 0), part) == XK_FAILURE) {
      xTraceP0(self, TR_ERRORS, "xOpenEnable == XK_FAILURE");
      return XK_FAILURE;
    }
  } 
#endif

  for (i = 0; i < pstate->num_pub_clocks; i++) {

    partInit(part,1);
  
    master_ref_clock = pstate->pub_clocks[i].ref_clock_id;
    master_port = pstate->pub_clocks[i].udp_port ? 
      pstate->pub_clocks[i].udp_port : pstate->dft_udp_port;

    xTraceP2(self, TR_MAJOR_EVENTS, "openeable: ref_clock %d on %d\n",
	    master_ref_clock, master_port);

    partPush(part[0], &master_port, sizeof(long));
    partPush(part[0], &master_ref_clock, sizeof (u_bit32_t)); 

    if (xOpenEnable (self, self, xGetDown(self, 0), part) == XK_FAILURE) {
      xTraceP0(self, TR_ERRORS, "xOpenEnable == XK_FAILURE");
      return XK_FAILURE;
    }
  }

  /* For each local clock, Open() session to appropriate ref clock. */

  for (i = 0; i < pstate->num_local_clocks; i++) {

    partInit(part,2); 
    
    local_dest_clock = pstate->local_clocks[i].local_clock_id;
    master_ref_clock = pstate->local_clocks[i].ref_clock_id;
    
    /* use defaults */
    master_ipaddr = pstate->dft_ipaddr;
    master_port = pstate->dft_udp_port;
    
    /* override w/ ref clock on list */
    for (j = 0; j < pstate->num_ref_clocks; j++) {
      if (pstate->ref_clocks[j].ref_clock_id == master_ref_clock) {
	master_ipaddr = pstate->ref_clocks[j].ip_addr;
	master_port = pstate->ref_clocks[j].udp_port ? 
	  pstate->ref_clocks[j].udp_port : pstate->dft_udp_port;
	break;
      }
    }
    
    xTraceP4(self, TR_MAJOR_EVENTS, "open: local %d, remote %d at %s, %d\n",
	    local_dest_clock, master_ref_clock, ipHostStr(&master_ipaddr), 
	    master_port);
    
    partPush(part[0], &master_ipaddr, sizeof(IPhost));/* remote: master addr */
    partPush(part[0], &master_port, sizeof(long));/* remote: master port */
    partPush(part[0], &master_ref_clock, sizeof(u_bit32_t));/* remote: clock */
    
    partPush(part[1], ANY_HOST, 0); /* local */
    partPush(part[1], &any_port, sizeof(long)); /* local */
    partPush(part[1], &local_dest_clock, sizeof (clock_id_t)); /* local */
    

    /* no need to maintain reference to session (?) */
    lls = xOpen(self, self, xGetDown(self, 0), part, self->path);
    if (lls == ERR_XOBJ) {
      xTraceP0(self, TR_ERRORS, "xOpen");
      return XK_FAILURE;
    }
    
    /* send parameters */
    
    if (xControl(lls, TIME_USE_PARAMS, (char *)&(pstate->local_clocks[i].alg_consts), sizeof(alg_consts_t)) != sizeof(alg_consts_t)) {
      xTraceP0(self, TR_ERRORS, "xControl(params)");	
      return XK_FAILURE;
    }
    
    /* start synchronizing as a client */
    
    if (xControl(lls, TIME_SYNC_START, (char *)&unused, sizeof(int)) < 0) {
      xTraceP0(self, TR_ERRORS, "xControl(start)");	
      return XK_FAILURE;
    }
  }	
  
  xTraceP0 (self, TR_FULL_TRACE, "exit");
  return XK_SUCCESS;
}

