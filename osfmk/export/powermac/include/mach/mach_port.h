#ifndef	_mach_port_user_
#define	_mach_port_user_

/* Module mach_port */

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

#ifndef	mach_port_MSG_COUNT
#define	mach_port_MSG_COUNT	31
#endif	/* mach_port_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* Routine mach_port_names */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_names
#if	defined(LINTLIBRARY)
    (task, names, namesCnt, types, typesCnt)
	mach_port_t task;
	mach_port_array_t *names;
	mach_msg_type_number_t *namesCnt;
	mach_port_type_array_t *types;
	mach_msg_type_number_t *typesCnt;
{ return mach_port_names(task, names, namesCnt, types, typesCnt); }
#else
(
	mach_port_t task,
	mach_port_array_t *names,
	mach_msg_type_number_t *namesCnt,
	mach_port_type_array_t *types,
	mach_msg_type_number_t *typesCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_type */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_type
#if	defined(LINTLIBRARY)
    (task, name, ptype)
	mach_port_t task;
	mach_port_t name;
	mach_port_type_t *ptype;
{ return mach_port_type(task, name, ptype); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_type_t *ptype
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_rename */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_rename
#if	defined(LINTLIBRARY)
    (task, old_name, new_name)
	mach_port_t task;
	mach_port_t old_name;
	mach_port_t new_name;
{ return mach_port_rename(task, old_name, new_name); }
#else
(
	mach_port_t task,
	mach_port_t old_name,
	mach_port_t new_name
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_allocate_name */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_allocate_name
#if	defined(LINTLIBRARY)
    (task, right, name)
	mach_port_t task;
	mach_port_right_t right;
	mach_port_t name;
{ return mach_port_allocate_name(task, right, name); }
#else
(
	mach_port_t task,
	mach_port_right_t right,
	mach_port_t name
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_allocate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_allocate
#if	defined(LINTLIBRARY)
    (task, right, name)
	mach_port_t task;
	mach_port_right_t right;
	mach_port_t *name;
{ return mach_port_allocate(task, right, name); }
#else
(
	mach_port_t task,
	mach_port_right_t right,
	mach_port_t *name
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_destroy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_destroy
#if	defined(LINTLIBRARY)
    (task, name)
	mach_port_t task;
	mach_port_t name;
{ return mach_port_destroy(task, name); }
#else
(
	mach_port_t task,
	mach_port_t name
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_deallocate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_deallocate
#if	defined(LINTLIBRARY)
    (task, name)
	mach_port_t task;
	mach_port_t name;
{ return mach_port_deallocate(task, name); }
#else
(
	mach_port_t task,
	mach_port_t name
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_get_refs */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_get_refs
#if	defined(LINTLIBRARY)
    (task, name, right, refs)
	mach_port_t task;
	mach_port_t name;
	mach_port_right_t right;
	mach_port_urefs_t *refs;
{ return mach_port_get_refs(task, name, right, refs); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_right_t right,
	mach_port_urefs_t *refs
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_mod_refs */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_mod_refs
#if	defined(LINTLIBRARY)
    (task, name, right, delta)
	mach_port_t task;
	mach_port_t name;
	mach_port_right_t right;
	mach_port_delta_t delta;
{ return mach_port_mod_refs(task, name, right, delta); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_right_t right,
	mach_port_delta_t delta
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_allocate_subsystem */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_allocate_subsystem
#if	defined(LINTLIBRARY)
    (task, subsys, name)
	mach_port_t task;
	mach_port_t subsys;
	mach_port_t *name;
{ return mach_port_allocate_subsystem(task, subsys, name); }
#else
(
	mach_port_t task,
	mach_port_t subsys,
	mach_port_t *name
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_set_mscount */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_set_mscount
#if	defined(LINTLIBRARY)
    (task, name, mscount)
	mach_port_t task;
	mach_port_t name;
	mach_port_mscount_t mscount;
{ return mach_port_set_mscount(task, name, mscount); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_mscount_t mscount
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_get_set_status */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_get_set_status
#if	defined(LINTLIBRARY)
    (task, name, members, membersCnt)
	mach_port_t task;
	mach_port_t name;
	mach_port_array_t *members;
	mach_msg_type_number_t *membersCnt;
{ return mach_port_get_set_status(task, name, members, membersCnt); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_array_t *members,
	mach_msg_type_number_t *membersCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_move_member */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_move_member
#if	defined(LINTLIBRARY)
    (task, member, after)
	mach_port_t task;
	mach_port_t member;
	mach_port_t after;
{ return mach_port_move_member(task, member, after); }
#else
(
	mach_port_t task,
	mach_port_t member,
	mach_port_t after
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_request_notification */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_request_notification
#if	defined(LINTLIBRARY)
    (task, name, id, sync, notify, notifyPoly, previous)
	mach_port_t task;
	mach_port_t name;
	mach_msg_id_t id;
	mach_port_mscount_t sync;
	mach_port_t notify;
	mach_msg_type_name_t notifyPoly;
	mach_port_t *previous;
{ return mach_port_request_notification(task, name, id, sync, notify, notifyPoly, previous); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_msg_id_t id,
	mach_port_mscount_t sync,
	mach_port_t notify,
	mach_msg_type_name_t notifyPoly,
	mach_port_t *previous
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_insert_right */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_insert_right
#if	defined(LINTLIBRARY)
    (task, name, poly, polyPoly)
	mach_port_t task;
	mach_port_t name;
	mach_port_t poly;
	mach_msg_type_name_t polyPoly;
{ return mach_port_insert_right(task, name, poly, polyPoly); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_t poly,
	mach_msg_type_name_t polyPoly
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_extract_right */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_extract_right
#if	defined(LINTLIBRARY)
    (task, name, msgt_name, poly, polyPoly)
	mach_port_t task;
	mach_port_t name;
	mach_msg_type_name_t msgt_name;
	mach_port_t *poly;
	mach_msg_type_name_t *polyPoly;
{ return mach_port_extract_right(task, name, msgt_name, poly, polyPoly); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_msg_type_name_t msgt_name,
	mach_port_t *poly,
	mach_msg_type_name_t *polyPoly
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_set_seqno */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_set_seqno
#if	defined(LINTLIBRARY)
    (task, name, seqno)
	mach_port_t task;
	mach_port_t name;
	mach_port_seqno_t seqno;
{ return mach_port_set_seqno(task, name, seqno); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_seqno_t seqno
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_get_attributes */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_get_attributes
#if	defined(LINTLIBRARY)
    (task, name, flavor, port_info_out, port_info_outCnt)
	mach_port_t task;
	mach_port_t name;
	mach_port_flavor_t flavor;
	mach_port_info_t port_info_out;
	mach_msg_type_number_t *port_info_outCnt;
{ return mach_port_get_attributes(task, name, flavor, port_info_out, port_info_outCnt); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_flavor_t flavor,
	mach_port_info_t port_info_out,
	mach_msg_type_number_t *port_info_outCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_set_attributes */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_set_attributes
#if	defined(LINTLIBRARY)
    (task, name, flavor, port_info, port_infoCnt)
	mach_port_t task;
	mach_port_t name;
	mach_port_flavor_t flavor;
	mach_port_info_t port_info;
	mach_msg_type_number_t port_infoCnt;
{ return mach_port_set_attributes(task, name, flavor, port_info, port_infoCnt); }
#else
(
	mach_port_t task,
	mach_port_t name,
	mach_port_flavor_t flavor,
	mach_port_info_t port_info,
	mach_msg_type_number_t port_infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_activation_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t thread_activation_create
#if	defined(LINTLIBRARY)
    (task, name, user_stack, stack_size, new_act)
	mach_port_t task;
	mach_port_t name;
	vm_offset_t user_stack;
	vm_size_t stack_size;
	mach_port_t *new_act;
{ return thread_activation_create(task, name, user_stack, stack_size, new_act); }
#else
(
	mach_port_t task,
	mach_port_t name,
	vm_offset_t user_stack,
	vm_size_t stack_size,
	mach_port_t *new_act
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_allocate_qos */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_allocate_qos
#if	defined(LINTLIBRARY)
    (task, right, qos, name)
	mach_port_t task;
	mach_port_right_t right;
	mach_port_qos_t *qos;
	mach_port_t *name;
{ return mach_port_allocate_qos(task, right, qos, name); }
#else
(
	mach_port_t task,
	mach_port_right_t right,
	mach_port_qos_t *qos,
	mach_port_t *name
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port_allocate_full */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_port_allocate_full
#if	defined(LINTLIBRARY)
    (task, right, subs, qos, name)
	mach_port_t task;
	mach_port_right_t right;
	mach_port_t subs;
	mach_port_qos_t *qos;
	mach_port_t *name;
{ return mach_port_allocate_full(task, right, subs, qos, name); }
#else
(
	mach_port_t task,
	mach_port_right_t right,
	mach_port_t subs,
	mach_port_qos_t *qos,
	mach_port_t *name
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_mach_port
#define subsystem_to_name_map_mach_port \
    { "mach_port_names", 3200 },\
    { "mach_port_type", 3201 },\
    { "mach_port_rename", 3202 },\
    { "mach_port_allocate_name", 3203 },\
    { "mach_port_allocate", 3204 },\
    { "mach_port_destroy", 3205 },\
    { "mach_port_deallocate", 3206 },\
    { "mach_port_get_refs", 3207 },\
    { "mach_port_mod_refs", 3208 },\
    { "mach_port_allocate_subsystem", 3209 },\
    { "mach_port_set_mscount", 3211 },\
    { "mach_port_get_set_status", 3212 },\
    { "mach_port_move_member", 3213 },\
    { "mach_port_request_notification", 3214 },\
    { "mach_port_insert_right", 3215 },\
    { "mach_port_extract_right", 3216 },\
    { "mach_port_set_seqno", 3218 },\
    { "mach_port_get_attributes", 3219 },\
    { "mach_port_set_attributes", 3220 },\
    { "thread_activation_create", 3228 },\
    { "mach_port_allocate_qos", 3229 },\
    { "mach_port_allocate_full", 3230 }
#endif

#endif	 /* _mach_port_user_ */
