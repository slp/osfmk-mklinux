/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
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

#define MAJOR_NR	SCSI_CDROM_MAJOR
#include <linux/blk.h>

#define MAX_MINORS	256
int sr_blocksizes[MAX_MINORS];
int sr_sizes[MAX_MINORS];
int sr_refs[MAX_MINORS];
int sr_mach_blocksizes[MAX_MINORS];

struct file_operations sr_fops;

extern void blkdev_get_reference(kdev_t );

extern void blkdev_release_reference(kdev_t );

#define SR_DEBUG(do_printk_debug, args)                                        \
MACRO_BEGIN                                                             \
        if (do_printk_debug) {                                                 \
                printk args;                                            \
        }                                                               \
MACRO_END

char sr_do_debug = 0;

int cdrom_common_ioctl(struct inode *, struct file *, unsigned int,
			unsigned long);

int
cdrom_dev_to_name(
	kdev_t	dev,
	char	*name)
{
	unsigned int major, minor;

	major = MAJOR(dev);
	minor = MINOR(dev);

	if (major != SCSI_CDROM_MAJOR) {
		return -ENODEV;
	}
	if (osfmach3_blkdevs[major].name == NULL) {
		printk("cdrom_dev_to_name: no Mach info for dev %s\n",
		       kdevname(dev));
		panic("cdrom_dev_to_name");
	}

	if (minor >= MAX_MINORS) {
		return -ENODEV;
	}

	sprintf(name, "%s%d", osfmach3_blkdevs[major].name, minor);
	return 0;
}

int
sr_open(
	struct inode	*inode,
	struct file	*filp)
{

	return blkdev_fops.open(inode, filp);
}

void
sr_release(
	struct inode	*inode,
	struct file	*file)
{
	if (sr_refs[MINOR(inode->i_rdev)] == 1) {
		/*
		 * Releasing the last reference on the device.
		 * Clean buffers.
		 */
		sync_dev(inode->i_rdev);
		invalidate_inodes(inode->i_rdev);
		invalidate_buffers(inode->i_rdev);
	}

	blkdev_fops.release(inode, file);

	sr_sizes[MINOR(inode->i_rdev)] = 0;

	return;
}

int sr_init(void)
{
	static int	sr_registered = 0;
	int		minors_per_major;
	int		i;
	int		ret;

	if (sr_registered) {
		return 0;
	}

	sr_fops = blkdev_fops;
	sr_fops.ioctl = cdrom_common_ioctl;
	sr_fops.open = sr_open;
	sr_fops.release = sr_release;
	sr_fops.fsync = NULL;
	if (register_blkdev(MAJOR_NR, "sr", &sr_fops)) {
		printk("unable to get major %d for SCSI-CD\n",
		       MAJOR_NR);
		return -EBUSY;
	}

	for (minors_per_major = 0;
	     DEVICE_NR(minors_per_major) == 0 && minors_per_major < MAX_MINORS;
	     minors_per_major++);

	for (i = 0; i < MAX_MINORS; i++) {
		sr_blocksizes[i] = 2048;
		sr_sizes[i] = 0;
	}
	blksize_size[MAJOR_NR] = sr_blocksizes;
#if 0
	blk_size[MAJOR_NR] = sr_sizes;
#endif
	read_ahead[MAJOR_NR] = 32;

	ret = osfmach3_register_blkdev(MAJOR_NR,
				       "cdrom",
				       OSFMACH3_DEV_BSIZE,
				       sr_mach_blocksizes,
				       minors_per_major,
				       cdrom_dev_to_name,
				       NULL,
				       NULL);
	if (ret) {
		printk("sr_init: osfmach3_register_blkdev failed %d\n", ret);
		panic("sr_init: can't register Mach device");
	}

	for (i = 0; i < MAX_MINORS; i++) {
		sr_refs[i] = 0;
	}
	osfmach3_blkdev_refs[MAJOR_NR] = sr_refs;
	osfmach3_blkdev_alias[MAJOR_NR] = NULL;

	return 0;
}
