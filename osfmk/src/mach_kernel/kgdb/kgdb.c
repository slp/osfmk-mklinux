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

/*
 *	Machine dependent part of kgdb for parisc
 */
#include <kgdb.h>

#include <types.h>
#include <kern/processor.h>
#include <kern/thread.h>
#include <kern/misc_protos.h>
#include <mach/thread_status.h>
#include <mach/vm_param.h>
#include <machine/trap.h>
#include <machine/psw.h>
#include <machine/pmap.h>
#include <hp_pa/break.h>
#include <machine/setjmp.h>
#include <machine/asp.h>
#include <hp_pa/HP700/device.h>
#include <kgdb/kgdb_stub.h>
#include <kgdb/kgdb.h>

/*
 *	flag in pcb to indicate that we have saved kgdb state 
 */
#define	KGDB_STEP	2

volatile boolean_t   kgdb_request_ready = FALSE;
struct gdb_request   kgdb_request_buffer;
struct gdb_response  kgdb_response_buffer;

boolean_t kgdb_initialized = FALSE;
boolean_t kgdb_debug_init = FALSE;
boolean_t kgdb_connected = FALSE;
boolean_t kgdb_active = FALSE;
boolean_t kgdb_stop = FALSE;

int kgdb_dev;			/* unused (for dca.c) */

/*
 *	These are locations in the kernel text used for debugging:
 *
 *	user_sstep is a kgdb break instruction on the gateway page
 *	user_nullify is the next instruction after it
 *	kgdb_breaK_inst is the address of the break in kgdb_break
 */
extern void user_sstep(void);
extern void user_nullify(void);

#ifndef HCC
extern void kgdb_break_inst(void);	/* in asm, this extern is ok */
#endif

/*
** kgdb_read_timer: This routine returns the current inteval timer.
**
*/
unsigned int
kgdb_read_timer(void)

{

	unsigned int regval;

	__asm__ volatile ("mfctl 16,%0" : "=r" (regval));
	return regval;

}

/*
 *	Execute a break instruction that will invoke kgdb
 */
void
kgdb_break(void)
{
	if (kgdb_initialized)
	{
	    __asm__ (".export kgdb_break_inst ; .label kgdb_break_inst");
	    __asm__ ("break 0,5");
	}
}

#if 0 /* XYZZY not used any more */
/*
 *	Enable a page for access, faulting it in if necessary
 */
boolean_t
kgdb_mem_access(
	vm_offset_t	offset,
	vm_prot_t	access)
{
	/* XXX ugly hack for negative numbers masquerading as addresses... */
	if ((offset & 0xFFF80000) == 0xFFF80000)
		return 0;

	return((pmap_extract(pmap_kernel(), offset) == 0) ? 0 : 1);
}
#endif

/*
 *	See if we modified the kernel text and if so flush the caches.
 *	This routine is never called with a range that crosses a page
 *	boundary.
 */
void
kgdb_flush_cache(
	vm_offset_t	offset,
	vm_size_t	length)
{
#if 0
	if (offset >= kernel_text_end)
		return;
#endif

#ifdef	CMU_PMAP
	offset = parisc_atop(offset);

	flush_dcache_region(offset, offset);
	purge_icache_region(offset, offset);
#else	/* CMU_PMAP */
	fdcache(0, offset, PAGE_SIZE);
	ficache(0, offset, PAGE_SIZE);
#endif	/* CMU_PMAP */
}


/*
 *	setup the thread to continue from the current point or a specified 
 *	address.
 */
void
kgdb_continue(
	int		type,
	struct hp700_saved_state *ssp,
	vm_offset_t	addr)
{

	if (addr != (vm_offset_t) 0) {
		ssp->iioq_head = addr;
		ssp->iioq_tail = addr + 4;
		kgdb_debug(256, "new pc = %8.8x\n", (int)addr);
	}

	/*
	** If timing is turned on, lets start the timing now.
	*/
	if (kgdb_do_timing) {
		kgdb_start_time = kgdb_read_timer();
	}

	/*
	 *	Turn off the taken branch trap
	 */
	ssp->ipsw &= ~PSW_T;
}

/*
 * Single stepping has to save some stuff off in the current thread's
 * pcb.  Well, until idle_thread is started there ain't no current
 * thread.  So, if current_thread() is NULL, we use this instead (and
 * keep our fingers crossed). 
 */

static struct pcb no_threads_pcb = { { 0 } }; /* initialized to zeros */

/*
 *	setup the thread to single step from the current location or a
 *	specified address
 */
void
kgdb_single_step(
	int		type,
	struct hp700_saved_state *ssp,
	vm_offset_t	addr)
{
        struct thread_activation *act = current_act();
	pcb_t		pcb = act ? act->mact.pcb : &no_threads_pcb;
	if(!pcb)
	    printf("kgdb_single_step: error no pcb\n");

	if (addr != (vm_offset_t) 0) {
		ssp->iioq_head = addr;
		ssp->iioq_tail = addr + 4;
		kgdb_debug(512, "new pc = %p\n", addr);
	}

	/*
	** If timing is turned on, lets start the timing now.
	*/
	if (kgdb_do_timing) {
		kgdb_start_time = kgdb_read_timer();
	}

	/*
	 *	Save the old contents of the pc queue in the pcb so that we 
	 *	can restore them later.
	 */
	pcb->saved_oqt = ssp->iioq_tail;
	pcb->saved_sqt = ssp->iisq_tail;

	/*
	 *	Put a break instruction in the pc queue so that after executing
	 *	the queue head, we trap. 
	 */
	ssp->iioq_tail = (unsigned) user_sstep;
	ssp->iisq_tail = 0;

	/*
	 *	Set the taken branch trap bit in the PSW so that we can't 
	 *	branch around the breakpoint. Save the instruction at the head
	 *	of the queue in case we need to fix a link register after
	 *	a taken branch trap on a bl or blr.
	 */
	ssp->ipsw |= PSW_T;

	pcb->saved_insn = *(int *)(ssp->iioq_head & ~0x3);
	pcb->kgdb_trace |= KGDB_STEP;
}

/*
 *	fixup the pc if needed
 */
void
kgdb_fixup_pc(
	int				type,
	struct hp700_saved_state	*ssp)
{
	unsigned	insn;
	struct thread_activation   *act = current_act();
	pcb_t		pcb = act ? act->mact.pcb : &no_threads_pcb;

	if(! pcb)
	    printf("kgdb_fixup_pc: error no pcb\n");

	/* FIXME current_thread or pcb could be 0 */
	if (pcb->kgdb_trace & KGDB_STEP) {
		/*
		 *	If we performed a non-nullified branch-and-link,
		 *	the link register will point to user_nullify;
		 *	we must make it point after the original tail.
		 */
		if (type == I_TAKEN_BR) {
			insn = pcb->saved_insn;
			if ((insn & 0xfc000000) == 0xe4000000)	/* ble */
				ssp->r31 = pcb->saved_oqt + 4;
			else if ((insn & 0xfc000000) == 0xe8000000 &&
				 ((insn & 0xe000) == 0x4000 ||  /* blr */
				  (insn & 0xe000) == 0) &&      /* bl */
				 (insn & 0x3e00000) != 0) {	/*link != 0*/
				((int *)ssp)[(insn & 0x3e00000) >> 21] =
					pcb->saved_oqt + 4;
			}
			/* XXX also fiddle tail space on BE? */
		}

		/*
		 *	If we got to the break at user_nullify, we must have 
		 *	executed an instruction that nullified the user_sstep 
		 *	break but did not branch. In this case we must restore
		 *	the PSW N bit that was cleared at the successfully 
		 *	executed break.
		 */
		if (ssp->iioq_head == (unsigned) user_nullify)
			ssp->ipsw |= PSW_N;

		/*
		 *	If we didn't branch (the head or tail of the queue 
		 *	points after user_sstep), then the new tail follows 
		 *	the new head.
		 */
		if (ssp->iioq_head == (unsigned) user_nullify ||
		    ssp->iioq_tail == (unsigned) user_nullify) {
			ssp->iisq_tail = pcb->saved_sqt;
			ssp->iioq_tail = pcb->saved_oqt + 4;
		}

		/*
		 *	Restore the original succeeding pc.
		 */
		ssp->iisq_head = pcb->saved_sqt;
		ssp->iioq_head = pcb->saved_oqt;

		/*
		 *	Disable branch trap
		 */
		ssp->ipsw &= ~PSW_T;
		pcb->kgdb_trace &= ~KGDB_STEP;
	}

	/*
	 * If we arrived at kgdb_break, force us to continue
	 * past this point.
	 */
#ifndef HCC
	if (ssp->iioq_head == (unsigned) kgdb_break_inst) {
		ssp->iioq_head += 4;
		if (ssp->iioq_head == ssp->iioq_tail)
			ssp->iioq_tail += 4;
	}
#endif
}



