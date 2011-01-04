/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * Provides functions for allocating and deallocating the mach_msg buffers
 * used by MIG client stubs.
 *
 * These functions will be called automatically by MIG client stubs that
 * are compiled with the -maxonstack flag, if the msg buffer is bigger than
 * the limit specified.  The purpose is to avoid excessive stack usage.
 *
 * Currently, we use a single list of vm_allocated buffers whose members are
 * big enough for the largest message buffer in any of our MIG client stubs,
 * namely 9KB.  This is okay, because it's only virtual memory, and it
 * is better for locality of reference to reuse a small set of fixed-size
 * buffers rather than to use more buffers of lots of different sizes.
 */

#include <cthreads.h>

#include <mach_init.h>
#include <mach/mach_interface.h>

#include <linux/kernel.h>

spin_lock_t mig_user_lock;

struct mig_user_buff {
	struct mig_user_buff *next;
} *mig_user_buffs = NULL;

int mig_user_buff_count = 0;

/* Following is an arbitrarily chosen number that we hope is larger than
 * any MIG stub needs.  This hard-coded limit is bad practice, but it
 * greatly simplifies and speeds-up memory allocation.  There is only
 * one free list of message buffers, and they are all the same size.
 * XXX
 */
#define MSG_BUFF_SIZE (4096 * 5)

/* 
 * Get a buffer off the free list, if any, else vm_allocate one:
 */
char *
mig_user_allocate(unsigned int msg_size)
{
	struct mig_user_buff *new_buff;
	static boolean_t inited;

	if (!inited) {	/* Assume first called when single-threaded. */
		spin_lock_init(&mig_user_lock);
		inited = TRUE;
	}
	if (msg_size > MSG_BUFF_SIZE)
		panic("mig_user_allocate: requested msg buffer too large");
	spin_lock(&mig_user_lock);
	if (!mig_user_buffs) {
		kern_return_t rtn;

		mig_user_buff_count++;
		spin_unlock(&mig_user_lock);
		/* Following depends on the vm_allocate MIG stub not needing
		 * to call mig_user_allocate, i.e. that its mach_msg header
		 * is smaller than the -maxonstack argument used with MIG:
		 * Otherwise, this is infinite recursion:
		 */
		rtn = vm_allocate(mach_task_self(),
				  (vm_address_t *)&new_buff,
				  (vm_size_t)MSG_BUFF_SIZE, TRUE);
		if (rtn != KERN_SUCCESS)
			panic("mig_user_allocate: vm_allocate failed");
	} else {
		new_buff = mig_user_buffs;
		mig_user_buffs = mig_user_buffs->next;
		spin_unlock(&mig_user_lock);
	}

	return (char *)new_buff;
}

/*
 * Push the freed up buffer onto the free list:
 */
void
mig_user_deallocate(char *msg, unsigned int msg_size)
{
	struct mig_user_buff *free_buff;

	spin_lock(&mig_user_lock);
	free_buff = (struct mig_user_buff *)msg;
	free_buff->next = mig_user_buffs;
	mig_user_buffs = free_buff;
	spin_unlock(&mig_user_lock);
}
