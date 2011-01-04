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
/* CMU_HIST */
/*
 * Revision 2.7.2.1  92/03/28  10:15:58  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:32:54  jeffreyh]
 * 
 * Revision 2.8  92/02/23  22:44:52  elf
 * 	Changed the interface of a number of functions not to
 * 	require the scsi_softc pointer any longer.  It was
 * 	mostly unused, now it can be found via tgt->masterno.
 * 	[92/02/22  19:29:47  af]
 * 
 * Revision 2.7  91/08/24  12:28:34  af
 * 	Filled in with what was missing.  Works, too.
 * 	[91/08/02  03:46:32  af]
 * 
 * Revision 2.6  91/06/19  11:57:39  rvb
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.5  91/05/14  17:30:08  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:45:39  mrt
 * 	Added author notices
 * 	[91/02/04  11:19:23  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:18:04  mrt]
 * 
 * Revision 2.3  90/12/05  23:35:09  af
 * 
 * 
 * Revision 2.1.1.1  90/11/01  03:39:46  af
 * 	Created, from the SCSI specs:
 * 	"Small Computer Systems Interface (SCSI)", ANSI Draft
 * 	X3T9.2/82-2 - Rev 17B December 1985
 * 	"Small Computer System Interface - 2 (SCSI-II)", ANSI Draft
 * 	X3T9.2/86-109 -  Rev 10C March 1990
 * 	[90/10/11            af]
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 *	File: scsi_cpu.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	10/90
 *
 *	Middle layer of the SCSI driver: SCSI protocol implementation
 *
 * This file contains code for SCSI commands for PROCESSOR devices.
 */

#include <mach/std_types.h>
#include <scsi/compat_30.h>

#include <scsi/scsi.h>
#include <scsi/scsi2.h>
#include <scsi/scsi_defs.h>
#include <kern/misc_protos.h>

char *sccpu_name(
	boolean_t	internal)
{
	return internal ? "sc" : "cpu";
}

void
scsi_probe_luns(target_info_t		*tgt)
{
	int i;
	target_info_t *mtgt =scsi_softc[tgt->masterno]->target[tgt->target_id];
	if ((mtgt->flags & TGT_PROBED_LUNS) != TGT_PROBED_LUNS) {
		target_info_t stgt;
		bcopy((const char *)tgt, (char *)&stgt, sizeof(stgt));
		for(i=0;i<SCSI_NLUNS;i++) {
			tgt->lun = i;
			scsi_inquiry(tgt, SCSI_INQ_STD_DATA);
			scsi_verify_state(tgt);
			scsi_initialize(tgt);
		}
		bcopy((const char *)&stgt, (char *)tgt, sizeof(stgt));
		tgt->flags |= TGT_PROBED_LUNS;
		mtgt->flags |= TGT_PROBED_LUNS;
	}
}

void
scsi_send(
	register target_info_t	*tgt,
	io_req_t		ior)
{
	scsi_cmd_write_t	*cmd;
	unsigned		len;	/* in bytes */
	unsigned int		max_dma_data;
	boolean_t		passive = (ior->io_op & IO_PASSIVE); 

	scsi_probe_luns(tgt);
	tgt->ior = ior;

	max_dma_data = scsi_softc[(unsigned char)tgt->masterno]->max_dma_data;

	len = ior->io_count;
	if (len > max_dma_data)
		len = max_dma_data;
	if (len < tgt->block_size)
		len = tgt->block_size;

	if (!passive) {
		cmd = (scsi_cmd_write_t*) (tgt->cmd_ptr);
		cmd->scsi_cmd_code = SCSI_CMD_SEND;
		cmd->scsi_cmd_lun_and_lba1 = tgt->lun << 5;
		cmd->scsi_cmd_lba2 	   = len >> 16;
		cmd->scsi_cmd_lba3 	   = len >> 8;
		cmd->scsi_cmd_xfer_len     = len;
		cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	}
	
	tgt->cur_cmd = SCSI_CMD_SEND;

	scsi_go(tgt, sizeof(*cmd), (passive?len:0), FALSE);
}

void
scsi_receive(
	register target_info_t	*tgt,
	io_req_t		ior)
{
	scsi_cmd_read_t		*cmd;
	register unsigned	len;
	unsigned int		max_dma_data;
	boolean_t		passive = (ior->io_op & IO_PASSIVE); 

	scsi_probe_luns(tgt);
	tgt->ior = ior;

	max_dma_data = scsi_softc[(unsigned char)tgt->masterno]->max_dma_data;

	len = ior->io_count;
	if (len > max_dma_data)
		len = max_dma_data;
	if (len < tgt->block_size)
		len = tgt->block_size;

	if (!passive) {
		cmd = (scsi_cmd_read_t*) (tgt->cmd_ptr);
		cmd->scsi_cmd_code = SCSI_CMD_RECEIVE;
		cmd->scsi_cmd_lun_and_lba1 = tgt->lun << 5;
		cmd->scsi_cmd_lba2 	   = len >> 16;
		cmd->scsi_cmd_lba3 	   = len >>  8;
		cmd->scsi_cmd_xfer_len     = len;
		cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	}
	
	tgt->cur_cmd = SCSI_CMD_RECEIVE;

	scsi_go(tgt, sizeof(*cmd), (passive?0:len), FALSE);
}

