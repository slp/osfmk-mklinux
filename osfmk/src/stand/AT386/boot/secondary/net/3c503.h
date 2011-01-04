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
 * File : 3c503.h
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains the descriptions of the 3c503 used for Network bootstrap.
 */

#ifndef __3C503_H__
#define __3C503_H__
#ifdef	DEVICE_3C503
/*
 * 3C503 Gate Array registers
 */
#define	_3C503_GA_PSTR		0x400	/* Page Start */
#define	_3C503_GA_PSPR		0x401	/* Page Stop */
#define	_3C503_GA_DQTR		0x402	/* DRQ Timer */
#define	_3C503_GA_BCFR		0x403	/* Base Configuration */
#define	_3C503_GA_PCFR		0x404	/* EPROM Configuration */
#define	_3C503_GA_GACFR		0x405	/* Gate Array Configuration */
#define	_3C503_GA_CR		0x406	/* Control */
#define	_3C503_GA_STREG		0x407	/* Status */
#define	_3C503_GA_IDCFR		0x408	/* Interrupt/DMA Configuration */
#define	_3C503_GA_DAMSB		0x409	/* DMA MSB Address */
#define	_3C503_GA_DALSB		0x40A	/* DMA LSB Address */
#define	_3C503_GA_VPTR2		0x40B	/* Vector Pointer 2 */
#define	_3C503_GA_VPTR1		0x40C	/* Vector Pointer 1 */
#define	_3C503_GA_VPTR0		0x40D	/* Vector Pointer 0 */
#define	_3C503_GA_RFMSB		0x40E	/* Register File Access MSB */
#define	_3C503_GA_RFLSB		0x40F	/* Register File Access LSB */


/*
 * 3C503 Gate Array Configuration Register.
 */
#define	_3C503_GACFR_MSB0	0x01	/* Memory Bank Select 0 */
#define	_3C503_GACFR_MSB1	0x02	/* Memory Bank Select 1 */
#define	_3C503_GACFR_MSB2	0x04	/* Memory Bank Select 2 */
#define	_3C503_GACFR_RSEL	0x08	/* RAM Select */
#define	_3C503_GACFR_TEST	0x10	/* Test */
#define	_3C503_GACFR_0WS	0x20	/* Zero Wait State */
#define	_3C503_GACFR_TCM	0x40	/* Terminal Count Mask */
#define	_3C503_GACFR_NIM	0x80	/* NIC Interrupt Mask */

/*
 * 3C503 Gate Array Control Register.
 */
#define	_3C503_CR_RST		0x01	/* Software Reset */
#define	_3C503_CR_XSEL		0x02	/* XCVR Select */
#define	_3C503_CR_EALO		0x04	/* Ethernet Address Low */
#define	_3C503_CR_EAHI		0x08	/* Ethernet Address High */
#define	_3C503_CR_SHARE		0x10	/* Interrupt Share */
#define	_3C503_CR_DBSEL		0x20	/* Double Buffer Select */
#define	_3C503_CR_DDIR		0x40	/* DMA Direction */
#define	_3C503_CR_START		0x80	/* Start DMA */

/*
 * 3C503 Gate Array Status Register.
 */
#define	_3C503_STREG_REV	0x07	/* Gate Array Revision */
#define	_3C503_STREG_DIP	0x08	/* DMA In Progress */
#define	_3C503_STREG_DTC	0x10	/* DMA Terminal Count */
#define	_3C503_STREG_OFLW	0x20	/* DMA Overflow */
#define	_3C503_STREG_UFLW	0x40	/* DMA Underflow */
#define	_3C503_STREG_DPRDY	0x80	/* Data Port Ready */

/*
 * 3C503 Interrupt/DMA Configuration Register.
 */
#define	_3C503_IDCFR_DRQ1	0x01	/* DMA Request 1 */
#define	_3C503_IDCFR_DRQ2	0x02	/* DMA Request 2 */
#define	_3C503_IDCFR_DRQ3	0x04	/* DMA Request 3 */
#define	_3C503_IDCFR_IRQ2	0x10	/* Interrupt Request 2 */
#define	_3C503_IDCFR_IRQ3	0x20	/* Interrupt Request 3 */
#define	_3C503_IDCFR_IRQ4	0x40	/* Interrupt Request 4 */
#define	_3C503_IDCFR_IRQ5	0x80	/* Interrupt Request 5 */

/*
 * 3C503 Ethernet Address PROM.
 */
#define	_3C503_LAR_0		0
#define	_3C503_LAR_1		1
#define	_3C503_LAR_2		2
#define	_3C503_LAR_3		3
#define	_3C503_LAR_4		4
#define	_3C503_LAR_5		5

extern int probe_3c503(void);

#endif	/* DEVICE_3C503 */
#endif	/* __3C503_H__ */
