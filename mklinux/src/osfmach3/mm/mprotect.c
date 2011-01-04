/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/vm_prot.h>
#include <mach/mach_interface.h>

#include <osfmach3/mach3_debug.h>

#include <linux/sched.h>
#include <linux/mm.h>

void
osfmach3_mprotect_fixup(
	struct vm_area_struct	*vma,
	unsigned long		start,
	unsigned long		end,
	unsigned int		newflags)
{
	kern_return_t	kr;
	vm_prot_t	mach_prot;
	struct osfmach3_mach_task_struct *mach_task;
	
	mach_prot = VM_PROT_NONE;
	if (newflags & VM_READ)
		mach_prot |= VM_PROT_READ;
	if (newflags & VM_WRITE)
		mach_prot |= VM_PROT_WRITE;
	if (newflags & VM_EXEC)
		mach_prot |= VM_PROT_EXECUTE;

	mach_task = vma->vm_mm->mm_mach_task;
	kr = vm_protect(mach_task->mach_task_port,
			(vm_offset_t) start,
			(vm_size_t) end - start,
			FALSE,
			mach_prot);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_mprotect_fixup: vm_protect"));
	}

	/*
	 * Flush the copyin/copyout cache.
	 */
	user_memory_flush_area(mach_task,
			       (vm_address_t) start,
			       (vm_address_t) end);
}
