/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/vm_prot.h>
#include <mach/mach_host.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/uniproc.h>

#include <linux/sched.h>
#include <linux/mm.h>

void
osfmach3_mlock_fixup(
	struct vm_area_struct	*vma,
	unsigned int		newflags)
{
	kern_return_t	kr;
	vm_prot_t	mach_prot;
	struct osfmach3_mach_task_struct *mach_task;
	extern mach_port_t privileged_host_port;

	if ((newflags & VM_LOCKED) == (vma->vm_flags & VM_LOCKED)) {
		/* no change in wiring */
		return;
	}

	mach_prot = VM_PROT_NONE;
	if (newflags & VM_LOCKED) {
		if (newflags & VM_READ)
			mach_prot |= VM_PROT_READ;
		if (newflags & VM_WRITE)
			mach_prot |= VM_PROT_WRITE;
		if (newflags & VM_EXEC)
			mach_prot |= VM_PROT_EXECUTE;
	}

	mach_task = vma->vm_mm->mm_mach_task;
	server_thread_blocking(FALSE);
	kr = vm_wire(privileged_host_port,
		     mach_task->mach_task_port,
		     (vm_offset_t) vma->vm_start,
		     (vm_size_t) (vma->vm_end - vma->vm_start),
		     mach_prot);
	server_thread_unblocking(FALSE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_mprotect_fixup: vm_wire"));
	}
}
