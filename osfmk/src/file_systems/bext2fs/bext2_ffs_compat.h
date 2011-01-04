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
 * $Log: bext2_ffs_compat.h,v $
 * Revision 1.1.2.1  1997/09/26  13:39:44  barbou
 * 	Moved from ext2fs directory: big-endian ext2 fs support.
 * 	[1997/09/26  13:27:08  barbou]
 *
 * Revision 1.1.1.2  1997/09/26  13:27:08  barbou
 * 	Moved from ext2fs directory: big-endian ext2 fs support.
 *
 * Revision 1.1.7.1  1997/02/27  09:49:55  bruel
 * 	linux-PA: Changed the location of super block
 * 	depending of the block size.
 * 	[97/02/27            bruel]
 *
 * Revision 1.1.6.2  1997/02/27  09:49:17  bruel
 * 	linux-PA: Changed the location of super block
 * 	depending of the block size.
 * 	[97/02/27            bruel]
 *
 * Revision 1.1.3.1  1995/08/21  19:24:27  devrcs
 * 	Copied from CMU LINUX boot.
 * 	[95/03/21            bernadat]
 *
 * Revision 1.1.2.1  1995/06/30  13:27:35  bernadat
 * 	Copied from CMU LINUX boot.
 * 	[95/03/21            bernadat]
 *
 * Revision 1.1.1.2  1995/06/30  11:59:57  bernadat
 * 	Copied from CMU LINUX boot.
 * 	[95/03/21            bernadat]
 *
 *      Creation
 *      [94/03/10            card]
 */
/*
 * BSD FFS like declarations used to ease porting bootstrap to Linux bext2 fs
 */

#define	SBSIZE		BEXT2_MIN_BLOCK_SIZE	/* Size of superblock */
#ifdef  HPOSFCOMPAT
#define	SBLOCK		((daddr_t) 1)		/* Location of superblock */
#else
#define	SBLOCK		((daddr_t) 2)		/* Location of superblock */
#endif /* HPOSFCOMPAT */

#define	NDADDR		BEXT2_NDIR_BLOCKS
#define	NIADDR		(BEXT2_N_BLOCKS - BEXT2_NDIR_BLOCKS)

#define	MAXNAMLEN	255

#define	ROOTINO		BEXT2_ROOT_INO

#define	NINDIR(fs)	BEXT2_ADDR_PER_BLOCK(fs)

#define	IC_FASTLINK

#define	IFMT		00170000
#define	IFSOCK		0140000
#define	IFLNK		0120000
#define	IFREG		0100000
#define	IFBLK		0060000
#define	IFDIR		0040000
#define	IFCHR		0020000
#define	IFIFO		0010000
#define	ISUID		0004000
#define	ISGID		0002000
#define	ISVTX		0001000

int 	bext2_ino2blk(
		struct bext2_super_block *fs,
		struct bext2_group_desc *gd,
		int ino);

int 	bext2_fsbtodb(
		struct bext2_super_block *fs,
		int b);

int 	bext2_itoo(
	     struct bext2_super_block *fs,
	     int ino);

int 	bext2_blkoff(
	       struct bext2_super_block *fs,
	       vm_offset_t offset);

int	bext2_lblkno(
		struct bext2_super_block *fs,
		vm_offset_t offset);

typedef	struct	bext2fs_file *bext2fs_file_t;

int	bext2_blksize(
		struct bext2_super_block *fs,
		bext2fs_file_t fp,
		daddr_t file_block);




