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
 *	File:		rtclock.c
 *	Purpose:
 *		Routines for handling the machine dependent
 *		real-time clock. On the Sequent, this clock
 *		is generated for each processor.
 */

#include <cpus.h>
#include <platforms.h>
#include <kern/cpu_number.h>
#ifdef	MACH_KERNEL
#include <kern/clock.h>
#else	/* MACH_KERNEL */
#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#endif	/* MACH_KERNEL */
#include <machine/mach_param.h>	/* HZ */
#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>		/* for kernel_map */
#include <device/dev_hdr.h>	/* for device_t */

#include <sqt/cfg.h>
#include <sqt/clock.h>
#include <sqt/slic.h>

#include <sqt/ioconf.h>
#include <sqt/vm_defs.h>
#include <sqt/hwparam.h>
#include <sqt/intctl.h>

#include <sqtsec/sec.h>

int		rtc_config(), rtc_init();
kern_return_t	rtc_gettime(), rtc_settime();
kern_return_t	rtc_getattr(), rtc_setattr();
kern_return_t	rtc_maptime();
void		rtc_setalrm();

/*
 * List of real-time clock dependent routines.
 */
struct clock_ops  rtc_ops = {
	rtc_config,	rtc_init,	rtc_gettime,	rtc_settime,
	rtc_getattr,	rtc_setattr,	rtc_maptime,	rtc_setalrm,
};

/* local data declarations */
tvalspec_t		*RtcTime;
tvalspec_t		*RtcAlrm;
clock_res_t		RtcDelt;
static	boolean_t	enable_local_clock = TRUE;

/* global data declarations */
struct	{
	mapped_tvalspec_t *mtime;	/* mappable clock time */
#define time		mtime->mtv_time
	tvalspec_t	alarm_time;	/* time of next alarm */
	clock_res_t	new_ires;	/* pending new resolution (nano ) */
	clock_res_t	intr_nsec;	/* interrupt resolution (nano) */
	int		intr_freq;	/* corresponding frequency */
	int		intr_hertz;	/* interrupts per hertz */
	int		intr_count;	/* interrupt counter */
decl_simple_lock_data(,	lock)		/* real-time clock device lock */
} rtclock;

#define RTC_MAXRES	(NSEC_PER_SEC / HZ)	/* max resolution nsec */
#if	MACH_KPROF
#define	RTC_MINRES	(RTC_MAXRES)		/* min resolution nsec */
#else	/* MACH_KPROF */
#define	RTC_MINRES	(RTC_MAXRES / 20)	/* min resolution nsec */
#endif	/* MACH_KPROF */

/*
 *	Macros to lock/unlock real-time clock device.
 */
#define LOCK_RTC()			\
	splclock();			\
	simple_lock(&rtclock.lock);

#define UNLOCK_RTC(s)			\
	simple_unlock(&rtclock.lock);	\
	splx(s);


/*
 * Configure the real-time clock device. Return success (1)
 * or failure (0).
 */
int
rtc_config()
{
	int	RtcFlag;

	/*
	 * We should attempt to test the real-time clock
	 * device here. If it were to fail, we should panic
	 * the system.
	 */
	RtcFlag = /* test device */1;
	printf("realtime clock configured\n");
	return (RtcFlag);
}

/*
 * Initialize the real-time clock device. Return success (1)
 * or failure (0). Since the real-time clock is required to
 * provide canonical mapped time, we allocate a page to keep
 * the clock time value. In addition, various variables used
 * to support the clock are initialized.
 */
int
rtc_init()
{
	vm_offset_t	*vp;

	/*
	 * Allocate mapped time page.
	 */
	vp = (vm_offset_t *) &rtclock.mtime;
        if (kmem_alloc_wired(kernel_map, vp, PAGE_SIZE) != KERN_SUCCESS)
                panic("cannot allocate rtclock time page\n");
        bzero((char *)rtclock.mtime, PAGE_SIZE);
	RtcTime = &rtclock.time;

	/*
	 * Initialize non-zero clock structure values.
	 */
	simple_lock_init(&rtclock.lock);
	rtclock.intr_nsec = RTC_MAXRES;
	rtclock.intr_freq = (NSEC_PER_SEC / RTC_MAXRES);
	rtclock.intr_hertz = 1;
	rtclock.intr_count = 1;
	RtcDelt = RTC_MAXRES/2;
	
	return (1);
}

/*
 * Get the clock device time. This routine is responsible
 * for converting the device's machine dependent time value
 * into a canonical tvalspec_t value.
 */
kern_return_t
rtc_gettime(cur_time)
	tvalspec_t	*cur_time;	/* OUT */
{
	MTS_TO_TS(rtclock.mtime, cur_time);
	return (KERN_SUCCESS);
}

/*
 * Set the clock device time. The alarm clock layer has
 * verified the new_time as a legal tvalspec_t value.
 */
kern_return_t
rtc_settime(new_time)
	tvalspec_t	*new_time;
{
	int		s;

	s = LOCK_RTC();
	rtclock.mtime->mtv_csec = new_time->tv_sec;
	rtclock.mtime->mtv_nsec = new_time->tv_nsec;
	rtclock.mtime->mtv_sec  = new_time->tv_sec;
	RtcAlrm = 0;
	UNLOCK_RTC(s);

	return (KERN_SUCCESS);
}

/*
 * Get clock device attributes.
 */
kern_return_t
rtc_getattr(flavor, attr, count)
	clock_flavor_t		flavor;
	clock_attr_t		attr;		/* OUT */
	mach_msg_type_number_t	*count;		/* IN/OUT */
{
	if (*count != 1)
		return (KERN_FAILURE);
	switch (flavor) {

	case CLOCK_GET_TIME_RES:	/* >0 res */
	case CLOCK_MAP_TIME_RES:	/* >0 canonical */
	case CLOCK_ALARM_CURRES:	/* =0 no alarm */
		*(clock_res_t *) attr = rtclock.intr_nsec;
		break;

	case CLOCK_ALARM_MINRES:
		*(clock_res_t *) attr = RTC_MINRES;
		break;

	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *) attr = RTC_MAXRES;
		break;

	default:
		return (KERN_INVALID_VALUE);
	}
	return (KERN_SUCCESS);
}

/*
 * Set clock device attributes.
 */
kern_return_t
rtc_setattr(flavor, attr, count)
	clock_flavor_t		flavor;
	clock_attr_t		attr;		/* IN */
	mach_msg_type_number_t	count;		/* IN */
{
	int		s;
	int		freq;
	int		adj;
	clock_res_t	new_ires;

	if (count != 1)
		return (KERN_FAILURE);
	switch (flavor) {

	case CLOCK_GET_TIME_RES:
	case CLOCK_MAP_TIME_RES:
	case CLOCK_ALARM_MINRES:
	case CLOCK_ALARM_MAXRES:
		return (KERN_FAILURE);

	case CLOCK_ALARM_CURRES:
		new_ires = *(clock_res_t *) attr;

		/*
		 * The new resolution should be valid and a multiple
		 * of the minimum resolution.
		 */
		if (new_ires < RTC_MINRES || new_ires > RTC_MAXRES)
			return (KERN_INVALID_VALUE);
		if (new_ires % RTC_MINRES)
			return (KERN_INVALID_VALUE);
/*		freq = (NSEC_PER_SEC / new_ires);
		adj = (((SL_TIMERFREQ % freq) * new_ires) / SL_TIMERFREQ);
		if (adj > (new_ires / 1000))
			return (KERN_INVALID_VALUE);
*/
		/*
		 * Record the new alarm resolution which will take effect
		 * on the next HZ aligned clock tick.
		 */
		s = LOCK_RTC();
		if (new_ires  != rtclock.intr_nsec)
			rtclock.new_ires = new_ires;
		UNLOCK_RTC(s);
		return (KERN_SUCCESS);

	default:
		return (KERN_INVALID_VALUE);
	}
}

/*
 * Map the clock device time. The real-time clock is required
 * to map a canonical time value. We cheat here, by using the
 * device subsystem to generate a pager for the clock - this
 * should probably be changed at some point.
 */
kern_return_t
rtc_maptime(pager)
	ipc_port_t	*pager;		/* OUT */
{
	device_t	device;
	vm_size_t	size;

	/*
	 * Open the "rtclock" device. The only interface available
	 * to this device is the d_mmap() routine.
	 */
	device = device_lookup("rtclock");
	if (device == DEVICE_NULL)
		return (KERN_FAILURE);

	/*
	 * Note: this code does not touch the device state. This should
	 * be ok even though the device can be opened from user mode via
	 * a device_open() request since both the device and the pager
	 * structures should be protected from deletion by reference
	 * counts and since the dev_pager code doesn't depend upon any
	 * internal device state setup via device_open() other than
	 * that established from device_lookup().
	 */
	size = sizeof(mapped_tvalspec_t);
	return (device_pager_setup(device, VM_PROT_READ, 0, size, pager));
}

/*
 * Set next alarm time for the clock device. This call
 * always resets the time to deliver an alarm for the
 * clock.
 */
void
rtc_setalrm(alarm_time)
	tvalspec_t	*alarm_time;
{
	int		s;

	s = LOCK_RTC();
	rtclock.alarm_time = *alarm_time;
	RtcAlrm = &rtclock.alarm_time;
	UNLOCK_RTC(s);
}


/*
 * Reset the clock device. This causes the realtime clock
 * device to reload its mode and count value (frequency).
 */
rtclock_reset()
{
	unsigned int	count;
	struct cpuslic	*sl = va_slic;

	/*
	 * Called on all processors at boot time. Called
	 * only on master to change alarm rate after boot.
	 */
	if (!enable_local_clock)
		return;

	count  = (sys_clock_rate * 1000000);
	count /= (SL_TIMERDIV * rtclock.intr_freq);

	sl->sl_trv = count - 1;
	sl->sl_tcont = 0;
	sl->sl_tctl = SL_TIMERINT | LCLKBIN;	/* timer on in given bin */
}

/*
 * Map the clock device time. This is part of the "cheat"
 * we use to map the time via the device subsystem. This
 * routine is invoked via a vm_map() call by the user.
 */
rtclock_map(dev, off, prot)
	vm_prot_t	prot;
{
	vm_offset_t	voff;

#ifdef	lint
	dev++; off++;
#endif	/* lint */

	if (prot & VM_PROT_WRITE)
		return (-1);
	voff = pmap_extract(pmap_kernel(), (vm_offset_t) rtclock.mtime);
	return (i386_btop(voff));
}

/*
 * Real-time clock device interrupt. Called only on the
 * master processor. Updates the clock time and upcalls
 * into the higher level clock code to deliver alarms.
 */
int
rtclock_intr()
{
	int		s;
	int		i;
	tvalspec_t	clock_time;

	/*
	 * Update clock time. Do the update so that the macro
	 * MTS_TO_TS() for reading the mapped time works (e.g.
	 * update in order: mtv_csec, mtv_nsec, mtv_sec).
	 */	 
	s = LOCK_RTC();
	i = rtclock.mtime->mtv_nsec + rtclock.intr_nsec;
	if (i < NSEC_PER_SEC)
		rtclock.mtime->mtv_nsec = i;
	else {
		rtclock.mtime->mtv_csec++;
		rtclock.mtime->mtv_nsec = 0;
		rtclock.mtime->mtv_sec++;
	}

	/*
	 * Perform alarm clock processing if needed. The time
	 * passed up is incremented by a half-interrupt tick
	 * to trigger alarms closest to their desired times.
	 * The clock_alarm_intr() routine calls rtc_setalrm()
	 * before returning if later alarms are pending.
	 */
	if (RtcAlrm && (RtcAlrm->tv_sec < RtcTime->tv_sec ||
	   	        (RtcAlrm->tv_sec == RtcTime->tv_sec &&
	    		 RtcDelt >= RtcAlrm->tv_nsec - RtcTime->tv_nsec))) {
		clock_time.tv_sec = 0;
		clock_time.tv_nsec = RtcDelt;
		ADD_TVALSPEC (&clock_time, RtcTime);
		RtcAlrm = 0;
		UNLOCK_RTC(s);
		/*
		 * Call clock_alarm_intr() without RTC-lock.
		 * The lock ordering is always CLOCK-lock
		 * before RTC-lock.
		 */
		clock_alarm_intr(REALTIME_CLOCK, &clock_time);
		s = LOCK_RTC();
	}

	/*
	 * On a HZ-tick boundary: return 0 and adjust the clock
	 * alarm resolution (if requested).  Otherwise return a
	 * non-zero value.
	 */
	if ((i = --rtclock.intr_count) == 0) {
		if (rtclock.new_ires) {
			/* adjust rtclock values */
			rtclock.intr_nsec = rtclock.new_ires;
			rtclock.intr_freq = (NSEC_PER_SEC / rtclock.intr_nsec);
			rtclock.intr_hertz = (rtclock.intr_freq / HZ);
			RtcDelt = rtclock.intr_nsec / 2;
			rtclock.new_ires = 0;
			/* reset clock */
			rtclock_reset();
		}
		rtclock.intr_count = rtclock.intr_hertz;
	}
	UNLOCK_RTC(s);
	return (i);
}
