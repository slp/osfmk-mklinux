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
 * 
 */
/*
 * MkLinux
 */

/*
 * mach/flipc_locks.h
 *
 * The machine independent part of the flipc_simple_locks definitions.
 * Most of the locks definitions is in flipc_dep.h, but what isn't
 * dependent on the platform being used is here.
 */

/*
 * Note that the locks defined in this file and in flipc_dep.h are only
 * for use by user level code.  The reason why this file is visible to
 * the kernel is that the kernel section of flipc needs to initialize
 * these locks.
 */

#ifndef _MACH_FLIPC_LOCKS_H_
#define _MACH_FLIPC_LOCKS_H_

/* Get the simple lock type.  */
#include <mach/flipc_cb.h>

/*
 * Lock function prototypes.  This needs to be before any lock definitions
 * that happen to be macros.
 */

/* Initializes lock.  Always a macro (so that kernel code can use it without
   library assistance).  */
void flipc_simple_lock_init(flipc_simple_lock *lock);

/* Returns 1 if lock gained, 0 otherwise.  */
int flipc_simple_lock_try(flipc_simple_lock *lock);

/* Returns 1 if lock is locked, 0 otherwise.  */
int flipc_simple_lock_locked(flipc_simple_lock *lock);

/* Releases the lock.  */
void flipc_simple_lock_release(flipc_simple_lock *lock);

/* Take the lock.  */
void flipc_simple_lock_acquire(flipc_simple_lock *lock);

/* Take two locks.  Does not hold one while spinning on the
   other.  */
void flipc_simple_lock_acquire_2(flipc_simple_lock *lock1,
			   flipc_simple_lock *lock2);

/* Get the machine dependent stuff.  The things that need to be
 * defined in a machine dependent fashion are:
 *
 *   flipc_simple_lock_init	(must be a macro)
 *   flipc_simple_lock_try
 *   flipc_simple_lock_locked
 *   flipc_simple_lock_release
 *
 * These last three don't necessarily have to be macros, but if they
 * aren't definitions must be included in the machine dependent
 * part of the user level library code.
 */
#include <mach/machine/flipc_dep.h>

/*
 * Set at flipc initialization time to thread_yield argument to
 * FLIPC_domain_init
 */

extern void (*flipc_simple_lock_yield_fn)(void);

/*
 * Machine independent definitions; defined in terms of above routines.
 */

/* Take the lock.  Assumes an external define saying how long to
   spin, and an external function to call when we've spun too long.  */
#define flipc_simple_lock_acquire(lock)		\
do {						\
  int __spin_count = 0;				\
						\
  while (flipc_simple_lock_locked(lock)		\
	 || !flipc_simple_lock_try(lock))	\
    if (++__spin_count > LOCK_SPIN_LIMIT) {	\
      (*flipc_simple_lock_yield_fn)();		\
      __spin_count = 0;				\
    }						\
} while (0)

/* Take two locks.  Hold neither while spinning on the other.  */
#define flipc_simple_lock_acquire_2(lock1, lock2)	\
do {							\
  int __spin_count = 0;					\
							\
  while (1) {						\
    while (flipc_simple_lock_locked(lock1)		\
	   || !flipc_simple_lock_try(lock1))		\
      if (++__spin_count > LOCK_SPIN_LIMIT) {		\
	(*flipc_simple_lock_yield_fn)();		\
	__spin_count = 0;				\
      }							\
							\
    if (flipc_simple_lock_try(lock2))			\
      break;						\
    flipc_simple_lock_release(lock1);			\
							\
    while (flipc_simple_lock_locked(lock2)		\
	   || !flipc_simple_lock_try(lock2))		\
      if (++__spin_count > LOCK_SPIN_LIMIT) {		\
	(*flipc_simple_lock_yield_fn)();		\
	__spin_count = 0;				\
      }							\
							\
    if (flipc_simple_lock_try(lock1))			\
      break;						\
    flipc_simple_lock_release(lock2);			\
  }							\
} while (0)

#endif	/* _MACH_FLIPC_LOCKS_H_ */
