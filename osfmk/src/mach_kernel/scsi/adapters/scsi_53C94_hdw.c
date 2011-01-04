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

/* INTEL_HIST */
/* Revision 1.12.8.1  1994/08/04  16:15:19  richardg
 *  Reviewer: Jerry Coffman
 *  Risk: Medium
 *  Benefit or PTS #: 10171
 *  Testing: ran Tensor's tapewrt program simultaneously against both LUNs.
 *  Module(s): asc_go, asc_end
 *
 * Revision 1.12  1994/04/20  22:18:04  richardg
 *  Reviewer: Jerrie Coffman
 *  Risk: low
 *  Benefit or PTS #: added for 3480 Tape Library support - LOAD DISPLAY.
 *  Testing: tape EAT's and costom code to exercise the ioctl
 *  Module(s): asc_go()
 *
 * Revision 1.11  1994/04/20  21:53:21  richardg
 *  Reviewer: Jerrie Coffman
 *  Risk: low
 *  Benefit or PTS #: 9022
 *  Testing: tape and RAID EAT's
 *  Module(s): all of the asserts except on in scsi_53C94_hdw.c now have an
 * 		if(scsi_debug){} wrapper.  These asserts occur if a target
 * 		grabs the bus then releases it without transferring any
 * 		data.  This doesn't hurt anything, and is not prohibited by
 * 		the SCSI spec.
 *
/* Revision 1.10  1993/09/16  18:12:37  richardg
 * modified the memory allocation to evenly divide available MIO memory between existing scsi devices
 *
 * Revision 1.9  1993/09/15  21:39:48  jerrie
 * Added code for the SCSI restore pointers message.  Simply restore the
 * transient state script to become the active script (it was saved by the
 * disconnect code) and return a value from asc_disconnect() to indicate that
 * target is not actually disconnecting.
 *
 * Revision 1.8  1993/09/15  21:11:38  jerrie
 * Added code to detect and configure the NCR 53CF94/96-2 Fast SCSI Controller.
 *
 * Revision 1.7  1993/06/30  22:55:19  dleslie
 * Adding copyright notices required by legal folks
 *
 * Revision 1.6  1993/06/21  18:06:24  jerrie
 * Modified SCRIPT_MATCH macro to include an additional check for the
 * diconnect bit in the interrrupt register.  If a timeout occurs while
 * another target is reconnecting the csr register incorrectly shows
 * data in phase while all other registers appear correct.  Checking
 * the interrupt register for this case correctly causes the error
 * handler to be invoked.  Advance script following a disconnect.
 * Added assertion in asc_reconnect to check the target id.  Renamed
 * the tr array to asc_tr to overcome a link error.
 *
 * Revision 1.5  1993/05/12  17:19:32  jerrie
 * Fix for bugs 4824 and 4826.  Added code to asc_attempt_selection to
 * workaround a problem with the Emulex SCSI controller.  The SCSI command
 * is now loaded into the chip FIFO by the processor and sent to the SCSI bus
 * without DMA.  Fix for bug 5099 to allow the LUN field in a the identify
 * message to be used.
 *
 * Revision 1.4  1993/04/27  20:49:09  dleslie
 * Copy of R1.0 sources onto main trunk
 *
 * Revision 1.2.6.2  1993/04/22  18:54:41  dleslie
 * First R1_0 release
 */
/* INTEL_ENDHIST */

/* CMU_HIST */
/*
 * Revision 2.12.2.2  92/03/28  10:14:49  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:30:47  jeffreyh]
 * 
 * Revision 2.14  92/02/23  22:43:48  elf
 * 	Reflected changes in interface functions: probe_target,
 * 	go and restart-callup routines do not take a scsi_softc
 * 	descriptor any more.
 * 	[92/02/22  19:35:12  af]
 * 
 * Revision 2.13  92/01/03  20:41:58  dbg
 * 	Removed playing with FIFO in asc_reconnect(), see comment within.
 * 	This makes the case of multiple drives work again.
 * 	[91/12/26  11:14:02  af]
 * 
 * Revision 2.12  91/11/12  11:17:15  rvb
 * 	To cope with kmin, call dma complete function at end of
 * 	msg-in phase (disconnect sequence).  Also check no leftover
 * 	bytes in fifo.
 * 	[91/10/30  13:41:44  af]
 * 
 * Revision 2.11  91/08/24  12:25:37  af
 * 	Always call the end_xfer routine, might have to stop DMA engine.
 * 	In synch negotiation, actually send out the command we are
 * 	supposed to rather than blindly test_unit_ready.
 * 	[91/08/22  11:08:23  af]
 * 
 * 	Moved padding of regmap here.  Moved dma-like operations elsewhere,
 * 	now we have a new case in which a true DMA chip uses the 53C94.
 * 	Actually, on 3mins the two types of dma might/do coexist.
 * 	[91/08/02  04:17:22  af]
 * 
 * Revision 2.10  91/06/25  20:55:34  rpd
 * 	Tweaks to make gcc happy.
 * 
 * Revision 2.9  91/06/19  16:28:59  rvb
 * 	mips->DECSTATION; vax->VAXSTATION
 * 	[91/06/12  14:02:35  rvb]
 * 
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/07            rvb]
 * 
 * Revision 2.8  91/05/14  17:28:52  mrt
 * 	Correcting copyright
 * 
 * Revision 2.7  91/05/13  06:05:04  af
 * 	Added support for dynamically powering on/off devices.
 * 	Added faster, extremely pipelined WRITEs.  And some more.
 * 
 * 	Added disconnect-reconnect capability.
 * 	Also: avoid thread_block()ing, cleaned up and compacted scripts,
 * 	added much more debugging support including nice traces and stats,
 * 	support disks larger than 1Gb, schedule scsi bus properly by
 * 	queueing up request from multiple targets in FIFO order,
 * 	do request_sense on bad status, uncovered some more misteries,
 * 	made the no_synchronous option work by negotiating a zero value
 * 	for the sync offset. Pretty close to a full rewrite.
 * 	[91/05/12  16:15:14  af]
 * 
 * Revision 2.6.1.1  91/03/29  16:39:37  af
 * 	Added disconnect-reconnect capability.
 * 	Also: avoid thread_block()ing, cleaned up and compacted scripts,
 * 	added much more debugging support including nice traces and stats,
 * 	support disks larger than 1Gb, schedule scsi bus properly by
 * 	queueing up request from multiple targets in FIFO order,
 * 	do request_sense on bad status, uncovered some more misteries,
 * 	made the no_synchronous option work by negotiating a zero value
 * 	for the sync offset. Pretty close to a full rewrite.
 * 
 * Revision 2.6  91/02/14  14:35:31  mrt
 * 	In interrupt routine, drop priority as now required.
 * 	Also, sanity check for spurious interrupts.
 * 	Some more debugging tools.
 * 	[91/02/12  12:46:38  af]
 * 
 * Revision 2.5  91/02/05  17:45:07  mrt
 * 	Added author notices
 * 	[91/02/04  11:18:43  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:17:20  mrt]
 * 
 * Revision 2.4  91/01/08  15:48:24  rpd
 * 	Added continuation argument to thread_block.
 * 	[90/12/27            rpd]
 * 
 * Revision 2.3  90/12/05  23:34:48  af
 * 	Recovered from pmax merge.. and from the destruction of a disk.
 * 	[90/12/03  23:40:40  af]
 * 
 * Revision 2.1.1.1  90/11/01  03:39:09  af
 * 	Created, from the DEC specs:
 * 	"PMAZ-AA TURBOchannel SCSI Module Functional Specification"
 * 	Workstation Systems Engineering, Palo Alto, CA. Aug 27, 1990.
 * 	And from the NCR data sheets
 * 	"NCR 53C94, 53C95, 53C96 Advances SCSI Controller"
 * 	[90/09/03            af]
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
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS AS-IS
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
 *	File: scsi_53C94_hdw.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	9/90
 *
 *	Bottom layer of the SCSI driver: chip-dependent functions
 *
 *	This file contains the code that is specific to the NCR 53C94
 *	SCSI chip (Host Bus Adapter in SCSI parlance): probing, start
 *	operation, and interrupt routine.
 */

/*
 * This layer works based on small simple 'scripts' that are installed
 * at the start of the command and drive the chip to completion.
 * The idea comes from the specs of the NCR 53C700 'script' processor.
 *
 * There are various reasons for this, mainly
 * - Performance: identify the common (successful) path, and follow it;
 *   at interrupt time no code is needed to find the current status
 * - Code size: it should be easy to compact common operations
 * - Adaptability: the code skeleton should adapt to different chips without
 *   terrible complications.
 * - Error handling: and it is easy to modify the actions performed
 *   by the scripts to cope with strange but well identified sequences
 *
 */

			/***************************************/
#ifndef	PARAGON860	/* osc1_3b26 code EGREGIOUS HACK XXXXX */
			/***************************************/

	/* * Revision 1.2.4.5  1994/05/13  20:16:55  */

#include <asc.h>
#if	NASC > 0
#include <platforms.h>

#ifdef	DECSTATION
typedef unsigned char	asc_register_t;
#define	PAD(n)		char n[3];
#define	mb()
#ifdef	MACH_KERNEL
#define	HAS_MAPPED_SCSI
#endif
#define	ASC_PROBE_DYNAMICALLY FALSE	/* established custom */
#endif

#ifdef FLAMINGO
typedef	unsigned int	asc_register_t;
#define PAD(n)		int n;	/* sparse ! */
#define	mb() wbflush()		/* memory barrier */
#define	ASC_PROBE_DYNAMICALLY TRUE
#define DEBUG	1
#define TRACE	1
#endif

#ifdef	PARAGON860
typedef unsigned char	asc_register_t;
#define	check_memory(a,b)	0
#define	PAD(n)		char n
#define wbflush()
#define	mb()
#define	ASC_PROBE_DYNAMICALLY FALSE
#endif	/* PARAGON860 */

#include <kern/spl.h>
#include <mach/std_types.h>
#include <sys/types.h>
#include <chips/busses.h>
#include <scsi/compat_30.h>

#include <scsi/scsi.h>
#include <scsi/scsi2.h>

#include <scsi/adapters/scsi_53C94.h>
#include <scsi/scsi_defs.h>
#include <scsi/adapters/scsi_dma.h>

#ifdef	PAD
typedef struct {
	volatile asc_register_t	asc_tc_lsb;	/* rw: Transfer Counter LSB */
	PAD(pad0)
	volatile asc_register_t	asc_tc_msb;	/* rw: Transfer Counter MSB */
	PAD(pad1)
	volatile asc_register_t	asc_fifo;	/* rw: FIFO top */
	PAD(pad2)
	volatile asc_register_t	asc_cmd;	/* rw: Command */
	PAD(pad3)
	volatile asc_register_t	asc_csr;	/* r:  Status */
/*#define		asc_dbus_id asc_csr	/* w: Destination Bus ID */
	PAD(pad4)
	volatile asc_register_t	asc_intr;	/* r:  Interrupt */
/*#define		asc_sel_timo asc_intr	/* w: (re)select timeout */
	PAD(pad5)
	volatile asc_register_t	asc_ss;		/* r:  Sequence Step */
/*#define		asc_syn_p asc_ss	/* w: synchronous period */
	PAD(pad6)
	volatile asc_register_t	asc_flags;	/* r:  FIFO flags + seq step */
/*#define		asc_syn_o asc_flags	/* w: synchronous offset */
	PAD(pad7)
	volatile asc_register_t	asc_cnfg1;	/* rw: Configuration 1 */
	PAD(pad8)
	volatile asc_register_t	asc_ccf;	/* w:  Clock Conv. Factor */
	PAD(pad9)
	volatile asc_register_t	asc_test;	/* w:  Test Mode */
	PAD(pad10)
	volatile asc_register_t	asc_cnfg2;	/* rw: Configuration 2 */
	PAD(pad11)
	volatile asc_register_t	asc_cnfg3;	/* rw: Configuration 3 */
	PAD(pad12)
	volatile asc_register_t	asc_rfb;	/* w:  Reserve FIFO byte */
	PAD(pad13)
} asc_padded_regmap_t;

#else	/* !PAD */

typedef asc_regmap_t	asc_padded_regmap_t;

#endif	/* !PAD */

#define	get_reg(r,x)	((unsigned char)((r)->x))

#define	fifo_count(r)	((r)->asc_flags & ASC_FLAGS_FIFO_CNT)
#define	get_fifo(r)	get_reg(r,asc_fifo)

boolean_t asc_probe_dynamically = ASC_PROBE_DYNAMICALLY;

/* Matching on the condition value */
#define	ANY				0xff
#define	SCRIPT_MATCH(csr,ir,value)	((SCSI_PHASE(csr)==(value)) || \
					 (((value)==ANY) && \
					  ((ir)&(ASC_INT_DISC|ASC_INT_FC))))

/* When no command is needed */
#define	SCRIPT_END	-1

/*
 * A script has a three parts: a pre-condition, an action, and
 * an optional command to the chip.  The first triggers error
 * handling if not satisfied and in our case it is a match
 * of the expected and actual scsi-bus phases.
 * The action part is just a function pointer, and the
 * command is what the 53c90 should be told to do at the end
 * of the action processing.  This command is only issued and the
 * script proceeds if the action routine returns TRUE.
 * See asc_intr() for how and where this is all done.
 */

typedef struct script {
	unsigned char	condition;	/* expected state at interrupt */
	unsigned char	command;	/* command to the chip */
	unsigned short	flags;		/* unused padding */
	boolean_t	(*action)();	/* extra operations */
} *script_t;

/*
 * State descriptor for this layer.  There is one such structure
 * per (enabled) SCSI-53c90 interface
 */
struct asc_softc {
	struct watchdog_t	wd;
	asc_padded_regmap_t	*regs;		/* 53c90 registers */

	scsi_dma_ops_t	*dma_ops;	/* DMA operations and state */
	opaque_t	dma_state;

	script_t	script;		/* what should happen next */
	boolean_t	(*error_handler)();/* what if something is wrong */
	int		in_count;	/* amnt we expect to receive */
	int		out_count;	/* amnt we are going to ship */

	volatile char	state;
#define	ASC_STATE_BUSY		0x01	/* selecting or currently connected */
#define ASC_STATE_TARGET	0x04	/* currently selected as target */
#define ASC_STATE_COLLISION	0x08	/* lost selection attempt */
#define ASC_STATE_DMA_IN	0x10	/* tgt --> initiator xfer */
#define	ASC_STATE_SPEC_DMA	0x20	/* special, 8 byte threshold dma */
#define	ASC_STATE_FAS_CNTL	0x40	/* Emulex FAS SCSI controller */
#define	ASC_STATE_FSC_CNTL	0x80	/* NCR FSC SCSI controller */

	unsigned char	ntargets;	/* how many alive on this scsibus */
	unsigned char	done;
	unsigned char	extra_count;	/* sleazy trick to spare an interrupt */

	scsi_softc_t	*sc;		/* HBA-indep info */
	target_info_t	*active_target;	/* the current one */

	target_info_t	*next_target;	/* trying to seize bus */
	queue_head_t	waiting_targets;/* other targets competing for bus */

	unsigned char	ss_was;		/* districate powered on/off devices */
	unsigned char	cmd_was;

	unsigned char	timeout;	/* cache a couple numbers */
	unsigned char	ccf;
	unsigned char	clk;

} asc_softc_data[NASC];

typedef struct asc_softc *asc_softc_t;

asc_softc_t	asc_softc[NASC];

/*
 * Function Prototypes / Forward References:
 */

extern boolean_t	asc_end(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern boolean_t	asc_clean_fifo(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern boolean_t	asc_get_status(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern boolean_t	asc_put_status(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern boolean_t	asc_dma_in(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern int		asc_dma_in_r(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern boolean_t	asc_dma_out(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern boolean_t	asc_dma_out_r(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern boolean_t	asc_dosynch(
				register asc_softc_t	asc,
				register unsigned char	csr,
				register unsigned char	ir);

extern boolean_t	asc_msg_in(
				register asc_softc_t	asc,
				register unsigned char	csr,
				register unsigned char	ir);

extern boolean_t	asc_disconnected(
				register asc_softc_t	asc,
				register unsigned char	csr,
				register unsigned char	ir);

extern boolean_t	asc_reconnect(
				register asc_softc_t	asc,
				register unsigned char	csr,
				register unsigned char	ir);

extern int		asc_err_generic(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern int		asc_err_disconn(
				register asc_softc_t	asc,
				register unsigned char	csr,
				register unsigned char	ir);

extern int		asc_reset_scsibus(
				register asc_softc_t	asc);

extern boolean_t	asc_probe_target(
				target_info_t 		*tgt,
				io_req_t		ior);

extern int		asc_to_scsi_period(
				int			a,
				int			clk);

extern int		scsi_period_to_asc(
				int			p,
				int			clk);

extern int		asc_go(
				target_info_t		*tgt,
				int			cmd_count,
				int			in_count,
				boolean_t		cmd_only);

extern int		asc_probe(
				vm_offset_t		reg,
				struct bus_ctlr		*ui);

extern int		asc_intr(
				int			unit,
				spl_t			spllevel);

extern int		asc_set_dmaops(
				unsigned int		unit,
				scsi_dma_ops_t		*dmaops);

extern int		asc_state(
				asc_padded_regmap_t	*regs);

extern int		asc_target_state(
				target_info_t		*tgt);

extern int		asc_all_targets(
				int			unit);

extern int		asc_script_state(
				int			unit);

extern int		asc_print_log(
				int			skip);

extern int		asc_print_stat(void);

extern int		asc_reset(
				asc_softc_t		asc,
				int			quick,
				int			special_dma);

extern int		asc_attempt_selection(
				asc_softc_t		asc);

extern void		asc_bus_reset(
				register asc_softc_t	asc);

extern int		asc_target_intr(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

extern boolean_t	asc_selected(
				register asc_softc_t	asc,
				register unsigned int	csr,
				register unsigned int	ir);

extern boolean_t	asc_release_bus(
				register asc_softc_t	asc);

extern int		asc_odsynch(
				register asc_softc_t	asc,
				target_info_t		*initiator);

extern boolean_t	asc_disconnect(
				register asc_softc_t	asc,
				register unsigned char	csr,
				register unsigned char	ir);

extern int		asc_err_to_status(
				register asc_softc_t	asc,
				int			csr,
				int			ir);

static int		asc_wait(
				asc_padded_regmap_t	*regs,
				int			until);
static int		LOG(
				int			e,
				char			*f);

/*
 * Synch xfer parameters, and timing conversions
 */
int	asc_min_period = 5;	/* in CLKS/BYTE, e.g. 1 CLK = 40nsecs @25 Mhz */
int	asc_max_offset = 15;	/* pure number */

int
asc_to_scsi_period(
	int			a,
	int			clk)
{
	/* Note: the SCSI unit is 4ns, hence
		A_P * 1,000,000,000
		-------------------  =  S_P
		    C_Mhz  * 4
	 */
	return a * (250 / clk);
		
}

int
scsi_period_to_asc(
	int			p,
	int			clk)
{
	register int 	ret;

	ret = (p * clk) / 250;
	if (ret < asc_min_period)
		return asc_min_period;
	if ((asc_to_scsi_period(ret,clk)) < p)
		return ret + 1;
	return ret;
}

#define readback(a)	{register int foo; foo = a; mb();}

#define	u_min(a,b)	(((a) < (b)) ? (a) : (b))

/*
 * Definition of the controller for the auto-configuration program.
 */

int	asc_intr();

caddr_t	asc_std[NASC] = { 0 };
struct	bus_device *asc_dinfo[NASC*8];
struct	bus_ctlr *asc_minfo[NASC];
struct	bus_driver asc_driver = 
        { asc_probe, scsi_slave, scsi_attach, asc_go, asc_std, "rz", asc_dinfo,
	  "asc", asc_minfo, BUS_INTR_B4_PROBE};


int	asc_clock_speed_in_mhz[NASC] = {25,25,25,25};	/* original 3max */

int
asc_set_dmaops(
	unsigned int		unit,
	scsi_dma_ops_t		*dmaops)
{
	if (unit < NASC)
		asc_std[unit] = (caddr_t)dmaops;
}

/*
 * Scripts
 */
struct script
asc_script_data_in[] = {	/* started with SEL & DMA */
	{SCSI_PHASE_DATAI, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_in},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_data_out[] = {	/* started with SEL & DMA */
	{SCSI_PHASE_DATAO, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_out},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_try_synch[] = {
	{SCSI_PHASE_MSG_OUT, ASC_CMD_I_COMPLETE,0, asc_dosynch},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_simple_cmd[] = {
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_disconnect[] = {
	{ANY, ASC_CMD_ENABLE_SEL, 0, asc_disconnected},
/**/	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_reconnect}
},

asc_script_restart_data_in[] = { /* starts after disconnect */
	{SCSI_PHASE_DATAI, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_in_r},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_restart_data_out[] = { /* starts after disconnect */
	{SCSI_PHASE_DATAO, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_out_r},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

#if	documentation
/*
 * This is what might happen during a read
 * that disconnects
 */
asc_script_data_in_wd[] = { /* started with SEL & DMA & allow disconnect */
	{SCSI_PHASE_MSG_IN, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_msg_in},
	{ANY, ASC_CMD_ENABLE_SEL, 0, asc_disconnected},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_reconnect},
	{SCSI_PHASE_DATAI, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_in},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},
#endif

/*
 * Target mode scripts
 */
asc_script_t_data_in[] = {
	{SCSI_PHASE_CMD, ASC_CMD_RCV_DATA|ASC_CMD_DMA, 0, asc_dma_in_r},
	{SCSI_PHASE_DATAO, ASC_CMD_TERM, 0, asc_put_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_t_data_out[] = {
	{SCSI_PHASE_CMD, ASC_CMD_SND_DATA|ASC_CMD_DMA, 0, asc_dma_out_r},
	{SCSI_PHASE_DATAI, ASC_CMD_TERM, 0, asc_put_status},
	{ANY, SCRIPT_END, 0, asc_end}
};


#ifdef	DEBUG

#define	PRINT(x)	if (scsi_debug) printf x

int
asc_state(
	asc_padded_regmap_t	*regs)
{
	register unsigned char ff,csr,ir,d0,d1,cmd;

	if (regs == 0) {
		if (asc_softc[0])
			regs = asc_softc[0]->regs;
		else
			regs = (asc_padded_regmap_t*)0xbf400000;
	}
	ff = get_reg(regs,asc_flags);
	csr = get_reg(regs,asc_csr);
/*	ir = get_reg(regs,asc_intr);	nope, clears interrupt */
	d0 = get_reg(regs,asc_tc_lsb);
	d1 = get_reg(regs,asc_tc_msb);
	cmd = get_reg(regs,asc_cmd);
	printf("dma %x ff %x csr %x cmd %x\n",
		(d1 << 8) | d0, ff, csr, cmd);
	return 0;
}

int
asc_target_state(
	target_info_t		*tgt)
{
	if (tgt == 0)
		tgt = asc_softc[0]->active_target;
	if (tgt == 0)
		return 0;
	printf("@x%lx: fl %x dma %lx+%x cmd %x@%lx id %x per %x off %x ior %lx ret %x\n",
		tgt,
		tgt->flags, tgt->dma_ptr, tgt->transient_state.dma_offset, tgt->cur_cmd,
		tgt->cmd_ptr, tgt->target_id,
		tgt->sync_period, tgt->sync_offset,
		tgt->ior, tgt->done);
	if (tgt->flags & TGT_DISCONNECTED){
		script_t	spt;

		spt = tgt->transient_state.script;
		printf(": %x %x ", spt->condition, spt->command);
		printf("\n");
	}

	return 0;
}

int
asc_all_targets(
	int			unit)
{
	int i;
	target_info_t	*tgt;
	for (i = 0; i < 8; i++) {
		tgt = asc_softc[unit]->sc->target[i];
		if (tgt)
			asc_target_state(tgt);
	}
}

int
asc_script_state(
	int			unit)
{
	script_t	spt = asc_softc[unit]->script;

	if (spt == 0) return 0;
	printf(": %x %x ", spt->condition, spt->command);
	return 0;
}

#define TRMAX 200
int tr[TRMAX+3];
int trpt, trpthi;
#define	TR(x)	tr[trpt++] = x
#define TRWRAP	trpthi = trpt; trpt = 0;
#define TRCHECK	if (trpt > TRMAX) {TRWRAP}


#ifdef TRACE

#define LOGSIZE 256
int asc_logpt;
char asc_log[LOGSIZE];

#define MAXLOG_VALUE	0x22
struct {
	char *name;
	unsigned int count;
} logtbl[MAXLOG_VALUE];

static int
LOG(
	int		e,
	char		*f)
{
	asc_log[asc_logpt++] = (e);
	if (asc_logpt == LOGSIZE) asc_logpt = 0;
	if ((e) < MAXLOG_VALUE) {
		logtbl[(e)].name = (f);
		logtbl[(e)].count++;
	}
}

int
asc_print_log(
	int			skip)
{
	register int i, j;
	register unsigned char c;

	for (i = 0, j = asc_logpt; i < LOGSIZE; i++) {
		c = asc_log[j];
		if (++j == LOGSIZE) j = 0;
		if (skip-- > 0)
			continue;
		if (c < MAXLOG_VALUE)
			printf(" %s", logtbl[c].name);
		else
			printf("-x%x", c & 0x7f);
	}
}

int
asc_print_stat(void)
{
	register int i;
	register char *p;
	for (i = 0; i < MAXLOG_VALUE; i++) {
		if (p = logtbl[i].name)
			printf("%d %s\n", logtbl[i].count, p);
	}
}

#else	/*TRACE*/
#define	LOG(e,f)
#define LOGSIZE
#endif	/*TRACE*/

#else	/*DEBUG*/
#define PRINT(x)
#define	LOG(e,f)
#define LOGSIZE
#define	TR(x)
#define TRCHECK
#define TRWRAP

#endif	/*DEBUG*/


/*
 *	Probe/Slave/Attach functions
 */

/*
 * Probe routine:
 *	Should find out (a) if the controller is
 *	present and (b) which/where slaves are present.
 *
 * Implementation:
 *	Send a test-unit-ready to each possible target on the bus
 *	except of course ourselves.
 */
int
asc_probe(
	vm_offset_t		reg,
	struct bus_ctlr		*ui)
{
	int             unit = ui->unit;
	asc_softc_t     asc = &asc_softc_data[unit];
	int		target_id;
	scsi_softc_t	*sc;
	register asc_padded_regmap_t	*regs;
	spl_t		s;
	boolean_t	did_banner = FALSE;

	/*
	 * We are only called if the right board is there,
	 * but make sure anyways..
	 */
	if (check_memory(reg, 0))
		return 0;

#if  defined(HAS_MAPPED_SCSI)
	/* Mappable version side */
	ASC_probe(reg, ui);
#endif

	/*
	 * Initialize hw descriptor, cache some pointers
	 */
	asc_softc[unit] = asc;
	asc->regs = (asc_padded_regmap_t *) (reg);

	if ((asc->dma_ops = (scsi_dma_ops_t *)asc_std[unit]) == 0)
		/* use same as unit 0 if undefined */
		asc->dma_ops = (scsi_dma_ops_t *)asc_std[0];
	{
		int dma_bsize = 16;	/* bits, preferred */
		asc->dma_state = (*asc->dma_ops->init)(unit, reg, &dma_bsize);
		if (dma_bsize > 16)
			asc->state |= ASC_STATE_SPEC_DMA;
	}

	queue_init(&asc->waiting_targets);

	asc->clk = asc_clock_speed_in_mhz[unit];
	asc->ccf = mhz_to_ccf(asc->clk); /* see .h file */
	asc->timeout = asc_timeout_250(asc->clk,asc->ccf);

	sc = scsi_master_alloc(unit, (char *)asc);
	asc->sc = sc;

	sc->go = (void *)asc_go;
	sc->watchdog = scsi_watchdog;
	sc->probe = asc_probe_target;
	asc->wd.reset = asc_reset_scsibus;

#ifdef	MACH_KERNEL
	sc->max_dma_data = -1;
#else
	sc->max_dma_data = scsi_per_target_virtual;
#endif

	regs = asc->regs;

	/*
	 * Our SCSI id on the bus.
	 * The user can set this via the prom on 3maxen/pmaxen.
	 * If this changes it is easy to fix: make a default that
	 * can be changed as boot arg.
	 */
	{
		register unsigned char	my_id;

		my_id = scsi_initiator_id[unit] & 0x7;
		if (my_id != 7)
			regs->asc_cnfg1 = my_id; mb();
	}

	/*
	 * Reset chip, fully.  Note that interrupts are already enabled.
	 */
	s = splbio();
	asc_reset(asc, TRUE, asc->state & ASC_STATE_SPEC_DMA);

	sc->initiator_id = regs->asc_cnfg1 & ASC_CNFG1_MY_BUS_ID;
	printf("%s%d: SCSI id %d", ui->name, unit, sc->initiator_id);

	{
		register target_info_t	*tgt;

		tgt = scsi_slave_alloc(sc->masterno, sc->initiator_id, (char *)asc);
		(void) (*asc->dma_ops->new_target)(asc->dma_state, tgt);
		sccpu_new_initiator(tgt, tgt);
	}

    if (asc_probe_dynamically)
	printf("%s", ", will probe targets on demand");
    else {

	/*
	 * For all possible targets, see if there is one and allocate
	 * a descriptor for it if it is there.
	 */
	for (target_id = 0; target_id < 8; target_id++) {
		register unsigned char	csr, ss, ir, ff;
		register scsi_status_byte_t	status;

		/* except of course ourselves */
		if (target_id == sc->initiator_id)
			continue;

		regs->asc_cmd = ASC_CMD_FLUSH;	/* empty fifo */
		mb();
		delay(2);

		regs->asc_dbus_id = target_id; mb();
		regs->asc_sel_timo = asc->timeout; mb();

		/*
		 * See if the unit is ready.
		 * XXX SHOULD inquiry LUN 0 instead !!!
		 */
		regs->asc_fifo = SCSI_CMD_TEST_UNIT_READY; mb();
		regs->asc_fifo = 0; mb();
		regs->asc_fifo = 0; mb();
		regs->asc_fifo = 0; mb();
		regs->asc_fifo = 0; mb();
		regs->asc_fifo = 0; mb();

		/* select and send it */
		regs->asc_cmd = ASC_CMD_SEL; mb();

		/* wait for the chip to complete, or timeout */
		csr = asc_wait(regs, ASC_CSR_INT);
		ss = get_reg(regs,asc_ss);
		ir = get_reg(regs,asc_intr);

		/* empty fifo, there is garbage in it if timeout */
		regs->asc_cmd = ASC_CMD_FLUSH; mb();
		delay(2);

		/*
		 * Check if the select timed out
		 */
		if ((ASC_SS(ss) == 0) && (ir == ASC_INT_DISC))
			/* noone out there */
			continue;

		if (SCSI_PHASE(csr) != SCSI_PHASE_STATUS) {
			printf( " %s%d%s", "ignoring target at ", target_id,
				" cuz it acts weirdo");
			continue;
		}

		printf(",%s%d", did_banner++ ? " " : " target(s) at ",
				target_id);

		regs->asc_cmd = ASC_CMD_I_COMPLETE;
		wbflush();
		csr = asc_wait(regs, ASC_CSR_INT);
		ir = get_reg(regs,asc_intr); /* ack intr */
		mb();

		status.bits = get_fifo(regs); /* empty fifo */
		mb();
		ff = get_fifo(regs);

		if (status.st.scsi_status_code != SCSI_ST_GOOD)
			scsi_error( 0, SCSI_ERR_STATUS, status.bits, 0);

		regs->asc_cmd = ASC_CMD_MSG_ACPT; mb();
		csr = asc_wait(regs, ASC_CSR_INT);
		ir = get_reg(regs,asc_intr); /* ack intr */
		mb();

		/*
		 * Found a target
		 */
		asc->ntargets++;
		{
			register target_info_t	*tgt;
			tgt = scsi_slave_alloc(sc->masterno, target_id, (char *)asc);

			(*asc->dma_ops->new_target)(asc->dma_state, tgt);
		}
	}
    } /* asc_probe_dynamically */

	regs->asc_cmd = ASC_CMD_ENABLE_SEL; mb();

	printf(".\n");

	splx(s);
	return 1;
}

boolean_t
asc_probe_target(
	target_info_t 		*tgt,
	io_req_t		ior)
{
	asc_softc_t     asc = asc_softc[tgt->masterno];
	boolean_t	newlywed;

	newlywed = (tgt->cmd_ptr == 0);
	if (newlywed) {
		(*asc->dma_ops->new_target)(asc->dma_state, tgt);
	}

	if (scsi_inquiry(tgt, SCSI_INQ_STD_DATA) == SCSI_RET_DEVICE_DOWN)
		return FALSE;

	tgt->flags = TGT_ALIVE;
	return TRUE;
}

static int
asc_wait(
	asc_padded_regmap_t	*regs,
	int			until)
{
	int timeo = 1000000;
	while ((regs->asc_csr & until) == 0) {
		mb();
		delay(1);
		if (!timeo--) {
			printf("asc_wait TIMEO with x%x\n", get_reg(regs,asc_csr));
			break;
		}
	}
	return get_reg(regs,asc_csr);
}

int
asc_reset(
	asc_softc_t		asc,
	int			quick,
	int			special_dma)
{
	char	my_id;
	int	ccf;
	asc_padded_regmap_t	*regs;

	regs = asc->regs;

	/* preserve our ID for now */
	my_id = (regs->asc_cnfg1 & ASC_CNFG1_MY_BUS_ID);

	/*
	 * Reset chip and wait till done
	 */
	regs->asc_cmd = ASC_CMD_RESET;
	wbflush(); delay(25);

	/* spec says this is needed after reset */
	regs->asc_cmd = ASC_CMD_NOP;
	wbflush(); delay(25);

	/*
	 * Set up various chip parameters
	 */
	regs->asc_ccf = asc->ccf;
	wbflush();
	delay(25);
	regs->asc_sel_timo = asc->timeout; mb();
	/* restore our ID */
	regs->asc_cnfg1 = my_id | ASC_CNFG1_P_CHECK; mb();
	regs->asc_cnfg2 = /* ASC_CNFG2_EPL | */ ASC_CNFG2_SCSI2; mb();
	regs->asc_cnfg3 = special_dma ? (ASC_CNFG3_T8|ASC_CNFG3_ALT_DMA) : 0;
	mb();
	/* zero anything else */
	ASC_TC_PUT(regs, 0); mb();
	regs->asc_syn_p = asc_min_period; mb();
	regs->asc_syn_o = 0; mb();	/* asynch for now */

	regs->asc_cmd = ASC_CMD_ENABLE_SEL; mb();

	if (quick) return;

	/*
	 * reset the scsi bus, the interrupt routine does the rest
	 * or you can call asc_bus_reset().
	 */
	regs->asc_cmd = ASC_CMD_BUS_RESET; mb();
}


/*
 *	Operational functions
 */

/*
 * Start a SCSI command on a target
 */
int
asc_go(
	target_info_t		*tgt,
	int			cmd_count,
	int			in_count,
	boolean_t		cmd_only)
{
	asc_softc_t	asc;
	register spl_t	s;
	boolean_t	disconn;
	script_t	scp;
	boolean_t	(*handler)();

	LOG(1,"go");

	asc = (asc_softc_t)tgt->hw_state;

	tgt->transient_state.cmd_count = cmd_count; /* keep it here */

	(*asc->dma_ops->map)(asc->dma_state, tgt);

	disconn  = BGET(scsi_might_disconnect,tgt->masterno,tgt->target_id);
	disconn  = disconn && (asc->ntargets > 1);
	disconn |= BGET(scsi_should_disconnect,tgt->masterno,tgt->target_id);

	/*
	 * Setup target state
	 */
	tgt->done = SCSI_RET_IN_PROGRESS;

	handler = (disconn) ? asc_err_disconn : asc_err_generic;

	switch (tgt->cur_cmd) {
	    case SCSI_CMD_READ:
	    case SCSI_CMD_LONG_READ:
		LOG(2,"readop");
		scp = asc_script_data_in;
		break;
	    case SCSI_CMD_WRITE:
	    case SCSI_CMD_LONG_WRITE:
		LOG(0x18,"writeop");
		scp = asc_script_data_out;
		break;
	    case SCSI_CMD_INQUIRY:
		/* This is likely the first thing out:
		   do the synch neg if so */
		if (!cmd_only && ((tgt->flags&TGT_DID_SYNCH)==0)) {
			scp = asc_script_try_synch;
			tgt->flags |= TGT_TRY_SYNCH;
			break;
		}
	    case SCSI_CMD_REQUEST_SENSE:
	    case SCSI_CMD_MODE_SENSE:
	    case SCSI_CMD_RECEIVE_DIAG_RESULTS:
	    case SCSI_CMD_READ_CAPACITY:
	    case SCSI_CMD_READ_BLOCK_LIMITS:
	    case SCSI_CMD_READ_TOC:
	    case SCSI_CMD_READ_SUBCH:
	    case SCSI_CMD_READ_HEADER:
	    case 0xc4:	/* despised: SCSI_CMD_DEC_PLAYBACK_STATUS */
	    case 0xdd:	/* despised: SCSI_CMD_NEC_READ_SUBCH_Q */
	    case 0xde:	/* despised: SCSI_CMD_NEC_READ_TOC */
		scp = asc_script_data_in;
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    case SCSI_CMD_MODE_SELECT:
	    case SCSI_CMD_REASSIGN_BLOCKS:
	    case SCSI_CMD_FORMAT_UNIT:
	    case 0xc9: /* vendor-spec: SCSI_CMD_DEC_PLAYBACK_CONTROL */
		tgt->transient_state.cmd_count = sizeof_scsi_command(tgt->cur_cmd);
		tgt->transient_state.out_count =
			cmd_count - tgt->transient_state.cmd_count;
		scp = asc_script_data_out;
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    case SCSI_CMD_TEST_UNIT_READY:
		/*
		 * Do the synch negotiation here, unless prohibited
		 * or done already
		 */
		if (tgt->flags & TGT_DID_SYNCH) {
			scp = asc_script_simple_cmd;
		} else {
			scp = asc_script_try_synch;
			tgt->flags |= TGT_TRY_SYNCH;
			cmd_only = FALSE;
		}
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    default:
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		scp = asc_script_simple_cmd;
	}

	tgt->transient_state.script = scp;
	tgt->transient_state.handler = handler;
	tgt->transient_state.identify = (cmd_only) ? 0xff :
		(disconn ? SCSI_IDENTIFY|SCSI_IFY_ENABLE_DISCONNECT :
			   SCSI_IDENTIFY);

	if (in_count)
		tgt->transient_state.in_count =
			(in_count < tgt->block_size) ? tgt->block_size : in_count;
	else
		tgt->transient_state.in_count = 0;

	/*
	 * See if another target is currently selected on
	 * this SCSI bus, e.g. lock the asc structure.
	 * Note that it is the strategy routine's job
	 * to serialize ops on the same target as appropriate.
	 * XXX here and everywhere, locks!
	 */
	/*
	 * Protection viz reconnections makes it tricky.
	 */
	s = splbio();

	if (asc->wd.nactive++ == 0)
		asc->wd.watchdog_state = SCSI_WD_ACTIVE;

	if (asc->state & ASC_STATE_BUSY) {
		/*
		 * Queue up this target, note that this takes care
		 * of proper FIFO scheduling of the scsi-bus.
		 */
		LOG(3,"enqueue");
		enqueue_tail(&asc->waiting_targets, (queue_entry_t) tgt);
	} else {
		/*
		 * It is down to at most two contenders now,
		 * we will treat reconnections same as selections
		 * and let the scsi-bus arbitration process decide.
		 */
		asc->state |= ASC_STATE_BUSY;
		asc->next_target = tgt;
		asc_attempt_selection(asc);
		/*
		 * Note that we might still lose arbitration..
		 */
	}
	splx(s);
}

int
asc_attempt_selection(
	asc_softc_t		asc)
{
	target_info_t	*tgt;
	asc_padded_regmap_t	*regs;
	register int	out_count;

	regs = asc->regs;
	tgt = asc->next_target;

	LOG(4,"select");
	LOG(0x80+tgt->target_id,0);

	/*
	 * We own the bus now.. unless we lose arbitration
	 */
	asc->active_target = tgt;

	/* Try to avoid reselect collisions */
	if ((regs->asc_csr & (ASC_CSR_INT|SCSI_PHASE_MSG_IN)) ==
	    (ASC_CSR_INT|SCSI_PHASE_MSG_IN))
		return;

	/*
	 * Cleanup the FIFO
	 */
	regs->asc_cmd = ASC_CMD_FLUSH;
	readback(regs->asc_cmd);
	/*
	 * This value is not from spec, I have seen it failing
	 * without this delay and with logging disabled.  That had
	 * about 42 extra instructions @25Mhz.
	 */
	delay(2);/* XXX time & move later */


	/*
	 * Init bus state variables
	 */
	asc->script = tgt->transient_state.script;
	asc->error_handler = tgt->transient_state.handler;
	asc->done = SCSI_RET_IN_PROGRESS;

	asc->out_count = 0;
	asc->in_count = 0;
	asc->extra_count = 0;

	/*
	 * Start the chip going
	 */
	out_count = (*asc->dma_ops->start_cmd)(asc->dma_state, tgt);
	if (tgt->transient_state.identify != 0xff) {
		regs->asc_fifo = tgt->transient_state.identify;
		mb();
	}
	ASC_TC_PUT(regs, out_count);
	readback(regs->asc_cmd);

	regs->asc_dbus_id = tgt->target_id;
	readback(regs->asc_dbus_id);

	regs->asc_sel_timo = asc->timeout;
	readback(regs->asc_sel_timo);

	regs->asc_syn_p = tgt->sync_period;
	readback(regs->asc_syn_p);

	regs->asc_syn_o = tgt->sync_offset;
	readback(regs->asc_syn_o);

	/* ugly little help for compiler */
#define	command	out_count
	if (tgt->flags & TGT_DID_SYNCH) {
		command = (tgt->transient_state.identify == 0xff) ?
				ASC_CMD_SEL | ASC_CMD_DMA :
				ASC_CMD_SEL_ATN | ASC_CMD_DMA; /*preferred*/
	} else if (tgt->flags & TGT_TRY_SYNCH)
		command = ASC_CMD_SEL_ATN_STOP;
	else
		command = ASC_CMD_SEL | ASC_CMD_DMA;

	/* Try to avoid reselect collisions */
	if ((regs->asc_csr & (ASC_CSR_INT|SCSI_PHASE_MSG_IN)) !=
	    (ASC_CSR_INT|SCSI_PHASE_MSG_IN)) {
		register int	tmp;

		regs->asc_cmd = command; mb();
		/*
		 * Very nasty but infrequent problem here.  We got/will get
		 * reconnected but the chip did not interrupt.  The watchdog would
		 * fix it allright, but it stops the machine before it expires!
		 * Too bad we cannot just look at the interrupt register, sigh.
		 */
		tmp = get_reg(regs,asc_cmd);
		if ((tmp != command) && (tmp == (ASC_CMD_NOP|ASC_CMD_DMA))) {
		    if ((regs->asc_csr & ASC_CSR_INT) == 0) {
			delay(250); /* increase if in trouble */

			if (get_reg(regs,asc_csr) == SCSI_PHASE_MSG_IN) {
			    /* ok, take the risk of reading the ir */
			    tmp = get_reg(regs,asc_intr); mb();
			    if (tmp & ASC_INT_RESEL) {
				(void) asc_reconnect(asc, get_reg(regs,asc_csr), tmp);
				asc_wait(regs, ASC_CSR_INT);
				tmp = get_reg(regs,asc_intr); mb();
				regs->asc_cmd = ASC_CMD_MSG_ACPT;
				readback(regs->asc_cmd);
			    } else /* does not happen, but who knows.. */
				asc_reset(asc,FALSE,asc->state & ASC_STATE_SPEC_DMA);
			}
		    }
		}
	}
#undef	command
}

/*
 * Interrupt routine
 *	Take interrupts from the chip
 *
 * Implementation:
 *	Move along the current command's script if
 *	all is well, invoke error handler if not.
 */
int
asc_intr(
	int			unit,
	spl_t			spllevel)
{
	register asc_softc_t	asc;
	register script_t	scp;
	register int		ir, csr;
	register asc_padded_regmap_t	*regs;
#if  defined(HAS_MAPPED_SCSI)
	extern boolean_t	rz_use_mapped_interface;

	if (rz_use_mapped_interface)
		return ASC_intr(unit,spllevel);
#endif

	asc = asc_softc[unit];

	LOG(5,"\n\tintr");
	/* collect ephemeral information */
	regs = asc->regs;
	csr  = get_reg(regs,asc_csr); mb();
	asc->ss_was = get_reg(regs,asc_ss); mb();
	asc->cmd_was = get_reg(regs,asc_cmd); mb();

	/* drop spurious interrupts */
	if ((csr & ASC_CSR_INT) == 0)
		return;

	ir = get_reg(regs,asc_intr);	/* this re-latches CSR (and SSTEP) */
	mb();

TR(csr);TR(ir);TR(get_reg(regs,asc_cmd));TRCHECK

	/* this must be done within 250msec of disconnect */
	if (ir & ASC_INT_DISC) {
		regs->asc_cmd = ASC_CMD_ENABLE_SEL;
		readback(asc->regs->asc_cmd);
	}

	if (ir & ASC_INT_RESET) {
		asc_bus_reset(asc);
		return;
	}

	/* we got an interrupt allright */
	if (asc->active_target)
		asc->wd.watchdog_state = SCSI_WD_ACTIVE;

#if	mips
	splx(spllevel);	/* drop priority */
#endif

	if ((asc->state & ASC_STATE_TARGET) ||
	    (ir & (ASC_INT_SEL|ASC_INT_SEL_ATN)))
		return asc_target_intr(asc, csr, ir);

	/*
	 * In attempt_selection() we could not check the asc_intr
	 * register to see if a reselection was in progress [else
	 * we would cancel the interrupt, and it would not be safe
	 * anyways].  So we gave the select command even if sometimes
	 * the chip might have been reconnected already.  What
	 * happens then is that we get an illegal command interrupt,
	 * which is why the second clause.  Sorry, I'd do it better
	 * if I knew of a better way.
	 */
	if ((ir & ASC_INT_RESEL) ||
	    ((ir & ASC_INT_ILL) && (regs->asc_cmd & ASC_CMD_SEL_ATN)))
		return asc_reconnect(asc, csr, ir);

	/*
	 * Check for various errors
	 */
	if ((csr & (ASC_CSR_GE|ASC_CSR_PE)) || (ir & ASC_INT_ILL)) {
		char	*msg;
printf("{E%x,%x}", csr, ir);
		if (csr & ASC_CSR_GE)
			return;/* sit and prey? */

		if (csr & ASC_CSR_PE)
			msg = "SCSI bus parity error";
		if (ir & ASC_INT_ILL)
			msg = "Chip sez Illegal Command";
		/* all we can do is to throw a reset on the bus */
		printf( "asc%d: %s%s", asc - asc_softc_data, msg,
			", attempting recovery.\n");
		asc_reset(asc, FALSE, asc->state & ASC_STATE_SPEC_DMA);
		return;
	}

	if ((scp = asc->script) == 0)	/* sanity */
		return;

	LOG(6,"match");
	if (SCRIPT_MATCH(csr,ir,scp->condition)) {
		/*
		 * Perform the appropriate operation,
		 * then proceed
		 */
		if ((*scp->action)(asc, csr, ir)) {
			asc->script = scp + 1;
			regs->asc_cmd = scp->command; mb();
		}
	} else
		(void) (*asc->error_handler)(asc, csr, ir);
}

int
asc_target_intr(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	register script_t	scp;

	LOG(0x1e,"tmode");

	if ((asc->state & ASC_STATE_TARGET) == 0) {

		/*
		 * We just got selected
		 */
		asc->state |= ASC_STATE_TARGET;

		/*
		 * See if this selection collided with our selection attempt
		 */
		if (asc->state & ASC_STATE_BUSY)
			asc->state |= ASC_STATE_COLLISION;
		asc->state |= ASC_STATE_BUSY;

		return asc_selected(asc, csr, ir);

	}
	/* We must be executing a script here */
	scp = asc->script;
	assert(scp != 0);

	LOG(6,"match");
	if (SCRIPT_MATCH(csr,ir,scp->condition)) {
		/*
		 * Perform the appropriate operation,
		 * then proceed
		 */
		if ((*scp->action)(asc, csr, ir)) {
			asc->script = scp + 1;
			asc->regs->asc_cmd = scp->command; mb();
		}
	} else
		(void) (*asc->error_handler)(asc, csr, ir);
	
}

/*
 * All the many little things that the interrupt
 * routine might switch to
 */
boolean_t
asc_clean_fifo(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register char		ff;

	ASC_TC_PUT(regs,0);	/* stop dma engine */
	readback(regs->asc_cmd);

	LOG(7,"clean_fifo");

	while (fifo_count(regs)) {
		ff = get_fifo(regs);
		mb();
	}
	return TRUE;
}

boolean_t
asc_end(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	register target_info_t	*tgt;
	register io_req_t	ior;

	LOG(8,"end");

	asc->state &= ~ASC_STATE_TARGET;
	asc->regs->asc_syn_p = 0; mb();
	asc->regs->asc_syn_o = 0; mb();

	tgt = asc->active_target;
	if ((tgt->done = asc->done) == SCSI_RET_IN_PROGRESS)
		tgt->done = SCSI_RET_SUCCESS;

	asc->script = 0;

	if (asc->wd.nactive-- == 1)
		asc->wd.watchdog_state = SCSI_WD_INACTIVE;

	asc_release_bus(asc);

	if (ior = tgt->ior) {
		(*asc->dma_ops->end_cmd)(asc->dma_state, tgt, ior);
		LOG(0xA,"ops->restart");
		(*tgt->dev_ops->restart)( tgt, TRUE);
	}

	return FALSE;
}

boolean_t
asc_release_bus(
	register asc_softc_t	asc)
{
	boolean_t	ret = TRUE;

	LOG(9,"release");
	if (asc->state & ASC_STATE_COLLISION) {

		LOG(0xB,"collided");
		asc->state &= ~ASC_STATE_COLLISION;
		asc_attempt_selection(asc);

	} else if (queue_empty(&asc->waiting_targets)) {

		asc->state &= ~ASC_STATE_BUSY;
		asc->active_target = 0;
		asc->script = 0;
		ret = FALSE;

	} else {

		LOG(0xC,"dequeue");
		asc->next_target = (target_info_t *)
				dequeue_head(&asc->waiting_targets);
		asc_attempt_selection(asc);
	}
	return ret;
}

boolean_t
asc_get_status(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register scsi2_status_byte_t status;
	int			len;
	boolean_t		ret;
	io_req_t		ior;
	register target_info_t	*tgt = asc->active_target;

	LOG(0xD,"get_status");
TRWRAP

	asc->state &= ~ASC_STATE_DMA_IN;

	/*
	 * Get the last two bytes in FIFO
	 */
	while (fifo_count(regs) > 2) {
		status.bits = get_fifo(regs); mb();
	}

	status.bits = get_fifo(regs); mb();

	if (status.st.scsi_status_code != SCSI_ST_GOOD) {
		scsi_error(asc->active_target, SCSI_ERR_STATUS, status.bits, 0);
		asc->done = (status.st.scsi_status_code == SCSI_ST_BUSY) ?
			SCSI_RET_RETRY : SCSI_RET_NEED_SENSE;
	} else
		asc->done = SCSI_RET_SUCCESS;

	status.bits = get_fifo(regs);	/* just pop the command_complete */
	mb();

	/* if reading, move the last piece of data in main memory */
	if (len = asc->in_count) {
		register int	count;

		ASC_TC_GET(regs,count); mb();
		if (count) {
#if 0
			this is incorrect and besides..
			tgt->ior->io_residual = count;
#endif
			len -= count;
		}
		regs->asc_cmd = asc->script->command;
		readback(regs->asc_cmd);
		
		ret = FALSE;
	} else
		ret = TRUE;

	(*asc->dma_ops->end_xfer)(asc->dma_state, tgt, len);
	if (!ret)
		asc->script++;
	return ret;
}

boolean_t
asc_put_status(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register scsi2_status_byte_t	status;
	register target_info_t		*tgt = asc->active_target;
	int				len;

	LOG(0x21,"put_status");

	asc->state &= ~ASC_STATE_DMA_IN;

	if (len = asc->in_count) {
		register int	count;

		ASC_TC_GET(regs,count); mb();
		if (count)
			len -= count;
	}
	(*asc->dma_ops->end_xfer)(asc->dma_state, tgt, len);

/*	status.st.scsi_status_code = SCSI_ST_GOOD; */
	regs->asc_fifo = 0; mb();
	regs->asc_fifo = SCSI_COMMAND_COMPLETE; mb();

	return TRUE;
}


boolean_t
asc_dma_in(asc, csr, ir)
	register asc_softc_t	asc;
{
	register target_info_t	*tgt;
	register asc_padded_regmap_t	*regs = asc->regs;
	register int		count;
	unsigned char		ff = get_reg(regs,asc_flags); mb();

	LOG(0xE,"dma_in");
	tgt = asc->active_target;

	/*
	 * This seems to be needed on certain rudimentary targets
	 * (such as the DEC TK50 tape) which apparently only pick
	 * up 6 initial bytes: when you add the initial IDENTIFY
	 * you are left with 1 pending byte, which is left in the
	 * FIFO and would otherwise show up atop the data we are
	 * really requesting.
	 *
	 * This is only speculation, though, based on the fact the
	 * sequence step value of 3 out of select means the target
	 * changed phase too quick and some bytes have not been
	 * xferred (see NCR manuals).  Counter to this theory goes
	 * the fact that the extra byte that shows up is not alwyas
	 * zero, and appears to be pretty random.
	 * Note that asc_flags say there is one byte in the FIFO
	 * even in the ok case, but the sstep value is the right one.
	 * Note finally that this might all be a sync/async issue:
	 * I have only checked the ok case on synch disks so far.
	 *
	 * Indeed it seems to be an asynch issue: exabytes do it too.
	 */
	if ((tgt->sync_offset == 0) && ((ff & ASC_FLAGS_SEQ_STEP) != 0x80)) {
		regs->asc_cmd = ASC_CMD_NOP;
		wbflush();
		PRINT(("[tgt %d: %x while %d]", tgt->target_id, ff, tgt->cur_cmd));
		while ((ff & ASC_FLAGS_FIFO_CNT) != 0) {
			ff = get_fifo(regs); mb();
			ff = get_reg(regs,asc_flags); mb();
		}
	}

	asc->state |= ASC_STATE_DMA_IN;

	count = (*asc->dma_ops->start_datain)(asc->dma_state, tgt);
	ASC_TC_PUT(regs, count);
	readback(regs->asc_cmd);

	if ((asc->in_count = count) == tgt->transient_state.in_count)
		return TRUE;
	regs->asc_cmd = asc->script->command; mb();
	asc->script = asc_script_restart_data_in;
	return FALSE;
}

int
asc_dma_in_r(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	register target_info_t	*tgt;
	register asc_padded_regmap_t	*regs = asc->regs;
	register int		count;
	boolean_t		advance_script = TRUE;


	LOG(0x1f,"dma_in_r");
	tgt = asc->active_target;

	asc->state |= ASC_STATE_DMA_IN;

	if (asc->in_count == 0) {
		/*
		 * Got nothing yet, we just reconnected.
		 */
		register int avail;

		/*
		 * Rather than using the messy RFB bit in cnfg2
		 * (which only works for synch xfer anyways)
		 * we just bump up the dma offset.  We might
		 * endup with one more interrupt at the end,
		 * so what.
		 * This is done in asc_err_disconn(), this
		 * way dma (of msg bytes too) is always aligned
		 */

		count = (*asc->dma_ops->restart_datain_1)
				(asc->dma_state, tgt);

		/* common case of 8k-or-less read ? */
		advance_script = (tgt->transient_state.in_count == count);

	} else {

		/*
		 * We received some data.
		 */
		register int offset, xferred;

		/*
		 * Problem: sometimes we get a 'spurious' interrupt
		 * right after a reconnect.  It goes like disconnect,
		 * reconnect, dma_in_r, here but DMA is still rolling.
		 * Since there is no good reason we got here to begin with
		 * we just check for the case and dismiss it: we should
		 * get another interrupt when the TC goes to zero or the
		 * target disconnects.
		 */
		ASC_TC_GET(regs,xferred); mb();
		if (xferred != 0)
			return FALSE;

		xferred = asc->in_count - xferred;
		assert(xferred > 0);

		tgt->transient_state.in_count -= xferred;
		assert(tgt->transient_state.in_count > 0);

		count = (*asc->dma_ops->restart_datain_2)
				(asc->dma_state, tgt, xferred);

		asc->in_count = count;
		ASC_TC_PUT(regs, count);
		readback(regs->asc_cmd);
		regs->asc_cmd = asc->script->command; mb();

		(*asc->dma_ops->restart_datain_3)
			(asc->dma_state, tgt);

		/* last chunk ? */
		if (count == tgt->transient_state.in_count)
			asc->script++;

		return FALSE;
	}

	asc->in_count = count;
	ASC_TC_PUT(regs, count);
	readback(regs->asc_cmd);

	if (!advance_script) {
		regs->asc_cmd = asc->script->command;
		readback(regs->asc_cmd);
	}
	return advance_script;
}


/* send data to target.  Only called to start the xfer */

boolean_t
asc_dma_out(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register int		reload_count;
	register target_info_t	*tgt;
	int			command;

	LOG(0xF,"dma_out");

	ASC_TC_GET(regs, reload_count); mb();
	asc->extra_count = fifo_count(regs);
	reload_count += asc->extra_count;
	ASC_TC_PUT(regs, reload_count);
	asc->state &= ~ASC_STATE_DMA_IN;
	readback(regs->asc_cmd);

	tgt = asc->active_target;

	command = asc->script->command;

	if ((asc->out_count = reload_count) >=
	    tgt->transient_state.out_count)
		asc->script++;
	else
		asc->script = asc_script_restart_data_out;

	if ((*asc->dma_ops->start_dataout)
	      (asc->dma_state, tgt, &regs->asc_cmd, command)) {
			regs->asc_cmd = command;
			readback(regs->asc_cmd);
	}

	return FALSE;
}

/* send data to target. Called in two different ways:
   (a) to restart a big transfer and 
   (b) after reconnection
 */
boolean_t
asc_dma_out_r(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register target_info_t	*tgt;
	boolean_t		advance_script = TRUE;
	int			count;


	LOG(0x20,"dma_out_r");

	tgt = asc->active_target;
	asc->state &= ~ASC_STATE_DMA_IN;

	if (asc->out_count == 0) {
		/*
		 * Nothing committed: we just got reconnected
		 */
		count = (*asc->dma_ops->restart_dataout_1)
				(asc->dma_state, tgt);

		/* is this the last chunk ? */
		advance_script = (tgt->transient_state.out_count == count);
	} else {
		/*
		 * We sent some data.
		 */
		register int offset, xferred;

		ASC_TC_GET(regs,count); mb();

		/* see comment above */
		if (count) {
			return FALSE;
		}

		count += fifo_count(regs);
		count -= asc->extra_count;
		xferred = asc->out_count - count;
		assert(xferred > 0);

		tgt->transient_state.out_count -= xferred;
		assert(tgt->transient_state.out_count > 0);

		count = (*asc->dma_ops->restart_dataout_2)
				(asc->dma_state, tgt, xferred);

		/* last chunk ? */
		if (tgt->transient_state.out_count == count)
			goto quickie;

		asc->out_count = count;

		asc->extra_count = (*asc->dma_ops->restart_dataout_3)
					(asc->dma_state, tgt, &regs->asc_fifo);
		ASC_TC_PUT(regs, count);
		readback(regs->asc_cmd);
		regs->asc_cmd = asc->script->command; mb();

		(*asc->dma_ops->restart_dataout_4)(asc->dma_state, tgt);

		return FALSE;
	}

quickie:
	asc->extra_count = (*asc->dma_ops->restart_dataout_3)
				(asc->dma_state, tgt, &regs->asc_fifo);

	asc->out_count = count;

	ASC_TC_PUT(regs, count);
	readback(regs->asc_cmd);

	if (!advance_script) {
		regs->asc_cmd = asc->script->command;
	}
	return advance_script;
}

boolean_t
asc_dosynch(
	register asc_softc_t	asc,
	register unsigned char	csr,
	register unsigned char	ir)
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register unsigned char		c;
	int				i, per, offs;
	register target_info_t		*tgt;

	/*
	 * Phase is MSG_OUT here.
	 * Try to go synchronous, unless prohibited
	 */
	tgt = asc->active_target;
	regs->asc_cmd = ASC_CMD_FLUSH;
	readback(regs->asc_cmd);
	delay(1);

	per = asc_min_period;
	if (BGET(scsi_no_synchronous_xfer,asc->sc->masterno,tgt->target_id))
		offs = 0;
	else
		offs = asc_max_offset;

	tgt->flags |= TGT_DID_SYNCH;	/* only one chance */
	tgt->flags &= ~TGT_TRY_SYNCH;

	regs->asc_fifo = SCSI_EXTENDED_MESSAGE; mb();
	regs->asc_fifo = 3; mb();
	regs->asc_fifo = SCSI_SYNC_XFER_REQUEST; mb();
	regs->asc_fifo = asc_to_scsi_period(asc_min_period,asc->clk); mb();
	regs->asc_fifo = offs; mb();
	regs->asc_cmd = ASC_CMD_XFER_INFO;
	readback(regs->asc_cmd);
	csr = asc_wait(regs, ASC_CSR_INT);
	ir = get_reg(regs,asc_intr); mb();

	/* some targets might be slow to move to msg-in */
	if (SCSI_PHASE(csr) != SCSI_PHASE_MSG_IN) {

		/* wait for direction bit to flip */
		csr = asc_wait(regs, SCSI_IO);
		ir = get_reg(regs,asc_intr); mb();
		/* Some UGLY targets go stright to command phase !
		   "You could at least say goodbye" */
		if (SCSI_PHASE(csr) == SCSI_PHASE_CMD)
			goto did_not;
		if (SCSI_PHASE(csr) != SCSI_PHASE_MSG_IN)
			gimmeabreak();
	}

	regs->asc_cmd = ASC_CMD_XFER_INFO; mb();
	csr = asc_wait(regs, ASC_CSR_INT);
	ir = get_reg(regs,asc_intr); mb();

	/* some targets do not even take all the bytes out */
	while (fifo_count(regs) > 0) {
		c = get_fifo(regs);	/* see what it says */
		mb();
	}

	if (c == SCSI_MESSAGE_REJECT) {
did_not:
		printf(" did not like SYNCH xfer ");

		/* Tk50s get in trouble with ATN, sigh. */
		regs->asc_cmd = ASC_CMD_CLR_ATN;
		readback(regs->asc_cmd);

		goto cmd;
	}

	/*
	 * Receive the rest of the message
	 */
	regs->asc_cmd = ASC_CMD_MSG_ACPT; mb();
	asc_wait(regs, ASC_CSR_INT);
	ir = get_reg(regs,asc_intr); mb();

	if (c != SCSI_EXTENDED_MESSAGE)
		gimmeabreak();

	regs->asc_cmd = ASC_CMD_XFER_INFO; mb();
	asc_wait(regs, ASC_CSR_INT);
	c = get_reg(regs,asc_intr); mb();
	if (get_fifo(regs) != 3)
		panic("asc_dosynch");

	for (i = 0; i < 3; i++) {
		regs->asc_cmd = ASC_CMD_MSG_ACPT; mb();
		asc_wait(regs, ASC_CSR_INT);
		c = get_reg(regs,asc_intr); mb();

		regs->asc_cmd = ASC_CMD_XFER_INFO; mb();
		asc_wait(regs, ASC_CSR_INT);
		c = get_reg(regs,asc_intr);/*ack*/ mb();
		c = get_fifo(regs); mb();

		if (i == 1) tgt->sync_period = scsi_period_to_asc(c,asc->clk);
		if (i == 2) tgt->sync_offset = c;
	}

cmd:
	regs->asc_cmd = ASC_CMD_MSG_ACPT; mb();
	csr = asc_wait(regs, ASC_CSR_INT);
	c = get_reg(regs,asc_intr); mb();

	/* Might have to wait a bit longer for slow targets */
	for (c = 0; SCSI_PHASE(get_reg(regs,asc_csr)) == SCSI_PHASE_MSG_IN; c++) {
		mb();
		delay(2);
		if (c & 0x80) break;  /* waited too long */
	}
	csr = get_reg(regs,asc_csr); mb();

	/* phase should normally be command here */
	if (SCSI_PHASE(csr) == SCSI_PHASE_CMD) {
		register char	*cmd = tgt->cmd_ptr;

		/* test unit ready or inquiry */
		for (i = 0; i < sizeof(scsi_command_group_0); i++) {
			regs->asc_fifo = *cmd++; mb();
		}
		ASC_TC_PUT(regs,0xff); mb();
		regs->asc_cmd = ASC_CMD_XFER_PAD; /*0x18*/ mb();

		if (tgt->cur_cmd == SCSI_CMD_INQUIRY) {
			tgt->transient_state.script = asc_script_data_in;
			asc->script = tgt->transient_state.script;
			regs->asc_syn_p = tgt->sync_period;
			regs->asc_syn_o = tgt->sync_offset; mb();
			return FALSE;
		}

		csr = asc_wait(regs, ASC_CSR_INT);
		ir = get_reg(regs,asc_intr);/*ack*/ mb();
	}

	if (SCSI_PHASE(csr) != SCSI_PHASE_STATUS)
		csr = asc_wait(regs, SCSI_IO);  /* direction flip */

status:
	if (SCSI_PHASE(csr) != SCSI_PHASE_STATUS)
		gimmeabreak();

	return TRUE;
}

/*  The other side of the coin.. */
int
asc_odsynch(
	register asc_softc_t	asc,
	target_info_t		*initiator)
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register unsigned char		c;
	int				len, per, offs;

	/*
	 * Phase is MSG_OUT, we are the target and we have control.
	 * Any IDENTIFY messages have been handled already.
	 */
	initiator->flags |= TGT_DID_SYNCH;
	initiator->flags &= ~TGT_TRY_SYNCH;

	/*
	 * We only understand synch negotiations
	 */
	c = get_fifo(regs); mb();
	if (c != SCSI_EXTENDED_MESSAGE) goto bad;

	/*
	 * This is not in the specs, but apparently the chip knows
	 * enough about scsi to receive the length automatically.
	 * So there were two bytes in the fifo at function call.
	 */
	len = get_fifo(regs); mb();
	if (len != 3) goto bad;
	while (len) {
		if (fifo_count(regs) == 0) {
			regs->asc_cmd = ASC_CMD_RCV_MSG;
			readback(regs->asc_cmd);
			asc_wait(regs, ASC_CSR_INT);
			c = get_reg(regs,asc_intr); mb();
		}
		c = get_fifo(regs); mb();
		if (len == 1) offs = c;
		if (len == 2) per = c;
		len--;
	}

	/*
	 * Adjust the proposed parameters
	 */
	c = scsi_period_to_asc(per,asc->clk);
	initiator->sync_period = c;
	per = asc_to_scsi_period(c,asc->clk);

	if (offs > asc_max_offset) offs = asc_max_offset;
	initiator->sync_offset = offs;

	/*
	 * Tell him what the deal is
	 */
	regs->asc_fifo = SCSI_EXTENDED_MESSAGE; mb();
	regs->asc_fifo = 3; mb();
	regs->asc_fifo = SCSI_SYNC_XFER_REQUEST; mb();
	regs->asc_fifo = per; mb();
	regs->asc_fifo = offs; mb();
	regs->asc_cmd = ASC_CMD_SND_MSG;
	readback(regs->asc_cmd);
	asc_wait(regs, ASC_CSR_INT);
	c = get_reg(regs,asc_intr); mb();

	/*
	 * Exit conditions: fifo empty, phase undefined but non-command
	 */
	return;

	/*
	 * Something wrong, reject the message
	 */
bad:
	while (fifo_count(regs)) {
		c = get_fifo(regs);  mb();
	}
	regs->asc_fifo = SCSI_MESSAGE_REJECT; mb();
	regs->asc_cmd = ASC_CMD_SND_MSG;
	readback(regs->asc_cmd);
	asc_wait(regs, ASC_CSR_INT);
	c = get_reg(regs,asc_intr); mb();
}

/*
 * The bus was reset
 */
void
asc_bus_reset(
	register asc_softc_t	asc)
{
	register asc_padded_regmap_t	*regs = asc->regs;

	LOG(0x1d,"bus_reset");

	/*
	 * Clear bus descriptor
	 */
	asc->script = 0;
	asc->error_handler = 0;
	asc->active_target = 0;
	asc->next_target = 0;
	asc->state &= ASC_STATE_SPEC_DMA;	/* save this one bit only */
	queue_init(&asc->waiting_targets);
	asc->wd.nactive = 0;
	asc_reset(asc, TRUE, asc->state & ASC_STATE_SPEC_DMA);

	printf("asc: (%d) bus reset ", ++asc->wd.reset_count);

	/* some targets take long to reset */
	delay(	scsi_delay_after_reset +
		asc->sc->initiator_id * 500000); /* if multiple initiators */

	if (asc->sc == 0)	/* sanity */
		return;

	scsi_bus_was_reset(asc->sc);
}

/*
 * Disconnect/reconnect mode ops
 */

/* get the message in via dma */
boolean_t
asc_msg_in(
	register asc_softc_t	asc,
	register unsigned char	csr,
	register unsigned char	ir)
{
	register target_info_t	*tgt;
	register asc_padded_regmap_t	*regs = asc->regs;
	unsigned char		ff;

	LOG(0x10,"msg_in");
	/* must clean FIFO as in asc_dma_in, sigh */
	while (fifo_count(regs) != 0) {
		ff = get_fifo(regs); mb();
	}

	(void) (*asc->dma_ops->start_msgin)(asc->dma_state, asc->active_target);
	/* We only really expect two bytes, at tgt->cmd_ptr */
	ASC_TC_PUT(regs, sizeof(scsi_command_group_0));
	readback(regs->asc_cmd);

	return TRUE;
}

/* check the message is indeed a DISCONNECT */
boolean_t
asc_disconnect(
	register asc_softc_t	asc,
	register unsigned char	csr,
	register unsigned char	ir)
{
	register char		*msgs, ff;
	register target_info_t	*tgt;
	asc_padded_regmap_t	*regs;

	tgt = asc->active_target;

	(*asc->dma_ops->end_msgin)(asc->dma_state, tgt);

	/*
	 * Do not do this. It is most likely a reconnection
	 * message that sits there already by the time we
	 * get here.  The chip is smart enough to only dma
	 * the bytes that correctly came in as msg_in proper,
	 * the identify and selection bytes are not dma-ed.
	 * Yes, sometimes the hardware does the right thing.
	 */
#if 0
	/* First check message got out of the fifo */
	regs = asc->regs;
	while (fifo_count(regs) != 0) {
		*msgs++ = get_fifo(regs);
	}
#endif
	msgs = tgt->cmd_ptr;

	/* A SDP message preceeds it in non-completed READs */
	if ((msgs[0] == SCSI_DISCONNECT) ||	/* completed */
	    ((msgs[0] == SCSI_SAVE_DATA_POINTER) && /* non complete */
	     (msgs[1] == SCSI_DISCONNECT))) {
		/* that's the ok case */
	} else
		printf("[tgt %d bad SDP: x%x]",
			tgt->target_id, *((unsigned short *)msgs));

	return TRUE;
}

/* save all relevant data, free the BUS */
boolean_t
asc_disconnected(
	register asc_softc_t	asc,
	register unsigned char	csr,
	register unsigned char	ir)
{
	register target_info_t	*tgt;

	LOG(0x11,"disconnected");
	asc_disconnect(asc,csr,ir);

	tgt = asc->active_target;
	tgt->flags |= TGT_DISCONNECTED;
	tgt->transient_state.handler = asc->error_handler;
	/* anything else was saved in asc_err_disconn() */

	PRINT(("{D%d}", tgt->target_id));

	asc_release_bus(asc);

	return FALSE;
}

/* get reconnect message out of fifo, restore BUS */
boolean_t
asc_reconnect(
	register asc_softc_t	asc,
	register unsigned char	csr,
	register unsigned char	ir)
{
	register target_info_t	*tgt;
	asc_padded_regmap_t	*regs;
	unsigned int		id;

	LOG(0x12,"reconnect");
	/*
	 * See if this reconnection collided with a selection attempt
	 */
	if (asc->state & ASC_STATE_BUSY)
		asc->state |= ASC_STATE_COLLISION;

	asc->state |= ASC_STATE_BUSY;

	/* find tgt: first byte in fifo is (tgt_id|our_id) */
	regs = asc->regs;
	while (fifo_count(regs) > 2) /* sanity */ {
		id = get_fifo(regs); mb();
	}
	if (fifo_count(regs) != 2)
		gimmeabreak();

	id = get_fifo(regs);		/* must decode this now */
	mb();
	id &= ~(1 << asc->sc->initiator_id);
	{
		register int	i;
		for (i = 0; id > 1; i++)
			id >>= 1;
		id = i;
	}

	/* sanity, empty fifo */
	if (get_fifo(regs) != SCSI_IDENTIFY)
		gimmeabreak();

	tgt = asc->sc->target[id];
	if (tgt == 0) panic("asc_reconnect");

	/* synch things*/
	regs->asc_syn_p = tgt->sync_period;
	regs->asc_syn_o = tgt->sync_offset;
	readback(regs->asc_syn_o);

	PRINT(("{R%d}", id));
	if (asc->state & ASC_STATE_COLLISION)
		PRINT(("[B %d-%d]", asc->active_target->target_id, id));

	LOG(0x80+id,0);

	asc->active_target = tgt;
	tgt->flags &= ~TGT_DISCONNECTED;

	asc->script = tgt->transient_state.script;
	asc->error_handler = tgt->transient_state.handler;
	asc->in_count = 0;
	asc->out_count = 0;

	/*
	 * Not sure why, but this prevents tk50s from embarassing
	 * us quite a bit
	 */
	delay(4);

	regs->asc_cmd = ASC_CMD_MSG_ACPT;
	readback(regs->asc_cmd);

	return FALSE;
}


/* We have been selected as target */

boolean_t
asc_selected(
	register asc_softc_t	asc,
	register unsigned int	csr,
	register unsigned int	ir)
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register unsigned char		id;
	target_info_t			*self, *initiator;
	unsigned int			len;

	/*
	 * Find initiator's id: the head of the fifo is (init_id|our_id)
	 */

	id = get_fifo(regs);	/* must decode this now */
	mb();
	id &= ~(1 << asc->sc->initiator_id);
	{
		register int	i;
		for (i = 0; id > 1; i++)
			id >>= 1;
		id = i;
	}

	/*
	 * See if we have seen him before
	 */
	self = asc->sc->target[asc->sc->initiator_id];
	initiator = asc->sc->target[id];
	if (initiator == 0) {

		initiator = scsi_slave_alloc(asc->sc->masterno, id, (char *)asc);
                (*asc->dma_ops->new_target)(asc->dma_state, initiator);

	}

	if (! (initiator->flags & TGT_ONLINE) )
		sccpu_new_initiator(self, initiator);

	/*
	 * If selected with ATN the chip did the msg-out
	 * phase already, let us look at the message(s)
	 */
	if (ir & ASC_INT_SEL_ATN) {
		register unsigned char	m;

		m = get_fifo(regs); mb();
		if ((m & SCSI_IDENTIFY) == 0)
			gimmeabreak();

		csr = get_reg(regs,asc_csr); mb();
		if ((SCSI_PHASE(csr) == SCSI_PHASE_MSG_OUT) &&
		    fifo_count(regs))
			asc_odsynch(asc, initiator);

		/* Get the command now, unless we have it already */
		mb();
		if (fifo_count(regs) < sizeof(scsi_command_group_0)) {
			regs->asc_cmd = ASC_CMD_RCV_CMD;
			readback(regs->asc_cmd);
			asc_wait(regs, ASC_CSR_INT);
			ir = get_reg(regs,asc_intr); mb();
			csr = get_reg(regs,asc_csr); mb();
		}
	} else {
		/*
		 * Pop away the null byte that follows the id
		 */
		if (get_fifo(regs) != 0)
			gimmeabreak();

	}

	/*
	 * Take rest of command out of fifo
	 */
	self->dev_info.cpu.req_pending = TRUE;
	self->dev_info.cpu.req_id = id;
	self->dev_info.cpu.req_cmd = get_fifo(regs);
	self->dev_info.cpu.req_lun = get_fifo(regs);

	LOG(0x80+self->dev_info.cpu.req_cmd, 0);

	switch ((self->dev_info.cpu.req_cmd & SCSI_CODE_GROUP) >> 5) {
	case 0:
		len  = get_fifo(regs) << 16; mb();
		len |= get_fifo(regs) <<  8; mb();
		len |= get_fifo(regs); mb();
		break;
	case 1:
	case 2:
		len = get_fifo(regs);	/* xxx lba1 */ mb();
		len = get_fifo(regs);	/* xxx lba2 */ mb();
		len = get_fifo(regs);	/* xxx lba3 */ mb();
		len = get_fifo(regs);	/* xxx lba4 */ mb();
		len = get_fifo(regs);	/* xxx xxx */ mb();
		len = get_fifo(regs) << 8; mb();
		len |= get_fifo(regs); mb();
		break;
	case 5:
		len = get_fifo(regs);	/* xxx lba1 */ mb();
		len = get_fifo(regs);	/* xxx lba2 */ mb();
		len = get_fifo(regs);	/* xxx lba3 */ mb();
		len = get_fifo(regs);	/* xxx lba4 */ mb();
		len  = get_fifo(regs) << 24; mb();
		len |= get_fifo(regs) << 16; mb();
		len |= get_fifo(regs) <<  8; mb();
		len |= get_fifo(regs); mb();
		if (get_fifo(regs) != 0) ;
		break;
	default:
		panic("asc_selected");
	}
	self->dev_info.cpu.req_len = len;
/*if (scsi_debug) printf("[L x%x]", len);*/

	/* xxx pop the cntrl byte away */
	if (get_fifo(regs) != 0)
		gimmeabreak();
	mb();

	/*
	 * Setup state
	 */
	asc->active_target = self;
	asc->done = SCSI_RET_IN_PROGRESS;
	asc->out_count = 0;
	asc->in_count = 0;
	asc->extra_count = 0;

	/*
	 * Sync params.
	 */
	regs->asc_syn_p = initiator->sync_period;
	regs->asc_syn_o = initiator->sync_offset;
	readback(regs->asc_syn_o);

	/*
	 * Do the up-call
	 */
	LOG(0xB,"tgt-mode-restart");
	(*self->dev_ops->restart)( self, FALSE);

	/* The call above has either prepared the data,
	   placing an ior on self, or it handled it some
	   other way */
	if (self->ior == 0)
		return FALSE;

	/* sanity */
	if (fifo_count(regs)) {
		regs->asc_cmd = ASC_CMD_FLUSH;
		readback(regs->asc_cmd);
	}

	/* needed by some dma code to align things properly */
	self->transient_state.cmd_count = sizeof(scsi_command_group_0);
	self->transient_state.in_count = len;	/* XXX */

	(*asc->dma_ops->map)(asc->dma_state, self);

	if (asc->wd.nactive++ == 0)
		asc->wd.watchdog_state = SCSI_WD_ACTIVE;


	{
		register script_t		scp;
		unsigned char			command;

		switch (self->dev_info.cpu.req_cmd) {
		case SCSI_CMD_TEST_UNIT_READY:
			scp = asc_script_t_data_in+1; break;
		case SCSI_CMD_SEND:
			scp = asc_script_t_data_in; break;
		default:
			scp = asc_script_t_data_out; break;
		}

		asc->script = scp;
		command = scp->command;
		if (!(*scp->action)(asc, csr, ir))
			return FALSE;

/*if (scsi_debug) printf("[F%x]", fifo_count(regs));*/

		asc->script++;
		regs->asc_cmd = command; mb();

	}

	return FALSE;
}		
		



/*
 * Error handlers
 */

/*
 * Fall-back error handler.
 */
int
asc_err_generic(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	LOG(0x13,"err_generic");

	/* handle non-existant or powered off devices here */
	if ((ir == ASC_INT_DISC) &&
	    (asc_isa_select(asc->cmd_was)) &&
	    (ASC_SS(asc->ss_was) == 0)) {
		/* Powered off ? */
		target_info_t	*tgt = asc->active_target;

		tgt->flags = 0;
		tgt->sync_period = 0;
		tgt->sync_offset = 0; mb();
		asc->done = SCSI_RET_DEVICE_DOWN;
		asc_end(asc, csr, ir);
		return;
	}

	switch (SCSI_PHASE(csr)) {
	case SCSI_PHASE_STATUS:
		if (asc->script[-1].condition == SCSI_PHASE_STATUS) {
			/* some are just slow to get out.. */
		} else
			asc_err_to_status(asc, csr, ir);
		return;
		break;
	case SCSI_PHASE_DATAI:
		if (asc->script->condition == SCSI_PHASE_STATUS) {
			/*
			 * Here is what I know about it.  We reconnect to
			 * the target (csr 87, ir 0C, cmd 44), start dma in
			 * (81 10 12) and then get here with (81 10 90).
			 * Dma is rolling and keeps on rolling happily,
			 * the value in the counter indicates the interrupt
			 * was signalled right away: by the time we get
			 * here only 80-90 bytes have been shipped to an
			 * rz56 running synchronous at 3.57 Mb/sec.
			 */
/*			printf("{P}");*/
			return;
		}
		break;
	case SCSI_PHASE_DATAO:
		if (asc->script->condition == SCSI_PHASE_STATUS) {
			/*
			 * See comment above. Actually seen on hitachis.
			 */
/*			printf("{P}");*/
			return;
		}
		break;
	case SCSI_PHASE_CMD:
		/* This can be the case with drives that are not
		   even scsi-1 compliant and do not like to be
		   selected with ATN (to do the synch negot) and
		   go stright to command phase, regardless  */

		if (asc->script == asc_script_try_synch) {

		    target_info_t	*tgt = asc->active_target;
		    register asc_padded_regmap_t	*regs = asc->regs;

		    tgt->flags |= TGT_DID_SYNCH;	/* only one chance */
		    tgt->flags &= ~TGT_TRY_SYNCH;

		    if (tgt->cur_cmd == SCSI_CMD_INQUIRY)
			tgt->transient_state.script = asc_script_data_in;
		    else
			tgt->transient_state.script = asc_script_simple_cmd;
		    asc->script = tgt->transient_state.script;
		    regs->asc_cmd = ASC_CMD_CLR_ATN;
		    readback(regs->asc_cmd);
		    asc->regs->asc_cmd = ASC_CMD_XFER_PAD|ASC_CMD_DMA; /*0x98*/ mb();
		    printf(" did not like SYNCH xfer ");
		    return;
		}
		/* fall through */
	}
	gimmeabreak();
}

/*
 * Handle generic errors that are reported as
 * an unexpected change to STATUS phase
 */
int
asc_err_to_status(
	register asc_softc_t	asc,
	int			csr,
	int			ir)
{
	script_t		scp = asc->script;

	LOG(0x14,"err_tostatus");
	while (scp->condition != SCSI_PHASE_STATUS)
		scp++;
	(*scp->action)(asc, csr, ir);
	asc->script = scp + 1;
	asc->regs->asc_cmd = scp->command; mb();
#if 0
	/*
	 * Normally, we would already be able to say the command
	 * is in error, e.g. the tape had a filemark or something.
	 * But in case we do disconnected mode WRITEs, it is quite
	 * common that the following happens:
	 *	dma_out -> disconnect (xfer complete) -> reconnect
	 * and our script might expect at this point that the dma
	 * had to be restarted (it didn't notice it was completed).
	 * And in any event.. it is both correct and cleaner to
	 * declare error iff the STATUS byte says so.
	 */
	asc->done = SCSI_RET_NEED_SENSE;
#endif
}

/*
 * Handle disconnections as exceptions
 */
int
asc_err_disconn(
	register asc_softc_t	asc,
	register unsigned char	csr,
	register unsigned char	ir)
{
	register asc_padded_regmap_t	*regs;
	register target_info_t	*tgt;
	int			count;
	boolean_t		callback = FALSE;

	LOG(0x16,"err_disconn");

	if (SCSI_PHASE(csr) != SCSI_PHASE_MSG_IN)
		return asc_err_generic(asc, csr, ir);

	regs = asc->regs;
	tgt = asc->active_target;

	switch (asc->script->condition) {
	case SCSI_PHASE_DATAO:
		LOG(0x1b,"+DATAO");
		if (asc->out_count) {
			register int xferred, offset;

			ASC_TC_GET(regs,xferred); /* temporary misnomer */
			mb();
			xferred += fifo_count(regs);
			xferred -= asc->extra_count;
			xferred = asc->out_count - xferred; /* ok now */
			tgt->transient_state.out_count -= xferred;
			assert(tgt->transient_state.out_count > 0);

			callback = (*asc->dma_ops->disconn_1)
					(asc->dma_state, tgt, xferred);

		} else {

			callback = (*asc->dma_ops->disconn_2)
					(asc->dma_state, tgt);

		}
		asc->extra_count = 0;
		tgt->transient_state.script = asc_script_restart_data_out;
		break;


	case SCSI_PHASE_DATAI:
		LOG(0x17,"+DATAI");
		if (asc->in_count) {
			register int offset, xferred;

			ASC_TC_GET(regs,count); mb();
			xferred = asc->in_count - count;
			assert(xferred > 0);

if (regs->asc_flags & 0xf)
printf("{Xf %x,%x,%x}", xferred, asc->in_count, fifo_count(regs));
			tgt->transient_state.in_count -= xferred;
			assert(tgt->transient_state.in_count > 0);

			callback = (*asc->dma_ops->disconn_3)
					(asc->dma_state, tgt, xferred);

			tgt->transient_state.script = asc_script_restart_data_in;
			if (tgt->transient_state.in_count == 0)
				tgt->transient_state.script++;

		}
		tgt->transient_state.script = asc->script;
		break;

	case SCSI_PHASE_STATUS:
		/* will have to restart dma */
		ASC_TC_GET(regs,count); mb();
		if (asc->state & ASC_STATE_DMA_IN) {
			register int offset, xferred;

			LOG(0x1a,"+STATUS+R");

			xferred = asc->in_count - count;
			assert(xferred > 0);

if (regs->asc_flags & 0xf)
printf("{Xf %x,%x,%x}", xferred, asc->in_count, fifo_count(regs));
			tgt->transient_state.in_count -= xferred;
/*			assert(tgt->transient_state.in_count > 0);*/

			callback = (*asc->dma_ops->disconn_4)
					(asc->dma_state, tgt, xferred);

			tgt->transient_state.script = asc_script_restart_data_in;
			if (tgt->transient_state.in_count == 0)
				tgt->transient_state.script++;

		} else {

			/* add what's left in the fifo */
			count += fifo_count(regs);
			/* take back the extra we might have added */
			count -= asc->extra_count;
			/* ..and drop that idea */
			asc->extra_count = 0;

			LOG(0x19,"+STATUS+W");


			if ((count == 0) && (tgt->transient_state.out_count == asc->out_count)) {
				/* all done */
				tgt->transient_state.script = asc->script;
				tgt->transient_state.out_count = 0;
			} else {
				register int xferred;

				/* how much we xferred */
				xferred = asc->out_count - count;

				tgt->transient_state.out_count -= xferred;
				assert(tgt->transient_state.out_count > 0);

				callback = (*asc->dma_ops->disconn_5)
						(asc->dma_state,tgt,xferred);

				tgt->transient_state.script = asc_script_restart_data_out;
			}
			asc->out_count = 0;
		}
		break;
	default:
		gimmeabreak();
		return;
	}
	asc_msg_in(asc,csr,ir);
	asc->script = asc_script_disconnect;
	regs->asc_cmd = ASC_CMD_XFER_INFO|ASC_CMD_DMA; mb();
	if (callback)
		(*asc->dma_ops->disconn_callback)(asc->dma_state, tgt);
}

/*
 * Watchdog
 *
 * So far I have only seen the chip get stuck in a
 * select/reselect conflict: the reselection did
 * win and the interrupt register showed it but..
 * no interrupt was generated.
 * But we know that some (name withdrawn) disks get
 * stuck in the middle of dma phases...
 */
int
asc_reset_scsibus(
	register asc_softc_t	asc)
{
	register target_info_t	*tgt = asc->active_target;
	register asc_padded_regmap_t	*regs = asc->regs;
	register int		ir;

	if (scsi_debug && tgt) {
		int dmalen;
		ASC_TC_GET(asc->regs,dmalen); mb();
		printf("Target %d was active, cmd x%x in x%x out x%x Sin x%x Sou x%x dmalen x%x\n", 
			tgt->target_id, tgt->cur_cmd,
			tgt->transient_state.in_count, tgt->transient_state.out_count,
			asc->in_count, asc->out_count,
			dmalen);
	}
	ir = get_reg(regs,asc_intr); mb();
	if ((ir & ASC_INT_RESEL) && (SCSI_PHASE(regs->asc_csr) == SCSI_PHASE_MSG_IN)) {
		/* getting it out of the woods is a bit tricky */
		spl_t	s = splbio();

		(void) asc_reconnect(asc, get_reg(regs,asc_csr), ir);
		asc_wait(regs, ASC_CSR_INT);
		ir = get_reg(regs,asc_intr); mb();
		regs->asc_cmd = ASC_CMD_MSG_ACPT;
		readback(regs->asc_cmd);
		splx(s);
	} else {
		regs->asc_cmd = ASC_CMD_BUS_RESET; mb();
		delay(35);
	}
}

#endif	NASC > 0

	/****************************************************/
#else	/* PARAGON860	osc1_3b11 code EGREGIOUS HACK XXXXX */
	/****************************************************/
	/* revision 1.2.4.2, from osc1_3b11 */

#include <mach_kdb.h>

#include <asc.h>
#if	NASC > 0
#include <platforms.h>

#ifdef	DECSTATION
#define	PAD(n)		char n[3]
#endif

#ifdef	PARAGON860
#define	check_memory(a,b)	0
#define	PAD(n)		char n
#define wbflush()
#define	mb()
#endif	/* PARAGON860 */

#include <machine/machparam.h>		/* spl definitions */
#include <mach/std_types.h>
#include <sys/types.h>
#include <chips/busses.h>
#include <scsi/compat_30.h>
#include <machine/machparam.h>

#include <scsi/scsi.h>
#include <scsi/scsi2.h>

#include <scsi/adapters/scsi_53C94.h>
#include <scsi/scsi_defs.h>
#include <scsi/adapters/scsi_dma.h>

#if	!MACH_KDB
#define	db_printf()
#define	db_printsym()
#endif


#ifdef	PAD
typedef struct {
	volatile unsigned char	asc_tc_lsb;	/* rw: Transfer Counter LSB */
	PAD(pad0);
	volatile unsigned char	asc_tc_msb;	/* rw: Transfer Counter MSB */
	PAD(pad1);
	volatile unsigned char	asc_fifo;	/* rw: FIFO top */
	PAD(pad2);
	volatile unsigned char	asc_cmd;	/* rw: Command */
	PAD(pad3);
	volatile unsigned char	asc_csr;	/*  r: Status */
/*#define		asc_dbus_id asc_csr	/*  w: Destination Bus ID */
	PAD(pad4);
	volatile unsigned char	asc_intr;	/*  r: Interrupt */
/*#define		asc_sel_timo asc_intr	/*  w:(re)select timeout */
	PAD(pad5);
	volatile unsigned char	asc_ss;		/*  r: Sequence Step */
/*#define		asc_syn_p asc_ss	/*  w: Synchronous period */
	PAD(pad6);
	volatile unsigned char	asc_flags;	/*  r: FIFO flags + seq step */
/*#define		asc_syn_o asc_flags	/*  w: Synchronous offset */
	PAD(pad7);
	volatile unsigned char	asc_cnfg1;	/* rw: Configuration 1 */
	PAD(pad8);
	volatile unsigned char	asc_ccf;	/*  w: Clock Conv. Factor */
	PAD(pad9);
	volatile unsigned char	asc_test;	/*  w: Test Mode */
	PAD(pad10);
	volatile unsigned char	asc_cnfg2;	/* rw: Configuration 2 */
	PAD(pad11);
	volatile unsigned char	asc_cnfg3;	/* rw: Configuration 3 */
	PAD(pad12);
	volatile unsigned char	asc_hole1;	/*     Hole in ASC register map */
/*#define 		fsc_cnfg4 asc_hole1	/* rw: FSC Configuration 4 */
	PAD(pad13);
	volatile unsigned char	asc_hole2;	/*     Hole in ASC register map */
/*#define 		fas_tc_hi asc_hole2	/* rw: FAS Transfer Counter High */
	PAD(pad14);
	volatile unsigned char	asc_rfb;	/*  w: Reserve FIFO byte */
	PAD(pad15);
} asc_padded_regmap_t;

#else	/* !PAD */

typedef asc_regmap_t	asc_padded_regmap_t;

#endif	/* !PAD */

#define	get_reg(r,x)	((unsigned char)((r)->x))

#define	fifo_count(r)	((r)->asc_flags & ASC_FLAGS_FIFO_CNT)
#define	get_fifo(r)	get_reg(r,asc_fifo)

/*
 * A script has a three parts: a pre-condition, an action, and
 * an optional command to the chip.  The first triggers error
 * handling if not satisfied and in our case it is a match
 * of the expected and actual scsi-bus phases.
 * The action part is just a function pointer, and the
 * command is what the 53c90 should be told to do at the end
 * of the action processing.  This command is only issued and the
 * script proceeds if the action routine returns TRUE.
 * See asc_intr() for how and where this is all done.
 */

typedef struct script {
	unsigned char	condition;	/* expected state at interrupt */
	unsigned char	command;	/* command to the chip */
	unsigned short	flags;		/* unused padding */
	boolean_t	(*action)();	/* extra operations */
} *script_t;

/* Matching on the condition value */
#define	ANY				0xff
#define	SCRIPT_MATCH(csr,ir,value)	(((SCSI_PHASE(csr)==(value)) &&     \
					  !((ir)&(ASC_INT_DISC))) || 	    \
					 (((value)==ANY) &&		    \
					  ((ir)&(ASC_INT_DISC|ASC_INT_FC))) \
					)

/* When no command is needed */
#define	SCRIPT_END	-1

/* forward decls of script actions */
boolean_t
	asc_end(),			/* all come to an end */
	asc_clean_fifo(),		/* .. in preparation for status byte */
	asc_get_status(),		/* get status from target */
	asc_dma_in(),			/* get data from target via dma */
	asc_dma_in_r(),			/* get data from target via dma (restartable)*/
	asc_dma_out(),			/* send data to target via dma */
	asc_dma_out_r(),		/* send data to target via dma (restartable) */
	asc_dosynch(),			/* negotiate synch xfer */
	asc_msg_in(),			/* receive the disconenct message */
	asc_disconnected(),		/* target has disconnected */
	asc_reconnect();		/* target reconnected */

/* forward decls of error handlers */
boolean_t
	asc_err_generic(),		/* generic handler */
	asc_err_disconn(),		/* target disconnects amidst */
	gimmeabreak();			/* drop into the debugger */

int	asc_reset_scsibus();
boolean_t asc_probe_target();
static	asc_wait();

/*
 * State descriptor for this layer.  There is one such structure
 * per (enabled) SCSI-53c90 interface
 */
struct asc_softc {
	struct watchdog_t	wd;
	asc_padded_regmap_t	*regs;		/* 53c90 registers */

	scsi_dma_ops_t	*dma_ops;	/* DMA operations and state */
	opaque_t	dma_state;

	script_t	script;		/* what should happen next */
	boolean_t	(*error_handler)();/* what if something is wrong */
	int		in_count;	/* amnt we expect to receive */
	int		out_count;	/* amnt we are going to ship */

	volatile char	state;
#define	ASC_STATE_BUSY		0x01	/* selecting or currently connected */
#define ASC_STATE_TARGET	0x04	/* currently selected as target */
#define ASC_STATE_COLLISION	0x08	/* lost selection attempt */
#define ASC_STATE_DMA_IN	0x10	/* tgt --> initiator xfer */
#define	ASC_STATE_SPEC_DMA	0x20	/* special, 8 byte threshold dma */
#define	ASC_STATE_FAS_CNTL	0x40	/* Emulex FAS SCSI controller */
#define	ASC_STATE_FSC_CNTL	0x80	/* NCR FSC SCSI controller */

	unsigned char	ntargets;	/* how many alive on this scsibus */
	unsigned char	done;
	unsigned char	extra_count;	/* sleazy trick to spare an interrupt */

	scsi_softc_t	*sc;		/* HBA-indep info */
	target_info_t	*active_target;	/* the current one */

	target_info_t	*next_target;	/* trying to seize bus */
	queue_head_t	waiting_targets;/* other targets competing for bus */

	int		clock_mhz;	/* chip clock frequency in MHz */

	unsigned char	ss_was;		/* districate powered on/off devices */
	unsigned char	cmd_was;

} asc_softc_data[NASC];

typedef struct asc_softc *asc_softc_t;

asc_softc_t	asc_softc[NASC];

/*
 * Synch xfer parameters, and timing conversions
 */
int	asc_min_period	= 5;		/* in CLKS/BYTE */
int	fas_min_period	= 4;		/* in CLKS/BYTE */
int	asc_max_offset	= 15;		/* max sync offset */
int	asc_spec_dma_offset = 7;	/* max sync offset with special DMA */

#define asc_to_scsi_period(a)	(a * 250 / asc->clock_mhz)
#define scsi_period_to_asc(p)	(p * asc->clock_mhz / 250)

#define readback(a)	{register int foo; foo = a;}

#define	u_min(a,b)	(((a) < (b)) ? (a) : (b))

/*
 * Definition of the controller for the auto-configuration program.
 */

extern int asc_probe(
	unsigned	reg,
	struct bus_ctlr	*ui);

extern void asc_go(
	target_info_t		*tgt,
        unsigned int		cmd_count,
        unsigned int		in_count,
	boolean_t		cmd_only);

caddr_t	asc_std[NASC] = { 0 };
struct	bus_device *asc_dinfo[NASC*8];
struct	bus_ctlr *asc_minfo[NASC];
struct	bus_driver asc_driver = {
        ((int (*)(			/* Test for driver		*/
		caddr_t    port,
		void *ui)) asc_probe),
	((int (*)(			/* Test for slave		*/
		struct bus_device *device,
		caddr_t	virt)) scsi_slave),
  	((void (*)(			/* Setup driver after probe	*/
		struct bus_device *device)) scsi_attach),
	((int (*)(void))asc_go),	/* Start transfer		*/
  	asc_std,			/* Device CSR addresses		*/
  	"rz",				/* Device name			*/
  	asc_dinfo,			/* Backpointer to init structs	*/
	"asc",				/* Controller name		*/
	asc_minfo,			/* Backpointer to init structs	*/
	BUS_INTR_B4_PROBE};		/* Flags			*/


asc_set_dmaops(unit, dmaops)
	unsigned int	unit;
	scsi_dma_ops_t	*dmaops;
{
	if (unit < NASC)
		asc_std[unit] = (caddr_t)dmaops;
}

/*
 * Scripts
 */
struct script
asc_script_data_in[] = {	/* started with SEL & DMA */
	{SCSI_PHASE_DATAI, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_in},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_data_out[] = {	/* started with SEL & DMA */
	{SCSI_PHASE_DATAO, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_out},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_try_synch[] = {
	{SCSI_PHASE_MSG_OUT, ASC_CMD_I_COMPLETE,0, asc_dosynch},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_simple_cmd[] = {
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_disconnect[] = {
	{ANY, ASC_CMD_ENABLE_SEL, 0, asc_disconnected},
/**/	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_reconnect}
},

asc_script_restart_data_in[] = { /* starts after disconnect */
	{SCSI_PHASE_DATAI, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_in_r},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
},

asc_script_restart_data_out[] = { /* starts after disconnect */
	{SCSI_PHASE_DATAO, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_out_r},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
};

#if	documentation
/*
 * This is what might happen during a read
 * that disconnects
 */
asc_script_data_in_wd[] = { /* started with SEL & DMA & allow disconnect */
	{SCSI_PHASE_MSG_IN, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_msg_in},
	{ANY, ASC_CMD_ENABLE_SEL, 0, asc_disconnected},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_reconnect},
	{SCSI_PHASE_DATAI, ASC_CMD_XFER_INFO|ASC_CMD_DMA, 0, asc_dma_in},
	{SCSI_PHASE_STATUS, ASC_CMD_I_COMPLETE, 0, asc_clean_fifo},
	{SCSI_PHASE_MSG_IN, ASC_CMD_MSG_ACPT, 0, asc_get_status},
	{ANY, SCRIPT_END, 0, asc_end}
};
#endif
#ifndef	PARAGON860
#define DEBUG
#else	/* PARAGON860 */
#define DEBUG
#endif	/* PARAGON860 */
#ifdef	DEBUG

#define	PRINT(x)	if (scsi_debug) printf x

asc_state(regs)
	asc_padded_regmap_t	*regs;
{
	register unsigned char ff,csr,ir,d0,d1,cmd;

	if (regs == 0) {
		if (asc_softc[0])
			regs = asc_softc[0]->regs;
		else
#ifdef	PARAGON860
			regs = (asc_padded_regmap_t*)0x91000000;
#else	/* PARAGON860 */
			regs = (asc_padded_regmap_t*)0xbf400000;
#endif	/* PARAGON860 */
	}
	ff = get_reg(regs,asc_flags);
	csr = get_reg(regs,asc_csr);
/*	ir = regs->asc_intr;	nope, clears interrupt */
	d0 = get_reg(regs,asc_tc_lsb);
	d1 = get_reg(regs,asc_tc_msb);
	cmd = get_reg(regs,asc_cmd);

#if	PARAGON860
	printf("\n");
	printf("Command  . . . . . . . 0x%02x\n", cmd);
	printf("Transfer Counter . . . 0x%04x (%d)\n",
		(d1 << 8) | d0, (d1 << 8) | d0);
	printf("FIFO Count . . . . . . 0x%02x (%d)\n",
		ff & ASC_FLAGS_FIFO_CNT, ff & ASC_FLAGS_FIFO_CNT);
	printf("Sequence Step  . . . . 0x%02x\n", ff >> 5);
	printf("Status . . . . . . . . 0x%02x\n", csr);
	if (csr & ASC_CSR_INT) printf("    Interrupt\n");
	if (csr & ASC_CSR_GE)  printf("    Gross Error\n");
	if (csr & ASC_CSR_PE)  printf("    Parity Error\n");
	if (csr & ASC_CSR_TC)  printf("    Terminal Count\n");
	if (csr & ASC_CSR_VGC) printf("    Valid Group Code\n");
	csr = SCSI_PHASE(csr);
	printf("    Bus Phase: ");
	switch (csr) {
		case SCSI_PHASE_DATAO:
			printf("Bus Free or Data Out");
			break;

		case SCSI_PHASE_DATAI:
			printf("Data In");
			break;

		case SCSI_PHASE_CMD:
			printf("Command");
			break;

		case SCSI_PHASE_STATUS:
			printf("Status");
			break;

		case SCSI_PHASE_MSG_OUT:
			printf("Message Out");
			break;

		case SCSI_PHASE_MSG_IN:
			printf("Message In");
			break;

		default:
			printf("ANSI RESERVED!");
			break;
	}
	printf("\n");
#else	/* PARAGON860 */
	printf("dma %x ff %x csr %x cmd %x\n",
		(d1 << 8) | d0, ff, csr, cmd);
#endif	/* PARAGON860 */
	return 0;
}

asc_target_state(tgt)
	target_info_t	*tgt;
{
	if (tgt == 0)
		tgt = asc_softc[0]->active_target;
	if (tgt == 0)
		return 0;
	db_printf("@x%x: fl %x dma %X+%x cmd %x@%X id %x per %x off %x ior %X ret %X\n",
	
		tgt,
		tgt->flags, tgt->dma_ptr, tgt->transient_state.dma_offset,
		tgt->cur_cmd, tgt->cmd_ptr, tgt->target_id, tgt->sync_period,
		(tgt->sync_offset & ASC_SYNO_MASK), tgt->ior, tgt->done);
	if (tgt->flags & TGT_DISCONNECTED){
		script_t	spt;

		spt = tgt->transient_state.script;
		db_printf("disconnected at ");
		db_printsym(spt,1);
		db_printf(": %x %x ", spt->condition, spt->command);
		db_printsym(spt->action,1);
		db_printf(", ");
		db_printsym(tgt->transient_state.handler, 1);
		db_printf("\n");
	}

	return 0;
}

asc_all_targets(unit)
{
	int i;
	target_info_t	*tgt;
	for (i = 0; i < MAX_SCSI_TARGETS; i++) {
		tgt = asc_softc[unit]->sc->target[i];
		if (tgt)
			asc_target_state(tgt);
	}
}

asc_script_state(unit)
{
	script_t	spt = asc_softc[unit]->script;

	if (spt == 0) return 0;
	db_printsym(spt,1);
	db_printf(": %x %x ", spt->condition, spt->command);
	db_printsym(spt->action,1);
	db_printf(", ");
	db_printsym(asc_softc[unit]->error_handler, 1);
	return 0;
}

#define tr asc_tr
#define TRMAX 200
int tr[TRMAX+3];
int trpt, trpthi;
#define	TR(x)	tr[trpt++] = x
#define TRWRAP	trpthi = trpt; trpt = 0;
#define TRCHECK	if (trpt > TRMAX) {TRWRAP}

#define TRACE

#ifdef TRACE

#define LOGSIZE 256
int asc_logpt;
char asc_log[LOGSIZE];

#define MAXLOG_VALUE	0x1e
struct {
	char *name;
	unsigned int count;
} logtbl[MAXLOG_VALUE];

static LOG(e,f)
	char *f;
{
	asc_log[asc_logpt++] = (e);
	if (asc_logpt == LOGSIZE) asc_logpt = 0;
	if ((e) < MAXLOG_VALUE) {
		logtbl[(e)].name = (f);
		logtbl[(e)].count++;
	}
}

asc_print_log(skip)
	int skip;
{
	register int i, j;
	register unsigned char c;

	for (i = 0, j = asc_logpt; i < LOGSIZE; i++) {
		c = asc_log[j];
		if (++j == LOGSIZE) j = 0;
		if (skip-- > 0)
			continue;
		if (c < MAXLOG_VALUE)
			db_printf(" %s", logtbl[c].name);
		else
			db_printf("-x%x", c & 0x7f);
	}
}

asc_print_stat()
{
	register int i;
	register char *p;
	for (i = 0; i < MAXLOG_VALUE; i++) {
		if (p = logtbl[i].name)
			printf("%d %s\n", logtbl[i].count, p);
	}
}

#else	/*TRACE*/
#define	LOG(e,f)
#define LOGSIZE
#endif	/*TRACE*/

#else	/*DEBUG*/
#define PRINT(x)
#define	LOG(e,f)
#define LOGSIZE

#define TR(x)
#define TRWRAP	;
#define TRCHECK
#endif	/*DEBUG*/


/*
 *	Probe/Slave/Attach functions
 */

/*
 * Probe routine:
 *	Should find out (a) if the controller is
 *	present and (b) which/where slaves are present.
 *
 * Implementation:
 *	Send a test-unit-ready to each possible target on the bus
 *	except of course ourselves.
 */
asc_probe(reg, ui)
	unsigned int	reg;
	struct bus_ctlr	*ui;
{
	int             unit = ui->unit;
	asc_softc_t     asc = &asc_softc_data[unit];
	int		target_id;
	scsi_softc_t	*sc;
	register asc_padded_regmap_t	*regs;
	int		s;
	boolean_t	did_banner = FALSE;
	unsigned char	csr, ir;

#ifdef	PARAGON860
	if ((reg = (unsigned int)init_asc_probe(reg, ui)) == 0) return 0;
#endif	/* PARAGON860 */

	/*
	 * We are only called if the right board is there,
	 * but make sure anyways..
	 */
	if (check_memory(reg, 0))
		return 0;

#ifdef	MACH_KERNEL
#ifndef	PARAGON860
	/* Mappable version side */
	ASC_probe(reg, ui);
#endif	/* PARAGON860 */
#endif	MACH_KERNEL

	/*
	 * Initialize hw descriptor, cache some pointers
	 */
	asc_softc[unit] = asc;
	asc->regs = (asc_padded_regmap_t *) (reg);

	if ((asc->dma_ops = (scsi_dma_ops_t *)asc_std[unit]) == 0)
		/* use same as unit 0 if undefined */
		asc->dma_ops = (scsi_dma_ops_t *)asc_std[0];
	{
		int dma_bsize = 16;	/* bits, preferred */
		asc->dma_state = (*asc->dma_ops->init)(unit, reg, &dma_bsize);
		if (dma_bsize > 16)
			asc->state |= ASC_STATE_SPEC_DMA;
	}

	queue_init(&asc->waiting_targets);

	sc = scsi_master_alloc(unit, (char *)asc);
	asc->sc = sc;

	sc->go = asc_go;
	sc->watchdog = scsi_watchdog;
	sc->probe = asc_probe_target;
	asc->wd.reset = ((void (*)(
			struct watchdog_t	*wd))asc_reset_scsibus);

#ifdef	MACH_KERNEL
	sc->max_dma_data = ASC_TC_MAX;
#else
	sc->max_dma_data = scsi_per_target_virtual;
#endif

	regs = asc->regs;

	/*
	 * Reset chip, fully.  Note that interrupts are already enabled.
	 */
	s = splbio();
#ifdef	PARAGON860
	asc_reset(asc, FALSE, asc->state & ASC_STATE_SPEC_DMA);

	/*
	 * some targets take a long time to respond after a bus reset
	 */
	delay(scsi_delay_after_reset);
#else	/* PARAGON860 */
	asc_reset(asc, TRUE, asc->state & ASC_STATE_SPEC_DMA);
#endif	/* PARAGON860 */

	csr = asc_wait(regs, ASC_CSR_INT);
	ir = regs->asc_intr;

	/*
	 * Test for SCSI controller presence by fiddling with ASC FIFO
	 */
	regs->asc_cmd = ASC_CMD_FLUSH;	/* empty fifo */
	delay(2);

	regs->asc_fifo = 0x55;
	regs->asc_fifo = 0xaa;

	if ((regs->asc_flags != 2)    ||
	    (regs->asc_fifo  != 0x55) ||
	    (regs->asc_fifo  != 0xaa)) {
		splx(s);
		return 0;
	}

	/*
	 * Our SCSI id on the bus.
	 * The user can set this via the prom on 3maxen/pmaxen.
	 * If this changes it is easy to fix: make a default that
	 * can be changed as boot arg.
	 */
#ifdef	PARAGON860
	regs->asc_cnfg1 = (regs->asc_cnfg1 & ~ASC_CNFG1_MY_BUS_ID) |
			      (scsi_initiator_id[unit] & 0x7);
	sc->initiator_id = regs->asc_cnfg1 & ASC_CNFG1_MY_BUS_ID;
	printf("%s%d: %s SCSI Controller at id %d", ui->name, unit,
		(asc->state & ASC_STATE_FAS_CNTL) ? "Emulex FAS236" :
		(asc->state & ASC_STATE_FSC_CNTL) ? "NCR 53CF96" : "NCR 53C96",
		sc->initiator_id);
#else	/* PARAGON860 */
#ifdef	unneeded
	regs->asc_cnfg1 = (regs->asc_cnfg1 & ~ASC_CNFG1_MY_BUS_ID) |
			      (scsi_initiator_id[unit] & 0x7);
#endif
	sc->initiator_id = regs->asc_cnfg1 & ASC_CNFG1_MY_BUS_ID;
	printf("%s%d: SCSI id %d", ui->name, unit, sc->initiator_id);
#endif	/* PARAGON860 */

	/*
	 * For all possible targets, see if there is one and allocate
	 * a descriptor for it if it is there.
	 */
	for (target_id = 0; target_id < MAX_SCSI_TARGETS; target_id++) {
		register unsigned char	ss, ff;
		register scsi_status_byte_t	status;

		/* except of course ourselves */
		if (target_id == sc->initiator_id)
			continue;

		regs->asc_cmd = ASC_CMD_FLUSH;	/* empty fifo */
		delay(2);

		regs->asc_dbus_id = target_id;

		/*
		 * See if the unit is ready.
		 * XXX SHOULD inquiry LUN 0 instead !!!
		 */
		regs->asc_fifo = SCSI_CMD_TEST_UNIT_READY;
		regs->asc_fifo = 0;
		regs->asc_fifo = 0;
		regs->asc_fifo = 0;
		regs->asc_fifo = 0;
		regs->asc_fifo = 0;

		/* select and send it */
		regs->asc_cmd = ASC_CMD_SEL;

		/* wait for the chip to complete, or timeout */
		csr = asc_wait(regs, ASC_CSR_INT);
		ss = get_reg(regs,asc_ss);
		ir = get_reg(regs,asc_intr);

		/* empty fifo, there is garbage in it if timeout */
		regs->asc_cmd = ASC_CMD_FLUSH;
		delay(2);

		/*
		 * Check if the select timed out
		 */
		if ((ASC_SS(ss) == 0) && (ir == ASC_INT_DISC))
			/* no one out there */
			continue;

		if (SCSI_PHASE(csr) != SCSI_PHASE_STATUS) {
#ifdef	PARAGON860
			printf("%signoring target %d",
				did_banner ? ", " : " ", target_id);
#else	/* PARAGON860 */
			printf( " %s%d%s", "ignoring target at ", target_id,
				" cuz it acts weirdo");
#endif	/* PARAGON860 */
			continue;
		}

#ifdef	PARAGON860
	printf("%s%d", did_banner++ ? ", " : " target(s) at ",
		target_id);
#else	/* PARAGON860 */
		printf(",%s%d", did_banner++ ? " " : " target(s) at ",
				target_id);
#endif	/* PARAGON860 */

		regs->asc_cmd = ASC_CMD_I_COMPLETE;
		wbflush();
		csr = asc_wait(regs, ASC_CSR_INT);
		ir = get_reg(regs,asc_intr); /* ack intr */


		status.bits = get_fifo(regs); /* empty fifo */
		ff = get_fifo(regs);

		if (status.st.scsi_status_code != SCSI_ST_GOOD)
			scsi_error( 0, SCSI_ERR_STATUS, status.bits, 0);

		regs->asc_cmd = ASC_CMD_MSG_ACPT;
		csr = asc_wait(regs, ASC_CSR_INT);
		ir = get_reg(regs,asc_intr); /* ack intr */

		/*
		 * Found a target
		 */
		asc->ntargets++;
		{
			register target_info_t	*tgt;
			tgt = scsi_slave_alloc(sc->masterno,
					       target_id, (char *) asc);

#ifndef PARAGON860 /* performance mod */
			(*asc->dma_ops->new_target)(asc->dma_state, tgt);
#endif
		}
	}
	printf(".\n");
#ifdef PARAGON860 /* performance mod */
        /* get the chip type, if it's an old NCR or the bootmagic is
         * is set to force 64k, set asc->ntargets to 7 so
         * asc->dma_ops->new_target uses 64k as the targets buffer size.
         */

        {
                unsigned char code = regs->fas_tc_hi;
                int ntargets;

        if (getbootint("BOOT_SCSI_FORCE_64K", 0) ||
                !(asc->state & (ASC_STATE_FAS_CNTL | ASC_STATE_FSC_CNTL)))
                        ntargets = 7;
        else
                ntargets = asc->ntargets;

        /* loop through all possible target ids checking for TGT_ALIVE.
         * When found call asc->dma_ops->new_target to allocate the
         * targets buffer space.
         */
        for (target_id = 0; target_id < MAX_SCSI_TARGETS; target_id++) {
                        register target_info_t  *tgt;
                if (target_id == sc->initiator_id)
                        continue;
                tgt = sc->target[target_id];
                if(!tgt || !(tgt->flags&TGT_ALIVE))
                        continue;
                (*asc->dma_ops->new_target)(asc->dma_state, tgt,ntargets);
        }
        }
#endif
	splx(s);
	return 1;
}

boolean_t
asc_probe_target(tgt, ior)
	target_info_t		*tgt;
	io_req_t		ior;
{
	asc_softc_t     asc = asc_softc[tgt->masterno];
	boolean_t	newlywed;

/* MIO alert! don't do this becuase the memory has all been allocated */
	return FALSE;

	newlywed = (tgt->cmd_ptr == 0);
	if (newlywed) {
		(*asc->dma_ops->new_target)(asc->dma_state, tgt);
	}

	if (scsi_inquiry(tgt, SCSI_INQ_STD_DATA) == SCSI_RET_DEVICE_DOWN)
		return FALSE;

	tgt->flags = TGT_ALIVE;
	return TRUE;
}

static asc_wait(regs, until)
	asc_padded_regmap_t	*regs;
{
	int timeo = 1000000;

	while ((regs->asc_csr & until) == 0) {
		delay(1);
		if (!timeo--) {
			printf("\nasc_wait timer expired: csr 0x%02x\n",
			       get_reg(regs,asc_csr));
			break;
		}
	}

	return get_reg(regs,asc_csr);
}

asc_reset(asc, quick, special_dma)
	asc_softc_t	asc;
{
	asc_padded_regmap_t	*regs;
	char		my_id;
#ifdef	PARAGON860
	unsigned char	code;
#endif	/* PARAGON860 */

	regs = asc->regs;

	/* preserve our ID for now */
	my_id = (regs->asc_cnfg1 & ASC_CNFG1_MY_BUS_ID);

	/*
	 * Reset chip and wait till done
	 */
	regs->asc_cmd = ASC_CMD_RESET;
	wbflush(); delay(25);

	/* spec says this is needed after reset */
	regs->asc_cmd = (ASC_CMD_NOP|ASC_CMD_DMA);
	wbflush(); delay(25);

#ifdef	PARAGON860
	/*
	 * Check for Emulex FAS SCSI Controller
	 */
	regs->asc_cnfg2 = FAS_CNFG2_FEATURES;
	delay(2);
	regs->asc_cmd = (ASC_CMD_NOP|ASC_CMD_DMA);
	delay(2);
	code = regs->fas_tc_hi;
	delay(2);

	if ((FAMILY(code) == FAS_FAMILY_CODE) ||
	    (FAMILY(code) == FSC_FAMILY_CODE)) {

		/*
		 * Emulex FAS and NCR FSC Initilaization
		 */

		asc->state |= (FAMILY(code) == FAS_FAMILY_CODE) ?
			ASC_STATE_FAS_CNTL : ASC_STATE_FSC_CNTL;

#if	0
		printf("%s Series SCSI Controller Version %d\n",
		    (FAMILY(code)==FAS_FAMILY_CODE) ? "Emulex FAS" : "NCR FSC",
		    (FAMILY(code)==FAS_FAMILY_CODE) ? REV(code)+1  : REV(code));
#endif

		/*
		 * Set up various chip parameters
		 */
		asc->clock_mhz = 40;		/* chip clock freq in MHz */
		regs->asc_ccf = FAS_CCF_40MHz;	/* 35.01 MHz to 40 MHz clock */
		delay(25);
		regs->asc_sel_timo = ASC_TIMEOUT_250;
		regs->asc_syn_p = fas_min_period;
		regs->asc_syn_o = 0;	/* asynch for now */

		regs->asc_cnfg2 = FAS_CNFG2_FEATURES | ASC_CNFG2_SCSI2;

		/*
		 * Chip bug in version 3 of the Emulex controller prevents
		 * us from using the Threshold 8 bit.  Ignore special_dma.
		 */
		asc->state &= ~ASC_STATE_SPEC_DMA;	/* work around */
		special_dma = FALSE;			/* work around */

		regs->asc_cnfg3 = special_dma ? ASC_CNFG3_T8 : 0 |
			FAS_CNFG3_FASTSCSI | FAS_CNFG3_FASTCLK;

		/*
		 * Set NCR TolerANT technology bit for higher noise margin
		 */
		if (FAMILY(code) == FSC_FAMILY_CODE)
			regs->fsc_cnfg4 = FSC_CNFG4_EAN;

		regs->fas_tc_hi = 0;	/* clear transfer counter high */

	} else {

		/*
		 * NCR Initialization
		 */

		asc->state &= ~(ASC_STATE_FAS_CNTL | ASC_STATE_FSC_CNTL);
#if	0
		printf("NCR 53C90 Series SCSI Controller\n");
#endif

		/*
		 * Set up various chip parameters
		 */
		asc->clock_mhz = 17;	/* (rounded) chip clock freq in MHz */
		regs->asc_ccf = ASC_CCF_20MHz;	/* 15.01 MHz to 20 MHz clock */
		delay(25);

		/*
		 * Only running NCR chip at 16.667 MHz
		 * Must adjust SCSI bus timeout accordingly
		 *
		 * regs->asc_sel_timo = ASC_TIMEOUT_250;
		 */
		regs->asc_sel_timo = 0x88;	/* timeout at 16.667 MHz */
		regs->asc_syn_p = asc_min_period;
		regs->asc_syn_o = 0;	/* asynch for now */

		regs->asc_cnfg2 = /* ASC_CNFG2_EPL | */ ASC_CNFG2_SCSI2;
		regs->asc_cnfg3 = special_dma ? ASC_CNFG3_T8 : 0;
	}

	/* restore our ID */
	regs->asc_cnfg1 = my_id | ASC_CNFG1_P_CHECK;

	/* zero anything else */
	ASC_TC_PUT(regs, 0);

#else	/* PARAGON860 */

	/*
	 * Set up various chip parameters
	 */
	asc->clock_mhz = 25;		/* chip clock frequency in MHz */
	regs->asc_ccf = ASC_CCF_25MHz;	/* 25 MHz clock */
	wbflush();
	delay(25);
	regs->asc_sel_timo = ASC_TIMEOUT_250;
	/* restore our ID */
	regs->asc_cnfg1 = my_id | ASC_CNFG1_P_CHECK;
	regs->asc_cnfg2 = /* ASC_CNFG2_EPL | */ ASC_CNFG2_SCSI2;
	regs->asc_cnfg3 = special_dma ? (ASC_CNFG3_T8|ASC_CNFG3_ALT_DMA) : 0;
	/* zero anything else */
	ASC_TC_PUT(regs, 0);
	regs->asc_syn_p = asc_min_period;
	regs->asc_syn_o = 0;	/* asynch for now */

#endif	/* PARAGON860 */

	if (quick) return;

	/*
	 * reset the scsi bus, the interrupt routine does the rest
	 * or you can call sii_bus_reset().
	 */
	regs->asc_cmd = ASC_CMD_BUS_RESET;
	delay(35);
}

/*
 *	Operational functions
 */

/*
 * Start a SCSI command on a target
 */
void
asc_go(target_info_t		*tgt,
       unsigned int		cmd_count,
       unsigned int		in_count,
       boolean_t		cmd_only)
{
	asc_softc_t	asc;
	register int	s;
	boolean_t	disconn;
	script_t	scp;
	boolean_t	(*handler)();

	LOG(1,"go");

	asc = (asc_softc_t)tgt->hw_state;

	tgt->transient_state.cmd_count = cmd_count; /* keep it here */

	(*asc->dma_ops->map)(asc->dma_state, tgt);

	disconn  = BGET(scsi_might_disconnect,tgt->masterno,tgt->target_id);
	disconn  = disconn && (asc->ntargets > 1);
	disconn |= BGET(scsi_should_disconnect,tgt->masterno,tgt->target_id);

	/*
	 * Setup target state
	 */
	tgt->done = SCSI_RET_IN_PROGRESS;

	handler = (disconn) ? asc_err_disconn : asc_err_generic;

	switch (tgt->cur_cmd) {
	    case SCSI_CMD_READ:
	    case SCSI_CMD_LONG_READ:
		LOG(2,"readop");
		scp = asc_script_data_in;
		break;
	    case SCSI_CMD_WRITE:
	    case SCSI_CMD_LONG_WRITE:
		LOG(0x1a,"writeop");
		scp = asc_script_data_out;
		break;
	    case SCSI_CMD_INQUIRY:
		/* This is likely the first thing out:
		   do the synch neg if so */
		if (!cmd_only && ((tgt->flags&TGT_DID_SYNCH)==0)) {
			scp = asc_script_try_synch;
			tgt->flags |= TGT_TRY_SYNCH;
			break;
		}
	    case SCSI_CMD_REQUEST_SENSE:
	    case SCSI_CMD_MODE_SENSE:
	    case SCSI_CMD_LOG_SENSE:
	    case SCSI_CMD_RECEIVE_DIAG_RESULTS:
	    case SCSI_CMD_READ_CAPACITY:
	    case SCSI_CMD_READ_BLOCK_LIMITS:
	    case SCSI_CMD_READ_POSITION:
		scp = asc_script_data_in;
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    case SCSI_CMD_MODE_SELECT:
	    case SCSI_CMD_REASSIGN_BLOCKS:
	    case SCSI_CMD_FORMAT_UNIT:
            case SCSI_CMD_LOAD_DISPLAY:
		tgt->transient_state.cmd_count = sizeof(scsi_command_group_0);
		tgt->transient_state.out_count = cmd_count - sizeof(scsi_command_group_0);
		scp = asc_script_data_out;
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    case SCSI_CMD_TEST_UNIT_READY:
		/*
		 * Do the synch negotiation here, unless prohibited
		 * or done already
		 */
		if (tgt->flags & TGT_DID_SYNCH) {
			scp = asc_script_simple_cmd;
		} else {
			scp = asc_script_try_synch;
			tgt->flags |= TGT_TRY_SYNCH;
			cmd_only = FALSE;
		}
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    default:
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		scp = asc_script_simple_cmd;
	}

	tgt->transient_state.script = scp;
	tgt->transient_state.handler = handler;
	tgt->transient_state.identify = (cmd_only) ? 0xff :
		(disconn ? SCSI_IDENTIFY|SCSI_IFY_ENABLE_DISCONNECT :
			   SCSI_IDENTIFY);
	tgt->transient_state.identify |= tgt->lun & SCSI_IFY_LUN_MASK;

	if (in_count)
		tgt->transient_state.in_count =
			(in_count < tgt->block_size) ? tgt->block_size : in_count;
	else
		tgt->transient_state.in_count = 0;

	/*
	 * See if another target is currently selected on
	 * this SCSI bus, e.g. lock the asc structure.
	 * Note that it is the strategy routine's job
	 * to serialize ops on the same target as appropriate.
	 * XXX here and everywhere, locks!
	 */
	/*
	 * Protection viz reconnections makes it tricky.
	 */
	s = splbio();

	if (asc->wd.nactive++ == 0)
		asc->wd.watchdog_state = SCSI_WD_ACTIVE;

	if (asc->state & ASC_STATE_BUSY) {
		/*
		 * Queue up this target, note that this takes care
		 * of proper FIFO scheduling of the scsi-bus.
		 */
		LOG(3,"enqueue");
		enqueue_tail(&asc->waiting_targets, (queue_entry_t) tgt);
	} else {
		/*
		 * It is down to at most two contenders now,
		 * we will treat reconnections same as selections
		 * and let the scsi-bus arbitration process decide.
		 */
		asc->state |= ASC_STATE_BUSY;
		asc->next_target = tgt;
		asc_attempt_selection(asc);
		/*
		 * Note that we might still lose arbitration..
		 */
	}
	splx(s);
}

asc_attempt_selection(asc)
	asc_softc_t	asc;
{
	target_info_t	*tgt;
	asc_padded_regmap_t	*regs;
	register int	out_count;
#ifdef	PARAGON860		/* Emulex workaround */
	register int	i;
#endif	/* PARAGON860 */

	regs = asc->regs;
	tgt = asc->next_target;

	LOG(4,"select");
	LOG(0x80+tgt->target_id,0);

	/*
	 * We own the bus now.. unless we lose arbitration
	 */
	asc->active_target = tgt;

	/* Try to avoid reselect collisions */
	if ((regs->asc_csr & (ASC_CSR_INT|SCSI_PHASE_MSG_IN)) ==
	    (ASC_CSR_INT|SCSI_PHASE_MSG_IN))
		return;

	/*
	 * Cleanup the FIFO
	 */
	regs->asc_cmd = ASC_CMD_FLUSH;
	readback(regs->asc_cmd);
	/*
	 * This value is not from spec, I have seen it failing
	 * without this delay and with logging disabled.  That had
	 * about 42 extra instructions @25Mhz.
	 */
	delay(2);/* XXX time & move later */


	/*
	 * Init bus state variables
	 */
	asc->script = tgt->transient_state.script;
	asc->error_handler = tgt->transient_state.handler;
	asc->done = SCSI_RET_IN_PROGRESS;

	asc->out_count = 0;
	asc->in_count = 0;
	asc->extra_count = 0;

	/*
	 * Start the chip going
	 */

#ifdef	PARAGON860		/* Emulex workaround */
	out_count = tgt->transient_state.cmd_count;
#else	/* PARAGON860 */
	out_count = (*asc->dma_ops->start_cmd)(asc->dma_state, tgt);
#endif	/* PARAGON860 */

	if (tgt->transient_state.identify != 0xff)
		regs->asc_fifo = tgt->transient_state.identify;

#ifdef	PARAGON860		/* Emulex workaround */

	/*
	 * The Emulex FAS SCSI controller appears to have a problem
	 * when the command is transferred via DMA if the chip is clocked
	 * at 40 MHz and the DMA is suspended by other hardware taking
	 * control of the bus.  The command is transferred to the SCSI bus,
	 * but a copy of the last command byte remains in the SCSI FIFO.
	 *
	 * Slowing the clock to the chip to 20 MHz seems to fix the problem
	 * but does not allow the SCSI bus to be run at full speed.
	 *
	 * Another workaround is to make sure the FIFO is empty before
	 * issuing the transfer information command.  This would require
	 * changes to the asc_dma_in/out and restart routines.
	 *
	 * The workaround chosen is to load the command in the FIFO with
	 * the processor and issue the command without DMA.
	 */

	/* load command in the fifo */
	for (i = 0; i < out_count; i++) {
		regs->asc_fifo = (unsigned char)tgt->cmd_ptr[i];
	}

#else	/* PARAGON860 */

	ASC_TC_PUT(regs, out_count);
	readback(regs->asc_cmd);

#endif	/* PARAGON860 */

	regs->asc_dbus_id = tgt->target_id;
	readback(regs->asc_dbus_id);

	regs->asc_syn_p = tgt->sync_period;
	readback(regs->asc_syn_p);

	regs->asc_syn_o = tgt->sync_offset;
	readback(regs->asc_syn_o);

	/* ugly little help for compiler */
#define	command	out_count
	if (tgt->flags & TGT_DID_SYNCH) {
		command = (tgt->transient_state.identify == 0xff) ?
#ifdef	PARAGON860		/* Emulex workaround */
				ASC_CMD_SEL :
				ASC_CMD_SEL_ATN; /*preferred*/
#else	/* PARAGON860 */
				ASC_CMD_SEL | ASC_CMD_DMA :
				ASC_CMD_SEL_ATN | ASC_CMD_DMA; /*preferred*/
#endif	/* PARAGON860 */
	} else if (tgt->flags & TGT_TRY_SYNCH)
		command = ASC_CMD_SEL_ATN_STOP;
	else
#ifdef	PARAGON860		/* Emulex workaround */
		command = ASC_CMD_SEL;
#else	/* PARAGON860 */
		command = ASC_CMD_SEL | ASC_CMD_DMA;
#endif	/* PARAGON860 */

	/* Try to avoid reselect collisions */
	if ((regs->asc_csr & (ASC_CSR_INT|SCSI_PHASE_MSG_IN)) !=
	    (ASC_CSR_INT|SCSI_PHASE_MSG_IN)) {
		register int	tmp;

		regs->asc_cmd = command;
		delay(2);

		/*
		 * Very nasty but infrequent problem here.  We got/will get
		 * reconnected but the chip did not interrupt.  The watchdog would
		 * fix it allright, but it stops the machine before it expires!
		 * Too bad we cannot just look at the interrupt register, sigh.
		 */
		tmp = get_reg(regs,asc_cmd);
#ifdef	PARAGON860		/* Emulex workaround */
		if ((tmp != command) && (tmp == (ASC_CMD_NOP))) {
#else	/* PARAGON860 */
		if ((tmp != command) && (tmp == (ASC_CMD_NOP|ASC_CMD_DMA))) {
#endif	/* PARAGON860 */
		    if ((regs->asc_csr & ASC_CSR_INT) == 0) {
			delay(250); /* increase if in trouble */

			if (get_reg(regs,asc_csr) == SCSI_PHASE_MSG_IN) {
			    /* ok, take the risk of reading the ir */
			    tmp = get_reg(regs,asc_intr);
			    if (tmp & ASC_INT_RESEL) {
				(void) asc_reconnect(asc, regs->asc_csr, tmp);
				asc_wait(regs, ASC_CSR_INT);
				tmp = get_reg(regs,asc_intr);
				regs->asc_cmd = ASC_CMD_MSG_ACPT;
				readback(regs->asc_cmd);
			    } else /* does not happen, but who knows.. */
				asc_reset(asc,FALSE,asc->state & ASC_STATE_SPEC_DMA);
			}
		    }
		}
	}
#undef	command
}

/*
 * Interrupt routine
 *	Take interrupts from the chip
 *
 * Implementation:
 *	Move along the current command's script if
 *	all is well, invoke error handler if not.
 */
asc_intr(unit, spllevel)
	int	spllevel;
{
	register asc_softc_t	asc;
	register script_t	scp;
	register int		ir, csr;
	register asc_padded_regmap_t	*regs;
#ifdef	MACH_KERNEL
#ifndef	PARAGON860
	extern boolean_t	rz_use_mapped_interface;

	if (rz_use_mapped_interface)
		return ASC_intr(unit,spllevel);
#endif	/* PARAGON860 */
#endif	/*MACH_KERNEL*/

	asc = asc_softc[unit];

	LOG(5,"\n\tintr");
	/* collect ephemeral information */
	regs = asc->regs;
	csr  = get_reg(regs,asc_csr);
	asc->ss_was = get_reg(regs,asc_ss);
	asc->cmd_was = get_reg(regs,asc_cmd);

	/* drop spurious interrupts */
	if ((csr & ASC_CSR_INT) == 0)
		return;

	ir = get_reg(regs,asc_intr);	/* this re-latches CSR (and SSTEP) */

TR(csr);TR(ir);TR(regs->asc_cmd);TRCHECK

	/* this must be done within 250msec of disconnect */
	if (ir & ASC_INT_DISC) {
		regs->asc_cmd = ASC_CMD_ENABLE_SEL;
		readback(asc->regs->asc_cmd);
	}

	if (ir & ASC_INT_RESET)
		return asc_bus_reset(asc);

	/* we got an interrupt allright */
	if (asc->active_target)
		asc->wd.watchdog_state = SCSI_WD_ACTIVE;

#ifndef	PARAGON860
	splx(spllevel);	/* drop priority */
#endif	/* PARAGON860 */

	if ((asc->state & ASC_STATE_TARGET) ||
	    (ir & (ASC_INT_SEL|ASC_INT_SEL_ATN)))
		return asc_target_intr(asc);

	/*
	 * In attempt_selection() we could not check the asc_intr
	 * register to see if a reselection was in progress [else
	 * we would cancel the interrupt, and it would not be safe
	 * anyways].  So we gave the select command even if sometimes
	 * the chip might have been reconnected already.  What
	 * happens then is that we get an illegal command interrupt,
	 * which is why the second clause.  Sorry, I'd do it better
	 * if I knew of a better way.
	 */
	if ((ir & ASC_INT_RESEL) ||
	    ((ir & ASC_INT_ILL) && (regs->asc_cmd & ASC_CMD_SEL_ATN)))
		return asc_reconnect(asc, csr, ir);

	/*
	 * Check for various errors
	 */
	if ((csr & (ASC_CSR_GE|ASC_CSR_PE)) || (ir & ASC_INT_ILL)) {
		char	*msg;
printf("{E%x,%x}", csr, ir);

		if (csr & ASC_CSR_GE)
			panic("ASC: Gross Error\n");
			/*return;/* sit and prey? */

		if (csr & ASC_CSR_PE)
			msg = "SCSI bus parity error";

		if (ir & ASC_INT_ILL)
			msg = "SCSI Chip Illegal Command";

		/* all we can do is to throw a reset on the bus */
		printf( "asc%d: %s%s", asc - asc_softc_data, msg,
			", attempting recovery.\n");
		asc_reset(asc, FALSE, asc->state & ASC_STATE_SPEC_DMA);
		return;
	}

	if ((scp = asc->script) == 0) {	/* sanity */
		panic("Null SCSI script!");
		return;
	}

	LOG(6,"match");
	if (SCRIPT_MATCH(csr,ir,scp->condition)) {
		/*
		 * Perform the appropriate operation,
		 * then proceed
		 */
		if ((*scp->action)(asc, csr, ir)) {
			asc->script = scp + 1;
			regs->asc_cmd = scp->command;
		}
	} else
		return (*asc->error_handler)(asc, csr, ir);
}

asc_target_intr()
{
	panic("ASC: TARGET MODE !!!\n");
}

/*
 * All the many little things that the interrupt
 * routine might switch to
 */
boolean_t
asc_clean_fifo(asc, csr, ir)
	register asc_softc_t	asc;

{
	register asc_padded_regmap_t	*regs = asc->regs;
	register char		ff;

	LOG(7,"clean_fifo");

	while (regs->asc_flags & ASC_FLAGS_FIFO_CNT)
		ff = regs->asc_fifo;

	return TRUE;
}

boolean_t
asc_end(asc, csr, ir)
	register asc_softc_t	asc;
{
	register target_info_t	*tgt;
	register io_req_t	ior;

	LOG(8,"end");

	tgt = asc->active_target;
	if ((tgt->done = asc->done) == SCSI_RET_IN_PROGRESS)
		tgt->done = SCSI_RET_SUCCESS;

	asc->script = 0;

	if (asc->wd.nactive-- == 1)
		asc->wd.watchdog_state = SCSI_WD_INACTIVE;

	/* reset the LUN field to the real LUN value */
	tgt->lun = tgt->true_lun;

	asc_release_bus(asc);

	if (ior = tgt->ior) {
		(*asc->dma_ops->end_cmd)(asc->dma_state, tgt, ior);
		LOG(0xA,"ops->restart");
		(*tgt->dev_ops->restart)( tgt, TRUE);
	}

	return FALSE;
}

boolean_t
asc_release_bus(asc)
	register asc_softc_t	asc;
{
	boolean_t	ret = TRUE;

	LOG(9,"release");
	if (asc->state & ASC_STATE_COLLISION) {

		LOG(0xB,"collided");
		asc->state &= ~ASC_STATE_COLLISION;
		asc_attempt_selection(asc);

	} else if (queue_empty(&asc->waiting_targets)) {

		asc->state &= ~ASC_STATE_BUSY;
		asc->active_target = 0;
		asc->script = 0;
		ret = FALSE;

	} else {

		LOG(0xC,"dequeue");
		asc->next_target = (target_info_t *)
				dequeue_head(&asc->waiting_targets);
		asc_attempt_selection(asc);
	}
	return ret;
}

boolean_t
asc_get_status(asc, csr, ir)
	register asc_softc_t	asc;
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register scsi2_status_byte_t status;
	int			len;
	boolean_t		ret;
	io_req_t		ior;
	register target_info_t	*tgt = asc->active_target;

	LOG(0xD,"get_status");
TRWRAP

	asc->state &= ~ASC_STATE_DMA_IN;

	/*
	 * Get the last two bytes in FIFO
	 */
	while ((regs->asc_flags & ASC_FLAGS_FIFO_CNT) > 2)
		status.bits = regs->asc_fifo;

	status.bits = regs->asc_fifo;

	if (status.st.scsi_status_code != SCSI_ST_GOOD) {
		scsi_error(asc->active_target, SCSI_ERR_STATUS, status.bits, 0);
		asc->done = (status.st.scsi_status_code == SCSI_ST_BUSY) ?
			SCSI_RET_RETRY : SCSI_RET_NEED_SENSE;
	} else
		asc->done = SCSI_RET_SUCCESS;

	status.bits = regs->asc_fifo;	/* just pop the command_complete */

	/* if reading, move the last piece of data in main memory */
	if (len = asc->in_count) {
		register int	count;

		ASC_TC_GET(regs,count);
		if (count) {
			/*
			 * this is incorrect and besides..
			 * tgt->ior->io_residual = count;
			 */

			len -= count;
		}
		regs->asc_cmd = asc->script->command;
		readback(regs->asc_cmd);
		
		ret = FALSE;
	} else
		ret = TRUE;

	ASC_TC_PUT(regs,0);	/* stop dma engine */

	(*asc->dma_ops->end_xfer)(asc->dma_state, tgt, len);
	if (!ret)
		asc->script++;
	return ret;
}

boolean_t
asc_dma_in(asc, csr, ir)
	register asc_softc_t	asc;
{
	register target_info_t	*tgt;
	register asc_padded_regmap_t	*regs = asc->regs;
	register int		count;
	unsigned char		ff = get_reg(regs,asc_flags);

	LOG(0xE,"dma_in");
	tgt = asc->active_target;

	/*
	 * This seems to be needed on certain rudimentary targets
	 * (such as the DEC TK50 tape) which apparently only pick
	 * up 6 initial bytes: when you add the initial IDENTIFY
	 * you are left with 1 pending byte, which is left in the
	 * FIFO and would otherwise show up atop the data we are
	 * really requesting.
	 *
	 * This is only speculation, though, based on the fact the
	 * sequence step value of 3 out of select means the target
	 * changed phase too quick and some bytes have not been
	 * xferred (see NCR manuals).  Counter to this theory goes
	 * the fact that the extra byte that shows up is not alwyas
	 * zero, and appears to be pretty random.
	 * Note that asc_flags say there is one byte in the FIFO
	 * even in the ok case, but the sstep value is the right one.
	 * Note finally that this might all be a sync/async issue:
	 * I have only checked the ok case on synch disks so far.
	 *
	 * Indeed it seems to be an asynch issue: exabytes do it too.
	 */
	if (((tgt->sync_offset & ASC_SYNO_MASK) == 0) &&
	    ((ff & ASC_FLAGS_SEQ_STEP) != 0x80)) {
		regs->asc_cmd = ASC_CMD_NOP;
		delay(2);
		wbflush();
		PRINT(("[tgt %d: %x while %d]", tgt->target_id, ff, tgt->cur_cmd));
		while ((ff & ASC_FLAGS_FIFO_CNT) != 0) {
			ff = get_fifo(regs);
			ff = get_reg(regs,asc_flags);
		}
	}

	asc->state |= ASC_STATE_DMA_IN;

	count = (*asc->dma_ops->start_datain)(asc->dma_state, tgt);
	ASC_TC_PUT(regs, count);
	readback(regs->asc_cmd);

	if ((asc->in_count = count) == tgt->transient_state.in_count)
		return TRUE;
	regs->asc_cmd = asc->script->command;
	asc->script = asc_script_restart_data_in;
	return FALSE;
}

asc_dma_in_r(asc, csr, ir)
	register asc_softc_t	asc;
{
	register target_info_t	*tgt;
	register asc_padded_regmap_t	*regs = asc->regs;
	register int		count;
	boolean_t		advance_script = TRUE;


	LOG(0xE,"dma_in");
	tgt = asc->active_target;

	asc->state |= ASC_STATE_DMA_IN;

	if (asc->in_count == 0) {
		/*
		 * Got nothing yet, we just reconnected.
		 */
		register int avail;

		/*
		 * Rather than using the messy RFB bit in cnfg2
		 * (which only works for synch xfer anyways)
		 * we just bump up the dma offset.  We might
		 * endup with one more interrupt at the end,
		 * so what.
		 * This is done in asc_err_disconn(), this
		 * way dma (of msg bytes too) is always aligned
		 */

		count = (*asc->dma_ops->restart_datain_1)
				(asc->dma_state, tgt);

		/* common case of 8k-or-less read ? */
		advance_script = (tgt->transient_state.in_count == count);

	} else {

		/*
		 * We received some data.
		 */
		register int offset, xferred;

		/*
		 * Problem: sometimes we get a 'spurious' interrupt
		 * right after a reconnect.  It goes like disconnect,
		 * reconnect, dma_in_r, here but DMA is still rolling.
		 * Since there is no good reason we got here to begin with
		 * we just check for the case and dismiss it: we should
		 * get another interrupt when the TC goes to zero or the
		 * target disconnects.
		 */
		ASC_TC_GET(regs,xferred);
		if (xferred != 0)
			return FALSE;

		xferred = asc->in_count - xferred;
		if(scsi_debug){
			assert(xferred > 0);
		}

		tgt->transient_state.in_count -= xferred;
		if(scsi_debug){
			assert(tgt->transient_state.in_count > 0);
		}

		count = (*asc->dma_ops->restart_datain_2)
				(asc->dma_state, tgt, xferred);

		asc->in_count = count;
		ASC_TC_PUT(regs, count);
		readback(regs->asc_cmd);
		regs->asc_cmd = asc->script->command;

		(*asc->dma_ops->restart_datain_3)
			(asc->dma_state, tgt);

		/* last chunk ? */
		if (count == tgt->transient_state.in_count)
			asc->script++;

		return FALSE;
	}

	asc->in_count = count;
	ASC_TC_PUT(regs, count);
	readback(regs->asc_cmd);

	if (!advance_script) {
		regs->asc_cmd = asc->script->command;
		readback(regs->asc_cmd);
	}
	return advance_script;
}


/* send data to target.  Only called to start the xfer */

boolean_t
asc_dma_out(asc, csr, ir)
	register asc_softc_t	asc;
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register int		reload_count;
	register target_info_t	*tgt;
	int			command;

	LOG(0xF,"dma_out");

#ifdef	PARAGON860

	tgt = asc->active_target;

	asc->state &= ~ASC_STATE_DMA_IN;

	command = asc->script->command;

	(*asc->dma_ops->start_dataout)
		(asc->dma_state, tgt, &regs->asc_cmd, command);
	reload_count = tgt->transient_state.out_count;
	ASC_TC_PUT(regs, reload_count);
	readback(regs->asc_cmd);

	if ((asc->out_count = reload_count) >= tgt->transient_state.out_count)
		return TRUE;

	regs->asc_cmd = command;
	asc->script = asc_script_restart_data_out;

#else	/* PARAGON860 */

	ASC_TC_GET(regs, reload_count);
	asc->extra_count = regs->asc_flags & ASC_FLAGS_FIFO_CNT;
	reload_count += asc->extra_count;
	ASC_TC_PUT(regs, reload_count);
	asc->state &= ~ASC_STATE_DMA_IN;
	readback(regs->asc_cmd);

	tgt = asc->active_target;

	command = asc->script->command;

	if ((asc->out_count = reload_count) >=
	    tgt->transient_state.out_count)
		asc->script++;
	else
		asc->script = asc_script_restart_data_out;

	if ((*asc->dma_ops->start_dataout)
	      (asc->dma_state, tgt, &regs->asc_cmd, command)) {
			regs->asc_cmd = command;
			readback(regs->asc_cmd);
	}

#endif	/* PARAGON860 */

	return FALSE;
}

/* send data to target. Called in two different ways:
   (a) to restart a big transfer and 
   (b) after reconnection
 */
boolean_t
asc_dma_out_r(asc, csr, ir)
	register asc_softc_t	asc;
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register target_info_t	*tgt;
	boolean_t		advance_script = TRUE;
	int			count;


	LOG(0xF,"dma_out");

	tgt = asc->active_target;
	asc->state &= ~ASC_STATE_DMA_IN;

	if (asc->out_count == 0) {
		/*
		 * Nothing committed: we just got reconnected
		 */
		count = (*asc->dma_ops->restart_dataout_1)
				(asc->dma_state, tgt);

		/* is this the last chunk ? */
		advance_script = (tgt->transient_state.out_count == count);
	} else {
		/*
		 * We sent some data.
		 */
		register int offset, xferred;

		ASC_TC_GET(regs,count);

		/* see comment above */
		if (count) {
			return FALSE;
		}

		count += (regs->asc_flags  & ASC_FLAGS_FIFO_CNT);
		count -= asc->extra_count;
		xferred = asc->out_count - count;
		if(scsi_debug){
			assert(xferred > 0);
		}

		tgt->transient_state.out_count -= xferred;
		if(scsi_debug){
			assert(tgt->transient_state.out_count > 0);
		}

		count = (*asc->dma_ops->restart_dataout_2)
				(asc->dma_state, tgt, xferred);

		/* last chunk ? */
		if (tgt->transient_state.out_count == count)
			goto quickie;

		asc->out_count = count;

		asc->extra_count = (*asc->dma_ops->restart_dataout_3)
					(asc->dma_state, tgt, &regs->asc_fifo);
		ASC_TC_PUT(regs, count);
		readback(regs->asc_cmd);
		regs->asc_cmd = asc->script->command;

		(*asc->dma_ops->restart_dataout_4)(asc->dma_state, tgt);

		return FALSE;
	}

quickie:
	asc->extra_count = (*asc->dma_ops->restart_dataout_3)
				(asc->dma_state, tgt, &regs->asc_fifo);

	asc->out_count = count;

	ASC_TC_PUT(regs, count);
	readback(regs->asc_cmd);

	if (!advance_script) {
		regs->asc_cmd = asc->script->command;
	}
	return advance_script;
}

boolean_t
asc_dosynch(asc, csr, ir)
	register asc_softc_t	asc;
	register unsigned char	csr, ir;
{
	register asc_padded_regmap_t	*regs = asc->regs;
	register unsigned char	c;
	int i, per, offs;
	register target_info_t	*tgt;

	/*
	 * Phase is MSG_OUT here.
	 * Try synch negotiation, unless prohibited
	 */
	tgt = asc->active_target;
	regs->asc_cmd = ASC_CMD_FLUSH;
	delay(2);

	if (asc->state & (ASC_STATE_FAS_CNTL | ASC_STATE_FSC_CNTL))
		per = fas_min_period;
	else
		per = asc_min_period;

	if (BGET(scsi_no_synchronous_xfer,asc->sc->masterno,tgt->target_id))
		offs = 0;
	else
		offs = (asc->state & ASC_STATE_SPEC_DMA) ?
			asc_spec_dma_offset : asc_max_offset;

	tgt->flags |= TGT_DID_SYNCH;	/* only one chance */
	tgt->flags &= ~TGT_TRY_SYNCH;

	regs->asc_fifo = SCSI_EXTENDED_MESSAGE;
	regs->asc_fifo = 3;
	regs->asc_fifo = SCSI_SYNC_XFER_REQUEST;
	regs->asc_fifo = asc_to_scsi_period(per);
	regs->asc_fifo = offs;
	regs->asc_cmd = ASC_CMD_XFER_INFO;
	readback(regs->asc_cmd);
	csr = asc_wait(regs, ASC_CSR_INT);
	ir = get_reg(regs,asc_intr);

	/* some targets might be slow to move to msg-in */
	if (SCSI_PHASE(csr) != SCSI_PHASE_MSG_IN) {

		/* wait for direction bit to flip */
		csr = asc_wait(regs, SCSI_IO);
		ir = get_reg(regs,asc_intr); mb();
		/* Some UGLY targets go stright to command phase !
		   "You could at least say goodbye" */
		if (SCSI_PHASE(csr) == SCSI_PHASE_CMD)
			goto did_not;
		if (SCSI_PHASE(csr) != SCSI_PHASE_MSG_IN)
			gimmeabreak();
	}

	regs->asc_cmd = ASC_CMD_XFER_INFO;
	csr = asc_wait(regs, ASC_CSR_INT);
	ir = get_reg(regs,asc_intr);

	while ((regs->asc_flags & ASC_FLAGS_FIFO_CNT) > 0)
		c = get_fifo(regs);	/* see what it says */

	if (c == SCSI_MESSAGE_REJECT) {
did_not:
		printf(" (asynchronous)");

		regs->asc_cmd = ASC_CMD_MSG_ACPT;
		csr = asc_wait(regs, ASC_CSR_INT);
		ir = get_reg(regs,asc_intr);

		/*
		 * Phase is MSG_OUT here.
		 * Send a NOP message out to satisfy the targets
		 * request for a message and to drop the ATN signal
		 */
		regs->asc_fifo = SCSI_NOP;
		regs->asc_cmd = ASC_CMD_XFER_INFO;
		readback(regs->asc_cmd);
		csr = asc_wait(regs, ASC_CSR_INT);
		ir = get_reg(regs,asc_intr);

		goto cmd;
	}

	/*
	 * Receive the rest of the message
	 */
	regs->asc_cmd = ASC_CMD_MSG_ACPT;
	asc_wait(regs, ASC_CSR_INT);
	ir = get_reg(regs,asc_intr);

	if (c != SCSI_EXTENDED_MESSAGE)
		gimmeabreak();

	regs->asc_cmd = ASC_CMD_XFER_INFO;
	asc_wait(regs, ASC_CSR_INT);
	c = get_reg(regs,asc_intr);
	if (regs->asc_fifo != 3)
		panic("asc_dosynch");

	for (i = 0; i < 3; i++) {
		regs->asc_cmd = ASC_CMD_MSG_ACPT;
		asc_wait(regs, ASC_CSR_INT);
		c = get_reg(regs,asc_intr);

		regs->asc_cmd = ASC_CMD_XFER_INFO;
		asc_wait(regs, ASC_CSR_INT);
		c = get_reg(regs,asc_intr);/*ack*/
		c = get_fifo(regs);

		if (i == 1) tgt->sync_period = scsi_period_to_asc(c);
		if (i == 2) tgt->sync_offset = (c & ASC_SYNO_MASK);
	}

	regs->asc_cmd = ASC_CMD_MSG_ACPT;
	csr = asc_wait(regs, ASC_CSR_INT);
	c = get_reg(regs,asc_intr);

cmd:
	/* phase should normally be command here */
	if (SCSI_PHASE(csr) == SCSI_PHASE_CMD) {
		register char	*cmd = tgt->cmd_ptr;

		/* test unit ready or inquiry */
		for (i = 0; i < sizeof(scsi_command_group_0); i++)
			regs->asc_fifo = *cmd++;
		ASC_TC_PUT(regs,0xff);
		regs->asc_cmd = ASC_CMD_XFER_PAD; /*0x98*/

		if (tgt->cur_cmd == SCSI_CMD_INQUIRY) {
			tgt->transient_state.script = asc_script_data_in;
			asc->script = tgt->transient_state.script;
			return FALSE;
		}

		if (tgt->cur_cmd == SCSI_CMD_TEST_UNIT_READY) {
			tgt->transient_state.script = asc_script_simple_cmd;
			asc->script = tgt->transient_state.script;
			return FALSE;
		}

		csr = asc_wait(regs, ASC_CSR_INT);
		ir = get_reg(regs,asc_intr);/*ack*/
	}

status:
	if (SCSI_PHASE(csr) != SCSI_PHASE_STATUS)
		gimmeabreak();

	return TRUE;
}

/*
 * The bus was reset
 */
asc_bus_reset(asc)
	register asc_softc_t	asc;
{
	register asc_padded_regmap_t	*regs = asc->regs;

	LOG(0x1d,"bus_reset");

	/*
	 * Clear command (needed?)
	 */
	regs->asc_cmd = ASC_CMD_NOP;

	/*
	 * Clear bus descriptor
	 */
	asc->script = 0;
	asc->error_handler = 0;
	asc->active_target = 0;
	asc->next_target = 0;
	asc->state &= ASC_STATE_SPEC_DMA;	/* save this one bit only */
	queue_init(&asc->waiting_targets);
	asc->wd.nactive = 0;
	asc_reset(asc, TRUE, asc->state & ASC_STATE_SPEC_DMA);

	printf("asc: (%d) bus reset ", ++asc->wd.reset_count);
	delay(scsi_delay_after_reset); /* some targets take long to reset */

	if (asc->sc == 0)	/* sanity */
		return;

	scsi_bus_was_reset(asc->sc);
}

/*
 * Disconnect/reconnect mode ops
 */

/* get the message in via dma */
boolean_t
asc_msg_in(asc, csr, ir)
	register asc_softc_t	asc;
	register unsigned char	csr, ir;
{
	register target_info_t	*tgt;
	register asc_padded_regmap_t	*regs = asc->regs;
	unsigned char		ff = regs->asc_flags;

	LOG(0x10,"msg_in");
	/* must clean FIFO as in asc_dma_in, sigh */
	while ((ff & ASC_FLAGS_FIFO_CNT) != 0) {
		ff = regs->asc_fifo;
		ff = regs->asc_flags;
	}

	(void) (*asc->dma_ops->start_msgin)(asc->dma_state, asc->active_target);

	/*
	 * We only expect at most two bytes (in the asc fifo)
	 * Each byte must be received one at a time and creates an interrupt
	 */

	return TRUE;
}

/* check the message is indeed a DISCONNECT */
boolean_t
asc_disconnect(asc, csr, ir)
	register asc_softc_t	asc;
	register unsigned char	csr, ir;

{
	register target_info_t	*tgt;
	asc_padded_regmap_t	*regs;
	unsigned char		msg;

	tgt = asc->active_target;

	(*asc->dma_ops->end_msgin)(asc->dma_state, tgt);

	/*
	 * Do not do this. It is most likely a reconnection
	 * message that sits there already by the time we
	 * get here.  The chip is smart enough to only dma
	 * the bytes that correctly came in as msg_in proper,
	 * the identify and selection bytes are not dma-ed.
	 * Yes, sometimes the hardware does the right thing.
	 */

#if 0
	/* First check message got out of the fifo */
	regs = asc->regs;
	ff = regs->asc_flags;
	while ((ff & ASC_FLAGS_FIFO_CNT) != 0) {
		*msgs++ = regs->asc_fifo;
		ff = regs->asc_flags;
	}
#endif

	regs = asc->regs;

	while ((regs->asc_flags & ASC_FLAGS_FIFO_CNT) > 0)
		msg = regs->asc_fifo;	/* see what it says */

	delay(2);
	regs->asc_cmd = ASC_CMD_MSG_ACPT;
	delay(2);

	switch (msg) {

		case SCSI_DISCONNECT:		/* completed */
			break;

		case SCSI_SAVE_DATA_POINTER:	/* non-complete */
			/*
			 * A SDP message preceeds the disconnect
			 * message on non-completed READs
			 */

			/* wait for previous command to complete */
			csr = asc_wait(regs, ASC_CSR_INT);
			ir = regs->asc_intr;

			delay(2);
			regs->asc_cmd = ASC_CMD_XFER_INFO;
			csr = asc_wait(regs, ASC_CSR_INT);
			ir = regs->asc_intr;

			while ((regs->asc_flags & ASC_FLAGS_FIFO_CNT) > 0)
				msg = regs->asc_fifo;	/* see what it says */

			delay(2);
			regs->asc_cmd = ASC_CMD_MSG_ACPT;
			delay(2);

			if (msg != SCSI_DISCONNECT) {
				printf(
				    "[tgt %d bad disconnect message: 0x%02x]\n",
				    tgt->target_id, msg);
			}
			break;

		case SCSI_RESTORE_POINTERS:
#ifdef	PARAGON860	/* Restore Pointers message fix */
			if (scsi_debug)
			    printf("[tgt %d: restore data pointer message]\n",
				tgt->target_id);
			asc->script = tgt->transient_state.script;
#else	/* PARAGON860 */	/* Restore Pointers message fix */
			printf("[tgt %d ", tgt->target_id);
			panic("restore data pointer message]");
#endif	/* PARAGON860 */	/* Restore Pointers message fix */
			break;

		default:
			printf("[tgt %d bad SDP: 0x%02x]\n",
				tgt->target_id, msg);
			break;
	}

#ifdef	PARAGON860	/* Restore Pointers message fix */
	if (msg == SCSI_RESTORE_POINTERS)
		return FALSE;

#endif	/* PARAGON860 */	/* Restore Pointers message fix */

	/* wait for previous command to complete */
	csr = asc_wait(regs, ASC_CSR_INT);
	ir = regs->asc_intr;

	/* this must be done within 250msec of disconnect */
	if (ir & ASC_INT_DISC) {
		regs->asc_cmd = ASC_CMD_ENABLE_SEL;
		readback(regs->asc_cmd);
		asc->script++;
	} else {
		printf("[tgt %d did not disconnect]\n", tgt->target_id);
	}

	return TRUE;
}

/* save all relevant data, free the BUS */
boolean_t
asc_disconnected(asc, csr, ir)
	register asc_softc_t	asc;
	register unsigned char	csr, ir;

{
	register target_info_t	*tgt;

#ifdef	PARAGON860	/* Restore Pointers message fix */
	if (!asc_disconnect(asc,csr,ir))
		return FALSE;

	LOG(0x11,"disconnected");
#else	/* PARAGON860 */	/* Restore Pointers message fix */
	LOG(0x11,"disconnected");
	asc_disconnect(asc,csr,ir);
#endif	/* PARAGON860 */	/* Restore Pointers message fix */

	tgt = asc->active_target;
	tgt->flags |= TGT_DISCONNECTED;
	tgt->transient_state.handler = asc->error_handler;
	/* anything else was saved in asc_err_disconn() */

	PRINT(("{D%d}", tgt->target_id));

	asc_release_bus(asc);

	return FALSE;
}

/* get reconnect message out of fifo, restore BUS */
int reconn_cnt, collision_cnt;

boolean_t
asc_reconnect(asc, csr, ir)
	register asc_softc_t	asc;
	register unsigned char	csr, ir;

{
	register target_info_t	*tgt;
	asc_padded_regmap_t		*regs;
	register unsigned char	ff;
	int			id;

reconn_cnt++;
if (ir & ASC_INT_ILL) collision_cnt++;

	LOG(0x12,"reconnect");
	/*
	 * See if this reconnection collided with a selection attempt
	 */
	if (asc->state & ASC_STATE_BUSY)
		asc->state |= ASC_STATE_COLLISION;

	asc->state |= ASC_STATE_BUSY;

	/* find tgt: first byte in fifo is (tgt_id|our_id) */
	regs = asc->regs;

	/*
	 * The chip FIFO should have at least two bytes (the
	 * reselection bus id and the identify message) and
	 * may also have a third byte (the identify message
	 * from a command that was being sent to another target).
	 * See asc_attempt_selection().
	 */
	ff = regs->asc_flags & ASC_FLAGS_FIFO_CNT;
	if (ff < 2) {
		printf("asc_reconnect: fifo contains %d bytes\n", ff);
		gimmeabreak();
	}

	id = regs->asc_fifo & 0xff;	/* must decode this now */
	id &= ~(1 << asc->sc->initiator_id);
	assert(id != 0);
	{
		register int	i;
		for (i = 0; id > 1; i++)
			id >>= 1;
		id = i;
	}

	/* sanity, empty fifo */
	ff = regs->asc_fifo & ~SCSI_IFY_LUN_MASK;
	if (ff != SCSI_IDENTIFY) {
		printf("target id %d invalid identify message: x%x\n", id, ff);
		gimmeabreak();
	}

	tgt = asc->sc->target[id];
	if (id > MAX_SCSI_TARGETS-1 || tgt == 0) panic("asc_reconnect");

	/*
	 * The chip FIFO may have the identify message in it
	 * from a command that was being sent to another target
	 * Just flush it away; a collision should have been detected.
	 */

	/* must clean FIFO as in asc_dma_in, sigh */
	ff = regs->asc_flags;
	while ((ff & ASC_FLAGS_FIFO_CNT) != 0) {
		ff = regs->asc_fifo;
		ff = regs->asc_flags;
	}

	/* synch things*/
	regs->asc_syn_p = tgt->sync_period;
	regs->asc_syn_o = tgt->sync_offset;
	readback(regs->asc_syn_o);

	PRINT(("{R%d}", id));
	if (asc->state & ASC_STATE_COLLISION)
		PRINT(("[B %d-%d]", asc->active_target->target_id, id));

	LOG(0x80+id,0);

	asc->active_target = tgt;
	tgt->flags &= ~TGT_DISCONNECTED;

	asc->script = tgt->transient_state.script;
	asc->error_handler = tgt->transient_state.handler;
	asc->in_count = 0;
	asc->out_count = 0;

	/*
	 * Not sure why, but this prevents tk50s from embarassing
	 * us quite a bit
	 */
	delay(4);

	regs->asc_cmd = ASC_CMD_MSG_ACPT;
	readback(regs->asc_cmd);

	return FALSE;
}

/*
 * Error handlers
 */

/*
 * Fall-back error handler.
 */
asc_err_generic(asc, csr, ir)
	register asc_softc_t	asc;
{
	LOG(0x13,"err_generic");

	/* handle non-existant or powered off devices here */
	if ((ir == ASC_INT_DISC) &&
	    (asc_isa_select(asc->cmd_was)) &&
	    (ASC_SS(asc->ss_was) == 0)) {
		/* Powered off ? */
		if (asc->active_target->flags & TGT_FULLY_PROBED)
			asc->active_target->flags = 0;
		asc->done = SCSI_RET_DEVICE_DOWN;
		asc_end(asc, csr, ir);
		return;
	}

	switch (SCSI_PHASE(csr)) {
	case SCSI_PHASE_STATUS:
		if (asc->script[-1].condition == SCSI_PHASE_STATUS) {
			/* some are just slow to get out.. */
		} else
			asc_err_to_status(asc, csr, ir);
		return;
		break;
	case SCSI_PHASE_DATAI:
		if (asc->script->condition == SCSI_PHASE_STATUS) {
			/*
			 * Here is what I know about it.  We reconnect to
			 * the target (csr 87, ir 0C, cmd 44), start dma in
			 * (81 10 12) and then get here with (81 10 90).
			 * Dma is rolling and keeps on rolling happily,
			 * the value in the counter indicates the interrupt
			 * was signalled right away: by the time we get
			 * here only 80-90 bytes have been shipped to an
			 * rz56 running synchronous at 3.57 Mb/sec.
			 */
/*			printf("{P}");*/
			return;
		}
		break;
	case SCSI_PHASE_DATAO:
		if (asc->script->condition == SCSI_PHASE_STATUS) {
			/*
			 * See comment above. Actually seen on hitachis.
			 */
/*			printf("{P}");*/
			return;
		}
	}
	gimmeabreak();
}

/*
 * Handle generic errors that are reported as
 * an unexpected change to STATUS phase
 */
asc_err_to_status(asc, csr, ir)
	register asc_softc_t	asc;
{
	script_t		scp = asc->script;

	LOG(0x14,"err_tostatus");
	while (scp->condition != SCSI_PHASE_STATUS)
		scp++;
	(*scp->action)(asc, csr, ir);
	asc->script = scp + 1;
	asc->regs->asc_cmd = scp->command;
#if 0
	/*
	 * Normally, we would already be able to say the command
	 * is in error, e.g. the tape had a filemark or something.
	 * But in case we do disconnected mode WRITEs, it is quite
	 * common that the following happens:
	 *	dma_out -> disconnect (xfer complete) -> reconnect
	 * and our script might expect at this point that the dma
	 * had to be restarted (it didn't notice it was completed).
	 * And in any event.. it is both correct and cleaner to
	 * declare error iff the STATUS byte says so.
	 */
	asc->done = SCSI_RET_NEED_SENSE;
#endif
}

/*
 * Handle disconnections as exceptions
 */
asc_err_disconn(asc, csr, ir)
	register asc_softc_t	asc;
	register unsigned char	csr, ir;
{
	register asc_padded_regmap_t	*regs;
	register target_info_t	*tgt;
	int			count;
	boolean_t		callback = FALSE;

	LOG(0x16,"err_disconn");

	if (SCSI_PHASE(csr) != SCSI_PHASE_MSG_IN)
		return asc_err_generic(asc, csr, ir);

	regs = asc->regs;
	tgt = asc->active_target;

	switch (asc->script->condition) {
	case SCSI_PHASE_DATAO:
		LOG(0x1b,"+DATAO");
		if (asc->out_count) {
			register int xferred, offset;

			ASC_TC_GET(regs,xferred); /* temporary misnomer */
			xferred += regs->asc_flags & ASC_FLAGS_FIFO_CNT;
			xferred -= asc->extra_count;
			xferred = asc->out_count - xferred; /* ok now */
			tgt->transient_state.out_count -= xferred;
			if(scsi_debug){
				assert(tgt->transient_state.out_count > 0);
			}

			callback = (*asc->dma_ops->disconn_1)
					(asc->dma_state, tgt, xferred);

		} else {

			callback = (*asc->dma_ops->disconn_2)
					(asc->dma_state, tgt);

		}
		asc->extra_count = 0;
		tgt->transient_state.script = asc_script_restart_data_out;
		break;


	case SCSI_PHASE_DATAI:
		LOG(0x17,"+DATAI");
		if (asc->in_count) {
			register int offset, xferred;

			ASC_TC_GET(regs,count);
			xferred = asc->in_count - count;
			if(scsi_debug){
				assert(xferred > 0);
			}

/*
if (regs->asc_flags & 0xf)
	printf("{Xf %x,%x,%x}",
		xferred, asc->in_count, regs->asc_flags & ASC_FLAGS_FIFO_CNT);
*/
			tgt->transient_state.in_count -= xferred;
			if(scsi_debug){
				assert(tgt->transient_state.in_count > 0);
			}
			callback = (*asc->dma_ops->disconn_3)
					(asc->dma_state, tgt, xferred);

			tgt->transient_state.script = asc_script_restart_data_in;
			if (tgt->transient_state.in_count == 0)
				tgt->transient_state.script++;

		}
		else tgt->transient_state.script = asc->script;
		break;

	case SCSI_PHASE_STATUS:
		/* will have to restart dma */
		ASC_TC_GET(regs,count);
		if (asc->state & ASC_STATE_DMA_IN) {
			register int offset, xferred;

			LOG(0x1a,"+STATUS+R");

			xferred = asc->in_count - count;
			if(scsi_debug){
				assert(xferred > 0);
			}

/*
if (regs->asc_flags & 0xf)
	printf("{Xf %x,%x,%x}",
		xferred, asc->in_count, regs->asc_flags & ASC_FLAGS_FIFO_CNT);
*/
			tgt->transient_state.in_count -= xferred;
			if(scsi_debug){
				assert(tgt->transient_state.in_count > 0);
			}

			callback = (*asc->dma_ops->disconn_4)
					(asc->dma_state, tgt, xferred);

			tgt->transient_state.script = asc_script_restart_data_in;
			if (tgt->transient_state.in_count == 0)
				tgt->transient_state.script++;

		} else {

			/* add what's left in the fifo */
			count += (regs->asc_flags & ASC_FLAGS_FIFO_CNT);
			/* take back the extra we might have added */
			count -= asc->extra_count;
			/* ..and drop that idea */
			asc->extra_count = 0;

			LOG(0x19,"+STATUS+W");


			if ((count == 0) && (tgt->transient_state.out_count == asc->out_count)) {
				/* all done */
				tgt->transient_state.script = asc->script;
				tgt->transient_state.out_count = 0;
			} else {
				register int xferred, offset;

				/* how much we xferred */
				xferred = asc->out_count - count;

				tgt->transient_state.out_count -= xferred;
				if(scsi_debug){
				    assert(tgt->transient_state.out_count > 0);
				}

				callback = (*asc->dma_ops->disconn_5)
						(asc->dma_state,tgt,xferred);

				tgt->transient_state.script = asc_script_restart_data_out;
			}
			asc->out_count = 0;
		}
		break;
	default:
		gimmeabreak();
		return;
	}
	asc_msg_in(asc,csr,ir);
	asc->script = asc_script_disconnect;
	/*
	 * Problem:
	 *   Some targets (e.g. RAID controllers) disconnect frequently,
	 *   even on commands like READ_CAPACITY.  The upper layer expects
	 *   some results at tgt->cmd_ptr but the save data pointer and
	 *   disconnect messages were clobbering the first two bytes of
	 *   the data.
	 *
	 * Solution:
	 *   Do not use DMA transfer the messages to tgt->cmd_ptr.
	 *   Instead read the bytes one at a time out of the asc fifo
	 *   to avoid clobbering any data that may have been transfered.
	 */
	regs->asc_cmd = ASC_CMD_XFER_INFO;
	if (callback)
		(*asc->dma_ops->disconn_callback)(asc->dma_state, tgt);
}

/*
 * Watchdog
 *
 * So far I have only seen the chip get stuck in a
 * select/reselect conflict: the reselection did
 * win and the interrupt register showed it but..
 * no interrupt was generated.
 * But we know that some (name withdrawn) disks get
 * stuck in the middle of dma phases...
 */
asc_reset_scsibus(asc)
	register asc_softc_t	asc;
{
	register target_info_t	*tgt = asc->active_target;
	register asc_padded_regmap_t	*regs = asc->regs;
	register int		ir;

	if (scsi_debug && tgt) {
		int dmalen;
		ASC_TC_GET(asc->regs,dmalen);
		printf("Target %d was active, cmd x%x in x%x out x%x Sin x%x Sou x%x dmalen x%x\n", 
			tgt->target_id, tgt->cur_cmd,
			tgt->transient_state.in_count, tgt->transient_state.out_count,
			asc->in_count, asc->out_count,
			dmalen);
	}
	ir = regs->asc_intr;
	if ((ir & ASC_INT_RESEL) && (SCSI_PHASE(regs->asc_csr) == SCSI_PHASE_MSG_IN)) {
		/* getting it out of the woods is a bit tricky */
		int	s = splbio();

		(void) asc_reconnect(asc, regs->asc_csr, ir);
		asc_wait(regs, ASC_CSR_INT);
		ir = regs->asc_intr;
		regs->asc_cmd = ASC_CMD_MSG_ACPT;
		readback(regs->asc_cmd);
		splx(s);
	} else {
		regs->asc_cmd = ASC_CMD_BUS_RESET;
		delay(35);
	}
}

#endif	NASC > 0

	/****************************************************/
#endif	/* PARAGON860	osc1_3b11 code EGREGIOUS HACK XXXXX */
	/****************************************************/
