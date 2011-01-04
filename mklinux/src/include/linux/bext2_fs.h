/*
 *  linux/include/linux/bext2_fs.h
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/include/linux/minix_fs.h
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#ifndef _LINUX_BEXT2_FS_H
#define _LINUX_BEXT2_FS_H

#include <linux/ext2_fs.h>

/*
 * Debug code
 */
#ifdef EXT2FS_DEBUG
#	define bext2_debug(f, a...)	{ \
					printk ("BEXT2-fs DEBUG (%s, %d): %s:", \
						__FILE__, __LINE__, __FUNCTION__); \
				  	printk (f, ## a); \
					}
#else
#	define bext2_debug(f, a...)	/**/
#endif

#ifdef __KERNEL__
/*
 * Function prototypes
 */

/* acl.c */
extern int bext2_permission (struct inode *, int);

/* balloc.c */
extern int bext2_new_block (const struct inode *, unsigned long,
			   __u32 *, __u32 *, int *);
extern void bext2_free_blocks (const struct inode *, unsigned long,
			      unsigned long);
extern unsigned long bext2_count_free_blocks (struct super_block *);
extern void bext2_check_blocks_bitmap (struct super_block *);

/* bitmap.c */
extern unsigned long bext2_count_free (struct buffer_head *, unsigned);

/* dir.c */
extern int bext2_check_dir_entry (const char *, struct inode *,
				 struct ext2_dir_entry *, struct buffer_head *,
				 unsigned long);

/* file.c */
extern int bext2_read (struct inode *, struct file *, char *, int);
extern int bext2_write (struct inode *, struct file *, char *, int);

/* fsync.c */
extern int bext2_sync_file (struct inode *, struct file *);

/* ialloc.c */
extern struct inode * bext2_new_inode (const struct inode *, int, int *);
extern void bext2_free_inode (struct inode *);
extern unsigned long bext2_count_free_inodes (struct super_block *);
extern void bext2_check_inodes_bitmap (struct super_block *);

/* inode.c */
extern int bext2_bmap (struct inode *, int);

extern struct buffer_head * bext2_getblk (struct inode *, long, int, int *);
extern struct buffer_head * bext2_bread (struct inode *, int, int, int *);

extern int bext2_getcluster (struct inode * inode, long block);
extern void bext2_read_inode (struct inode *);
extern void bext2_write_inode (struct inode *);
extern void bext2_put_inode (struct inode *);
extern int bext2_sync_inode (struct inode *);
extern void bext2_discard_prealloc (struct inode *);

/* ioctl.c */
extern int bext2_ioctl (struct inode *, struct file *, unsigned int,
		       unsigned long);

/* namei.c */
extern void bext2_release (struct inode *, struct file *);
extern int bext2_lookup (struct inode *,const char *, int, struct inode **);
extern int bext2_create (struct inode *,const char *, int, int,
			struct inode **);
extern int bext2_mkdir (struct inode *, const char *, int, int);
extern int bext2_rmdir (struct inode *, const char *, int);
extern int bext2_unlink (struct inode *, const char *, int);
extern int bext2_symlink (struct inode *, const char *, int, const char *);
extern int bext2_link (struct inode *, struct inode *, const char *, int);
extern int bext2_mknod (struct inode *, const char *, int, int, int);
extern int bext2_rename (struct inode *, const char *, int,
			struct inode *, const char *, int, int);

/* super.c */
extern void bext2_error (struct super_block *, const char *, const char *, ...)
	__attribute__ ((format (printf, 3, 4)));
extern NORET_TYPE void bext2_panic (struct super_block *, const char *,
				   const char *, ...)
	__attribute__ ((NORET_AND format (printf, 3, 4)));
extern void bext2_warning (struct super_block *, const char *, const char *, ...)
	__attribute__ ((format (printf, 3, 4)));
extern void bext2_put_super (struct super_block *);
extern void bext2_write_super (struct super_block *);
extern int bext2_remount (struct super_block *, int *, char *);
extern struct super_block * bext2_read_super (struct super_block *,void *,int);
extern int init_bext2_fs(void);
extern void bext2_statfs (struct super_block *, struct statfs *, int);

/* truncate.c */
extern void bext2_truncate (struct inode *);

/*
 * Inodes and files operations
 */

/* dir.c */
extern struct inode_operations bext2_dir_inode_operations;

/* file.c */
extern struct inode_operations bext2_file_inode_operations;

/* symlink.c */
extern struct inode_operations bext2_symlink_inode_operations;

#endif	/* __KERNEL__ */

#endif	/* _LINUX_BEXT2_FS_H */
