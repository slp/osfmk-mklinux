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
 * Revision 2.7  91/05/14  17:48:23  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/02/05  17:57:44  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:31:19  mrt]
 * 
 * Revision 2.5  91/01/08  16:44:33  rpd
 * 	Changed zchange calls to make the zones non-collectable.
 * 	[90/12/29            rpd]
 * 
 * Revision 2.4  90/12/20  17:05:13  jeffreyh
 * 	Change zchange to accept new argument. Made zones collectable.
 * 	[90/12/11            jeffreyh]
 * 
 * Revision 2.3  90/05/29  18:38:33  rwd
 * 	Picked up rfr changes.
 * 	[90/04/12  13:46:29  rwd]
 * 
 * Revision 2.2  90/01/11  11:47:26  dbg
 * 	Add changes from mainline:
 * 		Fixed off-by-one error in vm_external_create.
 * 		[89/12/18  23:40:56  rpd]
 * 
 * 		Keep the bitmap size info around, as it may travel across
 * 		objects.  Reflect this in vm_external_destroy().
 * 		[89/10/16  15:32:06  af]
 * 
 * 		Removed lint.
 * 		[89/08/07            mwyoung]
 * 
 * Revision 2.1  89/08/03  16:44:41  rwd
 * Created.
 * 
 * Revision 2.3  89/04/18  21:24:49  mwyoung
 * 	Created.
 * 	[89/04/18            mwyoung]
 * 
 */
/* CMU_ENDHIST */
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
 */

/*
 *	This module maintains information about the presence of
 *	pages not in memory.  Since an external memory object
 *	must maintain a complete knowledge of its contents, this
 *	information takes the form of hints.
 */
#include <string.h>	/* for memcpy()/memset() */

#include <mach/boolean.h>
#include <vm/vm_external.h>
#include <kern/kalloc.h>
#include <mach/vm_param.h>
#include <kern/assert.h>

/*
 *	The implementation uses bit arrays to record whether
 *	a page has been written to external storage.  For
 *	convenience, these bit arrays come in various sizes.
 *	For example, a map N bytes long can record:
 *
 *	  16 bytes =   128 pages = (@ 4KB/page)    512KB
 *	1024 bytes =  8192 pages = (@ 4KB/page)  32MB
 *	4096 bytes = 32768 pages = (@ 4KB/page) 128MB
 *
 *	For a 32-bit machine with 4KB pages, the largest size
 *	would be 128KB = 32 pages. Machines with a larger page
 *	size are more efficient.
 *
 *	This subsystem must be very careful about memory allocation,
 *	since vm_external_create() is almost always called with
 *	vm_privilege set. The largest map to be allocated must be less
 *	than or equal to a single page, and the kalloc subsystem must
 *	never allocate more than a single page in response to a kalloc()
 *	request. Also, vm_external_destroy() must not take any blocking
 *	locks, since it is called with a vm_object lock held. This
 *	implies that kfree() MUST be implemented in terms of zfree()
 *	NOT kmem_free() for all request sizes that this subsystem uses.
 *
 *	For efficiency, this subsystem knows that the kalloc() subsystem
 *	is implemented in terms of power-of-2 allocation, and that the
 *	minimum allocation unit is KALLOC_MINSIZE
 * 
 *	XXXO
 *	Should consider using existence_map to hold bits directly
 *	when existence_size <= 4 bytes (i.e., 32 pages).
 */

#define	SMALL_SIZE	KALLOC_MINSIZE
#define	LARGE_SIZE	PAGE_SIZE

static vm_size_t power_of_2(vm_size_t size);

static vm_size_t
power_of_2(vm_size_t size)
{
	vm_size_t power;

	power = 2 * SMALL_SIZE;
	while (power < size) {
		power <<= 1;
	}
	return(power);
}

vm_external_map_t
vm_external_create(
	vm_offset_t	size)
{
	vm_size_t		bytes;
	vm_external_map_t	result = VM_EXTERNAL_NULL;

	bytes = stob(size);
	if (bytes <= SMALL_SIZE) {
		if ((result = (vm_external_map_t)kalloc(SMALL_SIZE)) != NULL) {
			memset(result, 0, SMALL_SIZE);
		}
	} else if (bytes <= LARGE_SIZE) {
		bytes = power_of_2(bytes);

		if ((result = (vm_external_map_t)kalloc(bytes)) != NULL) {
			memset(result, 0, bytes);
		}
	}
	return(result);
}

void
vm_external_destroy(
	vm_external_map_t	map,
	vm_size_t		size)
{
	vm_size_t bytes;

	if (map == VM_EXTERNAL_NULL)
		return;

	bytes = stob(size);
	if (bytes <= SMALL_SIZE) {
		bytes = SMALL_SIZE;
	} else {
		bytes = power_of_2(bytes);
	}
	kfree((vm_offset_t)map, bytes);
}

/*
 * Return the number of bytes needed for a vm_external_map given the
 * size of the object to be mapped, i.e. the size of the map that was
 * created by vm_external_create.
 */
vm_size_t
vm_external_map_size(
	vm_offset_t	size)
{
	vm_size_t	bytes;

	bytes = stob(size);
	if (bytes != 0)
	        if (bytes <= SMALL_SIZE) {
			bytes = SMALL_SIZE;
		} else {
			bytes = power_of_2(bytes);
		}
	return bytes;
}

void
vm_external_copy(
	vm_external_map_t	old_map,
	vm_size_t		old_size,
	vm_external_map_t	new_map)
{
	/*
	 * Cannot copy non-existent maps
	 */
	if ((old_map == VM_EXTERNAL_NULL) || (new_map == VM_EXTERNAL_NULL))
		return;

	/*
	 * Copy old map to new
	 */
	memcpy(new_map, old_map, stob(old_size));
}

boolean_t
vm_external_within(
	vm_size_t	new_size,
	vm_size_t	old_size)
{
	vm_size_t 	new_bytes;
	vm_size_t 	old_bytes;

	assert(new_size >= old_size);

	/*
	 * "old_bytes" is calculated to be the actual amount of space
	 * allocated for a map of size "old_size".
	 */
	old_bytes = stob(old_size);
	if (old_bytes <= SMALL_SIZE) old_bytes = SMALL_SIZE;
	else if (old_bytes <= LARGE_SIZE) old_bytes = power_of_2(old_bytes);

	/*
	 * "new_bytes" is the map size required to map the "new_size" object.
	 * Since the rounding algorithms are the same, we needn't actually
	 * round up new_bytes to get the correct answer
	 */
	new_bytes = stob(new_size);

	return(new_bytes <= old_bytes);
}

vm_external_state_t
_vm_external_state_get(
	vm_external_map_t	map,
	vm_offset_t		offset)
{
	unsigned
	int		bit, byte;

	assert (map != VM_EXTERNAL_NULL);

	bit = atop(offset);
	byte = bit >> 3;
	if (map[byte] & (1 << (bit & 07))) {
		return VM_EXTERNAL_STATE_EXISTS;
	} else {
		return VM_EXTERNAL_STATE_ABSENT;
	}
}

void
vm_external_state_set(
	vm_external_map_t	map,
	vm_offset_t		offset)
{
	unsigned
	int		bit, byte;

	if (map == VM_EXTERNAL_NULL)
		return;

	bit = atop(offset);
	byte = bit >> 3;
	map[byte] |= (1 << (bit & 07));
}

void	
vm_external_module_initialize(void)
{
}
