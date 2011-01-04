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
 * Revision 2.8  91/06/25  20:53:44  rpd
 * 	Tweaks to make gcc happy.
 * 	[91/06/25            rpd]
 * 
 * Revision 2.7  91/06/19  11:47:35  rvb
 * 	mips->DECSTATION; vax->VAXSTATION
 * 	[91/06/12  14:01:35  rvb]
 * 
 * 		File moved here from mips/PMAX since it tries to be generic;
 * 		it is used on the PMAX and the Vax3100.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.6  91/05/14  17:21:18  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/05/13  06:03:50  af
 * 	Added dz_input(), factoring some code out of dz_rint().
 * 	Now one can fake input easily, which I use for the
 * 	terminal emulator to generate escape sequence when
 * 	the mouse is clicked [non-X11 mode].
 * 	[91/04/09            af]
 * 
 * Revision 2.4  91/02/05  17:40:41  mrt
 * 	Added author notices
 * 	[91/02/04  11:13:10  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:11:04  mrt]
 * 
 * Revision 2.3  90/12/05  23:31:05  af
 * 	Let's just say it works now.
 * 	[90/12/03  23:16:51  af]
 * 
 * Revision 2.1.1.1  90/11/01  03:37:48  af
 * 	Created.
 * 	[90/09/03            af]
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
 *	File: dz_tty.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	9/90
 *
 *	DZ Serial Line Driver
 */

#include <dz_.h>
#if	NDZ_ > 0
#include <bm.h>
#include <platforms.h>

#include <mach_kdb.h>

#include <machine/machparam.h>
#include <chips/dz_defs.h>
#include <chips/screen_defs.h>

#ifdef	DECSTATION
#include <mips/prom_interface.h>
#endif	/*DECSTATION*/

static struct tty 	dz_tty_data[NDZ_*NDZ_LINE];
struct tty		*dz_tty[NDZ_*NDZ_LINE];	/* quick access */

#define	DEFAULT_SPEED	B9600
#define	DEFAULT_FLAGS	(TF_EVENP|TF_ODDP|TF_ECHO)

/*
 * A machine MUST have a console.  In our case
 * things are a little complicated by the graphic
 * display: people expect it to be their "console",
 * but we'd like to be able to live without it.
 * This is not to be confused with the "rconsole" thing:
 * that just duplicates the console I/O to
 * another place (for debugging/logging purposes).
 *
 * There is then another historical kludge: if
 * there is a graphic display it is assumed that
 * the minor "1" is the mouse, with some more
 * magic attached to it.  And again, one might like to
 * use the serial line 1 as a regular one.
 *
 * XXX This mess must be cleared out,
 * XXX (in a separate console.c file)
 * XXX but I haven't got much time left
 * XXX so I'll leave it at that
 */
#define	user_console	makedev(0,0)

dev_t	console = makedev(0,0);

/*
 * Kludge to get printf() going before autoconf.
 * XXX this belongs elsewhere XXX
 */
cons_enable(tube)
int tube;
{
	static struct bus_device d;
	register int		 i;
	struct tty		*tp;

	for (i = 0; i < NDZ_*NDZ_LINE; i++)
		dz_tty[i] = &dz_tty_data[i];

	d.unit = 0;
	dz_probe( 0, &d);

#ifdef	DECSTATION
	/*
	 * Remote console line
	 */
	{
		char *cp = prom_getenv("osconsole");
		if (cp && (string_to_int(cp) & 0x8))
			rcline = 3;
	}
#endif	/*DECSTATION*/
	/*
	 * Console always on unit 0. Fix if you need to
	 */
#if	NBM > 0
	if (tube && screen_probe(0)) {
		/* mouse and keybd */
		tp = dz_tty[DZ_LINE_KEYBOARD];
		tp->t_ispeed = B4800;
		tp->t_ospeed = B4800;
		tp->t_flags = TF_LITOUT|TF_EVENP|TF_ECHO|TF_XTABS|TF_CRMOD;
		dz_param(DZ_LINE_KEYBOARD);

		tp = dz_tty[DZ_LINE_MOUSE];
		tp->t_ispeed = B4800;
		tp->t_ospeed = B4800;
		tp->t_flags = TF_LITOUT|TF_ODDP;
		dz_param(DZ_LINE_MOUSE);
		/* dz_scan will turn on carrier */

		/* associate screen to console iff */
		if (console == user_console)
			screen_console = 0 | SCREEN_CONS_ENBL;
	} else {
#endif	/* NBM > 0 */
		/* use printer port as console */
		tp = dz_tty[DZ_LINE_PRINTER];
		tp->t_ispeed = B9600;
		tp->t_ospeed = B9600;
		tp->t_flags = TF_LITOUT|TF_EVENP|TF_ECHO|TF_XTABS|TF_CRMOD;
		dz_softCAR(0, DZ_LINE_PRINTER, 1);
		console = makedev(0,DZ_LINE_PRINTER);
		cnline = DZ_LINE_PRINTER;
		dz_param(DZ_LINE_PRINTER);
#if	NBM > 0
	}

	/*
	 * Enable rconsole interrupts for KDB
	 */
	if (tube && rcline != cnline) {
		tp = dz_tty[rcline];
		tp->t_ispeed = B9600;
		tp->t_ospeed = B9600;
		tp->t_flags = TF_LITOUT|TF_EVENP|TF_ECHO|TF_XTABS|TF_CRMOD;
		dz_softCAR(0, rcline, 1);
		dz_param(rcline);
	} else
	 	rcline = 0;
#endif	/* NBM > 0 */
	/* enough hacks, yeech */
}

/*
 *
 */
int	dz_start(), dz_stop();	/* forward */

dz_open(dev, flag, ior)
	dev_t	dev;
	int	flag;
	io_req_t ior;
{
	register struct tty *tp;
	register int unit;
 
	if (dev == user_console)
		dev = console;

	unit = minor(dev);
	if (unit >= NDZ_LINE*NDZ_)
		return (ENXIO);
	tp = dz_tty[unit];
	tp->t_addr = (caddr_t)&dz_pdma[unit/NDZ_LINE];
	tp->t_oproc = dz_start;
	tp->t_stop = dz_stop;

#if	NBM > 0
	if (SCREEN_ISA_CONSOLE() && ((unit & 0xfe) == 0))
		screen_open(SCREEN_CONS_UNIT(), unit==0);
#endif	/* NBM > 0 */

	if ((tp->t_state & TS_ISOPEN) == 0) {
		if (tp->t_ispeed == 0) {
			tp->t_ispeed = DEFAULT_SPEED;
			tp->t_ospeed = DEFAULT_SPEED;
			tp->t_flags = DEFAULT_FLAGS;
		}
		dz_param(unit);
	}
	(void) dz_mctl(dev, TM_DTR, DMSET);

	return (char_open(dev, tp, flag, ior));
}

dz_close(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
	spl_t		s;
 
	if (dev == user_console)
		dev = console;

	unit = minor(dev);

#if	NBM > 0
	if (SCREEN_ISA_CONSOLE() && ((unit & 0xfe) == 0))
		screen_close(SCREEN_CONS_UNIT(), unit==0);
#endif	/* NBM > 0 */

	tp = dz_tty[unit];

	s = spltty();
	simple_lock(&tp->t_lock);

	(void) dz_mctl(dev, TM_BRK, DMBIC);

	if ((tp->t_state&(TS_HUPCLS|TS_WOPEN)) || (tp->t_state&TS_ISOPEN)==0)
		(void) dz_mctl(dev, TM_HUP, DMSET);
	ttyclose(tp);

	simple_unlock(&tp->t_lock);
	splx(s);
}
 
dz_read(dev, ior)
	dev_t			dev;
	register io_req_t	ior;
{
	register struct tty *tp;
	register unit;
 
	if (dev == user_console)
		dev = console;

	unit = minor(dev);
#if	NBM > 0
	if (SCREEN_ISA_CONSOLE() && (unit == 1))	/* mouse */
		return screen_read(SCREEN_CONS_UNIT(), ior);
#endif	/* NBM > 0 */

	tp = dz_tty[unit];
	return char_read(tp, ior);
}
 
dz_write(dev, ior)
	dev_t			dev;
	register io_req_t	ior;
{
	register struct tty	*tp;
	register		unit; 

	if (dev == user_console)
		dev = console;

	unit = minor(dev);
#if	NBM > 0
	if (SCREEN_ISA_CONSOLE() && ((unit&0xfe) == 0))
		return screen_write(SCREEN_CONS_UNIT(), ior);
#endif	/* NBM > 0 */

	tp = dz_tty[unit];
	return char_write(tp, ior);
}

#if	MACH_KDB
int	l3break = 0x10;		/* dear old ^P, we miss you so bad. */
#endif	/* MACH_KDB */

dz_rint(unit, dz, c)
	int		dz;
	int		c;
{
	register struct tty *tp;
 
	/*
	 * Rconsole. Drop in the debugger on break or ^P.
	 * Otherwise pretend input came from keyboard.
	 */
	if (rcline && dz == rcline) {
#if	MACH_KDB
		if ((c & DZ_RBUF_FERR) ||
		    ((c & 0x7f) == l3break))
			return gimmeabreak();
#endif	/* MACH_KDB */
		dz = minor(console);
		goto process_it;	/* cleaner, but not strictly needed */
	}

#if	NBM > 0
	if (SCREEN_ISA_CONSOLE()) {
		if (dz == DZ_LINE_MOUSE)
			return mouse_input(SCREEN_CONS_UNIT(), c);
		if (dz == DZ_LINE_KEYBOARD) {
			c = lk201_rint(SCREEN_CONS_UNIT(), c, FALSE, FALSE);
			if (c == -1)
				return; /* shift or bad char */
		}
	}
#endif	/* NBM > 0 */
process_it:
	dz_input(unit, dz, c);
}

dz_input(unit, dz, c)
{
	register struct tty *tp;

	tp  = dz_tty[unit*NDZ_LINE + dz];

	if ((tp->t_state & TS_ISOPEN) == 0) {
		tt_open_wakeup(tp);
		return;
	}
	if (c&DZ_RBUF_OERR)
		log(LOG_WARNING, "dz%d,%d: silo overflow\n", unit, dz);

	if (c&DZ_RBUF_PERR)	
		if (((tp->t_flags & (TF_EVENP|TF_ODDP)) == TF_EVENP)
		  || ((tp->t_flags & (TF_EVENP|TF_ODDP)) == TF_ODDP))
			return;
	if (c&DZ_RBUF_FERR)	/* XXX autobaud XXX */
		c = tp->t_breakc;

	ttyinput(c & DZ_RBUF_CHAR, tp);
}
 
dz_tint(unit, line)
{
	register struct tty *tp;
	register struct pseudo_dma *dp;
	register dz;
 
	tp = dz_tty[unit*NDZ_LINE + line];
	dp = (struct pseudo_dma *)tp->t_addr;
	if (dp == 0)	/* not opened --> stray */
		return -1;
	tp->t_state &= ~TS_BUSY;
	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
#if 0
	else {
		ndflush(&tp->t_outq, dp->p_mem-tp->t_outq.c_cf);
		dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	}
#endif

	dz_start(tp);
	if (tp->t_outq.c_cc == 0 || (tp->t_state&TS_BUSY)==0)
		return -1;

	return getc(&tp->t_outq);
}

dz_start(tp)
	register struct tty *tp;
{
	register struct pseudo_dma *dp;
	register dz_regmap_t *regs;
	register int cc;
	int s, dz, unit;
 
	dp = (struct pseudo_dma *)tp->t_addr;
	regs = dp->p_addr;
	dz = minor(tp->t_dev) & 3;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;

	if (tp->t_outq.c_cc == 0)
		goto out;

	tp->t_state |= TS_BUSY;

#if 0
	cc = ndqb(&tp->t_outq, 0);
	dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	dp->p_end += cc;
#endif
	regs->dz_tcr |= (1<<dz);
out:
	splx(s);
}
 
/*
 * Stop output on a line.
 */
dz_stop(tp, flag)
	register struct tty *tp;
{
	register struct pseudo_dma *dp;
	register int s;

	dp = (struct pseudo_dma *)tp->t_addr;
	s = spltty();
	if (tp->t_state & TS_BUSY) {
#if 0
		dp->p_end = dp->p_mem;
#endif
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}
 

int
dz_portdeath(dev, port)
	dev_t	dev;
	mach_port_t port;
{
	if (dev == user_console)
		dev = console;
	return (tty_portdeath(dz_tty[minor(dev)], port));
}

io_return_t
dz_get_status(dev, flavor, data, status_count)
	dev_t		dev;
	int		flavor;
	int *		data;		/* pointer to OUT array */
	unsigned int	*status_count;		/* out */
{
	register struct tty *tp;
	register int	unit;

	if (dev == user_console)
		dev = console;

	unit = minor(dev);

#if	NBM > 0
	if (SCREEN_ISA_CONSOLE() && unit <= 1 &&
	    (screen_get_status(SCREEN_CONS_UNIT(),
			flavor, data, status_count) == D_SUCCESS))
		return D_SUCCESS;
#endif	/* NBM > 0 */

	tp = dz_tty[unit];

	switch (flavor) {
	    case TTY_MODEM:
		*data = dz_mctl(dev, 0, DMGET);
		*status_count = 1;
		break;
	    default:
		return (tty_get_status(tp, flavor, data, status_count));
	}
	return (D_SUCCESS);
}

io_return_t
dz_set_status(dev, flavor, data, status_count)
	dev_t		dev;
	int		flavor;
	int *		data;
	unsigned int	status_count;
{
	register struct tty *tp;
	register int	unit;
	register dz_regmap_t *	regs;

	if (dev == user_console)
		dev = console;

	unit = minor(dev);

#if	NBM > 0
	if (SCREEN_ISA_CONSOLE() && unit <= 1 &&
	    (screen_set_status(SCREEN_CONS_UNIT(),
			flavor, data, status_count) == D_SUCCESS))
		return D_SUCCESS;
#endif	/* NBM > 0 */

	tp  = dz_tty[unit];

	switch (flavor) {
	    case TTY_MODEM:
		if (status_count < TTY_MODEM_COUNT)
		    return (D_INVALID_OPERATION);
		(void) dz_mctl(dev, *data, DMSET);
		break;

	    case TTY_SET_BREAK:
		(void) dz_mctl(dev, TM_BRK, DMBIS);
		break;

	    case TTY_CLEAR_BREAK:
		(void) dz_mctl(dev, TM_BRK, DMBIC);
		break;

	    case TTY_STATUS:
	    {
		register int error;
		error = tty_set_status(tp, flavor, data, status_count);
		if (error == 0)
		    dz_param(unit);
		return (error);
	    }
	    default:
		return (tty_set_status(tp, flavor, data, status_count));
	}
	return (D_SUCCESS);
}



#endif	/* NDZ_ > 0 */
