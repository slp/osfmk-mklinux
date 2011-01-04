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
 * Revision 2.10.2.2  92/04/30  12:00:46  bernadat
 * 	Changes from TRUNK:
 * 		Use scsi_print_sense_data where applicable.
 * 		[92/04/06            af]
 * 
 * Revision 2.10.2.1  92/03/28  10:15:44  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:32:28  jeffreyh]
 * 
 * Revision 2.11  92/02/23  22:44:37  elf
 * 	Changed the interface of a number of functions not to
 * 	require the scsi_softc pointer any longer.  It was
 * 	mostly unused, now it can be found via tgt->masterno.
 * 	[92/02/22  19:30:10  af]
 * 
 * Revision 2.10  91/10/09  16:17:19  af
 * 	 Revision 2.9.1.2  91/09/01  17:16:50  af
 * 	 	Zero io_error before using the ior.
 * 	 Revision 2.9.1.1  91/08/29  18:08:50  af
 * 	 	Fixed to compile again under 2.5.
 * 
 * Revision 2.9.1.2  91/09/01  17:16:50  af
 * 	Zero io_error before using the ior.
 * 
 * Revision 2.9.1.1  91/08/29  18:08:50  af
 * 	Fixed to compile again under 2.5.
 * 
 * Revision 2.9  91/08/24  12:28:15  af
 * 	Flag tapes as exclusive open, Spl defs, now we
 * 	understand fixed-size tapes (QIC-11), multiP locks.
 * 	[91/08/02  03:56:41  af]
 * 
 *	Cast args for bcopy.
 * 
 * Revision 2.8  91/06/25  20:56:33  rpd
 * 	Tweaks to make gcc happy.
 * 
 * Revision 2.7  91/06/19  11:57:12  rvb
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.6  91/05/14  17:27:00  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/05/13  06:34:29  af
 * 	Do not say we failed to bspfile when the target really was
 * 	only busy.
 * 	Retrieve from mode_sense speed, density and writeprotect info.
 * 	Deal with targets that are busy, note when we get confused due
 * 	to a scsi bus reset.  Set speed and density if user asks to.
 * 	Generally, made it work properly [it passes Rudy's tests].
 * 	Tapes really work now.
 * 
 * Revision 2.4.2.1  91/05/12  16:05:10  af
 * 	Do not say we failed to bspfile when the target really was
 * 	only busy.
 * 
 * 	Retrieve from mode_sense speed, density and writeprotect info.
 * 	Deal with targets that are busy, note when we get confused due
 * 	to a scsi bus reset.  Set speed and density if user asks to.
 * 	Generally, made it work properly [it passes Rudy's tests].
 * 
 * 	Tapes really work now.
 * 
 * Revision 2.4.1.3  91/04/05  13:10:07  af
 * 	Do not say we failed to bspfile when the target really was
 * 	only busy.
 * 
 * Revision 2.4.1.2  91/03/29  17:10:38  af
 * 	Retrieve from mode_sense speed, density and writeprotect info.
 * 	Deal with targets that are busy, note when we get confused due
 * 	to a scsi bus reset.  Set speed and density if user asks to.
 * 	Generally, made it work properly [it passes Rudy's tests].
 * 
 * Revision 2.4.1.1  91/02/21  19:00:22  af
 * 	Tapes really work now.
 * 
 * 
 * Revision 2.4  91/02/05  17:44:01  mrt
 * 	Added author notices
 * 	[91/02/04  11:17:11  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:15:49  mrt]
 * 
 * Revision 2.3  90/12/05  23:34:04  af
 * 	Mild attempt to get it working.  Needs lots of work still.
 * 	[90/12/03  23:34:27  af]
 * 
 * Revision 2.1.1.1  90/11/01  03:44:11  af
 * 	Created.
 * 	[90/10/21            af]
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
 *	File: rz_tape.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	10/90
 *
 *	Top layer of the SCSI driver: interface with the MI.
 *	This file contains operations specific to TAPE-like devices.
 */

#include <mach/std_types.h>
#include <kern/spl.h>		/* spl definitions */
#include <kern/misc_protos.h>
#include <machine/machparam.h>
#include <scsi/compat_30.h>

#include <sys/ioctl.h>
#include <device/tape_status.h>
#include <device/misc_protos.h>
#include <device/ds_routines.h>

#include <scsi/scsi.h>
#include <scsi/scsi_defs.h>
#include <scsi/rz.h>

int scsi_tape_timeout = 5*60;	/* secs, tk50 is slow when positioning far apart */

#define TAPE_RETRIES	10

/*
 * Tape types
 */

#define SCSI_RR_TAPE	1 /* reel to reel */
#define SCSI_CT_TAPE	2 /* Cartridge */
#define SCSI_CS_TAPE	3 /* Cassette */

/*
 * Tape codes
 */

#define SCSI_NRZI_CODE	1 /* Non Return to Zero, change on ones */
#define SCSI_CGR_CODE	2 /* Group Code recording */
#define SCSI_PE_CODE	3 /* Phase Encoded */
#define SCSI_IMFM_CODE	4 /* Inverted Modified Frequency Modulation */
#define SCSI_MFM_CODE	5 /* Modified Frequency Modulation */
#define SCSI_DDS_CODE	6 /* DAT Data Storage */
#define SCSI_HSR_CODE	6 /* */

/* 
 * Indexed by the density code, gives the tape type, code & density
 * As decribed in SCSI-2 ANSI Draft August 1988.
 */

struct scsi_tape_density {
	char td_type;	
/*     	char td_code;	*/ /* Unused for now */
/*	int  td_bpi;	*/ /* Unused for now */
};

struct scsi_tape_density scsi_tape_density [] = {
/* 0*/	0,	      	/* 0,		     0, *//* Devices's Default */
/* 1*/	SCSI_RR_TAPE,	/* SCSI_NRZI_CODE,	   800, */ /* X3.22-1983 */
/* 2*/	SCSI_RR_TAPE,	/* SCSI_PE_CODE,	  1600, */ /* X3.39-1986 */
/* 3*/	SCSI_RR_TAPE,	/* SCSI_CGR_CODE,	  6250, */ /* X3.54-1986 */
/* 4*/	SCSI_CT_TAPE,	/* SCSI_CGR_CODE,	  8000, */ /* X3.136-198 */
/* 5*/	SCSI_CT_TAPE,	/* SCSI_CGR_CODE,	  8000, */ /* X3.136-1986 */
/* 6*/	SCSI_RR_TAPE,	/* SCSI_PE_CODE,	  3200, */ /* X3.157-1987 */
/* 7*/	SCSI_CT_TAPE,	/* SCSI_IMFM_CODE,	  6400, */ /* X3.116-1986 */
/* 8*/	SCSI_CS_TAPE,	/* SCSI_CGR_CODE,	  8000, */ /* X3.158-1987 */
/* 9*/	SCSI_CT_TAPE,	/* SCSI_CGR_CODE,	 37871, */ /* X3B5/87-099 */
/* a*/	SCSI_CT_TAPE,	/* SCSI_MFM_CODE,	  6667, */ /* X3B5/86-199 */
/* b*/	SCSI_CT_TAPE,	/* SCSI_PE_CODE,	  1600, */ /* X3.56-1986 */
/* c*/	SCSI_CT_TAPE,	/* SCSI_CGR_CODE,	 12690, */ /* HI-TC1 */
/* d*/	SCSI_CT_TAPE,	/* SCSI_CGR_CODE,	 25380, */ /* HI-TC2 */
/* e*/	0,	        /* 0,		     0, */ /* Reserved for ECMA */
/* f*/	SCSI_CT_TAPE,	/* SCSI_CGR_CODE,	 10000, */ /* QIC120 */
/*10*/	SCSI_CT_TAPE,	/* SCSI_CGR_CODE,	 10000, */ /* QIC150 */
/*11*/	SCSI_CT_TAPE,	/* SCSI_NRZI_CODE,	 18000, */ /* QIC300 */
/*12*/	SCSI_CT_TAPE,	/* SCSI_NRZI_CODE,	 36000, */ /* QIC600 */
/*13*/	SCSI_CS_TAPE,	/* SCSI_DDS_CODE,	 61000, */ /* DAT */
/*14*/	SCSI_CS_TAPE,	/* SCSI_HSR_CODE,	 64000, */ /* X3B5/88-036 */
};

int scsi_tape_density_cnt =
      (sizeof (scsi_tape_density) / sizeof (struct scsi_tape_density));

int scsi_skip_eof_on_close = 1; /* If set to one, on close after reads we
				 * force a skip to the next file. Otherwise
				 * do nothing */  
scsi_ret_t	
sctape_open(
	target_info_t	*tgt,
	io_req_t	req)
{
	io_return_t	ret;
	io_req_t	ior;
	int		i;
	scsi_mode_sense_data_t *mod;

	req->io_device->flag |= D_EXCL_OPEN;

	/* Preferably allow tapes to disconnect */
	if (BGET(scsi_might_disconnect,(unsigned char)tgt->masterno,tgt->target_id))
		BSET(scsi_should_disconnect,(unsigned char)tgt->masterno,tgt->target_id);

	tgt->lun = tzlun(req->io_unit);

	/*
	 * Dummy ior for proper sync purposes
	 */
	io_req_alloc(ior);
	ior->io_count = 0;

	/* Exabyte wants a mode sense first */
	do {
		ior->io_op = IO_INTERNAL;
		ior->io_next = 0;
		ior->io_error = 0;
		ret = scsi_mode_sense(tgt, 0, 32, ior);
	} while (ret == SCSI_RET_RETRY);

	mod = (scsi_mode_sense_data_t *)tgt->cmd_ptr;
	if (scsi_debug) {
		int	p[5];
		bcopy((char*)mod, (char*)p, sizeof(p));
		printf("[modsns(%x): x%x x%x x%x x%x x%x]", ret,
			p[0], p[1], p[2], p[3], p[4]);
	}
	if (ret == SCSI_RET_DEVICE_DOWN)
		goto out;
	if (ret == SCSI_RET_SUCCESS) {
		tape_spec_t *tape_spec = (tape_spec_t *)&mod->device_spec;
		tgt->dev_info.tape.read_only = tape_spec->wp;
		tgt->dev_info.tape.speed = tape_spec->speed;
		tgt->dev_info.tape.density = mod->bdesc[0].density_code;
	}

	/* Some tapes have limits on record-length */
again:
	ior->io_op = IO_INTERNAL;
	ior->io_next = 0;
	ior->io_error = 0;
	ret = scsi_read_block_limits( tgt, ior);
	if (ret == SCSI_RET_RETRY) goto again;
	if (!ior->io_error && (ret == SCSI_RET_SUCCESS)) {
		scsi_blimits_data_t	*lim;
		int			maxl;

		lim = (scsi_blimits_data_t *) tgt->cmd_ptr;

		tgt->block_size = (lim->minlen_msb << 8) |
				   lim->minlen_lsb;

		maxl =	(lim->maxlen_msb << 16) |
			(lim->maxlen_sb  <<  8) |
			 lim->maxlen_lsb;
		if (maxl == 0)
			maxl = (unsigned)-1;
		tgt->dev_info.tape.maxreclen = maxl;
		tgt->dev_info.tape.fixed_size = (maxl == tgt->block_size);
	} else {
		/* let the user worry about it */
		tgt->dev_info.tape.maxreclen = (unsigned)-1;
		tgt->dev_info.tape.fixed_size = FALSE;
	}

	/*
	 * Try hard to do a mode select.
 	 *
	 * On Cartdridge tapes, mode_select at other locations than
	 * BOF seem to fail (error 30 00, Incompatible medium Installed).
	 * This is the case for QIC120 and QIC150 at least.
	 * If density is null (Density not detected yet, BOF) or does
	 * not correspond to a cartridge tape we can invoke mode_select
	 */

	if ((tgt->dev_info.tape.density < scsi_tape_density_cnt) &&
	    (scsi_tape_density[tgt->dev_info.tape.density].td_type
	     != SCSI_CT_TAPE) )
	  	for (i = 0; i < 5; i++) {
	    		ior->io_op = IO_INTERNAL;
			ior->io_error = 0;
			ret = sctape_mode_select(tgt, 0, 0, FALSE, ior);
			if (ret == SCSI_RET_SUCCESS)
				break;
		}
	if (scsi_watchdog_period < scsi_tape_timeout)
		scsi_watchdog_period += scsi_tape_timeout;

#if 0	/* this might imply rewind, which we do not want, although yes, .. */
	/* we want the tape loaded */
	ior->io_op = IO_INTERNAL;
	ior->io_next = 0;
	ior->io_error = 0;
	ret = scsi_start_unit(tgt, SCSI_CMD_SS_START, ior);
#endif

out:
	io_req_free(ior);
	return ret;
}


scsi_ret_t
sctape_close(
	dev_t		dev,
	target_info_t	*tgt)
{
	io_return_t	ret = SCSI_RET_SUCCESS;
	io_req_t	ior;

	tgt->lun = tzlun(dev);

	/*
	 * Dummy ior for proper sync purposes
	 */
	io_req_alloc(ior);
	ior->io_op = IO_INTERNAL;
	ior->io_next = 0;
	ior->io_count = 0;

	if (tgt->flags & TGT_REMOVABLE_MEDIA) {
		ior->io_error = 0;
		/* too many dont support it. Sigh */
		tgt->flags |= TGT_OPTIONAL_CMD;
		(void) scsi_medium_removal( tgt, TRUE, ior);
		tgt->flags &= ~TGT_OPTIONAL_CMD;
		ior->io_op = IO_INTERNAL;
		ior->io_count = 0;
	}

	if (tgt->ior) printf("TAPE: Close with pending requests ?? \n");

	if ( ! (tgt->flags & TGT_ONLINE)) { /* the tape is already offline */
		io_req_free(ior);
		tgt->flags &= ~(TGT_WRITTEN_TO|TGT_REWIND_ON_CLOSE);
        	return ret;
	}

	/* 
	 * write a filemark if we xtnded/truncated the tape
	 *
	 * On cartridge tapes, It seems impossible to overwrite 
	 * after a back space command (error 50 00 Write Append error).
	 * Write a single file mark and do not back space.
	 */

	if (tgt->flags & TGT_WRITTEN_TO) {
		int double_file_mark = 1;
		int density_code;

		/* Do a mode sense first, to get effective density */

		do {
			ior->io_op = IO_INTERNAL;
			ior->io_next = 0;
			ior->io_error = 0;
			ret = scsi_mode_sense(tgt, 0, 32, ior);
		} while (ret == SCSI_RET_RETRY);

		density_code = ((scsi_mode_sense_data_t *)tgt->cmd_ptr)->bdesc[0].density_code;

		if (density_code < scsi_tape_density_cnt &&
		    scsi_tape_density[density_code].td_type == SCSI_CT_TAPE)
			double_file_mark = 0;

		tgt->ior = ior;
		ior->io_error = 0;
		ior->io_op = IO_INTERNAL;
		ior->io_next = 0;
		ret = scsi_write_filemarks(tgt, (double_file_mark) ? 2 : 1, ior);
		if (ret != SCSI_RET_SUCCESS)
			 printf("%s%d: wfmark failed x%x\n",
			 (*tgt->dev_ops->driver_name)(TRUE), tgt->target_id, ret);
		/*
		 * Don't bother repositioning if we'll rewind it
		 */
		if (tgt->flags & TGT_REWIND_ON_CLOSE)
			goto rew;

		if (!double_file_mark)
			goto rew;
retry:
		tgt->ior = ior;
		ior->io_op = IO_INTERNAL;
		ior->io_error = 0;
		ior->io_next = 0;
		ret = scsi_space(tgt, SCSI_CMD_SP_FIL, -1, ior);
		if (ret != SCSI_RET_SUCCESS) {
			if (ret == SCSI_RET_RETRY) {
				timeout((timeout_fcn_t)wakeup, tgt, hz);
				await(tgt);
				goto retry;
			}
			printf("%s%d: bspfile failed x%x\n",
			 (*tgt->dev_ops->driver_name)(TRUE), tgt->target_id, ret);
		}
	}
rew:
	if (tgt->flags & TGT_REWIND_ON_CLOSE) {
		/* Rewind tape */
		ior->io_error = 0;
		ior->io_op = IO_INTERNAL;
		ior->io_error = 0;
		tgt->ior = ior;
		(void) scsi_rewind(tgt, ior, FALSE);
		iowait(ior);
		if (tgt->done == SCSI_RET_RETRY) {
			timeout((timeout_fcn_t)wakeup, tgt, 5*hz);
			await(tgt);
			goto rew;
		}
	} else if (scsi_skip_eof_on_close && 
		   (tgt->flags & TGT_DATA_WAS_READ) &&
		   ((tgt->flags & TGT_FOUND_EOF) == 0)) {
		/* skip eof */
skip:
		tgt->ior = ior;
		ior->io_op = IO_INTERNAL;
		ior->io_error = 0;
		ior->io_next = 0;
		ret = scsi_space(tgt, SCSI_CMD_SP_FIL, 1, ior);
		if (ret != SCSI_RET_SUCCESS) {
			if (ret == SCSI_RET_RETRY) {
				timeout((timeout_fcn_t)wakeup, tgt, hz);
				await(tgt);
				goto skip;
			}
			printf("%s%d: skip eof failed x%x\n",
			 (*tgt->dev_ops->driver_name)(TRUE), tgt->target_id, ret);
		}
	}
	
	io_req_free(ior);

	tgt->flags &= ~(TGT_WRITTEN_TO|TGT_REWIND_ON_CLOSE|
			TGT_DATA_WAS_READ|TGT_FOUND_EOF);
	return ret;
}

void
sctape_strategy(
	register io_req_t	ior)
{
	target_info_t  *tgt;
	register int    i = ior->io_unit;

	/*
	 * Don't do anything for a pass-through device, just acknowledge
	 * the request.  This gives us a null SCSI device.
	 */
	if (rzpassthru(i)) {
		ior->io_residual = 0;
		iodone(ior);
		return;
	}

	tgt = scsi_softc[rzcontroller(i)]->target[rzslave(i)];

	if (((ior->io_op & IO_READ) == 0) &&
	    tgt->dev_info.tape.read_only) {
		ior->io_error = D_INVALID_OPERATION;
		ior->io_op |= IO_ERROR;
		ior->io_residual = ior->io_count;
		iodone(ior);
		return;
	}

	rz_simpleq_strategy( ior, sctape_start);
}

static void
do_residue(
	io_req_t	  ior,
	scsi_sense_data_t *sns,
	target_info_t	  *tgt)
{
	int residue;

	/* Not an error situation */
	ior->io_error = 0;
	ior->io_op &= ~IO_ERROR;

	if (!sns->addr_valid) {
		ior->io_residual = ior->io_count;
		return;
	}

	residue = sns->u.xtended.info0 << 24 |
		  sns->u.xtended.info1 << 16 |
		  sns->u.xtended.info2 <<  8 |
		  sns->u.xtended.info3;
	if(tgt->dev_info.tape.fixed_size)
		residue *= tgt->block_size;
	/*
	 * NOTE: residue == requested - actual
	 * We only care if > 0
	 */
	if (residue < 0) residue = 0;/* sanity */
	ior->io_residual += residue;
}

void
sctape_start(
	target_info_t	*tgt,
	boolean_t	done)
{
	io_req_t		head, ior = tgt->ior;

	if (ior == 0)
		return;

	if (done) {
                register unsigned int   xferred;
                unsigned int            max;

                max = scsi_softc[tgt->masterno]->max_dma_data;
		if (!tgt->dev_info.tape.fixed_size) {
			if (max > tgt->dev_info.tape.maxreclen)
				max = tgt->dev_info.tape.maxreclen;
		}

		/* see if we must retry */
		if ((tgt->done == SCSI_RET_RETRY) &&
		    ((ior->io_op & IO_INTERNAL) == 0)) {
			if(tgt->dev_info.tape.retry_count++ == TAPE_RETRIES) {
				tgt->done = SCSI_RET_DEVICE_DOWN;
			} else {
			    timeout((timeout_fcn_t)wakeup, tgt, hz);
			    await(tgt);
			    goto start;
			}
		}
		/* got a bus reset ? ouch, that hurts */
		if (tgt->done == (SCSI_RET_ABORTED|SCSI_RET_RETRY)) {
			/*
			 * we really cannot retry because the tape position
			 * is lost.
			 */
			printf("Lost tape position\n");
			ior->io_error = D_IO_ERROR;
			ior->io_op |= IO_ERROR;
		} else

		/* check completion status */

		if ((tgt->cur_cmd == SCSI_CMD_REQUEST_SENSE) &&
		    (!rzpassthru(ior->io_unit))) {
			scsi_sense_data_t *sns;

			ior->io_op = ior->io_temporary;
			ior->io_error = D_IO_ERROR;
			ior->io_op |= IO_ERROR;

			sns = (scsi_sense_data_t *)tgt->cmd_ptr;

			if (scsi_debug)
				scsi_print_sense_data(sns);

			if (scsi_check_sense_data(tgt, sns)) {
			    if (sns->u.xtended.ili) {
				if (ior->io_op & IO_READ) {
				    do_residue(ior, sns, tgt);
				    if (scsi_debug)
					printf("Tape Short Read (%ld)\n",
						ior->io_residual);
				}
			    } else if (sns->u.xtended.eom) {
				do_residue(ior, sns, tgt);
				if (scsi_debug)
					printf("End of Physical Tape!\n");
			    } else if (sns->u.xtended.fm) {
				tgt->flags |= TGT_FOUND_EOF;
				do_residue(ior, sns, tgt);
				if (scsi_debug)
					printf("File Mark\n");
			    }
			}
		}

		else if ((tgt->done != SCSI_RET_SUCCESS) &&
			 (!rzpassthru(ior->io_unit))) {

		    if (tgt->done == SCSI_RET_NEED_SENSE) {

			ior->io_temporary = ior->io_op;
			ior->io_op = IO_INTERNAL;
			if (scsi_debug)
				printf("[NeedSns x%lx x%lx]", ior->io_residual, ior->io_count);
			scsi_request_sense(tgt, ior, 0);
			return;

		    } else if (tgt->done == SCSI_RET_RETRY) {
			/* only retry here READs and WRITEs */
			if ((ior->io_op & IO_INTERNAL) == 0) {
				ior->io_residual = 0;
				goto start;
			} else{
				ior->io_error = D_WOULD_BLOCK;
				ior->io_op |= IO_ERROR;
			}
		    } else {
			ior->io_error = D_IO_ERROR;
			ior->io_op |= IO_ERROR;
		    }
		} else  {
			if ( (ior->io_op & IO_READ) && 
			    (ior->io_op & IO_INTERNAL) == 0) {
				tgt->flags |= TGT_DATA_WAS_READ;
				tgt->flags &= ~TGT_FOUND_EOF;
			}

                	/*
			 * No errors.
			 * See if we requested more than the max
			 * (We use io_residual in a flip-side way here)
			 */

                	if (ior->io_count > (xferred = max)) {
				ior->io_residual += xferred;
				ior->io_count -= xferred;
				ior->io_data += xferred;
				ior->io_recnum += xferred / tgt->block_size;
				if (scsi_debug){
					printf("[sctape_start:resid %d]",
					       ior->io_residual);
					printf(" [io_count %d]\n",
					       ior->io_count);
				}
                        	goto start;
                	} else if (xferred = ior->io_residual) {
                        	ior->io_data -= xferred;
				ior->io_count += xferred;
				ior->io_recnum -= xferred / tgt->block_size;
				ior->io_residual = 0;
                	}
		}

		if (scsi_debug)
			printf("[Residual x%lx]", ior->io_residual);

		/* If this is a pass-through device, save the target result */
		if (rzpassthru(ior->io_unit)) ior->io_error = tgt->done;

		/* dequeue next one */
		head = ior;

		simple_lock(&tgt->target_lock);
		ior = head->io_next;
		tgt->ior = ior;
		if (ior)
			ior->io_prev = head->io_prev;
		simple_unlock(&tgt->target_lock);

		iodone(head);

		if (ior == 0)
			return;
	}
	ior->io_residual = 0;
	tgt->dev_info.tape.retry_count = 0;
start:
	tgt->lun = tzlun(ior->io_unit);
	if (ior->io_op & IO_READ) {
		if(tgt->flags & TGT_FOUND_EOF) {
	                ior->io_residual = ior->io_count;
			iodone(ior);
		} else {
			tgt->flags &= ~TGT_WRITTEN_TO;
			sctape_read( tgt, ior );
		}
	} else if ((ior->io_op & IO_INTERNAL) == 0) {
		tgt->flags |= TGT_WRITTEN_TO;
		sctape_write( tgt, ior );
	}
}

/*ARGSUSED*/
io_return_t
sctape_get_status(
	dev_t			dev,
	target_info_t		*tgt,
	dev_flavor_t		flavor,
	dev_status_t		status,
	mach_msg_type_number_t	*status_count)
{
	tgt->lun = tzlun(dev);

	switch (flavor) {

	case DEV_GET_SIZE:
		status[DEV_GET_SIZE_DEVICE_SIZE] = tgt->dev_info.tape.maxreclen;
		status[DEV_GET_SIZE_RECORD_SIZE] = tgt->block_size;
		*status_count = DEV_GET_SIZE_COUNT;
		break;

	case TAPE_STATUS: {
		struct tape_status *ts = (struct tape_status *) status;

		ts->mt_type		= MACH_MT_ISSCSI;
		ts->speed		= tgt->dev_info.tape.speed;
		ts->density		= tgt->dev_info.tape.density;
		ts->flags		= (tgt->flags & TGT_REWIND_ON_CLOSE) ?
						TAPE_FLG_REWIND : 0;
		if (tgt->dev_info.tape.read_only)
			ts->flags |= TAPE_FLG_WP;
		*status_count = TAPE_STATUS_COUNT;
		break;
	    }
	/* U*x compat */
	case MACH_MTIOCGET: {
		struct mach_mtget *g = (struct mach_mtget *) status;

		bzero((char *)g, sizeof(struct mach_mtget));
#ifdef	PARAGON860
		g->mt_type = MACH_MT_ISSCSI;
#else	/*PARAGON860*/
		g->mt_type = 0x7;	/* Ultrix compat */
#endif  /*PARAGON860*/
		*status_count = sizeof(struct mach_mtget)/sizeof(int);
		break;
	    }
	default:
		return D_INVALID_OPERATION;
	}
	return D_SUCCESS;
}

/*ARGSUSED*/
io_return_t
sctape_set_status(
	dev_t			dev,
	target_info_t		*tgt,
	dev_flavor_t		flavor,
	dev_status_t		status,
	mach_msg_type_number_t	status_count)
{
	scsi_ret_t		ret;

	tgt->lun = tzlun(dev);

	switch (flavor) {
	case TAPE_STATUS: {
		struct tape_status *ts = (struct tape_status *) status;
		if (ts->flags & TAPE_FLG_REWIND)
			tgt->flags |= TGT_REWIND_ON_CLOSE;
		else
			tgt->flags &= ~TGT_REWIND_ON_CLOSE;

		if (ts->speed || ts->density) {
			unsigned int ospeed, odensity;
			io_req_t	ior;

			io_req_alloc(ior);
			ior->io_op = IO_INTERNAL;
			ior->io_error = 0;
			ior->io_next = 0;
			ior->io_count = 0;

			ospeed = tgt->dev_info.tape.speed;
			odensity = tgt->dev_info.tape.density;
			tgt->dev_info.tape.speed = ts->speed;
			tgt->dev_info.tape.density = ts->density;

			ret = sctape_mode_select(tgt, 0, 0, (ospeed == ts->speed), ior);
			if (ret != SCSI_RET_SUCCESS) {
				tgt->dev_info.tape.speed = ospeed;
				tgt->dev_info.tape.density = odensity;
			}

			io_req_free(ior);
		}

		break;
	    }
	/* U*x compat */
	case MACH_MTIOCTOP: {
		struct tape_params *mt = (struct tape_params *) status;
		io_req_t	ior;

		if (scsi_debug)
			printf("[sctape_sstatus: %x %x %x]\n",
				flavor, mt->mt_operation, mt->mt_repeat_count);

		io_req_alloc(ior);
retry:
		ior->io_count = 0;
		ior->io_op = IO_INTERNAL;
		ior->io_error = 0;
		ior->io_next = 0;
		tgt->ior = ior;

		switch (mt->mt_operation) {
		case MACH_MTWEOF:	/* write an end-of-file record */
			ret = scsi_write_filemarks(tgt, mt->mt_repeat_count, ior);
			tgt->flags &= ~TGT_WRITTEN_TO;
			break;
		case MACH_MTFSF:	/* forward space file */
			ret = scsi_space(tgt, SCSI_CMD_SP_FIL, mt->mt_repeat_count, ior);
			break;
		case MACH_MTBSF:	/* backward space file */
			ret = scsi_space(tgt, SCSI_CMD_SP_FIL, -mt->mt_repeat_count,ior);
			tgt->flags &= ~TGT_WRITTEN_TO;
			break;
		case MACH_MTFSR:	/* forward space record */
			ret = scsi_space(tgt, SCSI_CMD_SP_BLOCKS, mt->mt_repeat_count, ior);
			break;
		case MACH_MTBSR:	/* backward space record */
			ret = scsi_space(tgt, SCSI_CMD_SP_BLOCKS, -mt->mt_repeat_count, ior);
			tgt->flags &= ~TGT_WRITTEN_TO;
			break;
		case MACH_MTREW:	/* rewind */
		case MACH_MTOFFL:	/* rewind and put the drive offline */
			ret = scsi_rewind(tgt, ior, TRUE);
			iowait(ior);
			tgt->flags &= ~TGT_WRITTEN_TO;
			if (mt->mt_operation == MACH_MTREW) break;

			/* unlock removable media - tape should be removable, but check anyway */
			if (tgt->flags & TGT_REMOVABLE_MEDIA) {
				ior->io_op = IO_INTERNAL;
				ior->io_next = 0;
				ior->io_error = 0;
				/* too many dont support it. Sigh */
				tgt->flags |= TGT_OPTIONAL_CMD;
				(void) scsi_medium_removal( tgt, TRUE, ior);
				tgt->flags &= ~TGT_OPTIONAL_CMD;
			}

			ior->io_op = 0;
			ior->io_next = 0;
			ior->io_error = 0;
			(void) scsi_start_unit(tgt, 0, ior);

			/* mark drive as offline */
			tgt->flags &= ~(TGT_ONLINE|TGT_REWIND_ON_CLOSE);
			break;
		case MACH_MTNOP:	/* no operation, sets status only */
		case MACH_MTCACHE:	/* enable controller cache */
		case MACH_MTNOCACHE:	/* disable controller cache */
			ret = SCSI_RET_SUCCESS;
			break;
		default:
			tgt->ior = 0;
			io_req_free(ior);
			return D_INVALID_OPERATION;
		}

		if (ret == SCSI_RET_RETRY) {
			timeout((timeout_fcn_t)wakeup, ior, 5*hz);
			await(ior);
			goto retry;
		}

		io_req_free(ior);
		if (ret != SCSI_RET_SUCCESS || ior->io_error)
			return D_IO_ERROR;
		break;
	}
	case MACH_MTIOCIEOT:
	case MACH_MTIOCEEOT:
	default:
		return D_INVALID_OPERATION;
	}
	return D_SUCCESS;
}
