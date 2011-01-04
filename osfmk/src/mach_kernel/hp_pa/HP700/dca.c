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
 * 	Utah $Hdr: dca.c 1.18 94/12/16$
 */

/*
 * Copyright (c) 1982, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)dca.c	7.1 (Berkeley) 6/5/86
 */

#include "dca.h"
#if NDCA > 0
/*
 *  Driver for National Semiconductor INS8250/NS16550AF/WD16C552 UARTs.
 *  Includes:
 *	98626/98644/internal serial interface on hp300/hp400
 *	internal serial ports on hp700
 *
 *  N.B. On the hp700, there is a "secret bit" with undocumented behavior.
 *  The third bit of the Modem Control Register (MCR_IEN == 0x08) must be
 *  set to enable interrupts.  If DEBUG, we'll check at various places to
 *  make sure it actually is enabled.
 */
#include <kgdb.h>
#include <kgdb_lan.h>

#include <types.h>
#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <device/tty.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <kern/spl.h>

#include <mach_kdb.h>
#include <machine/asp.h>
#include <machine/iodc.h>
#include <machine/pdc.h>
#include <machine/machparam.h>

#include <machine/cpu.h>
#include <hp_pa/HP700/device.h>
#include <hp_pa/HP700/dcareg.h>
#include <hp_pa/HP700/cons.h>
#include <hp_pa/iomod.h>

#include <hp_pa/trap.h>
#include <hp_pa/HP700/bus_protos.h>

#ifndef DEFAULT_BAUD_RATE
#define DEFAULT_BAUD_RATE 9600
#endif

#ifndef CRTSCTS
#define CRTSCTS MDMBUF		/* XXX - hack waiting for 4.4 */
#endif

#ifdef KGDB_LAN		/* XXX */
#undef KGDB
#endif

int	dcasoftCAR = 0;
int	dca_active = 0;
int	ndca = NDCA;
int	dcaconsole = -1;
int	dcadefaultrate = DEFAULT_BAUD_RATE;
int	dcadelay[NDCA] = { 0 };
struct	dcadevice *dca_addr[NDCA] = { 0 };
struct	tty dca_tty[NDCA] = { 0 };

int	dcadefaultfifotrig = FIFO_TRIGGER_14;
int	dca_hasfifo = 0;
int	dcafifot[NDCA];

int	dca_speeds[] = {
	0, DCABRD(50), DCABRD(75), DCABRD(110), DCABRD(134), DCABRD(150),
	DCABRD(200), DCABRD(300), DCABRD(600), DCABRD(1200), DCABRD(1800),
	DCABRD(2400), DCABRD(4800), DCABRD(9600), DCABRD(19200), DCABRD(38400)
};

#ifdef KGDB
extern int kgdb_dev;
extern int kgdb_rate;
extern int kgdb_debug_init;
#define KGDB_ON	{ extern int kgdb_enabled; kgdb_enabled = 1; }
#endif

#ifdef MMAP
#include "user.h"
#include "proc.h"
#define DCA_MAPDEV(x)	(x & 0x80)
struct	dcammap {
	int	isopen;
	struct	proc *proc;
} dcammap[NDCA];
#endif

#ifdef DEBUG
long	fifoin[17];
long	fifoout[17];
long	dcaintrcount[16];
long	dcamintcount[16];
long	mcrintclear[NDCA][16];
#endif

#ifdef DEBUG
#define CHECKMCRINT(ix, u, dca) \
	if (((dca)->dca_mcr & MCR_IEN) == 0) { \
		mcrintclear[u][ix]++; \
		(dca)->dca_mcr |= MCR_IEN; \
	}
#else
#define CHECKMCRINT(ix, u, dca)
#endif

#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>

/* 
 * Forwards
 */

extern int dcacninit(struct consdev *);
extern void dcacnprobe(struct consdev *);
extern int dcacngetc(dev_t, int);
extern void dcacnputc(dev_t, int);

static int rate2speed(int);

static void dcainit(int, int);
static void dcaparam(int);

static void dcaeint(int, int, struct dcadevice *);
static void dcamint(int, struct dcadevice *);

/*
 * Externs
 */
extern void ttstart(struct tty *);

static int
rate2speed(int baudrate)
{
	static int rates[] = {0, 50, 75, 110, 134, 150, 200, 300, 600, 
			      1200, 1800, 2400, 4800, 9600, 19200, 38400};     

	register int b = 0;

	while(baudrate > rates[b])
		b++;

	return b;
}

void
dcaprobe(struct hp_device *hd)
{
	register struct dcadevice *dca;
	register int unit;

	dca = (struct dcadevice *)hd->hp_addr;

	unit = hd->hp_unit;
	if (unit == dcaconsole)
		DELAY(100000);

	/* look for a NS 16550AF UART with FIFOs */
	dcafifot[unit] = dcadefaultfifotrig;
	dca->dca_fifo = FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|dcafifot[unit];
	DELAY(100);
	if ((dca->dca_iir & IIR_FIFO_MASK) == IIR_FIFO_MASK)
		dca_hasfifo |= 1 << unit;

	dca_addr[unit] = dca;

	dca_active |= 1 << unit;
	if (hd->hp_flags)
		dcasoftCAR |= (1 << unit);

	/*
	 * May need to reset baud rate, etc. of console in case console
	 * defaults are different.  Also make sure console is always
	 * "hardwired".
	 */
	if (unit == dcaconsole) {
		dcainit(unit, dcadefaultrate);
		dcasoftCAR |= (1 << unit);
	}
	return;
}

int
dcaopen(dev_t dev, dev_mode_t flag, io_req_t ior)
{
	int         unit = minor(dev);
	struct tty *tp;
	spl_t       s;

	if (unit >= NDCA || (dca_active & (1 << unit)) == 0)
		return(D_NO_SUCH_DEVICE);

	tp = &dca_tty[unit];

#ifdef MMAP
	/*
	 * Disallow open if device is already opened for mapping
	 */
	if (dcammap[unit].isopen)
		return (EBUSY);
	/*
	 * If this is the mapped device:
	 *   disallow if already open or the console device
	 *   disable interrupts
	 */
	if (DCA_MAPDEV(dev)) {
		if ((tp->t_state & TS_ISOPEN) || unit == dcaconsole)
			return (EBUSY);
		s = spltty();

		dca_addr[unit]->dca_ier = 0;
		dcammap[unit].isopen = 1;
		splx(s);
		return(0);
	}
#endif
	s = spltty();

	if ((tp->t_state & (TS_ISOPEN|TS_WOPEN)) == 0) {
		ttychars(tp);
		tp->t_addr = (char*)dca_addr[unit];
		tp->t_dev = dev;
		tp->t_oproc = dcastart;
		tp->t_stop = dcastop;
		tp->t_mctl = dcamctl;
		tp->t_getstat = dcagetstat;
		tp->t_setstat = dcasetstat;

		tp->t_state |= TS_HUPCLS;
		tp->t_ispeed = dcadefaultrate;
		tp->t_ospeed = dcadefaultrate;
		tp->t_flags = (EVENP | ODDP | ECHO);
		if (unit == dcaconsole)
			tp->t_flags |= XTABS|CRMOD;
	}

	if ((tp->t_state & TS_ISOPEN) == 0)
		dcaparam(unit);

	tp->t_state |= TS_CARR_ON;

	splx(s);

	return (char_open(dev, tp, flag, ior));
}
 
void
dcaclose(dev_t dev, int flag)
{
	register struct tty *tp;
	register struct dcadevice *dca;
	register int unit;
	spl_t s;

	unit = minor(dev);

	dca = dca_addr[unit];
#ifdef MMAP
	/*
	 * If this is the mapped interface, then we haven't gone
	 * through the normal tty open sequence.  All we need to
	 * do is ensure that interrupts are cleared (but enabled).
	 */
	if (DCA_MAPDEV(dev)) {
		s = spltty();

		dcammap[unit].isopen = 0;
		dcammap[unit].proc = 0;
		dca->dca_ier = 0;

		splx(s);
	}
#endif
	tp = &dca_tty[unit];

	s = spltty();
	simple_lock(&tp->t_lock);

	ttstart(tp);
	ttydrain(tp);

	dca->dca_cfcr &= ~CFCR_SBREAK;
#ifdef KGDB
	/* do not disable interrupts if debugging */
	if (kgdb_dev != makedev(1, unit))
#endif
		dca->dca_ier = 0;
	(void) dcamctl(tp, 0, DMSET);
	if (tp->t_state & TS_HUPCLS)
		ttymodem(tp, 0);
	ttyclose(tp);
	simple_unlock(&tp->t_lock);
	splx(s);
}
 
io_return_t
dcaread(dev_t dev, io_req_t ior)
{
#ifdef MMAP
	if (DCA_MAPDEV(dev))
		return (EIO);
#endif

	return char_read(&dca_tty[minor(dev)], ior);
}
 
io_return_t
dcawrite(dev_t dev, io_req_t ior)
{
#ifdef MMAP
	if (DCA_MAPDEV(dev))
		return (EIO);
#endif
	return char_write(&dca_tty[minor(dev)], ior);
}
 
/*
 * Map the registers.  We assume that one page is enough.
 */
int
dcamap(dev_t dev, int off, int prot)
{
#ifdef MMAP
	int unit = minor(dev);

	if (!DCA_MAPDEV(dev) || off != 0)
		return (EINVAL);
	if (dcammap[unit].proc == 0)
		dcammap[unit].proc = u.u_procp;
	return (kvtop((u_int)dca_addr[unit]) >> PGSHIFT);
#else
	return 0;
#endif
}

void
dcaintr(int unit)
{
	register struct dcadevice *dca;
	register u_char code;
	register struct tty *tp;

	dca = dca_addr[unit];

#ifdef MMAP
	if (dcammap[unit].proc) {
		dca->dca_ier = 0;
		psignal(dcammap[unit].proc, SIGUSR2);
		return;
	}
#endif
	tp = &dca_tty[unit];
	while (1) {
		code = dca->dca_iir;
#ifdef DEBUG
		dcaintrcount[code & IIR_IMASK]++;
#endif
		switch (code & IIR_IMASK) {
		case IIR_NOPEND:
			CHECKMCRINT(0, unit, dca);
			return;
		case IIR_RXTOUT:
		case IIR_RXRDY:
			/* do time-critical read in-line */
/*
 * Process a received byte.  Inline for speed...
 */
#ifdef KGDB
#define	RCVBYTE() \
			code = dca->dca_data; \
			if ((tp->t_state & TS_ISOPEN) == 0) { \
				if (kgdb_dev == makedev(1, unit) && \
				    (code == '!' || code == '\003')) { \
					printf("kgdb trap from dca%d\n", unit); \
					KGDB_ON; \
					kgdb_break(); \
				} \
			} else \
				ttyinput(code, tp)
#else
#define	RCVBYTE() \
			code = dca->dca_data; \
			if ((tp->t_state & TS_ISOPEN) != 0) \
				ttyinput(code, tp)
#endif
			RCVBYTE();
			if (dca_hasfifo & (1 << unit)) {
#ifdef DEBUG
				register int fifocnt = 1;
#endif
				while ((code = dca->dca_lsr) & LSR_RCV_MASK) {
					if (code == LSR_RXRDY) {
						RCVBYTE();
					} else
						dcaeint(unit, code, dca);
#ifdef DEBUG
					fifocnt++;
#endif
				}
				CHECKMCRINT(1, unit, dca);
#ifdef DEBUG
				if (fifocnt > 16)
					fifoin[0]++;
				else
					fifoin[fifocnt]++;
#endif
			}
			break;
		case IIR_TXRDY:
			tp->t_state &=~ (TS_BUSY|TS_FLUSH);
			dcastart(tp);
			break;
		case IIR_RLS:
			dcaeint(unit, dca->dca_lsr, dca);
			CHECKMCRINT(2, unit, dca);
			break;
		default:
			if (code & IIR_NOPEND)
				return;
			log(LOG_WARNING, "dca%d: weird interrupt: 0x%x\n",
			    unit, code);
			/* fall through */
		case IIR_MLSC:
			dcamint(unit, dca);
			break;
		}
	}
	/*NOTREACHED*/
}

static void 
dcaeint(int unit, int stat, struct dcadevice *dca)
{
	register struct tty *tp;
	register int c;

	tp = &dca_tty[unit];
	c = dca->dca_data;
	if ((tp->t_state & TS_ISOPEN) == 0) {
#ifdef KGDB
		/* we don't care about parity errors */
		if (((stat & (LSR_BI|LSR_FE|LSR_PE)) == LSR_PE) &&
		    kgdb_dev == makedev(1, unit) && (c == '!' || c == '\003')) {
			printf("kgdb trap from dca%d\n", unit);
			/* trap into kgdb */
			kgdb_break();
		}
#endif
#if	MACH_KDB
		if (stat & LSR_BI)
			kdb_kintr();
#endif
		return;
	}
	if (stat & (LSR_BI | LSR_FE)) {
		c = tp->t_breakc;
	}
	else if (stat & LSR_PE) {
		if (((tp->t_flags & (EVENP|ODDP)) == EVENP)
		    || ((tp->t_flags & (EVENP|ODDP)) == ODDP))
			return;
	}
	else if (stat & LSR_OE) {
		log(LOG_WARNING, "dca%d: silo overflow\n", unit);
	}
	ttyinput(c, tp);
}

static void
dcamint(int unit, struct dcadevice *dca)
{
	register struct tty *tp;
	register u_char stat;

	tp = &dca_tty[unit];
	stat = dca->dca_msr;
#ifdef DEBUG
	dcamintcount[stat & 0xf]++;
#endif
	if ((stat & MSR_DDCD) && (dcasoftCAR & (1 << unit)) == 0) {
		if(stat & MSR_DCD)
			(void) ttymodem(tp, 1);
		else if (ttymodem(tp, 0) == 0)
			dca->dca_mcr &= ~(MCR_DTR | MCR_RTS);
	} else if ((stat & MSR_DCTS) && (tp->t_state & TS_ISOPEN) &&
		   (tp->t_flags & CRTSCTS)) {
		/* the line is up and we want to do rts/cts flow control */
		if (stat & MSR_CTS) {
			tp->t_state &=~ TS_TTSTOP;
			ttstart(tp);
		} else
			tp->t_state |= TS_TTSTOP;
	}
}

boolean_t
dcaportdeath(dev_t dev, ipc_port_t port)
{
	return (tty_portdeath(&dca_tty[minor(dev)], port));
}

io_return_t
dcagetstat(dev_t dev, dev_flavor_t flavor, int *data, natural_t *count)
{
	register struct tty *tp;
	register int	unit = minor(dev);

	tp = &dca_tty[unit];

	switch (flavor) {
	    case TTY_MODEM:
		*data = TM_CAR;
		*count = 1;
		break;

	    default:
		return (tty_get_status(tp, flavor, data, count));
	}
	return (D_SUCCESS);
}

io_return_t
dcasetstat(dev_t dev, dev_flavor_t flavor, int *data, natural_t count)
{
	struct tty       *tp;
	int	         unit;
	struct dcadevice *dca;
	int              error = D_SUCCESS;

	unit = minor(dev);
	tp = &dca_tty[unit];
	dca = (struct dcadevice*)tp->t_addr;

	switch (flavor) {
	case TTY_MODEM:
		break;

	case TTY_SET_BREAK:
		dca->dca_cfcr |= CFCR_SBREAK;
		break;
		
	case TTY_CLEAR_BREAK:
		dca->dca_cfcr &= ~CFCR_SBREAK;
		break;

	default: 
		error = tty_set_status(tp, flavor, data, count);

		if (error == D_SUCCESS && unit != dcaconsole &&
		    (flavor == TTY_STATUS_NEW || flavor == TTY_STATUS_COMPAT))
			dcaparam(unit);
		
		break;
	}

	return error;
}

static void
dcaparam(int unit)
{
	struct tty       *tp;
	struct dcadevice *dca;
	int              mode = 0;
	spl_t            s;
 
	s = spltty();
	tp = &dca_tty[unit];
	dca = dca_addr[unit];
	dca->dca_fifo = 0;	/* clear FIFOs so we dont lose TXRDY intrs */
	dca->dca_ier = IER_ERXRDY | IER_ETXRDY | IER_ERLS | IER_EMSC;
	CHECKMCRINT(3, unit, dca);
	dca->dca_mcr |= MCR_IEN;
	if (tp->t_ispeed == 0) {
		(void) dcamctl(tp, 0, DMSET);	/* hang up line */
		splx(s);
		return;
	}
	dca->dca_cfcr |= CFCR_DLAB;
	dca->dca_data = dca_speeds[rate2speed(tp->t_ispeed)] & 0xFF;
	dca->dca_ier = dca_speeds[rate2speed(tp->t_ispeed)] >> 8;

	if (tp->t_flags & (RAW|LITOUT|PASS8))
		mode |= CFCR_8BITS;
	else {
		mode |= CFCR_7BITS | CFCR_PENAB;
		if ((tp->t_flags & EVENP) == 0)
			mode |= CFCR_PODD;
		else if ((tp->t_flags & ODDP) == 0)
			mode |= CFCR_PEVEN;
		else
			mode |= CFCR_PZERO;
	}

	if (tp->t_ispeed == 110)
		mode |= CFCR_STOPB;
	dca->dca_cfcr = mode;
	if (dca_hasfifo & (1 << unit))
		dca->dca_fifo = FIFO_ENABLE | dcafifot[unit];
	splx(s);
}
 
void
dcastart(struct tty *tp)
{
	register struct dcadevice *dca;
	int s, unit, c;
 
	unit = minor(tp->t_dev);
	dca = dca_addr[unit];
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		tt_write_wakeup(tp);
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
#ifdef CCA_PAGE
	if (!(tp->t_flags & (RAW|CBREAK|LITOUT)) &&
	    tp->t_pagemode && (firstc(&tp->t_outq) & 0177) == '\n') {
		ttpage(tp);
		if (tp->t_state & TS_TTSTOP)
			goto out;
	}
#endif /* CCA_PAGE */
	if (dca->dca_lsr & LSR_TXRDY) {
		c = getc(&tp->t_outq);
		tp->t_state |= TS_BUSY;
		dca->dca_data = c;
		if (dca_hasfifo & (1 << unit)) {
			for (c = 1; c < 16 && tp->t_outq.c_cc; ++c)
				dca->dca_data = getc(&tp->t_outq);
#ifdef DEBUG
			if (c > 16)
				fifoout[0]++;
			else
				fifoout[c]++;
#endif
		}
	}
	CHECKMCRINT(4, unit, dca);
out:
	CHECKMCRINT(5, unit, dca);
	splx(s);
}
 
/*
 * Stop output on a line.
 */
void
dcastop(struct tty *tp, int flag)
{
	register int s;

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}
 
static int modtosoft(unsigned char);
static unsigned char softtomod(int, unsigned char);

int
dcamctl(struct tty *tp, int bits, int how)
{
	int unit;
	struct dcadevice *dca;
	spl_t s;
	unsigned char mbits = 0;

	unit = minor(tp->t_dev);
	dca = dca_addr[unit];

	/*
	 * Always make sure MCR_IEN is set (unless setting to 0)
	 */
#ifdef KGDB
	if (how == DMSET && kgdb_dev == makedev(1, unit))
		bits |= MCR_IEN;
	else
#endif
	if (how == DMBIS || (how == DMSET && bits))
		mbits |= MCR_IEN;
	else if (how == DMBIC)
		mbits &= ~MCR_IEN;

	s = spltty();
	CHECKMCRINT(6, unit, dca);
	switch (how) {
	case DMSET:
		dca->dca_mcr = softtomod(bits, mbits);
		break;
		
	case DMBIS:
		dca->dca_mcr |= softtomod(bits, mbits);
		if(bits & TM_BRK) 
			dca->dca_cfcr |= CFCR_SBREAK;
		break;
		
	case DMBIC:
		dca->dca_mcr &= ~softtomod(bits, mbits);
		if(bits & TM_BRK) 
			dca->dca_cfcr &= ~CFCR_SBREAK;
		break;

	case DMGET:
		bits = modtosoft(dca->dca_msr);
		break;

	default:
		panic("bad serial bit control\n");
		break;
	}

	splx(s);
	return(bits);
}

static int modtosoft(unsigned char mbits)
{
	int sbits = 0;

	if(mbits & MSR_RI)		
		sbits |= TM_RNG;

	if(mbits & MSR_CTS)		
		sbits |= TM_CTS;

	if(mbits & MSR_DSR)		
		sbits |= TM_DSR;

	if(mbits & MSR_DCD)		
		sbits |= TM_CAR;

	return sbits;
}

static unsigned char softtomod(int sbits, unsigned char mbits)
{
	if(sbits & TM_DTR)
		mbits |= (MCR_DTR | MCR_RTS);

	if(sbits & TM_RTS)
		mbits |= MCR_RTS;

	return mbits;
}


/*
 * Following are all routines needed for DCA to act as console
 */

void
dcacnprobe(struct consdev *cp)
{
	int unit, maj;

	/* XXX: ick */
	unit = 0;
	{
		extern struct iodc_data cons_iodc;	/* defined in... */
		extern struct device_path cons_dp;	/* vm_machdep.c */
		extern int isgecko;
		struct modtab *m;

		/*
		 * If the console we booted on was an RS232 port, then
		 * keep it as our console (CN_REMOTE).  Otherwise the
		 * DCA is just another console possibility (CN_NORMAL).
		 * and we choose the first one found.
		 */
		if (cons_iodc.iodc_sv_model == SVMOD_FIO_RS232) {
			if (cons_dp.dp_mod == 5) {	/* XXX? */
				m = getmod(IODC_TP_FIO, SVMOD_FIO_RS232, 1);	
				if(m) {
					dca_addr[1] = (struct dcadevice*)m->m_hpa;
					unit = 1;
				}
			} else {
				m = getmod(IODC_TP_FIO, SVMOD_FIO_RS232, 0);	
				if(m) {
					dca_addr[0] = (struct dcadevice*)m->m_hpa;
					unit = 0;
				}
			}
			cp->cn_pri = CN_REMOTE;
		} else if (cons_iodc.iodc_sv_model == SVMOD_FIO_GRS232) {
			m = getmod(IODC_TP_FIO, SVMOD_FIO_GRS232, 0);	
			if(m) {
				dca_addr[0] = (struct dcadevice *)m->m_hpa;
					unit = 0;
				cp->cn_pri = CN_REMOTE;
			}
		} else {
			m = getmod(IODC_TP_FIO, SVMOD_FIO_RS232, 0);
			if (m == 0)
				m = getmod(IODC_TP_FIO, SVMOD_FIO_GRS232, 0);
			if (m) {
				unit = (m->m_fixed == 5) ? 1 : 0;
				dca_addr[unit] = (struct dcadevice *)m->m_hpa;
				cp->cn_pri = CN_NORMAL;
			} else
				cp->cn_pri = CN_DEAD;
		}
	}
	/* No major number of because of indirection table */
	maj = 0;
	/* initialize required fields */
	cp->cn_dev = makedev(maj, unit);
}

int 
dcacninit(struct consdev *cp)
{
	int unit;

	unit = minor(cp->cn_dev);
	dcainit(unit, dcadefaultrate);
	dcaconsole = unit;
	return(0);
}

static void
dcainit(int unit, int ubrate)
{
	register struct dcadevice *dca;
	int s;
	u_char stat;
	int rate = DCABRD(ubrate);

	dca = dca_addr[unit];
	dcafifot[unit] = dcadefaultfifotrig;
	dcadelay[unit] = 10000000 / ubrate;
	s = splhigh();
	dca->dca_cfcr = CFCR_DLAB;
	dca->dca_data = rate & 0xFF;
	dca->dca_ier = rate >> 8;
	dca->dca_cfcr = CFCR_8BITS;
	dca->dca_ier = IER_ERXRDY | IER_ETXRDY;
	dca->dca_mcr |= MCR_IEN;
	dca->dca_fifo = FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|dcafifot[unit];
	DELAY(100);
	stat = dca->dca_iir;
	splx(s);
}

int
dcacngetc(dev_t dev, int wait)
{
	register struct dcadevice *dca = dca_addr[minor(dev)];
	register u_char stat;
	int c, s;

	s = splhigh();
	while (((stat = dca->dca_lsr) & LSR_RXRDY) == 0)
		if (!wait) {
			splx(s);
			return(0);
		}
	c = dca->dca_data;
	stat = dca->dca_iir;
	CHECKMCRINT(7, minor(dev), dca);
	splx(s);
	return(c);
}

/*
 * Console kernel output character routine.
 */
void
dcacnputc(dev_t dev, int c)
{
	register struct dcadevice *dca;
	register int timo;
	register u_char stat;
	int unit = minor(dev);
	int s = splhigh();

#ifdef KGDB
	if (dev == kgdb_dev) {
		/*
		 * In case someone sets kgdb_dev after boot time.
		 */
		if (kgdb_debug_init == 0) {
			dcainit(unit, kgdb_rate);
			kgdb_debug_init = 1;
		}
	} else
#endif
	if (dcaconsole == -1) {
#ifdef DEBUG
		dcainit(unit, dcadefaultrate);
		panic("yow! cnputc called before dcaconsole set\n");
#endif
		return;
	}
	dca = dca_addr[unit];
	/* wait for any pending transmission to finish */
	timo = dcadelay[unit];
	while (((stat = dca->dca_lsr) & LSR_TXRDY) == 0 && --timo)
		DELAY(1);
	dca->dca_data = c;
	/* wait for this transmission to complete */
	timo = dcadelay[unit];
	while (((stat = dca->dca_lsr) & LSR_TXRDY) == 0 && --timo)
		DELAY(1);
	/*
	 * If the "normal" interface was busy transfering a character
	 * we must let our interrupt through to keep things moving.
	 * Otherwise, we clear the interrupt that we have caused.
	 */
	if ((dca_tty[unit].t_state & TS_BUSY) == 0)
		stat = dca->dca_iir;
	CHECKMCRINT(8, unit, dca);
	splx(s);
}
#endif
