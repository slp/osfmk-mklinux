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
#include <ipc_types.h>

boolean_t	ipc_server();
boolean_t 	ipc_demux();

extern int rename_server_ports;
extern int rename_member_ports;
extern int sparse_factor;

extern mach_port_t *allocate_ports();
mach_port_t *port_list;
mach_port_t last_port_name;

#define NULL_ACCESS 0
#define READ_ACCESS 1
#define WRITE_ACCESS 2

mach_port_t simple_port, port_set, server_port;

jmp_buf saved_state;

int ipc_debug;

ipc_server_main(service_port)
mach_port_t service_port;
{
	char **argv;
	int argc = 0;

	if (debug > 1)
		printf("calling test_init\n");
	test_init();

	if (debug > 1)
		printf("getting port %x\n", service_port);
	if (rename_server_ports) {
		port_list = allocate_ports(rename_server_ports, 1);
		server_port = port_list[rename_server_ports/2];
	} else {
		/* try to use a generic port name */

		MACH_CALL( mach_port_allocate, (mach_task_self(),
						MACH_PORT_RIGHT_RECEIVE,
						&server_port));
		if (debug > 1)
			printf("generic port %x\n", server_port);
	}
	if (service_port) {
		MACH_CALL( mach_port_destroy, (mach_task_self(),
					       server_port));
		MACH_CALL( mach_port_rename, (mach_task_self(),
					      service_port, server_port));
	} else {
		MACH_CALL (netname_check_in, (name_server_port,
					      IPC_SERVER_NAME,
					      mach_task_self(),
					      server_port));
	}

	if (debug > 1)
		printf("calling mach_msg_server: server port %x\n",
		       server_port);
	ipc_debug = debug;
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
				  (ipc_demux,
				   IPC_MSG_BUF_SIZE,
				   server_port,
				   MACH_MSG_OPTION_NONE));
		}
	}
}

int do_null_snd_rcv(server)
mach_port_t	server;
{
  	server_count--;
	return;
}

int do_null_ipc(server)
mach_port_t	server;
{
  	server_count--;
	return;
}

int do_make_send_ipc(server, port)
mach_port_t	server, port;
{
	server_count--;
	return;
}

int do_copy_send_ipc(server, port)
mach_port_t	server, port;
{
	server_count--;
	return;
}

int do_inline_128_ipc(server, data)
mach_port_t	server;
inline_128_t	data;
{
	server_count--;
	return;
}

int do_inline_1024_ipc(server, data)
mach_port_t	server;
inline_1024_t	data;
{
	server_count--;
	return;
}

int do_inline_4096_ipc(server, data)
mach_port_t	server;
inline_4096_t	data;
{
	server_count--;
	return;
}

int do_inline_8192_ipc(server, data)
mach_port_t	server;
inline_8192_t	data;
{
	server_count--;
	return;
}

int do_ool_ipc(server, touch, data, data_count)
mach_port_t	server;
ool_t		data;
int		data_count;
{
	int val; 
	if (touch) {
		ool_t	data_ptr = data;
		int 	count = data_count;

		if ((unsigned char)*data != 0xaa)
		  	printf("ool_ipc receives corrupted data (%x) = %x\n",
					data, *data);
		if (ipc_debug && data_count < 2048 &&
		    (unsigned char) *(data+2048) == 0xbb)
			printf("ool_ipc receives unexpected data (%x) = %x\n",
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
	return;
}

create_port_set()
{
	if (!port_set) {
		if (rename_server_ports) {
			port_set = port_list[rename_server_ports/2+1];
			MACH_CALL(mach_port_destroy,
				  (mach_task_self(),
				   port_set));
			MACH_CALL(mach_port_allocate_name, 
				  (mach_task_self(),
				   MACH_PORT_RIGHT_PORT_SET,
				   port_set));
			last_port_name = port_list[rename_server_ports-1];
		} else
			MACH_CALL( mach_port_allocate,
				  (mach_task_self(),
				   MACH_PORT_RIGHT_PORT_SET,
				   &port_set));
	}
}

add_to_set(server, port)
mach_port_t server, port;
{
	mach_port_t task = mach_task_self();

	if (rename_member_ports) {
		last_port_name += sparse_factor;
		MACH_CALL(mach_port_rename, (task, port, last_port_name));
		port = last_port_name;
	}
	MACH_CALL( mach_port_move_member, (task, port, port_set));
	return(0);
}

do_add_to_port_set(server, ports, count)
mach_port_t server, *ports;
{
	mach_port_t task = mach_task_self();
	mach_port_t *port;
	register i;

	if (!port_set)
		create_port_set();
	for(i=0, port = ports; i < count; i++, port++)
		add_to_set(server, *port);
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )ports,
				  count*sizeof(mach_port_t)));
	return(0);
}

do_enable_port_set()
{
	if (port_set) {
		server_port = port_set;
		mach_longjmp(saved_state, 1);
	}
	return(0);
}

do_disable_port_set()
{
	if (port_set)
		clean_port_set();
	if (simple_port) {
		server_port = simple_port;
		mach_longjmp(saved_state, 1);
	}
	return(0);
}

clean_port_set()
{
	mach_port_t *ports, *port;
	mach_msg_type_number_t  port_count, i;

	if (debug > 1)
		printf("clean_port_set\n");

	if (!port_set)
		return;
	MACH_CALL( mach_port_get_set_status, (mach_task_self(),
					      port_set,
					      &ports,
					      &port_count));
	for (port=ports, i = 0; i < port_count; port++, i++) {
		MACH_CALL( mach_port_destroy, (mach_task_self(), *port));
	}
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )ports,
				  port_count*sizeof(mach_port_t)));
	MACH_CALL(mach_port_destroy, (mach_task_self(), port_set));
	port_set = MACH_PORT_NULL;
}

do_port_cleanup(server, port)
mach_port_t server, port;
{
	MACH_CALL(mach_port_destroy, (mach_task_self(), port));
	return(0);
}


boolean_t ipc_demux(in, out)
{
	if (ipc_server(in, out))
		return(TRUE);
	else
	    return(server_server(in, out));
}


