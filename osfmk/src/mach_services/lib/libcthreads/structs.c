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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
/*
 * MkLinux
 */
#include <cthreads.h>
#include <cthread_filter.h>
#include "cthread_internals.h"

/* Available to outside for statistics */

struct cthread_status_struct cthread_status = {
    CTHREADS_VERSION,           /* version */
    QUEUE_INITIALIZER,          /* cthreads */
    0,                          /* cthread_cthreads */
    0,                          /* alloc_cthreads */
    CTHREAD_NULL,               /* cthread_list */
    0,                          /* stack_size */
    0,                          /* stack_mask */
    0,                          /* lock_spin_count */
    1,                          /* processors */
    8192,                       /* wait_stack_size */
    0,                          /* max_kernel_threads */
    0,                          /* kernel_threads */
    0,                          /* waiter_spin_count */
    0,                          /* mutex_spin_count */
    SPIN_LOCK_INITIALIZER,      /* run_lock */
    QUEUE_INITIALIZER,          /* run_queue */
    CTHREAD_DLQ_INITIALIZER,    /* waiting_dlq */
    QUEUE_INITIALIZER,          /* waiters */
    CTHREAD_NULL };             /* exit_thread */

#ifdef STATISTICS
struct cthread_statistics_struct cthread_stats = {SPIN_LOCK_INITIALIZER,0,0,
						      0,0,0,0,0,0,0,0,0,0,0,
						      0,0,0,0,0};
#endif /*STATISTICS*/

