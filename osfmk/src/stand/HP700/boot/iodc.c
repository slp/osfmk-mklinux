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
/*
 * Console.
 */

#include <sys/param.h>
#include "pdc.h"
#include "iodc.h"
#include "stand.h"
#include <sys/disklabel.h>
#include "saio.h"

extern	int *pdcret;			/* from pdc.c, for PDC returns */

int (*cniodc)() = (int (*)()) 0;	/* console IODC entry point */
int (*kyiodc)() = (int (*)()) 0;	/* keyboard IODC entry point */
int (*btiodc)() = (int (*)()) 0;	/* boot IODC entry point */
int (*ctiodc)() = (int (*)()) 0;	/* cartridge tape IODC entry point */
struct	pz_device ctdev = { 1 };	/* cartridge tape (boot) device path */

#define	IONBPG		(2 * 1024)	/* page alignment for I/O buffers */
#define	IOPGSHIFT	11		/* LOG2(IONBPG) */
#define	IOPGOFSET	(IONBPG-1)	/* byte offset into I/O buffer */

#define	MINIOSIZ	64		/* minimum buffer size for IODC call */
#define	MAXIOSIZ	(64 * 1024)	/* maximum buffer size for IODC call */
#define	BTIOSIZ		(8 * 1024)	/* size of boot device I/O buffer */

#define	CN_HPA		PAGE0->mem_cons.pz_hpa
#define	CN_SPA		PAGE0->mem_cons.pz_spa
#define	CN_LAYER	PAGE0->mem_cons.pz_layers
#define	CN_IODC		PAGE0->mem_cons.pz_iodc_io
#define	CN_CLASS	PAGE0->mem_cons.pz_class

#define	KY_HPA		PAGE0->mem_kbd.pz_hpa
#define	KY_SPA		PAGE0->mem_kbd.pz_spa
#define	KY_LAYER	PAGE0->mem_kbd.pz_layers
#define	KY_IODC		PAGE0->mem_kbd.pz_iodc_io
#define	KY_CLASS	PAGE0->mem_kbd.pz_class

#define	BT_HPA		PAGE0->mem_boot.pz_hpa
#define	BT_SPA		PAGE0->mem_boot.pz_spa
#define	BT_LAYER	PAGE0->mem_boot.pz_layers
#define	BT_IODC		PAGE0->mem_boot.pz_iodc_io
#define	BT_CLASS	PAGE0->mem_boot.pz_class

#define	ANYSLOT		(-1)
#define	NOSLOT		(-2)

static char btbuf[BTIOSIZ + MINIOSIZ];
char *btdata = 0;
int HP800 = -1;	

btinit()
{
	int err;
	static int firstime = 1;

	btiodc = (int (*)()) ((u_int)PAGE0->mem_free + IODC_MAXSIZE);
	btdata = (char *) ((((int) &btbuf[0]) + MINIOSIZ-1) & ~(MINIOSIZ-1));

	if (firstime) {
		/*
		 * If we _rtt(), we will call btinit() again.
		 * We only want to do ctdev initialization once.
		 */
		bcopy((char *)&PAGE0->mem_boot, (char *)&ctdev,
		      sizeof(struct pz_device));
		firstime = 0;
	}

	/*
	 * Initialize "HP800" to boolean value (T=HP800 F=HP700).
	 */
	if (HP800 == -1) {
		struct pdc_model model;
		err = (*pdc)(PDC_MODEL, PDC_MODEL_INFO, &model, 0,0,0,0,0);
		if (err < 0) {
			HP800 = 1;	/* default: HP800 */
			printf("Proc model info ret'd %d (assuming %s)\n",
			       err, HP800? "HP800": "HP700");
		}
		HP800 = (((model.hvers >> 4) & 0xfff) < 0x200);
	}
}

int
btunit()
{
	return(HP800? BT_LAYER[1]: BT_LAYER[0]);
}

dkreset(slot, unit)
	int slot, unit;
{
	struct device_path bootdp;
	int err, srchtype;

	/*
	 * Save a copy of the previous boot device path.
	 */
	bcopy((char *)&PAGE0->mem_boot.pz_dp, (char *)&bootdp,
	      sizeof(struct device_path));

	/*
	 * Read the boot device initialization code into memory.
	 */
	err = (*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, BT_HPA, IODC_INIT,
	             btiodc, IODC_MAXSIZE);
	if (err < 0) {
		printf("Boot module ENTRY_INIT Read ret'd %d\n", err);
		goto bad;
	}

	/*
	 * Plod over boot devices looking for one with the same unit
	 * number as that which is in `unit'.
	 */
	srchtype = IODC_INIT_FIRST;
	while (1) {
		err = (*btiodc)(BT_HPA,srchtype,BT_SPA,BT_LAYER,pdcret,0,0,0,0);
		if (err < 0) {
			if (err == -9) {
				BT_IODC = 0;
				return(EUNIT);
			}
			printf("Boot module ENTRY_INIT Search ret'd %d\n", err);
			goto bad;
		}

		srchtype = IODC_INIT_NEXT;	/* for next time... */

		if (pdcret[1] != PCL_RANDOM)	/* only want disks */
			continue;

		if (HP800) {
			if (slot != ANYSLOT && slot != BT_LAYER[0])
				continue;

			if (BT_LAYER[1] == unit) {
				BT_CLASS = pdcret[1];
				break;
			}
		} else {
			if (slot != NOSLOT)
				continue;

			if (BT_LAYER[0] == unit) {
				BT_CLASS = pdcret[1];
				break;
			}
		}
	}

	/*
	 * If this is not the "currently initialized" boot device,
	 * initialize the new boot device we just found.
	 *
	 * N.B. We do not need/want to initialize the entire module
	 * (e.g. CIO, SCSI), and doing so may blow away our console.
	 * if the user specified a boot module other than the
	 * console module, we initialize both the module and device.
	 */
	if (bcmp((char *)&PAGE0->mem_boot.pz_dp, (char *)&bootdp,
	         sizeof(struct device_path)) != 0) {
		err = (*btiodc)(BT_HPA,(!HP800||BT_HPA==CN_HPA||BT_HPA==KY_HPA)?
		                IODC_INIT_DEV: IODC_INIT_ALL,
		                BT_SPA, BT_LAYER, pdcret, 0,0,0,0);
		if (err < 0) {
			printf("Boot module/device IODC Init ret'd %d\n", err);
			goto bad;
		}
	}

	err = (*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, BT_HPA, IODC_IO,
	             btiodc, IODC_MAXSIZE);
	if (err < 0) {
		printf("Boot device ENTRY_IO Read ret'd %d\n", err);
		goto bad;
	}

	BT_IODC = (int (*)(struct iomod*, ...))btiodc;
	return (0);
bad:
	BT_IODC = 0;
	return(-1);
}

/*
 * Generic READ/WRITE through IODC.  Takes pointer to PDC device
 * information, returns (positive) number of bytes actually read or
 * the (negative) error condition, or zero if at "EOF".
 */
iodc_rw(maddr, daddr, count, func, pzdev)
	caddr_t maddr; /* io->i_ma = labelbuf */
	u_int daddr;
	u_int count;   /* io->i_cc = DEV_BSIZE */
	int func;      
	struct pz_device *pzdev;
{
	register int	offset;
	register int	xfer_cnt = 0;
	register int	ret;

	if (pzdev == 0)		/* default: use BOOT device */ 
		pzdev = &PAGE0->mem_boot;
	
	if (pzdev->pz_iodc_io == 0)
		return(-1);

	/*
	 * IODC arguments are constrained in a number of ways.  If the
	 * request doesn't fit one or more of these constraints, we have
	 * to do the transfer to a buffer and copy it.
	 */
	if ((((int)maddr)&(MINIOSIZ-1))||(count&IOPGOFSET)||(daddr&IOPGOFSET))
	    for (; count > 0; count -= ret, maddr += ret, daddr += ret) {
		offset = daddr & IOPGOFSET;
		ret = (*pzdev->pz_iodc_io)(pzdev->pz_hpa,
			(func == F_READ)? IODC_IO_BOOTIN: IODC_IO_BOOTOUT,
			pzdev->pz_spa, pzdev->pz_layers, pdcret,
			daddr - offset, btdata, BTIOSIZ, BTIOSIZ);
		if (ret < 0)
			return (ret);
		ret = pdcret[0];
		if (ret == 0)
			break;
		ret -= offset;
		if (ret > count)
			ret = count;
		bcopy(btdata + offset, maddr, ret);
		xfer_cnt += ret;
	    }
	else 
	    for (; count > 0; count -= ret, maddr += ret, daddr += ret) {
		if ((offset = count) > MAXIOSIZ)
			offset = MAXIOSIZ;
		ret = (*pzdev->pz_iodc_io)(pzdev->pz_hpa,
			(func == F_READ)? IODC_IO_BOOTIN: IODC_IO_BOOTOUT,
			pzdev->pz_spa, pzdev->pz_layers, pdcret, daddr,
			maddr, offset, count);
		if (ret < 0) 
			return (ret);
		ret = pdcret[0];
		if (ret == 0)
			break;
		xfer_cnt += ret;
	    }
	return (xfer_cnt);
}

dkopen(f, adapt, ctlr, unit, part, type)
	struct open_file *f;
	int adapt, ctlr, unit, part, type;
{
	static struct disklabel dklabel;
	struct disklabel *lp;
	int dkstrategy();
	struct iob *io;
	int i;

	if (f->f_devdata == 0) 
		f->f_devdata = alloc(sizeof *io);
	io = f->f_devdata;
	bzero(io, sizeof *io);
	io->i_adapt = adapt;
	io->i_ctlr = ctlr;
	io->i_unit = unit;
	io->i_part = part;
	io->i_dev = type;
	lp = &dklabel;
	bzero(lp, sizeof *lp);
	
	if ((i = readdisklabel(io, dkstrategy, lp)) != 0) {
		return (i);
	}

	i = io->i_part;
	if ((u_int)i >= lp->d_npartitions || lp->d_partitions[i].p_size == 0) {
		return (EPART);
	}
	io->i_boff = lp->d_partitions[i].p_offset;

	return (0);
}

dkstrategy(io, func)
	register struct iob *io;
	int func;
{
	int i;
	char *c;
	int ret;
	u_int bn = io->i_bn + io->i_boff;

	ret = iodc_rw(io->i_ma,
	              (bn << DEV_BSHIFT) + ((io->i_flgs & F_SSI)? 256: 0),
	              io->i_cc, func, &PAGE0->mem_boot);
	if (ret < 0) {
		printf("%s: iodc ret'd %d\n", devsw[io->i_dev].dv_name, ret);
		return (-1);
	}
	
	return (ret);
}

rdopen(f, adapt, ctlr, unit, part)
	struct open_file *f;
	int ctlr, unit, part;
{
	extern int rddevix;
	int i;

	if ((i = dkreset(ANYSLOT, unit)) != 0)
		return (i < 0 ? EIO : i);

	return (dkopen(f, adapt, ctlr, unit, part, rddevix));
}

rdstrategy(io, func, dblk, size, buf, rsize)
	register struct iob *io;
	int func;
	daddr_t dblk;
	u_int size;
	char *buf;
	u_int *rsize;
{
	int cc;

	io->i_bn = dblk;
	io->i_cc = size;
	io->i_ma = buf;
	cc = dkstrategy(io, func);
	if (cc < 0)
		return (EIO);
	*rsize = (u_int) cc;
	return (0);
}

flopen(f, adapt, ctlr, unit, part)
	struct open_file *f;
	int adapt, ctlr, unit, part;
{
	int i;
	extern int fldevix;

	if ((i = dkreset(6, unit)) != 0)
		return (i < 0 ? EIO : i);

	return (dkopen(f, adapt, ctlr, unit, part, fldevix));
}

flstrategy(io, func, dblk, size, buf, rsize)
	register struct iob *io;
	int func;
	daddr_t dblk;
	u_int size;
	char *buf;
	u_int *rsize;
{
	int cc;

	io->i_bn = dblk;
	io->i_cc = size;
	io->i_ma = buf;
	cc = dkstrategy(io, func);
	if (cc < 0)
		return (EIO);
	*rsize = (u_int) cc;
	return (0);
}

sdopen(f, adapt, ctlr, unit, part)
	struct open_file *f;
	int adapt, ctlr, unit, part;
{
	register int i;
	extern int sddevix;

	if ((i = dkreset(NOSLOT, unit)) != 0){
		return (i < 0 ? EIO: i);
	}

	return (dkopen(f, adapt, ctlr, unit, part, sddevix));
}

sdstrategy(io, func, dblk, size, buf, rsize)
	register struct iob *io;
	int func;
	daddr_t dblk;
	u_int size;
	char *buf;
	u_int *rsize;
{
	int cc;

	io->i_bn = dblk;
	io->i_cc = size;
	io->i_ma = buf;
	cc = dkstrategy(io, func);
	if (cc < 0)
		return (EIO);
	*rsize = (u_int) cc;
	return (0);
}

/* hp800-specific comments:
 *
 * Tape driver ALWAYS uses "Alternate Boot Device", which is assumed to ALWAYS
 * be the boot device in pagezero (meaning we booted from it).
 *
 * NOTE about skipping file, below:  It's assumed that a read gets 2k (a page).
 * This is done because, even though the cartridge tape has record sizes of 1k,
 * and an EOF takes one record, reads through the IODC must be in 2k chunks,
 * and must start on a 2k-byte boundary.  This means that ANY TAPE FILE TO BE
 * SKIPPED OVER IS GOING TO HAVE TO BE AN ODD NUMBER OF 1 KBYTE RECORDS so the
 * read of the subsequent file can start on a 2k boundary.  If a real error
 * occurs, the record count is reset below, so this isn't a problem.
 */
int	ctbyteno;	/* block number on tape to access next */
int	ctworking;	/* flag:  have we read anything successfully? */

ctopen(f, adapt, ctlr, unit, part)
	struct open_file *f;
	int adapt, ctlr, unit, part;
{
	int i, ret;

	if (ctiodc == 0) {
		static int ctcode[IODC_MAXSIZE/sizeof(int)] = { 1 }; /* !BSS */

		ret = (*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, ctdev.pz_hpa,
		             IODC_IO, ctcode, IODC_MAXSIZE);
		if (ret < 0) {
			printf("ct: device ENTRY_IO Read ret'd %d\n", ret);
			return (EIO);
		} else {
			ctdev.pz_iodc_io = (int (*)(struct iomod*, ...))(ctiodc
				= (int (*)()) ctcode);
		}
	}

	if (ctiodc != NULL) {
		ret = (*ctiodc)(ctdev.pz_hpa, IODC_IO_BOOTIN, ctdev.pz_spa,
			ctdev.pz_layers, pdcret, 0, btdata, 0, 0);
		if (ret != 0)
			printf("ct: device rewind ret'd %d\n", ret);
	}
	ctbyteno = 0;
	for (i = part; --i >= 0; ) {
		ctworking = 0;
		for (;;) {
			ret = iodc_rw(btdata, ctbyteno, IONBPG, F_READ, &ctdev);
			ctbyteno += IONBPG;
			if (ret <= 0)
				break;
			ctworking = 1;
		}
		if (ret < 0 && (ret != -4 || !ctworking)) {
			printf("ct: error %d after %d %d-byte records\n",
				ret, ctbyteno >> IOPGSHIFT, IONBPG);
			ctbyteno = 0;
			ctworking = 0;
			return (EIO);
		}
	}
	ctworking = 0;
	return (0);
}

/*ARGSUSED*/
ctclose(f)
	struct open_file *f;
{
	ctbyteno = 0;
	ctworking = 0;
}

ctstrategy(io, func, dblk, size, buf, rsize)
	register struct iob *io;
	int func;
	daddr_t dblk;
	u_int size;
	char *buf;
	u_int rsize;
{
	int ret;

	ret = iodc_rw(buf, ctbyteno, size, func, &ctdev);
	if (ret < 0) {
		if (ret == -4 && ctworking)
			ret = 0;

		ctworking = 0;
	} else {
		ctworking = 1;
		ctbyteno += ret;
	}
	return (ret);
}

/*
 * Console.
 */

char *cndata = 0;

cninit()
{
	static char cnbuf[MINIOSIZ * 2];
	int err;

	cndata = (char *) ((((int) &cnbuf[0]) + MINIOSIZ-1) & ~(MINIOSIZ-1));

	cniodc = (int (*)()) PAGE0->mem_free;

	err = (*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, CN_HPA, IODC_INIT,
	             cniodc, IODC_MAXSIZE);
	if (err < 0)
		goto bad;

	err = (*cniodc)(CN_HPA, (CN_HPA==BT_HPA)? IODC_INIT_DEV: IODC_INIT_ALL,
	                CN_SPA, CN_LAYER, pdcret, 0,0,0,0);
	if (err < 0)
		goto bad;

	err = (*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, CN_HPA, IODC_IO,
	             cniodc, IODC_MAXSIZE);
	if (err < 0)
		goto bad;

	/*
	 * If the keyboard is separate from the console output device,
	 * we load the keyboard code at `kycode'.
	 *
	 * N.B. In this case, since the keyboard code is part of the
	 * boot code, it will be overwritten when we load a kernel.
	 */
	if (CN_CLASS != PCL_DUPLEX || KY_CLASS == PCL_KEYBD) {
		static int kycode[IODC_MAXSIZE/sizeof(int)];

		kyiodc = (int (*)()) kycode;

		err = (*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, KY_HPA,
		             IODC_INIT, kyiodc, IODC_MAXSIZE);
		if (err < 0)
			goto badky;

		err = (*kyiodc)(KY_HPA, (KY_HPA == BT_HPA || KY_HPA == CN_HPA)?
		                IODC_INIT_DEV: IODC_INIT_ALL,
		                KY_SPA, KY_LAYER, pdcret, 0,0,0,0);
		if (err < 0)
			goto badky;

		err = (*pdc)(PDC_IODC, PDC_IODC_READ, pdcret, KY_HPA,
		             IODC_IO, kyiodc, IODC_MAXSIZE);
		if (err < 0) {
badky:
			kyiodc = (int (*)()) 0;
		}
	} else {
		kyiodc = cniodc;

		bcopy((char *)&PAGE0->mem_cons, (char *)&PAGE0->mem_kbd,
		      sizeof(struct pz_device));
	}

	CN_IODC = (int (*)(struct iomod*, ...))cniodc;
	KY_IODC = (int (*)(struct iomod*, ...))kyiodc;

	return (0);
bad:
	/* morse code with the LED's?!! */
	CN_IODC = KY_IODC = 0;
	return(err);
}

void
cnputc(c)
char c;
{
	*cndata = c;

	if (CN_IODC == 0)
		return;

	(void) (*cniodc)(CN_HPA, IODC_IO_CONSOUT, CN_SPA, CN_LAYER,
	                 pdcret, 0, cndata, 1, 0);
}

cngetc()
{
	int err;

	if (KY_IODC == 0)
		return(0x100);

	err = (*kyiodc)(KY_HPA, IODC_IO_CONSIN, KY_SPA, KY_LAYER,
	                pdcret, 0, cndata, 1, 0);
	if (err < 0)
		panic("KBD input error: %d", err);

	if (pdcret[0] == 0)
		return (0);

#if 0
	if (pdcret[0] > 1)
		printf("KBD input got too much (%d)\n", pdcret[0]);
#endif

	if (*cndata == 0)		/* is this even possible? */
		return (0x100);

	return (*cndata);
}
