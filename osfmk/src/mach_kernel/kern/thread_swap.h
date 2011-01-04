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
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * Copyright (c) 1988 Carnegie-Mellon University
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * MkLinux
 */
/*
 *	File:	kern/thread_swap.h
 *
 *	Declarations of thread swap_states and swapping routines.
 */

/*
 *	Swap states for threads.
 */

#ifndef	_KERN_THREAD_SWAP_H_
#define _KERN_THREAD_SWAP_H_

#if	THREAD_SWAPPER
#define	TH_SW_STATE		7	/* mask of swap state bits */
#define TH_SW_UNSWAPPABLE	1	/* not swappable */
#define TH_SW_IN		2	/* swapped in */
#define TH_SW_GOING_OUT		3	/* being swapped out */
#define TH_SW_WANT_IN		4	/* being swapped out, but should
					   immediately be swapped in */
#define TH_SW_OUT		5	/* swapped out */
#define TH_SW_COMING_IN		6	/* queued for swapin, or being
					   swapped in */

#define TH_SW_MAKE_UNSWAPPABLE	8	/*not state, command to swapin_thread */

/* 
 * This flag is only used by the task swapper.  It implies that
 * the thread is about to be swapped, but hasn't yet.
 */
#define TH_SW_TASK_SWAPPING	0x10

/*
 *	exported routines
 */
extern void	thread_swapper_init(void);
extern void	swapin_thread(void);
extern void	swapout_thread(void);
extern void	thread_doswapin(thread_act_t thr_act);
extern void	thread_swapin(thread_act_t thr_act,
			      boolean_t make_unswappable);
extern boolean_t
		thread_swapin_blocking(thread_act_t thr_act);
extern void	thread_swapout(thread_act_t thr_act);
extern void	swapout_threads(boolean_t now);
extern void	thread_swapout_enqueue(thread_act_t thr_act);
extern void	thread_swap_disable(thread_act_t thr_act);

extern void	thread_swappable(thread_act_t thr_act, boolean_t swappable);

#else	/* THREAD_SWAPPER */
#define		thread_swappable(thr_act, swappable)
#endif	/* THREAD_SWAPPER */


#endif	/*_KERN_THREAD_SWAP_H_*/
