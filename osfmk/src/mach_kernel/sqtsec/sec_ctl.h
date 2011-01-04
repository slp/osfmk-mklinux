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

/* CMU_HIST */
/*
 * Revision 2.3  91/07/31  18:07:47  dbg
 * 	Changed copyright.
 * 	[91/07/31            dbg]
 * 
 * Revision 2.2  91/05/08  13:07:32  dbg
 * 	Added, from Sequent SYMMETRY sources.
 * 	[91/02/26            dbg]
 * 
 */

/*
 * $Header: /u1/osc/rcs/mach_kernel/sqtsec/sec_ctl.h,v 1.2.6.1 1994/09/23 03:11:46 ezf Exp $
 */

/*
 * sec_ctl.h
 * 	Definitions of the common structures used by SCSI/Ether drivers.
 */

/*
 * Revision 1.1  89/07/05  13:20:16  kak
 * Initial revision
 * 
 */

#ifndef	_SQT_SEC_CTL_H_
#define	_SQT_SEC_CTL_H_

/*
 * struct sed_dc - structure to maintain the queue's with the macros
 * below.
 */
struct seddc {
	/*
	 * device program management.
	 */
	struct	sec_cib *dc_cib;		/* cib ptr */
	struct	sec_progq *dc_diq;		/* diq ptr */
	struct	sec_progq *dc_doq;		/* doq ptr */
	int	dc_dsz;			/* doq size */
	int	dc_dp;			/* number of device programs */
	int	dc_dfree;
	struct	sec_dev_prog *dc_devp;	/* head of the device program ring */
	struct	sec_dev_prog *dc_sense;	/* aux device program for sense info */
	/*
	 * iat management.
	 */
	struct	sec_iat *dc_iat;	/* Start of iats */
	struct	sec_iat	*dc_istart;	/* current useable iat ring position */
	int	dc_isz;			/* iat ring size */
	int	dc_ihead;		/* iat head */
	int	dc_itail;		/* iat tail */
	int	dc_ifree;		/* current ring allocation */
};

#endif	/* _SQT_SEC_CTL_H_ */
