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
 * Revision 2.12.2.1  92/03/28  10:15:49  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:32:37  jeffreyh]
 * 
 * Revision 2.13  92/02/23  22:44:40  elf
 * 	Changed the interface of a number of functions not to
 * 	require the scsi_softc pointer any longer.  It was
 * 	mostly unused, now it can be found via tgt->masterno.
 * 	[92/02/22  19:30:58  af]
 * 
 * Revision 2.12  91/08/24  12:28:21  af
 * 	Spls defs, corrected scsi_master_alloc() to allocate
 * 	as expected, support for processor devices, 
 * 	made it possible to avoid sync neg at all (for rb's sake),
 * 	no start unit on processor devices, understand our
 * 	own descriptor.
 * 	[91/08/02  04:01:50  af]
 * 
 * Revision 2.11  91/07/09  23:22:46  danner
 * 	Added gross luna88k ifdef to use <sd.h> instead of <scsi.h>. Will be
 * 	fixed when I understand how to use the configuration tools.
 * 	[91/07/09  11:08:15  danner]
 * 
 * Revision 2.10  91/06/25  20:56:48  rpd
 * 	Tweaks to make gcc happy.
 * 
 * Revision 2.9  91/06/19  11:57:17  rvb
 * 	mips->DECSTATION; vax->VAXSTATION
 * 	[91/06/12  14:02:21  rvb]
 * 
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/04            rvb]
 * 
 * 	Printing of 'unsupported..' in scsi_slave was screwed cuz
 * 	ui->mi and ui->unit could be bogus at that point in time.
 * 	[91/05/30            af]
 * 
 * Revision 2.8  91/05/14  17:28:00  mrt
 * 	Correcting copyright
 * 
 * Revision 2.7  91/05/13  06:04:45  af
 * 	Added flags to control use of disconnect-reconnect mode.
 * 	Added device-specific close routine (for tapes).
 * 	Wait for start_unit to complete properly and tell the user
 * 	what we are waiting for.
 * 	[91/05/12  16:21:16  af]
 * 
 * Revision 2.6.1.1  91/03/29  16:59:15  af
 * 	Added flags to control use of disconnect-reconnect mode.
 * 	Added device-specific close routine (for tapes).
 * 	Wait for start_unit to complete properly and tell the user
 * 	what we are waiting for.
 * 
 * Revision 2.6  91/02/05  17:44:35  mrt
 * 	Added author notices
 * 	[91/02/04  11:18:01  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:16:40  mrt]
 * 
 * Revision 2.5  90/12/05  23:34:36  af
 * 	Cleanups, still awaits SCSI-2 fixes.
 * 	[90/12/03  23:38:14  af]
 * 
 * Revision 2.3.1.1  90/11/01  03:38:17  af
 * 	Created.
 * 	[90/09/03            af]
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
 *	File: scsi.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	9/90
 *
 *	Middle layer of the SCSI driver: chip independent functions
 *	This file contains Controller and Device-independent functions
 */

#include <scsi.h>

#if	NSCSI > 0
#include <scsiinfo.h>
#include <norma_scsi.h>
#include <platforms.h>
#include <kern/spl.h>		/*  spl*() definitions */
#include <mach/std_types.h>
#include <types.h>
#include <scsi/compat_30.h>

#include <device/subrs.h>
#include <device/ds_routines.h>
#include <chips/busses.h>
#include <scsi/scsi.h>
#include <scsi/scsi2.h>
#include <scsi/scsi_defs.h>
#include <scsi2.h>		/* NSCSI2 */
#include <machine/machparam.h>
#include <kern/misc_protos.h>
#include <string.h>
#include <device/conf.h>

/*
 *	Overall driver state
 */

target_info_t	scsi_target_data[NSCSI*8];	/* per target state */
scsi_softc_t	scsi_softc_data[NSCSI];		/* per HBA state */
scsi_softc_t	*scsi_softc[NSCSI];		/* quick access&checking */

int		scsi_sequential_disk_indirect_count; /* for device aliases */
int		scsi_sequential_tape_indirect_count; /* for device aliases */

/*
 * If a specific target should NOT be asked to go synchronous
 * then its bit in this bitmap should be set. Each SCSI controller
 * (Host Bus Adapter) can hold at most 8 targets --> use one
 * byte per controller.  A bit set to one means NO synchronous.
 * Patch with adb if necessary.
 */
unsigned char	scsi_no_synchronous_xfer[NSCSI];

/*
 * For certain targets it is wise to use the long form of the
 * read/write commands even if their capacity would not necessitate
 * it.  Same as above for usage.
 */
unsigned char	scsi_use_long_form[NSCSI];


/*
 * Control over disconnect-reconnect mode.
 */
#if defined(__alpha)
#define HBA_ID	6
#else
#define HBA_ID	7
#endif
unsigned char	scsi_might_disconnect[NSCSI]; 	/* do it if deemed appropriate */
unsigned char	scsi_should_disconnect[NSCSI];	/* just do it */
unsigned char	scsi_initiator_id[NSCSI] =	/* our id on the bus(ses) */
		{ HBA_ID
#if NSCSI > 1
		 ,HBA_ID
#endif
		};


/*
 * Miscellaneus config
 */
boolean_t	scsi_exabyte_filemarks = FALSE; /* use short filemarks */
int		scsi_watchdog_period = 10;	/* but exabyte needs >=30 for bspace */
#if	PARAGON860
int		scsi_delay_after_reset = 5000000;/* microseconds */
#else	/* PARAGON860 */
int		scsi_delay_after_reset = 1000000;/* microseconds */
#endif	/* PARAGON860 */
boolean_t	scsi_no_automatic_bbr = FALSE;	/* revector bad blocks automatically */

#ifdef	MACH_KERNEL
#else
/* This covers Exabyte's max record size */
extern unsigned int	scsi_per_target_virtual = 256*1024;
#endif	/* MACH_KERNEL */


/*
 * Device-specific operations are switched off this table
 */

scsi_devsw_t	scsi_devsw[] = {

/* SCSI_DISK */		{ scdisk_name, SCSI_OPTIMIZE_NULL,
			  scdisk_open, scdisk_close,
			  scdisk_strategy, scdisk_start,
			  scdisk_get_status, scdisk_set_status },

/* SCSI_TAPE */		{ sctape_name, sctape_optimize,
			  sctape_open, sctape_close,
			  sctape_strategy, sctape_start,
			  sctape_get_status, sctape_set_status },

/* SCSI_PRINTER */	{ scprt_name, scprt_optimize,
			  SCSI_OPEN_NULL, SCSI_CLOSE_NULL,
			  SCSI_STRATEGY_NULL, SCSI_START_NULL,
			  SCSI_GET_STATUS_NULL, SCSI_SET_STATUS_NULL },

/* SCSI_CPU */		{ sccpu_name, SCSI_OPTIMIZE_NULL,
			  SCSI_OPEN_NULL, SCSI_CLOSE_NULL,
			  sccpu_strategy, sccpu_start,},

/* SCSI_WORM */		{ scworm_name, SCSI_OPTIMIZE_NULL,
			  scdisk_open, scdisk_close,
			  scdisk_strategy, scdisk_start,
			  scdisk_get_status, scdisk_set_status },

/* SCSI_CDROM */	{ sccdrom_name, SCSI_OPTIMIZE_NULL,
			  scdisk_open, scdisk_close,
			  scdisk_strategy, scdisk_start,
			  scdisk_get_status, scdisk_set_status },

/* scsi2 */
#if	NSCSI2 > 0
/* SCSI_SCANNER */	{ scscn_name, SCSI_OPTIMIZE_NULL, /*XXX*/ },

/* SCSI_MEMORY */	{ scmem_name, SCSI_OPTIMIZE_NULL,
			  scdisk_open, scdisk_close,
			  scdisk_strategy, scdisk_start,
			  scdisk_get_status, scdisk_set_status },

/* SCSI_J_BOX */	{ scjb_name, SCSI_OPTIMIZE_NULL, /*XXX*/ },

/* SCSI_COMM */		{ sccomm_name, SCSI_OPTIMIZE_NULL, /*XXX*/ },
#endif
			{ 0 }
};

/*
 * Allocation routines for state structures
 */
scsi_softc_t *
scsi_master_alloc(
	unsigned	unit,
	char		*hw)
{
	scsi_softc_t	*sc;

	if (unit < NSCSI) {
		sc = &scsi_softc_data[unit];
		scsi_softc[unit] = sc;
		sc->masterno = unit;
		sc->hw_state = hw;
		return sc;
	}
	return 0;
}

target_info_t *
scsi_slave_alloc(
	unsigned	unit,
	unsigned	slave,
	char		*hw)
{
	target_info_t	*tgt;

	tgt = &scsi_target_data[(unit<<3) + slave];
	tgt->hw_state = hw;
	tgt->dev_ops = 0;	/* later */
	tgt->target_id = slave;
	tgt->lun = 0;
	tgt->masterno = unit;
	tgt->block_size = 1;	/* default */
	tgt->flags = TGT_ALIVE;
	tgt->sync_period = 0;
	tgt->sync_offset = 0;
	simple_lock_init(&tgt->target_lock, ETAP_IO_TARGET);

	scsi_softc[unit]->target[slave] = tgt;
	return tgt;
}

/*
 * Slave routine:
 *	See if the slave description (controller, unit, ..)
 *	matches one of the slaves found during probe
 *
 * Implementation:
 *	Send out an INQUIRY command to see what sort of device
 *	the slave is.
 * Notes:
 *	At this time the driver is fully functional and works
 *	off interrupts.
 * TODO:
 *	The SCSI2 spec says what exactly must happen: see F.2.3
 */
/*ARGSUSED*/
int
scsi_slave(
	struct bus_device	*ui,
	caddr_t			reg)
{
	scsi_softc_t		*sc = scsi_softc[(unsigned char)ui->ctlr];
	target_info_t		*tgt = sc->target[(unsigned char)ui->slave];
	scsi2_inquiry_data_t	*inq;
	int			scsi_std;
	int			ptype;
	spl_t			s;

	if (!tgt || !(tgt->flags & TGT_ALIVE))
		return 0;

	/* Might have scanned already */
	if (tgt->dev_ops)
		goto out;

#if	0 && NSCSI2 > 0
	This is what should happen:
	- for all LUNs 
		INQUIRY
		scsi_verify_state (see)
		scsi_initialize (see)
#endif	/* SCSI2 */

	tgt->unit_no = ui->slave;	/* incorrect, but needed early */

	s = splhigh();	/* we need interrupts */
	spllo();

	if (BGET(scsi_no_synchronous_xfer,(unsigned char)sc->masterno,tgt->target_id))
		tgt->flags |= TGT_DID_SYNCH;

	/*
	 * Ok, it is time to see what type of device this is,
	 * send an INQUIRY cmd and wait till done.
	 * Possibly do the synch negotiation here.
	 */
	scsi_inquiry(tgt, SCSI_INQ_STD_DATA);

#if	NSCSIINFO > 0
	bcopy((char *) tgt->cmd_ptr, (char *) &tgt->target_inquiry,
	      sizeof(scsi2_inquiry_data_t));
#endif /* NSCSIINFO > 0 */

	inq = (scsi2_inquiry_data_t*)tgt->cmd_ptr;
	ptype = inq->periph_type;

	switch (ptype) {
	case SCSI_DISK :
	case SCSI_TAPE :
	case SCSI_PRINTER :
	case SCSI_CPU :
	case SCSI_WORM :
	case SCSI_CDROM :
/*	case SCSI_PREPRESS1 : reserved, really
	case SCSI_PREPRESS2 :	*/
#if	NSCSI2 > 0
	case SCSI_SCANNER :
	case SCSI_MEMORY :
	case SCSI_J_BOX :
	case SCSI_COMM :
#endif	/* NSCSI2 > 0 */
		tgt->dev_ops = &scsi_devsw[ptype];
		break;
	default:
		printf("scsi%d: %s %d (x%x). ", ui->ctlr,
		       "Unsupported device type at SCSI id", ui->slave,
			inq->periph_type);
		scsi_print_inquiry((scsi2_inquiry_data_t*)inq,
			SCSI_INQ_STD_DATA, 0);
		tgt->flags = 0;
		splx(s);
		return 0;
	}

	if (inq->rmb)
		tgt->flags |= TGT_REMOVABLE_MEDIA;

	if (ptype != SCSI_DISK)
		tgt->flags &= ~TGT_CHAINED_IO_SUPPORT;
	/*
	 * Tell the user we know this target, then see if we
	 * can be a bit smart about it.
	 */
	scsi_print_inquiry((scsi2_inquiry_data_t*)inq,
		SCSI_INQ_STD_DATA, tgt->tgt_name);
	if (scsi_debug)
		scsi_print_inquiry((scsi2_inquiry_data_t*)inq,
			SCSI_INQ_STD_DATA, 0);

	/*
	 * The above says if it currently behaves as a scsi2,
	 * however scsi1 might just be the default setting.
	 * The spec say that even if in scsi1 mode the target
	 * should answer to the full scsi2 inquiry spec.
	 */
#if nosey
	scsi_std = (inq->ansi == 2 || inq->response_fmt == 2) ? 2 : 1;
	if (scsi_std == 2) {
		unsigned char	supp_pages[256], i;
		scsi2_impl_opdef_page_t	*impl;

		scsi_inquiry(tgt, SCSI_INQ_SUPP_PAGES);
		impl = (scsi2_impl_opdef_page_t	*)inq;
		npages = impl->page_len - 2;
		bcopy(impl->supp_opdef, supp_pages, npages);

		for (i = 0; i < npages; i++) {
			scsi_inquiry(tgt, supp_pages[i]);
			scsi_print_inquiry(inq, supp_pages[i], 0);
		}
	}

	if (scsi_std == 2) {
		scsi2_impl_opdef_page_t	*impl;
		int i;

		scsi_inquiry(tgt, SCSI_INQ_IMPL_OPDEF);
		impl = (scsi2_impl_opdef_page_t	*)inq;
		for (i = 0; i < impl->page_len - 2; i++)
			if (impl->supp_opdef[i] == SCSI2_OPDEF) {
				scsi_change_definition(tgt, SCSI2_OPDEF);
				/* if success .. */
					tgt->flags |= TGT_SCSI_2_MODE;
				break;
			}
	}
#endif	/* nosey */

	splx(s);
out:
	return (strcmp(ui->name, (*tgt->dev_ops->driver_name)(TRUE)) == 0);
}

void
scsi_verify_state(
	target_info_t *tgt)
{
	int ret, i;
	for(i=0;i<3;i++) {
		int retry = 10;
		while ((ret = scsi_test_unit_ready(tgt, 0))
		       == SCSI_RET_RETRY &&
		       retry--)
		    if (ret == SCSI_RET_SUCCESS)
			return;
	}
}

void
scsi_initialize(
	target_info_t *tgt)
{
	int i = 0;
	while (scsi_start_unit(tgt, SCSI_CMD_SS_START, 0) == SCSI_RET_RETRY) {
		delay(1000000);
		if (++i == 30)
		    break;
	}
	scsi_verify_state(tgt);
}

/*
 * Attach routine:
 *	Fill in all the relevant per-slave data and make
 *	the slave operational.
 *
 * Implementation:
 *	Get target's status, start the unit and then
 *	switch off to device-specific functions to gather
 *	as much info as possible about the slave.
 */
void
scsi_attach(
	register struct bus_device *ui)
{
	scsi_softc_t		*sc = scsi_softc[ui->mi->unit];
	target_info_t		*tgt = sc->target[(unsigned char)ui->slave];
	int			i;
	spl_t			s;
	int			tmpunit;
	dev_ops_t		scsi_dev_ops;
	int			unit_started = 0;
	int			ret;
	char			devname[16];
	extern int		disk_indirect_count;
	extern int		cdrom_indirect_count;

#if defined(__digital_os__)
	printf(" (%s) [%s] ", tgt->tgt_name, (*tgt->dev_ops->driver_name)(FALSE));
#else
	printf(" (%s %s) ", (*tgt->dev_ops->driver_name)(FALSE),tgt->tgt_name);
#endif

	/*
	 * Initialize optional SCSI parameters - XXX statically!
	 */
	scsi_might_disconnect[ui->mi->unit] = 0xff;	/* yes */
	scsi_should_disconnect[ui->mi->unit] = 0x00;	/* no */
	/* scsi_initiator_id[ui->mi->unit] = 7; */		/* seven */

	if (tgt->flags & TGT_US) {
		printf(" [this cpu]");
		return;
	}

	s = splhigh();
	spllo();

	/* sense return from inquiry */
	scsi_request_sense(tgt, 0, 0);

#if	PARAGON860
	if (getbootint("BOOT_SEND_SCSI_START_UNIT", 1))
#endif	/* PARAGON860 */

	for (i = 0; i < 5; i++) {

		if (tgt->dev_ops != &scsi_devsw[SCSI_CPU]
                                        && !unit_started) {
                        /*
                         * Once start unit is successful, don't issue
                         * the command again, otherwise may always
                         * see device busy when next command is issued.
                         */
                        ret = scsi_start_unit(tgt, SCSI_CMD_SS_START, 0);
                        if (ret == SCSI_RET_NEED_SENSE) {
                                scsi_request_sense(tgt, 0, 0);
                                ret = SCSI_RET_RETRY;
                        }
                } else
                        ret = SCSI_RET_SUCCESS;

                if (ret == SCSI_RET_SUCCESS) {
                        unit_started = 1;
                        ret = scsi_test_unit_ready(tgt, 0);
                }

                if (ret == SCSI_RET_SUCCESS)
                        break;

                if (ret == SCSI_RET_DEVICE_DOWN) {
                        i = 5;
                        break;
                }

                if (ret == SCSI_RET_NEED_SENSE) {
                        scsi_request_sense(tgt, 0, 0);
                }
		delay(1000000);
        }

	if(i == 5 && ret != SCSI_RET_SUCCESS)
		printf(" not yet online ");


	if (tgt->dev_ops->optimize != SCSI_OPTIMIZE_NULL)
		(*tgt->dev_ops->optimize)(tgt);

	tgt->flags |= TGT_FULLY_PROBED;

#if defined(__digital_os__)
#define DISK_NAME "rz"
#else
#if SCSI_SEQUENTIAL_DISKS
#define DISK_NAME "sdisk"
#define	SCSI_SEQUENTIAL_DISK_NAME	"sd"
#define TAPE_NAME "stape"
#define SCSI_SEQUENTIAL_TAPE_NAME	"st"
#else
#define DISK_NAME "sd"
#define TAPE_NAME "st"
#endif
#endif

	if (tgt->dev_ops == &scsi_devsw[SCSI_DISK]) {
#if SCSI_SEQUENTIAL_DISKS
		sprintf(devname, "%s%d", SCSI_SEQUENTIAL_DISK_NAME, scsi_sequential_disk_indirect_count);
		scsi_sequential_disk_indirect_count++;
		if (dev_name_lookup(DISK_NAME, &scsi_dev_ops, &tmpunit)) {
			dev_set_indirection(devname,
					    scsi_dev_ops,
					    ui->unit * scsi_dev_ops->d_subdev);
		}
#endif	/* SCSI_SEQUENTIAL_DISKS */

		/* 'disk' aliases are sequential disk names
		 * for all disk devices (incl. ide)
		 */
		strcpy(devname, "disk");
		itoa(disk_indirect_count, devname + strlen(devname));
		disk_indirect_count++;
		if (dev_name_lookup(DISK_NAME, &scsi_dev_ops, &tmpunit)) {
			dev_set_indirection(devname,
					    scsi_dev_ops,
					    ui->unit * scsi_dev_ops->d_subdev);
		}

	} else if (tgt->dev_ops == &scsi_devsw[SCSI_CDROM]) {
		strcpy(devname, "cdrom");
		itoa(cdrom_indirect_count, devname + strlen(devname));
		
		if (dev_name_lookup("cd_rom", &scsi_dev_ops, &tmpunit)) {
			dev_set_indirection(devname,
					    scsi_dev_ops,
					    ui->unit * scsi_dev_ops->d_subdev);
		}

		cdrom_indirect_count++;
	}
#if SCSI_SEQUENTIAL_DISKS
	else if (tgt->dev_ops == &scsi_devsw[SCSI_TAPE]) {
		sprintf(devname, "%s%d", SCSI_SEQUENTIAL_TAPE_NAME,
			scsi_sequential_tape_indirect_count);
		scsi_sequential_tape_indirect_count++;
		if (dev_name_lookup(TAPE_NAME, &scsi_dev_ops, &tmpunit)) {
			dev_set_indirection(devname,
					    scsi_dev_ops,
					    ui->unit * scsi_dev_ops->d_subdev);
		}
	}
#endif	/* SCSI_SEQUENTIAL_DISKS */
		
    no_indirection:

	splx(s);

	return;
}

/*
 * Probe routine:
 *	See if a device answers.  Used AFTER autoconf.
 *
 * Implementation:
 *	First ask the HBA to see if anyone is there at all, then
 *	call the scsi_slave and scsi_attach routines with a fake ui.
 */
boolean_t
scsi_probe(
	scsi_softc_t		*sc,
	target_info_t		**tgt_ptr,
	int			target_id,
	int			lun,
	io_req_t		ior)
{
	struct bus_device	ui;
	target_info_t		*tgt;

	if (!sc->probe || target_id > 7 || target_id == sc->initiator_id)
		return FALSE;	/* sanity */

	if (sc->target[target_id] == 0)
		scsi_slave_alloc( sc->masterno, target_id, sc->hw_state);
	tgt = sc->target[target_id];
	tgt->lun = lun;
	tgt->flags = 0;/* we donno yet */
	tgt->dev_ops = 0;

#if	NORMA_SCSI
	/*
	 * XXX This is a hack to specify the scsi bus for norma
	 */
	if (sc->initiator_id != 7) {
		extern scsi_softc_t *norma_scsi_softc;

		if (norma_scsi_softc == (scsi_softc_t *)0)
		    norma_scsi_softc = sc;
	}
#endif	/*NORMA_SCSI*/
	/* mildly enquire */
	if (!(sc->probe)(tgt, ior))
		goto fail;

	/* There is something there, see what it is */
	bzero((char *)&ui, sizeof(ui));
	ui.ctlr = sc->masterno;
	ui.unit =
	ui.slave = target_id;
	ui.name = "";

	/* this fails on the name for sure */
	(void) scsi_slave( &ui, 0 /* brrrr */);
	if ((tgt->flags & TGT_ALIVE) == 0)
		goto fail;

	{
		struct bus_ctlr	mi;

		mi.unit = sc->masterno;
		ui.mi = &mi;
		printf(" %s%d at slave %d(%d) ",
		       (*tgt->dev_ops->driver_name)(TRUE),
		       sc->masterno*MAX_SCSI_TARGETS+target_id,
		       target_id,lun);
		scsi_attach(&ui);
	}

	*tgt_ptr = tgt;
	return TRUE;
fail:
	tgt->flags = 0;
	return FALSE;
}


/*
 * Watchdog routine:
 *	Issue a SCSI bus reset if a target holds up the
 *	bus for too long.
 *
 * Implementation:
 *	Each HBA that wants to use this should have a
 *	watchdog_t structure at the head of its hardware
 *	descriptor.  This variable is set by this periodic
 *	routine and reset on bus activity. If it is not reset on
 *	time (say some ten seconds or so) we reset the
 *	SCSI bus.
 * NOTE:
 *	An HBA must be ready to accept bus reset interrupts
 *	properly in order to use this.
 */
void
scsi_watchdog(
	struct watchdog_t	*hw)
{
	spl_t		s = splbio();

	switch (hw->watchdog_state) {
	case SCSI_WD_EXPIRED:

		/* double check first */
		if (hw->nactive == 0) {
			hw->watchdog_state = SCSI_WD_INACTIVE;
			break;
		}
		if (scsi_debug)
			printf("SCSI Watchdog expired\n");
		hw->watchdog_state = SCSI_WD_INACTIVE;
		(*hw->reset)(hw);
		break;

	case SCSI_WD_ACTIVE:

		hw->watchdog_state = SCSI_WD_EXPIRED;
		break;

	case SCSI_WD_INACTIVE:

		break;
	}

	/* do this here, fends against powered down devices */
	if (scsi_watchdog_period != 0)
	    timeout((timeout_fcn_t)scsi_watchdog, hw, scsi_watchdog_period * hz);

	splx(s);
}


/*
 * BusReset Notification:
 *	Called when the HBA sees a BusReset interrupt
 *
 * Implementation:
 *	Go through the list of targets, redo the synch
 *	negotiation, and restart whatever operation was
 *	in progress for that target.
 */
void
scsi_bus_was_reset(
	scsi_softc_t	*sc)
{
	register target_info_t	*tgt;
	int			i;
	/*
	 * Redo the synch negotiation
	 */
	for (i = 0; i < 8; i++) {
		io_req_t	ior;
		spl_t		s;

		if (i == sc->initiator_id)
			continue;
		tgt = sc->target[i];
		if (!tgt || !(tgt->flags & TGT_ALIVE) ||
		    (tgt->flags & TGT_US))
			continue;

		tgt->flags &= ~(TGT_DID_SYNCH|TGT_DISCONNECTED);
#if 0
		/* the standard does *not* imply this gets reset too */
		tgt->sync_period = 0;
		tgt->sync_offset = 0;
#endif

		/*
		 * retry the synch negotiation
		 */
		ior = tgt->ior;
		tgt->ior = 0;
		printf(".. tgt %d ", tgt->target_id);
		if (BGET(scsi_no_synchronous_xfer,(unsigned char)sc->masterno,tgt->target_id))
			tgt->flags |= TGT_DID_SYNCH;
		else {
			s = splhigh();
			spllo();
			scsi_test_unit_ready(tgt, 0);
			splx(s);
		}
		tgt->ior = ior;
	}

	/*
	 * Notify each target of the accident
	 */
	for (i = 0; i < 8; i++) {
		if (i == sc->initiator_id)
			continue;
		tgt = sc->target[i];
		if (!tgt || !(tgt->flags & TGT_ALIVE))
			continue;
		tgt->done = SCSI_RET_ABORTED|SCSI_RET_RETRY;
		if (tgt->ior)
			(*tgt->dev_ops->restart)( tgt, TRUE);
	}

	printf("%s", " reset complete\n");
}

#endif	/* NSCSI > 0 */
