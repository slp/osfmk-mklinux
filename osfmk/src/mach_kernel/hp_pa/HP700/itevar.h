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
 * Copyright (c) 1988-1992, The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 *	Utah $Hdr: itevar.h 1.15 92/02/29$
 */

struct ite_softc {
	int	flags;
	struct  itesw *isw;
	struct  grf_softc *grf;
	caddr_t regbase, fbbase;
	short	curx, cury;
	short   cursorx, cursory;
	short   cblankx, cblanky;
	short	rows, cols;
	short   cpl;
	short	dheight, dwidth;
	short	fbheight, fbwidth;
	short	ftheight, ftwidth;
	short	fontx, fonty;
	short   attribute;
	u_char	*attrbuf;
	short	planemask;
	short	pos;
	char	imode, escape, fpd, hold;
	caddr_t	devdata;			/* display dependent data */
};

struct itesw {
	int	ite_hwid;		/* Hardware id */
	void	(*ite_init)(struct ite_softc *);
	void	(*ite_deinit)(struct ite_softc *);
	void	(*ite_clear)(struct ite_softc *, int, int, int, int);
	void	(*ite_putc)(struct ite_softc *, char, int, int, int);
	void	(*ite_cursor)(struct ite_softc *, int);
	void	(*ite_scroll)(struct ite_softc *, int, int, int, int);
	u_char	(*ite_readbyte)(struct ite_softc *);
	int	(*ite_writeglyph)(struct ite_softc *);
};

#define getbyte(ip, offset) \
	((*(ip)->isw->ite_readbyte)(ip, offset))

#define getword(ip, offset) \
	((getbyte(ip, offset) << 8) | getbyte(ip, (offset) + 2))

#define writeglyph(ip, offset, fontbuf) \
	((*(ip)->isw->ite_writeglyph)((ip), (offset), (fontbuf)))

/* Flags */
#define ITE_ALIVE	0x01	/* hardware exists */
#define ITE_INITED	0x02	/* device has been initialized */
#define ITE_CONSOLE	0x04	/* device can be console */
#define ITE_ISCONS	0x08	/* device is console */
#define ITE_ACTIVE	0x10	/* device is being used as ITE */
#define ITE_INGRF	0x20	/* device in use as non-ITE */
#define ITE_CURSORON	0x40	/* cursor being tracked */

#define attrloc(ip, y, x) \
	(ip->attrbuf + ((y) * ip->cols) + (x))

#define attrclr(ip, sy, sx, h, w) \
	bzero((char*)(ip->attrbuf + ((sy) * ip->cols) + (sx)), (h) * (w))
  
#define attrmov(ip, sy, sx, dy, dx, h, w) \
	bcopy((const char*)(ip->attrbuf + ((sy) * ip->cols) + (sx)), \
	      (char*)(ip->attrbuf + ((dy) * ip->cols) + (dx)), \
	      (h) * (w))

#define attrtest(ip, attr) \
	((* (u_char *) attrloc(ip, ip->cury, ip->curx)) & attr)

#define attrset(ip, attr) \
	((* (u_char *) attrloc(ip, ip->cury, ip->curx)) = attr)
  
/*
 * X and Y location of character 'c' in the framebuffer, in pixels.
 */
#define	charX(ip,c)	\
	(((c) % (ip)->cpl) * (ip)->ftwidth + (ip)->fontx)

#define	charY(ip,c)	\
	(((c) / (ip)->cpl) * (ip)->ftheight + (ip)->fonty)

/*
 * The cursor is just an inverted space.
 */
#define draw_cursor(ip) { \
	WINDOWMOVER(ip, ip->cblanky, ip->cblankx, \
		    ip->cury * ip->ftheight, \
		    ip->curx * ip->ftwidth, \
		    ip->ftheight, ip->ftwidth, RR_XOR); \
        ip->cursorx = ip->curx; \
	ip->cursory = ip->cury; }

#define erase_cursor(ip) \
  	WINDOWMOVER(ip, ip->cblanky, ip->cblankx, \
		    ip->cursory * ip->ftheight, \
		    ip->cursorx * ip->ftwidth, \
		    ip->ftheight, ip->ftwidth, RR_XOR);

/* Character attributes */
#define ATTR_NOR        0x0             /* normal */
#define	ATTR_INV	0x1		/* inverse */
#define	ATTR_UL		0x2		/* underline */
#define ATTR_ALL	(ATTR_INV | ATTR_UL)

/* Keyboard attributes */
#define ATTR_KPAD	0x4		/* keypad transmit */
  
/* Replacement Rules */
#define RR_CLEAR		0x0
#define RR_COPY			0x3
#define RR_XOR			0x6
#define RR_COPYINVERTED  	0xc

#define SCROLL_UP	0x01
#define SCROLL_DOWN	0x02
#define SCROLL_LEFT	0x03
#define SCROLL_RIGHT	0x04
#define DRAW_CURSOR	0x05
#define ERASE_CURSOR    0x06
#define MOVE_CURSOR	0x07

#define KBD_SSHIFT	4		/* bits to shift status */
#define	KBD_CHARMASK	0x7F

/* keyboard status */
#define	KBD_SMASK	0xF		/* service request status mask */
#define	KBD_CTRLSHIFT	0x8		/* key + CTRL + SHIFT */
#define	KBD_CTRL	0x9		/* key + CTRL */
#define	KBD_SHIFT	0xA		/* key + SHIFT */
#define	KBD_KEY		0xB		/* key only */

#define KBD_CAPSLOCK    0x18

#define KBD_EXT_LEFT_DOWN     0x12
#define KBD_EXT_LEFT_UP       0x92
#define KBD_EXT_RIGHT_DOWN    0x13
#define KBD_EXT_RIGHT_UP      0x93

#define	TABSIZE		8
#ifdef	MACH_KERNEL
#define	TABEND(u)	(ite_softc[u].rows - TABSIZE)
#else
#define	TABEND(u)	(ite_tty[u].t_winsize.ws_col - TABSIZE)
#endif

#ifdef MACH_KERNEL
extern	struct ite_softc ite_softc[];
extern	struct itesw itesw[];
extern	int nitesw;
#endif

/*
 * Prototypes.
 */
extern void stiite_init(struct ite_softc *);
extern void stiite_scroll(struct ite_softc *, int, int, int, int);
extern void stiite_deinit(struct ite_softc *);
extern void stiite_clear(struct ite_softc *, int, int, int, int);
extern void stiite_putc(struct ite_softc *, char, int, int, int);
extern void stiite_cursor(struct ite_softc *, int);

extern int iteon(dev_t, int);
extern void iteoff(dev_t, int);

