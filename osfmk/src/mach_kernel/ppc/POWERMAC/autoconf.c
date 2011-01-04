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
 * Log from ppc directory follows
 * Log: autoconf.c
 * Revision 1.1.9.3  1996/08/20  16:16:31  stephen
 * 	Comment out "configuring bus device:" printf
 * 	[1996/08/20  15:57:56  stephen]
 *
 * Revision 1.1.9.2  1996/07/16  10:46:47  stephen
 * 	Tidy up of constants
 * 	Clean up interrupt enabling
 * 	Change to initialize vm mappings for IO region
 * 	before calling probes
 * 	[1996/07/16  10:42:05  stephen]
 * 
 * Revision 1.1.9.1  1996/06/07  18:34:54  stephen
 * 	removed amic_init() call
 * 	[1996/06/07  18:34:39  stephen]
 * 
 * Revision 1.1.7.3  1996/05/03  17:26:20  stephen
 * 	Added APPLE_FREE_COPYRIGHT
 * 	[1996/05/03  17:20:43  stephen]
 * 
 * Revision 1.1.7.2  1996/04/29  09:05:04  stephen
 * 	Added second SCSI bus devices, renamed ethernet device code
 * 	[1996/04/29  08:58:52  stephen]
 * 
 * Revision 1.1.7.1  1996/04/11  09:07:27  emcmanus
 * 	Copied from mainline.ppc.
 * 	[1996/04/10  17:02:50  emcmanus]
 * 
 * Revision 1.1.5.5  1996/02/08  17:38:03  stephen
 * 	Added mouse init
 * 	[1996/02/08  17:18:25  stephen]
 * 
 * Revision 1.1.5.4  1996/01/22  14:57:40  stephen
 * 	Added hacked SCU ethernet driver - TODO - replace with clean code
 * 	[1996/01/22  14:38:37  stephen]
 * 
 * Revision 1.1.5.3  1996/01/12  16:15:14  stephen
 * 	Added cuda, adb, video console
 * 	[1996/01/12  14:33:51  stephen]
 * 
 * Revision 1.1.5.2  1995/12/19  10:19:44  stephen
 * 	Started on real autoconfigure code
 * 	[1995/12/19  10:15:01  stephen]
 * 
 * Revision 1.1.5.1  1995/11/23  17:41:03  stephen
 * 	first powerpc checkin to mainline.ppc
 * 	[1995/11/23  16:56:18  stephen]
 * 
 * Revision 1.1.3.2  1995/11/16  21:56:06  stephen
 * 	started autoconfiguring console
 * 	[1995/11/16  21:37:54  stephen]
 * 
 * Revision 1.1.3.1  1995/10/10  15:08:46  stephen
 * 	return from apple
 * 	[1995/10/10  14:35:16  stephen]
 * 
 * 	powerpc merge
 * 
 * Revision 1.1.1.2  95/09/05  17:57:36  stephen
 * 	Created to initialise interrupt control registers
 * 
 * EndLog_PPC
 */

#include <debug.h>
#include <cpus.h>
#include <chips/busses.h>	/* meb 11/2/95 */
#include <mach/ppc/thread_status.h>
#include <ppc/misc_protos.h>
#include <ppc/spl.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/proc_reg.h>
#include <ppc/pmap.h>

/* initialization typecasts */
#define	SPL_TTY		(caddr_t)SPLTTY
#define SPL_BIO		(caddr_t)SPLBIO

/* scsi 53C94 driver init */
#include <asc.h>
#if NASC > 0
extern struct  bus_driver  asc_driver;
int asc_intr();

#include <scsi/scsi.h>
#endif /* NASC > 0 */

#include <mesh.h>
#if NMESH > 0
extern struct bus_driver mesh_driver;
int mesh_intr();
#endif

#include <scc.h>
#if NSCC > 0
void	scc_intr(struct ppc_saved_state *);
extern 	struct	bus_driver scc_driver;
#endif /* NSCC > 0 */

#include <cuda.h>
#if NCUDA > 0
void	cudaintr();
extern struct bus_driver cuda_driver;
#endif

#include <pmu.h>
#if NPMU > 0
extern struct bus_driver pmu_driver;
#endif

#include <adb.h>
#if NADB > 0
extern struct bus_driver adb_driver;
#endif

#include <nvram.h>
#if NNVRAM > 0
extern struct bus_driver nvram_driver;
#endif

#include <pram.h>
#if NPRAM > 0
extern struct bus_driver pram_driver;
#endif

#include <vc.h>
#if NVC > 0
extern struct bus_driver vc_driver;
#endif

#include <mouse.h>
#if NMOUSE > 0
extern struct bus_driver mouse_driver; 
#endif

#include <fd.h>
#if NFD > 0
extern struct bus_driver fddriver; 
#endif

#include <lan.h>
#if NLAN > 0
#include <ppc/POWERMAC/if_mace_entries.h>
extern	struct	bus_driver	mace_driver;
#endif /* NLAN */

#include <tulip.h>
#if NTULIP > 0
#include <ppc/POWERMAC/if_tulip_entries.h>
extern struct bus_driver 	tulip_driver;
#endif

#include <bmac.h>
#if NBMAC > 0
#include <ppc/POWERMAC/if_bmac_entries.h>
extern struct bus_driver	bmac_driver;
#endif

#include <wd.h>
#if NWD > 0
#include <ppc/POWERMAC/wd_entries.h>
extern struct bus_driver wd_driver;
extern struct bus_driver wcd_driver;
#endif

#include <awacs.h>
#if NAWACS > 0
extern struct bus_driver awacs_driver;
#endif
struct	bus_ctlr	bus_master_init[] = {

/* driver       name   unit 	intr		address	    len  phys_address
 *   adaptor	alive	flags	spl		interrupt
 */

/* NOTE from lion@apple.com
 *
 * Try to keep the ordering of the SCSI busses from
 * internal to external for proper ordering of device names
 */
#if NMESH > 0
{&mesh_driver,	"mesh",	0,	(intr_t)mesh_intr,
	 (caddr_t)PCI_MESH_BASE_PHYS,	0, (caddr_t)PCI_MESH_BASE_PHYS,
	'?',	0,	0,	0, },
#endif
#if NASC > 1
{&asc_driver,	"asc",	0,	(intr_t)asc_intr,
	 (caddr_t)PDM_ASC2_BASE_PHYS,	0, (caddr_t)PDM_ASC2_BASE_PHYS,
	'?',	0,	0,	(caddr_t)PDM_SCSI_DMA_CTRL_BASE_PHYS, },
#endif /* NASC > 1*/
#if NASC > 0
{&asc_driver,	"asc",	1,	(intr_t)asc_intr,
	 (caddr_t)PDM_ASC_BASE_PHYS,	1, (caddr_t)PDM_ASC_BASE_PHYS,
	'?',	0,	0,	(caddr_t)PDM_SCSI_DMA_CTRL_BASE_PHYS, },
#endif /* NASC > 0 */
#if NWD > 0 
{&wd_driver, "ide", 0, (intr_t)NULL, (caddr_t)0, 0x1000, (caddr_t)0,
   '?', 0, 0, (caddr_t)SPLBIO, 0},
#if NWD > 1
{&wd_driver, "ide", 1, (intr_t)NULL, (caddr_t)1, 0x1000, (caddr_t)0,
   '?',  0, 0, (caddr_t)SPLBIO, 0},
#endif // NWD > 1
{&wcd_driver, "atapicd", 0, (intr_t)NULL, (caddr_t)1, 0x1000, (caddr_t)0,
   '?',  0, 0, (caddr_t)SPLBIO, 0},
#endif // NWD > 0
  0
};

struct	bus_device	bus_device_init[] = {

/* driver      name  unit intr      address    am   phys_address 
    adaptor alive ctlr  slave  flags *mi *next  sysdep sysdep */

#if NSCC > 0
{ &scc_driver,	"scc",	0,	(intr_t)scc_intr,
	  (caddr_t) 0,	0, (caddr_t)0,
	  '?',     0,    -1,   -1,    0,   0,  0, 
	  (caddr_t)0, 0 },
#endif /* NSCC > 0 */

#if NCUDA > 0
{ &cuda_driver, "cuda", 0,	(intr_t) cudaintr,
	(caddr_t) PDM_CUDA_BASE_PHYS,	0,	(caddr_t) PDM_CUDA_BASE_PHYS,
	'?',	0,	-1,	-1,	0,	0,	0, SPL_TTY, 16 },
#endif

#if NPMU > 0
{ &pmu_driver, "pmu", 0, (intr_t) NULL,
    (caddr_t) NULL, 0, (caddr_t) NULL,
    '?', 0, -1, -1, 0, 0, 0, SPL_TTY, 16},
#endif

#if NADB > 0
{ &adb_driver, "adb", 0,	(intr_t) NULL,
	(caddr_t) NULL, 0,	(caddr_t) NULL,
	'?',	0,	-1, 	-1,	0,	0,	0, SPL_TTY, 0},
#endif

#if NNVRAM > 0
{ &nvram_driver, "nvram", 0,	(intr_t) NULL,
	(caddr_t) NULL, 0,	(caddr_t) NULL,
	'?',	0,	-1, 	-1,	0,	0,	0, SPL_TTY, 0},
#endif

#if NPRAM > 0
{ &pram_driver, "pram", 0,	(intr_t) NULL,
	(caddr_t) NULL, 0,	(caddr_t) NULL,
	'?',	0,	-1, 	-1,	0,	0,	0, SPL_TTY, 0},
#endif

#if NVC > 0
/* Power Mac Video Console */
{&vc_driver,	"vc",	0,	(intr_t) NULL,
	(caddr_t) -1,	0,	(caddr_t) -1,
	'?',	0,	-1,	-1, 	0,	0, 0, SPL_TTY,	0},
#endif
#if NMOUSE > 0
/* ADB Mouse */
{&mouse_driver,	"mouse",	0,	(intr_t) NULL,
	(caddr_t) -1,	0,	(caddr_t) -1,
	'?',	0,	-1,	-1,	0,	0, 0, SPL_TTY, 0},
#endif

#if NFD > 0

{&fddriver,	"fd",	0,	(intr_t) NULL,
	(caddr_t) PCI_FLOPPY_BASE_PHYS,	0,  (caddr_t) PCI_FLOPPY_BASE_PHYS,
	'?',	0,	-1,	-1,	0,	0, 0, SPL_TTY, 0},
#endif

#if NAWACS > 0
/* Russ's  AWACS Driver */
{ &awacs_driver, "awacs", 0, (intr_t) NULL,
  (caddr_t) PCI_AUDIO_BASE_PHYS, 0, (caddr_t) PCI_AUDIO_BASE_PHYS,
    '?',    0,   -1,   -1,  0,  0,  0, (caddr_t) 0, 0 },  
#endif

/* scsi device init */
#if NASC > 0
{ &asc_driver, "rz", 0, NO_INTR, 0x0, 0, 0, '?', 0, 0, 0, 0, },
{ &asc_driver, "rz", 1, NO_INTR, 0x0, 0, 0, '?', 0, 0, 1, 0, },
{ &asc_driver, "rz", 2, NO_INTR, 0x0, 0, 0, '?', 0, 0, 2, 0, },
{ &asc_driver, "rz", 3, NO_INTR, 0x0, 0, 0, '?', 0, 0, 3, 0, },
{ &asc_driver, "rz", 4, NO_INTR, 0x0, 0, 0, '?', 0, 0, 4, 0, },
{ &asc_driver, "rz", 5, NO_INTR, 0x0, 0, 0, '?', 0, 0, 5, 0, },
{ &asc_driver, "rz", 6, NO_INTR, 0x0, 0, 0, '?', 0, 0, 6, 0, },
{ &asc_driver, "rz", 7, NO_INTR, 0x0, 0, 0, '?', 0, 0, 7, 0, },

{ &asc_driver, "tz", 0, NO_INTR, 0x0, 0, 0, '?', 0, 0, 0, 0, },
{ &asc_driver, "tz", 1, NO_INTR, 0x0, 0, 0, '?', 0, 0, 1, 0, },
{ &asc_driver, "tz", 2, NO_INTR, 0x0, 0, 0, '?', 0, 0, 2, 0, },
{ &asc_driver, "tz", 3, NO_INTR, 0x0, 0, 0, '?', 0, 0, 3, 0, },
{ &asc_driver, "tz", 4, NO_INTR, 0x0, 0, 0, '?', 0, 0, 4, 0, },
{ &asc_driver, "tz", 5, NO_INTR, 0x0, 0, 0, '?', 0, 0, 5, 0, },
{ &asc_driver, "tz", 6, NO_INTR, 0x0, 0, 0, '?', 0, 0, 6, 0, },
{ &asc_driver, "tz", 7, NO_INTR, 0x0, 0, 0, '?', 0, 0, 7, 0, },

#if NASC > 1
{ &asc_driver, "rz", 8, NO_INTR, 0x0, 0, 0, '?', 0, 1, 0, 0, },
{ &asc_driver, "rz", 9, NO_INTR, 0x0, 0, 0, '?', 0, 1, 1, 0, },
{ &asc_driver, "rz", 10, NO_INTR, 0x0, 0, 0, '?', 0, 1, 2, 0, },
{ &asc_driver, "rz", 11, NO_INTR, 0x0, 0, 0, '?', 0, 1, 3, 0, },
{ &asc_driver, "rz", 12, NO_INTR, 0x0, 0, 0, '?', 0, 1, 4, 0, },
{ &asc_driver, "rz", 13, NO_INTR, 0x0, 0, 0, '?', 0, 1, 5, 0, },
{ &asc_driver, "rz", 14, NO_INTR, 0x0, 0, 0, '?', 0, 1, 6, 0, },
{ &asc_driver, "rz", 15, NO_INTR, 0x0, 0, 0, '?', 0, 1, 7, 0, },

{ &asc_driver, "tz", 8, NO_INTR, 0x0, 0, 0, '?', 0, 1, 0, 0, },
{ &asc_driver, "tz", 9, NO_INTR, 0x0, 0, 0, '?', 0, 1, 1, 0, },
{ &asc_driver, "tz", 10, NO_INTR, 0x0, 0, 0, '?', 0, 1, 2, 0, },
{ &asc_driver, "tz", 11, NO_INTR, 0x0, 0, 0, '?', 0, 1, 3, 0, },
{ &asc_driver, "tz", 12, NO_INTR, 0x0, 0, 0, '?', 0, 1, 4, 0, },
{ &asc_driver, "tz", 13, NO_INTR, 0x0, 0, 0, '?', 0, 1, 5, 0, },
{ &asc_driver, "tz", 14, NO_INTR, 0x0, 0, 0, '?', 0, 1, 6, 0, },
{ &asc_driver, "tz", 15, NO_INTR, 0x0, 0, 0, '?', 0, 1, 7, 0, },
#endif 
#endif /* NASC > 0 */

#if  NMESH > 0
{ &mesh_driver, "rz", 0, NO_INTR, 0x0, 0, 0, '?', 0,  0, 0, 0, },
{ &mesh_driver, "rz", 1, NO_INTR, 0x0, 0, 0, '?', 0,  0, 1, 0, },
{ &mesh_driver, "rz", 2, NO_INTR, 0x0, 0, 0, '?', 0, 0, 2, 0, },
{ &mesh_driver, "rz", 3, NO_INTR, 0x0, 0, 0, '?', 0, 0, 3, 0, },
{ &mesh_driver, "rz", 4, NO_INTR, 0x0, 0, 0, '?', 0, 0, 4, 0, },
{ &mesh_driver, "rz", 5, NO_INTR, 0x0, 0, 0, '?', 0, 0, 5, 0, },
{ &mesh_driver, "rz", 6, NO_INTR, 0x0, 0, 0, '?', 0, 0, 6, 0, },
{ &mesh_driver, "rz", 7, NO_INTR, 0x0, 0, 0, '?', 0, 0, 7, 0, },

{ &mesh_driver, "tz", 0, NO_INTR, 0x0, 0, 0, '?', 0,  0, 0, 0, },
{ &mesh_driver, "tz", 1, NO_INTR, 0x0, 0, 0, '?', 0,  0, 1, 0, },
{ &mesh_driver, "tz", 2, NO_INTR, 0x0, 0, 0, '?', 0, 0, 2, 0, },
{ &mesh_driver, "tz", 3, NO_INTR, 0x0, 0, 0, '?', 0, 0, 3, 0, },
{ &mesh_driver, "tz", 4, NO_INTR, 0x0, 0, 0, '?', 0, 0, 4, 0, },
{ &mesh_driver, "tz", 5, NO_INTR, 0x0, 0, 0, '?', 0, 0, 5, 0, },
{ &mesh_driver, "tz", 6, NO_INTR, 0x0, 0, 0, '?', 0, 0, 6, 0, },
{ &mesh_driver, "tz", 7, NO_INTR, 0x0, 0, 0, '?', 0, 0, 7, 0, },
#endif

#if NWD > 0
{ &wd_driver, "ide", 0, NO_INTR, 0x0, 0,0, '?', 0, 0, 0, 0, },
{ &wd_driver, "ide", 1, NO_INTR, 0x0, 0,0, '?', 0, 0, 1, 0, },
{ &wd_driver, "ide", 2, NO_INTR, 0x0, 0,0, '?', 0, 1, 0, 0, },
{ &wd_driver, "ide", 3, NO_INTR, 0x0, 0,0, '?', 0, 1, 1, 0, },
{ &wcd_driver, "wcd", 0, NO_INTR, 0x0, 0,0, '?', 0, 0, 0, 0, },
#endif

#if NLAN > 0
  {&mace_driver, "et", 0, (intr_t)0,
     (caddr_t)PDM_MACE_BASE_PHYS, 0,(caddr_t)PDM_MACE_BASE_PHYS,
     '?',    0,   -1,    -1,    0,   0,        0,
     (caddr_t)PDM_MACE_ENET_ADDR_PHYS, (long)PDM_MACE_DMA_CTRL_PHYS},
#endif /* NLAN > 0 */
#if NTULIP> 0
  {&tulip_driver, "tulip", 0, (intr_t)0,
     (caddr_t)0, 0,(caddr_t)0,
     '?',    0,   -1,    -1,    0,   0,        0,
     (caddr_t)0, (long)0},
#endif /* NTULIP > 0 */
#if NBMAC > 0
  {&bmac_driver, "el", 0, (intr_t)0,
     (caddr_t)0, 0,(caddr_t)0,
     '?',    0,   -1,    -1,    0,   0,        0,
     (caddr_t)0, (long)0},
#endif /* NBMAC > 0 */
  0
};

/*
 * probeio:
 *
 * Probe and subsequently attach devices on the motherboard
 * and on any busses out there.
 *
 */
void
probeio(void)
{
	register struct	bus_device	*device;
	register struct	bus_ctlr	*master;
	int				i = 0;
	extern void pci_configure(void);

#if DEBUG
	printf("PowerMac is %sI/O-coherent\n",
		powermac_info.io_coherent == FALSE ? "NOT ": "");
#endif

	pci_configure();

	for (master = bus_master_init; master->driver; master++) {
#if DEBUG
		printf("configuring bus master : %s\n",master->name);
#endif /* DEBUG */
		if (configure_bus_master(master->name, master->address,
				master->phys_address, i, "motherboard"))
			i++;
	}

	for (device = bus_device_init; device->driver; device++) {
		/* ignore what we (should) have found already */
		if (device->alive || device->ctlr > -1)
			continue;
#if DEBUG
		printf("configuring bus device : %s\n",device->name);
#endif /* DEBUG */
		if (configure_bus_device(device->name, device->address,
				device->phys_address, i, "motherboard"))
			i++;
		else
			device->adaptor = ' ';	/* skip it next time */
	}
#if DEBUG
	printf("probeio() done.\n");
#endif
}


void
autoconf(void)
{
	/* make IO area mappable from user applications */

	(void)pmap_add_physical_memory(powermac_info.io_base_phys,
				       (powermac_info.io_base_phys +
					powermac_info.io_size),
				       FALSE,
				       PTE_WIMG_IO);

	/* remap IO area into virtual space so that devices can
	 * set up their pointers to a virtual mapping
	 */

	powermac_info.io_base_virt = io_map(powermac_info.io_base_phys,
					    powermac_info.io_size);

	powermac_io_init(powermac_info.io_base_virt);

	/* Make sure that interrupts are correctly initialized before
	 * we run the generic code. setting remains at splhigh. We must
	 * do this AFTER the io mappings have been initialised since
	 * interrupts may use registers in the io space
	 */
	interrupt_init();

#if NCPUS >1
	/*
	 *		Probe the MP hardware now that we have some memory
	 */
	mp_probe_cpus();
#endif	/* NCPUS > 1 */


	/* probeio needs interrupts enabled
	 * interrupts get switched on for the first time here.
	 */
	spllo();

	probeio();

	/* We no longer need BATs to map I/O */
	if (PROCESSOR_VERSION != PROCESSOR_VERSION_601) {
		mtdbatu(3,BAT_INVALID); mtdbatl(3,BAT_INVALID);
		mtdbatu(2,BAT_INVALID); mtdbatl(2,BAT_INVALID);
	}
	
	return;
}

