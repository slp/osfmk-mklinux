/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/config.h>

#include <mach/mach_interface.h>
#include <mach/mach_host.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/user_memory.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/stat.h>
#include <linux/malloc.h>
#include <linux/resource.h>
#include <linux/shm.h>
#include <linux/ipc.h>
#include <linux/swap.h>
#include <linux/mman.h>

#include <asm/pgtable.h>

#if	CONFIG_OSFMACH3_DEBUG
#define VMA_DEBUG	1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	VMA_DEBUG
int vma_debug = 0;
#endif	/* VMA_DEBUG */

void
osfmach3_exit_mmap(
	struct mm_struct * mm)
{
	struct osfmach3_mach_task_struct *mach_task;
	kern_return_t	kr;

	mach_task = mm->mm_mach_task;

	user_memory_flush_task(mach_task);

	/* flush the memory out of the kernel cache */
	server_thread_blocking(FALSE);
	kr = vm_msync(mach_task->mach_task_port,
		      VM_MIN_ADDRESS,
		      VM_MAX_ADDRESS - VM_MIN_ADDRESS,
		      VM_SYNC_SYNCHRONOUS);
	server_thread_unblocking(FALSE);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			MACH3_DEBUG(1, kr, ("osfmach3_exit_mmap: vm_msync"));
		}
	}

	kr = vm_deallocate(mach_task->mach_task_port,
			   VM_MIN_ADDRESS,
			   VM_MAX_ADDRESS - VM_MIN_ADDRESS);
	if (kr != KERN_SUCCESS) {
		if (kr != MACH_SEND_INVALID_DEST &&
		    kr != KERN_INVALID_ARGUMENT) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_exit_mmap: vm_deallocate"));
		}
	}
}

void
osfmach3_insert_vm_struct(
	struct mm_struct 	*mm,
	struct vm_area_struct	*vmp)
{
	memory_object_t		mem_obj;
	vm_offset_t		mem_obj_offset;
	kern_return_t		kr;
	unsigned short		vm_flags;
	boolean_t		is_shared;
	vm_prot_t		cur_prot, max_prot;
	vm_address_t		user_addr, wanted_addr;
	vm_size_t		size;
	unsigned int		id;
	struct shmid_ds		*shp;
	struct osfmach3_mach_task_struct *mach_task;
	extern struct shmid_ds	*shm_segs[SHMMNI];

	if (vmp->vm_flags & VM_REMAPPING) {
		/* don't mess with Mach VM: it's only Linux remappings */
		return;
	}

#ifdef	VMA_DEBUG
	if (vma_debug) {
		printk("VMA:osfmach3_insert_vm_struct: mm=0x%p, vmp=0x%p\n",
		       mm, vmp);
	}
#endif	/* VMA_DEBUG */

	mach_task = mm->mm_mach_task;
	if (vmp->vm_inode == NULL) {
		if (vmp->vm_pte != 0) {
			/* shared memory */
			id = SWP_OFFSET(vmp->vm_pte) & SHM_ID_MASK;
			shp = shm_segs[id];
			if (shp != IPC_UNUSED) {
				mem_obj = (mach_port_t) shp->shm_pages;
				mem_obj_offset = 0;
			} else {
				mem_obj = MEMORY_OBJECT_NULL;
				mem_obj_offset = 0;
			}
		} else {
			mem_obj = MEMORY_OBJECT_NULL;
			mem_obj_offset = 0;
		}
	} else if (S_ISREG(vmp->vm_inode->i_mode)) {
		mem_obj = inode_pager_setup(vmp->vm_inode);
		if (mem_obj == MEMORY_OBJECT_NULL) {
			panic("osfmach3_insert_vm_struct: can't setup pager");
		}
		mem_obj_offset = (vm_offset_t) vmp->vm_offset;
	} else if (vmp->vm_inode->i_mem_object != NULL) {
		/* special file, but with a pager already setup */
		mem_obj = vmp->vm_inode->i_mem_object->imo_mem_obj;
		mem_obj_offset = (vm_offset_t) vmp->vm_offset;
	} else {
		panic("osfmach3_insert_vm_struct: non-regular file");
	}

	vm_flags = vmp->vm_flags;
	cur_prot = VM_PROT_NONE;
	if (vm_flags & VM_READ)
		cur_prot |= VM_PROT_READ;
	if (vm_flags & VM_WRITE)
		cur_prot |= VM_PROT_WRITE;
	if (vm_flags & VM_EXEC)
		cur_prot |= VM_PROT_EXECUTE;
	max_prot = VM_PROT_ALL;
	is_shared = (vmp->vm_flags & VM_SHARED) != 0;
	user_addr = vmp->vm_start;
	wanted_addr = user_addr;
	size = vmp->vm_end - vmp->vm_start;

#ifdef	VMA_DEBUG
	if (vma_debug) {
		printk("VMA: vm_map(task=0x%x, user_addr=0x%x, size=0x%x, "
		       "mem_obj=0x%x, offset=0x%x, %sCOPY, cur_prot=0x%x, "
		       "max_prot=0x%x, %s)\n",
		       mach_task->mach_task_port,
		       user_addr,
		       size,
		       mem_obj,
		       mem_obj_offset,
		       is_shared ? "!" : "",
		       cur_prot,
		       max_prot,
		       is_shared ? "INHERIT_SHARE" : "INHERIT_COPY");
	}
#endif	/* VMA_DEBUG */
	
	server_thread_blocking(FALSE);
	kr = vm_map(mach_task->mach_task_port,
		    &user_addr,
		    size,
		    0,		/* no mask */
		    FALSE,	/* not anywhere */
		    mem_obj,
		    mem_obj_offset,
		    !is_shared,
		    cur_prot,
		    max_prot,
		    is_shared ? VM_INHERIT_SHARE : VM_INHERIT_COPY);
	server_thread_unblocking(FALSE);

	if (kr != KERN_SUCCESS) {
		printk("Failure: vm_map(task=0x%x, user_addr=0x%x, size=0x%x, "
		       "mem_obj=0x%x, offset=0x%x, %sCOPY, cur_prot=0x%x, "
		       "max_prot=0x%x, %s)\n",
		       mach_task->mach_task_port,
		       user_addr,
		       size,
		       mem_obj,
		       mem_obj_offset,
		       is_shared ? "!" : "",
		       cur_prot,
		       max_prot,
		       is_shared ? "INHERIT_SHARE" : "INHERIT_COPY");
		MACH3_DEBUG(1, kr, ("osfmach3_insert_vm_struct: vm_map"));
		printk("osfmach3_insert_vm_struct: can't map\n");
	}
	if (user_addr != wanted_addr) {
		printk("vm_map: mapped at 0x%x instead of 0x%x\n",
		       user_addr, wanted_addr);
		printk("osfmach3_insert_vm_struct: mapping at wrong address\n");
	}

	if (vmp->vm_flags & VM_LOCKED) {
		extern mach_port_t privileged_host_port;

		server_thread_blocking(FALSE);
		kr = vm_wire(privileged_host_port,
			     mach_task->mach_task_port,
			     user_addr,
			     size,
			     cur_prot);
		server_thread_unblocking(FALSE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("osfmach3_insert_vm_struct: "
				     "vm_wire(task=0x%x, addr=0x%x, size=0x%x, "
				     "prot=0x%x)",
				     mach_task->mach_task_port,
				     user_addr,
				     size,
				     cur_prot));
			printk("osfmach3_insert_vm_struct: vm_wire failed\n");
		}
	}
#if 0
	if (vmp->vm_inode != NULL) {
		/*
		 * If mem_obj was already cached in the kernel, we got an
		 * extra reference on its i_mem_object structure (inode_pager).
		 * If it was the first time we mapped the inode, the memory
		 * object has just been initialized by the kernel and we
		 * got a reference in memory_object_init(). In both cases,
		 * we have to release a reference.
		 */
		ASSERT(mem_obj != MEMORY_OBJECT_NULL);
		ASSERT(vmp->vm_inode->i_mem_object);
		ASSERT(vmp->vm_inode->i_mem_object->imo_mem_obj_control);
		inode_pager_release(vmp->vm_inode);
	}
#endif
}

void
osfmach3_remove_shared_vm_struct(
	struct vm_area_struct	*mpnt)
{
	struct osfmach3_mach_task_struct	*mach_task;
	struct mm_struct	*mm;
	kern_return_t		kr;
	struct vm_area_struct	*vmp;

#ifdef	VMA_DEBUG
	if (vma_debug) {
		printk("VMA:osfmach3_remove_shared_vm_struct: mpnt=0x%p\n",
		       mpnt);
	}
#endif	/* VMA_DEBUG */

	mm = mpnt->vm_mm;
	mach_task = mm->mm_mach_task;
	ASSERT(mach_task == current->osfmach3.task);

	vmp = find_vma(mm, mpnt->vm_start);
	if (vmp != NULL && vmp != mpnt && vmp->vm_start <= mpnt->vm_start) {
		/*
		 * There's another vm_area overlapping the removed one...
		 * This removal is probably the result of a 
		 * merge_segments(): that doesn't change anything to
		 * the VM layout.
		 */
		return;
	}

	if (mpnt->vm_inode) {
		/* mapped file: release a reference on the mem_object */
		inode_pager_release(mpnt->vm_inode);
	}

	if (mm->mmap == NULL) {
		/*
		 * osfmach3_exit_mmap was called before and
		 * cleaned the entire address space...
		 */
		return;
	}
#ifdef	VMA_DEBUG
	if (vma_debug) {
		printk("VMA: vm_deallocate(0x%x, 0x%lx, 0x%lx)\n",
		       mach_task->mach_task_port,
		       mpnt->vm_start,
		       mpnt->vm_end - mpnt->vm_start);
	}
#endif	/* VMA_DEBUG */

	if (mpnt->vm_inode) {
		/* mapped file: flush the object out of the cache */
		server_thread_blocking(FALSE);
		kr = vm_msync(mach_task->mach_task_port,
			      (vm_address_t) mpnt->vm_start,
			      (vm_size_t) (mpnt->vm_end - mpnt->vm_start),
			      VM_SYNC_SYNCHRONOUS);
		server_thread_unblocking(FALSE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_remove_share_vm_struct: "
				     "vm_msync"));
		}
	}

	kr = vm_deallocate(mach_task->mach_task_port,
			   (vm_address_t) mpnt->vm_start,
			   (vm_size_t) (mpnt->vm_end - mpnt->vm_start));
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_remove_share_vm_struct: vm_deallocate"));
		printk("osfmach3_remove_shared_vm_struct: can't deallocate\n");
	}
	/*
	 * Flush the copyin/copyout cache.	
	 */
	user_memory_flush_area(mach_task,
			       (vm_address_t) mpnt->vm_start,
			       (vm_address_t) mpnt->vm_end);
}

extern void avl_neighbours (struct vm_area_struct * node, struct vm_area_struct * tree, struct vm_area_struct ** to_the_left, struct vm_area_struct ** to_the_right);
extern void avl_insert (struct vm_area_struct * new_node, struct vm_area_struct ** ptree);

int
osfmach3_split_vm_struct(
	unsigned long	addr,
	size_t		len)
{
	struct mm_struct	*mm;
	struct vm_area_struct	*vmp, *prev, *next, *new_vmp;
	unsigned long		start, end;

	mm = current->mm;
	start = addr;
	end = addr + len;

	for (vmp = find_vma(mm, addr);
	     vmp && vmp->vm_start < end;
	     vmp = vmp->vm_next) {
		avl_neighbours(vmp, mm->mmap_avl, &prev, &next);

#ifdef	VMA_DEBUG
		if (vma_debug) {
			printk("VMA: split(addr=%lx,len=%lx) "
			       "vmp=(s=%lx,e=%lx,o=%lx)\n",
			       addr, (unsigned long) len,
			       vmp->vm_start,
			       vmp->vm_end,
			       vmp->vm_offset);
		}
#endif	/* VAM_DEBUG */

		if (vmp->vm_start < start) {
			/*
			 * Need to split here.
			 */
			new_vmp = (struct vm_area_struct *)
				kmalloc(sizeof *new_vmp, GFP_KERNEL);
			if (!new_vmp) {
				printk("osfmach3_split_vm_struct: no memory\n");
				return -ENOMEM;
			}

			*new_vmp = *vmp;
			if (new_vmp->vm_inode) {
				struct vm_area_struct *share;

				new_vmp->vm_inode->i_count++;
				share = vmp->vm_inode->i_mmap;
				ASSERT(share);
				new_vmp->vm_next_share = share->vm_next_share;
				new_vmp->vm_next_share->vm_prev_share = new_vmp;
				share->vm_next_share = new_vmp;
				new_vmp->vm_prev_share = share;
				/* take an extra reference on the mem_obj */
				inode_pager_setup(new_vmp->vm_inode);
			}
			if (new_vmp->vm_ops && new_vmp->vm_ops->open) {
				new_vmp->vm_end = new_vmp->vm_start; /* XXX ? */
				new_vmp->vm_ops->open(new_vmp);
			}
			new_vmp->vm_end = start;
			new_vmp->vm_next = vmp;
			vmp->vm_offset += start - vmp->vm_start;
			vmp->vm_start = start;
			if (prev)
				prev->vm_next = new_vmp;
			else
				current->mm->mmap = new_vmp;
			avl_insert(new_vmp, &mm->mmap_avl);
#ifdef	VMA_DEBUG
			if (vma_debug) {
				printk("VMA: split: "
				       "new_vmp(s=%lx,e=%lx,o=%lx) -> "
				       "vmp(s=%lx,e=%lx,o=%lx)\n",
				       new_vmp->vm_start,
				       new_vmp->vm_end,
				       new_vmp->vm_offset,
				       vmp->vm_start,
				       vmp->vm_end,
				       vmp->vm_offset);
			}
#endif	/* VAM_DEBUG */
		}

		if (vmp->vm_end > end) {
			/*
			 * Need to split here.
			 */
			new_vmp = (struct vm_area_struct *)
				kmalloc(sizeof *new_vmp, GFP_KERNEL);
			if (!new_vmp) {
				printk("osfmach3_split_vm_struct: no memory\n");
				return -ENOMEM;
			}

			*new_vmp = *vmp;
			if (new_vmp->vm_inode) {
				struct vm_area_struct *share;

				new_vmp->vm_inode->i_count++;
				share = vmp->vm_inode->i_mmap;
				ASSERT(share);
				new_vmp->vm_next_share = share->vm_next_share;
				new_vmp->vm_next_share->vm_prev_share = new_vmp;
				share->vm_next_share = new_vmp;
				new_vmp->vm_prev_share = share;
				/* take an extra reference on the mem_obj */
				inode_pager_setup(new_vmp->vm_inode);
			}
			if (new_vmp->vm_ops && new_vmp->vm_ops->open) {
				new_vmp->vm_start = new_vmp->vm_end;
				new_vmp->vm_offset = vmp->vm_offset + 
					(vmp->vm_end - vmp->vm_start);
				new_vmp->vm_ops->open(new_vmp);
			}
			new_vmp->vm_start = end;
			new_vmp->vm_offset =
				vmp->vm_offset + start - vmp->vm_start;
			vmp->vm_end = end;
			vmp->vm_next = new_vmp;
			avl_insert(new_vmp, &mm->mmap_avl);
#ifdef	VMA_DEBUG
			if (vma_debug) {
				printk("VMA: split: "
				       "vmp(s=%lx,e=%lx,o=%lx) -> "
				       "new_vmp(s=%lx,e=%lx,o=%lx)\n",
				       vmp->vm_start,
				       vmp->vm_end,
				       vmp->vm_offset,
				       new_vmp->vm_start,
				       new_vmp->vm_end,
				       new_vmp->vm_offset);
			}
#endif	/* VAM_DEBUG */
			vmp = new_vmp;
		}
	}

	return 0;
}

int expand_stack(struct vm_area_struct * vma, unsigned long address)
{
	unsigned long grow;
#if 0
	vm_address_t mach_addr;
	kern_return_t kr;
#else
	struct vm_area_struct *new_vma;
#endif

#ifdef STACK_GROWTH_UP
	unsigned long top;

	top = PAGE_MASK & (address + PAGE_SIZE);
	address = vma->vm_end;
	grow = top - address;
	if (top - vma->vm_start 
	    > (unsigned long) current->rlim[RLIMIT_STACK].rlim_cur)
		return -ENOMEM;
#else
	address &= PAGE_MASK;
	grow = vma->vm_start - address;
	if (vma->vm_end - address
	    > (unsigned long) current->rlim[RLIMIT_STACK].rlim_cur ||
	    (vma->vm_mm->total_vm << PAGE_SHIFT) + grow
	    > (unsigned long) current->rlim[RLIMIT_AS].rlim_cur)
		return -ENOMEM;
#endif /* STACK_GROWTH_UP */

#if	VMA_DEBUG
	if (vma_debug) {
		printk("expand_stack(vma=%p): "
		       "address=0x%lx, grow=0x%lx\n",
		       vma, address, grow);
	}
#endif	/* VMA_DEBUG */

#if 0
	mach_addr = (vm_address_t) address;
	kr = vm_allocate(current->osfmach3.task->mach_task_port,
			 &mach_addr,
			 (vm_size_t) grow,
			 FALSE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("expand_stack: vm_allocate(0x%x,0x%x)",
			     mach_addr, (vm_size_t) grow));
		return -ENOMEM;
	}
	if (mach_addr != (vm_address_t) address) {
		printk("expand_stack: "
		       "stack expanded at 0x%x instead of 0x%lx\n",
		       mach_addr, address);
		kr = vm_deallocate(current->osfmach3.task->mach_task_port,
				   mach_addr,
				   (vm_size_t) grow);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("expand_stack: "
				     "can't deallocate bogus stack "
				     "addr=0x%x size=0x%x",
				     mach_addr, (vm_size_t) grow));
		}
		return -ENOMEM;
	}

#ifdef STACK_GROWTH_UP
	vma->vm_end = top;
	vma->vm_offset += grow;
#else
	vma->vm_start = address;
	vma->vm_offset -= grow;
#endif 
#else	/* 0 */
	new_vma = (struct vm_area_struct *)
		kmalloc(sizeof *new_vma, GFP_KERNEL);
	if (!new_vma) {
		return -ENOMEM;
	}
	*new_vma = *vma;
#ifdef	STACK_GROWTH_UP
	new_vma->vm_start = vma->vm_end;
	new_vma->vm_end = vma->vm_end + grow;
	new_vma->vm_offset = vma->vm_offset + (vma->vm_end - vma->vm_start);
	insert_vm_struct(current->mm, new_vma);
	/* merge_segments(current->mm, vma->vm_start, new_vma->vm_end); */
#else	/* STACK_GROWTH_UP */
	new_vma->vm_start = vma->vm_start - grow;
	new_vma->vm_end = vma->vm_start;
	new_vma->vm_offset = 0;
	vma->vm_offset = grow;
	insert_vm_struct(current->mm, new_vma);
	/* merge_segments(current->mm, new_vma->vm_start, vma->vm_end); */
#endif	/* STACK_GROWTH_UP */
	vma = find_vma(current->mm, address);
#endif	/* 0 */

	vma->vm_mm->total_vm += grow >> PAGE_SHIFT;
	if (vma->vm_flags & VM_LOCKED)
		vma->vm_mm->locked_vm += grow >> PAGE_SHIFT;

#if	VMA_DEBUG
	if (vma_debug) {
		printk("expand_stack(vma=%p): "
		       "start=0x%lx,end=0x%lx,off=0x%lx\n",
		       vma, vma->vm_start, vma->vm_end, vma->vm_offset);
	}
#endif	/* VMA_DEBUG */

	return 0;
}

int
osfmach3_msync(
	struct vm_area_struct	*vmp,
	unsigned long		address,
	size_t			size,
	int			flags)
{
	kern_return_t	kr;
	struct osfmach3_mach_task_struct	*mach_task;
	vm_sync_t		sync_flags;

	mach_task = vmp->vm_mm->mm_mach_task;

	sync_flags = 0;
	if (flags & MS_ASYNC)
		sync_flags |= VM_SYNC_ASYNCHRONOUS;
	if (flags & MS_SYNC)
		sync_flags |= VM_SYNC_SYNCHRONOUS;
	if (flags & MS_INVALIDATE)
		sync_flags |= VM_SYNC_INVALIDATE;

	server_thread_blocking(FALSE);
	kr = vm_msync(mach_task->mach_task_port,
		      (vm_address_t) address,
		      (vm_size_t) size,
		      sync_flags);
	server_thread_unblocking(FALSE);

	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("osfmach3_msync(%p,%ld,%ld,0x%x): "
			     "vm_msync",
			     vmp, address, (unsigned long) size, flags));
		return -EIO;
	}

	return 0;
}
