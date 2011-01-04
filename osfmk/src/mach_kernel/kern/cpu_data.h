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

#ifndef	_CPU_DATA_H_
#define	_CPU_DATA_H_

#include <mach_rt.h>

typedef struct
{
	struct thread_shuttle	*active_thread;
#if	MACH_RT
	int		preemption_level;
#endif	/* MACH_RT */
	int		simple_lock_count;
	int		interrupt_level;
} cpu_data_t;

extern cpu_data_t	cpu_data[NCPUS];

/*
 * NOTE: For non-realtime configurations, the accessor functions for
 * cpu_data fields are here, coded in C. For MACH_RT configurations,
 * the accessor functions must be defined in the machine-specific
 * cpu_data.h header file.
 */

#if	MACH_RT

#include <machine/cpu_data.h>

#else	/* MACH_RT */

#include <kern/cpu_number.h>

#if	defined(__GNUC__)

#ifndef __OPTIMIZE__
#define extern static
#endif

extern struct thread_shuttle __inline__ *current_thread(void);
extern int __inline__		get_preemption_level(void);
extern int __inline__		get_simple_lock_count(void);
extern int __inline__		get_interrupt_level(void);

extern void __inline__		disable_preemption(void);
extern void __inline__		enable_preemption(void);
extern void __inline__		enable_preemption_no_check(void);

extern void __inline__		mp_disable_preemption(void);
extern void __inline__		mp_enable_preemption(void);
extern void __inline__		mp_enable_preemption_no_check(void);

extern struct thread_shuttle __inline__ *current_thread(void)
{
	return (cpu_data[cpu_number()].active_thread);
}

extern int __inline__	get_preemption_level(void)
{
	return (0);
}

extern int __inline__	get_simple_lock_count(void)
{
	return (cpu_data[cpu_number()].simple_lock_count);
}

extern int __inline__	get_interrupt_level(void)
{
	return (cpu_data[cpu_number()].interrupt_level);
}

extern void __inline__	disable_preemption(void)
{
}

extern void __inline__	enable_preemption(void)
{
}

extern void __inline__	enable_preemption_no_check(void)
{
}

extern void __inline__	mp_disable_preemption(void)
{
}

extern void __inline__	mp_enable_preemption(void)
{
}

extern void __inline__	mp_enable_preemption_no_check(void)
{
}

#ifndef	__OPTIMIZE__
#undef 	extern
#endif

#else	/* !defined(__GNUC__) */

#define current_thread()	(cpu_data[cpu_number()].active_thread)
#define get_preemption_level()	(0)
#define get_simple_lock_count()	(cpu_data[cpu_number()].simple_lock_count)
#define get_interrupt_level()	(cpu_data[cpu_number()].interrupt_level)
#define disable_preemption()	
#define enable_preemption()
#define enable_preemption_no_check()
#define mp_disable_preemption()	
#define mp_enable_preemption()
#define mp_enable_preemption_no_check()

#endif	/* defined(__GNUC__) */

#endif	/* MACH_RT */

#endif	/* _CPU_DATA_H_ */
