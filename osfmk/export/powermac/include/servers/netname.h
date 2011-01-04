#ifndef	_netname_user_
#define	_netname_user_

/* Module netname */

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

#ifndef	netname_MSG_COUNT
#define	netname_MSG_COUNT	7
#endif	/* netname_MSG_COUNT */

#include <mach/std_types.h>
#include <servers/netname_defs.h>

/* Routine netname_check_in */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t netname_check_in
#if	defined(LINTLIBRARY)
    (server_port, port_name, signature, port_id)
	mach_port_t server_port;
	netname_name_t port_name;
	mach_port_t signature;
	mach_port_t port_id;
{ return netname_check_in(server_port, port_name, signature, port_id); }
#else
(
	mach_port_t server_port,
	netname_name_t port_name,
	mach_port_t signature,
	mach_port_t port_id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine netname_look_up */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t netname_look_up
#if	defined(LINTLIBRARY)
    (server_port, host_name, port_name, port_id)
	mach_port_t server_port;
	netname_name_t host_name;
	netname_name_t port_name;
	mach_port_t *port_id;
{ return netname_look_up(server_port, host_name, port_name, port_id); }
#else
(
	mach_port_t server_port,
	netname_name_t host_name,
	netname_name_t port_name,
	mach_port_t *port_id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine netname_check_out */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t netname_check_out
#if	defined(LINTLIBRARY)
    (server_port, port_name, signature)
	mach_port_t server_port;
	netname_name_t port_name;
	mach_port_t signature;
{ return netname_check_out(server_port, port_name, signature); }
#else
(
	mach_port_t server_port,
	netname_name_t port_name,
	mach_port_t signature
);
#endif	/* defined(LINTLIBRARY) */

/* Routine netname_version */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t netname_version
#if	defined(LINTLIBRARY)
    (server_port, version)
	mach_port_t server_port;
	netname_name_t version;
{ return netname_version(server_port, version); }
#else
(
	mach_port_t server_port,
	netname_name_t version
);
#endif	/* defined(LINTLIBRARY) */

/* Routine netname_register_send_right */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t netname_register_send_right
#if	defined(LINTLIBRARY)
    (server_port, port_name, signature, port_id)
	mach_port_t server_port;
	netname_name_t port_name;
	mach_port_t signature;
	mach_port_t port_id;
{ return netname_register_send_right(server_port, port_name, signature, port_id); }
#else
(
	mach_port_t server_port,
	netname_name_t port_name,
	mach_port_t signature,
	mach_port_t port_id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine netname_debug_on */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t netname_debug_on
#if	defined(LINTLIBRARY)
    (server_port)
	mach_port_t server_port;
{ return netname_debug_on(server_port); }
#else
(
	mach_port_t server_port
);
#endif	/* defined(LINTLIBRARY) */

/* Routine netname_debug_off */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t netname_debug_off
#if	defined(LINTLIBRARY)
    (server_port)
	mach_port_t server_port;
{ return netname_debug_off(server_port); }
#else
(
	mach_port_t server_port
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_netname
#define subsystem_to_name_map_netname \
    { "netname_check_in", 1040 },\
    { "netname_look_up", 1041 },\
    { "netname_check_out", 1042 },\
    { "netname_version", 1043 },\
    { "netname_register_send_right", 1044 },\
    { "netname_debug_on", 1045 },\
    { "netname_debug_off", 1046 }
#endif

#endif	 /* _netname_user_ */
