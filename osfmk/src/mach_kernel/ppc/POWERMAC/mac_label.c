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
#include <mach_debug.h>
#include <scsi/scsi_defs.h>
#include <vm/vm_kern.h>
#include <ppc/POWERMAC/mac_label.h>
#include <ppc/machparam.h>

/* prototypes */

extern boolean_t convert_mac_to_osf1(struct mac_label *m_lab,
				     struct disklabel *lp);

extern boolean_t read_mac_label(target_info_t		*tgt,
				struct disklabel	*label,
				io_req_t		ior);

extern boolean_t rz_mac_label(target_info_t *tgt,
			      struct disklabel *label,
			      io_req_t		ior);

/*
 * Given a mac block number, convert into bsd block number,
 *    - block_size must be initialised from disk.
 */
static int block_size;

#define blk_to_bsd(count) (count)

/*
 * Find and convert from a Mac label to BSD
 */
boolean_t rz_mac_label(target_info_t *tgt,
		       struct disklabel *label,
		       io_req_t		ior)
{
	boolean_t	retval;


	/*
	 * Get physical parameters via SCSI commands
	 */

	(void) get_phys_parms(tgt, label, ior, FALSE);

	/* 
	 * Setup last partition that we can access entire physical disk
	 */

	label->d_partitions[MAXPARTITIONS].p_size = -1;
	label->d_partitions[MAXPARTITIONS].p_offset = 0;
	tgt->ior = ior;

	retval = read_mac_label(tgt, label, ior);

	return retval;
}

/*
 * Attempt to read a disk label from a device using the indicated stategy
 * routine.  The label must be partly set up before this: secpercyl and
 * anything required in the strategy routine (e.g., sector size) must be
 * filled in before calling us.  Returns NULL on success and an error
 * string on failure.
 */
/*
 * Find and convert from a Macintosh label to BSD
 */

boolean_t read_mac_label(target_info_t		*tgt,
			 struct disklabel	*label,
			 io_req_t		ior)
{
	struct mac_driver_descriptor *m_dd;
	struct mac_label 	*m_lab;
	struct disklabel 	*dlp;
	vm_offset_t 		data=0;
	boolean_t		rc = TRUE;
	boolean_t		is_a_partition;
	int			partition_index;
	int			i,j;


	/* This code currently assumes that the physical
	 * block size on the medium is 512 bytes
	 * - TODO NMGS fix for CDROMs with 2k blocks
	 */
	if (tgt->block_size != 512) {
		rc = FALSE;
		goto out;
	}		

        if (kmem_alloc(kernel_map,
		       (vm_offset_t *)&data,
		       DEV_BSIZE) != KERN_SUCCESS) {
		printf("can't allocate memory for mac label\n");
		return(FALSE);
	}

	if (label->d_secperunit == 0)
		label->d_secperunit = 0x1fffffff;
	label->d_npartitions = 0;
	if (label->d_partitions[0].p_size == 0)
		label->d_partitions[0].p_size = 0x1fffffff;
	label->d_partitions[0].p_offset = 0;

	ior->io_count = DEV_BSIZE;
	ior->io_op = IO_READ;
	ior->io_data = (char *)data;
	tgt->ior = ior;
	scdisk_read( tgt, MAC_DRIVER_DESCRIPTOR_OFFSET, ior);
	iowait(ior);
	m_dd = (struct mac_driver_descriptor*)ior->io_data;

#if MACH_DEBUG
	if (scsi_debug) {
		printf("Mac disk driver descriptor:\n");
		printf("sbSig       = 0x%x\n",m_dd->sbSig);
		printf("sbBlockSize = 0x%x\n",m_dd->sbBlockSize);
		printf("sbBlkCount  = 0x%x\n",m_dd->sbBlkCount);
		printf("sbDevType   = 0x%x\n",m_dd->sbDevType);
		printf("sbDevId     = 0x%x\n",m_dd->sbDevId);
		printf("sbData      = 0x%x\n",m_dd->sbData);
		printf("sbDrvrCount = 0x%x\n",m_dd->sbDrvrCount);
	}
#endif /* MACH_DEBUG */

	if (m_dd->sbSig != MAC_DRIVER_DESCRIPTOR_MAGIC) {
		rc = FALSE;
		goto out;
	}

	block_size = m_dd->sbBlockSize;

	/* Set up label */

	/* bcopy("Mac disk", label->d_packname, 16); */

	label->d_secsize = block_size;

	/* Ok, we've got a Mac disk, let's read in the partition tables */

	partition_index = MAC_PARTITION_BLOCK_OFFSET;

	m_lab = (struct mac_label*) ior->io_data;

	/* we cheat and read partitions up until there is an invalid
	 * entry, rather than reading only within the valid block count.
	 */
	while (1) {


		ior->io_count = DEV_BSIZE;
		ior->io_op = IO_READ;
		tgt->ior = ior;
		scdisk_read( tgt, partition_index++,  ior);
		iowait(ior);

		/* We look through partitions until no magic left */

		if (m_lab->pmSig != MAC_PARTITION_MAGIC) {
			
			goto out; /* No more magic */
		}
			
#if MACH_DEBUG
		if (scsi_debug) {
			printf("partition map at 0x%08x:\n",m_lab);
			printf("pmSig        = 0x%x\n",m_lab->pmSig);
			printf("pmSigPad     = 0x%x\n",m_lab->pmSigPad);
			printf("pmMapBlkCnt  = 0x%x\n",m_lab->pmMapBlkCnt);
			printf("pmPyPartStart= 0x%x\n",m_lab->pmPyPartStart);
			printf("pmPartBlkCnt = 0x%x\n",m_lab->pmPartBlkCnt);
			printf("pmPartName   = '%s'\n",m_lab->pmPartName);
			printf("pmPartType   = '%s'\n",m_lab->pmPartType);
			printf("pmLgDataStart= 0x%x\n",m_lab->pmLgDataStart);
			printf("pmDataCnt    = 0x%x\n",m_lab->pmDataCnt);
			printf("pmPartStatus = 0x%x\n",m_lab->pmPartStatus);
			printf("pmLgBootStart= 0x%x\n",m_lab->pmLgBootStart);
			printf("pmBootSize   = 0x%x\n",m_lab->pmBootSize);
			printf("pmBootLoad   = 0x%x\n",m_lab->pmBootLoad);
			printf("pmBootLoad2  = 0x%x\n",m_lab->pmBootLoad2);
			printf("pmBootEntry  = 0x%x\n",m_lab->pmBootEntry);
			printf("pmBootEntry2 = 0x%x\n",m_lab->pmBootEntry2);
			printf("pmBootCksum  = 0x%x\n",m_lab->pmBootCksum);
			printf("pmProcessor  = '%s'\n",m_lab->pmProcessor);
		}
#endif /* MACH_DEBUG */
		
		convert_mac_to_osf1(m_lab, label);
	}

out:
	/* Set up the last partition to map the whole disk */

	label->d_partitions[label->d_npartitions].p_size =
		blk_to_bsd(m_dd->sbBlkCount);
	label->d_partitions[label->d_npartitions].p_offset = 0;
	label->d_partitions[label->d_npartitions].p_fstype = 0;

	label->d_npartitions++;

	if (data) 
		(void) kmem_free(kernel_map, data, DEV_BSIZE);

	return rc;
}


/*
** This function is the same as the above function, except that
** it does not assume that it is on a SCSI system, and it takes
** different parameters. Somehow, these two functions should probably
** be combined with one API
*/
io_return_t 
read_ata_mac_label(
	       dev_t		dev,
	       int		base_sector,	/* base sector for partition */
	       int		ext_base,	/* base for extention */
	       int		*part_i,	/* partition index */
	       struct disklabel	*label,
	       io_return_t	(*rw_abs)(
					  dev_t		dev,
					  dev_status_t	data,
					  int		rw,
					  int		sec,
					  int		count))
{
	struct mac_driver_descriptor *m_dd;
	struct mac_label 	*m_lab;
	struct disklabel 	*dlp;
	vm_offset_t 		data=0;
	io_return_t		rc = TRUE;
	boolean_t		is_a_partition;
	int			partition_index;
	int			i,j;

        if (kmem_alloc(kernel_map,
		       (vm_offset_t *)&data,
		       DEV_BSIZE) != KERN_SUCCESS) {
		printf("can't allocate memory for mac label\n");
		return(FALSE);
	}

	/*
	 * read it
	 */
  	rc = (*rw_abs)(dev,
		       (dev_status_t)data,
		       IO_READ, 
		       base_sector, 
		       DEV_BSIZE);

	if (rc != D_SUCCESS)
		goto out;

	m_dd = (struct mac_driver_descriptor*)data;

	if (m_dd->sbSig != MAC_DRIVER_DESCRIPTOR_MAGIC) {
		printf("\nBAD SIGNATURE\n");
	        rc = D_IO_ERROR;
		goto out;
	}

	block_size = m_dd->sbBlockSize;

	/* Set up label */

	/* bcopy("Mac disk", label->d_packname, 16); */

	label->d_secsize = block_size;

	/* Ok, we've got a Mac disk, let's read in the partition tables */

	partition_index = MAC_PARTITION_BLOCK_OFFSET;

	m_lab = (struct mac_label*) data;

	/* we cheat and read partitions up until there is an invalid
	 * entry, rather than reading only within the valid block count.
	 */
	while (1) {


		rc = (*rw_abs)(dev,
				   (dev_status_t)data,
				   IO_READ, 
				   base_sector + partition_index, 
				   DEV_BSIZE);
	
		if (rc != D_SUCCESS)
			goto out;

		/* We look through partitions until no magic left */

		if (m_lab->pmSig != MAC_PARTITION_MAGIC) {
			
			goto out; /* No more magic */
		}
			
		convert_mac_to_osf1(m_lab, label);

		partition_index++;
	}

out:
	/* Set up the last partition to map the whole disk */

	label->d_partitions[label->d_npartitions].p_size =
		blk_to_bsd(m_dd->sbBlkCount);
	label->d_partitions[label->d_npartitions].p_offset = 0;
	label->d_partitions[label->d_npartitions].p_fstype = 0;

	label->d_npartitions++;

	if (data) 
		(void) kmem_free(kernel_map, data, DEV_BSIZE);

	return rc;
}

/* This function populates the disklabel structure from the mac_label,
 * returning TRUE if a unix partition is found. Whatever the partition
 * type, it is still inserted in the label.
 */

boolean_t convert_mac_to_osf1(struct mac_label *m_lab,
			      struct disklabel *lp)
{
	int i;
	boolean_t rc = TRUE;
	int partno = lp->d_npartitions++;


	lp->d_partitions[partno].p_size =
		blk_to_bsd(m_lab->pmPartBlkCnt);
	lp->d_partitions[partno].p_offset =
		blk_to_bsd(m_lab->pmPyPartStart);

	if (strcmp(m_lab->pmPartType, MAC_PART_UNIX_SVR2_TYPE) == 0) {

#if MACH_DEBUG
		struct mac_unix_bargs *bargs = &m_lab->pmBootArgs.unix_bargs;
		if (scsi_debug) {
			printf("boot args flags = 0x%4x\n",bargs->uxFlags);
		}
#endif /* MACH_DEBUG */

		lp->d_partitions[partno].p_fstype = FS_BSDFFS;
	} else {

		/* Must be a mac partition - mark as unused for now */
		/* TODO NMGS put a sane value here */
		lp->d_partitions[partno].p_fstype = FS_UNUSED;
		rc = FALSE;
	}

	return rc;
}
