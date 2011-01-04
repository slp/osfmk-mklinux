/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

#ifndef _BOOT_H_
#define _BOOT_H_

#ifdef __MWERKS__
#include "boot_info.h"
#else
#include <mach/boot_info.h>
#include <mach/vm_param.h>
#endif

#define BOOT_LINE_LENGTH        256
#define MAXBSIZE                1024

/*
 * Arguments to reboot system call.
 * These are converted to switches, and passed to startup program,
 * and on to init.
 */
#define RB_AUTOBOOT     0       /* flags for system auto-booting itself */

#define RB_ASKNAME      0x01    /* -a: ask for file name to reboot from */
#define RB_SINGLE       0x02    /* -s: reboot to single user only */
#define RB_KDB          0x04    /* -d: kernel debugger symbols loaded */
#define RB_HALT         0x08    /* -h: enter KDB at bootup */
                                /*     for host_reboot(): don't reboot,
                                       just halt */
#define RB_INITNAME     0x10    /* -i: name given for /etc/init (unused) */
#define RB_DFLTROOT     0x20    /*     use compiled-in rootdev */
#define RB_NOBOOTRC     0x20    /* -b: don't run /etc/rc.boot */
#define RB_ALTBOOT      0x40    /*     use /boot.old vs /boot */
#define RB_UNIPROC      0x80    /* -u: start only one processor */

#define RB_SHIFT        8       /* second byte is for server */


#define RB_DEBUGGER     0x1000  /*     for host_reboot(): enter kernel
                                       debugger from user level */


#define MAXREGIONS		8

#define BOOT_STACK_SIZE              (1024*1024)       /* 1M stack! NMGS TODO*/
#define BOOT_STACK_BASE              (VM_MAX_ADDRESS - BOOT_STACK_SIZE)
/* TODO NGMS we should put information onto the top of the boot stack */
#define BOOT_STACK_PTR               (VM_MAX_ADDRESS - 0x10)

/* These defines are taken empirically from an objdump of a gcc-compiled
 * binary. In theory, the code in elf.c should check that the blocks
 * appear in the correct order and are correctly named, but we skip this
 * test
 */
#define CODE_BLOCK		0
#define DATA_BLOCK		1
#define RODATA_BLOCK	2
#define BSS_BLOCK		3

#define GOT_PTR_OFFSET  0x8000 /* got ptr points to (got+GOT_PTR_OFFSET) */

typedef struct prog {
    struct region_desc regions[MAXREGIONS];
    int                region_count;
    vm_offset_t	       entry;
    unsigned int       base_addr; 			/* Physical base of object */

    vm_offset_t        args_start;
    vm_offset_t        args_size;
} prog;

/*
 * Video information.. 
 */

struct Boot_Video {
	unsigned long	v_baseAddr;	/* Base address of video memory */
	unsigned long	v_display;	/* Display Code (if Applicable */
	unsigned long	v_rowBytes;	/* Number of bytes per pixel row */
	unsigned long	v_width;	/* Width */
	unsigned long	v_height;	/* Height */
	unsigned long	v_depth;	/* Pixel Depth */
};

typedef struct Boot_Video	Boot_Video;

/* DRAM Bank definitions - describes physical memory layout.
 */
#define	kMaxDRAMBanks			26			/* maximum number of DRAM banks */

struct DRAMBank
{
	unsigned long	base;					/* physical base of DRAM bank */
	unsigned long	size;					/* size of bank */
};
typedef struct DRAMBank DRAMBank;

/* Boot argument structure - passed into Mach kernel at boot time.
 *  Current version is 2.
 */
#define KBOOTARGS_VERSION_PDM_ONLY	1
#define	kBootHaveOFWVersion		4	/* Starting version which the OFW device tree is passed in*/
#define kBootArgsVersion		4

typedef struct boot_args {
  unsigned int		Version;	/* Version of boot_args structure */
  unsigned long		Size;		/* Size of boot_args structure */
  prog			kern_info;	/* Kernel elf info */
  prog			task_info;	/* Bootstrap task elf info */
  char			CommandLine[BOOT_LINE_LENGTH];	/* Passed in command line */
  DRAMBank		PhysicalDRAM[kMaxDRAMBanks];	/* base and range pairs for the 26 DRAM banks */
  unsigned int		first_avail;	/* Unused */
  Boot_Video		Video;		/* Video Information */
  unsigned long 	machineType;	/* Machine Type */
  long			deviceTreeSize;						/* Size of the device tree */
  unsigned long         deviceTreeBuffer;
} boot_args;

extern boot_args passed_args;

#endif /* _BOOT_H_ */
