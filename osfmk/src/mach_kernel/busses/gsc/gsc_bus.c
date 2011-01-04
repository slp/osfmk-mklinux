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

#include <gsc.h>
#include <kern/assert.h>
#include <kern/kalloc.h>
#include <busses/iobus.h>
#include <busses/gsc/gsc.h>

struct gsc_board *gsc_board[GSC_MAX_INTR];	/* GSC config per interrupt */

/*
 * gsc_attach_slot
 *
 * Purpose : Graphic System Connect slot attach
 * Returns : Nothing
 *
 * IN_args : iotype ==> I/O bus type
 *	     iobase ==> I/O bus base address
 *	     type   ==> GSC board type
 *	     eir    ==> Interrupt number
 *	     ipl    ==> Interrupt Priority Level
 *	     attach ==> specific board attach routine
 *	     setup  ==> Machine-dependent setup function
 */
void
gsc_attach_slot(
    IOBUS_TYPE_T iotype,
    IOBUS_ADDR_T iobase,
    enum gsc_board_type type,
    unsigned int eir,
    unsigned int ipl,
    gsc_attach_t attach,
    void (*setup)(struct gsc_board *))
{
    struct gsc_board *gp, *gpp;
    unsigned int i;

    assert(eir < GSC_MAX_INTR);
    assert(IOBUS_TYPE_BUS(iotype) == IOBUS_TYPE_BUS_GSC);

    gp = (struct gsc_board *)kalloc(sizeof (struct gsc_board));

    gp->gsc_iotype = iotype;
    gp->gsc_iobase = iobase;
    gp->gsc_type = type;
    gp->gsc_eir = eir;
    gp->gsc_ipl = ipl;
    if (gsc_board[eir] != (struct gsc_board *)0) {
	if ((gsc_board[eir]->gsc_flags & GSC_FLAGS_EIR_SHARED) == 0)
	    gsc_board[eir]->gsc_flags |= GSC_FLAGS_EIR_SHARED;
	gp->gsc_flags = GSC_FLAGS_EIR_SHARED;
    } else
	gp->gsc_flags = 0;
    gp->gsc_next = gsc_board[eir];
    gsc_board[eir] = gp;

    (*attach)(gp);
    (*setup)(gp);
}

/*
 * gsc_intr
 *
 * Purpose : GSC bus common interrupt
 * Returns : Nothing
 *
 * IN_arg  : eir ==> Interrupt number
 */
void
gsc_intr(
    int eir)
{
    struct gsc_board *gp;

    assert(eir >= 0 && eir < GSC_MAX_INTR);

    for (gp == gsc_board[eir]; gp != (struct gsc_board *)0; gp = gp->gsc_next)
	(*gp->gsc_intr)(gp->gsc_unit);
}
