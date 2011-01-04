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

#ifndef	_CHIPS_MYRICOM_NET_MYRICOM_GSC_H_
#define	_CHIPS_MYRICOM_NET_MYRICOM_GSC_H_

#include <mach/boolean.h>
#include <mach/vm_types.h>
#include <device/if_myrinet.h>
#include <device/net_device.h>
#include <busses/iobus.h>
#include <busses/gsc/gsc.h>
#include <hp_pa/slipstream.h>

/*
 * TSI index of Slipstream specific registers
 */
#define	NET_MYRICOM_GSC_REG_RESET	      0	/* Reset LanAI board */
#define	NET_MYRICOM_GSC_REG_PNUM	      1	/* Page mapping */
#define	NET_MYRICOM_GSC_REG_MASK	      5	/* Page mask */
#define	NET_MYRICOM_GSC_REG_LANAI	    192	/* LanAI registers offset */

#define	NET_MYRICOM_GSC_REG_MASK_ENA 0x80000000	/* Enable wakeup */
#define	NET_MYRICOM_GSC_REG_MASK_MSK 0x000003FC	/* Mask wakeup */

/*
 * GSC bus-dependent MYRICOM board
 */

#define	NET_MYRICOM_GSC_IODC_OFFSET	     16	/* IODC offset of EEPROM ID */

typedef struct net_myricom_gsc {
    IOBUS_TYPE_T	 	 myio_iotype;	/* I/O bus type */
    IOBUS_ADDR_T	 	 myio_iobase;	/* I/O bus base address */
    unsigned int		 myio_eir;	/* Interrupt number */
    unsigned int		 myio_clockval;	/* Clockvalue of LanAI chip */
    unsigned int		 myio_ramsize;	/* On-board RAM size */
    unsigned int		 myio_lanai;	/* LANai id */
    vm_offset_t			 myio_page[SLIPSTREAM_NPAGES];
						/* LanAI mapped pages */
    unsigned int		 myio_ref[SLIPSTREAM_NPAGES];
						/* LanAI ref pages */
    unsigned int		 myio_flags;	/* flags */
} *net_myricom_gsc_t;

#define	NET_MYRICOM_GSC_FLAGS_RUNNING	0x01	/* LanAi chip running */
#define	NET_MYRICOM_GSC_FLAGS_INTERRUPT	0x02	/* LanAi interrupt on */

extern gsc_attach_t	net_myricom_gsc_probe(IOBUS_TYPE_T,
					      IOBUS_ADDR_T);

#endif /* _CHIPS_MYRICOM_NET_MYRICOM_GSC_H_ */
