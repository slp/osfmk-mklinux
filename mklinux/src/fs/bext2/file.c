/*
 *  linux/fs/bext2/file.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/file.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  ext2 fs regular file handling primitives
 */

#include <asm/segment.h>
#include <asm/system.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/bext2_fs.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/stat.h>
#include <linux/locks.h>
#include <linux/mm.h>
#include <linux/pagemap.h>

#define	NBUF	32

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#include <linux/fs.h>
#include <linux/ext2_fs.h>

static int bext2_file_write (struct inode *, struct file *, const char *, int);
static void bext2_release_file (struct inode *, struct file *);

/*
 * We have mostly NULL's here: the current defaults are ok for
 * the bext2 filesystem.
 */
static struct file_operations bext2_file_operations = {
	NULL,			/* lseek - default */
	generic_file_read,	/* read */
	bext2_file_write,	/* write */
	NULL,			/* readdir - bad */
	NULL,			/* select - default */
	bext2_ioctl,		/* ioctl */
	generic_file_mmap,	/* mmap */
	NULL,			/* no special open is needed */
	bext2_release_file,	/* release */
	bext2_sync_file,		/* fsync */
	NULL,			/* fasync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
};

struct inode_operations bext2_file_inode_operations = {
	&bext2_file_operations,/* default file operations */
	NULL,			/* create */
	NULL,			/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	generic_readpage,	/* readpage */
	NULL,			/* writepage */
	bext2_bmap,		/* bmap */
	bext2_truncate,		/* truncate */
	bext2_permission,	/* permission */
	NULL			/* smap */
};

static int bext2_file_write (struct inode * inode, struct file * filp,
			    const char * buf, int count)
{
	const loff_t two_gb = 2147483647;
	loff_t pos;
	off_t pos2;
	long block;
	int offset;
	int written, c;
	struct buffer_head * bh, *bufferlist[NBUF];
	struct super_block * sb;
	int err;
	int i,buffercount,write_error;

	write_error = buffercount = 0;
	if (!inode) {
		printk("bext2_file_write: inode = NULL\n");
		return -EINVAL;
	}
	sb = inode->i_sb;
	if (sb->s_flags & MS_RDONLY)
		/*
		 * This fs has been automatically remounted ro because of errors
		 */
		return -ENOSPC;

	if (!S_ISREG(inode->i_mode)) {
		bext2_warning (sb, "bext2_file_write", "mode = %07o",
			      inode->i_mode);
		return -EINVAL;
	}
	if (filp->f_flags & O_APPEND)
		pos = inode->i_size;
	else
		pos = filp->f_pos;
	pos2 = (off_t) pos;
	/*
	 * If a file has been opened in synchronous mode, we have to ensure
	 * that meta-data will also be written synchronously.  Thus, we
	 * set the i_osync field.  This field is tested by the allocation
	 * routines.
	 */
	if (filp->f_flags & O_SYNC)
		inode->u.ext2_i.i_osync++;
	block = pos2 >> EXT2_BLOCK_SIZE_BITS(sb);
	offset = pos2 & (sb->s_blocksize - 1);
	c = sb->s_blocksize - offset;
	written = 0;
	while (count > 0) {
		if (pos > two_gb) {
			if (!written)
				written = -EFBIG;
			break;
		}
		bh = bext2_getblk (inode, block, 1, &err);
		if (!bh) {
			if (!written)
				written = err;
			break;
		}
		count -= c;
		if (count < 0)
			c += count;
		if (c != sb->s_blocksize && !buffer_uptodate(bh)) {
			ll_rw_block (READ, 1, &bh);
			wait_on_buffer (bh);
			if (!buffer_uptodate(bh)) {
				brelse (bh);
				if (!written)
					written = -EIO;
				break;
			}
		}
		memcpy_fromfs (bh->b_data + offset, buf, c);
		update_vm_cache(inode, pos, bh->b_data + offset, c);
		pos2 += c;
		pos += c;
		written += c;
		buf += c;
		mark_buffer_uptodate(bh, 1);
		mark_buffer_dirty(bh, 0);
		if (filp->f_flags & O_SYNC)
			bufferlist[buffercount++] = bh;
		else
			brelse(bh);
		if (buffercount == NBUF){
			ll_rw_block(WRITE, buffercount, bufferlist);
			for(i=0; i<buffercount; i++){
				wait_on_buffer(bufferlist[i]);
				if (!buffer_uptodate(bufferlist[i]))
					write_error=1;
				brelse(bufferlist[i]);
			}
			buffercount=0;
		}
		if(write_error)
			break;
		block++;
		offset = 0;
		c = sb->s_blocksize;
	}
	if ( buffercount ){
		ll_rw_block(WRITE, buffercount, bufferlist);
		for(i=0; i<buffercount; i++){
			wait_on_buffer(bufferlist[i]);
			if (!buffer_uptodate(bufferlist[i]))
				write_error=1;
			brelse(bufferlist[i]);
		}
	}		
	if (pos > inode->i_size)
		inode->i_size = pos;
	if (filp->f_flags & O_SYNC)
		inode->u.ext2_i.i_osync--;
	inode->i_ctime = inode->i_mtime = CURRENT_TIME;
	filp->f_pos = pos;
	inode->i_dirt = 1;
	return written;
}

/*
 * Called when an inode is released. Note that this is different
 * from bext2_open: open gets called at every open, but release
 * gets called only when /all/ the files are closed.
 */
static void bext2_release_file (struct inode * inode, struct file * filp)
{
	if (filp->f_mode & 2)
		bext2_discard_prealloc (inode);
}
