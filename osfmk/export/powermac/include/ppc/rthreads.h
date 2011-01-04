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

#ifndef _MACHINE_RTHREADS_H_
#define _MACHINE_RTHREADS_H_

#ifndef RTHREADS
#define RTHREADS 1
#endif	/*RTHREADS*/

#include <mach/machine/thread_status.h>
#include <mach/boolean.h>

#define RTHREAD_STACK_OFFSET 128

extern int rthread_sp(void);

#define STATE_FLAVOR PPC_THREAD_STATE
#define STATE_COUNT PPC_THREAD_STATE_COUNT
typedef struct ppc_thread_state thread_state;
#define STATE_STACK(state) ((state)->r1)

#endif /* _MACHINE_RTHREADS_H_ */
