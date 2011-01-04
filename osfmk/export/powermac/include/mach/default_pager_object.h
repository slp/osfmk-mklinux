#ifndef	_default_pager_object_user_
#define	_default_pager_object_user_

/* Module default_pager_object */

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

#ifndef	default_pager_object_MSG_COUNT
#define	default_pager_object_MSG_COUNT	9
#endif	/* default_pager_object_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <mach/default_pager_types.h>
#include <device/device_types.h>
#include <device/net_status.h>

/* Routine default_pager_object_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t default_pager_object_create
#if	defined(LINTLIBRARY)
    (default_pager, memory_object, object_size)
	mach_port_t default_pager;
	memory_object_t *memory_object;
	vm_size_t object_size;
{ return default_pager_object_create(default_pager, memory_object, object_size); }
#else
(
	mach_port_t default_pager,
	memory_object_t *memory_object,
	vm_size_t object_size
);
#endif	/* defined(LINTLIBRARY) */

/* Routine default_pager_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t default_pager_info
#if	defined(LINTLIBRARY)
    (default_pager, info)
	mach_port_t default_pager;
	default_pager_info_t *info;
{ return default_pager_info(default_pager, info); }
#else
(
	mach_port_t default_pager,
	default_pager_info_t *info
);
#endif	/* defined(LINTLIBRARY) */

/* Routine default_pager_objects */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t default_pager_objects
#if	defined(LINTLIBRARY)
    (default_pager, objects, objectsCnt, ports, portsCnt)
	mach_port_t default_pager;
	default_pager_object_array_t *objects;
	mach_msg_type_number_t *objectsCnt;
	mach_port_array_t *ports;
	mach_msg_type_number_t *portsCnt;
{ return default_pager_objects(default_pager, objects, objectsCnt, ports, portsCnt); }
#else
(
	mach_port_t default_pager,
	default_pager_object_array_t *objects,
	mach_msg_type_number_t *objectsCnt,
	mach_port_array_t *ports,
	mach_msg_type_number_t *portsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine default_pager_object_pages */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t default_pager_object_pages
#if	defined(LINTLIBRARY)
    (default_pager, memory_object, pages, pagesCnt)
	mach_port_t default_pager;
	mach_port_t memory_object;
	default_pager_page_array_t *pages;
	mach_msg_type_number_t *pagesCnt;
{ return default_pager_object_pages(default_pager, memory_object, pages, pagesCnt); }
#else
(
	mach_port_t default_pager,
	mach_port_t memory_object,
	default_pager_page_array_t *pages,
	mach_msg_type_number_t *pagesCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine default_pager_backing_store_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t default_pager_backing_store_create
#if	defined(LINTLIBRARY)
    (default_pager, priority, clsize, backing_store)
	mach_port_t default_pager;
	int priority;
	int clsize;
	mach_port_t *backing_store;
{ return default_pager_backing_store_create(default_pager, priority, clsize, backing_store); }
#else
(
	mach_port_t default_pager,
	int priority,
	int clsize,
	mach_port_t *backing_store
);
#endif	/* defined(LINTLIBRARY) */

/* Routine default_pager_backing_store_delete */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t default_pager_backing_store_delete
#if	defined(LINTLIBRARY)
    (backing_store)
	mach_port_t backing_store;
{ return default_pager_backing_store_delete(backing_store); }
#else
(
	mach_port_t backing_store
);
#endif	/* defined(LINTLIBRARY) */

/* Routine default_pager_add_segment */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t default_pager_add_segment
#if	defined(LINTLIBRARY)
    (backing_store, device, offset, count, record_size)
	mach_port_t backing_store;
	mach_port_t device;
	recnum_t offset;
	recnum_t count;
	int record_size;
{ return default_pager_add_segment(backing_store, device, offset, count, record_size); }
#else
(
	mach_port_t backing_store,
	mach_port_t device,
	recnum_t offset,
	recnum_t count,
	int record_size
);
#endif	/* defined(LINTLIBRARY) */

/* Routine default_pager_backing_store_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t default_pager_backing_store_info
#if	defined(LINTLIBRARY)
    (backing_store, flavor, info, infoCnt)
	mach_port_t backing_store;
	backing_store_flavor_t flavor;
	backing_store_info_t info;
	mach_msg_type_number_t *infoCnt;
{ return default_pager_backing_store_info(backing_store, flavor, info, infoCnt); }
#else
(
	mach_port_t backing_store,
	backing_store_flavor_t flavor,
	backing_store_info_t info,
	mach_msg_type_number_t *infoCnt
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_default_pager_object
#define subsystem_to_name_map_default_pager_object \
    { "default_pager_object_create", 2275 },\
    { "default_pager_info", 2276 },\
    { "default_pager_objects", 2277 },\
    { "default_pager_object_pages", 2278 },\
    { "default_pager_backing_store_create", 2280 },\
    { "default_pager_backing_store_delete", 2281 },\
    { "default_pager_add_segment", 2282 },\
    { "default_pager_backing_store_info", 2283 }
#endif

#endif	 /* _default_pager_object_user_ */
