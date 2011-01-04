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
 * File : ns8390.h
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains all NS8390 descriptions used for Network bootstrap.
 */

#ifndef __NS8390_H__
#define __NS8390_H__
#ifdef	DEVICE_NS8390

struct ns8390 {
	i386_ioport_t	ns_iobase;		/* Base address for Board I/O */
	i386_ioport_t	ns_nicbase;		/* Base address for NIC Regs */
	void		*ns_rambase;		/* Base address for RAM */
	u32bits		ns_ramsize;		/* RAM size (in bytes) */
	u8bits		ns_packet;		/* Pointer to next packet */
	u8bits		ns_pstart;		/* Page start */
	u8bits		ns_pstop;		/* Page stop hold */
	u8bits		ns_txt;			/* Page transmit hold */
	u32bits		ns_flags;		/* Board flags */
	void		(*ns_en16bits)(void);	/* enable 16 bits access */
	void		(*ns_dis16bits)(void);	/* disable 16 bits access */
	void		(*ns_reset)(void);	/* reset board */
} ns8390;

#define	NS8390_FLAGS_WD83C690		0x1	/* WD83C690/WD83C790 Chip */
#define	NS8390_FLAGS_BOARD_16BITS	0x2	/* is a 16 bits board */
#define	NS8390_FLAGS_SLOT_16BITS	0x4	/* in a 16 bits slot */
#define	NS8390_FLAGS_MASK_16BITS	0x6	/* Mask for 16 bits move */
#define	NS8390_FLAGS_INTERFACE		0x8	/* (SMC only) has an IF chip */

struct ns8390_input {
	u8bits	ns_status;		/* receive status */
	u8bits	ns_next;		/* next packet pointer */
	u8bits	ns_lbc;			/* receive byte count 0 */
	u8bits	ns_hbc;			/* receive byte count 1 */
};

/*
 * Register I/O addresses (Page 0)
 */
#define	NS8390_CR	0x00	/* Command Register */
#define	NS8390_BNRY	0x03	/* Boundary Pointer */
#define	NS8390_TPSR	0x04	/* Transmit Page Start Register */
#define	NS8390_TBCR0	0x05	/* Transmit Byte Count Register 0 */
#define	NS8390_TBCR1	0x06	/* Transmit Byte Count Register 1 */
#define	NS8390_ISR	0x07	/* Interrupt Status Register */
#define	NS8390_RBCR0	0x0A	/* Remote Byte Count Register 0 */
#define	NS8390_RBCR1	0x0B	/* Remote Byte Count Register 1 */
#define	NS8390_RCR	0x0C	/* Receive Configuration Register */
#define	NS8390_RSR	0x0C	/* Receive Status Register */
#define	NS8390_TCR	0x0D	/* Transmit Configuration Register */
#define	NS8390_DCR	0x0E	/* Data Configuration Register */
#define	NS8390_IMR	0x0F	/* Interrupt Mask Register */
/*
 * Command Register bits (Page 1)
 */
#define	NS8390_PAR	0x1	/* Physical Address Register base */
#define	NS8390_CURR	0x7	/* Current page Register */
#define	NS8390_MAR	0x8	/* Multicast Address Register base */

/*
 * Command Register bits (Page 2)
 */
#define	NS8390_PSTART	0x01	/* Page Start Register */
#define	NS8390_PSTOP	0x02	/* Page Stop Register */

/*
 * Command Register bits
 */
#define	NS8390_STP	0x01	/* Stop: software reset */
#define	NS8390_STA	0x02	/* Start */
#define	NS8390_TXP	0x04	/* Transmit packet */
#define	NS8390_RD1	0x08	/* Remote DMA read */
#define	NS8390_RD2	0x10	/* Remote DMA write */
#define	NS8390_RD3	0x18	/* Remote DMA send */
#define	NS8390_RD4	0x20	/* Remote DMA abort */
#define	NS8390_PS0	0x00	/* Page select 0 */
#define	NS8390_PS1	0x40	/* Page select 1 */
#define	NS8390_PS2	0x80	/* Page select 2 */
#define	NS8390_PS_MASK	0x3F	/* Page select mask */

/*
 * Interrupt Status Register bits.
 */
#define	NS8390_PRX	0x01	/* Packet Received */
#define	NS8390_PTX	0x02	/* Packet Transmitted */
#define	NS8390_RXE	0x04	/* Receive Error */
#define	NS8390_TXE	0x08	/* Transmit Error */
#define	NS8390_OVW	0x10	/* Overwrite Warning */
#define	NS8390_CNT	0x20	/* Counter Overflow */
#define	NS8390_RDC	0x40	/* Remote DMA Complete */
#define	NS8390_RST	0x80	/* Reset Status */

/*
 * Receive Configuration Register.
 */
#define	NS8390_SEP	0x01	/* Save errored packets */
#define	NS8390_AR	0x02	/* Accept runt packets */
#define	NS8390_AB	0x04	/* Accept broadcast */
#define	NS8390_AM	0x08	/* Accept multicast */
#define	NS8390_PRO	0x10	/* Promiscuous physical */
#define	NS8390_MON	0x20	/* Monitor mode */

/*
 * Transmit Configuration Register bits.
 */
#define	NS8390_CRC	0x01	/* Inhibit CRC */
#define	NS8390_LB0	0x00	/* Normal Operation */
#define	NS8390_LB1	0x02	/* Internal Loopback */
#define	NS8390_LB2	0x04	/* External Loopback (LPBK = 1) */
#define	NS8390_LB3	0x06	/* External Loopback (LPBK = 0) */
#define	NS8390_ATD	0x08	/* Auto Transmit Disable */
#define	NS8390_OFST	0x10	/* Collision Offset Enable */

/*
 * Data Configuration Register bits.
 */
#define	NS8390_WTS	0x01	/* Word Transfert Select */
#define	NS8390_BOS	0x02	/* Byte Order Select */
#define	NS8390_LAS	0x04	/* Long Address Select */
#define	NS8390_LS	0x08	/* Loopback Select */
#define	NS8390_AIR	0x10	/* Auto-Initialize Remote */
#define	NS8390_FT0	0x00	/* FIFO Threashold (2 bytes) */
#define	NS8390_FT1	0x20	/* FIFO Threashold (4 bytes) */
#define	NS8390_FT2	0x40	/* FIFO Threashold (8 bytes) */
#define	NS8390_FT3	0x60	/* FIFO Threashold (12 bytes) */

/*
 * Receive Status Register bits.
 */
#define	NS8390_PRX	0x01	/* Packet Received Intact */
#define	NS8390_CRCE	0x02	/* CRC error */
#define	NS8390_FAE	0x04	/* Frame Alignment error */
#define	NS8390_FO	0x08	/* FIFO overrun */
#define	NS8390_MPA	0x10	/* Missed packet */
#define	NS8390_PHY	0x20	/* Physical/Multicast packet */
#define	NS8390_DIS	0x40	/* Receiver disabled */
#define	NS8390_DFR	0x80	/* Deferring */

/*
 *
 */
#define	NS8390_HEADER_SIZE	4	/* Received header size is 4 bytes */
#define	NS8390_PAGE_SIZE	256	/* Page size is 256 bytes */
#define	NS8390_MIN_LENGTH	60	/* Transmit at least 60 bytes */

extern int ns8390_output(void *, unsigned);
extern void ns8390_input(void *, unsigned);

#endif
#endif	/* __NS8390_H__ */
