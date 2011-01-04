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
 * Revision 2.7.5.4  92/06/24  17:58:40  jeffreyh
 * 	Updated to new at386_io_lock() interface.
 * 	Fixed locking bugs, spl() must be called before
 * 	simple_lock on tty lock.
 * 	Must bind to master before calling char_write() (Corollary)
 * 	[92/06/03            bernadat]
 * 
 * Revision 2.7.5.3  92/04/30  11:54:50  bernadat
 * 	Fix call to debugger, use kdb_kintr().
 * 	Added missing unlock/lock on tp->t_lock in comwaitforempty().
 * 	Set COM0 to 7bits/even at boot time to allow COM0 as console
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.7.5.2  92/03/28  10:05:50  jeffreyh
 * 	Wait for chars buffered in the serial chip to be sent before
 * 	closing.
 * 	[92/03/27            emcmanus]
 * 	04-Mar-92  emcmanus at gr.osf.org
 * 		Drain tty output before closing.  (Perhaps this should be up to the
 * 		user process using the new TTY_DRAIN request, instead.)
 * 		Also drain before starting a break, including any characters in the
 * 		serial chip's buffer.
 * 		Status interrupts modified so breaks and bad parity can be reported
 * 		to the user via the new out-of-band mechanism.
 * 		Call comintr() after initialising device registers in comparam();
 * 		otherwise interrupts never get going on some machines.
 * 	[92/03/10  07:56:25  bernadat]
 * 	Changes from TRUNK
 * 	[92/03/10  14:12:10  jeffreyh]
 * 
 * Revision 2.9  92/02/19  15:08:12  elf
 * 	Made comprobe more selective. Try to recognize chip.
 * 	[92/01/20            kivinen]
 * 
 * Revision 2.8  92/01/03  20:10:27  dbg
 * 	Don't drop software CARR_ON if carrier drops - modem drops
 * 	carrier but still can talk to machine.
 * 	[91/10/30            dbg]
 * 
 * Revision 2.7.5.1  92/02/18  18:42:50  jeffreyh
 * 	Picked up changes from latest. Below is the 2.8 log message from dbg:
 * 	Don't drop software CARR_ON if carrier drops - modem drops
 * 	carrier but still can talk to machine.
 * 	[92/02/18            jeffreyh]
 * 
 * Revision 2.7.4.1  92/02/13  18:44:50  jeffreyh
 * 	Added missing locks in comintr()
 * 	[91/09/09            bernadat]
 * 
 * 	Support for Corollary MP, switch to master for i386 ports R/W
 * 	Allow COM1 to be the console if no VGA (Z1000)
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.7  91/10/07  17:25:14  af
 * 	Add some improvements from 2.5 version.
 * 	[91/09/04  22:05:25  rvb]
 * 
 * Revision 2.6  91/08/24  11:57:21  af
 * 	New MI autoconf.
 * 	[91/08/02  02:50:03  af]
 * 
 * Revision 2.5  91/05/14  16:21:14  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/14  14:42:06  mrt
 * 	Merge of dbg's latest working com.c onto the old com.c
 * 	with the new autoconf and other major changes.
 * 	[91/01/28  15:26:13  rvb]
 * 
 * Revision 2.3  91/02/05  17:16:33  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:42:21  mrt]
 * 
 * Revision 2.2  90/11/26  14:49:26  rvb
 * 	jsb bet me to XMK34, sigh ...
 * 	[90/11/26            rvb]
 * 	Apparently first version is r2.2
 * 	[90/11/25  10:44:41  rvb]
 * 
 * 	Synched 2.5 & 3.0 at I386q (r2.3.1.6) & XMK35 (r2.2)
 * 	[90/11/15            rvb]
 * 
 * Revision 2.3.1.5  90/08/25  15:43:06  rvb
 * 	I believe that nothing of the early Olivetti code remains.
 * 	Copyright gone.
 * 	[90/08/21            rvb]
 * 
 * 	Use take_<>_irq() vs direct manipulations of ivect and friends.
 * 	[90/08/20            rvb]
 * 
 * 		Moved and rewrote much of the code to improve performance.
 * 	     Still suffers from overruns.
 * 	[90/08/14            mg32]
 * 
 * Revision 2.3.1.4  90/07/10  11:43:07  rvb
 * 	Rewrote several functions to look more like vax-BSD dh.c.
 * 	[90/06/25            mg32]
 * 
 * 	New style probe/attach.
 * 	Also com_struct has been radically reworked, ...
 * 	[90/06/15            rvb]
 * 
 * Revision 2.3.1.3  90/02/28  15:49:12  rvb
 * 	Fix numerous typo's in Olivetti disclaimer.
 * 	[90/02/28            rvb]
 * 
 * Revision 2.3.1.2  90/01/08  13:32:00  rvb
 * 	Add Olivetti copyright.
 * 	[90/01/08            rvb]
 * 
 * Revision 2.3.1.1  89/12/21  18:01:29  rvb
 * 	Changes from Ali Ezzet.
 * 
 * Revision 2.2.0.0  89/07/17  10:39:30  rvb
 * 	New from Olivetti.
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

#include <com.h>
#if NCOM > 0

#include <cpus.h>
#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <platforms.h>
#include <mp_v1_1.h>
#include <types.h>
#include <sys/time.h>
#include <device/conf.h>
#include <device/errno.h>
#include <device/buf.h>
#include <device/tty.h>
#include <kern/spl.h>
#include <i386/ipl.h>
#include <i386/pio.h>
#include <i386/misc_protos.h>
#include <chips/busses.h>
#include <i386/AT386/comreg.h>
#include <i386/AT386/kd.h>
#include <i386/AT386/kdsoft.h>
#include <i386/AT386/com_entries.h>
#include <i386/AT386/misc_protos.h>

#include <i386/AT386/mp/mp.h>
#ifdef	CBUS
#include <busses/cbus/cbus.h>
#elif MP_V1_1
#include <i386/AT386/mp/mp_v1_1.h>
#else
#define at386_io_lock_state()
#define at386_io_lock(op)	(TRUE)
#define at386_io_unlock()
#endif

#include <kern/misc_protos.h>
#include <kern/time_out.h>

/* Forward */

extern int		comprobe(
				caddr_t			port,
				struct bus_device	* dev);
extern void		comattach(
				struct bus_device	* dev);
extern void		comstart(
					struct tty	* tp);
extern io_return_t	comparam(
					int		unit);
extern void		comstop(
					struct tty	* tp,
					int		flags);
extern void		comtimer(void *);
extern void		comwaitforempty(
					struct tty	* tp);
extern int		commctl(
					struct tty	* tp,
					int bits,
					int how);
extern void		commodem_intr(
        				struct tty	* tp,
        				int		stat);
extern void		fix_modem_state(
					int		unit,
					int		modem_stat);
extern void		com_cons_init(
				   	void);
#if	MACH_KDB
extern int		compr(
			      		int		unit);

extern void		compr_addr(
				   	int		addr);
#endif	/* MACH_KDB */

struct bus_device *cominfo[NCOM];		/* ??? */

static caddr_t 	com_std[NCOM] = { 0 };
static struct 	bus_device *com_info[NCOM];

struct bus_driver comdriver = {
		(probe_t)comprobe,
		0,
		comattach,
		0,
		com_std,
		"com",
		com_info,
		0,
		0,
		0};

struct 		tty com_tty[NCOM];
int 		commodem[NCOM];
int		comcarrier[NCOM] = {1, 1};   	/* 0=ignore carrier,
						   1/2=pay attention */
boolean_t 	comfifo[NCOM];
boolean_t 	comtimer_active;
int 		comtimer_state[NCOM];

int 		cons_is_com1 = 0;		/* set to 1 if no PC keyboard
						   in this case com1 is the
						   console */
char 		com_halt_char = '_' & 0x1f; 	/* CTRL(_) to enter ddb */

#ifndef	PORTSELECTOR
#define ISPEED	9600
#define IFLAGS	(EVENP|ODDP|ECHO|CRMOD)
#else
#define ISPEED	4800
#define IFLAGS	(EVENP|ODDP)
#endif

#define CONSOLE_FLAGS_MASK (EVENP|ODDP|RAW|LITOUT|PASS8)

int	cons_ispeed = ISPEED;
int	cons_iflags = IFLAGS;

static struct baud_rate_info com_speeds[] = {
	0,	0,
	50,	0x900,
	75,	0x600,
	110,	0x417,
	134,	0x359,
	150,	0x300,
	300,	0x180,
	600,	0x0c0,
	1200,	0x060,
	1800,	0x040,
	2400,	0x030,
	4800,	0x018,
	9600,	0x00c,
	14400,	0x009,
	19200,	0x006,
	38400,	0x003,
	57600,	0x002,
	115200,	0x001,
	-1,	-1
};

int
comprobe(
	caddr_t			port,
	struct bus_device	*dev)
{
	i386_ioport_t	addr = (long)dev->address;
	int	unit = dev->unit;
	int     oldctl, oldmsb;
	char    *type = "8250";
	int     i;

	if ((unit < 0) || (unit >= NCOM)) {
		printf("com %d out of range\n", unit);
		return(0);
	}
	if (unit == 0 && cons_is_com1) {
		com_cons_init();
		return(1);
	}
	oldctl = inb(LINE_CTL(addr));	 /* Save old value of LINE_CTL */
	oldmsb = inb(BAUD_MSB(addr));	 /* Save old value of BAUD_MSB */
	outb(LINE_CTL(addr), 0);	 /* Select INTR_ENAB */    
	outb(BAUD_MSB(addr), 0);
	if (inb(BAUD_MSB(addr)) != 0)
	  {
	    outb(LINE_CTL(addr), oldctl);
	    outb(BAUD_MSB(addr), oldmsb);
	    return 0;
	  }
	outb(LINE_CTL(addr), iDLAB);	 /* Select BAUD_MSB */
	outb(BAUD_MSB(addr), 255);
	if (inb(BAUD_MSB(addr)) != 255)
	  {
	    outb(LINE_CTL(addr), oldctl);
	    outb(BAUD_MSB(addr), oldmsb);
	    return 0;
	  }
	outb(LINE_CTL(addr), 0);	 /* Select INTR_ENAB */
	if (inb(BAUD_MSB(addr)) != 0)	 /* Check that it has kept it's value*/
	  {
	    outb(LINE_CTL(addr), oldctl);
	    outb(BAUD_MSB(addr), oldmsb);
	    return 0;
	  }

	/* Com port found, now check what chip it has */
	
	for(i = 0; i < 256; i++)	 /* Is there Scratch register */
	  {
	    outb(addr + 7, i);
	    if (inb(addr + 7) != i)
	      break;
	  }
	if (i == 256)
	  {				 /* Yes == 450 or 460 */
	    outb(addr + 7, 0);
	    type = "82450 or 16450";
	    outb(INTR_ID(addr), 0xc1);	 /* Enable fifo */
	    if ((inb(INTR_ID(addr)) & 0xc0) != 0)
	      {				 /* Was it successfull */
		/* if both bits are not set then broken xx550 */
		if ((inb(INTR_ID(addr)) & 0xc0) == 0xc0)
		  {
		    type = "82550 or 16550";
		    comfifo[unit] = TRUE;
		    /* XXX even here the FIFO can be broken !? */
		  }
		else
		  {
		    type = "82550 or 16550 with non-working FIFO";
		  }
		outb(INTR_ID(addr), 0x00); /* Disable fifos */
	      }
	  }
	printf("com%d: %s chip.\n", dev->unit, type);
	return 1;
}

void
comattach(
	struct bus_device	*dev)
{
	u_char	unit = dev->unit;
	i386_ioport_t	addr = (long)dev->address;

	take_dev_irq(dev);
	printf(", port = %x, spl = %d, pic = %d. (DOS COM%d)",
		dev->address, dev->sysdep, dev->sysdep1, unit+1);

	cominfo[unit] = dev;
/*	comcarrier[unit] = addr->flags;*/
	commodem[unit] = 0;

	ttychars(&com_tty[unit]);

	if (unit == 0 && cons_is_com1)
		return;
	
	outb(INTR_ENAB(addr), 0);
	outb(MODEM_CTL(addr), 0);
	while (!(inb(INTR_ID(addr))&1)) {
		inb(LINE_STAT(addr)); 
		inb(TXRX(addr)); 
		inb(MODEM_STAT(addr)); 
	}
}

io_return_t
comopen(
	dev_t		dev,
	dev_mode_t	flag,
	io_req_t	ior)
{
	int		unit = minor(dev);
	int		forcedcarrier = 0;
	struct bus_device	*isai;
	struct tty	*tp;
	spl_t		s;
	io_return_t	result;
	at386_io_lock_state();

	if (unit >= NCOM || (isai = cominfo[unit]) == 0 || isai->alive == 0)
		return(D_NO_SUCH_DEVICE);
	tp = &com_tty[unit];
	at386_io_lock(MP_DEV_WAIT);
	s = spltty();

	simple_lock(&tp->t_lock);
	if ((tp->t_state & (TS_ISOPEN|TS_WOPEN)) == 0) {
		tp->t_addr = isai->address;
		tp->t_dev = dev;
		tp->t_oproc = comstart;
		tp->t_stop = comstop;
		tp->t_mctl = commctl;
		tp->t_getstat = comgetstat;
		tp->t_setstat = comsetstat;
#ifndef	PORTSELECTOR
		if (tp->t_ispeed == 0) {
#else
			tp->t_state |= TS_HUPCLS;
#endif	/* PORTSELECTOR */
			if (unit == 0) {
				tp->t_ispeed = cons_ispeed;
				tp->t_ospeed = cons_ispeed;
				tp->t_flags = cons_iflags;
			} else {
				tp->t_ispeed = ISPEED;
				tp->t_ospeed = ISPEED;
				tp->t_flags = IFLAGS;
			}
			tp->t_state &= ~TS_BUSY;
#ifndef	PORTSELECTOR
		}
#endif	/* PORTSELECTOR */
	}
/*rvb	tp->t_state |= TS_WOPEN; */
	if ((tp->t_state & TS_ISOPEN) == 0)
		(void) comparam(unit);
	if ((flag & D_NODELAY) ||
	    (comcarrier[unit] == 0) ||
	    (unit == 0 && cons_is_com1)) /* ignoring carrier */
		tp->t_state |= TS_CARR_ON;
	else {
		int modem_stat = inb(MODEM_STAT((long)tp->t_addr));
		if (modem_stat & iRLSD)
			tp->t_state |= TS_CARR_ON;
		else
			tp->t_state &= ~TS_CARR_ON;
		fix_modem_state(unit, modem_stat);
	} 

	/*
         * If no carrier and not NODELAY the server can't
         * handle it so for now force carrier on open.  The server
         * will do a read to block.
         */
        if (!(tp->t_state & TS_CARR_ON) && !(flag & D_NODELAY)) {
                tp->t_state |= TS_CARR_ON;
                forcedcarrier = 1;
        }


	simple_unlock(&tp->t_lock);
	splx(s);
	at386_io_unlock();
	result = char_open(dev, tp, flag, ior);
	if (forcedcarrier)
		tp->t_state &= ~TS_CARR_ON;  /* turn it back off */

	if (tp->t_flags & CRTSCTS) {
		simple_lock(&tp->t_lock);
		if (!(commodem[unit] & TM_CTS))
                        tp->t_state |= TS_TTSTOP;
		simple_unlock(&tp->t_lock);
	}

	if (!comtimer_active) {
		comtimer_active = TRUE;
		comtimer(0);
	}

	return (result);
}

void
comclose(
	dev_t		dev)
{
	struct tty	*tp = &com_tty[minor(dev)];
	i386_ioport_t	addr = (long)tp->t_addr;
	spl_t           s;
	at386_io_lock_state();

	at386_io_lock(MP_DEV_WAIT);

	s = spltty();
	simple_lock(&tp->t_lock);
	if (tp->t_state & TS_ONDELAY)
		tp->t_state &= ~TS_TTSTOP;
	ttstart(tp);
	ttydrain(tp);
	comwaitforempty(tp);
	ttyclose(tp);
	if (tp->t_state&TS_HUPCLS) { 
		outb(INTR_ENAB(addr), 0);
		outb(MODEM_CTL(addr), 0);
		tp->t_state &= ~(TS_BUSY|TS_HUPCLS);
		commodem[minor(dev)] = 0;
		if (comfifo[minor(dev)])
			outb(INTR_ID(addr), 0x00); /* Disable fifos */
	}
	if (comcarrier[minor(dev)] == 2)  /* turn off temporary true carrier */
		comcarrier[minor(dev)] = 0;
	simple_unlock(&tp->t_lock);
	splx(s);
	if (minor(dev) == 0 && cons_is_com1) {
		comreset();
		comintr(0);
	}
	at386_io_unlock();
}

io_return_t
comread(
	dev_t		dev,
	io_req_t	ior)
{
	return char_read(&com_tty[minor(dev)], ior);
}

io_return_t
comwrite(
	dev_t		dev,
	io_req_t	ior)
{
	io_return_t ret;
	at386_io_lock_state();

	at386_io_lock(MP_DEV_WAIT);
	ret = char_write(&com_tty[minor(dev)], ior);
	at386_io_unlock();
	return(ret);
}

boolean_t
comportdeath(
	dev_t		dev,
	ipc_port_t	port)
{
	return (tty_portdeath(&com_tty[minor(dev)], port));
}

io_return_t
comgetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	int		*data,		/* pointer to OUT array */
	natural_t	*count)		/* out */
{
	io_return_t	result = D_SUCCESS;
	int		unit = minor(dev);
	at386_io_lock_state();

	switch (flavor) {
	case TTY_MODEM:
		if (cons_is_com1 && unit == 0)
			return D_INVALID_OPERATION;
		at386_io_lock(MP_DEV_WAIT);
		fix_modem_state(unit,
		    inb(MODEM_STAT((long)cominfo[unit]->address)));
		at386_io_unlock();
		*data = commodem[unit];
		*count = 1;
		break;
	default:
		result = tty_get_status(&com_tty[unit], flavor, data, count);
		break;
	}
	return (result);
}

io_return_t
comsetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	int *		data,
	natural_t	count)
{
	io_return_t	result;
	io_return_t	interm_res = D_SUCCESS;
	int 		unit = minor(dev);
	struct tty *	tp = &com_tty[unit];
	spl_t		s;
	at386_io_lock_state();

	switch (flavor) {
	case TTY_SET_BREAK:
	case TTY_CLEAR_BREAK:
	        at386_io_lock(MP_DEV_WAIT);
		s = spltty();
    		simple_lock(&tp->t_lock);
		commctl(tp, TM_BRK, flavor == TTY_SET_BREAK ? DMBIS : DMBIC);
		simple_unlock(&tp->t_lock);
		splx(s);
		at386_io_unlock();
		break;
	case TTY_MODEM:
		if (cons_is_com1 && unit == 0)
			return D_INVALID_OPERATION;
	        at386_io_lock(MP_DEV_WAIT);
		s = spltty();
    		simple_lock(&tp->t_lock);
		commctl(tp, *data, DMSET);
		simple_unlock(&tp->t_lock);
		splx(s);
		at386_io_unlock();
		break;
	case TTY_NMODEM:
		s = spltty();
    		simple_lock(&tp->t_lock);
		switch (*data) {
			case 0:   /* turn on soft carrier */
			case 1:   /* turn off soft carrier permanently */
			case 2:   /* turn off soft carrier temporarily */
				comcarrier[unit] = *data;
				break;
			default:  /* ignore */
				break;
		}
		simple_unlock(&tp->t_lock);
		splx(s);
		break;
	case KDSKBENT:
        case KDSETBELL:
		if (cons_is_com1) {
			at386_io_lock_state();
			at386_io_lock(MP_DEV_WAIT);
			result = kdsetstat(dev, flavor, data, count);
			at386_io_unlock();
			return result;
		}
		break;
	default: 
#if	!MP_V1_1
		at386_io_lock(MP_DEV_WAIT);
#endif	/* !MP_V1_1 */
		result = tty_set_status(tp, flavor, data, count);
		s = spltty();
		simple_lock(&tp->t_lock);
		if (result == D_SUCCESS &&
		    (flavor == TTY_STATUS_NEW || flavor == TTY_STATUS_COMPAT)) {
			result = comparam(unit);
			if (tp->t_flags & CRTSCTS) {
				if (commodem[unit] & TM_CTS) {
					tp->t_state &= ~TS_TTSTOP;
					ttstart(tp);
				} else
					tp->t_state |= TS_TTSTOP;
			}
		}
		simple_unlock(&tp->t_lock);
		splx(s);
#if	!MP_V1_1
		at386_io_unlock();
#endif	/* !MP_V1_1 */
		return result;
	}
	return D_SUCCESS;
}

/* Wait for transmit buffer to empty.  Interrupts only tell us that
   the serial device can accept another character; there may still
   be buffered characters to be sent. */

void
comwaitforempty(
	struct tty *	tp)
{
	at386_io_lock_state();

	while (!(inb(LINE_STAT((long)tp->t_addr)) & iTSRE)) {
		assert_wait(0, TRUE);
		thread_set_timeout(1);
		simple_unlock(&tp->t_lock);
		/* if we are holding the io_lock release it before blocking */
		at386_io_unlock();
		thread_block((void (*)(void)) 0);
		reset_timeout_check(&current_thread()->timer);
		/* If we held the io_lock on entry, grab it back */
		at386_io_lock(MP_DEV_WAIT);
		simple_lock(&tp->t_lock);
	}
}

#if	MACH_KDB
#define check_debugger(c)					\
	if (c == com_halt_char && !unit && cons_is_com1) { 	\
		extern int rebootflag;				\
		/* if rebootflag is on, reboot the system */	\
		if (rebootflag)					\
			kdreboot();				\
		kdb_kintr();					\
		break;						\
	}
#elif	MACH_KGDB
#define check_debugger(c)					\
	if (c == com_halt_char && !unit && cons_is_com1) {	\
		extern int rebootflag;				\
		/* if rebootflag is on, reboot the system */	\
		if (rebootflag)					\
			kdreboot();				\
		kgdb_kintr();					\
		break;						\
	}
#else	/* !MACH_KDB */
#define check_debugger(c) {						\
	extern int rebootflag;						\
	if (c == com_halt_char && !unit && cons_is_com1 && rebootflag)	\
		kdreboot();						\
	}
#endif	/* !MACH_KDB */

void
comintr(
	int	unit)
{
	register struct tty	*tp = &com_tty[unit];
	i386_ioport_t		addr = (long)cominfo[unit]->address;
	char			line, intr_id;
	int			c;

	while (!((intr_id=inb(INTR_ID(addr))) & 1))
	    /* On each interrupt, poll all states at or below the
	       current level. Thus, do not change the order of the
	       cases in the following switch statement! */
	    switch (intr_id) { 
		default:
		case CTIi:	/* character timeout indication */
		case LINi: 
		case RECi:
		    for (;;) {
			line = inb(LINE_STAT(addr));
			c = inb(TXRX(addr));
			if (line & iOR) {
				static int comoverrun[NCOM];
				if (!comoverrun[unit]++)
					printf("com%d: overrun\n", unit);
			}
			if (line & (iDR | iFE | iBRKINTR | iPE)) {
			    check_debugger(c);
			    simple_lock(&tp->t_lock);
			    if (tp->t_state & TS_ISOPEN) {
			        if (line & (iFE | iBRKINTR)) {
				    /* framing error or break */
				    ttybreak(c, tp);
			        } else if (line & iPE) {
				    /* bad parity: must have asked for this */
				    ttyinputbadparity(c, tp);
			        } else
				    ttyinput(c, tp);
			    }
			    simple_unlock(&tp->t_lock);
			} else
			    break;
		    }
		    /* fall through */
		case TRAi:
		    if (inb(LINE_STAT(addr)) & iTHRE) {
			comtimer_state[unit] = 0;
			simple_lock(&tp->t_lock);
			tp->t_state &= ~TS_BUSY;
			comstart(tp);
			simple_unlock(&tp->t_lock);
		    }
		    /* fall through */
		case MODi: 
		    line = inb(MODEM_STAT(addr));
		    if (!((unit == 0) && cons_is_com1) &&
			(line & (iDCTS|iDDSR|iTERI|iDRLSD))) {
			simple_lock(&tp->t_lock);
		    	commodem_intr(tp, line);
			simple_unlock(&tp->t_lock);
		    }
	    }
}

boolean_t	com_cons_alter = FALSE;	/* Can we change console state ?
					   (parity, char size, stop bits,
					   speed) */

io_return_t
comparam(
int		unit)
{
	struct tty	*tp = &com_tty[unit];
	i386_ioport_t	addr = (long)tp->t_addr;
	int		mode;
	int		ospeed;
	io_return_t	ret = D_SUCCESS;

	/* Check that we don't change console flags and speeds */

	if (unit == 0 && cons_is_com1 && !com_cons_alter) {
	  	/* Don't change console char size,
		   parity, stop bits and speed */

		if ((tp->t_flags & CONSOLE_FLAGS_MASK) !=
		    (cons_iflags & CONSOLE_FLAGS_MASK)) {
			ret = D_INVALID_OPERATION;
			tp->t_flags &= ~(CONSOLE_FLAGS_MASK);
			tp->t_flags |= (cons_iflags & CONSOLE_FLAGS_MASK);
		}

		/* Don't change console speed */

		if (tp->t_ispeed != cons_ispeed) {
			ret = D_INVALID_OPERATION;
			tp->t_ispeed = cons_ispeed;
			tp->t_ospeed = cons_ispeed;
		}
	}

	ospeed = baud_rate_get_info(tp->t_ospeed, com_speeds);
	if (ospeed < 0 ||
	    (ospeed && tp->t_ispeed && tp->t_ispeed != tp->t_ospeed)) {
		return D_INVALID_OPERATION;
	}
	if (ospeed > 0) {
		outb(LINE_CTL(addr), iDLAB);
		outb(BAUD_LSB(addr), ospeed & 0xff);
		outb(BAUD_MSB(addr), ospeed >> 8);
	}

	if (tp->t_flags & (RAW|LITOUT|PASS8))
		mode = i8BITS;
	else
		mode = i7BITS;
	switch (tp->t_flags & (EVENP|ODDP)) {
	case 0:
		break;
	case ODDP:
		mode |= iPEN;
		break;
	case EVENP|ODDP:
	case EVENP:
		mode |= iPEN|iEPS;
		break;
	}
	if (tp->t_ispeed == 110)
		/*
		 * 110 baud uses two stop bits -
		 * all other speeds use one
		 */
		mode |= iSTB;

	outb(LINE_CTL(addr), mode);
	outb(INTR_ENAB(addr), iTX_ENAB|iRX_ENAB|iMODEM_ENAB|iERROR_ENAB);
	if (tp->t_ospeed) {
		outb(MODEM_CTL(addr), iDTR|iRTS|iOUT2);
		commodem[unit] |= (TM_DTR|TM_RTS);
	} else {
		tp->t_state |= TS_HUPCLS;
		outb(MODEM_CTL(addr), iOUT2);
		commodem[unit] &= ~(TM_DTR|TM_RTS);
	}
	simple_unlock(&tp->t_lock);
	/* Poll chip for interrupts to reset PIC latch */
	comintr(unit);
	simple_lock(&tp->t_lock);
	return ret;
}

void
comstart(
	struct tty	*tp)
{
	char nch;
	long addr = (long)tp->t_addr;

	if ((tp->t_state & (TS_ISOPEN|TS_TIMEOUT|TS_TTSTOP|TS_BUSY))
	    != TS_ISOPEN) {
		return;
	}
	while (tp->t_outq.c_cc) {
		nch = getc(&tp->t_outq);
		if ((tp->t_flags & LITOUT) == 0 && (nch & 0200)) {
		    timeout((timeout_fcn_t)ttrstrt, tp, (nch & 0x7f) + 6);
		    tp->t_state |= TS_TIMEOUT;
		    break;
		}
		/* Fill the chip's output FIFO */
                outb(TXRX(addr), nch);
                if (!(inb(LINE_STAT(addr)) & iTHRE) &&
                    !(inb(LINE_STAT(addr)) & iTHRE)) {
			tp->t_state |= TS_BUSY;
			break;
		}
	}
	if (tp->t_outq.c_cc <= TTLOWAT(tp))
		tt_write_wakeup(tp);
	return;
}

/* Check for stuck xmitters */
int comtimer_interval = 5;

void comtimer(void *param)
{
    spl_t   s;
    struct tty *tp = com_tty;
    int i, nch;

    at386_io_lock_state();
    at386_io_lock(MP_DEV_WAIT);
    s = spltty();

    for (i = 0; i < NCOM; i++, tp++) {
        if ((tp->t_state & TS_ISOPEN) == 0)
            continue;
        if (!tp->t_outq.c_cc)
            continue;
        if (tp->t_state & TS_TTSTOP)
            continue;
        if (++comtimer_state[i] < 2)
            continue;
        /* Its stuck */
printf("Tty %#x was stuck\n", (vm_offset_t)tp);
	comintr(i);
    }

    splx(s);
    at386_io_unlock();
    timeout(comtimer, 0, comtimer_interval*hz);
}

/*
 * Set receive modem state from modem status register.
 */

void
fix_modem_state(
	int	unit,
	int	modem_stat)
{
	int	stat = 0;

	if (modem_stat & iCTS)
	    stat |= TM_CTS;	/* clear to send */
	if (modem_stat & iDSR)
	    stat |= TM_DSR;	/* data set ready */
	if (modem_stat & iRI)
	    stat |= TM_RNG;	/* ring indicator */
	if ((modem_stat & iRLSD) || (comcarrier[unit] == 0))
	    stat |= TM_CAR;	/* carrier? */

	commodem[unit] = (commodem[unit] & ~(TM_CTS|TM_DSR|TM_RNG|TM_CAR))
				| stat;
}

/*
 * Modem change (input signals)
 */
void commodem_intr(
        struct tty *tp,
        int     stat)
{
        int     changed, unit = minor(tp->t_dev);

        changed = commodem[unit];
        fix_modem_state(unit, stat);
        stat = commodem[unit];

        /* Assumption: if the other party can handle
           modem signals then it should handle all
           the necessary ones. Else fix the cable. */

        changed ^= stat;        /* what changed ? */

        if (changed & TM_CTS)
                tty_cts( tp, stat & TM_CTS );

	/* if carrier changed and we're paying attention */
        if ((changed & TM_CAR) && (comcarrier[unit] != 0))
                ttymodem( tp, stat & TM_CAR );
}

/*
 * Set/get modem bits
 */
int commctl(
    register struct tty *tp,
    int bits,
    int how)
{
    spl_t       s;
    int     unit;
    i386_ioport_t	dev_addr;
    register int    b;

    unit = minor(tp->t_dev);

    if (bits == TM_HUP) { /* close line (internal) */
        bits = TM_DTR | TM_RTS;
        how = DMBIC;
    }

    if (how == DMGET)
	return commodem[unit];

    dev_addr = (long)cominfo[unit]->address;

    switch (how) {
    case DMSET:
        b = bits; break;
    case DMBIS:
        b = commodem[unit] | bits; break;
    case DMBIC:
        b = commodem[unit] & ~bits; break;
    }

    if (commodem[unit] == b)
	return commodem[unit];


    commodem[unit] = b;

    if (bits & TM_BRK) {
        /* Certain commands logically require that we drain whatever
           output we already have.  This is certainly true for
           starting breaks, but we don't do it for TTY_MODEM in
           case the user is trying to do (obsolete) DTR flow control.
           As currently written, new characters that are written
           while we are draining will also be waited for.  This
           makes little sense, but it's not expected that the user
           will do that. */
    	ttydrain(tp);
    	comwaitforempty(tp);

        if (b & TM_BRK) {
            outb(LINE_CTL(dev_addr), inb(LINE_CTL(dev_addr)) | iSETBREAK);
        } else {
            outb(LINE_CTL(dev_addr), inb(LINE_CTL(dev_addr)) & ~iSETBREAK);
        }
    }

#if 0
    /* do I need to do something on this ? */
    if (bits & TM_LE) { /* line enable */
    }
#endif
#if 0
    /* Unsupported */
    if (bits & TM_ST) { /* secondary transmit */
    }
    if (bits & TM_SR) { /* secondary receive */
    }
#endif
    if (bits & (TM_DTR|TM_RTS)) {   /* data terminal ready, request to send */
        how = iOUT2;
        if (b & TM_DTR) how |= iDTR;
        if (b & TM_RTS) how |= iRTS;
        outb(MODEM_CTL(dev_addr), how);
    }

    /* the rest are inputs */
    return commodem[unit];
}

void
comstop(
	struct tty	*tp,
	int		flags)
{
	/* unneeded */
}

#if	MACH_KDB
int
compr(
      	int	unit)
{
        compr_addr((int)cominfo[unit]->address);
        return(0);
}

void
compr_addr(
	int	addr)
{
	printf("TXRX(%x) %x, INTR_ENAB(%x) %x, INTR_ID(%x) %x, LINE_CTL(%x) %x,\n\
MODEM_CTL(%x) %x, LINE_STAT(%x) %x, MODEM_STAT(%x) %x\n",
	TXRX(addr), inb(TXRX(addr)),
	INTR_ENAB(addr), inb(INTR_ENAB(addr)),
	INTR_ID(addr), inb(INTR_ID(addr)),
	LINE_CTL(addr), inb(LINE_CTL(addr)),
	MODEM_CTL(addr), inb(MODEM_CTL(addr)),
	LINE_STAT(addr), inb(LINE_STAT(addr)),
	MODEM_STAT(addr), inb(MODEM_STAT(addr)));
}
#endif	/* MACH_KDB */

/*
 * all the followings are used for printf, and must work before com
 * is attached ==> we can not use cominfo
 */

#define COM0_ADDR 0x3f8

boolean_t	com_cons_initialized = FALSE;

void
com_cons_init(void)
{
  	int ctl, msb, lsb, speed_code;
	struct baud_rate_info *bauds;

	if (com_cons_initialized)
		return;

	/* save initial settings */
	cons_iflags &= ~CONSOLE_FLAGS_MASK;
	ctl = inb(LINE_CTL(COM0_ADDR));
	if ((ctl&i8BITS) == i8BITS)
		cons_iflags |= RAW|LITOUT|PASS8;
	switch(ctl&(iPEN|iEPS)) {
	case iPEN|iEPS:
		cons_iflags |= EVENP;
		break;
	case iPEN:
		cons_iflags |= ODDP;
		break;
	}
	outb(LINE_CTL(COM0_ADDR), iDLAB);
	msb = inb(BAUD_MSB(COM0_ADDR));
	lsb = inb(BAUD_LSB(COM0_ADDR));
	outb(LINE_CTL(COM0_ADDR), ctl);
	speed_code = (msb << 8) | lsb;
	for (bauds = com_speeds; bauds->br_rate != -1; bauds++)
		if (bauds->br_info == speed_code) {
			cons_ispeed = bauds->br_rate;
			break;
		}
	com_cons_initialized = TRUE;
}

void
comreset(void)
{
	int speed = baud_rate_get_info(cons_ispeed, com_speeds);
	int mode;

	com_cons_init();

        inb(INTR_ID(COM0_ADDR));
        inb(LINE_STAT(COM0_ADDR));
        inb(MODEM_STAT(COM0_ADDR));
        inb(TXRX(COM0_ADDR));

	outb(LINE_CTL(COM0_ADDR), iDLAB);
	outb(BAUD_LSB(COM0_ADDR), speed & 0xff);
	outb(BAUD_MSB(COM0_ADDR), speed >> 8);
	if (cons_iflags & (RAW|LITOUT|PASS8))
		mode = i8BITS;
	else
		mode = i7BITS;
	switch (cons_iflags & (EVENP|ODDP)) {
	case 0:
		break;
	case ODDP:
		mode |= iPEN;
		break;
	case EVENP|ODDP:
	case EVENP:
		mode |= iPEN|iEPS;
		break;
	}
	outb(LINE_CTL(COM0_ADDR), mode);
	outb(INTR_ENAB(COM0_ADDR), iTX_ENAB|iRX_ENAB);
	outb(MODEM_CTL(COM0_ADDR), iDTR|iRTS|iOUT2);
}

void
com_putc(
	char		c)
{
	register i;

	outb(INTR_ENAB(COM0_ADDR), 0);
	for (i=0; (!(inb(LINE_STAT(COM0_ADDR)) & iTHRE)) && (i < 1000); i++);
	outb(TXRX(COM0_ADDR),  c);
	for (i=0; (!(inb(LINE_STAT(COM0_ADDR)) & iTHRE)) && (i < 1000); i++);
	outb(INTR_ENAB(COM0_ADDR), iTX_ENAB|iRX_ENAB);
}

int
com_getc(
	boolean_t	wait)
{
	unsigned char	c = 0;

	outb(INTR_ENAB(COM0_ADDR), 0);
	while (!(inb(LINE_STAT(COM0_ADDR)) & iDR)) {
		if (!wait) {
			c = (unsigned char) -1;
			break;
		}
	}
	if (!c)
		c = inb(TXRX(COM0_ADDR));
	outb(INTR_ENAB(COM0_ADDR), iTX_ENAB|iRX_ENAB);
	if (c == K_CR)
	  	c = K_LF;
	return(c);
}

boolean_t
com_is_char(void)
{
	boolean_t	rc;

	outb(INTR_ENAB(COM0_ADDR), 0);
	if (!(inb(LINE_STAT(COM0_ADDR)) & iDR))
		rc = FALSE;
	else
		rc = TRUE;
	outb(INTR_ENAB(COM0_ADDR), iTX_ENAB|iRX_ENAB);
	return(rc);
}

#if MACH_KDB
/* Output the characters that are buffered for com1.  This is intended
   to be called from kdb, via "!comdrain".  When you enter kdb, via the
   keyboard or a breakpoint, there may be useful information that a user
   program has written to the console but that has not yet been
   displayed.  */
void comdrain(void);	/* improper prototype warning avoidance */

void
comdrain(void)
{
	int		c;
	struct tty	*tp = &com_tty[0];

	while (1) {
		tt_write_wakeup(tp);
		if (!tp->t_outq.c_cc)
			break;
		while ((c = getc(&tp->t_outq)) >= 0)
			com_putc(c);
	}
}
#endif /* MACH_KDB */

#endif /* NCOM */
