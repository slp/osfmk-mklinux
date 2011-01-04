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
 * Revision 2.3  91/07/31  18:00:37  dbg
 * 	Changed copyright.
 * 	[91/07/31            dbg]
 * 
 * Revision 2.2  91/05/08  12:55:31  dbg
 * 	Adapted from Sequent SYMMETRY sources.
 * 	[91/04/26  14:51:09  dbg]
 * 
 */

#ifndef	lint
static	char	rcsid[] = "$Header: /u1/osc/rcs/mach_kernel/sqt/consio.c,v 1.2.10.1 1994/09/23 03:04:22 ezf Exp $";
#endif

/*
 * consio.c
 *	Machine dependent cnputc(), putflush() implementation.
 *
 * Also, getchar for debug.
 */

/*
 * Revision 1.2  89/07/20  18:05:36  kak
 * moved balance includes
 * 
 * Revision 1.1  89/07/05  13:15:28  kak
 * Initial revision
 * 
 */

#include <sys/types.h>

#include <sqt/intctl.h>
#include <sqt/mutex.h>

#include <sqt/engine.h>


extern	short	upyet;		/* says if init done yet */
extern	u_char	cons_scsi;	/* SLIC id of console SCSI card */

#define	PUTCHAR_BIN	1	/* for SCSI putchar()'s */

#ifdef	DEBUG
/*
 * Data for DEBUG console....
 */
char		gc_last;		/* last input character */
#endif	DEBUG

/*
 * cnputc()
 *	Arrange that a character be output on the "console".
 *
 */

/*ARGSUSED*/
cnputc(c)
	char	c;
{
	register int me;
	spl_t	s;

	if (c == '\n')
		cnputc('\r');

	if (!upyet) {
		if (cons_scsi == 0)
			return;		/* va_slic is not set! */
		sq_putc(c);
		return;
	}

	/*
	 * Output character.
	 */

	s = splhi();				/* mutex SLIC usage */

	sq_putc(c);

	splx(s);
}

/*
 * sq_putc()
 *	Local version to do SCSI output of character.
 *
 * Assumes caller did splhi() or otherwise blocks interrupts.
 * Can't do this here, since this used at boot time before
 * turned on kernel page-tables (==> splhi(), splx() use wrong
 * virt-addr for SLIC).
 */

sq_putc(c)
	char	c;
{
	mIntr(cons_scsi, PUTCHAR_BIN, (u_char)c);
}
