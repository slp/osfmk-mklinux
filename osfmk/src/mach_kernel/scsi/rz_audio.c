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
 * Revision 2.4  93/03/26  18:03:33  mrt
 * 	Tested NEC driver.  Tested Sony driver.
 * 	[93/03/22            af]
 * 
 * 	Rewritten a lot.  Do not even try (some) SCSI-2 commands on those
 * 	drivers that aren't even SCSI-1 compliant.  Use instead a list of
 * 	vendor-specific work-arounds.
 * 	[93/03/22            af]
 * 
 * 	Functionality is now complete for RRD42.
 * 	[93/03/17            af]
 * 
 * Revision 2.3  93/03/11  14:08:33  danner
 * 	Forward decl. Replaced gimmeabreak with panic.
 * 
 * Revision 2.2  93/03/09  12:26:46  danner
 * 	Created.
 * 	[93/03/09            af]
 */
/* CMU_ENDHIST */ 
/*
 *	File: rz_audio.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	3/93
 *
 *	Top layer of the SCSI driver: interface with the MI.
 *	This file contains operations specific to audio CD-ROM devices.
 *	Unlike many others, it sits on top of the rz.c module.
 */

#include <mach/std_types.h>
#include <string.h>
#include <kern/spl.h>			/* spl definitions */
#include <machine/machparam.h>
#include <vm/vm_kern.h>
#include <device/ds_routines.h>
#include <mach/mach_server.h>		/* vm_deallocate() */

#include <scsi/compat_30.h>
#include <scsi/scsi.h>
#include <scsi/scsi2.h>
#include <scsi/scsi_defs.h>
#include <scsi/rz.h>
#include <kern/misc_protos.h>
#include <device/cdrom_status.h>
#include <device/conf.h>

#define RZ_MACSD(funcname) rz_##funcname

/* some data is two BCD digits in one byte */
#define	bcd_to_decimal(b)	(((b)&0xf) + 10 * (((b) >> 4) & 0xf))
#define decimal_to_bcd(b)	((((b) / 10) << 4)  |  ((b) % 10))

/*
 * Regular use of a CD-ROM is for data, and is handled
 * by the default set of operations. Ours is for funtime..
 */

static char unsupported[] = "Device does not support it.";

/*
 * Unfortunately, none of the vendors appear to
 * abide by the SCSI-2 standard and many of them
 * violate or stretch even the SCSI-1 one.
 * Therefore, we keep a red-list here of the worse
 * offendors and how to deal with them.
 * The user is notified of the problem and invited
 * to solicit his vendor to upgrade the firmware.
 * [They had plenty of time to do so]
 */
typedef struct red_list {
	char	*vendor;
	char	*product;
	char	*rev;
	/*
	 * The standard MANDATES [par 13.1.6] the play_audio command
	 * at least as a way to discover if the device
	 * supports audio operations at all. This is the only way
	 * we need to use it.
	 */
	scsi_ret_t	(*can_play_audio)( target_info_t *, dev_flavor_t,
					   dev_status_t , mach_msg_type_number_t *,
					   io_req_t );
	/*
	 * The standard defines the use of start_stop_unit to
	 * cause the drive to eject the disk.
	 */
	scsi_ret_t	(*eject)( target_info_t *, dev_flavor_t,
				  dev_status_t , mach_msg_type_number_t,
				  io_req_t );
	/*
	 * The standard defines read_subchannel as a way to
	 * get the current playing position.
	 */
	scsi_ret_t	(*current_position)( target_info_t *, dev_flavor_t,
					     dev_status_t ,
					     mach_msg_type_number_t *, io_req_t );
	/*
	 * The standard defines read_table_of_content to get
	 * the listing of audio tracks available.
	 */
	scsi_ret_t	(*read_toc)( target_info_t *, dev_flavor_t,
				     dev_status_t , mach_msg_type_number_t *,
				     io_req_t );
	/*
	 * The standard defines read_subchannel as the way to
	 * report the current audio status (playing/stopped/...).
	 */
	scsi_ret_t	(*get_status)( target_info_t *, dev_flavor_t,
				       dev_status_t , mach_msg_type_number_t *,
				       io_req_t );
	/*
	 * The standard defines two ways to issue a play command,
	 * depending on the type of addressing used.
	 */
	scsi_ret_t	(*play_msf)( target_info_t *, dev_flavor_t,
				     dev_status_t , mach_msg_type_number_t,
				     io_req_t );
	scsi_ret_t	(*play_ti)( target_info_t *, dev_flavor_t,
				    dev_status_t , mach_msg_type_number_t,
				    io_req_t );
	/*
	 * The standard defines the pause_resume command to
	 * suspend or resume playback of audio data.
	 */
	scsi_ret_t	(*pause_resume)( target_info_t *, dev_flavor_t,
					 dev_status_t , mach_msg_type_number_t,
					 io_req_t );
	/*
	 * The standard defines the audio page among the
	 * mode selection options as a way to control
	 * both volume and connectivity of the channels
	 */
	scsi_ret_t	(*volume_control)( target_info_t *, dev_flavor_t,
					   dev_status_t , mach_msg_type_number_t,
					   io_req_t );
} red_list_t;

#define	if_standard(some_cmd)						\
	if (tgt->cdrom_info.violates_standards &&			\
	    tgt->cdrom_info.violates_standards->some_cmd)		\
	    rc = (*tgt->cdrom_info.violates_standards->some_cmd)	\
			(tgt,flavor,status,status_count,ior);	\
	else

/*
 * So now that you know what they should have implemented :-),
 * check at the end of the file what the naughty boys did instead.
 */
/* static red_list_t	audio_replacements[];	/ * at end */

/*
 *	Function prototypes for CD-ROM internal services
 */

io_return_t		cd_command(
				dev_t		dev,
				char		*cmd,
				io_buf_len_t	count,
				io_req_t	req);
static unsigned char	decode_status_1(
				unsigned char	audio_status);
static void		curse_the_vendor(
				red_list_t	*list,
				boolean_t	not_really);
static void		zero_ior(
				io_req_t	ior );
void			cd_strategy(
				io_req_t	ior);
void			cd_start(
				target_info_t	*tgt,
				boolean_t	done);
static scsi_ret_t	get_op_not_supported(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	*status_count,
				io_req_t	ior);
static scsi_ret_t	set_op_not_supported(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	status_count,
				io_req_t	ior);
#if	0
static scsi_ret_t	rrd42_status(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	*status_count,
				io_req_t	ior);
#endif
static scsi_ret_t	rrd42_set_volume(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	status_count,
				io_req_t	ior);
static scsi_ret_t	nec_eject(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	status_count,
				io_req_t	ior);
static scsi_ret_t	nec_subchannel(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	*status_count,
				io_req_t	ior);
static scsi_ret_t	nec_read_toc(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	*status_count,
				io_req_t	ior);
static scsi_ret_t	nec_play(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	status_count,
				io_req_t	ior);
static scsi_ret_t	nec_pause_resume(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	status_count,
				io_req_t	ior);
#if	0
static scsi_ret_t	pioneer_eject(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	status_count,
				io_req_t	ior);
static scsi_ret_t	pioneer_position(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	*status_count,
				io_req_t	ior);
static scsi_ret_t	pioneer_toc(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	*status_count,
				io_req_t	ior);
static scsi_ret_t	pioneer_status(
				target_info_t	*tgt,
				unsigned char	*status,
				io_req_t	ior);
static scsi_ret_t	pioneer_play(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	status_count,
				io_req_t	ior);
static scsi_ret_t	pioneer_pause_resume(
				target_info_t	*tgt,
				dev_flavor_t	flavor,
				dev_status_t	status,
				mach_msg_type_number_t	status_count,
				io_req_t	ior);
#endif
static void		check_red_list(
				target_info_t		*tgt,
				scsi2_inquiry_data_t	*inq);


static scsi_devsw_t	scsi_audio = {
	sccdrom_name, 0, 0, 0, cd_strategy, cd_start, 0, 0
};

/*
 * Open routine.  Does some checking, sets up
 * the replacement pointer.
 */
io_return_t
cd_open(
	dev_t		dev,
	dev_mode_t	mode,
	io_req_t	req)
{
	scsi_softc_t	*sc = 0;
	target_info_t	*tgt;
	int		ret;
	scsi_ret_t	rc;
	io_req_t	ior = 0;

	if (!RZ_MACSD(check)(dev, &sc, &tgt)) {
		/*
		 * Probe it again: might have installed a new device
		 */
		if (!sc || !scsi_probe(sc, &tgt,rzslave(dev),rzlun(dev),ior))
			return D_NO_SUCH_DEVICE;
		bzero((char *)&tgt->cdrom_info, sizeof(tgt->cdrom_info));
	}

        /*
         * Check this is indeed a CD-ROM
         */
        if (tgt->dev_ops != &scsi_devsw[SCSI_CDROM]) {
                RZ_MACSD(close)(dev);
                return D_NO_SUCH_DEVICE;
        }

	tgt->cdrom_info.open_count++; /* increment the open count */

	/* Pessimistic */
	ret = D_INVALID_OPERATION;

	if (mode & D_NODELAY) {
	  /*
	   * Try to bring unit online
	   */
	  ret = RZ_MACSD(open)(dev, mode, req);
	  if (ret) {
	    if (scsi_debug)
	      printf("Cannot bring unit online. ret= 0x%x\n", ret);
	  }
	  ret = D_SUCCESS;
	}
	else {
	  /*
	   * Bring unit online
	   */
	  ret = RZ_MACSD(open)(dev, mode, req);
	  if (ret)
	    goto bad;
	}

	/*
	 * Switch temporary to audio ops.
	 */
	tgt->dev_ops = &scsi_audio;

	/*
	 * Check if this device is on the red list
	 */
	{
	  scsi2_inquiry_data_t	*inq;
	  
	  scsi_inquiry(tgt, SCSI_INQ_STD_DATA);
	  inq = (scsi2_inquiry_data_t*)tgt->cmd_ptr;
	  check_red_list( tgt, inq );
	}
	
	/*
	 * See if this CDROM can play audio data
	 */	
	{
	  dev_flavor_t		flavor = 0;
	  dev_status_t		status = 0;
	  mach_msg_type_number_t	*status_count = 0;
	  
	  io_req_alloc(ior);		
	  zero_ior( ior );

	  if_standard(can_play_audio) {
	    rc = scsi_play_audio( tgt, 0, 0, FALSE, ior);
	    if (rc != SCSI_RET_SUCCESS) {
	      if (scsi_debug)
		printf("Can't play audio or tray open. rc= 0x%x\n", rc);
	    }
	  }

	io_req_free(ior);	
	}

	tgt->dev_ops = &scsi_devsw[SCSI_CDROM];

	return ret;
bad:
	if (ior) io_req_free(ior);
	tgt->cdrom_info.open_count--;	/* open failed */
	return ret;
}

/*
 * Close routine.
 */
void
cd_close(
	dev_t	dev)
{
	scsi_softc_t	*sc;
	target_info_t	*tgt;

	if (!RZ_MACSD(check)(dev, &sc, &tgt))
		return;
	if (!tgt)
		return;
	/*
	 * Decrement the open count.
	 */
	assert(tgt->cdrom_info.open_count > 0);
	tgt->cdrom_info.open_count--;

	if (tgt->cdrom_info.open_count > 0) {
		/* nothing to be done */ 
		return;
	}

	/*
	 * Cleanup state
	 */
	RZ_MACSD(close)(dev);

	/*
	 * Check the opening mode (AUDIO or DISK).
	 */
	scdisk_close(dev, tgt);

	return;
}

/*
 * Write routine. Operation not supported on any CDROM.
 */
io_return_t
cd_write(
	dev_t		dev,
	io_req_t	ior)
{
	if (scsi_debug)
	  printf("cd_write: 0x%x 0x%x : Operation not supported on CDROM.\n",
		 dev, ior);

	return D_INVALID_OPERATION;
}

/*
 * Read routine. Operation not supported on audio CDROM.
 * Just check if the request is directed to a file-system CDROM.
 */
io_return_t
cd_read(
	dev_t		dev,
	io_req_t	ior)
{

  return RZ_MACSD(read)(dev, ior);
}

/* 
 * driver info for kernel.
 */
io_return_t
cd_devinfo(
	dev_t		dev,
	dev_flavor_t	flavor,
	char		* info)
{
        switch (flavor) {
        case D_INFO_CLONE_OPEN:
                *((boolean_t *) info) = TRUE;
                break;
        default:
                return (D_INVALID_OPERATION);
        }
        return (D_SUCCESS);
}

/*
 * cd_get_status routine.
 */
io_return_t
cd_get_status(
	dev_t			dev,
	dev_flavor_t		flavor,
	dev_status_t		status,
	mach_msg_type_number_t	*status_count)
{
  target_info_t	*tgt;
  io_req_t	ior;
  io_return_t	ret = D_INVALID_OPERATION;
  scsi_ret_t	rc;

  tgt = scsi_softc[rzcontroller(dev)]->target[rzslave(dev)];

  if (scsi_debug)
    printf("cd_get_status: 0x%x 0x%x 0x%lx 0x%lx\n",
	   dev, flavor, status, *status_count);

  /*
   * Switch temporary to audio ops.
   */
  tgt->dev_ops = &scsi_audio;

  io_req_alloc(ior);		
  zero_ior( ior );

  switch (flavor) {
	case MACH_SCMD_READ_HEADER:
	  /* "Get TH" */
	  if_standard(read_toc) {
	    cd_toc_header_t *toc_header = (cd_toc_header_t *)status;
    
	    if (*status_count < ((sizeof(cd_toc_header_t) + 3) >> 2)) {
	      rc = SCSI_RET_ABORTED;
	      ret = D_INVALID_SIZE;
	    }
	    else {
	      rc = scsi_read_toc(tgt, TRUE, 1, PAGE_SIZE, ior);

	      if (rc == SCSI_RET_SUCCESS) {
		cdrom_toc_t *toc = (cdrom_toc_t *)tgt->cmd_ptr;

		toc_header->len = (toc->len1 << 8) + toc->len2;
		toc_header->first_track = toc->first_track;
		toc_header->last_track = toc->last_track;
		*status_count = ((sizeof(cd_toc_header_t) + 3) >> 2);

	      }
	    }
	  }
	break;

	case MACH_SCMD_READ_TOC_MSF:
	case MACH_SCMD_READ_TOC_LBA:
	  /* "Toc MSF|ABS trackno" */
	  if_standard(read_toc) {
	    cd_toc_t *cd_toc = (cd_toc_t *)status;
	    int tr = 1; /* starting track */
	    int tlen, et_count = 0, next_size;
	    int reserved_count = *status_count;

	    if (*status_count < ((sizeof(cd_toc_t) + 3) >> 2)) {
	      rc = SCSI_RET_ABORTED;
	      ret = D_INVALID_SIZE;
	    }
	    else {
	      rc = scsi_read_toc(tgt, (flavor == MACH_SCMD_READ_TOC_MSF ? TRUE : FALSE),
				       tr, PAGE_SIZE, ior);

	      if (rc == SCSI_RET_SUCCESS) {
		cdrom_toc_t *toc = (cdrom_toc_t *)tgt->cmd_ptr;

		tlen = (toc->len1 << 8) + toc->len2;
		tlen -= 4; /* header */

		for (tr = 0; tlen > 0; tr++, tlen -= sizeof(struct cdrom_toc_desc)){
		  cd_toc->cdt_ctrl = toc->descs[tr].control;
		  cd_toc->cdt_adr = toc->descs[tr].adr;
		  cd_toc->cdt_track = toc->descs[tr].trackno;

		  if (flavor == MACH_SCMD_READ_TOC_MSF) {
		    cd_toc->cdt_addr.msf.minute = 
		    	toc->descs[tr].absolute_address.msf.minute;
		    cd_toc->cdt_addr.msf.second =
		    	toc->descs[tr].absolute_address.msf.second;
		    cd_toc->cdt_addr.msf.frame = 
		    	toc->descs[tr].absolute_address.msf.frame;
		    cd_toc->cdt_type = MACH_CDROM_MSF;
		  }
		  else {	/* flavor == MACH_SCMD_READ_TOC_LBA */
		    cd_toc->cdt_addr.lba = 
		    	(toc->descs[tr].absolute_address.lba.lba1<<24)+
		    	(toc->descs[tr].absolute_address.lba.lba2<<16)+
		    	(toc->descs[tr].absolute_address.lba.lba3<<8)+
		    	toc->descs[tr].absolute_address.lba.lba4;
		    cd_toc->cdt_type = MACH_CDROM_LBA;
		  }

		  cd_toc++; /* next Toc entry */
		  et_count++;
		  /* if not enough space for the next structure, stop now */
		  next_size = (((sizeof(cd_toc_t)  * (et_count + 1)) + 3) >> 2);
		  if (next_size > reserved_count)
		    break;  /* leave the loop */
		}
		*status_count = (((sizeof(cd_toc_t)  * et_count) + 3) >> 2);
		if (*status_count > reserved_count)
		  *status_count = reserved_count;

	      }
	    }
	  }

	break;

	case MACH_SCMD_READ_SUBCHNL_MSF:
	case MACH_SCMD_READ_SUBCHNL_LBA:

	  /* "Get Position MSF|ABS" */
	  if_standard(current_position) {
	    cd_subchnl_t *subchnl = (cd_subchnl_t *)status;
    
	    if (*status_count < ((sizeof(cd_subchnl_t) + 3) >> 2)) {
	      rc = SCSI_RET_ABORTED;
	      ret = D_INVALID_SIZE;
	    }
	    else {
	      rc = scsi_read_subchannel(tgt,
				(flavor == MACH_SCMD_READ_SUBCHNL_MSF ? TRUE : FALSE),
				SCSI_CMD_RS_FMT_CURPOS, 0, ior);

	      if (rc == SCSI_RET_SUCCESS) {
		cdrom_chan_curpos_t	*st;
		st = (cdrom_chan_curpos_t *)tgt->cmd_ptr;

		subchnl->cdsc_track = st->subQ.trackno;
		subchnl->cdsc_index = st->subQ.indexno;
		subchnl->cdsc_format = st->subQ.format;
		subchnl->cdsc_adr = st->subQ.adr;
		subchnl->cdsc_ctrl = st->subQ.control;
		subchnl->cdsc_audiostatus = st->audio_status;

		if (flavor == MACH_SCMD_READ_SUBCHNL_MSF) {
		  subchnl->cdsc_absolute_addr.msf.minute = 
		  	st->subQ.absolute_address.msf.minute;
		  subchnl->cdsc_absolute_addr.msf.second = 
		  	st->subQ.absolute_address.msf.second;
		  subchnl->cdsc_absolute_addr.msf.frame = 
		  	st->subQ.absolute_address.msf.frame;
		  subchnl->cdsc_relative_addr.msf.minute = 
		  	st->subQ.relative_address.msf.minute;
		  subchnl->cdsc_relative_addr.msf.second = 
		  	st->subQ.relative_address.msf.second;
		  subchnl->cdsc_relative_addr.msf.frame = 
		  	st->subQ.relative_address.msf.frame;

		  subchnl->cdsc_type = MACH_CDROM_MSF;
		}
		else {	/* flavor == MACH_SCMD_READ_SUBCHNL_LBA */
		  subchnl->cdsc_absolute_addr.lba = 
		  	(st->subQ.absolute_address.lba.lba1<<24)+
			(st->subQ.absolute_address.lba.lba2<<16)+
			(st->subQ.absolute_address.lba.lba3<< 8)+
			st->subQ.absolute_address.lba.lba4;
		  subchnl->cdsc_relative_addr.lba = 
			(st->subQ.relative_address.lba.lba1<<24)+
			(st->subQ.relative_address.lba.lba2<<16)+
			(st->subQ.relative_address.lba.lba3<< 8)+
			st->subQ.relative_address.lba.lba4;

		  subchnl->cdsc_type = MACH_CDROM_LBA;
		}
		*status_count = ((sizeof(cd_subchnl_t) + 3) >> 2);

	      }
	    }
	  }
	break;

	case MACH_CDROMVOLREAD:
		{
			cd_volctrl_t *volctrl;
			cdrom_audio_page_t	*aup;

			volctrl = (cd_volctrl_t *) status;

			if (*status_count != ((sizeof(cd_volctrl_t) + 3) >> 2)) {
				rc = SCSI_RET_ABORTED;
				ret = D_INVALID_SIZE;
			} else {
				rc = scsi_mode_sense(tgt,
						     SCSI_CD_AUDIO_PAGE,
						     sizeof(*aup),
						     ior);
				if (rc != SCSI_RET_SUCCESS) 
					break;
				aup = (cdrom_audio_page_t *) tgt->cmd_ptr;
				volctrl->channel0 = aup->vol0;
				volctrl->channel1 = aup->vol1;
				volctrl->channel2 = aup->vol2;
				volctrl->channel3 = aup->vol3;
			}
		}
		break;

	default:
		tgt->dev_ops = &scsi_devsw[SCSI_CDROM];
		ret = RZ_MACSD(get_status)(dev, flavor, status, status_count);
		goto done;
	break;
  }

  if (rc == SCSI_RET_SUCCESS)
    ret = D_SUCCESS;

  if (scsi_debug) {
    /* We are stateless, but.. */
    if (rc == SCSI_RET_NEED_SENSE) {
      zero_ior( ior );
      tgt->ior = ior;
      scsi_request_sense(tgt, ior, 0);
      iowait(ior);
      if (scsi_check_sense_data(tgt, (scsi_sense_data_t *)tgt->cmd_ptr)) {
	scsi_print_sense_data((scsi_sense_data_t *)tgt->cmd_ptr);
      }
    }
  }

done:
  io_req_free(ior);
  tgt->dev_ops = &scsi_devsw[SCSI_CDROM];
  return ret;
}

/*
 * cd_set_status routine.
 */
io_return_t
cd_set_status(
	dev_t			dev,
	dev_flavor_t		flavor,
	dev_status_t		status,
	mach_msg_type_number_t	status_count)
{
	target_info_t	*tgt;
	io_req_t	ior;
	io_return_t	ret = D_INVALID_OPERATION;
	scsi_ret_t	rc;

	tgt = scsi_softc[rzcontroller(dev)]->target[rzslave(dev)];

	if (scsi_debug)
		printf("cd_set_status: 0x%x 0x%x 0x%lx 0x%lx\n",
			dev, flavor, status, status_count);

	/*
	 * Switch temporary to audio ops.
	 */
	tgt->dev_ops = &scsi_audio;

	io_req_alloc(ior);		
	zero_ior( ior );
	
	switch (flavor) {
		case MACH_CDROMEJECT:
		  /* "Eject" */
		  /* too many dont support it. Sigh */
		  tgt->flags |= TGT_OPTIONAL_CMD;
		  (void) scsi_medium_removal( tgt, TRUE, ior);
		  tgt->flags &= ~TGT_OPTIONAL_CMD;

		  zero_ior( ior );

		  if_standard(eject)
		    {
		      rc = scsi_start_unit(tgt, SCSI_CMD_SS_EJECT, ior);
		    }
		break;

		case MACH_CDROMPLAYMSF:
		  /* "Play A startM startS startF endM endS endF" */
		  if_standard(play_msf)
		    {
		      cd_msf_t *msf_req;

		      msf_req = (cd_msf_t *) status;

		      if (status_count != ((sizeof(cd_msf_t) + 3) >> 2)) {
			rc = SCSI_RET_ABORTED;
			ret = D_INVALID_SIZE;
		      }
		      else
			rc = scsi_play_audio_msf(tgt,
						 msf_req->cdmsf_min0,
						 msf_req->cdmsf_sec0,
						 msf_req->cdmsf_frame0,
						 msf_req->cdmsf_min1,
						 msf_req->cdmsf_sec1,
						 msf_req->cdmsf_frame1,
						 ior);
		    }
		break;

		case MACH_CDROMPLAYTRKIND:
		  /* "Play TI startT startI endT endI" */
		  if_standard(play_ti)
		    {
		      cd_ti_t *ti_req;

		      ti_req = (cd_ti_t *) status;

		      if (status_count != ((sizeof(cd_ti_t) + 3) >> 2)) {
			rc = SCSI_RET_ABORTED;
			ret = D_INVALID_SIZE;
		      }
		      else
			rc = scsi_play_audio_track_index(tgt,
							 ti_req->cdti_track0,
							 ti_req->cdti_index0,
							 ti_req->cdti_track1,
							 ti_req->cdti_index1,
							 ior);
		    }
		break;

		case MACH_CDROMPAUSE:
		  /* "Pause" */
		  if_standard(pause_resume)
		    {
		      rc = scsi_pause_resume(tgt, TRUE, ior);
		    }
		break;

		case MACH_CDROMRESUME:
		  /* "Resume" */
		  if_standard(pause_resume)
		    {
		      rc = scsi_pause_resume(tgt, FALSE, ior);
		    }
		break;

		case MACH_CDROMSTART:
		  /* "Start" */
		  rc = scsi_start_unit(tgt, SCSI_CMD_SS_START, ior);
		break;

		case MACH_CDROMSTOP:
		  /* "Stop" */
		  rc = scsi_start_unit(tgt, SCSI_CMD_SS_START, ior);
		break;

		case MACH_CDROMVOLCTRL:
		/* "Set V chan0vol chan1vol chan2vol chan3vol" */
			if_standard(volume_control) {

				cd_volctrl_t *volctrl;
				int v0, v1, v2, v3, rc2;
				cdrom_audio_page_t	au, au_mask, *aup;

				volctrl = (cd_volctrl_t *) status;

				if (status_count != ((sizeof(cd_volctrl_t) + 3) >> 2)) {
					rc = SCSI_RET_ABORTED;
					ret = D_INVALID_SIZE;
				}
				else {
					rc = scsi_mode_sense(tgt,
							     SCSI_CD_AUDIO_PAGE,
							     sizeof(au),
							     ior);
					if (rc != SCSI_RET_SUCCESS) 
						break;
					aup = (cdrom_audio_page_t *) tgt->cmd_ptr;
					au = *aup;
					/* au.h.bdesc ... */
					au.vol0 = volctrl->channel0;
					au.vol1 = volctrl->channel1;
					au.vol2 = volctrl->channel2;
					au.vol3 = volctrl->channel3;
					au.imm = 1;
					au.aprv = 0;
					zero_ior( ior );

					/* Fetch 'mask' page so drive will tell us which */
					/* fields can be modified.                       */
					rc = scsi_mode_sense(tgt,
							     SCSI_CD_AUDIO_PAGE|0x40,
							     sizeof(au),
							     ior);
					if (rc != SCSI_RET_SUCCESS) 
						break;
					aup = (cdrom_audio_page_t *) tgt->cmd_ptr;
					au_mask = *aup;
					au.vol0 &= au_mask.vol0;
					au.vol1 &= au_mask.vol1;
					au.vol2 &= au_mask.vol2;
					au.vol3 &= au_mask.vol3;
					au.h.data_len = 0;

					zero_ior( ior );
					rc = scsi2_mode_select(tgt, FALSE, (unsigned char *)&au,
							       sizeof(au), ior);
				}
			}
		break;

		default:
		  tgt->dev_ops = &scsi_devsw[SCSI_CDROM];
		  rc = RZ_MACSD(set_status)(dev, flavor, status, status_count);
		break;
	}

	if (rc == SCSI_RET_SUCCESS)
	  ret = D_SUCCESS;

	if (scsi_debug) {
	  /* We are stateless, but.. */
	  if (rc == SCSI_RET_NEED_SENSE) {
	    zero_ior( ior );
	    tgt->ior = ior;
	    scsi_request_sense(tgt, ior, 0);
	    iowait(ior);
	    if (scsi_check_sense_data(tgt, (scsi_sense_data_t *)tgt->cmd_ptr)) {
	      scsi_print_sense_data((scsi_sense_data_t *)tgt->cmd_ptr);
	    }
	  }
	}
	io_req_free(ior);
	tgt->dev_ops = &scsi_devsw[SCSI_CDROM];
	return ret;
}


static	char st_invalid [] = "Drive would not say";
static	char st_playing [] = "Playing";
static	char st_paused  [] = "Suspended";
static	char st_complete[] = "Done playing";
static	char st_error   [] = "Stopped in error";
static	char st_nothing [] = "Idle";

/* some vendor specific use this instead */
static unsigned char 
decode_status_1(
	unsigned char	audio_status)
{
  unsigned char result_status;

  switch (audio_status) {
  case 0:	result_status = MACH_CDROM_AUDIO_PLAY; break;
  case 1:
  case 2:	result_status = MACH_CDROM_AUDIO_PAUSED; break;
  case 3:	result_status = MACH_CDROM_AUDIO_COMPLETED; break;
  default:	result_status = MACH_CDROM_AUDIO_NO_STATUS; break;
  }
  return result_status;
}

static void
curse_the_vendor(
	red_list_t	*list,
	boolean_t	not_really)
{
	if (not_really) return;

	printf("%s\n%s\n%s\n%s\n",
		"The CDROM you use is not fully SCSI-2 compliant.",
		"We invite You to contact Your vendor and ask",
		"that they provide You with a firmware upgrade.",
		"Here is a list of some known deficiencies");

	printf("Vendor: %s Product: %s.. Revision: %s..\n",
		list->vendor, list->product, list->rev);

#define check(x,y,z) \
	if (list->x) printf("Command code x%x %s not supported\n", y, z);

	check(can_play_audio, SCSI_CMD_PLAY_AUDIO, "PLAY_AUDIO");
	check(eject, SCSI_CMD_START_STOP_UNIT,
		"START_STOP_UNIT, flag EJECT(0x2) in byte 5");
	check(current_position, SCSI_CMD_READ_SUBCH, "READ_SUBCHANNEL");
	check(read_toc, SCSI_CMD_READ_TOC, "READ_TOC");
/*	check(get_status, ...); duplicate of current_position */
	check(play_msf, SCSI_CMD_PLAY_AUDIO_MSF, "PLAY_AUDIO_MSF");
	check(play_ti, SCSI_CMD_PLAY_AUDIO_TI, "PLAY_AUDIO_TRACK_INDEX");
	check(pause_resume, SCSI_CMD_PAUSE_RESUME, "PAUSE_RESUME");
	check(volume_control, SCSI_CMD_MODE_SELECT,
		"MODE_SELECT, AUDIO page(0xe)");

#undef	check
	printf("Will work around these problems...\n");
}

/*
 * Ancillaries
 */
static void
zero_ior(
	io_req_t	ior )
{
	ior->io_next = ior->io_prev = 0;
	ior->io_count = 0;
	ior->io_op = IO_INTERNAL;
	ior->io_error = 0;
}

void
cd_strategy(
	register io_req_t	ior)
{
	rz_simpleq_strategy( ior, cd_start);
}

void
cd_start(
	target_info_t	*tgt,
	boolean_t	done)
{
	io_req_t	ior;

	ior = tgt->ior;
	if (done && ior) {
		tgt->ior = 0;
		iodone(ior);
		return;
	}
	panic("cd start"); /* uhu? */
}


/*
 * When the hardware cannot
 */
/*ARGSUSED*/
static scsi_ret_t
get_op_not_supported(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	*status_count,
	io_req_t	ior)
{
	/*
	 * The command is not implemented, no way around it
	 */
	return D_INVALID_OPERATION;
}

static scsi_ret_t
set_op_not_supported(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	status_count,
	io_req_t	ior)
{
	/*
	 * The command is not implemented, no way around it
	 */
	return D_INVALID_OPERATION;
}

/****************************************/
/*	Vendor Specific Operations	*/
/****************************************/

	/*   DEC RRD42   */

#define SCSI_CMD_DEC_SET_ADDRESS_FORMAT		0xc0
#	define scsi_cmd_saf_fmt	scsi_cmd_xfer_len_2

#define SCSI_CMD_DEC_PLAYBACK_STATUS		0xc4
typedef struct {
    unsigned char	xxx;
    BITFIELD_2(unsigned char,
    	is_msf: 1,
	xxx1:   7);
    unsigned char	data_len1;
    unsigned char	data_len0;
    unsigned char	audio_status;
    BITFIELD_2(unsigned char,
        control	: 4,
	xxx2 : 4);
    cdrom_addr_t address;
    BITFIELD_2(unsigned char,
        chan0_select : 4,
	xxx3 : 4);
    unsigned char	chan0_volume;
    BITFIELD_2(unsigned char,
        chan1_select : 4,
	xxx4 : 4);
    unsigned char	chan1_volume;
    BITFIELD_2(unsigned char,
        chan2_select : 4,
	xxx5 : 4);
    unsigned char	chan2_volume;
    BITFIELD_2(unsigned char,
        chan3_select : 4,
	xxx6 : 4);
    unsigned char	chan3_volume;
} dec_playback_status_t;

#define SCSI_CMD_DEC_PLAYBACK_CONTROL		0xc9
typedef struct {
    unsigned char	xxx0;
    BITFIELD_2(unsigned char,
        fmt  : 1,
	xxx1 : 7);
    unsigned char	xxx[8];
    BITFIELD_2(unsigned char,
        chan0_select : 4,
	xxx3 : 4);
    unsigned char	chan0_volume;
    BITFIELD_2(unsigned char,
        chan1_select : 4,
	xxx4 : 4);
    unsigned char	chan1_volume;
    BITFIELD_2(unsigned char,
        chan2_select : 4,
	xxx5 : 4);
    unsigned char	chan2_volume;
    BITFIELD_2(unsigned char,
        chan3_select : 4,
	xxx6 : 4);
    unsigned char	chan3_volume;
} dec_playback_control_t;


#if	0

static scsi_ret_t
rrd42_status(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	*status_count,
	io_req_t	ior)
{
	scsi_ret_t		rc;
	scsi_command_group_2	c;
	io_return_t		ret = SCSI_RET_SUCCESS;
	dec_playback_status_t	*st;
	cd_subchnl_t *subchnl = (cd_subchnl_t *)status;

	/* We might have to specify addressing fmt */
	if ((flavor == MACH_SCMD_READ_SUBCHNL_MSF) ||
	    (flavor == MACH_SCMD_READ_SUBCHNL_LBA)) {
		scsi_command_group_2 saf;

		bzero((char *)&saf, sizeof(saf));
		saf.scsi_cmd_code = SCSI_CMD_DEC_SET_ADDRESS_FORMAT;
		saf.scsi_cmd_saf_fmt = (flavor == MACH_SCMD_READ_SUBCHNL_LBA) ? 0 : 1;

		rc = cdrom_vendor_specific(tgt, &saf, 0, 0, 0, ior);

		if (rc != SCSI_RET_SUCCESS) return rc;

		zero_ior( ior );
	}

	bzero((char *)&c, sizeof(c));
	c.scsi_cmd_code = SCSI_CMD_DEC_PLAYBACK_STATUS;
	c.scsi_cmd_xfer_len_2 = sizeof(*st);
	rc = cdrom_vendor_specific(tgt, &c, 0, 0, sizeof(*st), ior);

	if (rc != SCSI_RET_SUCCESS) return rc;

	st = (dec_playback_status_t *) tgt->cmd_ptr;

	if (*status_count < sizeof(cd_subchnl_t)/sizeof (natural_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;
	}
	else {
	  subchnl->cdsc_track = 0;
	  subchnl->cdsc_index = 0;
	  subchnl->cdsc_format = 0;
	  subchnl->cdsc_adr = 0;
	  subchnl->cdsc_ctrl = 0;
	  subchnl->cdsc_audiostatus = st->audio_status+0x11;

	  if (flavor == MACH_SCMD_READ_SUBCHNL_MSF) {
	    subchnl->cdsc_absolute_addr.msf.minute = 
	      st->address.msf.minute;
	    subchnl->cdsc_absolute_addr.msf.second = 
	      st->address.msf.second;
	    subchnl->cdsc_absolute_addr.msf.frame = 
	      st->address.msf.frame;
	    subchnl->cdsc_relative_addr.msf.minute = 0;
	    subchnl->cdsc_relative_addr.msf.second = 0;
	    subchnl->cdsc_relative_addr.msf.frame = 0;

	    subchnl->cdsc_type = MACH_CDROM_MSF;
	  }
	  else {	/* flavor == MACH_SCMD_READ_SUBCHNL_LBA */
	    subchnl->cdsc_absolute_addr.lba = 
	      (st->address.lba.lba1<<24)+
	      (st->address.lba.lba2<<16)+
	      (st->address.lba.lba3<< 8)+
	      st->address.lba.lba4;
	    subchnl->cdsc_relative_addr.lba = 0;

	    subchnl->cdsc_type = MACH_CDROM_LBA;
		}
	  *status_count = (sizeof(cd_subchnl_t)/sizeof(natural_t));
	}

	return ret;
}
#endif

static scsi_ret_t
rrd42_set_volume(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;
	dec_playback_control_t		req;
	io_return_t		ret = SCSI_RET_SUCCESS;
	cd_volctrl_t *volctrl;

	volctrl = (cd_volctrl_t *) status;

	if (status_count != sizeof(cd_volctrl_t)) {
	  ret = D_INVALID_SIZE;

	  return ret;
	}
	else {
	  bzero((char *)&c, sizeof(c));
	  c.scsi_cmd_code = SCSI_CMD_DEC_PLAYBACK_CONTROL;
	  c.scsi_cmd_xfer_len_2 = sizeof(req);
	  bzero((char *)&req, sizeof(req));
	  if (volctrl->channel0) {
	    req.chan0_select = 1;
	    req.chan0_volume = volctrl->channel0;
	  }
	  if (volctrl->channel1) {
	    req.chan1_select = 2;
	    req.chan1_volume = volctrl->channel1;
	  }
	  if (volctrl->channel2) {
	    req.chan2_select = 4;
	    req.chan2_volume = volctrl->channel2;
	  }
	  if (volctrl->channel3) {
	    req.chan3_select = 8;
	    req.chan3_volume = volctrl->channel3;
	  }
	  return cdrom_vendor_specific(tgt, &c,
				       (unsigned char *)&req, sizeof(req), 0, ior);
	}
}

	/* NEC CD-ROM */

#define SCSI_CMD_NEC_READ_TOC           	0xde
typedef struct {
    unsigned char	xxx[9];
    unsigned char	first_track;
    unsigned char	xxx1[9];
    unsigned char	last_track;
    unsigned char	xxx2[9];
    unsigned char	lead_out_addr[3];
    struct {
	BITFIELD_2(unsigned char,
	    adr  : 4,
	    ctrl : 4);
	unsigned char	xxx3[6];
	unsigned char	address[3];
    } track_info[1]; /* VARSIZE */
} nec_toc_data_t;

#define SCSI_CMD_NEC_SEEK_TRK			0xd8
#define SCSI_CMD_NEC_PLAY_AUDIO			0xd9
#define SCSI_CMD_NEC_PAUSE			0xda
#define	SCSI_CMD_NEC_EJECT			0xdc

#define SCSI_CMD_NEC_READ_SUBCH_Q		0xdd
typedef struct {
    unsigned char	audio_status;	/* see decode_status_1 */
    BITFIELD_2(unsigned char,
        ctrl : 4,
	xxx1 : 4);
    unsigned char	trackno;
    unsigned char	indexno;
    unsigned char	relative_address[3];
    unsigned char	absolute_address[3];
} nec_subch_data_t;

/*
 * Reserved bits in byte1
 */
#define NEC_LR_PLAY_MODE	0x01	/* RelAdr bit overload */
#define	NEC_LR_STEREO		0x02	/* mono/stereo */

/*
 * Vendor specific bits in the control byte.
 * NEC uses them to specify the addressing mode
 */
#define NEC_CTRL_A_ABS		0x00	/* XXX not sure about this */
#define NEC_CTRL_A_MSF		0x40	/* min/sec/frame */
#define NEC_CTRL_A_TI		0x80	/* track/index */
#define NEC_CTRL_A_CURRENT	0xc0	/* same as last specified */

/*ARGSUSED*/
static scsi_ret_t
nec_eject(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;

	bzero((char *)&c, sizeof(c));
	c.scsi_cmd_code = SCSI_CMD_NEC_EJECT;

	return cdrom_vendor_specific(tgt, &c, 0, 0, 0, ior);
}

static scsi_ret_t
nec_subchannel(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	*status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;
	nec_subch_data_t	*st;
	io_return_t		ret = SCSI_RET_SUCCESS;
	scsi_ret_t		rc;
	cd_subchnl_t *subchnl = (cd_subchnl_t *)status;

	bzero((char *)&c, sizeof(c));
	c.scsi_cmd_code = SCSI_CMD_NEC_READ_SUBCH_Q;
	c.scsi_cmd_lun_and_relbit = sizeof(*st);	/* Sic! */

	rc = cdrom_vendor_specific(tgt, &c, 0, 0, sizeof(*st), ior);
	if (rc != SCSI_RET_SUCCESS) return rc;
	
	st = (nec_subch_data_t *) tgt->cmd_ptr;

	if (*status_count < sizeof(cd_subchnl_t)/sizeof (natural_t)) {
	  ret =  D_INVALID_SIZE;
	}
	else {
	  subchnl->cdsc_track = st->trackno;
	  subchnl->cdsc_index = st->indexno;
	  subchnl->cdsc_format = 0;
	  subchnl->cdsc_adr = 0;
	  subchnl->cdsc_ctrl = 0;
	  subchnl->cdsc_audiostatus = decode_status_1(st->audio_status);

	  if (flavor == MACH_SCMD_READ_SUBCHNL_MSF) {
	    subchnl->cdsc_absolute_addr.msf.minute = 
	      bcd_to_decimal(st->absolute_address[0]);
	    subchnl->cdsc_absolute_addr.msf.second = 
	      bcd_to_decimal(st->absolute_address[1]);
	    subchnl->cdsc_absolute_addr.msf.frame = 
	      bcd_to_decimal(st->absolute_address[2]);
	    subchnl->cdsc_relative_addr.msf.minute = 
	      bcd_to_decimal(st->relative_address[0]);
	    subchnl->cdsc_relative_addr.msf.second = 
	      bcd_to_decimal(st->relative_address[1]);
	    subchnl->cdsc_relative_addr.msf.frame = 
	      bcd_to_decimal(st->relative_address[2]);

	    subchnl->cdsc_type = MACH_CDROM_MSF;
	  }
	  else {	/* flavor == MACH_SCMD_READ_SUBCHNL_LBA */
	    /* XXX can it do ABS addressing e.g. 'logical' ? */
	    subchnl->cdsc_type = MACH_CDROM_LBA;

	    ret = D_INVALID_OPERATION;
	  }

	  *status_count = (sizeof(cd_subchnl_t)/sizeof(natural_t));
	}

	return ret;
}

static scsi_ret_t
nec_read_toc(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	*status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;
	nec_toc_data_t		*t;
	scsi_ret_t		rc;
	io_return_t		ret = SCSI_RET_SUCCESS;
	int			first, last;

	bzero((char *)&c, sizeof(c));
	c.scsi_cmd_code = SCSI_CMD_NEC_READ_TOC;
	c.scsi_cmd_lun_and_relbit = NEC_LR_PLAY_MODE|NEC_LR_STEREO;

	rc = cdrom_vendor_specific(tgt, &c, 0, 0, 512/*XXX*/, ior);
	if (rc != SCSI_RET_SUCCESS) return rc;
	
	t = (nec_toc_data_t *) tgt->cmd_ptr;

	first = bcd_to_decimal(t->first_track);
	last  = bcd_to_decimal(t->last_track);

	/*
	 * "Get TH" wants summary, "TOC MSF|ABS from_track" wants all
	 */
	if (flavor == MACH_SCMD_READ_HEADER) {
	  cd_toc_header_t *toc_header = (cd_toc_header_t *)status;

	  if (*status_count < sizeof(cd_toc_header_t)/sizeof (natural_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    goto out;
	  }
	  else {
	    toc_header->len = sizeof(*t) + sizeof(t->track_info) * 
	      (last - first - 1);
	    toc_header->first_track = first;
	    toc_header->last_track = last;
	    *status_count = sizeof(cd_toc_header_t)/sizeof(natural_t);

	    goto out;
	  }
	}

	/*
	 * The whole shebang
	 */
	if ((flavor == MACH_SCMD_READ_TOC_MSF) || (flavor == MACH_SCMD_READ_TOC_LBA)) {
	  cd_toc_t *cd_toc = (cd_toc_t *)status;
	  int tr = 1; /* starting track */
	  int tlen, et_count = 0;
	  int reserved_count = *status_count;

	  if (*status_count < sizeof(cd_toc_t)/sizeof(natural_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;
	  }
	  else {
	    cdrom_toc_t *toc = (cdrom_toc_t *)tgt->cmd_ptr;

	    last -= first;
	    tr -= first;
	    while ((tr >= 0) && (tr <= last)) {
	      cd_toc->cdt_ctrl = t->track_info[tr].ctrl;
	      cd_toc->cdt_adr = t->track_info[tr].adr;
	      cd_toc->cdt_track = first + tr;

	      if (flavor == MACH_SCMD_READ_TOC_MSF) {
		cd_toc->cdt_addr.msf.minute = 
		  bcd_to_decimal(t->track_info[tr].address[0]);
		cd_toc->cdt_addr.msf.second =
		  bcd_to_decimal(t->track_info[tr].address[1]);
		cd_toc->cdt_addr.msf.frame = 
		  bcd_to_decimal(t->track_info[tr].address[2]);
		cd_toc->cdt_type = MACH_CDROM_MSF;
	      }
	      else {	/* flavor == MACH_SCMD_READ_TOC_LBA */
/* THIS IS WRONG */
		cd_toc->cdt_addr.lba = 
		  bcd_to_decimal(t->track_info[tr].address[0]) * 10000 +
		  bcd_to_decimal(t->track_info[tr].address[1]) * 100 +
		  bcd_to_decimal(t->track_info[tr].address[2]);
		cd_toc->cdt_type = MACH_CDROM_LBA;
	      }

	      cd_toc++; /* next Toc entry */
	      et_count++;
	      tr++;
	    }

	    /* To know how long the last track is */
	    cd_toc->cdt_ctrl = 0;	/* User expects this */
	    cd_toc->cdt_adr = 1;	/* User expects this */
	    cd_toc->cdt_track = 0xaa;	/* User expects this */

	    if (flavor == MACH_SCMD_READ_TOC_MSF) {
	      cd_toc->cdt_addr.msf.minute = 
		bcd_to_decimal(t->lead_out_addr[0]);
	      cd_toc->cdt_addr.msf.second =
		bcd_to_decimal(t->lead_out_addr[1]);
		cd_toc->cdt_addr.msf.frame = 
		bcd_to_decimal(t->lead_out_addr[2]);
	      cd_toc->cdt_type = MACH_CDROM_MSF;
	    }
	    else {	/* flavor == MACH_SCMD_READ_TOC_LBA */
/* THIS IS WRONG */
	      cd_toc->cdt_addr.lba = 
		bcd_to_decimal(t->lead_out_addr[0]) * 10000 +
		bcd_to_decimal(t->lead_out_addr[1]) * 100 +
		bcd_to_decimal(t->lead_out_addr[2]);
	      cd_toc->cdt_type = MACH_CDROM_LBA;
	      et_count++;

	      *status_count = (sizeof(cd_toc_t)/sizeof(natural_t)) * et_count;
	      if (*status_count > reserved_count)
		*status_count = reserved_count;
	    }
	  }
	}
out:
	return ret;
}


static scsi_ret_t
nec_play(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;
	int			sm, ss, sf, em, es, ef;
	int			st, si, et, ei;
	scsi_ret_t		rc;
	io_return_t		ret = SCSI_RET_SUCCESS;

	/*
	 * Seek to desired position
	 */
	bzero((char *)&c, sizeof(c));
	c.scsi_cmd_code = SCSI_CMD_NEC_SEEK_TRK;
	c.scsi_cmd_lun_and_relbit = NEC_LR_PLAY_MODE;

	/*
	 * Play_msf or Play_ti
	 */
	if (flavor == MACH_CDROMPLAYMSF) {
		/* "Play A startM startS startF endM endS endF" */
	  cd_msf_t *msf_req;

	  msf_req = (cd_msf_t *) status;

	  if (status_count != sizeof(cd_msf_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    return ret;
	  }
	  else {
	    c.scsi_cmd_lba1 = decimal_to_bcd(msf_req->cdmsf_min0);
	    c.scsi_cmd_lba2 = decimal_to_bcd(msf_req->cdmsf_sec0);
	    c.scsi_cmd_lba3 = decimal_to_bcd(msf_req->cdmsf_frame0);
	    c.scsi_cmd_ctrl_byte = NEC_CTRL_A_MSF;
	  }
	}
	else /* flavor == MACH_CDROMPLAYTRKIND */ {
	  /* "Play TI startT startI endT endI" */
	  cd_ti_t *ti_req;

	  ti_req = (cd_ti_t *) status;

	  if (status_count != sizeof(cd_ti_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    return ret;
	  }
	  else {
	    c.scsi_cmd_lba1 = decimal_to_bcd(ti_req->cdti_track0);
	    c.scsi_cmd_lba2 = decimal_to_bcd(ti_req->cdti_index0);
	    c.scsi_cmd_lba3 = 0;
	    c.scsi_cmd_ctrl_byte = NEC_CTRL_A_TI;
	  }
	}

	rc = cdrom_vendor_specific(tgt, &c, 0, 0, 0, ior);
	if (rc != SCSI_RET_SUCCESS) return rc;
	
	/*
	 * Now ask it to play until..
	 */
	zero_ior( ior );

	bzero((char *)&c, sizeof(c));
	c.scsi_cmd_code = SCSI_CMD_NEC_PLAY_AUDIO;
	c.scsi_cmd_lun_and_relbit = NEC_LR_PLAY_MODE|NEC_LR_STEREO;

	if (flavor == MACH_CDROMPLAYMSF) {
		/* "Play A startM startS startF endM endS endF" */
	  cd_msf_t *msf_req;

	  msf_req = (cd_msf_t *) status;

	  if (status_count != sizeof(cd_msf_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    return ret;
	  }
	  else {
	    c.scsi_cmd_lba1 = decimal_to_bcd(msf_req->cdmsf_min1);
	    c.scsi_cmd_lba2 = decimal_to_bcd(msf_req->cdmsf_sec1);
	    c.scsi_cmd_lba3 = decimal_to_bcd(msf_req->cdmsf_frame1);
	    c.scsi_cmd_ctrl_byte = NEC_CTRL_A_MSF;
	  }
	}
	else /* flavor == MACH_CDROMPLAYTRKIND */ {
	  /* "Play TI startT startI endT endI" */
	  cd_ti_t *ti_req;

	  ti_req = (cd_ti_t *) status;

	  if (status_count != sizeof(cd_ti_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    return ret;
	  }
	  else {
	    c.scsi_cmd_lba1 = decimal_to_bcd(ti_req->cdti_track1);
	    c.scsi_cmd_lba2 = decimal_to_bcd(ti_req->cdti_index1);
	    c.scsi_cmd_lba3 = 0;
	    c.scsi_cmd_ctrl_byte = NEC_CTRL_A_TI;
	  }
	}

	return cdrom_vendor_specific(tgt, &c, 0, 0, 0, ior);
}

static scsi_ret_t
nec_pause_resume(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;

	bzero((char *)&c, sizeof(c));
	/*
	 * "Resume" or "Stop"
	 */
	if (flavor == MACH_CDROMRESUME) {
	        c.scsi_cmd_code = SCSI_CMD_NEC_PLAY_AUDIO;
	        c.scsi_cmd_lun_and_relbit = NEC_LR_PLAY_MODE|NEC_LR_STEREO;
	        c.scsi_cmd_ctrl_byte = NEC_CTRL_A_CURRENT;
	} else /* flavor == MACH_CDROMPAUSE */ {
	        c.scsi_cmd_code = SCSI_CMD_NEC_PAUSE;
	}

	return cdrom_vendor_specific(tgt, &c, 0, 0, 0, ior);
}

#if	0
	/* I have info on these drives, but no drive to test */

	/* PIONEER DRM-600 */

#define SCSI_CMD_PIONEER_EJECT			0xc0

#define SCSI_CMD_PIONEER_READ_TOC		0xc1
typedef struct {
	unsigned char	first_track;
	unsigned char	last_track;
	unsigned char	xxx[2];
} pioneer_toc_hdr_t;
typedef struct {
	unsigned char	ctrl;
	unsigned char	address[3];
} pioneer_toc_info_t;

#define SCSI_CMD_PIONEER_READ_SUBCH		0xc2
typedef struct {
    BITFIELD_2(unsigned char,
        ctrl : 4,
	xxx1 : 4);
    unsigned char	trackno;
    unsigned char	indexno;
    unsigned char	relative_address[3];
    unsigned char	absolute_address[3];
} pioneer_subch_data_t;

#define SCSI_CMD_PIONEER_SEEK_TRK		0xc8
#define SCSI_CMD_PIONEER_PLAY_AUDIO		0xc9
#define SCSI_CMD_PIONEER_PAUSE			0xca

#define SCSI_CMD_PIONEER_AUDIO_STATUS		0xcc
typedef struct {
	unsigned char	audio_status;
	unsigned char	xxx[5];
} pioneer_status_t;

/*
 * Reserved bits in byte1
 */
#define PIONEER_LR_END_ADDR	0x10
#define PIONEER_LR_PAUSE	0x10
#define PIONEER_LR_RESUME	0x00

/*
 * Vendor specific bits in the control byte.
 */
#define PIONEER_CTRL_TH		0x00	/* TOC header */
#define PIONEER_CTRL_TE		0x80	/* one TOC entry */
#define PIONEER_CTRL_LO		0x40	/* lead-out track info */

#define PIONEER_CTRL_A_MSF	0x40	/* min/sec/frame addr */

static scsi_ret_t
pioneer_eject(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;

	bzero((char *)&c, sizeof(c));
        c.scsi_cmd_code = SCSI_CMD_PIONEER_EJECT;

	return cdrom_vendor_specific(tgt, &c, 0, 0, 0, ior);
}

static scsi_ret_t
pioneer_position(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	*status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;
	scsi_ret_t		rc;
	pioneer_subch_data_t	*st;
	io_return_t		ret = SCSI_RET_SUCCESS;
	cd_subchnl_t		*subchnl = (cd_subchnl_t *)status;
	unsigned char		audio_stat;

	bzero((char *)&c, sizeof(c));
        c.scsi_cmd_code = SCSI_CMD_PIONEER_READ_SUBCH;
        c.scsi_cmd_xfer_len_2 = sizeof(pioneer_subch_data_t); /* 9 bytes */

	rc = cdrom_vendor_specific(tgt, &c, 0, 0, sizeof(pioneer_subch_data_t), ior);
	if (rc != SCSI_RET_SUCCESS) return rc;

	st = (pioneer_subch_data_t *) tgt->cmd_ptr;

	if (*status_count < sizeof(cd_subchnl_t)/sizeof (natural_t)) {
	  ret =  D_INVALID_SIZE;
	}
	else {
	  subchnl->cdsc_track = st->trackno;
	  subchnl->cdsc_index = st->indexno;
	  subchnl->cdsc_format = 0;
	  subchnl->cdsc_adr = 0;
	  subchnl->cdsc_ctrl = st->ctrl;
	  rc = pioneer_status(tgt, &audio_stat, ior);
	  if (rc == SCSI_RET_SUCCESS)
	    subchnl->cdsc_audiostatus = audio_stat;
	  else
	    subchnl->cdsc_audiostatus = MACH_CDROM_AUDIO_NO_STATUS;

	  if (flavor == MACH_SCMD_READ_SUBCHNL_MSF) {
	    subchnl->cdsc_absolute_addr.msf.minute = 
	      bcd_to_decimal(st->absolute_address[0]);
	    subchnl->cdsc_absolute_addr.msf.second = 
	      bcd_to_decimal(st->absolute_address[1]);
	    subchnl->cdsc_absolute_addr.msf.frame = 
	      bcd_to_decimal(st->absolute_address[2]);
	    subchnl->cdsc_relative_addr.msf.minute = 
	      bcd_to_decimal(st->relative_address[0]);
	    subchnl->cdsc_relative_addr.msf.second = 
	      bcd_to_decimal(st->relative_address[1]);
	    subchnl->cdsc_relative_addr.msf.frame = 
	      bcd_to_decimal(st->relative_address[2]);

	    subchnl->cdsc_type = MACH_CDROM_MSF;
	  }
	  else {	/* flavor == MACH_SCMD_READ_SUBCHNL_LBA */
	    /* XXX can it do ABS addressing e.g. 'logical' ? */
	    subchnl->cdsc_type = MACH_CDROM_LBA;

	    ret = D_INVALID_OPERATION;
	  }

	  *status_count = (sizeof(cd_subchnl_t)/sizeof(natural_t));
	}

	return ret;
}

static scsi_ret_t
pioneer_toc(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	*status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;
	pioneer_toc_hdr_t	*th;
	pioneer_toc_info_t	*t;
	scsi_ret_t		rc;
	io_return_t		ret = SCSI_RET_SUCCESS;
	int			first, last;

	/* Read header first */
	bzero((char *)&c, sizeof(c));
        c.scsi_cmd_code = SCSI_CMD_PIONEER_READ_TOC;
        c.scsi_cmd_xfer_len_2 = sizeof(pioneer_toc_hdr_t);
        c.scsi_cmd_ctrl_byte = PIONEER_CTRL_TH;

	rc = cdrom_vendor_specific(tgt, &c, 0, 0, sizeof(pioneer_toc_hdr_t), ior);
	if (rc != SCSI_RET_SUCCESS) return rc;

	th = (pioneer_toc_hdr_t *)tgt->cmd_ptr;
	first = bcd_to_decimal(th->first_track);
	last = bcd_to_decimal(th->last_track);

	/*
	 * "Get TH" wants summary, "TOC MSF|ABS from_track" wants all
	 */
	if (flavor == MACH_SCMD_READ_HEADER) {
	  cd_toc_header_t *toc_header = (cd_toc_header_t *)status;

	  if (*status_count < sizeof(cd_toc_header_t)/sizeof (natural_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    goto out;
	  }
	  else {
	    toc_header->len = 0;
	    toc_header->first_track = first;
	    toc_header->last_track = last;
	    *status_count = sizeof(cd_toc_header_t)/sizeof(natural_t);

	    goto out;
	  }
	}

	/*
	 * Must do it one track at a time
	 */
	if ((flavor == MACH_SCMD_READ_TOC_MSF) || (flavor == MACH_SCMD_READ_TOC_LBA)) {
	  cd_toc_t *cd_toc = (cd_toc_t *)status;
	  int tr = 1; /* starting track */
	  int tlen, et_count = 0;
	  int reserved_count = *status_count;

	  if (*status_count < sizeof(cd_toc_t)/sizeof(natural_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;
	  }
	  else {
	    cdrom_toc_t *toc = (cdrom_toc_t *)tgt->cmd_ptr;

	    for ( ; tr <= last; tr++) {
	      zero_ior(ior);
	      bzero((char *)&c, sizeof(c));
	      c.scsi_cmd_code = SCSI_CMD_PIONEER_READ_TOC;
	      c.scsi_cmd_lba4 = decimal_to_bcd(tr);
	      c.scsi_cmd_xfer_len_2 = sizeof(pioneer_toc_info_t);
	      c.scsi_cmd_ctrl_byte = PIONEER_CTRL_TE;

	      rc = cdrom_vendor_specific(tgt, &c, 0, 0, sizeof(pioneer_toc_info_t),
					 ior);
	      if (rc != SCSI_RET_SUCCESS) break;

	      t = (pioneer_toc_info_t *)tgt->cmd_ptr;

	      cd_toc->cdt_ctrl = t->ctrl;
	      cd_toc->cdt_adr = 0;
	      cd_toc->cdt_track = tr;

	      if (flavor == MACH_SCMD_READ_TOC_MSF) {
		cd_toc->cdt_addr.msf.minute = 
		  bcd_to_decimal(t->address[0]);
		cd_toc->cdt_addr.msf.second =
		  bcd_to_decimal(t->address[1]);
		cd_toc->cdt_addr.msf.frame = 
		  bcd_to_decimal(t->address[2]);
		cd_toc->cdt_type = MACH_CDROM_MSF;
	      }
	      else {	/* flavor == MACH_SCMD_READ_TOC_LBA */
/* THIS IS WRONG */
		cd_toc->cdt_addr.lba = 
		  bcd_to_decimal(t->address[0]) * 10000 +
		  bcd_to_decimal(t->address[1]) * 100 +
		  bcd_to_decimal(t->address[2]);
		cd_toc->cdt_type = MACH_CDROM_LBA;
	      }

	      cd_toc++; /* next Toc entry */
	      et_count++;
	    }

	    /* To know how long the last track is */
	    zero_ior(ior);
	    bzero((char *)&c, sizeof(c));
	    c.scsi_cmd_code = SCSI_CMD_PIONEER_READ_TOC;
	    c.scsi_cmd_xfer_len_2 = sizeof(pioneer_toc_info_t);
	    c.scsi_cmd_ctrl_byte = PIONEER_CTRL_LO;

	    rc = cdrom_vendor_specific(tgt, &c, 0, 0, sizeof(pioneer_toc_info_t),
				       ior);
	    if (rc != SCSI_RET_SUCCESS) return rc;

	    cd_toc->cdt_ctrl = t->ctrl;	/* User expects this */
	    cd_toc->cdt_adr = 0;	/* User expects this */
	    cd_toc->cdt_track = 0xaa;	/* User expects this */

	    if (flavor == MACH_SCMD_READ_TOC_MSF) {
	      cd_toc->cdt_addr.msf.minute = 
		bcd_to_decimal(t->address[0]);
	      cd_toc->cdt_addr.msf.second =
		bcd_to_decimal(t->address[1]),
		cd_toc->cdt_addr.msf.frame = 
		bcd_to_decimal(t->address[2]);
	      cd_toc->cdt_type = MACH_CDROM_MSF;
	    }
	    else {	/* flavor == MACH_SCMD_READ_TOC_LBA */
/* THIS IS WRONG */
	      cd_toc->cdt_addr.lba = 
		bcd_to_decimal(t->address[0]) * 10000 +
		bcd_to_decimal(t->address[1]) * 100 +
		bcd_to_decimal(t->address[2]);
	      cd_toc->cdt_type = MACH_CDROM_LBA;
	      et_count++;

	      *status_count = (sizeof(cd_toc_t)/sizeof(natural_t)) * et_count;
	      if (*status_count > reserved_count)
		*status_count = reserved_count;
	    }
	  }
	}

out:
	return ret;
}

static scsi_ret_t
pioneer_status(
	target_info_t	*tgt,
	unsigned char 	*status,
	io_req_t	ior)
{
	scsi_command_group_2	c;
	pioneer_status_t	*st;
	scsi_ret_t		rc;

	bzero((char *)&c, sizeof(c));
        c.scsi_cmd_code = SCSI_CMD_PIONEER_AUDIO_STATUS;
        c.scsi_cmd_xfer_len_2 = sizeof(pioneer_status_t); /* 6 bytes */

	rc = cdrom_vendor_specific(tgt, &c, 0, 0, sizeof(pioneer_status_t), ior);
	if (rc != SCSI_RET_SUCCESS) return rc;

	st = (pioneer_status_t*) tgt->cmd_ptr;
	*status = decode_status_1(st->audio_status);

	return SCSI_RET_SUCCESS;
}

static scsi_ret_t
pioneer_play(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;
	int			sm, ss, sf, em, es, ef;
	int			st, si, et, ei;
	scsi_ret_t		rc;
	io_return_t		ret = SCSI_RET_SUCCESS;

	/*
	 * Seek to desired position
	 */
	bzero((char *)&c, sizeof(c));
        c.scsi_cmd_code = SCSI_CMD_PIONEER_SEEK_TRK;
	/*
	 * Play_msf or Play_ti
	 */
	if (flavor == MACH_CDROMPLAYMSF) {
		/* "Play A startM startS startF endM endS endF" */
	  cd_msf_t *msf_req;

	  msf_req = (cd_msf_t *) status;

	  if (status_count != sizeof(cd_msf_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    return ret;
	  }
	  else {
	    c.scsi_cmd_lba2 = decimal_to_bcd(msf_req->cdmsf_min0);
	    c.scsi_cmd_lba3 = decimal_to_bcd(msf_req->cdmsf_sec0);
	    c.scsi_cmd_lba4 = decimal_to_bcd(msf_req->cdmsf_frame0);
	    c.scsi_cmd_ctrl_byte = PIONEER_CTRL_A_MSF;
	  }
	}
	else /* flavor == MACH_CDROMPLAYTRKIND */ {
	  /* "Play TI startT startI endT endI" */
	  cd_ti_t *ti_req;

	  ti_req = (cd_ti_t *) status;

	  if (status_count != sizeof(cd_ti_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    return ret;
	  }
	  else {
	    c.scsi_cmd_lba3 = decimal_to_bcd(ti_req->cdti_track0);
	    c.scsi_cmd_lba4 = decimal_to_bcd(ti_req->cdti_index0);
	    c.scsi_cmd_ctrl_byte = 0x80;	/* Pure speculation!! */
	  }
	}

	rc = cdrom_vendor_specific(tgt, &c, 0, 0, 0, ior);
	if (rc != SCSI_RET_SUCCESS) return rc;
	
	/*
	 * Now ask it to play until..
	 */
	zero_ior( ior );

	bzero((char *)&c, sizeof(c));
	c.scsi_cmd_code = SCSI_CMD_PIONEER_PLAY_AUDIO;
	c.scsi_cmd_lun_and_relbit = PIONEER_LR_END_ADDR;

	if (flavor == MACH_CDROMPLAYMSF) {
		/* "Play A startM startS startF endM endS endF" */
	  cd_msf_t *msf_req;

	  msf_req = (cd_msf_t *) status;

	  if (status_count != sizeof(cd_msf_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    return ret;
	  }
	  else {
	    c.scsi_cmd_lba2 = decimal_to_bcd(msf_req->cdmsf_min1);
	    c.scsi_cmd_lba3 = decimal_to_bcd(msf_req->cdmsf_sec1);
	    c.scsi_cmd_lba4 = decimal_to_bcd(msf_req->cdmsf_frame1);
	    c.scsi_cmd_ctrl_byte = PIONEER_CTRL_A_MSF;
	  }
	}
	else /* flavor == MACH_CDROMPLAYTRKIND */ {
	  /* "Play TI startT startI endT endI" */
	  cd_ti_t *ti_req;

	  ti_req = (cd_ti_t *) status;

	  if (status_count != sizeof(cd_ti_t)) {
	    rc = SCSI_RET_ABORTED;
	    ret = D_INVALID_SIZE;

	    return ret;
	  }
	  else {
	    c.scsi_cmd_lba3 = decimal_to_bcd(ti_req->cdti_track1);
	    c.scsi_cmd_lba4 = decimal_to_bcd(ti_req->cdti_index1);
	    c.scsi_cmd_ctrl_byte = 0x80;	/* Pure speculation! */
	  }
	}

	return cdrom_vendor_specific(tgt, &c, 0, 0, 0, ior);
}

static scsi_ret_t
pioneer_pause_resume(
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	status_count,
	io_req_t	ior)
{
	scsi_command_group_2	c;

	bzero((char *)&c, sizeof(c));
        c.scsi_cmd_code = SCSI_CMD_PIONEER_PAUSE;
	/*
	 * "Resume" or "Stop"
	 */
	if (flavor == MACH_CDROMPAUSE)
		c.scsi_cmd_lun_and_relbit = PIONEER_LR_PAUSE;
	else /* falvor == MACH_CDROMRESUME */
		c.scsi_cmd_lun_and_relbit = PIONEER_LR_RESUME;

	return cdrom_vendor_specific(tgt, &c, 0, 0, 0, ior);
}

	/* DENON DRD-253 */

#define SCSI_CMD_DENON_PLAY_AUDIO		0x22
#define SCSI_CMD_DENON_EJECT			0xe6
#define SCSI_CMD_DENON_STOP_AUDIO		0xe7
#define SCSI_CMD_DENON_READ_TOC			0xe9
#define SCSI_CMD_DENON_READ_SUBCH		0xeb


	/* HITACHI 1750 */

#define SCSI_CMD_HITACHI_PLAY_AUDIO_MSF		0xe0
#define SCSI_CMD_HITACHI_PAUSE_AUDIO		0xe1
#define SCSI_CMD_HITACHI_EJECT			0xe4
#define SCSI_CMD_HITACHI_READ_SUBCH		0xe5
#define SCSI_CMD_HITACHI_READ_TOC		0xe8

#endif

/*
 * Tabulate all of the above
 */
static red_list_t	cdrom_exceptions[] = {

#if 0
	For documentation purposes, here are some SCSI-2 compliant drives:

	Vendor		Product			Rev	 Comments

	"SONY    "	"CD-ROMCDU-541   "	"2.6a"	 The NeXT drive
#endif

	/* vendor, product, rev */
	/* can_play_audio */
	/* eject */
	/* current_position */
	/* read_toc */
	/* get_status */
	/* play_msf */
	/* play_ti */
	/* pause_resume */
	/* volume_control */

	  /* We have seen a "RRD42(C)DEC     " "4.5d" */
	{ "DEC     ", "RRD42", "",
	  0, 0, 0, 0, 0, 0, 0, 0, rrd42_set_volume },

	  /* We have seen a "CD-ROM DRIVE:84 " "1.0 " */
	{ "NEC     ", "CD-ROM DRIVE:84", "",
	  get_op_not_supported, nec_eject, nec_subchannel, nec_read_toc,
	  nec_subchannel, nec_play, nec_play, nec_pause_resume,
	  set_op_not_supported },

#if  0
	{ "PIONEER ", "???????DRM-6", "",
	  get_op_not_supported, pioneer_eject, pioneer_position, pioneer_toc,
	  pioneer_position, pioneer_play, pioneer_play, pioneer_pause_resume,
	  set_op_not_supported },

	{ "DENON   ", "DRD 25X", "", ...},
	{ "HITACHI ", "CDR 1750S", "", ...},
	{ "HITACHI ", "CDR 1650S", "", ...},
	{ "HITACHI ", "CDR 3650", "", ...},

#endif

	/* Zero terminate this list */
	{ 0, }
};

static void
check_red_list(
	target_info_t		*tgt,
	scsi2_inquiry_data_t	*inq)

{
	red_list_t		*list;

	for (list = &cdrom_exceptions[0]; list->vendor; list++) {

		/*
		 * Prefix-Match all strings 
		 */
		if ((strncmp(list->vendor, (const char *)inq->vendor_id,
				strlen(list->vendor)) == 0) &&
		    (strncmp(list->product, (const char *)inq->product_id,
				strlen(list->product)) == 0) &&
		    (strncmp(list->rev, (const char *)inq->product_rev,
				 strlen(list->rev)) == 0)) {
			/*
			 * One of them..
			 */
			if (tgt->cdrom_info.violates_standards != list) {
			    tgt->cdrom_info.violates_standards = list;
			    curse_the_vendor( list, FALSE );
			}
			return;
		}
	}
}
