/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _MACHINE_SEMAPHORE_H
#define _MACHINE_SEMAPHORE_H

/*
 * SMP- and interrupt-safe semaphores..
 *
 * (C) Copyright 1996 Linus Torvalds
 */

#include <asm/atomic.h>
#include <asm/system.h>

struct semaphore {
	atomic_t count;
	atomic_t waking;
	int lock;			/* to make waking testing atomic */
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, 1, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, 1, NULL })

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
#error need code to take the lock here !
#endif
} /* get_buzz_lock */

extern inline void give_buzz_lock(int *lock_ptr)
{
#ifdef	__SMP__
#error need code to release the lock here !
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

#endif /* _MACHINE_SEMAPHORE_H */
