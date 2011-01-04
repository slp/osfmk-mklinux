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
/*	$CHeader: types.h 1.2 93/12/06 04:51:47 $	*/

/* 
 * Revised Copyright Notice:
 *
 * Copyright 1993, 1994 CONVEX Computer Corporation.  All Rights Reserved.
 *
 * This software is licensed on a royalty-free basis for use or modification,
 * so long as each copy of the software displays the foregoing copyright notice.
 *
 * This software is provided "AS IS" without warranty of any kind.  Convex 
 * specifically disclaims the implied warranties of merchantability and
 * fitness for a particular purpose.
 */

#ifndef	_KGDB_TYPES_H_
#define	_KGDB_TYPES_H_

/* 
 * This ethernet/ip/udp stuff has no business being here: gdb does not
 * have any knowledge of the transport mechanism.  These really belong
 * someplace that the hp700 i596 driver and kgdb_support driver can
 * get at them. XXX FIXME XYZZY
 */

struct ip_udp_header {
	char	filler1[6];	/* ip vers, tos, len, id */
	char    ip_flags[2];	/* flags, frag offset */
	char    filler2;	/* ttl */
	char	ip_protocol;
	char	ip_checksum[2];
	char	filler3[8];	/* ip src, dst */
	char	filler4[2];	/* udp src */
	short	udp_dest_port;
	char	filler5[4];	/* udp len, cksum */
};

#define ETHERTYPE_IP	0x0800
#define ETHERTYPE_ARP	0x0806
#define IPPROTO_UDP	17
#define GDB_PORT	468

/*
 * This ain't a type or a constant, but it breaks our screwed-up
 * header file mess if it's in kgdb.h or kgdb_stub.h.  This is because
 * machine/break.h has to have a definition of this variable.  The
 * variable is actually in kgdb_stub.c.  XXX FIXME XYZZY
 */

extern boolean_t 	kgdb_initialized;

#endif	/* _KGDB_TYPES_H_ */
