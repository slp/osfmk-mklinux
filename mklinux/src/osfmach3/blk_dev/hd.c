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

#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/cdrom.h>
#include <linux/mm.h>
#include <linux/malloc.h>
#define MAJOR_NR	HD_MAJOR
#include <linux/blk.h>


#define	HD_MAJOR_COUNT	4

int	hd_majors[HD_MAJOR_COUNT] =
	{ HD_MAJOR, IDE1_MAJOR, IDE2_MAJOR, IDE3_MAJOR };


#define MAX_MINORS	256
int hd_blocksizes[HD_MAJOR_COUNT][MAX_MINORS];
int hd_refs[HD_MAJOR_COUNT][MAX_MINORS];	/* number of refs on Mach devices */
kdev_t hd_aliases[HD_MAJOR_COUNT][MAX_MINORS];	/* aliases for special minors */
int hd_mach_blocksizes[HD_MAJOR_COUNT][MAX_MINORS];

char	hd_cdroms[HD_MAJOR_COUNT] = { 0} ;

struct file_operations hd_fops[HD_MAJOR_COUNT];
int cdrom_common_ioctl(struct inode *, struct file *, unsigned int,
                unsigned long);
extern	int probe_for_cdrom(char *device);
int	hd_is_cdrom(int major, int minor);

int
hd_is_cdrom(int major, int minor)
{
	int	ctlr;

	switch (major) {
	case	IDE0_MAJOR:
		ctlr = 0;
		break;

	case	IDE1_MAJOR:
		ctlr = 1;
		break;

	case	IDE2_MAJOR:
		ctlr = 2;
		break;

	case	IDE3_MAJOR:
		ctlr = 3;
		break;
	default:
		return FALSE;
	}

	if (hd_cdroms[ctlr] & 1<<(minor / osfmach3_blkdevs[major].minors_per_major))
		return	TRUE;
	else
		return	FALSE;
}


int
hd_disk_dev_to_name(kdev_t	dev, char	*name)
{
	unsigned int major, minor, unit;

	dev = blkdev_get_alias(dev);

	major = MAJOR(dev);
	minor = MINOR(dev);

	switch (major) {
	case	IDE0_MAJOR:
		unit = 0;
		break;

	case	IDE1_MAJOR:
		unit = 2;
		break;

	case	IDE2_MAJOR:
		unit = 4;
		break;

	case	IDE3_MAJOR:
		unit = 6;
		break;

	default:
		return -ENODEV;
	}

	sprintf(name, "hd%d%c",
		(minor / osfmach3_blkdevs[major].minors_per_major) + unit,
		'a' + (minor % osfmach3_blkdevs[major].minors_per_major) - 1);

	return 0;
}


static void
register_hd(int ctlr, int major_nr, int *hd_blocksizes,
	int *hd_refs, kdev_t *hd_aliases, int *hd_mach_blocksizes,
	struct file_operations *fops)
{
	int minors_per_major, unit, block_size;
	int i, p;
	int ret;
	char	cdrom_name[16];

	for (minors_per_major = 0;
	     DEVICE_NR(minors_per_major) == 0 && minors_per_major < MAX_MINORS;
	     minors_per_major++);

	memset(hd_aliases, 0, MAX_MINORS * sizeof(hd_aliases));
	memset(hd_refs, 0, MAX_MINORS * sizeof(hd_refs));

	*fops = blkdev_fops;
	fops->ioctl = gen_disk_ioctl;

	/*
	 * Probe for IDE/ATAPI cdrom devices and adjust the block size
	 * appropriately.
	 */

	for (unit = 0, p = 0; unit < 2; unit++) {
		sprintf(cdrom_name, "hd%da", (ctlr*2)+unit);
		if (probe_for_cdrom(cdrom_name)) {
			block_size = 2048;
			hd_cdroms[ctlr] |= (1<<unit);
			fops->ioctl = cdrom_common_ioctl;
		} else {
			/*
			 * Minor 0 of a unit is the whole unit.
			 * We don't have that concept in Mach, so just
			 * make an alias to the first real minor in the
			 * partition and hope it works...
			 */
			hd_aliases[p] = MKDEV(major_nr, p + 1);
			block_size = 1024;
		}

		for (i = 0; i < minors_per_major; i++, p++)
			hd_blocksizes[p] = block_size;

	}

	if (register_blkdev(major_nr, "hd", fops)) {
		printk("Unable to get major %d for harddisk\n", major_nr);
		return;
	}

	blksize_size[major_nr] = hd_blocksizes;
	/* no hardsect_size ... */
	read_ahead[major_nr] = 256;	/* 256 sector (128kB) read-ahead */

	ret = osfmach3_register_blkdev(major_nr,
				       "hd",
				       OSFMACH3_DEV_BSIZE,
				       hd_mach_blocksizes,
				       minors_per_major,
				       hd_disk_dev_to_name,
				       gen_disk_name_to_dev,
				       gen_disk_process_req_for_alias);
	if (ret) {
		printk("hd_init: osfmach3_register_blkdev failed %d\n", ret);
		panic("hd_init: can't register Mach device");
	}

	osfmach3_blkdev_refs[major_nr] = hd_refs;
	osfmach3_blkdev_alias[major_nr] = hd_aliases;

}

int hd_init(void)
{
	int	i;

	for (i = 0; i < HD_MAJOR_COUNT; i++) {
		register_hd(i, hd_majors[i], hd_blocksizes[i],
			hd_refs[i], hd_aliases[i], hd_mach_blocksizes[i],
			&hd_fops[i]);
	}
	return 0;
}

