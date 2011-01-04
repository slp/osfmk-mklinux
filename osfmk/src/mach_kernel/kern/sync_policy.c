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
 *	File:	kern/sync_policy.c
 *	Author:	Joseph CaraDonna
 */

#include <kern/misc_protos.h>
#include <kern/sync_policy.h>

void
sync_policy_enqueue (int policy, queue_t q_head, thread_t thread)
{
	register queue_entry_t scan, prev;


	/*
	 *  If the synchronizer's policy is not explicitly set
	 *  to fixed priority, then FIFO is assumed.
	 */

	if (policy == SYNC_POLICY_FIXED_PRIORITY) {

		/* Find insertion point */

		scan = queue_first(q_head);
		while (!queue_end(q_head, scan) &&
		       ((thread_t)scan)->priority <= thread->priority)
			scan = ((thread_t)scan)->wait_link.next;

		/* Insert */

		if (scan == q_head) {
			prev = scan->prev;
			scan->prev = (queue_entry_t) thread;
		}
		else {
			prev = ((thread_t)scan)->wait_link.prev;
			((thread_t)scan)->wait_link.prev =(queue_entry_t)thread;
		}

		if (prev == q_head)
			prev->next = (queue_entry_t) thread;
		else
			((thread_t)prev)->wait_link.next =(queue_entry_t)thread;

		thread->wait_link.prev = prev;
		thread->wait_link.next = scan;

	}
	else    /* SYNC_POLICY_FIFO */
		queue_enter(q_head, thread, thread_t, wait_link);
}

thread_t
sync_policy_dequeue (int policy, queue_t q)
{
	thread_t  thread;

	queue_remove_first(q, thread, thread_t, wait_link);
	return (thread);
}

void
sync_policy_remqueue (int policy, queue_t q, thread_t thread)
{
	queue_remove(q, thread, thread_t, wait_link);
}
