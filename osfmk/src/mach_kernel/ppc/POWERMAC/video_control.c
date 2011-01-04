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
 * MkLinux
 */

/*
 * Driver for the Control-Kaos video for PCI class of machines.
 *
 * (Someone in the hardware group watches too much TV! ;-))
 * 
 * Michael Burg. Apple Computer, Inc. (C) 1996-1997
 */

#include <vc.h>
#include <platforms.h>

#include <mach_kdb.h>
#include <kern/spl.h>
#include <machine/machparam.h>          /* spl definitions */
#include <types.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <chips/busses.h>
#include <vm/vm_kern.h>
#include <ppc/misc_protos.h>
#include <ppc/io_map_entries.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/video_console.h>
#include <ppc/POWERMAC/video_board.h>
#include <ppc/POWERMAC/device_tree.h>

io_return_t	control_init(struct vc_info *);
io_return_t	control_setcolor(int color, struct vc_color *);
io_return_t	control_getmode(struct vc_info *);
io_return_t	control_setmode(struct vc_info *);
int		control_probe(caddr_t port);

#define	CONTROL_CLUT_BASE_PHYS	0xF301B000
#define	CONTROL_CLUT_BASE_SIZE	0x1000

volatile unsigned char *control_clut_base_v
			= (volatile unsigned char *) CONTROL_CLUT_BASE_PHYS;

#define	CONTROL_CLUT_ADDR	(control_clut_base_v + 0x00)
#define CONTROL_CURSOR_DATA	(control_clut_base_v + 0x10)
#define	CONTROL_MULTIPORT	(control_clut_base_v + 0x20)
#define	CONTROL_CLUT_DATA	(control_clut_base_v + 0x30)

#define	CONTROL_CONTROL		0x20

struct video_board control_video = {
	control_init,
	control_setcolor,
	control_setmode,
	control_getmode,
	NULL,
	control_probe
};

struct vc_info			control_info;


decl_simple_lock_data(,control_lock)

static device_node_t	*control_node;

/*
 * Check to see if the device is around..
 */

int
control_probe(caddr_t port)
{
	return ((control_node = find_devices("control")) != NULL);
}

/*
 * Initialize the screen.. lookup the monitor type and
 * figure out what mode it is in.
 */

io_return_t
control_init(struct vc_info * info) 
{
	int			i;
	extern Boot_Video       boot_video_info;
	unsigned char		val;

	if (kernel_map) {
		control_clut_base_v = (volatile unsigned char *)
			io_map(CONTROL_CLUT_BASE_PHYS, CONTROL_CLUT_BASE_SIZE);
		simple_lock_init(&control_lock, ETAP_IO_TTY);
	}

	*CONTROL_CLUT_ADDR = CONTROL_CONTROL; eieio();
	val = *CONTROL_MULTIPORT; eieio();
	val &= ~0x2;
	*CONTROL_MULTIPORT = val; eieio();

	strcpy(control_info.v_name, "control");
	control_info.v_width = boot_video_info.v_width;
	control_info.v_height = boot_video_info.v_height;
	control_info.v_depth = boot_video_info.v_depth;
	control_info.v_rowbytes = boot_video_info.v_rowBytes;
	control_info.v_physaddr = boot_video_info.v_baseAddr;
	control_info.v_baseaddr = control_info.v_physaddr;
	control_info.v_type = VC_TYPE_PCI;

	memcpy(info, &control_info, sizeof(control_info));

	return	D_SUCCESS;
}

/*
 * Set the colors...
 */

io_return_t
control_setcolor(int count, struct vc_color *colors)
{
	int	i;

	simple_lock(&control_lock);

	*CONTROL_CLUT_ADDR  = 0;
	eieio();

	for (i = 0; i < count ;i++, colors++) {
		*CONTROL_CLUT_DATA  = colors->vp_red;
		eieio();
		*CONTROL_CLUT_DATA = colors->vp_green;
		eieio();
		*CONTROL_CLUT_DATA = colors->vp_blue;
		eieio();
	}

	simple_unlock(&control_lock);

	return	D_SUCCESS;
}

/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
control_setmode(struct vc_info * info)
{
	return	D_INVALID_OPERATION;
}

/*
 * Get the current video mode..
 */

io_return_t
control_getmode(struct vc_info *info)
{
	memcpy(info, &control_info, sizeof(*info));
	return	D_SUCCESS;
}
