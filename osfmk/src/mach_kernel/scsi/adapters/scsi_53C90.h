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
 * Revision 2.1.2.1  92/03/28  10:14:27  jeffreyh
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
 *	File: scsi_53C90.h
 * 	Author: Jerrie Coffman
 *		Intel Corporation Supercomputer Systems Division
 *	Date:	12/91
 *
 *	Defines for the NCR 53C90 ESP (SCSI interface)
 *	Modified code received from Alessandro Forin at Carnegie Mellon
 *	University for the NCR 53C94 ASP (SCSI interface)
 */

/*
 * Register map
 */

typedef struct {
	volatile unsigned char	esp_tc_lsb;	/* rw: Transfer Counter LSB */
	volatile unsigned char	esp_tc_msb;	/* rw: Transfer Counter MSB */
	volatile unsigned char	esp_fifo;	/* rw: FIFO top */
	volatile unsigned char	esp_cmd;	/* rw: Command */
	volatile unsigned char	esp_csr;	/* r:  Status */
#define			esp_dbus_id esp_csr	/* w: Destination Bus ID */
	volatile unsigned char	esp_intr;	/* r:  Interrupt */
#define			esp_sel_timo esp_intr	/* w: (re)select timeout */
	volatile unsigned char	esp_ss;		/* r:  Sequence Step */
#define			esp_syn_p esp_ss	/* w: synchronous period */
	volatile unsigned char	esp_flags;	/* r:  FIFO flags */
#define			esp_syn_o esp_flags	/* w: synchronous offset */
	volatile unsigned char	esp_cnfg;	/* rw: Configuration */
	volatile unsigned char	esp_ccf;	/* w:  Clock Conv. Factor */
	volatile unsigned char	esp_test;	/* rw: Test Mode */
} esp_regmap_t;


/*
 * Transfer Count: access macros
 * That a NOP is required after loading the dma counter
 * I learned on the NCR test code.
 */

#define	ESP_TC_MAX	0x10000

#define ESP_TC_GET(ptr, val)				\
	val = (ptr)->esp_tc_lsb;			\
	delay(2);					\
	val |= ((ptr)->esp_tc_msb << 8);

#define ESP_TC_PUT(ptr, val)				\
	(ptr)->esp_tc_lsb = (val);			\
	delay(2);					\
	(ptr)->esp_tc_msb = (val) >> 8;			\
	delay(2);					\
	(ptr)->esp_cmd = ESP_CMD_NOP | ESP_CMD_DMA

/*
 * FIFO register
 */

#define ESP_FIFO_DEEP		16


/*
 * Command register (command codes)
 */

#define ESP_CMD_DMA		0x80
					/* Miscellaneous */
#define ESP_CMD_NOP		0x00
#define ESP_CMD_FLUSH		0x01
#define ESP_CMD_RESET		0x02
#define ESP_CMD_BUS_RESET	0x03
					/* Initiator state */
#define ESP_CMD_XFER_INFO	0x10
#define ESP_CMD_I_COMPLETE	0x11
#define ESP_CMD_MSG_ACPT	0x12
#define ESP_CMD_XFER_PAD	0x18
#define ESP_CMD_SET_ATN		0x1a
					/* Target state */
#define ESP_CMD_SND_MSG		0x20
#define ESP_CMD_SND_STATUS	0x21
#define ESP_CMD_SND_DATA	0x22
#define ESP_CMD_DISC_SEQ	0x23
#define ESP_CMD_TERM		0x24
#define ESP_CMD_T_COMPLETE	0x25
#define ESP_CMD_DISC		0x27
#define ESP_CMD_RCV_MSG		0x28
#define ESP_CMD_RCV_CDB		0x29
#define ESP_CMD_RCV_DATA	0x2a
#define ESP_CMD_RCV_CMD		0x2b
					/* Disconnected state */
#define ESP_CMD_RESELECT	0x40
#define ESP_CMD_SEL		0x41
#define ESP_CMD_SEL_ATN		0x42
#define ESP_CMD_SEL_ATN_STOP	0x43
#define ESP_CMD_ENABLE_SEL	0x44
#define ESP_CMD_DISABLE_SEL	0x45

/* this is approximate (no ATN3) but good enough */	/* no ATN3 in 53C90 */
#define	esp_isa_select(cmd)	(((cmd)&0x7c)==0x40)

/*
 * Status register, and phase encoding
 */

#define ESP_CSR_GE		(1 << 6)
#define ESP_CSR_PE		(1 << 5)
#define ESP_CSR_TC		(1 << 4)
#define ESP_CSR_VGC		(1 << 3)
#define ESP_CSR_MSG		(1 << 2)
#define ESP_CSR_CD		(1 << 1)
#define ESP_CSR_IO		(1 << 0)

#define	ESP_PHASE(csr)		SCSI_PHASE(csr)

/*
 * Destination Bus ID
 */

#define ESP_DEST_ID_MASK	0x07


/*
 * Interrupt register
 */

#define ESP_INT_RESET		(1 << 7)
#define ESP_INT_ILL		(1 << 6)
#define ESP_INT_DISC		(1 << 5)
#define ESP_INT_BS		(1 << 4)
#define ESP_INT_FC		(1 << 3)
#define ESP_INT_RESEL		(1 << 2)
#define ESP_INT_SEL_ATN		(1 << 1)
#define ESP_INT_SEL		(1 << 0)


/*
 * Timeout register value:
 *
 *	val = (timeout * CLK_freq) / (8192 * CCF);
 */

#define	ESP_TIMEOUT_250		0xa3

/*
 * Sequence Step register
 */

#define ESP_SS_MASK		0x07
#define	ESP_SS(ss)		((ss)&ESP_SS_MASK)

/*
 * Synchronous Transfer Period
 */

#define ESP_STP_MASK		0x1f
#define ESP_STP_MIN		0x05	/* 5 clk/byte, 1 clk = 62.5 ns */
#define ESP_STP_MAX		0x03	/* after ovfl, 35 clk/byte */

/*
 * FIFO flags
 */

#define ESP_FLAGS_FIFO_CNT	0x1f

/*
 * Synchronous offset
 */

#define ESP_SYNO_MASK		0x0f		/* 0 -> asyn */

/*
 * Configuration
 */

#define ESP_CNFG_SLOW		(1 << 7)
#define ESP_CNFG_SRD		(1 << 6)
#define ESP_CNFG_P_TEST		(1 << 5)
#define ESP_CNFG_P_CHECK	(1 << 4)
#define ESP_CNFG_TEST		(1 << 3)
#define ESP_CNFG_MY_BUS_ID	0x07

/*
 * CCF register values:
 *
 *	val = 200 / input clock period (ns);
 *
 * CCF must never be loaded with 1
 */

#define ESP_CCF_10MHz		0x2	/* 10 MHz */
#define ESP_CCF_15MHz		0x3	/* 10.01 MHz to 15 MHz */
#define ESP_CCF_20MHz		0x4	/* 15.01 MHz to 20 MHz */
#define ESP_CCF_25MHz		0x5	/* 20.01 MHz to 25 MHz */

/*
 * Test register
 */

#define ESP_TEST_HI_Z		(1 << 2)
#define ESP_TEST_I		(1 << 1)
#define ESP_TEST_T		(1 << 0)
