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
 * Old Log from ppc directory follows:
 * Revision 1.1.9.3  1996/07/29  09:41:49  stephen
 * 	Added bcopy_nc
 * 	Added DMA_IFR - position of type of DMA interrupt
 * 	[1996/07/29  09:27:49  stephen]
 *
 * Revision 1.1.9.2  1996/07/16  10:47:47  stephen
 * 	Change macro definitions to use variable offset for
 * 	kernel vm mappings of IO space. Change prototype and
 * 	hence implementation of get_icr_level.
 * 	[1996/07/16  10:44:19  stephen]
 * 
 * Revision 1.1.9.1  1996/05/28  19:09:38  stephen
 * 	Added prototypes for powermac_reboot and powermac_powerdown
 * 	[1996/05/28  19:03:43  stephen]
 * 
 * Revision 1.1.7.4  1996/05/03  17:26:42  stephen
 * 	Added APPLE_FREE_COPYRIGHT
 * 	[1996/05/03  17:21:35  stephen]
 * 
 * Revision 1.1.7.3  1996/04/29  09:05:31  stephen
 * 	Second SCSI bus
 * 	[1996/04/29  08:59:47  stephen]
 * 
 * Revision 1.1.7.2  1996/04/11  18:03:07  emcmanus
 * 	Added DMA_BUFFER_UNCACHED_SIZE.
 * 	[1996/04/11  18:02:06  emcmanus]
 * 
 * Revision 1.1.7.1  1996/04/11  09:11:19  emcmanus
 * 	Copied from mainline.ppc.
 * 	[1996/04/11  08:03:51  emcmanus]
 * 
 * Revision 1.1.5.4  1996/01/22  14:58:10  stephen
 * 	Added DMA_BUFFER_ALIGNMENT
 * 	[1996/01/22  14:37:54  stephen]
 * 
 * Revision 1.1.5.3  1996/01/12  16:16:27  stephen
 * 	Added DMA_BUFFER_SIZE
 * 	[1996/01/12  14:30:07  stephen]
 * 
 * Revision 1.1.5.2  1995/12/19  10:19:33  stephen
 * 	Added more powermac information
 * 	[1995/12/19  10:14:35  stephen]
 * 
 * Revision 1.1.5.1  1995/11/23  10:56:35  stephen
 * 	first powerpc checkin to mainline.ppc
 * 	[1995/11/23  10:28:36  stephen]
 * 
 * Revision 1.1.3.1  1995/10/10  15:11:05  stephen
 * 	return from apple
 * 	[1995/10/10  14:40:46  stephen]
 * 
 * 	added a few definitions
 * 
 * Revision 1.1.1.2  95/09/05  17:57:44  stephen
 * 	Created
 * 
 * EndLog
 */

#ifndef _POWERMAC_H_
#define _POWERMAC_H_

#include <ppc/POWERMAC/powermac_pdm.h>
#include <ppc/POWERMAC/powermac_pci.h>

#ifndef ASSEMBLER

#include <mach/ppc/vm_types.h>

/*
 * Machine class 
 */

#define	POWERMAC_CLASS_PDM	1	/* Power Mac 61/71/8100 */
#define	POWERMAC_CLASS_PERFORMA	2	/* Power Mac 52/6200 */
#define	POWERMAC_CLASS_PCI	3	/* Power Mac 72/75/85/9500 */
#define	POWERMAC_CLASS_POWERBOOK 4	/* Power Book 2300/5300 */

typedef struct powermac_info {
	int		class;			/* Machine type */

	int		bus_clock_rate_hz;	/* Bus frequency */

	/* to convert from real time ticks to nsec convert by this*/
	unsigned long	proc_clock_to_nsec_numerator; 
	unsigned long	proc_clock_to_nsec_denominator; 
	
	int		io_size;		/* IO region */
	vm_offset_t	io_base_phys;		/* IO region */
	vm_offset_t	io_base_virt;		/* IO region */

	int		dma_buffer_alignment;	/* preallocated DMA region */
	int		dma_buffer_size;	/* preallocated DMA region */
	vm_offset_t	dma_buffer_phys;	/* preallocated DMA region */
	vm_offset_t	dma_buffer_virt;	/* preallocated DMA region */
	int		machine;		/* Gestalt value .. */
	unsigned int hashTableAdr;		/* Shadowed value of SDR1 */
	boolean_t	io_coherent;		/* Is I/O coherent? */
} powermac_info_t;

extern powermac_info_t powermac_info;

extern void		powermac_define_machine(int gestaltType);

/* Used to initialise IO once DMA and IO virtual space has been assigned */
extern void powermac_io_init(vm_offset_t powermac_io_base_v);

/* Macro to convert from a physical I/O address into a virtual I/O address */
#define POWERMAC_IO(addr)	(powermac_info.io_base_virt +	\
				 (addr - powermac_info.io_base_phys))

/* prototypes */

extern void powermac_powerdown(void);
extern void powermac_reboot(void);

/* Some useful typedefs for accessing control registers */

typedef volatile unsigned char	v_u_char;
typedef volatile unsigned short v_u_short;
typedef volatile unsigned int	v_u_int;
typedef volatile unsigned long  v_u_long;

/* And some useful defines for reading 'volatile' structures,
 * don't forget to be be careful about sync()s and eieio()s
 */
#define reg8(reg) (*(v_u_char *)reg)
#define reg16(reg) (*(v_u_short *)reg)
#define reg32(reg) (*(v_u_int *)reg)

/* Non-cached version of bcopy */
extern void	bcopy_nc(char *from, char *to, int size);

/* VIA1_AUXCONTROL values same across several powermacs */
#define	VIA1_AUX_AUTOT1LATCH 0x40	/* Autoreload of T1 from latches */

#endif /* ndef(ASSEMBLER) */
#endif /* _POWERMAC_H_ */
