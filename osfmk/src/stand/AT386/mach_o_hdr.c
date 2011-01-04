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
 *	NB: When the i386 goes to a tertiary boot, we space should
 *	be less tight, so SMALLHD will not be necessary and the
 *	standard version of this file in libld can be used.
 */

/*
 * mach_o_hdr.c
 *
 * utility routines to access the headers of Mach-O (OSF/ROSE) object
 * files, which are in canonical form (i.e. have the same bit
 * representation on all machines)
 *
 *****These routines are machine dependent.*****
 *      This is the version for the i386.
 *
 * There are copies of mach_o_hdr in several places in the source tree.
 * All of the copies for a given machine should be identical.
 *
 */


#include <mach_o_header.h>
#include <mach_o_header_md.h>
#include <machine/endian.h>
#include <sys/types.h>

#define MY_MAX_HDR_VERSION	1  /* max version this program knows about */
       static unsigned long 
	                mo_raw_hdr_size[MY_MAX_HDR_VERSION+1] 
	                        = {0, 56};  /* don't use element 0;
					     *  hdr version begins with 1 */

#ifdef __STDC__			/* ANSI compilant compiler */
        typedef void *generic_ptr_t;
#else /* __STDC__ */		/* non-ANSI compiler */
        typedef char *generic_ptr_t;
#endif /* __STDC__ */

/* decode_mach_o_hdr - convert the canonical header from a Mach-O (OSF/ROSE)
 * 			object file to readable form
 */
int 
decode_mach_o_hdr (in_bufp, in_bufsize, hdr_version, headerp)
       generic_ptr_t	in_bufp;
       size_t		in_bufsize;
       unsigned long	hdr_version;
       mo_header_t	*headerp;
{
	unsigned long	raw_hdr_version;
	unsigned long	min_hdr_version;
	raw_mo_header_t	*rhp;

	if ((hdr_version == 0) || (hdr_version > MOH_HEADER_VERSION))
		return (MO_ERROR_BAD_HDR_VERS);

	if (in_bufsize < mo_raw_hdr_size[hdr_version])
		return (MO_ERROR_BUF2SML);

	/* Check that the input buffer contains a header that we can read. */

	rhp = (raw_mo_header_t *)in_bufp;

	if (rhp->rmoh_magic != OUR_MOH_MAGIC)
		return (MO_ERROR_BAD_MAGIC);

	raw_hdr_version = (unsigned long) ntohs(rhp->rmoh_header_version);
	if ((raw_hdr_version == 0) 
	    || (raw_hdr_version > MO_RAW_HEADER_VERSION))
		return (MO_ERROR_BAD_RAW_HDR_VERS);
	
	(hdr_version < raw_hdr_version) ? 
		(min_hdr_version = hdr_version) 
		: (min_hdr_version = raw_hdr_version);

	if (min_hdr_version > MY_MAX_HDR_VERSION)
		return (MO_ERROR_UNSUPPORTED_VERS);

	/* Fill in output structure. */

	headerp->moh_magic = 		ntohl (rhp->rmoh_magic);
	headerp->moh_major_version = 	ntohs (rhp->rmoh_major_version);
	headerp->moh_minor_version = 	ntohs (rhp->rmoh_minor_version);
	headerp->moh_header_version = 	ntohs (rhp->rmoh_header_version);
	headerp->moh_max_page_size = 	ntohs (rhp->rmoh_max_page_size);
	headerp->moh_byte_order = 	ntohs (rhp->rmoh_byte_order);
	headerp->moh_data_rep_id = 	ntohs (rhp->rmoh_data_rep_id);
	headerp->moh_cpu_type = 	ntohl (rhp->rmoh_cpu_type);
	headerp->moh_cpu_subtype = 	ntohl (rhp->rmoh_cpu_subtype);
	headerp->moh_vendor_type = 	ntohl (rhp->rmoh_vendor_type);
	headerp->moh_flags = 		ntohl (rhp->rmoh_flags);
	headerp->moh_load_map_cmd_off = ntohl (rhp->rmoh_load_map_cmd_off);
	headerp->moh_first_cmd_off = 	ntohl (rhp->rmoh_first_cmd_off);
	headerp->moh_sizeofcmds = 	ntohl (rhp->rmoh_sizeofcmds);
	headerp->moh_n_load_cmds = 	ntohl (rhp->rmoh_n_load_cmds);
	headerp->moh_reserved[0] =	ntohl (rhp->rmoh_reserved[0]);
	headerp->moh_reserved[1] =	ntohl (rhp->rmoh_reserved[1]);

	/* This is the end if one or both structures are header version 1.
	 * If the versions are different, copy only the amount 
	 * corresponding to the smaller version.
	 */

	if (min_hdr_version > 1) ;  /* later version support starts here */

	return (MO_HDR_CONV_SUCCESS);
}


#ifndef SMALLHD
/* encode_mach_o_hdr - convert a Mach-O (OSF/ROSE) object file header from
 * native, readable form to canonical form
 */
int
encode_mach_o_hdr (headerp, out_bufp, out_bufsize)
       mo_header_t	*headerp;
       generic_ptr_t	out_bufp;
       size_t		out_bufsize;
{
	raw_mo_header_t	*rhp;
	unsigned long	in_hdr_version;

	in_hdr_version = headerp->moh_header_version;

	if (headerp->moh_magic != MOH_MAGIC)
		return (MO_ERROR_BAD_MAGIC);

	if ((in_hdr_version == 0)
	    || (in_hdr_version > MOH_HEADER_VERSION))
		return (MO_ERROR_BAD_HDR_VERS);

	if (out_bufsize < mo_raw_hdr_size[in_hdr_version])
		return (MO_ERROR_BUF2SML);

	if (in_hdr_version > MO_RAW_HEADER_VERSION)
		return (MO_ERROR_OLD_RAW_HDR_FILE);

	if (in_hdr_version > MY_MAX_HDR_VERSION)
		return (MO_ERROR_UNSUPPORTED_VERS);

	rhp = (raw_mo_header_t *)out_bufp;

	rhp->rmoh_magic = 		htonl (headerp->moh_magic);
	rhp->rmoh_major_version = 	htons (headerp->moh_major_version);
	rhp->rmoh_minor_version = 	htons (headerp->moh_minor_version);
	rhp->rmoh_header_version = 	htons (headerp->moh_header_version);
	rhp->rmoh_max_page_size = 	htons (headerp->moh_max_page_size);
	rhp->rmoh_byte_order = 		htons (headerp->moh_byte_order);
	rhp->rmoh_data_rep_id = 	htons (headerp->moh_data_rep_id);
	rhp->rmoh_cpu_type = 		htonl (headerp->moh_cpu_type);
	rhp->rmoh_cpu_subtype = 	htonl (headerp->moh_cpu_subtype);
	rhp->rmoh_vendor_type = 	htonl (headerp->moh_vendor_type);
	rhp->rmoh_flags = 		htonl (headerp->moh_flags);
	rhp->rmoh_load_map_cmd_off = 	htonl (headerp->moh_load_map_cmd_off);
	rhp->rmoh_first_cmd_off = 	htonl (headerp->moh_first_cmd_off);
	rhp->rmoh_sizeofcmds = 		htonl (headerp->moh_sizeofcmds);
	rhp->rmoh_n_load_cmds = 	htonl (headerp->moh_n_load_cmds);
	rhp->rmoh_reserved[0] =		htonl (headerp->moh_reserved[0]);
	rhp->rmoh_reserved[1] =		htonl (headerp->moh_reserved[1]);

	/* This is the end if the header version is 1. */

	if (in_hdr_version > 1) ;  /* later version support starts here */

	return (MO_HDR_CONV_SUCCESS);
}
#endif	/* !SMALLHD */
