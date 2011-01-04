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
 * Taken from
 *
 *  Copyright (c) 1994	Wolfgang Stanglmeier, Koeln, Germany
 *			<wolf@dentaro.GUN.de>
 */

#ifndef __PCI_DEVICE_H__
#define __PCI_DEVICE_H__

/*------------------------------------------------------------
 *
 *  Per driver structure.
 *
 *------------------------------------------------------------
*/

struct pci_driver {
    int     		(*probe )(pcici_t pci_ident);   /* test whether device
							   is present */
    int     		(*attach)(pcici_t pci_ident);   /* setup driver for a
							   device */
    pci_vendor_id_t 	vendor_id;			/* vendor pci id */
    pci_dev_id_t 	device_id;			/* device pci id */
    char    		*name;			    	/* device name */
    char    		*vendor;			/* device long name */
    void     		(*intr)(int);                   /* interupt handler */
};

/*-----------------------------------------------------------
 *
 *  Per device structure.
 *
 *  It is initialized by the config utility and should live in
 *  "ioconf.c". At the moment there is only one field.
 *
 *  This is a first attempt to include the pci bus to 386bsd.
 *  So this structure may grow ..
 *
 *-----------------------------------------------------------
*/

struct pci_device {
	struct pci_driver * pd_driver;
};

/*-----------------------------------------------------------
 *
 *  This table should be generated in file "ioconf.c"
 *  by the config program.
 *  It is used at boot time by the configuration function
 *  pci_configure()
 *
 *-----------------------------------------------------------
*/

extern struct pci_device pci_devtab[];

/*-----------------------------------------------------------
 *
 *  This functions may be used by drivers to map devices
 *  to virtual and physical addresses. The va and pa
 *  addresses are "in/out" parameters. If they are 0
 *  on entry, the mapping function assigns an address.
 *
 *-----------------------------------------------------------
*/

#include <mach/mach_types.h>

int pci_map_mem(pcici_t tag,
		unsigned long entry,
		vm_offset_t *va,
		vm_offset_t *pa);
#endif /*__PCI_DEVICE_H__*/
