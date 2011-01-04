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

#ifndef _POWERMAC_M2_H_
#define _POWERMAC_M2_H_

#include <mach/ppc/vm_types.h>
#include <ppc/spl.h>

#ifndef ASSEMBLER

/* Physical address and size of IO control registers region */

#define M2_IO_BASE_ADDR	0x50f00000
#define M2_IO_SIZE		0x42000


/* Interrupt control register */
#define M2_ICR			POWERMAC_IO(M2_IO_BASE_ADDR+0x2a000)

/* PMU registers */
#define M2_PMU_BASE_PHYS	(M2_IO_BASE_ADDR)

/* Interrupt controlling VIAs */

#define M2_VIA1_IFR		(POWERMAC_IO(M2_IO_BASE_ADDR+0x01a00))
#define M2_VIA1_IER		(POWERMAC_IO(M2_IO_BASE_ADDR+0x01c00))
#define M2_VIA1_PCR		(POWERMAC_IO(M2_IO_BASE_ADDR+0x01800))

#define	M2_VIA1_AUXCONTROL	(POWERMAC_IO(M2_IO_BASE_ADDR+0x01600))
#define	M2_VIA1_T1COUNTERLOW	(POWERMAC_IO(M2_IO_BASE_ADDR+0x00800))
#define	M2_VIA1_T1COUNTERHIGH	(POWERMAC_IO(M2_IO_BASE_ADDR+0x00A00))
#define	M2_VIA1_T1LATCHLOW	(POWERMAC_IO(M2_IO_BASE_ADDR+0x00C00))
#define	M2_VIA1_T1LATCHHIGH	(POWERMAC_IO(M2_IO_BASE_ADDR+0x00E00))

#define M2_VIA2_IFR		(POWERMAC_IO(M2_IO_BASE_ADDR+0x3a00))
#define M2_VIA2_IER		(POWERMAC_IO(M2_IO_BASE_ADDR+0x3c00))
#define M2_VIA2_SLOT_IFR	(POWERMAC_IO(M2_IO_BASE_ADDR+0x3e00))

#define	M2_DMA_IFR		(POWERMAC_IO(M2_IO_BASE_ADDR+0x2a008))
#define M2_DMA_AUDIO		(POWERMAC_IO(M2_IO_BASE_ADDR+0x2a00a))

#define	M2_IDE0_BASE		(0x50F1A000)

void   m2_interrupt_initialize(void);

#endif /* ndef ASSEMBLER */
#endif /* _POWERMAC_M2_H_ */
