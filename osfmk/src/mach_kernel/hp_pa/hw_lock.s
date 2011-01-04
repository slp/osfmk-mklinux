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

#include <cpus.h>
#include <mach_rt.h>
#include <mach_ldebug.h>

#include <mach_assert.h>
#include <machine/asm.h>
#include <assym.s>
#include <kern/etap_options.h>
#include <hp_pa/spl.h>
#include <hp_pa/psw.h>

	.space	$PRIVATE$
	.subspa	$DATA$
	.import	cpu_data

#if MACH_LDEBUG
$bad_mutex_type
        .string "mutex: not a mutex.\x00"
$bad_thread
        .string "mutex: bad thread.\x00"
#endif

	.space	$TEXT$
	.subspa $CODE$

	.import panic

#if	MACH_RT
	.import kernel_preempt_check,code

/* We rely on DISABLE_PREEMPTION not calling any procedures because it must
   keep arg0 intact.  See _mutex_lock.  */
#define DISABLE_PREEMPTION(temp1, temp2)	;\
	ldil	L%cpu_data,temp1		;\
	ldo	R%cpu_data(temp1),temp1		;\
	ldw	CPU_PREEMPTION_LEV(temp1),temp2	;\
	addi	1,temp2,temp2			;\
	stw	temp2,CPU_PREEMPTION_LEV(temp1)

#define ENABLE_PREEMPTION(temp1, temp2, frame)	;\
	ldil	L%cpu_data,temp1		;\
	ldo	R%cpu_data(temp1),temp1		;\
	ldw	CPU_PREEMPTION_LEV(temp1),temp2	;\
	addi	-1,temp2,temp2			;\
	stw	temp2,CPU_PREEMPTION_LEV(temp1)	;\
	comiclr,<> 0,temp2,r0			;\
	.call					;\
	bl	kernel_preempt_check,rp		;\
	ldw	FM_CRP(frame),arg0		;\
/* The comiclr nullifies the call to kernel_preempt_check if the preemption
   level is still nonzero after the decrement.  Note that there must be
   exactly one nullified instruction!  Do not change the "bl" into more
   than one other instruction.  */

#else

#define DISABLE_PREEMPTION(temp1, temp2)
#define ENABLE_PREEMPTION(temp1, temp2, frame)

#endif	/* MACH_RT */

#if MACH_LDEBUG
#define MUTEX_TAG 0x4d4d

#define CHECK_MUTEX_TYPE(temp1, temp2, label)	;\
	ldil	L%MUTEX_TAG,temp1		;\
	ldo	R%MUTEX_TAG(temp1),temp1	;\
	ldw	M_TYPE(arg0),temp2		;\
	combt,=,n temp1,temp2,label		;\
	ldil	L%$bad_mutex_type,arg0		;\
	bl	panic,rp			;\
	ldo	R%$bad_mutex_type(arg0),arg0	;\
label

#define CHECK_THREAD(thread, temp1, label)	;\
	ldil	L%cpu_data,temp1		;\
	ldw	R%cpu_data(temp1),temp1		;\
	combt,=,n thread,temp1,label		;\
	ldil	L%$bad_thread,arg0		;\
	bl	panic,rp			;\
	ldo	R%$bad_thread(arg0),arg0	;\
label

#else
#define	CHECK_MUTEX_TYPE(temp1, temp2, label)
#define CHECK_THREAD(thread, temp1, label)
#endif

/*
 * align the lock to a 16 byte boundary
 */
#define LOCK_ALIGN(from, offset, to)		;\
	ldo	15+offset(from),to		;\
	depi	0,31,4,to

	.space	$TEXT$
	.subspa $CODE$

	.import mutex_lock_wait
	.import mutex_unlock_wakeup
#if ETAP_LOCK_TRACE
	.import etap_mutex_init
	.import etap_mutex_miss
	.import etap_mutex_hold
	.import etap_mutex_unlock
#endif	/* ETAP_LOCK_TRACE */

	.export hw_lock_held,entry
	.proc
	.callinfo

hw_lock_held
	LOCK_ALIGN(arg0,0,t3)

	ldw	0(t3),ret0

	/*
	 * if ret0 is 0 then the lock is held.
	 */
	comclr,= r0,ret0,ret0
	ldi	1,ret0

	bv,n	0(rp)

	.procend

	.export hw_lock_try,entry
	.proc
	.callinfo

/*
 *	unsigned int hw_lock_try(hw_lock_t)
 *	MACH_RT:  returns with preemption disabled on success.
 */
hw_lock_try
	stw	rp,FM_CRP(sp)
	copy	r3,r1
	copy	sp,r3
	stwm	r1,128(sp)

	LOCK_ALIGN(arg0,0,t3)

	DISABLE_PREEMPTION(t1,t2)

	/*
	 * attempt to get the lock
	 */
	ldcws	0(t3),ret0

	/*
	 * if ret0 is 0 then we didn't get the lock and we return 0,
	 * else we own the lock and we return ret0 (still nonzero),
	 * leaving preemption disabled.
	 */
	comb,<>,n r0,ret0,$retf

	ENABLE_PREEMPTION(t1,t2,r3)
	or	r0,r0,ret0

$retf
	ldw	FM_CRP(r3),rp
	bv	0(rp)
	ldwm	-128(sp),r3

	.procend


	.export hw_lock_init,entry
	.proc
	.callinfo

hw_lock_init
	LOCK_ALIGN(arg0,0,t3)

	/*
	 * clear the lock by setting it non-zero
	 */
	ldi	-1,t1
	bv	0(rp)
	stw	t1,0(t3)

	.procend

	.export hw_lock_unlock,entry
	.proc
	.callinfo

/*
 *	void hw_lock_unlock(hw_lock_t)
 *
 *	Unconditionally release lock.
 *	MACH_RT:  release preemption level.
 */
hw_lock_unlock
	stw	rp,FM_CRP(sp)
	copy	r3,r1
	copy	sp,r3
	stwm	r1,128(sp)

	LOCK_ALIGN(arg0,0,t3)

	/*
	 * clear the lock by setting it non-zero
	 */
	ldi	-1,t1
	stw	t1,0(t3)

	ENABLE_PREEMPTION(t1,t2,r3)

	ldw	FM_CRP(r3),rp
	bv	0(rp)
	ldwm	-128(sp),r3

	.procend

/*
 * void interlock_unlock(hw_lock_t);
 */
	.export interlock_unlock,entry
	.proc
	.callinfo
interlock_unlock
	stw	rp,FM_CRP(sp)
	copy	r3,r1
	copy	sp,r3
	stwm	r1,128(sp)

	LOCK_ALIGN(arg0,0,t3)

	ldi	-1,t1
	stw	t1,M_INTERLOCK(t3)

	ENABLE_PREEMPTION(t1,t2,r3)

	ldw	FM_CRP(r3),rp
	bv	0(rp)
	ldwm	-128(sp),r3

	.procend


/*
 * void mutex_init(mutex_t*, etap_event_t)
 */
	.export mutex_init,entry
	.proc
	.callinfo
mutex_init
	LOCK_ALIGN(arg0,0,t2)
	ldi	-1,t1

	stw	t1,M_INTERLOCK(t2)	/* clear interlock */
	stw	t1,M_LOCKED(t2)		/* clear locked */

#if	MACH_LDEBUG
	ldil	L%MUTEX_TAG,t1
	ldo	R%MUTEX_TAG(t1),t1
	stw	t1,M_TYPE(arg0)
	stw	r0,M_PC(arg0)
	stw	r0,M_THREAD(arg0)
#endif

#if ETAP_LOCK_TRACE
	/* Call etap_mutex_init, which has the same arguments.  It can
	   return directly to our caller, so we don't need to use a blr
	   instruction.  The delay slot for both #if branches is the
	   stw into M_WAITERS(arg0).  */
	b	etap_mutex_init
#else	/* !ETAP_LOCK_TRACE */
	bv	0(rp)
#endif	/* !ETAP_LOCK_TRACE */
	stw	r0,M_WAITERS(arg0)  	/* clear waiters */

	.procend

#if	MACH_RT || (NCPUS > 1) || MACH_LDEBUG || ETAP_LOCK_TRACE

/*
 * void _mutex_lock(mutex_t*)
 */
	.export	_mutex_lock,entry
	.proc
	.callinfo
_mutex_lock
	stw	rp,FM_CRP(sp)
	or	r0,r3,r1
	or	r0,sp,r3
	stwm	r1,128(sp)

	stw	arg0,-36(r3)
#if ETAP_LOCK_TRACE
#define ML_MISSED 16
#define ML_SECS 20
#define ML_NSECS 24
	stw	r0,ML_MISSED(r3)	/* local variable: boolean_t missed */
	stw	r0,ML_SECS(r3)		/* local variable: wait time, secs */
	stw	r0,ML_NSECS(r3)		/* local variable: wait time, nsecs */
#endif	/* ETAP_LOCK_TRACE */

	CHECK_MUTEX_TYPE(t1,t2,$mutex_lock_type_ok)

_mutex_lock_retry
	LOCK_ALIGN(arg0,0,t3)

	DISABLE_PREEMPTION(t1,t2)

$ilk_again
	ldcws	  M_INTERLOCK(t3),t1
	comb,=	  r0,t1,$ilk_again
	nop
	/* Don't do what I did and try to eliminate the nop with "comb,=,n".
	   For backward branches, ",n" means the instruction is nullified
	   if the branch is *not* taken.  */

	ldo	  M_LOCKED(t3),t1
	ldcws	  0(t1),t1
	comb,=,n  r0,t1,$mut_failed

	ldi	-1,t1
	stw	t1,M_INTERLOCK(t3)

#if MACH_LDEBUG
	stw	rp,M_PC(arg0)
	ldil	L%cpu_data,t1
	ldw	R%cpu_data(t1),t1
	stw	t1,M_THREAD(arg0)
#endif

	ENABLE_PREEMPTION(t1,t2,r3)

#if ETAP_LOCK_TRACE
	ldw	-36(r3),arg0		/* mutex */
	ldw	FM_CRP(r3),arg1		/* pc */
	ldw	ML_SECS(r3),arg3	/* wait time, secs */
	.call
	bl	etap_mutex_hold,rp
	ldw	ML_NSECS(r3),arg2	/* wait time, nsecs */
	/* In accordance with the PA procedure call convention, the 64-bit
	   tvalspec_t value is passed with its first word in an odd-numbered
	   argument register.  Even if this is backwards from everything else.
	 */
#endif	/* ETAP_LOCK_TRACE */

	ldw	FM_CRP(r3),rp
	bv	0(rp)
	ldwm	-128(sp),r3

$mut_failed

#if ETAP_LOCK_TRACE
	ldw	ML_MISSED(r3),t1
	comb,<>,n r0,t1,$already_missed	/* already took a wait timestamp */
	.call
	bl	etap_mutex_miss,rp
	nop
	ldi	1,t1
	stw	t1,ML_MISSED(r3)	/* flag already_missed */
	stw	ret0,ML_SECS(r3)	/* wait start time, secs */
	stw	ret1,ML_NSECS(r3)	/* wait start time, nsecs */
	ldw	-36(r3),arg0		/* get back arg0 */
$already_missed
#endif	/* ETAP_LOCK_TRACE */

	/* mutex_lock_wait() will do ENABLE_PREEMPTION() before blocking. */
	.call
	bl	mutex_lock_wait,rp	/* arg0 is still mutex */
	ldw	-36(r3),arg0

	b	_mutex_lock_retry
	ldw	-36(r3),arg0

	.procend

/*
 * boolean_t _mutex_try(mutex_t*)
 */
	.export	_mutex_try,entry
	.proc
	.callinfo
_mutex_try
	CHECK_MUTEX_TYPE(t1,t2,$mutex_try_type_ok)

	LOCK_ALIGN(arg0,M_LOCKED,t2)

	ldcws	0(t2),ret0

#if ETAP_LOCK_TRACE
	comb,=,n r0,ret0,$try_failed
	stw	rp,FM_CRP(sp)
	copy	r3,r1
	copy	sp,r3
	stwm	r1,128(sp)

	/* Call etap_mutex_hold.  arg0 is still the mutex address.  */
	copy	rp,arg1
	ldi	0,arg2			/* wait start time, nsecs */
	bl	etap_mutex_hold,rp
	ldi	0,arg3			/* wait start time, secs */
	ldi	1,ret0			/* try succeeded */
	ldw	FM_CRP(r3),rp
	ldwm	-128(sp),r3
$try_failed

#else	/* !ETAP_LOCK_TRACE */

	/*
	 * if ret0 is 0 then the lock is held by someone else, so return 0.
	 * If ret0 is not zero (-1) then we just got the lock.  These
	 * values already correspond to the expected return from _mutex_try.
	 */

#endif	/* !ETAP_LOCK_TRACE */

	bv,n	0(rp)

	.procend

	/*
	 * void _mutex_unlock(mutex_t*)
	 */
	.export	mutex_unlock,entry
	.proc
	.callinfo
mutex_unlock
	stw	rp,FM_CRP(sp)
	copy	r3,r1
	copy	sp,r3
	stwm	r1,128(sp)

#if ETAP_LOCK_TRACE || MACH_LDEBUG	/* Will need arg0 after func call. */
	stw	arg0,-36(r3)
#endif

#if ETAP_LOCK_TRACE
	.call
	bl	etap_mutex_unlock,rp
	nop

	ldw	-36(r3),arg0
#endif	/* ETAP_LOCK_TRACE */

	CHECK_MUTEX_TYPE(t1,t2,$mutex_unlock_type_ok)

	LOCK_ALIGN(arg0,0,t3)

	DISABLE_PREEMPTION(t1,t2)

$iluk_again
	ldcws	  M_INTERLOCK(t3),t1	/* take the interlock lock */
	comb,=	  r0,t1,$iluk_again
	nop

	ldw	  M_WAITERS(arg0),t1	/* are there waiters? */
	comb,=,n r0,t1,$mu_no_waiters	/* skip if no */

	bl	  mutex_unlock_wakeup,rp
	stw	  t3,-40(r3)

	ldw	  -40(r3),t3

$mu_no_waiters
	ldi	-1,t1
	stw	t1,M_LOCKED(t3)		/* unlock locks */

	stw	t1,M_INTERLOCK(t3)

#if	MACH_LDEBUG
	ldw	-36(r3),arg0
	stw	r0,M_PC(arg0)
	stw	r0,M_THREAD(arg0)
#endif

	ENABLE_PREEMPTION(t1,t2,r3)

	ldw	FM_CRP(r3),rp
	bv	0(rp)
	ldwm	-128(sp),r3

	.procend

#endif	/* MACH_RT || (NCPUS > 1) || MACH_LDEBUG || ETAP_LOCK_TRACE */

	.end
