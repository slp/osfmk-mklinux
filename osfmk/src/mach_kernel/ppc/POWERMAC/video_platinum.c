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
/*
 * 
 * Driver for the Platinum video controller used on PCI boxes (7200)
 *
 * Michael Burg.
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

io_return_t	platinum_init(struct vc_info *);
io_return_t	platinum_setcolor(int color, struct vc_color *);
io_return_t	platinum_getmode(struct vc_info *);
io_return_t	platinum_setmode(struct vc_info *);
int		platinum_probe(caddr_t port);

#define	PLAT_CLUT_BASE_PHYS	0xF301B000
#define	PLAT_CLUT_BASE_SIZE	0x1000

volatile unsigned char *platinum_clut_base_v = (volatile unsigned char *) PLAT_CLUT_BASE_PHYS;

#define	PLAT_CLUT_ADDR	(platinum_clut_base_v + 0x00)
#define PLAT_MULTIPORT	(platinum_clut_base_v + 0x20)
#define	PLAT_CLUT_DATA	(platinum_clut_base_v + 0x30)

#define	PLAT_CONTROL	0x20	/* Only 1 bit defined - HW cursor enable/disable */
#define	PLAT_TEST	0x22

struct video_board platinum_video = {
	platinum_init,
	platinum_setcolor,
	platinum_setmode,
	platinum_getmode,
	NULL,
	platinum_probe
};

struct vc_info			platinum_info;
static device_node_t		*platinum_node;

int
platinum_probe(caddr_t port)
{
	return ((platinum_node = find_devices("platinum")) != NULL);
}

/*
 * Initialize the screen.. lookup the monitor type and
 * figure out what mode it is in.
 */

io_return_t
platinum_init(struct vc_info * info) 
{
	int			i;
	extern Boot_Video       boot_video_info;
	unsigned char		val;

	if (kernel_map) 
		platinum_clut_base_v = (volatile unsigned char *)
			io_map(PLAT_CLUT_BASE_PHYS, PLAT_CLUT_BASE_SIZE);

	*PLAT_CLUT_ADDR = PLAT_CONTROL; eieio();
	val = *PLAT_MULTIPORT; eieio();
	val &= ~0x1;	/* Clear cursor */
	*PLAT_MULTIPORT = val; eieio();

	strcpy(platinum_info.v_name, "platinum");
	platinum_info.v_width = boot_video_info.v_width;
	platinum_info.v_height = boot_video_info.v_height;
	platinum_info.v_depth = boot_video_info.v_depth;
	platinum_info.v_rowbytes = boot_video_info.v_rowBytes;
	platinum_info.v_physaddr = boot_video_info.v_baseAddr;
	platinum_info.v_baseaddr = platinum_info.v_physaddr;
	platinum_info.v_type = VC_TYPE_PCI;

	memcpy(info, &platinum_info, sizeof(platinum_info));

	return	D_SUCCESS;
}

/*
 * Set the colors...
 */

io_return_t
platinum_setcolor(int count, struct vc_color *colors)
{
	int	i;

	*PLAT_CLUT_ADDR  = 0;
	eieio();

	for (i = 0; i < count ;i++, colors++) {
		*PLAT_CLUT_DATA  = colors->vp_red;
		eieio();
		*PLAT_CLUT_DATA = colors->vp_green;
		eieio();
		*PLAT_CLUT_DATA = colors->vp_blue;
		eieio();
	}

	return	D_SUCCESS;
}

/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
platinum_setmode(struct vc_info * info)
{
	return	D_INVALID_OPERATION;
}

/*
 * Get the current video mode..
 */

io_return_t
platinum_getmode(struct vc_info *info)
{
	memcpy(info, &platinum_info, sizeof(*info));
	return	D_SUCCESS;
}
