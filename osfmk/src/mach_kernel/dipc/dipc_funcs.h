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

/*
 *	File:	dipc/dipc_funcs.c
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Public interfaces to DIPC.
 */

#include <mach/message.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_kmsg.h>
#include <dipc/dipc_types.h>
#include <dipc/dipc_kmsg.h>

extern void		dipc_kmsg_free(struct ipc_kmsg	*kmsg);

extern boolean_t	dipc_node_is_valid(node_name	node);

extern node_name	dipc_node_self(void);
extern node_name	dipc_port_destination(ipc_port_t	port);
extern boolean_t	dipc_is_remote(	ipc_port_t	port);

extern void		dipc_bootstrap(void);

extern dipc_return_t	dipc_pull_kmsg(
				meta_kmsg_t		mkmsg,
				ipc_kmsg_t		*kmsgp);

extern mach_msg_return_t dipc_mqueue_send(
				struct ipc_kmsg		*kmsg,
				mach_msg_option_t	option,
				mach_msg_timeout_t	timeout);

extern dipc_return_t	dipc_callback_sender(
				ipc_port_t		port);

extern dipc_return_t	dipc_port_dnrequest_init(
						 ipc_port_t	port);

extern void 		dipc_port_dnnotify(
					   ipc_port_t	port,
					   node_name	node);

extern void		dipc_meta_kmsg_free(
				meta_kmsg_t	mkmsg);

extern void		dipc_machine_init(void);

extern void		dipc_ast(void);

/* external variables */

extern ipc_port_t	dipc_kernel_port;
