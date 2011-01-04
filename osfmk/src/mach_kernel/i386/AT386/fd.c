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
 * Revision 2.6.2.3  92/04/30  11:55:03  bernadat
 * 	Initialized sector size in fd_getparms
 * 	[92/04/28            bernadat]
 * 
 * 	Program DMA registers for 32 bits addresses and 24 bits count
 * 	on Compaq Systempro.
 * 	[92/04/16            bernadat]
 * 
 * Revision 2.6.2.2  92/03/28  10:06:06  jeffreyh
 * 	Adaptations for Corollary:
 * 		Use of windows for memory above 16 Megs
 * 		Switch to Master CPU for IO regs access
 * 	[92/03/04            bernadat]
 * 	Changes from TRUNK
 * 	[92/03/10  13:21:43  jeffreyh]
 * 
 * Revision 2.8  92/02/23  22:42:57  elf
 * 	Added (mandatory) DEV_GET_SIZE flavor of get_status.
 * 	[92/02/22            af]
 * 
 * Revision 2.7  92/01/27  16:42:54  rpd
 * 	Fixed fdgetstat and fdsetstat to return D_INVALID_OPERATION
 * 	for unsupported flavors.
 * 	[92/01/26            rpd]
 * 
 * Revision 2.6  91/11/12  11:09:18  rvb
 * 	Amazing how hard getting the probe to work for all machines is.
 * 	[91/10/16            rvb]
 * 
 * Revision 2.5  91/10/07  17:25:22  af
 * 	Still better
 * 	[91/10/07  16:29:57  rvb]
 * 
 * 	From mg32: Better probe for multiple controllers now possible.
 * 	[91/09/23            rvb]
 * 	New chips/busses.[ch] nomenclature.
 * 	[91/09/09  17:12:23  rvb]
 * 
 * 	Added a reset in open to prevent "no such device" errors
 * 	Added dlb's fddevinfo.
 * 	Reworked to make 2.5/3.0 compatible
 * 	[91/09/04  15:46:49  rvb]
 * 
 *	Major rewrite by mg32.
 * 	[91/08/07            mg32]
 * 
 * Revision 2.4  91/08/24  11:57:32  af
 * 	New MI autoconf.
 * 	[91/08/02  02:53:26  af]
 * 
 * Revision 2.3  91/05/14  16:22:47  mrt
 * 	Correcting copyright
 * 
 * Revision 2.2  91/02/14  14:42:23  mrt
 * 	This file is the logical contatenation of the previous c765.c,
 * 	m765knl.c and m765drv.c, in that order.  
 * 	[91/01/15            rvb]
 * 
 * Revision 2.5  91/01/08  17:33:32  rpd
 * 	Add some 3.0 get/set stat stuff.
 * 	[91/01/04  12:21:06  rvb]
 * 
 * Revision 2.4  90/11/26  14:50:54  rvb
 * 	jsb bet me to XMK34, sigh ...
 * 	[90/11/26            rvb]
 * 	Synched 2.5 & 3.0 at I386q (r1.6.1.6) & XMK35 (r2.4)
 * 	[90/11/15            rvb]
 * 
 * Revision 1.6.1.6  90/11/27  13:44:55  rvb
 * 	Synched 2.5 & 3.0 at I386q (r1.6.1.6) & XMK35 (r2.4)
 * 	[90/11/15            rvb]
 * 
 * Revision 2.3  90/08/27  22:01:22  dbg
 * 	Remove include of device/param.h (unnecessary).  Flush ushort.
 * 	[90/07/17            dbg]
 * 
 * Revision 1.6.1.5  90/08/25  15:44:31  rvb
 * 	Use take_<>_irq() vs direct manipulations of ivect and friends.
 * 	[90/08/20            rvb]
 * 
 * Revision 1.6.1.4  90/07/27  11:26:53  rvb
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[90/07/27            rvb]
 * 
 * Revision 1.6.1.3  90/07/10  11:45:11  rvb
 * 	New style probe/attach.
 * 	NOTE: the whole probe/slave/attach is a crock.  Someone
 * 	who spends the time to understand the driver should do
 * 	it right.
 * 	[90/06/15            rvb]
 * 
 * Revision 2.2  90/05/03  15:45:37  dbg
 * 	Convert for pure kernel.
 *	Optimized fd_disksort iff dp empty.
 * 	[90/04/19            dbg]
 * 
 * Revision 1.6.1.2  90/01/08  13:30:14  rvb
 * 	Add Intel copyright.
 * 	[90/01/08            rvb]
 * 
 * Revision 1.6.1.1  89/10/22  11:34:51  rvb
 * 	Received from Intel October 5, 1989.
 * 	[89/10/13            rvb]
 * 
 * Revision 1.6  89/09/25  12:27:05  rvb
 * 	vtoc.h -> disk.h
 * 	[89/09/23            rvb]
 * 
 * Revision 1.5  89/09/09  15:23:15  rvb
 * 	Have fd{read,write} use stragegy now that physio maps correctly.
 * 	[89/09/06            rvb]
 * 
 * Revision 1.4  89/03/09  20:07:26  rpd
 * 	More cleanup.
 * 
 * Revision 1.3  89/02/26  12:40:28  gm0w
 * 	Changes for cleanup.
 *
 */
/* CMU_ENDHIST */

/* 
 * Mach Operating System
 * Copyright (c) 1992,1991,1990,1989 Carnegie Mellon University
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
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*	Copyright (c) 1987, 1988 TOSHIBA Corp.		*/
/*		All Rights Reserved			*/

#include <fd.h>
#include <platforms.h>
#include <cpus.h>
#include <eisa.h>
#include <himem.h>

#include <types.h>
#include <kern/spl.h>
#include <device/buf.h>
#include <device/conf.h>
#include <device/errno.h>
#include <device/dev_master.h>
#include <device/ds_routines.h>
#include <device/misc_protos.h>
#include <sys/ioctl.h>
#include <i386/pio.h>
#include <chips/busses.h>
#include <i386/AT386/fdreg.h>
#include <i386/AT386/fd_entries.h>
#include <i386/AT386/misc_protos.h>
#include <machine/disk.h>
#include <i386/AT386/eisa.h>
#include <i386/AT386/himem.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#ifdef	CBUS
#include <busses/cbus/cbus.h>
#endif	/* CBUS */

#ifdef	DEBUG
#define D(x) x
#else	/* DEBUG */
#define D(x)
#endif	/* DEBUG */

/* Forward */

extern int		fdprobe(
				int			ctrl,
				struct bus_ctlr		* bc);
extern int		fdslave(
				struct bus_device	* bd,
				caddr_t			xxx);
extern void		fdattach(
				struct bus_device	* bd);
extern void		fdstrategy(
				struct buf		* bp);
extern void		fdminphys(
				struct buf		* bp);
extern void		setqueue(
				struct buf		* bp,
				struct unit_info	* uip);
extern void		chkbusy(
				struct fdcmd		* cmdp);
extern void		openchk(
				struct fdcmd		* cmdp);
extern void		openfre(
				struct fdcmd		* cmdp);
extern void		m765io(
				struct unit_info	* uip);
extern void		m765iosub(
				struct unit_info	* uip);
extern void		rwcmdset(
				struct unit_info	* uip);
extern io_return_t	fd_setparms(
				int			unit,
				int			cmdarg);
extern io_return_t	fd_getparms(
				dev_t			dev,
				int			* cmdarg);
extern io_return_t	fd_format(
				dev_t			dev,
				int			* cmdarg);
extern int		makeidtbl(
				struct fmttbl		* tbl,
				struct fddrtab		* dr,
				unsigned short		track,
				unsigned short		intlv);
extern void		m765intrsub(
				struct unit_info	* uip);
extern void		rwintr(
				struct unit_info	* uip);
extern void		rwierr(
				struct unit_info	* uip);
extern void		rbintr(
				struct unit_info	* uip);
extern void		seekintr(
				struct unit_info	* uip);
extern void		seekintre(
				struct unit_info	* uip);
extern void		seekierr(
				struct unit_info	* uip,
				char			seekpoint);
extern void		m765sweep(
				struct unit_info	* uip,
				struct fddrtab		* cdr);
extern void		fd_disksort(
				struct unit_info	* uip,
				struct buf		* bp);
extern void		intrerr0(
				struct unit_info	* uip);
extern void		quechk(
				struct unit_info	* uip);
extern void		fdprint(
				dev_t			dev,
				char			* str);
extern void		rstout(
				struct unit_info	* uip);
extern void		specify(
				struct unit_info	* uip);
extern int		rbrate(
				char			mtype,
				struct unit_info	* uip);
extern int		rbirate(
				struct unit_info	*uip);
extern int		fdseek(
				char			mtype,
				struct unit_info	* uip,
				int			cylno);
extern int		fdiseek(
				struct unit_info	* uip,
				int			cylno);
extern int		outicmd(
				struct unit_info	* uip);
extern int		sis(
				struct unit_info	* uip);
extern int		fdc_sts(
				int			mode,
				struct unit_info	* uip);
extern void		mtr_on(
				struct unit_info	* uip);
extern int		mtr_start(
				struct unit_info	* uip);
extern void		mtr_off(
				struct unit_info	* uip);

/*
 * Floppy Device-Table Definitions (drtabs)
 *
 *      Cyls,Sec,spc,part,Mtype,RWFpl,FGpl
 */
struct	fddrtab m765f[] = {			/* format table */
	80, 18, 1440,  9, 0x88, 0x2a, 0x50,	/* [0] 3.50" 720  Kb  */
	80, 36, 2880, 18, 0x08, 0x1b, 0x6c,	/* [1] 3.50" 1.44 Meg */
	40, 18,  720,  9, 0xa8, 0x2a, 0x50,	/* [2] 5.25" 360  Kb  */
	80, 30, 2400, 15, 0x08, 0x1b, 0x54	/* [3] 5.25" 1.20 Meg */
};

/*
 * The following are static initialization variables
 * which are based on the configuration.
 */
struct ctrl_info ctrl_info[MAXUNIT>>1] = {		/* device data table */
	{  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ,
	{  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

struct unit_info unit_info[MAXUNIT];		/* unit buffer headers	*/

char *fderr = "FD Error on unit";
char *fdmsg[] = {
	"?",
	"Missing data address mark",
	"Write protected",
	"Sector not found",
	"Data Overrun",				/* Over run error */
	"Uncorrectable data read error",	/* CRC Error */
	"FDC Error",
	"Illegal format type",
	"Drive not ready",
	"diskette not present - please insert",
	"Illegal interrupt type"
};

struct buf	fdrbuf[MAXUNIT];	/* data transfer buffer structures */

caddr_t	fd_std[NFD] = { 0 };
struct	bus_device *fd_dinfo[NFD*2];
struct	bus_ctlr *fd_minfo[NFD];
struct	bus_driver	fddriver = 
	{(probe_t)fdprobe, fdslave, fdattach, 0, fd_std, "fd", fd_dinfo,
		"fdc", fd_minfo, 0};

int	m765verify[MAXUNIT] = {1,1,1,1};	/* write after read flag */
						/* 0 != verify mode	*/ 
						/* 0 == not verify mode */
#if 	CBUS || HIMEM
vm_offset_t fd_remap_addr;	/* address at which is remapped memory */
#ifdef	CBUS
int fd_dma_window;	/* DMA window to access above 16 Megs */
#endif	/* CBUS */
#if	HIMEM
hil_t	fd_hil;			/* Used for himem convert/revert */
#endif	/* HIMEM */
#endif	/* CBUS || HIMEM */

#define trfrate(uip, type)   outb(VFOREG(uip->addr),(((type)&RATEMASK)>>6))
#define rbskrate(uip, type)  trfrate(uip,(type)&RAPID?RPSEEK:NMSEEK)
#define getparm(type)   ((type<0||type>3)?(struct fddrtab *)ERROR:&m765f[type])
#define relative(s1,s2) ((s1)>(s2)?(s1)-(s2):(s2)-(s1))

int
fdprobe(
	int		port,
	struct bus_ctlr	*ctlr)
{
	int			spot = STSREG((int) ctlr->address);
	struct ctrl_info	*cip = &ctrl_info[ctlr->unit];
	int			i, in;

	outb(spot, DATAOK);
	for (i = 1000; i--;) {
		in = inb(spot);
		if ((in&DATAOK) == DATAOK && !(in&0x0f)) {
			take_ctlr_irq(ctlr);
			cip->b_cmd.c_rbmtr = 0;		/* recalibrate/moter flag */
			cip->b_cmd.c_intr = CMDRST;	/* interrupt flag */
			cip->b_unitf = 0;
			cip->b_uip = 0;
			cip->b_rwerr = cip->b_seekerr = cip->b_rberr = 0;
#if     CBUS
			if (!is_eisa_bus)
			    fd_dma_window = cbus_alloc_win(1);
#else   /* CBUS */
#if	HIMEM
 			himem_reserve(1);
#endif	/* HIMEM */
#endif	/* CBUS */
			printf("%s%d: port = %x, spl = %d, pic = %d.\n", ctlr->name,
				ctlr->unit, ctlr->address, ctlr->sysdep, ctlr->sysdep1);
			return(1);
		}
	}
	return(0);
}

int
fdslave(
	struct bus_device	*dev,
	caddr_t			xxx)
{
	return(1);	/* gross hack */
}

void
fdattach(
	struct bus_device	*dev)
{
	struct unit_info	*uip = &unit_info[dev->unit];
	struct ctrl_info	*cip = &ctrl_info[dev->ctlr];

	uip->dev = dev;
	dev->address = dev->mi->address;
	uip->addr = (long)dev->address;
	uip->b_cmd = &cip->b_cmd;
	uip->b_seekaddr = 0;
	uip->av_forw = 0;
	uip->wakeme = 0;
	if (cip->b_unitf) {
		uip->b_unitf=cip->b_unitf->b_unitf;
		cip->b_unitf->b_unitf=uip;
	} else {
		uip->b_unitf=uip;
		cip->b_unitf=uip;
	}
	uip->d_drtab.dr_type &= ~OKTYPE; 

	printf(", port = %x, spl = %d, pic = %d.",
		dev->address, dev->sysdep, dev->sysdep1);

	rstout(uip);
	specify(uip);
}
/*****************************************************************************
 *
 * TITLE:	fdopen
 *
 * ABSTRACT:	Open a unit. 
 *
 ****************************************************************************/

io_return_t
fdopen(
	dev_t		dev,
	dev_mode_t	flag,	/* not used */
	io_req_t	ior)	/* not used */
{
	struct fddrtab		*driv;
	struct buf		*wbp;
	spl_t			x = SPL();
	io_return_t		error = D_SUCCESS;
	int			unit = UNIT(dev);
	struct unit_info	*uip = &unit_info[unit];
	int			slave = uip->dev->slave;
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct	fdcmd		*cmdp = uip->b_cmd;

#if	NCPUS > 1
	io_grab_master();
#endif	/* NCPUS > 1 */
	if (unit < MAXUNIT){
	  /* Since all functions that use this are called from open, we only
	     set this once, right here. */
	  	rstout(uip);
		cip->b_wup = uip;
		openchk(cmdp);
		cmdp->c_devflag |= FDMCHK;
		chkbusy(cmdp);
		cmdp->c_stsflag |= MTRFLAG;
		mtr_on(uip);
		if(inb(VFOREG(uip->addr))&OPENBIT || 
		   !(uip->d_drtab.dr_type&OKTYPE)){
			uip->d_drtab.dr_type &= ~OKTYPE;
			if(!rbrate(RAPID, uip))
				fdseek(RAPID, uip, 2);
			if(inb(VFOREG(uip->addr))&OPENBIT)
				error = D_NO_SUCH_DEVICE;
		}
		cmdp->c_stsflag &= ~MTRFLAG;
		mtr_on(uip);
		openfre(cmdp);
		if(!error && !(uip->d_drtab.dr_type & OKTYPE)) {
			driv = &m765f[MEDIATYPE(dev)];
			wbp = geteblk(BLKSIZE);
			m765sweep(uip, driv);
			cmdp->c_rbmtr &= ~(1<<(RBSHIFT+(slave)));
			++cip->b_rwerr;
			wbp->b_dev = dev; wbp->b_error = 0; wbp->b_resid = 0;
			wbp->b_flags = (B_READ|B_VERIFY); wbp->b_bcount = 512;
			wbp->b_pfcent = 2*driv->dr_spc + driv->dr_nsec - 1;
			setqueue(wbp, uip);
			biowait(wbp);
			brelse(wbp);
			error = D_SUCCESS;
			uip->d_drtab.dr_type |= OKTYPE;
		}
	} else
		error = D_NO_SUCH_DEVICE;
      endopen:
#if	NCPUS > 1
	io_release_master();
#endif	/* NCPUS > 1 */
	splx(x);
	return(error);
}
/*****************************************************************************
 *
 * TITLE:	fdclose
 *
 * ABSTRACT:	Close a unit.
 *
 *	Called on last close. mark the unit closed and not-ready.
 *
 * 	Unix doesn't actually "open" an inode for rootdev, swapdev or pipedev.
 *	If UNIT(swapdev) != UNIT(rootdev), then must add code in init() to 
 *	"open" swapdev.  These	devices should never be closed.
 *
 *****************************************************************************/

void
fdclose(
	dev_t		dev)	/* major, minor numbers */
{
	extern			dev_t	rootdev, swapdev;
	struct unit_info	*uip = &unit_info[UNIT(dev)];
	spl_t			s;

#if	NCPUS > 1
	io_grab_master();
#endif	/* NCPUS > 1 */

	/* Clear the bit.
	 * If last close of drive insure drtab queue is empty before returning.
	 */
	s = SPL();
	while(uip->av_forw != 0) {
		uip->wakeme = 1;
		sleep((char *)uip, PRIBIO);
	}
	splx(s);
#if	NCPUS > 1
	io_release_master();
#endif	/* NCPUS > 1 */
}
/*****************************************************************************
 *
 * TITLE:	fdstrategy
 *
 * ABSTRACT:	Queue an I/O Request, and start it if not busy already.
 *
 *	Reject request if unit is not-ready.
 *
 *	Note:	check for not-ready done here ==> could get requests
 *		queued prior to unit going not-ready.
 *		not-ready status to those requests that are attempted
 *		before a new volume is inserted.  Once a new volume is
 *		inserted, would get good I/O's to wrong volume.
 *
 * CALLS:	iodone(),setqueue()
 *
 * CALLING ROUTINES:	fdread (indirectly, thru physio)
 *			fdwrite (indirectly, thru physio)
 *
 ****************************************************************************/

void
fdstrategy(
	struct buf	*bp)	/* buffer header */
{
	unsigned	        bytes_left;
	daddr_t			secno;
	struct unit_info	*uip = &unit_info[UNIT(bp->b_dev)];
	struct fddrtab		*dr = &uip->d_drtab;
	struct fddrtab		*sdr;

	bp->b_error = 0;
	/* set b_resid to b_bcount because we haven't done anything yet */
	bp->b_resid = bp->b_bcount;
	if (!(dr->dr_type & OKTYPE) || 
	    ((sdr = getparm(MEDIATYPE(bp->b_dev)))==(struct fddrtab *)ERROR) ||
	    /* wrong parameters */
	    (sdr->dr_ncyl != dr->dr_ncyl) || (sdr->dr_nsec != dr->dr_nsec) ||
	    ((sdr->dr_type|OKTYPE) != dr->dr_type) ||
	    (sdr->dr_rwgpl != dr->dr_rwgpl) ||
	    (sdr->dr_fgpl != dr->dr_fgpl)) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		biodone(bp);
		return;
	}
	/*
	 * Figure "secno" from b_blkno. Adjust sector # for partition.
	 *
	 * If reading just past the end of the device, it's
	 * End of File.  If not reading, or if read starts further in
	 * than the first sector after the partition, it's an error.
	 *
	 * secno is logical blockno / # of logical blocks per sector */
	secno = (bp->b_blkno * NBPSCTR) >> 9;
	if (secno >= dr->p_nsec) {
		if (!((bp->b_flags & B_READ) && (secno == dr->p_nsec))){
			/* off the deep end */
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
		}
		biodone(bp);
		return;
	}
/* At this point, it is no longer possible to directly return from strategy.
   We now set b_resid to the number of bytes we cannot transfer because
   they lie beyond the end of the request's partition.  This value is 0
   if the entire request is within the partition. */
	bytes_left = (dr->p_nsec - secno) << 9;
	bp->b_resid = ((bp->b_bcount<=bytes_left)?0:(bp->b_bcount-bytes_left));
	bp->b_pfcent = secno;
#if	(NCPUS > 1)
	io_grab_master();
#endif	/* (NCPUS > 1) */
	setqueue(bp, uip);
#if	(NCPUS > 1)
	io_release_master();
#endif	/* (NCPUS > 1) */
}

/***************************************************************************
 *
 *	set queue to buffer
 *
 ***************************************************************************/

void
setqueue(
	struct buf		*bp,
	struct unit_info	*uip)
{
	spl_t			x = SPL();
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct	fdcmd		*cmdp = uip->b_cmd;

	openchk(cmdp);		/* openning check */
	cmdp->c_devflag |= STRCHK;
	fd_disksort(uip, bp);	/* queue the request */
	/*
	 * If no requests are in progress, start this one up.  Else
	 * leave it on the queue, and fdintr will call m765io later.
	 */
	if(!cip->b_uip)
		m765io(uip);
	splx(x);
}
/***************************************************************************
 *
 *	check io_busy routine
 *
 ***************************************************************************/

void
chkbusy(
	struct	fdcmd	*cmdp)
{
	while(cmdp->c_devflag & STRCHK){
		cmdp->c_devflag |= STRWAIT;
		sleep((char *)&cmdp->c_devflag,PZERO);
	} 
}
/***************************************************************************
 *
 *	check fdopen() routine
 *
 ***************************************************************************/

void
openchk(
	struct	fdcmd	*cmdp)
{
	while(cmdp->c_devflag & FDMCHK ){
		cmdp->c_devflag |= FDWAIT;
		sleep((char *)&cmdp->c_devflag,PZERO);
	} 
}
/***************************************************************************
 *
 *	free fdopen() routine
 *
 ***************************************************************************/

void
openfre(
	struct	fdcmd	*cmdp)
{
	cmdp->c_devflag &= ~FDMCHK;
	if(cmdp->c_devflag & FDWAIT){
		cmdp->c_devflag &= ~FDWAIT;
		wakeup((char *)&cmdp->c_devflag);
	}
}
/*****************************************************************************
 *
 * TITLE:	m765io
 *
 * ABSTRACT:	Start handling an I/O request.
 *
 ****************************************************************************/

void
m765io(
	struct unit_info	*uip)
{
	register struct buf *bp;
	struct ctrl_info *cip = &ctrl_info[uip->dev->ctlr];

	bp = uip->av_forw; /*move bp to ctrl_info[ctrl].b_buf*/
	cip->b_buf = bp;
	cip->b_uip = uip;
	cip->b_xferaddr  = bp->b_un.b_addr;
	cip->b_xfercount = bp->b_bcount - bp->b_resid;
	cip->b_sector    = bp->b_pfcent;
	uip->b_cmd->c_stsflag |= MTRFLAG;
	if(!mtr_start(uip))
		timeout((timeout_fcn_t)m765iosub, uip, HZ);
	else
		m765iosub(uip);
}
/****************************************************************************
 *
 *	m765io subroutine
 *
 ****************************************************************************/

void
m765iosub(
	struct unit_info	*uip)
{
	struct fddrtab		*dr = &uip->d_drtab;
	int			startsec;
	int			slave = uip->dev->slave;
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct	fdcmd		*cmdp = uip->b_cmd;

	rwcmdset(uip);
	if(cip->b_buf->b_flags&B_FORMAT)
		goto skipchk;
	startsec = (cmdp->c_rwdata[3] * dr->dr_nsec) + cmdp->c_rwdata[4];
	if(startsec+(cip->b_xfercount>>9)-1 > dr->dr_spc)
		cip->b_xferdma = (dr->dr_spc-startsec+1) << 9;
	else
skipchk:	cip->b_xferdma = cip->b_xfercount;

	/* 
	 * Need to remap or use intermediate memory if above 16 Megs
	 * if it is not an EISA bus 
	 */

#if	CBUS || HIMEM
	fd_remap_addr = kvtophys((vm_offset_t)cip->b_xferaddr); 
#if	CBUS
	if (!is_eisa_bus) {
		cbus_set_win(fd_dma_window, fd_remap_addr);
		fd_remap_addr &= (I386_PGBYTES-1);
		fd_remap_addr += cbus_get_win_padd(fd_dma_window);
	} else
#else	/* CBUS */
#if	HIMEM
	assert(!fd_hil);
	if (high_mem_page(fd_remap_addr)) {
	  	vm_size_t dmalen;
		int op;

		dmalen = i386_trunc_page(fd_remap_addr) + I386_PGBYTES
		         - fd_remap_addr;
		if (dmalen > cip->b_xferdma)
			dmalen = cip->b_xferdma;
		if (cip->b_buf->b_flags&(B_READ))
			op = D_READ;
		else
			op = D_WRITE;
	    	fd_remap_addr = himem_convert(fd_remap_addr, dmalen, op,
					      &fd_hil);
	} else
#endif	/* HIMEM */
#endif	/* CBUS */
		fd_remap_addr = 0;
#endif	/* CBUS || HIMEM */

	if(!(cmdp->c_rbmtr & (1<<(RBSHIFT+slave))))
       		cip->b_status = rbirate(uip);
	else if(uip->b_seekaddr != cmdp->c_saddr)
		cip->b_status = fdiseek(uip,cmdp->c_saddr);
	else
		cip->b_status = outicmd(uip);
	if(cip->b_status)
		intrerr0(uip);
	return;
}
/***************************************************************************
 *
 *	read / write / format / verify command set to command table
 *
 ***************************************************************************/

void
rwcmdset(
	struct unit_info	*uip)
{
	short			resid;
	int			slave = uip->dev->slave;
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct fdcmd		*cmdp = uip->b_cmd;

	switch(cip->b_buf->b_flags&(B_FORMAT|B_VERIFY|B_READ|B_WRITE)){
	case B_VERIFY|B_WRITE:	/* VERIFY after WRITE */
		cmdp->c_rwdata[0] = RDMV;
		break;
	case B_FORMAT:
		cmdp->c_dcount = FMTCNT; 
		cmdp->c_rwdata[0] = FMTM;
		cmdp->c_saddr = cip->b_sector / uip->d_drtab.dr_spc;
		resid = cip->b_sector % uip->d_drtab.dr_spc;
		cmdp->c_rwdata[1] = slave|((resid/uip->d_drtab.dr_nsec)<<2);
		cmdp->c_rwdata[2] = 
			((struct fmttbl *)cip->b_buf->b_un.b_addr)->s_type;
		cmdp->c_rwdata[3] = uip->d_drtab.dr_nsec;
		cmdp->c_rwdata[4] = uip->d_drtab.dr_fgpl;
		cmdp->c_rwdata[5] = FMTDATA;
		break;
	case B_WRITE:
	case B_READ:
	case B_READ|B_VERIFY:
		cmdp->c_dcount = RWCNT;
		if(cip->b_buf->b_flags&B_READ)
			if(cip->b_buf->b_flags&B_VERIFY)
				cmdp->c_rwdata[0] = RDMV;
			else
				cmdp->c_rwdata[0] = RDM;
		else
			cmdp->c_rwdata[0] = WTM;	/* format or write */
		resid = cip->b_sector % uip->d_drtab.dr_spc;
		cmdp->c_rwdata[3] = resid / uip->d_drtab.dr_nsec;
		cmdp->c_rwdata[1] = slave|(cmdp->c_rwdata[3]<<2);
		cmdp->c_rwdata[2] = cmdp->c_saddr = 
			cip->b_sector / uip->d_drtab.dr_spc;
		cmdp->c_rwdata[4] = (resid % uip->d_drtab.dr_nsec) + 1;
		cmdp->c_rwdata[5] = 2;
		cmdp->c_rwdata[6] = uip->d_drtab.dr_nsec;
		cmdp->c_rwdata[7] = uip->d_drtab.dr_rwgpl;
		cmdp->c_rwdata[8] = DTL;
		D(printf("SET %x %x C%x H%x S%x %x %x %x %x ",
			cmdp->c_rwdata[0], cmdp->c_rwdata[1],
			cmdp->c_rwdata[2], cmdp->c_rwdata[3],
			cmdp->c_rwdata[4], cmdp->c_rwdata[5],
			cmdp->c_rwdata[6], cmdp->c_rwdata[7],
			cmdp->c_rwdata[8]));
		break;
	}
}
/*****************************************************************************
 *
 * TITLE:	fdread
 *
 * ABSTRACT:	"Raw" read.  Use physio().
 *
 * CALLS:	m765breakup (indirectly, thru physio)
 *
 ****************************************************************************/

io_return_t
fdread(
	dev_t		dev,
	io_req_t	uio)
{ 
	/* no need for page-size restriction */
	return (block_io(fdstrategy, minphys, uio));
}
/*****************************************************************************
 *
 * TITLE:	fdwrite
 *
 * ABSTRACT:	"Raw" write.  Use physio().
 *
 * CALLS:	m765breakup (indirectly, thru physio)
 *
 ****************************************************************************/

io_return_t
fdwrite(
	dev_t		dev,
	io_req_t	uio)
{
	/* no need for page-size restriction */
	return (block_io(fdstrategy, minphys, uio));
}
/*****************************************************************************
 *
 * TITLE:	fdminphys
 *
 * ABSTRACT:	Trim buffer length if buffer-size is bigger than page size
 *
 * CALLS:	physio
 *
 ****************************************************************************/

void
fdminphys(
	struct buf	*bp)
{
	if (bp->b_bcount > PAGESIZ)
		bp->b_bcount = PAGESIZ;
}

/* IOC_OUT only and not IOC_INOUT */
io_return_t
fdgetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,		/* pointer to OUT array */
	natural_t	*count)		/* OUT */
{
	struct disk_parms	p;
	io_return_t		ret;

	switch (flavor) {

	/* Mandatory flavors */

	case DEV_GET_SIZE:
		ret = fd_getparms(dev, (int *)&p);
		if (ret) return ret;
		data[DEV_GET_SIZE_DEVICE_SIZE] = p.dp_pnumsec * NBPSCTR;
		data[DEV_GET_SIZE_RECORD_SIZE] = NBPSCTR;
		*count = DEV_GET_SIZE_COUNT;
		return (D_SUCCESS);

	/* Extra flavors */

	case V_GETPARMS:
		if (*count < sizeof (struct disk_parms)/sizeof (int))
			return (D_INVALID_OPERATION);
		*count = sizeof (struct disk_parms)/sizeof(int);
		return (fd_getparms(dev, data));
        default:
		return (D_INVALID_OPERATION);
	}
}
/* IOC_VOID or IOC_IN or IOC_INOUT */

io_return_t
fdsetstat(
	dev_t		dev,
	dev_flavor_t	flavor,
	dev_status_t	data,
	natural_t	count)
{
	int			unit = UNIT(dev);
	switch (flavor) {
	case V_SETPARMS:    /* Caller wants reset_parameters */
		return(fd_setparms(unit,*(int *)data));
	case V_FORMAT:
		return(fd_format(dev,data));
	case V_VERIFY:	/* cmdarg : 0 == no verify, 0 != verify */
		m765verify[unit] = *(int *)data;
		return(D_SUCCESS);
	default:
		return(D_INVALID_OPERATION);
	}
}

/*
 *      Get block size
 */
io_return_t
fddevinfo(
	dev_t   	dev,
	dev_flavor_t	flavor,
	char		*info)
{
	register struct	fddrtab	*dr;
        register struct fdpart *p;
	register int result = D_SUCCESS;

	switch (flavor) {
	case D_INFO_BLOCK_SIZE:
		dr = &unit_info[UNIT(dev)].d_drtab;

	        if(dr->dr_type & OKTYPE)
        	        *((int *) info) =  512;
		else
			result = D_INVALID_OPERATION;

		break;
	default:
		result = D_INVALID_OPERATION;
	}

        return(result);
}

/****************************************************************************
 *
 *	set fd parameters 
 *
 ****************************************************************************/

io_return_t
fd_setparms(
	int		unit,
	int		cmdarg)
{
	struct fddrtab		*fdparm;
	spl_t			x;
	struct unit_info	*uip = &unit_info[unit];
	struct	fdcmd		*cmdp = uip->b_cmd;

	cmdp->c_rbmtr &= ~(1<<(RBSHIFT+uip->dev->slave));
	if ((fdparm = getparm(MEDIATYPE(cmdarg))) == (struct fddrtab *)ERROR)
	        return(D_INVALID_SIZE);
	x = SPL();
	openchk(cmdp);
	cmdp->c_devflag |= FDMCHK;
	chkbusy(cmdp);
	m765sweep(uip, fdparm);
	uip->d_drtab.dr_type |= OKTYPE;
	openfre(cmdp);
	splx(x);
	return(0);
}
/****************************************************************************
 *
 *	get fd parameters 
 *
 ****************************************************************************/

io_return_t
fd_getparms(
	dev_t		dev,	/* major, minor numbers */
	int		*cmdarg)
{
	struct disk_parms	*diskp = (struct disk_parms *)cmdarg;
	register struct	fddrtab	*dr = &unit_info[UNIT(dev)].d_drtab;

	if(dr->dr_type & OKTYPE){
		diskp->dp_type = DPT_FLOPPY;
		diskp->dp_heads = 2;
		diskp->dp_sectors = dr->dr_nsec;
		diskp->dp_pstartsec = 0;
		diskp->dp_cyls = dr->dr_ncyl;
		diskp->dp_pnumsec = dr->p_nsec;
		diskp->dp_secsiz = 512;
		return(0);
	}
	return(D_NO_SUCH_DEVICE);
}
/****************************************************************************
 *
 *	format command
 *
 ****************************************************************************/

io_return_t
fd_format(
	dev_t		dev,	/* major, minor numbers */
	int		*cmdarg)

{
	register struct buf	*bp;
	register daddr_t	track;
	union  io_arg		*varg;
	u_short			num_trks;
	register struct	fddrtab	*dr = &unit_info[UNIT(dev)].d_drtab;

	if(!(dr->dr_type & OKTYPE))
		return(D_INVALID_SIZE);	
	varg = (union io_arg *)cmdarg;
	num_trks = varg->ia_fmt.num_trks;
	track = (daddr_t)(varg->ia_fmt.start_trk*dr->dr_nsec);
	if((track + (num_trks*dr->dr_nsec))>dr->p_nsec)
		return(D_INVALID_SIZE);
	bp = geteblk(BLKSIZE);		/* get struct buf area */
	while (num_trks>0) {
		bp->b_flags &= ~B_DONE;
		bp->b_dev = dev;
		bp->b_error = 0; bp->b_resid = 0;
		bp->b_flags = B_FORMAT;	
		bp->b_bcount = dr->dr_nsec * FMTID;
		bp->b_blkno = (daddr_t)((track << 9) / NBPSCTR);
		if(makeidtbl((struct fmttbl *)bp->b_un.b_addr,dr,
			     varg->ia_fmt.start_trk++,varg->ia_fmt.intlv))
			return(D_INVALID_SIZE);
		fdstrategy(bp);
		biowait(bp);
		if(bp->b_error)
			if((bp->b_error == (char)EBBHARD) || 
			   (bp->b_error == (char)EBBSOFT))
				return(EIO);
			else
				return(bp->b_error);
		num_trks--;
		track += dr->dr_nsec;
	}
	brelse(bp);
	return(D_SUCCESS);
}
/****************************************************************************
 *
 *	make id table for format
 *
 ****************************************************************************/

int
makeidtbl(
	struct fmttbl		*tblpt,
	struct fddrtab		*dr,
	unsigned short		track,
	unsigned short		intlv)
{
	register int	i,j,secno;

	if(intlv >= dr->dr_nsec)
		return(1);
	for(i=0; i<dr->dr_nsec; i++)
		tblpt[i].sector = 0;
	for(i=0,j=0,secno=1; i<dr->dr_nsec; i++){
		tblpt[j].cyl = track >> 1;
		tblpt[j].head = track & 1;
		tblpt[j].sector = secno++;
		tblpt[j].s_type = 2;
		if((j+=intlv) < dr->dr_nsec)
			continue;
		for(j-=dr->dr_nsec; j < dr->dr_nsec ; j++)
			if(!tblpt[j].sector)
				break;
	}
	return(0);
}
/*****************************************************************************
 *
 * TITLE:	fdintr
 *
 * ABSTRACT:	Handle interrupt.
 *
 *	Interrupt procedure for m765 driver.  Gets status of last
 *	operation and performs service function according to the
 *	type of interrupt.  If it was an operation complete interrupt,
 *	switches on the current driver state and either declares the
 *	operation done, or starts the next operation
 *
 ****************************************************************************/

void
fdintr(
	int	ctrl)
{
	struct unit_info 	*uip = ctrl_info[ctrl].b_uip;
	struct unit_info 	*wup = ctrl_info[ctrl].b_wup;
	struct fdcmd		*cmdp = &ctrl_info[ctrl].b_cmd;
	if(cmdp->c_stsflag & INTROUT)
		untimeout((timeout_fcn_t)fdintr, (void *)ctrl);
	cmdp->c_stsflag &= ~INTROUT;	
	switch(cmdp->c_intr){
	case RWFLAG:
		rwintr(uip);
		break;	
	case SKFLAG:
	case SKEFLAG|SKFLAG:
	case RBFLAG:
		timeout((timeout_fcn_t)m765intrsub, uip, SEEKWAIT);
		break;
	case WUPFLAG:
		cmdp->c_intr &= ~WUPFLAG;
		wakeup((char *)wup);
	}
}
/*****************************************************************************
 *
 *	interrup subroutine (seek recalibrate)
 *
 *****************************************************************************/

void
m765intrsub(
	struct unit_info	*uip)
{
	struct ctrl_info *cip = &ctrl_info[uip->dev->ctlr];

	if((cip->b_status = sis(uip))!=  ST0OK)
		switch(uip->b_cmd->c_intr){
		case SKFLAG:
			seekintr(uip);
			break;
		case SKEFLAG|SKFLAG:
			seekintre(uip);
			break;
		case RBFLAG:
			rbintr(uip);
		}
}
/*****************************************************************************
 *
 *	read / write / format / verify interrupt routine
 *
 *****************************************************************************/

void
rwintr(
	struct unit_info	*uip)
{
	int			rsult[7];
	register int		rtn, count;
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct fdcmd		*cmdp = uip->b_cmd;

	cmdp->c_intr &= ~RWFLAG;
	if((cip->b_buf->b_flags&(B_READ|B_VERIFY))!=(B_READ|B_VERIFY))
		if(inb(VFOREG(uip->addr))&OPENBIT){
			if(cip->b_buf->b_flags&B_FORMAT){
				cip->b_status = TIMEOUT;
				intrerr0(uip);
			} else {
				if((inb(STSREG(uip->addr))&ST0OK)!=ST0OK)
					printf("%s %d : %s\n",
						fderr,
						uip-unit_info,
						fdmsg[DOORERR]);
				rstout(uip);
				specify(uip);
				cmdp->c_rbmtr &= RBRST;
				cmdp->c_intr |= SKEFLAG;
				if(cmdp->c_saddr > 2)
					fdiseek(uip, cmdp->c_saddr-2);
				else
					fdiseek(uip, cmdp->c_saddr+2);
			}
			return;
		}
	for( count = 0 ; count < 7 ; count++ ){
		if(rtn = fdc_sts(FD_ISTS, uip))	/* status check */
			goto rwend;
		rsult[count] = inb(DATAREG(uip->addr));
	}
	rtn = 0;
	if(rsult[0]&0xc0){
		rtn = cmdp->c_rwdata[0]<<8;
		if(rsult[0]&0x80){ rtn |= FDCERR;   goto rwend; }
		if(rsult[1]&0x80){ rtn |= NOREC;    goto rwend; }
		if(rsult[1]&0x20){ rtn |= CRCERR;   goto rwend; }
		if(rsult[1]&0x10){ rtn |= OVERRUN;  goto rwend; }
		if(rsult[1]&0x04){ rtn |= NOREC;    goto rwend; }
		if(rsult[1]&0x02){ rtn |= WTPRT;    goto rwend; }
		if(rsult[1]&0x01){ rtn |= ADDRERR;  goto rwend; }
		rtn |= FDCERR;
rwend:		outb(0x0a, 0x06);
	}
	if(cip->b_status = rtn) {
		D(printf("\n->rwierr %x ", rtn));
		rwierr(uip);
	} else { /* write command */
		if(((cip->b_buf->b_flags&(B_FORMAT|B_READ|B_WRITE))==B_WRITE) 
		   && !(cip->b_buf->b_flags & B_VERIFY)) {
			D(printf("->w/v "));
			cip->b_buf->b_flags |= B_VERIFY;
			rwcmdset(uip);
			if(cip->b_status = outicmd(uip))
				intrerr0(uip);
			return;
		}
		/* clear retry count */
		cip->b_buf->b_flags &= ~B_VERIFY;
		cip->b_rwerr = cip->b_seekerr = cip->b_rberr = 0;
		cip->b_xfercount -= cip->b_xferdma;
		cip->b_xferaddr += cip->b_xferdma;
		cip->b_sector = cip->b_sector+(cip->b_xferdma>>9);
		/* next address (cyl,head,sec) */
		if((int)cip->b_xfercount>0) {
#if	HIMEM
			if (fd_hil) {
				himem_revert(fd_hil);
				fd_hil = 0;
			}
#endif	/* HIMEM */
			m765iosub(uip);
		} else
			quechk(uip);
	}
}
/*****************************************************************************
 *
 *	read / write / format / verify error routine
 *
 *****************************************************************************/

void
rwierr(
	struct unit_info	*uip)
{
	short			status;
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct	fdcmd		*cmdp = uip->b_cmd;

	D(printf("%x-%x-%x ", cip->b_rwerr&SRMASK, cip->b_rwerr&MRMASK, cip->b_rwerr&LRMASK));
	if((cip->b_buf->b_flags&(B_READ|B_VERIFY))==(B_READ|B_VERIFY)){
		if((cip->b_rwerr&SRMASK)<MEDIARD)
			goto rwrtry;
		if((cip->b_rwerr&MRMASK)<MEDIASEEK)
			goto rwseek;
		goto rwexit;
	} else
		if(cip->b_buf->b_flags&B_VERIFY){
			cip->b_buf->b_flags &= ~B_VERIFY;
			rwcmdset(uip);
		}
rwrtry:	status = cip->b_status;
	if((++cip->b_rwerr&SRMASK)<SRETRY)
		cip->b_status = outicmd(uip);
	else {
rwseek:		cip->b_rwerr = (cip->b_rwerr&RMRMASK)+MINC;
		if((cip->b_rwerr&MRMASK)<MRETRY){
			cmdp->c_intr |= SKEFLAG;
			if(cmdp->c_saddr > 2)
				cip->b_status=fdiseek(uip,cmdp->c_saddr-2);
			else
				cip->b_status=fdiseek(uip,cmdp->c_saddr+2);
		} else {
			cip->b_rwerr = (cip->b_rwerr&LRMASK)+LINC;
			if((cip->b_rwerr&LRMASK)<LRETRY)
       				cip->b_status=rbirate(uip);
		}
	}
	if(cip->b_status){
		D(printf("ERR->intrerr0 "));
		cip->b_status = status;
rwexit:		intrerr0(uip);
	}
}
/*****************************************************************************
 *
 *	recalibrate interrupt routine
 *
 *****************************************************************************/

void
rbintr(
	struct unit_info	*uip)
{
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct fdcmd		*cmdp = uip->b_cmd;

	cmdp->c_intr &= ~RBFLAG;
	if(cip->b_status) {
		if(++cip->b_rberr<SRETRY)
			cip->b_status = rbirate(uip);
	} else {
		cmdp->c_rbmtr |= 1<<(RBSHIFT+uip->dev->slave);
		uip->b_seekaddr = 0;
		cip->b_rberr = 0;
		cip->b_status=fdiseek(uip, cmdp->c_saddr);
	}
	if(cip->b_status)
		intrerr0(uip);
}
/******************************************************************************
 *
 *	seek interrupt routine
 *
 *****************************************************************************/

void
seekintr(
	struct unit_info	*uip)
{
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct fdcmd		*cmdp = uip->b_cmd;

	cmdp->c_intr &= ~SKFLAG;
	if(cip->b_status)
		seekierr(uip, cmdp->c_saddr);
	else {
		uip->b_seekaddr = cmdp->c_saddr;
		cip->b_status = outicmd(uip);
	}
	if(cip->b_status)
		intrerr0(uip);
	else
		cip->b_seekerr = 0;
}
/*****************************************************************************
 *
 *	seek error retry interrupt routine
 *
 *****************************************************************************/

void
seekintre(
	struct unit_info	*uip)
{
	register char		seekpoint;
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct fdcmd		*cmdp = uip->b_cmd;

	cmdp->c_intr &= ~(SKEFLAG|SKFLAG);
	if(cmdp->c_saddr > 2)
		seekpoint = cmdp->c_saddr-2;
	else
		seekpoint = cmdp->c_saddr+2;
	if(cip->b_status)
		seekierr(uip, seekpoint);
	else {
		uip->b_seekaddr = seekpoint;
		cip->b_status = fdiseek(uip, cmdp->c_saddr);
	}
	if(cip->b_status)
		intrerr0(uip);
	else
		cip->b_seekerr = 0;
}
/*****************************************************************************
 *
 *	seek error routine
 *
 *****************************************************************************/

void
seekierr(
	struct unit_info	*uip,
	char			seekpoint)
{
	struct ctrl_info *cip = &ctrl_info[uip->dev->ctlr];

	if((++cip->b_seekerr&SRMASK)<SRETRY)
		cip->b_status=fdiseek(uip, seekpoint);
	else {
		cip->b_seekerr = (cip->b_seekerr&MRMASK) + MINC;
		if((cip->b_seekerr&MRMASK)<MRETRY)
			cip->b_status=rbirate(uip);
	}
	if(cip->b_status)
		intrerr0(uip);
}
/*****************************************************************************
 *
 * TITLE:	m765sweep
 *
 * ABSTRACT:	Perform an initialization sweep.  
 *
 **************************************************************************/

void
m765sweep(
	struct unit_info	*uip,
	struct fddrtab		*cdr)	/* device initialization data */
{
	register struct fddrtab *dr = &uip->d_drtab;

	dr->dr_ncyl = cdr->dr_ncyl;
	dr->dr_nsec = cdr->dr_nsec;
	dr->dr_spc  = cdr->dr_spc;
	dr->p_nsec  = cdr->p_nsec;
	dr->dr_type = cdr->dr_type;
	dr->dr_rwgpl= cdr->dr_rwgpl;
	dr->dr_fgpl = cdr->dr_fgpl;
}
/*****************************************************************************
 *
 *  TITLE:  m765disksort
 *
 *****************************************************************************/

void
fd_disksort(
	struct unit_info	*uip,	/* Pointer to head of active queue */
	struct buf		*bp)	/* Pointer to buffer to be inserted */
{
	register struct buf *bp2; /*  Pointer to next buffer in queue	*/
	register struct buf *bp1; /*  Pointer where to insert buffer	*/

	if (!(bp1 = uip->av_forw)) {
		/* No other buffers to compare against */
		uip->av_forw = bp;
		bp->av_forw = 0;
		return;
	}
	bp2 = bp1->av_forw;
	while(bp2 && (relative(bp1->b_pfcent,bp->b_pfcent) >=
		      relative(bp1->b_pfcent,bp2->b_pfcent))) {
		bp1 = bp2;
		bp2 = bp1->av_forw;
	}
	bp1->av_forw = bp;
	bp->av_forw = bp2;
}
/*****************************************************************************
 *
 *	Set Interrupt error and FDC reset
 *
 *****************************************************************************/

void
intrerr0(
	struct unit_info	*uip)
{
	struct buf		*bp; /* Pointer to next buffer in queue	*/
	int			resid;
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct	fdcmd		*cmdp = uip->b_cmd;
	register struct	fddrtab *dr = &uip->d_drtab;

	if((cip->b_buf->b_flags&(B_READ|B_VERIFY))!=(B_READ|B_VERIFY)){
		resid = cip->b_xfercount = cip->b_xferdma-1-inb(DMACNT)*0x101;
		resid = (cip->b_sector + (resid>>9)) % dr->dr_spc;
		printf("%s %d : %s\n",
			fderr,
			uip->dev->slave,
			fdmsg[cip->b_status&BYTEMASK]);
		printf("cylinder = %d  ",cmdp->c_saddr);
		printf("head = %d  sector = %d  byte/sec = %d\n",
		resid / dr->dr_nsec , (resid % dr->dr_nsec)+1 , 512);
	}
	cip->b_rwerr = cip->b_seekerr = cip->b_rberr = 0;
	cmdp->c_intr = CMDRST;
	if(((cip->b_buf->b_flags&(B_READ|B_VERIFY))!=(B_READ|B_VERIFY)) &&
	   uip->dev->slave)
		dr->dr_type &= ~OKTYPE; 
	bp = cip->b_buf;
	bp->b_flags |= B_ERROR;
	switch(cip->b_status&BYTEMASK){
	case ADDRERR:
	case OVERRUN:
	case FDCERR:
	case TIMEOUT:
		bp->b_error = EIO;
		break;
	case WTPRT:
		bp->b_error = ENXIO;
		break;
	case NOREC:
		bp->b_error = EBBHARD;
		break;
	case CRCERR:
		bp->b_error = EBBSOFT;
	}
	rstout(uip);
	specify(uip);
	cmdp->c_rbmtr &= RBRST;
	quechk(uip);
}
/*****************************************************************************
 *
 *	Next queue check routine
 *
 *****************************************************************************/

void
quechk(
	struct unit_info	*uip)
{
	register struct	buf 	*bp = uip->av_forw;
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct unit_info        *loop;
	struct fdcmd		*cmdp = uip->b_cmd;
	/* clear retry count */
	cip->b_rwerr = cip->b_seekerr = cip->b_rberr = 0;
	bp->b_resid = bp->b_resid + cip->b_xfercount;
	uip->av_forw=bp->av_forw;
	if (!uip->av_forw && uip->wakeme) {
		uip->wakeme = 0;
		wakeup((char *)uip);
	}
#if	HIMEM
	if (fd_hil) {
		himem_revert(fd_hil);
		fd_hil = 0;
	}
#endif	/* HIMEM */
	biodone(bp);
	loop = uip;
	do {
		loop=loop->b_unitf;
		if (loop->av_forw) {
			m765io(loop);
			return;
		}
	} while (loop!=uip);
	cip->b_uip = 0;
	cmdp->c_stsflag &= ~MTRFLAG;
	mtr_on(uip);
	cmdp->c_devflag &= ~STRCHK;
	if(cmdp->c_devflag & STRWAIT){
		cmdp->c_devflag &= ~STRWAIT;
		wakeup((char *)&cmdp->c_devflag);
	}
}

void
fdprint(
	dev_t		dev,
	char		*str)
{
	printf("floppy disk driver: %s on bad dev %d, partition %d\n",
			str, UNIT(dev), 0);
}
/*****************************************************************************
 *
 *	fdc reset routine
 *
 *****************************************************************************/

void
rstout(
	struct unit_info	*uip)
{
	register int	outd;

	outd = ((uip->b_cmd->c_rbmtr&MTRMASK)<<MTR_ON)|uip->dev->slave;
	outb(CTRLREG(uip->addr), outd);
	outd |= FDC_RST;
	outb(CTRLREG(uip->addr), outd);
	outd |= DMAREQ;
	outb(CTRLREG(uip->addr), outd);
}
/*****************************************************************************
 *
 *	specify command routine
 *
 *****************************************************************************/

void
specify(
	struct unit_info	*uip)
{
	/* status check */
	if(fdc_sts(FD_OSTS, uip))
		return;
	/* Specify command */
	outb(DATAREG(uip->addr), SPCCMD);
	/* status check */
	if(fdc_sts(FD_OSTS, uip))
		return;
	/* Step rate,Head unload time */
	outb(DATAREG(uip->addr), SRTHUT);
	/* status check */
	if(fdc_sts(FD_OSTS, uip))
		return;
	/* Head load time,Non DMA Mode*/
	outb(DATAREG(uip->addr), HLTND);
	return;
}
/****************************************************************************
 *
 *	recalibrate command routine
 *
 ****************************************************************************/

int
rbrate(
	char			mtype,
	struct unit_info	*uip)
{
	register int	rtn = 1, rty_flg=2;
	spl_t		x;
	struct	fdcmd	*cmdp = uip->b_cmd;

	rbskrate(uip, mtype);			/* set transfer rate */
	while((rty_flg--)&&rtn){
		if(rtn = fdc_sts(FD_OSTS, uip))	/* status check */
			break;
		/*recalibrate command*/
		outb(DATAREG(uip->addr), RBCMD);
		if(rtn = fdc_sts(FD_OSTS, uip))	/* status check */
			break;
		/* Device to wake up specified in open */
		cmdp->c_intr |= WUPFLAG;
		x = SPL();
		outb(DATAREG(uip->addr), uip->dev->slave);
		rtn = ERROR;
		while(rtn) {
			uip->wakeme = 1;
			sleep((char *)uip, PZERO);
			if((rtn = sis(uip)) == ST0OK)
			  /* Device to wake up specified in open */
				cmdp->c_intr |= WUPFLAG;
			else
				break;
		}
		splx(x);
	}
	return(rtn);
}
/*****************************************************************************
 *
 *	seek command routine
 *
 ****************************************************************************/

int
fdseek(
	char			mtype,
	struct unit_info	*uip,
	int			cylno)
{
	spl_t		x;
	int		rtn;
	struct	fdcmd	*cmdp = uip->b_cmd;

	rbskrate(uip, mtype);
	if(rtn = fdc_sts(FD_OSTS, uip))			/* status check */
		return(rtn);
	outb(DATAREG(uip->addr), SEEKCMD);		/* seek command */
	if(rtn = fdc_sts(FD_OSTS, uip))			/* status check */
		return(rtn);
	outb(DATAREG(uip->addr), uip->dev->slave);	/* drive number */
	if(rtn = fdc_sts(FD_OSTS, uip))			/* status check */
		return(rtn);
	x = SPL();
	/* Device to wake up specified in open */
	cmdp->c_intr |= WUPFLAG;
	outb(DATAREG(uip->addr), cylno);		/* seek count */
	rtn = ERROR;
	while(rtn){	
		uip->wakeme = 1;
		sleep((char *)uip, PZERO);
		if((rtn = sis(uip)) == ST0OK)
		  /* Device to wake up specified in open */
			cmdp->c_intr |= WUPFLAG;
		else
			break;
	}
	splx(x);
	return(rtn);
}
/*****************************************************************************
 *
 *	seek commnd routine(use interrupt)
 *
 *****************************************************************************/

int
fdiseek(
	struct unit_info	*uip,
	int			cylno)
{
	register int	rtn;

	D(printf("SK %x ", cylno));
	rbskrate(uip, uip->d_drtab.dr_type);/* set transfer rate */
	if(rtn = fdc_sts(FD_OSTS, uip))		/* status check */
		goto fdiend;
	outb(DATAREG(uip->addr), SEEKCMD);	/* seek command */
	if(rtn = fdc_sts(FD_OSTS, uip))		/* status check */
		goto fdiend;
	outb(DATAREG(uip->addr), uip->dev->slave);	/* drive number */
	if(rtn = fdc_sts(FD_OSTS, uip))		/* status check */
		goto fdiend;
	uip->b_seekaddr = cylno;
	if(uip->d_drtab.dr_type&DOUBLE)
		cylno = cylno * 2;
	uip->b_cmd->c_intr |= SKFLAG;
	outb(DATAREG(uip->addr), cylno);	/* seek count */
fdiend:	
	if(rtn)
		rtn |= SEEKCMD<<8;
	return(rtn);
}
/*****************************************************************************
 *
 *	recalibrate command routine(use interrupt)
 *
 *****************************************************************************/

int
rbirate(
	struct unit_info	*uip)
{
	register int	rtn;

	rbskrate(uip, uip->d_drtab.dr_type);/* set transfer rate */
	if(!(rtn = fdc_sts(FD_OSTS, uip))) {		/* status check */
		/* recalibrate command */
		outb(DATAREG(uip->addr), RBCMD);
		if(!(rtn = fdc_sts(FD_OSTS, uip))) {	/* status check */
			uip->b_cmd->c_intr |= RBFLAG;
			outb(DATAREG(uip->addr), uip->dev->slave);
		}
	}
	return(rtn ? rtn|RBCMD<<8 : 0);
}
/*****************************************************************************
 *
 *	read / write / format / verify command out routine(use interrupt)
 *
 *****************************************************************************/

int
outicmd(
	struct unit_info	*uip)
{
	int			rtn;
	register int		*data,cnt0,dmalen;
	register long		address;
	struct ctrl_info	*cip = &ctrl_info[uip->dev->ctlr];
	struct fdcmd		*cmdp = uip->b_cmd;
	spl_t			x = splhi();

	outb(DMACMD1,DMADATA0);	/* DMA #1 command register 	*/
	outb(DMAMSK1,DMADATA1);	/* DMA #1 all mask register	*/
	/* Perhaps outb(0x0a,0x02); might work better on line above? */
	switch(cmdp->c_rwdata[0]){
	case RDM:
		D(printf("RDM"));
		outb(DMABPFF,DMARD);
		outb(DMAMODE,DMARD);
		break;
	case WTM:
	case FMTM:
		D(printf("W"));
		outb(DMABPFF,DMAWT);
		outb(DMAMODE,DMAWT);
		break;
	case RDMV:
		D(printf("RDMV"));
		outb(DMABPFF,DMAVRF);
		outb(DMAMODE,DMAVRF);
	}
	/* get work buffer physical address */
	address = kvtophys((vm_offset_t)cip->b_xferaddr);
	dmalen = i386_trunc_page(address) + I386_PGBYTES - address;
	if ( (cip->b_rwerr&MRMASK) >= 0x10)
		dmalen = 0x200;
	if (dmalen<=cip->b_xferdma) 
		cip->b_xferdma = dmalen;
	else
		dmalen = cip->b_xferdma;

#if	CBUS || HIMEM
 	if (fd_remap_addr)
 		address = fd_remap_addr;
#endif	/* CBUS || HIMEM */

	D(printf(" %x L%x ", address, dmalen));
	/* set buffer address */
	outb(DMAADDR,(int)address&BYTEMASK);
	outb(DMAADDR,(((int)address>>8)&BYTEMASK));
	outb(DMAPAGE,(((int)address>>16)&BYTEMASK));
#if	EISA
	if (is_eisa_bus)
		outb(DMA0HIPAGE, (((int)address>>24)&BYTEMASK));
#endif	/* EISA	*/ /* set transfer count */
	outb(DMACNT,(--dmalen)&BYTEMASK);
	outb(DMACNT,((dmalen>>8)&BYTEMASK));
#if	EISA
	if (is_eisa_bus)
		outb(DMA0HICNT, ((dmalen>>16)&BYTEMASK));
#endif	/* EISA	*/ /* set transfer count */
	outb(DMAMSK,CHANNEL2);
	splx(x);
	trfrate(uip, uip->d_drtab.dr_type);	/* set transfer rate */
	data = &cmdp->c_rwdata[0];
	for(cnt0 = 0; cnt0<cmdp->c_dcount; cnt0++,data++){
		if(rtn = fdc_sts(FD_OSTS, uip))	/*status check*/
			break;
		outb(DATAREG(uip->addr), *data);
	}
	if(!rtn){
		cmdp->c_intr |= RWFLAG;
		cmdp->c_stsflag |= INTROUT;
		cnt0 = ((cip->b_buf->b_flags&(B_READ|B_VERIFY)) ==
			(B_READ|B_VERIFY))?TOUT:ITOUT;
		timeout((timeout_fcn_t)fdintr, (void *)(int)uip->dev->ctlr,
			cnt0);
	}
	return(rtn);
}
/*****************************************************************************
 *
 *	sense interrupt status routine
 *
 *****************************************************************************/

int
sis(
	struct unit_info	*uip)
{
	register int	rtn, st0;

	if(rtn = fdc_sts(FD_OSTS, uip))	/* status check */
		return(rtn);
	outb(DATAREG(uip->addr), SISCMD);
	if(rtn = fdc_sts(FD_ISTS, uip))	/* status check */
		return(rtn);
	st0 = inb(DATAREG(uip->addr)) & ST0OK;	/* get st0 */
	if(rtn = fdc_sts(FD_ISTS, uip))	/* status check */
		return(rtn);
	inb(DATAREG(uip->addr));	/* get pcn */
	if (st0&(ST0AT|ST0IC))
		st0 = FDCERR;
	return(st0);
}

/*****************************************************************************
 *
 *	fdc status get routine
 *
 *****************************************************************************/

int
fdc_sts(
	int			mode,
	struct unit_info	*uip)
{
	register int 	ind;
	int		cnt0 = STSCHKCNT;

	while(cnt0--)
		if(((ind=inb(STSREG(uip->addr))) & DATAOK) && 
		   ((ind & DTOCPU) == mode))
			return(0);
	return(TIMEOUT);
}
/*****************************************************************************
 *
 *	motor on routine
 *
 *****************************************************************************/

void
mtr_on(
	struct unit_info	*uip)
{
	struct	fdcmd	*cmdp = uip->b_cmd;

	if(!(mtr_start(uip))){
		timeout((timeout_fcn_t)wakeup,&cmdp->c_stsflag,HZ);
		sleep((char *)&cmdp->c_stsflag,PZERO);
	}
	cmdp->c_stsflag |= MTROFF;
	timeout((timeout_fcn_t)mtr_off,uip,MTRSTOP);
}
/*****************************************************************************
 *
 *	motor start routine
 *
 *****************************************************************************/

int
mtr_start(
	struct unit_info	*uip)
{
	int		status;
	struct	fdcmd	*cmdp = uip->b_cmd;
	int		slave = uip->dev->slave;
	if(cmdp->c_stsflag & MTROFF){
		untimeout((timeout_fcn_t)mtr_off, uip);
		cmdp->c_stsflag &= ~MTROFF;
	}
	status = cmdp->c_rbmtr&(1<<slave);
	cmdp->c_rbmtr |= (1<<slave);
	outb(CTRLREG(uip->addr), ((cmdp->c_rbmtr&MTRMASK)<<MTR_ON)|
				     FDC_RST|slave|DMAREQ);
	return(status);
}
/*****************************************************************************
 *
 *	motor off routine
 *
 *****************************************************************************/

void
mtr_off(
	struct unit_info	*uip)
{
	struct	fdcmd	*cmdp = uip->b_cmd;

	cmdp->c_stsflag &= ~MTROFF;
	if(!(cmdp->c_stsflag&MTRFLAG)){
		cmdp->c_rbmtr &= MTRRST;
		outb(CTRLREG(uip->addr), FDC_RST | DMAREQ);
	}
}
