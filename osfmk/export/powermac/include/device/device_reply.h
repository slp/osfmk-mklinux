#ifndef	_device_reply_user_
#define	_device_reply_user_

/* Module device_reply */

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

#ifndef	device_reply_MSG_COUNT
#define	device_reply_MSG_COUNT	15
#endif	/* device_reply_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <device/device_types.h>
#include <device/net_status.h>

/* SimpleRoutine device_write_reply */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ds_device_write_reply
#if	defined(LINTLIBRARY)
    (reply_port, reply_portPoly, return_code, bytes_written)
	mach_port_t reply_port;
	mach_msg_type_name_t reply_portPoly;
	kern_return_t return_code;
	io_buf_len_t bytes_written;
{ return ds_device_write_reply(reply_port, reply_portPoly, return_code, bytes_written); }
#else
(
	mach_port_t reply_port,
	mach_msg_type_name_t reply_portPoly,
	kern_return_t return_code,
	io_buf_len_t bytes_written
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine device_write_reply_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ds_device_write_reply_inband
#if	defined(LINTLIBRARY)
    (reply_port, reply_portPoly, return_code, bytes_written)
	mach_port_t reply_port;
	mach_msg_type_name_t reply_portPoly;
	kern_return_t return_code;
	io_buf_len_t bytes_written;
{ return ds_device_write_reply_inband(reply_port, reply_portPoly, return_code, bytes_written); }
#else
(
	mach_port_t reply_port,
	mach_msg_type_name_t reply_portPoly,
	kern_return_t return_code,
	io_buf_len_t bytes_written
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine device_read_reply */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ds_device_read_reply
#if	defined(LINTLIBRARY)
    (reply_port, reply_portPoly, return_code, data, dataCnt)
	mach_port_t reply_port;
	mach_msg_type_name_t reply_portPoly;
	kern_return_t return_code;
	io_buf_ptr_t data;
	mach_msg_type_number_t dataCnt;
{ return ds_device_read_reply(reply_port, reply_portPoly, return_code, data, dataCnt); }
#else
(
	mach_port_t reply_port,
	mach_msg_type_name_t reply_portPoly,
	kern_return_t return_code,
	io_buf_ptr_t data,
	mach_msg_type_number_t dataCnt
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine device_read_reply_inband */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ds_device_read_reply_inband
#if	defined(LINTLIBRARY)
    (reply_port, reply_portPoly, return_code, data, dataCnt)
	mach_port_t reply_port;
	mach_msg_type_name_t reply_portPoly;
	kern_return_t return_code;
	io_buf_ptr_inband_t data;
	mach_msg_type_number_t dataCnt;
{ return ds_device_read_reply_inband(reply_port, reply_portPoly, return_code, data, dataCnt); }
#else
(
	mach_port_t reply_port,
	mach_msg_type_name_t reply_portPoly,
	kern_return_t return_code,
	io_buf_ptr_inband_t data,
	mach_msg_type_number_t dataCnt
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine device_read_reply_overwrite */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ds_device_read_reply_overwrite
#if	defined(LINTLIBRARY)
    (reply_port, reply_portPoly, return_code, bytes_read)
	mach_port_t reply_port;
	mach_msg_type_name_t reply_portPoly;
	kern_return_t return_code;
	io_buf_len_t bytes_read;
{ return ds_device_read_reply_overwrite(reply_port, reply_portPoly, return_code, bytes_read); }
#else
(
	mach_port_t reply_port,
	mach_msg_type_name_t reply_portPoly,
	kern_return_t return_code,
	io_buf_len_t bytes_read
);
#endif	/* defined(LINTLIBRARY) */

/* SimpleRoutine device_open_reply */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t ds_device_open_reply
#if	defined(LINTLIBRARY)
    (reply_port, reply_portPoly, return_code, device_port)
	mach_port_t reply_port;
	mach_msg_type_name_t reply_portPoly;
	kern_return_t return_code;
	mach_port_t device_port;
{ return ds_device_open_reply(reply_port, reply_portPoly, return_code, device_port); }
#else
(
	mach_port_t reply_port,
	mach_msg_type_name_t reply_portPoly,
	kern_return_t return_code,
	mach_port_t device_port
);
#endif	/* defined(LINTLIBRARY) */

#ifndef subsystem_to_name_map_device_reply
#define subsystem_to_name_map_device_reply \
    { "device_write_reply", 2902 },\
    { "device_write_reply_inband", 2903 },\
    { "device_read_reply", 2904 },\
    { "device_read_reply_inband", 2905 },\
    { "device_read_reply_overwrite", 2913 },\
    { "device_open_reply", 2914 }
#endif

#endif	 /* _device_reply_user_ */
