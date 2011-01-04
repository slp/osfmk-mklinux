/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <mach/mach_interface.h>
#include <mach/mach_host.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>
#include <osfmach3/mm.h>

#include <linux/sched.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/malloc.h>

static int osfmach3_move_page_tables(
	struct mm_struct	*mm,
	struct vm_area_struct	*vma,
	unsigned long		new_addr,
	unsigned long		old_addr,
	unsigned long		len)
{
	kern_return_t	kr;
	vm_address_t	new_mach_addr;
	vm_prot_t	cur_prot, max_prot, expected_prot;
	vm_inherit_t	inheritance;
	unsigned short	vm_flags;

	vm_flags = vma->vm_flags;
	expected_prot = VM_PROT_NONE;
	if (vm_flags & VM_READ)
		expected_prot |= VM_PROT_READ;
	if (vm_flags & VM_WRITE)
		expected_prot |= VM_PROT_WRITE;
	if (vm_flags & VM_EXEC)
		expected_prot |= VM_PROT_EXECUTE;
	if ((vm_flags & (VM_SHARED|VM_WRITE)) != VM_WRITE)
		inheritance = VM_INHERIT_SHARE;
	else
		inheritance = VM_INHERIT_COPY;
	new_mach_addr = (vm_address_t) new_addr;

	kr = vm_remap(mm->mm_mach_task->mach_task_port,	/* target task */
		      &new_mach_addr,			/* target address */
		      (vm_size_t) len,			/* size */
		      (vm_address_t) 0,			/* page mask */
		      FALSE,				/* anywhere */
		      mm->mm_mach_task->mach_task_port,	/* source task */
		      (vm_address_t) old_addr,		/* source address */
		      FALSE,				/* copy */
		      &cur_prot,			/* cur_protection */
		      &max_prot,			/* max_protection */
		      inheritance);			/* inheritance */
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_move_page_tables: vm_remap"));
		return -1;
	}
	ASSERT((cur_prot & expected_prot) == expected_prot);
	return 0;
}

static unsigned long move_vma(struct vm_area_struct * vma, unsigned long addr, unsigned long old_len, unsigned long new_len)
{
	struct vm_area_struct	*new_vma, *extra_vma;

	new_vma = (struct vm_area_struct *)
		kmalloc(sizeof (struct vm_area_struct), GFP_KERNEL);
	if (new_vma) {
		unsigned long new_addr = get_unmapped_area(addr, new_len);

		if (new_addr && !osfmach3_move_page_tables(current->mm, vma, new_addr, addr, old_len)) {
#ifdef	CONFIG_OSFMACH3
			if (new_len != old_len) {
				/* Establish the extra Mach VM mapping */
				extra_vma = (struct vm_area_struct *)
					kmalloc(sizeof (struct vm_area_struct),
						GFP_KERNEL);
				if (!extra_vma) {
					kfree(new_vma);
					return -ENOMEM;
				}
				*extra_vma = *vma;
				extra_vma->vm_start = new_addr + old_len;
				extra_vma->vm_end = new_addr + new_len;
				extra_vma->vm_offset += (old_len +
							 (addr -
							  vma->vm_start));
				if (extra_vma->vm_inode)
					extra_vma->vm_inode->i_count++;
				if (extra_vma->vm_ops &&
				    extra_vma->vm_ops->open)
					extra_vma->vm_ops->open(extra_vma);
				insert_vm_struct(current->mm, extra_vma);
				/*
				 * insert_vm_struct() has taken an extra ref
				 * on the memory object.
				 */
			}
#endif	/* CONFIG_OSFMACH3 */

			*new_vma = *vma;
			new_vma->vm_start = new_addr;
#ifdef	CONFIG_OSFMACH3
			new_vma->vm_end = new_addr + old_len;
#else	/* CONFIG_OSFMACH3 */
			new_vma->vm_end = new_addr + new_len;
#endif	/* CONFIG_OSFMACH3 */
			new_vma->vm_offset = vma->vm_offset + (addr - vma->vm_start);
			if (new_vma->vm_inode)
				new_vma->vm_inode->i_count++;
			if (new_vma->vm_ops && new_vma->vm_ops->open)
				new_vma->vm_ops->open(new_vma);
#ifdef	CONFIG_OSFMACH3
			new_vma->vm_flags |= VM_REMAPPING;	/* XXX */
			insert_vm_struct(current->mm, new_vma);
			new_vma->vm_flags &= ~VM_REMAPPING;	/* XXX */
			/* take an extra ref on the mem_obj */
			if (new_vma->vm_inode)
				inode_pager_setup(new_vma->vm_inode);
			merge_segments(current->mm, new_addr, new_addr+new_len);
#else	/* CONFIG_OSFMACH3 */
			insert_vm_struct(current->mm, new_vma);
			merge_segments(current->mm, new_vma->vm_start, new_vma->vm_end);
#endif	/* CONFIG_OSFMACH3 */
			do_munmap(addr, old_len);
			current->mm->total_vm += new_len >> PAGE_SHIFT;
			return new_addr;
		}
		kfree(new_vma);
	}
	return -ENOMEM;
}

unsigned long
osfmach3_mremap(
	struct vm_area_struct	*vma,
	unsigned long		addr,
	unsigned long		old_len,
	unsigned long		new_len,
	unsigned long		flags)
{
#ifdef	CONFIG_OSFMACH3
	struct vm_area_struct 	extra_vma;

	ASSERT(! (!vma || vma->vm_start > addr));
	ASSERT(! (old_len > vma->vm_end - addr));
#endif	/* CONFIG_OSFMACH3 */

	/* old_len exactly to the end of the area.. */
	if (old_len == vma->vm_end - addr &&
	    (old_len != new_len || !(flags & MREMAP_MAYMOVE))) {
		unsigned long max_addr = TASK_SIZE;
		if (vma->vm_next)
			max_addr = vma->vm_next->vm_start;
		/* can we just expand the current mapping? */
		if (max_addr - addr >= new_len) {
			int pages = (new_len - old_len) >> PAGE_SHIFT;

#ifdef	CONFIG_OSFMACH3
			/*
			 * Establish the extra Mach VM mapping
			 */
			extra_vma = *vma;
			extra_vma.vm_start = addr + old_len;
			extra_vma.vm_end = addr + new_len;
			extra_vma.vm_offset += (extra_vma.vm_start
						- vma->vm_start);
			/*
			 * osfmach3_insert_vm_struct() will take an 
			 * extra reference on the associated memory
			 * object (if any).
			 */
			osfmach3_insert_vm_struct(current->mm, &extra_vma);
#endif	/* CONFIG_OSFMACH3 */

			vma->vm_end = addr + new_len;
			current->mm->total_vm += pages;
			if (vma->vm_flags & VM_LOCKED)
				current->mm->locked_vm += pages;
			return addr;
		}
	}

	/*
	 * We weren't able to just expand or shrink the area,
	 * we need to create a new one and move it..
	 */
	if (flags & MREMAP_MAYMOVE)
		return move_vma(vma, addr, old_len, new_len);
	return -ENOMEM;
}
