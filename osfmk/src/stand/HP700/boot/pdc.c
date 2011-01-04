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
 * Copyright (c) 1990 mt Xinu, Inc.  All rights reserved.
 * Copyright (c) 1990 University of Utah.  All rights reserved.
 *
 * This file may be freely distributed in any form as long as
 * this copyright notice is included.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	Utah $Hdr: pdc.c 1.8 92/03/14$
 */

#include <sys/param.h>
#include "pdc.h"
#include "nvm.h"
#include "reboot.h"

/*
 * Interface routines to initialize and access the PDC.
 */

int (*pdc)() = 0;
static	int pdcbuf[33];		/* PDC return buffer */
int	*pdcret = 0;			/* double-aligned ptr into `pdcbuf' */
struct	stable_storage sstor = { 0 };	/* contents of Stable Storage */
int	sstorsiz = 0;			/* size of Stable Storage */

extern u_int howto, bootdev, endaddr;

/*
 * Initialize PDC and related variables.
 * The console is also initialized here (by a call to cninit()).
 */
pdc_init(interactive, enda)
u_int interactive, enda;
{
	static u_int chasdata;
	int err;

	/*
	 * Initialize important global variables (defined above).
	 */
	pdc = PAGE0->mem_pdc;
	pdcret = (int *)((((int) pdcbuf) + 7) & ~7);

	err = (*pdc)(PDC_STABLE, PDC_STABLE_SIZE, pdcret, 0, 0);
	if (err >= 0) {
		sstorsiz = MIN(pdcret[0],sizeof(sstor));
		err = (*pdc)(PDC_STABLE, PDC_STABLE_READ, 0, &sstor, sstorsiz);
	}

	cninit();

	/*
	 * Now that we (may) have an output device, if we encountered
	 * an error reading Stable Storage (above), let them know.
	 */
	if (err)
		printf("Stable storage PDC_STABLE Read Ret'd %d\n", err);

	btinit();

	/*
	 * Clear the FAULT light (so we know when we get a real one)
	 */
	chasdata = PDC_OSTAT(PDC_OSTAT_BOOT) | 0xCEC0;
	(void) (*pdc)(PDC_CHASSIS, PDC_CHASSIS_DISP, chasdata);
#if 0
	pdc_busconf(MAXMODBUS, -1, -1, "");
#endif
	
	if(interactive)
	  howto |= RB_ASKNAME;

	endaddr = enda;
}

/*
 * Read in `bootdev' and `howto' from Non-Volatile Memory.
 */
getbinfo()
{
	static struct bootdata bd;
	static int bdsize = sizeof(struct bootdata);
	int err;

	/*
	 * Try to read bootdata from NVM through PDC.
	 * If successful, set `howto' and `bootdev'.
	 */
	err = (*pdc)(PDC_NVM, PDC_NVM_READ, NVM_BOOTDATA, &bd, bdsize);
	if (err < 0) {
		/*
		 * First, determine if this machine has Non-Volatile Memory.
		 * If not, just return (until we come up with a new plan)!
		 */
		if (err == -1)		/* Nonexistent procedure */
			return;
		printf("NVM bootdata Read ret'd %d\n", err);
	} else {
		if (bd.cksum == NVM_BOOTCKSUM(bd)) {
			/*
			 * The user may override the PDC auto-boot, setting
			 * an interactive boot.  We give them this right by
			 * or'ing the bootaddr flags into `howto'.
			 */
			howto |= bd.flags;
			bootdev = bd.device;
		} else {
			printf("NVM bootdata Bad Checksum (%x)\n", bd.cksum);
		}
	}

	/*
	 * Reset the bootdata to defaults (if necessary).
	 */
	if (bd.flags != RB_AUTOBOOT || bd.device != 0) {
		bd.flags = RB_AUTOBOOT;
		bd.device = 0;
		bd.cksum = NVM_BOOTCKSUM(bd);
		err = (*pdc)(PDC_NVM, PDC_NVM_WRITE, NVM_BOOTDATA, &bd, bdsize);
		if (err < 0)
			printf("NVM bootdata Write ret'd %d\n", err);
	}
}




