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

#include <cpus.h>
#include <mach_assert.h>
#include <mach_ldebug.h>
#include <mach_rt.h>

#include <kern/etap_options.h>
	
#include <ppc/asm.h>
#include <assym.s>

#if	NCPUS > 1
#include <ppc/POWERMAC/mp/mp.h>
#endif	/* NCPUS > 1 */

#define PROLOG(space)							\
	stwu	r1,	-((space)+16)(r1);				\
	mflr	r0		;					\
	stw	r0,	((space)+20)(r1);				\
	stw	r3,	12(r1)
#define EPILOG								\
	lwz	r1,	0(r1)	;					\
	lwz	r0,	4(r1)	;					\
	mtlr	r0		;					
	
#if	MACH_KDB || MACH_KGDB
	
#define OPTIONAL_PROLOG		PROLOG(4)
#define OPTIONAL_EPILOG		EPILOG
	
#else	/* MACH_KDB || MACH_KGDB */
	
#define OPTIONAL_PROLOG
#define OPTIONAL_EPILOG
	
#endif	/* MACH_KDB || MACH_KGDB */

#if	MACH_RT
#error DISABLE_PREEMPTION not implemented
#error ENABLE_PREEMPTION not implemented
#else	/* MACH_RT */
#define DISABLE_PREEMPTION()
#define ENABLE_PREEMPTION()
#endif	/* MACH_RT */
	
#if	MACH_LDEBUG
/*
 * Routines for general lock debugging.
 */

/*
 * Checks for expected lock types and calls "panic" on
 * mismatch.  Detects calls to Mutex functions with
 * type simplelock and vice versa.
 */
#define	CHECK_MUTEX_TYPE()						\
	lwz	r10,	MUTEX_TYPE(r3);					\
	cmpwi	r10,	MUTEX_TAG;					\
	beq+	1f;							\
	lis	r3,	not_a_mutex@h;					\
	ori	r3,	r3,	not_a_mutex@l;				\
	bl	EXT(panic)	;					\
	lwz	r3,	12(r1)	;					\
1:
	
	.data
not_a_mutex:
	.string	"not a mutex!\n"
	.text

#define CHECK_SIMPLE_LOCK_TYPE()					\
	lwz	r10,	SLOCK_TYPE(r3);					\
	cmpwi	r10,	USLOCK_TAG;					\
	beq+	1f;							\
	lis	r3,	not_a_slock@h;					\
	ori	r3,	r3,	not_a_slock@l;				\
	bl	EXT(panic)	;					\
	lwz	r3,	12(r1)	;					\
1:
	
	.data
not_a_slock:
	.string	"not a simple lock!\n"
	.text

/*
 * If one or more simplelocks are currently held by a thread,
 * an attempt to acquire a mutex will cause this check to fail
 * (since a mutex lock may context switch, holding a simplelock
 * is not a good thing).
 */
#if	MACH_RT
#error CHECK_PREEMPTION_LEVEL not implemented
#else	/* MACH_RT */
#define CHECK_PREEMPTION_LEVEL()
#endif	/* MACH_RT */
	
#define CHECK_NO_SIMPLELOCKS()						\
	lwz	r10,	PP_CPU_DATA(r2);				\
	lwz	r10,	CPU_SIMPLE_LOCK_COUNT(r10);			\
	cmpwi	r10,	0;						\
	beq+	1f;							\
	lis	r3,	simple_locks_held@h;				\
	ori	r3,	r3,	simple_locks_held@l;			\
	bl	EXT(panic)	;					\
	lwz	r3,	12(r1)	;					\
1:									\
	CHECK_PREEMPTION_LEVEL()
	
	.data
simple_locks_held:
	.string	"simple locks held!\n";
	.text

/* 
 * Verifies return to the correct thread in "unlock" situations.
 */
#define CHECK_THREAD(thread_offset)					\
	lwz	r10,	PP_CPU_DATA(r2);				\
	lwz	r10,	CPU_ACTIVE_THREAD(r10);				\
	cmpwi	r10,	0	;					\
	beq-	1f;							\
	lwz	r9,	thread_offset(r3);				\
	cmpw	r9,	r10	;					\
	beq+	1f;							\
	lis	r3,	wrong_thread@h;				\
	ori	r3,	r3,	wrong_thread@l;				\
	bl	EXT(panic)	;					\
	lwz	r3,	12(r1)	;					\
1:
	
	.data
wrong_thread:
	.string	"wrong thread!\n"
	.text

#define CHECK_MYLOCK(thread_offset)					\
	lwz	r10,	PP_CPU_DATA(r2);				\
	lwz	r10,	CPU_ACTIVE_THREAD(r10);				\
	cmpwi	r10,	0	;					\
	beq-	1f;							\
	lwz	r9,	thread_offset(r3);				\
	cmpw	r9,	r10	;					\
	bne+	1f;							\
	lis	r3,	mylock_attempt@h;				\
	ori	r3,	r3,	mylock_attempt@l;			\
	bl	EXT(panic)	;					\
	lwz	r3,	12(r1)	;					\
1:	
	
	.data
mylock_attempt:
	.string	"mylock attempt!\n"
	.text

#else	/* MACH_LDEBUG */

#define CHECK_MUTEX_TYPE()
#define CHECK_SIMPLE_LOCK_TYPE()
#define CHECK_THREAD(thread_offset)
#define CHECK_NO_SIMPLELOCKS()
#define CHECK_MYLOCK(thread_offset)

#endif	/* MACH_LDEBUG */
	
/*
 *      void hw_lock_init(hw_lock_t)
 *
 *      Initialize a hardware lock.
 * TODO NMGS Manual talks about possible races if write to same cache line?
 * TODO NMGS should these locks take up a whole cache line? See HP code
 */

ENTRY(hw_lock_init, TAG_NO_FRAME_USED)

#if 0
	lwz		r5,0(r1)				/* Get pointer to the previous stack frame */
	li		r0,0					/* set lock to free == 0 */
	lwz		r6,4(r5)				/* Get our caller's caller */
	lwz		r5,0(r5)				/* Get previous stack frame */
	mflr	r4						/* Get our caller */
	mr.		r5,r5					/* Have we unstacked all the way? */
	stw		r0,	0(r3)				/* Initialize the lock */
	stw		r6,8(r3)				/* Stamp the initiator's caller */		
	stw		r4,4(r3)				/* Stamp the initiator (I won't use the ARGn, I won't I won't I won't!) */
	beqlr							/* Leave if the whole stack undone... */
	lwz		r6,0(r5)				/* Unstack */
	lwz		r4,4(r5)				/* Get return */
	mr.		r6,r6					/* Any more stack left? */
	stw		r4,12(r3)				/* Save caller's caller's caller */
	beqlr							/* Leave if the whole stack undone... */
	lwz		r5,0(r6)				/* Unstack */
	lwz		r4,4(r6)				/* Get return */
	mr.		r5,r5					/* Any more stack left? */
	stw		r4,16(r3)				/* Save caller's caller's caller */
	beqlr							/* Leave if the whole stack undone... */
	lwz		r6,0(r5)				/* Unstack */
	lwz		r4,4(r5)				/* Get return */
	mr.		r6,r6					/* Any more stack left? */
	stw		r4,20(r3)				/* Save caller's caller's caller */
	beqlr							/* Leave if the whole stack undone... */
	lwz		r5,0(r6)				/* Unstack */
	lwz		r4,4(r6)				/* Get return */
	mr.		r5,r5					/* Any more stack left? */
	stw		r4,24(r3)				/* Save caller's caller's caller */
	beqlr							/* Leave if the whole stack undone... */
	lwz		r4,4(r5)				/* Get return */
	stw		r4,28(r3)				/* Save caller's caller's caller */
#else
	li	r0,	0						/* set lock to free == 0 */
	stw	r0,	0(r3)					/* Initialize the lock */
#endif
	blr
	
/*
 *      void hw_lock_lock(hw_lock_t)
 *
 *      Acquire lock, spinning until it becomes available.
 *      MACH_RT:  also return with preemption disabled. TODO NMGS
 *	NMGS - apparently not used. lock usually spins in C via try()
 */

ENTRY(hw_lock_lock, TAG_NO_FRAME_USED)

	PROLOG(0)
	li	ARG1,	1		/* value to be stored... 1==taken*/
	li	r0,	0	
.L_lock_lock_retry:
	DISABLE_PREEMPTION()
	lwarx	ARG2,	0,ARG0		/* Ld from addr of ARG0 and reserve */
	cmpwi	ARG2,	0		/* TEST... */
	bne-	.L_lock_lock_wait	/* loop if BUSY (predict not busy) */
	
	sync
	stwcx.  ARG1,	0,ARG0		/* And SET (if reserved) */
	bne-	.L_lock_lock_retry	/* If set failed, loop back */
	isync
	EPILOG
	blr
.L_lock_lock_wait:
#if	MACH_LDEBUG
	lis	ARG2,	0x10		/* roughly 1E6 */
	mtctr	ARG2
#endif	/* MACH_LDEBUG */
	ENABLE_PREEMPTION()
.L_lock_lock_loop:
#if	MACH_LDEBUG
	bdnz+	0f
	BREAKPOINT_TRAP
0:	
#endif	/* MACH_LDEBUG */
	lwz	ARG2,	0(ARG0)
	cmpwi	ARG2,	0
	bne+	.L_lock_lock_loop
	b	.L_lock_lock_retry

/*
 *      void hw_lock_unlock(hw_lock_t)
 *
 *      Unconditionally release lock.
 *      MACH_RT:  release preemption level. TODO NMGS
 */

	.text

ENTRY(hw_lock_unlock, TAG_NO_FRAME_USED)

	PROLOG(0)
	li	r0,	0		/* set lock to free == 0 */
	cmpwi	r0,	0		/* Ensure r0 is zero on 601 for sync */
	bne+	0f
	sync				/* Flush writes done under lock */
0:
	stw	r0,	0(ARG0)

	ENABLE_PREEMPTION()
	EPILOG
	blr

/*
 *      unsigned int hw_lock_to(hw_lock_t, unsigned int timeout)
 *
 *      Try to acquire spin-lock. Return success (1) or failure (0).
 *      Attempt will fail after timeout ticks of the timebase.
 *		We try fairly hard to get this lock.  We disable for interruptions, but
 *		reenable after a "short" timeout (1000 ticks, we may want to shorten this).
 *		After checking to see if the large timeout value (passed in) has expired and a
 *		sufficient number of cycles have gone by (to insure pending 'rupts are taken),
 *		we return either in abject failure, or disable and go back to the lock sniff routine.
 *		If the sniffer finds the lock free, it jumps right up and tries to grab it.
 *
 */
ENTRY(hw_lock_to,TAG_NO_FRAME_USED)

			mfmsr	r0							/* Get the MSR value */
			mr		r5,r3						/* Get the address of the lock */
			rlwinm	r7,r0,0,17,15				/* Get MSR that is uninterruptible */
	
			mtmsr	r7							/* Turn off interruptions */
			mftb	r8							/* Get the low part of the time base */
			
lcktry:		lwarx	r9,0,r5						/* Grab the lock value */
			li		r3,1						/* Use part of the delay time */
			mr.		r9,r9						/* Is it locked? */
			bne-	lcksniff					/* Yeah, wait for it to clear... */
			sync
			stwcx.	r3,0,r5						/* Try to sieze that there durn lock */
			beq+	lckgot						/* We got it, yahoo... */
			b		lcktry						/* Just start up again if the store failed... */
			
lcksniff:	lwz		r3,0(r5)					/* Get that lock in here */
			mr.		r3,r3						/* Is it free yet? */
			beq+	lcktry						/* Yeah, try for it again... */
			
			mftb	r10							/* Time stamp us now */
			sub		r10,r10,r8					/* Get the elapsed time */
			cmplwi	r10,1000					/* Have we been spinning for 1000 tb ticks? */
			blt+	lcksniff					/* Not yet... */
			
			mtmsr	r0							/* Say, any interrupts pending? */			

/*			The following instructions force the pipeline to be interlocked to that only one
			instruction is issued per cycle.  The insures that we stay enabled for a long enough
			time; if it's too short, pending interruptions will not have a chance to be taken */
			
			subi	r4,r4,1000					/* Back off elapsed time from timeout value */
			or		r4,r4,r4					/* Do nothing here but force a single cycle delay */
			mr.		r4,r4						/* See if we used the whole timeout	*/
			li		r3,0						/* Assume a timeout return code */
			or		r4,r4,r4					/* Do nothing here but force a single cycle delay */
			
			ble-	lckfail						/* We failed */
			mtmsr	r7							/* Disable for interruptions */
			mftb	r8							/* Get the low part of the time base */
			b		lcksniff					/* Now that we've opened an enable window, keep trying... */

lckgot:		mtmsr	r0							/* Enable for interruptions */
			isync								/* Make sure we don't use a speculativily loaded value */
			blr									/* Return with honors... (R3 should have a1 from above) */

lckfail:										/* We couldn't get the lock */			
			li		r3,0						/* Set failure return code */
			blr									/* Return, head hanging low... */

/*
 *      unsigned int hw_lock_try(hw_lock_t)
 *
 *      try to acquire spin-lock. Return success (1) or failure (0)
 *      MACH_RT:  returns with preemption disabled on success. TODO NMGS
 *
 */
ENTRY(hw_lock_try,TAG_NO_FRAME_USED)

	PROLOG(0)
	li	ARG1,	1		/* value to be stored... 1==taken */
#if	MACH_LDEBUG
	lis	ARG2,	0x10		/* roughly 1E6 */
	mtctr	ARG2
#endif	/* MACH_LDEBUG */
	DISABLE_PREEMPTION()
.L_lock_try_loop:	
#if	MACH_LDEBUG
	bdnz+	0f
	BREAKPOINT_TRAP
0:	
#endif	/* MACH_LDEBUG */
	lwarx	r0,	0,ARG0		/* Ld from addr of arg and reserve */

	cmpwi	r0,	0		/* TEST... */
	bne-	.L_lock_try_failed	/* branch if taken. Predict free */
	
	sync
	stwcx.  ARG1,	0,ARG0
		/* And SET (if reserved) */
	bne-	.L_lock_try_loop	/* If set failed, loop back */
	isync

	li	ARG0,	1		/* SUCCESS - lock was free */
	EPILOG
	blr

.L_lock_try_failed:
	ENABLE_PREEMPTION()
	li	ARG0,	0		/* FAILURE - lock was taken */
	EPILOG
	blr

/*
 *      unsigned int hw_lock_held(hw_lock_t)
 *
 *      Return 1 if lock is held
 *      MACH_RT:  doesn't change preemption state.
 *      N.B.  Racy, of course.
 *
 */
ENTRY(hw_lock_held,TAG_NO_FRAME_USED)

	OPTIONAL_PROLOG
	isync					/* Make sure we don't use a speculativily fetched lock */
	lwz	ARG0, 0(ARG0)		/* Return value of lock */
	OPTIONAL_EPILOG
	blr

/*
 *	void mutex_init(mutex_t* l, etap_event_t etap)
 */

ENTRY(mutex_init,TAG_NO_FRAME_USED)

	PROLOG(0)
	li	r10,	0
	stw	r10,	MUTEX_ILK(r3)		/* clear interlock */
	stw	r10,	MUTEX_LOCKED(r3)	/* clear locked flag */
	sth	r10,	MUTEX_WAITERS(r3)	/* init waiter count */

#if	MACH_LDEBUG
	stw	r10,	MUTEX_PC(r3)		/* init caller pc */
	stw	r10,	MUTEX_THREAD(r3)	/* and owning thread */
	li	r10,	MUTEX_TAG
	stw	r10,	MUTEX_TYPE(r3)		/* set lock type */
#endif	/* MACH_LDEBUG */
#if	ETAP_LOCK_TRACE
	bl	EXT(etap_mutex_init)		/* init ETAP data */
#if 0
	lwz	r3,	12(r1)		/* restore r3 (save in prolog) */
#endif
#endif	/* ETAP_LOCK_TRACE */

	EPILOG
	blr

/*
 *	void _mutex_lock(mutex_t*)
 */

ENTRY(_mutex_lock,TAG_NO_FRAME_USED)

	PROLOG(12)
	
#if	ETAP_LOCK_TRACE
#define	SWT_HI	16(r1)
#define SWT_LO	20(r1)
#define MISSED	24(r1)
	li	r0,	0
	stw	r0,	SWT_HI		/* set wait time to 0 (HI) */
	stw	r0,	SWT_LO		/* set wait time to 0 (LO) */
	stw	r0,	MISSED		/* clear local miss marker */
#endif	/* ETAP_LOCK_TRACE */

	CHECK_MUTEX_TYPE()
	CHECK_NO_SIMPLELOCKS()

.L_ml_retry:
	DISABLE_PREEMPTION()
	li	r10,	1		/* value to be stored... 1==taken */
	li	r9,	MUTEX_ILK	/* in the interlock field */
#if	MACH_LDEBUG
	lis	ARG2,	0x10		/* roughly 1E6 */
	mtctr	ARG2
#endif	/* MACH_LDEBUG */
.L_ml_get_hw:
#if	MACH_LDEBUG
	bdnz+	0f
	BREAKPOINT_TRAP
0:	
#endif	/* MACH_LDEBUG */
	lwarx	r0,	r9,	r3	/* ld from addr of ARG0 and reserve */
	cmpwi	r0,	0		/* test ... */
	bne-	.L_ml_get_hw		/* loop if busy (predict not busy) */
	sync
	stwcx.	r10,	r9,	r3	/* and set (if reserved) */
	bne-	.L_ml_get_hw		/* if set failed, loop back */
	isync

/*
 * Beware of a race between this code path and the inline ASM fast-path locking
 * sequence which attempts to lock a mutex by directly setting the locked flag
 */
.L_ml_lock_retry:	
#if	MACH_LDEBUG
	lis	ARG2,	0x10		/* roughly 1E6 */
	mtctr	ARG2
#endif	/* MACH_LDEBUG */
.L_ml_lock_retry_loop:
#if	MACH_LDEBUG
	bdnz+	0f
	BREAKPOINT_TRAP
0:	
#endif	/* MACH_LDEBUG */
	li	r9,	MUTEX_LOCKED	/* the locked field */
	lwarx	r0,	r9,	r3	/* ld from addr of ARG0 and reserve */
	cmpwi	r0,	0		/* is the mutex locked ? */
	bne-	.L_ml_fail		/* yes, we loose */
	sync
	stwcx.	r10,	r9,	r3	/* lock it (if reserved) */
	bne-	.L_ml_lock_retry_loop	/* if set failed, try again */
	isync

#if	MACH_LDEBUG
	mflr	r10
	stw	r10,	MUTEX_PC(r3)
	lwz	r10,	PP_CPU_DATA(r2)
	lwz	r10,	CPU_ACTIVE_THREAD(r10)
	stw	r10,	MUTEX_THREAD(r3)
	cmpwi	r10,	0
	beq-	.L_ml_no_active_thread
	lwz	r9,	THREAD_MUTEX_COUNT(r10)
	addi	r9,	r9,	1
	stw	r9,	THREAD_MUTEX_COUNT(r10)
.L_ml_no_active_thread:
#endif	/* MACH_LDEBUG */

	sync
	li	r10,	0
	stw	r10,	MUTEX_ILK(r3)	/* free the interlock */

	ENABLE_PREEMPTION()

#if	ETAP_LOCK_TRACE
	mflr	r4
	lwz	r5,	SWT_HI
	lwz	r6,	SWT_LO
	bl	EXT(etap_mutex_hold)	/* collect hold timestamp */
#if 0
	lwz	r3,	12(r1)		/* restore r3 (saved in prolog) */
#endif
#endif	/* ETAP_LOCK_TRACE */

	EPILOG
	blr

.L_ml_fail:
#if	ETAP_LOCK_TRACE
	lwz	r7,	MISSED
	cmpwi	r7,	0	/* did we already take a wait timestamp ? */
	bne	.L_ml_block	/* yup. carry-on */
	bl	EXT(etap_mutex_miss)	/* get wait timestamp */
	stw	r3,	SWT_HI	/* store timestamp */
	stw	r4,	SWT_LO
	li	r7,	1	/* mark wait timestamp as taken */
	stw	r7,	MISSED
	lwz	r3,	12(r1)	/* restore r3 (saved in prolog) */
#endif	/* ETAP_LOCK_TRACE */

.L_ml_block:
	CHECK_MYLOCK(MUTEX_THREAD)
	bl	EXT(mutex_lock_wait)	/* wait for the lock */
	lwz	r3,	12(r1)		/* restore r3 (saved in prolog) */
	
	b	.L_ml_retry		/* and try again */

	
/*
 *	void _mutex_try(mutex_t*)
 *
 * Do nothing for now
 */

ENTRY(_mutex_try,TAG_NO_FRAME_USED)

	PROLOG(8)	/* reserve space for SWT_HI and SWT_LO */
	
#if	ETAP_LOCK_TRACE
	li	r5,	0
	stw	r5,	STW_HI	/* set wait time to 0 (HI) */
	stw	r5,	SWT_LO	/* set wait time to 0 (LO) */
#endif	/* ETAP_LOCK_TRACE */

	CHECK_MUTEX_TYPE()
	CHECK_NO_SIMPLELOCKS()

	li	r10,	1
	li	r9,	MUTEX_LOCKED	/* the locked field */
#if	MACH_LDEBUG
	lis	ARG2,	0x10		/* roughly 1E6 */
	mtctr	ARG2
#endif	/* MACH_LDEBUG */
.L_mt_loop:
#if	MACH_LDEBUG
	bdnz+	0f
	BREAKPOINT_TRAP
0:	
#endif	/* MACH_LDEBUG */
	lwarx	r0,	r9,	r3	/* ld from addr of ARG0 and reserve */
	cmpwi	r0,	0		/* is the mutex locked ? */
	bne-	.L_mt_fail		/* yes, we loose */
	sync
	stwcx.	r10,	r9,	r3	/* lock it (if reserved) */
	bne-	.L_mt_loop		/* if reservation failed, try again - no false negatives */
	isync

#if	MACH_LDEBUG
	mflr	r10
	stw	r10,	MUTEX_PC(r3)
	lwz	r10,	PP_CPU_DATA(r2)
	lwz	r10,	CPU_ACTIVE_THREAD(r10)
	stw	r10,	MUTEX_THREAD(r3)
	cmpwi	r10,	0
	beq-	.L_mt_no_active_thread
	lwz	r9,	THREAD_MUTEX_COUNT(r10)
	addi	r9,	r9,	1
	stw	r9,	THREAD_MUTEX_COUNT(r10)
.L_mt_no_active_thread:
#endif	/* MACH_LDEBUG */

#if	ETAP_LOCK_TRACE
	mflr	r4
	lwz	r5,	SWT_HI
	lwz	r6,	SWT_LO
	bl	EXT(etap_mutex_hold)	/* get start hold timestamp */
	lwz	r3,	12(r1)		/* restore r3 (saved in prolog) */
#endif	/* ETAP_LOCK_TRACE */

	li	r3,	1		/* success */
.L_mt_done:	
	EPILOG
	blr

.L_mt_fail:	
	li	r3,	0		/* failure */
	b	.L_mt_done
		
/*
 *	void mutex_unlock(mutex_t* l)
 */

ENTRY(mutex_unlock, TAG_NO_FRAME_USED)

	PROLOG(0)
	
#if	ETAP_LOCK_TRACE
	bl	EXT(etap_mutex_unlock)	/* collect ETAP data */
	lwz	r3,	12(r1)		/* restore r3 (saved in prolog) */
#endif	/* ETAP_LOCK_TRACE */

	CHECK_MUTEX_TYPE()
	CHECK_THREAD(MUTEX_THREAD)

	DISABLE_PREEMPTION()

	li	r10,	1		/* value to be stored... 1==taken */
	li	r9,	MUTEX_ILK	/* in the interlock field */
#if	MACH_LDEBUG
	lis	ARG2,	0x10		/* roughly 1E6 */
	mtctr	ARG2
#endif	/* MACH_LDEBUG */
.L_mu_get_hw:
#if	MACH_LDEBUG
	bdnz+	0f
	BREAKPOINT_TRAP
0:	
#endif	/* MACH_LDEBUG */
	lwarx	r0,	r9,	r3	/* ld from addr of ARG0 and reserve */
	cmpwi	r0,	0		/* test ... */
	bne-	.L_mu_get_hw		/* loop if busy (predict not busy) */
	sync
	stwcx.	r10,	r9,	r3	/* and set (if reserved) */
	bne-	.L_mu_get_hw		/* if set failed, loop back */
	isync

	lhz	r10,	MUTEX_WAITERS(r3)	/* are there any waiters ? */
	cmpwi	r10,	0
	bne-	.L_mu_wakeup			/* yes, more work to do */

.L_mu_doit:
#if	MACH_LDEBUG
	li	r10,	0
	stw	r10,	MUTEX_THREAD(r3)	/* disown thread */
	lwz	r10,	PP_CPU_DATA(r2)
	lwz	r10,	CPU_ACTIVE_THREAD(r10)
	cmpwi	r10,	0
	beq-	.L_mu_no_active_thread
	lwz	r9,	THREAD_MUTEX_COUNT(r10)
	subi	r9,	r9,	1
	stw	r9,	THREAD_MUTEX_COUNT(r10)
.L_mu_no_active_thread:
#endif	/* MACH_LDEBUG */

	li	r0,	0
	sync
	stw	r0,	MUTEX_LOCKED(r3)	/* unlock the mutex */

	sync
	stw	r0,	MUTEX_ILK(r3)		/* unlock the interlock */

	ENABLE_PREEMPTION()

	EPILOG
	blr

.L_mu_wakeup:
	bl	EXT(mutex_unlock_wakeup)	/* yes, wake a thread */
	lwz	r3,	12(r1)		/* restore r3 (saved in prolog) */
	b	.L_mu_doit

/*
 *	void interlock_unlock(hw_lock_t lock)
 */
ENTRY(interlock_unlock, TAG_NO_FRAME_USED)

	li		r0,	0
	cmpwi	r0, 0		/* Ensure R0 is zero on 601 for sync */
	bne+	0f
	sync
0:
	stw	r0,	0(r3)

	ENABLE_PREEMPTION()

	blr
