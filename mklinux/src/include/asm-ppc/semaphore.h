#ifndef _PPC_SEMAPHORE_H
#define _PPC_SEMAPHORE_H

/*
 * SMP- and interrupt-safe semaphores..
 *
 * (C) Copyright 1996 Linus Torvalds
 * Adapted for PowerPC by Gary Thomas
 */

#include <asm/atomic.h>

struct semaphore {
	atomic_t count;
	atomic_t waiting;
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, NULL })

extern void __down(struct semaphore * sem);
extern void __up(struct semaphore * sem);

extern inline void down(struct semaphore * sem)
{
	for (;;) {
		if (atomic_dec_return(&sem->count) >= 0)
			break;
		__down(sem);
	}
}

extern inline void up(struct semaphore * sem)
{
	if (atomic_inc_return(&sem->count) <= 0)
		__up(sem);
}	

#endif
