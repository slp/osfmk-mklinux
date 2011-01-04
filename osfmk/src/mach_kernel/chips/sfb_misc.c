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
 * 	Flamingo, protos.
 * 	[93/02/04  01:39:27  af]
 * 
 * 	Created.
 * 	[92/12/10  15:44:58  af]
 */
/* CMU_ENDHIST */
/*
 *	File: sfb_misc.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date: 11/92
 *
 *	Driver for the PMAGB-BA smart color framebuffer
 *
 */

#include <sfb.h>
#if	(NSFB > 0)
#include <platforms.h>

/*
 * NOTE: This driver relies heavily on the pm one, as well as the cfb.
 */

#include <device/device_types.h>
#include <chips/screen_defs.h>
#include <chips/pm_defs.h>
typedef pm_softc_t	sfb_softc_t;

#include <chips/bt459.h>
#define	bt459		cursor_registers

#ifdef	DECSTATION
#include <mips/PMAX/pmagb_ba.h>
#endif

#ifdef	FLAMINGO
#include <alpha/DEC/pmagb_ba.h>
#endif

/*
 * Initialize color map, for kernel use
 */
#define	sfb_init_colormap		cfb_init_colormap

/*
 * Position cursor
 */
sfb_pos_cursor(
	bt459_regmap_t	*regs,
	int		x,
	int		y)
{
	bt459_pos_cursor( regs, x + 368 - 219, y + 37 - 34);
}

/*
 * Large viz small cursor
 */
#define	sfb_small_cursor_to_large	cfb_small_cursor_to_large

/*
 * Device-specific set status
 */
#define sfb_set_status			cfb_set_status

/*
 * Hardware initialization
 */
sfb_init_screen(
	sfb_softc_t *sfb)
{
	bt459_init( sfb->bt459,
		    sfb->bt459 + (SFB_OFFSET_RESET - SFB_OFFSET_BT459),
		    4 /* 4:1 MUX */);
}

/*
 * Do what's needed when X exits
 */
sfb_soft_reset(
	screen_softc_t	sc)
{
	sfb_softc_t	*sfb = (sfb_softc_t*) sc->hw_state;
	user_info_t	*up =  sc->up;
	extern cursor_sprite_t	dc503_default_cursor;

	/*
	 * Restore params in mapped structure
	 */
	pm_init_screen_params(sc,up);
	up->row = up->max_row - 1;

	up->dev_dep_2.pm.x26 = 2; /* you do not want to know */
	up->dev_dep_1.pm.x18 = (short*)2;

	/*
	 * Restore RAMDAC chip to default state
	 */
	sfb_init_screen(sfb);

	/*
	 * Load kernel's cursor sprite: just use the same pmax one
	 */
	sfb_small_cursor_to_large(up, dc503_default_cursor);
	bt459_cursor_sprite(sfb->bt459, up->dev_dep_2.pm.cursor_sprite);

	/*
	 * Color map and cursor color
	 */
	sfb_init_colormap(sc);
}


#endif	/* (NSFB > 0) */
