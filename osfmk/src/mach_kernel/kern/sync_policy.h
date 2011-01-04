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
 *	File:	kern/sync_policy.h
 *	Author:	Joseph CaraDonna
 */

#ifndef _KERN_SYNC_QUEUE_H_
#define _KERN_SYNC_QUEUE_H_

#include <kern/thread.h>
#include <kern/queue.h>
#include <mach/sync_policy.h>

extern void	sync_policy_enqueue	(int p, queue_t q, thread_t thread);
extern thread_t sync_policy_dequeue	(int p, queue_t q);
extern void 	sync_policy_remqueue    (int p, queue_t q, thread_t thread);

#endif /* _KERN_SYNC_QUEUE_H_ */
