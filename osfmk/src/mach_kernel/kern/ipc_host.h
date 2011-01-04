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
 * Revision 2.5.2.1  92/06/24  18:05:13  jeffreyh
 * 	Add extern definintion for convert_port_to_host_paging
 * 		for NORMA_IPC.
 * 	[92/06/17            jeffreyh]
 * 
 * Revision 2.5  91/06/25  10:28:30  rpd
 * 	Changed the convert_foo_to_bar functions
 * 	to use ipc_port_t instead of mach_port_t.
 * 	[91/05/27            rpd]
 * 
 * Revision 2.4  91/05/14  16:41:53  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:26:34  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:12:40  mrt]
 * 
 * Revision 2.2  90/06/02  14:54:04  rpd
 * 	Created for new IPC.
 * 	[90/03/26  23:46:41  rpd]
 * 
 * 	Merge to X96
 * 	[89/08/02  22:55:33  dlb]
 * 
 * 	Created.
 * 	[88/12/01            dlb]
 * 
 * Revision 2.3  89/10/15  02:04:36  rpd
 * 	Minor cleanups.
 * 
 * Revision 2.2  89/10/11  14:07:24  dlb
 * 	Merge.
 * 	[89/09/01  17:25:42  dlb]
 * 
 */ 
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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

#ifndef	_KERN_IPC_HOST_H_
#define	_KERN_IPC_HOST_H_

#include <mach/port.h>
#include <kern/clock.h>
#include <kern/host.h>
#include <kern/processor.h>

/* Initialize IPC host services */
extern void ipc_host_init(void);

/* Initialize ipc access to processor by allocating a port */
extern void ipc_processor_init(
	processor_t	processor);

/* Enable ipc control of processor by setting port object */
extern void ipc_processor_enable(
	processor_t	processor);

/* Initialize ipc control of a processor set */
extern void ipc_pset_init(
	processor_set_t		pset);

/* Enable ipc access to a processor set */
extern void ipc_pset_enable(
	processor_set_t		pset);

/* Disable ipc access to a processor set */
extern void ipc_pset_disable(
	processor_set_t		pset);

/* Deallocate the ipc control structures for a processor set */
extern void ipc_pset_terminate(
	processor_set_t		pset);

/* Initialize ipc control of a clock */
extern void ipc_clock_init(
	clock_t		clock);

/* Enable ipc access to a clock */
extern void ipc_clock_enable(
	clock_t		clock);

/* Convert from a port to a clock */
extern clock_t convert_port_to_clock(
	ipc_port_t	port);

/* Convert from a port to a clock control */
extern clock_t convert_port_to_clock_ctrl(
	ipc_port_t	port);

/* Convert from a clock to a port */
extern ipc_port_t convert_clock_to_port(
	clock_t		clock);

/* Convert from a clock control to a port */
extern ipc_port_t convert_clock_ctrl_to_port(
	clock_t		clock);

/* Convert from a clock name to a clock pointer */
extern clock_t port_name_to_clock(
	mach_port_t	clock_name);

/* Convert from a port to a host */
extern host_t convert_port_to_host(
	ipc_port_t	port);

/* Convert from a port to a host privilege port */
extern host_t convert_port_to_host_priv(
	ipc_port_t	port);

/*  Convert from a port to a host paging port */
extern host_t convert_port_to_host_paging(
	ipc_port_t	port);

/* Convert from a host to a port */
extern ipc_port_t convert_host_to_port(
	host_t		host);

/* Convert from a port to a processor */
extern processor_t convert_port_to_processor(
	ipc_port_t	port);

/* Convert from a processor to a port */
extern ipc_port_t convert_processor_to_port(
	processor_t	processor);

/* Convert from a port to a processor set */
extern processor_set_t convert_port_to_pset(
	ipc_port_t	port);

/* Convert from a port to a processor set name */
extern processor_set_t convert_port_to_pset_name(
	ipc_port_t	port);

/* Convert from a processor set to a port */
extern ipc_port_t convert_pset_to_port(
	processor_set_t		processor);

/* Convert from a processor set name to a port */
extern ipc_port_t convert_pset_name_to_port(
	processor_set_t		processor);

/* Convert from a port to a host security port */
extern host_t convert_port_to_host_security(
        ipc_port_t	port);


#endif	/* _KERN_IPC_HOST_H_ */
