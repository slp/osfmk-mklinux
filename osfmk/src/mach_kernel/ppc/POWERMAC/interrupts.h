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

#ifndef _POWERMAC_INTERRUPTS_H_
#define _POWERMAC_INTERRUPTS_H_

#include <mach/ppc/thread_status.h> /* for struct ppc_saved_state */

/*
 * Generic Power Macintosh Interrupts
 * (Things common across all systems)
 *
 */

/* DMA Interrupts */
#define	PMAC_DMA_SCSI0		0
#define	PMAC_DMA_SCSI1		1
#define	PMAC_DMA_AUDIO_OUT	2
#define	PMAC_DMA_AUDIO_IN	3
#define	PMAC_DMA_FLOPPY		4
#define	PMAC_DMA_ETHERNET_TX	5
#define	PMAC_DMA_ETHERNET_RX	6
#define	PMAC_DMA_SCC_A_TX	7
#define	PMAC_DMA_SCC_A_RX	8
#define	PMAC_DMA_SCC_B_TX	9
#define	PMAC_DMA_SCC_B_RX	10

#define	PMAC_DMA_START		0
#define	PMAC_DMA_END		125

/* Device Interrupts */

#define	PMAC_DEV_SCSI0		128
#define	PMAC_DEV_SCSI1		129
#define	PMAC_DEV_ETHERNET	130
#define	PMAC_DEV_SCC_A		131
#define	PMAC_DEV_SCC_B		132
#define	PMAC_DEV_AUDIO		134
#define	PMAC_DEV_CUDA		135
#define	PMAC_DEV_FLOPPY		136
#define	PMAC_DEV_NMI		137
#define PMAC_DEV_PMU            138

#define	PMAC_DEV_ATA0		140
#define	PMAC_DEV_ATA1		141

// OHare interrupts that are overlapped with the GC interrupts

#define OHARE_DEV_ATA0          PMAC_DEV_SCSI1
#define OHARE_DEV_ATA1          PMAC_DEV_ETHERNET

#define	PMAC_DEV_SCC		PMAC_DEV_SCC_A	/* Older SCC chip. */

/* Add-on cards */

#define	PMAC_DEV_CARD0		256
#define	PMAC_DEV_CARD1		257
#define	PMAC_DEV_CARD2		258
#define	PMAC_DEV_CARD3		259
#define	PMAC_DEV_CARD4		260
#define	PMAC_DEV_CARD5		261
#define	PMAC_DEV_CARD6		262
#define	PMAC_DEV_CARD7		263
#define	PMAC_DEV_CARD8		264
#define	PMAC_DEV_CARD9		265
#define	PMAC_DEV_CARD10		266

#define	PMAC_DEV_PDS		270	/* Processor Direct Slot - Nubus only */

/* Some NuBus aliases.. */
#define	PMAC_DEV_NUBUS0		PMAC_DEV_CARD0
#define	PMAC_DEV_NUBUS1		PMAC_DEV_CARD1
#define	PMAC_DEV_NUBUS2		PMAC_DEV_CARD2
#define	PMAC_DEV_NUBUS3		PMAC_DEV_CARD3

#define	PMAC_DEV_HZTICK		300
#define	PMAC_DEV_TIMER1		301
#define	PMAC_DEV_TIMER2		302
#define	PMAC_DEV_VBL		303	/* VBL Interrupt */

#define	PMAC_DEV_START		128
#define	PMAC_DEV_END		511

struct powermac_interrupt {
	void		(*i_handler)(int, void *);
	spl_t		i_level;
	int		i_device;
};

extern void	(*pmac_register_int)(int interrupt, spl_t level,
					void (*handler)(int, void *));

extern void	(*pmac_register_ofint)(int interrupt, spl_t level,
					void (*handler)(int, void *));
extern void	(*platform_interrupt)(int type, struct ppc_saved_state *ssp,
					unsigned int dsisr, unsigned int dar,
					spl_t old_level);

extern spl_t	(*platform_spl)   (spl_t level);
extern spl_t	(*platform_db_spl)(spl_t level);

#endif /* !POWERMAC_INTERRUPTS_H_ */
