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
#include "versiondate.h"
#include "boot.h"
#include "bios.h"
#include <sys/reboot.h>
#include "secondary/net.h"

static struct region_desc regions[MAXREGIONS];
int region_count;

struct thread_syscall_state thread_state;

static char boot_line[BOOT_LINE_LENGTH];

void reset_pic(void);

#define	MACH_KERNEL	"/mach_kernel"		   /* mach_kernel to boot */
#define	BOOTSTRAP	"/mach_servers/bootstrap"  /* bootstrap to boot */
#define	MACH_BOOT_ENV	"/stand/mach_boot.env"	   /* environment to boot */

#ifdef DEBUG
int debug;
#endif
#define	BDEV(dev,unit)	(((dev)==DEV_FLOPPY ? BIOS_DEV_FLOPPY:BIOS_DEV_WIN)+(unit))

#define BOOT_ENV_COMPAT 1	/* enables compatibility with previous
				   environment conventions */
int cnvmem;
int extmem;
int entry;
int sym_start;

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
static int 	loadtext(int *, struct prog *);
static void 	usage(void);
static char 	*copyargs(char *p, char **argsp);
int	atoi(const char *);


static int 	getbootline(char *, char *);
static struct 	prog boot_prog;
static struct 	prog env_prog;

static int
align(int addr)
{
	if (addr & (sizeof(int)-1)) {
		pclr((void *) addr, sizeof(int) - (addr & (sizeof(int)-1)));
		addr += sizeof(int) - (addr & (sizeof(int)-1));
	}
	return addr;
}

static int
page_align(int addr)
{
	int newaddr = i386_round_page(addr);

	if (newaddr != addr)
		pclr((void *) addr, newaddr - addr);
	return newaddr;
}

extern char com_line_stat;

void
boot(int drive)
{
	int 		addr;
	int 		top;
	int 		kern_entry;
	int 		boot_start;
	int 		boot_region_desc;
	int 		boot_thread_state;
	unsigned 	int size;
	int 		env_start;
	int 		env_size = 0;
	char 		*p;
	char 		*env = 0;
	char		*defaults = 0;
	int		defaults_size = 0;
	int 		i;
	int		idt[2];

	delayprompt = 10;	/* default to 10s at the prompt */

	switch(drive) {
	    case BIOS_DEV_FLOPPY:
		dev = DEV_FLOPPY;
		break;
	    case BIOS_FAKE_WD:
		dev = DEV_WD;
		break;
	    case BIOS_DEV_WIN:
	    default:
		dev = DEV_HD;
		break;
	}
	cnvmem = memsize(0);
	extmem = memsize(1);

	printf("\n>> Secondary Mach boot %s\n>> %d/%dk (? for help, ^C for intr)\n",
	       VERSIONDATE, cnvmem, extmem);
#ifdef	DEBUG
	printf("end x%x data 0x%x\n",end,edata);
#endif
	/* sanity check: */
	if ((int)end > KALLOC_OFFSET || (int)end > (BOOTSTACK-0x300)) {
		printf("Size problem: 0x%x > 0x%x\n",end,
		       ((int)end > KALLOC_OFFSET)?KALLOC_OFFSET:BOOTSTACK-0x300);
	}

#if	DEBUG
	if (debug) {
		printf("dev = %x\n", dev);
		printf("unit = %x\n", unit);
		printf("part = %x\n", part);
	}
#endif
#if 0
	dev = DEV_HD;
	unit = 0;
	part = 0;
#endif

	reset_pic(); 		/* Lilo breaks PIC, BIOS disk ints fail */

	calibrate_delay();	/* adjust delay for i/o operation */

	gateA20();
	if (dev == DEV_FLOPPY && devfs()) {
		printf("No fd FS, using hd\n");
		dev = DEV_HD;
		part = 0;
	}


	bcopy(MACH_KERNEL, kern_prog.name, sizeof(MACH_KERNEL));
	bcopy(BOOTSTRAP, boot_prog.name, sizeof(BOOTSTRAP));
	bcopy(MACH_BOOT_ENV, env_prog.name, sizeof(MACH_BOOT_ENV));

reload_env_file:
	if (openrd(env_prog.name, NULL) == 0 && fsize() != 0) {
		const char 	*value;
		/*
		 * Read mach_boot environment file if exists
		 */
		printf("Reading defaults from %s:\n",env_prog.name);
		defaults_size = fsize()+1;
		if (defaults != (char *)0)
			free(defaults);
		defaults = (char *)malloc(defaults_size);
		read(defaults, defaults_size-1);
		*(defaults+defaults_size-1) = 0;
		printf("%s", defaults);
		for (p = defaults; p < defaults + defaults_size; p++)
			if (*p == '\n')
				*p = '\0';
		value = get_env("CONSOLE", defaults, defaults_size);
		if (strcmp(value, "vga") == 0 || strcmp(value, "VGA") == 0)
			com_enabled = 0;
		/* WARNING: do not enable the remote console based
		 * on the latter argument in an environment file, since
		 * now, remote console enabling is decided by the primary
		 * boot ONLY and passed along through secondary's.
		 */
		if (*get_env("PROMPT", defaults, defaults_size) == '1')
			prompt = 1;
		if (com_enabled &&
		    (value = get_env("COM1_SETUP", defaults, defaults_size)))
			com_setup(value);
		if (value = get_env("DELAYPROMPT", defaults, defaults_size)) {
			delayprompt = atoi(value);
			/* don't allow stupid values */
			if (delayprompt < 3)
				delayprompt = 3;			
		}
	}
	for (;;) {
	    	if ((!getbootline(kern_prog.name, boot_prog.name)) && defaults ) {
		  	/*
			 * Get defaults from /mach_boot.env if any.
			 */
			const char 	*value;

			if (value = get_env("KERNEL_NAME",
					    defaults, defaults_size)) {
				strcpy(kern_prog.name, (char *)value);
				strcpy(kern_prog.args, (char *)value);
				kern_prog.args_size = strlen(value)+1;
			}
			if (value = get_env("KERNEL_ARGS",
					    defaults, defaults_size)) {
			  	char *args;
				args = kern_prog.args + kern_prog.args_size;
				while (*value)
					value = copyargs((char *)value, &args);
				kern_prog.args_size = args - kern_prog.args;
			}
			if (value = get_env("BOOTSTRAP_NAME",
					    defaults, defaults_size)) {
				strcpy(boot_prog.name, (char *)value);
				strcpy(boot_prog.args, (char *)value);
				boot_prog.args_size = strlen(value)+1;
			}
			if (value = get_env("BOOTSTRAP_ARGS",
					    defaults, defaults_size)) {
			  	char *args;
				args = boot_prog.args + boot_prog.args_size;
				while (*value)
					value = copyargs((char *)value, &args);
				boot_prog.args_size = args - boot_prog.args;
			}
	        }
		if (cons_is_com) {
			printf("console is COM1\n");
			/* check if we already enabled remote console? */
			p = kern_prog.args + kern_prog.args_size;
			*p++ = '-';
			*p++ = 'r';
			*p++ = 0;
			kern_prog.args_size += 3;
	        }

		addr = KERNEL_BOOT_ADDR;
		if (loadtext(&addr, &kern_prog)) {
			strcpy(env_prog.name, kern_prog.name);
			goto reload_env_file;
		} else if (loadprog(&addr, &kern_prog)) {
			printf("Can't load %s\n", kern_prog.name);
			usage();
			continue;
		}
		kern_entry = entry;

		if (dev == DEV_WD)
			net_get_root_device();
		env_start = addr;
		if (openrd("/mach_servers/environment", NULL) == 0 &&
		    fsize() != 0) {
			unsigned int total = fsize()+1;

			printf("Loading environment from /mach_servers/environment\n");
			env = (char *)malloc(total);
			read(env, total-1);
			*(env+total-1) = 0;
			for (p = env; p < env + total; p++)
				if (*p == '\n')
					*p = '\0';
			pcpy(env, (void *)addr, total);
			addr += total;
			env_size += total;
			free(env);
		} 
		env = (char *)malloc(BOOT_LINE_LENGTH);
#if	BOOT_ENV_COMPAT
		/* should go away when all kernels are converted 
		   to use BOOT_DEVICE */
		p = env;
		strcpy(p, "BOOTOFFSET=");
		p = itoa(p + strlen(p), boff) + 1;
		strcpy(p, "BOOTDEV=hd");
		p += strlen(p)+1;
		*(p-3) = dev;
		strcpy(p, "BOOTUNIT=");
		p = itoa(p + strlen(p), unit) + 1;
		strcpy(p, "BOOTPART=");
		p = itoa(p + strlen(p), part) + 1;
		size = p - env;
		pcpy(env, (void *)addr, size);
		addr += size;
		env_size += size;
#endif	/* BOOT_ENV_COMPAT */

		p = env;
		strcpy(p, "BOOT_DEVICE=hd");
		p += strlen(p);
		*(p-2) = dev;
		p = itoa(p, unit);
		*p++ = 'a'+part;

		size = p - env;
		pcpy(env, (void *)addr, size);
		addr += size;
		env_size += size;
		free(env);

		if (strncmp("none",boot_prog.name,sizeof("none"))==0
		    ||strncmp("null",boot_prog.name,sizeof("null"))==0) {
			boot_start = 0;
			boot_region_desc = 0;
			boot_prog.sym_start = 0;
			boot_prog.sym_size = 0;
			boot_prog.args_start = 0;
			boot_prog.args_size = 0;
			region_count = 0;
			boot_thread_state = 0;
		        top = page_align(addr);
			goto boot_kernel_only;
		}
		boot_start = addr = page_align(addr);
		if (loadprog(&addr, &boot_prog)) {
			printf("Can't load %s\n", boot_prog.name);
			usage();
			continue;
		}

		boot_region_desc = addr;
		addr = boot_region_desc + (region_count * sizeof(regions[0]));
		pcpy(regions, (void *) boot_region_desc,
		     addr - boot_region_desc);
		boot_thread_state = addr;
		addr += sizeof(thread_state);
		pcpy(&thread_state, (void *) boot_thread_state,
		     addr - boot_thread_state);

		top = page_align(addr);
boot_kernel_only:
#ifdef DEBUG
		if (debug) {
			printf("startprog(\n");
			printf("    entry 0x%x,\n", kern_entry);
			printf("    -1,\n");
			printf("    extmem 0x%x,\n", extmem);
			printf("    cnvmem 0x%x,\n", cnvmem);
			printf("    kern_sym_start 0x%x,\n",
			       kern_prog.sym_start);
			printf("    kern_sym_size 0x%x,\n",
			       kern_prog.sym_size);
			printf("    kern_args_start 0x%x,\n",
			       kern_prog.args_start);
			printf("    kern_args_size 0x%x,\n",
			       kern_prog.args_size);
			for (p = kern_prog.args;
			     p < &kern_prog.args[kern_prog.args_size];
			     p += strlen(p)+1)
				printf("<%s>", p);
			printf("\n");
			printf("    boot_sym_start 0x%x,\n",
			       boot_prog.sym_start);
			printf("    boot_sym_size 0x%x,\n",
			       boot_prog.sym_size);
			printf("    boot_args_start 0x%x,\n",
			       boot_prog.args_start);
			printf("    boot_args_size 0x%x,\n",
			       boot_prog.args_size);
			for (p = boot_prog.args;
			     p < &boot_prog.args[boot_prog.args_size];
			     p += strlen(p)+1)
				printf("<%s>", p);
			printf("\n");
			printf("    boot_start 0x%x,\n", boot_start);
			printf("    boot_size 0x%x,\n",
			       boot_prog.sym_start - boot_start);
			printf("    boot_region_desc 0x%x,\n",
			       boot_region_desc);
			printf("    boot_region_count 0x%x,\n", region_count);
			printf("    boot_thread_state_flavor %d,\n",
			       THREAD_SYSCALL_STATE);
			printf("    boot_thread_state 0x%x (eip 0x%x, esp 0x%x),\n",
			       boot_thread_state,
			       thread_state.eip, thread_state.esp);
			printf("    boot_thread_state_count %d,\n",
			       (int) i386_THREAD_SYSCALL_STATE_COUNT);
			printf("    env_start 0x%x,\n", env_start);
			printf("    env_size 0x%x,\n", env_size);
			printf("    top 0x%x)\n", (int) top);
getchar();
			continue;
		}
#endif /* DEBUG */

/*
 * New calling convention
 *
 * %esp ->	-1
 *		size of extended memory (K)
 *		size of conventional memory (K)
 *		kern_sym_start
 *		kern_sym_size
 *		kern_args_start
 *		kern_args_size
 *		boot_sym_start
 *		boot_sym_size
 *		boot_args_start
 *		boot_args_size
 *		boot_start
 *		boot_size
 *		boot_region_desc
 *		boot_region_count
 *		boot_thread_state_flavor
 *		boot_thread_state
 *		boot_thread_state_count
 *		env_start
 *		env_size
 *		top of loaded memory
 */
		startprog(
			  kern_entry,
			  -1,
			  extmem, cnvmem,
			  kern_prog.sym_start, kern_prog.sym_size,
			  kern_prog.args_start, kern_prog.args_size,
			  boot_prog.sym_start, boot_prog.sym_size,
			  boot_prog.args_start, boot_prog.args_size,
			  boot_start, boot_prog.sym_start - boot_start,
			  boot_region_desc, region_count,
			  THREAD_SYSCALL_STATE, boot_thread_state,
			  i386_THREAD_SYSCALL_STATE_COUNT,
			  env_start, env_size,
			  top);
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

#define	isspace(c) ((c) == ' ' || (c) == '\t')

int
atoi(const char *str)
{
	register int absval = 0;
	register int sign = 0;
	char *p = (char *)str;

	while (isspace(*p)) p++;
	switch(*p) {
	    case '-':
		sign = 1;
		/*FALLTHROUGH*/
	    case '+':
		p++;
	}
	while (*p >= '0' && *p <= '9')
		absval = (absval * 10) + (*p++ - '0');

	return (sign == 0 ? absval : -absval);
}

static int
loadprog(int *addrp, struct prog *p)
{
#if	DEBUG
	if (debug)
		printf("loadprog(%s)\n", p->name);
#endif	
	if (openrd(p->name, &p->basename)) {
		printf("Can't find %s\n", p->name);
		usage();
		return 1;
	}
	printf("Loading %cd(%d,%c)%s\n", 
	       dev, unit, 'a'+ part, p->basename);
	if (load_elf(p->name, addrp, regions)) {
		return 1;
	}
	p->sym_start = sym_start;
	p->sym_size = *addrp - sym_start;

	p->args_start = *addrp;
	pcpy(p->args, (void *) p->args_start, p->args_size);
	*addrp = align((*addrp) + p->args_size);
	return 0;
}

#define isprint(c)	((((c) >= ' ') && ((c) <= '~')) \
                        || (c) == '\t' || (c) == '\n' || (c) == '\r')

static int
loadtext(int *addrp, struct prog *p)
{
	/*
	 * try to read file as a text file 
	 * (configuration file should be typically less than an arbitrary 10K)
	 */
	if (openrd(p->name, NULL) == 0 && fsize() != 0 && fsize() < 10000) {
		boolean_t textfile = TRUE;
		int	size;
		char	*bf, *p;
		/*
		 * Read file if exists
		 */
		size = fsize()+1;
		bf = (char *)malloc(size);
		read(bf, size-1);
		*(bf+size-1) = 0;
		for (p = bf; (p < bf + size-1) && textfile; p++)
			if (!isprint(*p)) {
				textfile = FALSE;
			}
		free(bf);
		return(textfile);
	}
	return(FALSE);
}

void
usage(void)
{
	printf("\n\
Usage: [kernel] [kernel-flags] [--] [bootstrap] [bootstrap-flags]\n");
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
getbootline(char *kernel, char *bootstrap)
{
  	int 	rc = 1;
	char 	*ptr;
	char 	*kargs = kern_prog.args;
	char 	*bargs = boot_prog.args;

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
		while (*kargs++ = *kernel)
			kernel++;
		while (*ptr == '-') {
			if (*(ptr+1) == '-') {
				ptr += 2;
				while (*ptr == ' ')
					ptr++;
				break;
			}
			ptr = copyargs(ptr, &kargs);
		}
		kern_prog.args_size = kargs - kern_prog.args;
		ptr = copyword(ptr, bootstrap);
		while (*bargs++ = *bootstrap)
			bootstrap++;
		while (*ptr)
			ptr = copyargs(ptr, &bargs);
		boot_prog.args_size = bargs - boot_prog.args;
		return(rc);
	}
}

void
reset_pic(void)
{
	delayed_outb(0x20, 0x11);	/* INT-1, ICW1 */
	delayed_outb(0x21, 0x08);	/* INT-1, ICW2 vector addr 0x20 */
	delayed_outb(0x21, 0x04);	/* INT-1, ICW3 slave connection */
	delayed_outb(0x21, 0x01);	/* INT-1, ICW4 8086 mode */
	delayed_outb(0x21, 0x00);	/* INT-1, Interrupt mask */
	delayed_outb(0xA0, 0x11);	/* INT-2, ICW1 */
	delayed_outb(0xA1, 0x70);	/* INT-2, ICW2 vector addr 0x1c0 */
	delayed_outb(0xA1, 0x02);	/* INT-2, ICW3 slave */
	delayed_outb(0xA1, 0x01);	/* INT-2, ICW4 8086 mode */
	delayed_outb(0xA1, 0x0);	/* INT-2, Interrupt mask */
}
