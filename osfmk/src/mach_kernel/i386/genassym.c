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
 * Revision 2.12.7.2  92/03/03  16:15:30  jeffreyh
 * 	Pick up changes from TRUNK
 * 	[92/02/26  11:10:26  jeffreyh]
 * 
 * Revision 2.13  92/01/03  20:06:12  dbg
 * 	Add DISP_MIN.
 * 	[91/10/31            dbg]
 * 
 * Revision 2.12.7.1  92/02/18  18:45:09  jeffreyh
 * 	Support for the Corollary MP
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.12  91/07/31  17:36:13  dbg
 * 	Add microsecond timing.
 * 
 * 	Save user registers in PCB and switch to separate kernel stack
 * 	on entry.
 * 	[91/07/30  16:50:29  dbg]
 * 
 * Revision 2.11  91/06/19  11:55:05  rvb
 * 	cputypes.h->platforms.h
 * 	[91/06/12  13:44:45  rvb]
 * 
 * Revision 2.10  91/05/14  16:08:23  mrt
 * 	Correcting copyright
 * 
 * Revision 2.9  91/05/08  12:37:41  dbg
 * 	Add definitions for multiprocessors and for Sequent Symmetry.
 * 	Change '#define' to '.set' to help fool GCC preprocessor.
 * 	[91/04/26  14:35:03  dbg]
 * 
 * Revision 2.8  91/03/16  14:44:15  rpd
 * 	Removed k_ipl.
 * 	[91/03/01            rpd]
 * 	Added TH_SWAP_FUNC.
 * 	[91/02/24            rpd]
 * 
 * 	Added PCB_SIZE.
 * 	[91/02/01            rpd]
 * 
 * Revision 2.7  91/02/05  17:11:56  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:34:22  mrt]
 * 
 * Revision 2.6  91/01/09  22:41:19  rpd
 * 	Removed user_regs, k_stack_top.
 * 	[91/01/09            rpd]
 * 
 * Revision 2.5  91/01/08  15:10:36  rpd
 * 	Reorganized the pcb.
 * 	[90/12/11            rpd]
 * 
 * Revision 2.4  90/12/04  14:45:58  jsb
 * 	Changes for merged intel/pmap.{c,h}.
 * 	[90/12/04  11:15:00  jsb]
 * 
 * Revision 2.3  90/08/27  21:56:44  dbg
 * 	Use new names from new seg.h.
 * 	[90/07/25            dbg]
 * 
 * Revision 2.2  90/05/03  15:27:32  dbg
 * 	Created.
 * 	[90/02/11            dbg]
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

#include <platforms.h>
#include <cpus.h>
#include <mach_kdb.h>
#include <mach_ldebug.h>
#include <stat_time.h>

/*
 * Pass field offsets to assembly code.
 */
#include <kern/ast.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <kern/lock.h>
#include <kern/thread_act.h>
#include <kern/thread_pool.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <kern/host.h>
#include <kern/misc_protos.h>
#include <kern/syscall_emulation.h>
#include <i386/thread.h>
#include <mach/i386/vm_param.h>
#include <i386/seg.h>
#include <i386/pmap.h>
#include <i386/tss.h>
#include <mach/rpc.h>
#include <vm/vm_map.h>
#if	NCPUS > 1
#include <i386/mp_desc.h>
#endif
#ifdef	SYMMETRY
#include <sqt/slic.h>
#include <sqt/intctl.h>
#endif
#ifdef	SQT
#include <i386/SQT/slic.h>
#include <i386/SQT/intctl.h>
#endif	/* SQT */
#ifdef	CBUS
#include <busses/cbus/cbus.h>
#endif	/* CBUS */

extern void   kernel_preempt_check(void);
cpu_data_t    cpu_data[NCPUS];

/*
 * genassym.c is used to produce an
 * assembly file which, intermingled with unuseful assembly code,
 * has all the necessary definitions emitted. This assembly file is
 * then postprocessed with sed to extract only these definitions
 * and thus the final assyms.s is created.
 *
 * This convoluted means is necessary since the structure alignment
 * and packing may be different between the host machine and the
 * target so we are forced into using the cross compiler to generate
 * the values, but we cannot run anything on the target machine.
 */

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE)0)->MEMBER)

#define DECLARE(SYM,VAL) \
	__asm("#DEFINITION#\t.set\t" SYM ",\t%0" : : "n" ((u_int)(VAL)))

int	main(
		int		argc,
		char		** argv);

int
main(
	int	argc,
	char	**argv)
{

	DECLARE("AST_URGENT",		AST_URGENT);

#if	MACH_LDEBUG
	/*
	 * XXX 
	 */
#define	SIMPLE_LOCK_TAG	0x5353
#define	MUTEX_TAG	0x4d4d
	DECLARE("TH_MUTEX_COUNT",	offsetof(thread_t, mutex_count));
	DECLARE("SIMPLE_LOCK_TAG",	SIMPLE_LOCK_TAG);
	DECLARE("MUTEX_TAG",		MUTEX_TAG);
#endif	/* MACH_LDEBUG */
	DECLARE("TH_RECOVER",		offsetof(thread_t, recover));
	DECLARE("TH_CONTINUATION",	offsetof(thread_t, continuation));
	DECLARE("TH_TOP_ACT",		offsetof(thread_t, top_act));
	DECLARE("TH_KERNEL_STACK",	offsetof(thread_t, kernel_stack));

	DECLARE("TASK_EMUL", 		offsetof(task_t, eml_dispatch));
	DECLARE("TASK_MACH_EXC_PORT",
		offsetof(task_t, exc_actions[EXC_MACH_SYSCALL].port));

	/* These fields are being added on demand */
	DECLARE("ACT_MACH_EXC_PORT",
		offsetof(thread_act_t, exc_actions[EXC_MACH_SYSCALL].port));

        DECLARE("ACT_THREAD",	offsetof(thread_act_t, thread));
	DECLARE("ACT_TASK",	offsetof(thread_act_t, task));
	DECLARE("ACT_PCB",	offsetof(thread_act_t, mact.pcb));
	DECLARE("ACT_KLOADED",	offsetof(thread_act_t, kernel_loaded));
	DECLARE("ACT_KLOADING",	offsetof(thread_act_t, kernel_loading));
	DECLARE("ACT_LOWER",	offsetof(thread_act_t, lower));
	DECLARE("ACT_MAP",	offsetof(thread_act_t, map));

	DECLARE("MAP_PMAP",	offsetof(vm_map_t, pmap));

	/* XXX Until rpc buffers move from kernel stack to activation */
	DECLARE("RPC_CLIENT_BUF_SIZE",	 
			2 * RPC_KBUF_SIZE * sizeof(int) + 
			RPC_DESC_COUNT * sizeof(rpc_copy_state_data_t) +
			40 * sizeof(int));

	DECLARE("HOST_NAME",	offsetof(host_t, host_self));

	DECLARE("DISP_MIN",	offsetof(eml_dispatch_t, disp_min));
	DECLARE("DISP_COUNT",	offsetof(eml_dispatch_t, disp_count));
	DECLARE("DISP_VECTOR",	offsetof(eml_dispatch_t, disp_vector[0]));

#define IKS ((size_t) (STACK_IKS(0)))

	DECLARE("KSS_EBX", IKS + offsetof(struct i386_kernel_state *, k_ebx));
	DECLARE("KSS_ESP", IKS + offsetof(struct i386_kernel_state *, k_esp));
	DECLARE("KSS_EBP", IKS + offsetof(struct i386_kernel_state *, k_ebp));
	DECLARE("KSS_EDI", IKS + offsetof(struct i386_kernel_state *, k_edi));
	DECLARE("KSS_ESI", IKS + offsetof(struct i386_kernel_state *, k_esi));
	DECLARE("KSS_EIP", IKS + offsetof(struct i386_kernel_state *, k_eip));

	DECLARE("IKS_SIZE",	sizeof(struct i386_kernel_state));
	DECLARE("IEL_SIZE",	sizeof(struct i386_exception_link));

	DECLARE("PCB_FPS",	offsetof(pcb_t, ims.ifps));
	DECLARE("PCB_ISS",	offsetof(pcb_t, iss));

	DECLARE("FP_VALID",	offsetof(struct i386_fpsave_state *,fp_valid));
	DECLARE("FP_SAVE_STATE",
		offsetof(struct i386_fpsave_state *, fp_save_state));

	DECLARE("R_CS",		offsetof(struct i386_saved_state *, cs));
	DECLARE("R_SS",		offsetof(struct i386_saved_state *, ss));
	DECLARE("R_UESP",	offsetof(struct i386_saved_state *, uesp));
	DECLARE("R_EBP",	offsetof(struct i386_saved_state *, ebp));
	DECLARE("R_EAX",	offsetof(struct i386_saved_state *, eax));
	DECLARE("R_EBX",	offsetof(struct i386_saved_state *, ebx));
	DECLARE("R_ECX",	offsetof(struct i386_saved_state *, ecx));
	DECLARE("R_EDX",	offsetof(struct i386_saved_state *, edx));
	DECLARE("R_ESI",	offsetof(struct i386_saved_state *, esi));
	DECLARE("R_EDI",	offsetof(struct i386_saved_state *, edi));
	DECLARE("R_TRAPNO",	offsetof(struct i386_saved_state *, trapno));
	DECLARE("R_ERR",	offsetof(struct i386_saved_state *, err));
	DECLARE("R_EFLAGS",	offsetof(struct i386_saved_state *, efl));
	DECLARE("R_EIP",	offsetof(struct i386_saved_state *, eip));
	DECLARE("R_CR2",	offsetof(struct i386_saved_state *, cr2));
	DECLARE("ISS_SIZE",	sizeof (struct i386_saved_state));

        DECLARE("I_ECX",	offsetof(struct i386_interrupt_state *, ecx));
	DECLARE("I_EIP",	offsetof(struct i386_interrupt_state *, eip));
	DECLARE("I_CS",		offsetof(struct i386_interrupt_state *, cs));
	DECLARE("I_EFL",	offsetof(struct i386_interrupt_state *, efl));

	DECLARE("NBPG",			I386_PGBYTES);
	DECLARE("VM_MIN_ADDRESS",	VM_MIN_ADDRESS);
	DECLARE("VM_MAX_ADDRESS",	VM_MAX_ADDRESS);
	DECLARE("KERNELBASE",		VM_MIN_KERNEL_ADDRESS);
	DECLARE("LINEAR_KERNELBASE",	LINEAR_KERNEL_ADDRESS);
	DECLARE("KERNEL_STACK_SIZE",	KERNEL_STACK_SIZE);

	DECLARE("PDESHIFT",	PDESHIFT);
	DECLARE("PTESHIFT",	PTESHIFT);
	DECLARE("PTEMASK",	PTEMASK);

	DECLARE("PTE_PFN",	INTEL_PTE_PFN);
	DECLARE("PTE_V",	INTEL_PTE_VALID);
	DECLARE("PTE_W",	INTEL_PTE_WRITE);
	DECLARE("PTE_INVALID",	~INTEL_PTE_VALID);

	DECLARE("IDTSZ",	IDTSZ);
	DECLARE("GDTSZ",	GDTSZ);
	DECLARE("LDTSZ",	LDTSZ);

	DECLARE("KERNEL_CS",	KERNEL_CS);
	DECLARE("KERNEL_DS",	KERNEL_DS);
	DECLARE("USER_CS",	USER_CS);
	DECLARE("USER_DS",	USER_DS);
	DECLARE("KERNEL_TSS",	KERNEL_TSS);
	DECLARE("KERNEL_LDT",	KERNEL_LDT);
#if	MACH_KDB
	DECLARE("DEBUG_TSS",	DEBUG_TSS);
#endif	/* MACH_KDB */

        DECLARE("CPU_DATA",	CPU_DATA);
        DECLARE("CPD_ACTIVE_THREAD",
		offsetof(cpu_data_t *, active_thread));
#if	MACH_RT
        DECLARE("CPD_PREEMPTION_LEVEL",
		offsetof(cpu_data_t *, preemption_level));
#endif	/* MACH_RT */
        DECLARE("CPD_INTERRUPT_LEVEL",
		offsetof(cpu_data_t *, interrupt_level));
        DECLARE("CPD_SIMPLE_LOCK_COUNT",
		offsetof(cpu_data_t *,simple_lock_count));

	DECLARE("PTES_PER_PAGE",	NPTES);
	DECLARE("INTEL_PTE_KERNEL",	INTEL_PTE_VALID|INTEL_PTE_WRITE);

	DECLARE("KERNELBASEPDE",
		(LINEAR_KERNEL_ADDRESS >> PDESHIFT) *
		sizeof(pt_entry_t));
#ifdef	CBUS
	DECLARE("CBUS_START_PDE",	
		(CBUS_START >> PDESHIFT) *
		sizeof(pt_entry_t));
#endif	/* CBUS */

	DECLARE("TSS_ESP0",	offsetof(struct i386_tss *, esp0));
	DECLARE("TSS_SS0",	offsetof(struct i386_tss *, ss0));
	DECLARE("TSS_LDT",	offsetof(struct i386_tss *, ldt));
	DECLARE("TSS_PDBR",	offsetof(struct i386_tss *, cr3));
	DECLARE("TSS_LINK",	offsetof(struct i386_tss *, back_link));

	DECLARE("K_TASK_GATE",	ACC_P|ACC_PL_K|ACC_TASK_GATE);
	DECLARE("K_TRAP_GATE",	ACC_P|ACC_PL_K|ACC_TRAP_GATE);
	DECLARE("U_TRAP_GATE",	ACC_P|ACC_PL_U|ACC_TRAP_GATE);
	DECLARE("K_INTR_GATE",	ACC_P|ACC_PL_K|ACC_INTR_GATE);
	DECLARE("K_TSS",	ACC_P|ACC_PL_K|ACC_TSS);

	/*
	 *	usimple_lock fields
	 */
	DECLARE("USL_INTERLOCK",	offsetof(usimple_lock_t, interlock));

	DECLARE("INTSTACK_SIZE",	INTSTACK_SIZE);
#if	NCPUS > 1
	DECLARE("MP_GDT",	   offsetof(struct mp_desc_table *, gdt[0]));
	DECLARE("MP_IDT",	   offsetof(struct mp_desc_table *, idt[0]));
#endif	/* NCPUS > 1 */
#if	!STAT_TIME
	DECLARE("LOW_BITS",	   offsetof(struct timer *, low_bits));
	DECLARE("HIGH_BITS",	   offsetof(struct timer *, high_bits));
	DECLARE("HIGH_BITS_CHECK", offsetof(struct timer *, high_bits_check));
	DECLARE("TIMER_HIGH_UNIT", TIMER_HIGH_UNIT);
	DECLARE("TH_SYS_TIMER",    offsetof(struct timer *, system_timer));
	DECLARE("TH_USER_TIMER",   offsetof(struct timer *, user_timer));
#endif
#ifdef	SYMMETRY
	DECLARE("VA_SLIC",	VA_SLIC);
	DECLARE("VA_ETC",	VA_ETC);
	DECLARE("SL_PROCID",	offsetof(struct cpuslic *, sl_procid));
	DECLARE("SL_LMASK",	offsetof(struct cpuslic *, sl_lmask));
	DECLARE("SL_BININT",	offsetof(struct cpuslic *, sl_binint));
	DECLARE("SL_B0INT",	offsetof(struct cpuslic *, sl_b0int));
	DECLARE("BH_SIZE",	offsetof(struct bin_header *, bh_size));
	DECLARE("BH_HDLRTAB",	offsetof(struct bin_header *, bh_hdlrtab));
#endif	/* SYMMETRY */
#ifdef	SQT
	DECLARE("VA_SLIC",	VA_SLIC);
	DECLARE("VA_ETC",	VA_ETC);
	DECLARE("SL_PROCID",	offsetof(struct cpuslic *, sl_procid));
	DECLARE("SL_LMASK",	offsetof(struct cpuslic *, sl_lmask));
	DECLARE("SL_BININT",	offsetof(struct cpuslic *, sl_binint));
	DECLARE("SL_B0INT",	offsetof(struct cpuslic *, sl_b0int));
	DECLARE("BH_SIZE",	offsetof(struct bin_header *, bh_size));
	DECLARE("BH_HDLRTAB",	offsetof(struct bin_header *, bh_hdlrtab));
	DECLARE("SLIC_PDE",	pdenum(VA_SLIC)); 
	DECLARE("KV_PDE",	pdenum(VM_MIN_KERNEL_ADDRESS)); 
#endif	/* SQT */

	return (0);
}


/* Dummy to keep linker quiet */
void
kernel_preempt_check(void)
{
}
