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
 *
 * Support for HEATHROW
 *
 * Exactly like interrupt_pci (Grand Central/OHare), except 
 * that it supports 64-bits worth of intererupts. ACK!
 *
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


static void	heathrow_interrupt(int type,
			struct ppc_saved_state *ssp,
			unsigned int dsisr, unsigned int dar, spl_t old_lvl);
static void	heathrow_register_ofint(int ofirq, spl_t level,
				void (*handler)(int, void *));

static void heathrow_add_spl(int level, unsigned long mask, unsigned long mask2);

/* Layout of the Grand Central (and derived) interrupt controller */

struct heathrow_int_controller {
	volatile unsigned long	events2;
	volatile unsigned long	mask2;
	volatile unsigned long	clear2;
	volatile unsigned long	levels2;
	volatile unsigned long	events;
	volatile unsigned long	mask;
	volatile unsigned long	clear;
	volatile unsigned long	levels;
} *heathrow_ints;

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

unsigned long	heathrow_spl_mask[2][SPLLO+1];
unsigned long	heathrow_pending_irqs[2] = {0,0};

unsigned long	heathrow_soft_desired_mask[2],heathrow_soft_current_mask[2];

#if DEBUG
vm_offset_t spl_addr[NCPUS];
extern boolean_t check_spl_inversion;
int heathrow_in_interrupt[NCPUS];
#endif

static spl_t heathrow_spl(spl_t lvl)
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
	if ((heathrow_in_interrupt[mycpu] != 0) && old_level > SPLHIGH) {
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
	    heathrow_in_interrupt[mycpu] == 0 &&
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
	if (heathrow_in_interrupt[mycpu] && lvl > SPLHIGH) {
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

			heathrow_soft_desired_mask[0] = heathrow_spl_mask[0][lvl];
			heathrow_soft_desired_mask[1] = heathrow_spl_mask[1][lvl];

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

				if ((heathrow_soft_current_mask[0] &
				     heathrow_soft_desired_mask[0]) !=
				    heathrow_soft_desired_mask[0] ||
				  (heathrow_soft_current_mask[1] &
				     heathrow_soft_desired_mask[1]) !=
				    heathrow_soft_desired_mask[1] 
					) {
					
					/* an interrupt is disabled that
					 * should now be enabled - fix it
					 */

					heathrow_soft_current_mask[0] =
						heathrow_soft_desired_mask[0];

					heathrow_soft_current_mask[1] =
						heathrow_soft_desired_mask[1];

					heathrow_ints->mask =
						heathrow_soft_current_mask[0]; eieio();
					heathrow_ints->mask2 =
						heathrow_soft_current_mask[1]; eieio();
				}


				heathrow_pending_irqs[0] |= heathrow_ints->levels;
				eieio();

				heathrow_pending_irqs[1] |= heathrow_ints->levels2;
				eieio();

				if (heathrow_pending_irqs[0] &
				    heathrow_soft_desired_mask[0]
				|| heathrow_pending_irqs[1] & heathrow_soft_desired_mask[1]) {
			    
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
SPL_FUNCTION(heathrow_db_spl)
#endif	/* MACH_KDB */

static void	heathrow_via1_interrupt (int device, void *ssp);

#define NVIA1_INTERRUPTS 7
struct powermac_interrupt heathrow_via1_interrupts[NVIA1_INTERRUPTS] = {
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

struct powermac_interrupt  powermac_heathrow_interrupts[NPCI_INTERRUPTS] = {
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
	{ heathrow_via1_interrupt,SPLTTY,-1},  /* Bit 18 - VIA (cuda/pmu) */
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


struct	powermac_interrupt	powermac_heathrow_extended_interrupts[NPCI_INTERRUPTS];
void heathrow_register_int(int device, spl_t level, void (*handler)(int, void *));


/* Reset the hardware interrupt control */
void
heathrow_interrupt_initialize(void)
{
	heathrow_ints = (struct heathrow_int_controller *) POWERMAC_IO((PCI_IO_BASE_ADDR + 0x10));
	pmac_register_int	= heathrow_register_int;
	pmac_register_ofint	= heathrow_register_ofint;
	platform_interrupt	= heathrow_interrupt;
	platform_interrupt	= heathrow_interrupt;
	platform_spl		= heathrow_spl;
#if	MACH_KDB
	platform_db_spl		= heathrow_db_spl;
#endif	/* MACH_KDB */

	heathrow_ints->mask  = 0;	   /* Disable all interrupts */
	eieio();
	heathrow_ints->clear = 0xffffffff; /* Clear pending interrupts */
	eieio();
	heathrow_ints->mask  = 0;	   /* Disable all interrupts */
	eieio();

	heathrow_ints->mask2  = 0;	   /* Disable all interrupts */
	eieio();
	heathrow_ints->clear2 = 0xffffffff; /* Clear pending interrupts */
	eieio();
	heathrow_ints->mask2 = 0;	   /* Disable all interrupts */
	eieio();

	/* Add in the NMI interrupt by hand.. */
	heathrow_add_spl(PCI_INTERRUPT_NMI, SPLHIGH, 0);

	*(v_u_char *)PCI_VIA1_PCR               = 0x00; eieio();
        *(v_u_char *)PCI_VIA1_IER               = 0x7f; eieio();
        *(v_u_char *)PCI_VIA1_IFR               = 0x7f; eieio();
}

static __inline__ long
findbit(unsigned long bit)
{
	long	result;

	__asm__ volatile ("cntlzw	%0,%1"  : "=r" (result) : "r" (bit));

	return (31-result);
}

static int
heathrow_find_entry(int device,
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
heathrow_add_spl(int level, unsigned long mask, unsigned long mask2)
{
	int	i;

	for (i = SPLLO; i > level; i--) {
		heathrow_spl_mask[0][i] |= mask;
		heathrow_spl_mask[1][i] |= mask2;
	}
}
		
/*
 * Register OpenFirmware interrupt 
 */

static void
heathrow_register_ofint(int ofirq, spl_t level, void (*handler)(int, void *))
{
	int	i;
	unsigned long	beirq;
	unsigned long 	irq;
	struct powermac_interrupt	*p = powermac_heathrow_interrupts;
	boolean_t	is_extended = FALSE;

	if (ofirq >= 32) {
		is_extended = TRUE;
		p = powermac_heathrow_extended_interrupts;
		beirq = 1 << (ofirq-32);
	} else
		beirq = 1 << ofirq;

	irq = byte_reverse_word(beirq);

	p = &p[findbit(irq)];
	if (p->i_handler)
		panic("heathrow_register_ofint: interrupt %d is already taken!\n", ofirq);
	else {
		p->i_handler = handler;
		p->i_level = level;
		if (is_extended)
			heathrow_add_spl(level, 0, irq);
		else
			heathrow_add_spl(level, irq, 0);
	}
}

void
heathrow_register_int(int device, spl_t level, void (*handler)(int, void *))
{
	int	i;
	struct powermac_interrupt	*p;

	/* Check primary interrupts */
	p = powermac_heathrow_interrupts;
	i = heathrow_find_entry(device, &p, NPCI_INTERRUPTS);
	if (p) {
		if (p->i_handler) {
			panic("heathrow_register_int: "
			      "Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;
			heathrow_add_spl(level, 1<<i, 0);
			return;
		}
		return;
	}
	/* Check cascaded interrupts */
	p = heathrow_via1_interrupts;
	i = heathrow_find_entry(device, &p, NVIA1_INTERRUPTS);
	if (p) {
		if (p->i_handler) {
			panic("heathrow_register_int: "
				"Interrupt %d already taken!? ", device);
		} else {
			p->i_handler = handler;
			p->i_level = level;

			*((v_u_char *) PCI_VIA1_IER) |= (1 << i);
			eieio();

			heathrow_add_spl(level, 1<<10, 0);
			return;
		}
		return;
	}

	panic("heathrow_register_int: Interrupt %d not found", device);
}


/*
 * findbit, use the PowerPC cntlzw instruction to quickly
 * locate the first bit set in a word.
 */

static void
heathrow_interrupt( int type, struct ppc_saved_state *ssp,
	       unsigned int dsisr, unsigned int dar, spl_t old_lvl)
{
	unsigned long	irq, irq2,  events, events2;
	long		bit;
	struct powermac_interrupt	*handler;
	spl_t	s;
	int     mycpu;

	mycpu = cpu_number();

	/* Only master CPU should deal with device interrupts */
	assert(mycpu == master_cpu);

	events = heathrow_ints->events; eieio();
	events2 = heathrow_ints->events2; eieio();

	irq = events | heathrow_pending_irqs[0];
	irq2 = events2 | heathrow_pending_irqs[1];

	if (events & ~heathrow_soft_desired_mask[0]
	|| events2 & ~heathrow_soft_desired_mask[1]) {
		/* received interrupt that should have been masked,
		 * so lazily mask it and pretend it did not happen
		 */
	  	heathrow_soft_current_mask[0] = heathrow_soft_desired_mask[0];
		heathrow_ints->mask = heathrow_soft_current_mask[0]; eieio();

	  	heathrow_soft_current_mask[1] = heathrow_soft_desired_mask[1];
		heathrow_ints->mask2 = heathrow_soft_current_mask[1]; eieio();

		/* mark these interrupts as pending */
		heathrow_pending_irqs[0] |= irq;
		heathrow_pending_irqs[1] |= irq2;
	}

	/* Only deal with interrupts we are meant to have */
	irq = irq & heathrow_soft_desired_mask[0];
	irq2 = irq2 & heathrow_soft_desired_mask[1];

	/* Acknowledge interrupts that we have recieved */
	heathrow_ints->clear = events; eieio();
	heathrow_ints->clear2 = events2; eieio();

	/* mark any interrupts that we deal with as no longer pending */
	heathrow_pending_irqs[0] &= ~irq;
	heathrow_pending_irqs[1] &= ~irq2;

#if DEBUG
	heathrow_in_interrupt[mycpu]++;
#endif /* DEBUG */

	while ((bit = findbit(irq)) >= 0) {
		handler = &powermac_heathrow_interrupts[bit];
		irq &= ~(1<<bit);

		if (handler->i_handler) {
			/*s = heathrow_spl(handler->i_level | SPL_LOWER_MASK);*/
			handler->i_handler(handler->i_device, ssp);
			/*splx(s);*/
		} else {
			printf("{PCI INT %d}", bit);
		}
	}

	while ((bit = findbit(irq2)) >= 0) {
		handler = &powermac_heathrow_extended_interrupts[bit];
		irq2 &= ~(1<<bit);

		if (handler->i_handler) {
			/*s = heathrow_spl(handler->i_level | SPL_LOWER_MASK);*/
			handler->i_handler(handler->i_device, ssp);
			/*splx(s);*/
		} else {
			printf("{HEATHROW INT %d}", bit);
		}
	}
#if DEBUG
	heathrow_in_interrupt[mycpu]--;
#endif /* DEBUG */
}

void
heathrow_via1_interrupt(int device, void *ssp)
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

		handler = &heathrow_via1_interrupts[bit];

		if (handler->i_handler) 
				handler->i_handler(handler->i_device, ssp);
	}
}

