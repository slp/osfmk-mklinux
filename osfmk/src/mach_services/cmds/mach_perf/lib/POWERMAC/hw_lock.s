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

#include <ppc/asm.h>

/*
 *      void hw_lock_init(hw_lock_t)
 *
 *      Initialize a hardware lock.
 * TODO NMGS Manual talks about possible races if write to same cache line?
 * TODO NMGS should these locks take up a whole cache line? See HP code
 */

/*
 *      void hw_lock_unlock(hw_lock_t)
 *
 *      Unconditionally release lock.
 *      MACH_RT:  release preemption level. TODO NMGS
 */

	.text

ENTRY2(hw_lock_init, hw_lock_unlock, TAG_NO_FRAME_USED)

	sync				/* Flush writes done under lock */
	li	r0,	0		/* set lock to free == 0 */
	stw	r0,	0(ARG0)
	blr

/*
 *      void hw_lock_lock(hw_lock_t)
 *
 *      Acquire lock, spinning until it becomes available.
 *      MACH_RT:  also return with preemption disabled. TODO NMGS
 */

ENTRY(hw_lock_lock, TAG_NO_FRAME_USED)

	li	ARG1,	1		/* value to be stored... 1==taken*/
.Llock_lock_loop:	
	lwarx	ARG2,	0,ARG0		/* Ld from addr of ARG0 and reserve */
	cmpwi	ARG2,	0		/* TEST... */
	bne-	.Llock_lock_loop	/* loop if BUSY (predict not busy) */
	
	stwcx.  ARG1,	0,ARG0		/* And SET (if reserved) */
	bne-	.Llock_lock_loop	/* If set failed, loop back */
	isync
	blr

/*
 *      unsigned int hw_lock_try(hw_lock_t)
 *
 *      try to acquire spin-lock. Return success (1) or failure (0)
 *      MACH_RT:  returns with preemption disabled on success. TODO NMGS
 *
 */
ENTRY(hw_lock_try,TAG_NO_FRAME_USED)

	li	ARG1,	1		/* value to be stored... 1==taken */
.Llock_try_loop:	
	lwarx	ARG2,	0,ARG0		/* Ld from addr of arg and reserve */

	cmpwi	ARG2,	0		/* TEST... */
	bne-	.Llock_try_failed	/* branch if taken. Predict free */
	
	stwcx.  ARG1,	0,ARG0
		/* And SET (if reserved) */
	bne-	.Llock_lock_loop	/* If set failed, loop back */
	isync

	li	ARG0,	1		/* SUCCESS - lock was free */
	blr

.Llock_try_failed:
	li	ARG0,	0		/* FAILURE - lock was taken */
	blr

/*
 *      unsigned int hw_lock_held(hw_lock_t)
 *
 *      Return 1 if lock is held
 *      MACH_RT:  doesn't change preemption state.
 *      N.B.  Racy, of course.
 *
 */
ENTRY(hw_lock_held,TAG_NO_FRAME_USED)

	lwz	ARG0, 0(ARG0)		/* Return value of lock */
	blr
