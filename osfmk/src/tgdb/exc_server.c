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

#include <mach.h>
#include <tgdb.h>
#include <mach/notify_server.h>
#include <mach/exc_server.h>


/*
 * Maximum message size for mach_msg_server.
 */
mach_msg_size_t tgdb_msg_size = 128;

extern mach_port_t tgdb_wait_port_set;
extern tgdb_session_t tgdb_lookup_task(mach_port_t task);

boolean_t
tgdb_demux(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP)
{
	if(exc_server(InHeadP, OutHeadP)) {}
	else if(notify_server(InHeadP, OutHeadP)) {}
	else {
		printf("tgdb_demux: bad message\n");
		return FALSE;
	}
	return TRUE;
}
		
/*
 * A kernel thread awakened by receipt of a packet from a remote gdb.  This
 * thread can tolerate page faults while reading user data.
 *
 * This thread should be started before any tasks are started.
 */
void
tgdb_thread(void)
{
	kern_return_t kr;

	while(TRUE) {
	    kr = mach_msg_server(tgdb_demux,
				 tgdb_msg_size,
				 tgdb_wait_port_set,
				 MACH_MSG_OPTION_NONE);
	    printf("mach_msg_server return %x !!\n", kr);
    }
}
		
kern_return_t catch_exception_raise_state(
	mach_port_t exception_port,
	exception_type_t exception,
	exception_data_t code,
	mach_msg_type_number_t codeCnt,
	int *flavor,
	thread_state_t old_state,
	mach_msg_type_number_t old_stateCnt,
	thread_state_t new_state,
	mach_msg_type_number_t *new_stateCnt
)
{
	printf("tgdb: catch_exception_raise_state\n");
	return KERN_SUCCESS;
}

kern_return_t catch_exception_raise_state_identity(
	mach_port_t exception_port,
	mach_port_t thread,
	mach_port_t task,
	exception_type_t exception,
	exception_data_t code,
	mach_msg_type_number_t codeCnt,
	int *flavor,
	thread_state_t old_state,
	mach_msg_type_number_t old_stateCnt,
	thread_state_t new_state,
	mach_msg_type_number_t *new_stateCnt
)
{
	printf("tgdb: catch_exception_raise_state\n");
	return KERN_SUCCESS;
}

kern_return_t
do_mach_notify_port_deleted(
	mach_port_t	port,
	mach_port_t	name)			 
{
	printf("tgdb: got a mach_notify_port_deleted\n");
	return(KERN_FAILURE);
}

kern_return_t
do_mach_notify_port_destroyed(
	mach_port_t	port,
	mach_port_t	rcv_right)
{
	printf("tgdb: got a mach_notify_port_destroyed\n");
	return(KERN_FAILURE);
}

kern_return_t
do_mach_notify_no_senders(
	mach_port_t		port,
	mach_port_mscount_t 	count)
{
	printf("tgdb: got a mach_notify_no_senders\n");
	return(KERN_FAILURE);
}

kern_return_t
do_mach_notify_send_once(
	mach_port_t	port)
{
	printf("tgdb: got a mach_notify_send_once\n");
	return(KERN_FAILURE);
}

kern_return_t
do_mach_notify_dead_name(
	mach_port_t	port,
	mach_port_t	name)			 
{
	tgdb_session_t session;
	
	session = tgdb_lookup_task(name);

	if (!session)
		return(KERN_FAILURE);
	session->task = 0;
	tgdb_detach(session);
	return(KERN_SUCCESS);
}

