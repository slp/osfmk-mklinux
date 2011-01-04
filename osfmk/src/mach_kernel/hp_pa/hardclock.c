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
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
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

/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: clock.c 1.18 94/12/14$
 */

/*
 *	hppa low-level clock handling
 */

#include <mach_rt.h>
#include <mach_prof.h>
#include <mach/machine/vm_types.h>
#include <machine/thread.h>
#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/psw.h>
#include <kern/spl.h>
#include <machine/regs.h>
#include <machine/clock.h>
#include <hp_pa/HP700/iotypes.h>
#include <machine/asp.h>
#ifdef MAYFLY
#include <hpmayfly/mayfly.h>
#endif

#include <kern/time_out.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/c_support.h>
#include <hp_pa/HP700/bus_protos.h>
#include <hp_pa/trap.h>

struct time_value time;

/*
 * define a section of memory to get pdc return information. It must be 
 * equivalently mapped since the PDC call will be done in physical mode.
 */

#ifdef	PC596XMT_BUG
#include <sys/time.h>
#endif

#if	MACH_PROF
extern	void splx_pc(void);
#endif	/* MACH_PROF */

/*
 * This is the clock interrupt handling routine.
 * It is enterred from the eirr jump table (eirr_switch.int_action).
 */

void
hardclock(int ssp)
{
	register int s;
	s = splclock();

	if(rtclock_intr() != 0) {
	  	splx(s);
		return;
	} else
		splx(s);

	hertz_tick(USERMODE((struct hp700_saved_state*)ssp),
#if	MACH_PROF
		   (((struct hp700_saved_state*)ssp)->iioq_head == (unsigned int) splx_pc) ?
		   		      ((struct hp700_saved_state*)ssp)->rp :
		   ((struct hp700_saved_state*)ssp)->iioq_head);
#else	/* MACH_PROF */
		   ((struct hp700_saved_state*)ssp)->iioq_head);
#endif	/* MACH_PROF */

#ifdef	PC596XMT_BUG	/* consult "../hpsgc/if_i596.c" for details */
#include "lan.h"
#if NLAN > 0
	{
		extern int pc586_eir[];
		extern struct timeval pc586_toutxmt[];
		register struct timeval *timep = (struct timeval *) &time;
		register struct timeval *toutp;

		/*
		 * Check if our pc596 chip failed to interrupt on transmit.
		 * If it did, force a LAN interrupt for our CPU.
		 *
		 * Note: When there is only one LAN board, help the compiler
		 * optimize this hack (we're in hardclock, ya know).
		 *
		 * Note: Since we are in the clock interrupt code, it's safe
		 * to treat the system time as non-volatile.
		 */

#if NLAN == 1
#define	unit	0
#else
		register int unit;

		for (unit = 0; unit < NLAN; unit++)
#endif
		{
			toutp  = &pc586_toutxmt[unit];
			if (((long)toutp->tv_sec) > 0 && pc586_eir[unit] > 0 &&
			    timercmp(timep, toutp, >)) {
				toutp->tv_sec = -1;
				prochpa->io_eir = pc586_eir[unit];
			}
		}
#if NLAN == 1
#undef	unit
#endif
	}
#endif	/* NLAN > 0 */
#endif	/* PC596XMT_BUG */

#ifdef USELEDS
	/*
	 * Do really important stuff: pulse the heartbeat LED.  Turn off
	 * the disk/LAN LEDs in case we are idle and they got left on.
	 */
	{
		static int heartbeat;
		extern int hz, inledcontrol;

		if (++heartbeat == hz >> 1) {
			if (inledcontrol == 0)
				ledcontrol(0, LED_DISK|LED_LANRCV|LED_LANXMT,
					   LED_PULSE);
			heartbeat = 0;
		}
	}
#endif
}
