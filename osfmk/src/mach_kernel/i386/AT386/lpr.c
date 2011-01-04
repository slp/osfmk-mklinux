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
 * MkLinux
 */
/* 
 * Mach Operating System
 * Copyright (c) 1993-1990 Carnegie Mellon University
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
/* CMU_HIST */
/*
 * Revision 2.9  93/05/10  21:19:51  rvb
 * 	Lint.
 * 	[93/05/08  11:18:33  af]
 * 
 * Revision 2.8  93/02/04  07:58:18  danner
 * 	Warning fixups.
 * 	[93/01/27            danner]
 * 
 * Revision 2.7  93/01/24  13:18:12  danner
 * 	Convert lpr_addr to vm_offset_t.
 * 	[93/01/19            rvb]
 * 	Add d-link "600" ethernet device "de"
 * 	[92/08/13            rvb]
 * 
 * Revision 2.6  93/01/14  17:31:37  danner
 * 	Proper spl typing.
 * 	[92/11/30            af]
 * 
 * Revision 2.5  92/07/09  22:54:47  rvb
 * 	More robust probe.  On some hardware, inb's
 * 	return 0x00 vs 0xff.
 * 	[92/06/18            rvb]
 * 
 * Revision 2.4  92/04/03  12:09:11  rpd
 * 	Allow for stupid with more than one lpr as unit 0.
 * 	[92/03/10  10:22:40  rvb]
 * 
 * Revision 2.3  92/02/25  14:18:29  elf
 * 	Added protection against spurious interrupts.
 * 	[92/02/24            danner]
 * 
 * Revision 2.2  92/02/19  16:30:23  elf
 * 	Add lpr and par devices.  (taken from 2.5)
 * 	NOT DEBUGGED
 * 	[92/02/13            rvb]
 * 
 * Revision 2.3  91/07/15  13:43:28  mrt
 * 	Don't set UNUSED bits in intr_enable.
 * 	[91/06/27            mg32]
 * 
 * Revision 2.2  91/04/02  12:12:26  mbj
 * 	Use take_<>_irq() vs direct manipulations of ivect and friends.
 * 	[90/08/20            rvb]
 * 	Created 08/05/90.
 * 	Parallel port printer driver.
 * 	[90/08/14            mg32]
 * 
 */

/* 
 *	Parallel port printer driver v1.0
 *	All rights reserved.
 */ 
  
#include <lpr.h>
#if NLPR > 0
  
#ifdef	MACH_KERNEL
#include <cpus.h>
#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <platforms.h>
#if 0
#include <mach/std_types.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <device/conf.h>
#include <device/errno.h>
#include <device/tty.h>
#include <device/io_req.h>
#include <kern/spl.h>
#else	/* MACH_KERNEL */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/file.h>
#endif	/* MACH_KERNEL */
  
#include <i386/ipl.h>
#include <i386/pio.h>
#include <chips/busses.h>
#include <i386/AT386/lprreg.h>
#include <i386/AT386/lpr_entries.h>
#include <i386/AT386/misc_protos.h>

#include <i386/AT386/mp/mp.h>
#ifdef	CBUS
#include <busses/cbus/cbus.h>
#else	/* CBUS */
#define at386_io_lock_state()
#define at386_io_lock(op)	(TRUE)
#define at386_io_unlock()
#endif	/* CBUS */
  
#if 0
extern void 	splx();
extern spl_t	spltty();
extern void 	timeout();
extern void 	ttrstrt();
#endif

/* 
 * Driver information for auto-configuration stuff.
 */

extern int 	lprprobe(caddr_t port, struct bus_device *dev);
/* extern int	lprintr(int unit); */
extern void	lprstart(struct tty *tp);
extern void	lprattach(struct bus_device *dev);
extern void	lprstop(struct tty *tp, int flags);
/*
int lprgetstat(), lprsetstat();
*/

struct bus_device *lprinfo[NLPR];	/* ??? */

static caddr_t lpr_std[NLPR] = { 0 };
static struct bus_device *lpr_info[NLPR];
struct bus_driver lprdriver = {
	(probe_t) lprprobe, 0, lprattach, 0, lpr_std, "lpr", lpr_info, 0, 0, 0};

struct tty	lpr_tty[NLPR];

int lpr_alive[NLPR];


/* #define LPR_DEBUG */

#ifdef LPR_DEBUG
/***************************************/
/* for debug */
int lpr_debug = 0;

#define LPR_TRACE_BUFSIZ 5000
int lpr_trace = 0;
struct lpr_trace_buf {
    char cd;
    char port;
    char spl_level;
    char data;
} lpr_trace_buf[LPR_TRACE_BUFSIZ];
int lpr_trace_ptr = 0;

#define LPR_OPEN 1
#define LPR_WRITE 2
#define LPR_START 3
#define LPR_OUTPUT 4
#define LPR_INTR 5
#define LPR_INTRON 6
#define LPR_CLOSE 7

char *lpr_debug_mes[] = {
    0,
    "open",
    "write",
    "start",
    "output",
    "intr",
    "intron",
    "close"
};

extern spl_t curr_ipl[];

#define lpr_tr(code, x) \
    if (lpr_trace) { \
	int pt; \
	int s; \
 \
	s = spltty(); \
	lpr_trace_buf[lpr_trace_ptr].cd = code; \
	pt = inb(INTR_ENAB((i386_ioport_t) ((long) lpr_info[0]->address))); \
	lpr_trace_buf[lpr_trace_ptr].port = (char) pt; \
	lpr_trace_buf[lpr_trace_ptr].spl_level = (char) curr_ipl[0]; \
	lpr_trace_buf[lpr_trace_ptr].data = (char) (x); \
	lpr_trace_ptr++; \
	if (lpr_trace_ptr >= LPR_TRACE_BUFSIZ) { \
	    lpr_trace_ptr = 0; \
	} \
	splx(s); \
	if (lpr_debug) { \
	    s = splhigh(); \
	    printf("***%s:p=%x:spl=%d:d=%x\n", lpr_debug_mes[code], pt, \
		   curr_ipl[0], (x)); \
	    splx(s); \
	} \
    }

#define LPR_PRINT_BUFSIZ 2000
char lpr_print_buf[LPR_PRINT_BUFSIZ];
char *lpr_print_bufp = lpr_print_buf;

int last_write_status = 0;
int last_ptr = 0;

#else /* LPR_DEBUG */

#define lpr_tr(code, x)

#endif /* LPR_DEBUG */
/***************************************/

int lpr_timeout_count = 0;

extern void lpr_timeout(struct tty *tp);
extern void ttstart(struct tty *tp);

void lpr_timeout(struct tty *tp)
{
    spl_t		s;

    lpr_timeout_count++;
    if (tp == 0) {
	panic("lpr_timeout");
    }

    s = spltty();
    simple_lock(&tp->t_lock);

    tp->t_state &= ~(TS_TIMEOUT|TS_BUSY);
    ttstart(tp);

    simple_unlock(&tp->t_lock);
    splx(s);
}

int
lprprobe(port, dev)
    caddr_t port;
    struct bus_device *dev;
{
	i386_ioport_t	addr = (long) dev->address;
	int	unit = dev->unit;
	int ret;

	if ((unit < 0) || (unit > NLPR)) {
		printf("com %d out of range\n", unit);
		return(0);
	}

	/* data strobe signal must be inverse */
/*
	outb(INTR_ENAB(addr),0x07);
*/
	outb(INTR_ENAB(addr),0x06);
	outb(DATA(addr),0xaa);
	ret = inb(DATA(addr)) == 0xaa;
	if (ret) {
		if (lpr_alive[unit]) {
			printf("lpr: Multiple alive entries for unit %d.\n", unit);
			printf("lpr: Ignoring entry with address = %x .\n", addr);
			ret = 0;
		} else
			lpr_alive[unit]++;
	}
	return(ret);
}

void lprattach(struct bus_device *dev)
{
	u_char		unit = dev->unit;
	i386_ioport_t	addr = (long) dev->address;
	struct tty	*tp = &lpr_tty[unit];

	take_dev_irq(dev);
	printf(", port = %x, spl = %d, pic = %d.",
	       dev->address, dev->sysdep, dev->sysdep1);
	lprinfo[unit] = dev;
  
	outb(INTR_ENAB(addr), inb(INTR_ENAB(addr)) & 0x0f);
	ttychars(tp);
	return;
}

io_return_t
lpropen(dev_t dev, dev_mode_t flag, io_req_t ior)
{
    int unit = minor(dev);
    struct bus_device *isai;
    struct tty *tp;
    i386_ioport_t addr;
    io_return_t ret;
    spl_t s, ss;
    at386_io_lock_state();
  
    if (unit >= NLPR || (isai = lprinfo[unit]) == 0 || isai->alive == 0) {
	return(ENXIO);
    }
    tp = &lpr_tty[unit];
    at386_io_lock(MP_DEV_WAIT);
    lpr_tr(LPR_OPEN, 0);
    s = spltty();
    simple_lock(&tp->t_lock);
    if (!(tp->t_state & (TS_ISOPEN|TS_WOPEN))) {
	addr = (long) isai->address;
	tp->t_dev = dev;
	tp->t_addr = isai->address;
	tp->t_oproc = lprstart;
	tp->t_state |= TS_WOPEN;
	tp->t_stop = lprstop;
	tp->t_getstat = lprgetstat;
	tp->t_setstat = lprsetstat;
/*
	if ((tp->t_state & TS_ISOPEN) == 0) {
	    ttychars(tp);
	}
*/
	lpr_tr(LPR_INTRON, 0);
	outb(INTR_ENAB(addr), inb(INTR_ENAB(addr)) | 0x10);
	tp->t_state |= TS_CARR_ON;
	tp->t_state &= ~TS_WOPEN;
    }
    if (inb(STATUS(addr) & 0x20)) {
	ss = splhigh();
	printf("Printer out of paper!\n");
	splx(ss);
	tp->t_state &= ~TS_CARR_ON;
    }
    simple_unlock(&tp->t_lock);
    splx(s);
    at386_io_unlock();
    ret = char_open(dev, tp, flag, ior);
/*
    return (char_open(dev, tp, flag, ior));
*/
#ifdef LPR_DEBUG
    if (lpr_debug) {
	ss = splhigh();
	printf("lpropen:char_open: %d\n", ret);
	splx(ss);
    }
#endif /* LPR_DEBUG */
    return (ret);
}

void
lprclose(dev_t dev)
{
    int 		unit = minor(dev);
    struct	tty	*tp = &lpr_tty[unit];
    i386_ioport_t	addr = 	(long) lprinfo[unit]->address;
    spl_t s;
    at386_io_lock_state();
  
#ifndef	MACH_KERNEL
    (*linesw[tp->t_line].l_close)(tp);
#endif	/* MACH_KERNEL */
    at386_io_lock(MP_DEV_WAIT);
    lpr_tr(LPR_CLOSE, 0);
    s = spltty();
    simple_lock(&tp->t_lock);
    ttydrain(tp);
    ttyclose(tp);
    simple_unlock(&tp->t_lock);
    splx(s);
/*
	if (tp->t_state&TS_HUPCLS || (tp->t_state&TS_ISOPEN)==0) {
		outb(INTR_ENAB(addr), inb(INTR_ENAB(addr)) & 0x0f);
		tp->t_state &= ~TS_BUSY;
	} 
*/
    outb(INTR_ENAB(addr), inb(INTR_ENAB(addr)) & 0x0f);
    at386_io_unlock();
}

int
lprread(dev_t dev, io_req_t ior)
{
	return (char_read(&lpr_tty[minor(dev)], ior));
}

int
lprwrite(dev_t dev, io_req_t ior)
{
    int ret;
    struct tty *tp;
    i386_ioport_t addr;
    int status;
    spl_t s, ss;
    at386_io_lock_state();

    tp = &lpr_tty[minor(dev)];
    at386_io_lock(MP_DEV_WAIT);
    s = spltty();
    simple_lock(&tp->t_lock);
    addr = (long) tp->t_addr;
    status = inb(STATUS(addr));
#ifdef LPR_DEBUG
    last_write_status = status;
    last_ptr = lpr_trace_ptr;
#endif /* LPR_DEBUG */
    if (status & 0x20) {
	/* printf("Printer out of paper!\n"); */
	tp->t_state &= ~TS_CARR_ON;
    }
    simple_unlock(&tp->t_lock);
    splx(s);

#ifdef LPR_DEBUG
    if (lpr_debug) {
	ss = splhigh();
	/* for debug */
	printf("ior = %x\n", ior);
/*
	printf("data(%x): %s\n", ior->io_data, ior->io_data);
	splx(ss);
*/
    }
#endif /* LPR_DEBUG */
    lpr_tr(LPR_WRITE, status);
    ret = char_write(&lpr_tty[minor(dev)], ior);
    at386_io_unlock();
#ifdef LPR_DEBUG
    if (lpr_debug) {
	ss = splhigh();
	printf("char_write: %d\n", ret);
	splx(ss);
    }
#endif
    return (ret);
}

int
lprportdeath(dev_t dev, ipc_port_t port)
{
	return (tty_portdeath(&lpr_tty[minor(dev)], port));
}

io_return_t
lprgetstat(dev_t dev, dev_flavor_t flavor, int *data, natural_t *count)
{
	io_return_t	result = D_SUCCESS;
	int		unit = minor(dev);

	switch (flavor) {
	default:
		result = tty_get_status(&lpr_tty[unit], flavor, data, count);
		break;
	}
	return (result);
}

io_return_t
lprsetstat(dev_t dev, dev_flavor_t flavor, int *data, natural_t count)
{
	io_return_t	result = D_SUCCESS;
	int 		unit = minor(dev);
	int		s;

	switch (flavor) {
	default:
		result = tty_set_status(&lpr_tty[unit], flavor, data, count);
/*		if (result == D_SUCCESS && flavor == TTY_STATUS)
			lprparam(unit);
*/		return (result);
	}
	return (D_SUCCESS);
}

void lprintr(int unit)
{
	register struct tty *tp = &lpr_tty[unit];

/*
	lpr_tr(LPR_INTR,
	       inb(STATUS((i386_ioport_t) ((long) lpr_info[0]->address))));
*/
	simple_lock(&tp->t_lock);
	lpr_tr(LPR_INTR, tp->t_state);
	if (tp->t_state & TS_TIMEOUT) {
	    if (untimeout((timeout_fcn_t) lpr_timeout, tp) == FALSE) {
		panic("cannot untimeout\n");
	    }
	    tp->t_state &= ~TS_TIMEOUT;
	}
	if ((tp->t_state & TS_ISOPEN) == 0) {
	    simple_unlock(&tp->t_lock);
	    return;
	}

#ifdef LPR_DEBUG
	if (lpr_debug) {
	    spl_t ss = splhigh();

	    printf("lprintr\n");
	    splx(ss);
	}
#endif
	tp->t_state &= ~TS_BUSY;
	if (tp->t_state&TS_FLUSH)
		tp->t_state &=~TS_FLUSH;
	/* tt_write_wakeup(tp); */
	lprstart(tp);
	simple_unlock(&tp->t_lock);
}   

void lprstart(struct tty *tp)
{
	spl_t s = spltty();
	spl_t ss;
	i386_ioport_t addr = (long) tp->t_addr;
	int status = inb(STATUS(addr));
	char nch;
	at386_io_lock_state();

	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP|TS_BUSY)) {
		splx(s);
		return;
	}

	at386_io_lock(MP_DEV_WAIT);
	lpr_tr(LPR_START, tp->t_state);
	if (status & 0x20) {
	    ss = splhigh();
		printf("Printer out of paper!\n");
	    splx(ss);
		tp->t_state &= ~TS_CARR_ON;
		if (!(tp->t_state & TS_TIMEOUT)) {
		    timeout((timeout_fcn_t) lpr_timeout, tp, 1000);
		    tp->t_state |= TS_TIMEOUT;
		}
		splx(s);
		at386_io_unlock();
		return;
	} else {
	    tp->t_state |= TS_CARR_ON;
	}

	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		tt_write_wakeup(tp);
	}
	if (tp->t_outq.c_cc == 0) {
		splx(s);
		at386_io_unlock();
		return;
	}
#if 0
	if (STATUS(addr) & 0x80) {
	    /* the device is busy */
	    delay(100);
	    if (STATUS(addr) & 0x80) {
		/* still busy */
		if (!(tp->t_state & TS_TIMEOUT)) {
		    timeout((timeout_fcn_t) ttrstrt, tp, 1000);
		    tp->t_state |= TS_TIMEOUT;
		}
		splx(s);
		at386_io_unlock();
		return;
	    }
	}
#endif
	if (!(tp->t_state & TS_TIMEOUT)) {
	    timeout((timeout_fcn_t) lpr_timeout, tp, 1000);
	    tp->t_state |= TS_TIMEOUT;
	}
	nch = getc(&tp->t_outq);
	if ((tp->t_flags & LITOUT) == 0 && (nch & 0200)) {
		timeout((timeout_fcn_t) ttrstrt, tp, (nch & 0x7f) + 6);
		tp->t_state |= TS_TIMEOUT;
		splx(s);
		at386_io_unlock();
		return;
	}
#ifdef LPR_DEBUG
	/* for debug */
	if (lpr_debug) {
	    ss = splhigh();
	    printf("%x", nch);
	    splx(ss);
	}
#endif /* LPR_DEBUG */
	if (nch > 0) {
	    /* lpr_tr(LPR_OUTPUT, nch); */
	    lpr_tr(LPR_OUTPUT, STATUS(addr));

#ifdef LPR_DEBUG
	    *lpr_print_bufp++ = (char) nch;
	    if (lpr_print_bufp >= &lpr_print_buf[LPR_PRINT_BUFSIZ]) {
		lpr_print_bufp = lpr_print_buf;
	    }
#endif /* LPR_DEBUG */

	    outb(DATA(addr), nch);
	    outb(INTR_ENAB(addr),inb(INTR_ENAB(addr)) | 0x01);
	    outb(INTR_ENAB(addr),inb(INTR_ENAB(addr)) & 0x1e);
	    tp->t_state |= TS_BUSY;
	}
	splx(s);
	at386_io_unlock();
	return;
}

void
lprstop(tp, flags)
    register struct tty *tp;
    int	flags;
{
	if ((tp->t_state & TS_BUSY) && (tp->t_state & TS_TTSTOP) == 0)
		tp->t_state |= TS_FLUSH;
}
int lprpr(int unit);
void lprpr_addr(i386_ioport_t addr);

#ifdef LPR_DEBUG
int
lprpr(int unit)
{
	lprpr_addr((i386_ioport_t) ((long) lprinfo[unit]->address));
	return 0;
}

void
lprpr_addr(i386_ioport_t addr)
{
    at386_io_lock_state();

    at386_io_lock(MP_DEV_WAIT);
    printf("DATA(%x) %x, STATUS(%x) %x, INTR_ENAB(%x) %x\n",
	   DATA(addr), inb(DATA(addr)),
	   STATUS(addr), inb(STATUS(addr)),
	   INTR_ENAB(addr), inb(INTR_ENAB(addr)));
    at386_io_unlock();
}
#endif /* LPR_DEBUG */
#endif /* NLPR */
