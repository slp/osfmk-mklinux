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
 * Copyright (c) 1990,1991,1992,1993,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).
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
 * includes software developed by the Computer Systems Laboratory at
 * the University of Utah.''
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * Carnegie Mellon requests users of this software to return to
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 * 	Utah $Hdr: pmap.c 1.49 94/12/15$
 *	Author: Mike Hibler, Bob Wheeler, University of Utah CSL, 10/90
 */
 
/*
 *	Manages physical address maps for hppa.
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

/*
 * CAVAETS:
 *
 *	PAGE_SIZE must equal HP700_PGBYTES
 *	Needs more work for MP support
 */

#include <mach_assert.h>
#include <mach_vm_debug.h>
#include <mach_rt.h>

#include <kern/lock.h>
#include <kern/thread.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <machine/regs.h>
#include <kern/spl.h>
#include <machine/cpu.h>

#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/c_support.h>
#include <machine/pmap.h>
#include <hp_pa/pte.h>
#include <hp_pa/physmem.h>

#if defined(EQUIV_HACK) || defined(IO_HACK)
/*
 * We do not maintain mapping structures for equivalently mapped memory,
 * the theory being that we can easily construct TLB entries at miss time.
 * All we need to know is the virtual address and a protection.  The former
 * is part of the exception information, the latter is provided by the
 * following variables:
 *
 *	[ io_end - 0 )			KRW
 *	[ 0 - text_end )		KRX
 *	[ text_end - equiv_end )	KRW
 *
 * Note that this means we cannot multiply-map an address less than equiv_end
 * due to a lack of auxiliary information normally contained in the mapping
 * structure [XXX not entirely true anymore].
 */

extern vm_offset_t	avail_end;
extern vm_offset_t	virtual_avail;
extern vm_offset_t	virtual_end;
vm_offset_t		avail_next;
unsigned int		avail_remaining;

#endif
long equiv_end = 0;
long io_end = 0;

/* externs */
extern void disable_S_sid_hashing(void);
extern void disable_L_sid_hashing(void);
extern void disable_T_sid_hashing(void);
extern void phys_bzero(vm_offset_t, vm_size_t);
extern void phys_bcopy(vm_offset_t, vm_offset_t, vm_size_t);
extern void store_phys(vm_offset_t, vm_offset_t, vm_size_t);
extern void lpage_zero(unsigned, space_t, vm_offset_t);
extern void lpage_copy(unsigned, space_t, vm_offset_t, vm_offset_t);
extern void phys_page_copy(vm_offset_t, vm_offset_t);
extern void bcopy_phys(vm_offset_t, vm_offset_t, int);
extern void bcopy_lphys(vm_offset_t, vm_offset_t, int);
extern void bcopy_rphys(vm_offset_t, vm_offset_t, int);
extern void bzero_phys(vm_offset_t, int);

/* forward */
void pmap_activate(pmap_t pmap, thread_t th, int which_cpu);
void pmap_deactivate(pmap_t pmap, thread_t th, int which_cpu);

static void pmap_stash_init(void);
static void pmap_add_pmap(pmap_t);
static void pmap_rem_pmap(pmap_t);
static pmap_t pmap_get_pmap(space_t);
static void pmap_flush_writers(struct phys_entry *);

static struct mapping *pmap_find_mapping(space_t, vm_offset_t);
static void pmap_enter_mapping(struct mapping *);
static void pmap_free_mapping(struct mapping *);
static void pmap_clear_mappings(vm_offset_t, struct mapping *);

static __inline__ void pmap_clear_cache(space_t space, vm_offset_t offset);

#if MACH_VM_DEBUG
int pmap_list_resident_pages(pmap_t pmap, vm_offset_t *listp, int space);
#endif

struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;
struct zone	*pmap_zone;		/* zone of pmap structures */
boolean_t	pmap_initialized = FALSE;

#ifdef USEALIGNMENT
/*
 * Mask to determine if two VAs naturally align in the data cache:
 *	834/835:	0 (cannot do)
 *	720/730/750:	256k
 *	705/710:	64k
 *      715:            64k
 * Note that this is a mask only for the data cache since we are
 * only attempting to prevent excess cache flushing of RW data.
 */
vm_offset_t	pmap_alignmask;

#define pmap_aligned(va1, va2) \
	(((va1) & pmap_alignmask) == ((va2) & pmap_alignmask))
#endif

/*
 * Virtual-to-physical translations are handled with hashed lists of "mapping"
 * structures.  The hash table is vtop_table.
 */
struct vtop_entry *vtop_table;

/*
 * Physical-to-virtual translations are handled by an inverted page table
 * structure, phys_table.  Multiple mappings of a single page are handled
 * by linking the affected mapping structures.
 */
struct phys_entry *phys_table;

#ifdef	HPT
/*
 * Hashed (Hardware) Page Table, for use as a software cache, or as a
 * hardware accessible cache on machines with hardware TLB walkers.
 */
struct hpt_entry *hpt_table;
int	usehpt = 1;
#endif

/*
 * First and last physical addresses that we maintain any information for.
 * These provide the range of addresses represented in phys_table.
 * Note that they are long as vm_first_phys may be negative.
 */
long	vm_first_phys = 0;
long	vm_last_phys  = 0;

/*
 * Function to get index into phys_table for a given physical address
 */
#define PHYS_INDEX(pa)	(((pa) - (vm_offset_t)vm_first_phys) >> HP700_PGSHIFT)

#define PHYS_INRANGE(pa) \
	((long)(pa) >= vm_first_phys && (long)(pa) < vm_last_phys)
#define pmap_find_physentry(pa)	\
	(PHYS_INRANGE(pa) ? &phys_table[PHYS_INDEX(pa)] : PHYS_NULL)

/*
 * Convert a TLB ready page number to a physical address and visa versa
 *	PA 1.0:		pn << 4 implies pa >> 7 since pageshift == 11
 *	PA 1.1:		pn << 5 implies pa >> 7 since pageshift == 12
 */
#define tlbtopa(tlbpn)	((vm_offset_t)(tlbpn) << 7)
#define patotlb(pa)	((vm_offset_t)(pa) >> 7)

/*
 * XXX use mpqueue_head_t's??
 * (no one else does, are they coming or going?)
 */
queue_head_t		free_mapping;	   /* list of free mapping structs */
decl_simple_lock_data(,free_mapping_lock) /* and lock */

unsigned int		free_pmap_max = 32;/* Max number of free pmap */
unsigned int		free_pmap_cur;   /* Cur number of free pmap */
pmap_t			free_pmap;	   /* list of free pmaps */
decl_simple_lock_data(,free_pmap_lock)   /* and lock */

decl_simple_lock_data(,pmap_lock)	 /* XXX this is all broken */
decl_simple_lock_data(,sid_pid_lock)     /* pids */

/*
 * Mappings for the kernel pmap are managed through the free mapping
 * queue that is initialized at pmap bootstrap, while other mappings
 * are managed via the physmem module.
 */
struct physmem_queue	pmap_mapping;	/* queue of used pmap mapping structs */

int		pages_per_vm_page;
unsigned int	kern_prot[8], user_prot[8];
unsigned int	pid_counter;

#define	pmap_sid(pmap, va) \
	(((va & 0xc0000000) != 0xc0000000) ? pmap->space : HP700_SID_KERNEL)

#define pmap_prot(pmap, prot)	\
	(((pmap) == kernel_pmap ? kern_prot : user_prot)[prot]) 

/*
 * hp700_protection_init()
 *	Initialize the user/kern_prot_codes[] arrays which are 
 *	used to translate machine independent protection codes
 *	to HP-PA protection codes.
 */
void
hp700_protection_init(void)
{
	register unsigned *kp, *up;
	register int prot;

	kp = kern_prot;
	up = user_prot;
	for (prot = 0; prot < 8; prot++) {
		switch (prot) {
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE:
			*kp++ = TLB_AR_NA;
			*up++ = TLB_AR_NA;
			break;
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_NONE:
#if 1
			*kp++ = TLB_AR_KRX;
			*up++ = TLB_AR_URX;
#else
			*kp++ = TLB_AR_KR;
			*up++ = TLB_AR_UR;
#endif
			break;
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_EXECUTE:
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_EXECUTE:
			*kp++ = TLB_AR_KRX;
			*up++ = TLB_AR_URX;
			break;
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE:
#if 1
			*kp++ = TLB_AR_KRWX;
			*up++ = TLB_AR_URWX;
#else
			*kp++ = TLB_AR_KRW;
			*up++ = TLB_AR_URW;
#endif
			break;
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE:
			*kp++ = TLB_AR_KRWX;
			*up++ = TLB_AR_URWX;
			break;
		}
	}
}

/*
 * pmap_map(va, spa, epa, prot)
 *	is called during boot to map memory in the kernel's address map.
 *	A virtual address range starting at "va" is mapped to the physical
 *	address range "spa" to "epa" with machine independent protection
 *	"prot".
 *
 *	"va", "spa", and "epa" are byte addresses and must be on machine
 *	indpendent page boundaries.
 */
vm_offset_t
pmap_map(
	vm_offset_t va,
	vm_offset_t spa,
	vm_offset_t epa,
	vm_prot_t prot)
{
#ifndef IO_HACK
	boolean_t wrapped = FALSE;

	if (spa == epa)
		return(va);
	if (epa == 0) {
		wrapped = TRUE;
		epa = trunc_page(~0);
	}
#endif

	assert((prot & ~VM_PROT_ALL) == 0);
	while (spa < epa) {
#ifdef EQUIV_HACK
		if ((long)va >= (long)vm_first_phys && (long)va < equiv_end) {
			register struct phys_entry *pp;
			
			if (va != spa)
				panic("pmap_map: mapping equivalent address");
			/*
			 * equivalently mapped, just add protection to page
			 */
			pp = pmap_find_physentry(spa);
#ifndef IO_HACK
			if (wrapped)
			pp->tlbprot = pmap_prot(kernel_pmap, prot) | TLB_DIRTY;
			else
#endif
			pp->tlbprot =
			    pmap_prot(kernel_pmap, prot) | kernel_pmap->pid;
		} else
#endif
			pmap_enter(kernel_pmap, va, spa, prot, TRUE);

		va += PAGE_SIZE;
		spa += PAGE_SIZE;
	}

#ifndef IO_HACK
	if (wrapped) {
		if ((long)va >= (long)vm_first_phys && (long)va < equiv_end) {
			register struct phys_entry *pp;
			
			if (va != spa)
				panic("pmap_map: mapping equivalent address");
			/*
			 * equivalently mapped, just add protection to page
			 */
			pp = pmap_find_physentry(spa);
#if 1
			if (1)
			pp->tlbprot = pmap_prot(kernel_pmap, prot) | TLB_DIRTY;
			else
#endif
			pp->tlbprot =
			    pmap_prot(kernel_pmap, prot) | kernel_pmap->pid;
		} else
			pmap_enter(kernel_pmap, va, spa, prot, TRUE);
		va += PAGE_SIZE;
	}
#endif
	return(va);
}

#ifdef	BTLB
/*
 * pmap_block_map(pa, size, prot, entry, dtlb)
 *    Block map a physical region. Size must be a power of 2. Address must
 *    must be aligned to the size. Entry is the block TLB entry to use.
 *
 *    S-CHIP: Entries 0,2 and 1,3 must be the same size for each TLB type. 
 */
void
pmap_block_map(
	vm_offset_t pa,
	vm_size_t size,
	vm_prot_t prot,
	int entry, int tlbflag)
{
	unsigned tlbprot;
	
	assert((prot & ~VM_PROT_ALL) == 0);
	tlbprot = pmap_prot(kernel_pmap, prot) | kernel_pmap->pid;
	switch (tlbflag) {
		
	case BLK_ICACHE:
  		insert_block_itlb(entry, pa, size, tlbprot);
		break;
	case BLK_DCACHE:
		insert_block_dtlb(entry, pa, size, tlbprot);
		break;
	case BLK_COMBINED:
		insert_block_ctlb(entry, pa, size, tlbprot);
		break;
	case BLK_LCOMBINED:
		insert_L_block_ctlb(entry, pa, size, tlbprot);
		break;
	default:
		printf("pmap_block_map routine: unknown flag %d\n",tlbflag);
	}
}
#endif

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Map the kernel's code and data, and allocate the system page table.
 *	Called with mapping OFF.  Page_size must already be set.
 *
 *	Parameters:
 *	avail_start	PA of first available physical page
 *	avail_end	PA of last available physical page
 */
void
pmap_bootstrap(vm_offset_t *vstart, vm_offset_t *vend)
{
	register struct mapping *mp;
	vm_offset_t addr, eaddr, naddr;
	vm_size_t size, asize;
	int i, num;
	extern int etext;
	extern unsigned int io_size;
#ifdef USEALIGNMENT
	extern int cputype, dcache_size;
#endif

#ifdef IO_HACK
	vm_first_phys = 0;
#else
	vm_first_phys = (long) trunc_page(io_size);
#endif
	vm_last_phys = (long) *vend;

	pages_per_vm_page = PAGE_SIZE / HP700_PGBYTES;
	/* XXX for now */
	if (pages_per_vm_page != 1)
		panic("HP700 page != MACH page");

	hp700_protection_init();
	pmap_stash_init();

	/*
	 * Initialize kernel pmap
	 */
	kernel_pmap = &kernel_pmap_store;
#if	NCPUS > 1
	lock_init(&pmap_lock, FALSE, ETAP_VM_PMAP_SYS, ETAP_VM_PMAP_SYS_I);
#endif	/* NCPUS > 1 */
	simple_lock_init(&kernel_pmap->lock, ETAP_VM_PMAP_KERNEL);
	simple_lock_init(&free_pmap_lock, ETAP_VM_PMAP_FREE);
	simple_lock_init(&sid_pid_lock, ETAP_VM_PMAP_FREE);

	kernel_pmap->ref_count = 1;
	kernel_pmap->space = HP700_SID_KERNEL;
	kernel_pmap->pid = HP700_PID_KERNEL;
	kernel_pmap->next = PMAP_NULL;

	/*
	 * Allocate various tables and structures.
	 */
	addr = *vstart;

	/*
	 * Allocate the physical to virtual table.
	 */
	addr = cache_align(addr);
	phys_table = (struct phys_entry *) addr;
	
	num = atop(vm_last_phys - vm_first_phys);

	bzero((char*)phys_table, num * sizeof(struct phys_entry));
	for (i = 0; i < num; i++)
		queue_init(&phys_table[i].phys_link);
	addr = (vm_offset_t) &phys_table[num];

	/*
	 * load cr24 with the address of the phys_table entry for virtual
	 * address 0.
	 */
	mtctl(CR_PTOV, (unsigned) &phys_table[PHYS_INDEX((vm_offset_t) 0)]);

	/*
	 * Allocate the virtual to physical table.
	 */
	addr = cache_align(addr);
	vtop_table = (struct vtop_entry *) addr;

	bzero((char*)vtop_table,
	      HP700_HASHSIZE * sizeof (struct vtop_entry));
	for (i = 0; i < HP700_HASHSIZE; i++)
		queue_init(&vtop_table[i].hash_link);
	addr = (vm_offset_t) &vtop_table[HP700_HASHSIZE];

	/*
	 * load cr25 with the address of the hash table
	 */
	mtctl(CR_VTOP, (unsigned) vtop_table);

	/*
	 * Allocate at least the required number of mapping entries for the 
	 * kernel space. (XXX can't use a zone since zone package hasn't been
	 * initialized yet). We use any excess space (e.g. as required for
	 * alignment of the HPT) for mapping structures as well.
	 */
	addr = cache_align(addr);
	size = sizeof(struct mapping) * num;
#ifdef HPT
	/*
	 * HW requires HPT to be sligned to its size, so allocate mapping
	 * structs up to that boundary.
	 */
	if (usehpt)
		asize = sizeof(struct hpt_entry) * HP700_HASHSIZE;
	else
#endif
	asize = PAGE_SIZE;
	size = (((addr + size + asize - 1) / asize) * asize) - addr;
	num  = size / sizeof(struct mapping);

	mp = (struct mapping *) addr;

	queue_init(&free_mapping);

	simple_lock_init(&free_mapping_lock, ETAP_VM_PMAP_FREE);

	while (num--) {
		mp->space = -1;
		mp->offset = 0;
		mp->tlbpage = 0;
		mp->tlbprot = 0;
		mp->tlbsw = 0;
		queue_enter(&free_mapping, mp, struct mapping *, hash_link);
		mp++;
	}

	addr = round_page((vm_offset_t)mp);

#ifdef	HPT
	/*
	 * Allocate the HPT.
	 * We set the software pointer in each entry to the equivalent entry
	 * in the VTOP table. This is for the benefit of the TLB handlers,
	 * which would have to compute the new address anyway on a HPT
	 * cache miss. It means reading a word from memory, but I think its
	 * a wash, since the HPT entry is going to be in the data cache.
	 */
	size = sizeof(struct hpt_entry) * HP700_HASHSIZE;
	if (usehpt && (addr & (size - 1)))
		panic("HPT not aligned correctly!");

	hpt_table = (struct hpt_entry *) addr;
	for (i = 0; i < HP700_HASHSIZE; i++) {
		hpt_table[i].valid      = 0;
		hpt_table[i].vpn        = 0;
		hpt_table[i].space      = -1;
		hpt_table[i].tlbpage    = 0;
		hpt_table[i].tlbprot    = 0;
		hpt_table[i].vtop_entry = (unsigned) &vtop_table[i];
	}
	addr = round_page((vm_offset_t)&hpt_table[HP700_HASHSIZE]);
	
	/*
	 * load cr25 with the address of the HPT table
	 *
	 * NB: It sez CR_VTOP, but we (and the TLB handlers) know better ...
	 */
	mtctl(CR_VTOP, (unsigned) hpt_table);
#endif

	*vstart = addr;
	avail_remaining = atop(*vend - *vstart);
	avail_next = *vstart;

#ifdef USEALIGNMENT
	/*
	 * If we can take advantage of natural alignments in the cache
	 * set that up now.
	 */
	pmap_alignmask = dcache_size - 1;
#endif

#ifdef BTLB
	switch (cputype) {
	case CPU_PCXS:
                disable_S_sid_hashing();
		break;

	case CPU_PCXL:
		disable_L_sid_hashing();   
		break;

	case CPU_PCXT:
		disable_T_sid_hashing();  
		break;
	default:
		panic("unknown cpu type %d.", cputype);
	}
#endif

        /*
	 * Load the kernel's PID
	 */
	mtctl(CR_PIDR1, HP700_PID_KERNEL);

	/*
	 * Initialize physmem module
	 */
	physmem_init();
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

	physmem_add(&pmap_mapping, sizeof (struct mapping), ETAP_VM_PMAP_FREE);

	pid_counter = HP700_PID_KERNEL + 2;

	pmap_initialized = TRUE;
}

unsigned int
pmap_free_pages(void)
{
	return avail_remaining;
}

boolean_t
pmap_next_page(vm_offset_t *addrp)
{
	if (avail_next == avail_end)
		return FALSE;

	*addrp = avail_next;
	avail_next += PAGE_SIZE;
	avail_remaining--;
	return TRUE;
}

void 
pmap_virtual_space(
	vm_offset_t *startp,
	vm_offset_t *endp)
{
	*startp = virtual_avail;
	*endp = virtual_end;
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
	int pid;
	space_t sid;

	/*
	 * A software use-only map doesn't even need a pmap structure.
	 */
	if (size) 
		return(PMAP_NULL);

	/* 
	 * If there is a pmap in the pmap free list, reuse it. 
	 */
	simple_lock(&free_pmap_lock);
	if (free_pmap != PMAP_NULL) {
		pmap = free_pmap;
		free_pmap = pmap->next;
		assert(free_pmap_cur > 0);
		free_pmap_cur--;
	} else {
		assert(free_pmap_cur == 0);
		pmap = PMAP_NULL;
	}
	simple_unlock(&free_pmap_lock);

	/*
	 * Couldn't find a pmap on the free list, try to allocate a new one.
	 */
	if (pmap == PMAP_NULL) {
		pmap = (pmap_t) zalloc(pmap_zone);
		if (pmap == PMAP_NULL) {
			return(PMAP_NULL);
		}

		/*
		 * Allocate space and protection IDs for the pmap.
		 * If all are allocated, there is nothing we can do.
		 */

		simple_lock(&sid_pid_lock);
		if (pid_counter <= MAX_PID) {
			pid = pid_counter;
			pid_counter += 2;
		} else
			pid = 0;
		simple_unlock(&sid_pid_lock);

		if (pid == 0) {
			printf("Warning no more pmap id\n");
			return(PMAP_NULL);
		}

		/* 
		 * Initialize the sids and pid. This is a user pmap, so 
		 * sr4, sr5, and sr6 are identical. sr7 is always KERNEL_SID.
		 */
		pmap->pid = pid;
		simple_lock_init(&pmap->lock, ETAP_VM_PMAP);
	}

	simple_lock(&pmap->lock);
	pmap->space = pmap->pid;
	pmap->ref_count = 1;
	pmap->next = PMAP_NULL;
	pmap->stats.resident_count = 0;
	pmap->stats.wired_count = 0;
	pmap_add_pmap(pmap);
	simple_unlock(&pmap->lock);

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

	if (pmap == PMAP_NULL)
		return;

	simple_lock(&pmap->lock);

	ref_count = --pmap->ref_count;

	if (ref_count < 0)
		panic("pmap_destroy(): ref_count < 0");
	if (ref_count > 0) {
		simple_unlock(&pmap->lock);
		return;
	}

	assert(pmap->stats.resident_count == 0);

	pmap_rem_pmap(pmap);
	simple_unlock(&pmap->lock);

	/* 
	 * Add the pmap to the pmap free list or to the pmap_zone
	 */
	simple_lock(&free_pmap_lock);
	if (free_pmap_cur < free_pmap_max) {
		pmap->next = free_pmap;
		free_pmap = pmap;
		free_pmap_cur++;
		simple_unlock(&free_pmap_lock);
	} else {
	    simple_unlock(&free_pmap_lock);
	    zfree(pmap_zone, (vm_offset_t)pmap);
	}
}

/*
 * pmap_reference(pmap)
 *	gains a reference to the specified pmap.
 */
void
pmap_reference(pmap_t pmap)
{
	if (pmap != PMAP_NULL) {
		simple_lock(&pmap->lock);
		pmap->ref_count++;
		simple_unlock(&pmap->lock);
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
	space_t space;
	struct mapping *mp;

	assert(sva <= eva);

	if(pmap == PMAP_NULL)
		return;

	simple_lock(&pmap->lock);

	space = pmap_sid(pmap, sva);

	while (pmap->stats.resident_count && ((sva < eva))) {
		if (mp = pmap_find_mapping(space, sva)) {
			assert(space == mp->space);
			pmap->stats.resident_count--;
			pmap_free_mapping(mp);
		}
		sva += PAGE_SIZE;
	}

	simple_unlock(&pmap->lock);
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
	register struct phys_entry *pp;
	register struct mapping *mp;
	unsigned tlbprot;
	boolean_t remove;
	spl_t s;

	assert((prot & ~VM_PROT_ALL) == 0);
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

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL) {
		return;
	}

	if(remove) {
		pmap_t pmap;

		s = splvm();
		while (!queue_empty(&pp->phys_link)) {
			mp = (struct mapping *) queue_first(&pp->phys_link);
			pmap = pmap_get_pmap(mp->space);
			pmap->stats.resident_count--;
			pmap_free_mapping(mp);
			simple_unlock(&pmap->lock);
		}
		splx(s);
		return;
	}

	s = splvm();
	/*
	 * Modify mappings if necessary.  If waswriter is true then there is
	 * no need to modify the actual TLBs since the writer would have the
	 * only valid mapping in the TLB and it has already been flushed above.
	 */
	queue_iterate(&pp->phys_link, mp, struct mapping *, phys_link) {
		/*
		 * Compare new protection with old to see if
		 * anything needs to be changed.
		 */
#ifndef	FOOEY
		tlbprot = ((mp->space == HP700_SID_KERNEL) ?
			   kern_prot : user_prot)[prot];
#else		
		tlbprot = pmap_prot(mp->pmap, prot);
#endif
		if ((mp->tlbprot & TLB_AR_MASK) != tlbprot) {
			mp->tlbprot &= ~TLB_AR_MASK;
			mp->tlbprot |= tlbprot;

			pmap_clear_cache(mp->space, mp->offset);

			/*
			 * Purge the current TLB entry (if any) to force
			 * a fault and reload with the new protection.
			 */
			pdtlb(mp->space, mp->offset);
			pitlb(mp->space, mp->offset);
		}
	}
	splx(s);
}

boolean_t
pmap_verify_free(vm_offset_t pa)
{
	struct phys_entry *pp;
	boolean_t result;

	if (!pmap_initialized)
		return(TRUE);

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return(FALSE);

	return(queue_empty(&pp->phys_link));
}

/*
 * pmap_protect(pmap, s, e, prot)
 *	changes the protection on all virtual addresses v in the 
 *	virtual address range determined by [s, e) and pmap to prot.
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
	register struct mapping *mp;
	unsigned tlbprot;
	space_t space;
	vm_offset_t offset;

	assert((prot & ~VM_PROT_ALL) == 0);
	if (pmap == PMAP_NULL)
		return;

	assert(sva <= eva);

	if (prot == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	if (prot & VM_PROT_WRITE)
		return;

	simple_lock(&pmap->lock);

	space = pmap_sid(pmap, sva);
	
	for(; sva < eva; sva += PAGE_SIZE) {
		if(mp = pmap_find_mapping(space, sva)) {
			assert(space == mp->space);
			
			/*
			 * Determine if mapping is changing.
			 * If not, nothing to do.
			 */
			tlbprot = pmap_prot(pmap, prot);
			if ((mp->tlbprot & TLB_AR_MASK) == tlbprot)
				continue;
			
			mp->tlbprot &= ~TLB_AR_MASK;
			mp->tlbprot |= tlbprot;
			
			offset = mp->offset;
			
			pmap_clear_cache(space, offset);
			
			/*
			 * Purge the current TLB entry (if any) to force
			 * a fault and reload with the new protection.
			 */
			pdtlb(space, offset);
			pitlb(space, offset);
		}
	}
	simple_unlock(&pmap->lock);	
}

/*
 * pmap_enter
 *
 * Create a translation for the virtual address (virt) to the physical address
 * (phys) in the pmap with the protection requested. If the translation is 
 * wired then we can not allow a page fault to occur for this mapping.
 */
void
pmap_enter(pmap_t pmap, vm_offset_t va, vm_offset_t pa, vm_prot_t prot,
	   boolean_t wired)
{
	register struct mapping *mp, *mpp;
	struct phys_entry *pp;
	unsigned tlbpage, tlbprot;
	space_t space;
	boolean_t inst, waswired;

	assert((prot & ~VM_PROT_ALL) == 0);
        if (pmap == PMAP_NULL)
                return;

#if MACH_ASSERT
#ifdef IO_HACK
	/*
	 * We should never see enters on IO pages after pmap_init has been
	 * called.  If we do, then our assumption that such addresses are
	 * never multiply-mapped is bogus.
	 */
	if ((long)pa >= io_end && (long)pa < 0) {
		if (pmap_initialized)
			panic("enter: called with IO page");
		return;
	}
#endif
#ifdef EQUIV_HACK
	/*
	 * We should never see enters on equivalently mapped pages.
	 * If we do, then our assumption that such addresses are never
	 * multiply-mapped is bogus.
	 */
	if ((long)pa >= 0 && (long)pa < equiv_end)
		panic("enter: called with equiv page");
#endif
#endif /* MACH_ASSERT */

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL) {
		return;
	}

	/*
	 * Align page # for easy insertion in TLB.
	 */
	tlbpage = patotlb(pa);

	simple_lock(&pmap->lock);

	space = pmap_sid(pmap, va);

	mp = MAPPING_NULL;
	for (;;) {
		mpp = pmap_find_mapping(space, va);
		if (mpp) {
			if (mpp->tlbpage == tlbpage) {
				if (mp) {
					assert (space != HP700_SID_KERNEL);
					physmem_free(&pmap_mapping,
						     (vm_offset_t)mp);
				}
				break;
			}
			/*
			 * Mapping exists but for a different physical page.
			 * Clean up the old mapping first to simplify
			 * matters later.
			 */
			pmap->stats.resident_count--;
			pmap_free_mapping(mpp);
			mpp = MAPPING_NULL;
		}
		if (mp != MAPPING_NULL)
			break;

		if (space != HP700_SID_KERNEL) {
			simple_unlock(&pmap->lock);
			mp = (struct mapping *)physmem_alloc(&pmap_mapping);
			simple_lock(&pmap->lock);
			continue;
		}
		simple_lock(&free_mapping_lock);
		if (queue_empty(&free_mapping))
			panic("pmap_enter: can't find a free mapping entry");
		queue_remove_first(&free_mapping, mp,
				   struct mapping *, hash_link);
		simple_unlock(&free_mapping_lock);
		break;
	}

#ifdef USEALIGNMENT
	/*
	 * Determine if the new mapping changes the cache alignment status
	 * of this physical page.
	 */
	if (pmap_alignmask && mpp == MAPPING_NULL) {
		if (queue_empty(&pp->phys_link)) {
			pp->tlbprot |= TLB_ALIGNED;
		} else {
			register struct mapping *amp;

			if (pp->tlbprot & TLB_ALIGNED) {
#if MACH_ASSERT
				/*
				 * aligned means several mapping for a pa.
				 * for the cow work, no writer should be allowed
				 */
				if (pp->writer)
					panic("enter: aligned with writer");
#endif
				amp = (struct mapping *)
					queue_first(&pp->phys_link);
				if (!pmap_aligned(va, amp->offset)) {
					pmap_flush_writers(pp);
					pp->tlbprot &= ~TLB_ALIGNED;
				}
			}
		}
	}
#endif

	if (mpp == MAPPING_NULL) {
#ifdef	TLB_ZERO
		/*
		 * See if we need to zero the page, and do it now that
		 * we have a virtual address to use. 
		 */
		if (pp->tlbprot & TLB_ZERO) {
			lpage_zero(tlbpage, space, va);
			pp->tlbprot &= ~TLB_ZERO;
		}
#endif

		/*
		 * Mapping for this virtual address doesn't exist.
		 * Enter a new mapping structure.
		 */
		mp->space = space;
		mp->offset = va;
		mp->tlbpage = tlbpage;
		mp->tlbprot = 0;
		mp->tlbsw = 0;
		pmap_enter_mapping(mp);
		pmap->stats.resident_count++;
	}
	else {
		/*
		 * We are just changing the protection.
		 * Flush the current TLB entry to force a fault and reload.
		 */
		mp = mpp;
		pdtlb(mp->space, mp->offset);
		pitlb(mp->space, mp->offset);
	}

	/*
	 * Determine the protection information for this mapping.
	 *
	 * WARNING: It looks like there is a big race condition here.
	 *          We enter the mapping above with no prot field, and
	 *          then set it here. Another TLB miss could come in
	 *          the meantime, and get a bogus mp->tlbprot value.
	 */
	tlbprot = pmap_prot(pmap, prot) | pmap->pid;
	tlbprot |= (mp->tlbprot & ~(TLB_AR_MASK|TLB_PID_MASK));

	/*
	 * Add in software bits and adjust statistics
	 */
	waswired = tlbprot & TLB_WIRED;
	if (wired && !waswired) {
		tlbprot |= TLB_WIRED;
		pmap->stats.wired_count++;
	} else if (!wired && waswired) {
		tlbprot &= ~TLB_WIRED;
		pmap->stats.wired_count--;
	}
	mp->tlbprot = tlbprot;

	pmap_clear_cache(mp->space, mp->offset);
	simple_unlock(&pmap->lock);
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
	register struct mapping *mp;
	boolean_t waswired;

	if (pmap == PMAP_NULL)
		return;

	simple_lock(&pmap->lock);

	if ((mp = pmap_find_mapping(pmap_sid(pmap, va), va)) == MAPPING_NULL)
		panic("pmap_change_wiring: can't find mapping entry");

	waswired = mp->tlbprot & TLB_WIRED;
	if (wired && !waswired) {
		mp->tlbprot |= TLB_WIRED;
		pmap->stats.wired_count++;
	} else if (!wired && waswired) {
		mp->tlbprot &= ~TLB_WIRED;
		pmap->stats.wired_count--;
	}
	simple_unlock(&pmap->lock);
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
	struct mapping *mp;

	/*
	 * Handle easy special cases
	 */
	if (pmap == kernel_pmap) {
#if defined(EQUIV_HACK)
		if (va < (vm_offset_t)equiv_end)
			return(va);
#endif
		if ((long)va >= io_end && (long)va < 0)
			return(va);
	}

	mp = pmap_find_mapping(pmap_sid(pmap, va), va);

	if (mp == MAPPING_NULL)
		return(0);

	return(tlbtopa(mp->tlbpage) + (va & page_mask));
}

/*
 *	pmap_attributes:
 *
 *	Set/Get special memory attributes
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
	vm_offset_t 	sva, eva;
	kern_return_t	ret;

	if (attribute != MATTR_CACHE)
		return KERN_INVALID_ARGUMENT;

	if (pmap == PMAP_NULL)
		return KERN_SUCCESS;

	sva = trunc_page(address);
	eva = round_page(address + size);
	ret = KERN_SUCCESS;

	switch (*value) {

	case MATTR_VAL_CACHE_FLUSH:	/* flush from all caches */
	case MATTR_VAL_DCACHE_FLUSH:	/* flush from data cache(s) */
	case MATTR_VAL_ICACHE_FLUSH:	/* flush from instruction cache(s) */
	{
		int op;

		/* which cache should we flush */
		op = 0;
		if (*value == MATTR_VAL_CACHE_FLUSH ||
		    *value == MATTR_VAL_ICACHE_FLUSH)
			op = 1;
		if (*value == MATTR_VAL_CACHE_FLUSH ||
		    *value == MATTR_VAL_DCACHE_FLUSH)
			op |= 2;

		for (; sva < eva; sva += PAGE_SIZE) {
			if (pmap_extract(pmap, sva) == 0)
				continue;
			if (op & 1)
				ficache(pmap_sid(pmap, sva), sva, PAGE_SIZE);
			if (op & 2)
				fdcache(pmap_sid(pmap, sva), sva, PAGE_SIZE);
		}
		break;
	}

	case MATTR_VAL_OFF:		/* (generic) turn attribute off */
	case MATTR_VAL_ON:		/* (generic) turn attribute on */
	case MATTR_VAL_GET:		/* (generic) return current value */
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
}

/* ARGSUSED */
void
pmap_deactivate(
	pmap_t pmap,
	thread_t th,
	int which_cpu)
{
}

/*
 * pmap_zero_page(pa)
 *
 * pmap_zero_page zeros the specified page. 
 */
void
pmap_zero_page(vm_offset_t pa)
{
	register struct mapping *mp;
	register struct phys_entry *pp;

#if MACH_ASSERT
	/*
	 * XXX can these happen?
	 */
	if (!PHYS_INRANGE(pa))
		panic("zero_page: physaddr out of range");
	if ((long)pa < 0)
		panic("zero_page: IO page");
#endif
#ifdef EQUIV_HACK
	if (pa < equiv_end) {
		printf("pmap_zero_page: pa(%x)\n", pa);
		panic("Call for Help!");
	}
#endif

#ifdef	TLB_ZERO
	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL) {
		phys_bzero(pa, PAGE_SIZE);
		return;
	}

	/*
	 * If there are no mappings for this page, then just mark this page
	 * to be zeroed later, when pmap_enter is called, and there is a
	 * virtual address to use. Using virtual addresses means that we
	 * do not have to flush the cache (the physical addresses). This
	 * shortens the time to zero the page, and it results in a warm cache.
	 * Otherwise, do a slower physical page zero.
	 */
	if (queue_empty(&pp->phys_link)) {
		pp->tlbprot |= TLB_ZERO;
	}
	else {
		pmap_clear_mappings(pa, MAPPING_NULL);
		phys_bzero(pa, PAGE_SIZE);
	}
#else
	pmap_clear_mappings(pa, MAPPING_NULL);
	phys_bzero(pa, PAGE_SIZE);
#endif

}

/*
 * pmap_copy_page(src, dst)
 *
 * pmap_copy_page copies the src page to the destination page. If a mapping
 * can be found for the source, we use that virtual address. Otherwise, a
 * slower physical page copy must be done. The destination is always a
 * physical address since there is usually no mapping for it.
 */
void
pmap_copy_page(
	vm_offset_t spa,
	vm_offset_t dpa)
{
	register struct phys_entry *pp;
	vm_offset_t soffset;
	space_t sspace;
	struct mapping *mp;
	boolean_t realmode = FALSE;

#if MACH_ASSERT
	/*
	 * XXX can these happen?
	 */
	if (!PHYS_INRANGE(spa) || !PHYS_INRANGE(dpa))
		panic("copy_page: physaddr out of range");
	if ((long)spa < 0 || (long)dpa < 0)
		panic("copy_page: IO page");
#endif
#ifdef EQUIV_HACK
	if ((long)spa < equiv_end || (long)dpa < equiv_end) {
		printf("pmap_copy_page: spa(%x), dpa(%x)\n", spa, dpa);
		panic("Call for Help!");
	}
#endif
	/*
	 * Determine the best source address to use:
	 *
	 * 1. If currently mapped for a writer use that
	 * 2. If otherwise mapped use that
	 * 3. Else must do real mode copy
	 */
	pp = pmap_find_physentry(spa);
#ifdef	TLB_ZERO
	if (pp->tlbprot & TLB_ZERO) {
		pp->tlbprot &= ~TLB_ZERO;
	}
#endif
	if (mp = pp->writer) {
		sspace = mp->space;
		soffset = mp->offset;
	} else if (!queue_empty(&pp->phys_link)) {
		mp = (struct mapping *) queue_first(&pp->phys_link);
		sspace = mp->space;
		soffset = mp->offset;
	} else {
		realmode = TRUE;
	}

	pp = pmap_find_physentry(dpa);
#ifdef	TLB_ZERO
	if (pp->tlbprot & TLB_ZERO) {
		pp->tlbprot &= ~TLB_ZERO;
	}
#endif
	/*
	 * If no mapping for the source, we do a real mode copy. If we
	 * found a mapping for the source, we can copy from that virtual
	 * address to the destination physical address.
	 */
	if (realmode) {
		/*
		 * pmap_clear_mappings will handle flushing the proper
		 * source virtual address. We flush the destination 
		 * also, even though it is probably not mapped.
		 *
		 * XXX should really purge rather than flush any dpa cache.
		 */
		pmap_clear_mappings(spa, MAPPING_NULL);
		pmap_clear_mappings(dpa, MAPPING_NULL);
		phys_page_copy(spa, dpa);
	} else {
		lpage_copy(mp->tlbpage, sspace, soffset, dpa);
	}
}

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
	register struct phys_entry *pp;
	register struct mapping *mp;

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL || (pp->tlbprot & TLB_DIRTY) == 0)
		return;

	/*
	 * Flush any writer cache to ensure memory is up to date.
	 */
#ifdef USEALIGNMENT
	if (pp->tlbprot & TLB_ALIGNED)
		pmap_flush_writers(pp);
	else
#endif
	if (mp = pp->writer) {
		fdcache(mp->space, mp->offset, PAGE_SIZE);
		pdtlb(mp->space, mp->offset);

		mp->tlbsw &= ~TLB_DCACHE;
		mp->tlbprot &= ~TLB_DIRTY;

		pmap_clear_cache(mp->space, mp->offset);

		pp->writer = MAPPING_NULL;
	}
	pp->tlbprot &= ~TLB_DIRTY;
}

/*
 * pmap_set_modify(phys)
 *	sets the hardware modified ("dirty") bit for one
 *	machine independant page starting at the given
 *	physical address.  phys must be aligned on a machine
 *	independant page boundary.
 */
void
pmap_set_modify(vm_offset_t pa)
{
	register struct phys_entry *pp;
	register struct mapping *mp;

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL || (pp->tlbprot & TLB_DIRTY) == 1)
		return;

	if (!(mp = pp->writer)) {
		pmap_clear_mappings(pa, MAPPING_NULL);
	}
	else {
		panic("pmap_set_modify");
	}
	pp->tlbprot |= TLB_DIRTY;
}

/*
 * pmap_is_modified(phys)
 *	returns TRUE iff the given physical page has been modified 
 *	since the last call to pmap_clear_modify().
 */
boolean_t
pmap_is_modified(vm_offset_t pa)
{
	struct phys_entry *pp;

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL || (pp->tlbprot & TLB_DIRTY) == 0)
		return(FALSE);

	return(TRUE);
}

/*
 * pmap_clear_reference(phys)
 *	clears the hardware referenced bit in the given machine
 *	independant physical page.  
 *
 *	Currently, we treat a TLB miss as a reference; i.e. to clear
 *	the reference bit we flush all mappings for pa from the TLBs.
 */
void
pmap_clear_reference(vm_offset_t pa)
{
	register struct phys_entry *pp;
	register struct mapping *mp;
	spl_t s;

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return;

	s = splvm();
	queue_iterate(&pp->phys_link, mp, struct mapping *, phys_link) {
		pitlb(mp->space, mp->offset);
		pdtlb(mp->space, mp->offset);
		mp->tlbprot &= ~(TLB_REF);

		pmap_clear_cache(mp->space, mp->offset);
	}
	pp->tlbprot &= ~TLB_REF;
	splx(s);
}

/*
 * pmap_is_referenced(phys)
 *	returns TRUE iff the given physical page has been referenced 
 *	since the last call to pmap_clear_reference().
 */
boolean_t
pmap_is_referenced(vm_offset_t pa)
{
	struct phys_entry *pp;
	register struct mapping *mp;
	spl_t s;

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL)
		return(FALSE);
	if (pp->tlbprot & TLB_REF)
		return(TRUE);

	s = splvm();
	queue_iterate(&pp->phys_link, mp, struct mapping *, phys_link) {
		if (mp->tlbprot & TLB_REF) {
			pp->tlbprot |= TLB_REF;
			splx(s);
			return(TRUE);
		}
	}
	splx(s);
	return(FALSE);
}

/*
 *	Set the modify bit on the specified range
 *	of this map as requested.
 *
 * Touch the data being returned, to mark it dirty.
 * If the pages were filled by DMA, the pmap module
 * may think that they are clean.
 *
 */
void
pmap_modify_pages(
	pmap_t		pmap,
	vm_offset_t	sva,
	vm_offset_t	eva)
{
	vm_offset_t pa;


	assert(sva <= eva);

	if(pmap == PMAP_NULL)
		return;

	simple_lock(&pmap->lock);
	
	while (sva < eva) {
		if (pa = kvtophys(sva)) {
			register struct phys_entry *pp;
			register struct mapping *mp;

			if(pp = pmap_find_physentry(pa)) {
#ifdef USEALIGNMENT
				if (pp->tlbprot & TLB_ALIGNED)
					pmap_flush_writers(pp);
				else
#endif
					if (mp = pp->writer) {
						fdcache(mp->space, mp->offset, PAGE_SIZE);
						pdtlb(mp->space, mp->offset);
						
						mp->tlbsw |= TLB_DCACHE;
						mp->tlbprot |= TLB_DIRTY;

						pmap_clear_cache(mp->space, mp->offset);
						
						pp->writer = MAPPING_NULL;
					}
				pp->tlbprot |= TLB_DIRTY;
			}
		}
		sva += PAGE_SIZE;
	}
	

	simple_unlock(&pmap->lock);
}

#ifndef IO_HACK
/*
 * XXX
 *
 * Wretched hack for user IO mapping
 */
boolean_t
pmap_iomap(vm_offset_t va)
{
	struct phys_entry *pp;
	vm_offset_t eva;
	vm_size_t len = 0x2000000;

	if ((long)va < io_end || (long)va >= 0)
		return(FALSE);

	pp = pmap_find_physentry(trunc_page((long)va >> 14));
	if ((pp->tlbprot & TLB_AR_MASK) == TLB_AR_URWX)
		return(TRUE);

	pp->tlbprot &= ~TLB_AR_MASK;
	pp->tlbprot |= TLB_AR_URWX;
	eva = round_page(va + len);
	for (va = trunc_page(va); va < eva; va += PAGE_SIZE) {
		pitlb(HP700_SID_KERNEL, va);
		pdtlb(HP700_SID_KERNEL, va);
	}
	return(TRUE);
}

void
pmap_iounmap(vm_offset_t va)

{
	struct phys_entry *pp;
	vm_offset_t eva;
	vm_size_t len = 0x2000000;

	if ((long)va < io_end || (long)va >= 0)
		panic("pmap_iounmap: bad address");

	pp = pmap_find_physentry(trunc_page((long)va >> 14));

	if ((pp->tlbprot & TLB_AR_MASK) == TLB_AR_URWX) {
		pp->tlbprot &= ~TLB_AR_MASK;
		pp->tlbprot |= TLB_AR_KRWX;
		eva = round_page(va + len);
		for (va = trunc_page(va); va < eva; va += PAGE_SIZE) {
			pitlb(HP700_SID_KERNEL, va);
			pdtlb(HP700_SID_KERNEL, va);
		}
	}
}
#endif

/*
 * Auxilliary routines for manipulating mappings
 */
static void
pmap_enter_mapping(
	register struct mapping *mp)
{
	register struct mapping *pmp;
	struct phys_entry *pp;	
	unsigned page;
	int hash;
	spl_t s;

	page = tlbtopa(mp->tlbpage);
	pp = pmap_find_physentry(page);
	hash = pmap_hash(mp->space, mp->offset);

	s = splvm();

	queue_enter_first(&pp->phys_link,
			  mp, struct mapping *, phys_link);
	queue_enter_first(&vtop_table[hash].hash_link,
			  mp, struct mapping *, hash_link);
#ifdef	USEALIGNMENT
	/*
	 * Now set the TLB_ALIGNED bit for each mapping in the ptov chain
	 * so that the tlb miss handler can check the mapping directly,
	 * instead of looking in the ptov table.
	 */
	queue_iterate(&pp->phys_link, pmp, struct mapping *, phys_link) {
		if (pp->tlbprot & TLB_ALIGNED)
			pmp->tlbprot |= TLB_ALIGNED;
		else
			pmp->tlbprot &= ~TLB_ALIGNED;

		pmap_clear_cache(pmp->space, mp->offset);
	}
#endif
#ifdef	HPT
	hpt_table[hash].valid = 0;
	hpt_table[hash].space = -1;
#endif
	splx(s);
}

/*
 * pmap_find_mapping(space, offset)
 *	Lookup the virtual address <space,offset> in the mapping "table".
 *	Returns a pointer to the mapping or NULL if none.
 */
static struct mapping *
pmap_find_mapping(
	register space_t space,
	register vm_offset_t offset)
{
	register queue_t qp;
	register struct mapping *mp;
	spl_t s;

	offset = trunc_page(offset);

	qp = &vtop_table[pmap_hash(space, offset)].hash_link;

	s = splvm();
	queue_iterate(qp, mp, struct mapping *, hash_link) {
		if (mp->space == space && mp->offset == offset) {
			splx(s);
			return(mp);
		}
	}
	splx(s);
	return(MAPPING_NULL);
}

static void
pmap_free_mapping(register struct mapping *mp)
{
	register space_t space;
	register vm_offset_t offset;
	struct phys_entry *pp;
	int hash;
	spl_t s;

	space = mp->space;
	offset = mp->offset;

	/*
	 * First flush caches and TLBs.
	 * Must be done before removing from virtual/physical lists
	 * since cache flush may cause TLB miss.  Note that we also
	 * must flush the I-cache first since the FIC instruction uses
	 * the DTLB.  We also clear the writer pointer if it references
	 * this mapping.  This prevents an interrupt-time TLB miss from
	 * reentering the mapping in the TLB after we have just removed it.
	 *
	 * XXX I think we have a race here.  D-cache flush needs to be
	 * atomic wrt clearing the writer pointer.
	 */
	pp = pmap_find_physentry(tlbtopa(mp->tlbpage));
	if (pp->writer == mp)
		pp->writer = MAPPING_NULL;

	if (mp->tlbsw & TLB_ICACHE)
		ficache(space, offset, PAGE_SIZE);
	pitlb(space, offset);
	if (mp->tlbsw & TLB_DCACHE)
		fdcache(space, offset, PAGE_SIZE);
	pdtlb(space, offset);

	
	/*
	 * Now remove from the PTOV/VTOP lists and return to the free list.
	 * Note that we must block interrupts since they might cause TLB
	 * misses which would search (potentially inconsistant) lists.
	 */

	hash = pmap_hash(space, offset);

	s = splvm();

	queue_remove(&vtop_table[hash].hash_link,
		     mp, struct mapping *, hash_link);
	queue_remove(&pp->phys_link, mp, struct mapping *, phys_link);
	if (space == HP700_SID_KERNEL) {
		simple_lock(&free_mapping_lock);
		queue_enter(&free_mapping, mp, struct mapping *, hash_link);
		simple_unlock(&free_mapping_lock);
	} else
		physmem_free(&pmap_mapping, (vm_offset_t)mp);
	
#ifdef	HPT
	hpt_table[hash].valid = 0;
	hpt_table[hash].space = -1;
#endif

#ifdef USEALIGNMENT
	/*
	 * If we were not aligned, check all remaining entries to see if
	 * they are now aligned (i.e. we removed the unaligned one).
	 */
	if (pp->tlbprot & TLB_ALIGNED) {
		if (queue_empty(&pp->phys_link))
			pp->tlbprot &= ~TLB_ALIGNED;
	} else if (!queue_empty(&pp->phys_link)) {
		boolean_t aligned = TRUE;

		mp = (struct mapping *) queue_first(&pp->phys_link);
		offset = mp->offset;
		while (queue_last(&pp->phys_link) != (queue_entry_t)mp) {
			mp = (struct mapping *) queue_next(&mp->phys_link);
			if (!pmap_aligned(offset, mp->offset)) {
				aligned = FALSE;
				break;
			}
		}
		if (aligned) {
			pp->writer = MAPPING_NULL;
			pp->tlbprot |= TLB_ALIGNED;
		}
	}
	/*
	 * Now set/clear the TLB_ALIGNED bit for each mapping in the ptov 
	 * chain so that the tlb miss handler can check the mapping directly,
	 * instead of looking in the ptov table.
	 */
	queue_iterate(&pp->phys_link, mp, struct mapping *, phys_link) {
		if (pp->tlbprot & TLB_ALIGNED)
			mp->tlbprot |= TLB_ALIGNED;
		else
			mp->tlbprot &= ~TLB_ALIGNED;

		pmap_clear_cache(mp->space, mp->offset);
	}
#endif
	splx(s);
}

/*
 * Flush caches and TLB entries refering to physical page pa.  If cmp is
 * non-zero, we do not affect the cache or TLB entires for that mapping.
 */
static void
pmap_clear_mappings(
	vm_offset_t pa,
	struct mapping *cmp)
{
	register struct phys_entry *pp;
	register struct mapping *mp;
	spl_t s;

	pp = pmap_find_physentry(pa);
	if (pp == PHYS_NULL) {
		return;
	}

	/*
	 * If writer is non-zero we know that there is only one
	 * valid mapping active (i.e. with TLB and/or cache entries).
	 * Hence, there is no need to search the entire list.
	 *
	 * XXX needs to be atomic?
	 */
	if (mp = pp->writer) {
#if MACH_ASSERT
		if ((mp->tlbprot & TLB_DIRTY) == 0)
			panic("clear_mappings: writer w/o TLB_DIRTY");
		if ((mp->tlbsw & TLB_DCACHE) == 0)
			panic("clear_mappings: writer w/o TLB_DCACHE");
#endif
		if (mp != cmp) {
			fdcache(mp->space, mp->offset, PAGE_SIZE);
			pdtlb(mp->space, mp->offset);

			mp->tlbsw &= ~TLB_DCACHE;
			mp->tlbprot &= ~TLB_DIRTY;

			pmap_clear_cache(mp->space, mp->offset);

			pp->writer = MAPPING_NULL;
		}
		return;
	}

	/*
	 * Otherwise we traverse all mappings associated with this address
	 * purging TLBs and flushing caches.
	 */
	s = splvm();
	queue_iterate(&pp->phys_link, mp, struct mapping *, phys_link) {
		if (mp == cmp)
			continue;
		/*
		 * have to clear the icache first since fic uses the dtlb.
		 */
		if (mp->tlbsw & TLB_ICACHE)
			ficache(mp->space, mp->offset, PAGE_SIZE);
		pitlb(mp->space, mp->offset);
		if (mp->tlbsw & TLB_DCACHE)
			fdcache(mp->space, mp->offset, PAGE_SIZE);
		pdtlb(mp->space, mp->offset);

		mp->tlbsw &= ~(TLB_ICACHE|TLB_DCACHE);
#ifdef USEALIGNMENT
		mp->tlbprot &= ~TLB_DIRTY;
#endif
		pmap_clear_cache(mp->space, mp->offset);
	}
	splx(s);
}

/*
 * Clear the HPT table entry for the corresponding space/offset to reflect
 * the fact that we have possibly changed the mapping, and need to pick
 * up new values from the mapping structure on the next access.
 */
static __inline__ void
pmap_clear_cache(space_t space, vm_offset_t offset)
{
#ifdef HPT
	int hash = pmap_hash(space, offset);

	hpt_table[hash].valid = 0;
	hpt_table[hash].space = -1;
#endif
}

#ifdef USEALIGNMENT
/*
 * Flush all write mappings for a physical page.
 *
 * If all mappings for a page are aligned in the cache we may have
 * multiple writers.  They will all be identified by having TLB_DIRTY
 * set in their tlbprot field.  We flush the TLB entries for all of
 * these to ensure we detect the first attempt to write the page again.
 * Note that since all mappings are aligned, we only need one cache flush
 * and that can use any mapping.
 */
static void
pmap_flush_writers(struct phys_entry *pp)
{
	register struct mapping *mp;
	boolean_t flushed = FALSE;
	spl_t s;

	if (queue_empty(&pp->phys_link))
		return;

#if MACH_ASSERT
	if (pp->writer)
		panic("flush_writers: writer set");
#endif

	mp = (struct mapping *) queue_first(&pp->phys_link);

	/*
	 * XXX ugh!  This needs to be atomic to prevent TLB misses in
	 * interrupt routines from seeing inconsistent cache info.
	 */

	s = splvm();

	while (!queue_end(&pp->phys_link, (queue_entry_t)mp)) {
		if (mp->tlbprot & TLB_DIRTY) {
			if (!flushed) {
				fdcache(mp->space, mp->offset, PAGE_SIZE);
				flushed = TRUE;
			}
			pdtlb(mp->space, mp->offset);
			mp->tlbsw &= ~TLB_DCACHE;
			mp->tlbprot &= ~TLB_DIRTY;

			pmap_clear_cache(mp->space, mp->offset);
		}
		mp = (struct mapping *) queue_next(&mp->phys_link);
	}
	splx(s);
}
#endif

/*
 *	Find the logarithm base 2 of a value
 */
int 
log2(unsigned	value)
{
	unsigned	i;

	for (i = 0; i < 32; i++)
		if ((1 << i) >= value)
			break;

	return i;
}

/*
 * Maintain a table of pmap structures so that we can map from a space
 * id to the associated pmap (for the benefit of pmap_remove_all()).
 *
 * NOTE: Originally, there was a pointer in the mapping structure that
 *       could be used to get to the pmap. However, that pointer made
 *	 the mapping structure 4 bytes more than a cache line (32 bytes).
 *	 This is bad, bad, bad. Hence, this seemingly gratuatious code.
 */
#define	PMAP_STASH_SIZE		32
#define pmap_stash_hash(space)  ((space) % PMAP_STASH_SIZE)

queue_head_t pmap_stash[PMAP_STASH_SIZE];

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
pmap_get_pmap(space_t space)
{
	pmap_t	pmap;
	int 	hash = pmap_stash_hash(space);

	if (space == HP700_SID_KERNEL) {
		simple_lock(&kernel_pmap->lock);
		return(kernel_pmap);
	}
	
	queue_iterate(&pmap_stash[hash], pmap, pmap_t, pmap_link) {
		if (pmap->space == space) {
			simple_lock(&pmap->lock);
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

void
pmap_copy_part_page(
	vm_offset_t	src,
	vm_offset_t	src_offset,
	vm_offset_t	dst,
	vm_offset_t	dst_offset,
	vm_size_t	len)
{
	register struct phys_entry *pp_src, *pp_dst;
	int	s;

        assert(((dst & PAGE_MASK)+dst_offset+len) <= PAGE_SIZE);
        assert(((src & PAGE_MASK)+src_offset+len) <= PAGE_SIZE);

	/*
	 * pmap_clear_mappings will handle flushing the proper
	 * source virtual address. We flush the destination 
	 * also, even though it is probably not mapped.
	 * XXX should really purge rather than flush any dst cache.
	 */
	pp_src = pmap_find_physentry(src);
	if (pp_src != PHYS_NULL) {
		pmap_clear_mappings(src, MAPPING_NULL);
	}

	pp_dst = pmap_find_physentry(dst);
	if (pp_dst != PHYS_NULL) {
		pmap_clear_mappings(dst, MAPPING_NULL);
#ifdef	TLB_ZERO
		if (pp_dst->tlbprot & TLB_ZERO) {
			phys_bzero(trunc_page(dst), PAGE_SIZE);
			pp_dst->tlbprot &= ~TLB_ZERO;
		}
#endif
	}

	/*
	 * XXX This needs to be atomic to prevent TLB misses in
	 * interrupt routines from seeing inconsistent cache info.
	 */
	s = splhigh();

	/*
	 * Since the source and destination are physical addresses, 
	 * turn off data translation to perform a  bcopy() in bcopy_phys().
	 * The physical addresses are flushed from the data cache
	 * by bcopy_phys().
	 */
	bcopy_phys((vm_offset_t) src+src_offset, (vm_offset_t) dst+dst_offset, len);

	splx(s);
}

void
pmap_zero_part_page(
	vm_offset_t	p,
	vm_offset_t     offset,
	vm_size_t       len)
{
	register struct phys_entry *pp;
	int	s;

        assert(((p & PAGE_MASK)+offset+len) <= PAGE_SIZE);

	/*
	 * pmap_clear_mappings will handle flushing the proper
	 * source virtual address. 
	 */
	pp = pmap_find_physentry(p);
	if (pp != PHYS_NULL) {
		pmap_clear_mappings(p, MAPPING_NULL);
#ifdef	TLB_ZERO
		if (pp->tlbprot & TLB_ZERO) {
			phys_bzero(trunc_page(p), PAGE_SIZE);
			pp->tlbprot &= ~TLB_ZERO;
			return;
		}
#endif
	}

	/*
	 * XXX This needs to be atomic to prevent TLB misses in
	 * interrupt routines from seeing inconsistent cache info.
	 */
	s = splhigh();

	/*
	 * Since the destination is physical addresses, 
	 * turn off data translation to perform a  bzero() in bzero_phys().
	 * The physical addresses are flushed from the data cache
	 * by bzero_phys().
	 */
	bzero_phys((vm_offset_t) p+offset, len);

	splx(s);
}

void             (pmap_copy_part_lpage)(
                                vm_offset_t     src,
                                vm_offset_t     dst,
                                vm_offset_t     dst_offset,
                                vm_size_t       len);

/*
 *	pmap_copy_part_lpage copies part of a virtually addressed page 
 *	to a physically addressed page.
 */
void
pmap_copy_part_lpage(
	vm_offset_t	src,
	vm_offset_t	dst,
	vm_offset_t	dst_offset,
	vm_size_t	len)
{
	register struct phys_entry *pp_dst;
	int	s;

	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);
	assert(((dst & PAGE_MASK)+dst_offset+len) <= PAGE_SIZE);
	/*
	 * pmap_clear_mappings will handle flushing the proper
	 * source virtual address. We flush the destination 
	 * also, even though it is probably not mapped.
	 * XXX should really purge rather than flush any dst cache.
	 */

	pp_dst = pmap_find_physentry(dst);
	if (pp_dst != PHYS_NULL) {
		pmap_clear_mappings(dst, MAPPING_NULL);
#ifdef	TLB_ZERO
		if (pp_dst->tlbprot & TLB_ZERO) {
			phys_bzero(trunc_page(dst), PAGE_SIZE);
			pp_dst->tlbprot &= ~TLB_ZERO;
		}
#endif
	}
	/*
	 * XXX This needs to be atomic to prevent TLB misses in
	 * interrupt routines from seeing inconsistent cache info.
	 */
	s = splhigh();

	/*
	 * Since the destination is a physical address, turn off data
	 * translation for destination address to perform a bcopy() 
	 * in bcopy_lphys().
	 * The physical addresses are flushed from the data cache
	 * by bcopy_lphys().
	 */
	bcopy_lphys((vm_offset_t) src, (vm_offset_t) dst+dst_offset, len);

	splx(s);
}

/*
 *	pmap_copy_part_rpage copies part of a physically addressed page 
 *	to a virtually addressed page.
 */
void
pmap_copy_part_rpage(
	vm_offset_t	src,
	vm_offset_t	src_offset,
	vm_offset_t	dst,
	vm_size_t	len)
{
	register struct phys_entry *pp_src;
	int	s;

	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);
	assert(((src & PAGE_MASK)+src_offset+len) <= PAGE_SIZE);

	/*
	 * pmap_clear_mappings will handle flushing the proper
	 * source virtual address. We flush the destination 
	 * also, even though it is probably not mapped.
	 * XXX should really purge rather than flush any dst cache.
	 */
	pp_src = pmap_find_physentry(src);
	if (pp_src != PHYS_NULL) {
		pmap_clear_mappings(src, MAPPING_NULL);
#ifdef	TLB_ZERO
		if (pp_src->tlbprot & TLB_ZERO) {
			phys_bzero(trunc_page(src), PAGE_SIZE);
			pp_src->tlbprot &= ~TLB_ZERO;
		}
#endif
	}
	/*
	 * XXX This needs to be atomic to prevent TLB misses in
	 * interrupt routines from seeing inconsistent cache info.
	 */
	s = splhigh();

	/*
	 * Since the source is a physical address, turn off data
	 * translation for source address to perform a bcopy() 
	 * in bcopy_rphys().
	 * The physical addresses are flushed from the data cache
	 * by bcopy_rphys().
	 */
	bcopy_rphys((vm_offset_t) src+src_offset, (vm_offset_t) dst, len);

	splx(s);
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
	struct mapping *mp;

	/*
	 * Handle easy special cases
	 */
#if defined(EQUIV_HACK)
	if (va < (vm_offset_t)equiv_end)
		return(va);
#endif
	if ((long)va >= io_end && (long)va < 0)
		return(va);


	mp = pmap_find_mapping(HP700_SID_KERNEL, va);

	if (mp == MAPPING_NULL)
		return(0);
	return tlbtopa(mp->tlbpage) + (va & page_mask);
}
