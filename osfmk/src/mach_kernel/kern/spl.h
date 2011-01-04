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

#ifndef	_KERN_SPL_H_
#define	_KERN_SPL_H_

#include <cpus.h>
#include <mach_rt.h>

#include <machine/spl.h>

/*
 *	This file defines the interrupt priority levels used by
 *	machine-independent code. The machine-dependent header file
 *	<machine/spl.h> may define additional levels, inlining macros,
 *	or other extensions to these interfaces.
 */

extern spl_t	(splhigh)(void);	/* Block all interrupts */

extern spl_t	(splsched)(void);	/* Block all scheduling activity */

extern spl_t	(splclock)(void);	/* Block clock interrupt */

extern spl_t	(spltty)(void);		/* Block all terminal interrupts */

extern spl_t	(splbio)(void);		/* Block all disk and tape interrupts */

extern spl_t	(splimp)(void);		/* Block network interfaces */

extern spl_t	(splnet)(void);		/* Block software interrupts */

extern void	(splx)(spl_t);		/* Restore previous level */

extern void	(spllo)(void);		/* Enable all interrupts */

extern spl_t	(splvm)(void);

extern spl_t	(sploff)(void);		/* Disable interrupts at chip */

extern void	(splon)(spl_t);		/* Enable interrupts at chip */

extern spl_t	(getspl)(void);		/* return current level */


#include <mach_kdb.h>

#if	MACH_KDB

extern spl_t	(db_splhigh)(void);	/* Block all interrupts */

extern void	(db_splx)(spl_t);	/* Restore previous level */

#endif	/* MACH_KDB */

#endif	/* _KERN_SPL_H_ */
