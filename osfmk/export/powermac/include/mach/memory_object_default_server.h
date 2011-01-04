#ifndef	_memory_object_default_server_
#define	_memory_object_default_server_

/* Module memory_object_default */

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

#ifndef	memory_object_default_MSG_COUNT
#define	memory_object_default_MSG_COUNT	3
#endif	/* memory_object_default_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* SimpleRoutine memory_object_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_create
#if	defined(LINTLIBRARY)
    (old_memory_object, new_memory_object, new_object_size, new_control_port, new_page_size)
	mach_port_t old_memory_object;
	mach_port_t new_memory_object;
	vm_size_t new_object_size;
	mach_port_t new_control_port;
	vm_size_t new_page_size;
{ return memory_object_create(old_memory_object, new_memory_object, new_object_size, new_control_port, new_page_size); }
#else
(
	mach_port_t old_memory_object,
	mach_port_t new_memory_object,
	vm_size_t new_object_size,
	mach_port_t new_control_port,
	vm_size_t new_page_size
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine memory_object_data_initialize */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t memory_object_data_initialize
#if	defined(LINTLIBRARY)
    (memory_object, memory_control_port, offset, data, dataCnt)
	mach_port_t memory_object;
	mach_port_t memory_control_port;
	vm_offset_t offset;
	vm_offset_t data;
	mach_msg_type_number_t dataCnt;
{ return memory_object_data_initialize(memory_object, memory_control_port, offset, data, dataCnt); }
#else
(
	mach_port_t memory_object,
	mach_port_t memory_control_port,
	vm_offset_t offset,
	vm_offset_t data,
	mach_msg_type_number_t dataCnt
);
#endif	/* defined(LINTLIBRARY) */

extern boolean_t memory_object_default_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);

extern mig_routine_t memory_object_default_server_routine(
		mach_msg_header_t *InHeadP);


/* Description of this subsystem, for use in direct RPC */
extern struct memory_object_default_subsystem {
	struct subsystem *	subsystem;	/* Reserved for system use */
	mach_msg_id_t	start;	/* Min routine number */
	mach_msg_id_t	end;	/* Max routine number + 1 */
	unsigned int	maxsize;	/* Max msg size */
	vm_address_t	base_addr;	/* Base ddress */
	struct routine_descriptor	/*Array of routine descriptors */
		routine[3];
	struct routine_arg_descriptor	/*Array of arg descriptors */
		arg_descriptor[6];
} memory_object_default_subsystem;


#ifndef subsystem_to_name_map_memory_object_default
#define subsystem_to_name_map_memory_object_default \
    { "memory_object_create", 2250 },\
    { "memory_object_data_initialize", 2251 }
#endif

#endif	 /* _memory_object_default_server_ */
