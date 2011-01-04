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

static struct miscdevice psaux_mouse = {
	PSMOUSE_MINOR, "ps2aux", &mouse_fops
};

int psaux_init(void)
{
	misc_register(&psaux_mouse);
	return 0;
}
