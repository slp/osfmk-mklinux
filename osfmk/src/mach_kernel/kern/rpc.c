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
 *	Mach RPC implementation
 */

#include <mach_prof.h>
#include <mach_rt.h>

#include <mach/rpc.h>
#include <mach/kern_return.h>
#include <mach/mach_server.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_right.h>
#include <ipc/ipc_space.h>
#include <kern/assert.h>
#include <kern/etap_macros.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/zalloc.h>
#include <kern/cpu_number.h>
#include <kern/syscall_sw.h>
#include <kern/macro_help.h>
#include <kern/misc_protos.h>
#if 0
#include <kern/rpc_inline.c>
#endif


#if	!MACH_RT
#define	IP_RT(port)	(0)
#endif	/* !MACH_RT */

extern int switch_act_swapins;


/*
 * Forward refs to file-local functions
 */
boolean_t bbcopy(const char *, char *, vm_size_t);

/*
 *	bbcopy:
 *
 *	Make bcopy return success reliably (sigh).
 */
boolean_t
bbcopy(
	const char      *from,
	char            *to,
	vm_size_t        count)
{
	bcopy(from, to, count);
	return (0);
}


/*
 *	mach_rpc_trap:
 *
 *	Main entry point for Mach RPC service. This is the invoke side;
 *	see mach_rpc_return_trap for the reply side.
 *	
 *	XXX TODO:
 * 
 *	RPC_COW_SIZE is machine dependent.
 *
 *	When possible, argument data (arrays, space which out param ptrs
 *	reference) should be clustered on the stack and copied in en masse.
 *	Doing piecemeal copyin's of these params is sub-optimal.
 *
 *	Not handled yet: "canned" signatures, machine specific register
 *	optimizations, or full signature copying.
 *
 *	Not as designed yet: trap arguments and signature buffer should
 *	be stored in the thread control block (tcb), NDR header and rpc
 *	arguments should be stored on a per-shuttle stack.
 */

kern_return_t
mach_rpc_trap(
	mach_port_t		dest_name,
	mach_rpc_id_t		routine_num,
	mach_rpc_signature_t	sig_ptr,
	mach_rpc_size_t		sig_size)
{
	int				i;
	thread_t			cur_thread;
	thread_act_t			old_act, new_act;
	mach_port_t			name, out_name;
	ipc_port_t			port, orig_port;
	ipc_space_t			space;
	mig_impl_routine_t		server_func;
	rpc_subsystem_t			subsystem;
	vm_offset_t			new_sp, stack_top;
	vm_size_t			state_size;
	int				rpc_arg_buf_data[RPC_KBUF_SIZE];
	int				*rpc_arg_buf;
	int				*new_argv, rc, argc;
	vm_size_t			nbyte;
	char				*argv;
	routine_descriptor_t		routine;
        routine_arg_descriptor_t	dsc;
	rpc_copy_state_t		state;
	kern_return_t			kr;
        unsigned int                    point, direct;
        copyfunc_t              	infunc;
        copyfunc_t              	outfunc;
	etap_data_t			probe_data;


	/*
	 *	Check input arguments.
	 */
        if (!MACH_PORT_VALID(dest_name)
            /* || sig_ptr == NULL || sig_size <= 0 */) {
                return KERN_INVALID_ARGUMENT;
        }

	/*
 	 *	Convert the destination port.
	 */
	space = current_space();
	kr = ipc_object_copyin(space, dest_name, MACH_MSG_TYPE_COPY_SEND,
			       (ipc_object_t *) &port);

        if (kr != KERN_SUCCESS) {
                rpc_error( ("mach_rpc_trap: port copyin err"), ERR_INFO );
                return kr;
        }

	if (!IP_VALID(port)) {
		return KERN_INVALID_ARGUMENT;
	}

	/*
	 *	Find the subsystem associated with the port.
	 */
	subsystem = port->ip_subsystem;

	/*
	 * 	If this is a a port with no registered subsystem,
	 *	return a convenient error code so the mig stubs
	 *	can retry using an IPC message.
	 */
	if (subsystem == NULL || routine_num < subsystem->start || 
	    routine_num >= subsystem->end) {
		return KERN_NO_ACCESS;
	}

	/*
	 * 	Check if this is a norma proxy port. Return a convenient
	 *	error code so the mig stubs can retry using an IPC message.
	 */
	if (IP_IS_REMOTE(port)) {
		return KERN_NO_ACCESS;
	}

	/* Make sure there is a valid server work function.  Note that use
	 * of the "skip" feature of MIG, or mach_subsystem_join, can create
	 * subsystems in which there are gaps in the routine array, expressed
	 * as NULL impl_routine pointers:
	 */
	routine = &subsystem->routine[routine_num - subsystem->start];
	server_func = routine->impl_routine;
	if (server_func == NULL)
		return KERN_INVALID_ARGUMENT;

	/*
	 *	Obtain a thread into which to migrate the shuttle.
  	 *
	 *	Use the port set instead of a regular port, if there is one.
	 *	This relies on the fact that the port and port set structures
	 *	are laid out the same way. Empty activation pools can be assoc-
	 *	iated with port sets, but subsystems are never associated with
	 *	a port set (always a singular port).
	 */
	ip_lock(port);

	orig_port = port;
	if (port->ip_pset != IPS_NULL) {
		port = (ipc_port_t)(port->ip_pset);
		ip_lock(port);
		ip_unlock(orig_port);
	}

        for (;;) {
		new_act = thread_pool_get_act(port);

		ip_unlock(port);

		if (new_act == THR_ACT_NULL) {		/* port destroyed */
			kr = MACH_RCV_PORT_DIED;
			return(kr);
		}

#if		THREAD_SWAPPER
		if (new_act->swap_state == TH_SW_IN ||
		    new_act->swap_state == TH_SW_UNSWAPPABLE) {
			break;		/* got one ! */
		}
		act_locked_act_reference(new_act);
		ip_lock(port);
		thread_pool_put_act(new_act);
		(void)thread_swapin_blocking(new_act);
		act_locked_act_deallocate(new_act);
		act_unlock(new_act);
#else		/* THREAD_SWAPPER */
		break;
#endif		/* THREAD_SWAPPER */
	}

	/*
	 *	Lock current shuttle and retrieve top activation.
	 */
	cur_thread = current_thread();

	rpc_lock(cur_thread);
	old_act = cur_thread->top_act;

        if (old_act->kernel_loaded) {
                infunc = bbcopy;
        }
        else {
                infunc = copyin;
        }

	ETAP_DATA_LOAD(probe_data[0], dest_name);
	ETAP_DATA_LOAD(probe_data[1], routine_num);
	ETAP_DATA_LOAD(probe_data[2], old_act->task);
	ETAP_DATA_LOAD(probe_data[3], new_act->task);
	ETAP_PROBE_DATA(ETAP_P_RPC,
			EVENT_BEGIN,
			cur_thread,
			&probe_data,
			ETAP_DATA_SIZE);

	old_act->r_subsystem = subsystem;
	old_act->r_port_flags = (vm_offset_t) orig_port->ip_flags;

	/*
	 *	Find argc and user direct args. User args begin two
	 *	words above the frame pointer (past saved ebp and 
	 *	return address) on the user stack.
	 */
	argc = routine->argc;
	argv = MACH_RPC_ARGV(old_act);
#if	MACH_RPC_DEBUG > 2
	rpc_error( ("mach_rpc_trap: argv is 0x%x", argv), ERR_INFO );
#endif	/* MACH_RPC_DEBUG */

	/*
	 *	Ensure that the rpc copy state record is properly sized.
	 *	Default size is chosen to avoid kalloc in most cases.
	 */
	if (routine->descr_count > RPC_DESC_COUNT) {
		state_size = routine->descr_count * 
			     sizeof(rpc_copy_state_data_t);
		state = (rpc_copy_state_t) KALLOC(state_size, IP_RT(port));
		if (state == NULL) {
			rpc_error( ("kalloc failed"), ERR_GREEN );
			return KERN_RESOURCE_SHORTAGE;
		}
		bzero((char *)state, state_size);
		state[0].copy_type = COPY_TYPE_ALLOC_KRN;
		state[0].alloc_addr = (vm_offset_t) state;
	} 
	else {	
		state_size = RPC_DESC_COUNT * sizeof(rpc_copy_state_data_t);
		state = old_act->r_state_data;
		bzero((char *)state, state_size);
	}

	/*
	 *	Save the rpc trap parameters in the thread_activation,
	 *	sometimes called the tcb. These are used during the reply.
	 */
#ifdef	notyet
	old_act->r_sig_ptr = (vm_offset_t) sig_ptr;
	old_act->r_sig_size = sig_size;
#endif
	old_act->r_routine_num = routine_num;
	old_act->r_state = state;

	/*
	 *	Copyin the rpc argument buffer. We need to save a copy of
	 *	the original direct arguments because we will be overwriting
	 *	OUT pointers with values for the server's address space.
	 */
	nbyte = argc * sizeof(int);
	old_act->r_arg_buf = old_act->r_arg_buf_data;
	rpc_arg_buf = rpc_arg_buf_data;

	if (argc > 1) {
		if (nbyte > (RPC_KBUF_SIZE * sizeof(int))) {
			rpc_arg_buf = (int *) KALLOC(nbyte, IP_RT(port));
			if (rpc_arg_buf == NULL) {
                                rpc_error( ("kalloc failed"), ERR_GREEN );
                                kr = KERN_RESOURCE_SHORTAGE;
                                goto finish;
			}
		}

		if ((*infunc)(argv, (char *)rpc_arg_buf, nbyte)) {
			rpc_error( ("copyin fail"), ERR_GREEN );
			kr = KERN_FAILURE;	
			goto finish;
		}

		/*
	 	 *	The converted destination port is the first element in
	 	 *	the argument buffer.
	 	 */
		rpc_arg_buf[0] = (int) orig_port;

		/*
		 *	Make a copy of the original direct args buffer.
		 */
		bcopy((char *)rpc_arg_buf, (char *)old_act->r_arg_buf, nbyte);

		/*
	 	 *	Use signature to translate the argument buffer 
		 *	coming in. This will also copyin (gather) any arrays.
	 	 */ 
		kr = rpc_invke_args_in(routine, state, rpc_arg_buf, infunc);
		if (kr != KERN_SUCCESS) {
			rpc_error( ("rpc_invke_args_in %d", kr), ERR_GREEN );
			goto finish;
		}
	} 
	else {
		/*
                 *      The converted destination port is the first element in
                 *      the argument buffer.
                 */
                rpc_arg_buf[0] = (int) orig_port;
	}

	/*
	 *	Switch to the new activation's map.
	 */
	old_act = switch_act(new_act);

	new_sp = new_act->user_stack;

        if (new_act->kernel_loaded) {
                outfunc = bbcopy;
        }
        else {
                outfunc = copyout;
        }

	act_unlock(new_act);

	/*
	 *	Translate destination port.
	 */
	out_name = orig_port->ip_receiver_name;

	/*
	 *	Use signature to translate the argument buffer going out.
	 *	This will also copyout (scatter) any array arguments. Then
	 *	copyout the rpc argument buffer to the user space server
	 *	stack.
	 */ 
	if (argc > 1) {
        	kr = rpc_invke_args_out(routine, state, rpc_arg_buf, 
				        (int **)&new_sp, outfunc);
		if (kr != KERN_SUCCESS) {
			rpc_error( ("rpc_invke_args_out %d", kr), ERR_YELLOW );
			(void) switch_act(THR_ACT_NULL);
			goto finish;
		}
	}
	rpc_arg_buf[0] = out_name;

	new_argv = (int *)new_sp - argc;
	new_sp = (vm_offset_t)new_argv;
	old_act->r_new_argv = (vm_offset_t) new_argv;

	if ((*outfunc)((char *)rpc_arg_buf, (char *)new_argv, nbyte)) {
		rpc_error( ("copyout fail"), ERR_GREEN );
		kr = KERN_FAILURE;
		(void) switch_act(THR_ACT_NULL);
		goto finish;
	}

	/*
	 *	No longer need the argument buffer. Free it.
	 */
	if (rpc_arg_buf != rpc_arg_buf_data) {
		KFREE((vm_offset_t)rpc_arg_buf, nbyte, IP_RT(port));
		rpc_arg_buf = rpc_arg_buf_data;
	}

	/*
	 *	At this point, we are committed to persistent changes we've 
	 *	made to the server's address space. That is, when unwinding 
  	 *	state for error processing, we will not deallocate or otherwise
	 *	change the server's address space. So remove persistent server 
	 *	alloc records from the state vector.
	 *
	 *	Only want to clear the state record for mappings which should 
	 *	persist in the server. Temporary mappings which we setup to
	 *	optimize IN arguments should be left alone, since rpc_reply_
	 *	args_in needs to remove them from the server's address space. 
 	 *	Persist if and only if "pointer and (IN or INOUT)".
	 */
	dsc = routine->arg_descr;
        for ( i = 1 ; i < routine->descr_count ; i++ ) {
            switch( state[i].copy_type ) {
		case COPY_TYPE_ALLOC_SVR:
                case COPY_TYPE_VIRTUAL_OUT_SVR:
                	direct = RPC_FIELD(dsc[i].type, DIRECT_SHIFT, 
					   LAST_DIRECT_BIT);
			point  = RPC_FIELD(dsc[i].type, POINTER_SHIFT, 
					   LAST_POINTER_BIT);
			if (point && (direct & MACH_RPC_IN)) {
				state[i].copy_type = COPY_TYPE_NO_COPY;
				state[i].alloc_addr = 0;
			}
                        break;

                default:
                        break;
            }
        }

	/*
	 *	Setup user stack pointer and return address from
	 *	trap. Switch back to pcb stack and return from trap.
	 *	We use new_sp-4 to simulate pushing the return address
	 *	on the stack for a call instruction. 
	 *
	 *	Set mach_rpc_return_wrapper() as the calling PC -- not
	 *	useful (though benign) if caller is in a user-space
	 *	task, but just what we want if caller is collocated.
	 */	
	rc = KERN_SUCCESS;
	stack_top = (vm_offset_t) USER_REGS(new_act);
	*((int *)new_sp - 1) = (int)mach_rpc_return_wrapper;

	rpc_unlock(cur_thread);

	assert( new_sp < new_act->user_stack );
	assert( new_sp > (new_act->user_stack - new_act->user_stack_size) );
	/* log_thread_action("rpc invoke: rpc_upcall", 0, 0, 0); */

	rpc_upcall(stack_top, new_sp-4, (vm_offset_t) server_func, rc);
	/* NOTREACHED */

finish:

	if (state[0].copy_type == COPY_TYPE_ALLOC_KRN) {
		KFREE((vm_offset_t)state, state_size, IP_RT(port));
	}

	if (rpc_arg_buf != rpc_arg_buf_data) {
		KFREE((vm_offset_t)rpc_arg_buf, nbyte, IP_RT(port));
	}

	rpc_unlock(cur_thread);

	return kr;
}


/*
 *	mach_rpc_return_trap
 *
 *	Re-enter the kernel on the return path from an RPC. Transfer
 *	result parameters, set return code, and return to the client.
 */

#define	DSCSIZE	(dsc[i].count * dsc[i].size)
#define PORT_VALID(name)	MACH_PORT_VALID((mach_port_t)(name))

kern_return_t
mach_rpc_return_trap(void)
{
	int				i;
        char                    	*argv;
	thread_t			cur_thread;
	thread_act_t			svr_act, cln_act;
	kern_return_t			kr = KERN_SUCCESS;
	kern_return_t			ret;
	vm_offset_t			pcb_stack, old_stack;
	vm_size_t			size, state_size;
	struct ipc_port			port_data;
	ipc_port_t			port = &port_data;
	mach_rpc_id_t			routine_num;
        rpc_subsystem_t         	subsystem;
        routine_descriptor_t    	routine;
	routine_arg_descriptor_t	dsc;
	rpc_copy_state_t		state;
	int				*cln_arg_buf, argc;
        int                     	svr_arg_buf_data[RPC_KBUF_SIZE];
        int                     	*svr_arg_buf;
	unsigned int			uesp, ueip; 
	unsigned int			type, offset;
	vm_size_t			nbyte;
        copyfunc_t                      infunc;
        copyfunc_t                      outfunc;
	etap_data_t			probe_data;


	/*
	 *	Initialize thread and act pointers.
	 */
	cur_thread = current_thread();
	svr_act = cur_thread->top_act;
	svr_arg_buf = svr_arg_buf_data;

	/* If we were orphaned and there is nowhere to return to... */
	if ((svr_act->alerts & CLIENT_TERMINATED)
	    && (svr_act->lower == THR_ACT_NULL)) {
		thread_terminate_self();
		/*NOTREACHED*/
	}

	assert(svr_act->lower != NULL);
        cln_act = svr_act->lower;

        if (svr_act->kernel_loaded) {
                infunc = bbcopy;
        }
        else {
                infunc = copyin;
        }

	rpc_lock(cur_thread);

	/*
	 *	Find the routine signature, rpc state and server args.
	 */
	state = cln_act->r_state;
	cln_arg_buf = (int *) cln_act->r_arg_buf;
	routine_num = cln_act->r_routine_num;

	port->ip_flags = (ipc_port_flags_t) cln_act->r_port_flags;
        subsystem = cln_act->r_subsystem;
	assert(subsystem != NULL);
        routine = &subsystem->routine[routine_num - subsystem->start];

	/* Check for terminated bit (and eventually alerts?) */
	if ( cln_act->alerts & SERVER_TERMINATED ) {
		/* clear the server-terminated bit */
		cln_act->alerts &= ~SERVER_TERMINATED;

		/* switch back to the client activation */
		svr_act = switch_act(THR_ACT_NULL);

		MACH_RPC_RET(cln_act) = KERN_RPC_SERVER_TERMINATED;

		rpc_unlock(cur_thread);

		(void)mach_rpc_return_error();
		/*NOTREACHED*/
	}

        argc = routine->argc;
        argv = MACH_RPC_ARGV(svr_act);

        assert(cln_act->r_new_argv == (vm_offset_t) argv);
	assert(state != NULL);
	assert(cln_arg_buf != NULL);
        assert( (vm_offset_t)argv < svr_act->user_stack );
        assert( (vm_offset_t)argv > (svr_act->user_stack - 
		svr_act->user_stack_size) );

#if	MACH_RPC_DEBUG > 2
	rpc_error( ("reply new_argv is 0x%x argv is 0x%x", 
		   svr_act->lower->r_new_argv,  argv), ERR_INFO );
#endif	/* MACH_RPC_DEBUG */

	/*
	 *	Retrieve the server return code. If the server returned
	 *	an error, i.e. not success, then don't process return 
	 *	arguments. Just return to client.
	 */
	ret = (kern_return_t) MACH_RPC_RET(svr_act);
	if (ret != KERN_SUCCESS) {
		(void) switch_act(THR_ACT_NULL);
		goto finish;
	}

        /*
         *      Copyin the rpc argument buffer. Don't need to check direct
  	 *	arg buffer size, since this was already checked on the invoke
	 *	side.
         */
        nbyte = argc * sizeof(int);

        if (argc > 1) {
                if (nbyte > (RPC_KBUF_SIZE * sizeof(int))) {
                        svr_arg_buf = (int *) KALLOC(nbyte, IP_RT(port));
			if (svr_arg_buf == NULL) {
				rpc_error( ("kalloc failed"), ERR_GREEN );
				ret = KERN_RESOURCE_SHORTAGE;
				(void) switch_act(THR_ACT_NULL);
				goto finish;
			}
                }

                if ((*infunc)(argv, (char *)svr_arg_buf, nbyte)) {
			rpc_error( ("copyin fail"), ERR_GREEN );
        		unwind_rpc_state(routine, state, svr_arg_buf);
                        /* return new_act to pool */
                        ret = KERN_FAILURE;
			(void) switch_act(THR_ACT_NULL);
                        goto finish;
                }

		/*
	 	 *	The converted destination port is the first element in
	 	 *	the argument buffer.
	 	 */
		svr_arg_buf[0] = (int) port;

                /*
                 *      Use signature to translate the argument buffer
                 *      coming in. This will also copyin (gather) any arrays.
                 */
                kr = rpc_reply_args_in(routine, state, svr_arg_buf, infunc);
                if (kr != KERN_SUCCESS) {
			rpc_error( ("rpc_reply_args_in %d", kr), ERR_YELLOW );
                        /* return new_act to pool */
			ret = kr;
			(void) switch_act(THR_ACT_NULL);
                        goto finish;
                }
        }
	else {
		/*
	 	 *	The converted destination port is the first element in
	 	 *	the argument buffer.
	 	 */
		svr_arg_buf[0] = (int) port;
	}

	/*
	 *	Switch to the client activation.
	 */
	(void) switch_act(THR_ACT_NULL);
        argv = MACH_RPC_ARGV(cln_act);

        if (cln_act->kernel_loaded) {
                outfunc = bbcopy;
        }
        else {
                outfunc = copyout;
        }

	/*
	 *	Copyout (scatter) result parameters to client space. 
 	 */
	if (argc > 1) {
        	kr = rpc_reply_args_out(routine, state, svr_arg_buf, 
				        cln_arg_buf, outfunc);
        	if (kr != KERN_SUCCESS) {
			rpc_error( ("rpc_reply_args_out %d", kr), ERR_YELLOW );
			ret = kr;
                	goto finish;
        	}

        	if ((*outfunc)((char *)cln_arg_buf, (char *)argv, nbyte)) {
			rpc_error( ("copyout fail"), ERR_GREEN );
        		unwind_rpc_state(routine, state, svr_arg_buf);
                	ret = KERN_FAILURE;
                	goto finish;
        	}
	}

finish:
	/*
	 *	Since the rpc argument processing is complete, remove
	 *	all kernel allocations.
	 */
        dsc = routine->arg_descr;
        for ( i = 1 ; i < routine->descr_count ; i++ ) {
            switch( state[i].copy_type ) {
                case COPY_TYPE_ALLOC_KRN:
            		type = RPC_FIELD(dsc[i].type, TYPE_SHIFT, 
                                                 LAST_TYPE_BIT);
			if (type == MACH_RPC_ARRAY_VARIABLE) {
				offset = dsc[i].offset/sizeof(int);
				size = svr_arg_buf[offset+1] * dsc[i].size;
			}
			else {
				size = DSCSIZE;
			}
                        KFREE(state[i].alloc_addr, size, IP_RT(port));
			state[i].copy_type = COPY_TYPE_NO_COPY;
			state[i].alloc_addr = 0;
                        break;
                default:
                        break;
            }
        }

        /*
         *      Release the rpc copy state record, if we allocated one.
         */
        if (state[0].copy_type == COPY_TYPE_ALLOC_KRN) {
                state_size = routine->descr_count *
                             sizeof(rpc_copy_state_data_t);
                KFREE((vm_offset_t)state, state_size, IP_RT(port));
        }

	cln_act->r_state = NULL;

        if (svr_arg_buf != svr_arg_buf_data) {
                KFREE((vm_offset_t)svr_arg_buf, nbyte, IP_RT(port));
        }


	/*
 	 *	Return to the client via an upcall.
	 */
	pcb_stack = (vm_offset_t) USER_REGS(cln_act);
	ueip = MACH_RPC_UIP(cln_act); 
	uesp = MACH_RPC_USP(cln_act);

	rpc_unlock(cur_thread);

	ETAP_DATA_LOAD(probe_data[0], 0);
	ETAP_DATA_LOAD(probe_data[1], routine_num);
	ETAP_DATA_LOAD(probe_data[2], cln_act->task);
	ETAP_DATA_LOAD(probe_data[3], svr_act->task);
	ETAP_PROBE_DATA(ETAP_P_RPC,
			EVENT_END,
			cur_thread,
			&probe_data,
			ETAP_DATA_SIZE);

	rpc_upcall(pcb_stack, uesp, ueip, ret);
	/* NOTREACHED */

	return kr;
}




/*
 *	name_rpc_to_ipc
 *
 * 	Convert from IPC disposition nomenclature to RPC
 *	disposition nomenclature.
 */
unsigned int 
name_rpc_to_ipc(unsigned int action)
{
	unsigned int disp = 0;

	switch(action) {
	case MACH_RPC_SEND:
	     disp = MACH_MSG_TYPE_PORT_SEND;
	     break;

	case MACH_RPC_RECEIVE:
	     disp = MACH_MSG_TYPE_PORT_RECEIVE;
	     break;

	case MACH_RPC_SEND_ONCE:
	     disp = MACH_MSG_TYPE_PORT_SEND_ONCE;
	     break;

	case MACH_RPC_MOVE_SEND:
	     disp = MACH_MSG_TYPE_MOVE_SEND;
	     break;

	case MACH_RPC_COPY_SEND:
	     disp = MACH_MSG_TYPE_COPY_SEND;
	     break;

	case MACH_RPC_MAKE_SEND:
	     disp = MACH_MSG_TYPE_MAKE_SEND;
	     break;

	case MACH_RPC_MOVE_RECEIVE:
	     disp = MACH_MSG_TYPE_MOVE_RECEIVE;
	     break;

	case MACH_RPC_MOVE_SEND_ONCE:
	     disp = MACH_MSG_TYPE_MOVE_SEND_ONCE;
	     break;

	case MACH_RPC_MAKE_SEND_ONCE:
	     disp = MACH_MSG_TYPE_MAKE_SEND_ONCE;
	     break;

	default:
		rpc_error( ("name_rpc_to_ipc: bad action 0x%x", action), 
			   ERR_FATAL );
	}

	return(disp);
}

/*
 *	clean_port_array
 *
 *	Homogenize a partially processed port array before calling unwind_
 *	rpc_state. 
 *
 *	When an error occurs during port array processing, part of the
 *	array will contain converted ports (ipc_object_t's) and part of
 *	the array will contain port names (mach_port_t's). Whether the
 *	first part contains converted ports and the second names or vice
 *	versa depends on whether the error occurs during port copyin's
 * 	or port copyout's. The purpose of this routine is to produce an
 *	array which contains only converted ports (which will be destroyed
 *	by unwind) or null ports (which will be ignored by unwind).
 */

void
clean_port_array(ipc_object_t *array, unsigned count, unsigned cooked, 
		 unsigned direct)
{
	int i;

	if (array == NULL || cooked > count || 
            !(direct == MACH_RPC_IN || direct == MACH_RPC_OUT)) {
		rpc_error( ("bad args to clean_port_array"), ERR_RED );
		return;
	}

	if (direct == MACH_RPC_IN) {
		for (i = cooked ; i < count ; i++) {
			array[i] = IO_NULL; 
		}
	}

	if (direct == MACH_RPC_OUT) {
		for (i = 0 ; i <= cooked ; i++) {
			array[i] = IO_NULL; 
		}
	}

	return;
}


/*
 *	unwind_rpc_state
 *	
 *	For error processing, this function releases any resources aquired 
 *	so far in the rpc path. Includes kernel kalloc's, vm_allocate's on
 *	the user's address space, and cleaning up any COW mappings.
 *
 *	This routine unwinds only that state acculumuated in the rpc_invke/
 *	reply_in/out functions.
 *
 *	The state vector should not contain any alloc records which apply to
 *	any map other than the current map. Committed allocations have had
 *	their records removed from the state vector.
 *
 *	With respect to error handling for the client reply side, the current 
 *	approach is that each client argument will be either completely 
 *	processed or not started if there is an error return. Error cleanup of
 *	client reply arguments is handled by the client or its MiG stub. This 
 *	is subject to revision.
 */

void
unwind_rpc_state(routine_descriptor_t routine, rpc_copy_state_t state, int *buf)
{
        int                             i, j;
	boolean_t			rt;
        thread_t                	cur_thread;
        ipc_object_t                    object, *port;
	unsigned int			type, action, offset;
        routine_arg_descriptor_t        dsc;
        vm_map_copy_t                   copy;
	vm_map_t			map;
	vm_size_t			size;


	/*
 	 *	Find the current map.
	 */
	map = current_map();	
        dsc = routine->arg_descr;

	rt = IP_RT((ipc_port_t)buf[0]);

	/*
	 *	Clean up all recorded state.
	 */
        for ( i = 1 ; i < routine->descr_count ; i++ ) {

            offset = dsc[i].offset/sizeof(int);
	    type = RPC_FIELD(dsc[i].type, TYPE_SHIFT, LAST_TYPE_BIT);

	    switch( state[i].copy_type ) {
		case COPY_TYPE_NO_COPY:
		case COPY_TYPE_ON_SSTACK:
		case COPY_TYPE_ON_KSTACK:
			break;

		case COPY_TYPE_VIRTUAL_IN:
			copy = (vm_map_copy_t)state[i].alloc_addr;
			vm_map_copy_discard(copy);
			break;

                case COPY_TYPE_PORT:
                        /*
                         * Destroy port rights carried in the message.
                         * If it is a dead_name or a NULL port, do nothing.
                         */
                        if (!IO_VALID((ipc_object_t) buf[offset])) {
                            continue;
                        }
			action = RPC_FIELD(dsc[i].type, NAME_SHIFT, 
					   LAST_NAME_BIT);
                        action = name_rpc_to_ipc(action);

                        ipc_object_destroy((ipc_object_t) buf[offset], action);

                        break;

		case COPY_TYPE_ALLOC_SVR:
		case COPY_TYPE_ALLOC_CLN:
		case COPY_TYPE_VIRTUAL_OUT_SVR:
		case COPY_TYPE_VIRTUAL_OUT_CLN:
			if (type == MACH_RPC_PORT_ARRAY) {
				/* 
				 * Currently, errors which occur after success-
				 * fully copying out a port array to the client
				 * space will not clean up the port array and
				 * its ports. This is left to the client or its
				 * MiG stubs. May be revisited.
				 */
				rpc_error( ("srv/cln port array not cleaned"), 
					    ERR_YELLOW );
				size = buf[offset+1] * sizeof( mach_port_t );
				break;
			}

			if (type == MACH_RPC_ARRAY_VARIABLE) {
				size = buf[offset+1] * dsc[i].size;
			}
			else {
				size = DSCSIZE;
			}

			(void) vm_deallocate(map, state[i].alloc_addr, size);
			break;

		case COPY_TYPE_ALLOC_KRN:
			if (type == MACH_RPC_PORT_ARRAY) {
                           /*
                            * Destroy ports and deallocate the array.
                            */
                           action = RPC_FIELD(dsc[i].type, NAME_SHIFT,
                                              LAST_NAME_BIT);
                           action = name_rpc_to_ipc(action);
                           port = (ipc_object_t *)state[i].alloc_addr;

                           /*
                            * If it is a dead_name or a NULL port, do nothing.
                            */
                           for ( j = 0 ; j < buf[offset+1] ; j++ ) {
                              if (!IO_VALID((ipc_object_t) port[j])) {
                                 continue;
                              }
                              ipc_object_destroy((ipc_object_t) port[j], 
						 action);
                           }
			}

			if ( type & MACH_RPC_VARIABLE ) {
				size = buf[offset+1] * dsc[i].size;
			}
			else {
				size = DSCSIZE;
			}

			KFREE(state[i].alloc_addr, size, rt);
			break;
	
		default:
			rpc_error( ("unwind_rpc_state: state.copy_type botch"), 
				   ERR_FATAL );
			break;
	    }
	}

	return;
}



/*
 *	rpc_invke_args_in
 *
 *	Process the signature, translating all ports in the argument buffer.
 *	This routine also copies in all IN or IN/OUT arrays (fixed, bounded 
 *	and variable). 
 *
 *	The arrays will be copied to either kernel stack buffer, kernel
 *	kalloc'd space, or a COW mapping will be set up.
 * 
 *	The MiG deallocate bit specifies deallocation of a pointer-type array
 *	from the client address space (for IN or INOUT) and/or server address 
 *	space (for OUT or INOUT).
 */
kern_return_t   
rpc_invke_args_in(routine_descriptor_t routine, rpc_copy_state_t state, 
		  int *buf, copyfunc_t infunc)
{
	int				i, j;
	boolean_t			rt;
	vm_offset_t			buf_used, src, dst;
        vm_map_copy_t                   copy;
	vm_map_t			map = current_map();
	vm_size_t			new_size, size;
	unsigned int 			type, action, offset, direct, bound;
	unsigned int			point, dealloc, copytype;
	ipc_object_t			object;
	kern_return_t			kr = KERN_SUCCESS;
	routine_arg_descriptor_t	dsc;
        ipc_object_t                    *object_dst;

	/*
	 *	Check inputs
	 */
	if (routine == NULL || buf == NULL)
		return KERN_INVALID_ARGUMENT;

	/*
         *      Initialize amount of server stack which has been used.
	 */
	dsc = routine->arg_descr;
	buf_used = routine->argc;
	rt = IP_RT((ipc_port_t)buf[0]);

	/*
	 *	Loop over each complex argument.
	 *
	 *	XXX Skip dest port (first arg) for now, since we already
	 *	    converted it in order to lookup the signature. Fixme.
	 */
	assert(dsc != NULL);
	assert(dsc[0].type == (MACH_RPC_PORT | MACH_RPC_COPY_SEND));

	for ( i = 1 ; i < routine->descr_count ; i++ ) {
		bound  = RPC_BOUND(dsc[i]);
		offset = dsc[i].offset/sizeof(int);
		type   = RPC_FIELD(dsc[i].type, TYPE_SHIFT, LAST_TYPE_BIT);
                direct = RPC_FIELD(dsc[i].type, DIRECT_SHIFT, LAST_DIRECT_BIT);

		switch (type | direct) {
		case MACH_RPC_PORT | MACH_RPC_OUT:
		     break;

		case MACH_RPC_PORT:
		case MACH_RPC_PORT | MACH_RPC_IN:
		case MACH_RPC_PORT | MACH_RPC_INOUT:
		     /* Convert the port. For inout args, the direct arg is a
			pointer to the port. */

		     if (direct == MACH_RPC_INOUT) {
			kr = (*infunc)((char *)buf[offset],
				(char *)&buf[offset], sizeof(int));
			if (kr) {
				rpc_error( ("copyin fail"), ERR_GREEN );
				unwind_rpc_state(routine, state, buf);
				return KERN_FAILURE;
			}
		     }

		     if (!PORT_VALID(buf[offset])) {
			break;
		     }

		     action = RPC_FIELD(dsc[i].type, NAME_SHIFT, 
				        LAST_ACTION_BIT);
		     action = name_rpc_to_ipc(action);

		     kr = ipc_object_copyin(current_space(), 
				(mach_msg_type_name_t) buf[offset], 
				action, &object);

		     if (kr != KERN_SUCCESS) {
				rpc_error( ("ipc_object_copyin %d", kr), 
					   ERR_YELLOW );
				unwind_rpc_state(routine, state, buf);
				return kr;
		     }

		     buf[offset] = (int) object;
		     break;

		case MACH_RPC_PORT_ARRAY | MACH_RPC_INOUT:
		     rpc_error( ("not supported"), ERR_INFO );
		     break;

		case MACH_RPC_ARRAY_FIXED    | MACH_RPC_OUT:
		case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_OUT:
		case MACH_RPC_PORT_ARRAY     | MACH_RPC_OUT:
		     break;

		case MACH_RPC_ARRAY_FIXED    | MACH_RPC_IN:
		case MACH_RPC_ARRAY_FIXED    | MACH_RPC_INOUT:
		case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_IN:
		case MACH_RPC_PORT_ARRAY     | MACH_RPC_IN:
		     /* Copyin the array data, to stack or kernel or virtually,
			depending on size. */

		     /* Find the array size. */
		     if (type & MACH_RPC_VARIABLE) {
			size = buf[offset+1] * dsc[i].size;
			if (bound == MACH_RPC_BOUND && size > DSCSIZE) {
				rpc_error( ("bnded array too big"), ERR_GREEN );
				unwind_rpc_state(routine, state, buf);
				return KERN_INVALID_ARGUMENT;
			}
		     }
		     else {
			size = DSCSIZE;
		     }

		     new_size = ROUNDWORD(size)/sizeof(int);
		     src = (vm_offset_t) buf[offset];
		     dst = (vm_offset_t) &buf[buf_used];

                     point    = RPC_FIELD(dsc[i].type, POINTER_SHIFT, 
				          LAST_POINTER_BIT);
	  	     dealloc  = RPC_FIELD(dsc[i].type, DEALLOCATE_SHIFT,
					  LAST_DEALLOCATE_BIT);
	  	     copytype = RPC_FIELD(dsc[i].type, OPTION_SHIFT,
					  LAST_OPTION_BIT);

		     /* Can we do a virtual copy? */
		     if ( (new_size >= RPC_COW_SIZE || 
  			   copytype & MACH_RPC_VIRTUAL_COPY) && 
			  !(copytype & MACH_RPC_PHYSICAL_COPY) &&
			  !(type & MACH_RPC_PORT) &&
			  direct == MACH_RPC_IN ) {
			kr = vm_map_copyin(map, src, size, dealloc, &copy);
			if (kr != KERN_SUCCESS) {
				unwind_rpc_state(routine, state, buf);
				return kr;
			}
			state[i].copy_type = COPY_TYPE_VIRTUAL_IN;
			state[i].alloc_addr = (vm_offset_t) copy;
			break;
		     }

		     /* Do we have to alloc space for the array? */
		     else {
			rpc_error( ("kalloc space for array"), ERR_INFO );
			dst = KALLOC(size, rt);
			if (dst == 0) {
				unwind_rpc_state(routine, state, buf);
				return KERN_RESOURCE_SHORTAGE;
			}
			state[i].copy_type = COPY_TYPE_ALLOC_KRN;
			state[i].alloc_addr = dst;
		     }

		     kr = (*infunc)((char *)src, (char *)dst, size);
		     if (kr) {
			rpc_error( ("copyin fail"), ERR_GREEN );
			unwind_rpc_state(routine, state, buf);
			return KERN_FAILURE;
		     }

		     /* Do we need to deallocate the array in the client's
			address space? */
		     if (dealloc && point) {
			kr = vm_deallocate(map, src, size);
                        if (kr != KERN_SUCCESS) {
                                rpc_error( ("vm_deallocate fail"), ERR_GREEN );
                                unwind_rpc_state(routine, state, buf);
                                return kr;
                        }
		     }

                     /* Handle port arrays by getting the port right disposi-
                        tion and translating each port. */
                     if (type & MACH_RPC_PORT) {
                        action = RPC_FIELD(dsc[i].type, NAME_SHIFT,
                                           LAST_ACTION_BIT);
                        action = name_rpc_to_ipc(action);
                        object_dst = (ipc_object_t *) dst;

                        for ( j = 0 ; j <  buf[offset+1] ; j++ ) {
                     	   if (!PORT_VALID(object_dst[j])) {
                        	continue;
                     	   }  
                           kr = ipc_object_copyin(current_space(),
                                   (mach_msg_type_name_t) object_dst[j],
                                   action, &object_dst[j]);

                           if (kr != KERN_SUCCESS) {
                              rpc_error( ("object_copyin fail %d", kr),
                                         ERR_YELLOW );
			      clean_port_array(object_dst, buf[offset+1], j, 
					       MACH_RPC_IN);
                              unwind_rpc_state(routine, state, buf);
                              return kr;
                           }
                        }
                     }
 
		     break;

		default:
		     rpc_error( ("no case index=%d type=0x%x", i, 
				type|direct), ERR_RED );
		     unwind_rpc_state(routine, state, buf);
		     return KERN_INVALID_ARGUMENT;
		}

	}

	return kr;
}


/*
 *	rpc_invke_args_out
 *
 *	Translate all ports in the argument buffer for the server task.
 * 	This routine also copies out IN and IN/OUT arrays from the kernel 
 *	to the server and adjusts parameters which reference them.
 *
 *	The server stack pointer (*new_sp) will be adjusted to account for
 *	stack space used to store srrays.
 *
 *	All rpc data will be copied to either server stack, server vm_
 *	allocate'd space, or a COW mapping will be set up in the server.
 *	There are four cases for memory treatment in the server's address
 *	space: (a) stack reservation, (b) persistent vcopy/vm_alloc, (c)
 *	persistent vcopy/vm_alloc with indirect pointers on stack, and (d) 
 *	indirect pointers on stack w/o alloc.
 *
 *	The MiG deallocate bit specifies deallocation of a pointer-type array
 *	from the client address space (for IN or INOUT) and/or server address 
 *	space (for OUT or INOUT).
 */
kern_return_t
rpc_invke_args_out(routine_descriptor_t routine, rpc_copy_state_t state, 
		   int *buf, int **new_sp, copyfunc_t outfunc)
{
        int                             i, j, *sp, zero = 0;
	boolean_t			rt;
        vm_size_t                       new_size, size;
	vm_offset_t 			src, dst, stk;
        unsigned int                    type, action, offset; 
	unsigned int			direct, point, bound;
        mach_port_t                    	port;
        kern_return_t                   kr = KERN_SUCCESS;
	vm_map_t 			map = current_map();
	vm_size_t			sstk_size;
	vm_offset_t			ustack_used;
        vm_map_copy_t                   copy;
        routine_arg_descriptor_t        dsc;
        ipc_object_t                    *object_src;


	/*
	 *	Check inputs. 
	 */
	if (*new_sp == NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	/*
         *      Initialize amount of server stack which has been used.
	 */
	sp = *new_sp;
        dsc = routine->arg_descr;
	ustack_used = routine->argc;
	sstk_size = (current_act()->user_stack_size)/sizeof(int);
	assert(sstk_size >= 0);

        rt = IP_RT((ipc_port_t)buf[0]);

	/*
	 *	Make sure there is enough room on the server stack for the
	 *	direct arguments.
	 */
	if (ustack_used > sstk_size) {
       		rpc_error( ("no server stack space"), ERR_GREEN );
		unwind_rpc_state(routine, state, buf);
		return KERN_RESOURCE_SHORTAGE;
	}

        /*
         *      Loop over each complex argument.
         */
        assert(dsc != NULL);
        assert(dsc[0].type == (MACH_RPC_PORT | MACH_RPC_COPY_SEND));

        for ( i = 1 ; i < routine->descr_count ; i++ ) {
                bound  = RPC_BOUND(dsc[i]);
                offset = dsc[i].offset/sizeof(int);
                type   = RPC_FIELD(dsc[i].type, TYPE_SHIFT, LAST_TYPE_BIT);
                point  = RPC_FIELD(dsc[i].type, POINTER_SHIFT,LAST_POINTER_BIT);
                direct = RPC_FIELD(dsc[i].type, DIRECT_SHIFT, LAST_DIRECT_BIT);

                switch (type | direct | point | bound) {
                case MACH_RPC_PORT:
                case MACH_RPC_PORT | MACH_RPC_IN:
                case MACH_RPC_PORT | MACH_RPC_INOUT:
                     /* Get the port name. For inout ports, the direct arg is a
			pointer to the port. */

                     if (PORT_VALID(buf[offset])) {
                     	action = RPC_FIELD(dsc[i].type, NAME_SHIFT, 
					   LAST_NAME_BIT);
                     	action = name_rpc_to_ipc(action);

                     	kr = ipc_object_copyout(current_space(),
					(ipc_object_t) buf[offset],
                                	action, FALSE, &port);

                     	if (kr != KERN_SUCCESS) {
				rpc_error( ("copyout fail"), ERR_GREEN );
				unwind_rpc_state(routine, state, buf);
                                return kr;
		     	}
                     }
		     else {
			port = (mach_port_t) buf[offset];
		     }

		     if (direct != MACH_RPC_INOUT) {
			buf[offset] = (int) port;
			break;
		     }

		     /* Fall through */

                case MACH_RPC_PORT | MACH_RPC_OUT:
		     /* Arrange space for the out port in server; we're
			assuming there is space on the server stack */

		     new_size = ROUNDWORD(DSCSIZE)/sizeof(int);
		     if ((ustack_used + new_size) > sstk_size) {
			rpc_error( ("no server stack space"), ERR_GREEN );
			unwind_rpc_state(routine, state, buf);
			return KERN_RESOURCE_SHORTAGE;
		     }

		     dst = (vm_offset_t) (sp - new_size);
		     sp -= new_size;
                     ustack_used += new_size;
                     buf[offset] = (int) dst;

		     /* Copyout in port to server stack */
		     if (direct == MACH_RPC_INOUT) {
			kr = (*outfunc)((char *)&port,
				(char *)dst, sizeof(port));
                        if (kr) {
                                rpc_error( ("copyout fail"), ERR_GREEN );
                                unwind_rpc_state(routine, state, buf);
                                return KERN_FAILURE;
                        }			
		     }

		     break;

                case MACH_RPC_PORT_ARRAY:
                case MACH_RPC_PORT_ARRAY | MACH_RPC_IN:
                case MACH_RPC_PORT_ARRAY | MACH_RPC_OUT:
                case MACH_RPC_PORT_ARRAY | MACH_RPC_INOUT:
		     rpc_error( ("no MACH_RPC_PORT_ARRAY"), ERR_INFO );
		     break;

                case MACH_RPC_ARRAY_FIXED | MACH_RPC_IN :
                case MACH_RPC_ARRAY_FIXED | MACH_RPC_OUT:
                case MACH_RPC_ARRAY_FIXED | MACH_RPC_INOUT:
                case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_BOUND | MACH_RPC_IN:
                case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_BOUND | MACH_RPC_OUT:
                case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_BOUND | MACH_RPC_INOUT:
                case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_UNBOUND | MACH_RPC_IN:
		case MACH_RPC_PORT_ARRAY  | MACH_RPC_BOUND | MACH_RPC_IN:
		case MACH_RPC_PORT_ARRAY  | MACH_RPC_BOUND | MACH_RPC_OUT:
		     /* Reserve space for array on server stack. If no room
			is available, then return an error. If we started a
			vcopy, we'll complete it and remove it on reply. */

                     if ((type & MACH_RPC_VARIABLE) && (direct & MACH_RPC_IN)) {
                        size = buf[offset+1] * dsc[i].size;
                     }
                     else {
                        size = DSCSIZE;
                     }

                    if (state[i].copy_type == COPY_TYPE_VIRTUAL_IN) {
			assert(direct == MACH_RPC_IN);
                        copy = (vm_map_copy_t) state[i].alloc_addr;
                        kr = vm_map_copyout(map, &dst, copy);
                        if (kr != KERN_SUCCESS) {
                                unwind_rpc_state(routine, state, buf);
                                return kr;
                        }

                        /* Update state record and parameter reference */
                        state[i].copy_type = COPY_TYPE_VIRTUAL_OUT_SVR;
                        state[i].alloc_addr = (vm_offset_t) dst;

                        buf[offset] = (int) dst;
                        break;
                     }
                     
                     assert( direct == MACH_RPC_OUT ? 
			     state[i].copy_type == COPY_TYPE_NO_COPY :
			     (state[i].copy_type == COPY_TYPE_ON_KSTACK ||
                              state[i].copy_type == COPY_TYPE_ALLOC_KRN) );

		     new_size = ROUNDWORD(size)/sizeof(int);
		     if ((ustack_used + new_size) <= sstk_size) {

                     	dst = (vm_offset_t) (sp - new_size);

			/* Reserve OUT space on server stack */
			if (direct == MACH_RPC_OUT) {
				sp -= new_size;
				ustack_used += new_size;
				buf[offset] = (int) sp;

				/* Setup OUT count on server stack */
				if (type & MACH_RPC_VARIABLE) {
				   if ((ustack_used + 1) > sstk_size) {
				      rpc_error( ("no server stack room "), 
						 ERR_GREEN );
				      unwind_rpc_state(routine, state, buf);
				      return KERN_RESOURCE_SHORTAGE;
				   }
				   sp -= 1;
				   ustack_used += 1;
				   buf[offset+1] = (int) sp;
				}

				break;
			}

                        /* Handle port arrays. Find port type and translate
                           each port in the array. */
                        if (type & MACH_RPC_PORT) {
                           action = RPC_FIELD(dsc[i].type, NAME_SHIFT,
                                              LAST_NAME_BIT);
                           action = name_rpc_to_ipc(action);
                           object_src = (ipc_object_t *)state[i].alloc_addr;

                           for ( j = 0 ; j < buf[offset+1] ; j++ ) {
			      if (!PORT_VALID(object_src[j])) {
					continue;
			      }
                              kr = ipc_object_copyout(current_space(),
                                        (ipc_object_t)object_src[j],
                                        action, FALSE,
                                        (mach_port_t *)&object_src[j]);

                              if (kr != KERN_SUCCESS) {
                                 rpc_error( ("ipc_object_copyout fail %d", kr),
                                            ERR_YELLOW );
				 clean_port_array(object_src, buf[offset+1], j,
						  MACH_RPC_OUT);
                                 unwind_rpc_state(routine, state, buf);
                                 return kr;
                              }
                           }
                        }

			/* Copyout IN array to server stack */
			src = state[i].alloc_addr;
			kr = (*outfunc)((char *)src, (char *)dst, size);
			if (kr) {
				rpc_error( ("copyout fail"), ERR_GREEN );
				unwind_rpc_state(routine, state, buf);
				return KERN_FAILURE;
			}

			/* Release kernel buffer, update state record, and
			   update the parameter reference */
			sp -= new_size;
			ustack_used += new_size;
			if (state[i].copy_type == COPY_TYPE_ALLOC_KRN) {
				KFREE(state[i].alloc_addr, size, rt);
			}

			state[i].copy_type = COPY_TYPE_NO_COPY;
			state[i].alloc_addr = 0;

			buf[offset] = (int) dst;
			break;
		     }
	
		     /* No room on server stack -- error */
		     rpc_error( ("no server stack room "), ERR_GREEN );
		     unwind_rpc_state(routine, state, buf);
		     return KERN_RESOURCE_SHORTAGE;


                case MACH_RPC_ARRAY_FIX | MACH_RPC_IN | MACH_RPC_POINTER:
		case MACH_RPC_ARRAY_VAR | MACH_RPC_IN | MACH_RPC_POINTER |BOUND:
		case MACH_RPC_ARRAY_VAR | MACH_RPC_IN | MACH_RPC_POINTER |UNBND:
		case MACH_RPC_PORT_ARRAY| MACH_RPC_IN | MACH_RPC_POINTER |UNBND:
		     /* Persistent virtual copy or vm_allocate in server's
			address space. */

		     if (type & MACH_RPC_VARIABLE) {
			size = buf[offset+1] * dsc[i].size;
		     }
		     else {
			size = DSCSIZE;
		     }

                     if (state[i].copy_type == COPY_TYPE_VIRTUAL_IN) {
                        copy = (vm_map_copy_t) state[i].alloc_addr;
                        kr = vm_map_copyout(map, &dst, copy);
                        if (kr != KERN_SUCCESS) {
                                unwind_rpc_state(routine, state, buf);
                                return kr;
                        }

                        /* Update state record and parameter reference */
                        state[i].copy_type = COPY_TYPE_VIRTUAL_OUT_SVR;
                        state[i].alloc_addr = (vm_offset_t) dst;

                        buf[offset] = (int) dst;
                        break;
                     }

		     /* Allocate persistent server memory */
		     src = state[i].alloc_addr;
                     kr = vm_allocate(map, &dst, size, TRUE);
                     if (kr != KERN_SUCCESS) {
			rpc_error( ("vm_allocate fail %d", kr), ERR_YELLOW );
                       	unwind_rpc_state(routine, state, buf);
                       	return kr;
                     }

                     /* Handle port arrays. Find port type and translate
                        each port in the array. */
                     if (type & MACH_RPC_PORT) {
                           action = RPC_FIELD(dsc[i].type, NAME_SHIFT,
                                              LAST_NAME_BIT);
                           action = name_rpc_to_ipc(action);
                           object_src = (ipc_object_t *)state[i].alloc_addr;

                           for ( j = 0 ; j < buf[offset+1] ; j++ ) {
			      if (!PORT_VALID(object_src[j])) {
					continue;
			      }
                              kr = ipc_object_copyout(current_space(),
                                        (ipc_object_t)object_src[j],
                                        action, FALSE,
                                        (mach_port_t *)&object_src[j]);

                              if (kr != KERN_SUCCESS) {
                                 rpc_error( ("ipc_object_copyout fail %d", kr),
                                            ERR_YELLOW );
				 clean_port_array(object_src, buf[offset+1], j,
						  MACH_RPC_OUT);
                                 unwind_rpc_state(routine, state, buf);
                                 return kr;
                              }
                           }
                     }

                     kr = (*outfunc)((char *)src, (char *)dst, size);
                     if (kr) {
			/* State doesn't know about vm_alloc'd dst */
                       	(void) vm_deallocate(map, dst, size);
                       	unwind_rpc_state(routine, state, buf);
                       	return KERN_FAILURE;
                     }

                     /* Since copyout was successful, free kernel buffer
                        and convert state from _ALLOC_KRN to _ALLOC_SVR */
                     if (state[i].copy_type == COPY_TYPE_ALLOC_KRN) {
                     	KFREE(state[i].alloc_addr, size, rt);
		     }
                     state[i].copy_type = COPY_TYPE_ALLOC_SVR;
                     state[i].alloc_addr = dst;

                     /* Update parameter reference */
                     buf[offset] = (int) dst;
                     break;

		case MACH_RPC_PORT_ARRAY| MACH_RPC_OUT | MACH_RPC_POINTER:
                case MACH_RPC_ARRAY_FIX | MACH_RPC_OUT | MACH_RPC_POINTER:
		case MACH_RPC_ARRAY_VAR | MACH_RPC_OUT | MACH_RPC_POINTER|BOUND:
		case MACH_RPC_ARRAY_VAR | MACH_RPC_OUT | MACH_RPC_POINTER|UNBND:
		case MACH_RPC_ARRAY_VAR | MACH_RPC_OUT | UNBND:
		     /* Setup indirect pointer on server's stack which server
			will fill in with pointer to result (out) data. */

		     assert(state[i].copy_type == COPY_TYPE_NO_COPY);

		     /* Do we need space for the out count? */
		     if (type & MACH_RPC_VARIABLE) {
			new_size = 2;
		     }
		     else {
			new_size = 1;
		     }

		     /* Setup space for the out array pointer */
		     if ((ustack_used + new_size) <= sstk_size) {
                     	dst = (vm_offset_t) (sp - 1);

                        kr = (*outfunc)((char *)&zero,
				(char *)dst, sizeof(zero));
                        if (kr) {
				rpc_error( ("copyout fail"), ERR_YELLOW );
                                unwind_rpc_state(routine, state, buf);
                                return KERN_FAILURE;
                        }
			
			sp -= 1;
			ustack_used += 1;
			buf[offset] = (int) dst;

			if (!(type & MACH_RPC_VARIABLE)) {
				break;
			}

		     	/* Setup space for the out count pointer */
			dst = (vm_offset_t) (sp - 1);

                        kr = (*outfunc)((char *)&zero, (char *)dst, sizeof(zero));
                        if (kr) {
                                rpc_error( ("copyout fail"), ERR_YELLOW );
                                unwind_rpc_state(routine, state, buf);
                                return KERN_FAILURE;
                        }

                        sp -= 1;
                        ustack_used += 1;
                        buf[offset+1] = (int) dst;

			break;
		     }

		     /* No room on server stack -- return error */
                     rpc_error( ("no server stack room "), ERR_GREEN );
                     unwind_rpc_state(routine, state, buf);
                     return KERN_RESOURCE_SHORTAGE;

		     break;

                case MACH_RPC_ARRAY_FIXED | MACH_RPC_INOUT | MACH_RPC_POINTER:
		     /* Persistent virtual copy or vm_allocate in server's
			address space with indirect pointer on server's stack
			referencing the allocated data. */

		     /* Ensure there is space on server stack for pointer */
                     if ((ustack_used + 1) > sstk_size) {
			unwind_rpc_state(routine, state, buf);
			return KERN_RESOURCE_SHORTAGE;
		     }

		     size = DSCSIZE;

                     if (state[i].copy_type == COPY_TYPE_VIRTUAL_IN) {
			/* Setup virtual copy */
                        copy = (vm_map_copy_t) state[i].alloc_addr;
                        kr = vm_map_copyout(map, &dst, copy);
                        if (kr != KERN_SUCCESS) {
                                unwind_rpc_state(routine, state, buf);
                                return kr;
                        }

                        /* Update state record and parameter reference */
                        state[i].copy_type = COPY_TYPE_VIRTUAL_OUT_SVR;
                        state[i].alloc_addr = (vm_offset_t) dst;
                     }
		     else {
		     	/* Allocate persistent server memory */
			assert(state[i].copy_type == COPY_TYPE_ON_KSTACK ||
			       state[i].copy_type == COPY_TYPE_ALLOC_KRN);

		     	src = state[i].alloc_addr;
                     	kr = vm_allocate(map, &dst, size, TRUE);
                     	if (kr != KERN_SUCCESS) {
				rpc_error( ("vm_allocate fail %d", kr), 
					   ERR_YELLOW );
                       		unwind_rpc_state(routine, state, buf);
                       		return kr;
                     	}

                     	kr = (*outfunc)((char *)src, (char *)dst, size);
                     	if (kr) {
				/* State doesn't know about vm_alloc'd dst */
                       		(void) vm_deallocate(map, dst, size);
                       		unwind_rpc_state(routine, state, buf);
                       		return KERN_FAILURE;
                     	}

                     	/* Since copyout was successful, free kernel buffer
                           and convert state from _ALLOC_KRN to _ALLOC_SVR */
                     	if (state[i].copy_type == COPY_TYPE_ALLOC_KRN) {
                     		KFREE(state[i].alloc_addr, size, rt);
			}
                     	state[i].copy_type = COPY_TYPE_ALLOC_SVR;
                     	state[i].alloc_addr = dst;
		     }

                     stk = (vm_offset_t) (sp - 1);
                     kr = (*outfunc)((char *)&dst, (char *)stk, sizeof(dst));
                     if (kr) {
			rpc_error( ("copyout fail"), ERR_GREEN );
                        unwind_rpc_state(routine, state, buf);
                        return KERN_FAILURE;
                     }

		     sp -= 1;
                     ustack_used += 1;
                     buf[offset] = (int) stk;

                     break;

                default:
		     rpc_error( ("invke out: no case index=%d type=0x%x", i, 
			        type | direct | point | bound), ERR_RED );
                }

        }

	assert( sp == (*new_sp - ustack_used + routine->argc) );
	*new_sp = sp;
	return kr;
}




/*
 *      rpc_reply_args_in
 *
 *      Process the signature, translating all ports in the argument buffer.
 *      This routine also copies in all OUT or IN/OUT arrays (fixed, bounded
 *      and variable).
 *
 *      The arrays will be copied to either kernel stack buffer, kernel
 *      kalloc'd space, or a COW mapping will be set up.
 *
 *      The MiG deallocate bit specifies deallocation of a pointer-type array
 *      from the client address space (for IN or INOUT) and/or server address
 *      space (for OUT or INOUT).
 */
kern_return_t
rpc_reply_args_in(routine_descriptor_t routine, rpc_copy_state_t state,
                  int *svr_buf, copyfunc_t infunc)
{
        int                             i, j;
        boolean_t                      rt;
        vm_offset_t                     buf_used, src, dst;
        vm_map_copy_t                   copy;
        vm_map_t                        map = current_map();
        vm_size_t                       new_size, size, count;
        unsigned int                    type, action, offset, direct, dealloc; 
	unsigned int			point, bound, num_of_ports, copytype;
        ipc_object_t                    object, *object_dst;
        kern_return_t                   kr = KERN_SUCCESS;
        routine_arg_descriptor_t        dsc;

        /*
         *      Initialize amount of server stack which has been used.
         */
        dsc = routine->arg_descr;
        buf_used = routine->argc;
	rt = IP_RT((ipc_port_t)svr_buf[0]);

        /*
         *      Loop over each complex argument.
         *
         *      XXX Skip dest port (first arg) for now, since it does not
         *          have to be returned to the client.
         */
        assert(dsc != NULL);
        assert(dsc[0].type == (MACH_RPC_PORT | MACH_RPC_COPY_SEND));

        for ( i = 1 ; i < routine->descr_count ; i++ ) {
                bound  = RPC_BOUND(dsc[i]);
                offset = dsc[i].offset/sizeof(int);
                type   = RPC_FIELD(dsc[i].type, TYPE_SHIFT, LAST_TYPE_BIT);
                direct = RPC_FIELD(dsc[i].type, DIRECT_SHIFT, LAST_DIRECT_BIT);
                point  = RPC_FIELD(dsc[i].type,POINTER_SHIFT, LAST_POINTER_BIT);
		dealloc = RPC_FIELD(dsc[i].type, DEALLOCATE_SHIFT, 
				    LAST_DEALLOCATE_BIT);
		copytype = RPC_FIELD(dsc[i].type, OPTION_SHIFT,
                		     LAST_OPTION_BIT);

                switch (type | direct | bound | point) {
                case MACH_RPC_PORT | MACH_RPC_OUT:
                case MACH_RPC_PORT | MACH_RPC_INOUT:
		     /* Translate ports being returned to the client. The port
			will be stored in the server argument buffer. */

                     src = (vm_offset_t) svr_buf[offset];
                     dst = (vm_offset_t) &svr_buf[offset];

		     size = DSCSIZE;
                     assert(state[i].copy_type == COPY_TYPE_NO_COPY);

                     kr = (*infunc)((char *)src, (char *)dst, size);
                     if (kr) {
                        rpc_error( ("copyin fail"), ERR_GREEN );
                        unwind_rpc_state(routine, state, svr_buf);
                        return KERN_FAILURE;
                     }

		     if (!PORT_VALID(svr_buf[offset])) {
			break;
		     }

		     /* Translate the port */
                     action = RPC_FIELD(dsc[i].type, NAME_SHIFT, 
			                LAST_ACTION_BIT);
                     action = name_rpc_to_ipc(action);

                     kr = ipc_object_copyin(current_space(),
                                (mach_msg_type_name_t) svr_buf[offset],
                                action, &object);
                     if (kr != KERN_SUCCESS) {
				rpc_error( ("object_copyin fail %d", kr), 
					   ERR_YELLOW );
				unwind_rpc_state(routine, state, svr_buf);
                                return kr;
                     }

                     svr_buf[offset] = (int) object;
                     break;

                case MACH_RPC_ARRAY_FIXED    | MACH_RPC_IN:
              	case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_IN | MACH_RPC_BOUND:
              	case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_IN | MACH_RPC_UNBOUND:
		     /* If the vcopy optimization was used to provide the 
			server in array, remove the mapping now. */

		     if (state[i].copy_type == COPY_TYPE_VIRTUAL_OUT_SVR) {
			if (type == MACH_RPC_ARRAY_VARIABLE) {
				size = svr_buf[offset+1] * dsc[i].size;
			}
			else {
				size = DSCSIZE;
			}
			(void) vm_deallocate(map, state[i].alloc_addr, size);
			state[i].copy_type = COPY_TYPE_NO_COPY;
			state[i].alloc_addr = 0;
		     }

		     break;

                case MACH_RPC_ARRAY_FIXED    | MACH_RPC_OUT:
                case MACH_RPC_ARRAY_FIXED    | MACH_RPC_INOUT:
                case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_OUT   | MACH_RPC_BOUND:
		case MACH_RPC_PORT_ARRAY     | MACH_RPC_OUT   | MACH_RPC_BOUND:
		     /* The result data is on the server stack; copy it in so
			we can return it to the client. */
	
		     if (type & MACH_RPC_VARIABLE) {
			kr = (*infunc)((char *)svr_buf[offset+1], 
				       (char *)&count, sizeof(count));
			if (kr) {
				rpc_error( ("copyin fail"), ERR_GREEN );
                                unwind_rpc_state(routine, state, svr_buf);
                                return KERN_FAILURE;
                     	}

			size = count * dsc[i].size;

			if (size > DSCSIZE || size < 0) {
				rpc_error( ("bnded va too big"), ERR_GREEN );
				size = DSCSIZE;
			}

			svr_buf[offset+1] = (int) count;
		     }
		     else {
			size = DSCSIZE;
		     }
	
  		     new_size = ROUNDWORD(size)/sizeof(int);
                     src = (vm_offset_t) svr_buf[offset];
                     dst = (vm_offset_t) &svr_buf[buf_used];

		     assert(state[i].copy_type == COPY_TYPE_NO_COPY);
#if 0
                     /* Can we do a virtual copy? */
                    if ( (new_size >= RPC_COW_SIZE ||
			  copytype & MACH_RPC_VIRTUAL_COPY) &&
			  !(copytype & MACH_RPC_PHYSICAL_COPY) ) {
                        kr = vm_map_copyin(map, src, size, FALSE, &copy);
                        if (kr != KERN_SUCCESS) {
                                unwind_rpc_state(routine, state, svr_buf);
                                return kr;
                        }
                        state[i].copy_type = COPY_TYPE_VIRTUAL_IN;
                        state[i].alloc_addr = (vm_offset_t) copy;
                        break;
                     }
#endif
		     rpc_error( ("kalloc space for array"), ERR_INFO );
                     dst = KALLOC(size, rt);
                     if (dst == 0) {
                             unwind_rpc_state(routine, state, svr_buf);
                             return KERN_RESOURCE_SHORTAGE;
                     }
                     state[i].copy_type = COPY_TYPE_ALLOC_KRN;
                     state[i].alloc_addr = dst;

                     kr = (*infunc)((char *)src, (char *)dst, size);
                     if (kr) {
			rpc_error( ("copyin fail"), ERR_GREEN );
                        unwind_rpc_state(routine, state, svr_buf);
                        return KERN_FAILURE;
                     }

                    /* Handle port arrays by getting the port right disposi-
                       tion and translating each port. */
                     if (type & MACH_RPC_PORT) {
                        action = RPC_FIELD(dsc[i].type, NAME_SHIFT,
                                           LAST_ACTION_BIT);
                        action = name_rpc_to_ipc(action);
                        object_dst = (ipc_object_t *) dst;

                        for ( j = 0 ; j < count ; j++ ) {
			   if (!PORT_VALID(object_dst[j])) {
				continue;
			   }
                           kr = ipc_object_copyin(current_space(),
                                   (mach_msg_type_name_t) object_dst[j],
                                   action, &object_dst[j]);

                           if (kr != KERN_SUCCESS) {
                              rpc_error( ("object_copyin fail %d", kr),
                                         ERR_YELLOW );
			      clean_port_array(object_dst, svr_buf[offset+1], j,
					       MACH_RPC_IN);
                              unwind_rpc_state(routine, state, svr_buf);
                              return kr;
                           }
                        }
                     }

		     break;

                case MACH_RPC_ARRAY_FIX | MACH_RPC_OUT | MACH_RPC_POINTER:
                case MACH_RPC_ARRAY_VAR | MACH_RPC_OUT | MACH_RPC_POINTER|BOUND:
                case MACH_RPC_ARRAY_VAR | MACH_RPC_OUT | MACH_RPC_POINTER|UNBND:
                case MACH_RPC_ARRAY_VAR | MACH_RPC_OUT | UNBND:
		case MACH_RPC_PORT_ARRAY| MACH_RPC_OUT | MACH_RPC_POINTER:
		     /* Server has filled in an indirect pointer on its stack
			which points to the result data. Either vcopy or phys
			copy the data back to client. But first copy in the 
			pointer. */

                     assert(state[i].copy_type == COPY_TYPE_NO_COPY);

		     kr = (*infunc)((char *)svr_buf[offset], (char *)&src, 
				 sizeof(src));
		     if (kr) {
				rpc_error( ("copyin fail"), ERR_GREEN );
                                unwind_rpc_state(routine, state, svr_buf);
                                return KERN_FAILURE;
		     }

		     /* Find the array element count and array size */
                     if (type & MACH_RPC_VARIABLE) {
			kr = (*infunc)((char *)svr_buf[offset+1],
				(char *)&count, sizeof(count));
			if (kr) {
				rpc_error( ("copyin fail"), ERR_GREEN );
				unwind_rpc_state(routine, state, svr_buf);
				return KERN_FAILURE;
			}

			size = count * dsc[i].size;

			if ((int)size < 0) {
                                rpc_error( ("bad size"), ERR_GREEN );
                                unwind_rpc_state(routine, state, svr_buf);
                                return KERN_INVALID_ARGUMENT;
			}

			if (bound == MACH_RPC_BOUND && size > DSCSIZE) {
				rpc_error( ("bnded array too big"), ERR_GREEN );
				unwind_rpc_state(routine, state, svr_buf);
				return KERN_INVALID_ARGUMENT;
			}

			/* Save count for reply out */
			svr_buf[offset+1] = (int)count;
                     }
                     else {
                        size = DSCSIZE;
			count = dsc[i].count;
                     }

                     new_size = ROUNDWORD(size)/sizeof(int);
                     dst = (vm_offset_t) &svr_buf[buf_used];

		     /* Can we do a virtual copy? */
                    if ( (new_size >= RPC_COW_SIZE || 
			  copytype & MACH_RPC_VIRTUAL_COPY) &&
			 !(copytype & MACH_RPC_PHYSICAL_COPY) &&
			 !(type & MACH_RPC_PORT) ) {
                        kr = vm_map_copyin(map, src, size, dealloc, &copy);
                        if (kr != KERN_SUCCESS) {
                                unwind_rpc_state(routine, state, svr_buf);
                                return kr;
                        }
                        state[i].copy_type = COPY_TYPE_VIRTUAL_IN;
                        state[i].alloc_addr = (vm_offset_t) copy;
                        break;
                     }

		     /* Copy result to kalloc space */
		     rpc_error( ("kalloc space for array"), ERR_INFO );
                     dst = KALLOC(size, rt);
                     if (dst == 0) {
                             unwind_rpc_state(routine, state, svr_buf);
                             return KERN_RESOURCE_SHORTAGE;
                     }
                     state[i].copy_type = COPY_TYPE_ALLOC_KRN;
                     state[i].alloc_addr = dst;

                     kr = (*infunc)((char *)src, (char *)dst, size);
                     if (kr) {
			rpc_error( ("copyin fail"), ERR_GREEN );
                        unwind_rpc_state(routine, state, svr_buf);
                        return KERN_FAILURE;
                     }

		     /* If the dealloc bit it set, we deallocate the array
			memory in the server's address space now. */
		     if (dealloc) {
			kr = vm_deallocate(map, src, size);
			if (kr != KERN_SUCCESS) {
				rpc_error( ("vm_deallocate fail"), ERR_GREEN );
				unwind_rpc_state(routine, state, svr_buf);
				return kr;
			}
		     }

		     /* Handle port arrays by getting the port right disposi-
			tion and translating each port. */
		     if (type & MACH_RPC_PORT) {
                     	action = RPC_FIELD(dsc[i].type, NAME_SHIFT,
                                           LAST_ACTION_BIT);
                     	action = name_rpc_to_ipc(action);
			object_dst = (ipc_object_t *) dst;

                     	for ( j = 0 ; j < count ; j++ ) {
			   if (!PORT_VALID(object_dst[j])) {
				continue;
			   }
                           kr = ipc_object_copyin(current_space(),
				   (mach_msg_type_name_t) object_dst[j],
                                   action, &object_dst[j]);

                           if (kr != KERN_SUCCESS) {
                              rpc_error( ("object_copyin fail %d", kr),
                                         ERR_YELLOW );
			      clean_port_array(object_dst, count, j,
					       MACH_RPC_IN);
                              unwind_rpc_state(routine, state, svr_buf);
                              return kr;
                           }
                      	}
		     }
	
		     break;

                case MACH_RPC_ARRAY_FIXED | MACH_RPC_INOUT | MACH_RPC_POINTER:
		     /* Indirect pointer on server's stack points to the 
			result data; copy it in. Can't use our record of data
			location since server may have (intentionally) over-
			written it with another pointer. Either vcopy or phys
			copy the data in. */

                     assert(state[i].copy_type == COPY_TYPE_ALLOC_SVR ||
			    state[i].copy_type == COPY_TYPE_VIRTUAL_OUT_SVR);

                     kr = (*infunc)((char *)svr_buf[offset], (char *)&src,
                                 sizeof(src));
                     if (kr) {
                        	rpc_error( ("copyin fail"), ERR_GREEN );
                                unwind_rpc_state(routine, state, svr_buf);
                                return KERN_FAILURE;
                     }

                     /* No result provided */
                     if (src == 0)
                        break;

                     new_size = ROUNDWORD(DSCSIZE)/sizeof(int);
                     dst = (vm_offset_t) &svr_buf[buf_used];

                     /* Can we do a virtual copy? */
                    if ( (new_size >= RPC_COW_SIZE ||
			  copytype & MACH_RPC_VIRTUAL_COPY) &&
			 !(copytype & MACH_RPC_PHYSICAL_COPY) ) {
                        kr = vm_map_copyin(map,src,DSCSIZE,dealloc,&copy);
                        if (kr != KERN_SUCCESS) {
                                unwind_rpc_state(routine, state, svr_buf);
                                return kr;
                        }
                        state[i].copy_type = COPY_TYPE_VIRTUAL_IN;
                        state[i].alloc_addr = (vm_offset_t) copy;
                        break;
                     }

                     /* Copy result to kalloc space */
                     rpc_error( ("array too big for kstack"), ERR_GREEN );
                     dst = KALLOC(DSCSIZE, rt);
                     if (dst == 0) {
                             unwind_rpc_state(routine, state, svr_buf);
                             return KERN_RESOURCE_SHORTAGE;
                     }
                     state[i].copy_type = COPY_TYPE_ALLOC_KRN;
                     state[i].alloc_addr = dst;

                     kr = (*infunc)((char *)src, (char *)dst, DSCSIZE);
                     if (kr) {
			rpc_error( ("copyin fail"), ERR_GREEN );
                        unwind_rpc_state(routine, state, svr_buf);
                        return KERN_FAILURE;
                     }

                     /* If the dealloc bit it set, we deallocate the array
                        memory in the server's address space now. */
                     if (dealloc) {
                        kr = vm_deallocate(map, src, DSCSIZE);
                        if (kr != KERN_SUCCESS) {
                                rpc_error( ("vm_deallocate fail"), ERR_GREEN );
                                unwind_rpc_state(routine, state, svr_buf);
                                return kr;
                        }
                     }

		     break;

		default:
		     if (direct & MACH_RPC_OUT) {
		     	rpc_error( ("reply in: no case index=%d type=0x%x", i, 
			  	   type | direct | bound | point), ERR_RED );
		     }
		}
	}

	return kr;
}




/*
 *      rpc_reply_args_out
 *
 *      Translate all ports in the argument buffer for the client task.
 *      This routine also copies out OUT and IN/OUT arrays from the kernel
 *      to the client and adjusts parameters which reference them.
 *
 *      All rpc data will be copied to either client stack, client vm_
 *      allocate'd space, or a COW mapping will be set up in the client.
 *
 *      The MiG deallocate bit specifies deallocation of a pointer-type array
 *      from the client address space (for IN or INOUT) and/or server address
 *      space (for OUT or INOUT).
 */
kern_return_t
rpc_reply_args_out(routine_descriptor_t routine, rpc_copy_state_t state,
                   int *svr_buf, int *cln_buf, copyfunc_t outfunc)
{
        int                             i, j, *sp, zero = 0;
        boolean_t                      rt;
        vm_size_t                       new_size, size, count;
        vm_offset_t                     src, dst, stk;
        unsigned int                    type, action, offset;
        unsigned int                    direct, point, bound;
        mach_port_t                     port;
        kern_return_t                   kr = KERN_SUCCESS;
        vm_map_t                        map = current_map();
        vm_map_copy_t                   copy;
        vm_offset_t                     ustack_used;
	ipc_object_t			*object_src;
        routine_arg_descriptor_t        dsc;


	/*
	 *	Initialize
	 */
        dsc = routine->arg_descr;
	rt = IP_RT((ipc_port_t)svr_buf[0]);

        /*
         *      Loop over each complex argument.
         */
        assert(dsc != NULL);
        assert(dsc[0].type == (MACH_RPC_PORT | MACH_RPC_COPY_SEND));
        for ( i = 1 ; i < routine->descr_count ; i++ ) {
		bound  = RPC_BOUND(dsc[i]);
                offset = dsc[i].offset/sizeof(int);
                type   = RPC_FIELD(dsc[i].type, TYPE_SHIFT, LAST_TYPE_BIT);
                point  = RPC_FIELD(dsc[i].type,POINTER_SHIFT, LAST_POINTER_BIT);
                direct = RPC_FIELD(dsc[i].type, DIRECT_SHIFT, LAST_DIRECT_BIT);
		
                switch (type | direct | point | bound) {
                case MACH_RPC_PORT | MACH_RPC_OUT:
                case MACH_RPC_PORT | MACH_RPC_INOUT:
                     /* Translate ports being returned to the client. The port
                        is stored in the server argument buffer. */

                     /* Find the port type */
		     if (PORT_VALID(svr_buf[offset])) {
                     	action = RPC_FIELD(dsc[i].type, NAME_SHIFT, 
					   LAST_NAME_BIT);
                     	action = name_rpc_to_ipc(action);

                     	kr = ipc_object_copyout(current_space(),
                                	(ipc_object_t) svr_buf[offset],
                                	action, FALSE, &port);
                     	if (kr != KERN_SUCCESS) {
                                rpc_error( ("ipc_object_copyout fail %d", kr), 
					   ERR_YELLOW );
                                unwind_rpc_state(routine, state, svr_buf);
                                return kr;
                     	}
		     }
		     else {
			port = (mach_port_t) svr_buf[offset];
		     }

		     /* Write the result to the client's address space */
		     dst = (vm_offset_t) cln_buf[offset];
                     kr = (*outfunc)((char *)&port, (char *)dst, sizeof(port));
                     if (kr) {
                        rpc_error( ("copyout fail"), ERR_YELLOW );
                        unwind_rpc_state(routine, state, svr_buf);
                        return KERN_FAILURE;
                      }

                     break;

                case MACH_RPC_ARRAY_FIXED    | MACH_RPC_OUT:
		case MACH_RPC_ARRAY_FIXED    | MACH_RPC_INOUT:
		case MACH_RPC_ARRAY_VARIABLE | MACH_RPC_OUT   | MACH_RPC_BOUND:
		case MACH_RPC_PORT_ARRAY     | MACH_RPC_OUT   | MACH_RPC_BOUND:
		     /* Copyout result data to client space, only phys */

		     assert(state[i].copy_type == COPY_TYPE_ON_KSTACK ||
			    state[i].copy_type == COPY_TYPE_ALLOC_KRN);

                     /* Store count of result data in client's space */
                     if (type & MACH_RPC_VARIABLE) {
			size = svr_buf[offset+1] * dsc[i].size;
                        kr = (*outfunc)((char *)&svr_buf[offset+1], 
				     (char *) cln_buf[offset+1],
                                     sizeof(vm_size_t));
                        if (kr) {
                                rpc_error( ("copyout fail"), ERR_YELLOW );
                                unwind_rpc_state(routine, state, svr_buf);
                                return KERN_FAILURE;
                        }
                     }
                     else {
                        size = DSCSIZE;
                     }

                     /* Handle port arrays. Find port type and translate
                        each port in the array. */
                     if (type & MACH_RPC_PORT) {
                     	action = RPC_FIELD(dsc[i].type, NAME_SHIFT,
                                              LAST_NAME_BIT);
                       	action = name_rpc_to_ipc(action);
                       	object_src = (ipc_object_t *)state[i].alloc_addr;

                       	for ( j = 0 ; j < svr_buf[offset+1] ; j++ ) {
			   if (!PORT_VALID(object_src[j])) {
				continue;
			   }
                       	   kr = ipc_object_copyout(current_space(),
                                        (ipc_object_t)object_src[j],
                                        action, FALSE,
                                        (mach_port_t *)&object_src[j]);

			   if (kr != KERN_SUCCESS) {
                                 rpc_error( ("ipc_object_copyout fail %d", kr),
                                            ERR_YELLOW );
				 clean_port_array(object_src, svr_buf[offset+1],
						  j, MACH_RPC_OUT);
                                 unwind_rpc_state(routine, state, svr_buf);
                                 return kr;
                           }
			}
                     }

		     src = state[i].alloc_addr;
		     dst = cln_buf[offset];
		
                     kr = (*outfunc)((char *)src, (char *)dst, size);
                     if (kr) {
			rpc_error( ("copyout fail"), ERR_YELLOW );
                       	unwind_rpc_state(routine, state, svr_buf);
                       	return KERN_FAILURE;
                     }

		     /* Release kernel allocated buffer */
		     if (state[i].copy_type == COPY_TYPE_ALLOC_KRN) {
			KFREE(state[i].alloc_addr, size, rt);
		     }

		     state[i].copy_type = COPY_TYPE_NO_COPY;
		     state[i].alloc_addr = 0;

		     break; 

		case MACH_RPC_ARRAY_FIX | MACH_RPC_OUT  |MACH_RPC_POINTER:
                case MACH_RPC_ARRAY_FIX | MACH_RPC_INOUT|MACH_RPC_POINTER:
                case MACH_RPC_ARRAY_VAR | MACH_RPC_OUT  |MACH_RPC_POINTER|BOUND:
                case MACH_RPC_ARRAY_VAR | MACH_RPC_OUT  |MACH_RPC_POINTER|UNBND:
                case MACH_RPC_ARRAY_VAR | MACH_RPC_OUT  |UNBND:
		case MACH_RPC_PORT_ARRAY| MACH_RPC_OUT | MACH_RPC_POINTER:
		     /* Vcopy or alloc and phys copy result to client's address
			space. Then overwrite client's indirect pointer to the
			data area. */
		
                     if (state[i].copy_type == COPY_TYPE_VIRTUAL_IN) {
			assert(!(type & MACH_RPC_PORT));
                        copy = (vm_map_copy_t) state[i].alloc_addr;
                        kr = vm_map_copyout(map, &dst, copy);
                        if (kr != KERN_SUCCESS) {
                                unwind_rpc_state(routine, state, svr_buf);
                                return kr;
                        }

                        /* Update state record and parameter reference */
                        state[i].copy_type = COPY_TYPE_VIRTUAL_OUT_CLN;
                        state[i].alloc_addr = (vm_offset_t) dst;
                     }
		     else {
                        /* Allocate persistent client memory */
                        assert(state[i].copy_type == COPY_TYPE_ON_KSTACK ||
                               state[i].copy_type == COPY_TYPE_ALLOC_KRN);

                     	if (type & MACH_RPC_VARIABLE) {
                        	size = svr_buf[offset+1] * dsc[i].size;
                     	}
                     	else {
                        	size = DSCSIZE;
                     	}

                        src = state[i].alloc_addr;
                        kr = vm_allocate(map, &dst, size, TRUE);
                        if (kr != KERN_SUCCESS) {
                                rpc_error( ("vm_allocate fail %d", kr),
                                           ERR_YELLOW );
                                unwind_rpc_state(routine, state, svr_buf);
                                return kr;
                        }

			/* Handle port arrays. Find port type and translate
			   each port in the array. */
			if (type & MACH_RPC_PORT) {
                           action = RPC_FIELD(dsc[i].type, NAME_SHIFT, 
					      LAST_NAME_BIT);
                           action = name_rpc_to_ipc(action);
			   object_src = (ipc_object_t *)state[i].alloc_addr;

                           for ( j = 0 ; j < svr_buf[offset+1] ; j++ ) {
			      if (!PORT_VALID(object_src[j])) {
					continue;
			      }
                              kr = ipc_object_copyout(current_space(),
					(ipc_object_t)object_src[j],
                                       	action, FALSE,
                                       	(mach_port_t *)&object_src[j]);

                              if (kr != KERN_SUCCESS) {
                                 rpc_error( ("ipc_object_copyout fail %d", kr),
                                            ERR_YELLOW );
				 clean_port_array(object_src, svr_buf[offset+1],
						  j, MACH_RPC_OUT);
                                 unwind_rpc_state(routine, state, svr_buf);
                                 return kr;
                              }
                           }
			}

                        kr = (*outfunc)((char *)src, (char *)dst, size);
                        if (kr) {
                                /* State doesn't know about vm_alloc'd dst */
                        	rpc_error( ("copyout fail"), ERR_GREEN );
                                (void) vm_deallocate(map, dst, size);
                                unwind_rpc_state(routine, state, svr_buf);
                                return KERN_FAILURE;
                        }

                        /* Since copyout was successful, free kernel buffer
                           and convert state from _ALLOC_KRN to _ALLOC_CLN */
                        if (state[i].copy_type == COPY_TYPE_ALLOC_KRN) {
                                KFREE(state[i].alloc_addr, size, rt);
			}
                        state[i].copy_type = COPY_TYPE_ALLOC_CLN;
                        state[i].alloc_addr = dst;
                     }

	   	     /* Store addr of result data in client's indirect ptr */
		     stk = cln_buf[offset];
		     kr = (*outfunc)((char *)&dst, (char *)stk, sizeof(dst));
                     if (kr) {
			rpc_error( ("copyout fail"), ERR_YELLOW );
                        unwind_rpc_state(routine, state, svr_buf);
                        return KERN_FAILURE;
                      }

	   	     /* Store count of result data in client's indirect ptr */
		     if (type & MACH_RPC_VARIABLE) {
		     	stk = cln_buf[offset+1];
		     	kr = (*outfunc)((char *)&svr_buf[offset+1],
				(char *)stk, sizeof(vm_size_t));
                     	if (kr) {
				rpc_error( ("copyout fail"), ERR_YELLOW );
                        	unwind_rpc_state(routine, state, svr_buf);
                        	return KERN_FAILURE;
                      	}
		      }

		     break;

		default:
                     if (direct & MACH_RPC_OUT) {
                     	rpc_error( ("reply out: no case index=%d type=0x%x", i,
                                   type | direct | point), ERR_RED );
		     }

		}
	}


	return kr;
}




/*
 *	rpc_error_severity
 *
 *	Process an rpc subsystem error according to its severity.
 */

void
rpc_error_severity(int severity)
{

	if (severity == ERR_FATAL) {
		panic("rpc_error_severity");
	}

	return;
}

void
rpc_error_show_severity(int severity)
{
	switch(severity) {
		case ERR_INFO:
		     printf("(INFO) ");
		     break;

		case ERR_GREEN:
		     printf("(GREEN) ");
		     break;

		case ERR_YELLOW:
		     printf("(YELLOW) ");
		     break;

		case ERR_RED:
		     printf("(RED) ");
		     break;

		case ERR_FATAL:
		     printf("(FATAL) ");
		     break;

		default:
		     break;
	}
        return;
}





/*
 * 	RPC error reply 
 *
 *	Executed when on the client activation and the client act-
 *	ivation's kernel stack. Returns to the client.
 */
kern_return_t
mach_rpc_return_error(void)
{
        kern_return_t                   ret, kr;
	thread_act_t			cln_act;
        vm_offset_t                     pcb_stack;
        unsigned int                    uesp, ueip;


	cln_act = current_act();
        ret = (kern_return_t) MACH_RPC_RET(cln_act);

	kr = unwind_invoke_state(cln_act);

	/*
 	 *	Return to the client via an upcall.
	 */
	pcb_stack = (vm_offset_t) USER_REGS(cln_act);
	ueip = MACH_RPC_UIP(cln_act); 
	uesp = MACH_RPC_USP(cln_act);

	rpc_upcall(pcb_stack, uesp, ueip, ret);
	/* NOTREACHED */

	return kr;
}


/*
 * 	RPC error cleanup 
 */
kern_return_t
unwind_invoke_state(thread_act_t thr_act)
{
	int				i;
        char                            *argv;
        kern_return_t                   ret;
	vm_size_t			size, state_size;
        unsigned int                    type, offset;
        routine_arg_descriptor_t        dsc;
        routine_descriptor_t            routine;
        struct ipc_port                 port_data;
        ipc_port_t                      port = &port_data;
        mach_rpc_id_t                   routine_num;
        rpc_subsystem_t                 subsystem;
        rpc_copy_state_t                state;


	ret = KERN_SUCCESS;
        argv = MACH_RPC_ARGV(thr_act);

	/*
	 *	Short circuited rpc has no accumulated state 
	 *	to unwind.
	 */
	if (!thr_act->r_state) {
		return(ret);
	}

      	routine_num = thr_act->r_routine_num;
        port->ip_flags = (ipc_port_flags_t) thr_act->r_port_flags;
        subsystem = thr_act->r_subsystem;
        assert(subsystem != NULL);
        routine = &subsystem->routine[routine_num - subsystem->start];
        dsc = routine->arg_descr;

        state = thr_act->r_state;
        assert( state != NULL );


	/*
	 *	Since the rpc argument processing is complete, remove
	 *	all kernel allocations.
	 */
        for ( i = 1 ; i < routine->descr_count ; i++ ) {
            switch( state[i].copy_type ) {
                case COPY_TYPE_ALLOC_KRN:
            		type = RPC_FIELD(dsc[i].type, TYPE_SHIFT, 
                                                 LAST_TYPE_BIT);
			if (type == MACH_RPC_ARRAY_VARIABLE) {
				offset = dsc[i].offset/sizeof(int);
#if 0				/* XXX svr buff not available here XXX */
				size = svr_arg_buf[offset+1] * dsc[i].size;
#else
				continue;
#endif
			}
			else {
				size = DSCSIZE;
			}
                        KFREE(state[i].alloc_addr, size, IP_RT(port));
			state[i].copy_type = COPY_TYPE_NO_COPY;
			state[i].alloc_addr = 0;
                        break;
                default:
                        break;
            }
        }

        /*
         *      Release the rpc copy state record, if we allocated one.
         */
        if (state[0].copy_type == COPY_TYPE_ALLOC_KRN) {
                state_size = routine->descr_count *
                             sizeof(rpc_copy_state_data_t);
                KFREE((vm_offset_t)state, state_size, IP_RT(port));
        }
#if 0
        if (svr_arg_buf != svr_arg_buf_data) {
                KFREE((vm_offset_t)svr_arg_buf, nbyte, IP_RT(port));
        }
#endif

	/* Clear field to indicate `state' has been deallocated */
	thr_act->r_state = NULL;

	return ret;
}

