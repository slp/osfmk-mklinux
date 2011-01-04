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
 * For i386 PCI, just use direct dma map from generic direct dma support
 */

#include <types.h>
#include <kern/lock.h>
#include <device/dma_support.h>

u_long
dma_map_load(
	u_long			bc,
	vm_offset_t		va,
	thread_act_t		act,
	struct bus_ctlr		*ctlr,
	dma_handle_t		*sglistp,
	u_long			max_bc,
	int			flags)
{
	u_long	rtn_bc;

	/*
	 * handle the 'alloc on the fly' style dma.
	 */
	if (*sglistp == 0) {
		rtn_bc = direct_map_alloc (bc,
					   ctlr,
					   (sglist_t *)sglistp,
					   flags);
		if (rtn_bc == 0) { 
			*(sglist_t *)sglistp = (sglist_t)0;
			return(0);
		}
	} else
	  	rtn_bc = bc; 
	
	
	if (direct_map_load (rtn_bc,
			     va,
			     act,
			     *(sglist_t *)sglistp,
			     max_bc,
			     0))
		return(rtn_bc);
	else
		return(0);
}

int
dma_map_unload(
	int		flags,
	dma_handle_t	sglistp)
{
	if (sglistp == NULL)
		return(0);
	
	direct_map_unload ((sglist_t)sglistp);
	/*
	 * if flags has DMA_DEALLOC set, dealloc resources;
	 * mirror of dma_map_load where sglistp == 0 means do an alloc
	 */
	if (flags & DMA_DEALLOC)
		direct_map_dealloc ((sglist_t)sglistp);
	return(1);
}

u_long
dma_map_alloc(
	u_long 		bc,
	struct bus_ctlr *ctlr,
	dma_handle_t	*sglistp,
	int 		flags)
{
	u_long	rtn_bc;

	rtn_bc = direct_map_alloc (bc,
				   ctlr,
				   sglistp,
				   flags);
	if (rtn_bc == 0) { 
	  	*sglistp = 0;
		return(0);
	} else
		return(rtn_bc);
}

int	
dma_map_dealloc(
	dma_handle_t	sglistp)
{
	return(direct_map_dealloc((sglist_t)sglistp));
}
