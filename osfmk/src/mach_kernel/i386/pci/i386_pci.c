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
 * Taken from
 *
 *  Copyright (c) 1994	Wolfgang Stanglmeier, Koeln, Germany
 *			<wolf@dentaro.GUN.de>
*/

#include <pci.h>
#if NPCI > 0

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <kern/misc_protos.h>
#include <vm/vm_kern.h>
#include <busses/pci/pci.h>
#include <i386/pci/pci.h>
#include <i386/pci/pci_device.h>
#include <i386/pci/pcibios.h>
#include <i386/io_map_entries.h>

#define PCI_DEBUG 0

/*
 *      per device (interrupt) data structure.
 */

/*========================================================
 *
 *	Autoconfiguration of pci devices.
 *
 *	This is reverse to the isa configuration.
 *	(1) find a pci device.
 *	(2) look for a driver.
 *
 *========================================================
*/

/*--------------------------------------------------------
 *
 *	The pci devices can be mapped to any address.
 *	As default we start at the last gigabyte.
 *
 *--------------------------------------------------------
*/

#ifndef PCI_PMEM_START
#define PCI_PMEM_START 0xFEF00000
#endif

static vm_offset_t pci_paddr = PCI_PMEM_START;

/*---------------------------------------------------------
 *
 *	pci_configure ()
 *
 *---------------------------------------------------------
*/

void pci_configure()
{
	unsigned char	device;
	unsigned short	bus;
	pcici_t		tag;
	unsigned long	type;
	unsigned long	data;
	int		unit;
	int		pci_mode;
	pci_vendor_id_t	vendor_id;
	pci_dev_id_t	device_id;
	pci_class_t	class;
	pci_subclass_t	subclass;

	struct pci_driver *drp;
	struct pci_device *dvp;

	/*
	 *	check pci bus present
	*/

	pci_mode = pci_conf_mode();
	if (!pci_mode) return;

	/*
	 *	hello world ..
	*/

#if	PCI_DEBUG
	printf("PCI configuration mode %d.\n", pci_mode);
	printf("Scanning device %d..%d on pci bus %d..%d\n",
		PCI_FIRST_DEVICE,PCI_LAST_DEVICE,
	       PCI_FIRST_BUS, PCI_LAST_BUS);
#endif	/* PCI_DEBUG */
	for (bus=PCI_FIRST_BUS;bus<=PCI_LAST_BUS; bus++)
	    for (device=PCI_FIRST_DEVICE; device<=PCI_LAST_DEVICE; device ++) {
		tag = pcitag(bus, device, 0);
		type = pci_conf_read(tag, 0);

		vendor_id = type & 0xffff;
		device_id = type>>16;

		if (vendor_id == PCI_VID_INVALID)
			 continue;

		/*
		 *	lookup device in ioconfiguration:
		*/

		for (dvp = pci_devtab; drp=dvp->pd_driver; dvp++) {
			if (drp->vendor_id == vendor_id &&
			    drp->device_id == device_id)
				break;
		};

		printf("PCI%d:%d ", bus, device);

		if (!drp) {
			unsigned char	reg;
			unsigned int 	class_code;
			/*
			 *	not found
			*/

			class_code = pci_conf_read(tag, 8) ;
			class = class_code>>24;
			subclass = (class_code>>16)&0xff;

			pci_print_id(vendor_id, device_id, class, subclass);

			switch (class) {
			case BASE_BC:
			case BASE_NETWORK:
			case BASE_MEM:
			case BASE_BRIDGE:
				break;
			case BASE_DISPLAY:
				if (subclass == SUB_PRE_VGA)
					break;
			default:
				printf("[unsupported]");
				break;
			}

#if	PCI_DEBUG
			for (reg=0x10; reg<=0x20; reg+=4) {
				data = pci_conf_read(tag, reg);
				if (!data) continue;
				switch (data&7) {

				case 1:
				case 5:
					printf("	map(%x): io(%x)\n",
						reg, data & ~3);
					break;
				case 0:
					printf("	map(%x): mem32(%x)\n",
						reg, data & ~7);
					break;
				case 2:
					printf("	map(%x): mem20(%x)\n",
						reg, data & ~7);
					break;
				case 4:
					printf("	map(%x): mem64(%x)\n",
						reg, data & ~7);
					break;
				};
			};
#else	/* PCI_DEBUG */
			printf("\n");
#endif	/* PCI_DEBUG */
			continue;
		};

		/*
		 *	found it.
		 *	probe returns the device unit.
		*/

		printf("<%s>", drp -> vendor);

		unit = (*drp->probe)(tag);

		if (unit<0) {
			printf(" probe failed.\n");
			continue;
		};

		/*
		 *	enable memory access
		*/
		data = pci_conf_read(tag, 0x04) & 0xffff | 0x0002;
		pci_conf_write(tag, (unsigned char) 0x04, data);

		/*
		 *	attach device
		 *	may produce additional log messages,
		 *	i.e. when installing subdevices.
		*/

		printf(" as %s%d\n", drp->name,unit);
		(void)(*drp->attach)(tag);

	};

	if (pci_paddr > PCI_PMEM_START)
		printf("pci uses physical addresses from %x to %x\n",
		       PCI_PMEM_START, pci_paddr);
}

/*-----------------------------------------------------------------------
 *
 *	Map device into virtual and physical space
 *
 *-----------------------------------------------------------------------
*/

int pci_map_mem(pcici_t tag, unsigned long reg, vm_offset_t* va, vm_offset_t* pa)
{
	unsigned long data, result;
	vm_size_t vsize;
	vm_offset_t vaddr;
	vm_map_entry_t entry;

	/*
	 *	sanity check
	 */

	if (reg < 0x10 || reg >= 0x20 || (reg & 3))
		return (KERN_INVALID_ARGUMENT);

	/*
	 *	get size and type of memory
	 */

	pci_conf_write(tag, reg, 0xfffffffful);
	data = pci_conf_read(tag, reg);

	switch (data & 0x0f) {

	case 0x0:	/* 32 bit non cachable */
		break;

	default:	/* unknown */
		return (KERN_FAILURE);
	};

	vsize = round_page(-(data & 0xfffffff0));

	printf("0x%x bytes", vsize);

	if (!vsize) return (KERN_FAILURE);

	/*
	 *	align physical address to virtual size
	 */

	if (data = pci_paddr % vsize)
		pci_paddr += vsize - data;

	/*
	 *	Map device to virtual space
	 */


	vaddr = io_map(pci_paddr, vsize);

	/*
	 *	display values.
	 */

	printf(" at 0x%x (0x%x)", pci_paddr, vaddr);

	*va = vaddr;
	*pa = pci_paddr;

	/*
	 *	set device address
	 */

	pci_conf_write(tag, reg, pci_paddr);

	return (0);
}
#endif /* NPCI > 0 */
