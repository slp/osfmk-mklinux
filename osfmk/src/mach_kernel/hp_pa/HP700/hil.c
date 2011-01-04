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
 * Copyright (c) 1987-1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *	Utah $Hdr: hil.c 1.21 95/07/11$
 */

#include <sys/time.h>
#include <types.h>
#include <sys/ioctl.h>
#include <device/tty.h>
#include <device/cirbuf.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <kern/spl.h>
#include <machine/machparam.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <hp_pa/HP700/hilreg.h>
#include <hp_pa/HP700/hilioctl.h>
#include <hp_pa/HP700/hilvar.h>
#include <hp_pa/HP700/kbdmap.h>

#include <machine/cpu.h>

#include <device/ds_routines.h>
#include <kern/misc_protos.h>
#include <hp_pa/trap.h>

#include "hil.h"

struct  hilloop hilloop[NHIL];
struct	_hilbell default_bell = { BELLDUR, BELLFREQ };

extern struct kbdmap hkbd_map[];

/*
 * Forwards
 */

int kbdnmi(int);
void kbddisable(int);
io_return_t hilopen(dev_t, int, io_req_t);
void hilclose(dev_t);
io_return_t hilread(dev_t, io_req_t);
io_return_t hilgetstat(dev_t, int, int *, unsigned int *count);
io_return_t hilsetstat(dev_t, int, int *, unsigned int count);
vm_offset_t hilmap(dev_t, vm_offset_t, int);
void hilreset(struct hilloop *);
void send_hil_cmd(struct hil_dev*, u_char, u_char*, u_char, u_char *r);
void hilinfo(int);
boolean_t hilread_done(io_req_t);
void pollon(struct hil_dev *);
void polloff(struct hil_dev *);
void hil_process_int(struct hilloop *, u_char, u_char);
void hilbeep(struct hilloop *, struct _hilbell *);
io_return_t hilioctl(dev_t, int, caddr_t,  int);
void hilevent(struct hilloop *);
int hiliddev(struct hilloop *);
void send_hildev_cmd(struct hilloop *, char, char);
void hpuxhilevent(struct hilloop *, struct hilloopdev *);
#ifdef DEBUG
void printhilpollbuf(struct hilloop *);
void hilreport(struct hilloop *);
void printhilcmdbuf(struct hilloop *);
#endif

/*
 * Externs
 */
extern void itefilter(char, char);
extern void queue_delayed_reply(queue_t, io_req_t, boolean_t (*)(io_req_t));

#ifdef DEBUG
int 	hildebug = 0;
#define HDB_FOLLOW	0x01
#define HDB_MMAP	0x02
#define HDB_MASK	0x04
#define HDB_CONFIG	0x08
#define HDB_KEYBOARD	0x10
#define HDB_IDMODULE	0x20
#define HDB_EVENTS	0x80
#endif

void
hilsoftinit(
	int unit,
	struct hil_dev *hilbase)
{
  	register struct hilloop *hilp = &hilloop[unit];

#ifdef DEBUG
	if (hildebug & HDB_FOLLOW)
		printf("hilsoftinit(%d, %x)\n", unit, hilbase);
#endif
	/*
	 * Initialize loop information
	 */
	hilp->hl_addr = hilbase;
	hilp->hl_cmdending = FALSE;
	hilp->hl_actdev = hilp->hl_cmddev = 0;
	hilp->hl_cmddone = FALSE;
	hilp->hl_cmdbp = hilp->hl_cmdbuf;
	hilp->hl_pollbp = hilp->hl_pollbuf;
	hilp->hl_kbddev = 0;
	hilp->hl_kbdlang = KBD_DEFAULT;
	hilp->hl_kbdflags = 0;
	hilp->hl_device[HILLOOPDEV].hd_flags = (HIL_ALIVE|HIL_PSEUDO);
}

void
hilinit(
	int unit,
	struct hil_dev *hilbase)
{
  	register struct hilloop *hilp = &hilloop[unit];
	extern int havekeyboard;

#ifdef DEBUG
	if (hildebug & HDB_FOLLOW)
		printf("hilinit(%d, %x)\n", unit, hilbase);
#endif
	/*
	 * Initialize software (if not already done).
	 */
	if ((hilp->hl_device[HILLOOPDEV].hd_flags & HIL_ALIVE) == 0)
		hilsoftinit(unit, hilbase);
	/*
	 * Initialize hardware.
	 * Reset the loop hardware, and collect keyboard/id info
	 */
	hilreset(hilp);
	hilinfo(unit);
	hilkbdenable(unit);
	/* only if gkd was not found before */
	if(havekeyboard == 0)
		havekeyboard = 1;	
}

io_return_t
hilopen(
	dev_t dev,
	int flags,
	io_req_t ior)
{
  	register struct hilloop *hilp = &hilloop[HILLOOP(dev)];
	register struct hilloopdev *dptr;
	u_char device = HILUNIT(dev);
	spl_t s;

#ifdef DEBUG
	if (hildebug & HDB_FOLLOW)
		printf("hilopen(%d): loop %x device %x\n",
		       current_thread(), HILLOOP(dev), device);
#endif
	
	if ((hilp->hl_device[HILLOOPDEV].hd_flags & HIL_ALIVE) == 0)
		return(ENXIO);

	dptr = &hilp->hl_device[device];
	if ((dptr->hd_flags & HIL_ALIVE) == 0)
		return(D_NO_SUCH_DEVICE);

	/*
	 * In order to do as little damage as possible to the driver,
	 * we use tty queues and operations for the read interface.
	 * Since the select interface issues read calls even for the
	 * queue interface, we have to set this up now.
	 */
	if ((dptr->tty.t_state & TS_ISOPEN) == 0) {
		ttychars(&dptr->tty);
		dptr->tty.t_dev = dev;
		dptr->tty.t_state |= (TS_CARR_ON|TS_ISOPEN);
	}
	/*
	 * Pseudo-devices cannot be read, nothing more to do.
	 */
	if (dptr->hd_flags & HIL_PSEUDO)
		return(0);

	/*
	 * Open semantics:
	 * 1. Devices are exclusive use
	 * 2. Use read interface
	 */
	if (dptr->hd_flags & HIL_READIN)
		return (EBUSY);
	dptr->hd_flags |= HIL_READIN;

	send_hil_cmd(hilp->hl_addr, HIL_INTON, NULL, 0, NULL);
	/*
	 * Opened the keyboard, put in raw mode.
	 */
	s = splhil();
	if (device == hilp->hl_kbddev) {
		u_char mask = 0;
		send_hil_cmd(hilp->hl_addr, HIL_WRITEKBDSADR, &mask, 1, NULL);
		hilp->hl_kbdflags |= KBD_RAW;
#ifdef DEBUG
		if (hildebug & HDB_KEYBOARD)
			printf("hilopen: keyboard %d raw\n", hilp->hl_kbddev);
#endif
	}
	splx(s);
	return (0);
}

/* ARGSUSED */
void
hilclose(
    dev_t dev)
{
  	struct hilloop *hilp = &hilloop[HILLOOP(dev)];
	struct hilloopdev *dptr;
	struct tty *tp;
	int i;
	u_char device = HILUNIT(dev);
	u_char mask, lpctrl;
	spl_t s;

#ifdef DEBUG
	if (hildebug & HDB_FOLLOW)
		printf("hilclose(%d): device %x\n", current_thread(), device);
#endif

	dptr = &hilp->hl_device[device];
	if (device && (dptr->hd_flags & HIL_PSEUDO))
		return;

	/*
	 * Always flush the read buffer
	 */
	tp = &dptr->tty;

	s = splhil();
	simple_lock(&tp->t_lock);
	tp->t_state &= ~TS_CARR_ON;
	ttyclose(tp);
	ndflush(&tp->t_inq, tp->t_inq.c_cc);
	simple_unlock(&tp->t_lock);

	dptr->hd_flags &= ~(HIL_READIN|HIL_NOBLOCK);

	/*
	 * Set keyboard back to cooked mode when closed.
	 */
	if (device && device == hilp->hl_kbddev) {
		mask = 1 << (hilp->hl_kbddev - 1);
		send_hil_cmd(hilp->hl_addr, HIL_WRITEKBDSADR, &mask, 1, NULL);
		hilp->hl_kbdflags &= ~(KBD_RAW|KBD_AR1|KBD_AR2);
		/*
		 * XXX: We have had trouble with keyboards remaining raw
		 * after close due to the LPC_KBDCOOK bit getting cleared
		 * somewhere along the line.  Hence we check and reset
		 * LPCTRL if necessary.
		 */
		send_hil_cmd(hilp->hl_addr, HIL_READLPCTRL, NULL, 0, &lpctrl);
		if ((lpctrl & LPC_KBDCOOK) == 0) {
			printf("hilclose: bad LPCTRL %x, reset to %x\n",
			       lpctrl, lpctrl|LPC_KBDCOOK);
			lpctrl |= LPC_KBDCOOK;
			send_hil_cmd(hilp->hl_addr, HIL_WRITELPCTRL,
					&lpctrl, 1, NULL);
		}
#ifdef DEBUG
		if (hildebug & HDB_KEYBOARD)
			printf("hilclose: keyboard %d cooked\n",
			       hilp->hl_kbddev);
#endif
		hilkbdenable(HILLOOP(dev));
	}
	splx(s);
}

/*
 * Mach uses the read interface to implement select as well as
 * to just read data.  This would work fine in the absence of the
 * shared-queue interface, but since we may want to select on a
 * queue, we have to handle read requests when the queue interface
 * is being used.  Hence, we need a varient of the char_read routine.
 */
io_return_t
hilread(
	dev_t dev,
	io_req_t ior)
{
	struct hilloop *hilp = &hilloop[HILLOOP(dev)];
	register struct hilloopdev *dptr = &hilp->hl_device[HILUNIT(dev)];
	register struct tty *tp = &dptr->tty;
	kern_return_t rc;
	spl_t s;

#ifdef DEBUG
	if (hildebug & HDB_FOLLOW)
		printf("hilread: loop %x device %x ior %x\n",
		       HILLOOP(dev), HILUNIT(dev), ior);
#endif

	/*
	 * Allocate memory for read buffer.
	 */
	rc = device_read_alloc(ior, (vm_size_t)ior->io_count);
	if (rc != KERN_SUCCESS)
		return (rc);

	s = splhil();
	simple_lock(&tp->t_lock);

	if (tp->t_inq.c_cc == 0) {
		if (ior->io_mode & D_NOWAIT) {
			rc = D_WOULD_BLOCK;
		} else {
			ior->io_dev_ptr = (char *)dptr;
			queue_delayed_reply(&tp->t_delayed_read, ior, hilread_done);
			rc = D_IO_QUEUED;
		}
		simple_unlock(&tp->t_lock);
		splx(s);
		return rc;
	}
	
	ior->io_residual = ior->io_count -
		q_to_b(&tp->t_inq, ior->io_data, (int)ior->io_count);

	simple_unlock(&tp->t_lock);
	splx(s);
	return (D_SUCCESS);
}

boolean_t
hilread_done(io_req_t ior)
{
	struct hilloopdev *dptr = (struct hilloopdev *)ior->io_dev_ptr;
	struct tty *tp = &dptr->tty;
	spl_t s;

#ifdef DEBUG
	if (hildebug & HDB_FOLLOW)
		printf("hilread_done: ior %x\n", ior);
#endif
	s = splhil();
	simple_lock(&tp->t_lock);

	if (tp->t_inq.c_cc <= 0) {
		queue_delayed_reply(&tp->t_delayed_read, ior,
				    hilread_done);
		simple_unlock(&tp->t_lock);
		splx(s);
		return(FALSE);
	}

	ior->io_residual = ior->io_count -
		q_to_b(&tp->t_inq, ior->io_data, (int)ior->io_count);
	
	simple_unlock(&tp->t_lock);
	splx(s);

	(void) ds_read_done(ior);
	return (TRUE);
}

/*
 * Mach splits ioctls into those that write data into the kernel, and
 * those that read data out of the kernel (yep, very annoying).
 * Provide some glue around hilioctl().
 */

io_return_t
hilgetstat(
	dev_t		dev,
	int		flavor,
	int*		data,		/* pointer to OUT array */
	unsigned int	*count)		/* out */
{
	int error;

	error = hilioctl(dev, flavor, (caddr_t)data, 0);
	*count = ((flavor >> 16) & 0x7F) / sizeof(int);
	return(error);
}

#ifndef FIONBIO
#define	FIONBIO		_IOW('f', 126, int)	/* non-blocking i/o */
#endif
#ifndef FIOASYNC
#define	FIOASYNC	_IOW('f', 125, int)	/* async i/o */
#endif

io_return_t
hilsetstat(
	dev_t		dev,
	int		flavor,
	int *		data,
	unsigned int	count)
{
	return(hilioctl(dev, flavor, (caddr_t)data, 0));
}

io_return_t
hilioctl(
	 dev_t dev,
	 int cmd, 
	 caddr_t data,
	 int flag)
{
	register struct hilloop *hilp = &hilloop[HILLOOP(dev)];
	char device = HILUNIT(dev);
	struct hilloopdev *dptr;
	register int i;
	u_char hold;
	int error;

#ifdef DEBUG
	if (hildebug & HDB_FOLLOW)
		printf("hilioctl(%d): dev %x cmd %x\n",
		       current_thread(), device, cmd);
#endif
	
	dptr = &hilp->hl_device[device];
	if ((dptr->hd_flags & HIL_ALIVE) == 0)
		return (D_NO_SUCH_DEVICE);

	/*
	 * Don't allow hardware ioctls on virtual devices.
	 */
	if (dptr->hd_flags & HIL_PSEUDO) {
		switch (cmd) {
		case HILID:
			return D_NO_SUCH_DEVICE;
		default:
			break;
		}
	}

	hilp->hl_cmdbp = hilp->hl_cmdbuf;
	bzero((caddr_t)hilp->hl_cmdbuf, HILBUFSIZE);
	hilp->hl_cmddev = device;
	switch (cmd) {
	case HILSC:
	case HILID:
	case HILRN:
	case HILRS:
	case HILED:
	case HILP1:
	case HILP2:
	case HILP3:
	case HILP4:
	case HILP5:
	case HILP6:
	case HILP7:
	case HILP:
	case HILA1:
	case HILA2:
	case HILA3:
	case HILA4:
	case HILA5:
	case HILA6:
	case HILA7:
	case HILA:
		send_hildev_cmd(hilp, device, (cmd & 0xFF));
		bcopy((const char*)hilp->hl_cmdbuf, data, hilp->hl_cmdbp-hilp->hl_cmdbuf);
	  	break;

        case HILDKR:
        case HILER1:
        case HILER2:
		if (hilp->hl_kbddev) {
			hilp->hl_cmddev = hilp->hl_kbddev;
			send_hildev_cmd(hilp, hilp->hl_kbddev, (cmd & 0xFF));
			hilp->hl_kbdflags &= ~(KBD_AR1|KBD_AR2);
		}
		break;

	case EFTSBP:
		/* Send four data bytes to the tone gererator. */
		send_hil_cmd(hilp->hl_addr, HIL_STARTCMD, (u_char*)data, 4, NULL);
		/* Send the trigger beeper command to the 8042. */
		send_hil_cmd(hilp->hl_addr, (cmd & 0xFF), NULL, 0, NULL);
		break;

	case EFTRRT:
		/* Transfer the real time to the 8042 data buffer */
		send_hil_cmd(hilp->hl_addr, (cmd & 0xFF), NULL, 0, NULL);
		/* Read each byte of the real time */
		for (i = 0; i < 5; i++) {
			send_hil_cmd(hilp->hl_addr, HIL_READTIME + i, NULL,
					0, &hold);
			data[4-i] = hold;
		}
		break;
		
	case EFTRT:
		for (i = 0; i < 4; i++) {
			send_hil_cmd(hilp->hl_addr, (cmd & 0xFF) + i,
					NULL, 0, &hold);
			data[i] = hold;
		}
		break;

        case EFTRLC:
        case EFTRCC:
		send_hil_cmd(hilp->hl_addr, (cmd & 0xFF), NULL, 0, &hold);
		*data = hold;
		break;
		
        case EFTSRPG:
        case EFTSRD:
        case EFTSRR:
		send_hil_cmd(hilp->hl_addr, (cmd & 0xFF), (u_char*)data, 1, NULL);
		break;
		
	case EFTSBI:
		hold = 7 - (*(u_char *)data >> 5);
		*(int *)data = 0x84069008 | (hold << 8);
		send_hil_cmd(hilp->hl_addr, HIL_STARTCMD, (u_char*)data, 4, NULL);
		send_hil_cmd(hilp->hl_addr, 0xC4, NULL, 0, NULL);
		break;

	case FIONBIO:
		dptr = &hilp->hl_device[device];
		if (*(int *)data)
			dptr->hd_flags |= HIL_NOBLOCK;
		else
			dptr->hd_flags &= ~HIL_NOBLOCK;
		break;

	case FIOASYNC:
		break;

        default:
		hilp->hl_cmddev = 0;
		return(EINVAL);
	}
	hilp->hl_cmddev = 0;
	return(0);

}

/* ARGSUSED */
vm_offset_t
hilmap(
	dev_t dev,
	vm_offset_t off, 
	int prot)
{
	return(-1);
}

void
hilintr(int unit)
{
	struct hilloop *hilp = &hilloop[unit];
	register struct hil_dev *hildevice = hilp->hl_addr;
	u_char c, stat;

	stat = READHILSTAT(hildevice);
	c = READHILDATA(hildevice);		/* clears interrupt */
	hil_process_int(hilp, stat, c);
}

#include "ite.h"

void
hil_process_int(
	struct hilloop *hilp,
	u_char stat, 
	u_char c)
{
#ifdef DEBUG
	if (hildebug & HDB_EVENTS)
		printf("hilint: %x %x\n", stat, c);
#endif

	/* the shift enables the compiler to generate a jump table */
	switch ((stat>>HIL_SSHIFT) & HIL_SMASK) {

#if NITE > 0
	case HIL_KEY:
	case HIL_SHIFT:
	case HIL_CTRL:
	case HIL_CTRLSHIFT:
		itefilter(stat, c);
		return;
#endif
		
	case HIL_STATUS:			/* The status info. */
		if (c & HIL_ERROR) {
		  	hilp->hl_cmddone = TRUE;
			if (c == HIL_RECONFIG)
				hilconfig(hilp);
			break;
		}
		if (c & HIL_COMMAND) {
		  	if (c & HIL_POLLDATA)	/* End of data */
				hilevent(hilp);
			else			/* End of command */
			  	hilp->hl_cmdending = TRUE;
			hilp->hl_actdev = 0;
		} else {
		  	if (c & HIL_POLLDATA) {	/* Start of polled data */
			  	if (hilp->hl_actdev != 0)
					hilevent(hilp);
				hilp->hl_actdev = (c & HIL_DEVMASK);
				hilp->hl_pollbp = hilp->hl_pollbuf;
			} else {		/* Start of command */
				if (hilp->hl_cmddev == (c & HIL_DEVMASK)) {
					hilp->hl_cmdbp = hilp->hl_cmdbuf;
					hilp->hl_actdev = 0;
				}
			}
		}
	        return;

	case HIL_DATA:
		if (hilp->hl_actdev != 0)	/* Collecting poll data */
			*hilp->hl_pollbp++ = c;
		else if (hilp->hl_cmddev != 0)  /* Collecting cmd data */
			if (hilp->hl_cmdending) {
				hilp->hl_cmddone = TRUE;
				hilp->hl_cmdending = FALSE;
			} else  
				*hilp->hl_cmdbp++ = c;
		return;
		
	case 0:		/* force full jump table */
	default:
		return;
	}
}

void
hilevent(struct hilloop *hilp)
{
	register struct hilloopdev *dptr = &hilp->hl_device[hilp->hl_actdev];
	register int len, mask, qnum;
	register u_char *cp, *pp;
	struct timeval ourtime;
	hil_packet *proto;
	long tenths;
	spl_t s;

#ifdef PANICBUTTON
	static int first;
	extern int panicbutton;

	cp = hilp->hl_pollbuf;
	if (panicbutton && (*cp & HIL_KBDDATA)) {
		if (*++cp == 0x4E)
			first = 1;
		else if (first && *cp == 0x46 && !panicstr)
			panic("are we having fun yet?");
		else
			first = 0;
	}
#endif
#ifdef DEBUG
	if (hildebug & HDB_EVENTS) {
		printf("hilevent: dev %d pollbuf: ", hilp->hl_actdev);
		printhilpollbuf(hilp);
		printf("\n");
	}
#endif

	if (dptr->hd_flags & HIL_READIN)
		hpuxhilevent(hilp, dptr);
}

void
hpuxhilevent(
	     struct hilloop *hilp,
	     struct hilloopdev *dptr)
{
	register int len;
	struct timeval ourtime;
	long tstamp;
	spl_t s;
	struct tty *tp = &dptr->tty;
	struct cirbuf *queue = &tp->t_inq;

#ifdef DEBUG
	if (hildebug & HDB_EVENTS)
		printf("hpuxhilevent: dev %d\n", hilp->hl_actdev);
#endif
	/*
	 * Everybody gets the same time stamp
	 */
	s = splclock();
	ourtime = time;
	splx(s);
	tstamp = (ourtime.tv_sec * 100) + (ourtime.tv_usec / 10000);

	/*
	 * Each packet that goes into the buffer must be preceded by the
	 * number of bytes in the packet, and the timestamp of the packet.
	 * This adds 5 bytes to the packet size. Make sure there is enough
	 * room in the buffer for it, and if not, toss the packet.
	 */
	len = hilp->hl_pollbp - hilp->hl_pollbuf;
	if (queue->c_cc <= (HILMAXCLIST - (len+5))) {
		putc(len+5, queue);
		(void) b_to_q((char *)&tstamp, sizeof tstamp, queue);
		(void) b_to_q((char *)hilp->hl_pollbuf, len, queue);
	}
	s = splhil();
	tty_queue_completion(&tp->t_delayed_read);
	splx(s);
}

/*
 * Cooked keyboard functions for ite driver.
 * There is only one "cooked" ITE keyboard (the first keyboard found)
 * per loop.  There may be other keyboards, but they will always be "raw".
 */

void
hilkbdbell(int iteunit)
{
	int unit = 0;	/* XXX */
	struct hilloop *hilp = &hilloop[unit];

	hilbeep(hilp, &default_bell);
}

void
hilkbdenable(int iteunit)
{
	int unit = 0;	/* XXX */
	struct hilloop *hilp = &hilloop[unit];
	register struct hil_dev *hildevice = hilp->hl_addr;
	u_char db;

	/* Set the autorepeat rate register */
	db = ar_format(KBD_ARR);
	send_hil_cmd(hildevice, HIL_SETARR, &db, 1, NULL);

	/* Set the autorepeat delay register */
	db = ar_format(KBD_ARD);
	send_hil_cmd(hildevice, HIL_SETARD, &db, 1, NULL);

	/* Enable interrupts */
	send_hil_cmd(hildevice, HIL_INTON, NULL, 0, NULL);
}

void
hilkbddisable(int iteunit)
{
}

/*
 * XXX: read keyboard directly and return code.
 * Used by console getchar routine.  Could really screw up anybody
 * reading from the keyboard in the normal, interrupt driven fashion.
 */
int
hilkbdgetc(int iteunit, int *statp, int wait)
{
	int unit = 0;	/* XXX */
	struct hilloop *hilp = &hilloop[unit];
	register struct hil_dev *hildevice = hilp->hl_addr;
	register int c, stat;
	spl_t s;

	s = splhil();
	while (((stat = READHILSTAT(hildevice)) & HIL_DATA_RDY) == 0)
		if (wait == 0) {
			*statp = 0;
			splx(s);
			return(0);
		}
	c = READHILDATA(hildevice);
	splx(s);
	*statp = stat;
	return(c);
}

/*
 * Recoginize and clear keyboard generated NMIs.
 * Returns 1 if it was ours, 0 otherwise.  Note that we cannot use
 * send_hil_cmd() to issue the clear NMI command as that would actually
 * lower the priority to splimp() and it doesn't wait for the completion
 * of the command.  Either of these conditions could result in the
 * interrupt reoccuring.  Note that we issue the CNMT command twice.
 * This seems to be needed, once is not always enough!?!
 */
int 
kbdnmi(int unit)
{
	struct hilloop *hilp;

	hilp = &hilloop[unit];
	HILWAIT(hilp->hl_addr);
	WRITEHILCMD(hilp->hl_addr, HIL_CNMT);
	HILWAIT(hilp->hl_addr);
	WRITEHILCMD(hilp->hl_addr, HIL_CNMT);
	HILWAIT(hilp->hl_addr);
	return(1);
}

#define HILSECURITY	0x33
#define HILIDENTIFY	0x03
#define HILSCBIT	0x04

/*
 * Called at boot time to print out info about interesting devices
 */
void
hilinfo(int unit)
{
  	register struct hilloop *hilp = &hilloop[unit];
	register int id, len;
	register struct kbdmap *km;

#ifdef DEBUG
	if (hildebug & HDB_CONFIG)
		hilreport(hilp);
#endif

	/*
	 * Keyboard info.
	 */
	if (hilp->hl_kbddev) {
		printf("hil%d: ", hilp->hl_kbddev);
		for (km = hkbd_map; km->kbd_code; km++)
			if (km->kbd_code == hilp->hl_kbdlang) {
				printf("%s ", km->kbd_desc);
				break;
			}
		printf("keyboard\n");
	}
	/*
	 * ID module.
	 * Attempt to locate the first ID module and print out its
	 * security code.  Is this a good idea??
	 */
	id = hiliddev(hilp);
	if (id) {
		hilp->hl_cmdbp = hilp->hl_cmdbuf;
		hilp->hl_cmddev = id;
		send_hildev_cmd(hilp, id, HILSECURITY);
		len = hilp->hl_cmdbp - hilp->hl_cmdbuf;
		hilp->hl_cmdbp = hilp->hl_cmdbuf;
		hilp->hl_cmddev = 0;
		printf("hil%d: security code", id);
		for (id = 0; id < len; id++)
			printf(" %x", hilp->hl_cmdbuf[id]);
		while (id++ < 16)
			printf(" 0");
		printf("\n");
	}
}

#define HILAR1	0x3E
#define HILAR2	0x3F

/*
 * Called after the loop has reconfigured.  Here we need to:
 *	- determine how many devices are on the loop
 *	  (some may have been added or removed)
 *	- locate the ITE keyboard (if any) and ensure
 *	  that it is in the proper state (raw or cooked)
 *	  and is set to use the proper language mapping table
 *	- ensure all other keyboards are raw
 * Note that our device state is now potentially invalid as
 * devices may no longer be where they were.  What we should
 * do here is either track where the devices went and move
 * state around accordingly or, more simply, just mark all
 * devices as HIL_DERROR and don't allow any further use until
 * they are closed.  This is a little too brutal for my tastes,
 * we prefer to just assume people won't move things around.
 */
void
hilconfig(struct hilloop *hilp)
{
	u_char db;
	spl_t s;

	s = splhil();
#ifdef DEBUG
	if (hildebug & HDB_CONFIG) {
		printf("hilconfig: reconfigured: ");
		send_hil_cmd(hilp->hl_addr, HIL_READLPSTAT, NULL, 0, &db);
		printf("LPSTAT %x, ", db);
		send_hil_cmd(hilp->hl_addr, HIL_READLPCTRL, NULL, 0, &db);
		printf("LPCTRL %x, ", db);
		send_hil_cmd(hilp->hl_addr, HIL_READKBDSADR, NULL, 0, &db);
		printf("KBDSADR %x\n", db);
	}
#endif
	/*
	 * Determine how many devices are on the loop.
	 * Mark those as alive and real, all others as dead.
	 */
	db = 0;
	send_hil_cmd(hilp->hl_addr, HIL_READLPSTAT, NULL, 0, &db);
	hilp->hl_maxdev = db & LPS_DEVMASK;
#ifdef DEBUG
	if (hildebug & HDB_CONFIG)
		printf("hilconfig: %d devices found\n", hilp->hl_maxdev);
#endif
	for (db = 1; db < NHILD; db++) {
		if (db <= hilp->hl_maxdev)
			hilp->hl_device[db].hd_flags |= HIL_ALIVE;
		else
			hilp->hl_device[db].hd_flags &= ~HIL_ALIVE;
		hilp->hl_device[db].hd_flags &= ~HIL_PSEUDO;
	}
#ifdef DEBUG
	if (hildebug & (HDB_CONFIG|HDB_KEYBOARD))
		printf("hilconfig: max device %d\n", hilp->hl_maxdev);
#endif
	if (hilp->hl_maxdev == 0) {
		hilp->hl_kbddev = 0;
		splx(s);
		return;
	}
	/*
	 * Find out where the keyboards are and record the ITE keyboard
	 * (first one found).  If no keyboards found, we are all done.
	 */
	db = 0;
	send_hil_cmd(hilp->hl_addr, HIL_READKBDSADR, NULL, 0, &db);
#ifdef DEBUG
	if (hildebug & HDB_KEYBOARD)
		printf("hilconfig: keyboard: KBDSADR %x, old %d, new %d\n",
		       db, hilp->hl_kbddev, ffs((int)db));
#endif
	hilp->hl_kbddev = ffs((int)db);
	if (hilp->hl_kbddev == 0) {
		splx(s);
		return;
	}
	/*
	 * Determine if the keyboard should be cooked or raw and configure it.
	 */
	db = (hilp->hl_kbdflags & KBD_RAW) ? 0 : 1 << (hilp->hl_kbddev - 1);
	send_hil_cmd(hilp->hl_addr, HIL_WRITEKBDSADR, &db, 1, NULL);
	/*
	 * Re-enable autorepeat in raw mode, cooked mode AR is not affected.
	 */
	if (hilp->hl_kbdflags & (KBD_AR1|KBD_AR2)) {
		db = (hilp->hl_kbdflags & KBD_AR1) ? HILAR1 : HILAR2;
		hilp->hl_cmddev = hilp->hl_kbddev;
		send_hildev_cmd(hilp, hilp->hl_kbddev, db);
		hilp->hl_cmddev = 0;
	}
	/*
	 * Determine the keyboard language configuration, but don't
	 * override a user-specified setting.
	 */
	db = 0;
	send_hil_cmd(hilp->hl_addr, HIL_READKBDLANG, NULL, 0, &db);
#ifdef DEBUG
	if (hildebug & HDB_KEYBOARD)
		printf("hilconfig: language: old %x new %x\n",
		       hilp->hl_kbdlang, db);
#endif
	if (hilp->hl_kbdlang != KBD_SPECIAL) {
		struct kbdmap *km;

		for (km = hkbd_map; km->kbd_code; km++)
			if (km->kbd_code == db) {
				hilp->hl_kbdlang = db;
				/* XXX */
				kbd_keymap = km->kbd_keymap;
				kbd_shiftmap = km->kbd_shiftmap;
				kbd_ctrlmap = km->kbd_ctrlmap;
				kbd_ctrlshiftmap = km->kbd_ctrlshiftmap;
				kbd_stringmap = km->kbd_stringmap;
			}
	}
	splx(s);
}

void
hilreset(struct hilloop *hilp)
{
	register struct hil_dev *hildevice = hilp->hl_addr;
	u_char db;

#ifdef DEBUG
	if (hildebug & HDB_FOLLOW)
		printf("hilreset(%x)\n", hilp);
#endif
	/*
	 * Initialize the loop: reconfigure, don't report errors,
	 * cook keyboards, and enable autopolling.
	 */
	hilp->hl_maxdev = NHILD + 1;
	db = LPC_RECONF | LPC_KBDCOOK | LPC_NOERROR | LPC_AUTOPOLL;
	send_hil_cmd(hildevice, HIL_WRITELPCTRL, &db, 1, NULL);
	/*
	 * Delay one second for reconfiguration and then read the the
	 * data register to clear the interrupt (if the loop reconfigured).
	 */
	DELAY(1000000);
	if (READHILSTAT(hildevice) & HIL_DATA_RDY)
		db = READHILDATA(hildevice);
	/*
	 * The HIL loop may have reconfigured.  If so we proceed on,
	 * if not we loop until a successful reconfiguration is reported
	 * back to us.  The HIL loop will continue to attempt forever.
	 * Probably not very smart.
	 */
	do {
		send_hil_cmd(hildevice, HIL_READLPSTAT, NULL, 0, &db);
        } while ((db & (LPS_CONFFAIL|LPS_CONFGOOD)) == 0);
	/*
	 * At this point, the loop should have reconfigured and hilconfig()
	 * has been called.  For various reasons (e.g. interrupts turned
	 * off) this may not always be the case, so we may need to call
	 * hilconfig() manually.  Do that now.
	 *
	 * XXX note "volatile" to force reread of memory for hl_maxdev.
	 */
	if (((volatile struct hilloop *)hilp)->hl_maxdev == NHILD + 1) {
#ifdef DEBUG
		if (hildebug & HDB_CONFIG)
			printf("hilreset(%x): doing manual reconfig\n", hilp);
#endif
		hilp->hl_maxdev = 0;
		hilconfig(hilp);
	}
	/*
	 * The keyboard has been determined, so enable interrupts.
	 */
	send_hil_cmd(hildevice, HIL_INTON, NULL, 0, NULL);
#ifdef DEBUG
	if (hildebug & HDB_FOLLOW)
		printf("hilreset(%x): done\n", hilp);
#endif
}

void
hilbeep(
	struct hilloop *hilp,
	struct _hilbell *bp)
{
	u_char buf[2];

	buf[0] = ~((bp->duration - 10) / 10);
	buf[1] = bp->frequency;
	send_hil_cmd(hilp->hl_addr, HIL_SETTONE, buf, 2, NULL);
}

/*
 * Locate and return the address of the first ID module, 0 if none present.
 */
int
hiliddev(struct hilloop *hilp)
{
	register int i, len;

#ifdef DEBUG
	if (hildebug & HDB_IDMODULE)
		printf("hiliddev(%x): max %d, looking for idmodule...",
		       hilp, hilp->hl_maxdev);
#endif
	for (i = 1; i <= hilp->hl_maxdev; i++) {
		hilp->hl_cmdbp = hilp->hl_cmdbuf;
		hilp->hl_cmddev = i;
		send_hildev_cmd(hilp, i, HILIDENTIFY);
		/*
		 * XXX: the final condition checks to ensure that the
		 * device ID byte is in the range of the ID module (0x30-0x3F)
		 */
		len = hilp->hl_cmdbp - hilp->hl_cmdbuf;
		if (len > 1 && (hilp->hl_cmdbuf[1] & HILSCBIT) &&
		    (hilp->hl_cmdbuf[0] & 0xF0) == 0x30) {
			hilp->hl_cmdbp = hilp->hl_cmdbuf;
			hilp->hl_cmddev = i;
			send_hildev_cmd(hilp, i, HILSECURITY);
			break;
		}
	}		
	hilp->hl_cmdbp = hilp->hl_cmdbuf;
	hilp->hl_cmddev = 0;

#ifdef DEBUG
	if (hildebug & HDB_IDMODULE)
		if (i <= hilp->hl_maxdev)
			printf("found at %d\n", i);
		else
			printf("not found\n");
#endif
	return(i <= hilp->hl_maxdev ? i : 0);
}

/*
 * Low level routines which actually talk to the 8042 chip.
 */

/*
 * Send a command to the 8042 with zero or more bytes of data.
 * If rdata is non-null, wait for and return a byte of data.
 * We run at splimp() to make the transaction as atomic as
 * possible without blocking the clock (is this necessary?)
 */
void
send_hil_cmd(
	struct hil_dev *hildevice,
	u_char cmd, 
	u_char *data, 
	u_char dlen,
	u_char *rdata)
{
	u_char status;
	int s = splimp();

	HILWAIT(hildevice);
	WRITEHILCMD(hildevice, cmd);
	while (dlen--) {
	  	HILWAIT(hildevice);
		WRITEHILDATA(hildevice, *data++);
	}
	if (rdata) {
		do {
			HILDATAWAIT(hildevice);
			status = READHILSTAT(hildevice);
			*rdata = READHILDATA(hildevice);
		} while (((status >> HIL_SSHIFT) & HIL_SMASK) != HIL_68K);
	}
	splx(s);
}

/*
 * Send a command to a device on the loop.
 * Since only one command can be active on the loop at any time,
 * we must ensure that we are not interrupted during this process.
 * Hence we mask interrupts to prevent potential access from most
 * interrupt routines and turn off auto-polling to disable the
 * internally generated poll commands.
 *
 * splhigh is extremely conservative but insures atomic operation,
 * splimp (clock only interrupts) seems to be good enough in practice.
 */
void
send_hildev_cmd(
		struct hilloop *hilp,
		char device, 
		char cmd)
{
	register struct hil_dev *hildevice = hilp->hl_addr;
	u_char status, c;
	int s = splimp();

	polloff(hildevice);

	/*
	 * Transfer the command and device info to the chip
	 */
	HILWAIT(hildevice);
	WRITEHILCMD(hildevice, HIL_STARTCMD);
  	HILWAIT(hildevice);
	WRITEHILDATA(hildevice, 8 + device);
  	HILWAIT(hildevice);
	WRITEHILDATA(hildevice, cmd);
  	HILWAIT(hildevice);
	WRITEHILDATA(hildevice, HIL_TIMEOUT);
	/*
	 * Trigger the command and wait for completion
	 */
	HILWAIT(hildevice);
	WRITEHILCMD(hildevice, HIL_TRIGGER);
	hilp->hl_cmddone = FALSE;
	do {
		HILDATAWAIT(hildevice);
		status = READHILSTAT(hildevice);
		c = READHILDATA(hildevice);
		hil_process_int(hilp, status, c);
	} while (!hilp->hl_cmddone);

	pollon(hildevice);
	splx(s);
}

/*
 * Turn auto-polling off and on.
 * Also disables and enable auto-repeat.  Why?
 */
void
polloff(struct hil_dev *hildevice)
{
	register char db;

	/*
	 * Turn off auto repeat
	 */
	HILWAIT(hildevice);
	WRITEHILCMD(hildevice, HIL_SETARR);
	HILWAIT(hildevice);
	WRITEHILDATA(hildevice, 0);
	/*
	 * Turn off auto-polling
	 */
	HILWAIT(hildevice);
	WRITEHILCMD(hildevice, HIL_READLPCTRL);
	HILDATAWAIT(hildevice);
	db = READHILDATA(hildevice);
	db &= ~LPC_AUTOPOLL;
	HILWAIT(hildevice);
	WRITEHILCMD(hildevice, HIL_WRITELPCTRL);
	HILWAIT(hildevice);
	WRITEHILDATA(hildevice, db);
	/*
	 * Must wait til polling is really stopped
	 */
	do {	
		HILWAIT(hildevice);
		WRITEHILCMD(hildevice, HIL_READBUSY);
		HILDATAWAIT(hildevice);
		db = READHILDATA(hildevice);
	} while (db & BSY_LOOPBUSY);
}

void
pollon(struct hil_dev *hildevice)
{
	register char db;

	/*
	 * Turn on auto polling
	 */
	HILWAIT(hildevice);
	WRITEHILCMD(hildevice, HIL_READLPCTRL);
	HILDATAWAIT(hildevice);
	db = READHILDATA(hildevice);
	db |= LPC_AUTOPOLL;
	HILWAIT(hildevice);
	WRITEHILCMD(hildevice, HIL_WRITELPCTRL);
	HILWAIT(hildevice);
	WRITEHILDATA(hildevice, db);
	/*
	 * Turn on auto repeat
	 */
	HILWAIT(hildevice);
	WRITEHILCMD(hildevice, HIL_SETARR);
	HILWAIT(hildevice);
	WRITEHILDATA(hildevice, ar_format(KBD_ARR));
}

#ifdef DEBUG
void
printhilpollbuf(struct hilloop *hilp)
{
  	register u_char *cp;
	register int i, len;

	cp = hilp->hl_pollbuf;
	len = hilp->hl_pollbp - cp;
	for (i = 0; i < len; i++)
		printf("%x ", hilp->hl_pollbuf[i]);
	printf("\n");
}

void
printhilcmdbuf(struct hilloop *hilp)
{
  	register u_char *cp;
	register int i, len;

	cp = hilp->hl_cmdbuf;
	len = hilp->hl_cmdbp - cp;
	for (i = 0; i < len; i++)
		printf("%x ", hilp->hl_cmdbuf[i]);
	printf("\n");
}

void
hilreport(struct hilloop *hilp)
{
	register int i, len;
	spl_t s = splhil();

	for (i = 1; i <= hilp->hl_maxdev; i++) {
		hilp->hl_cmdbp = hilp->hl_cmdbuf;
		hilp->hl_cmddev = i;
		send_hildev_cmd(hilp, i, HILIDENTIFY);
		printf("hil%d: id: ", i);
		printhilcmdbuf(hilp);
		len = hilp->hl_cmdbp - hilp->hl_cmdbuf;
		if (len > 1 && (hilp->hl_cmdbuf[1] & HILSCBIT)) {
			hilp->hl_cmdbp = hilp->hl_cmdbuf;
			hilp->hl_cmddev = i;
			send_hildev_cmd(hilp, i, HILSECURITY);
			printf("hil%d: sc: ", i);
			printhilcmdbuf(hilp);
		}
	}		
	hilp->hl_cmdbp = hilp->hl_cmdbuf;
	hilp->hl_cmddev = 0;
	splx(s);
}
#endif
