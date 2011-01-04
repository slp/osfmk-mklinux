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
/* CMU_HIST */
/*
 * Revision 2.5  92/03/01  00:39:32  rpd
 * 	Fixed device_get_status argument types.
 * 	[92/02/29            rpd]
 * 
 * Revision 2.4  92/02/23  22:25:43  elf
 * 	Removed debugging printf.
 * 	[92/02/23  13:16:45  af]
 * 
 * 	Added variation of file_direct to page on raw devices.
 * 	[92/02/22  18:53:04  af]
 * 
 * 	Added remove_file_direct().  Commented out some unused code.
 * 	[92/02/19  17:31:01  af]
 * 
 * Revision 2.3  92/01/24  18:14:31  rpd
 * 	Added casts to device_read dataCnt argument to mollify
 * 	buggy versions of gcc.
 * 	[92/01/24            rpd]
 * 
 * Revision 2.2  92/01/03  19:57:13  dbg
 * 	Make file_direct self-contained: add the few fields needed from
 * 	the superblock.
 * 	[91/10/17            dbg]
 * 
 * 	Deallocate unused superblocks when switching file systems.  Add
 * 	file_close routine to free space taken by file metadata.
 * 	[91/09/25            dbg]
 * 
 * 	Move outside of kernel.
 * 	Unmodify open_file to put arrays back on stack.
 * 	[91/09/04            dbg]
 * 
 * Revision 2.8  91/08/28  11:09:42  jsb
 * 	Added struct file_direct and associated functions.
 * 	[91/08/19            rpd]
 * 
 * Revision 2.7  91/07/31  17:24:07  dbg
 * 	Call vm_wire instead of vm_pageable.
 * 	[91/07/30  16:38:01  dbg]
 * 
 * Revision 2.6  91/05/18  14:28:45  rpd
 * 	Changed block_map to avoid blocking operations
 * 	while holding an exclusive lock.
 * 	[91/04/06            rpd]
 * 	Added locking in block_map.
 * 	[91/04/03            rpd]
 * 
 * Revision 2.5  91/05/14  15:22:53  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:01:23  mrt
 * 	Changed to new copyright
 * 	[91/01/28  14:54:52  mrt]
 * 
 * Revision 2.3  90/10/25  14:41:42  rwd
 * 	Modified open_file to allocate arrays from heap, not stack.
 * 	[90/10/23            rpd]
 * 
 * Revision 2.2  90/08/27  21:45:27  dbg
 * 	Reduce lint.
 * 	[90/08/13            dbg]
 * 
 * 	Created from include files, bsd4.3-Reno (public domain) source,
 * 	and old code written at CMU.
 * 	[90/07/17            dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 */
/*
 * Stand-alone ufs file reading package.
 */

#include "ufs.h"
#include "externs.h"
#include <stdlib.h>
#include <stdio.h>

#include <device/device_types.h>
#include <device/device.h>

security_token_t null_security_token;

static void	free_file_buffers(
				  struct ufs_file *);

static int 	read_inode(
			   ino_t, 
			   struct ufs_file *);

static int	block_map(
			  struct ufs_file *,
			  daddr_t,
			  daddr_t *);

static int	buf_read_file(
			      struct ufs_file *,
			      vm_offset_t,
			      vm_offset_t *,
			      vm_size_t *);

static int 	search_directory(
				 char *,
				 struct ufs_file *,
				 ino_t *);

static int 	read_fs(
			struct device *,
			struct fs **);

static int 	mount_fs(
			 struct ufs_file *);

static void 	unmount_fs(
			   struct ufs_file *);

struct fs_ops ufs_ops = {
	ufs_open_file,
	ufs_close_file,
	ufs_read_file,
	ufs_file_size,
	ufs_file_is_directory,
	ufs_file_is_executable
};

/*
 * machine_read_fs can be initialized by machine dependant modules
 * The FS can be OSF/1 like, but not strictly identical
 * Still, it migth work with some super block conversion
 */

int (*machine_read_fs)(struct device *, struct fs **);

/*
 * Free file buffers, but don't close file.
 */
static
void
free_file_buffers(register struct ufs_file *fp)
{
	register int level;

	/*
	 * Free the indirect blocks
	 */
	for (level = 0; level < NIADDR; level++) {
	    if (fp->f_blk[level] != 0) {
		(void) vm_deallocate(mach_task_self(),
				     fp->f_blk[level],
				     (vm_size_t)fp->f_blksize[level]);
		fp->f_blksize[level] = 0;
		fp->f_blk[level] = 0;
	    }
	    fp->f_blkno[level] = -1;
	}

	/*
	 * Free the data block
	 */
	if (fp->f_buf != 0) {
	    (void) vm_deallocate(mach_task_self(),
				 fp->f_buf,
				 (vm_size_t)fp->f_buf_size);
	    fp->f_buf = 0;
	}
	fp->f_buf_blkno = -1;
}

/*
 * Read a new inode into a file structure.
 */
static
int
read_inode(ino_t inumber, register struct ufs_file *fp)
{
	vm_offset_t		buf;
	vm_size_t		buf_size;
	register struct fs	*fs;
	daddr_t			disk_block;
	kern_return_t		rc;

	fs = fp->f_fs;
	disk_block = itod(fs, inumber);

	rc = device_read(fp->f_dev.dev_port,
			 0,
			 (recnum_t) dbtorec(&fp->f_dev,
					    fsbtodb(fp->f_fs, disk_block)),
			 (int) fs->fs_bsize,
			 (char **)&buf,
			 (mach_msg_type_number_t *)&buf_size);
	if (rc != KERN_SUCCESS)
	    return (rc);

	{
	    register struct dinode *dp;

	    dp = (struct dinode *)buf;
	    dp += itoo(fs, inumber);
	    fp->f_di = *dp;
	}

	(void) vm_deallocate(mach_task_self(), buf, buf_size);

	/*
	 * Clear out the old buffers
	 */
	free_file_buffers(fp);
	return (0);	 
}

/*
 * Given an offset in a file, find the disk block number that
 * contains that block.
 */
static
int
block_map(
	struct ufs_file *fp,
	daddr_t file_block,
	daddr_t *disk_block_p)
{
	int		level;
	int		idx;
	daddr_t		ind_block_num;
	kern_return_t	rc;

	vm_offset_t	olddata[NIADDR+1];
	vm_size_t	oldsize[NIADDR+1];

	/*
	 * Index structure of an inode:
	 *
	 * i_db[0..NDADDR-1]	hold block numbers for blocks
	 *			0..NDADDR-1
	 *
	 * i_ib[0]		index block 0 is the single indirect
	 *			block
	 *			holds block numbers for blocks
	 *			NDADDR .. NDADDR + NINDIR(fs)-1
	 *
	 * i_ib[1]		index block 1 is the double indirect
	 *			block
	 *			holds block numbers for INDEX blocks
	 *			for blocks
	 *			NDADDR + NINDIR(fs) ..
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2 - 1
	 *
	 * i_ib[2]		index block 2 is the triple indirect
	 *			block
	 *			holds block numbers for double-indirect
	 *			blocks for blocks
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2 ..
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2
	 *				+ NINDIR(fs)**3 - 1
	 */

	if (file_block < NDADDR) {
	    /* Direct block. */
	    *disk_block_p = fp->f_di.di_db[file_block];
	    return (0);
	}

	file_block -= NDADDR;

	/*
	 * nindir[0] = NINDIR
	 * nindir[1] = NINDIR**2
	 * nindir[2] = NINDIR**3
	 *	etc
	 */
	for (level = 0; level < NIADDR; level++) {
	    if (file_block < fp->f_nindir[level])
		break;
	    file_block -= fp->f_nindir[level];
	}
	if (level == NIADDR) {
	    /* Block number too high */
	    return (FS_NOT_IN_FILE);
	}

	ind_block_num = fp->f_di.di_ib[level];

	/*
	 * Initialize array of blocks to free.
	 */
	for (idx = 0; idx < NIADDR; idx++)
	    oldsize[idx] = 0;

	for (; level >= 0; level--) {

	    vm_offset_t	data;
	    vm_size_t	size;

	    if (ind_block_num == 0)
		break;

	    if (fp->f_blkno[level] == ind_block_num) {
		/*
		 *	Cache hit.  Just pick up the data.
		 */

		data = fp->f_blk[level];
	    }
	    else {
		rc = device_read(fp->f_dev.dev_port,
				 0,
				 (recnum_t) dbtorec(&fp->f_dev,
						    fsbtodb(fp->f_fs,
							    ind_block_num)),
				 fp->f_fs->fs_bsize,
				 (char **)&data,
				 (mach_msg_type_number_t *)&size);
		if (rc != KERN_SUCCESS)
		    return (rc);

		olddata[level] = fp->f_blk[level];
		oldsize[level] = fp->f_blksize[level];

		fp->f_blkno[level] = ind_block_num;
		fp->f_blk[level] = data;
		fp->f_blksize[level] = size;

	    }

	    if (level > 0) {
		idx = file_block / fp->f_nindir[level-1];
		file_block %= fp->f_nindir[level-1];
	    }
	    else
		idx = file_block;

	    ind_block_num = ((daddr_t *)data)[idx];
	}

	/*
	 * After unlocking the file, free any blocks that
	 * we need to free.
	 */
	for (idx = 0; idx < NIADDR; idx++)
	    if (oldsize[idx] != 0)
		(void) vm_deallocate(mach_task_self(),
				     olddata[idx],
				     oldsize[idx]);

	*disk_block_p = ind_block_num;
	return (0);
}

/*
 * Read a portion of a file into an internal buffer.  Return
 * the location in the buffer and the amount in the buffer.
 */
static
int
buf_read_file(
	register struct ufs_file	*fp,
	vm_offset_t			offset,
	vm_offset_t			*buf_p,		/* out */
	vm_size_t			*size_p)	/* out */
{
	register struct fs	*fs;
	vm_offset_t		off;
	register daddr_t	file_block;
	daddr_t			disk_block;
	int			rc;
	vm_offset_t		block_size;

	if (offset >= fp->f_di.di_size)
	    return (FS_NOT_IN_FILE);

	fs = fp->f_fs;

	off = blkoff(fs, offset);
	file_block = lblkno(fs, offset);
	block_size = dblksize(fs, &(fp->f_di), file_block);
#if	DEBUG
	if(debug)
		printf("buf_read_file, off = %x\n", off);
#endif
	if (off || (!*buf_p) || *size_p < block_size ||
	    ((*buf_p) & (fp->f_dev.rec_size-1))) {
	    if (file_block != fp->f_buf_blkno) {
	        rc = block_map(fp, file_block, &disk_block);
		if (rc != 0)
		    return (rc);

		if (fp->f_buf) {
		    (void)vm_deallocate(mach_task_self(),
					fp->f_buf,
					(vm_size_t)fp->f_buf_size);
		    fp->f_buf = 0;
		}

	        if (disk_block == 0) {
		    (void)vm_allocate(mach_task_self(),
				      &fp->f_buf,
				      block_size,
				      TRUE);
		    fp->f_buf_size = block_size;
	        } else {
		    rc = device_read(fp->f_dev.dev_port,
				     0,
				     (recnum_t) dbtorec(&fp->f_dev,
							fsbtodb(fs,
								disk_block)),
				     (int) block_size,
				     (char **) &fp->f_buf,
				     (mach_msg_type_number_t *)&fp->f_buf_size);
	        }
	        if (rc)
		    return (rc);

		fp->f_buf_blkno = file_block;
	    }

	    /*
	     * Return address of byte in buffer corresponding to
	     * offset, and size of remainder of buffer after that
	     * byte.
	     */
	    *buf_p = fp->f_buf + off;
	    *size_p = block_size - off;
	} else {
		/*
		 * Directly read into caller buffer
		 */
	        rc = block_map(fp, file_block, &disk_block);
		if (rc != 0)
		    return (rc);

		rc = device_read_overwrite(fp->f_dev.dev_port,
				0,
				(recnum_t) dbtorec(&fp->f_dev,
						   fsbtodb(fs,
							   disk_block)),
				(int) block_size,
				*buf_p,
				(mach_msg_type_number_t *)size_p);
	        if (rc)
		    return (rc);
	}

	/*
	 * Truncate buffer at end of file.
	 */
	if (*size_p > fp->f_di.di_size - offset)
	    *size_p = fp->f_di.di_size - offset;

	return (0);
}

/*
 * Search a directory for a name and return its
 * i_number.
 */
static
int
search_directory(
	char *name,
	register struct ufs_file *fp,
	ino_t *inumber_p)
{
	vm_offset_t	buf;
	vm_size_t	buf_size;
	vm_offset_t	offset;
	register struct dirent *dp;
	int		length;
	kern_return_t	rc;

	length = strlen(name);

#if DEBUG
	if(debug)
		printf("search_directory(%s)\n", name);
#endif 
	offset = 0;
	while (offset < fp->f_di.di_size) {
	    buf = 0;
	    buf_size = 0;
	    rc = buf_read_file(fp, offset, &buf, &buf_size);
	    if (rc != KERN_SUCCESS)
		return (rc);

	    dp = (struct dirent *)buf;
	    if (dp->d_fileno != 0) {
		dp->d_name[dp->d_namlen] = 0;
		if (dp->d_namlen == length &&
		    !strcmp(name, dp->d_name))
	    	{
		    /* found entry */
		    *inumber_p = dp->d_fileno;
		    return (0);
		}
	    }
	    offset += dp->d_reclen;
	}
	return (FS_NO_ENTRY);
}

static
int
read_fs(struct device *dev, struct fs **fsp)
{
	register struct fs *fs;
	vm_offset_t	buf;
	vm_size_t	buf_size;
	int		error;

	error = device_read(dev->dev_port, 0,
		    		(recnum_t) dbtorec(dev, SBLOCK), SBSIZE,
			    	(char **) &buf, 
				(mach_msg_type_number_t *)&buf_size);
	if (error)
	    return (error);

	fs = (struct fs *)buf;
	if (fs->fs_magic != FS_MAGIC ||
	    fs->fs_bsize > MAXBSIZE ||
	    fs->fs_bsize < sizeof(struct fs)) {
		(void) vm_deallocate(mach_task_self(), buf, buf_size);
		return (FS_INVALID_FS);
	}
	/* don't read cylinder groups - we aren't modifying anything */

	*fsp = fs;
	return 0;
}

static
int
mount_fs(register struct ufs_file *fp)
{
	register struct fs *fs;
	int error;

	error = read_fs(&fp->f_dev, &fp->f_fs);
	if (error && machine_read_fs) 
	      error = (*machine_read_fs)(&fp->f_dev, &fp->f_fs);

	if (error)
	    return (error);
	fs = fp->f_fs;

	/*
	 * Calculate indirect block levels.
	 */
	{
	    register int	mult;
	    register int	level;

	    mult = 1;
	    for (level = 0; level < NIADDR; level++) {
		mult *= NINDIR(fs);
		fp->f_nindir[level] = mult;
	    }
	}

	return (0);
}

static
void
unmount_fs(register struct ufs_file *fp)
{
	if (fp->f_fs != 0) {
	    (void) vm_deallocate(mach_task_self(),
				 (vm_offset_t) fp->f_fs,
				 (vm_size_t)SBSIZE);
	    fp->f_fs = 0;
	}
}

/*
 * Open a file.
 */
int
ufs_open_file(
	struct device 	*dev,
	const char 	*path,
	fs_private_t 	*private)
{
#define	RETURN(code)	{ rc = (code); goto exit; }

	register char	*cp, *component;
	register int	c;	/* char */
	register int	rc;
	ino_t		inumber, parent_inumber;
	int		nlinks = 0;
	char	        *namebuf;
	struct ufs_file *fp;

	if (path == 0 || *path == '\0') {
	    return FS_NO_ENTRY;
	}
#if	DEBUG
	if(debug)
		printf("ufs_open_file(%s)\n", path);
#endif

	namebuf = (char*)malloc((size_t)(PATH_MAX+1));
	fp = (struct ufs_file *)malloc(sizeof(struct ufs_file));
	memset((char *)fp, 0, sizeof(struct ufs_file));
	fp->f_dev = *dev;
	/*
	 * Copy name into buffer to allow modifying it.
	 */
	strcpy(namebuf, path);

	cp = namebuf;

	rc = mount_fs(fp);
	if (rc) 
	    goto exit;

	inumber = (ino_t) ROOTINO;
	if ((rc = read_inode(inumber, fp)) != 0) {
	    printf("can't read root inode\n");
	    goto exit;
	}

	while (*cp) {

	    /*
	     * Check that current node is a directory.
	     */
	    if ((fp->f_di.di_mode & IFMT) != IFDIR)
		RETURN (FS_NOT_DIRECTORY);

	    /*
	     * Remove extra separators
	     */
	    while (*cp == '/')
		cp++;

	    /*
	     * Get next component of path name.
	     */
	    component = cp;
	    {
		register int	len = 0;

		while ((c = *cp) != '\0' && c != '/') {
		    if (len++ > NAME_MAX)
			RETURN (FS_NAME_TOO_LONG);
		    if (c & 0200)
			RETURN (FS_INVALID_PARAMETER);
		    cp++;
		}
		*cp = 0;
	    }

	    /*
	     * Look up component in current directory.
	     * Save directory inumber in case we find a
	     * symbolic link.
	     */
	    parent_inumber = inumber;
	    rc = search_directory(component, fp, &inumber);
	    if (rc)
       		goto exit;

	    *cp = c;

	    /*
	     * Open next component.
	     */
	    if ((rc = read_inode(inumber, fp)) != 0)
		goto exit;

	    /*
	     * Check for symbolic link.
	     */
	    if ((fp->f_di.di_mode & IFMT) == IFLNK) {
		int	link_len = fp->f_di.di_size;
		size_t	len;

		len = strlen(cp) + 1;

		if (link_len + len >= PATH_MAX - 1)
		    RETURN (FS_NAME_TOO_LONG);

		if (++nlinks > MAXSYMLINKS)
		    RETURN (FS_SYMLINK_LOOP);

		ovbcopy(cp, &namebuf[link_len], len);

#ifdef	IC_FASTLINK
		if ((fp->f_di.di_flags & IC_FASTLINK) != 0) {
		    memcpy(namebuf, fp->f_di.di_symlink, (unsigned)link_len);
		}
		else
#endif	/* IC_FASTLINK */
		{
		    /*
		     * Read file for symbolic link
		     */
		    vm_offset_t	buf;
		    vm_size_t	buf_size;
		    daddr_t	disk_block;
		    register struct fs *fs = fp->f_fs;

		    (void) block_map(fp, (daddr_t)0, &disk_block);
		    rc = device_read(fp->f_dev.dev_port,
				     0,
				     (recnum_t) dbtorec(&fp->f_dev,
							fsbtodb(fs,
								disk_block)),
				     (int) dblksize(fs, &(fp->f_di), 0),
				     (char **) &buf,
				     (mach_msg_type_number_t *)&buf_size);
		    if (rc)
			goto exit;

		    memcpy(namebuf, (char *)buf, (unsigned)link_len);
		    (void) vm_deallocate(mach_task_self(), buf, buf_size);
		}

		/*
		 * If relative pathname, restart at parent directory.
		 * If absolute pathname, restart at root.
		 * If pathname begins '/dev/<device>/',
		 *	restart at root of that device.
		 */
		cp = namebuf;
		if (*cp != '/') {
		    inumber = parent_inumber;
		}
#ifndef	MACH_DEV_LINK
		else
		    inumber = (ino_t)ROOTINO;
#else	/* MACH_DEV_LINK */
		else if (!strprefix(cp, "/dev/")) {
		    inumber = (ino_t)ROOTINO;
		}
		else {
		    cp += 5;
		    component = cp;
		    while ((c = *cp) != '\0' && c != '/') {
			cp++;
		    }
		    *cp = '\0';

		    /*
		     * Unmount current file system and free buffers.
		     */
		    ufs_close_file((fs_private_t)fp);

		    /*
		     * Open new root device.
		     */
		    rc = device_open(master_device_port, MACH_PORT_NULL,
				     D_READ, null_security_token,
				     component, &fp->f_dev.dev_port);

		    if (rc)
			return (rc);

		    fp->f_dev.rec_size = dev_rec_size(fp->f_dev.dev_port);

		    if (c == 0) {
			fp->f_fs = 0;
			goto out_ok;
		    }

		    *cp = c;

		    rc = mount_fs(fp);
		    if (rc)
			return (rc);

		    inumber = (ino_t)ROOTINO;
		}
#endif	/* MACH_DEV_LINK */
		if ((rc = read_inode(inumber, fp)) != 0)
		    goto exit;
	    }
	}

	/*
	 * Found terminal component.
	 */
#if MACH_DEV_LINK
    out_ok:
#endif	/* MACH_DEV_LINK */
	free(namebuf);
	*private = (fs_private_t) fp;
	return 0;

	/*
	 * At error exit, close file to free storage.
	 */
    exit:
	free(namebuf);
	ufs_close_file((fs_private_t)fp);
	return rc;
}

/*
 * Close file - free all storage used.
 */
void
ufs_close_file(
	fs_private_t private)
{
	register struct ufs_file *fp = (struct ufs_file *)private;
	/*
	 * Free the disk super-block.
	 */
	unmount_fs(fp);

	/*
	 * Free the inode and data buffers.
	 */
	free_file_buffers(fp);
	free(fp);
}

/*
 * Copy a portion of a file into kernel memory.
 * Cross block boundaries when necessary.
 */
int
ufs_read_file(
	fs_private_t private,
	vm_offset_t		offset,
	vm_offset_t		start,
	vm_size_t		size)
{
  	register struct ufs_file	*fp = (struct ufs_file *)private;
	int			rc;
	register vm_size_t	csize;
	vm_offset_t		buf;
	vm_size_t		buf_size;

	while (size != 0) {
	    buf = start;
	    buf_size = size;
	    rc = buf_read_file(fp, offset, &buf, &buf_size);
	    if (rc)
		return rc;

	    csize = size;
	    if (csize > buf_size)
		csize = buf_size;
	    if (csize == 0)
		return (size == 0) ? 0 : FS_NOT_IN_FILE;

	    if (buf != start)
		memcpy((char *)start, (const char *)buf, csize);

	    offset += csize;
	    start  += csize;
	    size   -= csize;
	}

	return 0;
}

boolean_t
ufs_file_is_directory(fs_private_t private)
{
  	register struct ufs_file	*fp = (struct ufs_file *)private;
	return((fp->f_di.di_mode & IFMT) == IFDIR);
}

size_t
ufs_file_size(fs_private_t private)
{
  	register struct ufs_file	*fp = (struct ufs_file *)private;
	return(fp->f_di.di_size);
}

boolean_t
ufs_file_is_executable(fs_private_t private)
{
  	register struct ufs_file	*fp = (struct ufs_file *)private;
	if ((fp->f_di.di_mode & IFMT) != IFREG ||
	    (fp->f_di.di_mode & (IEXEC|IEXEC>>3|IEXEC>>6)) == 0) {
		return (FALSE);
	}
	return(TRUE);
}
