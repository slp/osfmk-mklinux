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
/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
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
/* CMU_HIST */
/*
 * Revision 2.3  92/05/05  10:53:00  danner
 * 	Added frc_set_address(), to customize it.
 * 	Makes NSC's boards happy (any TC box).
 * 	[92/04/13            jcb]
 * 
 * Revision 2.2  92/04/01  15:14:23  rpd
 * 	Created, based on maxine's counter.
 * 	[92/03/10            af]
 * 
 */
/* CMU_ENDHIST */
/*
 *	File: frc.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	3/92
 *
 *	Generic, mappable free running counter driver.
 */

#include <frc.h>
#if	NFRC > 0

#include <mach/std_types.h>
#include <sys/types.h>
#include <chips/busses.h>
#include <device/device_types.h>

/*
 * Machine defines
 * All you need to do to get this working on a
 * random box is to define one macro and provide
 * the correct virtual address.
 */
#include	<platforms.h>
#ifdef	DECSTATION
#define	btop(x)		mips_btop(x)
#endif	/* DECSTATION */

/*
 * Autoconf info
 */

static caddr_t frc_std[NFRC] = { 0 };
static vm_size_t frc_offset[NFRC] = { 0 };
static struct bus_device *frc_info[NFRC];
static int frc_probe(vm_offset_t,struct bus_device *);
static int frc_attach(struct bus_device *);

struct bus_driver frc_driver =
       { frc_probe, 0, frc_attach, 0, frc_std, "frc", frc_info, };

/*
 * Externally visible functions
 */
io_return_t	frc_openclose(int,int);			/* user */
vm_offset_t	frc_mmap(int,vm_offset_t,vm_prot_t);
void		frc_set_address(int,vm_size_t);

/* machine-specific setups */
void
frc_set_address(
	int		unit,
	vm_size_t	offset)
{
	if (unit < NFRC) {
		frc_offset[unit] =  offset;
	}
}


/*
 * Probe chip to see if it is there
 */
static frc_probe (
	vm_offset_t	reg,
	struct bus_device *ui)
{
	/* see if something present at the given address */
	if (check_memory(reg, 0))
		return 0;
	frc_std[ui->unit] = (caddr_t) reg;
	printf("[mappable] ");
	return 1;
}

static frc_attach (
		   struct bus_device *ui)
{
	printf(": free running counter");
}

int frc_intr()
{
	/* we do not expect interrupts */
	panic("frc_intr");
}

io_return_t
frc_openclose(
	      int dev, 
	      int flag)
{
  if (frc_std[dev])
    return D_SUCCESS;
  else
    return D_NO_SUCH_DEVICE;
}

vm_offset_t
frc_mmap(
	int		dev,
	vm_offset_t	off,
	vm_prot_t	prot)
{
	vm_offset_t addr;
	if ((prot & VM_PROT_WRITE) || (off >= PAGE_SIZE) )
		return (-1);
	addr = (vm_offset_t) frc_std[dev] + frc_offset[dev];
	return btop(pmap_extract(pmap_kernel(), addr));
}

#endif
