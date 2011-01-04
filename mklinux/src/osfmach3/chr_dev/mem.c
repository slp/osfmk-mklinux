/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 *  linux/drivers/char/mem.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <mach/mach_interface.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/console.h>
#include <osfmach3/device_utils.h>
#include <osfmach3/mach_init.h>

extern mach_port_t privileged_host_port;

#include <linux/config.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/tty.h>
#include <linux/miscdevice.h>
#define CONFIG_QIC02_TAPE 1
#include <linux/tpqic02.h>
#include <linux/ftape.h>
#include <linux/malloc.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/random.h>

#include <asm/segment.h>
#include <asm/io.h>
#include <asm/pgtable.h>

#ifdef CONFIG_SOUND
void soundcard_init(void);
#endif
#ifdef CONFIG_ISDN
void isdn_init(void);
#endif
#ifdef CONFIG_PCWATCHDOG
void pcwatchdog_init(void);
#endif
#ifdef PPC
void adb_init(void);
#endif
#ifdef  CONFIG_GRF_CONSOLE
void grf_init(void);
void hil_init(void);
void gkd_init(void);
#endif

mach_port_t	osfmach3_physmem_port = MACH_PORT_NULL;
mach_port_t	osfmach3_physmem_memory_object = MACH_PORT_NULL;

void
osfmach3_physmem_init(void)
{
	kern_return_t	kr;

	if (osfmach3_physmem_port != MACH_PORT_NULL) {
		return;
	}

#ifdef	CONFIG_PMAC_CONSOLE
	return;
#endif	/* CONFIG_PMAC_CONSOLE */

	kr = device_open(device_server_port,
			 MACH_PORT_NULL,
			 D_READ | D_WRITE,
			 server_security_token,
			 "physmem0",
			 &osfmach3_physmem_port);
	if (kr != D_SUCCESS) {
		if (kr != D_NO_SUCH_DEVICE) {
			MACH3_DEBUG(2, kr,
				    ("osfmach3_physmem_init: "
				     "device_open(\"physmem0\")"));
			printk("osfmach3_physmem_init: "
			       "can't open Mach's physical memory\n");
		}
	} else {
		kr = device_map(osfmach3_physmem_port,
				VM_PROT_READ | VM_PROT_WRITE,
				0,
				PAGE_SIZE,	/* XXX not used (I hope !) */
				&osfmach3_physmem_memory_object,
				0);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("osfmach3_physmem_init: "
				     "device_map(port=0x%x, size=0x%lx)",
				     osfmach3_physmem_port, PAGE_SIZE));
			printk("can't map Mach's physical memory\n");
		}
	}
}

static int read_ram(struct inode * inode, struct file * file, char * buf, int count)
{
	return -EIO;
}

static int write_ram(struct inode * inode, struct file * file, const char * buf, int count)
{
	return -EIO;
}

static int read_mem(struct inode * inode, struct file * file, char * buf, int count)
{
	unsigned long p = file->f_pos;
	int read;

#if 0
	p += PAGE_OFFSET;
#endif
	if (count < 0)
		return -EINVAL;
	read = 0;

	if (osfmach3_physmem_memory_object != MACH_PORT_NULL) {
		kern_return_t		kr;
		io_buf_ptr_t		data;
		mach_msg_type_number_t	data_count;

#if DEBUG
		printk("read_mem(0x%lx,%d): will read physmem\n", p, count);
#endif
		kr = device_read(osfmach3_physmem_port,
				 0,
				 (recnum_t) p,
				 count,
				 &data,
				 &data_count);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(2, kr,
				    ("read_mem(0x%lx, %d): device_read",
				     p, count));
			return 0;
		}
		if (count > data_count) {
			count = data_count;
		}
		memcpy_tofs(buf, (char *) data, count);
		kr = vm_deallocate(mach_task_self(),
				   (vm_offset_t) data, data_count);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("read_mem: vm_deallocate(0x%x,%d)",
				     (vm_offset_t) data, data_count));
		}
	} else {
#if	defined(__i386__)
		if (p >= 0xa0000 && p < 0xa0000 + osfmach3_video_map_size) {
			if (count > 0xa0000 + osfmach3_video_map_size - p)
				count = 0xa0000 + osfmach3_video_map_size - p;
			memcpy_tofs(buf,
				    (void *) osfmach3_video_map_base
				    + p - 0xa0000,
				    count);
		} else
#endif	/* i386 */
		{
			p = 0;	/* just to avoid a warning */
			printk("read_mem(offset=0x%lx): not implemented\n", p);
			return 0;
		}
	}
	read += count;
	file->f_pos += read;
	return read;
}

static int write_mem(struct inode * inode, struct file * file, const char * buf, int count)
{
	printk("write_mem: not implemented !\n");
	return 0;
}

static int mmap_mem(struct inode * inode, struct file * file, struct vm_area_struct * vma)
{
	if (vma->vm_offset & ~PAGE_MASK)
		return -ENXIO;
	if (osfmach3_physmem_memory_object != MACH_PORT_NULL ||
	    (osfmach3_video_memory_object != MACH_PORT_NULL &&
	     vma->vm_offset >= PAGE_TRUNC(osfmach3_video_physaddr) &&
	     vma->vm_offset + (vma->vm_end - vma->vm_start) <=
	     osfmach3_video_physaddr + osfmach3_video_map_size)) {
		kern_return_t	kr;
		struct i_mem_object *imo;

		/*
		 * This process wants access to the memory, probably
		 * for efficiency (and most probably to access the 
		 * video memory), so make it unswappable to avoid
		 * undesireable delays.
		 */
		kr = task_swappable(privileged_host_port,
				    current->osfmach3.task->mach_task_port,
				    FALSE);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("P%d[%s] mmap_mem: task_swappable(0x%x)\n",
				     current->pid, current->comm,
				     current->osfmach3.task->mach_task_port));
		}

		if (osfmach3_physmem_memory_object == MACH_PORT_NULL) {
			/*
			 * XXX hack hack hack:
			 * mapping video memory.
			 */
			if (osfmach3_video_memory_object
			    == MEMORY_OBJECT_NULL) {
				return -EINVAL;
			}
		}
		kr = vm_deallocate(current->osfmach3.task->mach_task_port,
				   vma->vm_start,
				   vma->vm_end - vma->vm_start);
		if (kr != KERN_SUCCESS) {
			MACH3_DEBUG(1, kr,
				    ("mmap_mem(video): "
				     "vm_deallocate(0x%lx, 0x%lx)",
				     vma->vm_start,
				     vma->vm_end - vma->vm_start));
			return -EINVAL;
		}

		imo = inode->i_mem_object;
		if (imo == NULL) {
			imo = imo_create(inode, FALSE);
			if (imo == NULL) {
				return -EAGAIN;
			}
		}
		imo->imo_cacheable = FALSE;
		if (osfmach3_physmem_memory_object == MACH_PORT_NULL) {
			imo->imo_mem_obj = osfmach3_video_memory_object;
			/* offset is relative to mem_obj now */
			vma->vm_offset -= PAGE_TRUNC(osfmach3_video_physaddr);
		} else {
#if	DEBUG
			printk("mmap_mem(0x%lx,%ld): will vm_map physmem\n",
			       vma->vm_offset, vma->vm_end - vma->vm_start);
#endif
			imo->imo_mem_obj = osfmach3_physmem_memory_object;
		}
		imo->imo_mem_obj_control = MACH_PORT_DEAD;
		imo_ref(imo);	/* one ref to stay alive */
		imo_ref(imo);	/* one for the inode_pager_release later */
	} else {
		printk("mmap_mem(0x%lx) not implemented\n", vma->vm_offset);
		return -EINVAL;
	}

	vma->vm_inode = inode;
	inode->i_count++;
	return 0;
}

static int read_kmem(struct inode *inode, struct file *file, char *buf, int count)
{
	printk("read_kmem: not implemented !\n");
	return 0;
}

static int read_port(struct inode * inode,struct file * file,char * buf, int count)
{
	printk("read_port: not implemented !\n");
	return 0;
}

static int write_port(struct inode * inode,struct file * file, const char * buf, int count)
{
	printk("write_port: not implemented !\n");
	return 0;
}

static int read_null(struct inode * node, struct file * file, char * buf, int count)
{
	return 0;
}

static int write_null(struct inode * inode, struct file * file, const char * buf, int count)
{
	return count;
}

static int read_zero(struct inode * node, struct file * file, char * buf, int count)
{
	int left;

#ifdef	CONFIG_OSFMACH3
	for (left = count; left > 0; ) {
		extern const char *osfmach3_zero_page;
		unsigned long n;

		if (left > PAGE_SIZE)
			n = PAGE_SIZE;
		else
			n = left;
		memcpy_tofs(buf, osfmach3_zero_page, n);
		buf += n;
		left -= n;
		if (need_resched)
			schedule();
	}
#else	/* CONFIG_OSFMACH3 */
	for (left = count; left > 0; left--) {
		put_user(0,buf);
		buf++;
		if (need_resched)
			schedule();
	}
#endif	/* CONFIG_OSFMACH3 */
	return count;
}

static int mmap_zero(struct inode * inode, struct file * file, struct vm_area_struct * vma)
{
	if (vma->vm_flags & VM_SHARED)
		return -EINVAL;
#ifndef	CONFIG_OSFMACH3
	if (zeromap_page_range(vma->vm_start, vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;
#endif	/* CONFIG_OSFMACH3 */
	return 0;
}

static int read_full(struct inode * node, struct file * file, char * buf,int count)
{
	file->f_pos += count;
	return count;
}

static int write_full(struct inode * inode, struct file * file, const char * buf, int count)
{
	return -ENOSPC;
}

/*
 * Special lseek() function for /dev/null and /dev/zero.  Most notably, you can fopen()
 * both devices with "a" now.  This was previously impossible.  SRB.
 */

static int null_lseek(struct inode * inode, struct file * file, off_t offset, int orig)
{
	return file->f_pos=0;
}
/*
 * The memory devices use the full 32/64 bits of the offset, and so we cannot
 * check against negative addresses: they are ok. The return value is weird,
 * though, in that case (0).
 *
 * also note that seeking relative to the "end of file" isn't supported:
 * it has no meaning, so it returns -EINVAL.
 */
static int memory_lseek(struct inode * inode, struct file * file, off_t offset, int orig)
{
	switch (orig) {
		case 0:
			file->f_pos = offset;
			return file->f_pos;
		case 1:
			file->f_pos += offset;
			return file->f_pos;
		default:
			return -EINVAL;
	}
	if (file->f_pos < 0)
		return 0;
	return file->f_pos;
}

#define write_kmem	write_mem
#define mmap_kmem	mmap_mem
#define zero_lseek	null_lseek
#define write_zero	write_null

static struct file_operations ram_fops = {
	memory_lseek,
	read_ram,
	write_ram,
	NULL,		/* ram_readdir */
	NULL,		/* ram_select */
	NULL,		/* ram_ioctl */
	NULL,		/* ram_mmap */
	NULL,		/* no special open code */
	NULL,		/* no special release code */
	NULL		/* fsync */
};

static struct file_operations mem_fops = {
	memory_lseek,
	read_mem,
	write_mem,
	NULL,		/* mem_readdir */
	NULL,		/* mem_select */
	NULL,		/* mem_ioctl */
	mmap_mem,
	NULL,		/* no special open code */
	NULL,		/* no special release code */
	NULL		/* fsync */
};

static struct file_operations kmem_fops = {
	memory_lseek,
	read_kmem,
	write_kmem,
	NULL,		/* kmem_readdir */
	NULL,		/* kmem_select */
	NULL,		/* kmem_ioctl */
	mmap_kmem,
	NULL,		/* no special open code */
	NULL,		/* no special release code */
	NULL		/* fsync */
};

static struct file_operations null_fops = {
	null_lseek,
	read_null,
	write_null,
	NULL,		/* null_readdir */
	NULL,		/* null_select */
	NULL,		/* null_ioctl */
	NULL,		/* null_mmap */
	NULL,		/* no special open code */
	NULL,		/* no special release code */
	NULL		/* fsync */
};

static struct file_operations port_fops = {
	memory_lseek,
	read_port,
	write_port,
	NULL,		/* port_readdir */
	NULL,		/* port_select */
	NULL,		/* port_ioctl */
	NULL,		/* port_mmap */
	NULL,		/* no special open code */
	NULL,		/* no special release code */
	NULL		/* fsync */
};

static struct file_operations zero_fops = {
	zero_lseek,
	read_zero,
	write_zero,
	NULL,		/* zero_readdir */
	NULL,		/* zero_select */
	NULL,		/* zero_ioctl */
	mmap_zero,
	NULL,		/* no special open code */
	NULL		/* no special release code */
};

static struct file_operations full_fops = {
	memory_lseek,
	read_full,
	write_full,
	NULL,		/* full_readdir */
	NULL,		/* full_select */
	NULL,		/* full_ioctl */	
	NULL,		/* full_mmap */
	NULL,		/* no special open code */
	NULL		/* no special release code */
};

static int memory_open(struct inode * inode, struct file * filp)
{
	switch (MINOR(inode->i_rdev)) {
		case 0:
			filp->f_op = &ram_fops;
			break;
		case 1:
			filp->f_op = &mem_fops;
			break;
		case 2:
			filp->f_op = &kmem_fops;
			break;
		case 3:
			filp->f_op = &null_fops;
			break;
		case 4:
			filp->f_op = &port_fops;
			break;
		case 5:
			filp->f_op = &zero_fops;
			break;
		case 7:
			filp->f_op = &full_fops;
			break;
		case 8:
			filp->f_op = &random_fops;
			break;
		case 9:
			filp->f_op = &urandom_fops;
			break;
		default:
			return -ENXIO;
	}
	if (filp->f_op && filp->f_op->open)
		return filp->f_op->open(inode,filp);
	return 0;
}

static struct file_operations memory_fops = {
	NULL,		/* lseek */
	NULL,		/* read */
	NULL,		/* write */
	NULL,		/* readdir */
	NULL,		/* select */
	NULL,		/* ioctl */
	NULL,		/* mmap */
	memory_open,	/* just a selector for the real open */
	NULL,		/* release */
	NULL		/* fsync */
};

int chr_dev_init(void)
{
	osfmach3_physmem_init();
	if (register_chrdev(MEM_MAJOR,"mem",&memory_fops))
		printk("unable to get major %d for memory devs\n", MEM_MAJOR);
	rand_initialize();
	tty_init();
#ifdef CONFIG_PRINTER
	lp_init();
#endif
#if defined (CONFIG_BUSMOUSE) || defined(CONFIG_UMISC) || \
    defined (CONFIG_PSMOUSE) || defined (CONFIG_MS_BUSMOUSE) || \
    defined (CONFIG_ATIXL_BUSMOUSE) || defined(CONFIG_SOFT_WATCHDOG) || \
    defined (CONFIG_PCWATCHDOG) || \
    defined (CONFIG_APM) || defined (CONFIG_RTC) || defined (CONFIG_SUN_MOUSE)
	misc_init();
#endif
#ifdef CONFIG_SOUND
	soundcard_init();
#endif
#if CONFIG_QIC02_TAPE
	qic02_tape_init();
#endif
#if CONFIG_ISDN
	isdn_init();
#endif
#ifdef CONFIG_FTAPE
	ftape_init();
#endif
#ifdef PPC
	adb_init();
#endif

#ifdef  CONFIG_GRF_CONSOLE	
	grf_init();
	hil_init();
	gkd_init();
#endif
	return 0;
}
