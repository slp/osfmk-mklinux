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
 */
/*
 * MkLinux
 */

#include <physmem.h>

#include <types.h>
#include <device/buf.h>
#include <device/conf.h>
#include <device/errno.h>
#include <device/misc_protos.h>
#include <device/ds_routines.h>
#include <kern/misc_protos.h>
#include <i386/AT386/physmem_entries.h>
#include <vm/vm_kern.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/thread_act.h>
#include <device/net_io.h>
#include <device/if_hdr.h>
#include <mach/mach_host_server.h>

io_return_t
physmem_open(
	dev_t		dev,
	dev_mode_t	flag,
	io_req_t	ior)
{
	return D_SUCCESS;
}

void
physmem_close(
	dev_t		dev)
{
	return;
}

io_return_t
physmem_read(
	dev_t		dev,
	io_req_t 	ior)
{
	kern_return_t	rc;
	vm_offset_t	offset;
	int		ret;

	rc = device_read_alloc(ior, (vm_size_t) ior->io_count);
	if (rc != KERN_SUCCESS)
		return rc;

	/*
	 * Copy from the physical memory.
	 * Use copyin to recover from bad memory accesses...
	 */
	ret = copyin((void *) phystokv(ior->io_recnum),
		     ior->io_data,
		     ior->io_count);
	if (ret == 0) {
		ior->io_residual = 0;
	} else {
		ior->io_residual = ior->io_count;
		ior->io_error = D_INVALID_RECNUM;
		ior->io_op |= IO_ERROR;
	}

	iodone(ior);

	if (ior->io_op & IO_SYNC) {
		iowait(ior);
		return D_SUCCESS;
	}

	return D_IO_QUEUED;
}

io_return_t
physmem_write(
	dev_t		dev,
	io_req_t	ior)
{
	kern_return_t	rc;
	boolean_t	wait = FALSE;
	int		ret;

	rc = device_write_get(ior, &wait);
	if (rc != KERN_SUCCESS)
		return rc;

	/*
	 * Copy to the physical memory.
	 * Use copyout to recover from bad memory accesses...
	 */
	ret = copyout(ior->io_data, 
		      (void *) phystokv(ior->io_recnum),
		      ior->io_count);
	if (ret == 0) {
		ior->io_residual = 0;
	} else {
		ior->io_residual = ior->io_count;
		ior->io_error = D_INVALID_RECNUM;
		ior->io_op |= IO_ERROR;
	}

	iodone(ior);

	if (wait || (ior->io_op & IO_SYNC)) {
		iowait(ior);
		return D_SUCCESS;
	}

	return D_IO_QUEUED;
}

vm_offset_t
physmem_mmap(
	dev_t		dev,
	vm_offset_t	off,
	vm_prot_t	prot)
{
	return i386_btop(off);
}
