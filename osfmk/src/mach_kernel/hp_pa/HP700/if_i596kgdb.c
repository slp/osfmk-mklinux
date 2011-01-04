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
/*	From $CHeader: if_i596.c 1.5 93/04/26 17:00:46 $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * Copyright (c) 1991,1992 The University of Utah and
 * the Center for Software Science (CSS).
 * Copyright (c) 1993, CONVEX Computer Corporation.
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH, CSS, and CONVEX ALLOW FREE
 * USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY
 * LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
 * USE OF THIS SOFTWARE.
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
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: if_i596kgdb.c
 */
/* 
 *	Olivetti PC586 Mach Ethernet driver v1.0
 *	Copyright Ing. C. Olivetti & C. S.p.A. 1988, 1989
 *	All rights reserved.
 *
 */ 
/*
  Copyright 1988, 1989 by Olivetti Advanced Technology Center, Inc.,
Cupertino, California.

		All Rights Reserved

  Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Olivetti
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

  OLIVETTI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL OLIVETTI BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUR OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <kgdb.h>
#include <kgdb_lan.h>

#include <types.h>
#include <machine/spl.h>
#include <machine/asp.h>	/* XXX */
#include <mach/machine/vm_types.h>

#include <kern/time_out.h>
#include <device/device_types.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <device/if_hdr.h>
#include <device/if_ether.h>
#include <device/net_status.h>
#include <device/net_io.h>

#include <hp_pa/HP700/device.h>
#include <sys/time.h>
#include <hp_pa/HP700/if_i596var.h>
#include <hp_pa/HP700/kgdb_support.h>
#include <kern/misc_protos.h>

extern pc_softc_t pc_softc[];

/* 
 * Externs
 */
extern int pc586debug_read(int, volatile fd_t *);
extern volatile char *ram_to_ptr(u_short, int);

struct ip_udp_header {
	u_char	filler1[9];	/* ip vers, tos, len, id, frag, ttl */
	u_char	ip_protocol;
	u_char	filler2[10];	/* ip cksum, src, dst */
	u_char	filler3[2];	/* udp src */
	u_short	udp_dest_port;
	u_char	filler4[4];	/* udp len, cksum */
};

#define htons(s)	(s)
#define ntohs(s)	(s)
#define ETHERTYPE_IP	0x0800
#define ETHERTYPE_ARP	0x0806
#define IPPROTO_UDP	17
#define GDB_UDPPORT	468
char debug_read_buffer[ETHERMTU];

extern boolean_t kgdb_active;

int
pc586debug_read(
	int	unit,
	volatile fd_t	*fd_p)
{
	register pc_softc_t	*is = &pc_softc[unit];
	struct ether_header	*ehp;
	struct ip_udp_header	*uhp;
	char			*dp;
	rbd_t			*rbd_p;
	u_char			*buffer_p;
	u_short			len;
	u_short			bytes_in_msg;

	rbd_p = (rbd_t *)ram_to_ptr(fd_p->rbd_offset, unit);
	if (rbd_p == 0)
		return 0;	/* pc586read() will straighten this out */

	/*
	 * Peek at the header to determine whether it's for the debugger.
	 * If not, we don't want to copy the packet.
	 */
	pcacheline(HP700_SID_KERNEL, (vm_offset_t)rbd_p);
	buffer_p = (u_char *) (rbd_p->buffer_base << 16) + rbd_p->buffer_addr;
	bytes_in_msg = rbd_p->status & RBD_SW_COUNT;
	pdcache(HP700_SID_KERNEL, (vm_offset_t)buffer_p, bytes_in_msg);
	uhp = (struct ip_udp_header *) buffer_p;

	if ((bytes_in_msg > sizeof(struct ip_udp_header) &&
	     bcmp((char *)fd_p->destination,
		  (char *)is->ds_addr, ETHER_ADD_SIZE) == 0 &&
	     fd_p->length == htons(ETHERTYPE_IP) &&
	     uhp->ip_protocol == IPPROTO_UDP &&
	     uhp->udp_dest_port == htons(GDB_UDPPORT)))
		ehp = (struct ether_header *) debug_read_buffer;
	else if (kgdb_active)
	    	/*
		 * While stopped at a breakpoint, drop everything else.
		 * This prevents the driver from calling the net_io layer
		 * and possibly running into a breakpoint.
		 */
	    	return 1;
	else
		return 0;

	/*
	 * Get ether header.
	 */
	ehp->ether_type = fd_p->length;
	len = sizeof(struct ether_header);
	bcopy16(fd_p->source, ehp->ether_shost, ETHER_ADD_SIZE);
	bcopy16(fd_p->destination, ehp->ether_dhost, ETHER_ADD_SIZE);

	/*
	 * Get packet body.
	 */
	dp = (char *)(ehp + 1);

	do {
		pcacheline(HP700_SID_KERNEL, (vm_offset_t)rbd_p);
		buffer_p = (u_char *)
		    (rbd_p->buffer_base << 16) + rbd_p->buffer_addr;
		bytes_in_msg = rbd_p->status & RBD_SW_COUNT;
		pdcache(HP700_SID_KERNEL, (vm_offset_t)buffer_p, bytes_in_msg);
		bcopy16((u_short *)buffer_p,
			(u_short *)dp,
			(bytes_in_msg + 1) & ~1); /* but we know it's even */
		len += bytes_in_msg;
		dp += bytes_in_msg;
		if (rbd_p->status & RBD_SW_EOF)
			break;
		rbd_p = (rbd_t *)ram_to_ptr(rbd_p->next_rbd_offset, unit);
	} while ((int) rbd_p);

#if KGDB_LAN
	if (kgdb_packet_input(debug_read_buffer, len)) {
		return(1);
	}
#endif /* KGDB_LAN */

	return(0);
}

void
pc586debug_xmt(
	int	unit,
	char *	buffer,
	int	count)
{
	pc_softc_t			*is = &pc_softc[unit];
	register u_char			*xmtdata_p = (u_char *)(is->sram + OFFSET_TBUF);
	register u_short		*xmtshort_p;
	register struct ether_header	*eh_p = (struct ether_header *)buffer;
	volatile scb_t			*scb_p = (volatile scb_t *)(is->sram + OFFSET_SCB);
	volatile ac_t			*cb_p = (volatile ac_t *)(is->sram + OFFSET_CU);
	tbd_t				*tbd_p = (tbd_t *)(is->sram + OFFSET_TBD);
	u_short				tbd = OFFSET_TBD;
	u_short				len, clen = 0;

	cb_p->ac_status = 0;
	cb_p->ac_command = (AC_CW_EL|AC_TRANSMIT|AC_CW_I);
	cb_p->ac_link_offset = PC586NULL;
	cb_p->cmd.transmit.tbd_offset = OFFSET_TBD;

	bcopy16(eh_p->ether_dhost, cb_p->cmd.transmit.dest_addr, ETHER_ADD_SIZE);
	cb_p->cmd.transmit.length = (u_short)(eh_p->ether_type);
	fdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof *cb_p);

	tbd_p->act_count = 0;
	tbd_p->buffer_addr = (u_short)((int)xmtdata_p&0xffff);
	tbd_p->buffer_base = (int)xmtdata_p >> 16;

	bcopy16(is->ds_addr, buffer + 6, 6); /* source */

	{ int Rlen, Llen;
	    clen = count - sizeof(struct ether_header);
	    Llen = clen & 1;
	    Rlen = ((int)(buffer + sizeof(struct ether_header))) & 1;

	    bcopy16(buffer + sizeof(struct ether_header) - Rlen,
		    xmtdata_p,
		    clen + (Rlen + Llen) );
	    xmtdata_p += clen + Llen;
	    tbd_p->act_count = clen;
	    tbd_p->buffer_addr += Rlen;
	}

	if (clen < ETHERMIN) {
		tbd_p->act_count += ETHERMIN - clen;
		for (xmtshort_p = (u_short *)xmtdata_p;
		     clen < ETHERMIN;
		     clen += 2) *xmtshort_p++ = 0;
		xmtdata_p = (u_char *)xmtshort_p;
	}
	tbd_p->act_count |= TBD_SW_EOF;
	tbd_p->next_tbd_offset = PC586NULL;
	fcacheline(HP700_SID_KERNEL, (vm_offset_t)tbd_p);
	/* XXX take into account padding of odd length buffers */
	len = xmtdata_p - (u_char *)(is->sram + OFFSET_TBUF);
	if (len < clen)
		panic("pc586xmt: screwed the pooch");
	fdcache(HP700_SID_KERNEL, (vm_offset_t)(is->sram + OFFSET_TBUF), len);

	do
		purgescbcmd(scb_p);
	while (scb_p->scb_command);
	scb_p->scb_command = SCB_CU_STRT;
	flushscbcmd(scb_p);

	pc_softc[unit].hwaddr->attn = 0;
}
