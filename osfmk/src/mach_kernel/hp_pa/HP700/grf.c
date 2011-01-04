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
 * Copyright (c) 1988-1994, The University of Utah and
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
 *	Utah $Hdr: grf.c 1.41 94/12/14$
 */

/*
 * Graphics display driver for HP 300/400/700/800 machines.
 * This is the hardware-independent portion of the driver.
 * Hardware access is through the machine dependent grf switch routines.
 */

#include "grf.h"
#if NGRF > 0

#include <types.h>
#include <sys/ioctl.h>
#include <device/tty.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <kern/spl.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <hp_pa/HP700/grfioctl.h>
#include <hp_pa/HP700/grfvar.h>
#include <hp_pa/HP700/itevar.h>

#include <machine/cpu.h>
#include <kern/misc_protos.h>

#include "ite.h"
#if NITE == 0
#define	iteon(u,f)
#define	iteoff(u,f)
#endif

struct grf_softc grf_softc[NGRF];

#ifdef DEBUG
int grfdebug = 0;
#define GDB_DEVNO	0x01
#define GDB_MMAP	0x02
#define GDB_IOMAP	0x04
#define GDB_LOCK	0x08
#endif

/* Forwards */
int grfopen(dev_t, int);
void grfclose(dev_t, int);
io_return_t grfgetstat(dev_t, int, dev_status_t, natural_t *);
io_return_t grfsetstat(dev_t, int, dev_status_t, natural_t);
int grfmap(dev_t, int, int);
int grfon(dev_t);
int grfoff(dev_t);
int grfaddr(struct grf_softc *, int);
int grfmmap(dev_t, caddr_t*);
int grfunmmap(dev_t, caddr_t);

/*ARGSUSED*/
int 
grfopen(dev_t dev, int flags)
{
	int unit = GRFUNIT(dev);
	register struct grf_softc *gp = &grf_softc[unit];
	int error = 0;

	if (unit >= NGRF || (gp->g_flags & GF_ALIVE) == 0)
		return(ENXIO);
	if ((gp->g_flags & (GF_OPEN|GF_EXCLUDE)) == (GF_OPEN|GF_EXCLUDE))
		return(EBUSY);

	/*
	 * First open.
	 * XXX: always put in graphics mode.
	 */
	error = 0;
	if ((gp->g_flags & GF_OPEN) == 0) {
		gp->g_flags |= GF_OPEN;
		error = grfon(dev);
	}
	return(error);
}

/*ARGSUSED*/
void
grfclose(dev_t dev, int flags)
{
	register struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];

	(void) grfoff(dev);
	gp->g_flags &= GF_ALIVE;
}

io_return_t
grfgetstat(
	dev_t		dev,
	int		flavor,
	dev_status_t	data,		/* pointer to OUT array */
	natural_t	*count)
{
	struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];
	int error;

	switch (flavor) {
	case GCDESCRIBE:
		error = (*gp->g_sw->gd_mode)(gp, GM_DESCRIBE, (caddr_t)data);
		break;
	default:
		error = EINVAL;
	}
	
	*count = ((flavor >> 16) & 0x7F) / sizeof(int);
	return(error);
}

io_return_t
grfsetstat(
	dev_t		dev,
	int		flavor,
	dev_status_t	data,
	natural_t	count)
{
	struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];
	int error;

	switch (flavor) {
	case GCMAP:
		/* map in control regs and frame buffer */
		error = grfmmap(dev, (caddr_t*)data);
		break;

	case GCUNMAP:
		error = grfunmmap(dev, *(caddr_t*)data);
		break;

	default:
		error = EINVAL;
	}

	return error;
}

/*ARGSUSED*/
int
grfmap(dev_t dev, int off, int prot)
{
	return(grfaddr(&grf_softc[GRFUNIT(dev)], off));
}

int
grfon(dev_t dev)
{
	int unit = GRFUNIT(dev);
	struct grf_softc *gp = &grf_softc[unit];

	/*
	 * XXX: iteoff call relies on devices being in same order
	 * as ITEs and the fact that iteoff only uses the minor part
	 * of the dev arg.
	 */
	iteoff(unit, 3);
	return((*gp->g_sw->gd_mode)(gp, GM_GRFON, (caddr_t)0));
}

int 
grfoff(dev_t dev)
{
	int unit = GRFUNIT(dev);
	struct grf_softc *gp = &grf_softc[unit];
	int error;

	error = (*gp->g_sw->gd_mode)(gp, GM_GRFOFF, (caddr_t)0);
	/* XXX: see comment for iteoff above */
	iteon(unit, 2);
	return(error);
}

int 
grfaddr(
	struct grf_softc *gp,
	int off)
{
	/* bogus */
	return(-1);
}

int 
grfmmap(dev_t dev, caddr_t *addrp)
{
	struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];
	int len;

#ifdef DEBUG
	if (grfdebug & GDB_MMAP)
		printf("grfmmap(%d): addr %x\n", current_thread(), *addrp);
#endif
	*addrp = (caddr_t)((int)gp->g_display.gd_regaddr & 0xFC000000);
	len = 0x2000000;
#ifndef IO_HACK
	if (!pmap_iomap((vm_offset_t)*addrp))
		return(EINVAL);
#endif
	return(0);
}

int
grfunmmap(dev_t dev, caddr_t addr)
{
	struct grf_softc *gp = &grf_softc[GRFUNIT(dev)];

#ifdef DEBUG
	if (grfdebug & GDB_MMAP)
		printf("grfunmmap(%d): id %d addr %x\n",
		       current_thread(), minor(dev), addr);
#endif
	addr = (caddr_t)((int)gp->g_display.gd_regaddr & 0xFC000000);
#ifndef IO_HACK
	pmap_iounmap((vm_offset_t)addr);
#endif
	return(0);
}

#endif	/* NGRF > 0 */
