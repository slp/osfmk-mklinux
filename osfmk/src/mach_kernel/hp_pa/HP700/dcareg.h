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
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
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
 *	@(#)dcareg.h	7.1 (Berkeley) 6/5/86
 */

#include <hp_pa/HP700/iotypes.h>	/* XXX */

struct dcadevice {
	vu_char	dca_reset;
	vu_char dca_pad[0x800-1];
	vu_char	dca_data;			/* receive buf or xmit hold */
	vu_char	dca_ier;			/* interrupt enable */
	vu_char	dca_iir;			/* (RO) interrupt identify */
#define		dca_fifo	dca_iir		/* (WO) FIFO control */
	vu_char	dca_cfcr;			/* line control */
	vu_char	dca_mcr;			/* modem control */
	vu_char	dca_lsr;			/* line status */
	vu_char	dca_msr;			/* modem status */
	vu_char	dca_scr;			/* scratch pad */
};

/* interface reset/id (300 only) */
#define	DCAID0		0x02
#define DCAREMID0	0x82
#define	DCAID1		0x42
#define DCAREMID1	0xC2

/* interrupt control (300 only) */
#define	DCAIPL(x)	((((x) >> 4) & 3) + 3)
#define	IC_IR		0x40
#define	IC_IE		0x80

/*
 * 16 bit baud rate divisor (lower byte in dca_data, upper in dca_ier)
 * NB: This constant is for a 7.3728 clock frequency. The 300 clock
 *     frequency is 2.4576, giving a constant of 153600.
 */
#define	DCABRD(x)	(460800 / (x))

/* interrupt enable register */
#define	IER_ERXRDY	0x1
#define	IER_ETXRDY	0x2
#define	IER_ERLS	0x4
#define	IER_EMSC	0x8

/* interrupt identification register */
#define	IIR_IMASK	0xf
#define	IIR_RXTOUT	0xc
#define	IIR_RLS		0x6
#define	IIR_RXRDY	0x4
#define	IIR_TXRDY	0x2
#define	IIR_NOPEND	0x1
#define	IIR_MLSC	0x0
#define	IIR_FIFO_MASK	0xc0	/* set if FIFOs are enabled */

/* fifo control register */
#define	FIFO_ENABLE	0x01
#define	FIFO_RCV_RST	0x02
#define	FIFO_XMT_RST	0x04
#define	FIFO_DMA_MODE	0x08
#define	FIFO_TRIGGER_1	0x00
#define	FIFO_TRIGGER_4	0x40
#define	FIFO_TRIGGER_8	0x80
#define	FIFO_TRIGGER_14	0xc0

/* character format control register */
#define	CFCR_DLAB	0x80
#define	CFCR_SBREAK	0x40
#define	CFCR_PZERO	0x30
#define	CFCR_PONE	0x20
#define	CFCR_PEVEN	0x10
#define	CFCR_PODD	0x00
#define	CFCR_PENAB	0x08
#define	CFCR_STOPB	0x04
#define	CFCR_8BITS	0x03
#define	CFCR_7BITS	0x02
#define	CFCR_6BITS	0x01
#define	CFCR_5BITS	0x00

/* modem control register */
#define	MCR_LOOPBACK	0x10
#define	MCR_IEN		0x08
#define	MCR_DRS		0x04
#define	MCR_RTS		0x02
#define	MCR_DTR		0x01

/* line status register */
#define	LSR_RCV_FIFO	0x80
#define	LSR_TSRE	0x40
#define	LSR_TXRDY	0x20
#define	LSR_BI		0x10
#define	LSR_FE		0x08
#define	LSR_PE		0x04
#define	LSR_OE		0x02
#define	LSR_RXRDY	0x01
#define	LSR_RCV_MASK	0x1f

/* modem status register */
#define	MSR_DCD		0x80
#define	MSR_RI		0x40
#define	MSR_DSR		0x20
#define	MSR_CTS		0x10
#define	MSR_DDCD	0x08
#define	MSR_TERI	0x04
#define	MSR_DDSR	0x02
#define	MSR_DCTS	0x01

/*
 * Prototypes
 */
extern void dcaprobe(struct hp_device*);
extern void dcastart(struct tty *);
extern void dcastop(struct tty *, int);
extern int dcaopen(dev_t, dev_mode_t, io_req_t);
extern void dcaclose(dev_t, int);
extern io_return_t dcaread(dev_t, io_req_t);
extern io_return_t dcawrite(dev_t, io_req_t);
extern int dcamap(dev_t, int, int);
extern io_return_t dcagetstat(dev_t, dev_flavor_t, int *, natural_t *);
extern io_return_t dcasetstat(dev_t, dev_flavor_t, int *, natural_t);
extern boolean_t dcaportdeath(dev_t, ipc_port_t);
extern int dcamctl(struct tty *, int, int);


