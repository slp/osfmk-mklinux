#ifndef	_service_server_
#define	_service_server_

/* Module service */

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

#ifndef	service_MSG_COUNT
#define	service_MSG_COUNT	3
#endif	/* service_MSG_COUNT */

#include <mach/std_types.h>

/* Routine service_checkin */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_service_checkin
#if	defined(LINTLIBRARY)
    (service_request, service_desired, service_granted)
	mach_port_t service_request;
	mach_port_t service_desired;
	mach_port_t *service_granted;
{ return do_service_checkin(service_request, service_desired, service_granted); }
#else
(
	mach_port_t service_request,
	mach_port_t service_desired,
	mach_port_t *service_granted
);
#endif	/* defined(LINTLIBRARY) */

extern boolean_t service_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);

extern mig_routine_t service_server_routine(
		mach_msg_header_t *InHeadP);


/* Description of this subsystem, for use in direct RPC */
extern struct do_service_subsystem {
	struct subsystem *	subsystem;	/* Reserved for system use */
	mach_msg_id_t	start;	/* Min routine number */
	mach_msg_id_t	end;	/* Max routine number + 1 */
	unsigned int	maxsize;	/* Max msg size */
	vm_address_t	base_addr;	/* Base ddress */
	struct routine_descriptor	/*Array of routine descriptors */
		routine[3];
	struct routine_arg_descriptor	/*Array of arg descriptors */
		arg_descriptor[3];
} do_service_subsystem;


#ifndef subsystem_to_name_map_service
#define subsystem_to_name_map_service \
    { "service_checkin", 401 }
#endif

#endif	 /* _service_server_ */
