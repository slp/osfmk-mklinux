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
 * Video Support for Video Direct Slot (aka HPV aka Radacal)
 *
 * This is the beginning of support for VDS. There is no
 * support for change of video modes, and automatic detection
 * of the current video mode.
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
#include <ppc/POWERMAC/video_console.h>
#include <ppc/POWERMAC/video_board.h>
#include <ppc/POWERMAC/video_pdm.h>
#include <ppc/new_screen.h>
#include <ppc/io_map_entries.h>

#define	HPV_CONTROL_REGS_BASE_PHYS	0xFEC00000
#define HPV_CONTROL_REGS_SIZE		0x1000

/* this starts as a 1-1 mapping then changes during device_probe */

volatile unsigned char *hpv_control_base_v =
		(volatile unsigned char*)HPV_CONTROL_REGS_BASE_PHYS;

#define HPV_CONTROL_BASE	(hpv_control_base_v)
#define	HPV_OVERFLOW_REG	0xF0
#define	HPV_BASE_ADDR1		0x00
#define	HPV_BASE_ADDR2		0x08
#define	HPV_ROW_BYTES		0x10

#define	HPV_RADACAL_BASE	(HPV_CONTROL_BASE+0x207)
#define	HPV_CLUT_ADDR		(HPV_RADACAL_BASE+0)
#define	HPV_CLUT_DATA		(HPV_RADACAL_BASE+0x30)

#define	HPV_INT_VBL	0x01	/* VBL redraw interrupt */
#define	HPV_INT_ANIM	0x02	/* animation interrupt */
#define	HPV_INT_CURSOR	0x04	/* cursor interrupt */

#define	HPV_INT_STATUS		0x0110
#define	HPV_INT_MASK		0x0108
#define	HPV_CLEAR_VBL_INT	(HPV_CONTROL_BASE + 0x0128)

io_return_t	vhpv_init(struct vc_info *);
io_return_t	vhpv_setcolor(int color, struct vc_color *);
io_return_t	vhpv_getmode(struct vc_info *);
io_return_t	vhpv_setmode(struct vc_info *);
void		hpv_writereg(unsigned short regnum, unsigned short value);
unsigned short	hpv_readreg(unsigned short regnum);

unsigned int	vhpv_calculate_rowbytes(struct vc_info *);

struct video_board hpv_video = {
	vhpv_init,
	vhpv_setcolor,
	vhpv_setmode,
	vhpv_getmode,
	NULL
};

struct vc_info			hpv_info;

decl_simple_lock_data(,hpv_lock)

void
hpv_writereg(unsigned short reg, unsigned short value)
{
	*(HPV_CONTROL_BASE + HPV_OVERFLOW_REG) = (value>>6) & 0x3f;
	eieio();
	*(HPV_CONTROL_BASE + reg) = (value) & 0x3f;
	eieio();

	return;
}

unsigned short
hpv_readreg(unsigned short reg)
{
	unsigned short value;

	value = *(HPV_CONTROL_BASE + reg); eieio();
	value |= *(HPV_CONTROL_BASE + HPV_OVERFLOW_REG) << 6;
	eieio();

	return	 value;
}

/*
 * figure out the rowbyte values from the depth
 * and width of the screen.
 */

unsigned int
vhpv_calculate_rowbytes(struct vc_info * info)
{
	unsigned int rowbytes = info->v_width;

	switch (info->v_depth) {
	case	1:
		rowbytes = rowbytes >> 3;
		break;

	case	2:
		rowbytes = rowbytes >> 2;
		break;

	case	4:
		rowbytes = rowbytes >> 1;
		break;

	case	16:
		rowbytes = rowbytes << 1;
		break;
	}

	return rowbytes;
}

/*
 * Initialize the Monitor.. 
 *
 */

io_return_t
vhpv_init(struct vc_info * info) 
{
	int			i;
	extern Boot_Video	boot_video_info;

	/* this is called twice, once right at startup, and once in
	 * the device probe routines. In the latter, we can safely
	 * remap the video to valid kernel addresses to free up the
	 * segment register mappings
	 */

	if (kernel_map != VM_MAP_NULL) {
		hpv_control_base_v	=
			(volatile unsigned char*)
				io_map(HPV_CONTROL_REGS_BASE_PHYS,
				       HPV_CONTROL_REGS_SIZE);
		simple_lock_init(&hpv_lock, ETAP_IO_TTY);
	}
					 
	strcpy(hpv_info.v_name, "unknown (using VDS port)");

	hpv_info.v_width = boot_video_info.v_width;
	hpv_info.v_height = boot_video_info.v_height;
	hpv_info.v_depth = boot_video_info.v_depth;
	hpv_info.v_rowbytes = boot_video_info.v_rowBytes;
	hpv_info.v_baseaddr = boot_video_info.v_baseAddr;
	hpv_info.v_physaddr = boot_video_info.v_baseAddr;
	hpv_info.v_type = VC_TYPE_HPV;

#if 0
	hpv_writereg(HPV_BASE_ADDR1, 0);
	hpv_writereg(HPV_BASE_ADDR2, 0x1000);

	hpv_writereg(HPV_ROW_BYTES, (hpv_info.v_rowbytes / 8));
	hpv_info.v_rowbytes = hpv_readreg(HPV_ROW_BYTES) * 8;
#endif
	memcpy(info, &hpv_info, sizeof(hpv_info));

	return	D_SUCCESS;
}

/*
 * Set the colors...
 */

io_return_t
vhpv_setcolor(int count, struct vc_color *colors)
{
	int	i;

	/* Wait until a VBL interrupt happens before
	 * setting the colors
	 */

	simple_lock(&hpv_lock);

	hpv_writereg(HPV_INT_MASK, HPV_INT_VBL);
	/* Clear any pending VBL interrupts */
	i = *(HPV_CLEAR_VBL_INT); eieio();

	while ((hpv_readreg(HPV_INT_STATUS) & HPV_INT_VBL) == 0)
		;

	hpv_writereg(HPV_INT_MASK, 0);

	*(HPV_CLUT_ADDR) = 0;
	eieio();

	for (i = 0; i < count ;i++, colors++) {
		*((unsigned char *) HPV_CLUT_DATA) = colors->vp_red;
		eieio();
		*((unsigned char *) HPV_CLUT_DATA) = colors->vp_green;
		eieio();
		*((unsigned char *) HPV_CLUT_DATA) = colors->vp_blue;
		eieio();
	}
	simple_unlock(&hpv_lock);

	return	D_SUCCESS;
}

/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
vhpv_setmode(struct vc_info * info)
{
	return	D_INVALID_OPERATION;
}

/*
 * Get the current video mode..
 */

io_return_t
vhpv_getmode(struct vc_info *info)
{
	memcpy(info, &hpv_info, sizeof(*info));
	return	D_SUCCESS;
}
