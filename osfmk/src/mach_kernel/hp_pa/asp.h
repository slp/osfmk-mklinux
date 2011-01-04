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
 * Copyright (c) 1991,1992 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: Jasp.h 1.3 92/05/22$
 */

#ifndef	_HP_PA_ASP_H_
#define	_HP_PA_ASP_H_

#include <hp_pa/HP700/iotypes.h>

/*
 * The ASP Interrupt, LED control, and I/O Sub-System status registers
 * are mapped at ASP_CTRL and described in greater detail below.
 */

struct asp_ctrl {
	vu_int	irr;			/* (RO) Interrupt Request Register */
	vu_int	imr;			/* Interrupt Mask Register */
	vu_int	ipr;			/* Interrupt Pending Register */
	vu_int	icr;	
	vu_int  iar;	
	vu_int	resv1[3];
	vu_char	ledctrl;		/* (WO) LED Control */
	vu_char	pad[3];
	volatile struct {		/* (RO) I/O Sub-System Status */
		u_int		:20,
			spu_id	: 3,		/* SPU ID board jumper */
			sw_mode	: 1,		/* front swtch (0:svc, 1:nrm) */
			scsi_sel: 1,		/* SCSI clk sel (0:1X, 1:2X) */
			lan_jmpr: 2,		/* LAN jumper (see below) */
			lan_fuse: 1,		/* LAN AUI fuse (0:blown) */
			scsi_pwr: 1,		/* SCSI power (0:power lost) */
			scsi_id	: 3;		/* SCSI ID */
	} iostat;
};

/* interrupts defined for ASP (bitmasks and associated "line" numbers) */
#define	INT_EISA_NMI	0x00000001	/* EISA non-maskable */
#define	INT_EISA_NMI_L	31
#define	INT_8042_GEN	0x00000002	/* general 8042 */
#define	INT_8042_GEN_L	30
#define	INT_8042_HI	0x00000004	/* high priority 8042 */
#define	INT_8042_HI_L	29
#define	INT_FWSCSI	0x00000008	/* Fast/Wide SCSI */
#define	INT_FWSCSI_L	28
#define	INT_FDDI	0x00000010	/* FDDI */
#define	INT_FDDI_L	27
#define	INT_RS232_1	0x00000020	/* RS232 port #1 */
#define	INT_RS232_1_L	26
#define	INT_RS232_2	0x00000040	/* RS232 port #2 */
#define	INT_RS232_2_L	25
#define	INT_CENT	0x00000080	/* Centronics */
#define	INT_CENT_L	24
#define	INT_LAN		0x00000100	/* LAN */
#define	INT_LAN_L	23
#define	INT_SCSI	0x00000200	/* SCSI */
#define	INT_SCSI_L	22
#define	INT_EISA	0x00000400	/* EISA */
#define	INT_EISA_L	21
#define	INT_GRF1	0x00000800	/* graphics #1 */
#define	INT_GRF1_L	20
#define	INT_GRF2	0x00001000	/* graphics #2 */
#define	INT_GRF2_L	19
#define	INT_DOMKEY	0x00002000	/* Domain keyboard */
#define	INT_DOMKEY_L	18
#define	INT_BUSERR	0x00004000	/* bus error */
#define	INT_BUSERR_L	17

#define	INT_PS2		0x04000000	/* PS/2 keyboard/mouse interface */
#define	INT_PS2_L	5

#define	ASPINT_MASK	0x04007fff	/* bitmask of valid ASP interrupts */
#define	ASPINT_LAST_L	INT_PS2_L	/* last defined ASP interrupt */

#ifdef USELEDS

extern int inledcontrol;

void ledinit(void);
void ledcontrol(int, int, int);

#define	LED_DATA	0x01		/* data to shift (0:on 1:off) */
#define	LED_STROBE	0x02		/* strobe to clock data */

#define	LED7		0x80		/* top (or furthest right) LED */
#define	LED6		0x40
#define	LED5		0x20
#define	LED4		0x10
#define	LED3		0x08
#define	LED2		0x04
#define	LED1		0x02
#define	LED0		0x01		/* bottom (or furthest left) LED */

#define	LED_LANXMT	LED0		/* for LAN transmit activity */
#define	LED_LANRCV	LED1		/* for LAN receive activity */
#define	LED_DISK	LED2		/* for disk activity */
#define	LED_PULSE	LED3		/* heartbeat */

#endif /* USELEDS */

/* iostat, lan_jmpr */
#define	LANJMPR_INVAL	0		/* invalid */
#define	LANJMPR_AUI	1		/* AUI port selected */
#define	LANJMPR_THIN	2		/* internal ThinLAN port selected */
#define	LANJMPR_MALGN	3		/* jumper misaligned */
#define	DISP_LANJMPR(j)	\
	(j == LANJMPR_THIN)? "ThinLAN": (j == LANJMPR_AUI)? "AUI": \
	(j == LANJMPR_MALGN)? "_jumper_misaligned_": "_invalid_"

/* iostat, spu_id */
#define	SPUID_COBRA	0		/* cobra */
#define	SPUID_CORAL	1		/* coral */
#define	SPUID_BUSH	2		/* bushmaster */
#define	SPUID_HARDBALL	3		/* hardball */
#define	SPUID_SCORPIO	4		/* scorpio */
#define	SPUID_CORALII	5		/* coral II */

#define	MAXMODCORE	16

#define	CORE_ASP	((struct core_asp *)0xF082F000)
#define	CORE_LASI	((struct core_lasi *)0xF010C000)

/*
 * Core ASP DMA (located at CORE_DMA)
 *
 * A 32-bit physical address is constructed as follows:
 *	+--------+--------+-----------------+
 *	| hipage | lopage |      addr       |
 *	+--------+--------+-----------------+
 *
 * A 24-bit count is constructed as follows:
 *	+--------+-----------------+
 *	| hicnt  |       cnt       |
 *	+--------+-----------------+
 *
 * N.B. The ASP has only 1 DMA channel.  Therefore, everywhere you see
 * reference to a "channel mask", only the low order bit is significant.
 */
struct core_dma {
	vu_char	addr;		/* current byte address (16-bit, low/high) */
	vu_char	cnt;		/* current byte count (16-bit, low/high) */
	vu_char	resv1[6];
	vu_char	status;		/* (RO) status (see below) */
	vu_char	resv2;
	vu_char	wrmask;		/* (WO) set/clear a mask register bit */
	vu_char	mode;		/* (WO) mode (see below) */
	vu_char	clrbyte;	/* (WO) clear byte pointer */
	vu_char	reset;		/* (WO) master clear */
	vu_char	clrmask;	/* (WO) enable DMA (clear mask bit) */
	vu_char	mask;		/* 1:start DMA on a specific channel */
	vu_char	fifolim;	/* when to master SGC bus for rw transactions */
	vu_char	resv3[118];
	vu_char	lopage;		/* current low page of address */
	vu_char	resv4[889];
	vu_char	hicnt;		/* current high part of byte count */
	vu_char	resv5[8];
	vu_char	intrlog;	/* 1:DMA cnt register has changed */
	vu_char	resv6[124];
	vu_char	hipage;
};

/* core_dma.status */
#define	DMASTAT_DONE	0x01	/* cleared when DMA is finished */
#define	DMASTAT_SVC	0x08	/* set when DMA needs servicing */

/* core_dma.mode */
#define	DMAMODE_NONE	0x00	/* disable DMA */
#define	DMAMODE_WRITE	0x04	/* write transfer */
#define	DMAMODE_READ	0x08	/* read transfer */
#define	DMAMODE_NONE2	0x0C	/* disable DMA */

/* core_dma.fifolim */
#define	DMAFIFO_WMASK	0x0f	/* if words in fifo <=, memread masters SGC */
#define	DMAFIFO_RMASK	0xf0	/* if words in fifo >=, memwrite masters SGC */

/*
 * Core RRC, HP-HIL, Sound (located at CORE_8042)
 */
struct core_8042 {
	vu_char	rsthold;	/* (WO) reset hold (and Serial #3) */
	vu_char	resv1[2047];
	vu_char	data;		/* send/receive data to/from 8042 */
	vu_char	status;		/* status/control to/from 8042 */
	vu_char	resv2[1022];
	vu_char	rstrel;		/* (WO) reset release (and Serial #3) */
};

/*
 * Core RS232 ports (located at CORE_RS232_1 and CORE_RS232_2).
 */
struct core_rs232 {
	vu_char	reset;		/* (WO) reset RS232 ports */
	vu_char	resv1[2047];
	vu_char	dcadev[16];	/* see "struct dcadevice" in "dcareg.h" */
};

/*
 * Core Centronics Parallel Printer Interface (located at CORE_CENT).
 */
struct core_cent {
	vu_char	reset;		/* (WO) reset Centronix port */
	vu_char	resv1[2047];
	vu_char	data;		/* read/write data */
	vu_char	status;		/* (RO) status register */
	vu_char	control;	/* device control register */
	vu_char	resv2;
	vu_char	mode;		/* mode control */
	vu_char	intr;		/* IW control/interrupt status */
	vu_char	delay;		/* delay counter */
};

/*
 * Core SCSI (located at CORE_SCSI).
 */
struct core_scsi {
	vu_char	reset;		/* (WO) reset SCSI */
	vu_char	resv1[255];
	vu_char	scsidev[256];	/* see "struct scsidevice" in "scsireg.h" */
};

/*
 * Core LAN (located at CORE_LAN).
 *
 * This is an Intel 82596DX.
 */
struct core_lan {
	vu_int	reset;		/* (WO) reset LAN */
	vu_int	port;		/* (WO) 82596DX CPU cmd port (64-bit, up/low) */
	vu_int	attn;		/* (WO) issue channel attention to 82596DX */
};

/*
 * DMA reset (located at CORE_DMARST).
 */
struct core_dmarst {
	vu_char	reset;		/* (WO) DMA reset */
};

struct core_ps2 {
	vu_int id_reset;
	vu_int data;
	vu_int control;
	vu_int status;
};

/*
 * Structures defining memory-mapped registers in the Core IOSS.
 */
struct core_asp {
	vu_char	reset;		/* (WO) IOSS reset */
	vu_char	resv1[31];
	vu_char	version;	/* (RO) version byte */
	vu_char	resv2[15];
	vu_char	scsi_dsync;	/* (WO) SCSI DSYNC enable bit (1:enable) */
	vu_char	resv3[15];
	vu_char	error;		/* bus errors while ASP masters SGC/VSC */
};

/*
 * Structures defining memory-mapped registers in the Core IOSS (LASI).
 */
struct core_lasi {
	vu_int	power;
	vu_int	error;
	vu_int	version;
	vu_int	reset;
	vu_int	arbmask;
};

/* core_asp.error */
#define	ASPERR_SCSI	0x1	/* SCSI was bus master */
#define	ASPERR_LAN	0x2	/* LAN was bus master */
#define	ASPERR_PLDMA	0x4	/* Parallel Port DMA was bus master */

/*
 * Core FWSCSI (located at CORE_FWSCSI).
 */
struct core_fwscsi {
	vu_char	reset;		/* (WO) reset SCSI */
	vu_char	resv1[3];
	vu_char	numxfers;	/* (WO) number of transfers */
	vu_char	resv2[251];
	vu_char	fwscsidev[256];	/* see "struct scsidevice" in "scsireg.h" */
};

#endif	/* _HP_PA_ASP_H_ */
