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
 * File : ns8390.c
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains ns8390-based boards functions used for Network bootstrap.
 */

#ifdef	DEVICE_NS8390

#include <secondary/net.h>
#include <secondary/net_debug.h>
#include <secondary/net/ns8390.h>
#include <secondary/net/3c503.h>
#include <secondary/net/smc.h>
#include <secondary/net/dlink.h>
#include <secondary/net/endian.h>
#include <secondary/protocols/udpip.h>
#include <secondary/protocols/arp.h>

void 	ns8390_reset(void);
int 	ns8390_rcv(void *, unsigned);
void	ns8390_overwrite(char *, unsigned);

void
ns8390_reset(void)
{
	u8bits temp;
	unsigned i;

	(*ns8390.ns_reset)();
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, NS8390_STP|NS8390_RD4|NS8390_PS0);
	delayed_outb(ns8390.ns_nicbase + NS8390_RBCR0, 0);
	delayed_outb(ns8390.ns_nicbase + NS8390_RBCR1, 0);
	while ((inb(ns8390.ns_nicbase + NS8390_ISR) & NS8390_RST) == 0)
		continue;

	if (ns8390.ns_flags & NS8390_FLAGS_SLOT_16BITS) {
#if	BYTE_ORDER == LITTLE_ENDIAN
		temp = NS8390_WTS|NS8390_BOS|NS8390_FT2;
#endif
#if	BYTE_ORDER == BIG_ENDIAN
		temp = NS8390_WTS|NS8390_FT2;
#endif
	} else
		temp = NS8390_FT2;

	delayed_outb(ns8390.ns_nicbase + NS8390_DCR, temp);
	delayed_outb(ns8390.ns_nicbase + NS8390_RCR, NS8390_MON);
	delayed_outb(ns8390.ns_nicbase + NS8390_TCR, NS8390_LB2);
	delayed_outb(ns8390.ns_nicbase + NS8390_PSTART, ns8390.ns_pstart);
	delayed_outb(ns8390.ns_nicbase + NS8390_PSTOP, ns8390.ns_pstop);
	delayed_outb(ns8390.ns_nicbase + NS8390_BNRY, ns8390.ns_pstart);
	delayed_outb(ns8390.ns_nicbase + NS8390_ISR, 0xFF);
	delayed_outb(ns8390.ns_nicbase + NS8390_IMR, 0);

	delayed_outb(ns8390.ns_nicbase + NS8390_CR, NS8390_STP|NS8390_RD4|NS8390_PS1);
	for (i = 0; i < 6; i++)
		delayed_outb(ns8390.ns_nicbase + NS8390_PAR + i, dlink.dl_laddr[i]);
	for (i = 0; i < 8; i++)
		delayed_outb(ns8390.ns_nicbase + NS8390_MAR + i, 0);

	ns8390.ns_packet = ns8390.ns_pstart + 1;
	delayed_outb(ns8390.ns_nicbase + NS8390_CURR, ns8390.ns_packet);
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, NS8390_STA|NS8390_RD4|NS8390_PS0);
	delayed_outb(ns8390.ns_nicbase + NS8390_RCR, NS8390_AB);
	delayed_outb(ns8390.ns_nicbase + NS8390_TCR, 0);

#if	defined(NETDEBUG) && defined(NS8390DEBUG)
	NPRINT(("RESET : pstart = %x, pstop = %x, packet = %x",
		       ns8390.ns_pstart, ns8390.ns_pstop, ns8390.ns_packet));
#endif
	temp = inb(ns8390.ns_nicbase + NS8390_CR) & NS8390_PS_MASK;
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, temp|NS8390_PS1);
#if	defined(NETDEBUG) && defined(NS8390DEBUG)
	NPRINT((", curr = %x)\n", inb(ns8390.ns_nicbase + NS8390_CURR)));
#endif
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, temp|NS8390_PS0);
}

int
ns8390_rcv(void *addr,
	   unsigned len)
{
	volatile u8bits *ptr;
	u8bits cr;
	unsigned hbc;
	unsigned nic_overcount;
	unsigned pkt_size;
	unsigned wrap_size;
	unsigned ret;
	struct ns8390_input input;

	ptr = ((u8bits *)ns8390.ns_rambase) + (ns8390.ns_packet << 8);
	pcpy_in((u8bits *)ptr, &input, 4);

#if	defined(NETDEBUG) && defined(NS8390DEBUG) 
	NPRINT(("ns8390_rcv (ptr = 0x%x, stat = 0x%x, next = 0x%x, ",
		       (u8bits *)ptr, input.ns_status, input.ns_next));
#endif
	cr = inb(ns8390.ns_nicbase + NS8390_CR) & NS8390_PS_MASK;
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, cr|NS8390_PS1);
#if	defined(NETDEBUG) && defined(NS8390DEBUG) 
	NPRINT(("rbc0 = 0x%x, rbc1 = 0x%x, curr = 0x%x\n",
		       input.ns_lbc, input.ns_hbc,
		       inb(ns8390.ns_nicbase + NS8390_CURR)));
#endif
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, cr|NS8390_PS0);

	if (input.ns_next < ns8390.ns_pstart ||
	    input.ns_next >= ns8390.ns_pstop) {
		ns8390_reset();
		return(1);
	}

	if ((input.ns_lbc + NS8390_HEADER_SIZE) > NS8390_PAGE_SIZE)
		nic_overcount = 2;
	else
		nic_overcount = 1;
	if (input.ns_next > ns8390.ns_packet) {
		wrap_size = 0;
		hbc = input.ns_next - ns8390.ns_packet - nic_overcount;
	} else {
		wrap_size = ((unsigned) (ns8390.ns_pstop -
					 ns8390.ns_packet) << 8) -
						 NS8390_HEADER_SIZE;
		hbc = ns8390.ns_pstop - ns8390.ns_packet +
			input.ns_next - ns8390.ns_pstart - nic_overcount;
	}
	pkt_size = (hbc << 8) | (input.ns_lbc & 0xFF);

#if	defined(NETDEBUG) && defined(NS8390DEBUG)
	NPRINT(("ns8390_rcv (len = %x, pkt_size = %x, wrap_size = %x)\n",
		       len, pkt_size, wrap_size));
#endif
	if (pkt_size < 60) {
		ns8390_reset();
		return (1);
	}

	if (wrap_size > pkt_size)
		wrap_size = 0;

	ptr += NS8390_HEADER_SIZE;
	if (pkt_size <= len)
		if (ns8390.ns_flags & NS8390_FLAGS_SLOT_16BITS) {
			if (ns8390.ns_en16bits)
				(*ns8390.ns_en16bits)();
			if (wrap_size) {
				pcpy16_in((u8bits *)ptr, addr, wrap_size);
				ptr = ((u8bits *)ns8390.ns_rambase) +
					(ns8390.ns_pstart << 8);
			}
			pcpy16_in((u8bits *)ptr, ((char *)addr) + wrap_size,
				  pkt_size - wrap_size);
			if (ns8390.ns_dis16bits)
				(*ns8390.ns_dis16bits)();
		} else {
			if (wrap_size) {
				pcpy_in((u8bits *)ptr, addr, wrap_size);
				ptr = ((u8bits *)ns8390.ns_rambase) +
					(ns8390.ns_pstart << 8);
			}
			pcpy_in((u8bits *)ptr, ((char *)addr) + wrap_size,
				pkt_size - wrap_size);
		}

	if ((ns8390.ns_packet = input.ns_next) == ns8390.ns_pstart) {
		delayed_outb(ns8390.ns_nicbase + NS8390_BNRY, ns8390.ns_pstop - 1);
	} else {
		delayed_outb(ns8390.ns_nicbase + NS8390_BNRY, input.ns_next - 1);
	}
	if (pkt_size <= len)
		ether_input(addr, pkt_size);

	cr = inb(ns8390.ns_nicbase + NS8390_CR) & NS8390_PS_MASK;
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, cr | NS8390_PS1);
	ret = ns8390.ns_packet == inb(ns8390.ns_nicbase + NS8390_CURR);
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, cr | NS8390_PS0);
	return (ret);
}

void
ns8390_overwrite(char *addr,
		 unsigned len)
{
	u8bits cr;

	if (ns8390.ns_flags & NS8390_FLAGS_WD83C690) {
		cr = inb(ns8390.ns_nicbase + NS8390_CR) &
			NS8390_PS_MASK & ~NS8390_TXP;
		delayed_outb(ns8390.ns_nicbase + NS8390_CR, cr | NS8390_PS1);
		if (ns8390.ns_packet != inb(ns8390.ns_nicbase + NS8390_CURR)) {
			delayed_outb(ns8390.ns_nicbase + NS8390_CR, cr | NS8390_PS0);
			while (!ns8390_rcv((char *)0, 0))
				continue;
		} else {
			delayed_outb(ns8390.ns_nicbase + NS8390_CR, cr | NS8390_PS0);
			delayed_outb(ns8390.ns_nicbase + NS8390_BNRY,
			     inb(ns8390.ns_nicbase + NS8390_BNRY));
		}
	} else {
		delayed_outb(ns8390.ns_nicbase + NS8390_CR,
		     NS8390_STP|NS8390_RD4|NS8390_PS0);
		delayed_outb(ns8390.ns_nicbase + NS8390_RBCR0, 0);
		delayed_outb(ns8390.ns_nicbase + NS8390_RBCR1, 0);
		while ((inb(ns8390.ns_nicbase + NS8390_ISR) & NS8390_RST) == 0)
			continue;
		delayed_outb(ns8390.ns_nicbase + NS8390_TCR, NS8390_LB1);
		while (!ns8390_rcv((char *)0, 0))
			continue;
		delayed_outb(ns8390.ns_nicbase + NS8390_RCR, NS8390_AB);
		delayed_outb(ns8390.ns_nicbase + NS8390_TCR, 0);
	}
}

void
ns8390_input(void * addr,
	     unsigned len)
{
	u8bits status;

	status = inb(ns8390.ns_nicbase + NS8390_ISR);

	if (status & NS8390_RXE) {
		/*
		 * Error on packet received. Just clear it !
		 */
#if	defined(NETDEBUG) /* && defined(NS8390DEBUG) */
		if (netdebug &&
		    (inb(ns8390.ns_nicbase + NS8390_RSR) & NS8390_PRX) == 0)
			printf("NS8390 input error (status = 0x%x)\n",
			       inb(ns8390.ns_nicbase + NS8390_RSR));
#endif
		delayed_outb(ns8390.ns_nicbase + NS8390_ISR, NS8390_RXE);
	}
	if (status & NS8390_OVW) {
		/*
		 * Received buffer overwritten.
		 */
#if	defined(NETDEBUG) && defined(NS8390DEBUG)
		NPRINT(("Start ns8390_overwrite(status = 0x%x)\n",
			       status));
#endif
		delayed_outb(ns8390.ns_nicbase + NS8390_ISR, NS8390_OVW);
		ns8390_overwrite(addr, len);
	}
	if (status & NS8390_PRX) {
		/*
		 * Packet has been received.
		 */
#if	defined(NETDEBUG) && defined(NS8390DEBUG)
		NPRINT(("Start ns8390_rcv(status = 0x%x)\n", status));
#endif
		if (ns8390_rcv(addr, len))
			delayed_outb(ns8390.ns_nicbase + NS8390_ISR, NS8390_PRX);
	}
}

int
ns8390_output(void *addr,
	      unsigned len)
{
	volatile u8bits *ptr;
	u8bits status;
	unsigned i;

#if	defined(NETDEBUG) && defined(NS8390DEBUG)
	NPRINT(("Start ns8390_output(0x%x, 0x%x)\n", addr, len));
#endif
	ptr =  (u8bits *)ns8390.ns_rambase + ((ns8390.ns_txt << 8) & 0xFFFF);

	if (ns8390.ns_flags & NS8390_FLAGS_SLOT_16BITS) {

#if	defined(NETDEBUG) && defined(NS8390DEBUG)
		NPRINT(("pcpy16 (0x%x, 0x%x, 0x%x)\n", addr, ptr, len));
#endif
		if (ns8390.ns_en16bits)
			(*ns8390.ns_en16bits)();
		pcpy16_out(addr, (u8bits *)ptr, len);
		if (ns8390.ns_dis16bits)
			(*ns8390.ns_dis16bits)();
	} else {
#if	defined(NETDEBUG) && defined(NS8390DEBUG)
		NPRINT(("pcpy (0x%x, 0x%x, 0x%x)\n", addr, ptr, len));
#endif
		pcpy_out(addr, (u8bits *)ptr, len);
	}
	if (len < NS8390_MIN_LENGTH)
		len = NS8390_MIN_LENGTH;

#if	defined(NETDEBUG) && defined(NS8390DEBUG)
	NPRINT(("Start packet transmission @x%x\n", ns8390.ns_nicbase));
#endif
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, NS8390_RD4|NS8390_STA);
	delayed_outb(ns8390.ns_nicbase + NS8390_TPSR, ns8390.ns_txt);
	delayed_outb(ns8390.ns_nicbase + NS8390_TBCR0, len & 0xFF);
	delayed_outb(ns8390.ns_nicbase + NS8390_TBCR1, len >> 8);
	delayed_outb(ns8390.ns_nicbase + NS8390_CR, NS8390_TXP|NS8390_RD4|NS8390_STA);

	for (;;) {
		status = inb(ns8390.ns_nicbase + NS8390_ISR);
		if (status & (NS8390_PTX|NS8390_TXE)) {
#if	defined(NETDEBUG) && defined(NS8390DEBUG)
			NPRINT(("Sent ns8390_output(status = %x)\n",
				       status));
#endif
			delayed_outb(ns8390.ns_nicbase + NS8390_ISR,
			     NS8390_PTX|NS8390_TXE);
			break;
		}
	}
#if	defined(NETDEBUG) && defined(NS8390DEBUG)
	if (netdebug) {
		dump_packet("packet dump", addr, 128);
	}
#endif
	return 0;
}

#endif
