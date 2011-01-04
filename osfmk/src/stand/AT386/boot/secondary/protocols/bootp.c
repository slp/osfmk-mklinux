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
 * File : bootp.c
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains BOOTP functions used for Network bootstrap.
 */

#include <secondary/net.h>
#include <secondary/net/ns8390.h>
#include <secondary/net/3c503.h>
#include <secondary/net/dlink.h>
#include <secondary/net/endian.h>
#include <secondary/protocols/udpip.h>
#include <secondary/protocols/arp.h>
#include <secondary/protocols/bootp.h>

static u32bits bootp_xid;
static char bootp_file[128];
static char bootp_vend[MAX_VENDOR];
static u32bits bootp_giaddr;
static u8bits bootp_hops;
static u32bits bootp_waiting;

char *default_net_rootpath = (char *)0;

void
bootp_init()
{
	bootp_waiting = 0;
}

static int
bootp_output(char *file)
{
	struct frame_bootp *bp;
	struct udpip_output udpip;
	unsigned i;

	PPRINT(("Start bootp_output('%s')\n", file));

	bp = (struct frame_bootp *)&udpip_buffer[dlink.dl_hlen +
						 sizeof (struct frame_ip) +
						 sizeof (struct frame_udp)];

	bp->bootp_op = BOOTP_OP_REQUEST;
	bp->bootp_htype = dlink.dl_type;
	bp->bootp_hlen = dlink.dl_len;
	bp->bootp_hops = 0;
	bp->bootp_xid = htonl(bootp_xid);
	bp->bootp_secs = 0;
	bp->bootp_filler = 0;
	bp->bootp_ciaddr = 0;
	bp->bootp_yiaddr = 0;
	bp->bootp_siaddr = 0;
	bp->bootp_giaddr = 0;
	for (i = 0; i < dlink.dl_len; i++)
		bp->bootp_chaddr[i] = dlink.dl_laddr[i];
	for (i = 0; i < 64; i++)
		bp->bootp_sname[i] = '\0';
	for (i = 0; i < 128 && file[i] != '\0'; i++)
		bp->bootp_file[i] = file[i];
	while (i < 128)
		bp->bootp_file[i++] = '\0';
	bp->bootp_cookie = htonl(BOOTP_COOKIE);
	bp->bootp_vend[0] = BOOTP_TAG_END;
	for (i = 1; i < MAX_VENDOR; i++)
		bp->bootp_vend[i] = '\0';

	udpip.udpip_sport = IPPORT_BOOTPC;
	udpip.udpip_dport = IPPORT_BOOTPS;
	udpip.udpip_src = 0;
	udpip.udpip_dst = 0xFFFFFFFF;
	udpip.udpip_len = sizeof (struct frame_bootp);
	udpip.udpip_buffer = udpip_buffer;

	return udpip_output_with_protocol(&udpip,IPPROTO_UDP);
}

int
bootp_input(struct udpip_input *udpip_input)
{
	struct frame_bootp *bp;
	unsigned i;

	PPRINT(("Start bootp_input(0x%x)\n", udpip_input));

	if (bootp_waiting == 0) {
		PPRINT(("bootp_input: unexpected packet\n"));
		return (0);
	}
	bp = (struct frame_bootp *)udpip_input->udpip_addr;
	if (bp->bootp_op != BOOTP_OP_REPLY ||
	    bp->bootp_htype != dlink.dl_type ||
	    bp->bootp_hlen != dlink.dl_len) {
		PPRINT(("bootp_input: Bad operation\n"));
		return (0);
	}
	bp->bootp_xid = ntohl(bp->bootp_xid);
	if (bp->bootp_xid != bootp_xid)
		return (0);
	bp->bootp_ciaddr = ntohl(bp->bootp_ciaddr);
	bp->bootp_yiaddr = ntohl(bp->bootp_yiaddr);
	if (bp->bootp_ciaddr && (bp->bootp_ciaddr != bp->bootp_yiaddr ||
				 udpip_laddr != bp->bootp_yiaddr)) {
		PPRINT(("bootp_input: Bad address\n"));
		return (0);
	}

	for (i = 0; i < 128; i++)
		bootp_file[i] = bp->bootp_file[i];
	for (i = 0; i < MAX_VENDOR; i++)
		bootp_vend[i] =  bp->bootp_vend[i];
	if (udpip_laddr == 0)
		udpip_laddr = bp->bootp_yiaddr;
	udpip_raddr = ntohl(bp->bootp_siaddr);
	if ((bootp_hops = bp->bootp_hops) > 0 && bp->bootp_giaddr != 0)
		bootp_giaddr = ntohl(bp->bootp_giaddr);
	else
		bootp_giaddr = udpip_raddr;
	bootp_waiting = 0;
	return (1);
}

static int
bootp_vendor(unsigned char *bv)
{
	unsigned char	vtag;
	unsigned int 	cnt;
	unsigned char 	buffer[MAX_VENDOR];
	unsigned char   *start_bv = bv;

	PPRINT(("Start bootp_vendor(0x%x)\n", bv));

	for (;;) {
		if (bv-start_bv >= MAX_VENDOR - 1) {
			printf("bootp vendor zone ended.\n");
			break;
		}

		vtag = *bv++;

		switch(vtag) {
		    case BOOTP_TAG_PAD:
			continue;
		    case BOOTP_TAG_END:
			return (0);
		    default:
			cnt = *bv++;
			break;
		}

		switch (vtag) {
		    case BOOTP_TAG_SUBNET: 
		    case BOOTP_TAG_GATEWAY:
		    case BOOTP_TAG_TIME:
		    case BOOTP_TAG_DOMAIN:
/* #define	FULL_SIZE_NETBOOT */
#ifdef	FULL_SIZE_NETBOOT
			{
			u32bits nm = ntohl(*((u32bits *)bv));
			char *tagname;
			switch(vtag) {
			    case BOOTP_TAG_SUBNET:
				tagname="subnet";break;
			    case BOOTP_TAG_GATEWAY:
				tagname="gateway";break;
			    case BOOTP_TAG_TIME:
				tagname="time server";break;
			    case BOOTP_TAG_DOMAIN:
				tagname="domain";break;
			    default:
				tagname="unknown tag";break;
			    };
			printf("%s: %d.%d.%d.%d\n",
			       tagname,
			       (nm >> 24) & 0xFF,
			       (nm >> 16) & 0xFF,
			       (nm >> 8) & 0xFF,
			       nm & 0xFF);
			break;
		    }
#else
			break;
#endif
		    case BOOTP_TAG_TIMEOFFSET:
		    case BOOTP_TAG_FILESIZE:
#ifdef	FULL_SIZE_NETBOOT
			{
			u32bits nm = ntohl(*((u32bits *)bv));
			char *tagname;
			switch(vtag) {
			    case BOOTP_TAG_TIMEOFFSET:
				tagname="time offset";break;
			    case BOOTP_TAG_FILESIZE:
				tagname="file size";break;
/* XXX buggy file size is received: bootpd problem? */
			    default:
				tagname="unknown tag";break;
			    };

			printf("%s: %d\n", tagname,  nm);
			break;
		    }
#else
			break;
#endif
		    case BOOTP_TAG_ROOTPATH:
			if (default_net_rootpath != (char *)0) {
				PPRINT(("Replacing rootpath %s\n",default_net_rootpath));
				free(default_net_rootpath);
				default_net_rootpath = (char *)0;
			}
			{
				char *c, *t = bv;
				register int i;

				default_net_rootpath = (char *)malloc(cnt+1);
				c = default_net_rootpath;
				for(i=0; i<cnt; i++)
					*c++=*t++;
				default_net_rootpath[cnt]='\0';
				PPRINT(("root path: %s\n", default_net_rootpath));
			}
			break;

		    case BOOTP_TAG_EXTENSIONS:
			bcopy(bv, buffer, cnt);
			buffer[cnt] = '\0';
			printf("%s from %d.%d.%d.%d",
			       "Bootp extension downloaded from",
			       (udpip_raddr >> 24) & 0xFF,
			       (udpip_raddr >> 16) & 0xFF,
			       (udpip_raddr >> 8) & 0xFF,
			       udpip_raddr & 0xFF);
			if (bootp_hops > 0 && bootp_giaddr != 0)
				printf(" via %d.%d.%d.%d", 
				       (bootp_giaddr >> 24) & 0xFF,
				       (bootp_giaddr >> 16) & 0xFF,
				       (bootp_giaddr >> 8) & 0xFF,
				       bootp_giaddr & 0xFF);
			putchar('\n');
#if later
			switch (tftp_main(buffer, load, bootp_copy)) {
			    case 0:
				bootp_vendor(load, (void *)0);
				break;
			    case 1:
				printf("Extension file not found\n");
				return (1);
			    case 2:
				return (2);
			}
#endif
			break;
		    case BOOTP_TAG_DOMAINNAME:
		    case BOOTP_TAG_HOSTNAME:
		    case BOOTP_TAG_NODE_NUMBER:
		    case BOOTP_TAG_CLUSTER_NAME:
		    case BOOTP_TAG_CLUSTER_ID:
#ifdef	FULL_SIZE_NETBOOT
			{
			    char rootpath[128];
			    char *c, *t = bv, *tagname;
			    register int i;

			    switch(vtag) {
				case BOOTP_TAG_HOSTNAME:
				    tagname="hostname";break;
				case BOOTP_TAG_DOMAINNAME:
				    tagname="domain name";break;
				case BOOTP_TAG_NODE_NUMBER:
				    tagname="cluster node number";break;
				case BOOTP_TAG_CLUSTER_NAME:
				    tagname="cluster name";break;
				case BOOTP_TAG_CLUSTER_ID:
				    tagname="cluster id";break;
				default:
				    tagname="unknown tag";break;
			    };
			    c = rootpath;
			    for(i=0; i<cnt; i++)
				    *c++=*t++;
			    rootpath[cnt]='\0';
			    printf("%s: %s\n",
				   tagname, rootpath);
			    break;
		    }
#else
			break;
#endif
		    default:
		    {
			int i;
			printf("Unknown bootp tag %d (%d): ", vtag, cnt);
			for(i=0; i<cnt; i++)
				printf("%x ",*(bv+i));
			putchar('\n');
			break;
		    }
		}
		bv += cnt;
	}
	return (NETBOOT_SUCCESS);
}

int
bootp_request(/* in */ 	  char *file,
	      /* inout */ char *rootdevice,	/* may be (char *)0 */
	      /* inout */ char *rootpath)	/* may be (char *)0 */
{
	int ret;
	unsigned waiting;
	unsigned bootp_retransmit;

	PPRINT(("Start bootp_request(0x%x, 0x%x, x%x)\n",
		       file, rootdevice, rootpath));

	if (strlen(file) >= 128) {
		printf("Boot file name too long\n");
		return (NETBOOT_ERROR);
	}

	PPRINT(("Send bootp request for '%s'\n", *file ? file : BOOTP_DEFAULT));

	bootp_waiting = 1;
	if(bootp_output(file)) {
		printf("Error on bootp_output\n");
		return 1;
	}
	settimeout(BOOTP_RETRANSMIT);
	waiting = BOOTP_RETRANSMIT;
	bootp_retransmit = BOOTP_RETRANSMIT << 1;
	while (bootp_waiting) {
		if (isatimeout()) {
			if ((waiting += bootp_retransmit) > 60000) {
				printf("\nbootp: time-out\n");
				return (NETBOOT_ERROR);
			}
			putchar('.');
			(void)bootp_output(file);
			settimeout(bootp_retransmit);
			bootp_retransmit <<= 1;
		}
		if (is_intr_char()) {
			putchar('\n');
			return (NETBOOT_ABORT);
		}
		(*dlink.dl_input)(udpip_buffer, IP_MSS);
	}

	/* XXX do we have enough memory for 'file'
	 * strcpy(file, bootp_file);
	 */
	strcpy(bootp_file, file);
	printf("%s opened on %d.%d.%d.%d",
	       bootp_file,
	       (udpip_raddr >> 24) & 0xFF, (udpip_raddr >> 16) & 0xFF,
	       (udpip_raddr >> 8) & 0xFF, udpip_raddr & 0xFF);
	if (bootp_hops > 0 && bootp_giaddr != 0)
		printf(" via %d.%d.%d.%d", 
		       (bootp_giaddr >> 24) & 0xFF, (bootp_giaddr >> 16) & 0xFF,
		       (bootp_giaddr >> 8) & 0xFF, bootp_giaddr & 0xFF);
	putchar('\n');

	switch (arp_request(bootp_giaddr)) {
	    case NETBOOT_SUCCESS:
		break;
	    case NETBOOT_ERROR:
		printf("\nNo ARP reply for %d.%d.%d.%d\n",
		       (bootp_giaddr >> 24) & 0xFF, (bootp_giaddr >> 16) & 0xFF,
		       (bootp_giaddr >> 8) & 0xFF, bootp_giaddr & 0xFF);
		return (NETBOOT_ERROR);
	    case NETBOOT_ABORT:
		return (NETBOOT_ABORT);
	}

	if (ret = bootp_vendor(bootp_vend))
		return (ret);

PDWAIT;
	return (NETBOOT_SUCCESS);
}

