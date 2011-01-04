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
 * Revision 2.5.4.2  92/04/08  15:45:00  jeffreyh
 * 	Back out Mainline changes. Revert back to revision 2.5.
 * 	[92/04/07  10:29:40  jeffreyh]
 * 
 * Revision 2.5  91/05/14  17:03:28  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:37:50  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:28:30  mrt]
 * 
 * Revision 2.3  91/01/08  15:19:05  rpd
 * 	Moved ipc_info_bucket_t to mach_debug/hash_info.h.
 * 	[91/01/02            rpd]
 * 
 * Revision 2.2  90/06/02  15:00:28  rpd
 * 	Created for new IPC.
 * 	[90/03/26  23:45:14  rpd]
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
 *	File:	mach_debug/ipc_info.h
 *	Author:	Rich Draves
 *	Date:	March, 1990
 *
 *	Definitions for the IPC debugging interface.
 */

#ifndef	_MACH_DEBUG_IPC_INFO_H_
#define _MACH_DEBUG_IPC_INFO_H_

#include <mach/boolean.h>
#include <mach/port.h>
#include <mach/machine/vm_types.h>

/*
 *	Remember to update the mig type definitions
 *	in mach_debug_types.defs when adding/removing fields.
 */


typedef struct ipc_info_space {
	natural_t iis_genno_mask;	/* generation number mask */
	natural_t iis_table_size;	/* size of table */
	natural_t iis_table_next;	/* next possible size of table */
	natural_t iis_tree_size;	/* size of tree */
	natural_t iis_tree_small;	/* # of small entries in tree */
	natural_t iis_tree_hash;	/* # of hashed entries in tree */
} ipc_info_space_t;


typedef struct ipc_info_name {
	mach_port_t iin_name;		/* port name, including gen number */
/*boolean_t*/integer_t iin_collision;	/* collision at this entry? */
	mach_port_type_t iin_type;	/* straight port type */
	mach_port_urefs_t iin_urefs;	/* user-references */
	vm_offset_t iin_object;		/* object pointer */
	natural_t iin_next;		/* marequest/next in free list */
	natural_t iin_hash;		/* hash index */
} ipc_info_name_t;

typedef ipc_info_name_t *ipc_info_name_array_t;


typedef struct ipc_info_tree_name {
	ipc_info_name_t iitn_name;
	mach_port_t iitn_lchild;	/* name of left child */
	mach_port_t iitn_rchild;	/* name of right child */
} ipc_info_tree_name_t;

typedef ipc_info_tree_name_t *ipc_info_tree_name_array_t;

#endif	/* _MACH_DEBUG_IPC_INFO_H_ */
