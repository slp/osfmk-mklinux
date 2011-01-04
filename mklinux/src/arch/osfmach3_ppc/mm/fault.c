/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *  ARCH/ppc/mm/fault.c
 *
 *  Copyright (C) 1991, 1992, 1993, 1994  Linus Torvalds
 *  Ported to PPC by Gary Thomas
 */

#include <linux/config.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>

#include <asm/page.h>
#include <asm/pgtable.h>

extern void die_if_kernel(char *, struct pt_regs *, long);
extern void do_page_fault(struct pt_regs *, unsigned long, unsigned long);

#define SHOW_FAULTS
#undef  SHOW_FAULTS

#define PAUSE_AFTER_FAULT
#undef  PAUSE_AFTER_FAULT

#ifdef	CONFIG_OSFMACH3
#ifdef	CONFIG_OSFMACH3_DEBUG
#define FAULT_DEBUG	1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#if	FAULT_DEBUG
int fault_debug = 0;
#endif	/* FAULT_DEBUG */
#endif	/* CONFIG_OSFMACH3 */

#ifndef	CONFIG_OSFMACH3
void
DataAccessException(struct pt_regs *regs)
{
	pgd_t *dir;
	pmd_t *pmd;
	pte_t *pte;
	int tries, mode = 0;
	if (user_mode(regs)) mode |= 0x04;
	if (regs->dsisr & 0x02000000) mode |= 0x02;  /* Load/store */
	if (regs->dsisr & 0x08000000) mode |= 0x01;  /* Protection violation */
#ifdef SHOW_FAULTS
	printk("Data Access Fault - Loc: %x, DSISR: %x, PC: %x\n", regs->dar, regs->dsisr, regs->nip);
#ifdef PAUSE_AFTER_FAULT
cnpause();
#endif			
#endif
	if (mode & 0x01)
	{
#ifdef SHOW_FAULTS
printk("Write Protect Fault - Loc: %x, DSISR: %x, PC: %x\n", regs->dar, regs->dsisr, regs->nip);
#endif
		do_page_fault(regs, regs->dar, mode);
		return;
	}
	for (tries = 0;  tries < 1;  tries++)
	{
		dir = pgd_offset(current->mm, regs->dar & PAGE_MASK);
		if (dir)
		{
			pmd = pmd_offset(dir, regs->dar & PAGE_MASK);
			if (pmd && pmd_present(*pmd))
			{
				pte = pte_offset(pmd, regs->dar & PAGE_MASK);
				if (pte && pte_present(*pte))
				{
#ifdef SHOW_FAULTS
					printk("Page mapped - PTE: %x[%x], Context: %x\n", pte, *(long *)pte, current->mm->context);
#endif					
					MMU_hash_page(&current->tss, regs->dar & PAGE_MASK, pte);
					return;
				}
			}
		} else
		{
			printk("No PGD\n");
		}
		do_page_fault(regs, regs->dar, mode);
	}
}

void
InstructionAccessException(struct pt_regs *regs)
{
	pgd_t *dir;
	pmd_t *pmd;
	pte_t *pte;
	int tries, mode = 0;
	unsigned long addr = regs->nip;
	if (user_mode(regs)) mode |= 0x04;
#ifdef SHOW_FAULTS
	printk("Instruction Access Fault - Loc: %x, DSISR: %x, PC: %x\n", regs->dar, regs->dsisr, regs->nip);
#ifdef PAUSE_AFTER_FAULT
cnpause();
#endif
#endif	
	if (mode & 0x01)
	{
		do_page_fault(regs, addr, mode);
		return;
	}
	for (tries = 0;  tries < 1;  tries++)
	{
		dir = pgd_offset(current->mm, addr & PAGE_MASK);
		if (dir)
		{
			pmd = pmd_offset(dir, addr & PAGE_MASK);
			if (pmd && pmd_present(*pmd))
			{
				pte = pte_offset(pmd, addr & PAGE_MASK);
				if (pte && pte_present(*pte))
				{
#ifdef SHOW_FAULTS
					printk("Page mapped - PTE: %x[%x], Context: %x\n", pte, *(long *)pte, current->mm->context);
#endif					
					MMU_hash_page(&current->tss, addr & PAGE_MASK, pte);
					return;
				}
			}
		} else
		{
			printk("No PGD\n");
		}
		do_page_fault(regs, addr, mode);
	}
}
#endif	/* CONFIG_OSFMACH3 */

/*
 * This routine handles page faults.  It determines the address,
 * and the problem, and then passes it off to one of the appropriate
 * routines.
 *
 * The error_code parameter just the same as in the i386 version:
 *
 *	bit 0 == 0 means no page found, 1 means protection fault
 *	bit 1 == 0 means read, 1 means write
 *	bit 2 == 0 means kernel, 1 means user-mode
 */
void do_page_fault(struct pt_regs *regs, unsigned long address, unsigned long error_code)
{
	struct vm_area_struct * vma;
#ifndef	CONFIG_OSFMACH3
	unsigned long page;
#endif	/* CONFIG_OSFMACH3 */

#ifdef	CONFIG_OSFMACH3
#if	FAULT_DEBUG
	if (fault_debug) {
		printk("do_page_fault: fault at 0x%lx\n", address);
	}
#endif	/* FAULT_DEBUG */
#endif	/* CONFIG_OSFMACH3 */
	for (vma = current->mm->mmap ; ; vma = vma->vm_next)
	{
#ifdef SHOW_FAULTS
printk("VMA(%x) - Start: %x, End: %x, Flags: %x\n", vma, vma->vm_start, vma->vm_end, vma->vm_flags);		
#endif
		if (!vma)
			goto bad_area;
		if (vma->vm_end > address)
			break;
	}
	if (vma->vm_start <= address)
#ifdef	CONFIG_OSFMACH3
		goto bad_area;	/* we don't do swap in the server */
#else	/* CONFIG_OSFMACH3 */
		goto good_area;
#endif	/* CONFIG_OSFMACH3 */
	if (!(vma->vm_flags & VM_GROWSDOWN))
		goto bad_area;
#ifdef	CONFIG_OSFMACH3
	if (expand_stack(vma, address))
		goto bad_area;
#if	FAULT_DEBUG
	if (fault_debug) {
		printk("do_page_fault: vma: start=0x%lx,end=0x%lx,off=0x%lx\n",
		       vma->vm_start, vma->vm_end, vma->vm_offset);
	}
#endif	/* FAULT_DEBUG */
	return;
#else	/* CONFIG_OSFMACH3 */
	if (vma->vm_end - address > current->rlim[RLIMIT_STACK].rlim_cur)
		goto bad_area;
	vma->vm_offset -= vma->vm_start - (address & PAGE_MASK);
	vma->vm_start = (address & PAGE_MASK);
/*
 * Ok, we have a good vm_area for this memory access, so
 * we can handle it..
 */
good_area:
	/*
	 * was it a write?
	 */
	if (error_code & 2) {
		if (!(vma->vm_flags & VM_WRITE))
			goto bad_area;
	} else {
		/* read with protection fault? */
		if (error_code & 1)
			goto bad_area;
		if (!(vma->vm_flags & (VM_READ | VM_EXEC)))
			goto bad_area;
	}
	handle_mm_fault(vma, address, error_code & 2);
	flush_page(address);  /* Flush & Invalidate cache - note: address is OK now */
	return;
#endif	/* CONFIG_OSFMACH3 */

/*
 * Something tried to access memory that isn't in our memory map..
 * Fix it, but check if it's kernel or user first..
 */
bad_area:
#ifdef	CONFIG_OSFMACH3
#if	FAULT_DEBUG
	if (fault_debug) {
		printk("do_page_fault: bad area, SIGSEGV\n");
	}
#endif	/* FAULT_DEBUG */
	if (current != &init_task) {
		current->osfmach3.thread->fault_address = address;
		current->osfmach3.thread->error_code = error_code;
		current->osfmach3.thread->trap_no = 14;
		force_sig(SIGSEGV, current);
		return;
	}
	die_if_kernel("Oops", regs, error_code);
#else	/* CONFIG_OSFMACH3 */
printk("Task: %x, PC: %x/%x, bad area! - Addr: %x\n", current, regs->nip, current->tss.last_pc, address);
#if 0
print_user_backtrace(current->tss.user_stack);
print_kernel_backtrace();
#endif
#if 0
cnpause();
#if 0
if (!user_mode(regs))
{
   print_backtrace(regs->gpr[1]);
}
#endif
#endif
dump_regs(regs);
	if (user_mode(regs)) {
#if 0
		current->tss.cp0_badvaddr = address;
		current->tss.error_code = error_code;
		current->tss.trap_no = 14;
#endif
		force_sig(SIGSEGV, current);
		return;
	}
	if (current->tss.user_access)
	{
	  do_exit(SIGSEGV);
	}
printk("KERNEL! Task: %x, PC: %x, bad area! - Addr: %x, PGDIR: %x\n", current, regs->nip, address, current->tss.pg_tables);
dump_regs(regs);
while (1) ;
#if 0	
	/*
	 * Oops. The kernel tried to access some bad page. We'll have to
	 * terminate things with extreme prejudice.
	 */
	if ((unsigned long) (address-TASK_SIZE) < PAGE_SIZE) {
		printk(KERN_ALERT "Unable to handle kernel NULL pointer dereference");
		pg0[0] = pte_val(mk_pte(0, PAGE_SHARED));
	} else
		printk(KERN_ALERT "Unable to handle kernel paging request");
	printk(" at virtual address %08lx\n",address);
	page = current->tss.pg_dir;
	printk(KERN_ALERT "current->tss.pg_dir = %08lx\n", page);
	page = ((unsigned long *) page)[address >> PGDIR_SHIFT];
	printk(KERN_ALERT "*pde = %08lx\n", page);
	if (page & 1) {
		page &= PAGE_MASK;
		address &= 0x003ff000;
		page = ((unsigned long *) page)[address >> PAGE_SHIFT];
		printk(KERN_ALERT "*pte = %08lx\n", page);
	}
	die_if_kernel("Oops", regs, error_code);
#endif	
#endif	/* CONFIG_OSFMACH3 */
	do_exit(SIGKILL);
}

#ifndef	CONFIG_OSFMACH3
va_to_phys(unsigned long address)
{
	pgd_t *dir;
	pmd_t *pmd;
	pte_t *pte;
	dir = pgd_offset(current->mm, address & PAGE_MASK);
	if (dir)
	{
		pmd = pmd_offset(dir, address & PAGE_MASK);
		if (pmd && pmd_present(*pmd))
		{
			pte = pte_offset(pmd, address & PAGE_MASK);
			if (pte && pte_present(*pte))
			{
				return(pte_page(*pte) | (address & ~(PAGE_MASK-1)));
			}
		} else
		{
			return (0);
		}
	} else
	{
		return (0);
	}
	return (0);
}
#endif	/* CONFIG_OSFMACH3 */

/*
 * See if an address should be valid in the current context.
 */
#ifdef	CONFIG_OSFMACH3
int
#endif	/* CONFIG_OSFMACH3 */
valid_addr(unsigned long addr)
{
	struct vm_area_struct * vma;
	for (vma = current->mm->mmap ; ; vma = vma->vm_next)
	{
		if (!vma)
		{
			return (0);
		}
		if (vma->vm_end > addr)
			break;
	}
	if (vma->vm_start <= addr)
	{
		return (1);
	}
	return (0);
}

#ifdef	CONFIG_OSFMACH3

unsigned char get_user_byte(const char * addr)
{
	return (get_fs_byte(addr));
}


unsigned short get_user_word(const short *addr)
{
	return (get_fs_word(addr));
}


unsigned long get_user_long(const int *addr)
{
	return (get_fs_long(addr));
}


void put_user_byte(char val,char *addr)
{
	put_fs_byte(val, addr);
}


void put_user_word(short val,short * addr)
{
	put_fs_word(val, addr);
}


void put_user_long(unsigned long val,int * addr)
{
	put_fs_long(val, addr);
}

#else	/* CONFIG_OSFMACH3 */

extern int bad_user_access_length(void);

void __put_user(unsigned long x, void * y, int size)
{
	current->tss.user_access = 1;
	switch (size) {
		case 1:
			*(char *) y = x;
			break;
		case 2:
			*(short *) y = x;
			break;
		case 4:
			*(int *) y = x;
			break;
		case 8:
			*(long *) y = x;
			break;
		default:
			bad_user_access_length();
	}
	current->tss.user_access = 0;
}

unsigned long __get_user(const void * y, int size)
{
	current->tss.user_access = 1;
	switch (size) {
		case 1:
			return *(unsigned char *) y;
		case 2:
			return *(unsigned short *) y;
		case 4:
			return *(unsigned int *) y;
		case 8:
			return *(unsigned long *) y;
		default:
			return bad_user_access_length();
	}
	current->tss.user_access = 0;
}

unsigned char get_user_byte(const char * addr)
{
	char val;
	current->tss.user_access = 1;
	val = *addr;
	current->tss.user_access = 1;
	return(val);
}


unsigned short get_user_word(const short *addr)
{
	short val;
	current->tss.user_access = 1;
	val = *addr;
	current->tss.user_access = 1;
	return(val);
}


unsigned long get_user_long(const int *addr)
{
	long val;
	current->tss.user_access = 1;
	val = *addr;
	current->tss.user_access = 1;
	return(val);
}


void put_user_byte(char val,char *addr)
{
	current->tss.user_access = 1;
	*addr = val;
	current->tss.user_access = 1;
}


void put_user_word(short val,short * addr)
{
	current->tss.user_access = 1;
	*addr = val;
	current->tss.user_access = 1;
	*addr = val;
}


void put_user_long(unsigned long val,int * addr)
{
	current->tss.user_access = 1;
	*addr = val;
	current->tss.user_access = 1;
}

#endif	/* CONFIG_OSFMACH3 */
