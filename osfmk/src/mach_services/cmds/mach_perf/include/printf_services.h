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

#ifndef _PRINTF_SERVICES_H
#define _PRINTF_SERVICES_H

extern mach_port_t 	printf_port;
extern mach_thread_t	printf_thread;

extern int gen_printf(const char *format, ...);
extern int printf_enable(mach_port_t task);
extern int remote_printf(mach_port_t ,char *, int );
extern int print_header(struct test *test);
extern int print_results( struct time_stats *ts );
extern int print_one_sample(struct test *test, struct time_stats *ts);
extern int _printf_init();

#define printf gen_printf

#endif /* _PRINTF_SERVICES_H */
