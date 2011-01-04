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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

#ifndef _POWERMAC_PDM_H_
#define _POWERMAC_PDM_H_

#include <mach/ppc/vm_types.h>
#include <ppc/spl.h>

/* The Apple documentation uses little endian notation for the bits,
 * despite operating in big-endian mode. These numbers are big-endian
 * bit numbers. Confused?? Hope not. These can be used by the assembly
 * code, hence their position in this file.
 */
#define PDM_ICR_ACK_BIT		0
#define PDM_ICR_INT_MODE_BIT	1
#define PDM_ICR_NMI_BIT		2
#define PDM_ICR_DMA_BIT		3
#define PDM_ICR_ENET_BIT	4
#define PDM_ICR_SCC_BIT		5
#define PDM_ICR_VIA2_BIT	6
#define PDM_ICR_VIA1_BIT	7

#ifndef ASSEMBLER

/* Size of general-purpose DMA buffers (must be power of 2, see pmap.c).
 * This memory must be contiguous physical memory, aligned to a certain
 * physical size (PDM has same size/ alignment requirement).
 * We must be careful not to do bcopy to this area since kernel bcopy
 * assumes that destination cached. Used bcopy_nc instead
 */

#define PDM_DMA_BUFFER_SIZE			0x40000
#define PDM_DMA_BUFFER_ALIGNMENT		PDM_DMA_BUFFER_SIZE

#define PDM_DMA_BUFFER_SCC_A_TX_OFFSET		0x20000
#define PDM_DMA_BUFFER_SCC_A_RX_OFFSET		0x26000
#define PDM_DMA_BUFFER_SCC_B_TX_OFFSET		0x24000
#define PDM_DMA_BUFFER_SCC_B_RX_OFFSET		0x22000

#define PDM_DMA_BUFFER_AUDIO_IN0		0x0C000
#define PDM_DMA_BUFFER_AUDIO_IN1		0x0E000
#define PDM_DMA_BUFFER_AUDIO_OUT0		0x10000
#define PDM_DMA_BUFFER_AUDIO_OUT1		0x12000
			

/* Physical address and size of IO control registers region */

#define PDM_IO_BASE_ADDR	0x50f00000
#define PDM_IO_SIZE		0x42000

/* 0x50f31000 */
struct pdm_dma_base {
     volatile unsigned char   addr_1;   /* 31 -- 24 */
     volatile unsigned char   addr_2;   /* 23 -- 16 */
     volatile unsigned char   addr_3;   /* 15 -- 08 */
     volatile unsigned char   addr_4;   /* 07 -- 00 */
     };

/* sound chip */
#define PDM_AWACS_BASE_PHYS		(PDM_IO_BASE_ADDR+0x14000)

/* SCC registers (serial line) - physical addr is for probe */
#define PDM_SCC_BASE_PHYS		(PDM_IO_BASE_ADDR+0x04000)

#define	PDM_SCC_AMIC_BASE_PHYS		(PDM_IO_BASE_ADDR+0x32000)
#define	PDM_SCC_AMIC_CHANNEL_A_OFFS	0xA0
#define	PDM_SCC_AMIC_CHANNEL_B_OFFS	0x80

#define PDM_DMA_CTRL_BASE_PHYS		(PDM_IO_BASE_ADDR+0x31000)

/* SCSI DMA control registers */
#define PDM_SCSI_DMA_CTRL_BASE_PHYS	(PDM_DMA_CTRL_BASE_PHYS+0x1000)

#define PDM_SCSI_DMA_CTRL_DIR_BIT   1  /* apple doc uses little-endian */
#define PDM_SCSI_DMA_CTRL_FLUSH_BIT 3  /* numbering. We use big-endian. */
#define PDM_SCSI_DMA_CTRL_MODE1_BIT 4  /* Mode1 and Mode0 control dma times */
#define PDM_SCSI_DMA_CTRL_MODE0_BIT 5  /* bus of 50MHz use 00, 25Mhz use 11 */
#define PDM_SCSI_DMA_CTRL_RUN_BIT   6
#define PDM_SCSI_DMA_CTRL_RESET_BIT 7

/* MACE ethernet DMA registers */

#define PDM_MACE_DMA_CTRL_PHYS	(PDM_DMA_CTRL_BASE_PHYS + 0xc20)

/* XXX these two are used by interrupt_init, should be removed */
#define PDM_MACE_DMA_XMT_CTRL	POWERMAC_IO(PDM_MACE_DMA_CTRL_PHYS)
#define PDM_MACE_DMA_RCV_CTRL	POWERMAC_IO(PDM_MACE_DMA_CTRL_PHYS+0x408)

/* Am79c940 Media access controller for Ethernet - phys addr for autoconf */
#define PDM_MACE_BASE_PHYS	(PDM_IO_BASE_ADDR+0xa000)

/* XXX used by interrupt_init, should be removed */
#define PDM_MACE_IMR		POWERMAC_IO(PDM_MACE_BASE_PHYS + 0x90)

/* Ethernet address ROM */
#define PDM_MACE_ENET_ADDR_PHYS	(PDM_IO_BASE_ADDR+0x8001)

/* ASC registers (SCSI controller) - physical addresses for autoconf */
#define PDM_ASC_BASE_PHYS	(PDM_IO_BASE_ADDR+0x10000)
#define	PDM_ASC2_BASE_PHYS	(PDM_IO_BASE_ADDR+0x11000)

/* Interrupt control register */
#define PDM_ICR			POWERMAC_IO(PDM_IO_BASE_ADDR+0x2a000)

/* Cuda registers */
#define PDM_CUDA_BASE_PHYS	(PDM_IO_BASE_ADDR)

/* Interrupt controlling VIAs */

#define PDM_VIA1_IER		(POWERMAC_IO(PDM_IO_BASE_ADDR+0x01c00))
#define PDM_VIA1_IFR		(POWERMAC_IO(PDM_IO_BASE_ADDR+0x01a00))
#define PDM_VIA1_PCR		(POWERMAC_IO(PDM_IO_BASE_ADDR+0x01800))

#define	PDM_VIA1_AUXCONTROL	(POWERMAC_IO(PDM_IO_BASE_ADDR+0x01600))
#define	PDM_VIA1_T1COUNTERLOW	(POWERMAC_IO(PDM_IO_BASE_ADDR+0x00800))
#define	PDM_VIA1_T1COUNTERHIGH	(POWERMAC_IO(PDM_IO_BASE_ADDR+0x00A00))
#define	PDM_VIA1_T1LATCHLOW	(POWERMAC_IO(PDM_IO_BASE_ADDR+0x00C00))
#define	PDM_VIA1_T1LATCHHIGH	(POWERMAC_IO(PDM_IO_BASE_ADDR+0x00E00))

#define PDM_VIA2_IER		(POWERMAC_IO(PDM_IO_BASE_ADDR+0x26013))
#define PDM_VIA2_IFR		(POWERMAC_IO(PDM_IO_BASE_ADDR+0x26003))
#define PDM_VIA2_SLOT_IER	(POWERMAC_IO(PDM_IO_BASE_ADDR+0x26012))
#define PDM_VIA2_SLOT_IFR	(POWERMAC_IO(PDM_IO_BASE_ADDR+0x26002))

#define	PDM_DMA_IFR		(POWERMAC_IO(PDM_IO_BASE_ADDR+0x2a008))
#define PDM_DMA_AUDIO		(POWERMAC_IO(PDM_IO_BASE_ADDR+0x2a00a))

/* Macro for accessing VIA registers */
#define via_reg(X) reg8(X)


/* pdm prototypes */

void pdm_register_int(int device, spl_t level, void (*handler)(int, void *));
void pdm_interrupt_initialize(void);



#endif /* ndef ASSEMBLER */
#endif /* _POWERMAC_PDM_H_ */
