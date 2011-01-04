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
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
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
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#ifndef	_PPC_LOCK_H_
#define	_PPC_LOCK_H_

#include <cpus.h>
#include <mach_rt.h>

#include <kern/macro_help.h>
#include <kern/assert.h>

#define	NEED_ATOMIC	((NCPUS > 1) || MACH_RT)

struct slock {
	int		lock_data;
#if	NCPUS > 1
	/*
	 * The reservation granularity for the load-and-reserve instructions
	 * is a cache line, which is 32 bytes long currently. If there are more
	 * than one hw_lock in a "reservation granule", dead-lock can occur.
	 */
	int		lock_padding[7];
#endif	/* NCPUS > 1 */
};
#define hw_lock_addr(hwl)	(&((hwl).lock_data))

typedef struct slock hw_lock_data_t, *hw_lock_t;

#define mutex_try	_mutex_try
#define mutex_lock(m)							\
MACRO_BEGIN								\
	assert(current_thread() == THREAD_NULL ||			\
	       current_thread()->wait_event == NO_EVENT);		\
	_mutex_lock((m));						\
MACRO_END

extern void	kernel_preempt_check(void);

#endif	/* _PPC_LOCK_H_ */
