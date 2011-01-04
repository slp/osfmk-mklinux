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
/*
 *	Functions to manipulate RPC subsystem descriptions.
 */

#include <mach/port.h>
#include <mach/kern_return.h>
#include <kern/task.h>
#include <kern/lock.h>
#include <kern/spl.h>
#include <kern/zalloc.h>
#include <kern/ipc_subsystem.h>
#include <kern/subsystem.h>
#include <kern/misc_protos.h>

#define SUBSYSTEM_MIN_SIZE	12
#define SUBSYSTEM_MAX_SIZE	(2*1024*1024)	/* What value is correct? */

void
subsystem_init(
	 void)
{
	/* Nothing to do on bootstrap, at the moment. */
}

/*
 *	Routine:	mach_subsystem_create
 *	Purpose:
 *		Create a new RPC subsystem.
 *	Conditions:
 *		Nothing locked.  If successful, the subsystem is returned
 *		unlocked.  (The caller has a reference.)
 *	Returns:
 *		KERN_SUCCESS		The subsystem is allocated.
 *		KERN_INVALID_TASK	The task is dead.
 *		KERN_RESOURCE_SHORTAGE	Couldn't allocate memory.
 */

kern_return_t
mach_subsystem_create(
	register task_t parent_task,
	user_subsystem_t	user_subsys,
	mach_msg_type_number_t	user_subsysCount,
	subsystem_t		*subsystem_p)
{
	int		i;
	subsystem_t	new_subsystem;
	kern_return_t	kr;
	boolean_t	deallocate = FALSE;
	vm_size_t	size;
	vm_offset_t	offset;
	int		num_routines;
	boolean_t	bad_arg = FALSE;

	if (parent_task == TASK_NULL)
		return(KERN_INVALID_ARGUMENT);

	if (user_subsysCount < SUBSYSTEM_MIN_SIZE ||
				user_subsysCount > SUBSYSTEM_MAX_SIZE)
		return(KERN_INVALID_ARGUMENT);

	/*
	 *	Allocate a subsystem and initialize:
	 */

	size = (vm_size_t)user_subsysCount + sizeof(struct subsystem) -
					     sizeof(struct rpc_subsystem);
	new_subsystem = (subsystem_t) kalloc(size);

	if (new_subsystem == 0)
		return(KERN_RESOURCE_SHORTAGE);

	new_subsystem->task = parent_task;
	new_subsystem->ref_count = 1;	/* A reference for our caller */
	new_subsystem->size = size;
	subsystem_lock_init(new_subsystem);

	/* Copy the user subsystem data to a permanent place: */
	bcopy((char *)user_subsys, (char *)&(new_subsystem->user),
						(int)user_subsysCount);

	/* Validate the user-specified fields of the subsystem: */

	num_routines = new_subsystem->user.end - new_subsystem->user.start;
	if (num_routines < 0 ||
			(char *)&new_subsystem->user.routine[num_routines] >
			(char *)&new_subsystem->user + (int)user_subsysCount
	   ) {
		kfree((vm_offset_t)new_subsystem, size);
		return(KERN_INVALID_ADDRESS);
	}

	/* The following is for converting the user pointers in the
	 * subsystem struct to kernel pointers:
	 */
	offset = (char *)&new_subsystem->user -
		 (char *)new_subsystem->user.base_addr; /* The user addr */

	for (i = 0; i < num_routines; i++) {
		routine_descriptor_t routine = &new_subsystem->user.routine[i];

		/* If this is a "skip" routine, ignore it: */
		if (!routine->impl_routine)
			continue;
		
		/* Convert the user arg_descr pointer to a kernel pointer: */
		routine->arg_descr = (routine_arg_descriptor_t)
				     ((char *)routine->arg_descr + offset);

		if (routine->argc > 1000000 ||
				    routine->argc < routine->descr_count) {
			bad_arg = TRUE;
			break;
		}
		/* Validate that the arg_descr field is within the part of
		 * the struct that follows the routine array: */
		if ((char *)&routine->arg_descr[0] <
		    (char *)&new_subsystem->user.routine[num_routines]
					||
		    (char *)&routine->arg_descr[routine->descr_count] >
		    (char *)&new_subsystem->user + (int)user_subsysCount
		   ) {
			printf("Arg descr out of bounds: arg_descr=%x, &routine.num_routines=%x\n",
				&routine->arg_descr[0], &new_subsystem->user.routine[num_routines]);
			printf("		new_subsys->user + subsysCount = %x\n",
						    (char *)&new_subsystem->user + (int)user_subsysCount);
#if 	MACH_DEBUG && MACH_KDB
			subsystem_print(new_subsystem);
			/* Not all of the arg_descr pointers have necessarily
			   been corrected, but this just means that we print
			   the arg_descr from the user's input subsystem
			   instead of the copy we are building.  */
#endif	/* MACH_DEBUG && MACH_KDB */
			bad_arg = TRUE;
			break;
		}
	}
	if (bad_arg) {
		kfree((vm_offset_t)new_subsystem, size);
		return(KERN_INVALID_ADDRESS);
	}

	/* Convert the user base address to a kernel address: */
	new_subsystem->user.base_addr = (vm_address_t)&new_subsystem->user;

	/* Make the user subsystem point at the containing system data
	 * structure, so we can get from a port (which points to the user
	 * subsystem data) to the system subsystem struct:
	 */
	new_subsystem->user.subsystem = new_subsystem;

	ipc_subsystem_init(new_subsystem);

	task_lock(parent_task);
	if (parent_task->active) {
		parent_task->subsystem_count++;
		queue_enter(&parent_task->subsystem_list, new_subsystem,
					subsystem_t, subsystem_list);
	} else
		deallocate = TRUE;
	task_unlock(parent_task);
	
	if (deallocate) {
		/* release ref we would have given our caller */
		subsystem_deallocate(new_subsystem);
		return(KERN_INVALID_TASK);
	}

	ipc_subsystem_enable(new_subsystem);

	*subsystem_p = new_subsystem;
	return(KERN_SUCCESS);
}

/*
 *	Routine:	subsystem_reference
 *	Purpose:
 *		Increments the reference count on a subsystem.
 *	Conditions:
 *		Nothing is locked.
 */
void
subsystem_reference(
        register subsystem_t       subsystem)
{
        spl_t           s;

        if (subsystem == SUBSYSTEM_NULL)
                return;

        s = splsched();
        subsystem_lock(subsystem);
        subsystem->ref_count++;
        subsystem_unlock(subsystem);
        splx(s);
}



/*
 *	Routine:	subsystem_deallocate
 *	Purpose:
 *		Decrements the reference count on a subsystem.  If 0,
 *		destroys the subsystem.  Must have no ports registered on it
 *		when it is destroyed.
 *	Conditions:
 *		The subsystem is locked, and
 *		the caller has a reference, which is consumed.
 */

void
subsystem_deallocate(
	subsystem_t	subsystem)
{
	task_t		task;
        spl_t           s;

        if (subsystem == SUBSYSTEM_NULL)
                return;

        s = splsched();
        subsystem_lock(subsystem);
        if (--subsystem->ref_count > 0) {
                subsystem_unlock(subsystem);
                splx(s);
                return;
        }

	/*
	 * Count is 0, so destroy the subsystem.  Need to restore the
	 * reference temporarily, and lock the task first:
	 */
	ipc_subsystem_disable(subsystem);

	subsystem->ref_count = 1;
	subsystem_unlock(subsystem);
	splx(s);

	task = subsystem->task;
	task_lock(task);
        s = splsched();
        subsystem_lock(subsystem);

	/* Check again, since we temporarily unlocked the subsystem: */
	if (--subsystem->ref_count == 0) {

		task->subsystem_count--;
		queue_remove(&task->subsystem_list, subsystem, subsystem_t,
							subsystem_list);
		kfree((vm_offset_t) subsystem, subsystem->size);
		task_unlock(task);
		return;
	}

	ipc_subsystem_enable(subsystem);

	subsystem_unlock(subsystem);
	splx(s);
	task_unlock(task);
}


#include <mach_kdb.h>
#if	MACH_KDB

#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_print.h>
#include <ddb/db_command.h>

#define	printf	kdbprintf

/*
 *	Routine:	subsystem_print
 *	Purpose:
 *		Pretty-print a subsystem for kdb.
 */

void rpc_subsystem_print(rpc_subsystem_t subsys);

void
subsystem_print(
	subsystem_t	subsystem)
{
	extern int indent;

	iprintf("subsystem 0x%x\n", subsystem);

	indent += 2;

	iprintf("ref %d size %x task %x port %x\n", subsystem->ref_count,
		subsystem->size, subsystem->task, subsystem->ipc_self);
	rpc_subsystem_print(&subsystem->user);

/*	ipc_object_print(&port->ip_object);
 *	iprintf("receiver=0x%x", port->ip_receiver);
 *	printf(", receiver_name=0x%x\n", port->ip_receiver_name);
 */
	indent -=2;
}

struct flagnames {
    char *name;
    int bit;
} arg_type_names[] = {
    "port", MACH_RPC_PORT, "array", MACH_RPC_ARRAY,
    "variable", MACH_RPC_VARIABLE, "in", MACH_RPC_IN, "out", MACH_RPC_OUT,
    "pointer", MACH_RPC_POINTER, "phys_copy", MACH_RPC_PHYSICAL_COPY,
    "virt_copy", MACH_RPC_VIRTUAL_COPY, "deallocate", MACH_RPC_DEALLOCATE,
    "onstack", MACH_RPC_ONSTACK, "bounded", MACH_RPC_BOUND,
};

void
rpc_subsystem_print(
	rpc_subsystem_t	subsys)
{
    int i, num_routines;

    iprintf("rpc_subsystem 0x%x\n", subsys);

    indent += 2;

    num_routines = subsys->end - subsys->start;
    iprintf("start %d end %d (%d routines) maxsize %x base %x\n",
	    subsys->start, subsys->end, num_routines, subsys->maxsize,
	    subsys->base_addr);
    for (i = 0; i < num_routines; i++) {
	routine_descriptor_t routine = subsys->routine + i;
	routine_arg_descriptor_t args = routine->arg_descr;
	int j, type, disposition;
	struct flagnames *n;
	char *sep;

	iprintf("%x #%d:", routine, subsys->start + i);
	if (routine->impl_routine == 0) {
	    printf(" skip\n");
	    continue;
	}
	printf("\n");
	indent += 2;
	iprintf("impl ");
	db_printsym((db_expr_t) routine->impl_routine, DB_STGY_PROC);
	printf("\n");
	iprintf("stub ");
	db_printsym((db_expr_t) routine->stub_routine, DB_STGY_PROC);
	printf("\n");
	iprintf("argc %d descr_count %d max_reply %x\n",
		routine->argc, routine->descr_count, routine->max_reply_msg);
	for (j = 0; j < routine->descr_count; j++) {
	    iprintf("%x desc %d: size %d count %d offset %x type", &args[j], j,
		    args[j].size, args[j].count, args[j].offset);
	    sep = " ";
	    type = args[j].type;
	    for (n = arg_type_names; n->name != 0; n++) {
		if (type & n->bit) {
		    printf("%s%s", sep, n->name);
		    sep = "|";
		    type &= ~n->bit;	/* Might have an alias */
		}
	    }
#define NAME_MASK	(3 << NAME_SHIFT)	/* XXX magic numbers */
#define ACTION_MASK	(3 << ACTION_SHIFT)
#define DISPOSITION_MASK (NAME_MASK | ACTION_MASK)
	    disposition = type & DISPOSITION_MASK;
	    type &= ~DISPOSITION_MASK;
	    if (sep[0] != '|' || type != 0)
		printf("%s%x", sep, type);
	    switch (disposition & ACTION_MASK) {
	    case MACH_RPC_MOVE: printf(" move"); break;
	    case MACH_RPC_COPY: printf(" copy"); break;
	    case MACH_RPC_MAKE: printf(" make"); break;
	    }
	    switch (disposition & NAME_MASK) {
	    case MACH_RPC_RECEIVE:	printf(" receive"); break;
	    case MACH_RPC_SEND:		printf(" send"); break;
	    case MACH_RPC_SEND_ONCE:	printf(" send-once"); break;
	    }
	    printf("\n");
	}
	indent -= 2;
    }

    indent -= 2;
}

void
db_show_subsystem(
	db_expr_t	addr,
	boolean_t	have_addr,
	db_expr_t	count,
	char		*modif)
{
	if (!have_addr || addr == 0) {
		db_printf("No subsystem\n");
		return;
	}
	if (db_option(modif, 'r'))
		rpc_subsystem_print((rpc_subsystem_t) addr);
	else
		subsystem_print((subsystem_t) addr);
}

#endif	/* MACH_KDB || MACH_DEBUG */
