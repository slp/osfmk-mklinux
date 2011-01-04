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
 * 
 */
/*
 * MkLinux
 */
/*
 * Video Support for A/V boards
 *
 * This is the beginning of support for AV. There is no
 * support for change of video modes, and automatic detection
 * of the current video mode.
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

#define	AV_CONTROL_REGS_BASE_PHYS	0xE0036000
#define AV_CONTROL_REGS_SIZE		0x1000

#define	AV_MISC_REGS_BASE_PHYS	(0xE0F20000)
#define AV_MISC_REGS_SIZE	0x11000

/* this starts as a 1-1 mappings then changes during device_probe */

volatile unsigned char *av_control_base_v[2] =
		{(volatile unsigned char*)AV_CONTROL_REGS_BASE_PHYS,
		 (volatile unsigned char*)AV_MISC_REGS_BASE_PHYS};


#define	AV_BASE			(av_control_base_v[0])
#define AV_MISC			(av_control_base_v[1])

#define	AV_CLUT_ADDR		(AV_MISC+0x10000)
#define	AV_CLUT_DATA		(AV_MISC+0x10010)
#define	AV_VBL_STATUS		(AV_BASE+0x0000)
#define	AV_CLEAR_VBL_INTERRUPT	(AV_BASE+0x0120)

io_return_t	vav_init(struct vc_info *);
io_return_t	vav_setcolor(int color, struct vc_color *);
io_return_t	vav_getmode(struct vc_info *);
io_return_t	vav_setmode(struct vc_info *);
void		av_writereg(volatile unsigned char *address, unsigned short value);
unsigned short	av_readreg(volatile unsigned char *address);
int		 av_get_reg_width(volatile unsigned char *address);
void		 av_wait_for_vbl(void);

unsigned int	vav_calculate_rowbytes(struct vc_info *);

struct video_board av_video = {
	vav_init,
	vav_setcolor,
	vav_setmode,
	vav_getmode,
	NULL
};

struct vc_info			av_info;

decl_simple_lock_data(,av_lock)

/*
 * figure out the rowbyte values from the depth
 * and width of the screen.
 */

unsigned int
vav_calculate_rowbytes(struct vc_info * info)
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
vav_init(struct vc_info * info) 
{
	int			i;
	extern Boot_Video	boot_video_info;

	/* this is called twice, once right at startup, and once in
	 * the device probe routines. In the latter, we can safely
	 * remap the video to valid kernel addresses to free up the
	 * segment register mappings
	 */

	if (kernel_map != VM_MAP_NULL) {
		av_control_base_v[0]	=
			(volatile unsigned char*)
				io_map(AV_CONTROL_REGS_BASE_PHYS,
				       AV_CONTROL_REGS_SIZE);
		av_control_base_v[1]	=
			(volatile unsigned char*)
				io_map(AV_MISC_REGS_BASE_PHYS,
				       AV_MISC_REGS_SIZE);
		simple_lock_init(&av_lock, ETAP_IO_TTY);
	}
					 
	strcpy(av_info.v_name, "A/V Video");

	av_info.v_width = boot_video_info.v_width;
	av_info.v_height = boot_video_info.v_height;
	av_info.v_depth = boot_video_info.v_depth;
	av_info.v_rowbytes = boot_video_info.v_rowBytes;
	av_info.v_baseaddr = boot_video_info.v_baseAddr;
	av_info.v_physaddr = boot_video_info.v_baseAddr;
	av_info.v_type = VC_TYPE_AV;

	memcpy(info, &av_info, sizeof(av_info));

	return	D_SUCCESS;
}

/*
 * Set the colors...
 */
io_return_t
vav_setcolor(int count, struct vc_color *colors)
{
	int	i;
	unsigned char	new_color;
	int	ticks;

	ticks = nsec_to_processor_clock_ticks(260);

	simple_lock(&av_lock);
	av_wait_for_vbl();

	for (i = 0; i < count ;i++) {
		tick_delay(ticks);
		*(AV_CLUT_ADDR) = i;
		eieio();
		new_color = colors[i].vp_red;
		*(AV_CLUT_DATA) = new_color; eieio();
		new_color = colors[i].vp_green;
		*(AV_CLUT_DATA) = new_color; eieio();
		new_color = colors[i].vp_blue;
		*(AV_CLUT_DATA) = new_color; eieio();
		*(AV_CLUT_DATA) = 00; eieio();
	}
	simple_unlock(&av_lock);

	return	D_SUCCESS;
}

void
av_wait_for_vbl(void)
{
	unsigned short gotinterrupt= 0;

	av_writereg(AV_CLEAR_VBL_INTERRUPT, 0);	// Clear A/V vbl bit
	av_writereg(AV_CLEAR_VBL_INTERRUPT, 1);	// Prime for next interrupt

	while (!gotinterrupt)
		gotinterrupt = av_readreg(AV_VBL_STATUS);
}


/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
vav_setmode(struct vc_info * info)
{
	return	D_INVALID_OPERATION;
}

/*
 * Get the current video mode..
 */

io_return_t
vav_getmode(struct vc_info *info)
{
	memcpy(info, &av_info, sizeof(*info));
	return	D_SUCCESS;
}

int
av_get_reg_width(volatile unsigned char *address)
{
	/* these 'constants' are in fact macros that refer to variables,
	 * so we can't easily use a switch statement
	 */
	if ((address == AV_CLEAR_VBL_INTERRUPT) ||
	    (address == AV_VBL_STATUS)) {
		return 1;
	} else {
		return 8;
	}
}

unsigned short
av_readreg(volatile unsigned char *address)
{
	unsigned short value = 0;

	int count, width;

	width = av_get_reg_width(address);
	for (count = 0; count < width; count++) {
		value |= (1 & *address) << count;
		eieio();
		address++;
	}

	return	value;
}

void
av_writereg(volatile unsigned char *address, unsigned short value)
{
	int	count, width;

	width = av_get_reg_width(address);

	for (count = 0; count < width; count++) {
		*address++ = value; eieio();
		value >>= 1;
	}
}
