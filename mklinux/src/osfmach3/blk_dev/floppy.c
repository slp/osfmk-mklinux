/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <osfmach3/block_dev.h>
#include <osfmach3/mach3_debug.h>
#include <machine/disk.h>

#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/fd.h>
#include <linux/mm.h>

#ifdef hp_pa
#include <device/disk_status.h>
#endif

#define MAJOR_NR	FLOPPY_MAJOR
#include <linux/blk.h>

#if	CONFIG_OSFMACH3_DEBUG
#define FLOPPY_DEBUG	1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	FLOPPY_DEBUG
int floppy_debug = 0;
#endif	/* FLOPPY_DEBUG */

unsigned int fake_change = 0;

#define N_DRIVE 8

#ifdef hp_pa
#define TYPE(x) 1
#define DRIVE(x) 0
#else
static inline int TYPE(kdev_t x) {
	return  (MINOR(x)>>2) & 0x1f;
}
static inline int DRIVE(kdev_t x) {
	return (MINOR(x)&0x03) | ((MINOR(x)&0x80) >> 5);
}
#endif

/*
 * Maximum disk size (in kilobytes). This default is used whenever the
 * current disk size is unknown.
 * [Now it is rather a minimum]
 */
#define MAX_DISK_SIZE 4 /* 3984*/

/*
 * This struct defines the different floppy types.
 *
 * The 'stretch' tells if the tracks need to be doubled for some
 * types (ie 360kB diskette in 1.2MB drive etc). Others should
 * be self-explanatory.
 */
struct floppy_struct floppy_type[32] = {
	{    0, 0,0, 0,0,0x00,0x00,0x00,0x00,NULL    },	/*  0 no testing    */
#ifdef hp_pa
	{ 2880,18,2,80,0,0x1B,0x00,0xCF,0x6C,"H1440" },	/*  7 1.44MB 3.5"   */
#else
	{  720, 9,2,40,0,0x2A,0x02,0xDF,0x50,"d360"  }, /*  1 360KB PC      */
	{ 2400,15,2,80,0,0x1B,0x00,0xDF,0x54,"h1200" },	/*  2 1.2MB AT      */
	{  720, 9,1,80,0,0x2A,0x02,0xDF,0x50,"D360"  },	/*  3 360KB SS 3.5" */
	{ 1440, 9,2,80,0,0x2A,0x02,0xDF,0x50,"D720"  },	/*  4 720KB 3.5"    */
	{  720, 9,2,40,1,0x23,0x01,0xDF,0x50,"h360"  },	/*  5 360KB AT      */
	{ 1440, 9,2,80,0,0x23,0x01,0xDF,0x50,"h720"  },	/*  6 720KB AT      */
	{ 2880,18,2,80,0,0x1B,0x00,0xCF,0x6C,"H1440" },	/*  7 1.44MB 3.5"   */
	{ 5760,36,2,80,0,0x1B,0x43,0xAF,0x54,"E2880" },	/*  8 2.88MB 3.5"   */
	{ 5760,36,2,80,0,0x1B,0x43,0xAF,0x54,"CompaQ"},	/*  9 2.88MB 3.5"   */

	{ 2880,18,2,80,0,0x25,0x00,0xDF,0x02,"h1440" }, /* 10 1.44MB 5.25"  */
	{ 3360,21,2,80,0,0x1C,0x00,0xCF,0x0C,"H1680" }, /* 11 1.68MB 3.5"   */
	{  820,10,2,41,1,0x25,0x01,0xDF,0x2E,"h410"  },	/* 12 410KB 5.25"   */
	{ 1640,10,2,82,0,0x25,0x02,0xDF,0x2E,"H820"  },	/* 13 820KB 3.5"    */
	{ 2952,18,2,82,0,0x25,0x00,0xDF,0x02,"h1476" },	/* 14 1.48MB 5.25"  */
	{ 3444,21,2,82,0,0x25,0x00,0xDF,0x0C,"H1722" },	/* 15 1.72MB 3.5"   */
	{  840,10,2,42,1,0x25,0x01,0xDF,0x2E,"h420"  },	/* 16 420KB 5.25"   */
	{ 1660,10,2,83,0,0x25,0x02,0xDF,0x2E,"H830"  },	/* 17 830KB 3.5"    */
	{ 2988,18,2,83,0,0x25,0x00,0xDF,0x02,"h1494" },	/* 18 1.49MB 5.25"  */
	{ 3486,21,2,83,0,0x25,0x00,0xDF,0x0C,"H1743" }, /* 19 1.74 MB 3.5"  */

	{ 1760,11,2,80,0,0x1C,0x09,0xCF,0x00,"h880"  }, /* 20 880KB 5.25"   */
	{ 2080,13,2,80,0,0x1C,0x01,0xCF,0x00,"D1040" }, /* 21 1.04MB 3.5"   */
	{ 2240,14,2,80,0,0x1C,0x19,0xCF,0x00,"D1120" }, /* 22 1.12MB 3.5"   */
	{ 3200,20,2,80,0,0x1C,0x20,0xCF,0x2C,"h1600" }, /* 23 1.6MB 5.25"   */
	{ 3520,22,2,80,0,0x1C,0x08,0xCF,0x2e,"H1760" }, /* 24 1.76MB 3.5"   */
	{ 3840,24,2,80,0,0x1C,0x20,0xCF,0x00,"H1920" }, /* 25 1.92MB 3.5"   */
	{ 6400,40,2,80,0,0x25,0x5B,0xCF,0x00,"E3200" }, /* 26 3.20MB 3.5"   */
	{ 7040,44,2,80,0,0x25,0x5B,0xCF,0x00,"E3520" }, /* 27 3.52MB 3.5"   */
	{ 7680,48,2,80,0,0x25,0x63,0xCF,0x00,"E3840" }, /* 28 3.84MB 3.5"   */

	{ 3680,23,2,80,0,0x1C,0x10,0xCF,0x00,"H1840" }, /* 29 1.84MB 3.5"   */
	{ 1600,10,2,80,0,0x25,0x02,0xDF,0x2E,"D800"  },	/* 30 800KB 3.5"    */
	{ 3200,20,2,80,0,0x1C,0x00,0xCF,0x2C,"H1600" }, /* 31 1.6MB 3.5"    */
#endif
};

#define MAX_MINORS	256

/* Auto-detection: Disk type used until the next media change occurs. */
struct floppy_struct *current_type[N_DRIVE] = {
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};

int floppy_sizes[MAX_MINORS];
int floppy_blocksizes[MAX_MINORS] = { 0, };
int floppy_refs[MAX_MINORS];	/* number of refs on Mach devices */
int floppy_mach_blocksizes[MAX_MINORS];
extern char floppy_mach_minor[];

struct file_operations floppy_fops;

extern int machine_floppy_format(kdev_t device,
				 struct format_descr *tmp_format_req);

int floppy_dev_to_name(
	kdev_t	dev,
	char 	*name)
{
	unsigned int major, minor;
	int type;

	major = MAJOR(dev);
	minor = MINOR(dev);
	type = TYPE(dev);

	if (major >= MAX_BLKDEV) {
		return -ENODEV;
	}
	if (osfmach3_blkdevs[major].name == NULL) {
		printk("floppy_dev_to_name: no Mach device for dev %s\n",
		       kdevname(dev));
		panic("floppy_dev_to_name");
	}

	if (type <= 0 || type >= 32)
		return -ENODEV;

	if (floppy_mach_minor[type] == '?')
		return -ENODEV;

	sprintf(name, "%s%d%c",
		osfmach3_blkdevs[major].name,
		DRIVE(dev),
		floppy_mach_minor[type]);

#ifdef	FLOPPY_DEBUG
	if (floppy_debug) {
		printk("floppy_dev_to_name: dev %s name \"%s\"\n",
		       kdevname(dev), name);
	}
#endif	/* FLOPPY_DEBUG */

	return 0;
}

int 
floppy_name_to_dev(
	const char	*name,
	kdev_t		*devp)
{
	unsigned int major;
	unsigned int minor;

	printk("floppy_name_to_dev: not correctly implemented");
	return -ENODEV;

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
	minor = (*name - '0') * osfmach3_blkdevs[major].minors_per_major;

	*devp = MKDEV(major, minor);

	return 0;
}

/*
 * Misc Ioctl's and support
 * ========================
 */
int fd_copyout(void *param, volatile void *address, int size)
{
	int i;

	i = verify_area(VERIFY_WRITE,param,size);
	if (i)
		return i;
	memcpy_tofs(param,(void *) address, size);
	return 0;
}

#define FD_COPYOUT(x) (fd_copyout( (void *)param, &(x), sizeof(x)))
#define FD_COPYIN(x) (memcpy_fromfs( &(x), (void *) param, sizeof(x)),0)

int invalidate_drive(kdev_t rdev)
{
	set_bit(DRIVE(rdev), &fake_change);
	check_disk_change(rdev);
	return 0;
}

int
floppy_ioctl(
	struct inode * inode,
	struct file * filp,
	unsigned int cmd,
	unsigned long param)
{
#define IOCTL_MODE_BIT 8
#define OPEN_WRITE_BIT 16
#define IOCTL_ALLOWED (filp && (filp->f_mode & IOCTL_MODE_BIT))

	kdev_t			device;
	unsigned int		major, minor;
	mach_port_t		device_port;
	int			type, drive;
	struct floppy_struct 	*this_floppy;
	struct format_descr 	tmp_format_req;
#ifdef	V_EJECT
	kern_return_t		kr;
#endif	/* V_EJECT */

	device = inode->i_rdev;
	device = blkdev_get_alias(device);

	major = MAJOR(device);
	minor = MINOR(device);
	if (major >= MAX_BLKDEV) {
		return -ENODEV;
	}

	if (osfmach3_blkdevs[major].name == NULL) {
		panic("floppy_ioctl: no mach info for device %s\n",
		      kdevname(device));
	}

	device_port = bdev_port_lookup(device);
	if (!MACH_PORT_VALID(device_port)) {
		panic("floppy_ioctl: invalid device port for dev %s\n",
		      kdevname(device));
	}

#ifdef	FLOPPY_DEBUG
	if (floppy_debug) {
		printk("floppy_ioctl: dev %s cmd 0x%x\n",
		       kdevname(device), cmd);
	}
#endif	/* FLOPPY_DEBUG */

	switch (cmd) {
		RO_IOCTLS(device,param);
	}
	type = TYPE(device);
	drive = DRIVE(device);
	switch (cmd) {
	    case FDGETPRM:
		if (type)
			this_floppy = &floppy_type[type];
		else if ((this_floppy = current_type[drive]) == NULL)
			return -ENODEV;
		return FD_COPYOUT(this_floppy[0]);
	    case FDGETDRVTYP:
	    case FDGETMAXERRS:
	    case FDPOLLDRVSTAT:
	    case FDGETDRVSTAT:
	    case FDGETFDCSTAT:
	    case FDGETDRVPRM:
	    case FDWERRORGET:
		printk("floppy_ioctl(dev=%s, cmd=0x%x) not implemented\n",
		       kdevname(device), cmd);
		return -EINVAL;
	}
	if (!IOCTL_ALLOWED)
		return -EPERM;
	switch (cmd) {
	    case FDFMTTRK:
		/* XXX check for EBUSY ?? */
		FD_COPYIN(tmp_format_req);
		return machine_floppy_format(device, &tmp_format_req);
	    case FDFMTBEG:
		return 0;
	    case FDFMTEND:
	    case FDFLUSH:
		return invalidate_drive(device);
	    case FDEJECT: 
#ifdef	V_EJECT

		kr = device_set_status(device_port,
				       V_EJECT,
				       (dev_status_t) NULL,
				       0);

		if (kr != D_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("disk_ioctl(%s, V_EJECT): "
				     "device_get_status", kdevname(device)));
			return -EINVAL;
		}
		return 0;
#else	/* V_EJECT */
		/* Drop through */
#endif	/* V_EJECT */		

	    case FDWERRORCLR:
	    case FDRAWCMD:
	    case FDSETMAXERRS:
	    case FDCLRPRM:
	    case FDSETPRM:
	    case FDDEFPRM:
	    case FDRESET:
	    case FDMSGON:
	    case FDMSGOFF:
	    case FDSETEMSGTRESH:
	    case FDTWADDLE:
		printk("floppy_ioctl(dev=%s, cmd=0x%x) not implemented\n",
		       kdevname(device), cmd);
		return -EINVAL;
	}
	if (! suser())
		return -EPERM;
	switch (cmd) {
	    case FDSETDRVPRM:
		printk("floppy_ioctl(dev=%s, cmd=0x%x) not implemented\n",
		       kdevname(device), cmd);
		return -EINVAL;
#ifdef hp_pa
	case DIOCGDINFO:
	{
	        kern_return_t		kr;
		struct disklabel dl;
		unsigned int		count;

		if (!param)
			return -EINVAL;

		kr = verify_area(VERIFY_WRITE, (long *) param,
				    sizeof (struct disklabel));
		if (kr)
			return kr;

		count = sizeof(struct disklabel) / sizeof(int);
		kr = device_get_status(device_port,
				       DIOCGDINFO,
				       (dev_status_t) &dl,
				       &count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("floppy_ioctl(%s, 0x%x): "
				     "device_get_status", kdevname(device), cmd));
			return -EINVAL;
		}

		memcpy_tofs((void*)param, &dl, sizeof(dl));

		return 0;
	}
	case DIOCWDINFO:
	{
	        kern_return_t		kr;
		struct disklabel dl;
		unsigned int		count;

		if (!param)
			return -EINVAL;
		kr = verify_area(VERIFY_READ, (long *) param,
				    sizeof (struct disklabel));
		if (kr)
			return kr;
		count = sizeof(struct disklabel) / sizeof(int);

		memcpy_fromfs(&dl, (void *)param, sizeof(struct disklabel));

		kr = device_set_status(device_port,
				       DIOCWDINFO,
				       (dev_status_t) &dl,
				       count);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("floppy_ioctl(%s, 0x%x): "
				     "device_get_status", kdevname(device), cmd));
			return -EINVAL;
		}

		return 0;
	}
#endif
	    default:
		return -EINVAL;
	}
	return 0;
}

int floppy_open(struct inode * inode, struct file * filp)
{
	int error;

	if (!filp) {
		printk("floppy_open: Weird, open called with filp=0\n");
		return -EIO;
	}

	/* do a generic open first */
	error = blkdev_fops.open(inode, filp);
	if (error)
		return error;

	/* Allow ioctls if we have write-permissions even if read-only open */
	if ((filp->f_mode & 2) || (permission(inode,2) == 0))
		filp->f_mode |= IOCTL_MODE_BIT;
	if (filp->f_mode & 2)
		filp->f_mode |= OPEN_WRITE_BIT;

	/* XXX there's probably more stuff to do here */

	return 0;
}


/*
 * Check if the disk has been changed or if a change has been faked.
 */
int check_floppy_change(kdev_t dev)
{
	int drive = DRIVE( dev );

	if (MAJOR(dev) != MAJOR_NR) {
		printk("floppy_changed: not a floppy\n");
		return 0;
	}

	/* XXX should check for a real disk change here, but how ? */

	if (test_bit(drive, &fake_change))
		return 1;
	return 0;
}

int floppy_revalidate(kdev_t dev)
{
	int drive;

	drive = DRIVE(dev);

	/* XXX should do format autodetection here... */

	clear_bit(drive, &fake_change);

	return 0;
}

int floppy_init(void)
{
	int minors_per_major;
	int i;
	int ret;

	floppy_fops = blkdev_fops;
	floppy_fops.open = floppy_open;
	floppy_fops.ioctl = floppy_ioctl;
	floppy_fops.check_media_change = check_floppy_change;
	floppy_fops.revalidate = floppy_revalidate;
	if (register_blkdev(MAJOR_NR, "fd", &floppy_fops)) {
		printk("Unable to get major %d for floppy\n", MAJOR_NR);
		return -EBUSY;
	}

	for (minors_per_major = 0;
	     DEVICE_NR(minors_per_major) == 0 && minors_per_major < MAX_MINORS;
	     minors_per_major++);

	for (i = 0; i < MAX_MINORS; i++) {
		floppy_refs[i] = 0;
	}
		
	for (i = 0; i < 256; i++) {
		if (TYPE(i))
			floppy_sizes[i] = floppy_type[TYPE(i)].size >> 1;
		else
			floppy_sizes[i] = MAX_DISK_SIZE;
	}

	blk_size[MAJOR_NR] = floppy_sizes;
	blksize_size[MAJOR_NR] = floppy_blocksizes;

	ret = osfmach3_register_blkdev(MAJOR_NR,
				       "fd",
				       OSFMACH3_DEV_BSIZE,
				       floppy_mach_blocksizes,
				       minors_per_major,
				       floppy_dev_to_name,
				       floppy_name_to_dev,
				       NULL);
	if (ret) {
		printk("floppy_init: osfmach3_register_blkdev failed %d\n",
		       ret);
		panic("floppy_init: can't register Mach device");
	}

	osfmach3_blkdev_refs[MAJOR_NR] = floppy_refs;

	return 0;
}
