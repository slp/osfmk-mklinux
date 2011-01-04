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
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

#include <stdio.h>
#include <string.h>
#define	MACH_INIT_SLOTS		1
#include <mach.h>
#include <mach/message.h>
#include <mach/bootstrap.h>
#include <mach/notify.h>
#include <servers/service.h>

#include <servers/netname_server.h>

extern void netname_init(void);
extern void printf_init(mach_port_t);
extern void panic_init(mach_port_t);
extern void panic(const char *, ...);
extern char *mach_error_string(kern_return_t);
extern kern_return_t mach_msg_server(boolean_t (*)(mach_msg_header_t *,
						   mach_msg_header_t *),
				     mach_msg_size_t,
				     mach_port_t,
				     int);
extern boolean_t netname_server(mach_msg_header_t *, mach_msg_header_t *);
extern boolean_t notify_server(mach_msg_header_t *, mach_msg_header_t *);

char *program = NULL;
boolean_t Debug = FALSE;

mach_port_t notify_port;
mach_port_t netname_port;

mach_port_t root_ledger_wired;
mach_port_t root_ledger_paged;
mach_port_t security_port;

static boolean_t snames_demux(mach_msg_header_t *, mach_msg_header_t *);

static void
usage(void)
{
    panic("usage: %s [-d]\n", program);
}

int
main(int argc, char **argv)
{
    mach_port_t pset;
    int i;
    kern_return_t kr;
    mach_port_t host_port;
    mach_port_t device_port;

    program = argv[0];

    kr = bootstrap_ports(bootstrap_port,
		&host_port,
		&device_port,
		&root_ledger_wired,
		&root_ledger_paged,
		&security_port);
    if (kr != KERN_SUCCESS) {
	panic("%s: bootstrap_ports: %s\n",
		program, mach_error_string(kr));
    }

    printf_init(device_port);
    panic_init(host_port);

    printf("(name_server): started\n");

    if (argc > 0) {
	for (i = 1; i < argc; i++)
	    if (strcmp(argv[i], "-d") == 0)
		Debug = TRUE;
	    else if (strcmp(argv[i], "--") == 0) {
		i++;
		break;
	    } else if (argv[i][0] == '-')
		usage();
	    else
		break;

	argv += i;
	argc -= i;

	if (argc != 0)
	    usage();
    }

    /* Allocate our notification port. */

    kr = mach_port_allocate(mach_task_self(),
			    MACH_PORT_RIGHT_RECEIVE, &notify_port);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_allocate: %s\n",
		program, mach_error_string(kr));
    }

    kr = service_checkin(service_port, name_server_port, &netname_port);
    if (kr != KERN_SUCCESS) {
	panic("%s: service_checkin: %s\n",
	      program, mach_error_string(kr));
    }

    /*
     *	Prepare the name service.
     *	The three do_netname_check_in calls will consume user-refs
     *	for their port args, so we have to generate the refs.
     *	Note that mach_task_self() is just a macro;
     *	it doesn't return a ref.
     */

    netname_init();

    kr = mach_port_mod_refs(mach_task_self(), mach_task_self(),
			    MACH_PORT_RIGHT_SEND, 3);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_mod_refs: %s\n",
	      program, mach_error_string(kr));
    }

    kr = mach_port_mod_refs(mach_task_self(), netname_port,
			    MACH_PORT_RIGHT_SEND, 3);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_mod_refs: %s\n",
		program, mach_error_string(kr));
    }

    kr = do_netname_check_in(netname_port, (char *)"NameServer",
			     mach_task_self(), netname_port);
    if (kr != KERN_SUCCESS) {
	panic("%s: netname_check_in: %s\n",
		program, mach_error_string(kr));
    }

    kr = do_netname_check_in(netname_port, (char *)"NMMonitor",
			     mach_task_self(), netname_port);
    if (kr != KERN_SUCCESS) {
	panic("%s: netname_check_in: %s\n",
		program, mach_error_string(kr));
    }

    kr = do_netname_check_in(netname_port, (char *)"NMControl",
			     mach_task_self(), netname_port);
    if (kr != KERN_SUCCESS) {
	panic("%s: netname_check_in: %s\n",
		program, mach_error_string(kr));
    }

    /* Prepare our port set. */

    kr = mach_port_allocate(mach_task_self(),
			    MACH_PORT_RIGHT_PORT_SET, &pset);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_allocate: %s\n",
		program, mach_error_string(kr));
    }

    kr = mach_port_move_member(mach_task_self(), netname_port, pset);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_move_member: %s\n",
		program, mach_error_string(kr));
    }

    kr = mach_port_move_member(mach_task_self(), notify_port, pset);
    if (kr != KERN_SUCCESS) {
	panic("%s: mach_port_move_member: %s\n",
		program, mach_error_string(kr));
    }

    /*
     * Let the bootstrap know we are finished.
     */
    (void)bootstrap_completed(bootstrap_port, mach_task_self());

    /*
     * Enter service loop.
     */
    kr = mach_msg_server(snames_demux, 256, pset, MACH_MSG_OPTION_NONE);
    panic("%s: mach_msg_server: %s\n",
	    program, mach_error_string(kr));
    return(1);
}

static boolean_t
snames_demux(mach_msg_header_t *request, mach_msg_header_t *reply)
{
    if (request->msgh_local_port == netname_port)
	return netname_server(request, reply);
    else if (request->msgh_local_port == notify_port)
	return notify_server(request, reply);
    panic("%s: snames_demux: port = %x\n",
	  program, request->msgh_local_port);
    return (boolean_t)FALSE;
}
