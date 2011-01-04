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
 * 	Utah $Hdr: cache.s 1.23 94/12/14$
 */

/*
 * hppa cache and tlb routines
 *
 * fdcache(space, offset, byte_count) 	   - flush data cache
 * ficache(space, offset, byte_count) 	   - flush instruction cache
 * pdcache(space, offset, byte_count) 	   - purge data cache
 * fcacheline(space, offset)		   - flush a line from both caches
 * pcacheline(space, offset)		   - purge a line from both caches
 * fcacheall()				   - flush entire cache for both caches
 * fdcacheall()				   - flush entire data cache
 * pitlbentry(space, offset)		   - purge I-TLB entry (for ptlball)
 * pdtlbentry(space, offset)		   - purge D-TLB entry (for ptlball)
 * pitlb(space, offset)			   - purge I-TLB entry for address
 * pdtlb(space, offset)			   - purge D-TLB entry for address
 * idtlba(page, space, offset)		   - insert data TLB address
 * idtlbp(prot, space, offset)		   - insert data TLB protection
 * iitlba(page, space, offset)		   - insert instruction TLB address
 * iitlbp(prot, space, offset)		   - insert instruction TLB protection
 */

#include <machine/asm.h>

	.space	$PRIVATE$
	.subspa	$DATA$

	.import icache_stride,data
	.import icache_base,data
	.import icache_stride,data
	.import icache_count,data
	.import icache_loop,data

	.import dcache_stride,data
	.import dcache_base,data
	.import dcache_stride,data
	.import dcache_count,data
	.import dcache_loop,data


        .space  $TEXT$
        .subspa $CODE$

/*
 * fdcache(space, offset, byte_count)
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *	int		byte_count;
 *	int		stride;
 *
 * This routine flushes byte_count bytes from the data cache starting
 * at address (space, offset). 
 */

	.export fdcache,entry

        .proc
        .callinfo

#define sp	arg0
#define offset	arg1
#define count	arg2
#define stride	arg3

fdcache

	.entry
	ldil    L%dcache_stride,t1
	ldw     R%dcache_stride(t1),stride

	mtsp	sp,sr1			# move the space register to sr1
	add	offset,count,arg0	# get the last byte to flush in arg0

	zdep	stride,27,28,t1		# get size of a large (16 X) loop in t1
	comb,<  count,t1,fdc_short	# check for count < 16 * stride
	addi	-1,t1,t1		# compute size of large loop - 1

	andcm	count,t1,t1		# L = count - (count mod lenbigloop)
	add	offset,t1,t1		# ub for big loop is lb + L

	fdc,m   stride(sr1,offset)    	# Start flushing first cache line.
fdc_long
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	fdc,m   stride(sr1,offset)
	comb,<<,n offset,t1,fdc_long
	fdc,m   stride(sr1,offset)

fdc_short				# flush one line at a time
	comb,<<,n offset,arg0,fdc_short
	fdc,m   stride(sr1,offset)

	addi	-1,arg0,offset
	fdc	(sr1,offset)

	bv	0(r2)
	sync
	.exit
        .procend

#undef sp	arg0
#undef offset	arg1
#undef count	arg2
#undef stride	arg3


/*
 * ficache(space, offset, byte_count)
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *	int		byte_count;
 *	int		stride;
 *
 * This routine flushes byte_count bytes from the instruction cache starting
 * at address (space, offset).
 */

	.export ficache,entry
        .proc
        .callinfo

#define sp	arg0
#define offset	arg1
#define count	arg2
#define stride	arg3

ficache

	.entry
	ldil    L%icache_stride,t1
	ldw     R%icache_stride(t1),stride

	mtsp	sp,sr1			# move the space register to sr1
	add	offset,count,arg0	# get the last byte to flush in arg0

	zdep	stride,27,28,t1		# get size of a large (16 X) loop in t1
	comb,<  count,t1,fic_short	# check for count < 16 * stride
	addi	-1,t1,t1		# compute size of large loop - 1

	andcm	count,t1,t1		# L = count - (count mod lenbigloop)
	add	offset,t1,t1		# ub for big loop is lb + L

	fic,m   stride(sr1,offset)    	# Start flushing first cache line.
fic_long
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	fic,m   stride(sr1,offset)
	comb,<<,n offset,t1,fic_long
	fic,m   stride(sr1,offset)

fic_short				# flush one line at a time
	comb,<<,n offset,arg0,fic_short
	fic,m   stride(sr1,offset)

	addi	-1,arg0,offset
	fic	(sr1,offset)

	bv	0(r2)
	sync
	.exit
        .procend

#undef sp	arg0
#undef offset	arg1
#undef count	arg2
#undef stride	arg3


/*
 * pdcache(space, offset, byte_count)
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *	int		byte_count;
 *	int		stride;
 *
 * This routine purges byte_count bytes from the data cache starting
 * at address (space, offset).  NOTE: offset and byte_count had better
 * be cache-line aligned or you could purge more than you should!
 */

	.export pdcache,entry
        .proc
        .callinfo

#define sp	arg0
#define offset	arg1
#define count	arg2
#define stride	arg3

pdcache

	.entry
	ldil    L%dcache_stride,t1
	ldw     R%dcache_stride(t1),stride

	mtsp	sp,sr1			# move the space register to sr1
	add	offset,count,arg0	# get the last byte to purge in arg0

	zdep	stride,27,28,t1		# get size of a large (16 X) loop in t1
	comb,<  count,t1,pdc_short	# check for count < 16 * stride
	addi	-1,t1,t1		# compute size of large loop - 1

	andcm	count,t1,t1		# L = count - (count mod lenbigloop)
	add	offset,t1,t1		# ub for big loop is lb + L

	pdc,m   stride(sr1,offset)    	# Start purging first cache line.
pdc_long
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	pdc,m   stride(sr1,offset)
	comb,<<,n offset,t1,pdc_long
	pdc,m   stride(sr1,offset)

pdc_short				# purge one line at a time
	comb,<<,n offset,arg0,pdc_short
	pdc,m   stride(sr1,offset)

	addi	-1,arg0,offset
	pdc	(sr1,offset)

	bv	0(r2)
	sync
	.exit
	.procend

#undef sp	arg0
#undef offset	arg1
#undef count	arg2
#undef stride	arg3


/*
 * fcacheline(space, offset)
 * 	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Flush the data and instruction cache line specified from both data and 
 * instruction caches.
 */

	.export fcacheline,entry
	.proc
	.callinfo
fcacheline      

	.entry
	mtsp    arg0,sr1
	fdc     (sr1,arg1)
	fic     (sr1,arg1)
	bv	0(r2)
	sync
	.exit
	.procend


/*
 * pcacheline(space, offset)
 * 	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Purge the data and instruction cache line specified from both data and 
 * instruction caches.
 */
	.export pcacheline,entry
	.proc
	.callinfo

pcacheline

	.entry
	mtsp    arg0,sr1
	pdc     (sr1,arg1)
	fic     (sr1,arg1)
	bv	0(r2)
	sync
	.exit
	.procend


/*
 * fcacheall()
 *
 * Flush the entire cache.  First it flushes the entire i-cache, then
 * it flushes the entire d-cache, using the code for fdcacheall().
 * The i-cache flushing routine separates the cases when the
 * inner loop executes once (per outer loop) and when it iterates.  The
 * first case is straightforward; the inner loop counting is discarded
 * as a nop.  The second case is slightly more involved.  The inner loop
 * count is pre-adjusted by -1 to perform one fewer than specified fice
 * in the inner loop, then performs a fice,m (post-modify) to do the last
 * fice and adjust the address.
 */

	.export fcacheall,entry
	.proc
	.callinfo save_rp,calls
fcacheall
	.entry
	stw		rp,-20(sp)
	ldo		48(sp),sp

	ldil    	L%icache_base,t1
	ldw     	R%icache_base(t1),t2

	ldil    	L%icache_stride,t1
	ldw     	R%icache_stride(t1),t3

	ldil    	L%icache_loop,t1
	ldw     	R%icache_loop(t1),arg1

	ldil    	L%icache_count,t1
	ldw     	R%icache_count(t1),arg0

	addib,= 	-1,arg1,fialloneloop    # Preadjust and test
	movb,<,n        arg1,t1,fdall   	# If LOOP < 0, no i-cache

fiallmanyloop                                   # Loop if LOOP >= 2
	addib,> 	-1,t1,fiallmanyloop     # Adjusted inner loop decr
	fice    	0(0,t2)
	fice,m  	t3(0,t2)                # Last fice and addr adjust
	movb,tr 	arg1,t1,fiallmanyloop 	# Re-init inner loop count
	addib,<,n	-1,arg0,fdall           # Outer loop decr

fialloneloop                                    # Loop if LOOP = 1
	addib,>         -1,arg0,fialloneloop    # Outer loop count decr
	fice,m          t3(0,t2)                # Fice for one loop

fdall
	.import fdcacheall,code
	.call
	bl		fdcacheall,rp		# call fdcacheall to flush
	nop					# the data cache

	ldw		-68(sp),rp
	bv		0(rp)
	ldo		-48(sp),sp
	.exit
	.procend


/*
 * fdcacheall()
 *
 * Flush the entire dcache.  This routine separates the cases when the
 * inner loop executes once (per outer loop) and when it iterates.  The
 * first case is straightforward; the inner loop counting is discarded
 * as a nop.  The second case is slightly more involved.  The inner loop
 * count is pre-adjusted by -1 to perform one fewer than specified fdce
 * in the inner loop, then performs a fdce,m (post-modify) to do the last
 * fdce and adjust the address.
 */

	.export fdcacheall,entry
	.proc
	.callinfo

fdcacheall

	.entry
	ldil    	L%dcache_base,t1
	ldw     	R%dcache_base(t1),t2

	ldil    	L%dcache_stride,t1
	ldw     	R%dcache_stride(t1),t3

	ldil    	L%dcache_loop,t1
	ldw     	R%dcache_loop(t1),arg1

	ldil    	L%dcache_count,t1
	ldw     	R%dcache_count(t1),arg0

	addib,= 	-1,arg1,fdalloneloop    # Preadjust and test
	movb,<,n        arg1,t1,fdallcleanup

fdallmanyloop                                   # Loop if LOOP >= 2
	addib,> 	-1,t1,fdallmanyloop     # Adjusted inner loop decr
	fdce    	0(0,t2)
	fdce,m          t3(0,t2)                # Last fdce and addr adjust
	movb,tr         arg1,t1,fdallmanyloop 	# Re-init inner loop count
	addib,<,n       -1,arg0,fdallcleanup    # Outer loop decr

fdalloneloop                                    # Loop if LOOP = 1
	addib,>         -1,arg0,fdalloneloop    # Outer loop count decr
	fdce,m          t3(0,t2)                # Fdce for one loop

fdallcleanup
	bv	0(r2)
	sync
	.exit

	.procend


/*
 * pitlbentry(space, offset)
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Purge an ITLB entry by index.  Used in purge_all_tlbs().
 */

	.export pitlbentry,entry
	.proc
	.callinfo
pitlbentry

	.entry
	mtsp    arg0, sr1
	bv	0(r2)
	pitlbe  r0(sr1,arg1)
	.exit
	.procend


/*
 * pdtlbentry(space, offset)
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Purge an DTLB entry by index.  Used in purge_all_tlbs().
 */

	.export pdtlbentry,entry
	.proc
	.callinfo

pdtlbentry

	.entry
	mtsp    arg0, sr1
	bv	0(r2)
	pdtlbe  r0(sr1,arg1)
	.exit
	.procend


/*
 * pitlb(space, offset)
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Purge an ITLB entry by address.  Used by pmap module.
 */

	.export pitlb,entry
	.proc
	.callinfo
pitlb
	.entry
	mtsp	arg0, sr1
	bv	0(r2)
	pitlb	r0(sr1,arg1)
	.exit
	.procend


/*
 * pdtlb(space, offset)
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Purge an DTLB entry by address.  Used by pmap module.
 */

	.export pdtlb,entry
	.proc
	.callinfo
pdtlb

	.entry
	mtsp	arg0, sr1
	bv	0(r2)
	pdtlb	r0(sr1,arg1)
	.exit
	.procend


/*
 * idtlba(page, space, offset)
 *	vm_offset_t	page;
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Insert a TLB address in the data TLB
 */

	.export idtlba,entry
	.proc
	.callinfo

idtlba
	.entry
	mtsp    arg1, sr1
	bv	0(r2)
	idtlba  arg0,(sr1,arg2)
	.exit
	.procend


/*
 * idtlbp(prot, space, offset)
 *	prot_t		prot;
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Insert a TLB protection in the data TLB
 */

	.export idtlbp,entry
	.proc
	.callinfo

idtlbp

	.entry
	mtsp    arg1, sr1
	bv	0(r2)
	idtlbp  arg0,(sr1,arg2)
	.exit
	.procend


/*
 * iitlba(page, space, offset)
 *	vm_offset_t	page;
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Insert a TLB address in the instruction TLB
 */

	.export iitlba,entry
	.proc
	.callinfo

iitlba
	.entry
	mtsp    arg1, sr1
	bv	0(r2)
	iitlba  arg0,(sr1,arg2)
	.exit
	.procend


/*
 * iitlbp(prot, space, offset)
 *	prot_t		prot;
 *	vm_space_t	space;
 *	vm_offset_t	offset;
 *
 * Insert a TLB protection in the instruction TLB
 */

	.export iitlbp,entry
	.proc
	.callinfo

iitlbp

	.entry
	mtsp    arg1, sr1
	bv	0(r2)
	iitlbp  arg0,(sr1,arg2)
	.exit
	.procend

/*
 * The following code is covered by the CMU copyright.
 */
/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <machine/pmap.h>
#include <machine/psw.h>

#define PSW_RI	PSW_R|PSW_I

/*
 *	standard entry exit code for leaf procedure
 */	
#define LEAF_ENTRY(_name_)		;\
	.export _name_,entry		;\
_name_ 					;\
	.proc 				;\
	.callinfo 			;\
	.entry

#define LEAF_EXIT 			;\
	.exit 				;\
	.procend

	.code

/*
 *	mtcpu - move to cpu register

mtcpu	.macro greg,dreg
	{0..5}=0x05{26..31}
	{6..10}=dreg{27..31}
	{11..15}=greg{27..31}
	{16..18}=0x0{29..31}
	{19..26}=0xb0{24..31}
	{27..31}=0{27..31}
	.endm
*/
#define mtcpu(greg,dreg)\
 .word ((0x5 << 26) | ((dreg & 0x1f) << 21) | (greg & 0x1f) << 16 | 0x1600)
#define mtcpu2(greg,dreg)\
 .word ((0x5 << 26) | ((dreg & 0x1f) << 21) | (greg & 0x1f) << 16 | 0x0240)

/*
 *	mfcpu - move from cpu register 0

mfcpu	.macro dreg,greg
	{0..5}=0x05{26..31}
	{6..10}=dreg{27..31}
	{11..15}=greg{27..31}
	{16..18}=0x0{29..31}
	{19..26}=0xd0{24..31}
	{27..31}=0{27..31}
	.endm
 */

#define mfcpu(dreg,greg)\
 .word ((0x5 << 26) | ((dreg & 0x1f) << 21) | (greg & 0x1f) << 16 | 0x1a00)
#define mfcpu2(dreg,greg)\
 .word ((0x5 << 26) | ((dreg & 0x1f) << 21) | (greg & 0x1f) << 16 | 0x0600)
#define dtlb_reg	8
#define itlb_reg	9
#define itlb_size_even	24
#define itlb_size_odd	25
#define dtlb_size_even	26
#define dtlb_size_odd	27

/*
 * disable_S_sid_hashing(void)
 *
 * Disable SID hashing and flush all caches for S-CHIP
 *
 */
LEAF_ENTRY(disable_S_sid_hashing)
	mfcpu	(0,t1)			# get cpu diagnosic register
	mfcpu	(0,t1)			# black magic
	copy	t1,ret0
	depi	0,20,3,t1		# clear DHE, domain and IHE bits
	depi	1,16,1,t1		# enable quad-word stores
	depi	0,10,1,t1		# do not clear the DHPMC bit
	depi	0,14,1,t1		# do not clear the ILPMC bit
	mtcpu	(t1,0)			# set the cpu disagnostic register
	mtcpu	(t1,0)			# black magic
	bv,n	0(r2)
	LEAF_EXIT

/*
 * disable_T_sid_hashing(void)
 *
 * Disable SID hashing and flush all caches for T-CHIP
 *
 */
LEAF_ENTRY(disable_T_sid_hashing)
	mfcpu	(0,t1)			# get cpu diagnosic register
	mfcpu	(0,t1)			# black magic
	copy	t1,ret0
	depi	0,18,1,t1		# clear DHE bit
	depi	0,20,1,t1		# clear IHE bit
	depi	0,10,1,t1		# do not clear the DHPMC bit
	depi	0,14,1,t1		# do not clear the ILPMC bit
	mtcpu	(t1,0)			# set the cpu disagnostic register
	mtcpu	(t1,0)			# black magic
	bv,n	0(r2)
	LEAF_EXIT

/*
 * disable_L_sid_hashing(void)
 *
 * Disable SID hashing and flush all caches for L-CHIP
 *
 */
LEAF_ENTRY(disable_L_sid_hashing)
	mfcpu2	(0,t1)			# get cpu diagnosic register 
/* .word	0x14160600 */
	copy	t1,ret0
	depi	0,27,1,t1		# clear DHE bit
	depi	0,28,1,t1		# clear IHE bit
	depi	0,6,1,t1		# do not clear the L2IHPMC bit
	depi	0,8,1,t1		# do not clear the L2DHPMC bit
	depi	0,10,1,t1		# do not clear the L1IHPMC bit
	mtcpu2	(t1,0)			# set the cpu disagnostic register 
/* .word	0x14160240 */
	bv,n	0(r2)
	LEAF_EXIT

LEAF_ENTRY(get_dcpu_reg)
	mfcpu	(0,t1)			# Get cpu diagnostic register
	mfcpu	(0,t1)			# black magic
	copy t1,ret0
	bv,n	0(r2)
	LEAF_EXIT

/*
 *	insert block entries into data tlb (S-CHIP only)
 *
 *	arguments : 
 *		arg0 : unsigned  index
 *		arg1 : vm_offset_t address
 *		arg2 : vm_size_t size
 *		arg3 : vm_prot_t protection
 *
 *	Index must be 0,1,2 or 3. Address must be aligned to size. Size must
 *	be a power of two between 256k and 16M inclusive. Protection is in
 *	the standard PA format. The mapping created is an equivalent mapping.
 */
	.align	4096			# routine must fit on one page
LEAF_ENTRY(insert_block_dtlb)
	rsm	PSW_RI,t4		# can't have interrupts with diag inst
	ldil	L%0x7c40,t1		# index > 119, lockin, override
	ldo	R%0x7c40(t1),t1	
	addi	27,arg0,t2		# get block entry select bit
	mtctl	t2,sar			# put into the shift amount register
	vdepi	1,1,t1			# set the block enter select bit
	mtcpu	(t1,dtlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,dtlb_reg)		# black magic
	mtsp	r0,sr1			# kernel space only
	shd	r0,arg1,7,t2		# get physical address in right format
	or	r0,arg3,t3
	depi	1,3,1,t3		# set dirty bit in protection
	idtlba	t2,(sr1,arg1)		# insert address
	idtlbp	t3,(sr1,arg1)		# insert protection
	zdepi	-1,24,7,t1		# index = 127 no lockin, no override
	mtcpu	(t1,dtlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,dtlb_reg)		# black magic
	subi	0,arg2,t2		# complement size mask
	bb,<	arg0,31,odd_block_dtlb	# branch if dtlb slot 1 or 3
	extru	t2,15,8,t2		# get size into proper format
	mtcpu	(t2,dtlb_size_even)	# move into size register
	mtcpu	(t2,dtlb_size_even)	# black magic
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
odd_block_dtlb
	mtcpu	(t2,dtlb_size_odd)	# move into size register
	mtcpu	(t2,dtlb_size_odd)	# black magic
exit_insert_dtlb
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
	LEAF_EXIT


/*
 *	insert block entries into instruction tlb (S-CHIP only)
 *	arguments : 
 *		arg0 : unsigned  index
 *		arg1 : vm_offset_t address
 *		arg2 : vm_size_t size
 *		arg3 : vm_prot_t protection
 *
 *	Index must be 0,1,2 or 3. Address must be aligned to size. Size must
 *	be a power of two between 256k and 16M inclusive. Protection is in
 *	the standard mach format. The mapping created is an equivalent mapping.
 */
LEAF_ENTRY(insert_block_itlb)
	rsm	PSW_RI,t4		# can't have interrupts with diag inst
	ldil	L%0x7040,t1		# index > 95, lockin, override
	ldo	R%0x7040(t1),t1	
	addi	27,arg0,t2		# get block entry select bit
	mtctl	t2,sar			# put into the shift amount register
	vdepi	1,1,t1			# set the block enter select bit
	mtcpu	(t1,itlb_reg)		# move to the itlb diagnostic register
	mtcpu	(t1,itlb_reg)		# black magic
	mtsp	r0,sr1			# kernel space only
	shd	r0,arg1,7,t2		# get physical address in right format
	or	r0,arg3,t3
	iitlba	t2,(sr1,arg1)		# insert address
	iitlbp	t3,(sr1,arg1)		# insert protection
	zdepi	3,19,2,t1		# index > 95, no lockin, no override
	mtcpu	(t1,itlb_reg)		# move to the itlb diagnostic register
	mtcpu	(t1,itlb_reg)		# black magic
	subi	0,arg2,t2		# complement size mask
	bb,<	arg0,31,odd_block_itlb	# branch if itlb slot 1 or 3
	extru	t2,15,8,t2		# get size into proper format
	mtcpu	(t2,itlb_size_even)	# move into size register
	mtcpu	(t2,itlb_size_even)	# black magic
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
odd_block_itlb
	mtcpu	(t2,itlb_size_odd)	# move into size register
	mtcpu	(t2,itlb_size_odd)	# black magic
exit_insert_itlb
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
	LEAF_EXIT

/*
 *	purge block entry from data tlb (S-CHIP only)
 *
 *	arguments : 
 *		arg0 : unsigned  index
 */
LEAF_ENTRY(purge_block_dtlb)
	rsm	PSW_RI,t4		# can't have interrupts with diag inst
	ldil	L%0xffc0,t1		# index 127, lockin, override, mismatch
	ldo	R%0xffc0(t1),t1	
	addi	27,arg0,t2		# get block entry select bit
	mtctl	t2,sar			# put into the shift amount register
	vdepi	1,1,t1			# set the block enter select bit
	mtcpu	(t1,dtlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,dtlb_reg)		# black magic
	mtsp	r0,sr1			# kernel space only
	idtlba	r0,(sr1,r0)		# address does not matter
	idtlbp	r0,(sr1,r0)
	zdepi	3,19,2,t1		# index 120, no lockin, no override
	mtcpu	(t1,dtlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,dtlb_reg)		# black magic
	bb,<,n	arg0,31,odd_purge_dtlb	# branch if dtlb slot 1 or 3
	nop				# can not nullify a diagnostic inst
	mtcpu	(r0,dtlb_size_even)	# move into size register
	mtcpu	(r0,dtlb_size_even)	# black magic
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
odd_purge_dtlb
	mtcpu	(r0,dtlb_size_odd)	# move into size register
	mtcpu	(r0,dtlb_size_odd)	# black magic
exit_purge_dtlb
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
	LEAF_EXIT

/*
 *	purge block entry from instruction tlb (S-CHIP only)
 *
 *	arguments : 
 *		arg0 : unsigned  index
 */
LEAF_ENTRY(purge_block_itlb)
	rsm	PSW_RI,t4		# can't have interrupts with diag inst
	ldil	L%0xffc0,t1		# index 127, lockin, override, mismatch
	ldo	R%0xffc0(t1),t1	
	addi	27,arg0,t2		# get block entry select bit
	mtctl	t2,sar			# put into the shift amount register
	vdepi	1,1,t1			# set the block enter select bit
	mtcpu	(t1,itlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,itlb_reg)		# black magic
	mtsp	r0,sr1			# kernel space only
	iitlba	r0,(sr1,r0)		# address does not matter
	iitlbp	r0,(sr1,r0)
	zdepi	-1,24,7,t1		# index = 127 no lockin, no override
	mtcpu	(t1,itlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,itlb_reg)		# black magic
	bb,<,n	arg0,31,odd_purge_itlb	# branch if dtlb slot 1 or 3
	nop				# can not nullify a diagnostic inst
	mtcpu	(r0,itlb_size_even)	# move into size register
	mtcpu	(r0,itlb_size_even)	# black magic
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
odd_purge_itlb
	mtcpu	(r0,itlb_size_odd)	# move into size register
	mtcpu	(r0,itlb_size_odd)	# black magic
exit_purge_itlb
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
	LEAF_EXIT

/*
 * End of CMU covered code.
 */

/*
 *	purge block entry from a combined (Instruction and Data tlb)
 *      WARNING: this routine is T-CHIP specific
 *
 *	arguments : 
 *		arg0 : unsigned  index
 */
LEAF_ENTRY(purge_block_ctlb)
	rsm	PSW_RI,t4		# can't have interrupts with diag inst
	ldil	L%0xffc1,t1		# index 127, lockin, override, mismatch
	ldo	R%0xffc1(t1),t1	
	dep	arg0,30,4,t1		# set the block enter select bit
	mtcpu	(t1,dtlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,dtlb_reg)		# black magic
	mtsp	r0,sr1			# kernel space only
	idtlba	r0,(sr1,r0)		# address does not matter
	idtlbp	r0,(sr1,r0)
	zdepi	-1,24,7,t1		# index = 127 no lockin, no override
	mtcpu	(t1,dtlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,dtlb_reg)		# black magic
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
	LEAF_EXIT

/*
 *	insert block entries into combined tlb (T-CHIP only)
 *
 *	arguments : 
 *		arg0 : unsigned  index
 *		arg1 : vm_offset_t address
 *		arg2 : vm_size_t size
 *		arg3 : vm_prot_t protection
 *
 *	Index must be 0-15. Address must be aligned to size. Size must
 *	be a power of two between 512k and 64M inclusive. Protection is in
 *	the standard PA format. The mapping created is an equivalent mapping.
 */
LEAF_ENTRY(insert_block_ctlb)
	rsm	PSW_RI,t4		# can't have interrupts with diag inst
	ldil	L%0x7fc1,t1		# index 127, lockin, override
	ldo	R%0x7fc1(t1),t1	
        dep     arg0,30,4,t1            # set the block enter select bit
	mtcpu	(t1,dtlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,dtlb_reg)		# black magic
	mtsp	r0,sr1			# kernel space only
	shd	r0,arg1,7,t2		# get physical address in correct format
					# arg1 contains virtual address
					# t2 contains the physical address
	subi	0,arg2,t3               # form the complement of the size
	extru	t3,12,7,t3		# extract the 7 bits we care about
	ldi 	19,t1			# bit position in the phy and virtual
	mtctl	t1,sar			# tlb entries for storing the size mask
	vdep	t3,7,t2			# insert size mask into physical addr
	vdep    t3,7,arg1		# insert size mask into virtual addr
	or	r0,arg3,t3
	depi	1,3,1,t3		# set dirty bit in protection
	idtlba	t2,(sr1,arg1)		# insert address
	idtlbp	t3,(sr1,arg1)		# insert protection
	zdepi	-1,24,7,t1		# index = 127 no lockin, no override
	mtcpu	(t1,dtlb_reg)		# move to the dtlb diagnostic register
	mtcpu	(t1,dtlb_reg)		# black magic
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
	LEAF_EXIT

/*
 *	purge block entry from a combined (Instruction and Data tlb)
 *      WARNING: this routine is L-CHIP specific
 *
 *	arguments : 
 *		arg0 : unsigned  index
 */
LEAF_ENTRY(purge_L_block_ctlb)
	rsm	PSW_RI,t4		# can't have interrupts with diag inst
	ldil	L%0xc041,t1		# lockout, PE force-insert disable,
	ldo	R%0xc041(t1),t1		#   PE LRU-ins dis, BE force-ins enable
	dep	arg0,30,3,t1		# set the block enter select bit
	mtcpu2	(t1,dtlb_reg)		# move to the dtlb diagnostic register 
/* .word  0x15160240 */
	mtsp	r0,sr1			# kernel space only
	idtlba	r0,(sr1,r0)		# address does not matter
	idtlbp	r0,(sr1,r0)
	zdepi	-1,18,1,t1		# no lockout, PE force-ins disable
	mtcpu2	(t1,dtlb_reg)		# move to the dtlb diagnostic register 
/* .word  0x15160240 */
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
	LEAF_EXIT

/*
 *	insert block entries into combined tlb (L-CHIP only)
 *
 *	arguments : 
 *		arg0 : unsigned  index
 *		arg1 : vm_offset_t address
 *		arg2 : vm_size_t size
 *		arg3 : vm_prot_t protection
 *
 *	Index must be 0-7. Address must be aligned to size. Size must
 *	be a power of two between 512k and 64M inclusive. Protection is in
 *	the standard PA format. The mapping created is an equivalent mapping.
 */
LEAF_ENTRY(insert_L_block_ctlb)
	rsm	PSW_RI,t4		# can't have interrupts with diag inst
	ldil	L%0x6041,t1		# lockin, PE force-insert disable,
	ldo	R%0x6041(t1),t1		#   PE LRU-ins dis, BE force-ins enable
        dep     arg0,30,3,t1            # set the block enter select bit
	mtcpu2	(t1,dtlb_reg)		# move to the dtlb diagnostic register 
/* .word  0x15160240 */
	mtsp	r0,sr1			# kernel space only
	shd	r0,arg1,7,t2		# get physical address in correct format
					# arg1 contains virtual address
					# t2 contains the physical address
	subi	0,arg2,t3               # form the complement of the size
	extru	t3,12,7,t3		# extract the 7 bits we care about
	ldi 	19,t1			# bit position in the phy and virtual
	mtctl	t1,sar			# tlb entries for storing the size mask
	vdep	t3,7,t2			# insert size mask into physical addr
	vdep    t3,7,arg1		# insert size mask into virtual addr
	or	r0,arg3,t3
	depi	1,3,1,t3		# set dirty bit in protection
	idtlba	t2,(sr1,arg1)		# insert address
	idtlbp	t3,(sr1,arg1)		# insert protection
	zdepi	-1,18,1,t1		# no lockin, PE force-ins disable
	mtcpu2	(t1,dtlb_reg)		# move to the dtlb diagnostic register
/*  .word  0x15160240 */
	bv	0(r2)
	mtsm	t4			# reset system mask to allow intrrupts
	LEAF_EXIT
