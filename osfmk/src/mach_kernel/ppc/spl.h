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

#ifndef	_MACHINE_SPL_H_
#define	_MACHINE_SPL_H_

/*
 *	This file defines the interrupt priority levels used by
 *	machine-dependent code.
 *
 *      The powerpc has only primitive interrupt masking (all/none),
 *      spl behaviour is done in a platform-dependant manner
 */

#include <mach_kdb.h>
#include <mach_debug.h>

 /* Note also : if any new SPL's are introduced, please add to debugging list*/
#define SPLOFF		0       /* all interrupts disabled TODO NMGS  */
#define	SPLPOWER	1	/* power failure (unused) */
#define	SPLHIGH	        2	/* TODO NMGS any non-zero, non-INTPRI value */
#define SPLSCHED	SPLHIGH
#define	SPLCLOCK	SPLSCHED /* hard clock */
#define	SPLVM		4       /* pmap manipulations */
#define	SPLBIO		8	/* block I/O */
#define	SPLIMP		8	/* network & malloc */
#define	SPLTTY		16	/* TTY */
#define	SPLNET		24	/* soft net */
#define	SPLSCLK		27	/* soft clock */
#define	SPLLO		32	/* no interrupts masked */

/* internal - masked in to spl level if ok to lower priority (splx, splon)
 * the mask bit is never seen externally
 */
#define SPL_LOWER_MASK	0x8000

#define SPL_CMP_GT(a, b)        ((unsigned)(a) >  (unsigned)(b))
#define SPL_CMP_LT(a, b)        ((unsigned)(a) <  (unsigned)(b))
#define SPL_CMP_GE(a, b)        ((unsigned)(a) >= (unsigned)(b))
#define SPL_CMP_LE(a, b)        ((unsigned)(a) <= (unsigned)(b))

#ifndef ASSEMBLER

typedef unsigned spl_t;

/* SPL implementation is platform dependant */

extern spl_t	(*platform_spl)(spl_t level);

#define sploff()	platform_spl(SPLOFF)
#define splhigh()	platform_spl(SPLHIGH)
#define splsched()	platform_spl(SPLSCHED)
#define splclock()	platform_spl(SPLCLOCK)
#define splvm()		platform_spl(SPLVM)
#define splbio()	platform_spl(SPLBIO)
#define splimp()	platform_spl(SPLIMP)
#define spltty()	platform_spl(SPLTTY)
#define splnet()	platform_spl(SPLNET)
#define spllo()		platform_spl(SPLLO   | SPL_LOWER_MASK)
#define splx(l)		(void)platform_spl(l | SPL_LOWER_MASK)
#define splon(l)	(void)platform_spl(l | SPL_LOWER_MASK)

#if	MACH_KDB
extern spl_t (*platform_db_spl)(spl_t level);

#define db_splhigh()	platform_db_spl(SPLHIGH)
#define db_splx(l)	(void)platform_db_spl(l | SPL_LOWER_MASK)
#endif	/* MACH_KDB */

#endif /* ASSEMBLER */

#endif	/* _MACHINE_SPL_H_ */
