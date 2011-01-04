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
/*
 * 	File: 	malloc.c
 *	Author: Eric Cooper, Carnegie Mellon University
 *	Date:	July, 1988
 *
 * 	Memory allocator for use with multiple threads.
 */
/*
 *	NOTE:	This code assumes the proper rthread control locks
 *		are held.
 */

#include <rthreads.h>
#include <rthread_internals.h>

#include <string.h>

/*
 * Structure of memory block header.
 * When free, next points to next block on free list.
 * When allocated, fl points to free list.
 * Size of header is 4 bytes, so minimum usable block size is 8 bytes.
 */
typedef union header {
	union header *next;
	struct free_list *fl;
} *header_t;

#define MIN_SIZE	8	/* minimum block size */

typedef struct free_list {
	header_t head;		/* head of free list for this size */
#ifdef	DEBUG
	int in_use;		/* # mallocs - # frees */
#endif	/* DEBUG */
} *free_list_t;

/*
 * Free list with index i contains blocks of size 2^(i+3) including header.
 * Smallest block size is 8, with 4 bytes available to user.
 * Size argument to malloc is a signed integer for sanity checking,
 * so largest block size is 2^31.
 */
#define NBUCKETS	29

static struct free_list malloc_free_list[NBUCKETS];

static void
rthreads_more_memory(int size, register free_list_t fl)
{
	register int amount;
	register int n;
	vm_address_t where;
	register header_t h;
	kern_return_t r;

	if (size <= vm_page_size) {
		amount = vm_page_size;
		n = vm_page_size / size;
		/*
		 * We lose vm_page_size - n*size bytes here.
		 */
	} else {
		amount = size;
		n = 1;
	}

	MACH_CALL(vm_allocate(mach_task_self(),
			      &where,
			      (vm_size_t) amount,
			      TRUE), r);

	/* We mustn't allocate at address 0, since programs will then see
	 * what appears to be a null pointer for valid data.
	 */

	if (r == KERN_SUCCESS && where == 0) {
		MACH_CALL(vm_allocate(mach_task_self(), &where,
				      (vm_size_t) amount, TRUE), r);

		if (r == KERN_SUCCESS) {
			MACH_CALL(vm_deallocate(mach_task_self(),
						(vm_address_t) 0,
						(vm_size_t) amount), r);
		}
	}

	h = (header_t) where;

	do {
		h->next = fl->head;
		fl->head = h;
		h = (header_t) ((char *) h + size);
	} while (--n != 0);
}

void *
rthreads_malloc(register unsigned int size)
{
	register int i, n;
	register free_list_t fl;
	register header_t h;

	if ((int) size <= 0)		/* sanity check */
		return 0;

	size += sizeof(union header);

	/*
	 * Find smallest power-of-two block size
	 * big enough to hold requested size plus header.
	 */

	i = 0;
	n = MIN_SIZE;
	while (n < size) {
		i += 1;
		n <<= 1;
	}

	fl = &malloc_free_list[i];
	h = fl->head;

	if (h == 0) {
		/*
		 * Free list is empty;
		 * allocate more blocks.
		 */

		rthreads_more_memory(n, fl);
		h = fl->head;
		if (h == 0) {
			/*
			 * Allocation failed.
			 */
			return 0;
		}
	}

	/*
	 * Pop block from free list.
	 */
	fl->head = h->next;

#ifdef	DEBUG
	fl->in_use += 1;
#endif	/* DEBUG */

	/*
	 * Store free list pointer in block header
	 * so we can figure out where it goes
	 * at free() time.
	 */
	h->fl = fl;

	/*
	 * Return pointer past the block header.
	 */
	return ((char *) h) + sizeof(union header);
}

void
rthreads_free(void *base)
{
	register header_t h;
	register free_list_t fl;
	register int i;

	if (base == 0)
		return;
	/*
	 * Find free list for block.
	 */

	h = ((header_t) base) - 1;
	fl = h->fl;
	i = fl - malloc_free_list;

	/*
	 * Sanity checks.
	 */

	if (i < 0 || i >= NBUCKETS)
		return;

	if (fl != &malloc_free_list[i])
		return;

	/*
	 * Push block on free list.
	 */

	h->next = fl->head;
	fl->head = h;

#ifdef	DEBUG
	fl->in_use -= 1;
#endif	/* DEBUG */

	return;
}

void *
rthreads_realloc(void *old_base, unsigned int new_size)
{
	register header_t h;
	register free_list_t fl;
	register int i;
	unsigned int old_size;
	char *new_base;

	if (old_base == 0)
		return 0;
	/*
	 * Find size of old block.
	 */

	h = ((header_t) old_base) - 1;
	fl = h->fl;
	i = fl - malloc_free_list;

	/*
	 * Sanity checks.
	 */

	if (i < 0 || i >= NBUCKETS)
		return 0;

	if (fl != &malloc_free_list[i])
		return 0;

	/*
	 * Free list with index i contains blocks of size 2^(i+3)
	 * including header.
	 */

	old_size = (1 << (i+3)) - sizeof(union header);

	/*
	 * Allocate new block, copy old bytes, and free old block.
	 */

	new_base = rthreads_malloc(new_size);
	if (new_base != 0)
		memcpy(new_base,
		       old_base,
		       (int) (old_size < new_size ? old_size : new_size));

	rthreads_free(old_base);

	return (new_base);
}
