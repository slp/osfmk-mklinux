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
 *	File:	rpc_services.h
 *	Author:	Yves Paindaveine
 *
 *	Machine-dependent include file for RPC services.
 */	

#ifndef _AT386_RPC_SERVICES_H
#define _AT386_RPC_SERVICES_H

/*
 * Return to caller appropriately (according to whether or
 * not collocated).  Stash the return code in a caller-saves
 * register, so that the function return executed below (currently
 * only in collocated case) can't overwrite the code when
 * restoring callee-saves registers .
 */
#define return_from_rpc(v) 						\
do {	 								\
									\
	if (in_kernel) { 					\
		__asm__ volatile ("movl %0, %%ecx" : : "g" (v) ); 	\
		return; 						\
	}								\
	else { 								\
		__asm__ volatile ("movl %0, %%ecx" : : "g" (v) ); 	\
		mach_rpc_return_trap(); 				\
	}								\
	/* NOTREACHED */ 						\
} while (0)

#endif /* _AT386_RPC_SERVICES_H */
