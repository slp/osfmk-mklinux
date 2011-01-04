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
 * Revision 2.7.3.1  92/03/03  16:17:24  jeffreyh
 * 	Changes from TRUNK
 * 	[92/02/26  11:38:36  jeffreyh]
 * 
 * Revision 2.8  92/01/03  20:11:01  dbg
 * 	Add 'forbidden' list.  For DOS compatibility, allow any port not
 * 	on this list.
 * 	[92/01/02            dbg]
 * 
 * 	Add ports 70 and 71 for DOS programs (XXX).
 * 	[91/12/18            dbg]
 * 
 * 	Use io_port sets.
 * 	[91/11/09            dbg]
 * 
 * Revision 2.7  91/05/14  16:26:21  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/02/05  17:18:43  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:44:59  mrt]
 * 
 * Revision 2.5  90/11/26  14:50:15  rvb
 * 	jsb bet me to XMK34, sigh ...
 * 	[90/11/26            rvb]
 * 	Synched 2.5 & 3.0 at I386q (r2.1.1.3) & XMK35 (r2.5)
 * 	[90/11/15            rvb]
 * 
 * Revision 2.1.1.2  90/03/28  08:30:58  rvb
 * 	Add ioplmmap to map 0x0a0000 - 0x100000
 * 	[90/03/25            rvb]
 * 
 * Revision 2.1.1.1  90/02/01  13:36:55  rvb
 * 	Allow io privileges to the process.
 * 	[90/02/01            rvb]
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

#include <types.h>

#include <mach/vm_prot.h>
#include <mach/i386/vm_types.h>
#include <mach/i386/vm_param.h>

#include <kern/thread.h>

#include <ipc/ipc_port.h>

#include <device/io_req.h>
#include <device/dev_hdr.h>

#include <i386/io_port.h>
#include <i386/eflags.h>
#include <i386/pit.h>
#include <i386/pio.h>
#include <i386/iopb_entries.h>
#include <i386/AT386/iopl_entries.h>

/* Forward */

extern boolean_t	iopl_port_forbidden(
					int	io_port);

/*
 * IOPL device.
 */
ipc_port_t	iopl_device_port = IP_NULL;
device_t	iopl_device = 0;

/*
 * Ports that we allow access to.
 */
io_reg_t iopl_port_list[] = {
	/* timer 0 */
	0x40,
	/* timer 2 */
	0x42,
	/* speaker output */
	0x61,
	/* configuration RAM */
	0x70, 0x71,			/* XXX should not need! */
	/* game port */
	0x201,
	/* sound board */
	0x220, 0x221, 0x222, 0x223, 0x224, 0x225, 0x226, 0x227,
	0x228, 0x229, 0x22a, 0x22b, 0x22c, 0x22d, 0x22e, 0x22f,
	/* printer */
	0x378, 0x379, 0x37a,
	/* ega/vga */
	0x3b0, 0x3b1, 0x3b2, 0x3b3, 0x3b4, 0x3b5, 0x3b6, 0x3b7,
	0x3b8, 0x3b9, 0x3ba, 0x3bb, 0x3bc, 0x3bd, 0x3be, 0x3bf,
	0x3c0, 0x3c1, 0x3c2, 0x3c3, 0x3c4, 0x3c5, 0x3c6, 0x3c7,
	0x3c8, 0x3c9, 0x3ca, 0x3cb, 0x3cc, 0x3cd, 0x3ce, 0x3cf,
	0x3d0, 0x3d1, 0x3d2, 0x3d3, 0x3d4, 0x3d5, 0x3d6, 0x3d7,
	0x3d8, 0x3d9, 0x3da, 0x3db, 0x3dc, 0x3dd, 0x3de, 0x3df,
	/* PCI registers */
	0xcf8, 0xcf9, 0xcfa, 0xcfb, 0xcfc, 0xcfd,
	/* ET4000/W32 registers */
	0x217a, 0x217b,
	/* S3 registers (UGH -- forces very long tss bitmap): */
	0x42e8, 0x42e9,
	0x4ae8, 0x4ae9,
	0x82e8, 0x82e9,
	0x86e8, 0x86e9,
	0x8ae8, 0x8ae9,
	0x8ee8, 0x8ee9,
	0x92e8, 0x92e9,
	0x96e8, 0x96e9,
	0x9ae8, 0x9ae9,
	0x9ee8, 0x9ee9,
	0xa2e8, 0xa2e9, 0xa2ea, 0xa2eb,
	0xa6e8, 0xa6e9, 0xa6ea, 0xa6eb,
	0xaae8, 0xaae9, 0xaaea, 0xaaeb,
	0xaee8, 0xaee9, 0xaeea, 0xaeeb,
	0xb2e8, 0xb2e9, 0xb2ea, 0xb2eb,
	0xb6e8, 0xb6e9,
	0xbae8, 0xbae9,
	0xbee8, 0xbee9,
	0xe2e8, 0xe2e9,
	0xe2ea, 0xe2eb, 
	/* end of list */
	IO_REG_NULL
};

io_return_t
ioplopen(
	dev_t		dev,
	dev_flavor_t	flag,
	io_req_t	ior)
{
	iopl_device_port = ior->io_device->port;
	iopl_device = ior->io_device;

	io_port_create(iopl_device, iopl_port_list);
	return (D_SUCCESS);
}

void
ioplclose(
	dev_t		dev)
{
	io_port_destroy(iopl_device);
	iopl_device_port = IP_NULL;
	iopl_device = 0;
}

int iopl_all = 0;

vm_offset_t
ioplmmap(
	dev_t		dev,
	vm_offset_t	off,
	vm_prot_t	prot)
{
    if (iopl_all)
    {
	if (off == 0)
	    return 0;
	else if (off < 0xa0000 || off > 0x100000)
	    return -1;
	else
	    return i386_btop(off);
    }

    /* Get page frame number for the page to be mapped. */

    return(i386_btop(0xa0000 + off));
}

/*
 * For DOS compatibility, it's easier to list the ports we don't
 * allow access to.
 */
#define	IOPL_PORTS_USED_MAX	256
io_reg_t iopl_ports_used[IOPL_PORTS_USED_MAX] = {
	IO_REG_NULL
};

boolean_t
iopl_port_forbidden(
	int	io_port)
{
	int	i;

#if 0	/* we only read from these... it should be OK */

	if (io_port <= 0xff)
	    return TRUE;	/* system ports.  42,61,70,71 allowed above */

	if (io_port >= 0x130 && io_port <= 0x137)
	    return TRUE;	/* AHA disk */

	if (io_port >= 0x170 && io_port <= 0x177)
	    return TRUE;	/* HD disk */

	if (io_port >= 0x1f0 && io_port <= 0x1f7)
	    return TRUE;	/* HD disk */

	if (io_port >= 0x230 && io_port <= 0x237)
	    return TRUE;	/* AHA disk */

	if (io_port >= 0x280 && io_port <= 0x2df)
	    return TRUE;	/* 8390 network */

	if (io_port >= 0x300 && io_port <= 0x31f)
	    return TRUE;	/* 8390 network */

	if (io_port >= 0x330 && io_port <= 0x337)
	    return TRUE;	/* AHA disk */

	if (io_port >= 0x370 && io_port <= 0x377)
	    return TRUE;	/* FD disk */

	if (io_port >= 0x3f0 && io_port <= 0x3f7)
	    return TRUE;	/* FD disk */

#endif

	/*
	 * Must be OK, as far as we know...
	 * Record the port in the list, for
	 * curiosity seekers.
	 */
	for (i = 0; i < IOPL_PORTS_USED_MAX; i++) {
	    if (iopl_ports_used[i] == io_port)
		break;			/* in list */
	    if (iopl_ports_used[i] == IO_REG_NULL) {
		iopl_ports_used[i] = io_port;
		iopl_ports_used[i+1] = IO_REG_NULL;
		break;
	    }
	}

	return FALSE;
}

/*
 * Emulate certain IO instructions for the AT bus.
 *
 * We emulate writes to the timer control port, 43.
 * Only writes to timer 2 are allowed.
 *
 * Temporarily, we allow reads of any IO port,
 * but ONLY if the thread has the IOPL device mapped
 * and is not in V86 mode.
 *
 * This is a HACK and MUST go away when the DOS emulator
 * emulates these IO ports, or when we decide that
 * the DOS world can get access to all uncommitted IO
 * ports.  In that case, config() should remove the IO
 * ports for devices it exists from the allowable list.
 */

boolean_t
iopl_emulate(
	struct i386_saved_state	*regs,
	int			opcode,
	int			io_port)
{
	iopb_tss_t	iopb;

	iopb = current_act()->mact.pcb->ims.io_tss;
	if (iopb == 0)
	    return FALSE;		/* no IO mapped */

	/*
	 * Handle outb to the timer control port,
	 * for timer 2 only.
	 */
	if (io_port == PITCTL_PORT) {

	    int	io_byte = regs->eax & 0xff;

	    if (((iopb->bitmap[PITCTR2_PORT >> 3] & (1 << (PITCTR2_PORT & 0x7)))
		== 0)	/* allowed */
	     && (opcode == 0xe6 || opcode == 0xee)	/* outb */
	     && (io_byte & 0xc0) == 0x80)		/* timer 2 */
	    {
		outb(io_port, io_byte);
		return TRUE;
	    }
	    return FALSE;	/* invalid IO to port 42 */
	}

	/*
	 * If the thread has the IOPL device mapped, and
	 * the io port is not on the 'forbidden' list, allow
	 * reads from it.  Reject writes.
	 *
	 * Don`t do this for V86 mode threads
	 * (hack for DOS emulator XXX!)
	 */
	if (!(regs->efl & EFL_VM) &&
	    iopb_check_mapping(current_thread(), iopl_device) &&
	    !iopl_port_forbidden(io_port))
	{
	    /*
	     * handle inb, inw, inl
	     */
	    switch (opcode) {
		case 0xE4:		/* inb imm */
		case 0xEC:		/* inb dx */
		    regs->eax = (regs->eax & 0xffffff00)
				| inb(io_port);
		    return TRUE;

		case 0x66E5:		/* inw imm */
		case 0x66ED:		/* inw dx */
		    regs->eax = (regs->eax & 0xffff0000)
				| inw(io_port);
		    return TRUE;

		case 0xE5:		/* inl imm */
		case 0xED:		/* inl dx */
		    regs->eax = inl(io_port);
		    return TRUE;

		default:
		    return FALSE;	/* OUT not allowed */
	    }
	}

	/*
	 * Not OK.
	 */
	return FALSE;
}

