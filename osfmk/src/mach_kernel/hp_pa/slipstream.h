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

#ifndef	_MACHINE_SLIPSTREAM_H_
#define	_MACHINE_SLIPSTREAM_H_

#include <kern/macro_help.h>
#include <hp_pa/iomod.h>

#define	SLIPSTREAM_NPAGES	  3	/* # of expand pages */
#define	SLIPSTREAM_NTSIREGS	256	/* # of TSI registers */

/*
 * Definitions of the io_ii_rw (SRS 3) register
 */
#define	SLIPSTREAM_AR_ENABLED	0x80000000	/* Arbitration Enable */
#define	SLIPSTREAM_IT_RECEIVED	0x40000000	/* TSI interrupt received */
#define	SLIPSTREAM_IT_ENABLED	0x20000000	/* Interrupt Enabled */
#define	SLIPSTREAM_REV_CODE	0x1C000000	/* Revision Code */
#define	SLIPSTREAM_PF_ENABLED	0x02000000	/* Prefectch Enabled */
#define	SLIPSTREAM_CH_ENABLED	0x01000000	/* Two Channels Enabled */
#define	SLIPSTREAM_PR_ENABLED	0x00800000	/* Pended read Enabled */
#define	SLIPSTREAM_GRDY_ENABLED	0x00400000	/* Fast GSC ready Enabled */
#define	SLIPSTREAM_TSA_ENABLED	0x00200000	/* TSI Stream Abort Enabled */
#define	SLIPSTREAM_DGBR_ENABLED	0x00100000	/* Deferred GSC+ req. Enabled */
#define	SLIPSTREAM_IO_II	0x00000020	/* I/O Interrupt Sent */

/*
 * Define the Type-dependent Register Set for the Slipstream converter.
 * This structure maps over iomod's `hvrs' array (see "iomod.h").
 */
#define	SLIPSTREAM_TRS(iotype, iop)					\
	((struct slipstream_trs *)(((struct iomod *)(iop))->hvrs))

struct slipstream_trs {
    unsigned int	sl_resv1;			/* Reserved */
    unsigned int	sl_wr_data;			/* IODC write */
    unsigned int	sl_cntl;			/* IODC control */
    unsigned int	sl_resv2[253];			/* Reserved */
    unsigned int	sl_tsi[SLIPSTREAM_NTSIREGS];	/* TSI registers */
};

/* sl_cntl flags */
#define	SLIPSTREAM_EXPAND_IO	 0x40	/* 16K internal address range */
#define	SLIPSTREAM_WIPER_FLAG	 0x10	/* Bank1 selected */
#define	SLIPSTREAM_EEPROM_SIZE	 0x08	/* EEPROM size == 128K */
#define	SLIPSTREAM_WIPER_CHIP	 0x04	/* Do not Select EEPOT */
#define	SLIPSTREAM_WIPER_SELECT	 0x02	/* Goto Bank1 */
#define	SLIPSTREAM_WIPER_CONTROL 0x01	/* Wiper update */

/*
 * Access to the TSI registers
 */
#define	SLIPSTREAM_TSI(iotype, iop, reg)				\
	((SLIPSTREAM_TRS(iotype, iop)->sl_tsi)[(reg)])

#define	SLIPSTREAM_INIT(iotype, iop)					\
MACRO_BEGIN								\
    ((struct iomod *)(iop))->io_ii_rw &= ~(SLIPSTREAM_PF_ENABLED |	\
					   SLIPSTREAM_CH_ENABLED);	\
MACRO_END

#define	SLIPSTREAM_RESET(iotype, iop, proc)				\
MACRO_BEGIN								\
    u_int _status;							\
    ((struct iomod *)(iop))->io_command = CMD_RESET;			\
    for (;;) {								\
	_status = ((struct iomod *)(iop))->io_status;			\
	if (_status & IO_ERR_MEM_FE)					\
	    panic("%s: SLIPSTREAM Reset reported fatal error %d\n",	\
		  proc, IO_ERR_VAL(_status));				\
	if (_status & IO_ERR_MEM_RY)					\
	    break;							\
    }									\
    ((struct iomod *)(iop))->io_ii_rw = SLIPSTREAM_IO_II;		\
MACRO_END

#define	SLIPSTREAM_SET_INTR(iotype, iop, eir)				\
MACRO_BEGIN								\
    ((struct iomod *)(iop))->io_eim = (u_int)prochpa | (eir);		\
    ((struct iomod *)(iop))->io_ii_rw = SLIPSTREAM_IT_ENABLED;		\
MACRO_END

#define	SLIPSTREAM_GET_INTR(iotype, iop)				\
    ((((struct iomod *)(iop))->io_ii_rw & SLIPSTREAM_IO_II) != 0)

#define	SLIPSTREAM_CLR_INTR(iotype, iop)				\
MACRO_BEGIN								\
    ((struct iomod *)(iop))->io_ii_rw |= SLIPSTREAM_IO_II;		\
MACRO_END

#define	SLIPSTREAM_MSK_INTR(iotype, iop)				\
MACRO_BEGIN								\
    ((struct iomod *)(iop))->io_ii_rw = 0;				\
MACRO_END

#endif /* _MACHINE_SLIPSTREAM_H_ */
