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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
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
 * Revision 2.2.1.1  92/05/27  00:36:36  jeffreyh
 * 	A Paragon hack until I figure out how to get config to
 * 	generate <bm.h> when I need NBM to be 0.
 * 	[92/05/26            andyp@ssd.intel.com]
 * 
 * Revision 2.2  91/08/24  11:53:18  af
 * 	Created, splitting out most code from dz_tty.c file.
 * 	[91/07/07            af]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
 *	File: serial_console.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	7/91
 *
 *	Console driver for serial-line based consoles.
 */

#include <mach_kdb.h>
#include <platforms.h>
#include <serial_console_default.h>
#include <consfeed.h>

#include <mach_kdb.h>
#include <kern/spl.h>
#include <machine/machparam.h>		/* spl definitions */
#include <types.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <sys/syslog.h>
#include <chips/busses.h>
#include <chips/serial_console_entries.h>
#include <ppc/screen_defs.h>
#include <ppc/misc_protos.h>
#include <ppc/POWERMAC/video_console_entries.h>
#include <ppc/screen_switch.h>
#include <ppc/console_feed_entries.h>

/*
 * A machine MUST have a console.  In our case
 * things are a little complicated by the graphic
 * display: people expect it to be their "console",
 * but we'd like to be able to live without it.
 * This is not to be confused with the "rconsole" thing:
 * that just duplicates the console I/O to
 * another place (for debugging/logging purposes).
 */

const int console_unit = 0;
const int console_chan = CONSOLE_PORT;

#if MACH_KGDB
void no_spl_vcputc(char), no_spl_scputc(char);
int no_spl_vcgetc(void), no_spl_scgetc(void);

#define OPS(putc, getc, nosplputc, nosplgetc) putc, getc, nosplputc, nosplgetc

#else

#define OPS(putc, getc, nosplputc, nosplgetc) putc, getc

#endif	/* MACH_KGDB */

const struct console_ops {
	int	(*putc)(int, int, int);
	int	(*getc)(int, int, boolean_t, boolean_t);
#if MACH_KGDB
	void	(*no_spl_putc)(char);	/* Must not do splwhatever(). */
	int	(*no_spl_getc)(void);	/* Must not do splwhatever(). */
#endif
} cons_ops[] = {
#define SCC_CONS_OPS 0
	{OPS(scc_putc, scc_getc, no_spl_scputc, no_spl_scgetc)},
#define VC_CONS_OPS 1
	{OPS(vcputc, vcgetc, no_spl_vcputc, no_spl_vcgetc)},
};
#define NCONSOPS (sizeof cons_ops / sizeof cons_ops[0])

#if SERIAL_CONSOLE_DEFAULT
#define CONS_OPS SCC_CONS_OPS
#define CONS_NAME "com"
#else
#define CONS_OPS VC_CONS_OPS
#define CONS_NAME "vc"
#endif
unsigned int cons_ops_index = CONS_OPS;

void
cnputc(char c)
{
#if	NCONSFEED > 0
	if (console_feed_putc(c))
		cons_ops[cons_ops_index].putc(console_unit, console_chan, c);
#else	/* NCONSFEED > 0 */
	cons_ops[cons_ops_index].putc(console_unit, console_chan, c);
#endif	/* NCONSFEED > 0 */

	if (c == '\n')
		cnputc('\r');
}

int
cngetc()
{
	return cons_ops[cons_ops_index].getc(console_unit, console_chan,
					     TRUE, FALSE);
}

int
cnmaygetc()
{
	return cons_ops[cons_ops_index].getc(console_unit, console_chan,
					     FALSE, FALSE);
}

#if MACH_KGDB
void
kgdb_putc(char c)
{
	no_spl_scc_putc(KGDB_PORT, c);
}

int
kgdb_getc(boolean_t timeout)
{
	return no_spl_scc_getc(KGDB_PORT, timeout);
}

void
no_spl_putc(char c)
{
	cons_ops[cons_ops_index].no_spl_putc(c);
	if (c == '\n')
		cons_ops[cons_ops_index].no_spl_putc('\r');
}

int
no_spl_getc()
{
	return cons_ops[cons_ops_index].no_spl_getc();
}

void
no_spl_scputc(char c)
{
	no_spl_scc_putc(CONSOLE_PORT, c);
}

int
no_spl_scgetc()
{
	return no_spl_scc_getc(CONSOLE_PORT, FALSE);
}

void
no_spl_vcputc(char c)
{
	vc_putchar(c);
}

int
no_spl_vcgetc()
{
	return vcgetc(0, 0, TRUE, FALSE);
}
#endif /* MACH_KGDB */

boolean_t console_is_serial()
{
	return cons_ops_index == SCC_CONS_OPS;
}

int
switch_to_video_console()
{
	int old_cons_ops = cons_ops_index;
	cons_ops_index = VC_CONS_OPS;
	return old_cons_ops;
}

int
switch_to_serial_console()
{
	int old_cons_ops = cons_ops_index;
	cons_ops_index = SCC_CONS_OPS;
	return old_cons_ops;
}

/* The switch_to_{video,serial,kgdb}_console functions return a cookie that
   can be used to restore the console to whatever it was before, in the
   same way that splwhatever() and splx() work.  */
void
switch_to_old_console(int old_console)
{
	static boolean_t squawked;
	unsigned int ops = old_console;

	if (ops >= NCONSOPS && !squawked) {
		squawked = TRUE;
		printf("switch_to_old_console: unknown ops %d\n", ops);
	} else
		cons_ops_index = ops;
}

/*
 * This is basically a special form of autoconf,
 * to get printf() going before true autoconf.
 */
int
cons_find(boolean_t tube)
{
	register int		 i;
	struct tty		*tp;

	extern struct dev_ops dev_name_list[];
	extern int dev_name_count;
	extern struct dev_indirect dev_indirect_list[];

	/* Set up indirect console device */

	for (i = 0; i < dev_name_count; i++) {
		if (strcmp(dev_name_list[i].d_name, CONS_NAME) == 0) {
			dev_indirect_list[0].d_ops = &dev_name_list[i];
			break;
		}
	}

	return	1;
}
