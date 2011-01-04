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

/*
 *	File:		clock_sleep.c
 *	Purpose:
 *		Routine to implement clock_sleep as a system
 *		trap call.
 */

#include <mach.h>
#include <mach/message.h>
#include <mach/mach_syscalls.h>

/*
 * Sleep on a clock. System trap.
 */
kern_return_t
clock_sleep(mach_port_t	clock,
	    int		sleep_type,
	    tvalspec_t	sleep_time,
	    tvalspec_t	*wakeup_time)
{
	kern_return_t	result;

	result = clock_sleep_trap(clock, sleep_type, sleep_time.tv_sec,
				  sleep_time.tv_nsec, wakeup_time);
	return(result);
}
