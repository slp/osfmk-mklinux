#ifndef	_device_user_
#define	_device_user_

/* Module device */

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

#ifndef	device_MSG_COUNT
#define	device_MSG_COUNT	17
#endif	/* device_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <device/device_types.h>
#include <device/net_status.h>

/* Routine device_close */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_close
#if	defined(LINTLIBRARY)
    (device)
	mach_port_t device;
{ return device_close(device); }
#else
(
	mach_port_t device
);
#endif	/* defined(LINTLIBRARY) */

/* Routine device_write */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_write
#if	defined(LINTLIBRARY)
    (device, mode, recnum, data, dataCnt, bytes_written)
	mach_port_t device;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_ptr_t data;
	mach_msg_type_number_t dataCnt;
	io_buf_len_t *bytes_written;
{ return device_write(device, mode, recnum, data, dataCnt, bytes_written); }
#else
(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_ptr_t data,
	mach_msg_type_number_t dataCnt,
	io_buf_len_t *bytes_written
);
#endif	/* defined(LINTLIBRARY) */

/* Routine device_write_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_write_inband
#if	defined(LINTLIBRARY)
    (device, mode, recnum, data, dataCnt, bytes_written)
	mach_port_t device;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_ptr_inband_t data;
	mach_msg_type_number_t dataCnt;
	io_buf_len_t *bytes_written;
{ return device_write_inband(device, mode, recnum, data, dataCnt, bytes_written); }
#else
(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_ptr_inband_t data,
	mach_msg_type_number_t dataCnt,
	io_buf_len_t *bytes_written
);
#endif	/* defined(LINTLIBRARY) */

/* Routine device_read */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_read
#if	defined(LINTLIBRARY)
    (device, mode, recnum, bytes_wanted, data, dataCnt)
	mach_port_t device;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_len_t bytes_wanted;
	io_buf_ptr_t *data;
	mach_msg_type_number_t *dataCnt;
{ return device_read(device, mode, recnum, bytes_wanted, data, dataCnt); }
#else
(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_len_t bytes_wanted,
	io_buf_ptr_t *data,
	mach_msg_type_number_t *dataCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine device_read_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_read_inband
#if	defined(LINTLIBRARY)
    (device, mode, recnum, bytes_wanted, data, dataCnt)
	mach_port_t device;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_len_t bytes_wanted;
	io_buf_ptr_inband_t data;
	mach_msg_type_number_t *dataCnt;
{ return device_read_inband(device, mode, recnum, bytes_wanted, data, dataCnt); }
#else
(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_len_t bytes_wanted,
	io_buf_ptr_inband_t data,
	mach_msg_type_number_t *dataCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine device_map */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_map
#if	defined(LINTLIBRARY)
    (device, prot, offset, size, pager, unmap)
	mach_port_t device;
	vm_prot_t prot;
	vm_offset_t offset;
	vm_size_t size;
	mach_port_t *pager;
	boolean_t unmap;
{ return device_map(device, prot, offset, size, pager, unmap); }
#else
(
	mach_port_t device,
	vm_prot_t prot,
	vm_offset_t offset,
	vm_size_t size,
	mach_port_t *pager,
	boolean_t unmap
);
#endif	/* defined(LINTLIBRARY) */

/* Routine device_set_status */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_set_status
#if	defined(LINTLIBRARY)
    (device, flavor, status, statusCnt)
	mach_port_t device;
	dev_flavor_t flavor;
	dev_status_t status;
	mach_msg_type_number_t statusCnt;
{ return device_set_status(device, flavor, status, statusCnt); }
#else
(
	mach_port_t device,
	dev_flavor_t flavor,
	dev_status_t status,
	mach_msg_type_number_t statusCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine device_get_status */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_get_status
#if	defined(LINTLIBRARY)
    (device, flavor, status, statusCnt)
	mach_port_t device;
	dev_flavor_t flavor;
	dev_status_t status;
	mach_msg_type_number_t *statusCnt;
{ return device_get_status(device, flavor, status, statusCnt); }
#else
(
	mach_port_t device,
	dev_flavor_t flavor,
	dev_status_t status,
	mach_msg_type_number_t *statusCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine device_set_filter */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_set_filter
#if	defined(LINTLIBRARY)
    (device, receive_port, receive_portPoly, priority, filter, filterCnt)
	mach_port_t device;
	mach_port_t receive_port;
	mach_msg_type_name_t receive_portPoly;
	int priority;
	filter_array_t filter;
	mach_msg_type_number_t filterCnt;
{ return device_set_filter(device, receive_port, receive_portPoly, priority, filter, filterCnt); }
#else
(
	mach_port_t device,
	mach_port_t receive_port,
	mach_msg_type_name_t receive_portPoly,
	int priority,
	filter_array_t filter,
	mach_msg_type_number_t filterCnt
);
#endif	/* defined(LINTLIBRARY) */

/* Routine device_open */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_open
#if	defined(LINTLIBRARY)
    (master_port, ledger, mode, sec_token, name, device)
	mach_port_t master_port;
	mach_port_t ledger;
	dev_mode_t mode;
	security_token_t sec_token;
	dev_name_t name;
	mach_port_t *device;
{ return device_open(master_port, ledger, mode, sec_token, name, device); }
#else
(
	mach_port_t master_port,
	mach_port_t ledger,
	dev_mode_t mode,
	security_token_t sec_token,
	dev_name_t name,
	mach_port_t *device
);
#endif	/* defined(LINTLIBRARY) */

/* Routine io_done_queue_create */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t io_done_queue_create
#if	defined(LINTLIBRARY)
    (host, queue)
	mach_port_t host;
	mach_port_t *queue;
{ return io_done_queue_create(host, queue); }
#else
(
	mach_port_t host,
	mach_port_t *queue
);
#endif	/* defined(LINTLIBRARY) */

/* Routine io_done_queue_terminate */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t io_done_queue_terminate
#if	defined(LINTLIBRARY)
    (queue)
	mach_port_t queue;
{ return io_done_queue_terminate(queue); }
#else
(
	mach_port_t queue
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_device
#define subsystem_to_name_map_device \
    { "device_close", 2801 },\
    { "device_write", 2802 },\
    { "device_write_inband", 2803 },\
    { "device_read", 2804 },\
    { "device_read_inband", 2805 },\
    { "device_map", 2809 },\
    { "device_set_status", 2810 },\
    { "device_get_status", 2811 },\
    { "device_set_filter", 2812 },\
    { "device_open", 2814 },\
    { "io_done_queue_create", 2815 },\
    { "io_done_queue_terminate", 2816 }
#endif

#endif	 /* _device_user_ */
