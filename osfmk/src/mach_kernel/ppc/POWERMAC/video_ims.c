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
 * Driver for the ATI video for PCI class of machines.
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

int		ims_probe(caddr_t port);
io_return_t	ims_init(struct vc_info *);
io_return_t	ims_setcolor(int color, struct vc_color *);
io_return_t	ims_getmode(struct vc_info *);
io_return_t	ims_setmode(struct vc_info *);

volatile unsigned char *regBasePhys;
volatile unsigned char *regBase;

#define DAC_W_INDEX		0x00
#define DAC_DATA		0x04

struct video_board ims_video = {
	ims_init,
	ims_setcolor,
	ims_setmode,
	ims_getmode,
	NULL,
	ims_probe
};

struct vc_info	ims_info;
static device_node_t	*ims_node;

decl_simple_lock_data(,ims_lock)

/*
 * Initialize the screen.. lookup the monitor type and
 * figure out what mode it is in.
 */

static device_node_t *ims_node;

static char	*ims_names[] = {
	"IMS,tt128mb",
	"IMS,tt128mbA",
	"IMS,tt128mb8",
	"IMS,tt128mb4",
	"IMS,tt128mb2",
	NULL
};

int
ims_probe(caddr_t addr)
{
	int	i;

	ims_node = find_type("display");
	
	while (ims_node) {
		if (strncmp("IMS,", ims_node->name, 4) == 0)
			goto found;

		ims_node = ims_node->next;
	}

	for (i = 0; ims_names[i]; i++)
		if (ims_node = find_devices(ims_names[i])) {
			break;
		}

	if (ims_node == NULL)
		return 0;

found:

	regBase = regBasePhys = (volatile unsigned char *)
		(ims_node->addrs[0].address + 0x00840000);

	return 1;
}


io_return_t
ims_init(struct vc_info * info) 
{
	extern Boot_Video       boot_video_info;
	unsigned char		val;

	if (kernel_map) {
		regBase = (volatile unsigned char *)
			io_map((unsigned int)regBasePhys, 0x400);
		simple_lock_init(&ims_lock, ETAP_IO_TTY);
	}

	strcpy(ims_info.v_name, ims_node->name);
	ims_info.v_width = boot_video_info.v_width;
	ims_info.v_height = boot_video_info.v_height;
	ims_info.v_depth = boot_video_info.v_depth;
	ims_info.v_rowbytes = boot_video_info.v_rowBytes;
	ims_info.v_physaddr = boot_video_info.v_baseAddr;
	ims_info.v_baseaddr = ims_info.v_physaddr;
	ims_info.v_type = VC_TYPE_PCI;

	memcpy(info, &ims_info, sizeof(ims_info));

	return	D_SUCCESS;
}

/*
 * Set the colors...
 */

io_return_t
ims_setcolor(int count, struct vc_color *colors)
{
	int	i;

	simple_lock(&ims_lock);

	for (i = 0; i < count ;i++, colors++) {
		*(regBase + DAC_W_INDEX)  = i;
		eieio();
		*(regBase + DAC_DATA) = colors->vp_red;
		eieio();
		*(regBase + DAC_DATA) = colors->vp_green;
		eieio();
		*(regBase + DAC_DATA) = colors->vp_blue;
		eieio();
	}

	simple_unlock(&ims_lock);

	return	D_SUCCESS;
}

/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
ims_setmode(struct vc_info * info)
{
	return	D_INVALID_OPERATION;
}

/*
 * Get the current video mode..
 */

io_return_t
ims_getmode(struct vc_info *info)
{
	memcpy(info, &ims_info, sizeof(*info));

	return	D_SUCCESS;
}
