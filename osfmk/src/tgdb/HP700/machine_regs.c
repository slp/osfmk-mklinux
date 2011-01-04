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
 * Revised Copyright Notice:
 *
 * Copyright 1993, 1994 CONVEX Computer Corporation.  All Rights Reserved.
 *
 * This software is licensed on a royalty-free basis for use or modification,
 * so long as each copy of the software displays the foregoing copyright notice.
 *
 * This software is provided "AS IS" without warranty of any kind.  Convex 
 * specifically disclaims the implied warranties of merchantability and
 * fitness for a particular purpose.
 */

#include <mach.h>
#include <mach/message.h>
#include <mach/mach_types.h>
#include <mach/port.h>
#include <mach/mach_types.h>
#include <mach/mach_traps.h>
#include <hp_pa/psw.h>
#include <gdb_packet.h>
#include <tgdb.h>

void
tgdb_get_registers(mach_port_t thread, 
		   struct hp700_thread_state *ss,
		   struct hp700_float_state *fs)
{
	kern_return_t kr;
	mach_msg_type_number_t count;

	kr = thread_abort(thread);

	if (kr == KERN_NO_THREAD) {
		/* No thread attach to this activation */
		bzero(ss, HP700_THREAD_STATE_COUNT);
		bzero(fs, HP700_FLOAT_STATE_COUNT);
		return;
	}

	if (kr != KERN_SUCCESS && kr != KERN_NO_THREAD) {
		printf("tgdb: can't abort thread\n");
		return;
	}

	count = HP700_THREAD_STATE_COUNT;
	kr = thread_get_state(thread, HP700_THREAD_STATE, (thread_state_t)ss, &count);
	if (kr != KERN_SUCCESS) {
		printf("tgdb: can't get thread state\n");
		return;
	}
	ss->iioq_head &= ~3;
	ss->iioq_tail &= ~3;
	if(fs) {
		if(ss->fpu) {
			count = HP700_FLOAT_STATE_COUNT;
			kr = thread_get_state(thread, HP700_FLOAT_STATE, (thread_state_t)fs, &count);
			if (kr != KERN_SUCCESS) {
				printf("tgdb: can't get float thread state\n");
				return;
			}	
		}
		else {
			bzero(fs, HP700_FLOAT_STATE_COUNT);
		}
	}
}

/*
** tgdb_get_break_addr: Used to get the address of the instruction the
**                      breakpoint is on.
*/
int
tgdb_get_break_addr(mach_port_t thread)
{
	struct hp700_thread_state ss;

	tgdb_get_registers(thread, &ss, 0);
	return (int)ss.iioq_head;

}

void
tgdb_set_registers(mach_port_t thread, struct hp700_thread_state *ss, struct hp700_float_state *fs)
{
	kern_return_t kr;

#if 1
	kr = thread_abort(thread);

	if (kr == KERN_NO_THREAD) {
		return;
	}
#endif

	kr = thread_set_state(thread, HP700_THREAD_STATE, (thread_state_t)ss, HP700_THREAD_STATE_COUNT);
	if (kr != KERN_SUCCESS) {
		printf("tgdb: can't set thread state\n");
		return;
	}
	if(ss->fpu && fs) {
		kr = thread_set_state(thread, HP700_FLOAT_STATE,
				 (thread_state_t)fs, HP700_FLOAT_STATE_COUNT);
		if (kr != KERN_SUCCESS) {
			printf("tgdb: can't set float state\n");
			return;
		}
	}

	return;
}

/*
 * Set the trace bit in the thread's PSW.  This will cause it to
 * execute a single user-mode instruction, then raise a breakpoint
 * exception.
 */
void
tgdb_set_trace(mach_port_t thread)
{
	struct hp700_thread_state ss;

	tgdb_get_registers(thread, &ss, 0);
	ss.ipsw |= PSW_R;
	ss.rctr = 0;
	tgdb_set_registers(thread, &ss, 0);
}

/*
 * Suspend thread, retrieve registers, and clear the trace bit in the
 * thread's PSW.
 */
void
tgdb_clear_trace(mach_port_t thread)
{
	struct hp700_thread_state ss;

	if (thread_suspend(thread) != KERN_SUCCESS) {
		printf("tgdb: can't suspend thread\n");
		return;
	}
	tgdb_get_registers(thread, &ss, 0);
	ss.ipsw &= ~PSW_R;
	tgdb_set_registers(thread, &ss, 0);
}

/*
 * Retrieve basic information about each thread in the task.  This will
 * be simply formatted and printed for the remote gdb user.
 */
void
tgdb_basic_thread_info(
	mach_port_t thread,
	mach_port_t task,
	unsigned int *buffer)
{
	struct thread_basic_info tbi;
	unsigned int size = sizeof(tbi);
	struct hp700_thread_state ss;
	kern_return_t kr;
	
	kr = thread_info(thread, THREAD_BASIC_INFO,
			 (thread_info_t)&tbi, &size);
	if (kr == KERN_NO_THREAD) {
		*buffer++ = 0;	/* run state */
		*buffer++ = 0;	/* flags */
		*buffer++ = 0;  /* susp count */
		*buffer++ = 0;  /* pc */
	} else {
		tgdb_get_registers(thread, &ss, 0);
	  	*buffer++ = tbi.run_state;
		*buffer++ = tbi.flags;
		*buffer++ = tbi.suspend_count;
		if (ss.flags & 2) {
			if (tgdb_vm_read(task,
					 ss.r3 - 0x14, /* FM_CRP */
					 sizeof (*buffer),
					 (vm_offset_t) buffer) != KERN_SUCCESS)
				*buffer = ss.r31;
		} else
	  		*buffer = ss.iioq_head;
		*buffer++ &= ~0x3;
	}
}

/*
 * Macro for copying doubles without using the float registers, and
 * allowing for doubles that are on 4 byte boundaries instead of 8
 * byte boundaries.
 */

#define dblcpy(dst,src)						\
{								\
  ((unsigned*)&(dst))[0] = ((const unsigned*)&(src))[0];	\
  ((unsigned*)&(dst))[1] = ((const unsigned*)&(src))[1];	\
}

void
hp700ts_to_gdb(struct hp700_thread_state *hp, struct hp700_float_state *fp,
	       struct gdb_registers *gdb)
{
  gdb->flags		= hp->flags;
  gdb->r1		= hp->r1;
  gdb->rp		= hp->rp;
  gdb->r3		= hp->r3;
  gdb->r4		= hp->r4;
  gdb->r5		= hp->r5;
  gdb->r6		= hp->r6;
  gdb->r7		= hp->r7;
  gdb->r8		= hp->r8;
  gdb->r9		= hp->r9;
  gdb->r10		= hp->r10;
  gdb->r11		= hp->r11;
  gdb->r12		= hp->r12;
  gdb->r13		= hp->r13;
  gdb->r14		= hp->r14;
  gdb->r15		= hp->r15;
  gdb->r16		= hp->r16;
  gdb->r17		= hp->r17;
  gdb->r18		= hp->r18;
  gdb->t4		= hp->t4;
  gdb->t3		= hp->t3;
  gdb->t2		= hp->t2;
  gdb->t1		= hp->t1;
  gdb->arg3		= hp->arg3;
  gdb->arg2		= hp->arg2;
  gdb->arg1		= hp->arg1;
  gdb->arg0		= hp->arg0;
  gdb->dp		= hp->dp;
  gdb->ret0		= hp->ret0;
  gdb->ret1		= hp->ret1;
  gdb->sp		= hp->sp;
  gdb->r31		= hp->r31;

  gdb->sr0		= hp->sr0;
  gdb->sr1		= hp->sr1;
  gdb->sr2		= hp->sr2;
  gdb->sr3		= hp->sr3;
  gdb->sr4		= hp->sr4;
  gdb->sr5		= hp->sr5;
  gdb->sr6		= hp->sr6;
  gdb->sr7		= hp->sr7;

  gdb->rctr		= hp->rctr;
  gdb->pidr1		= hp->pidr1;
  gdb->pidr2		= hp->pidr2;
  gdb->ccr		= hp->ccr;
  gdb->sar		= hp->sar;
  gdb->pidr3		= hp->pidr3;
  gdb->pidr4		= hp->pidr4;

  gdb->iioq_head	= hp->iioq_head;
  gdb->iisq_head	= hp->iisq_head;
  gdb->iioq_tail	= hp->iioq_tail;
  gdb->iisq_tail	= hp->iisq_tail;

  gdb->ipsw		= hp->ipsw;

  dblcpy( gdb->fr0,	fp->fr0 );
  dblcpy( gdb->fr1,	fp->fr1 );
  dblcpy( gdb->fr2,	fp->fr2 );
  dblcpy( gdb->fr3,	fp->fr3 );
  dblcpy( gdb->farg0,	fp->farg0 );
  dblcpy( gdb->farg1,	fp->farg1 );
  dblcpy( gdb->farg2,	fp->farg2 );
  dblcpy( gdb->farg3,	fp->farg3 );
  dblcpy( gdb->fr8,	fp->fr8 );
  dblcpy( gdb->fr9,	fp->fr9 );
  dblcpy( gdb->fr10,	fp->fr10 );
  dblcpy( gdb->fr11,	fp->fr11 );
  dblcpy( gdb->fr12,	fp->fr12 );
  dblcpy( gdb->fr13,	fp->fr13 );
  dblcpy( gdb->fr14,	fp->fr14 );
  dblcpy( gdb->fr15,	fp->fr15 );
  dblcpy( gdb->fr16,	fp->fr16 );
  dblcpy( gdb->fr17,	fp->fr17 );
  dblcpy( gdb->fr18,	fp->fr18 );
  dblcpy( gdb->fr19,	fp->fr19 );
  dblcpy( gdb->fr20,	fp->fr20 );
  dblcpy( gdb->fr21,	fp->fr21 );
  dblcpy( gdb->fr22,	fp->fr22 );
  dblcpy( gdb->fr23,	fp->fr23 );
  dblcpy( gdb->fr24,	fp->fr24 );
  dblcpy( gdb->fr25,	fp->fr25 );
  dblcpy( gdb->fr26,	fp->fr26 );
  dblcpy( gdb->fr27,	fp->fr27 );
  dblcpy( gdb->fr28,	fp->fr28 );
  dblcpy( gdb->fr29,	fp->fr29 );
  dblcpy( gdb->fr30,	fp->fr30 );
  dblcpy( gdb->fr31,	fp->fr31 );
}

void
gdb_to_hp700ts(struct gdb_registers *gdb,
	       struct hp700_thread_state *hp, struct hp700_float_state *fp)
{
  hp->r1		= gdb->r1;
  hp->rp		= gdb->rp;
  hp->r3		= gdb->r3;
  hp->r4		= gdb->r4;
  hp->r5		= gdb->r5;
  hp->r6		= gdb->r6;
  hp->r7		= gdb->r7;
  hp->r8		= gdb->r8;
  hp->r9		= gdb->r9;
  hp->r10		= gdb->r10;
  hp->r11		= gdb->r11;
  hp->r12		= gdb->r12;
  hp->r13		= gdb->r13;
  hp->r14		= gdb->r14;
  hp->r15		= gdb->r15;
  hp->r16		= gdb->r16;
  hp->r17		= gdb->r17;
  hp->r18		= gdb->r18;
  hp->t4		= gdb->t4;
  hp->t3		= gdb->t3;
  hp->t2		= gdb->t2;
  hp->t1		= gdb->t1;
  hp->arg3		= gdb->arg3;
  hp->arg2		= gdb->arg2;
  hp->arg1		= gdb->arg1;
  hp->arg0		= gdb->arg0;
  hp->dp		= gdb->dp;
  hp->ret0		= gdb->ret0;
  hp->ret1		= gdb->ret1;
  hp->sp		= gdb->sp;
  hp->r31		= gdb->r31;

  hp->sar		= gdb->sar;

  hp->iioq_head		= gdb->iioq_head;
  hp->iisq_head		= gdb->iisq_head;
  hp->iioq_tail		= gdb->iioq_tail;
  hp->iisq_tail		= gdb->iisq_tail;

  hp->eiem		= gdb->eiem;
  hp->iir		= gdb->iir;
  hp->isr		= gdb->isr;
  hp->ior		= gdb->ior;
  hp->ipsw		= gdb->ipsw;

  hp->sr0		= gdb->sr0;
  hp->sr1		= gdb->sr1;
  hp->sr2		= gdb->sr2;
  hp->sr3		= gdb->sr3;

  hp->ipsw		= gdb->ipsw;

  dblcpy( fp->fr0,	gdb->fr0 );
  dblcpy( fp->fr1,	gdb->fr1 );
  dblcpy( fp->fr2,	gdb->fr2 );
  dblcpy( fp->fr3,	gdb->fr3 );
  dblcpy( fp->farg0,	gdb->farg0 );
  dblcpy( fp->farg1,	gdb->farg1 );
  dblcpy( fp->farg2,	gdb->farg2 );
  dblcpy( fp->farg3,	gdb->farg3 );
  dblcpy( fp->fr8,	gdb->fr8 );
  dblcpy( fp->fr9,	gdb->fr9 );
  dblcpy( fp->fr10,	gdb->fr10 );
  dblcpy( fp->fr11,	gdb->fr11 );
  dblcpy( fp->fr12,	gdb->fr12 );
  dblcpy( fp->fr13,	gdb->fr13 );
  dblcpy( fp->fr14,	gdb->fr14 );
  dblcpy( fp->fr15,	gdb->fr15 );
  dblcpy( fp->fr16,	gdb->fr16 );
  dblcpy( fp->fr17,	gdb->fr17 );
  dblcpy( fp->fr18,	gdb->fr18 );
  dblcpy( fp->fr19,	gdb->fr19 );
  dblcpy( fp->fr20,	gdb->fr20 );
  dblcpy( fp->fr21,	gdb->fr21 );
  dblcpy( fp->fr22,	gdb->fr22 );
  dblcpy( fp->fr23,	gdb->fr23 );
  dblcpy( fp->fr24,	gdb->fr24 );
  dblcpy( fp->fr25,	gdb->fr25 );
  dblcpy( fp->fr26,	gdb->fr26 );
  dblcpy( fp->fr27,	gdb->fr27 );
  dblcpy( fp->fr28,	gdb->fr28 );
  dblcpy( fp->fr29,	gdb->fr29 );
  dblcpy( fp->fr30,	gdb->fr30 );
  dblcpy( fp->fr31,	gdb->fr31 );
}
