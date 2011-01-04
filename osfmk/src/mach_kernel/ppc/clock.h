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
#ifndef _PPC_CLOCK_H_
#define _PPC_CLOCK_H_

#include <machine/mach_param.h>

#define CLK_SPEED	0.0000012766	/* time to complete a clock (3 MHz) */

#if HZ == 120
#  define CLK_INTERVAL	6528	/* clocks to hit CLK_TCK ticks per sec */
#elif HZ == 100
#  define CLK_INTERVAL	7833	/* clocks to hit CLK_TCK ticks per sec */
#elif HZ == 60
#  define CLK_INTERVAL	13055	/* clocks to hit CLK_TCK ticks per sec */
#else
#error "unknown clock speed"
#endif
			/* 6528 for 119.998 Hz. */
                        /* 7833 for 100.004 Hz */
			/* 13055 for 60.002 Hz. */
#define CLK_INTH	(CLK_INTERVAL >> 8)
#define CLK_INTL	(CLK_INTERVAL & 0xff)

#define	SECDAY	((unsigned)(24*60*60))
#define	SECYR	((unsigned)(365*SECDAY + SECDAY/4))

#endif /* _PPC_CLOCK_H_ */
