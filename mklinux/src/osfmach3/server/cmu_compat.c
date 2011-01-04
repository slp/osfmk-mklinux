/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/port.h>
#include <mach/mach_interface.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/mach_init.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>

#include <asm/ptrace.h>
#include <asm/segment.h>

extern mach_port_t	device_server_port;
extern mach_port_t	privileged_host_port;

/*
 *	Routine:	task_by_pid
 *	Purpose:
 *		Get the task port for another "process", named by its
 *		process ID on the same host as "target_task".
 *
 *		Only permitted to privileged processes, or processes
 *		with the same user ID.
 */
int
sys_task_by_pid(
#if	defined(i386)
	struct pt_regs	regs
#elif	defined(PPC)
	unsigned long arg1
#endif
) {
	mach_port_t		target_task;
	struct task_struct	*p;
	int			max_tries;
	mach_port_t		port, name;
	kern_return_t		kr;
	long			arg;

#if	defined(i386)
	/* get the arg from the stack: it's a Mach syscall ! */
	arg = get_fs_long(regs.esp + 4);
#elif	defined(PPC)
	arg = arg1;
#else
	panic("sys_task_by_pid: need to get arg from the stack or whereever");
#endif

	switch (arg) {
	    case -2:
		if (!suser()) 
			return -EACCES;
		port = device_server_port;	/* device server port */
		break;
	    case -1:
		if (!suser())
			return -EACCES;
		port = privileged_host_port;	/* privileged host port */
		break;
	    case 0:
		if (!suser())
			return -EACCES;
		port = mach_task_self();	/* server task port */
		break;
	    default:
		for_each_task(p) {
			if (p && p->pid == arg)
				break;
		}
		if (p == &init_task)
			return -ESRCH;
		if (!suser() &&
		    (current->uid != p->uid ||
		     current->euid != p->uid))
			return -EACCES;
		port = p->osfmach3.task->mach_task_port;
		break;
	}

	/* insert the selected port into the caller's IPC space */
	target_task = current->osfmach3.task->mach_task_port;
	name = MACH_PORT_NULL;
	max_tries = 20;
	do {
		/* get a clue for a possible port name */
		kr = mach_port_allocate(target_task,
					MACH_PORT_RIGHT_DEAD_NAME,
					&name);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("sys_task_by_pid: mach_port_allocate"));
			name = MACH_PORT_NULL;
			break;
		}
		/* make this port name available again */
		kr = mach_port_deallocate(target_task,
					  name);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr, 
				    ("sys_task_by_pid: mach_port_deallocate"));
			name = MACH_PORT_NULL;
			break;
		}

		/* try to insert the one we want */
		kr = mach_port_insert_right(target_task,
					    name,
					    port,
					    MACH_MSG_TYPE_COPY_SEND);
		if (kr == KERN_SUCCESS) {
			/* success ! */
			break;
		}
		if (kr == KERN_RIGHT_EXISTS) {
			/* 
			 * Problem: there's no interface that can tell us
			 * under which name the target_task knows the port
			 * right !
			 * So we have to scan its IPC space :-(
			 */
			mach_port_array_t	names;
			mach_msg_type_number_t	ncount;
			mach_port_type_array_t	types;
			mach_msg_type_number_t	tcount;
			int			i;
			mach_port_t		right;
			mach_msg_type_name_t	type;

			name = MACH_PORT_NULL;
			kr = mach_port_names(target_task,
					     &names, &ncount,
					     &types, &tcount);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("sys_task_by_pid: mach_port_names")
					    );
				return MACH_PORT_NULL;
			}
			for (i = 0; i < ncount; i++) {
				kr = mach_port_extract_right(
							target_task,
							names[i],
							MACH_MSG_TYPE_PORT_SEND,
							&right,
							&type);
				if (kr == KERN_SUCCESS) {
					kr = mach_port_insert_right(
						    target_task,
						    names[i],
						    right,
						    MACH_MSG_TYPE_COPY_SEND);
					if (right == port) {
						/* found ! */
						name = names[i];
						kr = mach_port_insert_right(
						       target_task,
						       name, port,
						       MACH_MSG_TYPE_COPY_SEND);
						if (kr != KERN_SUCCESS)
							name = MACH_PORT_NULL;
						(void) mach_port_deallocate(
							mach_task_self(),
							right);
						break;
					}
					kr =  mach_port_deallocate(
							mach_task_self(),
							right);
					if (kr != KERN_SUCCESS) {
						MACH3_DEBUG(1, kr,
							    ("sys_task_by_pid: "
							     "mach_port_"
							     "deallocate"));
					}
				}
			}
			kr = vm_deallocate(mach_task_self(),
					   (vm_address_t) names,
					   (sizeof *names) * ncount);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("sys_task_by_pid: "
					     "vm_deallocate(names)"));
			}
			kr = vm_deallocate(mach_task_self(),
					   (vm_address_t) types,
					   (sizeof *types) * tcount);
			if (kr != KERN_SUCCESS) {
				MACH3_DEBUG(1, kr,
					    ("sys_task_by_pid: "
					     "vm_deallocate(types)"));
			}
		}
	} while (name == MACH_PORT_NULL && max_tries-- > 0);

	if (max_tries == 0) {
		printk("sys_task_by_pid(%ld): failed to insert right\n", arg);
		return MACH_PORT_NULL;
	}

	return (int) name;
}
