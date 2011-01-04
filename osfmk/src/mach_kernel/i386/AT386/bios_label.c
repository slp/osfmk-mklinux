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
 * Read BIOS partitions and translate to BSD label format
 * Default is to use BIOS primary and extended partitions:
 *	a,b,c & d  	corresponds to the 4 first primary partitions.
 *	e .. p		correspond to the extended partition. In this case
 *			d corresponds to a partition containing all the
 *			extended ones.
 *			Note that c does not designate the antire disk.
 *
 * Also handles old vtoc layed out disks.
 *	a .. p 		are obtained from vtoc. c designates all disk.
 */

#include <scsi.h>
#include <platforms.h>
#if	AT386
#include <vtoc_compat.h>
#else	/* AT386 */
#if	PPC
#define VTOC_COMPAT 0	/* can't easily do struct alignment (+endianness) */
#else	/* PPC */
#define	VTOC_COMPAT 1
#endif	/* PPC */
#endif	/* AT386 */
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
#include <device/ds_routines.h>
#include <kern/misc_protos.h>
#include <i386/AT386/disk.h>
#include <i386/AT386/hdreg.h>
#include <i386/AT386/misc_protos.h>
#include <machine/endian.h>

/* Forward */

#define	BIOS_SECTOR_SIZE 512

#if	VTOC_COMPAT
static io_return_t
read_vtoc(
	  dev_t			dev,
	  int			base_sector,
	  int			part_i,
	  struct disklabel	*label,
	  struct alt_info	**alt_info,
	  io_return_t		(*rw_abs)(
					  dev_t		dev,
					  dev_status_t	data,
					  int		rw,
					  int		sec,
					  int		count));
#endif	/* VTOC_COMPAT */

/*
 * Read master boot block at sector <base_sector>
 * and decrypt bios partition table.
 */

io_return_t
read_bios_partitions(
	       dev_t		dev,
	       int		base_sector,	/* base sector for partition */
	       int		ext_base,	/* base for extention */
	       int		*part_i,	/* partition index */
	       struct disklabel	*label,
	       struct alt_info	**alt_info,
	       io_return_t	(*rw_abs)(
					  dev_t		dev,
					  dev_status_t	data,
					  int		rw,
					  int		sec,
					  int		count))
{
	struct mboot 		*mboot;
	io_return_t		rc;
  	struct ipart 		*part;
	register int 		i;
	struct label_partition	*l_part;

	/*
	 * Allocate space for master boot block
	 */

        if (kmem_alloc(kernel_map,
		       (vm_offset_t *)&mboot,
		       sizeof(*mboot)) != KERN_SUCCESS)
		return(D_NO_MEMORY);

	/*
	 * read it
	 */
  	rc = (*rw_abs)(dev,
		       (dev_status_t)mboot,
		       IO_READ, 
		       base_sector, 
		       sizeof(*mboot));

	if (rc != D_SUCCESS)
		goto out;

	/* Check signature in an endianness independent manner */
#ifdef	ltohs
	mboot->signature = ltohs(mboot->signature);
#endif	/* ltohs */
	if (mboot->signature != BOOT_MAGIC) {
		rc = D_IO_ERROR;
		goto out;
	}

	part = (struct ipart *)mboot->parts;

	for (i=0; i < FD_NUMPART && *part_i < MAXPARTITIONS; i++, part++) {

#ifdef ltohl
		/* if we've got a macro to do so, convert short & long
		 * entries into their native host format
		 */
		part->relsect = ltohl(part->relsect);
		part->numsect = ltohl(part->numsect);
#else
#if	BYTE_ORDER == BIG_ENDIAN
#error	ltohl not defined, cannot translate from little endian format
#endif	/* BYTE_ORDER == LITTLE_ENDIAN */
#endif	/* ltohl */

		if (part->systid == 0) {
			if (*part_i < FD_NUMPART)
				(*part_i)++;
			continue;
		}
		if (part->systid != EXT_PART || !ext_base) {
		  	l_part = &label->d_partitions[*part_i];
			l_part->p_size = part->numsect;
			l_part->p_offset = base_sector+part->relsect;
#if	VTOC_COMPAT
			/*
			 * Handle old vtoc layed out disks
			 */
			if (part->systid == UNIXOS && part->bootid == ACTIVE) {
#if	BYTE_ORDER == BIG_ENDIAN
				printf("bios_label.c: Warning: VTOC "
				       "partitioning not ported to big-endian "
				       "machines yet.\n");
				rc = D_IO_ERROR;
				goto out;
#endif	/* BYTE_ORDER == BIG_ENDIAN */
				if (read_vtoc(dev,
					      base_sector+part->relsect,
					      *part_i,
					      label,
					      alt_info,
					      rw_abs) == D_SUCCESS)
					return(D_SUCCESS);
			}
#endif	/* VTOC_COMPAT */
			(*part_i)++;
		}
		if (part->systid == EXT_PART) {
			int new_base;
			int ext_part_i;

			if (!ext_base) {
			  	ext_part_i = FD_NUMPART;
				ext_base = part->relsect;
				new_base = part->relsect+base_sector;
			} else {
				new_base = part->relsect+ext_base;
				ext_part_i = *part_i;
			}

			rc = read_bios_partitions(dev,
					    new_base,
					    ext_base,
					    &ext_part_i,
					    label,
					    0,
					    rw_abs);
			if (rc != D_SUCCESS)
				goto out;
			continue;
		} 
	}
out:
	(void) kmem_free(kernel_map,
			 (vm_offset_t)mboot,
			 sizeof(*mboot));

	return rc;	
}

#if	VTOC_COMPAT
/*
 * Attempt to read a vtoc from the specified bios partition.
 */
static io_return_t
read_vtoc(
       dev_t		dev,
       int		base_sector,
       int		part_i,
       struct disklabel	*label,
       struct alt_info	**alt_info,
       io_return_t	(*rw_abs)(
				  dev_t		dev,
				  dev_status_t	data,
				  int		rw,
				  int		sec,
				  int		count))
{
	struct evtoc	*evp;
	struct alt_info *alt = 0;
	io_return_t	rc;
	register int	i, n;

        if (kmem_alloc(kernel_map,
		       (vm_offset_t *)&evp,
		       sizeof(*evp)) != KERN_SUCCESS)
		return(D_NO_MEMORY);

  	rc = (*rw_abs)(dev,
		       (dev_status_t)evp,
		       IO_READ, 
		       base_sector+HDPDLOC, 
		       sizeof(*evp));

	if (rc != D_SUCCESS)
		goto out;

	if (evp->sanity != VTOC_SANE) {
		rc = D_IO_ERROR;
		goto out;
	}

	bcopy(evp->label, label->d_packname, 16);/* truncates, too bad */
	label->d_ncylinders = evp->cyls;
	label->d_ntracks = evp->tracks;
	label->d_nsectors = evp->sectors;
	label->d_secpercyl = evp->tracks * evp->sectors;
	assert(label->d_secsize == BIOS_SECTOR_SIZE);
	label->d_bbsize = BIOS_BOOT0_SIZE;

	/* copy info on first MAXPARTITIONS */
	n = evp->nparts;
	if (n > MAXPARTITIONS) n = MAXPARTITIONS;
		/* whole disk */
	label->d_partitions[MAXPARTITIONS].p_size = -1;
	label->d_partitions[MAXPARTITIONS].p_offset = 0;
		/* "c" is never read; always calculated */
	label->d_partitions[2].p_size = label->d_partitions[part_i].p_size;
	label->d_partitions[2].p_offset = label->d_partitions[part_i].p_offset;
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
	if (alt_info) {
        	if (!*alt_info) {
			if (kmem_alloc(kernel_map,
				       (vm_offset_t *)&alt,
				       sizeof(*alt)) != KERN_SUCCESS) {
				rc = D_NO_MEMORY;
				goto out;
			}
		} else
			alt = *alt_info;

	  	rc = (*rw_abs)(dev,
			       (dev_status_t)alt,
			       IO_READ, 
			       base_sector+evp->alt_ptr/BIOS_SECTOR_SIZE, 
			       sizeof(*alt));

		if (rc != D_SUCCESS)
			goto out;

		if (alt->alt_sanity != ALT_SANITY)
			printf("dev %d: alternate sectors corrupt\n", dev);
		else {
			*alt_info = alt;
			alt = 0;
		}
	}
out:
	(void) kmem_free(kernel_map,
			 (vm_offset_t)evp,
			 sizeof(*evp));

	if (alt != 0)
		(void) kmem_free(kernel_map,
				 (vm_offset_t)alt,
				 sizeof(*alt));
	return rc;

}
#endif	/* VTOC_COMPAT */

#if	NSCSI

/*
 * Read or write sectors using absolute offsets (bypass the partitions)
 */

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

	io_req_alloc(ior);
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

#endif 	/* NSCSI */


#if AT386

io_return_t
get_dos_parms(
	int 	disk_unit,
	short 	*cyls,
	char 	*heads,
	char 	*secs)
{
	u_long		n;
	u_char		*tbl;

	switch(disk_unit) {
	case 0:
		n = *(unsigned long *)phystokv(0x104);
		break;
	case 1:
		n = *(unsigned long *)phystokv(0x118);
		break;
        default:
		return D_IO_ERROR;
	}
	
	tbl = (unsigned char *)phystokv((n&0xffff) + ((n>>12)&0xffff0));
	*cyls   = *(unsigned short *)tbl;
	*heads = *(unsigned char  *)(tbl+2);
	*secs   = *(unsigned char  *)(tbl+14);

	return D_SUCCESS;
}
#endif /* AT386 */





