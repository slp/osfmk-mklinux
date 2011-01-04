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
 *	Machine-independent include file for RPC services.
 */	
#ifndef _RPC_SERVICES_H
#define _RPC_SERVICES_H

/*
 * Cache result below.
 */
extern boolean_t __in_kernel;
extern boolean_t __in_kernel_init;

/*
 * Check for being collocated.
 */
#define in_kernel_address(addr)	(				\
	(__in_kernel) ? 1 : 					\
	(__in_kernel_init) ? 0 : 				\
	(__in_kernel_init = TRUE, 				\
		__in_kernel = (VM_MAX_ADDRESS < (unsigned long)(addr))))

extern char etext;
#define in_kernel in_kernel_address(&etext)

#include <mach/rpc.h>
#include <machine/rpc_services.h>

#endif /* _RPC_SERVICES_H */
