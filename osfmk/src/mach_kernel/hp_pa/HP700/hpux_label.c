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
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_disksubr.c	7.17 (Berkeley) 11/1/91
 */

#include <scsi.h>
#include <platforms.h>
#include <mach/std_types.h>
#include <scsi/compat_30.h>
#include <scsi/scsi.h>
#include <scsi/scsi_defs.h>
#include <scsi/rz.h>
#include <scsi/rz_labels.h>
#include <scsi/scsi_entries.h>
#include <device/ds_routines.h>
#include <kern/misc_protos.h>
#include <hp_pa/HP700/lif.h>
#include <vm/vm_kern.h>

/*
 * Forwards
 */
boolean_t	read_hpux_label(
				target_info_t		*tgt,
				struct disklabel	*label,
				io_req_t		ior);

boolean_t	convert_lif_to_osf1(
				    struct lif_label *l_label,
				    struct disklabel *lp);

u_short		dkcksum(
			struct disklabel *lp);

#define DEV_BSIZE	1024
#undef	LABELSECTOR
#define	LABELSECTOR	(1024/DEV_BSIZE)	/* Apollo */


/*
 * Attempt to read a disk label from a device using the indicated stategy
 * routine.  The label must be partly set up before this: secpercyl and
 * anything required in the strategy routine (e.g., sector size) must be
 * filled in before calling us.  Returns NULL on success and an error
 * string on failure.
 */
/*
 * Find and convert from an HPUX label to BSD
 */
boolean_t
read_hpux_label(
	target_info_t		*tgt,
	struct disklabel	*label,
	io_req_t		ior)
{
	struct lif_header 	*l_header;
	struct lif_label 	*l_label;
	struct disklabel 	*dlp;
	vm_offset_t 		data;
	boolean_t		rc = FALSE;

        if (kmem_alloc(kernel_map,
		       (vm_offset_t *)&data,
		       DEV_BSIZE) != KERN_SUCCESS) {
		printf("cant read LIF header\n");
		return(FALSE);
	}

	if (label->d_secperunit == 0)
		label->d_secperunit = 0x1fffffff;
	label->d_npartitions = 1;
	if (label->d_partitions[0].p_size == 0)
		label->d_partitions[0].p_size = 0x1fffffff;
	label->d_partitions[0].p_offset = 0;

	ior->io_count = DEV_BSIZE;
	ior->io_op = IO_READ;
	ior->io_data = (char *)data;
	tgt->ior = ior;
	scdisk_read( tgt, LIF_HEADER_OFF/tgt->block_size, ior);
	iowait(ior);
	l_header = (struct lif_header*)ior->io_data;
	if (l_header->magic != LIF_MAGIC) {
		printf("cant find LIF_HEADER block\n");
		rc = FALSE;
		goto out;
	}

	if (!IS_OSF1_DISK(l_header)) {
		ior->io_count = DEV_BSIZE;
		ior->io_op = IO_READ;
		tgt->ior = ior;
		scdisk_read( tgt, LIF_LABEL_OFF/tgt->block_size, ior);
		iowait(ior);
		if (!convert_lif_to_osf1((struct lif_label*)ior->io_data, 
					 label)) {
			printf("No FS detected in LIF block\n");
			rc = FALSE;
			goto out;
		} else
			rc = TRUE;
	} else {
		ior->io_count = DEV_BSIZE;
		ior->io_op = IO_READ;
		tgt->ior = ior;
		scdisk_read( tgt, LABELSECTOR, ior);
		iowait(ior);

		for (dlp = (struct disklabel *)ior->io_data;
		     dlp <= (struct disklabel *)(ior->io_data
						 +DEV_BSIZE-sizeof(*dlp));
		     dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
			if (dlp->d_magic != DISKMAGIC ||
			    dlp->d_magic2 != DISKMAGIC) {
				continue;
			} else if (dlp->d_npartitions > MAXPARTITIONS ||
				   dkcksum(dlp) != 0) {
				printf("disk label corrupted\n");
				rc = FALSE;
				goto out;
			} else {
				rc = TRUE;
				*label = *dlp;
				break;
			}
		}
		if (rc == FALSE)
			printf("no disk label\n");
	}
out:
	if (data) 
		(void) kmem_free(kernel_map, data, DEV_BSIZE);
	return rc;
}

boolean_t
convert_lif_to_osf1(
	struct lif_label *l_label,
	struct disklabel *lp)
{
	boolean_t is_a_partition;
	boolean_t rc = FALSE;
	unsigned  last_block;
	struct lif_partition *l_part;

	/* LIF units are 256 bytes */
	int lif2sec = lp->d_secsize/256;

	lp->d_npartitions = 3;
	lp->d_partitions[2].p_offset = 0;
	lp->d_partitions[2].p_fstype = 0;
	lp->d_partitions[2].p_size = 0;
	for (l_part = &l_label->part[0];
	     l_part < &l_label->part[LIF_MAX_PART];
	     l_part++) {
	 	is_a_partition = TRUE;
		switch (l_part->type) {
		case LIF_FS:
			lp->d_partitions[0].p_size = l_part->size/lif2sec;
			lp->d_partitions[0].p_offset = l_part->start/lif2sec;
			lp->d_partitions[0].p_fstype = FS_BSDFFS;
			break;
		case LIF_SWAP:
			lp->d_partitions[1].p_size = l_part->size/lif2sec;
			lp->d_partitions[1].p_offset = l_part->start/lif2sec;
			lp->d_partitions[1].p_fstype = FS_SWAP;
			break;
	        case LIF_ISL:
		case LIF_AUTO:
		case LIF_HPUX:
		case LIF_IOMAP:
		case LIF_EST:
		case LIF_PAD:
			break;
		default:
			is_a_partition = FALSE;
			break;
		}
		if (!is_a_partition)
			continue;
		rc = TRUE;
		last_block = (l_part->start+l_part->size)/lif2sec;
		if (lp->d_partitions[2].p_size < last_block)
		     	lp->d_partitions[2].p_size = last_block;
	}
	return(rc);	
}


