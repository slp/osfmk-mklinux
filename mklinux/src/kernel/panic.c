/*
 *  linux/kernel/panic.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * This function is used through-out the kernel (including mm and fs)
 * to indicate a major problem.
 */
#include <linux/autoconf.h>

#include <stdarg.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>

#ifdef	CONFIG_OSFMACH3
#include <osfmach3/mach3_debug.h>
#include <osfmach3/server_thread.h>
#ifdef	__powerpc__
extern void print_kernel_backtrace(void);
#endif	/* __powerpc__ */
#endif	/* CONFIG_OSFMACH3 */

asmlinkage void sys_sync(void);	/* it's really int */
extern void hard_reset_now(void);
extern void do_unblank_screen(void);
extern void gdth_halt(void);
extern int C_A_D;

int panic_timeout = 0;

void panic_setup(char *str, int *ints)
{
	if (ints[0] == 1)
		panic_timeout = ints[1];
}

NORET_TYPE void panic(const char * fmt, ...)
{
	static char buf[1024];
	va_list args;
	int i;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	printk(KERN_EMERG "Kernel panic: %s\n",buf);
#ifdef __powerpc__
print_kernel_backtrace();	
#endif
	if (current == task[0])
		printk(KERN_EMERG "In swapper task - not syncing\n");
	else
		sys_sync();

	do_unblank_screen();

#ifdef	CONFIG_OSFMACH3
	Debugger("panic");
#endif	/* CONFIG_OSFMACH3 */

	if (panic_timeout > 0)
	{
		/*
	 	 * Delay timeout seconds before rebooting the machine. 
		 * We can't use the "normal" timers since we just panicked..
	 	 */
		printk(KERN_EMERG "Rebooting in %d seconds..",panic_timeout);
		for(i = 0; i < (panic_timeout*1000); i++)
			udelay(1000);
#ifdef CONFIG_SCSI_GDTH
		gdth_halt();
#endif
		hard_reset_now();
	}
#ifdef	CONFIG_OSFMACH3
		/* in case the reset fails... */
		for (;;)
			server_thread_yield(1000);	/* yield for 1s */
#endif	/* CONFIG_OSFMACH3 */
	for(;;);
}

/*
 * GCC 2.5.8 doesn't always optimize correctly; see include/asm/segment.h
 */

int bad_user_access_length(void)
{
        panic("bad_user_access_length executed (not cool, dude)");
}
