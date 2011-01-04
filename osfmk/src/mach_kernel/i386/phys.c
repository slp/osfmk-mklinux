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
/* CMU_HIST */
/*
 * Revision 2.8  91/05/18  14:30:02  rpd
 * 	Added vm_page_fictitious_addr assertions.
 * 	[91/04/10            rpd]
 * 
 * Revision 2.7  91/05/14  16:13:29  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/05/08  12:41:06  dbg
 * 	More cleanup.
 * 	[91/03/21            dbg]
 * 
 * Revision 2.5  91/03/16  14:45:06  rpd
 * 	Added resume, continuation arguments to vm_fault.
 * 	[91/02/05            rpd]
 * 
 * Revision 2.4  91/02/05  17:13:36  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:36:36  mrt]
 * 
 * Revision 2.3  90/12/04  14:46:19  jsb
 * 	Changes for merged intel/pmap.{c,h}.
 * 	[90/12/04  11:17:19  jsb]
 * 
 * Revision 2.2  90/05/03  15:35:56  dbg
 * 	Use 'write' bit in pte instead of protection field.
 * 	[90/03/25            dbg]
 * 
 * 	Use bzero instead of bclear.
 * 	[90/02/15            dbg]
 * 
 * Revision 1.3  89/02/26  12:32:59  gm0w
 * 	Changes for cleanup.
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
#include <string.h>

#include <mach/vm_param.h>
#include <mach/boolean.h>
#include <vm/pmap.h>
#include <vm/vm_page.h>
#include <kern/misc_protos.h>

/*
 *	pmap_zero_page zeros the specified (machine independent) page.
 */
void
pmap_zero_page(
	vm_offset_t p)
{
	assert(p != vm_page_fictitious_addr);
	bzero((char *)phystokv(p), PAGE_SIZE);
}

/*
 *	pmap_zero_part_page
 *	zeros the specified (machine independent) part of a page.
 */
void
pmap_zero_part_page(
	vm_offset_t	p,
	vm_offset_t     offset,
	vm_size_t       len)
{
	assert(p != vm_page_fictitious_addr);
	assert(offset + len <= PAGE_SIZE);

	bzero((char *)phystokv(p) + offset, len);
}

/*
 *	pmap_copy_page copies the specified (machine independent) pages.
 */
void
pmap_copy_page(
	vm_offset_t src,
	vm_offset_t dst)
{
	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);

	memcpy((void *)phystokv(dst), (void *)phystokv(src), PAGE_SIZE);
}

/*
 *	pmap_copy_page copies the specified (machine independent) pages.
 */
void
pmap_copy_part_page(
	vm_offset_t	src,
	vm_offset_t	src_offset,
	vm_offset_t	dst,
	vm_offset_t	dst_offset,
	vm_size_t	len)
{
	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);
	assert(((dst & PAGE_MASK) + dst_offset + len) <= PAGE_SIZE);
	assert(((src & PAGE_MASK) + src_offset + len) <= PAGE_SIZE);

        memcpy((void *)(phystokv(dst) + dst_offset),
	       (void *)(phystokv(src) + src_offset), len);
}

/*
 *      pmap_copy_part_lpage copies part of a virtually addressed page 
 *      to a physically addressed page.
 */
void
pmap_copy_part_lpage(
	vm_offset_t	src,
	vm_offset_t	dst,
	vm_offset_t	dst_offset,
	vm_size_t	len)
{
	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);
	assert(((dst & PAGE_MASK) + dst_offset + len) <= PAGE_SIZE);

        memcpy((void *)(phystokv(dst) + dst_offset), (void *)src, len);
}

/*
 *      pmap_copy_part_rpage copies part of a physically addressed page 
 *      to a virtually addressed page.
 */
void
pmap_copy_part_rpage(
	vm_offset_t	src,
	vm_offset_t	src_offset,
	vm_offset_t	dst,
	vm_size_t	len)
{
	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);
	assert(((src & PAGE_MASK) + src_offset + len) <= PAGE_SIZE);

        memcpy((void *)dst, (void *)(phystokv(src) + src_offset), len);
}

/*
 *	kvtophys(addr)
 *
 *	Convert a kernel virtual address to a physical address
 */
vm_offset_t
kvtophys(
	vm_offset_t addr)
{
	pt_entry_t *pte;

	if ((pte = pmap_pte(kernel_pmap, addr)) == PT_ENTRY_NULL)
		return 0;
	return i386_trunc_page(*pte) | (addr & INTEL_OFFMASK);
}
