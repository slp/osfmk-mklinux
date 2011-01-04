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

#include <hp_pa/asm.h>

	.import	$global$,data
	.import thread_switch,code

	.space	$PRIVATE$
	.subspa	$DATA$
	.align	4
$master_lock$
	.word	-1,-1,-1,-1
	.export _spin_tries,data
_spin_tries
	.word	1

	.space	$TEXT$
	.subspa	$CODE$

/* void _spin_lock(void *m) */
	.export _spin_lock,entry
	.proc
	.callinfo
_spin_lock

	/* Align the lock to a 16-byte boundary as required by the
	   ldcws instruction.  We have reserved 4 words for the lock so
	   we are sure that one of those words must be correctly
	   aligned.  */
	ldo	15(arg0),arg1
	depi	0,31,4,arg1

	/* Attempt to get the lock.  If ret0 is 0 then we didn't get
	   the lock and have to spin.  Or it may be that the lock has
	   never been initialised, in which case we will initialise it,
	   after synchronising with other threads that might also try
	   to initialise it.  */
	ldcws	0(arg1),ret0
	comiclr,= 0,ret0,r0
	bv,n	(rp)

	/* We're probably going to end up calling thread_switch, so
	   establish a stack frame.  Register usage will be:
	   r3	original sp (gcc calling standard)
	   arg0	address of flag word in lock
	   arg1	address of lock word in lock
	   arg2	address of lock word in master lock */
	stw	rp,FM_CRP(sp)
	copy	r3,r1
	copy	sp,r3
	stwm	r1,128(sp)
	stw	arg1,VA_ARG1(r3)

	/* If the flag word in the lock is also 0 then the lock has not
	   yet been initialised.  The flag word is the following word
	   if the original argument was already 16-byte aligned,
	   otherwise it is the word at the original argument.  */
	comclr,<> arg0,arg1,r0
	ldo	4(arg0),arg0
$testinit$
	ldw	0(arg0),ret0
	comib,<>,n 0,ret0,$spin$

	/* The lock has not been initialised.  Get the master lock and
	   then only initialise if someone else didn't do so in the
	   meantime.  */
	addil	l%$master_lock$-$global$,dp
	ldo	r%$master_lock$-$global$+15(r1),arg2
	depi	0,31,4,arg2
	ldcws	0(arg2),ret0
	comib,<>,n 0,ret0,$gotmaster$

	/* Didn't get the master.  Do a thread_switch and test again if we
	   need to initialise.  This should be a very rare case.  */
	stw	arg0,VA_ARG0(r3)
	copy	r0,arg0
	copy	r0,arg1
	bl	thread_switch,rp
	copy	r0,arg2
	/* thread_switch(MACH_PORT_NULL, SWITCH_OPTION_NONE,
			 MACH_MSG_TIMEOUT_NONE) */
	ldw	VA_ARG0(r3),arg0
	b	$testinit$
	ldw	VA_ARG1(r3),arg1

$gotmaster$
	/* Now if the lock is still not initialised we can initialise it
	   and consider it taken.  We can put any non-zero value in the
	   `initialised' flag.  Our return address could conceivably be
	   a useful trace.  We also drop it in the master lock to
	   release that.  */
	ldw	0(arg0),ret0
	comib,<> 0,ret0,$spin$
	stw	rp,0(arg2)

	stw	rp,0(arg0)
	ldw	FM_CRP(r3),rp
$return$
	bv	(rp)
	ldwm	-128(sp),r3

$spin$
	/* On a multiprocessor, pthread_init() will have set _spin_tries to
	   some value greater than one.  We spin that many times in the hope
	   that a thread on another processor holds the lock and will shortly
	   release it.  On a uniprocessor we spin just one more time.  */
	addil	l%_spin_tries-$global$,dp
	ldw	r%_spin_tries-$global$(r1),t1
$spinloop$
	ldcws	0(arg1),ret0
	comib,<>,n 0,ret0,$return$
	ldw	FM_CRP(r3),rp		/* Nullified if branch not taken. */

	addib,<>,n -1,t1,$spinloop$
	nop

	copy	r0,arg0
	copy	r0,arg1
	bl	thread_switch,rp
	copy	r0,arg2
	b	$spin$
	ldw	VA_ARG1(r3),arg1

	.procend


/* void _spin_unlock(void *m) */
	.export _spin_unlock,entry
	.proc
	.callinfo
_spin_unlock
	/* Align the lock to a 16-byte boundary.  */
	ldo	15(arg0),arg0
	depi	0,31,4,arg0

	/* Clear the lock by setting it non-zero. */
	bv	(rp)
	stw	rp,0(arg0)

	.procend

	.end
