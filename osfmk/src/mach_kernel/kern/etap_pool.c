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
 * File:       etap_pool.c
 *
 *             etap_pool.c contains the functions for maintenance
 *             of the start_data_pool.   The start_data_pool is
 *             used by the ETAP package.  Its primary
 *             objective is to provide start_data_nodes to complex
 *             locks so they can hold start information for read
 *             locks  (since multiple readers can acquire a read
 *             lock).   Each complex lock will maintain a linked
 *             list of these nodes.
 *
 * NOTES:      The start_data_pool is used instead of zalloc to
 *             eliminate complex lock dependancies.  If zalloc was used,
 *             then no complex locks could be used in zalloc code paths.
 *             This is both difficult and unrealistic, since zalloc
 *             allocates memory dynamically. Hence, this dependancy is
 *             eliminated with the use of the statically allocated
 *             start_data_pool.
 *
 */

#include <kern/lock.h>
#include <kern/spl.h>
#include <kern/etap_pool.h>
#include <kern/sched_prim.h>
#include <kern/macro_help.h>

#if	ETAP_LOCK_TRACE

/*
 *  Statically allocate the start data pool,
 *  header and lock.
 */

struct start_data_node  sd_pool [SD_POOL_ENTRIES];  /* static buffer */
start_data_node_t       sd_free_list;   /* pointer to free node list */
int                     sd_sleepers;    /* number of blocked threads */

simple_lock_data_t		sd_pool_lock;


/*
 *  Interrupts must be disabled while the 
 *  sd_pool_lock is taken.
 */

#define pool_lock(s)			\
MACRO_BEGIN				\
	s = splhigh();			\
	simple_lock(&sd_pool_lock);	\
MACRO_END

#define pool_unlock(s)			\
MACRO_BEGIN				\
	simple_unlock(&sd_pool_lock);	\
	splx(s);			\
MACRO_END


/*
 *  ROUTINE:    init_start_data_pool
 *
 *  FUNCTION:   Initialize the start_data_pool:
 *              - create the free list chain for the max 
 *                number of entries.
 *              - initialize the sd_pool_lock
 */

void
init_start_data_pool(void)
{
	int x;

	simple_lock_init(&sd_pool_lock, ETAP_MISC_SD_POOL);
    
	/*
	 *  Establish free list pointer chain
	 */

	for (x=0; x < SD_POOL_ENTRIES-1; x++)
		sd_pool[x].next = &sd_pool[x+1];

	sd_pool[SD_POOL_ENTRIES-1].next = SD_ENTRY_NULL;
	sd_free_list  = &sd_pool[0];
	sd_sleepers   = 0;
}

/*
 *  ROUTINE:    get_start_data_node
 *
 *  FUNCTION:   Returns a free node from the start data pool
 *              to the caller.  If none are available, the
 *              call will block, then try again.
 */

start_data_node_t
get_start_data_node(void)
{
	start_data_node_t avail_node;
	spl_t		  s;

	pool_lock(s);

	/*
	 *  If the pool does not have any nodes available,
	 *  block until one becomes free.
	 */

	while (sd_free_list == SD_ENTRY_NULL) {

		sd_sleepers++;
		assert_wait((event_t) &sd_pool[0], FALSE);
		pool_unlock(s);

		printf ("DEBUG-KERNEL: empty start_data_pool\n");
		thread_block((void (*)(void)) 0);

		pool_lock(s);
		sd_sleepers--;
	}

	avail_node   = sd_free_list;
	sd_free_list = sd_free_list->next;

	pool_unlock(s);

	bzero ((char *) avail_node, sizeof(struct start_data_node)); 
	avail_node->next = SD_ENTRY_NULL;

	return (avail_node);
}

/*
 *  ROUTINE:    free_start_data_node
 *
 *  FUNCTION:   Releases start data node back to the sd_pool,
 *              so that it can be used again.
 */

void
free_start_data_node (
	start_data_node_t   node)
{
	boolean_t   wakeup = FALSE;
	spl_t	    s;

	if (node == SD_ENTRY_NULL)
		return;

	pool_lock(s);

	node->next   = sd_free_list;
	sd_free_list = node;

	if (sd_sleepers)
		wakeup = TRUE;

	pool_unlock(s);

	if (wakeup)
		thread_wakeup((event_t) &sd_pool[0]);
}

#endif	/* ETAP_LOCK_TRACE */
