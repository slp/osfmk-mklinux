#ifndef	_exc_user_
#define	_exc_user_

/* Module exc */

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

#ifndef	exc_MSG_COUNT
#define	exc_MSG_COUNT	4
#endif	/* exc_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* Routine exception_raise */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t exception_raise
#if	defined(LINTLIBRARY)
    (exception_port, thread, task, exception, code, codeCnt)
	mach_port_t exception_port;
	mach_port_t thread;
	mach_port_t task;
	exception_type_t exception;
	exception_data_t code;
	mach_msg_type_number_t codeCnt;
{ return exception_raise(exception_port, thread, task, exception, code, codeCnt); }
#else
(
	mach_port_t exception_port,
	mach_port_t thread,
	mach_port_t task,
	exception_type_t exception,
	exception_data_t code,
	mach_msg_type_number_t codeCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine exception_raise_state */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t exception_raise_state
#if	defined(LINTLIBRARY)
    (exception_port, exception, code, codeCnt, flavor, old_state, old_stateCnt, new_state, new_stateCnt)
	mach_port_t exception_port;
	exception_type_t exception;
	exception_data_t code;
	mach_msg_type_number_t codeCnt;
	int *flavor;
	thread_state_t old_state;
	mach_msg_type_number_t old_stateCnt;
	thread_state_t new_state;
	mach_msg_type_number_t *new_stateCnt;
{ return exception_raise_state(exception_port, exception, code, codeCnt, flavor, old_state, old_stateCnt, new_state, new_stateCnt); }
#else
(
	mach_port_t exception_port,
	exception_type_t exception,
	exception_data_t code,
	mach_msg_type_number_t codeCnt,
	int *flavor,
	thread_state_t old_state,
	mach_msg_type_number_t old_stateCnt,
	thread_state_t new_state,
	mach_msg_type_number_t *new_stateCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine exception_raise_state_identity */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t exception_raise_state_identity
#if	defined(LINTLIBRARY)
    (exception_port, thread, task, exception, code, codeCnt, flavor, old_state, old_stateCnt, new_state, new_stateCnt)
	mach_port_t exception_port;
	mach_port_t thread;
	mach_port_t task;
	exception_type_t exception;
	exception_data_t code;
	mach_msg_type_number_t codeCnt;
	int *flavor;
	thread_state_t old_state;
	mach_msg_type_number_t old_stateCnt;
	thread_state_t new_state;
	mach_msg_type_number_t *new_stateCnt;
{ return exception_raise_state_identity(exception_port, thread, task, exception, code, codeCnt, flavor, old_state, old_stateCnt, new_state, new_stateCnt); }
#else
(
	mach_port_t exception_port,
	mach_port_t thread,
	mach_port_t task,
	exception_type_t exception,
	exception_data_t code,
	mach_msg_type_number_t codeCnt,
	int *flavor,
	thread_state_t old_state,
	mach_msg_type_number_t old_stateCnt,
	thread_state_t new_state,
	mach_msg_type_number_t *new_stateCnt
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_exc
#define subsystem_to_name_map_exc \
    { "exception_raise", 2401 },\
    { "exception_raise_state", 2402 },\
    { "exception_raise_state_identity", 2403 }
#endif

#endif	 /* _exc_user_ */
