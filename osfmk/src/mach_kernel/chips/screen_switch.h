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
 * Revision 2.6  91/06/19  11:54:28  rvb
 * 	File moved here from mips/PMAX since it tries to be generic;
 * 	it is used on the PMAX and the Vax3100.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.5  91/05/14  17:27:50  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:44:30  mrt
 * 	Added author notices
 * 	[91/02/04  11:17:53  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:16:32  mrt]
 * 
 * Revision 2.3  90/12/05  23:34:34  af
 * 
 * 
 * Revision 2.1.1.1  90/11/01  03:41:45  af
 * 	Re-created from pm_switch.h for new consistent naming scheme.
 * 	[90/10/04            af]
 * 
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
 *	File: screen_switch.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	10/90
 *
 *	Definitions of things that must be tailored to
 *	specific hardware boards for the Generic Screen Driver.
 */

#ifndef	SCREEN_SWITCH_H
#define	SCREEN_SWITCH_H	1

#include <mach/boolean.h>

/*
 *	List of probe routines, scanned at cold-boot time
 *	to see which, if any, graphic display is available.
 *	This is done before autoconf, so that printing on
 *	the console works early on.  The alloc routine is
 *	called only on the first device that answers.
 *	Ditto for the setup routine, called later on.
 */
struct screen_probe_vector {
	int		(*probe)(void);
	unsigned int	(*alloc)(void);
	int		(*setup)(int, user_info_t);
};

/*
 *	Low-level operations on the graphic device, used
 *	by the otherwise device-independent interface code
 */

/* Forward declaration of screen_softc_t */
typedef struct screen_softc *screen_softc_t;

struct screen_switch {
	int	(*graphic_open)(void);			/* when X11 opens */
	int	(*graphic_close)(screen_softc_t);	/* .. or closes */
	int	(*set_status)(screen_softc_t,
			      dev_flavor_t,
			      dev_status_t,
			      natural_t);		/* dev-specific ops */
	int	(*get_status)(screen_softc_t,
			      dev_flavor_t,
			      dev_status_t,
			      natural_t*);		/* dev-specific ops */
	int	(*char_paint)(screen_softc_t,
			      int,
			      int,
			      int);			/* blitc */
	int	(*pos_cursor)(void*,
			      int,
			      int);			/* cursor positioning*/
	int	(*insert_line)(screen_softc_t,
			       short);			/* ..and scroll down */
	int	(*remove_line)(screen_softc_t,
			       short);			/* ..and scroll up */
	int	(*clear_bitmap)(screen_softc_t);	/* blank screen */
	int	(*video_on)(void*,
			    user_info_t*);		/* screen saver */
	int	(*video_off)(void*,
			     user_info_t*);
	int	(*intr_enable)(void*,
			       boolean_t);
	int	(*map_page)(screen_softc_t,
			    vm_offset_t,
			    int);			/* user-space mapping*/
};

/*
 *	Each graphic device needs page-aligned memory
 *	to be mapped in user space later (for events
 *	and such).  Size and content of this memory
 *	is unfortunately device-dependent, even if
 *	it did not need to (puns).
 */
extern char  *screen_data;

extern struct screen_probe_vector screen_probe_vector[];

extern int screen_noop(void), screen_find(void);

#endif	/* SCREEN_SWITCH_H */
