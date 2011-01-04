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
 * Copyright (c) 1991 The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: pdcdump.c 1.7 92/05/22$
 *	Author: Leigh Stoller, Jeff Forys, University of Utah CSS
 */
/*
 * PDC support routines:
 *
 * pdcboot()	- load boot code from disk and execute.
 * pdcdump()	- dump kernel memory to disk.
 */

#ifdef MACH_KERNEL
#include <types.h>
#include <device/param.h>
#include <machine/machparam.h>
#include <machine/asm.h>
#include <machine/pdc.h>
#include <machine/iodc.h>
#include <machine/iomod.h>
#include <machine/iplhdr.h>
#else
#include "param.h"
#include "machine/pdc.h"
#include "machine/iodc.h"
#include "machine/iomod.h"
#include "../hp800stand/iplhdr.h"
#endif

#include <hp_pa/misc_protos.h>

#define	BT_HPA		PAGE0->mem_boot.pz_hpa
#define	BT_SPA		PAGE0->mem_boot.pz_spa
#define	BT_LAYER	PAGE0->mem_boot.pz_layers
#define	BT_IODC		PAGE0->mem_boot.pz_iodc_io
#define	BT_CLASS	PAGE0->mem_boot.pz_class

#define	MINIOSIZ	64		/* minimum buffer size for IODC call */
#define	MAXIOSIZ	(64 * 1024)	/* maximum buffer size for IODC call */
#define	IOREQALN	2048		/* alignment for user read requests */

#define	READLOOP(dev, mem, tot) {					\
	register u_int rdloc = (dev), wrloc = (mem), tcnt = (tot);	\
	register u_int n;						\
	do {								\
		if ((*btiodc)(BT_HPA, IODC_IO_BOOTIN, BT_SPA, BT_LAYER,	\
		              pdcret, rdloc, wrloc, MIN(tcnt,MAXIOSIZ),	\
		              MAXIOSIZ) < 0)				\
			goto bad;					\
		n = *pdcret; rdloc += n; wrloc += n; tcnt -= n;		\
	} while ((int)tcnt > 0);					\
}

/*
 * Load a boot program at `jefboot' (using the PDC) and execute it.
 * On failure, the hardware is reset causing the old, slow reboot.
 *
 * The grand assumption we make is that the boot device listed in
 * PAGE0 also houses a boot program.  We also assume that no one has
 * screwed with PAGE0 (which should be safe).
 *
 * This routine must be called in real mode, with interrupts disabled.
 * For a complete list of constraints, check the routine that calls it.
 */
void
pdcboot(
	int interactive,
	void (*jefboot)(int))
{
	char iplbuf[sizeof(struct ipl_image) + 64 + IOREQALN];
	struct ipl_image *iplptr;
	int *pdcret, pdcbuf[33];
	int (*btiodc)(struct iomod*, ...);

	fcacheall();

	/*
	 * Get our PDC entry point and pointer to IODC boot code.
	 */
	pdc = PAGE0->mem_pdc;
	if ((btiodc = BT_IODC) == 0)
		goto bad;

	/*
	 * Buffer alignment.
	 */
	iplptr = (struct ipl_image *) ((((u_int)iplbuf) + 63) & ~63);
	pdcret = (int *) ((((u_int)pdcbuf) + 7) & ~7);

	/*
	 * Read the boot device initialization code into memory,
	 * Initialize the boot module/device, and
	 * Load the boot device I/O code into memory.
	 */
	if ((*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, BT_HPA, IODC_INIT,
	           btiodc, IODC_MAXSIZE) < 0)
		goto bad;

	if ((*btiodc)(BT_HPA, IODC_INIT_ALL, BT_SPA, BT_LAYER, pdcret,
	              0,0,0,0) < 0)
		goto bad;

	if ((*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, BT_HPA, IODC_IO,
	           btiodc, IODC_MAXSIZE) < 0)
		goto bad;

	/*
	 * Load the IPL header from the disk, followed by the boot program.
	 * Calculate the entry point into the boot code.
	 */
	READLOOP(0, (u_int)iplptr, roundup(sizeof(struct ipl_image),IOREQALN));
	READLOOP(iplptr->ipl_addr, (u_int)jefboot, iplptr->ipl_size);
	jefboot = (void (*)(int)) ((u_int)jefboot + iplptr->ipl_entry);

	fcacheall();		/* for split I & D cache's (e.g. hp720) */
#ifndef MACH_KERNEL		/* TLB config data not saved at boot time */
	pgtlbs();		/* emulate PDC behavior as best we can */
#endif

	/*
	 * Launch the boot code!
	 */
	(*jefboot)(interactive);
	/*NOTREACHED*/

bad:
	/*
	 * If anything went wrong above, we end up here.  Since we trashed
	 * kernel memory (starting at `jefboot'), all we can do here is a
	 * hard reboot... which is what we would have done anyway!
	 */
	BT_IODC = (int (*)(struct iomod*, ...)) 0;
	(*(struct iomod *)LBCAST_ADDR).io_command = CMD_RESET;
}


#ifdef PDCDUMP
/*
 * Dump memory into the swap partition of the primary boot device. The
 * config code will compute the offset from the start of the disk, rather
 * then the start of the swap partition since this code does not know
 * anything about partitions.
 */
void
pdcdump(void)
{
	extern int dumpsize, dumpoffset;
	static int pdcbuf[33];
	int *pdcret = (int *) ((((u_int)pdcbuf) + 7) & ~7);
	int (*btiodc)(struct iomod*, ...);

	if (dumpoffset == -1)		/* initialized? */
		goto bad;

	fcacheall();

	pdc = PAGE0->mem_pdc;
	if ((btiodc = BT_IODC) == 0)
		goto bad;

#ifdef USELEDS
	ledcontrol(0xAA, 0x55, 0);
#endif

	/* Read the initialization code */
	if ((*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, BT_HPA, IODC_INIT,
	           (int) btiodc, IODC_MAXSIZE) < 0)
		goto bad;

	/* Initialize the module and the device */
	if ((*btiodc)(BT_HPA, IODC_INIT_ALL, BT_SPA, BT_LAYER, pdcret,
	              0,0,0,0) < 0)
		goto bad;

	/* Read the the io code */
	if ((*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, BT_HPA, IODC_IO,
	           (int) btiodc, IODC_MAXSIZE) < 0)
		goto bad;

	/*
	 * Write memory to disk; write it all at once if possible,
	 * o/w loop until done.
	 *
	 * PDC restrictions:
	 *	dumpsize - multiple of 2K bytes (guaranteed)
	 *	dumpoffset - 2K byte aligned (not enforced!)
	 */
	{
		register u_int rdloc = 0;
		register u_int wrloc = dumpoffset * DEV_BSIZE;
		register u_int tcnt = ctob(dumpsize);
		register u_int n;

		do {
#ifdef USELEDS
			ledcontrol(0, 0, 0xFF);
#endif
			if ((*btiodc)(BT_HPA,IODC_IO_BOOTOUT,BT_SPA,BT_LAYER,
			              pdcret,wrloc,rdloc,MIN(tcnt,MAXIOSIZ))<0)
				goto bad;
			n = *pdcret; rdloc += n; wrloc += n; tcnt -= n;
		} while ((int)tcnt > 0);
	}

#ifdef USELEDS
	ledcontrol(0, 0xFF, 0);
#endif

bad:
	/* reboot */
	(*(struct iomod *)LBCAST_ADDR).io_command = CMD_RESET;
}
#endif /* PDCDUMP */



