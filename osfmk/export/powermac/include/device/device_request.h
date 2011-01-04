#ifndef	_device_request_user_
#define	_device_request_user_

/* Module device_request */

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

#ifndef	device_request_MSG_COUNT
#define	device_request_MSG_COUNT	15
#endif	/* device_request_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <device/device_types.h>
#include <device/net_status.h>

/* SimpleRoutine device_write_request */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_write_request
#if	defined(LINTLIBRARY)
    (device, reply_port, mode, recnum, data, dataCnt)
	mach_port_t device;
	mach_port_t reply_port;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_ptr_t data;
	mach_msg_type_number_t dataCnt;
{ return device_write_request(device, reply_port, mode, recnum, data, dataCnt); }
#else
(
	mach_port_t device,
	mach_port_t reply_port,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_ptr_t data,
	mach_msg_type_number_t dataCnt
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine device_write_request_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_write_request_inband
#if	defined(LINTLIBRARY)
    (device, reply_port, mode, recnum, data, dataCnt)
	mach_port_t device;
	mach_port_t reply_port;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_ptr_inband_t data;
	mach_msg_type_number_t dataCnt;
{ return device_write_request_inband(device, reply_port, mode, recnum, data, dataCnt); }
#else
(
	mach_port_t device,
	mach_port_t reply_port,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_ptr_inband_t data,
	mach_msg_type_number_t dataCnt
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine device_read_request */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_read_request
#if	defined(LINTLIBRARY)
    (device, reply_port, mode, recnum, bytes_wanted)
	mach_port_t device;
	mach_port_t reply_port;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_len_t bytes_wanted;
{ return device_read_request(device, reply_port, mode, recnum, bytes_wanted); }
#else
(
	mach_port_t device,
	mach_port_t reply_port,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_len_t bytes_wanted
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine device_read_request_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_read_request_inband
#if	defined(LINTLIBRARY)
    (device, reply_port, mode, recnum, bytes_wanted)
	mach_port_t device;
	mach_port_t reply_port;
	dev_mode_t mode;
	recnum_t recnum;
	io_buf_len_t bytes_wanted;
{ return device_read_request_inband(device, reply_port, mode, recnum, bytes_wanted); }
#else
(
	mach_port_t device,
	mach_port_t reply_port,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_len_t bytes_wanted
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine device_open_request */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t device_open_request
#if	defined(LINTLIBRARY)
    (device_server_port, reply_port, ledger, mode, sec_token, name)
	mach_port_t device_server_port;
	mach_port_t reply_port;
	mach_port_t ledger;
	dev_mode_t mode;
	security_token_t sec_token;
	dev_name_t name;
{ return device_open_request(device_server_port, reply_port, ledger, mode, sec_token, name); }
#else
(
	mach_port_t device_server_port,
	mach_port_t reply_port,
	mach_port_t ledger,
	dev_mode_t mode,
	security_token_t sec_token,
	dev_name_t name
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_device_request
#define subsystem_to_name_map_device_request \
    { "device_write_request", 2802 },\
    { "device_write_request_inband", 2803 },\
    { "device_read_request", 2804 },\
    { "device_read_request_inband", 2805 },\
    { "device_open_request", 2814 }
#endif

#endif	 /* _device_request_user_ */
