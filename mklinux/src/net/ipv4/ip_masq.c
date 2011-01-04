/*
 *
 * 	Masquerading functionality
 *
 * 	Copyright (c) 1994 Pauline Middelink
 *
 *	See ip_fw.c for original log
 *
 * Fixes:
 *	Juan Jose Ciarlante	:	Modularized application masquerading (see ip_masq_app.c)
 *	Juan Jose Ciarlante	:	New struct ip_masq_seq that holds output/input delta seq.
 *	Juan Jose Ciarlante	:	Added hashed lookup by proto,maddr,mport and proto,saddr,sport
 *	Juan Jose Ciarlante	:	Fixed deadlock if free ports get exhausted
 *	Juan Jose Ciarlante	:	Added NO_ADDR status flag.
 *	Richard Lynch		:	Added IP Autoforward
 *	Nigel Metheringham	:	Added ICMP handling for demasquerade
 *	Nigel Metheringham	:	Checksum checking of masqueraded data
 *	Nigel Metheringham	:	Better handling of timeouts of TCP conns
 *	Keith Owens		:	Keep control channels alive if any related data entries.
 *	Delian Delchev		:	Added support for ICMP requests and replys
 *	Nigel Metheringham	:	ICMP in ICMP handling, tidy ups, bug fixes, made ICMP optional
 *	Juan Jose Ciarlante	:	re-assign maddr if no packet received from outside
 *	
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <asm/system.h>
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/inet.h>
#include <net/protocol.h>
#include <net/icmp.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/checksum.h>
#include <net/ip_masq.h>
#include <linux/ip_fw.h>

#define IP_MASQ_TAB_SIZE 256    /* must be power of 2 */

/*
 *	Implement IP packet masquerading
 */

static const char *strProt[] = {"UDP","TCP","ICMP"};

/*
 * masq_proto_num returns 0 for UDP, 1 for TCP, 2 for ICMP
 */

static int masq_proto_num(unsigned proto)
{
   switch (proto)
   {
      case IPPROTO_UDP:  return (0); break;
      case IPPROTO_TCP:  return (1); break;
      case IPPROTO_ICMP: return (2); break;
      default:           return (-1); break;
   }
}

#ifdef CONFIG_IP_MASQUERADE_ICMP
/*
 * Converts an ICMP reply code into the equivalent request code
 */
static __inline__ const __u8 icmp_type_request(__u8 type)
{
   switch (type)
   {
      case ICMP_ECHOREPLY: return ICMP_ECHO; break;
      case ICMP_TIMESTAMPREPLY: return ICMP_TIMESTAMP; break;
      case ICMP_INFO_REPLY: return ICMP_INFO_REQUEST; break;
      case ICMP_ADDRESSREPLY: return ICMP_ADDRESS; break;
      default: return (255); break;
   }
}

/*
 * Helper macros - attempt to make code clearer! 
 */

/* ID used in ICMP lookups */
#define icmp_id(icmph)		((icmph->un).echo.id)
/* (port) hash value using in ICMP lookups for requests */
#define icmp_hv_req(icmph)	((__u16)(icmph->code+(__u16)(icmph->type<<8)))
/* (port) hash value using in ICMP lookups for replies */
#define icmp_hv_rep(icmph)	((__u16)(icmph->code+(__u16)(icmp_type_request(icmph->type)<<8)))
#endif

static __inline__ const char *masq_proto_name(unsigned proto)
{
        return strProt[masq_proto_num(proto)];
}

/*
 *	Last masq_port number in use.
 *	Will cycle in MASQ_PORT boundaries.
 */
static __u16 masq_port = PORT_MASQ_BEGIN;

/*
 *	free ports counters (UDP & TCP)
 *
 *	Their value is _less_ or _equal_ to actual free ports:
 *	same masq port, diff masq addr (firewall iface address) allocated
 *	entries are accounted but their actually don't eat a more than 1 port.
 *
 *	Greater values could lower MASQ_EXPIRATION setting as a way to
 *	manage 'masq_entries resource'.
 *	
 */

int ip_masq_free_ports[3] = {
        PORT_MASQ_END - PORT_MASQ_BEGIN, 	/* UDP */
        PORT_MASQ_END - PORT_MASQ_BEGIN, 	/* TCP */
        PORT_MASQ_END - PORT_MASQ_BEGIN		/* ICMP */
};

static struct symbol_table ip_masq_syms = {
#include <linux/symtab_begin.h>
	X(ip_masq_new),
        X(ip_masq_set_expire),
        X(ip_masq_free_ports),
	X(ip_masq_expire),
	X(ip_masq_out_get_2),
#include <linux/symtab_end.h>
};

/*
 *	2 ip_masq hash tables: for input and output pkts lookups.
 */

struct ip_masq *ip_masq_m_tab[IP_MASQ_TAB_SIZE];
struct ip_masq *ip_masq_s_tab[IP_MASQ_TAB_SIZE];

/*
 * timeouts
 */

static struct ip_fw_masq ip_masq_dummy = {
	MASQUERADE_EXPIRE_TCP,
	MASQUERADE_EXPIRE_TCP_FIN,
	MASQUERADE_EXPIRE_UDP
};

struct ip_fw_masq *ip_masq_expire = &ip_masq_dummy;

#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW
/*
 *	Auto-forwarding table
 */

struct ip_autofw * ip_autofw_hosts = NULL;

/*
 *	Check if a masq entry should be created for a packet
 */

struct ip_autofw * ip_autofw_check_range (__u32 where, __u16 port, __u16 protocol, int reqact)
{
	struct ip_autofw *af;
	af=ip_autofw_hosts;
	port=ntohs(port);
	while (af)
	{
		if (af->type==IP_FWD_RANGE && 
		     port>=af->low && 
		     port<=af->high && 
		     protocol==af->protocol && 
		     /* it's ok to create masq entries after the timeout if we're in insecure mode */
		     (af->flags & IP_AUTOFW_ACTIVE || !reqact || !(af->flags & IP_AUTOFW_SECURE)) &&  
		     (!(af->flags & IP_AUTOFW_SECURE) || af->lastcontact==where || !reqact))
			return(af);
		af=af->next;
	}
	return(NULL);
}

struct ip_autofw * ip_autofw_check_port (__u16 port, __u16 protocol)
{
	struct ip_autofw *af;
	af=ip_autofw_hosts;
	port=ntohs(port);
	while (af)
	{
		if (af->type==IP_FWD_PORT && port==af->visible && protocol==af->protocol)
			return(af);
		af=af->next;
	}
	return(NULL);
}

struct ip_autofw * ip_autofw_check_direct (__u16 port, __u16 protocol)
{
	struct ip_autofw *af;
	af=ip_autofw_hosts;
	port=ntohs(port);
	while (af)
	{
		if (af->type==IP_FWD_DIRECT && af->low<=port && af->high>=port)
			return(af);
		af=af->next;
	}
	return(NULL);
}

void ip_autofw_update_out (__u32 who, __u32 where, __u16 port, __u16 protocol)
{
	struct ip_autofw *af;
	af=ip_autofw_hosts;
	port=ntohs(port);
	while (af)
	{
		if (af->type==IP_FWD_RANGE && af->ctlport==port && af->ctlproto==protocol)
		{
			if (af->flags & IP_AUTOFW_USETIME)
			{
				if (af->timer.expires)
					del_timer(&af->timer);
				af->timer.expires=jiffies+IP_AUTOFW_EXPIRE;
				add_timer(&af->timer);
			}
			af->flags|=IP_AUTOFW_ACTIVE;
			af->lastcontact=where;
			af->where=who;
		}
		af=af->next;
	}
}

void ip_autofw_update_in (__u32 where, __u16 port, __u16 protocol)
{
/*	struct ip_autofw *af;
	af=ip_autofw_check_range(where, port,protocol);
	if (af)
	{
		del_timer(&af->timer);
		af->timer.expires=jiffies+IP_AUTOFW_EXPIRE;
		add_timer(&af->timer);
	}*/
}

#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */

/*
 *	Returns hash value
 */

static __inline__ unsigned
ip_masq_hash_key(unsigned proto, __u32 addr, __u16 port)
{
        return (proto^ntohl(addr)^ntohs(port)) & (IP_MASQ_TAB_SIZE-1);
}

/*
 *	Hashes ip_masq by its proto,addrs,ports.
 *	should be called with masked interrupts.
 *	returns bool success.
 */

static __inline__ int
ip_masq_hash(struct ip_masq *ms)
{
        unsigned hash;

        if (ms->flags & IP_MASQ_F_HASHED) {
                printk("ip_masq_hash(): request for already hashed\n");
                return 0;
        }
        /*
         *	Hash by proto,m{addr,port}
         */
        hash = ip_masq_hash_key(ms->protocol, ms->maddr, ms->mport);
        ms->m_link = ip_masq_m_tab[hash];
        ip_masq_m_tab[hash] = ms;

        /*
         *	Hash by proto,s{addr,port}
         */
        hash = ip_masq_hash_key(ms->protocol, ms->saddr, ms->sport);
        ms->s_link = ip_masq_s_tab[hash];
        ip_masq_s_tab[hash] = ms;


        ms->flags |= IP_MASQ_F_HASHED;
        return 1;
}

/*
 *	UNhashes ip_masq from ip_masq_[ms]_tables.
 *	should be called with masked interrupts.
 *	returns bool success.
 */

static __inline__ int ip_masq_unhash(struct ip_masq *ms)
{
        unsigned hash;
        struct ip_masq ** ms_p;
        if (!(ms->flags & IP_MASQ_F_HASHED)) {
                printk("ip_masq_unhash(): request for unhash flagged\n");
                return 0;
        }
        /*
         *	UNhash by m{addr,port}
         */
        hash = ip_masq_hash_key(ms->protocol, ms->maddr, ms->mport);
        for (ms_p = &ip_masq_m_tab[hash]; *ms_p ; ms_p = &(*ms_p)->m_link)
                if (ms == (*ms_p))  {
                        *ms_p = ms->m_link;
                        break;
                }
        /*
         *	UNhash by s{addr,port}
         */
        hash = ip_masq_hash_key(ms->protocol, ms->saddr, ms->sport);
        for (ms_p = &ip_masq_s_tab[hash]; *ms_p ; ms_p = &(*ms_p)->s_link)
                if (ms == (*ms_p))  {
                        *ms_p = ms->s_link;
                        break;
                }

        ms->flags &= ~IP_MASQ_F_HASHED;
        return 1;
}

/*
 *	Returns ip_masq associated with addresses found in iph.
 *	called for pkts coming from outside-to-INside the firewall
 *
 * 	NB. Cannot check destination address, just for the incoming port.
 * 	reason: archie.doc.ac.uk has 6 interfaces, you send to
 * 	phoenix and get a reply from any other interface(==dst)!
 *
 * 	[Only for UDP] - AC
 */

struct ip_masq *
ip_masq_in_get(struct iphdr *iph)
{
 	__u16 *portptr;
        int protocol;
        __u32 s_addr, d_addr;
        __u16 s_port, d_port;

 	portptr = (__u16 *)&(((char *)iph)[iph->ihl*4]);
        protocol = iph->protocol;
        s_addr = iph->saddr;
        s_port = portptr[0];
        d_addr = iph->daddr;
        d_port = portptr[1];

        return ip_masq_in_get_2(protocol, s_addr, s_port, d_addr, d_port);
}

/*
 *	Returns ip_masq associated with supplied parameters, either
 *	broken out of the ip/tcp headers or directly supplied for those
 *	pathological protocols with address/port in the data stream
 *	(ftp, irc).  addresses and ports are in network order.
 *	called for pkts coming from outside-to-INside the firewall.
 *
 * 	NB. Cannot check destination address, just for the incoming port.
 * 	reason: archie.doc.ac.uk has 6 interfaces, you send to
 * 	phoenix and get a reply from any other interface(==dst)!
 *
 * 	[Only for UDP] - AC
 */

struct ip_masq *
ip_masq_in_get_2(int protocol, __u32 s_addr, __u16 s_port, __u32 d_addr, __u16 d_port)
{
        unsigned hash;
        struct ip_masq *ms;

        hash = ip_masq_hash_key(protocol, d_addr, d_port);
        for(ms = ip_masq_m_tab[hash]; ms ; ms = ms->m_link) {
 		if (protocol==ms->protocol &&
		    ((s_addr==ms->daddr || ms->flags & IP_MASQ_F_NO_DADDR)
#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW
		     || (ms->dport==htons(1558))
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */
		     ) &&
		    (s_port==ms->dport || ms->flags & IP_MASQ_F_NO_DPORT) &&
		    (d_addr==ms->maddr && d_port==ms->mport)) {
#ifdef DEBUG_IP_MASQUERADE_VERBOSE
			printk("MASQ: look/in %d %08X:%04hX->%08X:%04hX OK\n",
			       protocol,
			       s_addr,
			       s_port,
			       d_addr,
			       d_port);
#endif
                        return ms;
		}
        }
#ifdef DEBUG_IP_MASQUERADE_VERBOSE
	printk("MASQ: look/in %d %08X:%04hX->%08X:%04hX fail\n",
	       protocol,
	       s_addr,
	       s_port,
	       d_addr,
	       d_port);
#endif
        return NULL;
}

/*
 *	Returns ip_masq associated with addresses found in iph.
 *	called for pkts coming from inside-to-OUTside the firewall.
 */

struct ip_masq *
ip_masq_out_get(struct iphdr *iph)
{
 	__u16 *portptr;
        int protocol;
        __u32 s_addr, d_addr;
        __u16 s_port, d_port;

 	portptr = (__u16 *)&(((char *)iph)[iph->ihl*4]);
        protocol = iph->protocol;
        s_addr = iph->saddr;
        s_port = portptr[0];
        d_addr = iph->daddr;
        d_port = portptr[1];

        return ip_masq_out_get_2(protocol, s_addr, s_port, d_addr, d_port);
}

/*
 *	Returns ip_masq associated with supplied parameters, either
 *	broken out of the ip/tcp headers or directly supplied for those
 *	pathological protocols with address/port in the data stream
 *	(ftp, irc).  addresses and ports are in network order.
 *	called for pkts coming from inside-to-OUTside the firewall.
 *
 *	Normally we know the source address and port but for some protocols
 *	(e.g. ftp PASV) we do not know the source port initially.  Alas the
 *	hash is keyed on source port so if the first lookup fails then try again
 *	with a zero port, this time only looking at entries marked "no source
 *	port".
 */

struct ip_masq *
ip_masq_out_get_2(int protocol, __u32 s_addr, __u16 s_port, __u32 d_addr, __u16 d_port)
{
        unsigned hash;
        struct ip_masq *ms;


        hash = ip_masq_hash_key(protocol, s_addr, s_port);
        for(ms = ip_masq_s_tab[hash]; ms ; ms = ms->s_link) {
		if (protocol == ms->protocol &&
		    s_addr == ms->saddr && s_port == ms->sport &&
                    d_addr == ms->daddr && d_port == ms->dport ) {
#ifdef DEBUG_IP_MASQUERADE_VERBOSE
			printk("MASQ: lk/out1 %d %08X:%04hX->%08X:%04hX OK\n",
			       protocol,
			       s_addr,
			       s_port,
			       d_addr,
			       d_port);
#endif
                        return ms;
		}
        }
        hash = ip_masq_hash_key(protocol, s_addr, 0);
        for(ms = ip_masq_s_tab[hash]; ms ; ms = ms->s_link) {
		if (ms->flags & IP_MASQ_F_NO_SPORT &&
		    protocol == ms->protocol &&
		    s_addr == ms->saddr && 
                    d_addr == ms->daddr && d_port == ms->dport ) {
#ifdef DEBUG_IP_MASQUERADE_VERBOSE
			printk("MASQ: lk/out2 %d %08X:%04hX->%08X:%04hX OK\n",
			       protocol,
			       s_addr,
			       s_port,
			       d_addr,
			       d_port);
#endif
                        return ms;
		}
        }
#ifdef DEBUG_IP_MASQUERADE_VERBOSE
	printk("MASQ: lk/out1 %d %08X:%04hX->%08X:%04hX fail\n",
	       protocol,
	       s_addr,
	       s_port,
	       d_addr,
	       d_port);
#endif
        return NULL;
}

/*
 *	Returns ip_masq for given proto,m_addr,m_port.
 *      called by allocation routine to find an unused m_port.
 */

struct ip_masq *
ip_masq_getbym(int protocol, __u32 m_addr, __u16 m_port)
{
        unsigned hash;
        struct ip_masq *ms;

        hash = ip_masq_hash_key(protocol, m_addr, m_port);
        for(ms = ip_masq_m_tab[hash]; ms ; ms = ms->m_link) {
 		if ( protocol==ms->protocol &&
                    (m_addr==ms->maddr && m_port==ms->mport))
                        return ms;
        }
        return NULL;
}

static void masq_expire(unsigned long data)
{
	struct ip_masq *ms = (struct ip_masq *)data, *ms_data;
	unsigned long flags;

	if (ms->flags & IP_MASQ_F_CONTROL) {
		/* a control channel is about to expire */
		int idx = 0, reprieve = 0;
#ifdef DEBUG_CONFIG_IP_MASQUERADE
		printk("Masquerade control %s %lX:%X about to expire\n",
				masq_proto_name(ms->protocol),
				ntohl(ms->saddr),ntohs(ms->sport));
#endif
		save_flags(flags);
		cli();

		/*
		 * If any other masquerade entry claims that the expiring entry
		 * is its control channel then keep the control entry alive.
		 * Useful for long running data channels with inactive control
		 * links which we don't want to lose, e.g. ftp.
		 * Assumption: loops such as a->b->a or a->a will never occur.
		 */
		for (idx = 0; idx < IP_MASQ_TAB_SIZE && !reprieve; idx++) {
			for (ms_data = ip_masq_m_tab[idx]; ms_data ; ms_data = ms_data->m_link) {
				if (ms_data->control == ms) {
					reprieve = 1;	/* this control connection can live a bit longer */
					ip_masq_set_expire(ms, ip_masq_expire->tcp_timeout);
#ifdef DEBUG_CONFIG_IP_MASQUERADE
					printk("Masquerade control %s %lX:%X expiry reprieved\n",
							masq_proto_name(ms->protocol),
							ntohl(ms->saddr),ntohs(ms->sport));
#endif
					break;
				}
			}
		}
		restore_flags(flags);
		if (reprieve)
			return;
	}

#ifdef DEBUG_CONFIG_IP_MASQUERADE
	printk("Masqueraded %s %lX:%X expired\n",masq_proto_name(ms->protocol),ntohl(ms->saddr),ntohs(ms->sport));
#endif
	
	save_flags(flags);
	cli();

        if (ip_masq_unhash(ms)) {
                ip_masq_free_ports[masq_proto_num(ms->protocol)]++;
                if (ms->protocol != IPPROTO_ICMP)
                             ip_masq_unbind_app(ms);
                kfree_s(ms,sizeof(*ms));
        }

	restore_flags(flags);
}

#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW
void ip_autofw_expire(unsigned long data)
{
	struct ip_autofw * af;
	af=(struct ip_autofw *) data;
	af->flags&=0xFFFF ^ IP_AUTOFW_ACTIVE;
	af->timer.expires=0;
	af->lastcontact=0;
	if (af->flags & IP_AUTOFW_SECURE)
		af->where=0;
}
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */

/*
 * 	Create a new masquerade list entry, also allocate an
 * 	unused mport, keeping the portnumber between the
 * 	given boundaries MASQ_BEGIN and MASQ_END.
 */

struct ip_masq * ip_masq_new_enh(struct device *dev, int proto, __u32 saddr, __u16 sport, __u32 daddr, __u16 dport, unsigned mflags, __u16 matchport)
{
        struct ip_masq *ms, *mst;
        int ports_tried, *free_ports_p;
	unsigned long flags;
        static int n_fails = 0;

        free_ports_p = &ip_masq_free_ports[masq_proto_num(proto)];

        if (*free_ports_p == 0) {
                if (++n_fails < 5)
                        printk("ip_masq_new(proto=%s): no free ports.\n",
                               masq_proto_name(proto));
                return NULL;
        }
        ms = (struct ip_masq *) kmalloc(sizeof(struct ip_masq), GFP_ATOMIC);
        if (ms == NULL) {
                if (++n_fails < 5)
                        printk("ip_masq_new(proto=%s): no memory available.\n",
                               masq_proto_name(proto));
                return NULL;
        }
        memset(ms, 0, sizeof(*ms));
	init_timer(&ms->timer);
	ms->timer.data     = (unsigned long)ms;
	ms->timer.function = masq_expire;
        ms->protocol	   = proto;
        ms->saddr    	   = saddr;
        ms->sport	   = sport;
        ms->daddr	   = daddr;
        ms->dport	   = dport;
        ms->flags	   = mflags;
        ms->app_data	   = NULL;
	ms->control	   = NULL;

        if (proto == IPPROTO_UDP && !matchport)
                ms->flags |= IP_MASQ_F_NO_DADDR;
        
        /* get masq address from rif */
        ms->maddr	   = dev->pa_addr;
        /*
         *	Setup new entry as not replied yet.
         *	This flag will allow masq. addr (ms->maddr)
         *	to follow forwarding interface address.
         */
        ms->flags         |= IP_MASQ_F_NO_REPLY;

        for (ports_tried = 0; 
	     (*free_ports_p && (ports_tried <= (PORT_MASQ_END - PORT_MASQ_BEGIN)));
	     ports_tried++){
                save_flags(flags);
                cli();
                
		/*
                 *	Try the next available port number
                 */
                if (!matchport || ports_tried)
			ms->mport = htons(masq_port++);
		else
			ms->mport = matchport;
			
		if (masq_port==PORT_MASQ_END) masq_port = PORT_MASQ_BEGIN;
                
                restore_flags(flags);
                
                /*
                 *	lookup to find out if this port is used.
                 */
                
                mst = ip_masq_getbym(proto, ms->maddr, ms->mport);
                if (mst == NULL || matchport) {
                        save_flags(flags);
                        cli();
                
                        if (*free_ports_p == 0) {
                                restore_flags(flags);
                                break;
                        }
                        (*free_ports_p)--;
                        ip_masq_hash(ms);
                        
                        restore_flags(flags);
                        
                        if (proto != IPPROTO_ICMP)
                              ip_masq_bind_app(ms);
                        n_fails = 0;
                        return ms;
                }
        }
        
        if (++n_fails < 5)
                printk("ip_masq_new(proto=%s): could not get free masq entry (free=%d).\n",
                       masq_proto_name(ms->protocol), *free_ports_p);
        kfree_s(ms, sizeof(*ms));
        return NULL;
}

struct ip_masq * ip_masq_new(struct device *dev, int proto, __u32 saddr, __u16 sport, __u32 daddr, __u16 dport, unsigned mflags)
{
	return (ip_masq_new_enh(dev, proto, saddr, sport, daddr, dport, mflags, 0) );
}

/*
 * 	Set masq expiration (deletion) and adds timer,
 *	if timeout==0 cancel expiration.
 *	Warning: it does not check/delete previous timer!
 */

void ip_masq_set_expire(struct ip_masq *ms, unsigned long tout)
{
        if (tout) {
                ms->timer.expires = jiffies+tout;
                add_timer(&ms->timer);
        } else {
                del_timer(&ms->timer);
        }
}

static void recalc_check(struct udphdr *uh, __u32 saddr,
	__u32 daddr, int len)
{
	uh->check=0;
	uh->check=csum_tcpudp_magic(saddr,daddr,len,
		IPPROTO_UDP, csum_partial((char *)uh,len,0));
	if(uh->check==0)
		uh->check=0xFFFF;
}
	
int ip_fw_masquerade(struct sk_buff **skb_ptr, struct device *dev)
{
	struct sk_buff  *skb=*skb_ptr;
	struct iphdr	*iph = skb->h.iph;
	__u16	*portptr;
	struct ip_masq	*ms;
	int		size;
        unsigned long 	timeout;

	/*
	 * We can only masquerade protocols with ports...
	 * [TODO]
	 * We may need to consider masq-ing some ICMP related to masq-ed protocols
	 */

        if (iph->protocol==IPPROTO_ICMP) 
            return (ip_fw_masq_icmp(skb_ptr,dev));
	if (iph->protocol!=IPPROTO_UDP && iph->protocol!=IPPROTO_TCP)
		return -1;

	/*
	 *	Now hunt the list to see if we have an old entry
	 */

	portptr = (__u16 *)&(((char *)iph)[iph->ihl*4]);
#ifdef DEBUG_CONFIG_IP_MASQUERADE
	printk("Outgoing %s %lX:%X -> %lX:%X\n",
		masq_proto_name(iph->protocol),
		ntohl(iph->saddr), ntohs(portptr[0]),
		ntohl(iph->daddr), ntohs(portptr[1]));
#endif

        ms = ip_masq_out_get(iph);
	if (ms!=NULL) {
                ip_masq_set_expire(ms,0);
                
                /*
                 *	If sysctl !=0 and no pkt has been received yet
                 *	in this tunnel and routing iface address has changed...
                 *	 "You are welcome, diald".
                 */
                if ( sysctl_ip_dynaddr && ms->flags & IP_MASQ_F_NO_REPLY && dev->pa_addr != ms->maddr) {
                        unsigned long flags;
                        if (sysctl_ip_dynaddr > 1) {
                                printk(KERN_INFO "ip_fw_masquerade(): change maddr from %s",
                                       in_ntoa(ms->maddr));
                                printk(" to %s\n", in_ntoa(dev->pa_addr));
                        }
                        save_flags(flags);
                        cli();
                        ip_masq_unhash(ms);
                        ms->maddr = dev->pa_addr;
                        ip_masq_hash(ms);
                        restore_flags(flags);
                }
                
		/*
		 *      Set sport if not defined yet (e.g. ftp PASV).  Because
		 *	masq entries are hashed on sport, unhash with old value
		 *	and hash with new.
		 */

		if ( ms->flags & IP_MASQ_F_NO_SPORT && ms->protocol == IPPROTO_TCP ) {
			unsigned long flags;
			ms->flags &= ~IP_MASQ_F_NO_SPORT;
			save_flags(flags);
			cli();
			ip_masq_unhash(ms);
			ms->sport = portptr[0];
			ip_masq_hash(ms);	/* hash on new sport */
			restore_flags(flags);
#ifdef DEBUG_CONFIG_IP_MASQUERADE
			printk("ip_fw_masquerade(): filled sport=%d\n",
			       ntohs(ms->sport));
#endif
		}
	}

#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW
	/* update any ipautofw entries .. */
	ip_autofw_update_out(iph->saddr, iph->daddr, portptr[1], 
			     iph->protocol);
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */

	/*
	 *	Nope, not found, create a new entry for it
	 */
	if (ms==NULL)
	{
#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW
		/* if the source port is supposed to match the masq port, then
		   make it so */
		if (ip_autofw_check_direct(portptr[1],iph->protocol))
	                ms = ip_masq_new_enh(dev, iph->protocol,
        	                         iph->saddr, portptr[0],
                	                 iph->daddr, portptr[1],
                        	         0,
                        	         portptr[0]);
                else
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */
	                ms = ip_masq_new_enh(dev, iph->protocol,
        	                         iph->saddr, portptr[0],
                	                 iph->daddr, portptr[1],
                        	         0,
                        	         0);
                if (ms == NULL)
			return -1;
 	}

 	/*
 	 *	Change the fragments origin
 	 */
 	
 	size = skb->len - ((unsigned char *)portptr - skb->h.raw);
        /*
         *	Set iph addr and port from ip_masq obj.
         */
 	iph->saddr = ms->maddr;
 	portptr[0] = ms->mport;

 	/*
 	 *	Attempt ip_masq_app call.
         *	will fix ip_masq and iph seq stuff
 	 */
        if (ip_masq_app_pkt_out(ms, skb_ptr, dev) != 0)
	{
                /*
                 *	skb has possibly changed, update pointers.
                 */
                skb = *skb_ptr;
                iph = skb->h.iph;
                portptr = (__u16 *)&(((char *)iph)[iph->ihl*4]);
                size = skb->len - ((unsigned char *)portptr-skb->h.raw);
        }

 	/*
 	 *	Adjust packet accordingly to protocol
 	 */
 	
 	if (masq_proto_num(iph->protocol)==0)
 	{
                timeout = ip_masq_expire->udp_timeout;
 		recalc_check((struct udphdr *)portptr,iph->saddr,iph->daddr,size);
 	}
 	else
 	{
 		struct tcphdr *th;
 		th = (struct tcphdr *)portptr;

		/* Set the flags up correctly... */
		if (th->fin)
		{
			ms->flags |= IP_MASQ_F_SAW_FIN_OUT;
		}

		if (th->rst)
		{
			ms->flags |= IP_MASQ_F_SAW_RST;
		}

 		/*
 		 *	Timeout depends if FIN packet has been seen
		 *	Very short timeout if RST packet seen.
 		 */
 		if (ms->flags & IP_MASQ_F_SAW_RST)
		{
                        timeout = 1;
		}
 		else if ((ms->flags & IP_MASQ_F_SAW_FIN) == IP_MASQ_F_SAW_FIN)
		{
                        timeout = ip_masq_expire->tcp_fin_timeout;
		}
 		else timeout = ip_masq_expire->tcp_timeout;

		skb->csum = csum_partial((void *)(th + 1), size - sizeof(*th), 0);
 		tcp_send_check(th,iph->saddr,iph->daddr,size,skb);
 	}
        ip_masq_set_expire(ms, timeout);
 	ip_send_check(iph);

 #ifdef DEBUG_CONFIG_IP_MASQUERADE
 	printk("O-routed from %lX:%X over %s\n",ntohl(ms->maddr),ntohs(ms->mport),dev->name);
 #endif

	return 0;
 }


/*
 *	Handle ICMP messages in forward direction.
 *	Find any that might be relevant, check against existing connections,
 *	forward to masqueraded host if relevant.
 *	Currently handles error types - unreachable, quench, ttl exceeded
 */

int ip_fw_masq_icmp(struct sk_buff **skb_p, struct device *dev)
{
        struct sk_buff 	*skb   = *skb_p;
 	struct iphdr	*iph   = skb->h.iph;
	struct icmphdr  *icmph = (struct icmphdr *)((char *)iph + (iph->ihl<<2));
	struct iphdr    *ciph;	/* The ip header contained within the ICMP */
	__u16	        *pptr;	/* port numbers from TCP/UDP contained header */
	struct ip_masq	*ms;
	unsigned short   len   = ntohs(iph->tot_len) - (iph->ihl * 4);

#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
 	printk("Incoming forward ICMP (%d,%d) %lX -> %lX\n",
	        icmph->type, ntohs(icmp_id(icmph)),
 		ntohl(iph->saddr), ntohl(iph->daddr));
#endif

#ifdef CONFIG_IP_MASQUERADE_ICMP		
	if ((icmph->type == ICMP_ECHO ) ||
	    (icmph->type == ICMP_TIMESTAMP ) ||
	    (icmph->type == ICMP_INFO_REQUEST ) ||
	    (icmph->type == ICMP_ADDRESS )) {
#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
		printk("MASQ: icmp request rcv %lX->%lX id %d type %d\n",
		       ntohl(iph->saddr),
		       ntohl(iph->daddr),
		       ntohs(icmp_id(icmph)),
		       icmph->type);
#endif
		ms = ip_masq_out_get_2(iph->protocol,
				       iph->saddr,
				       icmp_id(icmph),
				       iph->daddr,
				       icmp_hv_req(icmph));
		if (ms == NULL) {
			ms = ip_masq_new(dev,
					 iph->protocol,
					 iph->saddr,
					 icmp_id(icmph),
					 iph->daddr,
					 icmp_hv_req(icmph),
					 0);
			if (ms == NULL)
				return (-1);
#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
			printk("MASQ: Create new icmp entry\n");
#endif	              
		}
		ip_masq_set_expire(ms, 0);
		/* Rewrite source address */
                
                /*
                 *	If sysctl !=0 and no pkt has been received yet
                 *	in this tunnel and routing iface address has changed...
                 *	 "You are welcome, diald".
                 */
                if ( sysctl_ip_dynaddr && ms->flags & IP_MASQ_F_NO_REPLY && dev->pa_addr != ms->maddr) {
                        unsigned long flags;
#ifdef DEBUG_CONFIG_IP_MASQUERADE
                        printk(KERN_INFO "ip_fw_masq_icmp(): change masq.addr %s",
                               in_ntoa(ms->maddr));
                        printk("-> %s\n", in_ntoa(dev->pa_addr));
#endif
                        save_flags(flags);
                        cli();
                        ip_masq_unhash(ms);
                        ms->maddr = dev->pa_addr;
                        ip_masq_hash(ms);
                        restore_flags(flags);
                }
                
		iph->saddr = ms->maddr;
		ip_send_check(iph);
		/* Rewrite port (id) */
		(icmph->un).echo.id = ms->mport;
		icmph->checksum = 0;
		icmph->checksum = ip_compute_csum((unsigned char *)icmph, len);
		ip_masq_set_expire(ms, MASQUERADE_EXPIRE_ICMP);
#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
		printk("MASQ: icmp request rwt %lX->%lX id %d type %d\n",
		       ntohl(iph->saddr),
		       ntohl(iph->daddr),
		       ntohs(icmp_id(icmph)),
		       icmph->type);
#endif
		return (1);
	}
#endif

	/* 
	 * Work through seeing if this is for us.
	 * These checks are supposed to be in an order that
	 * means easy things are checked first to speed up
	 * processing.... however this means that some
	 * packets will manage to get a long way down this
	 * stack and then be rejected, but thats life
	 */
	if ((icmph->type != ICMP_DEST_UNREACH) &&
	    (icmph->type != ICMP_SOURCE_QUENCH) &&
	    (icmph->type != ICMP_TIME_EXCEEDED))
		return 0;

	/* Now find the contained IP header */
	ciph = (struct iphdr *) (icmph + 1);

#ifdef CONFIG_IP_MASQUERADE_ICMP
	if (ciph->protocol == IPPROTO_ICMP) {
		/*
		 * This section handles ICMP errors for ICMP packets
		 */
		struct icmphdr  *cicmph = (struct icmphdr *)((char *)ciph + 
							     (ciph->ihl<<2));

#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
		printk("MASQ: fw icmp/icmp rcv %lX->%lX id %d type %d\n",
		       ntohl(ciph->saddr),
		       ntohl(ciph->daddr),
		       ntohs(icmp_id(cicmph)),
		       cicmph->type);
#endif
		ms = ip_masq_out_get_2(ciph->protocol, 
				      ciph->daddr,
				      icmp_id(cicmph),
				      ciph->saddr,
				      icmp_hv_rep(cicmph));

		if (ms == NULL)
			return 0;

		/* Now we do real damage to this packet...! */
		/* First change the source IP address, and recalc checksum */
		iph->saddr = ms->maddr;
		ip_send_check(iph);
	
		/* Now change the *dest* address in the contained IP */
		ciph->daddr = ms->maddr;
		ip_send_check(ciph);

		/* Change the ID to the masqed one! */
		(cicmph->un).echo.id = ms->mport;
	
		/* And finally the ICMP checksum */
		icmph->checksum = 0;
		icmph->checksum = ip_compute_csum((unsigned char *) icmph, len);

#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
		printk("MASQ: fw icmp/icmp rwt %lX->%lX id %d type %d\n",
		       ntohl(ciph->saddr),
		       ntohl(ciph->daddr),
		       ntohs(icmp_id(cicmph)),
		       cicmph->type);
#endif
		return 1;
	}
#endif /* CONFIG_IP_MASQUERADE_ICMP */

	/* We are only interested ICMPs generated from TCP or UDP packets */
	if ((ciph->protocol != IPPROTO_UDP) && (ciph->protocol != IPPROTO_TCP))
		return 0;

	/* 
	 * Find the ports involved - this packet was 
	 * incoming so the ports are right way round
	 * (but reversed relative to outer IP header!)
	 */
	pptr = (__u16 *)&(((char *)ciph)[ciph->ihl*4]);

	/* Ensure the checksum is correct */
	if (ip_compute_csum((unsigned char *) icmph, len)) 
	{
		/* Failed checksum! */
		printk(KERN_DEBUG "MASQ: forward ICMP: failed checksum from %s!\n", 
		       in_ntoa(iph->saddr));
		return(-1);
	}

#ifdef DEBUG_CONFIG_IP_MASQUERADE
	printk("Handling forward ICMP for %lX:%X -> %lX:%X\n",
	       ntohl(ciph->saddr), ntohs(pptr[0]),
	       ntohl(ciph->daddr), ntohs(pptr[1]));
#endif

	/* This is pretty much what ip_masq_out_get() does */
	ms = ip_masq_out_get_2(ciph->protocol, 
			       ciph->daddr, 
			       pptr[1], 
			       ciph->saddr, 
			       pptr[0]);

	if (ms == NULL)
		return 0;

	/* Now we do real damage to this packet...! */
	/* First change the source IP address, and recalc checksum */
	iph->saddr = ms->maddr;
	ip_send_check(iph);
	
	/* Now change the *dest* address in the contained IP */
	ciph->daddr = ms->maddr;
	ip_send_check(ciph);
	
	/* the TCP/UDP dest port - cannot redo check */
	pptr[1] = ms->mport;

	/* And finally the ICMP checksum */
	icmph->checksum = 0;
	icmph->checksum = ip_compute_csum((unsigned char *) icmph, len);

#ifdef DEBUG_CONFIG_IP_MASQUERADE
	printk("Rewrote forward ICMP to %lX:%X -> %lX:%X\n",
	       ntohl(ciph->saddr), ntohs(pptr[0]),
	       ntohl(ciph->daddr), ntohs(pptr[1]));
#endif

	return 1;
}

/*
 *	Handle ICMP messages in reverse (demasquerade) direction.
 *	Find any that might be relevant, check against existing connections,
 *	forward to masqueraded host if relevant.
 *	Currently handles error types - unreachable, quench, ttl exceeded
 */

int ip_fw_demasq_icmp(struct sk_buff **skb_p, struct device *dev)
{
        struct sk_buff 	*skb   = *skb_p;
 	struct iphdr	*iph   = skb->h.iph;
	struct icmphdr  *icmph = (struct icmphdr *)((char *)iph + (iph->ihl<<2));
	struct iphdr    *ciph;	/* The ip header contained within the ICMP */
	__u16	        *pptr;	/* port numbers from TCP/UDP contained header */
	struct ip_masq	*ms;
	unsigned short   len   = ntohs(iph->tot_len) - (iph->ihl * 4);

#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
 	printk("MASQ: icmp in/rev (%d,%d) %lX -> %lX\n",
	        icmph->type, ntohs(icmp_id(icmph)),
 		ntohl(iph->saddr), ntohl(iph->daddr));
#endif

#ifdef CONFIG_IP_MASQUERADE_ICMP		
	if ((icmph->type == ICMP_ECHOREPLY) ||
	    (icmph->type == ICMP_TIMESTAMPREPLY) ||
	    (icmph->type == ICMP_INFO_REPLY) ||
	    (icmph->type == ICMP_ADDRESSREPLY))	{
#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
		printk("MASQ: icmp reply rcv %lX->%lX id %d type %d, req %d\n",
		       ntohl(iph->saddr),
		       ntohl(iph->daddr),
		       ntohs(icmp_id(icmph)),
		       icmph->type,
		       icmp_type_request(icmph->type));
#endif
		ms = ip_masq_in_get_2(iph->protocol,
				      iph->saddr,
				      icmp_hv_rep(icmph),
				      iph->daddr,
				      icmp_id(icmph));
		if (ms == NULL)
			return 0;

		ip_masq_set_expire(ms,0);
                
                /*
                 *	got reply, so clear flag
                 */
                ms->flags &= ~IP_MASQ_F_NO_REPLY;

		/* Reset source address */
		iph->daddr = ms->saddr;
		/* Redo IP header checksum */
		ip_send_check(iph);
		/* Set ID to fake port number */
		(icmph->un).echo.id = ms->sport;
		/* Reset ICMP checksum and set expiry */
		icmph->checksum=0;
		icmph->checksum=ip_compute_csum((unsigned char *)icmph,len);
		ip_masq_set_expire(ms, MASQUERADE_EXPIRE_ICMP);
#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
		printk("MASQ: icmp reply rwt %lX->%lX id %d type %d\n",
		       ntohl(iph->saddr),
		       ntohl(iph->daddr),
		       ntohs(icmp_id(icmph)),
		       icmph->type);
#endif
		return 1;
	} else {
#endif
		if ((icmph->type != ICMP_DEST_UNREACH) &&
		    (icmph->type != ICMP_SOURCE_QUENCH) &&
		    (icmph->type != ICMP_TIME_EXCEEDED))
			return 0;
#ifdef CONFIG_IP_MASQUERADE_ICMP
	}
#endif
	/*
	 * If we get here we have an ICMP error of one of the above 3 types
	 * Now find the contained IP header
	 */
	ciph = (struct iphdr *) (icmph + 1);

#ifdef CONFIG_IP_MASQUERADE_ICMP
	if (ciph->protocol == IPPROTO_ICMP) {
		/*
		 * This section handles ICMP errors for ICMP packets
		 *
		 * First get a new ICMP header structure out of the IP packet
		 */
		struct icmphdr  *cicmph = (struct icmphdr *)((char *)ciph + 
							     (ciph->ihl<<2));

#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
		printk("MASQ: rv icmp/icmp rcv %lX->%lX id %d type %d\n",
		       ntohl(ciph->saddr),
		       ntohl(ciph->daddr),
		       ntohs(icmp_id(cicmph)),
		       cicmph->type);
#endif
		ms = ip_masq_in_get_2(ciph->protocol, 
				      ciph->daddr, 
				      icmp_hv_req(cicmph),
				      ciph->saddr, 
				      icmp_id(cicmph));

		if (ms == NULL)
			return 0;

		/* Now we do real damage to this packet...! */
		/* First change the dest IP address, and recalc checksum */
		iph->daddr = ms->saddr;
		ip_send_check(iph);
	
		/* Now change the *source* address in the contained IP */
		ciph->saddr = ms->saddr;
		ip_send_check(ciph);

		/* Change the ID to the original one! */
		(cicmph->un).echo.id = ms->sport;

		/* And finally the ICMP checksum */
		icmph->checksum = 0;
		icmph->checksum = ip_compute_csum((unsigned char *) icmph, len);

#ifdef DEBUG_CONFIG_IP_MASQUERADE_ICMP
		printk("MASQ: rv icmp/icmp rwt %lX->%lX id %d type %d\n",
		       ntohl(ciph->saddr),
		       ntohl(ciph->daddr),
		       ntohs(icmp_id(cicmph)),
		       cicmph->type);
#endif
		return 1;
	}
#endif /* CONFIG_IP_MASQUERADE_ICMP */

	/* We are only interested ICMPs generated from TCP or UDP packets */
	if ((ciph->protocol != IPPROTO_UDP) && 
	    (ciph->protocol != IPPROTO_TCP))
		return 0;

	/* 
	 * Find the ports involved - remember this packet was 
	 * *outgoing* so the ports are reversed (and addresses)
	 */
	pptr = (__u16 *)&(((char *)ciph)[ciph->ihl*4]);
	if (ntohs(pptr[0]) < PORT_MASQ_BEGIN ||
 	    ntohs(pptr[0]) > PORT_MASQ_END)
 		return 0;

	/* Ensure the checksum is correct */
	if (ip_compute_csum((unsigned char *) icmph, len)) 
	{
		/* Failed checksum! */
		printk(KERN_DEBUG "MASQ: reverse ICMP: failed checksum from %s!\n", 
		       in_ntoa(iph->saddr));
		return(-1);
	}

#ifdef DEBUG_CONFIG_IP_MASQUERADE
 	printk("Handling reverse ICMP for %lX:%X -> %lX:%X\n",
	       ntohl(ciph->saddr), ntohs(pptr[0]),
	       ntohl(ciph->daddr), ntohs(pptr[1]));
#endif

	/* This is pretty much what ip_masq_in_get() does, except params are wrong way round */
	ms = ip_masq_in_get_2(ciph->protocol,
			      ciph->daddr,
			      pptr[1],
			      ciph->saddr,
			      pptr[0]);

	if (ms == NULL)
		return 0;

	/* Now we do real damage to this packet...! */
	/* First change the dest IP address, and recalc checksum */
	iph->daddr = ms->saddr;
	ip_send_check(iph);
	
	/* Now change the *source* address in the contained IP */
	ciph->saddr = ms->saddr;
	ip_send_check(ciph);
	
	/* the TCP/UDP source port - cannot redo check */
	pptr[0] = ms->sport;

	/* And finally the ICMP checksum */
	icmph->checksum = 0;
	icmph->checksum = ip_compute_csum((unsigned char *) icmph, len);

#ifdef DEBUG_CONFIG_IP_MASQUERADE
 	printk("Rewrote reverse ICMP to %lX:%X -> %lX:%X\n",
	       ntohl(ciph->saddr), ntohs(pptr[0]),
	       ntohl(ciph->daddr), ntohs(pptr[1]));
#endif

	return 1;
}


 /*
  *	Check if it's an masqueraded port, look it up,
  *	and send it on its way...
  *
  *	Better not have many hosts using the designated portrange
  *	as 'normal' ports, or you'll be spending many time in
  *	this function.
  */

int ip_fw_demasquerade(struct sk_buff **skb_p, struct device *dev)
{
        struct sk_buff 	*skb = *skb_p;
 	struct iphdr	*iph = skb->h.iph;
 	__u16	*portptr;
 	struct ip_masq	*ms;
	unsigned short len;
	unsigned long 	timeout;
#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW 
 	struct ip_autofw *af;
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */

	switch (iph->protocol) {
	case IPPROTO_ICMP:
		return(ip_fw_demasq_icmp(skb_p, dev));
	case IPPROTO_TCP:
	case IPPROTO_UDP:
		/* Make sure packet is in the masq range */
		portptr = (__u16 *)&(((char *)iph)[iph->ihl*4]);
		if ((ntohs(portptr[1]) < PORT_MASQ_BEGIN ||
		     ntohs(portptr[1]) > PORT_MASQ_END)
#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW 
		    && !ip_autofw_check_range(iph->saddr, portptr[1], 
					      iph->protocol, 0)
		    && !ip_autofw_check_direct(portptr[1], iph->protocol)
		    && !ip_autofw_check_port(portptr[1], iph->protocol)
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */
			)
			return 0;

		/* Check that the checksum is OK */
		len = ntohs(iph->tot_len) - (iph->ihl * 4);
		if ((iph->protocol == IPPROTO_UDP) && (portptr[3] == 0))
			/* No UDP checksum */
			break;

		switch (skb->ip_summed) 
		{
			case CHECKSUM_NONE:
				skb->csum = csum_partial((char *)portptr, len, 0);
			case CHECKSUM_HW:
				if (csum_tcpudp_magic(iph->saddr, iph->daddr, len,
						      iph->protocol, skb->csum))
				{
					printk(KERN_DEBUG "MASQ: failed TCP/UDP checksum from %s!\n", 
					       in_ntoa(iph->saddr));
					return -1;
				}
			default:
				/* CHECKSUM_UNNECESSARY */
		}
		break;
	default:
		return 0;
	}


#ifdef DEBUG_CONFIG_IP_MASQUERADE
 	printk("Incoming %s %lX:%X -> %lX:%X\n",
 		masq_proto_name(iph->protocol),
 		ntohl(iph->saddr), ntohs(portptr[0]),
 		ntohl(iph->daddr), ntohs(portptr[1]));
#endif
 	/*
 	 * reroute to original host:port if found...
         */

        ms = ip_masq_in_get(iph);

#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW 

        if (ms == NULL && (af=ip_autofw_check_range(iph->saddr, portptr[1], iph->protocol, 0))) 
	{
#ifdef DEBUG_CONFIG_IP_MASQUERADE
		printk("ip_autofw_check_range\n");
#endif
        	ms = ip_masq_new_enh(dev, iph->protocol,
        			     af->where, portptr[1],
        			     iph->saddr, portptr[0],
        			     0,
        			     portptr[1]);
        }
        if ( ms == NULL && (af=ip_autofw_check_port(portptr[1], iph->protocol)) ) 
	{
#ifdef DEBUG_CONFIG_IP_MASQUERADE
		printk("ip_autofw_check_port\n");
#endif
        	ms = ip_masq_new_enh(dev, iph->protocol,
        			     af->where, htons(af->hidden),
        			     iph->saddr, portptr[0],
        			     IP_MASQ_F_AFW_PORT,
        			     htons(af->visible));
        }
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */

        if (ms != NULL)
        {
#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW 
        	ip_autofw_update_in(iph->saddr, portptr[1], iph->protocol);
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */
        	
		/* Stop the timer ticking.... */
		ip_masq_set_expire(ms,0);

                /*
                 *	got reply, so clear flag
                 */
                ms->flags &= ~IP_MASQ_F_NO_REPLY;
                
                /*
                 *	Set dport if not defined yet.
                 */

                if ( ms->flags & IP_MASQ_F_NO_DPORT && ms->protocol == IPPROTO_TCP ) {
                        ms->flags &= ~IP_MASQ_F_NO_DPORT;
                        ms->dport = portptr[0];
#ifdef DEBUG_CONFIG_IP_MASQUERADE
                        printk("ip_fw_demasquerade(): filled dport=%d\n",
                               ntohs(ms->dport));
#endif
                }
                if (ms->flags & IP_MASQ_F_NO_DADDR && ms->protocol == IPPROTO_TCP)  {
                        ms->flags &= ~IP_MASQ_F_NO_DADDR;
                        ms->daddr = iph->saddr;
#ifdef DEBUG_CONFIG_IP_MASQUERADE
                        printk("ip_fw_demasquerade(): filled daddr=%X\n",
                               ntohs(ms->daddr));
#endif
                }
                iph->daddr = ms->saddr;
                portptr[1] = ms->sport;

                /*
                 *	Attempt ip_masq_app call.
                 *	will fix ip_masq and iph ack_seq stuff
                 */

                if (ip_masq_app_pkt_in(ms, skb_p, dev) != 0)
                {
                        /*
                         *	skb has changed, update pointers.
                         */

                        skb = *skb_p;
                        iph = skb->h.iph;
                        portptr = (__u16 *)&(((char *)iph)[iph->ihl*4]);
                        len = ntohs(iph->tot_len) - (iph->ihl * 4);
                }

                /*
                 * Yug! adjust UDP/TCP and IP checksums, also update
		 * timeouts.
		 * If a TCP RST is seen collapse the tunnel (by using short timeout)!
                 */
                if (masq_proto_num(iph->protocol)==0)
		{
                        recalc_check((struct udphdr *)portptr,iph->saddr,iph->daddr,len);
			timeout = ip_masq_expire->udp_timeout;
		}
                else
                {
			struct tcphdr *th;
                        skb->csum = csum_partial((void *)(((struct tcphdr *)portptr) + 1),
                                                 len - sizeof(struct tcphdr), 0);
                        tcp_send_check((struct tcphdr *)portptr,iph->saddr,iph->daddr,len,skb);

			/* Check if TCP FIN or RST */
			th = (struct tcphdr *)portptr;
			if (th->fin)
			{
				ms->flags |= IP_MASQ_F_SAW_FIN_IN;
			}
			if (th->rst)
			{
				ms->flags |= IP_MASQ_F_SAW_RST;
			}
			
			/* Now set the timeouts */
			if (ms->flags & IP_MASQ_F_SAW_RST)
			{
				timeout = 1;
			}
			else if ((ms->flags & IP_MASQ_F_SAW_FIN) == IP_MASQ_F_SAW_FIN)
			{
				timeout = ip_masq_expire->tcp_fin_timeout;
			}
			else timeout = ip_masq_expire->tcp_timeout;
                }
		ip_masq_set_expire(ms, timeout);
                ip_send_check(iph);
#ifdef DEBUG_CONFIG_IP_MASQUERADE
                printk("I-routed to %lX:%X\n",ntohl(iph->daddr),ntohs(portptr[1]));
#endif
                return 1;
 	}

 	/* sorry, all this trouble for a no-hit :) */
 	return 0;
}

/*
 *	/proc/net entries
 */


#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW

static int ip_autofw_procinfo(char *buffer, char **start, off_t offset,
			      int length, int unused)
{
	off_t pos=0, begin=0;
	struct ip_autofw * af;
	int len=0;
	
	len=sprintf(buffer,"Type Prot Low  High Vis  Hid  Where    Last     CPto CPrt Timer Flags\n"); 
        
        for(af = ip_autofw_hosts; af ; af = af->next)
	{
		len+=sprintf(buffer+len,"%4X %4X %04X-%04X/%04X %04X %08lX %08lX %04X %04X %6lu %4X\n",
					af->type,
					af->protocol,
					af->low,
					af->high,
					af->visible,
					af->hidden,
					ntohl(af->where),
					ntohl(af->lastcontact),
					af->ctlproto,
					af->ctlport,
					(af->timer.expires<jiffies ? 0 : af->timer.expires-jiffies), 
					af->flags);

		pos=begin+len;
		if(pos<offset) 
		{
 			len=0;
			begin=pos;
		}
		if(pos>offset+length)
			break;
        }
	*start=buffer+(offset-begin);
	len-=(offset-begin);
	if(len>length)
		len=length;
	return len;
}
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */

static int ip_msqhst_procinfo(char *buffer, char **start, off_t offset,
			      int length, int unused)
{
	off_t pos=0, begin;
	struct ip_masq *ms;
	unsigned long flags;
	char temp[129];
        int idx = 0;
	int len=0;
	
	if (offset < 128) 
	{
#ifdef CONFIG_IP_MASQUERADE_ICMP
		sprintf(temp,
			"Prc FromIP   FPrt ToIP     TPrt Masq Init-seq  Delta PDelta Expires (free=%d,%d,%d)",
			ip_masq_free_ports[0], ip_masq_free_ports[1], ip_masq_free_ports[2]); 
#else /* !defined(CONFIG_IP_MASQUERADE_ICMP) */
		sprintf(temp,
			"Prc FromIP   FPrt ToIP     TPrt Masq Init-seq  Delta PDelta Expires (free=%d,%d)",
			ip_masq_free_ports[0], ip_masq_free_ports[1]); 
#endif /* CONFIG_IP_MASQUERADE_ICMP */
		len = sprintf(buffer, "%-127s\n", temp);
	}
	pos = 128;
	save_flags(flags);
	cli();
        
        for(idx = 0; idx < IP_MASQ_TAB_SIZE; idx++)
        for(ms = ip_masq_m_tab[idx]; ms ; ms = ms->m_link)
	{
		int timer_active;
		pos += 128;
		if (pos <= offset)
			continue;

		timer_active = del_timer(&ms->timer);
		if (!timer_active)
			ms->timer.expires = jiffies;
		sprintf(temp,"%s %08lX:%04X %08lX:%04X %04X %08X %6d %6d %7lu",
			masq_proto_name(ms->protocol),
			ntohl(ms->saddr), ntohs(ms->sport),
			ntohl(ms->daddr), ntohs(ms->dport),
			ntohs(ms->mport),
			ms->out_seq.init_seq,
			ms->out_seq.delta,
			ms->out_seq.previous_delta,
			ms->timer.expires-jiffies);
		if (timer_active)
			add_timer(&ms->timer);
		len += sprintf(buffer+len, "%-127s\n", temp);

		if(len >= length)
			goto done;
        }
done:
	restore_flags(flags);
	begin = len - (pos - offset);
	*start = buffer + begin;
	len -= begin;
	if(len>length)
		len = length;
	return len;
}

/*
 *	Initialize ip masquerading
 */
#ifdef	CONFIG_OSFMACH3
/*
 * Work-around some invalid code "optimizations": 
 * the proc_dir_entries below are stored in a read-only data section...
 */
#ifdef	CONFIG_PROC_FS
static int __proc_net_register(struct proc_dir_entry * dp)
{
	return proc_net_register(dp);
}
#endif
#define proc_net_register(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
{ \
	static struct proc_dir_entry xxx; \
	xxx = *(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10); \
	__proc_net_register(&xxx); \
}
#endif	/* CONFIG_OSFMACH3 */

int ip_masq_init(void)
{
        register_symtab (&ip_masq_syms);
#ifdef CONFIG_PROC_FS        
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_IPMSQHST, 13, "ip_masquerade",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		ip_msqhst_procinfo
	});
#ifdef CONFIG_IP_MASQUERADE_IPAUTOFW
	proc_net_register(&(struct proc_dir_entry) {
		PROC_NET_IPAUTOFW, 9, "ip_autofw",
		S_IFREG | S_IRUGO, 1, 0, 0,
		0, &proc_net_inode_operations,
		ip_autofw_procinfo
	});
#endif /* CONFIG_IP_MASQUERADE_IPAUTOFW */
#endif	
        ip_masq_app_init();

        return 0;
}
