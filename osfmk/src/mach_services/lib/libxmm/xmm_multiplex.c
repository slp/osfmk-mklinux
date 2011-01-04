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
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * MkLinux
 */
/*
 *	File:	xmm_multiplex.c
 *	Author:	Joseph S. Barrera III
 *	Date:	1991
 *
 *	Xmm class for allowing many objects to share a paging file.
 */

#include <mach.h>
#include "xmm_obj.h"

#undef	NBBY
#define	NBBY		8
#define	BYTEMASK	0xff

#undef	howmany
#define	howmany(a,b)	(((a) + (b) - 1)/(b))

#define	NO_BLOCK	((vm_offset_t)-1)

#define	MOBJ_STATE_UNCALLED	0
#define	MOBJ_STATE_CALLED	1
#define	MOBJ_STATE_READY	2

typedef struct request *request_t;

char *malloc();
char *calloc();

/*
 * Object that interposes on backing store object.
 * Represents what xmm interface (and old default pager)
 * call a 'partition', as in a disk partition that is used
 * for multiple memory objects.
 */
struct kobj {
	struct xmm_obj	obj;
	int		state;
	vm_size_t	total_size;	/* total number of blocks */
	vm_size_t	free;		/* number of blocks free */
	unsigned char	*bitmap;	/* allocation map */
	xmm_obj_t	mobj_list;
	request_t	request_list;
};

/*
 * Object created by m_multiplex_create. Many of these are mapped
 * onto one 'partition'.
 */
struct mobj {
	struct xmm_obj	obj;
	struct xmm_obj	*kobj;
	mach_port_t	pager_name;	/* Known name port */
	int		errors;		/* Pageout error count */
	vm_offset_t	*map;		/* block map */
	vm_size_t	size;		/* size of paging object, in pages */
	xmm_obj_t	mobj_next;
};

/*
 * Request struct, used to remember information known at data_request
 * time, such as for which mobj the request was made.
 */
struct request {
	xmm_obj_t	mobj;
	vm_offset_t	m_offset;
	vm_offset_t	k_offset;
	request_t	next;
};

kern_return_t m_multiplex_init();
kern_return_t m_multiplex_terminate();
kern_return_t m_multiplex_data_request();
kern_return_t m_multiplex_data_write();

xmm_decl(multiplex_class,
	/* m_init		*/	m_multiplex_init,
	/* m_terminate		*/	m_multiplex_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_multiplex_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_multiplex_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_invalid_data_provided,
	/* k_data_unavailable	*/	k_invalid_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_invalid_data_error,
	/* k_set_attributes	*/	k_invalid_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"multiplex",
	/* size			*/	sizeof(struct mobj)
);

kern_return_t k_partition_data_provided();
kern_return_t k_partition_data_unavailable();
kern_return_t k_partition_data_error();
kern_return_t k_partition_set_attributes();

xmm_decl(partition_class,
	/* m_init		*/	m_invalid_init,
	/* m_terminate		*/	m_invalid_terminate,
	/* m_copy		*/	m_invalid_copy,
	/* m_data_request	*/	m_invalid_data_request,
	/* m_data_unlock	*/	m_invalid_data_unlock,
	/* m_data_write		*/	m_invalid_data_write,
	/* m_lock_completed	*/	m_invalid_lock_completed,
	/* m_supply_completed	*/	m_invalid_supply_completed,
	/* m_data_return	*/	m_invalid_data_return,

	/* k_data_provided	*/	k_partition_data_provided,
	/* k_data_unavailable	*/	k_partition_data_unavailable,
	/* k_get_attributes	*/	k_invalid_get_attributes,
	/* k_lock_request	*/	k_invalid_lock_request,
	/* k_data_error		*/	k_partition_data_error,
	/* k_set_attributes	*/	k_partition_set_attributes,
	/* k_destroy		*/	k_invalid_destroy,
	/* k_data_supply	*/	k_invalid_data_supply,

	/* name			*/	"multiplex",
	/* size			*/	sizeof(struct kobj)
);

void		pager_dealloc_page();
void		pager_extend();
vm_offset_t	pager_read_offset();
vm_offset_t	pager_write_offset();
void		pager_dealloc();

static request_t request_allocate();
static void	request_deallocate();

kern_return_t
xmm_partition_allocate(mobj, size, partition_result)
	xmm_obj_t mobj;
	void **partition_result;
{
	xmm_obj_t kobj;
	kern_return_t kr;

	kr = xmm_obj_allocate(&partition_class, mobj, &kobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	size = atop(size);
	KOBJ->state = MOBJ_STATE_UNCALLED;
	KOBJ->total_size = size;
	KOBJ->free = size;
	KOBJ->bitmap = (unsigned char *) calloc(howmany(size, NBBY), 1);
	KOBJ->mobj_list = XMM_OBJ_NULL;
	KOBJ->request_list = (request_t) 0;
	*partition_result = (void *) kobj;
	return KERN_SUCCESS;
}

kern_return_t
xmm_multiplex_create(partition, new_mobj)
	void *partition;
	xmm_obj_t *new_mobj;
{
	xmm_obj_t mobj;
	kern_return_t kr;

	if (((xmm_obj_t) partition)->class != &partition_class) {
		printf("xmm_multiplex_create: invalid partition\n");
		return KERN_FAILURE;
	}
	kr = xmm_obj_allocate(&multiplex_class, XMM_OBJ_NULL, &mobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	MOBJ->kobj = (xmm_obj_t) partition;
	MOBJ->errors = 0;
	MOBJ->map = (vm_offset_t *) 0;
	MOBJ->size = 0;
	MOBJ->pager_name = MACH_PORT_NULL;
	*new_mobj = mobj;
	return KERN_SUCCESS;
}

kern_return_t
m_multiplex_init(mobj, k_kobj, pager_name, page_size)
	xmm_obj_t	mobj;
	xmm_obj_t	k_kobj;
	mach_port_t	pager_name;
	vm_size_t	page_size;
{
	xmm_obj_t kobj = MOBJ->kobj;

	if (mobj->k_kobj) {
		printf("m_multiplex_init: multiple users\n");
		_K_DESTROY(k_kobj, KERN_FAILURE);
		return KERN_FAILURE;
	}
	if (page_size != vm_page_size) {
		printf("m_multiplex_init: wrong page size\n");
		_K_DESTROY(k_kobj, KERN_FAILURE);
		return KERN_FAILURE;
	}
	k_kobj->m_kobj = mobj;
	mobj->k_kobj = k_kobj;
	MOBJ->pager_name = pager_name;

	if (KOBJ->state == MOBJ_STATE_READY) {
		K_SET_ATTRIBUTES(mobj, TRUE, FALSE, MEMORY_OBJECT_COPY_DELAY);
	} else {
		MOBJ->mobj_next = KOBJ->mobj_list;
		KOBJ->mobj_list = mobj;
		if (KOBJ->state == MOBJ_STATE_UNCALLED) {
			KOBJ->state = MOBJ_STATE_CALLED;
			M_INIT(kobj, kobj, MACH_PORT_NULL, vm_page_size);
		}
	}
	return KERN_SUCCESS;
}

kern_return_t
m_multiplex_terminate(mobj, kobj, pager_name)
	xmm_obj_t	mobj;
	xmm_obj_t	kobj;
	mach_port_t	pager_name;
{
	/*
	 * As with all my other terminates, a lot more should happen here.
	 */
#if     lint
	kobj++;
	pager_name++;
#endif  /* lint */
	pager_dealloc(mobj);
	return KERN_SUCCESS;
}

kern_return_t
m_multiplex_data_request(mobj, kobj, offset, length, desired_access)
	xmm_obj_t	mobj;
	xmm_obj_t	kobj;
	vm_offset_t	offset;
	vm_size_t	length;
	vm_prot_t	desired_access;
{
	vm_offset_t	block;
	request_t	r;

#if     lint
	length++;
	desired_access++;
#endif  /* lint */
	if (MOBJ->errors) {/* XXX if errors, should free up obj */
		K_DATA_ERROR(kobj, offset, vm_page_size, KERN_FAILURE);
		return KERN_FAILURE;
	}

	/*
	 * Find block in paging partition
	 */
	block = pager_read_offset(mobj, offset);
	if (block == NO_BLOCK) {
		K_DATA_UNAVAILABLE(kobj, offset, vm_page_size);
		return KERN_SUCCESS;
	}

	/*
	 * Allocate and queue request structure
	 */
	r = request_allocate();
	r->mobj = mobj;
	r->m_offset = offset;
	r->k_offset = ptoa(block);
	r->next = ((struct kobj *)MOBJ->kobj)->request_list;
	((struct kobj *)MOBJ->kobj)->request_list = r;

	/*
	 * Read data from that block
	 */
	M_DATA_REQUEST(MOBJ->kobj, MOBJ->kobj, r->k_offset, vm_page_size,
		       VM_PROT_ALL);
	return KERN_SUCCESS;
}

/*
 * m_multiplex_data_write: split up the stuff coming in from
 * a memory_object_data_write call
 * into individual pages and pass them off to multiplex_write.
 *
 * XXX
 * also by convention: don't deallocate data here;
 * bottom-level memory manager will take care of it.
 */
kern_return_t
m_multiplex_data_write(mobj, kobj, offset, data, length)
	xmm_obj_t	mobj;
	xmm_obj_t	kobj;
	vm_offset_t	offset;
	vm_offset_t	data;
	vm_size_t	length;
{
	kern_return_t	kr;
	vm_offset_t	block;

#if     lint
	kobj++;
#endif  /* lint */
	/*
	 * Find block in paging partition
	 */
	block = pager_write_offset(mobj, offset);
	if (block == NO_BLOCK) {
		printf("*** WRITE ERROR on multiplex_pageout: no space\n");
		MOBJ->errors++;
		return KERN_NO_SPACE;
	}

	/*
	 * Write data to that block
	 */
	kr = M_DATA_WRITE(MOBJ->kobj, MOBJ->kobj, offset, data, length);
	if (kr != KERN_SUCCESS) {
		printf("*** WRITE ERROR on multiplex_pageout\n");
		MOBJ->errors++;
		return kr;
	}
	return KERN_SUCCESS;
}

k_partition_reply(kobj, offset, data, length, lock_value, error_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	vm_prot_t lock_value;
{
	request_t r, *rp;
	xmm_obj_t mobj;

	for (rp = &KOBJ->request_list; r = *rp; rp = &r->next) {
		if (r->m_offset == offset) {
			*rp = r->next;
			break;
		}
	}
	if (! r) {
		printf("k_partition_reply: not found!\n");
		return KERN_FAILURE;
	}
	mobj = r->mobj;
	if (error_value) {
		MOBJ->errors++;
		mach_error("*** partition: data_error", error_value);
		K_DATA_ERROR(mobj, offset, length, error_value);
	} else if (data) {
		K_DATA_PROVIDED(mobj, offset, data, length, lock_value);
	} else {
		printf("*** partition: data_unavailable???\n");
		K_DATA_UNAVAILABLE(mobj, offset, length);
	}
	request_deallocate(r);
	return KERN_SUCCESS;
}

k_partition_data_provided(kobj, offset, data, length, lock_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_offset_t data;
	vm_size_t length;
	vm_prot_t lock_value;
{
	return k_partition_reply(kobj, offset, data, length, lock_value,
				 KERN_SUCCESS);
}

k_partition_data_unavailable(kobj, offset, length)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
{
	return k_partition_reply(kobj, offset, 0, length, VM_PROT_NONE,
				 KERN_SUCCESS);
}

k_partition_data_error(kobj, offset, length, error_value)
	xmm_obj_t kobj;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t error_value;
{
	return k_partition_reply(kobj, offset, 0, length, VM_PROT_NONE,
				 error_value);
}

k_partition_set_attributes(kobj, object_ready, may_cache, copy_strategy)
	xmm_obj_t kobj;
	boolean_t object_ready;
	boolean_t may_cache;
	memory_object_copy_strategy_t copy_strategy;
{
	xmm_obj_t mobj;

	if (KOBJ->state != MOBJ_STATE_CALLED) {
		return KERN_FAILURE;
	}
	if (! object_ready) {
		return KERN_SUCCESS;
	}
	KOBJ->state = MOBJ_STATE_READY;
	for (mobj = KOBJ->mobj_list; mobj; mobj = MOBJ->mobj_next) {
		K_SET_ATTRIBUTES(mobj, TRUE, may_cache, copy_strategy);
	}
	return KERN_SUCCESS;
}

/*
 * Allocate a page in a paging file
 */
vm_offset_t
pager_alloc_page(kobj)
	xmm_obj_t kobj;
{
	int	byte;
	int	bit;
	int	limit;
	
	if (KOBJ->free == 0) {
		return NO_BLOCK;	/* out of paging space */
	}
	limit = howmany(KOBJ->total_size, NBBY);
	for (byte = 0; byte < limit; byte++) {
		if (KOBJ->bitmap[byte] != BYTEMASK) {
			break;
		}
	}
	if (byte == limit) {
		printf("pager_alloc_page: free != 0 but no free blocks\n");
	}
	for (bit = 0; bit < NBBY; bit++) {
		if ((KOBJ->bitmap[byte] & (1<<bit)) == 0) {
			break;
		}
	}
	if (bit == NBBY) {
		printf("pager_alloc_page: no bits!\n");
	}
	KOBJ->bitmap[byte] |= (1<<bit);
	if (KOBJ->free-- == 0) {
		printf("Multiplex pager is full\n");
	}
	return byte*NBBY+bit;
}

/*
 * Deallocate a page in a paging file
 */
void
pager_dealloc_page(kobj, page)
	xmm_obj_t kobj;
	vm_offset_t page;
{
	int bit, byte;
	
	if (page >= KOBJ->total_size) {
		printf("dealloc_page\n");
	}
	byte = page / NBBY;
	bit  = page % NBBY;
	KOBJ->bitmap[byte] &= ~(1<<bit);
	KOBJ->free++;
}

/*
 * A paging object uses either a one- or a two-level map of offsets
 * into the associated paging partition.
 */
#define	PAGEMAP_ENTRIES		128
				/* number of pages in a second-level map */
#define	PAGEMAP_SIZE(npgs)	((npgs)*sizeof(vm_offset_t))

#define	INDIRECT_PAGEMAP_ENTRIES(npgs) \
		((((npgs)-1)/PAGEMAP_ENTRIES) + 1)
#define	INDIRECT_PAGEMAP_SIZE(npgs) \
		(INDIRECT_PAGEMAP_ENTRIES(npgs) * sizeof(vm_offset_t *))
#define	INDIRECT_PAGEMAP(size)	\
		(size > PAGEMAP_ENTRIES)

/*
 * Extend the map for a paging object.
 */
void
pager_extend(mobj, new_size)
	xmm_obj_t mobj;
	vm_size_t	new_size;	/* in pages */
{
	vm_offset_t *	new_map;
	vm_offset_t *	old_map;
	int		i;
	vm_size_t	old_size;
	
	/*
	 * Double current size until we cover new size.
	 */
	old_size = MOBJ->size;
	for (i = (old_size ? old_size : 1); i < new_size; i <<= 1) {
		continue;
	}
	new_size = i;
	
	if (INDIRECT_PAGEMAP(old_size)) {
		/*
		 * Pager already uses two levels.  Allocate
		 * a larger indirect block.
		 */
		new_map = (vm_offset_t *)
		    malloc(INDIRECT_PAGEMAP_SIZE(new_size));
		old_map = MOBJ->map;
		for (i = 0; i < INDIRECT_PAGEMAP_ENTRIES(old_size); i++) {
			new_map[i] = old_map[i];
		}
		for (; i < INDIRECT_PAGEMAP_ENTRIES(new_size); i++) {
			new_map[i] = 0;
		}
		if (old_map) {
			free((char *)old_map);
		}
		MOBJ->map = new_map;
		MOBJ->size = new_size;
		return;
	}
	
	if (INDIRECT_PAGEMAP(new_size)) {
		/*
		 * Changing from direct map to indirect map.
		 * Allocate both indirect and direct map blocks,
		 * since second-level (direct) block must be
		 * full size (PAGEMAP_SIZE(PAGEMAP_ENTRIES)).
		 */
		
		/*
		 * Allocate new second-level map first.
		 */
		new_map = (vm_offset_t *)malloc(PAGEMAP_SIZE(PAGEMAP_ENTRIES));
		old_map = MOBJ->map;
		for (i = 0; i < old_size; i++) {
			new_map[i] = old_map[i];
		}
		for (; i < PAGEMAP_ENTRIES; i++) {
			new_map[i] = NO_BLOCK;
		}
		if (old_map) {
			free((char *)old_map);
		}
		old_map = new_map;
		
		/*
		 * Now allocate indirect map.
		 */
		new_map = (vm_offset_t *)
		    malloc(INDIRECT_PAGEMAP_SIZE(new_size));
		new_map[0] = (vm_offset_t) old_map;
		for (i = 1; i < INDIRECT_PAGEMAP_ENTRIES(new_size); i++) {
			new_map[i] = 0;
		}
		MOBJ->map = new_map;
		MOBJ->size = new_size;
		return;
	}
	/*
	 * Enlarging a direct block.
	 */
	new_map = (vm_offset_t *) malloc(PAGEMAP_SIZE(new_size));
	old_map = MOBJ->map;
	for (i = 0; i < old_size; i++){
		new_map[i] = old_map[i];
	}
	for (; i < new_size; i++) {
		new_map[i] = NO_BLOCK;
	}
	if (old_map) {
		free((char *)old_map);
	}
	MOBJ->map = new_map;
	MOBJ->size = new_size;
}

/*
 * Given an offset within a paging object, find the
 * corresponding block within the paging partition.
 * Return NO_BLOCK if none allocated.
 */
vm_offset_t
pager_read_offset(mobj, offset)
	xmm_obj_t	mobj;
	vm_offset_t	offset;
{
	vm_offset_t	f_page;
	
	f_page = atop(offset);
	if (f_page >= MOBJ->size) {
#if 0
		/*
		 * This warning must have been in the original default pager,
		 * but I don't understand why. Perhaps someone further up
		 * is smart enough to do the data_unavailble for us.
		 */
		printf("*** PAGER WARNING: offset 0x%x out range [0..0x%x)\n",
		       f_page, MOBJ->size);
#endif
		return NO_BLOCK;
	}
	if (INDIRECT_PAGEMAP(MOBJ->size)) {
		vm_offset_t *map;
		
		map = (vm_offset_t *)MOBJ->map[f_page/PAGEMAP_ENTRIES];
		if (map == 0) {
			return NO_BLOCK;
		}
		
		return map[f_page%PAGEMAP_ENTRIES];
	} else {
		return MOBJ->map[f_page];
	}
}

/*
 * Given an offset within a paging object, find the
 * corresponding block within the paging partition.
 * Allocate a new block if necessary.
 *
 * WARNING: paging objects apparently may be extended
 * without notice!
 */
vm_offset_t
pager_write_offset(mobj, offset)
	xmm_obj_t	mobj;
	vm_offset_t	offset;
{
	vm_offset_t	block, f_page;
	vm_offset_t	*map;
	
	f_page = atop(offset);
	if (f_page >= MOBJ->size) {
		/*
		 * Paging object must be extended.
		 * Remember that offset is 0-based, but size is 1-based.
		 */
		pager_extend(mobj, f_page + 1);
	}
	if (INDIRECT_PAGEMAP(MOBJ->size)) {
		map = (vm_offset_t *)MOBJ->map[f_page/PAGEMAP_ENTRIES];
		if (map == 0) {
			/*
			 * Allocate the indirect block
			 */
			int i;

			map = (vm_offset_t *)
			    malloc(PAGEMAP_SIZE(PAGEMAP_ENTRIES));
			if (map == 0) {
				/* out of space! */
				return NO_BLOCK;
			}
			MOBJ->map[f_page/PAGEMAP_ENTRIES] = (vm_offset_t)map;
			for (i = 0; i < PAGEMAP_ENTRIES; i++) {
				map[i] = NO_BLOCK;
			}
		}
		f_page %= PAGEMAP_ENTRIES;
	} else {
		map = MOBJ->map;
	}
	block = map[f_page];
	if (block == NO_BLOCK) {
		block = pager_alloc_page(MOBJ->kobj);
		if (block == NO_BLOCK) {
			return NO_BLOCK;	/* out of space */
		}
		map[f_page] = block;
	}
	return block;
}

/*
 * Deallocate all of the blocks belonging to a paging object.
 */
void
pager_dealloc(mobj)
	xmm_obj_t mobj;
{
	int i, j;
	vm_offset_t block;
	vm_offset_t *map;
	
	if (INDIRECT_PAGEMAP(MOBJ->size)) {
		for (i = INDIRECT_PAGEMAP_ENTRIES(MOBJ->size); --i >= 0; ) {
			map = (vm_offset_t *) MOBJ->map[i];
			if (map) {
				for (j = 0; j < PAGEMAP_ENTRIES; j++) {
					block = map[j];
					if (block != NO_BLOCK) {
						pager_dealloc_page(MOBJ->kobj,
								   block);
					}
				}
				free((char *)map);
			}
		}
		free((char *)MOBJ->map);
	} else {
		map = MOBJ->map;
		for (i = 0; i < MOBJ->size; i++) {
			block = map[i];
			if (block != NO_BLOCK) {
				pager_dealloc_page(MOBJ->kobj, block);
			}
		}
		free((char *)MOBJ->map);
	}
}

/*
 * Allocate or deallocate a request structure.
 */

request_t free_request = 0;

static request_t
request_allocate()
{
	if (free_request) {
		register request_t r = free_request;
		free_request = 0;
		return r;
	} else {
		return (request_t) malloc(sizeof(struct request));
	}
}

static void
request_deallocate(r)
	request_t r;
{
	if (free_request) {
		free((char *) r);
	} else {
		free_request = r;
	}
}
