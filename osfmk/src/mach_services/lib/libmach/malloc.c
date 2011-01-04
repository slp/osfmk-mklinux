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
/* CMU_HIST */
/*
 * Revision 2.2  92/01/03  19:57:39  dbg
 * 	Rewrote for default pager use.
 * 	[91/10/01            dbg]
 * 
 * Revision 2.9  91/05/14  16:43:17  mrt
 * 	Correcting copyright
 * 
 * Revision 2.8  91/03/16  14:50:37  rpd
 * 	Updated for new kmem_alloc interface.
 * 	[91/03/03            rpd]
 * 
 * Revision 2.7  91/02/05  17:27:22  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:14:12  mrt]
 * 
 * Revision 2.6  90/06/19  22:59:06  rpd
 * 	Made the big kalloc zones collectable.
 * 	[90/06/05            rpd]
 * 
 * Revision 2.5  90/06/02  14:54:47  rpd
 * 	Added kalloc_max, kalloc_map_size.
 * 	[90/03/26  22:06:39  rpd]
 * 
 * Revision 2.4  90/01/11  11:43:13  dbg
 * 	De-lint.
 * 	[89/12/06            dbg]
 * 
 * Revision 2.3  89/09/08  11:25:51  dbg
 * 	MACH_KERNEL: remove non-MACH data types.
 * 	[89/07/11            dbg]
 * 
 * Revision 2.2  89/08/31  16:18:59  rwd
 * 	First Checkin
 * 	[89/08/23  15:41:37  rwd]
 * 
 * Revision 2.6  89/08/02  08:03:28  jsb
 * 	Make all kalloc zones 8 MB big. (No more kalloc panics!)
 * 	[89/08/01  14:10:17  jsb]
 * 
 * Revision 2.4  89/04/05  13:03:10  rvb
 * 	Guarantee a zone max of at least 100 elements or 10 pages
 * 	which ever is greater.  Afs (AllocDouble()) puts a great demand
 * 	on the 2048 zone and used to blow away.
 * 	[89/03/09            rvb]
 * 
 * Revision 2.3  89/02/25  18:04:39  gm0w
 * 	Changes for cleanup.
 * 
 * Revision 2.2  89/01/18  02:07:04  jsb
 * 	Give each kalloc zone a meaningful name (for panics);
 * 	create a zone for each power of 2 between MINSIZE
 * 	and PAGE_SIZE, instead of using (obsoleted) NQUEUES.
 * 	[89/01/17  10:16:33  jsb]
 * 
 *
 * 13-Feb-88  John Seamons (jks) at NeXT
 *	Updated to use kmem routines instead of vmem routines.
 *
 * 21-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */
/* CMU_ENDHIST */
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

/*
 *	All allocations of size less than kalloc_max are rounded to the
 *	next highest power of 2.
 */
static vm_size_t	kalloc_max;	/* max before we use vm_allocate */
#define		MINSIZE	8		/* minimum allocation size */

union header {
	vm_size_t	size;		/* size of item */
	union header	*next;		/* next item on free list */
	double		align;		/* unused - force alignment */
};

#define	KLIST_MAX	12
					/* sizes: 8, 16, 32, 64,
						128, 256, 512, 1024,
						2048, 4096, 8192, 16384 */
static union header	kfree_list[KLIST_MAX];

static vm_offset_t	kalloc_next_space = 0;
static vm_offset_t	kalloc_end_of_space = 0;

static vm_size_t	kalloc_wasted_space = 0;

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

	/*
	 * Support free lists for items up to vm_page_size or
	 * 16Kbytes, whichever is less.
	 */

	if (vm_page_size > 16*1024)
		kalloc_max = 16*1024;
	else
		kalloc_max = vm_page_size;

	for (i = 0; i < KLIST_MAX; i++)
	    kfree_list[i].next = 0;

	/*
	 * Do not allocate memory at address 0.
	 */
	kalloc_next_space = vm_page_size;
	kalloc_end_of_space = vm_page_size;
}

/*
 * Contiguous space allocator for items of less than a page size.
 */
static union header *
kget_space(vm_offset_t allocsize)
{
	vm_size_t	space_to_add = 0;
	vm_offset_t	new_space = 0;
	union header	*addr;

	while (kalloc_next_space + allocsize > kalloc_end_of_space) {
	    /*
	     * Add at least one page to allocation area.
	     */
	    space_to_add = round_page(allocsize);

	    if (new_space == 0) {
		/*
		 * Allocate memory.
		 * Try to make it contiguous with the last
		 * allocation area.
		 */
		new_space = kalloc_end_of_space;
		if (vm_map(mach_task_self(),
			   &new_space, space_to_add, (vm_offset_t) 0, TRUE,
			   MEMORY_OBJECT_NULL, (vm_offset_t) 0, FALSE,
			   VM_PROT_DEFAULT, VM_PROT_ALL, VM_INHERIT_DEFAULT)
			!= KERN_SUCCESS)
		    return 0;
		continue;
	    }

	    /*
	     * Memory was allocated in a previous iteration.
	     * Check whether the new region is contiguous with the
	     * old one.
	     */
	    if (new_space != kalloc_end_of_space) {
		/*
		 * Throw away the remainder of the old space,
		 * and start a new one.
		 */
		kalloc_wasted_space +=
			kalloc_end_of_space - kalloc_next_space;
		kalloc_next_space = new_space;
	    }
	    kalloc_end_of_space = new_space + space_to_add;

	    new_space = 0;
	}

	addr = (union header *)kalloc_next_space;
	kalloc_next_space += allocsize;

	if (new_space != 0)
	    (void) vm_deallocate(mach_task_self(), new_space, space_to_add);

	return addr;
}

static vm_size_t
get_allocsize(size_t size, union header **flp)
{
	vm_size_t allocsize;
	union header *fl;

	if (size >= kalloc_max)
	    return round_page(size);
	allocsize = MINSIZE;
	fl = kfree_list;
	while (allocsize < size) {
	    allocsize <<= 1;
	    fl++;
	}
	if (flp != NULL)
	    *flp = fl;
	return allocsize;
}


void *
malloc(size_t size)
{
	register vm_size_t allocsize;
	union header *addr;
	union header *fl;

	if (size <= 0)
		return NULL;

	if (!kalloc_initialized) {
	    kalloc_init();
	    kalloc_initialized = TRUE;
	}

	/* compute the size of the block that we will actually allocate */

	size += sizeof(union header);
	allocsize = get_allocsize(size, &fl);

	/*
	 * If our size is still small enough, check the queue for that size
	 * and allocate.
	 */

	if (allocsize < kalloc_max) {
	    if ((addr = fl->next) != 0) {
		fl->next = addr->next;
	    }
	    else {
		addr = kget_space(allocsize);
	    }
	}
	else {
	    /* This will allocate page 0 if it is free, but the header
	       will prevent us from returning a 0 pointer.  */
	    if (vm_allocate(mach_task_self(), (vm_offset_t *)&addr,
			    allocsize, TRUE) != KERN_SUCCESS)
		return(0);
	}

	addr->size = allocsize;
	return (void *) (addr + 1);
}

void
free(void *data)
{
	register vm_size_t freesize;
	union header *fl;
	union header *addr = ((union header *)data) - 1;

	freesize = get_allocsize(addr->size, &fl);

	if (freesize < kalloc_max) {
	    addr->next = fl->next;
	    fl->next = addr;
	}
	else {
	    (void) vm_deallocate(mach_task_self(), (vm_offset_t)addr,
				 freesize);
	}
}

void *realloc(void *data, size_t size);

/* The most common use of realloc is to manage a buffer of unlimited size
   that is grown as it fills.  So we try to optimise the case where you
   are growing the last object allocated to avoid copies.  */
void *
realloc(void *data, size_t size)
{
	void *p;
	union header *addr = ((union header *) data) - 1;
	vm_address_t vmaddr = (vm_address_t) addr;
	vm_address_t newaddr;
	vm_size_t oldsize, allocsize;
	size_t tocopy;

	if (data == NULL)
	    return malloc(size);

	oldsize = addr->size;
	allocsize = get_allocsize(size + sizeof(union header), NULL);
	if (allocsize == oldsize)
	    return data;

	/* Deal with every case where we don't want to do a simple
	   malloc+memcpy+free.  Otherwise it is a "simple case" in the
	   comments.  */
	if (allocsize < oldsize) {
	    /* Shrinking.  We favour space over time here since if time is
	       really important you can just not do the realloc.  */
	    if (oldsize >= kalloc_max) {
		/* Shrinking a lot.  */
		if (allocsize >= kalloc_max) {
		    (void) vm_deallocate(mach_task_self(), vmaddr + allocsize,
					 oldsize - allocsize);
		    addr->size = allocsize;
		    return data;
		}
		/* Simple case: shrinking from a whole page or pages to less
		   than a page.  */
	    } else {
		if (vmaddr + oldsize == kalloc_next_space) {
		    /* Shrinking the last item in the current page.  */
		    kalloc_next_space = vmaddr + allocsize;
		    addr->size = allocsize;
		    return data;
		}
		/* Simple case: shrinking enough to fit in a smaller power
		   of two.  */
	    }
	    tocopy = size;
	} else {
	    /* Growing.  */
	    if (allocsize >= kalloc_max) {
		/* Growing a lot.  */
		if (oldsize >= kalloc_max) {
		    /* We could try to vm_allocate extra pages after the old
		       data, but vm_allocate + vm_copy is not much more
		       expensive than that, even if it does fragment the
		       address space a bit more.  */
		    newaddr = vmaddr;
		    if (vm_allocate(mach_task_self(), &newaddr, allocsize,
				    TRUE) != KERN_SUCCESS ||
			vm_copy(mach_task_self(), vmaddr, oldsize, newaddr)
			!= KERN_SUCCESS)
			return NULL;
		    (void) vm_deallocate(mach_task_self(), vmaddr, oldsize);
		    addr = (union header *) newaddr;
		    addr->size = allocsize;
		    return (void *) (addr + 1);
		}
		/* Simple case: growing from less than a page to one or more
		   whole pages.  */
	    } else {
		/* Growing from a within-page size to a larger within-page
		   size.  Frequently the item being grown is the last one
		   allocated so try to avoid copies in that case.  */
		if (vmaddr + oldsize == kalloc_next_space) {
		    if (vmaddr + allocsize <= kalloc_end_of_space) {
			kalloc_next_space = vmaddr + allocsize;
			addr->size = allocsize;
			return data;
		    } else {
			newaddr = round_page(vmaddr);
			if (vm_allocate(mach_task_self(), &newaddr,
					vm_page_size, FALSE)
			    == KERN_SUCCESS) {
			    kalloc_next_space = vmaddr + allocsize;
			    kalloc_end_of_space = newaddr + vm_page_size;
			    addr->size = allocsize;
			    return (void *) (addr + 1);
			}
			/* Simple case: growing the last object in the page
			   past the end of the page when the next page is
			   unavailable.  */
		    }
		}
		/* Simple case: growing a within-page object that is not the
		   last object allocated. */
	    }
	    tocopy = oldsize - sizeof(union header);
	}

	/* So if we get here, we can't do any better than this: */
	p = malloc(size);
	if (p != NULL) {
	    memcpy(p, data, tocopy);
	    free(data);
	}
	return p;
}

void *calloc(size_t nmemb, size_t size);
void *calloc(size_t nmemb, size_t size)
{
	void *addr = malloc(nmemb * size);
	if (addr != NULL)
		memset(addr, 0, nmemb * size);
	return addr;
}

/* This is in linux libc too - sigh */
void cfree(void *ptr);

void cfree(void *ptr)
{
	free(ptr);
}
