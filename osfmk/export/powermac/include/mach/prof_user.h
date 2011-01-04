#ifndef	_prof_user_
#define	_prof_user_

/* Module prof */

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

#ifndef	prof_MSG_COUNT
#define	prof_MSG_COUNT	2
#endif	/* prof_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* SimpleRoutine samples */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t send_samples
#if	defined(LINTLIBRARY)
    (reply_port, samples, samplesCnt)
	mach_port_t reply_port;
	sample_array_t samples;
	mach_msg_type_number_t samplesCnt;
{ return send_samples(reply_port, samples, samplesCnt); }
#else
(
	mach_port_t reply_port,
	sample_array_t samples,
	mach_msg_type_number_t samplesCnt
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine notices */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t send_notices
#if	defined(LINTLIBRARY)
    (reply_port, samples, samplesCnt, options)
	mach_port_t reply_port;
	sample_array_t samples;
	mach_msg_type_number_t samplesCnt;
	mach_msg_options_t options;
{ return send_notices(reply_port, samples, samplesCnt, options); }
#else
(
	mach_port_t reply_port,
	sample_array_t samples,
	mach_msg_type_number_t samplesCnt,
	mach_msg_options_t options
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_prof
#define subsystem_to_name_map_prof \
    { "samples", 2450 },\
    { "notices", 2451 }
#endif

#endif	 /* _prof_user_ */
