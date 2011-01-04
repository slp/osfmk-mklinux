#ifndef	_ledger_user_
#define	_ledger_user_

/* Module ledger */

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

#ifndef	ledger_MSG_COUNT
#define	ledger_MSG_COUNT	6
#endif	/* ledger_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>

/* Routine ledger_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ledger_create
#if	defined(LINTLIBRARY)
    (parent_ledger, ledger_ledger, new_ledger, transfer)
	mach_port_t parent_ledger;
	mach_port_t ledger_ledger;
	mach_port_t *new_ledger;
	ledger_item_t transfer;
{ return ledger_create(parent_ledger, ledger_ledger, new_ledger, transfer); }
#else
(
	mach_port_t parent_ledger,
	mach_port_t ledger_ledger,
	mach_port_t *new_ledger,
	ledger_item_t transfer
);
#endif	/* defined(LINTLIBRARY) */

/* Routine ledger_get_remote */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ledger_get_remote
#if	defined(LINTLIBRARY)
    (ledger, host, service_ledger)
	mach_port_t ledger;
	mach_port_t host;
	mach_port_t *service_ledger;
{ return ledger_get_remote(ledger, host, service_ledger); }
#else
(
	mach_port_t ledger,
	mach_port_t host,
	mach_port_t *service_ledger
);
#endif	/* defined(LINTLIBRARY) */

/* Routine ledger_read */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ledger_read
#if	defined(LINTLIBRARY)
    (ledger, balance, limit)
	mach_port_t ledger;
	ledger_item_t *balance;
	ledger_item_t *limit;
{ return ledger_read(ledger, balance, limit); }
#else
(
	mach_port_t ledger,
	ledger_item_t *balance,
	ledger_item_t *limit
);
#endif	/* defined(LINTLIBRARY) */

/* Routine ledger_set_remote */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ledger_set_remote
#if	defined(LINTLIBRARY)
    (ledger, service_ledger)
	mach_port_t ledger;
	mach_port_t service_ledger;
{ return ledger_set_remote(ledger, service_ledger); }
#else
(
	mach_port_t ledger,
	mach_port_t service_ledger
);
#endif	/* defined(LINTLIBRARY) */

/* Routine ledger_terminate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ledger_terminate
#if	defined(LINTLIBRARY)
    (ledger)
	mach_port_t ledger;
{ return ledger_terminate(ledger); }
#else
(
	mach_port_t ledger
);
#endif	/* defined(LINTLIBRARY) */

/* Routine ledger_transfer */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ledger_transfer
#if	defined(LINTLIBRARY)
    (parent_ledger, child_ledger, transfer)
	mach_port_t parent_ledger;
	mach_port_t child_ledger;
	ledger_item_t transfer;
{ return ledger_transfer(parent_ledger, child_ledger, transfer); }
#else
(
	mach_port_t parent_ledger,
	mach_port_t child_ledger,
	ledger_item_t transfer
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_ledger
#define subsystem_to_name_map_ledger \
    { "ledger_create", 5000 },\
    { "ledger_get_remote", 5001 },\
    { "ledger_read", 5002 },\
    { "ledger_set_remote", 5003 },\
    { "ledger_terminate", 5004 },\
    { "ledger_transfer", 5005 }
#endif

#endif	 /* _ledger_user_ */
