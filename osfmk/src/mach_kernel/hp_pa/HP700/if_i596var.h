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
 * Support for Intel 82596 device (run in 586 compatibility mode)
 * on the HP 700 series machines.
 *
 * Derived from the Mach Intel 82586 driver for the PC.
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * Copyright (c) 1991,1992 The University of Utah and
 * the Center for Software Science (CSS).
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF
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
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: if_i596var.h 3.9 93/08/12$
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

#include <xkmachkernel.h>
#include <device/if_hdr.h>
#include <device/driver_lock.h>
#include <hp_pa/HP700/if_i596reg.h>	/* chip/board specific defines	*/

#define	STATUS_TRIES	15000
#define	ETHER_ADD_SIZE	6	/* size of a MAC address 		*/
#define ETHER_PCK_SIZE	1500	/* maximum size of an ethernet packet 	*/
#define	MULTICAST_SIZE	8	/* size of multicast filter array */

/*
 * Make sure everything is cache aligned (32 bytes).
 * The original offsets apparently had to fit in 16k hence the determination
 * of N_FD, N_RBD, N_TBD.  Do we have the same constraints?  I don't know,
 * but we stay within the 16k to be safe.
 *
 *	RU:   23 at 22 (32) bytes			[ 0x0000 - 0x02DF ]
 *	RBD:  23 at 10 (32) + 23 at 540 (544) bytes	[ 0x02E0 - 0x369F ]
 *	CU:   256 bytes					[ 0x36A0 - 0x379F ]
 *	TBD:  13 at 8 (32) bytes			[ 0x37A0 - 0x393F ]
 *	TBUF: 1594 (1600) bytes				[ 0x3940 - 0x3F7F ]
 *	SCB:  16 (64) bytes [1]				[ 0x3F80 - 0x3FBF ]
 *	ISCP: 8 (32) bytes				[ 0x3FC0 - 0x3FDF ]
 *	SCP:  10 (32) bytes				[ 0x3FE0 - 0x3FFF ]
 *
 * [1] 64 instead of 32 as the status/command fields must be split
 *     across two cache lines.
 */
#define OFFSET_SCP      0x3FE0
#define OFFSET_ISCP     0x3FC0
#define OFFSET_SCB      (0x3F80+0x1E)	/* stat/cmd straddle c-line boundary */
#define OFFSET_RU	0x0000
#define OFFSET_RBD	0x02E0
#define OFFSET_CU	0x36A0

#define OFFSET_TBD      0x37A0
#define OFFSET_TBUF     0x3940

#define N_FD              23
#define N_RBD             23
#define N_TBD		  13

#define purgescbcmd(scb)	pcacheline(HP700_SID_KERNEL, (vm_offset_t)(scb)+2)
#define purgescbstat(scb) 	pcacheline(HP700_SID_KERNEL, (vm_offset_t)(scb)-30)
#define purgescball(scb)	purgescbstat(scb), purgescbcmd(scb)

#define flushscbcmd(scb)	fcacheline(HP700_SID_KERNEL, (vm_offset_t)(scb)+2)
#define flushscbstat(scb) 	fcacheline(HP700_SID_KERNEL, (vm_offset_t)(scb)-30)
#define flushscball(scb)	flushscbstat(scb), flushscbcmd(scb)

#define RCVBUFSIZE	540
#define DL_DEAD         0xffff

#define CMD_0           0     
#define CMD_1           0xffff

#define PC586NULL	0xffff			/* pc586 NULL for lists	*/

#define	DSF_LOCK	1
#define DSF_RUNNING	2

#define MOD_ENAL 1
#define MOD_PROM 2

/*
 * Driver (not board) specific defines and structures:
 */

/*
 * 64 addresses are sufficient since the 596 is using an imperfect
 * filter mechanism based on 64 bits.
 */
#define PC_MCASTLEN     64
typedef struct  {
	unsigned short	ac_status;
	unsigned short	ac_command;
	unsigned short	ac_link_offset;
	/* multicast-specific data follows */
	unsigned short	mc_count;
	unsigned char	mc_addr[PC_MCASTLEN*ETHER_ADD_SIZE];
} multicast_t;

typedef	struct	{
	rbd_t	r;
	char	rbuffer[RCVBUFSIZE];
	char	rbd_pad[4];
} ru_t;

typedef struct { 
	struct	ifnet	ds_if;		/* generic interface header */
	unsigned char	ds_addr[6];	/* Ethernet hardware address */
	int	flags;
        int     seated;
#ifdef WATCHDOG
        int     timer;
#endif
        int     open;
        fd_t    *begin_fd;
	volatile fd_t    *end_fd;
	rbd_t   *end_rbd;
	volatile char *sram;
    	unsigned	dev_open;	/* number of opened devices */
	int     tbusy;
	struct	core_lan *hwaddr;
	short	mode;
	multicast_t sc_mcast;
	queue_head_t mcast_queue;	/* multicast hardware addr list */
	unsigned char mcast_hash[MULTICAST_SIZE];
	driver_lock_decl(,lock) /* driver lock */
#if XKMACHKERNEL
	unsigned int	 xk_up;		/* x-kernel up and running */
	char		*xk_body;	/* x-kernel body message */
	unsigned int	 xk_olen;	/* x-kernel output len */
	unsigned int	 xk_wlen;	/* x-kernel working output len */
	unsigned short	 xk_ltype;	/* x-kernel traffic lowest type */
	unsigned short	 xk_htype;	/* x-kernel traffic highest type */
#endif /* XKMACHKERNEL */
} pc_softc_t;

#define	bcopy16(s,d,l)	bcopy((char *)(s), (char *)(d), (int)(l))

/*
 * Prototypes.
 */
extern void		pc586attach(struct hp_device *);
extern int		pc586probe(struct hp_device *);
extern int		pc586init(int);
extern void		pc586timeout(void *);
extern int		pc586diag(int);
extern int		pc586config(int);
extern boolean_t	pc586bldcu(int);
extern char		*pc586bldru(int);
extern int		pc586hwrst(int);
extern void		pc586start(int);
extern int		pc586read(int, volatile fd_t *);
extern void 		pc586debug_xmt(int, char *, int);
extern io_return_t	pc586open(dev_t, dev_mode_t, io_req_t);
extern io_return_t	pc586devinfo(dev_t, dev_flavor_t, char *);
extern void		pc586close(dev_t);
