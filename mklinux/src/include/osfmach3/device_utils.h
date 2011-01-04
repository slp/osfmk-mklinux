/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * Support routines for device interface in out-of-kernel kernel.
 */
#ifndef	_OSFMACH3_DEVICE_UTILS_H_
#define	_OSFMACH3_DEVICE_UTILS_H_

#include <mach/message.h>
#include <device/device_types.h>
#include <device/device.h>

#include <linux/types.h>
#include <linux/kdev_t.h>

extern mach_port_t	device_server_port;
extern security_token_t server_security_token;

extern void	dev_utils_init(void);
extern int	dev_error_to_errno(int);

/* XXX - these should be exported by Mach, but aren't. */
extern kern_return_t
device_read_async(
    mach_port_t device, mach_port_t io_done_queue, mach_port_t reply_port,
    dev_mode_t mode, recnum_t recnum, int bytes_wanted);
extern kern_return_t
device_read_async_inband(
    mach_port_t device, mach_port_t io_done_queue, mach_port_t reply_port,
    dev_mode_t mode, recnum_t recnum, int bytes_wanted);
extern kern_return_t
device_read_overwrite_async(
    mach_port_t device, mach_port_t io_done_queue, mach_port_t reply_port,
    dev_mode_t mode, recnum_t recnum, int bytes_wanted, vm_address_t buffer);
extern kern_return_t
device_write_async(
    mach_port_t device, mach_port_t io_done_queue, mach_port_t reply_port,
    dev_mode_t mode, recnum_t recnum, io_buf_ptr_t data, unsigned int count);
extern kern_return_t
device_write_async_inband(
    mach_port_t device, mach_port_t io_done_queue, mach_port_t reply_port,
    dev_mode_t mode, recnum_t recnum, io_buf_ptr_t data, unsigned int count);

extern void		dev_to_object_init(void);
extern void		dev_to_object_register(kdev_t dev,
					       int type,
					       char *object);
extern char 		*dev_to_object_lookup(kdev_t dev,
					      int type);
extern void		dev_to_object_deregister(kdev_t dev,
						 int type);

extern kern_return_t serv_device_read_async(mach_port_t device,
					    mach_port_t reply_port,
					    dev_mode_t mode,
					    recnum_t recnum,
					    caddr_t data,
					    mach_msg_type_number_t count,
					    boolean_t inband);
extern kern_return_t serv_device_write_async(mach_port_t device,
					     mach_port_t reply_port,
					     dev_mode_t mode,
					     recnum_t recnum,
					     caddr_t data,
					     mach_msg_type_number_t count,
					     boolean_t inband);
extern memory_object_t device_pager_create(kdev_t dev,
					   vm_offset_t offset,
					   vm_size_t size,
					   vm_prot_t protection);

extern int osfmach3_check_blocksize(kdev_t dev,
				    int size);

#endif	/* _OSFMACH3_DEVICE_UTILS_H_ */
