#define DEFLUB 1
int deflub = 1;
/*
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
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
 * Stand-alone hfs file reading package.
 * modified from original hfslib package by Brad Midgley
 * brad@pht.com. GPL'd.
 */

#include <hfs/mainhfs.h>

#include "externs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <device/device_types.h>
#include <device/device.h>

security_token_t null_security_token;

struct fs_ops hfs_ops = {
	hfs_open_file,
	hfs_close_file,
	hfs_read_file,
	hfs_file_size,
	hfs_file_is_directory,
	hfs_file_is_executable
};

/*
 * Open a file.
 */
int
hfs_open_file(
	struct device *dev,
	const char *path,
	fs_private_t *private)
{
	struct hfs_file *fp;
	hfsvol *hvol;
	char macpath[MAXNAMLEN+1];
	int scan;

#ifdef	DEBUG
	if(debug)
		printf("hfs_open_file(dev, %s)\n", path);
#endif 

	if (path == 0 || *path == '\0') {
	    return FS_NO_ENTRY;
	}

	hvol = hfs_mount(dev);
	if (hvol == NULL)
		return FS_INVALID_FS;

	fp = (struct hfs_file *)malloc(sizeof(struct hfs_file));
	memset((void *)fp, '\0', sizeof(struct hfs_file));

	/* convert /'s to :'s */
	for (scan = 0; scan < MAXNAMLEN && path[scan]; scan++) {
	  if (path[scan] == '/')
	    macpath[scan] = ':';
	  else
	    macpath[scan] = path[scan];
	}
	macpath[scan] = '\0';

#ifdef	DEBUG
	if(debug)
		printf("hfs filesystem OK! opening %s\n", macpath);
#endif 

	fp->hfile = hfs_open(hvol, macpath);
	if (fp->hfile == NULL) {
	  free(fp);
	  return FS_NO_ENTRY;
	}

#ifdef	DEBUG
	if(debug)
		printf("hfs file found OK!\n");
#endif 

	*private = (fs_private_t) fp;
	return 0;
}

/*
 * Close file - free all storage used.
 */
void
hfs_close_file(
	fs_private_t private)
{
	register struct hfs_file *fp = (struct hfs_file *)private;
	hfsvol *hvol;

	/*
	 * get the filesystem mount handle
	 */
	hvol = fp->hfile->vol;

	/*
	 * close the file
	 */
	hfs_close(fp->hfile);

	/* 
	 * unmount the volume if there are no other open files
	 */
	if (hvol->files == NULL)
		hfs_umount(hvol);

	free(fp);
}

/*
 * Copy a portion of a file into kernel memory.
 * Cross block boundaries when necessary.
 */
int
hfs_read_file(
	fs_private_t private,
	vm_offset_t		offset,
	vm_offset_t		start,
	vm_size_t		size)
{
  	register struct hfs_file	*fp = (struct hfs_file *)private;
	register vm_size_t	csize;
	vm_offset_t		buf;
	vm_size_t		buf_size;

#ifdef	DEBUG
	if(debug)
		printf("hfs_read_file(%d, %d)\n", offset, size);
#endif 
	while (size != 0) {
	    buf = start;
	    buf_size = size;

	    /*	    rc = buf_read_file(fp, offset, &buf, &buf_size);
	    if (rc)
		return (rc);*/

	    if (hfs_lseek(fp->hfile, (long)offset, SEEK_SET) != (long)offset) 
	      return -1;
	    buf_size = (vm_size_t)hfs_read(fp->hfile, (void *)buf, (unsigned long)buf_size);
	    if (buf_size < 0)
	      return -1;

	    csize = size;
	    if (csize > buf_size)
		csize = buf_size;
	    if (csize == 0)
		break;

	    if (buf != start)            /* shouldn't we also dealloc buf after the copy? */
		memcpy((char *)start, (const char *)buf, csize);

	    offset += csize;
	    start  += csize;
	    size   -= csize;
	}
	return (0);
}

boolean_t
hfs_file_is_directory(fs_private_t private)
{
	/* if it's a file we can't have opened it in the first place */

#ifdef	DEBUG
	if(debug)
		printf("hfs_file_is_directory\n");
#endif 
	return FALSE;
}

size_t
hfs_file_size(fs_private_t private)
{
  	register struct hfs_file	*fp = (struct hfs_file *)private;
	hfsdirent ent;
	
	hfs_fstat(fp->hfile, &ent);

	return((size_t)(ent.dsize));
}

boolean_t
hfs_file_is_executable(fs_private_t private)
{
	return(TRUE);
}

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
 * 
 */
/*
 * MkLinux
 */
