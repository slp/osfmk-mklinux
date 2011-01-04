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
 * Revision 2.5  91/05/14  17:04:15  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:38:17  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:29:40  mrt]
 * 
 * Revision 2.3  90/06/19  23:00:29  rpd
 * 	Added zi_ prefix to zone_info field names.
 * 	Added zi_collectable field to zone_info.
 * 	Added zn_ prefix to zone_name field names.
 * 	[90/06/05            rpd]
 * 
 * Revision 2.2  90/06/02  15:00:54  rpd
 * 	Created.
 * 	[90/03/26  23:53:57  rpd]
 * 
 * Revision 2.2  89/05/06  12:36:08  rpd
 * 	Created.
 * 	[89/05/06  12:35:19  rpd]
 * 
 */ 
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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

#ifndef	_MACH_DEBUG_ZONE_INFO_H_
#define _MACH_DEBUG_ZONE_INFO_H_

#include <mach/boolean.h>
#include <mach/machine/vm_types.h>

/*
 *	Remember to update the mig type definitions
 *	in mach_debug_types.defs when adding/removing fields.
 */

#define ZONE_NAME_MAX_LEN		80

typedef struct zone_name {
	char		zn_name[ZONE_NAME_MAX_LEN];
} zone_name_t;

typedef zone_name_t *zone_name_array_t;


typedef struct zone_info {
	integer_t	zi_count;	/* Number of elements used now */
	vm_size_t	zi_cur_size;	/* current memory utilization */
	vm_size_t	zi_max_size;	/* how large can this zone grow */
	vm_size_t	zi_elem_size;	/* size of an element */
	vm_size_t	zi_alloc_size;	/* size used for more memory */
	integer_t	zi_pageable;	/* zone pageable? */
	integer_t	zi_sleepable;	/* sleep if empty? */
	integer_t	zi_exhaustible;	/* merely return if empty? */
	integer_t	zi_collectable;	/* garbage collect elements? */
} zone_info_t;

typedef zone_info_t *zone_info_array_t;

#endif	/* _MACH_DEBUG_ZONE_INFO_H_ */
