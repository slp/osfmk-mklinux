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

#ifndef	_CHIPS_MYRICOM_NET_MYRICOM_H_
#define	_CHIPS_MYRICOM_NET_MYRICOM_H_

#include <device/if_myrinet.h>

struct net_myricom_eeprom {
    unsigned int	myee_clockval;	/* Clockval initial value */
    unsigned short	myee_lanai;	/* LanAI Identification */
    unsigned char	myee_addr[IF_MYRINET_UID_SIZE];
					/* Myricom's uniq station-ID number */
    unsigned int	myee_ramsize;	/* On-board RAM size */
};

#define	NET_MYRICOM_ID_UNKNOWN	0x0000
#define	NET_MYRICOM_ID_LANAI23	0x0203
#define	NET_MYRICOM_ID_LANAI30	0x0300
#define	NET_MYRICOM_ID_LANAI31	0x0301
#define	NET_MYRICOM_ID_LANAI32	0x0302
#define	NET_MYRICOM_ID_LANAI40	0x0400
#define	NET_MYRICOM_ID_LANAI41	0x0401
#define	NET_MYRICOM_ID_LANAI42	0x0402

#define	NET_MYRICOM_ID_VERSION(lanai)	(((unsigned int)(lanai) >> 8) & 0xFF)
#define	NET_MYRICOM_ID_REVISION(lanai)	((unsigned int)(lanai) & 0xFF)

#define NET_MYRICOM_CLOCKVAL32	0x11071107	/* Clockval for lanAI 3.2 */
#define NET_MYRICOM_CLOCKVAL41	0x90449044	/* Clockval for lanAI 4.1 */

/*
 * TSI index of LanAI registers
 */
#define	NET_MYRICOM_REG_IPF0	  0	/* 5 context-0 state registers */
#define	NET_MYRICOM_REG_CUR0	  1
#define	NET_MYRICOM_REG_PREV0	  2
#define	NET_MYRICOM_REG_DATA0	  3
#define	NET_MYRICOM_REG_DPF0	  4

#define	NET_MYRICOM_REG_IPF1	  5	/* 5 context-1 state registers */
#define	NET_MYRICOM_REG_CUR1	  6
#define	NET_MYRICOM_REG_PREV1	  7
#define	NET_MYRICOM_REG_DATA1	  8
#define	NET_MYRICOM_REG_DPF1	  9

#define	NET_MYRICOM_REG_ISR	 10	/* Interrupt Status Register */
#define	NET_MYRICOM_REG_EIMR	 11	/* External Interrupt Mask Register */
#define	NET_MYRICOM_REG_IT	 12	/* Interrupt Timer */
#define	NET_MYRICOM_REG_RTC	 13	/* Real Time Clock */

#define	NET_MYRICOM_REG_CKS	 14	/* internet ChecKSum */
#define	NET_MYRICOM_REG_EAR	 15	/* EBus DMA External Address Register */
#define	NET_MYRICOM_REG_LAR	 16	/* EBus DMA Local Address Register */
#define	NET_MYRICOM_REG_DMA_CTR	 17	/* EBus DMA CounTer Register */

#define	NET_MYRICOM_REG_RMP	 18	/* Receive DMA Pointer */
#define	NET_MYRICOM_REG_RML	 19	/* Receive DMA Limit */

#define	NET_MYRICOM_REG_SMP	 20	/* Send DMA Pointer */
#define	NET_MYRICOM_REG_SML	 21	/* Send DMA Limit */
#define	NET_MYRICOM_REG_SMLT	 22	/* Send DMA Limit with Tail */

#define	NET_MYRICOM_REG_RB	 24	/* Receive a Byte */
#define	NET_MYRICOM_REG_RH	 25	/* Receive a Half-word */
#define	NET_MYRICOM_REG_RW	 26	/* Receive a Word */

#define	NET_MYRICOM_REG_SA	 27	/* Send Alignment */

#define	NET_MYRICOM_REG_SB	 28	/* Send a Byte */
#define	NET_MYRICOM_REG_SH	 29	/* Send a Half-word */
#define	NET_MYRICOM_REG_SW	 30	/* Send a Word */
#define	NET_MYRICOM_REG_ST	 31	/* Send the Tail */

#define	NET_MYRICOM_REG_DMA_DIR	 32	/* EBus DIRection */
#define	NET_MYRICOM_REG_DMA_STS	 33	/* Ebus STatuS */
#define	NET_MYRICOM_REG_TIMEOUT	 34	/* NRES-TIMEOUT selection */
#define	NET_MYRICOM_REG_MYRINET	 35	/* MYRINET link configuration */

#define	NET_MYRICOM_REG_HW_DBUG	 36	/* HardWare DeBUGging */
#define	NET_MYRICOM_REG_LED	 37	/* 11 output pins LED */
#define	NET_MYRICOM_REG_VERSION	 38	/* Myrinet link interface config. */
#define	NET_MYRICOM_REG_ACTIVATE 39	/* activate Myrinet-link */

#define	NET_MYRICOM_REG_CLOCK	 63	/* on-chip clock initialization */

/*
 * Interrupt Status register
 */
#define	NET_MYRICOM_ISR_DGB  0x80000000	/* Debug */
#define	NET_MYRICOM_ISR_HOST 0x40000000	/* Host signal */
#define	NET_MYRICOM_ISR_LAN7 0x00800000	/* LanAI 7 signal */
#define	NET_MYRICOM_ISR_LAN6 0x00400000	/* LanAI 6 signal */
#define	NET_MYRICOM_ISR_LAN5 0x00200000	/* LanAI 5 signal */
#define	NET_MYRICOM_ISR_LAN4 0x00100000	/* LanAI 4 signal */
#define	NET_MYRICOM_ISR_LAN3 0x00080000	/* LanAI 3 signal */
#define	NET_MYRICOM_ISR_LAN2 0x00040000	/* LanAI 2 signal */
#define	NET_MYRICOM_ISR_LAN1 0x00020000	/* LanAI 1 signal */
#define	NET_MYRICOM_ISR_LAN0 0x00010000	/* LanAI 0 signal */
#define	NET_MYRICOM_ISR_WORD 0x00008000	/* Word ready */
#define	NET_MYRICOM_ISR_HALF 0x00004000	/* Half-word ready */
#define	NET_MYRICOM_ISR_SRDY 0x00002000	/* Send ready */
#define	NET_MYRICOM_ISR_LINK 0x00001000	/* Open Link timeout (LANai 3.x) */
#define	NET_MYRICOM_ISR_NRES 0x00000800	/* Network reset interrupt */
#define	NET_MYRICOM_ISR_WAKE 0x00000400	/* Wake interrupt */
#define	NET_MYRICOM_ISR_OVR2 0x00000200	/* Overrun 2 interrupt */
#define	NET_MYRICOM_ISR_OVR1 0x00000100	/* Overrun 1 interrupt */
#define	NET_MYRICOM_ISR_TAIL 0x00000080	/* Tail interrupt */
#define	NET_MYRICOM_ISR_WDOG 0x00000040	/* WatchDog interrupt */
#define	NET_MYRICOM_ISR_TIME 0x00000020	/* Timer interrupt */
#define	NET_MYRICOM_ISR_DMA  0x00000010	/* end of DMA interrupt */
#define	NET_MYRICOM_ISR_SEND 0x00000008	/* Send interrupt */
#define	NET_MYRICOM_ISR_BUFF 0x00000004	/* Buffer exhausted interrupt */
#define	NET_MYRICOM_ISR_RECV 0x00000002	/* Receive interrupt */
#define	NET_MYRICOM_ISR_BYTE 0x00000001	/* Byte ready */

/*
 * EBUS-DMA Status Register
 */
#define	NET_MYRICOM_DMA_STS_16	0x08	/* 16-word bursts */
#define	NET_MYRICOM_DMA_STS_08	0x04	/*  8-word bursts */
#define	NET_MYRICOM_DMA_STS_04	0x02	/*  4-word bursts */
#define	NET_MYRICOM_DMA_STS_02	0x01	/*  2-word bursts */

/*
 * Flavors for the status function
 */
enum net_myricom_flavor {
    NET_MYRICOM_FLAVOR_INTR,		/* Enable LANai Interrupts */
    NET_MYRICOM_FLAVOR_START		/* Start LANai program */
};

#endif /* _CHIPS_MYRICOM_NET_MYRICOM_H_ */
