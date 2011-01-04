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


static void	pdm_via1_interrupt (int device, void *ssp);
static void	pdm_via2_interrupt (int device, void *ssp);
static void	pdm_dma_interrupt (int device, void *ssp);
static void	pdm_slot_interrupt (int device, void *ssp);

/* PDM interrupt support is very limited, a certain amount of
 * masking can be done using the ICR but we don't do this yet.
 * SPLs are currently all or nothing.
 */

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

SPL_FUNCTION(pdm_spl)

#if	MACH_KDB
SPL_FUNCTION(pdm_db_spl)
#endif	/* MACH_KDB */


struct powermac_interrupt via1_interrupts[7] = {
	{ 0,	0,	-1},			/* Cascade */
	{ 0,	0,	PMAC_DEV_HZTICK},
	{ 0,	0,	PMAC_DEV_CUDA},		
	{ 0,	0, 	-1},			/* VIA Data */
	{ 0,	0, 	-1},			/* VIA CLK Source */
	{ 0,	0,	PMAC_DEV_TIMER2},
	{ 0,	0,	PMAC_DEV_TIMER1}
};

struct powermac_interrupt via2_interrupts[7] = {
	{ 0, 	0,	-1 },		/* SCSI A DMA IRQ (not used) */
	{ pdm_slot_interrupt, 	0,	-1 },		/* Any interrupt */
	{ 0,	0,	-1 },		/* SCSI B DMA IRQ (not used) */
	{ 0,	0,	PMAC_DEV_SCSI0 },
	{ 0,	0,	-1 },		/* Reserved */
	{ 0,	0,	PMAC_DEV_FLOPPY },
	{ 0, 	0,	PMAC_DEV_SCSI1 }
};


struct powermac_interrupt pdm_dma_interrupts[9] = {
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

struct powermac_interrupt pdm_slot_interrupts[7] = {
	{ 0,	0,	-1 },			/* Unused */
	{ 0,	0,	-1 },			/* Unused */
	{ 0,	0,	PMAC_DEV_NUBUS3},	/* Slot 3 (stranging ordering here..) */
	{ 0,	0,	PMAC_DEV_NUBUS0 },	/* Slot 0 */
	{ 0, 	0,	PMAC_DEV_NUBUS1 },	/* Slot 1 */
	{ 0, 	0,	PMAC_DEV_PDS },		/* Slot 2 (PDS)*/
	{ 0,	0,	PMAC_DEV_VBL }		/* Video Interrupt */
};

struct powermac_interrupt pdm_icr_interrupts[8] = {
	{ pdm_via1_interrupt,	0,	-1 },	/* Cascade Interrupts */
	{ pdm_via2_interrupt,	0,	-1 },
	{ 0,			0,	PMAC_DEV_SCC },
	{ 0,			0,	PMAC_DEV_ETHERNET },
	{ pdm_dma_interrupt,	0,	-1 },	/* PDM DMA Interrupt */
	{ 0,			0,	PMAC_DEV_NMI},
	{ 0,			0,	-1 },	/* INT Mode bit */
	{ 0,			0,	-1 }	/* ACK Bit */
};

static void pdm_interrupt( int type, struct ppc_saved_state *ssp,
		   unsigned int dsisr, unsigned int dar, spl_t old_spl);

#define LEN(s)	(sizeof(s)/sizeof(s[0]))

static struct powermac_interrupt *
pdm_find_entry(int device, struct powermac_interrupt *list, int len)
{
	int	i;

	for (i = 0; i < len; i++, list++)
		if (list->i_device == device)
			return 	list;

	return NULL;
}


static void
pdm_register_ofint(int device, spl_t level,
		void (*handler)(int, void *))
{
	panic("Attempt to register OFW interrupt on PDM machine");
}

void
pdm_register_int(int device, spl_t level,
		void (*handler)(int, void *))
{
	struct powermac_interrupt *entry;
	volatile unsigned char	*enable_int = NULL;
	int		bit;
	spl_t		s = splhigh();

	if (entry = pdm_find_entry(device, via1_interrupts, LEN(via1_interrupts))) {
		enable_int = (unsigned char *) PDM_VIA1_IER;
		bit = entry - via1_interrupts;
		goto found;
	}

	if (entry = pdm_find_entry(device, via2_interrupts, LEN(via2_interrupts))) {
		enable_int = (unsigned char *) PDM_VIA2_IER;
		bit = entry - via2_interrupts;
		goto found;
	}

	if (entry = pdm_find_entry(device, pdm_dma_interrupts, LEN(pdm_dma_interrupts))) 
		goto found;

	if (entry = pdm_find_entry(device, pdm_icr_interrupts, LEN(pdm_icr_interrupts)))
		goto found;

	if (entry = pdm_find_entry(device, pdm_slot_interrupts, LEN(pdm_slot_interrupts))) {
		enable_int = (unsigned char *) PDM_VIA2_IER;
		*enable_int |= (0x82); eieio();	/* Enable slot interrupt int VIA2 */
		enable_int = (unsigned char *) PDM_VIA2_SLOT_IER;
		bit = entry - pdm_slot_interrupts;
		goto found;
	}

	panic("pdm_register_int: Interrupt %d is not supported on this platform.", device);

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
pdm_interrupt_initialize() 
{
	pmac_register_int	= pdm_register_int;
	pmac_register_ofint	= pdm_register_ofint;
	platform_interrupt	= pdm_interrupt;
	platform_spl		= pdm_spl;
#if	MACH_KDB
	platform_db_spl		= pdm_db_spl;
#endif	/* MACH_KDB */

	*(v_u_char *)PDM_ICR		= 0x80; eieio();
	*(v_u_char *)PDM_VIA1_PCR	= 0x00; eieio();

	/* VIAs are old... 
	 * quick lesson:
	 * to set one or more bits,   write (0x80 | BITS),
	 * to reset one or more bits, write (0x00 | BITS)
	 * ie. bit 7 holds the new value for the set bits in the mask
	 */
	*(v_u_char *)PDM_VIA1_IER	= 0x7f;	eieio();
	*(v_u_char *)PDM_VIA1_IFR	= 0x7f;	eieio();
	*(v_u_char *)PDM_VIA2_IER	= 0x7f;	eieio();
	*(v_u_char *)PDM_VIA2_IFR	= 0x7f;	eieio();
	*(v_u_char *)PDM_VIA2_SLOT_IER	= 0x7f; eieio();
	*(v_u_char *)PDM_VIA2_SLOT_IFR	= 0x7f; eieio();
}

static void
pdm_interrupt(int type, struct ppc_saved_state *ssp,
	unsigned int dsisr, unsigned int dar, spl_t old_spl)
{
	int irq, bit;
	unsigned char	value;

	struct powermac_interrupt *handler;

	irq = *(v_u_char*)PDM_ICR; eieio();

	handler = pdm_icr_interrupts;

	for (bit = 0; bit < 7; bit++, handler++) {
		if (irq & (1<<bit)) {
			if (handler->i_handler) 
				handler->i_handler(handler->i_device, ssp);
		}
	}
	
	/* Acknowledge interrupt */
	*(v_u_char *)PDM_ICR		= MASK8(PDM_ICR_ACK); eieio();
}

void
pdm_via1_interrupt(int device, void *ssp)
{
	register unsigned char intbits, bit;
	struct powermac_interrupt	*handler;

	intbits = via_reg(PDM_VIA1_IFR);	/* get interrupts pending */
	eieio();
	intbits &= via_reg(PDM_VIA1_IER); 	/* only care about enabled */
	eieio();

	if (intbits == 0)
		return;

	/*
	 * Unflag interrupts we're about to process.
	 */
	via_reg(PDM_VIA1_IFR) = intbits; eieio();

	handler = via1_interrupts;
	for (bit = 0; bit < 7 ; bit++, handler++) {
		if(intbits & (1<<bit)) {
			if (handler->i_handler)  
				handler->i_handler(handler->i_device, ssp);
			else
				printf("{via1 %d}", bit);
		}
	}

}

static void
pdm_via2_interrupt(int device, void *ssp)
{
	register unsigned char		intbits, bit;
	struct powermac_interrupt	*handler;


	intbits  = via_reg(PDM_VIA2_IFR); eieio();
	intbits &= via_reg(PDM_VIA2_IER); /* only care about enabled ints*/
	eieio();

	if (intbits == 0)
		return;

	handler = via2_interrupts;

	/*
	 * Unflag interrupts we're about to process.
	 */
	via_reg(PDM_VIA2_IFR) = intbits; eieio();

	for (bit = 0; bit < 7; bit++, handler++) {
		if(intbits & (1<<bit)){
			if (handler->i_handler) {
				handler->i_handler(handler->i_device, ssp);
			} else printf("{!via2 %d}", bit);
		}
	}


}

static void
pdm_slot_interrupt(int device, void *ssp)
{
	register unsigned char		intbits, bit;
	struct powermac_interrupt	*handler;


	intbits  = ~via_reg(PDM_VIA2_SLOT_IFR);/* Slot interrupts are reversed of everything else */
	eieio();
	intbits &= via_reg(PDM_VIA2_SLOT_IER); /* only care about enabled ints*/
	eieio();

	handler = pdm_slot_interrupts;

	/*
	 * Unflag interrupts we're about to process.
	 */
	/* via_reg(PDM_VIA2_SLOT_IFR) = intbits; */

	for (bit = 0; bit < 7; bit++, handler++) {
		if(intbits & (1<<bit)){
			if (bit == 6) {
		/* Special Case.. only VBL bit is allowed to be cleared */
				via_reg(PDM_VIA2_SLOT_IFR) = 0x40; eieio();
			}

			if (handler->i_handler) {
				handler->i_handler(handler->i_device, ssp);
			} else printf("{!via-slot %d}", bit);
		}
	}


}


void
pdm_dma_interrupt(int device, void *ssp)
{
	register unsigned char	bit, intbits;
	struct powermac_interrupt	*handler;

	intbits  = via_reg(PDM_DMA_AUDIO); eieio();
	handler = &pdm_dma_interrupts[7];
	for (bit = 0; bit < 2; bit++, handler++) {
		if(intbits & (1<<bit)){
			if (handler->i_handler) {
				handler->i_handler(handler->i_device, ssp);
			} else printf("{!audio dma %d}", bit);
		}
	}

	intbits  = via_reg(PDM_DMA_IFR); eieio();

	handler = pdm_dma_interrupts;
	for (bit = 0; bit < 7; bit++, handler++) {
		if(intbits & (1<<bit)){
			if (handler->i_handler) {
				handler->i_handler(handler->i_device, ssp);
			} else printf("{!dma %d}", bit);
		}
	}
}

