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
 * Copyright (c) 1990,1991,1992 The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: cons.c 1.11 92/06/27$
 *	Author: Bob Wheeler, University of Utah CSS
 */

/*
 *	Console selection, cnputc and cngetc routines
 */

#include <types.h>
#include <device/conf.h>
#include <hp_pa/HP700/cons.h>
#include <mach/boolean.h>

#include <kern/misc_protos.h>
#include <device/ds_routines.h>
#include <machine/pdc.h>

struct 	consdev *cn_tab = 0;		/* physical console device info */
int 	console_found  = 0;		/* no permanent console found yet */
int 	pdc_alive      = 0;		/* PDC is alive at boot */
char 	*msgbuf = (char *)0;		/* pointer to message buffer */
char 	*msgbuf_end = (char *)0;	/* next valid location to write to */

#define MSGBUF_SIZE 8192

void
cninit(void)
{
	int x;
	dev_ops_t cn_ops;
	struct consdev *cp;
	register char *p;

	/*
	 * Collect information about all possible consoles
	 * and find the one with highest priority
	 */
	for (cp = constab; cp->cn_probe; cp++) {
		(*cp->cn_probe)(cp);
		if (cp->cn_pri > CN_DEAD &&
		    (cn_tab == NULL || cp->cn_pri > cn_tab->cn_pri))
			cn_tab = cp;
	}
	/*
	 * No console, we cannot handle it yet
	 */
	if ((cp = cn_tab) == NULL)
		panic("can't find a console device");

	/*
	 * found a console, look up its dev_ops pointer in 
	 * the device table
	 */
	if (dev_name_lookup(cp->cn_name,&cn_ops,&x) == FALSE)
		panic("cninit: dev_name_lookup failed");

	/*
	 * initialize it as the console
	 */
	(*cp->cn_init)(cp);

	/*
	 * replace it in the device indirection table
	 */
	dev_set_indirection("console", cn_ops, cp->cn_dev);

	console_found = 1;

	/*
	 * now that we have a console dump the message buffer 
	 * if there is one
	 */
	if (msgbuf != (char *)0) {
		for (p = msgbuf; p < msgbuf_end; p++)
			cnputc(*p);
		kfree((vm_offset_t)msgbuf, MSGBUF_SIZE);
	}

}

/*
 * the boot program left the PDC console intact. We can use it to print until
 * we find a real console. There will be a point when we start to initialize
 * the devices in autoconf that we won't be able to use the PDC anymore. In
 * this case we will establish a message buffer to print to. Once we do get 
 * a console then we can print out the message buffer and start using the 
 * real console. In the event that we panic before the message buffer is 
 * printed, we can reestablish a PDC console and dump the buffer to it.
 */

int
cngetc(void)
{
	if (console_found)
		return (*cn_tab->cn_getc)(cn_tab->cn_dev, TRUE);

	if (pdc_alive)
		return pdcgetc();

	return 0;
}

/*
 * XXX Need to fix pdcgetc() so it can handle this.
 */
int
cnmaygetc(void)
{
	if (console_found)
		return (*cn_tab->cn_getc)(cn_tab->cn_dev, FALSE);

	return 0;
}

void
cnputc(char c)
{
	if (c) {
		if (console_found) {
			(*cn_tab->cn_putc)(cn_tab->cn_dev, c);
			if (c == '\n')
				(*cn_tab->cn_putc)(cn_tab->cn_dev, '\r');
			return;
		}

		if (pdc_alive) {
			pdcputc(c);
			if (c == '\n')
				pdcputc('\r');
			return;
		}

		/*
		 * we don't have a console and the PDC can't print right now.
		 * Establish a message buffer if we don't already have one 
		 * and print to it. We assume that the PDC gets turned off 
		 * after VM is on.
		 */
		if (msgbuf == (char *)0) {
			msgbuf = (char *) kalloc(MSGBUF_SIZE);
			msgbuf_end = msgbuf;
		}

		if (msgbuf_end != 0 && msgbuf_end < msgbuf + MSGBUF_SIZE)
			*msgbuf_end++ = c;
	}
}




