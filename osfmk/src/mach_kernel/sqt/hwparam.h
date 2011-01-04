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
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * Copyright (c) 1991 Sequent Computer Systems
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND SEQUENT COMPUTER SYSTEMS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * SEQUENT COMPUTER SYSTEMS DISCLAIM ANY LIABILITY OF ANY KIND FOR
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
 * HISTORY
 * $Log: hwparam.h,v $
 * Revision 1.2.10.1  1994/09/23  03:04:41  ezf
 * 	change marker to not FREE
 * 	[1994/09/22  21:55:15  ezf]
 *
 * Revision 1.2.9.2  1994/09/22  21:55:15  ezf
 * 	change marker to not FREE
 *
 * Revision 1.2.2.2  1993/06/09  02:49:53  gm
 * 	Added to OSF/1 R1.3 from NMK15.0.
 * 	[1993/06/02  21:27:47  jeffc]
 *
 * Revision 1.2.1.2  1993/06/09  01:13:49  gm
 * 	Added to OSF/1 R1.3 from NMK15.0.
 * 	[1993/06/02  21:27:47  jeffc]
 *
 * Revision 1.1.4.2  1993/06/03  00:10:12  jeffc
 * 	Added to OSF/1 R1.3 from NMK15.0.
 * 	[1993/06/02  21:27:47  jeffc]
 *
 * Revision 1.1.3.2  1993/06/02  21:27:47  jeffc
 * 	Added to OSF/1 R1.3 from NMK15.0.
 *
 * Revision 1.2  1993/04/19  17:05:08  devrcs
 * 	Initial Revision
 * 	[1993/01/18  09:24:01  bernadat]
 *
 * Revision 1.1.2.2  1993/01/18  14:47:08  bernadat
 * 	Initial Revision
 * 	[1993/01/18  09:24:01  bernadat]
 *
 * Revision 1.1.1.2  1993/01/18  09:24:01  bernadat
 * 	Initial Revision
 *
 * Revision 2.3  91/07/31  18:00:51  dbg
 * 	Changed copyright.
 * 	[91/07/31            dbg]
 * 
 * Revision 2.2  91/05/08  12:56:01  dbg
 * 	Added, from Sequent SYMMETRY sources.
 * 	[91/04/26  14:51:55  dbg]
 * 
 */

/*
 * $Header: /u1/osc/rcs/mach_kernel/sqt/hwparam.h,v 1.2.10.1 1994/09/23 03:04:41 ezf Exp $
 *
 * hwparam.h
 *	Define physical addresses and other parameters of the hardware.
 */

/* $Log: hwparam.h,v $
 * Revision 1.2.10.1  1994/09/23  03:04:41  ezf
 * 	change marker to not FREE
 * 	[1994/09/22  21:55:15  ezf]
 *
 * Revision 1.2.9.2  1994/09/22  21:55:15  ezf
 * 	change marker to not FREE
 *
 * Revision 1.2.2.2  1993/06/09  02:49:53  gm
 * 	Added to OSF/1 R1.3 from NMK15.0.
 * 	[1993/06/02  21:27:47  jeffc]
 *
 * Revision 1.2.1.2  1993/06/09  01:13:49  gm
 * 	Added to OSF/1 R1.3 from NMK15.0.
 * 	[1993/06/02  21:27:47  jeffc]
 *
 * Revision 1.1.4.2  1993/06/03  00:10:12  jeffc
 * 	Added to OSF/1 R1.3 from NMK15.0.
 * 	[1993/06/02  21:27:47  jeffc]
 *
 * Revision 1.1.3.2  1993/06/02  21:27:47  jeffc
 * 	Added to OSF/1 R1.3 from NMK15.0.
 *
 * Revision 1.2  1993/04/19  17:05:08  devrcs
 * 	Initial Revision
 * 	[1993/01/18  09:24:01  bernadat]
 *
 * Revision 1.1.2.2  1993/01/18  14:47:08  bernadat
 * 	Initial Revision
 * 	[1993/01/18  09:24:01  bernadat]
 *
 * Revision 1.1.1.2  1993/01/18  09:24:01  bernadat
 * 	Initial Revision
 *
 * Revision 2.3  91/07/31  18:00:51  dbg
 * 	Changed copyright.
 * 	[91/07/31            dbg]
 * 
 * Revision 2.2  91/05/08  12:56:01  dbg
 * 	Added, from Sequent SYMMETRY sources.
 * 	[91/04/26  14:51:55  dbg]
 * 
 * Revision 2.1.2.1  91/04/26  14:51:55  dbg
 * 	Added, from Sequent SYMMETRY sources.
 * 
 * Revision 2.1.1.1  91/02/26  11:02:26  dbg
 * 	Added, from Sequent SYMMETRY sources.
 * 	[91/02/26            dbg]
 * 
 * Revision 1.2  89/07/20  18:05:01  kak
 * moved balance includes
 * 
 * Revision 1.1  89/07/05  13:15:32  kak
 * Initial revision
 * 
 */

#ifndef	_SQT_HWPARAM_H_
#define	_SQT_HWPARAM_H_

/*
 * Physical addresses in processor board address space.
 */

#ifdef	KXX

#define	PHYS_SLIC	0x1BF0000	/* phys loc in CPU addr space */
#define	PHYS_SLIC_INTR	0x1BF1000	/* slic's interrupt vector register */
#define	PHYS_CACHE0SET	0x1BF8000	/* cache 0 set data */
#define	PHYS_CACHE1SET	0x1BF9000	/* cache 1 set data */
#define	PHYS_STATICRAM	0x1BFA000	/* local static RAM */
#define	PHYS_CACHE0REV	0x1BFC000	/* cache set 0 (parity reversed) */
#define	PHYS_CACHE1REV	0x1BFD000	/* cache set 1 (parity reversed) */
#define	PHYS_RAMREV	0x1BFE000	/* local static RAM (parity reversed) */

#define	PHYS_MBAD	0x1C00000	/* MBAd base address (phys) */

/*
 * Maximum processor addressable main-memory address is limited to 24Meg
 * on K20, due to naive mapping of SLIC in machine/startup.c
 * (alloclocalIO(), maplocalIO()).
 */

#define	MAX_PROC_ADDR_MEM (24*1024*1024)

#else	Real HW

#include <sqt/SGSproc.h>		/* SGS processor specific definitions */

#endif	KXX

/*
 * Define addressibility of Multi-bus interface.
 */

#define	VIRT_MBAD	PHYS_MBAD	/* MBAd base address (virt) */
#define	MBAD_ADDR_SPACE	(1024*1024)	/* 1M address space */

#define	VA_MBAd(i)	(VIRT_MBAD + (i) * MBAD_ADDR_SPACE)
#define	PA_MBAd(i)	(PHYS_MBAD + (i) * MBAD_ADDR_SPACE)

/*
 * For backwards compatibility -- MBAds started at fixed address.
 */

#define	OLD_VIRT_MBAD	0x0800000	/* MBAd base address (virt) */

#endif	/* _SQT_HWPARAM_H_ */
