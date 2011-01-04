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
 *  int hw_lock_try(spin_lock_t *s)
 *
 *  try to acquire spin-lock. Return success (1) or failure (0)
 *
 */
ENTRY(spin_try_lock,TAG_NO_FRAME_USED)

	li	r0,	1		/* value to be stored... 1==taken */
.L_lock_try_loop:	
	lwarx	ARG2,	0,ARG0		/* Ld from addr of arg and reserve */

	cmpwi	ARG2,	0		/* TEST... */
	bne-	.L_lock_try_failed	/* branch if taken. Predict free */
	
	stwcx.  r0,	0,ARG0
		/* And SET (if reserved) */
	bne-	.L_lock_try_loop	/* If set failed, loop back */
	isync

	li	ARG0,	1		/* SUCCESS - lock was free */
	blr

.L_lock_try_failed:
	li	ARG0,	0		/* FAILURE - lock was taken */
	blr

/*
 * void spin_unlock(spin_lock_t *s)
 *
 * Unconditionally release lock.
 */

ENTRY(spin_unlock, TAG_NO_FRAME_USED)

	li	r0,	0		/* set lock to free == 0 */
	cmpwi	r0,	0		/* Ensure r0 is zero on 601 for sync */
	bne+	0f
	sync				/* Flush writes done under lock */
0:
	stw	r0,	0(ARG0)
	blr
