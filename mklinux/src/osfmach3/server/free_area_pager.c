/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <cthreads.h>
#include <mach/mach_interface.h>
#include <mach/mach_host.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>
#include <osfmach3/mach_init.h>
#include <osfmach3/free_area_pager.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/serv_port.h>
#include <osfmach3/parent_server.h>

#include <linux/malloc.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/swap.h>
#ifdef	CONFIG_OSFMACH3_DEBUG
#include <linux/in.h>
#include <linux/netdevice.h>
#include <asm/checksum.h>
#endif	/* CONFIG_OSFMACH3_DEBUG */

#define USE_SILENT_OVERWRITE 1

#if	CONFIG_OSFMACH3_DEBUG
#define FREE_AREA_PAGER_DEBUG 1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	FREE_AREA_PAGER_DEBUG
int free_area_pager_debug = 0;
int free_area_pager_info_debug = 0;
int free_area_shrinker_debug = 0;
int free_area_pageout_debug = 0;
#endif	/* FREE_AREA_PAGER_DEBUG */

extern mach_port_t privileged_host_port;

extern boolean_t free_area_object_server(mach_msg_header_t *InHeadP,
					 mach_msg_header_t *OutHeadP);

#ifdef DUMP_FREE_AREA_STATS
void free_area_dump_stats(void);
#endif

memory_object_t		free_area_mem_obj = MACH_PORT_NULL;
memory_object_control_t	free_area_mem_obj_control = MACH_PORT_NULL;
struct task_struct	free_area_shrinker_task;
unsigned long		*free_area_status;
int			free_area_npages;
int			min_discard_page_nr;
unsigned long		free_area_flushable;
int			initial_min_free_pages;
unsigned long		free_area_discards_in_progress;
struct mutex		free_area_shrinker_mutex;
struct condition	free_area_shrinker_condition;
spin_lock_t		free_area_status_lock = SPIN_LOCK_INITIALIZER;

extern void 		free_area_grow_again(unsigned long ignored);

struct timer_list	free_area_timer = { NULL, NULL, 0, 0, 
					    free_area_grow_again };
boolean_t		free_area_timer_ticking;

/*
 * Statistics
 */
unsigned long		free_area_data_unavailables = 0;
unsigned long		free_area_data_unavailables_in_advance = 0;
unsigned long		free_area_data_supplies = 0;
unsigned long		free_area_data_returns = 0;
unsigned long		free_area_data_returns_for_nothing = 0;
unsigned long		free_area_discard_requests = 0;
unsigned long		free_area_discard_unused_success = 0;
unsigned long		free_area_discard_unused_failure = 0;
unsigned long		free_area_discard_shrink_success = 0;
unsigned long		free_area_discard_shrink_failure = 0;
int			free_area_grow_size;

unsigned long		reserved_swap_pages = 0;

struct vm_statistics osfmach3_vm_stats = { 0, };

void
osfmach3_update_vm_info(void)
{
	mach_msg_type_number_t count;
	kern_return_t kr;

	count = HOST_VM_INFO_COUNT;
	kr = host_statistics(privileged_host_port,
			     HOST_VM_INFO,
			     (host_info_t) &osfmach3_vm_stats,
			     &count);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(2, kr,
			    ("osfmach3_update_vm_info: "
			     "hosts_statistics(HOST_VM_INFO)"));
	}
}

int
vm_enough_memory(
	long pages)
{
	/*
	 * stupid algorithm to decide if we have enough memory: while
	 * simple, it hopefully works in most obvious cases.. Easy to
	 * fool it, but this should catch most mistakes.
	 */
	long freepages;
	static struct sysinfo si;
	static unsigned long counter = 0;
	static boolean_t first_time = TRUE;

	if (min_free_pages > initial_min_free_pages ||
	    counter >= 20 ||
	    pages >= reserved_swap_pages ||
	    first_time) {
		/*
		 * Update the swap information.
		 * The frequency (see test above) is purely arbitrary...
		 */
		si_swapinfo(&si);
		osfmach3_update_vm_info();
		counter = 0;
		first_time = FALSE;
	}
	counter += pages;

	freepages = buffermem >> PAGE_SHIFT;
	freepages >>= 1;
	freepages += osfmach3_vm_stats.free_count;
	if (si.totalswap != 0) { 
		freepages += (si.freeswap >> PAGE_SHIFT) - reserved_swap_pages ;
	}
	freepages -= osfmach3_mem_size >> (PAGE_SHIFT + 4);

	if ((si.freeswap >> PAGE_SHIFT) < reserved_swap_pages &&
	    si.totalswap > 0) {
		/*
		 * Try to protect the default pager from bad situations:
		 * stop allocating user memory when it gets low on paging
		 * space.
		 */
		freepages = 0;
	}

	if (freepages <= pages) {
#ifdef	FREE_AREA_PAGER_DEBUG
		if (free_area_pager_debug) {
			printk("vm_enough_memory(%ld pages): buffermem %d free_count %d freeswap %ld reserved %d - freepages %ld ==> %sOK\n",
			       pages,
			       buffermem >> PAGE_SHIFT,
			       osfmach3_vm_stats.free_count,
			       si.freeswap >> PAGE_SHIFT,
			       osfmach3_mem_size >> (PAGE_SHIFT + 4),
			       freepages,
			       freepages > pages ? "" : "**NOT**");
		}
#endif	/* FREE_AREA_PAGER_DEBUG */
		printk("vm_enough_memory: refusing %ld page%s to [P%d %s]\n",
		       pages, (pages == 1) ? "" : "s",
		       current->pid, current->comm);
	}

#ifdef	PPC
#if 0
	/*
	 * HACK HACK HACK 
	 * There's a bug in libc for the PowerPC: sbrk doesn't return
	 * ENOMEM if the system call returned less memory than asked !
	 * Work-around this bug the hard way !
	 */
	if (freepages <= pages)
		oom(current);	/* XXX */
#endif
#endif	/* PPC */

	return freepages > pages;
}

static char __status[4];
char *
free_area_page_status(
	int page)
{
	if (free_area_status[page] & FAP_PRESENT)
		__status[0] = 'P';
	else
		__status[0] = '.';
	if (free_area_status[page] & FAP_DIRTY)
		__status[1] = 'D';
	else
		__status[1] = '.';
	if (free_area_status[page] & FAP_IN_USE)
		__status[2] = 'U';
	else
		__status[2] = '.';
	if (free_area_status[page] & PAGE_MASK)
		__status[3] = 'O';
	else
		__status[3] = '.';
	return __status;	/* XXX warning: don't re-use in caller */
}

/*
 * Tunable parameters:
 */

/* duration of the free_area_timer (periodicity of growth after a shrink) */
unsigned long		free_area_timer_ticks = 1 * HZ;	/* 1 second */
/* # of pages to discard at each mo_discard_request */
int			free_area_discard_size = 10;

void
free_area_shrinker(void)
{
	unsigned long	addr;
	unsigned long	group_start_addr;
	boolean_t	flushed_unused_pages;
	int		loops;

	/*
	 * Try to discard unused pages first.
	 */
	flushed_unused_pages = FALSE;
	if (free_area_flushable > 0) {
		/* flush all the unused pages in free_area */
		group_start_addr = 0;

		spin_lock(&free_area_status_lock);
		for (addr = PAGE_OFFSET;
		     addr < high_memory;
		     addr += PAGE_SIZE) {
			if ((free_area_status[MAP_NR(addr)]
			     & (FAP_PRESENT|FAP_IN_USE))
			    == FAP_PRESENT) {
				ASSERT(free_area_flushable != 0);
				if (group_start_addr == 0) {
					/* start a new group */
					group_start_addr = addr;
				}
				continue;
			} else if (group_start_addr == 0) {
				continue;
			}
			spin_unlock(&free_area_status_lock);

			/*
			 * Flush the previous group of
			 * contiguous unused pages.
			 */
			free_area_discard_pages(group_start_addr,
						addr - group_start_addr);
			group_start_addr = 0;
			flushed_unused_pages = TRUE;
			spin_lock(&free_area_status_lock);
		}
		spin_unlock(&free_area_status_lock);
		if (group_start_addr != 0) {
			free_area_discard_pages(group_start_addr,
						addr - group_start_addr);
			flushed_unused_pages = TRUE;
		}
		if (flushed_unused_pages) {
			free_area_discard_unused_success++;
		} else {
			free_area_discard_unused_failure++;
		}
	}

	/*
	 * Shrink the buffer cache now...
	 */
	for (loops = free_area_discard_size; loops > 0; loops--) {
		if (try_to_free_page(GFP_KERNEL, 0, 1)) {
			free_area_discard_shrink_success++;
		} else {
			free_area_discard_shrink_failure++;
			/*
			 * Don't absurdly loop on try_to_free_page 
			 * if there's nothing to be freed.
			 */
			osfmach3_yield();
			break;
		}	
	}

	/*
	 * Prevent further allocations for some time by
	 * adjusting the "min_free_pages" variable.
	 */
	if (nr_free_pages > 0 &&
	    min_free_pages < nr_free_pages - 1) {
		min_free_pages = nr_free_pages - 1;
		if (! free_area_timer_ticking) {
			free_area_grow_size = 1;
			free_area_timer_ticking = TRUE;
			free_area_timer.expires = free_area_timer_ticks + jiffies;
			add_timer(&free_area_timer);
		}
#ifdef	FREE_AREA_PAGER_DEBUG
		if (free_area_shrinker_debug) {
			printk("free_area_shrinker: "
			       "min_free_pages = %d, nr_free_pages = %d\n",
			       min_free_pages, nr_free_pages);
		}
#endif	/* FREE_AREA_PAGER_DEBUG */
	}
}
  
void *
free_area_shrinker_thread(
	void	*arg)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t kr;

	cthread_set_name(cthread_self(), "free_area shrinker thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);

#if 0
	kr = server_thread_priorities(BASEPRI_SERVER-1, BASEPRI_SERVER-1);
#else
	kr = server_thread_priorities(host_pri_info.system_priority,
				      host_pri_info.system_priority);
#endif
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("free_area_shrinker_thread: "
			     "server_thread_priorities"));
	}

	/*
	 * The free_area shrinker runs in its own Linux task
	 * so that it can block...
	 */
	priv_data.current_task = &free_area_shrinker_task;
#if 0
	free_area_shrinker_task.osfmach3.thread->active_on_cthread =
		cthread_self();
#endif

	uniproc_enter();

	for (;;) {
		free_area_shrinker_task.state = TASK_INTERRUPTIBLE;

		server_thread_blocking(FALSE);

		mutex_lock(&free_area_shrinker_mutex);
		free_area_discards_in_progress--;
		if (! free_area_discards_in_progress) {
			condition_wait(&free_area_shrinker_condition,
				       &free_area_shrinker_mutex);
		}
		mutex_unlock(&free_area_shrinker_mutex);

		server_thread_unblocking(FALSE);

		free_area_shrinker_task.state = TASK_RUNNING;

		free_area_shrinker();
	}
}

void *
free_area_pager_thread(
	void	*arg)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t		kr;
	mach_msg_header_t	*in_msg;
	mach_msg_header_t	*out_msg;

	cthread_set_name(cthread_self(), "free_area pager thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);

#if 0
	kr = server_thread_priorities(BASEPRI_SERVER-1, BASEPRI_SERVER-1);
#else
	kr = server_thread_priorities(host_pri_info.system_priority,
				      host_pri_info.system_priority);
#endif
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("free_area_pager_thread: "
			     "server_thread_priorities"));
	}

	kr = vm_allocate(mach_task_self(),
			 (vm_offset_t *) &in_msg,
			 FREE_AREA_PAGER_MESSAGE_SIZE,
			 TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("free_area_pager_thread: vm_allocate"));
		panic("free_area_pager_thread: can't allocate in_msg");
	}
	kr = vm_allocate(mach_task_self(),
			 (vm_offset_t *) &out_msg,
			 FREE_AREA_PAGER_MESSAGE_SIZE,
			 TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("free_area_pager_thread: vm_allocate"));
		panic("free_area_pager_thread: can't allocate out_msg");
	}

	for (;;) {
		kr = mach_msg(in_msg, MACH_RCV_MSG,
			      0, FREE_AREA_PAGER_MESSAGE_SIZE,
			      free_area_mem_obj, MACH_MSG_TIMEOUT_NONE,
			      MACH_PORT_NULL);
		if (kr != MACH_MSG_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("free_area_pager_thread: mach_msg(RCV)"));
			continue;
		}

		if (!free_area_object_server(in_msg, out_msg)) {
			printk("free_area_pager_thread: invalid msg id 0x%x\n",
			       in_msg->msgh_id);
		}

		if (MACH_PORT_VALID(out_msg->msgh_remote_port)) {
			kr = mach_msg(out_msg, MACH_SEND_MSG,
				      out_msg->msgh_size, 0, MACH_PORT_NULL,
				      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
			if (kr != MACH_MSG_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("free_area_pager_thread: "
					     "mach_msg(SEND)"));
			}
		}
	}
}

const char *osfmach3_zero_page;

unsigned long
free_area_pager_init(
	vm_size_t	size,
	unsigned long	mask)
{
	kern_return_t		kr;
	int			nr;
	vm_address_t		free_area_addr;
	mach_msg_type_number_t	port_info_count;
	struct mach_port_limits	mpl;

	/*
	 * Allocate a page filled with zeroes to increase the performance
	 * in the occasions where we want to fill out a user memory zone
	 * with zeroes. Wire this zero page to avoid stupid paging.
	 */
	kr = vm_map(mach_task_self(),
		    (vm_address_t *) &osfmach3_zero_page,
		    PAGE_SIZE,
		    0,			/* mask */
		    TRUE,		/* anywhere */
		    MEMORY_OBJECT_NULL,
		    0,			/* offset */
		    FALSE,		/* copy */
		    VM_PROT_READ,	/* cur_protection */
		    VM_PROT_READ,	/* max_protection */
		    VM_INHERIT_NONE);	/* inheritance */
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("free_area_pager_init: vm_map(%ld)",
			     PAGE_SIZE));
		panic("free_area_init: can't allocate the ZERO page\n");
	}
	kr = vm_wire(privileged_host_port,
		     mach_task_self(),
		     (vm_address_t) osfmach3_zero_page,
		     PAGE_SIZE,
		     VM_PROT_READ);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("free_area_pager_init: vm_wire(%p, %ld)",
			     osfmach3_zero_page, PAGE_SIZE));
		printk("free_area_pager_init: can't wire the ZERO page\n");
	}

	initial_min_free_pages = min_free_pages;

	free_area_mem_obj = MACH_PORT_NULL;
	free_area_mem_obj_control = MACH_PORT_NULL;
	free_area_flushable = 0;
	free_area_discards_in_progress = 1;	/* see the shrinker thread */
	free_area_timer_ticking = FALSE;
	mutex_init(&free_area_shrinker_mutex);
	condition_init(&free_area_shrinker_condition);

	/*
	 * Allocate the memory object port.
	 */
	kr = mach_port_allocate(mach_task_self(),
				MACH_PORT_RIGHT_RECEIVE,
				&free_area_mem_obj);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("free_area_pager_init: mach_port_allocate"));
		panic("free_area_pager_init: can't allocate mem_obj port");
	}

	/*
	 * Get a send right for this port.
	 */
	kr = mach_port_insert_right(mach_task_self(),
				    free_area_mem_obj,
				    free_area_mem_obj,
				    MACH_MSG_TYPE_MAKE_SEND);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("free_area_pager_init: mach_port_insert_right"));
		panic("free_area_pager_init: can't insert send right");
	}

	/*
	 * Extend the size of the message queue to avoid loosing too
	 * many discard_request messages.
	 */
	port_info_count = MACH_PORT_LIMITS_INFO_COUNT;
	kr = mach_port_get_attributes(mach_task_self(),
				      free_area_mem_obj,
				      MACH_PORT_LIMITS_INFO,
				      (mach_port_info_t) &mpl,
				      &port_info_count);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("free_area_pager_init: "
			     "mach_port_get_attributes(0x%x, "
			     "MACH_PORT_LIMITS_INFO)",
			     free_area_mem_obj));
	} else {
		port_info_count = MACH_PORT_LIMITS_INFO_COUNT;
		mpl.mpl_qlimit = MACH_PORT_QLIMIT_MAX;
		kr = mach_port_set_attributes(mach_task_self(),
				      free_area_mem_obj,
				      MACH_PORT_LIMITS_INFO,
				      (mach_port_info_t) &mpl,
				      port_info_count);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("free_area_pager_init: "
				     "mach_port_set_attributes(0x%x, "
				     "MACH_PORT_LIMITS_INFO)",
				     free_area_mem_obj));
		}
	}

	/*
	 * Allocate an array to describe the status of each page
	 * the free_area microkernel-wise.
	 */
	free_area_npages = size / PAGE_SIZE;
	min_discard_page_nr = free_area_npages;
	kr = vm_allocate(mach_task_self(),
			 (vm_address_t *) &free_area_status,
			 free_area_npages * sizeof (*free_area_status),
			 TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("free_area_pager_init: vm_allocate(%d)",
			     free_area_npages * sizeof (*free_area_status)));
		panic("free_area_pager_init: can't allocate free_area_status");
	}
	/*
	 * Wire the "free_area_status" array to avoid annoying page
	 * faults on the paging path that could impact performance.
	 */
	kr = vm_wire(privileged_host_port,
		     mach_task_self(),
		     (vm_address_t) free_area_status,
		     free_area_npages * sizeof (*free_area_status),
		     VM_PROT_READ|VM_PROT_WRITE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("free_area_pager_init: vm_wire(0x%x,%d)",
			     (vm_address_t) free_area_status,
			     free_area_npages * sizeof (*free_area_status)));
		printk("free_area_pager_init: can't wire free_area_status[]\n");
	} else {
#ifdef	FREE_AREA_PAGER_DEBUG
		printk("Wired %ldKB for free_area_status[].\n",
		       PAGE_ALIGN(free_area_npages
				  * sizeof (*free_area_status)) >> PAGE_SHIFT);
#endif	/* FREE_AREA_PAGER_DEBUG */
	}
	/*
	 * Wire the "mem_map" array as well, since it's also widely
	 * accessed by the shrinker thread.
	 */
	kr = vm_wire(privileged_host_port,
		     mach_task_self(),
		     (vm_address_t) mem_map,
		     free_area_npages * sizeof (*mem_map),
		     VM_PROT_READ|VM_PROT_WRITE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("free_area_pager_init: vm_wire(0x%x,%d)",
			     (vm_address_t) mem_map,
			     free_area_npages * sizeof (*mem_map)));
		printk("free_area_pager_init: can't wire mem_map[]\n");
	} else {
#ifdef	FREE_AREA_PAGER_DEBUG
		printk("Wired %ldKB for mem_map[].\n",
		       PAGE_ALIGN(free_area_npages
				  * sizeof (*mem_map)) >> PAGE_SHIFT);
#endif	/* FREE_AREA_PAGER_DEBUG */
	}
	/* 
	 * The free_area_status[] array is zero-filled, which is OK for us:
	 * it means that all the pages are !present && !dirty
	 */

	/*
	 * Create a new Linux task for the free_area shrinker, so it can block
	 * and do real things like if it was in a user process's context.
	 * Skip task[0] (the server task) and task[1] (reserved for init).
	 */
	for (nr = 2; nr < NR_TASKS; nr++) {
		if (!task[nr])
			break;
	}
	if (nr >= NR_TASKS)
		panic("free_area_pager_init: can't find empty process");
	free_area_shrinker_task = *current;	/* XXX ? */
	strncpy(free_area_shrinker_task.comm,
		"free_area shrinker",
		sizeof (free_area_shrinker_task.comm));
	task[nr] = &free_area_shrinker_task;
	SET_LINKS(&free_area_shrinker_task);
	nr_tasks++;

	/*
	 * Start the free_area pager thread to handle requests
	 * on the free_area memory object.
	 */
	(void) server_thread_start(free_area_pager_thread, (void *) 0);

	/*
	 * Map the "free_area" zone.
	 */
	server_thread_blocking(TRUE);
	kr = vm_map(mach_task_self(),
		    &free_area_addr,
		    size,
		    ~mask,
		    TRUE,	/* anywhere */
		    free_area_mem_obj,
		    0,		/* offset */
		    FALSE,	/* copy */
		    VM_PROT_ALL,
		    VM_PROT_ALL,
		    VM_INHERIT_NONE);
	server_thread_unblocking(TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("free_area_pager_init: vm_map"));
		panic("free_area_pager_init: vm_map failed");
	}

	/*
	 * Start the "free_area" shrinker thread.
	 */
	(void) server_thread_start(free_area_shrinker_thread, (void *) 0);

	return (unsigned long) free_area_addr;
}

void
free_area_discard_pages(
	unsigned long	addr,
	unsigned long	size)
{
	kern_return_t	kr;
	int		page;
	vm_offset_t	offset;
	int		undiscard_count;
	int		undiscard_page_nr;
	int		i;
	unsigned long	data_addr;

	ASSERT(holding_uniproc());

	undiscard_count = size >> PAGE_SHIFT;

	spin_lock(&free_area_status_lock);
	for (page = MAP_NR(addr);
	     page < MAP_NR(addr + size);
	     page++) {
#if	FREE_AREA_PAGER_DEBUG
		if (free_area_pager_info_debug) {
			printk("FAP_discard(page=%d): 1st status before %s\n",
			       page, free_area_page_status(page));
		}
#endif	/* FREE_AREA_PAGER_DEBUG */
		if (free_area_status[page] & FAP_IN_USE) {
			/*
			 * This page is possibly dirty and needs to be
			 * zero-filled if re-used.
			 */
			free_area_status[page] |= FAP_DIRTY;
			free_area_flushable++;
			free_area_status[page] &= ~FAP_IN_USE;
		}
#if	FREE_AREA_PAGER_DEBUG
		if (free_area_pager_info_debug) {
			printk("FAP_discard(page=%d): 1st status after %s\n",
			       page, free_area_page_status(page));
		}
#endif	/* FREE_AREA_PAGER_DEBUG */
		if (free_area_status[page] & FAP_DISCARD) {
			free_area_status[page] &= ~FAP_DISCARD;
			undiscard_count--;
		}
		data_addr = free_area_status[page] & PAGE_MASK;
		if (data_addr != 0) {
#if	USE_SILENT_OVERWRITE
			free_area_status[page] &= ~PAGE_MASK;
#else	/* USE_SILENT_OVERWRITE */
			free_area_status[page] &= ~(PAGE_MASK | FAP_DIRTY);
#endif	/* USE_SILENT_OVERWRITE */
			spin_unlock(&free_area_status_lock);
			kr = vm_deallocate(mach_task_self(),
					   (vm_offset_t) data_addr,
					   PAGE_SIZE);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("free_area_discard_pages: "
					     "vm_deallocate(0x%x, %ld)",
					     (vm_offset_t) data_addr,
					     PAGE_SIZE));
			}
			spin_lock(&free_area_status_lock);
		}
	}

	spin_unlock(&free_area_status_lock);

	if (! free_area_discards_in_progress) {
		/*
		 * We're not too short on memory: no need to flush the
		 * pages out of the microkernel.
		 */
		return;
	}

	/*
	 * Find other pages to "un-discard_request".
	 */
	spin_lock(&free_area_status_lock);
	undiscard_page_nr = min_discard_page_nr;
	for (undiscard_page_nr = min_discard_page_nr;
	     undiscard_page_nr < free_area_npages;
	     undiscard_page_nr++) {
		if (free_area_status[undiscard_page_nr] & FAP_DISCARD) {
			break;
		}
	}
	for (i = 0;
	     i < undiscard_count && undiscard_page_nr + i < free_area_npages;
	     i++) {
		if (free_area_status[undiscard_page_nr+i] & FAP_DISCARD) {
			free_area_status[undiscard_page_nr+i] &= ~FAP_DISCARD;
		} else {	
			break;
		}
	}
	undiscard_count = i;
	min_discard_page_nr = undiscard_page_nr + undiscard_count + 1;
	/*
	 * We can undiscard "undiscard_count" pages from "undiscard_page_nr".
	 */
	spin_unlock(&free_area_status_lock);

	offset = (vm_offset_t) (addr - PAGE_OFFSET);

#ifdef	FREE_AREA_PAGER_DEBUG
	if (free_area_pager_debug) {
		printk("free_area_discard_pages(addr=0x%lx, size=0x%lx): "
		       "mo_discard_reply(req_offset=0x%x,req_length=0x%x,"
		       "discard_offset=0x%x,discard_length=0x%x)\n",
		       addr, size,
		       undiscard_page_nr << PAGE_SHIFT,
		       undiscard_count << PAGE_SHIFT,
		       offset, (vm_size_t) size);
	}
#endif	/* FREE_AREA_PAGER_DEBUG */
	kr = memory_object_discard_reply(free_area_mem_obj_control,
					 undiscard_page_nr << PAGE_SHIFT,
					 undiscard_count << PAGE_SHIFT,
					 offset,
					 (vm_size_t) size,
					 MEMORY_OBJECT_RETURN_NONE,
					 MACH_PORT_NULL);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("free_area_discard_pages(addr=0x%lx,size=0x%lx): "
			     "memory_object_discard_reply(0%x,0x%x,0x%x,0x%x)",
			     addr, size,
			     undiscard_page_nr << PAGE_SHIFT,
			     undiscard_count << PAGE_SHIFT,
			     offset, (vm_size_t) size));

		/*
		 * If the discard_reply failed, try a regular lock_request().
		 */
		kr = memory_object_lock_request(free_area_mem_obj_control,
						offset,
						(vm_size_t) size,
						MEMORY_OBJECT_RETURN_NONE,
						TRUE,	/* should_flush */
						VM_PROT_ALL,
						MACH_PORT_NULL);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("free_area_discard_pages"
				     "(addr=0x%lx,size=0x%lx): "
				     "memory_object_lock_request(0%x,0x%x)",
				     addr, size, offset, (vm_size_t) size));
		}
	}

	spin_lock(&free_area_status_lock);
	for (page = MAP_NR(addr);
	     page < MAP_NR(addr + size);
	     page++) {
#if	FREE_AREA_PAGER_DEBUG
		if (free_area_pager_info_debug) {
			printk("FAP_discard(page=%d): 2nd status before %s\n",
			       page, free_area_page_status(page));
		}
#endif	/* FREE_AREA_PAGER_DEBUG */
		ASSERT(! (free_area_status[page] & FAP_IN_USE));
		if (free_area_status[page] & FAP_PRESENT) {
			/*
			 * This page is no longer present nor dirty.
			 * Well, in fact it can still be dirty if the
			 * lock_request hasn't been processed yet.
			 */
			free_area_status[page] &= ~FAP_PRESENT;
#if	FREE_AREA_PAGER_DEBUG
			if (free_area_pager_info_debug) {
				printk("FAP_discard(page=%d): "
				       "2nd status after %s\n",
				       page, free_area_page_status(page));
			}
#endif	/* FREE_AREA_PAGER_DEBUG */
			ASSERT(free_area_flushable != 0);
			free_area_flushable--;
		} else {
#if	FREE_AREA_PAGER_DEBUG
			if (free_area_pager_info_debug) {
				printk("FAP_discard(page=%d): "
				       "2nd status after %s\n",
				       page, free_area_page_status(page));
			}
#endif	/* FREE_AREA_PAGER_DEBUG */
		}
	}
	spin_unlock(&free_area_status_lock);
}

void
free_area_mark_used(
	unsigned long	addr,
	unsigned long	size)
{
	int		page;
	kern_return_t	kr;

	ASSERT(holding_uniproc());

	spin_lock(&free_area_status_lock);
	for (page = MAP_NR(addr);
	     page < MAP_NR(addr + size);
	     page++) {
#if	FREE_AREA_PAGER_DEBUG
		if (free_area_pager_info_debug) {
			printk("FAP_mark_used(page=%d): status before %s\n",
			       page, free_area_page_status(page));
		}
#endif	/* FREE_AREA_PAGER_DEBUG */
		ASSERT(! (free_area_status[page] & FAP_IN_USE));
		free_area_status[page] |= FAP_IN_USE;
		if (! (free_area_status[page] & FAP_PRESENT)) {
			free_area_status[page] |= FAP_PRESENT;
#if	FREE_AREA_PAGER_DEBUG
			if (free_area_pager_info_debug) {
				printk("FAP_mark_used(page=%d): "
				       "status after %s\n",
				       page, free_area_page_status(page));
			}
#endif	/* FREE_AREA_PAGER_DEBUG */
			/* unavailable in advance */
			kr = memory_object_data_unavailable(free_area_mem_obj_control,
							    page << PAGE_SHIFT,
							    PAGE_SIZE);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("free_area_mark_used: "
					     "memory_object_data_unavailable"));
			}
			free_area_data_unavailables_in_advance++;
		} else {
#if	FREE_AREA_PAGER_DEBUG
			if (free_area_pager_info_debug) {
				printk("FAP_mark_used(page=%d): "
				       "status after %s\n",
				       page, free_area_page_status(page));
			}
#endif	/* FREE_AREA_PAGER_DEBUG */
#if	USE_SILENT_OVERWRITE
			ASSERT(free_area_status[page] & FAP_DIRTY);
#endif	/* USE_SILENT_OVERWRITE */
			/* page is present: nothing to do */
			free_area_flushable--;
		}
	}
	spin_unlock(&free_area_status_lock);
}

#if	FREE_AREA_PAGER_DEBUG
boolean_t
page_is_zeroed(
	unsigned long	addr)
{
	unsigned long *p;

	for (p = (unsigned long *) addr;
	     p < (unsigned long *) (addr + PAGE_SIZE);
	     p++) {
		if (*p != 0) {
			printk("page_is_zeroed(0x%lx): non-zero at %p\n",
			       addr, p);
			ASSERT(0);
			memset((void *) addr, 0, PAGE_SIZE);
			return TRUE;
		}
	}
	return TRUE;
}
#endif	/* FREE_AREA_PAGER_DEBUG */

boolean_t
free_area_page_dirty(
	unsigned long	addr)
{
	int		page;
	boolean_t	result;

	ASSERT(holding_uniproc());

	page = MAP_NR(addr);
	spin_lock(&free_area_status_lock);
	if (free_area_status[page] & FAP_DIRTY) {
		result = TRUE;
	} else {
		result = FALSE;
	}
	spin_unlock(&free_area_status_lock);
#if	FREE_AREA_PAGER_DEBUG
	if (free_area_pager_debug) {
		if (result == FALSE)
			ASSERT(page_is_zeroed(addr));
	}
#endif	/* FREE_AREA_PAGER_DEBUG */

	return result;
}

void
free_area_grow_again(
	unsigned long	ignored)
{
	min_free_pages -= free_area_grow_size;
	if ((free_area_grow_size << 1) > 0) {
		free_area_grow_size <<= 1;
	}
	if (min_free_pages < initial_min_free_pages) {
		min_free_pages = initial_min_free_pages;
		free_area_timer_ticking = FALSE;
	} else {
		free_area_timer.expires = free_area_timer_ticks + jiffies;
		add_timer(&free_area_timer);
	}
#ifdef	FREE_AREA_PAGER_DEBUG
	if (free_area_shrinker_debug) {
		printk("free_area_grow_again: "
		       "min_free_pages = %d, nr_free_pages = %d\n",
		       min_free_pages, nr_free_pages);
	}
#endif	/* FREE_AREA_PAGER_DEBUG */
}

kern_return_t
free_area_object_change_completed(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	memory_object_flavor_t	flavor)
{
	panic("free_area_object_change_completed");
}

kern_return_t
free_area_object_data_request(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length,
	vm_prot_t		desired_access)
{
	kern_return_t		kr;
	int			page_nr;
	unsigned long		data_addr;

#ifdef	FREE_AREA_PAGER_DEBUG
	if (free_area_pager_debug) {
		printk("free_area_object_data_request: "
		       "offset 0x%x length 0x%x\n",
		       offset, length);
	}
#endif	/* FREE_AREA_PAGER_DEBUG */

	ASSERT(length == PAGE_SIZE);
	page_nr = offset >> PAGE_SHIFT;

	spin_lock(&free_area_status_lock);
	ASSERT(free_area_status[page_nr] & FAP_IN_USE);
	data_addr = free_area_status[page_nr] & PAGE_MASK;
	ASSERT(free_area_status[page_nr] & FAP_PRESENT);
	free_area_status[page_nr] &= ~FAP_DISCARD;

	if (data_addr == 0) {
		spin_unlock(&free_area_status_lock);
		/*
		 * We don't have the page here. Tell the microkernel
		 * it's unavailable (i.e. it needs to be zero-filled).
		 */
		ASSERT(data_addr == 0);
#ifdef	FREE_AREA_PAGER_DEBUG
		if (free_area_pager_debug) {
			printk("mo_data_request: mo_data_unavailable("
			       "mo_ctl=0x%x, off=0x%x, size=0x%x)\n",
			       mem_obj_control,
			       offset,
			       length);
		}
		if (free_area_shrinker_debug) {
			printk("@");
		}
#endif	/* FREE_AREA_PAGER_DEBUG */

		kr = memory_object_data_unavailable(mem_obj_control,
						    offset,
						    length);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("free_area_object_data_request: "
				     "memory_object_data_unavailable"));
			panic("free_area_object_data_request: "
			      "memory_object_data_unavailable failed");
		}
		free_area_data_unavailables++;
	} else {
		/*
		 * We have the page here (from a m_o_data_return).
		 * Provide it to the microkernel.
		 */
#ifdef	FREE_AREA_PAGER_DEBUG
		if (free_area_pager_debug || free_area_pageout_debug) {
			printk("** offset 0x%x REQUEST 0x%lx CHKSUM 0x%x\n",
			       offset, data_addr,
			       ip_compute_csum((unsigned char *) data_addr,
					       PAGE_SIZE));
		}
		if (free_area_shrinker_debug) {
			printk("+");
		}
#endif	/* FREE_AREA_PAGER_DEBUG */
		/* reset the page address: we don't keep the page */
		free_area_status[page_nr] &= ~PAGE_MASK;
		spin_unlock(&free_area_status_lock);
		ASSERT(data_addr != 0);

#ifdef	FREE_AREA_PAGER_DEBUG
		if (free_area_pager_debug) {
			printk("mo_data_request: mo_data_supply("
			       "mo_ctl=0x%x, off=0x%x, size=0x%x, data=0x%x)\n",
			       mem_obj_control,
			       offset,
			       length,
			       (vm_offset_t) data_addr);
		}
#endif	/* FREE_AREA_PAGER_DEBUG */

		kr = memory_object_data_supply(mem_obj_control,
					       offset,
					       (vm_offset_t) data_addr,
					       length,
					       TRUE,
					       VM_PROT_NONE,
					       TRUE,
					       MACH_PORT_NULL);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("free_area_object_data_request: "
				     "memory_object_data_supply"
				     "(off=0x%x size=0x%x data=0x%x)",
				     offset, length,
				     (vm_offset_t) data_addr));
			panic("free_area_object_data_request: "
			      "memory_object_data_supply failed");
		}

		free_area_data_supplies++;
	}

#ifdef	FREE_AREA_PAGER_DEBUG
	if (free_area_pager_debug) {
		printk("free_area_object_data_request: done\n");
	}
#endif	/* FREE_AREA_PAGER_DEBUG */

	return KERN_SUCCESS;
}

kern_return_t
free_area_object_data_return(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_offset_t		data,
	vm_size_t		length,
	boolean_t		dirty,
	boolean_t		kernel_copy)
{
	int		page_nr;
	kern_return_t	kr;
	unsigned long	data_addr;
	boolean_t	dealloc_data;

#ifdef	FREE_AREA_PAGER_DEBUG
	if (free_area_pager_debug) {
		printk("free_area_object_data_return(off=0x%x length=0x%x\n",
		       offset, length);
	}
#endif	/* FREE_AREA_PAGER_DEBUG */

	free_area_data_returns++;
#ifdef DUMP_FREE_AREA_STATS
	if ((free_area_data_returns & 0xFF) == 1) free_area_dump_stats();
#endif

	/*
	 * It seems that the microkernel is still paging...
	 * Let's try to shrink the buffer cache some more.
	 */
	if (mutex_try_lock(&free_area_shrinker_mutex)) {
		free_area_discards_in_progress += length >> PAGE_SHIFT;
		/*
		 * Wake up the shrinker thread.
		 */
		condition_signal(&free_area_shrinker_condition);
		mutex_unlock(&free_area_shrinker_mutex);
	} else {
		/*
		 * We don't have time to waste on taking the mutex,
		 * just increment and hope it's atomic. Otherwise,
		 * what the heck, this counter is just a hint.
		 */
		free_area_discards_in_progress += length >> PAGE_SHIFT;
	}

	/*
	 * The kernel has forced a mandatory pageout on this page,
	 * so it must be really needing the space.
	 * The page it has sent us ("data") is now backed by the default
	 * pager (it's in an internal object), so just remember this
	 * address (for a future m_o_data_request on it) and let the
	 * default pager write the page to its paging space.
	 */
	page_nr = offset >> PAGE_SHIFT;
	ASSERT((data & ~PAGE_MASK) == 0);
	ASSERT(length == PAGE_SIZE);

	spin_lock(&free_area_status_lock);

	data_addr = free_area_status[page_nr] & PAGE_MASK; /* old data */
	free_area_status[page_nr] &= ~(PAGE_MASK | FAP_DISCARD);
	if (free_area_status[page_nr] & FAP_IN_USE) {
#if	USE_SILENT_OVERWRITE
		/*
		 * If we keep the page and it's overwritten in the
		 * micro-kernel, the old contents might sit in the
		 * mklinux server or in the default pager and waste
		 * physical memory or disk space. We'd better return
		 * the page to the kernel right now...
		 */
		dealloc_data = FALSE;
		kr = memory_object_data_supply(mem_obj_control,
					       offset,
					       data,
					       length,
					       TRUE,
					       VM_PROT_NONE,
					       TRUE,
					       MACH_PORT_NULL);
		if (kr != KERN_SUCCESS) {
			if (kr == KERN_MEMORY_PRESENT) {
				/* can happen with silent_overwrite ! */
				dealloc_data = TRUE;
			} else {
				MACH3_DEBUG(1, kr,
					    ("free_area_object_data_return: "
					     "memory_object_data_supply"
					     "(off=0x%x size=0x%x data=0x%x)",
					     offset, length, data));
				panic("free_area_object_data_return: "
				      "memory_object_data_supply failed");
			}
		}
#else	/* USE SILENT_OVERWRITE */
		free_area_status[page_nr] |= (data & PAGE_MASK);
		dealloc_data = FALSE;
#endif	/* USE_SILENT_OVERWRITE */
	} else {
		/* the page isn't used: no need to keep the data */
#if	USE_SILENT_OVERWRITE
		free_area_status[page_nr] &= ~FAP_PRESENT;
#else	/* USE_SILENT_OVERWRITE */
		free_area_status[page_nr] &= ~(FAP_PRESENT|FAP_DIRTY);
#endif	/* USE_SILENT_OVERWRITE */
		dealloc_data = TRUE;
	}
#ifdef	FREE_AREA_PAGER_DEBUG
	if (free_area_pager_debug || free_area_pageout_debug) {
		printk("** offset 0x%x RETURN 0x%x CHKSUM 0x%x\n",
		       offset, data,
		       ip_compute_csum((unsigned char *) data,
				       PAGE_SIZE));
	}
	if (free_area_shrinker_debug) {
		printk("-");
	}
#endif	/* FREE_AREA_PAGER_DEBUG */
	spin_unlock(&free_area_status_lock);

	if (dealloc_data) {
		kr = vm_deallocate(mach_task_self(),
				   data,
				   PAGE_SIZE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("free_area_object_data_return: "
				     "vm_deallocate(data=0x%x)",
				     data));
		}
	}

	if (data_addr) {
		free_area_data_returns_for_nothing++;
		kr = vm_deallocate(mach_task_self(),
				   (vm_offset_t) data_addr,
				   PAGE_SIZE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("free_area_object_data_return: "
				     "vm_deallocate(0x%x)",
				     (vm_offset_t) data_addr));
		}
	}

	return KERN_SUCCESS;
}

kern_return_t
free_area_object_synchronize(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_offset_t		length,
	vm_sync_t		flags)
{
	printk("free_area_object_synchronize!!\n");
	return KERN_SUCCESS;
}

kern_return_t
free_area_object_data_unlock(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length,
	vm_prot_t		desired_access)
{
	panic("free_area_object_data_unlock");
}

kern_return_t
free_area_object_discard_request(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length)
{
	int		page_nr;
	int		num_pages, i;

#ifdef	FREE_AREA_PAGER_DEBUG
	if (free_area_pager_debug) {
		printk("free_area_object_discard_request"
		       "(offset=0x%x,length=0x%x)\n",
		       offset, length);
	}
	if (free_area_shrinker_debug) {
		printk("!");
	}
#endif	/* FREE_AREA_PAGER_DEBUG */

	num_pages = length >> PAGE_SHIFT;
	free_area_discard_requests += num_pages;

	/*
	 * Remember that this page has been selected to be discarded,
	 * so that we can undo that in a m_o_discard_reply() later.
	 */
	page_nr = offset >> PAGE_SHIFT;
	spin_lock(&free_area_status_lock);
	for (i = 0; i < num_pages; i++) {
		free_area_status[page_nr+i] |= FAP_DISCARD;
	}
	if (page_nr < min_discard_page_nr)
		min_discard_page_nr = page_nr;
	spin_unlock(&free_area_status_lock);

	if (mutex_try_lock(&free_area_shrinker_mutex)) {
		free_area_discards_in_progress += num_pages;
		/*
		 * Wake up the shrinker thread.
		 */
		condition_signal(&free_area_shrinker_condition);
		mutex_unlock(&free_area_shrinker_mutex);
	} else {
		/*
		 * We don't have time to waste on taking the mutex,
		 * just increment and hope it's atomic. Otherwise,
		 * what the heck, this counter is just a hint.
		 */
		free_area_discards_in_progress += num_pages;
	}

#ifdef	FREE_AREA_PAGER_DEBUG
	if (free_area_pager_debug) {
		printk("free_area_object_discard_request"
		       "(offset=0x%x,length=0x%x): done\n",
		       offset, length);
	}
#endif	/* FREE_AREA_PAGER_DEBUG */

	return KERN_SUCCESS;
}


kern_return_t
free_area_object_init(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_size_t		page_size)
{
	memory_object_attr_info_data_t		attributes;
	memory_object_behave_info_data_t	behavior;
	kern_return_t				kr;

#ifdef	FREE_AREA_PAGER_DEBUG
	if (free_area_pager_debug) {
		printk("free_area_object_init: obj 0x%x, control 0x%x\n",
		       mem_obj, mem_obj_control);
	}
#endif	/* FREE_AREA_PAGER_DEBUG */

	free_area_mem_obj_control = mem_obj_control;

	/*
	 * Tell the micro-kernel that the memory object is ready on our side.
	 */
	attributes.copy_strategy = MEMORY_OBJECT_COPY_NONE;
	attributes.cluster_size = PAGE_SIZE;
	attributes.may_cache_object = FALSE;
	attributes.temporary = FALSE;
	kr = memory_object_change_attributes(mem_obj_control,
					     MEMORY_OBJECT_ATTRIBUTE_INFO,
					     (memory_object_info_t) &attributes,
					     MEMORY_OBJECT_ATTR_INFO_COUNT,
					     MACH_PORT_NULL);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("free_area_object_init: "
			     "mo_change_attributes(ATTRIBUTE)"));
		panic("free_area_object_init: change_attributes");
	}

	behavior.copy_strategy = MEMORY_OBJECT_COPY_NONE;
	behavior.temporary = FALSE;
	behavior.invalidate = FALSE;
#if	USE_SILENT_OVERWRITE
	behavior.silent_overwrite = TRUE;
#else	/* USE_SILENT_OVERWRITE */
	behavior.silent_overwrite = FALSE;
#endif	/* USE_SILENT_OVERWRITE */
	behavior.advisory_pageout = TRUE;
	kr = memory_object_change_attributes(mem_obj_control,
					     MEMORY_OBJECT_BEHAVIOR_INFO,
					     (memory_object_info_t) &behavior,
					     MEMORY_OBJECT_BEHAVE_INFO_COUNT,
					     MACH_PORT_NULL);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("free_area_object_init: "
			     "mo_change_attributes(BEHAVIOR)"));
		panic("free_area_object_init: change_attributes");
	}

	return KERN_SUCCESS;
}

kern_return_t
free_area_object_lock_completed(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length)
{
	panic("free_area_object_lock_completed");
}

kern_return_t
free_area_object_supply_completed(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length,
	kern_return_t		result,
	vm_offset_t		error_offset)
{
	panic("free_area_object_supply_completed");
}

kern_return_t
free_area_object_terminate(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control)
{
	printk("free_area_object_terminate!!\n");
	return KERN_SUCCESS;
}

#ifdef DUMP_FREE_AREA_STATS
#define P(x) printk(#x " = %ld\n", x)
void
free_area_dump_stats(void)
{
  P(free_area_data_unavailables);
  P(free_area_data_unavailables_in_advance);
  P(free_area_data_supplies);
  P(free_area_data_returns);
  P(free_area_data_returns_for_nothing);
  P(free_area_discard_requests);
  P(free_area_discard_unused_success);
  P(free_area_discard_unused_failure);
  P(free_area_discard_shrink_success);
  P(free_area_discard_shrink_failure);
}
#endif

#define memory_object_server		free_area_object_server
#define memory_object_server_routine	free_area_object_server_routine
#define memory_object_subsystem		free_area_object_subsystem

#define memory_object_init		free_area_object_init
#define memory_object_terminate		free_area_object_terminate
#define memory_object_data_request	free_area_object_data_request
#define memory_object_data_unlock	free_area_object_data_unlock
#define memory_object_lock_completed	free_area_object_lock_completed
#define memory_object_supply_completed	free_area_object_supply_completed
#define memory_object_data_return	free_area_object_data_return
#define memory_object_synchronize	free_area_object_synchronize
#define memory_object_change_completed	free_area_object_change_completed
#define memory_object_discard_request	free_area_object_discard_request

#include "memory_object_server.c"
