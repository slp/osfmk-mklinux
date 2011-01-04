/*
 *		IP_MASQ_VDOLIVE  - VDO Live masquerading module
 *
 *
 * Version:	@(#)$Id: ip_masq_vdolive.c,v 1.1.2.1 1997/03/04 12:04:47 davem Exp $
 *
 * Author:	Nigel Metheringham <Nigel.Metheringham@ThePLAnet.net>
 *		PLAnet Online Ltd
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 * Thanks:
 *	Thank you to VDOnet Corporation for allowing me access to
 *	a protocol description without an NDA.  This means that
 *	this module can be distributed as source - a great help!
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

#ifndef DEBUG_CONFIG_IP_MASQ_VDOLIVE
#define DEBUG_CONFIG_IP_MASQ_VDOLIVE 0
#endif

struct vdolive_priv_data {
	/* Ports used */
	unsigned short	origport;
	unsigned short	masqport;
	/* State of decode */
	unsigned short	state;
};

/* 
 * List of ports (up to MAX_MASQ_APP_PORTS) to be handled by helper
 * First port is set to the default port.
 */
int ports[MAX_MASQ_APP_PORTS] = {7000}; /* I rely on the trailing items being set to zero */
struct ip_masq_app *masq_incarnations[MAX_MASQ_APP_PORTS];

static int
masq_vdolive_init_1 (struct ip_masq_app *mapp, struct ip_masq *ms)
{
        MOD_INC_USE_COUNT;
	if ((ms->app_data = kmalloc(sizeof(struct vdolive_priv_data),
				    GFP_ATOMIC)) == NULL) 
		printk(KERN_INFO "VDOlive: No memory for application data\n");
	else 
	{
		struct vdolive_priv_data *priv = 
			(struct vdolive_priv_data *)ms->app_data;
		priv->origport = 0;
		priv->masqport = 0;
		priv->state = 0;
	}
        return 0;
}

static int
masq_vdolive_done_1 (struct ip_masq_app *mapp, struct ip_masq *ms)
{
        MOD_DEC_USE_COUNT;
	if (ms->app_data)
		kfree_s(ms->app_data, sizeof(struct vdolive_priv_data));
        return 0;
}

int
masq_vdolive_out (struct ip_masq_app *mapp, struct ip_masq *ms, struct sk_buff **skb_p, struct device *dev)
{
        struct sk_buff *skb;
	struct iphdr *iph;
	struct tcphdr *th;
	char *data, *data_limit;
	unsigned int tagval;	/* This should be a 32 bit quantity */
	struct ip_masq *n_ms;
	struct vdolive_priv_data *priv = 
		(struct vdolive_priv_data *)ms->app_data;

	/* This doesn't work at all if no priv data was allocated on startup */
	if (!priv)
		return 0;

	/* Everything running correctly already */
	if (priv->state == 3)
		return 0;

        skb = *skb_p;
	iph = skb->h.iph;
        th = (struct tcphdr *)&(((char *)iph)[iph->ihl*4]);
        data = (char *)&th[1];

        data_limit = skb->h.raw + skb->len;

	if (data+8 > data_limit) {
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
		printk("VDOlive: packet too short for ID %lx %lx\n",
		       data, data_limit);
#endif
		return 0;
	}
	memcpy(&tagval, data+4, 4);
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
	printk("VDOlive: packet seen, tag %ld, in initial state %d\n",
	       ntohl(tagval), priv->state);
#endif

	/* Check for leading packet ID */
	if ((ntohl(tagval) != 6) && (ntohl(tagval) != 1)) {
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
		printk("VDOlive: unrecognised tag %ld, in initial state %d\n",
		       ntohl(tagval), priv->state);
#endif
		return 0;
	}
		

	/* Check packet is long enough for data - ignore if not */
	if ((ntohl(tagval) == 6) && (data+36 > data_limit)) {
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
		printk("VDOlive: initial packet too short %lx %lx\n",
		       data, data_limit);
#endif
		return 0;
	} else if ((ntohl(tagval) == 1) && (data+20 > data_limit)) {
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
		printk("VDOlive: secondary packet too short %lx %lx\n",
		       data, data_limit);
#endif
		return 0;
	}

	/* Adjust data pointers */
	/*
	 * I could check the complete protocol version tag 
	 * in here however I am just going to look for the
	 * "VDO Live" tag in the hope that this part will
	 * remain constant even if the version changes
	 */
	if (ntohl(tagval) == 6) {
		data += 24;
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
		printk("VDOlive: initial packet found\n");
#endif
	} else {
		data += 8;
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
		printk("VDOlive: secondary packet found\n");
#endif
	}

	if (memcmp(data, "VDO Live", 8) != 0) {
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
		printk("VDOlive: did not find tag\n");
#endif
		return 0;
	}
	/* 
	 * The port number is the next word after the tag.
	 * VDOlive encodes all of these values
	 * in 32 bit words, so in this case I am
	 * skipping the first 2 bytes of the next
	 * word to get to the relevant 16 bits
	 */
	data += 10;

	/*
	 * If we have not seen the port already,
	 * set the masquerading tunnel up
	 */
	if (!priv->origport) {
		memcpy(&priv->origport, data, 2);

#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
		printk("VDOlive: found port %d\n",
		       ntohs(priv->origport));
#endif

		/* Open up a tunnel */
		n_ms = ip_masq_new(dev, IPPROTO_UDP,
				   ms->saddr, priv->origport,
				   ms->daddr, 0,
				   IP_MASQ_F_NO_DPORT);
					
		if (n_ms==NULL) {
			printk("VDOlive: unable to build UDP tunnel for %x:%x\n",
			       ms->saddr, priv->origport);
			/* Leave state as unset */
			priv->origport = 0;
			return 0;
		}

		ip_masq_set_expire(n_ms, ip_masq_expire->udp_timeout);
		priv->masqport = n_ms->mport;
	} else if (memcmp(data, &(priv->origport), 2)) {
		printk("VDOlive: ports do not match\n");
		/* Write the port in anyhow!!! */
	}

	/*
	 * Write masq port into packet
	 */
	memcpy(data, &(priv->masqport), 2);
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
	printk("VDOlive: rewrote port %d to %d, server %s\n",
	       ntohs(priv->origport), ntohs(priv->masqport), in_ntoa(ms->saddr));
#endif

	/*
	 * Set state bit to make which bit has been done
	 */

	priv->state |= (ntohl(tagval) == 6) ? 1 : 2;

	return 0;
}


struct ip_masq_app ip_masq_vdolive = {
        NULL,			/* next */
	"VDOlive",	       	/* name */
        0,                      /* type */
        0,                      /* n_attach */
        masq_vdolive_init_1,	/* ip_masq_init_1 */
        masq_vdolive_done_1,	/* ip_masq_done_1 */
        masq_vdolive_out,	/* pkt_out */
        NULL                    /* pkt_in */
};

/*
 * 	ip_masq_vdolive initialization
 */

int ip_masq_vdolive_init(void)
{
	int i, j;

	for (i=0; (i<MAX_MASQ_APP_PORTS); i++) {
		if (ports[i]) {
			if ((masq_incarnations[i] = kmalloc(sizeof(struct ip_masq_app),
							    GFP_KERNEL)) == NULL)
				return -ENOMEM;
			memcpy(masq_incarnations[i], &ip_masq_vdolive, sizeof(struct ip_masq_app));
			if ((j = register_ip_masq_app(masq_incarnations[i], 
						      IPPROTO_TCP, 
						      ports[i]))) {
				return j;
			}
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
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
 * 	ip_masq_vdolive fin.
 */

int ip_masq_vdolive_done(void)
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
#if DEBUG_CONFIG_IP_MASQ_VDOLIVE
				printk("VDOlive: unloaded support on port[%d] = %d\n",
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
        if (ip_masq_vdolive_init() != 0)
                return -EIO;
        register_symtab(0);
        return 0;
}

void cleanup_module(void)
{
        if (ip_masq_vdolive_done() != 0)
                printk("ip_masq_vdolive: can't remove module");
}

#endif /* MODULE */
