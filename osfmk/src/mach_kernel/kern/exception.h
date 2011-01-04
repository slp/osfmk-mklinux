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

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <ipc/ipc_port.h>
#include <ipc/ipc_kmsg.h>
#include <mach/exception.h>

/* Make an up-call to a thread's exception server */
extern void exception(
	exception_type_t	exception,
	exception_data_t	code,
	mach_msg_type_number_t	codeCnt);

/* Continue after blocking for an exception */
extern void exception_raise_continue(void);

/* Special-purpose fast continuation for exceptions */
extern void exception_raise_continue_fast(
	ipc_port_t		reply_port,
	ipc_kmsg_t		kmsg,
	mach_msg_type_number_t	codeCnt	);

/*
 * These must be implemented in machine dependent code
 */

/* Restart syscall */
extern void restart_mach_syscall(void);

/* Make an up-call to a thread's alert handler */
extern kern_return_t alert_exception(
	exception_type_t	exception,
	exception_data_t	code,
	int			codeCnt	);

#endif	/* _EXCEPTION_H_ */
