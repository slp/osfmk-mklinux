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

extern boolean_t	iopb_check_mapping(
					thread_t	thread,
					device_t	device);
extern kern_return_t	i386_io_port_add(
					thread_t	thread,
					device_t	device);
extern kern_return_t	i386_io_port_remove(
					thread_t	thread,
					device_t	device);
extern kern_return_t	i386_io_port_list(
					thread_t	thread,
					device_t	** list,
					unsigned int	* list_count);
extern void		iopb_init(void);
extern iopb_tss_t	iopb_create(void);
extern void		iopb_destroy(
					iopb_tss_t	iopb);
