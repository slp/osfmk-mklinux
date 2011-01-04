/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *	linux/arch/i386/kernel/ioport.c
 *
 * This contains the io-permission bitmap code - written by obz, with changes
 * by Linus.
 */

#include <linux/autoconf.h>

#include <osfmach3/device_utils.h>
#include <osfmach3/mach3_debug.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioport.h>

mach_port_t iopl_device_port = MACH_PORT_NULL;

void
osfmach3_sys_ioperm(void)
{
	kern_return_t		kr;
	mach_port_name_t	iopl_port_name;

	if (iopl_device_port == MACH_PORT_NULL) {
		kr = device_open(device_server_port,
				 MACH_PORT_NULL,
				 D_READ | D_WRITE,
				 server_security_token,
				 "iopl0",
				 &iopl_device_port);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_sys_ioperm: "
				     "device_open(\"iopl0\")"));
			return;
		}
	}

	if (current == FIRST_TASK) {
		/*
		 * The Linux server requesting privileges for itself:
		 * we're done.
		 */
		return;
	}

	/*
	 * Try to insert the port under a given name into the user task.
	 */
	iopl_port_name = (mach_port_name_t) 1;
	for (;;) {
		kr = mach_port_insert_right(current->osfmach3.task->mach_task_port,
					    iopl_port_name,
					    iopl_device_port,
					    MACH_MSG_TYPE_COPY_SEND);
		if (kr == KERN_SUCCESS || kr == KERN_RIGHT_EXISTS) {
			/*
			 * The iopl port had already been inserted or has
			 * been succesfully inserted this time.
			 */
			break;
		} else if (kr == KERN_NAME_EXISTS) {
			/*
			 * This port name is already in use. Try another one.
			 */
			iopl_port_name = (mach_port_name_t)
				((int) iopl_port_name + 1);
		} else {
			/* any other error */
			MACH3_DEBUG(1, kr,
				    ("osfmach3_sys_ioperm: "
				     "mach_port_insert_right(task=0x%x,"
				     "name=0x%x,port=0x%x)",
				     current->osfmach3.task->mach_task_port,
				     iopl_port_name,
				     iopl_device_port));
			break;
		}
	}
}

/*
 * this changes the io permissions bitmap in the current task.
 */
asmlinkage int sys_ioperm(unsigned long from, unsigned long num, int turn_on)
{
#ifdef	CONFIG_OSFMACH3
	if (from == 0 && num == 0) {
		/*
		 * This is the Mach-aware application telling us that it's
		 * going to use Mach services directly.
		 */
		if (!suser())
			return -EPERM;
		osfmach3_trap_mach_aware(current, (turn_on != 0),
					 EXCEPTION_STATE, i386_SAVED_STATE);
		return 0;
	}
#endif	/* CONFIG_OSFMACH3 */
	if (from + num <= from)
		return -EINVAL;
	if (from + num > IO_BITMAP_SIZE*32)
		return -EINVAL;
	if (!suser())
		return -EPERM;

	osfmach3_sys_ioperm();
	return 0;
}

unsigned int *stack;

/*
 * sys_iopl has to be used when you want to access the IO ports
 * beyond the 0x3ff range: to get the full 65536 ports bitmapped
 * you'd need 8kB of bitmaps/process, which is a bit excessive.
 *
 * Here we just change the eflags value on the stack: we allow
 * only the super-user to do it. This depends on the stack-layout
 * on system-call entry - see also fork() and the signal handling
 * code.
 */
asmlinkage int sys_iopl(long ebx,long ecx,long edx,
	     long esi, long edi, long ebp, long eax, long ds,
	     long es, long fs, long gs, long orig_eax,
	     long eip,long cs,long eflags,long esp,long ss)
{
	unsigned int level = ebx;

	if (level > 3)
		return -EINVAL;
	if (!suser())
		return -EPERM;
	osfmach3_sys_ioperm();
	*(&eflags) = (eflags & 0xffffcfff) | (level << 12);
	return 0;
}
