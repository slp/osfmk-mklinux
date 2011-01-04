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
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */

#include <mach/std_types.h>
#include <scsi/compat_30.h>
#include <scsi/scsi.h>
#include <scsi/scsi_defs.h>
#include <scsi/rz.h>
#include <scsi/rz_labels.h>
#include <scsi/scsi_entries.h>
#include <types.h>
#include <sys/ioctl.h>
#include <vm/vm_kern.h>
#include <device/conf.h>
#include <device/ds_routines.h>
#include <device/device_typedefs.h>
#include <device/subrs.h>
#include <kern/misc_protos.h>
#include <i386/AT386/hdreg.h>
#include <machine/disk.h>
#include <string.h>

/*
 * PC/AT specific set/get_status operations
 */

int scsi_abs_sec = -1;
int scsi_abs_count = -1;

extern	void		scsi_get_dos_parms(target_info_t	*tgt,
					   struct disk_parms 	*dp,
					   struct disklabel 	*lp);


io_return_t
scsi_i386_get_status(
	dev_t		dev,
	target_info_t	*tgt,
	dev_flavor_t	flavor,
	dev_status_t	status,
	mach_msg_type_number_t	*status_count)
{
	switch (flavor) {
	case V_GETPARMS: {
		struct disklabel *lp = &tgt->dev_info.disk.l;
		struct disk_parms *dp = (struct disk_parms *)status;
		int part = rzpartition(dev);

		if (*status_count < sizeof (struct disk_parms)/sizeof(int))
			return (D_INVALID_SIZE);
		dp->dp_type = DPT_WINI; 
		dp->dp_secsiz = lp->d_secsize;
		dp->dp_sectors = lp->d_nsectors;
		dp->dp_heads = lp->d_ntracks;
		dp->dp_cyls = lp->d_ncylinders;
	
		scsi_get_dos_parms(tgt, dp, lp);
		dp->dp_ptag = 0;
		dp->dp_pflag = 0;
		dp->dp_pstartsec = lp->d_partitions[part].p_offset;
		dp->dp_pnumsec = lp->d_partitions[part].p_size;
		*status_count = sizeof(struct disk_parms)/sizeof(int);
		break;
	}
	case V_RDABS: {
		io_return_t ret;

		if (*status_count < tgt->block_size/sizeof (int)) {
			printf("RDABS bad size %x", *status_count);
			return (D_INVALID_SIZE);
		}
		ret = scsi_rw_abs(dev, status, IO_READ, 
				  scsi_abs_sec, tgt->block_size);
		if (ret != D_SUCCESS)
			return ret;
		*status_count = tgt->block_size/sizeof(int);
		break;
	}
	case V_VERIFY: {
		int count = scsi_abs_count * tgt->block_size;
		int sec = scsi_abs_sec;
                int bsize = PAGE_SIZE;
		char *scsi_verify_buf;

                if (tgt->block_size > PAGE_SIZE)
                        bsize = tgt->block_size;
		(void) kmem_alloc(kernel_map,
				  (vm_offset_t *)&scsi_verify_buf,
				  bsize);

		*status = 0;
		while (count > 0) {
			int xcount = (count < bsize) ? count : bsize;
			if (scsi_rw_abs(dev, (dev_status_t)scsi_verify_buf,
					IO_READ, sec, xcount) != D_SUCCESS) {
				*status = BAD_BLK;
				break;
			} else {
				count -= xcount;
				sec += xcount / tgt->block_size;
			}
	        }
		(void) kmem_free(kernel_map, (vm_offset_t)scsi_verify_buf, bsize);
		*status_count = 1;
		break;
	}
	default:
		return(D_INVALID_OPERATION);
	}
	return D_SUCCESS;
}

io_return_t
scsi_i386_set_status(
	dev_t			dev,
	target_info_t		*tgt,
	dev_flavor_t		flavor,
	dev_status_t		status,
	mach_msg_type_number_t	status_count)
{
	switch (flavor) {
	case V_SETPARMS:
		printf("scsdisk_set_status: invalid flavor V_SETPARMS\n");
		return(D_INVALID_OPERATION);
		break;
	case V_REMOUNT:
		tgt->flags &= ~TGT_ONLINE;
		break;
	case V_ABS:
		scsi_abs_sec = status[0];
		if (status_count == 2)
			scsi_abs_count = status[1];
		break;
	case V_WRABS: {
		io_return_t ret;

		if (status_count < tgt->block_size/sizeof (int)) {
			printf("RDABS bad size %x", status_count);
			return (D_INVALID_SIZE);
		}
		ret = scsi_rw_abs(dev, status, IO_WRITE, 
				  scsi_abs_sec, tgt->block_size);
		if (ret != D_SUCCESS)
			return ret;
		break;
	}
	default:
		return(D_INVALID_OPERATION);
	}
	return D_SUCCESS;
}

void
scsi_get_dos_parms(
	target_info_t	*tgt,
	struct disk_parms *dp,
	struct disklabel *lp)
{
	dev_ops_t	ops;
	int		unit;
	char 		devname[16];
	register 	dos_unit;

	strcpy(devname, "disk");

	for (dos_unit = 0; dos_unit < 2; dos_unit++) {
		itoa(dos_unit, devname + 4);

		if (dev_name_lookup(devname, &ops, &unit) &&
		    ops->d_open == rz_open &&
		    tgt->unit_no * ops->d_subdev == unit) {
			(void) get_dos_parms(dos_unit,
				     &dp->dp_doscyls,
				     &dp->dp_dosheads,
				     &dp->dp_dossectors);
			return;
		}
	}

	dp->dp_dossectors = 32;
	dp->dp_dosheads = 64;
	dp->dp_doscyls = lp->d_secperunit / 64 / 32;
}
