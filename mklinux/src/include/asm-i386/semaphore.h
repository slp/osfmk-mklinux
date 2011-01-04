#ifndef _I386_SEMAPHORE_H
#define _I386_SEMAPHORE_H

#include <linux/linkage.h>
#include <asm/system.h>

/*
 * SMP- and interrupt-safe semaphores..
 *
 * (C) Copyright 1996 Linus Torvalds
 *
 * Modified 1996-12-23 by Dave Grothe <dave@gcom.com> to fix bugs in
 *                     the original code and to make semaphore waits
 *                     interruptible so that processes waiting on
 *                     semaphores can be killed.
 *
 * If you would like to see an analysis of this implementation, please
 * ftp to gcom.com and download the file
 * /pub/linux/src/semaphore/semaphore-2.0.24.tar.gz.
 *
 */

struct semaphore {
	int count;
	int waking;
	int lock ;			/* to make waking testing atomic */
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, 0, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, 0, NULL })

asmlinkage void down_failed(void /* special register calling convention */);
asmlinkage void up_wakeup(void /* special register calling convention */);

extern void __down(struct semaphore * sem);
extern void __up(struct semaphore * sem);

/*
 * This is ugly, but we want the default case to fall through.
 * "down_failed" is a special asm handler that calls the C
 * routine that actually waits. See arch/i386/lib/semaphore.S
 */
extern inline void down(struct semaphore * sem)
{
	__asm__ __volatile__(
		"# atomic down operation\n\t"
		"movl $1f,%%eax\n\t"
#ifdef __SMP__
		"lock ; "
#endif
		"decl 0(%0)\n\t"
		"js " SYMBOL_NAME_STR(down_failed) "\n"
		"1:\n"
		:/* no outputs */
		:"c" (sem)
		:"ax","dx","memory");
}

/*
 * Primitives to spin on a lock.  Needed only for SMP version.
 */
extern inline void get_buzz_lock(int *lock_ptr)
{
#ifdef __SMP__
        while (xchg(lock_ptr,1) != 0) ;
#endif
} /* get_buzz_lock */

extern inline void give_buzz_lock(int *lock_ptr)
{
#ifdef __SMP__
        *lock_ptr = 0 ;
#endif
} /* give_buzz_lock */

asmlinkage int down_failed_interruptible(void);  /* params in registers */

/*
 * This version waits in interruptible state so that the waiting
 * process can be killed.  The down_failed_interruptible routine
 * returns negative for signalled and zero for semaphore acquired.
 */
extern inline int down_interruptible(struct semaphore * sem)
{
	int	ret ;

        __asm__ __volatile__(
                "# atomic interruptible down operation\n\t"
                "movl $2f,%%eax\n\t"
#ifdef __SMP__
                "lock ; "
#endif
                "decl 0(%1)\n\t"
                "js " SYMBOL_NAME_STR(down_failed_interruptible) "\n\t"
                "xorl %%eax,%%eax\n"
                "2:\n"
                :"=a" (ret)
                :"c" (sem)
                :"ax","dx","memory");

	return(ret) ;
}

/*
 * Note! This is subtle. We jump to wake people up only if
 * the semaphore was negative (== somebody was waiting on it).
 * The default case (no contention) will result in NO
 * jumps for both down() and up().
 */
extern inline void up(struct semaphore * sem)
{
	__asm__ __volatile__(
		"# atomic up operation\n\t"
		"movl $1f,%%eax\n\t"
#ifdef __SMP__
		"lock ; "
#endif
		"incl 0(%0)\n\t"
		"jle " SYMBOL_NAME_STR(up_wakeup)
		"\n1:"
		:/* no outputs */
		:"c" (sem)
		:"ax", "dx", "memory");
}

#endif
