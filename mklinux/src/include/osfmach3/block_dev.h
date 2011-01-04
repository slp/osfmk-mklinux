/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_BLOCK_DEV_H
#define _OSFMACH3_BLOCK_DEV_H

#include <osfmach3/device_utils.h>

#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/hdreg.h>

extern struct file_operations blkdev_fops;

#if	i386
#define OSFMACH3_DEV_BSIZE 	512	/* XXX should be exported by Mach */
#else	/* i386 */
#ifdef	PPC
#define OSFMACH3_DEV_BSIZE	512	/* XXX should be exported by Mach */
#else	/* PPC */
#ifdef hp_pa
#define OSFMACH3_DEV_BSIZE	512
#endif
#endif
#endif

struct osfmach3_blkdev_struct {
	const char 	*name;
	int		default_block_size;
	int		*block_sizes;
	int 		minors_per_major;
	int (*dev_to_name)(kdev_t dev,
			   char *name);
	int (*name_to_dev)(const char *name,
			   kdev_t *devp);
	int (*process_req_for_alias)(int rw,
				     struct buffer_head *bh,
				     mach_port_t device_port);
};

extern struct osfmach3_blkdev_struct osfmach3_blkdevs[];
extern int *osfmach3_blkdev_refs[];
extern kdev_t *osfmach3_blkdev_alias[];

#define bdev_port_register(dev, port) \
	dev_to_object_register((dev), S_IFBLK, (char *) (port))
#define bdev_port_lookup(dev) \
	((mach_port_t) dev_to_object_lookup((dev), S_IFBLK))
#define bdev_port_deregister(dev) \
	dev_to_object_deregister((dev), S_IFBLK)

extern int block_dev_to_name(kdev_t dev, char *name);
extern int block_dev_to_mach_dev(kdev_t, mach_port_t *device_portp);
extern kdev_t blkdev_get_alias(kdev_t dev);
extern void blkdev_mach_blocknr(kdev_t dev,
				unsigned long linux_blocknr,
				unsigned long linux_size,
				unsigned long *mach_blocknr_p,
				unsigned long *mach_size_p,
				unsigned long *mach_blocksize_p);
extern kern_return_t block_read_reply(char *bp_handle,
				      kern_return_t return_code, 
				      char *data,
				      unsigned int data_count);
extern kern_return_t block_write_reply(char *bp_handle,
				       kern_return_t return_code, 
				       char *data,
				       unsigned int data_count);
extern int block_ioctl(struct inode *inode,
		       struct file *file,
		       unsigned int cmd,
		       unsigned long arg);
extern int block_open(struct inode * inode,
		      struct file * filp);
extern void block_release(struct inode * inode,
			  struct file * file);

extern int osfmach3_register_blkdev(unsigned int major,
				    const char *name,
				    int default_block_size,
				    int *block_sizes,
				    int minors_per_major,
				    int (*dev_to_name)(kdev_t dev,
						       char *name),
				    int (*name_to_dev)(const char *name,
						       kdev_t *devp),
				    int (*process_req_for_alias)(
						int rw,
						struct buffer_head *bh,
						mach_port_t device_port));
						       
extern int gen_disk_dev_to_name(kdev_t dev, char *name);
extern int gen_disk_name_to_dev(const char *name, kdev_t *devp);
extern int gen_disk_ioctl(struct inode *inode,
			  struct file *file,
			  unsigned int cmd,
			  unsigned long arg);
extern int gen_disk_process_req_for_alias(int rw,
					  struct buffer_head *bh,
					  mach_port_t device_port);
extern kern_return_t machine_disk_read_absolute(kdev_t dev,
						mach_port_t device_port,
						unsigned long recnum,
						char *data,
						unsigned long size);
extern kern_return_t machine_disk_write_absolute(kdev_t dev,
						 mach_port_t device_port,
						 unsigned long recnum,
						 char *data,
						 unsigned long size);
extern int machine_disk_get_params(kdev_t dev,
				   mach_port_t device_port,
				   struct hd_geometry *loc,
				   boolean_t partition_only);

extern int machine_disk_revalidate(kdev_t dev,
				   mach_port_t device_port);

#endif	/* _OSFMACH3_BLOCK_DEV_H */
