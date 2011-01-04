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
 * Revision 2.13.7.2  92/04/30  11:51:13  bernadat
 * 	Adaptations for Corollary and Systempro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.13.7.1  92/02/18  18:49:50  jeffreyh
 * 	Support for the Corollary MP
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.13  91/07/31  17:40:55  dbg
 * 	Add pointers to interrupt stack (for uniprocessor).
 * 	[91/07/30  16:57:13  dbg]
 * 
 * Revision 2.12  91/06/19  11:55:39  rvb
 * 	cputypes.h->platforms.h
 * 	[91/06/12  13:45:27  rvb]
 * 
 * Revision 2.11  91/05/14  16:16:59  mrt
 * 	Correcting copyright
 * 
 * Revision 2.10  91/05/08  12:42:43  dbg
 * 	Put parentheses around substituted immediate expressions, so
 * 	that they will pass through the GNU preprocessor.
 * 
 * 	Moved model-specific code to machine-dependent directories.
 * 	Added startup code for multiple CPUs.
 * 	[91/04/26  14:38:55  dbg]
 * 
 * Revision 2.9  91/02/05  17:14:50  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:38:15  mrt]
 * 
 * Revision 2.8  90/12/20  16:37:02  jeffreyh
 * 	Changes for __STDC__
 * 	[90/12/07  15:43:21  jeffreyh]
 * 
 *
 * Revision 2.7  90/12/04  14:46:38  jsb
 * 	iPSC2 -> iPSC386; ipsc2_foo -> ipsc_foo;
 * 	changes for merged intel/pmap.{c,h}.
 * 	[90/12/04  11:20:35  jsb]
 * 
 * Revision 2.6  90/11/24  15:14:56  jsb
 * 	Added AT386 conditional around "BIOS/DOS hack".
 * 	[90/11/24  11:44:47  jsb]
 * 
 * Revision 2.5  90/11/05  14:27:51  rpd
 * 	Since we steal pages after esym for page tables, use first_avail
 * 	to record the last page +1 that we stole.
 * 	Tell bios to warm boot on reboot.
 * 	[90/09/05            rvb]
 * 
 * Revision 2.4  90/09/23  17:45:20  jsb
 * 	Added support for iPSC386.
 * 	[90/09/21  16:42:34  jsb]
 * 
 * Revision 2.3  90/08/27  21:58:29  dbg
 * 	Change fix_desc to match new fake_descriptor format.
 * 	[90/07/25            dbg]
 * 
 * Revision 2.2  90/05/03  15:37:40  dbg
 * 	Created.
 * 	[90/02/14            dbg]
 * 
 */
/*
 * Revision 2.13.7.2  92/04/30  11:51:13  bernadat
 * 	Adaptations for Corollary and Systempro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.13.7.1  92/02/18  18:49:50  jeffreyh
 * 	Support for the Corollary MP
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.13  91/07/31  17:40:55  dbg
 * 	Add pointers to interrupt stack (for uniprocessor).
 * 	[91/07/30  16:57:13  dbg]
 * 
 * Revision 2.12  91/06/19  11:55:39  rvb
 * 	cputypes.h->platforms.h
 * 	[91/06/12  13:45:27  rvb]
 * 
 * Revision 2.11  91/05/14  16:16:59  mrt
 * 	Correcting copyright
 * 
 * Revision 2.10  91/05/08  12:42:43  dbg
 * 	Put parentheses around substituted immediate expressions, so
 * 	that they will pass through the GNU preprocessor.
 * 
 * 	Moved model-specific code to machine-dependent directories.
 * 	Added startup code for multiple CPUs.
 * 	[91/04/26  14:38:55  dbg]
 * 
 * Revision 2.9  91/02/05  17:14:50  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:38:15  mrt]
 * 
 * Revision 2.8  90/12/20  16:37:02  jeffreyh
 * 	Changes for __STDC__
 * 	[90/12/07  15:43:21  jeffreyh]
 * 
 *
 * Revision 2.7  90/12/04  14:46:38  jsb
 * 	iPSC2 -> iPSC386; ipsc2_foo -> ipsc_foo;
 * 	changes for merged intel/pmap.{c,h}.
 * 	[90/12/04  11:20:35  jsb]
 * 
 * Revision 2.6  90/11/24  15:14:56  jsb
 * 	Added AT386 conditional around "BIOS/DOS hack".
 * 	[90/11/24  11:44:47  jsb]
 * 
 * Revision 2.5  90/11/05  14:27:51  rpd
 * 	Since we steal pages after esym for page tables, use first_avail
 * 	to record the last page +1 that we stole.
 * 	Tell bios to warm boot on reboot.
 * 	[90/09/05            rvb]
 * 
 * Revision 2.4  90/09/23  17:45:20  jsb
 * 	Added support for iPSC386.
 * 	[90/09/21  16:42:34  jsb]
 * 
 * Revision 2.3  90/08/27  21:58:29  dbg
 * 	Change fix_desc to match new fake_descriptor format.
 * 	[90/07/25            dbg]
 * 
 * Revision 2.2  90/05/03  15:37:40  dbg
 * 	Created.
 * 	[90/02/14            dbg]
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

#include <platforms.h>
#include <cpus.h>
#include <mach_kdb.h>

#include <i386/asm.h>
#include <i386/proc_reg.h>
#include <assym.s>

#if	NCPUS > 1

#ifdef	SYMMETRY
#include <sqt/asm_macros.h>
#endif

#ifdef	SQT
#include <i386/SQT/asm_macros.h>
#endif	/* SQT */

#ifdef	MBUS
#include <busses/mbus/mbus.h>
#endif	/* MBUS */

#define	CX(addr,reg)	addr(,reg,4)

#else

#define	CPU_NUMBER(reg)
#define	CX(addr,reg)	addr

#endif	/* NCPUS > 1 */

#ifdef	AT386
#include <i386/AT386/mp/mp.h>
#endif	/* AT386 */

#ifdef	CBUS
#include <busses/cbus/cbus.h>
#endif	/* CBUS */

/*
 * GAS won't handle an intersegment jump with a relocatable offset.
 */
#define	LJMP(segment,address)	\
	.byte	0xea		;\
	.long	address		;\
	.word	segment



#define KVTOPHYS	(-KERNELBASE)
#define	KVTOLINEAR	LINEAR_KERNELBASE


#define	PA(addr)	(addr)+KVTOPHYS
#define	VA(addr)	(addr)-KVTOPHYS

	.data
/*
 * Interrupt and bootup stack for initial processor.
 */
	.align	ALIGN
	.globl	EXT(intstack)
EXT(intstack):
	.set	., .+INTSTACK_SIZE
	.globl	EXT(eintstack)
EXT(eintstack:)

#if	NCPUS == 1
	.globl	EXT(int_stack_high)	# all interrupt stacks
EXT(int_stack_high):			# must lie below this
	.long	EXT(eintstack)		# address

	.globl	EXT(int_stack_top)	# top of interrupt stack
EXT(int_stack_top):
	.long	EXT(eintstack)
#endif

#if	MACH_KDB
/*
 * Kernel debugger stack for each processor.
 */
	.align	ALIGN
	.globl	EXT(db_stack_store)
EXT(db_stack_store):
	.set	., .+(INTSTACK_SIZE*NCPUS)

/*
 * Stack for last-ditch debugger task for each processor.
 */
	.align	ALIGN
	.globl	EXT(db_task_stack_store)
EXT(db_task_stack_store):
	.set	., .+(INTSTACK_SIZE*NCPUS)
#endif	/* MACH_KDB */

/*
 * per-processor kernel debugger stacks
 */
        .align  ALIGN
        .globl  EXT(kgdb_stack_store)
EXT(kgdb_stack_store):
        .set    ., .+(INTSTACK_SIZE*NCPUS)


/*
 * Pointers to GDT and IDT.  These contain linear addresses.
 */
	.align	ALIGN
	.globl	EXT(gdtptr)
LEXT(gdtptr)
	.word	Times(8,GDTSZ)-1
	.long	EXT(gdt)+KVTOLINEAR

	.align	ALIGN
	.globl	EXT(idtptr)
LEXT(idtptr)
	.word	Times(8,IDTSZ)-1
	.long	EXT(idt)+KVTOLINEAR

#if	NCPUS > 1
	.data
	/*
	 *	start_lock is very special.  We initialize the
	 *	lock at allocation time rather than at run-time.
	 *	Although start_lock should be an instance of a
	 *	hw_lock, we hand-code all manipulation of the lock
	 *	because the hw_lock code may require function calls;
	 *	and we'd rather not introduce another dependency on
	 *	a working stack at this point.
	 */
	.globl	EXT(start_lock)
EXT(start_lock):
	.long	0			/ synchronizes processor startup

	.globl	EXT(master_is_up)
EXT(master_is_up):
	.long	0			/ 1 when OK for other processors
					/ to start
	.globl	EXT(mp_boot_pde)
EXT(mp_boot_pde):
	.long	0
#endif	/* NCPUS > 1 */

/*
 * All CPUs start here.
 *
 * Environment:
 *	protected mode, no paging, flat 32-bit address space.
 *	(Code/data/stack segments have base == 0, limit == 4G)
 */
	.text
	.align	ALIGN
	.globl	EXT(pstart)
LEXT(pstart)
	mov	$0,%ax			/ fs must be zeroed;
	mov	%ax,%fs			/ some bootstrappers don`t do this
	mov	%ax,%gs

#if	NCPUS > 1
	jmp	1f
0:	cmpl	$0,PA(EXT(start_lock))
	jne	0b
1:	movb	$1,%eax
	xchgl	%eax,PA(EXT(start_lock)) / locked
	testl	%eax,%eax
	jnz	0b

	cmpl	$0,PA(EXT(master_is_up))	/ are we first?
	jne	EXT(slave_start)		/ no -- system already up.
	movl	$1,PA(EXT(master_is_up))	/ others become slaves
#endif	/* NCPUS > 1 */

/*
 * Get startup parameters.
 */

#ifdef	SYMMETRY
#include <sqt/asm_startup.h>
#endif
#ifdef	SQT
#include <i386/SQT/asm_startup.h>
#endif
#ifdef	AT386
#include <i386/AT386/asm_startup.h>
#endif
#ifdef	iPSC386
#include <i386ipsc/asm_startup.h>
#endif

/*
 * Build initial page table directory and page tables.
 * %ebx holds first available physical address.
 */

	addl	$(NBPG-1),%ebx		/ round first avail physical addr
	andl	$(-NBPG),%ebx		/ to machine page size
	leal	-KVTOPHYS(%ebx),%eax	/ convert to virtual address
	movl	%eax,PA(EXT(kpde))	/ save as kernel page table directory
	movl	%ebx,%cr3		/ set physical address in CR3 now

	movl	%ebx,%edi		/ clear page table directory
	movl	$(PTES_PER_PAGE),%ecx	/ one page of ptes
	xorl	%eax,%eax
	cld
	rep
	stosl				/ edi now points to next page

#ifdef	CBUS

/*
 *	Map first meg at KERNELBASE for AT bus registers and for this code
 */
	movl	%edi, %esi		/ Use next available page as page table
	andl	$(-NBPG),%esi		/ mask out offset in page
	orl	$INTEL_PTE_KERNEL,%esi	/ add pte bits
	movl	%esi, KERNELBASEPDE(%ebx)
	movl	%edi, %esi		
	addl	$NBPG,%edi		/ update next available page
	movl	$INTEL_PTE_KERNEL,%eax	/ set up pte prototype
	movl	$256, %ecx
4:
	movl	%eax, (%esi)		/ set pte
	addl	$4,%esi			/ advance to next pte
	addl	$NBPG,%eax		/ advance to next phys page
	subl	$1, %ecx
	cmpl	$0, %ecx
	jne	4b

#endif	/* CBUS */

/*
 * Use next few pages for page tables.
 */
	addl	$(KERNELBASEPDE),%ebx	/ point to pde for kernel base
#ifdef	CBUS
	addl	$(CBUS_START_PDE), %ebx
#endif	/* CBUS */
	movl	%edi,%esi		/ point to end of current pte page

/*
 * Enter 1-1 mappings for kernel and for kernel page tables.
 */
	movl	$(INTEL_PTE_KERNEL),%eax / set up pte prototype
#ifdef	CBUS
	addl	$CBUS_START, %eax
#endif	/* CBUS */
0:
	cmpl	%esi,%edi		/ at end of pte page?
	jb	1f			/ if so:
	movl	%edi,%edx		/    get pte address (physical)
	andl	$(-NBPG),%edx		/    mask out offset in page
	orl	$(INTEL_PTE_KERNEL),%edx /   add pte bits
	movl	%edx,(%ebx)		/    set pde
	addl	$4,%ebx			/    point to next pde
	movl	%edi,%esi		/    point to
	addl	$(NBPG),%esi		/    end of new pte page
1:
	movl	%eax,(%edi)		/ set pte
	addl	$4,%edi			/ advance to next pte
	addl	$(NBPG),%eax		/ advance to next phys page
	cmpl	%edi,%eax		/ have we mapped this pte page yet?
	jb	0b			/ loop if not

/*
 * Zero rest of last pte page.
 */
	xor	%eax,%eax		/ don`t map yet
2:	cmpl	%esi,%edi		/ at end of pte page?
	jae	3f
	movl	%eax,(%edi)		/ zero mapping
	addl	$4,%edi
	jmp	2b
3:

#if	NCPUS > 1
/*
 * Grab (waste?) another page for a bootstrap page directory
 * for the other CPUs.  We don't want the running CPUs to see
 * addresses 0..3fffff mapped 1-1.
 */
	movl	%edi,PA(EXT(mp_boot_pde)) / save its physical address
	movl	$(PTES_PER_PAGE),%ecx	/ and clear it
	rep
	stosl
#endif	/* NCPUS > 1 */
	movl	%edi,PA(EXT(first_avail)) / save first available phys addr

/*
 * pmap_bootstrap will enter rest of mappings.
 */

/*
 * Fix initial descriptor tables.
 */
	lea	PA(EXT(idt)),%esi	/ fix IDT
	movl	$(IDTSZ),%ecx
	movl	$(PA(fix_idt_ret)),%ebx
	jmp	fix_desc_common		/ (cannot use stack)
fix_idt_ret:

	lea	PA(EXT(gdt)),%esi	/ fix GDT
	movl	$(GDTSZ),%ecx
	movl	$(PA(fix_gdt_ret)),%ebx
	jmp	fix_desc_common		/ (cannot use stack)
fix_gdt_ret:

	lea	PA(EXT(ldt)),%esi	/ fix LDT
	movl	$(LDTSZ),%ecx
	movl	$(PA(fix_ldt_ret)),%ebx
	jmp	fix_desc_common		/ (cannot use stack)
fix_ldt_ret:

/*
 * Turn on paging.
 */
	movl	%cr3,%eax		/ retrieve kernel PDE phys address
#ifdef	CBUS
	movl	(KERNELBASEPDE+CBUS_START_PDE)(%eax),%ecx
	movl	%ecx,CBUS_START_PDE(%eax)		
#else	/* CBUS */
	movl	KERNELBASEPDE(%eax),%ecx
#endif	/* CBUS */
	movl	%ecx,(%eax)		/ set it also as pte for location
					/ 0..3fffff, so that the code
					/ that enters paged mode is mapped
					/ to identical addresses after
					/ paged mode is enabled

	movl	$EXT(pag_start),%ebx	/ first paged code address

	movl	%cr0,%eax
	orl	$(CR0_PG),%eax		/ set PG bit in CR0
	orl	$(CR0_WP),%eax
	movl	%eax,%cr0		/ to enable paging

	jmp	*%ebx			/ flush prefetch queue

/*
 * We are now paging, and can run with correct addresses.
 */
LEXT(pag_start)
	lgdt	EXT(gdtptr)		/ load GDT
	lidt	EXT(idtptr)		/ load IDT
	LJMP(KERNEL_CS,EXT(vstart))	/ switch to kernel code segment

/*
 * Master is now running with correct addresses.
 */
LEXT(vstart)
	mov	$(KERNEL_DS),%ax	/ set kernel data segment
	mov	%ax,%ds
	mov	%ax,%es
	mov	%ax,%ss
	mov	%ax,EXT(ktss)+TSS_SS0	/ set kernel stack segment
					/ for traps to kernel
#if	MACH_KDB
	mov	%ax,EXT(dbtss)+TSS_SS0	/ likewise for debug task switch
	mov	%cr3,%eax		/ get PDBR into debug TSS
	mov	%eax,EXT(dbtss)+TSS_PDBR
	mov	$0,%eax
#endif

	movw	$(KERNEL_LDT),%ax	/ get LDT segment
	lldt	%ax			/ load LDT
#if	MACH_KDB
	mov	%ax,EXT(ktss)+TSS_LDT	/ store LDT in two TSS, as well...
	mov	%ax,EXT(dbtss)+TSS_LDT	/   ...matters if we switch tasks
#endif
	movw	$(KERNEL_TSS),%ax
	ltr	%ax			/ set up KTSS

	mov	$CPU_DATA,%ax
	mov	%ax,%gs

	lea	EXT(eintstack),%esp	/ switch to the bootup stack
	call	EXT(machine_startup)	/ run C code
	/*NOTREACHED*/
	hlt

#if	NCPUS > 1
/*
 * master_up is used by the master cpu to signify that it is done
 * with the interrupt stack, etc. See the code in pstart and svstart
 * that this interlocks with.
 */
	.align	ALIGN
	.globl	EXT(master_up)
LEXT(master_up)
	pushl	%ebp			/ set up
	movl	%esp,%ebp		/ stack frame
	movl	$0,%ecx			/ unlock start_lock
	xchgl	%ecx,EXT(start_lock)	/ since we are no longer using
					/ bootstrap stack
	leave				/ pop stack frame
	ret

/*
 * We aren't the first.  Call slave_main to initialize the processor
 * and get Mach going on it.
 */
	.align	ALIGN
	.globl	EXT(slave_start)
LEXT(slave_start)
	cli				/ disable interrupts, so we don`t
					/ need IDT for a while
	movl	EXT(kpde)+KVTOPHYS,%ebx	/ get PDE virtual address
	addl	$(KVTOPHYS),%ebx	/ convert to physical address

	movl	PA(EXT(mp_boot_pde)),%edx / point to the bootstrap PDE
#ifdef	CBUS
	movl	KERNELBASEPDE+CBUS_START_PDE(%ebx),%eax
					/ point to pte for KERNELBASE
	movl	%eax,KERNELBASEPDE+CBUS_START_PDE(%edx)
					/ set in bootstrap PDE
	movl	%eax,CBUS_START_PDE(%edx)
#else	/* CBUS */
	movl	KERNELBASEPDE(%ebx),%eax
					/ point to pte for KERNELBASE
	movl	%eax,KERNELBASEPDE(%edx)
					/ set in bootstrap PDE
#endif	/* CBUS */
	movl	%eax,(%edx)		/ set it also as pte for location
					/ 0..3fffff, so that the code
					/ that enters paged mode is mapped
					/ to identical addresses after
					/ paged mode is enabled
	movl	%edx,%cr3		/ use bootstrap PDE to enable paging

	movl	$EXT(spag_start),%edx	/ first paged code address

	movl	%cr0,%eax
	orl	$(CR0_PG),%eax		/ set PG bit in CR0
	orl	$(CR0_WP),%eax
	movl	%eax,%cr0		/ to enable paging

	jmp	*%edx			/ flush prefetch queue.

/*
 * We are now paging, and can run with correct addresses.
 */
LEXT(spag_start)

	lgdt	EXT(gdtptr)		/ load GDT
	lidt	EXT(idtptr)		/ load IDT
	LJMP(KERNEL_CS,EXT(svstart))	/ switch to kernel code segment

/*
 * Slave is now running with correct addresses.
 */
LEXT(svstart)
	mov	$(KERNEL_DS),%ax	/ set kernel data segment
	mov	%ax,%ds
	mov	%ax,%es
	mov	%ax,%ss

	movl	%ebx,%cr3		/ switch to the real kernel PDE 
#if	CBUS
	movl	%cr3, %eax
	orl	EXT(next_cpu), %eax
	movl	%eax, %cr3
	incl	EXT(cbus_ncpus)
#endif	/* CBUS */

#if	MBUS
	movl	%cr3, %eax
	orl	$1, %eax		/* cpu 1 */
	movl	%eax, %cr3
#endif	/* MBUS */

	CPU_NUMBER(%eax)
	movl	CX(EXT(interrupt_stack),%eax),%esp / get stack
	addl	$(INTSTACK_SIZE),%esp	/ point to top
	xorl	%ebp,%ebp		/ for completeness

	movl	$0,%ecx			/ unlock start_lock
	xchgl	%ecx,EXT(start_lock)	/ since we are no longer using
					/ bootstrap stack

/*
 * switch to the per-cpu descriptor tables
 */

	pushl	%eax			/ pass CPU number
	call	EXT(mp_desc_init)	/ set up local table
					/ pointer returned in %eax
	subl	$4,%esp			/ get space to build pseudo-descriptors
	
	CPU_NUMBER(%eax)
	movw	$(GDTSZ*8-1),0(%esp)	/ set GDT size in GDT descriptor
	movl	CX(EXT(mp_gdt),%eax),%edx
	addl	$KVTOLINEAR,%edx
	movl	%edx,2(%esp)		/ point to local GDT (linear address)
	lgdt	0(%esp)			/ load new GDT
	
	movw	$(IDTSZ*8-1),0(%esp)	/ set IDT size in IDT descriptor
	movl	CX(EXT(mp_idt),%eax),%edx
	addl	$KVTOLINEAR,%edx
	movl	%edx,2(%esp)		/ point to local IDT (linear address)
	lidt	0(%esp)			/ load new IDT
	
	movw	$(KERNEL_LDT),%ax
	lldt	%ax			/ load new LDT
	
	movw	$(KERNEL_TSS),%ax
	ltr	%ax			/ load new KTSS

	mov	$CPU_DATA,%ax
	mov	%ax,%gs

	call	EXT(slave_main)		/ start MACH
	/*NOTREACHED*/
	hlt
#endif	/* NCPUS > 1 */

/*
 * Convert a descriptor from fake to real format.
 *
 * Calls from assembly code:
 * %ebx = return address (physical) CANNOT USE STACK
 * %esi	= descriptor table address (physical)
 * %ecx = number of descriptors
 *
 * Calls from C:
 * 0(%esp) = return address
 * 4(%esp) = descriptor table address (physical)
 * 8(%esp) = number of descriptors
 *
 * Fake descriptor format:
 *	bytes 0..3		base 31..0
 *	bytes 4..5		limit 15..0
 *	byte  6			access byte 2 | limit 19..16
 *	byte  7			access byte 1
 *
 * Real descriptor format:
 *	bytes 0..1		limit 15..0
 *	bytes 2..3		base 15..0
 *	byte  4			base 23..16
 *	byte  5			access byte 1
 *	byte  6			access byte 2 | limit 19..16
 *	byte  7			base 31..24
 *
 * Fake gate format:
 *	bytes 0..3		offset
 *	bytes 4..5		selector
 *	byte  6			word count << 4 (to match fake descriptor)
 *	byte  7			access byte 1
 *
 * Real gate format:
 *	bytes 0..1		offset 15..0
 *	bytes 2..3		selector
 *	byte  4			word count
 *	byte  5			access byte 1
 *	bytes 6..7		offset 31..16
 */
	.globl	EXT(fix_desc)
LEXT(fix_desc)
	pushl	%ebp			/ set up
	movl	%esp,%ebp		/ stack frame
	pushl	%esi			/ save registers
	pushl	%ebx
	movl	B_ARG0,%esi		/ point to first descriptor
	movl	B_ARG1,%ecx		/ get number of descriptors
	lea	0f,%ebx			/ get return address
	jmp	fix_desc_common		/ call internal routine
0:	popl	%ebx			/ restore registers
	popl	%esi
	leave				/ pop stack frame
	ret				/ return

fix_desc_common:
0:
	movw	6(%esi),%dx		/ get access byte
	movb	%dh,%al
	andb	$0x14,%al
	cmpb	$0x04,%al		/ gate or descriptor?
	je	1f

/ descriptor
	movl	0(%esi),%eax		/ get base in eax
	rol	$16,%eax		/ swap 15..0 with 31..16
					/ (15..0 in correct place)
	movb	%al,%dl			/ combine bits 23..16 with ACC1
					/ in dh/dl
	movb	%ah,7(%esi)		/ store bits 31..24 in correct place
	movw	4(%esi),%ax		/ move limit bits 0..15 to word 0
	movl	%eax,0(%esi)		/ store (bytes 0..3 correct)
	movw	%dx,4(%esi)		/ store bytes 4..5
	jmp	2f

/ gate
1:
	movw	4(%esi),%ax		/ get selector
	shrb	$4,%dl			/ shift word count to proper place
	movw	%dx,4(%esi)		/ store word count / ACC1
	movw	2(%esi),%dx		/ get offset 16..31
	movw	%dx,6(%esi)		/ store in correct place
	movw	%ax,2(%esi)		/ store selector in correct place
2:
	addl	$8,%esi			/ bump to next descriptor
	loop	0b			/ repeat
	jmp	*%ebx			/ all done

/*
 * put arg in kbd leds and spin a while
 * eats eax, ecx, edx
 */
#define	K_RDWR		0x60
#define	K_CMD_LEDS	0xed
#define	K_STATUS	0x64
#define	K_IBUF_FULL	0x02		/* input (to kbd) buffer full */
#define	K_OBUF_FULL	0x01		/* output (from kbd) buffer full */

ENTRY(set_kbd_leds)
	mov	S_ARG0,%cl		/ save led value
	
0:	inb	$(K_STATUS),%al		/ get kbd status
	testb	$(K_IBUF_FULL),%al	/ input busy?
	jne	0b			/ loop until not
	
	mov	$(K_CMD_LEDS),%al	/ K_CMD_LEDS
	outb	%al,$(K_RDWR)		/ to kbd

0:	inb	$(K_STATUS),%al		/ get kbd status
	testb	$(K_OBUF_FULL),%al	/ output present?
	je	0b			/ loop if not

	inb	$(K_RDWR),%al		/ read status (and discard)

0:	inb	$(K_STATUS),%al		/ get kbd status
	testb	$(K_IBUF_FULL),%al	/ input busy?
	jne	0b			/ loop until not
	
	mov	%cl,%al			/ move led value
	outb	%al,$(K_RDWR)		/ to kbd

	movl	$10000000,%ecx		/ spin
0:	nop
	nop
	loop	0b			/ a while

	ret
