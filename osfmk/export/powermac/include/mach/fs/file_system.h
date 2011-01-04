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
 * File system independent interfaces definitions
 */

#ifndef _FS_FILE_SYSTEM_H_
#define _FS_FILE_SYSTEM_H_

#include <string.h>
#include <mach.h>
#include <device/device_types.h>
#include <mach/boot_info.h>

typedef struct fs_ops 	*fs_ops_t;
typedef void 		*fs_private_t;

struct device {
	mach_port_t	dev_port;	/* port to device */
	unsigned int	rec_size;	/* record size */
};

/*
 * In-core open file.
 */
struct file {
	fs_ops_t	f_ops;
	fs_private_t	f_private;	/* file system dependent */
	struct device	f_dev;		/* device */
};

struct fs_ops {
	int		(*open_file)(struct device *,
				     const char *,
				     fs_private_t *);
	void		(*close_file)(fs_private_t);
    	int 		(*read_file)(fs_private_t,
				     vm_offset_t,
				     vm_offset_t,
				     vm_size_t);
    	size_t 		(*file_size)(fs_private_t);
	boolean_t 	(*file_is_directory)(fs_private_t);
	boolean_t 	(*file_is_executable)(fs_private_t);
};

/*
 * Error codes for file system errors.
 */
#define	FS_NOT_DIRECTORY	5000	/* not a directory */
#define	FS_NO_ENTRY		5001	/* name not found */
#define	FS_NAME_TOO_LONG	5002	/* name too long */
#define	FS_SYMLINK_LOOP		5003	/* symbolic link loop */
#define	FS_INVALID_FS		5004	/* bad file system */
#define	FS_NOT_IN_FILE		5005	/* offset not in file */
#define	FS_INVALID_PARAMETER	5006	/* bad parameter to a routine */

extern int open_file(mach_port_t, const char *, struct file *);
extern void close_file(struct file *);
extern int read_file(struct file *, vm_offset_t, vm_offset_t, vm_size_t);
extern size_t file_size(struct file *);
extern boolean_t file_is_directory(struct file *);
extern boolean_t file_is_executable(struct file *);

#endif /* _FS_FILE_SYSTEM_H_ */
