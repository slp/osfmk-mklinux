#ifndef	_memory_object_server_
#define	_memory_object_server_

/* Module memory_object */

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

#ifndef	memory_object_MSG_COUNT
#define	memory_object_MSG_COUNT	15
#endif	/* memory_object_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* SimpleRoutine memory_object_init */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_init
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, memory_object_page_size)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_size_t memory_object_page_size;
{ return memory_object_init(memory_object, memory_control, memory_object_page_size); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_size_t memory_object_page_size
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_terminate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_terminate
#if	defined(LINTLIBRARY)
    (memory_object, memory_control)
	mach_port_t memory_object;
	mach_port_t memory_control;
{ return memory_object_terminate(memory_object, memory_control); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_data_request */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_data_request
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, length, desired_access)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{ return memory_object_data_request(memory_object, memory_control, offset, length, desired_access); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	vm_prot_t desired_access
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_data_unlock */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_data_unlock
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, length, desired_access)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
	vm_prot_t desired_access;
{ return memory_object_data_unlock(memory_object, memory_control, offset, length, desired_access); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	vm_prot_t desired_access
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_lock_completed */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_lock_completed
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, length)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
{ return memory_object_lock_completed(memory_object, memory_control, offset, length); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_supply_completed */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_supply_completed
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, length, result, error_offset)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
	kern_return_t result;
	vm_offset_t error_offset;
{ return memory_object_supply_completed(memory_object, memory_control, offset, length, result, error_offset); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length,
	kern_return_t result,
	vm_offset_t error_offset
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_data_return */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_data_return
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, data, dataCnt, dirty, kernel_copy)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_offset_t data;
	mach_msg_type_number_t dataCnt;
	boolean_t dirty;
	boolean_t kernel_copy;
{ return memory_object_data_return(memory_object, memory_control, offset, data, dataCnt, dirty, kernel_copy); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_offset_t data,
	mach_msg_type_number_t dataCnt,
	boolean_t dirty,
	boolean_t kernel_copy
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_synchronize */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_synchronize
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, length, sync_flags)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_offset_t length;
	vm_sync_t sync_flags;
{ return memory_object_synchronize(memory_object, memory_control, offset, length, sync_flags); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_offset_t length,
	vm_sync_t sync_flags
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_change_completed */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_change_completed
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, flavor)
	mach_port_t memory_object;
	mach_port_t memory_control;
	memory_object_flavor_t flavor;
{ return memory_object_change_completed(memory_object, memory_control, flavor); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	memory_object_flavor_t flavor
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_discard_request */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_discard_request
#if	defined(LINTLIBRARY)
    (memory_object, memory_control, offset, length)
	mach_port_t memory_object;
	mach_port_t memory_control;
	vm_offset_t offset;
	vm_size_t length;
{ return memory_object_discard_request(memory_object, memory_control, offset, length); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control,
	vm_offset_t offset,
	vm_size_t length
);
#endif	/* defined(LINTLIBRARY) */

extern boolean_t memory_object_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);

extern mig_routine_t memory_object_server_routine(
		mach_msg_header_t *InHeadP);


/* Description of this subsystem, for use in direct RPC */
extern struct memory_object_subsystem {
	struct subsystem *	subsystem;	/* Reserved for system use */
	mach_msg_id_t	start;	/* Min routine number */
	mach_msg_id_t	end;	/* Max routine number + 1 */
	unsigned int	maxsize;	/* Max msg size */
	vm_address_t	base_addr;	/* Base ddress */
	struct routine_descriptor	/*Array of routine descriptors */
		routine[15];
	struct routine_arg_descriptor	/*Array of arg descriptors */
		arg_descriptor[21];
} memory_object_subsystem;


#ifndef subsystem_to_name_map_memory_object
#define subsystem_to_name_map_memory_object \
    { "memory_object_init", 2200 },\
    { "memory_object_terminate", 2201 },\
    { "memory_object_data_request", 2203 },\
    { "memory_object_data_unlock", 2204 },\
    { "memory_object_lock_completed", 2206 },\
    { "memory_object_supply_completed", 2207 },\
    { "memory_object_data_return", 2208 },\
    { "memory_object_synchronize", 2210 },\
    { "memory_object_change_completed", 2213 },\
    { "memory_object_discard_request", 2214 }
#endif

#endif	 /* _memory_object_server_ */
