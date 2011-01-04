/* 
 * PMach Operating System
 * Copyright (c) 1995 Santa Clara University
 * All Rights Reserved.
 */
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
 * 
 */
/*
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/* 
 * MkLinux
 */
/*
  Copyright 1990 by Open Software Foundation,
Cambridge, MA.

		All Rights Reserved

  Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appears in all copies and
that both the copyright notice and this permission notice appear in
supporting documentation, and that the name of OSF or Open Software
Foundation not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

  OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <debug.h>
#include <lan.h>
#include <mach_kdb.h>

#include	<kern/time_out.h>
#include	<device/device_types.h>
#include	<device/ds_routines.h>
#include	<device/errno.h>
#include	<device/io_req.h>
#include	<device/if_hdr.h>
#include	<device/if_ether.h>
#include	<device/net_status.h>
#include	<device/net_io.h>
#include	<chips/busses.h>
#include	<machine/endian.h> /* for ntohl */

#include	<ppc/spl.h>
#include	<ppc/proc_reg.h>
#include	<ppc/POWERMAC/interrupts.h>
#include	<ppc/POWERMAC/enet_dma.h>
#include	<ppc/POWERMAC/mace.h>
#include	<ppc/POWERMAC/powermac.h>
#include	<ppc/POWERMAC/powermac_pci.h>
#include	<ppc/POWERMAC/dbdma.h>
#include	<ppc/POWERMAC/device_tree.h>

/* Typedefs */

typedef struct { 
	decl_simple_lock_data(, lock);
	struct	ifnet	ds_if;		/* generic interface header */
	u_char	ds_addr[6];		/* Ethernet hardware address */

	int	flags;
        int     timer;
	struct	mace_board *base;
        u_char   address[ETHER_ADD_SIZE];
	short	mode;
	int	badxmt;
	int	badrcv;
	int	spurious;
	int	rcv;
	int	xmt;
	int	chip_id;
	int	set;

	io_req_t	tx_active;

	dbdma_regmap_t		*dbdma_txchan, *dbdma_rvchan;
	dbdma_command_t		*rv_dma;
	dbdma_command_t		*tx_dma;

	unsigned char		*rv_dma_area;
	int			rv_tail, rv_head;

	struct mace_board	*ereg;	/* ethernet register set address */
	struct enetdma		*edma;

} mace_softc_t;

#define	ETHERNET_DBDMA_BUFS	48
#define	ETHERNET_BUF_SIZE	(ETHERMTU + 22)	/* FCS + Status */

#define	DBDMA_ETHERNET_EOP	0x40

/* Function prototypes */
void		mace_attach();
int		mace_probe();
void		mace_intr(int device, void *ssp);
void		mace_init(int unit);
void		mace_tx_intr(int device, void *ssp),
		mace_rv_intr(int device, void *ssp);
void		mace_get_hwid(unsigned char *hwid_addr, mace_softc_t *m);
int		mace_output();
int		mace_ioctl();
void		mace_reset(int unit);
int		mace_watch();
void		mace_start(int);
void		mace_dbdma_rv_intr(int unit, void *),
		mace_dbdma_tx_intr(int, void *), mace_pci_intr(int, void *),
		__mace_dbdma_tx_intr(int, void *);
void		mace_setup_dbdma(int);
void		mace_xmt_dbdma(int unit, io_req_t req);
void		mace_send_packet_up(io_req_t req, struct ether_header *eh,
				    mace_softc_t *m);

unsigned char	mace_swapbits(unsigned char);

/* Variable and structure declarations */

static caddr_t mace_std[NLAN] = { 0 };
static struct bus_device *mace_info[NLAN];
struct	bus_driver	mace_driver = 
	{(probe_t)mace_probe, 0,
	 (void (*)(struct bus_device*))mace_attach, 0,
	 mace_std, "et", mace_info, };

mace_softc_t	mace_softc[NLAN];

#if DEBUG
#define LANDEBUG_TRACE   1	/* set if we want debugging tracing */
#define LANDEBUG_VERBOSE 2	/* set for packet contents too */
#define LANDEBUG_ENABLED 4	/* (sticky) set when an error occurs */

int landebug=LANDEBUG_TRACE; /* tracing enabled only on error */

#define KGDB_PRINTF(A) if ((landebug & LANDEBUG_TRACE) && \
			   (landebug & LANDEBUG_ENABLED)) { printf A; }
#else
#define KGDB_PRINTF(A)
#endif


static
void mace_dump(unsigned char *ptr, int bytes)
{
	printf("{");
	while (bytes-- > 0) 
		printf("%2.2x, ", *ptr++);
	printf("}");
}

/*
 * mace_get_hwid
 *
 *	This function computes the Ethernet Hardware address
 *	from PROM. (Its best not to ask how this is done.)
 */

void
mace_get_hwid(unsigned char *hwid_addr, mace_softc_t *m)
{
	int		hi;

	for (hi = 0; hi < 6; hi++, hwid_addr += 16) 
		m->address[hi] = mace_swapbits(*hwid_addr);
}

unsigned char
mace_swapbits(unsigned char bits)
{
	unsigned char	mask = 0x1, i, newbits = 0;

	for (i = 0x80; i; mask <<= 1, i >>=1) {
		if (bits & mask)
			newbits |= i;
	}

	return newbits;
}
	
	
/*
 * mace_probe:
 *
 *	This function "probes" or checks for the MACE board on the bus to see
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
mace_probe(port, dev)
struct bus_device	*dev;
{
	int			unit, i;
	char			inbuf[50];
	unsigned char		status;
	mace_softc_t		*m;
	struct mace_board	*ereg;
	struct enetdma		*edma;
	device_node_t		*macedev = NULL;
	spl_t			s;

	KGDB_PRINTF(("mace probing\n"));

	unit = dev->unit;

	if ((unit < 0) || (unit >= NLAN)) {
		return(0);
	}

	m = &mace_softc[unit];

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PCI:
		macedev = find_devices("mace");
		if (macedev == NULL) {
			printf("Warning: no MACE style ethernet found\n");
			return 0;
		}

		ereg = (struct mace_board *)
				POWERMAC_IO(macedev->addrs[0].address);
		edma = NULL;
		m->dbdma_txchan = (dbdma_regmap_t *)
					POWERMAC_IO(macedev->addrs[1].address);
		m->dbdma_rvchan = (dbdma_regmap_t *)
					POWERMAC_IO(macedev->addrs[2].address);
		break;

	case	POWERMAC_CLASS_PDM:
		
		/* create virtual pointer to registers ethernet uses */
		ereg = (struct mace_board *)(POWERMAC_IO(dev->address));
		edma = (struct enetdma *)(POWERMAC_IO((vm_offset_t)dev->sysdep1));
		break;

	default:
		printf("Warning: MACE Ethernet is not supported on this machine.\n");
		return 0;
	}

	
	simple_lock_init(&m->lock, ETAP_IO_CHIP);
	s = splimp();

	/* simple_lock(&m->lock); */

	m->base = ereg;
	m->ereg = ereg;
	m->edma = edma;
	m->set = SET_1;	/* Ethernet bug. use only one buffer. */

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PCI:
		mace_get_hwid((unsigned char *)
				POWERMAC_IO(PCI_ETHERNET_ADDR_PHYS), m);
		break;

	case	POWERMAC_CLASS_PDM:
		/* Read the PROM ethernet ID */
		mace_get_hwid((unsigned char *)POWERMAC_IO(dev->sysdep), m);
		break;
	}


	/* Reset the board & AMIC.. */
	mace_reset(unit);

	if (powermac_info.class == POWERMAC_CLASS_PDM) {
		/* Disable user test register */
		m->ereg->utr = UTR_RTRD; eieio();

		/* Disable all interrupts */
		m->ereg->imr = 0xff; 	eieio();

		m->ereg->biucc = BIUCC_XMTSP64;	/* Xmit after 64 byte preamble */
		m->ereg->xmtfc = XMTFC_APADXNT;
		m->ereg->rcvfc = 0;
		m->ereg->fifocc = (FIFOCC_XFWU|FIFOCC_XFW_16|FIFOCC_RFWU|FIFOCC_RFW_64);
		m->ereg->plscc = 0; eieio();
		//m->ereg->phycc = PHYCC_ASEL; eieio();	/* Auto select */

	}

	/* grab the MACE chip rev  */
	m->chip_id = (m->ereg->chipid2 << 8 | m->ereg->chipid1);

	if (m->chip_id != MACE_REVISION_A2) {
		m->ereg->iac = IAC_ADDRCHG|IAC_PHYADDR; eieio();

		while ((status = m->ereg->iac)) {
			if ((status & IAC_ADDRCHG) == 0)
				 break;
			eieio();
		}
	} else {
		/* start to load the address.. */
		m->ereg->iac = IAC_PHYADDR; eieio();
	}

	for (i = 0; i < 6; i++)  {
		m->ereg->padr = m->address[i]; eieio();
	}

	/* Now clear the Multicast filter */
	if (m->chip_id != MACE_REVISION_A2) {
		m->ereg->iac = IAC_ADDRCHG|IAC_LOGADDR; eieio();

		while ((status = m->ereg->iac)) {
			if ((status & IAC_ADDRCHG) == 0)
				 break;
			eieio();
		}
	} else {
		m->ereg->iac = IAC_LOGADDR; eieio();
	}

	for (i=0; i < 8; i++) 
		m->ereg->ladrf = 0xFF; eieio();


	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PDM:
		pmac_register_int(PMAC_DEV_ETHERNET, SPLIMP, mace_intr);
		pmac_register_int(PMAC_DMA_ETHERNET_RX, SPLIMP, mace_intr);
		pmac_register_int(PMAC_DMA_ETHERNET_TX, SPLIMP, mace_intr);

		/* Clear any receive dma interrupts */
		edma->rcvcs = DMARUN | IF;eieio();

		/* Clear Transmit DMA interrupts.. only enable when needed */
		edma->xmtcs  = IF; eieio();
		edma->rcvtp = 0; eieio();	/* Clear tail pointer */
		break;

	case	POWERMAC_CLASS_PCI:
		/* Don't use base Interrupt.. everything is controlled by
		 * DMA interrupts
		 */
		mace_setup_dbdma(unit);

		pmac_register_ofint(macedev->intrs[0], SPLIMP, mace_pci_intr);
	
		pmac_register_ofint(macedev->intrs[1], SPLIMP,
							mace_dbdma_tx_intr);
		pmac_register_ofint(macedev->intrs[2], SPLIMP,
							mace_dbdma_rv_intr);

		break;
	}

	/* simple_unlock(&m->lock); */
	splx(s);

	return(1);
}

/*
 * mace_setup_dbdma
 *
 * Setup various dbdma pointers.
 */

void
mace_setup_dbdma(int unit)
{
	mace_softc_t	*m;
	int		i;
	dbdma_command_t	*d;
	vm_offset_t	address;

	m = &mace_softc[unit];
	if (m->rv_dma_area == NULL) {
		m->rv_dma_area = (unsigned char *)
			io_map(NULL,
		   	  round_page(ETHERNET_DBDMA_BUFS * ETHERNET_BUF_SIZE));
		m->rv_dma = dbdma_alloc(ETHERNET_DBDMA_BUFS+2);
		m->tx_dma = dbdma_alloc(4);
	}

	d = m->rv_dma;

	/* Setup a circle buffer.. */
	for (i = 0; i < ETHERNET_DBDMA_BUFS; i++, d++) {
		address = kvtophys((vm_offset_t) &m->rv_dma_area[i*ETHERNET_BUF_SIZE]);
		DBDMA_BUILD(d, DBDMA_CMD_IN_LAST, 0, ETHERNET_BUF_SIZE,
			address, DBDMA_INT_ALWAYS, 
			DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	}

#if 0
	DBDMA_BUILD(d, DBDMA_CMD_STOP, 0, 0, 0, DBDMA_INT_NEVER,
			DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	d++;
#endif

	DBDMA_BUILD(d, DBDMA_CMD_NOP, 0, 0, 0, DBDMA_INT_NEVER,
			DBDMA_WAIT_NEVER, DBDMA_BRANCH_ALWAYS);

	address = kvtophys((vm_offset_t) m->rv_dma);
	dbdma_st4_endian(&d->d_cmddep, address);

	m->rv_head = 0;
	m->rv_tail = ETHERNET_DBDMA_BUFS - 1;

	dbdma_reset(m->dbdma_txchan);
	dbdma_reset(m->dbdma_rvchan);

	/* Set the wait value.. */
	dbdma_st4_endian(&m->dbdma_rvchan->d_wait, DBDMA_SET_CNTRL(0x20));
}


/*
 * mace_attach:
 *
 *	This function attaches a MACE board to the "system".  The rest of
 *	runtime structures are initialized here (this routine is called after
 *	a successful probe of the board).  Once the ethernet address is read
 *	and stored, the board's ifnet structure is attached and readied.
 *
 * input	: bus_device structure setup in autoconfig
 * output	: board structs and ifnet is setup
 *
 */
void mace_attach(dev)
struct bus_device	*dev;
{
	mace_softc_t	*sp;
	struct	ifnet	*ifp;
	int		unit;
	caddr_t		base;
	spl_t		s;

	s = splimp();
	unit = (dev->unit);
	sp = &mace_softc[unit];
	simple_lock(&sp->lock);
	base = (caddr_t)sp->base;
	
	sp->timer = -1;
	sp->flags = 0;
	sp->mode = 0;

	mace_geteh(sp, sp->ds_addr);
	mace_geteh(sp, sp->address);
	
	ifp = &(sp->ds_if);
	ifp->if_unit = unit;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST|IFF_MULTICAST|IFF_ALLMULTI;

	ifp->if_header_size = sizeof(struct ether_header);
	ifp->if_header_format = HDR_ETHERNET;
	ifp->if_address_size = 6;
	ifp->if_address = (char *)&sp->address[0];
	if_init_queues(ifp);
        ifp->if_snd.ifq_maxlen = 16;
	simple_unlock(&sp->lock);
	splx(s);

	printf("  ethernet id [%02x:%02x:%02x:%02x:%02x:%02x] ",
		sp->address[0], sp->address[1], sp->address[2], 
		sp->address[3], sp->address[4], sp->address[5]);

}

/*
 * mace_watch():
 *
 */
mace_watch(b_ptr)

caddr_t	b_ptr;

{
	int	x,
		y,
		unit;
	mace_softc_t *is;


	unit = *b_ptr;

	timeout((timeout_fcn_t)mace_watch,b_ptr,20*hz);

	is = &mace_softc[unit];
	printf("\nxmt/bad	rcv/bad	spurious\n");
	printf("%d/%d		%d/%d	%d\n", is->xmt, is->badxmt, \
		is->rcv, is->badrcv, is->spurious);
	is->rcv=is->badrcv=is->xmt=is->badxmt=is->spurious=0;

}

/*
 * mace_geteh:
 *
 *	This function gets the ethernet address (array of 6 unsigned
 *	bytes) from the MACE board prom. 
 *
 */

mace_geteh(sp, ep)
mace_softc_t	*sp;
char *ep;
{
	int	i;
	unsigned char ep_temp;

	KGDB_PRINTF(("It is mace_geteh()\n"));
	
	sp->ereg->iac = IAC_PHYADDR; eieio();
	
	KGDB_PRINTF(("\nethernet id is "));
	
	for (i = 0; i < ETHER_ADD_SIZE; i++) {
		/* outw(IE_GP(base), i); */
		/* *ep++ = inb(IE_SAPROM(base)); */
		
		ep_temp = sp->ereg->padr; eieio();
		KGDB_PRINTF(("%02x ", ep_temp));
		*ep++ = ep_temp;
	}
	KGDB_PRINTF(("\n\n"));
}

/*
 * mace_seteh:
 *
 *	This function sets the ethernet address (array of 6 unsigned
 *	bytes) on the MACE board. 
 */

mace_seteh(mace_softc_t *sp, char *ep)
{
	int	i;
	unsigned char	status;

	if (sp->chip_id != MACE_REVISION_A2) {
		sp->base->iac = IAC_ADDRCHG|IAC_PHYADDR; eieio();

		while ((status = sp->ereg->iac)) {
			if ((status & IAC_ADDRCHG) == 0)
				 break;
			eieio();
		}
	} else {
		/* start to load the address.. */
		sp->base->iac = IAC_PHYADDR; eieio();
	}

	for (i = 0; i < ETHER_ADD_SIZE; i++) {
		sp->base->padr = *(ep+i); eieio();
	}
}

mace_output(dev, ior)
	dev_t		dev;
	io_req_t	ior;
{
	register int	unit;
	int		ret;
	mace_softc_t	*sp;

	unit = minor(dev);

	if (unit < 0 || unit >= NLAN ||
	    mace_softc[unit].base == 0) {
	    return (ENXIO);
	}
    
	
	sp = &mace_softc[unit];
	ret = net_write(&sp->ds_if, mace_start, ior);

	return ret;
}

mace_setinput(dev, receive_port, priority, filter, filter_count, device)
	dev_t		dev;
	mach_port_t	receive_port;
	int		priority;
	filter_t	filter[];
	u_int		filter_count;
	device_t	device;
{
	register int unit;
	int	ret;
	mace_softc_t	*sp;

	unit = minor(dev);

	KGDB_PRINTF(("It is mace_setinput()\n"));

	if (unit < 0 || unit >= NLAN ||
	    mace_softc[unit].base == 0) {
	    return (ENXIO);
	}

	ret = (net_set_filter(&mace_softc[unit].ds_if,
			(ipc_port_t)receive_port, priority,
			filter, filter_count, device));

	return ret;
}

/*
 * mace_init:
 *
 *	Another routine that interfaces the "if" layer to this driver.  
 *	Simply resets the structures that are used by "upper layers".  
 *	As well as calling mace_hwrst that does reset the mace_ board.
 *
 * input	: board number
 * output	: structures (if structs) and board are reset
 *
 */	

void
mace_init(int unit)
{
	struct	ifnet	*ifp;
	int		stat;
	int		new_maccc;
	spl_t		oldpri;
	mace_softc_t	*sp;

	KGDB_PRINTF(("It is mace_init()\n"));

	sp = &mace_softc[unit];
	ifp = &(sp->ds_if);

	oldpri = splimp();
	ifp->if_flags |= IFF_RUNNING;
	sp->flags |= DSF_RUNNING;
	sp->timer = 5;
	splx(oldpri);

	switch (powermac_info.class) {
	case POWERMAC_CLASS_PDM:
		sp->edma->rcvcs = DMARUN| IE; eieio();
		break;

	case	POWERMAC_CLASS_PCI:
		dbdma_start(sp->dbdma_rvchan, sp->rv_dma);
		break;
	}
	return;

}

/*ARGSUSED*/
mace_open(dev, flag)
	dev_t	dev;
	int	flag;
{
	register int	unit;
	mace_softc_t *sp;
	extern net_pool_t inline_pagepool;
	spl_t	oldpri;

	
	unit = minor(dev);
	KGDB_PRINTF(("It is mace_open()\n"));
	
	if (unit < 0 || unit >= NLAN ||
	    mace_softc[unit].base == 0) {
	    return (ENXIO);
	}

	sp = &mace_softc[unit];


	/*
	 * Populate immediately buffer pool, since we are
	 * are registering a network device.
	 */
	net_kmsg_more(inline_pagepool);

	oldpri = splimp();
	simple_lock(&sp->lock);

	sp->ds_if.if_flags |= IFF_UP;
	mace_init(unit);

	/* Start the chip... */
	sp->ereg->maccc = MACCC_ENXMT|MACCC_ENRCV; eieio();

	if (sp->mode & MOD_PROM) {
		sp->ereg->maccc |= MACCC_PROM; eieio();
	}

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PCI:
		sp->ereg->imr = 0xfe; eieio();
		break;

	case	POWERMAC_CLASS_PDM:
		sp->ereg->imr = 0xff; eieio();
		break;
	}

	simple_unlock(&sp->lock);
	splx(oldpri);

	return(0);
}

void
mace_start(int unit)
{
	io_req_t	m;
	struct	ifnet	*ifp;
	spl_t	s = splimp();
	mace_softc_t	*sp;


	sp = &mace_softc[unit];
	simple_lock(&sp->lock);

	if (sp->set == 0) {
		/* No sets available right at the moment.. */
		simple_unlock(&sp->lock);
		splx(s);
		return;
	}

	ifp = &sp->ds_if;

	IF_DEQUEUE(&ifp->if_snd, m);

	if (m) {
		switch (powermac_info.class) {
		case	POWERMAC_CLASS_PDM:
			mace_xmt(unit, m);
			break;

		case	POWERMAC_CLASS_PCI:
			mace_xmt_dbdma(unit, m);
			break;
		}
	}

	/* We check the queue again at the mace_tx_intr not here */
	simple_unlock(&sp->lock);
	splx(s);
}

/*
 * Send a packet back to higher levels - used for looping back
 * packets to second servers etc.
 */

void
mace_send_packet_up(
	io_req_t		m,
	struct ether_header 	*eh,
	mace_softc_t		*is)
{
	ipc_kmsg_t kmsg;
	struct ether_header *ehp;
	struct packet_header *pkt;

	kmsg = net_kmsg_get();
	if (kmsg == IKM_NULL) {
		/*
		 * Drop the packet.
		 */
		is->ds_if.if_rcvdrops++;
		return;
	}
	ehp = (struct ether_header *) (&net_kmsg(kmsg)->header[0]);
	pkt = (struct packet_header *) (&net_kmsg(kmsg)->packet[0]);
	bcopy((char *)eh, (char *)ehp, sizeof(struct ether_header));
	bcopy(&m->io_data[sizeof(struct ether_header)], (char *)(pkt + 1),
	      m->io_count - sizeof(struct ether_header));
	pkt->type = ehp->ether_type;
	pkt->length = m->io_count - sizeof(struct ether_header) + sizeof(struct packet_header);
	if (pkt->length < ETHERMIN + sizeof(struct ether_header) + sizeof(struct packet_header))
		pkt->length = ETHERMIN + sizeof(struct ether_header) + sizeof(struct packet_header);
	net_packet(&is->ds_if, kmsg, pkt->length, ethernet_priority(kmsg), m);

}



/*
 * mace_xmt_dbdma
 *
 * Transmit a pack through the DBDMA
 */

void
mace_xmt_dbdma(int unit, io_req_t req)
{
	mace_softc_t	*m = &mace_softc[unit];
	struct mace_board *board = m->base;
	dbdma_command_t	*d = m->tx_dma;
	int	i;
	unsigned long page, count;
	register struct ether_header *eh_p = (struct ether_header *)req->io_data;

	if (powermac_info.io_coherent == FALSE)
		flush_dcache((vm_offset_t) req->io_data, req->io_count, FALSE);

	page = (unsigned long ) req->io_data & PAGE_MASK;

	if ((page + req->io_count) <= PAGE_SIZE) {
		DBDMA_BUILD(d, DBDMA_CMD_OUT_LAST, DBDMA_KEY_STREAM0,
			req->io_count,
			kvtophys((vm_offset_t) req->io_data), DBDMA_INT_NEVER, 
			/*DBDMA_WAIT_IF_FALSE*/DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	} else {
#if DEBUG
		printf("{ETH PAGE}");
#endif
		count = PAGE_SIZE - page;
		DBDMA_BUILD(d, DBDMA_CMD_OUT_MORE, DBDMA_KEY_STREAM0,
			count,
			kvtophys((vm_offset_t) req->io_data), DBDMA_INT_NEVER, 
			DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);

		DBDMA_BUILD(d, DBDMA_CMD_OUT_LAST,
			DBDMA_KEY_STREAM0, req->io_count - count,
			kvtophys((vm_offset_t) ((unsigned char *) req->io_data + count)), DBDMA_INT_NEVER, 
			/*DBDMA_WAIT_IF_FALSE*/DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	}

#if 0
	d++;
	DBDMA_BUILD(d, DBDMA_CMD_LOAD_QUAD, DBDMA_KEY_SYSTEM,
		1, kvtophys((vm_offset_t) &board->xmtfs), DBDMA_INT_NEVER, 
		DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);


	d++;
	DBDMA_BUILD(d, DBDMA_CMD_LOAD_QUAD, DBDMA_KEY_SYSTEM,
		1, kvtophys((vm_offset_t) &board->ir), DBDMA_INT_ALWAYS,
		DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);

#endif
	d++;
	DBDMA_BUILD(d, DBDMA_CMD_STOP, 0, 0, 0, 0, 0, 0);

	/*
	 * Loopback Ethernet packet if :
	 * 1) It's a Broadcast,
	 * 2) Device has been opened more than once (multi-server) and
	 *	packet destination address is mine,
	 * 3) Interface is in promiscuous mode,
	 * 4) It's a Multicast and interface receive all Multicast packets.
	 *
	 * XXX Don't forget to reflect also multicast if to be received
	 */

	if ((eh_p->ether_dhost[0] == 0xFF &&
	     eh_p->ether_dhost[1] == 0xFF &&
	     eh_p->ether_dhost[2] == 0xFF &&
	     eh_p->ether_dhost[3] == 0xFF &&
	     eh_p->ether_dhost[4] == 0xFF &&
	     eh_p->ether_dhost[5] == 0xFF) ||
	    (req->io_device != DEVICE_NULL &&
	     req->io_device->open_count > 1 &&
	     eh_p->ether_dhost[0] == m->ds_addr[0] &&
	     eh_p->ether_dhost[1] == m->ds_addr[1] &&
	     eh_p->ether_dhost[2] == m->ds_addr[2] &&
	     eh_p->ether_dhost[3] == m->ds_addr[3] &&
	     eh_p->ether_dhost[4] == m->ds_addr[4] &&
	     eh_p->ether_dhost[5] == m->ds_addr[5]) ||
	    (m->mode & MOD_PROM) ||
	    ((m->mode & MOD_ENAL) && (eh_p->ether_dhost[0] & 1)))
		mace_send_packet_up(req, eh_p, m);

	/* Kick off the real transmission */
	dbdma_start(m->dbdma_txchan, m->tx_dma);

	m->tx_active = req;
	m->set = 0;
}

/*
 * mace_dbdma_rv_intr
 *
 * DBDMA interrupt receive..
 */

void
mace_dbdma_rv_intr(int device, void *ignored)
{
	mace_softc_t		*m;
	struct mace_board	*board;
	struct ifnet		*ifp;
	ipc_kmsg_t		new_kmsg;
	struct ether_header	*ehp;
	struct packet_header	*pkt;
	dbdma_command_t		*d;
	unsigned char		*packet_header;
	unsigned short		status;
	long			bytes;
	unsigned long		resid;
	vm_offset_t		address;
	int			i, start, first_rebuilt = -1;


	m = &mace_softc[0];	/* XXX unit ? */
	simple_lock(&m->lock);
	board = m->base;
	ifp = &m->ds_if;
	for (i = start = m->rv_head; i != first_rebuilt;) {
		d = &m->rv_dma[i];

		resid = dbdma_ld4_endian(&d->d_status_resid);
		status = (resid >> 16);
		bytes  = resid & 0xffff;

		bytes = ETHERNET_BUF_SIZE - bytes;

		/* Is this packet complete? */
		if ((status & DBDMA_ETHERNET_EOP) == 0)  {
#if DEBUG 
			if (status & DBDMA_CNTRL_ACTIVE)
				printf("{OVERFLOW?}");
#endif /* DEBUG */
			++i;
			if (i >= ETHERNET_DBDMA_BUFS) 
				i = 0;
			if (i == m->rv_head) {
				break;
			} else
				continue;
		}

		DBDMA_ST4_ENDIAN(&d->d_cmd_count, DBDMA_CMD_STOP << 28);
		eieio();

		if (bytes < 8+sizeof(struct ether_header)) { 
			unsigned char *st;
			ifp->if_rcvdrops++;
#if DEBUG
			printf("{short net pkt %d/%d/%x}",
				     bytes, i, resid);
			if (bytes > 4) {
				st = (unsigned char *)&m->rv_dma_area;
				printf("{%x}", (st[bytes-1] << 24)|
			(st[bytes-2]<<16) + (st[bytes-3]<<8) | (st[bytes-4]));
			}
#endif /* DEBUG */			
			goto rebuild;
		} 
		
		//mace_dump(&m->rv_dma_area[i*ETHERNET_BUF_SIZE], bytes);
		new_kmsg = net_kmsg_get();
		if (new_kmsg == IKM_NULL) {
			ifp->if_rcvdrops++;
#if DEBUG
#if	MACH_KDB
			/*
			 * We don't want a bunch of "drop" messages
			 * each time we continue from the kernel debugger.
			 */
			if (landebug & LANDEBUG_VERBOSE)
#endif	/* MACH_KDB */
			printf("{drop}");
#endif /* DEBUG */
		} else {
			ehp = (struct ether_header *)
					(&net_kmsg(new_kmsg)->header[0]);
			pkt = (struct packet_header *)
					(&net_kmsg(new_kmsg)->packet[0]);
		
			packet_header = &m->rv_dma_area[i*ETHERNET_BUF_SIZE];

			bytes -= 4+sizeof(struct ether_header);
	
			bcopy_nc((char *)packet_header, (char *) ehp, 
				sizeof(struct ether_header));
			bcopy_nc(packet_header + sizeof(struct ether_header),
	      				(char *)(pkt + 1), bytes);

			pkt->type = ehp->ether_type;
			pkt->length = bytes + sizeof(struct packet_header);
	
			net_packet(ifp, new_kmsg, pkt->length,
				   ethernet_priority(new_kmsg), (io_req_t)0);
		}

		/* Rebuild entry... */
rebuild:
		address = kvtophys((vm_offset_t) &m->rv_dma_area[i*ETHERNET_BUF_SIZE]);
		DBDMA_BUILD(d, DBDMA_CMD_IN_LAST, 0, ETHERNET_BUF_SIZE,
			address, DBDMA_INT_ALWAYS,
			DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
		eieio();

		if (first_rebuilt == -1)
			first_rebuilt = i;

		i++;
		if (i >= ETHERNET_DBDMA_BUFS) 
			i = 0;

		if (i == m->rv_head) {
			break;
		}
	}

	dbdma_continue(m->dbdma_rvchan);

	if (first_rebuilt != -1)
		m->rv_head = first_rebuilt;
	else
		m->rv_head = i;

	if (i == m->rv_tail) 
		m->rv_tail = start;

	simple_unlock(&m->lock);
}

/*
 * mace_pci_intr
 *
 * Service MACE interrupt
 */

void
mace_pci_intr(int device, void *ssp)
{
	mace_softc_t		*m;
	struct mace_board	*board;
	unsigned char	ir, retry, frame, packet, length;


	m = &mace_softc[0];	/* XXX unit ? */
	simple_lock(&m->lock);
	board = m->base;
	ir = m->ereg->ir; eieio();	/* Clear Interrupt */

	/* TODO - keep track of errors */

	packet = m->ereg->mpc; eieio();
	length = m->ereg->rntpc; eieio();

	if (ir & IR_XMTINT) {
		/* Handle xfer.. */
		retry = m->ereg->xmtrc; eieio(); /* Grab transmit retry count */
		frame = m->ereg->xmtfs; eieio();

		__mace_dbdma_tx_intr(0, ssp);	/* XXX unit ? */
	}

	simple_unlock(&m->lock);
}

/*
 *
 * mace_tx_intr
 *
 * Service the MACE/Ethernet Transmit Interrupt, we know that
 * IR_XMTINT was set before entering here.
 *
 */

void
mace_tx_intr(int device, void *ssp)
{
	io_req_t	m;
	struct	ifnet	*ifp;
	unsigned char	status, dma_status;
	mace_softc_t	*board = &mace_softc[0];	/* XXX unit ? */

	ifp = &board->ds_if;

	dma_status = board->edma->xmtcs; eieio(); /* Grab the DMA status */
	board->edma->xmtcs = IF;eieio();	/* Clear out the DMA */

	status = board->ereg->xmtrc; eieio();	/* Grab transmit retry count */

	dma_status |= SET_1;

	if (dma_status & SET_1) {	/* And clear out any counters (just in case) */
		board->edma->xmtbch_1 = 0;
		board->edma->xmtbcl_1 = 0;eieio();
	} 

#if 0
	/* Only using Set_1 for the moment */
	if (dma_status & SET_0) {
		board->edma->xmtbch_0 = 0;
		board->edma->xmtbcl_0 = 0; eieio();
	}
#endif

	/* Only grab one set.. Ethernet bug 
	 * (sometimes it locks up on doing double
	 * DMA'ing.. ARGH!)
	 */
	board->set = (dma_status & (SET_1));

	status = board->ereg->xmtfs; eieio();

	/* Check the status on the packet and
	 * if not valid, wait until we get a valid
	 * packet..
	 */

	if ((status & XMTFS_XNTSV) == 0) {
		return;
	}

	if (status & (0x7f))  {
		status = board->ereg->fifofc;	/* Grab fifo count */
		eieio();
	}


	if (board->set == 0) {
		return;
	}

	/* send the next packet */

	IF_DEQUEUE(&ifp->if_snd, m);

	if (m)
		mace_xmt(0, m);	/* XXX unit ? */
}

/* 
 * mace_dbdma_tx_intr
 *
 * DBDMA interrupt routine
 */

void
mace_dbdma_tx_intr(int device, void *ignored)
{
	mace_softc_t	*m;

	m = &mace_softc[0];	/* XXX unit ? */

	simple_lock(&m->lock);
	__mace_dbdma_tx_intr(0, ignored);	/* XXX unit ? */
	simple_unlock(&m->lock);
}

void
__mace_dbdma_tx_intr(int device, void *ignored)
{
	struct	ifnet	*ifp;
	mace_softc_t	*m = &mace_softc[0];	/* XXX unit ? */
	struct mace_board *board = m->base;

	io_req_t	finish_req = m->tx_active, req;

	ifp = &m->ds_if;

	dbdma_stop(m->dbdma_txchan);

	m->tx_active = NULL;

	m->set = SET_1;

	/* Clear out the transmit status from MACE */

	IF_DEQUEUE(&ifp->if_snd, req);

	if (req)
		mace_xmt_dbdma(0, req);	/* XXX unit ? */

	if (finish_req)
		iodone(finish_req);
}

/*ARGSUSED*/
mace_getstat(dev, flavor, status, count)
	dev_t		dev;
	int		flavor;
	dev_status_t	status;		/* pointer to OUT array */
	u_int		*count;		/* out */
{
	register int	unit = minor(dev);
	int retval;
	mace_softc_t	*sp;
	spl_t		s;

	KGDB_PRINTF(("It is mace_getstat()\n"));

	if (unit < 0 || unit >= NLAN ||
	    mace_softc[unit].base == 0) {
	    return (ENXIO);
	}

	sp = &mace_softc[unit];

	retval = (net_getstat(&sp->ds_if,
			    flavor,
			    status,
			    count));
	return retval;
}

mace_setstat(dev, flavor, status, count)
	dev_t		dev;
	int		flavor;
	dev_status_t	status;
	u_int		count;
{
	register int	unit = minor(dev);
	register mace_softc_t *sp;
	unsigned char new_maccc;
	spl_t	s;

	KGDB_PRINTF(("It is mace_setstat()\n"));

	if (unit < 0 || unit >= NLAN ||
	    mace_softc[unit].base == 0) {
	    return (ENXIO);
	}

	sp = &mace_softc[unit];
	s = splimp();
	simple_lock(&sp->lock);

	switch (flavor) {
	    case NET_STATUS:
	    {
		/*
		 * All we can change are flags, and not many of those.
		 */
		register struct net_status *ns = (struct net_status *)status;
		int	mode = 0;

		if (count < NET_STATUS_COUNT) {
			simple_unlock(&sp->lock);
			splx(s);
		    return (D_INVALID_SIZE);
		}

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
			    /* Let network layer know whether this
			     * interface is promiscuous */
			    sp->ds_if.if_flags &= ~IFF_PROMISC;
			    sp->ds_if.if_flags |= (ns->flags & IFF_PROMISC);

			    new_maccc = sp->ereg->maccc & ~MACCC_PROM; eieio();

			    if (sp->mode & MOD_PROM) 
				    new_maccc |= MACCC_PROM;

			    sp->ereg->maccc = new_maccc; eieio();
		    }
		}
		break;
	    }
	    case NET_ADDRESS:
	    {
		register union ether_cvt {
		    char	addr[6];
		    int		lwd[2];
		} *ec = (union ether_cvt *)status;

		if (count < sizeof(*ec)/sizeof(int)) {
			simple_unlock(&sp->lock);
		    return (D_INVALID_SIZE);
		}

		ec->lwd[0] = ntohl(ec->lwd[0]);
		ec->lwd[1] = ntohl(ec->lwd[1]);
   		mace_seteh(sp, ec->addr);
		break;
	    }
	    case NET_ADDMULTI: /* Michel Pollet */
		/*
		 * Don't cry on that message, it's useful for the
		 * upper layer to handle multicast properly
		 */
		break;
	    default:
	 	simple_unlock(&sp->lock);
		splx(s);
		return (D_INVALID_OPERATION);
	}

	simple_unlock(&sp->lock);
	splx(s);
	return (D_SUCCESS);
}

/*
 * mace_reset
 *
 * Reset the board..
 */

void
mace_reset(int unit)
{
	unsigned char	status;
	mace_softc_t *sp;
	spl_t	oldpri;
	
	sp = &mace_softc[unit];

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PDM:
		sp->edma->rcvcs =  RST; eieio();
		sp->edma->xmtcs = RST; eieio();
		sp->ereg->maccc = 0; eieio();
		sp->ereg->fifocc |= FIFOCC_RFWU; eieio();
		break;

	case	POWERMAC_CLASS_PCI:
		dbdma_reset(sp->dbdma_rvchan);
		dbdma_reset(sp->dbdma_txchan);
		return;	/* Nothing else needs to be done! */
	}
	
	sp->ereg->fifocc |= FIFOCC_XFWU; eieio();

	status = sp->ereg->ir; eieio();	/* Clear out any interrupts */
	sp->ereg->biucc = BIUCC_SWRST;eieio();	/* Reset board */
	
	while (sp->ereg->biucc & BIUCC_SWRST)
		eieio();	/* Wait until the board is reset */

	sp->ereg->maccc = 0; eieio();
	sp->ereg->imr = 0xff; eieio(); /* Disable all interrupts */

	return;
}

/* 
 * mace_reset_recv
 *
 * Reset the board's receiving side. (Usually in
 * response to an error.)
 */

mace_reset_recv(int unit)
{
	unsigned char maccc, status;
	mace_softc_t *sp;

	sp = &mace_softc[0];	/* XXX unit ? */

	maccc = sp->ereg->maccc; eieio();	/* Save off the configuration */
	sp->ereg->maccc = (maccc & ~MACCC_ENRCV); eieio();

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PDM:
		/* Reset DMA */
		sp->edma->rcvcs = RST; eieio();
		break;

	case	POWERMAC_CLASS_PCI:
		dbdma_reset(sp->dbdma_rvchan);
		break;
	}

	/* Reset Receive FIFO */
	sp->ereg->fifocc &= ~FIFOCC_RFWU; eieio();

	/* Now reset original MACCC value */
	sp->ereg->maccc = maccc; eieio();

	/* And.. enable interrupts once again.. */
	sp->ereg->maccc = maccc | MACCC_ENXMT | MACCC_ENRCV; eieio();

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PDM:
		sp->edma->rcvcs = DMARUN | IE; eieio();
		sp->edma->rcvtp = sp->edma->rcvhp; eieio();
		break;

	case	POWERMAC_CLASS_PCI:
		mace_setup_dbdma(unit);
		dbdma_start(sp->dbdma_rvchan, sp->rv_dma);
		break;
	}

	return;
}

	
void
mace_intr(int device, void *ssp)
{

	unsigned char status;
	mace_softc_t *sp;

	sp = &mace_softc[0];	/* XXX unit ? */

	simple_lock(&sp->lock);

	/* Get interrupt status, this clears on read */
	status = sp->ereg->ir; eieio();

	if (status & IR_XMTINT)
		mace_tx_intr(0, ssp);	/* XXX unit ? */
	if (status & IR_RCVINT)
		mace_rv_intr(0, ssp);	/* XXX unit ? */

	simple_unlock(&sp->lock);
}

/*
 * mace_rv_intr:
 *
 *	This function is the interrupt handler for the mace_ ethernet
 *	board.  This routine will be called whenever either a packet
 *	is received.
 *
 * input	: board number that interrupted
 * output	: a packet is received
 *
 */

void
mace_rv_intr(int device, void *ssp)
{
	int stat;
	ipc_kmsg_t	new_kmsg;
	struct ether_header *ehp;
	struct packet_header *pkt;
	u_short	len;
	int truelen;
	unsigned int	pointer;
	register struct ifnet *ifp;
	register mace_softc_t	*is;
	register struct	ifqueue		*inq;
	unsigned char *rcvtp, *rcvtp_v, *status, *packet_header;
	unsigned char dma_status;

	is = &mace_softc[0];	/* XXX unit ? */

	dma_status = is->edma->rcvcs; eieio();

	if ((dma_status & IF) == 0) {
		is->edma->rcvcs = DMARUN|IE;eieio();
		return;
	}

	is->edma->rcvcs = DMARUN | IF; eieio();

	if (dma_status & OVERRUN) {
		/* DMA over run.. */
		is->edma->rcvcs = OVERRUN;eieio(); /* Clear out the overrun */
#if DEBUG
		printf("{MACE_RESET_RECV(overrun)}\n");
#endif /* DEBUG */
		mace_reset_recv(0);	/* XXX unit ? */
		/* *sigh* Everything is lost */
		return;
	}

	ifp = &is->ds_if;

	is->rcv++;
	
	for (;;) {
		pointer = is->edma->rcvtp;

		if (is->edma->rcvhp == pointer)
			break;

		rcvtp   = (unsigned char*)(DMA_RCV_BASE  +(pointer << 8));
		rcvtp_v = (unsigned char*)(DMA_RCV_BASE_V +(pointer << 8));
		
		status = rcvtp_v;
		
		len = *status++ << 8;
		len = len + *status;
		
		if (len & 0xf000) {
			/* Crap! bad status from the board!
			 * The only work around is to
			 * reset the receive side, and lose
			 * all info!
			 */
#if DEBUG
			printf("{MACE_RESET_RECV(len)}\n");
#endif /* DEBUG */
			mace_reset_recv(0);	/* XXX unit ? */
			return;	
		}

		len = len & 0x0fff;
		packet_header = rcvtp_v + 8;
		
		rcvtp = rcvtp + 8 + len; /* status + frame */
		rcvtp = (char*)(((unsigned)rcvtp + 0xff) & ~0xff);
		
		if (rcvtp >= (unsigned char*)(DMA_RCV_BASE + DMA_RCV_SIZE)) {
			/* we've overrun circular buffer, wrap around */
			rcvtp -= DMA_RCV_SIZE;
		}
		
	    	truelen = len - sizeof(struct ether_header) - 4;

		if (truelen < 0) { 
			is->ds_if.if_rcvdrops++;
#if DEBUG
			printf("{MACE_RESET_RECV(-len)}\n");
#endif /* DEBUG */
			mace_reset_recv(0);	/* XXX unit ? */
			return;
		}
		
		/* packet header is within first 256 bytes, no risk of wrap */
		
		new_kmsg = net_kmsg_get();
		if (new_kmsg == IKM_NULL) {
			is->ds_if.if_rcvdrops++;
			
			is->edma->rcvtp = ((unsigned long)rcvtp >> 8) & 0xff; eieio();
			continue;
		}

		ehp = (struct ether_header *)(&net_kmsg(new_kmsg)->header[0]);		
		pkt = (struct packet_header *)(&net_kmsg(new_kmsg)->packet[0]);
		
		bcopy_nc((char *)packet_header, (char *) ehp,  sizeof(struct ether_header));

		KGDB_PRINTF(("copying received packet to %x\n", (char *)(pkt+1))); 
		
		/* Check if new backpointer is before current packet,
		 * ie. the backpointer has wrapped around the circular buffer.
		 * If so, and if new backpointer is after start of circular
		 * buffer, then a packet is split over the wraparound
		 */
		if ((rcvtp < packet_header) &&
		    (rcvtp > (unsigned char*)DMA_RCV_BASE)) {


			/* We've got a packet that is wrapped around the
			 * end of the circular buffer, so it must be
			 * copied in two parts
			 */
			char *ptr;
			int  mid_len;

			ptr = packet_header + sizeof(struct ether_header);

			KGDB_PRINTF(("WRAPAROUND on buffer size %d"
				     "(with header+crc,size=%d)\n",
				     truelen,len));
			mid_len = DMA_RCV_SIZE - ((unsigned int)ptr &
						  (DMA_RCV_SIZE-1));
			/* This gets even more complicated - the last
			 * 4 bytes aren't copied as they're the CRC,
			 * and the wraparound may occur here. Deal with
			 * this too!
			 */
			assert ((mid_len > 0) && (mid_len < (truelen+4)));

			if (mid_len >= truelen) {
				/* The wraparound occurs on the 4 byte CRC
				 * only copy stuff in the actual packet
				 */
				bcopy_nc((char*)ptr, (char *)(pkt + 1), truelen);
			} else {
				/* The wraparound occurs mid-packet, copy
				 * the two halves
				 */

				/* make sure we don't copy too much */
				assert(mid_len < truelen);

				bcopy_nc((char*)ptr, (char *)(pkt + 1), mid_len);
				bcopy_nc((char*)DMA_RCV_BASE,
				      (char *)(pkt + 1) + mid_len,
				      (truelen - mid_len));
			}
		} else {
			/* Normal case - buffer is contiguous */

			bcopy_nc((char*)packet_header + sizeof(struct ether_header),
			      (char *)(pkt + 1),
			      truelen);
		}

		pkt->type = ehp->ether_type;
		pkt->length = len - 4 - sizeof(struct ether_header) + sizeof(struct packet_header);
		
		net_packet(ifp, new_kmsg, pkt->length,
			   ethernet_priority(new_kmsg), (io_req_t)0);


		is->edma->rcvtp = ((unsigned long)rcvtp >> 8) & 0xff; eieio();
	}

	/* Start another receive transaction! */
	is->edma->rcvcs = DMARUN | IE; eieio();
	
}		

/* new xmt function */

mace_xmt(unit, m)
int	unit;
io_req_t	m;
{
	int		i;
	mace_softc_t	*is = &mace_softc[unit];
	u_short		count = 0;
	u_short		bytes_in_msg;
	unsigned char   cs, xmtrc, xmtfs;
	unsigned char   *xmt_base, *ch_ptr;
	vm_offset_t	phys_xmt_base;
	register struct ether_header *eh_p = (struct ether_header *)m->io_data;
	
	is->xmt++;	
	count = m->io_count;
	
#define max(a, b) 	(((a) > (b)) ? (a) : (b))

	bytes_in_msg = max(count, ETHERMIN + sizeof(struct ether_header));
	
	xmt_base = (unsigned char *) DMA_XMT_BASE_0_V;
	
	if (is->set & SET_1) {
		xmt_base = (unsigned char *) DMA_XMT_BASE_1_V;
		is->set &= ~SET_1;
	} else {
		xmt_base = (unsigned char *) DMA_XMT_BASE_0_V;
		is->set &= ~SET_0;
	}

	bcopy_nc(m->io_data, xmt_base, count);

	/* Its going to be garbaged message */
	if (count < bytes_in_msg) 
		count = bytes_in_msg;

	if ((int)xmt_base == DMA_XMT_BASE_0_V) {
		is->edma->xmtbch_0 = (count >> 8) & 0xf; eieio();
		is->edma->xmtbcl_0 = count & 0xff; eieio();
	} else {
		is->edma->xmtbch_1 = (count >> 8) & 0xf; eieio();
		is->edma->xmtbcl_1 = count & 0xff; eieio();
	}
	
	count = is->edma->xmtcs; eieio();	/* Clear out the IF flag */

	is->edma->xmtcs = DMARUN | IE; eieio();

	/*
	 * Loopback Ethernet packet if :
	 * 1) It's a Broadcast,
	 * 2) Device has been opened more than once (multi-server) and
	 *	packet destination address is mine,
	 * 3) Interface is in promiscuous mode,
	 * 4) It's a Multicast and interface receive all Multicast packets.
	 *
	 * XXX Don't forget to reflect also multicast if to be received
	 */

	if ((eh_p->ether_dhost[0] == 0xFF &&
	     eh_p->ether_dhost[1] == 0xFF &&
	     eh_p->ether_dhost[2] == 0xFF &&
	     eh_p->ether_dhost[3] == 0xFF &&
	     eh_p->ether_dhost[4] == 0xFF &&
	     eh_p->ether_dhost[5] == 0xFF) ||
	    (m->io_device != DEVICE_NULL &&
	     m->io_device->open_count > 1 &&
	     eh_p->ether_dhost[0] == is->ds_addr[0] &&
	     eh_p->ether_dhost[1] == is->ds_addr[1] &&
	     eh_p->ether_dhost[2] == is->ds_addr[2] &&
	     eh_p->ether_dhost[3] == is->ds_addr[3] &&
	     eh_p->ether_dhost[4] == is->ds_addr[4] &&
	     eh_p->ether_dhost[5] == is->ds_addr[5]) ||
	    (is->mode & MOD_PROM) ||
	    ((is->mode & MOD_ENAL) && (eh_p->ether_dhost[0] & 1)))
		mace_send_packet_up(m, eh_p, is);

	iodone(m);
}
