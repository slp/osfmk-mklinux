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
 * UFS entry points and structure definitions
 */

#include <string.h>
#include <types.h>
#include <ufs/defs.h>
#include <ufs/fs.h>
#include <ufs/dinode.h>
#include <ufs/dir.h>
#include <mach.h>
#include <device/device_types.h>
#include "file_system.h"

extern int ufs_open_file(struct device *, const char *, fs_private_t *);
extern void ufs_close_file(fs_private_t);
extern int ufs_read_file(fs_private_t, vm_offset_t, vm_offset_t, vm_size_t);
extern size_t ufs_file_size(fs_private_t);
extern boolean_t ufs_file_is_directory(fs_private_t);
extern boolean_t ufs_file_is_executable(fs_private_t);

/*
 * In-core open file.
 */
struct ufs_file {
	struct device	f_dev;		/* device */
	struct fs *	f_fs;		/* pointer to super-block */
	struct dinode	f_di;		/* copy of on-disk inode */
	int		f_nindir[NIADDR+1];
					/* number of blocks mapped by
					   indirect block at level i */
	vm_offset_t	f_blk[NIADDR];	/* buffer for indirect block at
					   level i */
	vm_size_t	f_blksize[NIADDR];
					/* size of buffer */
	daddr_t		f_blkno[NIADDR];
					/* disk address of block in buffer */
	vm_offset_t	f_buf;		/* buffer for data block */
	vm_size_t	f_buf_size;	/* size of data block */
	daddr_t		f_buf_blkno;	/* block number of data block */
};

extern struct fs_ops ufs_ops;
