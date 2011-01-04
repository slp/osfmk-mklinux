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
 * 
 */
/*
 * MkLinux
 */
#ifndef	_MACH_RPC_COMMON_H_
#define _MACH_RPC_COMMON_H_

#include <mach_debug.h>
#include <dipc.h>
#include <cpus.h>

/*
 * This structure defines the elements in common between
 * ports and port sets.  The ipc_common_data MUST BE FIRST here,
 * just as the ipc_object must be first within that.
 *
 * This structure must be used anywhere an ipc_object needs
 * to be locked.
 */
typedef struct rpc_common_data {
	struct ipc_common_data	rcd_comm;
	struct thread_pool	rcd_thread_pool;
#if	DIPC && NCPUS == 1
	usimple_lock_data_t	rcd_io_lock_data;
#else	/* DIPC */
	decl_mutex_data(,	rcd_io_lock_data)
#endif	/* DIPC */
} *rpc_common_t;

#endif	/* _MACH_RPC_COMMON_H_ */
