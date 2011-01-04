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
 * Copyright (c) 1991 Carnegie Mellon University
 * Copyright (c) 1991 Sequent Computer Systems
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND SEQUENT COMPUTER SYSTEMS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * SEQUENT COMPUTER SYSTEMS DISCLAIM ANY LIABILITY OF ANY KIND FOR
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

/* CMU_HIST */
/*
/*
 * 28-Sep-92  Philippe Bernadat (bernadat) at gr.osf.org
 *	Added zdgetstat() & zd_devinfo() from MK_78
 *	
 * Revision 2.4.2.1  92/09/15  17:35:49  jeffreyh
 * 	Replaced getstat/setstat entries of zd with nodev to return
 * 	an error. (Used to be nulldev)
 * 	[92/08/18            bernadat]
 * 
 * Revision 2.4  91/08/28  11:16:51  jsb
 * 	Added entries for new dev_info field.
 * 	[91/08/27  17:56:16  jsb]
 * 
 * Revision 2.3  91/07/31  18:05:01  dbg
 * 	Changed copyright.
 * 	[91/07/31            dbg]
 * 
 * Revision 2.2  91/05/08  13:01:48  dbg
 * 	Created.
 * 	[90/10/04            dbg]
 * 
 */

/*
 * Device table for Sequent.
 */

#include <device/conf.h>
#include <kern/clock.h>

extern int block_io_mmap();

extern	int	rtclock_map();
#define	rtclockname		"rtclock"

extern int	utime_map();
#define	timename		"time"

extern int	coopen(),coclose(),coread(),cowrite();
extern int	cogetstat(),cosetstat(), coportdeath();
#define	coname		"co"

#include <sd.h>
#if	NSD > 0
extern int	sdopen(), sdread(), sdwrite();
#define	sdname		"sd"
#endif

#include <zd.h>
#if	NZD > 0
extern int	zdopen(), zdclose(), zdread(), zdwrite(), zdgetstat();
extern int 	zd_devinfo();
#define	zdname		"zd"
#endif

#include <se.h>
#if	NSE > 0
extern int	se_open(), se_output(), se_getstat(), se_setstat();
extern int	se_setinput();
#define	sename		"se"
#endif

#include <hi_res_clock.h>
#if	HI_RES_CLOCK
extern int      hi_res_clk_mmap();
#define hi_res_clk_name         "hi_res_clock"
#endif  /* HI_RES_CLOCK */

/*
 * List of devices.  Console must be at slot 0.
 */
struct dev_ops	dev_name_list[] =
{
	/*name,		open,		close,		read,
	  write,	getstat,	setstat,	mmap,
	  async_in,	reset,		port_death,	subdev,
	  dev_info */

	{ coname,	coopen,		coclose,	coread,
	  cowrite,	cogetstat,	cosetstat,	nulldev,
	  nodev,	nulldev,	coportdeath,	0,
	  nodev },

	{ timename,	nulldev,	nulldev,	nulldev,
	  nulldev,	nulldev,	nulldev,	utime_map,
	  nodev,	nulldev,	nulldev,	0,
	  nodev },

	{ rtclockname,	nulldev,	nulldev,	nulldev,
	  nulldev,	nulldev,	nulldev,	rtclock_map,
	  nodev,	nulldev,	nulldev,	0,
	  nodev },

#if	NSD > 0
	{ sdname,	sdopen,		nulldev,	sdread,
	  sdwrite,	nulldev,	nulldev,	block_io_mmap,
	  nulldev,	nulldev,	nulldev,	8,
	  nodev },
#endif

#if	NZD > 0
	{ zdname,	zdopen,		zdclose,	zdread,
	  zdwrite,	zdgetstat,	nulldev,	block_io_mmap,
	  nodev,	nulldev,	nulldev,	8,
	  zd_devinfo },
#endif

#if	NSE > 0
	{ sename,	se_open,	nulldev,	nulldev,
	  se_output,	se_getstat,	se_setstat,	nulldev,
	  se_setinput,	nulldev,	nulldev,	0,
	  nodev },
#endif

#if	HI_RES_CLOCK
        { hi_res_clk_name, nulldev,	nulldev,	nodev,
          nodev,           nulldev,	nulldev,	hi_res_clk_mmap,
          nodev,           nulldev,	nulldev,	0,
	  nodev },
#endif  /* HI_RES_CLOCK */

};
int	dev_name_count = sizeof(dev_name_list)/sizeof(dev_name_list[0]);

/*
 * Indirect list.
 */
struct dev_indirect dev_indirect_list[] = {

	/* console */
	{ "console",	&dev_name_list[0],	0 }
};
int	dev_indirect_count = sizeof(dev_indirect_list)
				/ sizeof(dev_indirect_list[0]);

/*
 * Clock device subsystem configuration. The clock_list[]
 * table contains the clock structures for all clocks in
 * the system.
 */

extern	struct clock_ops	rtc_ops;
extern	struct clock_ops	bbc_ops;

/*
 * List of clock devices.
 */
struct	clock	clock_list[] = {

	/* REALTIME_CLOCK */
	{ &rtc_ops,	0,		0,		0 },

	/* BATTERY_CLOCK */
	{ &bbc_ops,	0,		0,		0 },

	/* HIGHRES_CLOCK */
	{ 0,		0,		0,		0 },
};
int	clock_count = sizeof(clock_list) / sizeof(clock_list[0]);
