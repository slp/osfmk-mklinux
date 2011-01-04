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
#ifndef DTIME_H
#define DTIME_H

/* ports */

#define REMOTE_TIME_PORT	7124

#if MACH_KERNEL
#define LOCAL_TIME_PORT 	7123
#else
#define LOCAL_TIME_PORT 	7124 
#endif

/* reference clock id's */
#define TIME_REF_CLOCK_BEST	0	/* not implemented */
#define TIME_REF_CLOCK_LOCAL	1
#define TIME_REF_CLOCK_NTP	2


/* local kernel clock objects, in clock_types.h (really) */

#define DISTRIB_CLOCK_0		3
#define DISTRIB_CLOCK_1		4

/* control operations */
#define TIME_USE_PARAMS		(TMP0_CTL * MAXOPS + 0)
#define TIME_SYNC_START		(TMP0_CTL * MAXOPS + 1)
#define TIME_TEST		(TMP0_CTL * MAXOPS + 2)

/* algorithmic parameters */

typedef struct {
  unsigned long		drift_const;	/* = (1 - P) / P */
  tvalspec_t		max_delay_2U;	/* maximum one-way delay (s/nsec) */
  unsigned long		min_delay_min;  /* minimum one-way delay (us) */
  unsigned long		attempt_max_k;	/* number of attempts */
  unsigned long		attempt_ival_W;	/* attempt_ival (ms) */
  unsigned long		bound_ms;	/* master/slave deviation (us) */
  unsigned long		sync_time;	/* k * W * 1000, keep around */ 
} alg_consts_t;


/* retrieve protocol (should be ifdef'ed */
#define DUMP_AREA_SIZE  64 * 1024

#endif /* DTIME_H */

