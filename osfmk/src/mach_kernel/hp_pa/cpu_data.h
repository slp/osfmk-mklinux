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

#ifndef _MACHINE_CPU_DATA_H_
#define _MACHINE_CPU_DATA_H_

#include <kern/spl.h>

extern void	kernel_preempt_check(vm_offset_t);
extern void	kernel_ast(thread_t, spl_t, vm_offset_t);

#define current_thread()        (cpu_data[0].active_thread)
#define get_preemption_level()	(cpu_data[0].preemption_level)
#define disable_preemption()	(cpu_data[0].preemption_level++)
#define enable_preemption()     if((--(cpu_data[0].preemption_level)) == 0) \
					kernel_preempt_check(getrpc())
#define get_simple_lock_count()	(cpu_data[0].simple_lock_count)
#define get_interrupt_level()	(cpu_data[0].interrupt_level)

#endif /* _MACHINE_CPU_DATA_H_*/
