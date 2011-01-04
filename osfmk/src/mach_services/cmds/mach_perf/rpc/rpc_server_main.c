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
 *	File:	rpc_server_main.c
 *	Author:	Yves Paindaveine
 *
 *	RPC test server.
 */	

#include <mach_perf.h>
#include <rpc_types.h>
#include <rpc_server.h>

#define NULL_ACCESS 0
#define READ_ACCESS 1
#define WRITE_ACCESS 2

int rpc_debug;

#define			rpc_server_port	server_port
mach_port_t		rpc_server_port;
mach_port_t		rpc_subsystem_port;
mach_port_t		rpc_server_activation;

static vm_offset_t	stack;
static vm_size_t	stack_size;

boolean_t
rpc_demux(in, out)
	mach_msg_header_t in;
	mach_msg_header_t out;
{
	/*
	 * We still receive plain Mach messages for intertask
	 * communications.
	 */
	if (server_server(in,out))
		return TRUE;

	printf("rpc_server_main: unexpected messages on rpc_demux (0x%x, 0x%x)\n",
	       in, out);
	return FALSE;
}

int
rpc_server_main(service_port)
	mach_port_t service_port;
{
	char **argv;
	int argc = 0;

	rpc_debug = debug;

	test_init();

	/*
	 * We never used the port passed in argument since
	 * it is a mere receive right not associated with
	 * any subsystem.
	 */
	if (MACH_PORT_VALID(service_port))
		MACH_CALL( mach_port_destroy, (mach_task_self(),
					       service_port));

	/*
	 *	Register our server subsystem with the kernel for upcalls.
	 */
	MACH_CALL( mach_subsystem_create, (mach_task_self(),
				   (user_subsystem_t) &s_rpc_subsystem,
				   sizeof(s_rpc_subsystem),
				   &rpc_subsystem_port));
	
	if (rpc_debug > 1)
		printf("rpc_server_main: subsystem port 0x%x\n",
		       rpc_subsystem_port);

	/*
	 *	Allocate a port for this object.
	 */
	MACH_CALL( mig_mach_port_allocate_subsystem, (mach_task_self(), 
						      rpc_subsystem_port,
						      &rpc_server_port));

	/*
	 *	Allocate a stack for the server thread.
 	 */
	stack_size = vm_page_size * 8;
	MACH_CALL( vm_allocate, (mach_task_self(),
				 &stack, stack_size, TRUE));

	if (rpc_debug > 1)
		printf("rpc_server_main: thread stack 0x%x(size 0x%x)\n",
		       stack + stack_size, stack_size);

	/*
	 *	Create an empty thread_activation to receive incoming
	 *	server requests.
	 *
	 *	Warning: implicit assumption of stack growing downwards.
	 */
	MACH_CALL( thread_activation_create, (mach_task_self(),
					      rpc_server_port,
					      stack + stack_size,
					      stack_size, 
					      &rpc_server_activation));
#if	!MONITOR
	/*
         *      Now check it into the name service.
	 *	No signature port means this is unprotected.
	 *	XXX netname server not yet implemented in standalone version.
         */
        MACH_CALL( netname_check_in, (name_server_port,
				      RPC_SERVER_NAME,
				      MACH_PORT_NULL,
				      rpc_server_port));

	if (rpc_debug > 1)
		printf("rpc_server_main: check in '%s' port 0x%x\n",
		       RPC_SERVER_NAME, rpc_server_port);
#endif	/* !MONITOR */

	/*
	 *	Announce that we will service on this port.
	 */
	server_announce_port(rpc_server_port);

	if (rpc_debug > 1)
		printf("rpc_server_main: calling mach_msg_server on server port %x\n",
		       rpc_server_port);

	test_end();

	/*
	 *	Main loop
	 */
	while (1) {
		MACH_CALL( mach_msg_server,
			  (rpc_demux,
			   RPC_MSG_BUF_SIZE,
			   rpc_server_port,
			   MACH_MSG_OPTION_NONE));
	}
}


kern_return_t
s_null_rpc(server)
	mach_port_t	server;
{
  	server_count--;
	return_from_rpc(KERN_SUCCESS);
}

kern_return_t
s_make_send_rpc(server, port)
mach_port_t	server, port;
{
	server_count--;
	return_from_rpc(KERN_SUCCESS);
}

kern_return_t
s_copy_send_rpc(server, port)
mach_port_t	server, port;
{
	server_count--;
	return_from_rpc(KERN_SUCCESS);
}

kern_return_t
s_inline_128_rpc(server, data)
mach_port_t	server;
inline_128_t	data;
{
	server_count--;
	return_from_rpc(KERN_SUCCESS);
}

kern_return_t
s_inline_1024_rpc(server, data)
mach_port_t	server;
inline_1024_t	data;
{
	server_count--;
	return_from_rpc(KERN_SUCCESS);
}

kern_return_t
s_inline_4096_rpc(server, data)
mach_port_t	server;
inline_4096_t	data;
{
	server_count--;
	return_from_rpc(KERN_SUCCESS);
}

kern_return_t
s_inline_8192_rpc(server, data)
mach_port_t	server;
inline_8192_t	data;
{
	server_count--;
	return_from_rpc(KERN_SUCCESS);
}

kern_return_t
s_ool_rpc(server, touch, data, data_count)
mach_port_t		server;
int			touch;
ool_t			data;
mach_msg_type_number_t  data_count;
{
	int val; 
	if (touch) {
		ool_t	data_ptr = data;
		int 	count = data_count;

		if ((unsigned char)*data != 0xaa)
		  	printf("ool_rpc receives corrupted data (%x) = %x\n",
					data, *data);
		if (rpc_debug && data_count < 2048 &&
		    (unsigned char) *(data+2048) == 0xbb)
			printf("ool_rpc receives unexpected data (%x) = %x\n",
					data+2048, *(data+2048));
		for (data = data_ptr, count - data_count;
		     count > 0;
		     count -= vm_page_size, data_ptr += vm_page_size) {
			if (touch == READ_ACCESS)
				val = *data_ptr;
			else
				*data_ptr = touch;
		}
	}
	MACH_CALL( vm_deallocate, (mach_task_self(), (vm_offset_t) data, data_count));
	server_count--;
	return_from_rpc(KERN_SUCCESS);
}

kern_return_t
s_rpc_port_cleanup(server, port)
mach_port_t server, port;
{
	MACH_CALL(mach_port_destroy, (mach_task_self(), port));
	return_from_rpc(KERN_SUCCESS);
}
