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
 * Revision 2.3  91/06/25  10:33:21  rpd
 * 	Changed memory_object_t to ipc_port_t where appropriate.
 * 	[91/05/28            rpd]
 * 
 * Revision 2.2  91/05/18  14:39:45  rpd
 * 	Created.
 * 	[91/03/22            rpd]
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

#ifndef	_VM_MEMORY_OBJECT_H_
#define	_VM_MEMORY_OBJECT_H_

#include <norma_vm.h>
#include <mach_pagemap.h>

#include <mach/boolean.h>
#include <ipc/ipc_types.h>

extern kern_return_t	memory_object_set_attributes_common(
				vm_object_t	object,
				boolean_t	may_cache,
				memory_object_copy_strategy_t copy_strategy,
				boolean_t	temporary,
#if	NORMA_VM && MACH_PAGEMAP
				vm_external_map_t existence_map,
				vm_offset_t	existence_size,
#endif	/* NORMA_VM && MACH_PAGEMAP */
				vm_size_t	cluster_size,
				boolean_t	silent_overwrite,
				boolean_t	advisory_pageout);

extern boolean_t	memory_object_sync (
				vm_object_t	object,
				vm_offset_t	offset,
				vm_size_t	size,
				boolean_t	should_flush,
				boolean_t	should_return);

extern ipc_port_t	memory_manager_default_reference(
					vm_size_t	*cluster_size);

extern boolean_t	memory_manager_default_port(ipc_port_t port);

extern kern_return_t	memory_manager_default_check(void);

extern void		memory_manager_default_init(void);

#endif	/* _VM_MEMORY_OBJECT_H_ */
