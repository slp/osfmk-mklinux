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
 * 
 */
/*
 * MkLinux
 */
#ifndef	_NET_H_
#define	_NET_H_

#include <secondary/net_debug.h>

/*
 * network error codes (I'm tired of mixed error codes)
 */
#define NETBOOT_SUCCESS	0
#define NETBOOT_ERROR	1
#define NETBOOT_ABORT	2
#define NETBOOT_RETRY	3	/* file needs to be "rewinded" */

typedef unsigned short 	i386_ioport_t;

typedef	unsigned char 	u8bits;
typedef	unsigned short 	u16bits;
typedef	unsigned int 	u32bits;
typedef	signed char 	s8bits;
typedef	signed short 	s16bits;
typedef	signed int 	s32bits;

/*
 * net.c
 */
extern int	net_open(void);		/* (hook in disk.c) */
extern int	net_read(void *, int, int);	/* (hook in sys.c/mach_boot.c) */
extern int	net_openfile(const char *);	/* (hook in sys.c/mach_boot.c) */
extern int 	net_probe(void);		/* split from net_open() */
extern int	network_used;			/* =1 if we were using net */

/*
 * i386at.c
 */
extern void timerinit(void);
extern void settimeout(unsigned);
extern int isatimeout(void);
extern unsigned rand(void);
extern unsigned getelapsed(void);
extern void calibrate_delay();
extern void delayed_outb(int, int);
extern int delayed_inb(int);

/*
 * asm.s (mainly for net/ns8390.c)
 * only the functionalities added for networking
 */
extern int  read_ticks(unsigned *);
extern void set_ticks(unsigned);
#define	    pcpy_out	pcpy
extern void pcpy_in(const void *, void *, int);
extern void pcpy16_out(const void *, void *, int);
extern void pcpy16_in(const void *, void *, int);

/*
 * arp.c
 */
extern void arp_init(void);

/*
 * bootp.c
 */
extern void bootp_init(void);
extern int bootp_request(char *, char *, char *);
extern char *default_net_rootpath;

#ifndef _BOOT_H_
/*
 * avoid recursive inclusion by copying here
 * the needed prototypes
 */

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

/*
 * boot.c
 */
extern void boot(int);
extern char *itoa(char *, unsigned int);

/*
 * disk.c
 */
extern int disk_open(void);

/*
 * io.c
 */
extern void gateA20(void);
extern int is_intr_char(void);
extern void putchar(int);
extern int getchar(void);
extern void flush_char(void);
extern void gets(char *);
extern int strlen(const char *);
extern void bcopy(const char *, char *, int);
extern void printf(const char *, ...);

/*
 * sys.c
 */
extern int read(void *, int);
extern int xread(void *, int);
extern int openrd(const char *, const char **);

#endif /* __LANGUAGE_ASSEMBLY */
#endif	/* _BOOT_H_ */

#endif	/* _NET_H_ */


