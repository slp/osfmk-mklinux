#ifndef	_clock_user_
#define	_clock_user_

/* Module clock */

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

#ifndef	clock_MSG_COUNT
#define	clock_MSG_COUNT	8
#endif	/* clock_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <mach/mach_types.h>

/* Routine host_get_clock_service */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_get_clock_service
#if	defined(LINTLIBRARY)
    (host, clock_id, clock)
	mach_port_t host;
	clock_id_t clock_id;
	mach_port_t *clock;
{ return host_get_clock_service(host, clock_id, clock); }
#else
(
	mach_port_t host,
	clock_id_t clock_id,
	mach_port_t *clock
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_get_clock_control */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_get_clock_control
#if	defined(LINTLIBRARY)
    (host_priv, clock_id, clock_ctrl)
	mach_port_t host_priv;
	clock_id_t clock_id;
	mach_port_t *clock_ctrl;
{ return host_get_clock_control(host_priv, clock_id, clock_ctrl); }
#else
(
	mach_port_t host_priv,
	clock_id_t clock_id,
	mach_port_t *clock_ctrl
);
#endif	/* defined(LINTLIBRARY) */

/* Routine clock_get_time */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t clock_get_time
#if	defined(LINTLIBRARY)
    (clock, cur_time)
	mach_port_t clock;
	tvalspec_t *cur_time;
{ return clock_get_time(clock, cur_time); }
#else
(
	mach_port_t clock,
	tvalspec_t *cur_time
);
#endif	/* defined(LINTLIBRARY) */

/* Routine clock_get_attributes */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t clock_get_attributes
#if	defined(LINTLIBRARY)
    (clock, flavor, clock_attr, clock_attrCnt)
	mach_port_t clock;
	clock_flavor_t flavor;
	clock_attr_t clock_attr;
	mach_msg_type_number_t *clock_attrCnt;
{ return clock_get_attributes(clock, flavor, clock_attr, clock_attrCnt); }
#else
(
	mach_port_t clock,
	clock_flavor_t flavor,
	clock_attr_t clock_attr,
	mach_msg_type_number_t *clock_attrCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine clock_set_time */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t clock_set_time
#if	defined(LINTLIBRARY)
    (clock_ctrl, new_time)
	mach_port_t clock_ctrl;
	tvalspec_t new_time;
{ return clock_set_time(clock_ctrl, new_time); }
#else
(
	mach_port_t clock_ctrl,
	tvalspec_t new_time
);
#endif	/* defined(LINTLIBRARY) */

/* Routine clock_set_attributes */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t clock_set_attributes
#if	defined(LINTLIBRARY)
    (clock_ctrl, flavor, clock_attr, clock_attrCnt)
	mach_port_t clock_ctrl;
	clock_flavor_t flavor;
	clock_attr_t clock_attr;
	mach_msg_type_number_t clock_attrCnt;
{ return clock_set_attributes(clock_ctrl, flavor, clock_attr, clock_attrCnt); }
#else
(
	mach_port_t clock_ctrl,
	clock_flavor_t flavor,
	clock_attr_t clock_attr,
	mach_msg_type_number_t clock_attrCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine clock_map_time */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t clock_map_time
#if	defined(LINTLIBRARY)
    (clock, pager)
	mach_port_t clock;
	mach_port_t *pager;
{ return clock_map_time(clock, pager); }
#else
(
	mach_port_t clock,
	mach_port_t *pager
);
#endif	/* defined(LINTLIBRARY) */

/* Routine clock_alarm */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t clock_alarm
#if	defined(LINTLIBRARY)
    (clock, alarm_type, alarm_time, alarm_port)
	mach_port_t clock;
	alarm_type_t alarm_type;
	tvalspec_t alarm_time;
	mach_port_t alarm_port;
{ return clock_alarm(clock, alarm_type, alarm_time, alarm_port); }
#else
(
	mach_port_t clock,
	alarm_type_t alarm_type,
	tvalspec_t alarm_time,
	mach_port_t alarm_port
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_clock
#define subsystem_to_name_map_clock \
    { "host_get_clock_service", 3125000 },\
    { "host_get_clock_control", 3125001 },\
    { "clock_get_time", 3125002 },\
    { "clock_get_attributes", 3125003 },\
    { "clock_set_time", 3125004 },\
    { "clock_set_attributes", 3125005 },\
    { "clock_map_time", 3125006 },\
    { "clock_alarm", 3125007 }
#endif

#endif	 /* _clock_user_ */
