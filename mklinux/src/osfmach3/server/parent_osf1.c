/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <mach/exception.h>

#include <osfmach3/mach3_debug.h>
#include <osfmach3/parent_server.h>

#include <linux/unistd.h>
#include <linux/signal.h>
#include <linux/kernel.h>

extern int errno;

extern int parent_osf1_sigaction(int, struct sigaction *, struct sigaction *);

void
parent_osf1_sig_handler(
	int sig)
{
	printk("** SERVER RECEIVED SIGNAL %d. Calling Debugger...\n", sig);
	Debugger("signal");
}

void
parent_osf1_catchall_signals(void)
{
	struct sigaction sa;
	int sig;

	sa.sa_handler = parent_osf1_sig_handler;
	sa.sa_mask = 0;
	sa.sa_flags = 0;

	for (sig = 1; sig < NSIG; sig++) {
		(void) parent_osf1_sigaction(sig, &sa, (struct sigaction *) 0);
	}
}

typedef unsigned int    tcflag_t;
typedef unsigned char   cc_t;
typedef unsigned int    speed_t;
#define	NCCS		20
struct termios {
	tcflag_t	c_iflag;	/* input flags */
	tcflag_t	c_oflag;	/* output flags */
	tcflag_t	c_cflag;	/* control flags */
	tcflag_t	c_lflag;	/* local flags */
	cc_t		c_cc[NCCS];	/* control chars */
	speed_t		c_ispeed;	/* input speed */
	speed_t		c_ospeed;	/* output speed */
};
#define	IGNBRK		0x00000001	/* ignore BREAK condition */
#define     CS8		    0x00000300	    /* 8 bits */
#define CREAD		0x00000800	/* enable receiver */
#define PARENB		0x00001000	/* parity enable */
#define VMIN		16	/* !ICANON */
#define VTIME		17	/* !ICANON */

#define	IOCPARM_MASK	0x1fffU		/* parameter length, at most 13 bits */
#define IOC_IN		0x80000000U	/* copy in parameters */
#define IOC_OUT		0x40000000	/* copy out parameters */
#define _IOC(inout,group,num,len) \
	(inout | ((len & IOCPARM_MASK) << 16) | ((group) << 8) | (num))
#define	_IOR(g,n,t)	_IOC(IOC_OUT,	(g), (n), (unsigned int)sizeof(t))
#define	_IOW(g,n,t)	_IOC(IOC_IN,	(g), (n), (unsigned int)sizeof(t))
#define	TIOCGETA	_IOR('t', 19, struct termios) /* get termios struct */
#define	TIOCSETAF	_IOW('t', 22, struct termios) /* drn out, fls in, set */

struct termios parent_osf1_console_termios;

int
parent_osf1_grab_console(void)
{
	struct termios termios;

	if (parent_server_ioctl(0, TIOCGETA,
				(char *) &parent_osf1_console_termios) < 0) {
		printk("parent_osf1_grab_console: ioctl(GETA) -> %d\n",
		       parent_server_errno);
	}
	termios = parent_osf1_console_termios;

	termios.c_iflag = IGNBRK;
	termios.c_oflag = 0;
	termios.c_cflag = CS8|CREAD|PARENB;
	termios.c_lflag = 0;
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 0;

	if (parent_server_ioctl(0, TIOCSETAF, (char *) &termios) < 0) {
		printk("parent_osf1_grab_console: ioctl(SETAF) -> %d\n",
		       parent_server_errno);
	}

	return 0;
}

int
parent_osf1_release_console(void)
{
	if (parent_server_ioctl(0, TIOCSETAF,
				(char *) &parent_osf1_console_termios) < 0) {
		printk("parent_osf1_release_console: ioctl(SETAF) -> %d\n",
		       parent_server_errno);
	}

	return 0;
}

exception_mask_t
parent_osf1_syscall_exc_mask(void)
{
	return EXC_MASK_SYSCALL;
}

int
parent_osf1_get_mach_privilege(void)
{
	return 0;
}
