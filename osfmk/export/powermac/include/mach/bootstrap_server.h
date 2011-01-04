#ifndef	_bootstrap_server_
#define	_bootstrap_server_

/* Module bootstrap */

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

#ifndef	bootstrap_MSG_COUNT
#define	bootstrap_MSG_COUNT	6
#endif	/* bootstrap_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* Routine bootstrap_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_bootstrap_ports
#if	defined(LINTLIBRARY)
    (bootstrap, priv_host, priv_device, wired_ledger, paged_ledger, host_security)
	mach_port_t bootstrap;
	mach_port_t *priv_host;
	mach_port_t *priv_device;
	mach_port_t *wired_ledger;
	mach_port_t *paged_ledger;
	mach_port_t *host_security;
{ return do_bootstrap_ports(bootstrap, priv_host, priv_device, wired_ledger, paged_ledger, host_security); }
#else
(
	mach_port_t bootstrap,
	mach_port_t *priv_host,
	mach_port_t *priv_device,
	mach_port_t *wired_ledger,
	mach_port_t *paged_ledger,
	mach_port_t *host_security
);
#endif	/* defined(LINTLIBRARY) */

/* Routine bootstrap_arguments */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_bootstrap_arguments
#if	defined(LINTLIBRARY)
    (bootstrap, task, arguments, argumentsCnt)
	mach_port_t bootstrap;
	mach_port_t task;
	vm_offset_t *arguments;
	mach_msg_type_number_t *argumentsCnt;
{ return do_bootstrap_arguments(bootstrap, task, arguments, argumentsCnt); }
#else
(
	mach_port_t bootstrap,
	mach_port_t task,
	vm_offset_t *arguments,
	mach_msg_type_number_t *argumentsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine bootstrap_environment */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_bootstrap_environment
#if	defined(LINTLIBRARY)
    (bootstrap, task, environment, environmentCnt)
	mach_port_t bootstrap;
	mach_port_t task;
	vm_offset_t *environment;
	mach_msg_type_number_t *environmentCnt;
{ return do_bootstrap_environment(bootstrap, task, environment, environmentCnt); }
#else
(
	mach_port_t bootstrap,
	mach_port_t task,
	vm_offset_t *environment,
	mach_msg_type_number_t *environmentCnt
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine bootstrap_completed */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_bootstrap_completed
#if	defined(LINTLIBRARY)
    (bootstrap, task)
	mach_port_t bootstrap;
	mach_port_t task;
{ return do_bootstrap_completed(bootstrap, task); }
#else
(
	mach_port_t bootstrap,
	mach_port_t task
);
#endif	/* defined(LINTLIBRARY) */

extern boolean_t bootstrap_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);

extern mig_routine_t bootstrap_server_routine(
		mach_msg_header_t *InHeadP);


/* Description of this subsystem, for use in direct RPC */
extern struct do_bootstrap_subsystem {
	struct subsystem *	subsystem;	/* Reserved for system use */
	mach_msg_id_t	start;	/* Min routine number */
	mach_msg_id_t	end;	/* Max routine number + 1 */
	unsigned int	maxsize;	/* Max msg size */
	vm_address_t	base_addr;	/* Base ddress */
	struct routine_descriptor	/*Array of routine descriptors */
		routine[6];
	struct routine_arg_descriptor	/*Array of arg descriptors */
		arg_descriptor[14];
} do_bootstrap_subsystem;


#ifndef subsystem_to_name_map_bootstrap
#define subsystem_to_name_map_bootstrap \
    { "bootstrap_ports", 1000001 },\
    { "bootstrap_arguments", 1000002 },\
    { "bootstrap_environment", 1000003 },\
    { "bootstrap_completed", 1000004 }
#endif

#endif	 /* _bootstrap_server_ */
