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

/* A marvelous selection of support routines for virtual memory */

#include <cpus.h>
#include <debug.h>
#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <mach_vm_debug.h>

#include <kern/cpu_number.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <machine/kgdb_defs.h>
#include <ppc/misc_protos.h>
#include <ppc/mem.h>
#include <ppc/pmap_internals.h>		/* For pmap_pteg_overflow */
#include <ppc/POWERMAC/powermac.h>

/* These refer to physical addresses and are set and referenced elsewhere */

unsigned int hash_table_base;
unsigned int hash_table_size;

unsigned int hash_function_mask;

/* Prototypes for static functions */

static pte_t *find_pte_in_pteg(pte_t* pte, pte0_t match);

/* gather statistics about hash table usage */

#if	DEBUG
#define MEM_STATS 1
#else
#define MEM_STATS 0
#endif /* DEBUG */

#if MEM_STATS
/* hash table usage information */
struct hash_table_stats {
	int find_pte_in_pteg_calls;
	int find_pte_in_pteg_not_found;
	int find_pte_in_pteg_location[8];
	struct find_or_alloc_calls {
		int found_primary;
		int found_secondary;
		int alloc_primary;
		int alloc_secondary;
		int overflow;
		int not_found;
	} find_or_alloc_calls[2];
	
} hash_table_stats[NCPUS];

#define INC_STAT(LOC) \
	hash_table_stats[cpu_number()].find_pte_in_pteg_location[LOC]++

#else	/* MEM_STATS */
#define INC_STAT(LOC)
#endif	/* MEM_STATS */

/* Set up the machine registers for the given hash table.
 * The table has already been zeroed.
 */

void hash_table_init(unsigned int base, unsigned int size)
{
	register pte_t* pte;
	unsigned int mask;
	int i;

#if MACH_VM_DEBUG
	if (size % (64*1024)) {
		DPRINTF(("ERROR! size = 0x%x\n",size));
		Debugger("FATAL");
	}
	if (base % size) {
		DPRINTF(("ERROR! base not multiple of size\n"));
		DPRINTF(("base = 0x%08x, size=%dK \n",base,size/1024));
		Debugger("FATAL");
	}
#endif

#if PTE_EMPTY != 0
	/* We do not use zero values for marking empty PTE slots. This
	 * initialisation could be sped up, but it's only called once.
	 * the hash table has already been bzeroed.
	 */
	for (pte = (pte_t *) base; (unsigned int)pte < (base+size); pte++) {
		pte->pte0.word = PTE_EMPTY;
	}
#endif /* PTE_EMPTY != 0 */

	mask = (size-1)>>16;							/* The easy way to make the mask */
	
	sync();											/* SYNC: it's not just the law, it's a good idea... */
	mtsdr1(hash_table_base | mask);					/* Slam the SDR1 with the has table address */
	sync();											/* SYNC: it's not just the law, it's a good idea... */
	isync();
	
	powermac_info.hashTableAdr = hash_table_base | mask;	/* Shadow copy of the hash table address */
	hash_function_mask = (mask << 16) | 0xFFC0;
}

/* Given the address of a pteg and the pte0 to match, this returns
 * the address of the matching pte or PTE_NULL. Can be used to search
 * for an empty pte by using PTE_EMPTY as the matching parameter.
 */

static __inline__ pte_t *
find_pte_in_pteg(pte_t* pte, pte0_t match)
{
	int index;

	pte0_t loose_match;
	pte0_t mask;

	/* other processors (if any) may be in the process of
	 * updating the modified/referenced bits of the pte, to
	 * do this they lock the pte and reset the valid bit.
	 * We want to quickly scan the pteg without having taken
	 * the pte lock so we check ignoring this bit, then take the lock
	 * (thus waiting for any other processor to release the lock)
	 * before checking for a sure match.
	 */
	mask.word = 0;
	mask.bits.valid = TRUE;
	mask.word = ~mask.word;

	loose_match.word = match.word & mask.word;

#if	MEM_STATS
	hash_table_stats[cpu_number()].find_pte_in_pteg_calls++;
#endif	/* MEM_STATS */

	for (index = 0; index < 8; index++) {
		if ((pte[index].pte0.word & mask.word) == loose_match.word) {
			LOCK_PTE(&pte[index]);
			if (pte[index].pte0.word == match.word) {
				INC_STAT(index);
				/* return with the PTE locked */
				return &pte[index];
			}
			/* the pte has been changed */
			UNLOCK_PTE(&pte[index]);
		}
	}

#if MEM_STATS
	hash_table_stats[cpu_number()].find_pte_in_pteg_not_found++;
#endif /* MEM_STATS */
	return PTE_NULL;
}

#if	MACH_KDB || MACH_KGDB

/*
 * Same thing for debuggers but without taking locks...
 */
static __inline__ pte_t *
db_find_pte_in_pteg(pte_t* pte, unsigned int match)
{
	int index;

#if	MEM_STATS
	hash_table_stats[cpu_number()].find_pte_in_pteg_calls++;
#endif	/* MEM_STATS */

	for (index = 0; index < 7; index++) {
		if (pte[index].pte0.word == match) {
			/* LOCK_PTE(&pte[index]); */
			if (pte[index].pte0.word == match) {
				INC_STAT(index);
				/* return with the PTE *NOT* locked */
				return &pte[index];
			}
			/* the pte has been changed */
			/* UNLOCK_PTE(&pte[index]); */
		}
	}

#if MEM_STATS
	hash_table_stats[cpu_number()].find_pte_in_pteg_not_found++;
#endif /* MEM_STATS */
	return PTE_NULL;
}
#endif	/* MACH_KDB || MACH_KGDB */

/* Given a space identifier and a virtual address, this returns the
 * address of the pte describing this virtual page.
 *
 * If allocate is FALSE, the function will return PTE_NULL if not found
 *
 * If allocate is TRUE the function will create a new PTE entry marked
 *                 as invalid with pte0 set up (ie no physical info or
 *                 protection/change info) and pte1 zeroed.
 *
 * Locking:
 * 	out: the PTE is locked (if not NULL)
 *	spl: VM
 */

pte_t *
find_or_allocate_pte(space_t sid, vm_offset_t v_addr, boolean_t allocate)
{
	register vm_offset_t	primary_hash,  secondary_hash;
	register pte0_t		primary_match, secondary_match;
	register pte_t		*pte;
	register unsigned int    segment_id;
	const pte0_t		match_empty = { PTE_EMPTY };
#if	DEBUG
	spl_t			s;

	/* Check we are called under SPLVM */
	s = splhigh();
	splx(s);
	assert(s <= SPLVM);
#endif	/* DEBUG */



	/* Horrible hack "(union *)&var-> " to retype to union */
	segment_id = (sid << 4) | (v_addr >> 28);

	primary_hash = segment_id ^ ((va_full_t *)&v_addr)->page_index;

	primary_hash = hash_table_base +
		((primary_hash << 6) & hash_function_mask);

	/* Firstly check out the primary pteg */

#if 0 && MACH_VM_DEBUG
	if (sid) {
		DPRINTF(("PRIMARY HASH OF SID 0x%x and addr 0x%x = 0x%x**********\n",
			 sid, v_addr, primary_hash));
	}
#endif

	/* TODO NMGS could be faster if all put on one line with shifts etc,
	 * does -O2 optimise this?
	 */

	primary_match.bits.valid 	= 1;
	/* virtual segment id = concat of space id + seg reg no */
	primary_match.bits.segment_id	= segment_id;
	primary_match.bits.hash_id	= 0;
	primary_match.bits.page_index	= ((va_abbrev_t *)&v_addr)->page_index;


	pte = find_pte_in_pteg((pte_t *) primary_hash, primary_match);

	if (pte != PTE_NULL) {
#if MEM_STATS
		hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].found_primary++;
#endif /* MEM_STATS */
		assert (pte->pte0.word != PTE_EMPTY);
		return pte;
	}

#if 0 && MACH_VM_DEBUG
	DPRINTF(("find_pte : searching secondary hash\n"));
#endif

	/* pte wasn't found in primary hash location, check secondary */

	secondary_match = primary_match;
	secondary_match.bits.hash_id = 1; /* Indicate secondary hash */
	
	secondary_hash = primary_hash ^ hash_function_mask;

	pte = find_pte_in_pteg((pte_t *) secondary_hash, secondary_match);

#if MEM_STATS
	if (pte != PTE_NULL)
		hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].found_secondary++;
	if ((pte == PTE_NULL) && !allocate)
		hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].not_found++;
#endif /* MEM_STATS */

	if ((pte == PTE_NULL) && allocate) {
		/* pte wasn't found - find one ourselves and fill in
		 * the pte0 entry (mark as invalid though?)
		 */
		pte = find_pte_in_pteg((pte_t *) primary_hash, match_empty);
		if (pte != PTE_NULL) {
				/* Found in primary location, set up pte0 */

			primary_match.bits.valid = 0;
			pte->pte0.word = primary_match.word;
				
#if MEM_STATS
			hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].alloc_primary++;
#endif /* MEM_STATS */

		} else {
			pte = find_pte_in_pteg((pte_t *) secondary_hash,
					       match_empty);
			if (pte != PTE_NULL) {
				/* Found in secondary */
				secondary_match.bits.valid = 0;
				pte->pte0.word = secondary_match.word;
#if MEM_STATS
				hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].alloc_secondary++;
#endif /* MEM_STATS */
			} else {
#if MEM_STATS
				hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].overflow++;
#endif /* MEM_STATS */
				/* Both PTEGs are full - we need to upcall
				 * to pmap to free up an entry, it will
				 * return the entry that has been freed up
				 */
#if DEBUG
				DPRINTF(("find_or_allocate : BOTH PTEGS ARE FULL\n"));
#endif /* DEBUG */
				pte = pmap_pteg_overflow(
					       (pte_t *)primary_hash,
							primary_match,
					       (pte_t *)secondary_hash,
						        secondary_match);
				assert (pte != PTE_NULL);
			}
		}
		/* Set up the new entry - pmap code assumes:
		 * pte0 contains good translation but valid bit is reset
		 * pte1 has been initialised to zero
		 */
		assert(pte->pte0.word != PTE_EMPTY);
		pte->pte1.word = 0;

	}

	return pte; /* Either the address or PTE_NULL */
}

#if	MACH_KDB || MACH_KGDB
/*
 * Same as above but without taking locks or spls...
 */
pte_t *
db_find_or_allocate_pte(space_t sid, vm_offset_t v_addr, boolean_t allocate)
{
	register vm_offset_t	primary_hash,  secondary_hash;
	register pte0_t		primary_match, secondary_match;
	register pte_t		*pte;
	register unsigned int    segment_id;

	assert(!allocate);	/* can't allocate PTE from debugger !!! */

	/* Horrible hack "(union *)&var-> " to retype to union */
	segment_id = (sid << 4) | (v_addr >> 28);

	primary_hash = segment_id ^ ((va_full_t *)&v_addr)->page_index;

	primary_hash = hash_table_base +
		((primary_hash << 6) & hash_function_mask);

	/* Firstly check out the primary pteg */

#if 0 && MACH_VM_DEBUG
	if (sid) {
		DPRINTF(("PRIMARY HASH OF SID 0x%x and addr 0x%x = 0x%x**********\n",
			 sid, v_addr, primary_hash));
	}
#endif

	/* TODO NMGS could be faster if all put on one line with shifts etc,
	 * does -O2 optimise this?
	 */

	primary_match.bits.valid 	= 1;
	/* virtual segment id = concat of space id + seg reg no */
	primary_match.bits.segment_id	= segment_id;
	primary_match.bits.hash_id	= 0;
	primary_match.bits.page_index	= ((va_abbrev_t *)&v_addr)->page_index;


	pte = db_find_pte_in_pteg((pte_t *) primary_hash, primary_match.word);

	if (pte != PTE_NULL) {
#if MEM_STATS
		hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].found_primary++;
#endif /* MEM_STATS */
		assert (pte->pte0.word != PTE_EMPTY);
		return pte;
	}

#if 0 && MACH_VM_DEBUG
	DPRINTF(("find_pte : searching secondary hash\n"));
#endif

	/* pte wasn't found in primary hash location, check secondary */

	secondary_match = primary_match;
	secondary_match.bits.hash_id = 1; /* Indicate secondary hash */
	
	secondary_hash = primary_hash ^ hash_function_mask;

	pte = db_find_pte_in_pteg((pte_t *) secondary_hash, secondary_match.word);

#if MEM_STATS
	if (pte != PTE_NULL)
		hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].found_secondary++;
	if ((pte == PTE_NULL) && !allocate)
		hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].not_found++;
#endif /* MEM_STATS */

	if ((pte == PTE_NULL) && allocate) {
		panic("db_find_or_allocate_pte: can't allocate PTE");
#if 0
		/* pte wasn't found - find one ourselves and fill in
		 * the pte0 entry (mark as invalid though?)
		 */
		pte = db_find_pte_in_pteg((pte_t *) primary_hash, PTE_EMPTY);
		if (pte != PTE_NULL) {
				/* Found in primary location, set up pte0 */

			primary_match.bits.valid = 0;
			pte->pte0.word = primary_match.word;
#if MEM_STATS
			hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].alloc_primary++;
#endif /* MEM_STATS */

		} else {
			pte = db_find_pte_in_pteg((pte_t *) secondary_hash,
						  PTE_EMPTY);
			if (pte != PTE_NULL) {
				/* Found in secondary */
				secondary_match.bits.valid = 0;
				pte->pte0.word = secondary_match.word;
#if MEM_STATS
				hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].alloc_secondary++;
#endif /* MEM_STATS */
			} else {
#if MEM_STATS
				hash_table_stats[cpu_number()].find_or_alloc_calls[allocate].overflow++;
#endif /* MEM_STATS */
				/* Both PTEGs are full - we need to upcall
				 * to pmap to free up an entry, it will
				 * return the entry that has been freed up
				 */
#if DEBUG
				DPRINTF(("find_or_allocate : BOTH PTEGS ARE FULL\n"));
#endif /* DEBUG */
				pte = db_pmap_pteg_overflow(
					       (pte_t *)primary_hash,
							primary_match,
					       (pte_t *)secondary_hash,
						        secondary_match);
				assert (pte != PTE_NULL);
			}
		}
		/* Set up the new entry - pmap code assumes:
		 * pte0 contains good translation but valid bit is reset
		 * pte1 has been initialised to zero
		 */
		assert(pte->pte0.word != PTE_EMPTY);
		pte->pte1.word = 0;
#endif	/* 0 */

	}

	return pte; /* Either the address or PTE_NULL */
}
#endif	/* MACH_KDB || MACH_KGDB*/
