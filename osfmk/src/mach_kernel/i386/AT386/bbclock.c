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
/* CMU_HIST (from rtc.c) */
/*
 * Revision 2.6.1.2  92/03/28  10:06:48  jeffreyh
 * 	Changes from lance@mtxinu.com to correct more leap year problems.
 * 	[92/03/11            jeffreyh]
 * 
 * Revision 2.6.1.1  92/03/03  17:18:21  jeffreyh
 * 	Correct logic in writetodc() for number of days in leap year.
 * 
 * Revision 2.6  91/05/14  16:29:48  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/02/05  17:20:21  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:47:09  mrt]
 * 
 * Revision 2.4  90/11/26  14:51:00  rvb
 * 	jsb bet me to XMK34, sigh ...
 * 	[90/11/26            rvb]
 * 	Synched 2.5 & 3.0 at I386q (r1.5.1.3) & XMK35 (r2.4)
 * 	[90/11/15            rvb]
 * 
 * Revision 1.5.1.2  90/07/27  11:27:01  rvb
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[90/07/27            rvb]
 * 
 * Revision 2.2  90/05/03  15:46:00  dbg
 * 	Converted for pure kernel.
 * 	[90/02/20            dbg]
 * 
 * Revision 1.5.1.1  90/01/08  13:29:51  rvb
 * 	Add Intel copyright.
 * 	[90/01/08            rvb]
 * 
 * Revision 1.5  89/09/25  12:27:28  rvb
 * 	File was provided by Intel 9/18/89.
 * 	[89/09/23            rvb]
 * 
 */

/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <types.h>
#include <mach/message.h>
#include <kern/clock.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <i386/pio.h>
#include <i386/AT386/rtc.h>
#include <i386/AT386/bbclock_entries.h>

/* Forward */

extern int		bbc_config(void);
extern int		bbc_init(void);
extern kern_return_t	bbc_getattr(
				clock_flavor_t		flavor,
				clock_attr_t		attr,
				mach_msg_type_number_t	* count);
extern kern_return_t	bbc_setattr(
				clock_flavor_t		flavor,
				clock_attr_t		attr,
				mach_msg_type_number_t	count);

struct clock_ops  bbc_ops = {
	bbc_config,	bbc_init,	bbc_gettime,	bbc_settime,
	bbc_getattr,	bbc_setattr,	NO_MAPTIME,	NO_SETALRM,
};

/* local data */
static	int	month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

extern	char	dectohexdec(
			int			n);
extern int	hexdectodec(
			char			c);
extern int	yeartoday(
			int			yr);
extern void	rtcput(
			struct rtc_st		* regs);
extern int	rtcget(
			struct rtc_st		* regs);

#define	LOCK_BBC()	splclock()
#define	UNLOCK_BBC(s)	splx(s)

/*
 * Configure battery-backed clock.
 */
int
bbc_config(void)
{
	int		BbcFlag;
	struct rtc_st	rtclk;

#if	NCPUS > 1 && AT386
	mp_disable_preemption();
	if (cpu_number() != master_cpu) {
		mp_enable_preemption();
		return(1);
	}
#endif
	/*
	 * Setup device.
	 */
	outb(RTC_ADDR, RTC_A);
	outb(RTC_DATA, RTC_DIV2 | RTC_RATE6);
	outb(RTC_ADDR, RTC_B);
	outb(RTC_DATA, RTC_HM);

	/*
	 * Probe the device by trying to read it.
	 */
	BbcFlag = (rtcget(&rtclk) ? 0 : 1);
	if (BbcFlag)
		printf("battery clock configured\n");
	else
		printf("WARNING: Battery Clock Failure!\n");
#if	NCPUS > 1 && AT386
	mp_enable_preemption();
#endif
	return (BbcFlag);
}

/*
 * Initialize battery-backed clock.
 */
int
bbc_init(void)
{
	/* nothing to do here unless we wanted to check the
	   sanity of the time value of the clock at boot */
	return (1);
}

/*
 * Get the current clock time.
 */
kern_return_t
bbc_gettime(
	tvalspec_t	*cur_time)	/* OUT */
{
	struct rtc_st	rtclk;
	time_t		n;
	int		sec, min, hr, dom, mon, yr;
	int		i, days = 0;
	spl_t		s;
	thread_t	thread;

#if 	NCPUS > 1 && AT386
	if ((thread = current_thread()) != THREAD_NULL) {
		thread_bind(thread, master_processor);
		mp_disable_preemption();
		if (current_processor() != master_processor) {
			mp_enable_preemption();
			thread_block((void (*)) 0);
		} else {
			mp_enable_preemption();
		}
	}
#endif
	s = LOCK_BBC();
	rtcget(&rtclk);
	sec = hexdectodec(rtclk.rtc_sec);
	min = hexdectodec(rtclk.rtc_min);
	hr = hexdectodec(rtclk.rtc_hr);
	dom = hexdectodec(rtclk.rtc_dom);
	mon = hexdectodec(rtclk.rtc_mon);
	yr = hexdectodec(rtclk.rtc_yr);
	yr = (yr < 70) ? yr+100 : yr;
	n = sec + 60 * min + 3600 * hr;
	n += (dom - 1) * 3600 * 24;
	if (yeartoday(yr) == 366)
		month[1] = 29;
	for (i = mon - 2; i >= 0; i--)
		days += month[i];
	month[1] = 28;
	for (i = 70; i < yr; i++)
		days += yeartoday(i);
	n += days * 3600 * 24;
	cur_time->tv_sec  = n;
	cur_time->tv_nsec = 0;
	UNLOCK_BBC(s);

#if	NCPUS > 1 && AT386
	if (thread != THREAD_NULL)
		thread_bind(thread, PROCESSOR_NULL);
#endif
	return (KERN_SUCCESS);
}

/*
 * Set the current clock time.
 */
kern_return_t
bbc_settime(
	tvalspec_t	*new_time)
{
	struct rtc_st	rtclk;
	time_t		n;
	int		diff, i, j;
	spl_t		s;
	thread_t	thread;

#if	NCPUS > 1 && AT386
	if ((thread = current_thread()) != THREAD_NULL) {
		thread_bind(thread, master_processor);
		mp_disable_preemption();
		if (current_processor() != master_processor) {
			mp_enable_preemption();
			thread_block((void (*)) 0);
		} else { 
			mp_enable_preemption();
		}
	}
#endif
	s = LOCK_BBC();
	rtcget(&rtclk);
	diff = 0;
	n = (new_time->tv_sec - diff) % (3600 * 24);   /* hrs+mins+secs */
	rtclk.rtc_sec = dectohexdec(n%60);
	n /= 60;
	rtclk.rtc_min = dectohexdec(n%60);
	rtclk.rtc_hr = dectohexdec(n/60);
	n = (new_time->tv_sec - diff) / (3600 * 24);	/* days */
	rtclk.rtc_dow = (n + 4) % 7;  /* 1/1/70 is Thursday */
	for (j = 1970; n >= (i = yeartoday(j)); j++)
		n -= i;
	rtclk.rtc_yr = dectohexdec(j - 1900);
	if (yeartoday(j) == 366)
		month[1] = 29;
	for (i = 0; n >= month[i]; i++)
		n -= month[i];
	month[1] = 28;
	rtclk.rtc_mon = dectohexdec(++i);
	rtclk.rtc_dom = dectohexdec(++n);
	rtcput(&rtclk);
	UNLOCK_BBC(s);

#if	NCPUS > 1 && AT386
	if (thread != THREAD_NULL)
		thread_bind(current_thread(), PROCESSOR_NULL);
#endif
	return (KERN_SUCCESS);
}

/*
 * Get clock device attributes.
 */
kern_return_t
bbc_getattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* OUT */
	mach_msg_type_number_t	*count)		/* IN/OUT */
{
	if (*count != 1)
		return (KERN_FAILURE);
	switch (flavor) {

	case CLOCK_GET_TIME_RES:	/* >0 res */
		*(clock_res_t *) attr = NSEC_PER_SEC;
		break;

	case CLOCK_MAP_TIME_RES:	/* =0 no mapping */
	case CLOCK_ALARM_CURRES:	/* =0 no alarm */
	case CLOCK_ALARM_MINRES:
	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *) attr = 0;
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
bbc_setattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* IN */
	mach_msg_type_number_t	count)		/* IN */
{
	return (KERN_FAILURE);
}


/* DEVICE SPECIFIC ROUTINES */

int
rtcget(
	struct rtc_st	* regs)
{
	outb(RTC_ADDR, RTC_D); 
	if (inb(RTC_DATA) & RTC_VRT == 0)
		return (-1);
	outb(RTC_ADDR, RTC_A);	
	while (inb(RTC_DATA) & RTC_UIP)		/* busy wait */
		outb(RTC_ADDR, RTC_A);	
	load_rtc((unsigned char *)regs);
	return (0);
}	

void
rtcput(
	struct rtc_st	* regs)
{
	register unsigned char	x;

	outb(RTC_ADDR, RTC_B);
	x = inb(RTC_DATA);
	outb(RTC_ADDR, RTC_B);
	outb(RTC_DATA, x | RTC_SET); 	
	save_rtc((unsigned char *)regs);
	outb(RTC_ADDR, RTC_B);
	outb(RTC_DATA, x & ~RTC_SET); 
}

int
yeartoday(
	int	year)
{
	return((year % 4) ? 365 :
	       ((year % 100) ? 366 : ((year % 400) ? 365: 366)));
}

int
hexdectodec(
	char	n)
{
	return ((((n >> 4) & 0x0F) * 10) + (n & 0x0F));
}

char
dectohexdec(
	int	n)
{
	return ((char)(((n / 10) << 4) & 0xF0) | ((n % 10) & 0x0F));
}
