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

#ifndef _MKPERF_H
#define _MKPERF_H

#include <conf.h>
#include <mach.h>
#include <mach_setjmp.h>

#include <mach/mach_types.h>
#include <mach/time_value.h>
#include <mach/thread_switch.h>
#include <mach/thread_status.h>
#include <mach/machine/vm_param.h>
#include <mach/vm_prot.h>
#include <mach/vm_region.h>
#include <mach/policy.h>

#if     TEST_DEVICE
#include <device/test_device_status.h>
#endif  /* TEST_DEVICE */
#include <device/device.h>
#include <device/device_request.h>
#include <device/device_reply.h>
#include <device/test_device_status.h>
#include <device/net_status.h>

#include <debug_services.h>
#include <mach_services.h>
#include <async_services.h>
#include <resource_services.h>
#include <stat_services.h>
#include <cli_services.h>
#include <synthetic_services.h>
#include <time_services.h>
#include <test_services.h>
#include <printf_services.h>
#include <server_services.h>
#include <prof_services.h>
#include <sched_services.h>
#include <rpc_services.h>

#endif /* _MKPERF_H */
