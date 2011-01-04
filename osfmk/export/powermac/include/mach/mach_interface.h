#ifndef	_mach_user_
#define	_mach_user_

/* Module mach */

#include <string.h>
#include <mach/ndr.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/notify.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/port.h>

#ifdef AUTOTEST
#ifndef FUNCTION_PTR_T
#define FUNCTION_PTR_T
typedef void (*function_ptr_t)(mach_port_t, char *, mach_msg_type_number_t);
typedef struct {
        char            *name;
        function_ptr_t  function;
} function_table_entry;
typedef function_table_entry 	*function_table_t;
#endif /* FUNCTION_PTR_T */
#endif /* AUTOTEST */

#ifndef	mach_MSG_COUNT
#define	mach_MSG_COUNT	103
#endif	/* mach_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* Routine task_wire */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_wire
#if	defined(LINTLIBRARY)
    (target_task, must_wire)
	mach_port_t target_task;
	boolean_t must_wire;
{ return task_wire(target_task, must_wire); }
#else
(
	mach_port_t target_task,
	boolean_t must_wire
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_create
#if	defined(LINTLIBRARY)
    (target_task, ledger_ports, ledger_portsCnt, inherit_memory, child_task)
	mach_port_t target_task;
	ledger_port_array_t ledger_ports;
	mach_msg_type_number_t ledger_portsCnt;
	boolean_t inherit_memory;
	mach_port_t *child_task;
{ return task_create(target_task, ledger_ports, ledger_portsCnt, inherit_memory, child_task); }
#else
(
	mach_port_t target_task,
	ledger_port_array_t ledger_ports,
	mach_msg_type_number_t ledger_portsCnt,
	boolean_t inherit_memory,
	mach_port_t *child_task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine memory_object_get_attributes */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_get_attributes
#if	defined(LINTLIBRARY)
    (memory_control, flavor, attributes, attributesCnt)
	mach_port_t memory_control;
	memory_object_flavor_t flavor;
	memory_object_info_t attributes;
	mach_msg_type_number_t *attributesCnt;
{ return memory_object_get_attributes(memory_control, flavor, attributes, attributesCnt); }
#else
(
	mach_port_t memory_control,
	memory_object_flavor_t flavor,
	memory_object_info_t attributes,
	mach_msg_type_number_t *attributesCnt
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_change_attributes */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_change_attributes
#if	defined(LINTLIBRARY)
    (memory_control, flavor, attributes, attributesCnt, reply_to)
	mach_port_t memory_control;
	memory_object_flavor_t flavor;
	memory_object_info_t attributes;
	mach_msg_type_number_t attributesCnt;
	mach_port_t reply_to;
{ return memory_object_change_attributes(memory_control, flavor, attributes, attributesCnt, reply_to); }
#else
(
	mach_port_t memory_control,
	memory_object_flavor_t flavor,
	memory_object_info_t attributes,
	mach_msg_type_number_t attributesCnt,
	mach_port_t reply_to
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_region */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_region
#if	defined(LINTLIBRARY)
    (target_task, address, size, flavor, info, infoCnt, object_name)
	mach_port_t target_task;
	vm_address_t *address;
	vm_size_t *size;
	vm_region_flavor_t flavor;
	vm_region_info_t info;
	mach_msg_type_number_t *infoCnt;
	mach_port_t *object_name;
{ return vm_region(target_task, address, size, flavor, info, infoCnt, object_name); }
#else
(
	mach_port_t target_task,
	vm_address_t *address,
	vm_size_t *size,
	vm_region_flavor_t flavor,
	vm_region_info_t info,
	mach_msg_type_number_t *infoCnt,
	mach_port_t *object_name
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_terminate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_terminate
#if	defined(LINTLIBRARY)
    (target_task)
	mach_port_t target_task;
{ return task_terminate(target_task); }
#else
(
	mach_port_t target_task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_get_emulation_vector */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_get_emulation_vector
#if	defined(LINTLIBRARY)
    (task, vector_start, emulation_vector, emulation_vectorCnt)
	mach_port_t task;
	int *vector_start;
	emulation_vector_t *emulation_vector;
	mach_msg_type_number_t *emulation_vectorCnt;
{ return task_get_emulation_vector(task, vector_start, emulation_vector, emulation_vectorCnt); }
#else
(
	mach_port_t task,
	int *vector_start,
	emulation_vector_t *emulation_vector,
	mach_msg_type_number_t *emulation_vectorCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_set_emulation_vector */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_emulation_vector
#if	defined(LINTLIBRARY)
    (task, vector_start, emulation_vector, emulation_vectorCnt)
	mach_port_t task;
	int vector_start;
	emulation_vector_t emulation_vector;
	mach_msg_type_number_t emulation_vectorCnt;
{ return task_set_emulation_vector(task, vector_start, emulation_vector, emulation_vectorCnt); }
#else
(
	mach_port_t task,
	int vector_start,
	emulation_vector_t emulation_vector,
	mach_msg_type_number_t emulation_vectorCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_threads */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_threads
#if	defined(LINTLIBRARY)
    (target_task, act_list, act_listCnt)
	mach_port_t target_task;
	thread_act_port_array_t *act_list;
	mach_msg_type_number_t *act_listCnt;
{ return task_threads(target_task, act_list, act_listCnt); }
#else
(
	mach_port_t target_task,
	thread_act_port_array_t *act_list,
	mach_msg_type_number_t *act_listCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_swappable */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_swappable
#if	defined(LINTLIBRARY)
    (host_priv, target_task, make_swappable)
	mach_port_t host_priv;
	mach_port_t target_task;
	boolean_t make_swappable;
{ return task_swappable(host_priv, target_task, make_swappable); }
#else
(
	mach_port_t host_priv,
	mach_port_t target_task,
	boolean_t make_swappable
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_create_security_token */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_create_security_token
#if	defined(LINTLIBRARY)
    (target_task, host_security, sec_token, ledger_ports, ledger_portsCnt, inherit_memory, child_task)
	mach_port_t target_task;
	mach_port_t host_security;
	security_token_t sec_token;
	ledger_port_array_t ledger_ports;
	mach_msg_type_number_t ledger_portsCnt;
	boolean_t inherit_memory;
	mach_port_t *child_task;
{ return task_create_security_token(target_task, host_security, sec_token, ledger_ports, ledger_portsCnt, inherit_memory, child_task); }
#else
(
	mach_port_t target_task,
	mach_port_t host_security,
	security_token_t sec_token,
	ledger_port_array_t ledger_ports,
	mach_msg_type_number_t ledger_portsCnt,
	boolean_t inherit_memory,
	mach_port_t *child_task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_set_security_token */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_security_token
#if	defined(LINTLIBRARY)
    (target_task, host_security, sec_token)
	mach_port_t target_task;
	mach_port_t host_security;
	security_token_t sec_token;
{ return task_set_security_token(target_task, host_security, sec_token); }
#else
(
	mach_port_t target_task,
	mach_port_t host_security,
	security_token_t sec_token
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_terminate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_terminate
#if	defined(LINTLIBRARY)
    (target_act)
	mach_port_t target_act;
{ return thread_terminate(target_act); }
#else
(
	mach_port_t target_act
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_get_state */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_get_state
#if	defined(LINTLIBRARY)
    (target_act, flavor, old_state, old_stateCnt)
	mach_port_t target_act;
	thread_state_flavor_t flavor;
	thread_state_t old_state;
	mach_msg_type_number_t *old_stateCnt;
{ return thread_get_state(target_act, flavor, old_state, old_stateCnt); }
#else
(
	mach_port_t target_act,
	thread_state_flavor_t flavor,
	thread_state_t old_state,
	mach_msg_type_number_t *old_stateCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_set_state */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_set_state
#if	defined(LINTLIBRARY)
    (target_act, flavor, new_state, new_stateCnt)
	mach_port_t target_act;
	thread_state_flavor_t flavor;
	thread_state_t new_state;
	mach_msg_type_number_t new_stateCnt;
{ return thread_set_state(target_act, flavor, new_state, new_stateCnt); }
#else
(
	mach_port_t target_act,
	thread_state_flavor_t flavor,
	thread_state_t new_state,
	mach_msg_type_number_t new_stateCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_allocate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_allocate
#if	defined(LINTLIBRARY)
    (target_task, address, size, anywhere)
	mach_port_t target_task;
	vm_address_t *address;
	vm_size_t size;
	boolean_t anywhere;
{ return vm_allocate(target_task, address, size, anywhere); }
#else
(
	mach_port_t target_task,
	vm_address_t *address,
	vm_size_t size,
	boolean_t anywhere
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_deallocate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_deallocate
#if	defined(LINTLIBRARY)
    (target_task, address, size)
	mach_port_t target_task;
	vm_address_t address;
	vm_size_t size;
{ return vm_deallocate(target_task, address, size); }
#else
(
	mach_port_t target_task,
	vm_address_t address,
	vm_size_t size
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_protect */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_protect
#if	defined(LINTLIBRARY)
    (target_task, address, size, set_maximum, new_protection)
	mach_port_t target_task;
	vm_address_t address;
	vm_size_t size;
	boolean_t set_maximum;
	vm_prot_t new_protection;
{ return vm_protect(target_task, address, size, set_maximum, new_protection); }
#else
(
	mach_port_t target_task,
	vm_address_t address,
	vm_size_t size,
	boolean_t set_maximum,
	vm_prot_t new_protection
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_inherit */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_inherit
#if	defined(LINTLIBRARY)
    (target_task, address, size, new_inheritance)
	mach_port_t target_task;
	vm_address_t address;
	vm_size_t size;
	vm_inherit_t new_inheritance;
{ return vm_inherit(target_task, address, size, new_inheritance); }
#else
(
	mach_port_t target_task,
	vm_address_t address,
	vm_size_t size,
	vm_inherit_t new_inheritance
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_read */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_read
#if	defined(LINTLIBRARY)
    (target_task, address, size, data, dataCnt)
	mach_port_t target_task;
	vm_address_t address;
	vm_size_t size;
	vm_offset_t *data;
	mach_msg_type_number_t *dataCnt;
{ return vm_read(target_task, address, size, data, dataCnt); }
#else
(
	mach_port_t target_task,
	vm_address_t address,
	vm_size_t size,
	vm_offset_t *data,
	mach_msg_type_number_t *dataCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_write */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_write
#if	defined(LINTLIBRARY)
    (target_task, address, data, dataCnt)
	mach_port_t target_task;
	vm_address_t address;
	vm_offset_t data;
	mach_msg_type_number_t dataCnt;
{ return vm_write(target_task, address, data, dataCnt); }
#else
(
	mach_port_t target_task,
	vm_address_t address,
	vm_offset_t data,
	mach_msg_type_number_t dataCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_copy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_copy
#if	defined(LINTLIBRARY)
    (target_task, source_address, size, dest_address)
	mach_port_t target_task;
	vm_address_t source_address;
	vm_size_t size;
	vm_address_t dest_address;
{ return vm_copy(target_task, source_address, size, dest_address); }
#else
(
	mach_port_t target_task,
	vm_address_t source_address,
	vm_size_t size,
	vm_address_t dest_address
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_msync */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_msync
#if	defined(LINTLIBRARY)
    (target_task, address, size, sync_flags)
	mach_port_t target_task;
	vm_address_t address;
	vm_size_t size;
	vm_sync_t sync_flags;
{ return vm_msync(target_task, address, size, sync_flags); }
#else
(
	mach_port_t target_task,
	vm_address_t address,
	vm_size_t size,
	vm_sync_t sync_flags
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_synchronize_completed */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_synchronize_completed
#if	defined(LINTLIBRARY)
    (memory_control, offset, length)
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_offset_t length;
{ return memory_object_synchronize_completed(memory_control, offset, length); }
#else
(
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_offset_t length
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_ports_register */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_ports_register
#if	defined(LINTLIBRARY)
    (target_task, init_port_set, init_port_setCnt)
	mach_port_t target_task;
	mach_port_array_t init_port_set;
	mach_msg_type_number_t init_port_setCnt;
{ return mach_ports_register(target_task, init_port_set, init_port_setCnt); }
#else
(
	mach_port_t target_task,
	mach_port_array_t init_port_set,
	mach_msg_type_number_t init_port_setCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_ports_lookup */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_ports_lookup
#if	defined(LINTLIBRARY)
    (target_task, init_port_set, init_port_setCnt)
	mach_port_t target_task;
	mach_port_array_t *init_port_set;
	mach_msg_type_number_t *init_port_setCnt;
{ return mach_ports_lookup(target_task, init_port_set, init_port_setCnt); }
#else
(
	mach_port_t target_task,
	mach_port_array_t *init_port_set,
	mach_msg_type_number_t *init_port_setCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_subsystem_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_subsystem_create
#if	defined(LINTLIBRARY)
    (target_task, user_subsys, user_subsysCnt, subsys)
	mach_port_t target_task;
	user_subsystem_t user_subsys;
	mach_msg_type_number_t user_subsysCnt;
	mach_port_t *subsys;
{ return mach_subsystem_create(target_task, user_subsys, user_subsysCnt, subsys); }
#else
(
	mach_port_t target_task,
	user_subsystem_t user_subsys,
	mach_msg_type_number_t user_subsysCnt,
	mach_port_t *subsys
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_behavior_set */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_behavior_set
#if	defined(LINTLIBRARY)
    (target_task, address, size, new_behavior)
	mach_port_t target_task;
	vm_address_t address;
	vm_size_t size;
	vm_behavior_t new_behavior;
{ return vm_behavior_set(target_task, address, size, new_behavior); }
#else
(
	mach_port_t target_task,
	vm_address_t address,
	vm_size_t size,
	vm_behavior_t new_behavior
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_data_unavailable */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_data_unavailable
#if	defined(LINTLIBRARY)
    (memory_control, offset, size)
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t size;
{ return memory_object_data_unavailable(memory_control, offset, size); }
#else
(
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t size
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_set_default_memory_manager */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_set_default_memory_manager
#if	defined(LINTLIBRARY)
    (host_priv, default_manager)
	mach_port_t host_priv;
	mach_port_t *default_manager;
{ return vm_set_default_memory_manager(host_priv, default_manager); }
#else
(
	mach_port_t host_priv,
	mach_port_t *default_manager
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_lock_request */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_lock_request
#if	defined(LINTLIBRARY)
    (memory_control, offset, size, should_return, should_flush, lock_value, reply_to)
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t size;
	memory_object_return_t should_return;
	boolean_t should_flush;
	vm_prot_t lock_value;
	mach_port_t reply_to;
{ return memory_object_lock_request(memory_control, offset, size, should_return, should_flush, lock_value, reply_to); }
#else
(
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t size,
	memory_object_return_t should_return,
	boolean_t should_flush,
	vm_prot_t lock_value,
	mach_port_t reply_to
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_set_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_info
#if	defined(LINTLIBRARY)
    (target_task, flavor, task_info_in, task_info_inCnt)
	mach_port_t target_task;
	task_flavor_t flavor;
	task_info_t task_info_in;
	mach_msg_type_number_t task_info_inCnt;
{ return task_set_info(target_task, flavor, task_info_in, task_info_inCnt); }
#else
(
	mach_port_t target_task,
	task_flavor_t flavor,
	task_info_t task_info_in,
	mach_msg_type_number_t task_info_inCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_suspend */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_suspend
#if	defined(LINTLIBRARY)
    (target_task)
	mach_port_t target_task;
{ return task_suspend(target_task); }
#else
(
	mach_port_t target_task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_resume */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_resume
#if	defined(LINTLIBRARY)
    (target_task)
	mach_port_t target_task;
{ return task_resume(target_task); }
#else
(
	mach_port_t target_task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_get_special_port */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_get_special_port
#if	defined(LINTLIBRARY)
    (task, which_port, special_port)
	mach_port_t task;
	int which_port;
	mach_port_t *special_port;
{ return task_get_special_port(task, which_port, special_port); }
#else
(
	mach_port_t task,
	int which_port,
	mach_port_t *special_port
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_set_special_port */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_special_port
#if	defined(LINTLIBRARY)
    (task, which_port, special_port)
	mach_port_t task;
	int which_port;
	mach_port_t special_port;
{ return task_set_special_port(task, which_port, special_port); }
#else
(
	mach_port_t task,
	int which_port,
	mach_port_t special_port
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_create
#if	defined(LINTLIBRARY)
    (parent_task, child_act)
	mach_port_t parent_task;
	mach_port_t *child_act;
{ return thread_create(parent_task, child_act); }
#else
(
	mach_port_t parent_task,
	mach_port_t *child_act
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_suspend */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_suspend
#if	defined(LINTLIBRARY)
    (target_act)
	mach_port_t target_act;
{ return thread_suspend(target_act); }
#else
(
	mach_port_t target_act
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_resume */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_resume
#if	defined(LINTLIBRARY)
    (target_act)
	mach_port_t target_act;
{ return thread_resume(target_act); }
#else
(
	mach_port_t target_act
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_abort */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_abort
#if	defined(LINTLIBRARY)
    (target_act)
	mach_port_t target_act;
{ return thread_abort(target_act); }
#else
(
	mach_port_t target_act
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_get_special_port */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_get_special_port
#if	defined(LINTLIBRARY)
    (thr_act, which_port, special_port)
	mach_port_t thr_act;
	int which_port;
	mach_port_t *special_port;
{ return thread_get_special_port(thr_act, which_port, special_port); }
#else
(
	mach_port_t thr_act,
	int which_port,
	mach_port_t *special_port
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_set_special_port */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_set_special_port
#if	defined(LINTLIBRARY)
    (thr_act, which_port, special_port)
	mach_port_t thr_act;
	int which_port;
	mach_port_t special_port;
{ return thread_set_special_port(thr_act, which_port, special_port); }
#else
(
	mach_port_t thr_act,
	int which_port,
	mach_port_t special_port
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_set_emulation */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_emulation
#if	defined(LINTLIBRARY)
    (target_port, routine_entry_pt, routine_number)
	mach_port_t target_port;
	vm_address_t routine_entry_pt;
	int routine_number;
{ return task_set_emulation(target_port, routine_entry_pt, routine_number); }
#else
(
	mach_port_t target_port,
	vm_address_t routine_entry_pt,
	int routine_number
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_set_ras_pc */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_ras_pc
#if	defined(LINTLIBRARY)
    (target_task, basepc, boundspc)
	mach_port_t target_task;
	vm_address_t basepc;
	vm_address_t boundspc;
{ return task_set_ras_pc(target_task, basepc, boundspc); }
#else
(
	mach_port_t target_task,
	vm_address_t basepc,
	vm_address_t boundspc
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_abort_safely */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_abort_safely
#if	defined(LINTLIBRARY)
    (target_act)
	mach_port_t target_act;
{ return thread_abort_safely(target_act); }
#else
(
	mach_port_t target_act
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_map */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_map
#if	defined(LINTLIBRARY)
    (target_task, address, size, mask, anywhere, memory_object, offset, copy, cur_protection, max_protection, inheritance)
	mach_port_t target_task;
	vm_address_t *address;
	vm_size_t size;
	vm_address_t mask;
	boolean_t anywhere;
	mach_port_t memory_object;
	vm_offset_t offset;
	boolean_t copy;
	vm_prot_t cur_protection;
	vm_prot_t max_protection;
	vm_inherit_t inheritance;
{ return vm_map(target_task, address, size, mask, anywhere, memory_object, offset, copy, cur_protection, max_protection, inheritance); }
#else
(
	mach_port_t target_task,
	vm_address_t *address,
	vm_size_t size,
	vm_address_t mask,
	boolean_t anywhere,
	mach_port_t memory_object,
	vm_offset_t offset,
	boolean_t copy,
	vm_prot_t cur_protection,
	vm_prot_t max_protection,
	vm_inherit_t inheritance
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_data_error */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_data_error
#if	defined(LINTLIBRARY)
    (memory_control, offset, size, error_value)
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t size;
	kern_return_t error_value;
{ return memory_object_data_error(memory_control, offset, size, error_value); }
#else
(
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t size,
	kern_return_t error_value
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_destroy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_destroy
#if	defined(LINTLIBRARY)
    (memory_control, reason)
	mach_port_t memory_control;
	kern_return_t reason;
{ return memory_object_destroy(memory_control, reason); }
#else
(
	mach_port_t memory_control,
	kern_return_t reason
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_data_supply */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_data_supply
#if	defined(LINTLIBRARY)
    (memory_control, offset, data, dataCnt, dataDealloc, lock_value, precious, reply_to)
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_offset_t data;
	mach_msg_type_number_t dataCnt;
	boolean_t dataDealloc;
	vm_prot_t lock_value;
	boolean_t precious;
	mach_port_t reply_to;
{ return memory_object_data_supply(memory_control, offset, data, dataCnt, dataDealloc, lock_value, precious, reply_to); }
#else
(
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_offset_t data,
	mach_msg_type_number_t dataCnt,
	boolean_t dataDealloc,
	vm_prot_t lock_value,
	boolean_t precious,
	mach_port_t reply_to
);
#endif	/* defined(LINTLIBRARY) */

/* Routine kernel_task_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t kernel_task_create
#if	defined(LINTLIBRARY)
    (target_task, map_base, map_size, child_task)
	mach_port_t target_task;
	vm_offset_t map_base;
	vm_size_t map_size;
	mach_port_t *child_task;
{ return kernel_task_create(target_task, map_base, map_size, child_task); }
#else
(
	mach_port_t target_task,
	vm_offset_t map_base,
	vm_size_t map_size,
	mach_port_t *child_task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_create_running */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_create_running
#if	defined(LINTLIBRARY)
    (parent_task, flavor, new_state, new_stateCnt, child_act)
	mach_port_t parent_task;
	thread_state_flavor_t flavor;
	thread_state_t new_state;
	mach_msg_type_number_t new_stateCnt;
	mach_port_t *child_act;
{ return thread_create_running(parent_task, flavor, new_state, new_stateCnt, child_act); }
#else
(
	mach_port_t parent_task,
	thread_state_flavor_t flavor,
	thread_state_t new_state,
	mach_msg_type_number_t new_stateCnt,
	mach_port_t *child_act
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_info
#if	defined(LINTLIBRARY)
    (target_task, flavor, task_info_out, task_info_outCnt)
	mach_port_t target_task;
	task_flavor_t flavor;
	task_info_t task_info_out;
	mach_msg_type_number_t *task_info_outCnt;
{ return task_info(target_task, flavor, task_info_out, task_info_outCnt); }
#else
(
	mach_port_t target_task,
	task_flavor_t flavor,
	task_info_t task_info_out,
	mach_msg_type_number_t *task_info_outCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_machine_attribute */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_machine_attribute
#if	defined(LINTLIBRARY)
    (target_task, address, size, attribute, value)
	mach_port_t target_task;
	vm_address_t address;
	vm_size_t size;
	vm_machine_attribute_t attribute;
	vm_machine_attribute_val_t *value;
{ return vm_machine_attribute(target_task, address, size, attribute, value); }
#else
(
	mach_port_t target_task,
	vm_address_t address,
	vm_size_t size,
	vm_machine_attribute_t attribute,
	vm_machine_attribute_val_t *value
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_info
#if	defined(LINTLIBRARY)
    (target_act, flavor, thread_info_out, thread_info_outCnt)
	mach_port_t target_act;
	thread_flavor_t flavor;
	thread_info_t thread_info_out;
	mach_msg_type_number_t *thread_info_outCnt;
{ return thread_info(target_act, flavor, thread_info_out, thread_info_outCnt); }
#else
(
	mach_port_t target_act,
	thread_flavor_t flavor,
	thread_info_t thread_info_out,
	mach_msg_type_number_t *thread_info_outCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine etap_get_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t etap_get_info
#if	defined(LINTLIBRARY)
    (host_priv, et_entries, st_entries, et_offset, st_offset, cb_width, mb_size, mb_entries, mb_cpus)
	mach_port_t host_priv;
	int *et_entries;
	int *st_entries;
	vm_offset_t *et_offset;
	vm_offset_t *st_offset;
	int *cb_width;
	int *mb_size;
	int *mb_entries;
	int *mb_cpus;
{ return etap_get_info(host_priv, et_entries, st_entries, et_offset, st_offset, cb_width, mb_size, mb_entries, mb_cpus); }
#else
(
	mach_port_t host_priv,
	int *et_entries,
	int *st_entries,
	vm_offset_t *et_offset,
	vm_offset_t *st_offset,
	int *cb_width,
	int *mb_size,
	int *mb_entries,
	int *mb_cpus
);
#endif	/* defined(LINTLIBRARY) */

/* Routine etap_trace_thread */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t etap_trace_thread
#if	defined(LINTLIBRARY)
    (target_act, trace_status)
	mach_port_t target_act;
	boolean_t trace_status;
{ return etap_trace_thread(target_act, trace_status); }
#else
(
	mach_port_t target_act,
	boolean_t trace_status
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_mach
#define subsystem_to_name_map_mach \
    { "task_wire", 2000 },\
    { "task_create", 2001 },\
    { "memory_object_get_attributes", 2002 },\
    { "memory_object_change_attributes", 2003 },\
    { "vm_region", 2004 },\
    { "task_terminate", 2008 },\
    { "task_get_emulation_vector", 2009 },\
    { "task_set_emulation_vector", 2010 },\
    { "task_threads", 2011 },\
    { "task_swappable", 2012 },\
    { "task_create_security_token", 2013 },\
    { "task_set_security_token", 2014 },\
    { "thread_terminate", 2016 },\
    { "thread_get_state", 2017 },\
    { "thread_set_state", 2018 },\
    { "vm_allocate", 2021 },\
    { "vm_deallocate", 2023 },\
    { "vm_protect", 2024 },\
    { "vm_inherit", 2025 },\
    { "vm_read", 2026 },\
    { "vm_write", 2027 },\
    { "vm_copy", 2028 },\
    { "vm_msync", 2031 },\
    { "memory_object_synchronize_completed", 2032 },\
    { "mach_ports_register", 2033 },\
    { "mach_ports_lookup", 2034 },\
    { "mach_subsystem_create", 2035 },\
    { "vm_behavior_set", 2036 },\
    { "memory_object_data_unavailable", 2039 },\
    { "vm_set_default_memory_manager", 2041 },\
    { "memory_object_lock_request", 2044 },\
    { "task_set_info", 2052 },\
    { "task_suspend", 2056 },\
    { "task_resume", 2057 },\
    { "task_get_special_port", 2058 },\
    { "task_set_special_port", 2059 },\
    { "thread_create", 2061 },\
    { "thread_suspend", 2062 },\
    { "thread_resume", 2063 },\
    { "thread_abort", 2064 },\
    { "thread_get_special_port", 2067 },\
    { "thread_set_special_port", 2068 },\
    { "task_set_emulation", 2070 },\
    { "task_set_ras_pc", 2071 },\
    { "thread_abort_safely", 2072 },\
    { "vm_map", 2089 },\
    { "memory_object_data_error", 2090 },\
    { "memory_object_destroy", 2092 },\
    { "memory_object_data_supply", 2093 },\
    { "kernel_task_create", 2094 },\
    { "thread_create_running", 2096 },\
    { "task_info", 2098 },\
    { "vm_machine_attribute", 2099 },\
    { "thread_info", 2100 },\
    { "etap_get_info", 2101 },\
    { "etap_trace_thread", 2102 }
#endif

#endif	 /* _mach_user_ */
