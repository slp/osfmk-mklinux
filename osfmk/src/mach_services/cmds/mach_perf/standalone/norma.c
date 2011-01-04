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
#include <norma_services.h>
#include <mach/norma_special_ports.h>

get_def_pager(argc, argv)
char *argv[];
{
	mach_port_t node_priv_port, vm_port;
	int node;

	if (argc != 2 || (!atod(argv[1], &node))) {
		printf("syntax: get_def_pager <node>\n");
		return;
	}

	if (!kernel_is_norma()) {
		printf("NORMA not configured\n");
		return;
	}

	MACH_CALL(norma_get_host_priv_port, (privileged_host_port,
					      node,
					      &node_priv_port));

	vm_port = MACH_PORT_NULL;
	MACH_CALL(vm_set_default_memory_manager, (node_priv_port,
						  &vm_port));

	printf("Default pager port for node %d: %d\n", node, vm_port);
}

set_def_pager(argc, argv)
char *argv[];
{
	mach_port_t port, node_priv_port, save_port;
	int node, my_node;

	if (argc != 3	|| (!atod(argv[1], (int *) &node))
			|| (!atod(argv[2], (int *) &port))) {
		printf("syntax: set_def_pager <node> <port name>\n");
		return;
	}


	if (!kernel_is_norma()) {
		printf("NORMA not configured\n");
		return;
	}

	MACH_CALL(norma_get_host_priv_port, (privileged_host_port,
					      node,
					      &node_priv_port));

	save_port = port;

	MACH_CALL(vm_set_default_memory_manager, (node_priv_port,
					      &port));

	MACH_CALL(norma_node_self, (host_port, &my_node));

	printf("Default pager port for node %d set to %d, was %d\n",
	       node, save_port, port);

}

node()
{
  	int *my_node;

	if (norma_node_self(host_port, &my_node) == KERN_SUCCESS)
		printf("node %d\n", my_node);
	else
		printf("NORMA not configured\n");
}

remote_node(argc, argv)
int argc;
char *argv[];
{
	int node;
	int my_node;

	if (argc < 2 || (!atod(argv[0], &node))) {
		return(0);
	}

	if (!kernel_is_norma()) {
		printf("NORMA not configured\n");
		return(0);
	}
	(void)norma_node_self(host_port, &my_node);
 	if (node == my_node)
		norma_node = NORMA_NODE_NULL;
	else
		norma_node = node;
	return(1);
}

