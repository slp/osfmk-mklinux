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
 * Revision 2.10.6.3  92/03/28  10:05:58  jeffreyh
 * 	Tapes must also be confiured with 16 partitions
 * 	to get the right unit number with rz macros.
 * 	[92/03/04            bernadat]
 * 
 * Revision 2.10.6.2  92/03/03  16:17:12  jeffreyh
 * 	Changes from TRUNK
 * 	[92/02/26  11:37:47  jeffreyh]
 * 
 * Revision 2.11  92/01/03  20:39:53  dbg
 * 	Added devinfo routine to scsi to accomodate MI change
 * 	that screwed up extra large writes.
 * 	[91/12/26  11:06:54  af]
 * 
 * Revision 2.10.6.1  92/02/18  18:51:26  jeffreyh
 * 	Suppressed old scsi driver
 * 	[91/09/27            bernadat]
 * 
 * 	Added high resolution clock device
 * 	(Jimmy Benjamin @ gr.orf.org)
 * 	[91/09/19            bernadat]
 * 
 * 	Added SCSI support
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.10  91/08/28  11:11:37  jsb
 * 	Fixed field-describing comment in dev_name_list definition.
 * 	[91/08/27  17:52:06  jsb]
 * 
 * 	Convert bsize entries to devinfo entries.  Add nodev entries for
 *	devices that don't support devinfo.
 * 	[91/08/15  18:43:13  jsb]
 * 
 * 	Add block size entries for hd and fd.
 * 	[91/08/12  17:32:55  dlb]
 * 
 * Revision 2.9  91/08/24  11:57:26  af
 * 	Added SCSI disks, tapes, and cpus.
 * 	[91/08/02  02:56:08  af]
 * 
 * Revision 2.8  91/05/14  16:22:01  mrt
 * 	Correcting copyright
 * 
 * Revision 2.7  91/02/14  14:42:13  mrt
 * 	Allow com driver and distinguish EtherLinkII from wd8003
 * 	[91/01/28  15:27:02  rvb]
 * 
 * Revision 2.6  91/02/05  17:16:44  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:42:38  mrt]
 * 
 * Revision 2.5  91/01/08  17:32:42  rpd
 * 	Support for get/set status on hd and fd.
 * 	Also fd has 64 minor devices per unit.
 * 	Switch wd8003 -> ns8390
 * 	[91/01/04  12:17:15  rvb]
 * 
 * Revision 2.4  90/10/01  14:23:02  jeffreyh
 * 	added wd8003 ethernet driver
 * 	[90/09/27  18:23:53  jeffreyh]
 * 
 * Revision 2.3  90/05/21  13:26:53  dbg
 * 	Add mouse, keyboard, IOPL devices.
 * 	[90/05/17            dbg]
 * 
 * Revision 2.2  90/05/03  15:41:34  dbg
 * 	Add 3c501 under name 'et'.
 * 	[90/04/27            dbg]
 * 
 * 	Created.
 * 	[90/02/20            dbg]
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
 * Device switch for i386 AT bus.
 */

#include <platforms.h>
#include <types.h>
#include <device/tty.h>
#include <device/conf.h>
#include <device/if_ether.h>
#include <kern/clock.h>
#include <chips/busses.h>

#include <i386/rtclock_entries.h>

#define	rtclockname		"rtclock"
#define	timename		"time"

#include <flipc.h>
#if	FLIPC > 0
#include <flipc/flipc_usermsg.h>
#define usermsgname		"usermsg"
#endif /* FLIPC > 0 */

#include <hd.h>
#if	NHD > 0
#include <i386/AT386/hd_entries.h>
#define	hdname			"hd"
#endif	/* NHD > 0 */

#include <fd.h>
#if	NFD > 0
#include <i386/AT386/fd_entries.h>
#define	fdname			"fd"
#endif	/* NFD > 0 */

#include <hba.h>
#if	NHBA > 0
#include <scsi/scsi_defs.h>
#define rzname "sd"
#define	tzname "st"
#define cdname	"cd_rom"	/* CD-ROM DA */

#endif	/*NHBA > 0*/

#include <aha.h>
#if	NAHA > 0
#include <scsi/scsi_defs.h>
#define rzname "sd"
#define	tzname "st"
#define cdname	"cd_rom"	/* CD-ROM DA */

#endif	/*NAHA > 0*/

#include <wt.h>
#if	NWT > 0
#define	wtname			"wt"
#endif	/* NWT > 0 */

#include <pc586.h>
#if	NPC586 > 0
#include <i386/AT386/if_pc586.h>
#include <i386/AT386/if_pc586_entries.h>
#define	pc586name		"pc"
#endif /* NPC586 > 0 */

#include <kkt.h>
#if	NKKT > 0
#include <i386/kkt/if_kkt.h>
#define ktname "kkt"
#endif	/* NKKT > 0 */

#include <sce.h>
#if	NSCE > 0
#include <scsi/if_sce.h>
#define scename "sce"
#endif	/* NSCE > 0 */

#include <ns8390.h>
#if	NNS8390 > 0
#include <i386/AT386/if_ns8390_entries.h>
#define	ns8390wdname		"wd"
#define	ns8390elname		"el"
#endif /* NNS8390 > 0 */

#include <at3c501.h>
#if	NAT3C501 > 0
#include <i386/AT386/if_3c501_entries.h>
#define	at3c501name		"et"
#endif /* NAT3C501 > 0 */

/* vvv MITCHEM vvv */
#include <ne.h>
#if	NNE > 0
#include <i386/AT386/if_ne_entries.h>
#define	nename		"ne"
#endif /* NNE > 0 */
/* ^^^ MITCHEM ^^^ */ 

#include <fta.h>
#if NFTA > 0
#include <i386/AT386/if_fta_entries.h>
#define	ftaname		"fta"
#endif /* NFTA > 0 */

#include <tu.h>
#if NTU > 0
#include <i386/AT386/if_tu_entries.h>
#define	tuname		"tu"
#endif /* NTU > 0 */

#include <par.h>
#if	NPAR > 0
#define	parname		"par"
#endif /* NPAR > 0 */

#include <i386/AT386/kd.h>
#include <i386/AT386/kdsoft.h>
#define	kdname			"kd"

#include <com.h>
#if	NCOM > 0
#include <i386/AT386/com_entries.h>
#define	comname			"com"
#endif	/* NCOM > 0 */

#include <sb.h>
#if	NSB > 0
#include <i386/AT386/sound/sb_entries.h>
#endif	/* NSB > 0 */

#include <lpr.h>
#if	NLPR > 0
#include <i386/AT386/lpr_entries.h>
#define	lprname			"lpr"
#endif	/* NLPR > 0 */

#include <qd.h>
#if	NQD > 0
#define	qdname			"qd"
#endif	/* NQD > 0 */

#include <blit.h>
#if NBLIT > 0
#include <i386/AT386/blitvar.h>
#define	blitname		"blit"
#endif

#include <i386/AT386/kbd_entries.h>
#define	kbdname			"kbd"

#include <i386/AT386/kd_mouse_entries.h>
#define	mousename		"mouse"

#include <i386/AT386/iopl_entries.h>
#define	ioplname		"iopl"

#include <vga.h>
#include <i386/AT386/vga_entries.h>
#define vganame			"vga"

#include <hi_res_clock.h>
#include <cpus.h>
#if	HI_RES_CLOCK && NCPUS > 1
#include <i386/hi_res_clock.h>
#define hi_res_clk_name         "hi_res_clock"
#endif  /* HI_RES_CLOCK && NCPUS > 1 */

#include <gprof.h>
#include <i386/AT386/gprof_entries.h>
#if     GPROF
#define gprofname               "gprof"
#endif

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

#include <physmem.h>
#if	NPHYSMEM > 0
#include <i386/AT386/physmem_entries.h>
#define physmemname		"physmem"
#endif	/* NPHYSMEM > 0 */

#include <device/data_device.h>
#define	datadevname		"data_device"

#include <scsiinfo.h>
#if NSCSIINFO > 0
#include <scsi/scsi_info_entries.h>
#define scsi_info_name  "scsi_info"
#endif

#include <i386/AT386/misc_protos.h>

/*
 * List of devices - console must be at slot 0
 */
struct dev_ops	dev_name_list[] =
{
	/*name,		open,		close,		read,
	  write,	getstat,	setstat,	mmap,
	  async_in,	reset,		port_death,	subdev,
	  dev_info */

	{ kdname,	kdopen,		kdclose,	kdread,
	  kdwrite,	kdgetstat,	kdsetstat,	kdmmap,
	  NO_ASYNC,	NULL_RESET,	kdportdeath,	0,
	  NO_DINFO },

	{ timename,	NULL_OPEN,	NULL_CLOSE,	NULL_READ,
	  NULL_WRITE,	NULL_GETS,	NULL_SETS,	utime_map,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

	{ rtclockname,	NULL_OPEN,	NULL_CLOSE,	NULL_READ,
	  NULL_WRITE,	NULL_GETS,	NULL_SETS,	rtclock_map,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

#if FLIPC > 0
	{ usermsgname,	usermsg_open,	usermsg_close,	NULL_READ,
	 NULL_WRITE,	usermsg_getstat,usermsg_setstat,usermsg_map,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },
#endif /* FLIPC > 0 */

#if	NHD > 0
	{ hdname,	hdopen,		hdclose,	hdread,
	  hdwrite,	hdgetstat,	hdsetstat,	NULL_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  hddevinfo },
#endif	/* NHD > 0 */

#if	NFD > 0
	{ fdname,	fdopen,		fdclose,	fdread,
	  fdwrite,	fdgetstat,	fdsetstat,	NULL_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	64,
	  fddevinfo },
#endif	/* NFD > 0 */

#if	NAHA > 0 || NHBA > 0
	{ rzname,	rz_open,	rz_close,	rz_read,
	  rz_write,	rz_get_status,	rz_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  rz_devinfo },

	{ tzname,	rz_open,	rz_close,	rz_read,
	  rz_write,	rz_get_status,	rz_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  NO_DINFO },

	{ cdname,	cd_open,	cd_close,	cd_read,
	  cd_write,	cd_get_status,	cd_set_status,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	16,
	  cd_devinfo },

#endif	/*NAHA > 0*/

#if	NWT > 0
	{ wtname,	wtopen,		wtclose,	wtread,
	  wtwrite,	NULL_GETS,	NULL_SETS,	NULL_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },
#endif	/* NWT > 0 */

#if	NPC586 > 0
	{ pc586name,	pc586open,	NULL_CLOSE,	NULL_READ,
	  pc586output,	pc586getstat,	pc586setstat,	NULL_MMAP,
	  pc586setinput,NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif

#if	NKKT > 0
	{ ktname,	ktopen,		NULL_CLOSE,	NULL_READ,
	  ktoutput,	ktgetstat,	ktsetstat,	NULL_MMAP,
	  ktsetinput,	NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif

#if	NSCE > 0
	{ scename,	sceopen,	NULL_CLOSE,	NULL_READ,
	  sceoutput,	scegetstat,	scesetstat,	NULL_MMAP,
	  scesetinput,	NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif

#if	NAT3C501 > 0
	{ at3c501name,	at3c501open,	NULL_CLOSE,	NULL_READ,
	  at3c501output,at3c501getstat,	at3c501setstat,	NULL_MMAP,
	  at3c501setinput, NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif

/* vvv MITCHEM vvv */
#if	NNE > 0
	{ nename,	neopen,	NULL_CLOSE,	NULL_READ,
	  neoutput,negetstat,	nesetstat,	NULL_MMAP,
	  nesetinput, NULL_RESET,	NULL_DEATH, 	0,
	  NO_DINFO },
#endif
/* ^^^ MITCHEM ^^^ */ 

#if	NNS8390 > 0
	{ ns8390wdname,	wd8003open,	ns8390close,	NULL_READ,
	  ns8390output, ns8390getstat,	ns8390setstat,	NULL_MMAP,
	  ns8390setinput, NULL_RESET,	NULL_DEATH, 	0,
	  ns8390devinfo },

	{ ns8390elname,	eliiopen,	ns8390close,	NULL_READ,
	  ns8390output, ns8390getstat,	ns8390setstat,	NULL_MMAP,
	  ns8390setinput, NULL_RESET,	NULL_DEATH, 	0,
	  ns8390devinfo },
#endif

#if NFTA > 0
	{ ftaname,       ftaopen,       NULL_CLOSE,     NULL_READ,
	  ftaoutput,     ftagetstatus,  ftasetstatus,   NULL_MMAP,
	  ftasetinput,   NULL_RESET,    NULL_DEATH,     0,
	  ftadevinfo },
#endif

#if NTU > 0
	{ tuname,        tuopen,        NULL_CLOSE,     NULL_READ,
	  tuoutput,      tugetstatus,   tusetstatus,    NULL_MMAP,
	  tusetinput,    NULL_RESET,    NULL_DEATH,     0,
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

#if	NSB > 0
	/*
	 * sb0 = mixer
	 * sb1 = sequencer
	 * sb2 = raw midi
	 * sb3 = dsp (degital voice)
	 * sb4 = audio
	 * sb5 16bit dsp
	 * sb6 status
	 * sb7 not used
	 * sb8 sequencer level 2 interface
	 * sb9 sndproc
	 */
	{ "sb",	sndopen,	sndclose,	sndread,
	  sndwrite,	sndgetstat,	sndsetstat,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NO_DEATH,	0,
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

	{ mousename,	mouseopen,	mouseclose,	mouseread,
	  NO_WRITE,	NULL_GETS,	NULL_SETS,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

	{ kbdname,	kbdopen,	kbdclose,	kbdread,
	  NO_WRITE,	kbdgetstat,	kbdsetstat,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

	{ ioplname,	ioplopen,	ioplclose,	NO_READ,
	  NO_WRITE,	NO_GETS,	NO_SETS,	ioplmmap,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

#if	NVGA > 0
	/* must be AFTER kd */
	{ vganame,	vgaopen,	vgaclose,	NO_READ,
	  NO_WRITE,	vgagetstat,	vgasetstat,	vgammap,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },
#endif

#if	HI_RES_CLOCK && NCPUS > 1
        { hi_res_clk_name, NULL_OPEN,	NULL_CLOSE,	NO_READ,
          NO_WRITE,        NULL_GETS,	NULL_SETS,	hi_res_clk_mmap,
          NO_ASYNC,        NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },
#endif  /* HI_RES_CLOCK && NCPUS > 1 */

#if     GPROF
        { gprofname,    gprofopen,      gprofclose,     gprofread,
          gprofwrite,   NULL_GETS,      NULL_SETS,      NULL_MMAP,
          NO_ASYNC,     NULL_RESET,     NULL_DEATH,     0,
          NO_DINFO },
#endif

#if     ETAP
        { etap_map_name,    etap_map_open,	NULL_CLOSE,	NO_READ,
          NO_WRITE,	    NULL_GETS,    	NULL_SETS,      etap_map_mmap,
          NO_ASYNC,	    NULL_RESET,		NULL_DEATH,     0,
          NO_DINFO },
#endif  /* ETAP */

#if	NTEST_DEVICE > 0
	{ testdevname,	testdev_open,	testdev_close,	testdev_read,
	  testdev_write,testdev_getstat,testdev_setstat,testdev_mmap,
	  testdev_async_in,testdev_reset,testdev_port_death,	0,
	  testdev_dev_info },
#endif	/* NTEST_DEVICE > 0 */

	{ datadevname,	datadev_open,	datadev_close,	datadev_read,
	  datadev_write,NULL_GETS,	NULL_SETS,	NO_MMAP,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
	  NO_DINFO },

#if	NPHYSMEM > 0
	{ physmemname,	physmem_open,	physmem_close,	physmem_read,
	  physmem_write,NULL_GETS,	NULL_SETS,	physmem_mmap,
	  NO_ASYNC,	NULL_RESET,	NULL_DEATH,	0,
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
 * Indirect list.
 */
struct dev_indirect dev_indirect_list[] = {
	{ "console",		&dev_name_list[0],	0 },
};
int	dev_indirect_count = sizeof(dev_indirect_list)
				/ sizeof(dev_indirect_list[0]);

int	disk_indirect_count = 0;
int	cdrom_indirect_count = 0;

/*
 * Clock device subsystem configuration. The clock_list[]
 * table contains the clock structures for all clocks in
 * the system.
 */

extern	struct clock_ops	rtc_ops;
extern	struct clock_ops	bbc_ops;

#include <cyctm.h>
#if	NCYCTM > 0
extern struct clock_ops		cyctm05_ops;
#endif

/*
 * List of clock devices.
 */
struct	clock	clock_list[] = {

	/* REALTIME_CLOCK */
	{ &rtc_ops,	0,		0,		0 },

	/* BATTERY_CLOCK */
	{ &bbc_ops,	0,		0,		0 },

	/* HIGHRES_CLOCK */
#if	NCYCTM > 0
	{ &cyctm05_ops,	0,		0,		0 },
#else
	{ 0,		0,		0,		0 },
#endif
};
int	clock_count = sizeof(clock_list) / sizeof(clock_list[0]);

#include <cdli.h>
#if	CDLI > 0
#include <device/cdli.h>

extern int cdli_ns8390init(void);

struct cdli_inits cdli_inits[] =
{
#if NCDLI8390 > 0
	{cdli_ns8390init},	/* Test CDLI driver using MK */
#endif
        {0}			/* Must be NULL terminated */
};
#endif
