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
 * Dummy Video driver
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

io_return_t	dummy_init(struct vc_info *);
io_return_t	dummy_setcolor(int color, struct vc_color *);
io_return_t	dummy_getmode(struct vc_info *);
io_return_t	dummy_setmode(struct vc_info *);

struct video_board dummy_video = {
	dummy_init,
	dummy_setcolor,
	dummy_setmode,
	dummy_getmode,
	NULL
};

struct vc_info			dummy_info;

/*
 * Initialize the screen.. lookup the monitor type and
 * figure out what mode it is in.
 */

io_return_t
dummy_init(struct vc_info * info) 
{
	int			i;
	extern Boot_Video       boot_video_info;

	strcpy(dummy_info.v_name, "VCI Video");
	dummy_info.v_width = boot_video_info.v_width;
	dummy_info.v_height = boot_video_info.v_height;
	dummy_info.v_depth = boot_video_info.v_depth;
	dummy_info.v_rowbytes = boot_video_info.v_rowBytes;
	dummy_info.v_physaddr = boot_video_info.v_baseAddr;
	dummy_info.v_baseaddr = dummy_info.v_physaddr;
	dummy_info.v_type = VC_TYPE_PCI;

	memcpy(info, &dummy_info, sizeof(dummy_info));

	return	D_SUCCESS;
}

/*
 * Set the colors...
 */

io_return_t
dummy_setcolor(int count, struct vc_color *colors)
{
	int	i;

	return	D_INVALID_OPERATION;
}

/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
dummy_setmode(struct vc_info * info)
{
	return	D_INVALID_OPERATION;
}

/*
 * Get the current video mode..
 */

io_return_t
dummy_getmode(struct vc_info *info)
{
	memcpy(info, &dummy_info, sizeof(*info));
	return	D_SUCCESS;
}
