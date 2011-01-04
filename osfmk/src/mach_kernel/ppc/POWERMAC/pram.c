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

#include <pram.h>
#include <platforms.h>

#include <mach_kdb.h>
#include <kern/spl.h>
#include <machine/machparam.h>          /* spl definitions */
#include <types.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>
#include <ppc/proc_reg.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/adb.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/powermac_gestalt.h>
#include <ppc/POWERMAC/powermac_pci.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/pram.h>


long	pram_notinit(long, char*, long);
int	pramprobe(caddr_t, void *);
void	pramattach( struct bus_device *device);

struct pram_ops none_pram_ops = {
	pram_notinit,
	pram_notinit
};

struct pram_ops *pram =  &none_pram_ops;
struct bus_device	*pram_info[NPRAM];
struct bus_driver	pram_driver =
	{ pramprobe, 0, pramattach, 0, NULL, "pram", pram_info, 0, 0, 0 };

char	*pram_device = "none";

extern struct pram_ops	adb_pram_ops, nvram_pram_ops;

int
pramprobe(caddr_t addr, void *ui)                                 

{
	if (find_devices("via-pmu")) {
		pram = &adb_pram_ops;
		pram_device = "pmu controller";
	} else if (find_devices("nvram")) {
		pram = &nvram_pram_ops;
		pram_device = "nvram";
	} else {
		pram = &adb_pram_ops;
		pram_device = "adb controller";
	}

	return 1;
}

void
pramattach( struct bus_device *device)
{
	printf(" attached to %s", pram_device);
}

long
pram_notinit(long offset, char *buffer, long size)
{
	printf("PRAM routine called and not initted!\n");
	return 0;
}
