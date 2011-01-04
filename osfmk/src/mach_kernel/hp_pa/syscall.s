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
 * Copyright (c) 1990,1991,1992 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: syscall.s 1.16 92/04/27$
 *	Author: Bob Wheeler, University of Utah CSS
 */

#include <mach_rt.h>		
#include <assym.s>
#include <kern/syscall_emulation.h>
#include <mach/exception.h>
#include <machine/thread.h>
#include <machine/pmap.h>
#include <machine/psw.h>
#include <machine/spl.h>
#include <mach/machine/vm_param.h>
#include <machine/asm.h>	
#include <etap.h>
#include <etap_event_monitor.h>

	.space	$PRIVATE$
	.subspa	$DATA$

	.import $global$
	.import	cpu_data
	.import	active_pcbs
	.import mach_trap_count
	.import mach_trap_table
	.import need_ast

$argcount_str
        .string "argument count for system call greater than 12\x00"

#if DEBUG
$badspl_str
        .string "returning from syscall with a bad spl level\x00"
#endif

	.space	$TEXT$
	.subspa $CODE$

	.import panic

/*
 * system calls put an hp700_kernel_state structure plus standard call frame
 * at the bottom of the kernel stack (initial entry point into kernel).
 */
#ifdef 	DEBUG
#define TF_SIZE		roundup((FM_SIZE+FM_SIZE+SS_SIZE+ARG_SIZE),FR_ROUND)
#else	/* DEBUG */
#define TF_SIZE		roundup((FM_SIZE+SS_SIZE+ARG_SIZE),FR_ROUND)
#endif	/* DEBUG */
#define KERNEL_STACK_MASK  (KERNEL_STACK_SIZE-1)

/*
 * syscall
 * 
 * Determine what kind of a system call we are dealing with and either handle
 * it internally or invoke the server for unix syscalls.
 */

	.align  HP700_PGBYTES
	
	.export $syscall
$syscall

	/*
	 * We can trash any caller saved register except r22 (t1) which is used
	 * to pass the system call number. We will need to save r31 and sr0
	 * before we call anything since they are caller saves registers.
	 */

	/*
	 * Get the active thread and set up the address of the recovery
	 * pointer should the thread get into trouble. Set sr1 to be the 
	 * kernel's space. Until we get the space registers set up we need
	 * to explicitly use sr1.
 	 */
	ldw	R%cpu_data(sr1,r1),r1

	/*
	 * save the current user's stack pointer
	 */
	copy	sp,t4

	/*
	 * We store user state in the PCB for the thread. Grab a pointer
	 * to the saved state structure within the PCB. Also grab the
	 * kernel stack pointer, and add a call frame to it.
	 *
	 * Interleaved with this we also set the saved state flags.
	 */
	ldw	THREAD_TOP_ACT(sr1,r1),t2
	ldw	ACT_PCB(sr1,t2),t3
	ldi	SS_INSYSCALL,t2

	ldw	PCB_KSP(sr1,t3),sp
	stw	t2,SS_FLAGS+PCB_SS(sr1,t3)
	ldo	KF_SIZE(sp),sp
	stw	r0,PCB_KSP(sr1,t3)

	/*
	 * save the relevent user state starting with user's sp
	 */
	stw	t4,SS_R30+PCB_SS(sr1,t3)

	/*
	 * Save all the callee save registers since we might return via
	 * thread_syscall_return().
	 */
	stw	r3,SS_R3+PCB_SS(sr1,t3)
	stw	r4,SS_R4+PCB_SS(sr1,t3)
	stw	r5,SS_R5+PCB_SS(sr1,t3)
	stw	r6,SS_R6+PCB_SS(sr1,t3)
	stw	r7,SS_R7+PCB_SS(sr1,t3)
	stw	r8,SS_R8+PCB_SS(sr1,t3)
	stw	r9,SS_R9+PCB_SS(sr1,t3)
	stw	r10,SS_R10+PCB_SS(sr1,t3)
	stw	r11,SS_R11+PCB_SS(sr1,t3)
	stw	r12,SS_R12+PCB_SS(sr1,t3)
	stw	r13,SS_R13+PCB_SS(sr1,t3)
	stw	r14,SS_R14+PCB_SS(sr1,t3)
	stw	r15,SS_R15+PCB_SS(sr1,t3)
	stw	r16,SS_R16+PCB_SS(sr1,t3)
	stw	r17,SS_R17+PCB_SS(sr1,t3)
	stw	r18,SS_R18+PCB_SS(sr1,t3)

	stw	r22,SS_R22+PCB_SS(sr1,t3)

	/*
	 * Save off arg regs so we can recontruct errors more easily.
	 */
	stw	r23,SS_R23+PCB_SS(sr1,t3)
	stw	r24,SS_R24+PCB_SS(sr1,t3)
	stw	r25,SS_R25+PCB_SS(sr1,t3)
	stw	r26,SS_R26+PCB_SS(sr1,t3)

	/*
	 * save user's stub return address, data pointer and return pointer
	 */
	stw	r31,SS_R31+PCB_SS(sr1,t3)
	stw	r27,SS_R27+PCB_SS(sr1,t3)

	extru,<> r31,31,2,r0
	b       $already_k
	stw	r2,SS_R2+PCB_SS(sr1,t3)

	mfctl	pidr3,t2
	mfctl	pidr4,t4
	stw	t2,SS_CR12+PCB_SS(sr1,t3)
	mfctl	pidr1,t2
	stw	t4,SS_CR13+PCB_SS(sr1,t3)
	stw	t2,SS_CR8+PCB_SS(sr1,t3)
	
	mfsp	sr4,t4
	mfsp	sr5,t2
	stw	t4,SS_SR4+PCB_SS(sr1,t3)
	mfsp	sr6,t4
	stw	t2,SS_SR5+PCB_SS(sr1,t3)
	mfsp	sr7,t2
	stw	t4,SS_SR6+PCB_SS(sr1,t3)
	stw	t2,SS_SR7+PCB_SS(sr1,t3)

	/*
	 * now put the kernel protection ID into pidr3 and pidr4
	 */
	ldi	HP700_PID_KERNEL,t4
	mtctl	t4,pidr3
	mtctl	t4,pidr4

	/*
	 * initialize kernel state, set space registers to point to kernel area
	 */
	mtsp	r0,sr4
	mtsp	r0,sr5
	mtsp	r0,sr6
	mtsp	r0,sr7

$already_k

	/*
	 * save space registers so that we can get them pointing to the kernel.
	 * We need to save sr0 since it is a caller saves registers and it
	 * holds the space we need to return to.
	 */
	mfsp	sr0,t4
	mfsp	sr3,t2
	stw	t4,SS_SR0+PCB_SS(sr1,t3)
	stw	t2,SS_SR3+PCB_SS(sr1,t3)
		
	/*
	 * save the protection registers, we loaded pidr2 into r28 above
	 */
	stw	r28,SS_CR9+PCB_SS(sr1,t3)
	
	/*
	 * previous stack pointer points to saved state
	 */
	stw	r0,FM_PSP(sr1,sp)

	/*
	 * set data pointer for kernel
	 */
	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	/*
	 * OK, all the state has been saved, negate the system call code in t1
	 * If the original code was positive then it's a server syscall.
	 */
	sub,>	r0,t1,t1
	b,n	$call_server_syscall 

	/*
	 * Syscalls 33, 34, and 41 go to the server.  Subtracting 33 first
	 * makes it quicker to test for these values.
	 */
	ldo	-33(t1),t2
	comib,>>,n 2,t2,$call_server_syscall
	comib,=,n 41-33,t2,$call_server_syscall

	/*
	 * get the maximum mach trap number and see if this is at or below it
	 */
	ldil	L%mach_trap_count,t2
	ldw	R%mach_trap_count(t2),t2

	comb,>=,n t1,t2,$call_server_exception

	/*
	 * get the mach trap table
	 */
	ldil	L%mach_trap_table,t2
	ldo	R%mach_trap_table(t2),t2

	/*
	 * Now shift the syscall code to the left 4 bits to get an index into
	 * the mach_trap_table. The first word of each entry is the argument
	 * count, the next word is the offset for the routine.
	 */
	shd	t1,r0,28,t1
	add	t1,t2,rp

	/*
	 * rp now points to the entry in mach_trap_table for this system call.
	 * get the argument count for this call
	 */
	ldw	0(rp),t2

	/*
	 * If the argument count is less than or equal to 4 then all the 
	 * arguments are already in the registers. In that case just call 
	 * the routine.
	 *
	 * Get address of routine to call in the delay slot. 
	 */
	comib,>= 4,t2,$mach_call_call
	ldw	4(rp),ret0

	/*
	 * argh... some of the arguments that we need are back on the caller's
	 * frame on the user's stack. Let's go pick them up and move them into
	 * our frame. We have room for NARGS arguments. 4 in registers and the
	 * rest in the stack frame. Set t2 to be the number of arguments that
	 * we don't have to move. At the same time check if we need to move
	 * more than NARGS registers.
	 */
	subi,<	NARGS,t2,t2
	b	$mach_call_cont
	nop

	/*
	 * panic! Someone has modified kern/syscall_sw.c and not made the 
	 * corrections to this file.
	 */
	ldil	L%$argcount_str,arg0

	ldil	L%panic,t1
	ldo	R%panic(t1),t1
	.call 
	blr     r0,rp
	bv      0(t1)
	ldo	R%$argcount_str(arg0),arg0

	/*
	 * never returns...
	 */

$mach_call_cont

	/*
	 * set up the recovery point for an illegal user space access
	 */
	ldil	L%$mach_call_addr,t4
	ldo	R%$mach_call_addr(t4),t4
	stw	t4,THREAD_RECOVER(r1)

	/*
	 * first get the user's data space pointer, actually sr4 - sr6 should
	 * all be the same for the user side. Put the user space into sr1.
	 */
	ldw	SS_SR5(t3),t4
	mtsp	t4,sr1

	/*
	 * now move the correct number of arguments over from the user's stack
	 * to our stack. This is a really specialized form of copyin!
	 *
	 * Get users's stack pointer into t4 from within the delay slot.
	 */
	blr	t2,r0
	ldw	SS_R30(t3),t4

	ldw	VA_ARG11(sr1,t4),t2
	stw	t2,VA_ARG11(sp)

	ldw	VA_ARG10(sr1,t4),t2
	stw	t2,VA_ARG10(sp)

	ldw	VA_ARG9(sr1,t4),t2
	stw	t2,VA_ARG9(sp)

	ldw	VA_ARG8(sr1,t4),t2
	stw	t2,VA_ARG8(sp)

	ldw	VA_ARG7(sr1,t4),t2
	stw	t2,VA_ARG7(sp)

	ldw	VA_ARG6(sr1,t4),t2
	stw	t2,VA_ARG6(sp)

	ldw	VA_ARG5(sr1,t4),t2
	stw	t2,VA_ARG5(sp)

	ldw	VA_ARG4(sr1,t4),t2
	stw	t2,VA_ARG4(sp)

	/*
	 * clear the thread recovery since we're done touching user data.
	 * This way we won't come flying back here when there is a problem
	 * in the system call.
	 */
	stw	r0,THREAD_RECOVER(r1)


$mach_call_call

#if ETAP_EVENT_MONITOR
	.import etap_machcall_probe1
	.import etap_machcall_probe2

	ldi	32 << 4,t2		/* Don't record mach_msg event */
	comb,=,n t2,t1,$make_syscall

	/*
	 * Calling the etap probe functions will trash the caller-saves
	 * registers that the system-call handler will be expecting (arg0
	 * to arg3) as well as the handler address itself.  So stash these
	 * in the current frame and create a new one for the etap function
	 * to play with.
	 */
	stw	arg0,VA_ARG0(sp)
	stw	arg1,VA_ARG1(sp)
	stw	arg2,VA_ARG2(sp)
	stw	arg3,VA_ARG3(sp)
	stw	ret0,FM_EDP(sp)		/* XXX somewhat bogus */
	ldo	KF_SIZE(sp),sp
	.call
	bl	etap_machcall_probe1,rp
	copy	t1,arg0			/* System-call number << 4 */

	ldo	-KF_SIZE(sp),sp
	ldw	FM_EDP(sp),ret0
	ldw	VA_ARG3(sp),arg3
	ldw	VA_ARG2(sp),arg2
	ldw	VA_ARG1(sp),arg1
	ldw	VA_ARG0(sp),arg0
	.call
	blr	r0,rp			/* Call the system-call handler */
	bv,n	0(ret0)

	stw	ret0,FM_EDP(sp)
	.call
	bl	etap_machcall_probe2,rp
	ldo	KF_SIZE(sp),sp

	ldo	-KF_SIZE(sp),sp
	b	$syscall_return
	ldw	FM_EDP(sp),ret0

$make_syscall
#endif	/* ETAP_EVENT_MONITOR */

	/*
	 * call the system routine
	 */
	.call
	blr     r0,rp
	bv,n    0(ret0)

$syscall_return

	/*
	 * get the active thread's PCB pointer and a pointer to user state
	 */
	ldil	L%active_pcbs,t2
	ldw	R%active_pcbs(t2),t2
	ldo	PCB_SS(t2),t3

$thread_syscall_return

	/*
	 * Check for AST delivery.
	 * If need_ast is not zero then we need to load the lower privilege
	 * transfer trap bit in the PSW.  This will mean turning off the
	 * Q-bit to load the PC queues so that we can do an RFI and set
	 * the L bit.
	 */
	ldil	L%need_ast,t2
	ldw	R%need_ast(t2),t2
	comb,=  r0,t2,$syscall_noast
	ldw	SS_R31+PCB_SS(t3),r31 

	/*
	 * save the current system mask and clear all of it
	 */
	rsm	RESET_PSW,t2
	
	/*
	 * now add the lower privilege transfer bit and move to ipsw
	 */
	depi	1,PSW_L_P,1,t2
	depi	1,PSW_C_P,1,t2

	extru,=	r31,31,2,r0
	b      $u_ast
	mtctl	t2,ipsw

	depi	PC_PRIV_USER,31,2,r31 
	stw	r31,SS_R31+PCB_SS(t3)

	ldil	L%$kreturn,t2
	ldo	R%$kreturn(t2),t2

	mtctl	t2,pcoq
	ldo	4(t2),t2
	mtctl	t2,pcoq

	rfi
	nop

$u_ast
	/*
	 * load the space and offset queues
	 */
	mtctl	r0,pcsq
	mtctl	r0,pcsq

	ldil	L%$syscall_noast,t2
	ldo	R%$syscall_noast(t2),t2

	mtctl	t2,pcoq
	ldo	4(t2),t2
	mtctl	t2,pcoq

	rfi
	nop

$syscall_noast
	/*
	 * restore protection ID registers, we'll have to keep pidr2 pointing
	 * at the kernel until the last minute...
	 */
	mtsp	r0,sr1

	ldw	SS_CR12+PCB_SS(sr1,t3),t2
	ldw	SS_CR13+PCB_SS(sr1,t3),t4
	mtctl	t2,pidr3
	ldw	SS_CR8+PCB_SS(sr1,t3),t2
	mtctl	t4,pidr4		
	mtctl	t2,pidr1

	ldw	SS_SR7+PCB_SS(sr1,t3),t2
	ldw	SS_SR6+PCB_SS(sr1,t3),t4
	mtsp	t2,sr7
	ldw	SS_SR5+PCB_SS(sr1,t3),t2
	mtsp	t4,sr6
	ldw	SS_SR4+PCB_SS(sr1,t3),t4
	mtsp	t2,sr5
	mtsp	t4,sr4

$kreturn
	mtsp	r0,sr1

	ldw	SS_R27+PCB_SS(sr1,t3),r27
	ldw	SS_R2+PCB_SS(sr1,t3),r2

	/*
	 * restore space registers, this means that we need to start explicitly
	 * saying sr1.
	 */

	ldw	SS_SR0+PCB_SS(sr1,t3),t4	
	ldw	SS_SR3+PCB_SS(sr1,t3),t2
	mtsp	t4,sr0
	mtsp	t2,sr3
	
	/*
	 * next cut back the kernel stack and save the kernel stack pointer 
	 * in the PCB
	 */
	ldo	-KF_SIZE(sp),t4
	stw	t4,PCB_KSP(sr1,t3)

	/*
	 * restore the user's stack pointer
	 */
	ldw	SS_R30+PCB_SS(sr1,t3),sp

	/*
	 * BOGUS: The spec says thread_abort followed by thread_set_state is
	 * the proper way to invoke a signal handler while in a mach system
	 * call. However, our wonderful system call interface does not
	 * restore most of the registers, so this does not work. I am adding
	 * enough to make me and my code happy. Needs more thought.
	 */
	ldw	SS_R19+PCB_SS(sr1,t3),r19	
	ldw	SS_R21+PCB_SS(sr1,t3),r21	/* T1 and T2 also */
	ldw	SS_R22+PCB_SS(sr1,t3),r22
	ldw	SS_R23+PCB_SS(sr1,t3),r23
	ldw	SS_R24+PCB_SS(sr1,t3),r24
	ldw	SS_R25+PCB_SS(sr1,t3),r25
	ldw	SS_R26+PCB_SS(sr1,t3),r26

	/*
	 * If returning from server syscall, use iioqh to return
	 */

	sub,>=	r22,r0,r0
	b,n	$mach_syscall_ret
	
	ldw	SS_IIOQH+PCB_SS(sr1,t3),r31

$mach_syscall_ret
	/*
	 * finally return to the caller and set the protection ID correctly
	 */
	ldw	SS_CR9+PCB_SS(sr1,t3),t3
	.call
	be	0(sr0,r31)
	mtctl	t3,pidr2


$call_server_syscall

	stw	r31,SS_IIOQH+PCB_SS(sr1,t3)
	addi	4,r31,t2
	stw	t2,SS_IIOQT+PCB_SS(sr1,t3)		

$call_server_exception

	ldi	EXC_SYSCALL,arg0
	sub	r0,r22,arg1

	.import	doexception
	ldil	L%doexception,t1
	ldo	R%doexception(t1),t1
	.call
	blr     r0,rp
	bv      0(t1)
	ldi	1,arg2

/*
 * If we cause a page fault that can't be resolved when we copy the arguments
 * off of the user's stack we will end up here. Sort of a longjump from 
 * the pagefault handler. Anyway, our state is that right before the arguments
 * were copied.
 */

$mach_call_addr

	/*
	 * clear the thread recovery pointer
	 */
	stw	r0,THREAD_RECOVER(r1)

	/*
	 * call syscall_error(exception, code, subcode, saved_state *)
	 * we already had a frame ready to call the system call, just use it.
	 */
	ldi	EXC_BAD_ACCESS,arg0
	ldi	EXC_HP700_VM_PROT_READ,arg1
	ldw	SS_R30(t3),arg2

	.import	syscall_error
	.call
	bl	syscall_error,rp
	copy	t3,arg3

	/*
	 * just handle this like a normal return from a system call...
	 */
	b,n	$syscall_return

	.export restart_mach_syscall,entry
	.proc
	.callinfo
restart_mach_syscall
	ldil	L%panic,r1
	ldo	R%panic(r1),r1
	.call
	bv,n      0(r1)
	.procend

	.import $thread_return

/*
 * thread_exception_return()
 *
 * Return to user mode directly (Ha!) from within a system call.
 */
	.export	thread_exception_return,entry

	.proc
	.callinfo
thread_exception_return
	/*
	 * get the active thread's PCB pointer and a pointer to user state
	 * so we can restore all the callee save registers.
	 */
	ldil	L%active_pcbs,t2
	ldw	R%active_pcbs(t2),t2
	ldo	PCB_SS(t2),t3

	ldw	SS_FLAGS(t3),t1
	extru,=	t1,30,1,r0
	b	$cont
	depi	0,30,1,t1
	
	.import hp_pa_astcheck		
        .call			        
	bl	hp_pa_astcheck,rp       
	copy	t3,r3

        mtctl   ret0,eiem               	
	copy	r3,t3
		
	ldil	L%KERNEL_STACK_MASK,t1
	ldo	R%KERNEL_STACK_MASK(t1),t1
	andcm   sp,t1,sp

	ldil	L%$thread_return,t2
	ldo	R%$thread_return(t2),t2
	bv      0(t2)
	ldo	TF_SIZE(sp),sp
	
$cont
	stw	t1,SS_FLAGS(t3)
	
#if DEBUG
	ldi	-1,t1		/* SPL0 == 0xffffffff == -1 */
	mfctl	eiem,t2
	combt,=	t1,t2,$cont_finish
	nop

	ldi	PSW_I,t1
	mfctl	ipsw,t2
	and	t1,t2,t2

	combt,=	t1,t2,$cont_finish
	nop

	ldil	L%$badspl_str,arg0

	ldil	L%panic,t1
	ldo	R%panic(t1),t1
	.call 
	blr     r0,rp
	bv      0(t1)
	ldo	R%$badspl_str(arg0),arg0
	
$cont_finish
#endif /* DEBUG */

	ldw	SS_R3(t3),r3
	ldw	SS_R4(t3),r4
	ldw	SS_R5(t3),r5
	ldw	SS_R6(t3),r6
	ldw	SS_R7(t3),r7
	ldw	SS_R8(t3),r8
	ldw	SS_R9(t3),r9
	ldw	SS_R10(t3),r10
	ldw	SS_R11(t3),r11
	ldw	SS_R12(t3),r12
	ldw	SS_R13(t3),r13
	ldw	SS_R14(t3),r14
	ldw	SS_R15(t3),r15
	ldw	SS_R16(t3),r16
	ldw	SS_R17(t3),r17
	ldw	SS_R18(t3),r18

	ldw	SS_R28(t3),r28
	ldw	SS_R29(t3),r29

	/*
	 * Knock up the stack, and branch into syscall return code to
	 * let it do the work. T1 and T3 must be set properly.
	 */
	ldil	L%KERNEL_STACK_MASK,t2
	ldo	R%KERNEL_STACK_MASK(t2),t2
	andcm   sp,t2,sp
	b	$thread_syscall_return
	ldo	KF_SIZE(sp),sp
	
	/*
	 * Never returns.
	 */
	.procend

#if MACH_RT	
	.export $syscall_end
$syscall_end	
#endif
		
	.end
