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
 * Copyright (c) 1988-1994, The University of Utah and
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
 *	Utah $Hdr: ite.c 1.37 94/12/14$
 */

/*
 * Bit-mapped display terminal emulator machine independent code.
 * This is a very rudimentary.  Much more can be abstracted out of
 * the hardware dependent routines.
 */
#include "ite.h"
#include "gkd.h"

#if NITE > 0

#include "grf.h"

#undef NITE
#define NITE	NGRF

#include "hil.h"

#include <types.h>
#include <sys/ioctl.h>
#include <device/tty.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <kern/spl.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <hp_pa/HP700/grfioctl.h>
#include <hp_pa/HP700/grfvar.h>
#include <hp_pa/HP700/itevar.h>
#include <hp_pa/HP700/hilioctl.h>
#include <hp_pa/HP700/hilvar.h>
#include <hp_pa/HP700/kbdmap.h>

#include <hp_pa/HP700/device.h>

#if NGKD > 0
#include <hp_pa/HP700/gkdvar.h>
#endif

#include <machine/machparam.h>
#include <machine/iodc.h>
#include <machine/asp.h>

#define set_attr(ip, attr)	((ip)->attribute |= (attr))
#define clr_attr(ip, attr)	((ip)->attribute &= ~(attr))

#include <hp_pa/HP700/cons.h>
#include <hp_pa/misc_protos.h>
#include <kern/misc_protos.h>

/*
 * Forwards
 */
void itestart(struct tty *);
void iterestart(struct tty *);
void itestop(struct tty *, int);
void iteinit(dev_t);
void iteputchar(int, dev_t);
void itefilter(char, char);
void ite_clrtoeol(struct ite_softc *, struct itesw *, int, int);
void ite_clrtoeos(struct ite_softc *, struct itesw *);
void itecheckwrap(struct ite_softc *, struct itesw *);
void ite_dchar(struct ite_softc *, struct itesw *);
void ite_dline(struct ite_softc *, struct itesw *);
void ite_iline(struct ite_softc *, struct itesw *);
void ite_ichar(struct ite_softc *, struct itesw *);
io_return_t iteopen(dev_t, int, io_req_t);
void iteclose(dev_t);
int iteread(dev_t, io_req_t);
int itewrite(dev_t, io_req_t);
io_return_t itegetstat(dev_t, int, int *, unsigned int *);
io_return_t itesetstat(dev_t, int, int *, unsigned int);
boolean_t iteportdeath(dev_t, ipc_port_t);
void itecnprobe(struct consdev *cp);
void itecninit(struct consdev *cp);
int itecngetc(dev_t dev, boolean_t);
void itecnputc(dev_t dev, char c);

/*
 * # of chars are output in a single itestart() call.
 * If this is too big, user processes will be blocked out for
 * long periods of time while we are emptying the queue in itestart().
 * If it is too small, console output will be very ragged.
 */
int	iteburst = 64;

int	nite = NITE;
struct  tty *kbd_tty = NULL;
struct	tty ite_tty[NITE];
struct  ite_softc ite_softc[NITE];

extern	struct tty *constty;

/*
 * Primary attribute buffer to be used by the first bitmapped console
 * found. Secondary displays alloc the attribute buffer as needed.
 * Size is based on a 64x160 display, which is currently our largest.
 */
u_char  console_attributes[0x2800];

#define ite_erasecursor(ip, sp)	{ \
	if ((ip)->flags & ITE_CURSORON) \
		(*(sp)->ite_cursor)((ip), ERASE_CURSOR); \
}
#define ite_drawcursor(ip, sp) { \
	if ((ip)->flags & ITE_CURSORON) \
		(*(sp)->ite_cursor)((ip), DRAW_CURSOR); \
}
#define ite_movecursor(ip, sp) { \
	if ((ip)->flags & ITE_CURSORON) \
		(*(sp)->ite_cursor)((ip), MOVE_CURSOR); \
}

#define CTRL(c)		('c'&037)

/*
 * Perform functions necessary to setup device as a terminal emulator.
 */
int
iteon(
    dev_t dev,
    int flag)
{
	int unit = minor(dev);
	struct tty *tp = &ite_tty[unit];
	struct ite_softc *ip = &ite_softc[unit];

	if (unit < 0 || unit >= NITE || (ip->flags&ITE_ALIVE) == 0)
		return(ENXIO);
	/* force ite active, overriding graphics mode */
	if (flag & 1) {
		ip->flags |= ITE_ACTIVE;
		ip->flags &= ~(ITE_INGRF|ITE_INITED);
	}
	/* leave graphics mode */
	if (flag & 2) {
		ip->flags &= ~ITE_INGRF;
		if ((ip->flags & ITE_ACTIVE) == 0)
			return(0);
	}
	ip->flags |= ITE_ACTIVE;
	if (ip->flags & ITE_INGRF)
		return(0);
	if (kbd_tty == NULL || kbd_tty == tp) {
		kbd_tty = tp;
		kbdenable(unit);
	}
	iteinit(dev);
	return(0);
}

void
iteinit(dev_t dev)
{
	int unit = minor(dev);
	struct ite_softc *ip = &ite_softc[unit];

	if (ip->flags & ITE_INITED)
		return;
	
	ip->curx = 0;
	ip->cury = 0;
	ip->cursorx = 0;
	ip->cursory = 0;

	(*ip->isw->ite_init)(ip);
	ip->flags |= ITE_CURSORON;
	ite_drawcursor(ip, ip->isw);

	ip->attribute = 0;
	if (ip->attrbuf == NULL)
		if (kmem_alloc(kernel_map, (vm_offset_t *)&ip->attrbuf, ip->rows * ip->cols)
		    != KERN_SUCCESS)
			panic("iteinit: no memory for attributes");

	bzero((char*)ip->attrbuf, (ip->rows * ip->cols));

	ip->imode = 0;
	ip->flags |= ITE_INITED;
}

/*
 * "Shut down" device as terminal emulator.
 * Note that we do not deinit the console device unless forced.
 * Deinit'ing the console every time leads to a very active
 * screen when processing /etc/rc.
 */
void
iteoff(
    dev_t dev,
    int flag)
{
	register struct ite_softc *ip = &ite_softc[minor(dev)];

	if (flag & 2) {
		ip->flags |= ITE_INGRF;
		ip->flags &= ~ITE_CURSORON;
	}
	if ((ip->flags & ITE_ACTIVE) == 0)
		return;
	if ((flag & 1) ||
	    (ip->flags & (ITE_INGRF|ITE_ISCONS|ITE_INITED)) == ITE_INITED)
		(*ip->isw->ite_deinit)(ip);
	if ((flag & 2) == 0)
		ip->flags &= ~ITE_ACTIVE;
}

/*ARGSUSED*/
int
iteopen(
	dev_t dev,
	int flag,
	io_req_t ior)
{
	int unit = minor(dev);
	register struct tty *tp = &ite_tty[unit];
	register struct ite_softc *ip = &ite_softc[unit];
	register int error;
	int first = 0;

	if ((ip->flags & ITE_ACTIVE) == 0) {
		error = iteon(dev, 0);
		if (error)
			return (error);
		first = 1;
	}
	tp->t_oproc = itestart;
	tp->t_stop = itestop;
	if ((tp->t_state&TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_ispeed = tp->t_ospeed = 9600;
		tp->t_state |= TS_ISOPEN|TS_CARR_ON;
		tp->t_flags = ECHO|CRMOD;
	}
	error = char_open(dev, tp, flag, ior);
	if (error) {
		if (first)
			iteoff(dev, 0);
		return (error);
	}
	return (0);
}

/*ARGSUSED*/
void
iteclose(dev_t dev)
{
	register struct tty *tp = &ite_tty[minor(dev)];

	ttyclose(tp);
	iteoff(dev, 0);
}

/*ARGSUSED*/
int
iteread(
	dev_t dev,
	struct uio *uio)
{
	register struct tty *tp = &ite_tty[minor(dev)];

	return ((*linesw[tp->t_line].l_read)(tp, uio));
}

/*ARGSUSED*/
int
itewrite(
	dev_t dev,
	struct uio *uio)
{
	int unit = minor(dev);
	register struct tty *tp = &ite_tty[unit];

	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

boolean_t
iteportdeath(
	dev_t   dev,
	ipc_port_t port)
{
	return (tty_portdeath(&ite_tty[minor(dev)], port));
}

io_return_t
itegetstat(
	dev_t		dev,
	int		flavor,
	int *		data,		/* pointer to OUT array */
	unsigned int	*count)		/* out */
{
	return(tty_get_status(&ite_tty[minor(dev)], flavor, data, count));
}

io_return_t
itesetstat(
	dev_t		dev,
	int		flavor,
	int *		data,
	unsigned int	count)
{
	return(tty_set_status(&ite_tty[minor(dev)], flavor, data, count));
}

void
itestop(
	struct tty *tp,
	int flag)
{
}

void
iterestart(struct tty *tp)
{
	register spl_t s = splhil();

	tp->t_state &= ~TS_TIMEOUT;
	itestart(tp);
	splx(s);
}

void
itestart(struct tty *tp)
{
	register int cc;
	spl_t s;
	int hiwat = 0, hadcursor = 0;
	struct ite_softc *ip;

	/*
	 * (Potentially) lower priority.  We only need to protect ourselves
	 * from keyboard interrupts since that is all that can affect the
	 * state of our tty (kernel printf doesn't go through this routine).
	 */
	s = splhil();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY;
	cc = tp->t_outq.c_cc;
	if (cc <= TTLOWAT(tp)) {
		tt_write_wakeup(tp);
	}
	/*
	 * Handle common (?) case
	 */
	if (cc == 1) {
		iteputchar(getc(&tp->t_outq), tp->t_dev);
	} else if (cc) {
		/*
		 * Limit the amount of output we do in one burst
		 * to prevent hogging the CPU.
		 */
		if (cc > iteburst) {
			hiwat++;
			cc = iteburst;
		}
		/*
		 * Turn off cursor while we output multiple characters.
		 * Saves a lot of expensive window move operations.
		 */
		ip = &ite_softc[minor(tp->t_dev)];
		if (ip->flags & ITE_CURSORON) {
			ite_erasecursor(ip, ip->isw);
			ip->flags &= ~ITE_CURSORON;
			hadcursor = 1;
		}
		while (--cc >= 0) {
#ifdef CCA_PAGE
			if (!(tp->t_flags & (RAW|CBREAK|LITOUT)) &&
			    tp->t_pagemode &&
			    (firstc(&tp->t_outq) & 0177) == '\n') {
				ttpage(tp);
				if (tp->t_state & TS_TTSTOP)
					break;
			}
#endif /* CCA_PAGE */
			iteputchar(getc(&tp->t_outq), tp->t_dev);
		}
		if (hadcursor) {
			ip->flags |= ITE_CURSORON;
			ite_drawcursor(ip, ip->isw);
		}
		if (hiwat) {
			tp->t_state |= TS_TIMEOUT;
			timeout((timeout_fcn_t)iterestart, tp, 1);
		}
	}
	tp->t_state &= ~TS_BUSY;
	splx(s);
}

void
itefilter(
	  char stat, 
	  char c)
{
	static int capsmode = 0;
	static int metamode = 0;
  	register char code, *str;

	if (kbd_tty == NULL || (kbd_tty->t_state & TS_ISOPEN) == 0)
		return;

	switch (c & 0xFF) {
	case KBD_CAPSLOCK:
		capsmode = !capsmode;
		return;

	case KBD_EXT_LEFT_DOWN:
	case KBD_EXT_RIGHT_DOWN:
		metamode = 1;
		return;
		
	case KBD_EXT_LEFT_UP:
	case KBD_EXT_RIGHT_UP:
		metamode = 0;
		return;
	}

	c &= KBD_CHARMASK;

	switch ((stat>>KBD_SSHIFT) & KBD_SMASK) {
	case KBD_KEY:
	        if (!capsmode) {
			code = kbd_keymap[c];
			break;
		}
		/* fall into... */

	case KBD_SHIFT:
		code = kbd_shiftmap[c];
		break;

	case KBD_CTRL:
		code = kbd_ctrlmap[c];
		break;
		
	case KBD_CTRLSHIFT:	
		code = kbd_ctrlshiftmap[c];
		break;
        }

	if (code == (char)NULL && (str = kbd_stringmap[c]) != NULL) {
		while (*str)
			(*linesw[kbd_tty->t_line].l_rint)(*str++, kbd_tty);
	} else {
		if (metamode)
			code |= 0x80;
		(*linesw[kbd_tty->t_line].l_rint)(code, kbd_tty);
	}
}

#if NGKD > 0
void
gkditefilter(
	     struct gkd_dev *kdev,
	     char stat,
	     char c)
{
	int capsmode = kdev->flags & F_GKD_CAPS;
	int numlckmode = kdev->flags & F_GKD_NUMLK;
	int metamode = kdev->flags & F_GKD_ALT;
  	char code, *str;

	if (kbd_tty == NULL || (kbd_tty->t_state & TS_ISOPEN) == 0)
		return;

	switch (c & 0xFF) {
	case KBD_CAPSLOCK:
		capsmode = !capsmode;
		return;

	case KBD_EXT_LEFT_DOWN:
	case KBD_EXT_RIGHT_DOWN:
		metamode = 1;
		return;
		
	case KBD_EXT_LEFT_UP:
	case KBD_EXT_RIGHT_UP:
		metamode = 0;
		return;
	}

	c &= KBD_CHARMASK;

	switch ((stat>>KBD_SSHIFT) & KBD_SMASK) {
	case KBD_KEY:
	        if (!capsmode) {
			code = kbd_keymap[c];
			if (numlckmode)
				gkd_chk_numlock(&code, c);
			break;
		}
		/* fall into... */

	case KBD_SHIFT:
		code = kbd_shiftmap[c];
		break;

	case KBD_CTRL:
		code = kbd_ctrlmap[c];
		break;
		
	case KBD_CTRLSHIFT:	
		code = kbd_ctrlshiftmap[c];
		break;
        }

	if (code == (char)NULL && (str = kbd_stringmap[c]) != NULL) {
		while (*str)
			(*linesw[kbd_tty->t_line].l_rint)(*str++, kbd_tty);
	} else {
		if (metamode)
			code |= 0x80;
		(*linesw[kbd_tty->t_line].l_rint)(code, kbd_tty);
	}
}
#endif /* NGKD > 0 */

void
iteputchar(
	   int c,
	   dev_t dev)
{
	int unit = minor(dev);
	register struct ite_softc *ip = &ite_softc[unit];
	register struct itesw *sp = ip->isw;
	register int n;

	if ((ip->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE)
	  	return;

	if (ip->escape) {
doesc:
		switch (ip->escape) {

		case '&':			/* Next can be a,d, or s */
			if (ip->fpd++) {
				ip->escape = c;
				ip->fpd = 0;
			}
			return;

		case 'a':				/* cursor change */
			switch (c) {

			case 'Y':			/* Only y coord. */
				ip->cury = MIN(ip->pos, ip->rows-1);
				ip->pos = 0;
				ip->escape = 0;
				ite_movecursor(ip, sp);
				clr_attr(ip, ATTR_INV);
				break;

			case 'y':			/* y coord first */
				ip->cury = MIN(ip->pos, ip->rows-1);
				ip->pos = 0;
				ip->fpd = 0;
				break;

			case 'C':			/* x coord */
				ip->curx = MIN(ip->pos, ip->cols-1);
				ip->pos = 0;
				ip->escape = 0;
				ite_movecursor(ip, sp);
				clr_attr(ip, ATTR_INV);
				break;

			default:	     /* Possibly a 3 digit number. */
				if (c >= '0' && c <= '9' && ip->fpd < 3) {
					ip->pos = ip->pos * 10 + (c - '0');
					ip->fpd++;
				} else {
					ip->pos = 0;
					ip->escape = 0;
				}
				break;
			}
			return;

		case 'd':				/* attribute change */
			switch (c) {

			case 'B':
				set_attr(ip, ATTR_INV);
				break;
		        case 'D':
				/* XXX: we don't do anything for underline */
				set_attr(ip, ATTR_UL);
				break;
		        case '@':
				clr_attr(ip, ATTR_ALL);
				break;
			}
			ip->escape = 0;
			return;

		case 's':				/* keypad control */
			switch (ip->fpd) {

			case 0:
				ip->hold = c;
				ip->fpd++;
				return;

			case 1:
				if (c == 'A') {
					switch (ip->hold) {
	
					case '0':
						clr_attr(ip, ATTR_KPAD);
						break;
					case '1':
						set_attr(ip, ATTR_KPAD);
						break;
					}
				}
				ip->hold = 0;
			}
			ip->escape = 0;
			return;

		case 'i':			/* back tab */
			if (ip->curx > TABSIZE) {
				n = ip->curx - (ip->curx & (TABSIZE - 1));
				ip->curx -= n;
			} else
				ip->curx = 0;
			ite_movecursor(ip, sp);
			ip->escape = 0;
			return;

		case '3':			/* clear all tabs */
			goto ignore;

		case 'K':			/* clear_eol */
			ite_clrtoeol(ip, sp, ip->cury, ip->curx);
			ip->escape = 0;
			return;

		case 'J':			/* clear_eos */
			ite_clrtoeos(ip, sp);
			ip->escape = 0;
			return;

		case 'B':			/* cursor down 1 line */
			if (++ip->cury == ip->rows) {
				--ip->cury;
				ite_erasecursor(ip, sp);
				(*sp->ite_scroll)(ip, 1, 0, 1, SCROLL_UP);
				ite_clrtoeol(ip, sp, ip->cury, 0);
			}
			else
				ite_movecursor(ip, sp);
			clr_attr(ip, ATTR_INV);
			ip->escape = 0;
			return;

		case 'C':			/* cursor forward 1 char */
			ip->escape = 0;
			itecheckwrap(ip, sp);
			return;

		case 'A':			/* cursor up 1 line */
			if (ip->cury > 0) {
				ip->cury--;
				ite_movecursor(ip, sp);
			}
			ip->escape = 0;
			clr_attr(ip, ATTR_INV);
			return;

		case 'P':			/* delete character */
			ite_dchar(ip, sp);
			ip->escape = 0;
			return;

		case 'M':			/* delete line */
			ite_dline(ip, sp);
			ip->escape = 0;
			return;

		case 'Q':			/* enter insert mode */
			ip->imode = 1;
			ip->escape = 0;
			return;

		case 'R':			/* exit insert mode */
			ip->imode = 0;
			ip->escape = 0;
			return;

		case 'L':			/* insert blank line */
			ite_iline(ip, sp);
			ip->escape = 0;
			return;

		case 'h':			/* home key */
			ip->cury = ip->curx = 0;
			ite_movecursor(ip, sp);
			ip->escape = 0;
			return;

		case 'D':			/* left arrow key */
			if (ip->curx > 0) {
				ip->curx--;
				ite_movecursor(ip, sp);
			}
			ip->escape = 0;
			return;

		case '1':			/* set tab in all rows */
			goto ignore;

		case ESC:
			if ((ip->escape = c) == ESC)
				break;
			ip->fpd = 0;
			goto doesc;

		default:
ignore:
			ip->escape = 0;
			return;

		}
	}

	switch (c &= 0x7F) {

	case '\n':

		if (++ip->cury == ip->rows) {
			--ip->cury;
			ite_erasecursor(ip, sp);
			(*sp->ite_scroll)(ip, 1, 0, 1, SCROLL_UP);
			ite_clrtoeol(ip, sp, ip->cury, 0);
		} else
			ite_movecursor(ip, sp);
		clr_attr(ip, ATTR_INV);
		break;

	case '\r':
		if (ip->curx) {
			ip->curx = 0;
			ite_movecursor(ip, sp);
		}
		break;
	
	case '\b':
		if (--ip->curx < 0)
			ip->curx = 0;
		else
			ite_movecursor(ip, sp);
		break;

	case '\t':
		if (ip->curx < TABEND(unit)) {
			n = TABSIZE - (ip->curx & (TABSIZE - 1));
			ip->curx += n;
			ite_movecursor(ip, sp);
		} else
			itecheckwrap(ip, sp);
		break;

	case CTRL('g'):
		if (&ite_tty[unit] == kbd_tty)
			kbdbell(unit);
		break;

	case ESC:
		ip->escape = ESC;
		break;

	default:
		if (c < ' ' || c == DEL)
			break;
		if (ip->imode)
			ite_ichar(ip, sp);
		if ((ip->attribute & ATTR_INV) || attrtest(ip, ATTR_INV)) {
			attrset(ip, ATTR_INV);
			(*sp->ite_putc)(ip, c, ip->cury, ip->curx, ATTR_INV);
		} else
			(*sp->ite_putc)(ip, c, ip->cury, ip->curx, ATTR_NOR);
		ite_drawcursor(ip, sp);
		itecheckwrap(ip, sp);
		break;
	}
}

void
itecheckwrap(
	     struct ite_softc *ip,
	     struct itesw *sp)
{
	if (++ip->curx == ip->cols) {
		ip->curx = 0;
		clr_attr(ip, ATTR_INV);
		if (++ip->cury == ip->rows) {
			--ip->cury;
			ite_erasecursor(ip, sp);
			(*sp->ite_scroll)(ip, 1, 0, 1, SCROLL_UP);
			ite_clrtoeol(ip, sp, ip->cury, 0);
			return;
		}
	}
	ite_movecursor(ip, sp);
}

void
ite_dchar(
	  struct ite_softc *ip,
	  struct itesw *sp)
{
	if (ip->curx < ip->cols - 1) {
		ite_erasecursor(ip, sp);
		(*sp->ite_scroll)(ip, ip->cury, ip->curx + 1, 1, SCROLL_LEFT);
		attrmov(ip, ip->cury, ip->curx + 1, ip->cury, ip->curx,
			1, ip->cols - ip->curx - 1);
	}
	attrclr(ip, ip->cury, ip->cols - 1, 1, 1);
	(*sp->ite_putc)(ip, ' ', ip->cury, ip->cols - 1, ATTR_NOR);
	ite_drawcursor(ip, sp);
}

void
ite_ichar(
	  struct ite_softc *ip,
	  struct itesw *sp)
{
	if (ip->curx < ip->cols - 1) {
		ite_erasecursor(ip, sp);
		(*sp->ite_scroll)(ip, ip->cury, ip->curx, 1, SCROLL_RIGHT);
		attrmov(ip, ip->cury, ip->curx, ip->cury, ip->curx + 1,
			1, ip->cols - ip->curx - 1);
	}
	attrclr(ip, ip->cury, ip->curx, 1, 1);
	(*sp->ite_putc)(ip, ' ', ip->cury, ip->curx, ATTR_NOR);
	ite_drawcursor(ip, sp);
}

void
ite_dline(
	  struct ite_softc *ip,
	  struct itesw *sp)
{
	if (ip->cury < ip->rows - 1) {
		ite_erasecursor(ip, sp);
		(*sp->ite_scroll)(ip, ip->cury + 1, 0, 1, SCROLL_UP);
		attrmov(ip, ip->cury + 1, 0, ip->cury, 0,
			ip->rows - ip->cury - 1, ip->cols);
	}
	ite_clrtoeol(ip, sp, ip->rows - 1, 0);
}

void
ite_iline(
	  struct ite_softc *ip,
	  struct itesw *sp)
{
	if (ip->cury < ip->rows - 1) {
		ite_erasecursor(ip, sp);
		(*sp->ite_scroll)(ip, ip->cury, 0, 1, SCROLL_DOWN);
		attrmov(ip, ip->cury, 0, ip->cury + 1, 0,
			ip->rows - ip->cury - 1, ip->cols);
	}
	ite_clrtoeol(ip, sp, ip->cury, 0);
}

void
ite_clrtoeol(
	     struct ite_softc *ip,
	     struct itesw *sp,
	     int y, 
	     int x)
{
	(*sp->ite_clear)(ip, y, x, 1, ip->cols - x);
	attrclr(ip, y, x, 1, ip->cols - x);
	ite_drawcursor(ip, sp);
}

void
ite_clrtoeos(
	     struct ite_softc *ip,
	     struct itesw *sp)
{
	(*sp->ite_clear)(ip, ip->cury, 0, ip->rows - ip->cury, ip->cols);
	attrclr(ip, ip->cury, 0, ip->rows - ip->cury, ip->cols);
	ite_drawcursor(ip, sp);
}

/*
 * Keyboard functions.
 * XXX split these out to another file?
 * XXX these are a total hack right now, needs to be thought out.
 */
int havekeyboard = 0;	/* may be set in busconf() */

int
kbdexists(int unit)
{

	return (havekeyboard);
}

void
kbdenable(int unit)
{
#if NGKD > 0
	if (havekeyboard == 2) {
		gkdkbdenable(unit);
		return;
	}
#endif
#if NHIL > 0
	if (havekeyboard == 1) {
		hilkbdenable(unit);
		return;
	}
#endif
	kbd_tty = NULL;
}

void
kbddisable(int unit)
{
#if NGKD > 0
	if (havekeyboard == 2) {
		gkdkbddisable(unit);
		return;
	}
#endif
#if NHIL > 0
	if (havekeyboard == 1) {
		hilkbddisable(unit);
		return;
	}
#endif
}

void
kbdbell(int unit)
{
#if NGKD > 0
	if (havekeyboard == 2) {
		gkdkbdbell(unit);
		return;
	}
#endif
#if NHIL > 0
	if (havekeyboard == 1) {
		hilkbdbell(unit);
		return;
	}
#endif
}

int
kbdgetc(int unit, int *statp, int wait)
{
#if NGKD > 0
	if (havekeyboard == 2)
		return (gkdkbdgetc(unit, statp, wait));
#endif
#if NHIL > 0
	if (havekeyboard == 1)
		return (hilkbdgetc(unit, statp, wait));
#endif
	return (0);
}

#ifdef DEBUG
/*
 * Minimum ITE number at which to start looking for a console.
 * Setting to 0 will do normal search, 1 will skip first ITE device,
 * NITE will skip ITEs and use serial port.
 */
int	whichconsole = 0;
#endif

void
itecnprobe(struct consdev *cp)
{
	register struct ite_softc *ip;
	int i, sw, maj, unit, pri;

	maj = 0;	/* No major number of because of indirection table */

	/* urk! */
	grfconfig();

	/* check all the individual displays and find the best */
	unit = -1;
	pri = CN_DEAD;
	for (i = 0; i < NITE; i++) {
		struct grf_softc *gp = &grf_softc[i];

		ip = &ite_softc[i];
		if ((gp->g_flags & GF_ALIVE) == 0)
			continue;
		ip->flags = (ITE_ALIVE|ITE_CONSOLE);

		/* locate the proper switch table. */
		for (sw = 0; sw < nitesw; sw++)
			if (itesw[sw].ite_hwid == gp->g_sw->gd_hwid)
				break;
		if (sw == nitesw)
			continue;

		ip->isw = &itesw[sw];
		ip->grf = gp;

#ifdef DEBUG
		if (i < whichconsole)
			continue;
#endif
		/*
		 * Must have an associated keyboard
		 */
		if (!kbdexists(i))
			continue;

		{
			extern struct iodc_data cons_iodc; /* defined in... */
			extern struct device_path cons_dp; /* vm_machdep.c */

			/*
			 * If the console we booted on was an ITE, then
			 * keep it as our console (CN_REMOTE).  Otherwise the
			 * ITE is just another console possibility (CN_NORMAL).
			 */
			if (unit < 0) {
				if (cons_iodc.iodc_sv_model == SVMOD_FIO_SGC)
					pri = CN_REMOTE;
				else
					pri = CN_INTERNAL;
				unit = i;
			}
		}
	}

	/* initialize required fields */
	cp->cn_dev = makedev(maj, unit);
	cp->cn_pri = pri;
}

void
itecninit(struct consdev *cp)
{
	int unit = minor(cp->cn_dev);
	struct ite_softc *ip = &ite_softc[unit];

	ip->attrbuf = console_attributes;
	iteinit(cp->cn_dev);
	ip->flags |= (ITE_ACTIVE|ITE_ISCONS);
	kbd_tty = &ite_tty[unit];
}

/* XXX HIL specific */
int
itecngetc(dev_t dev, boolean_t wait)
{
	int unit = 0;	/* XXX */
	int c, stat;

	c = kbdgetc(unit, &stat, wait);
	switch ((stat >> KBD_SSHIFT) & KBD_SMASK) {
	case KBD_SHIFT:
		c = kbd_shiftmap[c & KBD_CHARMASK];
		break;
	case KBD_CTRL:
		c = kbd_ctrlmap[c & KBD_CHARMASK];
		break;
	case KBD_KEY:
		c = kbd_keymap[c & KBD_CHARMASK];
		break;
	default:
		c = 0;
		break;
	}
	return(c);
}

void
itecnputc(
	dev_t dev,
	char c)
{
	static int paniced = 0;
	struct ite_softc *ip = &ite_softc[minor(dev)];
	extern char *panicstr;

	if (panicstr && !paniced &&
	    (ip->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE) {
		(void) iteon(dev, 3);
		paniced = 1;
	}
	iteputchar(c, dev);
}
#endif
