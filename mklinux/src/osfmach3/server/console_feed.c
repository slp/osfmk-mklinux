/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/kern_return.h>
#include <mach/mach_types.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/uniproc.h>

#include <linux/kernel.h>

mach_port_t osfmach3_console_feed_port = MACH_PORT_NULL;

void *
osfmach3_console_feed_thread(
	void *arg)
{
	struct server_thread_priv_data	priv_data;
	kern_return_t			kr;
	io_buf_ptr_inband_t		inbuf;	/* 128 chars */
	mach_msg_type_number_t		count;
	int				i, j;

	cthread_set_name(cthread_self(), "console feed");
	server_thread_set_priv_data(cthread_self(), &priv_data);

	uniproc_enter();

	for (;;) {
		count = sizeof inbuf;
		server_thread_blocking(FALSE);
		kr = device_read_inband(osfmach3_console_feed_port, 0, 0,
					(sizeof inbuf) - 1, inbuf, &count);
		server_thread_unblocking(FALSE);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_console_feed_thread: "
				     "device_read_inband"));
			continue;
		}
		if (count == 0) {
			osfmach3_yield();
			continue;
		}
		/* strip control characters */
		for (i = 0, j = 0; i < count ; i++) {
			if (j != i)
				inbuf[j] = inbuf[i];
			if (inbuf[j] >= ' ')
				j++;
		}
				
		inbuf[j] = '\0';
		printk("MACH:<%s>\n", inbuf);
	}
	/*NOTREACHED*/
}

void
osfmach3_console_feed_init(void)
{
	kern_return_t		kr;

	if (osfmach3_console_feed_port != MACH_PORT_NULL)
		return;

	kr = device_open(device_server_port,
			 MACH_PORT_NULL,
			 D_READ,
			 server_security_token,
			 "console_feed",
			 &osfmach3_console_feed_port);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(2, kr,
			    ("osfmach3_console_feed_init: "
			     "device_open(\"console_feed\")"));
#if	CONFIG_OSFMACH3_DEBUG
		printk("osfmach3_console_feed: "
		       "can't open \"console_feed\" device.\n");
#endif	/* CONFIG_OSFMACH3_DEBUG */
		return;
	}

	server_thread_start(osfmach3_console_feed_thread, (void *) 0);
}
