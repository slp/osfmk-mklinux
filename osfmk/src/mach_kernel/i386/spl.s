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
/* CMU_HIST */
/*
 * Revision 2.8.8.3  92/09/15  17:14:57  jeffreyh
 * 	Changes for profiling of kernel task & threads.
 * 	Arrange so that profiling is accurate even when
 * 	masked against the clock (ie: let clock interrupt
 * 	raise but deffer the calll to mach_clock()). Also
 * 	Let "sti" be last codeop before "ret" so that
 * 	caller gets sampled when lowering ipl level. interrupts
 * 	usually pop at sti+2 instructions.
 * 	[92/07/24            bernadat]
 * 
 * Revision 2.8.8.2  92/04/30  11:51:06  bernadat
 * 	Adaptations for Corollary and Systempro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.8.8.1  92/02/18  18:49:36  jeffreyh
 * 	Fixed slave interrupt code for new interrupt interface
 * 	Must not overwrite %ecx in handler
 * 	[91/08/22            bernadat]
 * 
 * 	Support for the Corollary MP
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.8  91/06/06  17:04:01  jsb
 * 	Added spldcm for i386ipsc.
 * 	[91/05/13  16:53:38  jsb]
 * 
 * Revision 2.7  91/05/14  16:16:41  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/05/08  12:42:29  dbg
 * 	Put parentheses around substituted immediate expressions, so
 * 	that they will pass through the GNU preprocessor.
 * 
 * 	Add set_spl_noi to reset PIC masks but leave interrupts disabled
 * 	(IF clear).
 * 	[91/04/26  14:38:31  dbg]
 * 
 * Revision 2.5  91/02/05  17:14:45  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:38:06  mrt]
 * 
 * Revision 2.4  90/12/20  16:36:50  jeffreyh
 * 	Changes for __STDC__
 * 	[90/12/07            jeffreyh]
 * 
 * Revision 2.3  90/11/26  14:48:50  rvb
 * 	Change Prime copyright as per Peter J. Weyman authorization.
 * 	[90/11/19            rvb]
 * 
 * Revision 2.2  90/05/03  15:37:36  dbg
 * 	Stole from Prime.
 * 	[90/02/14            dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
/*
 */

/*
Copyright (c) 1988,1989 Prime Computer, Inc.  Natick, MA 01760
All Rights Reserved.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and
without fee is hereby granted, provided that the above
copyright notice appears in all copies and that both the
copyright notice and this permission notice appear in
supporting documentation, and that the name of Prime
Computer, Inc. not be used in advertising or publicity
pertaining to distribution of the software without
specific, written prior permission.

THIS SOFTWARE IS PROVIDED "AS IS", AND PRIME COMPUTER,
INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
NO EVENT SHALL PRIME COMPUTER, INC.  BE LIABLE FOR ANY
SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN ACTION OF CONTRACT, NEGLIGENCE, OR
OTHER TORTIOUS ACTION, ARISING OUR OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <cpus.h>
#include <mach_kprof.h>
#include <platforms.h>
#include <mp_v1_1.h>

#include <assym.s>
#include <i386/asm.h>
#include <i386/ipl.h>
#include <i386/pic.h>

#if	MP_V1_1
#define	TRAP_DEBUG 0 /* Must match interrupt.s and mp_v1_1.c */
#endif	/* MP_V1_1 */

#if	AT386
#include <i386/AT386/mp/mp.h>

#if	NCPUS > 1

#define	CX(addr,reg)	addr(,reg,4)

#else	/* NCPUS > 1 */

#define	CPU_NUMBER(reg)
#define	CX(addr,reg)	addr

#endif	/* NCPUS > 1 */
#endif	/* AT386 */

///////////////////////////////////////////////////////////////////////////////
/	SET_PIC_MASK : this routine is run to set the pic masks.  It is 
/	implemented as a macro for efficiency reasons.
///////////////////////////////////////////////////////////////////////////////

#define SET_PIC_MASK							\
		movl	EXT(master_ocw),%edx	;			\
		OUTB				;			\
		addw	$(SIZE_PIC),%dx		;			\
		movb	%ah,%al			;			\
		OUTB

///////////////////////////////////////////////////////////////////////////////
/	SPLn : change interrupt priority level to that in %eax
/	SPLOFF : disable all interrupts, saving interrupt flag
/	SPLON: re-enable interrupt flag, undoes sploff()
/	Warning: SPLn and SPLOFF aren`t nestable.  That is,
/		a = sploff();
/		...
/		b = splmumble();
/		...
/		splx(b);
/		...
/		splon(a);
/	is going to do the wrong thing.
///////////////////////////////////////////////////////////////////////////////

Entry(spllo)
Entry(spllow)
	movl    $(SPL0), %eax
	jmp	EXT(set_spl)

Entry(splbio)
	movl    $(SPL5), %eax
	jmp	EXT(set_spl)

Entry(spltty)
Entry(splnet)
Entry(splimp)
	movl    $(SPL6), %eax
	jmp	EXT(set_spl)

Entry(splclock)
Entry(splvm)
Entry(splsched)

#if	MACH_KPROF
	/*
	 * If profiling is on, let clock interrupts raise even when mask
	 * against it
	 */

	movl	$(SPL7), %eax
	jmp	EXT(set_spl)
#endif	/* MACH_KPROF */

Entry(splhigh)
Entry(splhi)
	cli				/ disable interrupts
	MP_DISABLE_PREEMPTION(%edx)
	CPU_NUMBER(%edx)
	movl	CX(EXT(curr_ipl), %edx), %eax
	movl	$(IPLHI), CX(EXT(curr_ipl), %edx)
	MP_ENABLE_PREEMPTION(%edx)
	ret

Entry(getspl)
	MP_DISABLE_PREEMPTION(%edx)
	CPU_NUMBER(%edx)
	movl	CX(EXT(curr_ipl), %edx), %eax
	MP_ENABLE_PREEMPTION(%edx)
	ret

Entry(sploff)
	pushf				/ Flags reg NOT accessable
	popl	%eax			/ push onto stk, pop it into reg.
	movb	%ah,%al
	andb	$2,%al
	cli				/ DISABLE ALL INTERRUPTS
	ret


Entry(splon)
	pushf
	popl	%eax
	orb	4(%esp),%ah
	pushl	%eax			/ Flags regs not directly settable.
	popf				/ push value, pop into flags reg.
					/ IF ints were enabled before 
					/ then they are re-enabled now.
	ret


#if	MACH_KPROF
/*
 * If profiling is on, we let clock interrupts happen at any time
 * (except splhi) but then check if we are masked against it.
 * If yes we just memorize the PC so that  profiling is accurate.
 *  So at splx time we must call the
 * real clock handler (mach_clock) if this happened
 */

#define	CHECK_MISSED_CLOCK(cpu_reg)			\
	cmpl	$SPL7, %eax			   ;	\
	jge	0f				   ;	\
	cmpl	$0, CX(EXT(missed_clock), cpu_reg) ;	\
	je	0f				   ;	\
	pushl	%edx				   ;	\
	pushl	%eax				   ;	\
	pushl	%ecx				   ;	\
	call	EXT(delayed_clock)		   ;	\
	popl	%ecx				   ;	\
	popl	%eax				   ;	\
	popl	%edx				   ;	\
0:
#else	/* MACH_KPROF */
#define CHECK_MISSED_CLOCK(cpu_reg)
#endif	/* MACH_KPROF */

////////////////////////////////////////////////////////////////////////////////
/	 SPL : this routine sets the interrupt priority level, it is called from
/	 one of the above spln subroutines ONLY, NO-ONE should EVER call set_spl
/	 directly as it assumes that the parameters passed in are gospel.
/	SPLX	: This routine is used to set the ipl BACK to a level it was at
/	before spln() was called, which in turn calls set_spl(), which returns
/	(via %eax) the value of the curr_ipl prior to spln() being called, this 
/	routine is passed this level and so must check that it makes sense and 
/	if so then simply calls set_spl() with the appropriate interrupt level.
///////////////////////////////////////////////////////////////////////////////

Entry(splx)
	movl	0x04(%esp),%eax			/ get interrupt level from stack

#if	!MP_V1_1
#if     (NCPUS > 1 && AT386)
        CPU_NUMBER(%edx)
        cmpl    CX(EXT(curr_ipl), %edx), %eax
#else   /* NCPUS > 1 && AT386 */
        cmpl    EXT(curr_ipl), %eax		/  if equal to the current IPL,
#endif  /* NCPUS > 1 && AT386 */
        je      spl_done			/       nothing to do
#endif	/* !MP_V1_1 */

	cmpl	$0x00,%eax			/ check if  < 0
	jl	splxpanic

	cmpl	$(SPLHI),%eax			/ check if too high
	ja	splxpanic

///////////////////////////////////////////////////////////////////////////////
/	SET_SPL : This routine sets curr_spl, ipl, and the pic_mask as 
/	appropriate, basically given an spl# to adopt, see if it is the same as
/	the  old spl#, if so return. If the numbers are different, then set 
/	curr_spl, now check the corresponding ipl for the spl requested, if they
/	are the same then return otherwise set the new ipl and set the pic masks
/	accordingly.
///////////////////////////////////////////////////////////////////////////////

	.globl	EXT(set_spl)
LEXT(set_spl)

	pushl	%ebp
	movl	%esp,%ebp
	cli				/ disable interrupts
	MP_DISABLE_PREEMPTION(%edx)
	CPU_NUMBER(%edx)
	CHECK_MISSED_CLOCK(%edx)
#if	NCPUS > 1 && AT386 && !MP_V1_1
	cmpl	$0, %edx
	jne	slave_set_spl
#endif	/* NCPUS > 1 && AT386 && !MP_V1_1 */
	movl	CX(EXT(curr_ipl), %edx), %edx	/ get OLD ipl level
	pushl	%edx			/ save old level for return
	CPU_NUMBER(%edx)
	movl	%eax,CX(EXT(curr_ipl),%edx)/ set NEW ipl level
#if	!MP_V1_1
	cmpl	$(SPLHI), %eax		/ if SPLHI
	jne	1f
	MP_ENABLE_PREEMPTION(%edx)
	popl	%eax			/ return old level
	pop	%ebp
	ret				/ return with interrupt off
1:
#else	/* !MP_V1_1 */
	movl	EXT(mp_v1_1_initialized),%edx
	cmpl	$0, %edx
	je	1f

#if	TRAP_DEBUG
	pushl	%ecx
	movl	EXT(cur_trap_hist), %ecx
	movb	$01, 0(%ecx)
	movb	$00, 5(%ecx)
	movb	%al, 1(%ecx)
	movl	4(%esp), %edx
	movb	%dl, 2(%ecx)
#endif
	shll	$4, %eax		/ Switch to task priority
	addl	$0x5f, %eax
	movl	EXT(lapic_start), %edx	/ Get lapic address
#if	TRAP_DEBUG
	movb	%al, 3(%ecx)
	movl	LAPIC_TPR(%edx), %edx
	movb	%dl, 4(%ecx)
	popl	%ecx
	addl	$6, EXT(cur_trap_hist)
	movl	EXT(max_trap_hist), %edx
	cmpl	%edx, EXT(cur_trap_hist)
	jl	3f
	leal	EXT(trap_hist), %edx
	movl	%edx, EXT(cur_trap_hist)
3:
	movl	EXT(lapic_start), %edx	/ Get lapic address
#endif	/* TRAP_DEBUG */
	movl	%eax, LAPIC_TPR(%edx)	/ Save priority
	jmp	2f
#endif	/* !MP_V1_1 */

1:	movw	EXT(pic_mask)(,%eax,2), %ax
	cmpw	EXT(curr_pic_mask),%ax
	je	2f
	movw	%ax, EXT(curr_pic_mask)
	SET_PIC_MASK

2:	popl	%eax			/ return old level
	pop	%ebp
	sti
spl_done:
	MP_ENABLE_PREEMPTION(%edx)
	ret

#if	NCPUS > 1 && AT386 && !MP_V1_1
slave_set_spl:
	xchgl	CX(EXT(curr_ipl), %edx), %eax
	cmpl	$(MP_IPL), CX(EXT(curr_ipl), %edx)
	jl	1f
	pop	%ebp
	MP_ENABLE_PREEMPTION(%edx)
	ret				/ return with interrupt masked
1:
	pop	%ebp
	sti
	MP_ENABLE_PREEMPTION(%edx)
	ret
#endif	/* NCPUS > 1 && AT386 && !MP_V1_1 */


splxpanic:
	MP_DISABLE_PREEMPTION(%edx)
#if	NCPUS > 1 && AT386
	CPU_NUMBER(%edx)
	shl	$2, %edx
	pushl	EXT(curr_ipl)(%edx)
#else	/* NCPUS > 1 && AT386 */
	pushl	EXT(curr_ipl)		/ current level
#endif	/* NCPUS > 1 && AT386 */
	pushl	%eax			/ new level
	pushl	$splxpanic2
	MP_ENABLE_PREEMPTION(%edx)
	call	EXT(panic)
	hlt

	.data
splxpanic2:
	String	"splx(old %x, new %x): logic error in locore.s\n"
	.byte	0
	.text


/*
 * Set SPL, but leave interrupts disabled.  Called when returning
 * from interrupt.  Interrupts are already disabled.
 * New interrupt level is in %eax;
 * can't be SPLHI.
 */
	.globl	EXT(set_spl_noi)
LEXT(set_spl_noi)
	MP_DISABLE_PREEMPTION(%edx)
	CPU_NUMBER(%edx)
	CHECK_MISSED_CLOCK(%edx)
#if	NCPUS > 1 && AT386 && !MP_V1_1
	CPU_NUMBER(%edx)
	cmpl	$0, %edx
	jne	slave_set_spl_noi
#endif	/* NCPUS > 1 && AT386 && !MP_V1_1 */
	movl	%eax,CX(EXT(curr_ipl),%edx)	/ set new SPL level
	movw	EXT(pic_mask)(,%eax,2), %ax	/ get new pic mask
	cmpw	EXT(curr_pic_mask),%ax	/ if different,
	je	1f
	movw	%ax,EXT(curr_pic_mask)	/ change it
	SET_PIC_MASK
1:
	MP_ENABLE_PREEMPTION(%edx)
	ret

#if	MP_V1_1
Entry(set_pic_mask)
	movl	0x4(%esp), %eax
	SET_PIC_MASK
	ret
#else	/* MP_V1_1 */
slave_set_spl_noi:
	shl	$2, %edx
	addl	$EXT(curr_ipl), %edx
	movl	%eax, (%edx)
	MP_ENABLE_PREEMPTION(%edx)
	ret
#endif	/* MP_V1_1 */

#include <mach_kdb.h>

#if	MACH_KDB

/*
 * Kernel debugger versions of the spl*() functions.  This allows breakpoints
 * in the spl*() functions.  XXX set_spl is not cloned and therefore
 * cannot be breakpointed.
 */

Entry(db_splhigh)
	cli					/ disable interrupts
	CPU_NUMBER(%edx)
	movl	CX(EXT(curr_ipl), %edx), %eax
	movl	$(IPLHI), CX(EXT(curr_ipl), %edx) 
	ret

Entry(db_splx)
	movl	0x04(%esp),%eax			/ get interrupt level from stack

	MP_DISABLE_PREEMPTION(%edx)
#if     (NCPUS > 1 && AT386)
        CPU_NUMBER(%edx)
        cmpl    EXT(curr_ipl)(, %edx, 4), %eax
#else   /* NCPUS > 1 && AT386 */
        cmpl    EXT(curr_ipl), %eax		/  if equal to the current IPL,
#endif  /* NCPUS > 1 && AT386 */
        je      spl_done			/       nothing to do

	MP_ENABLE_PREEMPTION(%edx)
	
	cmpl	$0x00,%eax			/ check if  < 0
	jl	splxpanic

	cmpl	$(SPLHI),%eax			/ check if too high
	ja	splxpanic

	jmp	EXT(set_spl)

#endif	/* MACH_KDB */
