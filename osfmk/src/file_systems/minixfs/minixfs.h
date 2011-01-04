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

#ifndef	_MINIXFS_H_
#define	_MINIXFS_H_

/*
 * MINIX file-system entry points and structure definitions
 */

#include <string.h>
#include <types.h>
#include "../ext2fs/defs.h"
#include <minix_fs.h>
#include <mach.h>
#include <minix_ffs_compat.h>
#include <device/device_types.h>
#include <file_system.h>
#include "externs.h"

/*
 * Exported routines.
 */

extern int minixfs_open_file(struct device *, const char *, fs_private_t *);
extern void minixfs_close_file(fs_private_t);
extern int minixfs_read_file(fs_private_t, vm_offset_t, vm_offset_t, vm_size_t);
extern size_t minixfs_file_size(fs_private_t);
extern boolean_t minixfs_file_is_directory(fs_private_t);
extern boolean_t minixfs_file_is_executable(fs_private_t);


/*
 * In-core open file.
 */
struct minixfs_file {
	struct device		f_dev;		/* device */
	struct minix_super_block*f_fs;		/* pointer to super-block */
	struct minix_inode	i_ic;		/* copy of on-disk inode */
	int			f_nindir[MINIX_NIADDR+1];
						/* number of blocks mapped by
						   indirect block at level i */
	vm_offset_t		f_blk[MINIX_NIADDR];	
						/* buffer for indirect block at
						   level i */
	vm_size_t		f_blksize[MINIX_NIADDR];
						/* size of buffer */
	daddr_t			f_blkno[MINIX_NIADDR];
						/* disk address of block in
						   buffer */
	vm_offset_t		f_buf;		/* buffer for data block */
	vm_size_t		f_buf_size;	/* size of data block */
	daddr_t			f_buf_blkno;	/* block number of data block */
};

#define file_is_structured(_fp_)	((_fp_)->f_fs != 0)

/*
 * In-core open file, with in-core block map.
 */
struct file_direct {
	mach_port_t	fd_dev;		/* port to device */
	daddr_t *	fd_blocks;	/* array of disk block addresses */
	long		fd_size;	/* number of blocks in the array */
	long		fd_bsize;	/* disk block size */
	long		fd_bshift;	/* log2(fd_bsize) */
	long		fd_fsbtodb;	/* log2(fd_bsize / disk sector size) */
};

#define	file_is_device(_fd_)		((_fd_)->fd_blocks == 0)


#endif	/* _MINIXFS_H_ */
