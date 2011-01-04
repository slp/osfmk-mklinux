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


#define TEST_NAME	port

#include <mach_perf.h>

int port_allocate_and_destroy_test();
int port_allocate_or_destroy_test();
int name_allocate_and_destroy_test();
int name_allocate_or_destroy_test();
int reply_port_test();
int reply_port_and_destroy_test();
int port_rename_test();
int port_insert_right_and_deallocate_test();
int port_insert_right_or_deallocate_test();
int port_move_member_test();
int port_mod_refs_test();
int port_get_refs_test();
int port_get_attributes_test();
int port_set_attributes_test();
int port_set_mscount_test();
int port_set_seqno_test();
int port_type_test();
int port_request_notification_test();

char *private_options = "\n\
\t[-first_port <name>]     When using non generic port names\n\
\t                         the value <name> is used as the first name\n\
\t                         (default is 50000)\n\
\t[-max_port <count>]      Maximum port names count for IPC space\n\
\t                         is <count> (default is 20000)\n\
";

#define ALLOCATE	0
#define DEALLOCATE	1

#define MOVE 0
#define REMOVE 1

mach_port_t port_first_port_name = 0x50000;	/* !! update help message */
int port_max_port_names = 20000; 		/* !! update help message */
int port_debug;

struct test tests[] = {
"port_allocate",		0, port_allocate_or_destroy_test,
				ALLOCATE, 0, 0, 0,
"port_destroy",			0, port_allocate_or_destroy_test,
				DEALLOCATE, 0, 0, 0,
"port_allocate/destroy",	0, port_allocate_and_destroy_test, 0, 0, 0, 0,
"port_allocate_name *",		0, name_allocate_or_destroy_test,
				ALLOCATE, 0, 0, 0,
"port_allocate_name/destroy *",	0, name_allocate_and_destroy_test, 0, 0, 0, 0,
"reply_port",			0, reply_port_test, 0, 0, 0, 0,
"reply_port/destroy",		0, reply_port_and_destroy_test,	0, 0, 0, 0,
"port_insert_right/deallocate",	0, port_insert_right_and_deallocate_test,
				0, 0, 0, 0,
"port_insert_right",		0, port_insert_right_or_deallocate_test,
				ALLOCATE, 0, 0, 0,
"port_deallocate",		0, port_insert_right_or_deallocate_test,
				DEALLOCATE, 0, 0, 0,
"port_rename *",		0, port_rename_test, 0, 0, 0, 0,
"port_move_member (move)",	0, port_move_member_test,
				MOVE, 0, 0, 0,
"port_move_member (remove)",	0, port_move_member_test,
				REMOVE, 0, 0, 0,
"port_get_refs (send right)",	0, port_get_refs_test,
				MACH_PORT_RIGHT_SEND, 0, 0, 0,
"port_mod_refs (send right)",	0, port_mod_refs_test,
				MACH_PORT_RIGHT_SEND, 0, 0, 0,
"request_notification (dead)",	0, port_request_notification_test, 
				MACH_NOTIFY_DEAD_NAME, 0, 0, 0,
"req_notif (no send) ",		0, port_request_notification_test, 
				MACH_NOTIFY_NO_SENDERS, 0, 0, 0,
"req_notif (no send) / reply",	0, port_request_notification_test, 
				MACH_NOTIFY_NO_SENDERS, TRUE, 0, 0,
"port_get_attributes (limit)",	0, port_get_attributes_test, 
				MACH_PORT_LIMITS_INFO, 0, 0, 0,
"port_get_attributes (status)",	0, port_get_attributes_test, 
				MACH_PORT_RECEIVE_STATUS, 0, 0, 0,
"port_set_attributes (limit)",	0, port_set_attributes_test,
				MACH_PORT_LIMITS_INFO, 0, 0, 0,
"port_set_mscount",		0, port_set_mscount_test, 0, 0, 0, 0,
"port_set_seqno",		0, port_set_seqno_test, 0, 0, 0, 0,
"port_type",			0, port_type_test, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0
};

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	struct test *test = tests;

	test_init();
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i], "-first_port")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
		  	port_first_port_name = atoi(argv[i]); 
		} else if (!strcmp(argv[i], "-max_port")) {
			if (++i >= argc || *argv[i] == '-')
				usage();
		  	port_max_port_names = atoi(argv[i]); 
		} else if (!is_gen_opt(argc, argv, &i, tests, private_options))
			usage();

	if (in_kernel) {
		/* XXX be silent about disabled tests? */
		if (!topt) {
			topt = TRUE;
			enable_all_tests(tests);
		}
		disable_test_number(3,tests);	/* port_allocate_name */
		disable_test_number(4,tests);	/* allocate_name/destroy */
		disable_test_number(10,tests);	/* port_rename */
	}

	port_debug = debug;
	run_tests(tests);
	test_end();
}

port_allocate_and_destroy_test()
{
	register i;
	mach_port_t port;
	mach_port_t task = mach_task_self();

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_allocate, (task,
					 MACH_PORT_RIGHT_RECEIVE,
					 &port));
		MACH_CALL(mach_port_destroy, (task, port));
	}
	stop_time();
}

port_allocate_or_destroy_test(op)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *ports, *port;

	set_max_loops(port_max_port_names);
	MACH_CALL(vm_allocate, (mach_task_self(), (vm_offset_t *)&ports,
				loops*sizeof(mach_port_t), TRUE));

	if (op == ALLOCATE)
		start_time();

	for (i=loops, port = ports; i--; port++)  {
		MACH_CALL(mach_port_allocate, (task,
					       MACH_PORT_RIGHT_RECEIVE,
					       port));
	}

	if (op == ALLOCATE)
		stop_time();
	if (op == DEALLOCATE)
		start_time();

	for (i=loops, port = ports; i--; port++) 
		MACH_CALL(mach_port_destroy, (task, *port));

	if (op == DEALLOCATE)
		stop_time();
	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )ports,
				  loops*sizeof(mach_port_t)));
	
	test_cache(port_allocate_or_destroy_test, (op));
}

name_allocate_and_destroy_test()
{
	register i;
	mach_port_t port = port_first_port_name;
	mach_port_t task = mach_task_self();

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_allocate_name, (task,
					      MACH_PORT_RIGHT_RECEIVE,
					      port));
		MACH_CALL(mach_port_destroy, (task, port));
	}
	stop_time();
}

name_allocate_or_destroy_test(op)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *ports, *port;

	set_max_loops(port_max_port_names);

	MACH_CALL(vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&ports,
				  loops*sizeof(mach_port_t),
				  TRUE));

	for (i=0; i<loops; i++)
		*(ports+i) = port_first_port_name + i;

	if (op == ALLOCATE)
		start_time();

	for (i=loops, port = ports; i--; port++)
		MACH_CALL(mach_port_allocate_name, (task,
					 MACH_PORT_RIGHT_RECEIVE,
					 *port));

	if (op == ALLOCATE)
		stop_time();
	if (op == DEALLOCATE)
		start_time();

	for (i=loops, port = ports; i--; port++) 
		MACH_CALL(mach_port_destroy, (task, *port));

	if (op == DEALLOCATE)
		stop_time();

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )ports,
				  loops*sizeof(mach_port_t)));
	
	test_cache(name_allocate_or_destroy_test, (op));
}

reply_port_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *ports, *port;

	set_max_loops(port_max_port_names);

	MACH_CALL(vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&ports,
				  loops*sizeof(mach_port_t),
				  TRUE));

	start_time();

	for (i=loops, port = ports; i--; port++)
		MACH_FUNC(*port, mach_reply_port, ());

	stop_time();


	for (i=loops, port = ports; i--; port++) 
		MACH_CALL(mach_port_destroy, (task, *port));

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )ports,
				  loops*sizeof(mach_port_t)));
	
	test_cache(reply_port_test, ());
}

reply_port_and_destroy_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t port;

	start_time();

	for (i=loops; i--;)  {
		MACH_FUNC(port, mach_reply_port, ());
		MACH_CALL(mach_port_destroy, (task, port));
	}

	stop_time();
}


port_insert_right_and_deallocate_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t port;

	MACH_FUNC(port, mach_reply_port, ());

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_insert_right, (task, port, port,
					     MACH_MSG_TYPE_MAKE_SEND));
		MACH_CALL(mach_port_deallocate, (task, port));
	}

	stop_time();

	MACH_CALL(mach_port_destroy, (task, port));

}
		
port_insert_right_or_deallocate_test(op)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *ports, *port;

	max_loops = port_max_port_names;	
	MACH_CALL(vm_allocate, (mach_task_self(),
				  (vm_offset_t *)&ports,
				  loops*sizeof(mach_port_t),
				  TRUE));

	for (i=loops, port = ports; i--; port++) 
		MACH_FUNC(*port, mach_reply_port, ());

	if (op == ALLOCATE)
		start_time();

	for (i=loops, port = ports; i--; port++)
		MACH_CALL(mach_port_insert_right, (task, *port, *port,
					     MACH_MSG_TYPE_MAKE_SEND));

	if (op == ALLOCATE)
		stop_time();
	if (op == DEALLOCATE)
		start_time();

	for (i=loops, port = ports; i--; port++)
		MACH_CALL(mach_port_deallocate, (task, *port));

	if (op == DEALLOCATE)
		stop_time();


	for (i=loops, port = ports; i--; port++) 
		MACH_CALL(mach_port_destroy, (task, *port));

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )ports,
				  loops*sizeof(mach_port_t)));
}

port_get_attributes_test(flavor)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t port;
	struct mach_port_limits limits;
	struct mach_port_status status;
	mach_msg_type_number_t count;
	mach_port_info_t info;

	switch (flavor) {
	case MACH_PORT_LIMITS_INFO:
		info = (mach_port_info_t)&limits;
		count = MACH_PORT_LIMITS_INFO_COUNT;
		break;
	case MACH_PORT_RECEIVE_STATUS:
		info = (mach_port_info_t)&status;
		count = MACH_PORT_RECEIVE_STATUS_COUNT; 
		break;
	}


	MACH_FUNC(port, mach_reply_port, ());

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_get_attributes,
			  (task, port, flavor, info, &count));
	}

	stop_time();

	MACH_CALL(mach_port_destroy, (task, port));
}

port_set_attributes_test(flavor)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t port;
	struct mach_port_limits limits;
	struct mach_port_status status;
	mach_msg_type_number_t count;
	mach_port_info_t info;

	switch (flavor) {
	case MACH_PORT_LIMITS_INFO:
		info = (mach_port_info_t)&limits;
		count = sizeof limits;
		break;
	}


	MACH_FUNC(port, mach_reply_port, ());
	MACH_CALL(mach_port_get_attributes,
		  (task, port, flavor, info, &count));

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_set_attributes,
			  (task, port, flavor, info, count));
	}

	stop_time();

	MACH_CALL(mach_port_destroy, (task, port));
}

port_get_refs_test(right)
mach_port_right_t right;
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t port;
	mach_port_urefs_t refs;

	MACH_FUNC(port, mach_reply_port, ());
	MACH_CALL(mach_port_insert_right, (task, port, port,
					   MACH_MSG_TYPE_MAKE_SEND));

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_get_refs,
			  (task, port, right, &refs));
	}

	stop_time();

	MACH_CALL(mach_port_destroy, (task, port)); 
}

port_mod_refs_test(right)
mach_port_right_t right;
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t port;
	mach_port_delta_t delta = 1;

	MACH_FUNC(port, mach_reply_port, ());
	MACH_CALL(mach_port_insert_right, (task, port, port,
					   MACH_MSG_TYPE_MAKE_SEND));

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_mod_refs,
			  (task, port, right, delta));
		delta = -delta;
	}

	stop_time();

	MACH_CALL(mach_port_destroy, (task, port)); 
}

port_set_mscount_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t port;
	mach_port_mscount_t count = 2;

	MACH_FUNC(port, mach_reply_port, ());
	MACH_CALL(mach_port_insert_right, (task, port, port,
					   MACH_MSG_TYPE_MAKE_SEND));

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_set_mscount,
			  (task, port, count));
	}

	stop_time();

	MACH_CALL(mach_port_destroy, (task, port)); 
}

port_set_seqno_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t port;
	mach_port_seqno_t seqno = 2;

	MACH_FUNC(port, mach_reply_port, ());
	MACH_CALL(mach_port_insert_right, (task, port, port,
					   MACH_MSG_TYPE_MAKE_SEND));

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_set_seqno,
			  (task, port, seqno));
	}

	stop_time();

	MACH_CALL(mach_port_destroy, (task, port)); 
}

port_type_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t port;
	mach_port_type_t port_type;

	MACH_FUNC(port, mach_reply_port, ());
	MACH_CALL(mach_port_insert_right, (task, port, port,
					   MACH_MSG_TYPE_MAKE_SEND));

	start_time();

	for (i=loops; i--;)  {
		MACH_CALL(mach_port_type,
			  (task, port, &port_type));
	}

	stop_time();

	MACH_CALL(mach_port_destroy, (task, port)); 
}

port_rename_test()
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *port, *ports;
	register inc = port_first_port_name;

	set_max_loops(port_max_port_names);
	MACH_CALL(vm_allocate, (mach_task_self(), (vm_offset_t *)&ports,
				loops*sizeof(mach_port_t), TRUE));

	for (i=loops, port = ports; i--; port++)  {
		MACH_FUNC(*port, mach_reply_port, ());
	}

	start_time();

	for (i=loops, port = ports; i--; port++)  {
		MACH_CALL(mach_port_rename,
			  (task, *port, (*port)+inc));
	}

	stop_time();

	for (i=loops, port = ports; i--; port++) 
		MACH_CALL(mach_port_destroy, (task, (*port)+inc));

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )ports,
				  loops*sizeof(mach_port_t)));
}

port_move_member_test(op)
{
	register i;
	mach_port_t task = mach_task_self();
	mach_port_t *ports, *port, port_set;

	set_max_loops(port_max_port_names);
	MACH_CALL(vm_allocate, (mach_task_self(), (vm_offset_t *)&ports,
				loops*sizeof(mach_port_t), TRUE));

	for (i=loops, port = ports; i--; port++)
		MACH_FUNC(*port, mach_reply_port, ());

	MACH_CALL( mach_port_allocate, (mach_task_self(),
					MACH_PORT_RIGHT_PORT_SET,
					&port_set));
	
	if (op == MOVE)
		start_time();

	for (i=loops, port = ports; i--; port++) 
		MACH_CALL(mach_port_move_member, (task, *port, port_set));

	if (op == MOVE) {
		stop_time();
	} else {
		start_time();
		for (i=loops, port = ports; i--; port++) 
			MACH_CALL(mach_port_move_member, (task, *port,
							  MACH_PORT_NULL));

	}

	if (op == REMOVE)
		stop_time();

	for (i=loops, port = ports; i--; port++) 
		MACH_CALL(mach_port_destroy, (task, *port));

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )ports,
				  loops*sizeof(mach_port_t)));
	
	MACH_CALL(mach_port_destroy, (task, port_set));
}

port_request_notification_test(variant, reply)
mach_msg_id_t 	variant;
boolean_t	reply;
{
	register 		i;
	mach_port_t 		task = mach_task_self();
	mach_port_t 		*port;
	mach_port_t		*ports;
	mach_port_t		notification_port;
	mach_port_t		prev_port;
	mach_port_t		reply_port;
	mach_port_mscount_t	sync = 0;

	set_max_loops(port_max_port_names);
	MACH_CALL(vm_allocate, (mach_task_self(), (vm_offset_t *)&ports,
				loops*sizeof(mach_port_t), TRUE));

	for (i=loops, port = ports; i--; port++)
		MACH_FUNC(*port, mach_reply_port, ());

	MACH_FUNC(notification_port, mach_reply_port, ());


	start_time();

	for (i=loops, port = ports; i--; port++) {
		MACH_CALL(mach_port_request_notification,
			  (task, *port, variant,
			   sync,
			   notification_port,
			   MACH_MSG_TYPE_MAKE_SEND_ONCE,
			   &prev_port));
		if (reply)
			WAIT_REPLY(port_notify_server, notification_port, 128)
	}

	stop_time();

	for (i=loops, port = ports; i--; port++) 
		MACH_CALL(mach_port_destroy, (task, *port));

	MACH_CALL(vm_deallocate, (mach_task_self(),
				  (vm_offset_t )ports,
				  loops*sizeof(mach_port_t)));
	
	MACH_CALL(mach_port_destroy, (task, notification_port));

}

kern_return_t
port_mach_notify_port_deleted(
	mach_port_t	port,
	int		name)			 
{
	if (port_debug)
		printf("mach_notify_port_deleted(), name = %x\n", name);
	return(KERN_SUCCESS);
}

kern_return_t
port_mach_notify_port_destroyed(
	mach_port_t	port,
	int		rcv_right)
{
	if (port_debug)
		printf("mach_notify_port_destroyed(), rcv_right = %x\n", rcv_right);
	return(KERN_SUCCESS);
}

kern_return_t
port_mach_notify_no_senders(
	mach_port_t	port,
	int		count)
{
	if (port_debug)
		printf("mach_notify_no_senders(), count = %x\n", count);
	return(KERN_SUCCESS);
}

kern_return_t
port_mach_notify_send_once(
	mach_port_t	port)
{
	if (port_debug)
		printf("mach_notify_send_once()\n");
	return(KERN_SUCCESS);
}

kern_return_t
port_mach_notify_dead_name(
	mach_port_t	port,
	int		name)			 
{
	if (port_debug)
		printf("mach_notify_dead_name(), name = %x\n", name);
	return(KERN_SUCCESS);
}
