/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_USER_MEMORY_H_
#define _OSFMACH3_USER_MEMORY_H_

#include <mach/mach_types.h>

#include <osfmach3/queue.h>
#include <osfmach3/macro_help.h>

#include <linux/types.h>
#include <linux/sched.h>

typedef struct user_memory *user_memory_t;

struct user_memory {
	struct task_struct *task;    /* Which task's memory is this? */
	vm_address_t	user_page;   /* Page number of user address */
	vm_address_t	svr_addr;    /* Starting addr in server (page aligned)*/
	vm_size_t	size;	     /* Number of bytes (page aligned). */
	vm_prot_t	prot;	     /* Access protection for this region */

	user_memory_t	next_intask; /* Next/prev shared regions for this */
	user_memory_t	prev_intask; /*	task (protected by user_memory_lock) */
	user_memory_t	next_inchain;/* Next memory in collision resolution */
				     /* chain (protected by hash tbl locks) */
	user_memory_t	next_mru;    /* Most recently used list */
	user_memory_t	prev_mru;
	boolean_t	is_fresh;    /* Is in the "fresh" part of the mru list
				      * (see NUM_FRESH_BLOCKS, below
				      */
	int		ref_count;   /* Protects the struct and the actual
				      * data region from deallocation or change
				      * (except list links have other locks)
				      */
};

extern user_memory_t user_memory_slow_lookup(task_struct_t task,
					     vm_address_t user_addr,
					     vm_size_t size,
					     vm_prot_t prot,
					     vm_address_t *svr_addrp);
extern void user_memory_unlock(user_memory_t);
extern void user_memory_task_init(task_struct_t task);
extern void user_memory_flush_task(osfmach3_mach_task_struct_t mach_task);
extern void user_memory_flush_area(osfmach3_mach_task_struct_t mach_task,
				   vm_address_t start,
				   vm_address_t end);
extern void user_memory_lookup(task_struct_t task,
			       vm_address_t addr,
			       vm_size_t size,
			       vm_prot_t prot,
			       vm_address_t *svr_addrp,
			       user_memory_t *ump);
extern int user_memory_verify_area(int type,
				   const void *addr,
				   unsigned long size);
extern int user_memory_get_max_filename(unsigned long address);
extern unsigned long get_phys_addr(task_struct_t p,
				   unsigned long ptr,
				   user_memory_t *um);

#define user_memory_task_terminate(task)				\
	user_memory_flush_task(task)

#endif	/* _OSFMACH3_USER_MEMORY_H_ */
