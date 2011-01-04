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
#include <vm/vm_kern.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/thread_act.h>
#include <device/net_io.h>
#include <device/if_hdr.h>
#include <mach/mach_host_server.h>
#include <ppc/misc_protos.h>
#include <ppc/machparam.h>
#include <ppc/POWERMAC/physmem_entries.h>

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
	vm_offset_t	paddr;
	vm_offset_t	vaddr;
	vm_offset_t	daddr;
	vm_size_t	count;

	rc = device_read_alloc(ior, (vm_size_t) ior->io_count);
	if (rc != KERN_SUCCESS)
		return rc;

	paddr = ior->io_recnum;
	daddr = (vm_offset_t)ior->io_data;
	/*
	 * Copy from the physical memory. Only allow accesses
	 * to pages that are already mapped in the microkernel space
	 * (with io_map or other).
	 */
	while (ior->io_count > 0) {

		vaddr = phystokv(paddr);

		if (vaddr == (vm_offset_t)NULL) {
			ior->io_residual = ior->io_count;
			ior->io_error = D_INVALID_RECNUM;
			ior->io_op |= IO_ERROR;
			break;
		}
		count = round_page(vaddr+1) - vaddr;
		if (count > ior->io_count)
			count = ior->io_count;
		/* Do the bcopy without using cache instructions
		 * We are sure that it will work
		 */
		bcopy_nc((char *)vaddr, (char *)daddr, count);
		ior->io_count -= count;
		daddr += count;
		paddr += count;
	}
	ior->io_residual = 0;

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
	vm_offset_t	paddr;
	vm_offset_t	vaddr;
	vm_offset_t	saddr;
	vm_size_t	count;

	rc = device_read_alloc(ior, (vm_size_t) ior->io_count);
	if (rc != KERN_SUCCESS)
		return rc;

	paddr = ior->io_recnum;
	saddr = (vm_offset_t)ior->io_data;
	/*
	 * Copy to the physical memory. Only allow accesses
	 * to pages that are already mapped and expect them to be writable
	 * in the microkernel space (done with io_map or other).
	 */
	while (ior->io_count > 0) {

		vaddr = phystokv(paddr);

		if (vaddr == (vm_offset_t)NULL) {
			ior->io_residual = ior->io_count;
			ior->io_error = D_INVALID_RECNUM;
			ior->io_op |= IO_ERROR;
			break;
		}
		count = round_page(vaddr+1) - vaddr;
		if (count > ior->io_count)
			count = ior->io_count;
		/* Do the bcopy without using cache instructions
		 * We expect it to work.
		 */
		bcopy_nc((char *)saddr, (char *)vaddr, count);
		ior->io_count -= count;
		saddr += count;
		paddr += count;
	}
	ior->io_residual = 0;

	iodone(ior);

	if (ior->io_op & IO_SYNC) {
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
	return btop(off);
}
