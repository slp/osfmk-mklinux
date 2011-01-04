#ifndef	_machid_debug_user_
#define	_machid_debug_user_

/* Module machid_debug */

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

#ifndef	machid_debug_MSG_COUNT
#define	machid_debug_MSG_COUNT	7
#endif	/* machid_debug_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <mach_debug/mach_debug_types.h>
#include <servers/machid_types.h>

/* Routine port_get_srights */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_port_get_srights
#if	defined(LINTLIBRARY)
    (server, auth, task, name, srights)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_t name;
	mach_port_rights_t *srights;
{ return machid_port_get_srights(server, auth, task, name, srights); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mach_port_t name,
	mach_port_rights_t *srights
);
#endif	/* defined(LINTLIBRARY) */

/* Routine port_space_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_port_space_info
#if	defined(LINTLIBRARY)
    (server, auth, task, info, table_info, table_infoCnt, tree_info, tree_infoCnt)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	ipc_info_space_t *info;
	ipc_info_name_array_t *table_info;
	mach_msg_type_number_t *table_infoCnt;
	ipc_info_tree_name_array_t *tree_info;
	mach_msg_type_number_t *tree_infoCnt;
{ return machid_port_space_info(server, auth, task, info, table_info, table_infoCnt, tree_info, tree_infoCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	ipc_info_space_t *info,
	ipc_info_name_array_t *table_info,
	mach_msg_type_number_t *table_infoCnt,
	ipc_info_tree_name_array_t *tree_info,
	mach_msg_type_number_t *tree_infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine port_dnrequest_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_port_dnrequest_info
#if	defined(LINTLIBRARY)
    (server, auth, task, name, total, used)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_t name;
	unsigned *total;
	unsigned *used;
{ return machid_port_dnrequest_info(server, auth, task, name, total, used); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mach_port_t name,
	unsigned *total,
	unsigned *used
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_region_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_vm_region_info
#if	defined(LINTLIBRARY)
    (server, auth, task, addr, region, objects, objectsCnt)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	vm_offset_t addr;
	vm_info_region_t *region;
	vm_info_object_array_t *objects;
	mach_msg_type_number_t *objectsCnt;
{ return machid_vm_region_info(server, auth, task, addr, region, objects, objectsCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	vm_offset_t addr,
	vm_info_region_t *region,
	vm_info_object_array_t *objects,
	mach_msg_type_number_t *objectsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_stack_usage */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_stack_usage
#if	defined(LINTLIBRARY)
    (server, auth, host, reserved, total, space, resident, maxusage, maxstack)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	vm_size_t *reserved;
	unsigned *total;
	vm_size_t *space;
	vm_size_t *resident;
	vm_size_t *maxusage;
	vm_offset_t *maxstack;
{ return machid_host_stack_usage(server, auth, host, reserved, total, space, resident, maxusage, maxstack); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	vm_size_t *reserved,
	unsigned *total,
	vm_size_t *space,
	vm_size_t *resident,
	vm_size_t *maxusage,
	vm_offset_t *maxstack
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_stack_usage */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_stack_usage
#if	defined(LINTLIBRARY)
    (server, auth, pset, total, space, resident, maxusage, maxstack)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_t pset;
	unsigned *total;
	vm_size_t *space;
	vm_size_t *resident;
	vm_size_t *maxusage;
	vm_offset_t *maxstack;
{ return machid_processor_set_stack_usage(server, auth, pset, total, space, resident, maxusage, maxstack); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_t pset,
	unsigned *total,
	vm_size_t *space,
	vm_size_t *resident,
	vm_size_t *maxusage,
	vm_offset_t *maxstack
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_zone_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_zone_info
#if	defined(LINTLIBRARY)
    (server, auth, host, names, namesCnt, info, infoCnt)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	zone_name_array_t *names;
	mach_msg_type_number_t *namesCnt;
	zone_info_array_t *info;
	mach_msg_type_number_t *infoCnt;
{ return machid_host_zone_info(server, auth, host, names, namesCnt, info, infoCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	zone_name_array_t *names,
	mach_msg_type_number_t *namesCnt,
	zone_info_array_t *info,
	mach_msg_type_number_t *infoCnt
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_machid_debug
#define subsystem_to_name_map_machid_debug \
    { "port_get_srights", 2398925 },\
    { "port_space_info", 2398926 },\
    { "port_dnrequest_info", 2398927 },\
    { "vm_region_info", 2398928 },\
    { "host_stack_usage", 2398929 },\
    { "processor_set_stack_usage", 2398930 },\
    { "host_zone_info", 2398931 }
#endif

#endif	 /* _machid_debug_user_ */
