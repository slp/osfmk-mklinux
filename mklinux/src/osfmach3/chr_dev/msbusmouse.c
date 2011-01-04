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

static struct miscdevice ms_bus_mouse = {
	MS_BUSMOUSE_MINOR, "msbusmouse", &mouse_fops
};

int ms_bus_mouse_init(void)
{
	misc_register(&ms_bus_mouse);
	return 0;
}
