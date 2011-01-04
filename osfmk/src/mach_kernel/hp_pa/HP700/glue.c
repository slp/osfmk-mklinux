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
 * Copyright (c) 1991, 1992, 1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: glue.c 1.30 95/04/26$
 */
/*
 * Just what the name says
 */
#include <kgdb.h>
#include <types.h>
#include <machine/asp.h>
#include <kern/spl.h>
#include <mach/machine/vm_types.h>

#include <hp_pa/machparam.h>
#include <hp_pa/iomod.h>
#include <hp_pa/iodc.h>
#include <hp_pa/pdc.h>
#include <hp_pa/HP700/device.h>

#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/trap.h>
#include <hp_pa/HP700/bus_protos.h>
#include <device/ds_routines.h>
#include <device/conf.h>

#include <busses/gsc/gsc.h>

#include "dca.h"
#if NDCA > 0
#include <hp_pa/HP700/dcareg.h>
#endif

#include "hil.h"
#if NHIL > 0
#include <hp_pa/HP700/hilreg.h>
#include <hp_pa/HP700/hilioctl.h>
#include <hp_pa/HP700/hilvar.h>
#endif

#include <eisaha.h>
#if NEISAHA > 0
#include <hp_pa/HP700/eisa_common.h>
#include <fddi.h>
#if NFDDI > 0
#include <hp_pa/HP700/sh4811/if_sh4811var.h>
#endif
#endif

#include "lan.h"
#include "ncr.h"
#include "myri.h"
#include "grf.h"
#include "gkd.h"

#if NGKD > 0
#include <hp_pa/HP700/gkdvar.h>
#endif

#if NLAN > 0
#include <hp_pa/HP700/if_i596var.h>
#endif

#if NHIL > 0
void nmihand(int);
#endif

#if (NNCR > 0)
#include <scsi/adapters/scsi_53C700.h>
#endif

#if NMYRI > 0
#include <chips/myricom/net_myricom_gsc.h>
#endif /* NMYRI > 0 */

u_long	scsi_clock_freq = 0;			/* XXX for scsi */
int	isgecko = 0;				/* XXX */

#if KGDB
#include <kgdb/kgdb_stub.h>
#endif /* KGDB */

extern int eisaix, waxix;

/*
 * Forward declaration
 */
void
gsc_setup(struct gsc_board *
    );

void
gsc_setup(
    struct gsc_board *gp)
{
    if (gp->gsc_flags & GSC_FLAGS_EIR_SHARED)
	procitab(gp->gsc_ipl, gsc_intr, gp->gsc_eir);
    else
	procitab(gp->gsc_ipl, gp->gsc_intr, gp->gsc_unit);
}

void
ioconf(void)
{
	struct hp_device hd;
	struct modtab *m;
	extern struct device_path boot_dp;
	extern struct iodc_data boot_iodc;

	if (isgecko) {
		lasiinit();
		if (waxix >= 0)
			waxinit();			
	}
	else
		aspinit();

#if NDCA > 0
	if (isgecko) {
		m = getmod(IODC_TP_FIO, SVMOD_FIO_GRS232, 0);
		if (m) {
			hd.hp_addr = (char *)m->m_hpa;
			hd.hp_unit = 0;
			hd.hp_flags = (1 << hd.hp_unit);
			dcaprobe(&hd);
			aspitab(INT_RS232_1, SPLTTY, dcaintr, 0);
			/* XXX assumes second port is on WAX */
			m = getmod(IODC_TP_FIO, SVMOD_FIO_GRS232, 1);
			if (m && m->m_hpa == (struct iomod*)0xf0202000) {
				hd.hp_addr = (char *)m->m_hpa;
				hd.hp_unit = 1;
				hd.hp_flags = (1 << hd.hp_unit);
				dcaprobe(&hd);
				waxitab(0x40, SPLTTY, dcaintr, 1);
			}
		}
	} else {
		m = getmod(IODC_TP_FIO, SVMOD_FIO_RS232, 0);
		if (m) {
			hd.hp_addr = (char *)m->m_hpa;
			hd.hp_unit = (m->m_fixed == 4) ? 0 : 1;
			hd.hp_flags = (1 << hd.hp_unit);
			dcaprobe(&hd);
			aspitab(hd.hp_unit ? INT_RS232_2 : INT_RS232_1,
				SPLTTY, dcaintr, hd.hp_unit);
			m = getmod(IODC_TP_FIO, SVMOD_FIO_RS232, 1);
			if (m) {
				hd.hp_addr = (char *)m->m_hpa;
				hd.hp_unit = (m->m_fixed == 4) ? 0 : 1;
				hd.hp_flags = (1 << hd.hp_unit);
				dcaprobe(&hd);
				aspitab(hd.hp_unit ? INT_RS232_2 : INT_RS232_1,
					SPLTTY, dcaintr, hd.hp_unit);
			}
		}
	}
#endif

#if NLAN > 0
	m = getmod(IODC_TP_FIO, isgecko ? SVMOD_FIO_GLAN : SVMOD_FIO_LAN, 0);
	if (m) {
		hd.hp_addr = (char *)m->m_hpa;
		hd.hp_unit = 0;
		if (pc586probe(&hd)) {
			dev_ops_t	dp;

			pc586attach(&hd);
			for (dp = dev_name_list;
			     dp < &dev_name_list[dev_name_count]; dp++)
				if (dp->d_open == pc586open)
					dev_set_indirection("lan", dp, 0);
		}
	}
#endif

#if NMYRI > 0
	m = getmod(IODC_TP_A_DMA, SVMOD_ADMA_MYRINET, 0);
	while (m) {
	    IOBUS_TYPE_T iotype = IOBUS_TYPE(IOBUS_TYPE_BUS_GSC, 0,
					     IOBUS_TYPE_FLAGS_NOTCOHERENT);
	    IOBUS_ADDR_T iobase = (IOBUS_ADDR_T)m->m_hpa;
	    gsc_attach_t attach = net_myricom_gsc_probe(iotype, iobase);

	    if (attach != (gsc_attach_t)0)
		gsc_attach_slot(iotype, iobase, GSC_BOARD_TYPE_A_DMA,
				ffset(~SPLADMA), SPLADMA, attach, gsc_setup);
	    else
		printf("ioconf: unsupported myri0 at core%d\n", m->m_fixed);

	    m = getmod(IODC_TP_A_DMA, SVMOD_ADMA_MYRINET, 1);
	}
#endif

#if KGDB
	kgdb_mux_init();
#endif

#if NGKD > 0
	m = getmod(IODC_TP_FIO, SVMOD_FIO_GPCIO, 0);
	if (m) {
		hd.hp_addr = (char *)m->m_hpa;
		hd.hp_unit = 0;
		if (gkdprobe(&hd))
			gkdattach(&hd);
	}
#endif /* NGKD > 0 */

#if (NNCR > 0)
	m = getmod(IODC_TP_FIO, isgecko ? SVMOD_FIO_GSCSI : SVMOD_FIO_SCSI, 0);
	if (m) {
		struct hp_ctlrinfo scsictlr;
		struct hp_businfo scsibus;
#if 0
		extern struct asp_ctrl *CORE_CTRL;
		extern int itick_per_tick, hz;
		u_long freq;

		/*
		 * Determine the SCSI clock frequency
		 * XXX we use a magic value out of the PDC now.
		 */
		freq = itick_per_tick * hz;
		if (CORE_CTRL->iostat.scsi_sel == 0)
			freq >>= 1;
		if (m->m_scsiclk && m->m_scsiclk != freq)
			printf("SCSI HW freq: %d, computed: %d, using HW\n",
			       m->m_scsiclk, freq);
#endif
		/*
		 * XXX avoid changing the SCSI driver for now.
		 * Set the global scsi_clock_freq to reflect the value
		 * for the current controller.
		 */
		scsi_clock_freq = m->m_scsiclk;
		if (scsi_clock_freq == 0)
			scsi_clock_freq = 50000000;

		scsictlr.ctlrnum = 0;
		scsictlr.parent_ctlrinfop = &scsictlr;
		scsictlr.ctlr_businfop = &scsibus;
		scsictlr.numcsrs = 1;
		scsictlr.vcsrs[0] = (vm_offset_t)m->m_hpa;
		scsibus.bustype = BUSTYPE_SYS;
		if (ncr53c700_probe(&scsictlr) == 0)
			panic("ioconf: internal SCSI not found");
	}
#endif

#if (NNCR > 1)
	m = getmod(IODC_TP_A_DMA, SVMOD_ADMA_FWSCSI, 0);
	if (m)
	{
		struct hp_ctlrinfo scsictlr;
		struct hp_businfo scsibus;

		scsictlr.ctlrnum = 1;
		scsictlr.parent_ctlrinfop = &scsictlr;
		scsictlr.ctlr_businfop = &scsibus;
		scsictlr.numcsrs = 1;
		scsictlr.vcsrs[0] = (vm_offset_t)m->m_hpa;
		scsibus.bustype = BUSTYPE_ZALON;
		if (ncr53c700_probe(&scsictlr) == 0)
			panic("ioconf: internal F/W SCSI not found");

		if (bcmp((const char *)&m->mt_type,
			 (const char *)&boot_iodc, sizeof (boot_iodc)) == 0) {
		    /*
		     * The boot device is on the FWSCSI controller
		     * Adjust the unit number because FWSCSI is controller 1
		     */
		    boot_dp.dp_layers[0] += 8;
		}
	}
#endif
#if NGRF > 0
	grfconfig();
#endif
#if NHIL > 0
        m = getmod(IODC_TP_FIO, SVMOD_FIO_HIL, 0);
	if (m) {
		/*
		 * Under the ASP we get an initial interrupt when we unmask
		 * the device for the first time.  Hence, we must have the
		 * appropriate software structures initialized before we do
		 * the aspitab call (which in turn must be done before
		 * calling hilinit).
		 */
		hilsoftinit(0, (struct hil_dev *)m->m_hpa);
		if (isgecko && (waxix != -1))
			waxitab(0x2, SPLHIL, hilintr, 0);
		else
			aspitab(INT_8042_GEN, SPLHIL, hilintr, 0);
		hilinit(0, (struct hil_dev *)m->m_hpa);
#ifdef PANICBUTTON
		/*
		 * XXX not really proper to set SPL above SPLASP for
		 * this, but it works ok in this instance.
		 */
		if (isgecko && (waxix != -1))
			waxitab(0x04, SPLPOWER, nmihand, 0);
		else
			aspitab(INT_8042_HI, SPLPOWER, nmihand, 0);
#endif
	}
#endif

#if NEISAHA > 0
	if (eisaix != -1)
        	eisa_init();
#if NFDDI > 0
        m = getmod(IODC_TP_EISA, INTERPHASE, 0);
        if (m)
        {
                int unit=0;


                hd.hp_addr = (char *)m->m_hpa;
                hd.hp_unit = unit++;
                if (sh4811probe(&hd))
                        sh4811attach(&hd);
                while (m = getmod(IODC_TP_EISA, INTERPHASE, 1)) {
                        hd.hp_addr = (char *)m->m_hpa;
                        hd.hp_unit = unit++;
                        if (sh4811probe(&hd))
                                sh4811attach(&hd);
                }
        }
#endif
	if (waxix != -1) {
        	eisa_enable_intr();
	}
	else if (eisaix != -1) {
		aspitab(INT_EISA, SPLEISA, (void (*)(int))eisaintr, 0) ;
        	eisa_enable_intr();
	}
#endif /* NEISAHA > 0 */
}


#ifdef PANICBUTTON
int panicbutton = PANICBUTTON;
#if NHIL > 0
int crashandburn = 0;
int candbdelay = 50;		/* 1/HZths--give em half a second */

candbtimer()
{
	crashandburn = 0;
}

#if NHIL > 0
void 
nmihand(int unit)
{
	if (kbdnmi(unit)) {
		printf("Got a keyboard NMI\n");
		if (panicbutton) {
			if (crashandburn) {
				extern char *panicstr;

				crashandburn = 0;
				panic(panicstr ?
				      "forced crash, nosync" : "forced crash");
			}
			crashandburn++;
			timeout(candbtimer, (caddr_t)0, candbdelay);
		}
		return;
	}
	/* panic?? */
	printf("unexpected NMI interrupt ignored\n");
}
#endif
#endif
#endif
