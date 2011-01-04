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
/* PPC_HIST
 * Revision 1.1.9.4  1996/08/07  14:22:40  stephen
 * 	More robustness changes + 230400 baud support
 * 	[1996/08/07  14:19:46  stephen]
 *
 * Revision 1.1.9.3  1996/07/29  09:42:03  stephen
 * 	Huge changes for serial DMA to work
 * 	[1996/07/29  09:28:13  stephen]
 * 
 * Revision 1.1.9.2  1996/07/16  10:48:02  stephen
 * 	Initialise and use kernel virtual mappings, not 1-1
 * 	[1996/07/16  10:44:44  stephen]
 * 
 * Revision 1.1.9.1  1996/06/14  08:41:05  emcmanus
 * 	Another big edit.  Removed switch_to_kd() and switch_to_com1().
 * 	Renamed kgdb_scc_putc() to no_spl_scc_putc() and added a channel
 * 	argument to replace the global switch_to_ business.  Removed the
 * 	KGDB_ECHO code.  Added a timeout argument to no_spl_scc_getc()
 * 	to implement kgdb-protocol timeouts (this needs to be fixed so it
 * 	does not depend on the clock speed).
 * 	[1996/06/10  10:38:53  emcmanus]
 * 
 * 	More sweeping changes.  The summary is: added modem-control support
 * 	(still not tested); and generally updated the code because it was
 * 	based on an old driver that has not followed the changes in serial
 * 	driver code from current drivers (notably i386/AT386/com.c).
 * 	[1996/05/07  09:35:59  emcmanus]
 * 
 * Revision 1.1.7.4  1996/05/03  17:27:08  stephen
 * 	Added APPLE_FREE_COPYRIGHT
 * 	[1996/05/03  17:22:28  stephen]
 * 
 * Revision 1.1.7.3  1996/04/27  15:24:15  emcmanus
 * 	Big spring cleaning.  Functionality related to console moved to
 * 	ppc/serial_console.c, and functionality in that file related to
 * 	generic serial I/O moved here.  Bogus swapping of channels 0 and 1
 * 	removed: 0 is now always the console or printer and 1 is kgdb or
 * 	the modem.  Asynchronous entry into the kgdb subsystem on reception
 * 	of characters from the debugger port has been temporarily disabled
 * 	since it caused the system to hang.
 * 	[1996/04/27  15:04:20  emcmanus]
 * 
 * 	The tp->t_ispeed value passed to scc_param() is an actual baud rate
 * 	(e.g., 9600) not a code (e.g., 13).
 * 	[1996/04/25  14:58:17  emcmanus]
 * 
 * Revision 1.1.7.2  1996/04/25  14:44:42  stephen
 * 	#ifdef'd support for video console in preference to serial console
 * 	[1996/04/25  14:44:04  stephen]
 * 
 * Revision 1.1.7.1  1996/04/11  09:12:31  emcmanus
 * 	Copied from mainline.ppc.
 * 	[1996/04/11  08:06:11  emcmanus]
 * 
 * Revision 1.1.5.5  1996/04/10  12:32:47  stephen
 * 	Added warning about apple proprietory information
 * 	[1996/04/10  12:30:27  stephen]
 * 
 * Revision 1.1.5.4  1996/04/03  15:07:04  emcmanus
 * 	Replaced "..." includes with <...>.  Copied relevant bits of
 * 	scc_{get,put}c into kgdb_{get,put}c instead of calling the scc_
 * 	functions, because that incorrectly enabled interrupts when
 * 	the spl had been changed from assembler without updating the
 * 	current_priority variable.
 * 	[1996/04/03  14:47:34  emcmanus]
 * 
 * Revision 1.1.5.3  1996/01/12  16:16:36  stephen
 * 	Cleaned up debug
 * 	[1996/01/12  14:28:29  stephen]
 * 
 * Revision 1.1.5.2  1995/12/19  10:19:19  stephen
 * 	Bugfixes and some cleanup
 * 	[1995/12/19  10:14:08  stephen]
 * 
 * Revision 1.1.5.1  1995/11/23  10:55:42  stephen
 * 	first powerpc checkin to mainline.ppc
 * 	[1995/11/23  10:27:13  stephen]
 * 
 * Revision 1.1.2.5  1995/11/16  21:57:59  stephen
 * 	Mods from Mike for interrupt driven device
 * 	[1995/11/16  21:31:23  stephen]
 * 
 * Revision 1.1.2.4  1995/11/02  15:19:12  stephen
 * 	first implementation of serial console from MB oct-1024
 * 	[1995/10/30  08:25:20  stephen]
 * 
 * Revision 1.2.11.2  1995/01/06  18:52:44  devrcs
 * 	mk6 CR668 - 1.3b26 merge
 * 	* Revision 1.2.4.4  1994/05/06  18:36:42  tmt
 * 	Another attempt at fixing things in scc_scan.
 * 	Fix problem in scc_scan() to detect kdebug being active.
 * 	Remove KDEBUG conditionalization, kdebug_state() function
 * 	always exists for runtime checking.
 * 	Add additional checks for KDEBUG channel in scc_probe() to
 * 	prevent touching this line when we're debugging.
 * 	Add kdebug hooks where appropriate.
 * 	Include <kern/spl.h> to get spl definitions.
 * 	Fix ANSI C violations: trailing tokens on CPP directives.
 * 	No minor()s, no dev_t.
 * 	Flamingo, full_modem and isa_console per-line.
 * 	static/extern cleanups.
 * 	Proper spl typing.
 * 	*End1.3merge
 * 	[1994/11/04  08:24:31  dwm]
 * 
 * Revision 1.2.11.1  1994/09/23  01:10:22  ezf
 * 	change marker to not FREE
 * 	[1994/09/22  21:05:32  ezf]
 * 
 * Revision 1.2.4.2  1993/06/09  02:15:22  gm
 * 	Added to OSF/1 R1.3 from NMK15.0.
 * 	[1993/06/02  20:52:46  jeffc]
 * 
 * Revision 1.2  1993/04/19  15:20:41  devrcs
 * 	Changes from mk78:
 * 	Fixed how the interrupt routine plays with priorities.
 * 	Ask for buffering on all lines.
 * 	[92/05/04  11:15:43  af]
 * 	Fixed for more than just rconsole-ing.  Two bugs: the chip
 * 	needs one char out to generate the xmit-empty (so it really
 * 	is a xmit-done) interrupt, and the t_addr game was not played
 * 	properly.
 * 	Tested both on maxine and the two serial lines kmin has.
 * 	[92/04/14  11:47:26  af]
 * 	Uhmm, lotsa changes.  Basically, got channel B working
 * 	and made it possible to use it as rconsole line.
 * 	Missing modem bitsies only.
 * 	A joint Terri&Sandro feature presentation.
 * 	[92/02/10  17:03:08  af]
 * 	[93/02/04            bruel]
 * 
 * Revision 1.1  1992/09/30  02:00:34  robert
 * 	Initial revision
 * 
 * END_PPC_HIST
 */
/* CMU_HIST */
/*
 * Revision 2.3  91/08/28  11:09:53  jsb
 * 	Fixed scc_scan to actually check the tp->state each time,
 * 	we are not notified when the MI code brutally zeroes it
 * 	on close and so we cannot update our software CARrier.
 * 	[91/08/27  16:18:05  af]
 * 
 * Revision 2.2  91/08/24  11:52:54  af
 * 	Created, from the Zilog specs:
 * 	"Z8530 SCC Serial Communications Controller, Product Specification"
 * 	in the "1983/84 Components Data Book" pp 409-429, September 1983
 * 	Zilog, Campbell, CA 95008
 * 	[91/06/28            af]
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
 *	File: scc_8530_hdw.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	6/91
 *
 *	Hardware-level operations for the SCC Serial Line Driver
 */

#define	NSCC	1	/* Number of serial chips, two ports per chip. */
#if	NSCC > 0

#include <mach_kdb.h>
#include <mach_kgdb.h>

#include <platforms.h>

#include <kgdb/gdb_defs.h>
#include <kgdb/kgdb_defs.h>     /* For kgdb_printf */

#include <kern/spl.h>
#include <mach/std_types.h>
#include <types.h>
#include <sys/syslog.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>
#include <ppc/exception.h>
#include <ppc/POWERMAC/serial_io.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/scc_8530.h>
#include <ppc/POWERMAC/dma_funnel.h>

#if	MACH_KDB
#include <machine/db_machdep.h>
#endif	/* MACH_KDB */

#define	kdebug_state()	(1)
#define	delay(x)	{ volatile int _d_; for (_d_ = 0; _d_ < (10000*x); _d_++) ; }

#define	NSCC_LINE	2	/* 2 ttys per chip */

#if	!MACH_KGDB && !MACH_KDB
#define	SCC_DMA_TRANSFERS	1
#else
/* 
 * Don't deal with DMA because of the way KGDB needs to handle
 * the system and serial ports.
 */
#define	SCC_DMA_TRANSFERS	0
#endif

#undef	SCC_DMA_TRANSFERS
#define	SCC_DMA_TRANSFERS	0
  
struct tty scc_tty[NSCC_LINE];

#define scc_tty_for(chan)	(&scc_tty[chan])
/* #define scc_unit(dev_no)	(dev_no) */
#if 0  /* Old [backwards] device naming */
#define scc_dev_no(chan)	(chan)
#define scc_chan(dev_no)	(dev_no)
#else
#if 1  /* New [compatible] device naming */
#define scc_dev_no(chan) ((chan)^0x01)
#define scc_chan(dev_no) ((dev_no)^0x01)
#else  /* For debugging the new numbering */
#define scc_dev_no(chan) _scc_dev_no(chan, __FUNCTION__, __LINE__, (int)__builtin_return_address(0))
#define scc_chan(dev_no) _scc_chan(dev_no, __FUNCTION__, __LINE__, (int)__builtin_return_address(0))
static int _scc_chan(int dev, const char *func, int line, int ret);
static int 
_scc_chan(int dev, const char *func, int line, int ret)
{
	int chan = dev ^ 0x01;
	printf("%s (%d) - chan(%d) = %d, called from %x\n", func, line, dev, chan, ret);
	return (chan);
}
static int _scc_dev_no(int chan, const char *func, int line, int ret);
static int 
_scc_dev_no(int chan, const char *func, int line, int ret)
{
	int dev_no = chan ^ 0x01;
	printf("%s (%d) - dev_no(%d) = %d, called from %x\n", func, line, chan, dev_no, ret);
	return (dev_no);
}
#endif
#endif

int	serial_initted = 0;

static struct scc_byte {
	unsigned char	reg;
	unsigned char	val;
} scc_init_hw[] = {
	9, 0x80,
	4, 0x44,
	3, 0xC0,
	5, 0xE2,
	2, 0x00,
	10, 0x00,
	11, 0x50,
	12, 0x0A,
	13, 0x00,
	3, 0xC1,
	5, 0xEA,
	14, 0x01,
	15, 0x00,
	0, 0x10,
	0, 0x10,
	1, 0x12,
	9, 0x0A
};

static int	scc_init_hw_count = sizeof(scc_init_hw)/sizeof(scc_init_hw[0]);

enum scc_error {SCC_ERR_NONE, SCC_ERR_PARITY, SCC_ERR_BREAK, SCC_ERR_OVERRUN};


/*
 * BRG formula is:
 *				ClockFrequency (115200 for Power Mac)
 *	BRGconstant = 	---------------------------  -  2
 *			      BaudRate
 */

#define SERIAL_CLOCK_FREQUENCY (115200*2) /* Power Mac value */
#define	convert_baud_rate(rate)	((((SERIAL_CLOCK_FREQUENCY) + (rate)) / (2 * (rate))) - 2)

#define DEFAULT_SPEED 9600
#define DEFAULT_FLAGS (TF_LITOUT|TF_ECHO)

void	scc_attach(struct bus_device *ui );
void	scc_set_modem_control(scc_softc_t scc, boolean_t on);
int	scc_pollc(int unit, boolean_t on);
int	scc_param(struct tty *tp);
int	scc_mctl(struct tty* tp, int bits, int how);
int	scc_cd_scan(void);
void	scc_start(struct tty *tp);
void	scc_intr(int device, struct ppc_saved_state *);
int	scc_simple_tint(dev_t dev, boolean_t all_sent);
void	scc_input(dev_t dev, int c, enum scc_error err);
void	scc_stop(struct tty *tp, int flags);
void	scc_update_modem(struct tty *tp);
void	scc_waitforempty(struct tty *tp);

void	scc_pci_intr(int device, struct ppc_saved_state *);
void	do_scc_receive_chars(scc_regmap_t regs, int chan);
void	do_scc_send_chars(scc_regmap_t regs, int chan);
void	do_scc_status(scc_regmap_t regs, int chan);

struct scc_softc	scc_softc[NSCC];
caddr_t	scc_std[NSCC] = { (caddr_t) 0};

/*
 * Definition of the driver for the auto-configuration program.
 */

struct	bus_device *scc_info[NSCC];

struct	bus_driver scc_driver =
        {scc_probe,
	 0,
	scc_attach,
	 0,
	 scc_std,
	"scc",
	scc_info,
	0,
	0,
	0
};

#define SCC_RR1_ERRS (SCC_RR1_FRAME_ERR|SCC_RR1_RX_OVERRUN|SCC_RR1_PARITY_ERR)
#define SCC_RR3_ALL (SCC_RR3_RX_IP_A|SCC_RR3_TX_IP_A|SCC_RR3_EXT_IP_A|\
                     SCC_RR3_RX_IP_B|SCC_RR3_TX_IP_B|SCC_RR3_EXT_IP_B)

#define DEBUG_SCC
#undef  DEBUG_SCC

#ifdef DEBUG_SCC
static int total_chars, total_ints, total_overruns, total_errors, num_ints, max_chars;
static int chars_received[8];
static int __SCC_STATS = 0;
static int max_in_q = 0;
static int max_out_q = 0;
#endif

#if	SCC_DMA_TRANSFERS
extern struct scc_dma_ops	scc_amic_ops /*, scc_db_ops*/;
#endif

DECL_FUNNEL(, scc_funnel)	/* funnel to serialize the SCC driver */
boolean_t scc_funnel_initted = FALSE;
#define SCC_FUNNEL		scc_funnel
#define SCC_FUNNEL_INITTED	scc_funnel_initted


/*
 * Adapt/Probe/Attach functions
 */
boolean_t	scc_uses_modem_control = FALSE;/* patch this with adb */

/* This is called VERY early on in the init and therefore has to have
 * hardcoded addresses of the serial hardware control registers. The
 * serial line may be needed for console and debugging output before
 * anything else takes place
 */

void
initialize_serial()
{
	int i, chan, bits;
	scc_regmap_t	regs;
	static struct bus_device d;
	DECL_FUNNEL_VARS

	if (!SCC_FUNNEL_INITTED) {
		FUNNEL_INIT(&SCC_FUNNEL, master_processor);
		SCC_FUNNEL_INITTED = TRUE;
	}
	FUNNEL_ENTER(&SCC_FUNNEL);

	if (serial_initted) {
		FUNNEL_EXIT(&SCC_FUNNEL);
		return;
	}

	scc_softc[0].full_modem = TRUE;

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PDM:
	case	POWERMAC_CLASS_POWERBOOK:
		scc_std[0] = (caddr_t) PDM_SCC_BASE_PHYS;
		break;

	case	POWERMAC_CLASS_PCI:
		scc_std[0] = (caddr_t) PCI_SCC_BASE_PHYS;
		break;
	}

	regs = scc_softc[0].regs = (scc_regmap_t)scc_std[0];

	for (chan = 0; chan < NSCC_LINE; chan++) {
		if (chan == 1)
			scc_init_hw[0].val = 0x80;

		for (i = 0; i < scc_init_hw_count; i++) {
			scc_write_reg(regs, chan,
				      scc_init_hw[i].reg, scc_init_hw[i].val);
		}
	}

	/* Call probe so we are ready very early for remote gdb and for serial
	   console output if appropriate.  */
	d.unit = 0;
	if (scc_probe(0, (void *) &d)) {
		for (i = 0; i < NSCC_LINE; i++) {
			scc_softc[0].softr[i].wr5 = SCC_WR5_DTR | SCC_WR5_RTS;
			scc_param(scc_tty_for(i));
	/* Enable SCC interrupts (how many interrupts are to this thing?!?) */
			scc_write_reg(regs,  i,  9, SCC_WR9_NV);

			scc_read_reg_zero(regs, 0, bits);/* Clear the status */
		}
	}

	serial_initted = TRUE;

	FUNNEL_EXIT(&SCC_FUNNEL);
	return;
}


int
scc_probe(caddr_t  xxx, void *param)
{
	struct bus_device *ui = (struct bus_device *) param;
	scc_softc_t     scc;
	register int	val, i;
	register scc_regmap_t	regs;
	spl_t	s;
	DECL_FUNNEL_VARS

	if (!SCC_FUNNEL_INITTED) {
		FUNNEL_INIT(&SCC_FUNNEL, master_processor);
		SCC_FUNNEL_INITTED = TRUE;
	}
	FUNNEL_ENTER(&SCC_FUNNEL);

	/* Readjust the I/O address to handling 
	 * new memory mappings.
	 */

	scc_std[0] = POWERMAC_IO(scc_std[0]);

	regs = (scc_regmap_t)scc_std[0];

	if (regs == (scc_regmap_t) 0) {
		FUNNEL_EXIT(&SCC_FUNNEL);
		return 0;
	}

	scc = &scc_softc[0];
	scc->regs = regs;

	if (scc->probed_once++){
		/* Second time in means called from system */

		switch (powermac_info.class) {
		case	POWERMAC_CLASS_PDM:
#if	SCC_DMA_TRANSFERS
			scc_softc[0].dma_ops = &scc_amic_ops;
			pmac_register_int(PMAC_DEV_SCC, SPLTTY,
					  (void (*)(int, void *))scc_intr);
#else
			pmac_register_int(PMAC_DEV_SCC, SPLTTY,
					  (void (*)(int, void *))scc_pci_intr);
#endif
			break;


		case	POWERMAC_CLASS_POWERBOOK:
			pmac_register_int(PMAC_DEV_SCC, SPLTTY,
					  (void (*)(int, void *))scc_pci_intr);
			break;

		case	POWERMAC_CLASS_PCI:
#if	SCC_DMA_TRANSFERS
			/*scc_softc[0].dma_ops = &scc_db_ops;*/
#endif
			pmac_register_int(PMAC_DEV_SCC_A, SPLTTY,
					  (void (*)(int, void *))scc_pci_intr);
			pmac_register_int(PMAC_DEV_SCC_B, SPLTTY,
					  (void (*)(int, void *))scc_pci_intr);
			break;
		default:
			panic("unsupported class for serial code\n");
		}

		FUNNEL_EXIT(&SCC_FUNNEL);
		return 1;
	}

	s = spltty();

	for (i = 0; i < NSCC_LINE; i++) {
		register struct tty	*tp;
		tp = scc_tty_for(i);
		simple_lock_init(&tp->t_lock, ETAP_IO_TTY);
		tp->t_addr = (char*)(0x80000000L + (i&1));
		/* Set default values.  These will be overridden on
		   open but are needed if the port will be used
		   independently of the Mach interfaces, e.g., for
		   gdb or for a serial console.  */
		tp->t_ispeed = DEFAULT_SPEED;
		tp->t_ospeed = DEFAULT_SPEED;
		tp->t_flags = DEFAULT_FLAGS;
		scc->softr[i].speed = -1;

		/* do min buffering */
		tp->t_state |= TS_MIN;

		tp->t_dev = scc_dev_no(i);
	}

	splx(s);

	FUNNEL_EXIT(&SCC_FUNNEL);
	return 1;
}

boolean_t scc_timer_started = FALSE;

void
scc_attach( register struct bus_device *ui )
{
	extern int tty_inq_size, tty_outq_size;
	int i;
	struct tty *tp;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

#if	SCC_DMA_TRANSFERS
	/* DMA Serial can send a lot... ;-) */
	tty_inq_size = 16384;
	tty_outq_size = 16384;

	for (i = 0; i < NSCC_LINE; i++) {
		if (scc_softc[0].dma_ops) {
			scc_softc[0].dma_ops->scc_dma_init(i);
			scc_softc[0].dma_initted |= (1<<i);
		}
	}
#endif

	if (!scc_timer_started) {
		/* do all of them, before we call scc_scan() */
		/* harmless if done already */
		for (i = 0; i < NSCC_LINE; i++)  {
			tp = scc_tty_for(i);
			ttychars(tp);
			/* hack MEB 1/5/96 */
			tp->t_state |= TS_CARR_ON;
			scc_softc[0].modem[i] = 0;
		}

		scc_timer_started = TRUE;
		scc_cd_scan();
	}

	printf("\n sl0: ( alternate console )\n sl1:");

	FUNNEL_EXIT(&SCC_FUNNEL);
	return;
}

/*
 * Would you like to make a phone call ?
 */

void
scc_set_modem_control(scc, on)
	scc_softc_t      scc;
	boolean_t	on;
{
	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
	scc->full_modem = on;
	/* user should do an scc_param() ifchanged */
	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
}

/*
 * Polled I/O (debugger)
 */

int
scc_pollc(int unit, boolean_t on)
{
	scc_softc_t		scc;

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));

	scc = &scc_softc[unit];
	if (on) {
		scc->polling_mode++;
	} else
		scc->polling_mode--;

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
	return 0;
}

/*
 * Interrupt routine
 */
int scc_intr_count;

void
scc_intr(int device, struct ppc_saved_state *ssp)
{
	int			chan;
	scc_softc_t		scc = &scc_softc[0];
	register scc_regmap_t	regs = scc->regs;
	register int		rr1, rr2, status;
	register int		c;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	scc_intr_count++;

	scc_read_reg_zero(regs, 0, status);/* Clear the status */

	scc_read_reg(regs, SCC_CHANNEL_B, SCC_RR2, rr2);

	rr2 = SCC_RR2_STATUS(rr2);

	/*printf("{INTR %x}", rr2);*/
	if ((rr2 == SCC_RR2_A_XMIT_DONE) || (rr2 == SCC_RR2_B_XMIT_DONE)) {

		chan = (rr2 == SCC_RR2_A_XMIT_DONE) ?
					SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_TX_IP);

		c = scc_simple_tint(scc_dev_no(chan), FALSE);

		if (c == -1) {
			/* no more data for this line */
			c = scc->softr[chan].wr1 & ~SCC_WR1_TX_IE;
			scc_write_reg(regs, chan, SCC_WR1, c);
			scc->softr[chan].wr1 = c;

			c = scc_simple_tint(scc_dev_no(chan), TRUE);
			if (c != -1) {
				/* funny race, scc_start has been called
				   already */
				scc_write_data(regs, chan, c);
			}
		} else {

			scc_write_data(regs, chan, c);
			/* and leave it enabled */
		}
	}

	else if (rr2 == SCC_RR2_A_RECV_DONE || rr2 == SCC_RR2_B_RECV_DONE) {
		int	err = 0;
		chan = (rr2 == SCC_RR2_A_RECV_DONE) ?
					SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0,
			      SCC_RESET_HIGHEST_IUS);

#if MACH_KGDB
		if (chan == KGDB_PORT) {
			/* 11/10/95 MEB
			 * Drop into the debugger.. scc_getc() will
			 * pick up the character
			 */

			call_kgdb_with_ctx(T_INTERRUPT, 0, ssp);
			goto next_intr;
		}
#endif
		scc_read_data(regs, chan, c);
#if	MACH_KDB
		if (console_is_serial() &&
		    c == ('_' & 0x1f)) {
			/* Drop into the debugger */
			kdb_trap(T_INTERRUPT, 0, ssp);
			goto next_intr;
		}
#endif	/* MACH_KDB */


		scc_input(scc_dev_no(chan), c, SCC_ERR_NONE);
	}

	else if ((rr2 == SCC_RR2_A_EXT_STATUS) ||
		 (rr2 == SCC_RR2_B_EXT_STATUS)) {
		chan = (rr2 == SCC_RR2_A_EXT_STATUS) ?
			SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_read_reg(regs, chan, SCC_RR0, status);
		if (status & SCC_RR0_TX_UNDERRUN)
			scc_write_reg(regs, chan, SCC_RR0,
				      SCC_RESET_TXURUN_LATCH);
		if (status & SCC_RR0_BREAK)
			scc_input(scc_dev_no(chan), 0, SCC_ERR_BREAK);

		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_EXT_IP);
		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_HIGHEST_IUS);
		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_ERROR);

		/* Update the modem lines */
		scc_update_modem(scc_tty_for(chan));
	}

	else if ((rr2 == SCC_RR2_A_RECV_SPECIAL) ||
		 (rr2 == SCC_RR2_B_RECV_SPECIAL)) {
		chan = (rr2 == SCC_RR2_A_RECV_SPECIAL) ?
			SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_read_reg(regs, chan, SCC_RR1, rr1);
#if SCC_DMA_TRANSFERS
		if (scc->dma_initted & (chan<<1)) {
			scc->dma_ops->scc_dma_reset_rx(chan);
			scc->dma_ops->scc_dma_start_rx(chan);
		}
#endif
		if (rr1 & (SCC_RR1_PARITY_ERR | SCC_RR1_RX_OVERRUN | SCC_RR1_FRAME_ERR)) {
			enum scc_error err;
			scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_ERROR);
			if (rr1 & SCC_RR1_FRAME_ERR)
				err = SCC_ERR_BREAK;
			else if (rr1 & SCC_RR1_PARITY_ERR)
				err = SCC_ERR_OVERRUN;
			else {
				assert(rr1 & SCC_RR1_RX_OVERRUN);
				err = SCC_ERR_OVERRUN;
			}
#if SCC_DMA_TRANSFERS
			if ((scc->dma_initted & (chan<<1)) == 0)
#endif
				scc_input(scc_dev_no(chan), 0, err);
		}
		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_HIGHEST_IUS);
	}

next_intr:
	FUNNEL_EXIT(&SCC_FUNNEL);
	return;
}

void
do_scc_receive_chars(scc_regmap_t regs, int chan)
{
	int stat0, stat1, c, err;
#ifdef DEBUG_SCC
	int char_count = 0;
#endif
	do {
		scc_read_reg(regs, chan, SCC_RR0, stat0);
		if ((stat0 & SCC_RR0_RX_AVAIL) == 0)
			break;
		scc_read_reg(regs, chan, SCC_RR1, stat1);
		scc_read_data(regs, chan, c);
		if (stat1 & SCC_RR1_ERRS) {
			if (stat1 & SCC_RR1_FRAME_ERR)
				err = SCC_ERR_BREAK;
			if (stat1 & SCC_RR1_PARITY_ERR)
				err = SCC_ERR_PARITY;
			if (stat1 & SCC_RR1_RX_OVERRUN)
				err = SCC_ERR_OVERRUN;
			/* Reset error status */
			scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_ERROR);
#ifdef DEBUG_SCC
			if (err == SCC_ERR_OVERRUN) total_overruns++;
			total_errors++;
#endif
		} else {
			err = SCC_ERR_NONE;
		}
		scc_input(scc_dev_no(chan), c, err);
#ifdef DEBUG_SCC
		char_count++;
#endif
	} while (1);
#ifdef DEBUG_SCC
	if (char_count == 0) return;
	if (char_count > max_chars) max_chars = char_count;
	total_chars += char_count;
	chars_received[char_count-1]++;
	if (++num_ints == __SCC_STATS) {
		int i;
		total_ints += num_ints;
		printf("SCC Ints - total: %d, max: %d, mean: %d, overruns: %d, errors: %d, hist:", 
		       total_chars, max_chars, total_chars / total_ints, total_overruns, total_errors);
		for (i = 0;  i < 8;  i++) {
			printf(" %d", chars_received[i]);
			chars_received[i] = 0;
		}
		printf("\n");
		num_ints = 0;
		max_chars = 0;
	}
#endif
	scc_write_reg(regs, chan, SCC_RR0, SCC_IE_NEXT_CHAR);
}

void
do_scc_send_chars(scc_regmap_t regs, int chan)
{
	int c, stat;
	scc_read_reg(regs, chan, SCC_RR0, stat);
	if ((stat & SCC_RR0_TX_EMPTY) == 0)     
		return;
	c = scc_simple_tint(scc_dev_no(chan), FALSE);
	if (c == -1) {
		/* No more characters or need to stop */
		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_TX_IP);
		c = scc_simple_tint(scc_dev_no(chan), TRUE);
		if (c == -1) {
			return;
		}
		/* Found more to do & transmitter has been re-enabled */
	}
	scc_write_data(regs, chan, c);
}

void
do_scc_status(scc_regmap_t regs, int chan)
{
	int	status;
	printf("SCC status\n");
	scc_read_reg(regs, chan, SCC_RR0, status);
	if (status & SCC_RR0_TX_UNDERRUN)
		scc_write_reg(regs, chan, SCC_RR0,
			      SCC_RESET_TXURUN_LATCH);
	if (status & SCC_RR0_BREAK)
		scc_input(scc_dev_no(chan), 0, SCC_ERR_BREAK);
	
	scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_EXT_IP);
	scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_HIGHEST_IUS);
	scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_ERROR);
	scc_write_reg(regs, chan, SCC_RR0, SCC_IE_NEXT_CHAR);

	/* Update the modem lines */
	scc_update_modem(scc_tty_for(chan));
}

void
scc_pci_intr(int device, struct ppc_saved_state *ssp)
{
	int		chan;
	scc_softc_t	scc = &scc_softc[0];
	scc_regmap_t	regs = scc->regs;
	int		status, stat0;
	int		total = 0;
	DECL_FUNNEL_VARS
		
	FUNNEL_ENTER(&SCC_FUNNEL);
	scc_intr_count++;
	do {
		/* Read interrupt state */
		scc_read_reg(regs, SCC_CHANNEL_A, SCC_RR3, status);
		if ((status & SCC_RR3_ALL) == 0) {
			scc_read_reg(regs, SCC_CHANNEL_A, SCC_RR0, stat0);  /* Clear int status */
			break;
		}
		if (status & SCC_RR3_RX_IP_A) {
#if MACH_KGDB
			if (SCC_CHANNEL_A == KGDB_PORT) {
				call_kgdb_with_ctx(T_INTERRUPT, 0, ssp);
			} else
#endif	       
				do_scc_receive_chars(regs, SCC_CHANNEL_A);
		}
		if (status & SCC_RR3_TX_IP_A) {
			do_scc_send_chars(regs, SCC_CHANNEL_A);
		}
		if (status & SCC_RR3_EXT_IP_A) {
			do_scc_status(regs, SCC_CHANNEL_A);
		}
		if (status & SCC_RR3_RX_IP_B) {
#if MACH_KGDB
			if (SCC_CHANNEL_B == KGDB_PORT) {
				call_kgdb_with_ctx(T_INTERRUPT, 0, ssp);
			} else
#endif	       
				do_scc_receive_chars(regs, SCC_CHANNEL_B);
		}
		if (status & SCC_RR3_TX_IP_B) {
			do_scc_send_chars(regs, SCC_CHANNEL_B);
		}
		if (status & SCC_RR3_EXT_IP_B) {
			do_scc_status(regs, SCC_CHANNEL_B);
		}
		if (++total > 5) {
			printf("Stuck SCC interrupts? Status: %x\n", status);
			break;
		}
	} while (1);
	FUNNEL_EXIT(&SCC_FUNNEL);
	return;
}


/*
 * Start output on a line
 */

void
scc_start(tp)
	struct tty *tp;
{
	spl_t			s;
	int			cc;
	scc_regmap_t	regs;
	int			chan = scc_chan(tp->t_dev), temp;
	struct scc_softreg	*sr = &scc_softc[0].softr[chan];
	scc_softc_t		scc = &scc_softc[0];
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	s = spltty();

	/* Start up the DMA channel if it was paused */
	if ((tp->t_state & TS_TTSTOP) == 0 && sr->dma_flags & SCC_FLAGS_DMA_PAUSED) {
		/*printf("{DMA RESUME}");*/
		scc->dma_ops->scc_dma_continue_tx(chan);
		splx(s);
		FUNNEL_EXIT(&SCC_FUNNEL);
		return;
	}

	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) 
		goto out;


#if	SCC_DMA_TRANSFERS
	if (scc_softc[0].dma_initted & (1<<chan)) {
 	 	/* Don't worry about low water marks...
	  	 * The DMA operation should be able to pull off most
	  	 * if not all of the TTY output queue
	  	*/
	
		tt_write_wakeup(tp);

		if (tp->t_outq.c_cc <= 0) 
			goto out;

		tp->t_state |= TS_BUSY;

		scc_softc[0].dma_ops->scc_dma_start_tx(chan, tp);
	} else 
#endif
	{
		cc = tp->t_outq.c_cc;
		if (cc <= tp->t_lowater) {
			tt_write_wakeup(tp);
		}
		if (cc <= 0)
			goto out;
		tp->t_state |= TS_BUSY;
	
		regs = scc_softc[0].regs;
		sr   = &scc_softc[0].softr[chan];
	
		scc_read_reg(regs, chan, SCC_RR15, temp);
		temp |= SCC_WR15_TX_UNDERRUN_IE;
		scc_write_reg(regs, chan, SCC_WR15, temp);
	
		temp = sr->wr1 | SCC_WR1_TX_IE;
		scc_write_reg(regs, chan, SCC_WR1, temp);
		sr->wr1 = temp;
	
		/* but we need a first char out or no cookie */
		scc_read_reg(regs, chan, SCC_RR0, temp);
		if (temp & SCC_RR0_TX_EMPTY)
		{
			register char	c;
	
			c = getc(&tp->t_outq);
			scc_write_data(regs, chan, c);
		}
	}
out:
	splx(s);
	FUNNEL_EXIT(&SCC_FUNNEL);
}

#define	u_min(a,b)	((a) < (b) ? (a) : (b))

/*
 * Get a char from a specific SCC line
 * [this is only used for console&screen purposes]
 * must be splhigh since it may be called from another routine under spl
 */

int
scc_getc(int unit, int line, boolean_t wait, boolean_t raw)
{
	register scc_regmap_t	regs;
	unsigned char   c, value;
	int             rcvalue, from_line;
	spl_t		s = splhigh();
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	regs = scc_softc[0].regs;

	/*
	 * wait till something available
	 *
	 */
again:
	rcvalue = 0;
	while (1) {
		scc_read_reg_zero(regs, line, value);

		if (value & SCC_RR0_RX_AVAIL)
			break;

		if (!wait) {
			splx(s);
			FUNNEL_EXIT(&SCC_FUNNEL);
			return -1;
		}
	}

	/*
	 * if nothing found return -1
	 */

	scc_read_reg(regs, line, SCC_RR1, value);
	scc_read_data(regs, line, c);

#if	MACH_KDB
	if (console_is_serial() &&
	    c == ('_' & 0x1f)) {
		/* Drop into the debugger */
		Debugger("Serial Line Request");
		scc_write_reg(regs, line, SCC_RR0, SCC_RESET_HIGHEST_IUS);
		if (wait) {
			goto again;
		}
		splx(s);
		FUNNEL_EXIT(&SCC_FUNNEL);
		return -1;
	}
#endif	/* MACH_KDB */

	/*
	 * bad chars not ok
	 */
	if (value&(SCC_RR1_PARITY_ERR | SCC_RR1_RX_OVERRUN | SCC_RR1_FRAME_ERR)) {
		scc_write_reg(regs, line, SCC_RR0, SCC_RESET_ERROR);

		if (wait) {
			scc_write_reg(regs, line, SCC_RR0, SCC_RESET_HIGHEST_IUS);
			goto again;
		}
	}

	scc_write_reg(regs, line, SCC_RR0, SCC_RESET_HIGHEST_IUS);
	splx(s);

	FUNNEL_EXIT(&SCC_FUNNEL);
	return c;
}

/*
 * Put a char on a specific SCC line
 * use splhigh since we might be doing a printf in high spl'd code
 */

int
scc_putc(int unit, int line, int c)
{
	scc_regmap_t	regs;
	spl_t            s = splhigh();
	unsigned char	 value;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	regs = scc_softc[0].regs;

	do {
		scc_read_reg(regs, line, SCC_RR0, value);
		if (value & SCC_RR0_TX_EMPTY)
			break;
		delay(1);
	} while (1);

	scc_write_data(regs, line, c);
/* wait for it to swallow the char ? */

	do {
		scc_read_reg(regs, line, SCC_RR0, value);
		if (value & SCC_RR0_TX_EMPTY)
			break;
	} while (1);
	scc_write_reg(regs, line, SCC_RR0, SCC_RESET_HIGHEST_IUS);
	splx(s);

	FUNNEL_EXIT(&SCC_FUNNEL);
	return 0;
}

int
scc_param(struct tty *tp)
{
	scc_regmap_t	regs;
	unsigned char	value;
	unsigned short	speed_value;
	int		bits, chan;
	spl_t		s;
	struct scc_softreg	*sr;
	scc_softc_t	scc;

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));

	chan = scc_chan(tp->t_dev);
	scc = &scc_softc[0];
	regs = scc->regs;

	sr = &scc->softr[chan];

	/* Do a quick check to see if the hardware needs to change */
	if ((sr->flags & (TF_ODDP|TF_EVENP)) == (tp->t_flags & (TF_ODDP|TF_EVENP))
	    && sr->speed == tp->t_ispeed) {
		assert(FUNNEL_IN_USE(&SCC_FUNNEL));
		return 0;
	}

	sr->flags = tp->t_flags;
	sr->speed = tp->t_ispeed;

	s = spltty();

	if (tp->t_ispeed == 0) {
		sr->wr5 &= ~SCC_WR5_DTR;
		scc_write_reg(regs,  chan, 5, sr->wr5);
		splx(s);

		assert(FUNNEL_IN_USE(&SCC_FUNNEL));
		return 0;
	}
	

#if	SCC_DMA_TRANSFERS
	if (scc->dma_initted & (1<<chan)) 
		scc->dma_ops->scc_dma_reset_rx(chan);
#endif

	value = SCC_WR4_1_STOP;

	/* 
	 * For 115K the clocking divide changes to 64.. to 230K will
	 * start at the normal clock divide 16.
	 *
	 * However, both speeds will pull from a different clocking
	 * source
	 */

	if (tp->t_ispeed == 115200)
		value |= SCC_WR4_CLK_x32;
	else	
		value |= SCC_WR4_CLK_x16 ;

	/* .. and parity */
	if ((tp->t_flags & (TF_ODDP | TF_EVENP)) == TF_EVENP)
		value |= (SCC_WR4_EVEN_PARITY |  SCC_WR4_PARITY_ENABLE);
	else if ((tp->t_flags & (TF_ODDP | TF_EVENP)) == TF_ODDP)
		value |= SCC_WR4_PARITY_ENABLE;

	/* set it now, remember it must be first after reset */
	sr->wr4 = value;

	/* Program Parity, and Stop bits */
	scc_write_reg(regs,  chan, 4, sr->wr4);

	/* Setup for 8 bits */
	scc_write_reg(regs,  chan, 3, SCC_WR3_RX_8_BITS);

	// Set DTR, RTS, and transmitter bits/character.
	sr->wr5 = SCC_WR5_TX_8_BITS | SCC_WR5_RTS | SCC_WR5_DTR;

	scc_write_reg(regs,  chan, 5, sr->wr5);
	
	scc_write_reg(regs, chan, 14, 0);	/* Disable baud rate */

	/* Setup baud rate 57.6Kbps, 115K, 230K should all yeild
	 * a converted baud rate of zero
	 */
	speed_value = convert_baud_rate(tp->t_ispeed);

	if (speed_value == 0xffff)
		speed_value = 0;

	scc_set_timing_base(regs, chan, speed_value);
	
	if (tp->t_ispeed == 115200 || tp->t_ispeed == 230400) {
		/* Special case here.. change the clock source*/
		scc_write_reg(regs, chan, 11, 0);
		/* Baud rate generator is disabled.. */
	} else {
		scc_write_reg(regs, chan, 11, SCC_WR11_RCLK_BAUDR|SCC_WR11_XTLK_BAUDR);
		/* Enable the baud rate generator */
		scc_write_reg(regs,  chan, 14, SCC_WR14_BAUDR_ENABLE);
	}


	scc_write_reg(regs,  chan,  3, SCC_WR3_RX_8_BITS|SCC_WR3_RX_ENABLE);


	sr->wr1 = SCC_WR1_RXI_FIRST_CHAR | SCC_WR1_EXT_IE;
	scc_write_reg(regs,  chan,  1, sr->wr1);
       	scc_write_reg(regs,  chan, 15, SCC_WR15_ENABLE_ESCC);
	scc_write_reg(regs,  chan,  7, SCC_WR7P_RX_FIFO);
	scc_write_reg(regs,  chan,  0, SCC_IE_NEXT_CHAR);


	/* Clear out any pending external or status interrupts */
	scc_write_reg(regs,  chan,  0, SCC_RESET_EXT_IP);
	scc_write_reg(regs,  chan,  0, SCC_RESET_EXT_IP);
	//scc_write_reg(regs,  chan,  0, SCC_RESET_ERROR);

	/* Enable SCC interrupts (how many interrupts are to this thing?!?) */
	scc_write_reg(regs,  chan,  9, SCC_WR9_MASTER_IE|SCC_WR9_NV);

	scc_read_reg_zero(regs, 0, bits);/* Clear the status */

#if	SCC_DMA_TRANSFERS
	if (scc->dma_initted & (1<<chan))  {
		scc->dma_ops->scc_dma_start_rx(chan);
		scc->dma_ops->scc_dma_setup_8530(chan);
	} else
#endif
	{
		sr->wr1 = SCC_WR1_RXI_FIRST_CHAR | SCC_WR1_EXT_IE;
		scc_write_reg(regs, chan, 1, sr->wr1);
		scc_write_reg(regs, chan, 0, SCC_IE_NEXT_CHAR);
	}

	sr->wr5 |= SCC_WR5_TX_ENABLE;
	scc_write_reg(regs,  chan,  5, sr->wr5);

	splx(s);

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
	return 0;

}

void
scc_update_modem(struct tty *tp)
{
	scc_softc_t scc = &scc_softc[0];
	int chan = scc_chan(tp->t_dev);
	scc_regmap_t	regs = scc->regs;
	unsigned char rr0, old_modem;

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));

	old_modem = scc->modem[chan];
	scc->modem[chan] &= ~(TM_CTS|TM_CAR|TM_RNG|TM_DSR);
	scc->modem[chan] |= TM_DSR|TM_CTS;

	scc_read_reg_zero(regs, chan, rr0);

	if (rr0 & SCC_RR0_DCD) {
		scc->modem[chan] |= TM_CAR;
		scc->dcd_timer[chan] = 0;
		if ((old_modem & TM_CAR) == 0) {
			/*printf("{DTR-ON %x/%x}", rr0, old_modem);*/
			/*
			 * The trick here is that
			 * the device_open does not hang
			 * waiting for DCD, but a message
			 * is sent to the process 
			 */

			if ((tp->t_state & (TS_ISOPEN|TS_WOPEN))
			    && tp->t_flags & TF_OUT_OF_BAND) {
				/*printf("{NOTIFY}");*/
				tp->t_outofband = TOOB_CARRIER;
				tp->t_outofbandarg = TRUE;
				tty_queue_completion(&tp->t_delayed_read);
			}
		}
	} else if (old_modem & TM_CAR) {
		if (tp->t_state & (TS_ISOPEN|TS_WOPEN)) {
			if (++scc->dcd_timer[chan] >= DCD_TIMEOUT) {
				/*printf("{DTR-OFF %x/%x}", rr0, old_modem);*/
				if (tp->t_flags & TF_OUT_OF_BAND) {
					tp->t_outofband = TOOB_CARRIER;
					tp->t_outofbandarg = FALSE;
					tty_queue_completion(&tp->t_delayed_read);
				} else
					ttymodem(tp, FALSE);
			}
		}
	}

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
}

/*
 * Modem control functions
 */
int
scc_mctl(struct tty* tty, int bits, int how)
{
	register dev_t dev = tty->t_dev;
	int sccline;
	register int tcr, msr, brk, n_tcr, n_brk;
	int b;
	scc_softc_t      scc;
	int wr5;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	sccline = scc_chan(dev);

	if (bits == TM_HUP) {	/* close line (internal) */
	    bits = TM_DTR | TM_RTS;
	    how = DMBIC;
	}

	scc = &scc_softc[0];
 	wr5 = scc->softr[sccline].wr5;

	if (how == DMGET) {
	    scc_update_modem(tty);
	    FUNNEL_EXIT(&SCC_FUNNEL);
	    return scc->modem[sccline];
	}

	switch (how) {
	case DMSET:
	    b = bits; break;
	case DMBIS:
	    b = scc->modem[sccline] | bits; break;
	case DMBIC:
	    b = scc->modem[sccline] & ~bits; break;
	default:
	    FUNNEL_EXIT(&SCC_FUNNEL);
	    return 0;
	}

	if (scc->modem[sccline] == b) {
	    FUNNEL_EXIT(&SCC_FUNNEL);
	    return b;
	}

	scc->modem[sccline] = b;

	if (bits & TM_BRK) {
	    ttydrain(tty);
	    scc_waitforempty(tty);
	}

	wr5 &= ~(SCC_WR5_SEND_BREAK|SCC_WR5_DTR);

	if (b & TM_BRK)
		wr5 |= SCC_WR5_SEND_BREAK;

	if (b & TM_DTR)
		wr5 |= SCC_WR5_DTR;

	wr5 |= SCC_WR5_RTS;

	scc_write_reg(scc->regs, sccline, 5, wr5);
	scc->softr[sccline].wr5 = wr5;

	b = scc->modem[sccline];

	FUNNEL_EXIT(&SCC_FUNNEL);
	return b;
}

/*
 * Periodically look at the CD signals:
 * they do generate interrupts but we
 * must fake them on channel A.  We might
 * also fake them on channel B.
 */

int
scc_cd_scan(void)
{
	spl_t s = spltty();
	scc_softc_t	scc;
	int		j;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);
	scc = &scc_softc[0];
	for (j = 0; j < NSCC_LINE; j++) {
		if (scc_tty[j].t_state & (TS_ISOPEN|TS_WOPEN))
			scc_update_modem(&scc_tty[j]);
	}
	splx(s);

	timeout((timeout_fcn_t)scc_cd_scan, (void *)0, hz/4);

	FUNNEL_EXIT(&SCC_FUNNEL);
	return 0;
}

#if MACH_KGDB
void no_spl_scc_putc(int chan, char c)
{
	register scc_regmap_t	regs;
	register unsigned char	value;

	if (!serial_initted)
		initialize_serial();

	regs = scc_softc[0].regs;

	do {
		scc_read_reg(regs, chan, SCC_RR0, value);
		if (value & SCC_RR0_TX_EMPTY)
			break;
		delay(100);
	} while (1);

	scc_write_data(regs, chan, c);
/* wait for it to swallow the char ? */

	do {
		scc_read_reg(regs, chan, SCC_RR0, value);
		if (value & SCC_RR0_TX_EMPTY)
			break;
	} while (1);
	scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_HIGHEST_IUS);

	if (c == '\n')
		no_spl_scc_putc(chan, '\r');
}

#define	SCC_KGDB_BUFFER	15

int no_spl_scc_getc(int chan, boolean_t timeout)
{
	register scc_regmap_t	regs;
	unsigned char   c, value, i;
	int             rcvalue, from_line;
	int	timeremaining = timeout ? 10000000 : 0;	/* XXX */
	static unsigned char	buffer[2][SCC_KGDB_BUFFER];
	static int		bufcnt[2], bufidx[2];

	/* This should be rewritten to produce a constant timeout
	   regardless of the processor speed.  */

	if (!serial_initted)
		initialize_serial();

	regs = scc_softc[0].regs;

get_char:
	if (bufcnt[chan]) {
		bufcnt[chan] --;
		return ((unsigned int) buffer[chan][bufidx[chan]++]);
	}

	/*
	 * wait till something available
	 *
	 */
	bufidx[chan] = 0;

	for (i = 0; i < SCC_KGDB_BUFFER; i++) {
		rcvalue = 0;

		while (1) {
			scc_read_reg_zero(regs, chan, value);
			if (value & SCC_RR0_RX_AVAIL)
				break;
			if (timeremaining && !--timeremaining) {
				if (i)
					goto get_char;
				else
					return KGDB_GETC_TIMEOUT;
			}
		}

		scc_read_reg(regs, chan, SCC_RR1, value);
		scc_read_data(regs, chan, c);
		buffer[chan][bufcnt[chan]] = c;
		bufcnt[chan]++;

		/*
	 	 * bad chars not ok
	 	 */


		if (value&(SCC_RR1_PARITY_ERR | SCC_RR1_RX_OVERRUN | SCC_RR1_FRAME_ERR)) {
			scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_ERROR);

			scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_HIGHEST_IUS);
			bufcnt[chan] = 0;
			return KGDB_GETC_BAD_CHAR;
		}

			
		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_HIGHEST_IUS);
		
		for (timeremaining = 0; timeremaining < 1000; timeremaining++) {
			scc_read_reg_zero(regs, chan, value);

			if (value & SCC_RR0_RX_AVAIL)
				continue;
		}

		if (timeout == FALSE)
			break;

	}


	goto get_char;
}
#endif	/* MACH_KGDB */

/*
 * Open routine
 */

io_return_t
scc_open(
	dev_t		dev,
	dev_mode_t	flag,
	io_req_t	ior)
{
	register struct tty	*tp;
	spl_t			s;
	scc_softc_t		scc;
	int			chan;
	int			forcedcarrier;
	io_return_t		result;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	if (dev >= NSCC * NSCC_LINE) {
		FUNNEL_EXIT(&SCC_FUNNEL);
		return D_NO_SUCH_DEVICE;
	}

	chan = scc_chan(dev);
	tp = scc_tty_for(chan);
	scc = &scc_softc[0];

	/* But was it there at probe time */
	if (tp->t_addr == 0) {
		FUNNEL_EXIT(&SCC_FUNNEL);
		return D_NO_SUCH_DEVICE;
	}

	s = spltty();
	simple_lock(&tp->t_lock);

	if (!(tp->t_state & (TS_ISOPEN|TS_WOPEN))) {
		tp->t_dev	= dev;
		tp->t_start	= scc_start;
		tp->t_stop	= scc_stop;
		tp->t_mctl	= scc_mctl;
		tp->t_getstat	= scc_get_status;
		tp->t_setstat	= scc_set_status;
		scc->modem[chan] = 0;	/* No assumptions on things.. */
		if (tp->t_ispeed == 0) {
			tp->t_ispeed = DEFAULT_SPEED;
			tp->t_ospeed = DEFAULT_SPEED;
			tp->t_flags = DEFAULT_FLAGS;
		}

		scc->softr[chan].speed = -1;	/* Force reset */
		scc->softr[chan].wr5 |= SCC_WR5_DTR;
		scc_param(tp);
	}

	scc_update_modem(tp);

	tp->t_state |= TS_CARR_ON;	/* Always.. */

	simple_unlock(&tp->t_lock);
	splx(s);
	result = char_open(dev, tp, flag, ior);

	if (tp->t_flags & CRTSCTS) {
		simple_lock(&tp->t_lock);
		if (!(scc->modem[chan] & TM_CTS)) 
			tp->t_state |= TS_TTSTOP;
		simple_unlock(&tp->t_lock);
	}

	FUNNEL_EXIT(&SCC_FUNNEL);
	return result;
}

/*
 * Close routine
 */
void
scc_close(
	dev_t	dev)
{
	register struct tty	*tp;
	spl_t			s;
	scc_softc_t		scc = &scc_softc[0];
	int			chan = scc_chan(dev);
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	tp = scc_tty_for(chan);

	s = spltty();
	simple_lock(&tp->t_lock);

	ttstart(tp);
	ttydrain(tp);
	scc_waitforempty(tp);

	/* Disable Receiver.. */
	scc_write_reg(scc->regs, chan, SCC_WR3, 0);
#if SCC_DMA_TRANSFERS
	if (scc->dma_initted & (chan <<1))
		scc->dma_ops->scc_dma_reset_rx(chan);
#endif

	ttyclose(tp);
	if (tp->t_state & TS_HUPCLS) {
		scc->softr[chan].wr5 &= ~(SCC_WR5_DTR);
		scc_write_reg(scc->regs, chan, SCC_WR5, scc->softr[chan].wr5);
		scc->modem[chan] &= ~(TM_DTR|TM_RTS);
	}


	tp->t_state &= ~TS_ISOPEN;

#ifdef DEBUG_SCC
	{
		int i;
		total_ints += num_ints;
		printf("SCC Ints - total: %d, max: %d, mean: %d, overruns: %d, errors: %d, max q in: %d, out: %d\n",
		       total_chars, max_chars, total_chars / total_ints, total_overruns, total_errors, max_in_q, max_out_q);
		printf("           hist:");
		for (i = 0;  i < 8;  i++) {
			printf(" %d", chars_received[i]);
			chars_received[i] = 0;
		}
		printf("\n");
		num_ints = 0;
		max_chars = 0;
	}
#endif
	simple_unlock(&tp->t_lock);
	splx(s);

	FUNNEL_EXIT(&SCC_FUNNEL);
}

io_return_t
scc_read(
	dev_t		dev,
	io_req_t	ior)
{
	io_return_t	retval;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);
	retval = char_read(scc_tty_for(scc_chan(dev)), ior);
	FUNNEL_EXIT(&SCC_FUNNEL);

	return retval;
}

io_return_t
scc_write(
	dev_t		dev,
	io_req_t	ior)
{
	io_return_t	retval;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);
	retval = char_write(scc_tty_for(scc_chan(dev)), ior);
#ifdef DEBUG_SCC
	if (scc_tty[dev].t_outq.c_cc > max_out_q) max_out_q = scc_tty[dev].t_outq.c_cc;
#endif
	FUNNEL_EXIT(&SCC_FUNNEL);

	return retval;
}

/*
 * Stop output on a line.
 */
void
scc_stop(
	struct tty	*tp,
	int		flags)
{
	int chan;
	scc_softc_t scc;
	struct scc_softreg *sr;
	spl_t s;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	chan = scc_chan(tp->t_dev);
	scc = &scc_softc[0];
	sr = &scc->softr[chan];

	s = spltty();

	if (tp->t_state & TS_BUSY) {
		if (sr->dma_flags & SCC_FLAGS_DMA_TX_BUSY) {
			/*printf("{DMA OFF}");*/
			scc->dma_ops->scc_dma_pause_tx(chan);
		} else if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}

	splx(s);
	/*printf("{STOP %x}", flags);*/

	FUNNEL_EXIT(&SCC_FUNNEL);
}

/*
 * Abnormal close
 */
boolean_t
scc_portdeath(
	dev_t		dev,
	ipc_port_t	port)
{
	boolean_t	retval;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);
	retval = tty_portdeath(scc_tty_for(scc_chan(dev)), port);
	FUNNEL_EXIT(&SCC_FUNNEL);

	return retval;
}

/*
 * Get/Set status rotuines
 */
io_return_t
scc_get_status(
	dev_t			dev,
	dev_flavor_t		flavor,
	dev_status_t		data,
	mach_msg_type_number_t	*status_count)
{
	register struct tty *tp;
	io_return_t	retval;
	int		chan = scc_chan(dev);
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	tp = scc_tty_for(chan);

	switch (flavor) {
	case TTY_MODEM:
		scc_update_modem(tp);
		*data = scc_softc[0].modem[chan];
		*status_count = 1;
		FUNNEL_EXIT(&SCC_FUNNEL);
		return (D_SUCCESS);
	default:
		retval = tty_get_status(tp, flavor, data, status_count);
		FUNNEL_EXIT(&SCC_FUNNEL);
		return retval;
	}
}

io_return_t
scc_set_status(
	dev_t			dev,
	dev_flavor_t		flavor,
	dev_status_t		data,
	mach_msg_type_number_t	status_count)
{
	register struct tty *tp;
	spl_t s;
	io_return_t result = D_SUCCESS;
	scc_softc_t scc;
	int chan;
	DECL_FUNNEL_VARS

	FUNNEL_ENTER(&SCC_FUNNEL);

	scc = &scc_softc[0];
	chan = scc_chan(dev);

	tp = scc_tty_for(scc_chan(dev));

	s = spltty();
	simple_lock(&tp->t_lock);

	switch (flavor) {
	case TTY_MODEM:
		(void) scc_mctl(tp, *data, DMSET);
		break;

	case TTY_NMODEM:
		break;

	case TTY_SET_BREAK:
		(void) scc_mctl(tp, TM_BRK, DMBIS);
		break;

	case TTY_CLEAR_BREAK:
		(void) scc_mctl(tp, TM_BRK, DMBIC);
		break;

	default:
		simple_unlock(&tp->t_lock);
		splx(s);
		result = tty_set_status(tp, flavor, data, status_count);
		s = spltty();
		simple_lock(&tp->t_lock);
		if (result == D_SUCCESS &&
		    (flavor== TTY_STATUS_NEW || flavor == TTY_STATUS_COMPAT)) {
			result = scc_param(tp);

			if (tp->t_flags & CRTSCTS) {
				if (scc->modem[chan] & TM_CTS) {
					tp->t_state &= ~TS_TTSTOP;
					ttstart(tp);
				} else
					tp->t_state |= TS_TTSTOP;
			}
		}
		break;
	}

	simple_unlock(&tp->t_lock);
	splx(s);

	FUNNEL_EXIT(&SCC_FUNNEL);
	return result;
}

void
scc_waitforempty(struct tty *tp)
{
	int chan = scc_chan(tp->t_dev);
	scc_softc_t scc = &scc_softc[0];
	int rr0;

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));

	while (1) {
		scc_read_reg(scc->regs, chan, SCC_RR0, rr0);
		if (rr0 & SCC_RR0_TX_EMPTY)
			break;
		assert_wait(0, TRUE);
		thread_set_timeout(1);
		simple_unlock(&tp->t_lock);
		thread_block((void (*)(void)) 0);
		reset_timeout_check(&current_thread()->timer);
		simple_lock(&tp->t_lock);
	}

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
}

/*
 * Send along a character on a tty.  If we were waiting for
 * this char to complete the open procedure do so; check
 * for errors; if all is well proceed to ttyinput().
 */

void
scc_input(dev_t dev, int c, enum scc_error err)
{
	register struct tty *tp;

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));

	tp = scc_tty_for(scc_chan(dev));

	if ((tp->t_state & TS_ISOPEN) == 0) {
		if (tp->t_state & TS_INIT)
			tt_open_wakeup(tp);
		assert(FUNNEL_IN_USE(&SCC_FUNNEL));
		return;
	}
	switch (err) {
	case SCC_ERR_NONE:
		ttyinput(c, tp);
#ifdef DEBUG_SCC
		if (tp->t_inq.c_cc > max_in_q) max_in_q = tp->t_inq.c_cc;
#endif
		break;
	case SCC_ERR_OVERRUN:
		/*log(LOG_WARNING, "sl%d: silo overflow\n", dev);*/
		/* Currently the Mach interface doesn't define an out-of-band
		   event that we could use to signal this error to the user
		   task that has this device open.  */
		break;
	case SCC_ERR_PARITY:
		ttyinputbadparity(c, tp);
		break;
	case SCC_ERR_BREAK:
		ttybreak(c, tp);
		break;
	}

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
}

/*
 * Transmission of a character is complete.
 * Return the next character or -1 if none.
 */
int
scc_simple_tint(dev_t dev, boolean_t all_sent)
{
	register struct tty *tp;
	int	retval;

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));

	tp = scc_tty_for(scc_chan(dev));
	if ((tp->t_addr == 0) || /* not probed --> stray */
	    (tp->t_state & TS_TTSTOP)) {
		assert(FUNNEL_IN_USE(&SCC_FUNNEL));
		return -1;
	}

	if (all_sent) {
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;

		scc_start(tp);
	}

	if (tp->t_outq.c_cc == 0 || (tp->t_state&TS_BUSY)==0) {
		assert(FUNNEL_IN_USE(&SCC_FUNNEL));
		return -1;
	}

	retval = getc(&tp->t_outq);

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
	return retval;
}

void
powermac_scc_set_datum(scc_regmap_t regs, unsigned int offset, unsigned char value)
{
	volatile unsigned char *address = (unsigned char *) regs + offset;
  
	assert(FUNNEL_IN_USE(&SCC_FUNNEL));

	*address = value;
	eieio();

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
}
  
unsigned char
powermac_scc_get_datum(scc_regmap_t regs, unsigned int offset)
{
	volatile unsigned char *address = (unsigned char *) regs + offset;
	unsigned char	value;
  
	assert(FUNNEL_IN_USE(&SCC_FUNNEL));

	value = *address; eieio();
	return value;

	assert(FUNNEL_IN_USE(&SCC_FUNNEL));
}
#endif	/* NSCC > 0 */
