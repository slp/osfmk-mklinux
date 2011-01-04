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


#ifndef	_MACH_DEFAULT_PAGER_TYPES_H_
#define _MACH_DEFAULT_PAGER_TYPES_H_

/*
 *	Remember to update the mig type definitions
 *	in default_pager_types.defs when adding/removing fields.
 */

typedef struct default_pager_info {
	vm_size_t dpi_total_space;	/* size of backing store */
	vm_size_t dpi_free_space;	/* how much of it is unused */
	vm_size_t dpi_page_size;	/* the pager's vm page size */
} default_pager_info_t;

typedef integer_t *backing_store_info_t;
typedef int	backing_store_flavor_t;

#define BACKING_STORE_BASIC_INFO	1
#define BACKING_STORE_BASIC_INFO_COUNT \
		(sizeof(struct backing_store_basic_info)/sizeof(integer_t))
struct backing_store_basic_info {
	natural_t	pageout_calls;		/* # pageout calls */
	natural_t	pagein_calls;		/* # pagein calls */
	natural_t	pages_in;		/* # pages paged in (total) */
	natural_t	pages_out;		/* # pages paged out (total) */
	natural_t	pages_unavail;		/* # zero-fill pages */
	natural_t	pages_init;		/* # page init requests */
	natural_t	pages_init_writes;	/* # page init writes */

	natural_t	bs_pages_total;		/* # pages (total) */
	natural_t	bs_pages_free;		/* # unallocated pages */
	natural_t	bs_pages_in;		/* # page read requests */
	natural_t	bs_pages_in_fail;	/* # page read errors */
	natural_t	bs_pages_out;		/* # page write requests */
	natural_t	bs_pages_out_fail;	/* # page write errors */

	integer_t	bs_priority;
	integer_t	bs_clsize;
};
typedef struct backing_store_basic_info	*backing_store_basic_info_t;


typedef struct default_pager_object {
	vm_offset_t dpo_object;		/* object managed by the pager */
	vm_size_t dpo_size;		/* backing store used for the object */
} default_pager_object_t;

typedef default_pager_object_t *default_pager_object_array_t;


typedef struct default_pager_page {
	vm_offset_t dpp_offset;		/* offset of the page in its object */
} default_pager_page_t;

typedef default_pager_page_t *default_pager_page_array_t;

#define DEFAULT_PAGER_BACKING_STORE_MAXPRI	4

#endif	/* _MACH_DEFAULT_PAGER_TYPES_H_ */
