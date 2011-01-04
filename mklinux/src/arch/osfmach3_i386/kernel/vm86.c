/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/kernel.h>
#include <linux/errno.h>

#include <asm/vm86.h>

asmlinkage int sys_vm86old(struct vm86_struct * v86)
{
	printk("sys_vm86old not implemented\n");
	return -ENOSYS;
}

asmlinkage int sys_vm86(unsigned long subfunction, struct vm86plus_struct * v86)
{
	printk("sys_vm86 not implemented\n");
	return -ENOSYS;
}
