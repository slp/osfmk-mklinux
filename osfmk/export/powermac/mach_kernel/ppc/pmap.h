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
/*
 * Copyright (c) 1990 The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: pmap.h 1.13 91/09/25$
 *	Author: Mike Hibler, Bob Wheeler, University of Utah CSS, 9/90
 */

/*
 *	Pmap header for ppc
 */

#ifndef	_PPC_PMAP_H_
#define	_PPC_PMAP_H_

#include <mach/vm_types.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <kern/queue.h>

/* The size of the hash table and mapping entries and the size
 * of the VM needed by the linux server is proportional
 * to the amount of physical memory. The hash table and mapping
 * entries must be allocated in contiguous physical memory.
 * Currently, the limits are set up so that around 100M is the
 * ceiling (found by experimentation on the mailing list)
 * - can be overridden via command line argument -mXXX (in megs)
 */

#define MAX_SUPPORTED_PHYSICAL_MEMORY	(90*1024*1024)

struct pmap {
	queue_head_t		pmap_link;   /* MUST BE FIRST */
	decl_simple_lock_data(,lock)	     /* lock on map */
	int			ref_count;   /* reference count */
	space_t			space;	     /* space for this pmap */
	struct pmap_statistics	stats;	     /* statistics */
};

typedef struct pmap *pmap_t;

#define PMAP_NULL  ((pmap_t) 0)

extern pmap_t	kernel_pmap;			/* The kernel's map */

#define	PMAP_SWITCH_USER(th, map, my_cpu) th->map = map;	

#define PMAP_ACTIVATE(pmap, th, cpu)
#define PMAP_DEACTIVATE(pmap, th, cpu)
#define PMAP_CONTEXT(pmap,th)

#define pmap_kernel_va(VA)	\
	(((VA) >= VM_MIN_KERNEL_ADDRESS) && ((VA) <= VM_MAX_KERNEL_ADDRESS))

#define	PPC_SID_KERNEL  0       /* Must change KERNEL_SEG_REG0_VALUE if !0 */
#define SID_MAX	((1<<20) - 1)	/* Space ID=20 bits, segment_id=SID + 4 bits */
#define PPC_SID_PRIME 356299	/* generate non-repetative "different" sids */
#define PPC_SID_MASK  0xfffff   /* within a 20-bit field */

#define pmap_kernel()			(kernel_pmap)
#define	pmap_resident_count(pmap)	((pmap)->stats.resident_count)
#define pmap_remove_attributes(pmap,start,end)
#define pmap_copy(dpmap,spmap,da,len,sa)
#define	pmap_update()

#define pmap_phys_address(x)	((x) << PPC_PGSHIFT)
#define pmap_phys_to_frame(x)	((x) >> PPC_PGSHIFT)

/* 
 * prototypes.
 */
extern void 		ppc_protection_init(void);
extern vm_offset_t	kvtophys(vm_offset_t addr);
extern vm_offset_t	phystokv(vm_offset_t addr);
extern vm_offset_t	pmap_map(vm_offset_t va,
				 vm_offset_t spa,
				 vm_offset_t epa,
				 vm_prot_t prot);
extern kern_return_t    pmap_add_physical_memory(vm_offset_t spa,
						 vm_offset_t epa,
						 boolean_t available,
						 unsigned int attr);
extern vm_offset_t	pmap_map_bd(vm_offset_t va,
				    vm_offset_t spa,
				    vm_offset_t epa,
				    vm_prot_t prot);
extern void		pmap_bootstrap(unsigned int mem_size,
				       vm_offset_t *first_avail);
extern void		pmap_block_map(vm_offset_t pa,
				       vm_size_t size,
				       vm_prot_t prot,
				       int entry, 
				       int dtlb);
extern void		pmap_switch(pmap_t);

extern vm_offset_t pmap_extract(pmap_t pmap,
				vm_offset_t va);

extern void pmap_remove_all(vm_offset_t pa);

extern boolean_t pmap_verify_free(vm_offset_t pa);

extern void sync_cache(vm_offset_t pa, unsigned length);
extern void flush_dcache(vm_offset_t va, unsigned length, boolean_t phys);
extern void invalidate_dcache(vm_offset_t va, unsigned length, boolean_t phys);
extern void invalidate_icache(vm_offset_t va, unsigned length, boolean_t phys);
extern void invalidate_cache_for_io(vm_offset_t va, unsigned length, boolean_t phys);

#endif /* _PPC_PMAP_H_ */

