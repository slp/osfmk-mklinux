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
 * Copyright (c) 1989 Carnegie Mellon University.
 * Copyright (c) 1990,1991,1992 The University of Utah and
 * the Center for Software Science (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
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
 *
 * 	Utah $Hdr: conf.c 1.14 92/04/27$
 */

/*
 *	Device switch table
 */
#include <device/conf.h>
#include <kern/clock.h>

/* prototypes */
#include <hp_pa/HP700/cons.h>
#include <device/io_req.h>
#include <hp_pa/rtclock_entries.h>
#include <hp_pa/misc_protos.h>

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

#define	rtclockname	"rtclock"
#define timename	"time"


#include <dca.h>
#define	dcaname		"com"
#if NDCA > 0
extern io_return_t dcaopen(dev_t, dev_mode_t, io_req_t);
extern void  dcaclose(dev_t);
extern int dcaread(dev_t, io_req_t);
extern int dcawrite(dev_t, io_req_t);
extern io_return_t dcagetstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t *);
extern io_return_t dcasetstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t);
extern boolean_t dcaportdeath(dev_t, ipc_port_t);
#else
#define dcaopen 	NO_OPEN
#define dcaclose 	NO_CLOSE
#define dcaread 	NO_READ
#define dcawrite 	NO_WRITE
#define dcagetstat 	NO_GETS
#define dcasetstat 	NO_SETS
#define dcareset 	NO_RESET
#define dcaportdeath 	NO_DEATH
#endif

#include <grf.h>
#define	grfname		"grf"
#if NGRF > 0
extern io_return_t grfopen(dev_t, dev_mode_t, io_req_t);
extern void grfclose(dev_t);
extern io_return_t grfgetstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t *);
extern io_return_t grfsetstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t);
extern vm_offset_t grfmap(dev_t,  vm_offset_t, int);
#else
#define grfopen 	NO_OPEN
#define grfclose 	NO_CLOSE
#define grfgetstat 	NO_GETS
#define grfsetstat 	NO_SETS
#define grfmap		NO_MMAP
#endif

#include <ite.h>
#define	itename		"ite"
#if NITE > 0
extern io_return_t iteopen(dev_t, dev_mode_t, io_req_t);
extern void iteclose(dev_t);
extern int iteread(dev_t, io_req_t);
extern int itewrite(dev_t, io_req_t);
extern io_return_t itegetstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t *);
extern io_return_t itesetstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t);
extern boolean_t iteportdeath(dev_t, ipc_port_t);
#else
#define iteopen 	NO_OPEN
#define iteclose 	NO_CLOSE
#define iteread 	NO_READ
#define itewrite 	NO_WRITE
#define itegetstat 	NO_GETS
#define itesetstat 	NO_SETS
#define itereset 	NO_RESET
#define iteportdeath 	NO_DEATH
#endif

#include <hil.h>
#define	hilname		"hil"
#if NHIL > 0
extern io_return_t hilopen(dev_t, dev_mode_t, io_req_t);
extern void hilclose(dev_t);
extern io_return_t hilread(dev_t, io_req_t);
extern io_return_t hilgetstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t *);
extern io_return_t hilsetstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t);
extern vm_offset_t hilmap(dev_t, vm_offset_t, int);
#else
#define hilopen 	NO_OPEN
#define hilclose 	NO_CLOSE
#define hilread 	NO_READ
#define hilgetstat 	NO_GETS
#define hilsetstat 	NO_SETS
#define hilmap	 	NO_MMAP
#endif

#include <scsi.h>
#include <ncr.h>
#if NNCR > 0
#include <scsi/scsi_defs.h>
#include <scsi/rz.h>
#define rzname "sd"
#define	tzname "st"
#define	fdname "fd"
#define cdname "cd_rom"
#endif /* NNCR > 0 */

#include <lan.h>
#define	pc586name	"pc"
#if NLAN > 0
extern io_return_t pc586open(dev_t, dev_mode_t, io_req_t);
extern void pc586close(dev_t);
extern int  pc586output(dev_t, io_req_t);
extern int  pc586getstat(dev_t, dev_flavor_t,
			 dev_status_t, mach_msg_type_number_t *);
extern int  pc586setstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t);
extern void pc586reset(dev_t);
extern int  pc586setinput(dev_t, ipc_port_t, int, filter_t[],
			  unsigned int, device_t);
extern io_return_t pc586devinfo(dev_t, dev_flavor_t, char *);
#else
#define pc586open	NO_OPEN
#define pc586close	NO_CLOSE
#define pc586output	NO_WRITE
#define pc586getstat	NO_GETS
#define pc586setstat	NO_SETS
#define pc586reset	NO_RESET
#define pc586setinput	NO_ASYNC
#define	pc586devinfo	NO_DINFO
#endif

#include <gkd.h>
#define	gkdname		"gkd"
#if NGKD > 0
extern int  gkdopen(dev_t, dev_mode_t, io_req_t);
extern void gkdclose(dev_t);
extern int  gkdread(dev_t, io_req_t);
extern int  gkdgetstat(dev_t, dev_flavor_t,
		       dev_status_t, mach_msg_type_number_t *);
extern int  gkdsetstat(dev_t, dev_flavor_t,
			      dev_status_t, mach_msg_type_number_t);
extern vm_offset_t gkdmap(dev_t, vm_offset_t, int);
#else
#define gkdopen 	NO_OPEN
#define gkdclose 	NO_CLOSE
#define gkdread 	NO_READ
#define gkdgetstat 	NO_GETS
#define gkdsetstat 	NO_SETS
#define gkdmap	 	NO_MMAP
#endif

#include <fddi.h>
#define sh4811name      "seahawk"
#if NFDDI > 0
extern int  sh4811open(dev_t, dev_mode_t, io_req_t);
extern void  sh4811close(dev_t);
extern void sh4811reset(dev_t);
extern int  sh4811output(dev_t, io_req_t);
extern int  sh4811getstat(dev_t, dev_flavor_t, dev_status_t,
			  mach_msg_type_number_t *);
extern int  sh4811setstat(dev_t, dev_flavor_t, dev_status_t,
			  mach_msg_type_number_t);
extern int  sh4811setinput(dev_t, ipc_port_t, int, filter_t[],
			   unsigned int, device_t);
#else
#define sh4811open      NO_OPEN
#define sh4811close     NULL_CLOSE
#define sh4811output    NO_WRITE
#define sh4811input     NO_READ
#define sh4811getstat   NO_GETS
#define sh4811setstat   NO_SETS
#define sh4811reset     NO_RESET
#define sh4811setinput  NO_ASYNC
#endif

#include <device/data_device.h>
#define	datadevname		"data_device"

#include <etap.h>
#if     ETAP
#include <kern/etap_map.h>
#define etap_map_name		"etap_map"
#endif  /* ETAP */

#include <test_device.h>
#if	NTEST_DEVICE > 0
#include <device/test_device_entries.h>
#define testdevname		"test_device"
#endif	/* NTEST_DEVICE > 0 */

#include <scsiinfo.h>
#if NSCSIINFO > 0
#include <scsi/scsi_info_entries.h>
#define scsi_info_name  "scsi_info"
#endif

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

	{ timename,	NULL_OPEN,	NULL_CLOSE,	NULL_READ,
	  NULL_WRITE,	NULL_GETS,	NULL_SETS,	utime_map,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

	{ rtclockname,	NULL_OPEN,	NULL_CLOSE,	NULL_READ,
	  NULL_WRITE,	NULL_GETS,	NULL_SETS,	rtclock_map,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

#if NNCR > 0 
	{ rzname,       rz_open,	rz_close,	rz_read,
	  rz_write,	rz_get_status,	rz_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  rz_devinfo },

	{ tzname,	rz_open,	rz_close,	rz_read,
	  rz_write,	rz_get_status,	rz_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  NO_DINFO },

	{ fdname,	rz_open,	rz_close,	rz_read,
	  rz_write,	rz_get_status,	rz_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	64,
	  NO_DINFO },

	{ cdname,	cd_open,	cd_close,	cd_read,
	  cd_write,	cd_get_status,	cd_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  cd_devinfo },
#endif	/* NNCR > 0 */

	{ pc586name,	pc586open,	pc586close,	NO_READ,
	  pc586output,	pc586getstat,	pc586setstat,	NO_MMAP,
	  pc586setinput,pc586reset,	NULL_DEATH,	0,
	  pc586devinfo },
	
	{ grfname,	grfopen,	grfclose,	NO_READ,
	  NO_WRITE,	grfgetstat,	grfsetstat,	grfmap,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

	{ hilname,	hilopen,	hilclose,	hilread,
	  NO_WRITE,	hilgetstat,	hilsetstat,	hilmap,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

	{ dcaname,	dcaopen,	dcaclose,	dcaread,
	  dcawrite,	dcagetstat,	dcasetstat,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	dcaportdeath,	0,
	  NO_DINFO },

	{ itename,	iteopen,	iteclose,	iteread,
	  itewrite,	itegetstat,	itesetstat,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	iteportdeath,	0,
	  NO_DINFO },

	{ gkdname,	gkdopen,	gkdclose,	gkdread,
	  NO_WRITE,	gkdgetstat,	gkdsetstat,	gkdmap,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

	{ sh4811name,	sh4811open,	sh4811close,	NO_READ,
	  sh4811output, sh4811getstat,	sh4811setstat,	NO_MMAP,
          sh4811setinput,sh4811reset,   NULL_DEATH,     0,
          NO_DINFO },

	{ datadevname,	datadev_open,	datadev_close,	datadev_read,
	  datadev_write,NULL_GETS,	NULL_SETS,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  datadev_dinfo },

#if     ETAP
	{ etap_map_name,etap_map_open,	NULL_CLOSE,	NO_READ,
	  NO_WRITE,	NULL_GETS,	NULL_SETS,	etap_map_mmap,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },
#endif  /* ETAP */

#if	NTEST_DEVICE > 0
	{ testdevname,	testdev_open,	testdev_close,	testdev_read,
	  testdev_write,testdev_getstat,testdev_setstat,testdev_mmap,
	  testdev_async_in,testdev_reset,testdev_port_death,	0,
	  testdev_dev_info },
#endif	/* NTEST_DEVICE > 0 */

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

	/* boot device */
	{ "boot_device",	0,		0 },

	{ "lan",		0,		0 }

};
int
    dev_indirect_count = sizeof(dev_indirect_list)
				/sizeof(dev_indirect_list[0]);
int cdrom_indirect_count = 0;

#if NSCSI > 0
int	disk_indirect_count = 0;
#endif /* NSCSI > 0  */
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
