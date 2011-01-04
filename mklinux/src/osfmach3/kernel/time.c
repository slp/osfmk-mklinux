/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * Machine-independent routines from arch/.../kernel/time.c.
 */

#include <mach/mach_interface.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/mach_init.h>

#include <linux/sched.h>

void do_gettimeofday(struct timeval *tv)
{
	osfmach3_get_time(&xtime);
	*tv = xtime;
}

void do_settimeofday(struct timeval *tv)
{
	xtime = *tv;
	osfmach3_set_time(&xtime);
}

void time_init(void)
{
	osfmach3_get_time(&xtime);
}
