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
 * Revision 2.5  91/05/14  16:46:04  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:28:50  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:16:37  mrt]
 * 
 * Revision 2.3  90/08/07  17:58:47  rpd
 * 	Picked up fix to MACH_FIXPRI version of csw_needed.
 * 	[90/08/07            rpd]
 * 
 * Revision 2.2  90/06/02  14:55:44  rpd
 * 	Updated to new scheduling technology.
 * 	[90/03/26  22:15:22  rpd]
 * 
 * Revision 2.1  89/08/03  15:52:50  rwd
 * Created.
 * 
 * 20-Oct-88  David Golub (dbg) at Carnegie-Mellon University
 *	Use macro_help to avoid lint.
 *
 * 11-Aug-88  David Black (dlb) at Carnegie-Mellon University
 *	Make csw_needed a macro here.  Ignore first_quantum for local_runq.
 *
 *  9-Aug-88  David Black (dlb) at Carnegie-Mellon University
 *	No more runrun.
 *
 * 18-May-88  David Black (dlb) at Carnegie-Mellon University
 *	Added shutdown queue for shutdown thread.
 *
 * 29-Mar-88  David Black (dlb) at Carnegie-Mellon University
 *	SIMPLE_CLOCK: added sched_usec for drift compensation.
 *
 * 25-Mar-88  David Black (dlb) at Carnegie-Mellon University
 *	Added sched_load and related constants.  Moved thread_timer_delta
 *	here because it depends on sched_load.
 *
 * 19-Feb-88  David Black (dlb) at Carnegie-Mellon University
 *	Added sched_tick and shift definitions for more flexible ageing.
 *
 * 18-Nov-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Removed conditionals, purged history.
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
/*
 *	File:	sched.h
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1985
 *
 *	Header file for scheduler.
 *
 */

#ifndef	_KERN_SCHED_H_
#define _KERN_SCHED_H_

#include <cpus.h>
#include <mach_rt.h>
#include <simple_clock.h>
#include <stat_time.h>

#include <mach/policy.h>
#include <kern/kern_types.h>
#include <kern/queue.h>
#include <kern/lock.h>
#include <kern/macro_help.h>

#if	STAT_TIME

/*
 *	Statistical timing uses microseconds as timer units.  18 bit shift
 *	yields priorities.  PRI_SHIFT_2 isn't needed.
 */
#define PRI_SHIFT	18

#else	/* STAT_TIME */

/*
 *	Otherwise machine provides shift(s) based on time units it uses.
 */
#include <machine/sched_param.h>

#endif	/* STAT_TIME */

#if	MACH_RT
#define	NRQS	128			/* 128 run queues per cpu */
#else	/* MACH_RT */
#define NRQS	32			/* 32 run queues per cpu */
#endif	/* MACH_RT */

#define NRQBM	(NRQS / 32)		/* number of run queue bit maps */
#define LPRI (NRQS - 3)			/* lowest pri reachable w/o depress*/
#define DEPRESSPRI (NRQS - 2)		/* depress priority */
#define MINPRI	DEPRESSPRI		/* lowest legal priority schedulable */
#define	IDLEPRI	(NRQS - 1)		/* idle thread priority */

/*
 *	Default base priorities for threads.
 */
#define PRI_USER_CHUNK  	(16+(NRQS/8))
#define PRI_KERNEL_CHUNK 	(NRQS-PRI_USER_CHUNK)/2
#define	BASEPRI_KERNEL		0
#define BASEPRI_SYSTEM		PRI_KERNEL_CHUNK
#define BASEPRI_USER		(NRQS-PRI_USER_CHUNK)
#define BASEPRI_SERVER		BASEPRI_SYSTEM+(BASEPRI_USER-BASEPRI_SYSTEM)/2

/*
 *	Macro to check for invalid priorities.
 */
#define invalid_pri(pri) (((pri) < 0) || ((pri) > MINPRI))


struct run_queue {
	queue_head_t		runq[NRQS];	/* one for each priority */
	decl_simple_lock_data(,lock)		/* one lock for all queues */
	int			bitmap[NRQBM];	/* run queue bitmap array */
	int			low;		/* low queue value */
	int			count;		/* count of threads runable */
	int			depress_count;	/* count of depressed threads */
};

typedef struct run_queue	*run_queue_t;
#define RUN_QUEUE_NULL	((run_queue_t) 0)

#if NCPUS > 1

#define csw_needed(thread, processor) (					  \
	(thread)->state & TH_SUSP ||					  \
	((processor)->first_quantum? 					  \
	 ((processor)->runq.low < (thread)->sched_pri ||		  \
	  (processor)->processor_set->runq.low < (thread)->sched_pri) :	  \
	 ((processor)->runq.low <= (thread)->sched_pri ||		  \
	  (processor)->processor_set->runq.low <= (thread)->sched_pri)))

#else /*NCPUS > 1*/

#define csw_needed(thread, processor) (					  \
	(thread)->state & TH_SUSP ||					  \
	((processor)->first_quantum?				  	  \
	  ((processor)->processor_set->runq.low < (thread)->sched_pri) :  \
	 ((processor)->processor_set->runq.low <= (thread)->sched_pri)))

#endif /*NCPUS > 1*/

/*
 *	Scheduler routines.
 */

/* Remove thread from its run queue */
extern run_queue_t	rem_runq(
				thread_t	th);

/* Mach factor computation (in mach_factor.c) */
extern void		compute_mach_factor(void);

/* Update threads quantum (in priority.c) */
extern void		thread_quantum_update(
				int		mycpu,
				thread_t	thread,
				int		nticks,
				int		state);

extern queue_head_t	action_queue;	/* assign/shutdown queue */

decl_simple_lock_data(,action_lock)

extern int		min_quantum;	/* defines max context switch rate */

/*
 *	Macro to check for invalid priorities.
 */

#define invalid_pri(pri) (((pri) < 0) || ((pri) > MINPRI))

/*
 *	Shift structures for holding update shifts.  Actual computation
 *	is  usage = (usage >> shift1) +/- (usage >> abs(shift2))  where the
 *	+/- is determined by the sign of shift 2.
 */
struct shift {
	int	shift1;
	int	shift2;
};

typedef	struct shift	*shift_t, shift_data_t;

/*
 *	sched_tick increments once a second.  Used to age priorities.
 */

extern unsigned	sched_tick;

#define SCHED_SCALE	128
#define SCHED_SHIFT	7

/*
 *	thread_timer_delta macro takes care of both thread timers.
 */

#define thread_timer_delta(thread)  				\
MACRO_BEGIN							\
	register unsigned	delta;				\
								\
	delta = 0;						\
	TIMER_DELTA((thread)->system_timer,			\
		(thread)->system_timer_save, delta);		\
	TIMER_DELTA((thread)->user_timer,			\
		(thread)->user_timer_save, delta);		\
	(thread)->cpu_delta += delta;				\
	(thread)->sched_delta += delta * 			\
			(thread)->processor_set->sched_load;	\
MACRO_END

#if	SIMPLE_CLOCK
/*
 *	sched_usec is an exponential average of number of microseconds
 *	in a second for clock drift compensation.
 */

extern int	sched_usec;
#endif	/* SIMPLE_CLOCK */

#endif	/* _KERN_SCHED_H_ */
