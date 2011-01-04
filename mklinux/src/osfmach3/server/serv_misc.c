/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/machine.h>
#include <mach/processor_info.h>

#include <osfmach3/mach3_debug.h>

#include <linux/sched.h>
#include <linux/kernel_stat.h>

extern mach_port_t	privileged_host_port;
extern mach_port_t	host_port;

void *__get_instruction_pointer(void) {
	return __builtin_return_address(0);
}

void
osfmach3_update_load_info(void)
{
	kern_return_t		kr;
	mach_msg_type_number_t	count;
	struct host_load_info	hli;
	int			i;

	count = HOST_LOAD_INFO_COUNT;
	kr = host_statistics(privileged_host_port,
			     HOST_LOAD_INFO,
			     (host_info_t) &hli,
			     &count);
	if (kr != KERN_SUCCESS || count != HOST_LOAD_INFO_COUNT) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_update_load_info: host_statistics"));
		return;
	}

	/*
	 * XXX the avenrun frequencies are not the same in Mach and Linux.
	 */
	for (i = 0; i < 3; i++) {
		avenrun[i] = hli.avenrun[i];
		/*
		 * Compensate the extra load of the "jiffies" thread,
		 * which is awaken every time the micro-kernel computes
		 * the load: remove 1*LOAD_SCALE to the load numbers.
		 */
		if (avenrun[i] < LOAD_SCALE)
			avenrun[i] = 0;
		else
			avenrun[i] -= LOAD_SCALE;
	}
}

void
osfmach3_update_cpu_load_info(void)
{
#ifdef	HOST_CPU_LOAD_INFO
	kern_return_t			kr;
	mach_msg_type_number_t		count;
	struct host_cpu_load_info	hcli;

	count = HOST_CPU_LOAD_INFO_COUNT;
	kr = host_statistics(privileged_host_port,
			     HOST_CPU_LOAD_INFO,
			     (host_info_t) &hcli,
			     &count);
	if (kr != KERN_SUCCESS || count != HOST_CPU_LOAD_INFO_COUNT) {
		MACH3_DEBUG(2, kr,
			    ("osfmach3_update_cpu_load_info: host_statistics"));
		return;
	}

	kstat.cpu_user = hcli.cpu_ticks[CPU_STATE_USER];
#if 0
	kstat.cpu_nice = hcli.cpu_ticks[CPU_STATE_NICE];
#else
	/* Mach doesn't have the same notion of priorities as Linux... */
	kstat.cpu_user += hcli.cpu_ticks[CPU_STATE_NICE];
#endif
	kstat.cpu_system = hcli.cpu_ticks[CPU_STATE_SYSTEM];
#endif	/* HOST_CPU_LOAD_INFO */
}
