/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <cthreads.h>
#include <mach/clock_types.h>
#include <mach/clock.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/parent_server.h>

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/param.h>
#include <asm/ptrace.h>

extern mach_port_t	rt_clock;
extern boolean_t	server_halted;
extern mach_port_t	host_port;
extern kern_return_t	clock_sleep(clock_t		clock,
				    sleep_type_t	sleep_type,
				    tvalspec_t		sleep_time,
				    tvalspec_t		*wake_time);

struct condition osfmach3_idle_condition = CONDITION_INITIALIZER;

tvalspec_t	cur_time;
unsigned long	jiffies_catchup = 0;

#define JIFFIES_BUNCH	1
#define NSEC_JIFFIES	((NSEC_PER_SEC / HZ) * JIFFIES_BUNCH)

void *
jiffies_thread(
	void	*arg)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t	kr;
	tvalspec_t	sleep_time;
	struct timeval	new_xtime;

	cthread_set_name(cthread_self(), "jiffies thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);

    restart:
	kr = clock_get_time(rt_clock, &cur_time);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("jiffies_thread: clock_get_time"));
		panic("jiffies_thread: can't get clock time");
	}

	for (;;) {
		if (cur_time.tv_nsec >= NSEC_PER_SEC - NSEC_JIFFIES) {
			sleep_time.tv_nsec = cur_time.tv_nsec - 
				(NSEC_PER_SEC - NSEC_JIFFIES);
			sleep_time.tv_sec = cur_time.tv_sec + 1;
		} else {
			sleep_time.tv_nsec = cur_time.tv_nsec + NSEC_JIFFIES;
			sleep_time.tv_sec = cur_time.tv_sec;
		}
		kr = clock_sleep(rt_clock,
				 TIME_ABSOLUTE,
				 sleep_time,
				 &cur_time);
		if (server_halted && parent_server) {
			/*
			 * clock_sleep is not interruptible, so the parent
			 * server will have problems completing our exit().
			 */
			for (;;) {
				server_thread_yield(1000); /* yield for 1s */
			}
		}
		if (kr != KERN_SUCCESS) {
			long lost_ticks;

			uniproc_enter();
			if (kr != KERN_INVALID_VALUE) {
				MACH3_DEBUG(1, kr,
					    ("jiffies_thread: clock_sleep"));
			}
			/*
			 * Catch up with the micro-kernel clock...
			 */
			jiffies_catchup++;
			osfmach3_get_time(&new_xtime);
			lost_ticks = (new_xtime.tv_sec - xtime.tv_sec) * HZ;
			if (new_xtime.tv_usec > xtime.tv_usec) {
				lost_ticks += (((new_xtime.tv_usec
						 - xtime.tv_usec) * HZ)
					       / 1000000);
			} else {
				lost_ticks -= (((xtime.tv_usec
						 - new_xtime.tv_usec) * HZ)
					       / 1000000);
			}
			if (lost_ticks < 0) {
				printk("jiffies_thread: lost_ticks=%ld. "
				       "xtime=<%d,%d> new_xtime=<%d,%d>\n",
				       lost_ticks,
				       xtime.tv_sec, xtime.tv_usec,
				       new_xtime.tv_sec, new_xtime.tv_usec);
			} else {
				jiffies += lost_ticks;
			}
			xtime = new_xtime;
			uniproc_exit();
			goto restart;
		}
		uniproc_enter();
		do_timer(NULL);
		if (intr_count == 0 && (bh_active & bh_mask)) {
			/*
			 * There are some bottom half handlers to run.
			 * Wake up the idle thread to do it.
			 */
			condition_signal(&osfmach3_idle_condition);
		}
		uniproc_exit();
	}
	/*NOTREACHED*/
}

void
jiffies_thread_init(void)
{
	kern_return_t	kr;

#if 1
	/* rt_clock has been initialized by init_mapped_time() */
	assert(MACH_PORT_VALID(rt_clock));
#else
	kr = host_get_clock_service(host_port,
				    REALTIME_CLOCK,
				    &rt_clock);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("jiffies_thread_init: host_get_clock_service"));
		panic("can't get real time clock");
	}
#endif

	kr = clock_get_time(rt_clock, &cur_time);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("jiffies_thread_init: clock_get_time"));
		panic("can't get real time");
	}

	osfmach3_get_time(&xtime);

	(void) server_thread_start(jiffies_thread, (void *) 0);
}
