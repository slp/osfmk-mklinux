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
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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
 *	File:	kern/kalloc.c
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1985
 *
 *	General kernel memory allocator.  This allocator is designed
 *	to be used by the kernel to manage dynamic memory fast.
 */

#include <mach.h>
#include <stdlib.h>
#include <boot.h>

/*
 *	All allocations of size less than kalloc_max are rounded to the
 *	next highest power of 2.
 */
static vm_size_t	kalloc_max;	/* max before we use vm_allocate */
#define		MINSIZE	8		/* minimum allocation size */

union header {
	vm_size_t	size;		/* size of item */
	union header	*next;		/* next item on free list */
};

#define	KLIST_MAX	12
					/* sizes: 8, 16, 32, 64,
						128, 256, 512, 1024,
						2048, 4096, 8192, 16384 */
static union header	kfree_list[KLIST_MAX];

static vm_offset_t	kalloc_next_space = 0;

static boolean_t	kalloc_initialized = FALSE;

/*
 *	Initialize the memory allocator.  This should be called only
 *	once on a system wide basis (i.e. first processor to get here
 *	does the initialization).
 *
 *	This initializes all of the zones.
 */

static void
kalloc_init(void)
{
	register int i;

#if	DEBUG
	if (debug)
		printf("kalloc_init()\n");
#endif
	/*
	 * Support free lists for items up 16Kbytes
	 */

	kalloc_max = 16*1024;

	for (i = 0; i < KLIST_MAX; i++)
	    kfree_list[i].next = 0;

	kalloc_next_space = KALLOC_OFFSET;

}

/*
 * Contiguous space allocator for items of less than a page size.
 */
static union header *
kget_space(vm_offset_t allocsize)
{
	union header	*addr;

#if	DEBUG
	if (debug)
		printf("kget_space(%x)\n", allocsize);
#endif
	addr = (union header *)kalloc_next_space;
	kalloc_next_space += allocsize;

#if	DEBUG
	if (debug)
		printf("kget_space() returns %x\n",  addr);
#endif
	return addr;
}

void *
malloc(size_t size)
{
	register vm_size_t allocsize;
	union header *addr;
	register union header *fl;

#if	DEBUG
	if (debug)
		printf("malloc(%x)\n", size);
#endif
	if (!kalloc_initialized) {
	    kalloc_init();
	    kalloc_initialized = TRUE;
	}

	/* compute the size of the block that we will actually allocate */

	size += sizeof(union header);
	allocsize = size;
	if (size < kalloc_max) {
	    allocsize = MINSIZE;
	    fl = kfree_list;
	    while (allocsize < size) {
		allocsize <<= 1;
		fl++;
	    }
	} else {
		printf("malloc more than 16 K !!!\n");
		for(;;);
	}

	/*
	 * Check the queue for that size
	 * and allocate.
	 */

	if ((addr = fl->next) != 0) {
	    fl->next = addr->next;
	}
        else {
	    addr = kget_space(allocsize);
	}

	addr->size = allocsize;
#if	DEBUG
	if (debug)
		printf("malloc() returns %x\n", (void *) (addr + 1));
#endif
	return (void *) (addr + 1);
}

void
free(void *data)
{
	register vm_size_t freesize;
	register union header *fl;
	union header *addr = ((union header *)data) - 1;

#if	DEBUG
	if (debug)
		printf("free(%x)\n", data);
#endif
	freesize = addr->size;
	if (freesize > kalloc_max) {
		printf("free more than 16 K !!!\n");
		for(;;);
	}
	freesize = MINSIZE;
	fl = kfree_list;
	while (freesize < addr->size) {
	    freesize <<= 1;
	    fl++;
	}

    	addr->next = fl->next;
	fl->next = addr;
}
