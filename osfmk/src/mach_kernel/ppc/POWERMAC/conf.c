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
/* ppc directory log follows:
 * Log: conf.c
 * Revision 1.1.10.1  1996/08/20  10:47:07  stephen
 * 	Added cd_audio device
 * 	[1996/08/20  10:44:54  stephen]
 *
 * Revision 1.1.7.5  1996/05/03  17:26:24  stephen
 * 	Added APPLE_FREE_COPYRIGHT
 * 	[1996/05/03  17:20:53  stephen]
 * 
 * Revision 1.1.7.4  1996/04/29  09:05:16  stephen
 * 	Support for second SCSI bus, and renamed ethernet code
 * 	[1996/04/29  08:59:14  stephen]
 * 
 * Revision 1.1.7.3  1996/04/27  15:23:49  emcmanus
 * 	The cons_*() functions are now scc_*(), declared in ppc/serial_io.h.
 * 	The name of the serial device is now "com" instead of "scc", so servers
 * 	that already work on the x86 can work on the ppc without #ifdefs.
 * 	[1996/04/27  15:03:44  emcmanus]
 * 
 * Revision 1.1.7.2  1996/04/24  17:36:55  barbou
 * 	Added cdrom_indirect_count.
 * 	[96/04/19            barbou]
 * 
 * Revision 1.1.7.1  1996/04/11  09:08:06  emcmanus
 * 	Copied from mainline.ppc.
 * 	[1996/04/10  17:04:12  emcmanus]
 * 
 * Revision 1.1.5.6  1996/01/30  13:29:15  stephen
 * 	Added vcmmap to console device to map video memory
 * 	[1996/01/30  13:22:41  stephen]
 * 
 * Revision 1.1.5.5  1996/01/22  14:57:44  stephen
 * 	Added hacked SCU ethernet driver - TODO - replace with clean code
 * 	[1996/01/22  14:37:34  stephen]
 * 
 * Revision 1.1.5.4  1996/01/15  13:53:57  stephen
 * 	Replaced ppc/serial_console_entries.h by chips/serial_console_entries.h
 * 	[1996/01/15  13:50:22  stephen]
 * 
 * Revision 1.1.5.3  1996/01/12  16:15:18  stephen
 * 	Added video console
 * 	[1996/01/12  14:31:03  stephen]
 * 
 * Revision 1.1.5.2  1995/12/19  10:19:36  stephen
 * 	Added first rev of working console + added disk_indirect count
 * 	[1995/12/19  10:14:43  stephen]
 * 
 * Revision 1.1.5.1  1995/11/23  17:38:10  stephen
 * 	first powerpc checkin to mainline.ppc
 * 	[1995/11/23  16:48:44  stephen]
 * 
 * Revision 1.1.2.2  1995/10/10  15:09:11  stephen
 * 	return from apple
 * 	[1995/10/10  14:36:30  stephen]
 * 
 * 	not a lot
 * 
 * Revision 1.1.3.2  95/09/05  17:49:08  stephen
 * 	Replaced hp700 based copy with that from AT386
 * 
 * Revision 1.1.2.1  1995/08/25  06:33:29  stephen
 * 	     Initial checkin of files for PowerPC port
 * 	     [1995/08/23  15:07:48  stephen]
 * 
 * EndLog
 */

/* Old AT386 revision history
 * Revision 1.2.21.4  1995/01/10  04:50:51  devrcs
 * 	mk6 CR801 - merge up from nmk18b4 to nmk18b7
 * 	* Rev 1.2.21.3  1994/10/21  18:41:30  joe
 * 	  Added ETAP support
 * 	[1994/12/09  20:37:40  dwm]
 */
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
 * Device switch for PowerMac
 */

#include <platforms.h>
#include <types.h>
#include <device/tty.h>
#include <device/conf.h>
#include <device/if_ether.h>
#include <kern/clock.h>
#include <chips/busses.h>

#include <ppc/POWERMAC/rtclock_entries.h>

#define	rtclockname		"rtclock"
#define	timename		"time"

/*
 * we don't define a console here. After we have configured the devices
 * we will assign a console though the indirect table.
 */
#define	cnname		"cn"
#define cnopen 		NULL_OPEN
#define cnclose 	NULL_CLOSE
#define cnread 		NULL_READ
#define cnwrite 	NULL_WRITE
#define cngetstat 	NULL_GETS
#define cnsetstat 	NULL_SETS
#define cnreset 	NULL_RESET
#define cnportdeath 	NULL_DEATH

#include <hi_res_clock.h>
#include <cpus.h>
#if	HI_RES_CLOCK && NCPUS > 1
#include <machine/hi_res_clock.h>
#define hi_res_clk_name         "hi_res_clock"
#endif  /* HI_RES_CLOCK && NCPUS > 1 */

#include <scc.h>
#if NSCC > 0
#include <ppc/POWERMAC/serial_io.h>
#define scc_name "com"
#endif /* NSCC */

#include <fd.h>
#if NFD > 0
#include <ppc/POWERMAC/floppy/fd_entries.h>
#define fdname "fd"
#endif /* NFD > 0 */

/*
 * Video Console  - MEB
 */

#include <vc.h>
#if	NVC > 0
#include	<ppc/POWERMAC/video_console_entries.h>
#define		vcname	"vc"
#endif

/*
 * Console feed to user application
 */
#include <consfeed.h>
#if	NCONSFEED > 0
#include	<ppc/console_feed_entries.h>
#define  consfeedname "console_feed"
#endif

/*
 * SCSI things
 */
#include <asc.h>
#if	NASC > 0
#include <scsi/scsi_defs.h>
#ifdef	SCSI_SEQUENTIAL_DISKS
#define	rzname	"sdisk"		/* disk */
#define tzname	"stape"		/* tape */
#else
#define	rzname	"sd"		/* disk */
#define tzname	"st"		/* tape */
#endif
#define cdname	"cd_rom"	/* CD-ROM DA */
#endif	/* NASC > 0 */

#include <physmem.h>
#if	NPHYSMEM > 0
#include <ppc/POWERMAC/physmem_entries.h>
#define physmemname		"physmem"
#endif	/* NPHYSMEM > 0 */

#if NOTYET /* NMGS - ETAP not implemented */
#include <etap.h>
#if     ETAP
#include <i386/etap_map.h>
#define etap_map_name		"etap_map"
#endif  /* ETAP */
#endif /* NOTYET */

#include <test_device.h>
#if	NTEST_DEVICE > 0
#include <device/test_device_entries.h>
#define testdevname		"test_device"
#endif	/* NTEST_DEVICE > 0 */

#include <lan.h>
#if	NLAN > 0
#include <ppc/POWERMAC/if_mace_entries.h>
#define	lanname		"wd0"
#endif /* NLAN > 0 */

#include <tulip.h>
#if	NTULIP> 0
#include <ppc/POWERMAC/if_tulip_entries.h>
#define	tulipname		"tulip"
#endif /* NTULIP > 0 */

#include <bmac.h>
#if	NBMAC > 0
#include <ppc/POWERMAC/if_bmac_entries.h>
#define	bmacname		"el"
#endif /* NTULIP > 0 */

#include <adb.h>
#if NADB > 0
#include <ppc/POWERMAC/adb.h>
#define	adbname	"adb0"
#endif

#include <wd.h>
#if NWD > 0
#include <ppc/POWERMAC/wd_entries.h>
#define    wdname     "ide"
#include <ppc/POWERMAC/wcd_entries.h>
#define	  wcdname	"wcd"
#endif

#include <awacs.h>
#if NAWACS > 0
io_return_t awacs_open();
void        awacs_close();
io_return_t awacs_read();
io_return_t awacs_write();
io_return_t awacs_getstat();
io_return_t awacs_setstat();
#define awacsname "awacs"
#endif

#include <scsiinfo.h>
#if NSCSIINFO > 0
#include <scsi/scsi_info_entries.h>
#define	scsi_info_name	"scsi_info"
#endif

#include <ppc/misc_protos.h>

/*
 * List of devices - console must be at slot 0
 */
struct dev_ops	dev_name_list[] =
{
	/*name,		open,		close,		read,
	  write,	getstat,	setstat,	mmap,
	  async_in,	reset,		port_death,	subdev,
	  dev_info */

	{ cnname,	cnopen,		cnclose,	cnread,
	  cnwrite,	cngetstat,	cnsetstat,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	cnportdeath,	0,
	  NO_DINFO },

	{ scc_name,	scc_open,	scc_close,	scc_read,
	  scc_write,	scc_get_status,	scc_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	scc_portdeath,	0,
	  NO_DINFO },

	{ timename,	NULL_OPEN,	NULL_CLOSE,	NULL_READ,
	  NULL_WRITE,	NULL_GETS,	NULL_SETS,	utime_map,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

	{ rtclockname,	NULL_OPEN,	NULL_CLOSE,	NULL_READ,
	  NULL_WRITE,	NULL_GETS,	NULL_SETS,	rtclock_map,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

#if	NVC > 0
	{ vcname,	vcopen,		vcclose,	vcread,
	  vcwrite,	vcgetstatus,	vcsetstatus,	vcmmap,
	NO_ASYNC,	NULL_RESET,	vcportdeath,	0,
	NO_DINFO },
#endif

#if	NASC > 0
		/* SCSI disk */
	{ rzname,	rz_open,	rz_close,	rz_read,
	  rz_write,	rz_get_status,	rz_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NO_DEATH,	16,
	  rz_devinfo },
		/* SCSI tape */
	{ tzname,	rz_open,	rz_close,	rz_read,
	  rz_write,	rz_get_status,	rz_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NO_DEATH,	16,
	  NO_DINFO },

	{ cdname,	cd_open,	cd_close,	cd_read,
	  cd_write,	cd_get_status,	cd_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  cd_devinfo },

#endif	/* NASC > 0 */

#if	NWD > 0
	{ wdname,	wdopen,	wdclose,	wdread,
	  wdwrite,	wdgetstat,	wdsetstat,	NULL_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  wddevinfo },
	{ wcdname,	wcdopen,	wcdclose,	wcdread,
	  wcdwrite,	wcdgetstat,	wcdsetstat,	NULL_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  wcddevinfo },
#endif	/* NWD > 0 */

#if	NFD > 0
	{ fdname,	fdopen,		fdclose,	fdread,
	  fdwrite,	fdgetstat,	fdsetstat,	NULL_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	64,
	  fddevinfo },
#endif	/* NFD > 0 */

#if	NAHA > 0
	{ rzname,	rz_open,	rz_close,	rz_read,
	  rz_write,	rz_get_status,	rz_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  rz_devinfo },

	{ tzname,	rz_open,	rz_close,	rz_read,
	  rz_write,	rz_get_status,	rz_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  NO_DINFO },

#endif	/*NAHA > 0*/

#if	NWT > 0
	{ wtname,	wtopen,		wtclose,	wtread,
	  wtwrite,	NULL_GETS,	NULL_SETS,	NULL_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },
#endif	/* NWT > 0 */

#if	NLAN > 0
	{ lanname,	mace_open,	NULL_CLOSE,	NULL_READ,
	  mace_output,	mace_getstat,	mace_setstat,	NULL_MMAP,
	  mace_setinput, NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif

#if	NTULIP > 0
	{ tulipname,	tulip_open,	NULL_CLOSE,	NULL_READ,
	  tulip_output,	tulip_getstat,	tulip_setstat,	NULL_MMAP,
	  tulip_setinput, NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif

#if	NBMAC > 0
	{ bmacname,	bmac_open,	NULL_CLOSE,	NULL_READ,
	  bmac_output,	bmac_getstat,	bmac_setstat,	NULL_MMAP,
	  bmac_setinput, NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif

#if	NNS8390 > 0
	{ ns8390wdname,	wd8003open,	NULL_CLOSE,	NULL_READ,
	  ns8390output, ns8390getstat,	ns8390setstat,	NULL_MMAP,
	  ns8390setinput, NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },

	{ ns8390elname,	eliiopen,	NULL_CLOSE,	NULL_READ,
	  ns8390output, ns8390getstat,	ns8390setstat,	NULL_MMAP,
	  ns8390setinput, NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif

#if	NPAR > 0
	{ parname,	paropen,	NULL_CLOSE,	NULL_READ,
	  paroutput,	pargetstat,	parsetstat,	NULL_MMAP,
	  parsetinput,	NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif

#if	NCOM > 0
	{ comname,	comopen,	comclose,	comread,
	  comwrite,	comgetstat,	comsetstat,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	comportdeath,	0,
	  NO_DINFO },
#endif

#if	NLPR > 0
	{ lprname,	lpropen,	lprclose,	lprread,
	  lprwrite,	lprgetstat,	lprsetstat,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	lprportdeath,	0,
	  NO_DINFO },
#endif

#if	NBLIT > 0
	{ blitname,	blitopen,	blitclose,	NO_READ,
	  NO_WRITE,	blit_get_stat,	NO_SETS,	blitmmap,
	  NO_ASYNC,	NO_RESET,	NO_DEATH,	0,
	  NO_DINFO },
#endif

#if NADB > 0
	{ adbname,	adbopen,	adbclose,	adbread,
	  adbwrite,	adbgetstatus,	adbsetstatus,	NULL_MMAP,
	  NO_ASYNC,	NULL_RESET, 	NULL_DEATH, 	0,
          NO_DINFO },
#endif


#if	NTEST_DEVICE > 0
	{ testdevname,	testdev_open,	testdev_close,	testdev_read,
	  testdev_write,testdev_getstat,testdev_setstat,testdev_mmap,
	  testdev_async_in,testdev_reset,testdev_port_death,	0,
	  testdev_dev_info },
#endif	/* NTEST_DEVICE > 0 */

#if	NPHYSMEM > 0
	{ physmemname,	physmem_open,	physmem_close,	physmem_read,
	  physmem_write,NULL_GETS,	NULL_SETS,	physmem_mmap,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },
#endif

#if	NCONSFEED > 0
	{ consfeedname,	console_feed_open, console_feed_close,
	  console_feed_read, NULL_WRITE, NULL_GETS, NULL_SETS, NULL_MMAP,
	  NO_ASYNC, NULL_RESET, NULL_DEATH, 0, NO_DINFO },
#endif

#if	NAWACS > 0
	{ awacsname,	awacs_open,	awacs_close,	awacs_read,
	  awacs_write,	awacs_getstat,	awacs_setstat,	NO_MMAP,
	  NO_ASYNC,	NO_RESET,	NO_DEATH,	0,
	  NO_DINFO },
#endif	
#if	NSCSIINFO > 0
	{ scsi_info_name,scsi_info_open,scsi_info_close,scsi_info_read,
	  scsi_info_write,scsi_info_getstatus,scsi_info_setstatus,NO_MMAP,
	  NO_ASYNC,	NO_RESET,	NO_DEATH,	0,
	  NO_DINFO },
#endif	
};
int	dev_name_count = sizeof(dev_name_list)/sizeof(dev_name_list[0]);

/*
 * Indirect list. We'll stomp on the console device when we finally figure
 * out where the console should be.
 */
struct dev_indirect dev_indirect_list[] = {

	/* console */
	{ "console",	&dev_name_list[0],	0 },

};
int	dev_indirect_count = sizeof(dev_indirect_list)
				/sizeof(dev_indirect_list[0]);

int	disk_indirect_count = 0;
int	cdrom_indirect_count = 0;

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
