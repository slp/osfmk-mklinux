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
 * INTEL_HISTORY
 * pmap.h,v
 * Revision 1.9  1994/07/22  16:34:38  andyp
 * Added the notion of "private pmaps" -- TLB updates are ignored
 * on private pmaps.  Added support for "show pmap" debugger command.
 *
 * Revision 1.8  1994/06/15  19:46:28  lenb
 *         delete reference to paragon_mark_cpu_[active|idle](),
 *         defining led_set()/led_clear() to be empty
 *
 * Revision 1.7  1994/03/19  01:18:09  lenb
 * Benefit: ASMP kernel: SIMPLE_LOCK for pmap->lock
 * Testing: SAT
 * Reviewer: sean
 *
 * Revision 1.6  1993/06/30  22:41:49  dleslie
 * Adding copyright notices required by legal folks
 *
 * Revision 1.5  1993/04/27  20:32:26  dleslie
 * Copy of R1.0 sources onto main trunk
 *
 * Revision 1.4.6.1  1993/04/22  18:32:13  dleslie
 * First R1_0 release
 *
 * Revision 1.5  1993/04/05  02:49:15  andyp
 * Prohibit the pmap_enter()-ing non-dram addresses with cacheing.
 * Added the beginnings of pmap_attribute().
 *
 * Revision 1.4  1992/12/04  06:29:19  andyp
 * afs ate our pmap.h,v.
 *
 * 04-Dec-92 Recreated and remarked the log.  Our revs used to look like:
 *
 * 	Revision 1.3  1992/11/28  18:14:24  andyp
 * 	Added definition for 4M pages for the i860XP.
 *
 * 	Revision 1.2  1992/11/20  19:57:29  andyp
 * 	Record the time when going active and/or going idle.
 *
 * 	Revision 1.1  1992/10/15  23:56:19  SSD
 * 	Initial revision
 *
 * Revision 2.7.4.3  92/04/30  11:58:51  bernadat
 * 	No leds for SystemPro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.7.4.2  92/03/28  10:09:14  jeffreyh
 * 	"Wired" for the 860 (A & D bits); New lightshow for iPSC386
 * 	[92/03/20            jeffreyh]
 * 
 * Revision 2.7.4.1  92/02/18  19:05:21  jeffreyh
 * 	Use leds on Sequent
 * 	[91/12/06            bernadat]
 * 
 * 	Support for Corollary MP leds
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.7  91/12/10  16:32:23  jsb
 * 	Fixes from Intel
 * 	[91/12/10  15:51:47  jsb]
 * 
 * Revision 2.6  91/08/28  11:13:15  jsb
 * 	From Intel SSD: turn off caching for i860 for now.
 * 	[91/08/26  18:31:19  jsb]
 * 
 * Revision 2.5  91/05/14  16:30:44  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/05/08  12:46:54  dbg
 * 	Add volatile declarations.  Load CR3 when switching to kernel
 * 	pmap in PMAP_ACTIVATE_USER.  Fix PMAP_ACTIVATE_KERNEL.
 * 	[91/04/26  14:41:54  dbg]
 * 
 * Revision 2.3  91/02/05  17:20:49  mrt
 * 	Changed to new Mach copyright
 * 	[91/01/31  18:17:51  mrt]
 * 
 * Revision 2.2  90/12/04  14:50:35  jsb
 * 	First checkin (for intel directory).
 * 	[90/12/03  21:55:40  jsb]
 * 
 * Revision 2.4  90/08/06  15:07:23  rwd
 * 	Remove ldt (not used).
 * 	[90/07/17            dbg]
 * 
 * Revision 2.3  90/06/02  14:48:53  rpd
 * 	Added PMAP_CONTEXT definition.
 * 	[90/06/02            rpd]
 * 
 * Revision 2.2  90/05/03  15:37:16  dbg
 * 	Move page-table definitions into i386/pmap.h.
 * 	[90/04/05            dbg]
 * 
 * 	Define separate Write and User bits in pte instead of protection
 * 	code.
 * 	[90/03/25            dbg]
 * 
 * 	Load dirbase directly from pmap.  Split PMAP_ACTIVATE and
 * 	PMAP_DEACTIVATE into separate user and kernel versions.
 * 	[90/02/08            dbg]
 * 
 * Revision 1.6  89/09/25  12:25:50  rvb
 * 	seg_desc -> fakedesc
 * 	[89/09/23            rvb]
 * 
 * Revision 1.5  89/09/05  20:41:38  jsb
 * 	Added pmap_phys_to_frame definition.
 * 	[89/09/05  18:47:08  jsb]
 * 
 * Revision 1.4  89/03/09  20:03:34  rpd
 * 	More cleanup.
 * 
 * Revision 1.3  89/02/26  12:33:18  gm0w
 * 	Changes for cleanup.
 * 
 * 31-Dec-88  Robert Baron (rvb) at Carnegie-Mellon University
 *	Derived from MACH2.0 vax release.
 *
 * 17-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	MARK_CPU_IDLE and MARK_CPU_ACTIVE must manipulate a separate
 *	cpu_idle set.  The scheduler's cpu_idle indication is NOT
 *	synchronized with these calls.  MARK_CPU_ACTIVE also needs spls.
 *
 */
/* CMU_HIST */
/*
 * Revision 2.7.4.3  92/04/30  11:58:51  bernadat
 * 	No leds for SystemPro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.7.4.2  92/03/28  10:09:14  jeffreyh
 * 	"Wired" for the 860 (A & D bits); New lightshow for iPSC386
 * 	[92/03/20            jeffreyh]
 * 
 * Revision 2.7.4.1  92/02/18  19:05:21  jeffreyh
 * 	Use leds on Sequent
 * 	[91/12/06            bernadat]
 * 
 * 	Support for Corollary MP leds
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.7  91/12/10  16:32:23  jsb
 * 	Fixes from Intel
 * 	[91/12/10  15:51:47  jsb]
 * 
 * Revision 2.6  91/08/28  11:13:15  jsb
 * 	From Intel SSD: turn off caching for i860 for now.
 * 	[91/08/26  18:31:19  jsb]
 * 
 * Revision 2.5  91/05/14  16:30:44  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/05/08  12:46:54  dbg
 * 	Add volatile declarations.  Load CR3 when switching to kernel
 * 	pmap in PMAP_ACTIVATE_USER.  Fix PMAP_ACTIVATE_KERNEL.
 * 	[91/04/26  14:41:54  dbg]
 * 
 * Revision 2.3  91/02/05  17:20:49  mrt
 * 	Changed to new Mach copyright
 * 	[91/01/31  18:17:51  mrt]
 * 
 * Revision 2.2  90/12/04  14:50:35  jsb
 * 	First checkin (for intel directory).
 * 	[90/12/03  21:55:40  jsb]
 * 
 * Revision 2.4  90/08/06  15:07:23  rwd
 * 	Remove ldt (not used).
 * 	[90/07/17            dbg]
 * 
 * Revision 2.3  90/06/02  14:48:53  rpd
 * 	Added PMAP_CONTEXT definition.
 * 	[90/06/02            rpd]
 * 
 * Revision 2.2  90/05/03  15:37:16  dbg
 * 	Move page-table definitions into i386/pmap.h.
 * 	[90/04/05            dbg]
 * 
 * 	Define separate Write and User bits in pte instead of protection
 * 	code.
 * 	[90/03/25            dbg]
 * 
 * 	Load dirbase directly from pmap.  Split PMAP_ACTIVATE and
 * 	PMAP_DEACTIVATE into separate user and kernel versions.
 * 	[90/02/08            dbg]
 * 
 * Revision 1.6  89/09/25  12:25:50  rvb
 * 	seg_desc -> fakedesc
 * 	[89/09/23            rvb]
 * 
 * Revision 1.5  89/09/05  20:41:38  jsb
 * 	Added pmap_phys_to_frame definition.
 * 	[89/09/05  18:47:08  jsb]
 * 
 * Revision 1.4  89/03/09  20:03:34  rpd
 * 	More cleanup.
 * 
 * Revision 1.3  89/02/26  12:33:18  gm0w
 * 	Changes for cleanup.
 * 
 * 31-Dec-88  Robert Baron (rvb) at Carnegie-Mellon University
 *	Derived from MACH2.0 vax release.
 *
 * 17-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	MARK_CPU_IDLE and MARK_CPU_ACTIVE must manipulate a separate
 *	cpu_idle set.  The scheduler's cpu_idle indication is NOT
 *	synchronized with these calls.  MARK_CPU_ACTIVE also needs spls.
 *
 */
/* CMU_ENDHIST */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */

/*
 *	File:	pmap.h
 *
 *	Authors:  Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Machine-dependent structures for the physical map module.
 */

#ifndef	_PMAP_MACHINE_
#define _PMAP_MACHINE_	1

#ifndef	ASSEMBLER

#include <platforms.h>
#include <mp_v1_1.h>

#include <mach/kern_return.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <mach/machine/vm_param.h>
#include <kern/lock.h>

/*
 *	Define the generic in terms of the specific
 */

#if	i386
#define	INTEL_PGBYTES		I386_PGBYTES
#define INTEL_PGSHIFT		I386_PGSHIFT
#define	intel_btop(x)		i386_btop(x)
#define	intel_ptob(x)		i386_ptob(x)
#define	intel_round_page(x)	i386_round_page(x)
#define	intel_trunc_page(x)	i386_trunc_page(x)
#define trunc_intel_to_vm(x)	trunc_i386_to_vm(x)
#define round_intel_to_vm(x)	round_i386_to_vm(x)
#define vm_to_intel(x)		vm_to_i386(x)
#endif	/* i386 */
#if	i860
#define	INTEL_PGBYTES		I860_PGBYTES
#define INTEL_PGSHIFT		I860_PGSHIFT
#define	intel_btop(x)		i860_btop(x)
#define	intel_ptob(x)		i860_ptob(x)
#define	intel_round_page(x)	i860_round_page(x)
#define	intel_trunc_page(x)	i860_trunc_page(x)
#define trunc_intel_to_vm(x)	trunc_i860_to_vm(x)
#define round_intel_to_vm(x)	round_i860_to_vm(x)
#define vm_to_intel(x)		vm_to_i860(x)
#endif	/* i860 */

/*
 *	i386/i486/i860 Page Table Entry
 */

typedef unsigned int	pt_entry_t;
#define PT_ENTRY_NULL	((pt_entry_t *) 0)

#endif	/* ASSEMBLER */

#define INTEL_OFFMASK	0xfff	/* offset within page */
#define PDESHIFT	22	/* page descriptor shift */
#define PDEMASK		0x3ff	/* mask for page descriptor index */
#define PTESHIFT	12	/* page table shift */
#define PTEMASK		0x3ff	/* mask for page table index */

/*
 *	Convert kernel virtual address to linear address
 */

#define kvtolinear(a)	((a)+LINEAR_KERNEL_ADDRESS)

/*
 *	Convert address offset to page descriptor index
 */
#define pdenum(pmap, a)	(((((pmap) == kernel_pmap) ?	\
			   kvtolinear(a) : (a))		\
			  >> PDESHIFT) & PDEMASK)

/*
 *	Convert page descriptor index to user virtual address
 */
#define pdetova(a)	((vm_offset_t)(a) << PDESHIFT)

/*
 *	Convert address offset to page table index
 */
#define ptenum(a)	(((a) >> PTESHIFT) & PTEMASK)

#define NPTES	(intel_ptob(1)/sizeof(pt_entry_t))
#define NPDES	(intel_ptob(1)/sizeof(pt_entry_t))

/*
 *	Hardware pte bit definitions (to be used directly on the ptes
 *	without using the bit fields).
 */

#if	i860
#define INTEL_PTE_valid		0x00000001
#else
#define INTEL_PTE_VALID		0x00000001
#endif
#define INTEL_PTE_WRITE		0x00000002
#define INTEL_PTE_USER		0x00000004
#define INTEL_PTE_WTHRU		0x00000008
#define INTEL_PTE_NCACHE 	0x00000010
#define INTEL_PTE_REF		0x00000020
#define INTEL_PTE_MOD		0x00000040
#if	i860
#if	i860XP
#define INTEL_PTE_4M		0x00000080
#endif	/* i860XP */
#define	INTEL_PTE_wired		0x00000200
#else
#define INTEL_PTE_WIRED		0x00000200
#endif
#define INTEL_PTE_PFN		0xfffff000

#if	i860
/*
 *	"wired" implies a fault-free page; the i860 will
 *	generate traps on stores to an unmodified page and/or
 *	accesses to an unreferenced page during a locked
 *	sequence.
 *
 *	This used to define WIRED to be WIRED|REF|MOD, but this
 *	cannot be correctly substituted for just the WIRED bit
 *	in all cases.  Instead, the three bits are explicitly used
 *	as needed in pmap.c.
 */
#define	INTEL_PTE_WIRED		(INTEL_PTE_wired	\
				|INTEL_PTE_REF		\
				|INTEL_PTE_MOD		\
				)
#if	NOCACHE
#define	INTEL_PTE_VALID		(INTEL_PTE_valid	\
				|INTEL_PTE_WTHRU	\
				|INTEL_PTE_NCACHE	\
				)
#else	/* NOCACHE */
#define	INTEL_PTE_VALID		(INTEL_PTE_valid	\
				)
#endif	/* NOCACHE */


#endif	/* i860 */

#define	pa_to_pte(a)		((a) & INTEL_PTE_PFN)
#define	pte_to_pa(p)		((p) & INTEL_PTE_PFN)
#define	pte_increment_pa(p)	((p) += INTEL_OFFMASK+1)

/*
 *	Convert page table entry to kernel virtual address
 */
#define ptetokv(a)	(phystokv(pte_to_pa(a)))

#ifndef	ASSEMBLER
typedef	volatile long	cpu_set;	/* set of CPUs - must be <= 32 */
					/* changed by other processors */

struct pmap {
	pt_entry_t	*dirbase;	/* page directory pointer register */
	vm_offset_t	pdirbase;	/* phys. address of dirbase */
	int		ref_count;	/* reference count */
	decl_simple_lock_data(,lock)	/* lock on map */
	struct pmap_statistics	stats;	/* map statistics */
	cpu_set		cpus_using;	/* bitmap of cpus using pmap */
#if	PARAGON860
	boolean_t	private_pmap;	/* if set, flush_tlb() not needed */
#endif	/* PARAGON860 */
};

typedef struct pmap	*pmap_t;

#define PMAP_NULL	((pmap_t) 0)

#if	PARAGON860
#if	NORMA2
typedef char *pmap_frozen_t;
#endif	/* NORMA2 */
#endif	/* PARAGON860 */

extern pmap_t	kernel_pmap;			/* The kernel's map */

/* 
 * Optimization avoiding some TLB flushes when switching to
 * kernel-loaded threads.  This is effective only for i386:
 * Since user task, kernel task and kernel loaded tasks share the
 * same virtual space (with appropriate protections), any pmap
 * allows mapping kernel and kernel loaded tasks. 
 *
 * The idea is to avoid switching to another pmap unnecessarily when
 * switching to a kernel-loaded task, or when switching to the kernel
 * itself.
 *
 * We store the pmap we are really using (from which we fetched the
 * dirbase value) in real_pmap[cpu_number()].
 *
 * Invariant:
 * current_pmap() == real_pmap[cpu_number()] || current_pmap() == kernel_pmap.
 */

extern pmap_t	real_pmap[NCPUS];

#if	i860
/*#define	set_dirbase(dirbase)	flush_and_ctxsw(dirbase)*//*akp*/
#endif	/* i860 */

#if	i386
#include <i386/proc_reg.h>
/*
 * If switching to the kernel pmap, don't incur the TLB cost of switching
 * to its page tables, since all maps include the kernel map as a subset.
 * Simply record that this CPU is logically on the kernel pmap (see
 * pmap_destroy).
 * 
 * Similarly, if switching to a pmap (other than kernel_pmap) that is already
 * in use, don't do anything to the hardware, to avoid a TLB flush.
 */

#if	NCPUS > 1
#define	PMAP_CPU_SET(pmap, my_cpu) i_bit_set(my_cpu, &((pmap)->cpus_using))
#define	PMAP_CPU_CLR(pmap, my_cpu) i_bit_clear(my_cpu, &((pmap)->cpus_using))
#else	/* NCPUS > 1 */
#define	PMAP_CPU_SET(pmap,my_cpu)    (pmap)->cpus_using = TRUE	
#define	PMAP_CPU_CLR(pmap,my_cpu)    (pmap)->cpus_using = FALSE
#endif	/* NCPUS > 1 */


#define	set_dirbase(pmap, my_cpu) {					\
	pmap_t		*ppmap = &real_pmap[my_cpu];			\
	vm_offset_t	pdirbase = pmap->pdirbase;			\
									\
	if (*ppmap == (vm_offset_t)NULL) {				\
		*ppmap = pmap;						\
		PMAP_CPU_SET(pmap, my_cpu);				\
		set_cr3(pdirbase);					\
	} else if (pmap != kernel_pmap && pmap != *ppmap ) {		\
		if (*ppmap != kernel_pmap)				\
			PMAP_CPU_CLR(*ppmap, my_cpu);			\
		*ppmap = pmap;						\
		PMAP_CPU_SET(pmap, my_cpu);				\
		set_cr3(pdirbase);					\
	}								\
	assert(pmap == *ppmap || pmap == kernel_pmap);			\
}
#endif	/* i386 */

#if	PARAGON860
#define	set_led(cpu)
#define clear_led(cpu)
#endif	/* PARAGON860 */

#if	NCPUS > 1
/*
 *	List of cpus that are actively using mapped memory.  Any
 *	pmap update operation must wait for all cpus in this list.
 *	Update operations must still be queued to cpus not in this
 *	list.
 */
extern cpu_set		cpus_active;

/*
 *	List of cpus that are idle, but still operating, and will want
 *	to see any kernel pmap updates when they become active.
 */
extern cpu_set		cpus_idle;

/*
 *	Quick test for pmap update requests.
 */
extern volatile
boolean_t	cpu_update_needed[NCPUS];

/*
 *	External declarations for PMAP_ACTIVATE.
 */

extern void		process_pmap_updates(pmap_t pmap);
extern void		pmap_update_interrupt(void);
extern pmap_t		kernel_pmap;

#endif	/* NCPUS > 1 */

/*
 *	Machine dependent routines that are used only for i386/i486/i860.
 */
extern vm_offset_t	(phystokv)(
				vm_offset_t	pa);

extern vm_offset_t	(kvtophys)(
				vm_offset_t	addr);

extern pt_entry_t	*pmap_pte(
				pmap_t		pmap,
				vm_offset_t	addr);

extern vm_offset_t	pmap_map(
				vm_offset_t	virt,
				vm_offset_t	start,
				vm_offset_t	end,
				vm_prot_t	prot);

extern vm_offset_t	pmap_map_bd(
				vm_offset_t	virt,
				vm_offset_t	start,
				vm_offset_t	end,
				vm_prot_t	prot);

extern void		pmap_bootstrap(
				vm_offset_t	load_start);

extern boolean_t	pmap_valid_page(
				vm_offset_t	pa);

extern int		pmap_list_resident_pages(
				pmap_t		pmap,
				vm_offset_t	*listp,
				int		space);

extern void		flush_tlb(void);

/*
 *	Macros for speed.
 */

#if	NCPUS > 1

#include <kern/spl.h>

/*
 *	For multiple CPUS, PMAP_ACTIVATE and PMAP_DEACTIVATE must manage
 *	fields to control TLB invalidation on other CPUS.
 */

#define	PMAP_ACTIVATE_KERNEL(my_cpu)	{				\
									\
	/*								\
	 *	Let pmap updates proceed while we wait for this pmap.	\
	 */								\
	i_bit_clear((my_cpu), &cpus_active);				\
									\
	/*								\
	 *	Lock the pmap to put this cpu in its active set.	\
	 *	Wait for updates here.					\
	 */								\
	simple_lock(&kernel_pmap->lock);				\
									\
	/*								\
	 *	Process invalidate requests for the kernel pmap.	\
	 */								\
	if (cpu_update_needed[(my_cpu)])				\
	    process_pmap_updates(kernel_pmap);				\
									\
	/*								\
	 *	Mark that this cpu is using the pmap.			\
	 */								\
	i_bit_set((my_cpu), &kernel_pmap->cpus_using);			\
									\
	/*								\
	 *	Mark this cpu active - IPL will be lowered by		\
	 *	load_context().						\
	 */								\
	i_bit_set((my_cpu), &cpus_active);				\
									\
	simple_unlock(&kernel_pmap->lock);				\
}

#define	PMAP_DEACTIVATE_KERNEL(my_cpu)	{				\
	/*								\
	 *	Mark pmap no longer in use by this cpu even if		\
	 *	pmap is locked against updates.				\
	 */								\
	i_bit_clear((my_cpu), &kernel_pmap->cpus_using);		\
}

#define PMAP_ACTIVATE_MAP(map, my_cpu)	{				\
	register pmap_t		tpmap;					\
									\
	tpmap = vm_map_pmap(map);					\
	if (tpmap == kernel_pmap) {					\
	    /*								\
	     *	If this is the kernel pmap, switch to its page tables.	\
	     */								\
	    set_dirbase(kernel_pmap, my_cpu);				\
	}								\
	else {								\
	    /*								\
	     *	Let pmap updates proceed while we wait for this pmap.	\
	     */								\
	    i_bit_clear((my_cpu), &cpus_active);			\
									\
	    /*								\
	     *	Lock the pmap to put this cpu in its active set.	\
	     *	Wait for updates here.					\
	     */								\
	    simple_lock(&tpmap->lock);					\
									\
	    /*								\
	     *	No need to invalidate the TLB - the entire user pmap	\
	     *	will be invalidated by reloading dirbase.		\
	     */								\
	    set_dirbase(tpmap, my_cpu);					\
									\
	    /*								\
	     *	Mark this cpu active - IPL will be lowered by		\
	     *	load_context().						\
	     */								\
	    i_bit_set((my_cpu), &cpus_active);				\
									\
	    simple_unlock(&tpmap->lock);				\
	}								\
}

#define PMAP_DEACTIVATE_MAP(map, my_cpu)

#define PMAP_ACTIVATE_USER(th, my_cpu)	{				\
	spl_t		spl;						\
									\
	spl = splvm();							\
	PMAP_ACTIVATE_MAP(th->map, my_cpu)				\
	splx(spl);							\
}

#define PMAP_DEACTIVATE_USER(th, my_cpu)	{			\
	spl_t		spl;						\
									\
	spl = splvm();							\
	PMAP_DEACTIVATE_MAP(th->map, my_cpu)				\
	splx(spl);							\
}

#define	PMAP_SWITCH_CONTEXT(old_th, new_th, my_cpu) {			\
	spl_t		spl;						\
									\
	if (old_th->map != new_th->map) {				\
		spl = splvm();						\
		PMAP_DEACTIVATE_MAP(old_th->map, my_cpu);		\
		PMAP_ACTIVATE_MAP(new_th->map, my_cpu);			\
		splx(spl);						\
	}								\
}

#define	PMAP_SWITCH_USER(th, new_map, my_cpu) {				\
	spl_t		spl;						\
									\
	spl = splvm();							\
	PMAP_DEACTIVATE_MAP(th->map, my_cpu);				\
	th->map = new_map;						\
	PMAP_ACTIVATE_MAP(th->map, my_cpu);				\
	splx(spl);							\
}

#ifdef	SYMMETRY
#include <sqt/clkarb.h>
#define	set_led	light_on
#define clear_led light_off
#endif	/* SYMMETRY */

#ifdef	SQT
/*
 * For now, we have not implementated set and clear.
 */
#define	set_led(cpu)
#define	clear_led(cpu)
#endif	/* SQT */

#ifdef	MBUS
#define	set_led(cpu)
#define clear_led(cpu)
#endif	/* MBUS */

#ifdef	CBUS
/*
 * Get led routine declarations.
 */
#include <busses/cbus/cbus.h>
#endif	/* CBUS */

#if	MP_V1_1
#define	set_led(cpu)
#define clear_led(cpu)
#endif	/* MP_V1_1  */

#define MARK_CPU_IDLE(my_cpu)	{					\
	/*								\
	 *	Mark this cpu idle, and remove it from the active set,	\
	 *	since it is not actively using any pmap.  Signal_cpus	\
	 *	will notice that it is idle, and avoid signaling it,	\
	 *	but will queue the update request for when the cpu	\
	 *	becomes active.						\
	 */								\
	int	s = splvm();						\
	i_bit_set((my_cpu), &cpus_idle);				\
	i_bit_clear((my_cpu), &cpus_active);				\
	splx(s);							\
	set_led(my_cpu); 						\
}

#define MARK_CPU_ACTIVE(my_cpu)	{					\
									\
	int	s = splvm();						\
	/*								\
	 *	If a kernel_pmap update was requested while this cpu	\
	 *	was idle, process it as if we got the interrupt.	\
	 *	Before doing so, remove this cpu from the idle set.	\
	 *	Since we do not grab any pmap locks while we flush	\
	 *	our TLB, another cpu may start an update operation	\
	 *	before we finish.  Removing this cpu from the idle	\
	 *	set assures that we will receive another update		\
	 *	interrupt if this happens.				\
	 */								\
	i_bit_clear((my_cpu), &cpus_idle);				\
									\
	if (cpu_update_needed[(my_cpu)])				\
	    pmap_update_interrupt();					\
									\
	/*								\
	 *	Mark that this cpu is now active.			\
	 */								\
	i_bit_set((my_cpu), &cpus_active);				\
	splx(s);							\
	clear_led(my_cpu); 						\
}

#else	/* NCPUS > 1 */

#ifdef	CBUS
#define MARK_CPU_IDLE(my_cpu)	set_led(my_cpu)
#define MARK_CPU_ACTIVE(my_cpu)	clear_led(my_cpu)
#endif	/* CBUS */

#ifdef	iPSC386
#define MARK_CPU_IDLE(my_cpu)	led_idle()
#define MARK_CPU_ACTIVE(my_cpu)	led_active()
#endif	/* iPSC386 */

#ifdef	PARAGON860
#define MARK_CPU_IDLE(my_cpu)	set_led(my_cpu)
#define MARK_CPU_ACTIVE(my_cpu)	clear_led(my_cpu)
#endif	/* PARAGON860 */

/*
 *	With only one CPU, we just have to indicate whether the pmap is
 *	in use.
 */

#define	PMAP_ACTIVATE_KERNEL(my_cpu)	{				\
	kernel_pmap->cpus_using = TRUE;					\
}

#define	PMAP_DEACTIVATE_KERNEL(my_cpu)	{				\
	kernel_pmap->cpus_using = FALSE;				\
}

#define	PMAP_ACTIVATE_MAP(map, my_cpu)					\
	set_dirbase(vm_map_pmap(map), my_cpu)

#define PMAP_DEACTIVATE_MAP(map, my_cpu)

#define PMAP_ACTIVATE_USER(th, my_cpu)					\
	PMAP_ACTIVATE_MAP(th->map, my_cpu)

#define PMAP_DEACTIVATE_USER(th, my_cpu) 				\
	PMAP_DEACTIVATE_MAP(th->map, my_cpu)

#define	PMAP_SWITCH_CONTEXT(old_th, new_th, my_cpu) {			\
	if (old_th->map != new_th->map) {				\
		PMAP_DEACTIVATE_MAP(old_th->map, my_cpu);		\
		PMAP_ACTIVATE_MAP(new_th->map, my_cpu);			\
	}								\
}

#define	PMAP_SWITCH_USER(th, new_map, my_cpu) {				\
	PMAP_DEACTIVATE_MAP(th->map, my_cpu);				\
	th->map = new_map;						\
	PMAP_ACTIVATE_MAP(th->map, my_cpu);				\
}

#endif	/* NCPUS > 1 */

#define PMAP_CONTEXT(pmap, thread)

#define pmap_kernel_va(VA)	\
	(((VA) >= VM_MIN_KERNEL_ADDRESS) && ((VA) <= VM_MAX_KERNEL_ADDRESS))

#define	pmap_kernel()			(kernel_pmap)
#define pmap_resident_count(pmap)	((pmap)->stats.resident_count)
#define pmap_phys_address(frame)	((vm_offset_t) (intel_ptob(frame)))
#define pmap_phys_to_frame(phys)	((int) (intel_btop(phys)))
#define	pmap_copy(dst_pmap,src_pmap,dst_addr,len,src_addr)
#ifndef	PARAGON860
#define	pmap_attribute(pmap,addr,size,attr,value) \
					(KERN_INVALID_ADDRESS)
#endif	/* PARAGON860 */

#endif	/* ASSEMBLER */

#endif	/* _PMAP_MACHINE_ */
