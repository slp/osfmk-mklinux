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
 * Revision 2.5.5.2  92/04/30  11:58:45  bernadat
 * 	Removed extra line in ivect table (fpintr used twice)
 * 	[92/04/27            bernadat]
 * 
 * 	Adaptations for Systempro
 * 	Use fpintr again, as it as been fixed
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.5.5.1  92/02/18  18:57:10  jeffreyh
 * 	Suppressed fpintr vector (see i386/locore.s)
 * 	[91/12/06            bernadat]
 * 
 * Revision 2.5  91/05/14  16:29:38  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:20:18  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:46:58  mrt]
 * 
 * Revision 2.3  91/01/08  17:33:40  rpd
 * 	Initially, do not allow clock interrupts
 * 	[91/01/04  12:21:54  rvb]
 * 
 * Revision 2.2  90/11/26  14:50:57  rvb
 * 	jsb bet me to XMK34, sigh ...
 * 	[90/11/26            rvb]
 * 	Apparently first revision is -r2.2
 * 	[90/11/25  10:47:33  rvb]
 * 
 * 	Synched 2.5 & 3.0 at I386q (r2.1.1.2) & XMK35 (r2.2)
 * 	[90/11/15            rvb]
 * 
 * 	We don't want to see clock interrupts till clkstart(),
 * 	but we can not turn the interrupt off so we disable them.
 * 	2.5 ONLY!
 * 	[90/11/05            rvb]
 * 	Created based on pic.c
 * 	[90/06/16            rvb]
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

#include <types.h>
#include <chips/busses.h>
#include <mach/i386/thread_status.h>
#include <i386/thread.h>
#include <i386/ipl.h>
#include <i386/pic.h>
#include <i386/fpu.h>
#include <i386/hardclock_entries.h>
#include <i386/misc_protos.h>
#include <i386/AT386/misc_protos.h>
#include <i386/AT386/kdsoft.h>
#include <platforms.h>
#include <cpus.h>

#if	MBUS && NCPUS > 1
#include <busses/mbus/mbus.h>
#endif	/* MBUS && NCPUS > 1 */

intr_t ivect[NINTR]= {
	/* 00 */	(intr_t)hardclock,	/* always */
	/* 01 */	(intr_t)kdintr,		/* kdintr, ... */
	/* 02 */	(intr_t)intnull,
	/* 03 */	(intr_t)intnull,	/* lnpoll, comintr, ... */

	/* 04 */	(intr_t)intnull,	/* comintr, ... */
	/* 05 */	(intr_t)intnull,	/* comintr, wtintr, ... */
	/* 06 */	(intr_t)intnull,	/* fdintr, ... */
	/* 07 */	(intr_t)intnull,	/* qdintr, ... */

	/* 08 */	(intr_t)intnull,
	/* 09 */	(intr_t)intnull,	/* ether */
	/* 10 */	(intr_t)intnull,
	/* 11 */	(intr_t)intnull,

	/* 12 */	(intr_t)intnull,

#if	MBUS && NCPUS > 1
	/* Inter processor interrupt also use this vector */
	/* 13 */	(intr_t)mp_intr,	/* always */
#else	/* MBUS && NCPUS > 1 */
	/* 13 */	(intr_t)fpintr,		/* always */
#endif	/* MBUS && NCPUS > 1 */
	/* 14 */	(intr_t)intnull,	/* hdintr, ... */
	/* 15 */	(intr_t)intnull,	/* ??? */
};

u_char intpri[NINTR] = {
	/* 00 */   	0,	SPL6,	0,	0,
	/* 04 */	0,	0,	0,	0,
	/* 08 */	0,	0,	0,	0,
	/* 12 */	0,	SPL1,	0,	0,
};
