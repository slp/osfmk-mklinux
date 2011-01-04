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
/*
 * Copyright (c) 1988,1991 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr$
 */

/*
 * Memory special file
 */

#include <sys/types.h>
#include <device/param.h>
#include <device/errno.h>
#include <device/buf.h>
#include <device/conf.h>
#include <machine/machparam.h>
#include <kern/spl.h>
#include <mach/vm_param.h>

#include <device/ds_routines.h>
#include <kern/misc_protos.h>

/*
 * Forwards
 */
io_return_t mmopen(dev_t, int, io_req_t);
int mmread(dev_t, io_req_t);
int mmwrite(dev_t, io_req_t);
int memaccess(unsigned int, unsigned int, int);
int mmrw(dev_t, struct uio *, int);

io_return_t
mmopen(
       dev_t dev,
       int flags,
       io_req_t	ior)
{
    return 0;
}

int
mmread(
       dev_t dev,
       struct uio *uio)
{
	return (mmrw(dev, uio, IO_READ));
}

int
mmwrite(
	dev_t dev,
	struct uio *uio)
{
	return (mmrw(dev, uio, IO_WRITE));
}

int
mmrw(
	dev_t dev,
	struct uio *uio,
	int rw)
{
	int error = 0;
	int s;
	unsigned int c;

	switch (minor(dev)) {

	case 0:			/* minor device 0 is physical memory */
		return (ENXIO);

	case 1:			/* minor device 1 is kernel memory */
		c = (unsigned)uio->io_count;
		
		if (!memaccess(uio->io_recnum, c, 0))
			return (EIO);

		if (rw == IO_WRITE) {
			/*
		 	 * Must be at splvm to keep out ISRs from trying to 
		 	 * look at the virtual address before we flush it. 
		 	 */
			
			s = splvm();
			bcopy(uio->io_data, (caddr_t)uio->io_recnum, c);
			fdcache(0, (vm_offset_t)uio->io_recnum, c);
			splx(s);
			return (error);
		}

		/*
		 * Allocate memory for read buffer.
		 */
		error = device_read_alloc(uio, c);
		if (error != KERN_SUCCESS)
			return (error);

		s = splvm();
		bcopy((caddr_t)uio->io_recnum, uio->io_data, c);
		fdcache(0, (vm_offset_t)uio->io_recnum, c);
		splx(s);
		return (error);

	    default:
		return EIO;
	}
}

/*
 * See if count bytes starting at VA addr are accessible.
 */
int
memaccess(
	  unsigned int addr,
	  unsigned int count, 
	  int rw)
{
	extern pmap_t		kernel_pmap;
	register vm_offset_t	va;
	register vm_offset_t	vend = round_page(addr + count);

	if (addr >= 0 && addr <= VM_MIN_KERNEL_ADDRESS)
		return 1;

	for (va = trunc_page(addr); va != vend; va += HP700_PGBYTES)
		if (pmap_extract(kernel_pmap, va) == 0)
			return 0;
	return 1;
}
