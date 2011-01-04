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
 * 
 */
/*
 * MkLinux
 */

/*
 *	File:	dipc/dipc_timer.h
 *	Author:	Steve Sears
 *	Date:	1994
 *
 *	General purpose timer functions
 *	Includes:
 *		Ability to set up a timer with an arbitrary name and
 *		associate an ASCII string name with it.
 *		Ability to start and stop a timer multiple times.
 *		Ability to continuously bump a timer and obtain
 *		the aggregate time.
 *		Ability to accumulate arbitrary values with counter.
 */

#include <dipc_timer.h>

void		dipc_timer_init(void);

#if	DIPC_TIMER

kern_return_t	timer_set(
			unsigned int	key,
			char		*name);

kern_return_t	timer_start(
			unsigned int	key);

kern_return_t	timer_stop(
			unsigned int	key,
			unsigned int	add);

kern_return_t	timer_check(
			unsigned int	key);

kern_return_t	timer_free(
			unsigned int	key);

kern_return_t	timer_add(
			unsigned int	prin,
			unsigned int	adden);
kern_return_t	timer_abort(
			unsigned int	key);
void		timer_clear_tree(void);

#if	MACH_KDB

void		db_timer_print(void);

void		db_show_one_timer(
			unsigned int	key);

#endif	/* MACH_KDB */

#else	/* DIPC_TIMER */

#define	timer_set(a,b)
#define	timer_start(a)
#define	timer_stop(a,b)
#define	timer_check(a)
#define	timer_free(a)
#define	timer_add(a,b)

#endif	/* DIPC_TIMER */
