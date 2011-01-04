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
 * Microkernel interface to common profiling.
 */
/*
 * MkLinux
 */

#include <profile/profile-internal.h>
#include <mach/std_types.h>
#include <types.h>
#include <chips/busses.h>
#include <device/device_types.h>
#include <device/device_typedefs.h>

extern void kmstartup(void);
extern int gprofprobe(caddr_t, void *);
extern void gprofattach(struct bus_device *);
extern io_return_t gprofopen(dev_t, int, io_req_t);
extern void gprofclose(dev_t);
extern void gprofstrategy(io_req_t);
extern io_return_t gprofread(dev_t, io_req_t);
extern io_return_t gprofwrite(dev_t, io_req_t);

/*
 * Macros to access the nth cpu's profile variable structures.
 */

#if NCPUS <= 1
#define PROFILE_VARS(cpu) (&_profile_vars)

#else
extern struct profile_vars *_profile_vars_cpus[NCPUS];
#define PROFILE_VARS(cpu) (_profile_vars_cpus[(cpu)])
#endif


