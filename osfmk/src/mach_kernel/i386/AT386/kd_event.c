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
 * Revision 2.7.7.1  92/02/18  18:55:20  jeffreyh
 * 	Parallelized event handling
 * 	[91/07/16            bernadat]
 * 
 * Revision 2.7  91/05/14  16:27:12  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/02/05  17:19:10  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:45:36  mrt]
 * 
 * Revision 2.5  90/11/26  14:50:31  rvb
 * 	jsb bet me to XMK34, sigh ...
 * 	[90/11/26            rvb]
 * 	Synched 2.5 & 3.0 at I386q (r1.5.1.4) & XMK35 (r2.5)
 * 	[90/11/15            rvb]
 * 
 * Revision 2.4  90/08/09  16:32:32  rpd
 * 	Added kdb/X support from rvb.
 * 	[90/08/08            rpd]
 *
 * Revision 2.3  90/05/21  13:27:21  dbg
 * 	Replace stub with real file, converted for pure kernel.
 * 
 * Revision 1.5.1.3  90/05/14  13:21:36  rvb
 * 	Support for entering kdb from X;
 * 	[90/04/30            rvb]
 * 
 * Revision 1.5.1.2  90/02/28  15:50:22  rvb
 * 	Fix numerous typo's in Olivetti disclaimer.
 * 	[90/02/28            rvb]
 * 
 * Revision 1.5.1.1  90/01/08  13:30:51  rvb
 * 	Add Olivetti copyright.
 * 	[90/01/08            rvb]
 * 
 * Revision 1.5  89/07/17  10:41:17  rvb
 * 	Olivetti Changes to X79 upto 5/9/89:
 * 	[89/07/11            rvb]
 * 
 * Revision 1.4.1.1  89/04/27  12:21:11  kupfer
 * Merge X79 with our latest and greatest.
 * 
 * Revision 1.1.1.1  89/04/27  11:53:53  kupfer
 * X79 from CMU.
 * 
 * Revision 1.4  89/03/09  20:06:41  rpd
 * 	More cleanup.
 * 
 * Revision 1.3  89/02/26  12:42:31  gm0w
 * 	Changes for cleanup.
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
 *  File:         kd_event.c
 *  Description:  Driver for event interface to keyboard.
 * 
 *  $ Header: $
 * 
 *  Copyright Ing. C. Olivetti & C. S.p.A. 1989.  All rights reserved.
 * 
 *   Copyright 1988, 1989 by Olivetti Advanced Technology Center, Inc.,
 * Cupertino, California.
 * 
 * 		All Rights Reserved
 * 
 *   Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appears in all
 * copies and that both the copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Olivetti
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * 
 *   OLIVETTI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL OLIVETTI BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUR OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <mach/boolean.h>
#include <types.h>
#include <sys/time.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <i386/AT386/kd.h>
#include <i386/AT386/kd_queue.h>
#include <i386/AT386/kbd_entries.h>
#include <i386/pio.h>
#include <device/ds_routines.h>
#include <kern/misc_protos.h>

/* Forward */

extern void		kbdinit(void);
extern void		kbd_enqueue(
				kd_event	*ev);
extern io_return_t	X_kdb_enter_init(
				u_int		*data,
				u_int		count);
extern io_return_t	X_kdb_exit_init(
				u_int		*data,
				u_int		count);
extern boolean_t	kbd_read_done(
				io_req_t	ior);
extern void		kdb_in_out(
				u_int		*p);

/*
 * Code for /dev/kbd.   The interrupt processing is done in kd.c,
 * which calls into this module to enqueue scancode events when
 * the keyboard is in Event mode.
 */

/*
 * Note: These globals are protected by raising the interrupt level
 * via SPLKD.
 */

kd_event_queue kbd_queue;		/* queue of keyboard events */
mpqueue_head_t	kbd_read_queue;
static boolean_t initialized = FALSE;

/*
 * kbdinit - set up event queue.
 */

void
kbdinit(void)
{
	int s = SPLKD();
	
	if (!initialized) {
		kdq_reset(&kbd_queue);
		mpqueue_init(&kbd_read_queue);
		initialized = TRUE;
	}
	splx(s);
}


/*
 * kbdopen - Verify that open is read-only and remember process
 * group leader.
 */

io_return_t
kbdopen(
	dev_t		dev,
	dev_flavor_t	flags,
	io_req_t	ior)
{
	kbdinit();

	return(D_SUCCESS);
}


/*
 * kbdclose - Make sure that the kd driver is in Ascii mode and
 * reset various flags.
 */

void
kbdclose(
	dev_t		dev)
{
	int s = SPLKD();

	kb_mode = KB_ASCII;
	kdq_reset(&kbd_queue);

	splx(s);
}

io_return_t
kbdgetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,		/* pointer to OUT array */
	natural_t	*count)		/* OUT */
{
	io_return_t	result;

	switch (flavor) {
	    case KDGKBDTYPE:
		*data = KB_VANILLAKB;
		*count = 1;
		break;
	    default:
		return (D_INVALID_OPERATION);
	}
	return (D_SUCCESS);
}

io_return_t
kbdsetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,
	natural_t	count)
{
	io_return_t	result;

	switch (flavor) {
	    case KDSKBDMODE:
		kb_mode = *data;
		/* XXX - what to do about unread events? */
		/* XXX - should check that 'data' contains an OK valud */
		break;
	    case K_X_KDB_ENTER:
		return X_kdb_enter_init((unsigned int *)data, count);
	    case K_X_KDB_EXIT:
		return X_kdb_exit_init((unsigned int *)data, count);
	    default:
		return (D_INVALID_OPERATION);
	}
	return (D_SUCCESS);
}

/*
 * kbdread - dequeue and return any queued events.
 */

int
kbdread(
	dev_t		dev,
	io_req_t	ior)
{
	register int	err, s, count;

	err = device_read_alloc(ior, (vm_size_t)ior->io_count);
	if (err != KERN_SUCCESS)
	    return (err);

	s = SPLKD();
	kdq_lock(&kbd_queue);
	if (kdq_empty(&kbd_queue)) {
	    if (ior->io_mode & D_NOWAIT) {
		kdq_unlock(&kbd_queue);
		splx(s);
		return (D_WOULD_BLOCK);
	    }
	    ior->io_done = kbd_read_done;
	    mpenqueue_tail(&kbd_read_queue, (queue_entry_t) ior);
	    kdq_unlock(&kbd_queue);
	    splx(s);
	    return (D_IO_QUEUED);
	}
	count = 0;
	while (!kdq_empty(&kbd_queue) && count < ior->io_count) {
	    register kd_event *ev;

	    ev = kdq_get(&kbd_queue);
	    *(kd_event *)(&ior->io_data[count]) = *ev;
	    count += sizeof(kd_event);
	}
	kdq_unlock(&kbd_queue);
	splx(s);
	ior->io_residual = ior->io_count - count;
	return (D_SUCCESS);
}

boolean_t
kbd_read_done(
	io_req_t	ior)
{
	register int	s, count;

	s = SPLKD();
	kdq_lock(&kbd_queue);
	if (kdq_empty(&kbd_queue)) {
	    ior->io_done = kbd_read_done;
	    mpenqueue_tail(&kbd_read_queue, (queue_entry_t)ior);
	    kdq_unlock(&kbd_queue);
	    splx(s);
	    return (FALSE);
	}

	count = 0;
	while (!kdq_empty(&kbd_queue) && count < ior->io_count) {
	    register kd_event *ev;

	    ev = kdq_get(&kbd_queue);
	    *(kd_event *)(&ior->io_data[count]) = *ev;
	    count += sizeof(kd_event);
	}
	kdq_unlock(&kbd_queue);
	splx(s);

	ior->io_residual = ior->io_count - count;
	ds_read_done(ior);

	return (TRUE);
}

/*
 * kd_enqsc - enqueue a scancode.  Should be called at SPLKD.
 */

void
kd_enqsc(
	Scancode sc)
{
	kd_event ev;

	ev.type = KEYBD_EVENT;
	ev.time = time;
	ev.value.sc = sc;
	kbd_enqueue(&ev);
}


/*
 * kbd_enqueue - enqueue an event and wake up selecting processes, if
 * any.  Should be called at SPLKD.
 */

void
kbd_enqueue(
	kd_event *ev)
{
	kdq_lock(&kbd_queue);
	if (kdq_full(&kbd_queue))
		printf("kbd: queue full\n");
	else
		kdq_put(&kbd_queue, ev);

	{
	    io_req_t	ior;
	    for(;;) {
		mpdequeue_head(&kbd_read_queue, (queue_entry_t *)&ior);
		if (ior)
			iodone(ior);
		else
			break;
	    }
	}
	kdq_unlock(&kbd_queue);
}

u_int X_kdb_enter_str[512], X_kdb_exit_str[512];
int   X_kdb_enter_len = 0,  X_kdb_exit_len = 0;

void
kdb_in_out(
	u_int	*p)
{
register int t = p[0];

	switch (t & K_X_TYPE) {
		case K_X_IN|K_X_BYTE:
			inb(t & K_X_PORT);
			break;

		case K_X_IN|K_X_WORD:
			inw(t & K_X_PORT);
			break;

		case K_X_IN|K_X_LONG:
			inl(t & K_X_PORT);
			break;

		case K_X_OUT|K_X_BYTE:
			outb(t & K_X_PORT, p[1]);
			break;

		case K_X_OUT|K_X_WORD:
			outw(t & K_X_PORT, p[1]);
			break;

		case K_X_OUT|K_X_LONG:
			outl(t & K_X_PORT, p[1]);
			break;
	}
}

void
X_kdb_enter(void)
{
register u_int *u_ip, *endp;

	for (u_ip = X_kdb_enter_str, endp = &X_kdb_enter_str[X_kdb_enter_len];
	     u_ip < endp;
	     u_ip += 2)
	    kdb_in_out(u_ip);
}

void
X_kdb_exit(void)
{
register u_int *u_ip, *endp;

	for (u_ip = X_kdb_exit_str, endp = &X_kdb_exit_str[X_kdb_exit_len];
	     u_ip < endp;
	     u_ip += 2)
	   kdb_in_out(u_ip);
}

io_return_t
X_kdb_enter_init(
    u_int *data,
    u_int count)
{
    if (count * sizeof X_kdb_enter_str[0] > sizeof X_kdb_enter_str)
	return D_INVALID_OPERATION;

    bcopy((char *)data, (char *)X_kdb_enter_str,
	count * sizeof X_kdb_enter_str[0]);
    X_kdb_enter_len = count;
    return D_SUCCESS;
}

io_return_t
X_kdb_exit_init(
    u_int *data,
    u_int count)
{
    if (count * sizeof X_kdb_exit_str[0] > sizeof X_kdb_exit_str)
	return D_INVALID_OPERATION;

    bcopy((char *)data, (char *)X_kdb_exit_str,
	count * sizeof X_kdb_exit_str[0]);
    X_kdb_exit_len = count;
    return D_SUCCESS;
}
