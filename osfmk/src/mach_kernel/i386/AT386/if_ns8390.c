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
/* CMU_HIST */
/*
 * Revision 2.14.2.8  92/09/15  17:15:19  jeffreyh
 * 	Do better checking for bogus recv packets
 * 	[92/07/20            sjs]
 * 
 * Revision 2.14.2.7  92/06/24  17:58:52  jeffreyh
 * 	Fix to be able to compile STD+COMPAQ (Single cpu)
 * 	[92/06/09            bernadat]
 * 
 * 	Updated to new at386_io_lock() interface.
 * 	[92/06/03            bernadat]
 * 
 * Revision 2.14.2.6  92/05/27  00:37:43  jeffreyh
 * 	Fixed merge problem with packet_drop not being
 * 	a defined symbol.
 * 
 * Revision 2.14.2.5  92/05/26  11:43:03  jeffreyh
 * 	   Drop packets with unreasonable data packet lengths.
 * 	[92/05/15            sjs]
 * 
 * Revision 2.14.2.4  92/04/30  11:55:49  bernadat
 * 	Adaptations for Corollary and Systempro.
 * 	To many problems with NORMA_MK8 version, Use NORMA_MK7 version
 * 	of wd80xxget_board_id().
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.14.2.3  92/03/28  10:06:26  jeffreyh
 * 	Changes from MK71
 * 	[92/03/20  12:15:23  jeffreyh]
 * 
 * Revision 2.15  92/02/19  15:08:22  elf
 * 	Made wd driver work with 16 bit cards. Better recognize different
 * 	cards. Print card name when probing. Tested on wd8003, wd8013EP,
 * 	wd8003EP.
 * 	Add IFWD_ prefix to defines.
 * 	[92/01/20            kivinen]
 * 
 * Revision 2.14.2.2  92/02/18  18:52:38  jeffreyh
 * 	Fixed probe code to check if board is configured
 * 	[91/08/22            bernadat]
 * 
 * 	Support for Corollary MP, switch to master for i386 ports R/W
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.14.2.1  92/01/09  18:43:41  jsb
 * 	Fixes to really make two wd boards work (with help from jefferyh).
 * 	[92/01/09  13:07:14  jsb]
 * 
 * Revision 2.14  91/11/12  11:09:39  rvb
 * 	Undo "strict" wrong change to probe()
 * 	[91/10/25            rvb]
 * 
 * Revision 2.13  91/10/09  16:07:43  af
 * 	 Revision 2.12.1.1  91/09/03  17:28:50  af
 * 	 	Fixes from 2.5 (from rvb), made sure two WD boards work (with
 * 	 	help from jeffreyh).
 * 
 * Revision 2.12  91/08/24  11:58:01  af
 * 	New MI autoconf.
 * 	[91/08/02  02:55:17  af]
 * 
 * Revision 2.11  91/05/14  16:24:56  mrt
 * 	Correcting copyright
 * 
 * Revision 2.10  91/05/13  06:02:41  af
 * 	Made code under CMUCS standard.
 * 	[91/05/12  15:50:35  af]
 * 
 * Revision 2.9  91/03/16  14:46:23  rpd
 * 	Changed net_filter to net_packet.
 * 	[91/01/15            rpd]
 * 
 * Revision 2.8  91/02/14  14:42:44  mrt
 * 	Distinguish EtherLinkII vs WD8003 on open.  Get packet
 * 	size right for statistics.  Fix 3.0 buf that sometimes
 * 	reported packets too large.
 * 	[91/01/28  15:31:22  rvb]
 * 
 * Revision 2.7  91/02/05  17:17:52  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:44:04  mrt]
 * 
 * Revision 2.6  91/01/09  16:07:19  rpd
 * 	Fixed typo in ns8390probe.
 * 	[91/01/09            rpd]
 * 
 * Revision 2.5  91/01/08  17:35:46  rpd
 * 	Changed NET_KMSG_GET to net_kmsg_get.
 * 	[91/01/08            rpd]
 * 
 * Revision 2.4  91/01/08  17:33:05  rpd
 * 	A few bug fixes.
 * 	[91/01/08  16:41:04  rvb]
 * 
 * 	Make this a generic driver for ns8390 from wd8003 because
 * 	we now will also support etherlink ii.
 * 	[91/01/04  12:25:21  rvb]
 * 
 * Revision 2.2  90/10/01  14:23:09  jeffreyh
 * 	Changes for MACH_KERNEL. 
 * 	initial checkin.
 * 	[90/09/27  18:22:09  jeffreyh]
 *
 * Revision 2.12.1.1  91/09/03  17:28:50  af
 * 	Fixes from 2.5 (from rvb), made sure two WD boards work (with
 * 	help from jeffreyh).
 * 
 * Revision 2.1.1.7  90/11/27  13:43:18  rvb
 * 	Synched 2.5 & 3.0 at I386q (r2.1.1.7) & XMK35 (r2.3)
 * 	[90/11/15            rvb]
 * 
 *
 * Revision 2.1.1.6  90/09/18  08:38:39  rvb
 * 	Unfortunately, the switches to bank 0 seem necessary so are back
 * 	in.
 * 	[90/09/08            rvb]
 * 
 * Revision 2.1.1.5  90/08/25  15:42:58  rvb
 * 	Use take_<>_irq() vs direct manipulations of ivect and friends.
 * 	[90/08/20            rvb]
 * 
 * 	Flush unnecessary switches to bank 0. Use error counters
 * 	vs printouts.  Fix DSF_RUNNING.  Some more cleanup.
 * 	[90/08/14            rvb]
 * 
 * Revision 2.1.1.4  90/07/28  10:00:40  rvb
 * 	Get correct counter offsets
 * 
 * Revision 2.1.1.3  90/07/27  17:16:05  rvb
 * 	No multicast for now.
 * 
 * Revision 2.1.1.2  90/07/27  11:25:11  rvb
 * 	Add boardID support for wd80xxyyy family of boards.	[rweiss]
 * 	Bunch of cleanup and ...		[rweiss&rvb]
 * 
 * Revision 2.1.1.1  90/07/10  11:44:46  rvb
 * 	Added to system.
 * 	[90/07/06            rvb]
 *
 *
 *   Author: Ron Weiss (rweiss)
 *
 */
/* CMU_ENDHIST */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
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
 */

/*
 * NS8390-base boards Mach Ethernet driver (for intel 80386)
 * Copyright (c) 1990 by Open Software Foundation (OSF).
 */

#include 	<platforms.h>
#include	<cpus.h>
#include	<ns8390.h>
#include	<xkmachkernel.h>
#include	<xk_debug.h>
#include 	<mp_v1_1.h>

#include	<kern/kalloc.h>
#include	<kern/time_out.h>
#include	<kern/spl.h>
#include	<kern/misc_protos.h>
#include	<device/device_types.h>
#include	<device/errno.h>
#include	<device/io_req.h>
#include	<device/if_hdr.h>
#include	<device/if_ether.h>
#include	<device/net_status.h>
#include	<device/ds_routines.h>
#include	<device/net_io.h>
#include	<device/conf.h>
#include	<i386/endian.h>
#include	<i386/AT386/mp/mp.h>

#if	NCPUS > 1

#if	CBUS
#include	<busses/cbus/cbus.h>
#endif	/* CBUS */

#if	MBUS
#include	<busses/mbus/mbus.h>
struct mp_dev_lock		ns8390_dev_lock[NNS8390];
#define	at386_io_lock(op)	mbus_dev_lock(&ns8390_dev_lock[unit], (op))
#define	at386_io_unlock()	mbus_dev_unlock(&ns8390_dev_lock[unit])
#endif	/* MBUS */

#if	MP_V1_1
#include <i386/AT386/mp/mp_v1_1.h>
#endif	/* MP_V1_1 */

#endif	/* NCPUS > 1 */

#include	<i386/ipl.h>
#include	<i386/pio.h>
#include	<chips/busses.h>
#include	<i386/AT386/if_ns8390.h>
#include	<i386/AT386/if_wd8003.h>
#include	<i386/AT386/if_3c503.h>
#include	<i386/AT386/if_ns8390_entries.h>
#include	<i386/AT386/misc_protos.h>


#if XKMACHKERNEL

#include 	<xkern/include/xkernel.h>
#include	<xkern/include/msg.h>
#include	<xkern/framework/msg_internal.h>
#include	<xkern/protocols/eth/eth_i.h>
#include	<xkern/include/trace.h>
#include	<xkern/include/list.h>
#include        <uk_xkern/include/eth_support.h>
#include        <vm/pmap.h>

/*
 * When used with promiscous traffic,
 * x-kernel traffic is identified by
 * the following interval.
 */
#define XK_LOWEST_TYPE  0x3000
#define XK_HIGHEST_TYPE 0x3fff

#if     XK_DEBUG
int     tracexkwdp;
#endif /* XK_DEBUG */

XObj self_protocol[NNS8390];

#if XKSTATS
int  xk_eth_error;
#endif /* XKSTATS */
#endif /* XKMACHKERNEL */

/*
 * Switch between x-kernel traffic and regular traffic
 */
#if XKMACHKERNEL
#define   IS_XKERNEL(X, is)\
                ((is->xk_lowest_type) <= (X) && (X) < (is->xk_highest_type))
#else  /* XKMACHKERNEL */
#define   IS_XKERNEL(X, is)     0
#endif /* XKMACHKERNEL */

#include 	<device/eth_common.h>	/* machine-independent utilities */

int wd_debug = 0;

struct	bus_device	*ns8390info[NNS8390];	/* ???? */

static caddr_t ns8390_std[NNS8390] = { 0 };
static struct bus_device *ns8390_info[NNS8390];

char *wd8003_card = "wd";
char *elii_card = "el";
/*		  2e0, 2a0, 280,  250,   350,   330,   310, 300*/
int elii_irq[8] = {5,   2,   2, 0x711, 0x711, 0x711, 0x711,  5};
int elii_bnc[8] = {1,   0,   1, 0x711, 0x711, 0x711, 0x711,  1};
/*int elii_bnc[8] = {0, 1,   1,     1,     1,     1,     0,  1}; */

#define	NS8390_TX_MIN		((ETHERMTU + sizeof(struct ether_header) + 0xFF) >> 8)
				 	/* transmit minimum # buffers */
#define	NS8390_TX_RATIO		0x2000
					/* NS8390_TX_MIN buffers per 8K */
#define	NS8390_TX_SIZE(a)	(((a) / NS8390_TX_RATIO) * NS8390_TX_MIN)
					/* transmit # buffers formula */
#define	NS8390_TX_MAX		NS8390_TX_SIZE(0x10000)
					/* transmit maximum # buffers */

#define	NS8390_MULTICAST_SIZE	8	/* size of multicast filter array */

typedef struct {
	struct ifnet	ds_if;		/* generic interface header */
	timer_elt_data_t timer;		/* timeout element */
	int     	interrupt;
	i386_ioport_t	nic;
	u_char		address[ETHER_ADDR_SIZE];
	int     	tbusy;
	char    	*sram;		/* beginning of RAM shared memory */
	int     	read_nxtpkt_ptr;/* pointer to next packet available */
	int     	pstart;		/* page start hold */
	int     	pstop;		/* page stop hold */
	int     	tpsr;		/* transmit page start hold */
	int     	fifo_depth;	/* NIC fifo threshold */
	char		*card;
	int		board_id;
	unsigned	dev_open;	/* number of opened devices */
	queue_head_t	multicast_queue;/* multicast hardware addr list */
	unsigned char	multicast[NS8390_MULTICAST_SIZE];
					/* multicast address array */
#if XKMACHKERNEL
	boolean_t xk_up;
	u_short xk_lowest_type, xk_highest_type; /* x-kernel traffic space */
#endif /* XKMACHKERNEL */
	unsigned	time_out;	/* time_out timestamp */
	unsigned	time_xmt;	/* start of transmission timestamp */
	unsigned	time_txt;	/* end of transmission timestamp */
	int		tx_curr;	/* current transmit buffer */
	int		tx_free;	/* first available transmit buffer */
	int		tx_size;	/* transmit buffer size */
	io_req_t tx_map[NS8390_TX_MAX];	/* transmit buffer usage map */
}
ns8390_softc_t;

ns8390_softc_t	ns8390_softc[NNS8390];

struct ns8390_cntrs {
u_int	ovw,
	jabber,
	crc,
	frame,
	miss,
	fifo,
	rcv;
u_int	xmt,
	xmti,
	busy,
	heart,
	reset,
	xmtp,
	xmta,
	coll,
	deadlock;
} ns8390_cntrs[NNS8390];

int  imr_hold = PRXE|PTXE|RXEE|TXEE|OVWE|CNTE;  /* Interrupts mask bits */
  
/* 
 * Forward declarations
 */
int ns8390probe(
	int			port,
	struct bus_device	*dev);

void ns8390attach(
	struct bus_device	*dev);

void ns8390timeout(
	int	unit);

void ns8390watchdog(
	void	*softc);

int ns8390init(
	int	unit);

void ns8390start(
	int	unit);

int ns8390hwrst(
	int	unit);

void ns8390txt(
	int	unit,
	int     status);

void ns8390over_write(
	int	unit);

int ns8390rcv(
	int	unit);

void ns8390send_packet_up(
	io_req_t		m,
	struct ether_header	*eh,
	ns8390_softc_t		*is,
	int			unit);

void ns8390lost_frame(
	int	unit);

int ns8390get_CURR(
	int	unit);

void ns8390xmt(
	int		unit);

boolean_t config_nic(
	int	unit);

boolean_t config_wd8003(
	int	unit);

boolean_t config_3c503(
	int	unit);

void ns8390intoff(
	int	unit);

unsigned long wd80xxget_eeprom_info(
     i386_ioport_t	hdwbase,
     long		board_id);

long wd80xxget_board_id(
	struct bus_device	*dev);

void en_16bit_access(
	i386_ioport_t	hdwbase,
	long		board_id);

void dis_16bit_access(
	i386_ioport_t	hdwbase,
	long		board_id);

boolean_t wd80xxcheck_for_585(
	i386_ioport_t hdwbase);

void ns8390_multicast_create(
	net_multicast_t *cur);

void ns8390_multicast_compute(
	ns8390_softc_t	*sp);

#if     XKMACHKERNEL

static boolean_t
xkcopy(
	char 		*src,
	long		length,
	void		*arg);

static boolean_t
xkcopy_test(
	    char 		*src,
	    long		length,
	    void		*arg);

static boolean_t
xkcopy16(
	char 		*src,
	long		length,
	void		*arg);

static boolean_t
xkcopy16_test(
	      char 		*src,
	      long		length,
	      void		*arg);

boolean_t
xkcopy_to_device(
	register io_req_t       ior,
	char			*physaddr,
	i386_ioport_t 		hdwbase,
	int     		board_id);

xkern_return_t
xkwd_openenable(
	XObj self, 
	XObj hlp, 
	XObj hlptype,
	Part part);

io_return_t
xknet_write(
	register struct ifnet *ifp,
	void		(*start)(int unit),
	io_req_t	ior);

boolean_t
free_ior(
	io_req_t ior);

xmsg_handle_t
xkwd_push(
	XObj s,
	Msg downMsg);

int
xkwd_control(
	XObj  self,
	int   op,
	char *buf,
	int   len);

xkern_return_t
xkwd_init(
	XObj self);

#endif   /* XKMACHKERNEL */

struct	bus_driver	ns8390driver =
	{(probe_t) ns8390probe, 0, ns8390attach, 0, ns8390_std, "ns8390", ns8390_info, 0, 0, 0};


/*
 * ns8390probe:
 *
 *	This function "probes" or checks for the wd8003 board on the bus to see
 *	if it is there.  As far as I can tell, the best break between this
 *	routine and the attach code is to simply determine whether the board
 *      is configured in properly. Currently my approach to this is to test the
 *      base I/O special offset for the Western Digital unique byte sequence
 *      identifier.  If the bytes match we assume board is there.
 *	The config code expects to see a successful return from the probe
 *	routine before attach will be called.
 *
 * input	: address device is mapped to, and unit # being checked
 * output	: a '1' is returned if the board exists, and a 0 otherwise
 *
 */

int
ns8390probe(
	int			port,
	struct bus_device	*dev)
{
	i386_ioport_t	hdwbase = (long)dev->address;
	int		unit = dev->unit;
	ns8390_softc_t	*sp = &ns8390_softc[unit];
	int		tmp;

	if ((unit < 0) || (unit >= NNS8390)) {
		printf("ns8390 ethernet unit %d out of range\n", unit);
		return(0);
	}
	if (((u_char) inb(hdwbase+IFWD_LAR_0) == (u_char) WD_NODE_ADDR_0) &&
	    ((u_char) inb(hdwbase+IFWD_LAR_1) == (u_char) WD_NODE_ADDR_1) &&
	    ((u_char) inb(hdwbase+IFWD_LAR_2) == (u_char) WD_NODE_ADDR_2)) {
			ns8390info[unit] = dev;
			sp->card = wd8003_card;
			dev->name = wd8003_card;
			sp->nic = hdwbase + OFF_8390;
				/* enable mem access to board */
			sp->board_id = wd80xxget_board_id(dev);

			*(sp->address)      = inb(hdwbase+IFWD_LAR_0);
			*(sp->address + 1)  = inb(hdwbase+IFWD_LAR_1);
			*(sp->address + 2)  = inb(hdwbase+IFWD_LAR_2);
			*(sp->address + 3)  = inb(hdwbase+IFWD_LAR_3);
			*(sp->address + 4)  = inb(hdwbase+IFWD_LAR_4);
			*(sp->address + 5)  = inb(hdwbase+IFWD_LAR_5);
			return (1);
		}  /* checks the address of the board to verify that it is a WD */
	if (tmp = inb(hdwbase+BCFR)) {
		switch(tmp) {
		case (1<<7):	sp->board_id = 7; break;	/*irq5 xvcr*/
#ifdef	not_currently_possible
		case (1<<6):	sp->board_id = 6; break;
		case (1<<5):	sp->board_id = 5; break;
		case (1<<4):	sp->board_id = 4; break;
		case (1<<3):	sp->board_id = 3; break;
#endif	/* not_currently_possible */
		case (1<<2):	sp->board_id = 2; break;	/*irq2 bnc*/
		case (1<<1):	sp->board_id = 1; break;	/*irq2 xvcr*/
		case (1<<0):	sp->board_id = 0; break;	/*irq5 bnc*/
		default:	return 0;
		}
		switch (inb(hdwbase+PCFR)) {
		case (1<<7):	dev->phys_address = (caddr_t) 0xDC000; break;
		case (1<<6):	dev->phys_address = (caddr_t) 0xD8000; break;
#ifdef	not_currently_possible
		case (1<<5):	dev->phys_address = (caddr_t) 0xCC000; break;
		case (1<<4):	dev->phys_address = (caddr_t) 0xC8000; break;
#endif	/* not_currently_possible */
		default:
			printf("EtherLink II with NO memory configured\n");
			return 0;
		}
		ns8390info[unit] = dev;
		dev->sysdep1 = elii_irq[sp->board_id];
		if (dev->sysdep1 == 2)
			dev->sysdep1 = 9;
		sp->card = elii_card;
		dev->name = elii_card;
		sp->nic = hdwbase;
		return 1;
	}
	return(0);
}

/*
 * ns8390attach:
 *
 *	This function attaches a ns8390 board to the "system".  The rest of
 *	runtime structures are initialized here (this routine is called after
 *	a successful probe of the board).  Once the ethernet address is read
 *	and stored, the board's ifnet structure is attached and readied.
 *
 * input	: bus_device structure setup in autoconfig
 * output	: board structs and ifnet is setup
 *
 */

void
ns8390attach(
	struct bus_device	*dev)
{
	ns8390_softc_t	*sp;
	struct	ifnet	*ifp;
	u_char		unit;
	int             temp;
	unsigned int	i;

	take_dev_irq(dev);
	unit = (u_char)dev->unit;
	sp = &ns8390_softc[unit];

#if	MBUS && NCPUS > 1
	simple_lock_init(&ns8390_dev_lock[unit].lock, ETAP_IO_DEVINS);
	ns8390_dev_lock[unit].unit = unit;
	ns8390_dev_lock[unit].pending_ops = 0;
	ns8390_dev_lock[unit].op[MP_DEV_OP_START] = ns8390start;
	ns8390_dev_lock[unit].op[MP_DEV_OP_INTR] = ns8390intr;
	ns8390_dev_lock[unit].op[MP_DEV_OP_TIMEO] = ns8390timeout;
#endif	/* MBUS && NCPUS > 1 */
	printf(", port = %x, spl = %d, pic = %d. ",
		dev->address, dev->sysdep, dev->sysdep1);

	if (sp->card == elii_card) {
		if (elii_bnc[sp->board_id])
			printf("cheapernet ");
		else
			printf("ethernet ");
	} else
		printf("ethernet ");

	sp->sram = (char *) phystokv(dev->phys_address);
	dev->address = (char *) phystokv(dev->address);
	queue_init(&sp->multicast_queue);
	for (i = 0; i < NS8390_MULTICAST_SIZE; i++)
		sp->multicast[i] = '\0';

	if (!ns8390hwrst(unit)) {
		printf("%s%d: attach(): reset failed.\n",
		       sp->card, unit);
		return;
	}

	printf("id [%x:%x:%x:%x:%x:%x]",
		sp->address[0],sp->address[1],sp->address[2],
		sp->address[3],sp->address[4],sp->address[5]);
	ifp = &(sp->ds_if);
	ifp->if_unit = unit;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST;
	ifp->if_header_size = sizeof(struct ether_header);
	ifp->if_header_format = HDR_ETHERNET;
	ifp->if_address_size = 6;
	ifp->if_address = (char *)&sp->address[0];
	if_init_queues(ifp);
	if (ifp->if_snd.ifq_maxlen > sp->tx_size)
		ifp->if_snd.ifq_maxlen = sp->tx_size;
}

/*
 * ns8390timeout:
 *
 *	This function is called either by ns8390timeout function, or
 *	by the at386_io_lock mechanism with spl and lock held.
 */
void
ns8390timeout(
	int		unit)
{
	ns8390_softc_t	*is = &ns8390_softc[unit];

	if (is->time_xmt == is->time_txt) {
		/*
		 * Set up last txt seen.
		 */
		is->time_out = is->time_txt;

	} else if (is->time_out > is->time_txt) {
		/*
		 * NIC chip is in transmitter deadlock condition.
		 */
		ns8390_cntrs[unit].deadlock++;
		(void)ns8390hwrst(unit);

	} else if (is->time_out == is->time_txt) {
		/*
		 * Set up deadlock condition.
		 */
		is->time_out++;

	} else {
		/*
		 * Set up last txt seen.
		 */
		is->time_out = is->time_txt;
	}
	
	/*
	 * Rearm timer if needed.
	 */
	if (is->time_xmt != is->time_txt && !is->timer.set)
		set_timeout(&is->timer, NIC_TIMEOUT * hz);
}
/*
 * ns8390watchdog:
 *
 *	This function is waken up every NIC_TIMEOUT seconds in order to
 *	verify that the transmitter is not in deadlock condition.
 */
void
ns8390watchdog(
	void		*sp)
{
	spl_t		oldpri;
	int		unit = (ns8390_softc_t *)sp - ns8390_softc;
	at386_io_lock_state();

	oldpri = splimp();
	if (at386_io_lock(MP_DEV_OP_TIMEO)) {
		ns8390timeout(unit);
		at386_io_unlock();
	}
	splx(oldpri);
}

io_return_t
wd8003open(
	dev_t		dev,
	dev_flavor_t	flag,
	io_req_t	ior)
{
	register int	unit = minor(dev);

	if (ns8390_softc[unit].card != wd8003_card)
		return (ENXIO);
	if (unit < 0 || unit >= NNS8390 ||
		ns8390_softc[unit].nic == 0)
	    return (ENXIO);

#if XKMACHKERNEL
	if (ns8390_softc[unit].xk_up &&
	    ns8390_softc[unit].xk_lowest_type != XK_LOWEST_TYPE) {
	    /*
	     * x-kernel traffic has to respect its boundaries!!
	     */
	    ns8390_softc[unit].xk_lowest_type = XK_LOWEST_TYPE;
	    ns8390_softc[unit].xk_highest_type = XK_HIGHEST_TYPE;
	}
#endif /* XKMACHKERNEL */
	ns8390init(unit);
	return(D_SUCCESS);
}

io_return_t
eliiopen(
	dev_t		dev,
	dev_flavor_t	flag,
	io_req_t	ior)
{
	register int	unit = minor(dev);

	if (ns8390_softc[unit].card != elii_card)
		return (ENXIO);
	if (unit < 0 || unit >= NNS8390 ||
		ns8390_softc[unit].nic == 0)
	    return (ENXIO);

	if (ns8390init(unit))
	    return (D_SUCCESS);
	return (ENXIO);
}

void
ns8390close(
	dev_t		dev)
{
	register int	unit = minor(dev);
	spl_t		oldpri;
	at386_io_lock_state();

	oldpri = splimp();
	at386_io_lock(MP_DEV_WAIT);

	assert((ns8390_softc[unit].ds_if.if_flags & IFF_UP) &&
	       ns8390_softc[unit].dev_open > 0);
	if (--ns8390_softc[unit].dev_open == 0)
	    ns8390_softc[unit].ds_if.if_flags &= ~IFF_UP;

	at386_io_unlock();
	splx(oldpri);
}

io_return_t
ns8390devinfo(
	dev_t		dev,
	dev_flavor_t	flavor,
	char		*info)
{
	switch (flavor) {
	case D_INFO_CLONE_OPEN:
		*((boolean_t *) info) = TRUE;
		break;
	default:
		return (D_INVALID_OPERATION);
	}
	return (D_SUCCESS);
}

io_return_t
ns8390output(
	dev_t		dev,
	io_req_t	ior)
{
	register int	unit = minor(dev);

	if (unit < 0 || unit >= NNS8390 ||
		ns8390_softc[unit].nic == 0)
	    return (ENXIO);
	return (net_write(&ns8390_softc[unit].ds_if, ns8390start, ior));
}

io_return_t
ns8390setinput(
	dev_t		dev,
	ipc_port_t	receive_port,
	int		priority,
	filter_t	filter[],
	unsigned int	filter_count,
	device_t	device)
{
	register int unit = minor(dev);

	if (unit < 0 || unit >= NNS8390 ||
		ns8390_softc[unit].nic == 0)
	    return (ENXIO);

	return (net_set_filter(&ns8390_softc[unit].ds_if,
			receive_port, priority,
			filter, filter_count, device));
}

/*
 * ns8390init:
 *
 *	Another routine that interfaces the "if" layer to this driver.
 *	Simply resets the structures that are used by "upper layers".
 *	As well as calling ns8390hwrst that does reset the ns8390 board.
 *
 * input	: board number
 * output	: structures (if structs) and board are reset
 *
 */

int
ns8390init(
	int	unit)
{
	ns8390_softc_t	*is;
	struct	ifnet	*ifp;
	int		stat;
	spl_t		oldpri;
	at386_io_lock_state();

	is = &ns8390_softc[unit];
	ifp = &is->ds_if;

	oldpri = splimp();
	at386_io_lock(MP_DEV_WAIT);

	if ((stat = ns8390hwrst(unit)) == TRUE) {
		ifp->if_flags |= IFF_RUNNING | IFF_UP;
		ns8390start(unit);
		ns8390_softc[unit].dev_open++;
	} else
		printf("ns8390init(): trouble resetting board %s%d\n",
		       is->card, unit);

	at386_io_unlock();
	splx(oldpri);
	return(stat);
}

/*
 * ns8390start:
 *
 *	This is yet another interface routine that simply tries to output a
 *	in an mbuf after a reset.
 *
 * input	: board number
 * output	: stuff sent to board if any there
 *
 */

void
ns8390start(
	int	unit)
{
	register ns8390_softc_t *is = &ns8390_softc[unit];
	struct	ifnet		*ifp;
	io_req_t	m;
	at386_io_lock_state();

	if (!at386_io_lock(MP_DEV_OP_START))
		return;

	/*
	 * New transmission algorithm :
	 *	Instead of filling on-board shared memory at each new packet
	 *	to be sent, write first packet, start I/O and then try to copy
	 *	more packets into shared memory. So that at next End-of-Tx
	 *	interrupt, a new packet will be ready to be sent immediately.
	 */
	ifp = &ns8390_softc[unit].ds_if;
	for (;;) {
		int i, j, new;
		volatile char *sram;
		spl_t s;

		if (!is->tbusy && is->tx_map[is->tx_curr] != (io_req_t)0) {
			ns8390_cntrs[unit].xmt++;
			ns8390xmt(unit);
		}
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == 0)
			break;
		if (m->io_count > (NS8390_TX_MIN << 8)) {
			printf("ns8390start: Too long packet (size = %d)\n",
			       m->io_count);
			iodone(m);
			continue;
		}
		if (is->tbusy)
			ns8390_cntrs[unit].xmtp++;
		j = (m->io_count + 0xFF) >> 8;
		new = is->tx_free;
		if (new % NS8390_TX_MIN &&
		    (new % NS8390_TX_MIN) + j >= NS8390_TX_MIN) {
			new += NS8390_TX_MIN - (new % NS8390_TX_MIN);
			for (i = is->tx_free; i < new; i++)
				if (is->tx_map[i])
					break;
			if (i < new) {
				IF_PREPEND(&ifp->if_snd, m);
				break;
			}
			if (new >= is->tx_size)
				new = 0;
		}
		for (i = 0; i < j; i++)
			if (is->tx_map[new + i])
				break;
		if (i < j) {
			IF_PREPEND(&ifp->if_snd, m);
			break;
		}
		if (is->tbusy)
			ns8390_cntrs[unit].xmta++;
		else if (is->tx_free != new)
			is->tx_curr = new;
		is->tx_map[new] = m;
		if (new + j >= is->tx_size)
			is->tx_free = 0;
		else
			is->tx_free = new + j;

		sram = is->sram + ((is->tpsr + new) << 8);
#if     XKMACHKERNEL
        	if (m->io_op & IO_IS_XK_MSG) {
            	    if (!xkcopy_to_device(m, (char *) sram, 
					  (i386_ioport_t) (long) ns8390info[unit]->address, is->board_id)) {
			is->tx_map[new] = (io_req_t)0;
			iodone(m);
#if XKSTATS
			xk_eth_error++;
#endif /* XKSTATS */
			continue;
		    }
		} else  
#endif /* XKMACHKERNEL */
		    if (is->board_id & IFWD_SLOT_16BIT) {
			s = splhi();
			en_16bit_access((i386_ioport_t) (long) ns8390info[unit]->address, is->board_id);
			bcopy16((char *) m->io_data, (char *) sram,  m->io_count);
			dis_16bit_access((i386_ioport_t) (long) ns8390info[unit]->address, is->board_id);
			splx(s);
		     } else
			bcopy((char *) m->io_data, (char *) sram,  m->io_count);

		for (i = m->io_count; i < ETHERMIN + sizeof (struct ether_header); i++)
			*(sram + i) = 0;
	}
	at386_io_unlock();
}

/*ARGSUSED*/
io_return_t
ns8390getstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	status,		/* pointer to OUT array */
	natural_t	*count)		/* out */
{
	register int	unit = minor(dev);

	if (unit < 0 || unit >= NNS8390 ||
		ns8390_softc[unit].nic == 0)
	    return (ENXIO);

	return (net_getstat(&ns8390_softc[unit].ds_if,
			    flavor,
			    status,
			    count));
}
io_return_t
ns8390setstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	status,
	natural_t	count)
{
	register int	unit = minor(dev);
	register ns8390_softc_t *sp;
	spl_t		oldpri;
	unsigned	i;
	at386_io_lock_state();

	if (unit < 0 || unit >= NNS8390 ||
		ns8390_softc[unit].nic == 0)
	    return (ENXIO);

	sp = &ns8390_softc[unit];

	switch (flavor) {
	    case NET_STATUS:
	    {
		/*
		 * All we can change are flags, and not many of those.
		 */
		register struct net_status *ns = (struct net_status *)status;
		boolean_t setup_mcast;

		if (count < NET_STATUS_COUNT)
		    return (D_INVALID_SIZE);

		/*
		 * Force a complete reset if the receive mode changes
		 * so that these take effect immediately.
		 */
		oldpri = splimp();
		at386_io_lock(MP_DEV_WAIT);
		if ((sp->ds_if.if_flags & IFF_CANTCHANGE) !=
		    (ns->flags & ~IFF_CANTCHANGE)) {
		    /*
		     * Manage PROMISCUOUS setup.
		     */
		    setup_mcast = ((ns->flags & IFF_PROMISC) !=
				   (sp->ds_if.if_flags & IFF_PROMISC));
		    sp->ds_if.if_flags &= IFF_CANTCHANGE;
		    sp->ds_if.if_flags |= ns->flags & ~IFF_CANTCHANGE;

		    if (setup_mcast)
			ns8390_multicast_compute(sp);

		    if (ns8390hwrst(unit)) {
			sp->ds_if.if_flags |= IFF_RUNNING;
			ns8390start(unit);
		    } else
			printf("ns8390setstat(): "
			       "trouble resetting board %s%d\n",
			       sp->card, unit);
		}
		at386_io_unlock();
		splx(oldpri);
		break;
	    }

	    case NET_ADDMULTI:
	    {
		net_multicast_t	*new;
		unsigned temp;
		boolean_t allmulti;

		if (count != ETHER_ADDR_SIZE * 2)
		    return (KERN_INVALID_VALUE);
		if ((((unsigned char *)status)[0] & 1) == 0 ||
		    ((unsigned char *)status)[6] !=
		    ((unsigned char *)status)[0])
		    return (KERN_INVALID_VALUE);

		temp = NET_MULTICAST_LENGTH(ETHER_ADDR_SIZE);
		if ((sp->board_id & IFWD_NIC_690_BIT) == 0)
		    temp += NS8390_MULTICAST_SIZE;
		new = (net_multicast_t *)kalloc(temp);
		new->size = temp;

		net_multicast_create(new, (unsigned char *)status,
				     &((unsigned char *)
				       status)[ETHER_ADDR_SIZE],
				     ETHER_ADDR_SIZE);
		if ((sp->board_id & IFWD_NIC_690_BIT) == 0)
		    ns8390_multicast_create(new);

		oldpri = splimp();
		at386_io_lock(MP_DEV_WAIT);
		net_multicast_insert(&sp->multicast_queue, new);
		temp = inb(sp->nic + CR) & 0x3F;
		if ((sp->board_id & IFWD_NIC_690_BIT) == 0) {
		    allmulti = TRUE;
		    outb(sp->nic + CR, temp | PS1);
		    for (i = 0; i < NS8390_MULTICAST_SIZE; i++) {
			sp->multicast[i] |= NET_MULTICAST_MISC(new)[i];
			outb(sp->nic + MAR0 + i, sp->multicast[i]);
			if (allmulti && sp->multicast[i] != 0xFF)
			    allmulti = FALSE;
		    }
		    if (allmulti && (sp->ds_if.if_flags & IFF_ALLMULTI) == 0)
			sp->ds_if.if_flags |= IFF_ALLMULTI;
		}
		outb(sp->nic + CR, temp | PS0);
		if (sp->ds_if.if_flags & IFF_PROMISC)
		    outb(sp->nic + RCR, AB|AM|PRO);
		else
		    outb(sp->nic + RCR, AB|AM);
		at386_io_unlock();
		splx(oldpri);
		break;
	    }

	    case NET_DELMULTI:
	    {
		net_multicast_t	*cur;
		unsigned temp;

		if (count != ETHER_ADDR_SIZE * 2)
		    return (KERN_INVALID_VALUE);

		oldpri = splimp();
		at386_io_lock(MP_DEV_WAIT);
		cur = net_multicast_remove(&sp->multicast_queue,
					   (unsigned char *)status,
					   &((unsigned char *)
					     status)[ETHER_ADDR_SIZE],
					   ETHER_ADDR_SIZE);
		if (cur == (net_multicast_t *)0) {
		    at386_io_unlock();
		    splx(oldpri);
		    return (KERN_INVALID_VALUE);
		}
		ns8390_multicast_compute(sp);
		temp = inb(sp->nic + CR) & 0x3F;
		if ((sp->board_id & IFWD_NIC_690_BIT) == 0) {
		    outb(sp->nic + CR, temp | PS1);
		    for (i = 0; i < NS8390_MULTICAST_SIZE; i++)
			outb(sp->nic + MAR0 + i, sp->multicast[i]);
		}
		outb(sp->nic + CR, temp | PS0);
		if (sp->ds_if.if_flags & IFF_PROMISC)
		    outb(sp->nic + RCR, AB|AM|PRO);
		else if (!queue_empty(&sp->multicast_queue))
		    outb(sp->nic + RCR, AB|AM);
		else
		    outb(sp->nic + RCR, AB);
		at386_io_unlock();
		splx(oldpri);

		kfree((vm_offset_t)cur, cur->size);
		break;
	    }

	    default:
		return (D_INVALID_OPERATION);
	}
	return (D_SUCCESS);
}

/*
 * ns8390hwrst:
 *
 *	This routine resets the ns8390 board that corresponds to the
 *	board number passed in.
 *
 * input	: board number to do a hardware reset
 * output	: board is reset
 *
 */

int
ns8390hwrst(
	int		unit)
{
	int		i;
	ns8390_softc_t *is = &ns8390_softc[unit];

	ns8390_cntrs[unit].reset++;
	for (i = 0; i < is->tx_size; i++)
		if (i != is->tx_curr && is->tx_map[i] != (io_req_t)0)
			iodone(is->tx_map[i]);

	if ((is->board_id & IFWD_NIC_790_BIT) == 0) {
		if (is->time_xmt != is->time_txt)
			(void)reset_timeout(&is->timer);
		is->time_out = is->time_xmt = is->time_txt = 0;
	}
	is->tbusy = 0;
	
	if (is->card == wd8003_card && config_wd8003(unit) == FALSE) {
		printf("%s%d hwrst(): config_wd8003 failed.\n",
		       is->card, unit);
		return(FALSE);
	}
	if (is->card == elii_card &&  config_3c503(unit) == FALSE) {
		printf("%s%d hwrst(): config_3c503 failed.\n",
		       is->card, unit);
		return(FALSE);
	}
	if (config_nic(unit) == FALSE) {
		printf("%s%d hwrst(): config_nic failed.\n",
		       is->card, unit);
		return(FALSE);
	}

	return(TRUE);
}

/*
 * ns8390txt:
 *
 *	This function deals with packet end of transmission.
 */

void
ns8390txt(
	int	unit,
	int     status)
{
	ns8390_softc_t *is = &ns8390_softc[unit];
	int tsr;

	if (is->tbusy != 1) {
		printf("ns8390txt on non-busy interface %d ???\n", unit);
		return;
	}

	tsr = inb(is->nic + TSR);
	if ((is->board_id & IFWD_NIC_790_BIT) == 0)
		tsr &= ~0x2;	/* unadvertised special */

	if (tsr & COL)
		ns8390_cntrs[unit].coll += inb(is->nic + NCR) & 0xF;
	if ((status & TXE) == 0)
		ns8390_cntrs[unit].xmti++;
	else if (tsr == (CDH|ABT))
		ns8390_cntrs[unit].heart++;
	else
		printf("%s%d ns8390txt(): isr = %x, TSR = %x\n",
		       is->card, unit, status, tsr);

	is->tx_map[is->tx_curr] = (io_req_t)0;
	do {
		if (++is->tx_curr == is->tx_size)
			is->tx_curr = 0;
	} while (is->tx_curr != is->tx_free &&
		 is->tx_map[is->tx_curr] == (io_req_t)0);

	is->tbusy = 0;
	if (is->time_out <= is->time_txt)
		is->time_txt++;
	else
		is->time_txt = is->time_out + 1;
}

/*
 * ns8390intr:
 *
 *	This function is the interrupt handler for the ns8390 ethernet
 *	board.  This routine will be called whenever either a packet
 *	is received, or a packet has successfully been transfered and
 *	the unit is ready to transmit another packet.
 *
 * input	: board number that interrupted
 * output	: either a packet is received, or a packet is transfered
 *
 */
void
ns8390intr(
	int	unit)
{
	int	opri, i;
	int     isr_status;
	int     temp_cr;
	i386_ioport_t nic = ns8390_softc[unit].nic;
	at386_io_lock_state();

	if (!at386_io_lock(MP_DEV_OP_INTR))
		return;

	temp_cr = inb(nic+CR) & ~TXP;
	outb(nic+CR, (temp_cr & 0x3f) | PS0);
	outb(nic+IMR, 0);                   /* stop board interrupts */
	outb(nic+CR, temp_cr);
	while (isr_status = inb(nic+ISR)) {
		outb(nic+ISR, isr_status);          /* clear interrupt status */

		/*
		 * It seems to be better to handle first End-of-Transmit
		 *	interrupts to start immediately another transmission
		 *	before dealing with received packets.
		 */
		if (isr_status & (TXE|PTX)) {
			ns8390txt(unit, isr_status);
	 		ns8390start(unit);
		}

		if ((isr_status & (OVW|RXE)) == RXE) {
			int rsr = inb(nic+RSR);
			if (rsr & DFR) ns8390_cntrs[unit].jabber++;
			if (rsr & ~(DFR|PHY|FAE|CRC|PRX))
				printf("%s%d intr(): isr = %x, RSR = %x\n",
					ns8390_softc[unit].card, unit,
					isr_status, rsr);
		} else if (isr_status & OVW) {
			ns8390_cntrs[unit].ovw++;
			ns8390over_write(unit);
		}
		if (isr_status & PRX)	/* DFR & PRX is possible */
		 	ns8390rcv(unit);

		if (isr_status & CNT) {
			int c0 = inb(nic+CNTR0);
			int c1 = inb(nic+CNTR1);
			int c2 = inb(nic+CNTR2);
			ns8390_cntrs[unit].frame += c0;
			ns8390_cntrs[unit].crc += c1;
			ns8390_cntrs[unit].miss += c2;
#ifdef	COUNTERS
			printf("%s%d %s: isr = %x, FRAME %x, CRC %x, MISS %x\n",
				ns8390_softc[unit].card, unit, "ns8390intr()",
				isr_status, c0, c1, c2);
			printf("%s%d %s: TOTAL   , FRAME %x, CRC %x, MISS %x\n",
				ns8390_softc[unit].card, unit, "ns8390intr()",
				ns8390_cntrs[unit].frame,
				ns8390_cntrs[unit].crc,
				ns8390_cntrs[unit].miss);
#endif	/* COUNTERS */
			outb(nic+ISR, CNT); /* clear interrupt status again */
		}
	}
	temp_cr=inb(nic+CR) & ~TXP;
	outb(nic+CR, (temp_cr & 0x3f) | PS0);
	outb(nic+IMR, imr_hold);
	outb(nic+CR, temp_cr);
	at386_io_unlock();
	return;
}

/*
 * Called if on board buffer has been completely filled by ns8390intr.
 * It stops the board, reads in all the buffers that are currently in
 * the buffer, and then restarts the board.
 */
void
ns8390over_write(
	int	unit)
{
	ns8390_softc_t *is = &ns8390_softc[unit];
	i386_ioport_t nic = is->nic;
	int no;
	int count;
	int temp_cr;

	if ((is->board_id & (IFWD_NIC_690_BIT|IFWD_NIC_790_BIT)) == 0) {
		assert((inb(nic+CR) & STP) == 0);
		outb(nic+CR, ABR|STP|PS0);	/* clear the receive buffer */
		outb(nic+RBCR0, 0);
		outb(nic+RBCR1, 0);

		if ((inb(nic + ISR) & RST) == 0) {
			/*
			 * The NIC is guaranteed to be reset after the longest
			 * packet time (1500bytes = 1.2ms).
			 */
			delay(1200);
		}
		no = ns8390rcv(unit);
#ifdef	OVWBUG
		printf("%s%d over_write(): ns8390 OVW ... %d.\n",
		       is->card, unit, no);
#endif	/* OVWBUG */
		outb(nic+TCR, LB1);               /* External loopback mode */
		outb(nic+CR, ABR|STA|PS0);
		outb(nic+TCR, 0);

	} else if ((is->board_id & IFWD_NIC_690_BIT) &&
		    is->read_nxtpkt_ptr == ns8390get_CURR(unit)) {
		temp_cr = inb(nic+CR) & ~TXP;
		outb(nic+CR, (temp_cr & 0x3f) | PS0);
		outb(nic+BNDY, inb(nic+BNDY));
		outb(nic+CR, temp_cr);
	} else
		no = ns8390rcv(unit);
}

/*
 * ns8390rcv:
 *
 *	This routine is called by the interrupt handler to initiate a
 *	packet transfer from the board to the "if" layer above this
 *	driver.  This routine checks if a buffer has been successfully
 *	received by the ns8390.  If so, it does the actual transfer of the
 *      board data (including the ethernet header) into a packet (consisting
 *      of an mbuf chain) and enqueues it to a higher level.
 *      Then check again whether there are any packets in the receive ring,
 *      if so, read the next packet, until there are no more.
 *
 * input	: number of the board to check
 * output	: if a packet is available, it is "sent up"
 */
int
ns8390rcv(
	int	unit)
{
	register ns8390_softc_t	*is = &ns8390_softc[unit];
	register struct ifnet	*ifp = &is->ds_if;
	i386_ioport_t		nic = is->nic;
	int			packets = 0;
	header_t		header_support;
	header_t		*hs = &header_support;
	struct ether_header	*ehp;
	u_short			mlen, len, bytes_in_mbuf, bytes;
        u_short			eth_type;
	u_short			remaining;
	int			temp_cr;
	u_char			*mb_p;
	int                     board_id = is->board_id;
	i386_ioport_t	hdwbase = (long)ns8390info[unit]->address;
	spl_t			s;
	boolean_t		is_xk;

	/* calculation of pkt size */
	int nic_overcount;         /* NIC says 1 or 2 more than we need */
	int pkt_size;              /* calculated size of received data */
	int wrap_size;             /* size of data before wrapping it */
	int header_nxtpkt_ptr;     /* NIC's next pkt ptr in rcv header */
	int low_byte_count;        /* low byte count of read from rcv header */
	int high_byte_count;       /* calculated high byte count */
	int status;                /* received status */

	volatile char *sram_nxtpkt_ptr;   /* mem location of next packet */
	volatile char *sram_getdata_ptr;  /* next location to be read */
	ipc_kmsg_t	new_kmsg;
	struct packet_header *pkt;
	char *base, *header, *body;

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		ns8390intoff(unit);
		return -1;
	}

	while(is->read_nxtpkt_ptr != ns8390get_CURR(unit))   {

		/* while there is a packet to read from the buffer */

		if ((is->read_nxtpkt_ptr < is->pstart) ||
		    (is->read_nxtpkt_ptr >= is->pstop)) {
			ns8390hwrst(unit);
			return -1;
		}    /* if next packet pointer is out of receive ring bounds */

		packets++;
		ns8390_cntrs[unit].rcv++;
		sram_nxtpkt_ptr = (char *) (is->sram + (is->read_nxtpkt_ptr << 8));

		/* get packet size and location of next packet */
		header_nxtpkt_ptr = *(sram_nxtpkt_ptr + 1);
		header_nxtpkt_ptr &= 0xFF;
		low_byte_count = *(sram_nxtpkt_ptr + 2);
		low_byte_count &= 0xFF;

		if ((low_byte_count + NIC_HEADER_SIZE) > NIC_PAGE_SIZE)
			nic_overcount = 2;
		else
			nic_overcount = 1;
		if (header_nxtpkt_ptr > is->read_nxtpkt_ptr) {
			wrap_size = 0;
			high_byte_count = header_nxtpkt_ptr - is->read_nxtpkt_ptr -
			    nic_overcount;
		} else {
			wrap_size = (int) (is->pstop - is->read_nxtpkt_ptr - nic_overcount);
			high_byte_count = is->pstop - is->read_nxtpkt_ptr +
			    header_nxtpkt_ptr - is->pstart - nic_overcount;
		}
		pkt_size = (high_byte_count << 8) | (low_byte_count & 0xFF);
		/* does not seem to include NIC_HEADER_SIZE */
		len = pkt_size;

		sram_getdata_ptr = sram_nxtpkt_ptr + NIC_HEADER_SIZE;
		if (board_id & IFWD_SLOT_16BIT) {
			s = splhi();
			en_16bit_access(hdwbase, board_id);
			bcopy16 ((char *) sram_getdata_ptr,
				 (char *) hs,
				 sizeof(header_t));
			dis_16bit_access (hdwbase, board_id);
			splx(s);
		} else {
			bcopy ((char *) sram_getdata_ptr,
			       (char *) hs,
			       sizeof(header_t));
		}		    
		sram_getdata_ptr += sizeof(struct ether_header);
		/*
		 * Note that size in bcopy differs from 
		 * sizeof(struct ether_header): we want to
                 * peek at the bytes beyond the header.
		 * These extra bytes (6) will get re-bcopied 
 		 * once the eth_resources are located.
		 */ 
		len -= (sizeof(struct ether_header) + 4);	/* crc size */
		/*
		* on the 3COM cards we occasionally see bogus lengths:
		* do a simple check and bail out if too big ( > 32K)
		* This needs to be revisited if we get a card with
		* a buffer larger then 32k
		*/
	        wrap_size = (is->sram + (is->pstop << 8) - sram_getdata_ptr);
                if ((short)len < 0 || len - wrap_size > 32768) {
                    is->ds_if.if_rcvdrops++;
                    ns8390lost_frame(unit);
                    return packets;
	        }
		ehp = (struct ether_header *)hs;
                /*
                 * Look if this packet is to be accepted.
                 */
                if ((ehp->ether_dhost[0] & 1) && /* multicast/broadcast */
                    (ehp->ether_dhost[0] != 0xFF ||
                     ehp->ether_dhost[1] != 0xFF ||
                     ehp->ether_dhost[2] != 0xFF ||
                     ehp->ether_dhost[3] != 0xFF ||
                     ehp->ether_dhost[4] != 0xFF ||
                     ehp->ether_dhost[5] != 0xFF) &&
                    net_multicast_match(&is->multicast_queue,
                                        ehp->ether_dhost, ETHER_ADDR_SIZE
                                        ) == (net_multicast_t *)0) {
                        ns8390lost_frame(unit);
                        continue;
                }

		eth_type = ntohs(ehp->ether_type);
		is_xk = IS_XKERNEL(eth_type, is);
		if (eth_resources(is_xk, hs, eth_type, len, &base, &header, &body) 
			!= KERN_SUCCESS) {
		    /*
		     * Drop the packet.
		     */
		    is->ds_if.if_rcvdrops++;    
		    ns8390lost_frame(unit);
		    return packets;/* packets;*/
		}
		*(struct ether_header *)header = *ehp;
		if (len > wrap_size) {
		    /* if needs to wrap */
		    if (board_id & IFWD_SLOT_16BIT) {
			s = splhi();
			en_16bit_access(hdwbase, board_id);
			bcopy16 ((char *) sram_getdata_ptr, body,
				 wrap_size);
			dis_16bit_access (hdwbase, board_id);
			splx(s);
		    } else {
			bcopy ((char *) sram_getdata_ptr, body,
			       wrap_size);
		    }
		    sram_getdata_ptr = (volatile char *)
			    (is->sram + (is->pstart << 8));
		} else {   
		    /* normal getting data from buffer */
		    wrap_size = 0;
		}
		if (board_id & IFWD_SLOT_16BIT) {
		    s = splhi();
		    en_16bit_access(hdwbase, board_id);
		    bcopy16 ((char *) sram_getdata_ptr,
		   	     body + wrap_size,
			     len - wrap_size);
		    dis_16bit_access (hdwbase, board_id);
		    splx(s);
		} else {
		    bcopy ((char *) sram_getdata_ptr,
		           body + wrap_size,
			   len - wrap_size);
		}
		is->read_nxtpkt_ptr = *(sram_nxtpkt_ptr + 1);
		is->read_nxtpkt_ptr &= 0xFF;

		temp_cr = inb(nic+CR) & ~TXP;
		outb(nic+CR, (temp_cr & 0x3f) | PS0);

		if (is->read_nxtpkt_ptr == is->pstart) {
		    outb(nic+BNDY, is->pstop - 1);
		} else {
		    outb(nic+BNDY, is->read_nxtpkt_ptr - 1);
		}
		outb(nic+CR, temp_cr);

	  	/*
	   	 * Hand the packet to the above layers.
		 * Clone it if needed.
	   	 */
		eth_dispatch(is_xk, ifp, eth_type, len, unit, base, (io_req_t)0);
#if XKMACHKERNEL
		/*
		 * Few packet types require the PDU to be cloned 
		 * (e.g., ARP) for x-kernel's consumption.
		 */
		if (eth_tobecloned(is_xk, eth_type))  {
		    struct ether_header *new_header;
		    char *new_body;

		    if (eth_resources(TRUE, hs, eth_type, len, &base, (char **) &new_header, &new_body) 
			              == KERN_SUCCESS) {
		
			*new_header = *(struct ether_header *)header;
			memcpy(new_body, (const char *)body, len);
			eth_dispatch(TRUE, ifp, eth_type, len, unit, base,
				     (io_req_t)0);
		    }
		}
#endif /* XKMACHKERNEL */
	}
	return packets;
}

/*
 * Send a packet back to higher levels.
 */
void
ns8390send_packet_up(
	io_req_t		m,
	struct ether_header	*eh,
	ns8390_softc_t		*is,
	int			unit)
{
	ipc_kmsg_t kmsg;
	struct ether_header *ehp;
	struct packet_header *pkt;
	char *base, *header, *body;
	unsigned short	len;
	unsigned short type = ntohs(eh->ether_type);
	boolean_t is_xk = IS_XKERNEL(type, is);

	len = m->io_count - sizeof(struct ether_header);

	if (eth_resources(is_xk, (header_t *)eh, type, len, &base, &header, &body) 
			!= KERN_SUCCESS) {
	    /*
	     * Drop the packet.
	     */
	    is->ds_if.if_rcvdrops++;
	    return;
	}
#if     XKMACHKERNEL
        if (m->io_op & IO_IS_XK_MSG) {
	    eh = (struct ether_header *)msgGetAttr((Msg)m->io_data, 0);
	    bcopy((char *)eh, header, sizeof(struct ether_header));
	    msgForEachAlternate((Msg)m->io_data, (XCharFun)xkcopy, (XCharFun)xkcopy_test, (void *) &body);
	} else
#endif  /* XKMACHKERNEL */ 
	{    
	    bcopy((char *)eh, header, sizeof(struct ether_header));
	    bcopy((char *) &m->io_data[sizeof(struct ether_header)], body, len);
	}
	eth_dispatch(is_xk, &is->ds_if, type, len, unit, base, m); 
}

/*
 *  ns8390lost_frame:
 *  this routine called by ns8390read after memory for mbufs could not be
 *  allocated.  It sets the boundary pointers and registers to the next
 *  packet location.
 */

void
ns8390lost_frame(
	int	unit)
{
	ns8390_softc_t *is = &ns8390_softc[unit];
	i386_ioport_t nic = is->nic;
	volatile char  *sram_nxtpkt_ptr;
	int            temp_cr;



	sram_nxtpkt_ptr = (volatile char *) (is->sram +
	    (is->read_nxtpkt_ptr << 8));

	is->read_nxtpkt_ptr = *(sram_nxtpkt_ptr + 1);
	is->read_nxtpkt_ptr &= 0xFF;

	temp_cr = inb(nic+CR) & ~TXP;
	outb(nic+CR, (temp_cr & 0x3f) | PS0);

	/* update boundary register */
	if (is->read_nxtpkt_ptr == is->pstart) {
		outb(nic+BNDY, is->pstop - 1);
	} else {
		outb(nic+BNDY, is->read_nxtpkt_ptr - 1);
	}
	outb(nic+CR, temp_cr);

	return;
}

/*
 * ns8390get_CURR():
 *
 * Returns the value of the register CURR, which points to the next
 * available space for NIC to receive from network unto receive ring.
 *
 */

int
ns8390get_CURR(
	int	unit)
{
	i386_ioport_t nic = ns8390_softc[unit].nic;
	int	temp_cr;
	int	ret_val;
  
	temp_cr = inb(nic+CR) & ~TXP;		/* get current CR value */
	outb(nic+CR, ((temp_cr & 0x3F) | PS1));	/* select page 1 registers */
	ret_val = inb(nic+CURR);		/* read CURR value */
	outb(nic+CR, temp_cr);
	return (ret_val & 0xFF);
}

/*
 * ns8390xmt:
 *
 *	This routine fills in the appropriate registers and memory
 *	locations on the ns8390 board and starts the board off on
 *	the transmit.
 *
 * input	: board number of interest, and a pointer to the mbuf
 * output	: board memory and registers are set for xfer and attention
 *
 */

void
ns8390xmt(
	int		unit)
{
	io_req_t		m;
	ns8390_softc_t		*is = &ns8390_softc[unit];
	i386_ioport_t		nic = is->nic;
	struct ether_header	*eh;
	u_short			count = 0;	/* amount of data already copied */

	m = is->tx_map[is->tx_curr];
	assert(m != (io_req_t)0);
	is->tbusy = 1;

	count = m->io_count;
	if (count < ETHERMIN + sizeof(struct ether_header))
		count = ETHERMIN + sizeof(struct ether_header);
	outb(nic+CR, ABR|STA|PS0);		/* select page 0 */
	outb(nic+TPSR, is->tx_curr + is->tpsr);	/* xmt page start */
	outb(nic+TBCR1, count >> 8);		/* upper byte of count */
	outb(nic+TBCR0, count & 0xFF);		/* lower byte of count */
	outb(nic+CR, TXP|ABR|STA);		/* start transmission */

#if     XKMACHKERNEL
        if (m->io_op & IO_IS_XK_MSG) {
	    eh = (struct ether_header *)msgGetAttr((Msg)m->io_data, 0);
	} else
#endif  /* XKMACHKERNEL */ 
	    eh = (struct ether_header *)m->io_data;
		/*
		 * Loopback Ethernet packet if the device has been opened
		 * more than once (multi-server) and if:
		 * 1) The interface is in promiscuous mode,
		 * 2) The destination addresss is the broadcast address,
		 * 3) The destination address is mine,
		 * 4) The destination address is included in the local set
		 *	of the accepted multicast addresses.
		 */
	if ((m->io_device != DEVICE_NULL &&
	     (m->io_device->open_count > 1 || is->dev_open > 1
#if     XKMACHKERNEL
	      || m->io_op & IO_IS_XK_MSG
#endif  /* XKMACHKERNEL */
	      )) &&
	    (is->ds_if.if_flags & IFF_PROMISC) ||
	    (eh->ether_dhost[0] == 0xFF &&
	      eh->ether_dhost[1] == 0xFF &&
	      eh->ether_dhost[2] == 0xFF &&
	      eh->ether_dhost[3] == 0xFF &&
	      eh->ether_dhost[4] == 0xFF &&
	      eh->ether_dhost[5] == 0xFF) ||
	    (eh->ether_dhost[0] == is->address[0] &&
	     eh->ether_dhost[1] == is->address[1] &&
	     eh->ether_dhost[2] == is->address[2] &&
	     eh->ether_dhost[3] == is->address[3] &&
	     eh->ether_dhost[4] == is->address[4] &&
	     eh->ether_dhost[5] == is->address[5]) ||
	     ((eh->ether_dhost[0] & 1) &&
	      net_multicast_match(&is->multicast_queue,
				  eh->ether_dhost, ETHER_ADDR_SIZE)))
		ns8390send_packet_up(m, eh, is, unit);
	iodone(m);
	
	/*
	 * Start watchdog if needed.
	 */
	if (is->time_out == is->time_xmt++ &&
	    is->time_out == is->time_txt &&
	    !is->timer.set &&
	    (is->board_id & IFWD_NIC_790_BIT) == 0)
		set_timeout_setup(&is->timer, ns8390watchdog,
				  is, NIC_TIMEOUT * hz,
				  current_thread()->bound_processor);
}

boolean_t
config_nic(
	int	unit)
{
	ns8390_softc_t *is = &ns8390_softc[unit];
	i386_ioport_t nic = is->nic;
	int       i;
	int       temp;
	int       count;
	spl_t     s;

	outb (nic+CR, PS0|ABR|STP);             /* soft reset and page 0 */
	outb (nic+RBCR0, 0);			/* clear remote byte count */
	outb (nic+RBCR1, 0);
	if ((inb(nic + ISR) & RST) == 0) {
		/*
		 * The NIC is guaranteed to be reset after the longest
		 * packet time (1500bytes = 1.2ms).
		 */
		delay(1200);
	}

	temp = ((is->fifo_depth & 0x0c) << 3) | BMS;    /* fifo depth | not loopback */
	if (is->board_id & IFWD_SLOT_16BIT)
		temp |= WTS;			/* word xfer select (16 bit cards ) */
	outb (nic+DCR, temp);
	outb (nic+TCR, 0);
	outb (nic+RCR, MON);			/* receive configuration register */
	outb (nic+PSTART, is->pstart);		/* recieve ring starts 2k into RAM */
	outb (nic+PSTOP, is->pstop);		/* stop at last RAM buffer rcv location */
	outb (nic+BNDY, is->pstart);		/* boundary pointer for page 0 */
	s = splimp();

	outb (nic+CR, PS1|ABR|STP);		/* maintain rst | sel page 1 */
	is->read_nxtpkt_ptr = is->pstart + 1;	/* internal next packet pointer */
	outb (nic+CURR, is->read_nxtpkt_ptr);	/* Current page register */
	for(i=0; i<ETHER_ADDR_SIZE; i++)
		outb (nic+PAR0+i, is->address[i]);

	if ((is->board_id & IFWD_NIC_690_BIT) == 0)
		for (i = 0; i < NS8390_MULTICAST_SIZE; i++)
			outb(is->nic+MAR0+i, is->multicast[i]);

	outb (nic+CR, PS0|STP|ABR);
	splx(s);
	outb (nic+ISR, 0xff);			/* clear all interrupt status bits */
	outb (nic+IMR, imr_hold);		/* Enable interrupts */

	outb (nic+CR, PS0|STA|ABR);		/* start NIC | select page 0 */

	if (is->ds_if.if_flags & IFF_PROMISC)
		outb (nic+RCR, AB|AM|PRO); /* receive configuration register */
	else if (!queue_empty(&is->multicast_queue))
		outb (nic+RCR, AB|AM);	/* receive configuration register */
	else
		outb (nic+RCR, AB);	/* receive configuration register */

	return TRUE;
}

/*
 * config_wd8003:
 *
 *	This routine does a standard config of a wd8003 family board, with
 *      the proper modifications to different boards within this family.
 *
 */

boolean_t
config_wd8003(
	int	unit)
{
	ns8390_softc_t *is = &ns8390_softc[unit];
	i386_ioport_t hdwbase = (long)ns8390info[unit]->address;
	int       i;
	int       RAMsize;
	volatile char  *RAMbase;
	int       addr_temp;
	int	  temp_reg;

	switch (is->board_id & IFWD_RAM_SIZE_MASK) {
	      case IFWD_RAM_SIZE_8K:  
		RAMsize =  0x2000;	break;
	      case IFWD_RAM_SIZE_16K: 
		RAMsize =  0x4000;	break;
	      case IFWD_RAM_SIZE_32K: 
		RAMsize =  0x8000;	break;
	      case IFWD_RAM_SIZE_64K: 
		RAMsize = 0x10000;	break;
	      default:           
		RAMsize =  0x2000;	break;
	}
	is->tpsr = 0;			/* transmit page start hold */
	is->sram = (char *)phystokv(ns8390info[unit]->phys_address);
	is->tx_size = NS8390_TX_SIZE(RAMsize);
	if (is->tx_size > NS8390_TX_MAX)
		is->tx_size = NS8390_TX_MAX;
	is->tx_curr = is->tx_free = 0;
	for (i = is->tx_size - 1; i >= 0; i--)
		is->tx_map[i] = 0;
	is->pstart = is->tx_size;	/* receive page start hold */
	is->read_nxtpkt_ptr = is->pstart + 1; /* internal next packet pointer */
	is->fifo_depth = 0x08;		/* NIC fifo threshold */
	is->pstop = (((int)RAMsize >> 8) & 0x0ff);      /* rcv page stop hold */
	RAMbase = ns8390info[unit]->phys_address;

	switch(is->board_id & IFWD_INTERFACE_MASK) {
	case IFWD_INTERFACE_5X3_CHIP: /* 583 or no interface chip */
		outb(hdwbase + IFWD_MSR, IFWD_RST); /* Reset board */

		addr_temp = ((int)(RAMbase) >> 13) & 0x3F; /* convert to MSR */
		outb(hdwbase + IFWD_MSR, addr_temp | IFWD_MENB); /* init MSR */
		break;

	case IFWD_INTERFACE_584_CHIP:
		outb(hdwbase + IFWD_MSR, IFWD_RST); /* Reset board */

		addr_temp = inb(hdwbase + IFWD_REG_5) & ~IFWD_REG5_MEM_MASK;
		addr_temp |= (((int)(RAMbase) >> 16) & 0xF8) >> 3;
		outb(hdwbase + IFWD_REG_5, addr_temp);

		addr_temp = ((int)(RAMbase) >> 13) & 0x3F; /* convert to MSR */
		outb(hdwbase + IFWD_MSR, addr_temp | IFWD_MENB); /* ini. MSR */
		break;

	case IFWD_INTERFACE_585_CHIP:
		temp_reg = inb(hdwbase + IFWD_REG_4);
		outb(hdwbase + IFWD_REG_4, temp_reg | 0x80);
		addr_temp = inb(hdwbase + IFWD_EEPROM_3) & 0x30;
		if ((int)(RAMbase) & 0xF00000)
			addr_temp |= 0x80;
		if ((int)(RAMbase) & 0x20000)
			addr_temp |=  0x40;
		addr_temp |= ((int)(RAMbase) >> 13) & 0xF;
		outb(hdwbase + IFWD_EEPROM_3, addr_temp);
		outb(hdwbase + IFWD_REG_4, temp_reg);

		outb(hdwbase + IFWD_REG_0, IFWD_MENB);
		break;

	case IFWD_INTERFACE_594_CHIP:
		/* not supported */
		break;
	}

	/* enable 16 bit access from lan controller */
	if (is->board_id & IFWD_SLOT_16BIT) {
		if (is->board_id & IFWD_INTERFACE_CHIP) {
			outb(hdwbase+IFWD_REG_5,
			     (inb(hdwbase + IFWD_REG_5) & IFWD_REG5_MEM_MASK) |
			     IFWD_LAN16ENB);
		} else {
			outb(hdwbase+IFWD_REG_5, (IFWD_LAN16ENB | IFWD_LA19));
		}
	}
	/*
	  outb(hdwbase+LAAR, LAN16ENB | LA19| MEM16ENB | SOFTINT);
	  */
	
	return TRUE;
}

/*
 * config_3c503:
 *
 *	This routine does a standard config of a 3 com etherlink II board.
 *
 */

boolean_t
config_3c503(
	int	unit)
{
	ns8390_softc_t	*is = &ns8390_softc[unit];
	struct bus_device	*dev = ns8390info[unit];
	i386_ioport_t	hdwbase = (long)dev->address;
	int		RAMsize = dev->am;
	int		i;

	is->tpsr =   0x20;		/* transmit page start hold */
	is->sram = (char *)phystokv(dev->phys_address) - is->tpsr * 0x100;
					/* When NIC says page 20, this means go to
					   the beginning of the sram range */
	is->tx_size = NS8390_TX_SIZE(RAMsize);
	if (is->tx_size > NS8390_TX_MAX)
		is->tx_size = NS8390_TX_MAX;
	is->tx_curr = is->tx_free = 0;
	for (i = is->tx_size - 1; i > 0; i--)
		is->tx_map[i] = 0;
	is->pstart = is->tpsr + is->tx_size;	/* receive page start hold */
	is->read_nxtpkt_ptr = is->pstart + 1; /* internal next packet pointer */
	is->fifo_depth = 0x08;		/* NIC fifo threshold */
	is->pstop = is->tpsr + ((RAMsize >> 8) & 0x0ff);      /* rcv page stop hold */

	outb(hdwbase+CTLR, CTLR_RST|CTLR_THIN);
	outb(hdwbase+CTLR, CTLR_THIN);
	outb(hdwbase+CTLR, CTLR_STA_ADDR|CTLR_THIN);
	for (i = 0; i < 6; i++)
		is->address[i] = inb(hdwbase+i);
	outb(hdwbase+CTLR, elii_bnc[is->board_id]?CTLR_THIN:CTLR_THICK);
	outb(hdwbase+PSTR, is->pstart);
	outb(hdwbase+PSPR, is->pstop);
	outb(hdwbase+IDCFR, IDCFR_IRQ2 << (elii_irq[is->board_id] - 2));
	outb(hdwbase+GACFR, GACFR_TCM|GACFR_8K);
	/* BCFR & PCRFR ro */
	/* STREG ro & dma */
	outb(hdwbase+DQTR, 0);
	outb(hdwbase+DAMSB, 0);
	outb(hdwbase+DALSB, 0);
	outb(hdwbase+VPTR2, 0);
	outb(hdwbase+VPTR1, 0);
	outb(hdwbase+VPTR0, 0);
	outb(hdwbase+RFMSB, 0);
	outb(hdwbase+RFLSB, 0);
	return TRUE;
}

/*
 * ns8390intoff:
 *
 *	This function turns interrupts off for the ns8390 board indicated.
 *
 */

void
ns8390intoff(
	int	unit)
{
	i386_ioport_t nic = ns8390_softc[unit].nic;
	int temp_cr = inb(nic+CR) & ~TXP;	/* get current CR value */

	outb(nic+CR,(temp_cr & 0x3f)|PS0|STP);
	outb(nic+IMR, 0);                       /* Interrupt Mask Register  */

	if ((inb(nic + ISR) & RST) == 0) {
		/*
		 * The NIC is guaranteed to be reset after the longest
		 * packet time (1500bytes = 1.2ms).
		 */
		delay(1200);
	}
	outb(nic+CR, temp_cr);
}
/*
 * wd80xxcheck_for_585: check that the bic chip is a 83C785 or a 83C790.
 */
boolean_t
wd80xxcheck_for_585(
	i386_ioport_t hdwbase)
{
	unsigned i;
	unsigned char reg_temp;
	unsigned char reg_save;

	reg_temp = inb(hdwbase + IFWD_REG_4);
	for (i = 0; i < 6; i++) {
		outb(hdwbase + IFWD_REG_4, reg_temp | 0x80);
		reg_save = inb(hdwbase + IFWD_LAR_0 + i);
		outb(hdwbase + IFWD_REG_4, reg_temp & ~0x80);
		if (reg_save != inb(hdwbase + IFWD_LAR_0 + i)) {
			outb(hdwbase + IFWD_REG_4, reg_temp);
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 *   wd80xxget_board_id:
 *
 *   determine which board is being used.
 *   Currently supports:
 *    wd8003E (tested)
 *    wd8003EBT
 *    wd8003EP (tested)
 *    wd8013EP (tested)
 *
 */

long
wd80xxget_board_id(
	struct bus_device	*dev)
{
	i386_ioport_t hdwbase = (long)dev->address;
	long           unit = dev->unit;
	long           board_id = 0;
	int            reg_temp;
	int            rev_num;			/* revision number */
	int            ram_flag;
	int            gcr_save;
	int            cr_save;
	int            tcr_save;
	int            intr_temp;
	int	       i;
	boolean_t      register_aliasing;

	rev_num = (inb(hdwbase + IFWD_BOARD_ID) & IFWD_BOARD_REV_MASK) >> 1;
	printf("%s%d: ", ns8390_softc[unit].card, unit);
	
	if (rev_num == 0) {
		printf("rev 0x00\n");
		/* It must be 8000 board */
		return 0;
	}
	
	/* Check if register aliasing is true, that is reading from register
	   offsets 0-7 will return the contents of register offsets 8-f */
	
	register_aliasing = TRUE;
	for (i = 1; i < 5; i++) {
		if (inb(hdwbase + IFWD_REG_0 + i) !=
		    inb(hdwbase + IFWD_LAR_0 + i))
		    register_aliasing = FALSE;
	}
	if (inb(hdwbase + IFWD_REG_7) != inb(hdwbase + IFWD_CHKSUM))
	    register_aliasing = FALSE;
	
	
	if (register_aliasing == FALSE) {
		/* Check if board has interface chip */
		
		reg_temp = inb(hdwbase + IFWD_REG_7);   /* save old */
		outb(hdwbase + IFWD_REG_7, 0x35);	/* write value */
		inb(hdwbase + IFWD_REG_0);		/* dummy read */
		if ((inb(hdwbase + IFWD_REG_7) & 0xff) == 0x35) {
			outb(hdwbase + IFWD_REG_7, 0x3a);/* Try another value*/
			inb(hdwbase + IFWD_REG_0);	/* dummy read */
			if ((inb(hdwbase + IFWD_REG_7) & 0xff) == 0x3a) {
				board_id |= IFWD_INTERFACE_CHIP;
				outb(hdwbase + IFWD_REG_7, reg_temp);
				/* restore old value */
			}
		}
		if (!(board_id & IFWD_INTERFACE_CHIP) &&
		    wd80xxcheck_for_585(hdwbase))
			board_id |= IFWD_INTERFACE_CHIP;
		
		/* Check if board is 16 bit by testing if bit zero in
		   register 1 is unchangeable by software. If so then
		   card has 16 bit capability */
		reg_temp = inb(hdwbase + IFWD_REG_1);
		outb(hdwbase + IFWD_REG_1, reg_temp ^ IFWD_16BIT);
		inb(hdwbase + IFWD_REG_0);		     /* dummy read */
		if ((inb(hdwbase + IFWD_REG_1) & IFWD_16BIT) ==
		    (reg_temp & IFWD_16BIT)) {        /* Is bit unchanged */
			board_id |= IFWD_BOARD_16BIT; /* Yes == 16 bit */
			reg_temp &= 0xfe;	      /* For 16 bit board
							 always reset bit 0 */
		}
		outb(hdwbase + IFWD_REG_1, reg_temp);    /* write value back */
		
		/* Test if 16 bit card is in 16 bit slot by reading bit zero in
		   register 1. */
		if (board_id & IFWD_BOARD_16BIT) {
			if (inb(hdwbase + IFWD_REG_1) & IFWD_16BIT) {
				board_id |= IFWD_SLOT_16BIT;
			}
		}
	}
	
	/* Get media type */
	
	if (inb(hdwbase + IFWD_BOARD_ID) & IFWD_MEDIA_TYPE) {
		board_id |= IFWD_ETHERNET_MEDIA;
	} else if (rev_num == 1) {
		board_id |= IFWD_STARLAN_MEDIA;
	} else {
		board_id |= IFWD_TWISTED_PAIR_MEDIA;
	}

	if (rev_num == 2) {
		if (inb(hdwbase + IFWD_BOARD_ID) & IFWD_SOFT_CONFIG) {
			if ((board_id & IFWD_STATIC_ID_MASK) == WD8003EB ||
			    (board_id & IFWD_STATIC_ID_MASK) == WD8003W) {
				board_id |= IFWD_ALTERNATE_IRQ_BIT;
			}
		}
		/* Check for memory size */
		
		ram_flag = inb(hdwbase + IFWD_BOARD_ID) & IFWD_MEMSIZE;
	    
		switch (board_id & IFWD_STATIC_ID_MASK) {
		      case WD8003E: /* same as WD8003EBT */
		      case WD8003S: /* same as WD8003SH */
		      case WD8003WT:
		      case WD8003W:
		      case WD8003EB: /* same as WD8003EP */
			if (ram_flag)
			    board_id |= IFWD_RAM_SIZE_32K;
			else
			    board_id |= IFWD_RAM_SIZE_8K;
			break;
		      case WD8003ETA:
		      case WD8003STA:
		      case WD8003EA:
		      case WD8003SHA:
		      case WD8003WA:
			board_id |= IFWD_RAM_SIZE_16K;
			break;
		      case WD8013EBT:
			if (board_id & IFWD_SLOT_16BIT) {
				if (ram_flag)
				    board_id |= IFWD_RAM_SIZE_64K;
				else
				    board_id |= IFWD_RAM_SIZE_16K;
			} else {
				if (ram_flag)
				    board_id |= IFWD_RAM_SIZE_32K;
				else
				    board_id |= IFWD_RAM_SIZE_8K;
			}
			break;
		      default:
			board_id |= IFWD_RAM_SIZE_UNKNOWN;
			break;
		}
	} else if (rev_num >= 3) {
		board_id &= (long) ~IFWD_MEDIA_MASK;   /* remove media info */
		board_id |= (wd80xxcheck_for_585(hdwbase) ?
			     IFWD_INTERFACE_585_CHIP :
			     IFWD_INTERFACE_584_CHIP);
		board_id |= wd80xxget_eeprom_info(hdwbase, board_id);

		if (rev_num >= 4) {
			board_id |= IFWD_ADVANCED_FEATURES;
			if (board_id & IFWD_INTERFACE_585_CHIP) {
				board_id &= ~IFWD_INTERFACE_MASK;
				switch(inb(hdwbase + IFWD_REG_7) >> 4) {
				case 4:
					board_id |= IFWD_NIC_SUPERSET;
				case 2:
					board_id |= IFWD_NIC_790_BIT;
				case 3:
					board_id |= IFWD_INTERFACE_585_CHIP;
				}
			}
		}
	} else {
		ram_flag = 0;
		/* Check for memory size */
		if (board_id & IFWD_BOARD_16BIT) {
			if (board_id & IFWD_SLOT_16BIT)
			    board_id |= IFWD_RAM_SIZE_16K;
			else
			    board_id |= IFWD_RAM_SIZE_8K;
		} else if (board_id & IFWD_MICROCHANNEL)
		    board_id |= IFWD_RAM_SIZE_16K;
		else if (board_id & IFWD_INTERFACE_CHIP) {
			if (inb(hdwbase + IFWD_REG_1) & IFWD_MEMSIZE)
			    board_id |= IFWD_RAM_SIZE_32K;
			else
			    board_id |= IFWD_RAM_SIZE_8K;
		} else
		    board_id |= IFWD_RAM_SIZE_UNKNOWN;
	}

	/* Check for WD83C690 chip */

	if (!(board_id & IFWD_NIC_790_BIT)) {
		/*
		 * Enable NIC access.
		 */
		if ((board_id & IFWD_INTERFACE_CHIP) == 0)
			outb(hdwbase + IFWD_REG_0,
			     inb(hdwbase + IFWD_REG_0) & ~(IFWD_RST|IFWD_MENB));

		cr_save = inb(hdwbase + OFF_8390 + CR) & ~TXP;
		outb(hdwbase + OFF_8390 + CR, (cr_save & 0x3F) | PS2);
		tcr_save = inb(hdwbase + OFF_8390 + TCR);
		outb(hdwbase + OFF_8390 + CR, cr_save | PS0);
		outb(hdwbase + OFF_8390 + TCR, ATD | OFST);
		outb(hdwbase + OFF_8390 + CR, (cr_save & 0x3F) | PS2);
		if ((inb(hdwbase + OFF_8390 + TCR) & (ATD | OFST))
		    != (ATD | OFST))
			board_id |= IFWD_NIC_690_BIT;

		/* write values back */
		outb(hdwbase + OFF_8390 + CR, cr_save | PS0);
		outb(hdwbase + OFF_8390 + TCR, tcr_save);
	}

	switch (board_id & IFWD_STATIC_ID_MASK) {
	      case WD8003E: printf("WD8003E or WD8003EBT"); break;
	      case WD8003S: printf("WD8003S or WD8003SH"); break;
	      case WD8003WT: printf("WD8003WT"); break;
	      case WD8003W: printf("WD8003W"); break;
	      case WD8003EB:
		if (board_id & IFWD_INTERFACE_584_CHIP)
		    printf("WD8003EP");
		else
		    printf("WD8003EB");
		break;
	      case WD8003EW: printf("WD8003EW"); break;
	      case WD8003ETA: printf("WD8003ETA"); break;
	      case WD8003STA: printf("WD8003STA"); break;
	      case WD8003EA:
		if (board_id & IFWD_INTERFACE_594_CHIP)
		    printf("WD8013EPA");
		else
		    printf("WD8003EA");
		break;
	      case WD8003SHA: printf("WD8003SHA"); break;
	      case WD8003WA:
		if (board_id & IFWD_INTERFACE_594_CHIP)
		    printf("WD8003WPA");
		else
		    printf("WD8003WA");
		break;
	      case WD8013EBT: printf("WD8013EBT"); break;
	      case WD8013EB:
		if (board_id & IFWD_INTERFACE_584_CHIP)
		    printf("WD8013EP");
		else
		    printf("WD8013EB");
		break;
	      case WD8013W: printf("WD8013W"); break;
	      case WD8013EW: printf("WD8013EW"); break;
	      case WD8013EWC: printf("WD8013EWC"); break;
	      case WD8013WC: printf("WD8013WC"); break;
	      case WD8013EPC: printf("WD8013EPC"); break;
	      case WD8003WC: printf("WD8003WC"); break;
	      case WD8003EPC: printf("WD8003EPC"); break;
	      case WD8208T: printf("WD8208T"); break;
	      case WD8208: printf("WD8208"); break;
	      case WD8208C: printf("WD8208C"); break;
	      case WD8216T: printf("WD8216T"); break;
	      case WD8216: printf("WD8216"); break;
	      case WD8216C: printf("WD8216C"); break;
	      case WD8216L: printf("WD8216L"); break;
	      case WD8216LT: printf("WD8216LT"); break;
	      case WD8216LC: printf("WD8216LC"); break;
	      case WD8216N: printf("WD8216N"); break;
	      case WD8216TN: printf("WD8216TN"); break;
	      case WD8216CN: printf("WD8216CN"); break;
	      case WD8216TH: printf("WD8216TH"); break;
	      case WD8216LTH: printf("WD8216LTH"); break;
	      case WD8416: printf("WD8416"); break;
	      case WD8416T: printf("WD8416T"); break;
	      case WD8416C: printf("WD8416C"); break;
	      case WD8416L: printf("WD8416L"); break;
	      case WD8416LT: printf("WD8416LT"); break;
	      case WD8416LC: printf("WD8416LC"); break;
	      default: printf("unknown SMC board"); break;
	}
	printf(" rev 0x%02x", rev_num);
	switch(board_id & IFWD_RAM_SIZE_RES_7) {
	      case IFWD_RAM_SIZE_UNKNOWN:
		break;
	      case IFWD_RAM_SIZE_8K:
		printf(" 8 kB ram");
		break;
	      case IFWD_RAM_SIZE_16K:
		printf(" 16 kB ram");
		break;
	      case IFWD_RAM_SIZE_32K:
		printf(" 32 kB ram");
		break;
	      case IFWD_RAM_SIZE_64K:
		printf(" 64 kB ram");
		break;
	      default:
		printf("wd: Internal error ram size value invalid %d\n",
		       (board_id & IFWD_RAM_SIZE_RES_7)>>16);
	}
	
	if (board_id & IFWD_BOARD_16BIT) {
		if (board_id & IFWD_SLOT_16BIT) {
			printf(", in 16 bit slot");
		} else {
			printf(", 16 bit board in 8 bit slot");
		}
	}
	if (board_id & IFWD_INTERFACE_CHIP) {
		switch (board_id & IFWD_INTERFACE_MASK) {
		case IFWD_INTERFACE_584_CHIP:
			printf(", 584 chip");
			break;
		case IFWD_INTERFACE_594_CHIP:
			printf(", 594 chip");
			break;
		case IFWD_INTERFACE_585_CHIP:
			printf(", 585 chip");
			break;
		case IFWD_INTERFACE_5X3_CHIP:
			printf(", 583 chip");
			break;
		default:
			printf(", Unknown BIC chip");
			break;
		}
	}
	if (board_id & IFWD_NIC_790_BIT) {
		printf(", 83C790 nic");
	} else if (board_id & IFWD_NIC_690_BIT) {
		printf(", 83C690 nic");
	} else {
		printf(", NS8390 nic");
	}

	if ((board_id & IFWD_INTERFACE_CHIP) == IFWD_INTERFACE_CHIP) {
		if (board_id & IFWD_INTERFACE_585_CHIP) {
			/* program the WD83C585 IRQ registers */
			outb(hdwbase + IFWD_REG_6, 0x01);
			reg_temp = inb(hdwbase + IFWD_REG_4);
			if (!(reg_temp & 0x80))
				outb(hdwbase + IFWD_REG_4, reg_temp | 0x80);
			gcr_save = inb(hdwbase + IFWD_EEPROM_5) & ~0x4C;
			switch (ns8390info[unit]->sysdep1) {
			case 2:
			case 9:
				gcr_save |= 0x04;
				break;
			case 3:
				gcr_save |= 0x08;
				break;
			case 4:
				if (board_id & IFWD_BOARD_16BIT)
					goto error_585;
				gcr_save |= 0x0C;
				break;
			case 5:
				if (board_id & IFWD_BOARD_16BIT)
					gcr_save |= 0x0C;
				else
					gcr_save |= 0x40;
				break;
			case 7:
				if (board_id & IFWD_BOARD_16BIT)
					gcr_save |= 0x40;
				else
					gcr_save |= 0x44;
				break;
			case 10:
				if (!(board_id & IFWD_BOARD_16BIT))
					goto error_585;
				gcr_save |= 0x44;
				break;
			case 11:
				if (!(board_id & IFWD_BOARD_16BIT))
					goto error_585;
				gcr_save |= 0x48;
				break;
			case 15:
				if (!(board_id & IFWD_BOARD_16BIT))
					goto error_585;
				gcr_save |= 0x4C;
				break;
			default:
			error_585:
				printf("%s%d: wd80xx_get_board_id(): Could not set Interrupt Request Register according to pic(%d).\n",
				       ns8390_softc[unit].card, unit,
				       ns8390info[unit]->sysdep1);
				break;
			}
			outb(hdwbase + IFWD_EEPROM_5, gcr_save);
			if (!(reg_temp & 0x80))
				outb(hdwbase + IFWD_REG_4, reg_temp);

		} else {
			/* program the WD83C58x EEPROM registers */
			int irr_temp, icr_temp;
		
			icr_temp = inb(hdwbase + IFWD_ICR);
			irr_temp = inb(hdwbase + IFWD_IRR);
		
			irr_temp &= ~(IFWD_IR0 | IFWD_IR1);
			irr_temp |= IFWD_IEN;
		
			icr_temp &= IFWD_WTS;
		
			if (!(board_id & IFWD_INTERFACE_584_CHIP)) {
				icr_temp |= IFWD_DMAE | IFWD_IOPE;
				if (ram_flag)
					icr_temp |= IFWD_MSZ;
			}
		
			if (board_id & IFWD_INTERFACE_584_CHIP) {
				switch(ns8390info[unit]->sysdep1) {
				case 10:
					icr_temp |= IFWD_DMAE;
					break;
				case 2:
				case 9: /* Same as 2 */
					break;
				case 11:
					icr_temp |= IFWD_DMAE;
					/*FALLTHROUGH*/
				case 3:
					irr_temp |= IFWD_IR0;
					break;
				case 15:
					icr_temp |= IFWD_DMAE;
					/*FALLTHROUGH*/
				case 5:
					irr_temp |= IFWD_IR1;
					break;
				case 4:
					icr_temp |= IFWD_DMAE;
					/*FALLTHROUGH*/
				case 7:
					irr_temp |= IFWD_IR0 | IFWD_IR1;
					break;
				default:
					printf("%s%d: wd80xx_get_board_id(): Could not set Interrupt Request Register according to pic(%d).\n",
					       ns8390_softc[unit].card, unit,
					       ns8390info[unit]->sysdep1);
					break;
				}
			} else {
				switch(ns8390info[unit]->sysdep1) {
					/* attempt to set interrupt according to assigned pic */
				case 2:
				case 9: /* Same as 2 */
					break;
				case 3:
					irr_temp |= IFWD_IR0;
					break;
				case 4:
					irr_temp |= IFWD_IR1;
					break;
				case 5:
					irr_temp |= IFWD_IR1 | IFWD_AINT;
					break;
				case 7: 
					irr_temp |= IFWD_IR0 | IFWD_IR1;
					break;
				default: 
					printf("%s%d: wd80xx_get_board_id(): Could not set Interrupt Request Register according to pic(%d).\n",
					       ns8390_softc[unit].card, unit,
					       ns8390info[unit]->sysdep1);
				}
			}
			outb(hdwbase + IFWD_IRR, irr_temp);
			outb(hdwbase + IFWD_ICR, icr_temp);
		}
	}
	printf("\n");
	return (board_id);
}

unsigned long
wd80xxget_eeprom_info(
     i386_ioport_t	hdwbase,
     long		board_id)
{
  unsigned long new_bits = 0;
  int 		reg_temp;
  
  if (wd80xxcheck_for_585(hdwbase)) {
	  outb(hdwbase + IFWD_REG_1, IFWD_OFFSET_ENGR_PAGE | IFWD_EER_RC);
	  while (inb(hdwbase + IFWD_REG_1) & IFWD_EER_RC)
		  ;
  } else {
	  outb(hdwbase + IFWD_REG_1,
	       ((inb(hdwbase + IFWD_REG_1) & IFWD_ICR_MASK) | IFWD_OTHER_BIT));
	  outb(hdwbase + IFWD_REG_3,
	       ((inb(hdwbase + IFWD_REG_3) & IFWD_EAR_MASK) | IFWD_ENGR_PAGE));
	  outb(hdwbase + IFWD_REG_1,
	       ((inb(hdwbase + IFWD_REG_1) & IFWD_ICR_MASK) |
		(IFWD_RLA | IFWD_OTHER_BIT)));
	  while (inb(hdwbase + IFWD_REG_1) & IFWD_RECALL_DONE_MASK)
		  ;
  }
  
  reg_temp = inb(hdwbase + IFWD_EEPROM_1);
  switch (reg_temp & IFWD_EEPROM_BUS_TYPE_MASK) {
	case IFWD_EEPROM_BUS_TYPE_AT:
	  if (wd_debug & 1) printf("wd: AT bus, ");
	  break;
	case IFWD_EEPROM_BUS_TYPE_MCA:
	  if (wd_debug & 1) printf("wd: MICROCHANNEL, ");
	  new_bits |= IFWD_MICROCHANNEL;
	  break;
	default:
	  break;
  }

  if (reg_temp & IFWD_EEPROM_RAM_PAGING)
	  new_bits |= IFWD_PAGED_RAM;
  if (reg_temp & IFWD_EEPROM_ROM_PAGING)
	  new_bits |= IFWD_PAGED_ROM;

  switch (reg_temp & IFWD_EEPROM_BUS_SIZE_MASK) {
	case IFWD_EEPROM_BUS_SIZE_8BIT:
	  if (wd_debug & 1) printf("8 bit bus size, ");
	  break;
	case IFWD_EEPROM_BUS_SIZE_16BIT:
	  if (wd_debug & 1) printf("16 bit bus size ");
	  new_bits |= IFWD_BOARD_16BIT;
	  if (wd80xxcheck_for_585(hdwbase)) {
		  if (inb(hdwbase + IFWD_REG_4) & 0x10)
			  new_bits |= IFWD_SLOT_16BIT;
	  } else if (inb(hdwbase + IFWD_REG_1) & IFWD_16BIT) {
		  new_bits |= IFWD_SLOT_16BIT;
		  if (wd_debug & 1)
		      printf("in 16 bit slot, ");
	  } else {
		  if (wd_debug & 1)
		      printf("in 8 bit slot (why?), ");
	  }
	  break;
	default:
	  if (wd_debug & 1) printf("bus size other than 8 or 16 bit, ");
	  break;
  }
  reg_temp = inb(hdwbase + IFWD_EEPROM_0);
  switch (reg_temp & IFWD_EEPROM_MEDIA_MASK) {
	case IFWD_STARLAN_TYPE:
	  if (wd_debug & 1) printf("Starlan media, ");
	  new_bits |= IFWD_STARLAN_MEDIA;
	  break;
	case IFWD_TP_TYPE:
	  if (wd_debug & 1) printf("Twisted pair media, ");
	  new_bits |= IFWD_TWISTED_PAIR_MEDIA;
	  break;
	case IFWD_EW_TYPE:
	  if (wd_debug & 1) printf("Ethernet and twisted pair media, ");
	  new_bits |= IFWD_EW_MEDIA;
	  break;
	case IFWD_ETHERNET_TYPE: /*FALLTHROUGH*/
	default:
	  if (wd_debug & 1) printf("ethernet media, ");
	  new_bits |= IFWD_ETHERNET_MEDIA;
	  break;
  }
  switch (reg_temp & IFWD_EEPROM_IRQ_MASK) {
	case IFWD_ALTERNATE_IRQ_1:
	  if (wd_debug & 1) printf("Alternate irq 1\n");
	  new_bits |= IFWD_ALTERNATE_IRQ_BIT;
	  break;
	default:
	  if (wd_debug & 1) printf("\n");
	  break;
  }
  switch (reg_temp & IFWD_EEPROM_RAM_SIZE_MASK) {
	case IFWD_EEPROM_RAM_SIZE_8K:
	  new_bits |= IFWD_RAM_SIZE_8K;
	  break;
	case IFWD_EEPROM_RAM_SIZE_16K:
	  if ((new_bits & IFWD_BOARD_16BIT) && (new_bits & IFWD_SLOT_16BIT))
	      new_bits |= IFWD_RAM_SIZE_16K;
	  else
	      new_bits |= IFWD_RAM_SIZE_8K;
	  break;
	case IFWD_EEPROM_RAM_SIZE_32K:
	  new_bits |= IFWD_RAM_SIZE_32K;
	  break;
	case IFWD_EEPROM_RAM_SIZE_64K:
	  if ((new_bits & IFWD_BOARD_16BIT) && (new_bits & IFWD_SLOT_16BIT))
	      new_bits |= IFWD_RAM_SIZE_64K;
	  else
	      new_bits |= IFWD_RAM_SIZE_32K;
	  break;
	default:
	  new_bits |= IFWD_RAM_SIZE_UNKNOWN;
	  break;
  }

  reg_temp = inb(hdwbase + IFWD_EEPROM_3);
  if (reg_temp & IFWD_EEPROM_LOW_COST)
	  new_bits |= IFWD_LITE_VERSION;
  if (reg_temp & IFWD_EEPROM_HMI)
	  new_bits |= IFWD_HMI_ADAPTER;

  if (wd80xxcheck_for_585(hdwbase)) {
	outb(hdwbase + IFWD_REG_1, IFWD_OFFSET_LAN_ADDR | IFWD_EER_RC);
	while (inb(hdwbase + IFWD_REG_1) & IFWD_EER_RC)
	    ;
  } else {
	outb(hdwbase + IFWD_REG_1,
	     ((inb(hdwbase + IFWD_REG_1) & IFWD_ICR_MASK) | IFWD_OTHER_BIT));
	outb(hdwbase + IFWD_REG_3,
	     ((inb(hdwbase + IFWD_REG_3) & IFWD_EAR_MASK) | IFWD_EA6));
	outb(hdwbase + IFWD_REG_1,
	     ((inb(hdwbase + IFWD_REG_1) & IFWD_ICR_MASK) | IFWD_RLA));
	while (inb(hdwbase + IFWD_REG_1) & IFWD_RECALL_DONE_MASK)
	    ;
  }
  return (new_bits);
}


#if	used
wdpr(unit)
{
	i386_ioport_t nic = ns8390_softc[unit].nic;
	spl_t	s;
	int	temp_cr;

	s = splimp();
	temp_cr = inb(nic);		/* get current CR value */

	printf("CR %x, BNDRY %x, TSR %x, NCR %x, FIFO %x, ISR %x, RSR %x\n",
		inb(nic+0x0), inb(nic+0x3), inb(nic+0x4), inb(nic+0x5),
		inb(nic+0x6), inb(nic+0x7), inb(nic+0xc));
	printf("CLD %x:%x, CRD %x:%x, FR %x, CRC %x, Miss %x\n",
		inb(nic+0x1), inb(nic+0x2),
		inb(nic+0x8), inb(nic+0x9),
		inb(nic+0xd), inb(nic+0xe), inb(nic+0xf));

	
	outb(nic, (temp_cr&0x3f)|PS1);		/* page 1 CR value */
	printf("PHYS %x:%x:%x:%x:%x CUR %x\n",
		inb(nic+0x1), inb(nic+0x2), inb(nic+0x3),
		inb(nic+0x4), inb(nic+0x5), inb(nic+0x6),
		inb(nic+0x7));
	printf("MAR %x:%x:%x:%x:%x:%x:%x:%x\n",
		inb(nic+0x8), inb(nic+0x9), inb(nic+0xa), inb(nic+0xb),
		inb(nic+0xc), inb(nic+0xd), inb(nic+0xe), inb(nic+0xf));
	outb(nic, temp_cr);			/* restore current CR value */
	splx(s);
}
#endif	/* used */


/*
  This sets bit 7 (0 justified) of register offset 0x05. It will enable
  the host to access shared RAM 16 bits at a time. It will also maintain
  the LAN16BIT bit high in addition, this routine maintains address bit 19
  (previous cards assumed this bit high...we must do it manually)
  
  note 1: this is a write only register
  note 2: this routine should be called only after interrupts are disabled
    and they should remain disabled until after the routine 'dis_16bit_access'
    is called
*/

void
en_16bit_access(
	i386_ioport_t	hdwbase,
	long		board_id)
{
	if (board_id & IFWD_INTERFACE_CHIP) {
		outb(hdwbase+IFWD_REG_5,
		     (inb(hdwbase+IFWD_REG_5) & IFWD_REG5_MEM_MASK)
		     | IFWD_MEM16ENB | IFWD_LAN16ENB);
        } else {
		outb(hdwbase+IFWD_REG_5, (IFWD_MEM16ENB | IFWD_LAN16ENB |
					  IFWD_LA19));
	}
}

/*
  This resets bit 7 (0 justified) of register offset 0x05. It will disable
  the host from accessing shared RAM 16 bits at a time. It will maintain the
  LAN16BIT bit high in addition, this routine maintains address bit 19
  (previous cards assumed this bit high...we must do it manually)

  note: this is a write only register
*/

void
dis_16bit_access(
	i386_ioport_t	hdwbase,
	long		board_id)
{
	if (board_id & IFWD_INTERFACE_CHIP) {
		outb(hdwbase+IFWD_REG_5,
		     ((inb(hdwbase+IFWD_REG_5) & IFWD_REG5_MEM_MASK) |
		      IFWD_LAN16ENB));
	} else {
		outb(hdwbase+IFWD_REG_5, (IFWD_LAN16ENB | IFWD_LA19));
	}
}

/*
 * Create a multicast check array given a range of addresses.
 */
void
ns8390_multicast_create(
	net_multicast_t *cur)
{
	unsigned long	crc32;
	unsigned char	*p;
	int		i;
	unsigned	j;
	unsigned char	*crc;
	unsigned char	*to;
	unsigned char	from[ETHER_ADDR_SIZE];
	unsigned char	c;

	for (i = 0; i < ETHER_ADDR_SIZE; i++)
	    from[i] = NET_MULTICAST_FROM(cur)[i];
	to = NET_MULTICAST_TO(cur);
	crc = p = NET_MULTICAST_MISC(cur);

	for (;;) {
	    /*
	     * Compute the CRC32 of the address.
	     */
	    crc32 = 0xFFFFFFFF;
	    for (i = 0; i < ETHER_ADDR_SIZE; i++) {
		c = from[i];
		for (j = 0; j < 8; j++) {
		    if ((c & 0x01) ^ ((crc32 & 0x80000000) ? 1 : 0))
			crc32 = ((crc32 << 1) & 0xFFFFFFFF) ^ 0x04C11DB7;
		    else
			crc32 = (crc32 << 1) & 0xFFFFFFFF;
		    c >>= 1;
		}
	    }

	    /*
	     * Update CRC array with the 6 most significant bits.
	     */
	    crc32 = (crc32 >> 26) & (NS8390_MULTICAST_SIZE * 8 - 1);
	    crc[crc32 / NS8390_MULTICAST_SIZE] |=
		    (1 << (crc32 % NS8390_MULTICAST_SIZE));
	    while (*p == 0xFF)
		if (p < &crc[NS8390_MULTICAST_SIZE-1])
		    p++;
		else
		    break;

	    /*
	     * Goto next address.
	     */
	    if (cur->hash >= 0)
		break;

	    for (i = ETHER_ADDR_SIZE-1; i >= 0 ; i--)
		if (++from[i])
		     break;
	    for (i = 0; i < ETHER_ADDR_SIZE; i++)
		if (from[i] != to[i])
		    break;
	    if (i == ETHER_ADDR_SIZE)
		break;
	}
}

/*
 * Recompute a new multicast array from its multicast entries.
 * MP notes: MP protection has been setup before being called.
 */
void
ns8390_multicast_compute(
	ns8390_softc_t	*sp)
{
	unsigned	i;
	net_multicast_t	*cur;
	unsigned char	*crc;
	unsigned char	*p;

	if (sp->ds_if.if_flags & IFF_PROMISC) {
	    for (i = 0; i < NS8390_MULTICAST_SIZE; i++)
		sp->multicast[i] = 0xFF;
	    if ((sp->ds_if.if_flags & IFF_ALLMULTI) == 0)
		sp->ds_if.if_flags |= IFF_ALLMULTI;
	    return;
	}

	for (i = 0; i < NS8390_MULTICAST_SIZE; i++)
	    sp->multicast[i] = '\0';

	if (queue_empty(&sp->multicast_queue)) {
	    if (sp->ds_if.if_flags & IFF_ALLMULTI)
		sp->ds_if.if_flags &= ~IFF_ALLMULTI;
	    return;
	}

	if ((sp->board_id & IFWD_NIC_690_BIT) == 0) {
	    if ((sp->ds_if.if_flags & IFF_ALLMULTI) == 0)
		sp->ds_if.if_flags |= IFF_ALLMULTI;
	    return;
	}

	p = sp->multicast;
	queue_iterate(&sp->multicast_queue, cur, net_multicast_t *, chain) {
	    crc = NET_MULTICAST_MISC(cur);
	    for (i = 0; i < NS8390_MULTICAST_SIZE; i++)
		sp->multicast[i] |= crc[i];
	    while (*p == 0xFF)
		if (p < &sp->multicast[NS8390_MULTICAST_SIZE-1])
		    p++;
		else {
		    if ((sp->ds_if.if_flags & IFF_ALLMULTI) == 0)
			sp->ds_if.if_flags |= IFF_ALLMULTI;
		    return;
	        }
	}

	if (sp->ds_if.if_flags & IFF_ALLMULTI)
	    sp->ds_if.if_flags &= ~IFF_ALLMULTI;
}

#if XKMACHKERNEL

static boolean_t
xkcopy(
	char 		*src,
	long		length,
	void		*arg)
{
	char **physaddr = (char **)arg;

	(void)bcopy((char *)src, *physaddr, length);

	*physaddr += length;
	return TRUE;
}

static boolean_t
xkcopy_test(
	    char 		*src,
	    long		length,
	    void		*arg)
{
	char **physaddr = (char **)arg;

	/* 
         * Validate source address 
	 * (in case of messages constructed in place,
	 * users may have withdrawn the bits)
	 */
	if (pmap_extract(kernel_pmap, (vm_offset_t)src) == (vm_offset_t)0) {
	    return FALSE;
	}

	(void)bcopy((char *)src, *physaddr, length);

	*physaddr += length;
	return TRUE;
}

static boolean_t
xkcopy16(
	char 		*src,
	long		length,
	void		*arg)
{
	char **physaddr = (char **)arg;

	(void)bcopy16((char *)src, *physaddr, length);

	*physaddr += length;
	return TRUE;
}

static boolean_t
xkcopy16_test(
	      char 		*src,
	      long		length,
	      void		*arg)
{
	char **physaddr = (char **)arg;
	/* 
         * Validate source address 
	 * (in case of messages constructed in place,
	 * users may have withdrawn the bits)
	 */
	if (pmap_extract(kernel_pmap, (vm_offset_t)src) == (vm_offset_t)0) {
	    return FALSE;
	}

	(void)bcopy16((char *)src, *physaddr, length);

	*physaddr += length;
	return TRUE;
}

boolean_t
xkcopy_to_device(
	register io_req_t       ior,
	char			*physaddr,
	i386_ioport_t 		hdwbase,
	int     		board_id)
{
	register	ETHhdr *hdr;
	boolean_t	copy16 = (board_id & IFWD_SLOT_16BIT);
	int     ss;
	char    *ph = physaddr;
	char    *phend = physaddr + ior->io_count;
	Msg     ms = (Msg)ior->io_data;

	xIfTrace(xkwdp, TR_EVENTS) {
	    printf("xkwd: Outgoing message: ");
	    msgShow(ms);
	}
	hdr = (ETHhdr *)msgGetAttr(ms, 0);
	if (copy16) {
	    ss = splhi();
	    en_16bit_access((i386_ioport_t)(long) hdwbase, board_id);
	    (void)bcopy16((char *)hdr, ph, sizeof (ETHhdr));
	    ph += sizeof (ETHhdr);
	    msgForEachAlternate(ms, (XCharFun)xkcopy16, (XCharFun)xkcopy16_test, (void *) &ph);
	    dis_16bit_access((i386_ioport_t)(long) hdwbase, board_id);
	    splx(ss);
	} else {
	    (void)bcopy((char *)hdr, ph, sizeof (ETHhdr));
	    ph += sizeof (ETHhdr);
	    msgForEachAlternate(ms, (XCharFun)xkcopy, (XCharFun)xkcopy_test, (void *) &ph);
	}
	if ((vm_offset_t)phend - (vm_offset_t)ph)
	    return FALSE;
	return TRUE;
}

/*
 *    Support for generic xkernel operations
 *    The openenable interface allows the driver 
 *    get the address of the higher level protocol to which it interfaces
 */
xkern_return_t
xkwd_openenable(
	XObj self, 
	XObj hlp, 
	XObj hlptype,
	Part part)
{ 
	int index;

	index = self->instName[strlen(self->instName) - 1] - '0';
	if (index < 0 || index >= NNS8390) {
            xTrace0(xkwdp, TR_ERRORS, 
		    "Interface name cannot be translated to interface index");
	    return XK_FAILURE;
	}
	self_protocol[index] = self;
	xSetUp(self, hlp);
	return XK_SUCCESS;
}

io_return_t
xknet_write(
	register struct ifnet *ifp,
	void		(*start)(int unit),
	io_req_t	ior)
{
	int	s;
	kern_return_t	rc;
	boolean_t	wait;

	/*
	 * Reject the write if the interface is down.
	 */
	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
	    return (D_DEVICE_DOWN);

	/*
	 * Reject the write if the packet is too large or too small.
	 */
	if (ior->io_count < ifp->if_header_size ||
	    ior->io_count > ifp->if_header_size + ifp->if_mtu)
	    return (D_INVALID_SIZE);

	/*
	 * Queue the packet on the output queue, and
	 * start the device.
	 */
	s = splimp();
	IF_ENQUEUE(&ifp->if_snd, ior);
	(*start)(ifp->if_unit);
	splx(s);

	return (D_IO_QUEUED);
}

boolean_t
free_ior(
	io_req_t ior)
{
	spl_t s;

	assert(ior->io_op & IO_IS_XK_DIRTY);
	assert(ior->io_op & IO_IS_XK_MSG);
	s = splimp();
	enqueue_head(&ior_xkernel_qhead, (queue_entry_t)ior);
#if     XKSTATS
	xk_ior_queued++;
#endif /* XKSTATS */
	splx(s);
	return TRUE;
}

/*
 * xkwd_push
 * the interface from the xkernel for outgoing packets
 */
xmsg_handle_t
xkwd_push(
	XObj s,
	Msg downMsg)
{
	ETHhdr  *hdr = (ETHhdr *)msgGetAttr(downMsg, 0);
	ETHtype type;
	ETHhost *dest;
	io_req_t ior;
	char *msgattr;
	unsigned short eth_type;
	Msg  xkm;
	spl_t spl;

        type = hdr->type;
        dest = &hdr->dst;

	spl = splimp();
        ior = (io_req_t)dequeue_head(&ior_xkernel_qhead);
	if (ior == (io_req_t)0) {
#if     XKSTATS
	    xk_ior_dropped++;
#endif /* XKSTATS */
	    splx(spl);
	    return XMSG_NULL_HANDLE;
	}
#if     XKSTATS
	xk_ior_queued--;
#endif /* XKSTATS */
	splx(spl);

	xkm = (Msg)ior->io_data;
	if (ior->io_op & IO_IS_XK_DIRTY) {
	    msgDestroy(xkm);
	    ior->io_op &= ~IO_IS_XK_DIRTY;
	}
	msgConstructCopy(xkm, downMsg);
        msgattr = (char *)(xkm + 1);
	memcpy((char *)msgattr, (const char *)hdr, sizeof(ETHhdr));
        msgSetAttr(xkm, 0, msgattr, sizeof(ETHhdr));

	xTrace3(xkwdp, TR_EVENTS, "xkwd: send to %x.%x.%x", dest->high, dest->mid, dest->low);

	ior->io_count = msgLen(xkm) + sizeof(ETHhdr);

	/* operation is write and call io_done_thread when done */
	/* IO_IS_XK_MSG to be able to cope with xk and !xk msgs */
	ior->io_op = IO_WRITE | IO_LOANED | IO_IS_XK_MSG | IO_IS_XK_DIRTY;
	ior->io_unit = (int)s->state;
	ior->io_done = free_ior;
	 
	/*
	 * xknet_write is a scaled-down version of net_write, to skip
         * the device_write_get() call.
	 */
	if (xknet_write(&ns8390_softc[ior->io_unit].ds_if, ns8390start, ior)
	   						!= D_IO_QUEUED )
	    panic( "xknet_write: bad return value" );

	return XMSG_NULL_HANDLE;
}

static void
getLocalEthHost(
	ETHhost *host,
	int unit)
{
	bcopy ((char *)&ns8390_softc[unit].address[0], (char *)host, sizeof( ETHhost ));
        xTrace3(xkwdp, TR_EVENTS, "xkwd: host addr %x.%x.%x",
                host->high, host->mid, host->low);
}

/*
 *  xkwd_control
 *   Control operations
 */
int
xkwd_control(
	XObj  self,
	int   op,
	char *buf,
	int   len)
{  
	int	unit;

	xTrace1(xkwdp, TR_FULL_TRACE, "xkwd_control: operation %x", op);

        unit = self->instName[strlen(self->instName) - 1] - '0';

        switch ( op ) {
        case GETMYHOST:
            if (len >= sizeof(ETHhost)) {
                if (unit >= 0 && unit < NNS8390) {
                    getLocalEthHost((ETHhost *)buf, unit);
                    return sizeof(ETHhost);
                }
            }
            break;
        case ADDMULTI:
        case DELMULTI:
            {
                /*
                 * Faked-up argument structure for the multicast setstat()s.
                 */
                struct {
                        char c[2 * sizeof(ETHhost)];
                } addr;
                int ret;

                if (len < sizeof (ETHhost))
                        break;

                bcopy(buf, &addr.c[0], sizeof (ETHhost));
                bcopy(buf, &addr.c[sizeof (ETHhost)], sizeof (ETHhost));
                ret = ns8390setstat( (dev_t)unit,
                        (dev_flavor_t)(op == ADDMULTI ? NET_ADDMULTI : NET_DELMULTI),
                        (dev_status_t)&addr,
                        (natural_t)sizeof addr);
                if ( ret == D_SUCCESS )
                        return 0;
            }
            break;
        }

	return -1;
}

xkern_return_t
xkwd_init(
	XObj self)
{
	int	unit, i;

	xTrace0(xkwdp, TR_GROSS_EVENTS, "xkwd_init");

	unit = self->instName[strlen(self->instName) - 1] - '0';
	if (unit < 0 || unit >= NNS8390) {
	  unit = 0;
	  xTrace1(xkwdp, TR_ERRORS, "xkwd: assuming unit 0 for %s",
		  self->instName);
	}
	/* 
	 * We kludge the unit number in the 'state' pointer field of the XObj.
	 */
	self->state = (char *)unit;
	self_protocol[unit] = self;
	/* 
	 * If the interface is in use, we respect the boundaries assigned.
	 * Otherwise, we can take all of the traffic space (0, 0xffff).
	 * This is particluarly usefull to run bootp and arp at 
	 * boot time 
	 */
	if ((ns8390_softc[unit].ds_if.if_flags & (IFF_UP|IFF_RUNNING)) ==
	                                             (IFF_UP|IFF_RUNNING)) {
	    ns8390_softc[unit].xk_lowest_type = XK_LOWEST_TYPE;
	    ns8390_softc[unit].xk_highest_type = XK_HIGHEST_TYPE;
	} else {
	    ns8390_softc[unit].xk_lowest_type = 0;
	    ns8390_softc[unit].xk_highest_type = 0xffff;
	}	    
	if (wd8003open((dev_t)unit, 0, (io_req_t) 0) == ENXIO) 
	    panic("xkwd_init: the device does not exist");
	ns8390_softc[unit].xk_up = TRUE;

	if (allocateResources(self) != XK_SUCCESS)
	    panic("xkwd_init: couldn't allocate memory to start the driver");
	
	self->openenable = xkwd_openenable;
	self->push = xkwd_push;
	self->control = xkwd_control;

	xTrace0(xkwdp, TR_GROSS_EVENTS, "xkwd_init returns");
	return XK_SUCCESS;
}
#endif /* XKMACHKERNEL */
