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

#ifndef	I386_CPU_DATA
#define I386_CPU_DATA

#include <cpus.h>
#include <mach_assert.h>

#if	defined(__GNUC__)

#include <kern/assert.h>

#if 0
#ifndef	__OPTIMIZE__
#define extern static
#endif
#endif

extern struct thread_shuttle __inline__ *current_thread(void);
extern int 	__inline__	get_preemption_level(void);
extern void 	__inline__	disable_preemption(void);
extern void 	__inline__      enable_preemption(void);
extern void 	__inline__      enable_preemption_no_check(void);
extern void 	__inline__	mp_disable_preemption(void);
extern void 	__inline__      mp_enable_preemption(void);
extern void 	__inline__      mp_enable_preemption_no_check(void);
extern int 	__inline__      get_simple_lock_count(void);
extern int 	__inline__      get_interrupt_level(void);

extern struct thread_shuttle __inline__	*current_thread(void)
{
	register struct thread_shuttle	*ct;
	register int		idx = (int)&((cpu_data_t *)0)->active_thread;

	__asm__ volatile ("	movl %%gs:(%1),%0" : "=r" (ct) : "r" (idx));

	return (ct);
}

extern int __inline__		get_preemption_level(void)
{
	register int	idx = (int)&((cpu_data_t *)0)->preemption_level;
	register int	pl;

	__asm__ volatile ("	movl %%gs:(%1),%0" : "=r" (pl) : "r" (idx));

	return (pl);
}

extern void __inline__		disable_preemption(void)
{
#if	MACH_ASSERT
	extern void _disable_preemption(void);

	_disable_preemption();
#else	/* MACH_ASSERT */
	register int	idx = (int)&((cpu_data_t *)0)->preemption_level;

	__asm__ volatile ("	incl %%gs:(%0)" : : "r" (idx));
#endif	/* MACH_ASSERT */
}

extern void __inline__		enable_preemption(void)
{
#if	MACH_ASSERT
	extern void _enable_preemption(void);

	assert(get_preemption_level() > 0);
	_enable_preemption();
#else	/* MACH_ASSERT */
	extern void		kernel_preempt_check (void);
	register int	idx = (int)&((cpu_data_t *)0)->preemption_level;
	register void (*kpc)(void)=	kernel_preempt_check;

	__asm__ volatile ("decl %%gs:(%0); jne 1f; \
			call %1; 1:"
			: /* no outputs */
			: "r" (idx), "r" (kpc)
			: "%eax", "%ecx", "%edx", "cc", "memory");
#endif	/* MACH_ASSERT */
}

extern void __inline__		enable_preemption_no_check(void)
{
#if	MACH_ASSERT
	extern void _enable_preemption_no_check(void);

	assert(get_preemption_level() > 0);
	_enable_preemption_no_check();
#else	/* MACH_ASSERT */
	register int	idx = (int)&((cpu_data_t *)0)->preemption_level;

	__asm__ volatile ("decl %%gs:(%0)"
			: /* no outputs */
			: "r" (idx)
			: "cc", "memory");
#endif	/* MACH_ASSERT */
}

extern void __inline__		mp_disable_preemption(void)
{
#if	NCPUS > 1
	disable_preemption();
#endif	/* NCPUS > 1 */
}

extern void __inline__		mp_enable_preemption(void)
{
#if	NCPUS > 1
	enable_preemption();
#endif	/* NCPUS > 1 */
}

extern void __inline__		mp_enable_preemption_no_check(void)
{
#if	NCPUS > 1
	enable_preemption_no_check();
#endif	/* NCPUS > 1 */
}

extern int __inline__		get_simple_lock_count(void)
{
	register int	idx = (int)&((cpu_data_t *)0)->simple_lock_count;
	register int	pl;

	__asm__ volatile ("	movl %%gs:(%1),%0" : "=r" (pl) : "r" (idx));

	return (pl);
}

extern int __inline__		get_interrupt_level(void)
{
	register int	idx = (int)&((cpu_data_t *)0)->interrupt_level;
	register int	pl;

	__asm__ volatile ("	movl %%gs:(%1),%0" : "=r" (pl) : "r" (idx));

	return (pl);
}

#if 0
#ifndef	__OPTIMIZE__
#undef 	extern 
#endif
#endif

#else	/* !defined(__GNUC__) */

#endif	/* defined(__GNUC__) */

#endif	/* I386_CPU_DATA */
