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
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)wd.c	7.2 (Berkeley) 5/9/91
 *	$Id: wd.c,v 1.1.2.8 1998/04/21 08:50:41 stephen Exp $
 */

/* TODO:
 *	o Bump error count after timeout.
 *	o Satisfy ATA timing in all cases.
 *	o Finish merging berry/sos timeout code (bump error count...).
 *	o Merge/fix TIH/NetBSD bad144 code.
 *	o Don't use polling except for initialization.  Need to
 *	  reorganize the state machine.  Then "extra" interrupts
 *	  shouldn't happen (except maybe one for initialization).
 *	o Fix disklabel, boot and driver inconsistencies with
 *	  bad144 in standard versions.
 *	o Support extended DOS partitions.
 *	o Support swapping to DOS partitions.
 *	o Handle bad sectors, clustering, disklabelling, DOS
 *	  partitions and swapping driver-independently.  Use
 *	  i386/dkbad.c for bad sectors.  Swapping will need new
 *	  driver entries for polled reinit and polled write).
 */

#include "wd.h"
#if     NWD > 0

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
#include <ppc/io_map_entries.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/wdreg.h>
#include <ppc/POWERMAC/wd_entries.h>
#include <ppc/POWERMAC/dbdma.h>
#include <ppc/POWERMAC/atapi.h>
#include <ppc/POWERMAC/powermac_m2.h>

extern void wdstart(int ctrlr);

#define	ATAPI

#define	DEV_BSIZE	512
#define TIMEOUT		1000
#define	RETRIES		5	/* number of retries before giving up */
#define RECOVERYTIME	500000	/* usec for controller to recover after err */
#define	MAXTRANSFER	255	/* max size of transfer in sectors */
				/* correct max is 256 but some controllers */
				/* can't handle that in all cases */
#define WDOPT_32BIT	0x8000
#define WDOPT_SLEEPHACK	0x4000
#define WDOPT_FORCEHD(x)	(((x)&0x0f00)>>8)
#define WDOPT_MULTIMASK	0x00ff

#define	DELAY(x)	delay(x)

#define PARTITION(z)    (minor(z) & 0x0f)
#define UNIT(z)         ((minor(z) >> 4) & 0x03)
#define SEQUNIT(z)	wd_logical_map[UNIT(z)]

#define     howmany(x, y)   (((x)+((y)-1))/(y))

#define	min(x,y)	((x) < (y) ? (x) : (y))

//#define	WDDEBUG 1

/*
 * This biotab field doubles as a field for the physical unit number on
 * the controller.
 */
#define	id_physid id_scsiid

/*
 * Drive states.  Used to initialize drive.
 */

#define	CLOSED		0	/* disk is closed. */
#define	WANTOPEN	1	/* open requested, not started */
#define	RECAL		2	/* doing restore */
#define	DMA_TIMING	3	/* doing DMA timing */
#define	OPEN		4	/* done with open */

#define PRIMARY		0

#define	NIDE		4

unsigned short single_dma_timing[]        = {960, 480, 240};
unsigned short multi_dma_timing[]         = {480, 150, 120};


/*
 * Disk geometry.  A small part of struct disklabel.
 * XXX disklabel.5 contains an old clone of disklabel.h.
 */
struct diskgeom {
	u_long	d_secsize;		/* # of bytes per sector */
	u_long	d_nsectors;		/* # of data sectors per track */
	u_long	d_ntracks;		/* # of tracks per cylinder */
	u_long	d_ncylinders;		/* # of data cylinders per unit */
	u_long	d_secpercyl;		/* # of data sectors per cylinder */
	u_long	d_secperunit;		/* # of data sectors per unit */
	u_long	d_precompcyl;		/* XXX always 0 */
};

struct io_segment {
	vm_offset_t	physaddr[2];	/* Assumes DEV_BSIZE < PAGE_SIZE */
	vm_offset_t	virtaddr;
};

/*
 * The structure of a disk drive.
 */
struct disk {
	long	dk_bc;		/* byte count left */
	short	dk_skip;	/* blocks already transferred */
	int	dk_ctrlr;	/* physical controller number */
#ifdef CMD640
	int	dk_ctrlr_cmd640;/* controller number for CMD640 quirk */
#endif
	char	dk_state;	/* control state */
	u_char	dk_status;	/* copy of status reg. */
	u_char	dk_error;	/* copy of error reg. */
	u_char	dk_timeout;	/* countdown to next timeout */

#ifdef	DEVFS
	void	*dk_bdev;	/* devfs token for whole disk */
	void	*dk_cdev;	/* devfs token for raw whole disk */
#endif
	short	dk_unit;	
	short	dk_lunit;	
	u_long	cfg_flags;	/* configured characteristics */
	short	dk_opens;
	u_short	dk_flags;	/* drive characteristics found */
#define	DKFL_SINGLE	0x00004	/* sector at a time mode */
#define	DKFL_ERROR	0x00008	/* processing a disk error */
#define	DKFL_LABELLING	0x00080	/* readdisklabel() in progress */
#define	DKFL_32BIT	0x00100	/* use 32-bit i/o mode */
#define	DKFL_MULTI	0x00200	/* use multi-i/o mode */
#define	DKFL_BADSCAN	0x00400	/* report all errors */
#define DKFL_CAN_DMA	0x00800	/* can use DMA */
#define DKFL_USING_DMA	0x01000	/* Using DMA */
#define	DKFL_DONE_DMA	0x02000	/* DMA completed.. */

	struct wdparams dk_params; /* ESDI/IDE drive/controller parameters */
	int	dk_dkunit;	/* disk stats unit number */
	int	dk_multi;	/* multi transfers */
	int	dk_currentiosize;	/* current io size */
	int	dk_dma_supported;

	unsigned long	dk_port_wd_data;
	unsigned long	dk_port_wd_error;
	unsigned long	dk_port_wd_precomp; 
	unsigned long	dk_port_wd_features;
	unsigned long	dk_port_wd_seccnt;
	unsigned long	dk_port_wd_sector;
	unsigned long	dk_port_wd_cyl_lo;
	unsigned long	dk_port_wd_cyl_hi;
	unsigned long	dk_port_wd_sdh;
	unsigned long	dk_port_wd_command;
	unsigned long	dk_port_wd_status;
	unsigned long	dk_port_wd_altsts;
	unsigned long	dk_port_wd_ctlr;

#define	DMA_NO_SUPPORT	0
#define	DMA_SINGLE_MODE	1
#define	DMA_MULTIPLE_MODE	2


	dbdma_command_t		*dk_dma_cmds;
	dbdma_regmap_t		*dk_dma_chan;
	int			dk_dma_max_segments;
	struct disklabel	dk_dd;	/* device configuration data */
	int			dk_segcount;
	struct io_segment	dk_segments[MAXTRANSFER*2];
};

#define WD_COUNT_RETRIES
static int wdtest = 0;

static struct disk	*wddrives[NIDE];	/* table of units */
static struct io_req	*drive_queue[NIDE];	/* head of queue per drive */
static struct {
	int	b_errcnt;
	int	b_active;
} wdutab[NIDE];

static	vm_offset_t	wdaddrs[NWD];

/*
static struct buf wdtab[NWD];
*/
static struct {
#if CHAINED_IOS
	struct io_req	*chain;
#endif
	queue_head_t	controller_queue;
	int		b_errcnt;
	int		b_active;
} wdtab[NWD];

static int	wd_logical_map[NWD];

#define	TAILQ_INIT(x)	simple_lock_init((&x)->io_req_lock, ETAP_IO_REQ)

/* Forward */

extern int		wdprobe(
				int			port,
				struct bus_ctlr		* ctlr);
extern int		wdslave(
				struct bus_device	* bd,
				caddr_t			xxx);
extern void		wdattach(
				struct bus_device	*dev);
extern void wdstrategy(register struct buf *bp);
extern void		wdminphys(
				struct buf		* bp);
void wd_sync_request(io_req_t ior);
void	wd_build_dma(struct disk *drive, io_req_t ior, long count);
void	wd_build_segments(struct disk *drive, io_req_t ior);
static boolean_t	getvtoc(dev_t dev);
static boolean_t	getvtoc(dev_t dev);
void			wdintr(int unit);
io_return_t wd_rw_abs(dev_t, dev_status_t, int, int, int);
static void wdustart(struct disk *du);
static int wdcontrol(struct buf *bp);
static int wdcommand(struct disk *du, u_int cylinder, u_int head,
		     u_int sector, u_int count, u_int command);
static int wdsetctlr(struct disk *du);
#if 0
static int wdwsetctlr(struct disk *du);
#endif
static int wdgetctlr(struct disk *du);
static void wderror(struct buf *bp, struct disk *du, char *mesg);
static void wdflushirq(struct disk *du, int old_ipl);
static int wdreset(struct disk *du);
static void wdsleep(int ctrlr, char *wmesg);
static void	wdtimeout(void *param);
static int wdunwedge(struct disk *du);
static int wdwait(struct disk *du, u_char bits_wanted, int timeout);
void	wdintr_0(int unit, void *ssp);
void	wdintr_1(int unit, void *ssp);


void	(*wd_ints[2])(int, void *sp) = {
	wdintr_0, wdintr_1
};

extern io_return_t
read_ata_mac_label(
               dev_t            dev,
               int              sector,
               int              ext_base,
               int              *part_name,
               struct disklabel *label,
               io_return_t      (*rw_abs)(
                                          dev_t         dev,
                                          dev_status_t  data,
                                          int           rw,
                                          int           sec,
                                          int           count));

#ifdef CMD640
static int      atapictrlr;
static int      eide_quirks;
#endif

device_node_t	*ata_devices[NWD] = {NULL};

#define	TAIL_QUEUE(x)	simple_lock_init(&(x)->io_req_lock, ETAP_IO_REQ)

caddr_t wd_std[NWD] = { 0 };
struct  bus_device	*wd_dinfo[NWD*NIDE];
struct  bus_ctlr	*wd_minfo[NWD];
struct  bus_driver      wd_driver = {
	(probe_t)wdprobe, wdslave, wdattach, 0, wd_std, "hd", wd_dinfo,
       "ide", wd_minfo, 0};

/*
 * Cycle timing values..
 */



static void __inline__
outb(unsigned long addr, volatile unsigned char value)
{
	*(volatile unsigned char *) addr  = value;
	eieio();
}

static volatile unsigned char __inline__
inb(unsigned long addr)
{
	volatile unsigned char value;

	value = *(volatile unsigned char *) addr;
	eieio();

	return value;
}

static void volatile __inline__
outw(unsigned long addr, volatile unsigned short value)
{
	*(volatile unsigned short *) addr = value; eieio();

}

static volatile unsigned short __inline__
inw_le(unsigned long addr)
{
	volatile unsigned short value;

	asm volatile("lhbrx %0,0,%1" : "=r" (value) : "r" (addr)); eieio();

	return value;
}

static volatile unsigned long __inline__
inl_le(unsigned long addr)
{
	volatile unsigned long value;

	asm volatile("lwbrx %0,0,%1" : "=r" (value) : "r" (addr)); eieio();

	return value;
}


static volatile unsigned short __inline__
inw(unsigned long addr)
{
	volatile unsigned short value;

	value = *(volatile unsigned short *) addr; eieio();

	return value;
}

static void __inline__
outsw(unsigned long addr, void *_value, long count)
{
	volatile unsigned short *value = _value;

	while (count-- > 0)
		outw(addr, *value++);
}

static void __inline__
insw_le(unsigned long addr, void *_value, long count)
{
	volatile unsigned short *value = _value;

	while (count-- > 0)
		*value++ = inw_le(addr);
}

static void __inline
insw(unsigned long addr, void *_value, long count)
{
	volatile unsigned short *value = _value;

	while (count-- > 0)
		*value++ = inw(addr);
}

static void __inline__
outsl(unsigned long addr, void *_value, long count)
{
	volatile unsigned long *value = _value;

	while (count-- > 0) {
		*(volatile unsigned long  *) addr = *value++; eieio();
	}
}

static void __inline__
insl_le(unsigned long addr, void *_value, long count)
{
	volatile unsigned long *value = _value;

	while (count-- > 0) {
		asm volatile("lwbrx %0,0,%1" : "=r" (*value) : "r" (addr));
		eieio();
		value++;
	}
}

static void __inline__
insl(unsigned long addr, void *_value, long count)
{
	volatile unsigned long *value = _value;

	while (count-- > 0) {
		*value++ = *(volatile unsigned long *)addr;
		eieio();
	}
}

/*
 *  Here we use the pci-subsystem to find out, whether there is
 *  a cmd640b-chip attached on this pci-bus. This public routine
 *  will be called by wdc_p.c .
 */

#ifdef CMD640
void
wdc_pci(int quirks)
{
	eide_quirks = quirks;
}
#endif

static void
wd_setup_addrs(struct disk *du)
{
	unsigned long	addr;

	addr = wdaddrs[du->dk_ctrlr];

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_POWERBOOK:
		du->dk_port_wd_data = addr;
		du->dk_port_wd_error = addr + 0x4;
		du->dk_port_wd_precomp = addr + 0x4; 
		du->dk_port_wd_features = addr + 0x4 ;
		du->dk_port_wd_seccnt = addr + 0x8;
		du->dk_port_wd_sector = addr + 0xc;
		du->dk_port_wd_cyl_lo = addr + 0x10;
		du->dk_port_wd_cyl_hi = addr + 0x14;
		du->dk_port_wd_sdh = addr + 0x18;
		du->dk_port_wd_command = addr + 0x1c;
		du->dk_port_wd_status = addr + 0x1c;
		du->dk_port_wd_altsts = addr + 0x38;
		du->dk_port_wd_ctlr = addr + 0x38;
		break;

	default:
		du->dk_port_wd_data = addr + wd_data;
		du->dk_port_wd_error = addr + wd_error;
		du->dk_port_wd_precomp = addr + wd_precomp;
		du->dk_port_wd_features = addr + wd_features;
		du->dk_port_wd_seccnt = addr + wd_seccnt;
		du->dk_port_wd_sector = addr + wd_sector;
		du->dk_port_wd_cyl_lo = addr + wd_cyl_lo;
		du->dk_port_wd_cyl_hi = addr + wd_cyl_hi;
		du->dk_port_wd_sdh = addr + wd_sdh;
		du->dk_port_wd_command = addr + wd_command;
		du->dk_port_wd_status = addr + wd_status;
		du->dk_port_wd_altsts = addr + wd_altsts;
		du->dk_port_wd_ctlr = addr + wd_ctlr;
		break;
	}
}

/*
 * Probe for controller.
 */

int
wdprobe(int port, struct bus_ctlr *bus)
{
	int	unit = bus->unit, i, dev;
	unsigned long addr;
	struct disk *du;
	device_node_t	*atadev, *devlist = NULL, *next,
			**devs, *mediadevs = NULL, **media;
	static char	*ata_names[] = { "ata", "ATA", "ide", NULL };
	int		intr;

	if (unit >= NWD) 
		return (0);

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_POWERBOOK:
		if (unit)
			return 0;

		addr = M2_IDE0_BASE;
		break;

	default:
		if (unit == 0) {
			/*
			 * Build up a list of OFW devices. 
			 */
			media = &mediadevs;
			devs = &devlist;
	
			for (dev = 0, i = 0; ata_names[i]; i++) {
				atadev = find_devices(ata_names[i]);
			
				for (; atadev; atadev = next) {
					next = atadev->next;
	
					/*
					 * Keep the device list somewhat sorted.
					 * For pb3400's the CDROM media-bay shows up
					 * first, but consider it last..
					 */
	
					if (strcmp(atadev->parent->name, "media-bay") == 0) {
						*media = atadev;
						media = &atadev->next;
					} else {
						*devs = atadev;
						devs = &atadev->next;
					}
					atadev->next = NULL;
				}
					
			}
	
			*devs = mediadevs;
	
			for (atadev = devlist; dev < NWD && atadev; dev++) {
				ata_devices[dev] = atadev;
				atadev = atadev->next;
			}
		}
		if ((atadev = ata_devices[unit]) == NULL) 
			return 0;

		addr = atadev->addrs[0].address;
		break;
	
	}



	if (kmem_alloc(kernel_map, (vm_offset_t *)&du, sizeof *du) != KERN_SUCCESS)
		return 0;
;
	bzero((void *)du, sizeof *du);
	du->dk_ctrlr = unit;
	du->dk_unit = 0;
	wdaddrs[unit] = io_map(addr, 4096);

	wd_setup_addrs(du);

	/* check if we have registers that work */
	outb(du->dk_port_wd_sdh, WDSD_IBM);   /* set unit 0 */
	outb(du->dk_port_wd_cyl_lo, 0xa5);	/* wd_cyl_lo is read/write */
	if (inb(du->dk_port_wd_cyl_lo) == 0xff) {     /* XXX too weak */
#ifdef ATAPI
		/* There is no master, try the ATAPI slave. */
		outb(du->dk_port_wd_sdh, WDSD_IBM | 0x10);
		outb(du->dk_port_wd_cyl_lo, 0xa5);
		if (inb(du->dk_port_wd_cyl_lo) == 0xff)
#endif
		{
			goto nodevice;
		}
	}

	if (wdreset(du) == 0) 
		goto reset_ok;

#ifdef ATAPI
	/* test for ATAPI signature */
	outb(du->dk_port_wd_sdh, WDSD_IBM);           /* master */
	if (inb(du->dk_port_wd_cyl_lo) == 0x14 &&
	    inb(du->dk_port_wd_cyl_hi) == 0xeb) {
		goto reset_ok;
	}

	du->dk_unit = 1;
	outb(du->dk_port_wd_sdh, WDSD_IBM | 0x10); /* slave */
	if (inb(du->dk_port_wd_cyl_lo) == 0x14 &&
	    inb(du->dk_port_wd_cyl_hi) == 0xeb) {
		goto reset_ok;
	}
#endif

	DELAY(RECOVERYTIME);
	if (wdreset(du) != 0) 
		goto nodevice;

reset_ok:

	/* execute a controller only command */
	if (wdcommand(du, 0, 0, 0, 0, WDCC_DIAGNOSE) != 0
	    || wdwait(du, 0, TIMEOUT) < 0) {
		printf("Diagnose failed\n");
		goto nodevice;
	}

	/*
	 * drive(s) did not time out during diagnostic :
	 * Get error status and check that both drives are OK.
	 * Table 9-2 of ATA specs suggests that we must check for
	 * a value of 0x01
	 *
	 * Strangely, some controllers will return a status of
	 * 0x81 (drive 0 OK, drive 1 failure), and then when
	 * the DRV bit is set, return status of 0x01 (OK) for
	 * drive 2.  (This seems to contradict the ATA spec.)
	 */
	du->dk_error = inb(du->dk_port_wd_error);
	/*printf("Error : %x\n", du->dk_error); */
	if(du->dk_error != 0x01) {
		if(du->dk_error & 0x80) { /* drive 1 failure */

			/* first set the DRV bit */
			u_int sdh;
			sdh = inb(du->dk_port_wd_sdh);
			sdh = sdh | 0x10;
			outb(du->dk_port_wd_sdh, sdh);

			/* Wait, to make sure drv 1 has completed diags */
			if ( wdwait(du, 0, TIMEOUT) < 0)
				goto nodevice;

			/* Get status for drive 1 */
			du->dk_error = inb(du->dk_port_wd_error);
			printf("Error (drv 1) : %x\n", du->dk_error); 
			/*
			 * Sometimes (apparently mostly with ATAPI
			 * drives involved) 0x81 really means 0x81
			 * (drive 0 OK, drive 1 failed).
			 */
			if(du->dk_error != 0x01 && du->dk_error != 0x81)
				goto nodevice;
		} else	/* drive 0 fail */
			goto nodevice;
	}


	wdreset(du);	/* Try to unwedge the diagnostic */

	/* lion@apple.com 1/15/97 -  *ARGH!* On the 6400's 
	 * OpenFirmware DOES NOT report ANY interrupt numbers..Fudge it.
	 */
	if (powermac_info.class == POWERMAC_CLASS_POWERBOOK) {
		pmac_register_int(PMAC_DEV_ATA0, SPLBIO, wd_ints[unit]);
	} else {
		intr = ata_devices[unit]->n_intrs ? ata_devices[unit]->intrs[0] : 13;

		pmac_register_ofint(intr, SPLBIO, wd_ints[unit]);
	}

	kmem_free(kernel_map, (vm_offset_t) du, sizeof(*du));
	return 1;

nodevice:
	kmem_free(kernel_map, (vm_offset_t) du, sizeof(*du));
	return (0);
}

/*
 * Attach each drive if possible.
 */
void 
wdattach(struct bus_device *dev)
{
	int	unit = dev->unit, tmpunit;
	int	ctrl = dev->unit / 2;
	struct	disk *du = wddrives[dev->unit];
	char	devname[10];
        dev_ops_t       wd_ops;



	/*
	 * Print out description of drive.
	 * wdp_model may not be null terminated.
	 */
	printf("(logical hd%d) <%.*s>", du->dk_dkunit,
			sizeof du->dk_params.wdp_model,
			du->dk_params.wdp_model);
	if (du->dk_flags & DKFL_32BIT)
		printf(", 32-bit");
	if (du->dk_multi > 1)
		printf(", multi-block %d", du->dk_multi);
	if (du->cfg_flags & WDOPT_SLEEPHACK)
		printf(", sleep-hack");
	printf("\n");

	if (du->dk_params.wdp_heads == 0)
		printf("wd%d: size unknown, using %s values\n",
		       du->dk_lunit, du->dk_dd.d_secperunit > 17
			      ? "BIOS" : "fake");

	printf( "  %luMB (%lu sectors), %lu cyls, %lu heads, %lu S/T, %lu B/S",
	       du->dk_dd.d_secperunit/((1024L * 1024L)/du->dk_dd.d_secsize),
		du->dk_dd.d_secperunit,
		du->dk_dd.d_ncylinders,
		du->dk_dd.d_ntracks,
		du->dk_dd.d_nsectors,
		du->dk_dd.d_secsize);


	strcpy(devname, "hd");
	itoa(unit, &devname[2]);

	if (dev_name_lookup("ide", &wd_ops, &tmpunit)) {
		dev_set_indirection(devname, wd_ops,
				dev->unit * wd_ops->d_subdev);
	} 

	/*
	 * Start timeout routine for this drive.
	 * XXX timeout should be per controller.
	 */
	wdtimeout(du);

	return;
}


int
wdslave(struct bus_device * bd, caddr_t  xxx)
{
	int	lunit = bd->unit, unit, i;
	struct disk *du;
	int	tmpunit;
	dev_ops_t	ide_dev_ops;
	static int	dk_ndrive =0;
	char	devname[8];

	if (lunit >= NWD)
		return 0;

	unit = bd->ctlr;

#ifdef CMD640
	if (eide_quirks & Q_CMD640B) {
		if (unit == PRIMARY) {
			printf("wdc0: CMD640B workaround enabled\n");
			TAILQ_INIT( &wdtab[PRIMARY].controller_queue);
		}
	} else
		TAILQ_INIT( &wdtab[dvp->id_unit].controller_queue);

#else
	queue_init(&wdtab[unit].controller_queue);
#endif

	if (kmem_alloc(kernel_map, (vm_offset_t*) &du, sizeof(*du)) != KERN_SUCCESS)
		return 0 ;

	if (wddrives[lunit] != NULL)
		panic("drive attached twice");

	wddrives[lunit] = du;
	bzero((void *)du, sizeof *du);
	du->dk_ctrlr = unit;

#ifdef CMD640
	if (eide_quirks & Q_CMD640B) {
		du->dk_ctrlr_cmd640 = PRIMARY;
	} else {
		du->dk_ctrlr_cmd640 = du->dk_ctrlr;
	}
#endif
	du->dk_unit = bd->slave;
	du->dk_lunit = lunit;
	wd_setup_addrs(du);

	wdtab[unit].b_active = 0;

	if (wdgetctlr(du)) {
		/* See if ATAPI can attach to it.. */
		if (powermac_info.class != POWERMAC_CLASS_POWERBOOK
		&& atapi_attach(du->dk_ctrlr, du->dk_unit, wdaddrs[du->dk_ctrlr]))
			wdtab[unit].b_active = 2;

		kmem_free(kernel_map, (vm_offset_t) du, sizeof(*du));
		wddrives[lunit] = NULL;
		return 0;
	}

	wdtab[unit].b_active = 2;

#ifdef DKFL_CAN_DMA
	if (powermac_info.class != POWERMAC_CLASS_POWERBOOK 
	&&  ata_devices[unit]->n_addrs > 1) {
		du->dk_dma_chan = (dbdma_regmap_t *) io_map(ata_devices[unit]->addrs[1].address, 4096);
		du->dk_dma_cmds = dbdma_alloc((du->dk_multi*2)+2);
		du->dk_flags |= DKFL_CAN_DMA;
	}
	du->dk_dma_max_segments = du->dk_multi;
#endif

	du->dk_dkunit = dk_ndrive++;
	wd_logical_map[du->dk_dkunit] = du->dk_lunit;
	return 1;

}



#if CHAINED_IOS

#define IO_OP(op) (op & (IO_READ|IO_WRITE))

/* 
 * can_chain_io_reqs(a, b)
 * Can we chain two IOs ?
 * Check that:
 *	OPs (Read, write) are identical,
 *	Record numbers are consecutive,
 *	SECLIMIT is not reached.
 */

#define can_chain_io_reqs(a, b, du)					\
  	(IO_OP(a->io_op) == IO_OP(b->io_op) &&				\
	 (a->io_unit == b->io_unit) &&					\
	 (a->io_recnum + ((a->io_count + 511) >> 9) == b->io_recnum ||	\
	  b->io_recnum + ((b->io_count + 511) >> 9) == a->io_recnum) && \
	 ((a->io_count + b->io_count) <= (MAXTRANSFER*DEV_BSIZE)) && \
	 ((a->io_seg_count + b->io_seg_count) <= du->dk_dma_max_segments) )
	 
#endif /* CHAINED_IOS */

/* Read/write routine for a buffer.  Finds the proper unit, range checks
 * arguments, and schedules the transfer.  Does not wait for the transfer
 * to complete.  Multi-page transfers are supported.  All I/O requests must
 * be a multiple of a sector in length.
 */
void
wdstrategy(register struct buf *bp)
{
	struct disk *du;
	int	lunit = UNIT(bp->b_dev);
	int	s;
	struct label_partition *part;
	struct io_req	**queue;

	/* valid unit, controller, and request?  */
	if (lunit >= NIDE || bp->b_blkno < 0 || (du = wddrives[lunit]) == NULL
	    || bp->b_bcount % DEV_BSIZE != 0) {

		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		printf("strategy.. rejecting.. unit %d, blkno %d, %x, bcount %d", lunit, bp->b_blkno, wddrives[lunit], bp->b_bcount);
		goto done;
	}

#if 0
	/*
	 * Do bounds checking, adjust transfer, and set b_pblkno.
	 */
	if (dscheck(bp, du->dk_slices) <= 0)
		goto done;
#endif

	wd_sync_request(bp);

	if (bp->b_flags & B_MD1)
		part = &du->dk_dd.d_partitions[MAXPARTITIONS];
	else
		part = &du->dk_dd.d_partitions[PARTITION(bp->b_dev)];

	if (part->p_size <= 0) {
		printf("%d partition is ZERO sized (%x) ", PARTITION(bp->b_dev), bp->b_flags);
		bp->b_error = ENXIO;
		bp->b_flags |= IO_ERROR;
		bp->io_residual = bp->io_count;
		goto done;
	}

	if (bp->b_blkno >= part->p_size) {
		bp->b_error = D_INVALID_RECNUM;
		bp->io_residual = bp->io_count;
		bp->b_flags |= IO_ERROR;
		goto done;
	}

#if 0
	/*
	 * Check for *any* block on this transfer being on the bad block list
	 * if it is, then flag the block as a transfer that requires
	 * bad block handling.  Also, used as a hint for low level disksort
	 * clustering code to keep from coalescing a bad transfer into
	 * a normal transfer.  Single block transfers for a large number of
	 * blocks associated with a cluster I/O are undesirable.
	 *
	 * XXX the old disksort() doesn't look at B_BAD.  Coalescing _is_
	 * desirable.  We should split the results at bad blocks just
	 * like we should split them at MAXTRANSFER boundaries.
	 */
	if (dsgetbad(bp->b_dev, du->dk_slices) != NULL) {
		long *badsect = dsgetbad(bp->b_dev, du->dk_slices)->bi_bad;
		int i;
		int nsecs = howmany(bp->b_bcount, DEV_BSIZE);
		/* XXX pblkno is too physical. */
		daddr_t nspblkno = bp->b_pblkno
		    - du->dk_slices->dss_slices[dkslice(bp->b_dev)].ds_offset;
		int blkend = nspblkno + nsecs;

		for (i = 0; badsect[i] != -1 && badsect[i] < blkend; i++) {
			if (badsect[i] >= nspblkno) {
				bp->b_flags |= B_BAD;
				break;
			}
		}
	}
#endif

	/* queue transfer on drive, activate drive and controller if idle */
	s = splbio();

	bp->io_next = bp->io_prev = NULL;

	queue = &drive_queue[lunit];


	if (*queue == NULL) {
		*queue = bp;

	} else {
		disksort(*queue, bp);

#if CHAINED_IOS
		if (!(bp->io_op & IO_SGLIST) && (du->dk_flags & DKFL_CAN_DMA)) {
		  	bp->io_seg_count = 1;
			if (bp->io_prev && (bp->io_prev != *queue ||
			   !wdtab[du->dk_ctrlr].b_active) &&
			   can_chain_io_reqs(bp->io_prev, bp, du)) {
				chain_io_reqs(bp->io_prev, bp, *queue);
			}
			if (bp->io_next && can_chain_io_reqs(bp, bp->io_next, du)) {
				chain_io_reqs(bp, bp->io_next, *queue);
			}
		}
#endif
	}

	if (wdutab[lunit].b_active == 0)
		wdustart(du);	/* start drive */

#if 0
	/* Pick up changes made by readdisklabel(). */
	if (du->dk_flags & DKFL_LABELLING && du->dk_state > RECAL) {
		wdsleep(du->dk_ctrlr, "wdlab");
		du->dk_state = WANTOPEN;
	}
#endif

#ifdef CMD640
	if (wdtab[du->dk_ctrlr_cmd640].b_active == 0)
#else
	if (wdtab[du->dk_ctrlr].b_active == 0)
#endif
		wdstart(du->dk_ctrlr);	/* start controller */

#if 0
	if (du->dk_dkunit >= 0) {
		/*
		 * XXX perhaps we should only count successful transfers.
		 */
		dk_xfer[du->dk_dkunit]++;
		/*
		 * XXX we can't count seeks correctly but we can do better
		 * than this.  E.g., assume that the geometry is correct
		 * and count 1 seek if the starting cylinder of this i/o
		 * differs from the starting cylinder of the previous i/o,
		 * or count 1 seek if the starting bn of this i/o doesn't
		 * immediately follow the ending bn of the previos i/o.
		 */
		dk_seek[du->dk_dkunit]++;
	}
#endif

	splx(s);
	return;

done:
	s = splbio();
#if CHAINED_IOS
        if (bp->io_op & IO_CHAINED) {
                chained_iodone(bp);
        }
        else    
#endif  /* CHAINED_IOS */
	/* toss transfer, we're done early */
	biodone(bp);
	splx(s);
}

/*
 * Routine to queue a command to the controller.  The unit's
 * request is linked into the active list for the controller.
 * If the controller is idle, the transfer is started.
 */
static void
wdustart(register struct disk *du)
{
	register struct buf *bp;
#ifdef CMD640
	int	ctrlr = du->dk_ctrlr_cmd640;
#else
	int	ctrlr = du->dk_ctrlr;
#endif

	/* unit already active? */
	if (wdutab[du->dk_lunit].b_active)
		return;

	if (drive_queue[du->dk_lunit] == NULL)
		return;

	bp = (struct buf *) drive_queue[du->dk_lunit];
	drive_queue[du->dk_lunit] = bp->io_next;

	/* link onto controller queue */
	enqueue_tail(&wdtab[ctrlr].controller_queue, (queue_entry_t) bp);

	/* mark the drive unit as busy */
	wdutab[du->dk_lunit].b_active = 1;
}

/*
 * Controller startup routine.  This does the calculation, and starts
 * a single-sector read or write operation.  Called to start a transfer,
 * or from the interrupt routine to continue a multi-sector transfer.
 * RESTRICTIONS:
 * 1. The transfer length must be an exact multiple of the sector size.
 */

void
wdstart(int ctrlr)
{
	register struct disk *du;
	register struct buf *bp;
	struct disklabel	*lp;	/* XXX sic */
	long	blknum;
	long	secpertrk, secpercyl, offset;
	int	lunit;
	int	count;
#ifdef CMD640
	int	ctrlr_atapi;

	if (eide_quirks & Q_CMD640B) {
		ctrlr = PRIMARY;
		ctrlr_atapi = atapictrlr;
	} else {
		ctrlr_atapi = ctrlr;
	}
#endif

#ifdef ATAPI
	if (wdtab[ctrlr].b_active == 2)
		wdtab[ctrlr].b_active = 0;
	if (wdtab[ctrlr].b_active)
		return;
#endif
	/* is there a drive for the controller to do a transfer with? */
	if (queue_empty(&wdtab[ctrlr].controller_queue)) {
#ifdef ATAPI
#ifdef CMD640
		if (atapi_start && atapi_start (ctrlr_atapi))
			wdtab[ctrlr].b_active = 3;
#else
		if (atapi_start && atapi_start (ctrlr))
			/* mark controller active in ATAPI mode */
			wdtab[ctrlr].b_active = 3;
#endif
#endif
		return;
	}

	bp = (struct buf *) queue_first(&wdtab[ctrlr].controller_queue);

	/* obtain controller and drive information */
	lunit = UNIT(bp->b_dev);
	du = wddrives[lunit];

#ifdef DKFL_CAN_DMA
	du->dk_flags &= ~(DKFL_USING_DMA|DKFL_DONE_DMA);/* Safety check.. */
#endif

	/* if not really a transfer, do control operations specially */
	if (du->dk_state < OPEN) {
		if (du->dk_state != WANTOPEN)
			printf("wd%d: wdstart: weird dk_state %d\n",
			       du->dk_lunit, du->dk_state);
		if (wdcontrol(bp) != 0)
			printf("wd%d: wdstart: wdcontrol returned nonzero, state = %d\n",
			       du->dk_lunit, du->dk_state);
		return;
	}

	/* calculate transfer details */
	if (bp->b_flags & B_MD1)
		offset = du->dk_dd.d_partitions[MAXPARTITIONS].p_offset;
	else
		offset = du->dk_dd.d_partitions[PARTITION(bp->b_dev)].p_offset;

	blknum = bp->b_blkno + offset + du->dk_skip;

	//printf("{u%d,p%d, %d}", UNIT(bp->b_dev), PARTITION(bp->b_dev), blknum);

#ifdef WDDEBUG
	if (du->dk_skip == 0)
		printf("wd%d: wdstart: %s %d@%d; map ", lunit,
		       (bp->b_flags & B_READ) ? "read" : "write",
		       bp->b_bcount, blknum);
	else
		printf(" %d)%x", du->dk_skip, inb(du->dk_port_wd_altsts));
#endif

	lp = &du->dk_dd;
	secpertrk = lp->d_nsectors;
	secpercyl = lp->d_secpercyl;

	if (du->dk_skip == 0) {
		du->dk_bc = bp->b_bcount;

		wd_build_segments(du, bp);

#if 0
		if (bp->b_flags & B_BAD
		    /*
		     * XXX handle large transfers inefficiently instead
		     * of crashing on them.
		     */
		    || howmany(du->dk_bc, DEV_BSIZE) > MAXTRANSFER)
			du->dk_flags |= DKFL_SINGLE;
#endif
	}


#if 0
	if (du->dk_flags & DKFL_SINGLE
	    && dsgetbad(bp->b_dev, du->dk_slices) != NULL) {
		/* XXX */
		u_long ds_offset =
		    du->dk_slices->dss_slices[dkslice(bp->b_dev)].ds_offset;

		blknum = transbad144(dsgetbad(bp->b_dev, du->dk_slices),
				     blknum - ds_offset) + ds_offset;
	}
#endif

	wdtab[ctrlr].b_active = 1;	/* mark controller active */

	/* if starting a multisector transfer, or doing single transfers */
	if (du->dk_skip == 0 || (du->dk_flags & DKFL_SINGLE)
#ifdef	DKFL_CAN_DMA
	||	(du->dk_flags & DKFL_CAN_DMA)
#endif
) {
		u_int	command;
		u_int	count;
		long	cylin, head, sector;

		cylin = blknum / secpercyl;
		head = (blknum % secpercyl) / secpertrk;
		sector = blknum % secpertrk;

		if (wdtab[ctrlr].b_errcnt && (bp->b_flags & B_READ) == 0)
			du->dk_bc += DEV_BSIZE;
		count = howmany( du->dk_bc, DEV_BSIZE);

		du->dk_flags &= ~DKFL_MULTI;

#ifdef B_FORMAT
		if (bp->b_flags & B_FORMAT) {
			command = WDCC_FORMAT;
			count = lp->d_nsectors;
			sector = lp->d_gap3 - 1;	/* + 1 later */
		} else
#endif

		{
#ifdef	DKFL_CAN_DMA
			if (du->dk_flags & DKFL_CAN_DMA) {
				command = bp->b_flags & B_READ ? WDCC_READ_MULTI_DMA : WDCC_WRITE_MULTI_DMA;
				if (count > du->dk_multi) 
					count = du->dk_multi;

				if (du->dk_flags & DKFL_SINGLE)
					count = 1;

				du->dk_currentiosize = count;

				wd_build_dma(du, bp, count);

				dbdma_start(du->dk_dma_chan, du->dk_dma_cmds);
				du->dk_flags |= DKFL_USING_DMA;
			} else
#endif
			if (du->dk_flags & DKFL_SINGLE) {
				command = (bp->b_flags & B_READ)
					  ? WDCC_READ : WDCC_WRITE;
				count = 1;
				du->dk_currentiosize = 1;
			} else {
				if( (count > 1) && (du->dk_multi > 1)) {
					du->dk_flags |= DKFL_MULTI;
					if( bp->b_flags & B_READ) {
						command = WDCC_READ_MULTI;
					} else {
						command = WDCC_WRITE_MULTI;
					}
					du->dk_currentiosize = du->dk_multi;
					if( du->dk_currentiosize > count)
						du->dk_currentiosize = count;
				} else {
					if( bp->b_flags & B_READ) {
						command = WDCC_READ;
					} else {
						command = WDCC_WRITE;
					}
					du->dk_currentiosize = 1;
				}
			}
		}

		/*
		 * XXX this loop may never terminate.  The code to handle
		 * counting down of retries and eventually failing the i/o
		 * is in wdintr() and we can't get there from here.
		 */
		if (wdtest != 0) {
			if (--wdtest == 0) {
				wdtest = 100;
				printf("dummy wdunwedge\n");
				wdunwedge(du);
			}
		}
#if 0
		if(du->dk_dkunit >= 0) {
			dk_busy |= 1 << du->dk_dkunit;
		}
#endif
		while (wdcommand(du, cylin, head, sector, count, command)
		       != 0) {
			wderror(bp, du,
				"wdstart: timeout waiting to give command");
			wdunwedge(du);
		}

#ifdef WDDEBUG
		printf("cylin %ld head %ld sector %ld addr %x sts %x\n",
		       cylin, head, sector,
		       (int)bp->b_un.b_addr + du->dk_skip * DEV_BSIZE,
		       inb(du->dk_port_wd_altsts));
#endif
	}

	/*
	 * Schedule wdtimeout() to wake up after a few seconds.  Retrying
	 * unmarked bad blocks can take 3 seconds!  Then it is not good that
	 * we retry 5 times.
	 *
	 * XXX wdtimeout() doesn't increment the error count so we may loop
	 * forever.  More seriously, the loop isn't forever but causes a
	 * crash.
	 *
	 * TODO fix b_resid bug elsewhere (fd.c....).  Fix short but positive
	 * counts being discarded after there is an error (in physio I
	 * think).  Discarding them would be OK if the (special) file offset
	 * was not advanced.
	 */
	du->dk_timeout = 1 + 3;

#ifdef DKFL_CAN_DMA
	if (du->dk_flags & DKFL_USING_DMA) {
		return;
	}
#endif

	/* If this is a read operation, just go away until it's done. */
	if (bp->b_flags & B_READ)
		return;

	/* Ready to send data? */
	if (wdwait(du, WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ, TIMEOUT) < 0) {
		wderror(bp, du, "wdstart: timeout waiting for DRQ");
		/*
		 * XXX what do we do now?  If we've just issued the command,
		 * then we can treat this failure the same as a command
		 * failure.  But if we are continuing a multi-sector write,
		 * the command was issued ages ago, so we can't simply
		 * restart it.
		 *
		 * XXX we waste a lot of time unnecessarily translating block
		 * numbers to cylin/head/sector for continued i/o's.
		 */
	}

	count = 1;
	if( du->dk_flags & DKFL_MULTI) {
		count = howmany(du->dk_bc, DEV_BSIZE);
		if( count > du->dk_multi)
			count = du->dk_multi;
		if( du->dk_currentiosize > count)
			du->dk_currentiosize = count;
	}

	if (du->dk_flags & DKFL_32BIT)
		outsl(du->dk_port_wd_data,
		      (void *)(du->dk_segments[du->dk_skip].virtaddr),
		      (count * DEV_BSIZE) / sizeof(long));
	else
		outsw(du->dk_port_wd_data,
		      (void *)(du->dk_segments[du->dk_skip].virtaddr),
		      (count * DEV_BSIZE) / sizeof(short));
	du->dk_bc -= DEV_BSIZE * count;
#if 0
	if (du->dk_dkunit >= 0) {
		/*
		 * `wd's are blocks of 32 16-bit `word's according to
		 * iostat.  dk_wds[] is the one disk i/o statistic that
		 * we can record correctly.
		 * XXX perhaps we shouldn't record words for failed
		 * transfers.
		 */
		dk_wds[du->dk_dkunit] += (count * DEV_BSIZE) >> 6;
	}
#endif
}

void
wdintr_0(int _unit, void *ssp)
{
	wdintr(0);
}

void
wdintr_1(int _unit, void *ssp)
{
	wdintr(1);
}

/* Interrupt routine for the controller.  Acknowledge the interrupt, check for
 * errors on the current operation, mark it done if necessary, and start
 * the next request.  Also check for a partially done transfer, and
 * continue with the next chunk if so.
 */
void
wdintr(int unit)
{
	register struct	disk *du;
	register struct buf *bp;
	boolean_t	used_dma = FALSE;

#ifdef CMD640
	int ctrlr_atapi;

	if (eide_quirks & Q_CMD640B) {
		unit = PRIMARY;
		ctrlr_atapi = atapictrlr;
	} else {
		ctrlr_atapi = unit;
	}
#endif

	if (wdtab[unit].b_active == 2)
		return;		/* intr in wdflushirq() */
	if (!wdtab[unit].b_active) {
#ifdef WDDEBUG
		/*
		 * These happen mostly because the power-mgt part of the
		 * bios shuts us down, and we just manage to see the
		 * interrupt from the "SLEEP" command.
 		 */
		printf("wdc%d: extra interrupt\n", unit);
#endif
		return;
	}
#ifdef ATAPI
	if (wdtab[unit].b_active == 3) {
		/* process an ATAPI interrupt */
#ifdef CMD640
		if (atapi_intr && atapi_intr (ctrlr_atapi))
#else
		if (atapi_intr && atapi_intr (unit))
#endif
			/* ATAPI op continues */
			return;
		/* controller is free, start new op */
		wdtab[unit].b_active = 0;
		wdstart (unit);
		return;
	}
#endif
	bp = (struct buf *) queue_first(&wdtab[unit].controller_queue);
	du = wddrives[UNIT(bp->b_dev)];
	du->dk_timeout = 0;

	if (wdwait(du, 0, TIMEOUT) < 0) {
		wderror(bp, du, "wdintr: timeout waiting for status");
		du->dk_status |= WDCS_ERR;	/* XXX */
	}


#ifdef DKFL_USING_DMA
	if (du->dk_flags & DKFL_USING_DMA) {
		dbdma_stop(du->dk_dma_chan);

		used_dma = TRUE;
		du->dk_flags &= ~DKFL_USING_DMA;
	}
#endif

	/* is it not a transfer, but a control operation? */
	if (du->dk_state < OPEN) {
		wdtab[unit].b_active = 0;
		switch (wdcontrol(bp)) {
		case 0:
			return;
		case 1:
			wdstart(unit);
			return;
		case 2:
			goto done;
		}
	}


	/* have we an error? */
	if (du->dk_status & (WDCS_ERR | WDCS_ECCCOR)) {
oops:
		/*
		 * XXX bogus inb() here, register 0 is assumed and intr status
		 * is reset.
		 */
		if( (du->dk_flags & DKFL_MULTI) && (inb(du->dk_port_wd_status) & WDERR_ABORT)) {
			wderror(bp, du, "reverting to non-multi sector mode");
			du->dk_multi = 1;
		}
#ifdef WDDEBUG
		wderror(bp, du, "wdintr");
#endif
		if ((du->dk_flags & DKFL_SINGLE) == 0) {
			du->dk_flags |= DKFL_ERROR;
			goto outt;
		}
#ifdef B_FORMAT
		if (bp->b_flags & B_FORMAT) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			goto done;
		}
#endif

		if (du->dk_flags & DKFL_BADSCAN) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
		} else if (du->dk_status & WDCS_ERR) {
			if (++wdtab[unit].b_errcnt < RETRIES) {
#if CHAINED_IOS
				if (bp->io_op & IO_CHAINED)
					split_io_reqs(bp);
#endif
				wdtab[unit].b_active = 0;
			} else 
			{
				wderror(bp, du, "hard error");
				bp->b_error = EIO;
				bp->b_flags |= B_ERROR;	/* flag the error */
			}
		} else
			wderror(bp, du, "soft ecc");
	}


#ifdef DKFL_USING_DMA
	if (used_dma) {
		if ((bp->b_flags & B_ERROR) == 0)
			du->dk_bc -= du->dk_currentiosize * DEV_BSIZE;
	} else
#endif

	/*
	 * If this was a successful read operation, fetch the data.
	 */
	if (((bp->b_flags & (B_READ | B_ERROR)) == B_READ)
	    && wdtab[unit].b_active) {
		int	chk, dummy, multisize;
		multisize = chk = du->dk_currentiosize * DEV_BSIZE;
		if( du->dk_bc < chk) {
			chk = du->dk_bc;
			if( ((chk + DEV_BSIZE - 1) / DEV_BSIZE) < du->dk_currentiosize) {
				du->dk_currentiosize = (chk + DEV_BSIZE - 1) / DEV_BSIZE;
				multisize = du->dk_currentiosize * DEV_BSIZE;
			}
		}

		/* ready to receive data? */
		if ((du->dk_status & (WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ))
		    != (WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ))
			wderror(bp, du, "wdintr: read intr arrived early");
		if (wdwait(du, WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ, TIMEOUT) != 0) {
			wderror(bp, du, "wdintr: read error detected late");
			goto oops;
		}

		/* suck in data */
		if( du->dk_flags & DKFL_32BIT)
			insl(du->dk_port_wd_data,
			     (void *)(du->dk_segments[du->dk_skip].virtaddr),
					chk / sizeof(long));
		else
			insw(du->dk_port_wd_data,
			     (void *)(du->dk_segments[du->dk_skip].virtaddr),
					chk / sizeof(short));
		du->dk_bc -= chk;

		/* XXX for obsolete fractional sector reads. */
		while (chk < multisize) {
			insw(du->dk_port_wd_data, &dummy, 1);
			chk += sizeof(short);
		}

#if 0
		if (du->dk_dkunit >= 0)
			dk_wds[du->dk_dkunit] += chk >> 6;
#endif
	}

outt:
	if (wdtab[unit].b_active) {
		if ((bp->b_flags & B_ERROR) == 0) {
			du->dk_skip += du->dk_currentiosize;/* add to successful sectors */
			if (wdtab[unit].b_errcnt)
				wderror(bp, du, "soft error");
			wdtab[unit].b_errcnt = 0;

			/* see if more to transfer */
			if (du->dk_bc > 0 && (du->dk_flags & DKFL_ERROR) == 0) {
				if( (du->dk_flags & (DKFL_SINGLE|DKFL_CAN_DMA))
				|| du->dk_currentiosize == 1
				|| ((bp->b_flags & B_READ) == 0)) {
					wdtab[unit].b_active = 0;
					wdstart(unit);
				} else {
					du->dk_timeout = 1 + 3;
				}
				return;	/* next chunk is started */
			} else if ((du->dk_flags & (DKFL_SINGLE | DKFL_ERROR))
				   == DKFL_ERROR) {
				du->dk_skip = 0;
				du->dk_flags &= ~DKFL_ERROR;
				du->dk_flags |= DKFL_SINGLE;
				wdtab[unit].b_active = 0;
				wdstart(unit);
				return;	/* redo xfer sector by sector */
			}
		}

done: ;
		/* done with this transfer, with or without error */
		du->dk_flags &= ~DKFL_SINGLE;
		bp = (struct buf *) dequeue_head(&wdtab[unit].controller_queue);
		wdtab[unit].b_errcnt = 0;
		bp->b_resid = bp->b_bcount - du->dk_skip * DEV_BSIZE;
		wdutab[du->dk_lunit].b_active = 0;
		wdutab[du->dk_lunit].b_errcnt = 0;
		du->dk_skip = 0;

#if CHAINED_IOS
		if (bp->b_flags & IO_CHAINED) {
			chained_iodone(bp);
		} else
#endif
		biodone(bp);
	}

#if 0
	if(du->dk_dkunit >= 0) {
		dk_busy &= ~(1 << du->dk_dkunit);
	}
#endif

	/* controller idle */
	wdtab[unit].b_active = 0;

	/* anything more on drive queue? */
	wdustart(du);
	/* anything more for controller to do? */
#ifndef ATAPI
	/* This is not valid in ATAPI mode. */
	if (!queue_empty(&wdtab[unit].controller_queue))
#endif
		wdstart(unit);
}

/*
 * Initialize a drive.
 */
int
wdopen(dev_t dev, dev_mode_t flags, io_req_t ior)
{
	register unsigned int lunit;
	register struct disk *du;
	int	error = KERN_SUCCESS;
	spl_t	s;


	lunit = UNIT(dev);
	if (lunit >= NIDE)
		return (ENXIO);
	du = wddrives[lunit];
	if (du == NULL)
		return (ENXIO);

	/* Finish flushing IRQs left over from wdattach(). */
#ifdef CMD640
	if (wdtab[du->dk_ctrlr_cmd640].b_active == 2)
		wdtab[du->dk_ctrlr_cmd640].b_active = 0;
#else
	if (wdtab[du->dk_ctrlr].b_active == 2)
		wdtab[du->dk_ctrlr].b_active = 0;
#endif

	if (du->dk_opens++)
		return KERN_SUCCESS;

	du->dk_flags &= ~DKFL_BADSCAN;

	while (du->dk_flags & DKFL_LABELLING)  
		wdsleep(du->dk_ctrlr, "wdopn");

	du->dk_flags |= DKFL_LABELLING;
	du->dk_state = WANTOPEN;
	if (!getvtoc(dev))
		error = D_NO_SUCH_DEVICE;
	du->dk_flags &= ~DKFL_LABELLING;
	return (error);
}


/*
 * Implement operations other than read/write.
 * Called from wdstart or wdintr during opens and formats.
 * Uses finite-state-machine to track progress of operation in progress.
 * Returns 0 if operation still in progress, 1 if completed, 2 if error.
 */
static int
wdcontrol(register struct buf *bp)
{
	register struct disk *du;
	struct wdparams	*wp;
	u_char	mode;
	int	ctrlr;

	du = wddrives[UNIT(bp->b_dev)];
#ifdef CMD640
	ctrlr = du->dk_ctrlr_cmd640;
#else
	ctrlr = du->dk_ctrlr;
#endif

	switch (du->dk_state) {
	case WANTOPEN:
tryagainrecal:
		wdtab[ctrlr].b_active = 1;
		if (wdcommand(du, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP) != 0) {
			wderror(bp, du, "wdcontrol: wdcommand failed");
			goto maybe_retry;
		}
		du->dk_state = RECAL;
		return (0);
	case RECAL:
		if (du->dk_status & WDCS_ERR || wdsetctlr(du) != 0) {
			wderror(bp, du, "wdcontrol: recal failed");
maybe_retry:
			if (du->dk_status & WDCS_ERR)
				wdunwedge(du);
			du->dk_state = WANTOPEN;
			if (++wdtab[ctrlr].b_errcnt < RETRIES)
				goto tryagainrecal;
			bp->b_error = ENXIO;	/* XXX needs translation */
			bp->b_flags |= B_ERROR;
			return (2);
		}
		wdtab[ctrlr].b_errcnt = 0;
		du->dk_state = OPEN;

#if 0
		if ((du->dk_flags & DKFL_CAN_DMA) == 0) {
			/*
		 	* The rest of the initialization can be done by normal
		 	* means.
		 	*/
			return (1);
		}
		
#define	WORDEQ(x)	((((x) & 0x700) >> 8)) == ((x) & 0xff)

		/* Now setup the DMA timing values.. */
		wp = &du->dk_params;
		mode = du->dk_dma_mode;

		switch (du->dk_dma_supported) {
		case	DMA_SINGLE_MODE:
			if (WORDEQ(wp->wdp_single_dma)) {
				printf("DMA MODE ALREADY SETUP!\n");
				return 1;
			}
			mode |= 0x10;
			break;
		case	DMA_MULTIPLE_MODE:
			if (WORDEQ(wp->wdp_multi_dma)) {
				printf("DMA MODE ALREADY SETUP!\n");
				return 1;
			}
			mode |= 0x20;
			break;

		default:
			return 1;
		}

		wdtab[ctrlr].b_active = 1;
		du->dk_state = DMA_TIMING;

		outb(du->dk_port_wd_seccnt, mode);

		if (wdcommand(du, 0, 0, 0, WDFEA_SET_MODE, WDCC_FEATURES)) {
			du->dk_state = OPEN;
			return 1;
		}
		return 0;

	case	DMA_TIMING:
#endif
		du->dk_state = OPEN;
		return 1;
	}
	panic("wdcontrol");
	return (2);
}

/*
 * Wait uninterruptibly until controller is not busy, then send it a command.
 * The wait usually terminates immediately because we waited for the previous
 * command to terminate.
 */
static int
wdcommand(struct disk *du, u_int cylinder, u_int head, u_int sector,
	  u_int count, u_int command)
{
	if (du->cfg_flags & WDOPT_SLEEPHACK) {
		/* OK, so the APM bios has put the disk into SLEEP mode,
		 * how can we tell ?  Uhm, we can't.  There is no 
		 * standardized way of finding out, and the only way to
		 * wake it up is to reset it.  Bummer.
		 *
		 * All the many and varied versions of the IDE/ATA standard
		 * explicitly tells us not to look at these registers if
		 * the disk is in SLEEP mode.  Well, too bad really, we
		 * have to find out if it's in sleep mode before we can 
		 * avoid reading the registers.
		 *
		 * I have reason to belive that most disks will return
		 * either 0xff or 0x00 in all but the status register 
		 * when in SLEEP mode, but I have yet to see one return 
		 * 0x00, so we don't check for that yet.
		 *
		 * The check for WDCS_BUSY is for the case where the
		 * bios spins up the disk for us, but doesn't initialize
		 * it correctly					/phk
		 */
		if(inb(du->dk_port_wd_precomp) + inb(du->dk_port_wd_cyl_lo) +
		    inb(du->dk_port_wd_cyl_hi) + inb(du->dk_port_wd_sdh) +
		    inb(du->dk_port_wd_sector) + inb(du->dk_port_wd_seccnt) == 6 * 0xff) {
			//if (bootverbose)
				printf("wd(%d,%d): disk aSLEEP\n",
					du->dk_ctrlr, du->dk_unit);
			wdunwedge(du);
		} else if(inb(du->dk_port_wd_status) == WDCS_BUSY) {
			//if (bootverbose)
				printf("wd(%d,%d): disk is BUSY\n",
					du->dk_ctrlr, du->dk_unit);
			wdunwedge(du);
		}
	}

	if (wdwait(du, command == WDCC_RESTORE ? WDCS_READY : 0, TIMEOUT) < 0)
		return (1);

	if( command == WDCC_FEATURES) 
		outb(du->dk_port_wd_features, count);
	else if (command != WDCC_RESTORE) {
		outb(du->dk_port_wd_sdh, WDSD_IBM | (du->dk_unit << 4) | head);

		if (wdwait(du, command == WDCC_DIAGNOSE || command == WDCC_IDC
		       ? 0 : (WDCS_READY | WDCS_SEEKCMPLT), TIMEOUT) < 0)
			return 1;

		outb(du->dk_port_wd_precomp, du->dk_dd.d_precompcyl / 4);
		outb(du->dk_port_wd_cyl_lo, cylinder);
		outb(du->dk_port_wd_cyl_hi, cylinder >> 8);
		outb(du->dk_port_wd_sector, sector + 1);
		outb(du->dk_port_wd_seccnt, count);
		outb(du->dk_port_wd_ctlr, WDCTL_4BIT);
	}
	outb(du->dk_port_wd_command, command);
	return (0);
}

static void
wdsetmulti(struct disk *du)
{
	if (du->dk_multi == 0)
		du->dk_multi = 1;

	du->cfg_flags |= WDOPT_SLEEPHACK;

#if 0
	/*
	 * The config option flags low 8 bits define the maximum multi-block
	 * transfer size.  If the user wants the maximum that the drive
	 * is capable of, just set the low bits of the config option to
	 * 0x00ff.
	 */
	if ((du->cfg_flags & WDOPT_MULTIMASK) != 0 && (du->dk_multi > 1)) {
		int configval = du->cfg_flags & WDOPT_MULTIMASK;
		du->dk_multi = min(du->dk_multi, configval);
		if (wdcommand(du, 0, 0, 0, du->dk_multi, WDCC_SET_MULTI)) {
			du->dk_multi = 1;
		} else {
		    	if (wdwait(du, WDCS_READY, TIMEOUT) < 0) {
				du->dk_multi = 1;
			}
		}
	} else {
		du->dk_multi = 1;
	}
#endif
}

/*
 * issue IDC to drive to tell it just what geometry it is to be.
 */
static int
wdsetctlr(struct disk *du)
{
	int error = 0;


#ifdef WDDEBUG
	printf("wd(%d,%d): wdsetctlr: C %lu H %lu S %lu\n",
	       du->dk_ctrlr, du->dk_unit,
	       du->dk_dd.d_ncylinders, du->dk_dd.d_ntracks,
	       du->dk_dd.d_nsectors);
#endif
	if (du->dk_dd.d_ntracks == 0 || du->dk_dd.d_ntracks > 16) {
		struct wdparams *wp;

		printf("wd%d: can't handle %lu heads from partition table ",
		       du->dk_lunit, du->dk_dd.d_ntracks);
		/* obtain parameters */
		wp = &du->dk_params;
		if (wp->wdp_heads > 0 && wp->wdp_heads <= 16) {
			printf("(controller value %u restored)\n",
				wp->wdp_heads);
			du->dk_dd.d_ntracks = wp->wdp_heads;
		}
		else {
			printf("(truncating to 16)\n");
			du->dk_dd.d_ntracks = 16;
		}
	}

	if (du->dk_dd.d_nsectors == 0 || du->dk_dd.d_nsectors > 255) {
		printf("wd%d: cannot handle %lu sectors (max 255)\n",
		       du->dk_lunit, du->dk_dd.d_nsectors);
		error = 1;
	}
	if (error) {
#ifdef CMD640 
		wdtab[du->dk_ctrlr_cmd640].b_errcnt += RETRIES;
#else
		wdtab[du->dk_ctrlr].b_errcnt += RETRIES;
#endif
		return (1);
	}
	if (wdcommand(du, du->dk_dd.d_ncylinders, du->dk_dd.d_ntracks - 1, 0,
		      du->dk_dd.d_nsectors, WDCC_IDC) != 0
	    || wdwait(du, WDCS_READY, TIMEOUT) < 0) {
		wderror((struct buf *)NULL, du, "wdsetctlr failed");
		return (1);
	}

	wdsetmulti(du);

#ifdef NOTYET
/* set read caching and write caching */
	wdcommand(du, 0, 0, 0, WDFEA_RCACHE, WDCC_FEATURES);
	wdwait(du, WDCS_READY, TIMEOUT);

	wdcommand(du, 0, 0, 0, WDFEA_WCACHE, WDCC_FEATURES);
	wdwait(du, WDCS_READY, TIMEOUT);
#endif

	return (0);
}

#if 0
/*
 * Wait until driver is inactive, then set up controller.
 */
static int
wdwsetctlr(struct disk *du)
{
	int	stat;
	int	x;

	wdsleep(du->dk_ctrlr, "wdwset");
	x = splbio();
	stat = wdsetctlr(du);
	wdflushirq(du, x);
	splx(x);
	return (stat);
}
#endif

int bcmp(
	const char	*a,
	const char	*b,
	vm_size_t	len)
{
	if (len == 0)
		return 0;

	do
		if (*a++ != *b++)
			break;
	while (--len);

	return len;
}

static void
wdupdatelabel(struct disk *du) 
{
	struct wdparams *wp = &du->dk_params;

	/* update disklabel given drive information */
	du->dk_dd.d_secsize = DEV_BSIZE;
	du->dk_dd.d_ncylinders = wp->wdp_cylinders;	/* +- 1 */
	du->dk_dd.d_ntracks = wp->wdp_heads;
	du->dk_dd.d_nsectors = wp->wdp_sectors;
	du->dk_dd.d_secpercyl = du->dk_dd.d_ntracks * du->dk_dd.d_nsectors;
	du->dk_dd.d_secperunit = du->dk_dd.d_secpercyl * du->dk_dd.d_ncylinders;
	if (WDOPT_FORCEHD(du->cfg_flags)) {
		du->dk_dd.d_ntracks = WDOPT_FORCEHD(du->cfg_flags);
		du->dk_dd.d_secpercyl = 
		    du->dk_dd.d_ntracks * du->dk_dd.d_nsectors;
		du->dk_dd.d_ncylinders =
		    du->dk_dd.d_secperunit / du->dk_dd.d_secpercyl;
	}

	du->dk_dd.d_partitions[MAXPARTITIONS].p_size = du->dk_dd.d_secperunit;
	du->dk_dd.d_partitions[MAXPARTITIONS].p_offset = 0;

#if 0
	du->dk_dd.d_partitions[RAW_PART].p_size = du->dk_dd.d_secperunit;
	/* dubious ... */
	bcopy("ESDI/IDE", du->dk_dd.d_typename, 9);
	bcopy(wp->wdp_model + 20, du->dk_dd.d_packname, 14 - 1);
	/* better ... */
	du->dk_dd.d_type = DTYPE_ESDI;
	du->dk_dd.d_subtype |= DSTYPE_GEOMETRY;
#endif
}


/*
 * issue READP to drive to ask it what it is.
 */
static int
wdgetctlr(struct disk *du)
{
	int	i;
	char    tb[DEV_BSIZE], tb2[DEV_BSIZE];
	struct wdparams *wp = NULL;
	u_long flags = du->cfg_flags, value;
	u_char	mode;

again:
	if (wdcommand(du, 0, 0, 0, 0, WDCC_READP) != 0
	    || wdwait(du, WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ, TIMEOUT) != 0) {

		/*
		 * if we failed on the second try, assume non-32bit
		 */
		if( du->dk_flags & DKFL_32BIT)
			goto failed;

		/* XXX need to check error status after final transfer. */
		/*
		 * Old drives don't support WDCC_READP.  Try a seek to 0.
		 * Some IDE controllers return trash if there is no drive
		 * attached, so first test that the drive can be selected.
		 * This also avoids long waits for nonexistent drives.
		 */
		if (wdwait(du, 0, TIMEOUT) < 0)
			return (1);
		outb(du->dk_port_wd_sdh, WDSD_IBM | (du->dk_unit << 4));
		DELAY(5000);	/* usually unnecessary; drive select is fast */
		/*
		 * Do this twice: may get a false WDCS_READY the first time.
		 */
		inb(du->dk_port_wd_status);
		if ((inb(du->dk_port_wd_status) & (WDCS_BUSY | WDCS_READY))
		    != WDCS_READY
		    || wdcommand(du, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP) != 0
		    || wdwait(du, WDCS_READY | WDCS_SEEKCMPLT, TIMEOUT) != 0)
			return (1);

#if 0
		if (du->dk_unit == bootinfo.bi_n_bios_used) {
			du->dk_dd.d_secsize = DEV_BSIZE;
			du->dk_dd.d_nsectors =
			    bootinfo.bi_bios_geom[du->dk_unit] & 0xff;
			du->dk_dd.d_ntracks =
			    ((bootinfo.bi_bios_geom[du->dk_unit] >> 8) & 0xff)
			    + 1;
			/* XXX Why 2 ? */
			du->dk_dd.d_ncylinders =
			    (bootinfo.bi_bios_geom[du->dk_unit] >> 16) + 2;
			du->dk_dd.d_secpercyl =
			    du->dk_dd.d_ntracks * du->dk_dd.d_nsectors;
			du->dk_dd.d_secperunit =
			    du->dk_dd.d_secpercyl * du->dk_dd.d_ncylinders;
#if 0
			du->dk_dd.d_partitions[WDRAW].p_size =
				du->dk_dd.d_secperunit;
			du->dk_dd.d_type = DTYPE_ST506;
			du->dk_dd.d_subtype |= DSTYPE_GEOMETRY;
			strncpy(du->dk_dd.d_typename, "Bios geometry",
				sizeof du->dk_dd.d_typename);
			strncpy(du->dk_params.wdp_model, "ST506",
				sizeof du->dk_params.wdp_model);
#endif
			bootinfo.bi_n_bios_used ++;
			return 0;
		}
#endif
		/*
		 * Fake minimal drive geometry for reading the MBR.
		 * readdisklabel() may enlarge it to read the label and the
		 * bad sector table.
		 */
		du->dk_dd.d_secsize = DEV_BSIZE;
		du->dk_dd.d_nsectors = 17;
		du->dk_dd.d_ntracks = 1;
		du->dk_dd.d_ncylinders = 1;
		du->dk_dd.d_secpercyl = 17;
		du->dk_dd.d_secperunit = 17;

#if 0
		/*
		 * Fake maximal drive size for writing the label.
		 */
		du->dk_dd.d_partitions[RAW_PART].p_size = 64 * 16 * 1024;

		/*
		 * Fake some more of the label for printing by disklabel(1)
		 * in case there is no real label.
		 */
		du->dk_dd.d_type = DTYPE_ST506;
		du->dk_dd.d_subtype |= DSTYPE_GEOMETRY;
		strncpy(du->dk_dd.d_typename, "Fake geometry",
			sizeof du->dk_dd.d_typename);
#endif

		/* Fake the model name for printing by wdattach(). */
		strncpy(du->dk_params.wdp_model, "unknown",
			sizeof du->dk_params.wdp_model);

		return (0);
	}

	/* obtain parameters */
	wp = &du->dk_params;
	if (du->dk_flags & DKFL_32BIT)
		insl_le(du->dk_port_wd_data, tb, sizeof(tb) / sizeof(long));
	else
		insw_le(du->dk_port_wd_data, tb, sizeof(tb) / sizeof(short));

	/* try 32-bit data path (VLB IDE controller) */
	if (flags & WDOPT_32BIT) {
		if (! (du->dk_flags & DKFL_32BIT)) {
			bcopy(tb, tb2, sizeof(struct wdparams));
			du->dk_flags |= DKFL_32BIT;
			goto again;
		}

		/* check that we really have 32-bit controller */
		if (bcmp (tb, tb2, sizeof(struct wdparams)) != 0) {
failed:
			/* test failed, use 16-bit i/o mode */
			bcopy(tb2, tb, sizeof(struct wdparams));
			du->dk_flags &= ~DKFL_32BIT;
		}
	}

	bcopy(tb, (void *)wp, sizeof(struct wdparams));

	/* shuffle string byte order */
	for (i = 0; i < sizeof(wp->wdp_model); i += 2) {
		u_short *p;

		p = (u_short *) (wp->wdp_model + i);
		*p = ((wp->wdp_model[i] << 8) | wp->wdp_model[i+1]);
	}

	/*
	 * Clean up the wdp_model by converting nulls to spaces, and
	 * then removing the trailing spaces.
	 */
	for (i=0; i < sizeof(wp->wdp_model); i++) {
		if (wp->wdp_model[i] == '\0') {
			wp->wdp_model[i] = ' ';
		}
	}
	for (i=sizeof(wp->wdp_model)-1; i>=0 && wp->wdp_model[i]==' '; i--) {
		wp->wdp_model[i] = '\0';
	}

	/*
	 * find out the drives maximum multi-block transfer capability
	 */
	if (powermac_info.class == POWERMAC_CLASS_POWERBOOK)
		du->dk_multi = 1;
	else
		du->dk_multi = wp->wdp_nsecperint & 0xff;

	wdsetmulti(du);

	wdupdatelabel(du);

#ifdef WDDEBUG
	printf(
"\nwd(%d,%d): wdgetctlr: gc %x cyl %d trk %d sec %d type %d sz %u model %s\n",
	       du->dk_ctrlr, du->dk_unit, wp->wdp_config, wp->wdp_cylinders,
	       wp->wdp_heads, wp->wdp_sectors, wp->wdp_buffertype,
	       wp->wdp_buffersize, wp->wdp_model);
#endif

	return (0);
}

void
wdclose(dev_t dev)
{
	wddrives[UNIT(dev)]->dk_opens--;
	return;
}


io_return_t
wdread(
	dev_t		dev,
	io_req_t 	ior)
{
	return (block_io(wdstrategy, wdminphys, ior));
}

io_return_t
wdwrite(
	dev_t		dev,
	io_req_t	ior)
{
	return (block_io(wdstrategy, wdminphys, ior));
}

void
wdminphys(struct buf * bp)
{
	if (bp->b_bcount > DEV_BSIZE * MAXTRANSFER)
		bp->b_bcount = DEV_BSIZE * MAXTRANSFER;
}


int abs_sec   = -1;
int abs_count = -1;

/* IOC_OUT only and not IOC_INOUT */
io_return_t
wdgetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,	/* pointer to OUT array */
	natural_t	*count)	/* OUT */
{
	int		unit = UNIT(dev);
	struct buf	*bp1;
	int		i;
	struct disk	*drive = wddrives[unit];
	struct label_partition *part_p = &drive->dk_dd.d_partitions[PARTITION(dev)];

	switch (flavor) {

	/* Mandatory flavors */

	case DEV_GET_SIZE: {
		int 		part = PARTITION(dev);
		int 		size;
		
		data[DEV_GET_SIZE_DEVICE_SIZE] = part_p->p_size * DEV_BSIZE;
		data[DEV_GET_SIZE_RECORD_SIZE] = DEV_BSIZE;
		*count = DEV_GET_SIZE_COUNT;
		break;
	}

	/* Extra flavors */

	case V_GETPARMS: {
		struct disk_parms 	*dp;
		int 			part = PARTITION(dev);

		if (*count < sizeof (struct disk_parms)/sizeof(int))
			return (D_INVALID_OPERATION);
		dp = (struct disk_parms *) data;
		dp->dp_type = DPT_WINI;
		dp->dp_heads = wddrives[unit]->dk_dd.d_ntracks;
		dp->dp_cyls = wddrives[unit]->dk_dd.d_ncylinders;
		dp->dp_sectors  = wddrives[unit]->dk_dd.d_nsectors;
  		dp->dp_dosheads = wddrives[unit]->dk_params.wdp_heads;
		dp->dp_doscyls = wddrives[unit]->dk_params.wdp_cylinders;
		dp->dp_dossectors  = wddrives[unit]->dk_params.wdp_sectors;
		dp->dp_secsiz = DEV_BSIZE;
		dp->dp_ptag = 0;
		dp->dp_pflag = 0;
		dp->dp_pstartsec = part_p->p_offset;
		dp->dp_pnumsec = part_p->p_size;
		*count = sizeof(struct disk_parms)/sizeof(int);
		break;
	}
	case V_RDABS: {
		/* V_RDABS is relative to head 0, sector 0, cylinder 0 */
		if (*count < DEV_BSIZE/sizeof (int)) {
			printf("wd%d: RDABS bad size %x", unit, count);
			return (D_INVALID_OPERATION);
		}
		if (wd_rw_abs(dev, data, IO_READ, 
				abs_sec, DEV_BSIZE) != D_SUCCESS) 
			return(D_INVALID_OPERATION);
		*count = DEV_BSIZE/sizeof(int);
		return KERN_SUCCESS;
	}
	case V_VERIFY: 
	{
		int cnt = abs_count * DEV_BSIZE;
		int sec = abs_sec;
                int bsize = PAGE_SIZE;
		char *verify_buf;

		(void) kmem_alloc(kernel_map,
				  (vm_offset_t *)&verify_buf,
				  bsize);

		*data = 0;
		while (cnt > 0) {
			int xcount = (cnt < bsize) ? cnt : bsize;
			if (wd_rw_abs(dev, (dev_status_t)verify_buf,
					IO_READ, sec, xcount) != D_SUCCESS) {
				*data = BAD_BLK;
				break;
			} else {
				cnt -= xcount;
				sec += xcount / DEV_BSIZE;
			}
	        }

		(void) kmem_free(kernel_map, (vm_offset_t)verify_buf, bsize);
		*count = 1;
		break;
	}
	default:
		return(D_INVALID_OPERATION);
	}
	return D_SUCCESS;
}

/* IOC_VOID or IOC_IN or IOC_INOUT */
io_return_t
wdsetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,
	natural_t	count)
{
	struct buf	*bp1;
	io_return_t	errcode = D_SUCCESS;
	int		unit = UNIT(dev);
	struct disk	*drive = wddrives[unit];
	struct label_partition *part_p =
				&drive->dk_dd.d_partitions[PARTITION(dev)];

	switch (flavor) {
	case V_REMOUNT:
		(void) getvtoc(dev);
		break;
	case V_ABS:
		abs_sec = *(int *)data;
		if ((int) count == 2)
			abs_count = data[1];
		break;
	case V_WRABS:
		/* V_WRABS is relative to head 0, sector 0, cylinder 0 */
		if (count < DEV_BSIZE/sizeof (int)) {
			printf("wd%d: WRABS bad size %x", unit, count);
			return (D_INVALID_OPERATION);
		}
		if (wd_rw_abs(dev, data, IO_WRITE, 
			       abs_sec, DEV_BSIZE) != D_SUCCESS)
			return(D_INVALID_OPERATION);
		break;
	default:
		return (D_INVALID_OPERATION);
	}
	return (errcode);
}


/*
 *	Routine to return information to kernel.
 */
io_return_t
wddevinfo(
	dev_t		dev,
	dev_flavor_t	flavor,
	char		*info)
{
	register int	result;

	result = D_SUCCESS;

	switch (flavor) {
	case D_INFO_BLOCK_SIZE:
		*((int *) info) = DEV_BSIZE;
		break;
	default:
		result = D_INVALID_OPERATION;
	}

	return(result);
}

static boolean_t
getvtoc(dev_t dev)
{
	struct disk		*drive = wddrives[UNIT(dev)];
	struct disklabel	*label = &drive->dk_dd, *new_label;
	int			partition_index = 0;
	spl_t			s;

	if (kmem_alloc(kernel_map, (vm_offset_t *)&new_label, sizeof(*new_label)) != KERN_SUCCESS)
		return 0;

	bzero((char *) new_label, sizeof(*new_label));

	/* 
	 * Setup last partition that we can access entire physical disk
	 */

	label->d_secsize = DEV_BSIZE;
	label->d_partitions[MAXPARTITIONS].p_size = -1;
	label->d_partitions[MAXPARTITIONS].p_offset = 0;

	s = splbio();
	wdreset(drive);	/* Try to fix TIMEOUT bug.. */
	splx(s);

	if (read_ata_mac_label(dev,
			0,			/* sector 0 */
			0,			/* ext base */
			&partition_index,	/* first partition */
			new_label,
			wd_rw_abs) != D_SUCCESS) {
		/* make partition 'c' the whole disk in case of failure */
		label->d_partitions[PART_DISK].p_offset = 0;
		label->d_partitions[PART_DISK].p_size =
		  	drive->dk_params.wdp_cylinders *
			drive->dk_params.wdp_heads * 
			drive->dk_params.wdp_sectors;
	} else
		bcopy((char *) new_label, (char *) label, sizeof(*new_label));

	wdupdatelabel(drive);

	label->d_precompcyl = 0;	/* Force it.. */
	
	kmem_free(kernel_map, (vm_offset_t) new_label, sizeof(*new_label));

	return(TRUE);
}

io_return_t
wd_rw_abs(
	dev_t		dev,
	dev_status_t	data,
	int		rw,
	int		sec,
	int		count)
{
	io_req_t	ior;
	io_return_t	error;

	io_req_alloc(ior);
	ior->io_next = 0;
	ior->io_unit = dev;	
	ior->io_data = (io_buf_ptr_t)data;
	ior->io_count = count;
	ior->io_recnum = sec;
	ior->io_error = 0;
	if (rw == IO_READ)
		ior->io_op = IO_READ;
	else
		ior->io_op = IO_WRITE;
	ior->io_op |= B_MD1;
	wdstrategy(ior);
	iowait(ior);
	error = ior->io_error;
	io_req_free(ior);
	return(error);
}


#if 0
int
wdioctl(dev_t dev, int cmd, caddr_t addr, int flags, struct proc *p)
{
	int	lunit = UNIT(dev);
	register struct disk *du;
	int	error;
#ifdef notyet
	struct uio auio;
	struct iovec aiov;
	struct format_op *fop;
#endif

	du = wddrives[lunit];
	wdsleep(du->dk_ctrlr, "wdioct");
	error = dsioctl("wd", dev, cmd, addr, flags, &du->dk_slices,
			wdstrategy1, (ds_setgeom_t *)NULL);
	if (error != -1)
		return (error);

	switch (cmd) {
	case DIOCSBADSCAN:
		if (*(int *)addr)
			du->dk_flags |= DKFL_BADSCAN;
		else
			du->dk_flags &= ~DKFL_BADSCAN;
		return (0);
#ifdef notyet
	case DIOCWFORMAT:
		if (!(flag & FWRITE))
			return (EBADF);
		fop = (struct format_op *)addr;
		aiov.iov_base = fop->df_buf;
		aiov.iov_len = fop->df_count;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_resid = fop->df_count;
		auio.uio_segflg = 0;
		auio.uio_offset = fop->df_startblk * du->dk_dd.d_secsize;
#error /* XXX the 386BSD interface is different */
		error = physio(wdformat, &rwdbuf[lunit], 0, dev, B_WRITE,
			       minphys, &auio);
		fop->df_count -= auio.uio_resid;
		fop->df_reg[0] = du->dk_status;
		fop->df_reg[1] = du->dk_error;
		return (error);
#endif

	default:
		return (ENOTTY);
	}
}
#endif

#ifdef B_FORMAT
int
wdformat(struct buf *bp)
{

	bp->b_flags |= B_FORMAT;
	wdstrategy(bp);
	/*
	 * phk put this here, better that return(wdstrategy(bp));
	 * XXX
	 */
	return -1;
}
#endif

#if 0
int
wdsize(dev_t dev)
{
	struct disk *du;
	int	lunit;

	lunit = UNIT(dev);
	if (lunit >= NIDE || dktype(dev) != 0)
		return (-1);
	du = wddrives[lunit];
	if (du == NULL)
		return (-1);
	return (dssize(dev, &du->dk_slices, wdopen, wdclose));
}

/*
 * Dump core after a system crash.
 */
int
wddump(dev_t dev)
{
	register struct disk *du;
	struct disklabel *lp;
	long	num;		/* number of sectors to write */
	int	lunit, part;
	long	blkoff, blknum;
	long	blkchk, blkcnt, blknext;
	long	cylin, head, sector;
	long	secpertrk, secpercyl, nblocks;
	u_long	ds_offset;
	char   *addr;
	static int wddoingadump = 0;

	/* Toss any characters present prior to dump. */
	while (cncheckc() != -1)
		;

	/* Check for acceptable device. */
	/* XXX should reset to maybe allow du->dk_state < OPEN. */
	lunit = UNIT(dev);	/* eventually support floppies? */
	part = dkpart(dev);
	if (lunit >= NIDE || (du = wddrives[lunit]) == NULL
	    || du->dk_state < OPEN
	    || (lp = dsgetlabel(dev, du->dk_slices)) == NULL)
		return (ENXIO);

	/* Size of memory to dump, in disk sectors. */
	num = (u_long)Maxmem * PAGE_SIZE / du->dk_dd.d_secsize;

	secpertrk = du->dk_dd.d_nsectors;
	secpercyl = du->dk_dd.d_secpercyl;
	nblocks = lp->d_partitions[part].p_size;
	blkoff = lp->d_partitions[part].p_offset;
	/* XXX */
	ds_offset = du->dk_slices->dss_slices[dkslice(dev)].ds_offset;
	blkoff += ds_offset;

#if 0
	pg("part %x, nblocks %d, dumplo %d num %d\n",
	   part, nblocks, dumplo, num);
#endif

	/* Check transfer bounds against partition size. */
	if (dumplo < 0 || dumplo + num > nblocks)
		return (EINVAL);

	/* Check if we are being called recursively. */
	if (wddoingadump)
		return (EFAULT);

#if 0
	/* Mark controller active for if we panic during the dump. */
	wdtab[du->dk_ctrlr].b_active = 1;
#endif
	wddoingadump = 1;

	/* Recalibrate the drive. */
	DELAY(5);		/* ATA spec XXX NOT */
	if (wdcommand(du, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP) != 0
	    || wdwait(du, WDCS_READY | WDCS_SEEKCMPLT, TIMEOUT) != 0
	    || wdsetctlr(du) != 0) {
		wderror((struct buf *)NULL, du, "wddump: recalibrate failed");
		return (EIO);
	}

	du->dk_flags |= DKFL_SINGLE;
	addr = (char *) 0;
	blknum = dumplo + blkoff;
	while (num > 0) {
		blkcnt = num;
		if (blkcnt > MAXTRANSFER)
			blkcnt = MAXTRANSFER;
		/* Keep transfer within current cylinder. */
		if ((blknum + blkcnt - 1) / secpercyl != blknum / secpercyl)
			blkcnt = secpercyl - (blknum % secpercyl);
		blknext = blknum + blkcnt;

		/*
		 * See if one of the sectors is in the bad sector list
		 * (if we have one).  If the first sector is bad, then
		 * reduce the transfer to this one bad sector; if another
		 * sector is bad, then reduce reduce the transfer to
		 * avoid any bad sectors.
		 */
		if (du->dk_flags & DKFL_SINGLE
		    && dsgetbad(dev, du->dk_slices) != NULL) {
		  for (blkchk = blknum; blkchk < blknum + blkcnt; blkchk++) {
			daddr_t blknew;
			blknew = transbad144(dsgetbad(dev, du->dk_slices),
					     blkchk - ds_offset) + ds_offset;
			if (blknew != blkchk) {
				/* Found bad block. */
				blkcnt = blkchk - blknum;
				if (blkcnt > 0) {
					blknext = blknum + blkcnt;
					goto out;
				}
				blkcnt = 1;
				blknext = blknum + blkcnt;
#if 1 || defined(WDDEBUG)
				printf("bad block %lu -> %lu\n",
				       blknum, blknew);
#endif
				break;
			}
		  }
		}
out:

		/* Compute disk address. */
		cylin = blknum / secpercyl;
		head = (blknum % secpercyl) / secpertrk;
		sector = blknum % secpertrk;

#if 0
		/* Let's just talk about this first... */
		pg("cylin l%d head %ld sector %ld addr 0x%x count %ld",
		   cylin, head, sector, addr, blkcnt);
#endif

		/* Do the write. */
		if (wdcommand(du, cylin, head, sector, blkcnt, WDCC_WRITE)
		    != 0) {
			wderror((struct buf *)NULL, du,
				"wddump: timeout waiting to to give command");
			return (EIO);
		}
		while (blkcnt != 0) {
			pmap_enter(kernel_pmap, (vm_offset_t)CADDR1, trunc_page(addr),
				   VM_PROT_READ, TRUE);

			/* Ready to send data? */
			DELAY(5);	/* ATA spec */
			if (wdwait(du, WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ, TIMEOUT)
			    < 0) {
				wderror((struct buf *)NULL, du,
					"wddump: timeout waiting for DRQ");
				return (EIO);
			}
			if (du->dk_flags & DKFL_32BIT)
				outsl(du->dk_port_wd_data,
				      CADDR1 + ((int)addr & PAGE_MASK),
				      DEV_BSIZE / sizeof(long));
			else
				outsw(du->dk_port_wd_data,
				      CADDR1 + ((int)addr & PAGE_MASK),
				      DEV_BSIZE / sizeof(short));
			addr += DEV_BSIZE;
			if ((unsigned)addr % (1024 * 1024) == 0)
				printf("%ld ", num / (1024 * 1024 / DEV_BSIZE));
			num--;
			blkcnt--;
		}

		/* Wait for completion. */
		DELAY(5);	/* ATA spec XXX NOT */
		if (wdwait(du, WDCS_READY | WDCS_SEEKCMPLT, TIMEOUT) < 0) {
			wderror((struct buf *)NULL, du,
				"wddump: timeout waiting for status");
			return (EIO);
		}

		/* Check final status. */
		if (du->dk_status
		    & (WDCS_READY | WDCS_SEEKCMPLT | WDCS_DRQ | WDCS_ERR)
		    != (WDCS_READY | WDCS_SEEKCMPLT)) {
			wderror((struct buf *)NULL, du,
				"wddump: extra DRQ, or error");
			return (EIO);
		}

		/* Update block count. */
		blknum = blknext;

		/* Operator aborting dump? */
		if (cncheckc() != -1)
			return (EINTR);
	}
	return (0);
}
#endif

static void
wderror(struct buf *bp, struct disk *du, char *mesg)
{
	if (bp == NULL)
		printf("wd%d: %s:\n", du->dk_lunit, mesg);
	else
#if 0
		diskerr(bp, "wd", mesg, LOG_PRINTF, du->dk_skip,
			dsgetlabel(bp->b_dev, du->dk_slices));
#else
		printf("ide%d: %s: \n", UNIT(bp->b_dev), mesg);
#endif

	printf("wd%d: status %b error %b state=%d\n", du->dk_lunit,
	       du->dk_status, WDCS_BITS, du->dk_error, WDERR_BITS,
		du->dk_state);
}

/*
 * Discard any interrupts that were latched by the interrupt system while
 * we were doing polled i/o.
 */
static void
wdflushirq(struct disk *du, int old_ipl)
{
#ifdef CMD640
	wdtab[du->dk_ctrlr_cmd640].b_active = 2;
	splx(old_ipl);
	(void)splbio();
	wdtab[du->dk_ctrlr_cmd640].b_active = 0;
#else
	wdtab[du->dk_ctrlr].b_active = 2;
	splx(old_ipl);
	(void)splbio();
	wdtab[du->dk_ctrlr].b_active = 0;
#endif  
}

/*
 * Reset the controller.
 */
static int
wdreset(struct disk *du)
{
	int     err = 0;


	(void)wdwait(du, 0, TIMEOUT);
	outb(du->dk_port_wd_ctlr, WDCTL_IDS | WDCTL_RST | WDCTL_4BIT);
	DELAY(30 * 1000);
	outb(du->dk_port_wd_ctlr, /*WDCTL_IDS |*/ WDCTL_4BIT);
#ifdef ATAPI
	if (wdwait(du, WDCS_READY | WDCS_SEEKCMPLT, TIMEOUT) != 0)
		err = 1;                /* no IDE drive found */
	du->dk_error = inb(du->dk_port_wd_error);
	if (du->dk_error != 0x01)
		err = 1;                /* the drive is incompatible */
#else
	if (wdwait(du, WDCS_READY | WDCS_SEEKCMPLT, TIMEOUT) != 0
	    || (du->dk_error = inb(du->dk_port_wd_error)) != 0x01)
		return (1);
#endif
#if 0
	outb(du->dk_port_wd_ctlr, WDCTL_4BIT);
#endif

	return (err);
}

/*
 * Sleep until driver is inactive.
 * This is used only for avoiding rare race conditions, so it is unimportant
 * that the sleep may be far too short or too long.
 */
static void
wdsleep(int ctrlr, char *wmesg)
{
	int s = splbio();
#ifdef CMD640
	if (eide_quirks & Q_CMD640B)
		ctrlr = PRIMARY;
#endif
	while (wdtab[ctrlr].b_active) {
		thread_set_timeout(1);
		thread_block((void (*)(void)) 0);
		reset_timeout_check(&current_thread()->timer);
	}
	splx(s);
}

static void
wdtimeout(void *cdu)
{
	struct disk *du;
	int	x;
	static	int	timeouts = 0;

	du = (struct disk *)cdu;
	x = splbio();
	if (du->dk_timeout != 0 && --du->dk_timeout == 0) {
		printf("{IDE TIMEOUT}");
		if(timeouts++ == 5) {
			timeouts = 0;
		} else if(timeouts++ < 5)
			wderror((struct buf *)NULL, du, "interrupt timeout");
		wdunwedge(du);
		wdflushirq(du, x);
		du->dk_skip = 0;
		du->dk_flags |= DKFL_SINGLE;
#ifdef	DKFL_CAN_DMA
		if (du->dk_flags & DKFL_USING_DMA) {
			printf("Status %x\n", inl_le((unsigned long) &du->dk_dma_chan->d_status));
			dbdma_stop(du->dk_dma_chan);
			du->dk_flags &= ~DKFL_USING_DMA;
		}
#endif

#ifdef	CHAINED_IOS
		{
			struct buf *bp;

			bp = (struct buf *)
				queue_first(&wdtab[du->dk_ctrlr].controller_queue);
			if (bp && bp->io_op & IO_CHAINED)
				split_io_reqs(bp);
		}
#endif
		wdstart(du->dk_ctrlr);
	}
	timeout(wdtimeout, cdu, hz);
	splx(x);
}

/*
 * Reset the controller after it has become wedged.  This is different from
 * wdreset() so that wdreset() can be used in the probe and so that this
 * can restore the geometry .
 */
static int
wdunwedge(struct disk *du)
{
	struct disk *du1;
	int	lunit;

	/* Schedule other drives for recalibration. */
	for (lunit = 0; lunit < NIDE; lunit++)
		if ((du1 = wddrives[lunit]) != NULL && du1 != du
		    && du1->dk_ctrlr == du->dk_ctrlr
		    && du1->dk_state > WANTOPEN)
			du1->dk_state = WANTOPEN;

	DELAY(RECOVERYTIME);
	if (wdreset(du) == 0) {
		/*
		 * XXX - recalibrate current drive now because some callers
		 * aren't prepared to have its state change.
		 */
		if (wdcommand(du, 0, 0, 0, 0, WDCC_RESTORE | WD_STEP) == 0
		    && wdwait(du, WDCS_READY | WDCS_SEEKCMPLT, TIMEOUT) == 0
		    && wdsetctlr(du) == 0)
			return (0);
	}
	wderror((struct buf *)NULL, du, "wdunwedge failed");
	return (1);
}

/*
 * Wait uninterruptibly until controller is not busy and either certain
 * status bits are set or an error has occurred.
 * The wait is usually short unless it is for the controller to process
 * an entire critical command.
 * Return 1 for (possibly stale) controller errors, -1 for timeout errors,
 * or 0 for no errors.
 * Return controller status in du->dk_status and, if there was a controller
 * error, return the error code in du->dk_error.
 */
#ifdef WD_COUNT_RETRIES
static int min_retries[NWD];
#endif

static int
wdwait(struct disk *du, u_char bits_wanted, int timeout)
{
	u_char	status;

#define	POLLING		1000

	timeout += POLLING;

/*
 * This delay is really too long, but does not impact the performance
 * as much when using the multi-sector option.  Shorter delays have
 * caused I/O errors on some drives and system configs.  This should
 * probably be fixed if we develop a better short term delay mechanism.
 */
	DELAY(1);

	do {
#ifdef WD_COUNT_RETRIES
		if (min_retries[du->dk_ctrlr] > timeout
		    || min_retries[du->dk_ctrlr] == 0)
			min_retries[du->dk_ctrlr] = timeout;
#endif
		du->dk_status = status = inb(du->dk_port_wd_status);
#if 1 /*def ATAPI*/
		/*
		 * Atapi drives have a very interesting feature, when attached
		 * as a slave on the IDE bus, and there is no master.
		 * They release the bus after getting the command.
		 * We should reselect the drive here to get the status.
		 */
		if (status == 0xff) {
			outb(du->dk_port_wd_sdh, WDSD_IBM | du->dk_unit << 4);
			du->dk_status = status = inb(du->dk_port_wd_status);
		}
#endif
		if (!(status & WDCS_BUSY)) {
			if (status & WDCS_ERR) {
				du->dk_error = inb(du->dk_port_wd_error);
				/*
				 * We once returned here.  This is wrong
				 * because the error bit is apparently only
				 * valid after the controller has interrupted
				 * (e.g., the error bit is stale when we wait
				 * for DRQ for writes).  So we can't depend
				 * on the error bit at all when polling for
				 * command completion.
				 */
			}
			if ((status & bits_wanted) == bits_wanted)
				return (status & WDCS_ERR);
		}
		if (timeout < TIMEOUT)
			/*
			 * Switch to a polling rate of about 1 KHz so that
			 * the timeout is almost machine-independent.  The
			 * controller is taking a long time to respond, so
			 * an extra msec won't matter.
			 */
			DELAY(1000);
		else
			DELAY(1);
	} while (--timeout != 0);
	return (-1);
}


static struct io_segment *
wd_construct_entry(vm_offset_t address, long count, struct io_segment *segp)
{
	vm_offset_t	physaddr;
	long		block, real_count;


	while (count > 0) {
		physaddr = kvtophys(address);

		real_count = 0x1000 - (physaddr & 0xfff);

		segp->physaddr[0] = physaddr;

		if (real_count < DEV_BSIZE) 
			segp->physaddr[1] = kvtophys(address + real_count);
		else
			segp->physaddr[1] = 0;

		segp->virtaddr = address;

		count -= DEV_BSIZE;
		address += DEV_BSIZE;
		segp++;
	}

	return segp;

}

void
wd_build_segments(struct disk *drive, io_req_t ior)
{
	long	count;
	struct  io_segment *segp = &drive->dk_segments[0];

	if ((ior->io_op & IO_CHAINED) == 0) {
		(void) wd_construct_entry((vm_offset_t) ior->io_data,
						ior->io_count, segp);
	} else {
		for (; ior; ior = ior->io_link) {
			count = ior->io_count;

			if (ior->io_link)
				count -= ior->io_link->io_count;

			segp = wd_construct_entry((vm_offset_t) ior->io_data,
					count, segp);
		}
	}
}

#if DKFL_CAN_DMA
void
wd_build_dma(struct disk *drive, io_req_t ior, long count)
{
	struct		io_segment	*segp;
	vm_offset_t	data;
	dbdma_command_t	*cmd = drive->dk_dma_cmds;
	unsigned long	real_count, 
			dir = (ior->io_op & IO_READ) ? DBDMA_CMD_IN_MORE : DBDMA_CMD_OUT_MORE;


	segp = &drive->dk_segments[drive->dk_skip];

	while (count) {
		/* Check to see if this spans a page.. */
		if (segp->physaddr[1]) 
			real_count = 0x1000 - (segp->physaddr[0] & 0xfff);
		 else
			real_count = DEV_BSIZE;

	        DBDMA_BUILD(cmd, dir, 0, real_count, segp->physaddr[0],
			DBDMA_INT_NEVER, DBDMA_BRANCH_NEVER, DBDMA_WAIT_NEVER);
 

		if (segp->physaddr[1]) {
			printf("{span %d}", real_count);
			real_count = DEV_BSIZE - real_count;
			cmd++;
	        	DBDMA_BUILD(cmd, dir, 0, real_count, segp->physaddr[1],
				DBDMA_INT_NEVER, DBDMA_BRANCH_NEVER, DBDMA_WAIT_NEVER);
		}
			
		cmd++;
		count--;
		segp++;
	}

	DBDMA_BUILD(cmd, DBDMA_CMD_NOP, 0, 0, 0, DBDMA_INT_NEVER,
			DBDMA_BRANCH_NEVER, DBDMA_WAIT_NEVER);

	cmd++;

	DBDMA_BUILD(cmd, DBDMA_CMD_STOP, 0, 0, 0, DBDMA_INT_NEVER,
			DBDMA_BRANCH_NEVER, DBDMA_WAIT_NEVER);

	eieio();	/* Wait until things flush out.. */
}

void
wd_sync_request(io_req_t ior)
{
	long	count;
	boolean_t	isread = (ior->io_op & IO_READ) ? TRUE : FALSE;

	if ((ior->io_op & IO_CHAINED) == 0) {
		if (isread)
			invalidate_cache_for_io((vm_offset_t)ior->io_data, ior->io_count, FALSE);
		else
			flush_dcache((vm_offset_t)ior->io_data, ior->io_count, FALSE);
	} else {
		while (ior) {
			count = ior->io_count;

			if (ior->io_link)
				count -= ior->io_link->io_count;

			if (isread)
				invalidate_cache_for_io((vm_offset_t) ior->io_data, count, FALSE);
			else
				flush_dcache((vm_offset_t)ior->io_data, count, FALSE);

			ior = ior->io_link;
		}
	}
}
#endif

#endif /* NWD > 0 */
