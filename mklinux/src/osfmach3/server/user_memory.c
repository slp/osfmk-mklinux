/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * Implements a cache of memory regions in the server that are shared with
 * the memory of user tasks.  Used for efficient copyin/copyout.
 *
 * The server uses the vm_remap service to map at will any range of 
 * virtual addresses in any user task into itself  The user is not an
 * active participant in the sharing -- it does not know about it and cannot
 * control it.  The size of the cache is limited by flushing out (and
 * deallocating) the least recently used memory regions.
 */

#include <linux/autoconf.h>

#include <mach/vm_prot.h>
#include <mach/mach_host.h>
#include <mach/mach_interface.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/user_memory.h>
#include <osfmach3/macro_help.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>

#include <asm/page.h>

#include <linux/sched.h>
#include <linux/malloc.h>

#define use_hints 1	/* still buggy ??? */

#if	CONFIG_OSFMACH3_DEBUG
#define USER_MEMORY_DEBUG 1
#endif	/* CONFIG_OSFMACH3_DEBUG */

extern boolean_t	holding_uniproc(void);

extern int map_user_mem_to_copy;

#ifdef	USER_MEMORY_DEBUG

int	user_memory_debug = 0;

#define debug_prf(lev, args) 						\
MACRO_BEGIN								\
	if (user_memory_debug >= lev)					\
		printk args;						\
MACRO_END
#define debug(lev, stuff)						\
MACRO_BEGIN								\
	if (user_memory_debug >= lev) {					\
		stuff;							\
	}								\
MACRO_END

#else	/* USER_MEMORY_DEBUG */

#define debug_prf(lev, args)
#define debug(lev, stuff)

#endif	/* USER_MEMORY_DEBUG */

struct user_memory mru_list_dummy;

#define USER_HASH_SIZE	 997	/* A prime number is best here. */
				/* Don't let the hash table get too full */
#define USER_MEMORY_SIZE (USER_HASH_SIZE * 4)

				/* Hash to find a desired shared memory */
#define USER_MEMORY_HASH(task, page_num)		\
		((((page_num) << 15) ^ (task->pid)) % USER_HASH_SIZE)

				/* The hash table for mapping a user task and
				 * address to a share memory:
				 */
struct user_memory_bucket {
	user_memory_t	chain;	/* List of regions that hash to same index */
} user_memory_hash[USER_HASH_SIZE];

				/* The number of initial entries in the mru
				 * list that won't be reordered on each access
				 * (an optimization).
				 */
#define NUM_FRESH_BLOCKS   (USER_MEMORY_SIZE / 3)

user_memory_t mru_list;		/* List of all shared regions from most to least
				 * recently used (protectd by user_memory_lock).
				 */
user_memory_t oldest_fresh;	/* The last of the initial part of the mru list
				 * that will not be reordered on each use.
				 */
int user_memory_num_entries;	/* Current number of shared memory regions */

#define newest_region (mru_list->next_mru)
#define oldest_region (mru_list->prev_mru)

/* Address, size and page number macros */
#define ALIGN_ADDR_DOWN(addr) ((addr) & (PAGE_MASK))
#define ALIGN_ADDR_UP(addr)   (((addr) + (~PAGE_MASK)) & (PAGE_MASK))
#define PAGENUM_TO_ADDR(num)  ((num) << PAGE_SHIFT)
#define ADDR_TO_PAGENUM(addr) ((addr) >> PAGE_SHIFT)

/* Forward declarations */
void user_memory_flush_lru(void);

#ifdef	USER_MEMORY_DEBUG

int user_memory_num_lookups = 0;
int user_memory_num_maps = 0;
int user_memory_num_extends = 0;
int user_memory_ave_size = 0;
int user_memory_hint1_hits = 0;
int user_memory_hint2_hits = 0;

void
user_memory_statistics(void)
{
	int successes = user_memory_num_lookups -
			(user_memory_num_maps + user_memory_num_extends);
	printk("Share Cache Statistics ------------------\n");
	printk("Num entries: %d\n", user_memory_num_entries);
	printk("Num lookups/successes (hit rate): %d/%d (%d %%)\n",
			user_memory_num_lookups, successes,
			successes * 100 / user_memory_num_lookups);
	printk("Ave requested size: %d\n", user_memory_ave_size);
	printk("Num map extends: %d\n", user_memory_num_extends);
	printk("\n");
}

#endif	/* USER_MEMORY_DEBUG */


/*
 * Maps into the server the address range specified by user_addr and size
 * in the task specified by task.  The range need not be page aligned.
 * Returns a newly allocated shared memory region that is locked.  Does not
 * enter the region into share cache or initialize any of its list links.
 */
user_memory_t
map_region(
	struct task_struct	*task,
	vm_address_t		user_addr,
	vm_size_t		size,
	vm_prot_t		prot)
{
	kern_return_t	ret;
	vm_address_t	svr_addr;
	vm_address_t	aligned_user_addr;
	user_memory_t	region_p;
	vm_prot_t	cur_prot;
	vm_prot_t	max_prot;

	svr_addr = 0;
	aligned_user_addr = ALIGN_ADDR_DOWN(user_addr);

	debug(0, ++user_memory_num_maps);

	size = ALIGN_ADDR_UP(user_addr + size) - aligned_user_addr;
	user_addr = aligned_user_addr;

	server_thread_blocking(FALSE);
	ret = vm_remap(mach_task_self(),
		       &svr_addr,
		       size,
		       0,		/* alignment */
		       TRUE,		/* anywhere */
		       task->osfmach3.task->mach_task_port, /* from_task */
		       user_addr,
		       FALSE,		/* copy */
		       &cur_prot,
		       &max_prot,
		       VM_INHERIT_NONE);
	server_thread_unblocking(FALSE);

	if (ret != KERN_SUCCESS) {
		debug_prf(1, ("%s to map addr %x, len %x of task %p, because\n",
			  "map_region: failed", user_addr, size, task));
		debug_prf(1,
			  ("           vm_remap failed. (Returned %x)\n", ret));
		return NULL;
	}
	debug_prf(2, ("map_region: mapped addr %x, len %x of task %p.\n",
				user_addr, size, task));

	region_p = (user_memory_t) kmalloc(sizeof (*region_p), GFP_KERNEL);
	if (!region_p)
		panic("map_region: kmalloc failed for user_memory elt\n");

	region_p->task = task;
	region_p->user_page = ADDR_TO_PAGENUM(user_addr);
	region_p->svr_addr = svr_addr;
	region_p->size = size;
	region_p->prot = cur_prot;
	region_p->ref_count = 0;

	return region_p;
}


/*
 * Makes a mapped region bigger, by extending it at the end.  Does not change
 * the user_memory address or the first mapped user address, hence there is
 * no need to update the share cache hash or any other meta-data.  Assumes
 * the region is locked.
 */
boolean_t
map_region_bigger(
	user_memory_t	region_p,
	vm_size_t	new_size,
	vm_prot_t	prot)
{
	vm_address_t	user_addr;
	user_memory_t	new_region_p;
	kern_return_t	kr;

	debug(0, ++user_memory_num_extends);

	user_addr = PAGENUM_TO_ADDR(region_p->user_page);

	/*
	 * Cancel the increment that will be done by map_region
	 */
	debug(0, --user_memory_num_maps);

	if ((new_region_p = map_region(region_p->task, user_addr,
				       new_size, prot)) != NULL) {
		/*
		 * Deallocate the old shared memory
		 */
		kr = vm_deallocate(mach_task_self(), region_p->svr_addr,
				   region_p->size);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("map_region_bigger: vm_deallocate"));
		}
		/*
		 * Copy the relevant fields of the new descriptor on top of
		 * the old one
		 */
		region_p->svr_addr = new_region_p->svr_addr;
		region_p->size = new_region_p->size;

		/*
		 * Free the new region descriptor.  We unlock it for the
		 * benefit of debugging code that keeps track of the locks
		 * held by a thread.
		 */
		ASSERT(new_region_p->ref_count == 0);
		new_region_p->ref_count--;
		kfree(new_region_p);
	} else {
		return FALSE;
	}

	debug_prf(2,("%s: extended map at %x to len %d for task %p.\n",
		     "map_region_bigger",
		     user_addr, new_size, region_p->task));
	return TRUE;
}


/*
 * Unmap from the server the specified memory region and also deallocate the
 * region descriptor.  Assumes the region is not locked by current thread, but
 * has been removed from all accessible lists
 */
void
unmap_region(
	user_memory_t	region_p)
{
	kern_return_t	kr;

	/*
	 * Although there is no longer any way for anyone to find it,
	 * we need to wait for anyone still using it to finish
	 */
	ASSERT(region_p->ref_count >= 0);
	while (region_p->ref_count != 0) {
		osfmach3_yield();
		ASSERT(region_p->ref_count >= 0);
	}

	/*
	 * Deallocate the shared memory
	 */
	kr = vm_deallocate(mach_task_self(),
			   region_p->svr_addr, region_p->size);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("unmap_region: vm_deallocate"));
	}

	/*
	 * Deallocate the struct
	 */
	ASSERT(region_p->ref_count == 0);
	region_p->ref_count = -1;
	kfree(region_p);
}


/*
 * Links the given region at the front of the mru list (assumes the list
 * and the region are locked), and makes the appropriate adjustment to the
 * oldest_fresh pointer and the freshness fields:
 */
void
insert_mru(
	user_memory_t	region_p)
{
	/*
	 * Link it into the mru list
	 */
	region_p->next_mru = newest_region;
	region_p->prev_mru = mru_list;
	newest_region = region_p;
	region_p->next_mru->prev_mru = region_p;

	/*
	 * Mark it fresh
	 */
	region_p->is_fresh = TRUE;

	if (++user_memory_num_entries > NUM_FRESH_BLOCKS) {
		/*
		 * Mark the oldest fresh region stale
		 */
		oldest_fresh->is_fresh = FALSE;
		oldest_fresh = oldest_fresh->prev_mru;
		if (oldest_fresh == mru_list)
			panic("bug in insert_mru.\n");
	} else if (!oldest_fresh)	/* Only happens on first insert */
		oldest_fresh = region_p;
}


/*
 * Unlinks the given region from the mru list (assumes the list
 * and the region are locked), and makes the appropriate adjustment to the
 * oldest_fresh pointer and the freshness fields:
 */
void
delete_mru(
	user_memory_t	region_p)
{
	if (region_p->is_fresh && user_memory_num_entries > NUM_FRESH_BLOCKS) {
		/*
		 * Mark the newest stale region fresh
		 */
		oldest_fresh = oldest_fresh->next_mru;
		if (oldest_fresh == mru_list)
			panic("bug in delete_mru.\n");
		oldest_fresh->is_fresh = TRUE;
	}

	/*
	 * Unlink it from mru_list
	 */
	region_p->prev_mru->next_mru = region_p->next_mru;
	region_p->next_mru->prev_mru = region_p->prev_mru;

	--user_memory_num_entries;
}


/*
 * Inserts the shared memory region into the share cache, whether or not
 * an identical or overlapping range is already entered for the same task.
 * If this would take the cache size over the limit, the oldest region in
 * the cache is first flushed (removed from the cache and deallocated).
 * Assumes the region is locked by the caller.
 */
void
user_memory_insert(
	user_memory_t	region_p)
{
	struct task_struct		*task;
	struct user_memory_bucket	*bucket;

	ASSERT(holding_uniproc());

	debug_prf(2,
		  ("user_memory_insert: linking addr %x, len %x of task %p.\n",
		   PAGENUM_TO_ADDR(region_p->user_page),
		   region_p->size, region_p->task));

	task = region_p->task;
	bucket = &user_memory_hash[USER_MEMORY_HASH(task, region_p->user_page)];

	while (user_memory_num_entries >= USER_MEMORY_SIZE)
		user_memory_flush_lru();

	/*
	 * Link it into the hash table
	 */
	region_p->next_inchain = bucket->chain;
	bucket->chain = region_p;

	/*
	 * Link it into the task list
	 */
	region_p->next_intask = region_p->task->osfmach3.task->user_memory;
	region_p->prev_intask = NULL;
	if (region_p->next_intask)
		region_p->next_intask->prev_intask = region_p;
	region_p->task->osfmach3.task->user_memory = region_p;

	/*
	 * Put it at beginning of the mru list
	 */
	insert_mru(region_p);
}


void 
user_memory_lookup(
	struct task_struct	*task,
	vm_address_t		addr,
	vm_size_t		size,
	vm_prot_t		prot,
	vm_address_t		*svr_addrp,
	user_memory_t		*ump)
{
	user_memory_t	region_p;

#if use_hints
	region_p = task->osfmach3.task->um_hint1;
	if (region_p != NULL &&
	    addr >= PAGENUM_TO_ADDR(region_p->user_page) &&
	    addr + size < (PAGENUM_TO_ADDR(region_p->user_page)
			   + region_p->size)) {
		debug(0, user_memory_hint1_hits++);
		goto found_region;
	}

	region_p = task->osfmach3.task->um_hint2;
	if (region_p != NULL &&
	    addr >= PAGENUM_TO_ADDR(region_p->user_page) &&
	    addr + size < (PAGENUM_TO_ADDR(region_p->user_page)
			   + region_p->size)) {
		debug(0, user_memory_hint2_hits++);
		goto found_region;
	}
#endif

	region_p = user_memory_slow_lookup(task, addr, size,
					   prot, svr_addrp);
	if (region_p) {
		/* add it as a hint */
		if (task->osfmach3.task->um_hint1 == NULL) {
			task->osfmach3.task->um_hint1 = region_p;
		} else if (task->osfmach3.task->um_hint2 == NULL) {
			task->osfmach3.task->um_hint2 = region_p;
		} else {
			task->osfmach3.task->um_hint1 =
				task->osfmach3.task->um_hint2;
			task->osfmach3.task->um_hint2 = region_p;
		}
	}

#if use_hints
    found_region:
#endif
	if (region_p) {
		if ((prot & region_p->prot) != prot) {
			/* required access is denied on the region */
			region_p = NULL;
			*svr_addrp = 0;
		} else {
			*svr_addrp = (region_p->svr_addr + addr
				      - PAGENUM_TO_ADDR(region_p->user_page));
			ASSERT(region_p->ref_count >= 0);
			region_p->ref_count++;
		}
	}

	*ump = region_p;
}


/*
 * Returns the starting address in the server that is mapped to the
 * range of user addresses specified by task, user_addr and size.  The
 * address range will be mapped into the server and entered into the share
 * cache by this call if it is not already.  If part of it is mapped, but
 * not all of it, the mapped range will be extended into contiguous server
 * addresses.  Neither the input range nor the returned starting address
 * are necessarily page aligned.
 *
 * If successful, the region of memory is locked and an identifier of the
 * region (for use as an arg to user_memory_unlock_region) is returned through
 * the region_id_p ptr); otherwise NULL is returned.
 */
user_memory_t
user_memory_slow_lookup(
	struct task_struct	*task,
	vm_address_t		user_addr,
	vm_size_t		size,
	vm_prot_t		prot,
	vm_address_t		*svr_addrp)
{
	vm_address_t			pagenum;
	int				pageoffset;
	user_memory_t			region_p;
	struct user_memory_bucket	*bucket;

	debug(0, user_memory_ave_size =
		(user_memory_ave_size * user_memory_num_lookups + size) /
		(user_memory_num_lookups + 1));
	debug(0, ++user_memory_num_lookups);
	debug(1, if (user_memory_num_lookups % 100 == 0) user_memory_statistics() );

	pagenum = ADDR_TO_PAGENUM(user_addr);
	pageoffset = user_addr - PAGENUM_TO_ADDR(pagenum);

	bucket = &user_memory_hash[USER_MEMORY_HASH(task, pagenum)];
	for (region_p = bucket->chain; region_p;
	     region_p = region_p->next_inchain) {
		if (region_p->task == task && region_p->user_page == pagenum)
			break;
	}
	if (!region_p) {
		/*
		 * It's not in the cache
		 */
		if ((region_p = map_region(task, user_addr, size, prot))
		    != NULL) {
			user_memory_insert(region_p);
			*svr_addrp = region_p->svr_addr + pageoffset;
		} else
			*svr_addrp = 0;
	} else {
		/*
		 * NOTE: Block must be locked before releasing list lock
		 */
		if (region_p->size < pageoffset + size) {
			/*
			 * Extend the region to include user_addr + size
			 */
			if (!map_region_bigger(region_p, pageoffset + size,
					       prot)) {
				*svr_addrp = 0;
				return NULL;
			}
		}
		if (!region_p->is_fresh) {
			/*
			 * Put it at beginning of the mru list
			 */
			delete_mru(region_p);
			insert_mru(region_p);
		}
		*svr_addrp = region_p->svr_addr + pageoffset;
	}
	return region_p;
}


/*
 * Unlinks the specified region from all the lists (except the task list,
 * if task_list is FALSE), and decrements the number of entries.
 * Does not deallocate the shared memory or the region descriptor.
 *
 * Assumes that the user_cash_lock is locked and the hash bucket is not.
 */
void
user_memory_delete(
	user_memory_t	region_p,
	boolean_t	do_task_list,
	char		*caller_name)
{
	register user_memory_t		chain_p;
	struct user_memory_bucket	*bucket;
	struct task_struct		*t;

	ASSERT(holding_uniproc());

	debug_prf(2,
		  ("user_memory_delete: unlinking addr %x, len %x of task %p.\n",
		   PAGENUM_TO_ADDR(region_p->user_page),
		   region_p->size, region_p->task));

	bucket = &user_memory_hash[USER_MEMORY_HASH(region_p->task,
						   region_p->user_page)];

	delete_mru(region_p);

	t = region_p->task;
	if (t->osfmach3.task->um_hint2 == region_p) {
		t->osfmach3.task->um_hint2 = NULL;
	}
	if (t->osfmach3.task->um_hint1 == region_p) {
		t->osfmach3.task->um_hint1 = t->osfmach3.task->um_hint2;
		t->osfmach3.task->um_hint2 = NULL;
	}

	/*
	 * Unlink it from the task list, if desired
	 */
	if (do_task_list) {
		if (region_p->prev_intask)
			region_p->prev_intask->next_intask =
				region_p->next_intask;
		else {
			/*
			 * It's first in the list, so
			 */
			region_p->task->osfmach3.task->user_memory =
				region_p->next_intask;
		}
		if (region_p->next_intask)
			region_p->next_intask->prev_intask =
				region_p->prev_intask;
	}

	/*
	 * Unlink it from hash bucket chain.  This is a singly linked
	 * list, so we have to search it from the beginning
	 */
	chain_p = bucket->chain;
	if (!chain_p) {
		printk("%s: memory not in hash table\n", caller_name);
		panic("user_memory_delete: fatal bug\n");
	} else if (chain_p == region_p) {
		bucket->chain = region_p->next_inchain;
	} else {
		for (;
		     chain_p->next_inchain && chain_p->next_inchain != region_p;
		     chain_p = chain_p->next_inchain);
		if (!chain_p->next_inchain) {
			printk("%s: memory not in hash table\n", caller_name);
			panic("user_memory_delete: fatal bug\n");
		}
		/*
		 * Unlink from bucket chain
		 */
		chain_p->next_inchain = region_p->next_inchain;
	}
}


void
user_memory_unlock(
	user_memory_t		region_p)
{
	if (!region_p)
		return;
	ASSERT(region_p->ref_count > 0);
	region_p->ref_count--;
	if (region_p->ref_count == 0 &&
	    region_p->task->osfmach3.task->mach_aware) {
		/*
		 * This task can change its memory at any time using Mach
		 * calls, so don't keep anything in the user memory cache.
		 */
		user_memory_delete(region_p, TRUE, "user_memory_unlock");
		unmap_region(region_p);
	}
}


/*
 * Flush from the share cache the least-recently-used region of memory
 * and unmap it from the server.  Assumes that only the mru list is
 * locked.
 */
void
user_memory_flush_lru()
{
	register user_memory_t	region_p;

	region_p = oldest_region;

	ASSERT(region_p->ref_count >= 0);
	if (region_p->ref_count > 0) {
		/* can't flush it now: try later */
		return;
	}

	/*
	 * Unlink it from the cache lists
	 */
	user_memory_delete(region_p, TRUE, "user_memory_flush_lru");

	debug_prf(2, ("user_memory_flush_lru()\n"));

	/*
	 * Deallocate the memory and the region descriptor
	 */
	unmap_region(region_p);
}


/*
 * Called when a task is created.  Initializes the list of shared memory
 * regions for that task.
 */
void
user_memory_task_init(
	struct task_struct	*task)
{
	task->osfmach3.task->user_memory = NULL;
	task->osfmach3.task->um_hint1 = NULL;
	task->osfmach3.task->um_hint2 = NULL;
}


/*
 * Called when a VM area is deallocated in the task's address space.
 * Flushes the shared memory regions in that area.
 */
void
user_memory_flush_area(
	struct osfmach3_mach_task_struct	*mach_task,
	vm_address_t				start,
	vm_address_t				end)
{
	user_memory_t	region_p, next_p;
	vm_address_t	start_region, end_region;

	for (region_p = mach_task->user_memory;
	     region_p;
	     region_p = next_p) {
		next_p = region_p->next_intask;
		start_region = PAGENUM_TO_ADDR(region_p->user_page);
		end_region = start_region + region_p->size;
		if (end_region > start && start_region < end) {
			user_memory_delete(region_p, TRUE,
					   "user_memory_flush_area");
			unmap_region(region_p);
		}
	}
}


/*
 * Called when a task exits.  Flushes all the shared memory regions for
 * that task.
 */
void
user_memory_flush_task(
	struct osfmach3_mach_task_struct	*mach_task)
{
	register user_memory_t	region_p, next_p;
	int			starting_num_entries;

	region_p = mach_task->user_memory;
	starting_num_entries = user_memory_num_entries;

	/*
	 * The first time through, just unlink the regions from all lists
	 * but don't deallocate them, to minimize list unavailibility
	 */
	for ( ; region_p; region_p = next_p) {
		next_p = region_p->next_intask;
		user_memory_delete(region_p, FALSE, "user_memory_flush_task");
	}

	region_p = mach_task->user_memory; /* Save it for next pass */
	mach_task->user_memory = NULL;
	ASSERT(mach_task->um_hint1 == NULL);
	ASSERT(mach_task->um_hint2 == NULL);

	/*
	 * This time through we will deallocate all the regions, which are
	 * no longer in any accessible list
	 */
	for ( ; region_p; region_p = next_p) {
		next_p = region_p->next_intask;
		unmap_region(region_p);
	}

	debug_prf(2, ("user_memory_flush_task: "
		      "deallocated %d regions of task %p\n",
		      starting_num_entries - user_memory_num_entries,
		      mach_task));
}

/*
 * Compute memory statistics for a task.
 */
void statm_page_range(struct task_struct *task, 
		      vm_address_t address, 
		      vm_address_t end,
		      int *pages, 
		      int *shared, 
		      int *dirty, 
		      int *total);

void statm_page_range(struct task_struct *mach_task, 
		      vm_address_t vm_start, 
		      vm_address_t vm_end,
		      int *pages, 
		      int *shared, 
		      int *dirty, 
		      int *total)
{
  vm_address_t			start_addr;
  mach_port_t			mach_task_port;
  kern_return_t			kr;
  vm_machine_attribute_val_t	val;
  *pages = 0;
  *shared = 0;
  *dirty = 0;
  *total = 0;

  mach_task_port = mach_task->osfmach3.task->mach_task_port;
  start_addr = vm_start;
  /* Hack: 'kswapd' and 'kflushd' have all of memory mapped! Skip them... */
  if ((unsigned)vm_end >= 0x80000000) {
    *pages = *shared = *total = 0;
    return;
  }
  val = MATTR_VAL_GET_INFO;
  kr = vm_machine_attribute(mach_task_port,
			    start_addr, 
			    vm_end - vm_start,
			    MATTR_CACHE, &val);
  if (kr == KERN_SUCCESS) {
    *pages = val >> 16;
    *shared = val & 0xFFFF;
  } else {
    *pages = 0;
    *shared = 0;
  }
  *total = (vm_end - vm_start) / PAGE_SIZE;
}

/*
 * Display user memory structure for a task.
 */
void
show_mem_range(task_struct_t mach_task,
	       vm_address_t	vm_start,
	       vm_address_t	vm_end)
{
	vm_address_t			start_addr;
	vm_size_t			size, remain;
	vm_region_basic_info_data_t	info;
	mach_msg_type_number_t		count;
	mach_port_t			mach_task_port, objname;
	kern_return_t			kr;
	vm_machine_attribute_val_t	val;

	struct task_struct ** p;

	p = task;
	while (++p < task+NR_TASKS) {
	  if (*p) printk("Task: %x, pid: %d\n", (unsigned)*p, (*p)->pid);
	}

	mach_task_port = mach_task->osfmach3.task->mach_task_port;
	start_addr = vm_start;
	while (start_addr != vm_end) {
	  val = MATTR_VAL_GET;
	  kr = vm_machine_attribute(mach_task_port,
				    start_addr, (vm_size_t)0x1,  /* Just this page! */
				    MATTR_CACHE, &val);
	  if (kr < 0) {
	    printk("Can't get attributes on page 0x%x\n", (unsigned)start_addr);
	  }
	  printk("VA: 0x%08x, PA: 0x%08x, Count: %d\n", start_addr, val, kr);
	  start_addr += PAGE_SIZE;
	}
	mach_task_port = mach_task->osfmach3.task->mach_task_port;
	start_addr = vm_start;
	count = VM_REGION_BASIC_INFO_COUNT;
	kr = vm_region(mach_task_port, &start_addr, &size, VM_REGION_BASIC_INFO,
		       (vm_region_info_t) &info, &count, &objname);
	if (kr != KERN_SUCCESS) {
	  printk("Can't get region info on page 0x%x\n", (unsigned)start_addr);
	  /*	  return; */
	}
	val = MATTR_VAL_GET;
	kr = vm_machine_attribute(mach_task_port,
				  start_addr, (vm_size_t)0x1,  /* Just this page! */
				  MATTR_CACHE, &val);
	if (kr != KERN_SUCCESS) {
	  printk("Can't get attributes on page 0x%x\n", (unsigned)start_addr);
	}
	printk("VMA: 0x%08x..0x%08x 0x%08x, Size: 0x%08x, Prot: 0x%08x, Pg: 0x%08x\n",
	       (unsigned)vm_start,
	       (unsigned)vm_end,
	       (unsigned)start_addr,
	       (unsigned)size,
	       (unsigned)info.protection,
	       (unsigned)val);
	remain = 0;
}

void
user_memory_init(void)
{
	extern boolean_t in_kernel;
	register int i;

	if (in_kernel) {
		map_user_mem_to_copy = 0;
	}

	debug_prf(2, ("user_memory_init: PAGE_SHIFT = %d\n", PAGE_SHIFT));
	debug_prf(2, ("                  PAGE_MASK = %lx\n", PAGE_MASK));

	debug_prf(2, ("user_memory_init: initializing mru list ...\n"));
	/*
	 * Initialize the mru list.  We keep a dummy element in the list, for
	 * simplicity of insertion/deletion
	 */
	oldest_fresh = NULL;
	mru_list = &mru_list_dummy;
	mru_list->next_mru = mru_list->prev_mru = mru_list;
	user_memory_num_entries = 0;

	debug_prf(2, ("user_memory_init: initializing hash table ...\n"));
	/*
	 * Initialize the hash table
	 */
	for (i = 0; i < USER_HASH_SIZE; i++) {
		user_memory_hash[i].chain = NULL;
	}
}
