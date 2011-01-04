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

#include <mach.h>

/* default_pager.c */
extern mach_msg_return_t mach_msg_server(boolean_t (*)(mach_msg_header_t *,
						       mach_msg_header_t *),
					 mach_msg_size_t,
					 mach_port_t,
					 mach_msg_options_t);
extern void *kalloc(size_t);
extern void kfree(void *, size_t);
extern boolean_t default_pager_object_server(mach_msg_header_t *,
					     mach_msg_header_t *);
extern char *strbuild(char *, ...);

/* file_io.c */
extern boolean_t strprefix(const char *, const char *);
extern void ovbcopy(char *, char *, size_t);
