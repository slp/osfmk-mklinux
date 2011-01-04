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

#include <asc.h>

#if     NASC > 0
#include <platforms.h>
#include <ppc/proc_reg.h> /* For isync */
#include <mach_debug.h>
#include <kern/spl.h>
#include <mach/std_types.h>
#include <types.h>
#include <chips/busses.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/scsi_53C94.h>

void	scsi_amic_init(int unit);
void	scsi_amic_setup(int unit, vm_offset_t address, 
				vm_size_t len, boolean_t isread);
void	scsi_amic_start(int unit, boolean_t isread);
void	scsi_amic_end(int unit, boolean_t isread);

scsi_curio_dma_ops_t	scsi_amic_ops = {
	scsi_amic_init,
	scsi_amic_setup,
	scsi_amic_start,
	scsi_amic_end
};

struct dma_softc {
	volatile unsigned char	*base;
	volatile unsigned char	*ctrl;
} scsi_amic_softc[NASC];

typedef struct dma_softc dma_softc_t;

void
scsi_amic_init(int unit)
{
	dma_softc_t	*dmap = &scsi_amic_softc[unit];


	if (unit == 1) {
		dmap->base = (v_u_char *) POWERMAC_IO(PDM_SCSI_DMA_CTRL_BASE_PHYS);
		dmap->ctrl = (v_u_char *) POWERMAC_IO(PDM_SCSI_DMA_CTRL_BASE_PHYS+0x8);
	} else {
		dmap->base = (v_u_char *) POWERMAC_IO(PDM_SCSI_DMA_CTRL_BASE_PHYS+0x4);
		dmap->ctrl = (v_u_char *) POWERMAC_IO(PDM_SCSI_DMA_CTRL_BASE_PHYS+0x9);
	}

}


void
scsi_amic_end(int unit, boolean_t isread)
{
	dma_softc_t	*dmap = &scsi_amic_softc[unit];


	/*
	 * Flush out the DMA, and kill it.
	 */

	eieio();
	if ((*dmap->ctrl & MASK8(PDM_SCSI_DMA_CTRL_DIR)) == 0) {
		*dmap->ctrl |= MASK8(PDM_SCSI_DMA_CTRL_FLUSH);
		do {
			eieio();
		} while (*dmap->ctrl & MASK8(PDM_SCSI_DMA_CTRL_FLUSH));
	}

	*dmap->ctrl &= ~MASK8(PDM_SCSI_DMA_CTRL_RUN); eieio();

	while (*dmap->ctrl & MASK8(PDM_SCSI_DMA_CTRL_RUN))
		eieio();

}

void
scsi_amic_setup(int unit, vm_offset_t addr, vm_size_t data_in_count,
		boolean_t isread)
{
	dma_softc_t	*dmap = &scsi_amic_softc[unit];
	

	*dmap->ctrl |= MASK8(PDM_SCSI_DMA_CTRL_RESET); eieio();

	while (*dmap->ctrl & MASK8(PDM_SCSI_DMA_CTRL_RUN))
		eieio();

	if (addr & 7) 
		panic("SCSI DMA - non-aligned address");

	dmap->base[0] = (unsigned char)((addr >> 24) & 0xff);
	dmap->base[1] = (unsigned char)((addr >> 16) & 0xff);
	dmap->base[2] = (unsigned char)((addr >>  8) & 0xff);
	dmap->base[3] = (unsigned char)((addr) & 0xff);

	eieio();

	return;
}

void
scsi_amic_start(int unit, boolean_t isread)
{
	dma_softc_t	*dmap = &scsi_amic_softc[unit];
	

	if (isread)
		*dmap->ctrl = MASK8(PDM_SCSI_DMA_CTRL_RUN);
	else
		*dmap->ctrl = MASK8(PDM_SCSI_DMA_CTRL_DIR)|MASK8(PDM_SCSI_DMA_CTRL_RUN);
	eieio();

	return;
}

#endif 	/* NASC > 0*/
