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
 *  (c) Copyright 1992 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */
/*
 * MkLinux
 */
/* CMU_HIST */
/*
 * Revision 2.1  91/08/28  12:01:52  af
 * Created.
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
 */

#include <machine/endian.h>
#include <hp_pa/HP700/device.h>

#define pmap_flush_range(p, v, s)	\
{ \
	space_t sid = (p)->space; \
	if ((((int)(v) & 31) + (s)) <= 32) \
		fcacheline(sid, (vm_offset_t)(v)); \
	else if (sid == HP700_SID_KERNEL) \
		fdcache(sid, v, s); \
	else \
		panic("pmap_flush_range: not kernel"); \
}
#define pmap_purge_range(p, v, s)	\
{ \
	space_t sid = (p)->space; \
	if ((((int)(v) & 31) + (s)) <= 32) \
		pcacheline(sid, (vm_offset_t)(v)); \
	else if (sid == HP700_SID_KERNEL) \
		pdcache(sid, v, s); \
	else \
		panic("pmap_purge_range: not kernel"); \
}

extern	 int	ncr53c700_probe(
				hp_ctlrinfo_t *);

/* controller types */

#define CTYPE_700	0
#define CTYPE_710	1
#define CTYPE_720	2


/*
 * NCR53C700 series hardware registers
 */

union csr {

    struct {				
        volatile unsigned char	pad0[0x34];	/* 0x00: pad */
	volatile unsigned char	dmode;		/* 0x34: dma mode */
	volatile unsigned char	pad1[0x40-0x35];/* 0x35: pad */
    } c700;

    struct {					
	volatile unsigned char	pad0[0x10];	/* 0x00: pad */
	volatile unsigned long	dsa;		/* 0x10: data structure 
						   address (c710) */
	volatile unsigned char	pad1[0x22-0x14];/* 0x14: pad */
	volatile unsigned char	ctest8;		/* 0x22: chip test 8 (c710) */
	volatile unsigned char	lcrc;		/* 0x23: longitudinal parity */
	volatile unsigned char	pad2[0x34-0x24];/* 0x24: pad */
	volatile unsigned long	scratch;	/* 0x34: scratch pad (c710) */
	volatile unsigned char	dmode;		/* 0x38: dma mode (c710) */
	volatile unsigned char	pad3[0x3c-0x39];/* 0x39: pad */
	volatile unsigned long	adder;		/* 0x3c: adder (c710) */
    } c710;

    struct {
	volatile unsigned char	pad0[0x02];
	volatile unsigned char	scntl2;		/* 0x02: scsi control 2 */
	volatile unsigned char	scntl3;		/* 0x03: scsi control 3 */
	volatile unsigned char	pad1[0x02];
	volatile unsigned char	sdid;		/* 0x06: scsi destination id */
	volatile unsigned char	gpreg;		/* 0x07: general purpose */
	volatile unsigned char	pad2[0x01];
	volatile unsigned char	socl;		/* 0x09: scsi output control
						   latch */
	volatile unsigned char	ssid;		/* 0x0a: scsi selector id */
	volatile unsigned char	pad3[0x05];
	volatile unsigned long	dsa;		/* 0x10: data structure 
						   address */
	volatile unsigned char	istat;		/* 0x14: interrupt status */
	volatile unsigned char	pad4[0x03];
	volatile unsigned char	ctest0;		/* 0x18: chip test 0 */
	volatile unsigned char	ctest1;		/* 0x19: chip test 1 */
	volatile unsigned char	ctest2;		/* 0x1a: chip test 2 */
	volatile unsigned char	ctest3;		/* 0x1b: chip test 3 */
	volatile unsigned char	pad5[0x05];
	volatile unsigned char	ctest4;		/* 0x21: chip test 4 */
	volatile unsigned char	ctest5;		/* 0x22: chip test 5 */
	volatile unsigned char	ctest6;		/* 0x23: chip test 6 */
	volatile unsigned char	pad6[0x14];
	volatile unsigned char	dmode;		/* 0x38: dma mode */
	volatile unsigned char	pad7[0x07];
	volatile unsigned char	sien0;		/* 0x40: scsi interrupt
						   enable 0 */
	volatile unsigned char	sien1;		/* 0x41: scsi interrupt
						   enable 1 */
	volatile unsigned char	sist0;		/* 0x42: scsi interrupt
						   status 0 */
	volatile unsigned char	sist1;		/* 0x43: scsi interrupt
						   status 1 */
	volatile unsigned char	slpar;		/* 0x44: scsi longitudinal
						   parity */
	volatile unsigned char	swide;		/* 0x45: scsi longitudinal
						   parity */
	volatile unsigned char	macntl;		/* 0x46: memory access
						   control */
	volatile unsigned char	gpcntl;		/* 0x47: general purpose
						   control */
	volatile unsigned char	stime0;		/* 0x48: scsi timer 0 */
	volatile unsigned char	stime1;		/* 0x49: scsi timer 1 */
	volatile unsigned char	respid0;	/* 0x4a: response id 0 */
	volatile unsigned char	respid1;	/* 0x4b: response id 1 */
	volatile unsigned char	stest0;		/* 0x4c: scsi test 0 */
	volatile unsigned char	stest1;		/* 0x4d: scsi test 1 */
	volatile unsigned char	stest2;		/* 0x4e: scsi test 2 */
	volatile unsigned char	stest3;		/* 0x4f: scsi test 3 */
	volatile unsigned short	sidl;		/* 0x50: scsi input data
						   latch */
	volatile unsigned char	pad8[0x02];
	volatile unsigned short	sodl;		/* 0x54: scsi output data
						   latch */
	volatile unsigned char	pad9[0x02];
	volatile unsigned short	sbdl;		/* 0x58: scsi bus data lines */
	volatile unsigned char	pada[0x06];
    } c720;

    /* common csr definition */

    struct {					
	volatile unsigned char	scntl0;		/* 0x00: scsi control 0 */
	volatile unsigned char	scntl1;		/* 0x01: scsi control 1 */
	volatile unsigned char	sdid;		/* 0x02: scsi destination id */
	volatile unsigned char	sien;		/* 0x03: scsi interrupt
						   enable */
	volatile unsigned char	scid;		/* 0x04: scsi chip id */
	volatile unsigned char	sxfer;		/* 0x05: scsi transfer */
	volatile unsigned char	sodl;		/* 0x06: scsi output data
						   latch */
	volatile unsigned char	socl;		/* 0x07: scsi output control
						   latch */
	volatile unsigned char	sfbr;		/* 0x08: scsi first byte
						   received */
	volatile unsigned char	sidl;		/* 0x09: scsi input data
						   latch */
	volatile unsigned char	sbdl;		/* 0x0a: scsi bus data lines */
	volatile unsigned char	sbcl;		/* 0x0b: scsi bus control
						   lines */
	volatile unsigned char	dstat;		/* 0x0c: dma status */
	volatile unsigned char	sstat0;		/* 0x0d: scsi status 0 */
	volatile unsigned char	sstat1;		/* 0x0e: scsi status 1 */
	volatile unsigned char	sstat2;		/* 0x0f: scsi status 2 */
	volatile unsigned char	res0[4];	/* 0x10: reserved */
	volatile unsigned char	ctest0;		/* 0x14: chip test 0 */
	volatile unsigned char	ctest1;		/* 0x15: chip test 1 */
	volatile unsigned char	ctest2;		/* 0x16: chip test 2 */
	volatile unsigned char	ctest3;		/* 0x17: chip test 3 */
	volatile unsigned char	ctest4;		/* 0x18: chip test 4 */
	volatile unsigned char	ctest5;		/* 0x19: chip test 5 */
	volatile unsigned char	ctest6;		/* 0x1a: chip test 6 */
	volatile unsigned char	ctest7;		/* 0x1b: chip test 7 */
	volatile unsigned long	temp;		/* 0x1c: temporary stack */
	volatile unsigned char	dfifo;		/* 0x20: dma fifo */
	volatile unsigned char	istat;		/* 0x21: interrupt status */
	volatile unsigned char	res1[2];	/* 0x22: reserved */
	    union {
	    	volatile unsigned long	dbc;	/* 0x24-0x027: dma byte
						   count */
		struct {
		    volatile unsigned char	padd[3];
		    volatile unsigned char	dcmd;	/* 0x27: dma command */
		} d;
	    } d;
	volatile unsigned long	dnad;		/* 0x28: dma address or
						   next inst address */
	volatile unsigned long	dsp;		/* 0x2c: dma scripts pointer */
	volatile unsigned long	dsps;		/* 0x30: dma scripts pointer
						   save */
	volatile unsigned char	res2[5];	/* 0x34: reserved */
	volatile unsigned char	dien;		/* 0x39: dma interrupt
						   enable */
	volatile unsigned char	dwt;		/* 0x3a: dma watchdog 
						   timeout */
	volatile unsigned char	dcntl;		/* 0x3b: dma control */
	volatile unsigned char	res3[4];	/* 0x3c: reserved */
    } rw;

    struct {
	volatile unsigned char	pad[0xc80];
	volatile unsigned long	id;		/* 0xc80: standard eisa
						   card id */
	volatile unsigned char	control;	/* 0xc84: standard eisa
						   control */
	volatile unsigned char	rsrc;		/* 0xc85: resource register */
    } eisa;
};


/* scntl0 */

#define C700_SCNTL0_TRG         0x01	/* chip is a target device */
#define C700_SCNTL0_AAP         0x02    /* assert ATN/ on parity error */
#define C700_SCNTL0_EPG         0x04    /* enable parity generation */
#define C700_SCNTL0_EPC         0x08    /* enable parity checking */
#define C700_SCNTL0_WATN        0x10    /* select with ATN/ on a start
					   sequence */
#define C700_SCNTL0_START       0x20    /* start sequence */
#define C700_SCNTL0_ARB0        0x40    /* arbitration */
#define C700_SCNTL0_ARB1        0x80    /* arbitration */

#define C700_SCNTL0_SMPLARB     (0)
#define C700_SCNTL0_FULLARB     (C700_SCNTL0_ARB1|C700_SCNTL0_ARB0)

/* scntl1 */

#define C700_SCNTL1_RCV         0x01    /* start scsi rcv operation */
#define C700_SCNTL1_SND         0x02    /* start scsi send operation */
#define C700_SCNTL1_PAR         0x04    /* assert even parity */
#define C700_SCNTL1_RST         0x08    /* assert rst/ */
#define C700_SCNTL1_CON         0x10    /* connected */
#define C700_SCNTL1_ESR         0x20    /* enable reselection */
#define C700_SCNTL1_ADB         0x40    /* assert contents of sodl reg
					   onto data bus */
#define C700_SCNTL1_EXC         0x80    /* extra clock cycle of data setup */

/* scntl3 */

#define C720_SCNTL3_CCF0	0x01	/* clock conversion factor */
#define C720_SCNTL3_CCF1	0x02	/* clock conversion factor */
#define C720_SCNTL3_CCF2	0x04	/* clock conversion factor */
#define C720_SCNTL3_EWS		0x08	/* enable wide SCSI */
#define C720_SCNTL3_SCF0	0x10	/* sync clock convertion factor */
#define C720_SCNTL3_SCF1	0x20	/* sync clock convertion factor */
#define C720_SCNTL3_SCF2	0x40	/* sync clock convertion factor */
#define C720_SCNTL3_CFDIV3	0x00	/* 50.01-66 Mhz */
#define C720_SCNTL3_CFDIV1	0x01	/* 16.67-25 Mhz */
#define C720_SCNTL3_CFDIV15	0x02	/* 25.01-37.5 Mhz */
#define C720_SCNTL3_CFDIV2	0x03	/* 37.51-50 Mhz */

/* sien */

#define C700_SIEN_PAR           0x01    /* enable parity error interrupt */
#define C700_SIEN_RST           0x02    /* enable RST/ interrupt interrupt */
#define C700_SIEN_UDC           0x04    /* enable unexpected disconnect
					   interrupt */
#define C700_SIEN_SGE           0x08    /* enable gross error interrupt */
#define C700_SIEN_SEL           0x10    /* enable selection or reselection
					   interrupt */
#define C700_SIEN_STO           0x20    /* enable selection timeout
					   interrupt */
#define C700_SIEN_FCMP          0x40    /* enable function complete
					   interrupt */
#define C700_SIEN_MA            0x80    /* enable phase mismatch interrupt */
#define C720_SIEN_RSL           0x10    /* enable reselection interrupt */
#define C720_SIEN_SEL           0x20    /* enable selection interrupt */

#define C720_SIEN_HTH           0x01    /* handshake timeout interrupt */
#define C720_SIEN_GEN           0x02    /* general purpose timeout interrupt */
#define C720_SIEN_STO           0x04    /* enable selection timeout
					   interrupt (sien1) */

/* scid */

#define C720_SCID_ID0		0x01	/* chip SCSI ID */
#define C720_SCID_ID1		0x02	/* chip SCSI ID */
#define C720_SCID_ID2		0x04	/* chip SCSI ID */
#define C720_SCID_ID3		0x08	/* chip SCSI ID */
#define C720_SCID_SRE		0x20	/* enable response to selection */
#define C720_SCID_RRE		0x40	/* enable response to reselection */

/* sxfer */

#define C700_SXFER_MO0          0x01    /* maximum synchronous offset bit 0 */
#define C700_SXFER_MO1          0x02    /* maximum synchronous offset bit 1 */
#define C700_SXFER_MO2          0x04    /* maximum synchronous offset bit 2 */
#define C700_SXFER_MO3          0x08    /* maximum synchronous offset bit 3 */
#define C700_SXFER_TP0          0x10    /* synchronous xfer period bit 0 */
#define C700_SXFER_TP1          0x20    /* synchronous xfer period bit 1 */
#define C700_SXFER_TP2          0x40    /* synchronous xfer period bit 2 */
#define C700_SXFER_DHP          0x80    /* disable halt on parity error
					   or atn/ (target mode only) */

/* macros for controlling sync transfers */

#define SET_SXFER(cdata, csr, tdata) {				\
	if ((cdata)->ctype == CTYPE_720) {			\
		WRITEBYTEREG(csr->c720.scntl3, (tdata)->sbcl);	\
	} else {						\
		WRITEBYTEREG(csr->rw.sbcl, (tdata)->sbcl);	\
	}							\
	WRITEBYTEREG(csr->rw.sxfer, (tdata)->sxfer);		\
}

/* socl */

#define C700_SOCL_IO            0x01	/* assert SCSI I/O  signal */
#define C700_SOCL_CD            0x02    /* assert SCSI C/D  signal */
#define C700_SOCL_MSG           0x04    /* assert SCSI MSG/ signal */
#define C700_SOCL_ATN           0x08    /* assert SCSI ATN/ signal */
#define C700_SOCL_SEL           0x10    /* assert SCSI SEL/ signal */
#define C700_SOCL_BSY           0x20    /* assert SCSI BSY/ signal */
#define C700_SOCL_ACK           0x40    /* assert SCSI ACK/ signal */
#define C700_SOCL_REQ           0x80    /* assert SCSI REQ/ signal */

/* sbcl (on write for 700/710) */

#define C700_SOCL_SSCF0         0x01    /* synchronous SCSI clock frequency
					   bit 0 */
#define C700_SOCL_SSCF1         0x02    /* synchronous SCSI clock frequency
					   bit 1 */
#define C700_SBCL_DIV_DCNTL	0x00
#define C710_SBCL_DIV1		0x01

/* dstat */

#define C700_DSTAT_IID          0x01    /* illegal instruction detected */
#define C700_DSTAT_WTD          0x02    /* watchdog timeout interrupt */
#define C700_DSTAT_SIR          0x04    /* script interrupt instruction
					   received interrupt */
#define C700_DSTAT_SPI          0x08    /* script pipeline/single-step
					   interrupt */
#define C700_DSTAT_ABRT         0x10    /* aborted interrupt */
#define C700_DSTAT_BF           0x20    /* bus fault */
#define C700_DSTAT_RES6         0x40    /* reserved */
#define C700_DSTAT_DFE          0x80    /* dma fifo empty interrupt */

/* sstat0 (sist0/1 on 720) */

#define C700_SSTAT0_PAR         0x01    /* parity error */
#define C700_SSTAT0_RST         0x02    /* rst/ received */
#define C700_SSTAT0_UDC         0x04    /* unexpected disconnect */
#define C700_SSTAT0_SGE         0x08    /* gross error */
#define C700_SSTAT0_SEL         0x10    /* siop select or reselected */
#define C700_SSTAT0_STO         0x20    /* selection timeout */
#define C700_SSTAT0_FCMP        0x40    /* function complete (arbitration,
					   selection only or
					   full arbitration) */
#define C700_SSTAT0_MA          0x80    /* phase mismatch */

/* sstat1 (sstat0 on 720) */

#define C700_SSTAT1_SDP         0x01    /* SDP/PARITY signal*/
#define C700_SSTAT1_RST         0x02    /* RST/ signal */
#define C700_SSTAT1_WOA         0x04    /* won arbitration */
#define C700_SSTAT1_LOA         0x08    /* lost arbitration */
#define C700_SSTAT1_AIP         0x10    /* arbitration in progress */
#define C700_SSTAT1_OLF         0x20    /* sodl reg full */
#define C700_SSTAT1_ORF         0x40    /* sodr reg full */
#define C700_SSTAT1_ILF         0x80    /* sidl reg full */

/* sstat2 (sstat1 on 720) */

#define C700_SSTAT2_IO          0x01    /* scsi bus phase bit 0 */
#define C700_SSTAT2_CD          0x02    /* scsi bus phase bit 1 */
#define C700_SSTAT2_MSG         0x04    /* scsi bus phase bit 2 */
#define C700_SSTAT2_SDP         0x08    /* latched sdp/scsi parity */
#define C700_SSTAT2_FF0         0x10    /* scsi sync fifo bytes */
#define C700_SSTAT2_FF1         0x20    /* scsi sync fifo bytes */
#define C700_SSTAT2_FF2         0x40    /* scsi sync fifo bytes */
#define C700_SSTAT2_FF3         0x80    /* scsi sync fifo bytes */

#define C700_SSTAT_BP(b)       ((b)&0x07)	/* phase bits */

#define C700_GETBP(cdata, csr)					\
	C700_SSTAT_BP((cdata)->ctype==CTYPE_720 ?		\
		      READBYTEREG((csr)->rw.sstat1):		\
		      READBYTEREG((csr)->rw.sstat2))

/* sstat2 (720) */

#define C720_SSTAT2_SDP1        0x01    /* SDP1/PARITY signal */
#define C720_SSTAT2_LDSC        0x02    /* last disconnect */
#define C720_SSTAT2_SPL1        0x08    /* latched SCSI parity for SD15-8 */
#define C720_SSTAT2_OLF1        0x20    /* sodl reg MSB full */
#define C720_SSTAT2_ORF1        0x40    /* sodr reg MSB full */
#define C720_SSTAT2_ILF1        0x80    /* sidl reg MSB full */

/* ctest0 */

#define C700_CTEST0_DDIR        0x01    /* data direction read (0),
					   write (1) */
#define C700_CTEST0_RES1        0x02    /* reserved */
#define C700_CTEST0_RES2        0x04    /* reserved */
#define C700_CTEST0_RES3        0x08    /* reserved */
#define C700_CTEST0_RES4        0x10    /* reserved */
#define C700_CTEST0_RES5        0x20    /* reserved */
#define C700_CTEST0_RES6        0x40    /* reserved */
#define C700_CTEST0_RES7        0x80    /* reserved */

/* ctest7 */

#define C710_CTEST7_DIFF        0x01    /* enable diff mode */
#define C710_CTEST7_TT1         0x02    /* transfer type bit */
#define C710_CTEST7_EVP         0x04    /* even parity */
#define C710_CTEST7_DFP         0x08    /* DMA fifo parity */
#define C710_CTEST7_NOTIME      0x10    /* selection timeout disable */
#define C710_CTEST7_SC0         0x20    /* snoop control bits */
#define C710_CTEST7_SC1         0x40    /* snoop control bits */
#define C710_CTEST7_CDIS        0x80    /* cache burst disable */

/* dfifo */

#define C700_DFIFO_BO0          0x01    /* fifo byte offset counter bit 0 */
#define C700_DFIFO_BO1          0x02    /* fifo byte offset counter bit 1 */
#define C700_DFIFO_BO2          0x04    /* fifo byte offset counter bit 2 */
#define C700_DFIFO_BO3          0x08    /* fifo byte offset counter bit 3 */
#define C700_DFIFO_BO4          0x10    /* fifo byte offset counter bit 4 */
#define C700_DFIFO_BO5          0x20    /* fifo byte offset counter bit 5 */

#define C700_DFIFO_CLF          0x40    /* clear dma fifo */
#define C710_DFIFO_BO6          0x40    /* fifo byte offset counter bit 6 */

#define C700_DFIFO_FLF          0x80    /* flush dma fifo */
#define C710_DFIFO_RES7         0x80    /* reserved */

#define C700_DFIFO_BO_MASK      0x3f    /* mask to access fifo byte offset
					   bits */
#define C710_DFIFO_BO_MASK      0x7f    /* mask to access fifo byte offset
					   bits */

/* istat */

#define C700_ISTAT_DIP          0x01    /* dma  interrupt pending, look
					   in dstat  */
#define C700_ISTAT_SIP          0x02    /* scsi interrupt pending, look
					   in sstat0 */
#define C700_ISTAT_PRE          0x04    /* pointer register empty */
#define C700_ISTAT_CON          0x08    /* connected to scsi bus */
#define C700_ISTAT_RES4         0x10    /* reserved */
#define C710_ISTAT_SIGP         0x20    /* signal script */
#define C710_ISTAT_RST          0x40    /* software reset */
#define C700_ISTAT_ABRT         0x80    /* abort operation */

/* ctest0 */

#define C710_CTEST0_DDIR        0x01    /* data transfer direction */
#define C710_CTEST0_RES         0x02    /* reserved */
#define C710_CTEST0_ERF         0x04    /* extend REQ/ACK filtering */
#define C710_CTEST0_HSG         0x08    /* halt scsi clock */
#define C710_CTEST0_EAN         0x10    /* enable active negation */
#define C710_CTEST0_GRP         0x20    /* generate receive parity */
#define C710_CTEST0_BTD         0x40    /* byte-to-byte timer disable */
#define C710_CTEST0_RES7        0x80    /* reserved */

#define C720_CTEST0_EHP         0x04    /* enable host parity */

/* ctest3 */

#define C720_CTEST3_SM          0x01    /* snoop pin mode */
#define C720_CTEST3_FM          0x02    /* fetch pin mode */
#define C720_CTEST3_CLF         0x04    /* clear fifos */
#define C720_CTEST3_FLF         0x08    /* flush fifos */
#define C720_CTEST3_V0          0x10    /* chip rev bit 0 */
#define C720_CTEST3_V1          0x20    /* chip rev bit 1 */
#define C720_CTEST3_V2          0x40    /* chip rev bit 2 */
#define C720_CTEST3_V3          0x80    /* chip rev bit 3 */

/* ctest4 */

#define C720_CTEST4_MUX         0x80    /* host bus multiplex mode */
#define C720_CTEST4_EHPC        0x08    /* enable host parity check */

/* ctest8 */

#define C710_CTEST8_SM          0x01    /* snoop pin mode */
#define C710_CTEST8_FM          0x02    /* fetch pin mode */
#define C710_CTEST8_CLF         0x04    /* clear fifos */
#define C710_CTEST8_FLF         0x08    /* flush fifos */
#define C710_CTEST8_V0          0x10    /* chip rev bit 0 */
#define C710_CTEST8_V1          0x20    /* chip rev bit 1 */
#define C710_CTEST8_V2          0x40    /* chip rev bit 2 */
#define C710_CTEST8_V3          0x80    /* chip rev bit 3 */

#define C700_REV(x)		(((x)>>4)&0xf)
#define C700_CLF(cdata, csr)					\
	if (cdata->ctype == CTYPE_700) { 			\
		SETBYTEREG(csr->rw.dfifo, C700_DFIFO_CLF);	\
	} else if (cdata->ctype == CTYPE_710) {			\
		SETBYTEREG(csr->c710.ctest8, C710_CTEST8_CLF);	\
	} else {						\
		SETBYTEREG(csr->c720.ctest3, C720_CTEST3_CLF);	\
	}

/* dcmd */

#define C700_DCMD_BP(b)         ((b)&0x07)	/* phase bits */

/* dmode */

#define C700_DMODE_MAN          0x01	/* manual start mode (1) */
#define C700_DMODE_PIPE         0x02    /* pipeline mode (1) */
#define C700_DMODE_FAM          0x04    /* fixed address mode (used
					   for padded xfers) */
#define C700_DMODE_IOM          0x08    /* i/o (1) or memory mapped mode (0) */
#define C700_DMODE_286          0x10    /* 286 mode (1) or 386 mode (0) */
#define C700_DMODE_BW16         0x20    /* bus width 16 bits (1) else
					   32 bits (0) */
#define C700_DMODE_BL0          0x40    /* burst length */
#define C700_DMODE_BL1          0x80    /* burst length */

/* tranfer bursts */

#define C700_DMODE_B1           (0)			/* 1 transfer burst */
#define C700_DMODE_B2           (C700_DMODE_BL0)	/* 2 transfer burst */
#define C700_DMODE_B4           (C700_DMODE_BL1)	/* 4 transfer burst */
#define C700_DMODE_B8           (C700_DMODE_BL0|C700_DMODE_BL1)
							/* 8 transfer burst */
#define C720_DMODE_B2           (0)                     /* 2 transfer burst */
#define C720_DMODE_B4           (C700_DMODE_BL0)        /* 4 transfer burst */
#define C720_DMODE_B8           (C700_DMODE_BL1)        /* 8 transfer burst */
#define C720_DMODE_B16          (C700_DMODE_BL0|C700_DMODE_BL1)
							/* 16 transfer burst */

/* dien */

#define C700_DIEN_IID           0x01	/* enable illegal intruction
					   detected interrupt */
#define C700_DIEN_WTD           0x02    /* enable watchdog timeout interrupt */
#define C700_DIEN_SIR           0x04    /* enable script interrupt
					   instruction received interrupt */
#define C700_DIEN_SPI           0x08    /* enable script pipeline interrupt */
#define C700_DIEN_ABRT          0x10    /* enable aborted interrupt */
#define C700_DIEN_BF            0x20    /* enable bus fault interrupt */
#define C700_DIEN_RES6          0x40    /* reserved */
#define C700_DIEN_RES7          0x80    /* reserved */

/* dcntl */

#define C700_DCNTL_RST          0x01    /* software reset of siop */
#define C710_DCNTL_COM          0x01    /* c700 compatibility 0 => c700 like */

#define C700_DCNTL_RES1         0x02    /* reserved */
#define C700_DCNTL_STD          0x04    /* start dma operation */
#define C700_DCNTL_LLM          0x08    /* low-level mode */
#define C700_DCNTL_SSM          0x10    /* single step mode */
#define C700_DCNTL_S16          0x20    /* scripts loaded in 16 bit mode */
#define C720_DCNTL_EA           0x20    /* enable ack */
#define C700_DCNTL_CF0          0x40    /* clock freq */
#define C700_DCNTL_CF1          0x80    /* clock freq */

#define C700_DCNTL_CFDIV2       0x00    /* 37.51-50 mhz */
#define C700_DCNTL_CFDIV15      0x40    /* 25.01-37.50 mhz */
#define C700_DCNTL_CFDIV1       0x80    /* 16.67-25.00 mhz */
#define C700_DCNTL_CFDIV3       0xc0    /* 50.01-66 mhz */

#define C720_STIME0_SEL0	0x01	/* selection timeout */
#define C720_STIME0_SEL1	0x02	/* selection timeout */
#define C720_STIME0_SEL2	0x04	/* selection timeout */
#define C720_STIME0_SEL3	0x08	/* selection timeout */
#define C720_STIME0_SEL204	0x0C	/* STO: 204.8 ms */
#define C720_STIME0_HTH1_6	0xC0	/* HTH: 1.6 s */

#define C720_RESPID0_ID0	0x01	/* select/reselect ID */
#define C720_RESPID0_ID1	0x02	/* select/reselect ID */
#define C720_RESPID0_ID2	0x04	/* select/reselect ID */
#define C720_RESPID0_ID3	0x08	/* select/reselect ID */
#define C720_RESPID0_ID4	0x10	/* select/reselect ID */
#define C720_RESPID0_ID5	0x20	/* select/reselect ID */
#define C720_RESPID0_ID6	0x40	/* select/reselect ID */
#define C720_RESPID0_ID7	0x80	/* select/reselect ID */
#define C720_RESPID1_ID8	0x01	/* select/reselect ID */
#define C720_RESPID1_ID9	0x02	/* select/reselect ID */
#define C720_RESPID1_ID10	0x04	/* select/reselect ID */
#define C720_RESPID1_ID11	0x08	/* select/reselect ID */
#define C720_RESPID1_ID12	0x10	/* select/reselect ID */
#define C720_RESPID1_ID13	0x20	/* select/reselect ID */
#define C720_RESPID1_ID14	0x40	/* select/reselect ID */
#define C720_RESPID1_ID15	0x80	/* select/reselect ID */

#define C720_STEST0_SOM		0x01	/* SCSI sync offset max */
#define C720_STEST0_SOZ		0x02	/* SCSI sync offset zero */
#define C720_STEST0_ART		0x04	/* arbitration piroity encoder test */
#define C720_STEST0_SLT		0x08	/* selection response logic test */

#define C720_STEST2_LOW		0x01	/* SCSI low level mode */
#define C720_STEST2_EXT		0x02	/* extend REQ/ACK filtering */
#define C720_STEST2_AWS		0x04	/* always wide SCSI */
#define C720_STEST2_SZM		0x08	/* SCSI high-impedance mode */
#define C720_STEST2_SLB		0x10	/* SCSI loopback mode */
#define C720_STEST2_DIF		0x20	/* SCSI differential mode */
#define C720_STEST2_ROF		0x40	/* reset SCSI offset */
#define C720_STEST2_SCE		0x80	/* SCSI control enable */

#define C720_STEST3_STW		0x01	/* SCSI FIFO test write */
#define C720_STEST3_CSF		0x02	/* clear SCSI FIFO */
#define C720_STEST3_TTM		0x04	/* timer test mode */
#define C720_STEST3_S16		0x08	/* 16-bit systems */
#define C720_STEST3_DSI		0x10	/* disable single initiator */
#define C720_STEST3_HSC		0x20	/* halt SCSI clock */
#define C720_STEST3_STR		0x40	/* SCSI FIFO test read */
#define C720_STEST3_TE		0x80	/* TolerANT enable (active negation) */

#define C700_BUS_RESET_DELAY	100

/*
 * Deal with byte swapping on some of the controllers.
 *
 * XXX note implicit use of "cdata" in BIG_END.  I didn't want to change
 * lots of macros to pass around a cdata pointer so I cheat knowing that
 * cdata is defined and initialized everywhere we use these.
 */

#define BIG_END		(cdata->bigend)

#define BYTESWAP(x)	(BIG_END ? (x) : byte_reverse_word(x))
#define SWAPINST(ip) \
	((ip)->l[0] = BYTESWAP((ip)->l[0]), (ip)->l[1] = BYTESWAP((ip)->l[1]))

#define READBYTEREG(r) \
	*((BIG_END) ? (u_char *)((int)&(r) ^ 3) : &(r))
#define WRITEBYTEREG(r, v) \
	*((BIG_END) ? (u_char *)((int)&(r) ^ 3) : &(r)) = (v)
#define SETBYTEREG(r, b) \
	*((BIG_END) ? (u_char *)((int)&(r) ^ 3) : &(r)) |= (b)
#define CLRBYTEREG(r, b) \
	*((BIG_END) ? (u_char *)((int)&(r) ^ 3) : &(r)) &= ~(b)


#define C700_MAXPHYS		(128 * 1024)
#define C700_MAX_DMA_INST       ((C700_MAXPHYS+NBPG-1)/NBPG + 1) 
				/* 1 extra for jump back to script */



