#ifndef	_mach_debug_user_
#define	_mach_debug_user_

/* Module mach_debug */

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

#ifndef	mach_debug_MSG_COUNT
#define	mach_debug_MSG_COUNT	19
#endif	/* mach_debug_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <mach_debug/mach_debug_types.h>

/* Routine host_zone_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_zone_info
#if	defined(LINTLIBRARY)
    (host, names, namesCnt, info, infoCnt)
	mach_port_t host;
	zone_name_array_t *names;
	mach_msg_type_number_t *namesCnt;
	zone_info_array_t *info;
	mach_msg_type_number_t *infoCnt;
{ return host_zone_info(host, names, namesCnt, info, infoCnt); }
#else
(
	mach_port_t host,
	zone_name_array_t *names,
	mach_msg_type_number_t *namesCnt,
	zone_info_array_t *info,
	mach_msg_type_number_t *infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_get_srights */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_get_srights
#if	defined(LINTLIBRARY)
    (task, name, srights)
	mach_port_t task;
	mach_port_t name;
	mach_port_rights_t *srights;
{ return mach_port_get_srights(task, name, srights); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_rights_t *srights
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_ipc_hash_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_ipc_hash_info
#if	defined(LINTLIBRARY)
    (host, info, infoCnt)
	mach_port_t host;
	hash_info_bucket_array_t *info;
	mach_msg_type_number_t *infoCnt;
{ return host_ipc_hash_info(host, info, infoCnt); }
#else
(
	mach_port_t host,
	hash_info_bucket_array_t *info,
	mach_msg_type_number_t *infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_space_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_space_info
#if	defined(LINTLIBRARY)
    (task, info, table_info, table_infoCnt, tree_info, tree_infoCnt)
	mach_port_t task;
	ipc_info_space_t *info;
	ipc_info_name_array_t *table_info;
	mach_msg_type_number_t *table_infoCnt;
	ipc_info_tree_name_array_t *tree_info;
	mach_msg_type_number_t *tree_infoCnt;
{ return mach_port_space_info(task, info, table_info, table_infoCnt, tree_info, tree_infoCnt); }
#else
(
	mach_port_t task,
	ipc_info_space_t *info,
	ipc_info_name_array_t *table_info,
	mach_msg_type_number_t *table_infoCnt,
	ipc_info_tree_name_array_t *tree_info,
	mach_msg_type_number_t *tree_infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_dnrequest_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_dnrequest_info
#if	defined(LINTLIBRARY)
    (task, name, total, used)
	mach_port_t task;
	mach_port_t name;
	unsigned *total;
	unsigned *used;
{ return mach_port_dnrequest_info(task, name, total, used); }
#else
(
	mach_port_t task,
	mach_port_t name,
	unsigned *total,
	unsigned *used
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_vm_region_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_vm_region_info
#if	defined(LINTLIBRARY)
    (task, address, region, objects, objectsCnt)
	mach_port_t task;
	vm_address_t address;
	vm_info_region_t *region;
	vm_info_object_array_t *objects;
	mach_msg_type_number_t *objectsCnt;
{ return mach_vm_region_info(task, address, region, objects, objectsCnt); }
#else
(
	mach_port_t task,
	vm_address_t address,
	vm_info_region_t *region,
	vm_info_object_array_t *objects,
	mach_msg_type_number_t *objectsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_mapped_pages_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t vm_mapped_pages_info
#if	defined(LINTLIBRARY)
    (task, pages, pagesCnt)
	mach_port_t task;
	page_address_array_t *pages;
	mach_msg_type_number_t *pagesCnt;
{ return vm_mapped_pages_info(task, pages, pagesCnt); }
#else
(
	mach_port_t task,
	page_address_array_t *pages,
	mach_msg_type_number_t *pagesCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_stack_usage */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_stack_usage
#if	defined(LINTLIBRARY)
    (host, reserved, total, space, resident, maxusage, maxstack)
	mach_port_t host;
	vm_size_t *reserved;
	unsigned *total;
	vm_size_t *space;
	vm_size_t *resident;
	vm_size_t *maxusage;
	vm_offset_t *maxstack;
{ return host_stack_usage(host, reserved, total, space, resident, maxusage, maxstack); }
#else
(
	mach_port_t host,
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
kern_return_t processor_set_stack_usage
#if	defined(LINTLIBRARY)
    (pset, total, space, resident, maxusage, maxstack)
	mach_port_t pset;
	unsigned *total;
	vm_size_t *space;
	vm_size_t *resident;
	vm_size_t *maxusage;
	vm_offset_t *maxstack;
{ return processor_set_stack_usage(pset, total, space, resident, maxusage, maxstack); }
#else
(
	mach_port_t pset,
	unsigned *total,
	vm_size_t *space,
	vm_size_t *resident,
	vm_size_t *maxusage,
	vm_offset_t *maxstack
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_virtual_physical_table_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_virtual_physical_table_info
#if	defined(LINTLIBRARY)
    (host, info, infoCnt)
	mach_port_t host;
	hash_info_bucket_array_t *info;
	mach_msg_type_number_t *infoCnt;
{ return host_virtual_physical_table_info(host, info, infoCnt); }
#else
(
	mach_port_t host,
	hash_info_bucket_array_t *info,
	mach_msg_type_number_t *infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_load_symbol_table */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_load_symbol_table
#if	defined(LINTLIBRARY)
    (host, task, name, symtab, symtabCnt)
	mach_port_t host;
	mach_port_t task;
	symtab_name_t name;
	vm_offset_t symtab;
	mach_msg_type_number_t symtabCnt;
{ return host_load_symbol_table(host, task, name, symtab, symtabCnt); }
#else
(
	mach_port_t host,
	mach_port_t task,
	symtab_name_t name,
	vm_offset_t symtab,
	mach_msg_type_number_t symtabCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_kernel_object */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_kernel_object
#if	defined(LINTLIBRARY)
    (task, name, object_type, object_addr)
	mach_port_t task;
	mach_port_t name;
	unsigned *object_type;
	vm_offset_t *object_addr;
{ return mach_port_kernel_object(task, name, object_type, object_addr); }
#else
(
	mach_port_t task,
	mach_port_t name,
	unsigned *object_type,
	vm_offset_t *object_addr
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_mach_debug
#define subsystem_to_name_map_mach_debug \
    { "host_zone_info", 3005 },\
    { "mach_port_get_srights", 3007 },\
    { "host_ipc_hash_info", 3008 },\
    { "mach_port_space_info", 3010 },\
    { "mach_port_dnrequest_info", 3011 },\
    { "mach_vm_region_info", 3012 },\
    { "vm_mapped_pages_info", 3013 },\
    { "host_stack_usage", 3014 },\
    { "processor_set_stack_usage", 3015 },\
    { "host_virtual_physical_table_info", 3016 },\
    { "host_load_symbol_table", 3017 },\
    { "mach_port_kernel_object", 3018 }
#endif

#endif	 /* _mach_debug_user_ */
