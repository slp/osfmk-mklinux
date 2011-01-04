/*
 *  linux/arch/ppc/kernel/setup.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Adapted from 'alpha' version by Gary Thomas
 */

/*
 * bootup setup stuff..
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/ldt.h>
#include <linux/user.h>
#include <linux/a.out.h>
#include <linux/tty.h>

#define SIO_CONFIG_RA	0x398
#define SIO_CONFIG_RD	0x399

#include <asm/pgtable.h>
#include <asm/residual.h>
#include <asm/processor.h>

extern unsigned long *end_of_DRAM;
extern PTE *Hash;
extern unsigned long Hash_size, Hash_mask;
extern unsigned long _TotalMemory;
extern unsigned long isBeBox[];

RESIDUAL _Residual;

char sda_root[] = "root=/dev/sda1";
extern int root_mountflags;
kdev_t DEFAULT_ROOT_DEV;

unsigned char aux_device_present;

/*
 * The format of "screen_info" is strange, and due to early
 * i386-setup code. This is just enough to make the console
 * code think we're on a EGA+ colour display.
 */
struct screen_info screen_info = {
	0, 0,			/* orig-x, orig-y */
	0, 0,			/* unused */
	0,			/* orig-video-page */
	0,			/* orig-video-mode */
	80,			/* orig-video-cols */
	0,			/* unused [short] */
	0,			/* ega_bx */
	0,			/* unused [short] */
	25,			/* orig-video-lines */
	0,			/* isVGA */
	16			/* video points */
};

unsigned long bios32_init(unsigned long memory_start, unsigned long memory_end)
{
	return memory_start;
}

unsigned long find_end_of_memory(void)
{
	unsigned long total;
#if 0
	unsigned char dram_size = inb(0x0804);
_printk("DRAM Size = %x\n", dram_size);
_printk("Config registers = %x/%x/%x/%x\n", inb(0x0800), inb(0x0801), inb(0x0802), inb(0x0803));
	switch (dram_size & 0x07)
	{
		case 0:
			total = 0x08000000;  /* 128M */
			break;
		case 1:
			total = 0x02000000;  /* 32M */
			break;
		case 2:
			total = 0x00800000;  /* 8M */
			break;
		case 3:
			total = 0x00400000;  /* 4M - can't happen! */
			break;
		case 4:
			total = 0x10000000;  /* 256M */
			break;
		case 5:
			total = 0x04000000;  /* 64M */
			break;
		case 6:
			total = 0x01000000;  /* 16M */
			break;
		case 7:
			total = 0x04000000;  /* Can't happen */
			break;
	}
	switch ((dram_size>>4) & 0x07)
	{
		case 0:
			total += 0x08000000;  /* 128M */
			break;
		case 1:
			total += 0x02000000;  /* 32M */
			break;
		case 2:
			total += 0x00800000;  /* 8M */
			break;
		case 3:
			total += 0x00000000;  /* Module not present */
			break;
		case 4:
			total += 0x10000000;  /* 256M */
			break;
		case 5:
			total += 0x04000000;  /* 64M */
			break;
		case 6:
			total += 0x01000000;  /* 16M */
			break;
		case 7:
			total += 0x00000000;  /* Module not present */
			break;
	}
/* TEMP */ total = 0x01000000;
/* _cnpause();	*/
#else
	if (isBeBox[0]) {
	  _TotalMemory = 0x01000000;  /* Fix me! */
	}
	total = _TotalMemory;
	if ((_get_PVR() >> 16) == 1) {
	  /* Primitive mappings only 16 Mbytes! */
	  total = 0x01000000;
	}
	/* Determine machine type */
	if (isBeBox[0]) {
	  _Processor = _PROC_Be;
	} else
        if (strncmp(_Residual.VitalProductData.PrintableModel, "IBM", 3) == 0) {
	  _Processor = _PROC_IBM;
	} else {
	  _Processor = _PROC_Motorola;
	}
	_printk("Total Memory = %x\n", total);
	_printk("Processor type = %d\n", _Processor);
#endif
/* CAUTION!! This can be done more elegantly! */	
	if (total < 0x01000000)	
	{
		Hash_size = HASH_TABLE_SIZE_64K;
		Hash_mask = HASH_TABLE_MASK_64K;
	} else
	if (total < 0x02000000)
	{
		Hash_size = HASH_TABLE_SIZE_128K;
		Hash_mask = HASH_TABLE_MASK_128K;
	} else
	{
		Hash_size = HASH_TABLE_SIZE_256K;
		Hash_mask = HASH_TABLE_MASK_256K;
	}
	Hash = (PTE *)((total-Hash_size)+KERNELBASE);
_printk("Hash at %x, Size: %x\n", Hash, Hash_size);
	bzero(Hash, Hash_size);
	return ((unsigned long)Hash);
}

int size_memory;

/* #define DEFAULT_ROOT_DEVICE 0x0200	/* fd0 */
#define DEFAULT_ROOT_DEVICE 0x0801	/* sda1 */

void setup_arch(char **cmdline_p,
	unsigned long * memory_start_p, unsigned long * memory_end_p)
{
	extern int _end;
	extern char cmd_line[];
	unsigned char reg;

	/* Set up floppy in PS/2 mode */
	outb(0x09, SIO_CONFIG_RA);
	reg = inb(SIO_CONFIG_RD);
	reg = (reg & 0x3F) | 0x40;
	outb(reg, SIO_CONFIG_RD);
	outb(reg, SIO_CONFIG_RD);	/* Have to write twice to change! */
	set_root_dev();
	DEFAULT_ROOT_DEV = ROOT_DEV;
	aux_device_present = 0xaa;
	*cmdline_p = cmd_line;
	*memory_start_p = (unsigned long) &_end;
	*memory_end_p = (unsigned long *)end_of_DRAM;
	size_memory = *memory_end_p - KERNELBASE;  /* Relative size of memory */
}

asmlinkage int sys_ioperm(unsigned long from, unsigned long num, int on)
{
	return -EIO;
}

extern char builtin_ramdisk_image;
extern long builtin_ramdisk_size;

void
builtin_ramdisk_init(void)
{
	if ((ROOT_DEV == DEFAULT_ROOT_DEV) && (builtin_ramdisk_size != 0))
	{
		rd_preloaded_init(&builtin_ramdisk_image, builtin_ramdisk_size);
	} else
	{  /* Not ramdisk - assume root needs to be mounted read only */
		root_mountflags |= MS_RDONLY;
	}

}

#define MAJOR(n) (((n)&0xFF00)>>8)
#define MINOR(n) ((n)&0x00FF)

int
get_cpuinfo(char *buffer)
{
	int pvr = _get_PVR();
	char *model;
	switch (pvr>>16)
	{
		case 1:
		  model = "601";
		  break;
		case 3:
			model = "603";
			break;
		case 4:
			model = "604";
			break;
		case 6:
			model = "603e";
			break;
		case 7:
			model = "603ev";
			break;
		default:
			model = "unknown";
			break;
	}
	return sprintf(buffer, "PowerPC %s rev %d.%d\n", model, MAJOR(pvr), MINOR(pvr));
}
