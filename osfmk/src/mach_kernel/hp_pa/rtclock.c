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

#include <mach/machine/vm_types.h>
#include <machine/thread.h>
#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/psw.h>
#include <kern/spl.h>
#include <machine/regs.h>
#include <kern/clock.h>
#include <machine/clock.h>
#include <kern/misc_protos.h>
#include <machine/machparam.h>	
#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>		/* for kernel_map */
#include <device/dev_hdr.h>	/* for device_t */
#include <device/ds_routines.h>
#include <hp_pa/HP700/iotypes.h>
#include <machine/asp.h>
#include <hp_pa/rtclock_entries.h>

#include <hp_pa/trap.h>
#include <hp_pa/c_support.h>
#include <hp_pa/misc_protos.h>

/*
 * List of real-time clock dependent routines.
 */
struct clock_ops  rtc_ops = {
	rtc_config,	rtc_init,	rtc_gettime,	rtc_settime,
	rtc_getattr,	rtc_setattr,	rtc_maptime,	rtc_setalrm,
};

/* local data declarations */
tvalspec_t		*RtcTime = (tvalspec_t *)0;
tvalspec_t		*RtcAlrm;
clock_res_t		RtcDelt;

/* global data declarations */
struct	{
	mapped_tvalspec_t *mtime;	/* mappable clock time */
	tvalspec_t	alarm_time;	/* time of next alarm */
	clock_res_t	new_ires;	/* pending new resolution (nano ) */
	clock_res_t	timer_nsec;	/* timer resolution (nano) */
	clock_res_t	intr_nsec;	/* interrupt resolution (nano) */
	int		intr_freq;	/* corresponding frequency */
	int		intr_hertz;	/* interrupts per hertz */
	int		intr_count;	/* interrupt counter */
	int		timer_count;	/* Interval timer ticks between
					   2 interrupts */
	int 		*last_timer_val;/* mapped timer value for last 
					   interrupt */
	decl_simple_lock_data(,lock)	/* real-time clock device lock */
} rtclock;

int time_inval;		                /* timer value for next interrupt */

static unsigned int rtc_maxres;		/* max resolution nsec */
static unsigned int rtc_minres;		/* min resolution nsec */

/*
 *	Macros to lock/unlock real-time clock device.
 */
#define LOCK_RTC(s)			\
	s = splclock();			\
	simple_lock(&rtclock.lock);

#define UNLOCK_RTC(s)			\
	simple_unlock(&rtclock.lock);	\
	splx(s);


void         rtc_setvals(clock_res_t );

/*
 * Initialize non-zero clock structure values.
 */
void
rtc_setvals(clock_res_t  new_ires)
{
	rtclock.intr_freq = (NSEC_PER_SEC / new_ires);
	rtclock.intr_hertz = rtclock.intr_freq / hz;

	rtclock.intr_count = 1;
	RtcDelt = rtclock.intr_nsec/2;
}

int
rtc_config(void)
{
	/*
	 * Set the clock resolution.
	 *
	 * NOTE:  This code presumes 'hz' is already setup
	 */
	rtc_minres = (NSEC_PER_SEC / hz);
	rtc_maxres = rtc_minres / 20;

        printf("realtime clock configured\n");
        return 1;
}

int
rtc_init(void)
{
	vm_offset_t	*vp;

	/*
	 * Allocate mapped time page.
	 */
	vp = (vm_offset_t *) &rtclock.mtime;
        if (kmem_alloc_wired(kernel_map, vp, PAGE_SIZE) != KERN_SUCCESS)
                panic("cannot allocate rtclock time page\n");
        bzero((char *)rtclock.mtime, PAGE_SIZE);
	RtcTime = &rtclock.mtime->mtv_time;
	rtclock.last_timer_val = (int *)(rtclock.mtime+1);

	/*
	 * Initialize non-zero clock structure values.
	 */
	simple_lock_init(&rtclock.lock, ETAP_MISC_RT_CLOCK);
	rtclock.intr_nsec = rtc_minres;

	/*
	 * The PDC puts the value of itics / 10 msec into low memory at the
	 * location MEM_10MSEC.
	 */
	rtclock.timer_count = PAGE0->mem_10msec;
#if 0
	/* this division hangs us !! */
	rtclock.timer_nsec = rtclock.intr_nsec / rtclock.timer_count;
#endif
	rtc_setvals(rtc_minres);

	time_inval = mfctl(CR_ITMR);
	procitab(SPLCLOCK, hardclock, 0);	

	return 1;
}

static tvalspec_t	stime;
static tvalspec_t	itime = {0, 0};

/*
 * Get the clock device time. This routine is responsible
 * for converting the device's machine dependent time value
 * into a canonical tvalspec_t value.
 */
kern_return_t
rtc_gettime(
	tvalspec_t	*cur_time)	/* OUT */
{
	int		s;

	if(!RtcTime) {
		/* Uninitialized */
		cur_time->tv_nsec = 0;
		cur_time->tv_sec = 0;
		return (KERN_SUCCESS);
	}
	MTS_TO_TS(rtclock.mtime, cur_time);

	stime = *cur_time;

	/*
	 * Determine the incremental
	 * time since the last interrupt. 
	 */
	s = splclock();
	itime.tv_nsec = (mfctl(CR_ITMR) - *(rtclock.last_timer_val)) *  rtclock.timer_nsec;
	splx(s);

	/*
	 * If the time has changed, use the
	 * new time, otherwise utilize the incremental time.
	 */
	MTS_TO_TS(rtclock.mtime, cur_time);
	if (CMP_TVALSPEC(((tvalspec_t *)&stime), cur_time) == 0)
		ADD_TVALSPEC(cur_time, ((tvalspec_t *)&itime));
	return (KERN_SUCCESS);

}

/*
 * Get the clock device time when ALL interrupts are already disabled.
 * This routine is responsible for converting the device's machine dependent
 * time value into a canonical tvalspec_t value.
 */
void
rtc_gettime_interrupts_disabled(
	tvalspec_t	*cur_time)	/* OUT */
{
	MTS_TO_TS(rtclock.mtime, cur_time);

	stime = *cur_time;

	/*
	 * Determine the incremental
	 * time since the last interrupt. 
	 */
	itime.tv_nsec = (mfctl(CR_ITMR) - *(rtclock.last_timer_val)) * rtclock.timer_nsec;

	/*
	 * If the time has changed, use the
	 * new time, otherwise utilize the incremental time.
	 */
	MTS_TO_TS(rtclock.mtime, cur_time);
	if (CMP_TVALSPEC(((tvalspec_t *)&stime), cur_time) == 0)
		ADD_TVALSPEC(cur_time, ((tvalspec_t *)&itime));
}

/*
 * Set the clock device time. The alarm clock layer has
 * verified the new_time as a legal tvalspec_t value.
 */
kern_return_t
rtc_settime(
	tvalspec_t	*new_time)
{
	spl_t s;

	LOCK_RTC(s);
	rtclock.mtime->mtv_csec = new_time->tv_sec;
	rtclock.mtime->mtv_nsec = new_time->tv_nsec;
	rtclock.mtime->mtv_sec  = new_time->tv_sec;
	RtcAlrm = 0;
	UNLOCK_RTC(s);

	return KERN_SUCCESS;
}

/*
 * Get clock device attributes.
 */
kern_return_t
rtc_getattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* OUT */
	mach_msg_type_number_t	*count)		/* IN/OUT */
{
	if (*count != 1)
		return (KERN_FAILURE);
	switch (flavor) {

	case CLOCK_GET_TIME_RES:
	  	if (!rtclock.timer_nsec) {
			/* may not be initialized, see rtc_init */
			rtclock.timer_nsec = rtclock.intr_nsec / rtclock.timer_count;
		}
		*(clock_res_t *) attr = rtclock.timer_nsec;
		break;
	case CLOCK_MAP_TIME_RES:
	case CLOCK_ALARM_CURRES:
		*(clock_res_t *) attr = rtclock.intr_nsec;
		break;

	case CLOCK_ALARM_MINRES:
		*(clock_res_t *) attr = rtc_minres;
		break;

	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *) attr = rtc_maxres;
		break;
	
	case CLOCK_BASE_FREQ:
		/* We report the nominal frequency -- the actual
		   frequency may be less if the counter increments
		   by some value other than 1.  */
		*(clock_res_t *) attr = 100 * rtclock.timer_count;
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
rtc_setattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* IN */
	mach_msg_type_number_t	count)		/* IN */
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
		 * The new resolution must be within the predetermined
		 * range. 
		 */
		if (new_ires < rtc_maxres || new_ires > rtc_minres)
			return (KERN_INVALID_VALUE);

		freq = (NSEC_PER_SEC / new_ires);

		/*
		 * Record the new alarm resolution which will take effect
		 * on the next HZ aligned clock tick.
		 */
		LOCK_RTC(s);
		if ( freq != rtclock.intr_freq ) {
		    rtclock.new_ires = new_ires;
		}
		UNLOCK_RTC(s);
		return (KERN_SUCCESS);

	case CLOCK_BASE_FREQ:
		rtclock.timer_nsec = *((clock_res_t *) attr) / NSEC_PER_SEC;
		LOCK_RTC(s);
		/* setting new_ires will cause the new settings to
		 * take effect at the next clock tick */
		rtclock.new_ires = NSEC_PER_SEC / rtclock.intr_freq;
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
rtc_maptime(
	ipc_port_t	*pager)		/* OUT */
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
rtc_setalrm( tvalspec_t	*alarm_time)
{
	spl_t s;

	LOCK_RTC(s);
	rtclock.alarm_time = *alarm_time;
	RtcAlrm = &rtclock.alarm_time;
	UNLOCK_RTC(s);
}

void
rtclock_reset(void)
{
	register int time_remaining;
    
	/*
	 * see if we missed any clock tics
	 */

	if (!rtclock.timer_nsec) {
		/* may not be initialized, see rtc_init */
	  	rtclock.timer_nsec = rtclock.intr_nsec / rtclock.timer_count;
	}

	do {
		mtctl(CR_ITMR, (int)(time_inval += rtclock.timer_count));
		mtctl(CR_EIRR, SPLCLOCK);
		time_remaining = time_inval - mfctl(CR_ITMR);
	} while (time_remaining < 0);

}

vm_offset_t
rtclock_map(
	dev_t		dev,
	vm_offset_t	off,
	int		prot)
{
	vm_offset_t	voff;

	if (prot & VM_PROT_WRITE)
		return (-1);

	return btop(pmap_extract(pmap_kernel(), (vm_offset_t) rtclock.mtime));
}

#if	DEBUG
int    	tick_too_soon = 0;
#endif

/*
 * Real-time clock device interrupt. Called only on the
 * master processor. Updates the clock time and upcalls
 * into the higher level clock code to deliver alarms.
 */
int
rtclock_intr(void)
{
	int		s;
	int		i;
	tvalspec_t	clock_time;

#ifdef DEBUG
	unsigned int 	now;
	int 		time_since_tick_due;

	now = mfctl(CR_ITMR);
	time_since_tick_due = now - time_inval;

	/*
	 * Ticks should only happen at or after the time they're
	 * scheduled for.  If one arrives too early, we complain, but 
	 * just ignore it.  If this message shows up, it's time to
	 * analyze the system with a 64000 to see where the interrupt
	 * really originated.
	 */
	if (time_since_tick_due < 0) {
		tick_too_soon++;
		/*
		 * Fudge this and keep going otherwise we may hang
		 * around for a complete wrap around of the interval
		 * timer before the machine comes back. Rdb causes
		 * this.
		 */
		time_inval = now;
	}
#endif

	/*
	 * Update clock time. Do the update so that the macro
	 * MTS_TO_TS() for reading the mapped time works (e.g.
	 * update in order: mtv_csec, mtv_nsec, mtv_sec).
	 */	 
	LOCK_RTC(s);

	*(rtclock.last_timer_val) = time_inval;
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
		LOCK_RTC(s);
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
			rtc_setvals(rtclock.new_ires);
			rtclock.new_ires = 0;
			rtclock.timer_count = rtclock.intr_nsec / rtclock.timer_nsec;
		}
		rtclock.intr_count = rtclock.intr_hertz;
	}

	rtclock_reset();
	UNLOCK_RTC(s);
	return i;
}
