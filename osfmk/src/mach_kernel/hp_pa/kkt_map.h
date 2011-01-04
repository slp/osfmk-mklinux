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

/*
 * Values of virtual address status.
 */
#define FREE_VADDR 0
#define USED_VADDR 1
#define MAP_VADDR  2

#define KKT_VIRT_SIZE 1024

extern void 			kkt_map_init(void);

extern vm_offset_t 		kkt_find_mapping(vm_page_t, vm_offset_t,boolean_t);

extern void 			kkt_do_mappings (void);

extern void 			kkt_unmap(vm_offset_t);

#define KKT_MAP_INIT()			   kkt_map_init()
#define KKT_FIND_MAPPING(vm_page, vm_offset, can_block) 			\
			kkt_find_mapping((vm_page), (vm_offset), (can_block))
#define KKT_DO_MAPPINGS()		   kkt_do_mappings()
#define KKT_UNMAP(vaddr)		   kkt_unmap((vaddr))
