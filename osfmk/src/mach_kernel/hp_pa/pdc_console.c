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
 * Copyright (c) 1990, 1991 The University of Utah and
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
 * 	Utah $Hdr$
 */

/*
 *	Routines to use as a console device the PDC left over
 *	from the boot program.
 */

#include <machine/pdc.h>
#include <machine/iodc.h>
#include <machine/iomod.h>
#include <kern/misc_protos.h>

extern int vm_on;
extern int pdc_alive;

/*
 * all of these pointers are based on what the boot program left around for
 * us to use
 */
#define	KY_HPA		PAGE0->mem_kbd.pz_hpa
#define	KY_SPA		PAGE0->mem_kbd.pz_spa
#define	KY_LAYER	PAGE0->mem_kbd.pz_layers
#define	KY_IODC		PAGE0->mem_kbd.pz_iodc_io
#define	KY_CLASS	PAGE0->mem_kbd.pz_class

#define	CN_HPA		PAGE0->mem_cons.pz_hpa
#define	CN_SPA		PAGE0->mem_cons.pz_spa
#define	CN_LAYER	PAGE0->mem_cons.pz_layers
#define	CN_IODC		PAGE0->mem_cons.pz_iodc_io
#define	CN_CLASS	PAGE0->mem_cons.pz_class

#define	MINIOSIZ	64	        /* minimum buffer size for IODC call */
static char cnbuf[MINIOSIZ * 2];
static char *cndata = 0;

static int *pdcret = 0;
static int pdcbuf[33];
static int (*cniodc)(int, ...) = (int (*)(int, ...)) 0; /* console IODC entry point */
static int (*kyiodc)(int, ...) = (int (*)(int, ...)) 0;  /* console IODC entry point */

void
pdc_console_init(void)
{
	pdcret = (int *)((((int) pdcbuf) + 7) & ~7);
	cndata = (char *) ((((int) &cnbuf[0]) + MINIOSIZ-1) & ~(MINIOSIZ-1));

	/*
	 * the boot program loaded the IODC code for the console at mem_free
	 */
	cniodc = (int (*)(int, ...)) PAGE0->mem_free;
	kyiodc = (int (*)(int, ...)) PAGE0->mem_free;

	pdc_alive = 1;
}

/*
 * if virtual memory is off then we can just call the routines directly. When
 * VM is on, we have to use the routine pdc_iodc() which turns off VM and puts
 * us in a state that we can make the iodc call.
 */
int
pdcgetc(void)
{
	int err;

	if (!vm_on)
		err = (*kyiodc)((int)KY_HPA, IODC_IO_CONSIN, KY_SPA, KY_LAYER,
				pdcret, 0, cndata, 1, 0);
	else
		err = pdc_iodc(kyiodc, 0, KY_HPA, IODC_IO_CONSIN, KY_SPA, 
			       KY_LAYER, pdcret, 0, cndata, 1, 0);

	if (err < 0)
		printf("KBD input error: %d\n", err);

	if (pdcret[0] == 0)
		return (0);

	if (pdcret[0] > 1)
		printf("KBD input got too much (%d)\n", pdcret[0]);

	return (int) *cndata;
}

void
pdcputc(char c)
{
	*cndata = c;

	if (!vm_on)
		(void) (*cniodc)((int)CN_HPA, IODC_IO_CONSOUT, CN_SPA, CN_LAYER,
				 pdcret, 0, cndata, 1, 0);
	else
		pdc_iodc(cniodc, 0, CN_HPA, IODC_IO_CONSOUT, CN_SPA, CN_LAYER,
			 pdcret, 0, cndata, 1, 0);
}



