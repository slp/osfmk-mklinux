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
/* CMU_HIST */
/*
 * Revision 2.4  91/05/14  16:50:00  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:31:10  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:21:08  mrt]
 * 
 * Revision 2.2  90/08/07  18:00:13  rpd
 * 	Picked up new timer representation.
 * 
 * Revision 2.1  89/08/03  15:57:34  rwd
 * Created.
 * 
 * 27-Oct-88  David Black (dlb) at Carnegie-Mellon University
 *	Added TIMER_LOW_FULL mask to check for possible overflow of
 *	low_bits field.  Removed TIMER_SERVICE flag.
 *
 * Revision 2.3  88/08/24  02:49:17  mwyoung
 * 	Adjusted include file references.
 * 	[88/08/17  02:25:43  mwyoung]
 * 
 *
 *  7-Apr-88  David Black (dlb) at Carnegie-Mellon University
 *	Added interface definitions and null routines for STAT_TIME.
 *
 * 19-Feb-88  David Black (dlb) at Carnegie-Mellon University
 *	Renamed fields, added timer_save structure.
 *
 * 31-May-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	machine/machtimer.h -> machine/timer.h
 *
 * 23-Feb-87  David L. Black (dlb) at Carnegie-Mellon University
 *	Created.
 */ 
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */

#ifndef	_KERN_TIMER_H_
#define _KERN_TIMER_H_

#include <cpus.h>
#include <stat_time.h>

#include <kern/macro_help.h>
#include <kern/thread.h>

#if	STAT_TIME
/*
 *	Statistical timer definitions - use microseconds in timer, seconds
 *	in high unit field.  No adjustment needed to convert to time_value_t
 *	as a result.  Service timers once an hour.
 */

#define TIMER_RATE	1000000
#define TIMER_HIGH_UNIT	TIMER_RATE
#undef	TIMER_ADJUST

#else	/* STAT_TIME */
/*
 *	Machine dependent definitions based on hardware support.
 */

#include <machine/timer.h>

#endif	/* STAT_TIME */

/*
 *	Definitions for accurate timers.  high_bits_check is a copy of
 *	high_bits that allows reader to verify that values read are ok.
 */

struct timer {
	unsigned	low_bits;
	unsigned	high_bits;
	unsigned	high_bits_check;
	unsigned	tstamp;
};

typedef struct timer		timer_data_t;
typedef	struct timer		*timer_t;

/*
 *	Mask to check if low_bits is in danger of overflowing
 */

#define	TIMER_LOW_FULL	0x80000000

/*
 *	Kernel timers and current timer array.  [Exported]
 */

extern timer_t		current_timer[NCPUS];
extern timer_data_t	kernel_timer[NCPUS];

/*
 *	save structure for timer readings.  This is used to save timer
 *	readings for elapsed time computations.
 */

struct timer_save {
	unsigned	low;
	unsigned	high;
};

typedef struct timer_save	timer_save_data_t, *timer_save_t;

/*
 *	Exported kernel interface to timers
 */

#if	STAT_TIME
#define start_timer(timer)
#define timer_switch(timer)
#else	/* STAT_TIME */
/* Start timer for this cpu */
extern void		start_timer(
				timer_t		timer);

/* Switch to a new timer */
extern void		timer_switch(
				timer_t		new_timer);
#endif	/* STAT_TIME */

/* Initialize timer module */
extern void		init_timers(void);

/*
 * Initializes a single timer.
 */
extern void		timer_init(
				timer_t	this_timer);

/* Normalize timer value */
extern void		timer_normalize(
				timer_t	timer);

/* Read value of timer into tv */
extern void		timer_read(
				timer_t timer,
				time_value_t *tv);

/* Read thread times */
extern void		thread_read_times(
				thread_t 	thread,
				time_value_t	*user_time_p,
				time_value_t	*system_time_p);

/* Compute timer difference */
extern unsigned		timer_delta(
				timer_t		timer,
				timer_save_t	save);

#if	STAT_TIME
/*
 *	Macro to bump timer values.
 */	
#define timer_bump(timer, usec)					\
MACRO_BEGIN							\
	(timer)->low_bits += usec;				\
	if ((timer)->low_bits & TIMER_LOW_FULL) {		\
		timer_normalize(timer);				\
	}							\
MACRO_END

#else	/* STAT_TIME */
/*
 *	Exported hardware interface to timers
 */
/* Time trap entry */
extern void		time_trap_uentry(
				unsigned	ts);

/* Time trap exit */
extern void		time_trap_uexit(
				unsigned	ts);

/* Time interrupt entry */
extern timer_t		time_int_entry(
				unsigned	ts,
				timer_t		new_timer);

/* Time interrrupt exit */
extern void		time_int_exit(
				unsigned	ts,
				timer_t		old_timer);

#endif	/* STAT_TIME */

/*
 *	TIMER_DELTA finds the difference between a timer and a saved value,
 *	and updates the saved value.  Look at high_bits check field after
 *	reading low because that's the first written by a normalize
 *	operation; this isn't necessary for current usage because
 *	this macro is only used when the timer can't be normalized:
 *	thread is not running, or running thread calls it on itself at
 *	splsched().
 */

#define TIMER_DELTA(timer, save, result)			\
MACRO_BEGIN							\
	register unsigned	temp;				\
								\
	temp = (timer).low_bits;				\
	if ((save).high != (timer).high_bits_check) {		\
		result += timer_delta(&(timer), &(save));	\
	}							\
	else {							\
		result += temp - (save).low;			\
		(save).low = temp;				\
	}							\
MACRO_END

#endif	/* _KERN_TIMER_H_ */
