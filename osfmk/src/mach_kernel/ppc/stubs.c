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



/* TODO NMGS REMOVE ALL OF THESE AND THEN THIS FILE !!! */

#include <types.h>
#include <ppc/trap.h>
#include <kern/misc_protos.h>
#include <kern/sched_prim.h>
#include <kern/kern_types.h>
#include <kern/exception.h>
#include <ppc/pmap.h>
#include <ppc/misc_protos.h>
#include <ppc/fpu_protos.h>
#include <mach/rpc.h>
#include <ppc/machine_rpc.h>

int
procitab(u_int spllvl,
         void (*handler)(int),
         int unit) 
{
	printf("NMGS TODO NOT YET");
	return 0;
}

void restart_mach_syscall(void)
{
	panic(__FUNCTION__/* NOT YET NMGS */);
}
