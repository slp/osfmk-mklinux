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
/*
 * Video Support for PowerBook 5300's
 *
 * ECSC Video chip / Flat Panel Display
 *
 * -- Michael Burg, Apple Computer Inc. Copyright 1998
 */

#include <vc.h>
#include <platforms.h>

#include <mach_kdb.h>
#include <kern/spl.h>
#include <machine/machparam.h>          /* spl definitions */
#include <types.h>
#include <vm/vm_kern.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>
#include <ppc/POWERMAC/video_console.h>
#include <ppc/POWERMAC/video_board.h>
#include <ppc/POWERMAC/video_pdm.h>
#include <ppc/new_screen.h>
#include <ppc/io_map_entries.h>

#define	M2_ECSC_BASE	0x50F20000

struct	csc_regs {
	char	_pad[0x40];
	volatile unsigned char	clut_waddr;	/* 0x40 */
	char	_pad1;
	volatile unsigned char	clut_data;	/* 0x42 */
	char	_pad2[0x3];
	volatile unsigned char	clut_raddr;	/* 0x46 */
} *csc_regs = (struct csc_regs *) M2_ECSC_BASE;

int		ecsc_probe(caddr_t port);
io_return_t	ecsc_init(struct vc_info *);
io_return_t	ecsc_setcolor(int color, struct vc_color *);
io_return_t	ecsc_getmode(struct vc_info *);
io_return_t	ecsc_setmode(struct vc_info *);

struct video_board ecsc_video = {
	ecsc_init,
	ecsc_setcolor,
	ecsc_setmode,
	ecsc_getmode,
	NULL,
	ecsc_probe
};

struct vc_info			ecsc_info;

decl_simple_lock_data(,ecsc_lock)

int
ecsc_probe(caddr_t port)
{
	/*
	 * Check to see if this is the PB5300 AND the video base
	 * is where it should
	 */
	
	if (powermac_info.class == POWERMAC_CLASS_POWERBOOK
	&& ((unsigned long)port & 0xf0000000) == 0x60000000)
		return	TRUE;
	else
		return	FALSE;
}

/*
 * Initialize the Monitor.. 
 *
 */

io_return_t
ecsc_init(struct vc_info * info) 
{
	int			i;
	extern Boot_Video	boot_video_info;

	/* this is called twice, once right at startup, and once in
	 * the device probe routines. In the latter, we can safely
	 * remap the video to valid kernel addresses to free up the
	 * segment register mappings
	 */

	if (kernel_map != VM_MAP_NULL) 
		csc_regs = (struct  csc_regs *) io_map(M2_ECSC_BASE, 4096);
					 
	strcpy(ecsc_info.v_name, "PB5300 Flat Panel Display");

	ecsc_info.v_width = boot_video_info.v_width;
	ecsc_info.v_height = boot_video_info.v_height;
	ecsc_info.v_depth = boot_video_info.v_depth;
	ecsc_info.v_rowbytes = boot_video_info.v_rowBytes;
	ecsc_info.v_baseaddr = boot_video_info.v_baseAddr;
	ecsc_info.v_physaddr = boot_video_info.v_baseAddr;
	ecsc_info.v_type = VC_TYPE_AV;

	memcpy(info, &ecsc_info, sizeof(ecsc_info));

	return	D_SUCCESS;
}

/*
 * Set the colors...
 */
io_return_t
ecsc_setcolor(int count, struct vc_color *colors)
{
	int	i;
	unsigned char	new_color;
	int	ticks;

	ticks = nsec_to_processor_clock_ticks(260);

	simple_lock(&ecsc_lock);

	for (i = 0; i < count ;i++) {
		tick_delay(ticks);
		csc_regs->clut_waddr = i; eieio();
		new_color = colors[i].vp_red;
		csc_regs->clut_data = new_color; eieio();
		new_color = colors[i].vp_green;
		csc_regs->clut_data = new_color; eieio();
		new_color = colors[i].vp_blue;
		csc_regs->clut_data = new_color; eieio();
	}
	simple_unlock(&ecsc_lock);

	return	D_SUCCESS;
}

/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
ecsc_setmode(struct vc_info * info)
{
	return	D_INVALID_OPERATION;
}

/*
 * Get the current video mode..
 */

io_return_t
ecsc_getmode(struct vc_info *info)
{
	memcpy(info, &ecsc_info, sizeof(*info));
	return	D_SUCCESS;
}
