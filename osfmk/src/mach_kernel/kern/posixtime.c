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
/* CMU_HIST (from mach_clock.c) */
/*
 * Revision 2.15  91/07/31  17:45:57  dbg
 * 	Implemented host_adjust_time.
 * 	[91/07/30  17:03:54  dbg]
 * 
 * Revision 2.14  91/05/18  14:32:29  rpd
 * 	Fixed host_set_time to update the mapped time value.
 * 	Changed the mapped time value to include a check field.
 * 	[91/03/19            rpd]
 *
 * Revision 2.8  90/10/12  18:07:29  rpd
 * 	Fixed calls to thread_bind in host_set_time.
 * 	Fix from Philippe Bernadat.
 * 	[90/10/10            rpd]
 * 
 * Revision 2.4  90/01/11  11:43:31  dbg
 * 	Switch to master CPU in host_set_time.
 * 	[90/01/03            dbg]
 * 
 * Revision 2.2  89/08/05  16:07:11  rwd
 * 	Added mappable time code.
 * 	[89/08/02            rwd]
 */ 

/*
 ***************  MONUMENT ******************************
 ********************************************************

    host_get_time, etc. are considered deprecated and are not
    documented in the kernel interfaces manual.  At this time
    (5/16/96) they still work, however the time is not as accurate as
    it could be.  The new strategy is to have the server keep the base
    time and the kernel keep the elapsed time since system start.
    Thus the current time-of-day is the sum of the two.

    The REALTIME_CLOCK does in fact accurately keep the time since
    system startup, and that time is NOT affected by adjustments or by
    setting the time of day.  This is a desirable feature for interval
    timing -- the time never goes backward or advances in any way
    other than at the proper rate.  Adjusting or resetting time is
    done by tweaking the base time.  The REALTIME_CLOCK does provide a
    fine tuning knob which allows the accounting of time to be closely
    matched to the crystal or other physical time source on the
    machine.  See clock attribute CLOCK_BASE_FREQ.

    Time-of-day herein is inaccurate because it assumes that clock
    ticks are exactly 10 ms.  Taint so.  Depending on hardware, the
    interrupts may be more or less than 10 ms.  Using the PC (386)
    timer, for example, the interrupts may be plus or minus about 500
    nanoseconds.  On top of that, the PC clock generator isn't tightly
    specified.  Emperically, it's frequency may vary from standard by
    200 ppm.  The REALTIME_CLOCK elapsed time takes account of all
    that, so time-of-day computed from time base plus elapsed time is
    accurate.  Time computed by adding 10 ms on every interrupt is
    not accurate.

    -dlm

 **********************************************************
 */

/* exclude highres time regardless of RT */
#define PTIME_MACH_RT      0

#include <cpus.h>
#include <stat_time.h>
#include <mach_prof.h>
#include <mach_assert.h>

#include <mach/boolean.h>
#include <mach/clock_types.h>
#include <mach/machine.h>
#include <mach/time_value.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <kern/counters.h>
#include <kern/cpu_number.h>
#include <kern/host.h>
#include <kern/lock.h>
#include <kern/mach_param.h>
#include <kern/misc_protos.h>
#include <kern/posixtime.h>
#include <kern/processor.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/spl.h>
#include <kern/thread.h>
#include <kern/time_out.h>
#include <vm/vm_kern.h>
#include <machine/mach_param.h>	/* HZ */
#if	PTIME_MACH_RT
#include <machine/rtclock_entries.h>
#endif	/* PTIME_MACH_RT */

#include <mach/mach_host_server.h>
 
/* Externs */
extern kern_return_t	bbc_gettime(
				time_value_t *time);

extern kern_return_t	bbc_settime(
				time_value_t *time);

#if	HZ > 500
int		tickadj = 1;		/* can adjust HZ usecs per second */
#else
int		tickadj = 500 / HZ;	/* can adjust 500 usecs per second */
#endif
int		bigadj = 1000000;	/* adjust 10*tickadj if adj>bigadj */

/* module declared variables */
volatile time_value_t          time = { 0, 0 };
integer_t                      timedelta = 0;
integer_t                      tickdelta = 0;
volatile mapped_time_value_t   posixtime = { 0, 0 };
volatile mapped_time_value_t * mtime = &posixtime;
volatile tvalspec_t            last_utime_tick = { 0, 0 };

#define update_mapped_time(time)				\
MACRO_BEGIN							\
	if (mtime != 0) {					\
		mtime->check_seconds = (time)->seconds;		\
		mtime->microseconds = (time)->microseconds;	\
		mtime->seconds = (time)->seconds;		\
	}							\
MACRO_END

/*
 * Universal (Posix) time initialization.
 */
void
utime_init(void)
{
	vm_offset_t	*vp;

	vp = (vm_offset_t *) &mtime;
	if (kmem_alloc_wired(kernel_map, vp, PAGE_SIZE)	!= KERN_SUCCESS)
		panic("mapable_time_init");
	bzero((char *)mtime, PAGE_SIZE);
	(void)bbc_gettime((time_value_t *)&time);
}

/*
 * Universal (Posix) time tick. This is called from the clock
 * interrupt path at splclock() interrupt level.
 */
void
utime_tick(void)
{
	register int	delta;

	delta = tick;
	if (timedelta < 0) {
		delta -= tickdelta;
		timedelta += tickdelta;
	} else
	if (timedelta > 0) {
		delta += tickdelta;
		timedelta -= tickdelta;
	}
	time_value_add_usec(&time, delta);
	update_mapped_time(&time);
#if PTIME_MACH_RT
	(void) rtc_gettime((tvalspec_t *)&last_utime_tick);
#endif	/* PTIME_MACH_RT */
}	

/*
 * Read the Universal (Posix) time.
 */
kern_return_t
host_get_time(
	host_t		host,
	time_value_t	*current_time)	/* OUT */
{
#if	PTIME_MACH_RT
	tvalspec_t	now_tick;
	tvalspec_t	last_tick;
#endif	/* PTIME_MACH_RT */

	if (host == HOST_NULL)
		return(KERN_INVALID_HOST);

#if	PTIME_MACH_RT
	/*
	 * If we are interrupted during the following calculation,
	 * we have to do it over.  If last_utime_tick changes, then
	 * we have been interrupted.
	 */
	do {
	    last_tick.tv_nsec = last_utime_tick.tv_nsec;
	    last_tick.tv_sec  = last_utime_tick.tv_sec;
	    do {
		current_time->seconds = mtime->seconds;
		current_time->microseconds = mtime->microseconds;
	    } while (current_time->seconds != mtime->check_seconds);
	    
	    (void) rtc_gettime(&now_tick);
	    SUB_TVALSPEC(&now_tick, &last_tick);
	    time_value_add_usec(current_time, now_tick.tv_nsec/NSEC_PER_USEC);
	} while ( last_tick.tv_nsec != last_utime_tick.tv_nsec );
#else	/* PTIME_MACH_RT */
	do {
	    current_time->seconds = mtime->seconds;
	    current_time->microseconds = mtime->microseconds;
	} while (current_time->seconds != mtime->check_seconds);
#endif	/* PTIME_MACH_RT */

	return (KERN_SUCCESS);
}

/*
 * Set the Universal (Posix) time. Privileged call.
 */
kern_return_t
host_set_time(
	host_t		host,
	time_value_t	new_time)
{
	spl_t	s;

	if (host == HOST_NULL)
		return(KERN_INVALID_HOST);

#if	NCPUS > 1
	thread_bind(current_thread(), master_processor);
	mp_disable_preemption();
	if (current_processor() != master_processor) {
		mp_enable_preemption();
		thread_block((void (*)(void)) 0);
	} else {
		mp_enable_preemption();
	}
#endif	/* NCPUS > 1 */

	s = splhigh();
	time = new_time;
	update_mapped_time(&time);
#if	PTIME_MACH_RT
	rtc_gettime_interrupts_disabled((tvalspec_t *)&last_utime_tick);
#endif	/* PTIME_MACH_RT */
#if 0
	(void)bbc_settime((time_value_t *)&time);
#endif
	splx(s);

#if	NCPUS > 1
	thread_bind(current_thread(), PROCESSOR_NULL);
#endif	/* NCPUS > 1 */

	return (KERN_SUCCESS);
}

/*
 * Adjust the Universal (Posix) time gradually.
 */
kern_return_t
host_adjust_time(
	host_t		host,
	time_value_t	newadj,
	time_value_t	*oldadj)	/* OUT */
{
	time_value_t	oadj;
	integer_t	ndelta;
	spl_t		s;

	if (host == HOST_NULL)
		return (KERN_INVALID_HOST);

	ndelta = (newadj.seconds * 1000000) + newadj.microseconds;

#if	NCPUS > 1
	thread_bind(current_thread(), master_processor);
	mp_disable_preemption();
	if (current_processor() != master_processor) {
		mp_enable_preemption();
		thread_block((void (*)(void)) 0);
	} else {
		mp_enable_preemption();
	}
#endif	/* NCPUS > 1 */

	s = splclock();
	oadj.seconds = timedelta / 1000000;
	oadj.microseconds = timedelta % 1000000;
	if (timedelta == 0) {
		if (ndelta > bigadj)
			tickdelta = 10 * tickadj;
		else
			tickdelta = tickadj;
	}
	if (ndelta % tickdelta)
		ndelta = ndelta / tickdelta * tickdelta;
	timedelta = ndelta;
	splx(s);

#if	NCPUS > 1
	thread_bind(current_thread(), PROCESSOR_NULL);
#endif	/* NCPUS > 1 */

	*oldadj = oadj;

	return (KERN_SUCCESS);
}

