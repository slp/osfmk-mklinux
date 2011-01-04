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
 *                             <wolf@dentaro.GUN.de>
*/


#include <i386/pci/pci.h>
#include <i386/pci/pcibios.h>
#include <i386/pio.h>
#include <kern/misc_protos.h>
#include <mach/std_types.h>

static char pci_mode;

/*--------------------------------------------------------------------
 *
 *      Determine configuration mode
 *
 *--------------------------------------------------------------------
*/


#define CONF1_ENABLE       0x80000000ul
#define CONF1_ADDR_PORT    0x0cf8
#define CONF1_DATA_PORT    0x0cfc


#define CONF2_ENABLE_PORT  0x0cf8
#define CONF2_FORWARD_PORT 0x0cfa


int pci_conf_mode (void)
{
	unsigned long result, oldval;

	int *v;

	for(v=(int *)phystokv(0xe0000); v< (int *)phystokv(0xfffff); v += 4)
	    if (*v == 0x5f32335f) {
		    break;
		    printf("BIOS32 Service Directory found at 0x%x\n",v);
	    }


	/*---------------------------------------
	 *      Configuration mode 2 ?
	 *---------------------------------------
	*/

	outb (CONF2_ENABLE_PORT,     0);
	outb (CONF2_FORWARD_PORT, 0);
	if (!inb (CONF2_ENABLE_PORT) && !inb (CONF2_FORWARD_PORT)) {
		pci_mode = 2;
		return (2);
	};

	/*---------------------------------------
	 *      Configuration mode 1 ?
	 *---------------------------------------
	*/

	oldval = inl (CONF1_ADDR_PORT);
	outl (CONF1_ADDR_PORT, CONF1_ENABLE);
	result = inl (CONF1_ADDR_PORT);
	outl (CONF1_ADDR_PORT, oldval);

	if (result == CONF1_ENABLE) {
		pci_mode = 1;
		return (1);
	};

	/*---------------------------------------
	 *      No PCI bus available.
	 *---------------------------------------
	*/
	return (0);
}

/*--------------------------------------------------------------------
 *
 *      Build a pcitag from bus, device and function number
 *
 *--------------------------------------------------------------------
*/


pcici_t pcitag (unsigned char bus, 
		unsigned char device,
		unsigned char func)
{
	pcici_t tag;

	tag.cfg1 = 0;
	if (device >= 32) return tag;
	if (func   >=  8) return tag;

	switch (pci_mode) {

	case 1:
		tag.cfg1 = CONF1_ENABLE
			| (((unsigned long) bus   ) << 16ul)
			| (((unsigned long) device) << 11ul)
			| (((unsigned long) func  ) <<  8ul);
		break;
	case 2:
		if (device >= 16) break;
		tag.cfg2.port    = 0xc000 | (device << 8ul);
		tag.cfg2.enable  = 0xf1 | (func << 1ul);
		tag.cfg2.forward = bus;
		break;
	};
	return tag;
}

/*--------------------------------------------------------------------
 *
 *      Read register from configuration space.
 *
 *--------------------------------------------------------------------
*/


unsigned long pci_conf_read (pcici_t tag, unsigned long reg)
{
	unsigned long addr, data = 0;

	if (!tag.cfg1) return (0xfffffffful);

	switch (pci_mode) {

	case 1:
		addr = tag.cfg1 | reg & 0xfc;
#ifdef PCI_DEBUG
		printf ("pci_conf_read(1): addr=%x ", addr);
#endif
		outl (CONF1_ADDR_PORT, addr);
		data = inl (CONF1_DATA_PORT);
		outl (CONF1_ADDR_PORT, 0   );
		break;

	case 2:
		addr = tag.cfg2.port | reg & 0xfc;
#ifdef PCI_DEBUG
		printf ("pci_conf_read(2): addr=%x ", addr);
#endif
		outb (CONF2_ENABLE_PORT , tag.cfg2.enable );
		outb (CONF2_FORWARD_PORT, tag.cfg2.forward);

		data = inl ((unsigned short) addr);

		outb (CONF2_ENABLE_PORT,  0);
		outb (CONF2_FORWARD_PORT, 0);
		break;
	};

#ifdef PCI_DEBUG
	printf ("data=%x\n", data);
#endif

	return (data);
}

/*--------------------------------------------------------------------
 *
 *      Write register into configuration space.
 *
 *--------------------------------------------------------------------
*/


void pci_conf_write (pcici_t tag, unsigned long reg, unsigned long data)
{
	unsigned long addr;

	if (!tag.cfg1) return;

	switch (pci_mode) {

	case 1:
		addr = tag.cfg1 | reg & 0xfc;
#ifdef PCI_DEBUG
		printf ("pci_conf_write(1): addr=%x data=%x\n",
			addr, data);
#endif
		outl (CONF1_ADDR_PORT, addr);
		outl (CONF1_DATA_PORT, data);
		outl (CONF1_ADDR_PORT,   0 );
		break;

	case 2:
		addr = tag.cfg2.port | reg & 0xfc;
#ifdef PCI_DEBUG
		printf ("pci_conf_write(2): addr=%x data=%x\n",
			addr, data);
#endif
		outb (CONF2_ENABLE_PORT,  tag.cfg2.enable);
		outb (CONF2_FORWARD_PORT, tag.cfg2.forward);

		outl ((unsigned short) addr, data);

		outb (CONF2_ENABLE_PORT,  0);
		outb (CONF2_FORWARD_PORT, 0);
		break;
	};
}
