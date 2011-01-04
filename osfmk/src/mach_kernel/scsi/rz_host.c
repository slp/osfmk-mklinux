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
 * Revision 2.2.2.1  92/03/28  10:15:36  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:32:08  jeffreyh]
 * 
 * Revision 2.3  92/02/23  22:44:29  elf
 * 	Changed the interface of a number of functions not to
 * 	require the scsi_softc pointer any longer.  It was
 * 	mostly unused, now it can be found via tgt->masterno.
 * 	[92/02/22  19:30:24  af]
 * 
 * Revision 2.2  91/08/24  12:28:02  af
 * 	Created.
 * 	[91/08/02  03:47:02  af]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 *	File: rz_host.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	7/91
 *
 *	Top layer of the SCSI driver: interface with the MI.
 *	This file contains operations specific to CPU-like devices.
 *
 * We handle here the case of other hosts that are capable of
 * sophisticated host-to-host communication protocols, we make
 * them look like... you'll see.
 *
 * There are two sides of the coin here: when we take the initiative
 * and when the other host does it.  Code for handling both cases is
 * provided in this one file.
 */

#include <mach/std_types.h>
#include <machine/machparam.h>		/* spl definitions */
#include <scsi/compat_30.h>

#include <scsi/scsi.h>
#include <scsi/scsi_defs.h>
#include <scsi/rz.h>

/*
 *	Function prototypes for internal routines.
 */

/*	SCSI host close routine (always succeeds)	*/
scsi_ret_t	schost_close(
			dev_t		dev,
			target_info_t	*tgt);

/*	Translate boolean to "sh" or "host"		*/
char  *	schost_name(
		boolean_t	internal);

/*	SCSI host open routine (always succeeds)	*/
scsi_ret_t	schost_open(
			target_info_t	*tgt,
			io_req_t	ior);

/*	SCSI host I/O start routine (nugatory)		*/
void	schost_start(
		target_info_t	*tgt,
		boolean_t	done);

/*	SCSI host strategy routine (nugatory)		*/
void		schost_strategy(
			io_req_t	ior);

/* Since we have invented a new "device" this cannot go into the
   the 'official' scsi_devsw table.  Too bad. */

scsi_devsw_t	scsi_host = {
	schost_name,		SCSI_OPTIMIZE_NULL,
	schost_open,		schost_close,
	schost_strategy,	schost_start,
	SCSI_GET_STATUS_NULL,	SCSI_SET_STATUS_NULL
};

char 
*schost_name(
	boolean_t	internal)
{
	return internal ? "sh" : "host";
}

/*ARGSUSED*/
scsi_ret_t
schost_open(
	target_info_t	*tgt,
	io_req_t	ior)
{
	return SCSI_RET_SUCCESS;	/* XXX if this is it, drop it */
}

/*ARGSUSED*/
scsi_ret_t
schost_close(
	dev_t		dev,
	target_info_t	*tgt)
{
	return SCSI_RET_SUCCESS;	/* XXX if this is it, drop it */
}

void
schost_strategy(
	io_req_t	ior)
{
	rz_simpleq_strategy( ior, schost_start);
}

void
schost_start(
	target_info_t	*tgt,
	boolean_t	done)
{
	if (done || (!tgt->dev_info.cpu.req_pending)) {
		sccpu_start( tgt, done);
		return;
	}
}
