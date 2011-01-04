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
/* CMU_HIST */
/*
 * Revision 2.2.4.2  92/04/30  11:55:16  bernadat
 * 	Used #defined name for slave field in dev number.
 * 	Removed tgt->unit_no from io_unit initialization in grab_bob_label()
 * 	since it is not properly initialized at that time (= slave).
 * 	Print warning about capacity only if probed size > theory
 * 	Changes from TRUNK
 * 	[92/04/28            bernadat]
 * 
 * 	Remove buffer allocation on stack in scsi_i386_get_status()
 * 	since it is to big.
 * 	[92/04/21            bernadat]
 * 
 * Revision 2.2.4.1  92/03/28  10:06:14  jeffreyh
 * 	Add AT specific setstatus/getstatus flavors
 * 	to support vtoc, diskutil, verify ...
 * 	[92/03/04            bernadat]
 * 	Changes from TRUNK
 * 	[92/03/10  13:21:51  jeffreyh]
 * 
 * Revision 2.4  92/02/23  22:43:04  elf
 * 	Dropped first scsi_softc argument.
 * 	[92/02/22  19:58:21  af]
 * 
 * Revision 2.3  92/02/19  16:29:46  elf
 * 	On 25-Jan, did not consider NO ACTIVE mach parition.
 * 	[92/01/31            rvb]
 * 
 * 	Add "BIOS" support -- always boot mach partition NOT active one.
 * 	[92/01/25            rvb]
 * 
 * Revision 2.2  91/08/24  11:57:41  af
 * 	Temporarily created, till we evaporate the religious issues.
 * 	[91/08/02            af]
 * 
 */
/* CMU_ENDHIST */
/*
 */
/* This goes away as soon as we move it in the Ux server */

#include <mach/std_types.h>
#include <scsi/compat_30.h>
#include <scsi/scsi.h>
#include <scsi/scsi_defs.h>
#include <scsi/rz.h>
#include <scsi/rz_labels.h>
#include <scsi/scsi_entries.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <vm/vm_kern.h>
#include <device/ds_routines.h>
#include <kern/misc_protos.h>

#define partition xpartition /* partition already defined in disk_status.h */
#include <machine/disk.h>
#undef partition

#include <i386/AT386/hdreg.h>

/* Forward */

extern io_return_t	scsi_rw_abs(
				dev_t				dev,
				dev_status_t			data,
				int				rw,
				int				sec,
				int				count);

int
grab_bob_label(
	target_info_t				*tgt,
	struct disklabel			*label,
	io_req_t				ior,
	struct bios_partition_info		*part)
{
	register int		i, n;
	struct evtoc		*evp;

	{
		int nmach = 0;
		int	mabr = 0,
			mbr = 0;

		for (i = 0; i < 4; i++) {
			if (part[i].systid == UNIXOS) {
				if (!nmach++)
					mbr = i;
				if (part[i].bootid == BIOS_BOOTABLE)
					mabr = i;
			}
		}
		if (mabr)
			i = mabr;
		else if (nmach == 1)
			i = mbr;
		else if (nmach == 0)
			return 0;	/* DOS, no Mach */
		else {
			printf("Warning: More than one Mach partition and none active.\n");
			printf("Using DOS partition #%d\n", mbr);
			i = mbr;
		}
	}
	/*
	 * In rz_labels:rz_bios_label(), we have set up DOS partitions 0-3 to correspond
	 * to the parittions 0-3 in the label.  Hence we use the "i" above, in
	 * the line below.
	 */
	ior->io_unit = (tgt->masterno<<RZCONTROLLER_SHIFT)
	  + (tgt->target_id<<RZSLAVE_SHIFT) + (i);
	ior->io_count = DEV_BSIZE;
	ior->io_error = 0;
	ior->io_op = IO_READ;
	ior->io_recnum = 29;	/* that's where the vtoc is */
	scdisk_strategy(ior);
	iowait(ior);
	evp = (struct evtoc *)ior->io_data;

	if (evp->sanity != VTOC_SANE)
		return 0;

	bcopy(evp->label, label->d_packname, 16);/* truncates, too bad */
	label->d_ncylinders = evp->cyls;
	label->d_ntracks = evp->tracks;
	label->d_nsectors = evp->sectors;
	label->d_secpercyl = evp->tracks * evp->sectors;
#ifdef	you_care
	if (label->d_secperunit > evp->tracks * evp->sectors * evp->cyls)
		printf("sd%d: capacity (%d) != S*H*C (%d)\n", tgt->target_id,
			label->d_secperunit, evp->tracks * evp->sectors * evp->cyls);
#endif
	label->d_secsize = DEV_BSIZE;

	/* copy info on first MAXPARTITIONS */
	n = evp->nparts;
	if (n > MAXPARTITIONS) n = MAXPARTITIONS;
		/* whole disk */
	label->d_partitions[MAXPARTITIONS].p_size = -1;
	label->d_partitions[MAXPARTITIONS].p_offset = 0;
		/* "c" is never read; always calculated */
	label->d_partitions[2].p_size = label->d_partitions[i].p_size;
	label->d_partitions[2].p_offset = label->d_partitions[i].p_offset;
	for (i = 0; i < n; i++) {
		if (i == 2)
			continue;
		label->d_partitions[i].p_size = evp->part[i].p_size;
		label->d_partitions[i].p_offset = evp->part[i].p_start;
	}
	for ( ; i < MAXPARTITIONS; i++) {
		if (i == 2)
			continue;
		label->d_partitions[i].p_size = 0;
		label->d_partitions[i].p_offset = 0;
	}
	return 1;
}

int scsi_abs_sec = -1;
int scsi_abs_count = -1;

io_return_t
scsi_rw_abs(
	dev_t		dev,
	dev_status_t	data,
	int		rw,
	int		sec,
	int		count)
{
	io_req_t	ior;
	io_return_t	error;

	io_req_alloc(ior,0);
	ior->io_next = 0;
	ior->io_unit = dev & (~(MAXPARTITIONS-1));	/* sort of */
	ior->io_unit |= PARTITION_ABSOLUTE;
	ior->io_data = (io_buf_ptr_t)data;
	ior->io_count = count;
	ior->io_recnum = sec;
	ior->io_error = 0;
	if (rw == IO_READ)
		ior->io_op = IO_READ;
	else
		ior->io_op = IO_WRITE;
	scdisk_strategy(ior);
	iowait(ior);
	error = ior->io_error;
	io_req_free(ior);
	return(error);
}

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
	
		dp->dp_dossectors = 32;
		dp->dp_dosheads = 64;
		dp->dp_doscyls = lp->d_secperunit / 64 / 32;
		dp->dp_ptag = 0;
		dp->dp_pflag = 0;
		dp->dp_pstartsec = lp->d_partitions[part].p_offset;
		dp->dp_pnumsec = lp->d_partitions[part].p_size;
		*status_count = sizeof(struct disk_parms)/sizeof(int);
		break;
	}
	case V_RDABS: {
		io_return_t ret;

		if (*status_count < DEV_BSIZE/sizeof (int)) {
			printf("RDABS bad size %x", *status_count);
			return (D_INVALID_SIZE);
		}
		ret = scsi_rw_abs(dev, status, IO_READ,
				  scsi_abs_sec, DEV_BSIZE);
		if (ret != D_SUCCESS)
			return ret;
		*status_count = DEV_BSIZE/sizeof(int);
		break;
	}
	case V_VERIFY: {
		int count = scsi_abs_count * DEV_BSIZE;
		int sec = scsi_abs_sec;
		char *scsi_verify_buf;

		(void) kmem_alloc(kernel_map, (vm_offset_t *)&scsi_verify_buf,
			PAGE_SIZE);

		*status = 0;
		while (count > 0) {
			int xcount = (count < PAGE_SIZE) ? count : PAGE_SIZE;
			if (scsi_rw_abs(dev, (dev_status_t)scsi_verify_buf,
					IO_READ, sec, xcount) != D_SUCCESS) {
				*status = BAD_BLK;
				break;
			} else {
				count -= xcount;
				sec += xcount / DEV_BSIZE;
			}
	        }
		(void) kmem_free(kernel_map, (vm_offset_t)scsi_verify_buf,
			PAGE_SIZE);
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

		if (status_count < DEV_BSIZE/sizeof (int)) {
			printf("RDABS bad size %x", status_count);
			return (D_INVALID_SIZE);
		}
		ret = scsi_rw_abs(dev, status, IO_WRITE,
				  scsi_abs_sec, DEV_BSIZE);
		if (ret != D_SUCCESS)
			return ret;
		break;
	}
	default:
		return(D_INVALID_OPERATION);
	}
	return D_SUCCESS;
}

