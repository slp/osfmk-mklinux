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
 * Valkyrie Video Controller (6400 Performas)
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

io_return_t	valkyrie_init(struct vc_info *);
io_return_t	valkyrie_setcolor(int color, struct vc_color *);
io_return_t	valkyrie_getmode(struct vc_info *);
io_return_t	valkyrie_setmode(struct vc_info *);
int		valkyrie_probe(caddr_t port);


#define	CLUT_OFFSET	0x304000

struct valkyrie_clut {
	volatile unsigned char	addr;
	unsigned char	_pad1[7];
	volatile unsigned char  data;
};

volatile struct valkyrie_clut *valkyrie_clut;

struct video_board valkyrie_video = {
	valkyrie_init,
	valkyrie_setcolor,
	valkyrie_setmode,
	valkyrie_getmode,
	NULL,
	valkyrie_probe
};

struct vc_info			valkyrie_info;


decl_simple_lock_data(,valkyrie_lock)

static device_node_t	*valkyrie_node;

/*
 * Check to see if the device is around..
 */

int
valkyrie_probe(caddr_t port)
{
	if (valkyrie_node = find_devices("valkyrie")) {
		valkyrie_clut = (struct valkyrie_clut *) valkyrie_node->addrs[0].address + CLUT_OFFSET;
		return TRUE;
	} else 
		return FALSE;
}

/*
 * Initialize the screen.. lookup the monitor type and
 * figure out what mode it is in.
 */

io_return_t
valkyrie_init(struct vc_info * info) 
{
	int			i;
	extern Boot_Video       boot_video_info;
	unsigned char		val;

	if (kernel_map) {
		valkyrie_clut = (struct valkyrie_clut *)
			io_map(valkyrie_node->addrs[0].address + CLUT_OFFSET, 4096);
		simple_lock_init(&valkyrie_lock, ETAP_IO_TTY);
	}

	strcpy(valkyrie_info.v_name, "valkyrie");
	valkyrie_info.v_width = boot_video_info.v_width;
	valkyrie_info.v_height = boot_video_info.v_height;
	valkyrie_info.v_depth = boot_video_info.v_depth;
	valkyrie_info.v_rowbytes = boot_video_info.v_rowBytes;
	valkyrie_info.v_physaddr = boot_video_info.v_baseAddr;
	valkyrie_info.v_baseaddr = valkyrie_info.v_physaddr;
	valkyrie_info.v_type = VC_TYPE_PCI;

	memcpy(info, &valkyrie_info, sizeof(valkyrie_info));

	return	D_SUCCESS;
}

/*
 * Set the colors...
 */

io_return_t
valkyrie_setcolor(int count, struct vc_color *colors)
{
	int	i;

	simple_lock(&valkyrie_lock);

	valkyrie_clut->addr = 0; eieio();
	delay(10);

	for (i = 0; i < count ;i++, colors++) {
		valkyrie_clut->data  = colors->vp_red; eieio();
		valkyrie_clut->data = colors->vp_green; eieio();
		valkyrie_clut->data = colors->vp_blue; eieio();
		delay(10);
	}

	simple_unlock(&valkyrie_lock);

	return	D_SUCCESS;
}

/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
valkyrie_setmode(struct vc_info * info)
{
	return	D_INVALID_OPERATION;
}

/*
 * Get the current video mode..
 */

io_return_t
valkyrie_getmode(struct vc_info *info)
{
	memcpy(info, &valkyrie_info, sizeof(*info));
	return	D_SUCCESS;
}
