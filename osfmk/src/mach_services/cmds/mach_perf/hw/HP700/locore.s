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
	.subspa $CODE$

/*
 * Copy of kernel routines used to copy and clear pages
 */

/*
 * copy_page(from, to, byte count)
 */
	.export	hw_copy_page,entry
	.proc
	.callinfo 
hw_copy_page

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
	 * bytes then just do it byte by byte. Otherwise, see if the data has 
	 * the same basic alignment. We will add in the byte offset to size to
	 * keep track of what we have to move even though the stbys instruction
	 * won't physically move it. 
	 */

        comib,>= 15,arg2,$bcopy_byte
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
	 * The source and destination are not alligned on the same boundary 
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
	.procend


/*
 * zero_page(char * addr, unsigned int length)
 */
	.export	hw_zero_page,entry
	.proc
	.callinfo 
hw_zero_page

        comb,>=,n r0,arg1,$bzero_exit

	/*
	 * If we need to clear less than a word do it a byte at a time
	 */

	comib,>,n 4,arg1,$bzero_bytes

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
	 * zero by bytes
	 */

$bzero_bytes
        addib,> -1,arg1,$bzero_bytes
        stbs,ma r0,1(arg0) 

$bzero_exit
	bv,n	0(r2)
	.procend

	.end
