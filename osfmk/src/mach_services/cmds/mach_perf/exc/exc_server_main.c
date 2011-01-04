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

#include <mach_perf.h>
#include <exc_types.h>

boolean_t	exc_server();
boolean_t 	exc_demux();

extern int banner;

mach_port_t simple_port, port_set, server_port;

jmp_buf saved_state;

exc_server_main(service_port)
mach_port_t service_port;
{
	char **argv;
	int argc = 0;

	if (debug > 1)
		printf("calling test_init\n");

	test_init();

	if (debug > 1)
		printf("getting port %x\n", service_port);

	if (service_port) {
		server_port = service_port;
	} else {
		MACH_CALL( mach_port_allocate, (mach_task_self(),
						MACH_PORT_RIGHT_RECEIVE,
						&server_port));
		if (debug > 1)
			printf("server port %x\n", server_port);
		MACH_CALL (netname_check_in, (name_server_port,
					      EXC_SERVER_NAME,
					      mach_task_self(),
					      server_port));
	}

	if (debug > 1)
		printf("calling mach_msg_server: server port %x\n",
		       server_port);

	simple_port = server_port;
	/*
	 *	Announce that we will service on this port.
	 */
	server_announce_port(server_port);
	test_end();

	while (1) {
		thread_malloc_state_t mallocs;

		mallocs = save_mallocs(thread_self());
		if (mach_setjmp(saved_state))
			restore_mallocs(thread_self(), mallocs);
		else {
			MACH_CALL( mach_msg_server,
				  (exc_demux,
				   EXC_MSG_BUF_SIZE,
				   server_port,
				   MACH_MSG_OPTION_NONE));
		}
	}
}

boolean_t exc_demux(in, out)
{
	if (exc_server(in, out))
		return(TRUE);
	else
	    return(server_server(in, out));
}
