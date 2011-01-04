/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/miscdevice.h>
#include <linux/fs.h>

extern struct file_operations mouse_fops;	/* generic mouse operations */

static struct miscdevice sun_mouse_mouse = {
	SUN_MOUSE_MINOR, "sunmouse", &mouse_fops
};

int sun_mouse_init(void)
{
	misc_register(&sun_mouse_mouse);
	return 0;
}
