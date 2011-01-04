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
 *	Machine-dependent definitions for cpu identification.
 *
 */
#ifndef	_PPC_CPU_NUMBER_H_
#define	_PPC_CPU_NUMBER_H_

#include <cpus.h>

#include <ppc/exception.h>

#if	NCPUS > 1

static __inline__ int cpu_number(void)
{
	register int cpu;

	__asm__ volatile
		("lhz %0, %1(2)"
		 : "=r" (cpu)
		 : "n" ((unsigned int)
			(&((struct per_proc_info *)0)->cpu_number)));
	return cpu;
}

#else	/* NCPUS > 1 */
#error cpu_number() macro is defined only on MP machines
#endif	/* NCPUS > 1 */

#endif	/* _PPC_CPU_NUMBER_H_ */
