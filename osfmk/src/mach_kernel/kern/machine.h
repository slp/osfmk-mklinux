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

#ifndef	_KERN_MACHINE_H_
#define	_KERN_MACHINE_H_

#include <cpus.h>
#include <mach/processor_info.h>
#include <kern/processor.h>
#include <kern/thread.h>

/*
 * Machine support declarations.
 */

extern void		action_thread(void);

extern void		cpu_down(
				int	cpu);

extern void		cpu_up(
				int	cpu);

/*
 * Must be implemented in machine dependent code.
 */

#if	NCPUS > 1
/* Initialize machine dependent ast code */
extern void		init_ast_check(
				processor_t	processor);

/* Cause check for ast */
extern void		cause_ast_check(
				processor_t	processor);

extern kern_return_t	cpu_start(
				int	slot_num);

extern kern_return_t	cpu_control(
				int			slot_num,
				processor_info_t	info,
				unsigned int		count);

extern void		switch_to_shutdown_context(
				thread_t	thread,
				void		(*doshutdown)(processor_t),
				processor_t	processor);

#endif	/* NCPUS > 1 */
#endif	/* _KERN_MACHINE_H_ */
