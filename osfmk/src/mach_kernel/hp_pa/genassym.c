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
 * Copyright (c) 1990,1991 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: genassym.c 1.5 92/07/08$
 *	Author: Bob Wheeler, University of Utah CSS
 */

/*
 *	Generate assembly symbols
 */

#include <kern/thread.h>

#include <machine/rpb.h>
#include <machine/pdc.h>
#include <machine/iomod.h>
#include <hp_pa/pte.h>
#include <machine/pmap.h>
#include <kern/cpu_data.h>

int
main(int argc, char *argv[])
{
	struct pcb *pcbp = (struct pcb *)0;
	struct hp700_saved_state *ssp = (struct hp700_saved_state *)0;
	struct hp700_float_state *sfp = (struct hp700_float_state *)0;
	struct hp700_kernel_state *ksp = (struct hp700_kernel_state *)0;
	struct hp700_trap_state *tsp = (struct hp700_trap_state *)0;
	struct mapping *map = (struct mapping *)0;
	struct phys_entry *ptov = (struct phys_entry *)0;
	struct vtop_entry *vtop = (struct vtop_entry *)0;
	struct thread_shuttle *thread = (struct thread_shuttle *)0;
	struct thread_activation *act = (struct thread_activation *)0;
	struct vm_map *vm_map = (struct vm_map *)0;
	struct pmap * pmap = (struct pmap *)0;
	struct task *task = (struct task *)0;
	struct rpb *rp = (struct rpb *)0;
	struct pagezero *pzp = (struct pagezero *)0;
	struct iomod *hpa = (struct iomod *)0;
	mutex_t *mut = (mutex_t *)0;
#ifdef	HPT
	struct hpt_entry *hpt = (struct hpt_entry *)0;
#endif
	cpu_data_t *cpudat = (cpu_data_t *)0;

	printf("\n/*\n * Process Control Block\n */\n");
	printf("#define\tPCB_SS\t0x%x\n", &pcbp->ss);
	printf("#define\tPCB_SF\t0x%x\n", &pcbp->sf);
	printf("#define\tPCB_KSP\t0x%x\n", &pcbp->ksp);
	printf("#define\tPCB_SIZE\t%d\n", sizeof (struct pcb));

	printf("\n/*\n * Trap State Structure\n */\n");
	printf("#define\tTRAP_R21\t0x%x\n", &tsp->t2);
	printf("#define\tTRAP_R22\t0x%x\n", &tsp->t1);
	printf("#define\tTRAP_R26\t0x%x\n", &tsp->arg0);
	printf("#define\tTRAP_R30\t0x%x\n", &tsp->sp);
	printf("#define\tTRAP_IIOQH\t0x%x\n", &tsp->iioqh);
	printf("#define\tTRAP_IIOQT\t0x%x\n", &tsp->iioqt);
	printf("#define\tTRAP_IISQH\t0x%x\n", &tsp->iisqh);
	printf("#define\tTRAP_IISQT\t0x%x\n", &tsp->iisqt);
	printf("#define\tTRAP_CR15\t0x%x\n", &tsp->eiem);
	printf("#define\tTRAP_CR19\t0x%x\n", &tsp->iir);
	printf("#define\tTRAP_CR20\t0x%x\n", &tsp->isr);
	printf("#define\tTRAP_CR21\t0x%x\n", &tsp->ior);
	printf("#define\tTRAP_CR22\t0x%x\n", &tsp->ipsw);
	printf("#define\tTRAP_SIZE\t0x%x\n", sizeof (*tsp));

	printf("\n/*\n * Kernel State Structure\n */\n");
	printf("#define\tKS_R2\t0x%x\n", &ksp->rp);
	printf("#define\tKS_R3\t0x%x\n", &ksp->r3);
	printf("#define\tKS_R4\t0x%x\n", &ksp->r4);
	printf("#define\tKS_R5\t0x%x\n", &ksp->r5);
	printf("#define\tKS_R6\t0x%x\n", &ksp->r6);
	printf("#define\tKS_R7\t0x%x\n", &ksp->r7);
	printf("#define\tKS_R8\t0x%x\n", &ksp->r8);
	printf("#define\tKS_R9\t0x%x\n", &ksp->r9);
	printf("#define\tKS_R10\t0x%x\n", &ksp->r10);
	printf("#define\tKS_R11\t0x%x\n", &ksp->r11);
	printf("#define\tKS_R12\t0x%x\n", &ksp->r12);
	printf("#define\tKS_R13\t0x%x\n", &ksp->r13);
	printf("#define\tKS_R14\t0x%x\n", &ksp->r14);
	printf("#define\tKS_R15\t0x%x\n", &ksp->r15);
	printf("#define\tKS_R16\t0x%x\n", &ksp->r16);
	printf("#define\tKS_R17\t0x%x\n", &ksp->r17);
	printf("#define\tKS_R18\t0x%x\n", &ksp->r18);
	printf("#define\tKS_R30\t0x%x\n", &ksp->sp);
	printf("#define\tKS_SIZE\t0x%x\n", sizeof(struct hp700_kernel_state));

	printf("\n/*\n * Save State Structure\n */\n");
	printf("#define\tSS_FLAGS\t0x%x\n", &ssp->flags);
	printf("#define\tSS_R1\t0x%x\n", &ssp->r1);
	printf("#define\tSS_R2\t0x%x\n", &ssp->rp);
	printf("#define\tSS_R3\t0x%x\n", &ssp->r3);
	printf("#define\tSS_R4\t0x%x\n", &ssp->r4);
	printf("#define\tSS_R5\t0x%x\n", &ssp->r5);
	printf("#define\tSS_R6\t0x%x\n", &ssp->r6);
	printf("#define\tSS_R7\t0x%x\n", &ssp->r7);
	printf("#define\tSS_R8\t0x%x\n", &ssp->r8);
	printf("#define\tSS_R9\t0x%x\n", &ssp->r9);
	printf("#define\tSS_R10\t0x%x\n", &ssp->r10);
	printf("#define\tSS_R11\t0x%x\n", &ssp->r11);
	printf("#define\tSS_R12\t0x%x\n", &ssp->r12);
	printf("#define\tSS_R13\t0x%x\n", &ssp->r13);
	printf("#define\tSS_R14\t0x%x\n", &ssp->r14);
	printf("#define\tSS_R15\t0x%x\n", &ssp->r15);
	printf("#define\tSS_R16\t0x%x\n", &ssp->r16);
	printf("#define\tSS_R17\t0x%x\n", &ssp->r17);
	printf("#define\tSS_R18\t0x%x\n", &ssp->r18);
	printf("#define\tSS_R19\t0x%x\n", &ssp->t4);
	printf("#define\tSS_R20\t0x%x\n", &ssp->t3);
	printf("#define\tSS_R21\t0x%x\n", &ssp->t2);
	printf("#define\tSS_R22\t0x%x\n", &ssp->t1);
	printf("#define\tSS_R23\t0x%x\n", &ssp->arg3);
	printf("#define\tSS_R24\t0x%x\n", &ssp->arg2);
	printf("#define\tSS_R25\t0x%x\n", &ssp->arg1);
	printf("#define\tSS_R26\t0x%x\n", &ssp->arg0);
	printf("#define\tSS_R27\t0x%x\n", &ssp->dp);
	printf("#define\tSS_R28\t0x%x\n", &ssp->ret0);
	printf("#define\tSS_R29\t0x%x\n", &ssp->ret1);
	printf("#define\tSS_R30\t0x%x\n", &ssp->sp);
	printf("#define\tSS_R31\t0x%x\n", &ssp->r31);
	printf("#define\tSS_SR0\t0x%x\n", &ssp->sr0);
	printf("#define\tSS_SR1\t0x%x\n", &ssp->sr1);
	printf("#define\tSS_SR2\t0x%x\n", &ssp->sr2);
	printf("#define\tSS_SR3\t0x%x\n", &ssp->sr3);
	printf("#define\tSS_SR4\t0x%x\n", &ssp->sr4);
	printf("#define\tSS_SR5\t0x%x\n", &ssp->sr5);
	printf("#define\tSS_SR6\t0x%x\n", &ssp->sr6);
	printf("#define\tSS_SR7\t0x%x\n", &ssp->sr7);
	printf("#define\tSS_CR0\t0x%x\n", &ssp->rctr);
	printf("#define\tSS_CR8\t0x%x\n", &ssp->pidr1);
	printf("#define\tSS_CR9\t0x%x\n", &ssp->pidr2);
	printf("#define\tSS_CR11\t0x%x\n", &ssp->sar);
	printf("#define\tSS_CR12\t0x%x\n", &ssp->pidr3);
	printf("#define\tSS_CR13\t0x%x\n", &ssp->pidr4);
	printf("#define\tSS_CR15\t0x%x\n", &ssp->eiem);
	printf("#define\tSS_IISQH\t0x%x\n", &ssp->iisq_head);
	printf("#define\tSS_IISQT\t0x%x\n", &ssp->iisq_tail);
	printf("#define\tSS_IIOQH\t0x%x\n", &ssp->iioq_head);
	printf("#define\tSS_IIOQT\t0x%x\n", &ssp->iioq_tail);
	printf("#define\tSS_CR19\t0x%x\n", &ssp->iir);
	printf("#define\tSS_CR20\t0x%x\n", &ssp->isr);
	printf("#define\tSS_CR21\t0x%x\n", &ssp->ior);
	printf("#define\tSS_CR22\t0x%x\n", &ssp->ipsw);
	printf("#define\tSS_FPU\t0x%x\n", &ssp->fpu);
	printf("#define\tSS_SIZE\t0x%x\n", sizeof(struct hp700_saved_state));

	printf("\n/*\n * Float State Structure\n */\n");
	printf("#define\tSF_FR0\t0x%x\n", &sfp->fr0);
	printf("#define\tSF_FR1\t0x%x\n", &sfp->fr1);
	printf("#define\tSF_FR2\t0x%x\n", &sfp->fr2);
	printf("#define\tSF_FR3\t0x%x\n", &sfp->fr3);
	printf("#define\tSF_FR4\t0x%x\n", &sfp->farg0);
	printf("#define\tSF_FR5\t0x%x\n", &sfp->farg1);
	printf("#define\tSF_FR6\t0x%x\n", &sfp->farg2);
	printf("#define\tSF_FR7\t0x%x\n", &sfp->farg3);
	printf("#define\tSF_FR8\t0x%x\n", &sfp->fr8);
	printf("#define\tSF_FR9\t0x%x\n", &sfp->fr9);
	printf("#define\tSF_FR10\t0x%x\n", &sfp->fr10);
	printf("#define\tSF_FR11\t0x%x\n", &sfp->fr11);
	printf("#define\tSF_FR12\t0x%x\n", &sfp->fr12);
	printf("#define\tSF_FR13\t0x%x\n", &sfp->fr13);
	printf("#define\tSF_FR14\t0x%x\n", &sfp->fr14);
	printf("#define\tSF_FR15\t0x%x\n", &sfp->fr15);
	printf("#define\tSF_FR16\t0x%x\n", &sfp->fr16);
	printf("#define\tSF_FR17\t0x%x\n", &sfp->fr17);
	printf("#define\tSF_FR18\t0x%x\n", &sfp->fr18);
	printf("#define\tSF_FR19\t0x%x\n", &sfp->fr19);
	printf("#define\tSF_FR20\t0x%x\n", &sfp->fr20);
	printf("#define\tSF_FR21\t0x%x\n", &sfp->fr21);
	printf("#define\tSF_FR22\t0x%x\n", &sfp->fr22);
	printf("#define\tSF_FR23\t0x%x\n", &sfp->fr23);
	printf("#define\tSF_FR24\t0x%x\n", &sfp->fr24);
	printf("#define\tSF_FR25\t0x%x\n", &sfp->fr25);
	printf("#define\tSF_FR26\t0x%x\n", &sfp->fr26);
	printf("#define\tSF_FR27\t0x%x\n", &sfp->fr27);
	printf("#define\tSF_FR28\t0x%x\n", &sfp->fr28);
	printf("#define\tSF_FR29\t0x%x\n", &sfp->fr29);
	printf("#define\tSF_FR30\t0x%x\n", &sfp->fr30);
	printf("#define\tSF_FR31\t0x%x\n", &sfp->fr31);

	printf("\n/*\n * Mapping structure parameters\n */\n");
	printf("#define\tHP700_MAP_HLINKN\t0x%x\n", &map->hash_link.next);
	printf("#define\tHP700_MAP_HLINKP\t0x%x\n", &map->hash_link.prev);
	printf("#define\tHP700_MAP_PLINKN\t0x%x\n", &map->phys_link.next);
	printf("#define\tHP700_MAP_PLINKP\t0x%x\n", &map->phys_link.prev);
	printf("#define\tHP700_MAP_SPACE\t0x%x\n", &map->space);
	printf("#define\tHP700_MAP_OFFSET\t0x%x\n", &map->offset);
	printf("#define\tHP700_MAP_PAGE\t0x%x\n", &map->tlbpage);
	printf("#define\tHP700_MAP_PROT\t0x%x\n", &map->tlbprot);
	printf("#define\tHP700_MAP_SW\t0x%x\n", &map->tlbsw);

	printf("\n/*\n * physical to virtual mapping parameters\n */\n");
	printf("#define\tPTOV_LINKN\t0x%x\n", &ptov->phys_link.next);
	printf("#define\tPTOV_LINKP\t0x%x\n", &ptov->phys_link.prev);
	printf("#define\tPTOV_WRITER\t0x%x\n", &ptov->writer);
	printf("#define\tPTOV_TLBPROT\t0x%x\n", &ptov->tlbprot);

	printf("#define\tVTOP_LINKN\t0x%x\n", &vtop->hash_link.next);
	printf("#define\tVTOP_LINKP\t0x%x\n", &vtop->hash_link.prev);

#ifdef HPT
	printf("\n/*\n * Hashed Page Table parameters\n */\n");
	printf("#define\tHPT_TAG\t0x%x\n", 0);
	printf("#define\tHPT_VTOP\t0x%x\n", &hpt->vtop_entry);
	printf("#define\tHPT_PROT\t0x%x\n", &hpt->tlbprot);
	printf("#define\tHPT_PAGE\t0x%x\n", &hpt->tlbpage);
#endif

	printf("\n/*\n * values from kern/thread.h\n */\n");
        printf("#define\tTHREAD_TOP_ACT\t0x%x\n", &thread->top_act);
	printf("#define\tTHREAD_CONTINUATION\t0x%x\n", &thread->continuation);
        printf("#define\tTHREAD_RECOVER\t0x%x\n", &thread->recover);
	printf("#define\tTHREAD_KERNEL_STACK\t0x%x\n", &thread->kernel_stack);

	printf("\n/*\n * values from kern/thread_act.h\n */\n");
	printf("#define\tACT_TASK\t0x%x\n", &act->task);
        printf("#define\tACT_PCB\t0x%x\n", &act->mact.pcb);
        printf("#define\tACT_AST\t0x%x\n", &act->ast);
        printf("#define\tACT_LOWER\t0x%x\n", &act->lower);
        printf("#define\tACT_MAP\t0x%x\n", &act->map);
        printf("#define\tACT_KLOADED\t0x%x\n", &act->kernel_loaded);
        printf("#define\tACT_KLOADING\t0x%x\n", &act->kernel_loading);

	printf("\n/*\n * values from kern/task.h\n */\n");
	printf("#define\tTASK_MAP\t0x%0x\n", &task->map);
        printf("#define\tTASK_EML_DISPATCH\t0x%x\n", &task->eml_dispatch);

	printf("\n/*\n * values from vm/vm_map.h\n */\n");
	printf("#define\tVMMAP_PMAP\t0x%08x\n", &vm_map->pmap);
	printf("#define\tVMMAP_COUNT\t0x%08x\n", &vm_map->ref_count);

	printf("\n/*\n * values from machine/pmap.h\n */\n");
	printf("#define\tPMAP_SPACE\t0x%08x\n", &pmap->space);
	printf("#define\tPMAP_PID\t0x%08x\n", &pmap->pid);
	
	/* Restart Parameter Block, Processor Internal Memory */
	printf("#define\tRP_SP %d\n", &rp->rp_sp);
	printf("#define\tRP_FLAG %d\n", &rp->rp_flag);
	printf("#define\tRP_PIM %d\n", &rp->rp_pim);
	printf("#define\tRP_PIM_SP %d\n", &rp->rp_pim.gr[30]);
	printf("#define\tRP_PIM_R4 %d\n", &rp->rp_pim.gr[4]);
	printf("#define\tRPBSTRUCTSIZE %d\n", sizeof(struct rpb));
	printf("#define\tPIMSTRUCTSIZE %d\n", sizeof(struct pim));
	printf("#define\tMEM_TOC %d\n", &pzp->ivec_toc);
	printf("#define\tMEM_TOC_LEN %d\n", &pzp->ivec_toclen);
	printf("#define\tHPA_IOCMD %d\n", &hpa->io_command);

	printf("#define\tM_INTERLOCK %d\n", &mut->interlock);
	printf("#define\tM_LOCKED %d\n", &mut->locked);
	printf("#define\tM_WAITERS %d\n", &mut->waiters);
#if MACH_LDEBUG
	printf("#define\tM_TYPE %d\n", &mut->type);
	printf("#define\tM_PC %d\n", &mut->pc);
	printf("#define\tM_THREAD %d\n", &mut->thread);
#endif	

	printf("\n/*\n * cpu datas\n */\n");
	printf("#define\tCPU_ACTIVE_THREAD\t0x%x\n", &cpudat->active_thread);
	printf("#define\tCPU_SIMPLE_LOCK_CNT\t0x%x\n", &cpudat->simple_lock_count);
#if	MACH_RT
	printf("#define\tCPU_PREEMPTION_LEV\t0x%x\n", &cpudat->preemption_level);
#endif

	return 0;
}
