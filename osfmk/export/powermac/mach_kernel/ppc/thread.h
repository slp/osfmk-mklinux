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
 *	File:	machine/thread.h
 *
 *	This file contains the structure definitions for the thread
 *	state as applied to PPC processors.
 */

#ifndef	_PPC_THREAD_H_
#define _PPC_THREAD_H_

#include <mach/machine/vm_types.h>

/*
 * Return address of the function that called current function, given
 *	address of the first parameter of current function. We can't
 *      do it this way, since parameter was copied from a register
 *      into a local variable. Call an assembly sub-function to 
 *      return this.
 */

extern vm_offset_t getrpc(void);
#define	GET_RETURN_PC(addr)	getrpc()

#define STACK_IKS(stack)		\
	((struct ppc_kernel_state *)(((vm_offset_t)stack)+KERNEL_STACK_SIZE)-1)

#define syscall_emulation_sync(task) /* do nothing */

/*
 * Defining this indicates that MD code will supply an exception()
 * routine, conformant with kern/exception.c (dependency alert!)
 * but which does wonderfully fast, machine-dependent magic.
 */

#define MACHINE_FAST_EXCEPTION 1

#endif	/* _PPC_THREAD_H_ */


