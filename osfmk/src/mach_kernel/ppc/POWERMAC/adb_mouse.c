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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */
/*
 * work original based on adb.c from NetBSD-1.0
 */

/*	$NetBSD: adb.c,v 1.3 1995/06/30 01:23:21 briggs Exp $	*/

/*-
 * Copyright (C) 1994	Bradley A. Grantham
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Bradley A. Grantham.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <mouse.h>
#include <platforms.h>

#include <mach_kdb.h>
#include <kern/spl.h>
#include <machine/machparam.h>
#include <types.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>

#include <ppc/POWERMAC/adb.h>

#define MOUSE_DEBUG 0

#define KEY_OPTION 0x3a
#define KEY_LEFT_ARROW 0x3b
#define KEY_RIGHT_ARROW 0x3c
#define MOUSE_ESCAPE 0x7e
#define KEY_MIDDLE_BUTTON 0x3f
#define KEY_RIGHT_BUTTON 0x40

#define POS(x) ((x) & 0x7f)
#define BUTTON(x) ((x) & 0x80)
#define UP 0x80
#define DOWN 0x00

struct mouse_state {
  unsigned char xpos;
  unsigned char ypos;
  unsigned char left;
  unsigned char middle;
  unsigned char right;
};

typedef struct mouse_state mouse_state_t;

mouse_state_t last_state = {0, 0, UP, UP, UP}; /* all buttons up */

void mouse_adbhandler(int number, unsigned char *buffer, int count);
int mouse_probe(caddr_t port, void *ui);
void mouse_attach(struct bus_device *dev);
void mouse_do_buttons(int button, int up);

boolean_t	mouse_initted = FALSE;

caddr_t mouse_std[NMOUSE] = { (caddr_t) 0 };

struct bus_device *mouse_info[NMOUSE];

struct bus_driver	mouse_driver = {
	mouse_probe, 0, mouse_attach, 0, mouse_std, "mouse", mouse_info, 0, 0, 0 };


extern boolean_t vc_xservermode; /* Is the tty accepting mouse input. */
extern struct tty vc_tty;

int
mouse_probe(caddr_t port, void *ui)
{
	adb_request_t	readreg;
	int val;

	return 1;
}


void
mouse_attach(struct bus_device *dev)
{
	struct adb_device	*devp;
	int	i, ret;
	unsigned short	adbreg3;
	spl_t		s;

	adb_register_handler(ADB_DEV_MOUSE, mouse_adbhandler);

	s = splhigh();
	adb_polling = TRUE;

	devp = adb_devices;
	for (i = 0; i < ADB_DEVICE_COUNT; i++, devp++) {
		if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0)
			continue;

		if (devp->a_dev_type == ADB_DEV_MOUSE) {
			/* 
			 * See if the mouse is a three button
			 * mouse.. this is done by setting the
			 * handler id to 4
			 */

			adb_set_handler(devp, 4);
		}
	}
	adb_polling = FALSE;
	splx(s);

}


void 
mouse_adbhandler(int number, unsigned char *buffer, int count)
{

#if 0
  printf("mouse_adbhandler -  num: %d, buf0: %X, buf1:%X, buf2:%X, count: %d\n", number, (int) buffer[0], (int) buffer[1], (int)buffer[2], count);
#endif
  /* If the tty is accepting mouse events, then pass them on. */
  if(vc_xservermode) {
    if (POS(buffer[0]) || POS(buffer[1]) ||
	  (BUTTON(buffer[0]) != last_state.left)) {
#if MOUSE_DEBUG
	  if (POS(buffer[0]) || POS(buffer[1]) {
	    mouse_report("position");
	  } else if (BUTTON(buffer[0]) != last_state.left)) {
	    mouse_report("left");
	  }
#endif
	  ttyinput(MOUSE_ESCAPE, &vc_tty);	/* Let the TTY know this is mouse stuff. */
	  ttyinput(buffer[0], &vc_tty); /* Send one position delta and the button state. */
	  ttyinput(buffer[1], &vc_tty);	/* Send the second position delta. */
	  last_state.xpos = POS(buffer[0]);
	  last_state.ypos = POS(buffer[1]);
	  last_state.left = BUTTON(buffer[0]);
	}
    if (count > 2 && BUTTON(buffer[1]) != last_state.middle) {
#if MOUSE_DEBUG
	  mouse_report("middle");
#endif
	  ttyinput(KEY_MIDDLE_BUTTON | BUTTON(buffer[1]), &vc_tty);
	  last_state.middle = BUTTON(buffer[1]);
    }
    if (count > 2 && BUTTON(buffer[2]) != last_state.right) {
#if MOUSE_DEBUG
	  mouse_report("right");
#endif
	  ttyinput(KEY_RIGHT_BUTTON | BUTTON(buffer[2]), &vc_tty);
	  last_state.right = BUTTON(buffer[2]);
    }
  }
}

#if MOUSE_DEBUG
void
mouse_report(char *str)
{
  printf(str);
  printf(", b0=%X, b1=%X, b2=%X, x=%X, y=%X, ",
	 (int)BUTTON(buffer[0]), (int)BUTTON(buffer[1]),
	 (int)BUTTON(buffer[2]), (int)POS(buffer[0]), (int)POS(buffer[1]));
  printf("last: lx=%X, ly=%X, ll=%X, lm=%X, lr=%X\n",
	 (int)last_state.xpos, (int)last_state.ypos, (int)last_state.left,
	 (int)last_state.middle, (int)last_state.right);
}
#endif

void mouse_do_buttons(int button,int up)
{
  unsigned char buffer[3];
  

  buffer[0]=last_state.left;
  buffer[1]=last_state.middle;
  buffer[2]=last_state.right;

  buffer[button]=up<<7;
  mouse_adbhandler(3,buffer,4);

}
