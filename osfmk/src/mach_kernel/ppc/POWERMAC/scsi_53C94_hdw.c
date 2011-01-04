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
 */

/*
 * MkLinux
 */

/*	$NetBSD: asc.c,v 1.18 1996/03/18 01:39:47 jonathan Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)asc.c	8.3 (Berkeley) 7/3/94
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
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * CMU HISTORY
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


#include <asc.h>
#if	NASC > 0
#include <platforms.h>

#include <ppc/proc_reg.h> /* For isync */

typedef	unsigned char asc_register_t;
#define PAD(n) u_char n[15];

#define	ASC_PROBE_DYNAMICALLY FALSE
#include <mach_debug.h>


#include <kern/spl.h>
#include <mach/std_types.h>
#include <types.h>
#include <chips/busses.h>
#include <scsi/compat_30.h>

#include <scsi/scsi.h>
#include <scsi/scsi2.h>
#include <scsi/scsi_defs.h>

#include <ppc/POWERMAC/scsi_53C94.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/powermac_gestalt.h>
#include <ppc/POWERMAC/device_tree.h>

#define	readback(a)	{ register int foo;  eieio(); foo = (a);}

/* Used only for debugging asserts */
#include <ppc/mem.h>
#include <ppc/pmap.h>
#include <ppc/POWERMAC/interrupts.h>


int	asc_to_scsi_period(int asc, int clk);

void	asc_intr(int device, void *ssp);

/*
 * Internal forward declarations.
 */
static void asc_reset();
static void asc_startcmd();

#if	MACH_DEBUG
#define ASC_DEBUG	1
#define TRACE	1
#endif

#define	ASC_DEBUG 1

#if ASC_DEBUG
int	asc_trace_debug = 0;
#endif

/*
 * Scripts are entries in a state machine table.
 * A script has four parts: a pre-condition, an action, a command to the chip,
 * and an index into asc_scripts for the next state. The first triggers error
 * handling if not satisfied and in our case it is formed by the
 * values of the interrupt register and status register, this
 * basically captures the phase of the bus and the TC and BS
 * bits.  The action part is just a function pointer, and the
 * command is what the 53C94 should be told to do at the end
 * of the action processing.  This command is only issued and the
 * script proceeds if the action routine returns TRUE.
 * See asc_intr() for how and where this is all done.
 */
typedef struct script {
	int		condition;	/* expected state at interrupt time */
	int		(*action)();	/* extra operations */
	int		command;	/* command to the chip */
	struct script	*next;		/* index into asc_scripts for next state */
} script_t;

/* Matching on the condition value */
/* MEB - note mask was 67 to catch Gross Error, but
 * the chip reports a Gross Error in response to a phase
 * change while performing  synchronous transfers.
 */

#define	SCRIPT_MATCH(ir, csr)		((ir) | (((csr) & 0x27) << 8))

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
	PAD(pad4)
	volatile asc_register_t	asc_intr;	/* r:  Interrupt */
	PAD(pad5)
	volatile asc_register_t	asc_ss;		/* r:  Sequence Step */
	PAD(pad6)
	volatile asc_register_t	asc_flags;	/* r:  FIFO flags + seq step */
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
	volatile asc_register_t asc_cnfg4;	/* rw: Configuration 4 */
	PAD(pad13);
	volatile asc_register_t fas_tc_hi;	/* rw: high register count */
	PAD(pad14);
} asc_curio_regmap_t;

/* Various Aliases.. */

#define	asc_dbus_id	asc_csr
#define	asc_sel_timo 	asc_intr
#define	asc_syn_p	asc_ss
#define	asc_syn_o	asc_flags

/*
 * DMA Maps.. 
 */


typedef struct dma_softc {
	volatile unsigned char	*base;
	volatile unsigned char	*ctrl;
	volatile unsigned char	*cur_ptr;
} dma_softc_t;

dma_softc_t	dma_softc[NASC];

#define	u_min(a, b)	(((a) < (b)) ? (a) : (b))

/*
 * State kept for each active SCSI device.
 */

#define	ASC_MAX_SG_SEGS	64

struct sg_segment {
	unsigned long	sg_offset;
	unsigned long	sg_count;
	vm_offset_t	sg_physaddr;
	unsigned char	*sg_vmaddr;
};

typedef struct scsi_state {
	script_t *script;	/* saved script while processing error */
	char	cdb[16];	/* Command block */
	int	statusByte;	/* status byte returned during STATUS_PHASE */
	int	error;		/* errno to pass back to device driver */
	int	dmalen;		/* amount to transfer in this chunk */
	int	total_count;	/* total bytes to transfer */
	u_char	*dma_area;	/* base address to DMA to from */
	int	buflen;		/* total remaining amount of data to transfer */
	int	saved_buflen;	/* Saved buflen for Save Pointers Msg */
	int	saved_sg_index;	/* Saved S/G Index for Save Pointers Msg */
	int	flags;		/* see below */
	int	sg_segment_count;	/* Number of S/G segments */
	struct	sg_segment	sg_list[ASC_MAX_SG_SEGS];
	int	sg_index;	/* Index into sg_list */
	int	device_flags;	/* various device flags */
	int	msg_sent;	/* What message was sent to device */
	int	msglen;		/* number of message bytes to read */
	int	msgcnt;		/* number of message bytes received */
	u_char	msg_out;	/* next MSG_OUT byte to send */
	u_char	msg_in[16];	/* buffer for multibyte messages */
} State;

/* state flags */
#define STATE_DISCONNECTED	0x001	/* currently disconnected from bus */
#define STATE_DMA_RUNNING	0x002	/* data DMA started */
#define STATE_DMA_IN		0x004	/* reading from SCSI device */
#define STATE_DMA_OUT		0x010	/* writing to SCSI device */
#define	STATE_CHAINED_IO	0x020	/* request is a S/G one */
#define STATE_PARITY_ERR	0x080	/* parity error seen */
#define	STATE_DMA_FIFO		0x100	/* using FIFO to transfer data */
#define	STATE_FIFO_XFER		0x200	/* request has to use FIFO */

/* device flags */
#define	DEV_NO_DISCONNECT	0x001	/* Do not preform disconnect */
#define	DEV_RUN_SYNC		0x002	/* Run synchronous transfers */

#define	ASC_NCMD	8

/*
 * State kept for each active SCSI host interface (53C94).
 */
struct asc_softc {
	queue_head_t		target_queue;	/* Target Queue */
	struct watchdog_t	wd;
	asc_curio_regmap_t	*regs;		/* chip address */
	scsi_curio_dma_ops_t	*dma_ops;	/* DMA ops */

	decl_simple_lock_data(, lock);
	decl_simple_lock_data(, queue_lock);

	int		unit;		/* Unit number */
	int		sc_id;		/* SCSI ID of this interface */
	int		myidmask;	/* ~(1 << myid) */
	int		state;		/* current SCSI connection state */
	int		target;		/* target SCSI ID if busy */
	script_t	*script;	/* next expected interrupt & action */
	target_info_t	*cmd[ASC_NCMD];	/* active command indexed by SCSI ID */
	State		st[ASC_NCMD];	/* state info for each active command */
	int		min_period;	/* Min transfer period clk/byte */
	int		max_scsi_offset;	/* Max transfer offset byte */
	int		max_transfer;	/* Max byte count to allowed to
					   transfer in any given DMA chunk */
	int		ccf;		/* CCF, whatever that really is? */
	int		timeout;	/* timeout */
	int		clk;		/* Clock in MHZ */
	scsi_softc_t	*sc;		/* Higher level SCSI driver */
	int		type;		/* Type of SCSI controller this is */
	u_char		nondma_cnfg3;	/* Non-dma config3 register */
	u_char		dma_cnfg3;	/* dma config3 register */

	char		cmd_areas[ASC_NCMD][512];
};

#define	ASC_STATE_IDLE		0	/* idle state */
#define	ASC_STATE_BUSY		1	/* selecting or currently connected */
#define ASC_STATE_TARGET	2	/* currently selected as target */
#define ASC_STATE_RESEL		3	/* currently waiting for reselect */

typedef struct asc_softc *asc_softc_t;

struct asc_softc asc_softc_data[NASC];
asc_softc_t	 asc_softc[NASC];

boolean_t	asc_probe_dynamically = FALSE;

/* 
 * Various constants.. 
 */

#define	ASC_MAX_OFFSET		7
#define	ASC_MIN_PERIOD_20	5
#define	ASC_MIN_PERIOD_40	4

#define	ASC_NCR_53C94		0	/* Regular 53C94 */
#define	ASC_NCR_53CF94		1	/* Fast-SCSI version of 53C94 */

/*
 * Dma operations.
 */
#define	ASCDMA_READ	1
#define	ASCDMA_WRITE	2

/* forward decls of script actions */
void	    asc_start_new_request(asc_softc_t asc);
static int script_nop(asc_softc_t, int, int, int);		/* when nothing needed */
static int asc_end(asc_softc_t, int, int, int);			/* all come to an end */
static int asc_get_status(asc_softc_t, int, int, int);		/* get status from target */
static int asc_dma_in(asc_softc_t, int, int, int);		/* start reading data from target */
static int asc_last_dma_in(asc_softc_t, int, int, int);		/* cleanup after all data is read */
static int asc_dma_out(asc_softc_t, int, int, int);		/* send data to target via dma */
static int asc_last_dma_out(asc_softc_t, int, int, int);		/* cleanup after all data is written */
static void asc_end_read_transfer(asc_softc_t asc, State * state);
static void asc_end_write_transfer(asc_softc_t asc, State * state);
static int asc_sendsync(asc_softc_t, int, int, int);		/* negotiate sync xfer */
static int asc_replysync(asc_softc_t, int, int, int);		/* negotiate sync xfer */
static int asc_msg_in(asc_softc_t, int, int, int);		/* process a message byte */
static int asc_disconnect(asc_softc_t, int, int, int);		/* process an expected disconnect */
static int asc_send_command(asc_softc_t, int, int, int);		/* send the command bytes */
void asc_sync_cache_data(target_info_t *tgt, State *state);
static int asc_clean_fifo(asc_softc_t asc, int status, int ss, int ir);
void	asc_build_sglist(State *st, target_info_t *tgt);
static boolean_t asc_get_io(asc_softc_t asc, State *st, unsigned char **address,
		unsigned long *count, boolean_t has_to_poll);
static void asc_flush_fifo(asc_softc_t asc);
int asc_go(target_info_t *tgt, int cmd_count, int in_count,
		boolean_t cmd_only);
static void asc_load_fifo(asc_softc_t asc, unsigned char * buf, int count);
int asc_reset_scsibus(asc_softc_t	asc);


/* Define the index into asc_scripts for various state transitions */
#define	SCRIPT_DATA_IN		0
#define	SCRIPT_CONTINUE_IN	2
#define	SCRIPT_DATA_OUT		3
#define	SCRIPT_CONTINUE_OUT	5
#define	SCRIPT_SIMPLE		6
#define	SCRIPT_GET_STATUS	7
#define	SCRIPT_DONE		8
#define	SCRIPT_MSG_IN		9
#define	SCRIPT_REPLY_SYNC	11
#define	SCRIPT_TRY_SYNC		12
#define	SCRIPT_STATE_DISCONNECT	15
#define	SCRIPT_RESEL		16
#define	SCRIPT_RESUME_IN	17
#define	SCRIPT_RESUME_DMA_IN	18
#define	SCRIPT_RESUME_OUT	19
#define	SCRIPT_RESUME_DMA_OUT	20
#define	SCRIPT_RESUME_NO_DATA	21

#define	SCRIPT_NUM(x)	(x ? x - asc_scripts : -1)

/*
 * Scripts
 */
script_t asc_scripts[] = {
	/* start data in */
	{SCRIPT_MATCH(ASC_INT_FC | ASC_INT_BS, SCSI_PHASE_DATAI),	/*  0 */
		asc_dma_in, ASC_CMD_XFER_INFO | ASC_CMD_DMA,
		&asc_scripts[SCRIPT_DATA_IN + 1]},
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_STATUS),			/*  1 */
		asc_last_dma_in, ASC_CMD_I_COMPLETE,
		&asc_scripts[SCRIPT_GET_STATUS]},

	/* continue data in after a chunk is finished */
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_DATAI),			/*  2 */
		asc_dma_in, ASC_CMD_XFER_INFO | ASC_CMD_DMA,
		&asc_scripts[SCRIPT_DATA_IN + 1]},

	/* start data out */
	{SCRIPT_MATCH(ASC_INT_FC | ASC_INT_BS, SCSI_PHASE_DATAO),	/*  3 */
		asc_dma_out, ASC_CMD_XFER_INFO | ASC_CMD_DMA,
		&asc_scripts[SCRIPT_DATA_OUT + 1]},
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_STATUS),			/*  4 */
		asc_last_dma_out, ASC_CMD_I_COMPLETE,
		&asc_scripts[SCRIPT_GET_STATUS]},

	/* continue data out after a chunk is finished */
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_DATAO),			/*  5 */
		asc_dma_out, ASC_CMD_XFER_INFO | ASC_CMD_DMA,
		&asc_scripts[SCRIPT_DATA_OUT + 1]},

	/* simple command with no data transfer */
	{SCRIPT_MATCH(ASC_INT_FC | ASC_INT_BS, SCSI_PHASE_STATUS),	/*  6 */
		asc_clean_fifo, ASC_CMD_I_COMPLETE,
		&asc_scripts[SCRIPT_GET_STATUS]},

	/* get status and finish command */
	{SCRIPT_MATCH(ASC_INT_FC, SCSI_PHASE_MSG_IN),			/*  7 */
		asc_get_status, ASC_CMD_MSG_ACPT,
		&asc_scripts[SCRIPT_DONE]},
	{SCRIPT_MATCH(ASC_INT_DISC, 0),					/*  8 */
		asc_end, ASC_CMD_NOP,
		&asc_scripts[SCRIPT_DONE]},

	/* message in */
	{SCRIPT_MATCH(ASC_INT_FC, SCSI_PHASE_MSG_IN),			/*  9 */
		asc_msg_in, ASC_CMD_MSG_ACPT,
		&asc_scripts[SCRIPT_MSG_IN + 1]},
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_MSG_IN),			/* 10 */
		script_nop, ASC_CMD_XFER_INFO,
		&asc_scripts[SCRIPT_MSG_IN]},

	/* send synchonous negotiation reply */
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_MSG_OUT),			/* 11 */
		asc_replysync, ASC_CMD_XFER_INFO,
		&asc_scripts[SCRIPT_REPLY_SYNC]},

	/* try to negotiate synchonous transfer parameters */
	{SCRIPT_MATCH(ASC_INT_FC | ASC_INT_BS, SCSI_PHASE_MSG_OUT),	/* 12 */
		asc_sendsync, ASC_CMD_XFER_INFO,
		&asc_scripts[SCRIPT_TRY_SYNC + 1]},
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_MSG_IN),			/* 13 */
		script_nop, ASC_CMD_XFER_INFO,
		&asc_scripts[SCRIPT_MSG_IN]},
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_CMD),			/* 14 */
		asc_send_command, ASC_CMD_XFER_INFO,
		&asc_scripts[SCRIPT_RESUME_NO_DATA]},

	/* handle a disconnect */
	{SCRIPT_MATCH(ASC_INT_DISC, SCSI_PHASE_DATAO),			/* 15 */
		asc_disconnect, ASC_CMD_ENABLE_SEL,
		&asc_scripts[SCRIPT_RESEL]},

	/* reselect sequence: this is just a placeholder so match fails */
	{SCRIPT_MATCH(0, SCSI_PHASE_MSG_IN),				/* 16 */
		script_nop, ASC_CMD_MSG_ACPT,
		&asc_scripts[SCRIPT_RESEL]},

	/* resume data in after a message */
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_DATAI),			/* 17 */
		asc_dma_in, ASC_CMD_XFER_INFO | ASC_CMD_DMA,
		&asc_scripts[SCRIPT_DATA_IN + 1]},

	/* resume partial DMA data in after a message */
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_DATAI),			/* 18 */
		asc_dma_in, ASC_CMD_XFER_INFO | ASC_CMD_DMA,
		&asc_scripts[SCRIPT_DATA_IN + 1]},

	/* resume data out after a message */
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_DATAO),			/* 19 */
		asc_dma_out, ASC_CMD_XFER_INFO | ASC_CMD_DMA,
		&asc_scripts[SCRIPT_DATA_OUT + 1]},

	/* resume partial DMA data out after a message */
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_DATAO),			/* 20 */
		asc_dma_out, ASC_CMD_XFER_INFO | ASC_CMD_DMA,
		&asc_scripts[SCRIPT_DATA_OUT + 1]},

	/* resume after a message when there is no more data */
	{SCRIPT_MATCH(ASC_INT_BS, SCSI_PHASE_STATUS),			/* 21 */
		asc_clean_fifo, ASC_CMD_I_COMPLETE,
		&asc_scripts[SCRIPT_GET_STATUS]},
};


#ifdef ASC_DEBUG
int	asc_ASC_DEBUG = 1;

#define NLOG 32

struct asc_log {
	char	*msg;
	int	sequence;
	int	unit;
	int	target;
	int	script;
	u_long	args[5];
} asc_log[NLOG], *asc_logp = asc_log;

static void	asc_trace_log(asc_softc_t asc,
			char *msg, u_long a1, u_long a2, u_long a3,
			u_long a4, u_long a5);

#define	ASC_TRACE(msg)	asc_trace_log(asc, msg, 0, 0, 0, 0, 0)
#define	ASC_TRACE1(msg, a)	asc_trace_log(asc, msg, (u_long) (a), 0, 0, 0, 0)
#define	ASC_TRACE2(msg, a,b)	asc_trace_log(asc, msg, (u_long) (a), (u_long) (b), 0, 0, 0)
#define	ASC_TRACE3(msg, a,b,c)	asc_trace_log(asc, msg, (u_long) (a), (u_long) (b), (u_long) (c), 0, 0)
#define	ASC_TRACE4(msg, a,b,c,d) \
	asc_trace_log(asc, msg, (u_long) (a), (u_long) (b), (u_long) (c), (u_long)(d), 0)
#define	ASC_TRACE5(msg, a,b,c,d, e) \
	asc_trace_log(asc, msg, (u_long) (a), (u_long) (b), (u_long) (c), (u_long)(d), (u_long)(e))

#else

#define	ASC_TRACE(msg)	/* ; */
#define	ASC_TRACE1(msg, a)	/* ; */
#define	ASC_TRACE2(msg, a,b)	/* ; */
#define	ASC_TRACE3(msg, a,b,c)	/* ; */
#define	ASC_TRACE4(msg, a,b,c,d)	/* ; */
#define	ASC_TRACE5(msg, a,b,c,d,e)	/* ; */
#endif

/*
 * Autoconfiguration data for config.
 */
void	asc_probe_for_targets(asc_softc_t asc, asc_curio_regmap_t *regs);
int	asc_probe( caddr_t reg, void *ptr);
boolean_t asc_probe_target(target_info_t *tgt, io_req_t ior);
void	asc_alloc_cmdptr(asc_softc_t asc, target_info_t *tgt);

/* Various MACH driver variables.. */
caddr_t	asc_std[NASC] = { 0 };
struct	bus_device *asc_dinfo[NASC*8];
struct	bus_ctlr *asc_minfo[NASC];
struct	bus_driver asc_driver = 
        { asc_probe, scsi_slave, scsi_attach, (int(*)(void))asc_go,
	  asc_std, "rz", asc_dinfo, "asc", asc_minfo, BUS_INTR_B4_PROBE};

extern scsi_curio_dma_ops_t scsi_amic_ops, scsi_curio_dbdma_ops;

int	asc_clock_speed_in_mhz[NASC] = {
40
#if	NASC > 1
, 20
#endif /* NASC > 1*/
};

static void
asc_set_flags(unsigned char *env, int flags)
{
	register int unit, i;


	if (strcmp(env, "all") == 0) {
		for (unit = 0; unit < NASC; unit++)
			for (i = 0; i < 7; i++)
				asc_softc_data[unit].st[i].device_flags |= flags;
	} else {
		while (*env) {
			if (*env >= '0' && *env < '7') {
				for (unit = 0; unit < NASC; unit++)
					asc_softc_data[unit].st[*env - '0'].device_flags |= flags;
			}
			env++;
		}
	}

}

static  void
asc_getconfig(void)
{
	register int unit, i;
	unsigned char	*env;
	extern unsigned char *getenv(char *);


	if ((env = getenv("scsi_nodisconnect")) != NULL)
		asc_set_flags(env, DEV_NO_DISCONNECT);

	if (powermac_info.class == POWERMAC_CLASS_PCI) {
		if ((env = getenv("scsi_sync")) != NULL)
			asc_set_flags(env, DEV_RUN_SYNC);
	}

}

				
int
asc_probe(
	caddr_t		reg,
	void		*ptr)
{
	struct bus_ctlr	*ui	 = (struct bus_ctlr *) ptr;
	int		unit	= ui->unit;
	register asc_softc_t asc = &asc_softc_data[unit];
	register asc_curio_regmap_t *regs;
	target_info_t	*tgt;
	scsi_softc_t	*sc;
	int id, s, i, a;
	device_node_t	*curiodev = NULL;


	switch (powermac_info.class) {
	case	POWERMAC_CLASS_PDM:
		asc->dma_ops = &scsi_amic_ops;
		if (unit == 1) 
			asc->regs = (asc_curio_regmap_t *)PDM_ASC_BASE_PHYS;
		else
			asc->regs = (asc_curio_regmap_t *)PDM_ASC2_BASE_PHYS;
		break;


	default:
	case	POWERMAC_CLASS_PCI:
		/* Note - one one curio! */
		if (unit != 1 || (curiodev = find_devices("53c94")) == NULL) 
			return 0;

		asc->dma_ops = &scsi_curio_dbdma_ops;
		asc->regs = (asc_curio_regmap_t *)
				curiodev->addrs[0].address;
		break;

	}

	s = splbio();

	simple_lock_init(&asc->lock, ETAP_IO_DEV);
	simple_lock_init(&asc->queue_lock, ETAP_IO_DEV);

	simple_lock(&asc->lock);

	asc->unit = unit;
	asc_softc[unit] = asc;
	regs = asc->regs = (asc_curio_regmap_t *)
			POWERMAC_IO((vm_offset_t)asc->regs);

	asc->clk = asc_clock_speed_in_mhz[unit];

	if (unit == 1)
		asc->ccf = ASC_CCF_20MHz;
	else
		asc->ccf = FAS_CCF_40MHz;

	asc->timeout = asc_timeout_250(asc->clk, asc->ccf);

	/*
	 * Check to see if there is a chip present.. 
	 */

	regs->asc_cmd = ASC_CMD_FLUSH; eieio();
	delay(100);
	regs->asc_fifo = 0x55; eieio();
	regs->asc_fifo = 0xaa; eieio();

	if (((a = regs->asc_flags & ASC_FLAGS_FIFO_CNT)) != 2) {
		simple_unlock(&asc->lock);
		splx(s);
		return	0;	/* Nothing present.. */
	}


	eieio();

	if ((a = regs->asc_fifo) != 0x55) {
		simple_unlock(&asc->lock);
		splx(s);
		return	0;
	}

	eieio();

	if ((a = regs->asc_fifo) != 0xaa) {
		simple_unlock(&asc->lock);
		splx(s);
		return	0;
	}

	asc->max_scsi_offset = ASC_MAX_OFFSET;

	switch (unit) {
	case	1:
		asc->min_period = ASC_MIN_PERIOD_20;
		asc->type = ASC_NCR_53C94;
		asc->max_transfer = ASC_TC_MAX;
		break;

	case	0:
		asc->min_period = ASC_MIN_PERIOD_40;
		asc->type = ASC_NCR_53CF94;
		asc->max_transfer = FSC_TC_MAX;
		break;

	default:
		printf("asc_probe: Unknown SCSI Bus %d??\n", unit);
		simple_unlock(&asc->lock);
		splx(s);
		return 0;
	}

	if (asc->dma_ops)
		asc->dma_ops->dma_init(unit);

	queue_init(&asc->target_queue);

	sc = scsi_master_alloc(unit, (char *) asc);
	asc->sc = sc;

	sc->go = (void *) asc_go;
	sc->watchdog = scsi_watchdog;
	sc->probe = asc_probe_target;
	asc->wd.reset = ((void (*)(struct watchdog_t *))asc_reset_scsibus);

	sc->max_dma_data = -1;
	sc->max_dma_segs = ASC_MAX_SG_SEGS;

	asc->state = ASC_STATE_IDLE;
	asc->target = -1;

	regs = asc->regs;

	/* preserve our ID for now */
	asc->sc_id = 7; /*regs->asc_cnfg1 & ASC_CNFG1_MY_BUS_ID; eieio();*/
	asc->myidmask = ~(1 << asc->sc_id);

	asc_reset(asc, regs);

	id = asc->sc_id;
	sc->initiator_id = id;

	printf("%s%d: NCR %s %d mhz SCSI ID %d",
		ui->name, unit,
		(asc->type == ASC_NCR_53C94 ? "53C94" : "53CF94"),
		asc->clk, id); 

	tgt = scsi_slave_alloc(sc->masterno, sc->initiator_id, (char *) asc);
	asc_alloc_cmdptr(asc, tgt);
	sccpu_new_initiator(tgt, tgt);
	asc_probe_for_targets(asc, regs);
	asc_getconfig();
	printf("\n");

	switch (unit) {
	case	1:
		if (curiodev)
			pmac_register_ofint(curiodev->intrs[0], SPLBIO, asc_intr);
		else
			pmac_register_int(PMAC_DEV_SCSI0, SPLBIO, asc_intr);
		break;

	case	0:
		pmac_register_int(PMAC_DEV_SCSI1, SPLBIO, asc_intr);
		break;
	}

	simple_unlock(&asc->lock);
	splx(s);

	return 1;
}

boolean_t
asc_probe_target(
	target_info_t 		*tgt,
	io_req_t		ior)
{
	asc_softc_t     asc;

	asc = asc_softc[tgt->masterno];

	if (scsi_inquiry(tgt, SCSI_INQ_STD_DATA) == SCSI_RET_DEVICE_DOWN) 
		return FALSE;

	tgt->flags |= TGT_ALIVE|TGT_CHAINED_IO_SUPPORT;

	return TRUE;
}

void
asc_alloc_cmdptr(asc_softc_t asc, target_info_t *tgt)
{
	tgt->cmd_ptr = asc->cmd_areas[tgt->target_id];
	tgt->dma_ptr = 0;
}

void
asc_probe_for_targets(asc_softc_t asc, asc_curio_regmap_t *regs)
{
	int	target_id, did_banner = 0;
	register unsigned char	csr, ss, ir, ff;
	register scsi_status_byte_t	status;
	scsi_softc_t		*sc = asc->sc;

    	if (asc_probe_dynamically) {
		printf("asc%d: will probe targets on demand\n",
			asc->unit);
		return;
	}

	/*
	 * For all possible targets, see if there is one and allocate
	 * a descriptor for it if it is there.
	 */
	for (target_id = 0; target_id < 8; target_id++) {

		/* except of course ourselves */
		if (target_id == sc->initiator_id)
			continue;

		regs->asc_cmd = ASC_CMD_FLUSH;	/* empty fifo */
		eieio();
		delay(2);

		regs->asc_dbus_id = target_id; eieio();
		regs->asc_sel_timo = asc->timeout; eieio();

		/*
		 * See if the unit is ready.
		 * XXX SHOULD inquiry LUN 0 instead !!!
		 */
		regs->asc_fifo = SCSI_CMD_TEST_UNIT_READY; eieio();
		regs->asc_fifo = 0; eieio();
		regs->asc_fifo = 0; eieio();
		regs->asc_fifo = 0; eieio();
		regs->asc_fifo = 0; eieio();
		regs->asc_fifo = 0; eieio();

		/* select and send it */
		regs->asc_cmd = ASC_CMD_SEL; eieio();

		/* wait for the chip to complete, or timeout */
		csr = asc_wait(regs, ASC_CSR_INT);
		ss = regs->asc_ss; eieio();
		ir = regs->asc_intr; eieio();

		/* empty fifo, there is garbage in it if timeout */
		asc_flush_fifo(asc);

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

		regs->asc_cmd = ASC_CMD_I_COMPLETE; eieio();
		csr = asc_wait(regs, ASC_CSR_INT);
		ir = regs->asc_intr; /* ack intr */
		eieio();

		status.bits = regs->asc_fifo; /* empty fifo */
		eieio();
		ff = regs->asc_fifo; eieio();

		if (status.st.scsi_status_code != SCSI_ST_GOOD)
			scsi_error( 0, SCSI_ERR_STATUS, status.bits, 0);

		regs->asc_cmd = ASC_CMD_MSG_ACPT; eieio();
		csr = asc_wait(regs, ASC_CSR_INT);
		ir = regs->asc_intr; /* ack intr */
		eieio();

		/*
		 * Found a target
		 */
		{
			register target_info_t	*tgt;
			tgt = scsi_slave_alloc(sc->masterno, target_id, (char *)asc);
			asc_alloc_cmdptr(asc, tgt);
			tgt->flags |= TGT_CHAINED_IO_SUPPORT;
		}
	}

} 

int
asc_to_scsi_period(int	a, int	clk)
{
	/* Note: the SCSI unit is 4ns, hence
		A_P * 1,000,000,000
		-------------------  =  S_P
		    C_Mhz  * 4
	 */
	return a * (250 / clk);
		
}

int
scsi_period_to_asc( int p, int clk, int	min_period)
{
	register int 	ret;

	ret = (p * clk) / 250;

	if (ret < min_period)
		return min_period;

	if ((asc_to_scsi_period(ret,clk)) < p)
		return ret + 1;
	return ret;
}


/*
 * Start activity on a SCSI device.
 * We maintain information on each device separately since devices can
 * connect/disconnect during an operation.
 */

int
asc_go(
	target_info_t		*tgt,
	int			cmd_count,
	int			in_count,
	boolean_t		cmd_only)
{
	register asc_softc_t asc;
	State		*state;
	int s;



	asc = (asc_softc_t) tgt->hw_state;

	if (asc->cmd[tgt->target_id]) 
		panic("SCSI Bus %d: Queueing Problem: Target %d busy at start\n", 
			asc->unit, tgt->target_id);

	tgt->transient_state.cmd_count = cmd_count; /* keep it here */

	state = &asc->st[tgt->target_id];

	/*
	 * Setup target state
	 */
	tgt->done = SCSI_RET_IN_PROGRESS;
	state->flags = 0;
	state->buflen = 0;
	state->saved_buflen = 0;
	state->sg_index = 0;
	state->sg_segment_count = 0;

	if ((state->device_flags & DEV_RUN_SYNC) == 0)
		tgt->flags |= TGT_DID_SYNCH;	/* Prevent Sync Transfers*/

	/* 
 	 * WHY doesn't the MACH SCSI drivers tell
	 * the SCSI controllers what the I/O direction
	 * is supposed to be?!? ARGHH..
	 */

	switch (tgt->cur_cmd) {
	case SCSI_CMD_INQUIRY:
		if ((tgt->flags & TGT_DID_SYNCH) == 0) {
			state->script = &asc_scripts[SCRIPT_TRY_SYNC];
			tgt->flags |= TGT_TRY_SYNCH;
			state->flags |= STATE_DMA_IN;
			break;
		} 
		/* Fall thru.. */
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
	case SCSI_CMD_READ:
	case SCSI_CMD_LONG_READ:
		state->script = &asc_scripts[SCRIPT_DATA_IN];
		state->flags |= STATE_DMA_IN;
		break;

	case SCSI_CMD_WRITE:
	case SCSI_CMD_LONG_WRITE:
		state->script = &asc_scripts[SCRIPT_DATA_OUT];
		state->flags |= STATE_DMA_OUT;
		break;

	case SCSI_CMD_MODE_SELECT:
	case SCSI_CMD_REASSIGN_BLOCKS:
	case SCSI_CMD_FORMAT_UNIT:
	case 0xc9: /* vendor-spec: SCSI_CMD_DEC_PLAYBACK_CONTROL */
		tgt->transient_state.cmd_count = sizeof_scsi_command(tgt->cur_cmd);
		tgt->transient_state.out_count =
			cmd_count - tgt->transient_state.cmd_count;
		state->script = &asc_scripts[SCRIPT_DATA_OUT];
		state->flags |= STATE_DMA_OUT|STATE_FIFO_XFER;
		break;

	case SCSI_CMD_TEST_UNIT_READY:
		/*
		 * Do the synch negotiation here, unless prohibited
		 * or done already
		 */

		if ((tgt->flags & TGT_DID_SYNCH) == 0) {
			state->script = &asc_scripts[SCRIPT_TRY_SYNC];
			tgt->flags |= TGT_TRY_SYNCH;
			break;
		} else
			state->script = &asc_scripts[SCRIPT_SIMPLE];
		break;

	 default:
		state->script = &asc_scripts[SCRIPT_SIMPLE];
		break;
	}

	tgt->transient_state.in_count = in_count;

	/* Figure out where to DMA to/from.. 
	  (Again, WHY?!? does MACH do this?!?)
	*/

	switch (tgt->cur_cmd) {
	case	SCSI_CMD_READ:
	case	SCSI_CMD_LONG_READ:
		state->buflen = in_count;
		goto check_chain;

	case	SCSI_CMD_WRITE:
	case	SCSI_CMD_LONG_WRITE:
		tgt->transient_state.out_count = tgt->ior->io_count;
		state->buflen = tgt->ior->io_count; 
		
check_chain:
		if (tgt->ior->io_op & IO_CHAINED) {
			state->flags |= STATE_CHAINED_IO;
		} else 
			state->dma_area = tgt->ior->io_data;
		break;

	case	SCSI_CMD_MODE_SELECT:
	case	SCSI_CMD_REASSIGN_BLOCKS:
	case	SCSI_CMD_FORMAT_UNIT:
	case	0xc9: /* vendor-spec: SCSI_CMD_DEC_PLAYBACK_CONTROL */
		state->flags |= STATE_FIFO_XFER;
		state->buflen = tgt->transient_state.out_count;
		state->dma_area = (u_char *) tgt->cmd_ptr + sizeof(scsi_command_group_0);
		break;

	case SCSI_CMD_INQUIRY:
	case SCSI_CMD_REQUEST_SENSE:
	case SCSI_CMD_MODE_SENSE:
	case SCSI_CMD_RECEIVE_DIAG_RESULTS:
 	case SCSI_CMD_READ_CAPACITY:
	case SCSI_CMD_READ_BLOCK_LIMITS:
	case SCSI_CMD_READ_TOC:
	case SCSI_CMD_READ_SUBCH:
	case SCSI_CMD_READ_HEADER:
		state->flags |= STATE_FIFO_XFER;
		state->buflen = tgt->transient_state.in_count;
		state->dma_area = (u_char *) tgt->cmd_ptr;
		break;

	default:
		state->buflen = 0;
		state->dma_area = 0;
		break;
	}

	/* The following should never happen.. */

	if (asc->dma_ops == NULL) 
		state->flags |= STATE_FIFO_XFER;

	/*
	 * Save off the CDB just in case the CDB and the
	 * area to DMA from/to are in the same cache line.
	 */

	bcopy(tgt->cmd_ptr, state->cdb, tgt->transient_state.cmd_count);

	state->total_count = state->buflen;

	if (state->total_count)
		asc_build_sglist(state, tgt);


	/*
	 * Check if another command is already in progress.
	 * We may have to change this if we allow SCSI devices with
	 * separate LUNs.
	 */

	s = splbio();
	simple_lock(&asc->lock);

	asc->cmd[tgt->target_id] = tgt;

	if (asc->target == -1) 
		asc_startcmd(asc, tgt->target_id);
	else {
		simple_lock(&asc->queue_lock);
		enqueue_tail(&asc->target_queue, (queue_entry_t) tgt);
		simple_unlock(&asc->queue_lock);
	}

	simple_unlock(&asc->lock);
	splx(s);

}

void
asc_start_new_request(asc_softc_t asc)
{
	target_info_t	*tgt;

	simple_lock(&asc->queue_lock);
	tgt = (target_info_t *) dequeue_head(&asc->target_queue);
	simple_unlock(&asc->queue_lock);

	if (tgt)
		asc_startcmd(asc, tgt->target_id);

}

static void
asc_reset(asc_softc_t asc, asc_curio_regmap_t * regs)
{

	asc->wd.nactive = 0;

	/*
	 * Reset chip and wait till done
	 */
	regs->asc_cmd = ASC_CMD_RESET;
	eieio(); delay(25);

	/* spec says this is needed after reset */
	regs->asc_cmd = ASC_CMD_NOP;
	eieio(); delay(25);

	/*
	 * Set up various chip parameters
	 */
	regs->asc_ccf = asc->ccf; eieio();
	delay(25);
	regs->asc_sel_timo = asc->timeout; eieio();

	/* restore our ID */

	regs->asc_cnfg1 = asc->sc_id | ASC_CNFG1_SLOW; eieio();

	switch (asc->type) {
	case	ASC_NCR_53C94:
		regs->asc_cnfg2 = ASC_CNFG2_SCSI2; eieio();
		if (powermac_info.class == POWERMAC_CLASS_PCI) {
			asc->nondma_cnfg3 = asc->dma_cnfg3 = ASC_CNFG3_SRB;
		} else {
			asc->nondma_cnfg3 = ASC_CNFG3_SRB;
			asc->dma_cnfg3 = ASC_CNFG3_T8|ASC_CNFG3_ALT_DMA|
					  ASC_CNFG3_SRB;
		}
		break;

	case	ASC_NCR_53CF94:
		regs->asc_cnfg2 = FAS_CNFG2_FEATURES | ASC_CNFG2_SCSI2;eieio();
		asc->nondma_cnfg3 = ASC_CNFG3_SRB | FAS_CNFG3_FASTSCSI | FAS_CNFG3_FASTCLK;
		asc->dma_cnfg3 = ASC_CNFG3_ALT_DMA|ASC_CNFG3_SRB|
				  ASC_CNFG3_T8 | FAS_CNFG3_FASTSCSI |
				  FAS_CNFG3_FASTCLK;
		regs->asc_cnfg4 = FSC_CNFG4_EAN; eieio();
		regs->fas_tc_hi = 0; eieio();
		break;
	}

	regs->asc_cnfg3 = asc->nondma_cnfg3; eieio();

	/* zero anything else */
	ASC_TC_PUT(regs, 0);
	regs->asc_syn_p = asc->min_period; eieio();
	regs->asc_syn_o = 0;	/* async for now */
	eieio();

}

/*
 * Start a SCSI command on a target.
 */
static void
asc_startcmd(asc_softc_t asc, int target)
{
	register asc_curio_regmap_t *regs;
	register target_info_t *tgt;
	register State *state;
	int len;
	u_char	command;


	regs = asc->regs;

	/*
	 * If a reselection is in progress, it is Ok to ignore it since
	 * the ASC will automatically cancel the command and flush
	 * the FIFO if the ASC is reselected before the command starts.
	 * If we try to use ASC_CMD_DISABLE_SEL, we can hang the system if
	 * a reselect occurs before starting the command.
	 */

	asc->state = ASC_STATE_BUSY;
	asc->target = target;

	/* cache some pointers */
	tgt = asc->cmd[target];
	state = &asc->st[target];
	state->error = SCSI_RET_IN_PROGRESS;

	/*
	 * Init the chip and target state.
	 */
	state->msg_out = SCSI_NOP;
	asc->script = state->script;
	state->script = (script_t *) 0;
	state->dmalen = 0;

	/* preload the FIFO with the message to be sent */
	if (state->device_flags & DEV_NO_DISCONNECT
	|| state->flags & STATE_FIFO_XFER) {
		state->msg_sent = 0;
		regs->asc_fifo = SCSI_IDENTIFY;
	} else {
		state->msg_sent =  SCSI_IFY_ENABLE_DISCONNECT;
		regs->asc_fifo = SCSI_IDENTIFY|SCSI_IFY_ENABLE_DISCONNECT;
	}
		
	eieio();


	if (asc->script != &asc_scripts[SCRIPT_TRY_SYNC])
		/* Don't both sending command if phase is
		 * going to change to Msg-In immediately
		 */
		asc_load_fifo(asc, state->cdb, tgt->transient_state.cmd_count);

	/* initialize the DMA */
	ASC_TC_PUT(regs, 0); readback(regs->asc_cmd);

	regs->asc_dbus_id = target; readback(regs->asc_dbus_id);
	regs->asc_syn_p = tgt->sync_period; readback(regs->asc_syn_p);
	regs->asc_syn_o = tgt->sync_offset; readback(regs->asc_syn_o);

	if (asc->script == &asc_scripts[SCRIPT_TRY_SYNC])
		command = ASC_CMD_SEL_ATN_STOP;
	else
		command = ASC_CMD_SEL_ATN;

	/* Try to avoid reselect collisions */

	if ((regs->asc_csr & (ASC_CSR_INT|SCSI_PHASE_MSG_IN)) !=
	    (ASC_CSR_INT|SCSI_PHASE_MSG_IN))  {
		regs->asc_cmd = command;  
		readback(regs->asc_cmd);

		if (asc->wd.nactive++ == 0)
			asc->wd.watchdog_state = SCSI_WD_ACTIVE;

		/*
		 * Sync the cache here because this is something
		 * that can be done after the request for target
		 * selection has been issued. This helps to
		 * keep the SCSI bus from waiting on the
		 * CPU.
		 * (Possible item to look into, build
		 * S/G list as well here instead of above?
		 * -- Mike Burg 6/10/96)
		 */

		if (powermac_info.io_coherent == FALSE 
		&& state->buflen && !(state->flags & STATE_FIFO_XFER))
			asc_sync_cache_data(tgt, state);

		ASC_TRACE3("start {cmd %x, size %d, script #%d}",
			tgt->cur_cmd, state->buflen, SCRIPT_NUM(asc->script));
	} else 
		ASC_TRACE1("Collision start {cmd %x}", tgt->cur_cmd);

}

/*
 * Interrupt routine
 *	Take interrupts from the chip
 *
 * Implementation:
 *	Move along the current command's script if
 *	all is well, invoke error handler if not.
 */

void
asc_intr(int device, void *ssp)
{
	int	unit;
	register asc_softc_t asc;
	register asc_curio_regmap_t *regs;
	register State *state;
	register script_t *scpt;
	register int ss, ir, status;
	register unsigned char cmd_was;


	unit = (device == PMAC_DEV_SCSI0 ? 1 : 0);
	asc = &asc_softc_data[unit];
	regs =  asc->regs;

	simple_lock(&asc->lock);

	/* collect ephemeral information */
	status = regs->asc_csr; eieio();
again:
	ss = regs->asc_ss; eieio();
	cmd_was = regs->asc_cmd; eieio();

	/* drop spurious interrupts */
	if ((status & ASC_CSR_INT) == 0) {
		simple_unlock(&asc->lock);
		return;
	}

	ir = regs->asc_intr; eieio();	/* this resets the previous two: i.e.,*/
					/* this re-latches CSR (and SSTEP) */
	scpt = asc->script;


	ASC_TRACE5("intr {status %x, ss %x, ir %x, cond %d:%x}",
		status, ss, ir, SCRIPT_NUM(scpt), scpt ? scpt->condition : -1);

	if (scpt == 0) {
#if	ASC_DEBUG
		asc_DumpLog("NIL Script Pointer!");
#endif
		panic("SCSI State Error - Script ptr is NIL?!?!");
	}

	/* check the expected state */
	if (SCRIPT_MATCH(ir, status) == scpt->condition) {
		/*
		 * Perform the appropriate operation, then proceed.
		 */
do_command:
		if ((*scpt->action)(asc, status, ss, ir)) {
			regs->asc_cmd = scpt->command;
			readback(regs->asc_cmd);
			ASC_TRACE2("script match {cmd %x, new script %d}",
				scpt->command, SCRIPT_NUM(scpt->next));
			asc->script = scpt->next;
		} else
			ASC_TRACE1("script no-match {script %d}",
				SCRIPT_NUM(asc->script));
		goto done;
	}

	/*
	 * Check for parity error.
	 * Hardware will automatically set ATN
	 * to request the device for a MSG_OUT phase.
	 */
	if (status & ASC_CSR_PE) {
		printf("SCSI Bus %d target %d: incomming parity error seen\n",
			asc->unit, asc->target);
		asc->st[asc->target].flags |= STATE_PARITY_ERR;
	}

	/*
	 * Check for gross error.
	 * Probably a bug in a device driver.
	 */
	if (status & ASC_CSR_GE && !(ir & ASC_INT_BS)) {
		printf("SCSI Bus %d target %d: gross error\n",
			asc->unit, asc->target);
		goto abort;
	}

	/* check for message in or out */
	if ((ir & ~ASC_INT_FC) == ASC_INT_BS) {
		register int len, fifo;

		state = &asc->st[asc->target];

		if (state->flags & STATE_DMA_RUNNING) {
			if (state->flags & STATE_DMA_IN) 
				asc_end_read_transfer(asc, state);
			else
				asc_end_write_transfer(asc, state);
		} 

		/* Flush the FIFO if needed .. */
		if (regs->asc_flags & ASC_FLAGS_FIFO_CNT)
			asc_flush_fifo(asc);

		switch (ASC_PHASE(status)) {
		case SCSI_PHASE_DATAI:
		case SCSI_PHASE_DATAO:
			if (!state->buflen || (state->flags & (STATE_DMA_IN|STATE_DMA_OUT)) == 0) {
				asc_DumpLog("Device transfer overrun");
				panic("SCSI Interrupt Phase Error");
			} else if (state->flags & STATE_DMA_IN) 
				state->script = &asc_scripts[SCRIPT_RESUME_DMA_IN];
			else
				state->script = &asc_scripts[SCRIPT_RESUME_DMA_OUT];
			scpt = state->script;
			goto do_command;

		case SCSI_PHASE_MSG_IN:
			break;

		case SCSI_PHASE_MSG_OUT:
			/*
			 * Check for parity error.
			 * Hardware will automatically set ATN
			 * to request the device for a MSG_OUT phase.
			 */
			if (state->flags & STATE_PARITY_ERR) {
				state->flags &= ~STATE_PARITY_ERR;
				state->msg_out = SCSI_MSG_PARITY_ERROR;
				/* reset message in counter */
				state->msglen = 0;
			} else
				state->msg_out = SCSI_NOP;
			ASC_TRACE1("new msg_in {msg %x}", state->msg_out);
			regs->asc_fifo = state->msg_out; eieio();
			regs->asc_cmd = ASC_CMD_XFER_INFO;
			readback(regs->asc_cmd);
			goto done;

		case SCSI_PHASE_STATUS:
			/* probably an error in the SCSI command */
			asc->script = &asc_scripts[SCRIPT_GET_STATUS];
			regs->asc_cmd = ASC_CMD_I_COMPLETE;
			readback(regs->asc_cmd);
			goto done;

		case	SCSI_PHASE_CMD:
			/* Typically happens when Sync Neg. occurs */
			asc_send_command(asc, status, ss, ir);
			regs->asc_cmd = ASC_CMD_XFER_INFO;
			readback(regs->asc_cmd);

			if (state->flags & STATE_DMA_IN)
				asc->script = &asc_scripts[SCRIPT_DATA_IN];
			else if (state->flags & STATE_DMA_OUT)
				asc->script = &asc_scripts[SCRIPT_DATA_OUT];
			else
				asc->script = &asc_scripts[SCRIPT_RESUME_NO_DATA];
			goto done;

		default:
			goto abort;
		}

		if (state->script)
			goto abort;

		if (state->flags & (STATE_DMA_IN|STATE_DMA_OUT)) {
			if (state->buflen)
				state->script = &asc_scripts[state->flags & STATE_DMA_IN ? SCRIPT_RESUME_DMA_IN : SCRIPT_RESUME_DMA_OUT];
			else
				state->script = &asc_scripts[SCRIPT_RESUME_NO_DATA];
		} else if (asc->script == &asc_scripts[SCRIPT_SIMPLE])
			state->script = &asc_scripts[SCRIPT_RESUME_NO_DATA];
		else
			state->script = asc->script;

		/* setup to receive a message */
		asc->script = &asc_scripts[SCRIPT_MSG_IN];
		state->msglen = 0;
		regs->asc_cmd = ASC_CMD_XFER_INFO;
		readback(regs->asc_cmd);
		goto done;
	}

	/* check for SCSI bus reset */
	if (ir & ASC_INT_RESET) {
		register int i;

		printf("SCSI Bus %d: SCSI bus reset!!\n", asc->unit);
		/* rearbitrate synchronous offset */
		for (i = 0; i < ASC_NCMD; i++) {
			asc->st[i].flags = 0;
			asc->cmd[i] = NULL;
		}
		asc->target = -1;
		asc->wd.nactive = 0;
		asc_reset(asc, regs);
		scsi_bus_was_reset(asc->sc);
		simple_unlock(&asc->lock);
		return; /* XXX ??? */
	}

	/* check for command errors 
	 * However, if the command was a selection, then
	 * it means the chip was in the process of being
	 * reselected
	 */

	if (ir & ASC_INT_ILL && (cmd_was & ASC_CMD_SEL_ATN) == 0) 
		goto abort;

	/* check for disconnect */
	if (ir & ASC_INT_DISC) {
		state = &asc->st[asc->target];
		switch (asc->script - asc_scripts) {
		case SCRIPT_DONE:
		case SCRIPT_STATE_DISCONNECT:
			/*
			 * Disconnects can happen normally when the
			 * command is complete with the phase being
			 * either SCSI_PHASE_DATAO or SCSI_PHASE_MSG_IN.
			 * The SCRIPT_MATCH() only checks for one phase
			 * so we can wind up here.
			 * Perform the appropriate operation, then proceed.
			 */
			if ((*scpt->action)(asc, status, ss, ir)) {
				regs->asc_cmd = scpt->command;
				readback(regs->asc_cmd);
				ASC_TRACE2("disconn match {cmd %x, new script %d}",
					scpt->command, SCRIPT_NUM(scpt->next));
				asc->script = scpt->next;
			} else
				ASC_TRACE1("disconn non-match {script %d}",
					SCRIPT_NUM(asc->script));
				
			goto done;

		case SCRIPT_TRY_SYNC:
		case SCRIPT_SIMPLE:
		case SCRIPT_DATA_IN:
		case SCRIPT_DATA_OUT: /* one of the starting scripts */
			if (ASC_SS(ss) == 0) {
				ASC_TRACE("no response");
				/* device did not respond */
				if (regs->asc_flags & ASC_FLAGS_FIFO_CNT) {
					regs->asc_cmd = ASC_CMD_FLUSH;
					readback(regs->asc_cmd);
				}
				state->error = SCSI_RET_DEVICE_DOWN;
				asc_end(asc, status, ss, ir);
				simple_unlock(&asc->lock);
				return; /* XXX ??? */
			}
			/* FALLTHROUGH */

		default:
			printf("SCSI Bus target %d: unexpected disconnect\n",
				asc->unit, asc->target);
#ifdef ASC_DEBUG
			asc_DumpLog("asc_disc");
#endif
			/*
			 * On rare occasions my RZ24 does a disconnect during
			 * data in phase and the following seems to keep it
			 * happy.
			 * XXX Should a scsi disk ever do this??
			 */
			asc->script = &asc_scripts[SCRIPT_RESEL];
			asc->state = ASC_STATE_RESEL;
			state->flags |= STATE_DISCONNECTED;
			regs->asc_cmd = ASC_CMD_ENABLE_SEL;
			readback(regs->asc_cmd);
			simple_unlock(&asc->lock);
			return; /* XXX ??? */
		}
	}

	/* check for reselect */
	if (ir & ASC_INT_RESEL
	|| ((ir & ASC_INT_ILL) && (cmd_was & ASC_CMD_SEL_ATN) == 0)) {
		unsigned fifo, id, msg;

		/* Was another target in the process of being
		 * started and got bumpped by a reselection?
		 */
		if (asc->target >= 0)  {
			asc->st[asc->target].script = asc->script;
			asc->wd.nactive--;

			/* Queue it back up as the next request */
			enqueue_head(&asc->target_queue, (queue_entry_t) asc->cmd[asc->target]);
		}
		
		fifo = regs->asc_flags & ASC_FLAGS_FIFO_CNT; eieio();
		if (fifo < 2)
			goto abort;
		/* read unencoded SCSI ID and convert to binary */
		msg = regs->asc_fifo & asc->myidmask; eieio();
		for (id = 0; (msg & 1) == 0; id++)
			msg >>= 1;
		/* read identify message */
		msg = regs->asc_fifo; eieio();

		asc->state = ASC_STATE_BUSY;
		asc->target = id;
		state = &asc->st[id];
		asc->script = state->script;
		state->script = (script_t *)0;
		if (!(state->flags & STATE_DISCONNECTED))
			goto abort;
		state->flags &= ~STATE_DISCONNECTED;
		regs->asc_syn_p = asc->cmd[id]->sync_period; eieio();
		regs->asc_syn_o = asc->cmd[id]->sync_offset; eieio();
		regs->asc_cmd = ASC_CMD_MSG_ACPT; eieio();
		ASC_TRACE2("reselected {targ %d, msg %x}", id, msg);
		goto done;
	}

	/* check if we are being selected as a target */
	if (ir & (ASC_INT_SEL | ASC_INT_SEL_ATN))
		goto abort;

	/*
	 * 'ir' must be just ASC_INT_FC.
	 * This is normal if canceling an ASC_ENABLE_SEL.
	 */

done:
	eieio();
	/* watch out for HW race conditions and setup & hold time violations */
	ir = regs->asc_csr; eieio();
	while (ir != (status = regs->asc_csr)) {
		ir = status;
		eieio();
	}

	if (status & ASC_CSR_INT)
		goto again;

	simple_unlock(&asc->lock);
	return; /* XXX ??? */

abort:
#if ASC_DEBUG
	asc_DumpLog("Interrupt Problem");
#endif
	panic("SCSI Interrupt Error");
	simple_unlock(&asc->lock);
}

/*
 * All the many little things that the interrupt
 * routine might switch to.
 */

/* ARGSUSED */
static int
script_nop(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	return (1);
}

static int
asc_clean_fifo(asc_softc_t asc, int status, int ss, int ir)
{
	asc_curio_regmap_t	*regs = asc->regs;
	unsigned char ff;


	ASC_TC_PUT(regs,0);	/* stop dma engine */
	readback(regs->asc_cmd);

	eieio();
	while (regs->asc_flags & ASC_FLAGS_FIFO_CNT) {
		ff = regs->asc_fifo;
		eieio();
	}

	return TRUE;
}

static void
asc_flush_fifo(asc_softc_t asc)
{
	asc_curio_regmap_t	*regs = asc->regs;


	regs->asc_cmd = ASC_CMD_FLUSH; eieio();
	readback(regs->asc_cmd);
	regs->asc_cmd = ASC_CMD_NOP; eieio();
	readback(regs->asc_cmd);
	regs->asc_cmd = ASC_CMD_NOP; eieio();
	readback(regs->asc_cmd);

	return;
}


static int
asc_send_command(asc_softc_t asc, int status, int ss, int ir)
{
	target_info_t	*tgt = asc->cmd[asc->target];
	State		*st = &asc->st[asc->target];


	asc_load_fifo(asc, (unsigned char *) st->cdb, 
			tgt->transient_state.cmd_count);

	return (1);
}

/* ARGSUSED */
static int
asc_get_status(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register asc_curio_regmap_t *regs = asc->regs;
	register int data;
	scsi2_status_byte_t	statusByte;
	State			*st = &asc->st[asc->target];


	/*
	 * Get the last two bytes in the FIFO.
	 */

	data = regs->asc_flags;	eieio();
	
	if ((data & ASC_FLAGS_FIFO_CNT) != 2) {
#ifdef ASC_DEBUG
		printf("asc_get_status: cmdreg %x, fifo cnt %d\n",
		       regs->asc_cmd, data); /* XXX */
		asc_DumpLog("Get Status FIFO problem"); /* XXX */
#endif
		if (data < 2) {
			asc->regs->asc_cmd = ASC_CMD_MSG_ACPT;
			readback(asc->regs->asc_cmd);
			return (0);
		}
		do {
			eieio();
			data = regs->asc_fifo; eieio();
		} while ((regs->asc_flags & ASC_FLAGS_FIFO_CNT) > 2);
	}

	/* save the status byte */
	statusByte.bits = regs->asc_fifo; eieio();

	if (statusByte.st.scsi_status_code != SCSI_ST_GOOD) {
		st->error = (statusByte.st.scsi_status_code == SCSI_ST_BUSY) ?
			SCSI_RET_RETRY : SCSI_RET_NEED_SENSE;
	} else
		st->error = SCSI_RET_SUCCESS;

	data = regs->asc_fifo; eieio();

	/* get the (presumed) command_complete message */
	if (data == SCSI_COMMAND_COMPLETE) {
		ASC_TRACE1("getstatus okay {st %x}", asc->st[asc->target].statusByte);
		return (1);
	}

	ASC_TRACE2("getstatus prob? {st %x, data %x}", asc->st[asc->target].statusByte,
			data);
#ifdef ASC_DEBUG
	printf("asc_get_status: status %x cmd %x\n",
		asc->st[asc->target].statusByte, data);
	asc_DumpLog("Get Status Error");
#endif
	return (0);
}

/* ARGSUSED */
static int
asc_end(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register target_info_t *tgt;
	register State *state;
	register int i, target;

	asc->state = ASC_STATE_IDLE;
	target = asc->target;
	asc->target = -1;
	tgt = asc->cmd[target];
	asc->cmd[target] = (target_info_t *)0;
	state = &asc->st[target];

	ASC_TRACE5("end {bus%d target %d cmd %x err %d resid %d}",
	  asc->unit, target, tgt->cmd_ptr[0], state->error, state->buflen);

	/* look for disconnected devices */
	for (i = 0; i < ASC_NCMD; i++) {
		if (!asc->cmd[i] || !(asc->st[i].flags & STATE_DISCONNECTED))
			continue;
		ASC_TRACE1("end {reconn tgt %d}", i);
		asc->regs->asc_cmd = ASC_CMD_ENABLE_SEL;
		readback(asc->regs->asc_cmd);
		asc->state = ASC_STATE_RESEL;
		asc->script = &asc_scripts[SCRIPT_RESEL];
		break;
	}

	/*
	 * Look for another device that is ready.
	 * May want to keep last one started and increment for fairness
	 * rather than always starting at zero.
	 */

	simple_lock(&asc->queue_lock);
	if (!queue_empty(&asc->target_queue)) {
		simple_unlock(&asc->queue_lock);
		asc_start_new_request(asc);
	} else
		simple_unlock(&asc->queue_lock);

	tgt->done = state->error;

	if (tgt->done == SCSI_RET_IN_PROGRESS)
		tgt->done = SCSI_RET_SUCCESS;

	if (asc->wd.nactive-- == 1)
		asc->wd.watchdog_state = SCSI_WD_INACTIVE;

	/* Note - SCSI driver may call into itself */
	simple_unlock(&asc->lock);

	if (tgt->ior) {
		(*tgt->dev_ops->restart)(tgt, TRUE);
	}

	simple_lock(&asc->lock);

	return (0);
}

static int
asc_dma_in(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register asc_curio_regmap_t *regs = asc->regs;
	register State *state = &asc->st[asc->target];
	unsigned char *address;
	long	len;


	/* check for previous chunk in buffer */
	if (state->flags & STATE_DMA_RUNNING) 
		asc_end_read_transfer(asc, state);

	state->flags |= STATE_DMA_RUNNING;

	regs->asc_cnfg1 = asc->sc_id; eieio();	/* Turn off slow cable mode.. */

	/* setup to start reading the next chunk */
	if (asc_get_io(asc, state, &address, &len, FALSE)) {

		regs->asc_cnfg3 = asc->dma_cnfg3; eieio();
		asc->dma_ops->dma_setup_transfer(asc->unit,
					(vm_offset_t) address, len, TRUE);
		state->dmalen = len;
		ASC_TC_PUT(regs, len);
		/* check for next chunk */
		regs->asc_cmd = ASC_CMD_XFER_INFO | ASC_CMD_DMA;
		readback(regs->asc_cmd);
		asc->dma_ops->dma_start_transfer(asc->unit, TRUE);

	} else {
		regs->asc_cnfg3 = asc->nondma_cnfg3; eieio();
		len = state->dmalen = 1;
		state->flags |= STATE_DMA_FIFO;
		regs->asc_cmd = ASC_CMD_XFER_INFO;
		readback(regs->asc_cmd);
	}

	ASC_TRACE2("read dma {xfer %d/left %d}", len, state->buflen);

	if (len != state->buflen) 
		asc->script = &asc_scripts[SCRIPT_CONTINUE_IN];
	else 
		asc->script = asc->script->next;

	return (0);
}

static void
asc_end_read_transfer(asc_softc_t asc, State * state)
{
	asc_curio_regmap_t *regs = asc->regs;
	unsigned long len, fifo, count;
	unsigned char *address;


	/*
	 * Only count bytes that have been copied to memory.
	 * There may be some bytes in the FIFO if synchonous transfers
	 * are in progress.
	 */
	if ((state->flags & STATE_DMA_FIFO) == 0) {
		ASC_TC_GET(regs, len);
		asc->dma_ops->dma_end(asc->unit, TRUE);
		len = state->dmalen - len;
		state->buflen -= len;

		ASC_TRACE2("read dma end {resid %d, left %d}",
			len, state->buflen);
	}

	regs->asc_cnfg1 = asc->sc_id | ASC_CNFG1_SLOW; eieio();

	regs->asc_cnfg3 = asc->nondma_cnfg3; eieio();

	len = regs->asc_flags & ASC_FLAGS_FIFO_CNT; eieio();

	if (len) {
		ASC_TRACE2("read fifo end {fifo %d, left %d}",
			len, (state->buflen - len));

		while (len-- > 0 && state->buflen > 0) {
			(void) asc_get_io(asc, state, &address, &count, TRUE);

			/* Try to compenstate for cache 
			 * synch problems..
			 */

			if (powermac_info.io_coherent == FALSE
			&& (state->flags & STATE_FIFO_XFER) == 0)
				invalidate_dcache((vm_offset_t) address,
						  1,
						  FALSE);

			*address = regs->asc_fifo; eieio();
			state->buflen--;

			if (powermac_info.io_coherent == FALSE
			&& (state->flags & STATE_FIFO_XFER) == 0)
				flush_dcache((vm_offset_t) address, 1, FALSE);
		}

		if (len)
			asc_flush_fifo(asc);
	}

	state->flags &= ~(STATE_DMA_RUNNING|STATE_DMA_FIFO);

}


/* ARGSUSED */
static int
asc_last_dma_in(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register asc_curio_regmap_t *regs = asc->regs;
	register State *state = &asc->st[asc->target];
	register int len, fifo;


	asc_end_read_transfer(asc, state);

	return (1);
}

static void
asc_end_write_transfer(asc_softc_t asc, State *state)
{
	register asc_curio_regmap_t *regs = asc->regs;
	unsigned long len, fifo;


	/* check to be sure previous chunk was finished */
	if ((state->flags & STATE_DMA_FIFO) == 0) {
		ASC_TC_GET(regs, len);
		asc->dma_ops->dma_end(asc->unit, FALSE);
	} else
		len = 0;

	regs->asc_cnfg1 = asc->sc_id | ASC_CNFG1_SLOW; eieio();

	regs->asc_cnfg3 = asc->nondma_cnfg3; eieio();

	fifo = regs->asc_flags & ASC_FLAGS_FIFO_CNT; eieio();

	ASC_TRACE2("end write {resid %d, buflen %d}",
			fifo+len, state->buflen);

	if (fifo) { 
		asc_flush_fifo(asc);
		len += fifo;
	}

	len = state->dmalen - len;
	state->buflen -= len;

	state->flags &= ~(STATE_DMA_FIFO|STATE_DMA_RUNNING);

}

/* ARGSUSED */
static int
asc_dma_out(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register asc_curio_regmap_t *regs = asc->regs;
	register State *state = &asc->st[asc->target];
	unsigned char *address;
	unsigned long  len, fifo;


	if (state->flags & STATE_DMA_RUNNING) 
		asc_end_write_transfer(asc, state);

	state->flags |= STATE_DMA_RUNNING;

	regs->asc_cnfg1 = asc->sc_id; eieio();	/* Turn off slow cable mode */

	/* check for next chunk */
	if (asc_get_io(asc, state, &address, &len, FALSE)) {
		regs->asc_cnfg3 = asc->dma_cnfg3; eieio();
		asc->dma_ops->dma_setup_transfer(asc->unit,
					(vm_offset_t) address, len, FALSE);
		state->dmalen = len;
		ASC_TC_PUT(regs, len);
		regs->asc_cmd = ASC_CMD_XFER_INFO | ASC_CMD_DMA;
		readback(regs->asc_cmd);
		asc->dma_ops->dma_start_transfer(asc->unit, FALSE);
	} else {
		regs->asc_cnfg3 = asc->nondma_cnfg3; eieio();
		asc_load_fifo(asc, address, len);
		state->dmalen = len;
		state->flags |= STATE_DMA_FIFO;
		regs->asc_cmd = ASC_CMD_XFER_INFO;
		readback(regs->asc_cmd);
	}


	ASC_TRACE2("dma out {xfer %d, left %d}", len, state->buflen);

	if (len != state->buflen) 
		asc->script = &asc_scripts[SCRIPT_CONTINUE_OUT];
	else
		asc->script = asc->script->next;

	return (0);
}

/* ARGSUSED */
static int
asc_last_dma_out(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register State *state = &asc->st[asc->target];


	asc_end_write_transfer(asc, state);

	return (1);
}

/* ARGSUSED */
static int
asc_sendsync(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register asc_curio_regmap_t *regs = asc->regs;
	register State *state = &asc->st[asc->target];


	ASC_TRACE2("sendsync {per %d, off %d}",
		asc->min_period, asc->max_scsi_offset);

	/* send the extended synchronous negotiation message */
	regs->asc_fifo = SCSI_EXTENDED_MESSAGE;
	eieio();
	regs->asc_fifo = 3;
	eieio();
	regs->asc_fifo = SCSI_SYNC_XFER_REQUEST;
	eieio();
	regs->asc_fifo = asc_to_scsi_period(asc->min_period, asc->clk);
	eieio();
	regs->asc_fifo = asc->max_scsi_offset;
	/* state to resume after we see the sync reply message */
	state->script = asc->script + 2;
	state->msglen = 0;
	state->msg_sent = SCSI_SYNC_XFER_REQUEST;

	return (1);
}

/* ARGSUSED */
static int
asc_replysync(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register asc_curio_regmap_t *regs = asc->regs;
	register State *state = &asc->st[asc->target];
	target_info_t		*tgt = asc->cmd[asc->target];


	ASC_TRACE2("replysync {per %d, off %d}",
		tgt->sync_period, tgt->sync_offset);

	/* send synchronous transfer in response to a request */
	regs->asc_fifo = SCSI_EXTENDED_MESSAGE; eieio();
	regs->asc_fifo = 3; eieio();
	regs->asc_fifo = SCSI_SYNC_XFER_REQUEST; eieio();
	regs->asc_fifo = asc_to_scsi_period(tgt->sync_period, asc->clk); eieio();
	regs->asc_fifo = tgt->sync_offset; eieio();
	regs->asc_cmd = ASC_CMD_XFER_INFO;
	readback(regs->asc_cmd);

	state->msg_sent = SCSI_SYNC_XFER_REQUEST;

	/* return to the appropriate script */
	if (!state->script) {
#if	ASC_DEBUG
		asc_DumpLog("Reply Sync Error");
#endif
		panic("SCSI Error - NIL Script for reply sync");
	}
	asc->script = state->script;
	state->script = (script_t *)0;

	return (0);
}

/* ARGSUSED */
static int
asc_msg_in(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register asc_curio_regmap_t *regs = asc->regs;
	register State *state = &asc->st[asc->target];
	target_info_t	*tgt = asc->cmd[asc->target];
	register int msg;
	int i;


	/* read one message byte */
	msg = regs->asc_fifo; eieio();

	ASC_TRACE2("msg_in {msg %x, len %d}",
		msg, state->msglen);

	/* check for multi-byte message */
	if (state->msglen != 0) {
		/* first byte is the message length */
		if (state->msglen < 0) {
			state->msglen = msg;
			return (1);
		}
		if (state->msgcnt >= state->msglen)
			goto abort;
		state->msg_in[state->msgcnt++] = msg;

		/* did we just read the last byte of the message? */
		if (state->msgcnt != state->msglen) {
			return (1);
		}

		/* process an extended message */
		switch (state->msg_in[0]) {
		case SCSI_SYNC_XFER_REQUEST:
			tgt->flags |= TGT_DID_SYNCH;
			tgt->sync_offset = state->msg_in[2];

			/* convert SCSI period to ASC period */
			i = state->msg_in[1];

			tgt->sync_period = scsi_period_to_asc(i, asc->clk, asc->min_period);

			/*
			 * If this is a request, check minimums and
			 * send back an acknowledge.
			 */
			if (!(tgt->flags & TGT_TRY_SYNCH)) {
				regs->asc_cmd = ASC_CMD_SET_ATN;
				readback(regs->asc_cmd);

				if (tgt->sync_period < asc->min_period)
					tgt->sync_period =
						asc->min_period;

				if (tgt->sync_offset > asc->max_scsi_offset)
					tgt->sync_offset = asc->max_scsi_offset;

				/* Is device allowed to run sync? */
				if ((state->device_flags & DEV_RUN_SYNC) == 0)
					tgt->sync_offset = 0;

				asc->script = &asc_scripts[SCRIPT_REPLY_SYNC];
				regs->asc_syn_p = tgt->sync_period;
				readback(regs->asc_syn_p);
				regs->asc_syn_o = tgt->sync_offset;
				readback(regs->asc_syn_o);
				regs->asc_cmd = ASC_CMD_MSG_ACPT;
				readback(regs->asc_cmd);
				return (0);
			}

			regs->asc_syn_p = tgt->sync_period;
			readback(regs->asc_syn_p);
			regs->asc_syn_o = tgt->sync_offset;
			readback(regs->asc_syn_o);
			tgt->flags &= ~TGT_TRY_SYNCH;
			goto done;

		default:
			printf("SCSI Bus %d target %d: rejecting extended message 0x%x\n",
				asc->unit, asc->target,
				state->msg_in[0]);
			goto reject;
		}
	}

	/* process first byte of a message */

	switch (msg) {
	case SCSI_MESSAGE_REJECT:
		switch (state->msg_sent) {
		case	SCSI_SYNC_XFER_REQUEST:
			tgt->flags |= TGT_DID_SYNCH;	/* No sync transfers */
			tgt->flags &= ~TGT_TRY_SYNCH;
			state->device_flags &= ~DEV_RUN_SYNC;
			tgt->sync_offset = 0;
			break;

		case	SCSI_IFY_ENABLE_DISCONNECT:
			state->device_flags &= ~DEV_NO_DISCONNECT;
			break;

		}
		break;

	case SCSI_EXTENDED_MESSAGE: /* read an extended message */
		/* setup to read message length next */
		state->msglen = -1;
		state->msgcnt = 0;
		return (1);

	case SCSI_NOP:
		break;

	case SCSI_SAVE_DATA_POINTER:
		state->saved_buflen = state->buflen;
		state->saved_sg_index  = state->sg_index;
		/* expect another message (Disconnect) */
		return (1);

	case SCSI_RESTORE_POINTERS:
		state->buflen = state->saved_buflen;
		state->sg_index = state->saved_sg_index;
		break;

	case SCSI_DISCONNECT:
		if (state->flags & STATE_DISCONNECTED)
			goto abort;
		state->flags |= STATE_DISCONNECTED;
		regs->asc_cmd = ASC_CMD_MSG_ACPT;
		readback(regs->asc_cmd);
		asc->script = &asc_scripts[SCRIPT_STATE_DISCONNECT];
		return (0);

	default:
		printf(" SCSI Bus %d target %d: rejecting message 0x%x\n",
			asc->unit, asc->target, msg);
	reject:
		/* request a message out before acknowledging this message */
		state->msg_out = SCSI_MESSAGE_REJECT;
		regs->asc_cmd = ASC_CMD_SET_ATN;
		readback(regs->asc_cmd);
	}

done:
	/* return to original script */
	regs->asc_cmd = ASC_CMD_MSG_ACPT;
	readback(regs->asc_cmd);
	if (!state->script) {
	abort:
#if ASC_DEBUG
		asc_DumpLog("Message In Error");
#endif
		panic("SCSI Error - NIL Script in Msg-In");
	}
	asc->script = state->script;
	state->script = (script_t *)0;
	return (0);
}

/* ARGSUSED */
static int
asc_disconnect(asc, status, ss, ir)
	register asc_softc_t asc;
	register int status, ss, ir;
{
	register State *state = &asc->st[asc->target];


	ASC_TRACE("disconnect");
#ifdef DIAGNOSTIC
	if (!(state->flags & STATE_DISCONNECTED)) {
		printf("asc_disconnect: device %d: STATE_DISCONNECTED not set!\n",
			asc->target);
	}
#endif /*DIAGNOSTIC*/
	asc->target = -1;

	asc->state = ASC_STATE_RESEL;

	asc_flush_fifo(asc);

	if (!queue_empty(&asc->target_queue)) {
		asc->regs->asc_cmd = ASC_CMD_ENABLE_SEL;
		readback(asc->regs->asc_cmd);

		asc_start_new_request(asc);
		return 0;
	} else {
		asc->state = ASC_STATE_RESEL;
		return (1);
	}
}

static void 
asc_load_fifo(asc_softc_t asc, unsigned char * buf, int count)
{
	asc_curio_regmap_t	*regs = asc->regs;


	while(count-- > 0) {
		regs->asc_fifo = *buf++; eieio();
	}

	return;
}

int
asc_wait(
	asc_curio_regmap_t	*regs,
	int			until)
{
	int timeo = 1000000;
	unsigned char 	value;


	while ((regs->asc_csr & until) == 0) {
		eieio();
		delay(1);
		if (!timeo--) {
			printf("asc_wait TIMEO with x%x\n", regs->asc_csr);
			break;
		}
	}
	value = regs->asc_csr; eieio();

	return value;
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
	register target_info_t		*tgt;
	register asc_curio_regmap_t	*regs;
	register int			ir;

	simple_lock(&asc->lock);

	tgt = asc->cmd[asc->target];
	regs = asc->regs;

#if ASC_DEBUG
	asc_DumpLog("Request Timeout");
	printf("SCSI Timeout - cmd %x csr %x ",
		regs->asc_cmd, regs->asc_csr);
	ir = regs->asc_intr; eieio();
	printf(" ir %x\n", ir);
#endif
		
	regs->asc_cmd = ASC_CMD_BUS_RESET; eieio();
	delay(35);

	simple_unlock(&asc->lock);
}

void 
asc_construct_entry(State *st, vm_offset_t address, long count, long offset)
{
	struct sg_segment *sgp = &st->sg_list[st->sg_segment_count],
			  *prev_sgp;
	vm_offset_t	physaddr;
	long	real_count;
	int	i;


	if ((i = st->sg_segment_count) != 0)
		prev_sgp = &st->sg_list[i-1];
	else
		prev_sgp = NULL;

	for (i = st->sg_segment_count; i < ASC_MAX_SG_SEGS && count > 0;) {
		physaddr = kvtophys(address);

		if (physaddr == 0) {
			printf("asc_construct_entry(0x%x,0x%x,0x%x,0x%x)\n",
			       (unsigned int)st, address, count, offset);
			printf("kvtophys(0x%x)=0x%x\n",address,physaddr);
			printf("real_count=0x%x, i=0x%x\n",real_count,i);
			for (i = 0; i < 10; i++) {
				printf("%d : kvtophys = 0x%x\n",
				       kvtophys(address));
				delay(1);
			}

			panic("SCSI DMA - Zero physical address");
		}

		real_count = 0x1000 - (physaddr & 0xfff);

		real_count = u_min(real_count, count);

		/*
		 * Check to see if this address and count
		 * can be combined with the previous 
		 * S/G element. This helps to reduce the
		 * number of interrupts.
		 */

		if (prev_sgp && (prev_sgp->sg_physaddr+prev_sgp->sg_count) == physaddr) {
			prev_sgp->sg_count += real_count;

			if (prev_sgp->sg_count & 0x7)
				st->flags |= STATE_FIFO_XFER;
		} else {
			if (physaddr & 0x7 || real_count & 0x7)
				st->flags |= STATE_FIFO_XFER;

			sgp->sg_offset = offset;
			sgp->sg_count = real_count;
			sgp->sg_physaddr = physaddr;
			sgp->sg_vmaddr = (unsigned char *) address;
			prev_sgp =  sgp++;
			i++;
		}

		count -= real_count;
		offset += real_count;
		address += real_count;
	}

	if (count)
		panic("SCSI - S/G overflow.");

	st->sg_segment_count = i;

}

void
asc_build_sglist(State *st, target_info_t *tgt)
{
	io_req_t	ior;
	int	i;
	struct sg_segment	*sg = st->sg_list;
	unsigned long offset = 0, count;


	st->sg_segment_count = 0;
	st->sg_index = 0;

	if ((st->flags & STATE_CHAINED_IO) == 0) {
		asc_construct_entry(st, (vm_offset_t) st->dma_area, st->buflen, 0);
		return;
	}

	ior = tgt->ior;

	for (ior = tgt->ior; ior; ior = ior->io_link) {
		count = ior->io_count;

		if (ior->io_link)
			count -= ior->io_link->io_count;

		asc_construct_entry(st, (vm_offset_t) ior->io_data, count, offset);
		offset += count;
	}

}

/*
 * asc_get_io
 *
 * Get the next physical address and count to transfer. There are
 * several things happening here, if the request is to 
 * be polled, or if the address and/or count prevent from
 * doing a DMA transfer, then the address is returned as
 * a virtual address, and the count is limited to 16 bytes
 * or less.
 */

static boolean_t
asc_get_io(asc_softc_t asc, State *st, unsigned char **address,
		unsigned long *count, boolean_t has_to_poll)
{
	long offset, xfer;
	struct sg_segment	*sgp;
	boolean_t		can_dma_transfer = TRUE;


	*count = st->buflen;
	offset = st->total_count - *count;

	sgp = &st->sg_list[st->sg_index];

	if (offset >= sgp->sg_offset
	&& offset < (sgp->sg_offset + sgp->sg_count)) 
		goto done;

	if (offset > sgp->sg_offset) {
		sgp++;
		st->sg_index++;

		if (st->sg_index >= st->sg_segment_count)  {
#if ASC_DEBUG
			asc_DumpLog("SCSI DMA error");
#endif
			panic("SCSI DMA error : sg_index > sg_segment_count");
		}

		if (offset >= sgp->sg_offset
		&& offset < (sgp->sg_offset + sgp->sg_count)) 
			goto done;

#if ASC_DEBUG
		asc_DumpLog("SCSI DMA Error");
#endif
		printf("Current offset %d, s/g item offset %d\n",
			offset, sgp->sg_offset);
		panic("SCSI Error : DMA Offset Error");
	} else {
		printf("New offset is less than current offset??! Current %d, sg %d\n",
			offset, sgp->sg_offset);
			
		panic("SCSI Error: DMA Offset error");
	}

done:
	offset -= sgp->sg_offset;
	xfer = sgp->sg_count - offset;

	/* First, make sure count is within 
	 * the segment, then check to see
	 * if the count is within the
	 * max. xfer count for the chip per
	 * DMA transaction.
	 */
	if (*count > xfer)
		*count = xfer;

	if (*count > asc->max_transfer)
		*count = asc->max_transfer;

	if (has_to_poll || st->flags & STATE_FIFO_XFER) {
		*address = sgp->sg_vmaddr + offset;
		can_dma_transfer = FALSE;
	} else {
		*address = (unsigned char *) sgp->sg_physaddr + offset;

		/* Check to make sure the address and count
		 * can be used to do a DMA transfer.
		 */
		
		if ((unsigned long) *address & 0x7 || *count & 0x7) {
			*address = sgp->sg_vmaddr + offset;
			can_dma_transfer = FALSE;

		}
	}

	/* If this is to be a polled request, then
	 * limit the count to 16 (max size of the fifo)
	 */

	if (can_dma_transfer == FALSE && *count > 16)
		*count = 16;

	ASC_TRACE5("Data {Phys %x Addr %x, S/G Count %d Xfer Count %d, can dma? %d}", sgp->sg_physaddr, *address, sgp->sg_count, *count, can_dma_transfer);


	return can_dma_transfer;
}

void
asc_sync_cache_data(target_info_t *tgt, State *state)
{
	io_req_t ior, next_ior;
	long	 count;

	if (powermac_info.io_coherent)
		return;

	if ((state->flags  & STATE_CHAINED_IO) == 0)  {
		if (state->flags & STATE_DMA_IN) {
			invalidate_cache_for_io((vm_offset_t) state->dma_area,
					state->total_count, FALSE);
		} else {
			flush_dcache((vm_offset_t)state->dma_area,
				     state->total_count,
				     FALSE);
		}
		return;
	}

	for (ior = tgt->ior; ior; ior = next_ior) {
		count = ior->io_count;

		if ((next_ior = ior->io_link) != NULL)
			count -= next_ior->io_count;

		if (state->flags & STATE_DMA_IN) {
			/* Make sure we're cacheline aligned, if we're not
			 * then we need to flush first and last lines, not
			 * invalidate them
			 */
			invalidate_cache_for_io((vm_offset_t) ior->io_data,
					count, FALSE);
		} else {
			flush_dcache((vm_offset_t)ior->io_data,
				     count, FALSE);
		}
	}

}


#ifdef ASC_DEBUG
static int asc_trace_sequence = 0;

static void
asc_print_log(struct asc_log *lp)
{
	if (lp->msg) {
		printf("%d %d:%d ", lp->sequence, lp->unit, lp->target);
		printf(lp->msg, lp->args[0], lp->args[1], lp->args[2],
				lp->args[3], lp->args[4]);
		printf("\n");
	}
}

asc_DumpLog(str)
	char *str;
{
	register struct asc_log *lp;


	printf("*** Dumping SCSI Trace Log. Reason: %s\n", str);

	lp = asc_logp;
	do {
		asc_print_log(lp);
		if (++lp >= &asc_log[NLOG])
			lp = asc_log;
	} while (lp != asc_logp);
	printf("**** Please try to submit the SCSI Trace Log\n");
	printf("**** to bugs@mklinux.apple.com. Thank you.\n\n");

}

static void
asc_trace_log(asc_softc_t asc, char *msg, u_long a1, u_long a2,
		u_long a3, u_long a4, u_long a5)
{
	struct asc_log	*lp = asc_logp;


	lp->unit = asc->unit;
	lp->target = asc->target;
	lp->sequence = asc_trace_sequence++;
	lp->msg = msg;
	lp->args[0] = a1;
	lp->args[1] = a2;
	lp->args[2] = a3;
	lp->args[3] = a4;
	lp->args[4] = a5;

	if (asc_trace_debug)
		asc_print_log(lp);

	lp++;
	if (lp >= &asc_log[NLOG])
		lp = asc_log;

	asc_logp = lp;

}

#endif /*ASC_DEBUG*/

#endif	/* NASC > 0 */
