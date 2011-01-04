/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <mach/std_types.h>
#include <machine/disk.h>
#include <device/device.h>
#include <linux/types.h> /* for daddr_t */
#include <device/disk_status.h>

#include <osfmach3/device_utils.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/block_dev.h>

#include <linux/hdreg.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ioctl.h>

#include <asm/segment.h>

#include "mach_ioctl.h"

#if	CONFIG_OSFMACH3_DEBUG
#define GENDISK_DEBUG	1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	GENDISK_DEBUG
extern int gendisk_debug;
#endif	/* GENDISK_DEBUG */

int
machine_disk_get_params(
	kdev_t			dev,
	mach_port_t		device_port,
	struct hd_geometry	*loc,
	boolean_t		partition_only)
{
	kern_return_t		kr;
	struct disk_parms	dp;
#if 0
	struct disklabel	dl;
#endif
	unsigned int		count;
	unsigned char		heads;
	unsigned char		sectors;
	unsigned short		cylinders;
	long			start;

	switch ((int) MAJOR(dev)) {

	    case SCSI_DISK_MAJOR:
#if 0
		count = sizeof (struct disklabel) / sizeof (int);
		kr = device_get_status(device_port,
				       DIOCGDINFO,
				       (dev_status_t) &dl,
				       &count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("machine_get_disk_params(%s): "
				     "device_get_status(0x%x, DIOCGDINFO)",
				     kdevname(dev), device_port));
			return -EINVAL;
		}
		heads = (unsigned char) dl.d_ntracks;
		sectors = (unsigned char) dl.d_nsectors;	/* XXX */
		cylinders = (unsigned short) dl.d_ncylinders;
		start = 0;	/* XXX */
		break;
#endif

	    case HD_MAJOR:
	    case FLOPPY_MAJOR:
		count = sizeof (struct disk_parms) / sizeof (int);
		kr = device_get_status(device_port,
				       OSFMACH3_V_GETPARMS,
				       (dev_status_t) &dp,
				       &count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("machine_get_disk_params(%s): "
				     "device_get_status(0x%x, V_GETPARMS)",
				     kdevname(dev), device_port));
			return -EINVAL;
		}
		heads = dp.dp_dosheads;
		cylinders = dp.dp_doscyls;
		sectors = dp.dp_dossectors;
		if (partition_only) {
#if 0
			sectors = (dp.dp_pnumsec * dp.dp_secsiz) / 512;
#endif
			start = (dp.dp_pstartsec * dp.dp_secsiz) / 512;
		} else {
			start = 0;
		}
		break;

	    default:
		return -EINVAL;
	}

#ifdef	GENDISK_DEBUG
	if (gendisk_debug) {
		printk("machine_disk_get_params(dev=%s, part_only=%d): "
		       "%d heads, %d cylinders, %d sectors, start at %ld\n",
		       kdevname(dev), partition_only,
		       heads, cylinders, sectors, start);
	}
#endif	/* GENDISK_DEBUG */

	put_fs_byte(heads, (char *) &loc->heads);
	put_fs_byte(sectors, (char *) &loc->sectors);
	put_fs_word(cylinders, (short *) &loc->cylinders);
	put_fs_long(start, (long *) &loc->start);

	return 0;
}

kern_return_t
machine_disk_read_absolute(
	kdev_t		dev,
	mach_port_t	device_port,
	unsigned long	recnum,
	char		*data,
	unsigned long	size)
{
	kern_return_t		kr;
	mach_msg_type_number_t	count;
	int			abs_sec; /* absolute sector to be read */

	abs_sec = recnum;
	while (size > 0) {
#ifdef	GENDISK_DEBUG
		if (gendisk_debug) {
			printk("machine_disk_read_absolute(dev=%s, "
			       "recnum=0x%lx, size=0x%lx): RDABS(0x%x,0x%x)\n",
			       kdevname(dev),
			       recnum, size, abs_sec, OSFMACH3_DEV_BSIZE);
		}
#endif	/* GENDISK_DEBUG */

		/*
		 * Tell the micro-kernel which sector to read.
		 */
		count = 1;
		kr = device_set_status(device_port,
				       OSFMACH3_V_ABS,
				       (dev_status_t) &abs_sec,
				       count);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr, ("machine_disk_read_absolute(%s): "
					    "device_set_status(0x%x, V_ABS, "
					    "0x%x)",
					    kdevname(dev),
					    device_port, abs_sec));
			return kr;
		}

		/*
		 * Read the sector now.
		 */
		count = OSFMACH3_DEV_BSIZE / sizeof (int);
		server_thread_blocking(FALSE);
		kr = device_get_status(device_port,
				       OSFMACH3_V_RDABS,
				       (dev_status_t) data,
				       &count);
		server_thread_unblocking(FALSE);
		if (kr != D_SUCCESS) {
			if (kr != D_INVALID_RECNUM &&
			    kr != D_INVALID_SIZE) {
				MACH3_DEBUG(1, kr,
					    ("machine_disk_read_absolute(%s): "
					     "device_get_status(0x%x, "
					     "V_RDABS(0x%x,%d))",
					     kdevname(dev), device_port,
					     abs_sec, count));
			}
			return kr;
		}
		if ((count * sizeof (int)) > size) 
			break;
		size -= count * sizeof (int);
		data += count * sizeof (int);
		abs_sec++;
	}

	return D_SUCCESS;
}

kern_return_t
machine_disk_write_absolute(
	kdev_t		dev,
	mach_port_t	device_port,
	unsigned long	recnum,
	char		*data,
	unsigned long	size)
{
	kern_return_t		kr;
	mach_msg_type_number_t 	count;
	int			abs_sec; /* absolute sector to be written */

	abs_sec = recnum;
	while (size > 0) {
#ifdef	GENDISK_DEBUG
		if (gendisk_debug) {
			printk("machine_disk_write_absolute(dev=%s, "
			       "recnum=0x%lx, size=0x%lx): WRABS(0x%x,0x%x)\n",
			       kdevname(dev),
			       recnum, size, abs_sec, OSFMACH3_DEV_BSIZE);
		}
#endif	/* GENDISK_DEBUG */

		/*
		 * Tell the micro-kernel which sector to write.
		 */
		count = 1;
		kr = device_set_status(device_port,
				       OSFMACH3_V_ABS,
				       (dev_status_t) &abs_sec,
				       count);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("machine_disk_write_absolute(%s): "
				     "device_set_status(0x%x, V_ABS, 0x%x)",
				     kdevname(dev), device_port, abs_sec));
			return kr;
		}

		/*
		 * Write the sector now.
		 */
		count = OSFMACH3_DEV_BSIZE / sizeof (int);
		server_thread_blocking(FALSE);
		kr = device_set_status(device_port,
				       OSFMACH3_V_WRABS,
				       (dev_status_t) data,
				       count);
		server_thread_unblocking(FALSE);
		if (kr != D_SUCCESS) {
			if (kr != D_INVALID_RECNUM &&
			    kr != D_INVALID_SIZE) {
				MACH3_DEBUG(1, kr,
					    ("machine_disk_write_absolute(%s):"
					     " device_set_status(0x%x, "
					     "V_WRABS(0x%x,%d)",
					     kdevname(dev), device_port,
					     abs_sec, count));
			}
			return kr;
		}
		if ((count * sizeof (int)) > size)
			break;
		size -= count * sizeof (int);
		data += count * sizeof (int);
		abs_sec++;
	}

	return D_SUCCESS;
}

int
machine_disk_revalidate(
	kdev_t			dev,
	mach_port_t		device_port)
{
	kern_return_t		kr;
	int			count;

	switch ((int) MAJOR(dev)) {

	    case SCSI_DISK_MAJOR:
	    case HD_MAJOR:
	    case FLOPPY_MAJOR:
		count = 0;
		kr = device_set_status(device_port,
				       OSFMACH3_V_REMOUNT,
				       &count,
				       count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("machine_disk_revalidate(%s): "
				     "device_get_status(0x%x, V_REMOUNT)",
				     kdevname(dev), device_port));
			return -EINVAL;
		}
		break;
	    default:
		return -EINVAL;
	}
	return 0;
}
