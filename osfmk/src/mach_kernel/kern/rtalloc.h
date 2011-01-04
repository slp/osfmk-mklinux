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
 * Revision 2.7  91/05/18  14:31:59  rpd
 * 	Added kalloc_init.
 * 	[91/03/22            rpd]
 * 
 * Revision 2.6  91/05/14  16:43:32  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/02/05  17:27:26  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:14:21  mrt]
 * 
 * Revision 2.4  90/06/02  14:54:51  rpd
 * 	Changed types to vm_offset_t.
 * 	[90/03/26  22:07:07  rpd]
 * 
 * Revision 2.3  89/09/08  11:25:56  dbg
 * 	MACH_KERNEL: remove non-MACH data types.
 * 	[89/07/11            dbg]
 * 
 * Revision 2.2  89/08/31  16:19:04  rwd
 * 	First Checkin
 * 	[89/08/23  15:41:50  rwd]
 * 
 * Revision 2.10  89/03/09  20:13:03  rpd
 * 	More cleanup.
 * 
 * Revision 2.9  89/02/25  18:04:45  gm0w
 * 	Kernel code cleanup.	
 * 	Put entire file under #ifdef KERNEL
 * 	[89/02/15            mrt]
 * 
 * Revision 2.8  89/02/07  01:01:53  mwyoung
 * Relocated from sys/kalloc.h
 * 
 * Revision 2.7  89/01/18  02:10:51  jsb
 * 	Fixed log.
 * 	[88/01/18            rpd]
 * 
 * Revision 2.2  89/01/18  01:16:25  jsb
 * 	Use MINSIZE of 16 instead of 64 (mostly for afs);
 * 	eliminate NQUEUES (see kalloc.c).
 * 	[89/01/13            jsb]
 *
 * 26-Oct-87 Peter King (king) at NeXT, Inc.
 *	Created.
 */ 
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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

#ifndef	_KERN_RTALLOC_H_
#define _KERN_RTALLOC_H_

#include <mach/machine/vm_types.h>

#define RTALLOC_MINSIZE		16

#define RT_ZONE_MAX_1		0
#define RT_ZONE_MAX_2		0
#define RT_ZONE_MAX_4		0
#define RT_ZONE_MAX_8		0
#define RT_ZONE_MAX_16		8
#define RT_ZONE_MAX_32		8
#define RT_ZONE_MAX_64		8
#define RT_ZONE_MAX_128		8
#define RT_ZONE_MAX_256		8
#define RT_ZONE_MAX_512		8
#define RT_ZONE_MAX_1024	8
#define RT_ZONE_MAX_2048        8
#define RT_ZONE_MAX_4096	8
#define RT_ZONE_MAX_8192	8
#define RT_ZONE_MAX_16384	0
#define RT_ZONE_MAX_32768	0

/*
 * Zones start with RTALLOC_MINSIZE
 */
#define RT_ZONE_SIZE		\
		(RT_ZONE_MAX_16		* 16 + \
		 RT_ZONE_MAX_32		* 32 + \
		 RT_ZONE_MAX_64		* 64 + \
		 RT_ZONE_MAX_128	* 128 + \
		 RT_ZONE_MAX_256	* 256 + \
		 RT_ZONE_MAX_512	* 512 + \
		 RT_ZONE_MAX_1024	* 1024 + \
		 RT_ZONE_MAX_2048	* 2048 + \
		 RT_ZONE_MAX_4096	* 4096 + \
		 RT_ZONE_MAX_8192	* 8192)

extern vm_offset_t	rtalloc(
				vm_size_t	size);

extern vm_offset_t	rtget(
				vm_size_t	size);

extern void		rtfree(
				vm_offset_t	data,
				vm_size_t	size);

extern void		rtalloc_init(
				void);

#endif	/* _KERN_RTALLOC_H_ */
