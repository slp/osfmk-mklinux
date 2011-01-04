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
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * MkLinux
 */
/*
 *	File:	mig_loop.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 */

#include <mach.h>
#include <cthreads.h>
#include <mach/message.h>
#include <mach/mach_param.h>
#include <mig_errors.h>
#include "xmm_obj.h"

typedef struct max_msg {
	mach_msg_header_t header;
	char data[8192 - sizeof(mach_msg_header_t)];
} *max_msg_t;

extern struct mutex master_mutex;
struct mutex mig_mutex;

kern_return_t
xmm_mig_loop(port, routine, name)
	mach_port_t port;
	int (*routine)();
	char *name;
{
	struct max_msg in_msg, out_msg;
	kern_return_t kr;
	boolean_t success;
	
	mutex_init(&mig_mutex);	/* XXX beware of multiple threads */
	master_lock();
	for (;;) {
		master_unlock();
		in_msg.header.msgh_local_port = port;
		in_msg.header.msgh_size = sizeof(in_msg);
		mutex_lock(&mig_mutex);
		kr = mach_msg_receive(&in_msg.header, MACH_MSG_OPTION_NONE, 0);
		if (kr != MACH_MSG_SUCCESS) {
			return kr;
		}
		master_lock();
		mutex_unlock(&mig_mutex);
		success = (*routine)(&in_msg.header, &out_msg.header);
		if (! success) {
			out_msg.header.msgh_size = sizeof(out_msg.header);
			out_msg.header.msgh_id = 0;
			out_msg.header.msgh_local_port = MACH_PORT_NULL;
			kr = mach_msg_send(&out_msg.header, MACH_MSG_OPTION_NONE,0);
			printf("%s mig loop: invalid msg id %d\n",
			       name, in_msg.header.msgh_id);
			mach_error("death pill", kr);
			continue;
		}
		if (out_msg.header.msgh_remote_port != MACH_PORT_NULL) {
			kr = mach_msg_send(&out_msg.header, MACH_MSG_OPTION_NONE,0);
			if (kr != MACH_MSG_SUCCESS) {
				mach_error("mach_msg_send", kr);
				printf("%s mig loop: msgh_id %d port 0x%x\n",
				       name,
				       in_msg.header.msgh_id,
				       out_msg.header.msgh_remote_port);
			}
		}
	}
}

mach_port_t xmm_port_set = MACH_PORT_NULL;

xmm_mig_init()
{
	if (xmm_port_set == MACH_PORT_NULL) {
		return mach_port_allocate(mach_task_self(),
					  MACH_PORT_RIGHT_PORT_SET,
					  &xmm_port_set);
	} else {
		return KERN_SUCCESS;
	}
}

the_mig_loop()
{
	extern obj_server();
	kern_return_t kr;

	kr = xmm_mig_loop(xmm_port_set, obj_server, "obj_server");
	mach_error("xmm_mig_loop", kr);
	cthread_exit((any_t) 1);
}

mach_port_get(port_name)
	mach_port_t *port_name;
{
	extern mach_port_t xmm_port_set;
	kern_return_t kr;
	
	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				port_name);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	kr = mach_port_insert_right(mach_task_self(), *port_name,
				    *port_name, MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	kr = mach_port_move_member(mach_task_self(), *port_name,
				   xmm_port_set);
	if (kr != KERN_SUCCESS) {
		return MACH_PORT_NULL;
	}
	return KERN_SUCCESS;
}
