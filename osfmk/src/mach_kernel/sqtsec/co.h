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
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * Copyright (c) 1991 Sequent Computer Systems
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND SEQUENT COMPUTER SYSTEMS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * SEQUENT COMPUTER SYSTEMS DISCLAIM ANY LIABILITY OF ANY KIND FOR
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

/* CMU_HIST */
/*
 * Revision 2.3  91/07/31  18:06:27  dbg
 * 	Changed copyright.
 * 	[91/07/31            dbg]
 * 
 * Revision 2.2  91/05/08  13:05:01  dbg
 * 	Added volatile declarations.
 * 	[91/03/25            dbg]
 * 
 * 	Added output buffer since Mach kernel circular buffers are not
 * 	within SCED-addressable space (lowest 4 Meg).
 * 	[90/12/18            dbg]
 * 
 * 	Adapted for pure kernel.
 * 	[90/09/24            dbg]
 * 
 */

/*
 * $Header: /u1/osc/rcs/mach_kernel/sqtsec/co.h,v 1.2.10.1 1994/09/23 03:10:07 ezf Exp $
 */

/*
 * co.h
 *	Info passed between the SCSI/Ether console driver and the
 *	binary config file.
 *
 * binary config table for the console driver.  There is one entry in
 * the co_bin_config[] table for each controller in the system.
 */

/*
 * Revision 1.1  89/07/05  13:20:12  kak
 * Initial revision
 * 
 */
#ifndef	_SQTSEC_CO_H_
#define	_SQTSEC_CO_H_

#include <sys/types.h>
#include <device/tty.h>

#include <sqt/mutex.h>
#include <sqtsec/sec.h>

struct co_bin_config {
	gate_t	cobc_gate;
};

/*
 * Console state.
 * There is one such structure for each tty device (minor device) on
 * each SCSI/Ether controller.
 *
 * The local and remote console devices are always minor devices 0
 * and 1, regardless of where they appear on the bus.  This is
 * guaranteed at config time.
 * 
 * Mutex notes:
 * 	We could lock the input and output sides separately.  
 *	But, this would probably not be a big win.  So, use one
 * 	lock for all state information.
 */

/*
 * throw together a structure containing the size of the program queue
 * and its state.
 */

struct sec_pq {
	struct sec_progq	*sq_progq;
	u_short			sq_size;
};

#define	CBSIZE	64				/* size of input buffer */

struct co_state {
	struct	tty	ss_tty;
	u_char	ss_alive;
	u_char	ss_initted;
						/* input state */
	volatile
	  int	is_status;
	struct	sec_dev	*is_sd;
	struct	sec_pq	is_reqq;
	struct	sec_pq	is_doneq;
	u_long	is_ovc;			/* count number of overflow errors */
	u_long	is_parc;		/* count number of parity errors */
	char	is_buffer[CBSIZE];
	u_char	is_restart_read;	/* restart read flag		 */
	u_char	is_initted;
						/* output state */
	volatile
	  int	os_status;
	struct	sec_dev *os_sd;
	struct	sec_pq	os_reqq;
	struct	sec_pq	os_doneq;
	struct	sec_smode	os_smode;
	sema_t	os_busy_wait;
	u_char	os_busy_flag;
	u_char	os_initted;
#ifdef	MACH_KERNEL
	char	os_buffer[CBSIZE];
#endif	/* MACH_KERNEL */

};

#define	CM_BAUD		sm_un.sm_cons.cm_baud
#define CM_FLAGS	sm_un.sm_cons.cm_flags

#ifdef	MACH_KERNEL
extern struct 	co_bin_config co_bin_config[];
extern int	co_bin_config_count;
#ifdef	DEBUG
extern int	co_debug;
#endif	DEBUG
#endif	MACH_KERNEL

#ifdef DEBUG
extern char gc_last;
#define	DBGCHAR	('f'&037)
#endif	DEBUG

#endif	/* _SQTSEC_CO_H_ */
