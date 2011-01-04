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
 * Copyright (c) 1990,1991 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: lock.s 1.6 92/07/08$
 *	Author: Bob Wheeler, University of Utah CSS
 */

#include <machine/asm.h>

	.space	$TEXT$
	.subspa $CODE$

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
 * spin_unlock(s)
 *	spin_lock_t *s;
 *
 * Release a lock.
 */
	.export	spin_unlock,entry
	.proc
	.callinfo 

spin_unlock
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







