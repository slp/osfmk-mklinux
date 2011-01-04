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
#include <device/device_request.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/device_reply_hdlr.h>
#include <osfmach3/assert.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/block_dev.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/config.h>
#include <linux/locks.h>
#include <linux/mm.h>

#include <asm/system.h>
#include <asm/io.h>

#if 0	/* not yet */
#define CONFIG_CDU31A
#endif
#define CONFIG_MCD
#if 0	/* not yet */
#define CONFIG_MCDX
#define CONFIG_SBPCD
#define CONFIG_AZTCD
#define CONFIG_CDU535
#define CONFIG_GSCD
#define CONFIG_CM206
#define CONFIG_OPTCD
#define CONFIG_SJCD
#define CONFIG_CDI_INIT
#endif
#define CONFIG_BLK_DEV_HD
#define CONFIG_BLK_DEV_IDE
#define CONFIG_BLK_DEV_XD
#define CONFIG_BLK_DEV_FD
#include <linux/blk.h>

#if	CONFIG_OSFMACH3_DEBUG
#define BLOCKDEV_DEBUG 1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	BLOCKDEV_DEBUG
int blockdev_debug = 0;
#endif	/* BLOCKDEV_DEBUG */

extern void initrd_load(void);

extern int chr_dev_init(void);
extern int blk_dev_init(void);
extern int scsi_dev_init(void);
extern int net_dev_init(void);

/* not exported by the micro-kernel header files !!? */
extern kern_return_t device_read_overwrite(mach_port_t device_port,
					   dev_mode_t mode,
					   recnum_t recnum,
					   io_buf_len_t bytes_wanted,
					   io_buf_ptr_t data,
					   mach_msg_type_number_t *data_count);

struct file_operations blkdev_fops = {
	NULL,			/* lseek - default */
	block_read,		/* read - general block-dev read */
	block_write,		/* write - general block-dev write */
	NULL,			/* readdir - bad */
	NULL,			/* select */
	block_ioctl,		/* ioctl */
	NULL,			/* mmap */
	block_open,		/* open */
	block_release,		/* release */
	block_fsync,		/* fsync */
	NULL,			/* fasync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
};

/*
 * The "disk" task queue is used to start the actual requests
 * after a plug
 */
DECLARE_TASK_QUEUE(tq_disk);

/* This specifies how many sectors to read ahead on the disk.  */

int read_ahead[MAX_BLKDEV] = {0, };

/*
 * blk_size contains the size of all block-devices in units of 1024 byte
 * sectors:
 *
 * blk_size[MAJOR][MINOR]
 *
 * if (!blk_size[MAJOR]) then no minor size checking is done.
 */
int * blk_size[MAX_BLKDEV] = { NULL, NULL, };

/*
 * blksize_size contains the size of all block-devices:
 *
 * blksize_size[MAJOR][MINOR]
 *
 * if (!blksize_size[MAJOR]) then 1024 bytes is assumed.
 */
int * blksize_size[MAX_BLKDEV] = { NULL, NULL, };

/*
 * hardsect_size contains the size of the hardware sector of a device.
 *
 * hardsect_size[MAJOR][MINOR]
 *
 * if (!hardsect_size[MAJOR])
 *		then 512 bytes is assumed.
 * else
 *		sector_size is hardsect_size[MAJOR][MINOR]
 * This is currently set by some scsi device and read by the msdos fs driver
 * This might be a some uses later.
 */
int * hardsect_size[MAX_BLKDEV] = { NULL, NULL, };

/*
 * osfmach3_blkdev_alias[MAJOR][MINOR]: aliases to other devices.
 */
kdev_t *osfmach3_blkdev_alias[MAX_BLKDEV] = { 0 };

kdev_t
blkdev_get_alias(
	kdev_t	dev)
{
	unsigned int minor, major;
	kdev_t alias;

	major = MAJOR(dev);
	minor = MINOR(dev);

	if (major >= MAX_BLKDEV)
		return dev;

	if (osfmach3_blkdev_alias[major]) {
		alias = osfmach3_blkdev_alias[major][minor];
		if (alias) {
#ifdef	BLOCKDEV_DEBUG
			if (blockdev_debug) {
				printk("blkdev_get_alias: %s --> %s\n",
				       kdevname(dev), kdevname(alias));
			}
#endif	/* BLOCKDEV_DEBUG */
			return alias;
		}
	}

	return dev;
}

/* RO fail safe mechanism */

static long ro_bits[MAX_BLKDEV][8];

int is_read_only(kdev_t dev)
{
	int minor,major;

	major = MAJOR(dev);
	minor = MINOR(dev);
	if (major < 0 || major >= MAX_BLKDEV) return 0;
	return ro_bits[major][minor >> 5] & (1 << (minor & 31));
}

void set_device_ro(kdev_t dev,int flag)
{
	int minor,major;

	major = MAJOR(dev);
	minor = MINOR(dev);
	if (major < 0 || major >= MAX_BLKDEV) return;
	if (flag) ro_bits[major][minor >> 5] |= 1 << (minor & 31);
	else ro_bits[major][minor >> 5] &= ~(1 << (minor & 31));
}

/*
 * osfmach3_blkdev_refs[MAJOR][MINOR]: reference count on the Mach device.
 */
int *osfmach3_blkdev_refs[MAX_BLKDEV] = { 0 };

void
blkdev_get_reference(
	kdev_t	dev)
{
	int	minor, major;
	kdev_t	alias_dev;

	alias_dev = blkdev_get_alias(dev);
	if (alias_dev != dev) {
		major = MAJOR(dev);
		minor = MINOR(dev);
		if (osfmach3_blkdev_refs[major]) {
			ASSERT(osfmach3_blkdev_refs[major][minor] >= 0);
			osfmach3_blkdev_refs[major][minor]++;
		}
	}

	major = MAJOR(alias_dev);
	minor = MINOR(alias_dev);
	if (osfmach3_blkdev_refs[major]) {
		ASSERT(osfmach3_blkdev_refs[major][minor] >= 0);
		osfmach3_blkdev_refs[major][minor]++;
	}
}

void
blkdev_release_reference(
	kdev_t	dev)
{
	int		major, minor;
	kdev_t		alias_dev;
	mach_port_t	device_port;
	kern_return_t	kr;

	alias_dev = blkdev_get_alias(dev);
	if (alias_dev != dev) {
		major = MAJOR(dev);
		minor = MINOR(dev);
		if (osfmach3_blkdev_refs[major]) {
			ASSERT(osfmach3_blkdev_refs[major][minor] > 0);
			if (--osfmach3_blkdev_refs[major][minor] == 0) {
				/* device no longer in use: flush everything */
				fsync_dev(dev);
				invalidate_inodes(dev);
				invalidate_buffers(dev);
			}
		}
		dev = alias_dev;
	}
	major = MAJOR(dev);
	minor = MINOR(dev);
	if (osfmach3_blkdev_refs[major]) {
		ASSERT(osfmach3_blkdev_refs[major][minor] > 0);
		if (--osfmach3_blkdev_refs[major][minor] > 0) {
			return;
		}
		/* device no longer in use: flush everything */
		fsync_dev(dev);
		invalidate_inodes(dev);
		invalidate_buffers(dev);
	}

#ifdef	BLOCKDEV_DEBUG
	if (blockdev_debug) {
		printk("blkdev_release_reference: closing dev %s\n",
		       kdevname(dev));
	}
#endif	/* BLOCKDEV_DEBUG */

	device_port = bdev_port_lookup(dev);
	ASSERT(MACH_PORT_VALID(device_port));

	/*
	 * Close the Mach device, deregister it and destroy the device port.
	 */
	kr = device_close(device_port);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(0, kr, ("blkdev_release_reference: device_close"));
		panic("blkdev_release_reference: device_close failed");
	}

	bdev_port_deregister(dev);

	kr = mach_port_destroy(mach_task_self(), device_port);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("blkdev_release_reference: mach_port_destroy"));
		panic("blkdev_release_reference: can't destroy device port");
	}
}

int
osfmach3_check_blocksize(
	kdev_t	dev,
	int	size)
{
	int major, minor;

	major = MAJOR(dev);
	minor = MINOR(dev);

	if (major == RAMDISK_MAJOR) {
		/* no Mach device for RamDisk */
		return 0;
	}

#ifdef	BLOCKDEV_DEBUG
	{
		int mach_blksize;
		mach_blksize = osfmach3_blkdevs[major].block_sizes[minor];
		if (mach_blksize > size) {
			printk("osfmach3_check_blocksize(dev %s): "
			       "using blocksize %d - min is %d\n",
			       kdevname(dev), size, mach_blksize);
		}
	}
#endif
	return 0;
}

void
blkdev_mach_blocknr(
	kdev_t		dev,
	unsigned long	linux_blocknr,
	unsigned long	linux_size,
	unsigned long	*mach_blocknr_p,
	unsigned long	*mach_size_p,
	unsigned long	*mach_blocksize_p)
{
	unsigned long linux_blksize, mach_blksize;
	unsigned long mach_blocknr;
	int major, minor;

	major = MAJOR(dev);
	minor = MINOR(dev);

	/*
	 * Don't use aliasing here, cause we're supposed to access
	 * the original device, using its block size, not the block_size
	 * of the aliased device.
	 */

	linux_blksize = BLOCK_SIZE;
	if (blksize_size[major] &&
	    blksize_size[major][minor]) {
		linux_blksize = blksize_size[major][minor];
	}

	/*
	 * The blocknr is now (linux-2.0) a 512-byte-sector number.
	 * Convert it back into a block nr.
	 */
	linux_blocknr = (linux_blocknr * 512) / linux_blksize;

	mach_blksize = osfmach3_blkdevs[major].block_sizes[minor];
	if (mach_blksize == 0) {
		panic("blkdev_mach_blocknr: no Mach block_size for dev %s\n",
		      kdevname(dev));
	}

#if 0
	/*
	 * If the Mach blocksize is bigger than the Linux blocksize,
	 * we'll have some rounding problems...
	 */
	if (mach_blksize > linux_blksize) {
		printk("blkdev_mach_blocknr(dev %s): "
		       "mach_blksize (%ld) > linux_blksize (%ld).\n",
		       kdevname(dev), mach_blksize, linux_blksize);
		return ~0;
	}
#endif

	mach_blocknr = (linux_blocknr * linux_blksize) / mach_blksize;

	*mach_blocknr_p = mach_blocknr;
	*mach_size_p = (((linux_blocknr * linux_blksize + linux_size - 1)
			 / mach_blksize) - mach_blocknr + 1) * mach_blksize;
	*mach_blocksize_p = mach_blksize;
}

static inline void drive_stat_acct(int cmd, unsigned long nr_sectors,
                                   short disk_index)
{
	kstat.dk_drive[disk_index]++;
	if (cmd == READ) {
		kstat.dk_drive_rio[disk_index]++;
		kstat.dk_drive_rblk[disk_index] += nr_sectors;
	} else if (cmd == WRITE) {
		kstat.dk_drive_wio[disk_index]++;
		kstat.dk_drive_wblk[disk_index] += nr_sectors;
	} else
		printk(KERN_ERR "drive_stat_acct: cmd not R/W?\n");
}

void make_request(int major, int rw, struct buffer_head * bh)
{
	unsigned int	sector, count;
	int 		rw_ahead;
	mach_port_t	device_port;
	kern_return_t	kr;
	kdev_t		dev;
	unsigned long	recnum, mach_size, mach_blocksize;
	short		disk_index;
	char		*data_addr;

	count = bh->b_size >> 9;
	sector = bh->b_rsector;

	/* Uhhuh.. Nasty dead-lock possible here.. */
	if (buffer_locked(bh)) {
#ifdef	BLOCKDEV_DEBUG
#if 0
		printk("make_request(0x%x, bh=%p): "
		       "bh (rdev=%s,rsector=0x%lx) is locked\n",
		       rw,
		       bh,
		       kdevname(bh->b_rdev),
		       bh->b_rsector);
#endif
#endif	/* BLOCKDEV_DEBUG */
		return;
	}
	/* Maybe the above fixes it, and maybe it doesn't boot. Life is interesting */

	lock_buffer(bh);

	/*
	 * Prepare a request to the micro-kernel
	 */

	dev = bh->b_rdev;
	dev = blkdev_get_alias(dev);

#ifdef	CONFIG_BLK_DEV_RAM
	if (MAJOR(dev) == RAMDISK_MAJOR) {
		/* no Mach device for RamDisk */
		device_port = MACH_PORT_DEAD;
		data_addr = NULL;
	} else
#endif	/* CONFIG_BLK_DEV_RAM */
	{
		device_port = bdev_port_lookup(dev);
		ASSERT(MACH_PORT_VALID(device_port));

		if (bh->b_reply_port == MACH_PORT_NULL) {
			device_reply_register(&bh->b_reply_port,
					      (char *) bh,
					      block_read_reply,
					      block_write_reply);
		}
		ASSERT(MACH_PORT_VALID(bh->b_reply_port));

		if (dev != bh->b_rdev &&
		    (osfmach3_blkdevs[MAJOR(dev)].process_req_for_alias
		     != NULL)) {
			/*
			 * Request for an alias to another device. 
			 * Let the device specific routine handle it.
			 */
			if (osfmach3_blkdevs[MAJOR(dev)].process_req_for_alias(
							rw,
							bh,
							device_port) == 0) {
				/* the request has been processed: we're done */
				return;
			}
			/*
			 * If the request hasn't been processed
			 * by the special-purpose routine,
			 * fall back to the normal case.
			 */
		}

		/*
		 * Use the original device (not the alias) to compute the
		 * block number conversion.
		 */
		blkdev_mach_blocknr(dev, sector, bh->b_size,
				    &recnum, &mach_size, &mach_blocksize);
		if (mach_size != bh->b_size) {
			/*
			 * Reading more than requested:
			 * can't overwrite bh->b_data.
			 */
			if (rw != READ && rw != READA) {
				/*
				 * Can't write more than requested yet !!!
				 */
				recnum = ~0;
			}
			data_addr = NULL;
		} else {
			/*
			 * Read data directly into the buffer.
			 */
			data_addr = bh->b_data;
		}
		if (recnum == ~0) {
			/*
			 * Conversion error: abort the request.
			 */
			switch (rw) {
			case READ:
			case READA:
				block_read_reply((char *) bh,
						 KERN_FAILURE, NULL, 0);
				break;
			case WRITE:
			case WRITEA:
				block_write_reply((char *) bh,
						  KERN_FAILURE, NULL, 0);
				break;
			}
			return;
		}
	}

	if (blk_size[major])
		if (blk_size[major][MINOR(bh->b_rdev)] < (sector + count)>>1) {
			bh->b_state &= (1 << BH_Lock) | (1 << BH_FreeOnIO);
                        /* This may well happen - the kernel calls bread()
                           without checking the size of the device, e.g.,
                           when mounting a device. */
			printk(KERN_INFO
                               "attempt to access beyond end of device\n");
			printk(KERN_INFO "%s: rw=%d, want=%d, limit=%d\n",
                               kdevname(bh->b_rdev), rw,
                               (sector + count)>>1,
                               blk_size[major][MINOR(bh->b_rdev)]);
			unlock_buffer(bh);
			return;
		}

	rw_ahead = 0;	/* normal case; gets changed below for READA/WRITEA */
	switch (rw) {
		case READA:
			rw_ahead = 1;
			rw = READ;	/* drop into READ */
		case READ:
			if (buffer_uptodate(bh)) {
				unlock_buffer(bh); /* Hmmph! Already have it */
				return;
			}
			kstat.pgpgin++;
			break;
		case WRITEA:
			rw_ahead = 1;
			rw = WRITE;	/* drop into WRITE */
		case WRITE:
			if (!buffer_dirty(bh)) {
				unlock_buffer(bh); /* Hmmph! Nothing to write */
				return;
			}
			kstat.pgpgout++;
			break;
		default:
			printk(KERN_ERR "make_request: bad block dev cmd,"
                               " must be R/W/RA/WA\n");
			unlock_buffer(bh);
			return;
	}

	/* from add_request() ... */
	switch (MAJOR(dev)) {
	    case SCSI_DISK_MAJOR:
		disk_index = (MINOR(dev) & 0x0070) >> 4;
		if (disk_index < 4)
			drive_stat_acct(rw, count, disk_index);
		break;
	    case IDE0_MAJOR:	/* same as HD_MAJOR */
	    case XT_DISK_MAJOR:
		disk_index = (MINOR(dev) & 0x0040) >> 6;
		drive_stat_acct(rw, count, disk_index);
		break;
	    case IDE1_MAJOR:
		disk_index = ((MINOR(dev) & 0x0040) >> 6) + 2;
		drive_stat_acct(rw, count, disk_index);
	    default:
		break;
	}

#if	CONFIG_BLK_DEV_RAM
	if (MAJOR(dev) == RAMDISK_MAJOR) {
		/* code adapted from rd_request() */
		if (rw == READ)
			memset(bh->b_data, 0, bh->b_size);
		else
			set_bit(BH_Protected, &bh->b_state);

		mark_buffer_uptodate(bh, 1);
		mark_buffer_clean(bh);
#if 0
		clear_bit(BH_Lock, &bh->b_state);
#else
		unlock_buffer(bh);
#endif
	} else
#endif	/* CONFIG_BLK_DEV_RAM */
	{
		switch (rw) {
		case READ:
			ASSERT(!buffer_dirty(bh));
			ASSERT(!buffer_uptodate(bh));
#ifdef	BLOCKDEV_DEBUG
			if (blockdev_debug) {
				printk("make_request: "
				       "READ%s data 0x%p blk 0x%lx size %ld "
				       "recnum 0x%lx size %ld dev %s\n",
				       rw_ahead ? "A" : "",
				       bh->b_data, bh->b_rsector, bh->b_size,
				       recnum, mach_size, kdevname(dev));
			}
#endif	/* BLOCKDEV_DEBUG */
			server_thread_blocking(FALSE);
			kr = serv_device_read_async(device_port,
						    bh->b_reply_port,
						    D_NOWAIT,
						    recnum,
						    data_addr,
						    mach_size,
						    FALSE);
			server_thread_unblocking(FALSE);
			if (kr != D_SUCCESS) {
				block_read_reply((char *) bh, kr, NULL, 0);
			}
			break;
		case WRITE:
			ASSERT(buffer_dirty(bh));
			ASSERT(buffer_uptodate(bh));
#ifdef	BLOCKDEV_DEBUG
			if (blockdev_debug) {
				printk("make_request: "
				       "WRITE data 0x%p blk 0x%lx size 0x%lx "
				       "recnum 0x%lx size %ld dev %s\n",
				       bh->b_data, bh->b_rsector, bh->b_size,
				       recnum, mach_size, kdevname(dev));
			}
#endif	/* BLOCKDEV_DEBUG */
			server_thread_blocking(FALSE);
			kr = serv_device_write_async(device_port,
						     bh->b_reply_port,
						     D_NOWAIT,
						     recnum,
						     (io_buf_ptr_t) bh->b_data,
						     bh->b_size,
						     FALSE);
			server_thread_unblocking(FALSE);
			if (kr != D_SUCCESS) {
				block_write_reply((char *) bh, kr, NULL, 0);
			}
			break;
		}
	}
}

/* This function can be used to request a number of buffers from a block
   device. Currently the only restriction is that all buffers must belong to
   the same device */

void ll_rw_block(int rw, int nr, struct buffer_head * bh[])
{
	unsigned int major;
	int correct_size;
	int i;

	/* Make sure that the first block contains something reasonable */
	while (!*bh) {
		bh++;
		if (--nr <= 0)
			return;
	};

#ifdef	CONFIG_BLK_DEV_RAM
	if (MAJOR(bh[0]->b_dev) == RAMDISK_MAJOR) {
		/* no Mach device for RamDisk */
		major = MAJOR(bh[0]->b_dev);
	} else
#endif	/* CONFIG_BLK_DEV_RAM */
	if ((major = MAJOR(bh[0]->b_dev)) >= MAX_BLKDEV ||
	    osfmach3_blkdevs[major].name == NULL) {
		printk(KERN_ERR 
		       "ll_rw_block: Trying to read nonexistent block-device "
		       "%s (%ld)\n",
		       kdevname(bh[0]->b_dev), bh[0]->b_blocknr);
		goto sorry;
	}

	/* Determine correct block size for this device.  */
	correct_size = BLOCK_SIZE;
	if (blksize_size[major]) {
		i = blksize_size[major][MINOR(bh[0]->b_dev)];
		if (i)
			correct_size = i;
	}

	/* Verify requested block sizes.  */
	for (i = 0; i < nr; i++) {
		if (bh[i] && bh[i]->b_size != correct_size) {
			printk(KERN_NOTICE "ll_rw_block: device %s: "
			       "only %d-char blocks implemented (%lu)\n",
			       kdevname(bh[0]->b_dev),
			       correct_size, bh[i]->b_size);
			goto sorry;
		}

		/* Md remaps blocks now */
		bh[i]->b_rdev = bh[i]->b_dev;
		bh[i]->b_rsector=bh[i]->b_blocknr*(bh[i]->b_size >> 9);
#ifdef CONFIG_BLK_DEV_MD
		if (major==MD_MAJOR &&
		    md_map (MINOR(bh[i]->b_dev), &bh[i]->b_rdev,
			    &bh[i]->b_rsector, bh[i]->b_size >> 9)) {
		        printk (KERN_ERR
				"Bad md_map in ll_rw_block\n");
		        goto sorry;
		}
#endif
	}

	if ((rw == WRITE || rw == WRITEA) && is_read_only(bh[0]->b_dev)) {
		printk(KERN_NOTICE "Can't write to read-only device %s\n",
		       kdevname(bh[0]->b_dev));
		goto sorry;
	}

	for (i = 0; i < nr; i++) {
		if (bh[i]) {
			set_bit(BH_Req, &bh[i]->b_state);

			make_request(MAJOR(bh[i]->b_rdev), rw, bh[i]);
		}
	}

	return;

      sorry:
	for (i = 0; i < nr; i++) {
		if (bh[i]) {
			clear_bit(BH_Dirty, &bh[i]->b_state);
			clear_bit(BH_Uptodate, &bh[i]->b_state);
		}
	}
	return;
}

int blk_dev_init(void)
{
	memset(ro_bits,0,sizeof(ro_bits));
#ifdef CONFIG_BLK_DEV_RAM
	rd_init();
#endif
#ifdef CONFIG_BLK_DEV_LOOP
	loop_init();
#endif
#ifdef CONFIG_CDI_INIT
	cdi_init();		/* this MUST precede ide_init */
#endif CONFIG_CDI_INIT
#ifdef CONFIG_BLK_DEV_IDE
	ide_init();		/* this MUST precede hd_init */
#endif
#ifdef CONFIG_BLK_DEV_HD
	hd_init();
#endif
#ifdef CONFIG_BLK_DEV_XD
	xd_init();
#endif
#ifdef CONFIG_BLK_DEV_FD
	floppy_init();
#endif
#ifdef CONFIG_CDU31A
	cdu31a_init();
#endif CONFIG_CDU31A
#ifdef CONFIG_MCD
	mcd_init();
#endif CONFIG_MCD
#ifdef CONFIG_MCDX
	mcdx_init();
#endif CONFIG_MCDX
#ifdef CONFIG_SBPCD
	sbpcd_init();
#endif CONFIG_SBPCD
#ifdef CONFIG_AZTCD
        aztcd_init();
#endif CONFIG_AZTCD
#ifdef CONFIG_CDU535
	sony535_init();
#endif CONFIG_CDU535
#ifdef CONFIG_GSCD
	gscd_init();
#endif CONFIG_GSCD
#ifdef CONFIG_CM206
	cm206_init();
#endif
#ifdef CONFIG_OPTCD
	optcd_init();
#endif CONFIG_OPTCD
#ifdef CONFIG_SJCD
	sjcd_init();
#endif CONFIG_SJCD
#ifdef CONFIG_BLK_DEV_MD
	md_init();
#endif CONFIG_BLK_DEV_MD

	return 0;
}

void device_setup(void)
{
	extern void console_map_init(void);

	chr_dev_init();
	blk_dev_init();
	scsi_dev_init();
#ifdef	CONFIG_INET
	net_dev_init();
#endif
	console_map_init();
#ifdef	CONFIG_BLK_DEV_RAM
#ifdef	CONFIG_BLK_DEV_INITRD
	if (initrd_start && mount_initrd) initrd_load();
	else
#endif
	rd_load();
#endif
}

/*
 * SCSI devices
 */
extern void sd_init(void);
extern void sr_init(void);
extern void st_init(void);
extern void sg_init(void);

extern void osfmach3_scsi_procfs_init(void);

unsigned int scsi_init(void)
{
	static int called = 0;

	if (called)
		return 0;
	called = 1;

	osfmach3_scsi_procfs_init();

	sd_init();
	sr_init();
	st_init();
	sg_init();

	return 0;
}

int scsi_dev_init(void)
{
#if 0
	/* Register the /proc/scsi/scsi entry */
#if CONFIG_PROC_FS
	proc_scsi_register(0, &proc_scsi_scsi);
#endif
#endif

	/* initialize all hosts */
	scsi_init();

	return 0;
}

struct osfmach3_blkdev_struct osfmach3_blkdevs[MAX_BLKDEV] = {
	{ NULL, 	/* name */
	  0,		/* default_block_size */
	  NULL,		/* block_sizes[] */
	  0,		/* minors_per_major */
	  NULL,		/* dev_to_name */
	  NULL,		/* name_to_dev */
	  NULL		/* process_req_for_alias */
	},
};
struct semaphore osfmach3_blkdevs_sem = MUTEX;

int
osfmach3_register_blkdev(
	unsigned int major,
	const char *name,
	int default_block_size,
	int *block_sizes,
	int minors_per_major,
	int (*dev_to_name)(kdev_t dev, char *name),
	int (*name_to_dev)(const char *name, kdev_t *devp),
	int (*process_req_for_alias)(int rw,
				     struct buffer_head *bh,
				     mach_port_t device_port))
{
	if (major == 0 || major >= MAX_BLKDEV) {
		return -EINVAL;
	}
	if (osfmach3_blkdevs[major].name &&
	    strcmp(name, osfmach3_blkdevs[major].name) != 0) {
		return -EBUSY;
	}
	if (block_sizes == NULL) {
		/*
		 * The block_sizes array is MANDATORY.
		 */
		printk("osfmach3_register_blkdev(maj=%d): NULL block_sizes\n",
		       major);
		return -EINVAL;
	}
		       
	osfmach3_blkdevs[major].name = name;
	osfmach3_blkdevs[major].default_block_size = default_block_size;
	osfmach3_blkdevs[major].block_sizes = block_sizes;
	osfmach3_blkdevs[major].minors_per_major = minors_per_major;
	osfmach3_blkdevs[major].dev_to_name = dev_to_name;
	osfmach3_blkdevs[major].name_to_dev = name_to_dev;
	osfmach3_blkdevs[major].process_req_for_alias = process_req_for_alias;

	return 0;
}

int
block_dev_to_name(
	kdev_t	dev,
	char	*name)
{
	unsigned int	major, minor;
	int		ret;

	dev = blkdev_get_alias(dev);

	major = MAJOR(dev);
	minor = MINOR(dev);
	if (major >= MAX_BLKDEV) {
		return -ENODEV;
	}

	if (osfmach3_blkdevs[major].name == NULL) {
		panic("block_dev_to_name: no Mach info for dev %s\n",
		      kdevname(dev));
	}

	ret = osfmach3_blkdevs[major].dev_to_name(dev, name);

	return ret;
}

int
block_dev_to_mach_dev(
	kdev_t		dev,
	mach_port_t	*device_portp)
{
	unsigned int major;

	dev = blkdev_get_alias(dev);

	major = MAJOR(dev);
	if (major >= MAX_BLKDEV)
		return -ENODEV;
	if (osfmach3_blkdevs[major].name == NULL) {
		panic("block_dev_to_mach_dev: no mach info for device %s\n",
		      kdevname(dev));
	}

	*device_portp = bdev_port_lookup(dev);
	if (*device_portp == MACH_PORT_NULL) 
		return -ENODEV;

	return 0;
}

int block_ioctl(struct inode * inode, struct file * file,
	unsigned int cmd, unsigned long arg)
{
	printk("block_ioctl(dev=%s, cmd=0x%x) not implemented\n",
	       kdevname(inode->i_rdev), cmd);
	return -EINVAL;
}

int block_open(struct inode * inode, struct file * filp)
{
	kdev_t		dev, real_dev;
	unsigned int	major, minor;
	char		name[16];
	mach_port_t	device_port;
	kern_return_t	kr;
	int		ret;
	int		dev_mode;
	int		dev_status[DEV_GET_SIZE_COUNT];
	unsigned int	count;
#if 0
	int		mach_blksize, linux_blksize;
#endif
	extern		int	hd_is_cdrom(int major, int minor);

	real_dev = inode->i_rdev;
	dev = blkdev_get_alias(real_dev);

	major = MAJOR(dev);
	minor = MINOR(dev);
	if (major >= MAX_BLKDEV) {
		return -ENODEV;
	}

	if (osfmach3_blkdevs[major].name == NULL) {
		panic("block_open: no mach info for device %s\n",
		      kdevname(real_dev));
	}

	/*
	 * Critical section: avoid doing more than one device_open
	 * at a time, to avoid race conditions leading to multiply-opened
	 * Mach devices.
	 */
	down(&osfmach3_blkdevs_sem);

	/*
	 * See whether we have already opened the device.
	 */
	device_port = bdev_port_lookup(dev);
	if (device_port != MACH_PORT_NULL) {
		ASSERT(MACH_PORT_VALID(device_port));
		blkdev_get_reference(real_dev);
		up(&osfmach3_blkdevs_sem);
		return 0;
	}

	ret = block_dev_to_name(dev, name);
	if (ret != 0) {
		up(&osfmach3_blkdevs_sem);
		return -ENODEV;
	}

#ifdef	BLOCKDEV_DEBUG
	if (blockdev_debug) {
		printk("block_open: dev %s name \"%s\"\n",
		       kdevname(real_dev), name);
	}
#endif	/* BLOCKDEV_DEBUG */

	if (major == SCSI_CDROM_MAJOR || hd_is_cdrom(major, minor)) {
		/* fix modes */
		if (filp->f_flags & O_NONBLOCK)
	  		dev_mode = D_READ | D_NODELAY;
	  	else
	  		dev_mode = D_READ;
	}
	else {
		dev_mode = D_READ | D_WRITE;
	}

	/*
	 * Open the Mach device: release the master lock since
	 * this operation can take some time if the device is
	 * a removable and is not yet online (CD-ROMs, etc...).
	 */
	server_thread_blocking(FALSE);
	kr = device_open(device_server_port,
			 MACH_PORT_NULL,
			 dev_mode,
			 server_security_token,
			 name,
			 &device_port);
	server_thread_unblocking(FALSE);
	if (kr != D_SUCCESS) {
		if (kr == D_NO_SUCH_DEVICE) {
			/* nothing */
		} else if (kr == D_DEVICE_DOWN) {
			printk("block_open(dev %s): "
			       "Mach device \"%s\" is down\n",
			       kdevname(real_dev), name);
		} else {
			MACH3_DEBUG(1, kr,
				    ("block_open(%s): device_open(\"%s\")",
				     kdevname(real_dev), name));
		}
		up(&osfmach3_blkdevs_sem);
		return -ENODEV;
	}

	/*
	 * Get the device block size.
	 */
	count = DEV_GET_SIZE_COUNT;
	kr = device_get_status(device_port,
			       DEV_GET_SIZE,
			       (dev_status_t) dev_status,
			       &count);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("block_open(%s): device_get_status "
			     "(\"%s\",DEV_GET_SIZE)",
			     kdevname(real_dev), name));
		printk("block_open(%s): can't get block_size for "
		       "Mach device \"%s\". Assuming default %d.\n",
		       kdevname(real_dev), name,
		       osfmach3_blkdevs[major].default_block_size);
		osfmach3_blkdevs[major].block_sizes[minor] = 
			osfmach3_blkdevs[major].default_block_size;
		if (blk_size[major]) {
			blk_size[major][minor] = INT_MAX;
		}
	} else {
		osfmach3_blkdevs[major].block_sizes[minor] =
			dev_status[DEV_GET_SIZE_RECORD_SIZE];
		if (blk_size[major]) {
			/* Mach returns the device size in bytes */
			blk_size[major][minor] = 
				dev_status[DEV_GET_SIZE_DEVICE_SIZE] / 1024;
		}
	}
#if 0
	mach_blksize = osfmach3_blkdevs[major].block_sizes[minor];
	linux_blksize = BLOCK_SIZE;
	if (blksize_size[major] &&
	    blksize_size[major][minor]) {
		linux_blksize = blksize_size[major][minor];
	}
	if (mach_blksize > linux_blksize) {
		/*
		 * Problem: we won't be able to read as small a
		 * block as "linux_blksize" if Mach's blocksize is
		 * bigger.
		 * Force Linux's blocksize to Mach's.
		 */
		if (blksize_size[major]) {
			printk("block_open(%s): "
			       "forced Linux block size to %d (was %d)\n",
			       kdevname(real_dev), mach_blksize, linux_blksize);
			blksize_size[major][minor] = mach_blksize;
		} else {
			/* can't force the block size: abort the open */
			printk("block_open: "
			       "can't open device %s with blocksize %d "
			       "(min %d)\n",
			       kdevname(real_dev), linux_blksize, mach_blksize);
			kr = device_close(device_port);
			if (kr != D_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("block_open(%s): "
					     "device_close(0x%x)",
					     kdevname(real_dev), device_port));
			}
			up(&osfmach3_blkdevs_sem);
			return -ENODEV;
		}
	}
#endif
	
	ASSERT(osfmach3_blkdev_refs[major] == 0 ||
	       osfmach3_blkdev_refs[major][minor] == 0);
	blkdev_get_reference(real_dev);
	bdev_port_register(dev, device_port);

	up(&osfmach3_blkdevs_sem);	/* end of critical section */

	return 0;
}

void block_release(struct inode * inode, struct file * file)
{
	kdev_t		dev, real_dev;
	unsigned int	major, minor;
	mach_port_t	device_port;

	real_dev = inode->i_rdev;
	dev = blkdev_get_alias(real_dev);

	major = MAJOR(dev);
	minor = MINOR(dev);
	if (major >= MAX_BLKDEV) {
		panic("block_release(dev=%s): bad device\n", kdevname(dev));
	}

	if (osfmach3_blkdev_refs[major] &&
	    osfmach3_blkdev_refs[major][minor] > 1) {
		/* device is still in use */
		sync_dev(dev);
	}

	if (osfmach3_blkdevs[major].name == NULL) {
		panic("block_release: no mach info for device %s\n",
		      kdevname(dev));
	}

	device_port = bdev_port_lookup(dev);
	if (device_port == MACH_PORT_NULL) {
		panic("block_release: no mach device port for dev %s\n",
		      kdevname(dev));
	}

	blkdev_release_reference(real_dev);
}

kern_return_t
block_request_reply(
	char			*caller,
	char			*bp_handle,
	kern_return_t		return_code,
	char			*data,
	unsigned int		data_count)
{
	struct buffer_head	*bp;

	bp = (struct buffer_head *) bp_handle;

	if (return_code != D_SUCCESS) {
		/*
		 * D_INVALID_RECNUM is acceptable error created due to
		 * request beyond end of disk.
		 * D_INVALID_SIZE is questionnable: it can occur if a
		 * partition has some extra disk blocks that don't make
		 * a full "logical" block...
		 */
		if (return_code != D_INVALID_RECNUM &&
		    return_code != D_INVALID_SIZE) {
			MACH3_DEBUG(2, return_code,
				    ("%s: rdev %s rsector 0x%lx size 0x%lx",
				     caller,
				     kdevname(bp->b_rdev),
				     bp->b_rsector, bp->b_size));
		}
		mark_buffer_uptodate(bp, 0);
		clear_bit(BH_Req, &bp->b_state);
	} else {
		mark_buffer_uptodate(bp, 1);
		mark_buffer_clean(bp);
	}

	ASSERT(bp->b_reqnext == NULL);
#ifdef	BLOCKDEV_DEBUG
	if (blockdev_debug) {
		printk("%s: GOT data 0x%p rsector 0x%lx size %ld rdev %s %s\n",
		       caller,
		       bp->b_data, bp->b_rsector, bp->b_size,
		       kdevname(bp->b_rdev),
		       buffer_uptodate(bp) ? "OK" : "ERROR");
	}
#endif	/* BLOCKDEV_DEBUG */

	if (return_code == D_SUCCESS && data && data != bp->b_data) {
		unsigned long mach_blocknr;
		unsigned long mach_blocksize;
		unsigned long mach_size;
		kern_return_t kr;

		/*
		 * We read more than requested. Copy what was requested
		 * and discard the rest.
		 */
		blkdev_mach_blocknr(bp->b_rdev, bp->b_rsector, bp->b_size,
				    &mach_blocknr, &mach_size, &mach_blocksize);
		ASSERT(mach_blocknr != ~0);
#ifdef	BLOCKDEV_DEBUG
		if (blockdev_debug) {
			printk("%s: COPY from 0x%p to 0x%p size %ld\n",
			       caller, data, bp->b_data, bp->b_size);
		}
#endif	/* BLOCKDEV_DEBUG */
		memcpy(bp->b_data,
		       (data
			+ (bp->b_rsector << 9)
			- (mach_blocknr * mach_blocksize)),
		       bp->b_size);
		kr = vm_deallocate(mach_task_self(),
				   (vm_offset_t) data,
				   data_count);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("block_request_reply: "
				     "vm_deallocate(0x%x,0x%x)",
				     (vm_offset_t) data, data_count));
		}
	}

	unlock_buffer(bp);

	return KERN_SUCCESS;
}

kern_return_t
block_read_reply(
	char			*bp_handle,
	kern_return_t		return_code,
	char			*data,
	unsigned int		data_count)
{
	return block_request_reply("block_read_reply",
				   bp_handle,
				   return_code,
				   data,
				   data_count);
}

kern_return_t
block_write_reply(
	char			*bp_handle,
	kern_return_t		return_code,
	char			*data,		/* irrelevant */
	unsigned int		data_count)
{
	return block_request_reply("block_write_reply",
				   bp_handle,
				   return_code,
				   data,
				   data_count);
}
