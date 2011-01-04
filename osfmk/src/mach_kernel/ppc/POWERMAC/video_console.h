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

#ifndef _POWERMAC_VIDEO_CONSOLE_H_
#define _POWERMAC_VIDEO_CONSOLE_H_

/*
 * ioctl commands for the Apple Video Console
 */

#define	VC_TYPE_MOTHERBOARD	1
#define	VC_TYPE_HPV		2
#define	VC_TYPE_PCI		3
#define	VC_TYPE_MOTHERBOARD_PCI	4
#define	VC_TYPE_AV		5

struct vc_info {
	unsigned long	v_height;	/* pixels */
	unsigned long	v_width;	/* pixels */
	unsigned long	v_depth;
	unsigned long	v_rowbytes;
	unsigned long	v_baseaddr;
	unsigned long	v_type;
	char		v_name[32];
	unsigned long	v_physaddr;
	unsigned long	v_rows;		/* characters */
	unsigned long	v_columns;	/* characters */
	unsigned long	v_rowscanbytes;	/* Actualy number of bytes used for display per row*/
					/* Note for PCI (VCI) systems, part of the row byte line
					  is used for the hardware cursor which is not to be touched */
	unsigned long	v_reserved[5];
};

struct vc_color {
	unsigned char	vp_index;
	unsigned char	vp_red;
	unsigned char	vp_green;
	unsigned char	vp_blue;
};

/*
 * ioctl commands for the keyboard
 */

#define VC_LED_NUMLOCK		0x0001
#define VC_LED_CAPSLOCK		0x0002
#define VC_LED_SCROLLLOCK	0x0004

/* The get/set_status commands themselves */

#define	VC_GETINFO		(('v'<<16)|1)
#define	VC_SETXMODE		(('v'<<16)|2)
#define	VC_SETNORMAL		(('v'<<16)|3)
#define	VC_SETCOLOR		(('v'<<16)|4)
#define	VC_GETCOLOR		(('v'<<16)|5)
#define	VC_SETVIDEOMODE		(('v'<<16)|6)
#define VC_GETKEYBOARDLEDS	(('v'<<16)|7)
#define VC_SETKEYBOARDLEDS	(('v'<<16)|8)

#endif /* _POWERMAC_VIDEO_CONSOLE_H_ */
