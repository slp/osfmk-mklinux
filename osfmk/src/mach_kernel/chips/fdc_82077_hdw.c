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
 * Copyright (c) 1992 Carnegie Mellon University
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
/* CMU_HIST */
/*
 * Revision 2.2  92/03/02  18:32:45  rpd
 * 	Created, initial rough cut at init and probe only.
 * 	[92/01/19            af]
 * 
 */
/* CMU_ENDHIST */
/*
 *	File: fdi_82077_hdw.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	1/92
 *
 *	Driver for the Intel 82077 Floppy Disk Controller.
 */

#include <fd.h>
#if	NFD > 0

#include <kern/spl.h>
#include <mach/std_types.h>
#include <sys/types.h>
#include <chips/busses.h>

#include <chips/fdc_82077.h>
#include <platforms.h>

#ifdef	MAXINE

/* we can only take one */
#define	MAX_DRIVES		1

#define	my_fdc_type	fdc_82077aa
#define	the_fdc_type	fd->fdc_type
/* later: #define the_fdc_type my_fdc_type */

/* Registers are read/written as words, byte 0 */
/* padding is to x40 boundaries */
typedef struct {
	volatile unsigned int	fd_sra;		/* r:  status register A */
	int pad0[15];
	volatile unsigned int	fd_srb;		/* r:  status register B */
	int pad1[15];
	volatile unsigned int	fd_dor;		/* rw: digital output reg */
	int pad2[15];
	volatile unsigned int	fd_tdr;		/* rw: tape drive register */
	int pad3[15];
	volatile unsigned int	fd_msr;		/* r:  main status register */
/*#define			fd_dsr	fd_msr;	/* w:  data rate select reg */
	int pad4[15];
	volatile unsigned int	fd_data;	/* rw: fifo */
	int pad5[15];
	volatile unsigned int	fd_xxx;		/* --reserved-- */
	int pad6[15];
	volatile unsigned int	fd_dir;		/* r:  digital input reg */
/*#define			fd_ccr	fd_dir;	/* w:  config control reg */
} fd_padded_regmap_t;

#define	machdep_reset_8272a(f,r)

#else	/* MAXINE */

/* Pick your chip and padding */
#define	my_fdc_type		fdc_8272a
#define the_fdc_type		my_fdc_type

#define	fd_padded_regmap_t	fd_8272a_regmap_t

#define	machdep_reset_8272a(f,r)	1

#endif	/* MAXINE */


#ifndef	MAX_DRIVES
#define	MAX_DRIVES	DRIVES_PER_FDC
#endif

/*
 * Autoconf info
 */

static caddr_t fd_std[NFD] = { 0 };
static struct bus_device *fd_info[NFD];
static struct bus_ctlr	 *fd_minfo[NFD];
static int fd_probe(), fd_slave(), fd_attach(), fd_go();

struct bus_driver fd_driver =
       { fd_probe, fd_slave, fd_attach, fd_go, fd_std, "fd", fd_info,
         "fdc", fd_minfo, /*BUS_INTR_B4_PROBE*/};

/*
 * Externally visible functions
 */
int	fd_intr();					/* kernel */

/*
 * Software status of chip
 */
struct fd_softc {
	fd_padded_regmap_t	*regs;
	char			fdc_type;
	char			fdc_mode;
	char			messed_up;
	char			slave_active;
	char			bytes_expected;
	struct {
		/* status at end of last command */
		unsigned char	st0;
		unsigned char	st1;
		unsigned char	st2;
		unsigned char	c;
		unsigned char	h;
		unsigned char	r;
		unsigned char	n;
		unsigned char	st3;
		/* ... */
		unsigned char	medium_status;
#		define	ST_MEDIUM_PRESENT	1
#		define	ST_MEDIUM_KNOWN		2

	} drive_status[DRIVES_PER_FDC];
} fd_softc_data[NFD];

typedef struct fd_softc	*fd_softc_t;

fd_softc_t	fd_softc[NFD];

static char *chip_names[4] = { "8272-A", "82072", "82077-AA", 0 };
static char *mode_names[4] = { "PC AT", "PS/2", "Model 30", 0 };

/*
 * Probe chip to see if it is there
 */
static fd_probe (reg, ctlr)
	vm_offset_t	reg;
	struct bus_ctlr *ctlr;
{
	int		unit = ctlr->unit;
	fd_softc_t	fd;
	fd_padded_regmap_t	*regs;

	/*
	 * See if we are here
	 */
	if (check_memory(reg, 0)) {
		/* no rides today */
		return 0;
	}

	fd = &fd_softc_data[unit];
	fd_softc[unit] = fd;

	regs = (fd_padded_regmap_t *)reg;
	fd->regs = regs;
	fd->fdc_type = my_fdc_type;

	fd_reset(fd);

	if (the_fdc_type == fdc_82077aa) {
		/* See if properly functioning */
		unsigned char	temp = FD_CMD_VERSION;
		if (!fd_command(fd, &temp, 1))
			return 0;	/* total brxage */
		if (!fd_get_result(fd, &temp, 1, FALSE))
			return 0;	/* partial brxage */
		if (temp != FD_VERSION_82077AA)
			printf( "{ %s x%x } ",
				"Accepting non-82077aa version id",
				temp);
	}

	printf("%s%d: %s chip controller",
		ctlr->name, ctlr->unit, chip_names[fd->fdc_type]);
	if (the_fdc_type == fdc_82077aa)
		printf(" in %s mode", mode_names[fd->fdc_mode]);
	printf(".\n");

	return 1;
}

/* See if we like this slave */
static fd_slave(ui, reg)
	struct bus_device	*ui;
	vm_offset_t		reg;
{
	int			slave = ui->slave;
	fd_softc_t		fd;
	unsigned char		sns[2];

	if (slave >= MAX_DRIVES) return 0;

	fd = fd_softc[ui->ctlr];

	sns[0] = FD_CMD_SENSE_DRIVE_STATUS;
	sns[1] = slave & 0x3;
	if (the_fdc_type == fdc_82072)
		sns[1] |= FD_CMD_SDS_NO_MOT;
	if (!fd_command(fd, sns, 2)) return 0;
	if (!fd_get_result(fd, sns, 1, FALSE)) return 0;

	fd->drive_status[slave].st3 = sns[0];

	return 1;
}

static fd_attach (ui)
	struct bus_device *ui;
{
	/* Attach a slave */
}

static fd_go()
{
}

fd_intr (unit, spllevel)
{
	fd_softc_t      fd;
	fd_padded_regmap_t *regs;

	splx(spllevel);

	fd = fd_softc[unit];
	regs = fd->regs;
#if 1
	regs->fd_dor = 0;

	printf("fd%d: stray intr, disabled", unit);
#else
	/* did polling see a drive change */
	/* busy bit in msr sez ifasync or not */

	msr = regs->fd_msr;
	if (msr & FD_MSR_BSY) {
		/* result phase */
		fd_get_result(fd, &fd->drive_status[fd->slave_active],
			      fd->bytes_expected, FALSE);
		/* .... */
		return;
	}
	/* async interrupt, either seek complete or media change */
	while (1) {
		unsigned char   st[2];

		*st = FD_CMD_SNS_INT_STATUS;
		fd_command(fd, st, 1);

		fd_get_result(fd, st, 2, FALSE);

		slave = *st & FD_ST0_DS;

		switch (*st & FD_ST0_IC_MASK) {

		    case FD_ST0_IC_OK:
		    case FD_ST0_IC_AT:

		    case FD_ST0_IC_BAD_CMD:
			return;

		    case FD_ST0_IC_AT_POLL:
			m = fd->drive_status[slave].medium_status;
			if (m & ST_MEDIUM_PRESENT)
				m &= ~ST_MEDIUM_PRESENT;
			else
				m |= ST_MEDIUM_PRESENT;
			fd->drive_status[slave].medium_status = m;
		}
	}
#endif
}

/*
 * Non-interface functions and utilities
 */

fd_reset(fd)
	fd_softc_t	fd;
{
	register	fd_padded_regmap_t	*regs;

	regs = fd->regs;

	/*
	 * Reset the chip
	 */
	if (the_fdc_type == fdc_82072)
		/* Fix if your box uses an external PLL */
		regs->fd_dsr = FD_DSR_RESET | FD_DSR_EPLL;
	else if (the_fdc_type == fdc_82077aa)
		regs->fd_dor = 0;
	else
		machdep_reset_8272a(fd, regs);

	delay(5);	/* 4usecs in specs */

	/*
	 * Be smart with the smart ones
	 */
	if (the_fdc_type == fdc_82077aa) {

		/*
		 * See in which mood we are (it cannot be changed)
		 */
		int	temp;

		/* Take chip out of hw reset */
		regs->fd_dor = FD_DOR_ENABLE | FD_DOR_DMA_GATE;
		delay(10);

		/* what do we readback from the DIR register as datarate ? */
		regs->fd_ccr = FD_DSR_SD_125;
		delay(10);

		temp = regs->fd_dir;
		if ((temp & 0x7) == FD_DSR_SD_125)
			fd->fdc_mode = mod30_mode;
		else if ((temp & (FD_DIR_ones | FD_DIR_DR_MASK_PS2)) ==
			 ((FD_DSR_SD_125 << FD_DIR_DR_SHIFT_PS2) | FD_DIR_ones))
			fd->fdc_mode = ps2_mode;
		else
			/* this assumes tri-stated bits 1&2 read the same */
			fd->fdc_mode = at_mode;

	}

	/*
	 * Send at least 4 sense interrupt cmds, one per drive
	 */
	{

		unsigned char	sns, st[2];
		int		i, nloops;

		sns = FD_CMD_SENSE_INT_STATUS;
		i   = nloops = 0;

		do {
			nloops++;

			(void) fd_command(fd, &sns, 1);

			st[0] = 0; /* in case bad status */
			(void) fd_get_result(fd, st, 2, TRUE);

			if ((st[0] & FD_ST0_IC_MASK) == FD_ST0_IC_AT_POLL) {
				register int 	drive;

				drive = st[0] & FD_ST0_DS;
				fd->drive_status[drive].st0 = st[0];
				fd->drive_status[drive].c   = st[1];
				i++;
			}
		} while ( (nloops < 30) &&
			  ((i < 4) || (st[0] != FD_ST0_IC_BAD_CMD)) );

		/* sanity check */
		if (nloops == 30) {
			(void) fd_messed_up(fd);
			return;
		}
	}

	/*
	 * Install current parameters
	 */
	if (the_fdc_type != fdc_8272a) {

		unsigned char	cnf[4];

		/* send configure command to turn polling off */
		cnf[0] = FD_CMD_CONFIGURE;
		cnf[1] = 0x60; /* moff 110 */
		cnf[2] = 0x0f; /* poll, thr=16 */
		cnf[3] = 0;
		if (!fd_command(fd, cnf, 4))
			return;
		/* no status */
	}

	/*
	 * Send specify to select defaults
	 */
	{
		unsigned char	sfy[3];

		sfy[0] = FD_CMD_SPECIFY;
		sfy[1] = (12 << 4) | 7; /* step 4, hut 112us @500 */
		sfy[2] = 2 << 1; /* hlt 29us @500 */
		(void) fd_command(fd, sfy, 3);
		/* no status */
	}
}

#define	FD_MAX_WAIT	1000

boolean_t
fd_command(fd, cmd, cmd_len)
	fd_softc_t	fd;
	char		*cmd;
{
	register fd_padded_regmap_t	*regs;

	regs = fd->regs;

	while (cmd_len > 0) {
		register int i, s;

		/* there might be long delays, so we pay this price */
		s = splhigh();
		for (i = 0; i < FD_MAX_WAIT; i++)
			if ((regs->fd_msr & (FD_MSR_RQM|FD_MSR_DIO)) ==
			    FD_MSR_RQM)
				break;
			else
				delay(10);
		if (i == FD_MAX_WAIT) {
			splx(s);
			return fd_messed_up(fd);
		}
		regs->fd_data = *cmd++;
		splx(s);
		if (--cmd_len) delay(12);
	}

	return TRUE;
}

boolean_t
fd_get_result(fd, st, st_len, ignore_errors)
	fd_softc_t	fd;
	char		*st;
{
	register fd_padded_regmap_t	*regs;

	regs = fd->regs;

	while (st_len > 0) {
		register int i, s;

		/* there might be long delays, so we pay this price */
		s = splhigh();
		for (i = 0; i < FD_MAX_WAIT; i++)
			if ((regs->fd_msr & (FD_MSR_RQM|FD_MSR_DIO)) ==
			    (FD_MSR_RQM|FD_MSR_DIO))
				break;
			else
				delay(10);
		if (i == FD_MAX_WAIT) {
			splx(s);
			return (ignore_errors) ? FALSE : fd_messed_up(fd);
		}
		*st++ = regs->fd_data;
		splx(s);
		st_len--;
	}

	return TRUE;
}


boolean_t
fd_messed_up(fd)
	fd_softc_t	fd;
{
	fd->messed_up++;
	printf("fd%d: messed up, disabling.\n", fd - fd_softc_data);
	/* here code to 
		ior->error = ..;
		restart
	 */
	return FALSE;
}

/*
 * Debugging aids
 */

fd_state(unit)
{
	fd_softc_t	fd = fd_softc[unit];
	fd_padded_regmap_t	*regs;

	if (!fd || !fd->regs) return 0;
	regs = fd->regs;
	if (the_fdc_type == fdc_8272a)
		printf("msr %x\n", regs->fd_msr);
	else
		printf("sra %x srb %x dor %x tdr %x msr %x dir %x\n",
			regs->fd_sra, regs->fd_srb, regs->fd_dor,
			regs->fd_tdr, regs->fd_msr, regs->fd_dir);
}

#endif

/*   to be moved in separate file, or the above modified to live with scsi */

#include <device/param.h>
#include <device/io_req.h>
#include <device/device_types.h>

fd_open(dev, mode, ior)
	int		dev;
	dev_mode_t	mode;
	io_req_t	ior;
{
	/* find out what medium we have */
}

fd_close(dev)
	int		dev;
{
	/* do not delete media info, do that ifinterrupt sez changed */
}

fd_strategy(ior)
	io_req_t	ior;
{
}
extern minphys();

fd_read(dev, ior)
	int		dev;
	io_req_t	ior;
{
	return block_io(fd_strategy, minphys, ior);
}

fd_write(dev, ior)
	int		dev;
	io_req_t	ior;
{
/*	check if writeable */

	return block_io(fd_strategy, minphys, ior);
}

fd_set_status(dev, flavor, status, status_count)
	int		dev;
	int		flavor;
	dev_status_t	status;
	unsigned int	*status_count;
{}

fd_get_status(dev, flavor, status, status_count)
	int		dev;
	int		flavor;
	dev_status_t	status;
	unsigned int	status_count;
{}

