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

#ifndef	_IPC_MIG_H_
#define	_IPC_MIG_H_

#include <mach/message.h>
#include <kern/thread.h>
#include <ipc/ipc_thread.h>

/* Send a message from the kernel */
extern mach_msg_return_t mach_msg_send_from_kernel(
	mach_msg_header_t	*msg,
	mach_msg_size_t		send_size);

/* Send a message from the kernel and receive a reply */
extern mach_msg_return_t mach_msg_rpc_from_kernel(
	mach_msg_header_t	*msg,
	mach_msg_size_t		send_size,
	mach_msg_size_t		rcv_size);

/* Destroy the thread's ith_rpc_reply port */
extern void mach_msg_abort_rpc(
	ipc_thread_t	thread);

/* In kernel version of mach_msg_overwrite_trap */
extern mach_msg_return_t mach_msg_overwrite(
	mach_msg_header_t	*msg,
	mach_msg_option_t	option,
	mach_msg_size_t		send_size,
	mach_msg_size_t		rcv_size,
	mach_port_t		rcv_name,
	mach_msg_timeout_t	timeout,
	mach_port_t		notify,
	mach_msg_header_t	*rcv_msg,
        mach_msg_size_t		rcv_msg_size);

#endif	/* _IPC_MIG_H_ */
