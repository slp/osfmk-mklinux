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

#include <hp_pa/asm.h>

	.space	$TEXT$
	.subspa	$CODE$

/* void *_sp(void) */
	.export _sp,entry
	.proc
	.callinfo
_sp
	bv	(rp)
	copy	sp,ret0

	.procend

/* int _dp(void)
   This register is used to access global variables and must be propagated
   to new threads.  */
	.export _dp,entry
	.proc
	.callinfo
_dp
	bv	(rp)
	copy	dp,ret0

	.procend

/* vm_address_t _adjust_sp(vm_address_t)
   The stack is supposed to be aligned on a 16-byte boundary.  We make
   sure that there is a frame of at least 64 bytes, though 48 would
   really suffice.  (The function we're going to be calling is
   allowed to decide to store its argument at sp-36.)  */
	.export _adjust_sp,entry
	.proc
	.callinfo
_adjust_sp
	ldo	64+15(arg0),ret0
	bv	(rp)
	depi	0,31,4,ret0

	.procend

	.end
