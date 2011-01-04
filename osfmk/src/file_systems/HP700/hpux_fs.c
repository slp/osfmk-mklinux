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

#include "file_system.h"
#include "externs.h"
#include <device/device.h>
#include <ufs/fs.h>
#include <hpux_fs.h>


#define max(a, b) (((a) > (b)) ? a : b)

int read_hpux_fs(struct device *, struct fs **);

int (*machine_read_fs)(struct device *, struct fs **) = read_hpux_fs;

int
read_hpux_fs(struct device *dev, struct fs **fsp)
{
	register struct hpux_fs *hpux_fs;
	struct fs 	*osf1_fs;
	vm_offset_t	buf;
	vm_size_t	buf_size;
	int		error;

	error = device_read(dev->dev_port, 0,
			    (recnum_t) dbtorec(dev, HPUX_SBLOCK), SBSIZE,
			    (char **) &buf, (unsigned int *)&buf_size);
	if (error)
	    return (error);

	hpux_fs = (struct hpux_fs *)buf;
	if (hpux_fs->fs_magic != HPUX_FS_MAGIC || 
	    hpux_fs->fs_bsize > MAXBSIZE ||
	    hpux_fs->fs_bsize < sizeof(struct hpux_fs)) {
		(void) vm_deallocate(mach_task_self(), buf, buf_size);
		return (FS_INVALID_FS);
	}

	error = vm_allocate(mach_task_self(), (vm_offset_t *)&osf1_fs,
			    SBSIZE, TRUE); 

	if (error)
	    return (error);

	hpux_to_osf1_fs(hpux_fs, osf1_fs);

	(void) vm_deallocate(mach_task_self(), buf, buf_size);

	/* don't read cylinder groups - we aren't modifying anything */

	*fsp = osf1_fs;
	return 0;
}
