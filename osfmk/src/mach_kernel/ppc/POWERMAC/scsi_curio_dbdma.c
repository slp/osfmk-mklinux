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
#include <ppc/POWERMAC/dbdma.h>
#include <ppc/POWERMAC/dma_funnel.h>
#include <ppc/POWERMAC/device_tree.h>

void	scsi_curio_dbdma_init(int unit);
void	scsi_curio_dbdma_setup(int unit, vm_offset_t address, 
				vm_size_t len, boolean_t isread);
void	scsi_curio_dbdma_start(int unit, boolean_t isread);
void	scsi_curio_dbdma_end(int unit, boolean_t isread);

scsi_curio_dma_ops_t scsi_curio_dbdma_ops = {
	scsi_curio_dbdma_init,
	scsi_curio_dbdma_setup,
	scsi_curio_dbdma_start,
	scsi_curio_dbdma_end
};

dbdma_command_t	*scsi_curio_dbdma_commands;

dbdma_regmap_t	*curio_chan;

void
scsi_curio_dbdma_init(int unit)
{
	device_node_t	*curio;

	scsi_curio_dbdma_commands = dbdma_alloc(2);

	curio = find_devices("53c94");
	curio_chan = (dbdma_regmap_t *) POWERMAC_IO(curio->addrs[1].address);
}

void
scsi_curio_dbdma_setup(int unit, vm_offset_t address, vm_size_t len,
			boolean_t isread)
{
	dbdma_command_t	*dmap = scsi_curio_dbdma_commands;


	DBDMA_BUILD(dmap, isread ? DBDMA_CMD_IN_MORE : DBDMA_CMD_OUT_MORE,
			0, len, address, DBDMA_INT_NEVER, DBDMA_BRANCH_NEVER,
			DBDMA_WAIT_NEVER);
	dmap++;

	DBDMA_BUILD(dmap, DBDMA_CMD_STOP, 0, 0, 0, DBDMA_INT_NEVER,
		DBDMA_BRANCH_NEVER, DBDMA_WAIT_NEVER);

	eieio();	/* Make sure things are flushed out.. */

}

void
scsi_curio_dbdma_start(int unit, boolean_t isread)
{
	dbdma_start(curio_chan, scsi_curio_dbdma_commands);
}

void
scsi_curio_dbdma_end(int unit, boolean_t isread)
{
	dbdma_stop(curio_chan);
}

#endif /* NASC > 0 */
