#ifndef	_mach_host_user_
#define	_mach_host_user_

/* Module mach_host */

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

#ifndef	mach_host_MSG_COUNT
#define	mach_host_MSG_COUNT	111
#endif	/* mach_host_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* Routine host_processors */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_processors
#if	defined(LINTLIBRARY)
    (host, processor_list, processor_listCnt)
	mach_port_t host;
	processor_port_array_t *processor_list;
	mach_msg_type_number_t *processor_listCnt;
{ return host_processors(host, processor_list, processor_listCnt); }
#else
(
	mach_port_t host,
	processor_port_array_t *processor_list,
	mach_msg_type_number_t *processor_listCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_default_memory_manager */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_default_memory_manager
#if	defined(LINTLIBRARY)
    (host_priv, default_manager, cluster_size)
	mach_port_t host_priv;
	mach_port_t *default_manager;
	vm_size_t cluster_size;
{ return host_default_memory_manager(host_priv, default_manager, cluster_size); }
#else
(
	mach_port_t host_priv,
	mach_port_t *default_manager,
	vm_size_t cluster_size
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_start */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_start
#if	defined(LINTLIBRARY)
    (processor)
	mach_port_t processor;
{ return processor_start(processor); }
#else
(
	mach_port_t processor
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_exit */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_exit
#if	defined(LINTLIBRARY)
    (processor)
	mach_port_t processor;
{ return processor_exit(processor); }
#else
(
	mach_port_t processor
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_default */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_default
#if	defined(LINTLIBRARY)
    (host, default_set)
	mach_port_t host;
	mach_port_t *default_set;
{ return processor_set_default(host, default_set); }
#else
(
	mach_port_t host,
	mach_port_t *default_set
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_create
#if	defined(LINTLIBRARY)
    (host, new_set, new_name)
	mach_port_t host;
	mach_port_t *new_set;
	mach_port_t *new_name;
{ return processor_set_create(host, new_set, new_name); }
#else
(
	mach_port_t host,
	mach_port_t *new_set,
	mach_port_t *new_name
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_destroy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_destroy
#if	defined(LINTLIBRARY)
    (set)
	mach_port_t set;
{ return processor_set_destroy(set); }
#else
(
	mach_port_t set
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_assign */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_assign
#if	defined(LINTLIBRARY)
    (processor, new_set, wait)
	mach_port_t processor;
	mach_port_t new_set;
	boolean_t wait;
{ return processor_assign(processor, new_set, wait); }
#else
(
	mach_port_t processor,
	mach_port_t new_set,
	boolean_t wait
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_get_assignment */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_get_assignment
#if	defined(LINTLIBRARY)
    (processor, assigned_set)
	mach_port_t processor;
	mach_port_t *assigned_set;
{ return processor_get_assignment(processor, assigned_set); }
#else
(
	mach_port_t processor,
	mach_port_t *assigned_set
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_assign */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_assign
#if	defined(LINTLIBRARY)
    (thread, new_set)
	mach_port_t thread;
	mach_port_t new_set;
{ return thread_assign(thread, new_set); }
#else
(
	mach_port_t thread,
	mach_port_t new_set
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_assign_default */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_assign_default
#if	defined(LINTLIBRARY)
    (thread)
	mach_port_t thread;
{ return thread_assign_default(thread); }
#else
(
	mach_port_t thread
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_get_assignment */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_get_assignment
#if	defined(LINTLIBRARY)
    (thread, assigned_set)
	mach_port_t thread;
	mach_port_t *assigned_set;
{ return thread_get_assignment(thread, assigned_set); }
#else
(
	mach_port_t thread,
	mach_port_t *assigned_set
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_assign */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_assign
#if	defined(LINTLIBRARY)
    (task, new_set, assign_threads)
	mach_port_t task;
	mach_port_t new_set;
	boolean_t assign_threads;
{ return task_assign(task, new_set, assign_threads); }
#else
(
	mach_port_t task,
	mach_port_t new_set,
	boolean_t assign_threads
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_assign_default */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_assign_default
#if	defined(LINTLIBRARY)
    (task, assign_threads)
	mach_port_t task;
	boolean_t assign_threads;
{ return task_assign_default(task, assign_threads); }
#else
(
	mach_port_t task,
	boolean_t assign_threads
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_get_assignment */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_get_assignment
#if	defined(LINTLIBRARY)
    (task, assigned_set)
	mach_port_t task;
	mach_port_t *assigned_set;
{ return task_get_assignment(task, assigned_set); }
#else
(
	mach_port_t task,
	mach_port_t *assigned_set
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_kernel_version */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_kernel_version
#if	defined(LINTLIBRARY)
    (host, kernel_version)
	mach_port_t host;
	kernel_version_t kernel_version;
{ return host_kernel_version(host, kernel_version); }
#else
(
	mach_port_t host,
	kernel_version_t kernel_version
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_max_priority */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_max_priority
#if	defined(LINTLIBRARY)
    (processor_set, max_priority, change_threads)
	mach_port_t processor_set;
	int max_priority;
	boolean_t change_threads;
{ return processor_set_max_priority(processor_set, max_priority, change_threads); }
#else
(
	mach_port_t processor_set,
	int max_priority,
	boolean_t change_threads
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_policy_enable */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_policy_enable
#if	defined(LINTLIBRARY)
    (processor_set, policy)
	mach_port_t processor_set;
	int policy;
{ return processor_set_policy_enable(processor_set, policy); }
#else
(
	mach_port_t processor_set,
	int policy
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_policy_disable */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_policy_disable
#if	defined(LINTLIBRARY)
    (processor_set, policy, change_threads)
	mach_port_t processor_set;
	int policy;
	boolean_t change_threads;
{ return processor_set_policy_disable(processor_set, policy, change_threads); }
#else
(
	mach_port_t processor_set,
	int policy,
	boolean_t change_threads
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_tasks */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_tasks
#if	defined(LINTLIBRARY)
    (processor_set, task_list, task_listCnt)
	mach_port_t processor_set;
	task_port_array_t *task_list;
	mach_msg_type_number_t *task_listCnt;
{ return processor_set_tasks(processor_set, task_list, task_listCnt); }
#else
(
	mach_port_t processor_set,
	task_port_array_t *task_list,
	mach_msg_type_number_t *task_listCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_threads */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_threads
#if	defined(LINTLIBRARY)
    (processor_set, thread_list, thread_listCnt)
	mach_port_t processor_set;
	thread_port_array_t *thread_list;
	mach_msg_type_number_t *thread_listCnt;
{ return processor_set_threads(processor_set, thread_list, thread_listCnt); }
#else
(
	mach_port_t processor_set,
	thread_port_array_t *thread_list,
	mach_msg_type_number_t *thread_listCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_processor_sets */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_processor_sets
#if	defined(LINTLIBRARY)
    (host_priv, processor_sets, processor_setsCnt)
	mach_port_t host_priv;
	processor_set_name_port_array_t *processor_sets;
	mach_msg_type_number_t *processor_setsCnt;
{ return host_processor_sets(host_priv, processor_sets, processor_setsCnt); }
#else
(
	mach_port_t host_priv,
	processor_set_name_port_array_t *processor_sets,
	mach_msg_type_number_t *processor_setsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_processor_set_priv */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_processor_set_priv
#if	defined(LINTLIBRARY)
    (host_priv, set_name, set)
	mach_port_t host_priv;
	mach_port_t set_name;
	mach_port_t *set;
{ return host_processor_set_priv(host_priv, set_name, set); }
#else
(
	mach_port_t host_priv,
	mach_port_t set_name,
	mach_port_t *set
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_depress_abort */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_depress_abort
#if	defined(LINTLIBRARY)
    (thread)
	mach_port_t thread;
{ return thread_depress_abort(thread); }
#else
(
	mach_port_t thread
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_set_time */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_set_time
#if	defined(LINTLIBRARY)
    (host_priv, new_time)
	mach_port_t host_priv;
	time_value_t new_time;
{ return host_set_time(host_priv, new_time); }
#else
(
	mach_port_t host_priv,
	time_value_t new_time
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_adjust_time */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_adjust_time
#if	defined(LINTLIBRARY)
    (host_priv, new_adjustment, old_adjustment)
	mach_port_t host_priv;
	time_value_t new_adjustment;
	time_value_t *old_adjustment;
{ return host_adjust_time(host_priv, new_adjustment, old_adjustment); }
#else
(
	mach_port_t host_priv,
	time_value_t new_adjustment,
	time_value_t *old_adjustment
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_get_time */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_get_time
#if	defined(LINTLIBRARY)
    (host, current_time)
	mach_port_t host;
	time_value_t *current_time;
{ return host_get_time(host, current_time); }
#else
(
	mach_port_t host,
	time_value_t *current_time
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_reboot */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_reboot
#if	defined(LINTLIBRARY)
    (host_priv, options)
	mach_port_t host_priv;
	int options;
{ return host_reboot(host_priv, options); }
#else
(
	mach_port_t host_priv,
	int options
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_wire */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_wire
#if	defined(LINTLIBRARY)
    (host_priv, task, address, size, access)
	mach_port_t host_priv;
	mach_port_t task;
	vm_address_t address;
	vm_size_t size;
	vm_prot_t access;
{ return vm_wire(host_priv, task, address, size, access); }
#else
(
	mach_port_t host_priv,
	mach_port_t task,
	vm_address_t address,
	vm_size_t size,
	vm_prot_t access
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_wire */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_wire
#if	defined(LINTLIBRARY)
    (host_priv, thread, wired)
	mach_port_t host_priv;
	mach_port_t thread;
	boolean_t wired;
{ return thread_wire(host_priv, thread, wired); }
#else
(
	mach_port_t host_priv,
	mach_port_t thread,
	boolean_t wired
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_info
#if	defined(LINTLIBRARY)
    (host, flavor, host_info_out, host_info_outCnt)
	mach_port_t host;
	host_flavor_t flavor;
	host_info_t host_info_out;
	mach_msg_type_number_t *host_info_outCnt;
{ return host_info(host, flavor, host_info_out, host_info_outCnt); }
#else
(
	mach_port_t host,
	host_flavor_t flavor,
	host_info_t host_info_out,
	mach_msg_type_number_t *host_info_outCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_info
#if	defined(LINTLIBRARY)
    (processor, flavor, host, processor_info_out, processor_info_outCnt)
	mach_port_t processor;
	processor_flavor_t flavor;
	mach_port_t *host;
	processor_info_t processor_info_out;
	mach_msg_type_number_t *processor_info_outCnt;
{ return processor_info(processor, flavor, host, processor_info_out, processor_info_outCnt); }
#else
(
	mach_port_t processor,
	processor_flavor_t flavor,
	mach_port_t *host,
	processor_info_t processor_info_out,
	mach_msg_type_number_t *processor_info_outCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_control */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_control
#if	defined(LINTLIBRARY)
    (processor, processor_cmd, processor_cmdCnt)
	mach_port_t processor;
	processor_info_t processor_cmd;
	mach_msg_type_number_t processor_cmdCnt;
{ return processor_control(processor, processor_cmd, processor_cmdCnt); }
#else
(
	mach_port_t processor,
	processor_info_t processor_cmd,
	mach_msg_type_number_t processor_cmdCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_get_boot_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_get_boot_info
#if	defined(LINTLIBRARY)
    (host_priv, boot_info)
	mach_port_t host_priv;
	kernel_boot_info_t boot_info;
{ return host_get_boot_info(host_priv, boot_info); }
#else
(
	mach_port_t host_priv,
	kernel_boot_info_t boot_info
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_statistics */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_statistics
#if	defined(LINTLIBRARY)
    (host_priv, flavor, host_info_out, host_info_outCnt)
	mach_port_t host_priv;
	host_flavor_t flavor;
	host_info_t host_info_out;
	mach_msg_type_number_t *host_info_outCnt;
{ return host_statistics(host_priv, flavor, host_info_out, host_info_outCnt); }
#else
(
	mach_port_t host_priv,
	host_flavor_t flavor,
	host_info_t host_info_out,
	mach_msg_type_number_t *host_info_outCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_page_size */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_page_size
#if	defined(LINTLIBRARY)
    (host, page_size)
	mach_port_t host;
	vm_size_t *page_size;
{ return host_page_size(host, page_size); }
#else
(
	mach_port_t host,
	vm_size_t *page_size
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_processor_slots */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_processor_slots
#if	defined(LINTLIBRARY)
    (host, slots, slotsCnt)
	mach_port_t host;
	processor_slot_t slots;
	mach_msg_type_number_t *slotsCnt;
{ return host_processor_slots(host, slots, slotsCnt); }
#else
(
	mach_port_t host,
	processor_slot_t slots,
	mach_msg_type_number_t *slotsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_discard_reply */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_discard_reply
#if	defined(LINTLIBRARY)
    (memory_control, requested_offset, requested_size, discard_offset, discard_size, should_return, reply_to)
	mach_port_t memory_control;
	vm_offset_t requested_offset;
	vm_size_t requested_size;
	vm_offset_t discard_offset;
	vm_size_t discard_size;
	memory_object_return_t should_return;
	mach_port_t reply_to;
{ return memory_object_discard_reply(memory_control, requested_offset, requested_size, discard_offset, discard_size, should_return, reply_to); }
#else
(
	mach_port_t memory_control,
	vm_offset_t requested_offset,
	vm_size_t requested_size,
	vm_offset_t discard_offset,
	vm_size_t discard_size,
	memory_object_return_t should_return,
	mach_port_t reply_to
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_sample */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_sample
#if	defined(LINTLIBRARY)
    (thread, reply)
	mach_port_t thread;
	mach_port_t reply;
{ return thread_sample(thread, reply); }
#else
(
	mach_port_t thread,
	mach_port_t reply
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_sample */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_sample
#if	defined(LINTLIBRARY)
    (task, reply)
	mach_port_t task;
	mach_port_t reply;
{ return task_sample(task, reply); }
#else
(
	mach_port_t task,
	mach_port_t reply
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_remap */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_remap
#if	defined(LINTLIBRARY)
    (target_task, target_address, size, mask, anywhere, src_task, src_address, copy, cur_protection, max_protection, inheritance)
	mach_port_t target_task;
	vm_address_t *target_address;
	vm_size_t size;
	vm_address_t mask;
	boolean_t anywhere;
	mach_port_t src_task;
	vm_address_t src_address;
	boolean_t copy;
	vm_prot_t *cur_protection;
	vm_prot_t *max_protection;
	vm_inherit_t inheritance;
{ return vm_remap(target_task, target_address, size, mask, anywhere, src_task, src_address, copy, cur_protection, max_protection, inheritance); }
#else
(
	mach_port_t target_task,
	vm_address_t *target_address,
	vm_size_t size,
	vm_address_t mask,
	boolean_t anywhere,
	mach_port_t src_task,
	vm_address_t src_address,
	boolean_t copy,
	vm_prot_t *cur_protection,
	vm_prot_t *max_protection,
	vm_inherit_t inheritance
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_set_exception_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_set_exception_ports
#if	defined(LINTLIBRARY)
    (thread, exception_mask, new_port, behavior, new_flavor)
	mach_port_t thread;
	exception_mask_t exception_mask;
	mach_port_t new_port;
	exception_behavior_t behavior;
	thread_state_flavor_t new_flavor;
{ return thread_set_exception_ports(thread, exception_mask, new_port, behavior, new_flavor); }
#else
(
	mach_port_t thread,
	exception_mask_t exception_mask,
	mach_port_t new_port,
	exception_behavior_t behavior,
	thread_state_flavor_t new_flavor
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_set_exception_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_exception_ports
#if	defined(LINTLIBRARY)
    (task, exception_mask, new_port, behavior, new_flavor)
	mach_port_t task;
	exception_mask_t exception_mask;
	mach_port_t new_port;
	exception_behavior_t behavior;
	thread_state_flavor_t new_flavor;
{ return task_set_exception_ports(task, exception_mask, new_port, behavior, new_flavor); }
#else
(
	mach_port_t task,
	exception_mask_t exception_mask,
	mach_port_t new_port,
	exception_behavior_t behavior,
	thread_state_flavor_t new_flavor
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_get_exception_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_get_exception_ports
#if	defined(LINTLIBRARY)
    (thread, exception_mask, masks, masksCnt, old_ports, old_behaviors, old_flavors)
	mach_port_t thread;
	exception_mask_t exception_mask;
	exception_mask_array_t masks;
	mach_msg_type_number_t *masksCnt;
	exception_port_array_t old_ports;
	exception_behavior_array_t old_behaviors;
	exception_flavor_array_t old_flavors;
{ return thread_get_exception_ports(thread, exception_mask, masks, masksCnt, old_ports, old_behaviors, old_flavors); }
#else
(
	mach_port_t thread,
	exception_mask_t exception_mask,
	exception_mask_array_t masks,
	mach_msg_type_number_t *masksCnt,
	exception_port_array_t old_ports,
	exception_behavior_array_t old_behaviors,
	exception_flavor_array_t old_flavors
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_get_exception_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_get_exception_ports
#if	defined(LINTLIBRARY)
    (task, exception_mask, masks, masksCnt, old_ports, old_behaviors, old_flavors)
	mach_port_t task;
	exception_mask_t exception_mask;
	exception_mask_array_t masks;
	mach_msg_type_number_t *masksCnt;
	exception_port_array_t old_ports;
	exception_behavior_array_t old_behaviors;
	exception_flavor_array_t old_flavors;
{ return task_get_exception_ports(task, exception_mask, masks, masksCnt, old_ports, old_behaviors, old_flavors); }
#else
(
	mach_port_t task,
	exception_mask_t exception_mask,
	exception_mask_array_t masks,
	mach_msg_type_number_t *masksCnt,
	exception_port_array_t old_ports,
	exception_behavior_array_t old_behaviors,
	exception_flavor_array_t old_flavors
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_swap_exception_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_swap_exception_ports
#if	defined(LINTLIBRARY)
    (thread, exception_mask, new_port, behavior, new_flavor, masks, masksCnt, old_ports, old_behaviors, old_flavors)
	mach_port_t thread;
	exception_mask_t exception_mask;
	mach_port_t new_port;
	exception_behavior_t behavior;
	thread_state_flavor_t new_flavor;
	exception_mask_array_t masks;
	mach_msg_type_number_t *masksCnt;
	exception_port_array_t old_ports;
	exception_behavior_array_t old_behaviors;
	exception_flavor_array_t old_flavors;
{ return thread_swap_exception_ports(thread, exception_mask, new_port, behavior, new_flavor, masks, masksCnt, old_ports, old_behaviors, old_flavors); }
#else
(
	mach_port_t thread,
	exception_mask_t exception_mask,
	mach_port_t new_port,
	exception_behavior_t behavior,
	thread_state_flavor_t new_flavor,
	exception_mask_array_t masks,
	mach_msg_type_number_t *masksCnt,
	exception_port_array_t old_ports,
	exception_behavior_array_t old_behaviors,
	exception_flavor_array_t old_flavors
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_swap_exception_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_swap_exception_ports
#if	defined(LINTLIBRARY)
    (task, exception_mask, new_port, behavior, new_flavor, masks, masksCnt, old_ports, old_behaviors, old_flavors)
	mach_port_t task;
	exception_mask_t exception_mask;
	mach_port_t new_port;
	exception_behavior_t behavior;
	thread_state_flavor_t new_flavor;
	exception_mask_array_t masks;
	mach_msg_type_number_t *masksCnt;
	exception_port_array_t old_ports;
	exception_behavior_array_t old_behaviors;
	exception_flavor_array_t old_flavors;
{ return task_swap_exception_ports(task, exception_mask, new_port, behavior, new_flavor, masks, masksCnt, old_ports, old_behaviors, old_flavors); }
#else
(
	mach_port_t task,
	exception_mask_t exception_mask,
	mach_port_t new_port,
	exception_behavior_t behavior,
	thread_state_flavor_t new_flavor,
	exception_mask_array_t masks,
	mach_msg_type_number_t *masksCnt,
	exception_port_array_t old_ports,
	exception_behavior_array_t old_behaviors,
	exception_flavor_array_t old_flavors
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_policy_control */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_policy_control
#if	defined(LINTLIBRARY)
    (pset, flavor, policy_info, policy_infoCnt, change)
	mach_port_t pset;
	processor_set_flavor_t flavor;
	processor_set_info_t policy_info;
	mach_msg_type_number_t policy_infoCnt;
	boolean_t change;
{ return processor_set_policy_control(pset, flavor, policy_info, policy_infoCnt, change); }
#else
(
	mach_port_t pset,
	processor_set_flavor_t flavor,
	processor_set_info_t policy_info,
	mach_msg_type_number_t policy_infoCnt,
	boolean_t change
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_policy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_policy
#if	defined(LINTLIBRARY)
    (task, policy, base, baseCnt, set_limit, change)
	mach_port_t task;
	policy_t policy;
	policy_base_t base;
	mach_msg_type_number_t baseCnt;
	boolean_t set_limit;
	boolean_t change;
{ return task_policy(task, policy, base, baseCnt, set_limit, change); }
#else
(
	mach_port_t task,
	policy_t policy,
	policy_base_t base,
	mach_msg_type_number_t baseCnt,
	boolean_t set_limit,
	boolean_t change
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_set_policy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_policy
#if	defined(LINTLIBRARY)
    (task, pset, policy, base, baseCnt, limit, limitCnt, change)
	mach_port_t task;
	mach_port_t pset;
	policy_t policy;
	policy_base_t base;
	mach_msg_type_number_t baseCnt;
	policy_limit_t limit;
	mach_msg_type_number_t limitCnt;
	boolean_t change;
{ return task_set_policy(task, pset, policy, base, baseCnt, limit, limitCnt, change); }
#else
(
	mach_port_t task,
	mach_port_t pset,
	policy_t policy,
	policy_base_t base,
	mach_msg_type_number_t baseCnt,
	policy_limit_t limit,
	mach_msg_type_number_t limitCnt,
	boolean_t change
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_policy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_policy
#if	defined(LINTLIBRARY)
    (thr_act, policy, base, baseCnt, set_limit)
	mach_port_t thr_act;
	policy_t policy;
	policy_base_t base;
	mach_msg_type_number_t baseCnt;
	boolean_t set_limit;
{ return thread_policy(thr_act, policy, base, baseCnt, set_limit); }
#else
(
	mach_port_t thr_act,
	policy_t policy,
	policy_base_t base,
	mach_msg_type_number_t baseCnt,
	boolean_t set_limit
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_set_policy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_set_policy
#if	defined(LINTLIBRARY)
    (thr_act, pset, policy, base, baseCnt, limit, limitCnt)
	mach_port_t thr_act;
	mach_port_t pset;
	policy_t policy;
	policy_base_t base;
	mach_msg_type_number_t baseCnt;
	policy_limit_t limit;
	mach_msg_type_number_t limitCnt;
{ return thread_set_policy(thr_act, pset, policy, base, baseCnt, limit, limitCnt); }
#else
(
	mach_port_t thr_act,
	mach_port_t pset,
	policy_t policy,
	policy_base_t base,
	mach_msg_type_number_t baseCnt,
	policy_limit_t limit,
	mach_msg_type_number_t limitCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_statistics */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_statistics
#if	defined(LINTLIBRARY)
    (pset, flavor, info_out, info_outCnt)
	mach_port_t pset;
	processor_set_flavor_t flavor;
	processor_set_info_t info_out;
	mach_msg_type_number_t *info_outCnt;
{ return processor_set_statistics(pset, flavor, info_out, info_outCnt); }
#else
(
	mach_port_t pset,
	processor_set_flavor_t flavor,
	processor_set_info_t info_out,
	mach_msg_type_number_t *info_outCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t processor_set_info
#if	defined(LINTLIBRARY)
    (set_name, flavor, host, info_out, info_outCnt)
	mach_port_t set_name;
	int flavor;
	mach_port_t *host;
	processor_set_info_t info_out;
	mach_msg_type_number_t *info_outCnt;
{ return processor_set_info(set_name, flavor, host, info_out, info_outCnt); }
#else
(
	mach_port_t set_name,
	int flavor,
	mach_port_t *host,
	processor_set_info_t info_out,
	mach_msg_type_number_t *info_outCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine act_get_state */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t act_get_state
#if	defined(LINTLIBRARY)
    (target_act, flavor, old_state, old_stateCnt)
	mach_port_t target_act;
	int flavor;
	thread_state_t old_state;
	mach_msg_type_number_t *old_stateCnt;
{ return act_get_state(target_act, flavor, old_state, old_stateCnt); }
#else
(
	mach_port_t target_act,
	int flavor,
	thread_state_t old_state,
	mach_msg_type_number_t *old_stateCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine act_set_state */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t act_set_state
#if	defined(LINTLIBRARY)
    (target_act, flavor, new_state, new_stateCnt)
	mach_port_t target_act;
	int flavor;
	thread_state_t new_state;
	mach_msg_type_number_t new_stateCnt;
{ return act_set_state(target_act, flavor, new_state, new_stateCnt); }
#else
(
	mach_port_t target_act,
	int flavor,
	thread_state_t new_state,
	mach_msg_type_number_t new_stateCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_set_port_space */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_port_space
#if	defined(LINTLIBRARY)
    (task, table_entries)
	mach_port_t task;
	int table_entries;
{ return task_set_port_space(task, table_entries); }
#else
(
	mach_port_t task,
	int table_entries
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_allocate_cpm */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_allocate_cpm
#if	defined(LINTLIBRARY)
    (host_priv, task, address, size, anywhere)
	mach_port_t host_priv;
	mach_port_t task;
	vm_address_t *address;
	vm_size_t size;
	boolean_t anywhere;
{ return vm_allocate_cpm(host_priv, task, address, size, anywhere); }
#else
(
	mach_port_t host_priv,
	mach_port_t task,
	vm_address_t *address,
	vm_size_t size,
	boolean_t anywhere
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_mach_host
#define subsystem_to_name_map_mach_host \
    { "host_processors", 2600 },\
    { "host_default_memory_manager", 2601 },\
    { "processor_start", 2603 },\
    { "processor_exit", 2604 },\
    { "processor_set_default", 2606 },\
    { "processor_set_create", 2608 },\
    { "processor_set_destroy", 2609 },\
    { "processor_assign", 2611 },\
    { "processor_get_assignment", 2612 },\
    { "thread_assign", 2613 },\
    { "thread_assign_default", 2614 },\
    { "thread_get_assignment", 2615 },\
    { "task_assign", 2616 },\
    { "task_assign_default", 2617 },\
    { "task_get_assignment", 2618 },\
    { "host_kernel_version", 2619 },\
    { "processor_set_max_priority", 2623 },\
    { "processor_set_policy_enable", 2625 },\
    { "processor_set_policy_disable", 2626 },\
    { "processor_set_tasks", 2627 },\
    { "processor_set_threads", 2628 },\
    { "host_processor_sets", 2629 },\
    { "host_processor_set_priv", 2630 },\
    { "thread_depress_abort", 2631 },\
    { "host_set_time", 2632 },\
    { "host_adjust_time", 2633 },\
    { "host_get_time", 2634 },\
    { "host_reboot", 2635 },\
    { "vm_wire", 2636 },\
    { "thread_wire", 2637 },\
    { "host_info", 2638 },\
    { "processor_info", 2639 },\
    { "processor_control", 2641 },\
    { "host_get_boot_info", 2642 },\
    { "host_statistics", 2643 },\
    { "host_page_size", 2644 },\
    { "host_processor_slots", 2645 },\
    { "memory_object_discard_reply", 2660 },\
    { "thread_sample", 2677 },\
    { "task_sample", 2678 },\
    { "vm_remap", 2679 },\
    { "thread_set_exception_ports", 2680 },\
    { "task_set_exception_ports", 2681 },\
    { "thread_get_exception_ports", 2682 },\
    { "task_get_exception_ports", 2683 },\
    { "thread_swap_exception_ports", 2684 },\
    { "task_swap_exception_ports", 2685 },\
    { "processor_set_policy_control", 2690 },\
    { "task_policy", 2691 },\
    { "task_set_policy", 2692 },\
    { "thread_policy", 2693 },\
    { "thread_set_policy", 2694 },\
    { "processor_set_statistics", 2695 },\
    { "processor_set_info", 2696 },\
    { "act_get_state", 2707 },\
    { "act_set_state", 2708 },\
    { "task_set_port_space", 2709 },\
    { "vm_allocate_cpm", 2710 }
#endif

#endif	 /* _mach_host_user_ */
