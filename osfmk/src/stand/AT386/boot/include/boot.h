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
/* CMU_HIST */
/*
 * Revision 2.1.2.1  92/04/30  11:53:01  bernadat
 * 	Use mk inludes only
 * 	Copied from main line
 * 	[92/03/19            bernadat]
 * 
 * Revision 2.2  92/04/04  11:35:03  rpd
 * 	Fabricated from 3.0 bootstrap.  But too many things are global.
 * 	[92/03/30            mg32]
 * 
 */
/* CMU_ENDHIST */
/*
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 */
#ifndef	_BOOT_H_
#define	_BOOT_H_

#ifndef __LANGUAGE_ASSEMBLY

#include <ufs/fs.h>
#include <ufs/dinode.h>
#include <mach/machine/vm_param.h>
#include <mach/boot_info.h>
#include <mach/thread_status.h>

struct inode {
	struct dinode i_di;
#define i_size i_di.di_size
#define i_mode i_di.di_mode
#define i_db i_di.di_db
#define i_ib i_di.di_ib
};

extern struct fs *fs;
extern struct inode inode;
extern int unit, part, dos_part, boff, poff, bnum, cnt, active_partition;
extern char dev;

#endif /* __LANGUAGE_ASSEMBLY */

#define BOOT_LINE_LENGTH	256
#define	KERNEL_BOOT_ADDR	0x100000	/* load at 1 Megabyte */
#define	MAXREGIONS		8
#define	STACK_SIZE		(64*1024)	/* 64k stack */
#define STACK_BASE		(VM_MAX_ADDRESS-STACK_SIZE)
#define STACK_PTR		(VM_MAX_ADDRESS-0x10) /* XXX */
#define BOOTSTACK		0xfff0	/* 16 bytes align */
#define BOOTOFFSET		0x1000	
#define KALLOC_OFFSET		0x10000	/* take the next segment after stack */

#define	RB_ALTBOOT	0x40

/* Device IDs */

#define	DEV_HD		'h'
#define	DEV_FLOPPY	'f'
#define DEV_SD		's'
#define DEV_WD		'w'

#ifndef __LANGUAGE_ASSEMBLY

/*
 * asm.s
 */
extern void outb(int, int);
extern int inb(int);
extern void startprog(int, ...);
extern void pcpy(const void *, void *, int);
extern void pclr(void *, int);

/*
 * bios.s
 */
extern int biosread(int, int, int, int, int, int);
extern void putc(int);
extern int getc(void);
extern int ischar(void);
extern int get_diskinfo(int);
extern int memsize(int);
extern void com_putc(int);
extern int com_getc(void);
extern void com_init(int);
extern int com_getstat(void);
extern int read_ticks(unsigned *);
extern void set_ticks(unsigned);

/*
 * boot.c
 */
extern void boot(int);
extern char *itoa(char *, unsigned int);

/*
 * disk.c
 */
extern int disk_open(void);
extern int read_partitions(void);
extern int devfs(void);
extern int badsect(int);
extern int disk_read(int, int, vm_offset_t);

/*
 * io.c
 */
extern void 	gateA20(void);
extern int 	is_intr_char(void);
extern int 	getchar(void);
extern void 	flush_char(void);
extern void 	gets(char *);
extern int 	strncmp(const char *, const char *, size_t);
extern int 	strlen(const char *);
extern void 	bcopy(const char *, char *, int);
extern void 	printf(const char *, ...);
extern int 	cons_is_com;
extern int 	com_enabled;
extern int 	prompt;
extern int 	delayprompt;

/*
 * sys.c
 */
extern int 	read(void *, int);
extern int 	xread(void *, int);
extern int 	openrd(const char *, const char **);

/*
 * a_out.c
 */
extern int load_aout(const char *, int *, struct region_desc *);

/*
 * elf.c
 */
extern int load_elf(const char *, int *, struct region_desc *);

/*
 * mach_boot.c
 */

const char *get_env(const char *name, const char *env_start, int env_size);

char *is_sub_string(const char *sub_str, const char *str, int len);

#if	DEBUG
extern	int	debug;
#endif	/* DEBUG */

#endif /* __LANGUAGE_ASSEMBLY */

#endif	/* _BOOT_H_ */
