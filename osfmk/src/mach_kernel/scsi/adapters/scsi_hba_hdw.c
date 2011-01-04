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
 *	scsi_hba_hdw.c:
 *		Shim between OSF/1 SCSI upper layers and SVR4 HBA driver.
 *		It will simulate many PDI and SDI functions to achieve this.
 *	Created:
 *		Thu Jan 11 16:49:20 UTC 1996
 *	Copyrights:
 *		(c) 1996 CPLabs Ltd.
 *	Author:
 *		Peter S. Calvert
 *	Notes:
 *		(a) Use ts=4 in vi when editiing this file
 *		(b) Multiple SCSI buses per controller is not supported.
 *		(c) Multiple Lun's per SCSI target is not supported.
 *		(d) Configured to support only 8 devices per SCSI controller.
 *		(e) Adapters of the same type MUST have consecutive unit numbers
 *			This is due the the fact that all adapters for a particular
 *			HBA driver are located at the same time, not allowing
 *			a seperate probe call per controller.
 *		(f) Not handling SENSE request too well yet (see sdi_cmd_done()).
 *		(f) Watchdog's and Command timeouts have not bee addressed.
 *		(g) Multi-processor locking not impletemented.
 *		(h) Currenly does only very simple searching for SCSI targets
 *
 */

#include "hba.h"

#if NHBA > 0  &&  defined(AT386)

#include <cpus.h>
#include <platforms.h>
#include <mach/std_types.h>
#include <types.h>
#include <sys/syslog.h>
#include <kern/misc_protos.h>
#include <scsi/scsi_defs.h>
#include <scsi/scsi.h>
#include <i386/pio.h>		/* inlining of outb and inb */
#include <i386/AT386/mp/mp.h>
#include <himem.h>
#include <i386/AT386/himem.h>
#include <kern/assert.h>
#include <mach/i386/vm_types.h>
#include <vm/vm_kern.h>
#include <kern/spl.h>

#include <sys/sdi.h>
#include <device/hbaconf.h>

#if	MP_V1_1
#include <i386/AT386/mp/mp_v1_1.h>
#endif	/* MP_V1_1 */

#define HBA_DBG	0

#if HBA_DBG > 0
#define HBAMSG(level, msg)	if (level <= HBA_DBG) printf msg; else (void) 0
#else
#define HBAMSG(level, msg)	(void) 0
#endif

#define LOCAL	/* static */
#define EXPORT
#define IMPORT	extern


#define	min(a,b)	((a < b)?  a:  b)

/* if bus is supported, define this to !0 */
#define BUS_SUPPORT	0

/* if lun is supported, define this to !0 */
#define LUN_SUPPORT	0


#define	HBA_PROC_INFO	0
#define	HBA_IOBUS_TYPE	1

#define HBA_BUS_ISA		0
#define HBA_BUS_EISA	1

#define	HBA_CPU_UNK		0
#define	HBA_CPU_i386	1
#define	HBA_CPU_i486	2
#define	HBA_CPU_i586	3
#define	HBA_CPU_i686	4
#define	HBA_CPU_i786	5


#define HBA_CE_CONT		0
#define HBA_CE_NOTE		1
#define HBA_CE_WARN		2
#define HBA_CE_PANIC	3

#define HBA_B_WRITE		0x00
#define HBA_B_READ		0x01
#define HBA_B_PHYS		0x10

#define HBA_EPERM		1
#define HBA_ENOENT		2
#define HBA_EIO			5
#define HBA_EAGAIN		11
#define HBA_EWOULDBLOCK	HBA_EAGAIN
#define HBA_ENOMEM		12
#define HBA_ENODEV		19
#define HBA_ECONFIG		157

#define PDI_UNIXWARE11  1
#define PDI_SVR42_DTMT  2
#define PDI_SVR42MP     3
#define PDI_UNIXWARE20  4

#define DEVICES_MAX		32
#define EDT_MAX			DEVICES_MAX





struct cpuparms {
	ulong_t cpu_id;
	ulong_t cpu_step;
	ulong_t cpu_resrvd[2];
};

struct jpool {
	struct jpool	*nextp;
};

struct head {
	unsigned int	size;
	struct jpool	*jpp;
};



#if HIMEM
/* This module uses proc_t to pass iotype and hil to PDI_vtop() */
typedef struct proc_s {
	unsigned char	iotype;		/* F_DMA_24 and other flags */
	hil_t			*hil;		/* pointer to pointer to himem coversion info */
	int				rw;			/* D_READ or D_WRITE access type to data */
} proc_t;
#else
typedef int		proc_t;
#endif



#define TARGET_MAX		8
#define MAX_HIMEM_PAGES	16


struct hba_softc_s {
	struct watchdog_t	wd;			/* must be present & first element */

	/* driver specific data */
	struct hba_info		*infop;
	struct hba_idata_v4	*idatap;
	int					unit;		/* convert from hbano to OSF unit */
};





/* forward declaration of functions */
LOCAL int hba_probe(unsigned int port, struct bus_ctlr *ui);
LOCAL void hba_set_data(target_info_t *tgt,struct xsb *xsb,
	unsigned int cmd_count, unsigned int in_count);
LOCAL void hba_go(target_info_t *tgt, unsigned int cmd_count,
	unsigned int in_count, boolean_t cmd_only);
LOCAL void hba_reset_scsibus(struct hba_softc_s *hbap);
LOCAL int sdi_cmp_scsi_adr(struct scsi_adr *sar1, struct scsi_adr *sar2);
LOCAL void sdi_init(void);
EXPORT struct sdi_edt *sdi_redt(int hba, int scsi_id, int lun);
LOCAL struct sdi_edt *sdi_insert_edt(struct scsi_adr *sarp, struct ident *ip);
LOCAL void sdi_remove_edt(struct sdi_edt *sep);
EXPORT struct jpool *sdi_get(struct head *headp, int sleepflag);
EXPORT struct sb *sdi_getblk(int sleepflag);
EXPORT void sdi_free(struct head *headp, struct jpool *jpp);
EXPORT long sdi_freeblk(struct sb *sbp);
EXPORT int sdi_gethbano(int hbano);
LOCAL int sdi_scsi_command(struct scsi_adr	*sarp, void *cmdpt, int cmdsz,
	void *datapt, int datasz, int mode);
EXPORT int sdi_translate(struct sb *sbp, int bflags, proc_t *procp,
	int sleepflag);
LOCAL int sdi_inquire(struct scsi_adr *sarp, scsi_command_group_0 *scg0p,
	struct ident *identp);
LOCAL int sdi_find_targets(struct hba_softc_s *hscp);
EXPORT int sdi_register(void *v_infop, void *v_idatap);
EXPORT void sdi_callback(struct sb *sbp);
LOCAL void sdi_cmd_done(struct sb *sbp);
EXPORT void PDI_biodone(hba_buf_t *bp);
EXPORT int PDI_biowait(hba_buf_t *bp);
EXPORT void PDI_cmn_err(int level, const char *fmt, ...);
EXPORT int PDI_drv_getparm(ulong_t parm, void *value_p);
EXPORT int PDI_drv_priv(void *credp);
EXPORT void PDI_drv_usecwait(hba_clock_t count);
EXPORT void PDI_freerbuf(hba_buf_t *bp);
EXPORT minor_t PDI_getminor(unsigned long dev);
EXPORT dev_t PDI_makedevice(major_t major, minor_t minor);
EXPORT hba_buf_t *PDI_getrbuf(int sleepflag);
EXPORT ulong_t PDI_ind(int port);
EXPORT void PDI_outd(int port, ulong_t value);
EXPORT void PDI_kmem_free(void *addr, size_t size);
EXPORT void *PDI_kmem_alloc(size_t size, int flags);
EXPORT void *PDI_kmem_zalloc(size_t size, int flags);
EXPORT void PDI_mod_drvattach(struct mod_drvintr *mdp);
EXPORT void PDI_mod_drvdetach(struct mod_drvintr *aip);
EXPORT int PDI_physiock(void (*strat)(hba_buf_t *bp), hba_buf_t *bp,
	dev_t dev, int rw, daddr_t devsize, hba_uio_t *uiop);
EXPORT paddr_t PDI_vtop(caddr_t vaddr, proc_t *procp);
LOCAL boolean_t hba_probe_target(target_info_t *tgt, io_req_t ior);
EXPORT ulong_t PDI_ptob(ulong_t page);
EXPORT int PDI_spl5s(void);
EXPORT long sdi_swap32(ulong_t value);
EXPORT caddr_t PDI_physmap(paddr_t paddr, ulong_t size, uint_t sleepflag);
EXPORT void PDI_physmap_free(caddr_t caddr, ulong_t size, uint_t sleepflag);
EXPORT int PDI_drv_gethardware(ulong_t parm, void *valuep);
EXPORT void PDI_wakeup(caddr_t caddr);
EXPORT int PDI_sleep(caddr_t caddr, int mask);





LOCAL struct hba_softc_s	hba_softc [NHBA] = { 0 };

/* Used to assign unique HBA controller numbers in sdi_gethbano() */
LOCAL unsigned char hbano_count = { 0 };

/* Used to assign HBA controller to OSF unit numbers */
LOCAL unsigned char used_hbano_count = { 0 };

LOCAL caddr_t			hba_std [NHBA] = { 0 };
LOCAL struct bus_device	*hba_dinfo [NHBA * TARGET_MAX] = { 0 };
LOCAL struct bus_ctlr	*hba_minfo [NHBA] = { 0 };

EXPORT struct bus_driver	hba_driver = {
	(int (*)(caddr_t port, void *ui)) hba_probe,
	(int (*) (struct bus_device *device, caddr_t virt)) scsi_slave,
	(void (*) (struct bus_device *device)) scsi_attach,
	(int (*)(void)) hba_go,
	hba_std,
	"rz",
	hba_dinfo,
	"hbac",
	hba_minfo,
	/* BUS_INTR_B4_PROBE */
};

LOCAL int	hba_doing_init = { 0 };


LOCAL unsigned char		cmd_data [DEVICES_MAX][256] = { 0 };
LOCAL unsigned char		cmd_count = { 0 };

LOCAL unsigned char		lg_data [DEVICES_MAX][LG_POOLSIZE] = { 0 };
LOCAL unsigned char		sm_data [DEVICES_MAX][SM_POOLSIZE] = { 0 };

LOCAL int				sdi_inited = { 0 };

LOCAL unsigned int		num_edt = { 0 };
LOCAL struct sdi_edt	*edt_table [EDT_MAX] = { 0 };
LOCAL struct sdi_edt	edt_cache [EDT_MAX] = { 0 };

LOCAL int				sdi_sleepflag = { NOSLEEP };


EXPORT struct head		lg_poolhead = { LG_POOLSIZE, NULL };
EXPORT struct head		sm_poolhead = { SM_POOLSIZE, NULL };
EXPORT int				sdi_started = { 0 };










LOCAL int
hba_probe(
	unsigned int	port,
	struct bus_ctlr	*ui)
{
	unsigned char	driverno;

HBAMSG(2, ("HBA: hba_probe: The Sarga begins ....\n"));
#if HBA_DBG > 10
__asm__("int3");
#endif

	if (hbano_count < NHBA) {
		if (used_hbano_count == hbano_count) {
			/* setup unit so sdi_register() will have it for scsi_master_alloc()
			 * and scsi_slave_alloc()
			 */
			hba_softc[used_hbano_count].unit = ui->unit;
		}
		else if (hba_softc[used_hbano_count].unit != ui->unit) {
			PDI_cmn_err(HBA_CE_WARN, "HBA: hba_probe: HBA controllers of the "
			  "same type MUST have consecutive unit numbers (unit = %d)\n",
			  ui->unit);
		}
	}

	for (driverno = 0;  driverno < num_hba_driver; ++driverno) {
		/* if a more controllers found than used, we have found one */
		if (used_hbano_count < hbano_count)
			break;

		/* Only load a particular HBA driver once.  HBA drivers will locate all
		 * controllers during the load.
		 */
		if (!hba_driver_tab[driverno].probed) {
			hba_driver_tab[driverno].probed = !0;

			hba_doing_init = !0;
			sdi_sleepflag = NOSLEEP;

			/* calling the load function will cause sdi_gethbano() and
			 * sdi_register() to be called once for each HBA controller found
			 * for the given driver.
			 */
			(void) hba_driver_tab[driverno].mwp->mw_load();

			sdi_sleepflag = SLEEP;
			hba_doing_init = 0;
		}
	}
	if (used_hbano_count < hbano_count) {
		++used_hbano_count;
		/* 
		 * The Toshiba CD-ROM appears to hang after executing
		 * a start unit command while resetting,  Since I can't
		 * remove the reset from the 7850 driver code (don't
		 * have source code), we wait for the device to reset.
		 */
		delay(7000000);
		return (!0);
	}
	return (0);
}






LOCAL boolean_t
hba_probe_target(
	target_info_t	*tgt,
	io_req_t		ior)
{
	struct scsi_adr		sar;
	struct hba_softc_s	*hscp;

HBAMSG(2, ("hba_probe_target(unit=%d, id=%d, lun=%d)\n", tgt->masterno, tgt->target_id, tgt->lun));

	if (!tgt->cmd_ptr) {
		/* A previously undiscovered device.  For now I don't won't to
		 * have to think about allocating himem area's and sep's on the fly,
		 * just fail always.
		 */
		return (FALSE);
	}

	hscp = (struct hba_softc_s *) tgt->hw_state;

	if (scsi_inquiry(tgt, SCSI_INQ_STD_DATA) == SCSI_RET_DEVICE_DOWN)
		return (FALSE);
	
	/* update the inquiry data in the edt table */
	sar.scsi_ctl = hscp->idatap->cntlr;	/* hbano */
	sar.scsi_bus = 0;
	sar.scsi_target = tgt->target_id;
	sar.scsi_lun = tgt->lun;
	(void) sdi_insert_edt(&sar, (struct ident *) tgt->cmd_ptr);

	tgt->flags = TGT_ALIVE;

#if HBA_DBG > 10
__asm__("int3");
#endif

	return (TRUE);
}




LOCAL void
hba_set_data(
	target_info_t	*tgt,
	struct xsb		*xsb,
	unsigned int	cmd_count,
	unsigned int	in_count)
{
	unsigned short	mode;
	caddr_t			datapt;
	long			datasz;
	caddr_t			cmdpt;
	long			cmdsz;

HBAMSG(10, ("hba_set_data()"));

	mode = SCB_WRITE;
	cmdsz = cmd_count;
	cmdpt = (caddr_t) tgt->cmd_ptr;

	switch (tgt->cur_cmd) {
		case SCSI_CMD_LONG_READ:
		case SCSI_CMD_READ:
#if HBA_DBG > 2
{
	scsi_command_group_0	*rp = (scsi_command_group_0 *) cmdpt;

	HBAMSG(2, ("[C=0x%x,%x B=0x%02x%02x%02x L=%d F=0x%x]\n",
	  rp->scsi_cmd_code, rp->scsi_cmd_lun_and_lba1,
	  (rp->scsi_cmd_lun_and_lba1 & SCSI_LBA_MASK), rp->scsi_cmd_lba2,
	  rp->scsi_cmd_lba3, rp->scsi_cmd_xfer_len, rp->scsi_cmd_ctrl_byte));
}
#endif
			datapt = (caddr_t) tgt->ior->io_data;
			datasz = in_count;
			mode = SCB_READ;
HBAMSG(2, ("[R 0x%x-%d]", datapt, datasz));
bzero(datapt, datasz);
			break;

		case SCSI_CMD_WRITE:
		case SCSI_CMD_LONG_WRITE:
			datapt = (caddr_t) tgt->ior->io_data;
			datasz = tgt->ior->io_count;
HBAMSG(2, ("[W %d]", datasz));
			break;

		case SCSI_CMD_INQUIRY:
		case SCSI_CMD_REQUEST_SENSE:
		case SCSI_CMD_MODE_SENSE:
		case SCSI_CMD_RECEIVE_DIAG_RESULTS:
		case SCSI_CMD_READ_CAPACITY:
		case SCSI_CMD_READ_BLOCK_LIMITS:
			datapt = (caddr_t) tgt->cmd_ptr;
			datasz = in_count;
			mode = SCB_READ;
HBAMSG(2, ("[S_R(0x%x) %d]", tgt->cur_cmd, datasz));
			break;

		case SCSI_CMD_MODE_SELECT:
		case SCSI_CMD_REASSIGN_BLOCKS:
		case SCSI_CMD_FORMAT_UNIT:
			cmdsz = sizeof (scsi_command_group_0);
			datasz = cmd_count - cmdsz;
			datapt = (caddr_t) ((char *) tgt->cmd_ptr + cmdsz);
HBAMSG(2, ("[S_W(0x%x) %d]", tgt->cur_cmd, datasz));
			break;

		default:
			datapt = NULL;
			datasz = 0;
			mode = SCB_WRITE;
HBAMSG(2, ("[O(0x%x) %d]", tgt->cur_cmd, cmdsz));
			break;
	}
	xsb->sb.SCB.sc_cmdpt = cmdpt;
	xsb->sb.SCB.sc_cmdsz = cmdsz;
	xsb->sb.SCB.sc_datapt = datapt;
	xsb->sb.SCB.sc_datasz = datasz;
	xsb->sb.SCB.sc_mode = mode;
HBAMSG(8, ("\n"));
	return;
}




LOCAL void
hba_go(
	target_info_t	*tgt,
	unsigned int	cmd_count,
	unsigned int	in_count,
	boolean_t		cmd_only)
{
#if HIMEM
	struct proc_s		proc;
#endif
	struct proc_s		*procp;
	struct hba_softc_s	*hscp = (struct hba_softc_s *) tgt->hw_state;
	struct xsb			*xsb;
	struct sdi_edt		*sep;
	unsigned int		bflags;
	int					rc;
	at386_io_lock_state();

HBAMSG(2, ("hba_go(unit=%d, id=%d, lun=%d)", tgt->masterno, tgt->target_id, tgt->lun));
HBAMSG(10, ("[tgt=0x%x, tgt->ior=0x%x, tgt->dev_ops=0x%x]\n", tgt, tgt->ior, tgt->dev_ops));
HBAMSG(10, ("[C=0x%x M=%d I=%d]", tgt->cur_cmd, tgt->masterno, tgt->target_id));

	/* assume the worst, set the done flag for the default case.
	 * This should work on a multiprocessor machine as
	 * it will only be during startup that a busy wait will occur by
	 * polling the tgt->done variable, and I read that only one thread
	 * is running at startup.  After that the caller will be informed
	 * by a callback so you wont get a race condition.
	 */
	tgt->done = SCSI_RET_ABORTED;

	if (!(sep = sdi_redt(hscp->idatap->cntlr, tgt->target_id, tgt->lun))) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: hba_go: sdi_redt(%d, %d, %d) failed\n",
		  hscp->idatap->cntlr, tgt->target_id, tgt->lun);
		if (tgt->ior)
			(tgt->dev_ops->restart)(tgt, TRUE);
		return;
	}

	/***********************************************/
	/* CONVERT from OSF scsi to SDI/HBA structures */
	/***********************************************/

	/* allocate a HBA SCSI request structure */
	xsb = (struct xsb *) sdi_getblk(sdi_sleepflag);
	if (!xsb) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: hba_go: sdi_getblk() failed\n");
		if (tgt->ior)
			(tgt->dev_ops->restart)(tgt, TRUE);
		return;
	}

	/* allocate HBA specific portion of the xsb data structure */
	xsb->hbadata_p = (hscp->infop->hba_getblk)(sdi_sleepflag,
	  hscp->idatap->cntlr);
	if (!xsb->hbadata_p) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: hba_go: hba_getblk failed()\n");
		(void) sdi_freeblk(&xsb->sb);
		if (tgt->ior)
			(tgt->dev_ops->restart)(tgt, TRUE);
		return;
	}

	/* setup back link to xsb in hbadata_p */
	xsb->hbadata_p->sb = xsb;

	/* the type of HBA scsi request, use SCB part of union in request */
	xsb->sb.sb_type = SCB_TYPE;

	/* function to execute (by sdi_callback()) when command is complete */
	xsb->sb.SCB.sc_int = sdi_cmd_done;

	/* Save reference to OSF SCSI request structure in HBA request structure.
	 * This will be used by the sdi_cmd_done() function.
	 * Personally I think it is bad design to use a long to store a pointer
	 * but this is what is done by HBA Target drivers.
	 */
	xsb->sb.SCB.sc_wd = (long) tgt;

	xsb->sb.SCB.sc_link = NULL;
	xsb->sb.SCB.sc_resid = 0;
	xsb->sb.SCB.sc_time = 10 * 1000;	/* ten seconds */

	xsb->sb.SCB.sc_dev.sa_major = 0;

	xsb->sb.SCB.sc_dev.sa_minor = SC_MKMINOR(sep->scsi_adr.scsi_ctl,
	  sep->scsi_adr.scsi_target, sep->scsi_adr.scsi_lun,
	  sep->scsi_adr.scsi_bus);
	xsb->sb.SCB.sc_dev.sa_lun = sep->scsi_adr.scsi_lun;
	xsb->sb.SCB.sc_dev.sa_bus = sep->scsi_adr.scsi_bus;
	xsb->sb.SCB.sc_dev.sa_exta = sep->scsi_adr.scsi_target;
	xsb->sb.SCB.sc_dev.sa_ct = SDI_SA_CT(sep->scsi_adr.scsi_ctl,
	  sep->scsi_adr.scsi_target);

	hba_set_data(tgt, xsb, cmd_count, in_count);

	bflags = (xsb->sb.SCB.sc_mode & SCB_READ?  HBA_B_READ:  HBA_B_WRITE);

#if HIMEM
#if 0
	/* ensure that the procp gets passed all the way through to vtop by
	 * setting this flag
	 */
	bflags |= HBA_B_PHYS;
#endif

	if (sep->iotype & F_DMA_24) {
		/* procp will be re-used to pass a pointer to information for
		 * the vtop() function to determine if himem_convert() is required.
		 * I think it sucks to convert an array of 4 chars into storage for
		 * a hil_t pointer, but again, this is what is done by OSF SCSI
		 * drivers.
		 */
		procp = &proc;
		procp->iotype = sep->iotype;
		procp->hil = (hil_t *) tgt->transient_state.hba_dep;
		procp->rw = (bflags & HBA_B_READ?  D_READ:  D_WRITE);

		/* set hil to NULL so that sdi_cmd_done() will not call himem_revert()
		 * if himem_convert() not used in hba_xlat()=>vtop().
		 */
		*procp->hil = NULL;
	}
	else
#endif
		procp = NULL;

	/* get driver to convert from virtual to physical */
	rc = (hscp->infop->hba_xlat)(xsb->hbadata_p, bflags, procp, sdi_sleepflag);

	if ((hscp->idatap->version_num & HBA_VMASK) > HBA_SVR4_2  &&
	  rc != SDI_RET_OK) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: hba_g: hba_xlat failed()\n");
		xsb->sb.SCB.sc_comp_code == SDI_MEMERR;
		sdi_cmd_done(&xsb->sb);
	}
	else
	{
		tgt->done = SCSI_RET_IN_PROGRESS;

		if (sdi_sleepflag != NOSLEEP)
			at386_io_lock(MP_DEV_WAIT);

		rc = (hscp->infop->hba_send)(xsb->hbadata_p, sdi_sleepflag);

		if (sdi_sleepflag != NOSLEEP)
			at386_io_unlock();

		/* NOTE: at this point it can be assumed that sdi_callback() will
		 * be called, and it can free up the HBA request structures.
		 * Also xsb could already be free'ed by now so it can't be touched.
		 */

		if (rc != SDI_RET_OK)
			PDI_cmn_err(HBA_CE_WARN, "HBA: hba_g: hba_send failed()\n");
	}
	return;
}





LOCAL void
hba_reset_scsibus(
	struct hba_softc_s	*hscp)
{

HBAMSG(2, ("hba_reset_scsibus()\n"));

	/* reset the scsi bus on a timeout */
	return;
}










LOCAL void
sdi_init(void)
{
	int	i;

HBAMSG(3, ("sdi_init()\n"));

	/* initialise the large and small pools */
	for (i = 0;  i < DEVICES_MAX;  ++i) {
		struct jpool	*jpp;

		jpp = (struct jpool *) lg_data[i];
		jpp->nextp = lg_poolhead.jpp;
		lg_poolhead.jpp = jpp;

		jpp = (struct jpool *) sm_data[i];
		jpp->nextp = sm_poolhead.jpp;
		sm_poolhead.jpp = jpp;
	}
	sdi_inited = !0;
	return;
}







/* sdi_cmp_scsi_addr() -
 *	compares two scsi_adr's and determines whether they are:
 *	sar1 == sar2	0
 *	sar1 <  sar2	-1
 *	sar1 >  sar2	1
 */

LOCAL int
sdi_cmp_scsi_adr(
	struct scsi_adr	*sar1,
	struct scsi_adr	*sar2)
{

HBAMSG(10, ("sdi_cmp_scsi_adr([%d %d], [%d %d])\n", sar1->scsi_ctl, sar1->scsi_target, sar2->scsi_ctl, sar2->scsi_target));

	if (sar1->scsi_ctl < sar2->scsi_ctl)
		return (-1);
	if (sar1->scsi_ctl > sar2->scsi_ctl)
		return (1);

#if BUS_SUPPORT
	if (sar1->scsi_bus < sar2->scsi_bus)
		return (-1);
	if (sar1->scsi_bus > sar2->scsi_bus)
		return (1);
#endif	/* BUS_SUPPORT */

	if (sar1->scsi_target < sar2->scsi_target)
		return (-1);
	if (sar1->scsi_target > sar2->scsi_target)
		return (1);

#if LUN_SUPPORT
	if (sar1->scsi_lun < sar2->scsi_lun)
		return (-1);
	if (sar1->scsi_lun > sar2->scsi_lun)
		return (1);
#endif

	return (0);
}






/* sdi_redt()
 *	Look up the given target in the equipment data table, and if found
 *	return a pointer to it.  The table is an array of sorted pointers to
 *	'struct sdi_edt's, so a binary search is performed.
 *	The PDI manual indicates that the SDI uses hash tables and the sdi_edt
 *	has fields in it to support this, so latter this function may use hashing
 *	for even faster lookup speed.
 */

EXPORT struct sdi_edt *
sdi_redt(
	int	hba,
	int	scsi_id,
	int	lun)
{
	struct scsi_adr	sar;
	unsigned int	min;
	unsigned int	mid;
	unsigned int	max;
	int				cmp;

HBAMSG(10, ("sdi_redt(%d, %d, %d)\n", hba, scsi_id, lun));

	min = 0;
	max = num_edt;

	sar.scsi_ctl = hba;
	sar.scsi_bus = 0;
	sar.scsi_target = scsi_id;
	sar.scsi_lun = lun;

	/* binary search for entry */
	while (min != max) {
		mid = min + ((max - min) / 2);
		cmp = sdi_cmp_scsi_adr(&sar, &edt_table[mid]->scsi_adr);
		if (cmp == 0)
			break;
		if (cmp < 0)
			max = mid;
		else
			min = mid + 1;
	}

	if (min == max)
		return (NULL);
	return (edt_table[mid]);
}






LOCAL struct sdi_edt *
sdi_insert_edt(
	struct scsi_adr	*sarp,
	struct ident	*ip)
{
	struct scsi_ad		sa;
	struct hbagetinfo	hgi;
	char				name [SDI_NAMESZ];
	struct sdi_edt		*sep;
	int					min = 0;
	int					cur;
	int					ins;
	int					cmp;

HBAMSG(4, ("sdi_insert_edt(H=%d, B=%d, I=%d, L=%d)\n", sarp->scsi_ctl, sarp->scsi_bus, sarp->scsi_target, sarp->scsi_lun));

	/* see if it is already in the table */
	sep = sdi_redt(sarp->scsi_ctl, sarp->scsi_target, sarp->scsi_lun);
	if (!sep) {
		if (num_edt == EDT_MAX) {
			PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_inset_edt: Too many edt_table "
			  "entries, max=%d\n", EDT_MAX);
			return (NULL);
		}

		/* search back until an entry is found that is less than the one about
		 * to be inserted, shuffling a hole down to the spot.  This will ensure
		 * the table of entries is sorted on scsi_adr.  This will allow a
		 * binary search to be carried out on the table later.
		 */
		cur = num_edt;
		while (cur > min) {
			cmp = sdi_cmp_scsi_adr(sarp, &(edt_table[cur - 1]->scsi_adr));
			if (cmp > 0)
				break;
			edt_table[cur] = edt_table[cur - 1];
			--cur;
		}
		ins = cur;

		for (cur = 0;  cur < EDT_MAX  &&  edt_cache[cur].res1;  ++cur)
			;

		assert(cur != EDT_MAX);

		sep = &edt_cache[cur];
		edt_table[ins] = sep;

		/* increment count as extra entry added to table */
		++num_edt;

		/* mark as in use */
		sep->res1 = !0;

		sep->scsi_adr = *sarp;
		sep->hba_no = sarp->scsi_ctl;
		sep->scsi_id = sarp->scsi_target;
		sep->lun = sarp->scsi_lun;

		/* setup arguments for a call to hba_getinfo, to get 'iotype' */
		sa.sa_major = 0;
		sa.sa_minor = SC_MKMINOR(sarp->scsi_ctl, sarp->scsi_target,
		  sarp->scsi_lun, sarp->scsi_bus);
		sa.sa_lun = sarp->scsi_lun;
		sa.sa_bus = sarp->scsi_bus;
		sa.sa_exta = sarp->scsi_target;
		sa.sa_ct = SDI_SA_CT(sarp->scsi_ctl, sarp->scsi_target);
		hgi.name = name;

		if (hba_softc[sarp->scsi_ctl].infop->hba_getinfo)
			(hba_softc[sarp->scsi_ctl].infop->hba_getinfo)(&sa, &hgi);
		else {
			PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_inset_edt: NULL hba_getinfo()");
			hgi.iotype = 0;
		}
		sep->iotype = hgi.iotype;
	}

	sep->edt_ident =  *ip;
	sep->pdtype = ip->id_type;
	bcopy(ip->id_vendor, sep->inquiry, sizeof (sep->inquiry) - 1);

	return (sep);
}




LOCAL void
sdi_remove_edt(
	struct sdi_edt	*sep)
{
	int	max = num_edt - 1;
	int	cur;

HBAMSG(4, ("sdi_remove_edt()\n"));

	assert(num_edt != 0);

	/* mark as NOT in use */
	sep->res1 = 0;

	/* find the sep in the edt_table */
	for (cur = 0;  cur < num_edt  &&  sep != edt_table[cur];  ++cur)
		;
	
	assert(cur != num_edt);

	/* Simply shuffle back whole table over top of entry to be removed,
	 * this will ensure table remains sorted.
	 */
	while (cur < num_edt - 1) {
		edt_table[cur] = edt_table[cur + 1];
		++cur;
	}

	/* one less edt_table entry */
	--num_edt;
	return;
}





/* sdi_get()
 *	Allocate a buffer from the pool pointed to by the headp passed in.
 *	headp:
 *		must be &lg_poolhead or &sm_poolhead
 *	sleepflag
 *		Currently ignored.  Later we could use sleep() and wakeup() to
 *		wait for a buffer here if the sleep flag permits.
 *		Also we coud always leave a free buffer for no-sleep callers.
 *	Currently the returned buffer is guarenteed to be below 16Meg for
 *	DMA purposes.  It is not, however, guarenteed to be contiguous.
 *	Latter this may cause problems, and calls to himem.c may be required
 *	which will allow dynamic allocation and PAGE allignment of buffers.
 *
 *  This may not be called at interrupt time, but the corresponding function
 *	sdi_free() may well be.  We will have to protect from both interrupt
 *	and multi-processor while playing with the free list!
 */

EXPORT struct jpool *
sdi_get(
	struct head	*headp,
	int			sleepflag)
{
	struct jpool	*jpp;
	spl_t			x;

HBAMSG(10, ("sdi_get()\n"));

	assert(headp == &lg_poolhead  ||  headp == &sm_poolhead);

	if (!sdi_inited)
		sdi_init();

	x = splhigh();
	jpp = headp->jpp;
	if (jpp)
		headp->jpp = jpp->nextp;
	else {
		PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_get: Out of %d sized pool\n",
		  headp->size);
	}
	splx(x);

	bzero((void *) jpp, headp->size);
	return (jpp);
}






/* sdi_getblk()
 *	
 */
EXPORT struct sb *
sdi_getblk(
	int	sleepflag)
{

HBAMSG(10, ("sdi_getblk()\n"));

	return ((struct sb *) sdi_get(&lg_poolhead, sleepflag));
}




/* sdi_free()
 *	return a buffer back to the pool.
 *	 We will have to protect from both interrupt and multi-processor while
 *	playing with the free list!
 */
EXPORT void
sdi_free(
	struct head		*headp,
	struct jpool	*jpp)
{
	spl_t	x;

HBAMSG(10, ("sdi_free()\n"));

	assert(headp == &lg_poolhead  ||  headp == &sm_poolhead);
	assert(sdi_inited);
	assert(jpp);

	x = splhigh();
	jpp->nextp = headp->jpp;
	headp->jpp = jpp;
	splx(x);

	return;
}





EXPORT long
sdi_freeblk(
	struct sb	*sbp)
{

HBAMSG(10, ("sdi_freeblk()\n"));

	sdi_free(&lg_poolhead, (struct jpool *) sbp);
	return (SDI_RET_OK);
}





/* sdi_gethbano() -
 *	returns a unique number to identify this HBA controller number.
 *	This implementation ignores the value passed, will assign one always,
 *	that can be used interchanably with the unit number.
 */
EXPORT int
sdi_gethbano(
	int	hbano)
{

HBAMSG(3, ("sdi_gethbano(%d)\n", hbano));

	if (hbano_count == NHBA)
		return (-HBA_ENODEV);
	return (hbano_count++);
}




/* sdi_scsi_command()
 *	Executes a SCSI command and waits for it to complete.
 *	Can only be called at init time.
 *	mode:
 *		SCB_WRITE or SCB_READ
 *	cmdpt:
 *	datapt:
 *		Must be in kernel data space (below 16meg) as himem.c functions
 *		are not going to be used for the initialisation SCSI commands
 */

LOCAL int
sdi_scsi_command(
	struct scsi_adr	*sarp,
	void			*cmdpt,
	int				cmdsz,
	void			*datapt,
	int				datasz,
	int				mode)
{
	struct hba_softc_s	*hscp;
	struct xsb			*xsb;
	unsigned int		bflags;
	int					rc;

HBAMSG(2, ("sdi_scsi_command()\n"));

	hscp = &hba_softc[sarp->scsi_ctl];				/* scsi_ctl == hbano */
	xsb = (struct xsb *) sdi_getblk(sdi_sleepflag);

	if (!xsb) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_scsi_command: sdi_getblk "
		  "failed()\n");
		return (SDI_MEMERR);
	}

	/* allocate HBA specific portion of the xsb data structure */
	xsb->hbadata_p = (hscp->infop->hba_getblk)(sdi_sleepflag, sarp->scsi_ctl);
	
	if (!xsb->hbadata_p) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_scsi_command: hba_getblk "
		  "failed()\n");
		(void) sdi_freeblk(&xsb->sb);
		return (SDI_MEMERR);
	}

	xsb->hbadata_p->sb = xsb;

	xsb->sb.sb_type = ISCB_TYPE;
	xsb->sb.SCB.sc_int = NULL;
	xsb->sb.SCB.sc_wd = 0;
	xsb->sb.SCB.sc_link = NULL;

	xsb->sb.SCB.sc_dev.sa_major = 0;
	xsb->sb.SCB.sc_dev.sa_minor = SC_MKMINOR(sarp->scsi_ctl, sarp->scsi_target,
	  sarp->scsi_lun, sarp->scsi_bus);
	xsb->sb.SCB.sc_dev.sa_lun = sarp->scsi_lun;
	xsb->sb.SCB.sc_dev.sa_bus = sarp->scsi_bus;
	xsb->sb.SCB.sc_dev.sa_exta = sarp->scsi_target;
	xsb->sb.SCB.sc_dev.sa_ct = SDI_SA_CT(sarp->scsi_ctl, sarp->scsi_target);

	xsb->sb.SCB.sc_cmdpt = cmdpt;
	xsb->sb.SCB.sc_cmdsz = cmdsz;
	xsb->sb.SCB.sc_datapt = datapt;
	xsb->sb.SCB.sc_datasz = datasz;
	xsb->sb.SCB.sc_mode = mode;

	xsb->sb.SCB.sc_resid = 0;
	xsb->sb.SCB.sc_time = 0;	/* no timeout */

	/* construct bflags from mode */
	bflags = (mode & SCB_READ?  HBA_B_READ:  HBA_B_WRITE);

#if 0
	/* To ensure that the procp gets passed all the way through to vtop,
	 * set the HBA_B_PHYS flag.  Not very important in this case as procp
	 * will be NULL.
	 */
	bflags |= HBA_B_PHYS;
#endif

HBAMSG(4, ("[cmdpt=0x%x cmdsz=%d datapt=0x%x datasz=%d mode=0x%x bflags=0x%x]", cmdpt, cmdsz, datapt, datasz, mode, bflags));

	/* get driver to convert from virtual to physical */
	rc = (hscp->infop->hba_xlat)(xsb->hbadata_p, bflags, NULL, sdi_sleepflag);
	
	if ((hscp->idatap->version_num & HBA_VMASK) > HBA_SVR4_2  &&
	  rc != SDI_RET_OK) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_scsi_command: hba_xlat failed()"
		  " %d\n", rc);
		rc = SDI_MEMERR;
	}
	else {
		/* Since this hba_icmd is being called during init time this function
		 * will not return until the command is complete.
		 */
		rc = (hscp->infop->hba_icmd)(xsb->hbadata_p, sdi_sleepflag);
		
		if (rc != SDI_RET_OK)
			rc = SDI_HAERR;
		else
			rc = xsb->sb.SCB.sc_comp_code;
	}

	/* Free data used for command */
	if ((hscp->infop->hba_freeblk)(xsb->hbadata_p, sarp->scsi_ctl) !=
	  SDI_RET_OK) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_scsi_command: hba_freeblk() "
		  "failed\n");
	}
	xsb->hbadata_p = NULL;
	(void) sdi_freeblk(&xsb->sb);

	return (rc);
}






/* sdi_translate() -
 *	This function should:
 *		- If not done already, allocate a driver structure via hba_getblk.
 *		- Call the HBA drivers hba_xlat
 *	But it appears that this function is only used by HBA drivers for
 *	pass-through mode, and as this module does not use HBA drivers in
 *	pass-through mode, it should never be called, so dummy it.
 */

EXPORT int
sdi_translate(
	struct sb	*sbp,
	int			bflags,
	proc_t		*procp,
	int			sleepflag)
{

HBAMSG(2, ("sdi_translate()\n"));

	PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_translate: Not implemented\n");
	return (SDI_RET_ERR);
}





/* sdi_inquire() -
 *	Sets up a SCSI INQUIRY command and calls the HBA driver to execute it.
 *	Returns SDI_ASW for success
 */

LOCAL int
sdi_inquire(
	struct scsi_adr			*sarp,
	scsi_command_group_0	*scg0p,
	struct ident			*identp)
{
	int		rc;

HBAMSG(2, ("sdi_inquire(H=%d, B=%d, I=%d, L=%d)\n", sarp->scsi_ctl, sarp->scsi_bus, sarp->scsi_target, sarp->scsi_lun));

	scg0p->scsi_cmd_code = SCSI_CMD_INQUIRY;
	scg0p->scsi_cmd_lun_and_lba1 = (sarp->scsi_lun << 5);
	scg0p->scsi_cmd_lba2 = 0;
	scg0p->scsi_cmd_lba3 = 0;
	scg0p->scsi_cmd_xfer_len = sizeof (*identp);
	scg0p->scsi_cmd_ctrl_byte = 0;

	rc = sdi_scsi_command(sarp, scg0p, sizeof (*scg0p), identp,
	  sizeof (*identp), SCB_READ);
	
	if (rc == SDI_ASW) {
		if (identp->id_pqual == SCSI_INQ_TNC)
			rc = SDI_HAERR;
	}
	else if (rc == SDI_CKSTAT) {
		/* ??? should check the Sense data ??? */
	}
	
	return (rc);
}








LOCAL int
sdi_find_targets(
	struct hba_softc_s	*hscp)
{
	struct scsi_adr	scsi_adr;
	struct sdi_edt	*sep;
	target_info_t	*tgt;
	int				scsiid;
	int				lun;
	int				bus;
	int				ha_id;
	int				num_targets;
	int				max_dma_data;
	int				max_luns;
	int				max_buses;
	int				max_targets;

HBAMSG(3, ("sdi_find_targets()\n"));

	scsi_adr.scsi_ctl = hscp->idatap->cntlr;	/* hbano */
	ha_id = (int) hscp->idatap->ha_id;

	/* default */
	max_dma_data = hscp->infop->max_xfer;

	if ((hscp->idatap->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
		max_buses = hscp->idatap->idata_nbus;
		max_targets = hscp->idatap->idata_ntargets;
		max_luns = hscp->idatap->idata_nluns;
	}
	else {
		max_buses = 1;
		max_targets = 8;
		max_luns = 8;
	}

#if !LUN_SUPPORT
	max_luns = 1;
#endif

#if !BUS_SUPPORT
	max_buses = 1;
#endif

	for (bus = 0;  bus < max_buses;  ++bus) {
		scsi_adr.scsi_bus = 0;
		scsi_adr.scsi_target = ha_id;
		scsi_adr.scsi_lun = 0;

		/* ensure spare command areas available */
		if (cmd_count == DEVICES_MAX)
			break;

		/* used next free cmd_data area to send command */
		if (sdi_inquire(&scsi_adr, (scsi_command_group_0 *) cmd_data[cmd_count],
		  (struct ident *) cmd_data[cmd_count]) != SDI_ASW) {
			continue;
		}

		if (!(sep = sdi_insert_edt(&scsi_adr, (struct ident *)
		  cmd_data[cmd_count]))) {
			continue;
		}

#if HIMEM
		if (sep->iotype & F_DMA_24) {
			max_dma_data = min(hscp->infop->max_xfer,
			  MAX_HIMEM_PAGES * PAGE_SIZE);
		}
#endif

HBAMSG(2, ("[iotype=0x%x]", sep->iotype));

		/* init count of targets found */
		num_targets = 0;

		for (scsiid = 0;  scsiid < max_targets;  ++scsiid) {
			if (scsiid == ha_id)
				continue;

			scsi_adr.scsi_target = scsiid;

			for (lun = 0; lun < max_luns; lun++) {
				scsi_adr.scsi_lun = lun;

				/* ensure spare command areas available */
				if (cmd_count == DEVICES_MAX)
					break;

				/* used next free cmd_data area to send command */
				if (sdi_inquire(&scsi_adr, (scsi_command_group_0 *)
				  cmd_data[cmd_count], (struct ident *) cmd_data[cmd_count]) !=
				  SDI_ASW) {
					break;
				}

				if (!(sep = sdi_insert_edt(&scsi_adr, (struct ident *)
				  cmd_data[cmd_count]))) {
					break;
				}

				/* register this target with the upper SCSI layers */
				tgt = scsi_slave_alloc(hscp->unit, scsiid, (char *) hscp);
				if (!tgt)
					break;

				tgt->cmd_ptr = cmd_data[cmd_count++];
				tgt->dma_ptr = NULL;

#if HIMEM
				if (sep->iotype & F_DMA_24) {
					/* the number of pages to reserve for himem DMA */
					himem_reserve(max_dma_data / PAGE_SIZE);
				}
#endif

				++num_targets;

				/* limit the number of targets */
				if (num_targets == TARGET_MAX)
					break;
			}
			if (num_targets == TARGET_MAX)
				break;
		}
		if (num_targets == TARGET_MAX)
			break;
	}
	return (max_dma_data);
}




EXPORT int
sdi_register(
	void	*v_infop,
	void	*v_idatap)
{
	struct hba_softc_s	*hscp;
	scsi_softc_t		*scp;

HBAMSG(3, ("sdi_register()\n"));

	hscp = &hba_softc[((struct hba_idata_v4 *) v_idatap)->cntlr];
	hscp->idatap = v_idatap;
	hscp->infop = v_infop;

	/* We have to make up a unit number if this is not the first
	 * controller found on this probe.  The unit number choosen is
	 * the next consecutive one after the last one used.
	 */
	if (used_hbano_count < hbano_count - 1)
		hscp->unit = hba_softc[hbano_count - 1].unit + 1;

	hscp->wd.reset = (void (*) (struct watchdog_t *wd)) hba_reset_scsibus;

	/* register this controller with the upper SCSI layers */
	scp = scsi_master_alloc(hscp->unit, (char *) hscp);
	if (!scp) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_register: scsi_master_alloc() "
		  "failed\n");
		return (-HBA_ENOMEM);
	}

	scp->go = hba_go;
	scp->watchdog = scsi_watchdog;
	scp->probe = hba_probe_target;
	scp->initiator_id = hscp->idatap->ha_id;

	/* look for devices on the SCSI bus */
	scp->max_dma_data = sdi_find_targets(hscp);

	return (hscp->idatap->cntlr);
}







EXPORT long
sdi_swap32(
	ulong_t	value)
{
	long			r;
	unsigned char	*from = (unsigned char *) &value;
	unsigned char	*to = (unsigned char *) &r;

	to[0] = from[3];
	to[1] = from[2];
	to[2] = from[1];
	to[3] = from[0];

HBAMSG(2, ("{0x%x -> 0x%x}", from, to));

	return (r);
}








/* sdi_callback()
 * Called by HBA driver when SCSI command has completed.
 */

EXPORT void
sdi_callback(
	struct sb	*sbp)
{

HBAMSG(2, ("sdi_callback(sc_int=0x%x) ", sbp->SCB.sc_int));

	if (sbp->sb_type == SCB_TYPE  ||  sbp->sb_type == ISCB_TYPE) {
		if (sbp->SCB.sc_int)
			(sbp->SCB.sc_int)(sbp);
	}
	else {
		PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_callback: Non-SCB type\n");
		if (sbp->SFB.sf_int)
			(sbp->SFB.sf_int)(sbp);
	}
	return;
}



/* sdi_cmd_done()
 *	Convert from HBA SCSI command/result back into OSF format and
 *	call the layer above to indicate command completion.
 *
 *	Called at interrupt time from the HBA driver.
 *	(Actually not allways at interrupt time)
 */

LOCAL void
sdi_cmd_done(
	struct sb	*sbp)
{
	struct hba_info		*infop;
	struct xsb			*xsb = (struct xsb *) sbp;
	target_info_t		*tgt = (target_info_t *) xsb->sb.SCB.sc_wd;
	struct hba_softc_s	*hscp = (struct hba_softc_s *) tgt->hw_state;
	unsigned long		comp_code = xsb->sb.SCB.sc_comp_code;
	unsigned char		status = xsb->sb.SCB.sc_status;

HBAMSG(2, ("sdi_cmd_done() "));

	/****************************************************/
	/* CONVERT from SDI/HBA back to OSF scsi structures */
	/****************************************************/

#if HIMEM
	/* if the hil is non-zero, undo the himem_convert() */
	if (*(hil_t *) tgt->transient_state.hba_dep)
		himem_revert(*(hil_t *) tgt->transient_state.hba_dep);
#endif

#if HBA_DBG > 10
if (tgt->cur_cmd == SCSI_CMD_READ) {
	unsigned char			*cp;
	int						i;
	int						j;

HBAMSG(2, ("[R 0x%x-%d]\n", tgt->ior->io_data, tgt->ior->io_count));

	cp = (unsigned char *) tgt->ior->io_data;
	for (i = 0;  i < 64;  i += 16) {
		printf("%04x: ", i);
		for (j = 0;  j < 16;  ++j)
			printf("%02x ", cp[i + j]);
		printf(": ");
		for (j = 0;  j < 16;  ++j) {
			if (cp[i + j] > ' '  &&  cp[i + j] < 0x7f)
				printf("%c", cp[i + j]);
			else
				printf(".");
		}
		printf("\n");
	}

}
#endif

	/* set the SCSI command status */
	if (comp_code == SDI_ASW)
		tgt->done = SCSI_RET_SUCCESS;
	else if (comp_code == SDI_TCERR)
		tgt->done = SCSI_RET_DEVICE_DOWN;
	else if (comp_code == SDI_CKSTAT) {
		scsi_error(tgt, SCSI_ERR_STATUS, status, 0);
		if (status == S_BUSY)
			tgt->done = SCSI_RET_RETRY;
		else
			tgt->done = SCSI_RET_NEED_SENSE;
	}
	else if (comp_code & SDI_RETRY)
		tgt->done = SCSI_RET_RETRY;
	else {
		tgt->done = SCSI_RET_ABORTED;
	}

#if HBA_DBG > 0
if (comp_code != SDI_ASW  &&  comp_code != SDI_CKSTAT) {
	HBAMSG(1, ("[done=0x%x, comp_code=0x%x, status=0x%x]\n", tgt->done, comp_code, status));
	HBAMSG(1, ("[cur_cmd=0x%x masterno=%d target_id=%d lun=%d]\n", tgt->cur_cmd, tgt->masterno, tgt->target_id, tgt->lun));
#if HBA_DBG > 10
	__asm__("int3");
#endif
}
#if HBA_DBG > 8
else {
	if (tgt->done != SCSI_RET_SUCCESS) {
		HBAMSG(8, ("[done=0x%x, comp_code=0x%x, status=0x%x]\n", tgt->done, comp_code, status));
		HBAMSG(8, ("[cur_cmd=0x%x masterno=%d target_id=%d lun=%d]\n", tgt->cur_cmd, tgt->masterno, tgt->target_id, tgt->lun));
	}
	else {
		HBAMSG(8, ("\n"));
	}
}
#endif
#endif

	/* Free data used for command */
	if ((hscp->infop->hba_freeblk)(xsb->hbadata_p, hscp->idatap->cntlr) !=
		SDI_RET_OK) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: sdi_cmd_done: hba_freeblk() failed\n");
	}

	xsb->hbadata_p = NULL;
	(void) sdi_freeblk(&xsb->sb);

	/* This will call back into drivers such as rzdisk.c: scdisk_start() */
	if (tgt->ior)
		(tgt->dev_ops->restart)(tgt, TRUE);

	return;
}






/*-------------------PDI simulated interfaces for HBA drivers---------------*/



#define DRV_GETPARM_UPROCP	2
#define DRV_GETPARM_LBOLT	4






/* Normally this is a 'struct mod_operations', but it is only used in HBA
 * driver wrappers, and not by the HBA driver directly, so it should be fine
 * just to statisfy the HBA driver external reference with this int of the
 * same name
 */
EXPORT int	PDI_mod_misc_ops = { 0 };






EXPORT void
PDI_biodone(
	hba_buf_t	*bp)
{

HBAMSG(5, ("PDI_biodone()\n"));

	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_biodone: Not implemented\n");
	return;
}






EXPORT int
PDI_biowait(
	hba_buf_t	*bp)
{

HBAMSG(5, ("PDI_biowait()\n"));

	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_biowait: Not implemented\n");
	return (-HBA_EIO);
}






EXPORT void
PDI_cmn_err(
	int			level,
	const char	*fmt,
	...)
{
	va_list	listp;
	char	*prefix;
	int		do_panic = FALSE;

	va_start(listp, fmt);
	switch (level) {
		case (HBA_CE_CONT):
			prefix = "";
			break;

		case (HBA_CE_NOTE):
			prefix = "NOTICE: ";
			break;

		case (HBA_CE_WARN):
			prefix = "WARNING: ";
			break;

		default:
		case (HBA_CE_PANIC):
			prefix = "PANIC: ";
			do_panic = TRUE;
			break;
	}
	printf(prefix);
	_doprnt(fmt, &listp, cnputc, 0);
	if (do_panic)
		panic("SDI HBA interface");
	return;
}







EXPORT int
PDI_drv_getparm(
	ulong_t	parm,
	void	*valuep)
{
HBAMSG(4, ("PDI_drv_getparm()\n"));

	switch (parm) {
		case (DRV_GETPARM_LBOLT): {
			hba_clock_t	*clockp = (hba_clock_t *) valuep;
			extern natural_t	timeout_ticks;

			*clockp = timeout_ticks;
			break;
		}

		case (DRV_GETPARM_UPROCP): {
			proc_t	**procp = (proc_t **) valuep;

			*procp = NULL;
			break;
		}
		
		default: {
			PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_drv_getparm: %d Not "
			  "implemented\n", parm);
			return (-1);
		}
	}
	return (0);
}





/* PDI_drv_priv() -
 *	always return that user is Superuser, this function should never
 *	be used by the HBA in this implementation, so it is not a security
 *	hole.
 */
 
EXPORT int
PDI_drv_priv(
	void	*credp)
{

HBAMSG(5, ("PDI_drv_priv()\n"));

	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_drv_priv: Not implemented\n");
	return (0);
}




/* PDI_drv_usecwait() -
 *	delay count microseconds.
 *	One day an accurate, CPU independent, function should be used or
 *	delay() fixed up.
 */

EXPORT void
PDI_drv_usecwait(
	hba_clock_t	count)
{

HBAMSG(20, ("(PDI_drv_usecwait)"));

	delay(count);
	return;
}




/* PDI_freerbuf() -
 *	Should never get called, just dummy it
 */

EXPORT void
PDI_freerbuf(
	hba_buf_t	*bp)
{

HBAMSG(4, ("(PDI_freerbuf)"));

	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_freerbuf: Not implemented\n");
	return;
}





/* PDI_getminor() -
 *	This will only get called by the HBA driver, which will only have SVR4
 *	style devs thrown at it, so this functions will handles dev's the SVR4 way.
 */

EXPORT minor_t
PDI_getminor(
	unsigned long	dev)
{

HBAMSG(4, ("PDI_getminor(0x%x)\n", dev));

	return (dev & 0x3ffff);
}






EXPORT dev_t
PDI_makedevice(
	major_t	major,
	minor_t	minor)
{

HBAMSG(4, ("(PDI_makedevice)"));

	return ((major << 18) | (minor & 0x3ffff));
}






/* PDI_getrbuf() -
 *
 */

EXPORT hba_buf_t *
PDI_getrbuf(
	int		sleepflag)
{

HBAMSG(5, ("PDI_getrbuf()\n"));

	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_getrbuf: Not implemented\n");
	return (NULL);
}





EXPORT ulong_t
PDI_ind(
	int	port)
{
	return (inl(port));
}






EXPORT void
PDI_outd(
	int		port,
	ulong_t	value)
{
	outl(port, value);
}





EXPORT void
PDI_kmem_free(
	void	*addr,
	size_t	size)
{

HBAMSG(5, ("PDI_kmem_free()\n"));

	kmem_free(kernel_map, (vm_offset_t) addr, size);
}





EXPORT void *
PDI_kmem_alloc(
	size_t	size,
	int		flags)
{
	vm_offset_t		addr;
	int				rc;
	int				osf_flags = 0;

HBAMSG(5, ("(K %d)", size));

	if (flags & ~(KM_NOSLEEP | KM_SLEEP)) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_kmem_alloc: %x Not implemented\n",
			flags);
		return (NULL);
	}
	if (size == 0) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_kmem_alloc: size = 0\n");
		return (NULL);
	}

	if (flags & KM_NOSLEEP)
		osf_flags |= KMA_NOPAGEWAIT;
	rc = kernel_memory_allocate(kernel_map, &addr, (vm_size_t) size, 0,
		osf_flags);

	if (rc != KERN_SUCCESS)
			return (NULL);
	return ((void *) addr);
}






EXPORT void *
PDI_kmem_zalloc(
	size_t	size,
	int		flags)
{
	void	*addr;

HBAMSG(5, ("(KZ %d)", size));

	addr = PDI_kmem_alloc(size, flags);
	if (addr)
		bzero(addr, size);
	return (addr);
}






/* PDI_mod_drvattach() -
 *	There are 3 forms of structures that can be passed into this routine.
 *	It will only cope with the old two forms.
 */

EXPORT void
PDI_mod_drvattach(
	struct mod_drvintr	*mdp)
{
	struct bus_ctlr			bc;
	struct o_mod_drvintr	*omdp;
	struct intr_info		*iip;

HBAMSG(2, ("PDI_mod_drvattach()"));

	if (mdp->di_magic == MOD_INTR_MAGIC  &&  mdp->di_version == MOD_INTR_VER) {
		PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_mod_drvattach: New version not"
		  " Supported\n");
		return;
	}

	omdp = (struct o_mod_drvintr *) mdp;
	iip = omdp->drv_intrinfo;
	while (iip->ivect_no) {

		/* convert from HBA form to OSF form.
		 * HBA drivers require INTRNO instead of unit as an argument
		 * and OSF does not look at the unit value so it should work.
		 * NOT USED: iip->itype
		 */
		bc.intr = (intr_t) omdp->ihndler;
		bc.sysdep = (caddr_t) iip->int_pri;
		bc.sysdep1 = INTRNO(iip);
		bc.unit = INTRNO(iip);

HBAMSG(2, ("[I=%d, P=%d, H=0x%x]", bc.sysdep1, bc.sysdep, bc.intr));

		take_ctlr_irq(&bc);

		if ((iip->ivect_no & MOD_INTRVER_MASK) == MOD_INTRVER_42)
			++iip;
		else if ((iip->ivect_no & MOD_INTRVER_MASK) == 0)
			iip = (struct intr_info *) ((struct intr_info0 *) iip + 1);
		else
			break;
	}

HBAMSG(2, ("\n"));

#if HBA_DBG > 10
__asm__("int3");
#endif

	return;
}







EXPORT void
PDI_mod_drvdetach(
	struct mod_drvintr	*aip)
{

HBAMSG(2, ("PDI_mod_drvdetach()\n"));

	return;
}







EXPORT int
PDI_physiock(
	void		(*strat)(hba_buf_t *bp),
	hba_buf_t	*bp,
	dev_t		dev,
	int			rw,
	daddr_t		devsize,
	hba_uio_t	*uiop)
{

	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_physiock: Not implemented\n");
	return (HBA_EPERM);
}






EXPORT paddr_t
PDI_vtop(
	caddr_t	vaddr,
	proc_t	*procp)
{
	paddr_t	phys;

HBAMSG(10, ("PDI_vtop()"));

	phys = (paddr_t) kvtophys((vm_offset_t) vaddr);

HBAMSG(10, ("[virt=0x%x phys=0x%x]", vaddr, phys));

#if HIMEM
	/* depending on the iotype field, do fixes for DMA, "himem.c" stuff */
	if (procp  &&  (procp->iotype & F_DMA_24)  &&  high_mem_page(phys)) {
		int		len = PAGE_SIZE - (phys & PAGE_MASK);
		phys = himem_convert(phys, len, procp->rw, procp->hil);
HBAMSG(10, ("[HIMEM phys=0x%x len=%d]", phys, len));
	}
#endif
	
	return (phys);
}






EXPORT ulong_t
PDI_ptob(
	ulong_t	page)
{
	return (page << PAGE_SHIFT);
}





#ifdef SPL5_EXISTS
EXPORT int
PDI_spl5s(void)
{
	return ((int) spl5());
}

#else
EXPORT spl_t
spl5(void)
{
	return (splbio());
}


EXPORT int
PDI_spl5s(void)
{
	return ((int) splbio());
}


#endif






EXPORT caddr_t
PDI_physmap(
	paddr_t	paddr,
	ulong_t	size,
	uint_t	sleepflag)
{
	return ((caddr_t) phystokv(paddr));
}






EXPORT void
PDI_physmap_free(
	caddr_t	caddr,
	ulong_t	size,
	uint_t	sleepflag)
{
	return;
}




/* PDI_drv_gethardware() -
 *
 *	WARNING:I have hardcoded cpu to 486, step 0!!
 *			This will have to be until I work out how to determine it.
 */


EXPORT int
PDI_drv_gethardware(
	ulong_t	parm,
	void	*valuep)
{
	switch (parm) {
		case (HBA_IOBUS_TYPE): {
			ulong_t	*busp = valuep;

#if EISA
			*busp = HBA_BUS_EISA;
#else
			*busp = HBA_BUS_ISA;
#endif
			break;
		}

		case (HBA_PROC_INFO): {
			struct cpuparms	*cpp = valuep;

			cpp->cpu_id = HBA_CPU_i486;
			cpp->cpu_step = 0;
			cpp->cpu_resrvd[0] = 0;
			cpp->cpu_resrvd[1] = 0;
			break;
		}

		default:
			PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_gethardware: %d Not "
			  "implemented\n", parm);
			return (-1);
	}
	return (0);
}






EXPORT int
PDI_sleep(
	caddr_t	caddr,
	int		mask)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_sleep: Not implemented\n");
	return (0);
}






EXPORT void
PDI_wakeup(
	caddr_t	caddr)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_wakeup: Not implemented\n");
	return;
}




#ifdef UW2

EXPORT void
PDI_buf_breakup(
	void	(*strat)(buf_t *),
	buf_t	*bp,
	bcb_t	*bcbp)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_buf_breakup: Not implemented\n");
	return;
}





/*----------------------------------------------------------------*/
typedef int		rm_key_t;

typedef struct cm_args {
	rm_key_t	cm_key;
	char		*cm_param;
	void		*cm_val;
	size_t		cm_vallen;
	unit_t		cm_n;
} cm_args_t;
/*----------------------------------------------------------------*/




/* PDI_cm_getval() -
 *
 *	returns: 0 for success, else an errno value.
 */

EXPORT int
PDI_cm_getval(
	cm_args_t	*cmap)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_cm_getval: UNDER CONSTRUCTION\n");
	return (EINVAL);
}









/* PDI_cm_read_devconfig() -
 *
 *	return actual size read
 */

EXPORT int
PDI_cm_read_devconfig(
	rm_key_t	key,
	off_t		offset,
	void		*valp,
	size_t		size)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_cm_read_devconfig: UNDER "
	  "CONSTRUCTION\n");
	return (0);
}






/* PDI_dma_cascade() -
 *
 *	mode - one of DMA_SLEEP, DMA_NOSLEEP, DMA_ENABLE, DMA_DISABLE
 *
 *	returns TRUE for success, FALSE for failure
 */

EXPORT boolean_t
PDI_dma_cascade(
	int		chan,
	uchar_t	mode)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_dma_cascade: UNDER CONSTRUCTION\n");
	return (0);
}








EXPORT minor_t
PDI_geteminor(
	dev_t	dev)
{
	return (PDI_getminor(dev));
}






/*----------------------------------------------------------------*/
typedef spl_t	pl_t;
typedef int		toid_t;
/*----------------------------------------------------------------*/


/* PDI_itimeout() -
 *
 *	Returns 0 for failure, else an id for untimeout.
 */

EXPORT toid_t
PDI_itimeout(
	void	(*funcp)(),
	void	*arg,
	long	ticks,
	pl_t	pl)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_itimeout: UNDER CONSTRUCTION\n");
	return (1);
}





/*----------------------------------------------------------------*/
typedef struct physreq {
	paddr_t	phys_align;
	paddr_t	phys_boundary;
	uchar_t	phys_dmasize;
	uchar_t	phys_max_scgth;
	uchar_t	phys_flags;
	void	*phys_brkup_poolp;
};
#define PREQ_PHYSCONTIG	0x02
/*----------------------------------------------------------------*/


/* PDI_kmem_alloc_physreq() - 
 *
 *	
 *	flags : KM_SLEEP, KM_NOSLEEP
 *
 *	Returns NULL if fails, else a pointer to some virtual memory.
 */

EXPORT void *
PDI_kmem_alloc_physreq(
	size_t		size,
	physreq_t	*prp,
	int			sleepflags)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_kmem_alloc_physreq: UNDER "
	  "CONSTRUCTION\n");
	return (NULL);
}








/*----------------------------------------------------------------*/
typedef struct lkinfo_s {
	char	*lk_name;
	int		lk_flags;
	long	lk_pad [2];
} lkinfo_t;

#define LK_BASIC	0x01
#define LK_SLEEP	0x02
#define LK_NOSTATS	0x04
/*----------------------------------------------------------------*/

/* PDI_lock_alloc() -
 *
 *
 */

EXPORT lock_t *
PDI_lock_alloc(
	uchar_t		h,
	pl_t		pl,
	lkinfo_t	*lip,
	int			sleepflags)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_lock_alloc: UNDER CONSTRUCTION\n");
	return (NULL);
}








EXPORT void
PDK_lock_dealloc(
	lock_t	*lp)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_lock_dealloc: UNDER CONSTRUCTION\n");
	return;
}









EXPORT pl_t
PDI_lock_nodbg(
	lock_t	*lp,
	pl_t	pl)
{
	PDI_cmn_err(HBA_CE_WARN, "HBA: PDI_lock_nodbg: UNDER CONSTRUCTION\n");
	return ();
}
#endif	/* UW2 */

#endif	/* NHBA > 0 */
