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

	.space	$TEXT$
	.subspa	$CODE$

/* 
 * task_by_pid (-1)
 */

	.export	mach_host_priv_self	

mach_host_priv_self
	.proc		
	.callinfo

	mtsp	%r0,%sr0

	ldil	l%-33,%r22		/* task_by_pid */
	ldo	r%-33(%r22),%r22
	ldil	l%-1,%r26
	ldo	r%-1(%r26),%r26

	ldil	L%0xC0000004,%r1
	ble	R%0xC0000004(%sr0,%r1)
	nop

	bv	0(%r2)
	nop

	.procend

	.end



