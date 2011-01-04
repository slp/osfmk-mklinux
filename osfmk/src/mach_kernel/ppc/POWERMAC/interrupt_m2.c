
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
 * Interrupt Handler for M2 (PowerBook 5300) 
 * 
 * -- Michael Burg, Apple Computer Inc. (C) 1998
 */


#include <mach_kgdb.h>
#include <debug.h>
#include <mach_kdb.h>
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
#include <ppc/POWERMAC/powermac_m2.h>


static void	m2_via1_interrupt (int device, void *ssp);
static void	m2_via2_interrupt (int device, void *ssp);
static void	m2_dma_interrupt (int device, void *ssp);
static void	m2_slot_interrupt (int device, void *ssp);

/* PDM interrupt support is very limited, a certain amount of
 * masking can be done using the ICR but we don't do this yet.
 * SPLs are currently all or nothing.
 */

/* Two identical functions with different names to allow single stepping
 * of m2_spl whilst in kdb. Define contents in a macro so that it stays
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

SPL_FUNCTION(m2_spl)

#if	MACH_KDB
SPL_FUNCTION(m2_db_spl)
#endif	/* MACH_KDB */


struct powermac_interrupt m2_via1_interrupts[7] = {
	{ 0,	0,	-1},			/* Cascade */
	{ 0,	0,	PMAC_DEV_HZTICK},
	{ 0,	0,	-1},		
	{ 0,	0, 	-1},			/* VIA Data */
	{ 0,	0, 	PMAC_DEV_PMU},			/* VIA CLK Source */
	{ 0,	0,	PMAC_DEV_TIMER2},
	{ 0,	0,	PMAC_DEV_TIMER1}
};

struct powermac_interrupt m2_via2_interrupts[7] = {
	{ 0, 	0,	-1 },		/* SCSI A DMA IRQ (not used) */
	{ m2_slot_interrupt, 	0,	-1 },		/* Any interrupt */
	{ 0,	0,	-1 },		/* SCSI B DMA IRQ (not used) */
	{ 0,	0,	PMAC_DEV_SCSI0 },
	{ 0,	0,	-1 },		/* Reserved */
	{ 0,	0,	PMAC_DEV_FLOPPY },
	{ 0, 	0,	PMAC_DEV_SCSI1 }
};


#if 0
struct powermac_interrupt m2_dma_interrupts[9] = {
	{ 0,	0,	PMAC_DMA_SCC_B_RX },
	{ 0,	0,	PMAC_DMA_SCC_B_TX },
	{ 0,	0,	PMAC_DMA_SCC_A_RX },
	{ 0,	0,	PMAC_DMA_SCC_A_TX },
	{ 0,	0,	PMAC_DMA_ETHERNET_RX },
	{ 0,	0,	PMAC_DMA_ETHERNET_TX },
	{ 0,	0,	PMAC_DMA_FLOPPY },
	{ 0,	0,	PMAC_DMA_AUDIO_IN },
	{ 0,	0,	PMAC_DMA_AUDIO_OUT }
};
#endif

struct powermac_interrupt m2_slot_interrupts[7] = {
	{ 0,	0,	-1 },			/* PCMCIA */
	{ 0,	0,	-1 },			/* CSC */
	{ 0,	0,	PMAC_DEV_NUBUS3},	/* Modem */
	{ 0,	0,	PMAC_DEV_ATA0 },	/* ATA0 */
	{ 0, 	0,	PMAC_DEV_ATA1 },	/* ATA1 */
	{ 0, 	0,	PMAC_DEV_VBL },		/* Keystone */
};

struct powermac_interrupt m2_icr_interrupts[8] = {
	{ m2_via1_interrupt,	0,	-1 },	/* Cascade Interrupts */
	{ m2_via2_interrupt,	0,	-1 },
	{ 0,			0,	PMAC_DEV_SCC },
	{ 0,			0,	PMAC_DEV_ETHERNET },
	{ 0 /*m2_dma_interrupt*/,	0,	-1 },	/* PDM DMA Interrupt */
	{ 0,			0,	PMAC_DEV_NMI},
	{ 0,			0,	-1 },	/* INT Mode bit */
	{ 0,			0,	-1 }	/* ACK Bit */
};

static void m2_interrupt( int type, struct ppc_saved_state *ssp,
		   unsigned int dsisr, unsigned int dar, spl_t old_spl);

#define LEN(s)	(sizeof(s)/sizeof(s[0]))

static struct powermac_interrupt *
m2_find_entry(int device, struct powermac_interrupt *list, int len)
{
	int	i;

	for (i = 0; i < len; i++, list++)
		if (list->i_device == device)
			return 	list;

	return NULL;
}



static void
m2_register_ofint(int device, spl_t level,
		void (*handler)(int, void *))
{
	panic("Attempt to register OFW interrupt on PDM machine");
}

static void
m2_register_int(int device, spl_t level,
		void (*handler)(int, void *))
{
	struct powermac_interrupt *entry;
	volatile unsigned char	*enable_int = NULL;
	int		bit;
	spl_t		s = splhigh();

	if (entry = m2_find_entry(device, m2_via1_interrupts, LEN(m2_via1_interrupts))) {
		enable_int = (unsigned char *) M2_VIA1_IER;
		bit = entry - m2_via1_interrupts;
		goto found;
	}

	if (entry = m2_find_entry(device, m2_via2_interrupts, LEN(m2_via2_interrupts))) {
		enable_int = (unsigned char *) M2_VIA2_IER;
		bit = entry - m2_via2_interrupts;
		goto found;
	}

#if 0
	if (entry = m2_find_entry(device, m2_dma_interrupts, LEN(m2_dma_interrupts))) 
		goto found;

#endif

	if (entry = m2_find_entry(device, m2_icr_interrupts, LEN(m2_icr_interrupts))) {
		goto found;
	}

	if (entry = m2_find_entry(device, m2_slot_interrupts, LEN(m2_slot_interrupts))) {
		enable_int = (unsigned char *) M2_VIA2_IER;
		*enable_int |= (0x82); eieio();	/* Enable slot interrupt int VIA2 */
#if 0
		enable_int = (unsigned char *) M2_VIA2_SLOT_IER;
		bit = entry - m2_slot_interrupts;
#endif
		goto found;
	}

	panic("m2_register_int: Interrupt %d is not supported on this platform.", device);

found:
	entry->i_level = level;
	entry->i_handler = handler;

	if (enable_int)  {
		*enable_int = (*enable_int) | 0x80 | (1<<bit);
		eieio();
	}

	splx(s);
}

void
m2_interrupt_initialize() 
{
	pmac_register_int	= m2_register_int;
	pmac_register_ofint	= m2_register_ofint;
	platform_interrupt	= m2_interrupt;
	platform_spl		= m2_spl;
#if	MACH_KDB
	platform_db_spl		= m2_db_spl;
#endif	/* MACH_KDB */

	*(v_u_long*)M2_ICR		= 0x80; eieio();
	*(v_u_char *)M2_VIA1_PCR	= 0x00; eieio();

	/* VIAs are old... 
	 * quick lesson:
	 * to set one or more bits,   write (0x80 | BITS),
	 * to reset one or more bits, write (0x00 | BITS)
	 * ie. bit 7 holds the new value for the set bits in the mask
	 */
	*(v_u_char *)M2_VIA1_IER	= 0x7f;	eieio();
	*(v_u_char *)M2_VIA1_IFR	= 0x7f;	eieio();
	*(v_u_char *)M2_VIA2_IFR	= 0x7f;	eieio();
	*(v_u_char *)M2_VIA2_SLOT_IFR	= 0x7f; eieio();
}

static void
m2_interrupt(int type, struct ppc_saved_state *ssp,
	unsigned int dsisr, unsigned int dar, spl_t old_spl)
{
	unsigned long irq, bit;
	unsigned long value;

	struct powermac_interrupt *handler;

	irq = *(v_u_long*)M2_ICR; eieio();
	/* Acknowledge interrupt */
	*(v_u_long*)M2_ICR		= 0x80; eieio();

	irq &= 0x7;
	handler = m2_icr_interrupts;

	for (bit = 0; bit < 7; bit++, handler++) {
		if (irq & (1<<bit)) {
			if (handler->i_handler) 
				handler->i_handler(handler->i_device, ssp);
		}
	}
}

void
m2_via1_interrupt(int device, void *ssp)
{
	register unsigned char intbits, bit;
	struct powermac_interrupt	*handler;

	intbits = via_reg(M2_VIA1_IFR);	/* get interrupts pending */
	eieio();
#if 0
	intbits &= via_reg(M2_VIA1_IER); 	/* only care about enabled */
	eieio();

	if (intbits == 0)
		return;
#endif

	/*
	 * Unflag interrupts we're about to process.
	 */
	via_reg(M2_VIA1_IFR) = intbits; eieio();

	handler = m2_via1_interrupts;
	for (bit = 0; bit < 7 ; bit++, handler++) {
		if(intbits & (1<<bit)) {
			if (handler->i_handler)  
				handler->i_handler(handler->i_device, ssp);
		}
	}

}

static void
m2_via2_interrupt(int device, void *ssp)
{
	register unsigned char		intbits, bit;
	struct powermac_interrupt	*handler;


	intbits  = via_reg(M2_VIA2_IFR); eieio();
#if 0
	intbits &= via_reg(M2_VIA2_IER); /* only care about enabled ints*/
	eieio();

	if (intbits == 0)
		return;
#endif

	handler = m2_via2_interrupts;

	/*
	 * Unflag interrupts we're about to process.
	 */
	via_reg(M2_VIA2_IFR) = intbits; eieio();

	for (bit = 0; bit < 7; bit++, handler++) {
		if(intbits & (1<<bit)){
			if (handler->i_handler) 
				handler->i_handler(handler->i_device, ssp);
		}
	}


}

static void
m2_slot_interrupt(int device, void *ssp)
{
	register unsigned char		intbits, bit;
	struct powermac_interrupt	*handler;


	intbits  = ~via_reg(M2_VIA2_SLOT_IFR);/* Slot interrupts are reversed of everything else */
	eieio();
#if 0
	intbits &= via_reg(M2_VIA2_SLOT_IER); /* only care about enabled ints*/
	eieio();
#endif

	handler = m2_slot_interrupts;

	/*
	 * Unflag interrupts we're about to process.
	 */
	/* via_reg(M2_VIA2_SLOT_IFR) = intbits; */

	for (bit = 0; bit < 7; bit++, handler++) {
		if(intbits & (1<<bit)){
			if (bit == 6) {
		/* Special Case.. only VBL bit is allowed to be cleared */
				//via_reg(M2_VIA2_SLOT_IFR) = 0x40; eieio();
			}

			if (handler->i_handler) 
				handler->i_handler(handler->i_device, ssp);
		}
	}


}


#if 0
void
m2_dma_interrupt(int device, void *ssp)
{
	register unsigned char	bit, intbits;
	struct powermac_interrupt	*handler;

	intbits  = via_reg(PDM_DMA_AUDIO); eieio();
	handler = &m2_dma_interrupts[7];
	for (bit = 0; bit < 2; bit++, handler++) {
		if(intbits & (1<<bit)){
			if (handler->i_handler) {
				handler->i_handler(handler->i_device, ssp);
			} else printf("{!audio dma %d}", bit);
		}
	}

	intbits  = via_reg(PDM_DMA_IFR); eieio();

	handler = m2_dma_interrupts;
	for (bit = 0; bit < 7; bit++, handler++) {
		if(intbits & (1<<bit)){
			if (handler->i_handler) {
				handler->i_handler(handler->i_device, ssp);
			} else printf("{!dma %d}", bit);
		}
	}
}
#endif
