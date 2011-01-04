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
 * Revision 2.2  93/01/14  17:22:30  danner
 * 	Created.
 * 	[92/11/25            jtp]
 */
/* CMU_ENDHIST */
/*
 * 	File: xcfb_monitor.h
 *	Author: Jukka Partanen, Helsinki University of Technology 1992.
 *
 *	A type describing the physical properties of a monitor
 */

#ifndef _XCFB_MONITOR_H_
#define _XCFB_MONITOR_H_

typedef struct xcfb_monitor_type {
	char *name;
	short frame_visible_width; /* pixels */
	short frame_visible_height;
	short frame_scanline_width;
	short frame_height;
	short half_sync;	/* screen units (= 4 pixels) */
	short back_porch;
	short v_sync;		/* lines */
	short v_pre_equalize;
	short v_post_equalize;
	short v_blank;
	short line_time;	/* screen units */
	short line_start;
	short mem_init;		/* some units */
	short xfer_delay;
} *xcfb_monitor_type_t;

#endif /* _XCFB_MONITOR_H_ */
