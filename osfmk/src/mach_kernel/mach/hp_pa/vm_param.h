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
 * Copyright (c) 1990, 1991, 1992, The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: vm_param.h 1.10 92/05/22$
 */

#ifndef	_MACH_HP700_VM_PARAM_H_
#define _MACH_HP700_VM_PARAM_H_

#define BYTE_SIZE	8	/* byte size in bits */

#define HP700_PGBYTES	4096	/* bytes per hp700 page */
#define HP700_PGSHIFT	12	/* number of bits to shift for pages */

/*
 *	Convert bytes to pages and convert pages to bytes.
 *	No rounding is used.
 */

#define hp700_btop(x)		(((unsigned)(x)) >> HP700_PGSHIFT)
#define machine_btop(x)		hp700_btop(x)
#define hp700_ptob(x)		(((unsigned)(x)) << HP700_PGSHIFT)

#define VM_MIN_ADDRESS	((vm_offset_t) 0)
#define VM_MAX_ADDRESS	((vm_offset_t) 0xfffff000U)
#define USER_STACK_END  ((vm_offset_t) 0xc0000000U)

#define hp700_round_page(x)	((((unsigned)(x)) + HP700_PGBYTES - 1) & \
					~(HP700_PGBYTES-1))
#define hp700_trunc_page(x)	(((unsigned)(x)) & ~(HP700_PGBYTES-1))

#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t) 0x1000)
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t) 0xef000000)

#if	MACH_KERNEL
#include <norma_vm.h>
#include <xkmachkernel.h>

#if	!NORMA_VM && !XKMACHKERNEL
#if	DEBUG
#define KERNEL_STACK_SIZE	(2 * HP700_PGBYTES) /* -g ==> larger frames */
#else	/* MACH_ASSERT */
#define KERNEL_STACK_SIZE	(1 * HP700_PGBYTES)
#endif	/* MACH_ASSERT */
#else	/* !NORMA_VM  && !XKMACHKERNEL */
#define KERNEL_STACK_SIZE	(8 * HP700_PGBYTES)
#endif	/* !NORMA_VM && !XKMACHKERNEL */
#define INTSTACK_SIZE		(5 * HP700_PGBYTES)
#endif	/* MACH_KERNEL */

#ifndef ASSEMBLER
#define VM_MIN_KERNEL_LOADED_ADDRESS	((vm_offset_t)  0x1FE00000U)
#else /* ASSEMBLER */
#define VM_MIN_KERNEL_LOADED_ADDRESS	0x1FE00000
#endif /* ASSEMBLER */

#define VM_MAX_KERNEL_LOADED_ADDRESS   (VM_MIN_KERNEL_LOADED_ADDRESS + (512 * 1024 * 1024))

#endif	/* _HP700_VM_PARAM_H_ */





