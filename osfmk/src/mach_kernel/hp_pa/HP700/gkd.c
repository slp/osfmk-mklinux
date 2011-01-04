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
 * Copyright (c) 1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: gkd.c 1.5 94/12/14$
 */
#include "gkd.h"



#if NGKD > 0

/*
 *  Driver for IBM PS2 compatible keyboard/mouse driver
 */
#include <types.h>
#include <sys/ioctl.h>
#include <device/tty.h>
#include <device/cirbuf.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <device/ds_routines.h>
#include <hp_pa/HP700/device.h>
#include <hp_pa/HP700/gkdreg.h>
#include <hp_pa/HP700/gkdvar.h>
#include <hp_pa/HP700/gkdhpux.h>
#include <hp_pa/HP700/itevar.h>
#include <hp_pa/HP700/kbdmap.h>
#include <kern/spl.h>
#include <machine/asp.h>
#include <machine/cpu.h>
#include <machine/machparam.h>

#include <hp_pa/trap.h>
#include <kern/misc_protos.h>

/* Forwards */
int ps2_boot_ident(struct gkd_dev *);
int ps2_sendbyte_with_ack(struct gkd_reg*, unsigned char);
int ps2_readbyte_retrans(struct gkd_reg*, unsigned char *, unsigned, int);
int ps2_writebyte_timeout(struct gkd_reg*, unsigned char, unsigned);
int ps2_readbyte_timeout(struct gkd_reg *, unsigned char *, unsigned int);

void gkd_init(struct gkd_dev *);
int  gkdopen(dev_t, int, io_req_t);
void gkdclose(dev_t);
int  gkdread(dev_t, io_req_t);
boolean_t gkdread_done(io_req_t);
int  gkdgetstat(dev_t, int, dev_status_t, unsigned int*);
int  gkdsetstat(dev_t, int, dev_status_t, unsigned int);
vm_offset_t gkdmap(dev_t, vm_offset_t, int);
void gkd_kbd_led(struct gkd_dev *, unsigned char);
void gkdintr(int);
void gkd_kbd_intr(struct gkd_dev *, struct gkd_reg *);
void gkd_mouse_intr(struct gkd_dev *, struct gkd_reg *);
int gkd_process_special(struct gkd_dev *, unsigned char *);
void gkd_config(struct gkd_dev *);
int gkd_send_cmd(struct gkd_dev *, unsigned char *);
void gkd_issue_cmd(struct gkd_dev *, unsigned char *);
void gkd_write_cmd(struct gkd_reg *, unsigned);
int gkdioctl(dev_t, int, caddr_t, int);
void gkd_continue_cmd(struct gkd_dev *, unsigned char);
int gkd_give_cmd(struct gkd_dev *, unsigned char *);

/* Values for PS2_PORTSTAT first return byte */

#define PS2_KEYBD       0
#define PS2_MOUSE       1

/* Values for PS2_PORTSTAT second return byte */

#define INTERFACE_HAS_ITE       0x01
#define PORT_HAS_FIRST_KEYBD	0x02
#define PORT_HAS_FIRST_MOUSE	0x04

/*
 * table describing software view of the hardware
 * Note, at this time, we only support 1 ps2 interface 
 * with exactly one mouse and one keyboard
 */
struct	gkd_dev	gkd_device[GKD_MAX_PORTS];
static  int  gkd_kbd_index, gkd_mouse_index; 

/*
 * Commands that we can issue to devices
 */
static unsigned char gkd_kbd_set_leds[] = { 2, 0, GKD_KBD_LEDS, 0};
static unsigned char gkd_mouse_stream[] = { 1, 0, 0xEA};
static unsigned char gkd_mouse_resolution[] = { 2, 0, 0xE8, 0};
static unsigned char gkd_device_ident[] = { 1, 2, 0xF2};
static unsigned char gkd_device_disable[] = { 1, 0, 0xF5};
static unsigned char gkd_device_enable[] = { 1, 0, 0xF4};
static unsigned char gkd_device_reset[] = { 1, 0, 0xFF};
static unsigned char gkd_device_rate[] = { 2, 0, 0xF3, 0};
static unsigned char gkd_device_setdefault[] = { 1, 0, 0xF6 };
static unsigned char gkd_kbd_scancode[] = { 2, 0, 0xF0, 0};
static unsigned char gkd_kbd_all_tmat_mkbrk[] = { 1, 0, 0xFA };
static unsigned char gkd_kbd_all_tmat[] = { 1, 0, 0xF7 };
static unsigned char gkd_kbd_all_mkbrk[] = { 1, 0, 0xF8 };
static unsigned char gkd_kbd_all_mk[] = { 1, 0, 0xF9 };
static unsigned char gkd_mouse_promptmode[] = { 1, 0, 0xF0 };

/*
 * Probe to see if we have a PS/2 style keyboard and mouse
 */
int
gkdprobe(struct hp_device *hd)
{
	struct gkd_reg *regs = (struct gkd_reg *) hd->hp_addr;

	if (regs->id_reset == 0 && gkd_next(regs)->id_reset ==1)
		return(1);

	return (0);
}

/*
 * Attach the keyboard
 */
void
gkdattach(struct hp_device *hd)
{
        extern int asp2procibit[];
	int ibit;

	gkd_device[0].regs =	(struct gkd_reg *) hd->hp_addr;
	gkd_device[1].regs =	(struct gkd_reg *) gkd_next(hd->hp_addr);

	gkd_kbd_index = ps2_boot_ident(&gkd_device[0]);
	gkd_mouse_index = ps2_boot_ident(&gkd_device[1]);

	if(gkd_kbd_index == -1 || gkd_mouse_index == -1)
		return;

	gkd_init(gkd_device);
	gkd_config(&gkd_device[gkd_kbd_index]);

	ibit = aspitab(INT_PS2, SPLGKD, gkdintr, 0);
}

/*
 * initialize this interface
 */
void
gkd_init(struct gkd_dev *dev)
{	
	int nport;
	extern int havekeyboard;

	for (nport = 0; nport < GKD_MAX_PORTS; nport++ ) {

		if (dev[nport].flags & F_GKD_INITED)
			continue;
		
		/*
		 * Verify the ports are correct (i.e. double 
		 *        check the probe routine)
		 */	
		if (dev[nport].regs->id_reset != nport ) {
			/* panic here eventually */
			printf("yikes! something wrong in gkd_init\n");
		}

		dev[nport].flags |= F_GKD_INITED;
	}
	havekeyboard = 2;	
}

/*
 * Open the keyboard or mouse
 */
int
gkdopen(
	dev_t dev,
	int flag,
	io_req_t i)
{
	struct gkd_dev *dptr;
	int m;

	m = minor(dev);

	if (m >= GKD_MAX_PORTS)
		return(ENXIO);	/* no such device */

	dptr = &gkd_device[m];
	if ((dptr->flags & F_GKD_INITED) == 0)
		return(ENXIO);	/* device wasn't attached */

	if (dptr->flags & F_GKD_OPEN)
		return(EBUSY);	/* device already opened */

	/*
	 * In order to do as little damage as possible to the driver,
	 * we use tty queues and operations for the read interface.
	 * Since the select interface issues read calls even for the
	 * queue interface, we have to set this up now.
	 */
	if ((dptr->tty.t_state & TS_ISOPEN) == 0) {
		ttychars(&dptr->tty);
		dptr->tty.t_dev = dev;
		dptr->tty.t_state |= (TS_ISOPEN | TS_CARR_ON);
	}

	if (flag & D_NODELAY)
		dptr->flags |= F_GKD_NOBLOCK;

	dptr->flags |= F_GKD_OPEN;

	dptr->regs->id_reset = 0;     /* reset */
	gkd_enable(dptr->regs);	/* enable the device */
	return(0);		/* should be all set */
}
 
void
gkdclose(dev_t dev)
{
	struct gkd_dev *dptr;
	int m;
	spl_t s;
	struct tty *tp;

	m = minor(dev);

	if (m >= GKD_MAX_PORTS)
		return;

	dptr = &gkd_device[m];
	if ((dptr->flags & F_GKD_INITED) == 0)
		return;

	tp = &dptr->tty;

	s = splgkd();
	simple_lock(&tp->t_lock);
	tp->t_state &= ~TS_CARR_ON;
	ttyclose(tp);
	ndflush(&tp->t_inq, tp->t_inq.c_cc);
	simple_unlock(&tp->t_lock);
	splx(s);

	dptr->flags &= ~ (F_GKD_OPEN | F_GKD_NOBLOCK); /* shut off open flag */
	
	gkd_disable(dptr->regs);     /* shut off device */

	if (m == gkd_kbd_index)
		kbdenable(0); /* ITE console needs this */
}

extern void queue_delayed_reply(queue_t, io_req_t, boolean_t (*)(io_req_t));

int
gkdread(
	dev_t dev,
	register io_req_t ior)
{
	struct gkd_dev *dptr;
	int m;
	struct tty *tp;
	kern_return_t rc;
	spl_t s;

	m = minor(dev);
	if (m >= GKD_MAX_PORTS)
		return(ENXIO);	/* no such device */

	dptr = &gkd_device[m];
	tp = &dptr->tty;

	/*
	 * Allocate memory for read buffer.
	 */
	rc = device_read_alloc(ior, (vm_size_t)ior->io_count);
	if (rc != KERN_SUCCESS)
		return (rc);

	s = splgkd();
	simple_lock(&tp->t_lock);

	if (tp->t_inq.c_cc == 0) {
		if (ior->io_mode & D_NOWAIT) {
			rc = D_WOULD_BLOCK;
		} else {
			ior->io_dev_ptr = (char *)dptr;
			queue_delayed_reply(&tp->t_delayed_read, ior, gkdread_done);
			rc = D_IO_QUEUED;
		}
		simple_unlock(&tp->t_lock);
		splx(s);
		return rc;
	}
	
	ior->io_residual = ior->io_count -
		q_to_b(&tp->t_inq, ior->io_data, (int)ior->io_count);

	simple_unlock(&tp->t_lock);
	splx(s);
	return (D_SUCCESS);
}
 
boolean_t
gkdread_done(io_req_t ior)
{
	struct gkd_dev *dptr = (struct gkd_dev *)ior->io_dev_ptr;
	struct tty *tp = &dptr->tty;
	spl_t s;

	s = splgkd();
	simple_lock(&tp->t_lock);

	if (tp->t_inq.c_cc <= 0) {
		queue_delayed_reply(&tp->t_delayed_read, ior,
				    gkdread_done);
		simple_unlock(&tp->t_lock);
		splx(s);
		return(FALSE);
	}
	ior->io_residual = ior->io_count -
		q_to_b(&tp->t_inq, ior->io_data, (int)ior->io_count);
	
	simple_unlock(&tp->t_lock);
	splx(s);

	(void) ds_read_done(ior);
	return (TRUE);
}

/*
 * Map the registers.  We assume that one page is enough.
 */
vm_offset_t
gkdmap(dev_t dev, vm_offset_t off, int prot)
{
	printf("Warning: gkdmap not implemented.\n");

	return (vm_offset_t)0;
}

/*
 * Interrupt routine -- respond to keyboard/mouse interrupts!
 */ 
void
gkdintr(int unit)
{
	struct gkd_dev *dev;
	struct gkd_reg *regs;

	dev = &gkd_device[PS2_KEYBD];
	regs = dev->regs;
	
	if(regs->status & (PS2_CMPINT | PS2_RBNE)) 
		gkd_kbd_intr(dev, regs);
	
	dev = &gkd_device[PS2_MOUSE];
	regs = dev->regs;

	if(regs->status & (PS2_CMPINT | PS2_RBNE)) 
		gkd_mouse_intr(dev, regs);
}

/*
 * Handle a keyboard interrupt
 *
 */
void
gkd_kbd_intr(struct gkd_dev *dev, struct gkd_reg *regs)
{
 	unsigned char scancode, status;
	struct tty *tp;
	spl_t s;

	while ( regs->status & PS2_RBNE ) {

		scancode = regs->data;	/* get the data */
		DELAY(30000);

		/* if we're processing a command this takes priority */
		if (dev->in_cmd) {
			(dev->cmd_func)(dev, scancode);
			continue;
		}

		if (dev->flags & F_GKD_OPEN) {
			/*
			 * A user program has the buffer open,
			 * default to that
			 */
			
			tp = &dev->tty;

			s = splgkd();
			simple_lock(&tp->t_lock);
			(void) b_to_q((char*)&scancode, 1, &tp->t_inq);
			tty_queue_completion(&tp->t_delayed_read);
			simple_unlock(&tp->t_lock);
			splx(s);
			continue;
		}

		status = GKBD_KEY; /* assume just a key pressed */

		/*
		 * We should be on the ITE console, process the keys
		 */
		
		if ((scancode == GKD_SPEC_KEY1) || (scancode == GKD_SPEC_KEY2) || dev->spec) {
			if (gkd_process_special(dev, &scancode) != GKD_DONE)
				continue;
			if (!scancode)
				continue; /* nothing more to do */
		}

		if (scancode == GKD_KEY_UP) {
			dev->released = 1;
			continue;
		} 

		switch (scancode) {
		case GKD_CAPS: 
			if (dev->released) {
				dev->released = 0;
				continue;
			}
			/* caps lock was pressed -- convert */
			scancode = KBD_CAPSLOCK;
			if (dev->flags & F_GKD_CAPS) {
				dev->flags &= ~F_GKD_CAPS;
				gkd_kbd_led(dev, dev->leds &= ~KBD_CAP_LOCK);
			} else {
				dev->flags |= F_GKD_CAPS;
				gkd_kbd_led(dev, dev->leds |= KBD_CAP_LOCK);
			}
			break;

		case GKD_ALT:
			if (dev->released) {
				dev->flags &= ~F_GKD_ALT;
				scancode = KBD_EXT_LEFT_UP;
			} else {
				dev->flags |= F_GKD_ALT;
				scancode = KBD_EXT_LEFT_DOWN;
			}
			dev->released = 0; 
			continue;

		case GKD_CTRL:
			if (dev->released)
				dev->flags &= ~F_GKD_CTRL;
			else
				dev->flags |=  F_GKD_CTRL;
			dev->released = 0;
			continue;

		case GKD_NUM_LCK:
			if (dev->released) {
				dev->released = 0;
				continue;
			}
			/* num lock was pressed -- what do we do?? */
			if (dev->flags & F_GKD_NUMLK) {
				dev->flags &= ~F_GKD_NUMLK;
				gkd_kbd_led(dev, dev->leds &= ~KBD_NUM_LOCK);
			} else {
				dev->flags |= F_GKD_NUMLK;
				gkd_kbd_led(dev, dev->leds |= KBD_NUM_LOCK);
			}
			continue;

		case GKD_SCROLL_LCK:
			if (dev->released) {
				dev->released = 0;
				continue;
			}
			/* scroll lock was pressed -- what do we do?? */
			if (dev->flags & F_GKD_SCRLK) {
				dev->flags &= ~F_GKD_SCRLK;
				gkd_kbd_led(dev, dev->leds &= ~KBD_SCRL_LOCK);
			} else {
				dev->flags |= F_GKD_SCRLK;
				gkd_kbd_led(dev, dev->leds |= KBD_SCRL_LOCK);
			}
			continue;
			
		case GKD_RIGHT_SHIFT:
		case GKD_LEFT_SHIFT:
			if (dev->released)
				dev->flags &= ~F_GKD_SHIFT;
			else
				dev->flags |=  F_GKD_SHIFT;
			dev->released = 0;
			continue;

		default:
			switch (dev->flags & F_KBD_SCC) {
			case F_GKD_SHIFT:
				status = GKBD_SHIFT;
				break;
			case F_GKD_CTRL:
				status = GKBD_CTRL;
				break;
			case F_GKD_CTRL|F_GKD_SHIFT:
				status = GKBD_CTRLSHIFT;
				break;
			}
		}
			
		if (dev->released) {
			dev->released = 0;
		} else {
			if (scancode == GKD_KEY_UP) 
				scancode = 0;
			gkditefilter(dev, status, scancode);
		}
	}
}

/*
 * Handle a mouse interrupt
 */
void
gkd_mouse_intr(struct gkd_dev *dev, struct gkd_reg *regs)
{
 	unsigned char data, status;
	spl_t s;

	while ( regs->status & PS2_RBNE ) {
		data = regs->data;	/* get the data */

		/* if we're processing a command this takes priority */
		if (dev->in_cmd) {
			(dev->cmd_func)(dev, data);
			continue;
		}

		if ((dev->flags & F_GKD_OPEN) == 0)
			continue; /* no one is reading the mouse */

		(void)b_to_q((char*)&data, 1, &dev->tty.t_inq);
		if (dev->flags & F_GKD_ASLEEP) {
			dev->flags &= ~F_GKD_ASLEEP;
			thread_wakeup((event_t)dev);
		}

		s = splgkd();
		tty_queue_completion(&dev->tty.t_delayed_read);
		splx(s);
	}
}

/*
 * Handle the special keys from the keyboard (ala keyboard arrows etc )
 */
int
gkd_process_special(
	struct gkd_dev *dev,
	unsigned char *scancode)
{
	unsigned char c = *scancode;


	if (!dev->spec) {
		dev->spec = c;
		return GKD_NEED_MORE;
	}

	if (c == GKD_KEY_UP) {
		dev->released = 1;
		return GKD_NEED_MORE;
	}

	/*
	 * Looks like high key numbers are single codes
	 * need to watch out for SysReq however..
	 */
	if (c != 0x12) {

		if ((dev->spec == GKD_SPEC_KEY2) && (c != 0x77)) {
			/* processing of Pause/Break key is not done yet */
			return GKD_NEED_MORE;
		} else if (dev->spec == GKD_SPEC_KEY2) {
			*scancode = 0;
			if (dev->released) dev->released = 0;
		} else if (dev->spec == GKD_SPEC_KEY3) {
			if (c == GKD_SPEC_KEY1)
				return GKD_NEED_MORE;
			*scancode = 0;
			if (dev->released) dev->released = 0;
		}

		dev->spec = 0;
		return GKD_DONE;
	} else {
		/* yikes!!  we have a sysreq -- what to do? */

		dev->spec == GKD_SPEC_KEY3;
		return GKD_NEED_MORE;
	}
}

/*
 * check up on a numlock scancode and return the appropriate value
 */
void
gkd_chk_numlock(char * code, char c)
{

        static char gkd_num_lck_table[] = {'1','\0','4','7','\0','\0','\0',
                                           '0','.','2','5','6','8','\0','\0',
                                           '\0','+','3','-','*','9'};
        /*
         * check to see if numlock character and return if so
         */

        if (c < 0x69 || c > 0x7d)
                return;         /* nope, it ain't */

        *code = gkd_num_lck_table[ c - 0x69];
}


/*
 * Config keyboard device
 */
void
gkd_config(struct gkd_dev *kbd)
{
	struct kbdmap *km;
	int  lang_code = KBD_US; /* XXX fix this someday */
	spl_t  s;
	extern struct kbdmap gkbd_map[];

	s = splgkd();
	for (km = gkbd_map; km->kbd_code; km++) {
		if (km->kbd_code == lang_code) {
			kbd_keymap = km->kbd_keymap;
			kbd_shiftmap = km->kbd_shiftmap;
			kbd_ctrlmap = km->kbd_ctrlmap;
			kbd_ctrlshiftmap = km->kbd_ctrlshiftmap;
			kbd_stringmap = km->kbd_stringmap;
		}
	}
	splx(s);
}

/*
 * Handle keyboard commands to turn on/off the leds
 */
void
gkd_kbd_led(
	struct gkd_dev *dev,
	unsigned char val)
{

	gkd_kbd_set_leds[3] = val; /* XXX this is known to be here! */
	gkd_send_cmd(dev, gkd_kbd_set_leds);
#ifdef no

	if (!dev->in_cmd) {
		/* 
		 * We're starting the command, setup the state
		 */
		dev->in_cmd = 1; /* commands takes priority over key strokes */
		dev->cmd_state = 0; /* initial state */
		dev->cmd_func = gkd_kbd_led;
		dev->cmd_buf[0] = GKD_KBD_LEDS;
		dev->cmd_buf[1] = val;

		/*
		 * Send the first byte
		 */
		gkd_write_cmd(dev->regs, dev->cmd_buf[0]);
		return;
	}

	/*
	 * in the middle of processing this command
	 */
	switch (dev->cmd_state) {
		/* 
		 * State 0: receive ack, go to state 1
                 *          else resend command, loop in state 0
		 */
	case 0:		
		if (val == GKD_DEV_ACK) {
			/* command was acked, go on */
			dev->cmd_state = 1;
			gkd_write_cmd(dev->regs, dev->cmd_buf[1]);
		} else if (val == GKD_DEV_RETRY) {
			/* command needs to be sent again, do it */
			gkd_write_cmd(dev->regs, dev->cmd_buf[1]);
			return;
		} else {
			printf("gkd ack:unknown val %d in state %d\n", val, 0);
		}
		break;
	case 1:
		/*
		 * State 1: receive ack, finished
		 *          else, resend data arg, loop in state 1
		 */
		if (val == GKD_DEV_ACK) {
			/* val was acked, we're done */
			dev->in_cmd = 0; /* no longer processing */
			return;
		} else if (val == GKD_DEV_RETRY) {
			/* command needs to be sent again, do it */
			gkd_write_cmd(dev->regs, dev->cmd_buf[1]);
			return;
		} else {
			printf("gkd ack:unknown val %d in state %d\n", val, 1);
		}
	}
#endif
}

/*
 * Send a command to the specified device
 */
int
gkd_send_cmd(
	struct gkd_dev *dev,
	unsigned char *cmd2send)
{
	if (dev->in_cmd)	/* we can only do one command at a time */
		return(-1);

	/* 
	 * We're starting the command, setup the state
	 */
	dev->in_cmd = 1; /* commands take priority over key strokes */
	dev->cmd_state = 0; /* initial state */
	dev->cmd_func = gkd_continue_cmd;
	dev->cmd_ptr = cmd2send;
	dev->cmd_serr = dev->cmd_terr = dev->cmd_nread = dev->cmd_nindx = 0;

	/*
	 * Send the first byte
	 */
	gkd_write_cmd(dev->regs, GKD_GSB(dev));
	return 0;
}

/*
 * continue processing a command
 */
void
gkd_continue_cmd(
		 struct gkd_dev *dev,
		 unsigned char val)
{
	/*
	 *  Assume positive -- the command accepted okay
	 */
	if (val == GKD_DEV_ACK) {
		/* command was acked, go on */
		dev->cmd_state += 1;
		if (dev->cmd_state == GKD_SNUM(dev) && 
		                   dev->cmd_nindx == GKD_GSN(dev)) {
			/* we're done! */
cdone:
			dev->in_cmd = 0;
			if (dev->flags & F_GKD_CSLEEP) {
				dev->flags &= ~F_GKD_CSLEEP;
				thread_wakeup((event_t)dev->cmd_ptr);
			}
			return;
		}
		gkd_write_cmd(dev->regs, GKD_GSB(dev));
		return;
	}

	if (val == GKD_DEV_RETRY) {
		/* oh great! send the command again */
		dev->cmd_serr++;
		dev->cmd_terr++;
		if (dev->cmd_serr > 5)
			goto cdone;  /* too many errs -- bail! */
		gkd_write_cmd(dev->regs, GKD_GSB(dev));
		return;
	}

	/*
	 * anything else, check to see if we want the info otherwise
	 * silently drop
	 */
	if (GKD_GSN(dev) > dev->cmd_nindx)
		dev->cmd_buf[dev->cmd_nindx++] = val;

	/*
	 * XXX special case: IDENT command for mouse only returns 1
	 */
	if ((dev->cmd_ptr[2] == (unsigned char)0xF2) /*XXX ident command*/  && (val == 0)) {
		/* this is a mouse and we're done */
		goto cdone;
	}
		
}

/*
 * Give a command and let the interrupt routine carry it out.
 * Called from the top half of the driver 
 */
int
gkd_give_cmd(struct gkd_dev *dev,
	unsigned char *cmd)
{
	spl_t s = splgkd();

	if (gkd_send_cmd(dev, cmd)) {
		splx(s);
		return(-1);	/* this failed, no use continuing */
	}

	dev->flags |= F_GKD_CSLEEP;
	while ((dev->flags & F_GKD_CSLEEP) && dev->in_cmd)
	{
		assert_wait((event_t)cmd, TRUE); 
		thread_block((void (*)(void)) 0); 
	}

	if (dev->flags & F_GKD_CSLEEP) {
		printf("gkd_give_cmd: command completed before sleep!\n");
		dev->flags &= ~F_GKD_CSLEEP;
	}

	splx(s);
	return(0);
}

/*
 * Drive a command in "poll" mode (i.e. simulate the interrupt routine)
 */
void
gkd_issue_cmd(
	struct gkd_dev *dev,
	unsigned char *cmd)
{
	unsigned char data;
	struct gkd_reg *regs = dev->regs;
	spl_t s = splgkd();
	
	/* chuck everything to start */
	while (regs->status & PS2_RBNE) {
		data = regs->data;
		DELAY(30000);
	}

	/* call the function to kick things off */
	if (gkd_send_cmd(dev, cmd)) {
		splx(s);
		return;
	}

	while (dev->in_cmd) {
		while (!(regs->status & PS2_RBNE)) DELAY(30000);
		data = regs->data;
		gkd_continue_cmd(dev, data);
	}
	   
	/* chuck everything else */
	while (regs->status & PS2_RBNE) {
		data = regs->data;
		DELAY(30000);
	}
	splx(s);
}
	

/*
 * write this command out
 */
void
gkd_write_cmd(
	struct gkd_reg *regs,
	unsigned byte)
{	
	int max_count = 2000;	/* max tries */

	while (regs->status & PS2_TBNE) {
		if (!max_count--) {
			printf("gkd: warning! can't talk with the keyboard!\n");
			return;
		}
		DELAY(500);
	}

	regs->data = byte;
        gkd_enable(regs);	/* send the command */
}

/*
 * Mach splits ioctls into those that write data into the kernel, and
 * those that read data out of the kernel (yep, very annoying).
 * Provide some glue around hilioctl().
 */

int
gkdgetstat(
	dev_t		dev,
	int		flavor,
	dev_status_t	data,
	unsigned int	*count)		/* out */
{
	int error;

	error = gkdioctl(dev, flavor, (caddr_t)data, 0);
	*count = ((flavor >> 16) & 0x7F) / sizeof(int);
	return(error);
}

int
gkdsetstat(
	dev_t		dev,
	int		flavor,
	dev_status_t	data,
	unsigned int	count)
{
	return(gkdioctl(dev, flavor, (caddr_t)data, 0));
}

int
gkdioctl(dev_t dev, int cmd, caddr_t data, int flag)
{
	int m, err;
	struct gkd_dev *dptr;
	unsigned char *arg;

	m = minor(dev);
	err = 0;

	if (m >= GKD_MAX_PORTS) {
		printf("gkdioctl m = 0x%x\n", m);
		return(ENXIO);	/* no such device */
	}

	dptr = &gkd_device[m];
	arg = (unsigned char *)data;

	switch (cmd) {
	case PS2_SETDEFAULT:
		if (gkd_give_cmd(dptr, gkd_device_setdefault)) 
			err = EIO;
		break;

	case PS2_DISABLE:
		if (gkd_give_cmd(dptr, gkd_device_disable))
			err = EIO;
		break;

	case PS2_RESET:
		if (gkd_give_cmd(dptr, gkd_device_reset))
		         err = EIO;
		break;

	case PS2_SAMPLERATE:
		/* only for mouse */
		gkd_device_rate[3] = arg[0];
		if (gkd_give_cmd(dptr, gkd_device_rate))
			err = EIO;
		break;

	case PS2_RATEDELAY:
		/* only for keyboard */
		gkd_device_rate[3] = arg[0];
		if (gkd_give_cmd(dptr, gkd_device_rate))
			err = EIO;
		break;

	case PS2_STREAMMODE:
		/* for mouse only */
		if (gkd_give_cmd(dptr, gkd_mouse_stream))
			err = EIO;
		break;

	case PS2_RESOLUTION:
		/* for mouse only */
                gkd_mouse_resolution[3] = arg[0];
				 
		if (gkd_give_cmd(dptr, gkd_mouse_resolution))
			err = EIO;	 

		break;

	case PS2_ENABLE:
		if (gkd_give_cmd(dptr, gkd_device_enable))
			err = EIO;
	        break;

	case PS2_SCANCODE:
		/* for keyboard only */
		gkd_kbd_scancode[3] = arg[0];
		
		if (gkd_give_cmd(dptr, gkd_kbd_scancode))
			err = EIO;
		break;

	case PS2_INDICATORS:
		/* for keyboard only */

		gkd_kbd_set_leds[3] = arg[0];
		if (gkd_give_cmd(dptr, gkd_kbd_set_leds))
			err = EIO;
		break;

	case PS2_ALL_TMAT_MKBRK:
		
		if (gkd_give_cmd(dptr, gkd_kbd_all_tmat_mkbrk))
			err = EIO;
		break;
	       
	case PS2_ALL_TMAT:
		
		if (gkd_give_cmd(dptr, gkd_kbd_all_tmat))
			err = EIO;
		break;
	}

	return(err);
}

/*
 * Cooked keyboard functions for ite driver.
 */

/*
 * Ring the bell
 */
void
gkdkbdbell(int b)
{
	printf("no bell !\n");
}

/*
 * Enable the keyboard
 */
void
gkdkbdenable(int unit)
{
     spl_t s = splgkd();
     struct gkd_dev *dptr = &gkd_device[gkd_kbd_index];
	
     gkd_enable(dptr->regs);	/* enable interrupts */

     gkd_kbd_scancode[3] = 2;	/* reset scancode */
     gkd_issue_cmd(dptr, gkd_kbd_scancode);

     gkd_kbd_set_leds[3] = 0;   /* reset LEDS and state */
     dptr->flags = F_GKD_INITED;
     gkd_issue_cmd(&gkd_device[0], gkd_kbd_set_leds);

     splx(s);
}

/*
 * Disable the keyboard
 */
void
gkdkbddisable(int unit)
{
     spl_t s = splgkd();
     struct gkd_dev *dptr = &gkd_device[gkd_kbd_index];
     
     gkd_disable(dptr->regs);	/* disable the device */

     splx(s);
}

/*
 * Get a character
 */
int
gkdkbdgetc(int unit , int *statp, int wait)
{
	struct gkd_reg *kbd_regs;
	unsigned char scancode;
	spl_t s;

	kbd_regs = gkd_device[gkd_kbd_index].regs;

	s = splgkd();

	while(! (kbd_regs->status & (PS2_RBNE | PS2_CMPINT))) 
		if(! wait) {
			*statp = 0;			
			splx(s);
			return 0;
		}

	scancode = kbd_regs->data;

	if (scancode == GKD_KEY_UP) {
		while(! (kbd_regs->status & (PS2_RBNE | PS2_CMPINT))) 
			;
		scancode = kbd_regs->data;		
		*statp = 0;
		splx(s);
		return 0;
	} 

	*statp = GKBD_KEY;

	splx(s);
	return scancode;		
}

int ps2_boot_ident(struct gkd_dev *port)
{
     int status;
     int device_type;
     int x;
     unsigned char data;

     port->regs->id_reset = 0;                    /* reset, just in case */
     port->regs->control = PS2_ENBL;           /* re-enable the interface */
     DELAY(0x20000);

     port->regs->control = 0; /* Soft disable */ 
     DELAY(20000);      /* Wait 20 mSec, just in case of 10th bit */

     port->regs->id_reset = 0;           /* reset the interface */
     port->regs->control = PS2_DIAG;  /* diagnostic mode. both clk and data low */
     DELAY(20000);      /* Wait 20 mSec, just in case of 10th bit */

     status = ps2_sendbyte_with_ack(port->regs, PS2_DEFAULT_DISABLE);
     
     if( status == PS2_TIMEOUT){
	     if( port->regs->status & PS2_TBNE){  /* nothing read the byte */
		     port->regs->id_reset = 0;  /* reset the port. Clear the xmit register */
		     port->regs->control = PS2_ENBL;
		     return -1;
	     }
	     else 
		     return -1;
     }
     else if( status == PS2_NO_ACK)
	     return -1;

     if(ps2_sendbyte_with_ack(port->regs, PS2_ID) != PS2_SUCCESS)
	     return -1;

     if(ps2_readbyte_retrans(port->regs, &data, PS2_500mSec_Try, PS2_MAX_RETRANS) 
	!= PS2_SUCCESS)
	     return -1;

     if( data == PS2_MOUSE_ID){
	     device_type = PS2_MOUSE;
     }
     else if( data == PS2_KEYBOARD_ID_BYTE1){ 
	     /* might be a keyboard, check next byte */

	     if( ps2_readbyte_retrans(port->regs, &data, 1000, PS2_MAX_RETRANS)
                                                       != PS2_SUCCESS)
		     return -1;

	     if( data == PS2_KEYBOARD_ID_BYTE2)
		     device_type = PS2_KEYBD;
	     else
		     return -1;
     }
     else
	     device_type = -1;

     /* Finally, send the ps2 enable command. The mouse powers up disabled, so
	we won't enable it here. */

     if( device_type != PS2_MOUSE)
	     if( ps2_sendbyte_with_ack( port->regs, PS2_ENAB) == PS2_TIMEOUT){
		     port->regs->id_reset = 0;           /* reset the port */
		     port->regs->control = PS2_ENBL;  /* re-enable the port */
	     }

     return device_type;
}

int ps2_sendbyte_with_ack(struct gkd_reg *portregs, unsigned char cmdbyte)
{
	int out_retranscount;
	int in_retranscount;
	int skipcount;
	int read_status;
	unsigned char reply;

	out_retranscount =0;
	while( out_retranscount < PS2_MAX_RETRANS){

		/* Try to write the byte to the xmit register. Timeout if the register
		   is not empty after 500 mSec. */

		if( ps2_writebyte_timeout( portregs, cmdbyte, PS2_500mSec_Try) == PS2_FAIL)
			return( PS2_TIMEOUT);

		/* Wait up to 500 mSec for the acknowledge byte. Allow only 
		   PS2_MAX_RETRANS retramsmits if parity errors are encountered. */

		read_status = ps2_readbyte_retrans(portregs, &reply, PS2_500mSec_Try,
						   PS2_MAX_RETRANS); 

		if( read_status != PS2_SUCCESS)
			return( read_status);

		if( reply == PS2_RETRANS_REQ)
			out_retranscount++;
		else if( reply == PS2_ACK)
			return( PS2_SUCCESS);
		else
			return( PS2_NO_ACK);
	}
	return( PS2_TIMEOUT);
}

int ps2_writebyte_timeout(struct gkd_reg *portregs, unsigned char byteval, unsigned num_tries)
{
	/* Wait for the xmit reg to clear or num_tries exceeded, whichever comes 
	   first */
	
	while( num_tries--){
		if( !(portregs->status & PS2_TBNE))
			break;
		DELAY(500); /* wait 500 usec */
	}

	if( portregs->status & PS2_TBNE){
		printf("ps2_writebyte_timeout: TBNE active after 500 uSec, cmd %x\n",
			   byteval);
		return(PS2_TIMEOUT);     /* fail if transmit register still isn't empty */
	}

	portregs->data = byteval;  /* write it to the ps2 port */ 
	portregs->control = PS2_ENBL;
	return(PS2_SUCCESS);
}

int ps2_readbyte_retrans(
			 struct gkd_reg *portregs,
			 unsigned char *reply,
			 unsigned num_tries_read,
			 int num_tries_retrans)
{
	int read_status;
	
	do {
		read_status = ps2_readbyte_timeout( portregs, reply, num_tries_read);
	} while ( (read_status == PS2_WAS_RETRANS) && num_tries_retrans--);
	
	return( read_status);
} 


int ps2_readbyte_timeout(
			 struct gkd_reg *portregs,
			 unsigned char *reply,
			 unsigned int num_tries)
{
	int retranscount;
	int i;

	/* Wait for the rec reg receive a byte, a parity error, or num_tries attempts
	   are made, whichever comes first */                
	
	for( i= num_tries; i > 0; i--){
		if(portregs->status & (PS2_RBNE | PS2_TERR | PS2_PERR))
			break;
		DELAY(500); /* wait 500 usec, about half of the minimum time for the
				  ps2 protocol to xfer a byte */
	}

	if( i <= 0)
		return(PS2_TIMEOUT);

	/* When the LASI ps2 port hardware senses a parity error on incoming data,
	   the device is inhibited from further transmissions and the PS2_PERR
	   bit in the status register is set. The read data register is a four
	   byte deep fifo, so the error is only associated with the last byte read
	   that empties the fifo. */

	if( portregs->status & (PS2_PERR | PS2_TERR)){  /* Parity error or timeout 
						   error detected */
		if( !(portregs->status & PS2_RBNE)){ 
			/* this is a bizarre case which should never happen */
			portregs->id_reset = 0;          /* reset interface */
			portregs->control = PS2_ENBL; /* enable interface */
			return(PS2_TIMEOUT);
		}

		/* we now have an a error and at least one byte in the fifo */

		*reply = portregs->data; /* read byte from fifo */
		if( !(portregs->status & PS2_RBNE)){ /* then this byte had the error */
			portregs->id_reset = 0;          /* reset interface */
			portregs->control = PS2_ENBL; /* enable interface */
			/* send a retransmit request for the last byte */
			if( ps2_writebyte_timeout( portregs, PS2_RETRANS_REQ, num_tries) 
			   == PS2_SUCCESS)
				return( PS2_WAS_RETRANS);
			else
				return( PS2_TIMEOUT);
		}
		else /* The error was not associated with this scancode */
			return(PS2_SUCCESS);
		
	} 
	else { /* No error, and there is at least one byte to read */
		*reply = portregs->data; /* read byte from fifo */
		return(PS2_SUCCESS);
	}
}

#endif /* NGKD > 0 */
