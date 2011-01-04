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
e*    notice, this list of conditions and the following disclaimer in the
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

#include <vc.h>
#include <platforms.h>

#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <kern/spl.h>
#include <machine/machparam.h>
#include <types.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>

#include <ppc/POWERMAC/keyboard.h>
#include <ppc/POWERMAC/adb.h>
#include <ppc/POWERMAC/video_console.h>

/* Key repeat parameters */
static int keyboard_initialdelay = 40;	/* ticks before auto-repeat */
static int keyboard_repeatinterval = 8;	/* ticks between auto-repeat */
static boolean_t keyboard_repeating = FALSE;	/* key that is auto-repeating */
static unsigned char  keyboard_repeating_key;
extern struct tty	vc_tty;
static int keyboard_led_state;		/* state of caps/num/scroll lock */

void keyboard_translatekey(unsigned char key);
void keyboard_autorepeat(void *keyp);
void keyboard_doupdown(unsigned char key);
void keyboard_processdata(unsigned char key);
void keyboard_adbhandler(int number, unsigned char *buffer, int count);
void keyboard_init(void);
void keyboard_updown(unsigned char key);
void mouse_do_buttons(int button,int up);

io_return_t	keyboard_set_led_state(int state);
int		keyboard_get_led_state(void);
void		keyboard_initialize_led_state(void);

int  vcgetc(int l, int u, boolean_t wait, boolean_t raw);

unsigned char	keyboard_polling_char;
boolean_t	keyboard_have_char, keyboard_polling = FALSE;
boolean_t	keyboard_initted = FALSE;
int		keyboard_adb_addr = -1;
char have_three_buttons=0;

/* Michel Pollet: Support for different keyboard layout */
int		keyboard_keymap = ADB_KEYMAP_US;
/* That is the table used by the Cmd-Option-Space sequence for cycling
   between keyboard mapping. You only have to :
	1) Create your particular keymap in keyboard.h
	2) Add its entry in that table
*/
adb_keymap_t	*keyboard[] = {
	&keyboard_us,
	&keyboard_french,
	&keyboard_german,
	NULL	/* Insert your keymap BEFORE that line */
};

extern boolean_t	vc_xservermode;

void
keyboard_init(void)
{
	struct adb_device *devp;
	unsigned short adbreg3;
	int	i;
	spl_t	s;
	int std=0,othr=0;

	if (!keyboard_initted) {
		keyboard_initted = TRUE;

		s = splhigh();
		adb_polling = TRUE;

		devp = adb_devices;
		for (i = 0; i < ADB_DEVICE_COUNT; i++, devp++) {
			if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0)
				continue;

			if (devp->a_dev_type == ADB_DEV_KEYBOARD) {
				/* Find the first ADB keyboard device, and use that. */
				if(keyboard_adb_addr<0)
					keyboard_adb_addr = i;
				
				if(devp->a_dev_handler==1) {
					if(!std)
						std=1;
				}
				else
					othr=1;

				if(othr&&std) {
					have_three_buttons=1;
					break;
				}
			}
		}
		
		for (; i < ADB_DEVICE_COUNT; i++, devp++) {
		    if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0)
				continue;

			if (devp->a_dev_type == ADB_DEV_KEYBOARD) {
			}
		}

		adb_polling = FALSE;
		splx(s);

		/* Pickup all keyboard devices */
		adb_register_handler(ADB_DEV_KEYBOARD, keyboard_adbhandler);
	}

}

/* Called the first time the console is opened, can't be done
 * above since it's too early
 */
void
keyboard_initialize_led_state(void)
{
	keyboard_led_state = keyboard_get_led_state();
}

int
keyboard_get_led_state(void)
{
	unsigned short	lights = 0;
	int state = 0;

	if (keyboard_adb_addr == -1) 
		return 0;

	adb_readreg(keyboard_adb_addr, 2, &lights);

	lights = ~lights;	/* inverse things since ADB uses 0=set ... */

	/* Use LED state to get keyboard state */

	if (lights & (ADBKS_CAPSLOCK|ADBKS_LED_CAPSLOCK))
		state |= VC_LED_CAPSLOCK;
	if (lights & (ADBKS_SCROLL_LOCK|ADBKS_LED_SCROLLLOCK))
		state |= VC_LED_SCROLLLOCK;
	if (lights & (ADBKS_NUMLOCK| ADBKS_LED_NUMLOCK))
		state |= VC_LED_NUMLOCK;

	return state;
}
	
int
keyboard_set_led_state(int state)
{
	unsigned short	lights = 0;

	if (keyboard_adb_addr == -1) 
		return D_NO_SUCH_DEVICE;

	if (state & VC_LED_NUMLOCK)
		lights |= ADBKS_LED_NUMLOCK;
	if (state & VC_LED_CAPSLOCK)
		lights |= ADBKS_LED_CAPSLOCK;
	if (state & VC_LED_SCROLLLOCK)
		lights |= ADBKS_LED_SCROLLLOCK;

	/* Inverse things for ADB */
	lights = ~lights;
	adb_writereg_async(keyboard_adb_addr, 2, lights);

	return D_SUCCESS;
}
	
void 
keyboard_autorepeat(void *keyp)
{
	unsigned char key = keyboard_repeating_key;
	spl_t	s = spltty();

	keyboard_translatekey(ADBK_KEYDOWN(key));
	keyboard_translatekey(ADBK_KEYUP(key));

	timeout(keyboard_autorepeat, &keyboard_repeating_key, keyboard_repeatinterval);
}

void 
keyboard_adbhandler(int number, unsigned char *buffer, int count)
{

#if MACH_KGDB || MACH_KDB
	if (buffer[0] == ADBK_POWER) {
		Debugger("Keyboard Request");
		return;
	}
#endif /* MACH_KGDB || MACH_KDB */


	if (keyboard_polling) {
		if (ADBK_PRESS(buffer[0])) {
			keyboard_have_char = TRUE;
			keyboard_polling_char = ADBK_KEYVAL(buffer[0]);
			return;
		}
	}

	if (vc_xservermode) {
		if (have_three_buttons &&
		    (count==1) &&
		    /* (adb_devices[number].a_dev_handler == 1) && */
		     ((ADBK_KEYVAL(buffer[0])==ADBK_LEFT)||
		      (ADBK_KEYVAL(buffer[0])==ADBK_RIGHT)))
		  {
		    mouse_do_buttons(ADBK_KEYVAL(buffer[0])-ADBK_LEFT+1,
				     !ADBK_PRESS(buffer[0]));
		    return;
		  }
		ttyinput(buffer[0], &vc_tty);
		
		if (buffer[1] != 0xff)
			ttyinput(buffer[1], &vc_tty);

		return;
	}

	keyboard_updown(buffer[0]);

	if (buffer[1] != 0xff)
		keyboard_updown(buffer[1]);

}

void
keyboard_updown(unsigned char key)
{

	if (!ADBK_MODIFIER(key)) {
		if ((vc_tty.t_state & TS_ISOPEN) == 0) {
			return;		/* Don't bother if keyboard isn't open. */
		}
		if (ADBK_PRESS(key)) {
			keyboard_repeating_key = ADBK_KEYVAL(key);
			if (!keyboard_repeating) {
				keyboard_repeating = TRUE;
				timeout(keyboard_autorepeat, (void *)&keyboard_repeating_key, keyboard_initialdelay);
			}
		} else if (keyboard_repeating) {
			untimeout(keyboard_autorepeat, (void *) &keyboard_repeating_key);
			keyboard_repeating = FALSE;
		}
	}

	keyboard_translatekey(key);

}
		
void
keyboard_translatekey(unsigned char key)
{
	static int shift = 0, control = 0, flower = 0, option = 0;
	int      press, val, state;
	unsigned char str[10], *s;


	press = ADBK_PRESS(key);
	val = ADBK_KEYVAL(key);

	switch (val) {
	case	ADBK_FLOWER:
		flower = press;
		break;

	case	ADBK_OPTION:
	case	ADBK_OPTION_R:
		option = press;
		break;

	case	ADBK_SHIFT:
	case	ADBK_SHIFT_R:
		shift = press;
		break;

	case	ADBK_CAPSLOCK:
		if (press)
			keyboard_led_state |= VC_LED_CAPSLOCK;
		else
			keyboard_led_state &= ~VC_LED_CAPSLOCK;
		keyboard_set_led_state(keyboard_led_state);
		break;

	case	ADBK_NUMLOCK:
		
		keyboard_led_state ^= VC_LED_NUMLOCK;
		keyboard_set_led_state(keyboard_led_state);
		break;

	default:
		if (val == ADBK_CONTROL || val == ADBK_CONTROL_R) {
			control = press;
		} else {
			if (press) {
				switch (val) {
				case ADBK_UP:
					str[0] = '\033';
					str[1] = 'O';
					str[2] = 'A';
					str[3] = 0xff;
					break;
				case ADBK_DOWN:
					str[0] = '\033';
					str[1] = 'O';
					str[2] = 'B';
					str[3] = 0xff;
					break;
				case ADBK_RIGHT:
					str[0] = '\033';
					str[1] = 'O';
					str[2] = 'C';
					str[3] = 0xff;
					break;
				case ADBK_LEFT:
					str[0] = '\033';
					str[1] = 'O';
					str[2] = 'D';
					str[3] = 0xff;
					break;
				case ADBK_SPACE:
					/* Michel Pollet:
					   Look for Cmd-Option space
					   for switching keyboard layout
					*/
					if (flower && option) {
						if (!keyboard[++keyboard_keymap])
							keyboard_keymap = 0;
						str[0] = 0xff;
						break;
					}
					/* Fall */
				default:
					state = 0;
					if (shift || (keyboard_led_state & VC_LED_CAPSLOCK)) {
						state = 1;
					}
					str[0] = (*keyboard[keyboard_keymap])[val][state];

					if (control && (str[0] != 0xff))
						str[0] = str[0] & 0x1f;

					str[1] = 0xff;
					break;
				}
				if (vc_tty.t_state & TS_ISOPEN) {
					for (s = str; (*s != 0xff); s++) {
						ttyinput(*s, &vc_tty);
					}
				}

			}
		}
		break;
	}

}

int
vcgetc(int l, int u, boolean_t wait, boolean_t raw)
{
	unsigned char c;


	if (adb_hardware == NULL) {
		printf("vcgetc called before adb_hardware ready\n");
		return -1;
	}
	keyboard_polling = TRUE;
	if (wait) {
		while (!keyboard_have_char)
			adb_poll();
		keyboard_polling = FALSE;
		keyboard_have_char = FALSE;
		c = keyboard_polling_char;
		return c;
	} else {
		adb_poll();
	}

	keyboard_polling = FALSE;

	if (keyboard_have_char) {
		keyboard_have_char = FALSE;
		c = keyboard_polling_char;
		return c;
	}

	return -1;
}
