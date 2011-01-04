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
 */
/*
 *	File:	kern/kalloc.c
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1985
 *
 *	General kernel memory allocator.  This allocator is designed
 *	to be used by the kernel to manage dynamic memory fast.
 */

#include <zone_debug.h>

#include <mach/boolean.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_param.h>
#include <kern/misc_protos.h>
#include <kern/zalloc.h>
#include <kern/kalloc.h>
#include <kern/lock.h>
#include <vm/vm_kern.h>
#include <vm/vm_object.h>
#include <vm/vm_map.h>

vm_map_t kalloc_map;
vm_size_t kalloc_map_size = 8 * 1024 * 1024;
vm_size_t kalloc_max;
vm_size_t kalloc_max_prerounded;

/*
 *	All allocations of size less than kalloc_max are rounded to the
 *	next highest power of 2.  This allocator is built on top of
 *	the zone allocator.  A zone is created for each potential size
 *	that we are willing to get in small blocks.
 *
 *	We assume that kalloc_max is not greater than 64K;
 *	thus 16 is a safe array size for k_zone and k_zone_name.
 *
 *	Note that kalloc_max is somewhat confusingly named.
 *	It represents the first power of two for which no zone exists.
 *	kalloc_max_prerounded is the smallest allocation size, before
 *	rounding, for which no zone exists.
 */

int first_k_zone = -1;
struct zone *k_zone[16];
static char *k_zone_name[16] = {
	"kalloc.1",		"kalloc.2",
	"kalloc.4",		"kalloc.8",
	"kalloc.16",		"kalloc.32",
	"kalloc.64",		"kalloc.128",
	"kalloc.256",		"kalloc.512",
	"kalloc.1024",		"kalloc.2048",
	"kalloc.4096",		"kalloc.8192",
	"kalloc.16384",		"kalloc.32768"
};

/*
 *  Max number of elements per zone.  zinit rounds things up correctly
 *  Doing things this way permits each zone to have a different maximum size
 *  based on need, rather than just guessing; it also
 *  means its patchable in case you're wrong!
 */
unsigned long k_zone_max[16] = {
      1024,		/*      1 Byte  */
      1024,		/*      2 Byte  */
      1024,		/*      4 Byte  */
      1024,		/*      8 Byte  */
      1024,		/*     16 Byte  */
      4096,		/*     32 Byte  */
      4096,		/*     64 Byte  */
      4096,		/*    128 Byte  */
      4096,		/*    256 Byte  */
      1024,		/*    512 Byte  */
      1024,		/*   1024 Byte  */
      1024,		/*   2048 Byte  */
      1024,		/*   4096 Byte  */
      4096,		/*   8192 Byte  */
      64,		/*  16384 Byte  */
      64,		/*  32768 Byte  */
};

/*
 *	Initialize the memory allocator.  This should be called only
 *	once on a system wide basis (i.e. first processor to get here
 *	does the initialization).
 *
 *	This initializes all of the zones.
 */

void
kalloc_init(
	void)
{
	kern_return_t retval;
	vm_offset_t min;
	vm_size_t size;
	register int i;

	retval = kmem_suballoc(kernel_map, &min, kalloc_map_size,
			       FALSE, TRUE, &kalloc_map);
	if (retval != KERN_SUCCESS)
		panic("kalloc_init: kmem_suballoc failed");

	/*
	 *	Ensure that zones up to size 8192 bytes exist.
	 *	This is desirable because messages are allocated
	 *	with kalloc, and messages up through size 8192 are common.
	 */

	if (PAGE_SIZE < 16*1024)
		kalloc_max = 16*1024;
	else
		kalloc_max = PAGE_SIZE;
	kalloc_max_prerounded = kalloc_max / 2 + 1;

	/*
	 *	Allocate a zone for each size we are going to handle.
	 *	We specify non-paged memory.
	 */
	for (i = 0, size = 1; size < kalloc_max; i++, size <<= 1) {
		if (size < KALLOC_MINSIZE) {
			k_zone[i] = 0;
			continue;
		}
		if (size == KALLOC_MINSIZE) {
			first_k_zone = i;
		}
		k_zone[i] = zinit(size, k_zone_max[i] * size, size,
				  k_zone_name[i]);
	}
}

vm_offset_t
kalloc(
	vm_size_t	size)
{
	register int zindex;
	register vm_size_t allocsize;

	/*
	 * If size is too large for a zone, then use kmem_alloc.
	 * (We use kmem_alloc instead of kmem_alloc_wired so that
	 * krealloc can use kmem_realloc.)
	 */

	if (size >= kalloc_max_prerounded) {
		vm_offset_t addr;

		if (kmem_alloc(kalloc_map, &addr, size) != KERN_SUCCESS)
			addr = 0;
		return(addr);
	}

	/* compute the size of the block that we will actually allocate */

	allocsize = KALLOC_MINSIZE;
	zindex = first_k_zone;
	while (allocsize < size) {
		allocsize <<= 1;
		zindex++;
	}

	/* allocate from the appropriate zone */

	assert(allocsize < kalloc_max);
	return(zalloc(k_zone[zindex]));
}

void
krealloc(
	vm_offset_t	*addrp,
	vm_size_t	old_size,
	vm_size_t	new_size,
	simple_lock_t	lock)
{
	register int zindex;
	register vm_size_t allocsize;
	vm_offset_t naddr;

	/* can only be used for increasing allocation size */

	assert(new_size > old_size);

	/* if old_size is zero, then we are simply allocating */

	if (old_size == 0) {
		simple_unlock(lock);
		naddr = kalloc(new_size);
		simple_lock(lock);
		*addrp = naddr;
		return;
	}

	/* if old block was kmem_alloc'd, then use kmem_realloc if necessary */

	if (old_size >= kalloc_max_prerounded) {
		old_size = round_page(old_size);
		new_size = round_page(new_size);
		if (new_size > old_size) {

			if (kmem_realloc(kalloc_map, *addrp, old_size, &naddr,




					 new_size) != KERN_SUCCESS) {
				panic("krealloc: kmem_realloc");
				naddr = 0;
			}

			simple_lock(lock);
			*addrp = naddr;

			/* kmem_realloc() doesn't free old page range. */
			kmem_free(kalloc_map, *addrp, old_size);
		}
		return;
	}

	/* compute the size of the block that we actually allocated */

	allocsize = KALLOC_MINSIZE;
	zindex = first_k_zone;
	while (allocsize < old_size) {
		allocsize <<= 1;
		zindex++;
	}

	/* if new size fits in old block, then return */

	if (new_size <= allocsize) {
		return;
	}

	/* if new size does not fit in zone, kmem_alloc it, else zalloc it */

	simple_unlock(lock);
	if (new_size >= kalloc_max_prerounded) {
		if (kmem_alloc(kalloc_map, &naddr, new_size) != KERN_SUCCESS) {
			panic("krealloc: kmem_alloc");
			simple_lock(lock);
			*addrp = 0;
			return;
		}
	} else {
		register int new_zindex;

		allocsize <<= 1;
		new_zindex = zindex + 1;
		while (allocsize < new_size) {
			allocsize <<= 1;
			new_zindex++;
		}
		naddr = zalloc(k_zone[new_zindex]);
	}
	simple_lock(lock);

	/* copy existing data */

	bcopy((const char *)*addrp, (char *)naddr, old_size);

	/* free old block, and return */

	zfree(k_zone[zindex], *addrp);

	/* set up new address */

	*addrp = naddr;
}


vm_offset_t
kget(
	vm_size_t	size)
{
	register int zindex;
	register vm_size_t allocsize;

	/* size must not be too large for a zone */

	if (size >= kalloc_max_prerounded) {
		/* This will never work, so we might as well panic */
		panic("kget");
	}

	/* compute the size of the block that we will actually allocate */

	allocsize = KALLOC_MINSIZE;
	zindex = first_k_zone;
	while (allocsize < size) {
		allocsize <<= 1;
		zindex++;
	}

	/* allocate from the appropriate zone */

	assert(allocsize < kalloc_max);
	return(zget(k_zone[zindex]));
}

void
kfree(
	vm_offset_t	data,
	vm_size_t	size)
{
	register int zindex;
	register vm_size_t freesize;

	/* if size was too large for a zone, then use kmem_free */

	if (size >= kalloc_max_prerounded) {
		kmem_free(kalloc_map, data, size);
		return;
	}

	/* compute the size of the block that we actually allocated from */

	freesize = KALLOC_MINSIZE;
	zindex = first_k_zone;
	while (freesize < size) {
		freesize <<= 1;
		zindex++;
	}

	/* free to the appropriate zone */

	assert(freesize < kalloc_max);
	zfree(k_zone[zindex], data);
}
