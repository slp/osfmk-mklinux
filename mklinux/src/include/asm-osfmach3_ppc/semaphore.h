/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_PPC_SEMAPHORE_H
#define _ASM_OSFMACH3_PPC_SEMAPHORE_H

/*
 * SMP- and interrupt-safe semaphores..
 *
 * (C) Copyright 1996 Linus Torvalds
 * Adapted for PowerPC by Gary Thomas
 */

#include <asm/atomic.h>

struct semaphore {
	atomic_t count;
	atomic_t waking;
	int lock;		/* XXX lock granularity */
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, 0, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, 0, NULL })

extern void __down(struct semaphore * sem);
extern int  __down_interruptible(struct semaphore *sem);
extern void __up(struct semaphore * sem);

extern inline void down(struct semaphore * sem)
{
	if (atomic_dec_return(&sem->count) < 0)
		__down(sem);
}

/*
 * Primitives to spin on a lock.  Needed only for SMP version.
 */
extern inline void get_buzz_lock(int *lock_ptr)
{
#ifdef __SMP__
#error need code to get the lock here
#endif
} /* get_buzz_lock */

extern inline void give_buzz_lock(int *lock_ptr)
{
#ifdef __SMP__
#error need code to release the lock here
#endif
} /* give_buzz_lock */

extern inline int down_interruptible(struct semaphore *sem)
{
	int ret = 0;
	if (atomic_dec_return(&sem->count) < 0)
		ret = __down_interruptible(sem);
	return ret;
}

extern inline void up(struct semaphore * sem)
{
	if (atomic_inc_return(&sem->count) <= 0)
		__up(sem);
}	

#endif	/* _ASM_OSFMACH3_PPC_SEMAPHORE_H */
