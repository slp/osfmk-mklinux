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


#include <mach_perf.h>
#include <pager_types.h>

#include <mach/memory_object.h>

struct pager_object {
	vm_size_t				size;
	int					flags;
	struct memory_object_behave_info	behavior;
	struct memory_object_attr_info		attributes;
	vm_prot_t				lock;
	mach_port_t				control;
	vm_offset_t				datas[1]; /* 1 per page */
};

typedef struct pager_object *pager_object_t;

boolean_t       pager_server();
boolean_t       memory_object_server();
boolean_t       pager_demux();

mach_port_t 	server_port;
mach_port_t 	port_set;
jmp_buf 	saved_state;
int		pager_server_debug;

pager_server_main(service_port)
mach_port_t service_port;
{
        char **argv;
        int argc = 0;

        test_init();

	if (service_port) {
		server_port = service_port;
	} else {
		MACH_CALL( mach_port_allocate, (mach_task_self(),
						MACH_PORT_RIGHT_RECEIVE,
						&server_port));
		if (debug)
			printf("server port %x\n", server_port);
		MACH_CALL (netname_check_in, (name_server_port,
					      PAGER_SERVER_NAME,
					      mach_task_self(),
					      server_port));
	}
        MACH_CALL( mach_port_allocate, (mach_task_self(),
					MACH_PORT_RIGHT_PORT_SET,
					&port_set));
        MACH_CALL( mach_port_move_member, (mach_task_self(),
					   server_port,
					   port_set));

/*
	if(standalone)
		set_emm_priority();
*/

	pager_server_debug = debug;

	/*
	 *	Announce that we will service on this port.
	 */
	if (pager_server_debug)
		printf("calling server_announce_port on %x\n",
		       server_port);
	server_announce_port(server_port);
	test_end();

        if (pager_server_debug)
                printf("calling mach_msg_server: port %x (server %x)\n",
                       port_set, server);

	while (1) {
		thread_malloc_state_t mallocs;

		mallocs = save_mallocs(thread_self());
		if (mach_setjmp(saved_state))
			restore_mallocs(thread_self(), mallocs);
		else {
			MACH_CALL( mach_msg_server,
				  (pager_demux,
				   PAGER_MSG_BUF_SIZE,
				   port_set,
				   MACH_MSG_OPTION_NONE));
		}
	}
}

boolean_t
pager_demux(in, out)
{
	if (memory_object_server(in, out))
		return(TRUE);
	else if (pager_server(in, out))
		return(TRUE);
	else
		return(server_server(in, out));
}

kern_return_t
do_create_memory_objects(
	mach_port_t		server,
	int			nobjs,
	mach_port_array_t	*obj_ports,
	mach_msg_type_number_t	*count,
	vm_size_t		size,
	int			flags,
	vm_prot_t      		prot)
{
	mach_port_array_t	ports;
	pager_object_t		objects, obj;
	mach_port_t		port;
	vm_offset_t		data;
	int			i;

	if (pager_server_debug) {
		printf("create_memory_objects: nobjs %d size %d flags %x\n",
			      nobjs, size, flags);
		printf("                       prot %d\n",
			      prot);
	}

	MACH_CALL(vm_allocate, (mach_task_self(),
				(vm_offset_t *)&ports,
				nobjs*sizeof(mach_port_t),
				TRUE));

	MACH_CALL(vm_allocate,
		  (mach_task_self(),
		   (vm_offset_t *)&objects,
		   nobjs*(sizeof(struct pager_object)
			  +(size/vm_page_size) * sizeof(vm_offset_t)),
		   TRUE));

	if (flags & FILL)
		MACH_CALL(vm_allocate, (mach_task_self(),
					&data,
					size*nobjs,
					TRUE));

	for (i = 0; i < nobjs; i++) {
	  	obj = objects+i;
		port = (mach_port_t)obj;
		MACH_CALL(mach_port_allocate_name, (mach_task_self(),
					      MACH_PORT_RIGHT_RECEIVE,
					      port));
		*(ports+i) = port;
		obj->size = size;
		obj->flags = flags;
		obj->lock = VM_PROT_ALL & ~prot;
		obj->control = MACH_PORT_NULL;
		obj->attributes.may_cache_object = 
					(flags & CACHED) ? TRUE:FALSE;
		obj->attributes.copy_strategy = MEMORY_OBJECT_COPY_TEMPORARY;
                obj->attributes.cluster_size = vm_page_size;
                obj->attributes.temporary = (flags & CACHED) ? FALSE:TRUE;
		obj->behavior.copy_strategy = MEMORY_OBJECT_COPY_TEMPORARY;
		obj->behavior.temporary = (flags & CACHED) ? FALSE:TRUE;
		obj->behavior.invalidate = FALSE;
		if (pager_server_debug) {
			printf("may_cache %x\n", obj->attributes.may_cache_object);
			printf("temporary %x\n", obj->behavior.temporary);
		}
		if (flags & FILL) {
			int j;
			int npages = size/vm_page_size;

			for (j = 0; j < npages; j ++) {
				obj->datas[j] = data + j*vm_page_size;
				*(int *)(obj->datas[j]) = READ_PATTERN;
			}
		}
		data += size;
		if (!(flags & NO_LISTEN))
			MACH_CALL( mach_port_move_member, (mach_task_self(),
							   port,
							   port_set));
	}		

	if (pager_server_debug)
		printf("create_memory_objects: ports (%x) = %x\n",
			      ports, *ports);
	*count = nobjs;
	*obj_ports = ports;
	return(KERN_SUCCESS);
}

kern_return_t
do_destroy_memory_objects(
	mach_port_t		server,
	mach_port_array_t	obj_ports,
	mach_msg_type_number_t	count)
{
	/* 
	 * Assumes the ports to be detroyed have
	 * been allocated together at once with
	 * create_pager_objects
	 */  

	pager_object_t		obj;
	mach_port_t		port;
	int			i;

	for (i = 0; i < count; i++) {
	  	port = *(obj_ports+i);
		obj = (pager_object_t) port;
		if (pager_server_debug)
			printf("destroy_memory_objects: port %d: %x\n",
			       i, port);

		if ((obj->flags & CACHED) && obj->control != MACH_PORT_NULL) {
			MACH_CALL( memory_object_destroy, (obj->control, 0));
		}
		if (!(obj->flags & NO_LISTEN)) {
			MACH_CALL( mach_port_move_member, (mach_task_self(),
							   port,
							   MACH_PORT_NULL));
		}
		MACH_CALL(mach_port_destroy, (mach_task_self(),
					      port));
		if (obj->flags & FILL) {
			int j;
			int npages = obj->size/vm_page_size;

			for (j = 0; j < npages; j++) {
				if (obj->datas[j]) {
					MACH_CALL(vm_deallocate,
						  (mach_task_self(),
						   obj->datas[j],
						   vm_page_size));
				}
			}
		}
		if (obj->control != MACH_PORT_NULL)
			MACH_CALL(mach_port_destroy, (mach_task_self(),
						      obj->control));
	}

	obj = (pager_object_t) *obj_ports;


	MACH_CALL(vm_deallocate,
		  (mach_task_self(),
		   (vm_offset_t)obj,
		   count*(sizeof(struct pager_object)
			  +(obj->size/vm_page_size) * sizeof(vm_offset_t))));

	MACH_CALL(vm_deallocate, (mach_task_self(),
				(vm_offset_t)obj_ports,
				count*sizeof(mach_port_t)));

	return(KERN_SUCCESS);
}

kern_return_t
memory_object_init(
	mach_port_t     pager,
	mach_port_t     control,
	vm_size_t       size)
{
	pager_object_t obj;
	memory_object_flavor_t	flavor;
	mach_msg_type_number_t	count;

	if (pager_server_debug)
		printf("memory object init\n");

	obj = (pager_object_t) pager;
        flavor = MEMORY_OBJECT_ATTRIBUTE_INFO;
        count = MEMORY_OBJECT_ATTR_INFO_COUNT;

	if (!(obj->flags & NO_INIT)) {
		MACH_CALL (memory_object_change_attributes,
			   (control,
			    flavor,
			    (memory_object_info_t) &obj->attributes,
			    count,
			    MACH_PORT_NULL));
	}
	obj->control = control;
	server_count--;
	return(KERN_SUCCESS);
}

kern_return_t
memory_object_terminate(
	mach_port_t     pager,
	mach_port_t     control)
{
	pager_object_t	obj = (pager_object_t) pager;

	if (pager_server_debug)
		printf("memory object terminate\n");

	if (!(obj->flags & NO_TERMINATE)) {
		/*
		 * We have to avoid extra mach_port_destroy for 
		 * the port control we are destroying here
		 */
                if (obj->control == control) {
                        obj->control = MACH_PORT_NULL;
			MACH_CALL(mach_port_destroy, (mach_task_self(), control));
                }
		else
			printf("m_o_terminate: pager_object table is broken\n");

	}
	server_count--;
	return(KERN_SUCCESS);
}

kern_return_t
memory_object_data_request(
	memory_object_t pager,
	mach_port_t     reply,
	vm_offset_t     offset,
	vm_size_t       length,
	vm_prot_t       prot)
{
	pager_object_t obj;
	vm_offset_t data;

	if  (pager_server_debug)
		printf("memory_object_data_request(%x, %x, %x, %x, %x)\n",
		       pager, reply, offset, length, prot);
	obj = (pager_object_t) pager;
	data = obj->datas[offset/page_size_for_vm];
	if (obj->flags & PRECIOUS)
		obj->datas[offset/page_size_for_vm] = 0;
	if (obj->flags & FILL) {
		if  (pager_server_debug)
			printf("data_supply %x lock %x precious %x\n",
			       offset, obj->lock, obj->flags & PRECIOUS);

		MACH_CALL(memory_object_data_supply,
			  (reply,
			   offset,
			   data,
			   length,
			   (obj->flags & PRECIOUS), /* deallocate */
			   obj->lock,
			   (obj->flags & PRECIOUS),
			   MACH_PORT_NULL));
	} else {
		if  (pager_server_debug)
			printf("data_unavailable %x\n",
			       offset);

		MACH_CALL(memory_object_data_unavailable,
			  (reply,
			   offset,
			   length));
	}

	server_count--;
	return(KERN_SUCCESS);
}

kern_return_t
memory_object_data_unlock(
        memory_object_t pager,
        mach_port_t     control,
        vm_offset_t     offset,
        vm_size_t       size,
	vm_prot_t	prot)
{
	if (pager_server_debug) 
		printf("memory object unlock off : %d size : %d prot %x\n",
		       offset, size, prot);

	MACH_CALL(memory_object_lock_request, (control,
					       offset,
					       size,
					       MEMORY_OBJECT_RETURN_ALL,
					       FALSE, /* should flush */
					       VM_PROT_ALL & ~prot,
					       MACH_PORT_NULL));

	server_count--;
        return(KERN_SUCCESS);
}

kern_return_t
memory_object_copy(
        memory_object_t 	old_memory_object,
        memory_object_control_t old_memory_control,
        vm_offset_t     	offset,
        vm_size_t       	length,
        memory_object_t 	new_memory_object)
{
	if (pager_server_debug) 
	  printf("memory object copy\n");

        return(KERN_FAILURE);
}
kern_return_t
memory_object_lock_completed(
        memory_object_t pager,
        mach_port_t     pager_request,
        vm_offset_t     offset,
        vm_size_t       length)
{
	if (pager_server_debug) 
		printf("memory object lock completed\n");

        return(KERN_SUCCESS);
}

kern_return_t
memory_object_supply_completed(
        memory_object_t pager,
        mach_port_t     pager_request,
        vm_offset_t     offset,
        vm_size_t       length,
        kern_return_t   result,
        vm_offset_t     error_offset)
{
	if (pager_server_debug)
		printf("memory object supply completed\n");

        return(KERN_FAILURE);
}
kern_return_t
memory_object_data_return(
        memory_object_t pager,
        mach_port_t     control,
        vm_offset_t     offset,
        pointer_t       addr,
        vm_size_t       size,
        boolean_t       dirty,
        boolean_t       copy)
{
	pager_object_t obj;
	vm_offset_t		data;

	if (pager_server_debug)
		printf("memory object data return: offset :%x size :%x\n",
		       offset,size);

	obj = (pager_object_t) pager;
	if (dirty) {
		if (*(int *)addr != WRITE_PATTERN)
			printf("data_return: invalid content (%x instead of %x)\n",
			       *(int *)addr, WRITE_PATTERN);
	} else {
		if (*(int *)addr != READ_PATTERN)
			printf("data_return: invalid content (%x instead of %x)\n",
			       *(int *)addr, READ_PATTERN);
	}
	if ((obj->flags & PRECIOUS) && !obj->datas[offset/vm_page_size]) {
		/* save a copy of the data */

		MACH_CALL(vm_allocate, (mach_task_self(),
					&data,
					vm_page_size,
					TRUE));
		*(int *)data = *(int *)addr;
		obj->datas[offset/vm_page_size] = data;
	}

	MACH_CALL(vm_deallocate, (mach_task_self(), addr, vm_page_size));
	return(KERN_SUCCESS);
}

kern_return_t
memory_object_synchronize(
        mach_port_t             pager,
        mach_port_t             control,
        vm_offset_t             offset,
        vm_offset_t             length,
        vm_sync_t               flags )
{
	if (pager_server_debug) 
		printf("memory object synchronize\n");

	MACH_CALL(memory_object_synchronize_completed,(control,
						       offset,
						       length));
	server_count--;
	return(KERN_SUCCESS);
}

kern_return_t
memory_object_rejected(
        mach_port_t             pager,
        mach_port_t             pager_request,
        kern_return_t           reason)
{
	if (pager_server_debug) 
		printf("memory object rejected\n");

	return(KERN_FAILURE);
}

kern_return_t
memory_object_change_completed(
        mach_port_t             reply,
        mach_port_t             pager_request,
        memory_object_flavor_t  flavor)
{
	if (pager_server_debug) 
		printf("memory object change completed\n");

	return(KERN_FAILURE);
}

kern_return_t
memory_object_discard_request(
        mach_port_t             reply,
        mach_port_t             pager_request,
	vm_offset_t offset, 
	vm_size_t length)
{
	if (pager_server_debug) 
		printf("memory object discrad request\n");

	return KERN_FAILURE;
}

kern_return_t
do_mo_attr_test(
	mach_port_t 	me, 
	mach_port_t 	object,
	int 		op)
{
	register		i;
	pager_object_t 		obj;
	mach_port_t     	control;
	memory_object_flavor_t	flavor;
	memory_object_info_t 	mo_info;
	mach_msg_type_number_t	count;
	

	if  (pager_server_debug) {
		printf("do_mo_attr_test server %x loops %x\n", server, loops);
	}

	obj = (pager_object_t) object;
	control = obj->control;
	mo_info = (memory_object_info_t) &obj->attributes;
	flavor = MEMORY_OBJECT_ATTRIBUTE_INFO;
	count = MEMORY_OBJECT_ATTR_INFO_COUNT;

	start_time();

	for (i=loops; i--; )  {
		if (op == GET_ATTR) {
	  		MACH_CALL (memory_object_get_attributes,
				   (control,
				    flavor, 
				    mo_info,
				    &count));
		} else {
			MACH_CALL (memory_object_change_attributes,
				   (control,
				    flavor,
				    mo_info,
				    count,
				    MACH_PORT_NULL));
		}
	}

	stop_time();
	return(KERN_SUCCESS);
}
	       
do_mo_data_supply_test(
	mach_port_t 	me, 
	mach_port_t 	object,
	int 		op)
{
	register		i;
	pager_object_t 		obj;
	mach_port_t     	control;
	vm_offset_t		offset, *data;
	

	if  (pager_server_debug) {
		printf("do_mo_data_supply_test server %x loops %x\n", server, loops);
	}

	obj = (pager_object_t) object;
	control = obj->control;

	start_time();

	for (i=loops, offset = 0, data = obj->datas;
	     i--; offset += page_size_for_vm, data++)  {
		MACH_CALL(memory_object_data_supply,
			  (control,
			   offset,
			   *data,
			   page_size_for_vm,
			   (obj->flags & PRECIOUS), /* deallocate */
			   obj->lock,
			   (obj->flags & PRECIOUS),
			   MACH_PORT_NULL));
	}

	stop_time();
	return(KERN_SUCCESS);
}

/* run the server at a higher priority */
set_emm_priority()
{
        policy_base_t                   base;
        policy_limit_t                  limit;
        policy_rr_base_data_t         rr_base;
        policy_rr_limit_data_t        rr_limit;

        base = (policy_base_t) &rr_base;
        limit = (policy_limit_t) &rr_limit;
	rr_base.quantum = 0;
        rr_base.base_priority = 80;
        rr_limit.max_priority = 80;

	MACH_CALL(thread_set_policy, (mach_thread_self(),
				get_processor_set(), 
				POLICY_RR,
                                base, 
				POLICY_RR_BASE_COUNT,
                                limit,
				POLICY_RR_LIMIT_COUNT
				));
}
