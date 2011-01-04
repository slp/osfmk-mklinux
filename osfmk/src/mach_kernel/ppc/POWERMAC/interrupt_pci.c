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

#include <mach_kgdb.h>
#include <mach_kdb.h>
#include <debug.h>
#include <mach_mp_debug.h>
#include <mach_debug.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>

#include <kgdb/gdb_defs.h>
#include <kgdb/kgdb_defs.h>	/* For kgdb_printf */

#include <ppc/spl.h>
#include <ppc/proc_reg.h>
#include <ppc/misc_protos.h>
#include <ppc/trap.h>
#include <ppc/exception.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/powermac_pci.h>
#include <ppc/endian.h>

/* PDM interrupt support is very limited, a certain amount of
 * masking can be done using the ICR but we don't do this yet.
 * SPLs are currently all or nothing.
 */


static void	pci_interrupt(int type,
			struct ppc_saved_state *ssp,
			unsigned int dsisr, unsigned int dar, spl_t old_lvl);
static void	pci_register_ofint(int ofirq, spl_t level,
				void (*handler)(int, void *));

static void pci_add_spl(int level, unsigned long mask);

/* Layout of the Grand Central (and derived) interrupt controller */

struct gc_int_controller {
	volatile unsigned long	events;
	volatile unsigned long	mask;
	volatile unsigned long	clear;
	volatile unsigned long	levels;
} *gc_ints;

/* Two identical functions with different names to allow single stepping
 * of pdm_spl whilst in kdb. Define contents in a macro so that it stays
 * identical!
 */

#define SPL_FUNCTION(NAME)						\
static spl_t NAME(spl_t lvl);						\
spl_t NAME(spl_t lvl)							\
{									\
	spl_t old_level;						\
									\
	old_level = get_interrupt_level();				\
									\
	lvl = lvl & ~SPL_LOWER_MASK;					\
	if (lvl == SPLLO) {						\
		cpu_data[cpu_number()].interrupt_level = lvl;		\
		interrupt_enable();					\
	} else {							\
		interrupt_disable();					\
		cpu_data[cpu_number()].interrupt_level = lvl;		\
	}								\
									\
	/* Make sure that the write completes before we continue */	\
									\
	sync();								\
									\
	return old_level;						\
}

#if (SPLLO < SPLHI) || (SPLOFF != 0)
Yikes! The SPL values changed! Gotta fix em!
#endif
#if	MACH_MP_DEBUG
/* when debugging we let clock ticks through at splvm in order to catch
 * any deadlocks, however the rtclock code just checks for deadlock
 * without doing a tick if we are at splvm with MACH_MP_DEBUG
 */
#define	SPL_IRQS_DISABLED	SPLCLOCK
#else	/* MACH_MP_DEBUG */
/* SPLVM is at the same level as SPLCLOCK (and SPLHIGH) normally */
#define SPL_IRQS_DISABLED	SPLVM
#endif	/* MACH_MP_DEBUG */

unsigned long	pci_spl_mask[SPLLO+1];
unsigned long	pci_pending_irqs = 0;

unsigned long	pci_soft_desired_mask,pci_soft_current_mask;

#if DEBUG
vm_offset_t spl_addr[NCPUS];
boolean_t check_spl_inversion = FALSE;
int pci_in_interrupt[NCPUS];
#endif

static spl_t pci_spl(spl_t lvl)
{
	spl_t old_level;
	unsigned long events, flags;
	int mycpu;
	int lowering;

	interrupt_disable();

	mycpu = cpu_number();

	old_level = get_interrupt_level();

	lowering = lvl & SPL_LOWER_MASK;
	lvl = lvl & ~SPL_LOWER_MASK;

#if DEBUG
	if ((pci_in_interrupt[mycpu] != 0) && old_level > SPLHIGH) {
		printf("In interrupt with old_level != SPLHIGH, "
			"lvl=%d, old_level=%d\n", lvl, old_level);
		Debugger("BOGUS OLD_LEVEL");
	}		

	/* Only start checking SPL inversions after we have
	 * really started running with all interrupts enabled,
	 * not during kernel setup
	 */
	if (lvl == SPLLO) {
		check_spl_inversion = TRUE;
	}
	if (check_spl_inversion &&
	    pci_in_interrupt[mycpu] == 0 &&
	    SPL_CMP_GT(lvl, old_level) && !lowering) {
		printf("SPL inversion from %d at 0x%08x to %d at 0x%08x\n",
		       old_level, spl_addr[mycpu],
		       lvl, (unsigned long) __builtin_return_address(0));
		Debugger("SPL INVERSION");
	}
	if (!lowering) {
	    spl_addr[mycpu] = (unsigned long)__builtin_return_address(0);
	}
#endif
	/* Only open up new interrupts if we asked to lower spl level */
	if (!lowering && (lvl > old_level))
		lvl = old_level;

#if DEBUG
	if (pci_in_interrupt[mycpu] && lvl > SPLHIGH) {
		Debugger("Should be at SPLHIGH whilst in interrupt\n");
	}
#endif
	cpu_data[mycpu].interrupt_level = lvl; sync();

	/*
	 * Try to avoid touching hardware registers. We do not disable
	 * interrupts when calling spl, we simply set a variable
	 * to indicate that they should not occur. The interrupt handler
	 * will then ignore the interrupt and mask it off if it does
	 * occur. This is because the icr control register costs a LOT
	 * to access.
	 */

	if (mycpu == master_cpu) {
		if (lvl > SPL_IRQS_DISABLED) {

			pci_soft_desired_mask = pci_spl_mask[lvl];

			/* Were we not being lazy about the interrupt
			 * masking, we should change the hardware mask here
			 */

			if (old_level < lvl) {
				/* Only checked for missed interrupts
				 * when SPL is being lowered 
				 * (this a g/c bug since an interrupt
				 * will not be reasserted if it was
				 * previously masked)
				 */

				if ((pci_soft_current_mask &
				     pci_soft_desired_mask) !=
				    pci_soft_desired_mask) {
					
					/* an interrupt is disabled that
					 * should now be enabled - fix it
					 */

					pci_soft_current_mask =
						pci_soft_desired_mask;

					gc_ints->mask =
						pci_soft_current_mask; eieio();
				}

				pci_pending_irqs |= gc_ints->levels;
				eieio();

				if (pci_pending_irqs &
				    pci_soft_desired_mask) {
			    
					/* This causes us to
					 * fake a T_INTERRUPT
					 * exception HARDWARE
					 * INTERRUPTS MUST BE
					 * DISABLED when
					 * entering this
					 * routine
					 */
					create_fake_interrupt();
				}
			}
			interrupt_enable();
		}
	} else {
		/* We are not on the master CPU, can only do OFF/ON */
		if (lvl > SPL_IRQS_DISABLED) {
			interrupt_enable();
		}
	}
	
	return old_level;
}

#if	MACH_KDB
SPL_FUNCTION(pci_db_spl)
#endif	/* MACH_KDB */

static void	pci_via1_interrupt (int device, void *ssp);

#define NVIA1_INTERRUPTS 7
struct powermac_interrupt pci_via1_interrupts[NVIA1_INTERRUPTS] = {
	{ 0,	0,	-1},			/* Cascade */
	{ 0,	0,	PMAC_DEV_HZTICK},
	{ 0,	0,	PMAC_DEV_CUDA},		
	{ 0,	0, 	-1},			/* VIA Data */
	{ 0,	0, 	PMAC_DEV_PMU},		/* VIA CLK Source */
	{ 0,	0,	PMAC_DEV_TIMER2},
	{ 0,	0,	PMAC_DEV_TIMER1}
};

#define NPCI_INTERRUPTS 32

/* This structure is little-endian formatted... */

struct powermac_interrupt  powermac_pci_interrupts[NPCI_INTERRUPTS] = {
	{ 0, 	0, PMAC_DEV_CARD4},	  /* Bit 24 - External Int 4 */
	{ 0, 	0, PMAC_DEV_CARD5},	  /* Bit 25 - External Int 5 */
	{ 0, 	0, PMAC_DEV_CARD6},	  /* Bit 26 - External Int 6 */
	{ 0, 	0, PMAC_DEV_CARD7},	  /* Bit 27 - External Int 7 */
	{ 0, 	0, PMAC_DEV_CARD8},	  /* Bit 28 - External Int 8 */
	{ 0, 	0, PMAC_DEV_CARD9},	  /* Bit 29 - External Int 9 */
	{ 0, 	0, PMAC_DEV_CARD10},	  /* Bit 30 - External Int 10 */
	{ 0, 	0, -1},			  /* Bit 31 - Reserved */
	{ 0,	0, PMAC_DEV_SCC_B},	  /* Bit 16 - SCC Channel B */
	{ 0,	0, PMAC_DEV_AUDIO}, 	  /* Bit 17 - Audio */
	{ pci_via1_interrupt,SPLTTY,-1},  /* Bit 18 - VIA (cuda/pmu) */
	{ 0,	0, PMAC_DEV_FLOPPY},	  /* Bit 19 - SwimIII/Floppy */
	{ 0, 	0, PMAC_DEV_CARD0},	  /* Bit 20 - External Int 0 */
	{ 0, 	0, PMAC_DEV_CARD1},	  /* Bit 21 - External Int 1 */
	{ 0, 	0, PMAC_DEV_CARD2},	  /* Bit 22 - External Int 2 */
	{ 0, 	0, PMAC_DEV_CARD3},	  /* Bit 23 - External Int 3 */
	{ 0,	0, PMAC_DMA_AUDIO_OUT},	  /* Bit 8 - DMA Audio Out */
	{ 0,	0, PMAC_DMA_AUDIO_IN},	  /* Bit 9 - DMA Audio In */
	{ 0,	0, PMAC_DMA_SCSI1},	  /* Bit 10 - DMA SCSI 1 */
	{ 0,	0, PMAC_DEV_NMI},	  /* Bit 11 - Reserved */
	{ 0,	0, PMAC_DEV_SCSI0},	  /* Bit 12 - SCSI 0 */
	{ 0,	0, PMAC_DEV_SCSI1},	  /* Bit 13 - SCSI 1 */
	{ 0,	0, PMAC_DEV_ETHERNET},	  /* Bit 14 - MACE/Ethernet */
	{ 0,	0, PMAC_DEV_SCC_A},	  /* Bit 15 - SCC Channel A */
	{ 0,	0, PMAC_DMA_SCSI0},	  /* Bit 0 - DMA SCSI 0 */
	{ 0,	0, PMAC_DMA_FLOPPY},	  /* Bit 1 - DMA Floppy */
	{ 0,	0, PMAC_DMA_ETHERNET_TX}, /* Bit 2 - DMA Ethernet Tx */
	{ 0,	0, PMAC_DMA_ETHERNET_RX}, /* Bit 3 - DMA Ethernet Rx */
	{ 0,	0, PMAC_DMA_SCC_A_TX},	  /* Bit 4 - DMA SCC Channel A TX */
	{ 0,	0, PMAC_DMA_SCC_A_RX},	  /* Bit 5 - DMA SCC Channel A RX */
	{ 0,	0, PMAC_DMA_SCC_B_TX},	  /* Bit 6 - DMA SCC Channel B TX */
	{ 0,	0, PMAC_DMA_SCC_B_RX}	  /* Bit 7 - DMA SCC Channel B RX */
};


/* Reset the hardware interrupt control */
void
pci_interrupt_initialize(void)
{
	gc_ints = (struct gc_int_controller *) PCI_INTERRUPT_EVENTS;
	pmac_register_int	= pci_register_int;
	pmac_register_ofint	= pci_register_ofint;
	platform_interrupt	= pci_interrupt;
	platform_interrupt	= pci_interrupt;
	platform_spl		= pci_spl;
#if	MACH_KDB
	platform_db_spl		= pci_db_spl;
#endif	/* MACH_KDB */

	gc_ints->mask  = 0;	   /* Disable all interrupts */
	eieio();
	gc_ints->clear = 0xffffffff; /* Clear pending interrupts */
	eieio();
	gc_ints->mask  = 0;	   /* Disable all interrupts */
	eieio();

	/* Add in the NMI interrupt by hand.. */
	pci_add_spl(PCI_INTERRUPT_NMI, SPLHIGH);

	*(v_u_char *)PCI_VIA1_PCR               = 0x00; eieio();
        *(v_u_char *)PCI_VIA1_IER               = 0x7f; eieio();
        *(v_u_char *)PCI_VIA1_IFR               = 0x7f; eieio();
}

static int
pci_find_entry(int device,
	       struct powermac_interrupt **handler,
	       int nentries)
{
	int	i;

	for (i = 0; i < nentries; i++, (*handler)++)
		if ( (*handler)->i_device == device)
			return i;

	*handler = NULL;
	return 0;
}

static void
pci_add_spl(int level, unsigned long mask)
{
	int	i;

	for (i = SPLLO; i > level; i--) 
		pci_spl_mask[i] |= mask;
}
		
/*
 * Register OpenFirmware interrupt 
 */

static void
pci_register_ofint(int ofirq, spl_t level, void (*handler)(int, void *))
{
	int	i;
	unsigned long	beirq = 1 << ofirq;
	unsigned long 	irq = byte_reverse_word(beirq);
	struct powermac_interrupt	*p = powermac_pci_interrupts;

	for (i = 0; i < NPCI_INTERRUPTS; i++, p++) {
		if (irq & (1<<i)) {
			if (p->i_handler)
				panic("pci_register_ofint: interrupt %d is already taken!\n", ofirq);
			else {
				p->i_handler = handler;
				p->i_level = level;
				pci_add_spl(level, irq);
			}
			return;
		}
	}

	panic("Strange.. I couldn't find the bit for ofw %d\n", ofirq);

}

void
pci_register_int(int device, spl_t level, void (*handler)(int, void *))
{
	int	i;
	struct powermac_interrupt	*p;

	/* Check primary interrupts */
	p = powermac_pci_interrupts;
	i = pci_find_entry(device, &p, NPCI_INTERRUPTS);
	if (p) {
		if (p->i_handler) {
			panic("pci_register_int: "
			      "Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;
			pci_add_spl(level, 1<<i);
			return;
		}
		return;
	}
	/* Check cascaded interrupts */
	p = pci_via1_interrupts;
	i = pci_find_entry(device, &p, NVIA1_INTERRUPTS);
	if (p) {
		if (p->i_handler) {
			panic("pci_register_int: "
				"Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;

			*((v_u_char *) PCI_VIA1_IER) |= (1 << i);
			eieio();

			pci_add_spl(level, 1<<10);
			return;
		}
		return;
	}

	panic("pci_register_int: Interrupt %d not found", device);
}


/*
 * findbit, use the PowerPC cntlzw instruction to quickly
 * locate the first bit set in a word.
 */

static __inline__ long
findbit(unsigned long bit)
{
	long	result;

	__asm__ volatile ("cntlzw	%0,%1"  : "=r" (result) : "r" (bit));

	return (31-result);
}

static void
pci_interrupt( int type, struct ppc_saved_state *ssp,
	       unsigned int dsisr, unsigned int dar, spl_t old_lvl)
{
	unsigned long	irq, events;
	long		bit;
	struct powermac_interrupt	*handler;
	spl_t	s;
	int     mycpu;

	mycpu = cpu_number();

	/* Only master CPU should deal with device interrupts */
	assert(mycpu == master_cpu);

	events = gc_ints->events; eieio();

	irq = events | pci_pending_irqs;

	if (events & ~pci_soft_desired_mask) {
		/* received interrupt that should have been masked,
		 * so lazily mask it and pretend it did not happen
		 */
	  	pci_soft_current_mask = pci_soft_desired_mask;
		gc_ints->mask = pci_soft_current_mask;

		/* mark these interrupts as pending */
		pci_pending_irqs |= irq;
	}

	/* Only deal with interrupts we are meant to have */
	irq = irq & pci_soft_desired_mask;

	/* Acknowledge interrupts that we have recieved */
	gc_ints->clear = events;
	eieio();

	/* mark any interrupts that we deal with as no longer pending */
	pci_pending_irqs &= ~irq;

#if DEBUG
	pci_in_interrupt[mycpu]++;
#endif /* DEBUG */

	while ((bit = findbit(irq)) >= 0) {
		handler = &powermac_pci_interrupts[bit];
		irq &= ~(1<<bit);

		if (handler->i_handler) {
			/*s = pci_spl(handler->i_level | SPL_LOWER_MASK);*/
			handler->i_handler(handler->i_device, ssp);
			/*splx(s);*/
		} else {
			printf("{PCI INT %d}", bit);
		}
	}
#if DEBUG
	pci_in_interrupt[mycpu]--;
#endif /* DEBUG */
}

void
pci_via1_interrupt(int device, void *ssp)
{
	register unsigned char intbits;
	long bit;
	struct powermac_interrupt	*handler;
	spl_t		s;

	intbits = via_reg(PCI_VIA1_IFR); eieio();/* get interrupts pending */
	intbits &= via_reg(PCI_VIA1_IER); eieio();	/* only care about enabled */

	if (intbits == 0)
		return;

	via_reg(PCI_VIA1_IFR) = intbits; eieio();

	while ((bit = findbit((unsigned long)intbits)) > 0) {
		intbits &= ~(1<<bit);

		if (bit >= NVIA1_INTERRUPTS)
			continue;

		handler = &pci_via1_interrupts[bit];

		if (handler->i_handler) 
				handler->i_handler(handler->i_device, ssp);
	}
}

