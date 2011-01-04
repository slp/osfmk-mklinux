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

#ifndef DTIME_I_H
#define DTIME_I_H

#define TIME_VER_1_1		((1<<16) + 1)
#define TIME_VER_1_0		((1 << 16) + 0)

#define TIME_VERSION		TIME_VER_1_1

/* pick some numbers */

#define TIME_PASSIVE_MAP_SIZE 10
#define TIME_ACTIVE_MAP_SIZE  10

/* map keys */

typedef struct {
  XObj		lls;	
  u_bit32_t	ref_clock_id;  		/* (remote) source clock */
  clock_id_t	local_clock_id;		/* (local) destination clock */
} active_key_t;

typedef struct {
  u_bit32_t 	hlp_num;
  u_bit32_t	ref_clock_id;
} passive_key_t;


/* for retrieve area */  

#define RETRIEVE_AREA_SIZE	1024 	 /* 4 byte aligned */

#define RT_DELAY_CODE		1
#define RT_TOO_LONG_CODE	2
#define RT_DNS_CODE		3
#define RT_MASTER_MS		4

typedef struct {
    unsigned long prod;
    unsigned long cons;
    char          *addr;
} retrieve_t;

/* protocol and session state */

typedef struct {
  xkSemaphore	serviceSem;
  Map		active_map;
  Map  		passive_map;
#if MACH_KERNEL
  clock_t	local_rt;
  host_t	host;
#else
  mach_port_t	local_rt;
  mach_port_t	host;
#endif
  retrieve_t	retrieve;
} PState;	

typedef struct {
  xkSemaphore	sessionSem;
  u_bit32_t	hlp_num;
  u_bit32_t	ref_clock_id;
  clock_id_t	local_clock_id;
#if MACH_KERNEL
  clock_t	local_dclock_ctl;
#else
  mach_port_t	local_dclock_ctl;
#endif
  Event		sync_event;
  tvalspec_t	current_id_time;
  u_bit32_t	attempt_count;
  alg_consts_t	alg_consts;
} SState;

/* time request and response messages */

typedef struct {
  tvalspec_t	id_time;
} time_req_t;

typedef struct {
  tvalspec_t	id_time;
  tvalspec_t	master_time;
} time_resp_t;

/* Type is implied by high level protocol (master/client), but 
 * other data, e.g. a particular source or destination clock
 * could be also included.
 */

typedef enum {
  TIME_REQ,
  TIME_RESP
} time_msg_type_t;


/* It would be better to have a variable length message, different
 * reference clocks and clock object use different information?
 */

typedef struct {
  u_bit32_t		version;
  u_bit32_t		hlp_num;
  u_bit32_t		ref_clock_id;
  clock_id_t		local_clock_id;
  time_msg_type_t	type;
  union {
    time_req_t		req;
    time_resp_t		resp;
  } msg;
} time_msg_t;


#endif /* DTIME_I_H */
