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

#ifndef _POWERMAC_PCI_H_
#define _POWERMAC_PCI_H_

#include <ppc/spl.h>

/* Physical address and size of IO control registers region */

/* Grand Central Items */

#define	GRAND_CENTRAL_BASE	0xf3000000
#define GRAND_CENTRAL_SIZE	0x20000		/* 128k */

/* macros used to define IO space needing mapping, map all of GRAND_CENTRAL */
#define PCI_IO_BASE_ADDR	GRAND_CENTRAL_BASE
#define PCI_IO_SIZE		GRAND_CENTRAL_SIZE

/* Items are in offset from Grand Central */

/* DMA */

#define	PCI_DMA_BASE_PHYS	(PCI_IO_BASE_ADDR + 0x8000)

/* Interrupts */
#define	GC_INTERRUPT_EVENTS	0x00020
#define	GC_INTERRUPT_MASK	0x00024
#define	GC_INTERRUPT_CLEAR	0x00028
#define	GC_INTERRUPT_LEVELS	0x0002C

/* SCC registers (serial line) - physical addr is for probe */
#define	PCI_SCC_BASE_PHYS		(0xf3012000)

/* ASC registers (external scsi) - physical address for probe */
#define PCI_ASC_BASE_PHYS	(PCI_IO_BASE_ADDR + 0x10000)

/* MESH (internal scsi) controller */
#define PCI_MESH_BASE_PHYS	(PCI_IO_BASE_ADDR + 0x18000)

/* audio controller */
#define PCI_AUDIO_BASE_PHYS	(PCI_IO_BASE_ADDR + 0x14000)

/* floppy controller */
#define PCI_FLOPPY_BASE_PHYS	(PCI_IO_BASE_ADDR + 0x15000)

/* Ethernet controller */
#define	PCI_ETHERNET_BASE_PHYS	(PCI_IO_BASE_ADDR + 0x11000)
#define	PCI_ETHERNET_ADDR_PHYS	(PCI_IO_BASE_ADDR + 0x19000)

/* VIA controls, misc stuff (including CUDA) */
#define PCI_VIA_BASE_PHYS	(PCI_IO_BASE_ADDR + 0x16000)

#define	PCI_VIA1_AUXCONTROL	(POWERMAC_IO(PCI_VIA_BASE_PHYS+0x01600))
#define	PCI_VIA1_T1COUNTERLOW	(POWERMAC_IO(PCI_VIA_BASE_PHYS+0x00800))
#define	PCI_VIA1_T1COUNTERHIGH	(POWERMAC_IO(PCI_VIA_BASE_PHYS+0x00A00))
#define	PCI_VIA1_T1LATCHLOW	(POWERMAC_IO(PCI_VIA_BASE_PHYS+0x00C00))
#define	PCI_VIA1_T1LATCHHIGH	(POWERMAC_IO(PCI_VIA_BASE_PHYS+0x00E00))

#define PCI_VIA1_IER		(POWERMAC_IO(PCI_VIA_BASE_PHYS+0x01c00))
#define PCI_VIA1_IFR		(POWERMAC_IO(PCI_VIA_BASE_PHYS+0x01a00))
#define PCI_VIA1_PCR		(POWERMAC_IO(PCI_VIA_BASE_PHYS+0x01800))

/* Am79c940 Media access controller for Ethernet - phys addr for autoconf */
#define PCI_MACE_BASE_PHYS	(PCI_IO_BASE_ADDR + 0x11000)

/* sound */
#define PCI_AWACS_BASE_PHYS	(PCI_IO_BASE_ADDR + 0x14000)

/* Interrupt controller */
#define	PCI_INTERRUPT_EVENTS	((v_u_long *) POWERMAC_IO(PCI_IO_BASE_ADDR + GC_INTERRUPT_EVENTS))

#define	PCI_INTERRUPT_MASK	((v_u_long *) POWERMAC_IO(PCI_IO_BASE_ADDR + GC_INTERRUPT_MASK))

#define	PCI_INTERRUPT_CLEAR	((v_u_long *) POWERMAC_IO(PCI_IO_BASE_ADDR + GC_INTERRUPT_CLEAR))

#define PCI_INTERRUPT_LEVELS	((v_u_long *) POWERMAC_IO(PCI_IO_BASE_ADDR + GC_INTERRUPT_LEVELS))

#define PCI_INTERRUPT_NMI 0x00080000

// defines for OHARE (things that are different than GC)

#define    OHARE_BASE        GRAND_CENTRAL_BASE
#define    OHARE_SIZE        0x80000              // 512 K

#define OHARE_MESH_BASE_PHYS	(OHARE_BASE + 0x10000)
#define OHARE_ATA0_BASE_PHYS    (OHARE_BASE + 0x20000)
#define OHARE_ATA1_BASE_PHYS    (OHARE_BASE + 0x21000)



/* prototypes */

#ifndef ASSEMBLER

void	pci_interrupt_initialize(void);
void	pci_register_int(int device, spl_t level,
			 void (*handler)(int, void *));

void	heathrow_interrupt_initialize(void);
#endif /* ndef ASSEMBLER */

#endif /* _POWERMAC_PCI_H_ */
