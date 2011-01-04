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

#include <sys/timers.h>
#include <errno.h>
#include <mach.h>
#include <mach/clock.h>
#include <mach/mach_traps.h>

static mach_port_t realtime_clock_port;

/* Return a clock value for the given clock type.  We accept only TIMEOFDAY,
   as this is the only value defined by POSIX.  Currently we return just the
   Mach real-time clock value, but we have no way of knowing what the zero
   time for that clock is.  So all this function guarantees is that the
   difference between invocations corresponds to the elapsed time.  */
int
getclock(int clktype, struct timespec *val)
{
    tvalspec_t now;

    if (clktype != TIMEOFDAY)
	return EINVAL;
    if (realtime_clock_port == MACH_PORT_NULL) {
	if (host_get_clock_service(mach_host_self(), REALTIME_CLOCK,
				   &realtime_clock_port) != KERN_SUCCESS)
	    return EIO;
    }
    if (clock_get_time(realtime_clock_port, &now) != KERN_SUCCESS)
	return EIO;
    val->tv_sec = now.tv_sec;
    val->tv_nsec = now.tv_nsec;
    return 0;
}
