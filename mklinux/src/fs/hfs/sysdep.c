/*
 * linux/fs/hfs/sysdep.c
 *
 * Copyright (C) 1996  Paul H. Hargrove
 * This file may be distributed under the terms of the GNU Public License.
 *
 * This file contains the code to do various system dependent things.
 *
 * "XXX" in a comment is a note to myself to consider changing something.
 *
 * In function preconditions the term "valid" applied to a pointer to
 * a structure means that the pointer is non-NULL and the structure it
 * points to has all fields initialized to consistent values.
 */

#include "hfs.h"
#include <linux/hfs_fs_sb.h>
#include <linux/hfs_fs_i.h>
#include <linux/hfs_fs.h>

/*
 * hfs_buffer_get()
 *
 * Return a buffer for the 'block'th block of the media.
 * If ('read'==0) then the buffer is not read from disk.
 */
hfs_buffer hfs_buffer_get(hfs_sysmdb sys_mdb, int block, int read) {
	hfs_buffer tmp = HFS_BAD_BUFFER;

	if (read) {
		tmp = bread(sys_mdb->s_dev, block, HFS_SECTOR_SIZE);
	} else {
		tmp = getblk(sys_mdb->s_dev, block, HFS_SECTOR_SIZE);
		if (tmp) {
			mark_buffer_uptodate(tmp, 1);
		}
	}
	if (!tmp) {
		hfs_error("hfs_fs: unable to read block 0x%08x from dev %s\n",
			  block, hfs_mdb_name(sys_mdb));
	}

	return tmp;
}
