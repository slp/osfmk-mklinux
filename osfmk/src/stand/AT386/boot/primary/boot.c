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
 * Revision 2.1.2.1  92/04/30  11:52:49  bernadat
 * 	Add default kernel names.
 * 	Try default name when typed name is wrong.
 * 	[92/04/29            bernadat]
 * 
 * 	Merged in MK tree.
 * 	Fixed code to use mk includes only (not sys/inode.h ....).
 * 	Do not loop on default names if user entered a bad file name.
 * 	Fix maj/unit assignments.
 * 	Pass complete typed line to kernel
 * 	Copied from main line
 * 	[92/03/19            bernadat]
 * 
 * Revision 2.2  92/04/04  11:34:37  rpd
 * 	Change date in banner.
 * 	[92/04/03  16:51:14  rvb]
 * 
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[92/04/03            rvb]
 * 	From 2.5 version.
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

/*
  Copyright 1988, 1989, 1990, 1991, 1992 
   by Intel Corporation, Santa Clara, California.

                All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "boot.h"
#include "bios.h"
#include <sys/reboot.h>

static struct region_desc regions[MAXREGIONS];
int region_count;

struct thread_syscall_state thread_state;

static char boot_line[BOOT_LINE_LENGTH];

#ifdef DEBUG
int debug;
#endif

#define	MACH_BOOT	"/stand/mach_boot"	/* ELF program to boot */

#define BOOT_ENV_COMPAT 1	/* enables compatibility with previous
				   environment conventions */
int cnvmem;
int extmem;
int entry;
int sym_start;

extern int com_initialized;

extern char end[], edata[];

struct prog {
	char name[BOOT_LINE_LENGTH];
	char args[BOOT_LINE_LENGTH];
	const char *basename;
	int sym_start;
	int sym_size;
	int args_start;
	int args_size;
};

static struct 	prog kern_prog;
static int 	loadprog(int *, struct prog *);
static void 	usage(void);
static char 	*copyargs(char *p, char **argsp);


static int 	getbootline(char *);
#define		fsize()	inode.i_size

static int
align(int addr)
{
	if (addr & (sizeof(int)-1)) {
		pclr((void *) addr, sizeof(int) - (addr & (sizeof(int)-1)));
		addr += sizeof(int) - (addr & (sizeof(int)-1));
	}
	return addr;
}

extern char com_line_stat;

void
boot(int drive)
{
	int 		addr;
	int 		kern_entry;
	int 		tentatives = 0;
	int 		i;
	int		bootdev;

	/*
	 * Clear bss first
	 */
	for (i = 0; i < end - edata; i++)
		edata[i] = 0;

#if	REMOTE
	com_enabled = 1;
#endif	/* REMOTE */

	delayprompt = 10;	/* default to 10s at the prompt */

	dev = (drive&BIOS_DEV_WIN) ? DEV_HD : DEV_FLOPPY;
	cnvmem = memsize(0);
	extmem = memsize(1);
	printf("\nPrimary "
#if   	REMOTE
	       "rboot"
#else	/* REMOTE */
	       "boot"
#endif	/* REMOTE */
	       "\n>> %d/%dk (? for help, ^C for intr)\n",
	       cnvmem, extmem);
	gateA20();
	if (dev == DEV_FLOPPY && devfs()) {
		printf("No fd FS, using hd\n");
		dev = DEV_HD;
	}

	if (dev != DEV_FLOPPY) {
		read_partitions();
		part = active_partition; 
	}


	bcopy(MACH_BOOT, kern_prog.name, sizeof(MACH_BOOT));

	for (;;) {
		if (tentatives++)
			getbootline(kern_prog.name);

		addr = KERNEL_BOOT_ADDR;
		if (loadprog(&addr, &kern_prog))
			continue;
		kern_entry = entry;
	       	bootdev = (((dev == DEV_FLOPPY) ?
			    1 : ((dev == DEV_SD) ? 4 : 3)) << B_TYPESHIFT) |
			      (unit << B_UNITSHIFT) |
				(part << B_PARTITIONSHIFT);
#ifdef DEBUG
		if (debug) {
			printf("startprog(\n");
			printf("    entry 0x%x,\n", kern_entry);
			printf("    bootdev, 0x%x\n", bootdev);
			printf("    extmem 0x%x,\n", extmem);
			printf("    cnvmem 0x%x,\n", cnvmem);
		        printf("    howto 0x%x,\n", 0); /*howto*/
		        printf("    esym 0x%x)\n",
			       kern_prog.sym_start+kern_prog.sym_size);
			getchar();
			continue;
		}
#endif /* DEBUG */

		startprog(
			  kern_entry,
			  bootdev,
			  extmem,
		          cnvmem,
		          com_enabled?RB_ALTBOOT:0, /*howto*/
		          kern_prog.sym_start+kern_prog.sym_size);
	}
}

char *
itoa(char *p, unsigned int n)
{
	char numbuf[32];
	char *q;

	q = numbuf;
	do
		*q++ = '0' + n % 10;
	while (n /= 10);
	do
		*p++ = *--q;
	while (q != numbuf);
	*p = 0;
	return p;
}

static int
loadprog(int *addrp, struct prog *p)
{
	if (openrd(p->name, &p->basename)) {
		printf("Can't find %s\n", p->name);
		usage();
		return 1;
	}
  	printf("Loading %cd(%d,%c)%s\n", 
	       dev, unit, 'a'+ part, p->basename);
	if (load_elf(p->name, addrp, regions)) {
		printf("Can't load %s\n", p->name);
		usage();
		return 1;
	}
	p->sym_start = sym_start;
	p->sym_size = *addrp - sym_start;

	p->args_start = *addrp;
	pcpy(p->args, (void *) p->args_start, p->args_size);
	*addrp = align((*addrp) + p->args_size);
	return 0;
}

void
usage(void)
{
	printf("\n\
Usage: [boot prog]\n");
}

static char *
copyargs(char *p, char **argsp)
{
	char *args = *argsp;

	do {
		*args++ = *p++;
	} while (*p && *p != ' ');
	*args++ = 0;
	*argsp = args;
	while (*p == ' ')
		p++;
	return p;
}

static char *
copyword(char *p, char *copy)
{
	if (*p == 0 || *p == '-')
		return p;
	return copyargs(p, &copy);
}

int
getbootline(char *kernel)
{
  	int 	rc = 1;
	char 	*ptr;

	for (;;) {
		if (com_enabled)
			flush_char();
		printf("\nboot: ");
		gets(ptr = boot_line);
		if (!*ptr)
			rc = 0;
		while (*ptr == ' ')
			ptr++;
		if (*ptr == '?') {
			usage();
			continue;
		}
#ifdef DEBUG
		if (*ptr == '~') {
			debug = !debug;
			continue;
		}
#endif
		ptr = copyword(ptr, kernel);
		return(rc);
	}
}
