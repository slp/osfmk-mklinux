/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_PARENT_SERVER_H
#define _OSFMACH3_PARENT_SERVER_H

#include <mach/exception.h>

#define PARENT_SERVER_NONE	0
#define PARENT_SERVER_LINUX	1	
#define PARENT_SERVER_OSF1	2

extern int parent_server;
extern int parent_server_type;
extern void parent_server_set_type(int type);

extern int parent_server_errno;

extern int parent_server_getpid(void);
extern void parent_server_catchall_signals(void);
extern int parent_server_write(int fd, char *buf, int count);
extern int parent_server_read(int fd, char *buf, int count);
extern int parent_server_ioctl(int fd, int request, char *data);
extern int parent_server_exit(int error_code);
extern int parent_server_grab_console(void);
extern int parent_server_release_console(void);
extern exception_mask_t parent_server_syscall_exc_mask(void);
extern int parent_server_get_mach_privilege(void);

#endif	/* _OSFMACH3_PARENT_SERVER_H */
