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
 * Revision 2.7  91/08/24  11:51:27  af
 * 	Mods to cope with the 386 PC/AT ISA bus.
 * 	[91/06/20            af]
 * 
 * Revision 2.7  91/06/19  11:46:25  rvb
 * 	File moved here from mips/PMAX since it tries to be generic;
 * 	it is used on the PMAX and the Vax3100.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.6  91/05/14  17:32:30  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/02/05  17:47:09  mrt
 * 	Added author notices
 * 	[91/02/04  11:21:07  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:24:40  mrt]
 * 
 * Revision 2.4  90/12/05  23:50:20  af
 * 
 * 
 * Revision 2.3  90/12/05  20:49:31  af
 * 	Made reentrant.
 * 	New flag defs for new TC autoconf code.  Defs for exported funcs.
 * 	[90/12/03  23:01:22  af]
 * 
 * Revision 2.2  90/08/07  22:21:08  rpd
 * 
 * 	Created.
 * 	[90/04/18            af]
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
/*
 */
/*
 *	File: busses.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	4/90
 *
 *	Structures used by configuration routines to
 *	explore a given bus structure.
 */

#ifndef	_CHIPS_BUSSES_H_
#define	_CHIPS_BUSSES_H_

#include <types.h>
#include <mach/boolean.h>

/*
 *
 * This is mildly modeled after the Unibus on Vaxen,
 * one of the most complicated bus structures.
 * Therefore, let's hope this can be done once and forall.
 *
 * At the bottom level there is a "bus_device", which
 * might exist in isolation (e.g. a clock on the CPU
 * board) or be a standard component of an architecture
 * (e.g. the bitmap display on some workstations).
 *
 * Disk devices and communication lines support multiple
 * units, hence the "bus_driver" structure which is more
 * flexible and allows probing and dynamic configuration
 * of the number and type of attached devices.
 *
 * At the top level there is a "bus_ctlr" structure, used
 * in systems where the I/O bus(ses) are separate from
 * the memory bus(ses), and/or when memory boards can be
 * added to the main bus (and they must be config-ed
 * and/or can interrupt the processor for ECC errors).
 *
 * The autoconfiguration process typically starts at
 * the top level and walks down tables that are
 * defined either in a generic file or are specially
 * created by config(1).
 */

typedef void (*intr_t)(void);
#define NO_INTR		    (intr_t) 0

typedef int (*probe_t)(caddr_t port, void * ui);

/*
 * Per-controller structure.
 */
struct bus_ctlr {
	struct bus_driver  *driver;	/* myself, as a device */
	char		   *name;	/* readability */
	int		    unit;	/* index in driver */
	/* interrupt handler(s) */
	intr_t		    intr;
	caddr_t		    address;	/* device virtual address */
	int		    am;		/* address modifier */
	caddr_t		    phys_address;/* device phys address */
	char		    adaptor;	/* slot where found */
	char		    alive;	/* probed successfully */
	char		    flags;	/* any special conditions */
	caddr_t		    sysdep;	/* On some systems, queue of
					 * operations in-progress */
	long		    sysdep1;	/* System dependent */
};

/*
 * Per-``device'' structure
 */
struct bus_device {
	struct bus_driver  *driver;	/* autoconf info */
	char		   *name;	/* my name */
	int		    unit;
	intr_t		    intr;
	caddr_t		    address;	/* device address */
	int		    am;		/* address modifier */
	caddr_t		    phys_address;/* device phys address */
	char		    adaptor;
	char		    alive;
	char		    ctlr;
	char		    slave;
	int		    flags;
	struct bus_ctlr    *mi;		/* backpointer to controller */
	struct bus_device  *next;	/* optional chaining */
	caddr_t		    sysdep;	/* System dependent */
	long		    sysdep1;	/* System dependent */
};

/*
 * General flag definitions
 */
#define BUS_INTR_B4_PROBE  0x01		/* enable interrupts before probe */
#define BUS_INTR_DISABLED  0x02		/* ignore all interrupts */
#define	BUS_CTLR	   0x04		/* descriptor for a bus adaptor */
#define BUS_XCLU	   0x80		/* want exclusive use of bdp's */

/*
 * Per-driver structure.
 *
 * Each bus driver defines entries for a set of routines
 * that are used at boot time by the configuration program.
 */
struct bus_driver {
	/* see if the driver is there */
	int	(*probe)(
		caddr_t	port, 
		void *ui);		/* either a bus_device or a bus_ctlr */
	/* see if any slave is there */
	int	(*slave)(
		struct bus_device *device, 
		caddr_t	virt);	
	/* setup driver after probe */
	void	(*attach)(
		struct bus_device *device);	
	/* start transfer */
	int	(*dgo)(void); 	
	caddr_t	*addr;			/* device csr addresses */
	char	*dname;			/* name of a device */
	struct	bus_device **dinfo;	/* backpointers to init structs */
	char	*mname;			/* name of a controller */
	struct	bus_ctlr **minfo;	/* backpointers to init structs */
	int	flags;
};

#define NO_PROBE	(int (*)(caddr_t port, void *ui)) 0
#define NO_SLAVE	(int (*)(struct bus_device *device, caddr_t virt)) 0
#define NO_ATTACH	(int (*)(struct bus_device *device)) 0
#define NO_DGO		(int (*)(void)) 0

#ifdef	MACH_KERNEL
extern struct bus_ctlr		bus_master_init[];
extern struct bus_device	bus_device_init[];
extern int 			nulldev(void);

extern void 	take_irq(int pic,
			 int unit,
			 int spl,
			 intr_t intr);

extern void 	reset_irq(int 		pic,
			  int 		*unit,
			  int		*spl,
			  intr_t 	*intr);

/* given a name of a bus controller, explore the interface 
   with the probe, slave, attach methods */
extern boolean_t configure_bus_master(
		char		*name,
		caddr_t		 virt,
		caddr_t		 phys,
		int		 adpt_no,
		char		*bus_name);

/* given a name of a bus device, explore the interface 
   with the probe, slave, attach methods */
extern boolean_t configure_bus_device(
		char		*name,
		caddr_t		 virt,
		caddr_t		 phys,
		int		 adpt_no,
		char		*bus_name);

extern void		take_ctlr_irq(
				struct bus_ctlr *ctlr);

#endif	/* MACH_KERNEL */
#endif	/* _CHIPS_BUSSES_H_ */
