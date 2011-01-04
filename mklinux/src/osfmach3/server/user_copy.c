/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * Routines to access user memory.
 */

#include <linux/autoconf.h>

#include <mach/mach_interface.h>
#include <mach/mach_host.h>
#include <mach/machine/rpc.h>

#include <osfmach3/user_memory.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/mm.h>
#include <osfmach3/mach_init.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>

#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/segment.h>
#include <asm/string.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>

extern boolean_t in_kernel;
int map_user_mem_to_copy = 1;	/* set to 0 to use vm_read_overwrite */

extern kern_return_t vm_read_overwrite(task_port_t task,
				       vm_address_t address,
				       vm_size_t size,
				       vm_address_t data,
				       mach_msg_type_number_t *data_size);

unsigned long get_fs(void)
{
	return current->osfmach3.thread->reg_fs;
}

void set_fs(unsigned long val)
{
	current->osfmach3.thread->reg_fs = val;
}


int
user_memory_verify_area(
	int		type,
	const void	*addr,
	unsigned long	size)
{
	user_memory_t	um;
	vm_address_t	svr_addr;
	vm_prot_t	prot;

	switch (type) {
	    case VERIFY_READ:
		prot = VM_PROT_READ;
		break;
	    case VERIFY_WRITE:
		prot = VM_PROT_WRITE;
		break;
	    default:
		printk("user_memory_verify_area: unknown type 0x%x\n", type); 
		return -EFAULT;
	}
	
	user_memory_lookup(current, (vm_address_t) addr,
			   (vm_size_t) size, prot,
			   &svr_addr, &um);
	if (um == NULL)
		return -EFAULT;
	user_memory_unlock(um);
	return 0;
}

/*
 * How long a filename can we get from user space ?
 *  -EFAULT if invalid area
 *  0 if ok (ENAMETOOLONG) before EFAULT)
 *  >0 EFAULT after xx bytes
 */
int user_memory_get_max_filename(unsigned long address)
{
	vm_address_t			start_addr, user_addr;
	vm_size_t			size, remain;
	vm_region_basic_info_data_t	info;
	mach_msg_type_number_t		count;
	mach_port_t			task_port, objname;
	kern_return_t			kr;

	user_addr = (vm_address_t) address;

	/*
	 * First check that the starting address is valid. -EFAULT if not.
	 */
	task_port = current->osfmach3.task->mach_task_port;
	start_addr = user_addr;
	count = VM_REGION_BASIC_INFO_COUNT;
	kr = vm_region(task_port, &start_addr, &size, VM_REGION_BASIC_INFO,
		       (vm_region_info_t) &info, &count, &objname);
	if (kr != KERN_SUCCESS) {
		return -EFAULT;
	}
	if (MACH_PORT_VALID(objname)) {
		kr = mach_port_deallocate(mach_task_self(), objname);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("user_memory_get_max_filename(%ld): "
				     "mach_port_deallocate(0x%x)",
				     address, objname));
		}
	}
	if (start_addr > user_addr ||
	    (!(info.protection & VM_PROT_READ))) {
		return -EFAULT;
	}

	/*
	 * Now see how much valid memory there us after user_addr. If there
	 * is at least PAGE_SIZE, we can return 0 right now. Otherwise, see
	 * whether there is another readable memory region immediately after
	 * the one containing user_addr; if so, we will be able to read at least
	 * PAGE_SIZE bytes and can again return 0. The remaining case is
	 * when user_addr is in a page followed by unreadable memory, in
	 * which case we return the number of bytes after user_addr in that
	 * page.
	 */
	remain = start_addr +size - user_addr;
	if (remain > PAGE_SIZE) {
		return 0;
	}
	start_addr += size;
	kr = vm_region(task_port, &start_addr, &size, VM_REGION_BASIC_INFO,
		       (vm_region_info_t) &info, &count, &objname);
	if (kr != KERN_SUCCESS) {
		return remain;
	}
	if (MACH_PORT_VALID(objname)) {
		kr = mach_port_deallocate(mach_task_self(), objname);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("user_memory_get_max_filename(%ld): "
				     "mach_port_deallocate(0x%x)",
				     address, objname));
		}
	}
	if (start_addr != user_addr + remain ||
	    !(info.protection & VM_PROT_READ)) {
		return remain;
	}

	return 0;
}

unsigned long
get_phys_addr(
	struct task_struct	*p,
	unsigned long		ptr,
	user_memory_t		*ump)
{
	vm_address_t	svr_addr;
	vm_size_t	size;

	size = PAGE_SIZE - (ptr & ~PAGE_MASK);
	user_memory_lookup(p, (vm_address_t) ptr, size, VM_PROT_READ,
			   &svr_addr, ump);
	if (*ump == NULL) {
		return 0;
	}

	return (unsigned long) svr_addr;
}

unsigned long
get_long(
	struct task_struct	*tsk,
	struct vm_area_struct	*vma,
	unsigned long		addr)
{
	vm_address_t	svr_addr;
	user_memory_t	um;
	unsigned long	data;

	user_memory_lookup(tsk, (vm_address_t) addr,
			   sizeof (unsigned long), VM_PROT_READ,
			   &svr_addr, &um);
	if (um == NULL) {
#if	CONFIG_OSFMACH3_DEBUG
		printk("get_long: no user memory for 0x%lx\n", addr);
#endif	/* CONFIG_OSFMACH3_DEBUG */
		return 0;
	}

	server_thread_blocking(TRUE);
	data = *(unsigned long *)svr_addr;
	server_thread_unblocking(TRUE);

	user_memory_unlock(um);

	return data;
}

void
put_long(
	struct task_struct	*tsk,
	struct vm_area_struct	*vma,
	unsigned long		addr,
	unsigned long		data)
{
	vm_address_t	svr_addr;
	user_memory_t	um;

	user_memory_lookup(tsk, (vm_address_t) addr,
			   sizeof (unsigned long), VM_PROT_WRITE,
			   &svr_addr, &um);
	if (um == NULL) {
		if (! (vma->vm_flags & VM_WRITE)) {
			/*
			 * We might be trying to write a breakpoint into a
			 * text segment. We need to temporarily unprotect
			 * the page and reprotect it afterwards.
			 */
			osfmach3_mprotect_fixup(vma,
						PAGE_TRUNC(addr),
						PAGE_TRUNC(addr) + PAGE_SIZE,
						vma->vm_flags | VM_WRITE);
			/* try the lookup again */
			user_memory_lookup(tsk, (vm_address_t) addr,
					   sizeof (unsigned long),
					   VM_PROT_WRITE, &svr_addr, &um);
			if (um != NULL) {
				/* it worked: sneak in our change */
				server_thread_blocking(TRUE);
				*(unsigned long *) svr_addr = data;
				server_thread_unblocking(TRUE);
				user_memory_unlock(um);
			} else {
#if	CONFIG_OSFMACH3_DEBUG
				printk("put_long: "
				       "really no user memory for 0x%lx\n",
				       addr);
#endif	/* CONFIG_OSFMACH3_DEBUG */
			}
			/* restore the previous protection */
			osfmach3_mprotect_fixup(vma,
						PAGE_TRUNC(addr),
						PAGE_TRUNC(addr) + PAGE_SIZE,
						vma->vm_flags);
			return;
		}

#if	CONFIG_OSFMACH3_DEBUG
		printk("put_long: no user memory for 0x%lx\n", addr);
#endif	/* CONFIG_OSFMACH3_DEBUG */
		return;
	}

	server_thread_blocking(TRUE);
	*(unsigned long *)svr_addr = data;
	server_thread_unblocking(TRUE);

	user_memory_unlock(um);

#if	defined(PPC) || defined(__hppa__)
	if (vma->vm_flags & VM_EXEC) {
		/* flush the instruction cache */
		flush_instruction_cache_range(vma->vm_mm,
					      addr,
					      addr + sizeof (unsigned long));
	}
#endif	/* PPC || __hppa__ */
}

int
remap_user_copyin(
	struct task_struct	*task,
	vm_address_t		from,
	char			*to,
	int			count)
{
	vm_address_t	svr_addr;
	user_memory_t	um;

	user_memory_lookup(task, from, count, VM_PROT_READ, &svr_addr, &um);
	if (um == NULL) {
		return -EFAULT;
	}

	server_thread_blocking(TRUE);
	memcpy(to, (char *)svr_addr, count);
	server_thread_unblocking(TRUE);
	user_memory_unlock(um);

	return 0;
}

int
task_copyin(
	struct task_struct 	*task,
	vm_address_t 		from,
	char			*to,
	int			count)
{
	vm_size_t	bytes_read;
	kern_return_t	kr;

	server_thread_blocking(TRUE);
	kr = vm_read_overwrite(task->osfmach3.task->mach_task_port,
			       from, count,
			       (vm_address_t) to, &bytes_read);
	server_thread_unblocking(TRUE);
	if (kr != KERN_SUCCESS || bytes_read != count) {
		return -EFAULT;
	}
	return 0;
}

void
copyin(
	vm_address_t	from,
	char		*to,
	unsigned long	count)
{
	int		error;
	mach_port_t	task_port;

	if (in_kernel) {
		task_port = current->osfmach3.task->mach_task_port;
		server_thread_blocking(TRUE);
#ifdef i386
		/* stupid COPYIN macro */
		error = COPYIN(task_port, from, to, count);
#else	/* i386 */
		error = COPYIN(task_port,
			       (vm_offset_t) from, (vm_offset_t) to, count);
#endif	/* i386 */
		server_thread_unblocking(TRUE);
		if (error) {
			printk("[P%d %s] copyin(0x%x,0x%p,%ld): "
			       "COPYIN returned %d (0x%x)\n",
			       current->pid, current->comm,
			       from, to, count, error, error);
			ASSERT(0);
		}
	} else if (map_user_mem_to_copy) {
		error = remap_user_copyin(current, from, to, count);
		if (error) {
			printk("[P%d %s] copyin(0x%x,0x%p,%ld): "
			       "remap_user_copyin returned %d (0x%x)\n",
			       current->pid, current->comm,
			       from, to, count, error, error);
			ASSERT(0);
		}
	} else {
		error = task_copyin(current, from, to, count);
		if (error) {
			printk("[P%d %s] copyin(0x%x,0x%p,%ld): "
			       "task_copyin returned %d (0x%x)\n",
			       current->pid, current->comm,
			       from, to, count, error, error);
			ASSERT(0);
		}
	}

	return;
}

int
remap_user_copyinstr(
	struct task_struct	*task,
	vm_address_t		from,
	char			*to,
	int			max,
	int			*len)
{
	char		*user_ptr;
	vm_address_t	copy_addr;
	vm_offset_t	copy_offset;
	vm_size_t	copy_len;
	vm_size_t	remains_len;
	vm_size_t	max_map_size;
	user_memory_t	um;
	int		count;
	int		error;

	count = 0;
	max_map_size = PAGE_ALIGN(from + 1) - from;
	copy_offset = 0;
	remains_len = max;

	while (count < max) {
		user_memory_lookup(task, from, max_map_size,
				   VM_PROT_READ, &copy_addr, &um);
		if (um == NULL) {
			error = -EFAULT;
			break;
		}

		copy_addr += copy_offset;
		if (remains_len > max_map_size - copy_offset)
			copy_len = max_map_size - copy_offset;
		else
			copy_len = remains_len;
		remains_len -= copy_len;
		user_ptr = (char *) copy_addr;

		server_thread_blocking(TRUE);
		while (copy_len-- > 0) {
			count++;
			if (to != NULL) {
				*to++ = *user_ptr;
			}
			if (*user_ptr == '\0') {
				server_thread_unblocking(TRUE);
				user_memory_unlock(um);
				error = 0;
				goto done;
			}
			user_ptr++;
		}
		server_thread_unblocking(TRUE);
		user_memory_unlock(um);
		copy_offset = max_map_size;
		max_map_size += PAGE_SIZE;
	}

	error = 0; /* Never return -ENAMETOOLONG, already checked */

    done:
	if (len)
		*len = count;
	return error;
}

int
task_copyinstr(
	struct task_struct 	*task,
	vm_address_t 		from,
	char			*to,
	int			max,
	int			*len)
{
	kern_return_t	kr;
	vm_size_t	bytes_read;
	vm_size_t	copy_size;
	int		count;
	int 		error;
	unsigned long	alloced_page;

	count = 0;

	if (to == NULL) {
		alloced_page = get_free_page(GFP_KERNEL);
	} else {
		alloced_page = 0;
	}

	while (count < max) {
		if (alloced_page)
			to = (char *) alloced_page;

		/* Don't cross pages in the user task */
		if (from & ~PAGE_MASK) {
			copy_size = PAGE_ALIGN(from) - from;
		} else {
			copy_size = PAGE_SIZE;
		}
		if (copy_size > max)
			copy_size = max;
		server_thread_blocking(TRUE);
		kr = vm_read_overwrite(task->osfmach3.task->mach_task_port,
				       from, copy_size,
				       (vm_address_t) to, &bytes_read);
		server_thread_unblocking(TRUE);
		if (kr != KERN_SUCCESS || bytes_read != copy_size) {
			error = -EFAULT;
			goto done;
		}

		while (bytes_read-- > 0) {
			if (count < max) {
				count++;
				if (*to++ == 0) {
					error = 0;
					goto done;
				}
			} else {
				error = -EFAULT;
				break;
			}
		}
		from += copy_size;
		max -= copy_size;
	}
	error = 0; /* Never return -ENAMETOOLONG, already checked */

    done:
	if (len)
		*len = count;

	if (alloced_page)
		free_page(alloced_page);

	return error;
}

void
copyinstr(
	vm_address_t	from,
	char		*to,
	int		max,
	int		*len)
{
	int		error;
	mach_port_t	task_port;

	if (in_kernel) {
		task_port = current->osfmach3.task->mach_task_port;
		server_thread_blocking(TRUE);
#ifdef i386
		/* stupid COPYINSTR macro */
		error = COPYINSTR(task_port, from, to, max, len);
#else	/* i386 */
		error = COPYINSTR(task_port,
				  (vm_offset_t) from, (vm_offset_t) to,
				  max, len);
#endif	/* i386 */
		server_thread_unblocking(TRUE);
		if (error) {
			printk("[P%d %s] copyinstr(0x%x,0x%p,%d) :"
			       "COPYINSTR returned %d (0x%x)\n",
			       current->pid, current->comm,
			       from, to, max, error, error);
			ASSERT(0);
		}
	} else if (map_user_mem_to_copy) {
		error = remap_user_copyinstr(current, from, to, max, len);
		if (error) {
			printk("[P%d %s] copyinstr(0x%x,0x%p,%d) :"
			       "remap_user_copyinstr returned %d (0x%x)\n",
			       current->pid, current->comm,
			       from, to, max, error, error);
			ASSERT(0);
		}
	} else {
		error = task_copyinstr(current, from, to, max, len);
		if (error) {
			printk("[P%d %s] copyinstr(0x%x,0x%p,%d) :"
			       "task_copyinstr returned %d (0x%x)\n",
			       current->pid, current->comm,
			       from, to, max, error, error);
			ASSERT(0);
		}
	}

	return;
}

int
remap_user_copyout(
	struct task_struct	*task,
	char			*from,
	vm_address_t		to,
	int			count)
{
	vm_address_t	svr_addr;
	user_memory_t	um;

	user_memory_lookup(task, to, count, VM_PROT_WRITE, &svr_addr, &um);
	if (um == NULL) {
		return -EFAULT;
	}

	server_thread_blocking(TRUE);
	memcpy((char *) svr_addr, from, count);
	server_thread_unblocking(TRUE);
	user_memory_unlock(um);

	return 0;
}

int
task_copyout(
	struct task_struct 	*task,
	char			*from,
	vm_address_t		to,
	int			count)
{
	kern_return_t	kr;

	server_thread_blocking(TRUE);
	kr = vm_write(task->osfmach3.task->mach_task_port,
		      (vm_address_t) to,
		      (pointer_t) from,
		      count);
	server_thread_unblocking(TRUE);
	if (kr != KERN_SUCCESS) {
		return -EFAULT;
	}
	return 0;
}

void
copyout(
	char 		*from,
	vm_address_t	to,
	unsigned long	count)
{
	int		error;
	mach_port_t	task_port;

	if (in_kernel) {
		task_port = current->osfmach3.task->mach_task_port;
		server_thread_blocking(TRUE);
#ifdef	i386
		/* stupid COPYOUT macro */
		error = COPYOUT(task_port, from, to, count);
#else	/* i386 */
		error = COPYOUT(task_port,
				(vm_offset_t) from, (vm_offset_t) to, count);
#endif	/* i386 */
		server_thread_unblocking(TRUE);
		if (error) {
			printk("[P%d %s] copyout(0x%p,0x%x,%ld): "
			       "COPYOUT returned %d (0x%x)\n",
			       current->pid, current->comm,
			       from, to, count, error, error);
			ASSERT(0);
		}
	} else if (map_user_mem_to_copy) {
		error = remap_user_copyout(current, from, to, count);
		if (error) {
			printk("[P%d %s] copyout(0x%p,0x%x,%ld): "
			       "remap_user_copyout returned %d (0x%x)\n",
			       current->pid, current->comm,
			       from, to, count, error, error);
			ASSERT(0);
		}
	} else {
		error = task_copyout(current, from, to, count);
		if (error) {
			printk("[P%d %s] copyout(0x%p,0x%x,%ld): "
			       "task_copyout returned %d (0x%x)\n",
			       current->pid, current->comm,
			       from, to, count, error, error);
			ASSERT(0);
		}
	}

	return;
}

void
memcpy_tofs(
	void		*to,
	const void	*from,
	unsigned long	n)
{
	if (get_fs() == get_ds()) {
		/* Copy into server address space */
		memcpy(to, from, n);
		return;
	}

	copyout((char *) from, (vm_address_t) to, n);
}

void
memcpy_fromfs(
	void		*to,
	const void	*from,
	unsigned long	n)
{
	if (get_fs() == get_ds()) {
		/* Copy from server adress space */
		memcpy(to, from, n);
		return;
	}

	copyin((vm_address_t) from, (char *) to, n);
}

int
strlen_fromfs(
	const char	*addr)
{
	int len;

	if (get_fs() == get_ds()) {
		/* String is in the server's address space */
		return strlen(addr);
	}

	copyinstr((vm_address_t) addr, NULL, INT_MAX, &len);

	/*
	 * The length returned by copyinstr includes the trailing '\0'.
	 */
	return len - 1;
}

int
copystr_fromfs(
	char		*to,
	const char	*from,
	int		n)
{
	int len;

	if (get_fs() == get_ds()) {
		/* String is in the server's address space */
		strncpy(to, from, n);
		for (len = 0; len < n; len++) {
			if (*to++ == '\0')
				break;
		}
		return len;
	}

	copyinstr((vm_address_t) from, to, n, &len);

	return len - 1;
}
