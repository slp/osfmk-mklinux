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
/* CMU_HIST */
/*
 * Revision 2.7.4.1  92/03/03  16:26:07  jeffreyh
 * 	Changes from TRUNK
 * 	[92/02/26  12:34:10  jeffreyh]
 * 
 * Revision 2.8  92/01/14  16:47:54  rpd
 * 	Split vm_mem_init into vm_mem_bootstrap and vm_mem_init.
 * 	[91/12/29            rpd]
 * 
 * Revision 2.7  91/05/18  14:40:21  rpd
 * 	Replaced vm_page_startup with vm_page_bootstrap.
 * 	[91/04/10            rpd]
 * 
 * Revision 2.6  91/05/14  17:49:05  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/03/16  15:05:10  rpd
 * 	Added vm_fault_init.
 * 	[91/02/16            rpd]
 * 
 * Revision 2.4  91/02/05  17:58:17  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:32:11  mrt]
 * 
 * Revision 2.3  90/02/22  20:05:35  dbg
 * 	Reduce lint.
 * 	[90/01/29            dbg]
 * 
 * Revision 2.2  89/09/08  11:28:15  dbg
 * 	Initialize kalloc package here.
 * 	[89/09/06            dbg]
 * 
 * 	Removed non-XP code.
 * 	[89/04/28            dbg]
 * 
 * Revision 2.10  89/04/22  15:35:20  gm0w
 * 	Removed MACH_VFS code.
 * 	[89/04/14            gm0w]
 * 
 * Revision 2.9  89/04/18  21:25:30  mwyoung
 * 	Recent history:
 * 	 	Add memory_manager_default_init(), vm_page_module_init().
 * 
 * 	Older history has been integrated into the documentation for
 * 	this module.
 * 	[89/04/18            mwyoung]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *	File:	vm/vm_init.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Initialize the Virtual Memory subsystem.
 */

#include <mach_rt.h>

#include <mach/machine/vm_types.h>
#include <kern/zalloc.h>
#include <kern/kalloc.h>
#include <kern/rtalloc.h>
#include <vm/vm_object.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <vm/vm_kern.h>
#include <vm/memory_object.h>
#include <vm/vm_fault.h>
#include <vm/vm_init.h>

/*
 *	vm_mem_bootstrap initializes the virtual memory system.
 *	This is done only by the first cpu up.
 */

void
vm_mem_bootstrap(void)
{
	vm_offset_t	start, end;

	/*
	 *	Initializes resident memory structures.
	 *	From here on, all physical memory is accounted for,
	 *	and we use only virtual addresses.
	 */

	vm_page_bootstrap(&start, &end);

	/*
	 *	Initialize other VM packages
	 */

	zone_bootstrap();
	vm_object_bootstrap();
	vm_map_init();
	kmem_init(start, end);
	pmap_init();
	zone_init((vm_size_t)ptoa(vm_page_free_count));
	kalloc_init();
#if	MACH_RT
	rtalloc_init();
#endif	/* MACH_RT */
	vm_fault_init();
	vm_page_module_init();
	memory_manager_default_init();
}

void
vm_mem_init(void)
{
	vm_object_init();
}
