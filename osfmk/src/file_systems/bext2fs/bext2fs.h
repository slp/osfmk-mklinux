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
 * OLD HISTORY
 * Revision 2.6  93/05/10  19:40:28  rvb
 * 	Changed "" includes to <>
 * 	[93/04/30            mrt]
 * 
 * Revision 2.5  93/05/10  17:44:51  rvb
 * 	Include files specified with quotes dont work properly
 * 	when the C file in in the master directory but the
 * 	include file is in the shadow directory. Change to
 * 	using angle brackets.
 * 	Ian Dall <DALL@hfrd.dsto.gov.au>	4/28/93
 * 	[93/05/10  13:15:57  rvb]
 * 
 * Revision 2.4  92/02/23  22:25:47  elf
 * 	Added macros for raw (unstructured) devices.
 * 	[92/02/22  18:53:33  af]
 * 
 * 	Added defs for remove_file_direct(), exported.
 * 	[92/02/19  17:31:35  af]
 * 
 * Revision 2.3  92/01/14  16:43:23  rpd
 * 	Changed <mach/mach.h> to <mach.h>.
 * 	[92/01/07  13:43:43  rpd]
 * 
 * Revision 2.2  92/01/03  19:57:19  dbg
 * 	Make file_direct self-contained: add the few fields needed from
 * 	the superblock.
 * 	[91/10/17            dbg]
 * 
 * 	Add close_file to free storage.
 * 	[91/09/25            dbg]
 * 
 * 	Move outside of kernel.
 * 	[91/09/04            dbg]
 * 
 * Revision 2.6  91/08/28  11:09:46  jsb
 * 	Added struct file_direct and associated functions.
 * 	[91/08/19            rpd]
 * 
 * Revision 2.5  91/05/18  14:28:52  rpd
 * 	Added f_lock.
 * 	[91/04/03            rpd]
 * 
 * Revision 2.4  91/05/14  15:23:08  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:01:33  mrt
 * 	Changed to new copyright
 * 	[91/01/28  14:54:57  mrt]
 * 
 * Revision 2.2  90/08/27  21:45:45  dbg
 * 	Re-create as boot_ufs/file_io.h.
 * 	[90/07/18            dbg]
 * 
 * 	Add table containing number of blocks mapped by each level of
 * 	indirect block.
 * 	[90/07/17            dbg]
 * 
 * 	Declare error codes.
 * 	[90/07/16            dbg]
 * 
 * Revision 2.3  90/06/02  14:45:38  rpd
 * 	Converted to new IPC.
 * 	[90/03/26  21:31:55  rpd]
 * 
 * Revision 2.2  89/09/08  11:22:21  dbg
 * 	Put device_port and superblock pointer into inode.
 * 	Rename structure to 'struct file'.
 * 	[89/08/24            dbg]
 * 
 * 	Version that reads the disk instead of mapping it.
 * 	[89/07/17            dbg]
 * 
 * 26-Oct-88  David Golub (dbg) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_BEXT2FS_H_
#define	_BEXT2FS_H_

/*
 * BEXT2FS entry points and structure definitions
 */

#include <string.h>
#include <types.h>
#include <bext2fs/defs.h>
#include <bext2fs/bext2_fs.h>
#include <mach.h>
#include <bext2fs/bext2_ffs_compat.h>
#include <device/device_types.h>
#include <file_system.h>

/*
 * Exported routines.
 */

extern int bext2fs_open_file(struct device *, const char *, fs_private_t *);
extern void bext2fs_close_file(fs_private_t);
extern int bext2fs_read_file(fs_private_t, vm_offset_t, vm_offset_t, vm_size_t);
extern size_t bext2fs_file_size(fs_private_t);
extern boolean_t bext2fs_file_is_directory(fs_private_t);
extern boolean_t bext2fs_file_is_executable(fs_private_t);


/*
 * In-core open file.
 */
struct bext2fs_file {
	struct device		f_dev;		/* device */
	struct bext2_super_block*f_fs;		/* pointer to super-block */
	struct bext2_group_desc*	f_gd;		/* pointer to group
						   descriptors */
	vm_size_t		f_gd_size;	/* size of group descriptors */
	struct bext2_inode	i_ic;		/* copy of on-disk inode */
	int			f_nindir[NIADDR+1];
						/* number of blocks mapped by
						   indirect block at level i */
	vm_offset_t		f_blk[NIADDR];	/* buffer for indirect block at
						   level i */
	vm_size_t		f_blksize[NIADDR];
						/* size of buffer */
	daddr_t			f_blkno[NIADDR];
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

#endif	/* _BEXT2FS_H_ */
