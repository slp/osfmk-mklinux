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

#include <mach_kgdb.h>
#include <debug.h>
#include <mach_debug.h>

#include <mach/ppc/thread_status.h>
#include <mach/vm_types.h>
#include <kern/thread.h>
#include <kern/misc_protos.h>
#include <kgdb/gdb_defs.h>
#include <kgdb/kgdb_defs.h>
#include <ppc/proc_reg.h>
#include <ppc/pmap.h>
#include <ppc/misc_protos.h>
#include <ppc/kgdb_defs.h>
#include <ppc/exception.h>

/*
 * copyin/out_multiple - the assembler copyin/out functions jump to C for
 * help when the copyin lies over a segment boundary. The C breaks
 * down the copy into two sub-copies and re-calls the assembler with
 * these sub-copies. Very rare occurrance. Warning: These functions are
 * called whilst active_thread->thread_recover is still set.
 */

extern boolean_t copyin_multiple(const char *src,
				 char *dst,
				 vm_size_t count);

boolean_t copyin_multiple(const char *src,
			  char *dst,
			  vm_size_t count)
{
	const char *midpoint;
	vm_size_t first_count;
	boolean_t first_result;

	/* Assert that we've been called because of a segment boundary,
	 * this function is more expensive than the assembler, and should
	 * only be called in this difficult case.
	 */
	assert(((vm_offset_t)src & 0xF0000000) !=
	       ((vm_offset_t)(src + count -1) & 0xF0000000));
#if DEBUG && MACH_KGDB
	KGDB_DEBUG(("copyin across segment boundary,"
		    "src=0x%08x, dst=0x%08x, count=0x%x\n", src, dst, count));
#endif /* DEBUG && MACH_KGDB */
	/* TODO NMGS define sensible constants for segments, and apply
	 * to C and assembler (assembler is much harder)
	 */
	midpoint = (const char*) ((vm_offset_t)(src + count) & 0xF0000000);
	first_count = (midpoint - src);

#if DEBUG && MACH_KGDB
	KGDB_DEBUG(("copyin across segment boundary : copyin("
		    "src=0x%08x, dst=0x%08x ,count=0x%x)\n",
		    src, dst, first_count));
#endif /* DEBUG && MACH_KGDB */
	first_result = copyin(src, dst, first_count);
	
	/* If there was an error, stop now and return error */
	if (first_result != 0)
		return first_result;

	/* otherwise finish the job and return result */
#if DEBUG && MACH_KGDB
	KGDB_DEBUG(("copyin across segment boundary : copyin("
		    "src=0x%08x, dst=0x%08x, count=0x%x)\n",
		    midpoint, dst+first_count, count-first_count));
#endif /* DEBUG && MACH_KGDB */

	return copyin(midpoint, dst + first_count, count-first_count);
}

extern int copyout_multiple(const char *src, char *dst, vm_size_t count);

extern int copyout_multiple(const char *src, char *dst, vm_size_t count)
{
	char *midpoint;
	vm_size_t first_count;
	boolean_t first_result;

	/* Assert that we've been called because of a segment boundary,
	 * this function is more expensive than the assembler, and should
	 * only be called in this difficult case. For copyout, the
	 * segment boundary is on the dst
	 */
	assert(((vm_offset_t)dst & 0xF0000000) !=
	       ((vm_offset_t)(dst + count - 1) & 0xF0000000));

#if DEBUG && MACH_KGDB
	KGDB_DEBUG(("copyout across segment boundary,"
		    "src=0x%08x, dst=0x%08x, count=0x%x\n", src, dst, count));
#endif /* DEBUG && MACH_KGDB */

	/* TODO NMGS define sensible constants for segments, and apply
	 * to C and assembler (assembler is much harder)
	 */
	midpoint = (char *) ((vm_offset_t)(dst + count) & 0xF0000000);
	first_count = (midpoint - dst);

#if DEBUG && MACH_KGDB
	KGDB_DEBUG(("copyout across segment boundary : copyout("
		    "src=0x%08x, dst=0x%08x, count=0x%x)\n", src, dst, count));
#endif /* DEBUG && MACH_KGDB */
	first_result = copyout(src, dst, first_count);
	
	/* If there was an error, stop now and return error */
	if (first_result != 0)
		return first_result;

	/* otherwise finish the job and return result */

#if DEBUG && MACH_KGDB
	KGDB_DEBUG(("copyout across segment boundary : copyout("
		    "src=0x%08x, dst=0x%08x, count=0x%x)\n",
		    src + first_count, midpoint, count-first_count));
#endif /* DEBUG && MACH_KGDB */

	return copyout(src + first_count, midpoint, count-first_count);
}

#if DEBUG && MACH_KGDB
/*
 * This is a debugging routine, calling the assembler routine-proper
 *
 * Load the context for the first kernel thread, and go.
 */

extern void load_context(thread_t thread);

void load_context(thread_t thread)
{
	DPRINTF(("thread addr           =0x%08x\n",thread));
	DPRINTF(("thread top act        =0x%08x\n",thread->top_act));
	DPRINTF(("thread top act pcb.ksp=0x%08x\n",thread->top_act->mact.pcb->ksp));
	DPRINTF(("thread kstack =0x%08x\n",thread->kernel_stack));
	DPRINTF(("thread kss.r1 =0x%08x\n",(STACK_IKS(thread->kernel_stack))->r1));
	DPRINTF(("thread kss.lr =0x%08x\n",(STACK_IKS(thread->kernel_stack))->lr));
	DPRINTF(("calling Load_context\n"));
	Load_context(thread);
}
#endif /* DEBUG && MACH_KGDB */

#if DEBUG
void regDump(struct ppc_saved_state *state)
{
	int i;

	for (i=0; i<32; i++) {
		if ((i % 8) == 0)
			kgdb_printf("\n%4d :",i);
			kgdb_printf(" %08x",*(&state->r0+i));
	}

	kgdb_printf("\n");
	kgdb_printf("cr        = 0x%08x\t\t",state->cr);
	kgdb_printf("xer       = 0x%08x\n",state->xer); 
	kgdb_printf("lr        = 0x%08x\t\t",state->lr); 
	kgdb_printf("ctr       = 0x%08x\n",state->ctr); 
	kgdb_printf("srr0(iar) = 0x%08x\t\t",state->srr0); 
	kgdb_printf("srr1(msr) = 0x%08B\n",state->srr1,
		    "\x10\x11""EE\x12PR\x13""FP\x14ME\x15""FE0\x16SE\x18"
		    "FE1\x19""AL\x1a""EP\x1bIT\x1c""DT");
	kgdb_printf("mq        = 0x%08x\t\t",state->mq);
	kgdb_printf("sr_copyin = 0x%08x\n",state->sr_copyin);
	kgdb_printf("\n");

	/* Be nice - for user tasks, generate some stack trace */
	if (state->srr1 & MASK(MSR_PR)) {
		char *addr = (char*)state->r1;
		unsigned int buf[2];
		for (i = 0; i < 8; i++) {
			if (addr == (char*)NULL)
				break;
			if (!copyin(addr,(char*)buf, 2 * sizeof(int))) {
				printf("0x%08x : %08x\n",buf[0],buf[1]);
				addr = (char*)buf[0];
			} else {
				break;
			}
		}
	}
}
#endif /* DEBUG */

#if MACH_KGDB
struct ppc_saved_state *enterDebugger(unsigned int trap,
				      struct ppc_saved_state *ssp,
				      unsigned int dsisr)
{
	KGDB_DEBUG(("trap = 0x%x (no=0x%x), dsisr=0x%08x\n",
		    trap, trap / T_VECTOR_SIZE,dsisr));
	KGDB_DEBUG(("%c\n%s iar=0x%08x dar=0x%08x dsisr 0x%08b\n",
	       7,
	       trap_type[trap / T_VECTOR_SIZE],
	       ssp->srr0,mfdar(),
	       dsisr,"\20\02NO_TRANS\05PROT\06ILL_I/O\07STORE\12DABR\15EAR"));

	kgdb_trap(trap, trap, ssp);

	return ssp;
}
#endif /* MACH_KGDB */

/*
 * invalidate_cache_for_io
 *
 * Takes cache of those requests which may require to flush the
 * data cache first before invalidation.
 */


void
invalidate_cache_for_io(vm_offset_t area, unsigned count, boolean_t phys)
{
	vm_offset_t aligned_start, aligned_end, end;

	/* For unaligned reads we need to flush any
	 * unaligned cache lines. We invalidate the
	 * rest as this is faster
	 */

	aligned_start = area & ~(CACHE_LINE_SIZE-1);
	if (aligned_start != area)
		flush_dcache(aligned_start, CACHE_LINE_SIZE, phys);

	end = area + count;
	aligned_end = (end & ~(CACHE_LINE_SIZE-1));
	if (aligned_end != end)
		flush_dcache(aligned_end, CACHE_LINE_SIZE, phys);

	invalidate_dcache(area, count, phys);
}
