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
 * Revision 2.12.5.3  92/06/24  17:58:46  jeffreyh
 * 	Updated to new at386_io_lock() interface.
 * 	[92/06/03            bernadat]
 * 
 * Revision 2.12.5.2  92/04/30  11:55:40  bernadat
 * 	Adapt to CBUS/MBUS
 * 	[92/04/21            bernadat]
 * 
 * Revision 2.12.5.1  92/02/18  18:52:14  jeffreyh
 * 	Support for Corollary MP, switch to master for i386 ports R/W
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.12  91/08/24  11:57:53  af
 * 	New MI autoconf.
 * 	[91/08/02  02:54:56  af]
 * 
 * Revision 2.10  91/05/13  06:02:32  af
 * 	Made code under CMUCS standard.
 * 	[91/05/12  15:50:59  af]
 * 
 * Revision 2.9  91/03/16  14:46:08  rpd
 * 	Changed net_filter to net_packet.
 * 	[91/01/15            rpd]
 * 
 * Revision 2.8  91/02/05  17:17:32  mrt
 * 	Added OSF permission and disclaimer clause per instructions
 * 	of Philippe Bernadat.
 * 	[91/02/04            mrt]
 * 
 * Revision 2.7  91/01/08  15:11:34  rpd
 * 	Changed NET_KMSG_GET, NET_KMSG_FREE to net_kmsg_get, net_kmsg_put.
 * 	[91/01/05            rpd]
 * 
 * Revision 2.6  90/11/26  14:49:51  rvb
 * 	jsb bet me to XMK34, sigh ...
 * 	[90/11/26            rvb]
 * 	Synched 2.5 & 3.0 at I386q (r2.1.1.4) & XMK35 (r2.6)
 * 	[90/11/15            rvb]
 * 
 * Revision 2.5  90/08/27  22:00:04  dbg
 * 	Change ushort to unsigned short.
 * 	[90/07/17            dbg]
 * 
 * Revision 2.1.1.3  90/08/25  15:44:09  rvb
 * 	Use take_<>_irq() vs direct manipulations of ivect and friends.
 * 	[90/08/20            rvb]
 * 
 * 	Fix DSF_RUNNING.  Some more cleanup.
 * 	[90/08/14            rvb]
 * 
 * Revision 2.1.1.2  90/07/10  11:50:05  rvb
 * 	Set etherheader in hwrst to fend off madness of the wtprobe().
 * 	[90/07/02            rvb]
 * 
 * 	New style probe/attach.
 * 	[90/06/15            rvb]
 * 
 * 	Fix xmt if jam to retry.  Set tcp_recvspace for better
 * 	"sink" behaviour.
 * 	[90/03/29            rvb]
 * 
 * Revision 2.4  90/06/02  14:49:05  rpd
 * 	Converted to new IPC.
 * 	[90/06/01            rpd]
 * 
 * Revision 2.3  90/05/21  13:27:01  dbg
 * 	Fix bugs.
 * 	[90/05/17            dbg]
 * 
 * Revision 2.2  90/05/03  15:43:38  dbg
 * 	Convert for pure kernel.
 * 	[90/04/27            dbg]
 * 
 * Revision 2.1.1.1  90/03/16  18:15:22  rvb
 * 	Clean up the i386_dev/i386_ctlr confusion.
 * 	Some minor cleanups
 * 	installed.
 * 	[90/03/13            rvb]
 * 	Created by Philippe Bernadat
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
 */
/*
 *	File:	if_3c501.c
 *	Author: Philippe Bernadat
 *	Date:	1989
 * 	Copyright (c) 1989 OSF Research Institute 
 *
 * 	3COM Etherlink 3C501 Mach Ethernet drvier
 */

#include <at3c501.h>

#include	<platforms.h>
#include	<cpus.h>
#include <mp_v1_1.h>

#include	<string.h>
#include	<kern/time_out.h>
#include	<kern/spl.h>
#include	<kern/misc_protos.h>
#include	<device/device_types.h>
#include	<device/errno.h>
#include	<device/io_req.h>
#include	<device/if_hdr.h>
#include	<device/if_ether.h>
#include	<device/net_status.h>
#include	<device/net_io.h>
#include	<device/misc_protos.h>
#include	<device/ds_routines.h>
#include	<i386/ipl.h>
#include	<i386/pio.h>
#include	<chips/busses.h>
#include	<i386/AT386/if_3c501.h>
#include	<i386/AT386/if_3c501_entries.h>
#include	<i386/AT386/misc_protos.h>
#include	<machine/endian.h>

#include <i386/AT386/mp/mp.h>
#ifdef	CBUS
#include <busses/cbus/cbus.h>
#endif	/* CBUS */
#if	MP_V1_1
#include <i386/AT386/mp/mp_v1_1.h>
#endif	/* MP_V1_1 */

/* Forward */

extern int		at3c501probe(
				int			port,
				struct bus_device	* dev);
extern void		at3c501attach(
				struct bus_device	* dev);
extern void		at3c501watch(
				caddr_t			bp);
extern void		at3c501geteh(
				i386_ioport_t		base,
				unsigned char		* ep);
extern void		at3c501seteh(
				i386_ioport_t		base,
				unsigned char		* ep);
extern int		at3c501reset(
				int			unit);
extern int		at3c501init(
				int			unit);
extern void		at3c501start(
				int			unit);
extern boolean_t	at3c501hwrst(
				int			unit);
extern void		at3c501rcv(
				int			unit);
extern void		at3c501xmt(
				int			unit,
				io_req_t		m);
extern boolean_t	at3c501config(
				int			unit);
extern void		at3c501intoff(
				int			unit);

static caddr_t at3c501_std[NAT3C501] = { 0 };
static struct bus_device *at3c501_info[NAT3C501];
struct	bus_driver	at3c501driver = 
	{(probe_t)at3c501probe, 0, at3c501attach, 0, at3c501_std, "et",
	at3c501_info, };

int	watchdog_id;

typedef struct { 
	struct	ifnet	ds_if;		/* generic interface header */
	u_char	ds_addr[6];		/* Ethernet hardware address */
	int	flags;
        int     timer;
	i386_ioport_t	base;
        u_char   address[ETHER_ADD_SIZE];
	short	mode;
	int	badxmt;
	int	badrcv;
	int	spurious;
	int	rcv;
	int	xmt;
} at3c501_softc_t;

at3c501_softc_t	at3c501_softc[NAT3C501];


/*
 * at3c501probe:
 *
 *	This function "probes" or checks for the 3c501 board on the bus to see
 *	if it is there.  As far as I can tell, the best break between this
 *	routine and the attach code is to simply determine whether the board
 *	is configured in properly.  Currently my approach to this is to write
 *	and read a string from the Packet Buffer on the board being probed.
 *	If the string comes back properly then we assume the board is there.
 *	The config code expects to see a successful return from the probe
 *	routine before 	attach will be called.
 *
 * input	: address device is mapped to, and unit # being checked
 * output	: a '1' is returned if the board exists, and a 0 otherwise
 *
 */

int
at3c501probe(port, dev)
struct bus_device	*dev;
{
	i386_ioport_t	base = (i386_ioport_t)(int)dev->address;
	int		unit = dev->unit;
	char		inbuf[50];
	char		*str = "3c501 ethernet board %d out of range\n";
	int 		strsize = strlen(str);
	boolean_t	ret;
	unsigned char	val;
	unsigned char	val0;
	unsigned char	val1;
	unsigned char	save;
	at386_io_lock_state();

	if ((unit < 0) || (unit >= NAT3C501)) {
		printf(str, unit);
		return(0);
	}

	at386_io_lock(MP_DEV_WAIT);

	/*
	 * Get Ethernet PROM Address
	 */
	val0 = inb(IE_GP(base));
	val1 = inb(IE_GP(base) + 1);
	outb(IE_GP(base) + 1, 0);
	outb(IE_GP(base), 0);
	if (inb(IE_SAPROM(base)) != ETHERNET_3C501_ADDR0)
	    ret = FALSE;
	else {
	    outb(IE_GP(base), 1);
	    if (inb(IE_SAPROM(base)) != ETHERNET_3C501_ADDR1)
		ret = FALSE;
	    else {
		outb(IE_GP(base), 2);
		ret = (inb(IE_SAPROM(base)) == ETHERNET_3C501_ADDR2);
	    }
	}
	if (!ret) {
		/*
		 * Restore old values of registers
		 */
		outb(IE_GP(base) + 1, val1);
		outb(IE_GP(base), val0);
		at386_io_unlock();
		return (0);
	}

	/* reset */
	val = inb(IE_CSR(base));
	outb(IE_CSR(base), IE_RESET);

	/* write a string to the packet buffer */

	outb(IE_CSR(base), IE_RIDE | IE_SYSBFR);
	save = inb(IE_BFR(base));
	outw(IE_GP(base), 0);
	loutb(IE_BFR(base), str, strsize);

	/* read it back */

	outb(IE_CSR(base), IE_RIDE | IE_SYSBFR);
	outw(IE_GP(base), 0);
	linb(IE_BFR(base), inbuf, strsize);

	at386_io_unlock();

	/* compare them */
	if (strncmp(str, inbuf, strsize))
	{
		/*
		 * Restore old values of registers
		 */
		outb(IE_GP(base) + 1, val1);
		outb(IE_GP(base), val0);
		outb(IE_BFR(base), save);
		outb(IE_CSR(base), val);
		return (0);
	}
	at3c501_softc[unit].base = base;
	return (1);
}

/*
 * at3c501attach:
 *
 *	This function attaches a 3C501 board to the "system".  The rest of
 *	runtime structures are initialized here (this routine is called after
 *	a successful probe of the board).  Once the ethernet address is read
 *	and stored, the board's ifnet structure is attached and readied.
 *
 * input	: bus_device structure setup in autoconfig
 * output	: board structs and ifnet is setup
 *
 */

void
at3c501attach(
	struct bus_device	*dev)
{
	at3c501_softc_t	*sp;
	struct	ifnet	*ifp;
	u_char	unit;
	i386_ioport_t	base;
	at386_io_lock_state();

	take_dev_irq(dev);
	unit = (u_char)dev->unit;	
	printf(", port = %x, spl = %d, pic = %d. ",
		dev->address, dev->sysdep, dev->sysdep1);

	sp = &at3c501_softc[unit];
	base = sp->base;
	if (base != (i386_ioport_t)(int)dev->address) {
		printf("3C501 board %d attach address error\n", unit);
		return;
	}
	sp->timer = -1;
	sp->flags = 0;
	sp->mode = 0;
	at386_io_lock(MP_DEV_WAIT);
	outb(IE_CSR(sp->base), IE_RESET);
	at3c501geteh(base, sp->ds_addr);
	at3c501geteh(base, sp->address);
	at3c501seteh(base, sp->address);
	at386_io_unlock();
	printf("ethernet id [%x:%x:%x:%x:%x:%x]",
		sp->address[0],sp->address[1],sp->address[2], 
		sp->address[3],sp->address[4],sp->address[5]);
	ifp = &(sp->ds_if);
	ifp->if_unit = unit;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST;
	ifp->if_header_size = sizeof(struct ether_header);
	ifp->if_header_format = HDR_ETHERNET;
	ifp->if_address_size = 6;
	ifp->if_address = (char *)&sp->address[0];
	if_init_queues(ifp);
}

/*
 * at3c501watch():
 *
 */

void
at3c501watch(
	caddr_t		b_ptr)

{
	int	x,
		y,
		opri,
		unit;
	at3c501_softc_t *is;

	unit = *b_ptr;
	timeout((timeout_fcn_t)at3c501watch,b_ptr,20*hz);
	is = &at3c501_softc[unit];
	printf("\nxmt/bad	rcv/bad	spurious\n");
	printf("%d/%d		%d/%d	%d\n", is->xmt, is->badxmt, \
		is->rcv, is->badrcv, is->spurious);
	is->rcv=is->badrcv=is->xmt=is->badxmt=is->spurious=0;
}

/*
 * at3c501geteh:
 *
 *	This function gets the ethernet address (array of 6 unsigned
 *	bytes) from the 3c501 board prom. 
 *
 */

void
at3c501geteh(
	i386_ioport_t	base,
	unsigned char	*ep)
{
	int	i;

	for (i = 0; i < ETHER_ADD_SIZE; i++) {
		outw(IE_GP(base), i);
		*ep++ = inb(IE_SAPROM(base));
	}
}

/*
 * at3c501seteh:
 *
 *	This function sets the ethernet address (array of 6 unsigned
 *	bytes) on the 3c501 board. 
 *
 */

void
at3c501seteh(
	i386_ioport_t	base,
	unsigned char	*ep)
{
	int	i;

	for (i = 0; i < ETHER_ADD_SIZE; i++) {
		outb(EDLC_ADDR(base) + i, *ep++);
	}
}

io_return_t
at3c501output(
	dev_t		dev,
	io_req_t	ior)
{
	register int	unit = minor(dev);

	if (unit < 0 || unit >= NAT3C501 ||
		at3c501_softc[unit].base == 0)
	    return (D_NO_SUCH_DEVICE);

	return (net_write(&at3c501_softc[unit].ds_if, at3c501start, ior));
}

io_return_t
at3c501setinput(
	dev_t		dev,
	ipc_port_t	receive_port,
	int		priority,
	filter_t	filter[],
	unsigned int	filter_count,
	device_t	device)
{
	register int unit = minor(dev);

	if (unit < 0 || unit >= NAT3C501 ||
		at3c501_softc[unit].base == 0)
	    return (D_NO_SUCH_DEVICE);

	return (net_set_filter(&at3c501_softc[unit].ds_if,
			receive_port, priority,
			filter, filter_count, device));
}

/*
 * at3c501reset:
 *
 *	This routine is in part an entry point for the "if" code.  Since most 
 *	of the actual initialization has already (we hope already) been done
 *	by calling at3c501attach().
 *
 * input	: unit number or board number to reset
 * output	: board is reset
 *
 */

int
at3c501reset(
	int	unit)
{
	at3c501_softc[unit].ds_if.if_flags &= ~IFF_RUNNING;
	return(at3c501init(unit));
}

/*
 * at3c501init:
 *
 *	Another routine that interfaces the "if" layer to this driver.  
 *	Simply resets the structures that are used by "upper layers".  
 *	As well as calling at3c501hwrst that does reset the at3c501 board.
 *
 * input	: board number
 * output	: structures (if structs) and board are reset
 *
 */	

int
at3c501init(
	int		unit)
{
	struct	ifnet	*ifp;
	int		stat;
	spl_t		oldpri;

	ifp = &(at3c501_softc[unit].ds_if);
	oldpri = splnet();
	if ((stat = at3c501hwrst(unit)) == TRUE) {
		at3c501_softc[unit].ds_if.if_flags |= IFF_RUNNING;
		at3c501_softc[unit].flags |= DSF_RUNNING;
		at3c501start(unit);
	}
	else
		printf("3C501 trouble resetting board %d\n", unit);
	at3c501_softc[unit].timer = 5;
	splx(oldpri);
	return(stat);
}

io_return_t
at3c501open(
	dev_t		dev,
	dev_flavor_t	flag,
	io_req_t	ior)
{
	register int	unit = minor(dev);

	if (unit < 0 || unit >= NAT3C501 ||
		at3c501_softc[unit].base == 0)
	    return (D_NO_SUCH_DEVICE);

	at3c501_softc[unit].ds_if.if_flags |= IFF_UP;
	at3c501init(unit);
	return(D_SUCCESS);
}

/*
 * at3c501start:
 *
 *	This is yet another interface routine that simply tries to output a
 *	in an mbuf after a reset.
 *
 * input	: board number
 * output	: stuff sent to board if any there
 *
 */

void
at3c501start(
	int		unit)

{
	io_req_t	m;
	struct	ifnet	*ifp;
	at386_io_lock_state();

	ifp = &(at3c501_softc[unit].ds_if);
	at386_io_lock(MP_DEV_WAIT);
	for(;;) {
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m != 0)
			at3c501xmt(unit, m);
		else {
			at386_io_unlock();
			return;
		}
	}
}

io_return_t
at3c501getstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	status,		/* pointer to OUT array */
	natural_t	*count)		/* out */
{
	register int	unit = minor(dev);

	if (unit < 0 || unit >= NAT3C501 ||
		at3c501_softc[unit].base == 0)
	    return (D_NO_SUCH_DEVICE);

	return (net_getstat(&at3c501_softc[unit].ds_if,
			    flavor,
			    status,
			    count));
}

io_return_t
at3c501setstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	status,
	natural_t	count)
{
	register int	unit = minor(dev);
	register at3c501_softc_t *sp;

	if (unit < 0 || unit >= NAT3C501 ||
		at3c501_softc[unit].base == 0)
	    return (D_NO_SUCH_DEVICE);

	sp = &at3c501_softc[unit];

	switch (flavor) {
	    case NET_STATUS:
	    {
		/*
		 * All we can change are flags, and not many of those.
		 */
		register struct net_status *ns = (struct net_status *)status;
		int	mode = 0;

		if (count < NET_STATUS_COUNT)
		    return (D_INVALID_SIZE);

		if (ns->flags & IFF_ALLMULTI)
		    mode |= MOD_ENAL;
		if (ns->flags & IFF_PROMISC)
		    mode |= MOD_PROM;

		/*
		 * Force a compilete reset if the receive mode changes
		 * so that these take effect immediately.
		 */
		if (sp->mode != mode) {
		    sp->mode = mode;
		    if (sp->flags & DSF_RUNNING) {
			sp->flags &= ~(DSF_LOCK | DSF_RUNNING);
			at3c501init(unit);
		    }
		}
		break;
	    }
	    case NET_ADDRESS:
	    {
		register union ether_cvt {
		    unsigned char	addr[6];
		    int			lwd[2];
		} *ec = (union ether_cvt *)status;

		if (count < sizeof(*ec)/sizeof(int))
		    return (D_INVALID_SIZE);

		ec->lwd[0] = ntohl(ec->lwd[0]);
		ec->lwd[1] = ntohl(ec->lwd[1]);
   		at3c501seteh(sp->base, ec->addr);
		break;
	    }

	    default:
		return (D_INVALID_OPERATION);
	}
	return (D_SUCCESS);
}

/*
 * at3c501hwrst:
 *
 *	This routine resets the at3c501 board that corresponds to the 
 *	board number passed in.
 *
 * input	: board number to do a hardware reset
 * output	: board is reset
 *
 */
#define XMT_STAT (EDLC_16|EDLC_JAM|EDLC_UNDER|EDLC_IDLE)
#define RCV_STAT (EDLC_STALE|EDLC_ANY|EDLC_SHORT|EDLC_DRIBBLE|EDLC_OVER|EDLC_FCS)

boolean_t
at3c501hwrst(
	int		unit)
{
	u_char	stat;
	i386_ioport_t	base = at3c501_softc[unit].base;
	at386_io_lock_state();

	at386_io_lock(MP_DEV_WAIT);
	outb(IE_CSR(base), IE_RESET);
	outb(IE_CSR(base), 0);
	at3c501seteh(base, at3c501_softc[unit].address);
	if ((stat = inb(IE_CSR(base))) != IE_RESET) {
		printf("at3c501reset: can't reset CSR: %x\n", stat);
		at386_io_unlock();
		return(FALSE);
	}
	if ((stat = inb(EDLC_XMT(base))) & XMT_STAT) {
		printf("at3c501reset: can't reset XMT: %x\n", stat);
		at386_io_unlock();
		return(FALSE);
	}
	if (((stat = inb(EDLC_RCV(base))) & RCV_STAT) != EDLC_STALE) {
		printf("at3c501reset: can't reset RCV: %x\n", stat);
		at386_io_unlock();
		return(FALSE);
	}
	if (at3c501config(unit) == FALSE) {
		printf("at3c501hwrst(): failed to config\n");
		at386_io_unlock();
		return(FALSE);
	}
	outb(IE_RP(base), 0);
	outb(IE_CSR(base), IE_RIDE|IE_RCVEDLC);
	at386_io_unlock();
	return(TRUE);
}

/*
 * at3c501intr:
 *
 *	This function is the interrupt handler for the at3c501 ethernet
 *	board.  This routine will be called whenever either a packet
 *	is received, or a packet has successfully been transfered and
 *	the unit is ready to transmit another packet.
 *
 * input	: board number that interrupted
 * output	: either a packet is received, or a packet is transfered
 *
 */

void
at3c501intr(
	int		unit)
{
	at3c501rcv(unit);
	at3c501start(unit);
}


/*
 * at3c501rcv:
 *
 *	This routine is called by the interrupt handler to initiate a
 *	packet transfer from the board to the "if" layer above this
 *	driver.  This routine checks if a buffer has been successfully
 *	received by the at3c501.  If so, the routine at3c501read is called
 *	to do the actual transfer of the board data (including the
 *	ethernet header) into a packet (consisting of an mbuf chain).
 *
 * input	: number of the board to check
 * output	: if a packet is available, it is "sent up"
 *
 */

void
at3c501rcv(
	int		unit)
{
	int stat;
	i386_ioport_t	base;
	ipc_kmsg_t	new_kmsg;
	struct ether_header *ehp;
	struct packet_header *pkt;
	u_short	len;
	register struct ifnet *ifp;
	struct ether_header header;
	int	tlen;
	register at3c501_softc_t	*is;
	register struct	ifqueue		*inq;
	int	opri;
	struct	ether_header eh;

	is = &at3c501_softc[unit];
	ifp = &is->ds_if;
	base = at3c501_softc[unit].base;
	is->rcv++;
	if (inb(IE_CSR(base)) & IE_RCVBSY)
		is->spurious++;
	while (!((stat=inb(EDLC_RCV(base))) & EDLC_STALE)) {
		outb(IE_CSR(base), IE_SYSBFR);
		if (!(stat & EDLC_ANY)) {
			outw(IE_GP(base), 0);
			len = inw(IE_RP(base))-sizeof(struct ether_header);
			outb(IE_RP(base), 0);
			outb(IE_CSR(base), IE_RIDE|IE_RCVEDLC);
			is->badrcv++;
#ifdef DEBUG
			printf("at3c501rcv: received %d bad bytes", len);
			if (stat & EDLC_SHORT)
				printf(" Short frame");
			if (stat & EDLC_OVER) 
				printf(" Data overflow");
			if (stat & EDLC_DRIBBLE)
				printf(" Dribble error");
			if (stat & EDLC_FCS)
				printf(" CRC error");
			printf("\n");
#endif /* DEBUG */
		} else {
			outw(IE_GP(base), 0);
			len = inw(IE_RP(base));
			if (len < 60) {
				outb(IE_RP(base), 0);
				outb(IE_CSR(base), IE_RIDE|IE_RCVEDLC);
				return;
			}
			linb(IE_BFR(base), (char *)&eh,
				sizeof(struct ether_header));
			new_kmsg = net_kmsg_get();
			if (new_kmsg == IKM_NULL) {
			    /*
			     * Drop the packet.
			     */
			    is->ds_if.if_rcvdrops++;

			    outb(IE_RP(base), 0);
			    outb(IE_CSR(base), IE_RIDE|IE_RCVEDLC);
			    return;
			}

			ehp = (struct ether_header *)
				(&net_kmsg(new_kmsg)->header[0]);
			pkt = (struct packet_header *)
				(&net_kmsg(new_kmsg)->packet[0]);

			/*
			 * Get header.
			 */
			*ehp = eh;

			/*
			 * Get body
			 */
			linb(IE_BFR(base),
			     (char *)(pkt + 1),
			     len - sizeof(struct ether_header));

			outb(IE_RP(base), 0);
			outb(IE_CSR(base), IE_RIDE|IE_RCVEDLC);

			pkt->type = ehp->ether_type;
			pkt->length = len - sizeof(struct ether_header)
					  + sizeof(struct packet_header);

			/*
			 * Hand the packet to the network module.
			 */
			net_packet(ifp, new_kmsg, pkt->length,
				   ethernet_priority(new_kmsg), (io_req_t)0);
		}
	}
}


/*
 * at3c501xmt:
 *
 *	This routine fills in the appropriate registers and memory
 *	locations on the 3C501 board and starts the board off on
 *	the transmit.
 *
 * input	: board number of interest, and a pointer to the mbuf
 * output	: board memory and registers are set for xfer and attention
 *
 */

void
at3c501xmt(
	int		unit,
	io_req_t	m)
{
	int			i;
	at3c501_softc_t		*is = &at3c501_softc[unit];
	i386_ioport_t		base = is->base;
	u_short			count = 0;
	u_short			bytes_in_msg;

	is->xmt++;
	outb(IE_CSR(base), IE_SYSBFR);
	count = m->io_count;
#define	max(a,b)	(((a) > (b)) ? (a) : (b))
	bytes_in_msg = max(count,
			   ETHERMIN + sizeof(struct ether_header));
	outw(IE_GP(base), BFRSIZ-bytes_in_msg);
	loutb(IE_BFR(base), m->io_data, count);
	while (count < bytes_in_msg) {
		outb(IE_BFR(base), 0);
		count++;
	}
	do {
		if (!(int)m) {
			outb(IE_CSR(base), IE_SYSBFR);
		}
		outw(IE_GP(base), BFRSIZ-bytes_in_msg);
		outb(IE_CSR(base), IE_RIDE|IE_XMTEDLC);
		if (m) {
			iodone(m);
			m = 0;
		}
		for (i=0; inb(IE_CSR(base)) & IE_XMTBSY; i++);
		if ((i=inb(EDLC_XMT(base))) & EDLC_JAM) {
			is->badxmt++;
#ifdef DEBUG
			printf("at3c501xmt jam\n");
#endif /* DEBUG */
		}
	} while ((i & EDLC_JAM) && !(i & EDLC_16));

	if (i & EDLC_16) {
		printf("%");
	}
	return;
}

/*
 * at3c501config:
 *
 *	This routine does a standard config of the at3c501 board.
 *
 */

boolean_t
at3c501config(
	int		unit)
{
	i386_ioport_t	base = at3c501_softc[unit].base;
	u_char	stat;

	/* Enable DMA & Interrupts */

	outb(IE_CSR(base), IE_RIDE|IE_SYSBFR);

	/* No Transmit Interrupts */

	outb(EDLC_XMT(base), 0);
	inb(EDLC_XMT(base));

	/* Setup Receive Interrupts */

	outb(EDLC_RCV(base), EDLC_BROAD|EDLC_SHORT|EDLC_GOOD|EDLC_DRIBBLE|EDLC_OVER);
	inb(EDLC_RCV(base));

	outb(IE_CSR(base), IE_RIDE|IE_SYSBFR);
	outb(IE_RP(base), 0);
	outb(IE_CSR(base), IE_RIDE|IE_RCVEDLC);
	return(TRUE);
}

/*
 * at3c501intoff:
 *
 *	This function turns interrupts off for the at3c501 board indicated.
 *
 */

void
at3c501intoff(
	int		unit)
{
	i386_ioport_t	base = at3c501_softc[unit].base;
	outb(IE_CSR(base), 0);
}
