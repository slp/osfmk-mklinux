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
 * Revision 2.20.3.4  92/09/15  17:21:21  jeffreyh
 * 	Export paging and unique naming for default pager file.
 * 	[92/09/10            stans@ssd.intel.com]
 * 
 * 	Do not invoke the default pager on nodes > 0 for the iPSC860.  If
 * 	no pager is set up, this will prevent the various kernels from
 * 	overwriting paged out pages.
 * 	[92/07/13            sjs]
 * 
 * Revision 2.20.3.3  92/04/30  11:58:58  bernadat
 * 	Pass to the server arguments typed at boot prompt.
 * 	[92/03/19            bernadat]
 * 
 * Revision 2.20.3.2  92/03/28  10:09:49  jeffreyh
 * 	Changes from Intel
 * 	[92/03/26  11:52:02  jeffreyh]
 * 
 * 	Added logic to pass -v (verbose) flag to servers
 * 	[92/03/16   	sjs]
 * 
 * Revision 2.20.3.1  92/02/18  19:07:43  jeffreyh
 * 	Changes for iPSC. These should be made more general in the future.
 * 	[92/02/18            jeffreyh]
 * 
 * Revision 2.20  91/12/10  16:32:40  jsb
 * 	Fixes from Intel
 * 	[91/12/10  15:51:50  jsb]
 * 
 * Revision 2.19  91/11/12  11:51:53  rvb
 * 	Added task_insert_send_right.
 * 	Changed BOOTSTRAP_MAP_SIZE to 4 meg.
 * 	[91/11/12            rpd]
 * 
 * Revision 2.18  91/09/12  16:37:49  bohman
 * 	Made bootstrap task call mac2 machine dependent code before running
 * 	'startup', which is loaded from the UX file system.  This needs to
 * 	be handled more generally in the future.
 * 	[91/09/11  17:07:59  bohman]
 * 
 * Revision 2.17  91/08/28  11:14:22  jsb
 * 	Changed msgh_kind to msgh_seqno.
 * 	[91/08/10            rpd]
 * 
 * Revision 2.16  91/08/03  18:18:45  jsb
 * 	Moved bootstrap query earlier. Removed all NORMA specific code.
 * 	[91/07/25  18:25:35  jsb]
 * 
 * Revision 2.15  91/07/31  17:44:14  dbg
 * 	Pass host port to boot_load_program and read_emulator_symbols.
 * 	[91/07/30  17:02:40  dbg]
 * 
 * Revision 2.14  91/07/01  08:24:54  jsb
 * 	Removed notion of master/slave. Instead, refuse to start up
 * 	a bootstrap task whenever startup_name is null.
 * 	[91/06/29  16:48:14  jsb]
 * 
 * Revision 2.13  91/06/19  11:55:57  rvb
 * 	Ask for startup program to override default.
 * 	[91/06/18  21:39:17  rvb]
 * 
 * Revision 2.12  91/06/17  15:46:51  jsb
 * 	Renamed NORMA conditionals.
 * 	[91/06/17  10:49:04  jsb]
 * 
 * Revision 2.11  91/06/06  17:06:53  jsb
 * 	Allow slaves to act as masters (for now).
 * 	[91/05/13  17:36:17  jsb]
 * 
 * Revision 2.10  91/05/18  14:31:32  rpd
 * 	Added argument to kernel_thread.
 * 	[91/04/03            rpd]
 * 
 * Revision 2.9  91/05/14  16:40:06  mrt
 * 	Correcting copyright
 * 
 * Revision 2.8  91/02/05  17:25:42  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:11:22  mrt]
 * 
 * Revision 2.7  90/12/14  11:01:58  jsb
 * 	Changes to NORMA_BOOT support. Use real device port, not a proxy;
 * 	new device forwarding code handles forwarding of requests.
 * 	Have slave not bother starting bootstrap task if there is nothing
 * 	for it to run.
 * 	[90/12/13  21:37:57  jsb]
 * 
 * Revision 2.6  90/09/28  16:55:30  jsb
 * 	Added NORMA_BOOT support.
 * 	[90/09/28  14:04:43  jsb]
 * 
 * Revision 2.5  90/06/02  14:53:39  rpd
 * 	Load emulator symbols.
 * 	[90/05/11  16:58:37  rpd]
 * 
 * 	Made bootstrap_task available externally.
 * 	[90/04/05            rpd]
 * 	Converted to new IPC.
 * 	[90/03/26  22:03:39  rpd]
 * 
 * Revision 2.4  90/01/11  11:43:02  dbg
 * 	Initialize bootstrap print routines.  Remove port number
 * 	printout.
 * 	[89/12/20            dbg]
 * 
 * Revision 2.3  89/11/29  14:09:01  af
 * 	Enlarged the bootstrap task's map to accomodate some unnamed
 * 	greedy RISC box.  Sigh.
 * 	[89/11/07            af]
 * 	Made root_name and startup_name non-preallocated, so that
 * 	they can be changed at boot time on those machines like
 * 	mips and Sun where the boot loader passes command line
 * 	arguments to the kernel.
 * 	[89/10/28            af]
 * 
 * Revision 2.2  89/09/08  11:25:02  dbg
 * 	Pass root partition name to default_pager_setup.
 * 	[89/08/31            dbg]
 * 
 * 	Assume that device service has already been created.
 * 	Create bootstrap task here and give it the host and
 * 	device ports.
 * 	[89/08/01            dbg]
 * 
 * 	Call default_pager_setup.
 * 	[89/07/11            dbg]
 * 
 * 12-Apr-89  David Golub (dbg) at Carnegie-Mellon University
 *	Removed console_port.
 *
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
 * Bootstrap the various built-in servers.
 */

#include <string.h>

#include <mach_kdb.h>
#include <bootstrap_symbols.h>
#include <dipc.h>

#include <cputypes.h>
#include <mach/port.h>
#include <mach/message.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/boolean.h>
#include <mach/boot_info.h>
#include <mach/mach_server.h>
#include <mach/mach_port_server.h>
#include <mach/bootstrap_server.h>
#include <mach/elf.h>
#include <i386/multiboot.h>

#include <device/device_port.h>
#include <device/ds_routines.h>
#include <device/conf.h>
#include <device/subrs.h>
#include <ipc/ipc_space.h>

#include <kern/thread.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_entry.h>
#include <kern/host.h>
#include <kern/ledger.h>
#include <kern/processor.h>
#include <kern/zalloc.h>
#include <kern/misc_protos.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_map.h>
#include <vm/vm_fault.h>

#if	MACH_KDB
#include <ddb/db_sym.h>

#if defined(__alpha)
extern boolean_t	use_kdb;
extern int		stpages;
#endif /* defined(__alpha) */

#endif	/* MACH_KDB */

#include "boot_script.h"

mach_port_t	bootstrap_host_security_port;	/* local name */
mach_port_t	bootstrap_wired_ledger_port;	/* local name */
mach_port_t	bootstrap_paged_ledger_port;	/* local name */

extern struct multiboot_info mb_info;
vm_offset_t	kern_sym_start = 0;	/* pointer to kernel symbols */
vm_size_t	kern_sym_size = 0;	/* size of kernel symbols */
vm_offset_t	kern_args_start = 0;	/* kernel arguments */
vm_size_t	kern_args_size = 0;	/* size of kernel arguments */
vm_offset_t	boot_sym_start = 0;	/* pointer to bootstrap symbols */
vm_size_t	boot_sym_size = 0;	/* size of bootstrap symbols */
vm_offset_t	boot_args_start = 0;	/* bootstrap arguments */
vm_size_t	boot_args_size = 0;	/* size of bootstrap arguments */
vm_offset_t	boot_start = 0;		/* pointer to bootstrap image */
vm_size_t	boot_size = 0;		/* size of bootstrap image */
vm_offset_t exec_start = 0;
vm_size_t   exec_size = 0;
vm_offset_t	boot_region_desc = 0;	/* bootstrap region descriptions */
vm_size_t	boot_region_count = 0;	/* number of regions */
int		boot_thread_state_flavor = 0;
thread_state_t	boot_thread_state = 0;
unsigned int	boot_thread_state_count = 0;
vm_offset_t	env_start = 0;		/* environment */
vm_size_t	env_size = 0;		/* size of environment */

vm_offset_t	load_info_start = 0;	/* pointer to bootstrap load info */
vm_size_t	load_info_size = 0;	/* size of bootstrap load info */

#define	SYS_REBOOT_COMPAT	defined(i386) || defined(i860) || defined(hp_pa)

void ovbcopy_ints(vm_offset_t,vm_offset_t,vm_size_t); /* forward; */
vm_offset_t move_bootstrap(void);	 /* forward; */

#if	SYS_REBOOT_COMPAT

#ifdef hp_pa
#define	STACK_SIZE		(64*1024)	/* 64k stack */
#define STACK_BASE		(USER_STACK_END-STACK_SIZE)
extern void set_bootstrap_args(void);
#else
#define	STACK_SIZE		(64*1024)	/* 64k stack */
#define STACK_BASE		(VM_MAX_ADDRESS-STACK_SIZE)
#define STACK_PTR		(VM_MAX_ADDRESS-0x10) /* XXX */
#endif 

int	startup_single_user = 0;

struct thread_syscall_state thread_state;
struct region_desc regions[4];
char kernel_args_buf[256] = "/mach_kernel";
char boot_args_buf[256] = "/mach_servers/bootstrap";
char env_buf[256] = "";

extern int halt_in_debugger;
extern char boot_string_store[];
#ifdef	SQT
extern char bootdev[];
#else	/* SQT */
extern int boottype;
#endif	/* SQT */

static void
do_bootstrap_compat(void)
{
	char *startup_flags_begin = 0;
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *) boot_start;
	Elf32_Phdr *phdr, *ph;
	vm_offset_t entry;
	int result, i;
	int bss_start, bss_size;

	if (boot_size == 0)		/* if we don't have a bootstrap */
		return;			/* there isn't anything we can do */

	entry = (vm_offset_t) ehdr->e_entry;
	phdr = (Elf32_Phdr *) (boot_start + ehdr->e_phoff);
	
	printf("entry: 0x%x\n", entry);
	printf("ph offset: 0x%x\n", ehdr->e_phoff);
	printf("phdr: 0x%x\n", phdr);
        printf("Looking for program sections\n");
	printf("Theorical number of sections: %d\n", ehdr->e_phnum);

	for (i = 0, ph = phdr; i < ehdr->e_phnum; i++, ph++) {
		printf("Loop\n");
		switch ((int)ph->p_type) {
		case PT_LOAD:
			if (ph->p_flags == (PF_R | PF_X)) {
				printf("Found text region\n");
				regions[boot_region_count].prot = VM_PROT_READ|VM_PROT_EXECUTE;
				regions[boot_region_count].addr = trunc_page(ph->p_vaddr);
				regions[boot_region_count].size = round_page(ph->p_filesz);
				regions[boot_region_count].offset = trunc_page(ph->p_offset);
				regions[boot_region_count].mapped = TRUE;
			}
			else if (ph->p_flags == (PF_R | PF_W)) {
				printf("Found data region\n");
				regions[boot_region_count].prot = VM_PROT_READ|VM_PROT_WRITE;
				regions[boot_region_count].addr = trunc_page(ph->p_vaddr);
				regions[boot_region_count].size = round_page(ph->p_memsz);
				regions[boot_region_count].offset = trunc_page(ph->p_offset);
				regions[boot_region_count].mapped = TRUE;
				bzero((char *) (boot_start+ph->p_offset+ph->p_filesz),
					ph->p_memsz - ph->p_filesz);
				bss_start = ph->p_vaddr + ph->p_filesz;
				bss_size = ph->p_memsz - ph->p_filesz;
			}
			else {
				printf("Found PT_LOAD region with unknown flags\n");
				continue;
			}

			boot_region_count++;
			break;
		default:
			printf("Found unknown section: 0x%x\n", ph->p_type);
			break;
		}
	}

	printf("I've found: %d sections\n", boot_region_count);

	regions[boot_region_count].addr = STACK_BASE;
	regions[boot_region_count].size = STACK_SIZE;
	regions[boot_region_count].offset = 0;
	regions[boot_region_count].prot = VM_PROT_READ|VM_PROT_WRITE;
	regions[boot_region_count].mapped = FALSE;
	boot_region_count++;

	boot_region_desc = (vm_offset_t) regions;

	bzero((char *)&thread_state, sizeof(thread_state));
#if	defined(i386)
	thread_state.eip = entry;
	thread_state.esp = STACK_PTR;
	boot_thread_state_count = i386_THREAD_SYSCALL_STATE_COUNT;
#endif	/* defined(i386) */
#if	defined(i860)
	thread_state.pc =  entry;
	thread_state.sp =  STACK_PTR;
	boot_thread_state_count = i860_THREAD_STATE_COUNT;
#endif	/* defined(i860) */
#if     defined(hp_pa)
	thread_state.iioq_head |= lp->entry_1;
	thread_state.iioq_tail |= (lp->entry_1+4);
	thread_state.dp = lp->entry_2;
	thread_state.sp = STACK_BASE;
	boot_thread_state_count = HP700_SYSCALL_STATE_COUNT;
#endif  /* defined(hp_pa) */

	boot_thread_state_flavor = THREAD_SYSCALL_STATE;
	boot_thread_state = (thread_state_t) &thread_state;

#if 0
#ifndef hp_pa
	env_start = (vm_offset_t) env_buf;
	env_size = 0;
	{
	    dev_ops_t disk0_ops;
	    int unit;
	    char *p1 = env_buf + env_size;
	    char *p2 = "rootdev_name=";

	    while (*p1 = *p2++)
		p1++;
#ifdef	SQT
	    strcpy(p1, bootdev);
	    while (*p1++);	/* Get to end of string */
#else	/* SQT */
	    if (dev_name_lookup("disk0", &disk0_ops, &unit)) {
		p2 = disk0_ops->d_name;
		while (*p1 = *p2++)
		    p1++;
		itoa(unit, p1);
		while (*p1)
		    p1++;
		*p1++ = 'a' + ((boottype >> 8) & 0xff);
		*p1++ = 0;
	    } else {
		p2 = "boot_device";
		while (*p1++ = *p2++)
		    ;
	    }
#endif	/* SQT */
	    env_size = p1 - env_buf;
	}
	if (boot_string_store[0]) { 
	    char *p1 = env_buf + env_size;
	    char *p2 = "startup_name=";
	    int found_flags = FALSE;

	    while (*p1 = *p2++)
		p1++;

	    p2 = boot_string_store;

	    while (*p2 && *p2 != ':') 
		p2++;

	    if (*p2 == ':') { 
		p2++;
		while (*p2 && *p2 != ' ')
		    *p1++ = *p2++;
		*p1++ = 0;
		env_size = p1 - env_buf;

		/* check for trailing flags */
		while (*p2 && *p2 != '-')
		    p2++;

		if (*p2 == '-')
		    startup_flags_begin = p2;
	    } 

	    /* 
	     * Now put any startup flags to the environment buffer 
	     */
    	    p1 = env_buf + env_size;
	    p2 = "startup_flags=";

	    while (*p1 = *p2++)
		p1++;

	    /* If -s seen in kernel flags add it to the server
	       flags too. */
	    if (startup_single_user) {
		char *p2 = "-s ";
		    
		while (*p2) {
		    *p1++ = *p2++;
	        }

		found_flags = TRUE;
	    }

	    /* If -h seen in kernel flags add it to the server
	       flags too. */
	    if (halt_in_debugger) {
		p2 = " -h ";

		while (*p2) {
		    *p1++ = *p2++;
	        }

		found_flags = TRUE;
	    }

	    /* Any other startup flags seen on boot line. */
	    if (startup_flags_begin) {
		p2 = startup_flags_begin;
		    
		while (*p1++ = *p2++)
		    ;

		found_flags = TRUE;
	    }

	    if (found_flags) {
		*p1++ = 0;
		env_size = p1 - env_buf;
	    }

        }		
#endif /* !hp_pa */
#endif
}
#endif /* SYS_REBOOT_COMPAT */


static void
exec_load(vm_offset_t start, vm_size_t size)
{
	char *startup_flags_begin = 0;
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *) start;
	Elf32_Phdr *phdr, *ph;
	vm_offset_t entry;
	int result, i;
	int bss_start, bss_size;


	if (size == 0)		/* if we don't have a bootstrap */
		return;			/* there isn't anything we can do */

	entry = (vm_offset_t) ehdr->e_entry;
	phdr = (Elf32_Phdr *) (start + ehdr->e_phoff);
	
	printf("entry: 0x%x\n", entry);
	printf("ph offset: 0x%x\n", ehdr->e_phoff);
	printf("phdr: 0x%x\n", phdr);
        printf("Looking for program sections\n");
	printf("Theorical number of sections: %d\n", ehdr->e_phnum);

    boot_region_count = 0;

	for (i = 0, ph = phdr; i < ehdr->e_phnum; i++, ph++) {
		printf("Loop\n");
		switch ((int)ph->p_type) {
		case PT_LOAD:
			if (ph->p_flags == (PF_R | PF_X)) {
				printf("Found text region\n");
				regions[boot_region_count].prot = VM_PROT_READ|VM_PROT_EXECUTE;
				regions[boot_region_count].addr = trunc_page(ph->p_vaddr);
				regions[boot_region_count].size = round_page(ph->p_filesz);
				regions[boot_region_count].offset = trunc_page(ph->p_offset);
				regions[boot_region_count].mapped = TRUE;
			}
			else if (ph->p_flags == (PF_R | PF_W)) {
				printf("Found data region\n");
				regions[boot_region_count].prot = VM_PROT_READ|VM_PROT_WRITE;
				regions[boot_region_count].addr = trunc_page(ph->p_vaddr);
				regions[boot_region_count].size = round_page(ph->p_memsz);
				regions[boot_region_count].offset = trunc_page(ph->p_offset);
				regions[boot_region_count].mapped = TRUE;
				bzero((char *) (start+ph->p_offset+ph->p_filesz),
					ph->p_memsz - ph->p_filesz);
				bss_start = ph->p_vaddr + ph->p_filesz;
				bss_size = ph->p_memsz - ph->p_filesz;
			}
			else {
				printf("Found PT_LOAD region with unknown flags\n");
				continue;
			}

			boot_region_count++;
			break;
		default:
			printf("Found unknown section: 0x%x\n", ph->p_type);
			break;
		}
	}

	printf("I've found: %d sections\n", boot_region_count);

	regions[boot_region_count].addr = STACK_BASE;
	regions[boot_region_count].size = STACK_SIZE;
	regions[boot_region_count].offset = 0;
	regions[boot_region_count].prot = VM_PROT_READ|VM_PROT_WRITE;
	regions[boot_region_count].mapped = FALSE;
	boot_region_count++;

	boot_region_desc = (vm_offset_t) regions;

	bzero((char *)&thread_state, sizeof(thread_state));

	thread_state.eip = entry;
	//thread_state.esp = STACK_PTR;
	boot_thread_state_count = i386_THREAD_SYSCALL_STATE_COUNT;

	boot_thread_state_flavor = THREAD_SYSCALL_STATE;
	boot_thread_state = (thread_state_t) &thread_state;
}    

static void
exec_map(vm_offset_t start, vm_size_t size)
{
	int	i;
	struct region_desc *r;
	vm_offset_t addr;
	vm_object_t object;
	vm_page_t page;
	vm_offset_t off;

	/*
	 * allocate an object and map all of the bootstrap pages into it.
	 */
	object = vm_object_allocate(size);
	for (off = 0; off < size; off += PAGE_SIZE) {
		while ((page = vm_page_grab_fictitious()) == VM_PAGE_NULL)
			vm_page_more_fictitious();
		vm_object_lock(object);
		addr = pmap_extract(kernel_pmap, start + off);

		if (!addr) {
			printf("Warning: bootstrap task has bogus page mapping\n");
			vm_object_unlock(object);
			vm_page_free(page);
			continue;
		}

		vm_page_init(page, addr);
		page->dirty = TRUE;
		page->wire_count = 1;
		vm_page_insert(page, object, off);
		vm_page_lock_queues();		/* unwire will make page active */
		vm_page_unwire(page);
		vm_page_unlock_queues();
		PAGE_WAKEUP_DONE(page);
		vm_object_unlock(object);
	}

	/*
	 * map the bootstrap regions from the object into the user task.
	 * allocate non-mapped regions.
	 */
	r = (struct region_desc *) boot_region_desc;
	for (i = 0; i < boot_region_count; i++, r++) {
		if (r->size == 0)	/* e.g. no BSS */
			continue;
		addr = r->addr;
		if (r->mapped) {
			vm_object_reference(object);
			assert(page_aligned(r->size));
			vm_map_enter(current_task()->map,
				     &addr,
				     r->size,
				     0,
				     FALSE,
				     object,
				     r->offset,
				     FALSE,
				     r->prot,
				     VM_PROT_ALL,
				     VM_INHERIT_DEFAULT);
			/* XXX mapped regions always aligned? */
			assert(addr == r->addr);
#if	MACH_KDB || MACH_KGDB
			if (r->prot & VM_PROT_EXECUTE)
				for (off = 0; off < r->size; off += PAGE_SIZE)
					vm_fault(current_task()->map,
						 addr + off,
						 r->prot,
						 FALSE);
#endif
		} else
			vm_allocate(current_task()->map,
				    &addr,
				    r->size,
				    FALSE);
	}
	vm_object_deallocate(object);
}

static vm_offset_t
set_user_regs(vm_offset_t stack_base, vm_size_t stack_size, 
              vm_size_t arg_size)
{
    vm_offset_t arg_addr;

    arg_size = (arg_size + sizeof(int) - 1) & ~(sizeof(int)-1);
    arg_addr = stack_base + stack_size - arg_size;

	thread_state.esp = (int)arg_addr;

    return (arg_addr);
}

static void
build_args_and_stack(char **argv, char **envp)
{
	vm_offset_t	stack_base;
	vm_size_t	stack_size;
	register
	char *		arg_ptr;
	int		arg_count, envc;
	int		arg_len;
	char *		arg_pos;
	int		arg_item_len;
	char *		string_pos;
	char *		zero = (char *)0;
	int i;

	/*
	 * Calculate the size of the argument list.
	 */
	arg_len = 0;
	arg_count = 0;
	while (argv[arg_count] != 0) {
	    arg_ptr = argv[arg_count++];
	    arg_len += strlen(arg_ptr) + 1;
	}
	envc = 0;
	if (envp != 0)
	  while (envp[envc] != 0)
	    arg_len += strlen (envp[envc++]) + 1;

	/*
	 * Add space for:
	 *	arg count
	 *	pointers to arguments
	 *	trailing 0 pointer
	 *	pointers to environment variables
	 *	trailing 0 pointer
	 *	and align to integer boundary
	 */
	arg_len += (sizeof(integer_t)
		    + (arg_count + 1 + envc + 1) * sizeof(char *));
	arg_len = (arg_len + sizeof(integer_t) - 1) & ~(sizeof(integer_t)-1);

	/*
	 * Allocate the stack.
	 */
	stack_size = round_page(STACK_SIZE);
	stack_base = VM_MAX_ADDRESS - stack_size;
	(void) vm_allocate(current_task()->map,
			&stack_base,
			stack_size,
			FALSE);

    arg_len = (arg_len + sizeof(int) - 1) & ~(sizeof(int)-1);
    
	arg_pos = (char *)
		set_user_regs(stack_base, stack_size, arg_len);

	/*
	 * Start the strings after the arg-count and pointers
	 */
	string_pos = (arg_pos
		      + sizeof(integer_t)
		      + (arg_count + 1 + envc + 1) * sizeof(char *));

	/*
	 * first the argument count
	 */
	(void) copyout((char *)&arg_count,
			arg_pos,
			sizeof(integer_t));
	arg_pos += sizeof(integer_t);

	/*
	 * Then the strings and string pointers for each argument
	 */
	for (i = 0; i < arg_count; ++i) {
	    arg_ptr = argv[i];
	    arg_item_len = strlen(arg_ptr) + 1; /* include trailing 0 */

	    /* set string pointer */
	    (void) copyout((char *)&string_pos,
			arg_pos,
			sizeof (char *));
	    arg_pos += sizeof(char *);

	    /* copy string */
	    (void) copyout(arg_ptr, string_pos, arg_item_len);
	    string_pos += arg_item_len;
	}

	/*
	 * Null terminator for argv.
	 */
	(void) copyout((char *)&zero, arg_pos, sizeof(char *));
	arg_pos += sizeof(char *);

	/*
	 * Then the strings and string pointers for each environment variable
	 */
	for (i = 0; i < envc; ++i) {
	    arg_ptr = envp[i];
	    arg_item_len = strlen(arg_ptr) + 1; /* include trailing 0 */

	    /* set string pointer */
	    (void) copyout((char *)&string_pos,
			arg_pos,
			sizeof (char *));
	    arg_pos += sizeof(char *);

	    /* copy string */
	    (void) copyout(arg_ptr, string_pos, arg_item_len);
	    string_pos += arg_item_len;
	}

	/*
	 * Null terminator for envp.
	 */
	(void) copyout((char *)&zero, arg_pos, sizeof(char *));
}

struct user_bootstrap_info
{
    vm_offset_t start;
    vm_offset_t size;
  char **argv;
  int done;
  decl_simple_lock_data(,lock)
};

static void
user_bootstrap(void)
{
    struct user_bootstrap_info *info = (struct user_bootstrap_info *) current_thread()->saved.other;
    int err = 0;
    
    exec_load(info->start, info->size);
    //    if (err)
    //    panic ("Cannot load user executable module (error code %d): %s",
    //           err, info->argv[0]);

    exec_map(info->start, info->size);
    //if (err)
    //    panic ("Cannot load user executable module (error code %d): %s",
    //           err, info->argv[0]);
    
    printf ("task loaded:");
  
    {
        //char *argv[] = { "serverboot",
        //                 0 };
        char *envp[] = { 0, 0 };
        
        /* Set up the stack with arguments.  */
        build_args_and_stack(info->argv, envp);
    
        //for (av = info->argv; *av != 0; ++av)
        //    printf (" %s", *av);
    }    

    printf("check1\n");
	/*
	 * set the bootstrap task thread state.
	 */
	thread_setstatus(current_act(),
			 boot_thread_state_flavor,
			 boot_thread_state,
			 boot_thread_state_count);

    printf("check2\n");

    task_suspend (current_task());

    printf("check3\n");

    simple_lock(&info->lock);
    assert (!info->done);
    info->done = 1;
    thread_wakeup ((event_t) info);

	/*
	 * start running user thread.
	 */
	thread_bootstrap_return();
	/*NOTREACHED*/
}

#if 0
static void
user_bootstrap_old(void)
{
	int	i;
	struct region_desc *r;
	vm_offset_t addr;
	vm_object_t object;
	vm_page_t page;
	vm_offset_t off;

	/*
	 * allocate an object and map all of the bootstrap pages into it.
	 */
	object = vm_object_allocate(boot_size);
	for (off = 0; off < boot_size; off += PAGE_SIZE) {
		while ((page = vm_page_grab_fictitious()) == VM_PAGE_NULL)
			vm_page_more_fictitious();
		vm_object_lock(object);
		addr = pmap_extract(kernel_pmap, boot_start + off);

		if (!addr) {
			printf("Warning: bootstrap task has bogus page mapping\n");
			vm_object_unlock(object);
			vm_page_free(page);
			continue;
		}

		vm_page_init(page, addr);
		page->dirty = TRUE;
		page->wire_count = 1;
		vm_page_insert(page, object, off);
		vm_page_lock_queues();		/* unwire will make page active */
		vm_page_unwire(page);
		vm_page_unlock_queues();
		PAGE_WAKEUP_DONE(page);
		vm_object_unlock(object);
	}

	/*
	 * map the bootstrap regions from the object into the user task.
	 * allocate non-mapped regions.
	 */
	r = (struct region_desc *) boot_region_desc;
	for (i = 0; i < boot_region_count; i++, r++) {
		if (r->size == 0)	/* e.g. no BSS */
			continue;
		addr = r->addr;
		if (r->mapped) {
			vm_object_reference(object);
			assert(page_aligned(r->size));
			vm_map_enter(current_task()->map,
				     &addr,
				     r->size,
				     0,
				     FALSE,
				     object,
				     r->offset,
				     FALSE,
				     r->prot,
				     VM_PROT_ALL,
				     VM_INHERIT_DEFAULT);
			/* XXX mapped regions always aligned? */
			assert(addr == r->addr);
#if	MACH_KDB || MACH_KGDB
			if (r->prot & VM_PROT_EXECUTE)
				for (off = 0; off < r->size; off += PAGE_SIZE)
					vm_fault(current_task()->map,
						 addr + off,
						 r->prot,
						 FALSE);
#endif
		} else
			vm_allocate(current_task()->map,
				    &addr,
				    r->size,
				    FALSE);
	}
	vm_object_deallocate(object);

#if	MACH_KDB
#  if defined(__alpha)
        /*
         * Enter the bootstrap symbol table. We need to enter the bootstrap
         * symbol table into the kernel map so that we may access this
         * information when the current task is not the bootstrap task.
         */
        if (use_kdb && stpages && boot_sym_size) {
                vm_offset_t addr;
                vm_map_copy_t copy;

                vm_map_copyin(current_task()->map,
                                boot_sym_start,
                                boot_sym_size,
                                FALSE,
                                &copy);
                vm_map_copyout(kernel_map,
                                &addr,
                                copy);
                boot_sym_start = addr;
                vm_map_wire(kernel_map,
                        boot_sym_start,
                        round_page(boot_sym_start+boot_sym_size),
                        VM_PROT_READ|VM_PROT_WRITE, FALSE);
                X_db_sym_init((char *) boot_sym_start,
                              (char *) boot_sym_start + boot_sym_size,
                              "bootstrap",
                              (char *) current_task()->map);
	}
#  else /* !defined(__alpha) */
	/*
	 * Enter the bootstrap symbol table.
	 */
	if (boot_sym_size)
		X_db_sym_init((char *) boot_sym_start,
			      (char *) boot_sym_start + boot_sym_size,
			      "bootstrap",
			      (char *) current_task()->map);
#  endif /* defined(__alpha) */
#endif	/* MACH_KDB */

#ifdef hp_pa
	/* 
	 * the stack has finally been allocated, we can copy the arguments.
	 */  
	set_bootstrap_args();
#endif

	/*
	 * set the bootstrap task thread state.
	 */
	thread_setstatus(current_act(),
			 boot_thread_state_flavor,
			 boot_thread_state,
			 boot_thread_state_count);

	/*
	 * start running user thread.
	 */
	thread_bootstrap_return();
	/*NOTREACHED*/
}
#endif

kern_return_t
do_bootstrap_ports(
        ipc_port_t bootstrap,
        ipc_port_t *priv_hostp,
        ipc_port_t *priv_devicep,
        ipc_port_t *wired_ledgerp,
        ipc_port_t *paged_ledgerp,
        ipc_port_t *host_securityp)
{
#ifdef	lint
    bootstrap = ipc_port_make_send(realhost.host_priv_self);
#endif	/* lint */

    *priv_hostp = ipc_port_make_send(realhost.host_priv_self);
    *priv_devicep = ipc_port_make_send(master_device_port);
    *wired_ledgerp = ipc_port_make_send(root_wired_ledger_port);
    *paged_ledgerp = ipc_port_make_send(root_paged_ledger_port);
    *host_securityp = ipc_port_make_send(realhost.host_security_self);
    return KERN_SUCCESS;
}

mach_port_t task_insert_send_right( task_t, ipc_port_t); /* forward; */

mach_port_t
task_insert_send_right(
	task_t		task,
	ipc_port_t	port)
{
	mach_port_t	name;

	for (name = 1; ; ++name) {
		kern_return_t kr;

		kr = mach_port_insert_right(task->itk_space, name,
					    port, MACH_MSG_TYPE_PORT_SEND);
		if (kr == KERN_SUCCESS)
			break;
		assert(kr == KERN_NAME_EXISTS);
	}

	return name;
}

kern_return_t
do_bootstrap_arguments(
	ipc_port_t		bootstrap_port,
	task_t			task,
	vm_offset_t		*arguments_ptr,
	mach_msg_type_number_t	*arguments_count)
{
	vm_offset_t args_addr;
	unsigned int args_size;
	kern_return_t kr;
	vm_map_copy_t copy;

#ifdef	lint
	bootstrap_port = (ipc_port_t) 0;
	task->map = VM_MAP_NULL;
#endif	/* lint */

	if (boot_args_size == 0)
		args_size = PAGE_SIZE;
	else
		args_size = round_page(boot_args_size);
	kr = kmem_alloc_pageable(ipc_kernel_map, &args_addr, args_size);
	if (kr != KERN_SUCCESS)
		return kr;
	if (boot_args_size)
		bcopy((char *)boot_args_start, (char *)args_addr,
		      boot_args_size);
	if (args_size != boot_args_size)
		bzero((char *) args_addr + boot_args_size,
		      args_size - boot_args_size);
	kr = vm_map_copyin(ipc_kernel_map, args_addr, args_size, TRUE, &copy);
	assert(kr == KERN_SUCCESS);
	*arguments_ptr = (vm_offset_t) copy;
	*arguments_count = boot_args_size;
	return KERN_SUCCESS;
}


kern_return_t
do_bootstrap_environment(
	ipc_port_t		bootstrap_port,
	task_t			task,
	vm_offset_t		*environment_ptr,
	mach_msg_type_number_t	*environment_count)
{
	vm_offset_t env_addr;
	vm_size_t alloc_size;
	kern_return_t kr;
	vm_map_copy_t copy;

#ifdef	lint
	task->map = VM_MAP_NULL;
	bootstrap_port = (ipc_port_t) 0;
#endif	/* lint */

	if (env_size == 0)
		alloc_size = PAGE_SIZE;
	else
		alloc_size = round_page(env_size);
	kr = kmem_alloc_pageable(ipc_kernel_map, &env_addr, alloc_size);
	if (kr != KERN_SUCCESS)
		return kr;
	if (env_size)
		bcopy((char *)env_start, (char *)env_addr, env_size);
	if (alloc_size != env_size)
		bzero((char *) env_addr + env_size,
		      alloc_size - env_size);
	kr = vm_map_copyin(ipc_kernel_map, env_addr, alloc_size, TRUE, &copy);
	assert(kr == KERN_SUCCESS);
	*environment_ptr = (vm_offset_t) copy;
	*environment_count = env_size;
	return KERN_SUCCESS;
}

kern_return_t 
do_bootstrap_completed(
	ipc_port_t bootstrap_port,
	task_t task)
{
    /* Need do nothing; only the bootstrap task cares when a
       server signals bootstrap_completed.  */
    return KERN_SUCCESS;
}

void	load_info_print(void);

extern struct multiboot_module *mb_module;

void
bootstrap_create(void)
{
    int i, losers, maxlen, err;
    vm_offset_t foobar;
    char *kernel_cmdline = (char *) mb_info.cmdline;
    struct multiboot_module *bmods = ((struct multiboot_module *)
				    boot_start);

    foobar = bmods->mod_start;
    printf("mb_info: 0x%8x\n", &mb_info);
    printf("mb_info.count: 0x%8x\n", &mb_info.mods_count);
    printf("mb_info.mods_addr: 0x%8x\n", &mb_info.mods_addr);
    printf("mb_module: 0x%8x\n", mb_module);
    printf("mb_module->mod_start: 0x%8x\n", mb_module->mod_start);
    printf("string: 0x%8x\n", bmods[0].string);
    printf("mod_start: 0x%8x\n", bmods[0].mod_start);
    printf("boot_start: 0x%8x\n", boot_start);
    printf("boot_args_start: 0x%8x\n", boot_args_start);
    printf("foobar: 0x%8x\n", foobar);
    printf("mods_count: %d\n", mb_info.mods_count);

    if (/*(mb_info.flags & MULTIBOOT_MODS)
          ||*/ (mb_info.mods_count == 0))
        panic ("No bootstrap code loaded with the kernel!");  

    /* Initialize boot script variables.  We leak these send rights.  */
    losers = boot_script_set_variable
        ("host-port", VAL_PORT,
         (int)ipc_port_make_send(realhost.host_priv_self));
    if (losers)
        panic ("cannot set boot-script variable host-port: %s",
               boot_script_error_string (losers));
    losers = boot_script_set_variable
        ("device-port", VAL_PORT,
         (int) ipc_port_make_send(master_device_port));
    if (losers)
        panic ("cannot set boot-script variable device-port: %s",
               boot_script_error_string (losers));
    
    losers = boot_script_set_variable ("kernel-command-line", VAL_STR,
                                       (int) kernel_cmdline);
    if (losers)
        panic ("cannot set boot-script variable %s: %s",
               "kernel-command-line", boot_script_error_string (losers));
    
    {
        /* Turn each `FOO=BAR' word in the command line into a boot script
           variable ${FOO} with value BAR.  This matches what we get from
           oskit's environ in the oskit-mach case (above).  */
        /*
        int len = strlen (kernel_cmdline) + 1;
        char *s = memcpy (alloca (len), kernel_cmdline, len);
        char *word;
        while ((word = strsep (&s, " \t")) != 0)
        {
            char *eq = strchr (word, '=');
            if (eq == 0)
                continue;
            *eq++ = '\0';
            losers = boot_script_set_variable (word, VAL_STR, (int) eq);
            if (losers)
                panic ("cannot set boot-script variable %s: %s",
                       word, boot_script_error_string (losers));
        }
        */
    }


    maxlen = 0;
    /*
    for (i = 0; i < mb_info.mods_count; ++i)
	{
        int err;
        char *line = (char*)phystokv(bmods[i].string);
        int len = strlen (line) + 1;
        if (len > maxlen)
            maxlen = len;
        printf ("Hola\n");
        printf ("module %d: %s\n", i, line);
        err = boot_script_parse_line (&bmods[i], line);
        if (err)
	    {
            printf ("\n\tERROR: %s", boot_script_error_string (err));
            ++losers;
	    }
    }
    printf ("\r%d multiboot modules %*s", i, -maxlen, "");
    if (losers)
        panic ("%d of %d boot script commands could not be parsed",
               losers, mb_info.mods_count);
    */

    err = boot_script_parse_line (boot_start, boot_size, "ext2fs.static --multiboot-command-line=root=/dev/hd2s2 --host-priv-port=${host-port} --device-master-port=${device-port} --exec-server-task=${exec-task} -T typed device:hd2s2 $(task-create) $(task-resume)");
    err = boot_script_parse_line (exec_start, exec_size, "exec.static $(exec-task=task-create)");
    if (err)
    {
        printf ("\n\tERROR: %s", boot_script_error_string (err));
        ++losers;
    }
    
    losers = boot_script_exec ();
    if (losers)
        panic ("ERROR in executing boot script: %s",
               boot_script_error_string (losers));

    /* XXX at this point, we could free all the memory used
       by the boot modules and the boot loader's descriptors and such.  */
}

#if 0
void
bootstrap_create_old(void)
{
	ipc_port_t	master_bootstrap_port;
	task_t		bootstrap_task;
	thread_act_t	bootstrap_thr_act;
	kern_return_t	kr;
	ipc_port_t	root_device_port;

#if	DIPC
	if (no_bootstrap_task())
		return;
#endif	/* DIPC */

	/*
	 * Allocate the master bootstrap port;
	 */
	master_bootstrap_port = ipc_port_alloc_kernel();
	if (master_bootstrap_port == IP_NULL)
		panic("can't allocate master bootstrap port");

	/*
	 * Create the bootstrap task.
	 */
	if (boot_size == 0) {
		printf("Not starting bootstrap task.\n");
		return;
	}

#if	SYS_REBOOT_COMPAT
	do_bootstrap_compat();
#endif

	task_create_local(TASK_NULL, FALSE, FALSE, &bootstrap_task);
	thread_create(bootstrap_task, &bootstrap_thr_act);

	task_set_special_port(bootstrap_task,
			      TASK_BOOTSTRAP_PORT,
			      ipc_port_make_send(master_bootstrap_port));

	/*
	 * Start the bootstrap thread.
	 */
	thread_start(bootstrap_thr_act->thread, user_bootstrap);
	kr = thread_resume(bootstrap_thr_act);
	assert( kr == KERN_SUCCESS );
}
#endif

#if	DEBUG
void
load_info_print(void)
{
	struct loader_info *lp = (struct loader_info *)load_info_start;

	printf("Load info: text (%#x, %#x, %#x)\n",
		lp->text_start, lp->text_size, lp->text_offset);
	printf("           data (%#x, %#x, %#x)\n",
		lp->data_start, lp->data_size, lp->data_offset);
	printf("           bss  (%#x)\n", lp->bss_size);
	printf("           syms (%#x, %#x)\n",
		lp->sym_offset, lp->sym_size);
	printf("	   entry(%#x, %#x)\n",
		lp->entry_1, lp->entry_2);
}
#endif

/*
 *	Moves kernel symbol table, bootstrap image, and bootstrap
 *	load information out of BSS at startup.  Returns the
 *	first unused address.
 *
 *	PAGE_SIZE must be set.
 *
 *	On some machines, this code must run before the page
 *	tables are set up, and therefore must be re-created
 *	in assembly language.
 */

void
ovbcopy_ints(
	vm_offset_t	src,
	vm_offset_t	dst,
	vm_size_t	size)
{
	register int *srcp;
	register int *dstp;
	register unsigned int count;

	srcp = (int *)(src + size);
	dstp = (int *)(dst + size);
	count = size / sizeof(int);

	while (count-- != 0)
	    *--dstp = *--srcp;
}

#ifdef hp_pa
extern char edata;
extern char end;
#else
extern char	edata[];	/* start of BSS */
extern char	end[];		/* end of BSS */
#endif

vm_offset_t
move_bootstrap(void)
{
#ifdef hp_pa
#ifdef __ELF__
	struct boot_info *bi = (struct boot_info *)&edata;
#else
	struct boot_info *bi = (struct boot_info *)round_page(&edata);
#endif
	kern_sym_start = (vm_offset_t) &end;
#else
	struct boot_info *bi = (struct boot_info *)edata;

	kern_sym_start = (vm_offset_t) end;
#endif

	kern_sym_size  = bi->sym_size;
	/*
	 * Align start of bootstrap on page boundary,
	 * to allow mapping into user space.
	 */
	boot_start = round_page(kern_sym_start + kern_sym_size);
	boot_size  = bi->boot_size;
	load_info_start = boot_start + boot_size;
	load_info_size  = bi->load_info_size;

#if DEBUG && 0
	load_info_print();
#endif

	ovbcopy_ints((vm_offset_t)bi + sizeof(struct boot_info) + kern_sym_size,
		     boot_start,
		     boot_size + load_info_size);

	ovbcopy_ints((vm_offset_t)bi + sizeof(struct boot_info),
		     kern_sym_start,
		     kern_sym_size);

	return boot_start + boot_size + load_info_size;
}

void *
boot_script_malloc (unsigned int size)
{
  return (void *) kalloc (size);
}

void
boot_script_free (void *ptr, unsigned int size)
{
  kfree ((vm_offset_t)ptr, size);
}

int
boot_script_task_create (struct cmd *cmd)
{
    kern_return_t rc = task_create_local(TASK_NULL, FALSE, FALSE, &cmd->task);
    printf("boot_script_task_create\n");
  if (rc)
    {
      printf("boot_script_task_create failed with %x\n", rc);
      return BOOT_SCRIPT_MACH_ERROR;
    }
  return 0;
}

int
boot_script_task_resume (struct cmd *cmd)
{
    kern_return_t rc;
    printf("task_resume entry\n");
    rc = task_resume (cmd->task);
    printf("after task resume\n");
  if (rc)
    {
      printf("boot_script_task_resume failed with %x\n", rc);
      return BOOT_SCRIPT_MACH_ERROR;
    }
  printf ("\nstart %s: ", cmd->path);
  return 0;
}

int
boot_script_prompt_task_resume (struct cmd *cmd)
{
  char xx[5];

  printf ("Hit return to resume %s...", cmd->path);
  safe_gets (xx, sizeof xx);

  return boot_script_task_resume (cmd);
}

void
boot_script_free_task (task_t task, int aborting)
{
  if (aborting)
    task_terminate (task);
}

int
boot_script_insert_right (struct cmd *cmd, mach_port_t port, mach_port_t *name)
{
  *name = task_insert_send_right (cmd->task, (ipc_port_t)port);
  return 0;
}

int
boot_script_insert_task_port (struct cmd *cmd, task_t task, mach_port_t *name)
{
  *name = task_insert_send_right (cmd->task, task->itk_sself);
  return 0;
}

struct user_bootstrap_info info;

int
boot_script_exec_cmd (vm_offset_t start, vm_size_t size, task_t task, char *path, int argc,
		      char **argv, char *strings, int stringlen)
{
  thread_act_t thread_act;
  ipc_port_t master_bootstrap_port;
  int err;
  int i;

  info.start = start;
  info.size = size;
  info.argv = argv;
  info.done = 0;

  for (i=0; i < argc; ++i) {
      printf("argv[%d]: %s\n", i, argv[i]);
  }

  if (task != MACH_PORT_NULL)
    {
        simple_lock_init (&info.lock, ETAP_NO_TRACE);
        simple_lock(&info.lock);

      master_bootstrap_port = ipc_port_alloc_kernel();
      if (master_bootstrap_port == IP_NULL)
          panic("can't allocate master bootstrap port");

      err = thread_create ((task_t)task, &thread_act);
      assert(err == 0);

      task_set_special_port(task,
                            TASK_BOOTSTRAP_PORT,
                            ipc_port_make_send(master_bootstrap_port));
      
      thread_act->thread->saved.other = (char *) &info;
      thread_start(thread_act->thread, user_bootstrap);
      err = thread_resume(thread_act);
      assert(err == 0);

      /* We need to synchronize with the new thread and block this
	 main thread until it has finished referring to our local state.  */
      while (! info.done)
	{
	  thread_sleep_simple_lock((event_t) &info, simple_lock_addr(info.lock), FALSE);
	}
      printf("\n");
    }


  return 0;
}
