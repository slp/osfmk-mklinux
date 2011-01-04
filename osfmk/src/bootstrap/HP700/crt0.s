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
	.subspa $UNWIND_START$,QUAD=0,ALIGN=8,ACCESS=0x2c,SORT=56
	.subspa	$CODE$

	.proc
	.callinfo SAVE_SP,FRAME=128
	.export	$START$,entry
	.entry
$START$

/*
	copy %r0,%r1
$lab
	combt,=,n %r0,%r1,$lab
	nop
*/

	/*
	 * double word align the stack and allocate a stack frame to start out
	 * with.
	 */
	ldo     128(sp),sp
	depi    0,31,3,sp

	/*
	 * Okay, get ready to call main (finally). First we have to reload
	 * the argument registers.
	 */

	.import _start
	ldil	L%_start,t1
	ldo	R%_start(t1),t1

	.call
	blr     r0,rp
	bv,n    (t1)
	nop

	/*
	 * never returns
	 */
	break	0,0

	.exit
	.procend

	.proc
	.callinfo
	.export	__main,entry
	.entry
__main
	bv	0(rp)
	nop
	.exit
	.procend

	.subspa $UNWIND_START$
	.export $UNWIND_START
$UNWIND_START
	.subspa	$UNWIND_END$,QUAD=0,ALIGN=8,ACCESS=0x2c,SORT=72
	.export	$UNWIND_END
$UNWIND_END

	.subspa	$RECOVER_START$,QUAD=0,ALIGN=4,ACCESS=0x2c,SORT=73
	.export $RECOVER_START
$RECOVER_START
	.subspa	$RECOVER$MILLICODE$,QUAD=0,ALIGN=4,ACCESS=0x2c,SORT=78
	.subspa	$RECOVER$
	.subspa	$RECOVER_END$,QUAD=0,ALIGN=4,ACCESS=0x2c,SORT=88
	.export	$RECOVER_END
$RECOVER_END
	.space	$PRIVATE$
        .SUBSPA $DATA$

	.subspa	$GLOBAL$
	.export	$global$
$global$

	.end

