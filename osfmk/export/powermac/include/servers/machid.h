#ifndef	_machid_user_
#define	_machid_user_

/* Module machid */

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

#ifndef	machid_MSG_COUNT
#define	machid_MSG_COUNT	81
#endif	/* machid_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <servers/machid_types.h>

/* Routine mach_type */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_mach_type
#if	defined(LINTLIBRARY)
    (server, auth, id, mtype)
	mach_port_t server;
	mach_port_t auth;
	mach_id_t id;
	mach_type_t *mtype;
{ return machid_mach_type(server, auth, id, mtype); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mach_id_t id,
	mach_type_t *mtype
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_register */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_mach_register
#if	defined(LINTLIBRARY)
    (server, auth, port, mtype, id)
	mach_port_t server;
	mach_port_t auth;
	mach_port_t port;
	mach_type_t mtype;
	mach_id_t *id;
{ return machid_mach_register(server, auth, port, mtype, id); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mach_port_t port,
	mach_type_t mtype,
	mach_id_t *id
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_lookup */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_mach_lookup
#if	defined(LINTLIBRARY)
    (server, auth, name, atype, aname)
	mach_port_t server;
	mach_port_t auth;
	mach_id_t name;
	mach_type_t atype;
	mach_id_t *aname;
{ return machid_mach_lookup(server, auth, name, atype, aname); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mach_id_t name,
	mach_type_t atype,
	mach_id_t *aname
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_port */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_mach_port
#if	defined(LINTLIBRARY)
    (server, auth, name, port)
	mach_port_t server;
	mach_port_t auth;
	mach_id_t name;
	mach_port_t *port;
{ return machid_mach_port(server, auth, name, port); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mach_id_t name,
	mach_port_t *port
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_ports
#if	defined(LINTLIBRARY)
    (server, auth, host, phost)
	mach_port_t server;
	mach_port_t auth;
	mhost_t *host;
	mhost_priv_t *phost;
{ return machid_host_ports(server, auth, host, phost); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t *host,
	mhost_priv_t *phost
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_processor_sets */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_processor_sets
#if	defined(LINTLIBRARY)
    (server, auth, host, sets, setsCnt)
	mach_port_t server;
	mach_port_t auth;
	mhost_priv_t host;
	mprocessor_set_array_t *sets;
	mach_msg_type_number_t *setsCnt;
{ return machid_host_processor_sets(server, auth, host, sets, setsCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_priv_t host,
	mprocessor_set_array_t *sets,
	mach_msg_type_number_t *setsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_tasks */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_tasks
#if	defined(LINTLIBRARY)
    (server, auth, host, tasks, tasksCnt)
	mach_port_t server;
	mach_port_t auth;
	mhost_priv_t host;
	mtask_array_t *tasks;
	mach_msg_type_number_t *tasksCnt;
{ return machid_host_tasks(server, auth, host, tasks, tasksCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_priv_t host,
	mtask_array_t *tasks,
	mach_msg_type_number_t *tasksCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_threads */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_threads
#if	defined(LINTLIBRARY)
    (server, auth, host, threads, threadsCnt)
	mach_port_t server;
	mach_port_t auth;
	mhost_priv_t host;
	mthread_array_t *threads;
	mach_msg_type_number_t *threadsCnt;
{ return machid_host_threads(server, auth, host, threads, threadsCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_priv_t host,
	mthread_array_t *threads,
	mach_msg_type_number_t *threadsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_processors */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_processors
#if	defined(LINTLIBRARY)
    (server, auth, host, procs, procsCnt)
	mach_port_t server;
	mach_port_t auth;
	mhost_priv_t host;
	mprocessor_array_t *procs;
	mach_msg_type_number_t *procsCnt;
{ return machid_host_processors(server, auth, host, procs, procsCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_priv_t host,
	mprocessor_array_t *procs,
	mach_msg_type_number_t *procsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_threads */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_threads
#if	defined(LINTLIBRARY)
    (server, auth, pset, threads, threadsCnt)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_t pset;
	mthread_array_t *threads;
	mach_msg_type_number_t *threadsCnt;
{ return machid_processor_set_threads(server, auth, pset, threads, threadsCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_t pset,
	mthread_array_t *threads,
	mach_msg_type_number_t *threadsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_tasks */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_tasks
#if	defined(LINTLIBRARY)
    (server, auth, pset, tasks, tasksCnt)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_t pset;
	mtask_array_t *tasks;
	mach_msg_type_number_t *tasksCnt;
{ return machid_processor_set_tasks(server, auth, pset, tasks, tasksCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_t pset,
	mtask_array_t *tasks,
	mach_msg_type_number_t *tasksCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_threads */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_threads
#if	defined(LINTLIBRARY)
    (server, auth, task, threads, threadsCnt)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mthread_array_t *threads;
	mach_msg_type_number_t *threadsCnt;
{ return machid_task_threads(server, auth, task, threads, threadsCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mthread_array_t *threads,
	mach_msg_type_number_t *threadsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_basic_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_basic_info
#if	defined(LINTLIBRARY)
    (server, auth, host, info)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	host_basic_info_data_t *info;
{ return machid_host_basic_info(server, auth, host, info); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	host_basic_info_data_t *info
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_sched_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_sched_info
#if	defined(LINTLIBRARY)
    (server, auth, host, info)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	host_sched_info_data_t *info;
{ return machid_host_sched_info(server, auth, host, info); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	host_sched_info_data_t *info
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_load_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_load_info
#if	defined(LINTLIBRARY)
    (server, auth, host, info)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	host_load_info_data_t *info;
{ return machid_host_load_info(server, auth, host, info); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	host_load_info_data_t *info
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_default */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_default
#if	defined(LINTLIBRARY)
    (server, auth, host, pset)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	mprocessor_set_name_t *pset;
{ return machid_processor_set_default(server, auth, host, pset); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	mprocessor_set_name_t *pset
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_kernel_version */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_kernel_version
#if	defined(LINTLIBRARY)
    (server, auth, host, kernel_version)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	kernel_version_t kernel_version;
{ return machid_host_kernel_version(server, auth, host, kernel_version); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	kernel_version_t kernel_version
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_basic_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_basic_info
#if	defined(LINTLIBRARY)
    (server, auth, proc, host, info)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_t proc;
	mhost_t *host;
	processor_basic_info_data_t *info;
{ return machid_processor_basic_info(server, auth, proc, host, info); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_t proc,
	mhost_t *host,
	processor_basic_info_data_t *info
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_thread_times_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_thread_times_info
#if	defined(LINTLIBRARY)
    (server, auth, task, times)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	task_thread_times_info_data_t *times;
{ return machid_task_thread_times_info(server, auth, task, times); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	task_thread_times_info_data_t *times
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_terminate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_terminate
#if	defined(LINTLIBRARY)
    (server, auth, task)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
{ return machid_task_terminate(server, auth, task); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_suspend */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_suspend
#if	defined(LINTLIBRARY)
    (server, auth, task)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
{ return machid_task_suspend(server, auth, task); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_resume */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_resume
#if	defined(LINTLIBRARY)
    (server, auth, task)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
{ return machid_task_resume(server, auth, task); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_terminate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_terminate
#if	defined(LINTLIBRARY)
    (server, auth, thread)
	mach_port_t server;
	mach_port_t auth;
	mthread_t thread;
{ return machid_thread_terminate(server, auth, thread); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mthread_t thread
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_suspend */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_suspend
#if	defined(LINTLIBRARY)
    (server, auth, thread)
	mach_port_t server;
	mach_port_t auth;
	mthread_t thread;
{ return machid_thread_suspend(server, auth, thread); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mthread_t thread
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_resume */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_resume
#if	defined(LINTLIBRARY)
    (server, auth, thread)
	mach_port_t server;
	mach_port_t auth;
	mthread_t thread;
{ return machid_thread_resume(server, auth, thread); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mthread_t thread
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_abort */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_abort
#if	defined(LINTLIBRARY)
    (server, auth, thread)
	mach_port_t server;
	mach_port_t auth;
	mthread_t thread;
{ return machid_thread_abort(server, auth, thread); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mthread_t thread
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_destroy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_destroy
#if	defined(LINTLIBRARY)
    (server, auth, pset)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_t pset;
{ return machid_processor_set_destroy(server, auth, pset); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_t pset
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_start */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_start
#if	defined(LINTLIBRARY)
    (server, auth, processor)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_t processor;
{ return machid_processor_start(server, auth, processor); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_t processor
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_exit */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_exit
#if	defined(LINTLIBRARY)
    (server, auth, processor)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_t processor;
{ return machid_processor_exit(server, auth, processor); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_t processor
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_region */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_vm_region
#if	defined(LINTLIBRARY)
    (server, auth, task, addr, info)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	vm_offset_t addr;
	region_info_t *info;
{ return machid_vm_region(server, auth, task, addr, info); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	vm_offset_t addr,
	region_info_t *info
);
#endif	/* defined(LINTLIBRARY) */

/* Routine vm_read */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_vm_read
#if	defined(LINTLIBRARY)
    (server, auth, task, addr, size, data, dataCnt)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	vm_offset_t addr;
	vm_size_t size;
	vm_offset_t *data;
	mach_msg_type_number_t *dataCnt;
{ return machid_vm_read(server, auth, task, addr, size, data, dataCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	vm_offset_t addr,
	vm_size_t size,
	vm_offset_t *data,
	mach_msg_type_number_t *dataCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_max_priority */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_max_priority
#if	defined(LINTLIBRARY)
    (server, auth, pset, max_pri, change_threads)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_t pset;
	int max_pri;
	boolean_t change_threads;
{ return machid_processor_set_max_priority(server, auth, pset, max_pri, change_threads); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_t pset,
	int max_pri,
	boolean_t change_threads
);
#endif	/* defined(LINTLIBRARY) */

/* Routine port_names */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_port_names
#if	defined(LINTLIBRARY)
    (server, auth, task, names, namesCnt, types, typesCnt)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_array_t *names;
	mach_msg_type_number_t *namesCnt;
	mach_port_type_array_t *types;
	mach_msg_type_number_t *typesCnt;
{ return machid_port_names(server, auth, task, names, namesCnt, types, typesCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mach_port_array_t *names,
	mach_msg_type_number_t *namesCnt,
	mach_port_type_array_t *types,
	mach_msg_type_number_t *typesCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine port_type */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_port_type
#if	defined(LINTLIBRARY)
    (server, auth, task, name, ptype)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_t name;
	mach_port_type_t *ptype;
{ return machid_port_type(server, auth, task, name, ptype); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mach_port_t name,
	mach_port_type_t *ptype
);
#endif	/* defined(LINTLIBRARY) */

/* Routine port_get_refs */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_port_get_refs
#if	defined(LINTLIBRARY)
    (server, auth, task, name, right, refs)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_t name;
	mach_port_right_t right;
	mach_port_urefs_t *refs;
{ return machid_port_get_refs(server, auth, task, name, right, refs); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mach_port_t name,
	mach_port_right_t right,
	mach_port_urefs_t *refs
);
#endif	/* defined(LINTLIBRARY) */

/* Routine port_get_receive_status */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_port_get_receive_status
#if	defined(LINTLIBRARY)
    (server, auth, task, name, status)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_t name;
	mach_port_status_t *status;
{ return machid_port_get_receive_status(server, auth, task, name, status); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mach_port_t name,
	mach_port_status_t *status
);
#endif	/* defined(LINTLIBRARY) */

/* Routine port_get_set_status */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_port_get_set_status
#if	defined(LINTLIBRARY)
    (server, auth, task, name, members, membersCnt)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mach_port_t name;
	mach_port_array_t *members;
	mach_msg_type_number_t *membersCnt;
{ return machid_port_get_set_status(server, auth, task, name, members, membersCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mach_port_t name,
	mach_port_array_t *members,
	mach_msg_type_number_t *membersCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_get_assignment */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_get_assignment
#if	defined(LINTLIBRARY)
    (server, auth, proc, pset)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_t proc;
	mprocessor_set_name_t *pset;
{ return machid_processor_get_assignment(server, auth, proc, pset); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_t proc,
	mprocessor_set_name_t *pset
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_get_assignment */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_get_assignment
#if	defined(LINTLIBRARY)
    (server, auth, thread, pset)
	mach_port_t server;
	mach_port_t auth;
	mthread_t thread;
	mprocessor_set_name_t *pset;
{ return machid_thread_get_assignment(server, auth, thread, pset); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mthread_t thread,
	mprocessor_set_name_t *pset
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_get_assignment */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_get_assignment
#if	defined(LINTLIBRARY)
    (server, auth, task, pset)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mprocessor_set_name_t *pset;
{ return machid_task_get_assignment(server, auth, task, pset); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mprocessor_set_name_t *pset
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_processor_set_priv */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_processor_set_priv
#if	defined(LINTLIBRARY)
    (server, auth, host, psetn, pset)
	mach_port_t server;
	mach_port_t auth;
	mhost_priv_t host;
	mprocessor_set_name_t psetn;
	mprocessor_set_t *pset;
{ return machid_host_processor_set_priv(server, auth, host, psetn, pset); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_priv_t host,
	mprocessor_set_name_t psetn,
	mprocessor_set_t *pset
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_processor_set_names */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_processor_set_names
#if	defined(LINTLIBRARY)
    (server, auth, host, sets, setsCnt)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	mprocessor_set_name_array_t *sets;
	mach_msg_type_number_t *setsCnt;
{ return machid_host_processor_set_names(server, auth, host, sets, setsCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	mprocessor_set_name_array_t *sets,
	mach_msg_type_number_t *setsCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_create
#if	defined(LINTLIBRARY)
    (server, auth, host, pset, psetn)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	mprocessor_set_t *pset;
	mprocessor_set_name_t *psetn;
{ return machid_processor_set_create(server, auth, host, pset, psetn); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	mprocessor_set_t *pset,
	mprocessor_set_name_t *psetn
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_create
#if	defined(LINTLIBRARY)
    (server, auth, task, thread)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mthread_t *thread;
{ return machid_thread_create(server, auth, task, thread); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mthread_t *thread
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_assign */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_assign
#if	defined(LINTLIBRARY)
    (server, auth, processor, pset, wait)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_t processor;
	mprocessor_set_t pset;
	boolean_t wait;
{ return machid_processor_assign(server, auth, processor, pset, wait); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_t processor,
	mprocessor_set_t pset,
	boolean_t wait
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_assign */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_assign
#if	defined(LINTLIBRARY)
    (server, auth, thread, pset)
	mach_port_t server;
	mach_port_t auth;
	mthread_t thread;
	mprocessor_set_t pset;
{ return machid_thread_assign(server, auth, thread, pset); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mthread_t thread,
	mprocessor_set_t pset
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_assign_default */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_assign_default
#if	defined(LINTLIBRARY)
    (server, auth, thread)
	mach_port_t server;
	mach_port_t auth;
	mthread_t thread;
{ return machid_thread_assign_default(server, auth, thread); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mthread_t thread
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_assign */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_assign
#if	defined(LINTLIBRARY)
    (server, auth, task, pset, assign_threads)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	mprocessor_set_t pset;
	boolean_t assign_threads;
{ return machid_task_assign(server, auth, task, pset, assign_threads); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	mprocessor_set_t pset,
	boolean_t assign_threads
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_assign_default */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_assign_default
#if	defined(LINTLIBRARY)
    (server, auth, task, assign_threads)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	boolean_t assign_threads;
{ return machid_task_assign_default(server, auth, task, assign_threads); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	boolean_t assign_threads
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_policy_enable */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_policy_enable
#if	defined(LINTLIBRARY)
    (server, auth, processor_set, policy)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_t processor_set;
	int policy;
{ return machid_processor_set_policy_enable(server, auth, processor_set, policy); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_t processor_set,
	int policy
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_policy_disable */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_policy_disable
#if	defined(LINTLIBRARY)
    (server, auth, processor_set, policy, change_threads)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_t processor_set;
	int policy;
	boolean_t change_threads;
{ return machid_processor_set_policy_disable(server, auth, processor_set, policy, change_threads); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_t processor_set,
	int policy,
	boolean_t change_threads
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_default_pager */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_default_pager
#if	defined(LINTLIBRARY)
    (server, auth, host, default_pager)
	mach_port_t server;
	mach_port_t auth;
	mhost_priv_t host;
	mdefault_pager_t *default_pager;
{ return machid_host_default_pager(server, auth, host, default_pager); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_priv_t host,
	mdefault_pager_t *default_pager
);
#endif	/* defined(LINTLIBRARY) */

/* Routine default_pager_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_default_pager_info
#if	defined(LINTLIBRARY)
    (server, auth, default_pager, total_size, free_size)
	mach_port_t server;
	mach_port_t auth;
	mdefault_pager_t default_pager;
	vm_size_t *total_size;
	vm_size_t *free_size;
{ return machid_default_pager_info(server, auth, default_pager, total_size, free_size); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mdefault_pager_t default_pager,
	vm_size_t *total_size,
	vm_size_t *free_size
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_vm_statistics */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_vm_statistics
#if	defined(LINTLIBRARY)
    (server, auth, host, data)
	mach_port_t server;
	mach_port_t auth;
	mhost_priv_t host;
	vm_statistics_data_t *data;
{ return machid_host_vm_statistics(server, auth, host, data); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_priv_t host,
	vm_statistics_data_t *data
);
#endif	/* defined(LINTLIBRARY) */

/* Routine host_page_size */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_host_page_size
#if	defined(LINTLIBRARY)
    (server, auth, host, page_size)
	mach_port_t server;
	mach_port_t auth;
	mhost_t host;
	vm_size_t *page_size;
{ return machid_host_page_size(server, auth, host, page_size); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mhost_t host,
	vm_size_t *page_size
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_info
#if	defined(LINTLIBRARY)
    (server, auth, pset, flavor, host, info, infoCnt)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_name_t pset;
	int flavor;
	mhost_t *host;
	processor_set_info_t info;
	mach_msg_type_number_t *infoCnt;
{ return machid_processor_set_info(server, auth, pset, flavor, host, info, infoCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_name_t pset,
	int flavor,
	mhost_t *host,
	processor_set_info_t info,
	mach_msg_type_number_t *infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_info
#if	defined(LINTLIBRARY)
    (server, auth, task, flavor, info, infoCnt)
	mach_port_t server;
	mach_port_t auth;
	mtask_t task;
	int flavor;
	task_info_t info;
	mach_msg_type_number_t *infoCnt;
{ return machid_task_info(server, auth, task, flavor, info, infoCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t task,
	int flavor,
	task_info_t info,
	mach_msg_type_number_t *infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_info */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_info
#if	defined(LINTLIBRARY)
    (server, auth, thread, flavor, info, infoCnt)
	mach_port_t server;
	mach_port_t auth;
	mthread_t thread;
	int flavor;
	thread_info_t info;
	mach_msg_type_number_t *infoCnt;
{ return machid_thread_info(server, auth, thread, flavor, info, infoCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mthread_t thread,
	int flavor,
	thread_info_t info,
	mach_msg_type_number_t *infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine thread_policy */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_thread_policy
#if	defined(LINTLIBRARY)
    (server, auth, thread, policy, base, baseCnt, set_limit)
	mach_port_t server;
	mach_port_t auth;
	mthread_t thread;
	policy_t policy;
	policy_base_t base;
	mach_msg_type_number_t baseCnt;
	boolean_t set_limit;
{ return machid_thread_policy(server, auth, thread, policy, base, baseCnt, set_limit); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mthread_t thread,
	policy_t policy,
	policy_base_t base,
	mach_msg_type_number_t baseCnt,
	boolean_t set_limit
);
#endif	/* defined(LINTLIBRARY) */

/* Routine processor_set_statistics */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_processor_set_statistics
#if	defined(LINTLIBRARY)
    (server, auth, pset, flavor, info, infoCnt)
	mach_port_t server;
	mach_port_t auth;
	mprocessor_set_name_t pset;
	int flavor;
	processor_set_info_t info;
	mach_msg_type_number_t *infoCnt;
{ return machid_processor_set_statistics(server, auth, pset, flavor, info, infoCnt); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mprocessor_set_name_t pset,
	int flavor,
	processor_set_info_t info,
	mach_msg_type_number_t *infoCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine task_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_task_create
#if	defined(LINTLIBRARY)
    (server, auth, parent, ledger_ports, ledger_portsCnt, inherit, task)
	mach_port_t server;
	mach_port_t auth;
	mtask_t parent;
	ledger_port_array_t ledger_ports;
	mach_msg_type_number_t ledger_portsCnt;
	boolean_t inherit;
	mtask_t *task;
{ return machid_task_create(server, auth, parent, ledger_ports, ledger_portsCnt, inherit, task); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mtask_t parent,
	ledger_port_array_t ledger_ports,
	mach_msg_type_number_t ledger_portsCnt,
	boolean_t inherit,
	mtask_t *task
);
#endif	/* defined(LINTLIBRARY) */

/* Routine node_host_ports */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_node_host_ports
#if	defined(LINTLIBRARY)
    (server, auth, node, host, phost)
	mach_port_t server;
	mach_port_t auth;
	int node;
	mhost_t *host;
	mhost_priv_t *phost;
{ return machid_node_host_ports(server, auth, node, host, phost); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	int node,
	mhost_t *host,
	mhost_priv_t *phost
);
#endif	/* defined(LINTLIBRARY) */

/* Routine mach_node */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t machid_mach_node
#if	defined(LINTLIBRARY)
    (server, auth, id, node)
	mach_port_t server;
	mach_port_t auth;
	mach_id_t id;
	int *node;
{ return machid_mach_node(server, auth, id, node); }
#else
(
	mach_port_t server,
	mach_port_t auth,
	mach_id_t id,
	int *node
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_machid
#define subsystem_to_name_map_machid \
    { "mach_type", 9829283 },\
    { "mach_register", 9829284 },\
    { "mach_lookup", 9829285 },\
    { "mach_port", 9829286 },\
    { "host_ports", 9829287 },\
    { "host_processor_sets", 9829288 },\
    { "host_tasks", 9829289 },\
    { "host_threads", 9829290 },\
    { "host_processors", 9829291 },\
    { "processor_set_threads", 9829292 },\
    { "processor_set_tasks", 9829293 },\
    { "task_threads", 9829294 },\
    { "host_basic_info", 9829295 },\
    { "host_sched_info", 9829296 },\
    { "host_load_info", 9829297 },\
    { "processor_set_default", 9829298 },\
    { "host_kernel_version", 9829299 },\
    { "processor_basic_info", 9829300 },\
    { "task_thread_times_info", 9829305 },\
    { "task_terminate", 9829312 },\
    { "task_suspend", 9829313 },\
    { "task_resume", 9829314 },\
    { "thread_terminate", 9829315 },\
    { "thread_suspend", 9829316 },\
    { "thread_resume", 9829317 },\
    { "thread_abort", 9829318 },\
    { "processor_set_destroy", 9829320 },\
    { "processor_start", 9829321 },\
    { "processor_exit", 9829322 },\
    { "vm_region", 9829323 },\
    { "vm_read", 9829324 },\
    { "processor_set_max_priority", 9829328 },\
    { "port_names", 9829329 },\
    { "port_type", 9829330 },\
    { "port_get_refs", 9829331 },\
    { "port_get_receive_status", 9829332 },\
    { "port_get_set_status", 9829333 },\
    { "processor_get_assignment", 9829334 },\
    { "thread_get_assignment", 9829335 },\
    { "task_get_assignment", 9829336 },\
    { "host_processor_set_priv", 9829337 },\
    { "host_processor_set_names", 9829338 },\
    { "processor_set_create", 9829339 },\
    { "thread_create", 9829341 },\
    { "processor_assign", 9829342 },\
    { "thread_assign", 9829343 },\
    { "thread_assign_default", 9829344 },\
    { "task_assign", 9829345 },\
    { "task_assign_default", 9829346 },\
    { "processor_set_policy_enable", 9829348 },\
    { "processor_set_policy_disable", 9829349 },\
    { "host_default_pager", 9829350 },\
    { "default_pager_info", 9829351 },\
    { "host_vm_statistics", 9829354 },\
    { "host_page_size", 9829355 },\
    { "processor_set_info", 9829356 },\
    { "task_info", 9829357 },\
    { "thread_info", 9829358 },\
    { "thread_policy", 9829359 },\
    { "processor_set_statistics", 9829360 },\
    { "task_create", 9829361 },\
    { "node_host_ports", 9829362 },\
    { "mach_node", 9829363 }
#endif

#endif	 /* _machid_user_ */
