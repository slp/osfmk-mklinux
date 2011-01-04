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

#ifndef _MACHINE_PHYSMEM_H_
#define	_MACHINE_PHYSMEM_H_

#include <kern/lock.h>
#include <kern/queue.h>
#include <mach/etap.h>
#include <mach/vm_types.h>

typedef struct physmem_header {
    queue_chain_t	ph_pages;	/* Chain of pages */
    unsigned int	ph_used;	/* Number of used elements */
    vm_page_t		ph_page;	/* Associated vm_page_t */
    /* unsigned char	ph_free[];	   Free element array */
} *physmem_header_t;

typedef struct physmem_queue {
    queue_chain_t	pq_chain;	/* Chain of physmem_queue */
    queue_chain_t	pq_fpages;	/* Chain of busy full pages */
    queue_chain_t	pq_nfpages;	/* Chain of busy not full pages */
    vm_size_t		pq_length;	/* Element length */
    unsigned int	pq_num;		/* Number of elements per page */
    unsigned int	pq_used;	/* Number of used elements */
    unsigned int	pq_busy;	/* Number of busy pages */
    decl_simple_lock_data(, pq_lock)	/* Lock for fields */
} *physmem_queue_t;

extern void		physmem_init(void);

extern void		physmem_add(physmem_queue_t,
				    vm_size_t,
				    etap_event_t);

extern vm_offset_t	physmem_alloc(physmem_queue_t);

extern void		physmem_free(physmem_queue_t,
				     vm_offset_t);

extern void		physmem_collect_scan(void);

#endif /* _MACHINE_PHYSMEM_H_ */
