/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/exception.h>
#include <mach/mach_interface.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/parent_server.h>
#include <osfmach3/mach_init.h>

#include <linux/unistd.h>
#include <linux/signal.h>
#include <linux/kernel.h>
#include <linux/termios.h>

extern int errno;

#define __NR_parent_linux_getpid __NR_getpid
#define __NR_parent_linux_sigaction __NR_sigaction
#define __NR_parent_linux_write __NR_write
#define __NR_parent_linux_read __NR_read
#define __NR_parent_linux_ioctl __NR_ioctl
#define __NR_parent_linux__exit __NR_exit
#define __NR_parent_linux_ioperm __NR_ioperm

_syscall0(int, parent_linux_getpid)
_syscall3(int, parent_linux_sigaction,
	  int, sig,
	  const struct sigaction *, nsa,
	  struct sigaction *, osa)
_syscall3(int, parent_linux_write,
	  int, fd,
	  char *, buf,
	  int, count);
_syscall3(int, parent_linux_read,
	  int, fd,
	  char *, buf,
	  int, count);
_syscall3(int, parent_linux_ioctl,
	  int, fd,
	  int, request,
	  char *, data);
_syscall1(int, parent_linux__exit,
	  int, error_code);
_syscall3(int, parent_linux_ioperm,
	  unsigned long, from,
	  unsigned long, num,
	  int, on);

void
parent_linux_sig_handler(
	int sig)
{
	printk("** SERVER RECEIVED SIGNAL %d. Calling Debugger...\n", sig);
	Debugger("signal");
}

void
parent_linux_catchall_signals(void)
{
	struct sigaction sa;
	int sig;

	sa.sa_handler = parent_linux_sig_handler;
	sa.sa_mask = 0;
	sa.sa_flags = 0;

	for (sig = 1; sig < NSIG; sig++) {
		(void) parent_linux_sigaction(sig, &sa, (struct sigaction *) 0);
	}
}

struct termios parent_linux_console_termios;

int
parent_linux_grab_console(void)
{
	struct termios termios;

	if (parent_server_ioctl(0, TCGETS,
				(char *) &parent_linux_console_termios) < 0) {
		printk("parent_linux_grab_console:ioctl(TCGETS) -> %d\n",
		       errno);
	}
	termios = parent_linux_console_termios;

	termios.c_iflag = IGNBRK;
	termios.c_oflag = 0;
	termios.c_cflag = CS8|CREAD|PARENB;
	termios.c_lflag = 0;
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 0;

	if (parent_server_ioctl(0, TCSETS, (char *) &termios) < 0) {
		printk("parent_linux_grab_console: ioctl(TCSETS) -> %d\n",
		       errno);
	}

	return 0;
}

int
parent_linux_release_console(void)
{
	if (parent_server_ioctl(0, TCSETS,
				(char *) &parent_linux_console_termios) < 0) {
		printk("parent_linux_release_console: ioctl(TCSETS) -> %d\n",
		       errno);
	}

	return 0;
}

exception_mask_t
parent_linux_syscall_exc_mask(void)
{
	return EXC_MASK_SYSCALL;
}

int
parent_linux_get_mach_privilege(void)
{
	return parent_linux_ioperm(0, 0, 1);	/* see sys_ioperm() */
}

int parent_linux_exit(
	int code)
{
	kern_return_t	kr;

	/*
	 * exit() doesn't work very well for multi-threaded processes
	 * in Linux: it exits only the calling thread.
	 * Terminate the Mach task: this should cause the Linux processes
	 * to exit thanks to the dead-name notifications.
	 */
	kr = task_terminate(mach_task_self());
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr, ("parent_linux_server: task_terminate"));
	}
	/* try an exit if this fails... but I doubt it fails ! */
	parent_linux__exit(code);
	/*NOTREACHED*/
	return code;
}
