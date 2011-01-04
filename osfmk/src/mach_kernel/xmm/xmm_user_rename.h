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
 * Revision 2.3.2.1  92/06/24  18:03:45  jeffreyh
 * 	Add create, data_initialize.
 * 	[92/06/08            dlb]
 * 
 * Revision 2.3  91/08/28  11:16:32  jsb
 * 	Added supply_completed, data_return, change_completed.
 * 	[91/08/16  10:47:04  jsb]
 * 
 * Revision 2.2  91/06/17  15:48:51  jsb
 * 	First checkin.
 * 	[91/06/17  11:08:51  jsb]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 */
/*
 *	File:	norma/xmm_user_rename.h
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Renames to interpose on memory_object.defs user side.
 */

#include <norma_vm.h>

#if	NORMA_VM
#define	memory_object_init		k_memory_object_init
#define	memory_object_create		k_memory_object_create
#define	memory_object_terminate		k_memory_object_terminate
#define	memory_object_data_request	k_memory_object_data_request
#define	memory_object_data_unlock	k_memory_object_data_unlock
#define	memory_object_data_write	k_memory_object_data_write
#define memory_object_copy		k_memory_object_copy
#define	memory_object_lock_completed	k_memory_object_lock_completed
#define	memory_object_supply_completed	k_memory_object_supply_completed
#define	memory_object_data_return	k_memory_object_data_return
#define	memory_object_change_completed	k_memory_object_change_completed
#define	memory_object_synchronize	k_memory_object_synchronize
#define	memory_object_data_initialize	k_memory_object_data_initialize
#endif	/* NORMA_VM */
