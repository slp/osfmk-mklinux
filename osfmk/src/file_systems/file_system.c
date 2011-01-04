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
 */
/*
 * Stand-alone file system independent code
 * Used on top of mach microkernel services
 */

#include "file_system.h"
#include "externs.h"
#include <stdlib.h>
#include <stdio.h>
#include <device/device.h>
#include <device/device_types.h>

security_token_t null_security_token;

#if DEBUG
int debug = 1;

void print_fp(struct file *fp)
{
	if (debug) {
		printf("fp %x\n", (int) fp);
		if (fp) {
			printf("f_ops %x\n", (int)fp->f_ops);
			printf("f_private %x\n", (int)fp->f_private);
			printf("f_dev: port %x rec_size %x\n",
			       (int)fp->f_dev.dev_port, fp->f_dev.rec_size);
		}
	}
}

boolean_t invalid_fp(struct file *fp)
{
	if (!(fp->f_ops && fp->f_private)) {
		print_fp(fp);
		return TRUE;
	} else
		return FALSE;
}
#else
#define invalid_fp(fp) (!(fp->f_ops && fp->f_private))
#endif
/*
 * Open a file.
 */
int
open_file(
	mach_port_t master_device_port,
	const char *path,
        struct file *fp)
{
	register char	*cp, *component;
	register int	c;	/* char */
	register int	rc;
	char	        *namebuf;
	fs_ops_t	*fs_ops;

	if (path == 0 || *path == '\0') {
	    return FS_NO_ENTRY;
	}

#if	DEBUG
	if(debug)
		printf("open_file(%s)\n", path);
#endif  

	namebuf = (char*)malloc(strlen(path)+1);

	/*
	 * Copy name into buffer to allow modifying it.
	 */
	strcpy(namebuf, path);

	/*
	 * Look for '/dev/xxx' at start of path, for
	 * root device.
	 */
	if (!strprefix(namebuf, "/dev/")) {
		printf("no device name\n");
		free(namebuf);
		return FS_NO_ENTRY;
	}

	cp = namebuf + 5;	/* device */
	component = cp;
	while ((c = *cp) != '\0' && c != '/') {
	    	cp++;
	}

	*cp = '\0';

	rc = device_open(master_device_port, MACH_PORT_NULL,
			 D_READ|D_WRITE, null_security_token,
			 component, &fp->f_dev.dev_port);

	if (rc) {
		free(namebuf);
		return rc;
	}

	fp->f_dev.rec_size = dev_rec_size(fp->f_dev.dev_port);

	if (c == 0) {
		free(namebuf);
	   	fp->f_private = 0;
	    	return KERN_SUCCESS;
	}

	*cp = c;

	for (fs_ops = fs_switch; *fs_ops; fs_ops++) {
#if DEBUG
		printf("open %x\n", (*fs_ops)->open_file);
#endif
		rc = (*(*fs_ops)->open_file)(&fp->f_dev, cp, &fp->f_private);
#if DEBUG
		printf("open func returns %x\n", rc);
#endif
		if (rc == 0)
			break;
	}

	if (rc == 0)
		fp->f_ops = *fs_ops;

#if	DEBUG
	if (debug) {
		printf("open_file returns %x\n", rc);
		print_fp(fp);
	}
#endif
	free(namebuf);
	return rc;
}

/*
 * Close file - free all storage used.
 */
void
close_file(register struct file *fp)
{
#if	DEBUG
	if(debug)
		printf("close_file(%x)\n", fp);
#endif  
	if (!invalid_fp(fp))
		(*fp->f_ops->close_file)(fp->f_private);
	fp->f_ops = 0;
	fp->f_private = 0;
}

/*
 * Copy a portion of a file into kernel memory.
 * Cross block boundaries when necessary.
 */
int
read_file(register struct file	*fp,
	  vm_offset_t		offset,
	  vm_offset_t		start,
	  vm_size_t		size)
{
#if	DEBUG
	if(debug)
		printf("read_file(%x, %x, %x, %x)\n", fp, offset, start, size);
#endif  
	if (invalid_fp(fp))
		return(FS_INVALID_PARAMETER);
	return (*fp->f_ops->read_file)(fp->f_private, offset, start, size);
}

boolean_t
file_is_directory(struct file *fp)
{
	if (invalid_fp(fp))
		return(FS_INVALID_PARAMETER);
	return (*fp->f_ops->file_is_directory)(fp->f_private);
}

size_t
file_size(struct file *fp)
{
	if (invalid_fp(fp))
		return(FS_INVALID_PARAMETER);
	return (*fp->f_ops->file_size)(fp->f_private);
}

boolean_t
file_is_executable(struct file *fp)
{
	if (invalid_fp(fp))
		return(FALSE);
	return (*fp->f_ops->file_is_executable)(fp->f_private);
}

/*
 * Return TRUE if string 2 is a prefix of string 1.
 */
boolean_t
strprefix(
	register const char *s1,
	register const char *s2)
{
	register int	c;

	do {
		c = *s2++;
		if (c == '\0')
			return ((boolean_t) TRUE); /* s2 ended w/o difference,
						      includes case where s2
						      is the empty string */
	} while (c == *s1++);

	return ((boolean_t) FALSE);  /* found a difference before end of s2 */
}

unsigned int 
dev_rec_size(
	long dev)
{
	int		info[DEV_GET_SIZE_COUNT];
	mach_msg_type_number_t
			info_count;

	info_count = DEV_GET_SIZE_COUNT;
	if (device_get_status(dev, DEV_GET_SIZE,
			      info, &info_count) != KERN_SUCCESS) {
		printf("dev_get_status(DEV_GET_SIZE) failed - ABORTING\n");
		while (1)
			;
	}
	return(info[DEV_GET_SIZE_RECORD_SIZE]);
}

/* 
 * ovbcopy - like bcopy, but recognizes overlapping ranges and handles 
 *           them correctly.
 */
void
ovbcopy(char *from, char *to, size_t bytes)
{
	/* Assume that bcopy copies left-to-right (low addr first). */
	if (from + bytes <= to || to + bytes <= from || to == from)
		memcpy(to, from, bytes);	/* non-overlapping or no-op*/
	else if (from > to)
		memcpy(to, from, bytes);	/* overlapping but OK */
	else {
		/* to > from: overlapping, and must copy right-to-left. */
		from += bytes - 1;
		to += bytes - 1;
		while (bytes-- > 0)
			*to-- = *from--;
	}
}
