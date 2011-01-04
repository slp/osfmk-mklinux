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
#include <sa_mach.h>
#include <mach/clock.h>
#include <mach/mach_traps.h>

static mach_port_t clock_port;
static int initialized=0;

void
sleep(int seconds)
{
	usleep(USEC_PER_SEC*seconds);
}

void
usleep(int useconds)
{
	tvalspec_t inval, outval;
	int seconds;

	/*
	 * Make sure we initialized ourselves.
	 */
	if (!initialized) {
	    host_get_clock_service(mach_host_self(),
				   REALTIME_CLOCK, &clock_port);
	    initialized=1;
	}

	/*
	 * Sleep for the specified number of microseconds.
	 */
	seconds = useconds/USEC_PER_SEC;
	useconds -= seconds*USEC_PER_SEC;
	inval.tv_sec = seconds;
	inval.tv_nsec = useconds*NSEC_PER_USEC;
	clock_sleep(clock_port, TIME_RELATIVE, inval, &outval);
}
