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

static struct miscdevice bus_mouse = {
	BUSMOUSE_MINOR, "busmouse", &mouse_fops
};

int bus_mouse_init(void)
{
	misc_register(&bus_mouse);
	return 0;
}
