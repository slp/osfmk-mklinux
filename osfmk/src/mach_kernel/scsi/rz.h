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
 * Revision 2.7.4.2  92/04/30  12:00:16  bernadat
 * 	Used defines for controller, slave and partition fields
 * 	in dev number since they may be used elsewhere.
 * 	Changes from TRUNK:
 * 		Fabricate extra partition info to deal with the
 * 		first alternate partition range and "PARITITON_ABSOLUTE".
 * 		[92/04/01            rvb]
 * 
 * Revision 2.7.4.1  92/02/18  19:19:45  jeffreyh
 * 	Increase number of disk partitions to 16 to be compatible with 2.5
 * 	[92/02/08            bernadat]
 * 
 * Revision 2.7  91/06/19  11:56:57  rvb
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/04            rvb]
 * 
 * 	A couple of macros were not fully parenthesized, which screwed
 * 	up the second scsi bus on Vaxen.  [This was the only bug in the
 * 	multi-bus code, amazing].
 * 	[91/05/30            af]
 * 
 * Revision 2.6  91/05/14  17:26:19  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/05/13  06:04:20  af
 * 	Redefined naive macro names to avoid conflicts.
 * 	[91/05/12  16:08:55  af]
 * 
 * Revision 2.4  91/02/05  17:43:42  mrt
 * 	Added author notices
 * 	[91/02/04  11:16:33  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:15:29  mrt]
 * 
 * Revision 2.3  90/12/05  23:33:55  af
 * 
 * 
 * Revision 2.1.1.1  90/11/01  03:43:37  af
 * 	Created.
 * 	[90/10/21            af]
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
 *	File: rz.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	9/90
 *
 *	Mapping between U*x-like indexing and controller+slave
 *	Each controller handles at most 8 slaves, few controllers.
 */

#ifndef	_SCSI_RZ_
#define	_SCSI_RZ_

#define RZCONTROLLER_SHIFT	7
#define RZSLAVE_SHIFT		4

#define RZCONTROLLER_MASK	0x1
#define	RZSLAVE_MASK		0x7
#define RZPARTITION_MASK	0xf
#define TZLUN_MASK		0x7

/*
 * Until I figure a way around the server/minor number limitation
 * I will define rzpassthru to be the 'p' partition
 *
 * -jerrie
 */
#define	rzpassthru(dev)		(((dev)&RZPARTITION_MASK)==0xf)
#define	rzcontroller(dev)	(((dev)>>RZCONTROLLER_SHIFT)&RZCONTROLLER_MASK)
#define	rzslave(dev)		(((dev)>>RZSLAVE_SHIFT)&RZSLAVE_MASK)
#define	rzlun(dev)		(0)
#define	tzlun(dev)		((dev)&TZLUN_MASK)
#define	rzpartition(dev)	((PARTITION_TYPE(dev)==0xf)?MAXPARTITIONS:((dev)&RZPARTITION_MASK))

#define PARTITION_TYPE(dev)	(((dev)>>24)&0xf)
#define PARTITION_ABSOLUTE	(0xf<<24)

#define RZDISK_MAX_SECTOR       65536

#define RZ_DEFAULT_BSIZE        512

#ifdef	MACH_KERNEL
#else	/*MACH_KERNEL*/
#define tape_unit(dev)		((((dev)&0xe0)>>3)|((dev)&0x3))
#define	TAPE_UNIT(dev)		((dev)&(~0xff))|(tape_unit((dev))<<3)
#define	TAPE_REWINDS(dev)	(((dev)&0x1c)==0)||(((dev)&0x1c)==8)
#endif	/*MACH_KERNEL*/

#endif	/* _SCSI_RZ_ */





