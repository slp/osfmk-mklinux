#include <linux/config.h>
#include <linux/in.h>
#include <linux/errno.h>

void sys_iopl(void) 
{ printk("sys_iopl!\n");  return (ENOSYS); }

void sys_vm86(void)
{ printk("sys_vm86!\n");  return (ENOSYS); }

void sys_modify_ldt(void)
{ printk("sys_modify_ldt!\n");  return (ENOSYS); }

void sys_newselect(void)
{ printk("sys_newselect!\n");  return (ENOSYS); }


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

