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

/*
 * Secondary stage boot
 */

#include "boot.h"
#include "bios.h"
#include <sys/reboot.h>
#include <mach/fs/file_system.h>
#include <i386/disk.h>
#include <secondary/net.h>
#include <secondary/net/dlink.h>
extern struct dlink_st dlink;

extern 	char end[], edata[];
char	env[MAXBSIZE];
#define BPS 512

void mach_boot(
	  int 		howto,
	  int 		bootdev,		/* or -1 */
	  char		*env_start,
	  int		env_size);


void
mach_boot(
	  int 		howto,
	  int 		bootdev,		/* or -1 */
	  char		*env_start,
	  int		env_size)
{
	register i;
	const char *c;
	int dev;



	/*
	 * Clear bss first (at the very beginning to avoid saving
	 * and then restoring global variables).
	 */

	for (i = 0; i < end - edata; i++)
		edata[i] = 0;

	if (howto & RB_ALTBOOT) {
		/* enable remote console only if previous boot enabled it */
		com_enabled = 1;
	} else
		com_enabled = 0;

	
	if (bootdev == -1) {
	  	/* New arg passing convention */
		c = get_env("BOOTDEV", env_start, env_size);
		if (*c == 'f')
			dev = BIOS_DEV_FLOPPY;
		else if (*c == 'w')
			dev = BIOS_FAKE_WD;	/* network boot */
		else
			dev = BIOS_DEV_WIN;
		if (c = get_env("BOOTUNIT", env_start, env_size))
			unit = *c - '0';
		if (c = get_env("BOOTPART", env_start, env_size))
			part = *c - '0';
	} else {
	  	/* Old a.out arg passing convention */
		dev = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
		dev = (dev == 1) ? BIOS_DEV_FLOPPY : BIOS_DEV_WIN;
		unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;
		part = (bootdev >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
	}
#if	DEBUG
	if (debug)
		printf("dev %d, unit %d, part %d\n", dev, unit, part);
#endif

	boot(dev);
}

const char *
get_env(
       const char	*name,
       const char	*env_start,
       int		env_size)
{
	int len = strlen(name);
	const char *p = (const char *)env_start;
	const char *endp = p + env_size;

	while (p < endp) {
		if (len >= endp - p)
			break;
		if (strncmp(name, p, len) == 0 && *(p + len) == '=')
			return p + len + 1;
		while (*p++)
			;
	}
	return NULL;
}

char *
is_sub_string(
	      const	char *sub_str,
	      const	char *str,
	      int 	len)
{
	int sub_len = strlen(sub_str);
	const char *end = str + len - sub_len;
  
	while (str <= end)
	  	if (*str == *sub_str && strncmp(str, sub_str, sub_len) == 0)
			return (char *)str;
		else
			str++;
	return (char *)0;
}

/*
 * Following routines convert boot calls to libsa_fs calls:
 *
 *	devfs()
 *	openrd()
 *	fsize()
 *	read()
 */

int
devopen(void)
{
	if (dev == DEV_WD)
		return net_open();
	return disk_open();
}

/*
 * Called from boot() in boot.c.
 * Checks existence of a file system on current device
 */

devfs() {
	if (dev==DEV_WD) {
		return net_open();
	} 
	else
		return(openrd("", 0));
}

struct file fp;

/*
 * Called from boot() and loadprog() in boot.c
 * Opens a file on from current device
 * (boot manages a single file at a time)
 */

int
openrd(const char *name, const char **basename)
{
	const char *cp = name;
	char path_name[BOOT_LINE_LENGTH];
	char *p = path_name;

#ifdef DEBUG
	if (debug)
		printf("openrd(%s)\n", name);
#endif
	while (*cp && *cp!='(')
		cp++;
	if (!*cp)
		cp = name;
	else {
		if (cp++ != name) {
			if (name[1] != 'd' ||
			    (name[0] != 'h' && name[0] != 'f' &&
			     name[0] != 's' && name[0] != DEV_WD)) {
				printf("Unknown device\n");
				return 1;
			}
			dev = name[0];
		}
		if (*cp >= '0' && *cp <= '9')
			if ((unit = *cp++ - '0') > 1) {
				printf("Bad unit\n");
				return 1;
			}
		if (!*cp || (*cp == ',' && !*++cp))
			return 1;
		if (*cp >= 'a' && *cp <= 'p')
			part = *cp++ - 'a';
		while (*cp && *cp++!=')') ;
	}
	if (dev!=DEV_WD) {
		/* 
		 * convert to /dev/<device>/path for libsa_fs
		 */

		strcpy(p, "/dev/?d??");
		*(p+5) = dev;
		*(p+7) = '0'+ unit;
		*(p+8) = 'a' + part;
		p += strlen(p);
	} else {
		part=0;
		unit=0;
	}
	if (*cp) {
		if(*cp != '/')
			*p++ = '/';
		strcpy(p, cp);
	}
	if (dev!=DEV_WD) {
		if (fp.f_private)
			close_file(&fp);
	}

#if DEBUG
	if (debug)
		printf("openrd: open_file(%s)\n", path_name);
#endif
	if (dev != DEV_WD) {
		if (open_file(0, path_name, &fp) != KERN_SUCCESS)
			return 1;
	} else {
		int ret;
		if (net_open() != KERN_SUCCESS) {
		        printf("Can't open net device\n");
			return 1;
		}
		/*
		 * Operation sequence:
		 * - issue a bootp req for the file
		 *   (as a side effect, this initializes the network parms:
		 *    * issue arp for the received server ip addr
		 *    * read vendor specific area to initialize further).
		 * - find the server for this file and download the file
		 */
		p = path_name;
		if (strlen(p) > 128 /* bootp constraint */) {
			printf("openrd: filename too long\n");
			return 1;
		}
	        if ((ret=bootp_request(p,(char *)0, (char *)0)) != NETBOOT_SUCCESS)
			return 1;

		if ((ret=net_openfile(p)) != NETBOOT_SUCCESS)
			return 1;
	}

	poff = 0;
	if (basename)
		*basename = cp;
	return 0;
}

/*
 * return current file size.
 */

int
fsize(void)
{
	if (dev == DEV_WD)
		return BPS; /* XXX should return vendor area's filesize
			   * or tftp filesize (must solve bootp bug first).
			   */
	else
		return(file_size(&fp));
}

/*
 * Read from current file
 */

int
read(
     void *addr,
     int count)
{
	if (dev == DEV_WD) {
		/*
		 * network device is accessed through tftp
		 * to download files
		 */
		PPRINT(("net_read(%x, poff=x%x count=%d)\n",addr, poff, count));
		if (net_read(addr, count, /* phys */0) == NETBOOT_SUCCESS) {
			poff += count;
			return(0);
		}
	} else {
		if (read_file(&fp, poff, (vm_offset_t)addr, count) == KERN_SUCCESS) {
			poff += count;
			return(0);
		}
        }
	return(1);
}

/*
 * Read from current file (destination address above 1 Meg)
 */

int
xread(
     void *addr,
     int count)
{
	return(read(addr, count));
}

/*
 * Following interfaces are emulation of services required
 * from libsa_fs (usually provided by mach)
 *
 *	vm_allocate()
 *	device_open()
 *	device_read()
 *	device_read_overwrite()
 *	device_get_status()
 */

mach_port_t	mach_task_self_ = 0;

vm_allocate(
	mach_port_t target_task,
	vm_address_t *address,
	vm_size_t size,
	boolean_t anywhere)
{
#if	DEBUG
	if (debug)
		printf("vm_allocate(size %x) @%x\n",
		       size, *(((int *)&target_task)-1));
#endif
	*address = (vm_address_t) malloc(size);
#if	DEBUG
	if (debug)
		printf("vm_allocate () returns %x\n", *address);
#endif
	return(*address == 0);
}

kern_return_t 
vm_deallocate(
	mach_port_t target_task,
	vm_address_t address,
	vm_size_t size)
{
#if	DEBUG
	if (debug)
		printf("vm_deallocate(address %x, size %x) @%x\n",
		       address, size, *(((int *)&target_task)-1));
#endif
	free((void *)address);
	return(KERN_SUCCESS);
}

kern_return_t
device_open(
	mach_port_t master_port,
	mach_port_t ledger,
	dev_mode_t mode,
	security_token_t sec_token,
	dev_name_t name,
	mach_port_t *device)
{
#ifdef	DEBUG
	if (debug)
		printf("device_open(%s)\n", name);
#endif
	return devopen();
}


kern_return_t
device_get_status(
	mach_port_t 	master_port,
	dev_flavor_t 	flavor,
	dev_status_t 	status,
	mach_msg_type_number_t *status_count)
{
  	if (flavor == DEV_GET_SIZE) {
		status[DEV_GET_SIZE_RECORD_SIZE] = BPS;
		*status_count = DEV_GET_SIZE_COUNT;
	} else
		return(D_INVALID_OPERATION);

	return D_SUCCESS;
}


kern_return_t 
device_read(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_len_t bytes_wanted,
	io_buf_ptr_t *data,
	mach_msg_type_number_t *dataCnt)
{
	vm_offset_t dest;
	int offset, sector;
	int bps;

	sector = recnum + boff;
	*dataCnt = 0;

#if	DEBUG
	if (debug)
		printf("device_read(recnum %x, size %x)\n",
		       recnum, bytes_wanted);
#endif
	if (vm_allocate(mach_task_self(), &dest, bytes_wanted, TRUE))
		return(1);

	/* dont cross 64K boundary for floppy DMA */
	if (((dest + BOOTOFFSET) & ~0xffff) !=
	    ((dest + BOOTOFFSET + bytes_wanted) & ~0xffff)) {
	  	vm_offset_t old_dest = dest;
		if (vm_allocate(mach_task_self(), &dest, bytes_wanted, TRUE)) {
			vm_deallocate(mach_task_self(),
				      old_dest, bytes_wanted);
	       		return(1);
		}
		vm_deallocate(mach_task_self(), old_dest, bytes_wanted);
	}
	*data = (char *)dest;
	if (disk_read(sector, bytes_wanted, dest)) {
		vm_deallocate(mach_task_self(),
			      (vm_offset_t )*data, bytes_wanted);
		return(1);
	}
	*dataCnt = bytes_wanted;
	return(0);
}

int new_bread = 0;

vm_offset_t io_buf = 0;

kern_return_t 
device_read_overwrite(
	mach_port_t device,
	dev_mode_t mode,
	recnum_t recnum,
	io_buf_len_t bytes_wanted,
	vm_address_t data,
	mach_msg_type_number_t *dataCnt)
{
	int offset, sector;
	int bps;

#if	DEBUG
	if (debug)
		printf("device_read_overwrite(recnum %x, data %x, size %x)\n",
		       recnum, data, bytes_wanted);
#endif

	sector = recnum + boff;
	*dataCnt = 0;
	if ((vm_offset_t)data >= 0x100000 ||
	    /* dont cross 64K boundary for floppy DMA */
	    (((vm_offset_t)data + BOOTOFFSET) & ~0xffff) !=
	    (((vm_offset_t)data + BOOTOFFSET + bytes_wanted) & ~0xffff)) {
	  	if (!io_buf) 
			io_buf = (vm_offset_t)malloc(0x2000);
	  	while (bytes_wanted) {
			int size = (bytes_wanted < 0x2000) ? bytes_wanted :
			           0x2000;
			if (disk_read(sector, size, io_buf))
				return(1);
			memcpy((char *)data, (char *)io_buf, size);
			bytes_wanted -= size;
			data += size;
			sector += 0x2000/BPS;
			*dataCnt += size;
		}
	} else {
		if(disk_read(sector, bytes_wanted, (vm_offset_t)data))
			return(1);
		*dataCnt = bytes_wanted;
	}
	return 0;
}

/*
 * We need to provide our own version of memcopy, since
 * copying above 1 Meg requires special handling.
 */

void
*memcpy(
       void *to,
       const void* from,
       size_t size) 
{
#if	DEBUG
	if (debug) {
		extern void print_registers(void);
		printf("memcpy(%x %x %x)\n", from, to, size);
		print_registers();
	}
#endif
	if ((char *)to >= (char *)KERNEL_BOOT_ADDR)
		pcpy(from, (char *)to, size);
	else
	  	bcopy(from, (char *)to, size);
}


struct com_defs_to_values {
	char 	*name;
	int	value;
	int	mask;
};

struct com_defs_to_values com_speeds[] = {
  	"9600",	COM_9600,	COM_9600|COM_4800|COM_2400|COM_1200,
	"4800",	COM_4800,	COM_9600|COM_4800|COM_2400|COM_1200,
	"2400",	COM_2400,	COM_9600|COM_4800|COM_2400|COM_1200,
	"1200",	COM_1200,	COM_9600|COM_4800|COM_2400|COM_1200,
	0,		0,	0
};

struct com_defs_to_values com_flags[] = {
	"CS8",		COM_CS8,	COM_CS8|COM_CS7,
  	"CS7",		COM_CS7,	COM_CS8|COM_CS7,
	"ODDP",		COM_PARODD,	COM_PARODD|COM_PAREVEN,
	"EVENP",	COM_PAREVEN,	COM_PARODD|COM_PAREVEN,
  	0, 		0,		0
};

void
com_setup(char *s) {
	int com_setups = COM_CS8|COM_9600;
	struct com_defs_to_values *c2v;

	for (c2v = com_speeds; c2v->name; c2v++)
		if (is_sub_string(c2v->name, s, strlen(s))) {
			com_setups &= ~c2v->mask;
			com_setups = c2v->value;
			break;
		}
	for (c2v = com_flags; c2v->name; c2v++)
		if (is_sub_string(c2v->name, s, strlen(s))) {
			com_setups &= ~c2v->mask;
			com_setups |= c2v->value;
		}
	com_init(com_setups);
}

#if 0

void
print_hhex(unsigned i)
{
	if (i > 9)
		com_putc('a'+i-10);
	else
		com_putc('0'+i);
}

void
print_hex(int i)
{
	print_hhex((i&0xf0)>>4);
	print_hhex(i&0xf);
}

print_short(int i) {
	print_hex((i&0xff00)>>8);
	print_hex(i&0xff);
}

void
print_int(i)
{
	com_putc('0');
	com_putc('x');
	print_short((i&0xffff0000)>>16);
	print_short(i&0xffff);
	com_putc('\n');
	com_putc('\r');
}


where_am_i(int i)
{
  	register int j = *((&i)-1);
	print_int(j);
	print_int(&i);
}
#endif

