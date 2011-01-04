/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <mach/mach_interface.h>
#include <mach/mach_port.h>
#include <mach/mach_traps.h>

#include <osfmach3/device_utils.h>
#include <osfmach3/device_reply_hdlr.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/mach_init.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/uniproc.h>

#include <asm/byteorder.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#if	CONFIG_OSFMACH3_DEBUG
#define ETHER_DEBUG 1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	ETHER_DEBUG
int ether_debug = 0;
#endif	/* ETHER_DEBUG */

int ethernet_synchronous_xmit = 1;

struct ether_used {
	struct sk_buff_head	eu_skb_list;
	mach_port_t		eu_port;
	struct ether_used	*eu_next;
};

#define ETHER_USED_MAX	5	/* maximum number of used structures */
struct ether_used	*ether_used_root;	/* list of free ether_used */
unsigned int		ether_used_cur;		/* number of free ether_used */

struct ed_device {
	mach_port_t		ed_device_port;
	mach_port_t		ed_receive_port;
	struct enet_statistics	ed_stat;
	unsigned int		ed_maxlen; /* max output buffer queue length */
	unsigned int		ed_current;/* current index in the next table */
	struct ether_used	*ed_used;	/* output used buffer queue */
};

kern_return_t
ether_write_reply(
	char		*eu_handle,
	kern_return_t	return_code,
	char		*data,		/* irrelevant */
	unsigned int	data_count)
{
	struct ether_used	*eu;
	struct sk_buff		*skb;

	eu = (struct ether_used *) eu_handle;

	while ((skb = skb_dequeue(&eu->eu_skb_list))) {
		/*
		 * Relock the sk_buff: dev_kfree_skb expects it locked
		 * and we unlocked it in ether_start_xmit().
		 */
		ASSERT(!skb_device_locked(skb));
		skb_device_lock(skb);
		dev_kfree_skb(skb, FREE_WRITE);
	}

	if (ether_used_cur == ETHER_USED_MAX) {
		device_reply_deregister(eu->eu_port);
		kfree(eu);
	} else {
		eu->eu_next = ether_used_root;
		ether_used_root = eu;
		ether_used_cur++;
	}

	return KERN_SUCCESS;
}

void *
ether_input_thread(
	void	*dev_handle)
{
	struct server_thread_priv_data	priv_data;
	struct device			*dev;
	struct ed_device		*ed;
	int				pkt_len;
	kern_return_t			kr;
	net_rcv_msg_t			msg;
	struct sk_buff			*skb;

	cthread_set_name(cthread_self(), "ether input thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	uniproc_enter();

	dev = (struct device *) dev_handle;
	ed = (struct ed_device *) dev->priv;

	msg = (net_rcv_msg_t) kmalloc(8192, GFP_KERNEL);
	if (msg == NULL) {
		panic("ether_input_thread(\"%s\"): can't allocate msg",
		      dev->name);
	}


	for (;;) {
		server_thread_blocking(FALSE);
		kr = mach_msg(&msg->msg_hdr, MACH_RCV_MSG, 0, 8192,
			      ed->ed_receive_port,
			      MACH_MSG_TIMEOUT_NONE,
			      MACH_PORT_NULL);
		server_thread_unblocking(FALSE);
		if (kr != MACH_MSG_SUCCESS) {
			MACH3_DEBUG(1, kr, ("ether_input_thread(\"%s\"): "
					    "mach_msg", dev->name));
			ed->ed_stat.rx_errors++;
			continue;
		}

#ifdef	ETHER_DEBUG
		if (ether_debug) {
			printk("ether_input_thread: packet for \"%s\"\n",
			       dev->name);
		}
#endif	/* ETHER_DEBUG */

		ASSERT(msg->msg_hdr.msgh_id == NET_RCV_MSG_ID);

		pkt_len = msg->net_rcv_msg_packet_count
			- sizeof (struct packet_header);
		skb = dev_alloc_skb(pkt_len + dev->hard_header_len + 2);
		if (skb == NULL) {
			ed->ed_stat.rx_dropped++;
			continue;
		}
		skb_reserve(skb,2);	/* IP headers on 16 byte boundaries */
		skb->dev = dev;
		skb_put(skb, pkt_len + dev->hard_header_len);
		skb->len = pkt_len + dev->hard_header_len;
		/* copy the ethernet header */
		memcpy(skb->data,
		       &msg->header[0],
		       dev->hard_header_len);
		/* copy the packet */
		memcpy(skb->data + dev->hard_header_len,
		       &msg->packet[0] + sizeof (struct packet_header),
		       pkt_len);
		skb->protocol = eth_type_trans(skb,dev);
		netif_rx(skb);
		ed->ed_stat.rx_packets++;
	}

#ifdef	ETHER_DEBUG
	if (ether_debug) {
		printk("ether_input_thread(\"%s\"): terminating\n",
		       dev->name);
	}
#endif	/* ETHER_DEBUG */

	cthread_set_name(cthread_self(), "LATE ether thread");
	uniproc_exit();
	cthread_detach(cthread_self());
	cthread_exit((void *) 0);
	/*NOTREACHED*/

	return (void *) 0;
}

void
ether_set_filter(
	struct device	*dev)
{
	filter_t	filter[NET_MAX_FILTER];
	filter_t	*pf;
	kern_return_t	kr;
	struct ed_device *ed;
	union {
		u_long	l_addr;
		u_short	s_addr[2];
	} ip_addr;

	ed = (struct ed_device *) dev->priv;

	pf = filter;

	switch ((int) dev->family) {
	    case AF_INET:
		if (dev->flags & (IFF_PROMISC | IFF_ALLMULTI)) {
			/* Change filter to return ALL packets */
			/* The empty filter passes everything through */
			break;
		}

#ifdef CONFIG_ATALK
		/*
		 * Michel Pollet: get all packets, we need them for
		 * 802.2, and I don't know how to make a proper filter.
		 * IF you feel like it, have a look at net/inet/eth.c
		 * eth_type_trans() for a working method.
		 */
		/* The empty filter passes everything through */
		break;
#endif
		/*
		 * First, deal with ARP packets.
		 */
		*pf++ = NETF_PUSHHDR+6;		/* PUSH Ethernet Type */
		*pf++ = NETF_PUSHLIT|NETF_COR;	/* PUSH ... */
		*pf++ = htons(ETH_P_ARP);	/* ... ARP type */
						/* Return true if == */
		/*
		 * Deal now with IP packets.
		 */
		*pf++ = NETF_PUSHHDR+6;		/* PUSH Ethernet Type */
		*pf++ = NETF_PUSHLIT|NETF_CAND;	/* PUSH ... */
		*pf++ = htons(ETH_P_IP);	/* ... IP type */
						/* Return false if != */
		/*
		 *  Deal now with Unknown IP address
		 */
		*pf++ = NETF_PUSHWORD+8;	/* PUSH IP dst addr 1st word */
		*pf++ = NETF_PUSHZERO|NETF_EQ;	/* PUSH 0 */
						/* IP dst addr 1st word == */
		*pf++ = NETF_PUSHWORD+9;	/* PUSH IP dst addr 2nd word */
		*pf++ = NETF_PUSHZERO|NETF_EQ;	/* PUSH 0 */
						/* IP dst addr 2nd word == */
		*pf++ = NETF_AND;		/* IP dst addr == 0 */
		*pf++ = NETF_PUSHZERO|NETF_CNAND;/* PUSH 0 */
						/* Return true if != */
		/*
		 *  Deal now with Broadcast IP address
		 */
		*pf++ = NETF_PUSHWORD+8;	/* PUSH IP dst addr 1st word */
		*pf++ = NETF_PUSHLIT|NETF_EQ;	/* PUSH ... */
		*pf++ = 0xFFFF;			/* ... 0xFFFF */
						/* IP dst addr 1st word == */
		*pf++ = NETF_PUSHWORD+9;	/* PUSH IP dst addr 2nd word */
		*pf++ = NETF_PUSHLIT|NETF_EQ;	/* PUSH ... */
		*pf++ = 0xFFFF;			/* ... 0xFFFF */
						/* IP dst addr 2nd word == */
		*pf++ = NETF_AND;		/* IP dst addr == 0xFFFFFFFF */
		*pf++ = NETF_PUSHZERO|NETF_CNAND;/* PUSH 0 */
						/* Return true if != */
		/*
		 *  Deal now with Interface Broadcast IP address
		 */
		ip_addr.l_addr = dev->pa_brdaddr;
		*pf++ = NETF_PUSHWORD+8;	/* PUSH IP dst addr 1st word */
		*pf++ = NETF_PUSHLIT|NETF_EQ;	/* PUSH broadcast ... */
		*pf++ = ip_addr.s_addr[0];	/* ... addr 1st word */
						/* IP dst addr 1st word == */
		*pf++ = NETF_PUSHWORD+9;	/* PUSH IP dst addr 2nd word */
		*pf++ = NETF_PUSHLIT|NETF_EQ;	/* PUSH broadcast ... */
		*pf++ = ip_addr.s_addr[1];	/* ... addr 2nd word */
						/* IP dst addr 2nd word == */
		*pf++ = NETF_AND;		/* IP dst addr == ... */
						/* ... broadcast addr */
		*pf++ = NETF_PUSHZERO|NETF_CNAND;/* PUSH 0 */
						/* Return true if != */
		/*
		 *  Deal now with Interface IP address
		 */
		ip_addr.l_addr = dev->pa_addr;
		*pf++ = NETF_PUSHWORD+8;	/* PUSH IP dst addr 1st word */
		*pf++ = NETF_PUSHLIT|NETF_EQ;	/* PUSH ... */
		*pf++ = ip_addr.s_addr[0];	/* ... ifnet addr 1st word */
						/* IP dst addr 1st word == */
		*pf++ = NETF_PUSHWORD+9;	/* PUSH IP dst addr 2nd word */
		*pf++ = NETF_PUSHLIT|NETF_EQ;	/* PUSH ... */
		*pf++ = ip_addr.s_addr[1];	/* ... ifnet addr 2nd word */
						/* IP dst addr 2nd word == */
		*pf++ = NETF_AND;		/* IP dst addr == ifnet addr */
	}

	kr = device_set_filter(ed->ed_device_port,
			       ed->ed_receive_port,
			       MACH_MSG_TYPE_MAKE_SEND,
			       1,	/* priority */
			       filter,
			       pf - filter);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("ether_set_filter(\"%s\"): "
				    "device_set_filter",
				    dev->name));
	}
}

struct enet_statistics *
ether_get_stats(
	struct device	*dev)
{
	struct ed_device	*ed;

#ifdef	ETHER_DEBUG
	if (ether_debug) {
		printk("ether_get_stats(\"%s\")\n", dev->name);
	}
#endif	/* ETHER_DEBUG */

	ed = (struct ed_device *) dev->priv;
	return &ed->ed_stat;
}

int
ether_open(
	struct device	*dev)
{
	mach_msg_type_number_t	net_stat_count;
	struct net_status	net_stat;
	kern_return_t		kr;
	struct ed_device	*ed;
	
#ifdef	ETHER_DEBUG
	if (ether_debug) {
		printk("ether_open(\"%s\")\n", dev->name);
	}
#endif	/* ETHER_DEBUG */

	/*
	 * Setup a packet filter.
	 */
	ether_set_filter(dev);

	ed = (struct ed_device *) dev->priv;

	net_stat_count = NET_STATUS_COUNT;
	kr = device_get_status(ed->ed_device_port,
			       NET_STATUS,
			       (dev_status_t) &net_stat,
			       &net_stat_count);
	if (kr != D_SUCCESS || net_stat_count < NET_STATUS_COUNT) {
		MACH3_DEBUG(1, kr, ("ether_open(\"%s\"): "
				    "device_get_status(NET_STATUS)",
				    dev->name));
		return 0;
	}
	net_stat.flags |= NET_STATUS_UP;
	kr = device_set_status(ed->ed_device_port,
			       NET_STATUS,
			       (dev_status_t) &net_stat,
			       net_stat_count);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(1, kr, ("ether_open(\"%s\"): "
				    "device_set_status(NET_STATUS)",
				    dev->name));
	}

	dev->start = 1;

	return 0;
}

int
ether_stop(
	struct device	*dev)
{
	mach_msg_type_number_t	net_stat_count;
	struct net_status	net_stat;
	kern_return_t		kr;
	struct ed_device	*ed;
	
#ifdef	ETHER_DEBUG
	if (ether_debug) {
		printk("ether_stop(\"%s\")\n", dev->name);
	}
#endif	/* ETHER_DEBUG */

	ed = (struct ed_device *) dev->priv;

	net_stat_count = NET_STATUS_COUNT;
	kr = device_get_status(ed->ed_device_port,
			       NET_STATUS,
			       (dev_status_t) &net_stat,
			       &net_stat_count);
	if (kr != D_SUCCESS || net_stat_count < NET_STATUS_COUNT) {
		MACH3_DEBUG(1, kr, ("ether_stop(\"%s\"): "
				    "device_get_status(NET_STATUS)",
				    dev->name));
		return 0;
	}
	net_stat.flags &= ~NET_STATUS_UP;
	kr = device_set_status(ed->ed_device_port,
			       NET_STATUS,
			       (dev_status_t) &net_stat,
			       net_stat_count);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(1, kr, ("ether_stop(\"%s\"): "
				    "device_set_status(NET_STATUS)",
				    dev->name));
	}

	return 0;
}

void
ether_xmit(
	struct sk_buff	*skb,
	struct device	*dev)
{
	int				length;
	kern_return_t			kr;
	int				bytes_sent;
	struct ed_device 		*ed;
	struct ether_used		*eu, *old_used;

#ifdef	ETHER_DEBUG
	if (ether_debug) {
		printk("ether_xmit(\"%s\")\n", dev->name);
	}
#endif	/* ETHER_DEBUG */

	ed = (struct ed_device *) dev->priv;

	length = skb->len;
	if (length <= IO_INBAND_MAX) {
		if (!ethernet_synchronous_xmit)
			server_thread_blocking(FALSE);
		kr = serv_device_write_async(ed->ed_device_port,
					     MACH_PORT_NULL,
					     0,	/* mode */
					     0,	/* recnum */
					     skb->data,
					     length,
					     TRUE);
		if (!ethernet_synchronous_xmit)
			server_thread_unblocking(FALSE);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr, ("ether_xmit: "
					    "device_write_inband"));
			ed->ed_stat.tx_errors++;
		} else {
			ed->ed_stat.tx_packets++;
		}
		dev_kfree_skb(skb, FREE_WRITE);
	} else if (ed->ed_maxlen == 0) {
		if (!ethernet_synchronous_xmit)
			server_thread_blocking(FALSE);
		kr = device_write(ed->ed_device_port,
				  0, 0,
				  skb->data,
				  length,
				  &bytes_sent);
		if (!ethernet_synchronous_xmit)
			server_thread_unblocking(FALSE);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr, ("ether_xmit: "
					    "device_write"));
			ed->ed_stat.tx_errors++;
		} else if (bytes_sent != length) {
			printk("ether_xmit: "
			       "sent %d bytes out of %d\n",
			       bytes_sent, length);
			ed->ed_stat.tx_errors++;
		} else {
			ed->ed_stat.tx_packets++;
		}
		dev_kfree_skb(skb, FREE_WRITE);
	} else {
		if (ed->ed_current >= ed->ed_maxlen) {
			if (ether_used_root == NULL) {
				eu = (struct ether_used *)
					kmalloc(sizeof *eu, GFP_ATOMIC);
				if (!eu) {
					ed->ed_stat.tx_dropped++;
					dev_kfree_skb(skb, FREE_WRITE);
					return;
				}
				device_reply_register(&eu->eu_port,
						      (char *) eu,
						      NULL,
						      ether_write_reply);
			} else {
				eu = ether_used_root;
				ether_used_root = eu->eu_next;
				ether_used_cur--;
			}
			skb_queue_head_init(&eu->eu_skb_list);
			ed->ed_current = 0;
			old_used = ed->ed_used;
			ed->ed_used = eu;
		} else {
			old_used = NULL;
		}

		if (!ethernet_synchronous_xmit)
			server_thread_blocking(FALSE);
		kr = serv_device_write_async(ed->ed_device_port,
					     (old_used
					      ? old_used->eu_port
					      : MACH_PORT_NULL),
					     0,	/* mode */
					     0,	/* recnum */
					     skb->data,
					     length,
					     FALSE);
		if (!ethernet_synchronous_xmit)
			server_thread_unblocking(FALSE);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr, ("ether_xmit: "
					    "serv_device_write_async"));
			ed->ed_stat.tx_errors++;
			dev_kfree_skb(skb, FREE_WRITE);
		} else {
			ed->ed_stat.tx_packets++;
			ASSERT(skb->list == NULL);
			skb_queue_tail(&ed->ed_used->eu_skb_list, skb);
			ed->ed_current++;
			skb_device_unlock(skb);		/* XXX */
		}
	}
}

struct condition	ether_xmit_cond;
struct sk_buff_head	ether_xmit_queue;
long			ether_xmit_count = 0;

void *
ether_xmit_thread(
	void *arg)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t			kr;
	struct sk_buff			*skb;

	cthread_set_name(cthread_self(), "ether xmit thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	kr = server_thread_priorities(BASEPRI_SERVER-1, BASEPRI_SERVER-1);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("ether_xmit_thread: server_thread_priorities"));
	}

	uniproc_enter();

	for (;;) {
		while (ether_xmit_count == 0) {
			uniproc_will_exit();
			condition_wait(&ether_xmit_cond, &uniproc_mutex);
			uniproc_has_entered();
		}
		ether_xmit_count--;
		ASSERT(ether_xmit_count >= 0);
		skb = skb_dequeue(&ether_xmit_queue);
		if (skb != NULL) {
			ether_xmit(skb, skb->dev);
		} else {
#ifdef	ETHER_DEBUG
			if (ether_debug) {
				printk("ether_xmit_thread: no skb!\n");
			}
#endif	/* ETHER_DEBUG */
		}

	}
}

int
ether_start_xmit(
	struct sk_buff	*skb,
	struct device	*dev)
{
#ifdef	ETHER_DEBUG
	if (ether_debug) {
		printk("ether_start_xmit(\"%s\")\n", dev->name);
	}
#endif	/* ETHER_DEBUG */

	/*
	 * Sending a NULL skb means some higher layer thinks we've missed a
	 * tx-done interrupt. Caution: dev_tint() handles the cli()/sti()
	 * itself.
	 */
	if (skb == NULL) {
		dev_tint(dev);
		return 0;
	}

	if (skb->len <= 0)
		return 0;

	ASSERT(skb->list == NULL);
	if (ethernet_synchronous_xmit) {
		ether_xmit(skb, dev);
		return 0;
	}
	if (skb->dev != dev) {
		printk("ether_start_xmit: skb->dev (%p) != dev (%p)\n",
		       skb->dev, dev);
		skb->dev = dev;	/* XXX */
	}
	skb_queue_tail(&ether_xmit_queue, skb);
	ASSERT(ether_xmit_count >= 0);
	if (ether_xmit_count++ == 0) {
		condition_signal(&ether_xmit_cond);
	}
	return 0;
}

void
ether_set_multicast_list(
	struct device	*dev)
{
	int			addressIndex;
	struct dev_mc_list	*dmi;
	struct ed_device	*ed;
	unsigned int		new_flags;
	struct net_status	net_stat;
	unsigned int		net_stat_count;
	kern_return_t		kr;

#ifdef ETHER_DEBUG
	if (ether_debug) {
		printk("ether_set_multicast_list(\"%s\") %d addresses\n",
		       dev->name, dev->mc_count);
	}
#endif
	if (!dev) /* No device, don't crash please */
		return;

	ed = (struct ed_device *)dev->priv;

	new_flags = 0;
	if (dev->flags & IFF_PROMISC)
		new_flags |= NET_STATUS_PROMISC;
	if (dev->flags & IFF_ALLMULTI)
		new_flags |= NET_STATUS_ALLMULTI;

	/* Set the promiscuous mode. */
	net_stat_count = NET_STATUS_COUNT;
	kr = device_get_status(ed->ed_device_port,
			       NET_STATUS,
			       (dev_status_t) &net_stat,
			       &net_stat_count);
	if (kr != D_SUCCESS || net_stat_count < NET_STATUS_COUNT) {
		MACH3_DEBUG(1, kr, ("ether_set_multicast_list(\"%s\"): "
				    "device_get_status(NET_STATUS)",
				    dev->name));
		return;
	}
	net_stat.flags &= ~(NET_STATUS_PROMISC | NET_STATUS_ALLMULTI);
	net_stat.flags |= new_flags;
	kr = device_set_status(ed->ed_device_port,
			       NET_STATUS,
			       (dev_status_t) &net_stat,
			       net_stat_count);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(1, kr, ("ether_set_multicast_list(\"%s\"): "
				    "device_set_status(NET_STATUS)",
				    dev->name));
	}
#ifdef	ETHER_DEBUG
	if ((new_flags & NET_STATUS_PROMISC) &&
	    !(net_stat.flags & NET_STATUS_PROMISC)) {
		printk("%s: Promiscuous mode enabled.\n", dev->name);
	} else if (!(new_flags & NET_STATUS_PROMISC) &&
		   (net_stat.flags & NET_STATUS_PROMISC)) {
		printk("%s: Promiscuous mode disabled.\n", dev->name);
	}
#endif
	/* Update packet filter */
	ether_set_filter(dev);

	if (new_flags) {
		return;
	}

	dmi = dev->mc_list;
	for (addressIndex = 0; addressIndex < dev->mc_count; addressIndex++) {
		kern_return_t	kr;
		char data[ETH_ALEN * 2];  /* Store two addresses */
		unsigned char *a;

		a = (unsigned char*)(dmi->dmi_addr);
		dmi = dmi->next;

		/* the 'from' and 'to' addresses are actually equal */
		memcpy(&data[0], a, ETH_ALEN);
		memcpy(&data[ETH_ALEN], a, ETH_ALEN);
		
		kr = device_set_status(ed->ed_device_port,
				  NET_ADDMULTI,
				  (dev_status_t)data,
				  ETH_ALEN*2);
		if (kr != D_SUCCESS) {
			printk("ether_set_multicast_list: "
			       "%02x:%02x:%02x:%02x:%02x:%02x failed (%d)\n",
			       a[0],a[1],a[2],a[3],a[4],a[5], kr);
			printk("You may have an older version "
			       "of the MACE driver\n");
		}
	}

	return;
}

int
ether_set_mac_address(
	struct device	*dev,
	void		*addr)
{
#ifdef	ETHER_DEBUG
	if (ether_debug) {
		printk("ether_set_mac_address(\"%s\")\n", dev->name);
	}
#endif	/* ETHER_DEBUG */

	return -EOPNOTSUPP;
}

int
ether_do_ioctl(
	struct device	*dev,
	struct ifreq	*ifr,
	int		cmd)
{
#ifdef	ETHER_DEBUG
	if (ether_debug) {
		printk("ether_do_ioctl(\"%s\", 0x%x)\n",
		       dev->name, cmd);
	}
#endif	/* ETHER_DEBUG */

	return 0;
}

int
ether_set_config(
	struct device	*dev,
	struct ifmap	*map)
{
#ifdef	ETHER_DEBUG
	if (ether_debug) {
		printk("ether_set_config(\"%s\")\n", dev->name);
	}
#endif	/* ETHER_DEBUG */

	return -EOPNOTSUPP;
}

int
ethdev_init(
	struct device	*dev)
{
	if (dev->priv == NULL) {
		dev->priv = kmalloc(sizeof (struct ed_device),
				    GFP_KERNEL);
	}

	if (dev->open)
		printk("ethdev_init: dev->open 0x%p\n",
		       dev->open);
	dev->open = ether_open;

	if (dev->get_stats)
		printk("ethdev_init: dev->get_stats 0x%p\n",
		       dev->get_stats);
	dev->get_stats = ether_get_stats;

	if (dev->stop)
		printk("ethdev_init: dev->stop 0x%p\n",
		       dev->stop);
	dev->stop = ether_stop;

	if (dev->hard_start_xmit)
		printk("ethdev_init: dev->hard_start_xmit 0x%p\n",
		       dev->hard_start_xmit);
	dev->hard_start_xmit = ether_start_xmit;

	if (dev->set_multicast_list)
		printk("ethdev_init: dev->set_multicast_list 0x%p\n",
		       dev->set_multicast_list);
	dev->set_multicast_list = ether_set_multicast_list;

	if (dev->set_mac_address)
		printk("ethdev_init: dev->set_mac_address 0x%p\n",
		       dev->set_mac_address);
	dev->set_mac_address = ether_set_mac_address;

	if (dev->do_ioctl)
		printk("ethdev_init: dev->do_ioctl 0x%p\n",
		       dev->do_ioctl);
	dev->do_ioctl = ether_do_ioctl;

	if (dev->set_config)
		printk("ethdev_init: dev->set_config 0x%p\n",
		       dev->set_config);
	dev->set_config = ether_set_config;

	ether_setup(dev);

	return 0;
}

void
ethernet_init(void)
{
	static boolean_t	inited = FALSE;

	if (inited)
		return;

	inited = TRUE;
	ether_used_root = NULL;
	ether_used_cur = 0;

	condition_init(&ether_xmit_cond);
	skb_queue_head_init(&ether_xmit_queue);
	server_thread_start(ether_xmit_thread, (void *) 0);
}

char *osfmach3_ethif_devname[] = {
	"wd",		/* Western Digital / SMC */
	"el",		/* EtherLink */
	"pc",		/* PC586 */
	"et",		/* 3c501 */
	"tu",		/* Tulip */
	"ne",		/* NE2000 */
	NULL
};

int
osfmach3_ethif_probe(
	struct device	*dev)
{
	char		mach_devname[16];
	mach_port_t	device_port;
	kern_return_t	kr;
	char 		ether_addr[16];
	unsigned int	ether_addr_count;
	struct net_status net_stat;
	unsigned int	net_stat_count;
	int		i, j;
	int		*ip;
	struct ed_device *ed;
	mach_msg_type_number_t	port_info_count;
	struct mach_port_limits	mpl;

	ethernet_init();

	if (dev == NULL) {
		panic("osfmach3_ethif_probe: null dev");
	}

	/*
	 * Temporary translation of device name.
	 * This should go away when Mach sets "eth?" aliases to ethernet
	 * devices.
	 */
	device_port = MACH_PORT_NULL;
	for (i = 0; osfmach3_ethif_devname[i]; i++) {
		/* XXX try from dev0 up to dev9: completely arbitrary */
		for (j = 0; j < 10; j++) {
			/*
			 * Try to open the corresponding Mach device.
			 */
			sprintf(mach_devname, "%s%d",
				osfmach3_ethif_devname[i], j);
			kr = device_open(device_server_port,
					 MACH_PORT_NULL,
					 D_READ | D_WRITE,
					 server_security_token,
					 mach_devname,
					 &device_port);
			if (kr == D_SUCCESS) {
				break;
			}
		}
		if (device_port != MACH_PORT_NULL)
			break;
	}
	if (device_port == MACH_PORT_NULL)
		return -ENODEV;

	/*
	 * Try to get the ethernet address.
	 */
	ether_addr_count = sizeof (ether_addr) / sizeof (int);
	kr = device_get_status(device_port,
			       NET_ADDRESS,
			       (dev_status_t) ether_addr,
			       &ether_addr_count);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(1, kr, ("osfmach3_ethif_probe(\"%s\"): "
				    "device_get_status(\"%s\", NET_ADDRES)",
				    dev->name, mach_devname));
		kr = device_close(device_port);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr, ("osfmach3_ethif_probe(\"%s\"): "
					    "device_close(\"%s\")",
					    dev->name, mach_devname));
		}
		kr = mach_port_deallocate(mach_task_self(), device_port);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr, ("osfmach3_ethif_probe(\"%s\"): "
					    "mach_port_deallocate(0x%x)",
					    dev->name, device_port));
		}
		return -ENODEV;
	}
	/* Convert the ethernet address into host format. */
	for (i = 0, ip = (int *) ether_addr;
	     i < ether_addr_count;
	     i++, ip++) {
		*ip = ntohl(*ip);
	}
	for (i = 0; i < ether_addr_count * sizeof (int); i++) {
		dev->dev_addr[i] = ether_addr[i];
	}

	ethdev_init(dev);

	ed = (struct ed_device *) dev->priv;
	ed->ed_device_port = device_port;

	/*
	 * Allocate the port on which we will receive network packets
	 * from the micro-kernel.
	 */
	ed->ed_receive_port = mach_reply_port();
	if (!MACH_PORT_VALID(ed->ed_receive_port)) {
		panic("osfmach3_ethif_probe(\"%s\"): mach_reply_port failed",
		      dev->name);
	}
	/*
	 * Set the size of the message queue to the maximum 
	 * to avoid losing too many packets.
	 */
	port_info_count = MACH_PORT_LIMITS_INFO_COUNT;
	kr = mach_port_get_attributes(mach_task_self(),
				      ed->ed_receive_port,
				      MACH_PORT_LIMITS_INFO,
				      (mach_port_info_t) &mpl,
				      &port_info_count);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_ethif_probe(\"%s\": "
			     "mach_port_get_attributes(0x%x, "
			     "MACH_PORT_LIMITS_INFO)",
			     dev->name, ed->ed_receive_port));
	} else {
		port_info_count = MACH_PORT_LIMITS_INFO_COUNT;
		mpl.mpl_qlimit = MACH_PORT_QLIMIT_MAX;
		kr = mach_port_set_attributes(mach_task_self(),
					      ed->ed_receive_port,
					      MACH_PORT_LIMITS_INFO,
					      (mach_port_info_t) &mpl,
					      port_info_count);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_ethif_probe(\"%s\"): "
				     "mach_port_set_attributes(0x%x, "
				     "MACH_PORT_LIMITS_INFO)",
				     dev->name, ed->ed_receive_port));
		}
	}

	/*
	 * Get some info on the Mach device.
	 */
	net_stat_count = NET_STATUS_COUNT;
	kr = device_get_status(device_port,
			       NET_STATUS,
			       (dev_status_t) &net_stat,
			       &net_stat_count);
	if (kr != D_SUCCESS || net_stat_count < NET_STATUS_COUNT) {
		MACH3_DEBUG(1, kr, ("osfmach3_ethif_probe(\"%s\"): "
				    "device_get_status(\"%s\", NET_STATUS)",
				    dev->name, mach_devname));
	} else {
		dev->mtu = net_stat.max_packet_size - net_stat.header_size;
		if (net_stat.header_format != HDR_ETHERNET) {
			panic("osfmach3_ethif_probe(\"%s\"): device \"%s\" is not an ethernet device\n", dev->name, mach_devname);
		}
		ed->ed_maxlen = net_stat.max_queue_size;
		if (ed->ed_maxlen > 0) {
			ed->ed_current = 0;
			ed->ed_used = (struct ether_used *) 
				kmalloc(sizeof *ed->ed_used, GFP_KERNEL);
			skb_queue_head_init(&ed->ed_used->eu_skb_list);
			ed->ed_used->eu_next = NULL;
			device_reply_register(&ed->ed_used->eu_port,
					      (char *) ed->ed_used,
					      NULL,
					      ether_write_reply);
		}
	}

	/*
	 * Start a thread to receive incoming packets.
	 */
	server_thread_start(ether_input_thread, (void *) dev);

	printk("%s: Mach device \"%s\", ",
	       dev->name, mach_devname);
	printk("%02x:%02x:%02x:%02x:%02x:%02x",
	       dev->dev_addr[0],
	       dev->dev_addr[1],
	       dev->dev_addr[2],
	       dev->dev_addr[3],
	       dev->dev_addr[4],
	       dev->dev_addr[5]);
	printk(", max_queue_size=%d\n", ed->ed_maxlen);

	return 0;
}

