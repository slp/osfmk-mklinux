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

#include <mach_perf.h>


/*
 * Suitable location would be libsa_mach
 */

/*
 * Misc routines
 */

atoi(s)
char *s;
{
	int i = 0;

	while(*s)
		i = i*10+(*s++ - '0');
	return(i);
}

/*
_Seterrno() {}
*/

rand()
{
	return(sa_random());
}

mach_port_t
task_by_pid(pid)
{
	if (pid == -1)
		return(privileged_host_port);
	else if (pid == -2)
		return(master_device_port);
	else
		return(0);
}

void  *
memchr(const void *s,
       int c,
       unsigned long n)
{
	unsigned char *res = (unsigned char *)s;
	for (; res, n--; res++)
		if (*res == c)
			return(res);
	return(0);
}

char *
strchr(const char *s,
       int c)
{
	return (char *) memchr(s, c, strlen(s));
}

bcmp(
     const char *s1,
     const char *s2, 
     int n)
{
	while(n--)
		if (*s1++ != *s2++)
			return(1);
	return(0);
}

isdigit(c)
{
	if (c >= '0' && c <= '9')
		return(1);
	else
		return(0);
}

isascii(c)
{
	if (c >= 0 && c <= 0x3f)
		return(1);
	else
		return(0);
}

