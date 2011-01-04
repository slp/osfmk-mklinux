#ifndef	_bootstrap_user_
#define	_bootstrap_user_

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
kern_return_t bootstrap_ports
#if	defined(LINTLIBRARY)
    (bootstrap, priv_host, priv_device, wired_ledger, paged_ledger, host_security)
	mach_port_t bootstrap;
	mach_port_t *priv_host;
	mach_port_t *priv_device;
	mach_port_t *wired_ledger;
	mach_port_t *paged_ledger;
	mach_port_t *host_security;
{ return bootstrap_ports(bootstrap, priv_host, priv_device, wired_ledger, paged_ledger, host_security); }
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
kern_return_t bootstrap_arguments
#if	defined(LINTLIBRARY)
    (bootstrap, task, arguments, argumentsCnt)
	mach_port_t bootstrap;
	mach_port_t task;
	vm_offset_t *arguments;
	mach_msg_type_number_t *argumentsCnt;
{ return bootstrap_arguments(bootstrap, task, arguments, argumentsCnt); }
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
kern_return_t bootstrap_environment
#if	defined(LINTLIBRARY)
    (bootstrap, task, environment, environmentCnt)
	mach_port_t bootstrap;
	mach_port_t task;
	vm_offset_t *environment;
	mach_msg_type_number_t *environmentCnt;
{ return bootstrap_environment(bootstrap, task, environment, environmentCnt); }
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
kern_return_t bootstrap_completed
#if	defined(LINTLIBRARY)
    (bootstrap, task)
	mach_port_t bootstrap;
	mach_port_t task;
{ return bootstrap_completed(bootstrap, task); }
#else
(
	mach_port_t bootstrap,
	mach_port_t task
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_bootstrap
#define subsystem_to_name_map_bootstrap \
    { "bootstrap_ports", 1000001 },\
    { "bootstrap_arguments", 1000002 },\
    { "bootstrap_environment", 1000003 },\
    { "bootstrap_completed", 1000004 }
#endif

#endif	 /* _bootstrap_user_ */
