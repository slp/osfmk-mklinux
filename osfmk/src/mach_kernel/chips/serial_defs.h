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
/* CMU_HIST */
/*
 * Revision 2.2  91/08/24  11:53:23  af
 * 	Created.
 * 	[91/07/07            af]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *	File: serial_defs.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	7/91
 *
 *	Generic console driver for serial-line based consoles.
 */

#ifndef	_CHIPS_SERIAL_DEFS_
#define	_CHIPS_SERIAL_DEFS_

/*
 * Common defs
 */

extern int
	(*console_probe)(vm_offset_t, struct bus_device *),
	(*console_param)(struct tty *, int),
	(*console_start)(struct tty *),
	(*console_putc)(int, int, int),
	(*console_getc)(int, int, boolean_t, boolean_t),
	(*console_pollc)(int, boolean_t),
	(*console_mctl)(struct tty *, int, int),
	(*console_softCAR)(int, int, int);

extern struct tty	*console_tty[];
extern int rcline, cnline;
extern int	console;

/* Simple one-char-at-a-time scheme */
extern int cons_simple_tint(int, boolean_t);
extern void cons_simple_rint(int, int, int, int);

#define	CONS_ERR_PARITY		0x1000
#define	CONS_ERR_BREAK		0x2000
#define	CONS_ERR_OVERRUN	0x4000

/*
 * Function Prototypes for device-specific routines.
 */

#include <device/device_types.h>
#include <device/io_req.h>

extern io_return_t	cons_open(
				dev_t		dev,
				dev_mode_t	flag,
				io_req_t	ior);

extern void		cons_close(
				dev_t		dev);

extern io_return_t	cons_read(
				dev_t		dev,
				io_req_t	ior);

extern io_return_t	cons_write(
				dev_t		dev,
				io_req_t	ior);

extern io_return_t	cons_get_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				mach_msg_type_number_t	*status_count);

extern io_return_t	cons_set_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				mach_msg_type_number_t	status_count);

extern boolean_t	cons_portdeath(
				dev_t		dev,
				ipc_port_t	port);

#endif	/* _CHIPS_SERIAL_DEFS_ */
