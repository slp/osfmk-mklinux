/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifdef	__powerpc__

#include <linux/config.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/malloc.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>

#ifdef	CONFIG_OSFMACH3
#include <osfmach3/server_thread.h>
#include <osfmach3/uniproc.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/mach_init.h>
#include <ppc/adb_io.h>
#endif

#include <linux/kernel.h>

#ifdef CONFIG_OSFMACH3
mach_port_t	adb_port = MACH_PORT_NULL;
#endif

int		adb_refs;
struct adb_info	adb_devices[16];
int		adb_count;

char		*mach_adb_name = "adb0";

char	*adb_names[8] = {
	NULL,
	"protection device",
	"keyboard",
	"mouse",
	"tablet",
	"modem",
	NULL,
	"application"
};

int adb_open(
	struct inode	*inode,
	struct file	*file)
{
	kern_return_t	kr;

	if (adb_refs++ > 0) {
		return 0;
	}

#ifdef	CONFIG_OSFMACH3
	ASSERT(adb_port == MACH_PORT_NULL);

	kr = device_open(device_server_port,
			 MACH_PORT_NULL,
			 D_READ | D_WRITE,
			 server_security_token,
			 mach_adb_name,
			 &adb_port);

	if (kr != KERN_SUCCESS) {
		if (kr != D_NO_SUCH_DEVICE) {
			MACH3_DEBUG(1, kr,
				    ("adb_open(0x%x): "
				     "device_open(\"adb0\")",
				     inode->i_rdev));
		}
		return -ENODEV;
	}
#endif

	return 0;
}

void adb_release(
	struct inode	*inode,
	struct file	*file)
{
	kern_return_t	kr;

	if (--adb_refs == 0) {
#ifdef	CONFIG_OSFMACH3
		kr = device_close(adb_port);
		if (kr != D_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("adb_release(0x%x): device_close(0x%x)",
				     inode->i_rdev, adb_port));
			panic("adb_release: device_close failed");
		}
		kr = mach_port_destroy(mach_task_self(), adb_port);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("adb_release(0x%x): "
				     "mach_port_destroy(0x%x)",
				     inode->i_rdev, adb_port));
			panic("adb_release: mach_port_destroy failed");
		}
		adb_port = MACH_PORT_NULL;
#endif
	}
}

int
adb_ioctl(
	struct inode * inode,
	struct file * file,
	unsigned int cmd,
	unsigned long arg)
{
	int	i;
	struct adb_info		info;
	struct adb_regdata	reg;
	int	error, count;
	unsigned long	*ptr, orig_arg = arg;
	kern_return_t	kr;

	switch (cmd) {
	case	ADB_GET_INFO:
		error = verify_area(VERIFY_WRITE, (void *)arg, 16*sizeof(struct adb_info));
		if (error)
			return error;

		ptr = (unsigned long *) adb_devices;
		count = (adb_count * sizeof(struct adb_info))/ sizeof(long);

		while (count-- > 0) {
			put_fs_long(*ptr, (long *)arg);
			arg += sizeof(int);
			ptr++;
		}
		break;

	case	ADB_GET_COUNT:
		error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(int));

		if (error) 
			return error;

		put_fs_long(adb_count, (long *)arg);
		break;

#ifdef CONFIG_OSFMACH3
	case	ADB_READ_REG:
		error = verify_area(VERIFY_WRITE, (void *) arg, sizeof(struct adb_regdata));

		if (error)
			return error;

		count = sizeof(struct adb_regdata) / sizeof(int);

		ptr = (unsigned long *) &reg;
		while (count-- > 0) { 
			*ptr++ = get_fs_long((long *)arg);
			arg += sizeof(int);
		}

		count = sizeof(struct adb_regdata)/sizeof(int);

		kr = device_set_status(adb_port, ADB_READ_REG, (int *) &reg,
								count);

		if (kr != KERN_SUCCESS)
			return	-EIO;

		count = sizeof(struct adb_regdata)/sizeof(int);
		kr = device_get_status(adb_port, ADB_READ_DATA, (int *)&reg,
						(int *) &count);

		if (kr != KERN_SUCCESS)
			return	-EIO;

		arg = orig_arg;

		put_fs_word(reg.a_count, &((struct adb_regdata *) arg)->a_count);
		for (i = 0; i < reg.a_count; i++) 
			put_fs_byte(reg.a_buffer[i], &((struct adb_regdata *)arg)->a_buffer[i]);
		break;

	case	ADB_WRITE_REG:
		if (!suser())
			return -EACCES;

		error = verify_area(VERIFY_READ, (void *) arg, sizeof(struct adb_regdata));

		if (error)
			return error;

		count = sizeof(struct adb_regdata) / sizeof(int);

		ptr = (unsigned long *) &reg;
		while (count-- > 0) { 
			*ptr++ = get_fs_long((long *)arg);
			arg += sizeof(int);
		}

		count = sizeof(struct adb_regdata)/sizeof(int);

		kr = device_set_status(adb_port, ADB_WRITE_REG, (int *) &reg,
								count);

		if (kr != KERN_SUCCESS)
			return	-EIO;
		break;

	case	ADB_SET_HANDLER:
		if (!suser())
			return -EACCES;

		error = verify_area(VERIFY_READ, (void *)arg, sizeof(struct adb_info));

		if (error)
			return error;

		count = sizeof(struct adb_info)/sizeof(int);

		ptr = (unsigned long *) &info;
		while (count-- > 0) {
			*ptr++ = get_fs_long((long *)arg);
			arg += sizeof(long);
		}

		count = sizeof(struct adb_info)/sizeof(int);

		kr = device_set_status(adb_port, ADB_SET_HANDLER, (int *) &info, count);
		if (kr == D_IO_ERROR)
			return	-EIO;

		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr, ("adb_ioctl: "
					"device_set_status(ADB_SET_HANDLER)"));
			return -EINVAL;
		}
		/* Update the internal table */
		for (i = 0; i < adb_count; i++) {
			if (adb_devices[i].a_addr == info.a_addr) {
				adb_devices[i].a_handler = info.a_handler;
				break;
			}
		}
		break;
#endif

	default:
		return	-EINVAL;
	}

	return 0;
}

int adb_read(
	struct inode	*inode,
	struct file	*file,
	char		*buffer,
	int		count)
{
	return -EINVAL;
}

int adb_write(
	struct inode	*inode,
	struct file	*file,
	const char	*buffer,
	int		count)
{
	return -EINVAL;
}

int adb_select(
	struct inode	*inode,
	struct file	*file,
	int		sel_type,
	select_table	*wait)
{
	return 0;
}

struct file_operations adb_fops = {
        NULL,		/* seek */
	adb_read,
	adb_write,
	NULL,		/* readdir */
	adb_select,
	adb_ioctl,	/* ioctl */
	NULL,		/* mmap */
        adb_open,
        adb_release
};


#ifdef CONFIG_PROC_FS
static int proc_adb_read(char *buf, char **start, off_t offset, int len, int unused)
{
	int	i;

	struct adb_info	*a = adb_devices;

	len= sprintf(buf, "adb devices: %d\naddr type\n", adb_count);

	for (i = 0; i < adb_count; i++, a++) {
		if (a->a_type > (sizeof(adb_names)/sizeof(adb_names[0]))
		   || adb_names[a->a_type] == NULL)
			len += sprintf(buf+len, "%2i   unknown (%d)\n", (int)a->a_addr, (int)a->a_type);
		else
			len += sprintf(buf+len, "%2i   %s\n", (int)a->a_addr, adb_names[a->a_type]);
	}

	return len;
}

static struct proc_dir_entry adb_proc_dir_entry = {
	0, 3, "adb",
	S_IFREG | S_IRUGO, 1, 0, 0,
	0, NULL /* ops -- default to array */,
	&proc_adb_read /* get_info */,
};

#endif /* PROC_FS */


int
adb_init(void)
{
#ifdef	CONFIG_OSFMACH3
	int	count;
	kern_return_t	kr;

	kr = device_open(device_server_port,
			 MACH_PORT_NULL,
			 D_READ | D_WRITE,
			 server_security_token,
			 mach_adb_name,
			 &adb_port);

	if (kr != KERN_SUCCESS)
		return -EIO;

	count = 1;
	kr = device_get_status(adb_port, ADB_GET_COUNT, (int *) &adb_count, &count);

	if (kr != KERN_SUCCESS) 
		adb_count = 0;
	else {
		if (adb_count) {
			count = (adb_count*sizeof(struct adb_info))/sizeof(int);
			kr = device_get_status(adb_port, ADB_GET_INFO, (int *) adb_devices, &count);
		}
	}
	(void) device_close(adb_port);
	(void) mach_port_destroy(mach_task_self(), adb_port);

	adb_port = MACH_PORT_NULL;
#endif

#ifdef CONFIG_PROC_FS
	proc_register_dynamic(&proc_root, &adb_proc_dir_entry);
#endif /* PROC_FS */

	if (register_chrdev(ADB_MAJOR,"adb",&adb_fops)) {
	  printk("unable to get major %d for adb device\n",
		 ADB_MAJOR);
		return -EIO;
	}

	return 0;
}

#endif	/* __powerpc__ */
