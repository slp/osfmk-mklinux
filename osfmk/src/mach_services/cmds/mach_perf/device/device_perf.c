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


#define TEST_NAME	device


#include <mach_perf.h>

int device_open_test(void);
int device_open_request_test(boolean_t do_reply);
int device_close_test(void);
int device_read_test(int size, boolean_t dealloc, boolean_t inband);
int device_read_request_test(int size, boolean_t do_reply, boolean_t dealloc, boolean_t inband);
int device_read_reply_test(int size, boolean_t dealloc, boolean_t inband, boolean_t synchronous);
int device_read_overwrite_test(int size);
int device_read_request_overwrite_test(int size, boolean_t do_reply);
int device_read_reply_overwrite_test(int size, boolean_t synchronous);
int device_write_test(int size, boolean_t inband);
int device_write_request_test(int size, boolean_t do_reply, boolean_t inband);
int device_write_reply_test(int size, boolean_t inband, boolean_t synchronous);
int device_get_status_test(void);
int device_set_status_test(void);
int device_map_test(int size);
int device_set_filter_test(void);
int device_net_receive_test(int pkt_size, int filter_size, boolean_t synchronous);

char *private_options = "\n\
";

struct test tests[] = {
"Control operations",		0, 0, 0, 0, 0, 0,

"device_open",			0, device_open_test, 0, 0, 0, 0,
"device_open_request",		0, device_open_request_test, 0, 0, 0, 0,
"device_open_request+reply",	0, device_open_request_test, 1, 0, 0, 0,
"device_close",			0, device_close_test, 0, 0, 0, 0,
#if	TEST_DEVICE
"device_get_status",		0, device_get_status_test, 0, 0, 0, 0,
"device_set_status",		0, device_set_status_test, 0, 0, 0, 0,
"device_set_filter",		0, device_set_filter_test, 0, 0, 0, 0,
"device_map 1 page",		0, device_map_test, -1, 0, 0, 0,

"1 page I/O",	0, 0, 0, 0, 0, 0,

"device_read",			0, device_read_test, -1, 0, 0, 0,
"device_read/deallocate",	0, device_read_test, -1, 1, 0, 0,
"device_read_request", 		0, device_read_request_test, -1, 0, 0, 0,
"device_read_reply",		0, device_read_reply_test, -1, 0, 0, 0,
"device_read_reply/deallocate",	0, device_read_reply_test, -1, 1, 0, 0,
"device_read_request+reply",	0, device_read_request_test, -1, 1, 0, 0,
"device_read_req+reply+dealloc",0, device_read_request_test, -1, 1, 1, 0,
"device_read_overwrite",	0, device_read_overwrite_test, -1, 0, 0, 0,
"device_read_request_overwrite",0, device_read_request_overwrite_test, -1, 0, 0, 0,
"device_read_reply_overwrite",	0, device_read_reply_overwrite_test, -1, 0, 0, 0,
"device_read_req_overwr+reply", 0, device_read_request_overwrite_test, -1, 1, 0, 0,
"device_write",			0, device_write_test, -1, 0, 0, 0,
"device_write_request",		0, device_write_request_test, -1, 0, 0, 0,
"device_write_reply",		0, device_write_reply_test, -1, 0, 0, 0,
"device_write_request+reply",	0, device_write_request_test, -1, 1, 0, 0,

"Null data transfer I/O",	0, 0, 0, 0, 0, 0,

"device_read",			0, device_read_test, 0, 0, 0, 0,
"device_read_inband",		0, device_read_test, 0, 0, 1, 0,
"device_read_request",		0, device_read_request_test, 0, 0, 0, 0,
"device_read_reply",		0, device_read_reply_test, 0, 0, 0, 0,
"device_read_request_inband",	0, device_read_request_test, 0, 0, 0, 1,
"device_read_reply_inband",	0, device_read_reply_test, 0, 0, 1, 0,
"device_read_overwrite",	0, device_read_overwrite_test, 0, 0, 0, 0,
"device_read_request_overwrite",0, device_read_request_overwrite_test, 0, 0, 0, 0,
"device_read_reply_overwrite",	0, device_read_reply_overwrite_test, 0, 0, 0, 0,
"device_write",			0, device_write_test, 0, 0, 0, 0,
"device_write_request",		0, device_write_request_test, 0, 0, 0, 0,
"device_write_reply",		0, device_write_reply_test, 0, 0, 0, 0,
"device_write_inband",		0, device_write_test, 0, 1, 0, 0,
"device_write_request_inband", 	0, device_write_request_test, 0, 0, 1, 0,
"device_write_reply_inband",	0, device_write_reply_test, 0, 1, 0, 0,

"4 bytes I/O",			0, 0, 0, 0, 0, 0,

"device_read_overwrite",	0, device_read_overwrite_test, 4, 0, 0, 0,
"device_read_req_overwrite", 	0, device_read_request_overwrite_test, 4, 0, 0, 0,
"device_read_repl_overwrite",	0, device_read_reply_overwrite_test, 4, 0, 0, 0,

"INBAND_MAX (128) bytes I/O",	0, 0, 0, 0, 0, 0,

"device_read_inband",		0, device_read_test, IO_INBAND_MAX, 0, 1, 0,
"device_read_req_inband",	0, device_read_request_test, IO_INBAND_MAX, 0, 0, 1,
"device_read_request_inb+reply",0, device_read_request_test, IO_INBAND_MAX, 1, 0, 1,
"device_read_reply_inband",	0, device_read_reply_test, IO_INBAND_MAX, 0, 1, 0,
"device_write_inband",		0, device_write_test, IO_INBAND_MAX, 1, 0, 0,
"device_write_request_inband", 	0, device_write_request_test, IO_INBAND_MAX, 0, 1, 0,
"device_write_reply_inband",	0, device_write_reply_test, IO_INBAND_MAX, 1, 0, 0,
"device_write_req_inband+reply",0, device_write_request_test, IO_INBAND_MAX, 1, 1, 0,

"Sync 1 page I/O",		0, 0, 0, 0, 0, 0,

"device_read_reply",		0, device_read_reply_test, -1, 0, 0, 1,
"device_read_repl_overwrite",	0, device_read_reply_overwrite_test, -1, 1, 0, 0,
"device_write_reply",		0, device_write_reply_test, -1, 0, 1, 0,

"Sync 0 bytes I/O ",		0, 0, 0, 0, 0, 0,

"device_read_reply_inband",	0, device_read_reply_test, 0, 0, 1, 1,
"device_read_reply",		0, device_read_reply_test, 0, 0, 0, 1,
"device_read_repl_overwrite",	0, device_read_reply_overwrite_test, 0, 1, 0, 0,
"device_write_reply",		0, device_write_reply_test, 0, 0, 1, 0,
"device_write_reply_inband",	0, device_write_reply_test, 0, 1, 1, 0,

"Sync INBAND_MAX (128) bytes I/O",	0, 0, 0, 0, 0, 0,

"device_read_reply_inband",	0, device_read_reply_test, IO_INBAND_MAX, 0, 1, 1,
"device_write_reply_inband",	0, device_write_reply_test, IO_INBAND_MAX, 1, 1, 0,

#if 0
"net_receive 0b filter=0",	0, device_net_receive_test, 0, 0, 0, 0,
"net_receive 0b filter=MAX",	0, device_net_receive_test, 0, NET_MAX_FILTER, 0, 0,
"net_receive MAX filter=0",	0, device_net_receive_test, NET_RCV_MAX, 0, 0, 0,
"net_receive MAX filter=MAX",	0, device_net_receive_test, NET_RCV_MAX, NET_MAX_FILTER, 0, 0,
#endif
#if 0
"net_receive 0b filter=0 sync",0, device_net_receive_test, 0, 0, 1, 0,
"net_receive 0b filter=MAX sync",0, device_net_receive_test, 0, NET_MAX_FILTER, 1, 0,
"net_receive MAX filter=0 sync",0, device_net_receive_test, NET_RCV_MAX, 0, 1, 0,
"net_receive MAX filter=MAX sync",0, device_net_receive_test, NET_RCV_MAX, NET_MAX_FILTER, 1, 0,
#endif
#endif	/* TEST_DEVICE */
0, 0, 0, 0, 0, 0, 0
};

int vm_size;

boolean_t global_dealloc;
vm_address_t *global_dealloc_ptr;

main(
	int	argc,
	char	*argv[])
{
	int i;

	test_init();
	for (i = 1; i < argc; i++) 
		if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();
	vm_size = mem_size();

	run_tests(tests);
	test_end();
}

int
device_open_test(void)
{
	mach_port_t	device_port;
	int		i;

	/* device.open_count is a signed short */
	set_max_loops(1 << ((sizeof (short) * 8) - 1) - 2);

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(device_open, (master_device_port,
					MACH_PORT_NULL,
					0,
                                	mach_perf_token,
					"test_device",
					&device_port));
	}
	stop_time();

	for (i = 0; i <= loops; i++) {
		MACH_CALL(device_close, (device_port));
	}

	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
}

int
device_open_request_test(boolean_t do_reply)
{
	mach_port_t		device_port, reply_port;
	int			i;
	union {
		mach_msg_header_t	h;
		char			fill[4096];
	} msg, msg2;
	kern_return_t		kr;

	/* device.open_count is a signed short */
	set_max_loops(1 << ((sizeof (short) * 8) - 1) - 3);

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));
	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &reply_port));

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(device_open_request, (master_device_port,
						reply_port,
						MACH_PORT_NULL,
						0,
                                		mach_perf_token,
						"test_device"));
		if (do_reply) {
			MACH_CALL(mach_msg, (&(msg.h), MACH_RCV_MSG, 0,
					     sizeof msg, reply_port,
					     MACH_MSG_TIMEOUT_NONE,
					     MACH_PORT_NULL));
			if (! device_reply_server(&(msg.h), &(msg2.h))) {
				test_exit(
					  "%s() in %s at line %d: %s\n",
					  "device_reply_server",
					  __FILE__,
					  __LINE__,
					  "failed");
			}
		}
	}
	stop_time();

	MACH_CALL(mach_port_destroy, (mach_task_self(), reply_port));

	for (i = 0; i <= loops; i++) {
		MACH_CALL(device_close, (device_port));
	}

	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
}

int
device_close_test(void)
{
	mach_port_t	device_port;
	int		i;

	/* device.open_count is a short */
	set_max_loops(1 << ((sizeof (short) * 8) - 1) - 2);

	for (i = 0; i <= loops; i++) {
		MACH_CALL(device_open, (master_device_port,
					MACH_PORT_NULL,
					0,
                                	mach_perf_token,
					"test_device",
					&device_port));
	}

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(device_close, (device_port));
	}
	stop_time();

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
}

#if	TEST_DEVICE

int
device_read_test(
	int		size,
	boolean_t	dealloc,
	boolean_t	inband)
{
	mach_port_t		device_port;
	int			i;
	vm_address_t		*data_list, *data;
	unsigned int		data_count;
	io_buf_ptr_inband_t	inband_data;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	if (!inband && !dealloc && size > 0) {
		/* limit #loops to avoid paging */
		set_max_loops(vm_size /
			      ((size + vm_page_size - 1) / vm_page_size));
	}

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &data_list,
				loops * sizeof (vm_address_t),
				TRUE));

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));

	start_time();
	for (i = 0,data = &data_list[0]; i < loops; i++, data++) {
		if (inband) {
			data_count = size;
			MACH_CALL(device_read_inband, (device_port,
						       D_NOWAIT,
						       0,
						       size,
						       inband_data,
						       &data_count));
		} else {
			MACH_CALL(device_read, (device_port,
						D_NOWAIT,
						0,
						size,
						(io_buf_ptr_t *) data,
						&data_count));
			if (dealloc && data_count > 0) {
				MACH_CALL(vm_deallocate, (mach_task_self(),
							  *data,
							  data_count));
			}
		}
	}
	stop_time();

	if (!inband && !dealloc && size > 0) {
		for (i = 0, data = &data_list[0]; i < loops; i++, data++) {
			MACH_CALL(vm_deallocate, (mach_task_self(),
						  *data,
						  size));
		}
	}
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) data_list,
				  loops * sizeof (vm_address_t)));

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
}

int
device_read_request_test(
	int		size,
	boolean_t	do_reply,
	boolean_t	dealloc,
	boolean_t	inband)
{
	mach_port_t	device_port, reply_port;
	int		i;
	vm_address_t	*data_list, *data;
	unsigned int	data_count;
	union {
		mach_msg_header_t	h;
		char			fill[4096];
	} msg, msg2;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	global_dealloc = dealloc;

	if (! dealloc && size > 0) {
		/* limit #loops to avoid paging */
		set_max_loops(vm_size /
			      ((size + vm_page_size - 1) / vm_page_size));
	}

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &data_list,
				loops * sizeof (vm_address_t),
				TRUE));

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));
	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &reply_port));

	start_time();
	for (i = 0, global_dealloc_ptr = &data_list[0];
	     i < loops;
	     i++, global_dealloc_ptr++) {
		if (inband) {
			MACH_CALL(device_read_request_inband,
				  (device_port,
				   reply_port,
				   D_NOWAIT,
				   0,
				   size));
		} else {
			MACH_CALL(device_read_request, (device_port,
							reply_port,
							D_NOWAIT,
							0,
							size));
		}
		if (do_reply) {
			MACH_CALL(mach_msg, (&(msg.h), MACH_RCV_MSG, 0,
					     sizeof msg, reply_port,
					     MACH_MSG_TIMEOUT_NONE,
					     MACH_PORT_NULL));
			if (! device_reply_server(&(msg.h), &(msg2.h))) {
				test_exit(
					  "%s() in %s at line %d: %s\n",
					  "device_reply_server",
					  __FILE__,
					  __LINE__,
					  "failed");
			}
		}
	}
	stop_time();

	if (do_reply && !dealloc && size > 0 && !inband) {
		for (i = 0, data = &data_list[0]; i < loops; i++, data++) {
			MACH_CALL(vm_deallocate, (mach_task_self(),
						  *data,
						  size));
		}
	}
	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) data_list,
				  loops * sizeof (vm_address_t)));

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), reply_port));
}

int
device_read_reply_test(
	int		size,
	boolean_t	dealloc,
	boolean_t	inband,
	boolean_t	synchronous)
{
	mach_port_t	device_port, reply_port;
	int		i;
	union {
		mach_msg_header_t	h;
		char			fill[4096];
	} msg, msg2;
	test_device_status_t	status;
	vm_address_t		*data, *data_list;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	if (size > 0) {
		set_max_loops(((vm_size*vm_page_size) / round_page(size)) / 3);
	} else {
		set_max_loops(vm_size / 3);
	}

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));
	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &reply_port));

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &data_list,
				loops * sizeof (vm_address_t),
				TRUE));

	global_dealloc = dealloc;

	/* tell the driver to start sending reply messages */
	status.reply_port = reply_port;
	if (inband) {
		status.reply_type = DEVICE_TEST_READ_INBAND_REPLY;
	} else {
		status.reply_type = DEVICE_TEST_READ_REPLY;
	}
	status.reply_uaddr = 0;
	status.reply_count = loops;
	status.reply_size = size;
	status.reply_synchronous = synchronous;
	MACH_CALL(device_set_status, (device_port,
				      TEST_DEVICE_GENERATE_REPLIES,
				      (dev_status_t) &status,
				      TEST_DEVICE_STATUS_COUNT));

	start_time();
	for (i = 0, global_dealloc_ptr = &data_list[0];
	     i < loops;
	     i++, global_dealloc_ptr++) {
		MACH_CALL(mach_msg, (&(msg.h), MACH_RCV_MSG, 0,
				     sizeof msg, reply_port,
				     MACH_MSG_TIMEOUT_NONE,
				     MACH_PORT_NULL));
		if (! device_reply_server(&(msg.h), &(msg2.h))) {
			test_exit(
				  "%s() in %s at line %d: %s\n",
				  "device_reply_server",
				  __FILE__,
				  __LINE__,
				  "failed");
		}
	}
	stop_time();

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), reply_port));

	if (! dealloc && ! inband) {
		for (i = 0, data = &data_list[0]; i < loops; i++, data++) {
			MACH_CALL(vm_deallocate, (mach_task_self(),
						  (vm_offset_t) *data,
						  size));
		}
	}
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) data_list,
				  loops * sizeof (vm_address_t)));
}

int
device_read_overwrite_test(
	int		size)
{
	mach_port_t		device_port;
	int			i;
	vm_address_t		data;
	unsigned int		data_count;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &data,
				size,
				TRUE));
	/* workaround for a kernel bug: touch the target memory */
	for (i = 0; i < size; i += vm_page_size) {
		*(((char *)data) + i)  = '0';
	}

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(device_read_overwrite, (device_port,
						  D_NOWAIT,
						  0,
						  size,
						  data,
						  &data_count));
	}
	stop_time();

	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) data, size));
	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
}

int
device_read_request_overwrite_test(
	int		size,
	boolean_t	do_reply)
{
	mach_port_t	device_port, reply_port;
	int		i;
	vm_address_t	data;
	unsigned int	data_count;
	union {
		mach_msg_header_t	h;
		char			fill[4096];
	} msg, msg2;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &data,
				size,
				TRUE));
	/* workaround for a kernel bug: touch the target memory */
	for (i = 0; i < size; i += vm_page_size) {
		*(((char *)data) + i)  = '0';
	}

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));
	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &reply_port));

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(device_read_overwrite_request, (device_port,
							  reply_port,
							  D_NOWAIT,
							  0,
							  size,
							  data));
		if (do_reply) {
			MACH_CALL(mach_msg, (&(msg.h), MACH_RCV_MSG, 0,
					     sizeof msg, reply_port,
					     MACH_MSG_TIMEOUT_NONE,
					     MACH_PORT_NULL));
			if (! device_reply_server(&(msg.h), &(msg2.h))) {
				test_exit(
					  "%s() in %s at line %d: %s\n",
					  "device_reply_server",
					  __FILE__,
					  __LINE__,
					  "failed");
			}
		}
	}
	stop_time();

	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) data, size));
	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), reply_port));
}

int
device_read_reply_overwrite_test(
	int		size,
	boolean_t	synchronous)
{
	mach_port_t	device_port, reply_port;
	int		i;
	union {
		mach_msg_header_t	h;
		char			fill[4096];
	} msg, msg2;
	test_device_status_t	status;
	vm_address_t		data;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	if (size > 0) {
		set_max_loops(((vm_size*vm_page_size) / round_page(size)) / 3);
	} else {
		set_max_loops(vm_size / 3);
	}

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));
	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &reply_port));

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &data,
				size,
				TRUE));

	/* tell the driver to start sending reply messages */
	status.reply_port = reply_port;
	status.reply_type = DEVICE_TEST_READ_OVERWRITE_REPLY;
	status.reply_uaddr = data;
	status.reply_count = loops;
	status.reply_size = size;
	status.reply_synchronous = synchronous;
	MACH_CALL(device_set_status, (device_port,
				      TEST_DEVICE_GENERATE_REPLIES,
				      (dev_status_t) &status,
				      TEST_DEVICE_STATUS_COUNT));

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(mach_msg, (&(msg.h), MACH_RCV_MSG, 0,
				     sizeof msg, reply_port,
				     MACH_MSG_TIMEOUT_NONE,
				     MACH_PORT_NULL));
		if (! device_reply_server(&(msg.h), &(msg2.h))) {
			test_exit(
				  "%s() in %s at line %d: %s\n",
				  "device_reply_server",
				  __FILE__,
				  __LINE__,
				  "failed");
		}
	}
	stop_time();

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), reply_port));

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) data,
				  size));
}

int
device_write_test(
	int		size,
	boolean_t	inband)
{
	mach_port_t		device_port;
	int			i;
	io_buf_ptr_t		data;
	io_buf_ptr_inband_t	inband_data;
	int			bytes_written;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &data,
				size,
				TRUE));

	start_time();
	for (i = 0; i < loops; i++) {
		if (inband) {
			MACH_CALL(device_write_inband, (device_port,
							D_NOWAIT,
							0,
							inband_data,
							size,
							&bytes_written));
		} else {
			MACH_CALL(device_write, (device_port,
						 D_NOWAIT,
						 0,
						 data,
						 size,
						 &bytes_written));
		}
	}
	stop_time();

	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) data, size));
	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
}

int
device_write_request_test(
	int		size,
	boolean_t	do_reply,
	boolean_t	inband)
{
	mach_port_t		device_port, reply_port;
	int			i;
	io_buf_ptr_t		data;
	io_buf_ptr_inband_t	inband_data;
	union {
		mach_msg_header_t	h;
		char			fill[4096];
	} msg, msg2;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));
	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &reply_port));

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &data,
				size,
				TRUE));

	start_time();
	for (i = 0; i < loops; i++) {
		if (inband) {
			MACH_CALL(device_write_request_inband,
				  (device_port,
				   reply_port,
				   D_NOWAIT,
				   0,
				   inband_data,
				   size));
		} else {
			MACH_CALL(device_write_request,
				  (device_port,
				   reply_port,
				   D_NOWAIT,
				   0,
				   data,
				   size));
		}
		if (do_reply) {
			MACH_CALL(mach_msg, (&(msg.h), MACH_RCV_MSG, 0,
					     sizeof msg, reply_port,
					     MACH_MSG_TIMEOUT_NONE,
					     MACH_PORT_NULL));
			if (! device_reply_server(&(msg.h), &(msg2.h))) {
				test_exit(
					  "%s() in %s at line %d: %s\n",
					  "device_reply_server",
					  __FILE__,
					  __LINE__,
					  "failed");
			}
		}
	}
	stop_time();

	MACH_CALL(vm_deallocate, (mach_task_self(), (vm_offset_t) data, size));
	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), reply_port));
}

int
device_write_reply_test(
	int		size,
	boolean_t	inband,
	boolean_t	synchronous)
{
	mach_port_t	device_port, reply_port;
	int		i;
	union {
		mach_msg_header_t	h;
		char			fill[4096];
	} msg, msg2;
	test_device_status_t	status;
	vm_address_t		data;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	if (size > 0) {
		set_max_loops(((vm_size*vm_page_size) / round_page(size)) / 3);
	} else {
		set_max_loops(vm_size / 3);
	}

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));
	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &reply_port));

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *) &data,
				size,
				TRUE));
	/* touch the data to get it allocated */
	for (i = 0; i < size; i += vm_page_size) {
		*(((int *) data) + i) = 1;
	}

	/* tell the driver to start sending reply messages */
	status.reply_port = reply_port;
	if (inband) {
		status.reply_type = DEVICE_TEST_WRITE_INBAND_REPLY;
	} else {
		status.reply_type = DEVICE_TEST_WRITE_REPLY;
	}
	status.reply_uaddr = data;
	status.reply_count = loops;
	status.reply_size = size;
	status.reply_synchronous = synchronous;
	MACH_CALL(device_set_status, (device_port,
				      TEST_DEVICE_GENERATE_REPLIES,
				      (dev_status_t) &status,
				      TEST_DEVICE_STATUS_COUNT));

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(mach_msg, (&(msg.h), MACH_RCV_MSG, 0,
				     sizeof msg, reply_port,
				     MACH_MSG_TIMEOUT_NONE,
				     MACH_PORT_NULL));
		if (! device_reply_server(&(msg.h), &(msg2.h))) {
			test_exit(
				  "%s() in %s at line %d: %s\n",
				  "device_reply_server",
				  __FILE__,
				  __LINE__,
				  "failed");
		}
	}
	stop_time();

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), reply_port));

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t) data,
				  size));
}

int
device_get_status_test(void)
{
	mach_port_t		device_port;
	int			i;
	test_device_status_t	status;
	unsigned int		status_count;

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));

	start_time();
	for (i = 0; i < loops; i++) {
		status_count = TEST_DEVICE_STATUS_COUNT;
		MACH_CALL(device_get_status, (device_port,
					      TEST_DEVICE_STATUS,
					      (dev_status_t) &status,
					      &status_count));
	}
	stop_time();

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
}

int
device_set_status_test(void)
{
	mach_port_t		device_port;
	int			i;
	test_device_status_t	status;
	unsigned int		status_count;

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));

	status_count = TEST_DEVICE_STATUS_COUNT;

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(device_set_status, (device_port,
					      TEST_DEVICE_STATUS,
					      (dev_status_t) &status,
					      status_count));
	}
	stop_time();

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
}

int
device_map_test(
	int size)
{
	mach_port_t	device_port;
	int		i;
	mach_port_t	pager;

	if (size < 0) {
		/* number of pages */
		size = -size * vm_page_size;
	}

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(device_map, (device_port,
				       VM_PROT_READ,
				       0,
				       size,
				       &pager,
				       FALSE));
		/* we should map the object and touch it here... */
	}
	stop_time();

	MACH_CALL(mach_port_destroy, (mach_task_self(), pager));
	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
}

int
device_set_filter_test(void)
{
	mach_port_t	device_port, receive_port;
	filter_t	filter[1];
	int 		i;

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));
	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &receive_port));

	filter[0] = NETF_NOP;

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(device_set_filter, (device_port,
					      receive_port,
					      MACH_MSG_TYPE_MAKE_SEND,
					      1,
					      (filter_array_t) filter,
					      sizeof filter / sizeof filter[0]));
	}
	stop_time();

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), receive_port));
}

int
device_net_receive_test(
	int		pkt_size,
	int		filter_size,
	boolean_t	synchronous)
{
	mach_port_t		device_port, receive_port;
	filter_t		filter[NET_MAX_FILTER];
	int			i;
	test_device_status_t	status;
	struct net_rcv_msg	msg;

	MACH_CALL(device_open, (master_device_port,
				MACH_PORT_NULL,
				0,
                                mach_perf_token,
				"test_device",
				&device_port));
	MACH_CALL(mach_port_allocate, (mach_task_self(),
				       MACH_PORT_RIGHT_RECEIVE,
				       &receive_port));

	/* set up the filter */
	for (i = 0; i < filter_size; i++) {
		filter[i] = NETF_NOP;
	}
	MACH_CALL(device_set_filter, (device_port,
				      receive_port,
				      MACH_MSG_TYPE_MAKE_SEND,
				      1,
				      (filter_array_t) filter,
				      filter_size));

	/* tell the driver to start sending reply messages */
	status.reply_port = MACH_PORT_NULL;	/* not used */
	status.reply_type = DEVICE_TEST_FILTERED_PACKET;
	status.reply_uaddr = (vm_offset_t) 0;	/* not used */
	status.reply_count = loops;
	status.reply_size = pkt_size;
	status.reply_synchronous = synchronous;
	MACH_CALL(device_set_status, (device_port,
				      TEST_DEVICE_GENERATE_REPLIES,
				      (dev_status_t) &status,
				      TEST_DEVICE_STATUS_COUNT));

	start_time();
	for (i = 0; i < loops; i++) {
		MACH_CALL(mach_msg, (&(msg.msg_hdr),
				     MACH_RCV_MSG,
				     0,
				     sizeof msg,
				     receive_port,
				     MACH_MSG_TIMEOUT_NONE,
				     MACH_PORT_NULL));
	}
	stop_time();

	MACH_CALL(device_close, (device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), device_port));
	MACH_CALL(mach_port_destroy, (mach_task_self(), receive_port));
}

#endif	/* TEST_DEVICE */

kern_return_t
device_open_reply(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	mach_port_t	device_port)
{
	return KERN_SUCCESS;
}

kern_return_t
device_read_reply(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	io_buf_ptr_t	data,
	unsigned int	data_count)
{
	if (return_code != D_SUCCESS) {
		test_exit(
			"%s() in %s at line %d: %s\n",
			 "device_read_reply",
			__FILE__,
			__LINE__,
			mach_error_string(return_code));
	} else if (global_dealloc) {
		if (data_count > 0) {
			MACH_CALL(vm_deallocate, (mach_task_self(),
						  (vm_offset_t) data,
						  data_count));
		}
	} else {
		*global_dealloc_ptr = (vm_address_t) data;
	}
	return KERN_SUCCESS;
}

kern_return_t
device_read_reply_inband(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	io_buf_ptr_t	data,
	unsigned int	data_count)
{
	if (return_code != D_SUCCESS) {
		test_exit(
			"%s() in %s at line %d: %s\n",
			 "device_read_reply_inband",
			__FILE__,
			__LINE__,
			mach_error_string(return_code));
	}
	return KERN_SUCCESS;
}

kern_return_t
device_read_reply_overwrite(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	unsigned int	data_count)
{
	return KERN_SUCCESS;
}

kern_return_t
device_write_reply(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	int		bytes_written)
{
	return KERN_SUCCESS;
}

kern_return_t
device_write_reply_inband(
	mach_port_t	reply_port,
	kern_return_t	return_code,
	int		bytes_written)
{
	return KERN_SUCCESS;
}



