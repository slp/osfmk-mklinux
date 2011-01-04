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
 * Revision 2.3.2.1  92/02/21  11:28:06  jsb
 * 	Removed rename of cover and backward compatibility routines.
 * 	[92/02/09  13:56:07  jsb]
 * 
 * Revision 2.3  91/08/28  11:16:28  jsb
 * 	Added xxx_memory_object_lock_request, memory_object_data_supply,
 * 	memory_object_ready, and memory_object_change_attributes.
 * 	[91/08/16  14:32:26  jsb]
 * 
 * Revision 2.2  91/06/17  15:48:38  jsb
 * 	First checkin.
 * 	[91/06/17  11:09:08  jsb]
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
 *	File:	norma/xmm_server_rename.h
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Renames to interpose on memory_object.defs server side.
 */

#include <norma_vm.h>

#if	NORMA_VM
#define	memory_object_data_unavailable	k_memory_object_data_unavailable
#define	memory_object_get_attributes	k_memory_object_get_attributes
#define	memory_object_lock_request	k_memory_object_lock_request
#define	memory_object_data_error	k_memory_object_data_error
#define	memory_object_destroy		k_memory_object_destroy
#define	memory_object_data_supply	k_memory_object_data_supply
#define	memory_object_synchronize_completed \
					k_memory_object_synchronize_completed

extern kern_return_t	k_memory_object_data_unavailable(
				vm_object_t		object,
				vm_offset_t		offset,
				vm_size_t		size);

extern kern_return_t	k_memory_object_get_attributes(
        			vm_object_t     	object,
        			memory_object_flavor_t 	flavor,
				memory_object_info_t	attributes,
				mach_msg_type_number_t	*count);

extern kern_return_t	k_memory_object_lock_request(
				vm_object_t		object,
				vm_offset_t		offset,
				vm_size_t		size,
				memory_object_return_t	should_return,
				boolean_t		should_flush,
				vm_prot_t		prot,
				ipc_port_t		reply_to,
				mach_msg_type_name_t	reply_to_type);

extern kern_return_t	k_memory_object_data_error(
				vm_object_t		object,
				vm_offset_t		offset,
				vm_size_t		size,
				kern_return_t		error_value);

extern kern_return_t	k_memory_object_destroy(
				vm_object_t		object,
				kern_return_t		reason);

extern kern_return_t	k_memory_object_data_supply(
				vm_object_t		object,
				vm_offset_t		offset,
				vm_offset_t		data,
				mach_msg_type_number_t	data_cnt,
				vm_prot_t		lock_value,
				boolean_t		precious,
				ipc_port_t		reply_to,
				mach_msg_type_name_t	reply_to_type);

extern kern_return_t	k_memory_object_synchronize_completed(
				vm_object_t		object,
				vm_offset_t		offset,
				vm_offset_t		length);

extern kern_return_t	memory_object_uncaching_permitted(
				vm_object_t	object);

extern kern_return_t	memory_object_data_write(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj,
				vm_offset_t		offset,
				vm_map_copy_t		data,
				vm_size_t		length);

extern kern_return_t	memory_object_data_initialize(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj,
				vm_offset_t		offset,
				vm_map_copy_t		data,
				vm_size_t		length);

extern kern_return_t	memory_object_supply_completed(
				ipc_port_t		reply_to,
				mach_msg_type_name_t	reply_to_type,
				xmm_obj_t		mobj,
				vm_offset_t		offset,
				vm_size_t		length,
				kern_return_t		result,
				vm_offset_t		error_offset);

extern kern_return_t	memory_object_create_copy(
				vm_object_t		src_object,
				ipc_port_t		new_copy_pager,
				kern_return_t		*result);

extern kern_return_t	memory_object_data_return(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj,
				vm_offset_t		offset,
				vm_map_copy_t		data,
				vm_size_t		length,
				boolean_t		dirty,
				boolean_t		kernel_copy);

extern kern_return_t	memory_object_lock_completed(
				ipc_port_t		reply_to,
				mach_msg_type_name_t	reply_to_type,
				xmm_obj_t		mobj,
				vm_offset_t		offset,
				vm_size_t		length);

extern kern_return_t	memory_object_data_unlock(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj,
				vm_offset_t		offset,
				vm_size_t		length,
				vm_prot_t		desired_access);

extern kern_return_t	memory_object_data_request(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj,
				vm_offset_t		offset,
				vm_size_t		length,
				vm_prot_t		desired_access);

extern kern_return_t	memory_object_share(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj);

extern kern_return_t	memory_object_caching(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj);

extern kern_return_t	memory_object_terminate(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj);

extern kern_return_t	memory_object_copy(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj);

extern kern_return_t	memory_object_declare_page(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj,
				vm_offset_t		offset,
				vm_size_t		size);

extern kern_return_t	memory_object_declare_pages(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj,
				vm_external_map_t	existence_map,
				vm_offset_t		existence_size,
				boolean_t		frozen);

extern kern_return_t	memory_object_uncaching(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj);

extern kern_return_t	memory_object_freeze(
				ipc_port_t		memory_object,
				xmm_obj_t		mobj,
				vm_external_map_t	existence_map,
				vm_offset_t		existence_size);

extern kern_return_t	memory_object_synchronize(
				ipc_port_t		memory_object,
				xmm_obj_t 		mobj,
				vm_offset_t		offset,
				vm_size_t		length,
				vm_sync_t		sync_flags);

#endif	/* NORMA_VM */
