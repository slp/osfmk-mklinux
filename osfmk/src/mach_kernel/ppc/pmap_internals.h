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

/* Things that don't need to be exported from pmap. Putting
 * them here and not in pmap.h avoids major recompiles when
 * modifying something either here or in proc_reg.h
 */

#ifndef _PMAP_INTERNALS_H_
#define _PMAP_INTERNALS_H_

#include <cpus.h>
#include <mach_ldebug.h>
#include <debug.h>

#include <mach/vm_types.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <kern/assert.h>
#include <kern/cpu_number.h>
#include <kern/lock.h>
#include <kern/queue.h>
#include <ppc/proc_reg.h>


/* Page table entries are stored in groups (PTEGS) in a hash table */

#if __PPC__
#if _BIG_ENDIAN == 0
error - bitfield structures are not checked for bit ordering in words
#endif /* _BIG_ENDIAN */
#endif /* __PPC__ */

/*
 * Don't change these structures unless you change the assembly code in
 * locore.s
 */
struct mapping {
	struct mapping *next;		/* MUST BE FIRST - chain off physent */
	union {
		unsigned int word;
		struct {
			vm_offset_t	page:20;  /* virtual page number */
			unsigned int    wired:1;  /* boolean */
			unsigned int	:0;
		} bits;
	} vm_info;
	struct pmap *pmap;
};

#define MAPPING_NULL	((struct mapping *) 0)

/* Get the pte associated with a mapping */
/* Get the mapping associated with a pte */
/* Get space ID from a mapping */

#define MP2PTE(mp) ((pte_t*)&(((pte_t*)hash_table_base)[(mp)-mapping_base]))
#define PTE2MP(pte) (&mapping_base[(pte) - (pte_t *)hash_table_base])
#define MP_SPACE(mp) (MP2PTE(mp)->pte0.bits.segment_id >> 4)

struct phys_entry {
	struct mapping *phys_link;	/* MUST BE FIRST - chain of mappings */
	pte1_t		pte1;		/* referenced/changed/wimg info */
	volatile boolean_t locked;	/* pseudo-lock, protected by physentries_lock */
};

#define PHYS_NULL	((struct phys_entry *)0)

/* Memory may be non-contiguous. This data structure contains info
 * for mapping this non-contiguous space into the contiguous
 * physical->virtual mapping tables. An array of this type is
 * provided to the pmap system at bootstrap by ppc_vm_init.
 *
 * NB : regions must be in order in this structure.
 */

typedef struct mem_region {
	vm_offset_t start;	/* Address of base of region */
	struct phys_entry *phys_table; /* base of region's table */
	unsigned int end;       /* End address+1 */
} mem_region_t;

/* PMAP_MEM_REGION_MAX has a PowerMac dependancy - at least the value of
 * kMaxRAMBanks in ppc/POWERMAC/nkinfo.h
 */
#define PMAP_MEM_REGION_MAX 26

extern mem_region_t pmap_mem_regions[PMAP_MEM_REGION_MAX];
extern int          pmap_mem_regions_count;

/* keep track of free regions of physical memory so that we can offer
 * them up via pmap_next_page later on
 */

#define FREE_REGION_MAX 8
extern mem_region_t free_regions[FREE_REGION_MAX];
extern int          free_regions_count;

/* Prototypes */

struct phys_entry *pmap_find_physentry(vm_offset_t pa);

extern void pmap_free_mapping(struct mapping *mp,
			      struct phys_entry *pp);

extern void pmap_reap_mappings(void);

static struct mapping *pmap_enter_mapping(pmap_t pmap,
					  space_t space,
					  vm_offset_t va,
					  vm_offset_t pa,
					  pte_t *pte,
					  unsigned prot,
					  struct phys_entry *pp);

extern pte_t *pmap_pteg_overflow(pte_t *primary_hash, pte0_t primary_match,
				 pte_t *secondary_hash, pte0_t secondary_key);

/*
 *	Locking Protocols:
 *
 *	There are two structures in the pmap module that need locking:
 *	the pmaps themselves, and the per-page mapping lists (which are locked
 *	by setting the physentry's "locked" flag).
 * 	Most routines want to lock a pmap and then do operations in it that
 * 	require mapping list locking -- however pmap_page_protect and
 *	possibly other routines operate on a physical page basis and want to
 *	do the locking in the reverse order, i.e. lock a phys_entry and then
 *	go through all the pmaps referenced by that phys_entry's mappings.
 *	To protect against deadlock between these two cases, the
 *	pmap_system_lock is used.
 *	There are three different locking protocols as a result:
 *
 *  1.  pmap operations only (pmap_extract, pmap_access, ...)
 *		Lock only the pmap.
 *
 *  2.  pmap-based operations (pmap_enter, pmap_remove, ...)  
 *		Get a read lock on the pmap_system_lock (shared read), then
 *		lock the pmap and finally the phys_entries as needed [i.e. pmap
 *		lock before phys_entry lock.]
 *
 *  3.  phys_entry-based operations (pmap_page_protect, ...)
 *		Get a write lock on the pmap_lock (exclusive write); this
 *		also guaranteees exclusive access to the phys_entries.  Lock the
 *		pmaps as needed.
 *
 *	At no time may any routine hold more than one pmap lock or more than
 *	one phys_entry lock.  Because interrupt level routines can allocate
 *	mbufs and cause pmap_enter's, the pmap_system_lock and the lock on the
 *	kernel_pmap can only be held at splvm.
 */

#if	DEBUG
extern int pmdebug;
#define PDB_LOCK	0x100
#define LOCKPRINTF(args)	if (pmdebug & PDB_LOCK) printf args; else
#else	/* DEBUG */
#define LOCKPRINTF(args)
#endif	/* DEBUG */

#if	NCPUS > 1 || MACH_LDEBUG

decl_simple_lock_data(extern, physentries_lock)
decl_simple_lock_data(extern, pte_lock_table_lock)

#if	MACH_LDEBUG
decl_simple_lock_data(extern, fake_physentry_lock[NCPUS])
extern unsigned int	fake_physentry_lock_count[NCPUS];
decl_simple_lock_data(extern, fake_pte_lock[NCPUS])
extern unsigned int	fake_pte_lock_count[NCPUS];
#endif	/* MACH_LDEBUG */

extern vm_offset_t	hash_table_base;
extern unsigned int	hash_table_size;
extern char		*pte_lock_table;

static __inline__ void
lock_physentry(
	struct phys_entry *phe)
{
#if	DEBUG
	spl_t			s;

	/* Check we are called under SPLVM */
	s = splhigh();
	splx(s);
	assert(s <= SPLVM);
#endif	/* DEBUG */
	LOCKPRINTF((" time %8d thread 0x%x from 0x%x lock_physentry(0x%x)\n",
		    mftb(), (unsigned int)current_thread(),
		    __builtin_return_address(0), phe));
	for (;;) {
		simple_lock(&physentries_lock);
		if (!phe->locked) {
			phe->locked = TRUE;
#if	MACH_LDEBUG
			if (fake_physentry_lock_count[cpu_number()]++ == 0) {
				simple_lock(&fake_physentry_lock[cpu_number()]);
			}
#endif	/* MACH_LDEBUG */
			simple_unlock(&physentries_lock);
			LOCKPRINTF((" time %8d thread 0x%x from 0x%x lock_physentry(0x%x): locked\n",
				    mftb(), (unsigned int)current_thread(),
				    __builtin_return_address(0), phe));
			return;
		}
		simple_unlock(&physentries_lock);
		LOCKPRINTF((" time %8d thread 0x%x from 0x%x lock_physentry(0x%x): blocking on word (%d)\n",
			    mftb(), (unsigned int)current_thread(),
			    __builtin_return_address(0), phe, phe->locked));
#if NCPUS == 1
		/* we shouldn't block here on a uniprocessor machine */
		assert(0);
#endif	/* NCPUS == 1 */
		while (phe->locked);
		LOCKPRINTF((" time %8d thread 0x%x from 0x%x lock_phyentry(0x%x): unblocked\n",
			    mftb(), (unsigned int)current_thread(),
			    __builtin_return_address(0), phe));
	}
}

static __inline__ void
unlock_physentry(
	struct phys_entry *phe)
{
#if	DEBUG
	spl_t			s;

	/* Check we are called under SPLVM */
	s = splhigh();
	splx(s);
	assert(s <= SPLVM);
#endif	/* DEBUG */
	LOCKPRINTF((" time %8d thread 0x%x from 0x%x unlock_physentry(0x%x)\n",
		    mftb(), (unsigned int)current_thread(),
		    __builtin_return_address(0), phe));
	simple_lock(&physentries_lock);
	assert(phe->locked);
	phe->locked = FALSE;
#if	MACH_LDEBUG
	if (--fake_physentry_lock_count[cpu_number()] == 0) {
		simple_unlock(&fake_physentry_lock[cpu_number()]);
	}
#endif	/* MACH_LDEBUG */
	simple_unlock(&physentries_lock);
}

extern int tstbit(int index, char *s);

static __inline__ void
lock_pte(
	pte_t	*pte)
{
	int index;
#if	MACH_LDEBUG
	int busy_timeout_count;
#endif	/* MACH_LDEBUG */

#if	DEBUG
	spl_t			s;

	/* Check we are called under SPLVM */
	s = splhigh();
	splx(s);
	assert(s <= SPLVM);
#endif	/* DEBUG */

	index = pte - (pte_t *) hash_table_base;
	LOCKPRINTF(("  time %8d thread 0x%x from 0x%x lock_pte(0x%x) index=%d\n",
		    mftb(), (unsigned int)current_thread(),
		    __builtin_return_address(0), pte, index));
	assert(index >= 0 && index < hash_table_size / sizeof (pte_t));
	for (;;) {
		simple_lock(&pte_lock_table_lock);
		if (!tstbit(index, (void *) pte_lock_table)) {
			setbit(index, (void *) pte_lock_table);

			assert(tstbit(index, (void *) pte_lock_table));
#if	MACH_LDEBUG
			if (fake_pte_lock_count[cpu_number()]++ == 0) {
				simple_lock(&fake_pte_lock[cpu_number()]);
			}
#endif	/* MACH_LDEBUG */
			simple_unlock(&pte_lock_table_lock);
			LOCKPRINTF(("  time %8d thread 0x%x from 0x%x lock_pte(0x%x): locked\n",
				    mftb(), (unsigned int)current_thread(),
				    __builtin_return_address(0), pte));
			return;
		}
		simple_unlock(&pte_lock_table_lock);
		LOCKPRINTF(("  time %8d thread 0x%x from 0x%x lock_pte(0x%x): blocking, word=0x%08x\n",
			    mftb(), (unsigned int)current_thread(),
			    __builtin_return_address(0), pte,
			    ((int*)pte_lock_table)[index / 32]));
#if	NCPUS == 1
		/* we shouldn't block here on a uniprocessor machine */
		assert(0);
#endif	/* NCPUS == 1 */
#if	MACH_LDEBUG
		busy_timeout_count = 1000000;
#endif	/* MACH_LDEBUG */
		while (tstbit(index, (void *) pte_lock_table)) {
#if	MACH_LDEBUG
			if (busy_timeout_count-- == 0) {
				printf("lock_pte: spinning too long, "
				       "index=%d word=0x%08x, bit = %d\n",
				       index,
				       ((int*)pte_lock_table)[index / 32],
				       (index & 31));
				Debugger("lock_pte spinning too long");
				busy_timeout_count = 1000000;
			}
#endif	/* MACH_LDEBUG */
		}
		LOCKPRINTF(("  time %8d thread 0x%x from 0x%x lock_pte(0x%x): unblocked\n",
			    mftb(), (unsigned int)current_thread(),
			    __builtin_return_address(0), pte));
	}
}

static __inline__ void
unlock_pte(
	pte_t	*pte)
{
	int index;

#if	DEBUG
	spl_t			s;

	/* Check we are called under SPLVM */
	s = splhigh();
	splx(s);
	assert(s <= SPLVM);
#endif	/* DEBUG */

	index = pte - (pte_t *) hash_table_base;
	LOCKPRINTF(("  time %8d thread 0x%x from 0x%x unlock_pte(0x%x) index=%d\n",
		    mftb(), (unsigned int)current_thread(),
		    __builtin_return_address(0), pte, index));
	assert(index >= 0 && index < hash_table_size / sizeof (pte_t));
	simple_lock(&pte_lock_table_lock);
	assert(tstbit(index, (void *) pte_lock_table));
	clrbit(index, (void *) pte_lock_table);
	assert(!tstbit(index, (void *) pte_lock_table));
#if	MACH_LDEBUG
	if (--fake_pte_lock_count[cpu_number()] == 0) {
		simple_unlock(&fake_pte_lock[cpu_number()]);
	}
#endif	/* MACH_LDEBUG */
	simple_unlock(&pte_lock_table_lock);
}

#define PMAP_READ_LOCK(pmap, spl) {					      \
	LOCKPRINTF(("time %8d thread 0x%x from %s:%d PMAP_READ_LOCK(0x%x)\n", \
		    mftb(), (unsigned int)current_thread(),		      \
		    __FILE__, __LINE__, (pmap)));			      \
	spl = splvm();							      \
	lock_read(&pmap_system_lock);					      \
	simple_lock(&(pmap)->lock);					      \
}

#define PMAP_WRITE_LOCK(spl) {						   \
	LOCKPRINTF(("time %8d thread 0x%x from %s:%d PMAP_WRITE_LOCK()\n", \
		    mftb(), (unsigned int)current_thread(),		   \
		    __FILE__, __LINE__));				   \
	spl = splvm();							   \
	lock_write(&pmap_system_lock);					   \
}

#define PMAP_READ_UNLOCK(pmap, spl) {						\
	LOCKPRINTF(("time %8d thread 0x%x from %s:%d PMAP_READ_UNLOCK(0x%x)\n",	\
		    mftb(), (unsigned int)current_thread(),			\
		    __FILE__, __LINE__, (pmap)));				\
	simple_unlock(&(pmap)->lock);						\
	lock_read_done(&pmap_system_lock);					\
	splx(spl);								\
}

#define PMAP_WRITE_UNLOCK(spl) {					     \
	LOCKPRINTF(("time %8d thread 0x%x from %s:%d PMAP_WRITE_UNLOCK()\n", \
		    mftb(), (unsigned int)current_thread(),		     \
		    __FILE__, __LINE__));				     \
	lock_write_done(&pmap_system_lock);				     \
	splx(spl);							     \
}

#define LOCK_PHYSENTRY(phe)	lock_physentry(phe)
#define UNLOCK_PHYSENTRY(phe)	unlock_physentry(phe)

#define LOCK_PTE(pte)		lock_pte(pte)
#define UNLOCK_PTE(pte)		unlock_pte(pte)

#else	/* NCPUS > 1 || MACH_LDEBUG */

#if	MACH_RT
#define PMAP_READ_LOCK(pmap, spl)	{ (spl) = splvm(); }
#define PMAP_WRITE_LOCK(spl)		{ (spl) = splvm(); }
#define PMAP_READ_UNLOCK(pmap, spl)	{ splx(spl); }
#define PMAP_WRITE_UNLOCK(spl)		{ splx(spl); }
#define LOCK_PHYSENTRY(phe)		disable_preemption()
#define UNLOCK_PHYSENTRY(phe)		enable_preemption()
#define LOCK_PTE(pte)			disable_preemption()
#define UNLOCK_PTE(pte)			enable_preemption()
#else	/* MACH_RT */
#define PMAP_READ_LOCK(pmap, spl)	{ (spl) = splvm(); }
#define PMAP_WRITE_LOCK(spl)		{ (spl) = splvm(); }
#define PMAP_READ_UNLOCK(pmap, spl)	{ splx(spl); }
#define PMAP_WRITE_UNLOCK(spl)		{ splx(spl); }
#define LOCK_PHYSENTRY(phe)
#define UNLOCK_PHYSENTRY(phe)
#define LOCK_PTE(pte)
#define UNLOCK_PTE(pte)
#endif	/* MACH_RT */

#endif	/* NCPUS > 1 || MACH_LDEBUG */

#if	NCPUS > 1
#define TLBSYNC_SYNC()							\
	{								\
		if (PROCESSOR_VERSION != PROCESSOR_VERSION_601)		\
			__asm__ volatile("tlbsync ; sync");		\
	} while (0)
#else	/* NCPUS > 1 */
#define TLBSYNC_SYNC()
#endif	/* NCPUS > 1 */


#endif /* _PMAP_INTERNALS_H_ */
