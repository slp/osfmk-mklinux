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

int		ati_probe(caddr_t port);
io_return_t	ati_init(struct vc_info *);
io_return_t	ati_setcolor(int color, struct vc_color *);
io_return_t	ati_getmode(struct vc_info *);
io_return_t	ati_setmode(struct vc_info *);

volatile unsigned char *regBasePhys;
volatile unsigned char *regBase;

#define CRTC_OFF_PITCH	0x14
#define DAC_W_INDEX		0xc0
#define DAC_DATA		0xc1
#define CUR_HORZ_POSN	0x6c

struct video_board ati_video = {
	ati_init,
	ati_setcolor,
	ati_setmode,
	ati_getmode,
	NULL,
	ati_probe
};

struct vc_info			ati_info;
static device_node_t	*ati_node;

decl_simple_lock_data(,ati_lock)


/*
 * Initialize the screen.. lookup the monitor type and
 * figure out what mode it is in.
 */


static char *ati_names[] = {
        "ATY,mach64",
	"ATY,XCLAIM",
	"ATY,264VT",
        "ATY,mach64ii",
	"ATY,mach64_3D_pcc",
	NULL
};

int
ati_probe(caddr_t addr)
{
	int	i;
  	unsigned int base;

	ati_node = find_type("display");

	while (ati_node) {
		if (strncmp("ATY,", ati_node->name, 4) == 0)
			goto found;

		ati_node = ati_node->next;
	}

	for (i = 0; ati_names[i] != NULL ; i++) {
		if ((ati_node = find_devices(ati_names[i])) != NULL)
			break;
	}

	if (ati_node == NULL)
		return 0;

	/* check framebuffer address is in second 8M of aperture */

found:
	base = (unsigned int)addr;

	if( base & (8 * 1024 * 1024) == 0) {
		return 0;
	}

	regBasePhys = (unsigned char *) (base & 0xff800000) - 0x400;
	regBase = regBasePhys;

	base >>= 3;

	/* check HW base register matches */
	if( (*(regBasePhys + CRTC_OFF_PITCH) != ( base & 0xff ))
	    ||  (*(regBasePhys + CRTC_OFF_PITCH + 1) != ( (base >> 8) & 0xff )) ) {
	  	return 0;
	}

	/* move HW cursor off screen */
	*(regBasePhys + CUR_HORZ_POSN + 1) = 0xff;

	return	1;
}


io_return_t
ati_init(struct vc_info * info) 
{
	extern Boot_Video       boot_video_info;
	unsigned char		val;

	if (kernel_map) {
		regBase = (volatile unsigned char *)
			io_map((unsigned int)regBasePhys, 0x400);
		simple_lock_init(&ati_lock, ETAP_IO_TTY);
	}		

	strcpy(ati_info.v_name, ati_node->name);
	ati_info.v_width = boot_video_info.v_width;
	ati_info.v_height = boot_video_info.v_height;
	ati_info.v_depth = boot_video_info.v_depth;
	ati_info.v_rowbytes = boot_video_info.v_rowBytes;
	ati_info.v_physaddr = boot_video_info.v_baseAddr;
	ati_info.v_baseaddr = ati_info.v_physaddr;
	ati_info.v_type = VC_TYPE_PCI;

	memcpy(info, &ati_info, sizeof(ati_info));

	return	D_SUCCESS;
}

/*
 * Set the colors...
 */

io_return_t
ati_setcolor(int count, struct vc_color *colors)
{
	int	i;

	simple_lock(&ati_lock);

	*(regBase + DAC_W_INDEX)  = 0;
	eieio();

	for (i = 0; i < count ;i++, colors++) {
		*(regBase + DAC_DATA) = colors->vp_red;
		eieio();
		*(regBase + DAC_DATA) = colors->vp_green;
		eieio();
		*(regBase + DAC_DATA) = colors->vp_blue;
		eieio();
	}
	simple_unlock(&ati_lock);

	return	D_SUCCESS;
}

/*
 * Set the video mode based on the screen dimensions
 * provided. 
 */

io_return_t
ati_setmode(struct vc_info * info)
{
	return	D_INVALID_OPERATION;
}

/*
 * Get the current video mode..
 */

io_return_t
ati_getmode(struct vc_info *info)
{
	memcpy(info, &ati_info, sizeof(*info));

	return	D_SUCCESS;
}
