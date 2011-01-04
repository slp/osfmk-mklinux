/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_FREE_AREA_PAGER_H_
#define _OSFMACH3_FREE_AREA_PAGER_H_

#include <mach/mach_port.h>
#include <mach/memory_object.h>

#define FREE_AREA_PAGER_MAX_CLUSTER_SIZE	8	/* 8 pages */
#define FREE_AREA_PAGER_MESSAGE_SIZE	8192

#define FAP_PRESENT	0x1	/* page is mapped in the microkernel */
#define FAP_DIRTY	0x2	/* page is dirty (need to be zero-filled) */
#define FAP_IN_USE	0x4	/* page is currently allocated */
#define FAP_DISCARD	0x8	/* page has been "discard_request()ed" */

extern unsigned long	__mem_base;
extern vm_size_t	__mem_size;		/* emulated memory size */
extern vm_size_t	osfmach3_mem_size;	/* real memory size */

extern unsigned long	free_area_pager_init(vm_size_t size,
					     unsigned long mask);
extern void		free_area_discard_pages(unsigned long addr,
						unsigned long size);
extern void		free_area_mark_used(unsigned long addr,
					    unsigned long size);
extern boolean_t	free_area_page_dirty(unsigned long addr);

#endif	/* _OSFMACH3_FREE_AREA_PAGER_H_ */
