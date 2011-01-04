/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 *
 */
/*
 * MkLinux
 */

#include <mach/flipc_device.h>
#include <sys/types.h>
#include <device/io_req.h>

/*
 * flipc/flipc_usermsg.h
 *
 * Routines and data types in the kernel portion of flipc that need
 * to be exported to the rest of the kernel.  
 */

/*
 * Communications buffer description variables.  This duplicates
 * declarations in flipc_cb.h, and should be kept in sync with the
 * declarations in that file.  But there's plenty too much in that
 * file to be exported kernel wide.
 */
u_char *flipc_cb_base;
unsigned long flipc_cb_length;		/* In bytes.  */

/* Device driver entry points.  */
extern io_return_t usermsg_open(dev_t		dev,
				dev_mode_t	flag,
				io_req_t	ior);

extern vm_offset_t usermsg_map(dev_t dev, vm_offset_t off, vm_prot_t prot);
extern io_return_t usermsg_set_status(dev_t dev, int flavor,
				      dev_status_t status,
				      unsigned int status_count);
void usermsg_close(dev_t dev);
io_return_t usermsg_getstat(dev_t dev,
			    dev_flavor_t flavor,
			    dev_status_t data,
			    mach_msg_type_number_t *count);
io_return_t usermsg_setstat(dev_t dev,
			    dev_flavor_t flavor,
			    dev_status_t data,
			    mach_msg_type_number_t count);



/* Called at system initialization.  */
extern void flipc_system_init(void);

