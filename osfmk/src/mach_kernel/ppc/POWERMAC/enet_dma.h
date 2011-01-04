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
 * 
 */
/*
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */
/* 
 * PMach Operating System
 * Copyright (c) 1995 Santa Clara University
 * All Rights Reserved.
 */

/* 0x50f31c20 */
struct enetdma {
     volatile unsigned char   xmtcs;
     char enetdmapad0[1031];
     volatile unsigned char   rcvcs;
     char enetdmapad1[7];
     volatile unsigned char   rcvhp;
     char enetdmapad2[3];
     volatile unsigned char   rcvtp;
     char enetdmapad3[15];
     volatile unsigned char   xmtbch_0;
     volatile unsigned char   xmtbcl_0;
	 char enetdmapad4[14];
     volatile unsigned char   xmtbch_1;
     volatile unsigned char   xmtbcl_1;
     };

/* rcv dma control and status */
#define IF          0x80
#define OVERRUN     0x40
#define IE          0x08 
#define DMARUN      0x02
#define RST         0x01

/* xmt dma control and status */
#define IF          0x80
#define SET_1       0x40
#define SET_0       0x20
#define IE          0x08 
#define DMARUN      0x02
#define RST         0x01

#define DMA_RCV_BASE	powermac_info.dma_buffer_phys
#define DMA_RCV_BASE_V	powermac_info.dma_buffer_virt

#define DMA_RCV_SIZE	0xc000

#define DMA_XMT_BASE_0  (DMA_RCV_BASE + 0x14000)
#define DMA_XMT_BASE_1  (DMA_RCV_BASE + 0x14800)

#define DMA_XMT_BASE_0_V  (DMA_RCV_BASE_V + 0x14000)
#define DMA_XMT_BASE_1_V  (DMA_RCV_BASE_V + 0x14800)
