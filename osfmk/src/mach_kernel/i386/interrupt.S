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
 * Revision 2.14.4.2  92/04/30  11:50:17  bernadat
 * 	Adaptations for Corollary and Systempro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.14.4.1  92/02/18  18:46:34  jeffreyh
 * 	Adapt slave interrupt code to new interrupt handling for Corollary
 * 	[91/08/22            bernadat]
 * 
 * 	Support for the Corollary MP
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.14  91/10/07  17:24:48  af
 * 	From mg32: testing for spurious interrupts is bogus.
 * 	[91/09/23            rvb]
 * 
 * Revision 2.13  91/08/28  21:31:06  jsb
 * 	Check for out-of-range interrupts.
 * 	[91/08/20            dbg]
 * 
 * Revision 2.12  91/07/31  17:37:31  dbg
 * 	Support separate interrupt stack.  Interrupt handler may now be
 * 	called from different places.
 * 	[91/07/30  16:52:19  dbg]
 * 
 * Revision 2.11  91/06/19  11:55:12  rvb
 * 	cputypes.h->platforms.h
 * 	[91/06/12  13:44:55  rvb]
 * 
 * Revision 2.10  91/05/14  16:09:08  mrt
 * 	Correcting copyright
 * 
 * Revision 2.9  91/05/08  12:38:18  dbg
 * 	Put parentheses around substituted immediate expressions, so
 * 	that they will pass through the GNU preprocessor.
 * 
 * 	Use platforms.h.  Call version of set_spl that leaves interrupts
 * 	disabled (IF clear) until iret.
 * 	[91/04/26  14:35:53  dbg]
 * 
 * Revision 2.8  91/02/05  17:12:22  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:34:58  mrt]
 * 
 * Revision 2.7  91/01/08  17:32:06  rpd
 * 	Need special interrupt_return
 * 	[90/12/21  14:36:12  rvb]
 * 
 * Revision 2.6  90/12/20  16:35:58  jeffreyh
 * 	Changes for __STDC__
 * 	[90/12/07  15:43:38  jeffreyh]
 * 
 * Revision 2.5  90/12/04  14:46:08  jsb
 * 	iPSC2 -> iPSC386.
 * 	[90/12/04  11:16:47  jsb]
 * 
 * Revision 2.4  90/11/26  14:48:33  rvb
 * 	Change Prime copyright as per Peter J. Weyman authorization.
 * 	[90/11/19            rvb]
 * 
 * Revision 2.3  90/09/23  17:45:14  jsb
 * 	Added support for iPSC2.
 * 	[90/09/21  16:40:09  jsb]
 * 
 * Revision 2.2  90/05/03  15:27:54  dbg
 * 	Stole from Prime.
 * 	Pass new parameters to clock_interrupt (no longer called
 * 	hardclock).  Set curr_ipl correctly around call to clock_interrupt.
 * 	Moved softclock logic to splx.
 * 	Added kdb_kintr to find registers for kdb.
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
#include <platforms.h>
#include <mp_v1_1.h>
#include <etap_event_monitor.h>

#include <i386/asm.h>
#include <i386/ipl.h>
#include <i386/pic.h>
#include <assym.s>

#if	AT386
#include <i386/AT386/mp/mp.h>
#endif	/* AT386 */

#ifdef	CBUS
#include <busses/cbus/cbus.h>
#endif	/* CBUS */

#ifdef	MBUS
#include <busses/mbus/mbus.h>
#endif	/* MBUS */

#if	ETAP_EVENT_MONITOR
#define ETAP_INTERRUPT_PROBE(intr, flg)			\
	pushal					;	\
	pushl	flg				;	\
	pushl	intr				;	\
	call	EXT(etap_interrupt_probe)	;	\
	add	$8,%esp				;	\
	popal
#else	
#define ETAP_INTERRUPT_PROBE(intr, flg)
#endif			

#define BEGIN_INTR_HDLR		1
#define END_INTR_HDLR		0	
		
#if	MP_V1_1
#define TRAP_DEBUG 0 /* Must match spl.s and mp_v1_1.c */
#endif	/* MP_V1_1 */

#if	NCPUS > 1
#define	CX(addr,reg)	addr(,reg,4)
#else	/* NCPUS > 1 */
#define	CPU_NUMBER(reg)
#define	CX(addr,reg)	addr
#endif	/* NCPUS > 1 */

/*
 *	Called from locore.s.  The register save area is on top
 *	of the stack.  %eax contains the interrupt number.
 *	Only %ecx and %edx have been saved.
 */
Entry(interrupt)

	movl	%eax,%ecx		# stash interrupt vector number
#if	MP_V1_1
	movl	EXT(mp_v1_1_initialized), %eax
	cmpl	$0, %eax
	jne	mpv1_intr
#else	/* MP_V1_1 */
#if	NCPUS > 1 && AT386
	CPU_NUMBER(%eax)
	cmpl	$0, %eax		/* No PIC on slave processors */
	jne	EXT(slave_interrupt)
#endif	/* NCPUS > 1 && AT386 */
#endif	/* MP_V1_1 */

	subl	$0x40,%ecx		# interrupt vectors we use
					# start at 0x40, not 0
	jl	int_range_err		# and are between 0x40
	cmpl	$15,%ecx		# and 0x4f
	jg	int_range_err

	

/*
 *	check for spurious interrupt
 */

	movl	EXT(master_icw), %edx
#ifdef	AT386
	cmpl	$2, %ecx		# ATs slave the second pic on IRQ2
	je	interrupt_return
#endif	/* AT386 */

	cmpl	$7, %ecx		
#if	AT386
	je	int_check
#else	/* AT386 */
	je	interrupt_return	# iPSCs slave the second pic on IRQ7
#endif	/* AT386 */

	cmpl	$15, %ecx		# IRQ15
	jne	int_ok
	addw	$(SIZE_PIC),%dx		#  2
int_check:
	movw	$(OCW_TEMPLATE | READ_NEXT_RD | READ_IS_ONRD), %ax
	outb	%al,%dx
	inb	%dx,%al			# read ISR
	testb	$0x80, %al		# return if IS7 is off
	jz	interrupt_return


/*
 *	Now we must acknowledge the interrupt and issue an EOI command to
 *	the pics.  We send a NON-SPECIFIC EOI, as we assume that the pic
 *	automatically interrupts us with only the highest priority interrupt.
 */

	movl	EXT(master_icw),%edx	#  2	EOI for master.
int_ok:
	movw	EXT(PICM_OCW2),%ax	#  2
	outb	%al,%dx			#  4

	movw	EXT(PICS_OCW2),%ax	#  2	EOI for slave.
	addw	$(SIZE_PIC),%dx		#  2
	outb	%al,%dx			#  4

/*
 *	Now we must change the interrupt priority level, with interrupts
 *	turned off.  First we get the interrupt number and get
 *	the interrupt level associated with it, then we call set_spl().
 */


	movzbl	EXT(intpri)(%ecx), %eax	#  4	intpri[int#]  
					# we ought to save ecx here, but set_spl 
					# kindly does it for us 
	call	EXT(set_spl)		# interrupts are enabled

/*
 *	Interrupts are now enabled.  Call the relevant interrupt
 *	handler as per the ivect[] array set up in pic_init.
 */

	ETAP_INTERRUPT_PROBE(%ecx, $BEGIN_INTR_HDLR)

	pushl	%eax			#  2	save old IPL
	pushl	EXT(iunit)(,%ecx,4)	#  2	push unit# as int handler arg
	call	*EXT(ivect)(,%ecx,4)	#  4	*ivect[int#]()
return_from_interrupt:
	addl	$4,%esp			# remove interrupt number from stack
	cli				#  3	disable interrupts

	ETAP_INTERRUPT_PROBE($0, $END_INTR_HDLR)

/*
 *	5. Having dealt with the interrupt now we must return to the previous
 *	interrupt priority level.  This is done with interrupts turned off.
 */

	popl	%eax			# get old IPL from stack
	MP_DISABLE_PREEMPTION(%edx)
	CPU_NUMBER(%edx)
	cmpl	CX(EXT(curr_ipl),%edx), %eax
	je	no_splx
	MP_ENABLE_PREEMPTION(%edx)
	call	EXT(set_spl_noi)	# reset IPL to old value
					# leaving IF off.
no_splx:
	MP_ENABLE_PREEMPTION(%edx)

/*
 *	Return to caller.
 */

interrupt_return:
	ret

#if	MP_V1_1
mpv1_intr:
	MP_DISABLE_PREEMPTION(%edx)
	CPU_NUMBER(%edx)
	movl	CX(EXT(curr_ipl),%edx), %eax
	pushl	%eax			# Save old ipl
#if	TRAP_DEBUG
	movl	EXT(cur_trap_hist), %edx
	movb	$02, 0(%edx)
	movb	%cl, 1(%edx)
	movb	%al, 2(%edx)
	movb	$0, 3(%edx)
	movb	$0, 4(%edx)
#endif	/* TRAP_DEBUG */
	movl	%ecx, %eax
	subl	$0x50, %eax
	jl	2f
	shrl	$4, %eax
#if	TRAP_DEBUG
	movb	%al, 3(%edx)
	movb	$0, 5(%edx)
	addl	$6, EXT(cur_trap_hist)
	movl	EXT(max_trap_hist), %edx
	cmpl	%edx, EXT(cur_trap_hist)
	jl	8f
	leal	EXT(trap_hist), %edx
	movl	%edx, EXT(cur_trap_hist)
8:
	CPU_NUMBER(%edx)
#endif	/* TRAP_DEBUG */
	movl	%eax, CX(EXT(curr_ipl),%edx)
	MP_ENABLE_PREEMPTION(%edx)
	movl	EXT(lapic_start), %edx
	movl	%ecx, LAPIC_TPR(%edx)	# Set/Get Task priority (spl)
	subl	$0x50, %ecx		# XXX Must match mp_v1_1.c
	sti				# Enable interrupts
	pushl	EXT(mp_v1_1_unit)(,%ecx,4)
	call	*EXT(mp_v1_1_intr)(,%ecx,4)
	addl	$4, %esp
	cli				# Turn interrupts back off
	movl	EXT(lapic_start), %edx
	movl	$0, LAPIC_EOI(%edx)	# Acknowledge interrupt
#if	TRAP_DEBUG
	movl	LAPIC_TPR(%edx), %ecx
	movl	EXT(cur_trap_hist), %edx
	movb	$2, 0(%edx)
	movb	$0xff, 1(%edx)
	movb	$0, 2(%edx)
	movb	$0, 3(%edx)
	movb	%cl, 4(%edx)
#endif	/* TRAP_DEBUG */
	MP_DISABLE_PREEMPTION(%edx)
2:	popl	%ecx			# Get old spl
	CPU_NUMBER(%edx)
	movl	%ecx, CX(EXT(curr_ipl),%edx)
	MP_ENABLE_PREEMPTION(%edx)
	shll	$4, %ecx
	addl	$0x50, %ecx
	movl	EXT(lapic_start), %eax
	movl	%ecx, LAPIC_TPR(%eax)	# Set Task priority (spl)
#if	TRAP_DEBUG
	movl	EXT(cur_trap_hist), %edx
	movb	%cl, 5(%edx)
	addl	$6, EXT(cur_trap_hist)
	movl	EXT(max_trap_hist), %eax
	cmpl	%eax, EXT(cur_trap_hist)
	jl	1f
	leal	EXT(trap_hist), %eax
	movl	%eax, EXT(cur_trap_hist)
1:
#endif	/* TRAP_DEBUG */
	ret
#endif	/* MP_V1_1 */

/*
 * Interrupt number out of range.
 */
int_range_err:
	addl	$0x40,%ecx		# restore original interrupt number
#if	AT386
	cmpl	$2, %ecx		# int was a memory parity error
	jne	range_message
	pushl	%ecx			# push number
	pushl	$int_parity_message	# push message
	jmp	int_range_panic
range_message:
#endif
	pushl	%ecx			# push number
	pushl	$int_range_message	# push message
int_range_panic:
	call	EXT(panic)		# panic
	addl	$8,%esp			# pop stack
	ret				# return to caller

int_range_message:
	.ascii	"Bad interrupt number %#x"
	.byte	0

int_parity_message:
	.ascii	"Memory parity error interrupt"
	.byte	0


#if	NCPUS > 1 && AT386 && !MP_V1_1

Entry(slave_interrupt)
	shl	$2, %eax
	addl	$EXT(curr_ipl), %eax
	pushl	(%eax)
	movl	$(MP_IPL), (%eax)
	pushl	%ecx			# push vector as int handler arg
	call	EXT(mp_intr)
	jmp	return_from_interrupt

#endif	/* NCPUS > 1 && AT386 && !MP_V1_1*/
