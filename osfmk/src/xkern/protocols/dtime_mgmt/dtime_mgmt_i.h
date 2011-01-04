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
/* An alternative would be to use the array index as the id, but there
 * may be a significant number of gaps in the sequence, and the state
 * information should be kept small. 
 */

typedef struct 	{
  u_bit32_t	ref_clock_id;
  IPhost	ip_addr;
  long		udp_port;
} ref_clock_t;

#define	NUM_REF_CLOCKS		4	/* clocks we read */
#define NUM_PUB_CLOCKS		4	/* clocks we publish */

typedef struct {
  clock_id_t	local_clock_id;
  u_bit32_t	ref_clock_id;
  alg_consts_t	alg_consts;
} local_clock_t;

#define NUM_LOCAL_CLOCKS 	4	/* clocks we correct */

typedef struct {
  /* config lines */
  IPhost	dft_ipaddr;
  long		dft_udp_port;
  int		num_ref_clocks;
  ref_clock_t	ref_clocks[NUM_REF_CLOCKS];

  /* provide lines */
  int 		num_pub_clocks;
  ref_clock_t	pub_clocks[NUM_PUB_CLOCKS];

  /* local lines */
  int		num_local_clocks;
  local_clock_t	local_clocks[NUM_LOCAL_CLOCKS];
} PState;	
