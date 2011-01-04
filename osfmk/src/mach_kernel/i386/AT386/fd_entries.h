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

extern io_return_t	fdopen(
				dev_t			dev,
				dev_mode_t		flag,
				io_req_t		ior);
extern void		fdclose(
				dev_t			dev);
extern io_return_t	fdread(
				dev_t			dev,
				io_req_t		ior);
extern io_return_t	fdwrite(
				dev_t			dev,
				io_req_t		ior);
extern io_return_t	fdgetstat(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				natural_t		* count);
extern io_return_t	fdsetstat(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				natural_t		count);
extern io_return_t	fddevinfo(
				dev_t			dev,
				dev_flavor_t		flavor,
				char			* info);
extern void		fdintr(
				int			ctrl);

