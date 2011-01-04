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
 * CMU HISTORY
 * Revision 2.3  93/01/14  17:08:57  danner
 * 	64bit clean. Assumes 32bit filesystem. Breaks 16bit systems(PDP-11).
 * 	[92/11/30            af]
 * 
 * Revision 2.2  92/01/03  19:56:32  dbg
 * 	Add type names.  Add block-size definitions (should be
 * 	in device/disk_status.h).
 * 	[91/08/02            dbg]
 * 
 * Revision 2.4  91/05/14  15:22:01  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:01:05  mrt
 * 	Changed to new copyright
 * 	[91/01/28  14:54:37  mrt]
 * 
 * Revision 2.2  90/08/27  21:45:05  dbg
 * 	Created.
 * 	[90/07/16            dbg]
 * 
 */

/*
 * Common definitions for Berkeley Fast File System.
 */

/*
 * Compatibility definitions for disk IO.
 */

typedef unsigned long   ino_t;          /* inode number */

/*
 * Disk devices IO
 */
#ifdef  HPOSFCOMPAT
#define	DEV_BSIZE 1024
#define DEV_BSHIFT 10
#else
#define	DEV_BSIZE 512
#define DEV_BSHIFT 9
#endif /* HPOSFCOMPAT */

/*
 * Conversion between bytes and disk blocks.
 */
#define	btodb(byte_offset)	((byte_offset) >> DEV_BSHIFT)
#define	dbtob(block_number)	((block_number) << DEV_BSHIFT)

/*
 * The file system is made out of blocks of at most MAXBSIZE units,
 * with smaller units (fragments) only in the last direct block.
 * MAXBSIZE primarily determines the size of buffers in the buffer
 * pool.  It may be made larger without any effect on existing
 * file systems; however, making it smaller may make some file
 * systems unmountable.
 *
 * Note that the disk devices are assumed to have DEV_BSIZE "sectors"
 * and that fragments must be some multiple of this size.
 */
#define	MAXBSIZE	8192
#define	MAXFRAG		8

/*
 * MAXPATHLEN defines the longest permissible path length
 * after expanding symbolic links.
 *
 * MAXSYMLINKS defines the maximum number of symbolic links
 * that may be expanded in a path name.  It should be set
 * high enough to allow all legitimate uses, but halt infinite
 * loops reasonably quickly.
 */

#define PATH_MAX	1024
#define MAXPATHLEN	PATH_MAX
#define	MAXSYMLINKS	8

