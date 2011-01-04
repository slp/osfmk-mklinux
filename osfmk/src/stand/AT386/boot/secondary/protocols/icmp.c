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
 * File : icmp.c
 *
 * Author : Yves Paindaveine
 *
 * This file contains ICMProtocols used for Network bootstrap.
 */

#include <secondary/net.h>
#include <secondary/net/ns8390.h>
#include <secondary/net/3c503.h>
#include <secondary/net/dlink.h>
#include <secondary/net/endian.h>
#include <secondary/protocols/udpip.h>
#include <secondary/protocols/arp.h>
#include <secondary/protocols/icmp.h>

void
icmp_init()
{
	/* PLACEHOLDER */
}

static int
icmp_output(void)
{
	/* PLACEHOLDER */
	return 0;
}

int
icmp_input(struct udpip_input *udpip_input)
{
	extern	int	port_unreachable;
	struct frame_icmp *icp;

	PPRINT(("Start icmp_input(0x%x)\n", udpip_input));

	icp = (struct frame_icmp *)udpip_input->udpip_addr;
	switch(icp->icmp_type) {
	    case ICMP_TYPE_ECHO_REQUEST:
		icp->icmp_type = ICMP_TYPE_ECHO_REPLY;
		PPRINT(("ICMP ECHO\n"));
		(void)udpip_output_with_protocol((struct udpip_output *)udpip_input, IPPROTO_ICMP);
		break;
	    case ICMP_TYPE_DST_UNREACHABLE:
		/*
		 * When a tftp sequence is being aborted, a unreachable port
		 * means that tftpd has received the abort notification.
		 */
		if (icp->icmp_code == ICMP_CODE_PORT_UNREACHABLE) {
			PPRINT(("ICMP: port unreachable\n"));
			port_unreachable = 1;
		}
		else {
			PPRINT(("ICMP: dest unreachable (%d)\n",icp->icmp_code));
		}
		break;
	    default:
		printf("Unknown ICMP type %d\n",icp->icmp_type);
#ifdef	PROTODEBUG
		dump_packet("ICMP",icp,udpip_input->udpip_len);
#endif
		break;
	}
	return (1);
}

