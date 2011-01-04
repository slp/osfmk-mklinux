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
 * OLD HISTORY
 * Revision 1.1.6.7  1996/01/25  11:33:18  bernadat
 * 	Added remote_node for remote execution support.
 * 	[95/12/11            bernadat]
 * 
 * Revision 1.1.6.6  1995/08/21  20:58:31  devrcs
 * 	Move kernel_is_norma() to lib/test.c.
 * 	[1995/07/19  14:50:48  mmp]
 * 
 * Revision 1.1.6.5  1995/05/14  19:58:04  dwm
 * 	ri-osc CR1304 - merge (nmk19_latest - nmk19b1) diffs into mainline.
 * 	a couple of casts to quiet warnings.
 * 	[1995/05/14  19:48:00  dwm]
 * 
 * Revision 1.1.6.4  1995/02/23  14:56:33  alanl
 * 	Pulled from dipc2_shared (travos)
 * 	[1995/01/18  22:38:49  alanl]
 * 
 * Revision 1.1.12.1  1994/12/01  21:48:19  dwm
 * 	mk6 CR801 - copyright marker not _FREE
 * 	[1994/12/01  21:35:21  dwm]
 * 
 * Revision 1.1.6.1  1994/06/18  18:45:13  bolinger
 * 	Import NMK17.2 sources into MK6.
 * 	[1994/06/18  18:35:51  bolinger]
 * 
 * Revision 1.1.2.1  1994/04/14  13:54:56  bernadat
 * 	Initial revision.
 * 	[94/04/14            bernadat]
 * 
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
