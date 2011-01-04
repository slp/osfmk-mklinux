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
 * Mach Operating System
 * Copyright (c) 1990,1991,1992 The University of Utah and
 * the Center for Software Science (CSS).
 * Copyright (c) 1991,1987 Carnegie Mellon University.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that all advertising materials mentioning features or use of
 * this software display the following acknowledgement: ``This product
 * includes software developed by the Center for Software Science at
 * the University of Utah.''
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * Carnegie Mellon requests users of this software to return to
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 * 	Utah $Hdr: pmap.c 1.28 92/06/23$
 *	Author: Mike Hibler, Bob Wheeler, University of Utah CSS, 10/90
 */
 
/*
 *	Manages physical address maps for powerpc.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information to
 *	when physical maps must be made correct.
 *	
 */

#include <cpus.h>
#include <debug.h>
#include <mach_kgdb.h>
#include <mach_vm_debug.h>
#include <db_machine_commands.h>

#include <kern/thread.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <kern/spl.h>

#include <kern/misc_protos.h>
#include <ppc/misc_protos.h>
#include <ppc/gdb_defs.h> /* for kgdb_kernel_in_pmap used in assert */
#include <ppc/proc_reg.h>
#include <ppc/mem.h>
#include <ppc/pmap.h>
#include <ppc/pmap_internals.h>
#include <ppc/new_screen.h>
#include <ppc/Firmware.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/video_board.h>
#include <ppc/POWERMAC/video_pdm.h>
#include <ddb/db_output.h>

#if	DB_MACHINE_COMMANDS
/* optionally enable traces of pmap operations in post-mortem trace table */
/* #define PMAP_LOWTRACE 1 */
#define PMAP_LOWTRACE 0
#else	/* DB_MACHINE_COMMANDS */
/* Can not trace even if we wanted to */
#define PMAP_LOWTRACE 0
#endif	/* DB_MACHINE_COMMANDS */

extern unsigned int	avail_remaining;

vm_offset_t		avail_next;
int          		current_free_region;	/* Used in pmap_next_page */

/* forward */
void pmap_activate(pmap_t pmap, thread_t th, int which_cpu);
void pmap_deactivate(pmap_t pmap, thread_t th, int which_cpu);
void copy_to_phys(vm_offset_t sva, vm_offset_t dpa, int bytecount);

/* routines to manipulate a cache of pmaps based on space id */

#define MAPPING_HAS_PMAP_PTR 1
#if MAPPING_HAS_PMAP_PTR == 0
static void pmap_stash_init(void);
static void pmap_add_pmap(pmap_t);
static void pmap_rem_pmap(pmap_t);
static pmap_t pmap_get_pmap(struct mapping *);
#else
#define pmap_stash_init()
#define pmap_add_pmap(pmap)
#define pmap_rem_pmap(pmap)
#define pmap_get_pmap(mapping) (mapping->pmap)
#endif /* MAPPING_HAS_PMAP_PTR */
/*
 * Maintain a table of pmap structures so that we can map from a space
 * id to the associated pmap (for the benefit of pmap_remove_all()).
 *
 * NOTE: Originally, there was a pointer in the mapping structure that
 *       could be used to get to the pmap. However, that pointer made
 *	 the mapping structure large, and there are lots of these.
 *	 Hence this seemingly gratuatious code borrowed from HP
 */
#define	PMAP_STASH_SIZE		32
#define pmap_stash_hash(space)  ((space) % PMAP_STASH_SIZE)

queue_head_t pmap_stash[PMAP_STASH_SIZE];


#if MACH_VM_DEBUG
int pmap_list_resident_pages(pmap_t pmap, vm_offset_t *listp, int space);
#endif

#if DEBUG
#define PDB_USER	0x01	/* exported functions */
#define PDB_MAPPING	0x02	/* low-level mapping routines */
#define PDB_ENTER	0x04	/* pmap_enter specifics */
#define PDB_COPY	0x08	/* copy page debugging */
#define PDB_ZERO	0x10	/* zero page debugging */
#define PDB_WIRED	0x20	/* things concerning wired entries */
#define PDB_PTEG	0x40	/* PTEG overflows */
#define PDB_LOCK	0x100	/* locks */
#define PDB_IO		0x200	/* Improper use of WIMG_IO checks - PCI machines */

int pmdebug = PDB_PTEG | PDB_IO;

#define PCI_BASE	0x80000000
#endif

struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;
struct zone	*pmap_zone;		/* zone of pmap structures */
boolean_t	pmap_initialized = FALSE;

/*
 * Physical-to-virtual translations are handled by inverted page table
 * structures, phys_tables.  Multiple mappings of a single page are handled
 * by linking the affected mapping structures. We initialise one region
 * for phys_tables of the physical memory we know about, but more may be
 * added as it is discovered (eg. by drivers).
 */
struct phys_entry *phys_table;	/* For debugging */

#if	NCPUS > 1 || MACH_LDEBUG
lock_t	pmap_system_lock;

decl_simple_lock_data(,tlb_system_lock)

decl_simple_lock_data(,physentries_lock)

/*
 * Each entry in the hashed page table is locked by a bit
 * in the pte_lock_table.
 * The lock bits are accessed by the index in the hash_table.
 * The lock bit array is protected by the "pte_lock_table_lock".
 */
char *pte_lock_table;		/* pointer to array of bits */
#define pte_lock_table_size(n)	(((n)+BYTE_SIZE-1)/BYTE_SIZE)
decl_simple_lock_data(,pte_lock_table_lock)
decl_simple_lock_data(,pteg_overflow_lock)

#if	MACH_LDEBUG
decl_simple_lock_data(, fake_physentry_lock[NCPUS])
unsigned int	fake_physentry_lock_count[NCPUS];
decl_simple_lock_data(, fake_pte_lock[NCPUS])
unsigned int	fake_pte_lock_count[NCPUS];
#endif	/* MACH_LDEBUG */

#else	/* NCPUS > 1 || MACH_LDEBUG */

#define pte_lock_table_size(n)	(0)
char *pte_lock_table;		/* not used, actually ... */

#endif	/* NCPUS > 1 || MACH_LDEBUG */

/*
 *	free pmap list. caches the first free_pmap_max pmaps that are freed up
 */
int		free_pmap_max = 32;
int		free_pmap_count;
queue_head_t	free_pmap_list;
decl_simple_lock_data(,free_pmap_lock)

struct mapping *mapping_base, *mapping_last;

unsigned	prot_bits[8];

int		sid_counter=PPC_SID_KERNEL; /* seed for SID, set to kernel */
decl_simple_lock_data(,sid_counter_lock)

/*
 * Function to get index into phys_table for a given physical address
 */

struct phys_entry *pmap_find_physentry(vm_offset_t pa)
{
	int i;
	struct phys_entry *entry;

	for (i = pmap_mem_regions_count-1; i >= 0; i--) {
		if (pa < pmap_mem_regions[i].start)
			continue;
		if (pa >= pmap_mem_regions[i].end)
			return PHYS_NULL;
		
		entry = &pmap_mem_regions[i].phys_table[(pa - pmap_mem_regions[i].start) >> PPC_PGSHIFT];
		return entry;
	}
	printf("NMGS DEBUG : pmap_find_physentry 0x%08x out of range\n",pa);
	return PHYS_NULL;
}

/*
 * kern_return_t
 * pmap_add_physical_memory(vm_offset_t spa, vm_offset_t epa,
 *                          boolean_t available, unsigned int attr)
 *	Allocate some extra physentries for the physical addresses given,
 *	specifying some default attribute that on the powerpc specifies
 *      the default cachability for any mappings using these addresses
 *	If the memory is marked as available, it is added to the general
 *	VM pool, otherwise it is not (it is reserved for card IO etc).
 */


kern_return_t pmap_add_physical_memory(vm_offset_t spa, vm_offset_t epa,
				       boolean_t available, unsigned int attr)
{
	int i,j;
	spl_t s;
	struct phys_entry *phys_table;

	/* Only map whole pages */

	spa = trunc_page(spa);
	epa = round_page(epa);

	/* First check that the region doesn't already exist */

	assert (epa >= spa);
	for (i = 0; i < pmap_mem_regions_count; i++) {
		/* If we're below the next region, then no conflict */
		if (epa < pmap_mem_regions[i].start)
			break;
		if (spa < pmap_mem_regions[i].end) {
#if DEBUG
			printf("pmap_add_physical_memory(0x%08x,0x%08x,0x%08x) - memory already present\n",spa,epa,attr);
#endif /* DEBUG */
			return KERN_NO_SPACE;
		}
	}

	/* Check that we've got enough space for another region */
	if (pmap_mem_regions_count == PMAP_MEM_REGION_MAX)
		return KERN_RESOURCE_SHORTAGE;

	/* Once here, i points to the mem_region above ours in physical mem */

	/* allocate a new phys_table for this new region */

	phys_table =  (struct phys_entry *)
		kalloc(sizeof(struct phys_entry) * atop(epa-spa));

	/* Initialise the new phys_table entries */
	for (j = 0; j < atop(epa-spa); j++) {
		phys_table[j].phys_link = MAPPING_NULL;
		/* We currently only support these two attributes */
		assert((attr == PTE_WIMG_DEFAULT) ||
		       (attr == PTE_WIMG_IO));
		phys_table[j].pte1.bits.wimg = attr;
		phys_table[j].locked = FALSE;
	}
	s = splvm();
	
	/* Move all the phys_table entries up some to make room in
	 * the ordered list.
	 */
	for (j = pmap_mem_regions_count; j > i ; j--)
		pmap_mem_regions[j] = pmap_mem_regions[j-1];

	/* Insert a new entry with some memory to back it */

	pmap_mem_regions[i].start 	     = spa;
	pmap_mem_regions[i].end           = epa;
	pmap_mem_regions[i].phys_table    = phys_table;

	pmap_mem_regions_count++;
	splx(s);

	if (available) {
		printf("warning : pmap_add_physical_mem() "
		       "available not yet supported\n");
	}

	return KERN_SUCCESS;
}

/*
 * ppc_protection_init()
 *	Initialise the user/kern_prot_codes[] arrays which are 
 *	used to translate machine independent protection codes
 *	to powerpc protection codes. The PowerPc can only provide
 *      [no rights, read-only, read-write]. Read implies execute.
 * See PowerPC 601 User's Manual Table 6.9
 */
void
ppc_protection_init(void)
{
	prot_bits[VM_PROT_NONE | VM_PROT_NONE  | VM_PROT_NONE]     = 0;
	prot_bits[VM_PROT_READ | VM_PROT_NONE  | VM_PROT_NONE]     = 3;
	prot_bits[VM_PROT_READ | VM_PROT_NONE  | VM_PROT_EXECUTE]  = 3;
	prot_bits[VM_PROT_NONE | VM_PROT_NONE  | VM_PROT_EXECUTE]  = 3;
	prot_bits[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE]     = 2;
	prot_bits[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE]     = 2;
	prot_bits[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE]  = 2;
	prot_bits[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE]  = 2;
}

/*
 * pmap_map(va, spa, epa, prot)
 *	is called during boot to map memory in the kernel's address map.
 *	A virtual address range starting at "va" is mapped to the physical
 *	address range "spa" to "epa" with machine independent protection
 *	"prot".
 *
 *	"va", "spa", and "epa" are byte addresses and must be on machine
 *	independent page boundaries.
 */
vm_offset_t
pmap_map(
	vm_offset_t va,
	vm_offset_t spa,
	vm_offset_t epa,
	vm_prot_t prot)
{

	if (spa == epa)
		return(va);

	assert(epa > spa);

	while (spa < epa) {
		pmap_enter(kernel_pmap, va, spa, prot, TRUE);

		va += PAGE_SIZE;
		spa += PAGE_SIZE;
	}
	return(va);
}

/*
 * pmap_map_bd(va, spa, epa, prot)
 *	Back-door routine for mapping kernel VM at initialisation.
 *	Used for mapping memory outside the known physical memory
 *      space, with caching disabled. Designed for use by device probes.
 * 
 *	A virtual address range starting at "va" is mapped to the physical
 *	address range "spa" to "epa" with machine independent protection
 *	"prot".
 *
 *	"va", "spa", and "epa" are byte addresses and must be on machine
 *	independent page boundaries.
 *
 * WARNING: The current version of memcpy() can use the dcbz instruction
 * on the destination addresses.  This will cause an alignment exception
 * and consequent overhead if the destination is caching-disabled.  So
 * avoid memcpy()ing into the memory mapped by this function.
 *
 * also, many other pmap_ routines will misbehave if you try and change
 * protections or remove these mappings, they are designed to be permanent.
 *
 * Locking:
 *	this routine is called only during system initialization when only
 *	one processor is active, so no need to take locks...
 */
vm_offset_t
pmap_map_bd(
	vm_offset_t va,
	vm_offset_t spa,
	vm_offset_t epa,
	vm_prot_t prot)
{
	spl_t	spl;
	pte_t  *pte;
	struct phys_entry* pp;
	struct mapping *mp;

	if (spa == epa)
		return(va);

	assert(epa > spa);

	spl = splvm();

	while (spa < epa) {
		
		pte = find_or_allocate_pte(PPC_SID_KERNEL, va, TRUE);
		/* the PTE is locked now (if not NULL) */

		assert(pte != PTE_NULL);

		/* remapping of already-mapped addresses is OK, we just
		 * trample on the PTE, but make sure we're mapped to same
		 * address
		 */
		if (pte->pte0.bits.valid) {
			assert(trunc_page(pte->pte1.word) == spa);
		}

		pte->pte0.bits.valid = FALSE;
		sync();

		simple_lock(&tlb_system_lock);
		tlbie(va);
		simple_unlock(&tlb_system_lock);
		eieio();
		TLBSYNC_SYNC();
		pte->pte1.bits.protection = prot_bits[prot];
		pte->pte1.bits.phys_page  = spa >> PPC_PGSHIFT;
		pte->pte1.bits.wimg	  = PTE_WIMG_IO;

		eieio();

		pte->pte0.bits.valid = TRUE;	/* Validate pte entry */
		sync();				/* Why the hell not? */


		/* If memory is mappable (is in phys table) then set
		 * default cach attributes to non-cached and verify
		 * that there aren't any non-cached mappings which would
		 * break the processor spec (and hang the machine).
		 */
		pp = pmap_find_physentry(spa);
		if (pp != PHYS_NULL) {
			/* LOCK_PHYSENTRY(pp); no need to lock here */
			/* Set the default mapping type */
			pp->pte1.bits.wimg = PTE_WIMG_IO;
#if DEBUG
			/* Belt and braces check on mappings */
			for (mp = pp->phys_link; mp != NULL; mp = mp->next) {
				/* LOCK_PTE((MP2PTE(mp)); no need to lock here */
				assert(MP2PTE(mp)->pte1.bits.wimg ==
				       PTE_WIMG_IO);
				/* UNLOCK_PTE(MP2PTE(mp)); */
			}
#endif /* DEBUG */
			/* UNLOCK_PHYSENTRY(pp); */
		}

		UNLOCK_PTE(pte);

		va += PAGE_SIZE;
		spa += PAGE_SIZE;
	}

	splx(spl);

	return(va);
}

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Map the kernel's code and data, and allocate the system page table.
 *	Called with mapping done by BATs. Page_size must already be set.
 *
 *	Parameters:
 *	first_avail	PA of address where we can allocate structures.
 */
void
pmap_bootstrap(unsigned int mem_size, vm_offset_t *first_avail)
{
	register struct mapping *mp;
	vm_offset_t addr;
	vm_size_t size;
	int i, num;
	unsigned int mask;
	vm_offset_t	first_used_addr;

	*first_avail = round_page(*first_avail);

	assert(PAGE_SIZE == PPC_PGBYTES);

	ppc_protection_init();
	pmap_stash_init();	/* simple hashtable for space->pmap lookup */

	/*
	 * Initialize kernel pmap
	 */
	kernel_pmap = &kernel_pmap_store;

#if	NCPUS > 1 || MACH_LDEBUG
	lock_init(&pmap_system_lock,
		  FALSE,		/* NOT a sleep lock */
		  ETAP_VM_PMAP_SYS,
		  ETAP_VM_PMAP_SYS_I);
#endif	/* NCPUS > 1 || MACH_LDEBUG */

	simple_lock_init(&kernel_pmap->lock, ETAP_VM_PMAP_KERNEL);

	kernel_pmap->ref_count = 1;
	kernel_pmap->space = PPC_SID_KERNEL;

	/*
	 * Allocate: (from first_avail up)
	 *      aligned to its own size:
         *        hash table (for mem size 2**x, allocate 2**(x-10) entries)
	 *      optional on devices being used, aligned to 256Kbyte boundary
	 *        dma buffer, of size 256K
	 * (if hash_table_size is > DMA_BUFFER_ALIGNMENT then we reserve
	 *  dma buffer first in order to keep alignment)
	 *	physical page entries (1 per physical page)
	 *	physical -> virtual mappings (for multi phys->virt mappings)
	 */
	/* hash_table_size must be a power of 2, recommended sizes are
	 * taken from PPC601 User Manual, table 6-19. We take the next
	 * highest size if the memsize is not a power of two.
	 * TODO NMGS make this configurable at boot time.
	 */

	num = sizeof(pte_t) * (mem_size >> 10);

	for (hash_table_size = 64 * 1024;	/* minimum size = 64Kbytes */
	     hash_table_size < num; 
	     hash_table_size *= 2)
		continue;

	/* Scale to within any physical memory layout constraints */
	do {
		num = atop(mem_size);	/* num now holds mem_size in pages */

		/* size of all structures that we're going to allocate */

		size = (vm_size_t) (hash_table_size +
				    powermac_info.dma_buffer_size +
				    sizeof(struct phys_entry) * num +
				    (sizeof(struct mapping) *
				     hash_table_size / sizeof(pte_t)) +
				    (pte_lock_table_size(hash_table_size
							 / sizeof (pte_t))));

		size = round_page(size);

		/* DMA BUFFER MUST BE aligned to its size
		 * HASH TABLE MUST BE aligned to its size */

		if (hash_table_size > powermac_info.dma_buffer_alignment)
			addr = (*first_avail +
				(hash_table_size-1)) & ~(hash_table_size-1);
		else
			addr = (*first_avail +
				(powermac_info.dma_buffer_alignment-1)) &
				~(powermac_info.dma_buffer_alignment-1);

		if (addr + size > pmap_mem_regions[0].end) {
			printf("warning : hash table halved to fit in "
			       "lower memory - pmap.c needs fixing!\n");

			hash_table_size /= 2;
		} else {
			break;	/* EXIT */
		}
		/* If we have had to shrink hash table to too small, panic */
		if (hash_table_size == 32 * 1024)
			panic("cannot lay out pmap memory map correctly");
	} while (1);

	/* TODO NMGS - we step over the end of a read/write page here,
	 * TODO NMGS - in theory we could use this for mappings.
	 */
	if (round_page(*first_avail) + PPC_PGBYTES < round_page(addr)) {
		/* We are stepping over at least one page here, so
		 * add this region to the free regions so that it can
		 * be allocated by pmap_steal
		 */
		free_regions[free_regions_count].start = 
			round_page(*first_avail) + PPC_PGBYTES;
		free_regions[free_regions_count].end = round_page(addr);

		avail_remaining += (free_regions[free_regions_count].end -
				    free_regions[free_regions_count].start) /
					    PPC_PGBYTES;
#if DEBUG
		DPRINTF(("ADDED FREE REGION from 0x%08x to 0x%08x, avail_remaining = %d\n",free_regions[free_regions_count].start,free_regions[free_regions_count].end, avail_remaining));
#endif /* DEBUG */
		free_regions_count++;
	}

	/* Zero everything - this also invalidates the hash table entries */
	bzero((char *)addr, size);

	/* Set up some pointers to our new structures */

	/* from here,  addr points to the next free address */
	
	first_used_addr = addr;	/* remember where we started */

	/* Set up hash table address and dma buffer address, keeping
	 * alignment. These mappings are all 1-1,  so dma_r == dma_v
	 * 
	 * If hash_table_size == dma_buffer_alignment, then put hash_table
	 * first, since dma_buffer_size may be smaller than alignment, but
	 * hash table alignment==hash_table_size.
	 */
	if (hash_table_size >= powermac_info.dma_buffer_alignment) {
		hash_table_base = addr;
		addr += hash_table_size;

		powermac_info.dma_buffer_virt = addr;
		powermac_info.dma_buffer_phys = addr;

		addr += powermac_info.dma_buffer_size;
	} else {
		powermac_info.dma_buffer_virt = addr;
		powermac_info.dma_buffer_phys = addr;

		addr += powermac_info.dma_buffer_size;

		hash_table_base = addr;
		addr += hash_table_size;
	}
	assert((hash_table_base & (hash_table_size-1)) == 0);
	assert((powermac_info.dma_buffer_size == 0) ||
		(powermac_info.dma_buffer_virt &
		(powermac_info.dma_buffer_alignment-1)) == 0);

	/* phys_table is static to help debugging,
	 * this variable is no longer actually used
	 * outside of this scope
	 */

	phys_table = (struct phys_entry *) addr;

	for (i = 0; i < pmap_mem_regions_count; i++) {
		pmap_mem_regions[i].phys_table = phys_table;
		phys_table = phys_table +
			atop(pmap_mem_regions[i].end - pmap_mem_regions[i].start);
	}

	/* restore phys_table for debug */
	phys_table = (struct phys_entry *) addr;

	addr += sizeof(struct phys_entry) * num;
	simple_lock_init(&physentries_lock, ETAP_VM_PMAP_PHYSENTRIES);
#if	MACH_LDEBUG
	for (i = 0; i < NCPUS; i++) {
		simple_lock_init(&fake_physentry_lock[i], ETAP_VM_PMAP_PHYSENTRIES);
		fake_physentry_lock_count[i] = 0;
		simple_lock_init(&fake_pte_lock[i], ETAP_VM_PMAP_PTE);
		fake_pte_lock_count[i] = 0;
	}
#endif	/* MACH_LDEBUG */

	/* allocate pte_lock_table */
	pte_lock_table = (char *) addr;
	addr += pte_lock_table_size(hash_table_size / sizeof (pte_t));
	simple_lock_init(&pte_lock_table_lock, ETAP_VM_PMAP_PTE);
	simple_lock_init(&pteg_overflow_lock, ETAP_VM_PMAP_PTE_OVFLW);
	simple_lock_init(&tlb_system_lock, ETAP_VM_PMAP_TLB);

	/* Initialise the registers necessary for supporting the hashtable */
	hash_table_init(hash_table_base, hash_table_size);

	/* Initialise the physical table mappings */ 
	for (i = 0; i < num; i++) {
		phys_table[i].phys_link = MAPPING_NULL;
		phys_table[i].pte1.bits.wimg = PTE_WIMG_DEFAULT;
		phys_table[i].locked = FALSE;
	}

	/*
	 * Remaining space is for mapping entries.  Chain them
	 * together (XXX can't use a zone
	 * since zone package hasn't been initialized yet).
	 */

	mp = (struct mapping *) addr;

	mapping_base = (struct mapping *)addr;
	mapping_last = (struct mapping *)(first_used_addr + size)-1;

#if DEBUG
	printf("Mapping entries use memory from 0x%08x to 0x%08x\n",
	       mapping_base, mapping_last);
	/*
         * Map the memory we grabbed correctly using the page tables so
	 * that the BAT3 register becomes redundant.
	 */

DPRINTF(("mapping memory tables from 0x%08x to 0x%08x, to address 0x%08x\n",
	 first_used_addr, round_page(first_used_addr+size),
	 first_used_addr));
DPRINTF(("dma_buffer virt=0x%08x phys=0x%08x size=0x%08x\n",
	 powermac_info.dma_buffer_virt, powermac_info.dma_buffer_phys,
	 powermac_info.dma_buffer_size));
#endif /* DEBUG */

	pmap_map(first_used_addr,
		 first_used_addr,
		 round_page(first_used_addr+size),
		 VM_PROT_READ | VM_PROT_WRITE);

	/* Remap the DMA buffer region as a board (ie. non-cached) */
	pmap_map_bd(powermac_info.dma_buffer_virt,
		    powermac_info.dma_buffer_phys,
		    powermac_info.dma_buffer_virt +
		    	powermac_info.dma_buffer_size,
		    VM_PROT_READ | VM_PROT_WRITE);

	*first_avail = round_page(mapping_last);

	/* All the rest of memory is free - add it to the free
	 * regions so that it can be allocated by pmap_steal
	 */
	free_regions[free_regions_count].start = *first_avail;
	free_regions[free_regions_count].end = pmap_mem_regions[0].end;

	avail_remaining += (free_regions[free_regions_count].end -
			    free_regions[free_regions_count].start) /
				    PPC_PGBYTES;

#if DEBUG
	DPRINTF(("ADDED FREE REGION from 0x%08x to 0x%08x, avail_remaining = %d\n",free_regions[free_regions_count].start,free_regions[free_regions_count].end, avail_remaining));
#endif /* DEBUG */

	free_regions_count++;

	current_free_region = 0;

	avail_next = free_regions[current_free_region].start;
}

/*
 * pmap_init(spa, epa)
 *	finishes the initialization of the pmap module.
 *	This procedure is called from vm_mem_init() in vm/vm_init.c
 *	to initialize any remaining data structures that the pmap module
 *	needs to map virtual memory (VM is already ON).
 */
/*ARGSUSED*/
void
pmap_init(void)
{
	vm_size_t s;

	s = sizeof(struct pmap);
	pmap_zone = zinit(s, 400*s, 4096, "pmap");	/* XXX */

	simple_lock_init(&sid_counter_lock, ETAP_VM_PMAP_SID);

	pmap_initialized = TRUE;

	/*
	 *	Initialize list of freed up pmaps
	 */
	queue_init(&free_pmap_list);
	free_pmap_count = 0;
	simple_lock_init(&free_pmap_lock, ETAP_VM_PMAP_CACHE);
}

unsigned int pmap_free_pages(void)
{
	return avail_remaining;
}

boolean_t pmap_next_page(vm_offset_t *addrp)
{
	/* Non optimal, but only used for virtual memory startup.
         * Allocate memory from a table of free physical addresses
	 * If there are no more free entries, too bad. We have two
	 * tables to look through, free_regions[] which holds free
	 * regions from inside pmap_mem_regions[0], and the others...
	 * pmap_mem_regions[1..]
         */
	/* current_free_region indicates the next free entry,
	 * if it's less than free_regions_count, then we're still
	 * in free_regions, otherwise we're in pmap_mem_regions
	 */

	if (current_free_region >= free_regions_count) {
		/* We're into the pmap_mem_regions, handle this
		 * separately to free_regions
		 */

		int current_pmap_mem_region = current_free_region -
					 free_regions_count + 1;
		if (current_pmap_mem_region > pmap_mem_regions_count)
			return FALSE;
		*addrp = avail_next;
		avail_next += PAGE_SIZE;
		avail_remaining--;
		if (avail_next >= pmap_mem_regions[current_pmap_mem_region].end) {
			current_free_region++;
			current_pmap_mem_region++;
			avail_next = pmap_mem_regions[current_pmap_mem_region].start;
#if DEBUG
			DPRINTF(("pmap_next_page : next region start=0x%08x\n",avail_next));
#endif /* DEBUG */
		}
		return TRUE;
	}
	
	/* We're in the free_regions, allocate next page and increment
	 * counters
	 */
	*addrp = avail_next;

	avail_next += PAGE_SIZE;
	avail_remaining--;

	if (avail_next >= free_regions[current_free_region].end) {
		current_free_region++;
		if (current_free_region < free_regions_count)
			avail_next = free_regions[current_free_region].start;
		else
			avail_next = pmap_mem_regions[current_free_region -
						 free_regions_count + 1].start;
#if DEBUG
		printf("pmap_next_page : next region start=0x%08x\n",avail_next);
#endif 
	}
	return TRUE;
}

void pmap_virtual_space(
	vm_offset_t *startp,
	vm_offset_t *endp)
{
	/* Give first VM address above which free space is guaranteed, ignoring
         * the holes below. In this case, the memory is mapped :
	 *
	 *			PHYSICAL	VIRTUAL
	 *   KERNEL TEXT 	0-1M		  1-1
	 *   VIDEO		1-2M		  1-1
	 *   KERNEL DATA	2-xxx		  1-1
	 *   PMAP TABLES	after kernel data 1-1
	 *   IO stuff           > 0x50000000      1-1
	 *
	 * so the first guaranteed free virtual address is above the video
         */
	*startp = round_page(mapping_last);
	*endp   = VM_MAX_KERNEL_ADDRESS;
}

/*
 * pmap_create
 *
 * Create and return a physical map.
 *
 * If the size specified for the map is zero, the map is an actual physical
 * map, and may be referenced by the hardware.
 *
 * If the size specified is non-zero, the map will be used in software 
 * only, and is bounded by that size.
 */
pmap_t
pmap_create(vm_size_t size)
{
	pmap_t pmap;
	int s;
	space_t sid;

#if PMAP_LOWTRACE
	dbgTrace(0xF1D00001, size, 0);			/* (TEST/DEBUG) */
#endif

#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_create(size=%x)%c", size, size ? '\n' : ' ');
#endif

	/*
	 * A software use-only map doesn't even need a pmap structure.
	 */
	if (size)
		return(PMAP_NULL);

	/* 
	 * If there is a pmap in the pmap free list, reuse it. 
	 */
	s = splvm();
	simple_lock(&free_pmap_lock);
	if (!queue_empty(&free_pmap_list)) {
		queue_remove_first(&free_pmap_list,
				   pmap,
				   pmap_t,
				   pmap_link);
		free_pmap_count--;
	} else {
		pmap = PMAP_NULL;
	}
	simple_unlock(&free_pmap_lock);
	splx(s);

	/*
	 * Couldn't find a pmap on the free list, try to allocate a new one.
	 */
	if (pmap == PMAP_NULL) {
		pmap = (pmap_t) zalloc(pmap_zone);
		if (pmap == PMAP_NULL)
			return(PMAP_NULL);

		/*
		 * Allocate space IDs for the pmap.
		 * If all are allocated, there is nothing we can do.
		 */
		s = splvm();
		simple_lock(&sid_counter_lock);

		/* If sid_counter == MAX_SID, we've allocated
		 * all the possible SIDs and they're all live,
		 * we can't carry on, but this is HUGE, so don't
		 * expect it in normal operation.
		 */
		assert(sid_counter != SID_MAX);

		/* Try to spread out the sid's through the possible
		 * name space to improve the hashing heuristics
		 */
		sid_counter = (sid_counter + PPC_SID_PRIME) & PPC_SID_MASK;

		/* 
		 * Initialize the sids
		 */
		pmap->space = sid_counter;

		simple_unlock(&sid_counter_lock);
		splx(s);

		simple_lock_init(&pmap->lock, ETAP_VM_PMAP);
	}
	pmap->ref_count = 1;
	pmap->stats.resident_count = 0;
	pmap->stats.wired_count = 0;

	pmap_add_pmap(pmap);

#if PMAP_LOWTRACE
	dbgTrace(0xF1D00002, (unsigned int)pmap, (unsigned int)pmap->space);	/* (TEST/DEBUG) */
#endif

#if DEBUG
	if (pmdebug & PDB_USER)
		printf("-> %x, space id = %d\n", pmap, pmap->space);
#endif
	return(pmap);
}

/* 
 * pmap_destroy
 * 
 * Gives up a reference to the specified pmap.  When the reference count 
 * reaches zero the pmap structure is added to the pmap free list.
 *
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pmap_t pmap)
{
	int ref_count;
	spl_t s;

#if PMAP_LOWTRACE
	dbgTrace(0xF1D00003, (unsigned int)pmap, 0);			/* (TEST/DEBUG) */
#endif

#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_destroy(pmap=%x)\n", pmap);
#endif

	if (pmap == PMAP_NULL)
		return;

	s = splvm();
	simple_lock(&pmap->lock);
	ref_count = --pmap->ref_count;
	simple_unlock(&pmap->lock);

	if (ref_count < 0)
		panic("pmap_destroy(): ref_count < 0");
	if (ref_count > 0) {
		splx(s);
		return;
	}

	assert(pmap->stats.resident_count == 0);
	pmap_rem_pmap(pmap);
	/* 
	 * Add the pmap to the pmap free list. 
	 */
	simple_lock(&free_pmap_lock);
	if (free_pmap_count <= free_pmap_max) {
		
		queue_enter_first(&free_pmap_list, pmap, pmap_t, pmap_link);
		free_pmap_count++;
		simple_unlock(&free_pmap_lock);
	} else {
		simple_unlock(&free_pmap_lock);
		zfree(pmap_zone, (vm_offset_t) pmap);
	}
	splx(s);
}

/*
 * pmap_reference(pmap)
 *	gains a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{
	spl_t s;

#if PMAP_LOWTRACE
	dbgTrace(0xF1D00004, (unsigned int)pmap, 0);			/* (TEST/DEBUG) */
#endif

#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_reference(pmap=%x)\n", pmap);
#endif

	if (pmap != PMAP_NULL) {
		s = splvm();
		simple_lock(&pmap->lock);
		pmap->ref_count++;
		simple_unlock(&pmap->lock);
		splx(s);
	}
}

/*
 * pmap_remove(pmap, s, e)
 *	unmaps all virtual addresses v in the virtual address
 *	range determined by [s, e) and pmap.
 *	s and e must be on machine independent page boundaries and
 *	s must be less than or equal to e.
 */
void
pmap_remove(
	    pmap_t pmap,
	    vm_offset_t sva,
	    vm_offset_t eva)
{
	spl_t			spl;
	space_t			space;
	struct mapping		*mp;
	struct phys_entry	*phe;
	pte_t			*pte;

#if PMAP_LOWTRACE
	dbgTrace(0xF1D00005, (unsigned int)pmap, sva|((eva-sva)>>12));	/* (TEST/DEBUG) */
#endif

#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_remove(pmap=%x, sva=%x, eva=%x)\n",
		       pmap, sva, eva);
#endif

	if (pmap == PMAP_NULL)
		return;

	PMAP_READ_LOCK(pmap, spl);

	space = pmap->space;

	/* It is just possible that eva might have wrapped around to zero,
	 * and sometimes we get asked to liberate something of size zero
	 * even though it's dumb (eg. after zero length read_overwrites)
	 */
	assert(eva >= sva);

	/* If these are not page aligned the loop might not terminate */
	assert((sva == trunc_page(sva)) && (eva == trunc_page(eva)));

	/* We liberate addresses from high to low, since the stack grows
	 * down. This means that we won't need to test addresses below
	 * the limit of stack growth
	 */

	while (pmap->stats.resident_count && (eva > sva)) {
		eva -= PAGE_SIZE;

		pte = find_or_allocate_pte(space, eva, FALSE);
		/* the pte is locked if not null */
		if (pte != PTE_NULL) {
			mp = PTE2MP(pte);
			phe = pmap_find_physentry(trunc_page(pte->pte1.word));
			LOCK_PHYSENTRY(phe);
			pmap_free_mapping(mp, phe);
			pmap->stats.resident_count--;
			UNLOCK_PHYSENTRY(phe);
			UNLOCK_PTE(pte);
		}
	}

	PMAP_READ_UNLOCK(pmap, spl);
}

/*
 *	Routine:
 *		pmap_page_protect
 *
 *	Function:
 *		Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(
	vm_offset_t pa,
	vm_prot_t prot)
{
	spl_t				spl;
	register struct phys_entry 	*pp;
	register struct mapping 	*mp;
	unsigned int 			pteprot;
	boolean_t 			remove;
	pte_t				*pte;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00006, (unsigned int)pa, (unsigned int)prot);	/* (TEST/DEBUG) */
#endif

#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_page_protect(pa=%x, prot=%x)\n", pa, prot);
#endif

	switch (prot) {
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		remove = FALSE;
		break;
	case VM_PROT_ALL:
		return;
	default:
		remove = TRUE;
		break;
	}

	/*
	 * Lock the pmap system first, since we will be changing
	 * several pmaps.
	 * We don't have to lock the phys_entry because we have the
	 * entire pmap system locked. In fact, we can't lock it because
	 * we have to lock the PTEs after that and that would violate the
	 * usual lock ordering (lock PTE before phys_entry).
	 */
	PMAP_WRITE_LOCK(spl);

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL) {
		PMAP_WRITE_UNLOCK(spl);
		return;
	}

	if (remove) {
		while ((mp = pp->phys_link) != MAPPING_NULL) {
			pmap_get_pmap(mp)->stats.resident_count--;
			pte = MP2PTE(mp);
			LOCK_PTE(pte);
			pmap_free_mapping(mp, pp);
			UNLOCK_PTE(pte);
		}
		PMAP_WRITE_UNLOCK(spl);
		return;
	}

	/*
	 * Modify mappings if necessary.
	 */
	for (mp = pp->phys_link; mp != MAPPING_NULL; mp = mp->next) {

		pte_t *pte;

		pte = MP2PTE(mp);

		/*
		 * Compare new protection with old to see if
		 * anything needs to be changed.
		 */
		pteprot = prot_bits[prot];

		/*
		 * Lock the PTE.
		 */
		LOCK_PTE(pte);

		if (pte->pte1.bits.protection != pteprot) {

			/*
			 * Perform a sync to force saving
			 * of change/reference bits, followed by a
			 * fault and reload with the new protection.
			 */
			pte->pte0.bits.valid = FALSE;
			sync();

			simple_lock(&tlb_system_lock);
			tlbie(mp->vm_info.bits.page << PPC_PGSHIFT);
			simple_unlock(&tlb_system_lock);
			TLBSYNC_SYNC();
			pte->pte1.bits.protection = pteprot;
			eieio();

			pte->pte0.bits.valid = TRUE;
			sync();
		}
		pp->pte1.word |= pte->pte1.word;

		UNLOCK_PTE(pte);
	}
	PMAP_WRITE_UNLOCK(spl);
}

boolean_t
pmap_verify_free(vm_offset_t pa)
{
	spl_t			spl;
	struct phys_entry	*pp;
	boolean_t		result;

#if PMAP_LOWTRACE
	dbgTrace(0xF1D00007, (unsigned int)pa, 0);	/* (TEST/DEBUG) */
#endif

#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_verify_free(pa=%x)\n", pa);
#endif

	if (!pmap_initialized)
		return(TRUE);

	/*
	 * Lock the entire pmap system since we don't have a specific
	 * pmap to lock. XXX
	 */
	PMAP_WRITE_LOCK(spl);

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		result = FALSE;
	else 
		result = (pp->phys_link == MAPPING_NULL);

	PMAP_WRITE_UNLOCK(spl);

	return result;
}

/*
 * pmap_protect(pmap, s, e, prot)
 *	changes the protection on all virtual addresses v in the 
 *	virtual address range determined by [s, e] and pmap to prot.
 *	s and e must be on machine independent page boundaries and
 *	s must be less than or equal to e.
 */
void
pmap_protect(
	     pmap_t pmap,
	     vm_offset_t sva, 
	     vm_offset_t eva,
	     vm_prot_t prot)
{
	spl_t				spl;
	struct mapping			*mp;
	pte_t				*pte;
	unsigned int			pteprot;
	space_t				space;

#if PMAP_LOWTRACE
	dbgTrace(0xF1D00008, (unsigned int)pmap, (unsigned int)(sva|((eva-sva)>>12)));	/* (TEST/DEBUG) */
#endif

#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_protect(pmap=%x, sva=%x, eva=%x, prot=%x)\n",
		       pmap, sva, eva, prot);
#endif

	if (pmap == PMAP_NULL)
		return;

	assert(sva <= eva);
	assert(eva < 0xfffff000); /* Otherwise loop will not terminate */

	if (prot == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	if (prot & VM_PROT_WRITE)
		return;

	PMAP_READ_LOCK(pmap, spl);

	sva = trunc_page(sva);

	space = pmap->space;
	for ( ; sva < eva; sva += PAGE_SIZE) {
		pte = find_or_allocate_pte(space, sva, FALSE);
		if (pte == PTE_NULL)
			continue;
		/* pte is now locked */

		assert(pmap == pmap_get_pmap(PTE2MP(pte)));
		/*
		 * Determine if mapping is changing.
		 * If not, nothing to do.
		 */
		pteprot = prot_bits[prot];
		if (pte->pte1.bits.protection == pteprot) {
			UNLOCK_PTE(pte);
			continue;
		}
		
		/*
		 * Sync to force any modifications to 
		 * changed/referenced bits to be flushed, and tlbie so
		 * that future references will fault and reload with the
		 * new protection.
		 */

		/*	Perform a general case PTE update designed to work
		 *	in SMP configurations.
		 */
		pte->pte0.bits.valid = FALSE;	/* Invalidate pte entry */
		sync();				/* Force updates to complete */

		simple_lock(&tlb_system_lock);
		tlbie(sva);
		simple_unlock(&tlb_system_lock);
		TLBSYNC_SYNC();

		pte->pte1.bits.protection = pteprot;	/* Change just prot */

		eieio();
		pte->pte0.bits.valid = TRUE;	/* Validate pte entry */
		sync();

		UNLOCK_PTE(pte);
	}

	PMAP_READ_UNLOCK(pmap, spl);
}

/*
 * pmap_enter
 *
 * Create a translation for the virtual address (virt) to the physical
 * address (phys) in the pmap with the protection requested. If the
 * translation is wired then we can not allow a page fault to occur
 * for this mapping.
 *
 * NB: This is the only routine which MAY NOT lazy-evaluate
 *     or lose information.  That is, this routine must actually
 *     insert this page into the given map NOW.
 */
void
pmap_enter(pmap_t pmap, vm_offset_t va, vm_offset_t pa, vm_prot_t prot,
	   boolean_t wired)
{
	spl_t			spl;
	struct mapping		*mp,*mp2;
	struct phys_entry	*pp,*old_pp;
	pte_t			*pte;
	space_t			space;
	boolean_t		was_wired;
	int			x;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00009, (unsigned int)pmap, (unsigned int)va);	/* (TEST/DEBUG) */
	dbgTrace(0xF1D04009, (unsigned int)pa, (unsigned int)prot);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if ((wired && (pmdebug & (PDB_WIRED))) ||
	     (pmdebug & (PDB_USER|PDB_ENTER)))
		printf("pmap_enter(pmap=%x, va=%x, pa=%x, prot=%x, wire=%x)\n",
		       pmap, va, pa, prot, wired);
	if ((pmdebug & (PDB_IO)) && (trunc_page(pa) == 0) &&
	    !(pmap == kernel_pmap && va == KERNELBASE_TEXT)) {
		printf("WARNING pmap_enter(pmap=%x, va=%x, "
		       "pa=%x, prot=%x, wire=%x) "
		       "MAPPING PHYSICAL PAGE ZERO\n",
		       pmap, va, pa, prot, wired);
		Debugger("trying to map phys page zero");
	}

#endif
        if (pmap == PMAP_NULL)
                return;
	PMAP_READ_LOCK(pmap, spl);

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL) {
		PMAP_READ_UNLOCK(pmap, spl);
		return;
	}

	/* Find the pte associated with the virtual address/space ID */
	space = pmap->space;

	pte = find_or_allocate_pte(space,va,TRUE);
	/* the PTE is now locked (if not NULL) */

	assert(pte != PTE_NULL);
	/* Even if the pte is new, pte0 should be set up for the new mapping,
	 * but with the valid bit set to FALSE
	 */
	assert((pte->pte0.word & 0x7fffffff) != PTE_EMPTY);

	if (pte->pte0.bits.valid == TRUE) {

		/* mapping is already valid, thus virtual address was
		 * already mapped somewhere...
		 */

		if (((physical_addr_t *) &pa)->bits.page_no !=
	    	    pte->pte1.bits.phys_page) {

			/* The current mapping is to a different
			 * physical addr - release the mapping
			 * before mapping the new address. 
			 */

#if DEBUG
			if (pmdebug & (PDB_USER|PDB_ENTER))
				printf("Virt 0x%08x was mapped to different address 0x%08x, changing mapping\n",va,((physical_addr_t *) &pa)->bits.page_no * PPC_PGBYTES);
#endif

			/* Take advantage of the fact that pte1 can be
			 * considered as a pointer to an address in
			 * the physical page when looking for physentry.
			 */

			old_pp = pmap_find_physentry(
					     (vm_offset_t)(pte->pte1.word));
			LOCK_PHYSENTRY(old_pp);

			mp = PTE2MP(pte);

			/* pte entry must have good reverse mapping */
			assert(mp != NULL);
			assert(((mp->vm_info.bits.page >> 10) & 0x3f) ==
			       pte->pte0.bits.page_index);

			/* Wired/not wired is taken care of later on
			 * in this function. We can remove wired mappings.
			 */

			/* Do the equivalent of pmap_free_mapping followed
			 * by a pmap_enter_mapping, recycling the pte and
			 * the mapping
			 */

			pte->pte0.bits.valid = FALSE;
			sync();						/* Force update to complete */

			/* Flush tlb before looking at changed/reference */
			simple_lock(&tlb_system_lock);
			tlbie(va);					/* Wipe out this virtual address */
			simple_unlock(&tlb_system_lock);
			eieio();
			TLBSYNC_SYNC();

			/* keep track of changed and referenced at old
			 * address
			 */
			old_pp->pte1.word |= MP2PTE(mp)->pte1.word;

			/* Delete the mapping for the phys list */

			/* Find the mapping pointing to this one */
			mp2 = (struct mapping *)old_pp;
			while (mp2->next != mp)
				mp2 = mp2->next;
			/* And remove the mapping */
			mp2->next = mp->next;

			UNLOCK_PHYSENTRY(old_pp);

			mp->vm_info.bits.page  = (va >> PPC_PGSHIFT);

			LOCK_PHYSENTRY(pp);

			/* Insert mapping into new phys list */
			mp->next = pp->phys_link;
			pp->phys_link = mp;

			assert(pmap_get_pmap(mp) == pmap);

			/* Set up pte as it would have been done by
			 * find_or_allocate_pte() + pmap_enter_mapping()
			 */
			pte->pte1.word 		  = 0;
			pte->pte1.bits.phys_page  = pa >> PPC_PGSHIFT;

			pte->pte1.bits.wimg 	  = pp->pte1.bits.wimg; /* default */

			/* pte0 is already set up as we need it */

			/* Don't mark valid yet, will set protection below */

		} else {
			/* The current mapping is the same as the one
			 * we're being asked to make, find the mapping
			 * structure that goes along with it.
			 */
			LOCK_PHYSENTRY(pp);
			mp = PTE2MP(pte);

			/* invalidate tlb for previous mapping to make sure
			 * that we don't trample on changed/referenced bits
			 */

			pte->pte0.bits.valid = FALSE;
			sync();

			simple_lock(&tlb_system_lock);
			tlbie(va);
			simple_unlock(&tlb_system_lock);
			eieio();
			TLBSYNC_SYNC();

#if DEBUG
			if ((wired && (pmdebug & (PDB_WIRED))) ||
			    (pmdebug & (PDB_USER|PDB_ENTER))) {
				printf("address already mapped, changing prots or wiring\n");
				if (pte->pte1.bits.protection !=
				    prot_bits[prot])
					printf("Changing PROTS\n");
				if (wired != mp->vm_info.bits.wired)
				       printf("changing WIRING\n");
			}
#endif
		}
	} else {
		/* There was no pte match, a new entry was created
                 * (marked as invalid). We make sure that a new mapping
		 * structure is allocated as its partner.
		 */
		mp = MAPPING_NULL;
		assert(pte->pte0.word != PTE_EMPTY);
		LOCK_PHYSENTRY(pp);
	}

	/* Once here, pte contains a valid pointer to a structure
	 * that we may use to map the virtual address (which may
	 * already map this (and be valid) or something else (invalid)
	 * and mp contains either MAPPING_NULL (new mapping) or
	 * a pointer to the old mapping which may need it's
	 * privileges modified.
	 */

	/* set up the protection bits on new mapping
	 */

	pte->pte1.bits.protection = prot_bits[prot];
	/* eieio is done in either pmap_enter_mapping or other branch of if */

	if (mp == MAPPING_NULL) {
		/*
		 * Mapping for this virtual address doesn't exist.
		 * Get a mapping structure and, and fill it in,
		 * updating the pte that we already have, and
		 * making it valid
		 */
		mp = pmap_enter_mapping(pmap, space, va, pa, pte, prot, pp);
		pmap->stats.resident_count++;
		was_wired = FALSE;/* indicate that page was never wired */
	} else {
		/* make sure previous pte1 writes have completed */
		eieio();
		/* The pte is now ready, mark it valid */
		pte->pte0.bits.valid = TRUE;
		sync();
		
		/*
		 * We are just changing the protection of a current mapping.
		 */
		was_wired = mp->vm_info.bits.wired;

	}

	/* if wired status has changed, update stats and change bit */
	if (was_wired != wired) {
		pmap->stats.wired_count += wired ? 1 : -1;
		mp->vm_info.bits.wired = wired;
#if DEBUG
		if (pmdebug & (PDB_WIRED))
			printf("changing wired status to : %s\n",
			       wired ? "TRUE" : "FALSE");
#endif /* DEBUG */
	}
	UNLOCK_PHYSENTRY(pp);

	/* Belt and braces check to make sure we didn't give a bogus map,
	 * this map is given once when mapping the kernel
	 */
#if	MACH_KGDB
	assert((pte->pte1.bits.phys_page != 0) ||
	       (kgdb_kernel_in_pmap == FALSE));
#endif	/* MACH_KGDB */
	
#if DEBUG

	/*	Assert a PTE of type WIMG_IO is not mapped to general RAM.
	*/
	if ((pmdebug & PDB_IO)&& (pte->pte1.bits.wimg == PTE_WIMG_IO))
		assert ((((unsigned) pa) >= PCI_BASE) || IS_VIDEO_MEMORY(pa));

#endif	/* DEBUG */

	UNLOCK_PTE(pte);


#if PMAP_LOWTRACE
	dbgTrace(0xF1D08009, (unsigned int) pte, (unsigned int) pte->pte1.word);	/* (TEST/DEBUG) */
#endif
#if	DEBUG
	if (pmdebug & (PDB_USER|PDB_ENTER))
		printf("leaving pmap_enter\n");
#endif

	PMAP_READ_UNLOCK(pmap, spl);

}

/*
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 *
 * Change the wiring for a given virtual page. This routine currently is 
 * only used to unwire pages and hence the mapping entry will exist.
 */
void
pmap_change_wiring(
	register pmap_t	pmap,
	vm_offset_t	va,
	boolean_t	wired)
{
	spl_t			spl;
	struct mapping		*mp;
	boolean_t		waswired;
	pte_t			*pte;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D0000A, (unsigned int)pmap, (unsigned int)va);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & (PDB_USER | PDB_WIRED))
		printf("pmap_change_wiring(pmap=%x, va=%x, wire=%x)\n",
		       pmap, va, wired);
#endif
	if (pmap == PMAP_NULL)
		return;

	/*
	 * We must grab the pmap system lock because we may
	 * change a phys_entry's mapping.
	 */
	PMAP_READ_LOCK(pmap, spl);

	pte = find_or_allocate_pte(pmap->space, va, FALSE);
	/* pte is now locked */
	mp = PTE2MP(pte);

	/* Can't change the wiring of something that is not mappable */
	assert(pmap_find_physentry(trunc_page(pte->pte1.word)) != PHYS_NULL);

	waswired = mp->vm_info.bits.wired;
	if (wired && !waswired) {
		mp->vm_info.bits.wired = TRUE;
		pmap->stats.wired_count++;
	} else if (!wired && waswired) {
		mp->vm_info.bits.wired = FALSE;
		pmap->stats.wired_count--;
	}
	UNLOCK_PTE(pte);
	PMAP_READ_UNLOCK(pmap, spl);
}

/*
 * pmap_extract(pmap, va)
 *	returns the physical address corrsponding to the 
 *	virtual address specified by pmap and va if the
 *	virtual address is mapped and 0 if it is not.
 */
vm_offset_t
pmap_extract(
	pmap_t pmap,
	vm_offset_t va)
{
	spl_t		spl;
	pte_t		*pte;
	vm_offset_t	pa;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D0000B, (unsigned int)pmap, (unsigned int)va);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_extract(pmap=%x, va=%x)\n", pmap, va);
#endif

	spl = splvm();
	simple_lock(&pmap->lock);

	pte = find_or_allocate_pte(pmap->space, trunc_page(va), FALSE);
	/* the PTE is locked now (if not NULL) */
	if (pte == NULL) {
		pa = (vm_offset_t) 0;
	} else {
		pa = (vm_offset_t) 
			trunc_page(pte->pte1.word) | (va & page_mask);
		UNLOCK_PTE(pte);
	}

	simple_unlock(&pmap->lock);
	splx(spl);

	return pa;
}

/*
 *	pmap_attributes:
 *
 *	Set/Get special memory attributes
 *
 *	Note: 'VAL_GET_INFO' is used to return info about a page.
 *	  If less than 1 page is specified, return the physical page
 *	  mapping and a count of the number of mappings to that page.
 *	  If more than one page is specified, return the number
 *	  of resident pages and the number of shared (more than
 *	  one mapping) pages in the range;
 *
 */
kern_return_t
pmap_attribute(pmap, address, size, attribute, value)
	pmap_t		pmap;
	vm_offset_t	address;
	vm_size_t	size;
	vm_machine_attribute_t	attribute;
	vm_machine_attribute_val_t* value;	
{
	spl_t		s;
	vm_offset_t 	sva, eva;
	vm_offset_t	pa;
	pte_t		*pte;
	kern_return_t	ret;

	if (attribute != MATTR_CACHE)
		return KERN_INVALID_ARGUMENT;

	/* We can't get the caching attribute for more than one page
	 * at a time
	 */
	if ((*value == MATTR_VAL_GET) &&
	    (trunc_page(address) != trunc_page(address+size-1)))
		return KERN_INVALID_ARGUMENT;

	if (pmap == PMAP_NULL)
		return KERN_SUCCESS;

	sva = trunc_page(address);
	eva = round_page(address + size);
	ret = KERN_SUCCESS;

	switch (*value) {
	case MATTR_VAL_CACHE_SYNC:	/* sync I+D caches */
		if (PROCESSOR_VERSION == PROCESSOR_VERSION_601) {
			break;
		}
		sva = trunc_page(sva);
		s = splvm();
		simple_lock(&pmap->lock);
		for (; sva < eva; sva += PAGE_SIZE) {

			pte = find_or_allocate_pte(pmap->space,
						   trunc_page(sva), FALSE);
			/* the PTE is locked now (if not NULL) */
			if (pte == NULL) {
				continue;
			}
			if (pte->pte1.bits.wimg == PTE_WIMG_DEFAULT) {
				pa = (vm_offset_t) trunc_page(pte->pte1.word);
				simple_unlock(&pmap->lock);
				splx(s);
				/* do this outside of lock for preemptiveness*/
				sync_cache(pa, PAGE_SIZE);
				s = splvm();
				simple_lock(&pmap->lock);
			}
			UNLOCK_PTE(pte);
		}
		simple_unlock(&pmap->lock);
		splx(s);
		break;

	case MATTR_VAL_CACHE_FLUSH:	/* flush from all caches */
	case MATTR_VAL_DCACHE_FLUSH:	/* flush from data cache(s) */
	case MATTR_VAL_ICACHE_FLUSH:	/* flush from instr cache(s) */
		sva = trunc_page(sva);
		s = splvm();
		simple_lock(&pmap->lock);
		for (; sva < eva; sva += PAGE_SIZE) {

			pte = find_or_allocate_pte(pmap->space,
						   trunc_page(sva), FALSE);
			/* the PTE is locked now (if not NULL) */
			if (pte == NULL) {
				continue;
			}
			if (pte->pte1.bits.wimg == PTE_WIMG_DEFAULT) {
				pa = (vm_offset_t) trunc_page(pte->pte1.word);
				simple_unlock(&pmap->lock);
				splx(s);
				/* do this outside of lock for preemptiveness*/
				/* For DCACHE and CACHE FLUSH,
				   flush data cache */
				if (*value != MATTR_VAL_ICACHE_FLUSH)
					flush_dcache(pa, PAGE_SIZE, TRUE);
				/* For ICACHE and CACHE FLUSH
				   "flush" instr cache */
				if (*value != MATTR_VAL_DCACHE_FLUSH)
					invalidate_icache(pa, PAGE_SIZE,
							  TRUE);

				s = splvm();
				simple_lock(&pmap->lock);
			}
			UNLOCK_PTE(pte);
		}
		simple_unlock(&pmap->lock);
		splx(s);
		break;

	case MATTR_VAL_GET_INFO:	/* get info */
	  if (size < PAGE_SIZE) {
		sva = trunc_page(sva);
		s = splvm();
		pte = find_or_allocate_pte(pmap->space,
					   trunc_page(sva), FALSE);
		/* the PTE is locked now (if not NULL) */
		if (pte == NULL) {
		  *value = 0;
		} else {
		  register struct mapping *mp;
		  register struct phys_entry *pp;
		  int count = 0;
		  pp = pmap_find_physentry(pte->pte1.word);
		  count = 0;
		  for (mp = pp->phys_link; mp != NULL; mp = mp->next)
		    count++;
		  *value = (vm_offset_t) trunc_page(pte->pte1.word) | count;
		  UNLOCK_PTE(pte);
		}
		splx(s);
	  } else {
	    int total, shared;
	    total = 0;
	    shared = 0;
	    sva = trunc_page(sva);
	    s = splvm();
	    while (sva < eva) {
	      pte = find_or_allocate_pte(pmap->space,
					 sva, FALSE);
	      /* the PTE is locked now (if not NULL) */
	      if (pte != PTE_NULL) {
		register struct mapping *mp;
		register struct phys_entry *pp;
		pp = pmap_find_physentry(pte->pte1.word);
		mp = pp->phys_link;
		if (mp->next) shared++;
		total++;	/* Resident */
		UNLOCK_PTE(pte);
	      }
	      sva += PAGE_SIZE;
	    }
	    splx(s);
	    *value = (total << 16) | shared;
	  }
	  break;
	case MATTR_VAL_GET:		/* get current values */
	case MATTR_VAL_OFF:		/* turn attribute off */
	case MATTR_VAL_ON:		/* turn attribute on */
	default:
		ret = KERN_INVALID_ARGUMENT;
		break;
	}

	return ret;
}

/*
 * pmap_collect
 * 
 * Garbage collects the physical map system for pages that are no longer used.
 */
/* ARGSUSED */
void
pmap_collect(pmap_t pmap)
{

#if PMAP_LOWTRACE
	dbgTrace(0xF1D0000C, (unsigned int)pmap, 0);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_collect(pmap=%x)\n", pmap);
#endif
}

/*
 *	Routine:	pmap_activate
 *	Function:
 *		Binds the given physical map to the given
 *		processor, and returns a hardware map description.
 */
/* ARGSUSED */
void
pmap_activate(
	pmap_t pmap,
	thread_t th,
	int which_cpu)
{

#if PMAP_LOWTRACE
	dbgTrace(0xF1D0000D, (unsigned int)pmap, (unsigned int)th|which_cpu);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_activate(pmap=%x, th=%x, cpu=%x)\n",
		       pmap, th, which_cpu);
#endif
}

/* ARGSUSED */
void
pmap_deactivate(
	pmap_t pmap,
	thread_t th,
	int which_cpu)
{
#if PMAP_LOWTRACE
	dbgTrace(0xF1D0000E, (unsigned int)pmap, (unsigned int)th|which_cpu);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_activate(pmap=%x, th=%x, cpu=%x)\n",
		       pmap, th, which_cpu);
#endif
}

#if DEBUG

/*
 * pmap_zero_page
 * pmap_copy page
 * 
 * are implemented in movc.s, these
 * are just wrappers to help debugging
 */

extern void pmap_zero_page_assembler(vm_offset_t p);
extern void pmap_copy_page_assembler(vm_offset_t src, vm_offset_t dst);

/*
 * pmap_zero_page(pa)
 *
 * pmap_zero_page zeros the specified (machine independent) page pa.
 */
void
pmap_zero_page(
	vm_offset_t p)
{
	register struct mapping *mp;
	register struct phys_entry *pp;

	if (pmdebug & (PDB_USER|PDB_ZERO))
		printf("pmap_zero_page(pa=%x)\n", p);

	/*
	 * XXX can these happen?
	 */
	if (pmap_find_physentry(p) == PHYS_NULL)
		panic("zero_page: physaddr out of range");

	/* TODO NMGS optimisation - if page has no mappings, set bit in
	 * physentry to indicate 'needs zeroing', then zero page in
	 * pmap_enter or pmap_copy_page if bit is set. This keeps the
	 * cache 'hot'.
	 */

	pmap_zero_page_assembler(p);
}

/*
 * pmap_copy_page(src, dst)
 *
 * pmap_copy_page copies the specified (machine independent)
 * page from physical address src to physical address dst.
 *
 * We need to invalidate the cache for address dst before
 * we do the copy. Apparently there won't be any mappings
 * to the dst address normally.
 */
void
pmap_copy_page(
	vm_offset_t src,
	vm_offset_t dst)
{
	register struct phys_entry *pp;

	if (pmdebug & (PDB_USER|PDB_COPY))
		printf("pmap_copy_page(spa=%x, dpa=%x)\n", src, dst);
	if (pmdebug & PDB_COPY)
		printf("pmap_copy_page: phys_copy(%x, %x, %x)\n",
		       src, dst, PAGE_SIZE);

	pmap_copy_page_assembler(src, dst);
}
#endif /* DEBUG */

/*
 * pmap_pageable(pmap, s, e, pageable)
 *	Make the specified pages (by pmap, offset)
 *	pageable (or not) as requested.
 *
 *	A page which is not pageable may not take
 *	a fault; therefore, its page table entry
 *	must remain valid for the duration.
 *
 *	This routine is merely advisory; pmap_enter()
 *	will specify that these pages are to be wired
 *	down (or not) as appropriate.
 *
 *	(called from vm/vm_fault.c).
 */
/* ARGSUSED */
void
pmap_pageable(
	pmap_t		pmap,
	vm_offset_t	start,
	vm_offset_t	end,
	boolean_t	pageable)
{

#if PMAP_LOWTRACE
	dbgTrace(0xF1D0000F, (unsigned int)pmap|pageable, (unsigned int)(start|((end-start)>>12)));	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & (PDB_USER | PDB_WIRED))
		printf("pmap_pageable(pmap=%x, sva=%x, eva=%x, pageable=%s)\n",
		       pmap, start, end, pageable ? "TRUE" : "FALSE");
#endif
}

/*
 * pmap_modify_pages(pmap, s, e)
 *	sets the modified bit on all virtual addresses v in the 
 *	virtual address range determined by [s, e] and pmap,
 *	s and e must be on machine independent page boundaries and
 *	s must be less than or equal to e.
 */
void
pmap_modify_pages(
	     pmap_t pmap,
	     vm_offset_t sva, 
	     vm_offset_t eva)
{
	spl_t		spl;
	register pte_t	*pte;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00010, (unsigned int)pmap, (unsigned int)(sva|((eva-sva)>>12)));	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_modify_pages(pmap=%x, sva=%x, eva=%x)\n",
		       pmap, sva, eva);
#endif

	if (pmap == PMAP_NULL)
		return;

	PMAP_READ_LOCK(pmap, spl);

	for ( ; sva < eva; sva += PAGE_SIZE) {
		pte = find_or_allocate_pte(pmap->space, trunc_page(sva),FALSE);
		/* the PTE is now locked (if not NULL) */

		if (pte != PTE_NULL) {

		  	/* TODO NMGS - cheaper to find and modify physentry? */
		  
			/*  Perform a general case PTE update designed to work
			 *  in SMP configurations.
			 */
		  	assert(pte->pte0.bits.valid == TRUE);

			pte->pte0.bits.valid = FALSE;	/* Invalidate pte entry */
			sync();				/* Force updates to complete */
			simple_lock(&tlb_system_lock);
			tlbie(sva);			/* Wipe out this virtual address */
			simple_unlock(&tlb_system_lock);
			eieio();			/* Enforce ordering tlbie */
			TLBSYNC_SYNC();
			pte->pte1.bits.changed = TRUE;
			eieio();
		
			pte->pte0.bits.valid = TRUE;	/* Validate pte entry */
			sync();

			UNLOCK_PTE(pte);
		} else {
			printf("WARNING : pte missing for pmap_modify_pages\n");
		}
	}

	PMAP_READ_UNLOCK(pmap, spl);
}

/*
 * pmap_clear_modify(phys)
 *	clears the hardware modified ("dirty") bit for one
 *	machine independant page starting at the given
 *	physical address.  phys must be aligned on a machine
 *	independant page boundary.
 */
void
pmap_clear_modify(vm_offset_t pa)
{
	spl_t				spl;
	register struct phys_entry	*pp;
	register struct mapping		*mp;
	pte_t				*pte;
	register space_t		space;
	register vm_offset_t		offset;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00011, (unsigned int)pa, 0);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_clear_modify(pa=%x)\n", pa);
#endif

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return;

	/*
	 * Lock the pmap system first, since we will be changing
	 * several pmaps.
	 * No need to lock the phys_entry (pp) (pmap system is "write" locked).
	 */
	PMAP_WRITE_LOCK(spl);

	/*
	 * invalidate any pte entries and remove them from tlb
	 * TODO might it be faster to wipe all tlb entries outside loop? NO, rav.
	 */

	/* Remove any reference to modified to the physical page */
	pp->pte1.bits.changed = FALSE;

	/* mark PTE entries as having no modifies, and flush tlbs */
	for (mp = pp->phys_link; mp != NULL; mp = mp->next) {

		pte = MP2PTE(mp);
		space = MP_SPACE(mp);
		offset = mp->vm_info.bits.page << PPC_PGSHIFT;

		LOCK_PTE(pte);

		assert(pte->pte0.bits.valid == TRUE);

		/*	Perform a general case PTE update designed to work
		 *	in SMP configurations.
		 */
		pte->pte0.bits.valid = FALSE;
		sync();

		simple_lock(&tlb_system_lock);
		tlbie(offset);
		simple_unlock(&tlb_system_lock);
		eieio();
		TLBSYNC_SYNC();

		pte->pte1.bits.changed = FALSE;
		eieio();

		pte->pte0.bits.valid = TRUE;
		sync();

		UNLOCK_PTE(pte);
	}
	PMAP_WRITE_UNLOCK(spl);
}

/*
 * pmap_is_modified(phys)
 *	returns TRUE if the given physical page has been modified 
 *	since the last call to pmap_clear_modify().
 */
boolean_t
pmap_is_modified(register vm_offset_t pa)
{
	spl_t				spl;
	register struct phys_entry	*pp;
	register struct mapping		*mp;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00012, (unsigned int)pa, 0);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_is_modified(pa=%x)\n", pa);
#endif
	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return(FALSE);

	/*
	 * Lock the pmap system first, since we will be checking
	 * several pmaps.
	 */
	PMAP_WRITE_LOCK(spl);

	/* Check to see if we've already noted this as modified */
	if (pp->pte1.bits.changed) {
		PMAP_WRITE_UNLOCK(spl);
		return(TRUE);
	}

	/*
	 * No need to lock the phys_entry (pp), since we have the 
	 * pmap system "write" locked.
	 */

	/*	Make sure all TLB -> PTE updates have completed.
	*/
	sync();


	for (mp = pp->phys_link; mp != NULL; mp = mp->next) {

		pte_t *pte;
		assert(MP2PTE(mp)->pte0.bits.valid == TRUE);

		pte = MP2PTE(mp);
#if	NCPUS > 1 || MACH_LDEBUG
		LOCK_PTE(pte);
		/* Purge the current TLB entry for each mapping, to
		 * make sure that the mapping we have in memory
		 * reflects the true state of the entry
		 */
		pte->pte0.bits.valid = FALSE;
		sync();		/* Make sure invalid has completed */

		simple_lock(&tlb_system_lock);
		tlbie(mp->vm_info.bits.page << PPC_PGSHIFT);
		simple_unlock(&tlb_system_lock);
		eieio();
		TLBSYNC_SYNC();
#endif	/* NCPUS > 1 || MACH_LDEBUG */

		if (pte->pte1.bits.changed) {
			pp->pte1.bits.changed = TRUE;

#if	NCPUS > 1 || MACH_LDEBUG
			pte->pte0.bits.valid = TRUE;
			sync();
			UNLOCK_PTE(pte);
#endif	/* NCPUS > 1 || MACH_LDEBUG */
			PMAP_WRITE_UNLOCK(spl);
			return TRUE;
		}

#if	NCPUS > 1 || MACH_LDEBUG
		pte->pte0.bits.valid = TRUE;
		sync();
		UNLOCK_PTE(pte);
#endif	/* NCPUS > 1 || MACH_LDEBUG */
	}
	
	PMAP_WRITE_UNLOCK(spl);
	return FALSE;
}

/*
 * pmap_clear_reference(phys)
 *	clears the hardware referenced bit in the given machine
 *	independant physical page.  
 *
 */
void
pmap_clear_reference(vm_offset_t pa)
{
	spl_t				spl;
	register struct phys_entry	*pp;
	register struct mapping		*mp;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00013, (unsigned int)pa, 0);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_clear_reference(pa=%x)\n", pa);
#endif

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return;

	/*
	 * Lock the pmap system first, since we will be changing
	 * several pmaps.
	 */
	PMAP_WRITE_LOCK(spl);

	/*
	 * Flush any pte entries to ensure no referenced bits are set
	 */
	for (mp = pp->phys_link; mp != NULL; mp = mp->next) {
		pte_t *pte;

		pte = MP2PTE(mp);
		LOCK_PTE(pte);
		/*
		 * XXX
		 * we could avoid invalidating the PTE and the TLB
		 * if we changed only the byte containing the R bit
		 * (see section 7.6.3.2.2 of the Programming Environments).
		 */

		assert(pte->pte0.bits.valid == TRUE);

		/*	Perform a general case PTE update designed to work
		 *	in SMP configurations.
		*/
		pte->pte0.bits.valid = FALSE; /* Invalidate pte entry */
		sync();				/* Force updates to complete */

		simple_lock(&tlb_system_lock);
		tlbie(mp->vm_info.bits.page << PPC_PGSHIFT);
		simple_unlock(&tlb_system_lock);
		eieio();			/* Enforce ordering tlbie */
		TLBSYNC_SYNC();

		pte->pte1.bits.referenced = FALSE;
		eieio();

		pte->pte0.bits.valid = TRUE; /* Validate pte entry */
		sync();

		UNLOCK_PTE(pte);
	}

	/*	Mark the physical entry shadow copy as non-referenced too.
	*/
	pp->pte1.bits.referenced = FALSE;

	PMAP_WRITE_UNLOCK(spl);
}

/*
 * pmap_is_referenced(phys)
 *	returns TRUE if the given physical page has been referenced 
 *	since the last call to pmap_clear_reference().
 */
boolean_t
pmap_is_referenced(vm_offset_t pa)
{
	spl_t				spl;
	register struct phys_entry 	*pp;
	register struct mapping 	*mp;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00014, (unsigned int)pa, 0);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_USER)
		printf("pmap_is_referenced(pa=%x)\n", pa);
#endif
	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return(FALSE);

	PMAP_WRITE_LOCK(spl);

	if (pp->pte1.bits.referenced) {
		PMAP_WRITE_UNLOCK(spl);
		return (TRUE);
	}


	/*	Make sure all TLB -> PTE updates have completed.
	*/
	sync();

	/*	Check all the mappings to see if any one of them
		have recorded a reference to this physical page.
	*/
	for (mp = pp->phys_link; mp != NULL; mp = mp->next) {
		pte_t *pte;

		pte = MP2PTE(mp);
#if	NCPUS > 1 || MACH_LDEBUG
		/* Purge the current TLB entry for each mapping, to make
		 * sure that the mapping we have in memory reflects the
		 * true state of the entry
		 */
		LOCK_PTE(pte);
		assert(pte->pte0.bits.valid == TRUE);
		pte->pte0.bits.valid = FALSE;
		sync();

		simple_lock(&tlb_system_lock);
		tlbie(mp->vm_info.bits.page << PPC_PGSHIFT);
		simple_unlock(&tlb_system_lock);
		eieio();
		TLBSYNC_SYNC();
#endif	/* NCPUS > 1 || MACH_LDEBUG */

		if (pte->pte1.bits.referenced) {
			pp->pte1.bits.referenced = TRUE;

#if	NCPUS > 1 || MACH_LDEBUG
			pte->pte0.bits.valid = TRUE;
			sync();

			UNLOCK_PTE(pte);
#endif	/* NCPUS > 1 || MACH_LDEBUG */
			PMAP_WRITE_UNLOCK(spl);
			return (TRUE);
		}

#if	NCPUS > 1 || MACH_LDEBUG
		pte->pte0.bits.valid = TRUE;
		sync();
		UNLOCK_PTE(pte);
#endif	/* NCPUS > 1 || MACH_LDEBUG */
	}

	PMAP_WRITE_UNLOCK(spl);
	return(FALSE);
}

/*
 * Auxilliary routines for manipulating mappings
 */

/*
 * pmap_enter_mapping
 * Locking:
 *	in : pmap_system_lock (READ)
 *	     pmap lock
 *	     pte lock
 *	     pp lock
 *	out: nothing changed
 *	spl: VM
 */
/* static */
static struct mapping *
pmap_enter_mapping(pmap_t	pmap,
		   space_t	space,
		   vm_offset_t	va,
		   vm_offset_t	pa,
		   pte_t	*pte,
		   unsigned	prot,
		   struct phys_entry *pp)
{
	spl_t	spl;
	register struct mapping *mp;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00015, (unsigned int)pmap, (unsigned int)space);	/* (TEST/DEBUG) */
	dbgTrace(0xF1D04015, (unsigned int)va, (unsigned int)pa);	/* (TEST/DEBUG) */
	dbgTrace(0xF1D04015, (unsigned int)pte, (unsigned int)pp);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_MAPPING)
		printf("pmap_enter_mapping(pmap=%x, sp=%x, off=%x, &pte=%x, prot=%x)",
		       pmap, space, va, pte, prot);
#endif /* DEBUG */

	/* find the mapping that corresponds to the pte entry */
	mp = PTE2MP(pte);

	/* Can't change the wiring of something that is not mappable */
	assert(pmap_find_physentry(trunc_page(pte->pte1.word)) != PHYS_NULL);

	mp->vm_info.bits.wired = FALSE;
	mp->vm_info.bits.page  = (va >> PPC_PGSHIFT);
	
	/* Set up pte -
	 *   pte0 already contains everything except the valid bit.
	 *   pte1 already contains the protection information.
	 */

	assert(pte->pte0.word != PTE_EMPTY);

	pte->pte1.bits.phys_page = pa >> PPC_PGSHIFT;

	pte->pte1.bits.wimg = pp->pte1.bits.wimg; /* Set default cachability*/

	/*	Make sure those pte1 writes are ordered ahead of
		of the v bit.
	*/
	eieio();
	pte->pte0.bits.valid = TRUE;		/* Validate pte entry */
	sync();

#if MAPPING_HAS_PMAP_PTR
	mp->pmap = pmap;
#endif

	mp->next = pp->phys_link;
	pp->phys_link = mp;

#if	DEBUG
	/*	Assert a PTE of type WIMG_IO is not mapped to general RAM.
	*/
	if ((pmdebug & PDB_IO) && (pte->pte1.bits.wimg == PTE_WIMG_IO))
		assert ((((unsigned) pa) >= PCI_BASE) || IS_VIDEO_MEMORY(pa));

	if (pmdebug & PDB_MAPPING)
		printf(" -> %x\n");

#endif /* DEBUG */	
	return(mp);
}

/*
 * Locking:
 *	in : the phys_entry (pp) is locked,
 *	     the corresponding PTE (MP2PTE(mp)) is locked.
 *	out: the PTE is still locked,
 *	     the phys_entry is still locked.
 *	spl: VM
 */
/* static */
void
pmap_free_mapping(
	struct mapping		*mp,
	struct phys_entry	*pp)
{
	register space_t space;
	register vm_offset_t offset;
	struct mapping *mp2;
	pte_t *pte;
	int x;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00017, (unsigned int)mp, (unsigned int)pp);	/* (TEST/DEBUG) */
#endif
#if DEBUG
	if (pmdebug & PDB_MAPPING)
		printf("pmap_free_mapping(mp=%x), pte at 0x%x (0x%08x,0x%08x)\n",
		       mp, MP2PTE(mp), MP2PTE(mp)->pte0.word,MP2PTE(mp)->pte1.word);
#endif

	space = MP_SPACE(mp);
	offset = mp->vm_info.bits.page << PPC_PGSHIFT;
	pte = MP2PTE(mp);

	assert(pp != PHYS_NULL);

	pte->pte0.bits.valid = FALSE;
	sync();

	simple_lock(&tlb_system_lock);
	tlbie(offset);
	simple_unlock(&tlb_system_lock);
	eieio();
	TLBSYNC_SYNC();

	/* Reflect back the modify and reference bits for the pager */
	pp->pte1.word |= pte->pte1.word;

	/* Delete the mapping for the phys list */

	/* Find the mapping pointing to this one */
	mp2 = (struct mapping *)pp;
	while (mp2->next != mp)
		mp2 = mp2->next;
	/* And remove the mapping */
	mp2->next = mp->next;

	/* one day we can get sophisticated with the pte handling */
	/* remove_pte(pte); */

	/* for now, just remove the mapping */
	pte->pte0.word = PTE_EMPTY; /* Remove pte (v->p) lookup */
#if DEBUG
	pte->pte1.word = 0x12345678;
#endif
	sync();

	/* The mapping just lies unused */
}

/*
 * Deal with a hash-table pteg overflow. pmap_pteg_overflow is upcalled from
 * find_or_allocate_pte(), and assumes knowledge of the pteg hashing
 * structure
 */

static int pteg_overflow_counter=0;

static pte_t *pteg_pte_exchange(pte_t *pteg, pte0_t match);
pte_t *pmap_pteg_overflow(pte_t *primary_hash, pte0_t primary_match,
			  pte_t *secondary_hash, pte0_t secondary_match)
{
	pte_t *pte;

#if DEBUG
	int i;
#if PMAP_LOWTRACE
	dbgTrace(0xF1D00018, (unsigned int)primary_hash, *(unsigned int *)&primary_match);	/* (TEST/DEBUG) */
#endif
	if (pmdebug & PDB_PTEG) {
		printf("pteg overflow for ptegs 0x%08x and 0x%08x\n",
		       primary_hash,secondary_hash);
		printf("PRIMARY\t\t\t\t\tSECONDARY\n");
		for (i=0; i<8; i++) {
			printf("0x%08x : (0x%08x,0x%08x)\t"
			       "0x%08x : (0x%08x,0x%08x)\n",
			       &primary_hash[i],
			       primary_hash[i].pte0.word,
			       primary_hash[i].pte1.word,
			       &secondary_hash[i],
			       secondary_hash[i].pte0.word,
			       secondary_hash[i].pte1.word);
			assert(primary_hash[i].pte0.word != PTE_EMPTY);
			assert(secondary_hash[i].pte0.word != PTE_EMPTY);
		}
	}
#endif
	/* First try to replace an entry in the primary PTEG */
	pte = pteg_pte_exchange(primary_hash, primary_match);
	if (pte != PTE_NULL)
		return pte;
	pte = pteg_pte_exchange(secondary_hash, secondary_match);
	if (pte != PTE_NULL)
		return pte;

	panic("both ptegs are completely wired down\n");
	return PTE_NULL;
}
			
/* Used by the pmap_pteg_overflow function to scan through a pteg */
/*
 * Locking:
 *	spl: VM
 */
static pte_t *pteg_pte_exchange(pte_t *pteg, pte0_t match)
{
	/* Try and replace an entry in a pteg */

	int i;
	register struct phys_entry *pp;
	register struct mapping *mp;
	pte_t *pte;

	simple_lock(&pteg_overflow_lock);

	/* TODO NMGS pteg overflow algorithm needs working on!
	 * currently we just loop around the PTEGs rejecting
	 * the first available entry.
	 */

	for (i=0; i < 8; i++) {

		pte = &pteg[pteg_overflow_counter];
		pteg_overflow_counter = (pteg_overflow_counter+1) % 8;

		LOCK_PTE(pte);

		/* all mappings must be valid otherwise we wouldn't be here */
		assert(pte->pte0.bits.valid == TRUE);

		/* Take advantage of the fact that pte1 can be
		 * considered as a pointer to an address in
		 * the physical page when looking for physentry.
		 */
		pp = pmap_find_physentry(pte->pte1.word);

		/* pte entry must have reverse mapping, otherwise we
		 * cannot remove it (it was entered by pmap_map_bd)
		 */
		if (pp == PHYS_NULL) {
			UNLOCK_PTE(pte);
			continue;
		}

		LOCK_PHYSENTRY(pp);

		mp = PTE2MP(pte);

		/* if this entry isn't wired, isn't in the kernel space,
		 * and it doesn't have any special cachability attributes
		 * we can remove mapping without losing any information.
		 * XXX kernel space mustn't be touched as we may be doing
		 * XXX an I/O on a non-wired page, and we must not take
		 * XXX a page fault in an interrupt handler.
		 */
		/* TODO better implementation for cache info? */
		if (!mp->vm_info.bits.wired &&
		    MP_SPACE(mp) != PPC_SID_KERNEL &&
		    pte->pte1.bits.wimg == PTE_WIMG_DEFAULT) {
#if DEBUG
			if (pmdebug & PDB_PTEG) {
				printf("entry chosen at 0x%08x in %s PTEG\n",
				       pte,
				       pte->pte0.bits.hash_id ?
				       	"SECONDARY" : "PRIMARY");
				printf("throwing away mapping from sp 0x%x, "
				       "virtual 0x%08x to physical 0x%08x\n",
				       MP_SPACE(mp),
				       mp->vm_info.bits.page << PPC_PGSHIFT,
				       trunc_page(MP2PTE(mp)->pte1.word));
				printf("pteg_overflow_counter=%d\n",
				       pteg_overflow_counter);
				printf("pte0.vsid=0x%08x\n",
				       pte->pte0.bits.segment_id);
			}
#endif /* DEBUG */
			pmap_get_pmap(mp)->stats.resident_count--;

			/* release mapping structure and PTE, keeping
			 * track of changed and referenced bits
			 */
			pmap_free_mapping(mp, pp);

			/* Replace the pte entry by the new one */

			match.bits.valid = FALSE;
			pte->pte0 = match;
			sync();

			UNLOCK_PHYSENTRY(pp);
			simple_unlock(&pteg_overflow_lock);
			/* return with the PTE locked */
			return pte;
		}
		UNLOCK_PTE(pte);
		UNLOCK_PHYSENTRY(pp);
	}

	simple_unlock(&pteg_overflow_lock);
	return PTE_NULL;
}

#if MAPPING_HAS_PMAP_PTR == 0
static void
pmap_stash_init(void)
{
	int 	i;

	for (i = 0; i < PMAP_STASH_SIZE; i++)
		queue_init(&pmap_stash[i]);
}

/*
 * in:  locked
 * out: locked
 */
static void
pmap_add_pmap(pmap_t pmap)
{
	int 	hash = pmap_stash_hash(pmap->space);

	queue_enter_first(&pmap_stash[hash], pmap, pmap_t, pmap_link);
}

/*
 * out: locked
 */
static pmap_t
pmap_get_pmap(struct mapping *mp)
{
	pmap_t	pmap;
	int 	hash = pmap_stash_hash(MP_SPACE(mp));

	if (space == PPC_SID_KERNEL) {
		return(kernel_pmap);
	}
	
	queue_iterate(&pmap_stash[hash], pmap, pmap_t, pmap_link) {
		if (pmap->space == space) {
			return(pmap);
		}
	}
	panic("pmap_get_pmap: could not find pmap for space %d\n", space);
	return (pmap_t)0;
}

/*
 * in:  locked
 * out: locked
 */
static void
pmap_rem_pmap(pmap_t pmap)
{
	int 	hash = pmap_stash_hash(pmap->space);

	queue_remove(&pmap_stash[hash], pmap, pmap_t, pmap_link);
}
#endif	/* MAPPING_HAS_PMAP_PTR */

/*
 * Collect un-wired mappings. How best to do this??
 */
/* static */
void
pmap_reap_mappings(void)
{
#if DEBUG
	if (pmdebug & PDB_MAPPING)
		printf("pmap_reap_mappings()\n");
#endif

	panic("out of mapping structs");
}

#if	MACH_VM_DEBUG
int
pmap_list_resident_pages(
	register pmap_t		pmap,
	register vm_offset_t	*listp,
	register int		space)
{
	return 0;
}
#endif	/* MACH_VM_DEBUG */

/*
 * Locking:
 *	spl: VM
 */
void
pmap_copy_part_page(
	vm_offset_t	src,
	vm_offset_t	src_offset,
	vm_offset_t	dst,
	vm_offset_t	dst_offset,
	vm_size_t	len)
{
	register struct phys_entry *pp_src, *pp_dst;
	spl_t	s;


#if PMAP_LOWTRACE
	dbgTrace(0xF1D00019, (unsigned int)src+src_offset, (unsigned int)dst+dst_offset);	/* (TEST/DEBUG) */
	dbgTrace(0xF1D04019, (unsigned int)len, 0);	/* (TEST/DEBUG) */
#endif
	s = splvm();

        assert(((dst & PAGE_MASK)+dst_offset+len) <= PAGE_SIZE);
        assert(((src & PAGE_MASK)+src_offset+len) <= PAGE_SIZE);

#ifdef	TLB_ZERO
	/* for as-yet unused TLB_ZERO - lazily zero the page if needed */
	pp_dst = pmap_find_physentry(dst);
	if (pp_dst != PHYS_NULL) {
		LOCK_PHYSENTRY(pp_dst);
		if (pp_dst->tlbprot & TLB_ZERO) {
			phys_bzero(trunc_page(dst), PAGE_SIZE);
			pp_dst->tlbprot &= ~TLB_ZERO;
		}
		UNLOCK_PHYSENTRY(pp_dst);
	}
#endif

	/*
	 * Since the source and destination are physical addresses, 
	 * turn off data translation to perform a  bcopy() in bcopy_phys().
	 */
	phys_copy((vm_offset_t) src+src_offset,
		  (vm_offset_t) dst+dst_offset, len);

	splx(s);
}

void
pmap_zero_part_page(
	vm_offset_t	p,
	vm_offset_t     offset,
	vm_size_t       len)
{
    panic("pmap_zero_part_page");
}

/*
 *	kvtophys(addr)
 *
 *	Convert a kernel virtual address to a physical address
 */
vm_offset_t
kvtophys(
	vm_offset_t va)
{
	pte_t		*pte;
	vm_offset_t	pa;
	spl_t		s;
	
	s = splhigh();
	pte = find_or_allocate_pte(PPC_SID_KERNEL, trunc_page(va), FALSE);
	/* the PTE is now locked (if not NULL) */
	if (pte == NULL) {
		splx(s);
		return(0);
	}
	pa = trunc_page(pte->pte1.word) | (va & page_mask);
	UNLOCK_PTE(pte);
	splx(s);

	return pa;
}

/*
 *	phystokv(addr)
 *
 *	Convert a physical address to a kernel virtual address if
 *	there is a mapping, otherwise return NULL
 */
vm_offset_t
phystokv(
	vm_offset_t pa)
{
	struct phys_entry	*pp;
	struct mapping	 	*mp;
	vm_offset_t		va;
	spl_t			s;

	s = splvm();
	pp = pmap_find_physentry(pa);
	if (pp != PHYS_NULL) {
		LOCK_PHYSENTRY(pp);
		for (mp = pp->phys_link; mp != NULL; mp = mp->next) {
			if (MP_SPACE(mp) == PPC_SID_KERNEL) {
				va = mp->vm_info.bits.page << PPC_PGSHIFT |
					(pa & (PAGE_SIZE-1));
				UNLOCK_PHYSENTRY(pp);
				splx(s);
				return va;
			}
		}
		UNLOCK_PHYSENTRY(pp);
	}
	splx(s);
	return (vm_offset_t)NULL;
}

#if DEBUG
/* This function is included to test compiler functionality, it does
 * a simple slide through the bitfields of a pte to check
 * minimally correct bitfield behaviour. We need bitfields to work!
 */
#define TEST(PTE0,PTE1)							\
	if ((pte.pte0.word != (unsigned int)PTE0) ||			\
	    (pte.pte1.word != (unsigned int)PTE1)) {			\
		printf("BITFIELD TEST FAILED AT LINE %d\n",__LINE__);	\
		printf("EXPECTING 0x%08x and 0x%08x\n",			\
		       (unsigned int)PTE0,(unsigned int)PTE1);		\
		printf("GOT       0x%08x and 0x%08x\n",			\
		       pte.pte0.word,pte.pte1.word);			\
		return 1;						\
	}

extern int pmap_test_bitfields(void); /* Prototyped also in go.c */
int pmap_test_bitfields(void)
{
	pte_t	pte;
	pte.pte0.word = 0;
	pte.pte1.word = 0;

	pte.pte0.bits.segment_id = 0x103; TEST(0x103 << 7, 0);
	pte.pte0.bits.valid = 1;
	TEST((unsigned int)(0x80000000U| 0x103<<7), 0);
	pte.pte0.bits.hash_id = 1;
	TEST((unsigned int)(0x80000000U | 0x103 << 7 | 1<<6), 0);
	pte.pte0.bits.page_index = 3;
	TEST((unsigned int)(0x80000000U|0x103<<7|1<<6|3), 0);

	pte.pte0.bits.segment_id = 0;
	TEST((unsigned int)(0x80000000U|1<<6|3), 0);
	pte.pte0.bits.page_index = 0;
	TEST((unsigned int)(0x80000000U|1<<6), 0);
	pte.pte0.bits.valid = 0;
	TEST(1<<6, 0);

	pte.pte1.bits.referenced = 1; TEST(1<<6, 1<<8);
	pte.pte1.bits.protection = 3; TEST(1<<6, (unsigned int)(1<<8 | 3));
	pte.pte1.bits.changed = 1; TEST(1<<6,(unsigned int)(1<<8|1<<7|3));
	pte.pte1.bits.phys_page = 0xfffff;
	TEST(1<<6,(unsigned int)(0xfffff000U|1<<8|1<<7|3));

	pte.pte1.bits.changed = 0;
	TEST(1<<6,(unsigned int)(0xfffff000U|1<<8|3));

	return 0;
}
#endif /* DEBUG */


#if	MACH_KDB
/* TODO NMGS REMOVE THIS */
/*
 * pmap_debug_page(phys)
 * displays mappings for a given physical page
 */
void pmap_debug_page(vm_offset_t pa);

void
pmap_debug_page(vm_offset_t pa)

{
	spl_t				spl;
	register struct phys_entry	*pp;
	register struct mapping		*mp;

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return;

	/*
	 * Lock the pmap system first, since we will be changing
	 * several pmaps.
	 */

	for (mp = pp->phys_link; mp != MAPPING_NULL; mp = mp->next) {
		db_printf("phys 0x%08x : pmap=0x%x virt 0x%08x, hw prot=%d\n",
			pa,
			pmap_get_pmap(mp),
			(mp->vm_info.bits.page * PAGE_SIZE),
			MP2PTE(mp)->pte1.bits.protection);

	}
}
#endif /* MACH_KDB */

