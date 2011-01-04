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

#ifndef _BUSSES_GSC_GSC_H_
#define	_BUSSES_GSC_GSC_H_

#include <busses/iobus.h>

struct gsc_board;

typedef	void	(*gsc_attach_t)(	/* GSC attach function */
    struct gsc_board *);

enum gsc_board_type {
    GSC_BOARD_TYPE_B_DMA,
    GSC_BOARD_TYPE_A_DMA,
    GSC_BOARD_TYPE_A_DIRECT
};

#define	GSC_MAX_INTR	32		/* Max # of interrupts */
    
/*
 * Definition of a Graphics System Connect board
 */
struct gsc_board {
    struct gsc_board	 *gsc_next;		/* GSC board list */
    enum gsc_board_type	  gsc_type;		/* GSC board type */
    IOBUS_TYPE_T	  gsc_iotype;		/* I/O bus type */
    IOBUS_ADDR_T	  gsc_iobase;		/* I/O address base */
    unsigned int	  gsc_eir;		/* External Interrupt */
    unsigned int	  gsc_ipl;		/* Interrupt Priority Level */
    unsigned int	  gsc_flags;		/* flags */
    void		(*gsc_intr)(int);	/* Interrupt handler */
    int			  gsc_unit;		/* Interrupt handler argument */
};

#define	GSC_FLAGS_EIR_SHARED	0x01	/* EIR is shared */

extern void	gsc_attach_slot(IOBUS_TYPE_T,
				IOBUS_ADDR_T,
				enum gsc_board_type,
				unsigned int,
				unsigned int,
				gsc_attach_t,
				void (*)(struct gsc_board *));

extern void	gsc_intr(int);

#endif /* _BUSSES_GSC_GSC_H_ */
