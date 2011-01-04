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

#ifndef VM_PRINT_H
#define	VM_PRINT_H

#include <vm/vm_map.h>

extern void	vm_map_print(
			vm_map_t	map);

extern void	vm_map_copy_print(
			vm_map_copy_t	copy);

#include <vm/vm_object.h>

extern int	vm_follow_object(
			vm_object_t	object);

extern void	vm_object_print(
			vm_object_t	object,
			boolean_t	have_addr,
			int		arg_count,
			char		*modif);

#include <vm/vm_page.h>

extern void	vm_page_print(
			vm_page_t	p);

#include <mach_pagemap.h>
#if	MACH_PAGEMAP
#include <vm/vm_external.h>
extern void vm_external_print(
			vm_external_map_t	map,
			vm_size_t		size);
#endif	/* MACH_PAGEMAP */

extern void	db_vm(void);

extern vm_size_t db_vm_map_total_size(
			vm_map_t	map);

#endif	/* VM_PRINT_H */
