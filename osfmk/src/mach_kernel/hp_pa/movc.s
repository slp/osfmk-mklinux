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
/*
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * Copyright (c) 1990,1991,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: movc.s 1.26 94/12/14$
 */

#include <mach_rt.h>	
#include <machine/asm.h>
#include <assym.s>
#include <machine/psw.h>
#include <hp_pa/pte.h>	
#include <machine/pmap.h>
#include <machine/cpu.h>
#include <mach/machine/vm_param.h>

	.space	$PRIVATE$
	.subspa	$DATA$

	.import cpu_data
	.import dcache_line_mask,data

	.space	$TEXT$
	.subspa $CODE$

#if MACH_RT	
	.export $movc_start
$movc_start
#endif
		
	.code
	.export memcpy,entry
memcpy
	copy	arg0,arg3
	copy	arg1,arg0
	copy	arg3,arg1
	/* And fall into....  */
/*
 * void 
 * bcopy(src, dst, count)
 *	vm_offset_t	src;
 *	vm_offset_t	dst;
 *	int		count;
 */

	.export	bcopy,entry
	.proc
	.callinfo 
bcopy
	.entry
        comb,>=,n r0,arg2,$bcopy_exit

	/*
	 * See if the source and destination are word aligned and if the count
	 * is an integer number of words. If so then we can use an optimized 
	 * routine. If not then branch to bcopy_checkalign and see what we can
	 * do there.
	 */

        or	arg0,arg1,t1
        or	t1,arg2,t2
        extru,= t2,31,2,r0
        b,n     $bcopy_checkalign

        addib,<,n -16,arg2,$bcopy_movewords

	/*
	 * We can move the data in 4 word moves. We'll use 4 registers to 
	 * avoid interlock and pipeline stalls.
	 */

$bcopy_loop16

        ldwm	16(arg0),t1
        ldw	-12(arg0),t2
        ldw     -8(arg0),t3
        ldw     -4(arg0),t4
        stwm    t1,16(arg1)
        stw     t2,-12(arg1)
        stw     t3,-8(arg1)
        addib,>= -16,arg2,$bcopy_loop16
        stw     t4,-4(arg1)


	/*
	 * We have already decremented the count by 16, add 12 to it and then 
	 * we can test if there is at least 1 word left to move.
	 */

$bcopy_movewords
        addib,<,n 12,arg2,$bcopy_exit

	/*
	 * Clean up any remaining words that were not moved in the 16 byte
	 * moves
	 */

$bcopy_loop4
        ldwm	4(arg0),t1
        addib,>= -4,arg2,$bcopy_loop4
        stwm    t1,4(arg1)

	bv,n 	0(r2)


$bcopy_checkalign

	/*
	 * The source or destination is not word aligned or the count is not 
	 * an integral number of words. If we are dealing with less than 16 
	 * bytes then just do it byte by byte. Otherwise, if the destination
	 * space is I/O space, handle copy separately, since stbys seems to
	 * not work properly on the I/O addr. Otherwise, see if the data has 
	 * the same basic alignment. We will add in the byte offset to size to
	 * keep track of what we have to move even though the stbys instruction
	 * won't physically move it. 
	 */

        comib,>=,n 15,arg2,$bcopy_byte
        extru arg1,3,4,t1
        comib,=,n 0xF,t1,$bcopy_io
        extru   arg0,31,2,t1
        extru   arg1,31,2,t2
        add     arg2,t2,arg2
        comb,<> t2,t1,$bcopy_unaligned
        dep     0,31,2,arg0

	/*
	 * the source and destination have the same basic alignment. We will 
	 * move the data in blocks of 16 bytes as long as we can and then 
	 * we'll go to the 4 byte moves.
	 */

        addib,<,n -16,arg2,$bcopy_aligned2

$bcopy_loop_aligned4
        ldwm	16(arg0),t1
        ldw     -12(arg0),t2
        ldw     -8(arg0),t3
        ldw     -4(arg0),t4
        stbys,b,m t1,4(arg1)
        stwm    t2,4(arg1)
        stwm    t3,4(arg1)
        addib,>= -16,arg2,$bcopy_loop_aligned4
        stwm    t4,4(arg1)

	/*
	 * see if there is anything left that needs to be moved in a word move.
	 * Since the count was decremented by 16, add 12 to test if there are 
	 * any full word moves left to do.
	 */

$bcopy_aligned2
        addib,<,n 12,arg2,$bcopy_cleanup

$bcopy_loop_aligned2
        ldws,ma	4(arg0),t1
        addib,>= -4,arg2,$bcopy_loop_aligned2
        stbys,b,m t1,4(arg1)

	/*
	 * move the last bytes that may be unaligned on a word boundary
	 */

$bcopy_cleanup
         addib,=,n 4,arg2,$bcopy_exit
         ldws	0(arg0),t1
         add    arg1,arg2,arg1
	 bv	0(r2)
         stbys,e t1,0(arg1)

	/*
	 * The source and destination are not aligned on the same boundary 
	 * types. We will have to shift the data around. Figure out the shift 
	 * amount and load it into cr11.
	 */

$bcopy_unaligned
        sub,>=	t2,t1,t3
        ldwm    4(arg0),t1
        zdep    t3,28,29,t4
        mtctl   t4,11

	/*
	 * see if we can do some of this work in blocks of 16 bytes
	 */

        addib,<,n -16,arg2,$bcopy_unaligned_words

$bcopy_unaligned4
        ldwm	16(arg0),t2
	ldw	-12(arg0),t3
	ldw	-8(arg0),t4
	ldw	-4(arg0),r1
        vshd	t1,t2,r28
        stbys,b,m r28,4(arg1)
        vshd	t2,t3,r28
        stwm	r28,4(arg1)
        vshd	t3,t4,r28
        stwm	r28,4(arg1)
        vshd	t4,r1,r28
        stwm   	r28,4(arg1)
        addib,>= -16,arg2,$bcopy_unaligned4
	copy	r1,t1

	/*
	 * see if there is a full word that we can transfer
	 */

$bcopy_unaligned_words
        addib,<,n 12,arg2,$bcopy_unaligned_cleanup1

$bcopy_unaligned_loop
        ldwm	4(arg0),t2
        vshd    t1,t2,t3
        addib,< -4,arg2,$bcopy_unaligned_cleanup2
        stbys,b,m t3,4(arg1)

        ldwm	4(arg0),t1
        vshd    t2,t1,t3
        addib,>= -4,arg2,$bcopy_unaligned_loop
        stbys,b,m t3,4(arg1)

$bcopy_unaligned_cleanup1
	copy	t1,t2

$bcopy_unaligned_cleanup2
	addib,<=,n 4,arg2,$bcopy_exit
        add	arg1,arg2,arg1
	mfctl	sar,t3
	extru	t3,28,2,t3
	sub,<=	arg2,t3,r0
        ldwm    4(arg0),t1
        vshd    t2,t1,t3
        bv      0(r2)
        stbys,e t3,0(arg1)
	
	/*
	 * Move data to I/O space
	 */
$bcopy_io
	/*
	 * See if the src and dst are similary aligned
	 */
	xor	arg0,arg1,t1
	extru,=	t1,31,2,r0
	b,n	$bcopy_io_unaligned

	/*
	 * Align dst on a halfword
	 */
	extru,<> arg1,31,1,r0
	b,n	$bcopy_io_aligned_2bytes
	ldbs,ma	1(arg0),t1
	addi	-1,arg2,arg2
	stbs,ma	t1,1(arg1)

$bcopy_io_aligned_2bytes
	/*
	 * Align dst on a word
	 */
	extru,<> arg1,31,2,r0
	b,n	$bcopy_io_aligned_4bytes
	ldhs,ma	2(arg0),t1
	addi	-2,arg2,arg2
	sths,ma	t1,2(arg1)

$bcopy_io_aligned_4bytes
	/*
	 * Do as many 16bytes copies as possible
	 */
        addib,<,n -16,arg2,$bcopy_io_aligned_words
$bcopy_io_aligned_16bytes
        ldwm	16(arg0),t1
        ldw     -12(arg0),t2
        ldw     -8(arg0),t3
        ldw     -4(arg0),t4
        stwm	t1,4(arg1)
        stwm    t2,4(arg1)
        stwm    t3,4(arg1)
        addib,>= -16,arg2,$bcopy_io_aligned_16bytes
        stwm    t4,4(arg1)
	
$bcopy_io_aligned_words
	/*
	 * Do as many 4bytes copies as possible
	 * N.B. Since the count was decremented by 16, add 12
	 *	to test if there are any full word moves left to do.
	 */
        addib,<,n 12,arg2,$bcopy_io_aligned_hword
$bcopy_io_aligned_words_loop
        ldws,ma	4(arg0),t1
        addib,>= -4,arg2,$bcopy_io_aligned_words_loop
        stwm	t1,4(arg1)

$bcopy_io_aligned_hword
	/*
	 * Do a final half word copy if needed
	 * N.B. Since the count was decremented by 4, add 2
	 *	to test if there are any half word move left to do.
	 */
        addib,<,n 2,arg2,$bcopy_io_aligned_bytes
	ldhs,ma	2(arg0),t1
	addi	-2,arg2,arg2
	sths,ma	t1,2(arg1)

$bcopy_io_aligned_bytes
	/*
	 * Do a final byte copy if needed
	 * N.B. Since the count was decremented by 2, add 1
	 *	to test if there are any byte move left to do.
	 */
	addib,<,n 1,arg2,$bcopy_exit
	ldbs,ma	1(arg0),t1
	addi	-1,arg2,arg2
	bv	0(r2)
	stbs,ma	t1,1(arg1)

$bcopy_io_unaligned
	/*
	 * The source and destination are not aligned on the same boundary 
	 * types. We will have to shift the data around.
	 */
        extru,<> arg1,31,2,t1
	b,n	$bcopy_io_unaligned_4bytes
	
	/*
	 * Align dst on a halfword
	 */
	extru,<> arg1,31,1,r0
	b,n	$bcopy_io_unaligned_2bytes
	ldbs,ma	1(arg0),t1
	addi	-1,arg2,arg2
	stbs,ma	t1,1(arg1)

$bcopy_io_unaligned_2bytes
	/*
	 * Align dst on a word
	 */
	extru,<> arg1,31,2,r0
	b,n	$bcopy_io_unaligned_4bytes
	ldbs,ma	1(arg0),t1
	ldbs,ma	1(arg0),t2
	addi	-2,arg2,arg2
	stbs,ma	t1,1(arg1)
	stbs,ma	t2,1(arg1)

$bcopy_io_unaligned_4bytes
	/*
	 * Compute shift value
	 */
        extru   arg0,31,2,t1
	sub	r0,t1,t1
	zdep	t1,28,29,t2
	mtctl	t2,11

	/*
	 * Load first src word to t1
	 */
	dep	r0,31,2,arg0
	addib,< -16,arg2,$bcopy_io_unaligned_words
	ldws,ma	4(arg0),t1

$bcopy_io_unaligned_16bytes
        ldwm	16(arg0),t2
        ldw     -12(arg0),t3
        ldw     -8(arg0),t4
        ldw     -4(arg0),r1
        vshd	t1,t2,r28
        stwm	r28,4(arg1)
        vshd	t2,t3,r28
        stwm    r28,4(arg1)
        vshd	t3,t4,r28
        stwm    r28,4(arg1)
        vshd	t4,r1,r28
        stwm    r28,4(arg1)
        addib,>= -16,arg2,$bcopy_io_unaligned_16bytes
        copy	r1,t1

$bcopy_io_unaligned_words
	/*
	 * Do as many 4bytes copies as possible
	 * N.B. Since the count was decremented by 16, add 12
	 *	to test if there are any full word moves left to do.
	 */
        addib,<,n 12,arg2,$bcopy_io_unaligned_words_exit

$bcopy_io_unaligned_words_loop
        ldwm	4(arg0),t2
        vshd    t1,t2,t3
        addib,< -4,arg2,$bcopy_io_unaligned_hword
        stwm	t3,4(arg1)

        ldwm	4(arg0),t1
        vshd    t2,t1,t3
        addib,>= -4,arg2,$bcopy_io_unaligned_words_loop
        stwm	t3,4(arg1)

$bcopy_io_unaligned_words_exit
	copy	t1,t2

$bcopy_io_unaligned_hword
	/*
	 * Prepare in t3 the final bytes to copy to arg1
	 */
	addib,<=,n 4,arg2,$bcopy_exit
	mfctl	11,t3
	extru	t3,28,2,t3
	sub,<=	arg2,t3,r0
        ldwm    4(arg0),t1
        vshd    t2,t1,t3
	
	/*
	 * Do a final half word copy if needed
	 */
	comibf,<=,n 2,arg2,$bcopy_io_unaligned_bytes
        extru	t3,15,16,t1
	addi	-2,arg2,arg2
	sths,ma	t1,2(arg1)
	shd	t3,t3,16,t3

$bcopy_io_unaligned_bytes
	/*
	 * Do a final byte copy if needed
	 */
	comibf,<=,n 1,arg2,$bcopy_exit
        extru	t3,7,8,t1
	addi	-1,arg2,arg2
	bv	0(r2)
	stbs,ma	t1,1(arg1)

	/*
	 * move data one byte at a time
	 */

$bcopy_byte
        comb,>=,n r0,arg2,$bcopy_exit

$bcopy_loop_byte
        ldbs,ma	1(arg0),t1
        addib,> -1,arg2,$bcopy_loop_byte
        stbs,ma t1,1(arg1) 

$bcopy_exit
	bv,n	0(r2)
	.exit
	.procend


/*
 * void 
 * bzero(dst, count)
 *	vm_offset_t	dst;
 *	int		count;
 */

	.export memset
memset
	copy arg2,arg1
	/* And fall into....  */

	.export	bzero,entry
	.proc
	.callinfo 
bzero
	.entry
        comb,>=,n r0,arg1,$bzero_exit

	/*
	 * If we need to clear less than a word do it a byte at a time
	 */

	comib,>,n 4,arg1,$bzero_bytes

	/*
	 * If the destination is in the I/O space handle bzero separately,
	 * since stbys seems to not work properly on I/O addr.
	 */
	
	extru	arg0,3,4,t1
	comib,=,n 0xF,t1,$bzero_io

	/*
	 * Since we are only clearing memory the alignment restrictions 
	 * are simplified. Figure out how many "extra" bytes we need to
	 * store with stbys.
	 */

        extru   arg0,31,2,t2
        add     arg1,t2,arg1

	/*
	 * We will zero the destination in blocks of 16 bytes as long as we 
	 * can and then we'll go to the 4 byte moves.
	 */

        addib,<,n -16,arg1,$bzero_word

$bzero_loop_16
        stbys,b,m r0,4(arg0)
        stwm    r0,4(arg0)
        stwm    r0,4(arg0)
        addib,>= -16,arg1,$bzero_loop_16
        stwm    r0,4(arg0)

	/*
	 * see if there is anything left that needs to be zeroed in a word 
	 * move. Since the count was decremented by 16, add 12 to test if 
	 * there are any full word moves left to do.
	 */

$bzero_word
        addib,<,n 12,arg1,$bzero_cleanup

$bzero_loop_4
        addib,>= -4,arg1,$bzero_loop_4
        stbys,b,m r0,4(arg0)

	/*
	 * zero the last bytes that may be unaligned on a word boundary
	 */

$bzero_cleanup
        addib,=,n 4,arg1,$bzero_exit
        add	arg0,arg1,arg0
        bv      0(r2)
        stbys,e r0,0(arg0)

	/*
	 * Zero data in I/O space
	 */
$bzero_io
	/*
	 * Align dst on a halfword
	 */
	extru,<> arg0,31,1,r0
	b,n	$bzero_io_2bytes
	addi	-1,arg1,arg1
	stbs,ma	r0,1(arg0)

$bzero_io_2bytes
	/*
	 * Align dst on a word
	 */
	extru,<> arg0,31,2,r0
	b,n	$bzero_io_4bytes
	addi	-2,arg1,arg1
	sths,ma	r0,2(arg0)

$bzero_io_4bytes
	/*
	 * Do as many 16bytes copies as possible
	 */
        addib,<,n -16,arg1,$bzero_io_words
$bzero_io_16bytes
        stwm	r0,4(arg0)
        stwm    r0,4(arg0)
        stwm    r0,4(arg0)
        addib,>= -16,arg1,$bzero_io_16bytes
        stwm    r0,4(arg0)
	
$bzero_io_words
	/*
	 * Do as many 4bytes copies as possible
	 * N.B. Since the count was decremented by 16, add 12
	 *	to test if there are any full word moves left to do.
	 */
        addib,<,n 12,arg1,$bzero_io_hword
$bzero_io_words_loop
        addib,>= -4,arg1,$bzero_io_words_loop
        stwm	r0,4(arg0)

$bzero_io_hword
	/*
	 * Do a final half word copy if needed
	 * N.B. Since the count was decremented by 4, add 2
	 *	to test if there are any half word move left to do.
	 */
        addib,<,n 2,arg1,$bzero_io_bytes
	addi	-2,arg1,arg1
	sths,ma	r0,2(arg0)

$bzero_io_bytes
	/*
	 * Do a final byte copy if needed
	 * N.B. Since the count was decremented by 2, add 1
	 *	to test if there are any byte move left to do.
	 */
	addib,<,n 1,arg1,$bzero_exit
	addi	-1,arg1,arg1
	bv	0(r2)
	stbs,ma	r0,1(arg0)

	/*
	 * zero by bytes
	 */

$bzero_bytes
        addib,> -1,arg1,$bzero_bytes
        stbs,ma r0,1(arg0) 

$bzero_exit
	bv,n	0(r2)
	.exit
	.procend


/*
 * int
 * bcmp(src, dst, count)
 *	vm_offset_t	src;
 *	vm_offset_t	dst;
 *	int		count;
 */

	.export memcmp
memcmp
	/* fall into....  */

	.export	bcmp,entry
	.proc
	.callinfo 
bcmp
	.entry
        comb,>= r0,arg2,$bcmp_exit
	copy	t1,t2

$bcmp_loop
	ldbs,ma	1(arg0),t1
	ldbs,ma	1(arg1),t2
	comb,<>,n t1,t2,$bcmp_exit
	addib,<> -1,arg2,$bcmp_loop
	nop

$bcmp_exit
	bv	0(r2)
	sub	t1,t2,ret0

	.exit
	.procend


/*
 * int
 * copyin(src, dst, count)
 *	vm_offset_t	src;
 *	vm_offset_t	dst;
 *	int		count;
 */

	.export	copyin,entry
	.export	copyinmsg,entry
	.proc
	.callinfo 
copyin
copyinmsg

	.entry

	/*
	 * get the current thread pointer...
	 */

        ldil    L%cpu_data,t1
        ldw     R%cpu_data(t1),r29

	/*
	 * If zero bytes requested then exit success
	 */

        comb,=  r0,arg2,$copyin_exit
	ldi	0,ret0

	/*
	 * if negative number of bytes requested then error
	 */

        comb,>  r0,arg2,$copyin_exit
	ldi	-1,ret0	

	/*
	 * get the task pointer and finally the map pointer...
	 *
	 * Set up thread recovery (interleaved to reduce stalls) in
	 * case we touch some bad user memory.
	 */

	ldil	L%$copyin_exit,t2
	ldw	THREAD_TOP_ACT(r29),t1
	ldo	R%$copyin_exit(t2),t2
	ldw	ACT_MAP(t1),t1
	stw	t2,THREAD_RECOVER(r29)

	/*
	 * Then get the pmap pointer and finally get the space
	 *
	 * Use "or"s from alignment check to reduce load/use stalls.
	 */

	ldw	VMMAP_PMAP(t1),t1
        or	arg0,arg1,t2
        or	t2,arg2,t2

	/*
	 * set up the space register sr1 to point to user space
	 */

	ldw	PMAP_SPACE(t1),t4
	mtsp	t4,sr1
	ldw	PMAP_PID(t1),t4
	mtctl	t4,pidr1

	/*
	 * See if the source and destination are word aligned and if the count
	 * is an integer number of words. If so then we can use an optimized 
	 * routine. If not then branch to bcopy_checkalign and see what we can
	 * do there.
	 */

        extru,= t2,31,2,r0
        b,n     $copyin_checkalign

        addib,<,n -16,arg2,$copyin_movewords

	/*
	 * We can move the data in 4 word moves. We'll use 4 registers to
	 * avoid interlock and pipeline stalls.
	 */

$copyin_loop16

        ldwm	16(sr1,arg0),t1
        ldw	-12(sr1,arg0),t2
        ldw     -8(sr1,arg0),t3
        ldw     -4(sr1,arg0),t4
        stwm    t1,16(arg1)
        stw     t2,-12(arg1)
        stw     t3,-8(arg1)
        addib,>= -16,arg2,$copyin_loop16
        stw     t4,-4(arg1)


	/*
	 * We have already decremented the count by 16, add 12 to it and then 
	 * we can test if there is at least 1 word left to move.
	 */

$copyin_movewords
        addib,<,n 12,arg2,$copyin_done

	/*
	 * Clean up any remaining words that were not moved in the 16 byte
	 * moves
	 */

$copyin_loop4
        ldwm	4(sr1,arg0),t1
        addib,>= -4,arg2,$copyin_loop4
        stwm    t1,4(arg1)

	b	$copyin_exit
	copy	r0,ret0

$copyin_checkalign

	/*
	 * The source or destination is not word aligned or the count is not 
	 * an integral number of words. If we are dealing with less than 16 
	 * bytes then just do it byte by byte. Otherwise, see if the data has 
	 * the same basic alignment. We will add in the byte offset to size to
	 * keep track of what we have to move even though the stbys instruction
	 * won't physically move it. 
	 */

        comib,>= 15,arg2,$copyin_byte
        extru   arg0,31,2,t1
        extru   arg1,31,2,t2
        add     arg2,t2,arg2
        comb,<> t2,t1,$copyin_unaligned
        dep     0,31,2,arg0

	/*
	 * the source and destination have the same basic alignment. We will 
	 * move the data in blocks of 16 bytes as long as we can and then 
	 * we'll go to the 4 byte moves.
	 */

        addib,<,n -16,arg2,$copyin_aligned2

$copyin_loop_aligned4
        ldwm	16(sr1,arg0),t1
        ldw     -12(sr1,arg0),t2
        ldw     -8(sr1,arg0),t3
        ldw     -4(sr1,arg0),t4
        stbys,b,m t1,4(arg1)
        stwm    t2,4(arg1)
        stwm    t3,4(arg1)
        addib,>= -16,arg2,$copyin_loop_aligned4
        stwm    t4,4(arg1)

	/*
	 * see if there is anything left that needs to be moved in a word move.
	 * Since the count was decremented by 16, add 12 to test if there are 
	 * any full word moves left to do.
	 */

$copyin_aligned2
        addib,<,n 12,arg2,$copyin_cleanup

$copyin_loop_aligned2
        ldws,ma	4(sr1,arg0),t1
        addib,>= -4,arg2,$copyin_loop_aligned2
        stbys,b,m t1,4(arg1)

	/*
	 * move the last bytes that may be unaligned on a word boundary
	 */

$copyin_cleanup
	addib,=,n 4,arg2,$copyin_done
	ldws	0(sr1,arg0),t1
	add    arg1,arg2,arg1
	stbys,e t1,0(arg1)
	
	b $copyin_exit
	copy	r0,ret0

	/*
	 * The source and destination are not alligned on the same boundary 
	 * types. We will have to shift the data around. Figure out the shift 
	 * amount and load it into cr11.
	 */

$copyin_unaligned
        sub,>=	t2,t1,t3
        ldwm    4(sr1,arg0),t1
        zdep    t3,28,29,t4
        mtctl   t4,11

	/*
	 * see if we can do some of this work in blocks of 16 bytes
	 */

        addib,<,n -16,arg2,$copyin_unaligned_words

$copyin_unaligned4
        ldwm	16(sr1,arg0),t2
	ldw	-12(sr1,arg0),t3
	ldw	-8(sr1,arg0),t4
	ldw	-4(sr1,arg0),r1
        vshd	t1,t2,r28
        stbys,b,m r28,4(arg1)
        vshd	t2,t3,r28
        stwm	r28,4(arg1)
        vshd	t3,t4,r28
        stwm	r28,4(arg1)
        vshd	t4,r1,r28
        stwm   	r28,4(arg1)
        addib,>= -16,arg2,$copyin_unaligned4
	copy	r1,t1

	/*
	 * see if there is a full word that we can transfer
	 */

$copyin_unaligned_words
        addib,<,n 12,arg2,$copyin_unaligned_cleanup1

$copyin_unaligned_loop
        ldwm	4(sr1,arg0),t2
        vshd    t1,t2,t3
        addib,< -4,arg2,$copyin_unaligned_cleanup2
        stbys,b,m t3,4(arg1)

        ldwm	4(sr1,arg0),t1
        vshd    t2,t1,t3
        addib,>= -4,arg2,$copyin_unaligned_loop
        stbys,b,m t3,4(arg1)

$copyin_unaligned_cleanup1
	copy	t1,t2

$copyin_unaligned_cleanup2
	addib,<=,n 4,arg2,$copyin_done
        add	arg1,arg2,arg1
	mfctl	sar,t3
	extru	t3,28,2,t3
	sub,<=	arg2,t3,r0
        ldwm    4(sr1,arg0),t1
        vshd    t2,t1,t3
        stbys,e t3,0(arg1)

	b	$copyin_exit
	copy	r0,ret0

	/*
	 * move data one byte at a time
	 */

$copyin_byte
        comb,>=,n r0,arg2,$copyin_done

$copyin_loop_byte
        ldbs,ma	1(sr1,arg0),t1
        addib,> -1,arg2,$copyin_loop_byte
        stbs,ma t1,1(arg1) 

$copyin_done
	copy	r0,ret0

$copyin_exit
	bv	0(r2)
	stw	r0,THREAD_RECOVER(r29)

	.exit
	.procend


/*
 * void 
 * copyout(src, dst, count)
 *	vm_offset_t	src;
 *	vm_offset_t	dst;
 *	int		count;
 */

	.export	copyout,entry
	.export	copyoutmsg,entry
	.proc
	.callinfo 
copyout
copyoutmsg

	.entry

	/*
	 * get the current thread pointer...
	 */

        ldil    L%cpu_data,t1
        ldw     R%cpu_data(t1),r29

	/*
	 * If zero bytes requested then exit success
	 */

        comb,=  r0,arg2,$copyout_exit
	copy	r0,ret0

	/*
	 * if negative number of bytes requested then error
	 */

        comb,>  r0,arg2,$copyout_exit
	ldi	-1,ret0
	
	/*
	 * get the task pointer and finally the map pointer...
	 *
	 * set up the thread recovery (interleaved to avoid stalls) in
	 * case we touch some bad user memory.
	 */

	ldil	L%$copyout_exit,t2
	ldw	THREAD_TOP_ACT(r29),t1
	ldo	R%$copyout_exit(t2),t2
	ldw	ACT_MAP(t1),t1
	stw	t2,THREAD_RECOVER(r29)

	/*
	 * Then get the pmap pointer and finally get the space
	 *
	 * Use "or"s from alignment check to fill in load/use stall slots.
	 */

	ldw	VMMAP_PMAP(t1),t1
        or	arg0,arg1,t2
        or	t2,arg2,t2

	/*
	 * set up the space register sr1 to point to user space
	 */

	ldw	PMAP_SPACE(t1),t4
	mtsp	t4,sr1
	ldw	PMAP_PID(t1),t4
	mtctl	t4,pidr1

	/*
	 * See if the source and destination are word aligned and if the count
	 * is an integer number of words. If so then we can use an optimized 
	 * routine. If not then branch to bcopy_checkalign and see what we can
	 * do there.
	 */

        extru,= t2,31,2,r0
        b,n     $copyout_checkalign

        addib,<,n -16,arg2,$copyout_movewords

	/*
	 * We can move the data in 4 word moves. We'll use 4 registers to
	 * avoid interlock and pipeline stalls.
	 */

$copyout_loop16

        ldwm	16(arg0),t1
        ldw	-12(arg0),t2
        ldw     -8(arg0),t3
        ldw     -4(arg0),t4
        stwm    t1,16(sr1,arg1)
        stw     t2,-12(sr1,arg1)
        stw     t3,-8(sr1,arg1)
        addib,>= -16,arg2,$copyout_loop16
        stw     t4,-4(sr1,arg1)


	/*
	 * We have already decremented the count by 16, add 12 to it and then 
	 * we can test if there is at least 1 word left to move.
	 */

$copyout_movewords
        addib,<,n 12,arg2,$copyout_done

	/*
	 * Clean up any remaining words that were not moved in the 16 byte
	 * moves
	 */

$copyout_loop4
        ldwm	4(arg0),t1
        addib,>= -4,arg2,$copyout_loop4
        stwm    t1,4(sr1,arg1)

	b	$copyout_exit
	copy	r0,ret0

$copyout_checkalign

	/*
	 * The source or destination is not word aligned or the count is not 
	 * an integral number of words. If we are dealing with less than 16 
	 * bytes then just do it byte by byte. Otherwise, see if the data has 
	 * the same basic alignment. We will add in the byte offset to size to
	 * keep track of what we have to move even though the stbys instruction
	 * won't physically move it. 
	 */

        comib,>= 15,arg2,$copyout_byte
        extru   arg0,31,2,t1
        extru   arg1,31,2,t2
        add     arg2,t2,arg2
        comb,<> t2,t1,$copyout_unaligned
        dep     0,31,2,arg0

	/*
	 * the source and destination have the same basic alignment. We will 
	 * move the data in blocks of 16 bytes as long as we can and then 
	 * we'll go to the 4 byte moves.
	 */

        addib,<,n -16,arg2,$copyout_aligned2

$copyout_loop_aligned4
        ldwm	16(arg0),t1
        ldw     -12(arg0),t2
        ldw     -8(arg0),t3
        ldw     -4(arg0),t4
        stbys,b,m t1,4(sr1,arg1)
        stwm    t2,4(sr1,arg1)
        stwm    t3,4(sr1,arg1)
        addib,>= -16,arg2,$copyout_loop_aligned4
        stwm    t4,4(sr1,arg1)

	/*
	 * see if there is anything left that needs to be moved in a word move.
	 * Since the count was decremented by 16, add 12 to test if there are 
	 * any full word moves left to do.
	 */

$copyout_aligned2
        addib,<,n 12,arg2,$copyout_cleanup

$copyout_loop_aligned2
        ldws,ma	4(arg0),t1
        addib,>= -4,arg2,$copyout_loop_aligned2
        stbys,b,m t1,4(sr1,arg1)

	/*
	 * move the last bytes that may be unaligned on a word boundary
	 */

$copyout_cleanup
	addib,=,n 4,arg2,$copyout_done
	ldws	0(arg0),t1
	add    arg1,arg2,arg1
	stbys,e t1,0(sr1,arg1)

	b	$copyout_exit
	copy	r0,ret0

	/*
	 * The source and destination are not alligned on the same boundary 
	 * types. We will have to shift the data around. Figure out the shift 
	 * amount and load it into cr11.
	 */

$copyout_unaligned
        sub,>=	t2,t1,t3
        ldwm    4(arg0),t1
        zdep    t3,28,29,t4
        mtctl   t4,11

	/*
	 * see if we can do some of this work in blocks of 16 bytes
	 */

        addib,<,n -16,arg2,$copyout_unaligned_words

$copyout_unaligned4
        ldwm	16(arg0),t2
	ldw	-12(arg0),t3
	ldw	-8(arg0),t4
	ldw	-4(arg0),r1
        vshd	t1,t2,r28
        stbys,b,m r28,4(sr1,arg1)
        vshd	t2,t3,r28
        stwm	r28,4(sr1,arg1)
        vshd	t3,t4,r28
        stwm	r28,4(sr1,arg1)
        vshd	t4,r1,r28
        stwm   	r28,4(sr1,arg1)
        addib,>= -16,arg2,$copyout_unaligned4
	copy	r1,t1

	/*
	 * see if there is a full word that we can transfer
	 */

$copyout_unaligned_words
        addib,<,n 12,arg2,$copyout_unaligned_cleanup1

$copyout_unaligned_loop
        ldwm	4(arg0),t2
        vshd    t1,t2,t3
        addib,< -4,arg2,$copyout_unaligned_cleanup2
        stbys,b,m t3,4(sr1,arg1)

        ldwm	4(arg0),t1
        vshd    t2,t1,t3
        addib,>= -4,arg2,$copyout_unaligned_loop
        stbys,b,m t3,4(sr1,arg1)

$copyout_unaligned_cleanup1
	copy	t1,t2

$copyout_unaligned_cleanup2
	addib,<=,n 4,arg2,$copyout_done
        add	arg1,arg2,arg1
	mfctl	sar,t3
	extru	t3,28,2,t3
	sub,<=	arg2,t3,r0
        ldwm    4(arg0),t1
        vshd    t2,t1,t3
        stbys,e t3,0(sr1,arg1)
	
	b	$copyout_exit
	copy	r0,ret0

	/*
	 * move data one byte at a time
	 */

$copyout_byte
        comb,>=,n r0,arg2,$copyout_done

$copyout_loop_byte
        ldbs,ma	1(arg0),t1
        addib,> -1,arg2,$copyout_loop_byte
        stbs,ma t1,1(sr1,arg1) 

$copyout_done
	copy	r0,ret0

$copyout_exit
	bv	0(r2)
	stw	r0,THREAD_RECOVER(r29)
	.exit
	.procend


/* void
 * phys_page_copy(src, dst)
 * 	vm_offset_t	src;
 *	vm_offset_t	dst;
 *
 * This routine will copy a single page from physical address src to physical
 * address dst. 
 *
 * NB: All virtual addresses pointing to these pages should be flushed before
 * this call to avoid cache inconsistency.
 *
 * NB: This routine is optimized for 32 byte cache lines.
 * 
 */

	.export	phys_page_copy,entry
	.proc
	.callinfo 

phys_page_copy
	.entry
	ldi	-4,t2
	ldi	-CACHE_LINE_SIZE,t3
	ldi	HP700_PGBYTES-CACHE_LINE_SIZE,t4

	/*
	 * turn off all interrupts while the cache is in an inconsistent state
	 */

	rsm	PSW_I,t1

	/*
	 * move 4 words from a physical address to physical address
	 */
#define BT1 r31
#define BT2 arg3
#define BT3 ret0
#define BT4 ret1
#define BT5 arg2

$phys_bcopy_loop
	ldwas,ma 4(arg0),BT1
	ldwas,ma 4(arg0),BT2
	ldwas,ma 4(arg0),BT3
	ldwas,ma 4(arg0),BT4
	ldwas,ma 4(arg0),BT5
	stwas,ma BT1,4(arg1)
	stwas,ma BT2,4(arg1)
	stwas,ma BT3,4(arg1)
	stwas,ma BT4,4(arg1)
	stwas,ma BT5,4(arg1)
	ldwas,ma 4(arg0),BT1
	ldwas,ma 4(arg0),BT2
	ldwas,ma 4(arg0),BT3
	stwas,ma BT1,4(arg1)
	stwas,ma BT2,4(arg1)
	stwas,ma BT3,4(arg1)

	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	t2(arg0)
	fdc	t2(arg1)
	addb,>= t3,t4,$phys_bcopy_loop
	ssm	PSW_D,r0

#undef BT1
#undef BT2
#undef BT3
#undef BT4
#undef BT5

	sync
	bv	0(r2)
	mtsm	t1
	.exit
	.procend

/* void
 * phys_bzero(dst, bytecount)
 *	vm_offset_t	dst;
 *	int		bytecount
 *
 * This routine will zero bytecount bytes at physical address dst.
 *
 * NB: All virtual addresses pointing to the page should be flushed before
 * this call to avoid cache inconsistency. 
 * 
 * This routine assumes that dst is page aligned and that bytecount is 
 * divisible by 4.
 */

	.export	phys_bzero,entry
	.proc
	.callinfo 

phys_bzero
	.entry
        comb,>=,n r0,arg1,$phys_bzero_exit

	ldil	L%dcache_line_mask,t1
	ldw	R%dcache_line_mask(t1),t3
	ldi	-4,t2

$phys_bzero_loop1

	/*
	 * turn off all interrupts while the cache is in an inconsistent state
	 */

	rsm	PSW_I,t1

	/*
	 * clear a word with a physical address
	 */

$phys_bzero_loop2
	addib,<= -4,arg1,$phys_bzero_finish
	stwas,ma r0,4(arg0)

	/*
	 * check if the cache lines needs to be flushed
	 */

$phys_bzero_continue
	and,=	arg0,t3,r0
	b,n	$phys_bzero_loop2

	/*
	 * need to flush the cache, turn off data translation so that we can 
	 * flush a physical address from the cache. We want to flush out this 
	 * physical addressed cache line so that it can't bite us later. This
	 * way only virtual addresses will be in the cache. Also restore 
	 * interrupts momentarily since the cache is (for a second) consistent.
	 */

	rsm	PSW_D,r0
	fdc	t2(arg0)
	b	$phys_bzero_loop1
	mtsm	t1

	/*
	 * flush the last physical addresses from the cache
	 */

$phys_bzero_finish
	rsm	PSW_D,r0
	fdc	t2(arg0)
	mtsm	t1

	sync

$phys_bzero_exit
	bv,n	0(r2)
	.exit
	.procend

/* void
 * lpage_zero(tlbpage, space, va)
 *	int		tlbpage;
 *	vm_offset_t	va;
 *	space_t		space;
 *
 * This routine will zero a *single* page using a virtual address. 
 *
 * NB: This routine is optimized for 32 byte cache lines.
 */
	.export	lpage_zero,entry
	.proc
	.callinfo

lpage_zero
	.entry
	mtsp	arg1,sr1

	/*
	 * Create a TLB mapping that allows the kernel to access the
	 * virtual address. Mark it dirty to avoid the dirty trap.
	 */
	ldil	L%TLB_AR_KRW,arg3
	depi	1,TLB_DIRTY_POS,1,arg3

	/*
	 * Turn off all interrupts to prevent someone else from trying to
	 * touch the page in question.
	 */
	rsm	PSW_I,t1

	/*
	 * Enter the TLB mapping.
	 */
	idtlba  arg0,(sr1,arg2)
	idtlbp  arg3,(sr1,arg2)

	/*
	 * Now zero the page.
	 */
	ldi	HP700_PGBYTES-CACHE_LINE_SIZE,arg3
	copy	arg2,arg0
	ldi	-CACHE_LINE_SIZE,t2

$lpage_zero_loop
	stws,ma	r0,4(sr1,arg0)
	stws,ma	r0,4(sr1,arg0)
	stws,ma	r0,4(sr1,arg0)
	stws,ma	r0,4(sr1,arg0)
	stws,ma	r0,4(sr1,arg0)
	stws,ma	r0,4(sr1,arg0)
	stws,ma	r0,4(sr1,arg0)
        addb,>= t2,arg3,$lpage_zero_loop
	stws,ma	r0,4(sr1,arg0)

	/*
	 * Purge the data TLB now that we are finished with it, and turn
	 * interrupts back on.
	 */
	pdtlb	(sr1,arg2)
	bv	0(r2)
	mtsm	t1
	.exit
	.procend


/* void
 * lpage_copy(s_tlbpage, s_space, s_offset, dst)
 *	int		s_tlbpage;
 *	space_t		s_space;
 *	vm_offset_t	s_offset;
 *	vm_offset_t	dst;
 *
 * This routine will copy a virtual page to a physical page.
 *
 * WARNING: This routine is optimized for 32 byte cache lines. It will not
 * work on 16 byte lines, and will be non-optimal on 64 byte lines. There
 * is a check in the startup code that prints a warning for these cases.
 *
 */

	.export	lpage_copy,entry
	.proc
	.callinfo 

lpage_copy
	.entry
	ldi	-4,t2
	ldi	-CACHE_LINE_SIZE,t3
	mtsp	arg1,sr1

	/*
	 * Create a TLB mapping that allows the kernel to access the
	 * virtual address. Mark it dirty to avoid the dirty trap.
	 */
	ldil	L%TLB_AR_KRW,t4
	depi	1,TLB_DIRTY_POS,1,t4

	/*
	 * turn off all interrupts so no one can bother us.
	 */
	rsm	PSW_I,t1

	/*
	 * Enter the TLB mapping (purging any existing mapping first).
	 */
	pdtlb	(sr1,arg2)
	idtlba  arg0,(sr1,arg2)
	idtlbp  t4,(sr1,arg2)

	copy	arg2,arg0
	ldi	HP700_PGBYTES-CACHE_LINE_SIZE,arg1

#define BT1 r31
#define BT2 r1
#define BT3 ret0
#define BT4 ret1
#define BT5 t4
	/*
	 * move 32 bytes from a virtual address to physical address in each
	 * loop iteration. 
	 */
$lpage_copy_loop
	ldwm	4(sr1,arg0),BT1
	ldwm	4(sr1,arg0),BT2
	ldwm	4(sr1,arg0),BT3
	ldwm	4(sr1,arg0),BT4
	ldwm	4(sr1,arg0),BT5
	stwas,ma BT1,4(arg3)
	stwas,ma BT2,4(arg3)
	stwas,ma BT3,4(arg3)
	stwas,ma BT4,4(arg3)
	stwas,ma BT5,4(arg3)
	ldwm	4(sr1,arg0),BT1
	ldwm	4(sr1,arg0),BT2
	ldwm	4(sr1,arg0),BT3
	stwas,ma BT1,4(arg3)
	stwas,ma BT2,4(arg3)
	stwas,ma BT3,4(arg3)

	/*
	 * Since the destination is physical, flush it back to memory. Turn
	 * off data translation to do this.
	 */
	rsm	PSW_D,r0
	fdc	t2(arg3)
	addb,>=  t3,arg1,$lpage_copy_loop
	ssm	PSW_D,r0

#undef BT1
#undef BT2
#undef BT3
#undef BT4
#undef BT5
	/*
	 * Purge the data TLB now that we are finished with it, and turn
	 * interrupts back on.
	 */
	pdtlb	(sr1,arg2)
	mtsm	t1
	bv	0(r2)
	sync
	.exit
	.procend

#if MACH_RT	
	.export $movc_end
$movc_end
#endif	
	.end

/*
 * void 
 * bzero_phys(dst, count)
 *      vm_offset_t     dst;
 *      int             count;
 */

	.export	bzero_phys
	.proc
	.callinfo 
bzero_phys
	.entry

	/*
	 * zero data one byte at a time
	 */

$bzero_phys_byte
        comb,>=,n r0,arg1,$bzero_phys_exit
	ldi	-1,t2

$bzero_phys_loop_byte
	rsm	PSW_D,r0
        stbs,ma r0,1(arg0)
	fdc	t2(arg0)	 /* flush data cache line for dst */
        addib,> -1,arg1,$bzero_phys_loop_byte
	ssm	PSW_D,r0
	
$bzero_phys_exit
	sync
	bv	0(r2)
	nop
	.exit
	.procend

/*
 * void 
 * bcopy_phys(src, dst, count)
 *      vm_offset_t     src;
 *      vm_offset_t     dst;
 *      int             count;
 */

	.export	bcopy_phys
	.proc
	.callinfo 
bcopy_phys
	.entry
        comb,>=,n r0,arg2,$bcopy_phys_exit

	/*
	 * See if the source and destination are word aligned and if the count
	 * is an integer number of words. If so then we can use an optimized 
	 * routine. If not then branch to bcopy_phys_checkalign and see what we can
	 * do there.
	 */

	ldi 16,arg3		/* keep 16 in arg3 for further use */
	ldi -16,ret0		/* keep -16 in ret0 for further use */
	ldi -4,r1		/* keep -4 in r1 for further use */

        or	arg0,arg1,t1
        or	t1,arg2,t2
        extru,= t2,31,2,r0
        b,n     $bcopy_phys_checkalign
	
	comb,>,n arg3,arg2,$bcopy_phys_movewords

	/*
	 * We can move the data in 4 word moves. We'll use 4 registers to 
	 * avoid interlock and pipeline stalls.
	 */
$bcopy_phys_loop16

        ldwas,ma	4(arg0),t1
        ldwas,ma	4(arg0),t2
        ldwas,ma	4(arg0),t3
        ldwas,ma	4(arg0),t4
        stwas,ma	t1,4(arg1)
        stwas,ma	t2,4(arg1)
        stwas,ma	t3,4(arg1)
        stwas,ma	t4,4(arg1)
        addi		-16,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	ret0(arg0)
	fdc	ret0(arg1)
	comb,<= arg3,arg2,$bcopy_phys_loop16
	ssm	PSW_D,r0

	/*
	 * Test if there is at least 1 word left to move.
	 */

$bcopy_phys_movewords
        comib,>=,n 0,arg2,$bcopy_phys_exit

	/*
	 * Clean up any remaining words that were not moved in the 16 byte
	 * moves
	 */
$bcopy_phys_loop4
        ldwas,ma	4(arg0),t1
        stwas,ma	t1,4(arg1)
        addi		 -4,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	fdc	r1(arg1)
	comib,<= 4,arg2,$bcopy_phys_loop4
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_phys_exit 

$bcopy_phys_checkalign
	/*
	 * The source or destination is not word aligned or the count is not 
	 * an integral number of words.
	 */
	/*
	 * Check if less than 4 bytes to copy and handle it.
	 */
	comib,<,n 4,arg2,$bcopy_phys_long

$bcopy_phys_short
	/*
	 * Mask possible bytes of the first word of src before the 
	 * first byte to copy
	 */

	/* Align src on a word boundary */
	extru arg0,29,30,r31
	zdep r31,29,30,r31

	/* Load the word containing the first byte to copy */
	ldwas 0(r31),r1

	/* Purge first word of src */
	rsm	PSW_D,r0
	pdc	r0(r31)
	ssm	PSW_D,r0

	/* Check first if src is already word aligned */
	extru arg0,31,2,t3	/* src byte alignment */
	comib,=,n 0,t3,$bcopy_phys_short_src_aligned

	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	vshd r1,r0,r1

	/* Align the bytes to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd r0,r1,r1	/* r1: the first word to copy in dst */

$bcopy_phys_short_src_aligned
	/*
	 * Check if there is a second word from src to copy
	 */
	subi 4,t3,t2
	comb,<=,n arg2,t2,$bcopy_phys_short_one_src

$bcopy_phys_short_two_src
	/*
	 * There is a second word to copy from src. Mask possible bytes
	 * at end of second src word that must not be copied.
	 */

	/* Load the second word of src to copy */
	ldwas,mb 4(r31),t2

	/* Purge second word of src */
	rsm	PSW_D,r0
	pdc	r0(r31)
	ssm	PSW_D,r0

	subi 4,t3,t4
	sub arg2,t4,t4		/* Number of bytes to copy from second src word */

	/* Check first if all the word has to be copied */
	comib,=,n 4,t4,$bcopy_phys_short_mask_dst

	/* Compute bit shift */	
	subi 4,t4,t3
	sh3add t3,r0,t3
	mtctl t3,11

	vshd r0,t2,t2
	
	/* Align the bytes to copy on their original position */
	sh3add t4,r0,t4
	mtctl t4,11
	vshd t2,r0,r31		/* r31: the second word to copy in dst */

	b,n $bcopy_phys_short_mask_dst

$bcopy_phys_short_one_src
	/*
	 * Just one word to copy from src. Mask the possible bytes at end
	 * of the src word that must not ne copied
	 */
	ldi 0,r31		/* r31: zero as no second word to copy */

	/* Compute bit shift */
	add t3,arg2,t2

	/* Check first if rightmost byte has to be copied */
	comib,=,n 4,t2,$bcopy_phys_short_mask_dst

	subi 4,t2,t4
	sh3add t4,r0,t4
	mtctl t4,11

	vshd r0,r1,r1

	/* Align the bytes to copy on their original position */
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r1,r0,r1		/* r1: the first word to copy in dst */

$bcopy_phys_short_mask_dst
	/*
	 * Prepare dst to be merged with src word prepared before.
	 * It may contain bytes that must not be modified by the copy 
	 * operation at the begining or at the end.
	 */
	/*
	 * Check if there is a second word from dst to replace.
	 */
	extru arg1,31,2,t3
	subi 4,t3,t2
	comb,<=,n arg2,t2,$bcopy_phys_short_one_dst

$bcopy_phys_short_two_dst	
	/*
	 * There is a second word to replace in dst. Set the bytes to
	 * replace at end of first word and at begining of second
	 * word to zero for merge with src.
	 */
	/* Check first if dst is already word aligned */
	comib,=,n 0,t3,$bcopy_phys_short_dst1_aligned

	/* Compute shift depending on byte alignment of dst */
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,t1
	zdep t1,29,30,t1

	/* Load the word containing the first byte to replace */
	ldwas 0(t1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */
	
	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd,tr t4,r0,ret0	/* ret0: the first word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_phys_short_dst1_aligned
	/* Clear the register ret0 *if* no byte to keep from first dst word */
	ldi 0,ret0

	/* Isolate the bytes at end of second dst word that must not be replaced */
	subi 4,t3,t2
	sub arg2,t2,t2		/* Number of bytes to replace in second dst word */

	/* Check first if all second dst word has to be replaced */
	comib,=,n 4,t2,$bcopy_phys_short_all_dst_word

	/* Compute shift depending on byte alignment of dst and len */
	subi 4,t2,t4
	sh3add t4,r0,t4
	mtctl t4,11

	/* Load the second dst word to partially replace */
	ldwas,mb 4(t1),t4
	vshd t4,r0,t4

	/* Align the bytes to keep on their original position */
	sh3add t2,r0,t3
	mtctl t3,11
	vshd,tr r0,t4,ret1	/* ret1: the second word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_phys_short_all_dst_word
	/* Clear the register ret1 *if* no byte to keep from second dst word */
	ldi 0,ret1

	b,n $bcopy_phys_short_do_merge
	
$bcopy_phys_short_one_dst
	/*
	 * Just one word to replace in dst. Set the bytes to replace to zero
	 * for merge with src.
	 */

	ldi 0,ret1		/* ret1: zero as no second word to merge */
	
	/* Check first if all dst word has to be replaced */
	or arg1,arg2,t1
	extru,<> t1,31,2,r0
	b,n $bcopy_phys_short_clear_all

	/* Align dst on a word boundary */
	extru arg1,29,30,t1
	zdep t1,29,30,t1

	/* Load the word to partially replace */
	ldwas 0(t1),t1

	/* Check first if dst is already word aligned */
	comib,=,n 0,t3,$bcopy_phys_short_one_dst_aligned

	/* Compute bit shift depending on byte alignment of dst and len */	
	sh3add t2,r0,t2
	mtctl t2,11

	vshd r0,t1,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd,tr t4,r0,t4	/* t4: the begining of the word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_phys_short_one_dst_aligned
	/* Clear the register t4 *if* no byte to keep from begining of dst word */
	ldi 0,t4

	/* Check first if rightmost byte has to be replaced */
	add t3,arg2,t2
	comib,=,n 4,t2,$bcopy_phys_short_til_end
	
	/* Compute bit shift depending on byte alignment of dst and len */
	subi 4,t2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t1,r0,t1

	/* Align the bytes to replace on their original position */
	add t3,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd,tr r0,t1,t1	/* t1: the end of the word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_phys_short_til_end
	/* Clear the register t1 *if* no byte to keep from end of dst word */
	ldi 0,t1

	or,tr t4,t1,ret0	/* ret0: the only word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_phys_short_clear_all
	/* Clear the register ret0 *if* no byte to keep from first dst word */
	ldi 0,ret0

$bcopy_phys_short_do_merge
	/*
	 * Perform the merge of one or two src words and one or two dst word.
	 * arg0, arg1, arg2: The initial values (may be not word aligned).
	 * r1	: the first word of src (masked).
	 * r31	: the second word of src (masked) if any, zero otherwise.
	 * ret0	: the first word of dst (masked).
	 * ret1	: the second word of dst (masked) if any, zero otherwise.
	 */
	/*
	 * See if the src and dst are similary aligned
	 */
	xor	arg0,arg1,t1
	extru,=	t1,31,2,r0
	b,n $bcopy_phys_short_unaligned

$bcopy_phys_short_same_alignment
	/* Compute bit shift */
	extru arg1,31,2,t3	/* src and dst byte alignment */

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Merge the first words of src and dst correctly masked and store it*/ 
	or r1,ret0,t1
	stwas t1,0(arg1)

	/* Flush first word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there a second dst word to replace */
        comib,>=,n 0,arg2,$bcopy_phys_exit

	/* Merge the second words of src and dst correctly masked and store it*/
	or r31,ret1,t1
	stwas,mb t1,4(arg1)

	/* Flush second word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_phys_exit

$bcopy_phys_short_unaligned
	/*
	 * The source and destination are not aligned on the same boundary 
	 * types. We will have to shift the data around.
	 */

	/* 
	 * Compute the byte shift between src and dst and keep it in arg3.
	 */
	extru arg0,31,2,t2
	extru arg1,31,2,t3
	sub t3,t2,arg3		/* arg3 = shift between src and dst */
	comib,>,n 0,arg3,$bcopy_phys_short_shift_negative

	/*
	 * The shift is positive (byte shift of src is lower than dst one).
	 * Use the computed shift between src and dst (kept in arg3) to copy only
	 * the bytes that the first word of dst needs
	 */
	sh3add arg3,r0,t1
	mtctl t1,11
	vshd r0,r1,t1		/* t1: the first word to merge in dst */

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,ret0,t1
	stwas t1,0(arg1)

	/* Flush first word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_phys_exit

	/* Shift the two words of src by arg3 */
	vshd r1,r31,t1		/* t1: the exact last word of src to copy */

	/* Merge the two words of src (shifted) and the second dst word  */ 
	or t1,ret1,t1
	stwas,mb t1,4(arg1)

	/* Flush second word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_phys_exit
	
$bcopy_phys_short_shift_negative
	/*
	 * The shift is negative (byte shift of src is greater than dst one)
	 * t2: byte shift of src. t3: byte shift of dst.
	 */
	/* Recompute the shift to make it positive */
	sub t2,t3,arg3		/* arg3 = shift between dst and src */
	subi 4,arg3,arg3	/* Adapt the shift to vshd instruction */	

	/* 
	 * Use the computed shift between src and dst (kept in arg3) to copy
	 * the bytes that the first word of dst needs
	 */
	sh3add arg3,r0,t2
	mtctl t2,11
	vshd r1,r31,t1		/* t1: the first word to merge in dst */

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Merge the two words of src (shifted) and the first word of dst */ 
	or t1,ret0,t1
	stwas t1,0(arg1)

	/* Flush first word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_phys_exit

	/* Shift the last word of src by arg3 */
	vshd r31,r0,t1		/* t1: the exact last word of src to copy */

	/* Merge the second word of src (shifted) and the second word of dst */
	or t1,ret1,t1
	stwas,mb t1,4(arg1)

	/* Flush second word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_phys_exit

$bcopy_phys_long
	/*
	 * See if the src and dst are similary aligned
	 */
	xor	arg0,arg1,t1
	extru,=	t1,31,2,r0
	b,n	$bcopy_phys_unaligned

$bcopy_phys_same_alignment
	/*
	 * Align dst on a word by masking possible bytes of the first word
	 * of src before the first byte to copy
	 */
	extru arg0,31,2,t3	/* src and dst byte alignment */

	/* Check first if src and dst are already word aligned */
	comib,=,n 0,t3,$bcopy_phys_word_aligned
	
	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align src on a word boundary */
	extru arg0,29,30,arg0
	zdep arg0,29,30,arg0

	/* Load the word containing the first byte to copy */
	ldwas,ma 4(arg0),t1
	vshd t1,r0,t1		/* t1: just the bytes to copy but aligned on left */

	/* Align the bytes to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd r0,t1,t1		/* t1: the exact word to merge in dst */

	/*
	 * The first word of dst contains at least one byte that must
	 * not be modified by the copy operation. Prepare this word
	 * to be merged with the first word of src prepared in t1
	 */
	/* Compute shift depending on byte alignment of dst (=src alignment) */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Load the word containing the first byte to replace */
	ldwas 0(arg1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd t4,r0,t4		/* t4: the exact word to merge with src */
	
	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,t4,t1
	stwas,ma t1,4(arg1)

	/* Flush/Purge first word of src and dst. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	fdc	r1(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

$bcopy_phys_word_aligned
	/*
	 * Do as many 16bytes copies as possible
	 */
	comb,>,n arg3,arg2,$bcopy_phys_aligned_words
$bcopy_phys_aligned_16bytes
        ldwas,ma	4(arg0),t1
        ldwas,ma	4(arg0),t2
        ldwas,ma	4(arg0),t3
        ldwas,ma	4(arg0),t4
        stwas,ma	t1,4(arg1)
        stwas,ma	t2,4(arg1)
        stwas,ma	t3,4(arg1)
        stwas,ma	t4,4(arg1)
	addi		-16,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	ret0(arg0)
	fdc	ret0(arg1)
        comb,<= arg3,arg2,$bcopy_phys_aligned_16bytes
	ssm	PSW_D,r0

$bcopy_phys_aligned_words
	/*
	 * Do as many 4bytes copies as possible
	 */
        comib,>,n 4,arg2,$bcopy_phys_last_word
$bcopy_phys_aligned_words_loop
        ldwas,ma	4(arg0),t1
        stwas,ma	t1,4(arg1)
	addi		-4,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	fdc	r1(arg1)
        comib,<= 4,arg2,$bcopy_phys_aligned_words_loop
	ssm	PSW_D,r0

$bcopy_phys_last_word
	/*
	 * Do a final partial word copy if needed
	 */
        comib,>=,n 0,arg2,$bcopy_phys_exit
	/*
	 * Copy the 1,2 or 3 last bytes in dst.
	 */
	/* Load the last word of src to partially copy */
	ldwas 0(arg0),t3

	/* Mask the bytes at end of src word that must not be copied */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd t3,r0,t1		/* t1: the exact last word of src to copy */

	/* Load the last dst word to partially replace */
	ldwas 0(arg1),t3

	/* Isolate the bytes at end of dst word that must not be replaced */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t3,r0,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3		/* t3: the last word of dst to merge with src */

	/* Merge the last words of src and dst correctly masked and store it*/ 	
	or t1,t3,t1
	stwas t1,0(arg1)

	/* Flush/Purge last word of src and dst */
	rsm	PSW_D,r0
	pdc	0(arg0)
	fdc	0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_phys_exit 

$bcopy_phys_unaligned
	/*
	 * The source and destination are not aligned on the same boundary 
	 * types. We will have to shift the data around.
	 */

	/* 
	 * Compute the byte shift between src and dst and keep it in ret0.
	 */
	extru arg0,31,2,t3
	extru arg1,31,2,t2
	sub t2,t3,ret0		/* ret0 = shift between src and dst */
	comib,>,n 0,ret0,$bcopy_phys_shift_negative
	
	/*
	 * The shift is positive (byte shift of src is lower than dst one)
	 */

	/*
	 * Mask possible bytes of the first word of src before the
	 * first byte to copy
	 */
	/* Check first if src is already word aligned */
	comib,=,n 0,t3,$bcopy_phys_src_aligned

	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align src on a word boundary */
	extru arg0,29,30,arg0
	zdep arg0,29,30,arg0

	/* Load the word containing the first byte to copy */
	ldwas 0(arg0),t1
	vshd t1,r0,t1		/* t1: just the bytes to copy but aligned on left */

	/* Align the bytes to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd,tr r0,t1,t1	/* Always nullify the following ldwas instruction */

$bcopy_phys_src_aligned
	/* Load the word containing the first byte to copy *if* not done before*/
	ldwas 0(arg0),t1
	/* 
	 * Use the computed shift between src and dst (kept in ret0) to copy only
	 * the bytes that the first word of dst needs
	 */
	sh3add ret0,r0,t3
	mtctl t3,11
	vshd r0,t1,t1		/* t1: the exact word to merge in dst */

	/*
	 * The first word of dst contains at least one byte that must
	 * not be modified by the copy operation. Prepare this word
	 * to be merged with the first word of src prepared in t1
	 */
	/* Compute shift depending on byte alignment of dst */
	extru arg1,31,2,t3
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Load the word containing the first byte to replace */
	ldwas 0(arg1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd t4,r0,t4		/* t4: the exact word to merge with src */
	
	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,t4,t1
	stwas,ma t1,4(arg1)

	/* Flush/Purge first word of src and dst. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	0(arg0)
	fdc	r1(arg1)
	ssm	PSW_D,r0
	
	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_phys_exit

	/* 
	 * Initiate the word copy loop. 
	 * All the bytes of the first src word have not been copied.
	 */
	/* Reload the first word of src */
	ldwas,ma 4(arg0),t1

	/* Purge first word of src. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	ssm	PSW_D,r0

	/* Use the shift between src and dst computed before in ret0 */
	sh3add ret0,r0,t4	/* shift in bits */
	mtctl t4,11		/* store the shift in the Shift Amount Register */

	/* If less than a word to copy, don't enter the loop */
	comib,>,n 4,arg2,$bcopy_phys_no_loop

$bcopy_phys_word_loop:
	/* Load the next src word */
	ldwas,ma 4(arg0),t3
	vshd t1,t3,t4
	stwas,ma t4,4(arg1)
	addi -4,arg2,arg2	/* decrement the count */
	copy t3,t1		/* prepare the next loop */
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	fdc	r1(arg1)
	/* loop if more than a word to copy */
	comib,<= 4,arg2,$bcopy_phys_word_loop
	ssm	PSW_D,r0

	/* Is there something else to copy */
	comib,>=,n 0,arg2,$bcopy_phys_exit

$bcopy_phys_no_loop
	/*
	 * Copy the 1,2 or 3 last bytes in dst.
	 * ret0: the shift between src and dst. t1: the last word of src copied.
	 */
	/* Load the last word of src to partially copy */
	ldwas 0(arg0),t3

	/* Shift the last two words of src */
	vshd t1,t3,t4

	/* Mask the bytes at end of src word that must not be copied */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r0,t4,t4
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd t4,r0,t1		/* t1: the exact last word of src to copy */

	/* Load the last dst word to partially replace */
	ldwas 0(arg1),t3

	/* Isolate the bytes at end of dst word that must not be replaced */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t3,r0,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3		/* t3: the last word of dst to merge with src */

	/* Merge the last words of src and dst correctly masked and store it*/ 	
	or t1,t3,t1
	stwas t1,0(arg1)

	/* Flush/Purge last word of src and dst */
	rsm	PSW_D,r0
	pdc	0(arg0)
	fdc	0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_phys_exit 

$bcopy_phys_shift_negative
	/*
	 * The shift is negative (byte shift of src is greater than dst one)
	 * t3: byte shift of src. t2: byte shift of dst.
	 */
	/* Recompute the shift to make it positive */
	sub t3,t2,ret0		/* ret0 = shift between dst and src */
	subi 4,ret0,ret0	/* Adapt the shift to vshd instruction */

	/*
	 * Mask bytes of the first word of src before the first byte to copy
	 * (at least one).
	 */
	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align src on a word boundary */
	extru arg0,29,30,arg0
	zdep arg0,29,30,arg0

	/* Load the word containing the first byte to copy */
	ldwas,ma 4(arg0),t1
	vshd t1,r0,t1		/* t1: the bytes to copy but aligned on left */

	/* Align the bytes of first word to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd r0,t1,t1

	/* Purge first word of src. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	ssm	PSW_D,r0

	/* 
	 * Load the second word of src as there are more bytes to replace
	 * in the first word of dst than the number of bytes to copy from
	 * the first word of src
	 */
	ldwas 0(arg0),t3

	/* 
	 * Use the computed shift between src and dst (kept in ret0) to copy
	 * the bytes that the first word of dst needs
	 */
	sh3add ret0,r0,t2
	mtctl t2,11
	vshd t1,t3,t1		/* t1: the exact word to merge in dst */

	/*
	 * Prepare the first word of dst to be merged with the first word
	 * of src prepared in t1.
	 * It may contain 1 or 2 bytes that must not be modified 
	 * by the copy operation
	 */
	/* Check first if dst is already word aligned */
	extru arg1,31,2,t3
	comib,=,n 0,t3,$bcopy_phys_dst_aligned

	/* Compute shift depending on byte alignment of dst */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Load the word containing the first byte to replace */
	ldwas 0(arg1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd,tr t4,r0,t4	/* t4: the exact word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_phys_dst_aligned
	/* Clear the register t4 *if* no byte to keep from first dst word */
	ldi 0,t4
	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,t4,t1
	stwas,ma t1,4(arg1)

	/* Flush/Purge second word of src and first word of dst. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	0(arg0)
	fdc	r1(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_phys_exit

	/* 
	 * Initiate the word copy loop.
	 * All the bytes of the second src word have not been copied.
	 */
	/* Reload the second word of src */
	ldwas,ma 4(arg0),t1

	/* Purge second word of src. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	ssm	PSW_D,r0
	
	/* Use the shift between src and dst computed before in ret0 */
	sh3add ret0,r0,t4	/* shift in bits */
	mtctl t4,11		/* store the shift in the Shift Amount Register */

	/* If less than a word to copy, don't enter the loop */
	comib,>,n 4,arg2,$bcopy_phys_no_loop2

$bcopy_phys_word_loop2
	/* Load the next src word */
	ldwas,ma 4(arg0),t3
	vshd t1,t3,t4
	stwas,ma t4,4(arg1)
	addi -4,arg2,arg2	/* decrement the count */
	copy t3,t1		/* prepare the next loop */
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	fdc	r1(arg1)
	/* loop if more than a word to copy */
	comib,<= 4,arg2,$bcopy_phys_word_loop2
	ssm	PSW_D,r0

	/* Is there something else to copy */
	comib,>=,n 0,arg2,$bcopy_phys_exit

$bcopy_phys_no_loop2
	/*
	 * Copy the 1,2 or 3 last bytes in dst.
	 * ret0: the shift between src and dst. 
	 * t1: the last word of src partially copied.
	 */
	/* Check if a last word has to be copied from src */
	ldi 0,t3
	sub,> arg2,ret0,r0
	b,n $bcopy_phys_no_more_src

	/* Load the last word of src to partially copy *if* not already done */
	ldwas 0(arg0),t3

	/* Purge last word of src */
	rsm	PSW_D,r0
	pdc	0(arg0)
	ssm	PSW_D,r0

$bcopy_phys_no_more_src
	/* Shift the last word of src by ret0 */
	vshd t1,t3,t4

	/* Mask the bytes at end of src word that must not be copied */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r0,t4,t4
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd t4,r0,t1		/* t1: the exact last word of src to copy */

	/* Load the last dst word to partially replace */
	ldwas 0(arg1),t3

	/* Isolate the bytes at end of dst word that must not be replaced */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t3,r0,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3		/* t3: the last word of dst to merge with src */

	/* Merge the last words of src and dst correctly masked and store it*/ 	
	or t1,t3,t1
	stwas t1,0(arg1)

	/* Flush last word of dst */
	rsm	PSW_D,r0
	fdc	0(arg1)
	ssm	PSW_D,r0

	/* The End */

$bcopy_phys_exit
	sync
	bv,n	0(r2)
	.exit
	.procend

/*
 * void 
 * bcopy_lphys(src, dst, count)
 *      vm_offset_t     src;
 *      vm_offset_t     dst;
 *      int             count;
 */

	.export	bcopy_lphys
	.proc
	.callinfo 
bcopy_lphys
	.entry

        comb,>=,n r0,arg2,$bcopy_lphys_exit

	/*
	 * See if the source and destination are word aligned and if the count
	 * is an integer number of words. If so then we can use an optimized 
	 * routine. If not then branch to bcopy_lphys_checkalign and see what we can
	 * do there.
	 */

	ldi 16,arg3		/* keep 16 in arg3 for further use */
	ldi -16,ret0		/* keep -16 in ret0 for further use */
	ldi -4,r1		/* keep -4 in r1 for further use */

        or	arg0,arg1,t1
        or	t1,arg2,t2
        extru,= t2,31,2,r0
        b,n     $bcopy_lphys_checkalign
	
	comb,>,n arg3,arg2,$bcopy_lphys_movewords

	/*
	 * We can move the data in 4 word moves. We'll use 4 registers to 
	 * avoid interlock and pipeline stalls.
	 */
$bcopy_lphys_loop16

        ldws,ma	4(arg0),t1
        ldws,ma	4(arg0),t2
        ldws,ma	4(arg0),t3
        ldws,ma	4(arg0),t4
        stwas,ma	t1,4(arg1)
        stwas,ma	t2,4(arg1)
        stwas,ma	t3,4(arg1)
        stwas,ma	t4,4(arg1)
        addi		-16,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	fdc	ret0(arg1)
	comb,<= arg3,arg2,$bcopy_lphys_loop16
	ssm	PSW_D,r0

	/*
	 * Test if there is at least 1 word left to move.
	 */

$bcopy_lphys_movewords
        comib,>=,n 0,arg2,$bcopy_lphys_exit

	/*
	 * Clean up any remaining words that were not moved in the 16 byte
	 * moves
	 */
$bcopy_lphys_loop4
        ldws,ma	4(arg0),t1
        stwas,ma	t1,4(arg1)
        addi		 -4,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	fdc	r1(arg1)
	comib,<= 4,arg2,$bcopy_lphys_loop4
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_lphys_exit 

$bcopy_lphys_checkalign
	/*
	 * The source or destination is not word aligned or the count is not 
	 * an integral number of words.
	 */
	/*
	 * Check if less than 4 bytes to copy and handle it.
	 */
	comib,<,n 4,arg2,$bcopy_lphys_long

$bcopy_lphys_short
	/*
	 * Mask possible bytes of the first word of src before the 
	 * first byte to copy
	 */

	/* Align src on a word boundary */
	extru arg0,29,30,r31
	zdep r31,29,30,r31

	/* Load the word containing the first byte to copy */
	ldws 0(r31),r1

	/* Check first if src is already word aligned */
	extru arg0,31,2,t3	/* src byte alignment */
	comib,=,n 0,t3,$bcopy_lphys_short_src_aligned

	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	vshd r1,r0,r1

	/* Align the bytes to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd r0,r1,r1	/* r1: the first word to copy in dst */

$bcopy_lphys_short_src_aligned
	/*
	 * Check if there is a second word from src to copy
	 */
	subi 4,t3,t2
	comb,<=,n arg2,t2,$bcopy_lphys_short_one_src

$bcopy_lphys_short_two_src
	/*
	 * There is a second word to copy from src. Mask possible bytes
	 * at end of second src word that must not be copied.
	 */

	/* Load the second word of src to copy */
	ldws,mb 4(r31),t2

	subi 4,t3,t4
	sub arg2,t4,t4		/* Number of bytes to copy from second src word */

	/* Check first if all the word has to be copied */
	comib,=,n 4,t4,$bcopy_lphys_short_mask_dst

	/* Compute bit shift */	
	subi 4,t4,t3
	sh3add t3,r0,t3
	mtctl t3,11

	vshd r0,t2,t2
	
	/* Align the bytes to copy on their original position */
	sh3add t4,r0,t4
	mtctl t4,11
	vshd t2,r0,r31		/* r31: the second word to copy in dst */

	b,n $bcopy_lphys_short_mask_dst

$bcopy_lphys_short_one_src
	/*
	 * Just one word to copy from src. Mask the possible bytes at end
	 * of the src word that must not ne copied
	 */
	ldi 0,r31		/* r31: zero as no second word to copy */

	/* Compute bit shift */
	add t3,arg2,t2

	/* Check first if rightmost byte has to be copied */
	comib,=,n 4,t2,$bcopy_lphys_short_mask_dst

	subi 4,t2,t4
	sh3add t4,r0,t4
	mtctl t4,11

	vshd r0,r1,r1

	/* Align the bytes to copy on their original position */
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r1,r0,r1		/* r1: the first word to copy in dst */

$bcopy_lphys_short_mask_dst
	/*
	 * Prepare dst to be merged with src word prepared before.
	 * It may contain bytes that must not be modified by the copy 
	 * operation at the begining or at the end.
	 */
	/*
	 * Check if there is a second word from dst to replace.
	 */
	extru arg1,31,2,t3
	subi 4,t3,t2
	comb,<=,n arg2,t2,$bcopy_lphys_short_one_dst

$bcopy_lphys_short_two_dst	
	/*
	 * There is a second word to replace in dst. Set the bytes to
	 * replace at end of first word and at begining of second
	 * word to zero for merge with src.
	 */
	/* Check first if dst is already word aligned */
	comib,=,n 0,t3,$bcopy_lphys_short_dst1_aligned

	/* Compute shift depending on byte alignment of dst */
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,t1
	zdep t1,29,30,t1

	/* Load the word containing the first byte to replace */
	ldwas 0(t1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */
	
	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd,tr t4,r0,ret0	/* ret0: the first word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_lphys_short_dst1_aligned
	/* Clear the register ret0 *if* no byte to keep from first dst word */
	ldi 0,ret0

	/* Isolate the bytes at end of second dst word that must not be replaced */
	subi 4,t3,t2
	sub arg2,t2,t2		/* Number of bytes to replace in second dst word */

	/* Check first if all second dst word has to be replaced */
	comib,=,n 4,t2,$bcopy_lphys_short_all_dst_word

	/* Compute shift depending on byte alignment of dst and len */
	subi 4,t2,t4
	sh3add t4,r0,t4
	mtctl t4,11

	/* Load the second dst word to partially replace */
	ldwas,mb 4(t1),t4
	vshd t4,r0,t4

	/* Align the bytes to keep on their original position */
	sh3add t2,r0,t3
	mtctl t3,11
	vshd,tr r0,t4,ret1	/* ret1: the second word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_lphys_short_all_dst_word
	/* Clear the register ret1 *if* no byte to keep from second dst word */
	ldi 0,ret1

	b,n $bcopy_lphys_short_do_merge
	
$bcopy_lphys_short_one_dst
	/*
	 * Just one word to replace in dst. Set the bytes to replace to zero
	 * for merge with src.
	 */

	ldi 0,ret1		/* ret1: zero as no second word to merge */
	
	/* Check first if all dst word has to be replaced */
	or arg1,arg2,t1
	extru,<> t1,31,2,r0
	b,n $bcopy_lphys_short_clear_all

	/* Align dst on a word boundary */
	extru arg1,29,30,t1
	zdep t1,29,30,t1

	/* Load the word to partially replace */
	ldwas 0(t1),t1

	/* Check first if dst is already word aligned */
	comib,=,n 0,t3,$bcopy_lphys_short_one_dst_aligned

	/* Compute bit shift depending on byte alignment of dst and len */	
	sh3add t2,r0,t2
	mtctl t2,11

	vshd r0,t1,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd,tr t4,r0,t4	/* t4: the begining of the word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_lphys_short_one_dst_aligned
	/* Clear the register t4 *if* no byte to keep from begining of dst word */
	ldi 0,t4

	/* Check first if rightmost byte has to be replaced */
	add t3,arg2,t2
	comib,=,n 4,t2,$bcopy_lphys_short_til_end
	
	/* Compute bit shift depending on byte alignment of dst and len */
	subi 4,t2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t1,r0,t1

	/* Align the bytes to replace on their original position */
	add t3,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd,tr r0,t1,t1	/* t1: the end of the word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_lphys_short_til_end
	/* Clear the register t1 *if* no byte to keep from end of dst word */
	ldi 0,t1

	or,tr t4,t1,ret0	/* ret0: the only word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_lphys_short_clear_all
	/* Clear the register ret0 *if* no byte to keep from first dst word */
	ldi 0,ret0

$bcopy_lphys_short_do_merge
	/*
	 * Perform the merge of one or two src words and one or two dst word.
	 * arg0, arg1, arg2: The initial values (may be not word aligned).
	 * r1	: the first word of src (masked).
	 * r31	: the second word of src (masked) if any, zero otherwise.
	 * ret0	: the first word of dst (masked).
	 * ret1	: the second word of dst (masked) if any, zero otherwise.
	 */
	/*
	 * See if the src and dst are similary aligned
	 */
	xor	arg0,arg1,t1
	extru,=	t1,31,2,r0
	b,n $bcopy_lphys_short_unaligned

$bcopy_lphys_short_same_alignment
	/* Compute bit shift */
	extru arg1,31,2,t3	/* src and dst byte alignment */

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Merge the first words of src and dst correctly masked and store it*/ 
	or r1,ret0,t1
	stwas t1,0(arg1)

	/* Flush first word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there a second dst word to replace */
        comib,>=,n 0,arg2,$bcopy_lphys_exit

	/* Merge the second words of src and dst correctly masked and store it*/
	or r31,ret1,t1
	stwas,mb t1,4(arg1)

	/* Flush second word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_lphys_exit

$bcopy_lphys_short_unaligned
	/*
	 * The source and destination are not aligned on the same boundary 
	 * types. We will have to shift the data around.
	 */

	/* 
	 * Compute the byte shift between src and dst and keep it in arg3.
	 */
	extru arg0,31,2,t2
	extru arg1,31,2,t3
	sub t3,t2,arg3		/* arg3 = shift between src and dst */
	comib,>,n 0,arg3,$bcopy_lphys_short_shift_negative

	/*
	 * The shift is positive (byte shift of src is lower than dst one).
	 * Use the computed shift between src and dst (kept in arg3) to copy only
	 * the bytes that the first word of dst needs
	 */
	sh3add arg3,r0,t1
	mtctl t1,11
	vshd r0,r1,t1		/* t1: the first word to merge in dst */

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,ret0,t1
	stwas t1,0(arg1)

	/* Flush first word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_lphys_exit

	/* Shift the two words of src by arg3 */
	vshd r1,r31,t1		/* t1: the exact last word of src to copy */

	/* Merge the two words of src (shifted) and the second dst word  */ 
	or t1,ret1,t1
	stwas,mb t1,4(arg1)

	/* Flush second word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_lphys_exit
	
$bcopy_lphys_short_shift_negative
	/*
	 * The shift is negative (byte shift of src is greater than dst one)
	 * t2: byte shift of src. t3: byte shift of dst.
	 */
	/* Recompute the shift to make it positive */
	sub t2,t3,arg3		/* arg3 = shift between dst and src */
	subi 4,arg3,arg3	/* Adapt the shift to vshd instruction */	

	/* 
	 * Use the computed shift between src and dst (kept in arg3) to copy
	 * the bytes that the first word of dst needs
	 */
	sh3add arg3,r0,t2
	mtctl t2,11
	vshd r1,r31,t1		/* t1: the first word to merge in dst */

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Merge the two words of src (shifted) and the first word of dst */ 
	or t1,ret0,t1
	stwas t1,0(arg1)

	/* Flush first word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_lphys_exit

	/* Shift the last word of src by arg3 */
	vshd r31,r0,t1		/* t1: the exact last word of src to copy */

	/* Merge the second word of src (shifted) and the second word of dst */
	or t1,ret1,t1
	stwas,mb t1,4(arg1)

	/* Flush second word of dst */
	rsm	PSW_D,r0
	fdc	r0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_lphys_exit

$bcopy_lphys_long
	/*
	 * See if the src and dst are similary aligned
	 */
	xor	arg0,arg1,t1
	extru,=	t1,31,2,r0
	b,n	$bcopy_lphys_unaligned

$bcopy_lphys_same_alignment
	/*
	 * Align dst on a word by masking possible bytes of the first word
	 * of src before the first byte to copy
	 */
	extru arg0,31,2,t3	/* src and dst byte alignment */

	/* Check first if src and dst are already word aligned */
	comib,=,n 0,t3,$bcopy_lphys_word_aligned
	
	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align src on a word boundary */
	extru arg0,29,30,arg0
	zdep arg0,29,30,arg0

	/* Load the word containing the first byte to copy */
	ldws,ma 4(arg0),t1
	vshd t1,r0,t1		/* t1: just the bytes to copy but aligned on left */

	/* Align the bytes to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd r0,t1,t1		/* t1: the exact word to merge in dst */

	/*
	 * The first word of dst contains at least one byte that must
	 * not be modified by the copy operation. Prepare this word
	 * to be merged with the first word of src prepared in t1
	 */
	/* Compute shift depending on byte alignment of dst (=src alignment) */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Load the word containing the first byte to replace */
	ldwas 0(arg1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd t4,r0,t4		/* t4: the exact word to merge with src */
	
	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,t4,t1
	stwas,ma t1,4(arg1)

	/* Flush/Purge first word of src and dst. r1: contains -4 */
	rsm	PSW_D,r0
	fdc	r1(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

$bcopy_lphys_word_aligned
	/*
	 * Do as many 16bytes copies as possible
	 */
	comb,>,n arg3,arg2,$bcopy_lphys_aligned_words
$bcopy_lphys_aligned_16bytes
        ldws,ma	4(arg0),t1
        ldws,ma	4(arg0),t2
        ldws,ma	4(arg0),t3
        ldws,ma	4(arg0),t4
        stwas,ma	t1,4(arg1)
        stwas,ma	t2,4(arg1)
        stwas,ma	t3,4(arg1)
        stwas,ma	t4,4(arg1)
	addi		-16,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	fdc	ret0(arg1)
        comb,<= arg3,arg2,$bcopy_lphys_aligned_16bytes
	ssm	PSW_D,r0

$bcopy_lphys_aligned_words
	/*
	 * Do as many 4bytes copies as possible
	 */
        comib,>,n 4,arg2,$bcopy_lphys_last_word
$bcopy_lphys_aligned_words_loop
        ldws,ma	4(arg0),t1
        stwas,ma	t1,4(arg1)
	addi		-4,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	fdc	r1(arg1)
        comib,<= 4,arg2,$bcopy_lphys_aligned_words_loop
	ssm	PSW_D,r0

$bcopy_lphys_last_word
	/*
	 * Do a final partial word copy if needed
	 */
        comib,>=,n 0,arg2,$bcopy_lphys_exit
	/*
	 * Copy the 1,2 or 3 last bytes in dst.
	 */
	/* Load the last word of src to partially copy */
	ldws 0(arg0),t3

	/* Mask the bytes at end of src word that must not be copied */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd t3,r0,t1		/* t1: the exact last word of src to copy */

	/* Load the last dst word to partially replace */
	ldwas 0(arg1),t3

	/* Isolate the bytes at end of dst word that must not be replaced */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t3,r0,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3		/* t3: the last word of dst to merge with src */

	/* Merge the last words of src and dst correctly masked and store it*/ 	
	or t1,t3,t1
	stwas t1,0(arg1)

	/* Flush/Purge last word of src and dst */
	rsm	PSW_D,r0
	fdc	0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_lphys_exit 

$bcopy_lphys_unaligned
	/*
	 * The source and destination are not aligned on the same boundary 
	 * types. We will have to shift the data around.
	 */

	/* 
	 * Compute the byte shift between src and dst and keep it in ret0.
	 */
	extru arg0,31,2,t3
	extru arg1,31,2,t2
	sub t2,t3,ret0		/* ret0 = shift between src and dst */
	comib,>,n 0,ret0,$bcopy_lphys_shift_negative
	
	/*
	 * The shift is positive (byte shift of src is lower than dst one)
	 */

	/*
	 * Mask possible bytes of the first word of src before the
	 * first byte to copy
	 */
	/* Check first if src is already word aligned */
	comib,=,n 0,t3,$bcopy_lphys_src_aligned

	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align src on a word boundary */
	extru arg0,29,30,arg0
	zdep arg0,29,30,arg0

	/* Load the word containing the first byte to copy */
	ldws 0(arg0),t1
	vshd t1,r0,t1		/* t1: just the bytes to copy but aligned on left */

	/* Align the bytes to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd,tr r0,t1,t1	/* Always nullify the following ldwas instruction */

$bcopy_lphys_src_aligned
	/* Load the word containing the first byte to copy *if* not done before*/
	ldws 0(arg0),t1
	/* 
	 * Use the computed shift between src and dst (kept in ret0) to copy only
	 * the bytes that the first word of dst needs
	 */
	sh3add ret0,r0,t3
	mtctl t3,11
	vshd r0,t1,t1		/* t1: the exact word to merge in dst */

	/*
	 * The first word of dst contains at least one byte that must
	 * not be modified by the copy operation. Prepare this word
	 * to be merged with the first word of src prepared in t1
	 */
	/* Compute shift depending on byte alignment of dst */
	extru arg1,31,2,t3
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Load the word containing the first byte to replace */
	ldwas 0(arg1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd t4,r0,t4		/* t4: the exact word to merge with src */
	
	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,t4,t1
	stwas,ma t1,4(arg1)

	/* Flush/Purge first word of src and dst. r1: contains -4 */
	rsm	PSW_D,r0
	fdc	r1(arg1)
	ssm	PSW_D,r0
	
	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_lphys_exit

	/* 
	 * Initiate the word copy loop. 
	 * All the bytes of the first src word have not been copied.
	 */
	/* Reload the first word of src */
	ldws,ma 4(arg0),t1

	/* Use the shift between src and dst computed before in ret0 */
	sh3add ret0,r0,t4	/* shift in bits */
	mtctl t4,11		/* store the shift in the Shift Amount Register */

	/* If less than a word to copy, don't enter the loop */
	comib,>,n 4,arg2,$bcopy_lphys_no_loop

$bcopy_lphys_word_loop:
	/* Load the next src word */
	ldws,ma 4(arg0),t3
	vshd t1,t3,t4
	stwas,ma t4,4(arg1)
	addi -4,arg2,arg2	/* decrement the count */
	copy t3,t1		/* prepare the next loop */
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	fdc	r1(arg1)
	/* loop if more than a word to copy */
	comib,<= 4,arg2,$bcopy_lphys_word_loop
	ssm	PSW_D,r0

	/* Is there something else to copy */
	comib,>=,n 0,arg2,$bcopy_lphys_exit

$bcopy_lphys_no_loop
	/*
	 * Copy the 1,2 or 3 last bytes in dst.
	 * ret0: the shift between src and dst. t1: the last word of src copied.
	 */
	/* Load the last word of src to partially copy */
	ldws 0(arg0),t3

	/* Shift the last two words of src */
	vshd t1,t3,t4

	/* Mask the bytes at end of src word that must not be copied */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r0,t4,t4
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd t4,r0,t1		/* t1: the exact last word of src to copy */

	/* Load the last dst word to partially replace */
	ldwas 0(arg1),t3

	/* Isolate the bytes at end of dst word that must not be replaced */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t3,r0,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3		/* t3: the last word of dst to merge with src */

	/* Merge the last words of src and dst correctly masked and store it*/ 	
	or t1,t3,t1
	stwas t1,0(arg1)

	/* Flush/Purge last word of src and dst */
	rsm	PSW_D,r0
	fdc	0(arg1)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_lphys_exit 

$bcopy_lphys_shift_negative
	/*
	 * The shift is negative (byte shift of src is greater than dst one)
	 * t3: byte shift of src. t2: byte shift of dst.
	 */
	/* Recompute the shift to make it positive */
	sub t3,t2,ret0		/* ret0 = shift between dst and src */
	subi 4,ret0,ret0	/* Adapt the shift to vshd instruction */

	/*
	 * Mask bytes of the first word of src before the first byte to copy
	 * (at least one).
	 */
	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align src on a word boundary */
	extru arg0,29,30,arg0
	zdep arg0,29,30,arg0

	/* Load the word containing the first byte to copy */
	ldws,ma 4(arg0),t1
	vshd t1,r0,t1		/* t1: the bytes to copy but aligned on left */

	/* Align the bytes of first word to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd r0,t1,t1

	/* 
	 * Load the second word of src as there are more bytes to replace
	 * in the first word of dst than the number of bytes to copy from
	 * the first word of src
	 */
	ldws 0(arg0),t3

	/* 
	 * Use the computed shift between src and dst (kept in ret0) to copy
	 * the bytes that the first word of dst needs
	 */
	sh3add ret0,r0,t2
	mtctl t2,11
	vshd t1,t3,t1		/* t1: the exact word to merge in dst */

	/*
	 * Prepare the first word of dst to be merged with the first word
	 * of src prepared in t1.
	 * It may contain 1 or 2 bytes that must not be modified 
	 * by the copy operation
	 */
	/* Check first if dst is already word aligned */
	extru arg1,31,2,t3
	comib,=,n 0,t3,$bcopy_lphys_dst_aligned

	/* Compute shift depending on byte alignment of dst */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Load the word containing the first byte to replace */
	ldwas 0(arg1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd,tr t4,r0,t4	/* t4: the exact word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_lphys_dst_aligned
	/* Clear the register t4 *if* no byte to keep from first dst word */
	ldi 0,t4
	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,t4,t1
	stwas,ma t1,4(arg1)

	/* Flush/Purge second word of src and first word of dst. r1: contains -4 */
	rsm	PSW_D,r0
	fdc	r1(arg1)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_lphys_exit

	/* 
	 * Initiate the word copy loop.
	 * All the bytes of the second src word have not been copied.
	 */
	/* Reload the second word of src */
	ldws,ma 4(arg0),t1

	/* Use the shift between src and dst computed before in ret0 */
	sh3add ret0,r0,t4	/* shift in bits */
	mtctl t4,11		/* store the shift in the Shift Amount Register */

	/* If less than a word to copy, don't enter the loop */
	comib,>,n 4,arg2,$bcopy_lphys_no_loop2

$bcopy_lphys_word_loop2
	/* Load the next src word */
	ldws,ma 4(arg0),t3
	vshd t1,t3,t4
	stwas,ma t4,4(arg1)
	addi -4,arg2,arg2	/* decrement the count */
	copy t3,t1		/* prepare the next loop */
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	fdc	r1(arg1)
	/* loop if more than a word to copy */
	comib,<= 4,arg2,$bcopy_lphys_word_loop2
	ssm	PSW_D,r0

	/* Is there something else to copy */
	comib,>=,n 0,arg2,$bcopy_lphys_exit

$bcopy_lphys_no_loop2
	/*
	 * Copy the 1,2 or 3 last bytes in dst.
	 * ret0: the shift between src and dst. 
	 * t1: the last word of src partially copied.
	 */
	/* Check if a last word has to be copied from src */
	ldi 0,t3
	sub,> arg2,ret0,r0
	b,n $bcopy_lphys_no_more_src

	/* Load the last word of src to partially copy *if* not already done */
	ldws 0(arg0),t3

$bcopy_lphys_no_more_src
	/* Shift the last word of src by ret0 */
	vshd t1,t3,t4

	/* Mask the bytes at end of src word that must not be copied */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r0,t4,t4
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd t4,r0,t1		/* t1: the exact last word of src to copy */

	/* Load the last dst word to partially replace */
	ldwas 0(arg1),t3

	/* Isolate the bytes at end of dst word that must not be replaced */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t3,r0,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3		/* t3: the last word of dst to merge with src */

	/* Merge the last words of src and dst correctly masked and store it*/ 	
	or t1,t3,t1
	stwas t1,0(arg1)

	/* Flush last word of dst */
	rsm	PSW_D,r0
	fdc	0(arg1)
	ssm	PSW_D,r0

	/* The End */

$bcopy_lphys_exit
	sync
	bv,n	0(r2)
	.exit
	.procend

/*
 * void 
 * bcopy_rphys(src, dst, count)
 *      vm_offset_t     src;
 *      vm_offset_t     dst;
 *      int             count;
 */

	.export	bcopy_rphys
	.proc
	.callinfo 
bcopy_rphys
	.entry

        comb,>=,n r0,arg2,$bcopy_rphys_exit

	/*
	 * See if the source and destination are word aligned and if the count
	 * is an integer number of words. If so then we can use an optimized 
	 * routine. If not then branch to bcopy_rphys_checkalign and see what we can
	 * do there.
	 */

	ldi 16,arg3		/* keep 16 in arg3 for further use */
	ldi -16,ret0		/* keep -16 in ret0 for further use */
	ldi -4,r1		/* keep -4 in r1 for further use */

        or	arg0,arg1,t1
        or	t1,arg2,t2
        extru,= t2,31,2,r0
        b,n     $bcopy_rphys_checkalign
	
	comb,>,n arg3,arg2,$bcopy_rphys_movewords

	/*
	 * We can move the data in 4 word moves. We'll use 4 registers to 
	 * avoid interlock and pipeline stalls.
	 */
$bcopy_rphys_loop16

        ldwas,ma	4(arg0),t1
        ldwas,ma	4(arg0),t2
        ldwas,ma	4(arg0),t3
        ldwas,ma	4(arg0),t4
        stws,ma	t1,4(arg1)
        stws,ma	t2,4(arg1)
        stws,ma	t3,4(arg1)
        stws,ma	t4,4(arg1)
        addi		-16,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	ret0(arg0)
	comb,<= arg3,arg2,$bcopy_rphys_loop16
	ssm	PSW_D,r0

	/*
	 * Test if there is at least 1 word left to move.
	 */

$bcopy_rphys_movewords
        comib,>=,n 0,arg2,$bcopy_rphys_exit

	/*
	 * Clean up any remaining words that were not moved in the 16 byte
	 * moves
	 */
$bcopy_rphys_loop4
        ldwas,ma	4(arg0),t1
        stws,ma	t1,4(arg1)
        addi		 -4,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	comib,<= 4,arg2,$bcopy_rphys_loop4
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_rphys_exit 

$bcopy_rphys_checkalign
	/*
	 * The source or destination is not word aligned or the count is not 
	 * an integral number of words.
	 */
	/*
	 * Check if less than 4 bytes to copy and handle it.
	 */
	comib,<,n 4,arg2,$bcopy_rphys_long

$bcopy_rphys_short
	/*
	 * Mask possible bytes of the first word of src before the 
	 * first byte to copy
	 */

	/* Align src on a word boundary */
	extru arg0,29,30,r31
	zdep r31,29,30,r31

	/* Load the word containing the first byte to copy */
	ldwas 0(r31),r1

	/* Purge first word of src */
	rsm	PSW_D,r0
	pdc	r0(r31)
	ssm	PSW_D,r0

	/* Check first if src is already word aligned */
	extru arg0,31,2,t3	/* src byte alignment */
	comib,=,n 0,t3,$bcopy_rphys_short_src_aligned

	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	vshd r1,r0,r1

	/* Align the bytes to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd r0,r1,r1	/* r1: the first word to copy in dst */

$bcopy_rphys_short_src_aligned
	/*
	 * Check if there is a second word from src to copy
	 */
	subi 4,t3,t2
	comb,<=,n arg2,t2,$bcopy_rphys_short_one_src

$bcopy_rphys_short_two_src
	/*
	 * There is a second word to copy from src. Mask possible bytes
	 * at end of second src word that must not be copied.
	 */

	/* Load the second word of src to copy */
	ldwas,mb 4(r31),t2

	/* Purge second word of src */
	rsm	PSW_D,r0
	pdc	r0(r31)
	ssm	PSW_D,r0

	subi 4,t3,t4
	sub arg2,t4,t4		/* Number of bytes to copy from second src word */

	/* Check first if all the word has to be copied */
	comib,=,n 4,t4,$bcopy_rphys_short_mask_dst

	/* Compute bit shift */	
	subi 4,t4,t3
	sh3add t3,r0,t3
	mtctl t3,11

	vshd r0,t2,t2
	
	/* Align the bytes to copy on their original position */
	sh3add t4,r0,t4
	mtctl t4,11
	vshd t2,r0,r31		/* r31: the second word to copy in dst */

	b,n $bcopy_rphys_short_mask_dst

$bcopy_rphys_short_one_src
	/*
	 * Just one word to copy from src. Mask the possible bytes at end
	 * of the src word that must not ne copied
	 */
	ldi 0,r31		/* r31: zero as no second word to copy */

	/* Compute bit shift */
	add t3,arg2,t2

	/* Check first if rightmost byte has to be copied */
	comib,=,n 4,t2,$bcopy_rphys_short_mask_dst

	subi 4,t2,t4
	sh3add t4,r0,t4
	mtctl t4,11

	vshd r0,r1,r1

	/* Align the bytes to copy on their original position */
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r1,r0,r1		/* r1: the first word to copy in dst */

$bcopy_rphys_short_mask_dst
	/*
	 * Prepare dst to be merged with src word prepared before.
	 * It may contain bytes that must not be modified by the copy 
	 * operation at the begining or at the end.
	 */
	/*
	 * Check if there is a second word from dst to replace.
	 */
	extru arg1,31,2,t3
	subi 4,t3,t2
	comb,<=,n arg2,t2,$bcopy_rphys_short_one_dst

$bcopy_rphys_short_two_dst	
	/*
	 * There is a second word to replace in dst. Set the bytes to
	 * replace at end of first word and at begining of second
	 * word to zero for merge with src.
	 */
	/* Check first if dst is already word aligned */
	comib,=,n 0,t3,$bcopy_rphys_short_dst1_aligned

	/* Compute shift depending on byte alignment of dst */
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,t1
	zdep t1,29,30,t1

	/* Load the word containing the first byte to replace */
	ldws 0(t1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */
	
	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd,tr t4,r0,ret0	/* ret0: the first word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_rphys_short_dst1_aligned
	/* Clear the register ret0 *if* no byte to keep from first dst word */
	ldi 0,ret0

	/* Isolate the bytes at end of second dst word that must not be replaced */
	subi 4,t3,t2
	sub arg2,t2,t2		/* Number of bytes to replace in second dst word */

	/* Check first if all second dst word has to be replaced */
	comib,=,n 4,t2,$bcopy_rphys_short_all_dst_word

	/* Compute shift depending on byte alignment of dst and len */
	subi 4,t2,t4
	sh3add t4,r0,t4
	mtctl t4,11

	/* Load the second dst word to partially replace */
	ldws,mb 4(t1),t4
	vshd t4,r0,t4

	/* Align the bytes to keep on their original position */
	sh3add t2,r0,t3
	mtctl t3,11
	vshd,tr r0,t4,ret1	/* ret1: the second word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_rphys_short_all_dst_word
	/* Clear the register ret1 *if* no byte to keep from second dst word */
	ldi 0,ret1

	b,n $bcopy_rphys_short_do_merge
	
$bcopy_rphys_short_one_dst
	/*
	 * Just one word to replace in dst. Set the bytes to replace to zero
	 * for merge with src.
	 */

	ldi 0,ret1		/* ret1: zero as no second word to merge */
	
	/* Check first if all dst word has to be replaced */
	or arg1,arg2,t1
	extru,<> t1,31,2,r0
	b,n $bcopy_rphys_short_clear_all

	/* Align dst on a word boundary */
	extru arg1,29,30,t1
	zdep t1,29,30,t1

	/* Load the word to partially replace */
	ldws 0(t1),t1

	/* Check first if dst is already word aligned */
	comib,=,n 0,t3,$bcopy_rphys_short_one_dst_aligned

	/* Compute bit shift depending on byte alignment of dst and len */	
	sh3add t2,r0,t2
	mtctl t2,11

	vshd r0,t1,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd,tr t4,r0,t4	/* t4: the begining of the word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_rphys_short_one_dst_aligned
	/* Clear the register t4 *if* no byte to keep from begining of dst word */
	ldi 0,t4

	/* Check first if rightmost byte has to be replaced */
	add t3,arg2,t2
	comib,=,n 4,t2,$bcopy_rphys_short_til_end
	
	/* Compute bit shift depending on byte alignment of dst and len */
	subi 4,t2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t1,r0,t1

	/* Align the bytes to replace on their original position */
	add t3,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd,tr r0,t1,t1	/* t1: the end of the word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_rphys_short_til_end
	/* Clear the register t1 *if* no byte to keep from end of dst word */
	ldi 0,t1

	or,tr t4,t1,ret0	/* ret0: the only word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_rphys_short_clear_all
	/* Clear the register ret0 *if* no byte to keep from first dst word */
	ldi 0,ret0

$bcopy_rphys_short_do_merge
	/*
	 * Perform the merge of one or two src words and one or two dst word.
	 * arg0, arg1, arg2: The initial values (may be not word aligned).
	 * r1	: the first word of src (masked).
	 * r31	: the second word of src (masked) if any, zero otherwise.
	 * ret0	: the first word of dst (masked).
	 * ret1	: the second word of dst (masked) if any, zero otherwise.
	 */
	/*
	 * See if the src and dst are similary aligned
	 */
	xor	arg0,arg1,t1
	extru,=	t1,31,2,r0
	b,n $bcopy_rphys_short_unaligned

$bcopy_rphys_short_same_alignment
	/* Compute bit shift */
	extru arg1,31,2,t3	/* src and dst byte alignment */

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Merge the first words of src and dst correctly masked and store it*/ 
	or r1,ret0,t1
	stws t1,0(arg1)

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there a second dst word to replace */
        comib,>=,n 0,arg2,$bcopy_rphys_exit

	/* Merge the second words of src and dst correctly masked and store it*/
	or r31,ret1,t1
	stws,mb t1,4(arg1)

	/* The End */
	b,n $bcopy_rphys_exit

$bcopy_rphys_short_unaligned
	/*
	 * The source and destination are not aligned on the same boundary 
	 * types. We will have to shift the data around.
	 */

	/* 
	 * Compute the byte shift between src and dst and keep it in arg3.
	 */
	extru arg0,31,2,t2
	extru arg1,31,2,t3
	sub t3,t2,arg3		/* arg3 = shift between src and dst */
	comib,>,n 0,arg3,$bcopy_rphys_short_shift_negative

	/*
	 * The shift is positive (byte shift of src is lower than dst one).
	 * Use the computed shift between src and dst (kept in arg3) to copy only
	 * the bytes that the first word of dst needs
	 */
	sh3add arg3,r0,t1
	mtctl t1,11
	vshd r0,r1,t1		/* t1: the first word to merge in dst */

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,ret0,t1
	stws t1,0(arg1)

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_rphys_exit

	/* Shift the two words of src by arg3 */
	vshd r1,r31,t1		/* t1: the exact last word of src to copy */

	/* Merge the two words of src (shifted) and the second dst word  */ 
	or t1,ret1,t1
	stws,mb t1,4(arg1)

	/* The End */
	b,n $bcopy_rphys_exit
	
$bcopy_rphys_short_shift_negative
	/*
	 * The shift is negative (byte shift of src is greater than dst one)
	 * t2: byte shift of src. t3: byte shift of dst.
	 */
	/* Recompute the shift to make it positive */
	sub t2,t3,arg3		/* arg3 = shift between dst and src */
	subi 4,arg3,arg3	/* Adapt the shift to vshd instruction */	

	/* 
	 * Use the computed shift between src and dst (kept in arg3) to copy
	 * the bytes that the first word of dst needs
	 */
	sh3add arg3,r0,t2
	mtctl t2,11
	vshd r1,r31,t1		/* t1: the first word to merge in dst */

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Merge the two words of src (shifted) and the first word of dst */ 
	or t1,ret0,t1
	stws t1,0(arg1)

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_rphys_exit

	/* Shift the last word of src by arg3 */
	vshd r31,r0,t1		/* t1: the exact last word of src to copy */

	/* Merge the second word of src (shifted) and the second word of dst */
	or t1,ret1,t1
	stws,mb t1,4(arg1)

	/* The End */
	b,n $bcopy_rphys_exit

$bcopy_rphys_long
	/*
	 * See if the src and dst are similary aligned
	 */
	xor	arg0,arg1,t1
	extru,=	t1,31,2,r0
	b,n	$bcopy_rphys_unaligned

$bcopy_rphys_same_alignment
	/*
	 * Align dst on a word by masking possible bytes of the first word
	 * of src before the first byte to copy
	 */
	extru arg0,31,2,t3	/* src and dst byte alignment */

	/* Check first if src and dst are already word aligned */
	comib,=,n 0,t3,$bcopy_rphys_word_aligned
	
	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align src on a word boundary */
	extru arg0,29,30,arg0
	zdep arg0,29,30,arg0

	/* Load the word containing the first byte to copy */
	ldwas,ma 4(arg0),t1
	vshd t1,r0,t1		/* t1: just the bytes to copy but aligned on left */

	/* Align the bytes to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd r0,t1,t1		/* t1: the exact word to merge in dst */

	/*
	 * The first word of dst contains at least one byte that must
	 * not be modified by the copy operation. Prepare this word
	 * to be merged with the first word of src prepared in t1
	 */
	/* Compute shift depending on byte alignment of dst (=src alignment) */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Load the word containing the first byte to replace */
	ldws 0(arg1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd t4,r0,t4		/* t4: the exact word to merge with src */
	
	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,t4,t1
	stws,ma t1,4(arg1)

	/* Flush/Purge first word of src and dst. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

$bcopy_rphys_word_aligned
	/*
	 * Do as many 16bytes copies as possible
	 */
	comb,>,n arg3,arg2,$bcopy_rphys_aligned_words
$bcopy_rphys_aligned_16bytes
        ldwas,ma	4(arg0),t1
        ldwas,ma	4(arg0),t2
        ldwas,ma	4(arg0),t3
        ldwas,ma	4(arg0),t4
        stws,ma	t1,4(arg1)
        stws,ma	t2,4(arg1)
        stws,ma	t3,4(arg1)
        stws,ma	t4,4(arg1)
	addi		-16,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	ret0(arg0)
        comb,<= arg3,arg2,$bcopy_rphys_aligned_16bytes
	ssm	PSW_D,r0

$bcopy_rphys_aligned_words
	/*
	 * Do as many 4bytes copies as possible
	 */
        comib,>,n 4,arg2,$bcopy_rphys_last_word
$bcopy_rphys_aligned_words_loop
        ldwas,ma	4(arg0),t1
        stws,ma	t1,4(arg1)
	addi		-4,arg2,arg2
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
        comib,<= 4,arg2,$bcopy_rphys_aligned_words_loop
	ssm	PSW_D,r0

$bcopy_rphys_last_word
	/*
	 * Do a final partial word copy if needed
	 */
        comib,>=,n 0,arg2,$bcopy_rphys_exit
	/*
	 * Copy the 1,2 or 3 last bytes in dst.
	 */
	/* Load the last word of src to partially copy */
	ldwas 0(arg0),t3

	/* Mask the bytes at end of src word that must not be copied */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd t3,r0,t1		/* t1: the exact last word of src to copy */

	/* Load the last dst word to partially replace */
	ldws 0(arg1),t3

	/* Isolate the bytes at end of dst word that must not be replaced */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t3,r0,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3		/* t3: the last word of dst to merge with src */

	/* Merge the last words of src and dst correctly masked and store it*/ 	
	or t1,t3,t1
	stws t1,0(arg1)

	/* Flush/Purge last word of src and dst */
	rsm	PSW_D,r0
	pdc	0(arg0)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_rphys_exit 

$bcopy_rphys_unaligned
	/*
	 * The source and destination are not aligned on the same boundary 
	 * types. We will have to shift the data around.
	 */

	/* 
	 * Compute the byte shift between src and dst and keep it in ret0.
	 */
	extru arg0,31,2,t3
	extru arg1,31,2,t2
	sub t2,t3,ret0		/* ret0 = shift between src and dst */
	comib,>,n 0,ret0,$bcopy_rphys_shift_negative
	
	/*
	 * The shift is positive (byte shift of src is lower than dst one)
	 */

	/*
	 * Mask possible bytes of the first word of src before the
	 * first byte to copy
	 */
	/* Check first if src is already word aligned */
	comib,=,n 0,t3,$bcopy_rphys_src_aligned

	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align src on a word boundary */
	extru arg0,29,30,arg0
	zdep arg0,29,30,arg0

	/* Load the word containing the first byte to copy */
	ldwas 0(arg0),t1
	vshd t1,r0,t1		/* t1: just the bytes to copy but aligned on left */

	/* Align the bytes to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd,tr r0,t1,t1	/* Always nullify the following ldwas instruction */

$bcopy_rphys_src_aligned
	/* Load the word containing the first byte to copy *if* not done before*/
	ldwas 0(arg0),t1
	/* 
	 * Use the computed shift between src and dst (kept in ret0) to copy only
	 * the bytes that the first word of dst needs
	 */
	sh3add ret0,r0,t3
	mtctl t3,11
	vshd r0,t1,t1		/* t1: the exact word to merge in dst */

	/*
	 * The first word of dst contains at least one byte that must
	 * not be modified by the copy operation. Prepare this word
	 * to be merged with the first word of src prepared in t1
	 */
	/* Compute shift depending on byte alignment of dst */
	extru arg1,31,2,t3
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Load the word containing the first byte to replace */
	ldws 0(arg1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd t4,r0,t4		/* t4: the exact word to merge with src */
	
	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,t4,t1
	stws,ma t1,4(arg1)

	/* Flush/Purge first word of src and dst. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	0(arg0)
	ssm	PSW_D,r0
	
	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_rphys_exit

	/* 
	 * Initiate the word copy loop. 
	 * All the bytes of the first src word have not been copied.
	 */
	/* Reload the first word of src */
	ldwas,ma 4(arg0),t1

	/* Purge first word of src. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	ssm	PSW_D,r0

	/* Use the shift between src and dst computed before in ret0 */
	sh3add ret0,r0,t4	/* shift in bits */
	mtctl t4,11		/* store the shift in the Shift Amount Register */

	/* If less than a word to copy, don't enter the loop */
	comib,>,n 4,arg2,$bcopy_rphys_no_loop

$bcopy_rphys_word_loop:
	/* Load the next src word */
	ldwas,ma 4(arg0),t3
	vshd t1,t3,t4
	stws,ma t4,4(arg1)
	addi -4,arg2,arg2	/* decrement the count */
	copy t3,t1		/* prepare the next loop */
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	/* loop if more than a word to copy */
	comib,<= 4,arg2,$bcopy_rphys_word_loop
	ssm	PSW_D,r0

	/* Is there something else to copy */
	comib,>=,n 0,arg2,$bcopy_rphys_exit

$bcopy_rphys_no_loop
	/*
	 * Copy the 1,2 or 3 last bytes in dst.
	 * ret0: the shift between src and dst. t1: the last word of src copied.
	 */
	/* Load the last word of src to partially copy */
	ldwas 0(arg0),t3

	/* Shift the last two words of src */
	vshd t1,t3,t4

	/* Mask the bytes at end of src word that must not be copied */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r0,t4,t4
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd t4,r0,t1		/* t1: the exact last word of src to copy */

	/* Load the last dst word to partially replace */
	ldws 0(arg1),t3

	/* Isolate the bytes at end of dst word that must not be replaced */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t3,r0,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3		/* t3: the last word of dst to merge with src */

	/* Merge the last words of src and dst correctly masked and store it*/ 	
	or t1,t3,t1
	stws t1,0(arg1)

	/* Flush/Purge last word of src and dst */
	rsm	PSW_D,r0
	pdc	0(arg0)
	ssm	PSW_D,r0

	/* The End */
	b,n $bcopy_rphys_exit 

$bcopy_rphys_shift_negative
	/*
	 * The shift is negative (byte shift of src is greater than dst one)
	 * t3: byte shift of src. t2: byte shift of dst.
	 */
	/* Recompute the shift to make it positive */
	sub t3,t2,ret0		/* ret0 = shift between dst and src */
	subi 4,ret0,ret0	/* Adapt the shift to vshd instruction */

	/*
	 * Mask bytes of the first word of src before the first byte to copy
	 * (at least one).
	 */
	/* Compute bit shift */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align src on a word boundary */
	extru arg0,29,30,arg0
	zdep arg0,29,30,arg0

	/* Load the word containing the first byte to copy */
	ldwas,ma 4(arg0),t1
	vshd t1,r0,t1		/* t1: the bytes to copy but aligned on left */

	/* Align the bytes of first word to copy on their original position */
	sh3add t3,r0,t2
	mtctl t2,11
	vshd r0,t1,t1

	/* Purge first word of src. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	ssm	PSW_D,r0

	/* 
	 * Load the second word of src as there are more bytes to replace
	 * in the first word of dst than the number of bytes to copy from
	 * the first word of src
	 */
	ldwas 0(arg0),t3

	/* 
	 * Use the computed shift between src and dst (kept in ret0) to copy
	 * the bytes that the first word of dst needs
	 */
	sh3add ret0,r0,t2
	mtctl t2,11
	vshd t1,t3,t1		/* t1: the exact word to merge in dst */

	/*
	 * Prepare the first word of dst to be merged with the first word
	 * of src prepared in t1.
	 * It may contain 1 or 2 bytes that must not be modified 
	 * by the copy operation
	 */
	/* Check first if dst is already word aligned */
	extru arg1,31,2,t3
	comib,=,n 0,t3,$bcopy_rphys_dst_aligned

	/* Compute shift depending on byte alignment of dst */
	subi 4,t3,t2
	sh3add t2,r0,t2
	mtctl t2,11

	/* Align dst on a word boundary */
	extru arg1,29,30,arg1
	zdep arg1,29,30,arg1

	/* Load the word containing the first byte to replace */
	ldws 0(arg1),t4
	vshd r0,t4,t4		/* t4: the bytes to keep but aligned on right */

	/* Align the bytes to replace on their original position */
	sh3add t3,r0,t2		/* t3: the byte alignment of dst */
	mtctl t2,11
	vshd,tr t4,r0,t4	/* t4: the exact word to merge with src */
				/* Always nullify the following ldi instruction */
$bcopy_rphys_dst_aligned
	/* Clear the register t4 *if* no byte to keep from first dst word */
	ldi 0,t4
	/* Merge the first words of src and dst correctly masked and store it*/ 
	or t1,t4,t1
	stws,ma t1,4(arg1)

	/* Flush/Purge second word of src and first word of dst. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	0(arg0)
	ssm	PSW_D,r0

	/* Decrement the count of bytes to copy */
	subi 4,t3,t2		/* t3: the byte alignment of dst */
	sub arg2,t2,arg2	/* t2: the number of bytes copied in dst word */

	/* Is there something else to do */
	comb,>=,n r0,arg2,$bcopy_rphys_exit

	/* 
	 * Initiate the word copy loop.
	 * All the bytes of the second src word have not been copied.
	 */
	/* Reload the second word of src */
	ldwas,ma 4(arg0),t1

	/* Purge second word of src. r1: contains -4 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	ssm	PSW_D,r0
	
	/* Use the shift between src and dst computed before in ret0 */
	sh3add ret0,r0,t4	/* shift in bits */
	mtctl t4,11		/* store the shift in the Shift Amount Register */

	/* If less than a word to copy, don't enter the loop */
	comib,>,n 4,arg2,$bcopy_rphys_no_loop2

$bcopy_rphys_word_loop2
	/* Load the next src word */
	ldwas,ma 4(arg0),t3
	vshd t1,t3,t4
	stws,ma t4,4(arg1)
	addi -4,arg2,arg2	/* decrement the count */
	copy t3,t1		/* prepare the next loop */
	/*
	 * Since the source and destination are physical addresses, flush
	 * them back to memory. Turn off data translation to do this.
	 */
	rsm	PSW_D,r0
	pdc	r1(arg0)
	/* loop if more than a word to copy */
	comib,<= 4,arg2,$bcopy_rphys_word_loop2
	ssm	PSW_D,r0

	/* Is there something else to copy */
	comib,>=,n 0,arg2,$bcopy_rphys_exit

$bcopy_rphys_no_loop2
	/*
	 * Copy the 1,2 or 3 last bytes in dst.
	 * ret0: the shift between src and dst. 
	 * t1: the last word of src partially copied.
	 */
	/* Check if a last word has to be copied from src */
	ldi 0,t3
	sub,> arg2,ret0,r0
	b,n $bcopy_rphys_no_more_src

	/* Load the last word of src to partially copy *if* not already done */
	ldwas 0(arg0),t3

	/* Purge last word of src */
	rsm	PSW_D,r0
	pdc	0(arg0)
	ssm	PSW_D,r0

$bcopy_rphys_no_more_src
	/* Shift the last word of src by ret0 */
	vshd t1,t3,t4

	/* Mask the bytes at end of src word that must not be copied */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd r0,t4,t4
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd t4,r0,t1		/* t1: the exact last word of src to copy */

	/* Load the last dst word to partially replace */
	ldws 0(arg1),t3

	/* Isolate the bytes at end of dst word that must not be replaced */
	subi 4,arg2,t2
	sh3add t2,r0,t2
	mtctl t2,11
	vshd t3,r0,t3
	sh3add arg2,r0,t2
	mtctl t2,11
	vshd r0,t3,t3		/* t3: the last word of dst to merge with src */

	/* Merge the last words of src and dst correctly masked and store it*/ 	
	or t1,t3,t1
	stws t1,0(arg1)

	/* The End */

$bcopy_rphys_exit
	sync
	bv,n	0(r2)
	.exit
	.procend
