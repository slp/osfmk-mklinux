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
 *	        File:	kern/task_swap.h
 *      
 *	Task residency management primitives declarations.
 */

#ifndef	_KERN_TASK_SWAP_H_
#define	_KERN_TASK_SWAP_H_

#include <kern/host.h>

/*
 *	swap states
 */
#define TASK_SW_UNSWAPPABLE	1	/* not swappable */
#define TASK_SW_IN		2	/* swapped in (resident) */
#define TASK_SW_OUT		3	/* swapped out (non-resident) */
#define TASK_SW_COMING_IN	4	/* about to be swapped in */
#define TASK_SW_GOING_OUT	5	/* being swapped out */

/*
 *	swap flags
 */
#define TASK_SW_MAKE_UNSWAPPABLE	0x01	/* make it unswappable */
#define TASK_SW_WANT_IN			0x02	/* sleeping on state */
#define TASK_SW_ELIGIBLE		0x04	/* eligible for swapping */

/*
 * exported routines
 */
extern void task_swapper_init(void);
extern kern_return_t task_swapin(
				task_t,		/* task */
				boolean_t);	/* make_unswappable */
extern kern_return_t task_swapout(task_t /* task */);
extern void task_swapper(void);
extern void task_swap_swapout_thread(void);
extern void compute_vm_averages(void);
extern kern_return_t task_swappable(
				host_t,		/* host */
				task_t,		/* task */
				boolean_t); 	/* swappable */
extern void task_swapout_eligible(task_t /* task */);
extern void task_swapout_ineligible(task_t /* task */);
extern void swapout_ast(void);

#endif	/* _KERN_TASK_SWAP_H_ */
