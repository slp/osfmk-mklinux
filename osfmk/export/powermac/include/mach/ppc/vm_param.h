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
#ifndef	_MACH_PPC_VM_PARAM_H_
#define _MACH_PPC_VM_PARAM_H_

#define BYTE_SIZE	8	/* byte size in bits */

#define PPC_PGBYTES	4096	/* bytes per ppc page */
#define PPC_PGSHIFT	12	/* number of bits to shift for pages */

#define VM_MIN_ADDRESS	((vm_offset_t) 0)
#define VM_MAX_ADDRESS	((vm_offset_t) 0xfffff000U)

#define KERNELBASE_TEXT   	0x200000	/* Virt addr of kernel text */
#define KERNELBASE_TEXT_OFFSET	0x0		/* diff to physical addr*/

#define KERNELBASE_DATA   0x400000		/* Virt addr of kernel data */
#define KERNELBASE_DATA_OFFSET 0

#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t) 0x00001000)

/* We map the kernel using only SR0,SR1,SR2 leaving segments alone */
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t) 0x2fffffff)

#define USER_STACK_END  ((vm_offset_t) 0xffff0000U)

#define ppc_round_page(x)	((((unsigned)(x)) + PPC_PGBYTES - 1) & \
					~(PPC_PGBYTES-1))
#define ppc_trunc_page(x)	(((unsigned)(x)) & ~(PPC_PGBYTES-1))

#if	MACH_KERNEL
#define KERNEL_STACK_SIZE	(PPC_PGBYTES)
#define INTSTACK_SIZE		(5 * PPC_PGBYTES)
#endif	/* MACH_KERNEL */

#ifndef ASSEMBLER

#define VM_MIN_KERNEL_LOADED_ADDRESS	((vm_offset_t)  0x10000000U)
#define VM_MAX_KERNEL_LOADED_ADDRESS	((vm_offset_t)  0x2FFFFFFFU)

#else /* ASSEMBLER */

#define VM_MIN_KERNEL_LOADED_ADDRESS	0x10000000
#define VM_MAX_KERNEL_LOADED_ADDRESS	0x2FFFFFFF

#endif /* ASSEMBLER */

#endif	/* _PPC_VM_PARAM_H_ */
