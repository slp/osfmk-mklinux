/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/exception.h>

#include <linux/signal.h>
#include <linux/kernel.h>

/*
 * OSF/1 doesn't run on the PowerPC (yet), so these routines aren't used.
 */

int
parent_osf1_getpid(void)
{
	panic("parent_osf1_getpid");
}

int
parent_osf1_sigaction(
	int			sig,
	const struct sigaction	*nsa,
	struct sigaction	*osa)
{
	panic("parent_osf1_sigaction");
}

int
parent_osf1_write(
	int	fd,
	char *	buf,
	int	count)
{
	panic("parent_osf1_write");
}

int parent_osf1_read(
	int	fd,
	char 	*buf,
	int	count)
{
	panic("parent_osf1_read");
}

int parent_osf1_ioctl(
	int	fd,
	int	request,
	char 	*data)
{
	panic("parent_osf1_ioctl");
}

int parent_osf1_exit(
	int error_code)
{
	panic("parent_osf1_exit");
}

int parent_osf1_iopl(
	int level)
{
	panic("parent_osf1_iopl");
}
