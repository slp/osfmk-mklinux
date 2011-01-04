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
 * Revision 2.3.9.2  92/04/30  11:49:49  bernadat
 * 	Adaptations for Corollary and Systempro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.3.9.1  92/02/18  18:43:57  jeffreyh
 * 	Support for the Corollary MP
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.3  91/05/14  16:04:27  mrt
 * 	Correcting copyright
 * 
 * Revision 2.2  91/05/08  12:30:54  dbg
 * 	Created.
 * 	[91/03/21            dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 *	Machine-dependent definitions for cpu identification.
 *
 */
#ifndef	_I386_CPU_NUMBER_H_
#define	_I386_CPU_NUMBER_H_

#include <platforms.h>
#include <mp_v1_1.h>
#include <cpus.h>

#if	NCPUS > 1

extern int	cpu_number(void);

#if	AT386
#if	MP_V1_1
#include <i386/apic.h>
#include <i386/asm.h>

extern int lapic_id;

extern __inline__ int cpu_number(void)
{
	register int cpu;

	__asm__ volatile ("movl " CC_SYM_PREFIX "lapic_id, %0\n"
			  "	movl 0(%0), %0\n"
			  "	shrl %1, %0\n"
			  "	andl %2, %0"
		    : "=r" (cpu)
		    : "i" (LAPIC_ID_SHIFT), "i" (LAPIC_ID_MASK));

	return(cpu);
}
#else	/* MP_V1_1 */
/*
 * At least one corollary cpu type does not have local memory at all.
 * The only way I found to store the cpu number was in some 386/486
 * system register. cr3 has bits 0, 1, 2 and 5, 6, 7, 8, 9, 10, 11
 * available. Right now we use 0, 1 and 2. So we are limited to 8 cpus.
 * For more cpus, we could use bits 5 - 11 with a shift.
 *
 * Even for other machines, like COMPAQ this is much faster the inb/outb
 * 4 cycles instead of 10 to 30.
 */
#if	defined(__GNUC__)
#if	NCPUS	> 8
#error	cpu_number() definition only works for #cpus <= 8
#else

extern __inline__ int cpu_number(void)
{
	register int cpu;

	__asm__ volatile ("movl %%cr3, %0\n"
		"	andl $0x7, %0"
		    : "=r" (cpu));
	return(cpu);
}
#endif
#endif	/* defined(__GNUC__) */
#endif	/* AT386 */

#if	SYMMETRY
extern int	cpu_number(void);
#endif	/* SYMMETRY */

#endif	/* MP_V1_1 */
#else	/* NCPUS > 1 */
#error cpu_number() macro is defined only on MP machines
#endif	/* NCPUS */
#endif	/* _I386_CPU_NUMBER_H_ */
