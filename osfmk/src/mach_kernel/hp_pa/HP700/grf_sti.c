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
 * Copyright (c) 1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *	Utah $Hdr: grf_sti.c 1.14 94/12/16$
 */

#include "grf.h"
#if NGRF > 0

/*
 * Graphics routines for STI frame buffer devices.
 */
#include <types.h>
#ifdef	MACH_KERNEL
#include <machine/machparam.h>
#include <device/errno.h>
#include <hp_pa/HP700/grfioctl.h>
#include <hp_pa/HP700/grfvar.h>
#include <hp_pa/HP700/grfreg.h>
#else
#include <sys/param.h>
#include <sys/errno.h>
#include <hp_pa/HP700/grfioctl.h>
#include <hp_pa/HP700/grfvar.h>
#include <hp_pa/HP700/grfreg.h>
#endif

#include <machine/cpu.h>
#include <machine/iomod.h>

#include <kern/misc_protos.h>
#include <hp_pa/c_support.h>

/*
 * Forwards
 */
int reset_sti_device(struct grfdev *);
int sti_validate(struct grfinfo *gi);
#if DEBUG
void dump_sti_regions(struct sti_region *rp);
#endif

/*
 * Externs
 */
unsigned long strlen(register const char *string);

/*
 * Initialize hardware.
 * Must fill in the grfinfo structure in g_softc.
 */
int
sti_init(struct grf_softc *gp)
{
	int i, j, base, offset;
	struct grfdev *gd = (struct grfdev *) gp->g_data;
	struct grfinfo *gi = &gp->g_display;
	struct sti_inquireflags flags;
	struct sti_inquirein inputs;
	struct sti_inquireout outputs;
	char *cp;

	/*
 	 * Lets grab the region list so where know where the framebuffer and
	 * control spaces are located.
	 */
	cp = (char *) &gd->regions;
	base = (int)gd->romaddr;
	offset = STI_MMAP(gd->type, gd->romaddr);
	for (i = 0; i < STI_REGIONS; i++) {
		for (j = 0; j < sizeof(struct sti_region); ) {
			if (gd->type == STI_TYPE_BWGRF) {
				*cp = *(char *)(base + offset);
				cp++;
				j++;
			} else {
				*(int *)cp = *(int *)(base + offset);
				cp += sizeof(int);
				j += sizeof(int);
			}
			offset += sizeof(int);
		}
	}
#if 0 /* def DEBUG */
	dump_sti_regions(&gd->regions);
#endif
	/*
	 * Reset the STI device. This will fill in the "global config" info.
	 */
	if (!reset_sti_device(gd))
		return(0);
	
	/*
	 * Not all the info we need is returned in the config block, so we 
	 * call the "inquire config" routine to get it.
	 */
	bzero((char *)&flags, sizeof flags);
	flags.wait = 1;
	bzero((char *)&inputs, sizeof inputs);
	(*gd->ep->inq_conf)(&flags, &inputs, &outputs, &gd->config);

	gi->gd_id = STI_ID_HI(gd->type, gd->romaddr);
	gi->gd_regaddr = (caddr_t) gd->config.regions[1];
	gi->gd_regsize = gd->regions[2].length << PGSHIFT;
	gi->gd_fbaddr = (caddr_t) gd->config.regions[2];
	gi->gd_fbsize = gd->regions[1].length << PGSHIFT;
	gi->gd_fbwidth = outputs.fbwidth;
	gi->gd_fbheight = outputs.fbheight;
	gi->gd_dwidth = outputs.dwidth;
	gi->gd_dheight = outputs.dheight;
	gi->gd_planes = outputs.planes;
	gi->gd_colors = 1 << gi->gd_planes;
	bcopy(outputs.devname, gd->devname, strlen(outputs.devname));

	return(sti_validate(gi));
}

/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 *
 * XXX might be called with frame buffer mapped (i.e. PID assigned/enabled)
 * so we must be sure the kernel can access the STI ROM.  We do this
 * the same way the ITE does for now, by turning of PID checks.
 * WARNING: this assumes the ROM routines are well behaved!
 */
#include <machine/psw.h>

/*ARGSUSED*/
int
sti_mode(
	 struct grf_softc *gp,
	 int cmd,
	 caddr_t data)
{
	struct grfdev *gd = (struct grfdev *)gp->g_data;
	int error = 0;
	int _om_;

	switch (cmd) {
	case GM_GRFON:
	case GM_GRFOFF:
	{
		struct sti_initflags flags;
		struct sti_initin inputs;
		struct sti_initout outputs;

		bzero((caddr_t)&flags, sizeof flags);
		flags.wait = 1;
		if (cmd == GM_GRFON) {
			flags.no_change_text = 1;
			flags.graphon = 1;
		} else {
			flags.no_change_graph = 1;
			flags.texton = 1;
			flags.init_text_cmap = 1;
		}
		flags.no_change_bet = 1;
		flags.no_change_bei = 1;
		bzero((caddr_t)&inputs, sizeof inputs);	
		inputs.text_planes = 1;
		bzero((caddr_t)&outputs, sizeof outputs);
		_om_ = rsm(PSW_P);
		(*gd->ep->init_graph)(&flags, &inputs, &outputs, &gd->config);
		if (_om_ & PSW_P) ssm(_om_);
		if (outputs.errno)
			error = EIO;
		break;
	}

#ifndef MACH_KERNEL
	/* XXX enable/disable the mapping for this process, 'PUX method */
	case GM_MAP:
		error = ioblk_grant((caddr_t)data);
		break;

	case GM_UNMAP:
		(void) ioblk_remove(gp->g_display.gd_regaddr+0x8000);
		break;
#endif

	case GM_DESCRIBE:
	{
		struct grf_fbinfo *fi = (struct grf_fbinfo *)data;
		struct grfinfo *gi = &gp->g_display;
		int i, j;
		struct sti_inquireflags flags;
		struct sti_inquirein inputs;
		struct sti_inquireout outputs;

		flags.wait = 1;
		_om_ = rsm(PSW_P);
		(*gd->ep->inq_conf)(&flags, &inputs, &outputs, &gd->config);
		if (_om_ & PSW_P) ssm(_om_);

		/* feed it what HP-UX expects */
		fi->id = gi->gd_id;
		fi->mapsize = gi->gd_fbsize;
		fi->dwidth = outputs.dwidth;
		fi->dlength = outputs.dheight;
		fi->width = outputs.fbwidth;
		fi->length = outputs.fbheight;
		fi->xlen = (outputs.fbwidth * outputs.bpp / 8)
			* 4;	/* XXX word per pixel? */
		fi->bpp = outputs.bpp;
		fi->bppu = outputs.bits;
		fi->npl = outputs.planes;
		fi->nplbytes = outputs.fbheight * outputs.fbwidth;
		bcopy(outputs.devname, fi->name, 32);
		fi->attr = outputs.attributes;
		/*
		 * Copy all regions
		 */
		for (i = 0, j = 0; i < STI_REGIONS; i++) {
			if (i == 1)
				fi->fbbase = (caddr_t)gd->config.regions[i];
			else if (i == 2)
				fi->regbase = (caddr_t)gd->config.regions[i];
			else if (!gd->regions[i].sysonly &&
				 gd->regions[i].length)
				fi->regions[j++] =
					(caddr_t)gd->config.regions[i];
		}
		break;
	}

	default:
		error = EINVAL;
		break;
	}
	return(error);
}

/*
 * Validate the PDE entries for the framebuffer.
 * XXX uses 'PUX method for now.
 */
int
sti_validate(struct grfinfo *gi)
{
#ifndef	MACH_KERNEL
	struct {
		caddr_t vaddr;
		int size;
	} blk_parm;

	/*
	 * We will need to map one magic page of control space and the
	 * entire framebuffer.
	 */
	blk_parm.vaddr = (caddr_t)((int)gi->gd_regaddr & 0xFC000000);
	blk_parm.size = 0x2000000;
	if (ioblk_create(&blk_parm, 1)) {
		printf("sti_validate: ioblk_create(%x-%x) failed\n",
		       blk_parm.vaddr, blk_parm.vaddr+blk_parm.size);
		return(0);
	}
#endif
	return(1);
}

/*
 * Hard reset an STI graphics device. Need to fill in a bunch of structures,
 * and then call the STI code.
 */
int
reset_sti_device(struct grfdev *gd)
{
	struct {
		struct sti_initflags flags;
		struct sti_initin input;
		struct sti_initout output;
	} iparm;
	int i, base;
	
	/*
	 * Fill in the configuration block and reset the device. 
	 */
	bzero((char *)&gd->config, sizeof gd->config);
	for (i = 0; i < STI_REGIONS; i++) {
		/*
		 * Region 0 is relative to the STI ROM,
		 * all others are relative to the HPA.
		 */
		base = (i == 0) ? (int)gd->romaddr : (int)gd->hpa;
		gd->config.regions[i] = base + (gd->regions[i].offset<<PGSHIFT);
		if (gd->regions[i].last)
			break;
	}
	bzero((char *)&iparm, sizeof iparm);
	iparm.flags.wait = 1;
	iparm.flags.hardreset = 1;
	iparm.flags.clear = 1;
	iparm.flags.cmap_black = 1;
	iparm.flags.bus_error_timer = 1;
	iparm.input.text_planes = 1; /* White on Black */
	(*gd->ep->init_graph)(&iparm.flags, &iparm.input, &iparm.output,
			      &gd->config);
	if (iparm.output.text_planes != 1 || iparm.output.errno) {
		printf("Could not initialize STI device\n");
		return(0);
	}
	return(1);
}

#ifdef DEBUG
void
dump_sti_regions(struct sti_region *rp)
{
	int i;

	for (i = 0; i < STI_REGIONS; i++) {
		printf("region%d: addr0x%x, sys%d, cache%d, btlb%d, size%d\n",
		       i, rp->offset, rp->sysonly, rp->cache, rp->btlb,
		       rp->length);
		if (rp->last)
			break;
		rp++;
	}
}
#endif
#endif
