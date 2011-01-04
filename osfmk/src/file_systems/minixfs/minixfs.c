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
 * Stand-alone MINIX file-system file reading package.
 */

#include "minixfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <device/device_types.h>
#include <device/device.h>

#define mutex_lock(a)
#define mutex_unlock(a)
#define mutex_init(a)

security_token_t null_security_token;

static void free_file_buffers(
		struct minixfs_file *);

static int read_inode(
		ino_t,
		struct minixfs_file *);

static int block_map(
		struct minixfs_file *,
		minix_daddr_t,
		minix_daddr_t *);

static int buf_read_file(
		struct minixfs_file *,
		vm_offset_t,
		vm_offset_t *,
		vm_size_t *);

static int search_directory(
		char *,
	        struct minixfs_file *,
		ino_t *);

static int read_fs(
		struct device *,
		struct minix_super_block **);

static int mount_fs(
		struct minixfs_file *);

static void unmount_fs(
		struct minixfs_file *);

struct fs_ops minixfs_ops = {
	minixfs_open_file,
	minixfs_close_file,
	minixfs_read_file,
	minixfs_file_size,
	minixfs_file_is_directory,
	minixfs_file_is_executable
};

/*
 * Free file buffers, but don't close file.
 */
static void
free_file_buffers(register struct minixfs_file *fp)
{
	register int level;

	/*
	 * Free the indirect blocks
	 */
	for (level = 0; level < MINIX_NIADDR; level++) {
	    if (fp->f_blk[level] != 0) {
		(void) vm_deallocate(mach_task_self(),
				     fp->f_blk[level],
				     fp->f_blksize[level]);
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
				 fp->f_buf_size);
	    fp->f_buf = 0;
	}
	fp->f_buf_blkno = -1;
}

/*
 * Read a new inode into a file structure.
 */
static int
read_inode(ino_t inumber, register struct minixfs_file *fp)
{
	vm_offset_t		buf;
	mach_msg_type_number_t	buf_size;
	register
	struct minix_super_block	*fs;
	minix_daddr_t		disk_block;
	kern_return_t		rc;

	fs = fp->f_fs;
	disk_block = minix_ino2blk(fs, inumber);
#ifdef	DEBUG
	if(debug)
		printf("read_inode(%d), disk_block=0x%08x, rec=0x%08x\n",
		       (unsigned int)inumber,
		       (unsigned int)disk_block,
		       dbtorec(&fp->f_dev,minix_fsbtodb(fp->f_fs,disk_block)));
#endif 

	rc = device_read(fp->f_dev.dev_port,
			 0,
			 (recnum_t) dbtorec(&fp->f_dev,
					    minix_fsbtodb(fp->f_fs,
							  disk_block)),
			 (int) MINIX_BLOCK_SIZE,
			 (char **)&buf,
			 &buf_size);
	if (rc != KERN_SUCCESS)
	    return (rc);

	{
	    register struct minix_inode *dp;

	    dp = (struct minix_inode *)buf;
	    dp += minix_itoo(fs, inumber);
	    fp->i_ic = *dp;
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
static int
block_map(
	struct minixfs_file	*fp,
	minix_daddr_t		file_block,
	minix_daddr_t		*disk_block_p)	/* out */
{
	int		level;
	int		idx;
	minix_daddr_t	ind_block_num;
	kern_return_t	rc;

	vm_offset_t	olddata[MINIX_NIADDR+1];
	vm_size_t	oldsize[MINIX_NIADDR+1];

#ifdef	DEBUG
	int i = file_block;
	if(debug)
		printf("block_map(%d)\n", i);
#endif 
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

	mutex_lock(&fp->f_lock);

	if (file_block < MINIX_NDADDR) {
	    /* Direct block. */
	    *disk_block_p = fp->i_ic.i_zone[file_block];
	    mutex_unlock(&fp->f_lock);
	    return (0);
	}

	file_block -= MINIX_NDADDR;

	/*
	 * nindir[0] = NINDIR
	 * nindir[1] = NINDIR**2
	 * nindir[2] = NINDIR**3
	 *	etc
	 */
	for (level = 0; level < MINIX_NIADDR; level++) {
	    if (file_block < fp->f_nindir[level])
		break;
	    file_block -= fp->f_nindir[level];
	}
	if (level == MINIX_NIADDR) {
	    /* Block number too high */
	    mutex_unlock(&fp->f_lock);
	    return (FS_NOT_IN_FILE);
	}

	ind_block_num = fp->i_ic.i_zone[level + MINIX_NDADDR];

	/*
	 * Initialize array of blocks to free.
	 */
	for (idx = 0; idx < MINIX_NIADDR; idx++)
	    oldsize[idx] = 0;

	for (; level >= 0; level--) {

	    vm_offset_t	data;
	    mach_msg_type_number_t	size;

	    if (ind_block_num == 0)
		break;

	    if (fp->f_blkno[level] == ind_block_num) {
		/*
		 *	Cache hit.  Just pick up the data.
		 */

		data = fp->f_blk[level];
	    }
	    else {
		/*
		 *	Drop our lock while doing the read.
		 *	(The f_dev and f_fs fields don`t change.)
		 */
		mutex_unlock(&fp->f_lock);

		rc = device_read(fp->f_dev.dev_port,
				 0,
				 (recnum_t) dbtorec(&fp->f_dev,
					    minix_fsbtodb(fp->f_fs,
							  ind_block_num)),
				 MINIX_BLOCK_SIZE,
				 (char **)&data,
				 &size);
		if (rc != KERN_SUCCESS)
		    return (rc);

		/*
		 *	See if we can cache the data.  Need a write lock to
		 *	do this.  While we hold the write lock, we can`t do
		 *	*anything* which might block for memory.  Otherwise
		 *	a non-privileged thread might deadlock with the
		 *	privileged threads.  We can`t block while taking the
		 *	write lock.  Otherwise a non-privileged thread
		 *	blocked in the vm_deallocate (while holding a read
		 *	lock) will block a privileged thread.  For the same
		 *	reason, we can`t take a read lock and then use
		 *	lock_read_to_write.
		 */

		mutex_lock(&fp->f_lock);

		olddata[level] = fp->f_blk[level];
		oldsize[level] = fp->f_blksize[level];

		fp->f_blkno[level] = ind_block_num;
		fp->f_blk[level] = data;
		fp->f_blksize[level] = size;

		/*
		 *	Return to holding a read lock, and
		 *	dispose of old data.
		 */

	    }

	    if (level > 0) {
		idx = file_block / fp->f_nindir[level-1];
		file_block %= fp->f_nindir[level-1];
	    }
	    else
		idx = file_block;

	    ind_block_num = ((minix_daddr_t *)data)[idx];
	}

	mutex_unlock(&fp->f_lock);

	/*
	 * After unlocking the file, free any blocks that
	 * we need to free.
	 */
	for (idx = 0; idx < MINIX_NIADDR; idx++)
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
static int
buf_read_file(
	register struct minixfs_file	*fp,
	vm_offset_t			offset,
	vm_offset_t			*buf_p,		/* out */
	vm_size_t			*size_p)	/* out */
{
	register
	struct minix_super_block	*fs;
	vm_offset_t		off;
	register minix_daddr_t	file_block;
	minix_daddr_t		disk_block;
	int			rc;
	vm_offset_t		block_size;

#ifdef	DEBUG
	if(debug)
		printf("buf_read_file(%d, %d)\n", offset, *size_p);
#endif 
	if (offset >= fp->i_ic.i_size)
	    return (FS_NOT_IN_FILE);

	fs = fp->f_fs;

	off = minix_blkoff(fs, offset);
	file_block = minix_lblkno(fs, offset);
	block_size = minix_blksize(fs, fp, file_block);

	if (off || (!*buf_p) || *size_p < block_size ||
	    ((*buf_p) & (fp->f_dev.rec_size-1))) {
	    if (((daddr_t) file_block) != fp->f_buf_blkno) {
	        rc = block_map(fp, file_block, &disk_block);
		if (rc != 0)
		    return (rc);

		if (fp->f_buf) {
		    (void)vm_deallocate(mach_task_self(),
					fp->f_buf,
					fp->f_buf_size);
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
							minix_fsbtodb(fs,
								disk_block)),
				     (int) block_size,
				     (char **) &fp->f_buf,
				     (unsigned int *)&fp->f_buf_size);
	        }
		if (rc)
		    return (rc);

	        fp->f_buf_blkno = (daddr_t) file_block;
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
							minix_fsbtodb(fs,
								disk_block)),
				     (int) block_size,
				     (vm_address_t) *buf_p,
				     (unsigned int *)size_p);
	        if (rc)
		    return (rc);
	}

	/*
	 * Truncate buffer at end of file.
	 */
	if (*size_p > fp->i_ic.i_size - offset)
	    *size_p = fp->i_ic.i_size - offset;

	return (0);
}

/*
 * Search a directory for a name and return its
 * i_number.
 */
static int
search_directory(
	char *name,
	register struct minixfs_file *fp,
	ino_t *inumber_p)
{
	vm_offset_t	buf;
	vm_size_t	buf_size;
	vm_offset_t	offset;
	register struct minix_directory_entry *dp;
	int		length;
	kern_return_t	rc;
	char		tmp_name[15];

#ifdef	DEBUG
	if(debug)
		printf("search_directory(%s)\n", name);
#endif 
	length = strlen(name);

	offset = 0;
	while (offset < fp->i_ic.i_size) {
	    buf = 0;
	    buf_size = 0;
	    rc = buf_read_file(fp, offset, &buf, &buf_size);
	    if (rc != KERN_SUCCESS)
		return (rc);

	    dp = (struct minix_directory_entry *)buf;
	    if (dp->inode != 0) {
		strncpy (tmp_name, dp->name, MINIX_NAME_LEN /* XXX it's 14 */);
		tmp_name[MINIX_NAME_LEN] = '\0';
		if (strlen(tmp_name) == length &&
		    !strcmp(name, tmp_name))
	    	{
		    /* found entry */
		    *inumber_p = dp->inode;
		    return (0);
		}
	    }
	    offset += 16 /* MINIX dir. entry length - MINIX FS Ver. 1 */;
	}
	return (FS_NO_ENTRY);
}

static int
read_fs(
	struct device *dev,
	struct minix_super_block **fsp)
{
	register
	struct minix_super_block *fs;
	vm_offset_t		buf;
	mach_msg_type_number_t	buf_size;
	int			error;

#ifdef	DEBUG
	if(debug)
		printf("read_fs()\n");
#endif 
	/*
	 * Read the super block
	 */
	error = device_read(dev->dev_port, 0, 
			    (recnum_t) dbtorec(dev, MINIX_SBLOCK), MINIX_SBSIZE,
			    (char **) &buf, &buf_size);
	if (error)
	    return (error);

	/*
	 * Check the superblock
	 */
#if 0
	{
		int i;
		for (i = 0; i < 256; i++){
			if ((i % 16)==0)
				printf("\n%4d : ",i);
			printf("%02x ",*(((unsigned char*)buf)+i));
		}
	}
#endif
	fs = (struct minix_super_block *)buf;
	if (fs->s_magic != MINIX_SUPER_MAGIC) {
		(void) vm_deallocate(mach_task_self(), buf, buf_size);
		return (FS_INVALID_FS);
	}

	*fsp = fs;

	return 0;
}

static int
mount_fs(register struct minixfs_file	*fp)
{
	register struct minix_super_block *fs;
	int error;

	error = read_fs(&fp->f_dev, &fp->f_fs);
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
	    for (level = 0; level < MINIX_NIADDR; level++) {
		mult *= MINIX_NINDIR(fs);
		fp->f_nindir[level] = mult;
	    }
	}

	return (0);
}

static void
unmount_fs(register struct minixfs_file *fp)
{
	if (file_is_structured(fp)) {
	    (void) vm_deallocate(mach_task_self(),
				 (vm_offset_t) fp->f_fs,
				 MINIX_SBSIZE);
	    fp->f_fs = 0;
	}
}

/*
 * Open a file.
 */
int
minixfs_open_file(
	struct device *dev,
	const char *path,
	fs_private_t *private)
{
#define	RETURN(code)	{ rc = (code); goto exit; }

	register char	*cp, *component;
	register int	c;	/* char */
	register int	rc;
	ino_t		inumber, parent_inumber;
	char	        *namebuf;
	struct minixfs_file *fp;

#ifdef	DEBUG
 	if(debug)
		printf("minixfs_open_file(%s)\n", path);
#endif 
	if (path == 0 || *path == '\0') {
	    return FS_NO_ENTRY;
	}

	namebuf = (char*)malloc((size_t)(PATH_MAX+1));
	fp = (struct minixfs_file *)malloc(sizeof(struct minixfs_file));
	memset((char *)fp, 0, sizeof(struct minixfs_file));
	fp->f_dev = *dev;
	/*
	 * Copy name into buffer to allow modifying it.
	 */
	strcpy(namebuf, path);

	cp = namebuf;

	rc = mount_fs(fp);
	if (rc)
	    return rc;

	inumber = (ino_t) MINIX_ROOTINO;
	if ((rc = read_inode(inumber, fp)) != 0) {
	    printf("can't read root inode\n");
	    goto exit;
	}

	while (*cp) {
	    /*
	     * Check that current node is a directory.
	     */
	    if ((fp->i_ic.i_mode & IFMT) != IFDIR)
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
		    if (len++ > MINIX_MAXNAMLEN)
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
	}

	/*
	 * Found terminal component.
	 */
	mutex_init(&fp->f_lock);
	free(namebuf);
	*private = (fs_private_t) fp;
	return 0;

	/*
	 * At error exit, close file to free storage.
	 */
    exit:
	free(namebuf);
	minixfs_close_file((fs_private_t)fp);
	return rc;
}

/*
 * Close file - free all storage used.
 */
void
minixfs_close_file(
	fs_private_t private)
{
	register struct minixfs_file *fp = (struct minixfs_file *)private;

	/*
	 * Free the disk super-block.
	 */
	unmount_fs(fp);

	/*
	 * Free the inode and data buffers.
	 */
	free_file_buffers(fp);
}

/*
 * Copy a portion of a file into kernel memory.
 * Cross block boundaries when necessary.
 */
int
minixfs_read_file(
	fs_private_t private,
	vm_offset_t		offset,
	vm_offset_t		start,
	vm_size_t		size)
{
  	register struct minixfs_file	*fp = (struct minixfs_file *)private;
	int			rc;
	register vm_size_t	csize;
	vm_offset_t		buf;
	vm_size_t		buf_size;

#ifdef	DEBUG
	if(debug)
		printf("minixfs_read_file(%d, %d)\n", offset, size);
#endif 
	while (size != 0) {
	    buf = start;
	    buf_size = size;
	    rc = buf_read_file(fp, offset, &buf, &buf_size);
	    if (rc)
		return (rc);

	    csize = size;
	    if (csize > buf_size)
		csize = buf_size;
	    if (csize == 0)
		break;

	    if (buf != start)
		memcpy((char *)start, (const char *)buf, csize);

	    offset += csize;
	    start  += csize;
	    size   -= csize;
	}
#if 0
	if (resid)
	    *resid = size;
#endif
	return (0);
}

boolean_t
minixfs_file_is_directory(fs_private_t private)
{
  	register struct minixfs_file	*fp = (struct minixfs_file *)private;
	return((fp->i_ic.i_mode & IFMT) == IFDIR);
}

size_t
minixfs_file_size(fs_private_t private)
{
  	register struct minixfs_file	*fp = (struct minixfs_file *)private;
	return(fp->i_ic.i_size);
}

boolean_t
minixfs_file_is_executable(fs_private_t private)
{
  	register struct minixfs_file	*fp = (struct minixfs_file *)private;
	if ((fp->i_ic.i_mode & IFMT) != IFREG
#if 0
	    || (fp->i_ic.i_mode & (IEXEC|IEXEC>>3|IEXEC>>6)) == 0
#endif
	    ) {
		return (FALSE);
	}
	return(TRUE);
}
