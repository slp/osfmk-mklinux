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
 * This file describes the interface with the MCP 3.00
 */

#ifndef _CHIPS_MYRICOM_NET_MYRICOM_MCP3_H_
#define	_CHIPS_MYRICOM_NET_MYRICOM_MCP3_H_

#include <mach/boolean.h>
#include <device/if_myrinet.h>
#include <device/net_device.h>
#include <device/net_status.h>
#include <device/data_device.h>
#include <machine/endian.h>
#include <busses/iobus.h>
#include <kern/queue.h>
#include <chips/myricom/net_myricom.h>

/* MCP program to load */
#define	NET_MYRICOM_MCP3_LANAI_LOAD(version)	"mcp-l" # version "v3.boot"

/*
 * MCP3 configuration
 */
#define	NET_MYRICOM_MCP3_NO_CHECKSUM	  -1	/* No checksum on packet */
#define	NET_MYRICOM_MCP3_MULTICAST_MAX	   8	/* Max Multicast items */
#define	NET_MYRICOM_MCP3_GATHER_MAX	  16	/* Max Gather items */
#define	NET_MYRICOM_MCP3_SCATTER_MAX	   8	/* Max Scatter items */
#define	NET_MYRICOM_MCP3_SENDQ_MAX	  10	/* Max Send items */
#define	NET_MYRICOM_MCP3_RECVQ_MAX	  20	/* Max Receive items */
#define	NET_MYRICOM_MCP3_CHANNELS_MAX	   3	/* Max Channels */
#define	NET_MYRICOM_MCP3_ALIGNMENT	   4	/* MCP buffer alignement */
#define NET_MYRICOM_MCP3_L2_BASE     0x10000	/* LANai 2/3 shmem base */
#define NET_MYRICOM_MCP3_L4_BASE     0x1E000	/* LANai 4 shmem base */
#define	NET_MYRICOM_MCP3_SHAKE_TRIES	 100	/* # handshake trials */
#define	NET_MYRICOM_MCP3_SLEEP_USEC	  50	/* # handshake wait usecs */

/*
 * Generic memory item
 */
typedef	IOBUS_VAL32_T net_myricom_mcp3_32_t;

#define	net_myricom_mcp3_32_val(val)		(htonl(val))
#define	net_myricom_mcp3_32_get(self)		(ntohl(*(self)))
#define	net_myricom_mcp3_32_sram_bug_get(val)	(ntohs(((short *)(val))[1]))
#define	net_myricom_mcp3_32_set(self, val)	(*(self) = htonl(val))

#define	net_myricom_mcp3_32_sram_bug_set(self, val)	\
    ((short *)(self))[1] = htons((short)(val))

#define	net_myricom_mcp3_32_inc(self)			\
MACRO_BEGIN						\
    int _i = ntohl(*(self));				\
    *(self) = htonl(_i + 1);				\
MACRO_END

#define	net_myricom_mcp3_32_dec(self)			\
MACRO_BEGIN						\
    int _i = ntohl(*(self));				\
    *(self) = htonl(_i - 1);				\
MACRO_END
    
/*
 * Myrinet address
 */
typedef volatile unsigned char net_myricom_mcp3_addr_t[IF_MYRINET_MID_SIZE];

#define	net_myricom_mcp3_addr_multicast(self)	((self)[1] & 0xF)

#define	net_myricom_mcp3_addr_set(addr1, addr2)				\
MACRO_BEGIN								\
    unsigned int _i;							\
    assert((IF_MYRINET_MID_SIZE % sizeof (unsigned int)) == 0);		\
    for (_i = 0; _i < IF_MYRINET_MID_SIZE / sizeof (unsigned int); _i++)\
	((unsigned int *)(addr1))[_i] = ((unsigned int *)(addr2))[_i];	\
MACRO_END

extern boolean_t net_myricom_mcp3_addr_broadcast(net_myricom_mcp3_addr_t);
extern int net_myricom_mcp3_addr_compare(net_myricom_mcp3_addr_t,
					 net_myricom_mcp3_addr_t);

/*
 * Generic pair
 */
typedef volatile struct net_myricom_mcp3_pair {
    net_myricom_mcp3_addr_t	pa_address;	/* Myrinet address */
    net_myricom_mcp3_32_t	pa_value;	/* Associated value */
} net_myricom_mcp3_pair_t;

/*
 * Generic list
 */
typedef volatile struct net_myricom_mcp3_list {
    net_myricom_mcp3_32_t	ls_max;		/* Max length */
    net_myricom_mcp3_32_t	ls_length;	/* Current length */
    net_myricom_mcp3_32_t	ls_cache;	/* Item cache */
} net_myricom_mcp3_list_t;

extern boolean_t net_myricom_mcp3_list_search(net_myricom_mcp3_list_t *,
					      net_myricom_mcp3_addr_t,
					      unsigned *);
extern boolean_t net_myricom_mcp3_list_get(net_myricom_mcp3_list_t *,
					   net_myricom_mcp3_addr_t,
					   int *,
					   unsigned *);
extern boolean_t net_myricom_mcp3_list_put(net_myricom_mcp3_list_t *,
					   net_myricom_mcp3_addr_t,
					   int);
extern boolean_t net_myricom_mcp3_list_remove(net_myricom_mcp3_list_t *,
					      net_myricom_mcp3_addr_t);

/*
 * Multicast list
 */
typedef volatile struct net_myricom_mcp3_mcast {
    net_myricom_mcp3_list_t	mc_list;	/* Generic list */
    net_myricom_mcp3_pair_t	mc_buffer[NET_MYRICOM_MCP3_MULTICAST_MAX];
						/* Multicast item */
    net_myricom_mcp3_addr_t	mc_broadcast;	/* Broadcast address */
} net_myricom_mcp3_mcast_t;

/*
 * Gather Item
 */
typedef volatile struct net_myricom_mcp3_gather {
    net_myricom_mcp3_32_t	ga_pointer;	/* Gather pointer */
    net_myricom_mcp3_32_t	ga_length;	/* gather length */
} net_myricom_mcp3_gather_t;

/*
 * Send Item
 */
typedef volatile struct net_myricom_mcp3_sendi {
    net_myricom_mcp3_gather_t	si_gathers[NET_MYRICOM_MCP3_GATHER_MAX];
						/* Gather items */
    net_myricom_mcp3_32_t	si_num;		/* Gather number */
    net_myricom_mcp3_addr_t	si_address;	/* Destination address */
    net_myricom_mcp3_32_t	si_channel;	/* Destination channel */
    net_myricom_mcp3_32_t	si_length;	/* Packet length */
    net_myricom_mcp3_32_t	si_offset;	/* Checksum offset */
    net_myricom_mcp3_32_t	si_correction;	/* Checksum correction */
} net_myricom_mcp3_sendi_t;

/*
 * Send Queue
 */
typedef volatile struct net_myricom_mcp3_sendq {
    net_myricom_mcp3_32_t	sq_tail;	/* Queue tail */
    net_myricom_mcp3_32_t	sq_head;	/* Queue head */
    net_myricom_mcp3_32_t	sq_host;	/* Host debug */
    net_myricom_mcp3_32_t	sq_mcp;		/* MCP debug */
    net_myricom_mcp3_sendi_t	sq_items[NET_MYRICOM_MCP3_SENDQ_MAX];
						/* Send items */
} net_myricom_mcp3_sendq_t;

extern unsigned int net_myricom_mcp3_sendq_peekput(net_myricom_mcp3_sendq_t *);

extern void net_myricom_mcp3_sendq_put(net_myricom_mcp3_sendq_t *);

extern boolean_t net_myricom_mcp3_sendq_free(net_myricom_mcp3_sendq_t *,
					     unsigned int);

/*
 * Receive Item
 */
typedef volatile struct net_myricom_mcp3_recvi {
    net_myricom_mcp3_gather_t	ri_scatters[NET_MYRICOM_MCP3_SCATTER_MAX];
						/* Scatter items */
    net_myricom_mcp3_32_t	ri_checksum;	/* Scatter checksum */
    net_myricom_mcp3_32_t	ri_context;	/* Buffer context */
    net_myricom_mcp3_32_t	ri_num;		/* Scatter number */
} net_myricom_mcp3_recvi_t;
    
/*
 * Receive Queue
 */
typedef volatile struct net_myricom_mcp3_recvq {
    net_myricom_mcp3_32_t	rq_head;	/* Queue head */
    net_myricom_mcp3_32_t	rq_tail;	/* Queue tail */
    net_myricom_mcp3_32_t	rq_host;	/* Host debug */
    net_myricom_mcp3_32_t	rq_mcp;		/* MCP debug */
    net_myricom_mcp3_recvi_t	rq_items[NET_MYRICOM_MCP3_RECVQ_MAX + 1];
						/* Receive items */
} net_myricom_mcp3_recvq_t;

extern void net_myricom_mcp3_recvq_put(net_myricom_mcp3_recvq_t *,
				       net_myricom_mcp3_recvi_t *);

extern void net_myricom_mcp3_recvq_get(net_myricom_mcp3_recvq_t *,
				       net_myricom_mcp3_recvi_t *);

extern boolean_t net_myricom_mcp3_recvq_full(net_myricom_mcp3_recvq_t *);

extern boolean_t net_myricom_mcp3_recvq_empty(net_myricom_mcp3_recvq_t *);

/*
 * Channel status
 */
typedef enum net_myricom_mcp3_state {
    NET_MYRICOM_MCP3_STATE_HOST,	/* Waiting for Host */
    NET_MYRICOM_MCP3_STATE_NET,		/* Waiting for Net */
    NET_MYRICOM_MCP3_STATE_READY	/* Ready */
} net_myricom_mcp3_state_t;

/*
 * Channel Item
 */
typedef volatile struct net_myricom_mcp3_channel {
    net_myricom_mcp3_state_t	ch_state;	/* Channel state */
    net_myricom_mcp3_32_t	ch_busy;	/* Channel is busy */
    net_myricom_mcp3_sendq_t	ch_send;	/* Send queue */
    net_myricom_mcp3_recvq_t	ch_recv;	/* Receive queue */
    net_myricom_mcp3_recvq_t	ch_recvack;	/* Receive Ack queue */
    net_myricom_mcp3_32_t	ch_recvbytes;	/* Bytes received */
    net_myricom_mcp3_32_t	ch_sendbytes;	/* Bytes sent */
    net_myricom_mcp3_32_t	ch_recvmsgs;	/* Messages received */
    net_myricom_mcp3_32_t	ch_sendmsgs;	/* Messages sent */
    net_myricom_mcp3_mcast_t	ch_multicast;	/* Multicast list */
} net_myricom_mcp3_channel_t;

/*
 * Shared Memory Item
 */
typedef volatile struct net_myricom_mcp3_shmem {
    net_myricom_mcp3_addr_t	sh_address;	/* MCP address */
    net_myricom_mcp3_32_t	sh_num;		/* Channel number */
    net_myricom_mcp3_32_t	sh_burst;	/* Host DMA Burst size */
    net_myricom_mcp3_32_t	sh_shake;	/* Handshake wanted */
    net_myricom_mcp3_32_t	sh_send;	/* Send wanted */
    net_myricom_mcp3_32_t	sh_itmask;	/* Interrupt mask */
    net_myricom_mcp3_32_t	sh_maplevel;	/* Map level */
    net_myricom_mcp3_32_t	sh_debug[4];	/* Debug */
    net_myricom_mcp3_channel_t	sh_channel[1];	/* Channels (actually longer) */
} net_myricom_mcp3_shmem_t;

#define	NET_MYRICOM_MCP3_GETCHANNEL(shmem, chan)			\
    ((vm_offset_t)&((net_myricom_mcp3_shmem_t *)(shmem))->sh_channel[(chan)])

#define	NET_MYRICOM_MCP3_SETCHANNEL(netp, myri, chan, num)		\
MACRO_BEGIN								\
    vm_size_t _len;							\
    if ((myri)->myri_flags & NET_MYRICOM_MCP3_FLAGS_MAPMEM)		\
	(chan) = &(myri)->myri_mapchan[(num)];				\
    else if ((myri)->myri_chan == (num))				\
	(chan) = (myri)->myri_mapchan;					\
    else {								\
	_len = sizeof (*(chan));					\
	(myri)->myri_mapchan =						\
	    (net_myricom_mcp3_channel_t *)(*(myri)->myri_memmap)	\
		((netp), NET_MYRICOM_MCP3_GETCHANNEL((myri)->myri_mem,	\
						     (num)), &_len);	\
	assert(_len >= sizeof (*(chan)));				\
	(myri)->myri_mapchan = (chan);					\
	(myri)->myri_chan = (num);					\
    }									\
MACRO_END

/*
 * Host programming interface
 */
#define NET_MYRICOM_HPI_WAS_RESET(chan)					\
    (net_myricom_mcp3_32_get(&chan->ch_state) != NET_MYRICOM_MCP3_STATE_READY)

#define	NET_MYRICOM_HPI_WAKE_SEND(netp, myri)				\
    (*(myri)->myri_wakeup)((netp),					\
			   ((vm_offset_t)&((net_myricom_mcp3_shmem_t *)	\
					   (myri)->myri_mem)->sh_send),	\
			   net_myricom_mcp3_32_val(1))

#define	NET_MYRICOM_HPI_WAKE_SHAKE(netp, myri)				\
    (*(myri)->myri_wakeup)((netp),					\
			   ((vm_offset_t)&((net_myricom_mcp3_shmem_t *)	\
					   (myri)->myri_mem)->sh_shake),\
			   net_myricom_mcp3_32_val(1))

/*
 * MCP private structure in net_done elements
 */
typedef struct net_myricom_mcp3_done {
    unsigned char		mcpd_dmid[IF_MYRINET_MID_SIZE]; /* Dst UID */
    unsigned int		mcpd_schannel;	/* Source channel */
    unsigned int		mcpd_dchannel;	/* Destination channel */
    unsigned int		mcpd_cksum;	/* IP checksum offset */
    unsigned int		mcpd_flags;	/* Flags */
    vm_size_t			mcpd_rest;	/* Current unhandled size */
    int				mcpd_num;	/* Current # gather elements */
    vm_offset_t			mcpd_virt[NET_MYRICOM_MCP3_GATHER_MAX];
    vm_offset_t			mcpd_phys[NET_MYRICOM_MCP3_GATHER_MAX];
    vm_size_t			mcpd_size[NET_MYRICOM_MCP3_GATHER_MAX];
    int				mcpd_intnum;	/* Current # Internal buffers */
    vm_size_t			mcpd_intsize;	/* Internal buffer length */
    vm_offset_t			mcpd_intvirt[1];/* Internal DMA buffers */
    /* Must be last field because it may be longer than one element */
} *net_myricom_mcp3_done_t;
#define	NET_MYRICOM_MCP3_DONE_INTERNAL	0x01	/* last buf used was internal */
#define	NET_MYRICOM_MCP3_DONE_VIRTUAL	0x02	/* last addr seen was virtual */

/*
 * MCP private structure as NIC net_device
 */
typedef struct net_myricom_mcp3 {
    IOBUS_TYPE_T		  myri_iotype;	/* I/O bus address */
    IOBUS_ADDR_T		  myri_iobase;	/* I/O base address */
    unsigned int		  myri_lanai;	/* LANai id */
    unsigned int		  myri_ramsize;	/* Ram size */
    unsigned char		  myri_uid[IF_MYRINET_UID_SIZE];
						/* Myricom UID */
    IOBUS_SHMEM_T		(*myri_regmap)(net_device_t,
					       vm_offset_t,
					       vm_size_t *);
    						/* lANai register map */
    IOBUS_SHMEM_T		(*myri_memmap)(net_device_t,
					       vm_offset_t,
					       vm_size_t *);
    						/* Shared memory map */
    void			(*myri_control)(net_device_t,
						enum net_myricom_flavor);
						/* Control LANai chip */
    void			(*myri_wakeup)(net_device_t,
					       vm_offset_t,
					       unsigned int);
						/* Wakeup LANai chip */
    struct net_myrinet_statistics myri_stats;	/* Myrinet specific stats */
    unsigned int		  myri_status;	/* EBUS DMA status */
    unsigned int		  myri_flags;	/* flags */
    unsigned int		  myri_numchan;	/* # Myrinet channels */
    char			 *myri_mcp;	/* MCP program to load */
    vm_offset_t			  myri_mem;	/* Myrinet on-board memory */
    volatile unsigned int	 *myri_mapreg;	/* LANai regs mapped addr */
    unsigned int		  myri_reg;	/* Current LANai reg mapped */
    net_myricom_mcp3_channel_t	 *myri_mapchan;	/* Current channel pointer */
    unsigned int		  myri_chan;	/* Current channel mapped */
    struct datadev		  myri_datadev;	/* Data_device structure */
    net_done_t			  myri_netd[NET_MYRICOM_MCP3_CHANNELS_MAX *
					   NET_MYRICOM_MCP3_SENDQ_MAX];
						/* netd send list */
    int				  myri_module[NET_MYRICOM_MCP3_CHANNELS_MAX];
						/* Module associated */
    unsigned int		  myri_index[NET_MYRICOM_MCP3_CHANNELS_MAX];
						/* Head of myri_context */
    vm_offset_t			  myri_context[NET_MYRICOM_MCP3_CHANNELS_MAX *
					      (NET_MYRICOM_MCP3_RECVQ_MAX + 1)];
						/* Input context array */
} *net_myricom_mcp3_t;

#define	NET_MYRICOM_MCP3_FLAGS_READY	0x01	/* LANai ready to work */
#define	NET_MYRICOM_MCP3_FLAGS_MAPMEM	0x02	/* shmem completely mapped */
#define	NET_MYRICOM_MCP3_FLAGS_MAPREG	0x04	/* regs completely mapped */

extern net_device_t net_myricom_mcp3_attach(IOBUS_TYPE_T,
					    IOBUS_ADDR_T,
					    struct net_myricom_eeprom *,
					    unsigned int,
					    void (*)(net_device_t,
						     enum net_myricom_flavor),
					    void (*)(net_device_t,
						     vm_offset_t,
						     unsigned int),
					    IOBUS_SHMEM_T (*)(net_device_t,
							      unsigned int,
							      unsigned int *),
					    IOBUS_SHMEM_T (*)(net_device_t,
							      vm_offset_t,
							      vm_size_t *));

#endif /* _CHIPS_MYRICOM_NET_MYRICOM_MCP3_H_ */
