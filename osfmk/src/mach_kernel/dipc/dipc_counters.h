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
 *	File:	dipc/dipc_counters.h
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	DIPC internal statistics.
 */

#ifndef	_DIPC_DIPC_COUNTERS_H_
#define	_DIPC_DIPC_COUNTERS_H_

#include <dipc_stats.h>
#include <mach_assert.h>

#define	DIPC_DO_STATS	(DIPC_STATS || MACH_ASSERT)


/*
 *	Macros for declaring and using statistics variables.
 *	These variables are present whenever statistics
 *	or assertion checking are enabled.
 */
#if	DIPC_DO_STATS
#define	dstat_decl(clause)	clause
#define	dstat(clause)		clause
#define	dstat_max(m,c)		((c) > (m) ? (m) = (c) : 0)
#else	/* DIPC_STATS || MACH_ASSERT */
#define	dstat_decl(clause)
#define	dstat(clause)
#define	dstat_max(m,c)
#endif	/* DIPC_STATS || MACH_ASSERT */

/*
 *	Macros for declaring and using debugging counters.
 *	These variables are present only when assertion
 *	checking is enabled.
 */
#if	MACH_ASSERT
#define	dcntr_decl(clause)	clause
#define	dcntr(clause)		clause
#else	/* MACH_ASSERT */
#define	dcntr_decl(clause)
#define	dcntr(clause)
#endif	/* MACH_ASSERT */

#endif	/* _DIPC_DIPC_COUNTERS_H_ */
