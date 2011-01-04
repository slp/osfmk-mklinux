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
 *	File: ux_stubs.c
 *	Author: Yves Paindaveine (yp@osf.org)
 *
 *	Provide U*X stubs to satisfy the linker without retrieving
 *	the symbols from libc/libm. Warn when the stubs are used.
 */

#include <mach_perf.h>

#define warn(func) \
        printf("%s() called in a non-U*X configuration\n", func)

char
unix_read_char() {
	warn("unix_read_char");
	return;
}

/*
 * Used unbuffered I/Os. (default for libsa_mach)
 */
unbuffered_output()
{
}

void
read()
{
	warn("read");
	return;
}

void
write()
{
	warn("write");
	return;
}

void
ux_prof_print()
{
	warn("ux_prof_print");
	return;
}

void
ux_prof_init(void) {}

kern_return_t
do_start_prof_cmd(
	mach_port_t 	server,
	char 		*command,
	unsigned int 	len)
{
	warn("do_start_prof_cmd");
	return;
}

kern_return_t
do_write_to_prof_cmd(
	mach_port_t 	server,
	char 		*buf,
	unsigned int 	len)
{
	warn("do_write_to_prof_cmd");
	return;
}

kern_return_t
do_stop_prof_cmd(
	mach_port_t server)
{
	warn("do_stop_prof_cmd");
	return;
}
