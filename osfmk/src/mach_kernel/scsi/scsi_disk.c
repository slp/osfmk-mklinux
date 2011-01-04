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
 * Revision 2.11.2.1  92/03/28  10:16:09  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:33:08  jeffreyh]
 * 
 * Revision 2.12  92/02/23  22:44:59  elf
 * 	Changed the interface of a number of functions not to
 * 	require the scsi_softc pointer any longer.  It was
 * 	mostly unused, now it can be found via tgt->masterno.
 * 	[92/02/22  19:29:26  af]
 * 
 * Revision 2.11  91/08/24  12:29:08  af
 * 	Bcopy casts.
 * 
 * Revision 2.10  91/06/19  11:57:47  rvb
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.9  91/05/14  17:30:28  mrt
 * 	Correcting copyright
 * 
 * Revision 2.8  91/05/13  06:05:40  af
 * 	Enabled some group-1 commands needed on big disks.
 * 	Wrote all those that were missing, kept disabled until
 * 	we find a use for them (via ioctls, I am afraid).
 * 	[91/05/12  16:24:35  af]
 * 
 * Revision 2.7.1.1  91/03/29  17:13:10  af
 * 	Enabled some group-1 commands needed on big disks.
 * 	Wrote all those that were missing, kept disabled until
 * 	we find a use for them (via ioctls, I am afraid).
 * 
 * Revision 2.7  91/02/05  17:45:49  mrt
 * 	Added author notices
 * 	[91/02/04  11:19:35  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:18:21  mrt]
 * 
 * Revision 2.6  90/12/05  23:35:14  af
 * 	Use and prefer BSD/OSF labels.
 * 	[90/12/03  23:48:09  af]
 * 
 * Revision 2.4.1.1  90/11/01  03:39:58  af
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
 *	File: scsi_disk.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	10/90
 *
 *	Middle layer of the SCSI driver: SCSI protocol implementation
 *
 * This file contains code for SCSI commands for DISK devices.
 */

#include <mach/std_types.h>
#include <scsi/compat_30.h>

#include <scsi/scsi.h>
#include <scsi/scsi2.h>
#include <scsi/scsi_defs.h>
#include <scsi2.h>		/* NSCSI2 */
#include <kern/misc_protos.h>
#include <device/ds_routines.h>

char *scdisk_name(
	boolean_t	internal)
{
	return internal ? "rz" : "disk";
}

/*
 * SCSI commands partially specific to disks
 */

void
scdisk_read(
	register target_info_t	*tgt,
	register unsigned int	secno,
	io_req_t		ior)
{
	scsi_cmd_read_t		*cmd;
	register unsigned	len;
	unsigned int		max_dma_data;

	max_dma_data = scsi_softc[(unsigned char)tgt->masterno]->max_dma_data;

	len = ior->io_count;
	if (len > max_dma_data)
		len = max_dma_data;
	if (len < tgt->block_size)
		len = tgt->block_size;

	cmd = (scsi_cmd_read_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_READ;
	cmd->scsi_cmd_lun_and_lba1 = (secno>>16)&SCSI_LBA_MASK;
	cmd->scsi_cmd_lba2 	   = (secno>> 8)&0xff;
	cmd->scsi_cmd_lba3 	   = (secno    )&0xff;
	cmd->scsi_cmd_xfer_len     = btodb(len);
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	tgt->cur_cmd = SCSI_CMD_READ;

	scsi_go(tgt, sizeof(*cmd), len, FALSE);
}

void
scdisk_write(
	register target_info_t	*tgt,
	register unsigned int	secno,
	io_req_t		ior)
{
	scsi_cmd_write_t	*cmd;
	unsigned		len;	/* in bytes */
	unsigned int		max_dma_data;

	max_dma_data = scsi_softc[(unsigned char)tgt->masterno]->max_dma_data;

	len = ior->io_count;
	if (len > max_dma_data)
		len = max_dma_data;
	if (len < tgt->block_size)
		len = tgt->block_size;

	cmd = (scsi_cmd_write_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_WRITE;
	cmd->scsi_cmd_lun_and_lba1 = (secno>>16)&SCSI_LBA_MASK;
	cmd->scsi_cmd_lba2 	   = (secno>> 8)&0xff;
	cmd->scsi_cmd_lba3 	   = (secno    )&0xff;
	cmd->scsi_cmd_xfer_len     = btodb(len);
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	tgt->cur_cmd = SCSI_CMD_WRITE;

	scsi_go(tgt, sizeof(*cmd), 0, FALSE);
}


char
scdisk_mode_select(
	register target_info_t	*tgt,
	register int 		lbn,
	io_req_t		ior,
	char			*mdata,
	int			mlen,
	boolean_t		save)
{
	scsi_cmd_mode_select_t	 *cmd;
	scsi_mode_select_param_t *parm;

	bzero(tgt->cmd_ptr, sizeof(*cmd));
	cmd = (scsi_cmd_mode_select_t*) (tgt->cmd_ptr);

	cmd->scsi_cmd_code = SCSI_CMD_MODE_SELECT;
	cmd->scsi_cmd_xfer_len = mlen;

	if(mdata) {
		cmd->scsi_cmd_lun_and_lba1 = SCSI_CMD_MSL_PF;
		parm = (scsi_mode_select_param_t*) (cmd + 1);
		bcopy(mdata, (char*)parm, mlen);
	}

	if (save)
		cmd->scsi_cmd_lun_and_lba1 |= SCSI_CMD_MSL_SP;

	tgt->cur_cmd = SCSI_CMD_MODE_SELECT;
	scsi_go_and_wait(tgt, sizeof(*cmd) + mlen, 0, ior);

	return tgt->done;
}

/*
 * SCSI commands fully specific to disks
 */
char
scsi_read_capacity(
	register target_info_t	*tgt,
	int			lbn,
	io_req_t		ior)
{
	scsi_cmd_read_capacity_t	*cmd;

	bzero(tgt->cmd_ptr, sizeof(*cmd));
	cmd = (scsi_cmd_read_capacity_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_READ_CAPACITY;
	/* all zeroes, unless... */
	if (lbn) {
		cmd->scsi_cmd_rcap_flags = SCSI_CMD_RCAP_PMI;
		cmd->scsi_cmd_lba1 = (lbn>>24);
		cmd->scsi_cmd_lba2 = (lbn>>16)&0xff;
		cmd->scsi_cmd_lba3 = (lbn>> 8)&0xff;
		cmd->scsi_cmd_lba4 = (lbn    )&0xff;
	}
	
	tgt->cur_cmd = SCSI_CMD_READ_CAPACITY;

	scsi_go_and_wait(tgt, sizeof(*cmd), sizeof(scsi_rcap_data_t),ior);

	return tgt->done;
}

/*ARGSUSED*/
void
scsi_reassign_blocks(
	register target_info_t	*tgt,
	unsigned int		*defect_list,	/* In ascending order ! */
	int			n_defects,
	io_req_t		ior)
{
	scsi_cmd_reassign_blocks_t	*cmd;
	scsi_Ldefect_data_t		*parm;

	cmd = (scsi_cmd_reassign_blocks_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_REASSIGN_BLOCKS;
	cmd->scsi_cmd_lun_and_lba1 = 0;
	cmd->scsi_cmd_lba2 	   = 0;
	cmd->scsi_cmd_lba3 	   = 0;
	cmd->scsi_cmd_xfer_len     = 0;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */

	parm = (scsi_Ldefect_data_t *) (cmd + 1);
	parm->res1 = parm->res2 = 0;
	n_defects *= 4;	/* in 4-byte-ints */
	parm->list_len_msb = n_defects >> 8;
	parm->list_len_lsb = n_defects;
	bcopy((char*)defect_list, (char*)parm->defects, n_defects);

	tgt->cur_cmd = SCSI_CMD_REASSIGN_BLOCKS;

	scsi_go(tgt, sizeof(*cmd) + sizeof(*parm) + (n_defects - 4), 0, FALSE);
}

void
scsi_medium_removal(
	register target_info_t	*tgt,
	boolean_t		allow,
	io_req_t		ior)
{
	scsi_cmd_medium_removal_t	*cmd;

	cmd = (scsi_cmd_medium_removal_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_PREVENT_ALLOW_REMOVAL;
	cmd->scsi_cmd_lun_and_lba1 = 0;
	cmd->scsi_cmd_lba2 	   = 0;
	cmd->scsi_cmd_lba3 	   = 0;
	cmd->scsi_cmd_pa_prevent   = allow ? 0 : 1;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */


	tgt->cur_cmd = SCSI_CMD_PREVENT_ALLOW_REMOVAL;

	scsi_go_and_wait(tgt, sizeof(*cmd), 0, ior);
}

char
scsi_format_unit(
	register target_info_t	*tgt,
	unsigned char		mode,
	unsigned char		vuqe,
	register unsigned int	intlv,
	io_req_t		ior)
{
	scsi_cmd_format_t	*cmd;
	char			*parms;

	cmd = (scsi_cmd_format_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_FORMAT_UNIT;
	cmd->scsi_cmd_lun_and_lba1 =
		mode & (SCSI_CMD_FMT_FMTDATA|SCSI_CMD_FMT_CMPLIST|SCSI_CMD_FMT_LIST_TYPE);
	cmd->scsi_cmd_lba2 	   = vuqe;
	cmd->scsi_cmd_lba3 	   = intlv >>  8;
	cmd->scsi_cmd_xfer_len     = intlv;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */

	parms = (char*) cmd + 1;
	if (ior->io_count)
		bcopy(ior->io_data, parms, ior->io_count);
	else
		bzero(parms, 0xff - sizeof(*cmd));

	tgt->cur_cmd = SCSI_CMD_FORMAT_UNIT;

	scsi_go_and_wait(tgt, sizeof(*cmd) + ior->io_count, 0, ior);
	return tgt->done;
}


/* Group 1 Commands */

void
scsi_long_read(
	register target_info_t	*tgt,
	register unsigned int	secno, 
	io_req_t		ior)
{
	scsi_cmd_long_read_t	*cmd;
	register unsigned	len;
	unsigned int		max_dma_data;

	max_dma_data = scsi_softc[(unsigned char)tgt->masterno]->max_dma_data;

	len = ior->io_count;
	if (len > max_dma_data)
		len = max_dma_data;
	if (len < tgt->block_size)
		len = tgt->block_size;

	cmd = (scsi_cmd_long_read_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_LONG_READ;
	cmd->scsi_cmd_lun_and_relbit = 0;
	cmd->scsi_cmd_lba1	     = secno >> 24;
	cmd->scsi_cmd_lba2	     = secno >> 16;
	cmd->scsi_cmd_lba3	     = secno >>  8;
	cmd->scsi_cmd_lba4	     = secno;
	cmd->scsi_cmd_xxx	     = 0;
	cmd->scsi_cmd_xfer_len_1     = (btodb(len)) >> 8;
	cmd->scsi_cmd_xfer_len_2     = btodb(len);
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	tgt->cur_cmd = SCSI_CMD_LONG_READ;

	scsi_go(tgt, sizeof(*cmd), len, FALSE);
}

void
scsi_long_write(
	register target_info_t	*tgt,
	register unsigned int	secno, 
	io_req_t		ior)
{
	scsi_cmd_long_write_t	*cmd;
	unsigned		len;	/* in bytes */
	unsigned int		max_dma_data;

	max_dma_data = scsi_softc[(unsigned char)tgt->masterno]->max_dma_data;

	len = ior->io_count;
	if (len > max_dma_data)
		len = max_dma_data;
	if (len < tgt->block_size)
		len = tgt->block_size;

	cmd = (scsi_cmd_long_write_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_LONG_WRITE;
	cmd->scsi_cmd_lun_and_relbit = 0;
	cmd->scsi_cmd_lba1	     = secno >> 24;
	cmd->scsi_cmd_lba2	     = secno >> 16;
	cmd->scsi_cmd_lba3	     = secno >>  8;
	cmd->scsi_cmd_lba4	     = secno;
	cmd->scsi_cmd_xxx	     = 0;
	cmd->scsi_cmd_xfer_len_1     = (btodb(len)) >> 8;
	cmd->scsi_cmd_xfer_len_2     = btodb(len);
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	tgt->cur_cmd = SCSI_CMD_LONG_WRITE;

	scsi_go(tgt, sizeof(*cmd), 0, FALSE);
}

char
scdisk_verify(
	register target_info_t	*tgt,
	unsigned int		secno,
	unsigned int		nsectrs,
	io_req_t		ior)
{
	scsi_cmd_verify_long_t	*cmd;
	int			len;

	len = ior->io_count;

	cmd = (scsi_cmd_verify_long_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_VERIFY_1;
	cmd->scsi_cmd_lun_and_relbit = len ? SCSI_CMD_VFY_BYTCHK : 0;
	cmd->scsi_cmd_lba1	     = secno >> 24;
	cmd->scsi_cmd_lba2	     = secno >> 16;
	cmd->scsi_cmd_lba3	     = secno >>  8;
	cmd->scsi_cmd_lba4	     = secno;
	cmd->scsi_cmd_xxx	     = 0;
	cmd->scsi_cmd_xfer_len_1     = (nsectrs) >> 8;
	cmd->scsi_cmd_xfer_len_2     = nsectrs;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	tgt->cur_cmd = SCSI_CMD_VERIFY_1;

	scsi_go_and_wait(tgt, sizeof(*cmd) + len, 0, ior);
	return tgt->done;
}


char
scsi_read_defect(
	register target_info_t	*tgt,
	register unsigned char	mode, 
	io_req_t		ior)
{
	scsi_cmd_long_read_t	*cmd;
	register unsigned	len;

	len = ior->io_count;
	if (len > 0xffff)
		len = 0xffff;

	cmd = (scsi_cmd_read_defect_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_READ_DEFECT_DATA;
	cmd->scsi_cmd_lun_and_relbit = 0;
	cmd->scsi_cmd_lba1	     = mode & 0x1f;
	cmd->scsi_cmd_lba2	     = 0;
	cmd->scsi_cmd_lba3	     = 0;
	cmd->scsi_cmd_lba4	     = 0;
	cmd->scsi_cmd_xxx	     = 0;
	cmd->scsi_cmd_xfer_len_1     = (len) >> 8;
	cmd->scsi_cmd_xfer_len_2     = (len);
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	/* ++ HACK Alert */
/*	tgt->cur_cmd = SCSI_CMD_READ_DEFECT_DATA;*/
	tgt->cur_cmd = SCSI_CMD_LONG_READ;
	/* -- HACK Alert */

	scsi_go(tgt, sizeof(*cmd), len, FALSE);
	iowait(ior);
	return tgt->done;
}


#if	0 /* unused commands */
void
scsi_rezero_unit(
	register target_info_t	*tgt,
	io_req_t		ior)
{
	scsi_cmd_rezero_t	*cmd;

	cmd = (scsi_cmd_rezero_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_REZERO_UNIT;
	cmd->scsi_cmd_lun_and_lba1 = 0;
	cmd->scsi_cmd_lba2 	   = 0;
	cmd->scsi_cmd_lba3 	   = 0;
	cmd->scsi_cmd_xfer_len     = 0;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */


	tgt->cur_cmd = SCSI_CMD_REZERO_UNIT;

	scsi_go_and_wait(tgt, sizeof(*cmd), 0, ior);

}

void
scsi_seek(
	register target_info_t	*tgt,
	register unsigned int	where,
	io_req_t		ior)
{
	scsi_cmd_seek_t	*cmd;

	cmd = (scsi_cmd_seek_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_SEEK;
	cmd->scsi_cmd_lun_and_lba1 = (where >> 16) & 0x1f;
	cmd->scsi_cmd_lba2 	   = where >>  8;
	cmd->scsi_cmd_lba3 	   = where;
	cmd->scsi_cmd_xfer_len     = 0;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */


	tgt->cur_cmd = SCSI_CMD_SEEK;

	scsi_go_and_wait(tgt, sizeof(*cmd), 0, ior);

}

void
scsi_reserve(
	register target_info_t	*tgt,
	register unsigned int	len,
	unsigned char		id,
	unsigned char		mode,
	io_req_t		ior)
{
	scsi_cmd_reserve_t	*cmd;

	cmd = (scsi_cmd_reserve_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_RESERVE;
	cmd->scsi_cmd_lun_and_lba1 = mode & 0x1f;
	cmd->scsi_cmd_reserve_id   = id;
	cmd->scsi_cmd_extent_llen1 = len >>  8;
	cmd->scsi_cmd_extent_llen2 = len;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */


	tgt->cur_cmd = SCSI_CMD_RESERVE;

	scsi_go_and_wait(tgt, sizeof(*cmd), 0, ior);

}

void
scsi_release(
	register target_info_t	*tgt,
	unsigned char		id,
	unsigned char		mode,
	io_req_t		ior)
{
	scsi_cmd_release_t	*cmd;

	cmd = (scsi_cmd_release_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_RELEASE;
	cmd->scsi_cmd_lun_and_lba1 = mode & 0x1f;
	cmd->scsi_cmd_reserve_id   = id;
	cmd->scsi_cmd_lba3 	   = 0;
	cmd->scsi_cmd_xfer_len     = 0;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */


	tgt->cur_cmd = SCSI_CMD_RELEASE;

	scsi_go_and_wait(tgt, sizeof(*cmd), 0, ior);

}


/* Group 1 Commands */

void
scsi_long_seek(
	register target_info_t	*tgt,
	unsigned int		secno,
	io_req_t		ior)
{
	scsi_cmd_long_seek_t	*cmd;

	cmd = (scsi_cmd_long_seek_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_LONG_SEEK;
	cmd->scsi_cmd_lun_and_relbit = 0;
	cmd->scsi_cmd_lba1	     = secno >> 24;
	cmd->scsi_cmd_lba2	     = secno >> 16;
	cmd->scsi_cmd_lba3	     = secno >>  8;
	cmd->scsi_cmd_lba4	     = secno;
	cmd->scsi_cmd_xxx	     = 0;
	cmd->scsi_cmd_xfer_len_1     = 0;
	cmd->scsi_cmd_xfer_len_2     = 0;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	tgt->cur_cmd = SCSI_CMD_LONG_SEEK;

	scsi_go(tgt, sizeof(*cmd), 0, FALSE);
}

void
scsi_write_verify(
	register target_info_t	*tgt,
	unsigned int		secno,
	io_req_t		ior)
{
	scsi_cmd_write_vfy_t	*cmd;
	unsigned		len;	/* in bytes */
	unsigned int		max_dma_data;

	max_dma_data = scsi_softc[(unsigned char)tgt->masterno]->max_dma_data;

	len = ior->io_count;
	if (len > max_dma_data)
		len = max_dma_data;

	cmd = (scsi_cmd_write_vfy_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_WRITE_AND_VERIFY;
	cmd->scsi_cmd_lun_and_relbit = SCSI_CMD_VFY_BYTCHK;
	cmd->scsi_cmd_lba1	     = secno >> 24;
	cmd->scsi_cmd_lba2	     = secno >> 16;
	cmd->scsi_cmd_lba3	     = secno >>  8;
	cmd->scsi_cmd_lba4	     = secno;
	cmd->scsi_cmd_xxx	     = 0;
	cmd->scsi_cmd_xfer_len_1     = (btodb(len)) >> 8;
	cmd->scsi_cmd_xfer_len_2     = btodb(len);
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	tgt->cur_cmd = SCSI_CMD_WRITE_AND_VERIFY;

	scsi_go(tgt, sizeof(*cmd), 0, FALSE);
}

void
scsi_search_data(
	register target_info_t	*tgt,
	unsigned int		secno,
	unsigned char		how,
	unsigned char		flags,
	io_req_t		ior)
{
	scsi_cmd_search_t	*cmd;
	unsigned		len;	/* in bytes */
	unsigned int		max_dma_data;

	max_dma_data = scsi_softc[(unsigned char)tgt->masterno]->max_dma_data;

	if (how != SCSI_CMD_SEARCH_HIGH &&
	    how != SCSI_CMD_SEARCH_EQUAL &&
	    how != SCSI_CMD_SEARCH_LOW)
		panic("scsi_search_data");

	len = ior->io_count;
	if (len > max_dma_data)
		len = max_dma_data;

	cmd = (scsi_cmd_search_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = how;
	cmd->scsi_cmd_lun_and_relbit = flags & 0x1e;
	cmd->scsi_cmd_lba1	     = secno >> 24;
	cmd->scsi_cmd_lba2	     = secno >> 16;
	cmd->scsi_cmd_lba3	     = secno >>  8;
	cmd->scsi_cmd_lba4	     = secno;
	cmd->scsi_cmd_xxx	     = 0;
	cmd->scsi_cmd_xfer_len_1     = (btodb(len)) >> 8;
	cmd->scsi_cmd_xfer_len_2     = btodb(len);
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	tgt->cur_cmd = how;

	scsi_go(tgt, sizeof(*cmd), 0, FALSE);
}


void
scsi_set_limits(
	register target_info_t	*tgt,
	unsigned int		secno,
	unsigned short		nblocks,
	unsigned char		inhibit,
	io_req_t		ior)
{
	scsi_cmd_set_limits_t	*cmd;

	cmd = (scsi_cmd_set_limits_t*) (tgt->cmd_ptr);
	cmd->scsi_cmd_code = SCSI_CMD_SET_LIMITS;
	cmd->scsi_cmd_lun_and_relbit = inhibit & 0x3;
	cmd->scsi_cmd_lba1	     = secno >> 24;
	cmd->scsi_cmd_lba2	     = secno >> 16;
	cmd->scsi_cmd_lba3	     = secno >>  8;
	cmd->scsi_cmd_lba4	     = secno;
	cmd->scsi_cmd_xxx	     = 0;
	cmd->scsi_cmd_xfer_len_1     = nblocks >> 8;
	cmd->scsi_cmd_xfer_len_2     = nblocks;
	cmd->scsi_cmd_ctrl_byte = 0;	/* not linked */
	
	tgt->cur_cmd = SCSI_CMD_SET_LIMITS;

	scsi_go(tgt, sizeof(*cmd), 0, FALSE);
}


#endif

#if	0 && NSCSI2 > 0
scsi_lock_cache
scsi_prefetch
scsi_read_defect_data
scsi_sync_cache
scsi_write_same
#endif	/* SCSI2 */
