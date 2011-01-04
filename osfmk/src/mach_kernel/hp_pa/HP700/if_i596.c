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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * Copyright (c) 1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
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
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: if_i596.c 3.41 94/12/14$
 */
/* 
 *	Olivetti PC586 Mach Ethernet driver v1.0
 *	Copyright Ing. C. Olivetti & C. S.p.A. 1988, 1989
 *	All rights reserved.
 *
 */ 
/*
  Copyright 1988, 1989 by Olivetti Advanced Technology Center, Inc.,
Cupertino, California.

		All Rights Reserved

  Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Olivetti
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

  OLIVETTI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL OLIVETTI BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUR OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
 * NOTE:
 *		by rvb:
 *  1.	The best book on the 82586 is:
 *		LAN Components User's Manual by Intel
 *	The copy I found was dated 1984.  This really tells you
 *	what the state machines are doing
 *  2.	In the current design, we only do one write at a time,
 *	though the hardware is capable of chaining and possibly
 *	even batching.  The problem is that we only make one
 *	transmit buffer available in sram space.
 *  3.	
 *  n.	Board Memory Map (require 32-byte cache alignment)
	RFA/FD	0x0000 - 0x02df	0x2e0 bytes
		 0x2e0 = 0x17 * 0x20 bytes
	RBD	0x02e0 - 0x369f	0x33c0 bytes
		0x33c0 = 0x17 * 0x240 bytes
				== 0x20 bytes (bd) + 0x220 bytes
	CU	0x36a0 - 0x379f	0x100 bytes
	TBD	0x37a0 - 0x395f	0x1c0 bytes
		 0x1c0 = No 14 * 0x20 bytes
	TBUF	0x3960 - 0x3f9f	0x640 bytes (> 1594(10))
	SCB	0x3fa0 - 0x3fbf	0x20 bytes
	ISCP	0x3fc0 - 0x3fdf	0x20 bytes
	SCP	0x3fe0 - 0x3fff	0x20 bytes
 *		
 */

/*
 * NOTE:
 *
 *	Currently this driver doesn't support trailer protocols for
 *	packets.  Once that is added, please remove this comment.
 */

#if 0
#define DEBUG
#define WATCHDOG
#define IF_CNTRS	MACH
#endif

#include "lan.h"
#if NLAN > 0

#include <kgdb.h>
#include <kgdb_lan.h>
#include <xkmachkernel.h>
#include <xk_debug.h>

#include <types.h>
#include <kern/spl.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <machine/machparam.h>
#include <machine/asp.h>
#include <machine/cpu.h>
#include <machine/pdc.h>

#include <kern/spl.h>
#include <mach/machine/vm_types.h>

#include <kern/time_out.h>
#include <device/device_types.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <device/if_hdr.h>
#include <device/if_ether.h>
#include <device/net_status.h>
#include <device/net_io.h>
#include <device/conf.h>

#if XKMACHKERNEL
#include <xkern/include/xkernel.h>
#include <xkern/protocols/eth/eth_i.h>

/*
 * When used with promiscous traffic,
 * x-kernel traffic is identified by
 * the following interval.
 */
#define XK_LOWEST_TYPE  0x3000
#define XK_HIGHEST_TYPE 0x3fff

static XObj self_protocol[NLAN];

#if     XK_DEBUG
int tracexkecp;
#endif /* XK_DEBUG */

#if XKSTATS
unsigned int pc586_xkior_error;
unsigned int pc586_xkior_queued;
unsigned int pc586_xkior_dropped;
unsigned int pc586_xkior_lost;
#endif /* XKSTATS */

#define   IS_XKERNEL(X, is)	\
		(((is)->xk_ltype) <= (X) && (X) < ((is)->xk_htype))

struct pc586_xk {
    unsigned int	max;	/* maximum length to copy */
    unsigned int	len;	/* current length copied */
    void		*addr;	/* buffer to copy to */
};

#else  /* XKMACHKERNEL */
#define   IS_XKERNEL(X, nic)     0
#endif /* XKMACHKERNEL */

#include <hp_pa/HP700/device.h>
#include <hp_pa/HP700/if_i596var.h>

#include <hp_pa/misc_protos.h>
#include <kern/misc_protos.h>
#include <device/eth_common.h>   /* machine-independent utilities */
#include <device/ds_routines.h>
#include <hp_pa/trap.h>
#include <hp_pa/HP700/bus_protos.h>
#include <hp_pa/endian.h>

#if KGDB_LAN
#include <kgdb/kgdb.h>
#endif

/*
 * Forwards
 */
void pc586xmt(int, io_req_t);
int pc586output(dev_t, io_req_t);
int pc586getstat(dev_t, int, dev_status_t, unsigned int*);
int pc586setstat(dev_t, int, dev_status_t, unsigned int);
void pc586reset(dev_t);
int pc586setinput(dev_t, ipc_port_t, int, filter_t[], unsigned int, device_t);
void pc586rustrt(int);
void pc586ack(int);
void pc586rcv(int);
unsigned short ptr_to_ram(char	*, int);
volatile char *ram_to_ptr(unsigned short, int);
int bad_rbd_chain(unsigned short, int);
int pc586requeue(int, volatile fd_t *);

/*
 * Externs
 */ 
#if KGDB_LAN
extern int pc586debug_read(int, volatile fd_t *);
#endif

boolean_t
pc586_multicast_insert(
	pc_softc_t *,
	net_multicast_t *);

boolean_t
pc586_multicast_remove(
	pc_softc_t *,
	net_multicast_t *);

void
pc586_multicast_create(
	net_multicast_t *);

void
pc586send_packet_up(
	io_req_t		m,
	struct ether_header	*eh,
	pc_softc_t		*is,
	int			unit);

#if XKMACHKERNEL
boolean_t
pc586_xkcopy(
    char *,
    long,
    void *);

boolean_t
pc586_xkcopy_test(
    char *,
    long,
    void *);

boolean_t
pc586_xkcopy16(
    char *,
    long,
    void *);

boolean_t
pc586_xkcopy16_test(
    char *,
    long,
    void *);


xkern_return_t
xkec_openenable(
    XObj, 
    XObj, 
    XObj,
    Part);

io_return_t
xknet_write(
    struct ifnet *,
    void (*)(int unit),
    io_req_t);

boolean_t
free_ior(
    io_req_t);

xmsg_handle_t
xkec_push(
    XObj,
    Msg);

int
xkec_control(
    XObj,
    int,
    char *,
    int);

xkern_return_t
xkec_init(
    XObj);
#endif /* XKMACHKERNEL */

#define	LAN_BUFF_SIZE	(0x4000 + 16)

char	pc586sram[LAN_BUFF_SIZE+32];	/* equiv mapped to make vtop simple */
int	xmt_watch = 0;

pc_softc_t	pc_softc[NLAN];


#ifdef	PC596XMT_BUG
/*
 * The PC596 board on hp700's apparently can lose CX interrupts
 * (i.e. interrupts when transmit is complete).  We used to deal
 * with this by a 3-second watchdog timeout.  This is not adequate
 * for the newer, smaller, all-on-one-board hp710's and hp705's;
 * these newer machines lose many more CX interrupts when busy,
 * seriously degrading network performance.
 * 
 * A simple datapoint illustrates this more clearly.  A 730 which
 * transmitted 4.7 million packets lost 21 CX interrupts.  A 710
 * which transmitted 134 thousand packets lost 139 interrupts.
 *
 * When PC596XMT_BUG is #define'd: On packet transmission, determine
 * how long to wait for CX interrupt.  When a CX interrupt arrives,
 * clear this timer.  After each call to hardclock() (see "trap.c"),
 * if the current time exceeds our timer, set the timer's "tv_sec"
 * field to "-1" and create a LAN interrupt.
 *
 * Note: these data structures are accessed in hardclock(); hence
 * they are not part of `pc_softc_t'.
 *
 * Note: If pc586_waitxmt is too small then you'll end up declaring
 * tx interrupts lost too quickly (hardware may still be retransmitting
 * due to collisions).  When that happens you'll walk on-top of the
 * hardware tx buffer and trash the packet the hardware is still trying
 * to put on the wire.
 *
 * Simple version of the hardware backoff alg:
 *
 * while (retries < MAX)
 *   wait 2**min(retries,10) * slot time
 *
 * By default MAX == 15 and the slot time is ~50us.  Compute it out
 * and it would seem that a packet could live in the hardware for
 * about 332750us.  Round it an even 350000 for a little slack.
 *
 */
struct timeval pc586_toutxmt[NLAN];	/* last time packet xmit'd */
long pc586_waitxmt = 350000;		/* us. to wait b4 declaring CX lost */
int pc586_eir[NLAN];			/* prochpa lan interrupt register */
extern unsigned char lan_addr[];	/* Core LAN address */

#define pc586inton(unit) \
    { \
	extern int asp2procibit[]; \
	register int ibit = aspitab(INT_LAN, SPLLAN, pc586intr , unit); \
	if (ibit > 0 && asp2procibit[ibit] > 0) \
		pc586_eir[unit] = asp2procibit[ibit]; \
	else \
		pc586_eir[unit] = 0; \
    }
#define pc586intoff(unit) \
    { \
	aspitab(INT_LAN, SPLLAN, (void (*)(int))NULL, unit); \
	pc586_eir[unit] = 0; \
	timerclear(&pc586_toutxmt[unit]); \
    }
#else	/* !PC596XMT_BUG */
#define pc586inton(unit)  (void)aspitab(INT_LAN,SPLLAN, (void (*)(int))pc586intr,unit);
#define pc586intoff(unit) (void)aspitab(INT_LAN,SPLLAN, (void (*)(int))NULL, unit);
#endif	/* PC596XMT_BUG */
#define pc586chatt(unit)  pc_softc[unit].hwaddr->attn = 0

#define pc586putport(lanp, data) \
    { \
	extern int isgecko; \
	if (isgecko) { \
		(lanp)->port = (data); \
		DELAY(1000); \
		(lanp)->port = (data) >> 16; \
		DELAY(1000); \
	} else { \
		(lanp)->port = (data) >> 16; \
		DELAY(1000); \
		(lanp)->port = (data); \
		DELAY(1000); \
	 } \
    }

struct pc586_cntrs {
	struct {
		unsigned int xmt, xmti, xmtsi;
		unsigned int defer;
		unsigned int busy;
		unsigned int sleaze, intrinsic, intrinsic_count;
		unsigned int chain;
	} xmt; 
	struct {
		unsigned int rcv;
		unsigned int ovw;
		unsigned int crc;
		unsigned int frame;
		unsigned int rscerrs, ovrnerrs;
		unsigned int partial, bad_chain, fill;
	} rcv;
	unsigned int watch;
	unsigned int xmtwatch;
} pc586_cntrs[NLAN];

#ifdef	DEBUG
int	pc586debug = 0;
void dumpbytes(unsigned char *, int);
#endif

#ifdef	WATCHDOG
void	pc586watch(int);

int	pc586timo = 5;
#endif

#ifdef	IF_CNTRS
int pc586_narp = 1, pc586_arp = 0;
int pc586_ein[32], pc586_eout[32]; 
int pc586_lin[128/8], pc586_lout[128/8]; 
static
log_2(unsigned long no)
{
	return ({ unsigned long _temp__;
		asm("bsr %1, %0; jne 0f; xorl %0, %0; 0:" :
		    "=r" (_temp__) : "a" (no));
		_temp__;});
}
#endif

/*
 * pc586probe:
 *
 *	This function "probes" or checks for the pc586 board on the bus to see
 *	if it is there.  As far as I can tell, the best break between this
 *	routine and the attach code is to simply determine whether the board
 *	is configured in properly.  Currently my approach to this is to write
 *	and read a word from the SRAM on the board being probed.  If the word
 *	comes back properly then we assume the board is there.  The config
 *	code expects to see a successful return from the probe routine before
 *	attach will be called.
 *
 * input	: address device is mapped to, and unit # being checked
 * output	: a '1' is returned if the board exists, and a 0 otherwise
 *
 */
int
pc586probe(struct hp_device *dev)
{
	caddr_t		addr = dev->hp_addr;
	int		unit = dev->hp_unit;
	volatile char	*b_sram;
	pc_softc_t	*sp = &pc_softc[unit];
	extern int	isgecko, prom_addr;
	unsigned char	*br;

	if (unit < 0 || unit > NLAN) {
		printf("pc%d: board out of range [0..%d]\n", unit, NLAN);
		return(0);
	}

	if (addr < (caddr_t)0xF0000000)
		return(0);

	b_sram = (volatile char *) (((int)pc586sram + 31) & ~31);
#ifdef DEBUG
	if (pc586debug)
		printf("pc%d: probe: addr=0x%x pc586sram=0x%x b_sram=0x%x\n",
		       unit, dev->hp_addr, pc586sram, b_sram);
#endif

	sp->seated = TRUE;
	sp->sram = b_sram;
	return(1);
}

/*
 * pc586attach:
 *
 *	This function attaches a PC586 board to the "system".  The rest of
 *	runtime structures are initialized here (this routine is called after
 *	a successful probe of the board).  Once the ethernet address is read
 *	and stored, the board's ifnet structure is attached and readied.
 *
 * input	: hp_device structure setup in autoconfig
 * output	: board structs and ifnet is setup
 *
 */
void
pc586attach(struct hp_device *dev)
{
	struct	ifnet	*ifp;
	unsigned char	unit = (unsigned char)dev->hp_unit;	
	pc_softc_t	*sp = &pc_softc[unit];
	volatile scb_t	*scb_p;
	int		i;

#ifdef WATCHDOG
	sp->timer = -1;
	sp->xmttimer = -1;
#endif
	sp->mode = 0;
	sp->flags = 0;
	sp->open = 0;
	sp->sc_mcast.mc_count = 0;
	queue_init(&sp->mcast_queue);
	for (i = 0; i < MULTICAST_SIZE; i++)
	    sp->mcast_hash[i] = 0;
#ifdef WATCHDOG
	driver_lock_init(&sp->lock, unit, pc586start, pc586intr,
			 pc586watch, (void (*)(int))0);
#else
	driver_lock_init(&sp->lock, unit, pc586start, pc586intr,
			 (void (*)(int))0, (void (*)(int))0);
#endif
	sp->hwaddr = (struct core_lan *) dev->hp_addr;
	sp->hwaddr->reset = 0;
	for (i = 0; i < 6; i ++)
		sp->ds_addr[i] = lan_addr[i];
	printf("ethernet id [%x:%x:%x:%x:%x:%x]\n",
		sp->ds_addr[0], sp->ds_addr[1], sp->ds_addr[2],
		sp->ds_addr[3], sp->ds_addr[4], sp->ds_addr[5]);

	scb_p = (volatile scb_t *)(sp->sram + OFFSET_SCB);
	scb_p->scb_crcerrs = 0;			/* initialize counters */
	scb_p->scb_alnerrs = 0;
	scb_p->scb_rscerrs = 0;
	scb_p->scb_ovrnerrs = 0;
	flushscball(scb_p);

	ifp = &(sp->ds_if);
	ifp->if_unit = unit;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = NET_STATUS_BROADCAST | NET_STATUS_MULTICAST;
	ifp->if_header_size = sizeof(struct ether_header);
	ifp->if_header_format = HDR_ETHERNET;
	ifp->if_address_size = 6;
	ifp->if_address = (char *)&sp->ds_addr[0];
	if_init_queues(ifp);
}

/*
 * pc586reset:
 *
 *	This routine is in part an entry point for the "if" code.  Since most 
 *	of the actual initialization has already (we hope already) been done
 *	by calling pc586attach().
 *
 * input	: unit number or board number to reset
 * output	: board is reset
 *
 */
void
pc586reset(dev_t unit)
{
	spl_t	oldpri;
	driver_lock_state();

	oldpri = spllan();
	(void)driver_lock(&pc_softc[unit].lock,
			  DRIVER_OP_WAIT, FALSE);
	pc_softc[unit].ds_if.if_flags &= ~NET_STATUS_RUNNING;
	pc_softc[unit].flags &= ~(DSF_LOCK|DSF_RUNNING);
	pc586init(unit);
	driver_unlock(&pc_softc[unit].lock);
	splx(oldpri);
}

/*
 * pc586init:
 *
 *	Another routine that interfaces the "if" layer to this driver.  
 *	Simply resets the structures that are used by "upper layers".  
 *	As well as calling pc586hwrst that does reset the pc586 board.
 *
 * input	: board number
 * output	: structures (if structs) and board are reset
 *
 */	
int
pc586init(int unit)
{
	struct	ifnet	*ifp;
	int		stat;
	spl_t	oldpri;

	ifp = &(pc_softc[unit].ds_if);
	oldpri = spllan();
	if ((stat = pc586hwrst(unit)) == TRUE) {
#ifdef WATCHDOG
		untimeout(pc586timeout, &pc_softc[unit]);
		timeout(pc586timeout, &pc_softc[unit], pc586timo*hz);
		pc_softc[unit].timer = pc586timo;
#endif
		pc_softc[unit].ds_if.if_flags |= NET_STATUS_RUNNING;
		pc_softc[unit].flags |= DSF_RUNNING;
		pc_softc[unit].tbusy = 0;
		pc586start(unit);
	} else
		printf("pc%d init(): trouble resetting board.\n", unit);
	splx(oldpri);
	return(stat);
}

io_return_t
pc586devinfo(
	    dev_t dev,
	    dev_flavor_t flavor,
	    char *info)
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

/*ARGSUSED*/
io_return_t
pc586open(
	  dev_t	dev,
	  dev_mode_t	flag,
	  io_req_t uio)
{
	register int	unit;
	pc_softc_t	*sp;
	spl_t		oldpri;
	driver_lock_state();

	unit = minor(dev);	/* XXX */
	if (unit < 0 || unit >= NLAN || !pc_softc[unit].seated)
	    return (ENXIO);
	sp = &pc_softc[unit];

	oldpri = spllan();
	(void)driver_lock(&pc_softc[unit].lock,
			  DRIVER_OP_WAIT, FALSE);
#if XKMACHKERNEL
	if (sp->xk_up && sp->xk_ltype != XK_LOWEST_TYPE) {
	    /*
	     * x-kernel traffic has to respect its boundaries!
	     */
	    sp->xk_ltype = XK_LOWEST_TYPE;
	    sp->xk_htype = XK_HIGHEST_TYPE;
	}
#endif /* XKMACHKERNEL */
	sp->ds_if.if_flags |= NET_STATUS_UP;
	pc586init(unit);
	pc_softc[unit].dev_open++;
	driver_unlock(&pc_softc[unit].lock);
	splx(oldpri);
	return (0);
}

void
pc586close(
	dev_t dev)
{
	register int	unit;
	spl_t		oldpri;
	driver_lock_state();

	unit = minor(dev);

	oldpri = spllan();
	(void)driver_lock(&pc_softc[unit].lock,
			  DRIVER_OP_WAIT, FALSE);
	assert((pc_softc[unit].ds_if.if_flags & NET_STATUS_UP) &&
	       pc_softc[unit].dev_open > 0);
	if (--pc_softc[unit].dev_open == 0)
	    pc_softc[unit].ds_if.if_flags &= ~NET_STATUS_UP;
	driver_unlock(&pc_softc[unit].lock);
	splx(oldpri);
}

/*
 * pc586start:
 *
 *	This is yet another interface routine that simply tries to output a
 *	in an mbuf after a reset.
 *
 * input	: board number
 * output	: stuff sent to board if any there
 *
 */
void
pc586start(int unit)
{
	io_req_t		m;
	struct	ifnet		*ifp;
	register pc_softc_t	*is = &pc_softc[unit];
	driver_lock_state();

	if (!driver_lock(&is->lock, DRIVER_OP_START, FALSE))
		return;

#ifdef DEBUG
	if (pc586debug)
		printf("pc%d: start() busy %d\n", unit, is->tbusy);
#endif

	if (is->tbusy) {
		pc586_cntrs[unit].xmt.busy++;
		driver_unlock(&is->lock);
		return;
	}

	ifp = &(pc_softc[unit].ds_if);
	IF_DEQUEUE(&ifp->if_snd, m);
	if (m) {
		is->tbusy++;
		pc586_cntrs[unit].xmt.xmt++;
		pc586xmt(unit, m);
	}
	driver_unlock(&is->lock);
}

/*
 * pc586read:
 *
 *	This routine does the actual copy of data (including ethernet header
 *	structure) from the pc586 to an mbuf chain that will be passed up
 *	to the "if" (network interface) layer.  NOTE:  we currently
 *	don't handle trailer protocols, so if that is needed, it will
 *	(at least in part) be added here.  For simplicities sake, this
 *	routine copies the receive buffers from the board into a local (stack)
 *	buffer until the frame has been copied from the board.  Once in
 *	the local buffer, the contents are copied to an mbuf chain that
 *	is then enqueued onto the appropriate "if" queue.
 *
 * input	: board number, and an frame descriptor pointer
 * output	: the packet is put into an mbuf chain, and passed up
 * assumes	: if any errors occur, packet is "dropped on the floor"
 *
 */
int
pc586read(
    int	unit,
    volatile fd_t *fd_p)
{
	register pc_softc_t	*is = &pc_softc[unit];
	register struct ifnet	*ifp = &is->ds_if;
	struct ether_header	*ehp;
	char			*dp;
	unsigned short		bytes_in_msg, bytes_in_mbuf, bytes;
	unsigned short		mlen, len, clen, eth_type;
	unsigned char		*mb_p;
	volatile rbd_t		*rbd_p, *rrdb_p;
	unsigned char		*buffer_p;
	spl_t                   opri;
	boolean_t		is_xk;
	char			*base, *header, *body;
	header_t		header_support;
	char *start_body;

#if KGDB_LAN
	if (pc586debug_read(unit, fd_p))
		return 1;
	if ((ifp->if_flags & (NET_STATUS_UP |NET_STATUS_RUNNING))
 	    != (NET_STATUS_UP | NET_STATUS_RUNNING))
		return 1;	/* leave interrupts on, silently drop packet */
#else
	if ((ifp->if_flags & (NET_STATUS_UP | NET_STATUS_RUNNING))
	    != (NET_STATUS_UP | NET_STATUS_RUNNING)) {
		printf("pc%d read(): board is not running.\n", ifp->if_unit);
		pc586intoff(ifp->if_unit);
	}
#endif
	pc586_cntrs[unit].rcv.rcv++;
	if ((fd_p->destination[0] & 1) && /* multicast/broadcast */
	    (fd_p->destination[0] != 0xFF ||
	     fd_p->destination[1] != 0xFF ||
	     fd_p->destination[2] != 0xFF ||
	     fd_p->destination[3] != 0xFF ||
	     fd_p->destination[4] != 0xFF ||
	     fd_p->destination[5] != 0xFF) &&
	    net_multicast_match(&is->mcast_queue,
				(unsigned char *)fd_p->destination,
				ETHER_ADD_SIZE) == (net_multicast_t *)0)
	    return 1;

	/*
	 * Compute packet length.
	 */
	rrdb_p = rbd_p = (volatile rbd_t *)ram_to_ptr(fd_p->rbd_offset, unit);
	if (rbd_p == 0) {
	    printf("pc%d read(): Invalid buffer\n", unit);
	    if (pc586hwrst(unit) != TRUE) {
		printf("pc%d read(): hwrst trouble.\n", unit);
	    }
	    return 0;
	}

	mlen = 0;
	for (;;) {
	    pcacheline(HP700_SID_KERNEL, (vm_offset_t)rbd_p);
	    mlen += rbd_p->status & RBD_SW_COUNT;
	    if (rbd_p->status & RBD_SW_EOF)
		break;
	    rbd_p = (volatile rbd_t *)ram_to_ptr(rbd_p->next_rbd_offset, unit);
	}

	/*
	 * Get ether header.
	 */
	ehp = (struct ether_header *)&header_support;
	bcopy16(fd_p->source, ehp->ether_shost, ETHER_ADD_SIZE);
	bcopy16(fd_p->destination, ehp->ether_dhost, ETHER_ADD_SIZE);
	ehp->ether_type = fd_p->length;
	len = sizeof (struct ether_header);

	/*
	 * Get first ether buffers.
	 */
	rbd_p = rrdb_p;
	do {
	    buffer_p = (unsigned char *)
		(rbd_p->buffer_base << 16) + rbd_p->buffer_addr;
	    bytes_in_msg = rbd_p->status & RBD_SW_COUNT;
	    pdcache(HP700_SID_KERNEL, (vm_offset_t)buffer_p, bytes_in_msg);
	    if (sizeof (header_support) - len >= bytes_in_msg)
		clen = bytes_in_msg;
	    else if (sizeof (header_support) > len)
		clen = sizeof (header_support) - len;
	    else
		break;
	    bcopy16((unsigned short *)buffer_p,
		    (unsigned short *)(((char *)&header_support) + len), clen);
	    len += clen;
	    buffer_p += clen;
	    if (clen < bytes_in_msg)
		break;
	    if (rbd_p->status & RBD_SW_EOF) {
		printf("pc%d read(): Invalid packet\n", unit);
		if (pc586hwrst(unit) != TRUE) {
		    printf("pc%d read(): hwrst trouble.\n", unit);
		}
		return 0;
	    }
	    rbd_p = (volatile rbd_t *)ram_to_ptr(rbd_p->next_rbd_offset, unit);
	} while ((int) rbd_p);

	/*
	 * Get right buffer to store the packet
	 */
	eth_type = ntohs(ehp->ether_type);
	is_xk = IS_XKERNEL(eth_type, is);
	if (eth_resources(is_xk, &header_support, eth_type,
			  mlen, &base, &header, &body)) {
	    /*
	     * Drop the received packet.
	     */
	    is->ds_if.if_rcvdrops++;
#if XKSTATS
	    if (is_xk)
	    	pc586_xkior_lost++;
#endif /* XKSTATS */

	    /*
	     * not only do we want to return, we need to drop the packet on
	     * the floor to clear the interrupt.
	     */
	    return 1;
	}

	start_body = body;

	/*
	 * Copy packet header.
	 */
	*(struct ether_header *)header = *ehp;

	/*
	 * Copy packet body.
	 */
	bcopy(((char *)&header_support) + sizeof (struct ether_header),
	      body, sizeof (header_t) - sizeof (struct ether_header));
	body += sizeof (header_t) - sizeof (struct ether_header);
	bcopy16((unsigned short *)buffer_p,
		(unsigned short *)body, bytes_in_msg - clen);
	body += bytes_in_msg - clen;

	while (!(rbd_p->status & RBD_SW_EOF)) {
	    rbd_p = (volatile rbd_t *)ram_to_ptr(rbd_p->next_rbd_offset, unit);
	    buffer_p = (unsigned char *)
		    (rbd_p->buffer_base << 16) + rbd_p->buffer_addr;
	    bytes_in_msg = rbd_p->status & RBD_SW_COUNT;
	    pdcache(HP700_SID_KERNEL, (vm_offset_t)buffer_p, bytes_in_msg);
	    bcopy16((unsigned short *)buffer_p,
		    (unsigned short *)body,
		    (bytes_in_msg + 1) & ~1);	/* but we know it's even */
	    body += bytes_in_msg;
	}

	/*
	 * Hand the packet to the above layers.
	 * Clone it if needed.
	 */
	opri = splimp();
	eth_dispatch(is_xk, &is->ds_if, eth_type, mlen, unit, base, (io_req_t)0);
	splx(opri);
#if XKMACHKERNEL
		/*
		 * Few packet types require the PDU to be cloned 
		 * (e.g., ARP) for x-kernel's consumption.
		 */
		if (eth_tobecloned(is_xk, eth_type))  {
		    struct ether_header *new_header;
		    char *new_body;

		    if (eth_resources(TRUE, &header_support, eth_type, mlen,
				      &base, (char **) &new_header, &new_body) 
			== KERN_SUCCESS) {
			*new_header = *(struct ether_header *)header;
			memcpy(new_body, (const char *)start_body, mlen);
			opri = splimp();
			eth_dispatch(TRUE, &is->ds_if, eth_type, mlen,
				     unit, base, (io_req_t)0);
			splx(opri);
		    }
		}
#endif /* XKMACHKERNEL */
	return 1;
}

int
pc586output(
	dev_t	dev,
	io_req_t ior)
{
	register int	unit;

	unit = minor(dev);	/* XXX */
	if (unit < 0 || unit >= NLAN || !pc_softc[unit].seated)
	    return (ENXIO);

	return (net_write(&pc_softc[unit].ds_if, pc586start, ior));
}

int
pc586setinput(
	dev_t		dev,
	ipc_port_t	receive_port,
	int		priority,
	filter_t	filter[],
	unsigned int	filter_count,
	device_t        device)
{
	register int unit = minor(dev);
	if (unit < 0 || unit >= NLAN || !pc_softc[unit].seated)
	    return (ENXIO);

	return (net_set_filter(&pc_softc[unit].ds_if,
			receive_port, priority,
			filter, filter_count, device));
}

int
pc586getstat(
	dev_t		dev,
	int		flavor,
	dev_status_t	status,		/* pointer to OUT array */
	unsigned int	*count)		/* out */
{
	register int	unit = minor(dev);
	register pc_softc_t	*sp;

	if (unit < 0 || unit >= NLAN || !pc_softc[unit].seated)
	    return (ENXIO);

	sp = &pc_softc[unit];
	return (net_getstat(&sp->ds_if, flavor, status, count));
}

int
pc586setstat(
	dev_t		dev,
	int		flavor,
	dev_status_t	status,
	unsigned int	count)
{
	register int	unit = minor(dev);
	register pc_softc_t	*sp;
	spl_t oldpri;
	driver_lock_state();

	if (unit < 0 || unit >= NLAN || !pc_softc[unit].seated)
	    return (ENXIO);

	sp = &pc_softc[unit];

	switch (flavor) {
	    case NET_STATUS:
	    {
		/*
		 * All we can change are flags, and not many of those.
		 */
		register struct net_status *ns = (struct net_status *)status;
		int	mode = 0;

		if (count < NET_STATUS_COUNT)
		    return (D_INVALID_OPERATION);

		if (ns->flags & NET_STATUS_ALLMULTI)
		    mode |= MOD_ENAL;
		if (ns->flags & NET_STATUS_PROMISC)
		    mode |= MOD_PROM;

		/*
		 * Force a complete reset if the receive mode changes
		 * so that these take effect immediately.
		 */
		oldpri = spllan();
		(void)driver_lock(&sp->lock, DRIVER_OP_WAIT, FALSE);
		if (sp->mode != mode) {
		    sp->mode = mode;
		    if (sp->flags & DSF_RUNNING) {
			sp->flags &= ~(DSF_LOCK|DSF_RUNNING);
			pc586init(unit);
		    }
		}
		driver_unlock(&pc_softc[unit].lock);
		splx(oldpri);
		break;
	    }

	    case NET_ADDMULTI:
	    {
		net_multicast_t	*new;
		unsigned temp;

		if (count != ETHER_ADD_SIZE * 2)
		    return (KERN_INVALID_VALUE);
		if ((((unsigned char *)status)[0] & 1) == 0 ||
		    ((unsigned char *)status)[ETHER_ADD_SIZE] !=
		    ((unsigned char *)status)[0])
		    return (KERN_INVALID_VALUE);

		for (temp = 0; temp < ETHER_ADD_SIZE; temp++)
		    if (((unsigned char *)status)[temp] !=
			((unsigned char *)status)[ETHER_ADD_SIZE + temp])
			break;
		if (temp == ETHER_ADD_SIZE)
		    temp = (NET_MULTICAST_LENGTH(ETHER_ADD_SIZE) +
			    MULTICAST_SIZE);
		else
		    temp = (NET_MULTICAST_LENGTH(ETHER_ADD_SIZE) +
			    MULTICAST_SIZE +
			    ETHER_ADD_SIZE * MULTICAST_SIZE * 8);
		new = (net_multicast_t *)kalloc(temp);
		new->size = temp;

		net_multicast_create(new, (unsigned char *)status,
				     &((unsigned char *)
				       status)[ETHER_ADD_SIZE],
				     ETHER_ADD_SIZE);
		pc586_multicast_create(new);

		oldpri = spllan();
		(void)driver_lock(&sp->lock, DRIVER_OP_WAIT, FALSE);
		if (pc586_multicast_insert(sp, new)) {
		    net_multicast_insert(&sp->mcast_queue, new);
		    sp->flags &= ~(DSF_LOCK|DSF_RUNNING);
		    pc586init(unit);
		} else
		    net_multicast_insert(&sp->mcast_queue, new);
		driver_unlock(&pc_softc[unit].lock);
		splx(oldpri);
		break;
	    }

	    case NET_DELMULTI:
	    {
		net_multicast_t	*cur;
		unsigned temp;

		if (count != ETHER_ADD_SIZE * 2)
		    return (KERN_INVALID_VALUE);

		oldpri = spllan();
		(void)driver_lock(&sp->lock, DRIVER_OP_WAIT, FALSE);
		cur = net_multicast_remove(&sp->mcast_queue,
					   (unsigned char *)status,
					   &((unsigned char *)
					     status)[ETHER_ADD_SIZE],
					   ETHER_ADD_SIZE);
		if (cur == (net_multicast_t *)0) {
		    driver_unlock(&pc_softc[unit].lock);
		    splx(oldpri);
		    return (KERN_INVALID_VALUE);
		}
		if (pc586_multicast_remove(sp, cur)) {
		    sp->flags &= ~(DSF_LOCK|DSF_RUNNING);
		    pc586init(unit);
		}
		driver_unlock(&pc_softc[unit].lock);
		splx(oldpri);

		kfree((vm_offset_t)cur, cur->size);
		break;
	    }

	    default:
		return (D_INVALID_OPERATION);
	}
	return (D_SUCCESS);

}

boolean_t
pc586_multicast_insert(
	pc_softc_t *sp,
	net_multicast_t *new)
{
	unsigned int i;
	unsigned int j;
	unsigned int k;
	unsigned char *crc;
	unsigned char *newcrc;
	unsigned char *addr;
	net_multicast_t *cur;

	/*
	 * Find another multicast address which has the same hash bits on
	 */
	newcrc = NET_MULTICAST_MISC(new);
	if (queue_empty(&sp->mcast_queue)) {
	    for (i = 0; i < MULTICAST_SIZE; i++)
		if (newcrc[i] != 0)
		    break;
	} else {
	    queue_iterate(&sp->mcast_queue, cur, net_multicast_t *, chain) {
		crc = NET_MULTICAST_MISC(cur);
		for (i = 0; i < MULTICAST_SIZE; i++)
		    if ((crc[i] & newcrc[i]) != newcrc[i])
			break;
		if (i == MULTICAST_SIZE)
		    return (FALSE);
	    }
	}
	
	/*
	 * Add this element to the address list
	 */
	if (new->hash != -1) {
	    /*
	     * This is an uniq multicast address
	     */
	    sp->mcast_hash[i] |= newcrc[i];
	    addr = NET_MULTICAST_FROM(new);
	    for (k = 0; k < ETHER_ADD_SIZE; k++)
		sp->sc_mcast.mc_addr[sp->sc_mcast.mc_count++] = addr[k];
	    return (TRUE);
	}

	/*
	 * Now, deal with a range of multicast addresses
	 */
	while (i < MULTICAST_SIZE) {
	    if ((sp->mcast_hash[i] & newcrc[i]) != newcrc[i])
		for (j = 0; j < 8; j++) {
		    if (((1 << j) & sp->mcast_hash[i]) ||
			((1 << j) & crc[i]) == 0)
			continue;

		    sp->mcast_hash[i] |= (1 << j);
		    addr = newcrc + MULTICAST_SIZE +
			ETHER_ADD_SIZE * ((i * 8) + j);
		    for (k = 0; k < ETHER_ADD_SIZE; k++)
			sp->sc_mcast.mc_addr[sp->sc_mcast.mc_count++] = addr[k];
		    if ((sp->mcast_hash[i] & newcrc[i]) == newcrc[i])
			break;
		}
	    i++;
	}
	return (TRUE);
}

boolean_t
pc586_multicast_remove(
	pc_softc_t *sp,
	net_multicast_t *old)
{
	unsigned int i;
	unsigned int j;
	unsigned int k;
	net_multicast_t *cur;
	unsigned char *crc;
	unsigned char *oldcrc;
	unsigned char *addr;

	/*
	 * Find another multicast address which has the same hash bits on
	 */
	oldcrc = NET_MULTICAST_MISC(old);
	queue_iterate(&sp->mcast_queue, cur, net_multicast_t *, chain) {
	    crc = NET_MULTICAST_MISC(cur);
	    for (i = 0; i < MULTICAST_SIZE; i++)
		if ((crc[i] & oldcrc[i]) != oldcrc[i])
		    break;
	    if (i == MULTICAST_SIZE)
		return (FALSE);
	}

	/*
	 * Recompute the multicast hash bit array
	 */
	sp->sc_mcast.mc_count = 0;
	for (i = 0; i < MULTICAST_SIZE; i++)
	    sp->mcast_hash[i] = 0;
	queue_iterate(&sp->mcast_queue, cur, net_multicast_t *, chain) {
	    crc = NET_MULTICAST_MISC(cur);
	    for (i = 0; i < MULTICAST_SIZE; i++)
		if ((crc[i] & sp->mcast_hash[i]) != crc[i])
		    break;
	    if (i == MULTICAST_SIZE)
		continue;

	    /*
	     * Add this element to the address list
	     */
	    if (cur->hash != -1) {
		/*
		 * This is an uniq multicast address
		 */
		sp->mcast_hash[i] |= crc[i];
		addr = NET_MULTICAST_FROM(cur);
		for (k = 0; k < ETHER_ADD_SIZE; k++)
		    sp->sc_mcast.mc_addr[sp->sc_mcast.mc_count++] = addr[k];
		continue;
	    }

	    /*
	     * Now, deal with a range of multicast addresses
	     */
	    while (i < MULTICAST_SIZE) {
		if ((crc[i] & sp->mcast_hash[i]) != crc[i])
		    for (j = 0; j < 8; j++) {
			if (((1 << j) & sp->mcast_hash[i]) ||
			    ((1 << j) & crc[i]) == 0)
			    continue;

			sp->mcast_hash[i] |= 1 << j;
			addr = crc + MULTICAST_SIZE +
			    ETHER_ADD_SIZE * ((i * 8) + j);
			for (k = 0; k < ETHER_ADD_SIZE; k++)
			    sp->sc_mcast.mc_addr
				[sp->sc_mcast.mc_count++] = addr[k];
			if ((crc[i] & sp->mcast_hash[i]) == crc[i])
			    break;
		    }
		i++;
	    }
	}
	return (TRUE);
}

/*
 * Create a multicast check array given a range of addresses.
 */
void
pc586_multicast_create(
	net_multicast_t *cur)
{
	unsigned long	crc32;
	unsigned char	*p;
	int		i;
	unsigned	j;
	unsigned char	*crc;
	unsigned char	*to;
	unsigned char	from[ETHER_ADD_SIZE];
	unsigned char	c;

	for (i = 0; i < ETHER_ADD_SIZE; i++)
	    from[i] = NET_MULTICAST_FROM(cur)[i];
	to = NET_MULTICAST_TO(cur);
	crc = p = NET_MULTICAST_MISC(cur);

	for (;;) {
	    /*
	     * Compute the CRC32 of the address.
	     */
	    crc32 = 0xFFFFFFFF;
	    for (i = 0; i < ETHER_ADD_SIZE; i++) {
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
	     * Update CRC array with the bits 2-7.
	     */
	    crc32 = (crc32 >> 24) & (MULTICAST_SIZE * 8 - 1);
	    crc[crc32 / MULTICAST_SIZE] |=
		    (1 << (crc32 % MULTICAST_SIZE));
	    while (*p == 0xFF)
		if (p < &crc[MULTICAST_SIZE-1])
		    p++;
		else
		    break;

	    /*
	     * Goto next address.
	     */
	    if (cur->hash >= 0)
		break;

	    for (i = 0; i < ETHER_ADD_SIZE; i++)
		crc[MULTICAST_SIZE + crc32 * ETHER_ADD_SIZE] = from[i];

	    for (i = ETHER_ADD_SIZE-1; i >= 0 ; i--)
		if (++from[i])
		     break;
	    for (i = 0; i < ETHER_ADD_SIZE; i++)
		if (from[i] != to[i])
		    break;
	    if (i == ETHER_ADD_SIZE)
		break;
	}
}

/*
 * pc586hwrst:
 *
 *	This routine resets the pc586 board that corresponds to the 
 *	board number passed in.
 *
 * input	: board number to do a hardware reset
 * output	: board is reset
 *
 */
int
pc586hwrst(int unit)
{
	unsigned long data;
	volatile int *sram;
	register struct core_lan *lanp;

	lanp = pc_softc[unit].hwaddr;
	/* turn off interrupt */
	pc586intoff(unit);

	sram = (volatile int *) (&pc_softc[unit].sram[OFFSET_SCP]);
	data = kvtophys((vm_offset_t)sram) | 1;
#ifdef DEBUG
	if (pc586debug)
		printf("pc586hwrst: addr=0x%x, sram=%x(%lx)\n",
		       lanp, sram, data);
#endif

	/* self test */
	sram[1] = -1;
	fcacheline(HP700_SID_KERNEL, (vm_offset_t)sram);

	pc586putport(lanp, data);
	for (data = 1000000; data > 0; data--) {
		pcacheline(HP700_SID_KERNEL, (vm_offset_t)sram);
		if (sram[1] != -1)
			break;
	}
#ifdef DEBUG
	if (pc586debug)
		printf("pc%d: self test = 0x%x:0x%x\n",
		       unit, sram[0], sram[1]);
#endif
	if (sram[1] != 0)
		return(FALSE);
	if (pc586bldcu(unit) == FALSE)
		return(FALSE);
	if (pc586diag(unit) == FALSE)
		return(FALSE);

	if (pc586config(unit) == FALSE)
		return(FALSE);
	/* 
	 * insert code for loopback test here
	 */
	pc586rustrt(unit);
	pc586inton(unit);
	return(TRUE);
}

#ifdef WATCHDOG
void
pc586timeout(void *sp)
{
	spl_t	oldpri;
	driver_lock_state();

	oldpri = splimp();
	if (driver_lock(&((pc_softc_t *)sp)->lock,
			      DRIVER_OP_TIMEO, FALSE)) {
		pc586watch((pc_softc_t *)sp - pc_softc);
		driver_unlock(&((pc_softc_t *)sp)->lock);
	}
	splx(oldpri);
}

/*
 * pc586watch():
 *
 *	This routine is the watchdog timer routine for the pc586 chip.  If
 *	chip wedges, this routine will fire and cause a board reset and
 *	begin again.
 *
 * input	: which board is timing out
 * output	: potential board reset if wedged
 *
 */
int watch_dead = 0;
void
pc586watch(int unit)
{
	if (pc_softc[unit].timer == -1) {
		timeout(pc586timeout, &pc_softc[unit], pc586timo*hz);
		return;
	}
	if (--pc_softc[unit].timer != -1) {
		timeout(pc586timeout, &pc_softc[unit], 1*hz);
		return;
	}

#ifdef	notdef
	printf("pc%d watch(): %dsec timeout no %d\n",
	       unit, pc586timo, ++watch_dead);
#endif
	pc586_cntrs[unit].watch++;
	if (pc586hwrst(unit) != TRUE) {
		printf("pc%d watch(): hwrst trouble.\n", unit);
		pc_softc[unit].timer = 0;
	} else {
		timeout(pc586timeout, &pc_softc[unit], 1*hz);
		pc_softc[unit].timer = pc586timo;
	}
	printf("pc%d: watchdog: %d second timeout #%d\n",
	       unit, pc586timo, ++watch_dead);
}
#endif

/*
 * pc586intr:
 *
 *	This function is the interrupt handler for the pc586 ethernet
 *	board.  This routine will be called whenever either a packet
 *	is received, or a packet has successfully been transfered and
 *	the unit is ready to transmit another packet.
 *
 * input	: board number that interrupted
 * output	: either a packet is received, or a packet is transfered
 *
 */
void
pc586intr(int unit)
{
	register pc_softc_t	*is = &pc_softc[unit];
	volatile scb_t		*scb_p;
	volatile ac_t		*cb_p;
	unsigned short		int_type;
#if KGDB_LAN
	extern boolean_t kgdb_stop;
#endif
	driver_lock_state();

	if (!driver_lock(&is->lock, DRIVER_OP_INTR, FALSE))
		return;

	if (is->seated == FALSE) { 
		driver_unlock(&is->lock);
		printf("pc%d intr(): board not seated\n", unit);
		return;
	}

#ifdef	PC596XMT_BUG
	if (((long)pc586_toutxmt[unit].tv_sec) < 0) {
		timerclear(&pc586_toutxmt[unit]);
		if ((is->ds_if.if_flags & NET_STATUS_UP) == 0) {
			driver_unlock(&is->lock);
			return;
		}

		pc586_cntrs[unit].xmtwatch++;
                is->tbusy = 0;
                pc586start(unit);
	}
#endif	/* PC596XMT_BUG */

	scb_p = (volatile scb_t *)(is->sram + OFFSET_SCB);
	purgescbstat(scb_p);
	while ((int_type = (scb_p->scb_status & SCB_SW_INT)) != 0) {
		pc586ack(unit);
		if (int_type & SCB_SW_FR) {
			pc586rcv(unit);
#ifdef USELEDS
			if (inledcontrol == 0)
				ledcontrol(0, 0, LED_LANRCV);
#endif
		}
		if (int_type & SCB_SW_RNR) {
			pc586_cntrs[unit].rcv.ovw++;
#ifdef	notdef
			printf("pc%d intr(): receiver overrun! begin_fd = %x\n",
				unit, is->begin_fd);
#endif
			pc586rustrt(unit);
		}
		if (int_type & SCB_SW_CNA) {
			/*
			 * At present, we don't care about CNA's.  We
			 * believe they are a side effect of XMT.
			 */
		}
		if (int_type & SCB_SW_CX) {
#ifdef	PC596XMT_BUG
			timerclear(&pc586_toutxmt[unit]);
#endif	/* PC596XMT_BUG */
			/*
			 * At present, we only request Interrupt for
			 * XMT.
			 */
			cb_p = (volatile ac_t *)(is->sram + OFFSET_CU);
			pdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof(*cb_p));
#ifdef	DEBUG
			if (pc586debug)
				printf("pc%d: intr type: %x status %x\n",
				       unit, int_type, cb_p->ac_status);
#endif
			if ((!(cb_p->ac_status & AC_SW_OK)) ||
			    (cb_p->ac_status & (0xfff^TC_SQE))) {
				if (cb_p->ac_status & TC_DEFER) {
					if (xmt_watch) printf("DF");
					pc586_cntrs[unit].xmt.defer++;
				} else if (cb_p->ac_status & (TC_COLLISION|0xf)) {
					if (xmt_watch) printf("%x",cb_p->ac_status & 0xf);
				} else {
					if (xmt_watch) 
						printf("pc%d XMT: %x %x\n",
						       unit, cb_p->ac_status,
						       cb_p->ac_command);
					/*
					 * If transmitter claims to be busy,
					 * believe it.  This is probably a
					 * left-over interrupt as a result of
					 * the race condition in pc586start.
					 * DON'T restart the transmitter!
					 */
					if (cb_p->ac_status == AC_SW_B) {
						pc586_cntrs[unit].xmt.xmtsi++;
						goto xdone;
					}
					log(LOG_WARNING,
					    "pc%d: XMT: bad status %x\n",
					    unit, cb_p->ac_status);
				}
			} else {
#ifdef USELEDS
				if (inledcontrol == 0)
					ledcontrol(0, 0, LED_LANXMT);
#endif
			}
			pc586_cntrs[unit].xmt.xmti++;
			is->tbusy = 0;
			pc586start(unit);
		}
xdone:
#ifdef WATCHDOG
		is->timer = pc586timo;
		watch_dead = 0;
#endif
		purgescbstat(scb_p);
	}
	driver_unlock(&is->lock);

#if KGDB_LAN
	if (kgdb_stop)
		kgdb_break();
#endif
}

/*
 * pc586rcv:
 *
 *	This routine is called by the interrupt handler to initiate a
 *	packet transfer from the board to the "if" layer above this
 *	driver.  This routine checks if a buffer has been successfully
 *	received by the pc586.  If so, the routine pc586read is called
 *	to do the actual transfer of the board data (including the
 *	ethernet header) into a packet (consisting of an mbuf chain).
 *
 * input	: number of the board to check
 * output	: if a packet is available, it is "sent up"
 *
 */
void
pc586rcv(int unit)
{
	volatile fd_t		*fd_p;
	register pc_softc_t	*is = &pc_softc[unit];

	for (fd_p = is->begin_fd; fd_p != (fd_t *)NULL; fd_p = is->begin_fd) {
		pcacheline(HP700_SID_KERNEL, (vm_offset_t)fd_p);
		if (fd_p->status == 0xffff) {
			if (pc586hwrst(unit) != TRUE)
				printf("pc%d rcv(): hwrst ffff trouble.\n",
					unit);
			return;
		} else if (fd_p->status & AC_SW_C) {
			is->begin_fd =
				(fd_t *)ram_to_ptr(fd_p->link_offset, unit);

			if (fd_p->status == (RFD_DONE|RFD_RSC)) {
				printf("pc%d RCV: lost packet %x\n",
				       unit, fd_p->status);
				pc586_cntrs[unit].rcv.partial++;
			} else if (!(fd_p->status & RFD_OK)) {
				printf("pc%d RCV: %x\n", unit, fd_p->status);
			} else if (fd_p->status & 0xfff) {
				printf("pc%d RCV: %x\n", unit, fd_p->status);
			} else if (!pc586read(unit, fd_p))
				return;
			if (!pc586requeue(unit, fd_p)) {	/* abort on chain error */
				if (pc586hwrst(unit) != TRUE)
					printf("pc%d rcv(): hwrst trouble.\n", unit);
				return;
			}
		} else
			break;
	}
}

/*
 * pc586requeue:
 *
 *	This routine puts rbd's used in the last receive back onto the
 *	free list for the next receive.
 *
 */
int 
pc586requeue(
	     int	unit,
	     volatile fd_t	*fd_p)
{
	volatile rbd_t	*l_rbdp;
	rbd_t	*f_rbdp;

#ifndef	REQUEUE_DBG
	if (bad_rbd_chain(fd_p->rbd_offset, unit))
		return 0;
#endif
	f_rbdp = (rbd_t *)ram_to_ptr(fd_p->rbd_offset, unit);
	if (f_rbdp != NULL) {
		l_rbdp = f_rbdp;
		pcacheline(HP700_SID_KERNEL, (vm_offset_t)l_rbdp);
		while (!(l_rbdp->status & RBD_SW_EOF) &&
		       l_rbdp->next_rbd_offset != 0xffff)
		{
			l_rbdp->status = 0;
			fcacheline(HP700_SID_KERNEL, (vm_offset_t)l_rbdp);
		   	l_rbdp = (rbd_t *)ram_to_ptr(l_rbdp->next_rbd_offset,
						     unit);
			pcacheline(HP700_SID_KERNEL, (vm_offset_t)l_rbdp);
		}
		l_rbdp->next_rbd_offset = PC586NULL;
		l_rbdp->status = 0;
		l_rbdp->size |= AC_CW_EL;
		fcacheline(HP700_SID_KERNEL, (vm_offset_t)l_rbdp);
		pc_softc[unit].end_rbd->next_rbd_offset = 
			ptr_to_ram((char *)f_rbdp, unit);
		pc_softc[unit].end_rbd->size &= ~AC_CW_EL;
		fcacheline(HP700_SID_KERNEL, (vm_offset_t)pc_softc[unit].end_rbd);
		pc_softc[unit].end_rbd = (rbd_t *)l_rbdp;
	}

	fd_p->status = 0;
	fd_p->command = AC_CW_EL;
	fd_p->link_offset = PC586NULL;
	fd_p->rbd_offset = PC586NULL;
	fcacheline(HP700_SID_KERNEL, (vm_offset_t)fd_p);

	pc_softc[unit].end_fd->link_offset = ptr_to_ram((char *)fd_p, unit);
	pc_softc[unit].end_fd->command = 0;
	fcacheline(HP700_SID_KERNEL, (vm_offset_t)pc_softc[unit].end_fd);
	pc_softc[unit].end_fd = fd_p;

	return 1;
}

/*
 * Send a packet back to higher levels.
 */
void
pc586send_packet_up(
		    io_req_t           	m,
		    struct ether_header	*eh,
		    pc_softc_t		*is,
		    int			unit)
{
	char *base, *header, *body;
	unsigned short len;
	unsigned short type = ntohs(eh->ether_type);
	boolean_t is_xk = IS_XKERNEL(type, is);
	header_t header_support;
	spl_t opri;
	len = m->io_count - sizeof(struct ether_header);

	/*
	 * Set up correct header_t
	 */
	bcopy((char *)eh, (char *)&header_support, sizeof (*eh));
#if     XKMACHKERNEL
        if (m->io_op & IO_IS_XK_MSG) {
	    struct pc586_xk xk;
	    xk.addr = ((char *)&header_support) + sizeof (*eh);
	    xk.max = sizeof (header_support) - sizeof (*eh);
	    xk.len = 0;
	    msgForEachAlternate((Msg)m->io_data, (XCharFun)pc586_xkcopy,
				(XCharFun)pc586_xkcopy_test, &xk);
	} else
#endif  /* XKMACHKERNEL */
	    bcopy((char *)&m->io_data[sizeof(struct ether_header)],
		  ((char *)&header_support) + sizeof (*eh),
		  sizeof (header_support) - sizeof (*eh));

	if (eth_resources(is_xk, &header_support, type, len,
			  &base, &header, &body) != KERN_SUCCESS) {
	    /*
	     * Drop the packet.
	     */
	    is->ds_if.if_rcvdrops++;
	    return;
	}

	/*
	 * Send packet to higher levels
	 */
	bcopy((char *)eh, header, sizeof(struct ether_header));
#if     XKMACHKERNEL
        if (m->io_op & IO_IS_XK_MSG) {
	    struct pc586_xk xk;
	    xk.addr = body;
	    xk.max = len;
	    xk.len = 0;
	    msgForEachAlternate((Msg)m->io_data, (XCharFun)pc586_xkcopy,
				(XCharFun)pc586_xkcopy_test, &xk);
	} else
#endif  /* XKMACHKERNEL */ 
	    bcopy((char *)&m->io_data[sizeof(struct ether_header)], body, len);

	opri = splimp();
	eth_dispatch(is_xk, &is->ds_if, type, len, unit, base, m); 
	splx(opri);

#if XKMACHKERNEL
	/*
	 * Few packet types require the PDU to be cloned 
	 * (e.g., ARP) for x-kernel's consumption.
	 */
	if (eth_tobecloned(is_xk, type))  {
	    struct ether_header *new_header;
	    char *new_body;

	    if (eth_resources(TRUE, &header_support, type, len,
			      &base, (char **) &new_header, &new_body) 
		== KERN_SUCCESS) {
		*new_header = *(struct ether_header *)header;
		memcpy(new_body, (const char *)body, len);
		opri = splimp();
		eth_dispatch(TRUE, &is->ds_if, type, len,
			     unit, base, (io_req_t)0);
		splx(opri);
	    }
	}
#endif /* XKMACHKERNEL */
}

/*
 * pc586xmt:
 *
 *	This routine fills in the appropriate registers and memory
 *	locations on the PC586 board and starts the board off on
 *	the transmit.
 *
 * input	: board number of interest, and a pointer to the mbuf
 * output	: board memory and registers are set for xfer and attention
 *
 */
#ifdef	XMT_DEBUG
int xmt_debug = 0;
#endif
void
pc586xmt(
	 int	unit,
	 io_req_t m)
{
	pc_softc_t			*is = &pc_softc[unit];
	register unsigned char		*xmtdata_p = (unsigned char *)(is->sram + OFFSET_TBUF);
	register unsigned short		*xmtshort_p;
	register struct ether_header	*eh_p = (struct ether_header *)m->io_data;
	volatile scb_t			*scb_p = (volatile scb_t *)(is->sram + OFFSET_SCB);
	volatile ac_t			*cb_p = (volatile ac_t *)(is->sram + OFFSET_CU);
	tbd_t				*tbd_p = (tbd_t *)(is->sram + OFFSET_TBD);
	unsigned short			len, clen = 0;

	cb_p->ac_status = 0;
	cb_p->ac_command = (AC_CW_EL|AC_TRANSMIT|AC_CW_I);
	cb_p->ac_link_offset = PC586NULL;
	cb_p->cmd.transmit.tbd_offset = OFFSET_TBD;

#if     XKMACHKERNEL
	if (m->io_op & IO_IS_XK_MSG)
	    eh_p = (struct ether_header *)msgGetAttr((Msg)m->io_data, 0);
	else
#endif	/* XKMACHKERNREL */
	    eh_p = (struct ether_header *)m->io_data;
	bcopy16(eh_p->ether_dhost, cb_p->cmd.transmit.dest_addr, ETHER_ADD_SIZE);
	cb_p->cmd.transmit.length = (unsigned short)(eh_p->ether_type);
	fdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof *cb_p);

	tbd_p->act_count = 0;
	tbd_p->buffer_addr = (unsigned short)((unsigned int)xmtdata_p);
	tbd_p->buffer_base = (int)xmtdata_p >> 16;
#ifdef DEBUG
	if (pc586debug)
		printf("pc%d: create tbuf #1 base=%x addr=%x\n",
		       unit, tbd_p->buffer_base, tbd_p->buffer_addr);
#endif

	clen = m->io_count - sizeof(struct ether_header);
#if     XKMACHKERNEL
	if (m->io_op & IO_IS_XK_MSG) {
	    unsigned char *addr = xmtdata_p;
	    spl_t ss;

	    xIfTrace(xkecp, TR_EVENTS) {
		printf("xkec: Outgoing message: ");
		msgShow((Msg)m->io_data);
	    }
	    msgForEachAlternate((Msg)m->io_data, (XCharFun)pc586_xkcopy16,
				(XCharFun)pc586_xkcopy16_test, &addr);
	    if (addr - xmtdata_p != clen) {
		iodone(m);
#if XKSTATS
		pc586_xkior_error++;
#endif /* XKSTATS */
		return;
	    }
	    xmtdata_p = (unsigned char *)(((vm_offset_t)addr + 1) & ~1);
	} else  
#endif /* XKMACHKERNEL */
	{ int Rlen, Llen;
	    Llen = clen & 1;
	    Rlen = ((int)(m->io_data + sizeof(struct ether_header))) & 1;

	    bcopy16(m->io_data + sizeof(struct ether_header) - Rlen,
		    xmtdata_p,
		    clen + (Rlen + Llen) );
	    xmtdata_p += clen + Llen;
	    tbd_p->buffer_addr += Rlen;
	}
	tbd_p->act_count = clen;

#ifdef	XMT_DEBUG
	if (xmt_debug)
		printf("CLEN = %u\n", clen);
#endif
	if (clen < ETHERMIN) {
		tbd_p->act_count += ETHERMIN - clen;
		for (xmtshort_p = (unsigned short *)xmtdata_p;
		     clen < ETHERMIN;
		     clen += 2) *xmtshort_p++ = 0;
		xmtdata_p = (unsigned char *)xmtshort_p;
	}
	tbd_p->act_count |= TBD_SW_EOF;
	tbd_p->next_tbd_offset = PC586NULL;
	fcacheline(HP700_SID_KERNEL, (vm_offset_t)tbd_p);
	/* XXX take into account padding of odd length buffers */
	len = xmtdata_p - (unsigned char *)(is->sram + OFFSET_TBUF);
	if (len < clen)
		panic("pc586xmt: screwed the pooch");
	fdcache(HP700_SID_KERNEL, (vm_offset_t)(is->sram + OFFSET_TBUF), len);
#ifdef	IF_CNTRS
	clen += sizeof (struct ether_header) + 4 /* crc */;
	pc586_eout[log_2(clen)]++;
	if (clen < 128)  pc586_lout[clen>>3]++;
#endif
#ifdef	XMT_DEBUG
	if (xmt_debug) {
		pc586tbd(unit);
		printf("\n");
	}
#endif

	/*
	 * This just wedged on me (4/17/92).  We cant have this STUPID
	 * board screwing up our fine machine, can we?!!
	 */
	{
		int i = 1000000;

		do
			purgescbcmd(scb_p);
		while (scb_p->scb_command && i-- > 0);

		if (i == 0) {
			printf("pc%d: I have wedged myself in pc586xmt().\n",
			       unit);
			if (pc586hwrst(unit) != TRUE) {
				printf("pc%d: hardware reset failed.\n", unit);
#ifdef WATCHDOG
				pc_softc[unit].timer = 0;
#endif
			}
			iodone(m);
			return;
		}
	}
	scb_p->scb_command = SCB_CU_STRT;
	flushscbcmd(scb_p);
	pc586chatt(unit);
#ifdef	PC596XMT_BUG
	{
		register long tout;
		register struct timeval *tp = &pc586_toutxmt[unit];

		*tp = time;
		for (tout = tp->tv_usec + pc586_waitxmt;
		     tout >= 1000000; tout -= 1000000)
			tp->tv_sec += 1;
		tp->tv_usec = tout;
	}
#endif	/* PC596XMT_BUG */

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
	     (m->io_device->open_count > 1 || is->dev_open > 1)) &&
	    (is->ds_if.if_flags & NET_STATUS_PROMISC) ||
	    (eh_p->ether_dhost[0] == 0xFF &&
	     eh_p->ether_dhost[1] == 0xFF &&
	     eh_p->ether_dhost[2] == 0xFF &&
	     eh_p->ether_dhost[3] == 0xFF &&
	     eh_p->ether_dhost[4] == 0xFF &&
	     eh_p->ether_dhost[5] == 0xFF) ||
	    (eh_p->ether_dhost[0] == is->ds_addr[0] &&
	     eh_p->ether_dhost[1] == is->ds_addr[1] &&
	     eh_p->ether_dhost[2] == is->ds_addr[2] &&
	     eh_p->ether_dhost[3] == is->ds_addr[3] &&
	     eh_p->ether_dhost[4] == is->ds_addr[4] &&
	     eh_p->ether_dhost[5] == is->ds_addr[5]) ||
	    ((eh_p->ether_dhost[0] & 1) &&
	     net_multicast_match(&is->mcast_queue,
				 eh_p->ether_dhost, ETHER_ADD_SIZE)))
	    pc586send_packet_up(m, eh_p, is, unit);

	iodone(m);
}

/*
 * pc586bldcu:
 *
 *	This function builds up the command unit structures.  It inits
 *	the scp, iscp, scb, cb, tbd, and tbuf.
 *
 */
boolean_t
pc586bldcu(int unit)
{
	volatile char	*sram = pc_softc[unit].sram;
	scp_t		*scp_p = (scp_t *)(sram + OFFSET_SCP);
	volatile iscp_t	*iscp_p = (volatile iscp_t *)(sram + OFFSET_ISCP);
	volatile scb_t	*scb_p = (volatile scb_t *)(sram + OFFSET_SCB);
	volatile ac_t	*cb_p = (volatile ac_t *)(sram + OFFSET_CU);
	tbd_t		*tbd_p = (tbd_t *)(sram + OFFSET_TBD);
	int		i;

	scp_p->scp_sysbus = 0x68;
	i = kvtophys((vm_offset_t)iscp_p);
	scp_p->scp_iscp_lo = i;
	scp_p->scp_iscp_hi = i >> 16;
	scp_p->scp_unused1 = 0;
	scp_p->scp_unused2 = 0;

	iscp_p->iscp_busy = 1;
	iscp_p->iscp_scb_offset = OFFSET_SCB;
	i = kvtophys((vm_offset_t)sram);
	iscp_p->iscp_scb_lo = i;
	iscp_p->iscp_scb_hi = i >> 16;

	purgescball(scb_p);
	pc586_cntrs[unit].rcv.crc += scb_p->scb_crcerrs;
	pc586_cntrs[unit].rcv.frame += scb_p->scb_alnerrs;
	pc586_cntrs[unit].rcv.rscerrs += scb_p->scb_rscerrs;
	pc586_cntrs[unit].rcv.ovrnerrs += scb_p->scb_ovrnerrs;
	scb_p->scb_status = 0;
	scb_p->scb_command = 0;
	scb_p->scb_cbl_offset = OFFSET_CU;
	scb_p->scb_rfa_offset = OFFSET_RU;
	scb_p->scb_crcerrs = 0;
	scb_p->scb_alnerrs = 0;
	scb_p->scb_rscerrs = 0;
	scb_p->scb_ovrnerrs = 0;

	cb_p->ac_status = 0;
	cb_p->ac_command = (AC_CW_EL|AC_NOP);
	cb_p->ac_link_offset = OFFSET_CU;

	tbd_p->act_count = 0;
	tbd_p->next_tbd_offset = PC586NULL;
	tbd_p->buffer_addr = 0;
	tbd_p->buffer_base = 0;

	i = kvtophys((vm_offset_t)scp_p) | 0x2;
	fdcache(HP700_SID_KERNEL, (vm_offset_t)pc_softc[unit].sram, LAN_BUFF_SIZE);
	pc586putport(pc_softc[unit].hwaddr, i);
	pc586chatt(unit);
	for (i = 1000000; iscp_p->iscp_busy && i > 0; i--)
		pcacheline(HP700_SID_KERNEL, (vm_offset_t)iscp_p);
#ifdef DEBUG
	if (pc586debug)
		printf("pc%d: iscp status = %x\n", unit, iscp_p->iscp_busy);
#endif
	if (!i) {
		printf("pc%d bldcu(): iscp_busy timeout.\n", unit);
		return(FALSE);
	}
	for (i = STATUS_TRIES; i > 0; i--) {
		if (scb_p->scb_status == (SCB_SW_CX|SCB_SW_CNA)) 
			break;
		purgescbstat(scb_p);
	}
#ifdef DEBUG
	if (pc586debug)
		printf("pc%d: scb status = %x:%x %d\n",
		       unit, scb_p->scb_status, scb_p->scb_command, i);
#endif
	if (!i) {
		printf("pc%d bldcu(): not ready after reset.\n", unit);
		return(FALSE);
	}
	pc586ack(unit);
	return(TRUE);
}

/*
 * pc586bldru:
 *
 *	This function builds the linear linked lists of fd's and
 *	rbd's.  Based on page 4-32 of 1986 Intel microcom handbook.
 *
 */
char *
pc586bldru(int unit)
{
	fd_t	*fd_p = (fd_t *)(pc_softc[unit].sram + OFFSET_RU);
	ru_t	*rbd_p = (ru_t *)(pc_softc[unit].sram + OFFSET_RBD);
	int 	i;

	pc_softc[unit].begin_fd = fd_p;
	for(i = 0; i < N_FD; i++, fd_p++) {
		fd_p->status = 0;
		fd_p->command	= 0;
		fd_p->link_offset = ptr_to_ram((char *)(fd_p + 1), unit);
		fd_p->rbd_offset = PC586NULL;
	}
	pc_softc[unit].end_fd = --fd_p;
	fd_p->link_offset = PC586NULL;
	fd_p->command = AC_CW_EL;
	fd_p = (fd_t *)(pc_softc[unit].sram + OFFSET_RU);

	fd_p->rbd_offset = ptr_to_ram((char *)rbd_p, unit);
	for(i = 0; i < N_RBD; i++, rbd_p++) {
		rbd_p->r.status = 0;
		rbd_p->r.buffer_addr =
			(unsigned short)((unsigned int)rbd_p->rbuffer);
		rbd_p->r.buffer_base = (int)rbd_p->rbuffer >> 16;
#ifdef DEBUG
		if (pc586debug)
			printf("pc%d: create rbuf #4 base=%x addr=%x\n",
			       unit, rbd_p->r.buffer_base,
			       rbd_p->r.buffer_addr);
#endif
		rbd_p->r.size = RCVBUFSIZE;
		if (i != N_RBD-1) {
			rbd_p->r.next_rbd_offset =
				ptr_to_ram((char *)(rbd_p + 1), unit);
		} else {
			rbd_p->r.next_rbd_offset = PC586NULL;
			rbd_p->r.size |= AC_CW_EL;
			pc_softc[unit].end_rbd = (rbd_t *)rbd_p;
		}
	}
	return (char *)pc_softc[unit].begin_fd;
}

/*
 * pc586rustrt:
 *
 *	This routine starts the receive unit running.  First checks if the
 *	board is actually ready, then the board is instructed to receive
 *	packets again.
 *
 */
void
pc586rustrt(int unit)
{
	volatile scb_t	*scb_p;
	char		*strt;

	scb_p = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	purgescball(scb_p);
	if ((scb_p->scb_status & SCB_RUS_READY) == SCB_RUS_READY)
		return;

	strt = pc586bldru(unit);
	scb_p->scb_command = SCB_RU_STRT;
	scb_p->scb_rfa_offset = ptr_to_ram(strt, unit);
	fdcache(HP700_SID_KERNEL, (vm_offset_t)pc_softc[unit].sram, LAN_BUFF_SIZE);
	pc586chatt(unit);
}

/*
 * pc586diag:
 *
 *	This routine does a 586 op-code number 7, and obtains the
 *	diagnose status for the pc586.
 *
 */
int
pc586diag(int unit)
{
	volatile scb_t	*scb_p;
	volatile ac_t	*cb_p;
	int		i;

	scb_p = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	purgescball(scb_p);
	if (scb_p->scb_status & SCB_SW_INT) {
		printf("pc%d diag(): bad initial state %u\n",
			unit, scb_p->scb_status);
		pc586ack(unit);
	}
	cb_p = (volatile ac_t *)(pc_softc[unit].sram + OFFSET_CU);
	cb_p->ac_status	= 0;
	cb_p->ac_command = (AC_DIAGNOSE|AC_CW_EL);
	fdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof *cb_p);
	scb_p->scb_command = SCB_CU_STRT;
	flushscbcmd(scb_p);
	pc586chatt(unit);
	for (i = 0; i < 0xffff; i++) {
		if ((cb_p->ac_status & AC_SW_C))
			break;
		pdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof *cb_p);
	}
	if (i == 0xffff || !(cb_p->ac_status & AC_SW_OK)) {
		printf("pc%d: diag failed; status = %x, command = %x\n",
		       unit, cb_p->ac_status, cb_p->ac_command);
		return(FALSE);
	}
	purgescbstat(scb_p);
	if ( (scb_p->scb_status & SCB_SW_INT) && (scb_p->scb_status != SCB_SW_CNA) )  {
		printf("pc%d diag(): bad final state %x\n",
			unit, scb_p->scb_status);
		pc586ack(unit);
	}
	return(TRUE);
}

/*
 * pc586config:
 *
 *	This routine does a standard config of the pc586 board.
 *
 */
int
pc586config(int unit)
{
	volatile scb_t	*scb_p;
	volatile ac_t	*cb_p;
	int 		i;
	multicast_t	*mc_p;
	multicast_t	*mp;

	scb_p = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	purgescball(scb_p);
	if ((scb_p->scb_status != SCB_SW_CNA) &&
	    (scb_p->scb_status & SCB_SW_INT) ) {
		printf("pc%d config(): unexpected initial state %x\n",
			unit, scb_p->scb_status);
	}
	pc586ack(unit);

	cb_p = (volatile ac_t *)(pc_softc[unit].sram + OFFSET_CU);
	cb_p->ac_status	= 0;
	cb_p->ac_command = (AC_CONFIGURE|AC_CW_EL);
	cb_p->ac_link_offset = PC586NULL;

	/*
	 * below is the default board configuration from p2-28 from 586 book
	 */
#if 0
	cb_p->cmd.configure.fifolim_bytecnt 	= 0x080c;
	cb_p->cmd.configure.addrlen_mode  	= 0x2600;
	cb_p->cmd.configure.linprio_interframe	= 0x6000;
	cb_p->cmd.configure.slot_time      	= 0xf200;
	if (pc_softc[unit].ds_if.if_flags & NET_STATUS_PROMISC)
		cb_p->cmd.configure.hardware	= 0x0001;
	else
		cb_p->cmd.configure.hardware	= 0x0000;
	cb_p->cmd.configure.min_frame_len   	= 0x0040;
#else
	/*
	 * The above code was obviously written for a different
	 * byte order.  I added the `NET_STATUS_PROMISC' code just for
	 * completeness.  (jef)
	 */
	cb_p->cmd.configure.fifolim_bytecnt 	= 0x0c08;
	cb_p->cmd.configure.addrlen_mode  	= 0x0026;
	cb_p->cmd.configure.linprio_interframe	= 0x0060;
	cb_p->cmd.configure.slot_time      	= 0x00f2;
	if (pc_softc[unit].ds_if.if_flags & NET_STATUS_PROMISC)
		cb_p->cmd.configure.hardware	= 0x0100;
	else
		cb_p->cmd.configure.hardware	= 0x0000;
	cb_p->cmd.configure.min_frame_len   	= 0x4000;
#endif
	fdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof(*cb_p));

	scb_p->scb_command = SCB_CU_STRT;
	flushscbcmd(scb_p);
	pc586chatt(unit);

	for (i = 0; i < 0xffff; i++) {
		if ((cb_p->ac_status & AC_SW_C))
			break;
		pdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof *cb_p);
	}
	if (i == 0xffff || !(cb_p->ac_status & AC_SW_OK)) {
		printf("pc%d: config-configure failed; status = %x\n",
			unit, cb_p->ac_status);
		return(FALSE);
	}
	purgescbstat(scb_p);
#if 0
	if (scb_p->scb_status & SCB_SW_INT) {
		printf("pc%d configure(): bad configure state %x\n",
			unit, scb_p->scb_status);
		pc586ack(unit);
	}
#else
	/*
	 * I have no idea what the above test was supposed to be doing.
	 * As far as I can tell, the chip is *supposed* to set SCB_SW_CNA
	 * upon completion of a command.  (jef)
	 */
	if ((scb_p->scb_status & SCB_SW_INT) != SCB_SW_CNA) {
		printf("pc%d configure(): unexpected configure state %x\n",
			unit, scb_p->scb_status);
	}
#endif

	mc_p = (multicast_t *)cb_p;
	mp = &pc_softc[unit].sc_mcast;

	mc_p->ac_status	= 0;
	mc_p->ac_command = (AC_MCSETUP|AC_CW_EL);
	mc_p->ac_link_offset = PC586NULL;

	mc_p->mc_count = mp->mc_count;
	for (i = 0; i < mp->mc_count; i++)
	    mc_p->mc_addr[i] = mp->mc_addr[i];

	fdcache(HP700_SID_KERNEL, (vm_offset_t)mc_p, sizeof(*mc_p));

	scb_p->scb_command = SCB_CU_STRT;
	flushscbcmd(scb_p);
	pc586chatt(unit);

	for (i = 0; i < 0xffff; i++) {
		if ((cb_p->ac_status & AC_SW_C))
			break;
		pdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof *cb_p);
	}
	if (i == 0xffff || !(cb_p->ac_status & AC_SW_OK)) {
		printf("pc%d: multicast configure failed; status = %x\n",
		       unit, cb_p->ac_status);
		return(FALSE);
	}
	purgescbstat(scb_p);

	if ((scb_p->scb_status & SCB_SW_INT) != SCB_SW_CNA) {
		printf("pc%d configure(): unexpected multicast state %x\n",
		       unit, scb_p->scb_status);
	}

	cb_p->ac_status = 0;
	cb_p->ac_command = (AC_IASETUP|AC_CW_EL);
	cb_p->ac_link_offset = PC586NULL;

	bcopy16(pc_softc[unit].ds_addr, cb_p->cmd.iasetup, ETHER_ADD_SIZE);
	fdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof(*cb_p));

	scb_p->scb_command = SCB_CU_STRT;
	flushscbcmd(scb_p);
	pc586chatt(unit);

	for (i = 0; i < 0xffff; i++) {
		if ((cb_p->ac_status & AC_SW_C))
			break;
		pdcache(HP700_SID_KERNEL, (vm_offset_t)cb_p, sizeof *cb_p);
	}
	if (i == 0xffff || !(cb_p->ac_status & AC_SW_OK)) {
		printf("pc%d: config-address failed; status = %x\n",
			unit, cb_p->ac_status);
		return(FALSE);
	}

	purgescbstat(scb_p);
	if ((scb_p->scb_status & SCB_SW_INT) != SCB_SW_CNA) {
		printf("pc%d configure(): unexpected final state %x\n",
			unit, scb_p->scb_status);
	}
	pc586ack(unit);

	return(TRUE);
}

/*
 * pc586ack:
 */
void
pc586ack(int unit)
{
	volatile scb_t	*scb_p;
	int i;

	scb_p = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	purgescball(scb_p);
	scb_p->scb_command = (scb_p->scb_status & SCB_SW_INT);
	flushscbcmd(scb_p);
	if (!scb_p->scb_command)
		return;
	pc586chatt(unit);
	i = 1000000;
	do
		purgescbcmd(scb_p);
	while (scb_p->scb_command && i-- > 0);
	if (!i)
		printf("pc%d pc586ack(): board not accepting command.\n", unit);
}

volatile char *
ram_to_ptr(
	   unsigned short	offset,
	   int			unit)
{
	if (offset == PC586NULL)
		return(NULL);
	if (offset > 0x3fff) {
		printf("pc%d: ram_to_ptr(%x, %d)\n", unit, offset, unit);
		panic("range");
		return(NULL);
	}
	return(pc_softc[unit].sram + offset);
}

#ifndef	REQUEUE_DBG
int
bad_rbd_chain(
	unsigned short	offset,
	int		unit)
{
	volatile rbd_t	*rbdp;
	volatile char *sram = pc_softc[unit].sram;

	for (;;) {
		if (offset == PC586NULL)
			return 0;
		if (offset > 0x3fff) {
			printf("pc%d: bad_rbd_chain offset = %x\n",
				unit, offset);
			pc586_cntrs[unit].rcv.bad_chain++;
			return 1;
		}

		rbdp = (volatile rbd_t *)(sram + offset);
		pcacheline(HP700_SID_KERNEL, (vm_offset_t)rbdp);
		offset = rbdp->next_rbd_offset;
	}
}
#endif

unsigned short
ptr_to_ram(
	   char	*k_va,
	   int unit)
{
	return((unsigned short)(k_va - pc_softc[unit].sram));
}

#ifdef DEBUG
void
dumpbytes(
	unsigned char *dp,
	int len)
{
	static char digits[] = "0123456789ABCDEF";
	char cbuf[3];
	register int i;

	if (pc586debug < 3)
		return;

	cbuf[2] = '\0';
	for (i = 0; i < len; i++) {
		cbuf[0] = digits[*dp >> 4];
		cbuf[1] = digits[*dp++ & 0xF];
		printf("%s%c", cbuf, (i%32)==31 ? '\n' : ' ');
	}
	if (i%32)
		printf("\n");
}

#if 0
void
pc586scb(int unit)
{
	volatile scb_t	*scb;
	unsigned short	 i;

	scb = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	purgescball(scb);
	i = scb->scb_status;
	printf("stat: stat %x, cus %x, rus %x //",
		(i&0xf000)>>12, (i&0x0700)>>8, (i&0x0070)>>4);
	i = scb->scb_command;
	printf(" cmd: ack %x, cuc %x, ruc %x\n",
		(i&0xf000)>>12, (i&0x0700)>>8, (i&0x0070)>>4);

	printf("crc %u[%u], align %u[%u], rsc %u[%u], ovr %u[%u]\n",
		scb->scb_crcerrs, pc586_cntrs[unit].rcv.crc,
		scb->scb_alnerrs, pc586_cntrs[unit].rcv.frame,
		scb->scb_rscerrs, pc586_cntrs[unit].rcv.rscerrs,
		scb->scb_ovrnerrs, pc586_cntrs[unit].rcv.ovrnerrs);

	printf("cbl %u, rfa %u //", scb->scb_cbl_offset, scb->scb_rfa_offset);
}

void
pc586tbd(int unit)
{
	pc_softc_t	*is = &pc_softc[unit];
	volatile tbd_t	*tbd_p = (tbd_t *)(is->sram + OFFSET_TBD);
	int 		i = 0;
	int		sum = 0;

	do {
		pcacheline(HP700_SID_KERNEL, (vm_offset_t)tbd_p);
		sum += (tbd_p->act_count & ~TBD_SW_EOF);
		printf("pc%d: %d: addr=%x count=%u(%d) next=%x base=%x\n",
		       unit, i++, tbd_p->buffer_addr,
		       (tbd_p->act_count & ~TBD_SW_EOF), sum,
		       tbd_p->next_tbd_offset,
		       tbd_p->buffer_base);
		if (tbd_p->act_count & TBD_SW_EOF)
			break;
		tbd_p = (tbd_t *)(is->sram + tbd_p->next_tbd_offset);
	} while (1);
	dumpbytes((unsigned char *)(is->sram + OFFSET_TBUF), sum);
}

void
dumpehdr(struct ether_header *eh)
{
	printf("type=0x%x", eh->ether_type);
	printf(" daddr=%s", ether_sprintf(eh->ether_dhost));
	printf(" saddr=%s", ether_sprintf(eh->ether_shost));
}
#endif

#endif	/* DEBUG */

#if XKMACHKERNEL
boolean_t
pc586_xkcopy(
	char 		*src,
	long		length,
	void		*arg)
{
	struct pc586_xk *xk = (struct pc586_xk *)arg;

	if (xk->max - xk->len < length)
	    length = xk->max - xk->len;
	(void)bcopy((char *)src, (char *)xk->addr + xk->len, length);
	xk->len += length;
	return TRUE;
}

boolean_t
pc586_xkcopy_test(
	char 		*src,
	long		length,
	void		*arg)
{
	struct pc586_xk *xk = (struct pc586_xk *)arg;
	/* 
         * Validate source address 
	 * (in case of messages constructed in place,
	 * users may have withdrawn the bits)
	 */
	if (pmap_extract(kernel_pmap, (vm_offset_t)src) == (vm_offset_t)0) {
	    return FALSE;
	}

	if (xk->max - xk->len < length)
	    length = xk->max - xk->len;
	(void)bcopy((char *)src, (char *)xk->addr + xk->len, length);
	xk->len += length;
	return TRUE;
}

boolean_t
pc586_xkcopy16(
	char 		*src,
	long		length,
	void		*arg)
{
	char **physaddr = (char **)arg;

	(void)bcopy16((char *)src, *physaddr, length);

	*physaddr += length;
	return TRUE;
}

boolean_t
pc586_xkcopy16_test(
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

/*
 *    Support for generic xkernel operations
 *    The openenable interface allows the driver 
 *    get the address of the higher level protocol to which it interfaces
 */
xkern_return_t
xkec_openenable(
	XObj self, 
	XObj hlp, 
	XObj hlptype,
	Part part)
{ 
	int index;

	index = self->instName[strlen(self->instName) - 1] - '0';
	if (index < 0 || index >= NLAN) {
            xTrace0(xkecp, TR_ERRORS, 
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
	if ((ifp->if_flags & (NET_STATUS_UP | NET_STATUS_RUNNING))
	    != (NET_STATUS_UP|NET_STATUS_RUNNING))
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
	s = spllan();
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
	s = spllan();
	enqueue_head(&ior_xkernel_qhead, (queue_entry_t)ior);
#if     XKSTATS
	pc586_xkior_queued++;
#endif /* XKSTATS */
	splx(s);
	return TRUE;
}

/*
 * xkec_push
 * the interface from the xkernel for outgoing packets
 */
xmsg_handle_t
xkec_push(
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

	spl = spllan();
        ior = (io_req_t)dequeue_head(&ior_xkernel_qhead);
	if (ior == (io_req_t)0) {
#if     XKSTATS
	    pc586_xkior_dropped++;
#endif /* XKSTATS */
	    splx(spl);
	    return XMSG_NULL_HANDLE;
	}
#if     XKSTATS
	pc586_xkior_queued--;
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

	xTrace3(xkecp, TR_EVENTS, "xkec: send to %x.%x.%x", dest->high, dest->mid, dest->low);

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
	if (xknet_write(&pc_softc[ior->io_unit].ds_if,
			pc586start, ior) != D_IO_QUEUED )
	    panic( "xknet_write: bad return value" );

	return XMSG_NULL_HANDLE;
}

static void
getLocalEthHost(
	ETHhost *host,
	int unit)
{
	bcopy ((char *)pc_softc[unit].ds_addr, (char *)host, sizeof( ETHhost ));
        xTrace3(xkecp, TR_EVENTS, "xkec: host addr %x.%x.%x",
                host->high, host->mid, host->low);
}

/*
 *  xkec_control
 *   Control operations
 */
int
xkec_control(
	XObj  self,
	int   op,
	char *buf,
	int   len)
{  
	int	unit;

	xTrace1(xkecp, TR_FULL_TRACE, "xkec_control: operation %x", op);

        unit = self->instName[strlen(self->instName) - 1] - '0';

        switch ( op ) {
        case GETMYHOST:
            if (len >= sizeof(ETHhost)) {
                if (unit >= 0 && unit < NLAN) {
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
                ret = pc586setstat( (dev_t)unit,
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
xkec_init(
	XObj self)
{
	int	unit, i;

	xTrace0(xkecp, TR_GROSS_EVENTS, "xkec_init");

	unit = self->instName[strlen(self->instName) - 1] - '0';
	if (unit < 0 || unit >= NLAN) {
	  unit = 0;
	  xTrace1(xkecp, TR_ERRORS, "xkec: assuming unit 0 for %s",
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
	if ((pc_softc[unit].ds_if.if_flags &
	     (NET_STATUS_UP | NET_STATUS_RUNNING))
	    == (NET_STATUS_UP | NET_STATUS_RUNNING)) {
	    pc_softc[unit].xk_ltype = XK_LOWEST_TYPE;
	    pc_softc[unit].xk_htype = XK_HIGHEST_TYPE;
	} else {
	    pc_softc[unit].xk_ltype = 0;
	    pc_softc[unit].xk_htype = 0xffff;
	}	    
	if (pc586open((dev_t)unit, 0, (io_req_t) 0) == ENXIO) 
	    panic("xkec_init: the device does not exist");
	pc_softc[unit].xk_up = TRUE;

	if (allocateResources(self) != XK_SUCCESS)
	    panic("xkec_init: couldn't allocate memory to start the driver");
	
	self->openenable = xkec_openenable;
	self->push = xkec_push;
	self->control = xkec_control;

	xTrace0(xkecp, TR_GROSS_EVENTS, "xkec_init returns");
	return XK_SUCCESS;
}
#endif /* XKMACHKERNEL */

#endif	/* NLAN > 0 */
