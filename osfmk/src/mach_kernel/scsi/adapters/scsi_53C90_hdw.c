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
 * Revision 2.1.2.1  92/03/28  10:14:42  jeffreyh
 * 	scsi reconnect fix.
 * 	[92/03/26            jerrie]
 * 	Created, from the NCR data sheet
 * 	"NCR 53C90 Enhanced SCSI Processor"
 * 	[91/12/03            jerrie]
 * 
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
 *	File: scsi_53C90_hdw.c
 * 	Author: Jerrie Coffman
 *		Intel Corporation Supercomputer Systems Division
 *	Date:	12/91
 *
 *	Bottom layer of the SCSI driver: chip-dependent functions
 *
 *	This file contains the code that is specific to the NCR 53C90
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

#include <esp.h>
#if	NESP > 0
#include <platforms.h>

#ifdef	iPSC386
/*
 * XXX
 * I shouldn't have to do this
 * gimmeabreak()'s should be Debugger()'s
 */
#define gimmeabreak()		asm("int3")

#define check_memory(a,b)	0

#define MAPPABLE		0

#define	PAD(n)			char n[3]
#define	SHORTPAD(n)		char n[2]
#endif /* iPSC386 */

#include <machine/machparam.h>		/* spl definitions */
#include <mach/std_types.h>
#include <sys/types.h>
#include <vm/vm_kern.h>
#include <chips/busses.h>
#include <scsi/compat_30.h>
#include <machine/machparam.h>

#include <scsi/scsi.h>
#include <scsi/scsi2.h>

#include <scsi/scsi_defs.h>
#include <scsi/adapters/scsi_dma.h>
#include <scsi/adapters/scsi_53C90.h>
#include <i386ipsc/esp_ctlr.h>

extern scsi_dma_ops_t esp_dma_ops;

#ifdef	PAD
typedef struct {
	volatile unsigned char	esp_tc_lsb;	/* rw: Transfer Counter LSB */
	PAD(pad0);
	volatile unsigned char	esp_tc_msb;	/* rw: Transfer Counter MSB */
	PAD(pad1);
	volatile unsigned char	esp_fifo;	/* rw: FIFO top */
	PAD(pad2);
	volatile unsigned char	esp_cmd;	/* rw: Command */
	PAD(pad3);
	volatile unsigned short	esp_csr;	/* r:  Status */
/*#define		esp_dbus_id esp_csr	/* w: Destination Bus ID */
	SHORTPAD(pad4);
	volatile unsigned char	esp_intr;	/* r:  Interrupt */
/*#define		esp_sel_timo esp_intr	/* w: (re)select timeout */
	PAD(pad5);
	volatile unsigned char	esp_ss;		/* r:  Sequence Step */
/*#define		esp_syn_p esp_ss	/* w: synchronous period */
	PAD(pad6);
	volatile unsigned char	esp_flags;	/* r:  FIFO flags */
/*#define		esp_syn_o esp_flags	/* w: synchronous offset */
	PAD(pad7);
	volatile unsigned char	esp_cnfg;	/* rw: Configuration */
	PAD(pad8);
	volatile unsigned char	esp_ccf;	/* w:  Clock Conv. Factor */
	PAD(pad9);
	volatile unsigned char	esp_test;	/* rw: Test Mode */
	PAD(pad10);
} esp_padded_regmap_t;

#else	/* !PAD */

typedef esp_regmap_t	esp_padded_regmap_t;

#endif	/* !PAD */

caddr_t map_phys();

/*
 * A script has a three parts: a pre-condition, an action, and
 * an optional command to the chip.  The first triggers error
 * handling if not satisfied and in our case it is a match
 * of the expected and actual scsi-bus phases.
 * The action part is just a function pointer, and the
 * command is what the 53c90 should be told to do at the end
 * of the action processing.  This command is only issued and the
 * script proceeds if the action routine returns TRUE.
 * See esp_intr() for how and where this is all done.
 */

typedef struct script {
	unsigned char	condition;	/* expected state at interrupt */
	unsigned char	command;	/* command to the chip */
	unsigned short	flags;		/* unused padding */
	boolean_t	(*action)();	/* extra operations */
} *script_t;

/* Matching on the condition value */
#define	ANY				0xff
#define	SCRIPT_MATCH(csr,ir,value)	((SCSI_PHASE(csr)==(value)) || \
					 (((value)==ANY) && \
					  ((ir)&(ESP_INT_DISC|ESP_INT_FC))))

/* When no command is needed */
#define	SCRIPT_END	-1

/* forward decls of script actions */
boolean_t
	esp_end(),			/* all come to an end */
	esp_clean_fifo(),		/* .. in preparation for status byte */
	esp_get_status(),		/* get status from target */
	esp_xfer_in(),			/* get data from target via dma */
	esp_xfer_in_r(),		/* get data from target via dma (restartable)*/
	esp_xfer_out(),			/* send data to target via dma */
	esp_xfer_out_r(),		/* send data to target via dma (restartable) */
	esp_dosynch(),			/* negotiate synch xfer */
	esp_msg_in(),			/* receive the disconenct message */
	esp_disconnected(),		/* target has disconnected */
	esp_reconnect();		/* target reconnected */

/* forward decls of error handlers */
boolean_t
	esp_err_generic(),		/* generic handler */
	esp_err_disconn();		/* target disconnects amidst */

int		esp_reset_scsibus();
boolean_t	esp_probe_target();
static		esp_wait();

/*
 * State descriptor for this layer
 * There is one such structure per (enabled) SCSI-53C90 interface
 */
struct esp_softc {
	watchdog_t	wd;
	esp_padded_regmap_t	*regs;	/* 53C90 registers */

	/* HBA specific registers */
        volatile unsigned long	*scsi_fifo;	/* SCSI controller FIFOs */
        scsi_auxmap_t		*scsi_aux;	/* SCSI controller aux regs */
	int			hf_enable;	/* half-full enable flag */

	scsi_dma_ops_t	*dma_ops;	/* DMA operations and state */
	opaque_t	dma_state;

	script_t	script;		/* what should happen next */
	boolean_t	(*error_handler)();/* what if something is wrong */
	int		in_count;	/* amnt we expect to receive */
	int		out_count;	/* amnt we are going to ship */

	volatile char	state;
#define	ESP_STATE_BUSY		0x01	/* selecting or currently connected */
#define ESP_STATE_TARGET	0x04	/* currently selected as target */
#define ESP_STATE_COLLISION	0x08	/* lost selection attempt */
#define ESP_STATE_DMA_IN	0x10	/* tgt --> initiator xfer */
#define	ESP_STATE_SPEC_DMA	0x20	/* special, 8 byte threshold dma */

	unsigned char	ntargets;	/* how many alive on this scsibus */
	unsigned char	done;
	unsigned char	extra_count;	/* sleazy trick to spare an interrupt */

	scsi_softc_t	*sc;		/* HBA-indep info */
	target_info_t	*active_target;	/* the current one */

	target_info_t	*next_target;	/* trying to seize bus */
	queue_head_t	waiting_targets;/* other targets competing for bus */

	unsigned char	ss_was;		/* districate powered on/off devices */
	unsigned char	cmd_was;

} esp_softc_data[NESP];

typedef struct esp_softc *esp_softc_t;

esp_softc_t	esp_softc[NESP];

/*
 * Synch xfer parameters, and timing conversions
 */
int	esp_min_period = 5;	/* in CLKS/BYTE, 1 CLK = 62.5 nsecs */
int	esp_max_offset = 15;	/* pure number */

#define esp_to_scsi_period(a)	(a * 10)

int scsi_period_to_esp(p)
{
	register int 	ret;
	ret = p / 10;
	if (ret < esp_min_period)
		return esp_min_period;
	if ((ret * 10) < p)
		return ret + 1;
	return ret;
}

/*
 * dma-like copy structures
 */
typedef struct { unsigned char byte[4096]; } B4096;
typedef struct { unsigned char byte[2048]; } B2048;

/*
 * max dma size routine
 */
unsigned int
esp_max_dma(esp)
	register esp_softc_t	esp;
{
	return esp->sc->max_dma_data;
}

/*
 * Definition of the controller for the auto-configuration program
 */

int	esp_probe(), scsi_slave(), scsi_attach(), esp_go(), esp_intr();

caddr_t	esp_std[NESP] = { 0 };
struct	bus_device *esp_dinfo[NESP*8];
struct	bus_ctlr *esp_minfo[NESP];
struct	bus_driver esp_driver = 
        { esp_probe, scsi_slave, scsi_attach, esp_go, esp_std, "rz", esp_dinfo,
	  "esp", esp_minfo, BUS_INTR_B4_PROBE};

esp_set_dmaops(unit, dmaops)
	unsigned int    unit;
	scsi_dma_ops_t  *dmaops;
{
	if (unit < NESP) esp_std[unit] = (caddr_t)dmaops;
}

/*
 * Scripts
 */
struct script

esp_script_data_in[] = {	/* started with SEL & DMA */
	{SCSI_PHASE_DATAI, ESP_CMD_XFER_INFO|ESP_CMD_DMA, 0, esp_xfer_in},
	{SCSI_PHASE_STATUS, ESP_CMD_I_COMPLETE, 0, esp_clean_fifo},
	{SCSI_PHASE_MSG_IN, ESP_CMD_MSG_ACPT, 0, esp_get_status},
	{ANY, SCRIPT_END, 0, esp_end}
},

esp_script_data_out[] = {	/* started with SEL & DMA */
	{SCSI_PHASE_DATAO, ESP_CMD_XFER_INFO|ESP_CMD_DMA, 0, esp_xfer_out},
	{SCSI_PHASE_STATUS, ESP_CMD_I_COMPLETE, 0, esp_clean_fifo},
	{SCSI_PHASE_MSG_IN, ESP_CMD_MSG_ACPT, 0, esp_get_status},
	{ANY, SCRIPT_END, 0, esp_end}
},

esp_script_try_synch[] = {
	{SCSI_PHASE_MSG_OUT, ESP_CMD_I_COMPLETE,0, esp_dosynch},
	{SCSI_PHASE_MSG_IN, ESP_CMD_MSG_ACPT, 0, esp_get_status},
	{ANY, SCRIPT_END, 0, esp_end}
},

esp_script_simple_cmd[] = {
	{SCSI_PHASE_STATUS, ESP_CMD_I_COMPLETE, 0, esp_clean_fifo},
	{SCSI_PHASE_MSG_IN, ESP_CMD_MSG_ACPT, 0, esp_get_status},
	{ANY, SCRIPT_END, 0, esp_end}
},

esp_script_disconnect[] = {
	{ANY, ESP_CMD_ENABLE_SEL, 0, esp_disconnected},
	{SCSI_PHASE_MSG_IN, ESP_CMD_MSG_ACPT, 0, esp_reconnect}
},

esp_script_restart_data_in[] = { /* starts after disconnect */
	{SCSI_PHASE_DATAI, ESP_CMD_XFER_INFO|ESP_CMD_DMA, 0, esp_xfer_in_r},
	{SCSI_PHASE_STATUS, ESP_CMD_I_COMPLETE, 0, esp_clean_fifo},
	{SCSI_PHASE_MSG_IN, ESP_CMD_MSG_ACPT, 0, esp_get_status},
	{ANY, SCRIPT_END, 0, esp_end}
},

esp_script_restart_data_out[] = { /* starts after disconnect */
	{SCSI_PHASE_DATAO, ESP_CMD_XFER_INFO|ESP_CMD_DMA, 0, esp_xfer_out_r},
	{SCSI_PHASE_STATUS, ESP_CMD_I_COMPLETE, 0, esp_clean_fifo},
	{SCSI_PHASE_MSG_IN, ESP_CMD_MSG_ACPT, 0, esp_get_status},
	{ANY, SCRIPT_END, 0, esp_end}
};

#if	documentation
/*
 * This is what might happen during a read
 * that disconnects
 */
esp_script_data_in_wd[] = { /* started with SEL & DMA & allow disconnect */
	{SCSI_PHASE_MSG_IN, ESP_CMD_XFER_INFO|ESP_CMD_DMA, 0, esp_msg_in},
	{ANY, ESP_CMD_ENABLE_SEL, 0, esp_disconnected},
	{SCSI_PHASE_MSG_IN, ESP_CMD_MSG_ACPT, 0, esp_reconnect},
	{SCSI_PHASE_DATAI, ESP_CMD_XFER_INFO|ESP_CMD_DMA, 0, esp_xfer_in},
	{SCSI_PHASE_STATUS, ESP_CMD_I_COMPLETE, 0, esp_clean_fifo},
	{SCSI_PHASE_MSG_IN, ESP_CMD_MSG_ACPT, 0, esp_get_status},
	{ANY, SCRIPT_END, 0, esp_end}
};
#endif

#ifdef	DEBUG

#define	PRINT		if (scsi_debug) printf

esp_state(regs)
	esp_padded_regmap_t	*regs;
{
	register unsigned char ff,ir,d0,d1,cmd;
	register unsigned short csr;

	if (regs == 0) {
		if (esp_softc[0])
			regs = esp_softc[0]->regs;
		else
			regs = (esp_padded_regmap_t *)0xd4000000; /* XXX */
	}
	ff = regs->esp_flags;
	csr = regs->esp_csr;
/*	ir = regs->esp_intr;	nope, clears interrupt */
	d0 = regs->esp_tc_lsb;
	d1 = regs->esp_tc_msb;
	cmd = regs->esp_cmd;
	printf("dma %04x ff %02x csr %04x cmd %02x\n",
		(d1 << 8) | d0, ff, csr, cmd);
	return 0;
}

esp_target_state(tgt)
	target_info_t	*tgt;
{
	if (tgt == 0)
		tgt = esp_softc[0]->active_target;
	if (tgt == 0)
		return 0;
	db_printf("@x%x: fl %x dma %X+%x cmd %x@%X id %x per %x off %x ior %X ret %X\n",
		tgt,
		tgt->flags, tgt->dma_ptr, tgt->transient_state.dma_offset, tgt->cur_cmd,
		tgt->cmd_ptr, tgt->target_id, tgt->sync_period, tgt->sync_offset,
		tgt->ior, tgt->done);
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

esp_all_targets(unit)
{
	int i;
	target_info_t	*tgt;
	for (i = 0; i < 8; i++) {
		tgt = esp_softc[unit]->sc->target[i];
		if (tgt)
			esp_target_state(tgt);
	}
}

esp_script_state(unit)
{
	script_t	spt = esp_softc[unit]->script;

	if (spt == 0) return 0;
	db_printsym(spt,1);
	db_printf(": %x %x ", spt->condition, spt->command);
	db_printsym(spt->action,1);
	db_printf(", ");
	db_printsym(esp_softc[unit]->error_handler, 1);
	return 0;
}

#define TRMAX 200
int tr[TRMAX+3];
int trpt, trpthi;
#define	TR(x)	tr[trpt++] = x
#define TRWRAP	trpthi = trpt; trpt = 0
#define TRCHECK	if (trpt > TRMAX) {TRWRAP;}

#define TRACE
#ifdef TRACE

#define LOGSIZE 256
int esp_logpt = 0;
char esp_log[LOGSIZE];

#define MAXLOG_VALUE	0x1e
struct {
	char *name;
	unsigned int count;
} logtbl[MAXLOG_VALUE];

static LOG(e,f)
	char *f;
{
	esp_log[esp_logpt++] = (e);
	if (esp_logpt == LOGSIZE) esp_logpt = 0;
	if ((e) < MAXLOG_VALUE) {
		logtbl[(e)].name = (f);
		logtbl[(e)].count++;
	}
}

esp_print_log(skip)
	int skip;
{
	register int i, j;
	register unsigned char c;

	for (i = 0, j = esp_logpt; i < LOGSIZE; i++) {
		c = esp_log[j];
		if (++j == LOGSIZE) j = 0;
		if (skip-- > 0)
			continue;
		if (c < MAXLOG_VALUE)
			db_printf(" %s", logtbl[c].name);
		else
			db_printf("-x%x", c & 0x7f);
	}
}

esp_print_stat()
{
	register int i;
	register char *p;
	for (i = 0; i < MAXLOG_VALUE; i++) {
		if (p = logtbl[i].name)
			printf("%d %s\n", logtbl[i].count, p);
	}
}

#else	/* TRACE */
#define	LOG(e,f)
#endif	/* TRACE */

#else	/* DEBUG */
#define PRINT
#define	LOG(e,f)

#define TR(x)
#define TRWRAP
#define TRCHECK

#endif	/* DEBUG */


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
esp_probe(reg, ui)
	unsigned	reg;
	struct bus_ctlr	*ui;
{
	int             unit = ui->unit;
	esp_softc_t     esp = &esp_softc_data[unit];
	int		target_id;
	scsi_softc_t	*sc;
	register esp_padded_regmap_t	*regs;
	int		s;
	boolean_t	did_banner = FALSE;
	unsigned int	reg_save;
	register unsigned char	csr, ir;

	/*
	 * We are only called if the right board is there,
	 * but make sure anyways..
	 */
	if (check_memory(reg, 0))
		return 0;

#if	MAPPABLE
	/* Mappable version side */
	ESP_probe(reg, ui);
#endif

	/*
	 * Initialize hw descriptor
	 */
	esp_softc[unit] = esp;

	/*
	 * Map controller physical addresses to virtual addresses
	 */
	reg_save = reg;
	esp->scsi_fifo = (volatile unsigned long *)map_phys(reg, SCSI_FIFO_LEN);
	reg += SCSI_PHYS_OFF;
	esp->regs = (esp_padded_regmap_t *)map_phys(reg, ESP_REG_LEN);
	reg += SCSI_PHYS_OFF;
        esp->scsi_aux = (scsi_auxmap_t *)map_phys(reg, SCSI_AUX_LEN);
        reg = reg_save;

/*
printf("esp->scsi_fifo: virtual %x physical %x\n",
	esp->scsi_fifo, kvtophys(esp->scsi_fifo));
printf("esp->regs;      virtual %x physical %x\n",
	esp->regs, kvtophys(esp->regs));
printf("esp->scsi_aux:  virtual %x physical %x\n",
	esp->scsi_aux, kvtophys(esp->scsi_aux));

printf("\nesp aux map\n");
printf("\tscsi_clear_cnt    virtual %x physical %x\n",
	&esp->scsi_aux->scsi_clear_cnt,
	kvtophys(&esp->scsi_aux->scsi_clear_cnt));
printf("\tscsi_read_mode    virtual %x physical %x\n",
	&esp->scsi_aux->scsi_read_mode,
	kvtophys(&esp->scsi_aux->scsi_read_mode));
printf("\tscsi_reset_esp    virtual %x physical %x\n",
	&esp->scsi_aux->scsi_reset_esp,
	kvtophys(&esp->scsi_aux->scsi_reset_esp));
printf("\tscsi_write_mode   virtual %x physical %x\n",
	&esp->scsi_aux->scsi_write_mode,
	kvtophys(&esp->scsi_aux->scsi_write_mode));
printf("\tscsi_reset_fifo   virtual %x physical %x\n",
	&esp->scsi_aux->scsi_reset_fifo,
	kvtophys(&esp->scsi_aux->scsi_reset_fifo));
printf("\tscsi_enable_hf    virtual %x physical %x\n",
	&esp->scsi_aux->scsi_enable_hf,
	kvtophys(&esp->scsi_aux->scsi_enable_hf));
printf("\tscsi_disable_hf   virtual %x physical %x\n",
	&esp->scsi_aux->scsi_disable_hf,
	kvtophys(&esp->scsi_aux->scsi_disable_hf));

printf("\nesp register map\n");
printf("\ttc_lsb        virtual %x physical %x\n",
	&esp->regs->esp_tc_lsb, kvtophys(&esp->regs->esp_tc_lsb));
printf("\ttc_msb        virtual %x physical %x\n",
	&esp->regs->esp_tc_msb, kvtophys(&esp->regs->esp_tc_msb));
printf("\tfifo          virtual %x physical %x\n",
	&esp->regs->esp_fifo, kvtophys(&esp->regs->esp_fifo));
printf("\tcmd           virtual %x physical %x\n",
	&esp->regs->esp_cmd, kvtophys(&esp->regs->esp_cmd));
printf("\tcsr           virtual %x physical %x\n",
	&esp->regs->esp_csr, kvtophys(&esp->regs->esp_csr));
printf("\tdbus_id       virtual %x physical %x\n",
	&esp->regs->esp_dbus_id, kvtophys(&esp->regs->esp_dbus_id));
printf("\tintr          virtual %x physical %x\n",
	&esp->regs->esp_intr, kvtophys(&esp->regs->esp_intr));
printf("\tsel_timo      virtual %x physical %x\n",
	&esp->regs->esp_sel_timo, kvtophys(&esp->regs->esp_sel_timo));
printf("\tss            virtual %x physical %x\n",
	&esp->regs->esp_ss, kvtophys(&esp->regs->esp_ss));
printf("\tsyn_p         virtual %x physical %x\n",
	&esp->regs->esp_syn_p, kvtophys(&esp->regs->esp_syn_p));
printf("\tflags         virtual %x physical %x\n",
	&esp->regs->esp_flags, kvtophys(&esp->regs->esp_flags));
printf("\tsyn_o         virtual %x physical %x\n",
	&esp->regs->esp_syn_o, kvtophys(&esp->regs->esp_syn_o));
printf("\tcnfg          virtual %x physical %x\n",
	&esp->regs->esp_cnfg, kvtophys(&esp->regs->esp_cnfg));
printf("\tccf           virtual %x physical %x\n",
	&esp->regs->esp_ccf, kvtophys(&esp->regs->esp_ccf));
printf("\ttest          virtual %x physical %x\n",
	&esp->regs->esp_test, kvtophys(&esp->regs->esp_test));
*/

/*
 * XXX Should be called from somewhere else before the esp_probe routine
 */
esp_set_dmaops(unit, &esp_dma_ops);

	if ((esp->dma_ops = (scsi_dma_ops_t *)esp_std[unit]) == 0)
		/* use same as unit 0 if undefined */
		esp->dma_ops = (scsi_dma_ops_t *)esp_std[0];
	esp->dma_state = (*esp->dma_ops->init)(unit, reg);

	queue_init(&esp->waiting_targets);

	sc = scsi_master_alloc(unit, esp);
	esp->sc = sc;

	sc->go = esp_go;
	sc->watchdog = scsi_watchdog;
	sc->probe = esp_probe_target;
	esp->wd.reset = esp_reset_scsibus;

#ifdef	MACH_KERNEL
	sc->max_dma_data = ESP_TC_MAX;
#else
	sc->max_dma_data = scsi_per_target_virtual;
#endif

	regs = esp->regs;

	/*
	 * Test for SCSI controller presence by fiddling with ESP FIFO
	 */
	s = splbio();

	reset_esp(esp);
	delay(25);
	reset_fifo(esp);
	clear_count(esp);

	regs->esp_fifo = 0x55;
	regs->esp_fifo = 0xAA;
	delay(2);

	if ((regs->esp_flags != 2)    ||
	    (regs->esp_fifo  != 0x55) ||
	    (regs->esp_fifo  != 0xAA)) {
		splx(s);
		return 0;
	}

	/*
	 * Reset chip and bus, fully.
	 */
	esp_reset(esp, FALSE);

	csr = esp_wait(regs, SCSI_INT);
	ir = regs->esp_intr;
	delay(2);

	/*
	 * Our SCSI id on the bus.
	 * If this changes it is easy to fix: make a default that
	 * can be changed as boot arg.
	 */
	regs->esp_cnfg = (regs->esp_cnfg & ~ESP_CNFG_MY_BUS_ID) |
			      (scsi_initiator_id[unit] & 0x7);

	sc->initiator_id = regs->esp_cnfg & ESP_CNFG_MY_BUS_ID;
	printf("%s%d: SCSI id %d", ui->name, unit, sc->initiator_id);

	/*
	 * For all possible targets, see if there is one and allocate
	 * a descriptor for it if it is there.
	 */
	for (target_id = 0; target_id < 8; target_id++) {
		register unsigned char		ss, ff;
		register scsi_status_byte_t	status;

		/* except of course ourselves */
		if (target_id == sc->initiator_id)
			continue;

		regs->esp_cmd = ESP_CMD_FLUSH;	/* empty fifo */
		delay(2);

		regs->esp_dbus_id = target_id;
		delay(2);
		regs->esp_sel_timo = ESP_TIMEOUT_250;
		delay(2);

		/*
		 * See if the unit is ready.
		 * XXX SHOULD inquiry LUN 0 instead !!!
		 */
		regs->esp_fifo = SCSI_CMD_TEST_UNIT_READY;
		regs->esp_fifo = 0;
		regs->esp_fifo = 0;
		regs->esp_fifo = 0;
		regs->esp_fifo = 0;
		regs->esp_fifo = 0;
		delay(2);

		/* select and send it */
		regs->esp_cmd = ESP_CMD_SEL;

		/* wait for the chip to complete, or timeout */
		csr = esp_wait(regs, SCSI_INT);
		ss = regs->esp_ss;
		ir = regs->esp_intr;

		/* empty fifo, there is garbage in it if timeout */
		regs->esp_cmd = ESP_CMD_FLUSH;
		delay(2);

		/*
		 * Check if the select timed out
		 */
		if ((ESP_SS(ss) == 0) && (ir == ESP_INT_DISC))
			/* no one out there */
			continue;

		if (SCSI_PHASE(csr) != SCSI_PHASE_STATUS) {
			printf("%signoring target at %d",
				did_banner ? ", " : " ", target_id);
			continue;
		}

		printf("%s%d", did_banner++ ? ", " : " target(s) at ",
			target_id);

		regs->esp_cmd = ESP_CMD_I_COMPLETE;
		csr = esp_wait(regs, SCSI_INT);
		ir = regs->esp_intr; /* ack intr */

		status.bits = regs->esp_fifo; /* empty fifo */
		ff = regs->esp_fifo;

		if (status.st.scsi_status_code != SCSI_ST_GOOD)
			scsi_error( 0, SCSI_ERR_STATUS, status.bits, 0);

		regs->esp_cmd = ESP_CMD_MSG_ACPT;
		csr = esp_wait(regs, SCSI_INT);
		ir = regs->esp_intr; /* ack intr */

		/*
		 * Found a target
		 */
		esp->ntargets++;
		{
			register target_info_t	*tgt;
			tgt = scsi_slave_alloc(sc->masterno, target_id, esp);

			(*esp->dma_ops->new_target)(esp->dma_state, tgt);
		}
	}
	printf(".\n");

	splx(s);
	return 1;
}

boolean_t
esp_probe_target(tgt, ior)
	target_info_t		*tgt;
	io_req_t		ior;
{
	esp_softc_t     esp = esp_softc[tgt->masterno];
	boolean_t	newlywed;

	newlywed = (tgt->cmd_ptr == 0);
	if (newlywed) {
		(*esp->dma_ops->new_target)(esp->dma_state, tgt);
	}

	if (scsi_inquiry( tgt, SCSI_INQ_STD_DATA) == SCSI_RET_DEVICE_DOWN)
		return FALSE;

	tgt->flags = TGT_ALIVE;
	return TRUE;
}

static esp_wait(regs, until)
	esp_padded_regmap_t	*regs;
{
	int timeo = 1000000;

	while ((regs->esp_csr & until) != 0) {	/* active low */
		delay(1);
		if (!timeo--) {
			printf("\nesp_wait timer expired: csr 0x%04x\n",
				regs->esp_csr);
			break;
		}
	}

	return regs->esp_csr;
}

esp_reset(esp, quickly)
	esp_softc_t		esp;
	boolean_t		quickly;
{
	register esp_padded_regmap_t	*regs;
	register char		my_id;

	reset_controller(esp);
	
	regs = esp->regs;

	/* preserve our ID for now */
	my_id = (regs->esp_cnfg & ESP_CNFG_MY_BUS_ID);

	/*
	 * Reset chip and wait till done
	 */
	regs->esp_cmd = ESP_CMD_RESET;
	delay(25);

	/* spec says this is needed after reset */
	regs->esp_cmd = ESP_CMD_NOP;
	delay(25);

	/*
	 * Set up various chip parameters
	 */
	regs->esp_ccf = ESP_CCF_25MHz;
	delay(25);
	regs->esp_sel_timo = ESP_TIMEOUT_250;
	delay(2);
	/* restore our ID */
	regs->esp_cnfg = ESP_CNFG_SLOW | ESP_CNFG_P_CHECK | my_id;
	delay(2);
	/* zero anything else */
	ESP_TC_PUT(regs, 0);
	delay(2);
	regs->esp_syn_p = esp_min_period;
	delay(2);
	regs->esp_syn_o = 0;	/* asynch for now */
	delay(2);

	if (quickly) return;

	/*
	 * reset the scsi bus, the interrupt routine does the rest
	 * or you can call esp_bus_reset().
	 */
	regs->esp_cmd = ESP_CMD_BUS_RESET;
	delay(2);
}

reset_controller(esp)
	register esp_softc_t	esp;
{
	reset_esp(esp);
	delay(25);
	disable_hf(esp);
	delay(25);
	reset_fifo(esp);
	delay(25);
	clear_count(esp);
	delay(25);
}

reset_esp(esp)
	register esp_softc_t	esp;
{
	return esp->scsi_aux->scsi_reset_esp;
}

reset_fifo(esp)
	register esp_softc_t	esp;
{
	return esp->scsi_aux->scsi_reset_fifo;
}

clear_count(esp)
	register esp_softc_t	esp;
{
	return esp->scsi_aux->scsi_clear_cnt;
}

enable_hf(esp)
	register esp_softc_t	esp;
{
	esp->scsi_aux->scsi_enable_hf = 0;
	esp->hf_enable = TRUE;
}

disable_hf(esp)
	register esp_softc_t	esp;
{
	esp->scsi_aux->scsi_disable_hf = 0;
	esp->hf_enable = FALSE;
}

set_fifo_read(esp)
	register esp_softc_t	esp;
{
	register scsi_auxmap_t	*scsi_aux = esp->scsi_aux;

	scsi_aux->scsi_read_mode = scsi_aux->scsi_clear_cnt;
}

set_fifo_write(esp)
	register esp_softc_t	esp;
{
	register scsi_auxmap_t	*scsi_aux = esp->scsi_aux;

	scsi_aux->scsi_write_mode = scsi_aux->scsi_clear_cnt;
}

int
copy_in(esp, to, len)
	register esp_softc_t	esp;
	register unsigned char	*to;
	register int 		len;	/* in bytes */
{
	register volatile unsigned long *from;
	int copied;
	
	/*
	 * This routine copies the data in from the SCSI FIFOs
	 * It returns the number of bytes it transferred
	 */

	copied = len;

	from = esp->scsi_fifo;
	
	if (len >= sizeof(B4096)) {
		*(B4096 *)to = *(B4096 *)from;
		to += sizeof(B4096);
		len -= sizeof(B4096);
	}

	if (len >= sizeof(B2048)) {
		*(B2048 *)to = *(B2048 *)from;
		to += sizeof(B2048);
		len -= sizeof(B2048);
	}

	while (len >= sizeof(unsigned long)) {
		*(unsigned long *)to = *from;
		to += sizeof(unsigned long);
		len -= sizeof(unsigned long);
	}
	
	if (len > 0) {
		unsigned long dword;

		for (dword = *from; len > 0; dword >>= 8) {
			*to = (unsigned char)dword;
			to++;
			len--;
		}
	}

	return copied;
}

int
copy_out(esp, from, len)
	register esp_softc_t	esp;
	register unsigned char	*from;
	register int		len;	/* in bytes */
{
	register volatile unsigned long *to;
	int copied;

	/*
	 * This routine copies the data out to the SCSI FIFOs
	 * It returns the number of bytes it transferred
	 *
	 * Note:
	 *   If len is not a multiple of sizeof(unsigned long)
	 *   more than len bytes may be written to the FIFO
	 *   The maximum return value, however, is len
	 */

	if ((esp->regs->esp_csr & SCSI_WEF) == 0) {
		if (len > sizeof(B4096)) len = sizeof(B4096);
	}
	else if (len > sizeof(B2048)) len = sizeof(B2048);

	copied = len;
	
	to = esp->scsi_fifo;
	
	if (len >= sizeof(B4096)) {
		*(B4096 *)to = *(B4096 *)from;
		from += sizeof(B4096);
		len -= sizeof(B4096);
	} else {
		if (len >= sizeof(B2048)) {
			*(B2048 *)to = *(B2048 *)from;
			from += sizeof(B2048);
			len -= sizeof(B2048);
		}

		while (len >= sizeof(unsigned long)) {
			*to = *(unsigned long *)from;
			from += sizeof(unsigned long);
			len -= sizeof(unsigned long);
		}

		if (len > 0) {
			unsigned long dword;
			unsigned char *byte_p;
			
			dword = 0;
			byte_p = (unsigned char *)&dword;

			while (len > 0) {
				*byte_p++ = *from++;
				len -= sizeof(unsigned char);
			}

			*to = dword;
		}
	}

	return copied;
}

int
copy_out_zero(esp, len)
	register esp_softc_t	esp;
	register int		len;	/* in bytes */
{
	register volatile unsigned long *to;
	int copied;

	/*
	 * This routine copies zeros out to the SCSI FIFOs
	 * It returns the number of bytes it transferred
	 *
	 * Note:
	 *   If len is not a multiple of sizeof(unsigned long)
	 *   more than len bytes may be written to the FIFO
	 *   The maximum return value, however, is len
	 */

	if ((esp->regs->esp_csr & SCSI_WEF) == 0) {
		if (len > sizeof(B4096)) len = sizeof(B4096);
	}
	else if (len > sizeof(B2048)) len = sizeof(B2048);
	
	copied = len;

	to = esp->scsi_fifo;
	
	while (len > 0) {
		*to = 0;
		len -= sizeof(unsigned long);
	}

	return copied;
}

/*
 * map_phys: map physical SCSI addresses into kernel vm and return the
 * (virtual) address.
 */
caddr_t
map_phys(physaddr, length)
caddr_t physaddr;			/* address to map */
int length;				/* num bytes to map */
{
	vm_offset_t vmaddr;
	vm_offset_t pmap_addr;
	vm_offset_t pmap_map_bd();

	if (physaddr != (caddr_t)trunc_page(physaddr))
		panic("map_phys: Tryed to map address not on page boundary");
	if (kmem_alloc_pageable(kernel_map, &vmaddr, round_page(length))
						!= KERN_SUCCESS)
		panic("map_phys: Can't allocate VM");
	pmap_addr = pmap_map_bd(vmaddr, (vm_offset_t)physaddr, 
			(vm_offset_t)physaddr+length, 
			VM_PROT_READ | VM_PROT_WRITE);
	return((caddr_t) vmaddr);
}

/*
 *	Operational functions
 */

/*
 * Start a SCSI command on a target
 */
esp_go( tgt, cmd_count, in_count, cmd_only)
		target_info_t		*tgt;
	boolean_t		cmd_only;
{
	esp_softc_t	esp;
	register int	s;
	boolean_t	disconn;
	script_t	scp;
	boolean_t	(*handler)();

	LOG(1,"go");

	esp = (esp_softc_t)tgt->hw_state;

	tgt->transient_state.cmd_count = cmd_count;

	(*esp->dma_ops->map)(esp->dma_state, tgt);

	disconn  = BGET(scsi_might_disconnect,esp->sc->masterno,tgt->target_id);
	disconn  = disconn && (esp->ntargets > 1);
	disconn |= BGET(scsi_should_disconnect,esp->sc->masterno,tgt->target_id);

	/*
	 * Setup target state
	 */
	tgt->done = SCSI_RET_IN_PROGRESS;

	handler = (disconn) ? esp_err_disconn : esp_err_generic;

	switch (tgt->cur_cmd) {
	    case SCSI_CMD_READ:
	    case SCSI_CMD_LONG_READ:
		LOG(2,"readop");
		scp = esp_script_data_in;
		break;
	    case SCSI_CMD_WRITE:
	    case SCSI_CMD_LONG_WRITE:
		LOG(0x1a,"writeop");
		scp = esp_script_data_out;
		break;
	    case SCSI_CMD_INQUIRY:
		/* This is likely the first thing out:
		   do the synch neg if so */
		if (!cmd_only && ((tgt->flags&TGT_DID_SYNCH)==0)) {
			scp = esp_script_try_synch;
			tgt->flags |= TGT_TRY_SYNCH;
			break;
		}
	    case SCSI_CMD_REQUEST_SENSE:
	    case SCSI_CMD_MODE_SENSE:
	    case SCSI_CMD_RECEIVE_DIAG_RESULTS:
	    case SCSI_CMD_READ_CAPACITY:
	    case SCSI_CMD_READ_BLOCK_LIMITS:
		scp = esp_script_data_in;
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    case SCSI_CMD_MODE_SELECT:
	    case SCSI_CMD_REASSIGN_BLOCKS:
	    case SCSI_CMD_FORMAT_UNIT:
		tgt->transient_state.cmd_count = sizeof(scsi_command_group_0);
		tgt->transient_state.out_count = cmd_count - sizeof(scsi_command_group_0);
		scp = esp_script_data_out;
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    case SCSI_CMD_TEST_UNIT_READY:
		/*
		 * Do the synch negotiation here, unless prohibited
		 * or done already
		 */
		if (tgt->flags & TGT_DID_SYNCH) {
			scp = esp_script_simple_cmd;
		} else {
			scp = esp_script_try_synch;
			tgt->flags |= TGT_TRY_SYNCH;
			cmd_only = FALSE;
		}
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    default:
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		scp = esp_script_simple_cmd;
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
	 * this SCSI bus, e.g. lock the esp structure.
	 * Note that it is the strategy routine's job
	 * to serialize ops on the same target as appropriate.
	 * XXX here and everywhere, locks!
	 */
	/*
	 * Protection viz reconnections makes it tricky.
	 */
	s = splbio();

	if (esp->wd.nactive++ == 0)
		esp->wd.watchdog_state = SCSI_WD_ACTIVE;

	if (esp->state & ESP_STATE_BUSY) {
		/*
		 * Queue up this target, note that this takes care
		 * of proper FIFO scheduling of the scsi-bus.
		 */
		LOG(3,"enqueue");
		enqueue_tail(&esp->waiting_targets, (queue_entry_t) tgt);
	} else {
		/*
		 * It is down to at most two contenders now,
		 * we will treat reconnections same as selections
		 * and let the scsi-bus arbitration process decide.
		 */
		esp->state |= ESP_STATE_BUSY;
		esp->next_target = tgt;
		esp_attempt_selection(esp);
		/*
		 * Note that we might still lose arbitration..
		 */
	}
	splx(s);
}

esp_attempt_selection(esp)
	esp_softc_t	esp;
{
	target_info_t		*tgt;
	esp_padded_regmap_t	*regs;
	register int		out_count;

	regs = esp->regs;
	tgt = esp->next_target;

	LOG(4,"select");
	LOG(0x80+tgt->target_id,0);

	/*
	 * We own the bus now.. unless we lose arbitration
	 */
	esp->active_target = tgt;

	/* Try to avoid reselect collisions */
	if ((regs->esp_csr & (SCSI_INT|SCSI_PHASE_MSG_IN)) ==
	     SCSI_PHASE_MSG_IN)
		return;

	/*
	 * Init bus state variables
	 */
	esp->script = tgt->transient_state.script;
	esp->error_handler = tgt->transient_state.handler;
	esp->done = SCSI_RET_IN_PROGRESS;

	esp->out_count = 0;
	esp->in_count = 0;
	esp->extra_count = 0;

	/*
	 * Start the chip going
	 */
	out_count = (*esp->dma_ops->start_cmd)(esp->dma_state, tgt);
	if (tgt->transient_state.identify != 0xff) {
		regs->esp_fifo = tgt->transient_state.identify;
		delay(2);
	}
	ESP_TC_PUT(regs, out_count);
	delay(2);

	regs->esp_dbus_id = tgt->target_id;
	delay(2);

	regs->esp_sel_timo = ESP_TIMEOUT_250;
	delay(2);

	regs->esp_syn_p = tgt->sync_period;
	delay(2);

	regs->esp_syn_o = tgt->sync_offset;
	delay(2);

	/* ugly little help for compiler */
#define	command	out_count
	if (tgt->flags & TGT_DID_SYNCH) {
		command = (tgt->transient_state.identify == 0xff) ?
				ESP_CMD_SEL | ESP_CMD_DMA :
				ESP_CMD_SEL_ATN | ESP_CMD_DMA; /* preferred */
	} else if (tgt->flags & TGT_TRY_SYNCH)
		command = ESP_CMD_SEL_ATN_STOP;
	else
		command = ESP_CMD_SEL | ESP_CMD_DMA;

	/* Try to avoid reselect collisions */
	if ((regs->esp_csr & (SCSI_INT|SCSI_PHASE_MSG_IN)) !=
	     SCSI_PHASE_MSG_IN) {
		register int	tmp;

		regs->esp_cmd = command;
		delay(2);
		/*
		 * Very nasty but infrequent problem here.  We got/will get
		 * reconnected but the chip did not interrupt.  The watchdog would
		 * fix it allright, but it stops the machine before it expires!
		 * Too bad we cannot just look at the interrupt register, sigh.
		 */
		tmp = regs->esp_cmd;
		if ((tmp != command) && (tmp == (ESP_CMD_NOP|ESP_CMD_DMA))) {
		    if ((regs->esp_csr & SCSI_INT) != 0) {
			delay(250); /* increase if in trouble */

			if (SCSI_PHASE(regs->esp_csr) == SCSI_PHASE_MSG_IN) {
			    /* ok, take the risk of reading the ir */
			    tmp = regs->esp_intr;
			    if (tmp & ESP_INT_RESEL) {
				(void) esp_reconnect(esp, regs->esp_csr, tmp);
				esp_wait(regs, SCSI_INT);
				tmp = regs->esp_intr;
				regs->esp_cmd = ESP_CMD_MSG_ACPT;
				delay(2);
			    } else /* does not happen, but who knows.. */
				esp_reset(esp, FALSE);
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
esp_intr(unit, spllevel)
	int	unit;
	int	spllevel;
{
	register esp_softc_t	esp;
	register script_t	scp;
	register int		ir, csr;
	register esp_padded_regmap_t	*regs;
#if	MAPPABLE
	extern boolean_t	rz_use_mapped_interface;

	if (rz_use_mapped_interface)
		return ESP_intr(unit,spllevel);
#endif

	LOG(5,"\n\tintr");

	esp = esp_softc[unit];

	/* collect ephemeral information */
	regs = esp->regs;
	csr  = regs->esp_csr;
	esp->ss_was = regs->esp_ss;
	esp->cmd_was = regs->esp_cmd;

	/* drop spurious interrupts */
	if (!esp->hf_enable && (csr & SCSI_INT) != 0) {
		return;
	}

	/*
	 * Turn off FIFO half interrupt
	 */
	disable_hf(esp);

	ir = regs->esp_intr;	/* this re-latches CSR (and SSTEP) */

TR(csr);TR(ir);TR(regs->esp_cmd);TRCHECK

	/* this must be done within 250msec of disconnect */
	if (ir & ESP_INT_DISC)
		regs->esp_cmd = ESP_CMD_ENABLE_SEL;

	if (ir & ESP_INT_RESET)
		return esp_bus_reset(esp);

	/* we got an interrupt allright */
	if (esp->active_target)
		esp->wd.watchdog_state = SCSI_WD_ACTIVE;

#ifndef iPSC386
	splx(spllevel);	/* drop priority */
#endif

	if ((esp->state & ESP_STATE_TARGET) ||
	    (ir & (ESP_INT_SEL|ESP_INT_SEL_ATN)))
		return esp_target_intr(esp);

	/*
	 * In attempt_selection() we could not check the esp_intr
	 * register to see if a reselection was in progress [else
	 * we would cancel the interrupt, and it would not be safe
	 * anyways].  So we gave the select command even if sometimes
	 * the chip might have been reconnected already.  What
	 * happens then is that we get an illegal command interrupt,
	 * which is why the second clause.  Sorry, I'd do it better
	 * if I knew of a better way.
	 */
	if ((ir & ESP_INT_RESEL) ||
	    ((ir & ESP_INT_ILL) && (regs->esp_cmd & ESP_CMD_SEL_ATN))) {
		return esp_reconnect(esp, csr, ir);
	}

	/*
	 * Check for various errors
	 */
	if ((csr & (ESP_CSR_GE|ESP_CSR_PE)) || (ir & ESP_INT_ILL)) {
		char	*msg;
printf("{E%x,%x}", csr, ir);
		if (csr & ESP_CSR_GE) {
			panic("ESP: Gross Error\n");
			return;/* sit and prey? */
		}

		if (csr & ESP_CSR_PE)
			msg = "SCSI bus parity error";
		if (ir & ESP_INT_ILL)
			msg = "SCSI Chip Illegal Command";
		/* all we can do is to throw a reset on the bus */
		printf( "esp%d: %s%s", esp - esp_softc_data, msg,
			", attempting recovery.\n");
		esp_reset(esp, FALSE);
		return;
	}

	if ((scp = esp->script) == 0) {	/* sanity */
		panic("Null SCSI script!");
		return;
	}

	LOG(6,"match");
	if (SCRIPT_MATCH(csr,ir,scp->condition)) {
		/*
		 * Perform the appropriate operation,
		 * then proceed
		 */
		if ((*scp->action)(esp, csr, ir)) {
			esp->script = scp + 1;
			regs->esp_cmd = scp->command;
		}
	} else
		return (*esp->error_handler)(esp, csr, ir);
}

esp_target_intr()
{
	panic("ESP: TARGET MODE !!!\n");
}

/*
 * All the many little things that the interrupt
 * routine might switch to
 */
boolean_t
esp_clean_fifo(esp, csr, ir)
	register esp_softc_t	esp;

{
	register esp_padded_regmap_t	*regs = esp->regs;
	register char		ff;

	LOG(7,"clean_fifo");

	while (regs->esp_flags & ESP_FLAGS_FIFO_CNT) {
		delay(2);
		ff = regs->esp_fifo;
		delay(2);
	}

	return TRUE;
}

boolean_t
esp_end(esp, csr, ir)
	register esp_softc_t	esp;
{
	register target_info_t	*tgt;
	register io_req_t	ior;

	LOG(8,"end");

	tgt = esp->active_target;
	if ((tgt->done = esp->done) == SCSI_RET_IN_PROGRESS)
		tgt->done = SCSI_RET_SUCCESS;

	esp->script = 0;

	if (esp->wd.nactive-- == 1)
		esp->wd.watchdog_state = SCSI_WD_INACTIVE;

	esp_release_bus(esp);

	if (ior = tgt->ior) {
		(*esp->dma_ops->end_cmd)(esp->dma_state, tgt, ior);
		LOG(0xA,"ops->restart");
		(*tgt->dev_ops->restart)( tgt, TRUE);
	}

	return FALSE;
}

boolean_t
esp_release_bus(esp)
	register esp_softc_t	esp;
{
	boolean_t	ret = TRUE;

	LOG(9,"release");

	if (esp->state & ESP_STATE_COLLISION) {

		LOG(0xB,"collided");
		esp->state &= ~ESP_STATE_COLLISION;
		esp_attempt_selection(esp);

	} else if (queue_empty(&esp->waiting_targets)) {

		esp->state &= ~ESP_STATE_BUSY;
		esp->active_target = 0;
		esp->script = 0;
		ret = FALSE;

	} else {

		LOG(0xC,"dequeue");
		esp->next_target = (target_info_t *)
				dequeue_head(&esp->waiting_targets);
		esp_attempt_selection(esp);
	}

	return ret;
}

boolean_t
esp_get_status(esp, csr, ir)
	register esp_softc_t	esp;
{
	register esp_padded_regmap_t	*regs = esp->regs;
	register scsi2_status_byte_t status;
	int			len;
	boolean_t		ret;
	io_req_t		ior;
	register target_info_t	*tgt = esp->active_target;

	LOG(0xD,"get_status");
TRWRAP;

	esp->state &= ~ESP_STATE_DMA_IN;

	/*
	 * Get the last two bytes in FIFO
	 */
	while ((regs->esp_flags & ESP_FLAGS_FIFO_CNT) > 2) {
		delay(2);
		status.bits = regs->esp_fifo;
		delay(2);
	}

	status.bits = regs->esp_fifo;

	if (status.st.scsi_status_code != SCSI_ST_GOOD) {
		scsi_error(esp->active_target, SCSI_ERR_STATUS, status.bits, 0);
		esp->done = (status.st.scsi_status_code == SCSI_ST_BUSY) ?
			SCSI_RET_RETRY : SCSI_RET_NEED_SENSE;
	} else
		esp->done = SCSI_RET_SUCCESS;

	status.bits = regs->esp_fifo;	/* just pop the command_complete */

	/* if reading, move the last piece of data in main memory */
	if (len = esp->in_count) {
		register int	count;

		ESP_TC_GET(regs, count);
		delay(2);
		if (count) {
			/* 
			 * this is incorrect and besides..
			 * tgt->ior->io_residual = count;
			 */

			len -= count;
		}
		regs->esp_cmd = esp->script->command;
		
		ret = FALSE;
	} else
		ret = TRUE;

	ESP_TC_PUT(regs, 0);	/* stop dma engine */

	(*esp->dma_ops->end_xfer)(esp->dma_state, tgt, len);

	if (!ret) esp->script++;

	return ret;
}

boolean_t
esp_xfer_in(esp, csr, ir)
	register esp_softc_t	esp;
{
	register target_info_t	*tgt;
	register esp_padded_regmap_t	*regs = esp->regs;
	register int		count;
	unsigned char		ff;
	unsigned char		ss;

	LOG(0xE,"xfer_in");
	tgt = esp->active_target;

	ff = regs->esp_flags;
	delay(2);
	ss = regs->esp_ss;
	delay(2);

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
	 * the fact that the extra byte that shows up is not always
	 * zero, and appears to be pretty random.
	 * Note that esp_flags say there is one byte in the FIFO
	 * even in the ok case, but the sstep value is the right one.
	 * Note finally that this might all be a sync/async issue:
	 * I have only checked the ok case on synch disks so far.
	 *
	 * Indeed it seems to be an asynch issue: exabytes do it too.
	 */
	if ((tgt->sync_offset == 0) && (ESP_SS(ss) != 0x04)) {
		regs->esp_cmd = ESP_CMD_NOP;
		delay(2);
		PRINT("[tgt %d: %x while %d]", tgt->target_id, ff, tgt->cur_cmd);
		while ((ff & ESP_FLAGS_FIFO_CNT) != 0) {
			delay(2);
			ff = regs->esp_fifo;
			delay(2);
			ff = regs->esp_flags;
		}
	}

	esp->state |= ESP_STATE_DMA_IN;

       	count = (*esp->dma_ops->start_datain)(esp->dma_state, tgt);
	ESP_TC_PUT(regs, count);
	delay(2);

	esp->in_count = count;

	if (count <= sizeof(B4096) && count == tgt->transient_state.in_count)
		return TRUE;

	/*
	 * Turn on FIFO half interrupt
	 */
	enable_hf(esp);
	
	regs->esp_cmd = esp->script->command;
	esp->script = esp_script_restart_data_in;

	return FALSE;
}

esp_xfer_in_r(esp, csr, ir)
	register esp_softc_t	esp;
{
	register target_info_t	*tgt;
	register esp_padded_regmap_t	*regs = esp->regs;
	register int		count;
	boolean_t		advance_script = TRUE;


	LOG(0xE,"xfer_in_r");
	tgt = esp->active_target;

	esp->state |= ESP_STATE_DMA_IN;

	if (esp->in_count == 0) {
		/*
		 * Got nothing yet, we just reconnected.
		 */

		/*
		 * Rather than using the messy RFB bit in cnfg2
		 * (which only works for synch xfer anyways)
		 * we just bump up the dma offset.  We might
		 * endup with one more interrupt at the end,
		 * so what.
		 * This is done in esp_err_disconn(), this
		 * way dma (of msg bytes too) is always aligned
		 */

		count = (*esp->dma_ops->restart_datain_1)(esp->dma_state, tgt);
		ESP_TC_PUT(regs, count);
		delay(2);

		esp->in_count = count;

		if (count <= sizeof(B4096) &&
			count == tgt->transient_state.in_count) return TRUE;

		/*
		 * Turn on FIFO half interrupt
		 */
		enable_hf(esp);

		regs->esp_cmd = esp->script->command;

		return FALSE;
	}

	/*
	 * We received some data.
	 */

	count = (*esp->dma_ops->restart_datain_2)
			(esp->dma_state, tgt, sizeof(B2048));

	esp->in_count -= count;

	/* last chunk? */
	if (esp->in_count <= sizeof(B4096)) esp->script++;
	else enable_hf(esp);

	return FALSE;
}


/* send data to target.  Only called to start the xfer */

boolean_t
esp_xfer_out(esp, csr, ir)
	register esp_softc_t	esp;
{
	register esp_padded_regmap_t	*regs = esp->regs;
	register int		count;
	register target_info_t	*tgt;
	int			command;

	LOG(0xF,"xfer_out");
	tgt = esp->active_target;

	esp->state &= ~ESP_STATE_DMA_IN;

	command = esp->script->command;

	(*esp->dma_ops->start_dataout)
		(esp->dma_state, tgt, &regs->esp_cmd, command);
       	count = tgt->transient_state.out_count;
	ESP_TC_PUT(regs, count);
	delay(2);

	esp->out_count = count;

	if (count <= sizeof(B4096)) return TRUE;

	/*
	 * Turn on FIFO half interrupt
	 */
	enable_hf(esp);
	
	regs->esp_cmd = command;
	esp->script = esp_script_restart_data_out;

	return FALSE;
}

/* send data to target. Called in two different ways:
   (a) to restart a big transfer and 
   (b) after reconnection
 */
boolean_t
esp_xfer_out_r(esp, csr, ir)
	register esp_softc_t	esp;
{
	register esp_padded_regmap_t	*regs = esp->regs;
	register target_info_t	*tgt;
	int			count;

	LOG(0xF,"xfer_out_r");

	tgt = esp->active_target;
	esp->state &= ~ESP_STATE_DMA_IN;

	if (esp->out_count == 0) {

		/*
		 * Nothing committed: we just got reconnected
		 */
        
		count = (*esp->dma_ops->restart_dataout_1)
				(esp->dma_state, tgt);

		esp->extra_count = (*esp->dma_ops->restart_dataout_3)
					(esp->dma_state, tgt, &regs->esp_fifo);

		ESP_TC_PUT(regs, count);
		delay(2);

		esp->out_count = count;

		/* is this the last chunk ? */
		if (count <= sizeof(B4096)) return TRUE;

		/*
		 * Turn on FIFO half interrupt
		 */
		enable_hf(esp);
	
		regs->esp_cmd = esp->script->command;

		return FALSE;
	}

	/*
	 * We sent some data.
	 */

	count = (*esp->dma_ops->restart_dataout_2)
			(esp->dma_state, tgt, sizeof(B2048));

	/* last chunk? */
	if (tgt->transient_state.out_count == count) esp->script++;
	else enable_hf(esp);

	return FALSE;
}

boolean_t
esp_dosynch(esp, csr, ir)
	register esp_softc_t	esp;
	register unsigned char	csr, ir;
{
	register esp_padded_regmap_t	*regs = esp->regs;
	register unsigned char	c;
	int i, per, offs;
	register target_info_t	*tgt;

	/*
	 * Phase is MSG_OUT here.
	 * Try synch negotiation, unless prohibited
	 */
	tgt = esp->active_target;
	regs->esp_cmd = ESP_CMD_FLUSH;
	delay(2);

	per = esp_min_period;
	if (BGET(scsi_no_synchronous_xfer,esp->sc->masterno,tgt->target_id))
		offs = 0;
	else
		offs = esp_max_offset;

	tgt->flags |= TGT_DID_SYNCH;	/* only one chance */
	tgt->flags &= ~TGT_TRY_SYNCH;

	regs->esp_fifo = SCSI_EXTENDED_MESSAGE;
	regs->esp_fifo = 3;
	regs->esp_fifo = SCSI_SYNC_XFER_REQUEST;
	regs->esp_fifo = esp_to_scsi_period(esp_min_period);
	regs->esp_fifo = offs;
	delay(2);
	regs->esp_cmd = ESP_CMD_XFER_INFO;
	csr = esp_wait(regs, SCSI_INT);
	ir = regs->esp_intr;

	if (SCSI_PHASE(csr) != SCSI_PHASE_MSG_IN)
		gimmeabreak();

	delay(2);
	regs->esp_cmd = ESP_CMD_XFER_INFO;
	csr = esp_wait(regs, SCSI_INT);
	ir = regs->esp_intr;

	delay(2);
	while ((regs->esp_flags & ESP_FLAGS_FIFO_CNT) > 0) {
		delay(2);
		c = regs->esp_fifo;	/* see what it says */
		delay(2);
	}

	if (c == SCSI_MESSAGE_REJECT) {
		printf(" (asynchronous)");

		delay(2);
		regs->esp_cmd = ESP_CMD_MSG_ACPT;
		csr = esp_wait(regs, SCSI_INT);
		c = regs->esp_intr;

		/*
		 * Phase is MSG_OUT here.
		 * Send a NOP message out to satisfy the targets
		 * request for a message and to drop the ATN signal
		 */
		delay(2);
		regs->esp_fifo = SCSI_NOP;
		delay(2);
		regs->esp_cmd = ESP_CMD_XFER_INFO;
		csr = esp_wait(regs, SCSI_INT);
		ir = regs->esp_intr;

		goto cmd;
	}

	/*
	 * Receive the rest of the message
	 */
	delay(2);
	regs->esp_cmd = ESP_CMD_MSG_ACPT;
	esp_wait(regs, SCSI_INT);
	ir = regs->esp_intr;

	if (c != SCSI_EXTENDED_MESSAGE)
		gimmeabreak();

	delay(2);
	regs->esp_cmd = ESP_CMD_XFER_INFO;
	esp_wait(regs, SCSI_INT);
	c = regs->esp_intr;
	delay(2);
	if (regs->esp_fifo != 3)
		panic("esp_dosynch");

	for (i = 0; i < 3; i++) {
		delay(2);
		regs->esp_cmd = ESP_CMD_MSG_ACPT;
		esp_wait(regs, SCSI_INT);
		c = regs->esp_intr;

		delay(2);
		regs->esp_cmd = ESP_CMD_XFER_INFO;
		esp_wait(regs, SCSI_INT);
		c = regs->esp_intr; /* ack */
		delay(2);
		c = regs->esp_fifo;

		if (i == 1) tgt->sync_period = scsi_period_to_esp(c);
		if (i == 2) tgt->sync_offset = c;
	}

	delay(2);
	regs->esp_cmd = ESP_CMD_MSG_ACPT;
	csr = esp_wait(regs, SCSI_INT);
	c = regs->esp_intr;

cmd:
	reset_fifo(esp);

	/* phase should normally be command here */
	if (SCSI_PHASE(csr) == SCSI_PHASE_CMD) {
		register char	*cmd = tgt->cmd_ptr;

		/* test unit ready or inquiry */
		for (i = 0; i < sizeof(scsi_command_group_0); i++)
			regs->esp_fifo = *cmd++;
		delay(2);
		ESP_TC_PUT(regs,0xff);
		delay(2);
		regs->esp_cmd = ESP_CMD_XFER_PAD | ESP_CMD_DMA; /* 0x98 */

		if (tgt->cur_cmd == SCSI_CMD_INQUIRY) {
			tgt->transient_state.script = esp_script_data_in;
			esp->script = tgt->transient_state.script;
			return FALSE;
		}

		csr = esp_wait(regs, SCSI_INT);

		/* Cleanup the FIFO */
		regs->esp_cmd = ESP_CMD_FLUSH;
		delay(2);

		ir = regs->esp_intr; /* ack */
	}

status:
	if (SCSI_PHASE(csr) != SCSI_PHASE_STATUS)
		gimmeabreak();

	return TRUE;
}

/*
 * The bus was reset
 */
esp_bus_reset(esp)
	register esp_softc_t	esp;
{
	register esp_padded_regmap_t	*regs = esp->regs;

	LOG(0x1d,"bus_reset");

	/*
	 * Clear command (needed?)
	 */
	regs->esp_cmd = ESP_CMD_NOP;

	/*
	 * Clear bus descriptor
	 */
	esp->script = 0;
	esp->error_handler = 0;
	esp->active_target = 0;
	esp->next_target = 0;
	esp->state &= ESP_STATE_SPEC_DMA;	/* save this one bit only */
	queue_init(&esp->waiting_targets);
	esp->wd.nactive = 0;
	esp_reset(esp, TRUE);

	printf("esp: (%d) bus reset ", ++esp->wd.reset_count);
	delay(scsi_delay_after_reset); /* some targets take long to reset */

	if (esp->sc == 0)	/* sanity */
		return;

	scsi_bus_was_reset(esp->sc);
}

/*
 * Disconnect/reconnect mode ops
 */

/* get the message in via dma */
boolean_t
esp_msg_in(esp, csr, ir)
	register esp_softc_t	esp;
	register unsigned char	csr, ir;
{
	register target_info_t	*tgt;
	register esp_padded_regmap_t	*regs = esp->regs;
	unsigned char		ff = regs->esp_flags;

	LOG(0x10,"msg_in");

	/*
	 * Clear board FIFOs
	 */
	reset_fifo(esp);
        
	/* must clean FIFO as in esp_xfer_in, sigh */
	while ((ff & ESP_FLAGS_FIFO_CNT) != 0) {
		delay(2);
		ff = regs->esp_fifo;
		delay(2);
		ff = regs->esp_flags;
	}

	(void)(*esp->dma_ops->start_msgin)(esp->dma_state, esp->active_target);

#if	0
	/* We only really expect two bytes, at tgt->cmd_ptr */
	ESP_TC_PUT(regs, sizeof(scsi_command_group_0));
	delay(2);
#endif	/* 0 */

	/*
	 * We only really expect two bytes (in the esp fifo)
	 * Each byte must be received one at a time and creates an interrupt
	 */

	return TRUE;
}

/* check the message is indeed a DISCONNECT */
boolean_t
esp_disconnect(esp, csr, ir)
	register esp_softc_t	esp;
	register unsigned char	csr, ir;

{
#if	0
	register char		*msgs;
#endif	/* 0 */
	register unsigned char	ff;
	register target_info_t	*tgt;
	esp_padded_regmap_t	*regs;
	unsigned char		msg;

	tgt = esp->active_target;
#if	0
	msgs = tgt->cmd_ptr;
#endif	/* 0 */

	(*esp->dma_ops->end_msgin)(esp->dma_state, tgt);

	/*
	 * Do not do this. It is most likely a reconnection
	 * message that sits there already by the time we
	 * get here.  The chip is smart enough to only dma
	 * the bytes that correctly came in as msg_in proper,
	 * the identify and selection bytes are not dma-ed.
	 * Yes, sometimes the hardware does the right thing.
	 */

#if	0
	/* First check message got out of the fifo */
	regs = esp->regs;
	ff = regs->esp_flags;
	while ((ff & ESP_FLAGS_FIFO_CNT) != 0) {
		*msgs++ = regs->esp_fifo;
		ff = regs->esp_flags;
	}
	msgs = tgt->cmd_ptr;
#endif	/* 0 */

#if	0
	/* A SDP message preceeds it in non-completed READs */
	if ((msgs[0] == SCSI_DISCONNECT) ||	/* completed */
	    ((msgs[0] == SCSI_SAVE_DATA_POINTER) && /* non complete */
	     (msgs[1] == SCSI_DISCONNECT))) {
		/* that's the ok case */
	} else
		printf("[tgt %d bad SDP: x%x]",
			tgt->target_id, *((unsigned short *)msgs));
#endif	/* 0 */

	regs = esp->regs;

	ff = regs->esp_flags & ESP_FLAGS_FIFO_CNT;
	delay(2);
	assert(ff == 1);
	while (ff > 0) {
		msg = regs->esp_fifo;	/* see what it says */
		delay(2);
		ff = regs->esp_flags & ESP_FLAGS_FIFO_CNT;
		delay(2);
	}

	regs->esp_cmd = ESP_CMD_MSG_ACPT;
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
			csr = esp_wait(regs, SCSI_INT);
			ir = regs->esp_intr;

			delay(2);
			regs->esp_cmd = ESP_CMD_XFER_INFO;
			csr = esp_wait(regs, SCSI_INT);
			ir = regs->esp_intr;
			delay(2);

			ff = regs->esp_flags & ESP_FLAGS_FIFO_CNT;
			delay(2);
			assert(ff == 1);
			while (ff > 0) {
				msg = regs->esp_fifo;	/* see what it says */
				delay(2);
				ff = regs->esp_flags & ESP_FLAGS_FIFO_CNT;
				delay(2);
			}

			regs->esp_cmd = ESP_CMD_MSG_ACPT;
			delay(2);

			if (msg != SCSI_DISCONNECT) {
				printf("[tgt %d bad disconnect message: 0x%02x]\n",
					tgt->target_id, msg);
			}
			break;

		case SCSI_RESTORE_POINTERS:
			printf("[tgt %d]\n", tgt->target_id);
			panic("restore data pointer message");
			break;

		default:
			printf("[tgt %d bad SDP: 0x%02x]\n", tgt->target_id, msg);
			break;
	}

	/* wait for previous command to complete */
	csr = esp_wait(regs, SCSI_INT);
	ir = regs->esp_intr;

	/* this must be done within 250msec of disconnect */
	if (ir & ESP_INT_DISC) {
		delay(2);
		regs->esp_cmd = ESP_CMD_ENABLE_SEL;
		delay(2);
	} else {
		printf("[tgt %d did not disconnect]\n", tgt->target_id);
	}

	return TRUE;
}

/* save all relevant data, free the BUS */
boolean_t
esp_disconnected(esp, csr, ir)
	register esp_softc_t	esp;
	register unsigned char	csr, ir;

{
	register target_info_t	*tgt;

	LOG(0x11,"disconnected");
	esp_disconnect(esp,csr,ir);

	tgt = esp->active_target;
	tgt->flags |= TGT_DISCONNECTED;
	tgt->transient_state.handler = esp->error_handler;
	/* anything else was saved in esp_err_disconn() */

	PRINT("{D%d}", tgt->target_id);

	esp_release_bus(esp);

	return FALSE;
}

/* get reconnect message out of fifo, restore BUS */
int reconn_cnt, collision_cnt;

boolean_t
esp_reconnect(esp, csr, ir)
	register esp_softc_t	esp;
	register unsigned char	csr, ir;

{
	register target_info_t	*tgt;
	esp_padded_regmap_t	*regs;
	register unsigned char	ff;
	int			id;

reconn_cnt++;
if (ir & ESP_INT_ILL) collision_cnt++;

	LOG(0x12,"reconnect");
	/*
	 * See if this reconnection collided with a selection attempt
	 */
	if (esp->state & ESP_STATE_BUSY)
		esp->state |= ESP_STATE_COLLISION;

	esp->state |= ESP_STATE_BUSY;

	/* find tgt: first byte in fifo is (tgt_id|our_id) */
	regs = esp->regs;

	/*
	 * The chip FIFO should have at least two bytes (the
	 * reselection bus id and the identify message) and
	 * may also have a third byte (the identify message
	 * from a command that was being sent to another target).
	 * See esp_attempt_selection().
	 */
	ff = regs->esp_flags & ESP_FLAGS_FIFO_CNT;
	if (ff < 2) {
		/*
		 * The 53C90 does not handle a select command during a
		 * reselection very well.  The code below attempts to
		 * correct the situation.  The problem is:
		 *
		 *   The reselected interrupt has had time to establish
		 *   itself; the select command is written after the
		 *   interrupt occurs.  It is stacked in the command
		 *   register to be executed after the reselect interrupt
		 *   is serviced.  This causes a reselect interrupt followed
		 *   by an illegal command interrupt.
		 *
		 * This problem was fixed in the 53C90A part.
		 * Refer to the table on NCR SCSI Enginerring Note No. 820
		 * page 4 which shows the response when a select command is
		 * written to the command register.
		 */

		/*
		 * Should have had a collision and the last command was
		 * the a message accept for the reconnect sequence.
		 */
		if ((esp->state & ESP_STATE_COLLISION) &&
		    (regs->esp_cmd == ESP_CMD_MSG_ACPT)) return FALSE;

		/*
		 * The expected conditions for the chip reselect problem
		 * did not occur.  Print a message and continue on.  If
		 * things are completely busted the watchdog will catch it.
		 */
		printf("[tgt %d reconnect ff = %d]\n", tgt->target_id, ff);
		return FALSE;
	}

	id = regs->esp_fifo & 0xff;	/* must decode this now */
	id &= ~(1 << esp->sc->initiator_id);
	{
		register int	i;
		for (i = 0; id > 1; i++)
			id >>= 1;
		id = i;
	}

	/* sanity, empty fifo */
	if ((ff = regs->esp_fifo) != SCSI_IDENTIFY) {
		printf("[id %d bad identify msg: x%x]", id, ff);
		gimmeabreak();
	}

	tgt = esp->sc->target[id];
	if (id > 7 || tgt == 0) panic("esp_reconnect");

	/*
	 * The chip FIFO may have the identify message in it
	 * from a command that was being sent to another target
	 * Just flush it away; a collision should have been detected.
	 */
	/* must clean FIFO as in esp_xfer_in, sigh */
	ff = regs->esp_flags;
	delay(2);
	while ((ff & ESP_FLAGS_FIFO_CNT) != 0) {
		ff = regs->esp_fifo;
		delay(2);
		ff = regs->esp_flags;
		delay(2);
	}

	/* synch things*/
	regs->esp_syn_p = tgt->sync_period;
	delay(2);
	regs->esp_syn_o = tgt->sync_offset;
	delay(2);

	PRINT("{R%d}", id);
	if (esp->state & ESP_STATE_COLLISION)
		PRINT("[B %d-%d]", esp->active_target->target_id, id);

	LOG(0x80+id,0);

	esp->active_target = tgt;
	tgt->flags &= ~TGT_DISCONNECTED;

	esp->script = tgt->transient_state.script;
	esp->error_handler = tgt->transient_state.handler;
	esp->in_count = 0;
	esp->out_count = 0;

	/*
	 * Not sure why, but this prevents tk50s from embarassing
	 * us quite a bit
	 */
	delay(4);

	regs->esp_cmd = ESP_CMD_MSG_ACPT;
	delay(2);

	return FALSE;
}

/*
 * Error handlers
 */

/*
 * Fall-back error handler.
 */
esp_err_generic(esp, csr, ir)
	register esp_softc_t	esp;
{
	esp_padded_regmap_t	*regs = esp->regs;


	LOG(0x13,"err_generic");

	/* handle non-existant or powered off devices here */
	if ((ir == ESP_INT_DISC) &&
	    (esp_isa_select(esp->cmd_was)) &&
	    (ESP_SS(esp->ss_was) == 0)) {
		/* Powered off ? */
		if (esp->active_target->flags & TGT_FULLY_PROBED)
			esp->active_target->flags = 0;
		esp->done = SCSI_RET_DEVICE_DOWN;

		/* empty fifo, there is garbage in it if timeout */
		regs->esp_cmd = ESP_CMD_FLUSH;
		delay(2);

		esp_end(esp, csr, ir);
		return;
	}

	switch (SCSI_PHASE(csr)) {
	case SCSI_PHASE_STATUS:
		if (esp->script[-1].condition == SCSI_PHASE_STATUS) {
			/* some are just slow to get out.. */
		} else
			esp_err_to_status(esp, csr, ir);
		return;
		break;
	case SCSI_PHASE_DATAI:
		if (esp->script->condition == SCSI_PHASE_STATUS) {
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
			PRINT("{P}");
			return;
		}
		break;
	case SCSI_PHASE_DATAO:
		if (esp->script->condition == SCSI_PHASE_STATUS) {
			/*
			 * See comment above. Actually seen on hitachis.
			 */
			PRINT("{P}");
			return;
		}
	}
	gimmeabreak();
}

/*
 * Handle generic errors that are reported as
 * an unexpected change to STATUS phase
 */
esp_err_to_status(esp, csr, ir)
	register esp_softc_t	esp;
{
	script_t		scp = esp->script;

	LOG(0x14,"err_to_status");
	while (scp->condition != SCSI_PHASE_STATUS)
		scp++;
	(*scp->action)(esp, csr, ir);
	esp->script = scp + 1;
	esp->regs->esp_cmd = scp->command;
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
	esp->done = SCSI_RET_NEED_SENSE;
#endif
}

/*
 * Handle disconnections as exceptions
 */
esp_err_disconn(esp, csr, ir)
	register esp_softc_t	esp;
	register unsigned char	csr, ir;
{
	register esp_padded_regmap_t	*regs;
	register target_info_t	*tgt;
	int			count;
	boolean_t		callback = FALSE;

	LOG(0x16,"err_disconn");

	if (SCSI_PHASE(csr) != SCSI_PHASE_MSG_IN)
		return esp_err_generic(esp, csr, ir);

	regs = esp->regs;
	tgt = esp->active_target;

	switch (esp->script->condition) {
	case SCSI_PHASE_DATAO:
		LOG(0x1b,"+DATAO");
		if (esp->out_count) {
			register int xferred;

			ESP_TC_GET(regs,xferred); /* temporary misnomer */
			delay(2);
			xferred += regs->esp_flags & ESP_FLAGS_FIFO_CNT;
			xferred -= esp->extra_count;
			xferred = esp->out_count - xferred; /* ok now */
			tgt->transient_state.out_count -= xferred;

			callback = (*esp->dma_ops->disconn_1)
					(esp->dma_state, tgt, xferred);

			tgt->transient_state.script =
				esp_script_restart_data_out;

		} else {

			callback = (*esp->dma_ops->disconn_2)
					(esp->dma_state, tgt);

		}
		esp->extra_count = 0;
		break;


	case SCSI_PHASE_DATAI:
		LOG(0x17,"+DATAI");
		if (esp->in_count) {
			register int xferred;

			ESP_TC_GET(regs,count);
			delay(2);
			xferred = esp->in_count - count;

			tgt->transient_state.in_count = count;

			callback = (*esp->dma_ops->disconn_3)
					(esp->dma_state, tgt, xferred);

			tgt->transient_state.script = esp_script_restart_data_in;
			if (tgt->transient_state.in_count == 0)
				tgt->transient_state.script++;

		}
		else tgt->transient_state.script = esp->script;
		break;

	case SCSI_PHASE_STATUS:
		/* will have to restart dma */
		ESP_TC_GET(regs,count);
		delay(2);
		if (esp->state & ESP_STATE_DMA_IN) {
			register int xferred;

			LOG(0x1a,"+STATUS+R");

			xferred = esp->in_count - count;

			tgt->transient_state.in_count = count;

			callback = (*esp->dma_ops->disconn_4)
					(esp->dma_state, tgt, xferred);

			tgt->transient_state.script = esp_script_restart_data_in;
			if (tgt->transient_state.in_count == 0)
				tgt->transient_state.script++;

		} else {

			/* add what's left in the fifo */
			count += (regs->esp_flags & ESP_FLAGS_FIFO_CNT);
			/* take back the extra we might have added */
			count -= esp->extra_count;
			/* ..and drop that idea */
			esp->extra_count = 0;

			LOG(0x19,"+STATUS+W");

			if ((count == 0) &&
			    (tgt->transient_state.out_count == esp->out_count)) {
				/* all done */
				tgt->transient_state.script = esp->script;
				tgt->transient_state.out_count = 0;
			} else {
				register int xferred;

				/* how much we xferred */
				xferred = esp->out_count - count;

				tgt->transient_state.out_count -= xferred;

				callback = (*esp->dma_ops->disconn_5)
						(esp->dma_state, tgt, xferred);

				tgt->transient_state.script = esp_script_restart_data_out;
			}
			esp->out_count = 0;
		}
		break;
	default:
		gimmeabreak();
		return;
	}
	esp_msg_in(esp,csr,ir);
	esp->script = esp_script_disconnect;
#if	0
	regs->esp_cmd = ESP_CMD_XFER_INFO|ESP_CMD_DMA;
#endif	/* 0 */
	regs->esp_cmd = ESP_CMD_XFER_INFO;
	if (callback)
		(*esp->dma_ops->disconn_callback)(esp->dma_state, tgt);
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
esp_reset_scsibus(esp)
	register esp_softc_t	esp;
{
	register target_info_t	*tgt = esp->active_target;
	register esp_padded_regmap_t	*regs = esp->regs;
	register int		ir;

	if (scsi_debug && tgt) {
		int dmalen;
		ESP_TC_GET(esp->regs,dmalen);
		delay(2);
		printf("Target %d was active, cmd x%x in x%x out x%x Sin x%x Sou x%x dmalen x%x\n", 
			tgt->target_id, tgt->cur_cmd,
			tgt->transient_state.in_count, tgt->transient_state.out_count,
			esp->in_count, esp->out_count,
			dmalen);
	}
	ir = regs->esp_intr;
	if ((ir & ESP_INT_RESEL) && (SCSI_PHASE(regs->esp_csr) == SCSI_PHASE_MSG_IN)) {
		/* getting it out of the woods is a bit tricky */
		int	s = splbio();

		(void) esp_reconnect(esp, regs->esp_csr, ir);
		esp_wait(regs, SCSI_INT);
		ir = regs->esp_intr;
		regs->esp_cmd = ESP_CMD_MSG_ACPT;
		delay(2);
		splx(s);
	} else {
		regs->esp_cmd = ESP_CMD_BUS_RESET;
		delay(35);
	}
}

#endif	/* NESP > 0 */
