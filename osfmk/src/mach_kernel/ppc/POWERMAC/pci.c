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

#include <platforms.h>
#include <ppc/proc_reg.h> /* For isync */
#include <mach_debug.h>
#include <kern/assert.h>
#include <kern/cpu_number.h>
#include <kern/spl.h>
#include <mach/std_types.h>
#include <types.h>
#include <chips/busses.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/powermac_pci.h>
#include <ppc/io_map_entries.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/io.h>
#include <ppc/POWERMAC/pci.h>
#include <ppc/POWERMAC/pcireg.h>
#include <ppc/POWERMAC/pcivar.h>


#define	PCI_BANDIT		(11)
#define	APPLE_BANDIT_ID		0x106b

#define	PCI_REG_VENDOR_ID	0x00
#define	PCI_REG_DEVICE_ID	0x02
#define	PCI_REG_COMMAND		0x04
#define	PCI_REG_STATUS		0x06
#define	PCI_REG_REVISION_ID	0x08
#define	PCI_REG_CLASS_CODE	0x9
#define	PCI_REG_CACHE_LINE	0xC
#define	PCI_REG_LATENCY_TIMER	0xD
#define	PCI_REG_HEADER_TYPE	0xE
#define	PCI_REG_BIST		0xF
#define	PCI_REG_BANDIT_CFG	0x40
#define	PCI_REG_ADDR_MASK	0x48
#define	PCI_REG_MODE_SELECT	0x50
#define	PCI_REG_ARBUS_HOLDOFF	0x58


#define	PCI_STAT_IOSPACE	0x001	/* I/O space enabled */
#define	PCI_STAT_MEMSPACE	0x002	/* Memory space enabled */
#define	PCI_STAT_BUS_MASTER	0x004	/* Bus Master */
#define	PCI_STAT_SPECIAL_CYCLES 0x008	/* Respond to Special Cycles */
#define PCI_STAT_CACHE_ENABLED	0x010	/* Bandit is I/O coherent */
#define	PCI_STAT_VGA_SNOOP	0x020	/* .. not needed by Bandit */
#define	PCI_STAT_PARITY_ERR	0x040	/* Parity Error Response (not implemented) */
#define	PCI_STAT_WAIT_CYCLE	0x080	/* Not implemented*/
#define	PCI_STAT_SERR_ENABLE	0x100	/* System Error Pin implemented */
#define	PCI_STAT_FAST_BACK	0x200	/* Fast back to back cycles (not implemented) */

#define	PCI_MS_BYTESWAP		0x001	/* Enable Big Endian mode. (R/W)*/
#define	PCI_MS_PASSATOMIC	0x002	/* PCI Bus to ARBus Lock are always allowed (R)*/
#define	PCI_MS_NUMBER_MASK	0x00C	/* PCI Bus Number (R) */
#define	PCI_MS_IS_SYNC		0x010	/* Is Synchronous (1) or Async (0) ? (R)*/
#define	PCI_MS_VGA_SPACE	0x020	/* Map VGA I/O space  (R/W) */
#define	PCI_MS_IO_COHERENT	0x040	/* I/O Coherent (R/W) */
#define	PCI_MS_INT_ENABLE	0x080	/* Allow TEA or PCI Abort INT to pass to Grand Central (R/W) */

#define	BANDIT_SPECIAL_CYCLE	0xe00000	/* Special Cycle offset */

struct pci_bridge {
	int			bus_start;
	int			bus_end;
	unsigned long		config_addr;
	unsigned long		config_data;
	unsigned long		special_cycle;
	unsigned long		status;
	unsigned long		vendor_id; 
	unsigned long		port_base;
	device_node_t		*device;
	struct pci_bridge	*next;
};

typedef struct pci_bridge pci_bridge_t;

static char *pci_bridge_names[] = {
	"bandit",
	"chaos",
	NULL
};

int	pci_bridge_count;
pci_bridge_t	**pci_bridges, *pci_bridge_list = NULL;

static void scan_bridge(char *, pci_bridge_t **, unsigned int *);

void
powermac_scan_bridges(unsigned int *first_avail)
{
	pci_bridge_t	*bridge = NULL;
	int	i;

	for (i = 0; pci_bridge_names[i]; i++)
		scan_bridge(pci_bridge_names[i], &bridge, first_avail);

	if (pci_bridge_list == NULL)
		return;

	pci_bridge_count++;

	pci_bridges = (pci_bridge_t **) *first_avail;
	*first_avail += sizeof(pci_bridge_t **) * pci_bridge_count;

	for  (i = 0; i < pci_bridge_count; i++)
		pci_bridges[i] = 0;

	for (bridge = pci_bridge_list; bridge; bridge = bridge->next)
		for (i = bridge->bus_start; i <= bridge->bus_end; i++)
			pci_bridges[i] = bridge;
}

static void
scan_bridge(char *name, pci_bridge_t **prev, unsigned int *first_avail)
{
	device_node_t	*bridge_dev;
	pci_bridge_t	*br;
	unsigned long		*range, length;
	
	bridge_dev = find_devices(name);

	while (bridge_dev) {
		range  = (unsigned long *) get_property(bridge_dev, "bus-range", &length);
		if (range == NULL)
			continue;

		br = (pci_bridge_t *) *first_avail;
		*first_avail += sizeof(*br);
		
		if (*prev) 
			(*prev)->next = br;

		*prev = br;


		if (pci_bridge_list == NULL)
			pci_bridge_list = br;

		br->next = NULL;
		br->bus_start = range[0];
		br->bus_end = range[1];
		br->device = bridge_dev;
		br->port_base = bridge_dev->addrs[0].address;
		br->config_addr = bridge_dev->addrs[0].address + 0x800000;
		br->config_data = bridge_dev->addrs[0].address + 0xc00000;
		br->special_cycle = bridge_dev->addrs[0].address + BANDIT_SPECIAL_CYCLE;
		br->vendor_id = 0xdeadbeef;

		if (br->bus_end > pci_bridge_count)
			pci_bridge_count = br->bus_end;

		bridge_dev = bridge_dev->next;

	}
}

static __inline__ void
pci_set_config_reg(unsigned long addr, unsigned long value)
{
	outl_le(addr, value);
	delay(2);
}


boolean_t
powermac_is_coherent(void)
{
	pci_bridge_t	*bridge;
	unsigned long	cmd, data;
	boolean_t	is_coherent = TRUE;

	if (powermac_info.class != POWERMAC_CLASS_PCI)  
		return FALSE;

	for (bridge = pci_bridge_list ; bridge; bridge = bridge->next) {
		/* Next.. try to see if this is a I/O coherent system */

		pci_set_config_reg(bridge->config_addr,
				(1 << PCI_BANDIT) | PCI_REG_VENDOR_ID);

		data = inl_le(bridge->config_data);

		bridge->vendor_id = data & 0xffff;

		if (bridge->vendor_id != APPLE_BANDIT_ID)  
			continue;
		
		pci_set_config_reg(bridge->config_addr, (1 << PCI_BANDIT) | PCI_REG_MODE_SELECT);
		bridge->status = inl_le(bridge->config_data);

		if ((bridge->status & PCI_MS_IO_COHERENT) == 0) {
			bridge->status |= PCI_MS_IO_COHERENT;
			outl_le(bridge->config_data, bridge->status);
		}
	}

	if (find_devices("ohare"))
		is_coherent = FALSE;

	return is_coherent;
}

void
powermac_display_pci_busses(void)
{
	pci_bridge_t	*bus;
	int	 i;

	for (i = 0; i < pci_bridge_count; i++, bus++) {
		bus = pci_bridges[i];

		if (bus == NULL)
			continue;

		printf("PCI Bridge %s at 0x%x: covers PCI bus %d",
			bus->device->name,
			bus->device->addrs[0].address, bus->bus_start);

		if (bus->bus_start != bus->bus_end)
			printf("to %d", bus->bus_end);

		if (bus->vendor_id != APPLE_BANDIT_ID) 
			printf(" (NON APPLE PCI BRIDGE! id = %x)",
				bus->vendor_id);
		else
			printf(" Status = %x", bus->status);

		printf("\n");
		bus->config_addr = io_map(bus->config_addr, 4096);
		bus->config_data = io_map(bus->config_data, 4096);
		bus->special_cycle = io_map(bus->special_cycle, 4096);
	}
}

unsigned long
pci_io_base(int bus)
{
	if (pci_bridges[bus] == NULL)
		return -1;

	return	pci_bridges[bus]->port_base;
}

device_node_t *
pci_bus_device(int bus)
{
	return pci_bridges[bus]->device;
}

/*
 * Copyright (c) 1997, Stefan Esser <se@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: pci.c,v 1.1.2.3 1998/01/12 10:34:18 stephen Exp $
 *
 */

#include	<ppc/POWERMAC/pcibus.h>

#define	bootverbose	0

#ifdef PCI_COMPAT
/* XXX this is a terrible hack, which keeps the Tekram AMD SCSI driver happy */
#define cfgmech pci_mechanism
int cfgmech;
#else
static int cfgmech;
#endif /* PCI_COMPAT */
static int devmax;

/* enable configuration space accesses and return data port address */

static int
pci_cfgenable(unsigned bus, unsigned slot, unsigned func, int reg, int bytes)
{
	int dataport = 0;
	unsigned long value;
	pci_bridge_t	*busdev;

	if (bus < pci_bridge_count 
	    && (busdev = pci_bridges[bus]) != NULL
	    && slot < devmax
	    && func <= PCI_FUNCMAX
	    && reg <= PCI_REGMAX
	    && bytes != 3
	    && (unsigned) bytes <= 4
	    && (reg & (bytes -1)) == 0) {
		value = (1 << (11+slot)) | (func << 8) | (reg & ~0x03);

		/* Wait until BANDIT response with the right address */
		do {
			outl_le(busdev->config_addr, value);
		} while (inl_le(busdev->config_addr) != value);

		dataport = busdev->config_data + (reg & 0x03);
	}
	return (dataport);
}

/* disable configuration space accesses */

static void __inline__
pci_cfgdisable(int bus)
{
	// outl(pci_bridges[bus]->config_addr, 0);
}

/* read configuration space register */

int
pci_cfgread(pcicfgregs *cfg, int reg, int bytes)
{
	int data = -1;
	int port;

	port = pci_cfgenable(cfg->bus, cfg->slot, cfg->func, reg, bytes);


	if (port != 0) {
		delay(2);

		switch (bytes) {
		case 1:
			data = inb(port);
			break;
		case 2:
			data = inw_le(port);
			break;
		case 4:
			data = inl_le(port);
			break;
		}
		pci_cfgdisable(cfg->bus);
	}
	return (data);
}

/* write configuration space register */

void
pci_cfgwrite(pcicfgregs *cfg, int reg, int data, int bytes)
{
	int port;

	port = pci_cfgenable(cfg->bus, cfg->slot, cfg->func, reg, bytes);
	if (port != 0) {
		delay(2);

		switch (bytes) {
		case 1:
			outb(port, data);
			break;
		case 2:
			outw_le(port, data);
			break;
		case 4:
			outl_le(port, data);
			break;
		}
		pci_cfgdisable(cfg->bus);
	}
}

/* check whether the configuration mechanism has been correct identified */

static int
pci_cfgcheck(int maxdev)
{
	u_char device;

	if (bootverbose) 
		printf("pci_cfgcheck:\tdevice ");

	for (device = 0; device < maxdev; device++) {
		unsigned id, class, header;
		if (bootverbose) 
			printf("%d ", device);

		id = inl_le(pci_cfgenable(0, device, 0, 0, 4));
		if (id == 0 || id == -1)
			continue;

		class = inl_le(pci_cfgenable(0, device, 0, 8, 4)) >> 8;
		if (bootverbose)
			printf("[class=%06x] ", class);
		if (class == 0 || (class & 0xf8f0ff) != 0)
			continue;

		header = inb(pci_cfgenable(0, device, 0, 14, 1));
		if (bootverbose) 
			printf("[hdr=%02x] ", header);
		if ((header & 0x7e) != 0)
			continue;

		if (bootverbose)
			printf("is there (id=%08x)\n", id);

		pci_cfgdisable(0);
		return (1);
	}
	if (bootverbose) 
		printf("-- nothing found\n");

	pci_cfgdisable(0);
	return (0);
}

int
pci_cfgopen(void)
{
	devmax = 32;
#if 0
	cfgmech = 1;

	return (cfgmech);
#endif
	return 1;
}
