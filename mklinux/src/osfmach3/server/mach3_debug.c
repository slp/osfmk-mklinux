/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * MACH debugging.
 */

#include <mach_error.h>
#include <device/device.h>
#include <device/device_types.h>
#include <device/tty_status.h>
#include <mach/host_reboot.h>
#include <mach/mach_host.h>
#include <mach/mach_interface.h>

#include <osfmach3/mach_init.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/console.h>

#include <linux/kernel.h>

#include <osfmach3/parent_server.h>
extern mach_port_t privileged_host_port;

int mach3_debug = 2;
int mach3_debug_debugger = 0;

/*VARARGS3*/
void
mach3_debug_msg(int level, kern_return_t retcode)
{
	char 		*err_string;

	err_string = mach_error_string(retcode);
	printk(" err=0x%x=%d \"%s\"\n", retcode, retcode,
	       err_string ? err_string : "??");
	if (level <= mach3_debug_debugger)
		Debugger("mach3_debug");
}

void
Debugger(const char *message)
{
	kern_return_t ret;

	printk("Debugger: %s\n", message);

	if (parent_server) {
		printk("[ server (pid=%d) suspended ... ]\n",
		       parent_server_getpid());
		/* XXX should flush the console here */
		(void) task_suspend(mach_task_self());
		return;
	}

#ifdef	TTY_DRAIN
	{
		int word;

		/* flush console output */
		(void) device_set_status(osfmach3_console_port,
					 TTY_DRAIN, &word, 0);
	}
#endif	/* TTY_DRAIN */
	ret = host_reboot(privileged_host_port, HOST_REBOOT_DEBUGGER);

	/* In case host_reboot() fails, don't return */
	if (ret != KERN_SUCCESS) {
		printk("Debugger: host_reboot failed. -- Looping...\n");
		for (;;);
	}
}
