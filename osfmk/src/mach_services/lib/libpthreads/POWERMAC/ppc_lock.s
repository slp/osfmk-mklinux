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
 * void _spin_lock(void *lock)
 */

ENTRY(_spin_lock, TAG_NO_FRAME_USED)
10:	lwarx	r4,0,r3		/* load and lock */
	cmpi	0,r4,0		/* see if already locked */
	bne	10b		/* yes - just 'spin' */
	li	r4,1		/* no - try to set lock */
	stwcx.	r4,0,r3		/* store new value iff still locked */
	bne	10b		/* try again if lost */
	blr

/*	
 * void _spin_unlock(void *lock)
 */

ENTRY(_spin_unlock, TAG_NO_FRAME_USED)
	li	r4,0		/* just clear it */
	stw	r4,0(r3)
	blr
