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
 * Copyright (c) 1990,1993,1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: pmap.h 1.24 94/12/14$
 *	Author: Mike Hibler, Bob Wheeler, University of Utah CSL, 9/90
 */

/*
 *	Pmap header for hppa.
 */

#ifndef	_HP700_PMAP_H_
#define	_HP700_PMAP_H_

#ifndef ASSEMBLER

#define EQUIV_HACK	/* no multiple mapping of kernel equiv space allowed */

#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <kern/queue.h>

#endif /* ASSEMBLER */

#define BTLB		/* Use block TLBs: PA 1.1 and above */
#define HPT		/* Hashed (Hardware) Page Table */
#define USEALIGNMENT	/* Take advantage of cache alignment for optimization */

/*
 * Virtual to physical mapping macros/structures.
 * IMPORTANT NOTE: there is one mapping per HW page, not per MACH page.
 */

#define HP700_HASHSIZE		4096	/* size of hash table */
#define HP700_HASHSIZE_LOG2	12

#ifndef ASSEMBLER

/*
 * This hash function is the one used by the hardware TLB walker on the 7100.
 */
#define pmap_hash(space, offset) \
	(((unsigned) (space) << 5 ^ \
	  (unsigned) (offset) >> HP700_PGSHIFT) & \
	 (HP700_HASHSIZE-1))

struct pmap {
	decl_simple_lock_data(,lock)	/* lock on map */
	int			ref_count;   /* reference count */
	space_t			space;	     /* space for this pmap */
	int			pid;	     /* protection id for pmap */
	struct pmap		*next;	     /* linked list of free pmaps */
	struct pmap_statistics	stats;	     /* statistics */
	queue_head_t		pmap_link;   /* hashed list of pmaps */
};

typedef struct pmap *pmap_t;

#define PMAP_NULL  ((pmap_t) 0)

extern pmap_t	kernel_pmap;			/* The kernel's map */

struct vtop_entry {
	queue_head_t	hash_link;	/* head of vtop chain */
};
#define vtop_next	hash_link.next
#define vtop_prev	hash_link.prev

#ifdef 	HPT
/*
 * If HPT is defined, we cache the last miss for each bucket using a
 * structure defined for the 7100 hardware TLB walker. On non-7100s, this
 * acts as a software cache that cuts down on the number of times we have
 * to search the vtop chain. (thereby reducing the number of instructions
 * and cache misses incurred during the TLB miss).
 *
 * The vtop_entry pointer is the address of the associated vtop table entry.
 * This avoids having to reform the address into the new table on a cache
 * miss.
 */
struct hpt_entry {
	unsigned	valid:1,	/* Valid bit */
			vpn:15,		/* Virtual Page Number */
			space:16;	/* Space ID */
	unsigned	tlbprot;	/* prot/access rights (for TLB load) */
	unsigned	tlbpage;	/* physical page (for TLB load) */
	unsigned	vtop_entry;	/* Pointer to associated VTOP entry */
};
#endif

#endif /* ASSEMBLER */

#define HPT_SHIFT	27		/* 16 byte entry (31-4) */
#define VTOP_SHIFT	28		/* 8  byte entry (31-3) */
#define HPT_LEN		HP700_HASHSIZE_LOG2
#define VTOP_LEN	HP700_HASHSIZE_LOG2

#define MAX_PID		0xfffa
#define	HP700_SID_KERNEL  0
#define	HP700_PID_KERNEL  2

#define KERNEL_ACCESS_ID 1

#define KERNEL_TEXT_PROT (TLB_AR_KRX | (KERNEL_ACCESS_ID << 1))
#define KERNEL_DATA_PROT (TLB_AR_KRW | (KERNEL_ACCESS_ID << 1))

/* Block TLB flags */
#define BLK_ICACHE	0
#define BLK_DCACHE	1
#define BLK_COMBINED	2
#define BLK_LCOMBINED	3

#define	PMAP_SWITCH_USER(th, map, my_cpu) th->map = map;	

#define PMAP_ACTIVATE(pmap, th, cpu)
#define PMAP_DEACTIVATE(pmap, th, cpu)
#define PMAP_CONTEXT(pmap,th)

#define pmap_kernel_va(VA)	\
	(((VA) >= VM_MIN_KERNEL_ADDRESS) && ((VA) <= VM_MAX_KERNEL_ADDRESS))

#define pmap_kernel()			(kernel_pmap)
#define	pmap_resident_count(pmap)	((pmap)->stats.resident_count)
#define pmap_remove_attributes(pmap,start,end)
#define pmap_copy(dpmap,spmap,da,len,sa)
#define	pmap_update()

#define pmap_phys_address(x)	((x) << HP700_PGSHIFT)
#define pmap_phys_to_frame(x)	((x) >> HP700_PGSHIFT)

#define cache_align(x)		(((x) + 64) & ~(64 - 1))

#ifndef ASSEMBLER

/* 
 * prototypes.
 */
extern void
hp700_protection_init(void);

extern vm_offset_t	kvtophys(vm_offset_t addr);

extern vm_offset_t pmap_map(
			    vm_offset_t va,
			    vm_offset_t spa,
			    vm_offset_t epa,
			    vm_prot_t prot);

extern void
pmap_bootstrap(vm_offset_t *avail_start, vm_offset_t *avail_end);

extern void
pmap_block_map(
	       vm_offset_t pa,
	       vm_size_t size,
	       vm_prot_t prot,
	       int entry, 
	       int dtlb);

extern void pmap_switch(pmap_t);

extern vm_offset_t pmap_extract(
	pmap_t pmap,
	vm_offset_t va);

extern int 
log2(unsigned value);
extern void insert_block_dtlb(
			      int, 
			      vm_offset_t, 
			      vm_size_t, 
			      unsigned);

extern void insert_block_itlb(
			      int, 
			      vm_offset_t, 
			      vm_size_t, 
			      unsigned);

extern void insert_block_ctlb(
			      int, 
			      vm_offset_t, 
			      vm_size_t, 
			      unsigned);

extern void insert_L_block_ctlb(
			      int, 
			      vm_offset_t, 
			      vm_size_t, 
			      unsigned);

extern void
pmap_remove_all(vm_offset_t pa);

extern boolean_t
pmap_verify_free(vm_offset_t pa);

extern void pitlbentry(vm_offset_t, vm_offset_t);
extern void pdtlbentry(vm_offset_t, vm_offset_t);
extern void pdtlb(vm_offset_t, vm_offset_t);
extern void pitlb(vm_offset_t, vm_offset_t);
extern void purge_block_dtlb(int);
extern void purge_block_itlb(int);
extern void purge_L_block_ctlb(int);
extern void purge_block_ctlb(int);
extern void ficache(space_t, vm_offset_t, unsigned);
extern void fdcache(space_t, vm_offset_t, unsigned);
extern void pdcache(space_t, vm_offset_t, unsigned);
extern void lbzero(space_t, vm_offset_t, vm_size_t);
extern void lbcopy(space_t, vm_offset_t, space_t, vm_offset_t, vm_size_t);
extern boolean_t pmap_iomap(vm_offset_t);
extern void pmap_iounmap(vm_offset_t);
extern vm_offset_t ltor(space_t, vm_offset_t);
extern void fcacheline(space_t, vm_offset_t);
extern void pcacheline(space_t, vm_offset_t);

#endif /* ASSEMBLER */

#endif /* _HP700_PMAP_H_ */





