#ifndef	_mach_norma_user_
#define	_mach_norma_user_

/* Module mach_norma */

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

#ifndef	mach_norma_MSG_COUNT
#define	mach_norma_MSG_COUNT	13
#endif	/* mach_norma_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* Routine task_set_child_node */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t task_set_child_node
#if	defined(LINTLIBRARY)
    (target_task, child_node)
	mach_port_t target_task;
	int child_node;
{ return task_set_child_node(target_task, child_node); }
#else
(
	mach_port_t target_task,
	int child_node
);
#endif	/* defined(LINTLIBRARY) */

/* Routine norma_node_self */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t norma_node_self
#if	defined(LINTLIBRARY)
    (host, node)
	mach_port_t host;
	int *node;
{ return norma_node_self(host, node); }
#else
(
	mach_port_t host,
	int *node
);
#endif	/* defined(LINTLIBRARY) */

/* Routine norma_task_clone */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t norma_task_clone
#if	defined(LINTLIBRARY)
    (target_task, inherit_memory, child_node, child_task)
	mach_port_t target_task;
	boolean_t inherit_memory;
	int child_node;
	mach_port_t *child_task;
{ return norma_task_clone(target_task, inherit_memory, child_node, child_task); }
#else
(
	mach_port_t target_task,
	boolean_t inherit_memory,
	int child_node,
	mach_port_t *child_task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine norma_task_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t norma_task_create
#if	defined(LINTLIBRARY)
    (target_task, inherit_memory, child_node, child_task)
	mach_port_t target_task;
	boolean_t inherit_memory;
	int child_node;
	mach_port_t *child_task;
{ return norma_task_create(target_task, inherit_memory, child_node, child_task); }
#else
(
	mach_port_t target_task,
	boolean_t inherit_memory,
	int child_node,
	mach_port_t *child_task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine norma_get_special_port */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t norma_get_special_port
#if	defined(LINTLIBRARY)
    (host_priv, node, which, port)
	mach_port_t host_priv;
	int node;
	int which;
	mach_port_t *port;
{ return norma_get_special_port(host_priv, node, which, port); }
#else
(
	mach_port_t host_priv,
	int node,
	int which,
	mach_port_t *port
);
#endif	/* defined(LINTLIBRARY) */

/* Routine norma_set_special_port */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t norma_set_special_port
#if	defined(LINTLIBRARY)
    (host_priv, which, port)
	mach_port_t host_priv;
	int which;
	mach_port_t port;
{ return norma_set_special_port(host_priv, which, port); }
#else
(
	mach_port_t host_priv,
	int which,
	mach_port_t port
);
#endif	/* defined(LINTLIBRARY) */

/* Routine norma_task_teleport */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t norma_task_teleport
#if	defined(LINTLIBRARY)
    (target_task, inherit_memory, child_node, child_task)
	mach_port_t target_task;
	boolean_t inherit_memory;
	int child_node;
	mach_port_t *child_task;
{ return norma_task_teleport(target_task, inherit_memory, child_node, child_task); }
#else
(
	mach_port_t target_task,
	boolean_t inherit_memory,
	int child_node,
	mach_port_t *child_task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine norma_port_location_hint */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t norma_port_location_hint
#if	defined(LINTLIBRARY)
    (task, port, node)
	mach_port_t task;
	mach_port_t port;
	int *node;
{ return norma_port_location_hint(task, port, node); }
#else
(
	mach_port_t task,
	mach_port_t port,
	int *node
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_mach_norma
#define subsystem_to_name_map_mach_norma \
    { "task_set_child_node", 555001 },\
    { "norma_node_self", 555002 },\
    { "norma_task_clone", 555005 },\
    { "norma_task_create", 555006 },\
    { "norma_get_special_port", 555007 },\
    { "norma_set_special_port", 555008 },\
    { "norma_task_teleport", 555009 },\
    { "norma_port_location_hint", 555012 }
#endif

#endif	 /* _mach_norma_user_ */
