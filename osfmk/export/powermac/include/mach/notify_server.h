#ifndef	_notify_server_
#define	_notify_server_

/* Module notify */

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

#ifndef	notify_MSG_COUNT
#define	notify_MSG_COUNT	9
#endif	/* notify_MSG_COUNT */

#include <mach/std_types.h>

/* SimpleRoutine mach_notify_port_deleted */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_mach_notify_port_deleted
#if	defined(LINTLIBRARY)
    (notify, name)
	mach_port_t notify;
	mach_port_t name;
{ return do_mach_notify_port_deleted(notify, name); }
#else
(
	mach_port_t notify,
	mach_port_t name
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine mach_notify_port_destroyed */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_mach_notify_port_destroyed
#if	defined(LINTLIBRARY)
    (notify, rights)
	mach_port_t notify;
	mach_port_t rights;
{ return do_mach_notify_port_destroyed(notify, rights); }
#else
(
	mach_port_t notify,
	mach_port_t rights
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine mach_notify_no_senders */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_mach_notify_no_senders
#if	defined(LINTLIBRARY)
    (notify, mscount)
	mach_port_t notify;
	mach_port_mscount_t mscount;
{ return do_mach_notify_no_senders(notify, mscount); }
#else
(
	mach_port_t notify,
	mach_port_mscount_t mscount
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine mach_notify_send_once */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_mach_notify_send_once
#if	defined(LINTLIBRARY)
    (notify)
	mach_port_t notify;
{ return do_mach_notify_send_once(notify); }
#else
(
	mach_port_t notify
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine mach_notify_dead_name */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t do_mach_notify_dead_name
#if	defined(LINTLIBRARY)
    (notify, name)
	mach_port_t notify;
	mach_port_t name;
{ return do_mach_notify_dead_name(notify, name); }
#else
(
	mach_port_t notify,
	mach_port_t name
);
#endif	/* defined(LINTLIBRARY) */

extern boolean_t notify_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);

extern mig_routine_t notify_server_routine(
		mach_msg_header_t *InHeadP);


/* Description of this subsystem, for use in direct RPC */
extern struct do_notify_subsystem {
	struct subsystem *	subsystem;	/* Reserved for system use */
	mach_msg_id_t	start;	/* Min routine number */
	mach_msg_id_t	end;	/* Max routine number + 1 */
	unsigned int	maxsize;	/* Max msg size */
	vm_address_t	base_addr;	/* Base ddress */
	struct routine_descriptor	/*Array of routine descriptors */
		routine[9];
	struct routine_arg_descriptor	/*Array of arg descriptors */
		arg_descriptor[6];
} do_notify_subsystem;


#ifndef subsystem_to_name_map_notify
#define subsystem_to_name_map_notify \
    { "mach_notify_port_deleted", 65 },\
    { "mach_notify_port_destroyed", 69 },\
    { "mach_notify_no_senders", 70 },\
    { "mach_notify_send_once", 71 },\
    { "mach_notify_dead_name", 72 }
#endif

#endif	 /* _notify_server_ */
