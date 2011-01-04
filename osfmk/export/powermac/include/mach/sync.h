#ifndef	_sync_user_
#define	_sync_user_

/* Module sync */

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

#ifndef	sync_MSG_COUNT
#define	sync_MSG_COUNT	15
#endif	/* sync_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <mach/mach_types.h>

/* Routine semaphore_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t semaphore_create
#if	defined(LINTLIBRARY)
    (task, semaphore, policy, value)
	mach_port_t task;
	mach_port_t *semaphore;
	int policy;
	int value;
{ return semaphore_create(task, semaphore, policy, value); }
#else
(
	mach_port_t task,
	mach_port_t *semaphore,
	int policy,
	int value
);
#endif	/* defined(LINTLIBRARY) */

/* Routine semaphore_destroy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t semaphore_destroy
#if	defined(LINTLIBRARY)
    (task, semaphore)
	mach_port_t task;
	mach_port_t semaphore;
{ return semaphore_destroy(task, semaphore); }
#else
(
	mach_port_t task,
	mach_port_t semaphore
);
#endif	/* defined(LINTLIBRARY) */

/* Routine semaphore_signal */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t semaphore_signal
#if	defined(LINTLIBRARY)
    (semaphore)
	mach_port_t semaphore;
{ return semaphore_signal(semaphore); }
#else
(
	mach_port_t semaphore
);
#endif	/* defined(LINTLIBRARY) */

/* Routine semaphore_signal_all */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t semaphore_signal_all
#if	defined(LINTLIBRARY)
    (semaphore)
	mach_port_t semaphore;
{ return semaphore_signal_all(semaphore); }
#else
(
	mach_port_t semaphore
);
#endif	/* defined(LINTLIBRARY) */

/* Routine semaphore_wait */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t semaphore_wait
#if	defined(LINTLIBRARY)
    (semaphore)
	mach_port_t semaphore;
{ return semaphore_wait(semaphore); }
#else
(
	mach_port_t semaphore
);
#endif	/* defined(LINTLIBRARY) */

/* Routine lock_set_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t lock_set_create
#if	defined(LINTLIBRARY)
    (task, new_lock_set, n_ulocks, policy)
	mach_port_t task;
	mach_port_t *new_lock_set;
	int n_ulocks;
	int policy;
{ return lock_set_create(task, new_lock_set, n_ulocks, policy); }
#else
(
	mach_port_t task,
	mach_port_t *new_lock_set,
	int n_ulocks,
	int policy
);
#endif	/* defined(LINTLIBRARY) */

/* Routine lock_set_destroy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t lock_set_destroy
#if	defined(LINTLIBRARY)
    (task, lock_set)
	mach_port_t task;
	mach_port_t lock_set;
{ return lock_set_destroy(task, lock_set); }
#else
(
	mach_port_t task,
	mach_port_t lock_set
);
#endif	/* defined(LINTLIBRARY) */

/* Routine lock_acquire */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t lock_acquire
#if	defined(LINTLIBRARY)
    (lock_set, lock_id)
	mach_port_t lock_set;
	int lock_id;
{ return lock_acquire(lock_set, lock_id); }
#else
(
	mach_port_t lock_set,
	int lock_id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine lock_release */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t lock_release
#if	defined(LINTLIBRARY)
    (lock_set, lock_id)
	mach_port_t lock_set;
	int lock_id;
{ return lock_release(lock_set, lock_id); }
#else
(
	mach_port_t lock_set,
	int lock_id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine lock_try */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t lock_try
#if	defined(LINTLIBRARY)
    (lock_set, lock_id)
	mach_port_t lock_set;
	int lock_id;
{ return lock_try(lock_set, lock_id); }
#else
(
	mach_port_t lock_set,
	int lock_id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine lock_make_stable */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t lock_make_stable
#if	defined(LINTLIBRARY)
    (lock_set, lock_id)
	mach_port_t lock_set;
	int lock_id;
{ return lock_make_stable(lock_set, lock_id); }
#else
(
	mach_port_t lock_set,
	int lock_id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine lock_handoff */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t lock_handoff
#if	defined(LINTLIBRARY)
    (lock_set, lock_id)
	mach_port_t lock_set;
	int lock_id;
{ return lock_handoff(lock_set, lock_id); }
#else
(
	mach_port_t lock_set,
	int lock_id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine lock_handoff_accept */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t lock_handoff_accept
#if	defined(LINTLIBRARY)
    (lock_set, lock_id)
	mach_port_t lock_set;
	int lock_id;
{ return lock_handoff_accept(lock_set, lock_id); }
#else
(
	mach_port_t lock_set,
	int lock_id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine semaphore_signal_thread */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t semaphore_signal_thread
#if	defined(LINTLIBRARY)
    (semaphore, thread)
	mach_port_t semaphore;
	mach_port_t thread;
{ return semaphore_signal_thread(semaphore, thread); }
#else
(
	mach_port_t semaphore,
	mach_port_t thread
);
#endif	/* defined(LINTLIBRARY) */

/* Routine semaphore_timedwait */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t semaphore_timedwait
#if	defined(LINTLIBRARY)
    (semaphore, wait_time)
	mach_port_t semaphore;
	tvalspec_t wait_time;
{ return semaphore_timedwait(semaphore, wait_time); }
#else
(
	mach_port_t semaphore,
	tvalspec_t wait_time
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_sync
#define subsystem_to_name_map_sync \
    { "semaphore_create", 617000 },\
    { "semaphore_destroy", 617001 },\
    { "semaphore_signal", 617002 },\
    { "semaphore_signal_all", 617003 },\
    { "semaphore_wait", 617004 },\
    { "lock_set_create", 617005 },\
    { "lock_set_destroy", 617006 },\
    { "lock_acquire", 617007 },\
    { "lock_release", 617008 },\
    { "lock_try", 617009 },\
    { "lock_make_stable", 617010 },\
    { "lock_handoff", 617011 },\
    { "lock_handoff_accept", 617012 },\
    { "semaphore_signal_thread", 617013 },\
    { "semaphore_timedwait", 617014 }
#endif

#endif	 /* _sync_user_ */
