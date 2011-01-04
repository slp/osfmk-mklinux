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


/* INTEL_HIST */
/* Revision 1.7  1993/09/16  18:12:32  richardg
 * modified the memory allocation to evenly divide available MIO memory between existing scsi devices
 *
 * Revision 1.6  1993/09/15  21:06:41  jerrie
 * Added definitions for the NCR 53CF94 FSC SCSI chip from the
 * Rev. 2.0 "NCR 53CF94/96-2 Fast SCSI Controller Data Manual"
 * Dated 04/93.
 *
 * Revision 1.5  1993/06/30  22:55:15  dleslie
 * Adding copyright notices required by legal folks
 *
 * Revision 1.4  1993/04/27  20:49:06  dleslie
 * Copy of R1.0 sources onto main trunk
 *
 * Revision 1.2.6.2  1993/04/22  18:54:36  dleslie
 * First R1_0 release
 */
/* INTEL_ENDHIST */

/* CMU_HIST */
/*
 * Revision 2.8  91/08/24  12:25:28  af
 * 	Moved padding down in impl file.
 * 	[91/08/02  04:14:53  af]
 * 
 * Revision 2.7  91/06/19  16:28:46  rvb
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.6  91/05/14  17:28:39  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/05/13  06:04:57  af
 * 	Added asc_rfb register define, unused actually.
 * 	Use DMA for of nop for dma count loading.
 * 	Define macro to tell if command is a selection.
 * 	Move bus phase definitions in HBA-indep file.
 * 	[91/05/12  16:12:47  af]
 * 
 * Revision 2.4  91/02/05  17:44:59  mrt
 * 	Added author notices
 * 	[91/02/04  11:18:32  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:17:11  mrt]
 * 
 * Revision 2.3  90/12/05  23:34:46  af
 * 	Documented max DMA xfer size.
 * 	[90/12/03  23:39:36  af]
 * 
 * Revision 2.1.1.1  90/11/01  03:38:54  af
 * 	Created, from the DEC specs:
 * 	"PMAZ-AA TURBOchannel SCSI Module Functional Specification"
 * 	Workstation Systems Engineering, Palo Alto, CA. Aug 27, 1990.
 * 	And from the NCR data sheets
 * 	"NCR 53C94, 53C95, 53C96 Advanced SCSI Controller"
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
 *	File: scsi_53C94.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	9/90
 *
 *	Defines for the NCR 53C94 ASC (SCSI interface)
 * 	Some gotcha came from the "86C01/53C94 DMA lab work" written
 * 	by Ken Stewart (NCR MED Logic Products Applications Engineer)
 * 	courtesy of NCR.  Thanks Ken !
 */

#ifndef	_PPC_POWERMAC_SCSI_53C94_
#define	_PPC_POWERMAC_SCSI_53C94_

/*
 * Transfer Count: access macros
 * That a NOP is required after loading the dma counter
 * I learned on the NCR test code. Sic.
 */

/*
 * Note: a transfer count of 0 means the max transfer count
 * possible for each 53C94 or 53CF94 chip. Avoid this
 * by limiting the max transfer count to something slighty
 * below the max.
 */

#define       ASC_TC_MAX      (0x10000 - 4096)
#define       FAS_TC_MAX      (0x1000000 - 4096)
#define       FSC_TC_MAX      (0x1000000 - 4096)

#define ASC_TC_GET(ptr,val)                             \
        val = !(asc->type == ASC_NCR_53CF94) ? \
        (ptr)->asc_tc_lsb|((ptr)->asc_tc_msb<<8) :      \
        (ptr)->asc_tc_lsb|((ptr)->asc_tc_msb<<8)|((ptr)->fas_tc_hi<<16)

#define ASC_TC_PUT(ptr,val)                             \
        (ptr)->asc_tc_lsb=(val);                        \
        (ptr)->asc_tc_msb=(val)>>8;                     \
        if(asc->type == ASC_NCR_53CF94) \
                (ptr)->fas_tc_hi = (val)>>16;           \
        (ptr)->asc_cmd = ASC_CMD_NOP|ASC_CMD_DMA;

/*
 * Family Code and Revision Level: access macros
 *
 * Only valid when the following conditions are true:
 *
 *	- After power-up or chip reset
 *	- Before the fas_tc_hi register is loaded
 *	- The FAS_CNFG2_FEATURES bit is set
 *	- A DMA NOP command has been issued
 */

#define	FAMILY_MASK		0xf8
#define REV_MASK		0x07

#define FAS_FAMILY_CODE		0x02		/* Emulex FAS Controller */
#define FSC_FAMILY_CODE		0x14		/* NCR FSC Controller    */

#define	FAMILY(code)					\
	(((code)&FAMILY_MASK)>>3)

#define REV(level)					\
	((level)&REV_MASK)

/*
 * FIFO register
 */

#define ASC_FIFO_DEEP		16

/*
 * Command register (command bit masks)
 */

#define ASC_CMD_DMA		0x80
#define ASC_CMD_MODE_MASK	0x70
#define ASC_CMD_DISC_MODE	0x40
#define ASC_CMD_T_MODE		0x20
#define ASC_CMD_I_MODE		0x10
#define ASC_CMD_MASK		0x0f

/*
 * Command register (command codes)
 */
					/* Miscellaneous */
#define ASC_CMD_NOP		0x00
#define ASC_CMD_FLUSH		0x01
#define ASC_CMD_RESET		0x02
#define ASC_CMD_BUS_RESET	0x03
					/* Initiator state */
#define ASC_CMD_XFER_INFO	0x10
#define ASC_CMD_I_COMPLETE	0x11
#define ASC_CMD_MSG_ACPT	0x12
#define ASC_CMD_XFER_PAD	0x18
#define ASC_CMD_SET_ATN		0x1a
#define ASC_CMD_CLR_ATN		0x1b
					/* Target state */
#define ASC_CMD_SND_MSG		0x20
#define ASC_CMD_SND_STATUS	0x21
#define ASC_CMD_SND_DATA	0x22
#define ASC_CMD_DISC_SEQ	0x23
#define ASC_CMD_TERM		0x24
#define ASC_CMD_T_COMPLETE	0x25
#define ASC_CMD_DISC		0x27
#define ASC_CMD_RCV_MSG		0x28
#define ASC_CMD_RCV_CDB		0x29
#define ASC_CMD_RCV_DATA	0x2a
#define ASC_CMD_RCV_CMD		0x2b
#define ASC_CMD_ABRT_DMA	0x04
					/* Disconnected state */
#define ASC_CMD_RESELECT	0x40
#define ASC_CMD_SEL		0x41
#define ASC_CMD_SEL_ATN		0x42
#define ASC_CMD_SEL_ATN_STOP	0x43
#define ASC_CMD_ENABLE_SEL	0x44
#define ASC_CMD_DISABLE_SEL	0x45
#define ASC_CMD_SEL_ATN3	0x46

/* this is approximate (no ATN3) but good enough */
#define	asc_isa_select(cmd)	(((cmd)&0x7c)==0x40)

/*
 * Status register, and phase encoding
 */

#define ASC_CSR_INT		0x80
#define ASC_CSR_GE		0x40
#define ASC_CSR_PE		0x20
#define ASC_CSR_TC		0x10
#define ASC_CSR_VGC		0x08
#define ASC_CSR_MSG		0x04
#define ASC_CSR_CD		0x02
#define ASC_CSR_IO		0x01

#define	ASC_PHASE(csr)		SCSI_PHASE(csr)

/*
 * Destination Bus ID
 */

#define ASC_DEST_ID_MASK	0x07


/*
 * Interrupt register
 */

#define ASC_INT_RESET		0x80
#define ASC_INT_ILL		0x40
#define ASC_INT_DISC		0x20
#define ASC_INT_BS		0x10
#define ASC_INT_FC		0x08
#define ASC_INT_RESEL		0x04
#define ASC_INT_SEL_ATN		0x02
#define ASC_INT_SEL		0x01


/*
 * Timeout register:
 *
 *	The formula in the NCR specification does not yeild an accurate timeout
 *	The following formula is correct:
 *
 *	val = (timeout * CLK_freq) / (7682 * CCF);
 */

#define	asc_timeout_250(clk,ccf)	((31*clk)/ccf)

/* 250 msecs at the max clock frequency in each range (see CCF register ) */
#define	ASC_TIMEOUT_250		0xa3

/*
 * Sequence Step register
 */

#define ASC_SS_XXXX		0xf0
#define FAS_SS_XXXX		0xf8
#define ASC_SS_SOM		0x08
#define ASC_SS_MASK		0x07
#define	ASC_SS(ss)		((ss)&ASC_SS_MASK)

/*
 * Synchronous Transfer Period
 */

#define ASC_STP_MASK		0x1f
#define ASC_STP_MIN		0x05		/*  5 clk per byte */
#define ASC_STP_MAX		0x03		/* after ovfl, 35 clk/byte */
#define FAS_STP_MIN		0x04		/*  4 clk per byte */
#define FAS_STP_MAX		0x1f		/* 35 clk per byte */

/*
 * FIFO flags
 */

#define ASC_FLAGS_SEQ_STEP	0xe0
#define FAS_FLAGS_SEQ_CNT	0x20
#define ASC_FLAGS_FIFO_CNT	0x1f

/*
 * Synchronous offset
 */

#define ASC_SYNO_MASK		0x0f		/* 0 -> asyn */
#define	FAS_ASSERT_MASK		0x30		/* REQ/ACK assertion control */
#define	FAS_DEASSERT_MASK	0xc0		/* REQ/ACK deassertion control */

/*
 * Configuration 1
 */

#define ASC_CNFG1_SLOW		0x80
#define ASC_CNFG1_SRD		0x40
#define ASC_CNFG1_P_TEST	0x20
#define ASC_CNFG1_P_CHECK	0x10
#define ASC_CNFG1_TEST		0x08
#define ASC_CNFG1_MY_BUS_ID	0x07

/*
 * CCF register
 */

#define ASC_CCF_10MHz		0x2	/* 10 MHz */
#define ASC_CCF_15MHz		0x3	/* 10.01 MHz to 15 MHz */
#define ASC_CCF_20MHz		0x4	/* 15.01 MHz to 20 MHz */
#define ASC_CCF_25MHz		0x5	/* 20.01 MHz to 25 MHz */
#define FAS_CCF_30MHz		0x6	/* 25.01 MHz to 30 MHz */
#define FAS_CCF_35MHz		0x7	/* 30.01 MHz to 35 MHz */
#define FAS_CCF_40MHz		0x0	/* 35.01 MHz to 40 MHz, 8 clocks */

#define	mhz_to_ccf(x)		(((x-1)/5)+1)	/* see specs for limits */

/*
 * Test register
 */

#define ASC_TEST_XXXX		0xf8
#define ASC_TEST_HI_Z		0x04
#define ASC_TEST_I		0x02
#define ASC_TEST_T		0x01

/*
 * Configuration 2
 */

#define ASC_CNFG2_RFB		0x80
#define ASC_CNFG2_EPL		0x40
#define FAS_CNFG2_FEATURES	0x40
#define ASC_CNFG2_EBC		0x20
#define ASC_CNFG2_DREQ_HIZ	0x10
#define ASC_CNFG2_SCSI2		0x08
#define ASC_CNFG2_BPA		0x04
#define ASC_CNFG2_RPE		0x02
#define ASC_CNFG2_DPE		0x01

/*
 * Configuration 3
 */

#define FAS_CNFG3_IDRESCHK	0x80
#define	FAS_CNFG3_QUENB		0x40
#define	FAS_CNFG3_CDB10		0x20
#define	FAS_CNFG3_FASTSCSI	0x10
#define FAS_CNFG3_FASTCLK	0x08
#define ASC_CNFG3_XXXX		0xf8
#define ASC_CNFG3_SRB		0x04
#define ASC_CNFG3_ALT_DMA	0x02
#define ASC_CNFG3_T8		0x01

/*
 * FSC Configuration 4
 */

#define FSC_CNFG4_BBTE		0x01
#define FSC_CNFG4_TEST		0x02
#define FSC_CNFG4_EAN		0x04


struct scsi_curio_dma_ops {
	void		(*dma_init)(int unit);
	void		(*dma_setup_transfer)(int unit, vm_offset_t address,
					    vm_size_t len, boolean_t isread);
	void		(*dma_start_transfer)(int unit, boolean_t isread);
	void		(*dma_end)(int unit, boolean_t isread);
};

typedef struct scsi_curio_dma_ops scsi_curio_dma_ops_t;

#endif	/* _PPC_POWERMAC_SCSI_53C94_ */
