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
 */

/*
 * MkLinux
 */

#include <machine/conf.h>

/*
 * Code to acquire time via timers can be inlined (using macros)
 * if TIME_MACROS is set to 1 or called inside a subroutine
 * if TIME_MACROS is set to 0.
 *
 * Using macros increases code size by 30 %.
 * The test overhead is deduced from the elapsed time value.
 * Do the use of routines instead of a macros should not influence
 * the results.
 * (see include/time_services.h & lib/time.c)
 */

#define TIME_MACROS	0

/*
 * Two macros named MACH_CALL and MACH_FUNC are generaly used
 * to invoke mach services. In case of service failure, these macros
 * automaticaly report on the location of the call and the error. 
 *
 * When CALL_TRACE is set to 1, if the -trace option is specified 
 * a trace of these calls and functions is displayed
 * (See mach/services.h)
 */

#define CALL_TRACE	1

/*
 * The test_device device is optional
 */

#define TEST_DEVICE	1

/*
 * a couple of constants used in command line evaluation
 */

#define	LINE_LENGTH 	100
#define MAX_ARGS       	32

/* and for synthetic benchmarking */

#define MAX_BACKGROUND_TASKS 64

/* finally, for storage of `procs' used for synthetic benchmarking */

#define MAX_PROCS 32 /* number of procs */
#define MAX_CMDS 512 /* max number of stored command lines in all procs */

/*
 * Default clock 
 *	REALTIME_CLOCK  0: Realtime clock
 *      BATTERY_CLOCK	1: Battery clock
 *      HIGHRES_CLOCK	2: High resolution clock
 *                      3: User provided clock
 */

#ifndef DEFAULT_CLOCK
#define DEFAULT_CLOCK 	REALTIME_CLOCK
#endif /* DEFAULT_CLOCK */
