/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/exception.h>

#include <osfmach3/parent_server.h>

#include <linux/kernel.h>

int parent_server_type = PARENT_SERVER_NONE;

int parent_server_errno;
int errno;

void
parent_server_set_type(int type)
{
	switch (type) {
	    case PARENT_SERVER_NONE:
		printk("No parent server: running standalone.\n");
		break;
	    case PARENT_SERVER_LINUX:
		printk("Parent server is Linux.\n");
		break;
	    case PARENT_SERVER_OSF1:
		printk("Parent server is OSF/1.\n");
		break;
	    default:
		printk("Unknown parent server type (%d).\n", type);
		break;
	}
	parent_server_type = type;
}

extern int parent_linux_getpid(void);
extern int parent_osf1_getpid(void);

int
parent_server_getpid(void)
{
	int ret;

	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		ret = parent_linux_getpid();
		break;
	    case PARENT_SERVER_OSF1:
		ret = parent_osf1_getpid();
		break;
	    default:
		ret = -1;
		break;
	}

	parent_server_errno = errno;
	return ret;
}

extern void parent_linux_catchall_signals(void);
extern void parent_osf1_catchall_signals(void);

void
parent_server_catchall_signals(void)
{
	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		parent_linux_catchall_signals();
		break;
	    case PARENT_SERVER_OSF1:
		parent_osf1_catchall_signals();
		break;
	    default:
		break;
	}

	parent_server_errno = errno;
}

extern int parent_linux_write(int fd, char *buf, int count);
extern int parent_osf1_write(int fd, char *buf, int count);

int parent_server_write(
	int	fd,
	char	*buf,
	int	count)
{
	int ret;

	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		ret = parent_linux_write(fd, buf, count);
		break;
	    case PARENT_SERVER_OSF1:
		ret = parent_osf1_write(fd, buf, count);
		break;
	    default:
		ret = -1;
		break;
	}

	parent_server_errno = errno;
	return ret;
}

extern int parent_linux_read(int fd, char *buf, int count);
extern int parent_osf1_read(int fd, char *buf, int count);

int parent_server_read(
	int	fd,
	char	*buf,
	int	count)
{
	int ret;

	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		ret = parent_linux_read(fd, buf, count);
		break;
	    case PARENT_SERVER_OSF1:
		ret = parent_osf1_read(fd, buf, count);
		break;
	    default:
		ret = -1;
		break;
	}

	parent_server_errno = errno;
	return ret;
}

extern int parent_linux_ioctl(int fd, int request, char *data);
extern int parent_osf1_ioctl(int fd, int request, char *data);

int parent_server_ioctl(
	int	fd,
	int	request,
	char	*data)
{
	int ret;

	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		ret = parent_linux_ioctl(fd, request, data);
		break;
	    case PARENT_SERVER_OSF1:
		ret = parent_osf1_ioctl(fd, request, data);
		break;
	    default:
		ret = -1;
		break;
	}

	parent_server_errno = errno;
	return ret;
}

extern int parent_linux_exit(int error_code);
extern int parent_osf1_exit(int error_code);

int parent_server_exit(
	int	error_code)
{
	int ret;

	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		ret = parent_linux_exit(error_code);
		break;
	    case PARENT_SERVER_OSF1:
		ret = parent_osf1_exit(error_code);
		break;
	    default:
		ret = -1;
		break;
	}

	parent_server_errno = errno;
	return ret;
}

extern int parent_linux_grab_console(void);
extern int parent_osf1_grab_console(void);

int parent_server_grab_console(void)
{
	int ret;

	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		ret = parent_linux_grab_console();
		break;
	    case PARENT_SERVER_OSF1:
		ret = parent_osf1_grab_console();
		break;
	    default:
		ret = -1;
		break;
	}
	parent_server_errno = errno;
	return ret;
}

extern int parent_linux_release_console(void);
extern int parent_osf1_release_console(void);

int parent_server_release_console(void)
{
	int ret;

	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		ret = parent_linux_release_console();
		break;
	    case PARENT_SERVER_OSF1:
		ret = parent_osf1_release_console();
		break;
	    default:
		ret = -1;
		break;
	}
	parent_server_errno = errno;
	return ret;
}

extern exception_mask_t parent_linux_syscall_exc_mask(void);
extern exception_mask_t parent_osf1_syscall_exc_mask(void);

exception_mask_t parent_server_syscall_exc_mask(void)
{
	exception_mask_t mask;

	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		mask = parent_linux_syscall_exc_mask();
		break;
	    case PARENT_SERVER_OSF1:
		mask = parent_osf1_syscall_exc_mask();
		break;
	    default:
		mask = 0;
		break;
	}
	return mask;
}

extern int parent_linux_get_mach_privilege(void);
extern int parent_osf1_get_mach_privilege(void);

int parent_server_get_mach_privilege(void)
{
	int ret;

	switch (parent_server_type) {
	    case PARENT_SERVER_LINUX:
		ret = parent_linux_get_mach_privilege();
		break;
	    case PARENT_SERVER_OSF1:
		ret = parent_osf1_get_mach_privilege();
		break;
	    default:
		ret = -1;
		break;
	}
	parent_server_errno = errno;
	return ret;
}
