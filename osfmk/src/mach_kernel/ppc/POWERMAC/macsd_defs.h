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

/*	Close device					*/
extern void		macsd_close(
				dev_t		dev);

/*	Return device information			*/
extern io_return_t	macsd_devinfo(
				dev_t		dev,
				dev_flavor_t	flavor,
				char		*info);

/*	Get device status				*/
extern io_return_t	macsd_get_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	*status_count);

/*	Open device, subject to timeout			*/
extern io_return_t	macsd_open(
				dev_t		dev,
				dev_mode_t	mode,
				io_req_t	ior);

/*	SCSI read					*/
extern io_return_t	macsd_read(
				dev_t		dev,
				io_req_t	ior);

/*	Set device state				*/
extern io_return_t	macsd_set_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	status_count);

/*	SCSI write					*/
extern	io_return_t	macsd_write(
				dev_t		dev,
				io_req_t	ior);

/*	Misc.						*/

/*	Test if device is alive				*/
extern	boolean_t	macsd_check(
				dev_t		dev,
				scsi_softc_t	**p_sc,
				target_info_t	**p_tgt);

extern	void		macsd_simpleq_strategy(
				io_req_t	ior,
				void		(*start)(target_info_t *, boolean_t));

