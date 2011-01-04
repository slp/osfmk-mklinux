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

#include <scsi.h>

#if	NSCSI > 0
#include <norma_scsi.h>
#include <platforms.h>
#include <kern/spl.h>		/*  spl*() definitions */
#include <mach/std_types.h>
#include <types.h>
#include <scsi/compat_30.h>

#include <device/subrs.h>
#include <device/ds_routines.h>
#include <chips/busses.h>
#include <scsi/scsi.h>
#include <scsi/scsi2.h>
#include <scsi/scsi_defs.h>
#include <scsi2.h>		/* NSCSI2 */
#include <machine/machparam.h>
#include <kern/misc_protos.h>
#include <string.h>
#include <device/conf.h>
#include <scsi/scsi_info_entries.h>
#include <device/scsi_info.h>

extern scsi_softc_t	*scsi_softc[NSCSI];	/* quick access&checking */

#define	SCSI_TARGETS	7	/* Assume Id 7 is for the controller. */

io_return_t
scsi_info_open(dev_t dev, dev_mode_t flag, io_req_t ior)
{
	return	D_SUCCESS;
}

void
scsi_info_close(dev_t dev)
{
	return;
}

io_return_t
scsi_info_read(dev_t dev, io_req_t ior)
{
	return	D_INVALID_OPERATION;
}

io_return_t
scsi_info_write(dev_t dev, io_req_t ior)
{
	return	D_INVALID_OPERATION;
}

boolean_t
scsi_info_portdeath(dev_t dev, ipc_port_t port)
{
	return	FALSE;
}


io_return_t
scsi_info_getstatus(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	mach_msg_type_number_t *status_count)
{
	int	count = 0, i, id;
	scsi_info_t	*device;

	switch (flavor) {
	case	SCSI_INFO_GET_DEVICE_COUNT:
		for (i = 0; i < NSCSI; i++) {
			if (scsi_softc[i] == NULL)
				continue;

			for (id = 0; id <  SCSI_TARGETS; id++) {
				if (scsi_softc[i]->target[id] == NULL)
					continue;
				if (scsi_softc[i]->target[id]->flags & TGT_ALIVE)
					count++;
			}
		}

		*((unsigned int *) data) = count;
		*status_count = sizeof(count) / sizeof(int);
		break;

	case	SCSI_INFO_GET_DEVICE_INFO:
		device = (scsi_info_t *) data;

		for (i = 0; i < NSCSI; i++) {
			if (scsi_softc[i] == NULL)
				continue;

			for (id = 0; id <  SCSI_TARGETS; id++) {
				if (scsi_softc[i]->target[id] == NULL)
					continue;
			
				if ((scsi_softc[i]->target[id]->flags & TGT_ALIVE) == 0)
					continue;

				device->controller = i;
				device->target_id = id;
				device->lun = 0;
				device->flags = 0;
				bcopy((char *) &scsi_softc[i]->target[id]->target_inquiry,
				    (char *) &device->inquiry_data,
					sizeof(device->inquiry_data));
				device++;
				count++;
			}
		}
		*status_count = (sizeof(*device) * count) / sizeof(int);
		break;
	default:
		return	D_INVALID_OPERATION;
	}

	return	D_SUCCESS;
}

io_return_t
scsi_info_setstatus(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	mach_msg_type_number_t status_count)
{
	return	D_INVALID_OPERATION;
}

#endif /* NSCSI */
