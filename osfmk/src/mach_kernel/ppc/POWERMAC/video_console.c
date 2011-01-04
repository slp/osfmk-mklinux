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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */
/* MACH PPC - video_console.c 
 *
 * Original based on NetBSD's mac68k/dev/ite.c driver
 *
 * This driver differs in
 *	- MACH driver"ized"
 *	- Uses phys_copy and flush_cache to in several places
 *	  for performance optimizations
 *	- 7x15 font
 *	- Black background and white (character) foreground
 *	- Assumes 6100/7100/8100 class of machine 
 *
 * The original header follows...
 *
 *
 *	NetBSD: ite.c,v 1.16 1995/07/17 01:24:34 briggs Exp	
 *
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: ite.c 1.28 92/12/20$
 *
 *	@(#)ite.c	8.2 (Berkeley) 1/12/94
 */

/*
 * ite.c
 *
 * The ite module handles the system console; that is, stuff printed
 * by the kernel and by user programs while "desktop" and X aren't
 * running.  Some (very small) parts are based on hp300's 4.4 ite.c,
 * hence the above copyright.
 *
 *   -- Brad and Lawrence, June 26th, 1994
 *
 */

#include <vc.h>
#include <platforms.h>
#include <debug.h>

#include <mach_kdb.h>
#include <kern/spl.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <machine/machparam.h>          /* spl definitions */
#include <types.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>
#include <ppc/iso_font.h>
#include <ppc/new_screen.h>
#include <ppc/POWERMAC/video_console_entries.h>
#include <ppc/POWERMAC/video_console.h>
#include <ppc/POWERMAC/video_board.h>
#include <ppc/boot.h>
#include <ppc/io_map_entries.h>
#include <ppc/pmap.h>
#include <ppc/proc_reg.h>
#include <kern/lock.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/powermac_gestalt.h>
#include <ppc/POWERMAC/device_tree.h>

#define	DEFAULT_SPEED	9600
#define	DEFAULT_FLAGS	(TF_LITOUT|TF_ECHO)

#define	CHARWIDTH	8
#define	CHARHEIGHT	16

#define ATTR_NONE	0
#define ATTR_BOLD	1
#define ATTR_UNDER	2
#define ATTR_REVERSE	4

enum vt100state_e {
	ESnormal,		/* Nothing yet                             */
	ESesc,			/* Got ESC                                 */
	ESsquare,		/* Got ESC [				   */
	ESgetpars,		/* About to get or getting the parameters  */
	ESgotpars,		/* Finished getting the parameters         */
	ESfunckey,		/* Function key                            */
	EShash,			/* DEC-specific stuff (screen align, etc.) */
	ESsetG0,		/* Specify the G0 character set            */
	ESsetG1,		/* Specify the G1 character set            */
	ESask,
	EScharsize,
	ESignore		/* Ignore this sequence                    */
} vt100state = ESnormal;

boolean_t	vc_initted = FALSE;
boolean_t	vc_xservermode = FALSE;

extern struct video_board	pdm_video;
extern struct video_board	hpv_video;
extern struct video_board	av_video;
extern struct video_board	platinum_video;
extern struct video_board	control_video;
extern struct video_board	ati_video;
extern struct video_board	ims_video;
extern struct video_board	chips_video;
extern struct video_board	valkyrie_video;
extern struct video_board	ecsc_video;
extern struct video_board	dummy_video;

/* List of all known video boards */
struct video_board *video_boards[] = { &pdm_video,
				       &hpv_video, 
				       &av_video,
				       &platinum_video,
				       &control_video,
				       &ati_video,
				       &ims_video,
				       &chips_video,
				       &valkyrie_video,
				       &ecsc_video,
				       &dummy_video
				     };


int	video_board_count = sizeof(video_boards) / sizeof(video_boards[0]);

/* video board we're going to use */
struct video_board		*vboard = (struct video_board*)NULL;
struct vc_info			vinfo;

/* Calculated in vccninit(): */
static int vc_wrap_mode = 1, vc_relative_origin = 0;
static int vc_charset_select = 0, vc_save_charset_s = 0;
static int vc_charset[2] = { 0, 0 };
static int vc_charset_save[2] = { 0, 0 };

/* VT100 state: */
#define MAXPARS	16
static int x = 0, y = 0, savex, savey;
static int par[MAXPARS], numpars, hanging_cursor, attr, saveattr;

/* Our tty: */
struct tty vc_tty;

/* Misc */
void    vcstart(struct tty *tp);
void	vcstop(struct tty * tp, int flag);
int	vcmctl(struct tty* tp, int bits, int how);
void	vc_setcolor(struct vc_color *colors, int count);
void	vc_initialize(void);
void 	vc_flush_forward_buffer(void);
static void vc_store_char(unsigned char);
/* VT100 tab stops & scroll region */
static char tab_stops[255];
static int  scrreg_top, scrreg_bottom;

int	vc_probe(caddr_t port, void *ui);
void	vcattach(struct bus_device *dev);

extern void	keyboard_init(void);
extern void	keyboard_initialize_led_state(void);
io_return_t	keyboard_set_led_state(int state);
int		keyboard_get_led_state(void);


caddr_t	vc_std[NVC] = { (caddr_t) 0 };
struct bus_device	*vc_info[NVC];

struct bus_driver	vc_driver = {
	vc_probe, 0, vcattach, 0, vc_std, "vc", vc_info, 0, 0, 0 };

/* Boot Video Information */

Boot_Video	boot_video_info;

/*
 * For the color support (Michel Pollet)
 */
unsigned char vc_color_index_table[33] = 
	{  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	   1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 };

unsigned long vc_color_depth_masks[4] = 
	{ 0x000000FF, 0x00007FFF, 0x00FFFFFF };

unsigned long vc_colors[8][3] = {
	{ 0xFFFFFFFF, 0x00000000, 0x00000000 },	/* black */
	{ 0x23232323, 0x7C007C00, 0x00FF0000 },	/* red	*/
	{ 0xb9b9b9b9, 0x03e003e0, 0x0000FF00 },	/* green */
	{ 0x05050505, 0x7FE07FE0, 0x00FFFF00 },	/* yellow */
	{ 0xd2d2d2d2, 0x001f001f, 0x000000FF },	/* blue	 */
	{ 0x18181818, 0x7C1F7C1F, 0x00FF00FF },	/* magenta */
	{ 0xb4b4b4b4, 0x03FF03FF, 0x0000FFFF },	/* cyan	*/
	{ 0x00000000, 0x7FFF7FFF, 0x00FFFFFF }	/* white */
};

unsigned long vc_color_mask = 0;
unsigned long vc_color_fore = 0;
unsigned long vc_color_back = 0;
int vc_normal_background = 1;


/*
 * For the jump scroll and buffering (Michel Pollet)
 * 80*22 means on a 80*24 screen, the screen will
 * scroll jump almost a full screen
 * keeping only what's necessary for you to be able to read ;-)
 */
#define VC_MAX_FORWARD_SIZE	(80*22)

/*
 * Delay between console updates in clock hz units, the larger the
 * delay the fuller the jump-scroll buffer will be and so the faster the
 * (scrolling) output. The smaller the delay, the less jerky the
 * display. Heuristics show that at 10 touch-typists (Mike!) complain
 */
#define VC_CONSOLE_UPDATE_TIMEOUT	5

static unsigned char vc_forward_buffer[VC_MAX_FORWARD_SIZE];
static long vc_forward_buffer_size = 0;
static int vc_forward_buffer_enabled = 0;
decl_simple_lock_data(,vc_forward_lock)

/* 
 * New Rendering code from Michel Pollet
 */

/* That function will be called for drawing */
static void (*vc_paintchar) (unsigned char c, int x, int y, int attrs);

#ifdef RENDERALLOCATE
unsigned char *renderedFont = NULL;	/* rendered font buffer */
#else
#define REN_MAX_DEPTH	32
/* that's the size for a 32 bits buffer... */
#define REN_MAX_SIZE 	(128L*1024)
unsigned char renderedFont[REN_MAX_SIZE];
#endif

/* Rendered Font Size */
unsigned long vc_rendered_font_size = REN_MAX_SIZE;
long vc_rendered_error = 0;

/* If the one bit table was reversed */
short vc_one_bit_reversed = 0;

/* Size of a character in the table (bytes) */
int	vc_rendered_char_size = 0;

/*
# Attribute codes: 
# 00=none 01=bold 04=underscore 05=blink 07=reverse 08=concealed
# Text color codes:
# 30=black 31=red 32=green 33=yellow 34=blue 35=magenta 36=cyan 37=white
# Background color codes:
# 40=black 41=red 42=green 43=yellow 44=blue 45=magenta 46=cyan 47=white
*/
static void vc_color_set(int color)
{
	if (vinfo.v_depth < 8)
		return;
	if (color >= 30 && color <= 37)
		vc_color_fore = vc_colors[color-30][vc_color_index_table[vinfo.v_depth]];
	if (color >= 40 && color <= 47) {
		vc_color_back = vc_colors[color-40][vc_color_index_table[vinfo.v_depth]];
		vc_normal_background = color == 40;
	}

}

static void vc_render_font(short olddepth, short newdepth)
{
	int charIndex;	/* index in ISO font */
	union {
		unsigned char  *charptr;
		unsigned short *shortptr;
		unsigned long  *longptr;
	} current; 	/* current place in rendered font, multiple types. */

	unsigned char *theChar;	/* current char in iso_font */

	if (olddepth == newdepth && renderedFont) {
		return;	/* nothing to do */
	}
	
	if (olddepth != 1 && renderedFont) {
#ifdef RENDERALLOCATE
		(void) kmem_free(kernel_map, (vm_offset_t*)renderedFont, vc_rendered_font_size);
#endif
	}
	if (newdepth == 1) {
#ifdef RENDERALLOCATE
		renderedFont = iso_font;
#endif
		vc_rendered_char_size = 16;
		if (!vc_one_bit_reversed) {	/* reverse the font for the blitter */
			int i;
			for (i = 0; i < ((ISO_CHAR_MAX-ISO_CHAR_MIN+1) * vc_rendered_char_size); i++) {
				if (iso_font[i]) {
					unsigned char mask1 = 0x80;
					unsigned char mask2 = 0x01;
					unsigned char val = 0;
					while (mask1) { 
						if (iso_font[i] & mask1)
							val |= mask2;
						mask1 >>= 1;
						mask2 <<= 1;
					}
					renderedFont[i] = ~val;
				} else renderedFont[i] = 0xff;
			}
			vc_one_bit_reversed = 1;
		}
		return;
	}
	{
		long csize = newdepth / 8;	/* bytes per pixel */
		vc_rendered_char_size = csize ? CHARHEIGHT * (csize * CHARWIDTH) : 
				/* for 2 & 4 */	CHARHEIGHT * (CHARWIDTH/(6-newdepth));
		csize = (ISO_CHAR_MAX-ISO_CHAR_MIN+1) * vc_rendered_char_size;
#ifndef RENDERALLOCATE
		if (csize > vc_rendered_font_size) {
			vc_rendered_error = csize;
			return;
		} else 
			vc_rendered_font_size = csize;
#else
		vc_rendered_font_size = csize;
#endif
	}

#ifdef RENDERALLOCATE
	if (kmem_alloc(kernel_map,
		       (vm_offset_t *)&renderedFont,
		       vc_rendered_font_size) != KERN_SUCCESS) {
		renderedFont = NULL;
		vc_rendered_error = vc_rendered_font_size;
		return;
	}
#endif
	current.charptr = renderedFont;
	theChar = iso_font;
	for (charIndex = ISO_CHAR_MIN; charIndex <= ISO_CHAR_MAX; charIndex++) {
		int line;
		for (line = 0; line < CHARHEIGHT; line++) {
			unsigned char mask = 1;
			do {
				switch (newdepth) {
				case 2: {
					unsigned char value = 0;
					if (*theChar & mask) value |= 0xC0; mask <<= 1;
					if (*theChar & mask) value |= 0x30; mask <<= 1;
					if (*theChar & mask) value |= 0x0C; mask <<= 1;
					if (*theChar & mask) value |= 0x03;
					value = ~value;
					*current.charptr++ = value;
				}
					break;
				case 4:
				{
					unsigned char value = 0;
					if (*theChar & mask) value |= 0xF0; mask <<= 1;
					if (*theChar & mask) value |= 0x0F;
					value = ~value;
					*current.charptr++ = value;
				}
				break;
				case 8: 
					*current.charptr++ = (*theChar & mask) ? 0xff : 0;
					break;
				case 16:
					*current.shortptr++ = (*theChar & mask) ? 0xFFFF : 0;
					break;

				case 32: 
					*current.longptr++ = (*theChar & mask) ? 0xFFFFFFFF : 0;
					break;
				}
				mask <<= 1;
			} while (mask);	/* while the single bit drops to the right */
			theChar++;
		}
	}
}

static void vc_paint_char1(unsigned char ch, int xx, int yy, int attrs) 
{
	unsigned char *theChar;
	unsigned char *where;
	int i;
	
	theChar = (unsigned char*)(renderedFont + (ch * vc_rendered_char_size));
	where = (unsigned char*)(vinfo.v_baseaddr + 
				 (yy * CHARHEIGHT * vinfo.v_rowbytes) + 
				 (xx));

	if (!attrs) for (i = 0; i < CHARHEIGHT; i++) {	/* No attributes ? FLY !!!! */
		*where = *theChar++;
		
		where = (unsigned char*)(((unsigned char*)where)+vinfo.v_rowbytes);
	} else for (i = 0; i < CHARHEIGHT; i++) {	/* a little bit slower */
		unsigned char val = *theChar++, save = val;
		if (attrs & ATTR_BOLD) {		/* bold support */
			unsigned char mask1 = 0xC0, mask2 = 0x40;
			int bit = 0;
			for (bit = 0; bit < 7; bit++) {
				if ((save & mask1) == mask2)
					val &= ~mask2;
				mask1 >>= 1;
				mask2 >>= 1;
			}
		}
		if (attrs & ATTR_REVERSE) val = ~val;
		if (attrs & ATTR_UNDER &&  i == CHARHEIGHT-1) val = ~val;
		*where = val;
		
		where = (unsigned char*)(((unsigned char*)where)+vinfo.v_rowbytes);		
	}

}

static void vc_paint_char2(unsigned char ch, int xx, int yy, int attrs) 
{
	unsigned short *theChar;
	unsigned short *where;
	int i;

	theChar = (unsigned short*)(renderedFont + (ch * vc_rendered_char_size));
	where = (unsigned short*)(vinfo.v_baseaddr + 
				  (yy * CHARHEIGHT * vinfo.v_rowbytes) + 
				  (xx * 2));
	if (!attrs) for (i = 0; i < CHARHEIGHT; i++) {	/* No attributes ? FLY !!!! */
		*where = *theChar++;
		
		where = (unsigned short*)(((unsigned char*)where)+vinfo.v_rowbytes);
	} else for (i = 0; i < CHARHEIGHT; i++) {	/* a little bit slower */
		unsigned short val = *theChar++, save = val;
		if (attrs & ATTR_BOLD) {		/* bold support */
			unsigned short mask1 = 0xF000, mask2 = 0x3000;
			int bit = 0;
			for (bit = 0; bit < 7; bit++) {
				if ((save & mask1) == mask2)
					val &= ~mask2;
				mask1 >>= 2;
				mask2 >>= 2;
			}
		}
		if (attrs & ATTR_REVERSE) val = ~val;
		if (attrs & ATTR_UNDER &&  i == CHARHEIGHT-1) val = ~val;
		*where = val;
		
		where = (unsigned short*)(((unsigned char*)where)+vinfo.v_rowbytes);
	}

}

static void vc_paint_char4(unsigned char ch, int xx, int yy, int attrs) 
{
	unsigned long *theChar;
	unsigned long *where;
	int i;

	theChar = (unsigned long*)(renderedFont + (ch * vc_rendered_char_size));
	where = (unsigned long*)(vinfo.v_baseaddr + 
				 (yy * CHARHEIGHT * vinfo.v_rowbytes) + 
				 (xx * 4));

	if (!attrs) for (i = 0; i < CHARHEIGHT; i++) {	/* No attributes ? FLY !!!! */
		*where = *theChar++;
		
		where = (unsigned long*)(((unsigned char*)where)+vinfo.v_rowbytes);
	} else for (i = 0; i < CHARHEIGHT; i++) {	/* a little bit slower */
		unsigned long val = *theChar++, save = val;
		if (attrs & ATTR_BOLD) {		/* bold support */
			unsigned long mask1 = 0xff000000, mask2 = 0x0F000000;
			int bit = 0;
			for (bit = 0; bit < 7; bit++) {
				if ((save & mask1) == mask2)
					val &= ~mask2;
				mask1 >>= 4;
				mask2 >>= 4;
			}
		}
		if (attrs & ATTR_REVERSE) val = ~val;
		if (attrs & ATTR_UNDER &&  i == CHARHEIGHT-1) val = ~val;
		*where = val;
		
		where = (unsigned long*)(((unsigned char*)where)+vinfo.v_rowbytes);
	}

}

static void vc_paint_char8c(unsigned char ch, int xx, int yy, int attrs) 
{
	unsigned long *theChar;
	unsigned long *where;
	int i;
	
	theChar = (unsigned long*)(renderedFont + (ch * vc_rendered_char_size));
	where = (unsigned long*)(vinfo.v_baseaddr + 
					(yy * CHARHEIGHT * vinfo.v_rowbytes) + 
					(xx * CHARWIDTH));

	if (!attrs) for (i = 0; i < CHARHEIGHT; i++) {	/* No attr? FLY !*/
		unsigned long *store = where;
		int x;
		for (x = 0; x < 2; x++) {
			unsigned long val = *theChar++;
			val = (vc_color_back & ~val) | (vc_color_fore & val);
			*store++ = val;
		}
		
		where = (unsigned long*)(((unsigned char*)where)+vinfo.v_rowbytes);
	} else for (i = 0; i < CHARHEIGHT; i++) {	/* a little slower */
		unsigned long *store = where, lastpixel = 0;
		int x;
		for (x = 0 ; x < 2; x++) {
			unsigned long val = *theChar++, save = val;
			if (attrs & ATTR_BOLD) {	/* bold support */
				if (lastpixel && !(save & 0xFF000000))
					val |= 0xff000000;
				if ((save & 0xFFFF0000) == 0xFF000000)
					val |= 0x00FF0000;
				if ((save & 0x00FFFF00) == 0x00FF0000)
					val |= 0x0000FF00;
				if ((save & 0x0000FFFF) == 0x0000FF00)
					val |= 0x000000FF;
			}
			if (attrs & ATTR_REVERSE) val = ~val;
			if (attrs & ATTR_UNDER &&  i == CHARHEIGHT-1) val = ~val;

			val = (vc_color_back & ~val) | (vc_color_fore & val);
			*store++ = val;
			lastpixel = save & 0xff;
		}
		
		where = (unsigned long*)(((unsigned char*)where)+vinfo.v_rowbytes);		
	}

}
static void vc_paint_char16c(unsigned char ch, int xx, int yy, int attrs) 
{
	unsigned long *theChar;
	unsigned long *where;
	int i;
	
	theChar = (unsigned long*)(renderedFont + (ch * vc_rendered_char_size));
	where = (unsigned long*)(vinfo.v_baseaddr + 
				 (yy * CHARHEIGHT * vinfo.v_rowbytes) + 
				 (xx * CHARWIDTH * 2));

	if (!attrs) for (i = 0; i < CHARHEIGHT; i++) {	/* No attrs ? FLY ! */
		unsigned long *store = where;
		int x;
		for (x = 0; x < 4; x++) {
			unsigned long val = *theChar++;
			val = (vc_color_back & ~val) | (vc_color_fore & val);
			*store++ = val;
		}
		
		where = (unsigned long*)(((unsigned char*)where)+vinfo.v_rowbytes);
	} else for (i = 0; i < CHARHEIGHT; i++) { /* a little bit slower */
		unsigned long *store = where, lastpixel = 0;
		int x;
		for (x = 0 ; x < 4; x++) {
			unsigned long val = *theChar++, save = val;
			if (attrs & ATTR_BOLD) {	/* bold support */
				if (save == 0xFFFF0000) val |= 0xFFFF;
				else if (lastpixel && !(save & 0xFFFF0000))
					val |= 0xFFFF0000;
			}
			if (attrs & ATTR_REVERSE) val = ~val;
			if (attrs & ATTR_UNDER &&  i == CHARHEIGHT-1) val = ~val;

			val = (vc_color_back & ~val) | (vc_color_fore & val);

			*store++ = val;
			lastpixel = save & 0x7fff;
		}
		
		where = (unsigned long*)(((unsigned char*)where)+vinfo.v_rowbytes);		
	}

}
static void vc_paint_char32c(unsigned char ch, int xx, int yy, int attrs) 
{
	unsigned long *theChar;
	unsigned long *where;
	int i;
	
	theChar = (unsigned long*)(renderedFont + (ch * vc_rendered_char_size));
	where = (unsigned long*)(vinfo.v_baseaddr + 
					(yy * CHARHEIGHT * vinfo.v_rowbytes) + 
					(xx * CHARWIDTH * 4));

	if (!attrs) for (i = 0; i < CHARHEIGHT; i++) {	/* No attrs ? FLY ! */
		unsigned long *store = where;
		int x;
		for (x = 0; x < 8; x++) {
			unsigned long val = *theChar++;
			val = (vc_color_back & ~val) | (vc_color_fore & val);
			*store++ = val;
		}
		
		where = (unsigned long*)(((unsigned char*)where)+vinfo.v_rowbytes);
	} else for (i = 0; i < CHARHEIGHT; i++) {	/* a little slower */
		unsigned long *store = where, lastpixel = 0;
		int x;
		for (x = 0 ; x < 8; x++) {
			unsigned long val = *theChar++, save = val;
			if (attrs & ATTR_BOLD) {	/* bold support */
				if (lastpixel && !save)
					val = 0xFFFFFFFF;
			}
			if (attrs & ATTR_REVERSE) val = ~val;
			if (attrs & ATTR_UNDER &&  i == CHARHEIGHT-1) val = ~val;

			val = (vc_color_back & ~val) | (vc_color_fore & val);
			*store++ = val;
			lastpixel = save;
		}
		
		where = (unsigned long*)(((unsigned char*)where)+vinfo.v_rowbytes);		
	}

}

/*
 * That's a plain dumb reverse of the cursor position
 * It do a binary reverse, so it will not looks good when we have
 * color support. we'll see that later
 */
static void reversecursor(void)
{
	union {
		unsigned char  *charptr;
		unsigned short *shortptr;
		unsigned long  *longptr;
	} where;
	int line, col;
	
	where.longptr =  (unsigned long*)(vinfo.v_baseaddr + 
					(y * CHARHEIGHT * vinfo.v_rowbytes) + 
					(x /** CHARWIDTH*/ * vinfo.v_depth));
	for (line = 0; line < CHARHEIGHT; line++) {
		switch (vinfo.v_depth) {
			case 1:
				*where.charptr = ~*where.charptr;
				break;
			case 2:
				*where.shortptr = ~*where.shortptr;
				break;
			case 4:
				*where.longptr = ~*where.longptr;
				break;
/* that code still exists because since characters on the screen are
 * of different colors that reverse function may not work if the
 * cursor is on a character that is in a different color that the
 * current one. When we have buffering, things will work better. MP
 */
#ifdef VC_BINARY_REVERSE
			case 8:
				where.longptr[0] = ~where.longptr[0];
				where.longptr[1] = ~where.longptr[1];
				break;
			case 16:
				for (col = 0; col < 4; col++)
					where.longptr[col] = ~where.longptr[col];
				break;
			case 32:
				for (col = 0; col < 8; col++)
					where.longptr[col] = ~where.longptr[col];
				break;
#else
			case 8:
				for (col = 0; col < 8; col++)
					where.charptr[col] = where.charptr[col] != (vc_color_fore & vc_color_mask) ?
										vc_color_fore & vc_color_mask : vc_color_back & vc_color_mask;
				break;
			case 16:
				for (col = 0; col < 8; col++)
					where.shortptr[col] = where.shortptr[col] != (vc_color_fore & vc_color_mask) ?
										vc_color_fore & vc_color_mask : vc_color_back & vc_color_mask;
				break;
			case 32:
				for (col = 0; col < 8; col++)
					where.longptr[col] = where.longptr[col] != (vc_color_fore & vc_color_mask) ?
										vc_color_fore & vc_color_mask : vc_color_back & vc_color_mask;
				break;
#endif
		}
		where.charptr += vinfo.v_rowbytes;
	}
}


static void 
scrollup(int num)
{
	unsigned long *from, *to, linelongs, i, line, rowline, rowscanline;

	linelongs = vinfo.v_rowbytes * CHARHEIGHT / 4;
	rowline = vinfo.v_rowbytes / 4;
	rowscanline = vinfo.v_rowscanbytes / 4;

	to = (unsigned long *) vinfo.v_baseaddr + (scrreg_top * linelongs);
	from = to + (linelongs * num);	/* handle multiple line scroll (Michel Pollet) */

	i = (scrreg_bottom - scrreg_top) - num;

	while (i-- > 0) {
		for (line = 0; line < CHARHEIGHT; line++) {
			/*
			 * Only copy what is displayed
			 */
			video_scroll_up((unsigned int) from, 
					(unsigned int) (from+(vinfo.v_rowscanbytes/4)), 
					(unsigned int) to);

			from += rowline;
			to += rowline;
		}
	}

	/* Now set the freed up lines to the background colour */


	to = ((unsigned long *) vinfo.v_baseaddr + (scrreg_top * linelongs))
		+ ((scrreg_bottom - scrreg_top - num) * linelongs);

	for (linelongs = CHARHEIGHT * num;  linelongs-- > 0;) {
		from = to;
		for (i = 0; i < rowscanline; i++) 
			*to++ = vc_color_back;

		to = from + rowline;
	}

}

static void 
scrolldown(int num)
{
	unsigned long *from, *to,  linelongs, i, line, rowline, rowscanline;

	linelongs = vinfo.v_rowbytes * CHARHEIGHT / 4;
	rowline = vinfo.v_rowbytes / 4;
	rowscanline = vinfo.v_rowscanbytes / 4;


	to = (unsigned long *) vinfo.v_baseaddr + (linelongs * scrreg_bottom)
		- (rowline - rowscanline);
	from = to - (linelongs * num);	/* handle multiple line scroll (Michel Pollet) */

	i = (scrreg_bottom - scrreg_top) - num;

	while (i-- > 0) {
		for (line = 0; line < CHARHEIGHT; line++) {
			/*
			 * Only copy what is displayed
			 */
			video_scroll_down((unsigned int) from, 
					(unsigned int) (from-(vinfo.v_rowscanbytes/4)), 
					(unsigned int) to);

			from -= rowline;
			to -= rowline;
		}
	}

	/* Now set the freed up lines to the background colour */

	to = (unsigned long *) vinfo.v_baseaddr + (linelongs * scrreg_top);

	for (line = CHARHEIGHT * num; line > 0; line--) {
		from = to;

		for (i = 0; i < rowscanline; i++) 
			*(to++) = vc_color_back;

		to = from + rowline;
	}

}


static void 
clear_line(int which)
{
	int     start, end, i;

	/*
	 * This routine runs extremely slowly.  I don't think it's
	 * used all that often, except for To end of line.  I'll go
	 * back and speed this up when I speed up the whole vc
	 * module. --LK
	 */

	switch (which) {
	case 0:		/* To end of line	 */
		start = x;
		end = vinfo.v_columns-1;
		break;
	case 1:		/* To start of line	 */
		start = 0;
		end = x;
		break;
	case 2:		/* Whole line		 */
		start = 0;
		end = vinfo.v_columns-1;
		break;
	}

	for (i = start; i <= end; i++) {
		vc_paintchar(' ', i, y, ATTR_NONE);
	}

}

static void 
clear_screen(int which)
{
	unsigned long *p, *endp, *row;
	int      linelongs, col;
	int      rowline, rowlongs;

	rowline = vinfo.v_rowscanbytes / 4;
	rowlongs = vinfo.v_rowbytes / 4;

	p = (unsigned long*) vinfo.v_baseaddr;;
	endp = (unsigned long*) vinfo.v_baseaddr;

	linelongs = vinfo.v_rowbytes * CHARHEIGHT / 4;

	switch (which) {
	case 0:		/* To end of screen	 */
		clear_line(0);
		if (y < vinfo.v_rows - 1) {
			p += (y + 1) * linelongs;
			endp += rowlongs * vinfo.v_height;
		}
		break;
	case 1:		/* To start of screen	 */
		clear_line(1);
		if (y > 1) {
			endp += (y + 1) * linelongs;
		}
		break;
	case 2:		/* Whole screen		 */
		endp += rowlongs * vinfo.v_height;
		break;
	}

	for (row = p ; row < endp ; row += rowlongs) {
		for (col = 0; col < rowline; col++) 
			*(row+col) = vc_color_back;
	}

}

static void
reset_tabs(void)
{
	int i;

	for (i = 0; i<= vinfo.v_columns; i++) {
		tab_stops[i] = ((i % 8) == 0);
	}

}

static void
vt100_reset(void)
{
	reset_tabs();
	scrreg_top    = 0;
	scrreg_bottom = vinfo.v_rows;
	attr = ATTR_NONE;
	vc_charset[0] = vc_charset[1] = 0;
	vc_charset_select = 0;
	vc_wrap_mode = 1;
	vc_relative_origin = 0;
	vc_color_set(37);
	vc_color_set(40);	

}

static void 
putc_normal(unsigned char ch)
{
	switch (ch) {
	case '\a':		/* Beep			 */
#if 0
		asc_ringbell();
#else
	{
		unsigned long *ptr;
		int i, j;
		/* XOR the screen twice */
		for (i = 0; i < 2 ; i++) {
			/* For each row, xor the scanbytes */
			for (ptr = (unsigned long*)vinfo.v_baseaddr;
			     ptr < (unsigned long*)(vinfo.v_baseaddr +
				     (vinfo.v_height * vinfo.v_rowbytes));
			     ptr += (vinfo.v_rowbytes /
				     sizeof (unsigned long*)))
				for (j = 0;
				     j < vinfo.v_rowscanbytes /
					     sizeof (unsigned long*);
				     j++)
					*(ptr+j) =~*(ptr+j);
		}
	}
#endif
		break;
	case 127:		/* Delete		 */
	case '\b':		/* Backspace		 */
		if (hanging_cursor) {
			hanging_cursor = 0;
		} else
			if (x > 0) {
				x--;
			}
		break;
	case '\t':		/* Tab			 */
		while (x < vinfo.v_columns && !tab_stops[++x]);
		if (x >= vinfo.v_columns)
			x = vinfo.v_columns-1;
		break;
	case 0x0b:
	case 0x0c:
	case '\n':		/* Line feed		 */
		if (y >= scrreg_bottom -1 ) {
			scrollup(1);
			y = scrreg_bottom - 1;
		} else {
			y++;
		}
		break;
	case '\r':		/* Carriage return	 */
		x = 0;
		hanging_cursor = 0;
		break;
	case 0x0e:  /* Select G1 charset (Control-N) */
		vc_charset_select = 1;
		break;
	case 0x0f:  /* Select G0 charset (Control-O) */
		vc_charset_select = 0;
		break;
	case 0x18 : /* CAN : cancel */
	case 0x1A : /* like cancel */
			/* well, i do nothing here, may be later */
		break;
	case '\033':		/* Escape		 */
		vt100state = ESesc;
		hanging_cursor = 0;
		break;
	default:
		if (ch >= ' ') {
			if (hanging_cursor) {
				x = 0;
				if (y >= scrreg_bottom -1 ) {
					scrollup(1);
					y = scrreg_bottom - 1;
				} else {
					y++;
				}
				hanging_cursor = 0;
			}
			vc_paintchar((ch >= 0x60 && ch <= 0x7f) ? ch + vc_charset[vc_charset_select]
								: ch, x, y, attr);
			if (x == vinfo.v_columns - 1) {
				hanging_cursor = vc_wrap_mode;
			} else {
				x++;
			}
		}
		break;
	}

}

static void 
putc_esc(unsigned char ch)
{
	vt100state = ESnormal;

	switch (ch) {
	case '[':
		vt100state = ESsquare;
		break;
	case 'c':		/* Reset terminal 	 */
		vt100_reset();
		clear_screen(2);
		x = y = 0;
		break;
	case 'D':		/* Line feed		 */
	case 'E':
		if (y >= scrreg_bottom -1) {
			scrollup(1);
			y = scrreg_bottom - 1;
		} else {
			y++;
		}
		if (ch == 'E') x = 0;
		break;
	case 'H':		/* Set tab stop		 */
		tab_stops[x] = 1;
		break;
	case 'M':		/* Cursor up		 */
		if (y <= scrreg_top) {
			scrolldown(1);
			y = scrreg_top;
		} else {
			y--;
		}
		break;
	case '>':
		vt100_reset();
		break;
	case '7':		/* Save cursor		 */
		savex = x;
		savey = y;
		saveattr = attr;
		vc_save_charset_s = vc_charset_select;
		vc_charset_save[0] = vc_charset[0];
		vc_charset_save[1] = vc_charset[1];
		break;
	case '8':		/* Restore cursor	 */
		x = savex;
		y = savey;
		attr = saveattr;
		vc_charset_select = vc_save_charset_s;
		vc_charset[0] = vc_charset_save[0];
		vc_charset[1] = vc_charset_save[1];
		break;
	case 'Z':		/* return terminal ID */
		break;
	case '#':		/* change characters height */
		vt100state = EScharsize;
		break;
	case '(':
		vt100state = ESsetG0;
		break;
	case ')':		/* character set sequence */
		vt100state = ESsetG1;
		break;
	case '=':
		break;
	default:
		/* Rest not supported */
		break;
	}

}

static void
putc_askcmd(unsigned char ch)
{
	if (ch >= '0' && ch <= '9') {
		par[numpars] = (10*par[numpars]) + (ch-'0');
		return;
	}
	vt100state = ESnormal;

	switch (par[0]) {
		case 6:
			vc_relative_origin = ch == 'h';
			break;
		case 7:	/* wrap around mode h=1, l=0*/
			vc_wrap_mode = ch == 'h';
			break;
		default:
			break;
	}

}

static void
putc_charsizecmd(unsigned char ch)
{
	vt100state = ESnormal;

	switch (ch) {
		case '3' :
		case '4' :
		case '5' :
		case '6' :
			break;
		case '8' :	/* fill 'E's */
			{
				int xx, yy;
				for (yy = 0; yy < vinfo.v_rows; yy++)
					for (xx = 0; xx < vinfo.v_columns; xx++)
						vc_paintchar('E', xx, yy, ATTR_NONE);
			}
			break;
	}

}

static void
putc_charsetcmd(int charset, unsigned char ch)
{
	vt100state = ESnormal;

	switch (ch) {
		case 'A' :
		case 'B' :
		default:
			vc_charset[charset] = 0;
			break;
		case '0' :	/* Graphic characters */
		case '2' :
			vc_charset[charset] = 0x21;
			break;
	}

}

static void 
putc_gotpars(unsigned char ch)
{
	int     i;

	if (ch < ' ') {
		/* special case for vttest for handling cursor
		   movement in escape sequences */
		putc_normal(ch);
		vt100state = ESgotpars;
		return;
	}
	vt100state = ESnormal;
	switch (ch) {
	case 'A':		/* Up			 */
		y -= par[0] ? par[0] : 1;
		if (y < scrreg_top)
			y = scrreg_top;
		break;
	case 'B':		/* Down			 */
		y += par[0] ? par[0] : 1;
		if (y >= scrreg_bottom)
			y = scrreg_bottom - 1;
		break;
	case 'C':		/* Right		 */
		x += par[0] ? par[0] : 1;
		if (x >= vinfo.v_columns)
			x = vinfo.v_columns-1;
		break;
	case 'D':		/* Left			 */
		x -= par[0] ? par[0] : 1;
		if (x < 0)
			x = 0;
		break;
	case 'H':		/* Set cursor position	 */
	case 'f':
		x = par[1] ? par[1] - 1 : 0;
		y = par[0] ? par[0] - 1 : 0;
		if (vc_relative_origin)
			y += scrreg_top;
		hanging_cursor = 0;
		break;
	case 'X':		/* clear p1 characters */
		if (numpars) {
			int i;
			for (i = x; i < x + par[0]; i++)
				vc_paintchar(' ', i, y, ATTR_NONE);
		}
		break;
	case 'J':		/* Clear part of screen	 */
		clear_screen(par[0]);
		break;
	case 'K':		/* Clear part of line	 */
		clear_line(par[0]);
		break;
	case 'g':		/* tab stops	 	 */
		switch (par[0]) {
			case 1:
			case 2:	/* reset tab stops */
				/* reset_tabs(); */
				break;				
			case 3:	/* Clear every tabs */
				{
					int i;

					for (i = 0; i <= vinfo.v_columns; i++)
						tab_stops[i] = 0;
				}
				break;
			case 0:
				tab_stops[x] = 0;
				break;
		}
		break;
	case 'm':		/* Set attribute	 */
		for (i = 0; i < numpars; i++) {
			switch (par[i]) {
			case 0:
				attr = ATTR_NONE;
				vc_color_set(37);
				vc_color_set(40);	
				break;
			case 1:
				attr |= ATTR_BOLD;
				break;
			case 4:
				attr |= ATTR_UNDER;
				break;
			case 7:
				attr |= ATTR_REVERSE;
				break;
			case 22:
				attr &= ~ATTR_BOLD;
				break;
			case 24:
				attr &= ~ATTR_UNDER;
				break;
			case 27:
				attr &= ~ATTR_REVERSE;
				break;
			case 5:
			case 25:	/* blink/no blink */
				break;
			default:
				vc_color_set(par[i]);
				break;
			}
		}
		break;
	case 'r':		/* Set scroll region	 */
		x = y = 0;
		/* ensure top < bottom, and both within limits */
		if ((numpars > 0) && (par[0] < vinfo.v_rows)) {
			scrreg_top = par[0] ? par[0] - 1 : 0;
			if (scrreg_top < 0)
				scrreg_top = 0;
		} else {
			scrreg_top = 0;
		}
		if ((numpars > 1) && (par[1] <= vinfo.v_rows) && (par[1] > par[0])) {
			scrreg_bottom = par[1];
			if (scrreg_bottom > vinfo.v_rows)
				scrreg_bottom = vinfo.v_rows;
		} else {
			scrreg_bottom = vinfo.v_rows;
		}
		if (vc_relative_origin)
			y = scrreg_top;
		break;
	}

}

static void 
putc_getpars(unsigned char ch)
{
	if (ch == '?') {
		vt100state = ESask;
		return;
	}
	if (ch == '[') {
		vt100state = ESnormal;
		/* Not supported */
		return;
	}
	if (ch == ';' && numpars < MAXPARS - 1) {
		numpars++;
	} else
		if (ch >= '0' && ch <= '9') {
			par[numpars] *= 10;
			par[numpars] += ch - '0';
		} else {
			numpars++;
			vt100state = ESgotpars;
			putc_gotpars(ch);
		}
}

static void 
putc_square(unsigned char ch)
{
	int     i;

	for (i = 0; i < MAXPARS; i++) {
		par[i] = 0;
	}

	numpars = 0;
	vt100state = ESgetpars;

	putc_getpars(ch);

}

void 
vc_putchar(char ch)
{
	if (!ch) {
		return;	/* ignore null characters */
	}
	switch (vt100state) {
		default:vt100state = ESnormal;	/* FALLTHROUGH */
	case ESnormal:
		putc_normal(ch);
		break;
	case ESesc:
		putc_esc(ch);
		break;
	case ESsquare:
		putc_square(ch);
		break;
	case ESgetpars:
		putc_getpars(ch);
		break;
	case ESgotpars:
		putc_gotpars(ch);
		break;
	case ESask:
		putc_askcmd(ch);
		break;
	case EScharsize:
		putc_charsizecmd(ch);
		break;
	case ESsetG0:
		putc_charsetcmd(0, ch);
		break;
	case ESsetG1:
		putc_charsetcmd(1, ch);
		break;
	}

	if (x >= vinfo.v_columns) {
		x = vinfo.v_columns - 1;
	}
	if (x < 0) {
		x = 0;
	}
	if (y >= vinfo.v_rows) {
		y = vinfo.v_rows - 1;
	}
	if (y < 0) {
		y = 0;
	}

}

/*
 * Tty handling functions
 */

extern const char * getenv(const char *name);
static boolean_t probe_video;

int
vc_probe(caddr_t port, void *ui)
{

	int i;

	/* probe is called right at system startup time and again
	 * during the normal device probe routines. We don't need
	 * to clear the screen etc. on the second occasion, but
	 * must call the card's init routine as the card may need
	 * to map some virtual addresses which it couldn't do the
	 * first time it was called
	 */

	/* Try to probe if we have such a routine */
	for (i = 0; i < video_board_count; i++) {
		if (video_boards[i]->v_probe == NULL)
			continue;

		if (video_boards[i]->v_probe((caddr_t)boot_video_info.v_baseAddr)) {
			vboard = video_boards[i];
			break;
		}
	}		

  	if (vboard == (struct video_board*)NULL) {
		/* probing failed, so just guess the board */
		switch (powermac_info.class) {
		case	POWERMAC_CLASS_PDM:
			if (boot_video_info.v_baseAddr >=
		    		(unsigned long ) 0xF0000000)
				vboard = &hpv_video;
			else if ((boot_video_info.v_baseAddr &
				  (unsigned long) 0xFFF00000) ==
				 (unsigned long) 0xE0100000)
				vboard = &av_video;
			else
				vboard = &pdm_video;
			break;

		default:
			vboard = &dummy_video;
			break;
		}
	}

	vboard->v_init(&vinfo);

	vinfo.v_rows = vinfo.v_height / CHARHEIGHT;
	vinfo.v_columns = vinfo.v_width / CHARWIDTH;

	if (vinfo.v_depth >= 8) {
		vinfo.v_rowscanbytes = (vinfo.v_depth / 8) * vinfo.v_width;
	} else {
		vinfo.v_rowscanbytes = vinfo.v_width / (8 / vinfo.v_depth);
	}

	if (!vc_initted)
		vc_initialize();
	vc_initted = TRUE;

	return 1;
}

void
vc_initialize(void)
{
	vc_render_font(1, vinfo.v_depth);
	vc_color_mask = vc_color_depth_masks[vc_color_index_table[vinfo.v_depth]];
	vt100_reset();
	switch (vinfo.v_depth) {
	default:
	case 1:
		vc_paintchar = vc_paint_char1;
		break;
	case 2:
		vc_paintchar = vc_paint_char2;
		break;
	case 4:
		vc_paintchar = vc_paint_char4;
		break;
	case 8:
		vc_paintchar = vc_paint_char8c;
		break;
	case 16:
		vc_paintchar = vc_paint_char16c;
		break;
	case 32:
		vc_paintchar = vc_paint_char32c;
		break;
	}


	reversecursor();
	clear_screen(2);
	reversecursor();

}

/*
 * Actually draws the buffer, handle the jump scroll
 */
void vc_flush_forward_buffer(void)
{
	spl_t s;

	if (vc_forward_buffer_enabled) {
		s = spltty();
		simple_lock(&vc_forward_lock);
	}

	if (vc_forward_buffer_size) {
		int start = 0;
		reversecursor();
		do {
			int i;
			int plaintext = 1;
			int drawlen = start;
			int jump = 0;
			int param = 0, changebackground = 0;
			enum vt100state_e vtState = vt100state;
			/* 
			 * In simple words, here we're pre-parsing the text to look for
			 *  + Newlines, for computing jump scroll
			 *  + /\033\[[0-9;]*]m/ to continue on
			 * any other sequence will stop. We don't want to have cursor
			 * movement escape sequences while we're trying to pre-scroll
			 * the screen.
			 * We have to be extra carefull about the sequences that changes
			 * the background color to prevent scrolling in those 
			 * particular cases.
			 * That parsing was added to speed up 'man' and 'color-ls' a 
			 * zillion time (at least). It's worth it, trust me. 
			 * (mail Nick Stephen for a True Performance Graph)
			 * Michel Pollet
			 */
			for (i = start; i < vc_forward_buffer_size && plaintext; i++) {
				drawlen++;
				switch (vtState) {
					case ESnormal:
						switch (vc_forward_buffer[i]) {
							case '\033':
								vtState = ESesc;
								break;
							case '\n':
								jump++;
								break;
						}
						break;
					case ESesc:
						switch (vc_forward_buffer[i]) {
							case '[':
								vtState = ESgetpars;
								param = 0;
								changebackground = 0;
								break;
							default:
								plaintext = 0;
								break;
						}
						break;
					case ESgetpars:
						if ((vc_forward_buffer[i] >= '0' &&
						    vc_forward_buffer[i] <= '9') ||
						    vc_forward_buffer[i] == ';') {
							if (vc_forward_buffer[i] >= '0' &&
						    	    vc_forward_buffer[i] <= '9')
								param = (param*10)+(vc_forward_buffer[i]-'0');
							else {
								if (param >= 40 && param <= 47)
									changebackground = 1;
								if (!vc_normal_background &&
								    !param)
									changebackground = 1;
								param = 0;
							}
							break; /* continue on */
						}
						vtState = ESgotpars;
						/* fall */
					case ESgotpars:
						switch (vc_forward_buffer[i]) {
							case 'm':
								vtState = ESnormal;
								if (param >= 40 && param <= 47)
									changebackground = 1;
								if (!vc_normal_background &&
								    !param)
									changebackground = 1;
								if (changebackground) {
									plaintext = 0;
									jump = 0;
									/* REALLY don't jump */
								}
								/* Yup ! we've got it */
								break;
							default:
								plaintext = 0;
								break;
						}
						break;
					default:
						plaintext = 0;
						break;
				}
				
			}

			/*
			 * Then we look if it would be appropriate to forward jump
			 * the screen before drawing
			 */
			if (jump && (scrreg_bottom - scrreg_top) > 2) {
				jump -= scrreg_bottom - y - 1;
				if (jump > 0 ) {
					if (jump >= scrreg_bottom - scrreg_top)
						jump = scrreg_bottom - scrreg_top -1;
					y -= jump;
					scrollup(jump);
				}
			}
			/*
			 * and we draw what we've found to the parser
			 */
			for (i = start; i < drawlen; i++)
				vc_putchar(vc_forward_buffer[start++]);
			/*
			 * Continue sending characters to the parser until we're sure we're
			 * back on normal characters.
			 */
			for (i = start; i < vc_forward_buffer_size &&
					vt100state != ESnormal ; i++)
				vc_putchar(vc_forward_buffer[start++]);
			/* Then loop again if there still things to draw */
		} while (start < vc_forward_buffer_size);
		vc_forward_buffer_size = 0;
		reversecursor();
	}

	if (vc_forward_buffer_enabled) {
		simple_unlock(&vc_forward_lock);
		splx(s);
	}
}


/* 
 * Immediate character display.. kernel printf uses this. Make sure
 * pre-clock printfs get flushed and that panics get fully displayed.
 */

int
vcputc(int l, int u, int c)
{
	vc_store_char(c);

	if (vc_forward_buffer_enabled) {
		/*
		 * Force a draw. If we can't cancel the timeout, then the
		 * timeout has occurred already so we shouldn't need to do
		 * anything.
		 */
		if (untimeout((timeout_fcn_t)vc_flush_forward_buffer,
			      (void *)0)) {
			vc_flush_forward_buffer();
		}
	} else {
		/* no timeout to cancel, just flush the buffer */
		vc_flush_forward_buffer();
	}
	/* make sure we HAVE cleaned buffer */
	assert(vc_forward_buffer_size < VC_MAX_FORWARD_SIZE);

	return 0;
}

/*
 * Store characters to be drawn 'later', handle overflows
 */

static void
vc_store_char(unsigned char c)
{
	spl_t	s;

	/* Either we're really buffering stuff or we're not yet because
	 * the probe hasn't been done. If we're not, then we can only
	 * ever have a maximum of one character in the buffer waiting to
	 * be flushed
	 */
	assert(vc_forward_buffer_enabled || (vc_forward_buffer_size == 0));

	if (vc_forward_buffer_enabled) {
		s = spltty();
		simple_lock(&vc_forward_lock);
	}
	vc_forward_buffer[vc_forward_buffer_size++] = (unsigned char)c;

	if (vc_forward_buffer_enabled) {
		switch (vc_forward_buffer_size) {
		case 1:
			/* If we're adding the first character to the buffer,
			 * start the timer, otherwise it is already running.
			 */
			timeout((timeout_fcn_t)vc_flush_forward_buffer,
				(void *)0,
				VC_CONSOLE_UPDATE_TIMEOUT);
			break;
		case VC_MAX_FORWARD_SIZE:
			/*
			 * If there is an overflow, then we force a draw.
			 * If we can't cancel the timeout, then the
			 * timeout has occurred since the switch so
			 * we don't need to do anything.
			 */
			if (untimeout((timeout_fcn_t)vc_flush_forward_buffer,
				      (void *)0)) {
				simple_unlock(&vc_forward_lock);
				splx(s);
				vc_flush_forward_buffer();
				s = spltty();
				simple_lock(&vc_forward_lock);
			}
			/* make sure timout HAS cleaned buffer */
			assert(vc_forward_buffer_size <
			       VC_MAX_FORWARD_SIZE);
			break;
		default:
			/*
			 * the character will be flushed on timeout
			 */
			break;
		}
		simple_unlock(&vc_forward_lock);
		splx(s);
	}

}

void 
vcattach(struct bus_device * device)
{

	simple_lock_init(&vc_tty.t_lock, ETAP_IO_TTY);
	vc_tty.t_ispeed = DEFAULT_SPEED;
	vc_tty.t_ospeed = DEFAULT_SPEED;
	vc_tty.t_flags = TF_LITOUT|TF_ECHO|TF_CRMOD;
	vc_tty.t_dev = 0;
	ttychars(&vc_tty);
	keyboard_init();
	printf("\nvc0: ");
	if (vinfo.v_depth >= 8)
		printf("\033[31mC\033[32mO\033[33mL\033[34mO\033[35mR\033[37m ");
	printf("video console at 0x%x (%dx%dx%d) \"%s\"\n", vinfo.v_baseaddr,
		vinfo.v_width, vinfo.v_height,  vinfo.v_depth, vinfo.v_name);

	/* Map the address since motherboard video is
	 * already mapped. We use io_map to give
	 * us some valid kernel vm addresses.
	 */
	if ((vinfo.v_type != VC_TYPE_MOTHERBOARD) &&
	    (vinfo.v_baseaddr != 0)) {

		kern_return_t kr;
		vm_offset_t start;
		vm_offset_t size;

		/* add the video memory as mappable memory, since the
		 * video supports device_map
		 */
		kr = pmap_add_physical_memory(vinfo.v_baseaddr,
					      vinfo.v_baseaddr+
					      (vinfo.v_height *
					       vinfo.v_rowbytes),
					      FALSE,
					      PTE_WIMG_IO);
		assert(kr == KERN_SUCCESS);

		/* remap the v_baseaddr to a valid kernel virtual address
		 * to remove need for segment register mapping. We must
		 * map whole pages, even if the address isn't aligned.
		 */

		/* Steal some free VM addresses */

		size = round_page((vinfo.v_baseaddr & PAGE_MASK) + 
				  (vinfo.v_height * vinfo.v_rowbytes));

		(void) kmem_alloc_pageable(kernel_map, &start, size);

		pmap_map(start,
			 trunc_page(vinfo.v_baseaddr),
			 trunc_page(vinfo.v_baseaddr)+size,
			 VM_PROT_READ|VM_PROT_WRITE);

		/* Keep any non-alignment of baseaddr */
		vinfo.v_baseaddr = start + (vinfo.v_baseaddr & PAGE_MASK);

	}

	/*
	 * Added for the buffering and jump scrolling 
	 */
	/* Init our lock */
	simple_lock_init(&vc_forward_lock, ETAP_IO_TTY);

	vc_forward_buffer_enabled = 1;

	/* Register any interrupts if needed */
	if (vboard->v_register_int)
		vboard->v_register_int(&vinfo);
}

io_return_t
vcopen(dev_t dev, dev_mode_t flag, io_req_t ior)
{
	register struct tty *tp;
	io_return_t	retval;
	boolean_t	init_leds;

	tp = &vc_tty;

	tp->t_start = vcstart;
	tp->t_stop = vcstop;
	tp->t_mctl = vcmctl;
	tp->t_state |= TS_CARR_ON;

	if ((tp->t_state & TS_ISOPEN) == 0) {
		if (tp->t_ispeed == 0) {
			tp->t_flags = DEFAULT_FLAGS;
			tp->t_ospeed = DEFAULT_SPEED;
			tp->t_ispeed = DEFAULT_SPEED;
		}
		init_leds = TRUE;
	} else {
		init_leds = FALSE;
	}

	retval = char_open(dev, tp, flag, ior);

	if (init_leds) {
		keyboard_initialize_led_state();
	}

	return retval;
}

void
vcclose(dev_t dev)
{
	int s;

	s = spltty();
	simple_lock(&vc_tty.t_lock);
	ttyclose(&vc_tty);
	simple_unlock(&vc_tty.t_lock);
	splx(s);
}

io_return_t
vcread(dev_t dev, io_req_t ior)
{
	io_return_t	retval;

	retval = char_read(&vc_tty, ior);

	return retval;
}

io_return_t
vcwrite(dev_t dev, io_req_t ior)
{
	io_return_t	retval;
	retval = char_write(&vc_tty, ior);
	return retval;
}

int
vcmctl(struct tty *tp, int bits, int how)
{
	return	0;
}

vm_offset_t
vcmmap(
	dev_t		dev,
	vm_offset_t	off,
	vm_prot_t	prot)
{
    vm_offset_t		video_size;
    vm_offset_t		retval;

    /* Get page frame number for the page to be mapped. */
    video_size = ppc_round_page((vinfo.v_physaddr & PAGE_MASK) +
				(vinfo.v_rowbytes * vinfo.v_height));

    if (off >= video_size) {
	    printf("VIDEO CONSOLE DRIVER OVER MAP! Offset %d, size %d\n",
		   off, video_size);
	    off = 0;
    }

    /* Return physical address of page */
    retval = btop(trunc_page(vinfo.v_physaddr)+off);

    return retval;
}

void 
vcstart(register struct tty * tp)
{
	register int cc, s;
	int     hiwat = 0, hadcursor = 0;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
		splx(s);
		return;
	}

	if (tp->t_outq.c_cc == 0) {
		tt_write_wakeup(tp);
		splx(s);
		return;
	}

	while (tp->t_outq.c_cc > 0) {
		tp->t_state |= TS_BUSY;

		splx(s);
		while (tp->t_outq.c_cc > 0) 
			vc_store_char(getc(&tp->t_outq));

		s = spltty();
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;

		tt_write_wakeup(tp);

		if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) 
			break;
	}
	splx(s);

}

void 
vcstop(struct tty * tp, int flag)
{
	int     s;
	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state & TS_TTSTOP) == 0) {
			tp->t_state |= TS_FLUSH;
		}
	}
	splx(s);

}

void
screen_put_char(unsigned char c)
{
	vcputc(0,0,c);
}

void
initialize_screen(void * boot_args)
{
	memcpy(&boot_video_info, (unsigned char *) boot_args, sizeof(boot_video_info));
	vc_probe(0, 0);
}

boolean_t
vcportdeath(dev_t dev, ipc_port_t port)
{
	boolean_t	retval;
	retval = tty_portdeath(&vc_tty, port);
	return retval;
}

io_return_t
vcgetstatus(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	mach_msg_type_number_t *status_count)
{
	struct vc_info *vc = (struct vc_info *) data;
	io_return_t	retval;
	switch (flavor) {
	case	VC_GETINFO:
		if (*status_count > sizeof(struct vc_info) / sizeof (int))
			*status_count = sizeof(struct vc_info) / sizeof (int);

		memcpy(vc, &vinfo, *status_count * sizeof (int));
		retval = D_SUCCESS;
		break;

	default:
		retval = tty_get_status(&vc_tty, flavor, data, status_count);
		break;
	}

	return retval;
}

io_return_t
vcsetstatus(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	mach_msg_type_number_t status_count)
{
	io_return_t	ret;
	int		count;
	switch (flavor) {
	case	VC_SETCOLOR:
	{
		int cnt,i;
		int losum,hisum,tmpsum;
		struct vc_color *ptr;
		/* Find new colors for console foreground + background */
		cnt = (status_count * sizeof (int)) / sizeof(struct vc_color);
		if (cnt) {
			vc_colors[0][0] = 0;
			vc_colors[7][0] = 0;
			ptr = (struct vc_color *)data;
			losum = ptr->vp_red + ptr->vp_green + ptr->vp_blue;
			hisum = losum;
			for (i = 0; i < cnt ; i++,ptr++) {
				tmpsum = ptr->vp_red +
					ptr->vp_green +
					ptr->vp_blue;
				if (tmpsum <= losum) {
					losum = tmpsum;
					vc_colors[0][0] = i | (i << 8) ;
					vc_colors[0][0] |= vc_colors[0][0]<<16;
				}
				if (tmpsum >= hisum) {
					hisum = tmpsum;
					vc_colors[7][0] = i | (i << 8) ;
					vc_colors[7][0] |= vc_colors[7][0]<<16;
				}
			}
			vc_color_fore = vc_colors[7][0];
			vc_color_back = vc_colors[0][0];
		}
		ret = vboard->v_setcolor((status_count * sizeof (int))
					 / sizeof(struct vc_color),
					 (struct vc_color *) data);
		break;
	}
	case	VC_SETVIDEOMODE:
		ret = vboard->v_setmode((struct vc_info *) data);

		if (ret == D_SUCCESS) {
			vboard->v_getmode((struct vc_info *)&vinfo);
			vc_initialize();
		}
		break;

	case	VC_SETXMODE:
		/* flush any outstanding events */
		ttyflush(&vc_tty, D_READ);
		vc_xservermode = TRUE;
		ret = D_SUCCESS;
		break;

	case	VC_SETNORMAL:
		/* flush any outstanding events */
		ttyflush(&vc_tty, D_READ);
		vc_xservermode = FALSE;
		ret = D_SUCCESS;
		break;

	case	VC_GETKEYBOARDLEDS:
		if (status_count != 1) {
			ret = D_INVALID_SIZE;
			break;
		}
		*(int*)data = keyboard_get_led_state();
		ret = D_SUCCESS;
		break;

	case	VC_SETKEYBOARDLEDS:
		if (status_count != 1) {
			ret = D_INVALID_SIZE;
			break;
		}
		/* only allow setting of state if we're a raw keyboard */
		if (vc_xservermode == FALSE) {
			ret = D_READ_ONLY;
			break;
		}
		ret = keyboard_set_led_state(*(int*)data);
		break;

	case	TTY_DRAIN:
		/* We buffer information beyond that from the tty queue,
		 * so we must flush the tty queue then flush our buffer
		 */

		tty_set_status(&vc_tty, flavor, data, status_count);

		if (untimeout((timeout_fcn_t)vc_flush_forward_buffer,
			      (void *)0)) {
			vc_flush_forward_buffer();
		}
		ret = D_SUCCESS;
		break;

	default:
		ret = tty_set_status(&vc_tty, flavor, data, status_count);
		break;
	}

	return ret;
}
