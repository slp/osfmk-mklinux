/*
 *		IP_MASQ_RAUDIO  - Real Audio masquerading module
 *
 *
 * Version:	@(#)$Id: ip_masq_raudio.c,v 1.1.2.1 1997/03/04 12:04:53 davem Exp $
 *
 * Author:	Nigel Metheringham
 *		[strongly based on ftp module by Juan Jose Ciarlante & Wouter Gadeyne]
 *		[Real Audio information taken from Progressive Networks firewall docs]
 *		[Kudos to Progressive Networks for making the protocol specs available]
 *
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *
 * Limitations
 *	The IP Masquerading proxies at present do not have access to a processed
 *	data stream.  Hence for a protocol like the Real Audio control protocol,
 *	which depends on knowing where you are in the data stream, you either
 *	to keep a *lot* of state in your proxy, or you cheat and simplify the
 *	problem [needless to say I did the latter].
 *
 *	This proxy only handles data in the first packet.  Everything else is
 *	passed transparently.  This means it should work under all normal
 *	circumstances, but it could be fooled by new data formats or a
 *	malicious application!
 *
 *	At present the "first packet" is defined as a packet starting with
 *	the protocol ID string - "PNA".
 *	When the link is up there appears to be enough control data 
 *	crossing the control link to keep it open even if a long audio
 *	piece is playing.
 *
 *	The Robust UDP support added in RealAudio 3.0 is supported, but due
 *	to servers/clients not making great use of this has not been greatly
 *	tested.  RealVideo (as used in the Real client version 4.0beta1) is
 *	supported but again is not greatly tested (bandwidth requirements
 *	appear to exceed that available at the sites supporting the protocol).
 *
 * Multiple Port Support
 *	The helper can be made to handle up to MAX_MASQ_APP_PORTS (normally 12)
 *	with the port numbers being defined at module load time.  The module
 *	uses the symbol "ports" to define a list of monitored ports, which can
 *	be specified on the insmod command line as
 *		ports=x1,x2,x3...
 *	where x[n] are integer port numbers.  This option can be put into
 *	/etc/conf.modules (or /etc/modules.conf depending on your config)
 *	where modload will pick it up should you use modload to load your
 *	modules.
 *	
 */

#include <linux/module.h>
#include <asm/system.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <net/protocol.h>
#include <net/tcp.h>
#include <net/ip_masq.h>

#ifndef DEBUG_CONFIG_IP_MASQ_RAUDIO
#define DEBUG_CONFIG_IP_MASQ_RAUDIO 0
#endif

struct raudio_priv_data {
	/* Associated data connection - setup but not used at present */
	struct	ip_masq *data_conn;
	/* UDP Error correction connection - setup but not used at present */
	struct	ip_masq *error_conn;
	/* Have we seen and performed setup */
	short	seen_start;
};

/* 
 * List of ports (up to MAX_MASQ_APP_PORTS) to be handled by helper
 * First port is set to the default port.
 */
int ports[MAX_MASQ_APP_PORTS] = {7070}; /* I rely on the trailing items being set to zero */
struct ip_masq_app *masq_incarnations[MAX_MASQ_APP_PORTS];

static int
masq_raudio_init_1 (struct ip_masq_app *mapp, struct ip_masq *ms)
{
        MOD_INC_USE_COUNT;
	if ((ms->app_data = kmalloc(sizeof(struct raudio_priv_data),
				    GFP_ATOMIC)) == NULL) 
		printk(KERN_INFO "RealAudio: No memory for application data\n");
	else 
	{
		struct raudio_priv_data *priv = 
			(struct raudio_priv_data *)ms->app_data;
		priv->seen_start = 0;
		priv->data_conn = NULL;
		priv->error_conn = NULL;
	}
        return 0;
}

static int
masq_raudio_done_1 (struct ip_masq_app *mapp, struct ip_masq *ms)
{
        MOD_DEC_USE_COUNT;
	if (ms->app_data)
		kfree_s(ms->app_data, sizeof(struct raudio_priv_data));
        return 0;
}

int
masq_raudio_out (struct ip_masq_app *mapp, struct ip_masq *ms, struct sk_buff **skb_p, struct device *dev)
{
        struct sk_buff *skb;
	struct iphdr *iph;
	struct tcphdr *th;
	char *p, *data, *data_limit;
	struct ip_masq *n_ms;
	unsigned short version, msg_id, msg_len, udp_port;
	struct raudio_priv_data *priv = 
		(struct raudio_priv_data *)ms->app_data;

	/* Everything running correctly already */
	if (priv && priv->seen_start)
		return 0;

        skb = *skb_p;
	iph = skb->h.iph;
        th = (struct tcphdr *)&(((char *)iph)[iph->ihl*4]);
        data = (char *)&th[1];

        data_limit = skb->h.raw + skb->len;

	/* Check to see if this is the first packet with protocol ID */
	if (memcmp(data, "PNA", 3)) {
#if DEBUG_CONFIG_IP_MASQ_RAUDIO
		printk("RealAudio: not initial protocol packet - ignored\n");
#endif
		return(0);
	}
	data += 3;
	memcpy(&version, data, 2);

#if DEBUG_CONFIG_IP_MASQ_RAUDIO
	printk("RealAudio: initial seen - protocol version %d\n",
	       ntohs(version));
#endif
	if (priv)
		priv->seen_start = 1;

	if (ntohs(version) >= 256)
	{
		printk(KERN_INFO "RealAudio: version (%d) not supported\n",
		       ntohs(version));
		return 0;
	}

	data += 2;
	while (data+4 < data_limit) {
		memcpy(&msg_id, data, 2);
		data += 2;
		memcpy(&msg_len, data, 2);
		data += 2;
		if (ntohs(msg_id) == 0) {
			/* The zero tag indicates the end of options */
#if DEBUG_CONFIG_IP_MASQ_RAUDIO
			printk("RealAudio: packet end tag seen\n");
#endif
			return 0;
		}
#if DEBUG_CONFIG_IP_MASQ_RAUDIO
		printk("RealAudio: msg %d - %d byte\n",
		       ntohs(msg_id), ntohs(msg_len));
#endif
		if (ntohs(msg_id) == 0) {
			/* The zero tag indicates the end of options */
			return 0;
		}
		p = data;
		data += ntohs(msg_len);
		if (data > data_limit)
		{
			printk(KERN_INFO "RealAudio: Packet too short for data\n");
			return 0;
		}
		if ((ntohs(msg_id) == 1) || (ntohs(msg_id) == 7)) {
			/* 
			 * MsgId == 1
			 * Audio UDP data port on client
			 *
			 * MsgId == 7
			 * Robust UDP error correction port number on client
			 *
			 * Since these messages are treated just the same, they
			 * are bundled together here....
			 */
			memcpy(&udp_port, p, 2);

			/* 
			 * Sometimes a server sends a message 7 with a zero UDP port
			 * Rather than do anything with this, just ignore it!
			 */
			if (udp_port == 0)
				continue;

			n_ms = ip_masq_new(dev, IPPROTO_UDP,
					   ms->saddr, udp_port,
					   ms->daddr, 0,
					   IP_MASQ_F_NO_DPORT);
					
			if (n_ms==NULL)
				return 0;

			memcpy(p, &(n_ms->mport), 2);
#if DEBUG_CONFIG_IP_MASQ_RAUDIO
			printk("RealAudio: rewrote UDP port %d -> %d in msg %d\n",
			       ntohs(udp_port), ntohs(n_ms->mport), ntohs(msg_id));
#endif
			ip_masq_set_expire(n_ms, ip_masq_expire->udp_timeout);

			/* Make ref in application data to data connection */
			if (priv) {
				if (ntohs(msg_id) == 1)
					priv->data_conn = n_ms;
				else
					priv->error_conn = n_ms;
			}
		}
	}
	return 0;
}

struct ip_masq_app ip_masq_raudio = {
        NULL,			/* next */
	"RealAudio",	       	/* name */
        0,                      /* type */
        0,                      /* n_attach */
        masq_raudio_init_1,     /* ip_masq_init_1 */
        masq_raudio_done_1,     /* ip_masq_done_1 */
        masq_raudio_out,        /* pkt_out */
        NULL                    /* pkt_in */
};

/*
 * 	ip_masq_raudio initialization
 */

int ip_masq_raudio_init(void)
{
	int i, j;

	for (i=0; (i<MAX_MASQ_APP_PORTS); i++) {
		if (ports[i]) {
			if ((masq_incarnations[i] = kmalloc(sizeof(struct ip_masq_app),
							    GFP_KERNEL)) == NULL)
				return -ENOMEM;
			memcpy(masq_incarnations[i], &ip_masq_raudio, sizeof(struct ip_masq_app));
			if ((j = register_ip_masq_app(masq_incarnations[i], 
						      IPPROTO_TCP, 
						      ports[i]))) {
				return j;
			}
#if DEBUG_CONFIG_IP_MASQ_RAUDIO
			printk("RealAudio: loaded support on port[%d] = %d\n",
			       i, ports[i]);
#endif
		} else {
			/* To be safe, force the incarnation table entry to NULL */
			masq_incarnations[i] = NULL;
		}
	}
	return 0;
}

/*
 * 	ip_masq_raudio fin.
 */

int ip_masq_raudio_done(void)
{
	int i, j, k;

	k=0;
	for (i=0; (i<MAX_MASQ_APP_PORTS); i++) {
		if (masq_incarnations[i]) {
			if ((j = unregister_ip_masq_app(masq_incarnations[i]))) {
				k = j;
			} else {
				kfree(masq_incarnations[i]);
				masq_incarnations[i] = NULL;
#if DEBUG_CONFIG_IP_MASQ_RAUDIO
				printk("RealAudio: unloaded support on port[%d] = %d\n",
				       i, ports[i]);
#endif
			}
		}
	}
	return k;
}

#ifdef MODULE

int init_module(void)
{
        if (ip_masq_raudio_init() != 0)
                return -EIO;
        register_symtab(0);
        return 0;
}

void cleanup_module(void)
{
        if (ip_masq_raudio_done() != 0)
                printk("ip_masq_raudio: can't remove module");
}

#endif /* MODULE */
