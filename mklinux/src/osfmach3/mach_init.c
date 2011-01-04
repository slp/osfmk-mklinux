/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * This is a user process that is launched by the Linux server to
 * start the real "init".
 * Compile it without shared libraries on the target machine:
 * 	gcc -static -o mach_init mach_init.c
 * and install it as /mach_servers/mach_init.
 */

#include <linux/types.h>
#include <linux/fcntl.h>

char * argv_rc[] = { "/bin/sh", NULL };
char * envp_rc[] = { "HOME=/", "TERM=linux", NULL };
char * argv[] = { "-/bin/sh",NULL };
char * envp[] = { "HOME=/usr/root", "TERM=linux", NULL };

static int do_rc(void * rc)
{
	close(0);
	if (open(rc,O_RDONLY,0))
		_exit(3);
	_exit(execve("/bin/sh",argv_rc,envp_rc));
}

static int do_shell(void * shell)
{
	close(0);close(1);close(2);
	setsid();
	(void) open("dev/tty1",O_RDWR,0);
	(void) dup(0);
	(void) dup(0);
	_exit(execve(shell, argv, envp));
}

int
main(
	int	argc,
	char	**argv_init,
	char	**envp_init)
{
	int pid,i;

	/*
	 *	This keeps serial console MUCH cleaner, but does assume
	 *	the console driver checks there really is a video device
	 *	attached (Sparc effectively does).
	 */

	if ((open("/dev/tty1",O_RDWR,0) < 0) &&
	    (open("/dev/ttyS0",O_RDWR,0) < 0))
		printf("Unable to open an initial console.\n");

	(void) dup(0);
	(void) dup(0);

	execve("/etc/init",argv_init,envp_init);
	execve("/bin/init",argv_init,envp_init);
	execve("/sbin/init",argv_init,envp_init);
	/* if this fails, fall through to original stuff */

	if (!(pid=fork())) {
		do_rc("/etc/rc");
	}
	if (pid>0)
		while (pid != wait(&i))
			/* nothing */;
	while (1) {
		if ((pid = fork()) < 0) {
			printf("Fork failed in mach_init\n\r");
			continue;
		}
		if (!pid) {
			do_shell("/bin/sh");
		}
		while (1)
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();
	}
	_exit(0);
}

