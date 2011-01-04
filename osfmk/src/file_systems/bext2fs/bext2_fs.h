/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *  linux/include/linux/bext2_fs.h
 *
 *  Copyright (C) 1992, 1993, 1994  Remy Card (card@masi.ibp.fr)
 *                                  Laboratoire MASI - Institut Blaise Pascal
 *                                  Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/include/linux/minix_fs.h
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#ifndef _LINUX_BEXT2_FS_H
#define _LINUX_BEXT2_FS_H

/*
 * The second extended filesystem constants/structures
 */

/*
 * Define BEXT2FS_DEBUG to produce debug messages
 */
#undef BEXT2FS_DEBUG

/*
 * Define BEXT2FS_DEBUG_CACHE to produce cache debug messages
 */
#undef BEXT2FS_DEBUG_CACHE

/*
 * Define BEXT2FS_CHECK_CACHE to add some checks to the name cache code
 */
#undef BEXT2FS_CHECK_CACHE

/*
 * Define BEXT2FS_PRE_02B_COMPAT to convert ext 2 fs prior to 0.2b
 */
#undef BEXT2FS_PRE_02B_COMPAT

/*
 * Define DONT_USE_DCACHE to inhibit the directory cache
 */
#define DONT_USE_DCACHE

/*
 * Define BEXT2_PREALLOCATE to preallocate data blocks for expanding files
 */
#define BEXT2_PREALLOCATE

/*
 * The second extended file system version
 */
#define BEXT2FS_DATE		"94/03/10"
#define BEXT2FS_VERSION		"0.5"

/*
 * Debug code
 */
#ifdef BEXT2FS_DEBUG
#	define bext2_debug(f, a...)	{ \
					printk ("BEXT2-fs DEBUG (%s, %d): %s:", \
						__FILE__, __LINE__, __FUNCTION__); \
				  	printk (f, ## a); \
					}
#else
#	define bext2_debug(f, a...)	/**/
#endif

/*
 * Special inodes numbers
 */
#define	BEXT2_BAD_INO		 1	/* Bad blocks inode */
#define BEXT2_ROOT_INO		 2	/* Root inode */
#define BEXT2_ACL_IDX_INO	 3	/* ACL inode */
#define BEXT2_ACL_DATA_INO	 4	/* ACL inode */
#define BEXT2_BOOT_LOADER_INO	 5	/* Boot loader inode */
#define BEXT2_UNDEL_DIR_INO	 6	/* Undelete directory inode */
#define BEXT2_FIRST_INO		11	/* First non reserved inode */

/*
 * The second extended file system magic number
 */
#define BEXT2_PRE_02B_MAGIC	0xEF51
#define BEXT2_SUPER_MAGIC	0xEF53

/*
 * Maximal count of links to a file
 */
#define BEXT2_LINK_MAX		32000

/*
 * Macro-instructions used to manage several block sizes
 */
#define BEXT2_MIN_BLOCK_SIZE		1024
#define	BEXT2_MAX_BLOCK_SIZE		4096
#define BEXT2_MIN_BLOCK_LOG_SIZE		  10
#ifdef __KERNEL__
# define BEXT2_BLOCK_SIZE(s)		((s)->s_blocksize)
#else
# define BEXT2_BLOCK_SIZE(s)		(BEXT2_MIN_BLOCK_SIZE << (s)->s_log_block_size)
#endif
#define BEXT2_ACLE_PER_BLOCK(s)		(BEXT2_BLOCK_SIZE(s) / sizeof (struct bext2_acl_entry))
#define	BEXT2_ADDR_PER_BLOCK(s)		(BEXT2_BLOCK_SIZE(s) / sizeof (unsigned long))
#ifdef __KERNEL__
# define BEXT2_BLOCK_SIZE_BITS(s)	((s)->u.bext2_sb.s_es->s_log_block_size + 10)
#else
# define BEXT2_BLOCK_SIZE_BITS(s)	((s)->s_log_block_size + 10)
#endif
#define	BEXT2_INODES_PER_BLOCK(s)	(BEXT2_BLOCK_SIZE(s) / sizeof (struct bext2_inode))

/*
 * Macro-instructions used to manage fragments
 */
#define BEXT2_MIN_FRAG_SIZE		1024
#define	BEXT2_MAX_FRAG_SIZE		4096
#define BEXT2_MIN_FRAG_LOG_SIZE		  10
#ifdef __KERNEL__
# define BEXT2_FRAG_SIZE(s)		((s)->u.bext2_sb.s_frag_size)
# define BEXT2_FRAGS_PER_BLOCK(s)	((s)->u.bext2_sb.s_frags_per_block)
#else
# define BEXT2_FRAG_SIZE(s)		(BEXT2_MIN_FRAG_SIZE << (s)->s_log_frag_size)
# define BEXT2_FRAGS_PER_BLOCK(s)	(BEXT2_BLOCK_SIZE(s) / BEXT2_FRAG_SIZE(s))
#endif

/*
 * ACL structures
 */
struct bext2_acl_header	/* Header of Access Control Lists */
{
	unsigned long aclh_size;
	unsigned long aclh_file_count;
	unsigned long aclh_acle_count;
	unsigned long aclh_first_acle;
};

struct bext2_acl_entry	/* Access Control List Entry */
{
	unsigned long  acle_size;
	unsigned short acle_perms;	/* Access permissions */
	unsigned short acle_type;	/* Type of entry */
	unsigned short acle_tag;	/* User or group identity */
	unsigned short acle_pad1;
	unsigned long  acle_next;	/* Pointer on next entry for the */
					/* same inode or on next free entry */
};

/*
 * Structure of a blocks group descriptor
 */
struct bext2_old_group_desc
{
	unsigned long  bg_block_bitmap;		/* Blocks bitmap block */
	unsigned long  bg_inode_bitmap;		/* Inodes bitmap block */
	unsigned long  bg_inode_table;		/* Inodes table block */
	unsigned short bg_free_blocks_count;	/* Free blocks count */
	unsigned short bg_free_inodes_count;	/* Free inodes count */
};

struct bext2_group_desc
{
	unsigned long  bg_block_bitmap;		/* Blocks bitmap block */
	unsigned long  bg_inode_bitmap;		/* Inodes bitmap block */
	unsigned long  bg_inode_table;		/* Inodes table block */
	unsigned short bg_free_blocks_count;	/* Free blocks count */
	unsigned short bg_free_inodes_count;	/* Free inodes count */
	unsigned short bg_used_dirs_count;	/* Directories count */
	unsigned short bg_pad;
	unsigned long  bg_reserved[3];
};

/*
 * Macro-instructions used to manage group descriptors
 */
#ifdef __KERNEL__
# define BEXT2_BLOCKS_PER_GROUP(s)	((s)->u.bext2_sb.s_blocks_per_group)
# define BEXT2_DESC_PER_BLOCK(s)		((s)->u.bext2_sb.s_desc_per_block)
# define BEXT2_INODES_PER_GROUP(s)	((s)->u.bext2_sb.s_inodes_per_group)
#else
# define BEXT2_BLOCKS_PER_GROUP(s)	((s)->s_blocks_per_group)
# define BEXT2_DESC_PER_BLOCK(s)		(BEXT2_BLOCK_SIZE(s) / sizeof (struct bext2_group_desc))
# define BEXT2_INODES_PER_GROUP(s)	((s)->s_inodes_per_group)
#endif

/*
 * Constants relative to the data blocks
 */
#define	BEXT2_NDIR_BLOCKS		12
#define	BEXT2_IND_BLOCK			BEXT2_NDIR_BLOCKS
#define	BEXT2_DIND_BLOCK			(BEXT2_IND_BLOCK + 1)
#define	BEXT2_TIND_BLOCK			(BEXT2_DIND_BLOCK + 1)
#define	BEXT2_N_BLOCKS			(BEXT2_TIND_BLOCK + 1)

/*
 * Inode flags
 */
#define	BEXT2_SECRM_FL			0x0001	/* Secure deletion */
#define	BEXT2_UNRM_FL			0x0002	/* Undelete */
#define	BEXT2_COMPR_FL			0x0004	/* Compress file */
#define BEXT2_SYNC_FL			0x0008	/* Synchronous updates */

/*
 * ioctl commands
 */
#define	BEXT2_IOC_GETFLAGS		_IOR('f', 1, long)
#define	BEXT2_IOC_SETFLAGS		_IOW('f', 2, long)
#define	BEXT2_IOC_GETVERSION		_IOR('v', 1, long)
#define	BEXT2_IOC_SETVERSION		_IOW('v', 2, long)

/*
 * Structure of an inode on the disk
 */
struct bext2_inode {
	unsigned short i_mode;		/* File mode */
	unsigned short i_uid;		/* Owner Uid */
	unsigned long  i_size;		/* Size in bytes */
	unsigned long  i_atime;		/* Access time */
	unsigned long  i_ctime;		/* Creation time */
	unsigned long  i_mtime;		/* Modification time */
	unsigned long  i_dtime;		/* Deletion Time */
	unsigned short i_gid;		/* Group Id */
	unsigned short i_links_count;	/* Links count */
	unsigned long  i_blocks;	/* Blocks count */
	unsigned long  i_flags;		/* File flags */
	unsigned long  i_reserved1;
	unsigned long  i_block[BEXT2_N_BLOCKS];/* Pointers to blocks */
	unsigned long  i_version;	/* File version (for NFS) */
	unsigned long  i_file_acl;	/* File ACL */
	unsigned long  i_dir_acl;	/* Directory ACL */
	unsigned long  i_faddr;		/* Fragment address */
	unsigned char  i_frag;		/* Fragment number */
	unsigned char  i_fsize;		/* Fragment size */
	unsigned short i_pad1;
	unsigned long  i_reserved2[2];
};

/*
 * File system states
 */
#define	BEXT2_VALID_FS			0x0001	/* Unmounted cleany */
#define	BEXT2_ERROR_FS			0x0002	/* Errors detected */

/*
 * Mount flags
 */
#define BEXT2_MOUNT_CHECK_NORMAL		0x0001	/* Do some more checks */
#define BEXT2_MOUNT_CHECK_STRICT		0x0002	/* Do again more checks */
#define BEXT2_MOUNT_CHECK		(BEXT2_MOUNT_CHECK_NORMAL | \
					 BEXT2_MOUNT_CHECK_STRICT)
#define BEXT2_MOUNT_GRPID		0x0004	/* Create files with directory's group */
#define BEXT2_MOUNT_DEBUG		0x0008	/* Some debugging messages */
#define BEXT2_MOUNT_ERRORS_CONT		0x0010	/* Continue on errors */
#define BEXT2_MOUNT_ERRORS_RO		0x0020	/* Remount fs ro on errors */
#define BEXT2_MOUNT_ERRORS_PANIC		0x0040	/* Panic on errors */

#define clear_opt(o, opt)		o &= ~BEXT2_MOUNT_##opt
#define set_opt(o, opt)			o |= BEXT2_MOUNT_##opt
#define test_opt(sb, opt)		((sb)->u.bext2_sb.s_mount_opt & \
					 BEXT2_MOUNT_##opt)
/*
 * Maximal mount counts between two filesystem checks
 */
#define BEXT2_DFL_MAX_MNT_COUNT		20	/* Allow 20 mounts */
#define BEXT2_DFL_CHECKINTERVAL		0	/* Don't use interval check */

/*
 * Behaviour when detecting errors
 */
#define BEXT2_ERRORS_CONTINUE		1	/* Continue execution */
#define BEXT2_ERRORS_RO			2	/* Remount fs read-only */
#define BEXT2_ERRORS_PANIC		3	/* Panic */
#define BEXT2_ERRORS_DEFAULT		BEXT2_ERRORS_CONTINUE

/*
 * Structure of the super block
 */
struct bext2_super_block {
	unsigned long  s_inodes_count;	/* Inodes count */
	unsigned long  s_blocks_count;	/* Blocks count */
	unsigned long  s_r_blocks_count;/* Reserved blocks count */
	unsigned long  s_free_blocks_count;/* Free blocks count */
	unsigned long  s_free_inodes_count;/* Free inodes count */
	unsigned long  s_first_data_block;/* First Data Block */
	unsigned long  s_log_block_size;/* Block size */
	long           s_log_frag_size;	/* Fragment size */
	unsigned long  s_blocks_per_group;/* # Blocks per group */
	unsigned long  s_frags_per_group;/* # Fragments per group */
	unsigned long  s_inodes_per_group;/* # Inodes per group */
	unsigned long  s_mtime;		/* Mount time */
	unsigned long  s_wtime;		/* Write time */
	unsigned short s_mnt_count;	/* Mount count */
	short          s_max_mnt_count;	/* Maximal mount count */
	unsigned short s_magic;		/* Magic signature */
	unsigned short s_state;		/* File system state */
	unsigned short s_errors;	/* Behaviour when detecting errors */
	unsigned short s_pad;
	unsigned long  s_lastcheck;	/* time of last check */
	unsigned long  s_checkinterval;	/* max. time between checks */
	unsigned long  s_reserved[238];	/* Padding to the end of the block */
};

/*
 * Structure of a directory entry
 */
#define BEXT2_NAME_LEN 255

struct bext2_dir_entry {
	unsigned long  inode;			/* Inode number */
	unsigned short rec_len;			/* Directory entry length */
	unsigned short name_len;		/* Name length */
	char           name[BEXT2_NAME_LEN];	/* File name */
};

/*
 * BEXT2_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define BEXT2_DIR_PAD		 	4
#define BEXT2_DIR_ROUND 			(BEXT2_DIR_PAD - 1)
#define BEXT2_DIR_REC_LEN(name_len)	(((name_len) + 8 + BEXT2_DIR_ROUND) & \
					 ~BEXT2_DIR_ROUND)

#ifdef __KERNEL__
/*
 * Function prototypes
 */

/*
 * Ok, these declarations are also in <linux/kernel.h> but none of the
 * bext2 source programs needs to include it so they are duplicated here.
 */
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
# define NORET_TYPE    __volatile__
# define ATTRIB_NORET  /**/
# define NORET_AND     /**/
#else
# define NORET_TYPE    /**/
# define ATTRIB_NORET  __attribute__((noreturn))
# define NORET_AND     noreturn,
#endif

/* acl.c */
extern int bext2_permission (struct inode *, int);

/* balloc.c */
extern int bext2_new_block (struct super_block *, unsigned long,
			   unsigned long *, unsigned long *);
extern void bext2_free_blocks (struct super_block *, unsigned long,
			      unsigned long);
extern unsigned long bext2_count_free_blocks (struct super_block *);
extern void bext2_check_blocks_bitmap (struct super_block *);

/* bitmap.c */
extern unsigned long bext2_count_free (struct buffer_head *, unsigned);

#ifndef DONT_USE_DCACHE
/* dcache.c */
extern void bext2_dcache_invalidate (unsigned short);
extern unsigned long bext2_dcache_lookup (unsigned short, unsigned long,
					 const char *, int);
extern void bext2_dcache_add (unsigned short, unsigned long, const char *,
			     int, unsigned long);
extern void bext2_dcache_remove (unsigned short, unsigned long, const char *,
				int);
#endif

/* dir.c */
extern int bext2_check_dir_entry (char *, struct inode *,
				 struct bext2_dir_entry *, struct buffer_head *,
				 unsigned long);

/* file.c */
extern int bext2_read (struct inode *, struct file *, char *, int);
extern int bext2_write (struct inode *, struct file *, char *, int);

/* fsync.c */
extern int bext2_sync_file (struct inode *, struct file *);

/* ialloc.c */
extern struct inode * bext2_new_inode (const struct inode *, int);
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
extern int bext2_open (struct inode *, struct file *);
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
			struct inode *, const char *, int);

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
extern void bext2_statfs (struct super_block *, struct statfs *);

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
