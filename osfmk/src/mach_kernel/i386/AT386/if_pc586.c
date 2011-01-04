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
 * Revision 2.18.4.3  92/06/24  17:59:04  jeffreyh
 * 	Updated to new at386_io_lock() interface.
 * 	[92/06/03            bernadat]
 * 
 * Revision 2.18.4.2  92/04/30  11:56:13  bernadat
 * 	Adapt to CBUS/MBUS
 * 	[92/04/21            bernadat]
 * 
 * Revision 2.18.4.1  92/02/18  18:53:19  jeffreyh
 * 	Support for Corollary MP, switch to master for i386 ports R/W
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.18  91/11/12  11:09:47  rvb
 * 	Undo 2.16.1.1
 * 	[91/10/25            rvb]
 * 	Typo in transcribing the 2.5 changes missed ! before pc586read().
 * 	Funny thing is that we did have this fixed in a branch once.
 * 	[91/10/16            rvb]
 * 
 * Revision 2.17  91/10/07  17:25:48  af
 * 	Add a lot of fixes/improvements from 2.5.
 * 	[91/09/04            rvb]
 * 
 * Revision 2.16.1.1  91/09/03  17:27:39  af
 * 	Be strict on matching the port and the device in the probe routine.
 *
 * Revision 2.16  91/08/24  11:58:12  af
 * 	New MI autoconf.
 * 	[91/08/02  02:54:27  af]
 * 
 * Revision 2.15  91/05/14  16:25:37  mrt
 * 	Correcting copyright
 * 
 * Revision 2.14  91/05/13  06:02:51  af
 * 	Made code under CMUCS standard.
 * 	[91/05/12  15:50:10  af]
 * 
 * Revision 2.13  91/03/16  14:46:38  rpd
 * 	Updated for new kmem_alloc interface.
 * 	[91/03/03            rpd]
 * 	Changed net_filter to net_packet.
 * 	[91/01/15            rpd]
 * 
 * Revision 2.12  91/02/14  14:43:03  mrt
 * 	You must check RBD_SW_EOF for rbd chain termination, a link of 0xffff
 * 	does not work.  Also we've just seen a status of 0xffff in rcv() with
 * 	a bad fd_p->link_offset; we'll now reset.
 * 	[91/01/17            rvb]
 * 
 * Revision 2.11  91/02/05  17:18:19  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:44:30  mrt]
 * 
 * Revision 2.10  91/01/08  15:11:45  rpd
 * 	Changed NET_KMSG_GET, NET_KMSG_FREE to net_kmsg_get, net_kmsg_put.
 * 	[91/01/05            rpd]
 * 
 * Revision 1.8.1.11  90/12/07  22:53:44  rvb
 * 	Fix a few problems if your hardware goes haywire.
 * 	Noted by Ali.
 * 	[90/12/11            rvb]
 * 
 * Revision 2.9  90/12/20  16:38:09  jeffreyh
 * 	Changes for __STDC__
 * 	[90/12/07            jeffreyh]
 * 
 * Revision 1.8.1.12  91/01/06  22:11:54  rvb
 * 	Try try again to get the ram_to_ptr problem in requeue.
 * 	[90/11/29            rvb]
 * 
 * Revision 2.8  90/11/26  14:49:59  rvb
 * 	jsb bet me to XMK34, sigh ...
 * 	[90/11/26            rvb]
 * 	Synched 2.5 & 3.0 at I386q (r1.8.1.10) & XMK35 (r2.8)
 * 	[90/11/15            rvb]
 * 
 * Revision 2.7  90/11/05  14:28:11  rpd
 * 	Convert for pure kernel
 * 	[90/11/02            rvb]
 * 
 * 	Init scb and reset as per spec.  Use bcopy16 vs pc586copy as per spec.
 * 	Accumulate counters at hwrst.  Add counters everywhere.
 * 	Editing and style and consistency cleanup.
 * 	Flush NOP's: this required major mods to hwrst and everything it
 * 	called -- testing OK is not correct if the command is not COMPLETE.
 * 	Lot's of code clean up in xmt, rcv, reqfd.
 * 	Flush pc_softc_t "address" and pc586ehcpy.
 * 	Handle loopback of our ownbroadcast.
 * 	The rcv and xmt loops have been rewriten not copy sram
 * 	to the t_packet buffer.  This yields a teriffic thruput
 * 	improvement.
 * 	Add Notes and sram map.
 * 	[90/09/28            rvb]
 * 
 * Revision 1.8.1.9  90/09/18  08:39:03  rvb
 * 	Debugging printout to find shutdown reason.
 * 	[90/09/14            rvb]
 * 
 * Revision 1.8.1.8  90/08/25  15:44:14  rvb
 * 	Use take_<>_irq() vs direct manipulations of ivect and friends.
 * 	[90/08/20            rvb]
 * 
 * 	Fix DSF_RUNNING.  Some more cleanup.
 * 	[90/08/14            rvb]
 * 
 * Revision 1.8.1.7  90/07/27  11:26:06  rvb
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[90/07/27            rvb]
 * 
 * Revision 1.8.1.6  90/07/10  11:43:44  rvb
 * 	New style probe/attach.
 * 	[90/06/15            rvb]
 * 
 * Revision 2.4  90/06/02  14:49:13  rpd
 * 	Converted to new IPC.
 * 	[90/06/01            rpd]
 * 
 * Revision 2.3  90/05/29  18:48:51  rwd
 * 	Test for next_rbd_offset NULL instead of RBD_SW_EOF in
 * 	pc586reqfd.  I don't know why this is needed on the pure kernel
 * 	but not in the mainline - our slower device interface triggers a
 * 	race?
 * 	[90/05/25            dbg]
 * 
 * Revision 2.2  90/05/03  15:44:10  dbg
 * 	Check whether device exists on open.
 * 	[90/05/02            dbg]
 * 	MACH_KERNEL conversion.
 * 	[90/04/23            dbg]
 * 
 * Revision 1.8.1.5  90/03/16  18:15:26  rvb
 * 	Clean up the i386_dev/i386_ctlr confusion.
 * 	[90/03/15            rvb]
 * 
 * Revision 1.8.1.4  90/02/28  15:49:46  rvb
 * 	Fix numerous typo's in Olivetti disclaimer.
 * 	Revision 1.10  90/02/07  11:29:37  [eugene]
 * 	expanded NOP's and loops to allow for faster machines
 * 
 * Revision 1.8.1.3  90/01/08  13:31:17  rvb
 * 	Add Intel copyright.
 * 	Add Olivetti copyright.
 * 	[90/01/08            rvb]
 * 
 * Revision 1.8.1.2  89/11/10  09:52:15  rvb
 * 	Revision 1.7  89/10/31  16:05:45  eugene
 * 	added changes for new include files
 * 
 * Revision 1.7  89/10/31  16:05:45  eugene
 * added changes for new include files
 * 
 * Revision 1.6  89/10/02  10:32:04  eugene
 * got the previous change correct
 * 
 * Revision 1.8  89/09/20  17:28:33  rvb
 * 	Revision 1.3  89/08/01  11:17:32  eugene
 * 	Fixed ethernet driver hang on 25Mhz machines
 * 
 * Revision 1.7  89/08/08  21:46:46  jsb
 * 	Just count overruns and do not print anything.
 * 	[89/08/03            rvb]
 * 
 * Revision 1.6  89/07/17  10:40:38  rvb
 * 	Olivetti Changes to X79 upto 5/12/89:
 * 		& Fixed reseting when RNR is detected.
 * 	[89/07/11            rvb]
 * 
 * Revision 1.5  89/04/05  13:00:53  rvb
 * 	Moved "softc" structure from .h to here and some cleanup
 * 	[89/03/07            rvb]
 * 
 * 	made ac_t and scb_t volatile pointers for gcc.
 * 	[89/03/07            rvb]
 * 
 * 	collapse pack_u_long_t for gcc.
 * 	[89/03/06            rvb]
 * 
 * Revision 1.4  89/03/09  20:06:01  rpd
 * 	More cleanup.
 * 
 * Revision 1.3  89/02/26  12:42:12  gm0w
 * 	Changes for cleanup.
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
 */

/* 
 *	Olivetti PC586 Mach Ethernet driver v1.0
 *	Copyright Ing. C. Olivetti & C. S.p.A. 1988, 1989
 *	All rights reserved.
 *
 */ 

/*
 *   Copyright 1988, 1989 by Olivetti Advanced Technology Center, Inc.,
 * Cupertino, California.
 * 
 * 		All Rights Reserved
 * 
 *   Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appears in all
 * copies and that both the copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Olivetti
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * 
 *   OLIVETTI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL OLIVETTI BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUR OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * 
 * 
 *   Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.
 * 
 * 		All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appears in all
 * copies and that both the copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Intel
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * 
 * INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
 *  n.	Board Memory Map
	RFA/FD	0   -   227	0x228 bytes
		 226 = 0x19 * 0x16 bytes
	RBD	 228 - 3813	0x35ec bytes
		35e8 = 0x19 *	0x228 bytes
				== 0x0a bytes (bd) + 2 bytes + 21c bytes
	CU	3814 - 3913	0x100 bytes
	TBD	3914 - 39a3	0x90 bytes 
		  90 = No 18 * 0x08 bytes 
	TBUF	39a4 - 3fdd	0x63a bytes (= 1594(10))
	SCB	3fde - 3fed	0x10 bytes
	ISCP	3fee - 3ff5	0x08 bytes
	SCP	3ff6 - 3fff	0x0a bytes
 *		
 */

/*
 * NOTE:
 *
 *	Currently this driver doesn't support trailer protocols for
 *	packets.  Once that is added, please remove this comment.
 *
 *	Also, some lacking material includes the DLI code.  If you
 *	are compiling this driver with DLI set, lookout, that code
 * 	has not been looked at.
 *
 */

#define DEBUG
#undef IF_CNTRS
#define	NDLI	0

#include	<pc586.h>

#include	<platforms.h>
#include 	<mp_v1_1.h>
#include	<kern/time_out.h>
#include	<kern/spl.h>
#include	<device/device_types.h>
#include	<device/errno.h>
#include	<device/io_req.h>
#include	<device/if_hdr.h>
#include	<device/if_ether.h>
#include	<device/net_status.h>
#include	<device/net_io.h>
#include	<device/ds_routines.h>
#include	<i386/ipl.h>
#include	<i386/pmap.h>
#include	<mach/vm_param.h>
#include	<vm/vm_kern.h>
#include	<chips/busses.h>
#include	<i386/AT386/if_pc586.h>
#include	<i386/AT386/misc_protos.h>
#include	<kern/misc_protos.h>

#include <i386/AT386/mp/mp.h>
#ifdef	CBUS
#include <busses/cbus/cbus.h>
#endif	/* CBUS */

#if	MP_V1_1
#include <i386/AT386/mp/mp_v1_1.h>
#endif	/* MP_V1_1 */

#if	__STDC__
#define CMD(x, y, unit) *(u_short *)(pc_softc[unit].prom + OFFSET_ ## x) = (u_short) (y)
#else	/* __STDC__ */
#define CMD(x, y, unit) *(u_short *)(pc_softc[unit].prom + OFFSET_/**/x) = (u_short) (y)
#endif	/* __STDC__ */

#define pc586chatt(unit)  CMD(CHANATT, 0x0001, unit)
#define pc586inton(unit)  CMD(INTENAB, CMD_1,  unit)
#define pc586intoff(unit) CMD(INTENAB, CMD_0,  unit)

struct pc_softc { 
	struct	ifnet	ds_if;		/* generic interface header */
	u_char	ds_addr[6];		/* Ethernet hardware address */
	int	flags;
        int     seated;
        int     timer;
        int     open;
        fd_t    *begin_fd;
	fd_t    *end_fd;
	rbd_t   *end_rbd;
	char    *prom;
	char	*sram;
	int     tbusy;
	short	mode;
};
pc_softc_t	pc_softc[NPC586];

struct pc586_cntrs {
	struct {
		u_int xmt, xmti;
		u_int defer;
		u_int busy;
		u_int sleaze, intrinsic, intrinsic_count;
		u_int chain;
	} xmt; 
	struct {
		u_int rcv;
		u_int ovw;
		u_int crc;
		u_int frame;
		u_int rscerrs, ovrnerrs;
		u_int partial, bad_chain, fill;
	} rcv;
	u_int watch;
} pc586_cntrs[NPC586];

#include	<i386/AT386/if_pc586_entries.h>
#include	<i386/AT386/misc_protos.h>

/* Forward */

extern int		pc586probe(
				int			port,
				struct bus_device	* dev);
extern void		pc586attach(
				struct bus_device	* dev);
extern int		pc586reset(
				int			unit);
extern int		pc586init(
				int			unit);
extern void		pc586start(
				int			unit);
extern int		pc586read(
				int			unit,
				fd_t			*fd_p);
extern void		pc586send_packet_up(
				io_req_t		m,
				struct ether_header	*eh,
				pc_softc_t		*is);
extern int		pc586hwrst(
				int			unit);
extern void		pc586watch(
				caddr_t			b_ptr);
extern void		pc586rcv(
				int			unit);
extern int		pc586requeue(
				int			unit,
				fd_t			*fd_p);
extern void		pc586xmt(
				int			unit,
				io_req_t		m);
extern void		pc586bldcu(
				int			unit);
extern char		* pc586bldru(
				int			unit);
extern void		pc586rustrt(
				int			unit);
extern boolean_t	pc586diag(
				int			unit);
extern boolean_t	pc586config(
				int			unit);
extern void		pc586ack(
				int			unit);
extern void		pc586scb(
				int			unit);
extern void		pc586tbd(
				int			unit);
extern char		*ram_to_ptr(
				u_short			offset,
				int			unit);
extern int		bad_rbd_chain(
				u_short			offset,
				int			unit);
extern u_short		ptr_to_ram(
				char			*kva,
				int			unit);

static caddr_t pc586_std[NPC586] = { 0 };
static struct bus_device *pc586_info[NPC586];
struct	bus_driver	pcdriver = 
	{(probe_t)pc586probe, 0, pc586attach, 0, pc586_std, "pc",
	pc586_info, 0, 0, 0};

char	t_packet[ETHERMTU + sizeof(struct ether_header) + sizeof(long)];
int	xmt_watch = 0;

#ifdef	IF_CNTRS
int pc586_narp = 1, pc586_arp = 0;
int pc586_ein[32], pc586_eout[32]; 
int pc586_lin[128/8], pc586_lout[128/8]; 
static
log_2(no)
unsigned long no;
{
	return ({ unsigned long _temp__;
		__asm__("bsr %1, %0; jne 0f; xorl %0, %0; 0:" :
		    "=r" (_temp__) : "a" (no));
		_temp__;});
}
#endif	/* IF_CNTRS */

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
pc586probe(
	int			port,
	struct bus_device	*dev)
{
	caddr_t		addr = dev->address;
	int		unit = dev->unit;
	int		len = round_page(0x4000);
	int		sram_len = round_page(0x4000);
	extern		vm_size_t mem_size;
	int		i;
	volatile char	*b_prom;
	volatile char	*b_sram;
	at386_io_lock_state();

	if ((unit < 0) || (unit > NPC586)) {
		printf("pc%d: board out of range [0..%d]\n",
			unit, NPC586);
		return(0);
	}
	if (addr < (caddr_t)mem_size && addr > (caddr_t)0x100000)
		return 0;

	if (kmem_alloc_pageable(kernel_map, (vm_offset_t *) &b_prom, len)
							!= KERN_SUCCESS) {
		printf("pc%d: can not allocate memory for prom.\n", unit);
		return 0;
	}
	if (kmem_alloc_pageable(kernel_map, (vm_offset_t *) &b_sram, sram_len)
							!= KERN_SUCCESS) {
		printf("pc%d: can not allocate memory for sram.\n", unit);
		return 0;
	}
	(void)pmap_map((vm_offset_t)b_prom, (vm_offset_t)addr, 
			(vm_offset_t)addr+len, 
			VM_PROT_READ | VM_PROT_WRITE);
	if ((int)addr > 0x100000)			/* stupid hardware */
		addr += EXTENDED_ADDR;
	addr += 0x4000;					/* sram space */
	(void)pmap_map((vm_offset_t)b_sram, (vm_offset_t)addr, 
			(vm_offset_t)addr+sram_len, 
			VM_PROT_READ | VM_PROT_WRITE);

	at386_io_lock(MP_DEV_WAIT);
	*(b_prom + OFFSET_RESET) = 1;
	{ int i; for (i = 0; i < 1000; i++);	/* 4 clocks at 6Mhz */}
	*(b_prom + OFFSET_RESET) = 0;

	/*
	 * Test vendor address bytes.
	 */
	if (*(u_short *)(b_prom + OFFSET_PROM_IA1) != PC586_NODE_IA1 ||
	    *(u_short *)(b_prom + OFFSET_PROM_IA2) != PC586_NODE_IA2 ||
	    *(u_short *)(b_prom + OFFSET_PROM_IA3) != PC586_NODE_IA3) {
		kmem_free(kernel_map, (vm_offset_t)b_prom, len);
		kmem_free(kernel_map, (vm_offset_t)b_sram, sram_len);
		at386_io_unlock();
		return(0);
	}
	pc_softc[unit].seated = TRUE;
	pc_softc[unit].prom = (char *)b_prom;
	pc_softc[unit].sram = (char *)b_sram;
	at386_io_unlock();
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
 * input	: bus_device structure setup in autoconfig
 * output	: board structs and ifnet is setup
 *
 */

void
pc586attach(
	struct bus_device	*dev)
{
	struct	ifnet	*ifp;
	u_char		*addr_p;
	u_short		*b_addr;
	u_char		unit = (u_char)dev->unit;	
	pc_softc_t	*sp = &pc_softc[unit];
	volatile scb_t	*scb_p;
	at386_io_lock_state();

	take_dev_irq(dev);
	printf(", port = %x, spl = %d, pic = %d. ",
		dev->address, dev->sysdep, dev->sysdep1);

	sp->timer = -1;
	sp->flags = 0;
	sp->mode = 0;
	sp->open = 0;
	at386_io_lock(MP_DEV_WAIT);
	CMD(RESET, CMD_1, unit);
	{ int i; for (i = 0; i < 1000; i++);	/* 4 clocks at 6Mhz */}
	CMD(RESET, CMD_0, unit);
	b_addr = (u_short *)(sp->prom + OFFSET_PROM);
	addr_p = (u_char *)sp->ds_addr;
	addr_p[0] = b_addr[0];
	addr_p[1] = b_addr[1];
	addr_p[2] = b_addr[2];
	addr_p[3] = b_addr[3];
	addr_p[4] = b_addr[4];
	addr_p[5] = b_addr[5];
	printf("ethernet id [%x:%x:%x:%x:%x:%x]",
		addr_p[0], addr_p[1], addr_p[2],
		addr_p[3], addr_p[4], addr_p[5]);

	scb_p = (volatile scb_t *)(sp->sram + OFFSET_SCB);
	scb_p->scb_crcerrs = 0;			/* initialize counters */
	scb_p->scb_alnerrs = 0;
	scb_p->scb_rscerrs = 0;
	scb_p->scb_ovrnerrs = 0;
	at386_io_unlock();

	ifp = &(sp->ds_if);
	ifp->if_unit = unit;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST;
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

int
pc586reset(
	int		unit)
{
	pc_softc[unit].ds_if.if_flags &= ~IFF_RUNNING;
	pc_softc[unit].flags &= ~(DSF_LOCK|DSF_RUNNING);
	return(pc586init(unit));

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
pc586init(
	int		unit)
{
	struct	ifnet	*ifp;
	int		stat;
	spl_t		oldpri;

	ifp = &(pc_softc[unit].ds_if);
	oldpri = splnet();
	if ((stat = pc586hwrst(unit)) == TRUE) {
#undef	HZ
#define HZ hz
		timeout((timeout_fcn_t)pc586watch, (void *)&(ifp->if_unit),
			5*HZ);
		pc_softc[unit].timer = 5;

		pc_softc[unit].ds_if.if_flags |= IFF_RUNNING;
		pc_softc[unit].flags |= DSF_RUNNING;
		pc_softc[unit].tbusy = 0;
		pc586start(unit);
#if	DLI
		dli_init();
#endif	/* DLI */
	} else
		printf("pc%d init(): trouble resetting board.\n", unit);
	splx(oldpri);
	return(stat);
}

io_return_t
pc586open(
	dev_t		dev,
	dev_flavor_t	flag,
	io_req_t	ior)
{
	register int	unit;
	pc_softc_t	*sp;

	unit = minor(dev);	/* XXX */
	if (unit < 0 || unit >= NPC586 || !pc_softc[unit].seated)
	    return (D_NO_SUCH_DEVICE);

	pc_softc[unit].ds_if.if_flags |= IFF_UP;
	pc586init(unit);
	return (D_SUCCESS);
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
pc586start(
	int			unit)
{
	io_req_t	m;
	struct	ifnet		*ifp;
	register pc_softc_t	*is = &pc_softc[unit];
	volatile scb_t		*scb_p = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	at386_io_lock_state();

	at386_io_lock(MP_DEV_WAIT);
	if (is->tbusy) {
		if (!(scb_p->scb_status & 0x0700)) { /* ! IDLE */
			is->tbusy = 0;
			pc586_cntrs[unit].xmt.busy++;
			/*
			 * This is probably just a race.  The xmt'r is just
			 * became idle but WE have masked interrupts so ...
			 */
			if (xmt_watch) printf("!!");
		} else
			at386_io_unlock();
			return;
	}

	ifp = &(pc_softc[unit].ds_if);
	IF_DEQUEUE(&ifp->if_snd, m);
	if (m != 0)
	{
		is->tbusy++;
		pc586_cntrs[unit].xmt.xmt++;
		pc586xmt(unit, m);
	}
	at386_io_unlock();
	return;
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
	int			unit,
	fd_t			*fd_p)
{
	register pc_softc_t	*is = &pc_softc[unit];
	register struct ifnet	*ifp = &is->ds_if;
	struct	ether_header	eh;
	ipc_kmsg_t	new_kmsg;
	struct ether_header *ehp;
	struct packet_header *pkt;
	char 	*dp;
	rbd_t			*rbd_p;
	u_char			*buffer_p;
	u_char			*mb_p;
	u_short			mlen, len, clen;
	u_short			bytes_in_msg, bytes_in_mbuf, bytes;


	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		printf("pc%d read(): board is not running.\n", ifp->if_unit);
		pc586intoff(ifp->if_unit);
	}
	pc586_cntrs[unit].rcv.rcv++;
	new_kmsg = net_kmsg_get();
	if (new_kmsg == IKM_NULL) {
	    /*
	     * Drop the received packet.
	     */
	    is->ds_if.if_rcvdrops++;

	    /*
	     * not only do we want to return, we need to drop the packet on
	     * the floor to clear the interrupt.
	     */
	    return 1;
	}
	ehp = (struct ether_header *) (&net_kmsg(new_kmsg)->header[0]);
	pkt = (struct packet_header *)(&net_kmsg(new_kmsg)->packet[0]);

	/*
	 * Get ether header.
	 */
	ehp->ether_type = fd_p->length;
	len = sizeof(struct ether_header);
	bcopy16((char *)fd_p->source, (char *)ehp->ether_shost,
		ETHER_ADD_SIZE);
	bcopy16((char *)fd_p->destination, (char *)ehp->ether_dhost,
		ETHER_ADD_SIZE);

	/*
	 * Get packet body.
	 */
	dp = (char *)(pkt + 1);

	rbd_p = (rbd_t *)ram_to_ptr(fd_p->rbd_offset, unit);
	if (rbd_p == 0) {
	    printf("pc%d read(): Invalid buffer\n", unit);
	    if (pc586hwrst(unit) != TRUE) {
		printf("pc%d read(): hwrst trouble.\n", unit);
	    }
	    net_kmsg_put(new_kmsg);
	    return 0;
	}

	do {
	    buffer_p = (u_char *)(pc_softc[unit].sram + rbd_p->buffer_addr);
	    bytes_in_msg = rbd_p->status & RBD_SW_COUNT;
	    bcopy16((char *)buffer_p,
		       (char *)dp,
		       (bytes_in_msg + 1) & ~1);	/* but we know it's even */
	    len += bytes_in_msg;
	    dp += bytes_in_msg;
	    if (rbd_p->status & RBD_SW_EOF)
		break;
	    rbd_p = (rbd_t *)ram_to_ptr(rbd_p->next_rbd_offset, unit);
	} while ((int) rbd_p);

	pkt->type = ehp->ether_type;
	pkt->length =
		len - sizeof(struct ether_header)
		    + sizeof(struct packet_header);

	/*
	 * Send the packet to the network module.
	 */
	net_packet(ifp, new_kmsg, pkt->length,
		   ethernet_priority(new_kmsg), (io_req_t)0);
	return 1;
}

/*
 * Send a packet back to higher levels.
 */

void
pc586send_packet_up(
	io_req_t		m,
	struct ether_header 	*eh,
	pc_softc_t		*is)
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

io_return_t
pc586output(
	dev_t		dev,
	io_req_t	ior)
{
	register int	unit;

	unit = minor(dev);	/* XXX */
	if (unit < 0 || unit >= NPC586 || !pc_softc[unit].seated)
	    return (D_NO_SUCH_DEVICE);

	return (net_write(&pc_softc[unit].ds_if, pc586start, ior));
}

io_return_t
pc586setinput(
	dev_t		dev,
	ipc_port_t	receive_port,
	int		priority,
	filter_t	filter[],
	unsigned int	filter_count,
	device_t	device)
{
	register int unit = minor(dev);
	if (unit < 0 || unit >= NPC586 || !pc_softc[unit].seated)
	    return (D_NO_SUCH_DEVICE);

	return (net_set_filter(&pc_softc[unit].ds_if,
			receive_port, priority,
			filter, filter_count, device));
}

io_return_t
pc586getstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	status,		/* pointer to OUT array */
	natural_t	*count)		/* out */
{
	register int	unit = minor(dev);
	register pc_softc_t	*sp;

	if (unit < 0 || unit >= NPC586 || !pc_softc[unit].seated)
	    return (D_NO_SUCH_DEVICE);

	sp = &pc_softc[unit];
	return (net_getstat(&sp->ds_if, flavor, status, count));
}

io_return_t
pc586setstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	status,
	natural_t	count)
{
	register int	unit = minor(dev);
	register pc_softc_t	*sp;

	if (unit < 0 || unit >= NPC586 || !pc_softc[unit].seated)
	    return (D_NO_SUCH_DEVICE);

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

		if (ns->flags & IFF_ALLMULTI)
		    mode |= MOD_ENAL;
		if (ns->flags & IFF_PROMISC)
		    mode |= MOD_PROM;

		/*
		 * Force a complete reset if the receive mode changes
		 * so that these take effect immediately.
		 */
		if (sp->mode != mode) {
		    sp->mode = mode;
		    if (sp->flags & DSF_RUNNING) {
			sp->flags &= ~(DSF_LOCK|DSF_RUNNING);
			pc586init(unit);
		    }
		}
		break;
	    }

	    default:
		return (D_INVALID_OPERATION);
	}
	return (D_SUCCESS);
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
pc586hwrst(
	int		unit)
{
	at386_io_lock_state();

	at386_io_lock(MP_DEV_WAIT);
	CMD(CHANATT, CMD_0, unit);
	CMD(RESET, CMD_1, unit);
	{ int i; for (i = 0; i < 1000; i++);	/* 4 clocks at 6Mhz */}
	CMD(RESET,CMD_0, unit);

/*
 *	for (i = 0; i < 1000000; i++); 
 *	with this loop above and with the reset toggle also looping to
 *	1000000.  We don't see the reset behaving as advertised.  DOES
 *	IT HAPPEN AT ALL.  In particular, NORMODE, ENABLE, and XFER
 *	should all be zero and they have not changed at all.
 */
	CMD(INTENAB, CMD_0, unit);
	CMD(NORMMODE, CMD_0, unit);
	CMD(XFERMODE, CMD_1, unit);

	pc586bldcu(unit);

	if (pc586diag(unit) == FALSE || pc586config(unit) == FALSE) {
		at386_io_unlock();
		return(FALSE);
	}

	/* 
	 * insert code for loopback test here
	 *
	 */
	pc586rustrt(unit);

	pc586inton(unit);
	CMD(NORMMODE, CMD_1, unit);
	at386_io_unlock();
	return(TRUE);
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
pc586watch(
	caddr_t	b_ptr)
{
	spl_t	opri;
	int	unit = *b_ptr;

	if ((pc_softc[unit].ds_if.if_flags & IFF_UP) == 0)  {
		return;
	}
	if (pc_softc[unit].timer == -1) {
		timeout((timeout_fcn_t)pc586watch, (void *)b_ptr, 5*HZ);
		return;
	}
	if (--pc_softc[unit].timer != -1) {
		timeout((timeout_fcn_t)pc586watch, (void *)b_ptr, 1*HZ);
		return;
	}

	opri = splnet();
#ifdef	notdef
	printf("pc%d watch(): 6sec timeout no %d\n", unit, ++watch_dead);
#endif	/* notdef */
	pc586_cntrs[unit].watch++;
	if (pc586hwrst(unit) != TRUE) {
		printf("pc%d watch(): hwrst trouble.\n", unit);
		pc_softc[unit].timer = 0;
	} else {
		timeout((timeout_fcn_t)pc586watch, (void *)b_ptr, 1*HZ);
		pc_softc[unit].timer = 5;
	}
	splx(opri);
}

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
pc586intr(
	int		unit)
{
	volatile scb_t	*scb_p = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	volatile ac_t	*cb_p  = (volatile ac_t *)(pc_softc[unit].sram + OFFSET_CU);
	int		next, x;
	int		i;
	u_short		int_type;

	if (pc_softc[unit].seated == FALSE) { 
		printf("pc%d intr(): board not seated\n", unit);
		return;
	}

	while ((int_type = (scb_p->scb_status & SCB_SW_INT)) != 0) {
		pc586ack(unit);
		if (int_type & SCB_SW_FR) {
			pc586rcv(unit);
			watch_dead=0;
		}
		if (int_type & SCB_SW_RNR) {
			pc586_cntrs[unit].rcv.ovw++;
#ifdef	notdef
			printf("pc%d intr(): receiver overrun! begin_fd = %x\n",
				unit, pc_softc[unit].begin_fd);
#endif	/* notdef */
			pc586rustrt(unit);
		}
		if (int_type & SCB_SW_CNA) {
			/*
			 * At present, we don't care about CNA's.  We
			 * believe they are a side effect of XMT.
			 */
		}
		if (int_type & SCB_SW_CX) {
			/*
			 * At present, we only request Interrupt for
			 * XMT.
			 */
			if ((!(cb_p->ac_status & AC_SW_OK)) ||
			    (cb_p->ac_status & (0xfff^TC_SQE))) {
				if (cb_p->ac_status & TC_DEFER) {
					if (xmt_watch) printf("DF");
					pc586_cntrs[unit].xmt.defer++;
				} else if (cb_p->ac_status & (TC_COLLISION|0xf)) {
					if (xmt_watch) printf("%x",cb_p->ac_status & 0xf);
				} else if (xmt_watch) 
					printf("pc%d XMT: %x %x\n",
						unit, cb_p->ac_status, cb_p->ac_command);
			}
			pc586_cntrs[unit].xmt.xmti++;
			pc_softc[unit].tbusy = 0;
			pc586start(unit);
		}
		pc_softc[unit].timer = 5;
	}
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
pc586rcv(
	int	unit)
{
	fd_t	*fd_p;

	for (fd_p = pc_softc[unit].begin_fd; fd_p != (fd_t *)NULL;
	     fd_p = pc_softc[unit].begin_fd) {
		if (fd_p->status == 0xffff || fd_p->rbd_offset == 0xffff) {
			if (pc586hwrst(unit) != TRUE)
				printf("pc%d rcv(): hwrst ffff trouble.\n",
					unit);
			return;
		} else if (fd_p->status & AC_SW_C) {
			fd_t *bfd = (fd_t *)ram_to_ptr(fd_p->link_offset, unit);

			if (fd_p->status == (RFD_DONE|RFD_RSC)) {
					/* lost one */;
#ifdef	notdef
				printf("pc%d RCV: RSC %x\n",
					unit, fd_p->status);
#endif	/* notdef */
				pc586_cntrs[unit].rcv.partial++;
			} else if (!(fd_p->status & RFD_OK))
				printf("pc%d RCV: !OK %x\n",
					unit, fd_p->status);
			else if (fd_p->status & 0xfff)
				printf("pc%d RCV: ERRs %x\n",
					unit, fd_p->status);
			else
				if (!pc586read(unit, fd_p))
					return;
			if (!pc586requeue(unit, fd_p)) {	/* abort on chain error */
				if (pc586hwrst(unit) != TRUE)
					printf("pc%d rcv(): hwrst trouble.\n", unit);
				return;
			}
			pc_softc[unit].begin_fd = bfd;
		} else
			break;
	}
	return;
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
	fd_t	*fd_p)
{
	rbd_t	*l_rbdp;
	rbd_t	*f_rbdp;

#ifndef	REQUEUE_DBG
	if (bad_rbd_chain(fd_p->rbd_offset, unit))
		return 0;
#endif	/* REQUEUE_DBG */
	f_rbdp = (rbd_t *)ram_to_ptr(fd_p->rbd_offset, unit);
	if (f_rbdp != NULL) {
		l_rbdp = f_rbdp;
		while ( (!(l_rbdp->status & RBD_SW_EOF)) && 
			(l_rbdp->next_rbd_offset != 0xffff)) 
		{
			l_rbdp->status = 0;
		   	l_rbdp = (rbd_t *)ram_to_ptr(l_rbdp->next_rbd_offset,
						     unit);
		}
		l_rbdp->next_rbd_offset = PC586NULL;
		l_rbdp->status = 0;
		l_rbdp->size |= AC_CW_EL;
		pc_softc[unit].end_rbd->next_rbd_offset = 
			ptr_to_ram((char *)f_rbdp, unit);
		pc_softc[unit].end_rbd->size &= ~AC_CW_EL;
		pc_softc[unit].end_rbd= l_rbdp;
	}

	fd_p->status = 0;
	fd_p->command = AC_CW_EL;
	fd_p->link_offset = PC586NULL;
	fd_p->rbd_offset = PC586NULL;

	pc_softc[unit].end_fd->link_offset = ptr_to_ram((char *)fd_p, unit);
	pc_softc[unit].end_fd->command = 0;
	pc_softc[unit].end_fd = fd_p;

	return 1;
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
#ifdef	DEBUG
int xmt_debug = 0;
#endif	/* DEBUG */

void
pc586xmt(
	int				unit,
	io_req_t			m)
{
	pc_softc_t			*is = &pc_softc[unit];
	register u_char			*xmtdata_p = (u_char *)(is->sram + OFFSET_TBUF);
	register u_short		*xmtshort_p;
	register struct ether_header	*eh_p = (struct ether_header *)m->io_data;
	volatile scb_t			*scb_p = (volatile scb_t *)(is->sram + OFFSET_SCB);
	volatile ac_t			*cb_p = (volatile ac_t *)(is->sram + OFFSET_CU);
	tbd_t				*tbd_p = (tbd_t *)(is->sram + OFFSET_TBD);
	u_short				tbd = OFFSET_TBD;
	u_short				len, clen = 0;

	cb_p->ac_status = 0;
	cb_p->ac_command = (AC_CW_EL|AC_TRANSMIT|AC_CW_I);
	cb_p->ac_link_offset = PC586NULL;
	cb_p->cmd.transmit.tbd_offset = OFFSET_TBD;

	bcopy16((char *)eh_p->ether_dhost,
		(char *)cb_p->cmd.transmit.dest_addr, ETHER_ADD_SIZE);
	cb_p->cmd.transmit.length = (u_short)(eh_p->ether_type);

	tbd_p->act_count = 0;
	tbd_p->buffer_base = 0;
	tbd_p->buffer_addr = ptr_to_ram((char *)xmtdata_p, unit);
	{ int Rlen, Llen;
	    clen = m->io_count - sizeof(struct ether_header);
	    Rlen = ((int)(m->io_data + sizeof(struct ether_header))) & 1;
	    Llen = (clen - Rlen) & 1;

	    bcopy16((char *)m->io_data + sizeof(struct ether_header) - Rlen,
		    (char *)xmtdata_p,
		    clen + (Rlen + Llen) );
	    xmtdata_p += clen + (Rlen + Llen);
	    tbd_p->act_count = clen;
	    tbd_p->buffer_addr += Rlen;
	}
#ifdef	DEBUG
	if (xmt_debug)
		printf("CLEN = %d\n", clen);
#endif	/* DEBUG */
	if (clen < ETHERMIN) {
		tbd_p->act_count += ETHERMIN - clen;
		for (xmtshort_p = (u_short *)xmtdata_p;
		     clen < ETHERMIN;
		     clen += 2) *xmtshort_p++ = 0;
	}
	tbd_p->act_count |= TBD_SW_EOF;
	tbd_p->next_tbd_offset = PC586NULL;
#ifdef	IF_CNTRS
	clen += sizeof (struct ether_header) + 4 /* crc */;
	pc586_eout[log_2(clen)]++;
	if (clen < 128)  pc586_lout[clen>>3]++;
#endif	/* IF_CNTRS */
#ifdef	DEBUG
	if (xmt_debug) {
		pc586tbd(unit);
		printf("\n");
	}
#endif	/* DEBUG */

	while (scb_p->scb_command) ;
	scb_p->scb_command = SCB_CU_STRT;
	pc586chatt(unit);

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
		pc586send_packet_up(m, eh_p, is);
	iodone(m);
	return;
}

/*
 * pc586bldcu:
 *
 *	This function builds up the command unit structures.  It inits
 *	the scp, iscp, scb, cb, tbd, and tbuf.
 *
 */

void
pc586bldcu(
	int		unit)
{
	char		*sram = pc_softc[unit].sram;
	scp_t		*scp_p = (scp_t *)(sram + OFFSET_SCP);
	iscp_t		*iscp_p = (iscp_t *)(sram + OFFSET_ISCP);
	volatile scb_t	*scb_p = (volatile scb_t *)(sram + OFFSET_SCB);
	volatile ac_t	*cb_p = (volatile ac_t *)(sram + OFFSET_CU);
	tbd_t		*tbd_p = (tbd_t *)(sram + OFFSET_TBD);
	int		i;

	scp_p->scp_sysbus = 0;
	scp_p->scp_iscp = OFFSET_ISCP;
	scp_p->scp_iscp_base = 0;

	iscp_p->iscp_busy = 1;
	iscp_p->iscp_scb_offset = OFFSET_SCB;
	iscp_p->iscp_scb = 0;
	iscp_p->iscp_scb_base = 0;

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

	scb_p->scb_command = SCB_RESET;
	pc586chatt(unit);
	for (i = 1000000; iscp_p->iscp_busy && (i-- > 0); );
	if (!i) printf("pc%d bldcu(): iscp_busy timeout.\n", unit);
	for (i = STATUS_TRIES; i-- > 0; ) {
		if (scb_p->scb_status == (SCB_SW_CX|SCB_SW_CNA)) 
			break;
	}
	if (!i)
		printf("pc%d bldcu(): not ready after reset.\n", unit);
	pc586ack(unit);

	cb_p->ac_status = 0;
	cb_p->ac_command = AC_CW_EL;
	cb_p->ac_link_offset = OFFSET_CU;

	tbd_p->act_count = 0;
	tbd_p->next_tbd_offset = PC586NULL;
	tbd_p->buffer_addr = 0;
	tbd_p->buffer_base = 0;
	return;
}

/*
 * pc586bldru:
 *
 *	This function builds the linear linked lists of fd's and
 *	rbd's.  Based on page 4-32 of 1986 Intel microcom handbook.
 *
 */
char *
pc586bldru(
	int 	unit)
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
	for(i = 0; i < N_RBD; i++, rbd_p = (ru_t *) &(rbd_p->rbuffer[RCVBUFSIZE])) {
		rbd_p->r.status = 0;
		rbd_p->r.buffer_addr = ptr_to_ram((char *)(rbd_p->rbuffer),
					    	   unit);
		rbd_p->r.buffer_base = 0;
		rbd_p->r.size = RCVBUFSIZE;
		if (i != N_RBD-1) {
			rbd_p->r.next_rbd_offset=ptr_to_ram(&(rbd_p->rbuffer[RCVBUFSIZE]),
							    unit);
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
pc586rustrt(
	int 		unit)
{
	volatile scb_t	*scb_p = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	char		*strt;

	if ((scb_p->scb_status & SCB_RUS_READY) == SCB_RUS_READY)
		return;

	strt = pc586bldru(unit);
	scb_p->scb_command = SCB_RU_STRT;
	scb_p->scb_rfa_offset = ptr_to_ram(strt, unit);
	pc586chatt(unit);
	return;
}

/*
 * pc586diag:
 *
 *	This routine does a 586 op-code number 7, and obtains the
 *	diagnose status for the pc586.
 *
 */

boolean_t
pc586diag(
	int 		unit)
{
	volatile scb_t	*scb_p = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	volatile ac_t	*cb_p  = (volatile ac_t *)(pc_softc[unit].sram + OFFSET_CU);
	int		i;

	if (scb_p->scb_status & SCB_SW_INT) {
		printf("pc%d diag(): bad initial state %\n",
			unit, scb_p->scb_status);
		pc586ack(unit);
	}
	cb_p->ac_status	= 0;
	cb_p->ac_command = (AC_DIAGNOSE|AC_CW_EL);
	scb_p->scb_command = SCB_CU_STRT;
	pc586chatt(unit);

	for(i = 0; i < 0xffff; i++)
		if ((cb_p->ac_status & AC_SW_C))
			break;
	if (i == 0xffff || !(cb_p->ac_status & AC_SW_OK)) {
		printf("pc%d: diag failed; status = %x\n",
			unit, cb_p->ac_status);
		return(FALSE);
	}

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

boolean_t
pc586config(
	int 		unit)
{
	volatile scb_t	*scb_p	= (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	volatile ac_t	*cb_p	= (volatile ac_t *)(pc_softc[unit].sram + OFFSET_CU);
	int 		i;


/*
	if ((scb_p->scb_status != SCB_SW_CNA) && (scb_p->scb_status & SCB_SW_INT) ) {
		printf("pc%d config(): unexpected initial state %x\n",
			unit, scb_p->scb_status);
	}
*/
	pc586ack(unit);

	cb_p->ac_status	= 0;
	cb_p->ac_command = (AC_CONFIGURE|AC_CW_EL);

	/*
	 * below is the default board configuration from p2-28 from 586 book
	 */
	cb_p->cmd.configure.fifolim_bytecnt 	= 0x080c;
	cb_p->cmd.configure.addrlen_mode  	= 0x2600;
	cb_p->cmd.configure.linprio_interframe	= 0x6000;
	cb_p->cmd.configure.slot_time      	= 0xf200;
	if (pc_softc[unit].mode & MOD_PROM)
		cb_p->cmd.configure.hardware     = 0x0001;
	else
		cb_p->cmd.configure.hardware     = 0x0000;
	cb_p->cmd.configure.min_frame_len   	= 0x0040;

	scb_p->scb_command = SCB_CU_STRT;
	pc586chatt(unit);

	for(i = 0; i < 0xffff; i++)
		if ((cb_p->ac_status & AC_SW_C))
			break;
	if (i == 0xffff || !(cb_p->ac_status & AC_SW_OK)) {
		printf("pc%d: config-configure failed; status = %x\n",
			unit, cb_p->ac_status);
		return(FALSE);
	}
/*
	if (scb_p->scb_status & SCB_SW_INT) {
		printf("pc%d configure(): bad configure state %x\n",
			unit, scb_p->scb_status);
		pc586ack(unit);
	}
*/
	cb_p->ac_status = 0;
	cb_p->ac_command = (AC_IASETUP|AC_CW_EL);

	bcopy16((char *)pc_softc[unit].ds_addr,
		(char *)cb_p->cmd.iasetup, ETHER_ADD_SIZE);

	scb_p->scb_command = SCB_CU_STRT;
	pc586chatt(unit);

	for (i = 0; i < 0xffff; i++)
		if ((cb_p->ac_status & AC_SW_C))
			break;
	if (i == 0xffff || !(cb_p->ac_status & AC_SW_OK)) {
		printf("pc%d: config-address failed; status = %x\n",
			unit, cb_p->ac_status);
		return(FALSE);
	}
/*
	if ((scb_p->scb_status & SCB_SW_INT) != SCB_SW_CNA) {
		printf("pc%d configure(): unexpected final state %x\n",
			unit, scb_p->scb_status);
	}
*/
	pc586ack(unit);

	return(TRUE);
}

/*
 * pc586ack:
 */

void
pc586ack(
	int		unit)
{
	volatile scb_t	*scb_p = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	int i;

	if (!(scb_p->scb_command = scb_p->scb_status & SCB_SW_INT))
		return;
	CMD(CHANATT, 0x0001, unit);
	for (i = 1000000; scb_p->scb_command && (i-- > 0); );
	if (!i)
		printf("pc%d pc586ack(): board not accepting command.\n", unit);
}

char *
ram_to_ptr(
	u_short		offset,
	int		unit)
{
	if (offset == PC586NULL)
		return(NULL);
	if (offset > 0x3fff) {
		printf("ram_to_ptr(%x, %d)\n", offset, unit);
		panic("range");
		return(NULL);
	}
	return(pc_softc[unit].sram + offset);
}

#ifndef	REQUEUE_DBG
int
bad_rbd_chain(
	u_short		offset,
	int		unit)
{
	rbd_t	*rbdp;
	char	*sram = pc_softc[unit].sram;

	for (;;) {
		if (offset == PC586NULL)
			return 0;
		if (offset > 0x3fff) {
			printf("pc%d: bad_rbd_chain offset = %x\n",
				unit, offset);
			pc586_cntrs[unit].rcv.bad_chain++;
			return 1;
		}

		rbdp = (rbd_t *)(sram + offset);
		offset = rbdp->next_rbd_offset;
	}
}
#endif	/* REQUEUE_DBG */

u_short
ptr_to_ram(
	char	*k_va,
	int 	unit)
{
	return((u_short)(k_va - pc_softc[unit].sram));
}

void
pc586scb(
	int		unit)
{
	volatile scb_t	*scb = (volatile scb_t *)(pc_softc[unit].sram + OFFSET_SCB);
	volatile u_short*cmd = (volatile u_short *)(pc_softc[unit].prom + OFFSET_NORMMODE);
	u_short		 i;

	i = scb->scb_status;
	printf("stat: stat %x, cus %x, rus %x //",
		(i&0xf000)>>12, (i&0x0700)>>8, (i&0x0070)>>4);
	i = scb->scb_command;
	printf(" cmd: ack %x, cuc %x, ruc %x\n",
		(i&0xf000)>>12, (i&0x0700)>>8, (i&0x0070)>>4);

	printf("crc %d[%d], align %d[%d], rsc %d[%d], ovr %d[%d]\n",
		scb->scb_crcerrs, pc586_cntrs[unit].rcv.crc,
		scb->scb_alnerrs, pc586_cntrs[unit].rcv.frame,
		scb->scb_rscerrs, pc586_cntrs[unit].rcv.rscerrs,
		scb->scb_ovrnerrs, pc586_cntrs[unit].rcv.ovrnerrs);

	printf("cbl %x, rfa %x //", scb->scb_cbl_offset, scb->scb_rfa_offset);
	printf(" norm %x, ena %x, xfer %x //",
		cmd[0] & 1, cmd[3] & 1, cmd[4] & 1);
	printf(" atn %x, reset %x, type %x, stat %x\n",
		cmd[1] & 1, cmd[2] & 1, cmd[5] & 1, cmd[6] & 1);
}

void
pc586tbd(
	int		unit)
{
	pc_softc_t	*is = &pc_softc[unit];
	tbd_t		*tbd_p = (tbd_t *)(is->sram + OFFSET_TBD);
	int 		i = 0;
	int		sum = 0;

	do {
		sum += (tbd_p->act_count & ~TBD_SW_EOF);
		printf("%d: addr %x, count %d (%d), next %x, base %x\n",
			i++, tbd_p->buffer_addr,
			(tbd_p->act_count & ~TBD_SW_EOF), sum,
			tbd_p->next_tbd_offset,
			tbd_p->buffer_base);
		if (tbd_p->act_count & TBD_SW_EOF)
			break;
		tbd_p = (tbd_t *)(is->sram + tbd_p->next_tbd_offset);
	} while (1);
}

