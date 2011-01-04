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
 * 
 */
/*
 * MkLinux
 */

/*
 *	File:	kern/norma_bootstrap.c
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Bootstrap distributed kernel services.
 */

#include <dipc.h>
#include <norma_device.h>
#include <norma_task.h>
#include <norma_vm.h>
#include <kern/misc_protos.h>
#include <flipc.h>

#if	DIPC
#include <dipc/dipc_funcs.h>
#endif

#if	NORMA_VM
#include <xmm/xmm_obj.h>
#endif

#if	FLIPC
#include <flipc/flipc_usermsg.h>
#endif

void
norma_bootstrap(void)
{
#if	DIPC
	dipc_bootstrap();
#endif	/* DIPC */

#if	NORMA_VM
	norma_vm_init();
#endif	/* NORMA_VM */

#if	FLIPC
	flipc_system_init();
#endif	/* FLIPC */	
}
