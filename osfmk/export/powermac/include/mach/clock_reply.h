#ifndef	_clock_reply_user_
#define	_clock_reply_user_

/* Module clock_reply */

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

#ifndef	clock_reply_MSG_COUNT
#define	clock_reply_MSG_COUNT	8
#endif	/* clock_reply_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>

/* SimpleRoutine clock_alarm_reply */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t clock_alarm_reply
#if	defined(LINTLIBRARY)
    (alarm_port, alarm_portPoly, alarm_code, alarm_type, alarm_time)
	mach_port_t alarm_port;
	mach_msg_type_name_t alarm_portPoly;
	kern_return_t alarm_code;
	alarm_type_t alarm_type;
	tvalspec_t alarm_time;
{ return clock_alarm_reply(alarm_port, alarm_portPoly, alarm_code, alarm_type, alarm_time); }
#else
(
	mach_port_t alarm_port,
	mach_msg_type_name_t alarm_portPoly,
	kern_return_t alarm_code,
	alarm_type_t alarm_type,
	tvalspec_t alarm_time
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_clock_reply
#define subsystem_to_name_map_clock_reply \
    { "clock_alarm_reply", 3125107 }
#endif

#endif	 /* _clock_reply_user_ */
