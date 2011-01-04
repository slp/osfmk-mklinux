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

#ifndef _MACH_PPC_RPC_H_
#define _MACH_PPC_RPC_H_

#include <mach/kern_return.h>
#include <mach/port.h>

/*
 * Definition of RPC "glue code" operations vector -- entry
 * points needed to accomplish short-circuiting
 */
typedef struct rpc_glue_vector {
	kern_return_t	(*rpc_simple)(int, int, void *);
	boolean_t	(*copyin)(mach_port_t, vm_offset_t, vm_offset_t, int);
	boolean_t	(*copyout)(mach_port_t, vm_offset_t, vm_offset_t, int);
	boolean_t	(*copyinstr)(mach_port_t, vm_offset_t, vm_offset_t, int, int*);
	kern_return_t	(*thread_switch)(mach_port_t, int, int);
	kern_return_t	(*thread_depress_abort)(mach_port_t);
} *rpc_glue_vector_t;

/*
 * Macros used to dereference glue code ops vector -- note
 * hard-wired references to global defined below.  Also note
 * that most of the macros assume their caller has stacked
 * the target args somewhere, so that they can pass just the
 * address of the first arg to the short-circuited
 * implementation.
 */
#define CAN_SHCIRCUIT(name)	(_rpc_glue_vector->name != 0)
#define RPC_SIMPLE(port, rtn_num, argc, argv) \
	((*(_rpc_glue_vector->rpc_simple))(rtn_num, argc, (void *)(&(port))))
#define COPYIN(map, from, to, count) \
	((*(_rpc_glue_vector->copyin))(map, from, to, count))
#define COPYOUT(map, from, to, count) \
	((*(_rpc_glue_vector->copyout))(map, from, to, count))
#define COPYINSTR(map, from, to, max, actual) \
	((*(_rpc_glue_vector->copyinstr))(map, from, to, max, actual))
#define THREAD_SWITCH(thread, option, option_time) \
	((*(_rpc_glue_vector->thread_switch))(thread, option, option_time))
#define	THREAD_DEPRESS_ABORT(act)	\
	((*(_rpc_glue_vector->thread_depress_abort))(act))

/*
 * The implementation of glue functions for ppc
 */
extern boolean_t klcopyin(mach_port_t, vm_offset_t, vm_offset_t, int);
extern boolean_t klcopyout(mach_port_t, vm_offset_t, vm_offset_t, int);
extern boolean_t klcopyinstr(mach_port_t, vm_offset_t, vm_offset_t, int, int *);
extern kern_return_t klthread_switch(mach_port_t, int, int);

/*
 * An rpc_glue_vector_t defined either by the kernel or by crt0
 */
extern rpc_glue_vector_t _rpc_glue_vector;

#define MACH_RPC_ARGV(act)	(char*)(USER_REGS(act)->r3)  
#define MACH_RPC_RET(act)	( USER_REGS(act)->lr ) 
#define MACH_RPC_UIP(act)	( USER_REGS(act)->srr0 )
#define MACH_RPC_USP(act)	( USER_REGS(act)->r1 )

#endif	/* _MACH_PPC_RPC_H_ */











