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
 * File :  etap_map.c
 * 
 *	   Pseudo-device driver to calculate the virtual addresses
 *         of all mappable ETAP buffers and tables: event table, 
 *         subsystem table, cumulative buffer and monitor buffers.
 *
 */
/*
 * Minor device number representation:
 * 
 * 	0       = ETAP_TABLE_EVENT
 * 	1       = ETAP_TABLE_SUBSYSTEM
 *     	2       = ETAP_BUFFER_CUMULATIVE
 *     	3 & up  = a specific monitor buffer
 *
 */

#include <types.h>

#include <mach/vm_prot.h>
#include <mach/vm_param.h>
#include <mach/kern_return.h>
#include <vm/pmap.h>
#include <device/io_req.h>
#include <device/dev_hdr.h>

#include <cpus.h>
#include <kern/etap_options.h>
#include <mach/etap.h>
#include <kern/etap_map.h>


#if     ETAP_LOCK_ACCUMULATE
extern	cumulative_buffer_t 	cbuff;
#endif  /* ETAP_LOCK_ACCUMULATE */

#if     ETAP_MONITOR
extern	monitor_buffer_t	mbuff[];
#endif  /* ETAP_MONITOR */


/*
 * etap_map_open - Check for valid minor device
 */

io_return_t
etap_map_open(
        dev_t           dev,
        dev_mode_t      flags,
        io_req_t        ior)
{	
	int buffer = minor(dev);

	if (buffer >= ETAP_MAX_DEVICES)
		return(D_NO_SUCH_DEVICE);

	return(D_SUCCESS);
}

vm_offset_t
etap_map_mmap (
        dev_t		dev,
        vm_offset_t	off,
        vm_prot_t 	prot)
{
	int		buffer = minor(dev);
	vm_offset_t 	addr;

	/*
	 *  Check request validity
	 */
    
	if (prot & VM_PROT_WRITE)
		return(KERN_PROTECTION_FAILURE);

	if (buffer < 0 || buffer >= ETAP_MAX_DEVICES)
		return(KERN_INVALID_ARGUMENT);

	switch(buffer) {
		case ETAP_TABLE_EVENT : 
			addr = trunc_page((char *) event_table) + off;
			break;
		case ETAP_TABLE_SUBSYSTEM :
			addr = trunc_page((char *) subs_table) + off;
			break;
		case ETAP_BUFFER_CUMULATIVE :
#if     ETAP_LOCK_ACCUMULATE
			addr = (vm_offset_t) cbuff + off;
			break;
#else 	/* ETAP_LOCK_ACCUMULATE */
			return(KERN_INVALID_ARGUMENT);
#endif	/* ETAP_LOCK_ACCUMULATE */

		default :
#if	ETAP_MONITOR
			addr = (vm_offset_t) mbuff[buffer - 3] + off;
			break;
#else	/* ETAP_MONITOR */
			return(KERN_INVALID_ARGUMENT);
#endif	/* ETAP_MONITOR */

	}
	return machine_btop(pmap_extract(pmap_kernel(), addr));
}
