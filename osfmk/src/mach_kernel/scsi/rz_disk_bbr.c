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
 * Revision 2.6.2.1  92/03/28  10:15:32  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:31:59  jeffreyh]
 * 
 * Revision 2.7  92/02/23  22:44:25  elf
 * 	Changed the interface of a number of functions not to
 * 	require the scsi_softc pointer any longer.  It was
 * 	mostly unused, now it can be found via tgt->masterno.
 * 	[92/02/22  19:30:35  af]
 * 
 * Revision 2.6  91/08/24  12:27:58  af
 * 	Fixed includes.
 * 	[91/08/02  03:45:46  af]
 * 
 * Revision 2.5  91/07/09  23:22:33  danner
 * 	Upgraded to new label technology.
 * 	[91/07/09  11:16:12  danner]
 * 
 * Revision 2.4  91/06/19  11:57:06  rvb
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.3  91/05/14  17:26:42  mrt
 * 	Correcting copyright
 * 
 * Revision 2.2  91/05/13  06:04:29  af
 * 	Created.
 * 	[91/04/30            af]
 * 
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
 *	File: rz_disk_bbr.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	4/91
 *
 *	Top layer of the SCSI driver: interface with the MI.
 *	This file contains bad-block management functions
 *	(retry, replace) for disk-like devices.
 */

#include <mach/std_types.h>
#include <kern/misc_protos.h>
#include <scsi/compat_30.h>

#include <scsi/scsi.h>
#include <scsi/scsi_defs.h>
#include <scsi/rz.h>

#include <device/ds_routines.h>

/*
 * The defines below provide capability for soft and hard errors.
 * Sometimes disks can get themselves into a state where a block is
 * reported as bad when it is actually alright.  If the command can
 * complete sucessfully within SOFT_RETRIES times, the block is not
 * reassigned, otherwise it is a hard error which may be reassigned.
 */
#define	SOFT_RETRIES		4
#define HARD_RETRIES		6
#define	MAX_RETRIES		(SOFT_RETRIES + HARD_RETRIES)

int	scsi_bbr_retries = MAX_RETRIES;

#define	BBR_ACTION_COMPLETE	1
#define	BBR_ACTION_RETRY_READ	2
#define	BBR_ACTION_RETRY_WRITE	3
#define BBR_ACTION_REASSIGN	4
#define	BBR_ACTION_COPY		5
#define	BBR_ACTION_VERIFY	6

/*	Convert int to "little-endian" byte string	*/
static void make_msf(
		unsigned char	*buf,
		unsigned int	val);

/*
 * Bad block replacement routine, invoked on
 * unrecovereable disk read/write errors.
 */
boolean_t
scdisk_bad_block_repl(
	target_info_t		*tgt,
	unsigned int		blockno)
{
	register io_req_t	ior = tgt->ior;

	if (scsi_no_automatic_bbr || (ior->io_op & IO_INTERNAL))
		return FALSE;

	/* signal we took over */
	tgt->flags |= TGT_BBR_ACTIVE;

	printf("%s", "Attempting bad block replacement..");

	tgt->dev_info.disk.b.badblockno = blockno;
	tgt->dev_info.disk.b.retry_count = 0;

	tgt->dev_info.disk.b.save_rec   = ior->io_recnum;
	tgt->dev_info.disk.b.save_addr  = ior->io_data;
	tgt->dev_info.disk.b.save_count = ior->io_count;
	tgt->dev_info.disk.b.save_resid = ior->io_residual;

	/*
	 * On a write all we need is to rewire the offending block.
	 * Note that the sense data identified precisely which 512 sector
	 * is bad.  At the end we'll retry the entire write, so if there
	 * is more than one bad sector involved they will be handled one
	 * at a time.
	 */
	if ((ior->io_op & IO_READ) == 0) {
		unsigned char msf[sizeof(int)];
		ior->io_temporary = BBR_ACTION_COMPLETE;
		printf("%s", "just reassign..");
		make_msf(msf,blockno);
		scsi_reassign_blocks( tgt, (unsigned int *)msf, 1, ior);
	} else
	/*
	 * This is more complicated.  We asked for N bytes, and somewhere
	 * in there there is a chunk of bad data.  First off, we should retry
	 * at least a couple of times to retrieve that data [yes the drive
	 * should have done its best already so what].  If that fails we
	 * should recover as much good data as possible (before the bad one).
	 */
	{
		ior->io_temporary = BBR_ACTION_RETRY_READ;
		printf("%s", "retry read..");
		ior->io_residual = (io_buf_len_t) 0;
		scdisk_start_rw(tgt, ior);
	}

	return TRUE;
}

static void
make_msf(
	unsigned char	*buf,
	unsigned int	val)
{
	*buf++ = val >> 24;
	*buf++ = val >> 16;
	*buf++ = val >>  8;
	*buf++ = val >>  0;
}

/*
 * This effectively replaces the strategy routine during bbr.
 */
/*ARGSUSED*/
void
scdisk_bbr_start(
	target_info_t		*tgt,
	boolean_t		done)
{
	register io_req_t	ior = tgt->ior;
	char			*msg;

	switch (ior->io_temporary) {

	case BBR_ACTION_COMPLETE:

		/* all done, either way */
fin:
		tgt->flags &= ~TGT_BBR_ACTIVE;
		ior->io_recnum   = tgt->dev_info.disk.b.save_rec;
		ior->io_data     = tgt->dev_info.disk.b.save_addr;
		ior->io_count    = tgt->dev_info.disk.b.save_count;
		ior->io_residual = tgt->dev_info.disk.b.save_resid;

		if (tgt->done == SCSI_RET_SUCCESS) {
			/* restart normal life */
			register io_buf_len_t 	xferred;
			if (xferred = ior->io_residual) {
				ior->io_data -= xferred;
				ior->io_count += xferred;
				ior->io_recnum -= xferred / tgt->block_size;
				ior->io_residual = (io_buf_len_t) 0;
			}
			/* from the beginning */
			ior->io_error = 0;
			msg = "done, restarting.";
		} else {
			/* we could not fix it.  Tell user and give up */
			tgt->ior = ior->io_next;
			iodone(ior);
			msg = "done, but could not recover.";
		}

		printf("%s\n", msg);
		scdisk_start( tgt, FALSE);
		return;

	case BBR_ACTION_RETRY_READ:

		/* see if retry worked, if not do it again */
		if (tgt->done == SCSI_RET_SUCCESS) {
			unsigned char msf[sizeof(int)];

			/* whew, retry worked.  Now rewire that bad block
			 * and don't forget to copy the good data over */

			/*
			 * If the number of retries was within the
			 * soft error limit, don't reassign.
			 */
			if (tgt->dev_info.disk.b.retry_count < SOFT_RETRIES) {
				printf("%s", "recovered..");
				goto fin;
			}

			/*
			 * The number of retries was above the soft error
			 * limit.  Now reassign the bad block and don't
			 * forget to copy the good data over
			 */
			tgt->dev_info.disk.b.retry_count = 0;
			printf("%s", "ok now, reassign..");
			ior->io_temporary = BBR_ACTION_COPY;
			make_msf(msf, tgt->dev_info.disk.b.badblockno);
			scsi_reassign_blocks( tgt,(unsigned int *)msf, 1, ior);
			return;
		}
		else if (++tgt->dev_info.disk.b.retry_count < scsi_bbr_retries) {
			(void) scdisk_start_rw( tgt, ior);
			return;
		}
		/* retrying was hopeless. Leave the bad block there for maintainance */
		/* because we do not know what to write on it */
		printf("%s%d%s", "failed after ", scsi_bbr_retries, " retries..");
		goto fin;


	case BBR_ACTION_RETRY_WRITE:

		/* see if retry worked, if not do it again */
		if (tgt->done == SCSI_RET_SUCCESS) {

			/* Whew, retry worked. */

			/*
			 * AND the number of retries was
			 * within the soft error limit
			 */
			goto fin;
		}
		else if (++tgt->dev_info.disk.b.retry_count < SOFT_RETRIES) {
			(void) scdisk_start_rw( tgt, ior);
			return;
		}

		/*
		 * Retrying was hopeless.
		 * Just reassign the block and get on with it.
		 */
		{
			unsigned char msf[sizeof(int)];

			printf("%s", "reassign..");
			make_msf(msf, tgt->dev_info.disk.b.badblockno);
			scsi_reassign_blocks( tgt, (unsigned int *)msf, 1, ior);
		}
		goto fin;
		break;


	case BBR_ACTION_COPY:

		/* retrying succeded and we rewired the bad block. */
		if (tgt->done == SCSI_RET_SUCCESS) {
			unsigned int tmp;

			printf("%s", "ok, rewrite..");

			/* writeback only the bad sector */

			/* map blockno back to partition offset */
			tmp = rzpartition(ior->io_unit);
			tmp = tgt->dev_info.disk.b.badblockno - 
			      tgt->dev_info.disk.l.d_partitions[tmp].p_offset;
			ior->io_data += (tmp - ior->io_recnum)*tgt->block_size;
			ior->io_recnum = tmp;
			ior->io_count = tgt->block_size;
			ior->io_op &= ~IO_READ;

			ior->io_temporary = BBR_ACTION_VERIFY;
			(void) scdisk_start_rw( tgt, ior);
		} else {

			/* either unsupported command, or repl table full */
			printf("%s", "reassign failed (really needs reformatting), ");
			ior->io_error = 0;
			goto fin;
		}
		break;

	case BBR_ACTION_VERIFY:

		if (tgt->done == SCSI_RET_SUCCESS) {
			ior->io_op |= IO_READ;
			goto fin;
		}
		else if (++tgt->dev_info.disk.b.retry_count < scsi_bbr_retries) {
			/* retry, we are *this* close to success.. */
			(void) scdisk_start_rw( tgt, ior);
			return;
		}

		printf("%s%d%s",
			"failed after ", scsi_bbr_retries, " retries..");
		ior->io_op |= IO_READ;
		goto fin;

		break;

	case BBR_ACTION_REASSIGN:

		/* if we wanted to issue the reassign multiple times */
		/* XXX unimplemented XXX */
		break;

	default:	/* snafu */
		panic("scdisk_bbr_start");
	}
}
