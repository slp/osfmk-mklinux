/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */

/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 */
/*
 * MkLinux
 */
/*
 * Common routines to handle CDROMs.
 */

#include <linux/autoconf.h>

#include <mach/mach_interface.h>
#include <mach/mach_ioctl.h>
#include <device/device_request.h>
#include <device/device.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/device_reply_hdlr.h>
#include <osfmach3/assert.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/block_dev.h>
#include <device/cdrom_status.h>

#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/cdrom.h>
#include <linux/mm.h>
#include <linux/malloc.h>

#define	MAX_MINORS	256

#include <linux/blk.h>

#define SR_DEBUG(do_printk_debug, args)                                        \
MACRO_BEGIN                                                             \
        if (do_printk_debug) {                                                 \
                printk args;                                            \
        }                                                               \
MACRO_END

char cdrom_common_debug = 0;

int cdrom_common_ioctl(struct inode *, struct file *, unsigned int,
		unsigned long);

int probe_for_cdrom(char *);

int
cdrom_common_ioctl(
	struct inode	*inode,
	struct file	*file,
	unsigned int	cmd,
	unsigned long	arg)
{
	kdev_t		dev, real_dev;
	unsigned int	major, minor;
	mach_port_t	device_port;
	natural_t 	count, mach_count;
	register int	error = FALSE;
	unsigned int	mach_cmd  = 0;

	SR_DEBUG(cdrom_common_debug,
		    ("cdrom_ioctl: cmd= %x, arg= %lx \n", cmd, arg));

	real_dev = inode->i_rdev;
	dev = blkdev_get_alias(real_dev);

	major = MAJOR(dev);
	minor = MINOR(dev);

	if (major >= MAX_BLKDEV) {
		return -ENODEV;
	}

	if (minor >= MAX_MINORS) {
		return -ENXIO;
	}

	device_port = bdev_port_lookup(dev);
	if (device_port == MACH_PORT_NULL) {
	  SR_DEBUG(cdrom_common_debug,
		      ("cdrom_ioctl: device_port == MACH_PORT_NULL\n"));
		return ENXIO;	/* shouldn't happen */
	}

	ASSERT(MACH_PORT_VALID(device_port));

	if ((cmd & 0xffff) == DEV_GET_SIZE) {
		long status[DEV_GET_SIZE_COUNT];

		count = DEV_GET_SIZE_COUNT;

		error = verify_area(VERIFY_WRITE, (void *)arg, count*sizeof(long));

		if (error) {
		  SR_DEBUG(cdrom_common_debug,
			      ("verify_area error= %x\n", error));
		  return error;
		}

		error = device_get_status(device_port,
					  (cmd & 0xffff),
					  (int *) &status,
					  &count);
		if (error) {
		  MACH3_DEBUG(2, error,
			      (" device_get_status(DEV_GET_SIZE) error= %x\n",
			       error));
		  goto done;
		}

		memcpy_tofs((void *) arg, (const void *) status, 
			    count*sizeof(long)); 

		goto done;
	}

	switch (cmd & 0xffff) { /* Generic handling below the switch */
		/* Sun-compatible */
	    case CDROMPAUSE:
		mach_cmd = MACH_CDROMPAUSE;
		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMPAUSE\n"));
		break;
	    case CDROMRESUME:
		mach_cmd = MACH_CDROMRESUME;
		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMRESUME\n"));
		break;
	    case CDROMSTOP:
		mach_cmd = MACH_CDROMSTOP;
		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMSTOP\n"));
		break;
	    case CDROMSTART:
		mach_cmd = MACH_CDROMSTART;
		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMSTART\n"));
		break;
	    case CDROMEJECT:
		mach_cmd = MACH_CDROMEJECT;
		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMEJECT\n"));
		break;
	}

	if (mach_cmd) {
		long status;

		mach_count = ((sizeof(status) + 3) >> 2);

		status = 0;
		error = device_set_status(device_port, mach_cmd, 
				       (dev_status_t) &status, mach_count);
		if (error) {
		  MACH3_DEBUG(2, error,("device_set_status error= %x\n", error));
		  goto done;
		}
		goto done;
	}

	switch (cmd & 0xffff) {
	    case CDROMPLAYMSF:
	      {
		cd_msf_t status;
		struct cdrom_msf local_args;

		count = sizeof(local_args);		/* bytes */
		mach_count = (sizeof(status) + 3) >> 2;		/* ints */

		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMPLAYMSF\n"));
		
		error = verify_area(VERIFY_READ, (void *)arg, count);
		if (error) {
		  SR_DEBUG(cdrom_common_debug,("verify_area error= %x\n", error));
		  return error;
		}

		memcpy_fromfs((void *) &local_args, (const void *) arg, count);

		status.cdmsf_min0 = local_args.cdmsf_min0;
		status.cdmsf_sec0 = local_args.cdmsf_sec0;
		status.cdmsf_frame0 = local_args.cdmsf_frame0;
		status.cdmsf_min1 = local_args.cdmsf_min1;
		status.cdmsf_sec1 = local_args.cdmsf_sec1;
		status.cdmsf_frame1 = local_args.cdmsf_frame1;

		error = device_set_status(device_port, CDROMPLAYMSF, 
					  (dev_status_t) &status, mach_count);
		if (error) {
		  MACH3_DEBUG(2, error,(" device_set_status error= %x\n", error));
		  goto done;
		}
	      }
	    break;

	    case CDROMPLAYTRKIND:
	      {
		cd_ti_t status;
		struct cdrom_ti local_args;

		count = sizeof(local_args);		/* bytes */
		mach_count = (sizeof(status) + 3) >> 2;		/* ints */

		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMPLAYTRKIND\n"));
		
		error = verify_area(VERIFY_READ, (void *)arg, count);
		if (error) {
		  SR_DEBUG(cdrom_common_debug,("verify_area error= %x\n", error));
		  return error;
		}

		memcpy_fromfs((void *) &local_args, (const void *) arg, count);

		status.cdti_track0 = local_args.cdti_trk0;
		status.cdti_index0 = local_args.cdti_ind0;
		status.cdti_track1 = local_args.cdti_trk1;
		status.cdti_index1 = local_args.cdti_ind1;

		error = device_set_status(device_port, CDROMPLAYTRKIND, 
					  (dev_status_t) &status, mach_count);
		if (error) {
		  MACH3_DEBUG(2, error,(" device_set_status error= %x\n", error));
		  goto done;
		}
	      }
	    break;

	    case CDROMREADTOCHDR:
	      {
		cd_toc_header_t status;
		struct cdrom_tochdr local_args;

		count = sizeof(local_args);			/* bytes */
		mach_count = (sizeof(status) + 3) >> 2;		/* ints */

		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMREADTOCHDR\n"));

		error = verify_area(VERIFY_WRITE, (void *)arg, count);
		if (error) {
		  SR_DEBUG(cdrom_common_debug,("verify_area error= %x\n", error));
		  return error;
		}

		error = device_get_status(device_port, SCMD_READ_HEADER, 
					  (dev_status_t) &status,
					  &mach_count);
		if (error) {
		  MACH3_DEBUG(2, error,
			      (" device_get_status(SCMD_READ_HEADER) "
			       "error= %x\n", error));
		  goto done;
		}

		local_args.cdth_trk0 = status.first_track;
		local_args.cdth_trk1 = status.last_track;

		memcpy_tofs((void *) arg, (const void *) &local_args, count);
	      }
	    break;
	    case CDROMREADTOCENTRY:
	      {
		cd_toc_t *status;
		struct cdrom_tocentry local_args;
		unsigned int num_tracks = 1, req_track = 0;

		{   /* Get the number of tracks from the Toc Header */
			cd_toc_header_t lstatus;
			mach_msg_type_number_t lcount;

			lcount = ((sizeof(lstatus) + 3) >> 2);	/* ints */
			if (lcount == 0)
				lcount = 1;

			SR_DEBUG(cdrom_common_debug,("cdrom_ioctl:"
					   "CDROMREADTOCHDR FOR "
					   "CDROMREADTOCENTRY\n"));

			error = device_get_status(device_port, SCMD_READ_HEADER, 
						  (dev_status_t) &lstatus,
						  &lcount);
			if (error) {
			  MACH3_DEBUG(2, error,
				      (" device_get_status(SCMD_READ_HEADER) "
				       "error= %x\n", error));
			  goto done;
			}

			num_tracks = lstatus.last_track + 1;
			SR_DEBUG(cdrom_common_debug,
				    ("cdrom_ioctl: num_tracks= %d\n", num_tracks));
		}

		status = (cd_toc_t *) kmalloc(num_tracks * sizeof(cd_toc_t),
					      GFP_KERNEL);
		if (status == NULL) {
		  SR_DEBUG(cdrom_common_debug,("no memory for cd_toc_t status, ignoring\n"));
		  return -ENOMEM;
		}

		count = sizeof(local_args);		/* bytes */
		mach_count = (((num_tracks * sizeof(cd_toc_t)) + 3) >> 2); /*ints*/

		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMREADTOCENTRY\n"));
		error = verify_area(VERIFY_WRITE, (void *)arg, count);
		if (error) {
		  SR_DEBUG(cdrom_common_debug,("verify_area error= %x\n", error));
		  goto do_free;
		}

		memcpy_fromfs((void *) &local_args, (const void *) arg, count);

		if (local_args.cdte_format == CDROM_MSF)
			mach_cmd = MACH_SCMD_READ_TOC_MSF;
		else if (local_args.cdte_format == CDROM_LBA)
			mach_cmd = MACH_SCMD_READ_TOC_LBA;
		else { /* sanity */
			SR_DEBUG(cdrom_common_debug,
				    ("cdrom_ioctl: cmd= "
				     "CDROMREADTOCENTRY: Unknown format= %x\n",
				     local_args.cdte_format));
			goto do_free;
		}

		req_track = local_args.cdte_track;	/* requested track */
		SR_DEBUG(cdrom_common_debug,("req_track= %d\n", req_track));
		if (req_track > num_tracks)
			req_track = num_tracks;

		req_track--;	/* First track will be at offset 0 */

		error = device_get_status(device_port, mach_cmd, 
					  (dev_status_t) status,
					  &mach_count);
		if (error) {
		  MACH3_DEBUG(2, error,(" device_get_status(0x%x) error= %x\n",
					mach_cmd, error));
		  goto done;
		}

		local_args.cdte_ctrl = status[req_track].cdt_ctrl;
		local_args.cdte_adr = status[req_track].cdt_adr;
		local_args.cdte_track = status[req_track].cdt_track;
		if (status[req_track].cdt_type == MACH_CDROM_MSF) {
			local_args.cdte_format = CDROM_MSF;
			local_args.cdte_addr.msf.minute = 
				status[req_track].cdt_addr.msf.minute;
			local_args.cdte_addr.msf.second = 
				status[req_track].cdt_addr.msf.second;
			local_args.cdte_addr.msf.frame = 
				status[req_track].cdt_addr.msf.frame;
		}
		else if (status[req_track].cdt_type == MACH_CDROM_LBA) {
			local_args.cdte_format = CDROM_LBA;
			local_args.cdte_addr.lba = 
				status[req_track].cdt_addr.lba;
		}

		memcpy_tofs((void *) arg, (const void *) &local_args, count); 

do_free:
		kfree((void *) status);
	      }
		
	    break;

	    case CDROMVOLCTRL:
	      {
		cd_volctrl_t status;
		struct cdrom_volctrl local_args;

		count = sizeof(local_args);		/* bytes */
		mach_count = (sizeof(status) + 3) >> 2;		/* ints */

		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMVOLCTRL\n"));
		
		error = verify_area(VERIFY_READ, (void *)arg, count);
		if (error) {
		  SR_DEBUG(cdrom_common_debug,("verify_area error= %x\n", error));
		  return error;
		}

		memcpy_fromfs((void *) &local_args, (const void *) arg, count);

		status.channel0 = local_args.channel0;
		status.channel1 = local_args.channel1;
		status.channel2 = local_args.channel2;
		status.channel3 = local_args.channel3;

		error = device_set_status(device_port, CDROMVOLCTRL, 
					  (dev_status_t) &status, mach_count);
		if (error) {
		  if (error != D_INVALID_OPERATION)  /* Don't print this noise :-) */
   		     MACH3_DEBUG(2, error,(" device_set_status error= %x\n", error));
		  goto done;
		}
		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMVOLCTRL\n"));
	      }
	    break;

	    case CDROMVOLREAD:
	    {
		cd_volctrl_t status;
		struct cdrom_volctrl local_args;

		count = sizeof(local_args);		/* bytes */
		mach_count = (sizeof(status) + 3) >> 2;		/* ints */

		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMVOLREAD\n"));
		
		error = verify_area(VERIFY_WRITE, (void *)arg, count);
		if (error) {
		  SR_DEBUG(cdrom_common_debug,("verify_area error= %x\n", error));
		  return error;
		}


		error = device_get_status(device_port, MACH_CDROMVOLREAD, 
					  (dev_status_t) &status, &mach_count);
		if (error) {
		  if (error != D_INVALID_OPERATION)  /* Don't print this noise :-) */
   		     MACH3_DEBUG(2, error,(" device_set_status error= %x\n", error));
		  goto done;
		}


		local_args.channel0 = status.channel0;
		local_args.channel1 = status.channel1;
		local_args.channel2 = status.channel2;
		local_args.channel3 = status.channel3;

		memcpy_tofs((void *) arg, (const void *) &local_args, count);

		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMVOLCTRL\n"));
	    }
	    break;

	    case CDROMSUBCHNL:
	      {
		cd_subchnl_t status;
		struct cdrom_subchnl local_args;

		count = sizeof(local_args);		/* bytes */
		mach_count = (sizeof(status) + 3) >> 2;		/* ints */

		SR_DEBUG(cdrom_common_debug,("cdrom_ioctl: CDROMSUBCHNL\n"));

		error = verify_area(VERIFY_WRITE, (void *)arg, count);
		if (error) {
		  SR_DEBUG(cdrom_common_debug,("verify_area error= %x\n", error));
		  return error;
		}

		memcpy_fromfs((void *) &local_args, (const void *) arg, count);

		if (local_args.cdsc_format == CDROM_MSF)
			mach_cmd = MACH_SCMD_READ_SUBCHNL_MSF;
		else if (local_args.cdsc_format == CDROM_LBA)
			mach_cmd = MACH_SCMD_READ_SUBCHNL_LBA;
		else { /* sanity */
			SR_DEBUG(cdrom_common_debug,
				    ("cdrom_ioctl: cmd= CDROMSUBCHNL: "
				     "Unknown format= %x\n",
				     local_args.cdsc_format));
			goto done;
		}
		
		error = device_get_status(device_port, mach_cmd, 
					  (dev_status_t) &status,
					  &mach_count);
		if (error) {
		  if (error != D_INVALID_OPERATION)  /* Don't print this noise :-) */
			  MACH3_DEBUG(2, error,
				      (" device_get_status(0x%x) error= %x\n",
				       mach_cmd, error));
		  goto done;
		}

		if (status.cdsc_type == MACH_CDROM_MSF) {
			local_args.cdsc_format = CDROM_MSF;
			local_args.cdsc_absaddr.msf.minute = 
			  status.cdsc_absolute_addr.msf.minute;
			local_args.cdsc_absaddr.msf.second =
			  status.cdsc_absolute_addr.msf.second;
			local_args.cdsc_absaddr.msf.frame =
			  status.cdsc_absolute_addr.msf.frame;

			local_args.cdsc_reladdr.msf.minute = 
			  status.cdsc_relative_addr.msf.minute;
			local_args.cdsc_reladdr.msf.second =
			  status.cdsc_relative_addr.msf.second;
			local_args.cdsc_reladdr.msf.frame =
			  status.cdsc_relative_addr.msf.frame;
		}
		else if (status.cdsc_type == MACH_CDROM_LBA) {
			local_args.cdsc_format = CDROM_LBA;
			local_args.cdsc_absaddr.lba = 
			  status.cdsc_absolute_addr.lba;

			local_args.cdsc_reladdr.lba = 
			  status.cdsc_relative_addr.lba;
		}

		local_args.cdsc_trk = status.cdsc_track;
		local_args.cdsc_ind = status.cdsc_index;
		local_args.cdsc_audiostatus = status.cdsc_audiostatus;
		local_args.cdsc_adr = status.cdsc_adr;
		local_args.cdsc_ctrl = status.cdsc_ctrl;

		memcpy_tofs((void *) arg, (const void *) &local_args, count); 
	      }
	    break;

	    case CDROMREADMODE2:
		return -EINVAL;

	    case CDROMREADMODE1:
		return -EINVAL;

	    case CDROMMULTISESSION:
#if 0
		printk("cdrom_ioctl(dev=%s, cmd=CDROMMULTISESSION): "
		       "not implemented\n",
		       kdevname(dev));
#endif
		return -EOPNOTSUPP;

	    case BLKRASET:
		if (!suser()) return -EACCES;
		if (!inode->i_rdev) return -EINVAL;
		if (arg > 0xff) return -EINVAL;
		read_ahead[MAJOR(inode->i_rdev)] = arg;
		return 0;

	    RO_IOCTLS(dev,arg);
	    default:
		return blkdev_fops.ioctl(inode, file, cmd, arg);
	}

done:
	if (error) {
		SR_DEBUG(cdrom_common_debug,("Error 0x%x in cdrom_ioctl(dev=%s, cmd=0x%x) \n",
		       error, kdevname(inode->i_rdev), cmd));
		return -EINVAL;
	}
	else
		return 0;

}

int
probe_for_cdrom(char *device_name)
{
	kern_return_t	kr;
	mach_port_t	device_port;

	server_thread_blocking(FALSE);
	kr = device_open(device_server_port,
			 MACH_PORT_NULL,
			 D_READ | D_NODELAY,
			 server_security_token,
			 device_name,
			 &device_port);
	server_thread_unblocking(FALSE);

	if (kr) 		/* Nope.. it ain't it */
		return FALSE;

	/* Magic here.. to test for a CDROM device, simply check to
	 * see if the device will reject a CDROMVOLCTRL command with
	 * an invalid size error, anything else is assumed to be a
	 * harddisk.
	 */

	kr = device_set_status(device_port, CDROMVOLCTRL, (dev_status_t) 0, 0);

	device_close(device_port);

	return (kr == D_INVALID_SIZE) ? TRUE : FALSE;
}
/*
 * @OSF_FREE_FREE_COPYRIGHT@
 * 
 */
/*
 * HISTORY
 * $Log: cdrom.c,v $
 * Revision 1.1.2.2  1997/12/23  09:03:23  stephen
 * 	Add CDROMVOLREAD
 * 	[1997/12/23  09:01:41  stephen]
 *
 * Revision 1.1.3.2  1997/12/23  09:01:41  stephen
 * 	Add CDROMVOLREAD
 *
 * Revision 1.1.2.1  1997/12/18  14:04:45  stephen
 * 	New file
 * 	[1997/12/18  14:02:33  stephen]
 *
 * Revision 1.1.1.2  1997/12/18  14:02:33  stephen
 * 	New file
 *
 */
