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

#ifndef _KERN_IPC_SYNC_H_
#define _KERN_IPC_SYNC_H_

#include <ipc/ipc_types.h>
#include <kern/sync_sema.h>
#include <kern/sync_lock.h>

semaphore_t convert_port_to_semaphore (ipc_port_t port);
ipc_port_t  convert_semaphore_to_port (semaphore_t semaphore);

lock_set_t  convert_port_to_lock_set  (ipc_port_t port);
ipc_port_t  convert_lock_set_to_port  (lock_set_t lock_set);

#endif /* _KERN_IPC_SYNC_H_ */
