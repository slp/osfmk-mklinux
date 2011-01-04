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
#include <mach_prof.h>
#include <assym.s>
#include <machine/asm.h>	
#include <machine/spl.h>
#include <machine/psw.h>
#include <mach/machine/vm_param.h>
#include <hp_pa/pmap.h>
	
#define KERNEL_STACK_MASK  (KERNEL_STACK_SIZE-1)

	.space	$PRIVATE$
	.subspa	$DATA$

	.import cpu_data
	.import active_pcbs
	.import	server_dp
	.import	need_ast
	
	.space	$TEXT$
	.subspa $CODE$

	.import hp_pa_astcheck
	.import	port_name_to_map,code
	.import	vm_map_deallocate,code

	.import exception_return_wrapper
	.import ast_taken		
		
/* 
 * boolean_t
 * klcopyinstr(mach_port_t target, vm_offset_t src, vm_offset_t dst, int maxcount, int *out)
 */
	.export	klcopyinstr,entry
	.proc
	.callinfo 

klcopyinstr
	stw	rp,-20(sp)
	or	r0,r3,r1
	or	r0,sp,r3
	stw	r1,0(sp)

	stw 	dp,8(r3)
	stw 	r4,12(r3)
	stw 	r5,16(r3)
	stw 	r6,20(r3)

	stw 	arg1,24(r3)
	stw 	arg2,28(r3)
	stw 	arg3,32(r3)

	ldil	L%server_dp,t1
	ldw	R%server_dp(t1),t2
	combf,=  r0,t2,$dp_set
	
	ldo	R%server_dp(t1),t2
	stw	dp,0(t2)
	
$dp_set
	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	rsm	PSW_I,r0
	ldil	L%cpu_data,r6
	ldw	R%cpu_data(r6),r6
	ldw	THREAD_TOP_ACT(r6),r5
	
	ldw	ACT_PCB(r5),t1
	ldw	PCB_KSP(t1),sp

	ldo	KF_SIZE(sp),sp
	stw	r0,PCB_KSP(t1)
	ssm	PSW_I,r0

#if MACH_PROF
	stw	rp, PCB_SS+SS_IIOQH(t1)
#endif
	.call 
	bl port_name_to_map,rp
	ldw	ACT_MAP(r5),r4

	ldw	32(r3),arg2
	ldw	28(r3),arg1
	ldw	24(r3),arg0

	stw	ret0,ACT_MAP(r5)

        comb,=  r0,arg2,$copyinstr_ok
	or	arg1,r0,t2

        comb,>  r0,arg2,$copyinstr_error

	ldil	L%$copyinstr_error,t1
	ldo	R%$copyinstr_error(t1),t1
	stw	t1,THREAD_RECOVER(r6)

	ldw	VMMAP_PMAP(ret0),t1

	ldw	PMAP_SPACE(t1),t4
	mtsp	t4,sr1
	ldw	PMAP_PID(t1),t4
	mtctl	t4,pidr1

        ldbs,ma	1(sr1,arg0),t1

$copyinstr_loop_byte
	combt,= r0,t1,$copyinstr_ok
        stbs,ma t1,1(arg1) 
        addib,>,n -1,arg2,$copyinstr_loop_byte
        ldbs,ma	1(sr1,arg0),t1
		
$copyinstr_error
	ldw	-52(r3),t1
	stw	r0,0(t1)

	.call 
	bl vm_map_deallocate,rp
	ldw	ACT_MAP(r5),arg0
	b	$out
	ldi	1,ret0

$copyinstr_ok
	ldw	-52(r3),t1
	sub     arg1,t2,t2	
	stw	t2,0(t1)

	.call 
	bl vm_map_deallocate,rp
	ldw	ACT_MAP(r5),arg0

	copy	r0,ret0

$out
	stw	r0,THREAD_RECOVER(r6)

	stw	r4,ACT_MAP(r5)

	bl	hp_pa_astcheck,rp      
	copy	ret0,r4
        mtctl   ret0,eiem              
	copy	r4,ret0 

	ldw	ACT_PCB(r5),t1		
	ldo	-KF_SIZE(sp),t2
	stw	t2, PCB_KSP(t1)		
	or	r0,r3,sp
	
	ssm	PSW_I,r0

	ldw	-20(sp),rp
	ldw	20(sp),r6
	ldw	16(sp),r5
	ldw	12(sp),r4
	ldw	 8(sp),dp

	bv	0(rp)
	ldw	0(sp),r3

	.procend

/* 
 * boolean_t
 * klcopyin(mach_port_t target, vm_offset_t src, vm_offset_t dst, int count)
 */
	.export	klcopyin,entry
	.proc
	.callinfo 

klcopyin
	stw	rp,-20(sp)
	or	r0,r3,r1
	or	r0,sp,r3
	stw	r1,0(sp)

	stw 	dp,8(r3)
	stw 	r4,12(r3)
	stw 	r5,16(r3)
	stw 	r6,20(r3)

	stw 	arg1,24(r3)
	stw 	arg2,28(r3)
	stw 	arg3,32(r3)

	mfctl	pidr1,t1
	stw	t1,36(r3)
	mfsp	sr1,t1
	stw	t1,40(r3)
		
	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	rsm	PSW_I,r0
	ldil	L%cpu_data,t1
	ldw	R%cpu_data(t1),t1
	ldw	THREAD_TOP_ACT(t1),r5
	
	ldw	ACT_PCB(r5),t1
	ldw	PCB_KSP(t1),sp

	ldo	KF_SIZE(sp),sp
	stw	r0,PCB_KSP(t1)
	ssm	PSW_I,r0

#if MACH_PROF
	stw	rp, PCB_SS+SS_IIOQH(t1)
#endif

	.call 
	bl port_name_to_map,rp
	ldw	ACT_MAP(r5),r4	

	combt,=	r0,ret0,$klcopyin_out
	ldi	1,r6
	
	ldw	32(r3),arg2
	ldw	28(r3),arg1
	ldw	24(r3),arg0

	.import	copyin,code
	.call 
	bl copyin,rp
	stw	ret0,ACT_MAP(r5)

	ldw	ACT_MAP(r5),arg0

	.call 
	bl vm_map_deallocate,rp
	or	ret0,r0,r6

$klcopyin_out
	or	r6,r0,ret0

	stw	r4,ACT_MAP(r5)

	bl	hp_pa_astcheck,rp      
	copy	ret0,r4
        mtctl   ret0,eiem              
	copy	r4,ret0 
		
	ldw	ACT_PCB(r5),t1		
	ldo	-KF_SIZE(sp),t2
	stw	t2, PCB_KSP(t1)		
	or	r0,r3,sp

	ldw	-20(sp),rp

	ldw	40(sp),t1
	mtsp	t1,sr1	
	ldw	36(sp),t1
	mtctl	t1,pidr1
			
	ssm	PSW_I,r0
		
	ldw	20(sp),r6
	ldw	16(sp),r5
	ldw	12(sp),r4
	ldw	 8(sp),dp

	bv	0(rp)
	ldw	0(sp),r3

	.procend


/* 
 * boolean_t
 * klcopyout(mach_port_t target, vm_offset_t src, vm_offset_t dst, int count)
 */
	.export	klcopyout,entry
	.proc
	.callinfo 

klcopyout
	stw	rp,-20(sp)
	or	r0,r3,r1
	or	r0,sp,r3
	stw	r1,0(sp)

	stw 	dp,8(r3)
	stw 	r4,12(r3)
	stw 	r5,16(r3)
	stw 	r6,20(r3)

	stw 	arg1,24(r3)
	stw 	arg2,28(r3)
	stw 	arg3,32(r3)

	mfctl	pidr1,t1
	stw	t1,36(r3)
	mfsp	sr1,t1
	stw	t1,40(r3)
		
	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	rsm	PSW_I,r0
	ldil	L%cpu_data,t1
	ldw	R%cpu_data(t1),t1
	ldw	THREAD_TOP_ACT(t1),r5

	ldw	ACT_PCB(r5),t1
	ldw	PCB_KSP(t1),sp

	ldo	KF_SIZE(sp),sp
	stw	r0,PCB_KSP(t1)
	ssm	PSW_I,r0

#if MACH_PROF
	stw	rp, PCB_SS+SS_IIOQH(t1)
#endif

	.call 
	bl port_name_to_map,rp
	ldw	ACT_MAP(r5),r4

	combt,=	r0,ret0,$klcopyout_out
	ldi	1,r6
		
	ldw	32(r3),arg2
	ldw	28(r3),arg1
	ldw	24(r3),arg0

	.import	copyout,code
	.call 
	bl copyout,rp
	stw	ret0,ACT_MAP(r5)

	ldw	ACT_MAP(r5),arg0

	.call 
	bl vm_map_deallocate,rp
	or	ret0,r0,r6

$klcopyout_out
	or	r6,r0,ret0

	stw	r4,ACT_MAP(r5)

	bl	hp_pa_astcheck,rp      
	copy	ret0,r4
        mtctl   ret0,eiem              
	copy	r4,ret0 
	
	ldw	ACT_PCB(r5),t1		
	ldo	-KF_SIZE(sp),t2
	stw	t2, PCB_KSP(t1)		
	or	r0,r3,sp

	ldw	-20(sp),rp

	ldw	40(sp),t1
	mtsp	t1,sr1	
	ldw	36(sp),t1
	mtctl	t1,pidr1
		
	ssm	PSW_I,r0
		
	ldw	20(sp),r6
	ldw	16(sp),r5
	ldw	12(sp),r4
	ldw	 8(sp),dp

	bv	0(rp)
	ldw	0(sp),r3

	.procend

/*
 * kern_return_t
 * klthread_switch(mach_port_t thread, int option, int option_time)
 */
	.export	klthread_switch,entry
	.proc
	.callinfo 

klthread_switch
	stw	rp,-20(sp)
	or	r0,r3,r1
	or	r0,sp,r3
	stw	r1,0(sp)

	stw 	dp,8(r3)
	stw 	r5,16(r3)
	stw 	r6,20(r3)
			
	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	rsm	PSW_I,r0
	ldil	L%cpu_data,t1
	ldw	R%cpu_data(t1),t1
	ldw	THREAD_TOP_ACT(t1),r5
	ldw	ACT_PCB(r5),r5
	
	ldw	PCB_KSP(r5),sp
	ldo	KF_SIZE(sp),sp
	stw	r0,PCB_KSP(r5)

#if MACH_PROF
	stw	rp, PCB_SS+SS_IIOQH(r5)
#endif

	.import	syscall_thread_switch,code
	.call 
	bl syscall_thread_switch,rp
	ssm	PSW_I,r0

	bl	hp_pa_astcheck,rp      
	copy	ret0,r6
        mtctl   ret0,eiem              
	copy	r6,ret0 
	
	ldo	-KF_SIZE(sp),t2
	stw	t2, PCB_KSP(r5)		
	or	r0,r3,sp

	ssm	PSW_I,r0
		
	ldw	-20(sp),rp
	ldw	20(sp),r6	
	ldw	16(sp),r5
	ldw	 8(sp),dp

	bv	0(rp)
	ldw	0(sp),r3

	.procend
	
/* 
 * kern_return_t
 * klthread_depress_abort(mach_port_t name)
 */
	.export	klthread_depress_abort,entry
	.proc
	.callinfo 

klthread_depress_abort
	stw	rp,-20(sp)
	or	r0,r3,r1
	or	r0,sp,r3
	stw	r1,0(sp)

	stw 	dp,8(r3)
	stw 	r5,16(r3)
	stw 	r6,20(r3)
			
	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	rsm	PSW_I,r0
	ldil	L%cpu_data,t1
	ldw	R%cpu_data(t1),t1
	ldw	THREAD_TOP_ACT(t1),r5
	ldw	ACT_PCB(r5),r5
	
	ldw	PCB_KSP(r5),sp
	ldo	KF_SIZE(sp),sp
	stw	r0,PCB_KSP(r5)

#if MACH_PROF
	stw	rp, PCB_SS+SS_IIOQH(r5)
#endif

	.import	syscall_thread_depress_abort,code
	.call 
	bl syscall_thread_depress_abort,rp
	ssm	PSW_I,r0

        .call			       
	bl	hp_pa_astcheck,rp      
	copy	ret0,r6
        mtctl   ret0,eiem              
	copy	r6,ret0 
		
	ldo	-KF_SIZE(sp),t2
	stw	t2, PCB_KSP(r5)		
	or	r0,r3,sp

	ssm	PSW_I,r0
		
	ldw	-20(sp),rp
	ldw	20(sp),r6	
	ldw	16(sp),r5
	ldw	 8(sp),dp

	bv	0(rp)
	ldw	0(sp),r3

	.procend

/*
 * void
 * call_exc_serv(
 *		 mach_port_t exc_port, exception_type_t exception,
 *		 exception_data_t code, mach_msg_type_number_t codeCnt,
 *		 int *new_sp, mig_impl_routine_t func,
 *		 int *flavor, thread_state_t statep, unsigned int *state_cnt)
 */
	.export call_exc_serv,entry
	.proc
	.callinfo 

call_exc_serv
	/* we don't return from here, so don't need to
	 * save r3...
	 */ 
	ldw	-52(sp),t2	/* get *new_sp */
	ldw	-60(sp),t1	/* get flavor */
	stw	t1,20(t2)	/* store flavor into new stack (arg 4) */

	ldw	-64(sp),t1	/* get statep */
	stw	t1,16(t2)	/* store statep into new stack (arg 5) */
	stw	t1,8(t2)	/* store statep into new stack (arg 7) */
	
	ldw	-68(sp),t1	/* get state_cnt */
	stw	t1,4(t2)	/* store state_cnt into new stack (arg8) */
	ldw	0(t1),t1	/* get *state_cnt */
	stw	t1,12(t2)	/* store *state_cnt into new stack (arg6) */

	ldw	-56(sp),r1	/* get func */

	rsm	PSW_I,r0

        ldil    L%active_pcbs,t3
        ldw     R%active_pcbs(t3),t3

	ldil	L%KERNEL_STACK_MASK,t1
	ldo	R%KERNEL_STACK_MASK(t1),t1
	andcm   sp,t1,sp

	stw	sp,PCB_KSP(t3)

	ldil	L%server_dp,t1
	ldw	R%server_dp(t1),dp
	ldo	72(t2),sp
	
	.call
	blr     r0,rp
	bv     	(r1)
	ssm	PSW_I,r0
		
	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	rsm	PSW_I,r0
        ldil    L%active_pcbs,t3
        ldw     R%active_pcbs(t3),t3

	ldw	PCB_KSP(t3),sp
	ldo	KF_SIZE(sp),sp
	stw	r0,PCB_KSP(t3)
	ssm	PSW_I,r0

	ldil	L%exception_return_wrapper,t1
	ldo	R%exception_return_wrapper(t1),t1
	.call
	bv      0(t1)
	or	r0,ret0,arg0

	.procend

	.end
