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
 * File : etap_pool.h
 *
 *	  The start_data_node structure is primarily needed to hold
 *	  start information for read locks (since multiple readers
 * 	  can acquire a read lock).  For consistency, however, the
 * 	  structure is used for write locks as well.  Each complex
 *	  lock will maintain a linked list of these structures.
 */

#ifndef _KERN_ETAP_POOL_H_
#define _KERN_ETAP_POOL_H_

#include <kern/etap_options.h>
#include <mach/etap.h>
#include <mach/boolean.h>

#if	ETAP_LOCK_TRACE

#include <cpus.h>
#include <mach/clock_types.h>
#include <mach/kern_return.h>
#include <kern/misc_protos.h>

struct start_data_node {
	unsigned int	thread_id;           /* thread id                    */
	etap_time_t	start_hold_time;     /* time of last acquisition     */
	etap_time_t	start_wait_time;     /* time of first miss           */
	unsigned int	start_pc;            /* pc of acquiring function     */
	unsigned int	end_pc;              /* pc of relinquishing function */
	struct start_data_node *next;	     /* pointer to next list entry   */
};

typedef struct start_data_node* start_data_node_t;

/*
 *  The start_data_node pool is statically
 *  allocated and privatly maintained
 */
 
#define SD_POOL_ENTRIES     (NCPUS * 256)

extern  void			init_start_data_pool(void);
extern  start_data_node_t	get_start_data_node(void);
extern  void			free_start_data_node(start_data_node_t);

#else	/* ETAP_LOCK_TRACE */
typedef boolean_t start_data_node_t;
#define get_start_data_node()
#define free_start_start_data_node(node)
#endif	/* ETAP_LOCK_TRACE  */

#define SD_ENTRY_NULL	((start_data_node_t) 0)

#endif  /* _KERN_ETAP_POOL_H_ */
