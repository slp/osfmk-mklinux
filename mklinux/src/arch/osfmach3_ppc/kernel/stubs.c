/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/config.h>
#include <linux/in.h>
#include <linux/errno.h>

#ifdef	CONFIG_OSFMACH3
#include <linux/kernel.h>
#endif	/* CONFIG_OSFMACH3 */

#ifdef	CONFIG_OSFMACH3
int
#else	/* CONFIG_OSFMACH3 */
void
#endif	/* CONFIG_OSFMACH3 */
 sys_iopl(void) 
{ printk("sys_iopl!\n");  return (ENOSYS); }

#ifdef	CONFIG_OSFMACH3
int
#else	/* CONFIG_OSFMACH3 */
void
#endif	/* CONFIG_OSFMACH3 */
 sys_vm86(void)
{ printk("sys_vm86!\n");  return (ENOSYS); }

#ifdef	CONFIG_OSFMACH3
int
#else	/* CONFIG_OSFMACH3 */
void
#endif	/* CONFIG_OSFMACH3 */
 sys_modify_ldt(void)
{ printk("sys_modify_ldt!\n");  return (ENOSYS); }

#ifdef	CONFIG_OSFMACH3
int
#else	/* CONFIG_OSFMACH3 */
void
#endif	/* CONFIG_OSFMACH3 */
 sys_newselect(void)
{ printk("sys_newselect!\n");  return (ENOSYS); }

#ifndef	CONFIG_OSFMACH3
halt()
{
	_printk("\n...Halt!\n");
	printk("\n...Halt!\n");
	_do_halt();
}

_panic(char *msg)
{
	_printk("Panic: %s\n", msg);
	printk("Panic: %s\n", msg);
	abort();
}

_warn(char *msg)
{
	_printk("*** Warning: %s UNIMPLEMENTED!\n", msg);
}


void
saved_command_line(void)
{
	panic("saved_command_line");
}

void
KSTK_EIP(void)
{
	panic("KSTK_EIP");
}

void
KSTK_ESP(void)
{
	panic("KSTK_ESP");
}

#ifndef CONFIG_MODULES
void
scsi_register_module(void)
{
	panic("scsi_register_module");
}

void
scsi_unregister_module(void)
{
	panic("scsi_unregister_module");
}
#endif
#endif	/* CONFIG_OSFMACH3 */


