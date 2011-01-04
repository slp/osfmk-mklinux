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

/* $Source: /u1/osc/rcs/mach_kernel/hp_pa/asm_excp.s,v $
 * $Revision: 1.1.13.1 $	$Author: bruel $
 * $State: Exp $   	$Locker:  $
 * $Date: 1997/06/02 17:01:10 $
 */

/*
 *	A general framework for the handling of coprocessor and
 *	special function unit exceptions for the HP PA processor:
 *
 *	Coprocessor and SFU instructions are first decoded to specially
 *	process unit zero instructions for the multiply/divide SFU and the
 *	floating-point coprocessor.  Within this catagory the load and store
 *	coprocessor instructions go directly to realtrap, since a reserved-op 
 *	trap is the only type of exception trap they can generate.
 *	The fpu coprocessor and multiply/divide sfu registers are saved away 
 *	and a C decode routine is called.
 *
 *	This version makes the following assumptions:
 *	  1) There is only one valid exception register.
 *	  2) If the T-bit isn't set, it's a reservedop exception.
 *
 *	The register definitions below are rather randomly assigned to
 *	values.  They may not represent the current architecture.
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
#define RFPUREGISTER R'$fpu_register
#define RMDSFUREGISTER R'$mdsfu_register
#define THANDLER(arg) ; \
	addi    -14,r30,r30 ; \
	mfctl   IIR,r12 ; \
	mfctl   PCSQ,16 !\
	mfctl   PCOQ,17 ; \
	mtctl   2,TR0 ; \
	mfsp    0,2 ; \
	mtctl   2,PCSQ ; \
	mtctl   31,PCOQ ; \
	addi    4,31,31 ; \
	mtctl   2,PCSQ ; \
	mtctl   31,PCOQ ; \
	mfctl   TR0,2 ; \
	rfi ; nop

#endif AVPS

CCR	.equ		10
ITimer	.equ		16
PCSQ	.equ		17
PCOQ	.equ		18
IIR	.equ		19
TR0	.equ		24
TR2	.equ		26
TR3	.equ		27
TR4	.equ		28
TR5	.equ		29
TR6	.equ		30
TR7	.equ		31


#undef Bbset
#undef Bbclr
#undef Skip

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
# FP Coprocessor exception register equates.  Used instead of a define
#  since its use is in the shadow of a quote (ex ldil  L'reservedop,...).

#The exception field is the left 6 bits of the exception register.
#This can always be done with a 'depi' or 'zdepi' instruction.
#define exceptionpos	5
#define exceptionsize	6
#define INVALIDEXCEPTION 32
#define DIVISIONBYZEROEXCEPTION 16
#define OVERFLOWEXCEPTION 8
#define UNDERFLOWEXCEPTION 4
#define INEXACTEXCEPTION 2
#define UNIMPLEMENTEDEXCEPTION 1
#define RESERVEDOPEXCEPTION -1

#Trap vector numbers.  Used by the hardware to vector through the
# interrupt vector control register.
#define assistsexception	14
#define datamemoryprotection	18
#define assistsemulation	22

# Definitions for the special C language stack frame used by the 
# non-interruptable semantic routines.
#define SPECIAL_FRAME_SIZE 16*4+48

#define SPECIAL_FRAME_R28  -16*4-48
#define SPECIAL_FRAME_R27  -15*4-48
#define SPECIAL_FRAME_R26  -14*4-48
#define SPECIAL_FRAME_R25  -13*4-48
#define SPECIAL_FRAME_R24  -12*4-48
#define SPECIAL_FRAME_R23  -11*4-48
#define SPECIAL_FRAME_R22  -10*4-48
#define SPECIAL_FRAME_R21  -9*4-48
#define SPECIAL_FRAME_R20  -8*4-48
#define SPECIAL_FRAME_R19  -7*4-48
#define SPECIAL_FRAME_R2   -6*4-48
#define SPECIAL_FRAME_R1   -5*4-48
#define SPECIAL_FRAME_SAR  -4*4-48
#define SPECIAL_FRAME_IR   -3*4-48
#define SPECIAL_FRAME_SAVE_REG_PTR   -2*4-48
#define SPECIAL_FRAME_SAVE_ISTACKPTR -1*4-48

# Misc. definitions
#define times8		28
#define b0pos		0
#define b29pos		29
#define b30pos		30
#define b31pos		31

uidposition	.equ	25

#******************
# Imports.
#******************
	.space $TEXT$
        .subspa $CODE$
    .import $nonzerohandler
    .import decode_fpu
#ifdef MDSFU_SUPPORTED
    .import decode_mdsfu
#endif

#******************
# Exports.
#******************
    .export $excphandler

#******************************
# IVA entrypoint for handling SFU/Coprocessor exception traps.
#******************************

    .export $exception_trap,entry
$exception_trap

#ifdef TIMEX_BUG
    fic 0(0,0)
#endif
    mfctl   iir,r31			#Grab the interrupted instruction.
#ifdef TIMEX_BUG
    mtctl   rp,tr4		# save rp before making call
    mfctl   CCR,rp		# make sure we have a timex before calling
    Bbclr,n rp,24,no_timex_to_fix	#fixup routine
    bl      fix_rptbug,rp	# call fixup routine saving r3
    mtctl   r3,tr3		#   in the delay slot
    mfctl   tr3,r3		# restore registers
no_timex_to_fix
    mfctl   tr4,rp
#endif
    mtctl   r30,tr3
#ifdef TIMEX
    extru   r31,5,6,r30		# get major opcode and check for 0C
    comibf,= 0xc,r30,$excphandler	# not 0C, so no UID to check
    mfctl   tr3,r30
#endif

    extru,<> r31,uidposition,3,r0	#Skip when uid is not zero
    b,n       $excphandler		#Process multiply/divide and
					#floating-point instructions
    .call
    b	    $nonzerohandler
    nop

#*****************************************************************************
# Multply/divide SFU and floating-point coprocessor decode.  
#  This is entered from the interrupt vector table code.  Registers 31 & 30
#  have been saved in TR2 & TR3 respectively.
#*****************************************************************************
$excphandler
    mtctl   r29,TR4

    #Use a modified C calling convention.  Only provide the callee
    #needed structures: function-value return and argument list.  The
    #'general register save' and 'status save area' caller used 
    #structures are combined into a single save area.  The 'register spill'
    #and 'local storage' areas are not needed.  'space and floating-point
    #register save' areas are not currently needed.

    #If this routine is upgraded to allow interrupts a much longer and
    # involved sequence will be necessary.

#if defined(MACH_KERNEL)
    #If we're already on the interrupt stack (istackptr==0), then we
    #simply use the current sp.  Otherwise, use the value in istackptr,
    #which can contain only one other valid value: the address of istack.
    #If first frame on the stack, zero out the istackptr.
    ldil    L'istackptr,r29
    ldw     R'istackptr(r29),sp
    comb,<>,n 0,sp,istackatbase
    movb,tr sp,r29,istacknotatbase #Save istackptr
    mfctl   TR3,sp		   #Get sp back from temp register
istackatbase
    stw     0,R'istackptr(r29)
    copy    sp,r29		#Save istackptr
istacknotatbase

#else

    #Get a pointer to a local stack.
    ldil    L'$emulate_stack,sp
    ldo     R'$emulate_stack(sp),sp

#endif

    ldo     SPECIAL_FRAME_SIZE(sp),sp	#Set up save area.
    #Start saving away status and link registers.
    stw     r28,SPECIAL_FRAME_R28(sp)	
    stw     r27,SPECIAL_FRAME_R27(sp)
    stw     arg0,SPECIAL_FRAME_R26(sp)	#r26 (Proc Calling Conventions)
    stw     arg1,SPECIAL_FRAME_R25(sp)	#r25 (Proc Calling Conventions)
    stw     arg2,SPECIAL_FRAME_R24(sp)	#r24 (Proc Calling Conventions)
    stw     arg3,SPECIAL_FRAME_R23(sp)	#r23 (Proc Calling Conventions)
    stw     r22,SPECIAL_FRAME_R22(sp)	#Caller saves
    stw     r21,SPECIAL_FRAME_R21(sp)
    stw     r20,SPECIAL_FRAME_R20(sp)
    stw     r19,SPECIAL_FRAME_R19(sp)
    stw     r2,SPECIAL_FRAME_R2(sp)		#rp (Proc Calling Conventions)
				#End caller saves (Proc Calling Conventions)
				#and align on a double-word boundary.
				#Registers 29-31 are already saved
				#in temp control registers.
    stw     r1,SPECIAL_FRAME_R1(sp)

    mfctl   sar,arg0
    stw     arg0,SPECIAL_FRAME_SAR(sp)

#if defined(MACH_KERNEL)
    stw     r29,SPECIAL_FRAME_SAVE_ISTACKPTR(sp)
#endif

# Test for sfu instruction.
# Handle multiply/divide sfu instructions separately.
#ifdef TIMEX
# TIMEX defines new major opcodes.  The multiops do not set set bit 2.
# We must now check (bit 2 | bit 4) set to determine if it is a
# FPC instruction
    extru,<> r31,copropbit,1,r0 # check bit 2, if set nullify until fpuop
    extru,= r31,dblwdbit,1,r0   # check bit 4, if not set, execute branch
    or,TR r0,r0,r0              #   else nullify the branch to mdsfuop
#else
    extru,<> r31,copropbit,1,r0
#endif /* TIMEX */
    b,n    mdsfuop

#*****************************************************************************
# Fpuop - Floating-point coprocessor instruction.
#*****************************************************************************
fpuop
    #Establish addressability to the emulated floating-point registers.
    ldil    L'$fpu_register,arg0
    ldo     RFPUREGISTER(arg0),arg0

    #Copy coprocessor registers to emulation registers if the coprocessor
    #exists.  Otherwise they are already there.
    mfctl     CCR,arg2
    Bbclr     arg2,24,fpusaved
    copy      arg0,arg2

    fstds,ma  fr0,8(arg2)
    fstds,ma  fr1,8(arg2)
    fstds,ma  fr2,8(arg2)
    fstds,ma  fr3,8(arg2)
    fstds,ma  fr4,8(arg2)
    fstds,ma  fr5,8(arg2)
    fstds,ma  fr6,8(arg2)
    fstds,ma  fr7,8(arg2)
    fstds,ma  fr8,8(arg2)
    fstds,ma  fr9,8(arg2)
    fstds,ma  fr10,8(arg2)
    fstds,ma  fr11,8(arg2)
    fstds,ma  fr12,8(arg2)
    fstds,ma  fr13,8(arg2)
    fstds,ma  fr14,8(arg2)
#ifdef TIMEX
    fstds,ma  fr15,8(arg2) 
    .word	0x2f101230	# fstds,ma fr16,8(arg2)
    .word	0x2f101231	# fstds,ma fr17,8(arg2)
    .word	0x2f101232	# fstds,ma fr18,8(arg2)
    .word	0x2f101233	# fstds,ma fr19,8(arg2)
    .word	0x2f101234	# fstds,ma fr20,8(arg2)
    .word	0x2f101235	# fstds,ma fr21,8(arg2)
    .word	0x2f101236	# fstds,ma fr22,8(arg2)
    .word	0x2f101237	# fstds,ma fr23,8(arg2)
    .word	0x2f101238	# fstds,ma fr24,8(arg2)
    .word	0x2f101239	# fstds,ma fr25,8(arg2)
    .word	0x2f10123a	# fstds,ma fr26,8(arg2)
    .word	0x2f10123b	# fstds,ma fr27,8(arg2)
    .word	0x2f10123c	# fstds,ma fr28,8(arg2)
    .word	0x2f10123d	# fstds,ma fr29,8(arg2)
    .word	0x2f10123e	# fstds,ma fr30,8(arg2)
    .word	0x2f00121f	# fstds    fr31,0(arg2)
#else
    fstds     fr15,(arg2)
#endif /* TIMEX */

#ifdef FPC_BUG
#
# %%%% kludge for FPC hardware bug
#
    fcpy,dbl	fr4,fr4
#endif

fpusaved

    # The following registers contain:
    #      rp,   Return pointer
    #     r31,   general temporary register
    #     r30,   sp
    #     r29,   general temporary register
    #     r28-r27 unused
    #     r26,   arg0 - points to fpu_register
    #	  r25-r23 arg1-arg3 - general temporary registers
    #     r22-r3 unused
    #     r1     unused
    .call
    bl      decode_fpu,rp
    stw     arg2,SPECIAL_FRAME_SAVE_REG_PTR(sp)	#Save pointer to
						#floating-point registers
#****************************************************************
# Fpopreturn - floating-point return point.
#****************************************************************
    #Copy $fpu_register to coprocessor registers if the coprocessor
    #exists.
fpopreturn
    mfctl     CCR,arg2
    Bbclr     arg2,24,fpurstred
    ldw       SPECIAL_FRAME_SAVE_REG_PTR(sp),arg0 #Get pointer to
    						  #floating-point registers
#ifdef TIMEX
    .word     0x2f40101f	# fldds 0(arg0),fr31
    .word     0x2f51303e	# fldds,mb -8(arg0),fr30
    .word     0x2f51303d	# fldds,mb -8(arg0),fr29
    .word     0x2f51303c	# fldds,mb -8(arg0),fr28
    .word     0x2f51303b	# fldds,mb -8(arg0),fr27
    .word     0x2f51303a	# fldds,mb -8(arg0),fr26
    .word     0x2f513039	# fldds,mb -8(arg0),fr25
    .word     0x2f513038	# fldds,mb -8(arg0),fr24
    .word     0x2f513037	# fldds,mb -8(arg0),fr23
    .word     0x2f513036	# fldds,mb -8(arg0),fr22
    .word     0x2f513035	# fldds,mb -8(arg0),fr21
    .word     0x2f513034	# fldds,mb -8(arg0),fr20
    .word     0x2f513033	# fldds,mb -8(arg0),fr19
    .word     0x2f513032	# fldds,mb -8(arg0),fr18
    .word     0x2f513031	# fldds,mb -8(arg0),fr17
    .word     0x2f513030	# fldds,mb -8(arg0),fr16
    fldds,mb  -8(arg0),fr15 
#else
    fldds     (arg0),fr15
#endif /* TIMEX */
    fldds,mb  -8(arg0),fr14
    fldds,mb  -8(arg0),fr13
    fldds,mb  -8(arg0),fr12
    fldds,mb  -8(arg0),fr11
    fldds,mb  -8(arg0),fr10
    fldds,mb  -8(arg0),fr9
    fldds,mb  -8(arg0),fr8
    fldds,mb  -8(arg0),fr7
    fldds,mb  -8(arg0),fr6
    fldds,mb  -8(arg0),fr5
    fldds,mb  -8(arg0),fr4
    fldds,mb  -8(arg0),fr3
    fldds,mb  -8(arg0),fr2
    fldds,mb  -8(arg0),fr1
    fldds,mb  -8(arg0),fr0
fpurstred

    #restore all the registers which may have been modified.
finishup
    ldw      SPECIAL_FRAME_R19(0,sp),r19	
    ldw      SPECIAL_FRAME_R20(0,sp),r20
    ldw      SPECIAL_FRAME_R21(0,sp),r21
    ldw      SPECIAL_FRAME_R22(0,sp),r22
    ldw      SPECIAL_FRAME_R23(0,sp),r23		#arg3
    ldw      SPECIAL_FRAME_R24(0,sp),r24 		#arg2
    ldw      SPECIAL_FRAME_R25(0,sp),r25		#arg1
    ldw      SPECIAL_FRAME_R26(0,sp),r26		#arg0
    ldw      SPECIAL_FRAME_R27(0,sp),r27
    ldw	     SPECIAL_FRAME_R2(0,sp),r2
    ldw      SPECIAL_FRAME_R1(0,sp),r1
    ldw      SPECIAL_FRAME_SAR(0,sp),r31
    mtctl    r31,sar

#Need to determine if a trap really occurred.
    comib,<> 0,ret0,realtrap
    ldw      SPECIAL_FRAME_R28(0,sp),ret0

#if defined(MACH_KERNEL)
    ldw      SPECIAL_FRAME_SAVE_ISTACKPTR(sp),r31
    ldil     L'istackptr,r30
    stw      r31,R'istackptr(r30)
#endif

    mfctl    TR4,r29
    mfctl    TR3,r30
    mfctl    TR2,r31
    rfi
    nop

#*****************************************************************************
# Mdsfuop - Multiply/divide SFU instruction.
#*****************************************************************************
mdsfuop
#ifdef MDSFU_SUPPORTED
# OLD Code could go here!!!
#else
    movib,tr,n 1,ret0,finishup	# MPSFU not supported.  Force "realtrap".
    nop
#endif

#*****************************************************************************
realtrap
#if defined(MACH_KERNEL)
    ldw      SPECIAL_FRAME_SAVE_ISTACKPTR(sp),r31
    ldil     L'istackptr,r30
    stw      r31,R'istackptr(r30)
#endif

    mfctl   TR4,r29
    mfctl   TR3,r30
    mfctl   TR2,r31
    .call
    THANDLER(I_EXCEP)
    nop

#ifdef TIMEX_BUG

# fix_rptbug -- a routine to be called from the trap handler when there
# has been an exception trap.  It detects and corrects either of the
# two known mustang single-rpt bugs (I hope).
#
# It uses r3 and does not restore it.
#
# The trap handler must be very careful what fp instructions it does before
# calling this routine.  FP loads and stores should not be allowed to cache
# miss and they should be separated from eachother by at least 2 non-FP
# instructions.  Also, for best results the first instruction of the
# exception trap handler should be a d-cache instruction (e.g. fic 0(0,0)).
# This forces any streamed d-misses to complete.
#
# This routine returns to the location pointed to by r2.  All fp and cpu
# registers and control registers will be the same on exit as they were
# on entry, except general register r3 is overwritten.
#
# Will Walker, July 11, 1990


fix_rptbug
                # save r1 and r4
                ldil            l'fixrpt_save,r3
                ldo             r'fixrpt_save(r3),r3
                stwm            r1,4(r3)
                stwm            r4,4(r3)

                # stash the fp status and clear the T bit
                ldil            l'fixrpt_incache,r4
                ldo             r'fixrpt_incache(r4),r4
                ldws            0(0,r4),r1      # pre-load D-cache
                nop
                nop
                nop
                fstds,ma        0,8(0,r4)       # clear T-bit, save status
                nop
                nop

                # now try to detect the bug by putting known values in
                # fr4 and fr5, storing them out & checking the result

                copy            4,4
                copy            4,4
                copy            4,4
                fstds,ma        fr4,8(0,r3)     # save fr4
                copy            4,4
                copy            4,4

                ldil            l'0x01010101,r1
                ldo             r'0x01010101(r1),r1
                stws            r1,0(0,r4)
                fldws           0(0,r4),fr4     # fr4 <- 0x01010101
                nop
                nop

                copy            5,5
                copy            5,5
                copy            5,5
                fstds,ma        fr5,8(0,r3)     # save fr5
                copy            5,5
                copy            5,5

                ldil            l'0x02020202,r1
                ldo             r'0x02020202(r1),r1
                stws            r1,0(0,r4)
                fldws           0(0,r4),fr5     # fr5 <- 0x02020202
                nop
                nop

                fstws           fr4,0(0,r4)     # store & check
                fstws           fr5,4(0,r4)
                ldws            4(0,r4),r3
                combt,=,n       r1,r3,fixrpt_ok

                # The single-rpt bug has happened -- fix it.
                # Cause a floating-point exception during a load d-miss,
                # with an fp store right after the exception is signalled.

                ldil            l'fixrpt_save,r3
                ldo             r'fixrpt_save(r3),r3
                addi            24,r3,r3        # already saved r1,r4,fr4,fr5
                mfctl           cr14,r1         # save IVA
                stw             r1,0(r3)
                mfctl           cr22,r1         # save ipsw
                stw             r1,4(r3)

                ldil            l'fixrpt_iva,r1 # set fake iva
                ldo             r'fixrpt_iva(r1),r1
                mtctl           r1,cr14

                ldil            l'fixrpt_incache,r4
                ldo             r'fixrpt_incache(r4),r4
                ldw             0(0,r4),r1      # get fp status
                depi            1,25,1,r1       # set T bit
                stw             r1,8(0,r4)
                ldw             4(0,r4),r1      # get exc. reg. 1
                depi            1,31,1,r1       # set lsb
                stw             r1,12(0,r4)
                stw             r1,4(0,r4)
                fldds           8(0,r4),fr0

                fdc             0(0,r4)         # flush the line
                sync
                nop
                nop
                nop
                nop
                ldws            0(r4),r1        # load d-miss
                fadd,dbl        0,0,0           # signal fp exception
                fstds           5,8(r4)
                break           0,0             # should never get here

fixrpt_ok       ldil            l'fixrpt_save,r3
                ldo             r'fixrpt_save(r3),r3
                ldws,ma         4(r3),r1
                ldws,ma         4(r3),r4
                fldds,ma        8(r3),fr4
                fldds,ma        8(r3),fr5
                ldil            l'fixrpt_incache,r3
                ldo             r'fixrpt_incache(r3),r3
                bv              0(2)
                fldds           0(r3),0         # restore fp status

                .align 0x800
fixrpt_iva      .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32

fixrpt_fpe      ldw             0(r3),r1        # restore iva
                mtctl           r1,cr14
                ldw             4(r3),r1        # restore ipsw
                mtctl           r1,cr22
                b               fixrpt_ok
                fstds,ma        0,0(r4)         # clear T bit
                .blockz         8

                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32
                .blockz         32


	.data
	.align 0x800
_LOAD_$FIXRPTDATA
_CACHE_$FIXRPTDATA
                .align 32
fixrpt_incache  .blockz 32
fixrpt_save     .blockz 32

#endif /* TIMEX_BUG */

	.SPACE	$PRIVATE$
	.SUBSPA $BSS$

    .import $fpu_register
#ifdef MDSFU_SUPPORTED
    .import $mdsfu_register
#endif

#if defined(MACH_KERNEL)
    .import istackptr
#else
    .import $emulate_stack	#Space for special C environment stack
				#Used for testing purposes.
#endif
