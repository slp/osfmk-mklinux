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
#ifndef __ELF__	
	.subspa $UNWIND_START$,QUAD=0,ALIGN=8,ACCESS=0x2c,SORT=56

#endif
	.subspa	$CODE$
		
	.proc
	.callinfo SAVE_SP,FRAME=128
	.export	__start_mach,entry
	.entry
__start_mach

#if 0
	copy %r0,%r1
$lab
	combt,=,n %r0,%r1,$lab
	nop
#endif

	/*
	 * double word align the stack and allocate a stack frame to start out
	 * with. (should be already done in bootstrap ? )
	 */
	ldo     128(sp),sp
	depi    0,31,3,sp

	.import __c_start_mach
	ldil	L%__c_start_mach,t1
	ldo	R%__c_start_mach(t1),t1

	ldil    L%$global$,dp	       # Initialize the global
	ldo     R%$global$(dp),dp      # data pointer
	
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
	
#ifndef __ELF__
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
#endif /* __ELF__ */
	
	.export	$global$,data
$global$
	
	.end

