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
 *	Utah $Hdr: grf_machdep.c 1.10 94/12/14$
 */

#include "grf.h"
#if NGRF > 0

#ifdef	MACH_KERNEL
#include <types.h>
#include <machine/machparam.h>
#include <mach/vm_prot.h>
#include <machine/iomod.h>
#include <hp_pa/HP700/grfioctl.h>
#include <hp_pa/HP700/grfvar.h>
#include <hp_pa/HP700/grfreg.h>
#include <hp_pa/HP700/device.h>
#else
#include <sys/param.h>
#include <sys/vm.h>
#include <machine/pde.h>
#include <machine/iomod.h>
#include <hp_pa/HP700/grfioctl.h>
#include <hp_pa/HP700/grfvar.h>
#include <hp_pa/HP700/grfreg.h>
#include <hp_pa/HP700/device.h>
#endif

#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/HP700/bus_protos.h>

struct sti_entry stientry[NGRF];
char sticode[(STI_CODESIZ * STI_CODECNT * NGRF) + NBPG*2];

#define	STICODE_ALGN	(((u_int)&sticode[0] + NBPG-1) & ~(NBPG-1))

/* Display specific data, indexed by unit number. */
struct grfdev grfdev[NGRF];

/* Forwards */
int grfprobe(struct hp_device *);
int grfattach(struct hp_device *);
char *stiload(void (**)(void*, void*, void*, struct sti_config*),
	      struct grfdev *, int, int, char *);

/*
 * We need grfconfig() for backwards compatability with the ITE driver.
 */
void
grfconfig(void)
{
	struct hp_device hd;
	struct modtab *mptr;
	int noinit;
	int unit = 0;
	static int didconfig = 0;

	if (didconfig)
		return;

	for (noinit = 0; mptr = getmod(IODC_TP_FIO, -1, noinit); noinit = 1) {
		switch (mptr->mt_type.iodc_sv_model) {
		case SVMOD_FIO_SGC:
			/*
			 * XXX: The Medusa FDDI card looks just like a GRF.
			 */
			if (STI_ID_HI(STI_TYPE_BWGRF, mptr->m_hpa) == STI_ID_FDDI)
				continue;
			/* fall into... */
		case SVMOD_FIO_GGRF:
			if (unit == NGRF) {
				printf("grf%d: can't config (NGRF exceeded)\n",
				       unit);
				continue;
			}
			hd.hp_unit = unit++;
			hd.hp_addr = (char *)mptr;
			if (grfprobe(&hd))
				grfattach(&hd);
			break;

		default:
			break;
		}
	}

	/*
	 * XXX right now we are called for the first time from cnprobe
	 * before businit() has been called.  Hence getmod() will fail
	 * (return 0) and we should try again later.
	 */
	if (noinit)
		didconfig++;
}

int
grfprobe(struct hp_device *hd)
{
	struct modtab *mptr = (struct modtab *)hd->hp_addr;
	register char *rom;
	u_char devtype;
	u_int codemax;

	/*
	 * Locate STI ROM.
	 * On some machines it may not be part of the HPA space.
	 * On these, busconf will stash the address in m_stirom.
	 */
	rom = (char *)mptr->m_stirom;
	if (rom == 0)
		rom = (char *)mptr->m_hpa;

	/*
	 * Get STI ROM device type and make sure we have a graphics device.
	 */
	devtype = STI_DEVTYP(STI_TYPE_BWGRF, rom);
	if (devtype != STI_TYPE_BWGRF && devtype != STI_TYPE_WWGRF) {
		printf("grf%d: not a graphics device (device type:%x)\n",
		       hd->hp_unit, devtype);
		return(0);
	}

	/*
	 * Determine whether or not we have enough room for the code.
	 * We should, since no STI routine can excede STI_CODESIZ bytes.
	 */
	codemax = (STI_EADDR(devtype, rom) - STI_IGADDR(devtype, rom))
		/ sizeof(int);
	if (codemax > STI_CODESIZ * STI_CODECNT) {
		printf("grf%d: can't load STI code (too large: %u bytes)\n",
		       hd->hp_unit, codemax);
		return(0);
	}
	return(1);
}

int
grfattach(struct hp_device *hd)
{
	register char *rom;
	register struct sti_entry *ep;
	register char *cp;
	struct modtab *mptr = (struct modtab *)hd->hp_addr;
	struct grf_softc *gp = &grf_softc[hd->hp_unit];
	struct grfdev *gd = &grfdev[hd->hp_unit];
	int devtype;
	static int firstime = 1;

	if (gp->g_flags & GF_ALIVE)
		return(1);

	/*
	 * Locate STI ROM.
	 * On some machines it may not be part of the HPA space.
	 * On these, busconf will stash the address in m_stirom.
	 */
	rom = (char *)mptr->m_stirom;
	if (rom == 0)
		rom = (char *)mptr->m_hpa;

	/*
	 * Change page protection on `sticode' to KERNEL:rwx USER:rx.
	 * At this time, I dont know if users will be executing these
	 * routines; for now we'll give them permission to do so.
	 */
	if (firstime) {
#ifdef MACH_KERNEL
		pmap_map(STICODE_ALGN, STICODE_ALGN,
			 STICODE_ALGN + (STI_CODESIZ * STI_CODECNT * NGRF),
			 VM_PROT_READ | VM_PROT_EXECUTE | VM_PROT_WRITE);
#else
		register u_int pg = btop(STICODE_ALGN);
		register u_int pgcnt =
			(STI_CODESIZ * STI_CODECNT * NGRF + (NBPG-1)) / NBPG;

		while (pgcnt--)
			setaccess(pg++, PDE_AR_URXKW, 0, PDEAR);
#endif
		firstime = 0;
	}

	devtype = STI_DEVTYP(STI_TYPE_BWGRF, rom);

	/*
	 * Set addrs and type for stiload
	 */
	gd->romaddr = rom;
	gd->hpa = (char *)mptr->m_hpa;
	gd->type = devtype;

	/*
	 * Set `ep' to unit's STI routine entry points and `cp' to
	 * page-aligned code space.  Load STI routines and be sure
	 * to flush the (data) cache afterward; we actually flush
	 * both caches as we only call this routine a couple times.
	 */
	ep = &stientry[hd->hp_unit];
	cp = (char *) (STICODE_ALGN + STI_CODESIZ * STI_CODECNT * hd->hp_unit);

	cp = stiload(&ep->init_graph, gd,
		     STI_IGADDR(devtype, rom), STI_SMADDR(devtype, rom), cp);
	cp = stiload(&ep->state_mgmt, gd,
		     STI_SMADDR(devtype, rom), STI_FUADDR(devtype, rom), cp);
	cp = stiload(&ep->font_unpmv, gd,
		     STI_FUADDR(devtype, rom), STI_BMADDR(devtype, rom), cp);
	cp = stiload(&ep->block_move, gd,
		     STI_BMADDR(devtype, rom), STI_STADDR(devtype, rom), cp);
	cp = stiload(&ep->self_test, gd,
		     STI_STADDR(devtype, rom), STI_EHADDR(devtype, rom), cp);
	cp = stiload(&ep->excep_hdlr, gd,
		     STI_EHADDR(devtype, rom), STI_ICADDR(devtype, rom), cp);
	cp = stiload(&ep->inq_conf, gd,
		     STI_ICADDR(devtype, rom), STI_EADDR(devtype, rom), cp);

	fcacheall();

	gd->ep = &stientry[hd->hp_unit];
	gp->g_data = (caddr_t) gd;
	gp->g_sw = &grfsw[0];
	if ((*gp->g_sw->gd_init)(gp) == 0) {
		gp->g_data = (caddr_t) 0;
		return(0);
	}

	gp->g_flags = GF_ALIVE;
	return(1);
}

/*
 * stiload(funcp, gd, soff, eoff, dest)
 *
 * Load this device's ROM code (from `soff' to `eoff') at `dest'.
 * Set `funcp' to `dest' if code was successfully loaded, 0 o/w.
 * N.B. Each 32-bit word of ROM represents 8 bits of an instruction.
 *
 * Return value: the next usable word of memory after `dest'.
 */
char *
stiload(void (**funcp)(void*, void*, void*, struct sti_config*),
	struct grfdev *gd,
	int soff, 
	int eoff,
	char *dest)
{
	register char *rom = gd->romaddr;
	register u_int bcnt;
	register char *dptr;

	bcnt = eoff - soff;
	if (gd->type == STI_TYPE_BWGRF)
		bcnt /= sizeof(int);

	if (soff >= eoff || bcnt % sizeof(int) != 0) {
		printf("grf%d: STI ROM invalid (soff:%x eoff:%x dest:%x)\n",
		       gd-grfdev, soff, eoff, dest);
		*funcp = (void (*)(void*, void*, void*, struct sti_config*)) 0;
		return(dest);
	}

	if (gd->type == STI_TYPE_BWGRF) {
		dptr = dest;
		while (bcnt--) {
			*dptr++ = *(char *)((int)rom + soff);
			soff += sizeof(int);
		}
	} else {
		register int *iptr = (int *)dest;

		bcnt /= sizeof(int);
		while (bcnt--) {
			*iptr++ = *(int *)((int)rom + soff);
			soff += sizeof(int);
		}
		dptr = (char *)iptr;
	}

	*funcp = (void (*)(void*, void*, void*, struct sti_config*)) (u_int)dest;
	return(dptr);
}
#endif	/* NGRF > 0 */
