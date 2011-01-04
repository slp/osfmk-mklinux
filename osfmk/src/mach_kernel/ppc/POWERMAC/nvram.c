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

#include <nvram.h>
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
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/powermac_gestalt.h>
#include <ppc/POWERMAC/powermac_pci.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/pram.h>


#define	NVRAM_SIZE		8192
#define	NVRAM_PRAM_OFFSET	(4096+768)
#define	NVRAM_PRAM_SIZE		(256)


enum {
	NVRAM_IOMEM,
	NVRAM_PORT,
	NVRAM_NONE
} nvram_kind = NVRAM_NONE;

long nvram_pram_read(long offset, char *buffer, long size);
long nvram_pram_write(long offset, char *buffer, long size);

struct pram_ops nvram_pram_ops = {
	nvram_pram_read,
	nvram_pram_write
};


static v_u_char *nvram_data = NULL;
static v_u_char *nvram_port = NULL;

void	nvramattach(struct bus_device *device);
int	nvramprobe(caddr_t addr, void *ui);

boolean_t		nvram_initted = FALSE;
struct bus_device	*nvram_info[NNVRAM];
struct bus_driver	nvram_driver = { nvramprobe, 0, nvramattach, 0, NULL, "nvram", nvram_info, 0, 0, 0 };

decl_simple_lock_data(,nvram_lock)

int
nvramprobe(caddr_t addr, void *ui)
{
	device_node_t *nvramdev;

	simple_lock_init(&nvram_lock, ETAP_IO_CHIP);

	nvramdev = find_devices("nvram");

	if (nvramdev == NULL) 
		return 0;

	switch (nvramdev->n_addrs) {
	case	0:
		/* Hmmm.. this is probably running on a Ohare/Laptop 
		 * nvram is not a real device then..
		 */
		return 0;

	case	1:
		nvram_kind = NVRAM_IOMEM;
		nvram_data = (v_u_char *) POWERMAC_IO(nvramdev->addrs[0].address);
		break;

	case	2:
		nvram_kind = NVRAM_PORT;
		nvram_port = (v_u_char *) POWERMAC_IO(nvramdev->addrs[0].address);
		nvram_data = (v_u_char *) POWERMAC_IO(nvramdev->addrs[1].address);
		break;

	default:
		printf("WARNING: Unknown NVRAM - %d addresses. Disabling nvram driver\n", nvramdev->n_addrs);
		return 0;

	}

	return 1;
}

void
nvramattach(struct bus_device *device)
{
	printf(" attached");
}

static long
nvram_write(long offset, char *buffer, long size)
{
	long	wrote = 0;

	if (offset+size > NVRAM_SIZE)
		size = NVRAM_SIZE-size;

	simple_lock(&nvram_lock);

	offset &= 0x1fff;	/* To be safe */

	switch (nvram_kind) {
	case	NVRAM_IOMEM:
		for (wrote = 0; wrote < size; wrote++, offset++)  {
			nvram_data[offset << 4] = *buffer++; eieio();
		}
		break;

	case	NVRAM_PORT:
		for (wrote = 0; wrote < size; wrote++, offset++) {
			*nvram_port = offset >> 5; eieio();
			nvram_data[(offset & 0x1f) << 4] = *buffer++;
			eieio();
		}
		break;
	}

	simple_unlock(&nvram_lock);

	return wrote;
}

static long
nvram_read(long offset, char *buffer, long size)
{
	long	read = 0;

	offset &= 0x1fff;	/* To be safe */

	if (offset+size > NVRAM_SIZE)
		size = NVRAM_SIZE-size;

	simple_lock(&nvram_lock);

	switch (nvram_kind) {
	case	NVRAM_IOMEM:
		for (read = 0; read < size; read++, offset++)  {
			*buffer++ = nvram_data[offset << 4];
			eieio(); /* better safe than sorry! */
		}
		break;

	case	NVRAM_PORT:
		for (read = 0; read < size; read++, offset++) {
			*nvram_port = offset >> 5; eieio();
			*buffer++ = nvram_data[(offset & 0x1f) << 4];
			eieio();
		}
		break;
	}

	simple_unlock(&nvram_lock);
	return read;
}


long
nvram_pram_read(long offset, char *buffer, long size)
{
	if (size+offset > NVRAM_PRAM_SIZE)
		size = NVRAM_PRAM_SIZE - offset;

	return nvram_read(offset+NVRAM_PRAM_OFFSET, buffer, size);
}


long
nvram_pram_write(long offset, char *buffer, long size)
{
	if (size+offset > NVRAM_PRAM_SIZE)
		size = NVRAM_PRAM_SIZE - offset;

	return nvram_write(offset+NVRAM_PRAM_OFFSET, buffer, size);
}

