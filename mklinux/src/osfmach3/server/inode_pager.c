/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <cthreads.h>
#include <mach/mach_interface.h>
#include <mach/mach_host.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>
#include <osfmach3/mach_init.h>
#include <osfmach3/inode_pager.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/serv_port.h>

#include <linux/malloc.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>

#if	CONFIG_OSFMACH3_DEBUG
#define INODE_PAGER_DEBUG 1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	INODE_PAGER_DEBUG
int inode_pager_debug = 0;
#endif	/* INODE_PAGER_DEBUG */

extern boolean_t inode_object_server(mach_msg_header_t *InHeadP,
				     mach_msg_header_t *OutHeadP);

mach_port_t		inode_pager_port_set;
struct task_struct	inode_pager_task;

int		inode_pager_max_urefs = 10000;

/*
 * Macros to convert from memory_object ports to i_mem_object data structures
 * and vice versa.
 */
#define MO_TO_IMO(mem_obj)	(serv_port_name((mem_obj)))
#define IMO_TO_MO(imo)		((imo)->imo_mem_obj)

void *
inode_pager_thread(
	void	*arg)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t		kr;
	mach_msg_header_t	*in_msg;
	mach_msg_header_t	*out_msg;

	cthread_set_name(cthread_self(), "inode pager thread");
	server_thread_set_priv_data(cthread_self(), &priv_data);
	/*
	 * The inode pager runs in its own Linux task...
	 */
	priv_data.current_task = &inode_pager_task;
#if 0
	inode_pager_task.osfmach3.thread->active_on_cthread = cthread_self();
#endif
	/*
	 * Allow this thread to preempt preemptible threads, to solve deadlocks
	 * where the server touches some data that is backed by the inode
	 * pager. See user_copy.c.
	 */
	priv_data.preemptive = TRUE;

	uniproc_enter();

	kr = vm_allocate(mach_task_self(),
			 (vm_offset_t *) &in_msg,
			 INODE_PAGER_MESSAGE_SIZE,
			 TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("inode_pager_thread: vm_allocate"));
		panic("inode_pager_thread: can't allocate in_msg");
	}
	kr = vm_allocate(mach_task_self(),
			 (vm_offset_t *) &out_msg,
			 INODE_PAGER_MESSAGE_SIZE,
			 TRUE);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("inode_pager_thread: vm_allocate"));
		panic("inode_pager_thread: can't allocate out_msg");
	}

	inode_pager_task.state = TASK_INTERRUPTIBLE;
	server_thread_blocking(FALSE);
	for (;;) {
		kr = mach_msg(in_msg, MACH_RCV_MSG, 0, INODE_PAGER_MESSAGE_SIZE,
			      inode_pager_port_set, MACH_MSG_TIMEOUT_NONE,
			      MACH_PORT_NULL);
		server_thread_unblocking(FALSE);	/* can preempt ! */
		inode_pager_task.state = TASK_RUNNING;
		if (kr != MACH_MSG_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("inode_pager_thread: mach_msg(RCV)"));
			server_thread_blocking(FALSE);
			continue;
		}

		if (!inode_object_server(in_msg, out_msg)) {
			printk("inode_pager_thread: invalid msg id 0x%x\n",
			       in_msg->msgh_id);
		}

		inode_pager_task.state = TASK_INTERRUPTIBLE;
		server_thread_blocking(FALSE);
		
		if (MACH_PORT_VALID(out_msg->msgh_remote_port)) {
			kr = mach_msg(out_msg, MACH_SEND_MSG,
				      out_msg->msgh_size, 0, MACH_PORT_NULL,
				      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
			if (kr != MACH_MSG_SUCCESS) {
				MACH3_DEBUG(1, kr, ("inode_pager_thread: mach_msg(SEND)"));
			}
		}
	}
}

void
inode_pager_init(void)
{
	kern_return_t		kr;
	int			nr;

	kr = mach_port_allocate(mach_task_self(),
				MACH_PORT_RIGHT_PORT_SET,
				&inode_pager_port_set);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("inode_pager_init: mach_port_allocate"));
		panic("inode_pager_init: can't allocate port set");
	}

	/*
	 * Create a new Linux task for the inode pager, so it can block
	 * and handle page faults like if it was a user process.
	 * Skip task[0] (the server task) and task[1] (reserved for init).
	 */
	for (nr = 2; nr < NR_TASKS; nr++) {
		if (!task[nr])
			break;
	}
	if (nr >= NR_TASKS)
		panic("inode_pager_init: can't find empty process");
	inode_pager_task = *current;	/* XXX ? */
	strncpy(inode_pager_task.comm,
		"inode pager",
		sizeof (inode_pager_task.comm));
	task[nr] = &inode_pager_task;
	SET_LINKS(&inode_pager_task);
	nr_tasks++;

	(void) server_thread_start(inode_pager_thread, (void *) 0);
}

void
imo_ref(
	struct i_mem_object *imo)
{
	imo->imo_refcnt++;
}

void
imo_unref(
	struct i_mem_object *imo)
{
	kern_return_t	kr;

	ASSERT(imo->imo_refcnt > 0);
	imo->imo_refcnt--;
	if (imo->imo_refcnt == 0) {
		imo->imo_inode->i_mem_object = NULL;
		iput(imo->imo_inode);
		imo->imo_inode = NULL;
#ifdef	CONFIG_OSFMACH3_DEBUG
		/*
		 * We probably have a send-once right which the debugging
		 * code in serv_port_destroy would be upset about (it expects
		 * a pure receive right).
		 */
		kr = mach_port_deallocate(mach_task_self(), IMO_TO_MO(imo));
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("imo_unref: mach_port_deallocate(0x%x)",
				     IMO_TO_MO(imo)));
		}
#endif	/* CONFIG_OSFMACH3_DEBUG */
		kr = serv_port_destroy(IMO_TO_MO(imo));
		kfree(imo);
	}
}

struct i_mem_object *
imo_create(
	struct inode	*inode,
	boolean_t	allocate_port)
{
	struct i_mem_object	*imo;
	kern_return_t		kr;

	imo = (struct i_mem_object *)
		kmalloc(sizeof (struct i_mem_object), GFP_KERNEL);
	if (imo == NULL)
		return MEMORY_OBJECT_NULL;

	if (inode->i_mem_object != NULL) {
		/*
		 * Somebody else beat us...
		 */
		kfree(imo);
		return inode->i_mem_object;
	}

	inode->i_count++;
	inode->i_mem_object = imo;

	imo->imo_mem_obj = MACH_PORT_NULL;
	imo->imo_mem_obj_control = MACH_PORT_NULL;
	imo->imo_refcnt = 0;
	imo->imo_cacheable = TRUE;
	imo->imo_attrchange = FALSE;
	imo->imo_attrchange_wait = NULL;
	imo->imo_copy_strategy = MEMORY_OBJECT_COPY_DELAY;
	imo->imo_errors = 0;
	imo->imo_inode = inode;
	imo->imo_urefs = 0;

	if (allocate_port) {
		/*
		 * Allocate a memory object port
		 */
		kr = serv_port_allocate_name(&imo->imo_mem_obj, imo);
		if (kr != KERN_SUCCESS) {
			panic("imo_create: can't allocate port");
		}

		/*
		 * Get a send right for this port
		 */
		kr = mach_port_insert_right(mach_task_self(),
					    imo->imo_mem_obj, imo->imo_mem_obj,
					    MACH_MSG_TYPE_MAKE_SEND);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr,
				    ("imo_create: mach_port_insert_right(0x%x)",
				     imo->imo_mem_obj));
			panic("imo_create: can't allocate send right");
		}

		/*
		 * Add the new memory_object port to the port set
		 */
		kr = mach_port_move_member(mach_task_self(),
					   imo->imo_mem_obj,
					   inode_pager_port_set);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr,
				    ("imo_create: mach_port_move_member(0x%x)",
				     imo->imo_mem_obj));
			panic("imo_create: can't add object to port set");
		}
	}

	return imo;
}

memory_object_t
inode_pager_setup(
	struct inode	*inode)
{
	struct i_mem_object	*imo;

	if (inode == NULL)
		return MEMORY_OBJECT_NULL;

	imo = inode->i_mem_object;
	if (imo == NULL) {
		/*
		 * First time this inode is mapped. Create a memory object.
		 */
		imo = imo_create(inode, TRUE);
		if (imo == NULL)
			return MEMORY_OBJECT_NULL;
	}

	imo_ref(imo);	/* 1 ref for the mapper */

#ifdef	INODE_PAGER_DEBUG
	if (inode_pager_debug) {
		printk("inode_pager_setup: imo 0x%p: obj 0x%x, inode 0x%p\n",
		       imo, imo->imo_mem_obj, imo->imo_inode);
	}
#endif	/* INODE_PAGER_DEBUG */

	ASSERT(MACH_PORT_VALID(imo->imo_mem_obj));
	return imo->imo_mem_obj;
}

void
inode_pager_release(
	struct inode	*inode)
{
	struct i_mem_object 	*imo;

	if (inode == NULL)
		return;

	imo = inode->i_mem_object;
	if (imo == NULL) {
		panic("inode_pager_release: no i_mem_object for inode 0x%p",
		      inode);
	}

	imo_unref(imo);	/* get rid of a mapper's reference */
}


/*
 * Change the attributes of a memory object.  
 * 
 * The caller may choose to wait until the operation is known to have
 * completed.  In the case of setting cacheable to FALSE, specifying 'wait'
 * guarantee that the kernel has relinquished its data if it could 
 * (it's possible it couldn't if the data is currently accessible 
 * - i.e., mapped).
 *
 * The caller donates a "imo" reference to this routine, which is
 * consumed here.
 */
kern_return_t
inode_pager_change_attributes(
	struct i_mem_object	*imo,
	boolean_t		*cacheable,
	boolean_t		*temporary,
	boolean_t		do_wait)
{
	boolean_t			new_cacheable;
	memory_object_copy_strategy_t	new_copy_strategy;
	memory_object_attr_info_data_t	attributes;
	kern_return_t			kr;
	struct wait_queue		wait = { current, NULL };

	kr = KERN_SUCCESS;

    loop:
	if (cacheable)
		new_cacheable = *cacheable;
	else
		new_cacheable = imo->imo_cacheable;
	if (temporary)
		new_copy_strategy = MEMORY_OBJECT_COPY_DELAY;
	else
		new_copy_strategy = imo->imo_copy_strategy;

	if (new_cacheable != imo->imo_cacheable ||
	    new_copy_strategy != imo->imo_copy_strategy) {

		/*
		 * Attribute change required.
		 */
		if (imo->imo_mem_obj_control == MACH_PORT_NULL) {
			/*
			 * The object hasn't been initialized yet
			 * (i.e. the kernel hasn't used it yet).
			 * Just update the imo structure.
			 */
			imo->imo_cacheable = new_cacheable;
			imo->imo_copy_strategy = new_copy_strategy;
			goto done;
		}

		if (new_cacheable && !imo->imo_cacheable) {
			/* take a reference for the microkernel's cache */
			imo_ref(imo);
		} else if (!new_cacheable && imo->imo_cacheable) {
			/* release the microkernel's cache reference */
			imo_unref(imo);
		}

		/*
		 * The object has been initialized.
		 * Tell the kernel that the attributes changed.
		 */
		if (do_wait) {
			add_wait_queue(&imo->imo_attrchange_wait, &wait);
			if (imo->imo_attrchange) {
				/*
				 * We already have a change attribute
				 * request in progress. Wait and retry.
				 */
				schedule();
				remove_wait_queue(&imo->imo_attrchange_wait,
						  &wait);
				goto loop;
			}
			imo->imo_attrchange = TRUE;
		}
		imo->imo_cacheable = new_cacheable;
		imo->imo_copy_strategy = new_copy_strategy;

		attributes.copy_strategy = imo->imo_copy_strategy;
		if (imo->imo_inode)
			attributes.cluster_size = PAGE_SIZE;	/* XXX */
		else
			attributes.cluster_size = 0;	/* default */
		attributes.may_cache_object = imo->imo_cacheable;
		if (temporary && *temporary)
			attributes.temporary = TRUE;
		else
			attributes.temporary = FALSE;

		kr = memory_object_change_attributes(
				imo->imo_mem_obj_control,
				MEMORY_OBJECT_ATTRIBUTE_INFO,
				(memory_object_info_t) &attributes,
				MEMORY_OBJECT_ATTR_INFO_COUNT,
				do_wait ? IMO_TO_MO(imo) : MACH_PORT_NULL);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("inode_pager_change_attributes: "
				     "memory_object_change_attributes(0x%x)",
				     imo->imo_mem_obj_control));
			if (do_wait) {
				imo->imo_attrchange = FALSE;
				remove_wait_queue(&imo->imo_attrchange_wait,
						  &wait);
				goto done;
			}
		}

		if (do_wait) {
			schedule();
			remove_wait_queue(&imo->imo_attrchange_wait,
					  &wait);
		}
	}

    done:
	return kr;
}

void
inode_pager_uncache(
	struct inode	*inode)
{
	struct i_mem_object	*imo;
	boolean_t		cacheable;

	imo = inode->i_mem_object;
	if (imo == NULL)
		return;

	/*
	 * Tell the microkernel not to cache this memory object.
	 */
	imo_ref(imo);	/* keep object alive during the change_attr */
	cacheable = FALSE;
	inode_pager_change_attributes(imo, &cacheable, NULL, FALSE);
	imo_unref(imo);	/* get rid of above reference */
}

void
inode_pager_uncache_now(
	struct inode	*inode)
{
	struct i_mem_object	*imo;
	boolean_t		cacheable;

	imo = inode->i_mem_object;
	if (imo == NULL)
		return;

	/*
	 * Tell the microkernel not to cache this memory object.
	 */
	imo_ref(imo);	/* keep object alive during the change_attr */
	cacheable = FALSE;
	inode_pager_change_attributes(imo, &cacheable, NULL, TRUE);
	imo_unref(imo);	/* get rid of above reference */
}

struct i_mem_object *
inode_pager_check_request(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control)
{
	struct i_mem_object	*imo;
	kern_return_t		kr;

	imo = MO_TO_IMO(mem_obj);

	ASSERT(imo->imo_mem_obj_control == mem_obj_control);
	ASSERT(imo->imo_urefs > 0);

	if (++imo->imo_urefs > inode_pager_max_urefs) {
		/* deallocate excess user refs to the control port */
		kr = mach_port_mod_refs(mach_task_self(),
					mem_obj_control,
					MACH_PORT_RIGHT_SEND,
					- (imo->imo_urefs - 1));
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(0, kr, ("inode_pager_check_request: mach_port_mod_urefs"));
			panic("inode_pager_check_request: mod_refs");
		}
		imo->imo_urefs = 1;
	}

	return imo;
}

kern_return_t
inode_object_change_completed(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	memory_object_flavor_t	flavor)
{
	struct i_mem_object	*imo;

#ifdef	INODE_PAGER_DEBUG
	if (inode_pager_debug) {
		printk("inode_object_change_completed: obj 0x%x flavor 0x%x\n",
		       mem_obj, flavor);
	}
#endif	/* INODE_PAGER_DEBUG */

	if (mem_obj_control == MACH_PORT_NULL) {
		/*
		 * The object was terminated and the "imo" has probably
		 * been released already.
		 */
		return KERN_SUCCESS;
	}

	imo = inode_pager_check_request(mem_obj, mem_obj_control);
	if (imo->imo_attrchange) {
		imo->imo_attrchange = FALSE;
		wake_up(&imo->imo_attrchange_wait);
	}

	return KERN_SUCCESS;
}

kern_return_t
inode_object_data_request(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length,
	vm_prot_t		desired_access)
{
	struct i_mem_object	*imo;
	kern_return_t		kr;
	struct vm_area_struct	fake_vma;
	struct vm_area_struct	*vma;
	unsigned long		page;
	int			i;
	struct task_struct	*tsk;
	struct osfmach3_mach_task_struct *mach_task;
	struct mm_struct	*mm;
	unsigned long		(*nopage)(struct vm_area_struct * area,
					  unsigned long address,
					  int write_access);
	extern unsigned long	filemap_nopage(struct vm_area_struct *area,
					       unsigned long address,
					       int no_share);

#ifdef	INODE_PAGER_DEBUG
	if (inode_pager_debug) {
		printk("inode_object_data_request: obj 0x%x offset 0x%x length 0x%x\n",
		       mem_obj, offset, length);
	}
#endif	/* INODE_PAGER_DEBUG */

	if (length % PAGE_SIZE)
		panic("inode_object_data_request: bad length");

	imo = inode_pager_check_request(mem_obj, mem_obj_control);

	/*
	 * We don't know which user task caused the page fault,
	 * so just take the first vm_area that maps this inode
	 * and pretend that's the faulting one.
	 * XXX This relies on the fact that all mappings of a given
	 * inode are done with the same vm_ops...
	 * XXX of course this is a BAAAAD and WRONG assumption !
	 */
	vma = imo->imo_inode->i_mmap;
	ASSERT(vma);
	ASSERT(vma->vm_ops);
	ASSERT(vma->vm_ops->nopage);
	mm = vma->vm_mm;
	ASSERT(mm);
#if 0
	/* this can fail !!!??? */
	ASSERT(mm->count > 0);
#endif
	mach_task = mm->mm_mach_task;
	ASSERT(mach_task);
	for (i = 0; i < NR_TASKS; i++) {
		if (task[i] &&
		    task[i]->osfmach3.task == mach_task) {
			break;
		}
	}
	if (i == NR_TASKS) {
		panic("inode_object_data_request: can't locate target task\n");
	}
	tsk = task[i];
	ASSERT(tsk);
	ASSERT(tsk->mm == mm || tsk->mm == &init_mm);

	fake_vma.vm_inode = imo->imo_inode;
	fake_vma.vm_ops = vma->vm_ops;
	fake_vma.vm_start = 0;
	fake_vma.vm_offset = offset;
	fake_vma.vm_end = offset + PAGE_SIZE;  /* XXX we might map too much ! */

	/* XXX take the identity of the task that did the mapping */
	current->uid = tsk->uid;
	current->euid = tsk->euid;
	current->suid = tsk->suid;
	current->fsuid = tsk->fsuid;
	current->gid = tsk->gid;
	current->egid = tsk->egid;
	current->sgid = tsk->sgid;
	current->fsgid = tsk->fsgid;
	for (i = 0; i < NGROUPS; i++)
		current->groups[i] = tsk->groups[i];

	nopage = fake_vma.vm_ops->nopage;
	page = nopage(&fake_vma,
		      0,
		      (nopage != filemap_nopage)	/* no_share */);

	/* take back our (ghost) identity */
	current->uid = 0;
	current->euid = 0;
	current->suid = 0;
	current->fsuid = 0;
	current->gid = 0;
	current->egid = 0;
	current->sgid = 0;
	current->fsgid = 0;
	current->groups[0] = NOGROUP;
	
	if (!page) {
#ifdef	INODE_PAGER_DEBUG
		if (inode_pager_debug) {
			printk("mo_data_request: mo_data_error"
			       "(mo_ctl=0x%x, off=0x%x, size=0x%x)\n",
			       mem_obj_control,
			       offset,
			       length);
#if 0
			printk("mo_data_request: SIGBUS for P%d[%s]\n",
			       tsk->pid, tsk->comm);
#endif
		}
#endif	/* INODE_PAGER_DEBUG */
#if 0
		/* "tsk" is not necessarily the task that caused the fault */
		force_sig(SIGBUS, tsk);
#endif
		kr = memory_object_data_error(mem_obj_control,
					      offset,
					      length,
					      KERN_MEMORY_ERROR);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("mo_data_request: mo_data_error"));
			panic("mo_data_request: mo_data_error");
		}
		return KERN_SUCCESS;
	}

#if 0
	/* CAUTION: tsk might not exist anymore at this point... */
	++tsk->maj_flt;
	++vma->vm_mm->rss;
#endif

#ifdef	INODE_PAGER_DEBUG
	if (inode_pager_debug) {
		printk("mo_data_request: mo_data_supply(mo_ctl=0x%x, off=0x%x, data=0x%lx, size=0x%x, dealloc=FALSE, lock=VM_PROT_NONE, precious=FALSE, reply_port=NULL)\n",
		       mem_obj_control,
		       offset,
		       page,
		       length);
	}
#endif	/* INODE_PAGER_DEBUG */

	kr = memory_object_data_supply(mem_obj_control,
				       offset,
				       page,
				       length,
				       FALSE,
				       VM_PROT_NONE,
				       FALSE,
				       MACH_PORT_NULL);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("mo_data_request: mo_data_supply"));
		panic("mo_data_request: mo_data_supply failed");
	}

	if (nopage == filemap_nopage) {
		/*
		 * release the extra reference due to the sharing:
		 * the microkernel takes care of the sharing for us and
		 * keeping this reference would prevent the "shrinker"
		 * from freeing this page if needed.
		 */
		ASSERT(mem_map[MAP_NR(page)].count > 1);
		atomic_dec(&(mem_map[MAP_NR(page)].count));
	} else {
		/* page was allocated just for us: free it now */
		free_page(page);
	}

#ifdef	INODE_PAGER_DEBUG
	if (inode_pager_debug) {
		printk("mo_data_request: done\n");
	}
#endif	/* INODE_PAGER_DEBUG */

	return KERN_SUCCESS;
}

kern_return_t
inode_object_data_return(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_offset_t		data,
	vm_size_t		length,
	boolean_t		dirty,
	boolean_t		kernel_copy)
{
	struct i_mem_object	*imo;
	kern_return_t		kr;
	struct file		file;
	struct vm_area_struct	*vma;
	struct inode		*inode;
	struct task_struct	*tsk;
	struct osfmach3_mach_task_struct *mach_task;
	struct mm_struct	*mm;
	int			i;
	int			result;
	vm_offset_t		orig_data;
	vm_size_t		orig_length;

#ifdef	INODE_PAGER_DEBUG
	if (inode_pager_debug) {
		printk("inode_object_data_return: obj 0x%x control 0x%x "
		       "offset 0x%x length 0x%x\n",
		       mem_obj, mem_obj_control, offset, length);
	}
#endif	/* INODE_PAGER_DEBUG */

	imo = inode_pager_check_request(mem_obj, mem_obj_control);

	/*
	 * Flush the data to the disk.
	 */
	vma = imo->imo_inode->i_mmap;
	ASSERT(vma);
	if (!vma) {
#ifdef INODE_PAGER_DEBUG
		printk("inode_pager: return with no map!\n");
		inode = imo->imo_inode;
		file.f_op = inode->i_op->default_file_ops;
		printk("  inode: %ld, dev: %x, write: %x\n", 
		       inode->i_ino, (unsigned)inode->i_dev, (unsigned)file.f_op->write);
#endif
		orig_data = data;
		orig_length = length;
	} else {
	mm = vma->vm_mm;
	ASSERT(mm);
	mach_task = mm->mm_mach_task;
	ASSERT(mach_task);
	for (i = 0; i < NR_TASKS; i++) {
		if (task[i] &&
		    task[i]->osfmach3.task == mach_task) {
			break;
		}
	}
	if (i == NR_TASKS) {
		panic("inode_object_data_return: can't locate target task\n");
	}
	tsk = task[i];
	ASSERT(tsk);
	ASSERT(tsk->mm == mm || tsk->mm == &init_mm);

	/* XXX take the identity of the task that did the mapping */
	current->uid = tsk->uid;
	current->euid = tsk->euid;
	current->suid = tsk->suid;
	current->fsuid = tsk->fsuid;
	current->gid = tsk->gid;
	current->egid = tsk->egid;
	current->sgid = tsk->sgid;
	current->fsgid = tsk->fsgid;
	for (i = 0; i < NGROUPS; i++)
		current->groups[i] = tsk->groups[i];

	orig_data = data;
	orig_length = length;

	/*
	 * XXX: Code inspired directly from filemap_write_page, not generic!
	 */
	inode = imo->imo_inode;
	ASSERT(inode);
	file.f_op = inode->i_op->default_file_ops;
	if (file.f_op->write) {
		file.f_mode = 3;
		file.f_flags = 0;
		file.f_count = 1;
		file.f_inode = inode;
		file.f_pos = offset;
		file.f_reada = 0;

		down(&inode->i_sem);
		do {
			extern int do_write_page(struct inode * inode,
						 struct file * file,
						 const char * page,
						 unsigned long offset);

			result = do_write_page(inode, &file,
					       (const char *) data, offset);
			data += PAGE_SIZE;
			offset += PAGE_SIZE;
			length -= PAGE_SIZE;
		} while (length > 0);
		up(&inode->i_sem);
	} else {
		result = 0;
	}

	/* take back our (ghost) identity */
	current->uid = 0;
	current->euid = 0;
	current->suid = 0;
	current->fsuid = 0;
	current->gid = 0;
	current->egid = 0;
	current->sgid = 0;
	current->fsgid = 0;
	current->groups[0] = NOGROUP;
	
	if (result) {
		printk("inode_object_data_return: "
		       "do_write_page returned %d\n",
		       result);
	}
	} /* !vma */

	kr = vm_deallocate(mach_task_self(),
			   orig_data,
			   orig_length);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("inode_object_data_return: "
			     "vm_deallocate(0x%x,0x%x)",
			     orig_data, orig_length));
	}

	return KERN_SUCCESS;
}

kern_return_t
inode_object_data_unlock(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length,
	vm_prot_t		desired_access)
{
	panic("inode_object_data_unlock");
}

kern_return_t
inode_object_discard_request(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length)
{
	panic("inode_object_discard_request");
}

kern_return_t
inode_object_init(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_size_t		page_size)
{
	struct i_mem_object		*imo;
	memory_object_attr_info_data_t	attributes;
	kern_return_t			kr;

#ifdef	INODE_PAGER_DEBUG
	if (inode_pager_debug) {
		printk("inode_object_init: obj 0x%x, control 0x%x\n",
		       mem_obj, mem_obj_control);
	}
#endif	/* INODE_PAGER_DEBUG */

	imo = MO_TO_IMO(mem_obj);
	ASSERT(imo->imo_mem_obj_control == MACH_PORT_NULL);
	ASSERT(imo->imo_urefs == 0);

	imo->imo_mem_obj_control = mem_obj_control;
	imo->imo_urefs = 1;
	imo_ref(imo);	/* 1 ref for the kernel */

	if (imo->imo_cacheable) {
		/* take a reference for the microkernel's cache */
		imo_ref(imo);
	}

	/*
	 * Tell the micro-kernel that the memory object is ready on our side.
	 */
	attributes.copy_strategy = imo->imo_copy_strategy;
	if (imo->imo_inode != NULL) {
		attributes.cluster_size = PAGE_SIZE;	/* XXX ? */
	} else {
		attributes.cluster_size = 0;	/* default cluster size */
	}
	attributes.may_cache_object = imo->imo_cacheable;
	attributes.temporary = FALSE;
	kr = memory_object_change_attributes(mem_obj_control,
					     MEMORY_OBJECT_ATTRIBUTE_INFO,
					     (memory_object_info_t) &attributes,
					     MEMORY_OBJECT_ATTR_INFO_COUNT,
					     MACH_PORT_NULL);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr, ("mo_init: mo_change_attributes"));
		panic("inode_object_init: change_attributes");
	}

	return KERN_SUCCESS;
}

kern_return_t
inode_object_lock_completed(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length)
{
	panic("inode_object_lock_completed");
}

kern_return_t
inode_object_supply_completed(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_size_t		length,
	kern_return_t		result,
	vm_offset_t		error_offset)
{
	panic("inode_object_supply_completed");
}

kern_return_t
inode_object_synchronize(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control,
	vm_offset_t		offset,
	vm_offset_t		length,
	vm_sync_t		flags)
{
	struct i_mem_object	*imo;
	kern_return_t	kr;

#ifdef	INODE_PAGER_DEBUG
	if (inode_pager_debug) {
		printk("inode_object_synchronize: obj 0x%x control 0x%x "
		       "offset 0x%x length 0x%x flags 0x%x\n",
		       mem_obj, mem_obj_control, offset, length, flags);
	}
#endif	/* INODE_PAGER_DEBUG */

	imo = inode_pager_check_request(mem_obj, mem_obj_control);

	kr = memory_object_synchronize_completed(mem_obj_control,
						offset,
						length);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("inode_object_synchonize: "
			     "m_o_synchonize_completed"));
	}

	return KERN_SUCCESS;
}

kern_return_t
inode_object_terminate(
	memory_object_t		mem_obj,
	memory_object_control_t	mem_obj_control)
{
	struct i_mem_object	*imo;
	kern_return_t		kr;

#ifdef	INODE_PAGER_DEBUG
	if (inode_pager_debug) {
		printk("inode_object_terminate: obj 0x%x control 0x%x\n",
		       mem_obj, mem_obj_control);
	}
#endif	/* INODE_PAGER_DEBUG */

	imo = MO_TO_IMO(mem_obj);
	imo->imo_mem_obj_control = MACH_PORT_NULL;
	imo->imo_urefs = 0;

	if (imo->imo_attrchange) {
		imo->imo_attrchange = FALSE;
		wake_up(&imo->imo_attrchange_wait);
	} 

	imo_unref(imo);	/* get rid of kernel's reference */

	kr = mach_port_destroy(mach_task_self(), mem_obj_control);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("mo_terminate: mach_port_destroy(0x%x)",
				    mem_obj_control));
		panic("mo_terminate: can't destroy control port");
	}

	return KERN_SUCCESS;
}

#define memory_object_server		inode_object_server
#define memory_object_server_routine	inode_object_server_routine
#define memory_object_subsystem		inode_object_subsystem

#define memory_object_init		inode_object_init
#define memory_object_terminate		inode_object_terminate
#define memory_object_data_request	inode_object_data_request
#define memory_object_data_unlock	inode_object_data_unlock
#define memory_object_lock_completed	inode_object_lock_completed
#define memory_object_supply_completed	inode_object_supply_completed
#define memory_object_data_return	inode_object_data_return
#define memory_object_synchronize	inode_object_synchronize
#define memory_object_change_completed	inode_object_change_completed
#define memory_object_discard_request	inode_object_discard_request

#include "memory_object_server.c"
