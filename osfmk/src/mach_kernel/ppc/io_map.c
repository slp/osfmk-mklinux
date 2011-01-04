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
 * 
 */
/*
 * MkLinux
 */

#include <mach/vm_param.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <ppc/pmap.h>
#include <ppc/io_map_entries.h>

extern vm_offset_t	virtual_avail;

/*
 * Allocate and map memory for devices that may need to be mapped 
 * outside the usual physical memory. If phys_addr is NULL then
 * steal the appropriate number of physical pages from the vm
 * system and map them.
 */
vm_offset_t
io_map(phys_addr, size)
	vm_offset_t	phys_addr;
	vm_size_t	size;
{
	vm_offset_t	start;
	int		i;
	vm_page_t 	m;

	assert (kernel_map != VM_MAP_NULL);	/* VM must be initialised */

	if (phys_addr != 0) {
		/* make sure we map full contents of all the pages concerned */
		size = round_page(size + (phys_addr & PAGE_MASK));

		/* Steal some free virtual addresses */
		(void) kmem_alloc_pageable(kernel_map, &start, size);
	
		(void) pmap_map_bd(start,
				   trunc_page(phys_addr),
				   trunc_page(phys_addr) + size,
				   VM_PROT_READ|VM_PROT_WRITE);

		return (start + (phys_addr & PAGE_MASK));
	} else {
		/* Steal some free virtual addresses */
		(void) kmem_alloc_pageable(kernel_map, &start, size);

		/* Steal some physical pages and map them one by one */
		for (i = 0; i < size ; i += PAGE_SIZE) {
			m = VM_PAGE_NULL;
			while ((m = vm_page_grab()) == VM_PAGE_NULL)
				VM_PAGE_WAIT();
			vm_page_gobble(m);
			(void) pmap_map_bd(start + i,
					   m->phys_addr,
					   m->phys_addr + PAGE_SIZE,
					   VM_PROT_READ|VM_PROT_WRITE);
		}
		return start;
	}
}
