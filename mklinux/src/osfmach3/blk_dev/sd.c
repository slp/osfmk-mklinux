/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <osfmach3/block_dev.h>

#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/fs.h>

#define MAJOR_NR	SCSI_DISK_MAJOR
#include <linux/blk.h>

#define MAX_MINORS	256
int sd_blocksizes[MAX_MINORS];
int sd_hardsizes[MAX_MINORS];
int sd_refs[MAX_MINORS];	/* number of refs on Mach devices */
kdev_t sd_aliases[MAX_MINORS];	/* aliases for special minors */
int sd_mach_blocksizes[MAX_MINORS];

struct file_operations sd_fops;

int sd_init(void)
{
	int minors_per_major;
	int i;
	int ret;

	sd_fops = blkdev_fops;
	sd_fops.ioctl = gen_disk_ioctl;
	if (register_blkdev(MAJOR_NR, "sd", &sd_fops)) {
		printk("Unable to get major %d for SCSI disk\n", MAJOR_NR);
		return -EBUSY;
	}
	
	for (minors_per_major = 0;
	     DEVICE_NR(minors_per_major) == 0 && minors_per_major < MAX_MINORS;
	     minors_per_major++);

	for (i = 0; i < MAX_MINORS; i++) {
		sd_blocksizes[i] = 1024;
		sd_hardsizes[i] = 512;
	}
	blksize_size[MAJOR_NR] = sd_blocksizes;
	hardsect_size[MAJOR_NR] = sd_hardsizes;
	read_ahead[MAJOR_NR] = 256;	/* 256 sector read-ahead */

	ret = osfmach3_register_blkdev(MAJOR_NR,
				       "sd",
				       OSFMACH3_DEV_BSIZE,
				       sd_mach_blocksizes,
				       minors_per_major,
				       gen_disk_dev_to_name,
				       gen_disk_name_to_dev,
				       gen_disk_process_req_for_alias);
	if (ret) {
		printk("sd_init: osfmach3_register_blkdev failed %d\n", ret);
		panic("sd_init: can't register Mach device");
	}
	
	for (i = 0; i < MAX_MINORS; i++) {
		sd_refs[i] = 0;
		if (i % minors_per_major == 0) {
			/*
			 * Minor 0 of a unit is the whole unit.
			 * We don't have that concept in Mach, so just
			 * make an alias to the first real minor in the
			 * partition and hope it works...
			 */
			sd_aliases[i] = MKDEV(MAJOR_NR, i + 1);
		} else {
			sd_aliases[i] = 0;
		}
	}
	osfmach3_blkdev_refs[MAJOR_NR] = sd_refs;
	osfmach3_blkdev_alias[MAJOR_NR] = sd_aliases;

	return 0;
}
