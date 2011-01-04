/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/block_dev.h>

#include <linux/hdreg.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#include <asm/segment.h>

#ifdef hp_pa
#include <device/disk_status.h>
#endif

#if	CONFIG_OSFMACH3_DEBUG
#define GENDISK_DEBUG	1
#endif	/* CONFIG_OSFAMCH3_DEBUG */

#ifdef	GENDISK_DEBUG
int gendisk_debug = 0;
#endif	/* GENDISK_DEBUG */

int
gen_disk_dev_to_name(
	kdev_t	dev,
	char	*name)
{
	unsigned int major, minor;

	dev = blkdev_get_alias(dev);

	major = MAJOR(dev);
	minor = MINOR(dev);
	
	if (major >= MAX_BLKDEV) {
		return -ENODEV;
	}
	if (osfmach3_blkdevs[major].name == NULL) {
		panic("gen_disk_dev_to_name: no Mach info for dev %s\n",
		      kdevname(dev));
	}

	sprintf(name, "%s%d%c",
		osfmach3_blkdevs[major].name,
		minor / osfmach3_blkdevs[major].minors_per_major,
		'a' + (minor % osfmach3_blkdevs[major].minors_per_major) - 1);

	return 0;
}

int
gen_disk_name_to_dev(
	const char	*name,
	kdev_t		*devp)
{
	unsigned int major, minor;
	unsigned int disk;

	for (major = 0; major < MAX_BLKDEV; major++) {
		if (osfmach3_blkdevs[major].name != NULL &&
		    !strncmp(name, osfmach3_blkdevs[major].name,
			     strlen(osfmach3_blkdevs[major].name)))
			break;
	}
	if (major >= MAX_BLKDEV) {
		return -ENODEV;
	}

	name += strlen(osfmach3_blkdevs[major].name);
	for(disk = 0; (*name >= '0') && (*name <= '9'); name++)
		disk = (disk * 10) + (*name - '0');
	minor = disk * osfmach3_blkdevs[major].minors_per_major
		+ *name - 'a' + 1;

	*devp = MKDEV(major, minor);

	return 0;
}

/*
 * This routine is called to flush all partitions and partition tables
 * for a changed disk.
 * If we are revalidating a disk because of a media change, then we
 * enter with usage == 0.  If we are using an ioctl, we automatically have
 * usage == 1 (we need an open channel to use an ioctl :-), so this
 * is our limit.
 */
int gen_disk_revalidate(kdev_t dev, int maxusage)
{
	unsigned int major, minor;
	int max_p;
	int start, i;
	kdev_t partition;
	int usage;

	major = MAJOR(dev);
	minor = MINOR(dev);

	if (osfmach3_blkdevs[major].name == NULL) {
		panic("gen_disk_revalidate: no mach info for device %s\n",
		      kdevname(dev));
	}

	max_p = osfmach3_blkdevs[major].minors_per_major;
	start = minor - (minor % max_p);

#ifdef	GENDISK_DEBUG
	if (gendisk_debug) {
		printk("gen_disk_revalidate(%s): start=%d max_p=%d\n",
		       kdevname(dev), start, max_p);
	}
#endif	/* GENDISK_DEBUG */

	if (osfmach3_blkdev_refs[major]) {
		usage = 0;
		for (i = max_p; i > 0; i--) {
			usage += osfmach3_blkdev_refs[major][start + i];
		}
		if (usage > maxusage) {
#ifdef	GENDISK_DEBUG
			if (gendisk_debug) {
				printk("gen_disk_revalidate(%s): "
				       "usage %d > maxusage %d\n",
				       kdevname(dev), usage, maxusage);
			}
#endif	/* GENDISK_DEBUG */
			return -EBUSY;
		}
	}

	for (i = max_p; i >= 0; i--) {
		partition = MKDEV(major, start + i);
#ifdef	GENDISK_DEBUG
		if (gendisk_debug) {
			printk("gen_disk_revalidate(%s): "
			       "flushing partition %s\n",
			       kdevname(dev), kdevname(partition));
		}
#endif	/* GENDISK_DEBUG */
		sync_dev(partition);
		invalidate_inodes(partition);
		invalidate_buffers(partition);
	}

	return 0;
}

int
gen_disk_ioctl(
	struct inode * inode,
	struct file * file,
	unsigned int cmd,
	unsigned long arg)
{
	int			error;
	kern_return_t		kr;
	unsigned int		count;
	int			dev_status[DEV_GET_SIZE_COUNT];
	kdev_t			dev;
	unsigned int		major, minor;
	mach_port_t		device_port;
	struct hd_geometry	*loc;
	boolean_t		partition_only;

	dev = inode->i_rdev;
	dev = blkdev_get_alias(dev);

	major = MAJOR(dev);
	minor = MINOR(dev);
	if (major >= MAX_BLKDEV) {
		return -ENODEV;
	}

	if (osfmach3_blkdevs[major].name == NULL) {
		panic("gen_disk_ioctl: no mach info for device %s\n",
		      kdevname(dev));
	}

	device_port = bdev_port_lookup(dev);
	if (!MACH_PORT_VALID(device_port)) {
		panic("gen_disk_ioctl: invalid device port for dev %s\n",
		      kdevname(dev));
	}

#ifdef	GENDISK_DEBUG
	if (gendisk_debug) {
		printk("gen_disk_ioctl: dev %s cmd 0x%x\n", kdevname(dev), cmd);
	}
#endif	/* GENDISK_DEBUG */

	switch (cmd) {

	    case BLKGETSIZE:	/* Return device size (in sectors) */
		if (!arg)
			return -EINVAL;
		error = verify_area(VERIFY_WRITE, (long *) arg,
				    sizeof (long));
		if (error)
			return error;
		count = DEV_GET_SIZE_COUNT;
		kr = device_get_status(device_port,
				       DEV_GET_SIZE,
				       (dev_status_t) dev_status,
				       &count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("gen_disk_ioctl(%s, 0x%x): "
				     "device_get_status", kdevname(dev), cmd));
			return -EINVAL;
		}
		put_fs_long(dev_status[DEV_GET_SIZE_DEVICE_SIZE] / 
			    dev_status[DEV_GET_SIZE_RECORD_SIZE],
			    (long *) arg);
		return 0;

	    case BLKRAGET:	/* Get read_ahead */
		if (!arg)
			return -EINVAL;
		error = verify_area(VERIFY_WRITE, (long *) arg,
				    sizeof (long));
		if (error)
			return error;
		if (!dev)
			return -EINVAL;
		put_fs_long(read_ahead[MAJOR(dev)], (long *) arg);
		return 0;

	    case BLKRASET:	/* Set read_ahead */
		if (!suser())
			return -EACCES;
		if (!dev)
			return -EINVAL;
		if (arg > 0xff)
			return -EINVAL;
		read_ahead[MAJOR(dev)] = arg;
		return 0;

	    case BLKFLSBUF:	/* Flush buffer cache */
		if (!suser())
			return -EACCES;
		if (!dev)
			return -EINVAL;
		fsync_dev(dev);
		invalidate_buffers(dev);
		return 0;

	    case BLKRRPART:	/* Re-read partition tables */
		error = machine_disk_revalidate(dev, device_port);
		if (error) return (error);
		return gen_disk_revalidate(inode->i_rdev, 1);

	    case HDIO_GETGEO:
		if (!arg)
			return -EINVAL;
		loc = (struct hd_geometry *) arg;
		error = verify_area(VERIFY_WRITE, loc, sizeof (*loc));
		if (error)
			return error;
		if (dev != inode->i_rdev) {
			/*
			 * We're accessing an aliased device.
			 * That's probably a /dev/sda or so device,
			 * so get info on the whole disk unit, not
			 * only on the partition.
			 */
			partition_only = FALSE;
		} else {
			partition_only = TRUE;
		}
		error = machine_disk_get_params(dev,
						device_port,
						loc,
						partition_only);
		return error;

#ifdef hp_pa
	case DIOCGDINFO:
	{
		struct disklabel	dl;

		if (!arg)
			return -EINVAL;
		error = verify_area(VERIFY_WRITE, (long *) arg,
				    sizeof (struct disklabel));
		if (error)
			return error;

		count = sizeof(struct disklabel) / sizeof(int);
		kr = device_get_status(device_port,
				       DIOCGDINFO,
				       (dev_status_t) &dl,
				       &count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("gen_disk_ioctl(%s, 0x%x): "
				     "device_get_status", kdevname(dev), cmd));
			return -EINVAL;
		}

		memcpy_tofs((void*)arg, &dl, sizeof(dl));

		return 0;
	}
	case DIOCWDINFO:
	{
		struct disklabel dl;

		if (!arg)
			return -EINVAL;
		error = verify_area(VERIFY_READ, (long *) arg,
				    sizeof (struct disklabel));
		if (error)
			return error;
		count = sizeof(struct disklabel) / sizeof(int);

		memcpy_fromfs(&dl, (void *)arg, sizeof(struct disklabel));

		kr = device_set_status(device_port,
				       DIOCWDINFO,
				       (dev_status_t) &dl,
				       count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("gen_disk_ioctl(%s, 0x%x): "
				     "device_get_status", kdevname(dev), cmd));
			return -EINVAL;
		}

		return 0;
	}
#endif
	    default:
#ifdef GENDISK_DEBUG
		printk("gen_disk_ioctl(dev=%s, cmd=0x%x): not implemented\n",
		       kdevname(dev), cmd);
#endif
		return -EINVAL;
	}
}

int
gen_disk_process_req_for_alias(
	int			rw,
	struct buffer_head	*bh,
	mach_port_t		device_port)
{
	kdev_t		dev;
	unsigned long	recnum, mach_size, mach_blocksize;
	kern_return_t	kr;

	/*
	 * We're probably accessing a /dev/sda or such device
	 * that has been aliased to one of its partitions.
	 * Since we probably want to read outside the scope of this
	 * partition, do absolute reads... XXX
	 */

	dev = bh->b_rdev;
	dev = blkdev_get_alias(dev);

#if 0
	/*
	 * Use the original device to convert the block number since that's
	 * the device we're supposed to use and relative to which b_blocknr
	 * has been computed.	
	 */
	recnum = blkdev_mach_blocknr(bh->b_rdev, bh->b_rsector);
#else
	blkdev_mach_blocknr(dev, bh->b_rsector, bh->b_size,
			    &recnum, &mach_size, &mach_blocksize);
#endif

	if (mach_size != bh->b_size) {
		printk("gen_disk_process_req_for_alias(dev %s): "
		       "size mismatch\n", kdevname(dev));
		switch (rw) {
		    case READ:
			block_read_reply((char *) bh, KERN_FAILURE, NULL, 0);
			break;
		    case WRITE:
			block_write_reply((char *) bh, KERN_FAILURE, NULL, 0);
			break;
		}
		return 0;
	}

	switch (rw) {
	    case READ:
#ifdef	GENDISK_DEBUG
		if (gendisk_debug) {
			printk("gen_disk_process_req_for_alias: "
			       "RDABS rsector 0x%lx size %ld dev %s\n",
			       bh->b_rsector, bh->b_size, kdevname(dev));
			}
#endif	/* GENDISK_DEBUG */
		kr = machine_disk_read_absolute(dev, device_port,
						recnum,
						bh->b_data, bh->b_size);
		block_read_reply((char *) bh, kr, NULL, 0);
		break;
	    case WRITE:
#ifdef	GENDISK_DEBUG
		if (gendisk_debug) {
			printk("gen_disk_process_req_for_alias: "
			       "WRABS rsector 0x%lx size %ld dev %s\n",
			       bh->b_rsector, bh->b_size, kdevname(dev));
		}
#endif	/* GENDISK_DEBUG */
		kr = machine_disk_write_absolute(dev, device_port,
						 recnum,
						 bh->b_data,
						 bh->b_size);
		block_write_reply((char *) bh, kr, NULL, 0);
		break;
	}

	return 0;
}
