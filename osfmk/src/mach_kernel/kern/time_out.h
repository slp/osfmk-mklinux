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
 * Revision 2.5  91/07/31  17:51:15  dbg
 * 	Fix race condition.
 * 	[91/07/30  17:08:00  dbg]
 * 
 * Revision 2.4  91/05/14  16:49:27  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:30:51  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:20:31  mrt]
 * 
 * Revision 2.2  90/11/05  14:32:00  rpd
 * 	Changed untimeout to return boolean.
 * 	[90/10/29            rpd]
 * 
 * Revision 2.1  89/08/03  15:57:24  rwd
 * Created.
 * 
 * 14-Jun-88  David Golub (dbg) at Carnegie-Mellon University
 *	Created.
 *
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

#ifndef	_KERN_TIME_OUT_H_
#define	_KERN_TIME_OUT_H_

/*
 * Mach time-out and time-of-day facility.
 */

#include <mach/boolean.h>
#include <kern/lock.h>
#include <kern/queue.h>
#include <kern/zalloc.h>
#include <kern/processor.h>

/*
 *	Timers in kernel:
 */
extern unsigned long elapsed_ticks; /* number of ticks elapsed since bootup */
extern int	hz;		/* number of ticks per second */
extern int	tick;		/* number of usec per tick */

typedef void (*timeout_fcn_t)(void *);

/*
 *	Time-out element.
 */
struct timer_elt {
	queue_chain_t	chain;		/* chain in order of expiration */
	timeout_fcn_t	fcn;		/* function to call */
	void 		*param;		/* with this parameter */
	natural_t	ticks;		/* expiration time, in ticks */
	int		set;		/* unset | set | allocated */
#if	NCPUS > 1
	processor_t	bound_processor; /* Must be executed on this proc. */
#endif	/* NCPUS > 1 */
};
#define	TELT_UNSET	0		/* timer not set */
#define	TELT_SET	1		/* timer set */
#define	TELT_ALLOC	2		/* timer allocated from pool */
#define TELT_PENDING	3		/* timer fired, but fcn not done yet */

typedef	struct timer_elt	timer_elt_data_t;
typedef	struct timer_elt	*timer_elt_t;

#define	TIMER_ELT_NULL	(timer_elt_t)0


/* Initialize timeout module */
extern void	timeout_init(void);

/* Softclock timer thread */
extern void	timeout_thread(void);

extern void	hertz_tick(
			boolean_t	usermode, /* executing user code */
			natural_t	pc);

extern	void	softclock(void);

/* for 'private' timer elements */

/* Set timer going */
extern void		set_timeout(
				timer_elt_t	telt,	/* already loaded */
				natural_t 	interval);

/* Reset timer */
extern boolean_t	reset_timeout(
				timer_elt_t	telt);

/* for public timer elements */

/* Set timeout */
extern void		timeout(
				timeout_fcn_t	fcn,
				void		*param,
				int		interval);

/* Cancel timeout */
extern boolean_t	untimeout(
				timeout_fcn_t	fcn,
				void		*param);

#if	NCPUS > 1
#define	set_timeout_setup(telt, func, arg, interval, processor)	\
MACRO_BEGIN 							\
	(telt)->fcn = (func);					\
	(telt)->param = (char *)(arg);				\
	(telt)->set = TELT_UNSET;				\
	(telt)->bound_processor = processor;			\
	set_timeout((telt), (interval));			\
MACRO_END
#else	/* NCPUS > 1 */
#define	set_timeout_setup(telt, func, arg, interval, processor)	\
MACRO_BEGIN 							\
	(telt)->fcn = (func);					\
	(telt)->param = (char *)(arg);				\
	(telt)->set = TELT_UNSET;				\
	set_timeout((telt), (interval));			\
MACRO_END
#endif	/* NCPUS > 1 */

#define	reset_timeout_check(t) 			\
MACRO_BEGIN					\
	if ((t)->set) 				\
	    reset_timeout((t)); 		\
MACRO_END

#endif	/* _KERN_TIME_OUT_H_ */
