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
 */
/*
 * MkLinux
 */

#include <hp_pa/asm.h>

	.space	$TEXT$
	.subspa	$CODE$

/*
 * int
 * get_dp()
 *
 * Return the contents of the dp register.
 */

	.export	get_dp,entry
	.proc
	.callinfo
get_dp
	bv	0(rp)
	or	r0,dp,ret0
	.procend

/*
 * int
 * spin_try_lock(s)
 * 	spin_lock_t *s;
 * 
 * Try to acquire a lock, return 1 if successful, 0 if not.
 */


	.export	spin_try_lock,entry
	.proc
	.callinfo

spin_try_lock
	/*
	 * align the lock to a 16 byte boundary
	 */
	ldo	15(arg0),arg0
	depi	0,31,4,arg0

	/*
	 * attempt to get the lock
	 */
	ldcws	0(arg0),ret0

	/*
	 * if ret0 is 0 then we didn't get the lock and we return 0,
	 * else we own the lock and we return 1.
	 */
	subi,=	0,ret0,r0
	ldi	1,ret0

	bv	0(rp)
	nop
	.procend


/*
 * void
 * simple_unlock(s)
 *	spin_lock_t *s;
 *
 * Release a lock.
 */
	.export	simple_unlock,entry
	.proc
	.callinfo

simple_unlock
	/*
	 * align the lock to a 16 byte boundary
	 */
	ldo	15(arg0),arg0
	depi	0,31,4,arg0

	/*
	 * clear the lock by setting it non-zero
	 */
	ldi	-1,t1
	bv	0(rp)
	stw	t1,0(arg0)

	.procend

	.end





