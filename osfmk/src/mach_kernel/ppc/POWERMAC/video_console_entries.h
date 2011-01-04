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
 * 
 */
/*
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */
#include <device/device_types.h>

extern io_return_t	vcopen(
				dev_t			dev,
				dev_mode_t		flag,
				io_req_t		ior);
extern void		vcclose(
				dev_t			dev);
extern io_return_t	vcread(
				dev_t			dev,
				io_req_t		ior);
extern io_return_t	vcwrite(
				dev_t			dev,
				io_req_t		ior);
extern io_return_t	vcgetstatus(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				mach_msg_type_number_t	* status_count);
extern io_return_t	vcsetstatus(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				mach_msg_type_number_t	status_count);
extern vm_offset_t	vcmmap(
			       dev_t			dev,
			       vm_offset_t		off,
			       vm_prot_t		prot);
extern int		vcportdeath(
				dev_t			dev,
				ipc_port_t		port);
extern void		vc_putchar(
				char			ch);
extern int		vcputc(
				int			l,
				int			u,
				int			c);
extern int		vcgetc(
				int			l,
				int			u,
				boolean_t		wait,
				boolean_t		raw);

extern void video_scroll_up(unsigned long start,
			    unsigned long end,
			    unsigned long dest);

extern void video_scroll_down(unsigned long start,  /* HIGH addr */
			      unsigned long end,    /* LOW addr */
			      unsigned long dest);  /* HIGH addr */
