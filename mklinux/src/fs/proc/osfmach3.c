/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#ifdef	CONFIG_OSFMACH3

#include <mach/mach_interface.h>
#include <mach/mach_host.h>

#include <osfmach3/mach3_debug.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/config.h>
#include <linux/mm.h>

#include <asm/segment.h>

/* external declarations */
extern mach_port_t      host_port;

/* forward references */
static int proc_readosfmach3(struct inode *inode,
			     struct file *file,
			     char *buf,
			     int count);

static struct file_operations proc_osfmach3_operations = {
	NULL,			/* lseek - default */
	proc_readosfmach3,	/* read - bad */
	NULL,			/* write - bad */
	proc_readdir,		/* readdir */
	NULL,			/* select - default */
	NULL,			/* ioctl - default */
	NULL,			/* mmap */
	NULL,			/* no special open code */
	NULL,			/* no special release code */
	NULL			/* can't fsync */
};

/*
 * proc directories can do almost nothing..
 */
struct inode_operations proc_osfmach3_inode_operations = {
	&proc_osfmach3_operations,	/* osfmach3 directory file-ops */
	NULL,			/* create */
	proc_lookup,		/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* bmap */
	NULL,			/* truncate */
	NULL			/* permission */
};

static int proc_readosfmach3(
	struct inode *inode,
	struct file *file,
	char *buf,
	int count)
{
	char * page;
	int length;
	unsigned int ino;
	kern_return_t	kr;

	if (count < 0)
		return -EINVAL;
	if (!(page = (char*) __get_free_page(GFP_KERNEL)))
		return -ENOMEM;
	ino = inode->i_ino;

	switch (ino) 
	{
	case PROC_OSFMACH3_VERSION:

		kr = host_kernel_version(host_port, page);
		
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("proc_readosfmach3: host_kernel_version"));
		}
			
		length = strlen(page);
		break;
	case PROC_OSFMACH3_VM_STATISTICS: {
		
		extern struct vm_statistics osfmach3_vm_stats;

		osfmach3_update_vm_info();

		length = sprintf(page,
				 "free_count      : %d\n"
				 "active_count    : %d\n"
				 "inactive_count  : %d\n"
				 "wire_count      : %d\n"
				 "zero_fill_count : %d\n"
				 "reactivations   : %d\n"
				 "pageins         : %d\n"
				 "pageouts        : %d\n"
				 "faults          : %d\n"
				 "cow_faults      : %d\n"
				 "lookups         : %d\n"
				 "hits            : %d\n",
				 osfmach3_vm_stats.free_count,
				 osfmach3_vm_stats.active_count,
				 osfmach3_vm_stats.inactive_count,
				 osfmach3_vm_stats.wire_count,
				 osfmach3_vm_stats.zero_fill_count,
				 osfmach3_vm_stats.reactivations,
				 osfmach3_vm_stats.pageins,
				 osfmach3_vm_stats.pageouts,
				 osfmach3_vm_stats.faults,
				 osfmach3_vm_stats.cow_faults,
				 osfmach3_vm_stats.lookups,
				 osfmach3_vm_stats.hits);
		break;
	}

        case PROC_OSFMACH3_HOST_SCHED_INFO: {
		
		struct host_sched_info sched_info;
		int count = sizeof(sched_info);

		kr = host_info(host_port,
			       HOST_SCHED_INFO,
			       (host_info_t)&sched_info,
			       &count);

                if (kr != KERN_SUCCESS) {
                        MACH3_DEBUG(2, kr,
                                    ("proc_readosfmach3: host_info"));
                }

		length = sprintf(page,
				 "min_timeout : %d ms\n"
				 "min_quantum : %d ms\n",
				 sched_info.min_timeout,
				 sched_info.min_quantum);
		break;
	}

        case PROC_OSFMACH3_HOST_BASIC_INFO: {
		
		struct host_basic_info basic_info;
		int count = sizeof(basic_info);

		kr = host_info(host_port,
			       HOST_BASIC_INFO,
			       (host_info_t)&basic_info,
			       &count);

                if (kr != KERN_SUCCESS) {
                        MACH3_DEBUG(2, kr,
                                    ("proc_readosfmach3: host_info"));
                }

		length = sprintf(page,
				 "max_cpus    : %d\n"
				 "avail_cpus  : %d\n"
				 "memory_size : %d Mb\n",
				 basic_info.max_cpus,
				 basic_info.avail_cpus,
				 basic_info.memory_size / (1024 * 1024));

		break;
	}

	default:
		free_page((unsigned long) page);
		return -EBADF;
	}
		
	if (file->f_pos >= length) {
		free_page((unsigned long) page);
		return 0;
	}
	if (count + file->f_pos > length)
		count = length - file->f_pos;

	/*
	 *	Copy the bytes
	 */
	memcpy_tofs(buf, page + file->f_pos, count);

	free_page((unsigned long) page);

	file->f_pos += count;	/* Move down the file */

	return count;	/* all transfered correctly */
}

static struct proc_dir_entry osfmach3_version_dir_entry = {
	PROC_OSFMACH3_VERSION, 7, "version",
	S_IFREG | S_IRUGO, 1, 0, 0,
	0, &proc_osfmach3_inode_operations,
};

static struct proc_dir_entry osfmach3_vm_statistics_dir_entry = {
        PROC_OSFMACH3_VM_STATISTICS, 13, "vm_statistics",
	S_IFREG | S_IRUGO, 1, 0, 0,
	0, &proc_osfmach3_inode_operations,
};

static struct proc_dir_entry osfmach3_host_sched_info_dir_entry = {
        PROC_OSFMACH3_HOST_SCHED_INFO, 15, "host_sched_info",
	S_IFREG | S_IRUGO, 1, 0, 0,
	0, &proc_osfmach3_inode_operations,
};

static struct proc_dir_entry osfmach3_host_basic_info_dir_entry = {
        PROC_OSFMACH3_HOST_BASIC_INFO, 15, "host_basic_info",        
	S_IFREG | S_IRUGO, 1, 0, 0,
	0, &proc_osfmach3_inode_operations,
};


void
proc_osfmach3_init(void)
{
	proc_register(&proc_osfmach3, &osfmach3_version_dir_entry);
	proc_register(&proc_osfmach3, &osfmach3_vm_statistics_dir_entry);
	proc_register(&proc_osfmach3, &osfmach3_host_sched_info_dir_entry);
	proc_register(&proc_osfmach3, &osfmach3_host_basic_info_dir_entry);
}

#endif	/* CONFIG_OSFMACH3 */
