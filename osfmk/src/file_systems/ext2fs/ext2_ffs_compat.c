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
 * HISTORY
 * $Log: ext2_ffs_compat.c,v $
 * Revision 1.1.2.1  1997/09/26  13:40:20  barbou
 * 	Adapted to support little-endian "ext2" fs on all supported platforms.
 * 	[1997/09/26  13:32:39  barbou]
 *
 * Revision 1.1.1.2  1997/09/26  13:32:39  barbou
 * 	Adapted to support little-endian "ext2" fs on all supported platforms.
 *
 * Revision 1.1.3.1  1995/08/21  19:24:23  devrcs
 * 	Copied from CMU LINUX boot.
 * 	[95/03/21            bernadat]
 *
 * Revision 1.1.2.1  1995/06/30  13:27:32  bernadat
 * 	Copied from CMU LINUX boot.
 * 	[95/03/21            bernadat]
 *
 * Revision 1.1.1.2  1995/06/30  11:59:53  bernadat
 * 	Copied from CMU LINUX boot.
 * 	[95/03/21            bernadat]
 *
 *      Creation
 *      [94/03/10            card]
 */
/*
 * BSD FFS like functions used to ease porting bootstrap to Linux ext2 fs
 */


#include <ext2fs/ext2fs.h>

int ext2_ino2blk(
	    struct ext2_super_block *fs,
	    struct ext2_group_desc *gd, 
	    int ino)
{
        int group;
        int blk;

        group = (ino - 1) / EXT2_INODES_PER_GROUP(fs);
        blk = le32_to_cpu(gd[group].bg_inode_table) +
	      (((ino - 1) % EXT2_INODES_PER_GROUP(fs)) /
               EXT2_INODES_PER_BLOCK(fs));
        return blk;
}

int ext2_fsbtodb(
	    struct ext2_super_block *fs,
	    int b)
{
        return (b * EXT2_BLOCK_SIZE(fs)) / DEV_BSIZE;
}

int ext2_itoo(
	 struct ext2_super_block *fs,
	 int ino)
{
	return (ino - 1) % EXT2_INODES_PER_BLOCK(fs);
}

int ext2_blkoff(
	   struct ext2_super_block * fs,
	   vm_offset_t offset)
{
	return offset % EXT2_BLOCK_SIZE(fs);
}

int ext2_lblkno(
	   struct ext2_super_block * fs,
	   vm_offset_t offset)
{
	return offset / EXT2_BLOCK_SIZE(fs);
}

int ext2_blksize(
	    struct ext2_super_block *fs,
	    struct ext2fs_file *fp,
	    daddr_t file_block)
{
	return EXT2_BLOCK_SIZE(fs);	/* XXX - fix for fragments */
}

