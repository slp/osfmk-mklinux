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
 * Revision 2.13.3.5  92/09/15  17:37:01  jeffreyh
 * 	Replace vm_map_copyin with vm_map_copyin_common and add
 * 	invocation macros (vm_map_copyin, vm_map_copyin_maxprot).
 * 	[92/08/31            dlb]
 * 	Add pages_loose boolean to page_list vm_map_copy_t.
 * 	[92/06/30            dlb]
 * 
 * 	Use pageable copy objects for large NORMA messages,
 * 	which also requires improved technology for converting
 * 	copy objects from object flavor to page list flavor.
 * 	Define index field for object flavor to record progress
 * 	during conversions; define VM_MAP_COPY_PAGE_LIST_MAX_SIZE
 * 	for convenience.
 * 	[92/06/02            alanl]
 * 
 * Revision 2.13.3.4  92/03/28  10:17:15  jeffreyh
 * 	19-Mar-92 David L. Black (dlb) at Open Software Foundation
 * 	Add extend_cont continuation invocation macro to invoke
 * 	continuation without affecting current copy.  Declare
 * 	vm_map_copy_discard_cont.
 * 	[92/03/20  14:15:53  jeffreyh]
 * 
 * Revision 2.13.3.3  92/03/03  16:26:49  jeffreyh
 * 	Change unused wiring_allowed field to wiring_required
 * 	in vm_map data structure.
 * 	[92/02/20  15:19:12  dlb]
 * 
 * 	Add is_shared bit to map entry to detect sharing.
 * 	[92/02/19  14:26:45  dlb]
 * 
 * 	Remove all sharing map structure elements.
 * 	Make vm_map_verify_done() a macro.
 * 	[92/01/07  11:14:16  dlb]
 * 
 * Revision 2.13.3.2  92/02/18  19:24:53  jeffreyh
 * 	[intel] page list size expanded for iPSC as a vm workaround.
 * 	[92/02/13  13:12:01  jeffreyh]
 * 
 * Revision 2.13.3.1  92/01/09  18:46:47  jsb
 * 	Removed NORMA_IPC workaround.
 * 	[92/01/09  00:03:40  jsb]
 * 
 * Revision 2.13  91/12/10  13:27:03  jsb
 * 	5-Dec-91 David L. Black (dlb) at Open Software Foundation
 * 	Simplify page list continuation abort logic. Temporarily
 * 	increase size of page lists for NORMA_IPC until it supports
 * 	page list continuations.
 * 	[91/12/10  12:50:01  jsb]
 * 
 * Revision 2.12.1.1  91/12/05  10:57:48  dlb
 * 	 4-Dec-91 David L. Black (dlb) at Open Software Foundation
 * 	Fix type of null pointer in continuation invocation in
 * 	vm_map_copy_abort_cont.
 * 
 * Revision 2.12  91/08/28  11:18:33  jsb
 * 	Supplied missing argument to thread_block in vm_map_entry_wait.
 * 	[91/08/16  10:36:21  jsb]
 * 
 * 	Minor cleanups.
 * 	[91/08/06  17:26:24  dlb]
 * 
 * 	Discard pages before invoking or aborting a continuation.
 * 	[91/08/05  17:51:55  dlb]
 * 
 * 	Add declarations for in transition map entries and vm_map_copy
 * 	continuations.
 * 	[91/07/30  14:18:14  dlb]
 * 
 * Revision 2.11  91/07/01  08:27:43  jsb
 * 	18-Jun-91 David L. Black (dlb) at Open Software Foundation
 * 		Declarations for multiple-format vm map copy support.
 * 	[91/06/29  14:36:42  jsb]
 * 
 * Revision 2.10  91/05/18  14:41:09  rpd
 * 	Added kentry_data and friends, for vm_map_init.
 * 	[91/03/22            rpd]
 * 
 * Revision 2.9  91/05/14  17:50:06  mrt
 * 	Correcting copyright
 * 
 * Revision 2.8  91/03/16  15:06:07  rpd
 * 	Removed vm_map_find.  Added vm_map_find_entry.
 * 	[91/03/03            rpd]
 * 
 * Revision 2.7  91/02/05  17:59:07  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:33:15  mrt]
 * 
 * Revision 2.6  90/10/12  13:06:08  rpd
 * 	Removed copy_on_write field.
 * 	[90/10/08            rpd]
 * 
 * Revision 2.5  90/06/19  23:02:32  rpd
 * 	Picked up vm_submap_object.
 * 	[90/06/08            rpd]
 * 
 * Revision 2.4  90/06/02  15:11:24  rpd
 * 	New vm_map_pageable, with user_wired_count.
 * 	[90/03/26  23:15:58  rpd]
 * 
 * Revision 2.3  90/02/22  20:06:19  dbg
 * 	Combine fields in vm_map and vm_map_copy into a vm_map_header
 * 	structure.
 * 	[90/01/29            dbg]
 * 
 * 	Add changes from mainline:
 * 
 * 		Added documentation for exported routines.
 * 		Add vm_map_t->wait_for_space field.
 * 		Add vm_map_copy_t type, associated routine declarations, and
 * 		documentation.
 * 		Introduced vm_map_links, which contains those map entry fields
 * 		used in the map structure.
 * 		[89/08/31  21:13:56  rpd]
 * 
 * 		Optimization from NeXT:  is_a_map, is_sub_map, copy_on_write,
 * 		needs_copy are now bit-fields.
 * 		[89/08/19  23:44:53  rpd]
 * 
 * Revision 2.2  90/01/22  23:09:35  af
 * 	Added vm_map_machine_attribute() decl.
 * 
 * 	Changes for MACH_KERNEL:
 * 	. Added wiring_allowed to map.
 * 	[89/04/29            dbg]
 * 
 * Revision 2.1  89/08/03  16:45:29  rwd
 * Created.
 * 
 * Revision 2.9  89/04/18  21:26:14  mwyoung
 * 	Reset history.  All relevant material is in the documentation
 * 	here, and in the implementation file ("vm/vm_map.c").
 * 	[89/04/18            mwyoung]
 * 
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
 *	File:	vm/vm_map.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Virtual memory map module definitions.
 *
 * Contributors:
 *	avie, dlb, mwyoung
 */

#ifndef	_VM_VM_MAP_H_
#define _VM_VM_MAP_H_

#include <norma_vm.h>
#include <dipc.h>
#include <mach_rt.h>
#include <cpus.h>
#include <task_swapper.h>
#include <mach_assert.h>

#include <mach/kern_return.h>
#include <mach/boolean.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_inherit.h>
#include <mach/vm_behavior.h>
#include <vm/pmap.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <kern/lock.h>
#include <kern/zalloc.h>
#include <kern/macro_help.h>

/*
 *	Types defined:
 *
 *	vm_map_t		the high-level address map data structure.
 *	vm_map_entry_t		an entry in an address map.
 *	vm_map_version_t	a timestamp of a map, for use with vm_map_lookup
 *	vm_map_copy_t		represents memory copied from an address map,
 *				 used for inter-map copy operations
 */

/*
 *	Type:		vm_map_object_t [internal use only]
 *
 *	Description:
 *		The target of an address mapping, either a virtual
 *		memory object or a sub map (of the kernel map).
 */
typedef union vm_map_object {
	struct vm_object	*vm_object;	/* object object */
	struct vm_map		*sub_map;	/* belongs to another map */
} vm_map_object_t;

/*
 *	Type:		vm_map_entry_t [internal use only]
 *
 *	Description:
 *		A single mapping within an address map.
 *
 *	Implementation:
 *		Address map entries consist of start and end addresses,
 *		a VM object (or sub map) and offset into that object,
 *		and user-exported inheritance and protection information.
 *		Control information for virtual copy operations is also
 *		stored in the address map entry.
 */
struct vm_map_links {
	struct vm_map_entry	*prev;		/* previous entry */
	struct vm_map_entry	*next;		/* next entry */
	vm_offset_t		start;		/* start address */
	vm_offset_t		end;		/* end address */
};

struct vm_map_entry {
	struct vm_map_links	links;		/* links to other entries */
#define vme_prev		links.prev
#define vme_next		links.next
#define vme_start		links.start
#define vme_end			links.end
	union vm_map_object	object;		/* object I point to */
	vm_offset_t		offset;		/* offset into object */
	unsigned int
	/* boolean_t */		is_shared:1,	/* region is shared */
	/* boolean_t */		is_sub_map:1,	/* Is "object" a submap? */
	/* boolean_t */		in_transition:1, /* Entry being changed */
	/* boolean_t */		needs_wakeup:1,  /* Waiters on in_transition */
	/* vm_behavior_t */	behavior:2,	/* user paging behavior hint */
		/* Only used when object is a vm_object: */
	/* boolean_t */		needs_copy:1,	/* object need to be copied? */
		/* Only in task maps: */
	/* vm_prot_t */		protection:3,	/* protection code */
	/* vm_prot_t */		max_protection:3,/* maximum protection */
	/* vm_inherit_t */	inheritance:2;	/* inheritance */
	unsigned short		wired_count;	/* can be paged if = 0 */
	unsigned short		user_wired_count; /* for vm_wire */
};

/*
 * wired_counts are unsigned short.  This value is used to safeguard
 * against any mishaps due to runaway user programs.
 */
#define MAX_WIRE_COUNT		65535

typedef struct vm_map_entry	*vm_map_entry_t;

#define VM_MAP_ENTRY_NULL	((vm_map_entry_t) 0)


/*
 *	Type:		struct vm_map_header
 *
 *	Description:
 *		Header for a vm_map and a vm_map_copy.
 */
struct vm_map_header {
	struct vm_map_links	links;		/* first, last, min, max */
	int			nentries;	/* Number of entries */
	boolean_t		entries_pageable;
						/* are map entries pageable? */
};

/*
 *	Type:		vm_map_t [exported; contents invisible]
 *
 *	Description:
 *		An address map -- a directory relating valid
 *		regions of a task's address space to the corresponding
 *		virtual memory objects.
 *
 *	Implementation:
 *		Maps are doubly-linked lists of map entries, sorted
 *		by address.  One hint is used to start
 *		searches again from the last successful search,
 *		insertion, or removal.  Another hint is used to
 *		quickly find free space.
 */
typedef struct vm_map {
	lock_t			lock;		/* uni- and smp-lock */
	struct vm_map_header	hdr;		/* Map entry header */
#define min_offset		hdr.links.start	/* start of range */
#define max_offset		hdr.links.end	/* end of range */
	pmap_t			pmap;		/* Physical map */
	vm_size_t		size;		/* virtual size */
	int			ref_count;	/* Reference count */
#if	TASK_SWAPPER
	int			res_count;	/* Residence count (swap) */
	int			sw_state;	/* Swap state */
#endif	/* TASK_SWAPPER */
	decl_mutex_data(,	s_lock)		/* Lock ref, res, hint fields */
	vm_map_entry_t		hint;		/* hint for quick lookups */
	vm_map_entry_t		first_free;	/* First free space hint */
	boolean_t		wait_for_space;	/* Should callers wait
						   for space? */
	boolean_t		wiring_required;/* All memory wired? */
	boolean_t		no_zero_fill;	/* No zero fill absent pages */
	unsigned int		timestamp;	/* Version number */
} *vm_map_t;

#define		VM_MAP_NULL	((vm_map_t) 0)

#define vm_map_to_entry(map)	((struct vm_map_entry *) &(map)->hdr.links)
#define vm_map_first_entry(map)	((map)->hdr.links.next)
#define vm_map_last_entry(map)	((map)->hdr.links.prev)

#if	TASK_SWAPPER
/*
 * VM map swap states.  There are no transition states.
 */
#define MAP_SW_IN	 1	/* map is swapped in; residence count > 0 */
#define MAP_SW_OUT	 2	/* map is out (res_count == 0 */
#endif	/* TASK_SWAPPER */

/*
 *	Type:		vm_map_version_t [exported; contents invisible]
 *
 *	Description:
 *		Map versions may be used to quickly validate a previous
 *		lookup operation.
 *
 *	Usage note:
 *		Because they are bulky objects, map versions are usually
 *		passed by reference.
 *
 *	Implementation:
 *		Just a timestamp for the main map.
 */
typedef struct vm_map_version {
	unsigned int	main_timestamp;
} vm_map_version_t;

/*
 *	Type:		vm_map_copy_t [exported; contents invisible]
 *
 *	Description:
 *		A map copy object represents a region of virtual memory
 *		that has been copied from an address map but is still
 *		in transit.
 *
 *		A map copy object may only be used by a single thread
 *		at a time.
 *
 *	Implementation:
 * 		There are three formats for map copy objects.  
 *		The first is very similar to the main
 *		address map in structure, and as a result, some
 *		of the internal maintenance functions/macros can
 *		be used with either address maps or map copy objects.
 *
 *		The map copy object contains a header links
 *		entry onto which the other entries that represent
 *		the region are chained.
 *
 *		The second format is a single vm object.  This is used
 *		primarily in the pageout path.  The third format is a
 *		list of vm pages.  An optional continuation provides
 *		a hook to be called to obtain more of the memory,
 *		or perform other operations.  The continuation takes 3
 *		arguments, a saved arg buffer, a pointer to a new vm_map_copy
 *		(returned) and an abort flag (abort if TRUE).
 */

#define VM_MAP_COPY_PAGE_LIST_MAX	20
#define	VM_MAP_COPY_PAGE_LIST_MAX_SIZE	(VM_MAP_COPY_PAGE_LIST_MAX * PAGE_SIZE)


/*
 *	Options for vm_map_copyin_page_list.
 */

#define	VM_MAP_COPYIN_OPT_VM_PROT		0x7
#define	VM_MAP_COPYIN_OPT_SRC_DESTROY		0x8
#define	VM_MAP_COPYIN_OPT_STEAL_PAGES		0x10
#define	VM_MAP_COPYIN_OPT_PMAP_ENTER		0x20
#define	VM_MAP_COPYIN_OPT_NO_ZERO_FILL		0x40

/*
 *	Continuation structures for vm_map_copyin_page_list.
 */
typedef	struct {
	vm_map_t	map;
	vm_offset_t	src_addr;
	vm_size_t	src_len;
	vm_offset_t	destroy_addr;
	vm_size_t	destroy_len;
	int		options;
}  vm_map_copyin_args_data_t, *vm_map_copyin_args_t;

#define	VM_MAP_COPYIN_ARGS_NULL	((vm_map_copyin_args_t) 0)


/* vm_map_copy_cont_t is a type definition/prototype
 * for the cont function pointer in vm_map_copy structure.
 */
struct vm_map_copy;
typedef struct vm_map_copy *vm_map_copy_t;
typedef kern_return_t (*vm_map_copy_cont_t)(
				vm_map_copyin_args_t,
				vm_map_copy_t *);

#define	VM_MAP_COPY_CONT_NULL	((vm_map_copy_cont_t) 0)

struct vm_map_copy {
	int			type;
#define VM_MAP_COPY_ENTRY_LIST		1
#define VM_MAP_COPY_OBJECT		2
#define VM_MAP_COPY_PAGE_LIST		3
#define VM_MAP_COPY_KERNEL_BUFFER	4
	vm_offset_t		offset;
	vm_size_t		size;
	union {
	    struct vm_map_header	hdr;	/* ENTRY_LIST */
	    struct {				/* OBJECT */
	    	vm_object_t		object;
		vm_size_t		index;	/* record progress as pages
						 * are moved from object to
						 * page list; must be zero
						 * when first invoking
						 * vm_map_object_to_page_list
						 */
	    } c_o;
	    struct {				/* PAGE_LIST */
		int			npages;
		boolean_t		page_loose;
		vm_map_copy_cont_t	cont;
		vm_map_copyin_args_t	cont_args;
		vm_page_t		page_list[VM_MAP_COPY_PAGE_LIST_MAX];
	    } c_p;
	    struct {				/* KERNEL_BUFFER */
		vm_offset_t		kdata;
		vm_size_t		kalloc_size;  /* size of this copy_t */
	    } c_k;
	} c_u;
};


#define cpy_hdr			c_u.hdr

#define cpy_object		c_u.c_o.object
#define	cpy_index		c_u.c_o.index

#define cpy_page_list		c_u.c_p.page_list
#define cpy_npages		c_u.c_p.npages
#define cpy_page_loose		c_u.c_p.page_loose
#define cpy_cont		c_u.c_p.cont
#define cpy_cont_args		c_u.c_p.cont_args

#define cpy_kdata		c_u.c_k.kdata
#define cpy_kalloc_size		c_u.c_k.kalloc_size

#define	VM_MAP_COPY_NULL	((vm_map_copy_t) 0)

/*
 *	Useful macros for entry list copy objects
 */

#define vm_map_copy_to_entry(copy)		\
		((struct vm_map_entry *) &(copy)->cpy_hdr.links)
#define vm_map_copy_first_entry(copy)		\
		((copy)->cpy_hdr.links.next)
#define vm_map_copy_last_entry(copy)		\
		((copy)->cpy_hdr.links.prev)

/*
 *	Continuation macros for page list copy objects
 */

#define	vm_map_copy_invoke_cont(old_copy, new_copy, result)		\
MACRO_BEGIN								\
	assert(vm_map_copy_cont_is_valid(old_copy));			\
	vm_map_copy_page_discard(old_copy);				\
	*result = (*((old_copy)->cpy_cont))((old_copy)->cpy_cont_args,	\
					    new_copy);			\
	(old_copy)->cpy_cont = VM_MAP_COPY_CONT_NULL;			\
MACRO_END

#define	vm_map_copy_invoke_extend_cont(old_copy, new_copy, result)	\
MACRO_BEGIN								\
	assert(vm_map_copy_cont_is_valid(old_copy));			\
	*result = (*((old_copy)->cpy_cont))((old_copy)->cpy_cont_args,	\
					    new_copy);			\
	(old_copy)->cpy_cont = VM_MAP_COPY_CONT_NULL;			\
MACRO_END

#define vm_map_copy_abort_cont(old_copy)				\
MACRO_BEGIN								\
	assert(vm_map_copy_cont_is_valid(old_copy));			\
	vm_map_copy_page_discard(old_copy);				\
	(*((old_copy)->cpy_cont))((old_copy)->cpy_cont_args,		\
				  (vm_map_copy_t *) 0);			\
	(old_copy)->cpy_cont = VM_MAP_COPY_CONT_NULL;			\
  	(old_copy)->cpy_cont_args = VM_MAP_COPYIN_ARGS_NULL;		\
MACRO_END

#define vm_map_copy_has_cont(copy)					\
    (((copy)->cpy_cont) != VM_MAP_COPY_CONT_NULL)

/*
 *	Macro to determine number of pages in a page-list copy chain.
 */

#define vm_map_copy_page_count(copy)					\
    (round_page(((copy)->offset - trunc_page((copy)->offset)) + (copy)->size) \
    / PAGE_SIZE)

/*
 *	Macros:		vm_map_lock, etc. [internal use only]
 *	Description:
 *		Perform locking on the data portion of a map.
 *	When multiple maps are to be locked, order by map address.
 *	(See vm_map.c::vm_remap())
 */

#define vm_map_lock_init(map)						\
MACRO_BEGIN								\
	lock_init(&(map)->lock, TRUE, ETAP_VM_MAP, ETAP_VM_MAP_I);	\
	(map)->timestamp = 0;						\
MACRO_END
#define vm_map_lock(map)			\
MACRO_BEGIN					\
	lock_write(&(map)->lock);		\
	(map)->timestamp++;			\
MACRO_END

#define vm_map_unlock(map)	lock_write_done(&(map)->lock)
#define vm_map_lock_read(map)	lock_read(&(map)->lock)
#define vm_map_unlock_read(map)	lock_read_done(&(map)->lock)
#define vm_map_lock_write_to_read(map)					\
		lock_write_to_read(&(map)->lock)
#define vm_map_lock_read_to_write(map)					\
		(lock_read_to_write(&(map)->lock) || (((map)->timestamp++), 0))

#if	NORMA_VM
extern zone_t		vm_map_copy_zone; /* zone for vm_map_copy structures */
#endif	/* NORMA_VM */

/*
 *	Exported procedures that operate on vm_map_t.
 */

/* Initialize the module */
extern void		vm_map_init(void);

/* Create an empty map */
extern vm_map_t		vm_map_create(
				pmap_t		pmap,
				vm_offset_t	min,
				vm_offset_t	max,
				boolean_t	pageable);

/* Gain a reference to an existing map */
extern void		vm_map_reference(
				vm_map_t	map);

/* Lose a reference */
extern void		vm_map_deallocate(
				vm_map_t	map);

/* Get rid of a map */
extern void		vm_map_destroy(
				vm_map_t	map);

/* Lookup map entry containing or the specified address in the given map */
extern boolean_t	vm_map_lookup_entry(
				vm_map_t	map,
				vm_offset_t	address,
				vm_map_entry_t	*entry);	/* OUT */

/* simplify map entries */
extern void		vm_map_simplify(
				vm_map_t	map,
				vm_offset_t	start);

/* Allocate a range in the specified virtual address map and
 * return the entry allocated for that range. */
extern kern_return_t vm_map_find_space(
				vm_map_t	map,
				vm_offset_t	*address,	/* OUT */
				vm_size_t	size,
				vm_offset_t	mask,
				vm_map_entry_t	*o_entry);	/* OUT */

/* Enter a mapping */
extern kern_return_t	vm_map_enter(
				vm_map_t	map,
				vm_offset_t	*address,	/* IN/OUT */
				vm_size_t	size,
				vm_offset_t	mask,
				boolean_t	anywhere,
				vm_object_t	object,
				vm_offset_t	offset,
				boolean_t	needs_copy,
				vm_prot_t	cur_protection,
				vm_prot_t	max_protection,
				vm_inherit_t	inheritance);

/* Mark the given range as handled by a subordinate map. */
extern kern_return_t	vm_map_submap(
				vm_map_t	map,
				vm_offset_t	start,
				vm_offset_t	end,
				vm_map_t	submap);

/* Create a new task map using an existing task map as a template. */
extern vm_map_t		vm_map_fork(
				vm_map_t	old_map);

/* Change protection */
extern kern_return_t	vm_map_protect(
				vm_map_t	map,
				vm_offset_t	start,
				vm_offset_t	end,
				vm_prot_t	new_prot,
				boolean_t	set_max);

/* Change inheritance */
extern kern_return_t	vm_map_inherit(
				vm_map_t	map,
				vm_offset_t	start,
				vm_offset_t	end,
				vm_inherit_t	new_inheritance);

/* wire down a region */
extern kern_return_t	vm_map_wire(
				vm_map_t	map,
				vm_offset_t	start,
				vm_offset_t	end,
				vm_prot_t	access_type,
				boolean_t	user_wire);

/* unwire a region */
extern kern_return_t	vm_map_unwire(
				vm_map_t	map,
				vm_offset_t	start,
				vm_offset_t	end,
				boolean_t	user_wire);

/* Deallocate a region */
extern kern_return_t	vm_map_remove(
				vm_map_t	map,
				vm_offset_t	start,
				vm_offset_t	end,
				boolean_t	flags);

/* Steal all the pages from a vm_map_copy page_list */
extern void		vm_map_copy_steal_pages(
				vm_map_copy_t	copy);

/* Discard a copy without using it */
extern void		vm_map_copy_discard(
				vm_map_copy_t	copy);

/* Move the information in a map copy object to a new map copy object */
extern vm_map_copy_t	vm_map_copy_copy(
				vm_map_copy_t	copy);

/* A version of vm_map_copy_discard that can be called
 * as a continuation from a vm_map_copy page list. */
extern kern_return_t	vm_map_copy_discard_cont(
				vm_map_copyin_args_t	cont_args,
				vm_map_copy_t		*copy_result);/* OUT */

/* Overwrite existing memory with a copy */
extern kern_return_t	vm_map_copy_overwrite(
				vm_map_t	dst_map,
				vm_offset_t	dst_addr,
				vm_map_copy_t	copy,
				boolean_t	interruptible);

/* Place a copy into a map */
extern kern_return_t	vm_map_copyout(
				vm_map_t	dst_map,
				vm_offset_t	*dst_addr,	/* OUT */
				vm_map_copy_t	copy);

/* Version of vm_map_copyout() for page list vm map copies. */
extern kern_return_t	vm_map_copyout_page_list(
				vm_map_t	dst_map,
				vm_offset_t	*dst_addr,	/* OUT */
				vm_map_copy_t	copy);

/* Get rid of the pages in a page_list copy. */
extern void		vm_map_copy_page_discard(
				vm_map_copy_t	copy);

/* Create a copy object from an object. */
extern kern_return_t	vm_map_copyin_object(
				vm_object_t	object,
				vm_offset_t	offset,
				vm_size_t	size,
				vm_map_copy_t	*copy_result);	/* OUT */

/* Find the VM object, offset, and protection for a given virtual address
 * in the specified map, assuming a page fault of the	type specified. */
extern kern_return_t	vm_map_lookup_locked(
				vm_map_t	*var_map,	/* IN/OUT */
				vm_offset_t	vaddr,
				vm_prot_t	fault_type,
				vm_map_version_t *out_version,	/* OUT */
				vm_object_t	*object,	/* OUT */
				vm_offset_t	*offset,	/* OUT */
				vm_prot_t	*out_prot,	/* OUT */
				boolean_t	*wired,		/* OUT */
				int		*behavior,	/* OUT */
				vm_offset_t	*lo_offset,	/* OUT */
				vm_offset_t	*hi_offset);	/* OUT */

/* Verifies that the map has not changed since the given version. */
extern boolean_t	vm_map_verify(
				vm_map_t	 map,
				vm_map_version_t *version);	/* REF */

/* Make a copy of a region */
#if	DIPC
/*
 *	For DIPC only, src_volatile=TRUE means that the user promises not
 *	to modify data in transit.  Kernel may then avoid copy-on-write
 *	handling.
 *
 *	For non-DIPC kernel configurations, the src_volatile parameter
 *	is visible (even though it has no effect) to avoid ifdef'ing
 *	the interface.  The use of src_volatile has no effect in a
 *	non-DIPC kernel.
 */
#endif	/* DIPC */
extern kern_return_t	vm_map_copyin_common(
				vm_map_t	src_map,
				vm_offset_t	src_addr,
				vm_size_t	len,
				boolean_t	src_destroy,
				boolean_t	src_volatile,
				vm_map_copy_t	*copy_result,	/* OUT */
				boolean_t	use_maxprot);

/* Make a copy of a region using a page list copy */
extern kern_return_t	vm_map_copyin_page_list(
				vm_map_t	src_map,
				vm_offset_t	src_addr,
				vm_size_t	len,
				int		options,
				vm_map_copy_t	*copy_result,	/* OUT */
				boolean_t	is_cont);

/* Add or remove machine-dependent attributes from map regions */
extern kern_return_t	vm_map_machine_attribute(
				vm_map_t	map,
				vm_offset_t	address,
				vm_size_t	size,
				vm_machine_attribute_t	attribute,
				vm_machine_attribute_val_t* value); /* IN/OUT */
/* Set paging behavior */
extern kern_return_t	vm_map_behavior_set(
				vm_map_t	map,
				vm_offset_t	start,
				vm_offset_t	end,
				vm_behavior_t	new_behavior);

#if	DIPC
extern kern_return_t	vm_map_convert_to_page_list(
				vm_map_copy_t	*caller_copy);

extern kern_return_t	vm_map_object_to_page_list_cont(
				vm_map_copy_t	cont_args,
				vm_map_copy_t	*copy_result);	/* OUT */

extern kern_return_t	vm_map_object_to_page_list(
				vm_map_copy_t	*caller_copy);

extern kern_return_t	vm_map_convert_from_page_list(
				vm_map_copy_t	copy);

extern kern_return_t	vm_map_object_to_entry_list(
				vm_map_copy_t	copy);

extern void		vm_map_entry_reset(
				struct vm_map_header *header_map);

#endif	/* DIPC */

/* Split a vm_map_entry into 2 entries */
extern void		_vm_map_clip_start(
				struct vm_map_header	*map_header,
				vm_map_entry_t		entry,
				vm_offset_t		start);

#if	DIPC
extern vm_map_copy_t 	vm_map_entry_list_from_object(
			        vm_object_t		object,
				vm_offset_t		offset,
				vm_size_t		size,
				boolean_t		pageable);

extern kern_return_t	vm_map_convert_to_entry_list(
				vm_map_copy_t	copy,
				boolean_t	pageable);

extern vm_map_copy_t	vm_map_copy_overwrite_recv(
				vm_map_t	map,
				vm_offset_t	addr,
				vm_size_t	size);

extern kern_return_t	vm_map_copy_overwrite_recv_done(
				vm_map_t	map,
				vm_map_copy_t	copy);
#endif	/* DIPC */

extern vm_map_entry_t	vm_map_entry_insert(
				vm_map_t	map,
				vm_map_entry_t	insp_entry,
				vm_offset_t	start,
				vm_offset_t	end,
				vm_object_t	object,
				vm_offset_t	offset,
				boolean_t	needs_copy,
				boolean_t	is_shared,
				boolean_t	in_transition,
				vm_prot_t	cur_protection,
				vm_prot_t	max_protection,
				vm_behavior_t	behavior,
				vm_inherit_t	inheritance,
				unsigned	wired_count);

extern kern_return_t	vm_remap_extract(
				vm_map_t	map,
				vm_offset_t	addr,
				vm_size_t	size,
				boolean_t	copy,
				struct vm_map_header *map_header,
				vm_prot_t	*cur_protection,
				vm_prot_t	*max_protection,
				vm_inherit_t	inheritance,
				boolean_t	pageable);

extern kern_return_t	vm_remap_range_allocate(
				vm_map_t	map,
				vm_offset_t	*address,
				vm_size_t	size,
				vm_offset_t	mask,
				boolean_t	anywhere,
				vm_map_entry_t	*map_entry);

extern kern_return_t	vm_remap_extract(
				vm_map_t	map,
				vm_offset_t	addr,
				vm_size_t	size,
				boolean_t	copy,
				struct vm_map_header *map_header,
				vm_prot_t	*cur_protection,
				vm_prot_t	*max_protection,
				vm_inherit_t	inheritance,
				boolean_t	pageable);

extern kern_return_t	vm_remap_range_allocate(
				vm_map_t	map,
				vm_offset_t	*address,
				vm_size_t	size,
				vm_offset_t	mask,
				boolean_t	anywhere,
				vm_map_entry_t	*map_entry);

extern vm_map_t		vm_map_switch(
				vm_map_t	map);

extern int		vm_map_copy_cont_is_valid(
				vm_map_copy_t	copy);


/*
 *	Macros to invoke vm_map_copyin_common.  vm_map_copyin is the
 *	usual form; it handles a copyin based on the current protection
 *	(current protection == VM_PROT_NONE) is a failure.
 *	vm_map_copyin_maxprot handles a copyin based on maximum possible
 *	access.  The difference is that a region with no current access
 *	BUT possible maximum access is rejected by vm_map_copyin(), but
 *	returned by vm_map_copyin_maxprot.
 */
#define	vm_map_copyin(src_map, src_addr, len, src_destroy, copy_result) \
		vm_map_copyin_common(src_map, src_addr, len, src_destroy, \
					FALSE, copy_result, FALSE)

#define vm_map_copyin_maxprot(src_map, \
			      src_addr, len, src_destroy, copy_result) \
		vm_map_copyin_common(src_map, src_addr, len, src_destroy, \
					FALSE, copy_result, TRUE)

#if	DIPC
/*
 *	Caller promises not to modify data in transit.  Kernel may then
 *	avoid copy-on-write handling.
 *
 *	N.B.  When src_destroy=TRUE, turning on src_volatile=TRUE has
 *	no effect.
 */
#define	vm_map_copyin_volatile(src_map,src_addr,len,src_destroy,copy_result) \
		vm_map_copyin_common(src_map, src_addr, len, src_destroy, \
					TRUE, copy_result, FALSE)
#endif	/* DIPC */

/*
 *	Functions implemented as macros
 */
#define		vm_map_min(map)		((map)->min_offset)
						/* Lowest valid address in
						 * a map */

#define		vm_map_max(map)		((map)->max_offset)
						/* Highest valid address */

#define		vm_map_pmap(map)	((map)->pmap)
						/* Physical map associated
						 * with this address map */

#define		vm_map_verify_done(map, version)    vm_map_unlock_read(map)
						/* Operation that required
						 * a verified lookup is
						 * now complete */

/*
 * Macros/functions for map residence counts and swapin/out of vm maps
 */
#if	TASK_SWAPPER

#if	MACH_ASSERT
/* Gain a reference to an existing map */
extern void		vm_map_reference(
				vm_map_t	map);
/* Lose a residence count */
extern void		vm_map_res_deallocate(
				vm_map_t	map);
/* Gain a residence count on a map */
extern void		vm_map_res_reference(
				vm_map_t	map);
/* Gain reference & residence counts to possibly swapped-out map */
extern void		vm_map_reference_swap(
				vm_map_t	map);

#else	/* MACH_ASSERT */

#define vm_map_reference(map)			\
MACRO_BEGIN					\
	vm_map_t Map = (map);			\
	if (Map) {				\
		mutex_lock(&Map->s_lock);	\
		Map->res_count++;		\
		Map->ref_count++;		\
		mutex_unlock(&Map->s_lock);	\
	}					\
MACRO_END

#define vm_map_res_reference(map)		\
MACRO_BEGIN					\
	vm_map_t Lmap = (map);			\
	if (Lmap->res_count == 0) {		\
		mutex_unlock(&Lmap->s_lock);	\
		vm_map_lock(Lmap);		\
		vm_map_swapin(Lmap);		\
		mutex_lock(&Lmap->s_lock);	\
		++Lmap->res_count;		\
		vm_map_unlock(Lmap);		\
	} else					\
		++Lmap->res_count;		\
MACRO_END

#define vm_map_res_deallocate(map)		\
MACRO_BEGIN					\
	vm_map_t Map = (map);			\
	if (--Map->res_count == 0) {		\
		mutex_unlock(&Map->s_lock);	\
		vm_map_lock(Map);		\
		vm_map_swapout(Map);		\
		vm_map_unlock(Map);		\
		mutex_lock(&Map->s_lock);	\
	}					\
MACRO_END

#define vm_map_reference_swap(map)	\
MACRO_BEGIN				\
	vm_map_t Map = (map);		\
	mutex_lock(&Map->s_lock);	\
	++Map->ref_count;		\
	vm_map_res_reference(Map);	\
	mutex_unlock(&Map->s_lock);	\
MACRO_END
#endif 	/* MACH_ASSERT */

extern void		vm_map_swapin(
				vm_map_t	map);

extern void		vm_map_swapout(
				vm_map_t	map);

#else	/* TASK_SWAPPER */

#define vm_map_reference(map)			\
MACRO_BEGIN					\
	vm_map_t Map = (map);			\
	if (Map) {				\
		mutex_lock(&Map->s_lock);	\
		Map->ref_count++;		\
		mutex_unlock(&Map->s_lock);	\
	}					\
MACRO_END

#define vm_map_reference_swap(map)	vm_map_reference(map)
#define vm_map_res_reference(map)
#define vm_map_res_deallocate(map)

#endif	/* TASK_SWAPPER */

/*
 *	Submap object.  Must be used to create memory to be put
 *	in a submap by vm_map_submap.
 */
extern vm_object_t	vm_submap_object;

/*
 *	Wait and wakeup macros for in_transition map entries.
 */
#define vm_map_entry_wait(map, interruptible)    	\
        MACRO_BEGIN                                     \
        assert_wait((event_t)&(map)->hdr, interruptible);	\
        vm_map_unlock(map);                             \
	thread_block((void (*)(void))0);		\
        MACRO_END

#define vm_map_entry_wakeup(map)        thread_wakeup((event_t)(&(map)->hdr))


/*
 * Flags for vm_map_remove() and vm_map_delete()
 */
#define	VM_MAP_NO_FLAGS	  		0x0
#define	VM_MAP_REMOVE_KUNWIRE	  	0x1
#define	VM_MAP_REMOVE_INTERRUPTIBLE  	0x2
#define	VM_MAP_REMOVE_WAIT_FOR_KWIRE  	0x4

#define	vm_map_ref_fast(map)				\
	MACRO_BEGIN					\
	mutex_lock(&map->s_lock);			\
	map->ref_count++;				\
	vm_map_res_reference(map);			\
	mutex_unlock(&map->s_lock);			\
	MACRO_END

#define	vm_map_dealloc_fast(map)			\
	MACRO_BEGIN					\
	register int c;					\
							\
	mutex_lock(&map->s_lock);			\
	c = --map->ref_count;				\
	if (c > 0)					\
		vm_map_res_deallocate(map);		\
	mutex_unlock(&map->s_lock);			\
	if (c == 0)					\
		vm_map_destroy(map);			\
	MACRO_END

#endif	/* _VM_VM_MAP_H_ */

