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
/*
 * IDE CD-ROM driver for FreeBSD.
 * Supports ATAPI-compatible drives.
 *
 * Copyright (C) 1995 Cronyx Ltd.
 * Author Serge Vakulenko, <vak@cronyx.ru>
 *
 * This software is distributed with NO WARRANTIES, not even the implied
 * warranties for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Authors grant any other persons or organisations permission to use
 * or modify this software as long as this message is kept with the software,
 * all derivative works or modified versions.
 *
 * Version 1.9, Mon Oct  9 20:27:42 MSK 1995
 */

#include	"wd.h"

#if NWD > 0 

#define	NATAPICD	4

#include <cpus.h>
#include <chained_ios.h>
#include <device/param.h>
 
#include <string.h> 
#include <types.h>
#include <device/buf.h>
#include <device/conf.h>
#include <device/errno.h>
#include <device/subrs.h> 
#include <device/misc_protos.h>
#include <device/ds_routines.h>
#include <device/param.h>
#include <device/driver_lock.h>
#include <sys/ioctl.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <machine/disk.h>
#include <chips/busses.h>
#include <vm/vm_kern.h>
#include <ppc/proc_reg.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/wcd_entries.h>
#include <ppc/POWERMAC/atapi.h>
#include <ppc/POWERMAC/dbdma.h>
#include <device/cdrom_status.h>
#include <scsi/scsi.h>
#include <scsi/scsi2.h>

#define SECSIZE 2048                    /* CD-ROM sector size in bytes */

#define	DEV_BSIZE	SECSIZE
#define	MAXTRANSFER	64

#define UNIT(z)         ((minor(z) >> 4) & 0x03)

#define F_BOPEN         0x0001          /* The block device is opened */
#define F_MEDIA_CHANGED 0x0002          /* The media have changed since open */
#define F_DEBUG         0x0004          /* Print debug info */

/*
 * Disc table of contents.
 */
#define MAXTRK 99

struct atacd_toc_header {
        unsigned char   len1;           /* MSB */
        unsigned char   len2;           /* LSB */
        unsigned char   first_track;
        unsigned char   last_track;
};

typedef struct atacd_toc_header atacd_toc_header_t;

struct toc {
	atacd_toc_header_t	hdr;
	struct cdrom_toc_desc	tab[MAXTRK+1];
};

/*
 * Volume size info.
 */
struct volinfo {
	u_long volsize;         /* Volume size in blocks */
	u_long blksize;         /* Block size in bytes */
} info;

/*
 * Current subchannel status.
 */
struct subchan {
	u_char void0;
	u_char audio_status;
	u_short data_length;
	u_char data_format;
	u_char control;
	u_char track;
	u_char indx;
	u_long abslba;
	u_long rellba;
};

/*
 * Audio Control Parameters Page
 */
struct audiopage {
	/* Mode data header */
	u_short data_length;
	u_char  medium_type;
	u_char  reserved1[5];

	/* Audio control page */
	u_char  page_code;
#define AUDIO_PAGE      0x0e
#define AUDIO_PAGE_MASK 0x4e            /* changeable values */
	u_char  param_len;
	u_char  flags;
#define CD_PA_SOTC      0x02            /* mandatory */
#define CD_PA_IMMED     0x04            /* always 1 */
	u_char  reserved3[3];
	u_short lb_per_sec;
	struct port_control {
#ifdef	__powerpc__
		u_char	_res_ 	 : 4;
		u_char  channels : 4;
#else
		u_char  channels : 4;
		u_char	_res_ 	 : 4;
#endif
#define CHANNEL_0       1               /* mandatory */
#define CHANNEL_1       2               /* mandatory */
#define CHANNEL_2       4               /* optional */
#define CHANNEL_3       8               /* optional */
		u_char  volume;
	} port[4];
};

/*
 * CD-ROM Capabilities and Mechanical Status Page
 */
struct cappage {
	/* Mode data header */
	u_short data_length;
	u_char  medium_type;
#define MDT_UNKNOWN     0x00
#define MDT_DATA_120    0x01
#define MDT_AUDIO_120   0x02
#define MDT_COMB_120    0x03
#define MDT_PHOTO_120   0x04
#define MDT_DATA_80     0x05
#define MDT_AUDIO_80    0x06
#define MDT_COMB_80     0x07
#define MDT_PHOTO_80    0x08
#define MDT_NO_DISC     0x70
#define MDT_DOOR_OPEN   0x71
#define MDT_FMT_ERROR   0x72
	u_char  reserved1[5];

	/* Capabilities page */
	u_char  page_code;
#define CAP_PAGE        0x2a
	u_char  param_len;
	u_char  reserved2[2];

#ifdef __powerpc__
	u_char  : 1;
	u_char  multisession : 1;       /* multi-session photo-CD */
	u_char  mode2_form2 : 1;        /* mode 2 form 2 format */
	u_char  mode2_form1 : 1;        /* mode 2 form 1 (XA) read */
	u_char  dport2 : 1;             /* digital audio on port 2 */
	u_char  dport1 : 1;             /* digital audio on port 1 */
	u_char  composite : 1;          /* composite audio/video supported */
	u_char  audio_play : 1;         /* audio play supported */
#else
	u_char  audio_play : 1;         /* audio play supported */
	u_char  composite : 1;          /* composite audio/video supported */
	u_char  dport1 : 1;             /* digital audio on port 1 */
	u_char  dport2 : 1;             /* digital audio on port 2 */
	u_char  mode2_form1 : 1;        /* mode 2 form 1 (XA) read */
	u_char  mode2_form2 : 1;        /* mode 2 form 2 format */
	u_char  multisession : 1;       /* multi-session photo-CD */
	u_char  : 1;
#endif

#ifdef __powerpc__
	u_char  : 1;
	u_char  upc : 1;                /* can return the catalog number UPC */
	u_char  isrc : 1;               /* can return the ISRC info */
	u_char  c2 : 1;                 /* C2 error pointers supported */
	u_char  rw_corr : 1;            /* R-W subchannel data corrected */
	u_char  rw : 1;                 /* combined R-W subchannels */
	u_char  cd_da_stream : 1;       /* CD-DA streaming */
	u_char  cd_da : 1;              /* audio-CD read supported */
#else
	u_char  cd_da : 1;              /* audio-CD read supported */
	u_char  cd_da_stream : 1;       /* CD-DA streaming */
	u_char  rw : 1;                 /* combined R-W subchannels */
	u_char  rw_corr : 1;            /* R-W subchannel data corrected */
	u_char  c2 : 1;                 /* C2 error pointers supported */
	u_char  isrc : 1;               /* can return the ISRC info */
	u_char  upc : 1;                /* can return the catalog number UPC */
	u_char  : 1;
#endif

#ifdef __powerpc__
	u_char  mech : 3;               /* loading mechanism type */
	u_char  : 1;
	u_char  eject : 1;              /* can eject */
	u_char  prevent : 1;            /* prevent jumper installed */
	u_char  locked : 1;             /* current lock state */
	u_char  lock : 1;               /* could be locked */
#else
	u_char  lock : 1;               /* could be locked */
	u_char  locked : 1;             /* current lock state */
	u_char  prevent : 1;            /* prevent jumper installed */
	u_char  eject : 1;              /* can eject */
	u_char  : 1;
	u_char  mech : 3;               /* loading mechanism type */
#endif
#define MECH_CADDY      0
#define MECH_TRAY       1
#define MECH_POPUP      2
#define MECH_CHANGER    4
#define MECH_CARTRIDGE  5
#ifdef __powerpc__
	u_char  : 6;
	u_char  sep_mute : 1;           /* independent mute of channels */
	u_char  sep_vol : 1;            /* independent volume of channels */
#else
	u_char  sep_vol : 1;            /* independent volume of channels */
	u_char  sep_mute : 1;           /* independent mute of channels */
	u_char  : 6;
#endif

	u_short max_speed;              /* max raw data rate in bytes/1000 */
	u_short max_vol_levels;         /* number of discrete volume levels */
	u_short buf_size;               /* internal buffer size in bytes/1024 */
	u_short cur_speed;              /* current data rate in bytes/1000  */

	/* Digital drive output format description (optional?) */
	u_char  reserved3;
#ifdef __powerpc__
	u_char  : 3;
	u_char  dlen: 2;
	u_char  lsbf : 1;               /* set if LSB first */
	u_char  rch : 1;                /* high LRCK indicates left channel */
	u_char  bckf : 1;               /* data valid on failing edge of BCK */
#else
	u_char  bckf : 1;               /* data valid on failing edge of BCK */
	u_char  rch : 1;                /* high LRCK indicates left channel */
	u_char  lsbf : 1;               /* set if LSB first */
	u_char  dlen: 2;
	u_char  : 3;
#endif
#define DLEN_32         0               /* 32 BCKs */
#define DLEN_16         1               /* 16 BCKs */
#define DLEN_24         2               /* 24 BCKs */
#define DLEN_24_I2S     3               /* 24 BCKs (I2S) */
	u_char  reserved4[2];
};

struct wcd {
	struct atapi *ata;              /* Controller structure */
	int unit;                       /* IDE bus drive unit */
	int lun;                        /* Logical device unit */
	int flags;                      /* Device state flags */
	int refcnt;                     /* The number of raw opens */
	int	busy;			/* Is it busy? */
	struct io_req	*buf_queue;	/* Queue of i/o requests */
	struct atapi_params *param;     /* Drive parameters table */
	struct toc toc;                 /* Table of disc contents */
	struct volinfo info;            /* Volume size info */
	struct audiopage au;            /* Audio page info */
	struct cappage cap;             /* Capabilities page info */
	struct audiopage aumask;        /* Audio page mask */
	struct subchan subchan;         /* Subchannel info */
	char description[80];           /* Device description */
};

struct wcd *wcdtab[NATAPICD];      /* Drive info by unit number */
static int wcdnlun = 0;         /* Number of configured drives */

static void wcd_start (struct wcd *t);
static void wcd_done (struct wcd *t, struct buf *bp, int resid,
	struct atapires result);
static void wcd_error (struct wcd *t, struct atapires result);
static int wcd_read_toc (struct wcd *t);
static int wcd_request_wait (struct wcd *t, u_char cmd, u_char a1, u_char a2,
	u_char a3, u_char a4, u_char a5, u_char a6, u_char a7, u_char a8,
	u_char a9, char *addr, int count);
static void wcd_describe (struct wcd *t);
static int wcd_setchan (struct wcd *t,
	u_char c0, u_char c1, u_char c2, u_char c3);
static int wcd_eject (struct wcd *t, int closeit);
void wcdstrategy (struct buf *bp);

struct atapi_cdrom {
	struct	atapi	*controller;
	int		unit;
	boolean_t	found;
};

static struct atapi_cdrom	atapi_cdroms[NATAPICD];

int wcdprobe(int port,  struct bus_device *bus);
int wcdslave(struct bus_device  * bd, caddr_t xxx);
void wcdattach(struct bus_device  *dev);

caddr_t wcd_std[NATAPICD] = { 0 };
struct  bus_device      *wcd_dinfo[NATAPICD];
struct  bus_ctlr        *wcd_minfo[NATAPICD];
struct  bus_driver      wcd_driver = {
        (probe_t)wcdprobe, wcdslave, wcdattach, 0, wcd_std, "atapicd",
	wcd_dinfo, "atapicd", wcd_minfo, 0};

int
wcdprobe(int port,  struct bus_device *bus)
{
	int	i, subunit, unit = bus->unit;
	struct atapi_cdrom	*acd = atapi_cdroms;
	struct atapi		*ap = atapitab;

	if (unit == 0) {
		for (i = 0; i < 4; i++, ap++) {
			for (subunit = 0; subunit < 2; subunit++) {
				if (ap->params[subunit]
				&& ap->params[subunit]->devtype == AT_TYPE_CDROM) {
					if (wcdnlun < NATAPICD) {
						acd->controller = ap;
						acd->unit       = subunit;
						wcdnlun++;
					}
				}
			}
		}
	}

	return (unit  < wcdnlun);
}
	
int
wcdslave(struct bus_device  * bd, caddr_t xxx)
{
	return 1;
}

/*
 * Dump the array in hexadecimal format for debugging purposes.
 */
static void wcd_dump (int lun, char *label, void *data, int len)
{
	u_char *p = data;

	printf ("wcd%d: %s %x", lun, label, *p++);
	while (--len > 0)
		printf ("-%x", *p++);
	printf ("\n");
}

void
wcdattach(struct bus_device *dev)
{
	struct atapi *ata;
	int unit, debug = 0;
	struct atapi_params *ap;
	struct wcd *t;
	struct atapires result;
	int lun, tmpunit;
	struct	atapi_cdrom *acd = &atapi_cdroms[dev->unit];
	dev_ops_t	wcd_ops;
	char	devname[10];

	if (kmem_alloc(kernel_map, (vm_offset_t *)&t, sizeof(&t)) != KERN_SUCCESS) {
		printf ("wcd: out of memory\n");
		return;
	}

	ata = acd->controller;
	ap = ata->params[acd->unit];
	unit = acd->unit;

	wcdtab[dev->unit] = t;
	bzero ((char *)t, sizeof (struct wcd));
	t->ata = ata;
	t->unit = acd->unit;
	lun = t->lun = dev->unit;
	t->param = ap;
	t->flags = F_MEDIA_CHANGED;
	t->refcnt = 0;
	if (debug) {
		t->flags |= F_DEBUG;
		/* Print params. */
		wcd_dump (t->lun, "info", ap, sizeof *ap);
	}

	/* Get drive capabilities. */
	result = atapi_request_immediate (ata, unit, ATAPI_MODE_SENSE,
		0, CAP_PAGE, 0, 0, 0, 0, sizeof (t->cap) >> 8, sizeof (t->cap),
		0, 0, 0, 0, 0, 0, 0, (char*) &t->cap, sizeof (t->cap));

	/* Do it twice to avoid the stale media changed state. */
	if (result.code == RES_ERR &&
	    (result.error & AER_SKEY) == AER_SK_UNIT_ATTENTION)
		result = atapi_request_immediate (ata, unit, ATAPI_MODE_SENSE,
			0, CAP_PAGE, 0, 0, 0, 0, sizeof (t->cap) >> 8,
			sizeof (t->cap), 0, 0, 0, 0, 0, 0, 0,
			(char*) &t->cap, sizeof (t->cap));

	/* Some drives have shorter capabilities page. */
	if (result.code == RES_UNDERRUN)
		result.code = 0;

	if (result.code == 0) {
		wcd_describe (t);
		if (t->flags & F_DEBUG)
			wcd_dump (t->lun, "cap", &t->cap, sizeof t->cap);
	}

	strcpy(devname, "hd");
	itoa((t->ata->ctrlr * 2) + t->unit, &devname[2]);

	if (dev_name_lookup("wcd", &wcd_ops, &tmpunit)) {
		dev_set_indirection(devname, wcd_ops,
				dev->unit * wcd_ops->d_subdev);
	} 

	printf(" (%s)", devname);

	return;
}

void wcd_describe (struct wcd *t)
{
	char *m;

#if 0
	t->cap.max_speed      = ntohs (t->cap.max_speed);
	t->cap.max_vol_levels = ntohs (t->cap.max_vol_levels);
	t->cap.buf_size       = ntohs (t->cap.buf_size);
	t->cap.cur_speed      = ntohs (t->cap.cur_speed);
#endif

	printf ("wcd%d: ", t->lun);
	if (t->cap.cur_speed != t->cap.max_speed)
		printf ("%d/", t->cap.cur_speed * 1000 / 1024);
	printf ("%dKb/sec", t->cap.max_speed * 1000 / 1024);
	if (t->cap.buf_size)
		printf (", %dKb cache", t->cap.buf_size);

	if (t->cap.audio_play)
		printf (", audio play");
	if (t->cap.max_vol_levels)
		printf (", %d volume levels", t->cap.max_vol_levels);

	switch (t->cap.mech) {
	default:             m = 0;           break;
	case MECH_CADDY:     m = "caddy";     break;
	case MECH_TRAY:      m = "tray";      break;
	case MECH_POPUP:     m = "popup";     break;
	case MECH_CHANGER:   m = "changer";   break;
	case MECH_CARTRIDGE: m = "cartridge"; break;
	}
	if (m)
		printf (", %s%s", t->cap.eject ? "ejectable " : "", m);
	else if (t->cap.eject)
		printf (", eject");
	printf ("\n");

	printf ("wcd%d: ", t->lun);
	switch (t->cap.medium_type) {
	case MDT_UNKNOWN:   printf ("medium type unknown");          break;
	case MDT_DATA_120:  printf ("120mm data disc loaded");       break;
	case MDT_AUDIO_120: printf ("120mm audio disc loaded");      break;
	case MDT_COMB_120:  printf ("120mm data/audio disc loaded"); break;
	case MDT_PHOTO_120: printf ("120mm photo disc loaded");      break;
	case MDT_DATA_80:   printf ("80mm data disc loaded");        break;
	case MDT_AUDIO_80:  printf ("80mm audio disc loaded");       break;
	case MDT_COMB_80:   printf ("80mm data/audio disc loaded");  break;
	case MDT_PHOTO_80:  printf ("80mm photo disc loaded");       break;
	case MDT_NO_DISC:   printf ("no disc inside");               break;
	case MDT_DOOR_OPEN: printf ("door open");                    break;
	case MDT_FMT_ERROR: printf ("medium format error");          break;
	default:    printf ("medium type=0x%x", t->cap.medium_type); break;
	}
	if (t->cap.lock)
		printf (t->cap.locked ? ", locked" : ", unlocked");
	if (t->cap.prevent)
		printf (", lock protected");
	printf ("\n");
}

int 
wcdopen(dev_t dev, dev_mode_t flags, io_req_t ior)
{
	int lun = UNIT(dev);
	struct wcd *t;
	kern_return_t	err;

	/* Check that the device number is legal
	 * and the ATAPI driver is loaded. */
	if (lun >= wcdnlun) 
		return (ENXIO);
	t = wcdtab[lun];

	/* On the first open, read the table of contents. */
	if (!t->refcnt && (flags & D_NODELAY) == 0) {
		/* Read table of contents. */
		if (err = wcd_read_toc(t)) 
			return (err);

		/* Lock the media. */
		wcd_request_wait (t, ATAPI_PREVENT_ALLOW,
			0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
	}
	++t->refcnt;
	return (0);
}

/*
 * Close the device.  Only called if we are the LAST
 * occurence of an open device.
 */
void wcdclose (dev_t dev)
{
	int lun = UNIT(dev);
	struct wcd *t = wcdtab[lun];

	t->refcnt--;
	/* If we were the last open of the entire device, release it. */
	if (! t->refcnt)
		wcd_request_wait (t, ATAPI_PREVENT_ALLOW,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	return;

}


static void
wcdminphys(struct buf * bp)
{
	if (bp->b_bcount > DEV_BSIZE * MAXTRANSFER)
		bp->b_bcount = DEV_BSIZE * MAXTRANSFER;
}

io_return_t
wcdread(dev_t	dev, io_req_t ior)
{
	return (block_io(wcdstrategy, wcdminphys, ior));
}

io_return_t
wcdwrite(dev_t dev, io_req_t ior)
{
	return (block_io(wcdstrategy, wcdminphys, ior));
}


io_return_t
wcdgetstat( 
        dev_t           dev,
        dev_flavor_t    flavor,
        dev_status_t    data,   /* pointer to OUT array */
        natural_t       *count) /* OUT */
{

	struct wcd *cd = wcdtab[UNIT(dev)];

	switch (flavor) {
	case DEV_GET_SIZE: 
		data[DEV_GET_SIZE_DEVICE_SIZE] = cd->info.volsize * cd->info.blksize;
		data[DEV_GET_SIZE_RECORD_SIZE] = cd->info.blksize;
		*count = DEV_GET_SIZE_COUNT;
		break;


	case MACH_SCMD_READ_HEADER: {
		cd_toc_header_t *toc_header = (cd_toc_header_t *)data;
    
		if (*count < ((sizeof(cd_toc_header_t) + 3) >> 2)) 
			return D_INVALID_SIZE;
		else {
			toc_header->len = (cd->toc.hdr.len1 << 8)|cd->toc.hdr.len2;

			toc_header->first_track = cd->toc.hdr.first_track;
			toc_header->last_track = cd->toc.hdr.last_track;
		}

	}
		break;

	case MACH_SCMD_READ_TOC_MSF:
	case MACH_SCMD_READ_TOC_LBA: {
	    cd_toc_t *cd_toc = (cd_toc_t *)data;
	    int tr = 1; /* starting track */
	    int tlen, et_count = 0, next_size;
	    int reserved_count = *count;
	    cdrom_toc_t *toc = (cdrom_toc_t *) &cd->toc;

	    if (*count < ((sizeof(cd_toc_t) + 3) >> 2))
		return D_INVALID_SIZE;

		tlen = (toc->len1 << 8) + toc->len2;
		tlen -= 4; /* header */

		for (tr = 0; tlen > 0; tr++, tlen -= sizeof(struct cdrom_toc_desc)){
		  cd_toc->cdt_ctrl = toc->descs[tr].control;
		  cd_toc->cdt_adr = toc->descs[tr].adr;
		  cd_toc->cdt_track = toc->descs[tr].trackno;

		  if (flavor == MACH_SCMD_READ_TOC_MSF) {
		    cd_toc->cdt_addr.msf.minute = 
		    	toc->descs[tr].absolute_address.msf.minute;
		    cd_toc->cdt_addr.msf.second =
		    	toc->descs[tr].absolute_address.msf.second;
		    cd_toc->cdt_addr.msf.frame = 
		    	toc->descs[tr].absolute_address.msf.frame;
		    cd_toc->cdt_type = MACH_CDROM_MSF;
		  }
		  else {	/* flavor == MACH_SCMD_READ_TOC_LBA */
		    cd_toc->cdt_addr.lba = 
		    	(toc->descs[tr].absolute_address.lba.lba1<<24)+
		    	(toc->descs[tr].absolute_address.lba.lba2<<16)+
		    	(toc->descs[tr].absolute_address.lba.lba3<<8)+
		    	toc->descs[tr].absolute_address.lba.lba4;
		    cd_toc->cdt_type = MACH_CDROM_LBA;
		  }

		  cd_toc++; /* next Toc entry */
		  et_count++;
		  /* if not enough space for the next structure, stop now */
		  next_size = (((sizeof(cd_toc_t)  * (et_count + 1)) + 3) >> 2);
		  if (next_size > reserved_count)
		    break;  /* leave the loop */
		}
		*count = (((sizeof(cd_toc_t)  * et_count) + 3) >> 2);
		if (*count > reserved_count)
		  *count = reserved_count;

	}
	break;

	case MACH_SCMD_READ_SUBCHNL_MSF:
	case MACH_SCMD_READ_SUBCHNL_LBA: {

	  /* "Get Position MSF|ABS" */
	    cd_subchnl_t *subchnl = (cd_subchnl_t *)data;
	   cdrom_chan_curpos_t	*st;
    
	    if (*count < ((sizeof(cd_subchnl_t) + 3) >> 2)) 
		return D_INVALID_SIZE;

		if (wcd_request_wait (cd, ATAPI_READ_SUBCHANNEL, 0, 0x40, 1, 0,
		    0, 0, sizeof (cd->subchan) >> 8, sizeof (cd->subchan),
		    0, (char*)&cd->subchan, sizeof (cd->subchan)) != 0)
			return (EIO);

		st = (cdrom_chan_curpos_t *)&cd->subchan;

		subchnl->cdsc_track = st->subQ.trackno;
		subchnl->cdsc_index = st->subQ.indexno;
		subchnl->cdsc_format = st->subQ.format;
		subchnl->cdsc_adr = st->subQ.adr;
		subchnl->cdsc_ctrl = st->subQ.control;
		subchnl->cdsc_audiostatus = st->audio_status;

		if (flavor == MACH_SCMD_READ_SUBCHNL_MSF) {
		  subchnl->cdsc_absolute_addr.msf.minute = 
		  	st->subQ.absolute_address.msf.minute;
		  subchnl->cdsc_absolute_addr.msf.second = 
		  	st->subQ.absolute_address.msf.second;
		  subchnl->cdsc_absolute_addr.msf.frame = 
		  	st->subQ.absolute_address.msf.frame;
		  subchnl->cdsc_relative_addr.msf.minute = 
		  	st->subQ.relative_address.msf.minute;
		  subchnl->cdsc_relative_addr.msf.second = 
		  	st->subQ.relative_address.msf.second;
		  subchnl->cdsc_relative_addr.msf.frame = 
		  	st->subQ.relative_address.msf.frame;

		  subchnl->cdsc_type = MACH_CDROM_MSF;
		}
		else {	/* flavor == MACH_SCMD_READ_SUBCHNL_LBA */
		  subchnl->cdsc_absolute_addr.lba = 
		  	(st->subQ.absolute_address.lba.lba1<<24)+
			(st->subQ.absolute_address.lba.lba2<<16)+
			(st->subQ.absolute_address.lba.lba3<< 8)+
			st->subQ.absolute_address.lba.lba4;
		  subchnl->cdsc_relative_addr.lba = 
			(st->subQ.relative_address.lba.lba1<<24)+
			(st->subQ.relative_address.lba.lba2<<16)+
			(st->subQ.relative_address.lba.lba3<< 8)+
			st->subQ.relative_address.lba.lba4;

		  subchnl->cdsc_type = MACH_CDROM_LBA;
		}
		*count = ((sizeof(cd_subchnl_t) + 3) >> 2);
	  }
	break;

	case MACH_CDROMVOLREAD: {
		cd_volctrl_t *volctrl;
		int error;
	
		volctrl = (cd_volctrl_t *) data;

		if (*count != ((sizeof(cd_volctrl_t) + 3) >> 2)) 
			return D_INVALID_SIZE;

		error = wcd_request_wait (cd, ATAPI_MODE_SENSE, 0, AUDIO_PAGE,
			0, 0, 0, 0, sizeof (cd->au) >> 8, sizeof (cd->au), 0,
			(char*) &cd->au, sizeof (cd->au));
		if (error)
			return (error);

		if (cd->flags & F_DEBUG)
			wcd_dump (cd->lun, "au", &cd->au, sizeof cd->au);

		if (cd->au.page_code != AUDIO_PAGE)
			return (EIO);

		volctrl->channel0 = cd->au.port[0].volume;
		volctrl->channel1 = cd->au.port[1].volume;
		volctrl->channel2 = cd->au.port[2].volume;
		volctrl->channel3 = cd->au.port[3].volume;
		break;
	}
	default:
		return D_INVALID_OPERATION;
	}

	return KERN_SUCCESS;

}

io_return_t
wcdsetstat(
        dev_t           dev,
        dev_flavor_t    flavor,
        dev_status_t    data,
        natural_t       count)
{
	struct wcd	*t = wcdtab[UNIT(dev)];
	kern_return_t	error;

	switch (flavor) {
	case MACH_CDROMEJECT:
		if (t->refcnt != 1)
			return (D_ALREADY_OPEN);
		return wcd_eject(t, 0);

	case MACH_CDROMPLAYMSF:
		  /* "Play A startM startS startF endM endS endF" */
	{
		cd_msf_t *msf_req;

		msf_req = (cd_msf_t *) data;

		if (count != ((sizeof(cd_msf_t) + 3) >> 2)) 
			return D_INVALID_SIZE;
		else
			return wcd_request_wait (t, ATAPI_PLAY_MSF, 0, 0,
         			msf_req->cdmsf_min0, msf_req->cdmsf_sec0,
				msf_req->cdmsf_frame0,
				msf_req->cdmsf_min1, msf_req->cdmsf_sec1,
				msf_req->cdmsf_frame1,
				0,0,0);
	  }
		break;

	case MACH_CDROMPLAYTRKIND:
		  /* "Play TI startT startI endT endI" */
	{
		cd_ti_t *ti_req;
		int	t1, t2;
		u_long start, len, lba;

		ti_req = (cd_ti_t *) data;

		if (count != ((sizeof(cd_ti_t) + 3) >> 2)) 
			return D_INVALID_SIZE;

		/* Ignore index fields,
		 * play from start_track to end_track inclusive. */
		if (ti_req->cdti_track1 < t->toc.hdr.last_track+1)
			++ti_req->cdti_track1;
		if (ti_req->cdti_track1 > t->toc.hdr.last_track+1)
			ti_req->cdti_track1 = t->toc.hdr.last_track+1;
		t1 = ti_req->cdti_track0 - t->toc.hdr.first_track;
		t2 = ti_req->cdti_track1 - t->toc.hdr.first_track;
		if (t1 < 0 || t2 < 0)
			return (EINVAL);
#define	LBA(x)	((x.lba1<<24)+ (x.lba2<<16)+ (x.lba3<<8)+ (x.lba4))

		start = LBA(t->toc.tab[t1].absolute_address.lba);
		lba = LBA(t->toc.tab[t2].absolute_address.lba);
		len = lba - start;

		return wcd_request_wait (t, ATAPI_PLAY_BIG, 0,
			start >> 24 & 0xff, start >> 16 & 0xff,
			start >> 8 & 0xff, start & 0xff,
			len >> 24 & 0xff, len >> 16 & 0xff,
			len >> 8 & 0xff, len & 0xff, 0, 0);
	 }
		break;

	case MACH_CDROMPAUSE: /* "Pause" */
		return wcd_request_wait (t, ATAPI_PAUSE,
                        0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);


	case MACH_CDROMRESUME:
		return wcd_request_wait (t, ATAPI_PAUSE,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	case MACH_CDROMSTART: /* "Start" */
		return wcd_request_wait (t, ATAPI_START_STOP,
			1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);


	case MACH_CDROMSTOP: /* "Stop" */
		return wcd_request_wait (t, ATAPI_START_STOP,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


	case MACH_CDROMVOLCTRL:
	/* "Set V chan0vol chan1vol chan2vol chan3vol" */
	{
		cd_volctrl_t *volctrl;
		int v0, v1, v2, v3;
		cdrom_audio_page_t	au, *aup;

		volctrl = (cd_volctrl_t *) data;

		if (count != ((sizeof(cd_volctrl_t) + 3) >> 2)) 
			return D_INVALID_SIZE;

		error = wcd_request_wait (t, ATAPI_MODE_SENSE, 0, AUDIO_PAGE,
			0, 0, 0, 0, sizeof (t->au) >> 8, sizeof (t->au), 0,
			(char*) &t->au, sizeof (t->au));
		if (error)
			return (error);
		if (t->flags & F_DEBUG)
			wcd_dump (t->lun, "au", &t->au, sizeof t->au);
		if (t->au.page_code != AUDIO_PAGE)
			return (EIO);

		error = wcd_request_wait (t, ATAPI_MODE_SENSE, 0,
			AUDIO_PAGE_MASK, 0, 0, 0, 0, sizeof (t->aumask) >> 8,
			sizeof (t->aumask), 0, (char*) &t->aumask,
			sizeof (t->aumask));
		if (error)
			return (error);
		if (t->flags & F_DEBUG)
			wcd_dump (t->lun, "mask", &t->aumask, sizeof t->aumask);

		/* Sony-55E requires the data length field to be zeroed. */
		t->au.data_length = 0;

		t->au.port[0].channels = CHANNEL_0;
		t->au.port[1].channels = CHANNEL_1;
		t->au.port[0].volume = volctrl->channel0 & t->aumask.port[0].volume;
		t->au.port[1].volume = volctrl->channel1 & t->aumask.port[1].volume;
		t->au.port[2].volume = volctrl->channel2 & t->aumask.port[2].volume;
		t->au.port[3].volume = volctrl->channel3 & t->aumask.port[3].volume;
		return wcd_request_wait (t, ATAPI_MODE_SELECT_BIG, 0x10,
			0, 0, 0, 0, 0, sizeof (t->au) >> 8, sizeof (t->au),
			0, (char*) &t->au, - sizeof (t->au));
	}
		break;

	default:
		return D_INVALID_OPERATION;
	}

	return	KERN_SUCCESS;
}

io_return_t
wcddevinfo(
	dev_t		dev,
	dev_flavor_t	flavor,
	char		*info)
{
	register int	result;
	struct wcd *cd = wcdtab[UNIT(dev)];

	result = D_SUCCESS;

	switch (flavor) {
	case D_INFO_BLOCK_SIZE:
		*((int *) info) = cd->info.blksize;
		break;
	default:
		result = D_INVALID_OPERATION;
	}

	return(result);
}


/*
 * Actually translate the requested transfer into one the physical driver can
 * understand. The transfer is described by a buf and will include only one
 * physical transfer.
 */
void wcdstrategy (struct buf *bp)
{
	int lun = UNIT(bp->b_dev);
	struct wcd *t = wcdtab[lun];
	int x;

	/* Can't ever write to a CD. */
	if (! (bp->b_flags & B_READ)) {
		bp->b_error = D_READ_ONLY;
		bp->b_flags |= B_ERROR;
		biodone (bp);
		return;
	}

	/* If it's a null transfer, return immediatly. */
	if (bp->b_bcount == 0) {
		bp->b_resid = 0;
		biodone (bp);
		return;
	}

	/* Check to make sure byte count is in exact multiples of 
	 * the block size..
	 */

	if (bp->b_bcount % t->info.blksize) {
		bp->b_error = D_INVALID_SIZE;
		bp->b_flags |= IO_ERROR;
		biodone(bp);
		return;
	}

	/* Process transfer request. */
	bp->b_resid = bp->b_bcount;
	x = splbio();

	bp->io_prev = bp->io_next = NULL;

	/* Place it in the queue of disk activities for this disk. */
	if (t->buf_queue)
		disksort (t->buf_queue, bp);
	else
		t->buf_queue = bp;

	/* Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion. */
	if (t->busy == 0)
		wcd_start (t);
	splx(x);
}

/*
 * Look to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It dequeues the buf and creates an ATAPI command to perform the
 * transfer in the buf.
 * The bufs are queued by the strategy routine (wcdstrategy).
 * Must be called at the correct (splbio) level.
 */
static void wcd_start (struct wcd *t)
{
	struct buf *bp = t->buf_queue;
	u_long blkno, nblk;

	/* See if there is a buf to do and we are not already doing one. */
	if (! bp)
		return;

	/* Unqueue the request. */
	t->buf_queue = bp->io_next;

	/* Should reject all queued entries if media have changed. */
	if (t->flags & F_MEDIA_CHANGED) {
		bp->b_error = D_DEVICE_DOWN;
		bp->b_flags |= B_ERROR;
		biodone (bp);
		return;
	}

	t->busy = TRUE;

	/* We have a buf, now we should make a command
	 * First, translate the block to absolute and put it in terms of the
	 * logical blocksize of the device.
	 * What if something asks for 512 bytes not on a 2k boundary? */
#if 0
	blkno = bp->b_blkno / (SECSIZE / 512);
	nblk = (bp->b_bcount + (SECSIZE - 1)) / SECSIZE;
#else
	blkno = bp->io_recnum;
	nblk = bp->b_bcount/t->info.blksize;
#endif

	atapi_request_callback (t->ata, t->unit, ATAPI_READ_BIG, 0,
		blkno>>24, blkno>>16, blkno>>8, blkno, 0, nblk>>8, nblk, 0, 0,
		0, 0, 0, 0, 0, (u_char*) bp->io_data, bp->b_bcount,
		wcd_done, t, bp);
}

static void wcd_done (struct wcd *t, struct buf *bp, int resid,
	struct atapires result)
{
	if (result.code) {
		wcd_error (t, result);
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
	} else
		bp->b_resid = resid;
	t->busy = FALSE;
	biodone (bp);
	wcd_start (t);
}

static void wcd_error (struct wcd *t, struct atapires result)
{
	if (result.code != RES_ERR)
		return;
	switch (result.error & AER_SKEY) {
	case AER_SK_NOT_READY:
		if (result.error & ~AER_SKEY) {
			/* Audio disc. */
			//printf ("wcd%d: cannot read audio disc\n", t->lun);
			return;
		}
		/* Tray open. */
		if (! (t->flags & F_MEDIA_CHANGED))
			printf ("wcd%d: tray open\n", t->lun);
		t->flags |= F_MEDIA_CHANGED;
		return;

	case AER_SK_UNIT_ATTENTION:
		/* Media changed. */
		if (! (t->flags & F_MEDIA_CHANGED))
			printf ("wcd%d: media changed\n", t->lun);
		t->flags |= F_MEDIA_CHANGED;
		return;

	case AER_SK_ILLEGAL_REQUEST:
		/* Unknown command or invalid command arguments. */
		if (t->flags & F_DEBUG)
			printf ("wcd%d: invalid command\n", t->lun);
		return;
	}
	printf ("wcd%d: i/o error, status=%b, error=%b\n", t->lun,
		result.status, ARS_BITS, result.error, AER_BITS);
}

static int wcd_request_wait (struct wcd *t, u_char cmd, u_char a1, u_char a2,
	u_char a3, u_char a4, u_char a5, u_char a6, u_char a7, u_char a8,
	u_char a9, char *addr, int count)
{
	struct atapires result;

	result = atapi_request_wait (t->ata, t->unit, cmd,
		a1, a2, a3, a4, a5, a6, a7, a8, a9, 0, 0, 0, 0, 0, 0,
		addr, count);
	if (result.code) {
		wcd_error (t, result);
		return (EIO);
	}
	return (0);
}

static inline void lba2msf (int lba, u_char *m, u_char *s, u_char *f)
{
	lba += 150;             /* offset of first logical frame */
	lba &= 0xffffff;        /* negative lbas use only 24 bits */
	*m = lba / (60 * 75);
	lba %= (60 * 75);
	*s = lba / 75;
	*f = lba % 75;
}

/*
 * Read the entire TOC for the disc into our internal buffer.
 */
static int wcd_read_toc (struct wcd *t)
{
	int ntracks, len, res;
	struct atapires result;

	bzero ((char *)&t->toc, sizeof (t->toc));
	bzero ((char *)&t->info, sizeof (t->info));

	/* Check for the media.
	 * Do it twice to avoid the stale media changed state. */
	result = atapi_request_wait (t->ata, t->unit, ATAPI_TEST_UNIT_READY,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (result.code == RES_ERR &&
	    (result.error & AER_SKEY) == AER_SK_UNIT_ATTENTION) {
		t->flags |= F_MEDIA_CHANGED;
		result = atapi_request_wait (t->ata, t->unit,
			ATAPI_TEST_UNIT_READY, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	if (result.code) {
		wcd_error (t, result);
		return D_DEVICE_DOWN;
	}
	t->flags &= ~F_MEDIA_CHANGED;

	/* Read disc capacity. */
	if ((res = wcd_request_wait (t, ATAPI_READ_CAPACITY, 0, 0, 0, 0, 0, 0,
	    0, sizeof(t->info), 0, (char*)&t->info, sizeof(t->info))) != 0) {
		bzero ((char *)&t->info, sizeof (t->info));
		return D_DEVICE_DOWN;
	}

	/* First read just the header, so we know how long the TOC is. */
	len = sizeof(t->toc.hdr) + sizeof(t->toc.tab[0]);
	if ((res = wcd_request_wait (t, ATAPI_READ_TOC, 0, 0, 0, 0, 0, 0,
	    len >> 8, len & 0xff, 0, (char*)&t->toc, len)) != 0) {
err:            bzero ((char *)&t->toc, sizeof (t->toc));
		return (0);
	}

	ntracks = t->toc.hdr.last_track - t->toc.hdr.first_track + 1;

	if (ntracks <= 0 || ntracks > MAXTRK) 
		goto err;

	/* Now read the whole schmeer. */
	len = sizeof(t->toc.hdr) + (ntracks * sizeof(t->toc.tab[0]));
	if ((res = wcd_request_wait (t, ATAPI_READ_TOC, 0, 0, 0, 0, 0, 0,
	    len >> 8, len & 0xff, 0, (char*)&t->toc, len)) & 0xff)  {
		goto err;
	}


	/* make fake leadout entry */
	t->toc.tab[ntracks].control = t->toc.tab[ntracks-1].control;
	t->toc.tab[ntracks].absolute_address = t->toc.tab[ntracks-1].absolute_address;
	t->toc.tab[ntracks].trackno = 170; /* magic */
	*(unsigned long *)&t->toc.tab[ntracks].absolute_address.lba = t->info.volsize;


	/* Print the disc description string on every disc change.
	 * It would help to track the history of disc changes. */
	if (t->info.volsize && t->toc.hdr.last_track &&
	    (t->flags & F_MEDIA_CHANGED) && (t->flags & F_DEBUG)) {
		printf ("wcd%d: ", t->lun);
		if (t->toc.tab[0].control & 4)
			printf ("%ldMB ", t->info.volsize / 512);
		else
			printf ("%ld:%ld audio ", t->info.volsize/75/60,
				t->info.volsize/75%60);
		printf ("(%ld sectors), %d tracks\n", t->info.volsize,
			t->toc.hdr.last_track - t->toc.hdr.first_track + 1);
	}
	return (0);
}

/*
 * Set up the audio channel masks.
 */
static int wcd_setchan (struct wcd *t,
	u_char c0, u_char c1, u_char c2, u_char c3)
{
	int error;

	error = wcd_request_wait (t, ATAPI_MODE_SENSE, 0, AUDIO_PAGE,
		0, 0, 0, 0, sizeof (t->au) >> 8, sizeof (t->au), 0,
		(char*) &t->au, sizeof (t->au));
	if (error)
		return (error);
	if (t->flags & F_DEBUG)
		wcd_dump (t->lun, "au", &t->au, sizeof t->au);
	if (t->au.page_code != AUDIO_PAGE)
		return (EIO);

	/* Sony-55E requires the data length field to be zeroed. */
	t->au.data_length = 0;

	t->au.port[0].channels = c0;
	t->au.port[1].channels = c1;
	t->au.port[2].channels = c2;
	t->au.port[3].channels = c3;
	return wcd_request_wait (t, ATAPI_MODE_SELECT_BIG, 0x10,
		0, 0, 0, 0, 0, sizeof (t->au) >> 8, sizeof (t->au),
		0, (char*) &t->au, - sizeof (t->au));
}

static int wcd_eject (struct wcd *t, int closeit)
{
	struct atapires result;

	/* Try to stop the disc. */
	result = atapi_request_wait (t->ata, t->unit, ATAPI_START_STOP,
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (result.code == RES_ERR &&
	    ((result.error & AER_SKEY) == AER_SK_NOT_READY ||
	    (result.error & AER_SKEY) == AER_SK_UNIT_ATTENTION)) {
		int err;

		if (!closeit)
			return (0);
		/*
		 * The disc was unloaded.
		 * Load it (close tray).
		 * Read the table of contents.
		 */
		err = wcd_request_wait (t, ATAPI_START_STOP,
			0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0);
		if (err)
			return (err);

		/* Read table of contents. */
		wcd_read_toc (t);

		/* Lock the media. */
		wcd_request_wait (t, ATAPI_PREVENT_ALLOW,
			0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);

		return (0);
	}

	if (result.code) {
		wcd_error (t, result);
		return (EIO);
	}

	if (closeit) 
		return (0);


	thread_set_timeout(hz*3);
	thread_block((void (*)(void)) 0);
	reset_timeout_check(&current_thread()->timer);

	/* Unlock. */
	wcd_request_wait (t, ATAPI_PREVENT_ALLOW,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	/* Eject. */
	t->flags |= F_MEDIA_CHANGED;
	return wcd_request_wait (t, ATAPI_START_STOP,
		0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0);
}

#endif /* NWCD && NWDC && ATAPI */
