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
 * Driver for the PDM (6100,7100,8100) class of video
 * motherboard controllers.
 *
 * Michael Burg.
 */

#include <vc.h>
#include <platforms.h>

#include <mach_kdb.h>
#include <kern/spl.h>
#include <kern/lock.h>
#include <machine/machparam.h>          /* spl definitions */
#include <types.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/video_console.h>
#include <ppc/POWERMAC/video_board.h>
#include <ppc/POWERMAC/video_pdm.h>

io_return_t	vpdm_init(struct vc_info *);
io_return_t	vpdm_setcolor(int color, struct vc_color *);
io_return_t	vpdm_getmode(struct vc_info *);
io_return_t	vpdm_setmode(struct vc_info *);
io_return_t	vpdm_register_int(struct vc_info *);

void		vpdm_reset_sense_line(void);
void		vpdm_drive_sense_line(unsigned char);
unsigned char	vpdm_get_depth(void);
unsigned int	vpdm_calculate_rowbytes(struct vc_info *);
unsigned char	vpdm_get_sense_line(void);
unsigned char	vpdm_get_sense_code(void);
unsigned char	vpdm_get_extsense_code(void);

struct video_board pdm_video = {
	vpdm_init,
	vpdm_setcolor,
	vpdm_setmode,
	vpdm_getmode,
	vpdm_register_int
};

unsigned char	vpdm_current_sense, vpdm_current_ext_sense;


struct monitor {
	unsigned char	basesense;
	unsigned char	extsense;
	unsigned char	monitor;
} monitors[] = {
{0, MAC_MONITOR_Zero21Inch,		DISPLAY_21Inch},
{1, MAC_MONITOR_OnePortraitMono,	DISPLAY_PortraitMono},
{2, MAC_MONITOR_Two12Inch,		DISPLAY_12Inch},
{3, MAC_MONITOR_Three21InchRadius,	DISPLAY_21Inch},
{3, MAC_MONITOR_Three21InchMonoRadius,	DISPLAY_21InchMono},
{3, MAC_MONITOR_Three21InchMono,	DISPLAY_21InchMono},
{4, MAC_MONITOR_FourNTSC,		DISPLAY_NTSC},
{5, MAC_MONITOR_FivePortrait,		DISPLAY_Portrait},
{6, MAC_MONITOR_SixMSB1,		DISPLAY_MultiScanBand1},
{6, MAC_MONITOR_SixMSB2, 		DISPLAY_MultiScanBand2},
{6, MAC_MONITOR_SixMSB3,		DISPLAY_MultiScanBand3},
{6, MAC_MONITOR_SixStandard,		DISPLAY_Standard},
{7, MAC_MONITOR_SevenPAL,		DISPLAY_PAL},
{7, MAC_MONITOR_SevenNTSC,		DISPLAY_NTSC},
{7, MAC_MONITOR_SevenVGA,		DISPLAY_VGA},
{7, MAC_MONITOR_Seven16Inch,		DISPLAY_16Inch},
{7, MAC_MONITOR_SevenPALAlternate,	DISPLAY_PAL},
{7, MAC_MONITOR_Seven19Inch, 		DISPLAY_19Inch},
{7, MAC_MONITOR_SevenNoDisplay, 	DISPLAY_NoDisplay},
};

int	monitors_count = sizeof(monitors) / sizeof(monitors[0]);

unsigned char	mode_12inch[] = {SCREEN_512x384At60Hz};
unsigned char	mode_standard[] = {SCREEN_640x480At67Hz};
unsigned char	mode_portrait[] = {SCREEN_640x870At75Hz};
unsigned char	mode_portraitmono[] = {SCREEN_640x870At75Hz};
unsigned char	mode_16inch[] = {SCREEN_832x624At75Hz};
unsigned char	mode_vga[] = {SCREEN_640x480At60HzVGA};
unsigned char	mode_scanband1[] = {SCREEN_640x480At67Hz, SCREEN_512x384At60Hz,
					SCREEN_832x624At75Hz};
unsigned char	mode_scanband2[] = {SCREEN_832x624At75Hz, SCREEN_640x480At67Hz};
unsigned char	mode_scanband3[] = {SCREEN_832x624At75Hz,SCREEN_640x480At67Hz };

struct monitor_capability {
	unsigned char	monitor;
	int		mode_count;
	unsigned char	*modes;
	char		*name;
} monitor_capabilities[] = {
	{ DISPLAY_12Inch, 	1, mode_12inch, 	"12 Inch Display"},
	{ DISPLAY_Standard, 	1, mode_standard,	"12/13/14 Inch Display"},
	{ DISPLAY_Portrait, 	1, mode_portrait,	"15 Inch Portrait RGB" },
	{ DISPLAY_PortraitMono, 1, mode_portraitmono,	"14 Inch Portrait Mono"  }, 
	{ DISPLAY_16Inch, 	1, mode_16inch,		"16 Inch RGB" },
	{ DISPLAY_VGA, 		1, mode_vga,		"VGA Monitor" },
	{ DISPLAY_MultiScanBand1, 3, mode_scanband1,	"13-16 Inch Monitor" },
	{ DISPLAY_MultiScanBand2, 2, mode_scanband2,	"13-19 Inch Monitor" },
	{ DISPLAY_MultiScanBand3, 2, mode_scanband3,	"13-21 Inch Monitor" },
};
int	monitor_capability_count = sizeof(monitor_capabilities) / sizeof (monitor_capabilities[0]);


struct screen_value {
	unsigned char	screen;
	unsigned char	value;
	unsigned short	width;
	unsigned short	height;
	unsigned char	maxdepth;
} screen_values[] = {
	{SCREEN_512x384At60Hz, 		0x02,	512, 384, 16},
	{SCREEN_640x480At60HzVGA, 	0x0b,	640, 480, 8},
	{SCREEN_640x480At67Hz, 		0x06,	640, 480, 16},
	{SCREEN_640x870At75Hz, 		0x01,	640, 870, 8},
	{SCREEN_832x624At75Hz, 		0x09,	832, 624, 8}
};

int screen_value_count = sizeof(screen_values)/ sizeof(screen_values[0]);

struct depth {
	unsigned char	depth;
	unsigned char	control;
} depths[] = {
	1,	0x08,
	2,	0x09,
	4,	0x0a,
	8,	0x0b,
	16,	0x0c
};

int depth_count = sizeof(depths) / sizeof(depths[0]);

struct vc_info			pdm_info;
struct monitor_capability	*pdm_monitor;

volatile unsigned char	*pdm_monitor_id;
volatile unsigned char  *pdm_video_mode;
volatile unsigned char  *pdm_clut_addr;
volatile unsigned char  *pdm_clut_data;
volatile unsigned char  *pdm_video_mode;
volatile unsigned char  *pdm_video_pixeldepth;
volatile unsigned char  *pdm_clut_control;

int			vpdm_color_count;
struct vc_color		*vpdm_color_ptr;
int			vpdm_int_count;

decl_simple_lock_data(,vpdm_lock)
void	vpdm_interrupt(int unit, void *arg);

/* 
 * register interrupt for VBL interrupt
 */

io_return_t
vpdm_register_int(struct vc_info *vinfo)
{
	simple_lock_init(&vpdm_lock, ETAP_IO_TTY);
	pmac_register_int(PMAC_DEV_VBL, SPLTTY, vpdm_interrupt);

	return D_SUCCESS;
}

/*
 * Reset the sense lines.. 
 */

void
vpdm_reset_sense_line(void)
{
	*pdm_monitor_id = 0x07;
	eieio();
	delay(3);
}

/*
 * Setup to read a given sense line..
 */

void
vpdm_drive_sense_line(unsigned char line)
{
	*pdm_monitor_id = line;
	eieio();
	delay(3);
}

/*
 * Get the value of a sense line.
 */

unsigned char 
vpdm_get_sense_line(void)
{
	return	((*pdm_monitor_id) & 0x70) >> 4;
}

/*
 * Retrieve the (base) sense code
 */

unsigned char
vpdm_get_sense_code(void)
{
	unsigned char	ret;

	vpdm_reset_sense_line();
	ret = vpdm_get_sense_line();

	return ret;
}

/*
 * Get the extended sense code..
 */

unsigned char
vpdm_get_extsense_code(void)
{
	unsigned char exta, extb, extc;

	/*
	 * Don't ask why this is done this way...
	 */

	vpdm_drive_sense_line(VPDM_DRIVE_A_LINE);
	exta = (vpdm_get_sense_line() << 4) & 0x30;
	vpdm_drive_sense_line(VPDM_DRIVE_B_LINE);
	extb = vpdm_get_sense_line();
	extb = ((extb >> 2) << 3) | ((extb & 0x1) << 2);
	vpdm_drive_sense_line(VPDM_DRIVE_C_LINE);
	extc = (vpdm_get_sense_line() >> 1) & 0x3;;

	return (exta | extb | extc);
}
	
/*
 * Return the depth of the current mode the monitor is in..
 */

unsigned char
vpdm_get_depth(void)
{
	int	i;
	unsigned char	value, ret = 0;

	eieio();
	value = *pdm_clut_control & 0x7f;
	for (i = 0; i < depth_count; i++) {
		if (depths[i].control == value) {
			ret = depths[i].depth;
			break;
		}
	}

	return ret;
}

/*
 * figure out the rowbyte values from the depth
 * and width of the screen.
 */

unsigned int
vpdm_calculate_rowbytes(struct vc_info * info)
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
 * Initialize the screen.. lookup the monitor type and
 * figure out what mode it is in.
 */

io_return_t
vpdm_init(struct vc_info * info) 
{
	int			i;
	unsigned char		sense, extsense, mode;
	struct monitor		*code;
	struct screen_value	*value;
	extern Boot_Video       boot_video_info;

	pdm_monitor_id = (volatile unsigned char *)POWERMAC_IO(MONITOR_ID);
	pdm_video_mode = (volatile unsigned char *)POWERMAC_IO(VIDEO_MODE);
	pdm_clut_addr = (volatile unsigned char *)POWERMAC_IO(CLUT_ADDR);
	pdm_clut_data = (volatile unsigned char *)POWERMAC_IO(CLUT_DATA);
	pdm_video_mode = (volatile unsigned char *)POWERMAC_IO(VIDEO_MODE);
	pdm_video_pixeldepth = (volatile unsigned char *)POWERMAC_IO(VIDEO_PIXELDEPTH);
	pdm_clut_control = (volatile unsigned char *)POWERMAC_IO(CLUT_CONTROL);
	
#if 0
	/*
	 * Get the monitor codes, and lookup the monitor type
	 */

	vpdm_current_sense  = sense = vpdm_get_sense_code();
	vpdm_current_ext_sense =  extsense = vpdm_get_extsense_code();

	for (i = 0, code = monitors; i < monitors_count; i++, code++) {
		if (code->basesense == sense && code->extsense == extsense)
			goto found;
	}

	/* Not found.. default to SOMETHING.. */
	code = &monitors[9];

found:
	/* Now figure out what capabilities this monitor currently
	 * supports
	 */
	for (i = 0, pdm_monitor  = monitor_capabilities; i < monitor_capability_count; i++, pdm_monitor ++) {
		if (pdm_monitor->monitor == code->monitor) 
			goto found_monitor;
	}

	pdm_monitor  = &monitor_capabilities[0];

found_monitor:

	mode = *pdm_video_mode;

	/*
	 * Now try to figure out what mode the monitor is in..
 	 */

	for (i = 0, value = screen_values; i <  screen_value_count; i++, value++) {
		if (value->value ==  mode)
			goto found_mode;
	}
#endif

found_mode:
#if 0
	strcpy(pdm_info.v_name, pdm_monitor->name);
	pdm_info.v_width = value->width;
	pdm_info.v_height = value->height;
	pdm_info.v_depth = vpdm_get_depth();
	pdm_info.v_rowbytes = vpdm_calculate_rowbytes(&pdm_info);
#else
	strcpy(pdm_info.v_name, "Motherboard Video");
	pdm_info.v_width = boot_video_info.v_width;
	pdm_info.v_height = boot_video_info.v_height;
	pdm_info.v_depth = boot_video_info.v_depth;
	pdm_info.v_rowbytes = boot_video_info.v_rowBytes;
#endif

	pdm_info.v_physaddr = VPDM_PHYSADDR;
	pdm_info.v_baseaddr = VPDM_BASEADDR;
	pdm_info.v_type = VC_TYPE_MOTHERBOARD;

	memcpy(info, &pdm_info, sizeof(pdm_info));

	return	D_SUCCESS;
}

static void
vpdm_set_palette(int count, struct vc_color *colors)
{
	int	i;

	int 	ticks;

	ticks = nsec_to_processor_clock_ticks(260);

	*pdm_clut_addr = 0; eieio();
	tick_delay(ticks);

	for (i = 0; i < count; i++, colors++) {
		*pdm_clut_data  = colors->vp_red;
		eieio();
		*pdm_clut_data = colors->vp_green;
		eieio();
		*pdm_clut_data = colors->vp_blue;
		eieio();
		tick_delay(ticks);
	}
}

/*
 * Set the colors...
 */

io_return_t
vpdm_setcolor(int count, struct vc_color *colors)
{
	/* Wait for interrupt to come in */
	if (vpdm_int_count) {
		simple_lock(&vpdm_lock);
		vpdm_color_count = count;
		vpdm_color_ptr = colors;
		simple_unlock(&vpdm_lock);
		assert_wait((event_t) vpdm_color_ptr, TRUE);
		thread_block((void (*)(void)) 0);
	} else {
		vpdm_set_palette(count, colors);
	}

	return	D_SUCCESS;

}


/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
vpdm_setmode(struct vc_info * info)
{
	struct	screen_value *monitor;
	int	i, j, count;
	unsigned char	*modes;

	monitor = screen_values;
	count = pdm_monitor->mode_count;
	modes = pdm_monitor->modes;

	/*
	 * Figure out which mode is appropriate for 
	 * the screen dimensions passed.
	 */

	for (i = 0; i < screen_value_count; i++, monitor++) {
		for (j = 0; j < count; j++) {
			if (modes[j] == monitor->screen) {
				if (monitor->height == info->v_height
				&&  monitor->width  == info->v_width
				&&  info->v_depth <= monitor->maxdepth) 
					goto found;
			}
		}
	}

	/* Nothing is appropriate for this monitor.. */

	return	D_INVALID_OPERATION;

found:

	for (i = 0; i < depth_count; i++) {
		if (depths[i].depth == info->v_depth) 
			goto found_depth;
	}

	return D_INVALID_OPERATION;

found_depth:
	/* Setup the video mode.. */
	*pdm_video_mode = monitor->value;
	eieio();

	/* And program the pixel depth */

	*pdm_video_pixeldepth = i;
	eieio();
	delay(2);
	*pdm_clut_control = depths[i].control;
	eieio();

	pdm_info.v_height = info->v_height;
	pdm_info.v_width = info->v_width;
	pdm_info.v_depth = info->v_depth;
	pdm_info.v_rowbytes = vpdm_calculate_rowbytes(&pdm_info);

	return	D_SUCCESS;
}

/*
 * Get the current video mode..
 */

io_return_t
vpdm_getmode(struct vc_info *info)
{
	memcpy(info, &pdm_info, sizeof(*info));
	return	D_SUCCESS;
}

/*
 * Interrupt of VBL 
 */

void
vpdm_interrupt(int unit, void *arg)
{
	vpdm_int_count++;

	simple_lock(&vpdm_lock);
	if (vpdm_color_ptr)  {
		vpdm_set_palette(vpdm_color_count, vpdm_color_ptr);

		thread_wakeup((event_t) vpdm_color_ptr);
		vpdm_color_ptr = NULL;
		vpdm_color_count = 0;
	}
	simple_unlock(&vpdm_lock);
}
