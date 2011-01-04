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
 * mach/i386/flipc_dep.h
 *
 * This file will have all of the FLIPC implementation machine dependent
 * defines that need to be visible to both kernel and AIL (eg. bus locks
 * and bus synchronization primitives).
 */

#ifndef _MACH_FLIPC_DEP_H_
#define _MACH_FLIPC_DEP_H_

/* For the 386, we don't need to wrap synchronization variable writes
   at all.  */
#define SYNCVAR_WRITE(statement)  statement

/* And similarly (I believe; check on this), for the 386 there isn't any
   requirement for write fences.  */
#define WRITE_FENCE()

/*
 * Flipc simple lock defines.  These are almost completely for the use
 * of the AIL; the reason they are in this file is that they need to
 * be initialized properly in the communications buffer initialization
 * routine.  Sigh.  Note in particular that the kernel has no defined
 * "simple_lock_yield_function", so it had better never expand the
 * macro simple_lock_acquire.
 *
 * These locks may be declared by "flipc_simple_lock lock;".  If they
 * are instead declared by FLIPC_DECL_SIMPLE_LOCK(class,lockname) they
 * may be used without initialization.
 */

#define SIMPLE_LOCK_INITIALIZER 0
#define FLIPC_DECL_SIMPLE_LOCK(class,lockname) \
class flipc_simple_lock (lockname) = SIMPLE_LOCK_INITIALIZER

/*
 * Lower case because they may be macros or functions.
 * I'll include the function prototypes just for examples here.
 */

#define flipc_simple_lock_init(lock)		\
do {						\
    *(lock) = SIMPLE_LOCK_INITIALIZER;		\
} while (0)

/*
 * Defines of the actual routines, for gcc.
 */

#define flipc_simple_lock_locked(lock) ((*lock) != SIMPLE_LOCK_INITIALIZER)

#ifdef __GNUC__
     extern __inline__ int flipc_simple_lock_try(flipc_simple_lock *lock)
{
    int r;
    __asm__ volatile("movl $1, %0; xchgl %0, %1" : "=&r" (r), "=m" (*lock));
    return !r;
}

/* I don't know why this requires an ASM, but I'll follow the leader. */
extern __inline__ void flipc_simple_lock_release(flipc_simple_lock *lock)
{
    register int t;				
    
    __asm__ volatile("xorl %0, %0; xchgl %0, %1" : "=&r" (t), "=m" (*lock));
} 
#else	/* __GNUC__ */
/* If we aren't compiling with gcc, the above need to be functions.  */
#endif	/* __GNUC__ */

#endif /* _MACH_FLIPC_DEP_H_ */
