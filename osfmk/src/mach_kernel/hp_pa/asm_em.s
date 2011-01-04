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
 *  (c) Copyright 1991 HEWLETT-PACKARD COMPANY
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

/* $Source: /u1/osc/rcs/mach_kernel/hp_pa/asm_em.s,v $
 * $Revision: 1.1.13.1 $	$Author: bruel $
 * $State: Exp $   	$Locker:  $
 * $Date: 1997/06/02 17:01:03 $
 */

/*
 *	A general framework for the handling of coprocessors and
 *	special function units for the Series 800 processor when
 *	they are not present:
 *
 *	Coprocessor and SFU instructions are first decoded to specially
 *	process unit zero instructions for the multiply/divide SFU and the
 *	floating-point coprocessor.  Within this catagory the load and store
 *	coprocessor instructions are emulated without enabling interrupts
 *	while longer functional instructions do save the PC queue and allow
 *	nesting of interrupts.
 *
 *
 *	A load/store type of instruction will attempt to do the
 *	load or store, and give other traps for TLB miss, privilege
 *	violation, etc.  Only if there is no other trap, will the hardware
 *	cause the emulation trap.  This means that we don't have to worry
 *	about privilege and TLB's for the actual load/store in an
 *	uni-processor environment.  For MP,..............................
 *	If address modification is used, the modification will not be
 *	completed beforehand. The trap causes loading the offending
 *	instruction into the IIR (interrupt instruction register), and the
 *	location of the load/store target in the interrupt space and offset
 *	registers (ISR and IOR).  The PC queue is frozen.
 *
 *	The temp registers may be shared to some extent with the TLB
 *	miss handler to the degree that the TLB handler doesn't depend
 *	on their value between misses.
 */

#include "machine/asm_em.h"
#include "machine/asm.h"
#include "machine/trap.h"
#include "machine/locore.h"

#define RFPUREGISTER R'$fpu_register
#ifdef MDSFU_SUPPORTED
#define RMDSFUREGISTER R'$mdsfu_register
#endif

#ifdef AVPS
#define THANDLER(arg) break
#define RFPUREGISTER R'$fpu_register
#define RMDSFUREGISTER R'$mdsfu_register
#endif AVPS

SAR	.equ		11
IVA	.equ		14
PCSQ	.equ		17
PCOQ	.equ		18
IIR	.equ		19
ISR	.equ		20
IOR	.equ		21
IPSW	.equ		22
TR2	.equ		26
TR3	.equ		27
TR4	.equ		28
TR5	.equ		29
TR6	.equ		30
TR7	.equ		31

#undef Skip
#undef Bbset
#undef Bbclr

#define Bbset	bb,<
#define Bbclr	bb,>=
#define Skip    or,=   r0,r0,r0		#Unconditional skip

# Interrupt PSW definitions
    #Nullify bit in PSW or IPSW.
#define pswnullpos	10
    #Real/Virtual addressing bit in PSW.  'ssm' and 'rsm' use
    # 'datatrans' and the instructions that specify the bit position
    # use 'pswdbitpos'.
#define pswdbitpos	30
#define datatrans	2

# Coprocessor and SFU instruction decoding definitions
#define copropbit	2
#define spopcopropbit	3
#define dblwdbit	4
#define loadstorebit	5

# Floating-point Coprocessor instruction decoding definitions

# Floating-point Coprocessor load/store instruction decoding definitions
#define basepos		10
#define indexpos	15
#define ubitpos		18
#define shortindexbit	19
#define slbit		22
#define modifybit	26
#define rsvrdcopbit	27

# Floating-point Coprocessor operation instruction decoding definitions
#define fpr1pos		10
#define fpr2pos         15
#define fpop		22
#define fptpos		31

# SFU instruction decoding definitions
#define spopr1orr2pos	10
#define spopr1onlypos	15
#define spoptpos	31
#define spopoppos	20
#define spop2or3bit	21
#define spop1or3bit	22
#define spopnulbit	26
#define rsvrdsfubit	16

# FP Coprocessor status register definitions
#define tbitpos		25
#define ccbitpos	5
#define versionpos	20
#define modelpversionlen 11

# The software emulation has reserved model 0.
#  The version has been changed to 2 for the multiple exception
#  register bug fix.
#define VERSION 2

# The reserved fields need to be cleared in the floating-point status register.
#define rsv1pos     9
#define rsv1len     4
#define rsv2pos    24
#define rsv2len     2
#define rsv3pos    26
#define rsv3len     1
#define rsv2and3len 4

# FP Coprocessor exception register equates.  Used instead of a define
#  since its use is in the shadow of a quote (ex ldil  L'reservedop,...).

#The exception field is the left 6 bits of the exception register.
#This is can always be done with a 'depi' or 'zdepi' instruction.
#define exceptionpos	5
#define exceptionsize	6
#define UNIMPLEMENTEDEXCEPTION 1

#Trap vector numbers.  Used by the hardware to vector through the
# interrupt vector control register.
#define assistsexception	14
#define datamemoryprotection	18
#define assistsemulation	22

# Definitions for the special C language stack frame used by the 
# non-interruptable semantic routines.
#define SPECIAL_FRAME_SIZE 16*4+48

#define SPECIAL_FRAME_R28 -16*4-48
#define SPECIAL_FRAME_R27 -15*4-48
#define SPECIAL_FRAME_R26 -14*4-48
#define SPECIAL_FRAME_R25 -13*4-48
#define SPECIAL_FRAME_R24 -12*4-48
#define SPECIAL_FRAME_R23 -11*4-48
#define SPECIAL_FRAME_R22 -10*4-48
#define SPECIAL_FRAME_R21 -9*4-48
#define SPECIAL_FRAME_R20 -8*4-48
#define SPECIAL_FRAME_R19 -7*4-48
#define SPECIAL_FRAME_R2  -6*4-48
#define SPECIAL_FRAME_R1  -5*4-48
#define SPECIAL_FRAME_SAR -4*4-48
#define SPECIAL_FRAME_IR  -3*4-48
#define SPECIAL_FRAME_SAVE_REG_PTR   -2*4-48
#define SPECIAL_FRAME_SAVE_ISTACKPTR -1*4-48

# Misc. definitions
#define times8		28
#define b0pos		0
#define b29pos		29
#define b30pos		30
#define b31pos		31
#define fcnvbit		15

#ifdef TIMEX
#define righthalfbit	29
#endif

uidpos		.equ	25
dblwrdbit	.equ	4

#******************
# Imports.
#******************
	.space	$TEXT$
	.subspa	$CODE$
#ifdef MDSFU_SUPPORTED
    .import mdrr
    .import divsir
    .import divsim
    .import divuir
    .import divsfr
    .import divsfm
    .import divufr
    .import mpys
    .import mpyu
    .import mpyscv
    .import mpyaccs
    .import mpyucv
    .import mpyaccu
#endif

    .import dbl_fcmp
    .import sgl_fcmp
    .import dbl_fmpy
    .import sgl_fmpy
    .import dbl_fdiv
    .import sgl_fdiv
    .import dbl_frnd
    .import sgl_frnd
    .import sgl_fadd
    .import sgl_fsub
    .import dbl_fadd
    .import dbl_fsub
    .import dbl_frem
    .import sgl_frem
    .import dbl_fsqrt
    .import sgl_fsqrt

    .import sgl_to_dbl_fcnvff
    .import dbl_to_sgl_fcnvff

    .import sgl_to_sgl_fcnvxf
    .import sgl_to_dbl_fcnvxf
    .import dbl_to_sgl_fcnvxf
    .import dbl_to_dbl_fcnvxf

    .import sgl_to_sgl_fcnvfx
    .import sgl_to_dbl_fcnvfx
    .import dbl_to_sgl_fcnvfx
    .import dbl_to_dbl_fcnvfx

    .import sgl_to_sgl_fcnvfxt
    .import sgl_to_dbl_fcnvfxt
    .import dbl_to_sgl_fcnvfxt
    .import dbl_to_dbl_fcnvfxt

#******************
# Exports.
#******************
    .export $nonzerohandler,entry,priv_lev = 0
    .import emfpudispatch
    .export ftest,entry,priv_lev = 0

#****************************************************************************
# IVA entrypoint for handling SFU/Coprocessor emulation traps.
#
# This is entered from the interrupt vector table code.  Register 31 has
# been saved in TR2.
#****************************************************************************
    .export $emulation_trap,priv_lev = 0
#*****************************************************************************
# Start of Multply/divide SFU and floating-point coprocessor decode.
#
# First, separate out floating-point load/storeinstructions
#
#*****************************************************************************
$emulation_trap

    mfctl    iir,r31		#Grab the interrupted instruction.
    mtctl   r30,tr3		#save r30

    extru,<> r31,loadstorebit,1,r0	# check for load/store instruction
    b,n        mdfpop		# not load or store if bit 5 == 0

    extru    r31,uidpos,3,r30	#only uid 0 and 1 is valid for load/stores
    comib,<<,n 1,r30,$nonzerohandler

#ifdef TIMEX
    extru    r31,5,6,r30		# get major op
    comibt,=,n 0x0f,r30,quadstore	# check for quad store op
#endif
    extru,<> r31,dblwrdbit,1,r0 #check for single/double load store if not
				# a quad store
    b,n	    notdblwd

dblwd				#Instruction is one of the floating-point
				#double-word instructions.
    mtctl   r27,TR6
    ldil    L'$fpu_register,r27
    ldw     RFPUREGISTER(0,r27),r30	#Get the status reg to check t-bit

# In the mean time check for reserved operand indicated by specifying
# floating-point registers 16-31.  And save away another register.
# NOTE: When the architecture is extended to allow FPR[16..31] this check
#       will have to be modified.

#ifndef TIMEX
    Bbset   r31,rsvrdcopbit,causeresopexception
#endif
    mtctl   r29,TR4
    mtctl   r28,TR5

    Bbclr   r31,slbit,fldd	#Separate loads from stores.
    dep     r31,times8,5,r27	#Isolate and scale source reg spec.  This is
				#injected into the left part of the address of
				#$fpu_register


#*****************************************************************************
# FP coprocessor store double-word.
#
# Register contents upon entry: r1-r26,  Original contents.
#				r27,     L'$fpu_register+(8*reg spec)
#				r28,     Saved, but not used.
#				r29,     Saved, but not used.
#				r30,     FP Status Register
#				r31,     Trapping Instruction
#*****************************************************************************
fstd
    #T-bit is special since fstd can cancel a trap.
    Bbset,n r30,tbitpos,maybecauseexception
    ldw     RFPUREGISTER(0,r27),r30
fstdjoin
    extru,= r31,fptpos,5,r0		 #Check for the status register.
    b       fstdnot0
    ldw     RFPUREGISTER+4(0,r27),r31    #Pick up the FP register contents.
# Set the appropriate model and version and clear the reserved fields.
#ifndef TIMEX
    depi    VERSION,versionpos,modelpversionlen,r30
#endif
    depi    0,rsv1pos,rsv1len,r30
    depi    0,rsv2pos,rsv2len,r30
    depi    0,rsv3pos,rsv3len,r30
fstdnot0

#ifndef PCXBUG
    mfsp    sr1,r28		#Save space register.
    mfctl   ISR,r29
    mtsp    r29,sr1		#Point to user's data space
#endif
    mfctl   IPSW,r29
    depi    1,pswnullpos,1,r29  #Set nullify bit in interrupt PSW
#ifdef PCXBUG
pcxbug11
    mtctl   r29,IPSW
    mfctl   IPSW,r28
    combf,=,n r28,r29,pcxbug11
#endif
    mtctl   r29,IPSW
    extru,= r29,pswdbitpos,1,r0 #Is data translation enabled?
    ssm     datatrans,r0        #Yes.
#ifdef PCXBUG
    mfsp    sr1,r28             #Save space register.
    mfctl   ISR,r29
    mtsp    r29,sr1             #Point to user's data space
#endif
    mfctl   IOR,r29		#Point to user's data offset
    extru,= r29,b31pos,3,r0     #Is the address Aligned?
    b,n     doubleunaligned     # No Cause data memory protect trap.
    stw	    r30,0(sr1,r29)
    stw     r31,4(sr1,r29)	#Store FP dbl-reg into user's location.
    # No need to turn off translation since there are no further memory
    # references before returning from the interrupt.

    mfctl   TR6,r27
    # Discover if base register is modified, and emulate if necessary.
    mfctl   IIR,r31		#Get another copy of the instruction
    mtsp    r28,sr1		#Restore space
    Bbset   r31,modifybit,modifybase
    mfctl   TR5,r28

    # Otherwise, restore everthing and return
    mfctl   TR4,r29
    mfctl   TR3,r30
    mfctl   TR2,r31
    rfi
    nop

#*****************************************************************************
# FP coprocessor load double-word.
#
# Register contents upon entry: r1-r26,  Original contents.
#				r27,     L'$fpu_register+(8*reg spec)
#				r28,     Saved, but not used.
#				r29,     Saved, but not used.
#				r30,     FP Status Register
#				r31,     Trapping Instruction
#*****************************************************************************
fldd
    #Test for the T-bit being set.
    Bbset,n r30,tbitpos,causeexception
#ifndef PCXBUG
    mfsp    sr1,r28		#Save space register.
    mfctl   ISR,r29
    mtsp    r29,sr1		#Point to user's data space
#endif
    mfctl   IPSW,r29
    depi    1,pswnullpos,1,r29  #Set nullify bit in interrupt PSW
#ifdef PCXBUG
pcxbug13
    mtctl   r29,IPSW
    mfctl   IPSW,r28
    combf,=,n r28,r29,pcxbug13
#endif
    mtctl   r29,IPSW
    extru,= r29,pswdbitpos,1,r0 #Is data translation enabled?
    ssm     datatrans,r0        #Yes.
#ifdef PCXBUG
    mfsp    sr1,r28             #Save space register.
    mfctl   ISR,r29
    mtsp    r29,sr1             #Point to user's data space
#endif
    mfctl   IOR,r29		#Point to user's data offset
    extru,= r29,b31pos,3,r0     #Is the address Aligned?
    b,n     doubleunaligned     # No Cause data memory protect trap.
    ldw	    0(sr1,r29),r30
    ldw     4(sr1,r29),r31	#Store FP dbl-reg into user's location.
    rsm	    datatrans,r0	#Turn off data translation
    stw	    r30,RFPUREGISTER(0,r27)
    stw     r31,RFPUREGISTER+4(0,r27) #Pick up the FP register contents.

    mfctl   TR6,r27
    # Discover if base register is modified, and emulate if necessary.
    mfctl   IIR,r31		#Get another copy of the instruction
    mtsp    r28,sr1		#Restore space
    Bbset   r31,modifybit,modifybase
    mfctl   TR5,r28

    # Otherwise, restore everthing and return
    mfctl   TR4,r29
    mfctl   TR3,r30
    mfctl   TR2,r31
    rfi
    nop

#*****************************************************************************
# Continuation of Multply/divide SFU and floating-point coprocessor decode.
#
# Second, separate out floating-point single-word instructions (loads/stores).
#
# Register contents upon entry:  r1-r29,  Original contents.
#			         r30,     Saved, but not used.
#			         r31,     Trapping Instruction
#*****************************************************************************
notdblwd
    Bbset,n r31,spopcopropbit,mdfpop	#Branch if not load or store
    mtctl   r27,TR6

sglwd	#Instruction is one of the floating-point single-word instructions.

    ldil    L'$fpu_register,r27
    ldw     RFPUREGISTER(0,r27),r30	#Get the status reg to check t-bit

# In the mean time check for reserved operand indicated by specifying
# floating-point registers 16-31.  And save away another register.
# NOTE: When the architecture is extended to allow FPR[16..31] this check
#       will have to be modified.

#ifndef TIMEX
    Bbset   r31,rsvrdcopbit,causeresopexception
#endif
    mtctl   r29,TR4
    mtctl   r28,TR5

#ifndef TIMEX
    Bbclr   r31,slbit,fldw	#Separate loads from stores.
#endif
    dep     r31,times8,5,r27	#Isolate and scale source reg spec.  This is
				#injected into the left part of the address of
				#$fpu_register
#ifdef TIMEX
    extru   r31,uidpos,1,r28 	# add offset for right half of register
    Bbclr   r31,slbit,fldw	#Separate loads from stores.
    dep     r28,righthalfbit,1,r27
    mfctl   TR5,r28		# restore r28
#endif

#*****************************************************************************
# FP coprocessor store word
#
# Register contents upon entry: r1-r26,  Original contents.
#				r27,     L'$fpu_register+(8*reg spec)
#				r28,     Saved, but not used.
#				r29,     Saved, but not used.
#				r30,     FP Status Register
#				r31,     Trapping Instruction
#*****************************************************************************
fstw
    # test for set T-bit.  This is a simple goto.
    Bbset,n r30,tbitpos,causeexception
    extru,= r31,fptpos,5,r0	#Check for the status register.
    b       fstwnot0
    ldw	    RFPUREGISTER(0,r27),r30
# Set the appropriate model and version and clear reserved fields.
#ifndef TIMEX
    depi    VERSION,versionpos,modelpversionlen,r30
#endif
    depi    0,rsv1pos,rsv1len,r30
# We know that the t-bit is clear so we can clear both reserved fields at once.
    depi    0,rsv3pos,rsv2and3len,r30

fstwnot0
#ifndef PCXBUG
    mfsp    sr1,r28		#Save space register.
    mfctl   ISR,r29
    mtsp    r29,sr1		#Point to user's data space
#endif
    mfctl   IPSW,r29
    depi    1,pswnullpos,1,r29  #Set nullify bit in interrupt PSW
#ifdef PCXBUG
pcxbug15
    mtctl   r29,IPSW
    mfctl   IPSW,r28
    combf,=,n r28,r29,pcxbug15
#endif
    mtctl   r29,IPSW
    extru,= r29,pswdbitpos,1,r0 #Is data translation enabled?
    ssm     datatrans,r0        #Yes.
#ifdef PCXBUG
    mfsp    sr1,r28             #Save space register.
    mfctl   ISR,r29
    mtsp    r29,sr1             #Point to user's data space
#endif
    mfctl   IOR,r29		#Point to user's data offset
    extru,= r29,b31pos,2,r0     #Is the address Aligned?
    b,n     singleunaligned     # No Cause data memory protect trap.
    stw	    r30,0(sr1,r29)
    # No need to turn off translation since there are no further memory
    # references before returning from the interrupt.

    mfctl   TR6,r27
    # Discover if base register is modified, and emulate if necessary.
    mfctl   IIR,r31		#Get another copy of the instruction
    mtsp    r28,sr1		#Restore space
    Bbset   r31,modifybit,modifybase
    mfctl   TR5,r28

    # Otherwise, restore everthing and return
    mfctl   TR4,r29
    mfctl   TR3,r30
    mfctl   TR2,r31
    rfi
    nop

#*****************************************************************************
# FP coprocessor load word.
#
# Register contents upon entry: r1-r26,  Original contents.
#				r27,     L'$fpu_register+(8*reg spec)
#				r28,     Saved, but not used.
#				r29,     Saved, but not used.
#				r30,     FP Status Register
#				r31,     Trapping Instruction
#*****************************************************************************
fldw
    # test for set T-bit.  This is a simple goto.
    Bbset,n r30,tbitpos,causeexception
#ifndef PCXBUG
    mfsp    sr1,r28		#Save space register.
    mfctl   ISR,r29
    mtsp    r29,sr1		#Point to user's data space
#endif
    mfctl   IPSW,r29
    depi    1,pswnullpos,1,r29  #Set nullify bit in interrupt PSW
#ifdef PCXBUG
pcxbug17
    mtctl   r29,IPSW
    mfctl   IPSW,r28
    combf,=,n r28,r29,pcxbug17
#endif
    mtctl   r29,IPSW
    extru,= r29,pswdbitpos,1,r0 #Is data translation enabled?
    ssm     datatrans,r0        #Yes.
#ifdef PCXBUG
    mfsp    sr1,r28             #Save space register.
    mfctl   ISR,r29
    mtsp    r29,sr1             #Point to user's data space
#endif
    mfctl   IOR,r29		#Point to user's data offset
    extru,= r29,b31pos,2,r0     #Is the address Aligned?
    b,n     singleunaligned     # No Cause data memory protect trap.
    ldw	    0(sr1,r29),r30
    rsm	    datatrans,r0	#Turn off data translation

    stw	    r30,RFPUREGISTER(0,r27)

    mfctl   TR6,r27
    # Discover if base register is modified, and emulate if necessary.
    mfctl   IIR,r31		#Get another copy of the instruction
    mtsp    r28,sr1		#Restore space
    Bbset   r31,modifybit,modifybase
    mfctl   TR5,r28

    # Otherwise, restore everthing and return
    mfctl   TR4,r29
    mfctl   TR3,r30
    mfctl   TR2,r31
    rfi
    nop

#ifdef TIMEX
#******************************************************
# Quad store operation
#******************************************************
quadstore
	#
	# code not working yet, so generate unimplemented trap
	#
	b,n	unimplementedexception

#ifdef notdef	/* not only does this not work, it does not assemble! */
	extru,= r31,uidpos,3,r0	# check for uid <> 0
	b,n	$nonzerohandler

	mtctl	r27,tr6
	mfctl	IVA,r27		#get address of emulation registers
	ldw	EM_FPU_OFFSET(0,r27),r27
	ldw	RFPUREGISTER(0,r27),r30		#get FP status word

	mtctl	r29,tr4
	mtctl	r28,tr5
	# get offset to store
	zdep	r31,times8,5,r29
	Bbclr	r31,slbit,unimplementedexception	# no quad loads
	add	r27,r29,r27
	#
	# now r27 contains the offset to store from
	#
	# check for a store of FR0
	Bbset	r30,tbitpos,quadmaybecauseexception
	extru,=	r31,fptpos,5,r0
	b,n	fstqnot0
fstqjoin
	depi	0,rsv1pos,rsv1len,r30		# zero out reserved fields
	depi	0,rsv2pos,rsv2len,r30
	depi	0,rsv3pos,rsv3len,r30
	Skip
fstqnot0
	ldw	RFPUREGISTER(0,r27),r30		# get 1st word to store
	#
	# save r27 so the shadow IVA routines can also use it
	#
	copy	r27,r31
	mfctl	IOR,r27			# get address we're storeing to
	extru,=	r27,b31pos,4,r0		# check alignment
	b,n	quadunaligned
	mfctl	IPSW,r29
	Bbset,n	r29,pswdbitpos,fstq_dte
	depi	1,pswnullpos,r29	# set nullify bit
	mtctl	r29,IPSW
	mfsp	sr1,r28
	mfctl	ISR,r29			# set up proper space
	mtsp	r29,sr1
	#
	# store out the data
	#
	stw	r30,0(sr1,r27)
	ldw	RFPUREGISTER+4(0,r31),r30
	stw	r30,4(sr1,r27)
	ldw	RFPUREGISTER+8(0,r31),r30
	stw	r30,8(sr1,r27)
	ldw	RFPUREGISTER+12(0,r31),r30
	stw	r30,12(sr1,r27)

	mfctl	tr6,r27
	mfctl	IIR,r31
	mtsp	r28,sr1
	Bbset	r31,modifybit,modifybase
	mfctl	tr5,r28

	mfctl	tr4,r29
	mfctl	tr3,r30
	mfctl	tr2,r31
	rfi
	nop

fstq_dte
	mtctl	IVA,r28
	stw	r29,SH_IPSW_OFFSET(0,r28)
	mtctl	r28,tr7
	depi	1,pswnullpos,1,r29
	mtctl	r29,IPSW
	mfsp	sr1,r28			# set up porper space
	mfctl	ISR,r29
	mtsp	r29,sr1
	ldil	L_SH_IVA,r29		# set up shadow IVA
	ldo	R_SH_IVA(r29),r29
	mtctl	r29,IVA
	ssm	datatrans,r0		# turn on data translation
	stw	r30,0(sr1,r27)		# store the data
	ldw	RFPUREGISTER+4(0,r31),r30
	stw	r30,4(sr1,r27)
	ldw	RFPUREGISTER+8(0,r31),r30
	stw	r30,8(sr1,r27)
	ldw	RFPUREGISTER+12(0,r31),r30
	stw	r30,12(sr1,r27)
	rsm	datatrans,0
	mfctl	tr7,r29
	mtctl	r29,IVA
	mfctl	tr6,r27
	mfctl	IIR,r31
	mtsp	r28,sr1
	Bbset	r31,modifybit,modifybase
	mfctl	tr5,r28
	mfctl	tr4,r29
	mfctl	tr3,r30
	mfctl	tr2,r31
	rfi
#endif	/* notdef */
	nop

#endif /* TIMEX */

#******************************************************
# Modify the base register.
# Register contents: r31, Trapping instruction
#		 r29-r30, Free
#                r28-r1,  Original values
#******************************************************
modifybase
    #Branch when short load or store
    Bbclr   r31,shortindexbit,indexmodifybase
    extru   r31,indexpos,5,r30	#Get the index-reg/immediate spec.

    # Short load or store instruction.  Sign extend the immediate field
    # by first copying the low bit (remember the funny encoding) to the
    # left side of the immediate field.  Then sign extend and reposition
    # with an extract.
    #  Initially             ---+---+---+---+---+---+---+---+---+
    #                        ...    | 0 | 0 |4bit Immediate |sgn| 
    #                        ---+---+---+---+---+---+---+---+---+
    #  After the deposit     ---+---+---+---+---+---+---+---+---+
    #                        ...    | 0 |sgn|4bit Immediate |sgn| 
    #                        ---+---+---+---+---+---+---+---+---+
    #  After the extrs       ---+---+---+---+---+---+---+---+---+
    #                        ...|sgn|sgn|sgn|sgn|4bit Immediate |
    #                        ---+---+---+---+---+---+---+---+---+
shortdisp
    dep     r30,31-5,1,r30      #move 1 low bit to position 31-5
    extrs   r30,31-1,5,r30      #signextend 5bit field ending at bit 30
    b	    updatebase
    extru   r31,basepos,5,r29    #Get base register spec. in delay

    # Index load or store instruction.  Case jump based on the index register
    # spec to fetch the correct register.  The last couple registers must be
    # handled specially since the original values are in control registers.
indexmodifybase
    blr     r30,r0		#Switch( index-reg spec )
    extru   r31,basepos,5,r29    #Get base register spec. in delay
indexcase0
    movib,tr 0,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase1
    movb,tr r1,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase2
    movb,tr r2,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase3
    movb,tr r3,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase4
    movb,tr r4,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase5
    movb,tr r5,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase6
    movb,tr r6,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase7
    movb,tr r7,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase8
    movb,tr r8,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase9
    movb,tr r9,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase10
    movb,tr r10,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase11
    movb,tr r11,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase12
    movb,tr r12,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase13
    movb,tr r13,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase14
    movb,tr r14,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase15
    movb,tr r15,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase16
    movb,tr r16,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase17
    movb,tr r17,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase18
    movb,tr r18,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase19
    movb,tr r19,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase20
    movb,tr r20,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase21
    movb,tr r21,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase22
    movb,tr r22,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase23
    movb,tr r23,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase24
    movb,tr r24,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase25
    movb,tr r25,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase26
    movb,tr r26,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase27
    movb,tr r27,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase28
    movb,tr r28,r30,updatebase
    Bbset,n  r31,ubitpos,shiftneeded  #Br from normal flow if shift is needed.
indexcase29
    mfctl   TR4,r30		#Get the original contents from the temp reg.
    b,n     indexcase31p4
indexcase30
    mfctl   TR3,r30		#Must get r30 from save registers
    Skip			#Just skip over the next instruction
indexcase31
    mfctl   TR2,r30		#Must get r31 from save registers
indexcase31p4
    Bbclr,n r31,ubitpos,updatebase #Reverse branch sense in these last three
				   #cases.

shiftneeded
    sh2add  r30,r0,r30		#Calculate word offset
    extru,= r31,dblwdbit,1,r0	#If( its double-word offset )
    sh1add  r30,r0,r30		#  then scale again.

    # Updatebase - complete the address computation by adding the index reg
    # 	or immediate field (in r30) into the base register spec.  Again the
    #	last couple of register numbers are special and must refer to control
    #	registers.
updatebase
    mfctl   TR2,r31
    blr	    r29,r0		#Switch( base register spec)
    mfctl   TR4,r29		#Do a little house-cleaning in delay
basecase0
    mfctl   TR3,r30		#Restore register
    rfi				#Modify with base=0 is silly.
basecase1
    addb,tr r30,r1,casebaseend
    mfctl   TR3,r30		#Restore register
basecase2
    addb,tr r30,r2,casebaseend
    mfctl   TR3,r30		#Restore register
basecase3
    addb,tr r30,r3,casebaseend
    mfctl   TR3,r30		#Restore register
basecase4
    addb,tr r30,r4,casebaseend
    mfctl   TR3,r30		#Restore register
basecase5
    addb,tr r30,r5,casebaseend
    mfctl   TR3,r30		#Restore register
basecase6
    addb,tr r30,r6,casebaseend
    mfctl   TR3,r30		#Restore register
basecase7
    addb,tr r30,r7,casebaseend
    mfctl   TR3,r30		#Restore register
basecase8
    addb,tr r30,r8,casebaseend
    mfctl   TR3,r30		#Restore register
basecase9
    addb,tr r30,r9,casebaseend
    mfctl   TR3,r30		#Restore register
basecase10
    addb,tr r30,r10,casebaseend
    mfctl   TR3,r30		#Restore register
basecase11
    addb,tr r30,r11,casebaseend
    mfctl   TR3,r30		#Restore register
basecase12
    addb,tr r30,r12,casebaseend
    mfctl   TR3,r30		#Restore register
basecase13
    addb,tr r30,r13,casebaseend
    mfctl   TR3,r30		#Restore register
basecase14
    addb,tr r30,r14,casebaseend
    mfctl   TR3,r30		#Restore register
basecase15
    addb,tr r30,r15,casebaseend
    mfctl   TR3,r30		#Restore register
basecase16
    addb,tr r30,r16,casebaseend
    mfctl   TR3,r30		#Restore register
basecase17
    addb,tr r30,r17,casebaseend
    mfctl   TR3,r30		#Restore register
basecase18
    addb,tr r30,r18,casebaseend
    mfctl   TR3,r30		#Restore register
basecase19
    addb,tr r30,r19,casebaseend
    mfctl   TR3,r30		#Restore register
basecase20
    addb,tr r30,r20,casebaseend
    mfctl   TR3,r30		#Restore register
basecase21
    addb,tr r30,r21,casebaseend
    mfctl   TR3,r30		#Restore register
basecase22
    addb,tr r30,r22,casebaseend
    mfctl   TR3,r30		#Restore register
basecase23
    addb,tr r30,r23,casebaseend
    mfctl   TR3,r30		#Restore register
basecase24
    addb,tr r30,r24,casebaseend
    mfctl   TR3,r30		#Restore register
basecase25
    addb,tr r30,r25,casebaseend
    mfctl   TR3,r30		#Restore register
basecase26
    addb,tr r30,r26,casebaseend
    mfctl   TR3,r30		#Restore register
basecase27
    addb,tr r30,r27,casebaseend
    mfctl   TR3,r30		#Restore register
basecase28
    addb,tr r30,r28,casebaseend
    mfctl   TR3,r30		#Restore register
basecase29
    addb,tr r30,r29,casebaseend
    mfctl   TR3,r30		#Restore register
basecase30
    mfctl   TR3,r31		#Must get r30 from save registers
    addb,tr,n r31,r30,specialend
basecase31
    add     r30,r31,r31
    mfctl   TR3,r30		#Restore register
casebaseend
    rfi
    nop
specialend
    mfctl   TR2,r31
    rfi
    nop

#*******************************************************************
# Maybecauseexception - an exception is indicated by the T-bit, but
# must first check if the source is register zero.  If so, then
# clear the T-bit in the status register and continue as if the
# T-bit was clear.  Be sure the value returned to the user's memory
# has the T-bit set.
#
# Note that the check for register zero is actually done in the delay-slot
# of the branch to this location.  Also, this routine will return to
# the "fstd" routine via the "fstdjoin" label.
#
# Register contents upon entry: r1-r26,  Original contents.
#				   r27,  L'$fpu_register+(8*reg spec)
#				   r28,  Saved, but not used.
#				   r29,  Saved, but not used.
#				   r30,  FPU Status Register
#				   r31,  Trapping Instruction
#*******************************************************************
maybecauseexception
    extru,= r31,fptpos,5,r0     #If( target spec is not zero )
    b,n     causeexception	#Then go cause a real exception
    depi    0,tbitpos,1,r30	#Else Reg spec is 0, Clear out the T-bit
				#r27 is not biased by a register spec.
    stw	    r30,RFPUREGISTER(0,r27)    #Clear it in the hardware copy,
    b	    fstdjoin		# (jump back into fstd)
    depi    1,tbitpos,1,r30	# but set it in the version returned to
				# the users memory.

#*****************************************************************************
# Causeresopexception - a reserved-op exception is indicated.  Normal
#  T-bit testing has not occurred.
#  Reserved-op exceptions are immediate.  This routine 
#  resets the CPU registers and calls the exception trap handler.
#
#  On entry:	r31 - Trapping instruction
#	  	r30 - FP status register
#		r29 - Saved, but not yet used
#	        r28 - Original value
#               r27 - L'$fpu_register
#            r26-r1 - original values
#*****************************************************************************
causeresopexception

#*****************************************************************************
# Causeexception - an exception is indicated by the T-bit or an
#  immediate reserved-op exception is to be signalled.  Restore
#  all the appropriate registers and branch to the exception handling
#  code.  This will not be an exact emulation since the priority of traps
#  will mean a TLB fault is serviced when hardware does not exist (emulation
#  trap) but is not serviced when hardware exists. 
#  
#	Register contents: r1-r26,  Original contents.
#			   r27,     L'$fpu_register+(8*reg spec) or
#				    L'$fpu_register.
#			   r28,     Original contents
#			   r29,     Saved, but not used.
#			   r30,     Contains FPU status register.
#			   r31,     Trapping instruction
#*****************************************************************************
causeexception
    mfctl   TR6,r27
    #Clear the nullify bit that we set earlier to restore things exactly.
quickcauseexception
    mfctl   IPSW,r30
    depi    0,pswnullpos,1,r30
#ifdef PCXBUG
pcxbug19
    mtctl   r30,IPSW
    mfctl   IPSW,r31
    combf,=,n r31,r30,pcxbug19
#endif
    mtctl   r30,IPSW

    mfctl   TR3,r30
#Calculate address of assists exception vector location using the IVA
    mfctl   IVA,r31
    ldo     assistsexception*32(r31),r31  #32 bytes (8 I's) per entry
    bv      r0(r31)
    mfctl   TR2,r31
#*****************************************************************************
# Singleunaligned, doubleunaligned.  Unaligned data references are emulated
#  by causing a data memory protection trap.
#  
#	Register contents: r1-r26,  Original contents.
#			   r27,     L'$fpu_register+(8*reg spec) or
#			   r28,     Saved space register one.
#			   r29,     copy of IOR
#			   r30,     Data to be stored or unchanged.
#			   r31,     dbl: Data to be stored or copy of IIR
#				    sgl: copy of IIR
#*****************************************************************************
doubleunaligned
    mfctl   TR5,r28
singleunaligned
    mfctl   TR6,r27
    mfctl   TR4,r29
    #Clear the nullify bit that we set earlier to restore things exactly.
    mfctl   IPSW,r30
    depi    0,pswnullpos,1,r30
#ifdef PCXBUG
pcxbug20
    mtctl   r30,IPSW
    mfctl   IPSW,r31
    combf,=,n r30,r31,pcxbug20
#endif
    mtctl   r30,IPSW

    mfctl   TR3,r30
#Note that the PSW[D] bit may have been set prior to the alignment check.
#We need to clear the bit before vectoring to the data memory protection
#check handler in order to preserve its "entry requirements."
    rsm     datatrans,r0

#Calculate address of data memory protect vector location using the IVA
    mfctl   IVA,r31
    ldo     datamemoryprotection*32(r31),r31  #32 bytes (8 I's) per entry
    bv      r0(r31)
    mfctl   TR2,r31

#*****************************************************************************
# Continuation of Multply/divide SFU and floating-point coprocessor decode.
#
# Third, separate out floating-point operation instructions.
#
# Register contents upon entry:  r1-r29, Original contents.
#			            r30, Saved, but not used.
#			            r31, Trapping Instruction
#*****************************************************************************
mdfpop
    extru   r31,5,6,r30		#get major op
    comib,<>,n  0x0c,r30,not0c	# check for a uid of 0 for 0C ops
    extru,=     r31,uidpos,3,r0
    b,n	$nonzerohandler

    comib,= 0x4,r30,mdsfuop		# decode the SFU instructions
not0c
    mfctl   IPSW,r30

fpcoprop  #Instruction is a floating-point operation instruction.

    depi    1,pswnullpos,1,r30
#ifdef PCXBUG
    mtctl   r29,TR4
pcxbug21
    mtctl   r30,IPSW
    mfctl   IPSW,r29
    combf,=,n r29,r30,pcxbug21
#endif
    mtctl   r30,IPSW		# Set nullify bit in IPSW.
#ifndef PCXBUG
    mtctl   r29,TR4
#endif

    #Use a modified C calling convention.  Only provide the callee
    #needed structures: function-value return and argument list.  The
    #'general register save' and 'status save area' caller used 
    #structures are combined into a single save area.  The 'register spill'
    #and 'local storage' areas are not needed.  'space and floating-point
    #register save' areas are not currently needed.

    #If this routine is upgraded to allow interrupts a much longer and
    # involved sequence will be necessary.

    #If we're already on the interrupt stack (istackptr==0), then we
    #simply use the current sp.  Otherwise, use the value in istackptr,
    #which can contain only one other valid value: the address of istack.
    #If first frame on the stack, zero out istackptr.
#if defined(MACH_KERNEL) 
    ldil    L'istackptr,r29
    ldw     R'istackptr(r29),sp
    comb,<>,n 0,sp,istackatbase
    movb,tr sp,r29,istacknotatbase #Save istackptr
    mfctl   TR3,sp		   #Get sp back from temp reg.
istackatbase
    stw     0,R'istackptr(r29)
    copy    sp,r29		#Save istackptr
istacknotatbase

#else 

    #Get a pointer to a local stack.
    ldil    L'$emulate_stack,sp
    ldo     R'$emulate_stack(sp),sp

#endif

    ldo     SPECIAL_FRAME_SIZE(sp),sp	#Set up save area

    #Start saving away status and link registers.
    #Parameter register 0 must be saved first.
    stw     r28,SPECIAL_FRAME_R28(sp)	
    stw     r27,SPECIAL_FRAME_R27(sp)
    stw     r26,SPECIAL_FRAME_R26(sp)	#arg0 (Proc Calling Conventions)
    stw     r25,SPECIAL_FRAME_R25(sp)	#arg1 (Proc Calling Conventions)
    stw     r24,SPECIAL_FRAME_R24(sp)	#arg2 (Proc Calling Conventions)
    stw     r23,SPECIAL_FRAME_R23(sp)	#arg3 (Proc Calling Conventions)
    stw     r22,SPECIAL_FRAME_R22(sp)	#Caller saves (Proc Calling Conventions)
    stw     r21,SPECIAL_FRAME_R21(sp)
    stw     r20,SPECIAL_FRAME_R20(sp)
    stw     r19,SPECIAL_FRAME_R19(sp)
    stw     r2,SPECIAL_FRAME_R2(sp)	#rp (New Proc Calling Conventions)
				#End caller saves (Proc Calling Conventions)
				#and align on a double-word boundary.
				#Registers 29-31 are already saved
				#in temp control registers.
    stw     r1,SPECIAL_FRAME_R1(sp)
    mfctl   sar,arg3		#save Shift Amount Register
    stw     arg3,SPECIAL_FRAME_SAR(sp)

#if defined(MACH_KERNEL) 
    stw     r29,SPECIAL_FRAME_SAVE_ISTACKPTR(sp) #Save away old istackptr
#endif

    #Establish addressability to the emulated floating-point registers.
    ldil    L'$fpu_register,arg3
    ldo     RFPUREGISTER(arg3),arg3

    #Check if exception trap is signaled with the trap bit.
    ldw     0(arg3),arg0	#Use known temp.
    Bbset,n arg0,tbitpos,fpexceptiontrap	#Restore everthing and branch
						#to exception trap.
    stw     r31,SPECIAL_FRAME_IR(sp)
    stw     arg3,SPECIAL_FRAME_SAVE_REG_PTR(sp) #Save copy of pointer to 
						#floating-point registers
    #Call fpudispatch from an emulation trap.
    bl      emfpudispatch,rp
    copy    r31,arg0

#****************************************************************
# Fpopreturn - return point for all the semantic floating-point
#    routines.  If ret0 is non-zero, an exception needs to be
#    signaled.
#****************************************************************
fpopreturn
    #restore all the registers which may have been modified.
    ldw      SPECIAL_FRAME_R19(0,sp),r19	
    ldw      SPECIAL_FRAME_R20(0,sp),r20
    ldw      SPECIAL_FRAME_R21(0,sp),r21
    ldw      SPECIAL_FRAME_R22(0,sp),r22
    ldw      SPECIAL_FRAME_R23(0,sp),r23		#arg3
    ldw      SPECIAL_FRAME_R24(0,sp),r24 		#arg2
    ldw      SPECIAL_FRAME_R25(0,sp),r25		#arg1
    ldw      SPECIAL_FRAME_R26(0,sp),r26		#arg0
    ldw	     SPECIAL_FRAME_R1(0,sp),r1
    ldw	     SPECIAL_FRAME_R2(0,sp),r2
    ldw	     SPECIAL_FRAME_SAR(0,sp),r27
    mtctl    r27,sar

    comib,=  0,ret0,notrap		#Check for returned exception
    ldw      SPECIAL_FRAME_R27(0,sp),r27
    ldw      SPECIAL_FRAME_SAVE_REG_PTR(sp),r29 #Get pointer to $fpu_register
    ldw      SPECIAL_FRAME_IR(sp),r31	#Get a copy of the instruction.
    dep      ret0,exceptionpos,exceptionsize,r31
    stw      r31,4(0,r29)		#Set exception register 1
    ldw      0(0,r29),r31		#Get the floating-point status register
    depi     1,tbitpos,1,r31		#Set T-bit
    stw      r31,0(0,r29)		#Set status register
notrap
    ldw      SPECIAL_FRAME_R28(0,sp),r28

#if defined(MACH_KERNEL) 
    ldw     SPECIAL_FRAME_SAVE_ISTACKPTR(sp),r31
    ldil    L'istackptr,r30
    stw     r31,R'istackptr(r30)
#endif

    mfctl   TR4,r29
    mfctl   TR3,r30
    mfctl   TR2,r31
    rfi
    nop


#**************************************************************************
# ftest - test the state of the condition code and nullify the next
#   instruction if true.  The result register points to the fpu's
#   status register (all unused register specs must be zero).
#   arg1 contains the t field (5 bits)
#**************************************************************************
ftest
#ifdef TIMEX
    ldw      0(0,arg2),arg0		     #Get the status word
    blr      arg1,r0                         #switch on t-field
    ldi      0,ret0			     #return NOEXCEPTION
ftest_normal				# 00
    b        ftest_normal_check
    nop
ftest_acc				# 01
    b        ftest_acc_check
    nop
ftest_rej				# 02
    b        ftest_rej_check
    nop
ftest_03
    b	unimplementedexception
    nop
ftest_04
    b	unimplementedexception
    nop
ftest_acc8				# 05
    b ftest_acc8_check
    nop
ftest_rej8				# 06
    b ftest_rej8_check
    nop
ftest_07
    b unimplementedexception
    nop
ftest_08
    b unimplementedexception
    nop
ftest_acc6				# 09
    b ftest_acc6_check
    nop
ftest_0a
    b unimplementedexception
    nop
ftest_0b
    b unimplementedexception
    nop
ftest_0c
    b unimplementedexception
    nop
ftest_acc4
    b ftest_acc4_check
    nop
ftest_0e
    b unimplementedexception
    nop
ftest_0f
    b unimplementedexception
    nop
ftest_10
    b unimplementedexception
    nop
ftest_acc2				# 11
    b ftest_acc2_check
    nop
ftest_12
    b unimplementedexception
    nop
ftest_13
    b unimplementedexception
    nop
ftest_14
    b unimplementedexception
    nop
ftest_15
    b unimplementedexception
    nop
ftest_16
    b unimplementedexception
    nop
ftest_17
    b unimplementedexception
    nop
ftest_18
    b unimplementedexception
    nop
ftest_19
    b unimplementedexception
    nop
ftest_1a
    b unimplementedexception
    nop
ftest_1b
    b unimplementedexception
    nop
ftest_1c
    b unimplementedexception
    nop
ftest_1d
    b unimplementedexception
    nop
ftest_1e
    b unimplementedexception
    nop
ftest_1f
    b unimplementedexception
    nop

ftest_rej_check				# FTEST,REJ test code
    extru arg0,5,1,arg1			# check bit 5
    extru arg0,15,1,arg2		#check bit 15
    and,= arg1,arg2,r0			# nullify if bit(5) && bit(15)
    b,n   ftest_nullify
    extru arg0,10,1,arg1		# check bit 10
    extru arg0,16,1,arg2		#check bit 16
    and,= arg1,arg2,r0			# nullify if bit(10) && bit(16)
    b,n   ftest_nullify
    extru arg0,11,1,arg1		# check bit 11
    extru arg0,17,1,arg2		#check bit 17
    and,= arg1,arg2,r0			# nullify if bit(10) && bit(16)
    b,n   ftest_nullify
    extru arg0,12,1,arg1		# check bit 12
    extru arg0,18,1,arg2		#check bit 18
    and,= arg1,arg2,r0			# nullify if bit(10) && bit(16)
    b,n   ftest_nullify
    extru arg0,13,1,arg1		# check bit 13
    extru arg0,19,1,arg2		#check bit 19
    and,= arg1,arg2,r0			# nullify if bit(10) && bit(16)
    b,n   ftest_nullify
    extru arg0,14,1,arg1		# check bit 14
    extru arg0,20,1,arg2		#check bit 20
    and,= arg1,arg2,r0			# nullify if bit(10) && bit(16)
    b,n   ftest_nullify
    bv,n  0(rp)				# condition not satisfied, return

ftest_rej8_check			# FTEST,REJ8 test code
    extru arg0,5,1,arg1			# check bit 5
    extru arg0,13,1,arg2		#check bit 13
    and,= arg1,arg2,r0			# nullify if bit(5) && bit(13)
    b,n   ftest_nullify
    extru arg0,10,1,arg1		# check bit 10
    extru arg0,14,1,arg2		#check bit 14
    and,= arg1,arg2,r0			# nullify if bit(10) && bit(14)
    b,n   ftest_nullify
    extru arg0,11,1,arg1		# check bit 11
    extru arg0,15,1,arg2		#check bit 15
    and,= arg1,arg2,r0			# nullify if bit(10) && bit(15)
    b,n   ftest_nullify
    extru arg0,12,1,arg1		# check bit 12
    extru arg0,16,1,arg2		#check bit 16
    and,= arg1,arg2,r0			# nullify if bit(12) && bit(16)
    b,n   ftest_nullify
    bv,n  0(rp)

ftest_acc2_check			# ftest,acc2
    extru arg0,10,1,arg1		# get status[10]
    extru arg0,5,1,arg2			# get status[5]
    or,<> arg1,arg2,r0			# nullify if all zero's
    b,n   ftest_nullify
    bv,n  0(rp)

ftest_acc4_check			# ftest,acc4
    extru arg0,12,3,arg1		# get status[10:12]
    extru arg0,5,1,arg2			# get status[5]
    or,<> arg1,arg2,r0			# nullify if all zero's
    b,n   ftest_nullify
    bv,n  0(rp)

ftest_acc6_check			# ftest,acc6
    extru arg0,14,5,arg1		# get status[10:14]
    extru arg0,5,1,arg2			# get status[5]
    or,<> arg1,arg2,r0			# nullify if all zero's
    b,n   ftest_nullify
    bv,n  0(rp)

ftest_acc8_check			# ftest,acc8
    extru arg0,16,7,arg1		# get status[10:16]
    extru arg0,5,1,arg2			# get status[5]
    or,<> arg1,arg2,r0			# nullify if all zero's
    b,n   ftest_nullify
    bv,n  0(rp)

ftest_acc_check
    extru arg0,20,11,arg1		# get status[10:20]
    extru arg0,5,1,arg2			# get status[5]
    or,<> arg1,arg2,r0			# nullify if all zero's
    b,n   ftest_nullify
    bv,n  0(rp)

ftest_normal_check
#else
    ldw      0(0,arg2),arg0                  #Get the status word
    ldi      0,ret0                          #return NOEXCEPTION
#endif /* TIMEX */
    extru,<> arg0,ccbitpos,1,r0		     #Nop if condition bit is not set
    bv,n     0(rp)
ftest_nullify
    #Nullify second instruction in PCqueue. (First is already done).
    mtctl   r0,PCSQ	#Advance the PCqueue.  Ignoring the front element.
    mfctl   PCSQ,arg0	#Pick up the queue's back's space register
    mtctl   arg0,PCSQ   #Push it into the back.
    mtctl   arg0,PCSQ   #Now the space portion of the PCqueue contains two
			#copies of the orignal queue's back space register.
    mtctl   r0,PCOQ	#Advance the PCqueue.  Ignoring the front element.
    mfctl   PCOQ,arg0	#Pick up the queue's back's offset register
    mtctl   arg0,PCOQ   #Push it into the back.
    addi    4,arg0,arg0 #Point it 4 bytes past the nullified instruction.
    bv      0(rp)
    mtctl   arg0,PCOQ	#Now the offset portion of the PCqueue points to
			#the originally back element (nullified by the PSW)
			#and 4 bytes beyond that value.
#*********************************************************************
# unimplementedexception - trigger an unimplemented exception,
#    and return.  Do not change the contents of the target.
#*********************************************************************
unimplementedexception
    bv      0(rp)
    ldi     UNIMPLEMENTEDEXCEPTION,ret0	#return UNIMPLEMENTEDEXCEPTION

#*************************************************************************
# Fpexceptiontrap - restore the modified registers and branch to the
#  assists exception trap.
#  Only arg0, arg3, dp, ap, sp, and r31 have been modified.
#*************************************************************************
fpexceptiontrap
    ldw     SPECIAL_FRAME_R23(0,sp),r23
    ldw     SPECIAL_FRAME_R26(0,sp),r26
    ldw     SPECIAL_FRAME_R27(0,sp),r27
    ldw     SPECIAL_FRAME_R28(0,sp),r28
    mfctl   IPSW,r29
    depi    0,pswnullpos,1,r29

#if defined(MACH_KERNEL) 
    ldw     SPECIAL_FRAME_SAVE_ISTACKPTR(sp),r31
    ldil    L'istackptr,r30
    stw     r31,R'istackptr(r30)
#endif

#ifdef PCXBUG
pcxbug22
    mtctl   r29,IPSW
    mfctl   IPSW,r30
    combf,=,n r30,r29,pcxbug22
#endif
    mtctl   r29,IPSW
    mfctl   TR4,r29
    mfctl   TR3,r30
#Calculate address of assists exception vector location using the IVA
    mfctl   IVA,r31
    ldo     assistsexception*32(r31),r31  #32 bytes (8 I's) per entry
    bv      r0(r31)
    mfctl   TR2,r31

#****************
# dummy defualt location to call things we haven't written yet.
#****************
quad_fadd
quad_fcmp
quad_fdiv
quad_fmpy
quad_frem
quad_frnd
quad_fsqrt
quad_fsub

sgl_to_quad_fcnvff
dbl_to_quad_fcnvff
quad_to_sgl_fcnvff
quad_to_dbl_fcnvff

sgl_to_quad_fcnvfx
dbl_to_quad_fcnvfx
quad_to_sgl_fcnvfx
quad_to_dbl_fcnvfx
quad_to_quad_fcnvfx

sgl_to_quad_fcnvfxt
dbl_to_quad_fcnvfxt
quad_to_sgl_fcnvfxt
quad_to_dbl_fcnvfxt
quad_to_quad_fcnvfxt

sgl_to_quad_fcnvxf
dbl_to_quad_fcnvxf
quad_to_sgl_fcnvxf
quad_to_dbl_fcnvxf
quad_to_quad_fcnvxf
        b,n  unimplementedexception
	nop

#*****************************************************************************
# Continuation of Multply/divide SFU and floating-point coprocessor decode.
#
# Last, left with Multiply/Divide SFU operation instructions.
#
# Register contents upon entry:  r1-r29, Original contents.
#			            r30, Trapping IPSW
#			            r31, Trapping Instruction
#*****************************************************************************

mdsfuop
#ifdef MDSFU_SUPPORTED
    # Old MDSFU code goes here!!!
#else
    # Multiply/divide SFU is no longer supported.
    mfctl TR3,r30
    mfctl TR2,r31
    .call
    THANDLER(I_EMULAT)
    nop
#endif


#*****************************************************************************
# Emulation trap handler for assists with "uid" <> 0.
# Currently, none of these are supported.
#
# Register contents upon entry:  r1-r30, Original contents.
#			            r31, Trapping Instruction
#*****************************************************************************
$nonzerohandler
	nop			/* be careful out there! */
        mfctl TR2,r31
	.call
        THANDLER(I_EMULAT)
	nop			/* be careful out there! */

# allocate space for the floating-point registers

	.SPACE	$PRIVATE$
	.SUBSPA	$BSS$

    .export $fpu_register

#ifdef TIMEX
    .align 32
$fpu_register .comm 34*8 		#32 double-word registers and
				#one spare to use as the zero register.
#else /* not TIMEX */
    .align 32
$fpu_register .comm 18*8 		#Sixteen double-word registers and
				#one spare to use as the zero register.
#endif /* TIMEX */

#ifdef MDSFU_SUPPORTED
$mdsfu_register         .comm   24
#endif

#if defined(MACH_KERNEL) 
    .import istackptr
#else 
    .export $emulate_stack,priv_lev = 0
    .align 16			#Quad-word alignment
$emulate_stack .comm 1000*4		#Space for special C environment stack
				#Used for testing purposes.
#endif
