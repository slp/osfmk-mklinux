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
 *	File:	xmm_luser.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Definitions for non-kernel xmm use.
 */

#include <mach.h>
#include <cthreads.h>
#include <mach/message.h>
#include "xmm_obj.h"

struct mutex master_mutex;

master_lock()
{
	mutex_lock(&master_mutex);
}

master_unlock()
{
	mutex_unlock(&master_mutex);
}

/* XXX should move master_mutex stuff into mig_loop.c? */

xmm_user_init()
{
	extern int the_mig_loop();
	extern int verbose;

	mutex_init(&master_mutex);
	if (verbose) {
		printf("Initializing mig subsystem...\n");
	}
	xmm_mig_init();
	if (verbose) {
		printf("Initializing protocols...\n");
	}
	net_init();
	if (verbose) {
		printf("Initializing obj subsystem...\n");
	}
	obj_init();
	if (verbose) {
		printf("Initializing xmm subsystem...\n");
	}
	xmm_init();
#if 1
	/* XXX should at least make sure this is safe to do */
	cthread_detach(cthread_fork((cthread_fn_t) the_mig_loop, 0));
	if (verbose) {
		printf("ready.\n");
	}
#else
	if (verbose) {
		printf("ready.\n");
	}
	the_mig_loop();
#endif
}

obj_server(request, reply)	/* XXX move this into xmm_server */
	char *request;
	char *reply;
{
	if (memory_object_server(request, reply)) {
		return TRUE;
	}
	if (proxy_server(request, reply)) {
		return TRUE;
	}
	if (xmm_net_server(request, reply)) {
		return TRUE;
	}
	return FALSE;
}

panic(fmt, a, b, c, d, e, f, g, h, i, j, k, l)
	char *fmt;
	int a, b, c, d, e, f, g, h, i, j, k, l;
{
	printf("panic: ");
	printf(fmt, a, b, c, d, e, f, g, h, i, j, k, l);
	printf("\n");
	exit(1);
}

#undef	atop
#undef	ptoa

atop(a)
{
	return a / vm_page_size;
}

ptoa(p)
{
	return p * vm_page_size;
}
