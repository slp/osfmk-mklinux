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

#define BOOT_LINE_LENGTH 32

/*
 * Arguments to reboot system call.
 * These are converted to switches, and passed to startup program,
 * and on to init.
 */
#define	RB_AUTOBOOT	0	/* flags for system auto-booting itself */

#define	RB_ASKNAME	0x01	/* -a: ask for file name to reboot from */
#define	RB_SINGLE	0x02	/* -s: reboot to single user only */
#define	RB_KDB		0x04	/* -d: kernel debugger symbols loaded */
#define	RB_HALT		0x08	/* -h: enter KDB at bootup */
				/*     for host_reboot(): don't reboot,
				       just halt */
#define	RB_INITNAME	0x10	/* -i: name given for /etc/init (unused) */
#define	RB_DFLTROOT	0x20	/*     use compiled-in rootdev */
#define	RB_NOBOOTRC	0x20	/* -b: don't run /etc/rc.boot */
#define RB_ALTBOOT	0x40	/*     use /boot.old vs /boot */
#define	RB_UNIPROC	0x80	/* -u: start only one processor */

#define	RB_SHIFT	8	/* second byte is for ux */

#define	RB_DEBUGGER	0x1000	/*     for host_reboot(): enter kernel
				       debugger from user level */

