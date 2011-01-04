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

#include <cpus.h>
#include <va_list.h>
#include <types.h>

#include <kern/thread.h>
#include <kern/host.h>
#include <kern/lock.h>
#include <ppc/exception.h>
#include <ppc/thread_act.h>
#include <ppc/misc_protos.h>
#include <kern/syscall_sw.h>
#include <kern/ast.h>
#include <ppc/low_trace.h>
#if	NCPUS > 1
#include <ppc/POWERMAC/mp/MPPlugIn.h>
#endif	/* NCPUS > 1 */

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE)0)->MEMBER)

#define DECLARE(SYM,VAL) \
	__asm("#DEFINITION##define\t" SYM "\t%0" : : "n" ((u_int)(VAL)))

int main(void)
{
	/* Process Control Block */

	DECLARE("PCB_FLOAT_STATE", offsetof(struct pcb *, fs));

	/* Floating point state */

	DECLARE("PCB_FS_F0",	offsetof(struct pcb *, fs.fpregs[0]));
	DECLARE("PCB_FS_F1",	offsetof(struct pcb *, fs.fpregs[1]));
	DECLARE("PCB_FS_F2",	offsetof(struct pcb *, fs.fpregs[2]));
	DECLARE("PCB_FS_F3",	offsetof(struct pcb *, fs.fpregs[3]));
	DECLARE("PCB_FS_F4",	offsetof(struct pcb *, fs.fpregs[4]));
	DECLARE("PCB_FS_F5",	offsetof(struct pcb *, fs.fpregs[5]));
	DECLARE("PCB_FS_F6",	offsetof(struct pcb *, fs.fpregs[6]));
	DECLARE("PCB_FS_F7",	offsetof(struct pcb *, fs.fpregs[7]));
	DECLARE("PCB_FS_F8",	offsetof(struct pcb *, fs.fpregs[8]));
	DECLARE("PCB_FS_F9",	offsetof(struct pcb *, fs.fpregs[9]));
	DECLARE("PCB_FS_F10",	offsetof(struct pcb *, fs.fpregs[10]));
	DECLARE("PCB_FS_F11",	offsetof(struct pcb *, fs.fpregs[11]));
	DECLARE("PCB_FS_F12",	offsetof(struct pcb *, fs.fpregs[12]));
	DECLARE("PCB_FS_F13",	offsetof(struct pcb *, fs.fpregs[13]));
	DECLARE("PCB_FS_F14",	offsetof(struct pcb *, fs.fpregs[14]));
	DECLARE("PCB_FS_F15",	offsetof(struct pcb *, fs.fpregs[15]));
	DECLARE("PCB_FS_F16",	offsetof(struct pcb *, fs.fpregs[16]));
	DECLARE("PCB_FS_F17",	offsetof(struct pcb *, fs.fpregs[17]));
	DECLARE("PCB_FS_F18",	offsetof(struct pcb *, fs.fpregs[18]));
	DECLARE("PCB_FS_F19",	offsetof(struct pcb *, fs.fpregs[19]));
	DECLARE("PCB_FS_F20",	offsetof(struct pcb *, fs.fpregs[20]));
	DECLARE("PCB_FS_F21",	offsetof(struct pcb *, fs.fpregs[21]));
	DECLARE("PCB_FS_F22",	offsetof(struct pcb *, fs.fpregs[22]));
	DECLARE("PCB_FS_F23",	offsetof(struct pcb *, fs.fpregs[23]));
	DECLARE("PCB_FS_F24",	offsetof(struct pcb *, fs.fpregs[24]));
	DECLARE("PCB_FS_F25",	offsetof(struct pcb *, fs.fpregs[25]));
	DECLARE("PCB_FS_F26",	offsetof(struct pcb *, fs.fpregs[26]));
	DECLARE("PCB_FS_F27",	offsetof(struct pcb *, fs.fpregs[27]));
	DECLARE("PCB_FS_F28",	offsetof(struct pcb *, fs.fpregs[28]));
	DECLARE("PCB_FS_F29",	offsetof(struct pcb *, fs.fpregs[29]));
	DECLARE("PCB_FS_F30",	offsetof(struct pcb *, fs.fpregs[30]));
	DECLARE("PCB_FS_F31",	offsetof(struct pcb *, fs.fpregs[31]));
	DECLARE("PCB_FS_FPSCR",	offsetof(struct pcb *, fs.fpscr_pad));

	DECLARE("PCB_SAVED_STATE",offsetof(struct pcb *, ss));
	DECLARE("PCB_KSP",	offsetof(struct pcb *, ksp));
	DECLARE("PCB_SR0",	offsetof(struct pcb *, sr0));

	DECLARE("PCB_SIZE",	sizeof(struct pcb));

	/* Save State Structure */
	DECLARE("SS_R0",	offsetof(struct ppc_saved_state *, r0));
	DECLARE("SS_R1",	offsetof(struct ppc_saved_state *, r1));
	DECLARE("SS_R2",	offsetof(struct ppc_saved_state *, r2));
	DECLARE("SS_R3",	offsetof(struct ppc_saved_state *, r3));
	DECLARE("SS_R4",	offsetof(struct ppc_saved_state *, r4));
	DECLARE("SS_R5",	offsetof(struct ppc_saved_state *, r5));
	DECLARE("SS_R6",	offsetof(struct ppc_saved_state *, r6));
	DECLARE("SS_R7",	offsetof(struct ppc_saved_state *, r7));
	DECLARE("SS_R8",	offsetof(struct ppc_saved_state *, r8));
	DECLARE("SS_R9",	offsetof(struct ppc_saved_state *, r9));
	DECLARE("SS_R10",	offsetof(struct ppc_saved_state *, r10));
	DECLARE("SS_R11",	offsetof(struct ppc_saved_state *, r11));
	DECLARE("SS_R12",	offsetof(struct ppc_saved_state *, r12));
	DECLARE("SS_R13",	offsetof(struct ppc_saved_state *, r13));
	DECLARE("SS_R14",	offsetof(struct ppc_saved_state *, r14));
	DECLARE("SS_R15",	offsetof(struct ppc_saved_state *, r15));
	DECLARE("SS_R16",	offsetof(struct ppc_saved_state *, r16));
	DECLARE("SS_R17",	offsetof(struct ppc_saved_state *, r17));
	DECLARE("SS_R18",	offsetof(struct ppc_saved_state *, r18));
	DECLARE("SS_R19",	offsetof(struct ppc_saved_state *, r19));
	DECLARE("SS_R20",	offsetof(struct ppc_saved_state *, r20));
	DECLARE("SS_R21",	offsetof(struct ppc_saved_state *, r21));
	DECLARE("SS_R22",	offsetof(struct ppc_saved_state *, r22));
	DECLARE("SS_R23",	offsetof(struct ppc_saved_state *, r23));
	DECLARE("SS_R24",	offsetof(struct ppc_saved_state *, r24));
	DECLARE("SS_R25",	offsetof(struct ppc_saved_state *, r25));
	DECLARE("SS_R26",	offsetof(struct ppc_saved_state *, r26));
	DECLARE("SS_R27",	offsetof(struct ppc_saved_state *, r27));
	DECLARE("SS_R28",	offsetof(struct ppc_saved_state *, r28));
	DECLARE("SS_R29",	offsetof(struct ppc_saved_state *, r29));
	DECLARE("SS_R30",	offsetof(struct ppc_saved_state *, r30));
	DECLARE("SS_R31",	offsetof(struct ppc_saved_state *, r31));
	DECLARE("SS_CR",	offsetof(struct ppc_saved_state *, cr));
	DECLARE("SS_XER",	offsetof(struct ppc_saved_state *, xer));
	DECLARE("SS_LR",	offsetof(struct ppc_saved_state *, lr));
	DECLARE("SS_CTR",	offsetof(struct ppc_saved_state *, ctr));
	DECLARE("SS_SRR0",	offsetof(struct ppc_saved_state *, srr0));
	DECLARE("SS_SRR1",	offsetof(struct ppc_saved_state *, srr1));
	DECLARE("SS_MQ",	offsetof(struct ppc_saved_state *, mq));
	DECLARE("SS_SR_COPYIN",	offsetof(struct ppc_saved_state *, sr_copyin));
	DECLARE("SS_SIZE",	sizeof(struct ppc_saved_state));

	/* Per Proc info structure */
	DECLARE("PP_CPU_NUMBER",offsetof(struct per_proc_info *, cpu_number));
	DECLARE("PP_CPU_FLAGS",offsetof(struct per_proc_info *, cpu_flags));
	DECLARE("PP_ISTACKPTR",	offsetof(struct per_proc_info *, istackptr));
	DECLARE("PP_INTSTACK_TOP_SS",
		offsetof(struct per_proc_info *, intstack_top_ss));
#if	MACH_KGDB
	DECLARE("PP_GDBSTACKPTR",offsetof(struct per_proc_info *, gdbstackptr));
	DECLARE("PP_GDBSTACK_TOP_SS",
		offsetof(struct per_proc_info *, gdbstack_top_ss));
#endif	/* MACH_KGDB */
	DECLARE("PP_TEMPWORK1",
			offsetof(struct per_proc_info *, tempwork1));
	DECLARE("PP_TEMPWORK2",
			offsetof(struct per_proc_info *, tempwork2));
	DECLARE("PP_SAVE_SR0",
			offsetof(struct per_proc_info *, save_sr0));

	DECLARE("PP_SAVE_EXCEPTION_TYPE",
			offsetof(struct per_proc_info *, save_exception_type));
	DECLARE("PP_PHYS_EXCEPTION_HANDLERS",
		offsetof(struct per_proc_info *, phys_exception_handlers));
	DECLARE("PP_VIRT_PER_PROC",
		offsetof(struct per_proc_info *, virt_per_proc_info));
	DECLARE("PP_CPU_DATA",
		offsetof(struct per_proc_info *, cpu_data));
	DECLARE("PP_ACTIVE_KLOADED",
		offsetof(struct per_proc_info *, active_kloaded));
	DECLARE("PP_ACTIVE_STACKS",
		offsetof(struct per_proc_info *, active_stacks));
	DECLARE("PP_NEED_AST",
		offsetof(struct per_proc_info *, need_ast));
	DECLARE("PP_FPU_PCB",
		offsetof(struct per_proc_info *, fpu_pcb));

	DECLARE("PP_SAVE_CTR",	offsetof(struct per_proc_info *, save_ctr));
	DECLARE("PP_SAVE_CR",	offsetof(struct per_proc_info *, save_cr));
	DECLARE("PP_SAVE_SRR0",	offsetof(struct per_proc_info *, save_srr0));
	DECLARE("PP_SAVE_SRR1",	offsetof(struct per_proc_info *, save_srr1));
	DECLARE("PP_SAVE_DAR",	offsetof(struct per_proc_info *, save_dar));
	DECLARE("PP_SAVE_DSISR",offsetof(struct per_proc_info *, save_dsisr));
	DECLARE("PP_SAVE_LR",	offsetof(struct per_proc_info *, save_lr));
	DECLARE("PP_SAVE_CTR",	offsetof(struct per_proc_info *, save_ctr));
	DECLARE("PP_SAVE_XER",	offsetof(struct per_proc_info *, save_xer));

	DECLARE("PP_SAVE_R0",	offsetof(struct per_proc_info *, save_r0));
	DECLARE("PP_SAVE_R1",	offsetof(struct per_proc_info *, save_r1));
	DECLARE("PP_SAVE_R2",	offsetof(struct per_proc_info *, save_r2));
	DECLARE("PP_SAVE_R3",	offsetof(struct per_proc_info *, save_r3));
	DECLARE("PP_SAVE_R4",	offsetof(struct per_proc_info *, save_r4));
	DECLARE("PP_SAVE_R5",	offsetof(struct per_proc_info *, save_r5));
	DECLARE("PP_SAVE_R6",	offsetof(struct per_proc_info *, save_r6));
	DECLARE("PP_SAVE_R7",	offsetof(struct per_proc_info *, save_r7));
	DECLARE("PP_SAVE_R8",	offsetof(struct per_proc_info *, save_r8));
	DECLARE("PP_SAVE_R9",	offsetof(struct per_proc_info *, save_r9));
	DECLARE("PP_SAVE_R10",	offsetof(struct per_proc_info *, save_r10));
	DECLARE("PP_SAVE_R11",	offsetof(struct per_proc_info *, save_r11));
	DECLARE("PP_SAVE_R12",	offsetof(struct per_proc_info *, save_r12));
	DECLARE("PP_SAVE_R13",	offsetof(struct per_proc_info *, save_r13));
	DECLARE("PP_SAVE_R14",	offsetof(struct per_proc_info *, save_r14));
	DECLARE("PP_SAVE_R15",	offsetof(struct per_proc_info *, save_r15));
	DECLARE("PP_SAVE_R16",	offsetof(struct per_proc_info *, save_r16));
	DECLARE("PP_SAVE_R17",	offsetof(struct per_proc_info *, save_r17));
	DECLARE("PP_SAVE_R18",	offsetof(struct per_proc_info *, save_r18));
	DECLARE("PP_SAVE_R19",	offsetof(struct per_proc_info *, save_r19));
	DECLARE("PP_SAVE_R20",	offsetof(struct per_proc_info *, save_r20));
	DECLARE("PP_SAVE_R21",	offsetof(struct per_proc_info *, save_r21));
	DECLARE("PP_SAVE_R22",	offsetof(struct per_proc_info *, save_r22));
	DECLARE("PP_SAVE_R23",	offsetof(struct per_proc_info *, save_r23));
	DECLARE("PP_SAVE_R24",	offsetof(struct per_proc_info *, save_r24));
	DECLARE("PP_SAVE_R25",	offsetof(struct per_proc_info *, save_r25));
	DECLARE("PP_SAVE_R26",	offsetof(struct per_proc_info *, save_r26));
	DECLARE("PP_SAVE_R27",	offsetof(struct per_proc_info *, save_r27));
	DECLARE("PP_SAVE_R28",	offsetof(struct per_proc_info *, save_r28));
	DECLARE("PP_SAVE_R29",	offsetof(struct per_proc_info *, save_r29));
	DECLARE("PP_SAVE_R30",	offsetof(struct per_proc_info *, save_r30));
	DECLARE("PP_SAVE_R31",	offsetof(struct per_proc_info *, save_r31));

	/* Kernel State Structure - special case as we want offset from
	 * bottom of kernel stack, not offset into structure
	 */
#define IKSBASE (u_int)STACK_IKS(0)

	DECLARE("KS_PCB", IKSBASE + offsetof(struct ppc_kernel_state *, pcb));
	DECLARE("KS_R1",  IKSBASE + offsetof(struct ppc_kernel_state *, r1));
	DECLARE("KS_R13", IKSBASE + offsetof(struct ppc_kernel_state *, r13));
	DECLARE("KS_LR",  IKSBASE + offsetof(struct ppc_kernel_state *, lr));
	DECLARE("KS_CR",  IKSBASE + offsetof(struct ppc_kernel_state *, cr));
	DECLARE("KS_SIZE", sizeof(struct ppc_kernel_state));

	/* values from kern/thread.h */
	DECLARE("THREAD_TOP_ACT",
		offsetof(struct thread_shuttle *, top_act));
	DECLARE("THREAD_KERNEL_STACK",
		offsetof(struct thread_shuttle *, kernel_stack));
	DECLARE("THREAD_CONTINUATION",
		offsetof(struct thread_shuttle *, continuation));
	DECLARE("THREAD_RECOVER",
		offsetof(struct thread_shuttle *, recover));
#if	MACH_LDEBUG
	DECLARE("THREAD_MUTEX_COUNT",
		offsetof(struct thread_shuttle *, mutex_count));
#endif	/* MACH_LDEBUG */

	/* values from kern/thread_act.h */
	DECLARE("ACT_TASK",    offsetof(struct thread_activation *, task));
	DECLARE("ACT_MACT_PCB",offsetof(struct thread_activation *, mact.pcb));
	DECLARE("ACT_AST",     offsetof(struct thread_activation *, ast));
	DECLARE("ACT_VMMAP",   offsetof(struct thread_activation *, map));
	DECLARE("ACT_KLOADED",
		offsetof(struct thread_activation *, kernel_loaded));
	DECLARE("ACT_KLOADING",
		offsetof(struct thread_activation *, kernel_loading));
	DECLARE("ACT_MACH_EXC_PORT",
		offsetof(struct thread_activation *,
		exc_actions[EXC_MACH_SYSCALL].port));

	/* values from kern/task.h */
	DECLARE("TASK_MACH_EXC_PORT",
		offsetof(struct task *, exc_actions[EXC_MACH_SYSCALL].port));

	/* values from vm/vm_map.h */
	DECLARE("VMMAP_PMAP",	offsetof(struct vm_map *, pmap));

	/* values from machine/pmap.h */
	DECLARE("PMAP_SPACE",	offsetof(struct pmap *, space));

	/* Constants from pmap.h */
	DECLARE("PPC_SID_KERNEL", PPC_SID_KERNEL);

	/* values for accessing mach_trap table */
	DECLARE("MACH_TRAP_OFFSET_POW2",	4);

	DECLARE("MACH_TRAP_ARGC",
		offsetof(mach_trap_t *, mach_trap_arg_count));
	DECLARE("MACH_TRAP_FUNCTION",
		offsetof(mach_trap_t *, mach_trap_function));

	DECLARE("HOST_SELF", offsetof(host_t, host_self));

	/* values from cpu_data.h */
	DECLARE("CPU_ACTIVE_THREAD", offsetof(cpu_data_t *, active_thread));
	DECLARE("CPU_SIMPLE_LOCK_COUNT",
		offsetof(cpu_data_t *, simple_lock_count));
	DECLARE("CPU_INTERRUPT_LEVEL",offsetof(cpu_data_t *, interrupt_level));

	/* Misc values used by assembler */
	DECLARE("AST_ALL", AST_ALL);

	/* Simple Lock structure */
	DECLARE("SLOCK_ILK",	offsetof(simple_lock_t, interlock));
#if	MACH_LDEBUG
	DECLARE("SLOCK_TYPE",	offsetof(simple_lock_t, lock_type));
	DECLARE("SLOCK_PC",	offsetof(simple_lock_t, debug.lock_pc));
	DECLARE("SLOCK_THREAD",	offsetof(simple_lock_t, debug.lock_thread));
	DECLARE("SLOCK_DURATIONH",offsetof(simple_lock_t, debug.duration[0]));
	DECLARE("SLOCK_DURATIONL",offsetof(simple_lock_t, debug.duration[1]));
	DECLARE("USLOCK_TAG",	USLOCK_TAG);
#endif	/* MACH_LDEBUG */

	/* Mutex structure */
	DECLARE("MUTEX_ILK",	offsetof(mutex_t *, interlock));
	DECLARE("MUTEX_LOCKED",	offsetof(mutex_t *, locked));
	DECLARE("MUTEX_WAITERS",offsetof(mutex_t *, waiters));
#if	MACH_LDEBUG
	DECLARE("MUTEX_TYPE",	offsetof(mutex_t *, type));
	DECLARE("MUTEX_PC",	offsetof(mutex_t *, pc));
	DECLARE("MUTEX_THREAD",	offsetof(mutex_t *, thread));
	DECLARE("MUTEX_TAG",	MUTEX_TAG);
#endif	/* MACH_LDEBUG */

#if	NCPUS > 1
	/* values from mp/PlugIn.h */
	
	DECLARE("MPSversionID",	offsetof(struct MPPlugInSpec *, versionID));
	DECLARE("MPSareaAddr",	offsetof(struct MPPlugInSpec *, areaAddr));
	DECLARE("MPSareaSize",	offsetof(struct MPPlugInSpec *, areaSize));
	DECLARE("MPSoffsetTableAddr", offsetof(struct MPPlugInSpec *, offsetTableAddr));
	DECLARE("MPSbaseAddr",	offsetof(struct MPPlugInSpec *, baseAddr));
	DECLARE("MPSdataArea",	offsetof(struct MPPlugInSpec *, dataArea));
	DECLARE("MPSCPUArea",	offsetof(struct MPPlugInSpec *, CPUArea));
	DECLARE("MPSSIGPhandler",	offsetof(struct MPPlugInSpec *, SIGPhandler));

	DECLARE("CSAstate",	offsetof(struct CPUStatusArea *, state));
	DECLARE("CSAregsAreValid", offsetof(struct CPUStatusArea *,
					    regsAreValid));
	DECLARE("CSAgpr",	offsetof(struct CPUStatusArea *, gpr));
	DECLARE("CSAfpr",	offsetof(struct CPUStatusArea *, fpr));
	DECLARE("CSAcr",	offsetof(struct CPUStatusArea *, cr));
	DECLARE("CSAfpscr",	offsetof(struct CPUStatusArea *, fpscr));
	DECLARE("CSAxer",	offsetof(struct CPUStatusArea *, xer));
	DECLARE("CSAlr",	offsetof(struct CPUStatusArea *, lr));
	DECLARE("CSActr",	offsetof(struct CPUStatusArea *, ctr));
	DECLARE("CSAtbu",	offsetof(struct CPUStatusArea *, tbu));
	DECLARE("CSAtbl",	offsetof(struct CPUStatusArea *, tbl));
	DECLARE("CSApvr",	offsetof(struct CPUStatusArea *, pvr));
	DECLARE("CSAibat",	offsetof(struct CPUStatusArea *, ibat));
	DECLARE("CSAdbat",	offsetof(struct CPUStatusArea *, dbat));
	DECLARE("CSAsdr1",	offsetof(struct CPUStatusArea *, sdr1));
	DECLARE("CSAsr",	offsetof(struct CPUStatusArea *, sr));
	DECLARE("CSAdar",	offsetof(struct CPUStatusArea *, dar));
	DECLARE("CSAdsisr",	offsetof(struct CPUStatusArea *, dsisr));
	DECLARE("CSAsprg",	offsetof(struct CPUStatusArea *, sprg));
	DECLARE("CSAsrr0",	offsetof(struct CPUStatusArea *, srr0));
	DECLARE("CSAsrr1",	offsetof(struct CPUStatusArea *, srr1));
	DECLARE("CSAdec",	offsetof(struct CPUStatusArea *, dec));
	DECLARE("CSAdabr",	offsetof(struct CPUStatusArea *, dabr));
	DECLARE("CSAiabr",	offsetof(struct CPUStatusArea *, iabr));
	DECLARE("CSAear",	offsetof(struct CPUStatusArea *, ear));
	DECLARE("CSAhid",	offsetof(struct CPUStatusArea *, hid));
	DECLARE("CSAmmcr",	offsetof(struct CPUStatusArea *, mmcr));
	DECLARE("CSApmc",	offsetof(struct CPUStatusArea *, pmc));
	DECLARE("CSApir",	offsetof(struct CPUStatusArea *, pir));
	DECLARE("CSAsda",	offsetof(struct CPUStatusArea *, sda));
	DECLARE("CSAsia",	offsetof(struct CPUStatusArea *, sia));
	DECLARE("CSAmq",	offsetof(struct CPUStatusArea *, mq));
	DECLARE("CSAmsr",	offsetof(struct CPUStatusArea *, msr));
	DECLARE("CSApc",	offsetof(struct CPUStatusArea *, pc));
	DECLARE("CSAsysregs",	offsetof(struct CPUStatusArea *, sysregs));
	DECLARE("CSAsize",	sizeof(struct CPUStatusArea));


	DECLARE("MPPICStat",	offsetof(struct MPPInterface *, MPPICStat));
	DECLARE("MPPICParm0",	offsetof(struct MPPInterface *, MPPICParm0));
	DECLARE("MPPICParm1",	offsetof(struct MPPInterface *, MPPICParm1));
	DECLARE("MPPICParm2",	offsetof(struct MPPInterface *, MPPICParm2));
	DECLARE("MPPICspare0",	offsetof(struct MPPInterface *, MPPICspare0));
	DECLARE("MPPICspare1",	offsetof(struct MPPInterface *, MPPICspare1));
	DECLARE("MPPICParm0BU",	offsetof(struct MPPInterface *, MPPICParm0BU));
	DECLARE("MPPICPriv",	offsetof(struct MPPInterface *, MPPICPriv));



#endif	/* NCPUS > 1 */
						 
	/* values from low_trace.h */
	DECLARE("LTR_cpu",	offsetof(struct LowTraceRecord *, LTR_cpu));
	DECLARE("LTR_excpt",	offsetof(struct LowTraceRecord *, LTR_excpt));
	DECLARE("LTR_timeHi",	offsetof(struct LowTraceRecord *, LTR_timeHi));
	DECLARE("LTR_timeLo",	offsetof(struct LowTraceRecord *, LTR_timeLo));
	DECLARE("LTR_cr",	offsetof(struct LowTraceRecord *, LTR_cr));
	DECLARE("LTR_srr0",	offsetof(struct LowTraceRecord *, LTR_srr0));
	DECLARE("LTR_srr1",	offsetof(struct LowTraceRecord *, LTR_srr1));
	DECLARE("LTR_dar",	offsetof(struct LowTraceRecord *, LTR_dar));
	DECLARE("LTR_dsisr",	offsetof(struct LowTraceRecord *, LTR_dsisr));
	DECLARE("LTR_lr",	offsetof(struct LowTraceRecord *, LTR_lr));
	DECLARE("LTR_ctr",	offsetof(struct LowTraceRecord *, LTR_ctr));
	DECLARE("LTR_r0",	offsetof(struct LowTraceRecord *, LTR_r0));
	DECLARE("LTR_r1",	offsetof(struct LowTraceRecord *, LTR_r1));
	DECLARE("LTR_r2",	offsetof(struct LowTraceRecord *, LTR_r2));
	DECLARE("LTR_r3",	offsetof(struct LowTraceRecord *, LTR_r3));
	DECLARE("LTR_r4",	offsetof(struct LowTraceRecord *, LTR_r4));
	DECLARE("LTR_r5",	offsetof(struct LowTraceRecord *, LTR_r5));
	DECLARE("LTR_size",	sizeof(struct LowTraceRecord));

	return 0;
}
