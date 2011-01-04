/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <osfmach3/assert.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/mach_init.h>
#include <osfmach3/parent_server.h>
#include <osfmach3/console.h>
#include <osfmach3/server_thread.h>
#include <osfmach3/char_dev.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/tty.h>

#if	CONFIG_OSFMACH3_DEBUG
#define CHARDEV_DEBUG 1
#endif	/* CONFIG_OSFMACH3_DEBUG */

#ifdef	CHARDEV_DEBUG
int chardev_debug = 1;
#endif	/* CHARDEV_DEBUG */

#define cdev_port_register(dev, port) \
	dev_to_object_register((dev), S_IFCHR, (char *) (port))
#define cdev_port_lookup(dev) \
	(mach_port_t) dev_to_object_lookup((dev), S_IFCHR)
#define cdev_port_deregister(dev) \
	dev_to_object_deregister((dev), S_IFCHR)

struct osfmach3_chrdev_struct osfmach3_chrdevs[MAX_CHRDEV] = {
	{ NULL, 0, NULL },
};


extern mach_port_t	osfmach3_video_port;
extern mach_port_t	osfmach3_keyboard_port;

/*
 * osfmach3_chrdev_refs[MAJOR][MINOR]: reference count on the Mach device.
 */
int *osfmach3_chrdev_refs[MAX_CHRDEV] = { 0 };

void
chrdev_get_reference(
	kdev_t	dev)
{
	unsigned int minor, major;

	major = MAJOR(dev);
	minor = MINOR(dev);

	if (osfmach3_chrdev_refs[major]) {
		ASSERT(osfmach3_chrdev_refs[major][minor] >= 0);
		osfmach3_chrdev_refs[major][minor]++;
	}
}

void
chrdev_release_reference(
	kdev_t	dev)
{
	unsigned int	major, minor;
	mach_port_t	device_port;
	kern_return_t	kr;

	major = MAJOR(dev);
	minor = MINOR(dev);

	if (osfmach3_chrdev_refs[major]) {
		ASSERT(osfmach3_chrdev_refs[major][minor] > 0);
		if (--osfmach3_chrdev_refs[major][minor] > 0) {
			/* device is still in use */
			return;
		}
	}

#ifdef	CHARDEV_DEBUG
	if (chardev_debug) {
		printk("chrdev_release_reference: closing dev %s\n",
		       kdevname(dev));
	}
#endif	/* CHARDEV_DEBUG */

	device_port = cdev_port_lookup(dev);
	ASSERT(MACH_PORT_VALID(device_port));

	/*
	 * Close the Mach device, deregister it and destroy the device port.
	 */
	kr = device_close(device_port);
	if (kr != D_SUCCESS) {
		MACH3_DEBUG(0, kr, ("chrdev_release_reference: device_close"));
		panic("chrdev_release_reference: device_close failed");
	}

	cdev_port_deregister(dev);

	kr = mach_port_destroy(mach_task_self(), device_port);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(1, kr,
			    ("chrdev_release_reference: mach_port_destroy"));
		panic("chrdev_release_reference: can't destroy device port");
	}
}

int
osfmach3_register_chrdev(
	unsigned int	major,
	const char	*name,
	int		max_minors,
	int		(*dev_to_name)(kdev_t, char *),
	int		*refs_array)
{
	int i;

	if (major == 0 || major >= MAX_CHRDEV) {
		return -EINVAL;
	}
	if (osfmach3_chrdevs[major].name == NULL) {
		osfmach3_chrdevs[major].name = name;
		osfmach3_chrdevs[major].max_minors = max_minors;
		osfmach3_chrdevs[major].dev_to_name = dev_to_name;
		osfmach3_chrdev_refs[major] = refs_array;
		return 0;
	}

	if (strcmp(name, osfmach3_chrdevs[major].name) != 0 ||
	    dev_to_name != osfmach3_chrdevs[major].dev_to_name) {
		return -EBUSY;
	}

	if (osfmach3_chrdevs[major].max_minors < max_minors) {
		for (i = 0; i < osfmach3_chrdevs[major].max_minors; i++) {
			/*
			 * Copy the old array into the new bigger one.
			 */
			refs_array[i] = osfmach3_chrdev_refs[major][i];
		}
		osfmach3_chrdev_refs[major] = refs_array;
	}

	return 0;
}


/*
 * Generic routines for tty devices.
 */
int
osfmach3_tty_dev_to_name(
	kdev_t	dev,
	char	*name)
{
	unsigned int major, minor;

	major = MAJOR(dev);
	minor = MINOR(dev);

	if (major >= MAX_CHRDEV) {
		return -ENODEV;
	}
	if (osfmach3_chrdevs[major].name == NULL) {
		printk("osfmach3_tty_dev_to_name: no Mach info for dev %s\n",
		       kdevname(dev));
		panic("osfmach3_tty_dev_to_name");
	}

	if (minor < MAX_NR_CONSOLES+1) {
		sprintf(name, "console");
		return 0;
	}
	panic("osfmach3_tty_dev_to_name: not console!\n");
}

void
osfmach3_tty_async_read(
	struct tty_struct	*tty)
{
	kdev_t		dev;
	unsigned int	major, minor;

	dev = tty->device;
	major = MAJOR(dev);
	minor = MINOR(dev);

	if (major >= MAX_CHRDEV ||
	    osfmach3_chrdevs[major].name == NULL) {
		panic("osfmach3_tty_async_read: bad device %s\n",
		      kdevname(dev));
	}
		
	panic("osfmach3_tty_async_read: not implemented\n");
}

int
osfmach3_tty_open(
	struct tty_struct *tty)
{
	kdev_t		dev;
	unsigned int	major, minor;
	kern_return_t	kr;
	char		name[16];
	mach_port_t	device_port;

	dev = tty->device;
	major = MAJOR(dev);
	minor = MINOR(dev);

	if (major >= MAX_CHRDEV) {	
		panic("osfmach3_tty_open: bad device %s", kdevname(dev));
	}

	if (tty->driver.type == TTY_DRIVER_TYPE_CONSOLE) {
		/* it's a virtual console, not a tty */
		if (osfmach3_video_port != MACH_PORT_NULL) {
			/* we support virtual consoles: nothing to do */
			return 0;
		} else {
			/* we don't support virtual consoles: reject ... */
			/* ... except for the first virtual console */
			if (minor != tty->driver.minor_start) {
				return -ENODEV;
			}
		}
	}

	if (osfmach3_chrdevs[major].name == NULL) {
		panic("osfmach3_tty_open: no mach info for device %s\n",
		      kdevname(dev));
	}

	/*
	 * See whether we have already opened the device.
	 */
	device_port = cdev_port_lookup(dev);
	if (device_port != MACH_PORT_NULL) {
		ASSERT(MACH_PORT_VALID(device_port) ||
		       (parent_server &&
			tty->driver.type == TTY_DRIVER_TYPE_CONSOLE));
		chrdev_get_reference(dev);
		return 0;
	}

	kr = osfmach3_chrdevs[major].dev_to_name(dev, name);
	if (kr != 0) {
		panic("osfmach3_tty_open: can't get Mach name for dev %s\n",
		      kdevname(dev));
	}

#ifdef	CHRDEV_DEBUG
	if (chrdev_debug) {
		printk("osfmach3_tty_open: dev %s name \"%s\"\n",
		       kdevname(dev), name);
	}
#endif	/* CHRDEV_DEBUG */
	if (! (parent_server &&
	       tty->driver.type == TTY_DRIVER_TYPE_CONSOLE)) {
		kr = device_open(device_server_port,
				 MACH_PORT_NULL,
				 D_READ | D_WRITE,
				 server_security_token,
				 name,
				 &device_port);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_tty_open(%s): "
				     "device_open(\"%s\")",
				     kdevname(dev), name));
			return -ENODEV;
		}
	} else {
		device_port = MACH_PORT_DEAD;
	}

	ASSERT(osfmach3_chrdev_refs[major][minor] == 0);
	chrdev_get_reference(dev);
	cdev_port_register(dev, device_port);

	if (tty->driver.type == TTY_DRIVER_TYPE_CONSOLE) {
		/*
		 * Get an extra reference on the console 
		 * for the server itself...
		 */
		chrdev_get_reference(dev);
		tty->count++;	/* extra ref for that too */
		osfmach3_launch_console_read_thread((void *) tty);
	} else {
		/*
		 * Queue a read request to the Mach device:
		 * that's the equivalent of interrupts enabling.
		 */
		osfmach3_tty_async_read(tty);
	}

	return 0;
}

void
osfmach3_tty_start(
	struct tty_struct *tty)
{
	/*
	 * Queue another read request to the Mach device:
	 * that's the equivalent of interrupts enabling.
	 */
	osfmach3_tty_async_read(tty);
}
