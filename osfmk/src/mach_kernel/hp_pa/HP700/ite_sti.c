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
 *	Utah $Hdr: ite_sti.c 1.12 94/12/16$
 */

#include "ite.h"
#if NITE > 0

#include <types.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <device/tty.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <machine/spl.h>
#include <hp_pa/HP700/itevar.h>
#include <hp_pa/HP700/itereg.h>
#include <hp_pa/HP700/grfioctl.h>	/* XXX */
#include <hp_pa/HP700/grfvar.h>	/* XXX */
#include <hp_pa/HP700/grfreg.h>	/* XXX */

/*
 * XXX we have heard that not all displays have offscreen memory
 * but that some revs of the STI ROM say they do.  So for now,
 * we just don't use it and do putchar by just unpacking font
 * chars directly to the onscreen destination.
 */
/* #define DOOFFSCREEN */

#define DEVDATA		((struct grfdev *)(ip->devdata))
#define WINDOWMOVER 	sti_windowmove

#include <kern/misc_protos.h>

/*
 * Forwards
 */
void sti_windowmove(struct ite_softc *ip, int sy, int sx, int dy, int dx, int h, int w, int func);
void sti_fontinfo(struct ite_softc *ip);
void sti_fontinit(struct ite_softc *ip);
void sti_getfontchar(struct ite_softc *ip, int c, int dy, int dx, int invert);

void
stiite_init(struct ite_softc *ip)
{
	struct {
		struct sti_initflags flags;
		struct sti_initin input;
		struct sti_initout output;
	} iparm;
	ITEDECL;

	/* XXX */
	if (ip->regbase == NULL) {
		struct grfinfo *gi = &ip->grf->g_display;
		ip->regbase = gi->gd_regaddr;
		ip->fbbase = gi->gd_fbaddr;
		ip->fbwidth = gi->gd_fbwidth;
		ip->fbheight = gi->gd_fbheight;
		ip->dwidth = gi->gd_dwidth;
		ip->dheight = gi->gd_dheight;
		ip->devdata = (caddr_t) ip->grf->g_data;
	}
	/*
	 * Re-initialize the device in case an application like the
	 * X-server crashed hard and left us in a Really Bad State.
	 */
	bzero((char *)&iparm, sizeof iparm);

	iparm.flags.wait = 1;
#if 0
	iparm.flags.hardreset = 1;
	iparm.flags.cmap_black = 1;
#endif
	iparm.flags.texton = 1;
	iparm.flags.clear = 1;
	iparm.flags.init_text_cmap = 1;
	iparm.flags.no_change_bet = 1;
	iparm.flags.no_change_bei = 1;
	iparm.input.text_planes = 1;
	ITEACCESS;
	(*DEVDATA->ep->init_graph)(&iparm.flags, &iparm.input, &iparm.output,
				   &DEVDATA->config);
	ITEUNACCESS;
	if (iparm.output.errno)
		log(LOG_WARNING, "ite%d: init_graph returned %d\n",
		    ip - ite_softc, iparm.output.errno);

	/*
	 * Clear the entire frame buffer.
	 *
	 * XXX to be consistant with other display devices, this "clear"
	 * should clear all planes.  However, I don't know how to do
	 * that through the STI interface, it clears only text planes.
	 */
#ifdef DOOFFSCREEN
	sti_windowmove(ip, 0, 0, 0, 0, ip->fbheight, ip->fbwidth, RR_CLEAR);
#else
	sti_windowmove(ip, 0, 0, 0, 0, ip->dheight, ip->dwidth, RR_CLEAR);
#endif
	sti_fontinfo(ip);
	sti_fontinit(ip);
}
	
void
stiite_deinit(struct ite_softc *ip)
{
#ifdef DOOFFSCREEN
	sti_windowmove(ip, 0, 0, 0, 0, ip->fbheight, ip->fbwidth, RR_CLEAR);
#else
	sti_windowmove(ip, 0, 0, 0, 0, ip->dheight, ip->dwidth, RR_CLEAR);
#endif

   	ip->flags &= ~ITE_INITED;
}

void
stiite_cursor(
	      struct ite_softc *ip,
	      int flag)
{
	if (flag != DRAW_CURSOR) {
		/* erase cursor */
		sti_windowmove(ip, ip->cursory * ip->ftheight,
			       ip->cursorx * ip->ftwidth,
			       ip->cursory * ip->ftheight,
			       ip->cursorx * ip->ftwidth,
			       ip->ftheight, ip->ftwidth, RR_COPYINVERTED);
	}
	if (flag != ERASE_CURSOR) {
		/* draw cursor */
		sti_windowmove(ip, ip->cury * ip->ftheight,
			       ip->curx * ip->ftwidth,
			       ip->cury * ip->ftheight,
			       ip->curx * ip->ftwidth,
			       ip->ftheight, ip->ftwidth, RR_COPYINVERTED);
		ip->cursorx = ip->curx;
		ip->cursory = ip->cury;
	}
}
	
void
stiite_putc(
	    struct ite_softc *ip,
	    char c, 
	    int dy, 
	    int dx, 
	    int mode)
{
#ifdef DOOFFSCREEN
	sti_windowmove(ip, charY(ip, c), charX(ip, c),
		       dy * ip->ftheight, dx * ip->ftwidth,
		       ip->ftheight, ip->ftwidth,
		       mode == ATTR_INV ? RR_COPYINVERTED : RR_COPY);
#else
	sti_getfontchar(ip, c, dy * ip->ftheight, dx * ip->ftwidth,
			mode == ATTR_INV ? 1 : 0);
#endif
}
	
void
stiite_clear(
	     struct ite_softc *ip,
	     int sy, 
	     int sx, 
	     int h, 
	     int w)
{
	sti_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
		       sy * ip->ftheight, sx * ip->ftwidth, 
		       h  * ip->ftheight, w  * ip->ftwidth,
		       RR_CLEAR);
}

void
stiite_scroll(
        struct ite_softc *ip,
        int sy, 
	int sx, 
	int count,
        int dir)
{
	register int dy;
	register int dx = sx;
	register int height = 1;
	register int width = ip->cols;

	if (dir == SCROLL_UP) {
		dy = sy - count;
		height = ip->rows - sy;
	}
	else if (dir == SCROLL_DOWN) {
		dy = sy + count;
		height = ip->rows - dy - 1;
	}
	else if (dir == SCROLL_RIGHT) {
		dy = sy;
		dx = sx + count;
		width = ip->cols - dx;
	}
	else {
		dy = sy;
		dx = sx - count;
		width = ip->cols - sx;
	}		

	sti_windowmove(ip, sy * ip->ftheight, sx * ip->ftwidth,
		       dy * ip->ftheight, dx * ip->ftwidth,
		       height * ip->ftheight,
		       width  * ip->ftwidth, RR_COPY);
}

void
sti_windowmove(
	struct ite_softc *ip,
	int sy, int sx, int dy, int dx, int h, int w, int func)
{
	struct {
		struct sti_moveflags flags;
		struct sti_movein input;
		struct sti_moveout output;
	} mparm;
	ITEDECL;
	
	bzero((char *)&mparm, sizeof mparm);
	mparm.flags.wait = 1;
	if (func == RR_CLEAR) {
		mparm.flags.clear = 1;
		mparm.input.bg_color = 0;
	} else {
		if (sx == dx && sy == dy)
			mparm.flags.color = 1;
		if (func == RR_COPY) {
			mparm.input.fg_color = 1;
			mparm.input.bg_color = 0;
		} else {
			mparm.input.fg_color = 0;
			mparm.input.bg_color = 1;
		}
		mparm.input.src_y = sy;
		mparm.input.src_x = sx;
	}
	mparm.input.dest_y = dy;
	mparm.input.dest_x = dx;
	mparm.input.wheight = h;
	mparm.input.wwidth = w;

	ITEACCESS;
	(*DEVDATA->ep->block_move)(&mparm.flags, &mparm.input, &mparm.output,
				   &DEVDATA->config);
	ITEUNACCESS;
	if (mparm.output.errno)
		log(LOG_WARNING, "ite%d: block_move returned %d\n",
		    ip - ite_softc, mparm.output.errno);
}

void
sti_fontinfo(struct ite_softc *ip)
{
	u_int fontaddr;
	ITEDECL;

	ITEACCESS;
	fontaddr = (STI_FONTAD(DEVDATA->type, DEVDATA->romaddr) +
		    (u_int) DEVDATA->romaddr) & ~3;
	ip->ftheight = STIF_FHEIGHT(DEVDATA->type, fontaddr);
	ip->ftwidth  = STIF_FWIDTH(DEVDATA->type, fontaddr);
	ITEUNACCESS;

	ip->rows     = ip->dheight / ip->ftheight;
	ip->cols     = ip->dwidth / ip->ftwidth;

	if (ip->fbwidth > ip->dwidth) {
		/*
		 * Stuff goes to right of display.
		 */
		ip->fontx   = ip->dwidth;
		ip->fonty   = 0;
		ip->cpl     = (ip->fbwidth - ip->dwidth) / ip->ftwidth;
		ip->cblankx = ip->dwidth;
		ip->cblanky = ip->fonty + ((128 / ip->cpl) + 1) * ip->ftheight;
	}
	else {
		/*
		 * Stuff goes below the display.
		 */
		ip->fontx   = 0;
		ip->fonty   = ip->dheight;
		ip->cpl     = ip->fbwidth / ip->ftwidth;
		ip->cblankx = 0;
		ip->cblanky = ip->fonty + ((128 / ip->cpl) + 1) * ip->ftheight;
	}
}

void
sti_fontinit(struct ite_softc *ip)
{
#ifdef DOOFFSCREEN
	int c;

	for (c = 0; c < 128; c++)
		sti_getfontchar(ip, c, charY(ip, c), charX(ip, c), 0);
#endif
}

void
sti_getfontchar(
		struct ite_softc *ip,
		int c, int dy, int dx, int invert)
{
	struct sti_fontflags flags;
	struct sti_fontin input;
	struct sti_fontout output;
	ITEDECL;

	bzero((char *)&flags, sizeof flags);
	flags.wait = 1;
	if (invert) {
		input.fg_color = 0;
		input.bg_color = 1;
	} else {
		input.fg_color = 1;
		input.bg_color = 0;
	}
	input.index = c;
	input.dest_x = dx;
	input.dest_y = dy;
	input.future = 0;
	ITEACCESS;
	input.startaddr = STI_FONTAD(DEVDATA->type, DEVDATA->romaddr) +
		(int)DEVDATA->romaddr;
	(*DEVDATA->ep->font_unpmv)(&flags, &input, &output, &DEVDATA->config);
	ITEUNACCESS;
}

#endif
