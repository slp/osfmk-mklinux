/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 *
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <mach/mach_interface.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/console.h>
#include <osfmach3/parent_server.h>
#include <osfmach3/mach3_debug.h>

#include <linux/kernel.h>

#ifdef	CONFIG_PMAC_CONSOLE
extern void pmac_find_display(void);
#endif	/* CONFIG_PMAC_CONSOLE */

boolean_t
osfmach3_con_probe(void)
{
	kern_return_t	kr;

	if (parent_server || osfmach3_use_mach_console) {
		/* no virtual consoles */
		return FALSE;
	}

#ifdef	CONFIG_PMAC_CONSOLE
	/*
	 * Try the video console device.
	 */
	kr = device_open(device_server_port,
			 MACH_PORT_NULL,
			 D_READ | D_WRITE,
			 server_security_token,
			 "vc0",
			 &osfmach3_video_port);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(2, kr,
			    ("osfmach3_con_probe: device_open(\"vc0\")"));
		printk("osfmach3_con_probe: can't map display. "
		       "Using dumb console.\n");
		return FALSE;
	}

	pmac_find_display();
	return TRUE;
#else	/* CONFIG_PMAC_CONSOLE */
	return FALSE;
#endif	/* CONFIG_PMAC_CONSOLE */
}

/*
 * Console 'beep' support
 */

#include "console_sounds.h"
#include <osfmach3/device_utils.h>
#include <osfmach3/device_reply_hdlr.h>

extern void awacs_mksound(unsigned int hz, unsigned int ticks);
extern void awacs_busy(int state);

char *awacs_dev_name = "awacs";  /* Could be overidden */

static mach_port_t _awacs_device_port;
static mach_port_t _awacs_reply_port;
static char _awacs_param;
static int _awacs_busy;

static dev_reply_t _awacs_write_reply(char *param, kern_return_t rc, char *data, unsigned int count);
static dev_reply_t _awacs_read_reply(char *param, kern_return_t rc, char *data, unsigned int count);

#ifndef AWACS_DEVCMD_RATE
#define AWACS_DEVCMD_RATE 1
#endif

static dev_reply_t 
_awacs_write_reply( char *param, kern_return_t rc, char *data, unsigned int count)
{
	return KERN_SUCCESS;
}

static dev_reply_t 
_awacs_read_reply( char *param, kern_return_t rc, char *data, unsigned int count)
{
	return KERN_SUCCESS;
}

void
awacs_busy(int state)
{
	_awacs_busy = state;
}

void 
awacs_mksound(unsigned int hz, unsigned int ticks)
{
	kern_return_t		kr;  
	int			speed;
	int			count;

	if (_awacs_busy)
		return;  /* 'Sound' driver using device */
	if (_awacs_device_port == (mach_port_t)NULL) {
		kr = device_open( device_server_port,
				  MACH_PORT_NULL,
				  D_READ | D_WRITE,
				  server_security_token,
				  awacs_dev_name,
				  &_awacs_device_port );

		if ( kr != KERN_SUCCESS ) {
			/* Ignore errors, just 'bag' ringing the bell */
			return;
		}

		device_reply_register( &_awacs_reply_port,
				       (char *) &_awacs_param, 
				       (dev_reply_t) _awacs_read_reply,
				       (dev_reply_t) _awacs_write_reply );
	}
	/* Set port speed */
	speed = KBEEP_SPEED;
	count = KBEEP_BUFSIZE;
	kr = device_set_status(_awacs_device_port,
			       AWACS_DEVCMD_RATE,
			       &speed,
			       1);

	if (kr != D_SUCCESS) {
		return;
	}
	/* Write the 'bell' data */
	kr = serv_device_write_async((mach_port_t)            _awacs_device_port,
				     (mach_port_t)            _awacs_reply_port,
				     (dev_mode_t)             D_WRITE,
				     (recnum_t)               0,
				     (caddr_t)                kbeep_buf,
				     (mach_msg_type_number_t) count,
				     (boolean_t)              FALSE);
}
