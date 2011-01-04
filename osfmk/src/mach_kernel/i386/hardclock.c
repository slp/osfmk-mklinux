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
 * Revision 2.8.6.3  92/09/15  17:14:51  jeffreyh
 * 	Added support for profiling of kernel tasks & threads
 * 	For MPs do not propagate clock int to all CPUs at the same time
 * 	[92/07/24            bernadat]
 * 
 * Revision 2.8.6.2  92/04/30  11:50:12  bernadat
 * 	Adaptations for Corollary and Systempro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.8.6.1  92/02/18  18:45:23  jeffreyh
 * 	Added timestamp measurement variables for Corollary
 * 	[91/12/06            bernadat]
 * 
 * 	Changed clock_interrupt interface for profiling
 * 	(Bernard Tabib & Andrei Danes @ gr.osf.org)
 * 	[91/09/16            bernadat]
 * 
 * 	Support for the Corollary MP
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.8  91/07/31  17:36:33  dbg
 * 	New interrupt save area.
 * 	[91/07/30  16:50:59  dbg]
 * 
 * Revision 2.7  91/06/19  11:55:09  rvb
 * 	cputypes.h->platforms.h
 * 	[91/06/12  13:44:49  rvb]
 * 
 * Revision 2.6  91/05/14  16:08:38  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/05/08  12:37:56  dbg
 * 	Include sqt/intctl.h if building for Sequent.
 * 	[91/03/21            dbg]
 * 
 * Revision 2.4  91/02/05  17:12:01  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:34:29  mrt]
 * 
 * Revision 2.3  91/01/08  17:32:02  rpd
 * 	EFL_VM => user_mode
 * 	[90/12/21  10:50:54  rvb]
 * 
 * Revision 2.2  90/05/03  15:27:35  dbg
 * 	Created.
 * 	[90/03/06            dbg]
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
 * Clock interrupt.
 */
#include <cpus.h>
#include <time_stamp.h>
#include <mach_kdb.h>
#include <kern/cpu_number.h>
#include <kern/kern_types.h>
#include <platforms.h>
#include <mp_v1_1.h>
#include <mach_kprof.h>
#include <mach_mp_debug.h>
#include <mach/std_types.h>

#include <mach/clock_types.h>
#include <mach/boolean.h>
#include <i386/thread.h>
#include <i386/eflags.h>
#include <kern/assert.h>
#include <kern/misc_protos.h>
#include <i386/misc_protos.h>
#include <kern/time_out.h>

#ifdef	SYMMETRY
#include <sqt/intctl.h>
#endif

#ifdef	SQT
#include <sys/types.h>
#include <i386/SQT/intctl.h>
#endif	/* SQT */

#if	defined(AT386) || defined(iPSC386)
#include <i386/ipl.h>
#endif

#include <i386/hardclock_entries.h>
#include <i386/rtclock_entries.h>
#include <kern/time_out.h>

#if	MACH_MP_DEBUG
#include <i386/mach_param.h>	/* for HZ */
#endif	/* MACH_MP_DEBUG */

extern	char	return_to_iret[];

#if	TIME_STAMP && AT386 && NCPUS > 1
extern	unsigned time_stamp;
unsigned old_time_stamp, time_stamp_cum, nstamps;

/*
 *	If H/W provides a counter, record number of ticks and cumulated
 *	time stamps to know timestamps rate.
 *	This should go away when ALARMCLOCKS installed
 */
#define time_stamp_stat()					\
	if (my_cpu == 0)					\
	if (!old_time_stamp) {					\
		old_time_stamp = time_stamp;			\
		nstamps = 0;					\
	} else {						\
		nstamps++;					\
		time_stamp_cum = (time_stamp - old_time_stamp);	\
	}
#else	/* TIME_STAMP && AT386 && NCPUS > 1 */
#define time_stamp_stat()
#endif	/* TIME_STAMP && AT386 && NCPUS > 1 */

#if	MACH_KPROF
int	masked_pc[NCPUS];
int	missed_clock[NCPUS];
int	detect_lost_tick = 0;
#endif	/* MACH_KPROF */

#if	MACH_MP_DEBUG
int	masked_state_cnt[NCPUS];
int	masked_state_max = 10*HZ;
#endif	/* MACH_MP_DEBUG */

/*
 * In the interest of a fast clock interrupt service path,
 * this routine should be folded into assembly language with
 * a direct interrupt vector on the i386. The "pit" interrupt
 * should always call the rtclock_intr() routine on the master
 * processor. The return value of the rtclock_intr() routine
 * indicates whether HZ rate clock processing should be
 * performed. (On the Sequent, all slave processors will
 * run at HZ rate). For now, we'll leave this routine in C
 * (with TIME_STAMP, MACH_MP_DEBUG and MACH_KPROF code this
 * routine is way too large for assembler anyway).
 */

#ifdef	PARANOID_KDB
int paranoid_debugger = TRUE;
int paranoid_count = 1000;
int paranoid_current = 0;
int paranoid_cpu = 0;
#endif	/* PARANOID_KDB */

void
hardclock(
	int				ivect,
				/* interrupt number */
	int				old_ipl,
				/* old interrupt level */
	char				* ret_addr,
				/* return address in interrupt handler */
	struct i386_interrupt_state	*regs)
				/* saved registers */
{
	int mycpu;
	register unsigned pc;
	register boolean_t usermode;

	mp_disable_preemption();
	mycpu = cpu_number();

#ifdef	PARANOID_KDB
	if (paranoid_cpu == mycpu &&
	    paranoid_current++ >= paranoid_count) {
		paranoid_current = 0;
		if (paranoid_debugger)
		    Debugger("hardclock");
	}
#endif	/* PARANOID_KDB */

#if	MACH_MP_DEBUG
	/*
	 * Increments counter of clock ticks handled under a masked state.
	 * Debugger() is called if masked state is kept during 1 sec.
	 * The counter is reset by splx() when ipl mask is set back to SPL0,
	 * and by spl0().
	 */
	if (SPL_CMP_GT((old_ipl & 0xFF), SPL0)) {
		if (masked_state_cnt[mycpu]++ >= masked_state_max) {
			int max_save = masked_state_max;

			masked_state_cnt[mycpu] = 0;
			masked_state_max = 0x7fffffff;

			if (ret_addr == return_to_iret) {
				usermode = (regs->efl & EFL_VM) ||
						((regs->cs & 0x03) != 0);
				pc = (unsigned)regs->eip;
			} else {
				usermode = FALSE;
				pc = (unsigned)
				((struct i386_interrupt_state *)&old_ipl)->eip;
			}
			printf("looping at high IPL, usermode=%d pc=0x%x\n",
					usermode, pc);
			Debugger("");

			masked_state_cnt[mycpu] = 0;
			masked_state_max = max_save;
		}
	} else
		masked_state_cnt[mycpu] = 0;
#endif	/* MACH_MP_DEBUG */

#if	MACH_KPROF
	/*
	 * If we were masked against the clock skip call
	 * to rtclock_intr(). When MACH_KPROF is set, the
	 * clock frequency of the master-cpu is confined
	 * to the HZ rate.
	 */
	if (SPL_CMP_LT(old_ipl & 0xFF, SPL7))
#endif	/* MACH_KPROF */
	/*
	 * The master processor executes the rtclock_intr() routine
	 * on every clock tick. The rtclock_intr() routine returns
	 * a zero value on a HZ tick boundary.
	 */
	if (mycpu == master_cpu) {
		if (rtclock_intr() != 0) {
			mp_enable_preemption();
			return;
		}
	}

	/*
	 * The following code is executed at HZ rate by all processors
	 * in the system. This implies that the clock rate on slave
	 * processors must be HZ rate.
	 */

	time_stamp_stat();

	if (ret_addr == return_to_iret) {
		/*
		 * A kernel-loaded task executing within itself will look like
		 * "kernel mode", here.  This is correct with syscalls
		 * implemented using migrating threads, because it means that  
		 * the time spent in the server by a client thread will be
		 * treated as "system" time for the client thread (and nothing
		 * for the server).  This conforms to the CPU reporting for an
		 * integrated kernel.
		 */
		usermode = (regs->efl & EFL_VM) || ((regs->cs & 0x03) != 0);
		pc = (unsigned)regs->eip;
	} else {
		usermode = FALSE;
		pc = (unsigned)((struct i386_interrupt_state *)&old_ipl)->eip;
	}

#if	MACH_KPROF
	/*
	 * If we were masked against the clock, just memorize pc
	 * and the fact that the clock interrupt is delayed
	 */
	if (SPL_CMP_GE((old_ipl & 0xFF), SPL7)) {
		assert(!usermode);
		if (missed_clock[mycpu]++ && detect_lost_tick > 1)
			Debugger("");
		masked_pc[mycpu] = pc;
	} else
#endif	/* MACH_KPROF */

	hertz_tick(usermode, pc);

#if	NCPUS >1 && AT386
	/*
	 * Instead of having the master processor interrupt
	 * all active processors, each processor in turn interrupts
	 * the next active one. This avoids all slave processors
	 * accessing the same R/W data simultaneously.
	 */
	slave_clock();
#endif	/* NCPUS >1 && AT386 */

	mp_enable_preemption();
}

#if	MACH_KPROF
void
delayed_clock(void)
{
	int	i;
	int	my_cpu;

	mp_disable_preemption();
	my_cpu = cpu_number();

	if (missed_clock[my_cpu] > 1 && detect_lost_tick)
		printf("hardclock: missed %d clock interrupt(s) at %x\n",
		       missed_clock[my_cpu]-1, masked_pc[my_cpu]);
	if (my_cpu == master_cpu) {
		i = rtclock_intr();
		assert(i == 0);
	}
	hertz_tick(0, masked_pc[my_cpu]);
	missed_clock[my_cpu] = 0;

	mp_enable_preemption();
}
#endif	/* MACH_KPROF */
