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
 * Revision 2.12.5.4  92/09/15  17:15:04  jeffreyh
 * 	   Fix value of total number of blocks to read and
 * 	   b_resid if requested size is bigger than number
 * 	   of remaining blocks and not an integral number of blocks.
 * 	   [barbou@gr.osf.org]
 * 	Added infinitely-fast disk code for partition p, some IO counters
 * 	[92/08/10            sjs]
 * 
 * Revision 2.12.5.3  92/04/30  11:55:26  bernadat
 * 	Parallelization for SystemPro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.12.5.2  92/03/28  10:06:18  jeffreyh
 * 	Added to the loop in probe to account for new stupid hardware.
 * 	[92/03/26            jeffreyh]
 * 	Changes from MK71
 * 	[92/03/20  12:15:07  jeffreyh]
 * 
 * Revision 2.17  92/03/01  00:39:53  rpd
 * 	Cleaned up syntactically incorrect ifdefs.
 * 	[92/02/29            rpd]
 * 
 * Revision 2.16  92/02/23  22:43:07  elf
 * 	Added (mandatory) DEV_GET_SIZE flavor of get_status.
 * 	[92/02/22            af]
 * 
 * Revision 2.15  92/02/19  16:29:51  elf
 * 	On 25-Jan, did not consider NO ACTIVE mach parition.
 * 	Add "BIOS" support -- always boot mach partition NOT active one.
 * 	[92/01/31            rvb]
 * 
 * Revision 2.14  92/01/27  16:43:06  rpd
 * 	Fixed hdgetstat and hdsetstat to return D_INVALID_OPERATION
 * 	for unsupported flavors.
 * 	[92/01/26            rpd]
 * 
 * Revision 2.13  92/01/14  16:43:51  rpd
 * 	Error in badblock_mapping code in the case there was sector replacement.
 * 	For all the sectors in the disk block before the bad sector, you
 * 	badblock_mapping should give the identity map and it was not.
 * 	[92/01/08            rvb]
 * 
 * Revision 2.12  91/11/18  17:34:19  rvb
 * 	For now, back out the hdprobe(), hdslave() probes and use
 * 	the old simple test and believe BIOS.
 * 
 * Revision 2.11  91/11/12  11:09:32  rvb
 * 	Amazing how hard getting the probe to work for all machines is.
 * 	V_REMOUNT must clear gotvtoc[].
 * 	[91/10/16            rvb]
 * 
 * Revision 2.10  91/10/07  17:25:35  af
 * 	Now works with 2 disk drives, new probe/slave routines, misc cleanup
 * 	[91/08/07            mg32]
 * 
 * 	From 2.5:
 *	Rationalize p_flag
 *	Kill nuisance print out.
 *	Removed "hdioctl(): do not recognize ioctl ..." printf().
 * 	[91/08/07            rvb]
 * 
 * Revision 2.9  91/08/28  11:11:42  jsb
 * 	Replace hdbsize with hddevinfo.
 * 	[91/08/12  17:33:59  dlb]
 * 
 * 	Add block size routine.
 * 	[91/08/05  17:39:16  dlb]
 * 
 * Revision 2.8  91/08/24  11:57:46  af
 * 	New MI autoconf.
 * 	[91/08/02  02:52:47  af]
 * 
 * Revision 2.7  91/05/14  16:23:24  mrt
 * 	Correcting copyright
 * 
 * Revision 2.6  91/02/05  17:17:01  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:43:01  mrt]
 * 
 * Revision 2.5  91/01/08  17:32:51  rpd
 * 	Allow ioctl's
 * 	[90/12/19            rvb]
 * 
 * Revision 2.4  90/11/26  14:49:37  rvb
 * 	jsb bet me to XMK34, sigh ...
 * 	[90/11/26            rvb]
 * 	Synched 2.5 & 3.0 at I386q (r1.8.1.15) & XMK35 (r2.4)
 * 	[90/11/15            rvb]
 * 
 * Revision 1.8.1.14  90/09/18  08:38:49  rvb
 * 	Typo & vs && at line 592.		[contrib]
 * 	Make Status printout on error only conditional on hd_print_error.
 * 	So we can get printout during clobber_my_disk.
 * 	[90/09/08            rvb]
 * 
 * Revision 1.8.1.13  90/08/25  15:44:38  rvb
 * 	Use take_<>_irq() vs direct manipulations of ivect and friends.
 * 	[90/08/20            rvb]
 * 
 * Revision 1.8.1.12  90/07/27  11:25:30  rvb
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	Let anyone who as opened the disk do absolute io.
 * 	[90/07/27            rvb]
 * 
 * Revision 1.8.1.11  90/07/10  11:43:22  rvb
 * 	Unbelievable bug in setcontroller.
 * 	New style probe/slave/attach.
 * 	[90/06/15            rvb]
 * 
 * Revision 1.8.1.10  90/03/29  19:00:00  rvb
 * 	Conditionally, print out state info for "state error".
 * 	[90/03/26            rvb]
 * 
 * Revision 1.8.1.8  90/03/10  00:27:20  rvb
 * 	Fence post error iff (bp->b_blkno + hh.blocktotal ) > partition_p->p_size)
 * 	[90/03/10            rvb]
 * 
 * Revision 1.8.1.7  90/02/28  15:49:35  rvb
 * 	Fix numerous typo's in Olivetti disclaimer.
 * 	[90/02/28            rvb]
 * 
 * Revision 1.8.1.6  90/01/16  15:54:14  rvb
 * 	FLush pdinfo/vtoc -> evtoc
 * 	[90/01/16            rvb]
 * 
 * 	Must be able to return "dos{cyl,head,sector}"
 * 	[90/01/12            rvb]
 * 
 * 	Be careful about p_size bound's checks if B_MD1 is true.
 * 	[90/01/12            rvb]
 * 
 * Revision 1.8.1.5  90/01/08  13:29:29  rvb
 * 	Add Intel copyright.
 * 	Add Olivetti copyright.
 * 	[90/01/08            rvb]
 * 
 * 	It is no longer possible to set the start and size of disk
 * 	partition "PART_DISK" -- it is always loaded from the DOS
 * 	partition data.
 * 	[90/01/08            rvb]
 * 
 * Revision 1.8.1.4  90/01/02  13:54:58  rvb
 * 	Temporarily regress driver to one that is known to work with Vectra's.
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
 *   Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.
 * 
 * 		All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appears in all
 * copies and that both the copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Intel
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * 
 * INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 *  AT Hard Disk Driver
 *  Copyright Ing. C. Olivetti & S.p.A. 1989
 *  All rights reserved.
 *
 */
 
/* Copyright 1988, 1989 by Olivetti Advanced Technology Center, Inc.,
 * Cupertino, California.
 *
 *		All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appears in all
 * copies and that both the copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Olivetti
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * 
 * OLIVETTI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL OLIVETTI BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUR OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <hd.h>
#include <cpus.h>
#include <mp_v1_1.h>
#include <chained_ios.h>
#include <device/param.h>

#include <string.h>
#include <types.h>
#define PRIBIO 20
#include <device/buf.h>
#include <device/conf.h>
#include <device/errno.h>
#include <device/subrs.h>
#include <device/misc_protos.h>
#include <device/ds_routines.h>
#include <device/param.h>
#include <sys/ioctl.h>
#include <i386/pio.h>
#include <i386/ipl.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <machine/disk.h>
#include <chips/busses.h>
#include <i386/AT386/hdreg.h>
#include <i386/AT386/misc_protos.h>
#include <i386/AT386/hd_entries.h>
#include <i386/AT386/mp/mp.h>
#include <vm/vm_kern.h>

#if	NCPUS > 1

#if	CBUS
#include	<cbus/cbus.h>
#endif	/* CBUS */

#if	MBUS
#include <busses/mbus/mbus.h>
#define	at386_io_lock(op)	mbus_dev_lock(&hd_ctrl[ctrl]->lock, (op))
#define	at386_io_unlock()	mbus_dev_unlock(&hd_ctrl[ctrl]->lock)
#endif	/* MBUS */

#if	MP_V1_1
#include <i386/AT386/mp/mp_v1_1.h>
#endif	/* MP_V1_1 */

#endif	/* NCPUS > 1 */

#define b_cylin		b_resid
#define PAGESIZ 4096

/* Forward */

extern int		hdprobe(
				int			port,
				struct bus_ctlr		* ctlr);
extern int		hdslave(
				struct bus_device	* bd,
				caddr_t			xxx);
extern void		hdattach(
				struct bus_device	*dev);
extern void		hdstrategy(
				struct buf		* bp);
extern void		hdminphys(
				struct buf		* bp);
extern void		hdstart(
				int			ctrl);
extern void		hd_dump_registers(
				int			addr);
extern void		hderror(
				struct buf		*bp,
				int 			ctrl);
extern boolean_t	getvtoc(
				int			unit);
extern void		set_controller(
				int			unit);
extern void		waitcontroller(
				int			unit);
extern void		start_rw(
				int			read,
				int			ctrl);
extern int		badblock_mapping(
				int			ctrl);
extern int		xfermode(
				int			ctrl);

extern void		hd_read_id(
				   struct bus_device	*dev,
				   int			ctrl);

/*
 * Per controller info
 */

struct hd_ctrl {
	int			address; /* i386_port */
	struct	hh		state;
	int 			need_set_controller;
#if	NCPUS > 1 && MBUS
	struct mp_dev_lock	lock;
#endif	/* NCPUS > 1 && MBUS */
};

typedef struct hd_ctrl *hd_ctrl_t;

#define setcontroller(unit) \
	(hd_ctrl[unit>>1]->need_set_controller |= (1<<(unit&1)))

struct hd_ctrl *hd_ctrl[(NHD+1)/2];

/*
 * Per drive info
 */

typedef struct {
	unsigned short	ncyl;
	unsigned short	nheads;
	unsigned short	precomp;
	unsigned short	nsec;
} hdisk_t;

struct hd_drive {
	hdisk_t			cmos_parm;
	struct io_req 		io_queue;
	struct disklabel	label;
	struct alt_info		*alt_info;
};

typedef struct label_partition *partition_t;
typedef struct hd_drive *hd_drive_t;

struct hd_drive *hd_drive[NHD];

/*
 * metric variables
 */
int	hd_read_count = 0;
int	hd_write_count = 0;
int	hd_queue_length = 0;


#define	DISKTEST
#ifdef	DISKTEST
#define	NULLPARTITION(z)	((z) == 0xf)	/* last partition */
#endif	/* DISKTEST */

caddr_t	hd_std[NHD] = { 0 };
struct	bus_device *hd_dinfo[NHD*NDRIVES];
struct	bus_ctlr *hd_minfo[NHD];
struct	bus_driver	hddriver = {
	(probe_t)hdprobe, hdslave, hdattach, 0, hd_std, "hd", hd_dinfo,
		"hdc", hd_minfo, 0};

int
hdprobe(
	int		port,
	struct bus_ctlr	* ctlr)
{
	int 		i,
			ctrl = ctlr->unit;
	int 		addr = (int)ctlr->address;
	hd_ctrl_t	ctrl_p;	

	if (ctrl >= (NHD+1)/2)
		return(0);

	if (ctrl_p = hd_ctrl[ctrl])
		return(1);

	outb(PORT_DRIVE_HEADREGISTER(addr),0);
	outb(PORT_COMMAND(addr),CMD_RESTORE);
	if (inb(PORT_STATUS(addr))&(STAT_ECC|STAT_WRITEFAULT|STAT_ERROR))
		return(0);
	for (i=200; i && inb(PORT_STATUS(addr))&STAT_BUSY; i--)
	  	delay(10000);
	if (i && (inb(PORT_STATUS(addr))&(STAT_READY|STAT_ERROR)) == STAT_READY) {
		ctrl_p = (hd_ctrl_t) kalloc(sizeof(struct hd_ctrl));
		if (!ctrl_p) {
		  	printf("hd_probe: kalloc failed\n");
			return(0);
		}
		bzero((char *) ctrl_p, sizeof(struct hd_ctrl));
		hd_ctrl[ctrl] = ctrl_p;
		ctrl_p->address = addr;
		take_ctlr_irq(ctlr);
		ctrl_p->state.curdrive = ctrl<<1;
		printf("%s%d: port = %x, spl = %d, pic = %d.\n",
		       ctlr->name, ctlr->unit,
		       ctlr->address, ctlr->sysdep, ctlr->sysdep1);
#if	0
		/* may be necesary for two controllers */
		outb(FIXED_DISK_REG(ctrl), 4);
		for(i = 0; i < 10000; i++);
		outb(FIXED_DISK_REG(ctrl), 0);
#endif
#if	NCPUS > 1 && MBUS
		simple_lock_init(&ctrl_p->lock.lock, ETAP_IO_HD_PROBE);
		ctrl_p->lock.unit = ctrl;
		ctrl_p->lock.pending_ops = 0;
		ctrl_p->lock.op[MP_DEV_OP_START] = hdstart;
		ctrl_p->lock.op[MP_DEV_OP_INTR] = hdintr;
#endif	/* NCPUS > 1 && MBUS */
		return(1);
	}
	return(0);
}

/*
 * hdslave:
 *
 *	Actually should be thought of as a slave probe.
 *
 */

int
hdslave(
	struct bus_device	*dev,
	caddr_t			xxx)
{
	int	i,
		addr = hd_ctrl[dev->ctlr]->address;
	u_char	*bios_magic = (u_char *)phystokv(0x475);

	if (dev->ctlr == 0)	/* for now: believe DOS */
		if (*bios_magic >= 1 + dev->slave)
			return 1;
		else
			return 0;
	else
		return 1;
		
#if	0
	/* it is working on all types of PCs */
	outb(PORT_DRIVE_HEADREGISTER(addr),dev->slave<<4);
	outb(PORT_COMMAND(addr),CMD_RESTORE);

	for (i=350000; i && inb(PORT_STATUS(addr))&STAT_BUSY; i--);
	if (i == 0) {
		outb(FIXED_DISK_REG(dev->ctlr), 4);
		for(i = 0; i < 10000; i++);
		outb(FIXED_DISK_REG(dev->ctlr), 0);
		setcontroller(dev->slave);
		return 0;
	}
	return(i&STAT_READY);
#endif
}

/*
 * hdattach:
 *
 *	Attach the drive unit that has been successfully probed.  For the
 *	AT ESDI drives we will initialize all driver specific structures
 *	and complete the controller attach of the drive.
 *
 */

void
hdattach(
	struct bus_device	*dev)
{
	int 		unit = dev->unit;
	dev_ops_t 	hd_dev_ops;
	int		tmpunit;
	hd_drive_t	drive;

	drive =(hd_drive_t) kalloc(sizeof(struct hd_drive));
	if (!drive) {
		printf("hd_attach: kalloc failed\n");
		return;
	}
	hd_drive[unit] = drive;
	bzero((char *)drive, sizeof(struct hd_drive));
	simple_lock_init(&drive->io_queue.io_req_lock, ETAP_IO_REQ);
	if (dev_name_lookup("hd", &hd_dev_ops, &tmpunit)) {
		char devname[16];
		extern int disk_indirect_count;

		strcpy(devname, "disk");
		itoa(disk_indirect_count, devname + 4);
		dev_set_indirection(devname,
				    hd_dev_ops,
				    unit * hd_dev_ops->d_subdev);
		disk_indirect_count++;
	}
	hd_read_id(dev, unit);
	setcontroller(unit);
	return;
}

io_return_t
hdopen(
	dev_t		dev,
	dev_mode_t	flag,
	io_req_t	ior)
{
	u_char	unit = UNIT(dev),
		part = PARTITION(dev);

	hd_drive_t	drive;
	struct label_partition *part_p;

	if (!(drive = hd_drive[UNIT(dev)]))
		return(D_NO_SUCH_DEVICE);

	if (part >= V_NUMPAR)
		return(D_NO_SUCH_DEVICE);
#ifdef	DISKTEST
	if (NULLPARTITION(part)) 
		return(D_SUCCESS);
#endif	/* DISKTEST */
	if (!getvtoc(dev))
		return(D_NO_SUCH_DEVICE);
	part_p = &drive->label.d_partitions[part];
	if (part_p->p_size <= 0)
		return(D_NO_SUCH_DEVICE);
	return(D_SUCCESS);
}

void
hdclose(
	dev_t		dev)
{
}

void
hdminphys(
	struct buf	*bp)
{
	if (bp->b_bcount > SECLIMIT*SECSIZE)
		bp->b_bcount = SECLIMIT*SECSIZE;
}

io_return_t
hdread(
	dev_t		dev,
	io_req_t 	ior)
{
	hd_read_count++;
	return (block_io(hdstrategy, hdminphys, ior));
}

io_return_t
hdwrite(
	dev_t		dev,
	io_req_t	ior)
{
	hd_write_count++;
	return (block_io(hdstrategy, hdminphys, ior));
}

int abs_sec   = -1;
int abs_count = -1;

/* IOC_OUT only and not IOC_INOUT */
io_return_t
hdgetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,	/* pointer to OUT array */
	natural_t	*count)	/* OUT */
{
	int		unit = UNIT(dev);
	struct buf	*bp1;
	int		i;
	hd_drive_t	drive = hd_drive[unit];
	partition_t	part_p = &drive->label.d_partitions[PARTITION(dev)];

	switch (flavor) {

	/* Mandatory flavors */

	case DEV_GET_SIZE: {
		int 		part = PARTITION(dev);
		int 		size;
		
		data[DEV_GET_SIZE_DEVICE_SIZE] = part_p->p_size * SECSIZE;
		data[DEV_GET_SIZE_RECORD_SIZE] = SECSIZE;
		*count = DEV_GET_SIZE_COUNT;
		break;
	}

	/* Extra flavors */

	case V_GETPARMS: {
		struct disk_parms 	*dp;
		int 			part = PARTITION(dev);
		hdisk_t 		*parm;

		if (*count < sizeof (struct disk_parms)/sizeof(int))
			return (D_INVALID_OPERATION);
		dp = (struct disk_parms *) data;
		dp->dp_type = DPT_WINI;
		dp->dp_heads = hd_drive[unit]->label.d_ntracks;
		dp->dp_cyls = hd_drive[unit]->label.d_ncylinders;
		dp->dp_sectors  = hd_drive[unit]->label.d_nsectors;
  		dp->dp_dosheads = hd_drive[unit]->cmos_parm.nheads;
		dp->dp_doscyls = hd_drive[unit]->cmos_parm.ncyl;
		dp->dp_dossectors  = hd_drive[unit]->cmos_parm.nsec;
		dp->dp_secsiz = SECSIZE;
		dp->dp_ptag = 0;
		dp->dp_pflag = 0;
		dp->dp_pstartsec = part_p->p_offset;
		dp->dp_pnumsec = part_p->p_size;
		*count = sizeof(struct disk_parms)/sizeof(int);
		break;
	}
	case V_RDABS: {
		/* V_RDABS is relative to head 0, sector 0, cylinder 0 */
		if (*count < SECSIZE/sizeof (int)) {
			printf("hd%d: RDABS bad size %x", unit, count);
			return (D_INVALID_OPERATION);
		}
		if (hd_rw_abs(dev, data, IO_READ, 
				abs_sec, SECSIZE) != D_SUCCESS)
			return(D_INVALID_OPERATION);
		*count = SECSIZE/sizeof(int);
		break;
	}
	case V_VERIFY: {
		int cnt = abs_count * SECSIZE;
		int sec = abs_sec;
                int bsize = PAGE_SIZE;
		char *verify_buf;

		(void) kmem_alloc(kernel_map,
				  (vm_offset_t *)&verify_buf,
				  bsize);

		*data = 0;
		while (cnt > 0) {
			int xcount = (cnt < bsize) ? cnt : bsize;
			if (hd_rw_abs(dev, (dev_status_t)verify_buf,
					IO_READ, sec, xcount) != D_SUCCESS) {
				*data = BAD_BLK;
				break;
			} else {
				cnt -= xcount;
				sec += xcount / SECSIZE;
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
hdsetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,
	natural_t	count)
{
	struct buf	*bp1;
	io_return_t	errcode = D_SUCCESS;
	int		unit = UNIT(dev);
	hd_drive_t	drive = hd_drive[unit];
	partition_t	part_p = &drive->label.d_partitions[PARTITION(dev)];

	switch (flavor) {
	case V_REMOUNT:
		(void) getvtoc(dev);
		break;
	case V_ABS:
		abs_sec = *(int *)data;
		if (count == 2)
			abs_count = data[1];
		break;
	case V_WRABS:
		/* V_WRABS is relative to head 0, sector 0, cylinder 0 */
		if (count < SECSIZE/sizeof (int)) {
			printf("hd%d: WRABS bad size %x", unit, count);
			return (D_INVALID_OPERATION);
		}
		if (hd_rw_abs(dev, data, IO_WRITE, 
			       abs_sec, SECSIZE) != D_SUCCESS)
			return(D_INVALID_OPERATION);
		break;
	default:
		return (D_INVALID_OPERATION);
	}
	return (errcode);
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

#define can_chain_io_reqs(a, b)						\
  	(IO_OP(a->io_op) == IO_OP(b->io_op) &&				\
	 (a->io_unit == b->io_unit) &&					\
	 (a->io_recnum + ((a->io_count + 511) >> 9) == b->io_recnum ||	\
	  b->io_recnum + ((b->io_count + 511) >> 9) == a->io_recnum) && \
	 (a->io_count + b->io_count <= SECLIMIT*SECSIZE))
	 
#endif /* CHAINED_IOS */

void
hdstrategy(
	struct buf	*bp)
{
	dev_t		dev = bp->b_dev;
	u_char		unit = UNIT(dev),
			ctrl = unit>>1,
			part = PARTITION(dev);
	u_int		opri;
	hdisk_t		*parm;
	hd_drive_t	drive = hd_drive[unit];
	partition_t	part_p;
	int 		size;
	int		start;

#ifdef	DISKTEST
	if (NULLPARTITION(PARTITION(dev))) {
		bp->b_resid = 0;
		goto done;
	}
#endif	/* DISKTEST */

	
	if (!bp->b_bcount)
		goto done;

	/* if request is off the end or trying to write last block on out */
	if (bp->b_flags & B_MD1) {
		part_p  = &drive->label.d_partitions[MAXPARTITIONS];
	} else {
		part_p  = &drive->label.d_partitions[PARTITION(dev)];
	}
	if (part_p->p_size <= 0) {
		bp->b_error = ENXIO;
		goto bad;
	}
	size = part_p->p_size;
	start = part_p->p_offset;

	if ((bp->b_blkno > size) ||
	    (bp->b_blkno == size && !(bp->b_flags & B_READ))) {
	  	bp->b_error = ENXIO;
		goto bad;
	}
	if (bp->b_blkno == size) {
		/* indicate (read) EOF by setting b_resid
		   to b_bcount on last block */ 
	  	bp->b_resid = bp->b_bcount;
		goto done;
	}

	bp->b_cylin = (start + bp->b_blkno) / 
	              (drive->label.d_nsectors * drive->label.d_ntracks );
	opri = splbio();
	simple_lock(&drive->io_queue.io_req_lock);

	disksort(&drive->io_queue, bp);
	if (++drive->io_queue.io_total > hd_queue_length)
		hd_queue_length = drive->io_queue.io_total;
#if CHAINED_IOS
		if (!(bp->io_op & IO_SGLIST)) {
		  	bp->io_seg_count = 1;
			if (bp->io_prev && 
			    (bp->io_prev != drive->io_queue.b_actf ||
			     !hd_ctrl[ctrl]->state.controller_busy) &&
			    can_chain_io_reqs(bp->io_prev, bp)) {
				chain_io_reqs(bp->io_prev, bp,
					      &drive->io_queue);
				drive->io_queue.io_total--;
			}
			if (bp->io_next &&
			    can_chain_io_reqs(bp, bp->io_next)) {
				chain_io_reqs(bp, bp->io_next,
					      &drive->io_queue);
				drive->io_queue.io_total--;
			}
		}
#endif /* CHAINED_IOS */
	simple_unlock(&drive->io_queue.io_req_lock);
	if (!hd_ctrl[ctrl]->state.controller_busy)
		hdstart(ctrl);
	splx(opri);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
#if CHAINED_IOS
	if (bp->io_op & IO_CHAINED) {
		chained_iodone(bp);
	}
	else 
#endif	/* CHAINED_IOS */
		iodone(bp);
	return;
}

/* hdstart() is called at spl5 */
void
hdstart(
	int		ctrl)
{
	register struct buf	*bp;
	int                     i;
	partition_t		part_p;
	hd_ctrl_t		ctrl_p = hd_ctrl[ctrl];
	hd_drive_t		drive;
	int 			start, size;

	at386_io_lock_state();

	if (!at386_io_lock(MP_DEV_OP_START)) 
	  	return;
	if (ctrl_p->state.controller_busy)
		goto done;

	/* things should be quiet */
	if (i = ctrl_p->need_set_controller) {
		if (i&1) set_controller(ctrl<<1);
		if (i&2) set_controller((ctrl<<1)||1);
		ctrl_p->need_set_controller= 0;
	}
	if ((drive = hd_drive[ctrl_p->state.curdrive^1]) &&
	    (bp = drive->io_queue.b_actf))
		ctrl_p->state.curdrive^=1;
	else {
		drive = hd_drive[ctrl_p->state.curdrive];
		if (!(bp = drive->io_queue.b_actf))
			goto done;
	}
	ctrl_p->state.controller_busy = 1;
	ctrl_p->state.blocktotal = (bp->b_bcount + 511) >> 9;
	/* see V_RDABS and V_WRABS in hdioctl() */
	if (bp->b_flags & B_MD1) {
		part_p = &drive->label.d_partitions[MAXPARTITIONS];
	} else {
		part_p = &drive->label.d_partitions[PARTITION(bp->b_dev)];
	}
	size = part_p->p_size;
	start = part_p->p_offset;

 	ctrl_p->state.physblock = start + bp->b_blkno;
	if ((bp->b_blkno + ctrl_p->state.blocktotal) > size)
	  	ctrl_p->state.blocktotal = size - bp->b_blkno;
	ctrl_p->state.blockcount = 0;
	ctrl_p->state.rw_addr = (int)bp->b_un.b_addr;
	ctrl_p->state.retry_count = 0;
	start_rw(bp->b_flags & B_READ, ctrl);
done:
	at386_io_unlock();
}

#if	MACH_ASSERT
int	hd_debug = 1;
#else	/* MACH_ASSERT */
int	hd_debug = 0;
#endif  /* MACH_ASSERT */

void
hd_dump_registers(
	int ctrl)
{
	i386_ioport_t	base = hd_ctrl[ctrl]->address;

	if (!hd_debug)
		return;
	printf("Controller registers:\n");
	printf("Status Register: 0x%x\n", inb(PORT_STATUS(base)));
	waitcontroller(ctrl);
	printf("Error Register: 0x%x\n", inb(PORT_ERROR(base)));
	printf("Sector Count: 0x%x\n", inb(PORT_NSECTOR(base)));
	printf("Sector Number: 0x%x\n", inb(PORT_SECTOR(base)));
	printf("Cylinder High: 0x%x\n", inb(PORT_CYLINDERHIBYTE(base)));
	printf("Cylinder Low: 0x%x\n", inb(PORT_CYLINDERLOWBYTE(base)));
	printf("Drive/Head Register: 0x%x\n",
			inb(PORT_DRIVE_HEADREGISTER(base)));
}

int hd_print_error = 0;
void
hdintr(
	int		ctrl)
{
	register struct buf	*bp;
	hd_ctrl_t		ctrl_p = hd_ctrl[ctrl];
	int			addr = ctrl_p->address,
				unit = ctrl_p->state.curdrive;
	u_char status;
	at386_io_lock_state();

	if (!at386_io_lock(MP_DEV_OP_INTR))
		return;
	if (!ctrl_p->state.controller_busy) {
		printf("HD: false interrupt\n");
		hd_dump_registers(ctrl);
		goto done;
	}
	if (inb(PORT_STATUS(addr)) & STAT_BUSY) {
		printf("hdintr: interrupt w/controller not done.\n");
	}
	waitcontroller(ctrl);
	status = inb(PORT_STATUS(addr));
	bp = hd_drive[unit]->io_queue.b_actf;
	if (!bp) {
		/* there should be a read/write buffer queued at this point */
		printf("hd%d: no bp buffer to read or write\n",unit);
		goto done;
	}
	if (ctrl_p->state.restore_request) { /* Restore command has completed */
		ctrl_p->state.restore_request = 0;
		if (status & STAT_ERROR)
			hderror(bp,ctrl);
		else if (bp)
			start_rw(bp->b_flags & B_READ, ctrl);
		goto done;
	}
	if (status & STAT_WRITEFAULT) {
		printf("hdintr: write fault. status 0x%X\n", status);
		printf("hdintr: write fault. block %d, count %d, total %d\n",
		       ctrl_p->state.physblock,
		       ctrl_p->state.blockcount,
		       ctrl_p->state.blocktotal);
		printf("hdintr: write fault. cyl %d, head %d, sector %d\n",
		       ctrl_p->state.cylinder,
		       ctrl_p->state.head,
		       ctrl_p->state.sector);
		hd_dump_registers(ctrl);
		panic("hd: write fault\n");
	}
	if (status & STAT_ERROR) {
		if (hd_print_error) {
			 printf("hdintr: state error %x, error = %x\n",
			 	status, inb(PORT_ERROR(addr)));
			 printf("hdintr: state error. block %d, count %d, total %d\n",
			 	ctrl_p->state.physblock,
				ctrl_p->state.blockcount,
				ctrl_p->state.blocktotal);
			 printf("hdintr: state error. cyl %d, head %d, sector %d\n",
			 	ctrl_p->state.cylinder,
				ctrl_p->state.head,
				ctrl_p->state.sector);

		}
		hderror(bp,ctrl);
		goto done;
	}
	if (status & STAT_ECC) 
		printf("hd: ECC soft error fixed, unit %d, partition %d, physical block %d \n",
			unit, PARTITION(bp->b_dev), ctrl_p->state.physblock);
	if (bp->b_flags & B_READ) {
		while (!(inb(PORT_STATUS(addr)) & STAT_DATAREQUEST));
		linw(PORT_DATA(addr), (int *)ctrl_p->state.rw_addr, SECSIZE/2); 
	}
	if (++ctrl_p->state.blockcount == ctrl_p->state.blocktotal) {
		simple_lock(&hd_drive[unit]->io_queue.io_req_lock);
		hd_drive[unit]->io_queue.b_actf = hd_drive[unit]->io_queue.b_actf->av_forw;
		hd_drive[unit]->io_queue.io_total--;
		simple_unlock(&hd_drive[unit]->io_queue.io_req_lock);
		bp->b_resid = bp->b_bcount - (ctrl_p->state.blocktotal << 9);
		if (bp->b_resid < 0) 
			bp->b_resid = 0;
#if CHAINED_IOS
		if (bp->io_op & IO_CHAINED) {
			chained_iodone(bp);
		} else 
#endif	/* CHAINED_IOS */
			iodone(bp);
		ctrl_p->state.controller_busy = 0;
		hdstart(ctrl);
	} else {
#if CHAINED_IOS 
	        /*
		 * Look for next phys address. Not necessarily contiguous
		 */
		io_req_t next;

	  	if ((bp->io_op & IO_CHAINED) && (next = bp->io_link)) {
		     int count;
		     int resid = ctrl_p->state.blocktotal -
		                 ctrl_p->state.blockcount;
		     for (;;) {
		     	 assert(next);
		         count = (next->io_count + 511) >> 9;
			 if (count == resid) {
			     ctrl_p->state.rw_addr = 
				     (int)next->b_un.b_addr;
			     break;
			 } else if (count < resid ||
				    !(next = next->io_link)) {
			     ctrl_p->state.rw_addr += SECSIZE;
			     break;
			 }
		    }

		} else
#endif /* CHAINED_IOS */
			ctrl_p->state.rw_addr += SECSIZE;
		ctrl_p->state.physblock++;
		if (ctrl_p->state.single_mode)
			start_rw(bp->b_flags & B_READ, ctrl);
		else if (!(bp->b_flags & B_READ)) {
			/* Load sector into controller for next write */
			while (!(inb(PORT_STATUS(addr)) & STAT_DATAREQUEST));
			loutw(PORT_DATA(addr), (int *)ctrl_p->state.rw_addr,
				SECSIZE/2);
		}
	}
done:
	at386_io_unlock();
}

void
hderror(
	struct buf	*bp,
	int 		ctrl)
{
	hd_ctrl_t	ctrl_p = hd_ctrl[ctrl];
	int		addr = ctrl_p->address,
			unit = ctrl_p->state.curdrive;
	hdisk_t		*parm;
	
	parm = &hd_drive[unit]->cmos_parm;
	if(++ctrl_p->state.retry_count > 3) {
		if(bp) {
			int i;
			/****************************************************
			* We have a problem with this block, set the error  *
			* flag, terminate the operation and move on to the  *
			* next request.  With every hard disk transaction   *
			* error we set the reset requested flag so that the *
			* controller is reset before the next operation is  *
			* started.					    *
			****************************************************/
#if CHAINED_IOS
			if (bp->io_op & IO_CHAINED) {
			    /*
			     * Since multiple IOs are chained, split them
			     * and restart prior to error handling
			     */
			    simple_lock(&hd_drive[unit]->io_queue.io_req_lock);
			    split_io_reqs(bp);
			    hd_drive[unit]->io_queue.io_total++;
			    bp->b_resid = 0;
			    simple_unlock(&hd_drive[unit]->io_queue.io_req_lock);
			} else 
#endif	/* CHAINED_IOS */
			{
			    simple_lock(&hd_drive[unit]->io_queue.io_req_lock);
			    hd_drive[unit]->io_queue.b_actf =
			         hd_drive[unit]->io_queue.b_actf->av_forw;
			    hd_drive[unit]->io_queue.io_total--;
			    simple_unlock(&hd_drive[unit]->io_queue.io_req_lock);
			    bp->b_flags |= B_ERROR;
			    bp->b_resid = 0;
			    iodone(bp);
			}
			outb(FIXED_DISK_REG(ctrl), 4);
			for(i = 0; i < 10000; i++);
			outb(FIXED_DISK_REG(ctrl), 0);
			setcontroller(unit);
			ctrl_p->state.controller_busy = 0;
			hdstart(ctrl);
		}
	}
	else {
		/* lets do a recalibration */
		waitcontroller(ctrl);
		ctrl_p->state.restore_request = 1;
		outb(PORT_PRECOMP(addr), parm->precomp>>2);
		outb(PORT_NSECTOR(addr), parm->nsec);
		outb(PORT_SECTOR(addr), ctrl_p->state.sector);
		outb(PORT_CYLINDERLOWBYTE(addr),
		     ctrl_p->state.cylinder & 0xff);
		outb(PORT_CYLINDERHIBYTE(addr),
		     (ctrl_p->state.cylinder>>8) & 0xff);
		outb(PORT_DRIVE_HEADREGISTER(addr), (unit&1)<<4);
		outb(PORT_COMMAND(addr), CMD_RESTORE);
	}
}

void
set_controller(
	int		unit)
{
	int	ctrl = unit >> 1;
	int	addr = hd_ctrl[ctrl]->address;
	hdisk_t	*parm;
	
	waitcontroller(ctrl);
	outb(PORT_DRIVE_HEADREGISTER(addr),
	     (hd_drive[unit]->label.d_ntracks - 1) |
	     ((unit&1) << 4) | FIXEDBITS);
	outb(PORT_NSECTOR(addr), hd_drive[unit]->label.d_nsectors);
	outb(PORT_COMMAND(addr), CMD_SETPARAMETERS);
	waitcontroller(ctrl);
}

void
waitcontroller(
	int		ctrl)
{
	u_int	n = PATIENCE;

	while (--n && inb(PORT_STATUS(hd_ctrl[ctrl]->address)) & STAT_BUSY);
	if (n)
		return;
	panic("hd: waitcontroller() timed out");
}

void
start_rw(
	int	read,
	int	ctrl)
{
	u_int			track, disk_block, xblk;
	hd_ctrl_t		ctrl_p = hd_ctrl[ctrl];
	int			addr = ctrl_p->address,
				unit = ctrl_p->state.curdrive;
	hd_drive_t		drive = hd_drive[unit];
	struct disklabel	*label = &drive->label;
	
	disk_block = ctrl_p->state.physblock;
	/*# blks to transfer*/
	xblk=ctrl_p->state.blocktotal - ctrl_p->state.blockcount;
	if (!(drive->io_queue.b_actf->b_flags & B_MD1) &&
	    (ctrl_p->state.single_mode = xfermode(ctrl))) {
		xblk = 1;
		if (PARTITION(drive->io_queue.b_actf->b_dev) != PART_DISK)
			disk_block = badblock_mapping(ctrl);
	}
	/* disk is formatted starting sector 1, not sector 0 */
	ctrl_p->state.sector = (disk_block % label->d_nsectors) + 1;
	track = disk_block / label->d_nsectors;
	ctrl_p->state.head = track % label->d_ntracks | 
		(unit&1) << 4 | FIXEDBITS;
	ctrl_p->state.cylinder = track / label->d_ntracks ;
	waitcontroller(ctrl);
	outb(PORT_PRECOMP(addr), drive->cmos_parm.precomp >>2);
	outb(PORT_NSECTOR(addr), xblk);
	outb(PORT_SECTOR(addr), ctrl_p->state.sector);
	outb(PORT_CYLINDERLOWBYTE(addr), ctrl_p->state.cylinder & 0xff );
	outb(PORT_CYLINDERHIBYTE(addr), (ctrl_p->state.cylinder >> 8) & 0xff );
	outb(PORT_DRIVE_HEADREGISTER(addr), ctrl_p->state.head);
	if(read) {
		outb(PORT_COMMAND(addr), CMD_READ);
	} else {
 		outb(PORT_COMMAND(addr), CMD_WRITE);
		waitcontroller(ctrl);
		while (!(inb(PORT_STATUS(addr)) & STAT_DATAREQUEST));
		loutw(PORT_DATA(addr),
		      (int *)ctrl_p->state.rw_addr, SECSIZE/2);
	}
}

int badblock_mapping(
	int	ctrl)
{
	u_short		n;
	u_int		track,
			unit = hd_ctrl[ctrl]->state.curdrive,
			block = hd_ctrl[ctrl]->state.physblock,
			nsec;
	hd_drive_t	drive = hd_drive[unit];
	struct alt_info *alt_info = drive->alt_info;;

	if (!alt_info)
		return(block);

	nsec = hd_drive[unit]->label.d_nsectors;

	track = block / nsec;
	for (n = 0; n < alt_info->alt_trk.alt_used; n++)
		if (track == alt_info->alt_trk.alt_bad[n])
			return alt_info->alt_trk.alt_base +
			       nsec * n + (block % nsec);
	/* BAD BLOCK MAPPING */
	for (n = 0; n < alt_info->alt_sec.alt_used; n++)
		if (block == alt_info->alt_sec.alt_bad[n])
			return alt_info->alt_sec.alt_base + n;
	return block;
}

/*
 *  determine single block or multiple blocks transfer mode
 */
int
xfermode(
	int	ctrl)
{
	int		n, bblk, eblk, btrk, etrk;
	hd_ctrl_t	ctrl_p = hd_ctrl[ctrl];
	int		unit = ctrl_p->state.curdrive;
	hd_drive_t	drive = hd_drive[unit];
	struct alt_info *alt_info = drive->alt_info;;

	if (!alt_info)
		return 0;

	bblk = ctrl_p->state.physblock;
	eblk = bblk + ctrl_p->state.blocktotal - 1;
	btrk = bblk / drive->label.d_nsectors;
	etrk = eblk / drive->label.d_nsectors;
	
	for (n = 0; n < alt_info->alt_trk.alt_used; n++)
		if ((btrk <= alt_info->alt_trk.alt_bad[n]) &&
		    (etrk >= alt_info->alt_trk.alt_bad[n]))
			return 1;
	for (n = 0; n < alt_info->alt_sec.alt_used; n++)
		if ((bblk <= alt_info->alt_sec.alt_bad[n]) &&
		    (eblk >= alt_info->alt_sec.alt_bad[n]))
			return 1;
	return 0;
}

/*
 *	Routine to return information to kernel.
 */
io_return_t
hddevinfo(
	dev_t		dev,
	dev_flavor_t	flavor,
	char		*info)
{
	register int	result;

	result = D_SUCCESS;

	switch (flavor) {
	case D_INFO_BLOCK_SIZE:
		*((int *) info) = SECSIZE;
		break;
	default:
		result = D_INVALID_OPERATION;
	}

	return(result);
}

boolean_t
getvtoc(
	int			dev)
{
	hd_drive_t		drive = hd_drive[UNIT(dev)];
	struct disklabel	*label = &drive->label;
	int			partition_index = 0;

	/* 
	 * Setup last partition that we can access entire physical disk
	 */

	label->d_secsize = SECSIZE;
	label->d_partitions[MAXPARTITIONS].p_size = -1;
	label->d_partitions[MAXPARTITIONS].p_offset = 0;


	if (read_bios_partitions(dev,
			0,			/* sector 0 */
			0,			/* ext base */
			&partition_index,	/* first partition */
			label,
			&drive->alt_info,	/* alt_info ** */
			hd_rw_abs) == D_SUCCESS) {
	  	setcontroller(UNIT(dev));
	} else {
		/* make partition 'c' the whole disk in case of failure */
		label->d_partitions[PART_DISK].p_offset = 0;
		label->d_partitions[PART_DISK].p_size =
		  	drive->cmos_parm.ncyl *
			drive->cmos_parm.nheads * 
			drive->cmos_parm.nsec;
	}
	return(TRUE);
}

io_return_t
hd_rw_abs(
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
	hdstrategy(ior);
	iowait(ior);
	error = ior->io_error;
	io_req_free(ior);
	return(error);
}

void hd_read_id (
	struct bus_device	*dev,
	int			unit)
{
	int		ctrl = unit >> 1;
	hd_ctrl_t	ctrl_p = hd_ctrl[ctrl];
	int		addr = ctrl_p->address;
	hd_id_t		id;
	register 	i;
	u_long		n;
	u_char		*tbl;
	hdisk_t		parm;

	waitcontroller(ctrl);
	ctrl_p->state.restore_request = 1;
	outb(PORT_PRECOMP(addr), 0);
	outb(PORT_NSECTOR(addr), 0);
	outb(PORT_SECTOR(addr), 0);
	outb(PORT_CYLINDERLOWBYTE(addr), 0);
	outb(PORT_CYLINDERHIBYTE(addr), 0);
	outb(PORT_DRIVE_HEADREGISTER(addr), (unit&1)<<4);
	outb(PORT_COMMAND(addr), CMD_IDENTIFY);
	waitcontroller(ctrl);
	linw(PORT_DATA(addr), (int *)&id, sizeof(id)/2);

	n = *(unsigned long *)phystokv(dev->address);
	tbl = (unsigned char *)phystokv((n&0xffff) + ((n>>12)&0xffff0));

	parm.ncyl   = *(unsigned short *)tbl;
	parm.nheads = *(unsigned char  *)(tbl+2);
	parm.nsec   = *(unsigned char  *)(tbl+14);

	parm.precomp= *(unsigned short *)(tbl+5);
	hd_drive[unit]->cmos_parm = parm;
	if (id.val_cur_values & 1) {
		hd_drive[unit]->label.d_nsectors = id.cur_secs;
		hd_drive[unit]->label.d_ntracks = id.cur_heads;
		hd_drive[unit]->label.d_ncylinders = id.cur_cyls;
	} else {
		hd_drive[unit]->label.d_nsectors = parm.nsec;
		hd_drive[unit]->label.d_ntracks = parm.nheads;
		hd_drive[unit]->label.d_ncylinders = parm.ncyl;
	}
	printf(", stat = %x, spl = %d, pic = %d\n",
		dev->address, dev->sysdep, dev->sysdep1);
	if (unit < 2 || (id.val_cur_values & 1))
		printf(" hd%d: %d Meg, C:%d H:%d S:%d - ",
		       unit,
		       parm.ncyl*parm.nheads*parm.nsec * 512/1000000,
		       parm.ncyl,
		       parm.nheads,
		       parm.nsec);
	else
		printf("hd%d:   Capacity not available through bios\n",unit);

	/* model is big endian byte ordered */
	for (i=0; i < sizeof(id.model); i +=2) {
		unsigned char c = id.model[i];
		id.model[i] = id.model[i+1];
		id.model[i+1] = c;
	}
	id.model[sizeof(id.model)-1] = 0;
		
	printf("%s", id.model);

}
