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
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
 * This file contains the code that is specific to the Adaptec 174x
 * family of SCSI Adapters in enchanced mode.
 * Written by Dominic Herity (dherity@cs.tcd.ie)
 * Will refer to "Adaptec AHA-1740A/1742A/1744 Technical Reference Manual"
 * page x-y as TRMx-y in comments below.
 */

#include <cpus.h>
#include <platforms.h>
#include <eaha.h>
#include <eisa.h>
#if	NEAHA > 0

#include <platforms.h>		/* for now this is only a joke */
#include <mp_v1_1.h>
#include <chained_ios.h>

#include <mach/std_types.h>
#include <types.h>
#include <sys/syslog.h>
#include <kern/misc_protos.h>
#include <kern/spl.h>

#include <scsi/scsi_defs.h>

#include <scsi/adapters/scsi_aha15.h>
#include <vm/vm_kern.h>
#include <ddb/tr.h>

#ifdef	AT386
#define	MACHINE_PGBYTES		I386_PGBYTES
#define	MAPPABLE			0
#define	gimmeabreak()		__asm__("int3")


#include <i386/pio.h>		/* inlining of outb and inb */
#include <i386/AT386/mp/mp.h>
#endif	/*AT386*/

#ifdef	CBUS			
#include <busses/cbus/cbus.h>
#endif	/* CBUS	 */

#if	MP_V1_1
#include <i386/AT386/mp/mp_v1_1.h>
#endif	/* MP_V1_1 */

#ifndef	MACHINE_PGBYTES		/* cross compile check */
#define	MACHINE_PGBYTES		0x1000
#define	MAPPABLE			1
#define	gimmeabreak()		Debugger("gimmeabreak");
#endif

#include <mach_kdb.h>

#define WATCHDOG 0

#if TRACE

/*
 *	Function prototypes for conditionally-compiled TRACE routines
 */

static void	LOG(
			char		e,
			char		*f);

/*	Print AHA log					*/
void	eaha_print_log(
		int	skip);

/*	Print AHA statistics				*/
void	eaha_print_stat( void );

#define LOGSIZE 256
int eaha_logpt;
char eaha_log[LOGSIZE];

#define MAXLOG_VALUE	0x1f
struct {
	char *name;
	unsigned int count;
} logtbl[MAXLOG_VALUE];

static void
LOG(
	char	e,
	char	*f)
{
	eaha_log[eaha_logpt++] = (e);
	if (eaha_logpt == LOGSIZE) eaha_logpt = 0;
	if ((e) < MAXLOG_VALUE) {
		logtbl[(e)].name = (f);
		logtbl[(e)].count++;
	}
}

void
eaha_print_log(
	int	skip)
{
	register int i, j;
	register unsigned char c;

	for (i = 0, j = eaha_logpt; i < LOGSIZE; i++) {
		c = eaha_log[j];
		if (++j == LOGSIZE) j = 0;
		if (skip-- > 0)
			continue;
		if (c < MAXLOG_VALUE)
			printf(" %s", logtbl[c].name);
		else
			printf("-%x", c & 0x7f);
	}
	return;
}

void
eaha_print_stat( void )
{
	register int i;
	register char *p;
	for (i = 0; i < MAXLOG_VALUE; i++) {
		if (p = logtbl[i].name)
			printf("%d %s\n", logtbl[i].count, p);
	}
}

#else	/*TRACE*/
#define	LOG(e,f)
#define LOGSIZE
#endif	/*TRACE*/

#ifdef DEBUG
#define ASSERT(x) { if (!(x)) gimmeabreak() ; }
#define MARK() gimmeabreak()
#else
#define ASSERT(x)
#define MARK()
#endif

/*
 *	Notes :
 *
 * do each host command TRM6-4
 * find targets in probe
 * disable SCSI writes
 * matching port with structs, eaha_go with port, eaha_intr with port
 *
 */

/* eaha registers. See TRM4-11..23. dph */

#define HID0(z)		((z)+0xC80)
#define HID1(z)		((z)+0xC81)
#define HID2(z)		((z)+0xC82)
#define HID3(z)		((z)+0xC83)
#define EBCTRL(z)	((z)+0xC84)
#define PORTADDR(z)	((z)+0xCC0)
#define BIOSADDR(z)	((z)+0xCC1)
#define INTDEF(z)	((z)+0xCC2)
#define SCSIDEF(z)	((z)+0xCC3)
#define MBOXOUT0(z)	((z)+0xCD0)
#define MBOXOUT1(z)	((z)+0xCD1)
#define MBOXOUT2(z)	((z)+0xCD2)
#define MBOXOUT3(z)	((z)+0xCD3)
#define MBOXIN0(z)	((z)+0xCD8)
#define MBOXIN1(z)	((z)+0xCD9)
#define MBOXIN2(z)	((z)+0xCDA)
#define MBOXIN3(z)	((z)+0xCDB)
#define ATTN(z)		((z)+0xCD4)
#define G2CNTRL(z)	((z)+0xCD5)
#define G2INTST(z)	((z)+0xCD6)
#define G2STAT(z)	((z)+0xCD7)
#define G2STAT2(z)	((z)+0xCDC)

/*
 * Enhanced mode data structures: ring, enhanced ccbs, a per target buffer
 */

#define SCSI_TARGETS 8	/* Allow for SCSI-2 */


/* Extended Command Control Block Format. See TRM6-3..12. */

typedef struct {
	unsigned short command ;
#		define EAHA_CMD_NOP 0
#		define EAHA_CMD_INIT_CMD 1
#		define EAHA_CMD_DIAG 5
#		define EAHA_CMD_INIT_SCSI 6
#		define EAHA_CMD_READ_SENS 8
#		define EAHA_CMD_DOWNLOAD 9
#		define EAHA_CMD_HOST_INQ 0x0a
#		define EAHA_CMD_TARG_CMD 0x10

	/*
	 * It appears to be customary to tackle the endian-ness of
	 * bit fields as follows, so I won't deviate. However, nothing in
	 * K&R implies that bit fields are implemented so that the fields
	 * of an unsigned char are allocated lsb first. Indeed, K&R _warns_
	 * _against_ using bit fields to describe storage allocation.
	 * This issue is separate from endian-ness. dph
	 */
	BITFIELD_3(unsigned char,
		cne:1,
		xxx0:6,
		di:1) ;
	BITFIELD_7(unsigned char,
		xxx1:2,
		ses:1,
		xxx2:1,
		sg:1,
		xxx3:1,
		dsb:1,
		ars:1) ;
		
	BITFIELD_5(unsigned char,
		lun:3,
		tag:1,
		tt:2,
		nd:1,
		xxx4:1) ;
	BITFIELD_7(unsigned char,
		dat:1,
		dir:1,
		st:1,
		chk:1,
		xxx5:2,
		rec:1,
		nrb:1) ;

	unsigned short xxx6 ;

	caddr_t scather ; /* scatter/gather */
	unsigned scathlen ;
	caddr_t status ;
	caddr_t chain ;
	int xxx7 ;

	caddr_t sense_p ;
	unsigned char sense_len ;
	unsigned char cdb_len ;
	unsigned short checksum ;
	scsi_command_group_5 cdb ;
	unsigned char buffer[256] ; /* space for data returned. */

} eccb ;

#define NTARGETS (8)
#define NECCBS 74	/*
			 * Only 64 can ever be used at once, but the way
			 * we preallocate these with targets requires over
			 * doing it a bit
			 */
			 

typedef struct {	/* Status Block Format. See TRM6-13..19. */
	BITFIELD_8(unsigned char,
		don:1,
		du:1,
		xxx0:1,
		qf:1,
		sc:1,
		dover:1,
		ch:1,
		inti:1) ;
	BITFIELD_8(unsigned char,
		asa:1, /* Error in TRM6-15..16 says both asa and sns */
		sns:1,	 /* bit 9. Bits 8 and 10 are not mentioned. */
		xxx1:1,
		ini:1,
		me:1,
		xxx2:1,
		eca:1,
		xxx3:1) ;

	unsigned char ha_status ;
#		define HA_STATUS_SUCCESS 0x00
#		define HA_STATUS_HOST_ABORTED 0x04
#		define HA_STATUS_ADP_ABORTED 0x05
#		define HA_STATUS_NO_FIRM 0x08
#		define HA_STATUS_NOT_TARGET 0x0a
#		define HA_STATUS_SEL_TIMEOUT 0x11
#		define HA_STATUS_OVRUN 0x12
#		define HA_STATUS_BUS_FREE 0x13
#		define HA_STATUS_PHASE_ERROR 0x14
#		define HA_STATUS_BAD_OPCODE 0x16
#		define HA_STATUS_INVALID_LINK 0x17
#		define HA_STATUS_BAD_CBLOCK 0x18
#		define HA_STATUS_DUP_CBLOCK 0x19
#		define HA_STATUS_BAD_SCATHER 0x1a
#		define HA_STATUS_RSENSE_FAIL 0x1b
#		define HA_STATUS_TAG_REJECT 0x1c
#		define HA_STATUS_HARD_ERROR 0x20
#		define HA_STATUS_TARGET_NOATTN 0x21
#		define HA_STATUS_HOST_RESET 0x22
#		define HA_STATUS_OTHER_RESET 0x23
#		define HA_STATUS_PROG_BAD_SUM 0x80

	scsi2_status_byte_t	target_status ;

	unsigned residue ;
	caddr_t residue_buffer ;
	unsigned short add_stat_len ;
	unsigned char sense_len ;
	char xxx4[9] ;
	unsigned char cdb[6] ;

} status_block ;

typedef struct {
	caddr_t ptr ;
	unsigned len ;
} scather_entry ;

#define SCATHER_ENTRIES 128	/* TRM 6-11 */

struct erccbx {
	target_info_t	*active_target;
	char		*dma_ptr;
	struct erccbx	*next ;
	struct eaha_struct *eaha;
	status_block	status ;
	eccb		_eccb;
} ;

typedef struct erccbx erccb ;

struct eaha_host_inquiry_data {
	BITFIELD_4(unsigned char,
		 xxx0:5,
		 tmd:1,
		 tms:1,
		 xxx1:1) ;
	unsigned char xx2;
	unsigned char xx3;
	BITFIELD_2(unsigned char,
		 xxx4:7,
		 aen:1) ;
	unsigned char add_len;
	unsigned char no_target_luns;
	unsigned char no_cbs;
	BITFIELD_6(unsigned char,
		 xxx5:2,
		 dif:1,
		 lnk:1,
		 syn:1,
		 wid:1,
		 xxx6:2) ;
	char vendor_id[8];
	char product_id[8];
	char firmware_type[8];
	char firmware_revision[4];
	char release_date[8];
	char release_time[8];
	char xxx7[206];
};


#define DOUBLEI 1
/* #define MAX_TRANSFER 64*1024 */

int	eaha_sg_max_size = (1024 * 128);

/*
 * State descriptor for this layer.  There is one such structure
 * per (enabled) board
 */
typedef struct eaha_struct {
	struct watchdog_t	wd;
	decl_simple_lock_data(,aha_lock)
	int		port;		/* I/O port */

	int		unit;
	struct {
		unsigned int
		  has_sense_info:1,
#if	DOUBLEI
		  selected:1,
		    /* Since the board hangs for target mode on
		     * any LUN except 0, no need to keep more info
		     * then this
		     */
		  initialized:1,
#endif
		  sense_info_lun:3;
			/* 1742 enhanced mode will hang if target has
			 * sense info and host doesn't request it (TRM6-34).
			 * This sometimes happens in the scsi driver.
			 * These flags indicate when a target has sense
			 * info to disgorge.
			 * If set, eaha_go reads and discards sense info 
			 * before running any command except request sense.
			 * dph
			 */
#if	DOUBLEI
		erccb		*p_tm_r;
#endif	
	} ti[NTARGETS];

	scsi_softc_t	*sc;		/* HBA-indep info */

	erccb		_erccbs[NECCBS] ;	/* mailboxes */
	erccb		*toperccb ;

	/* This chicanery is for mapping back the phys address
	   of a CCB (which we get in an MBI) to its virtual */
	/* [we could use phystokv(), but it isn't standard] */
	vm_offset_t	I_hold_my_phys_address;

	struct eaha_host_inquiry_data host_inquiry_data;

} eaha_softc ;

eaha_softc eaha_softc_data[NEAHA];

typedef eaha_softc *eaha_softc_t;

eaha_softc_t	eaha_softc_pool[NEAHA];

int eaha_quiet ;

/*
 *	Function prototypes for internal routines
 */

/*	Allocate AHA target dynamic segment lists	*/
void	eaha_alloc_segment_list(
		char		**ptr);

/*	Reset controller bus				*/
void	eaha_bus_reset(
		eaha_softc_t     eaha);

/*	Start a command					*/
void	eaha_command(
		int	port,
		erccb	*_erccb);

/*	Start a SCSI command (enhanced mode)		*/
void	eaha_go(
		target_info_t	*tgt,
		unsigned int	cmd_count,
		unsigned int	in_count,
		boolean_t	cmd_only);

/*	Initialize controller (nugatory)		*/
void	eaha_init(
		eaha_softc_t	eaha,
		unsigned char	*my_scsi_id,
		unsigned char	*my_interrupt);

void
eaha_intr_common(
		 int			unit,
		 boolean_t		check);

/*	EAHA initiator interrupt routine		*/
void	eaha_initiator_intr(
		eaha_softc_t	eaha,
		erccb		*_erccb);

/*	Convert mailbox address				*/
void	eaha_mboxout(
		int		port,
		unsigned int	phys);

/*	Prepare Raw Command Control Block		*/
void	eaha_prepare_rccb(
		target_info_t	*tgt,
		erccb		*_erccb,
		vm_offset_t	virt,
		vm_size_t	len,
		boolean_t	sg,
		boolean_t	passive);

/*	Probe Host Adapter Board			*/
int	eaha_probe(
		int		port,
		struct bus_ctlr	*ui);

/*	Probe target device				*/
boolean_t eaha_probe_target(
		target_info_t	*tgt,
		io_req_t	ior);

/*	Reset controller and wait until done		*/
void	eaha_reset(
		eaha_softc_t	eaha,
		boolean_t	quick);

/*	Reset SCSI bus					*/
void	eaha_reset_scsibus(
		eaha_softc_t     eaha);

int	eaha_sg(
		scather_entry		**sgl,
		vm_offset_t		virt,
		vm_size_t		len);

boolean_t	eaha_host_adapter_inquiry(eaha_softc_t	eaha);

void	eaha_print_host_adapter_inquiry(eaha_softc_t eaha);

void 	eaha_print_active(eaha_softc_t eaha);

void	eaha_scsibus_was_reset(eaha_softc_t	eaha);

/*	Retrieve sense info from controller		*/
int	eaha_retrieve_sense_info(
                eaha_softc_t	eaha,
	        unsigned char	tid,
	        unsigned char	lun);

/*	EAHA target interrupt routine			*/
void	eaha_target_intr(
		eaha_softc_t	eaha,
		unsigned int	mbi,
		unsigned char	peer);

/*	Allocate EAHA target_info_t			*/
target_info_t * eaha_tgt_alloc(
			eaha_softc_t	eaha,
			unsigned char	id,
			target_info_t	*tgt);

/*	Set up target					*/
void	eaha_tgt_setup(
		target_info_t	*tgt);

/*	Allocate Raw Command Control Block		*/
erccb *	erccb_alloc(
		eaha_softc	*eaha);

/*	Free Raw Command Control Block			*/
void	erccb_free(
		eaha_softc	*eaha,
		erccb		*e);

/*
 *	End of function prototypes
 */


caddr_t	eaha_std[NEAHA] = { 0 };
struct	bus_device *eaha_dinfo[NEAHA*8];
struct	bus_ctlr *eaha_minfo[NEAHA];
struct	bus_driver eaha_driver = {
        ((int (*)(			/* Test for driver		*/
		caddr_t		port,
		void		*ui))eaha_probe),
	((int (*)(			/* Test for slave		*/
		struct bus_device *device,
		caddr_t		virt))scsi_slave),
	((void (*)(			/* Setup driver after probe	*/
		struct bus_device *device))scsi_attach),
	((int (*)(void))eaha_go),	/* Start transfer		*/
	eaha_std,			/* Device CSR addresses		*/
	"rz",				/* Device name			*/
	eaha_dinfo,			/* Backpointer to init structs	*/
	"eahac",			/* Controller name		*/
	eaha_minfo,			/* Backpointer to init structs	*/
	BUS_INTR_B4_PROBE};		/* Flags			*/


erccb *erccb_alloc(
	eaha_softc	*eaha)
{
	erccb *e ;
	spl_t x ;
	char *dma_ptr;

	do {
		while (eaha->toperccb == 0) ;/* Shouldn't be often or long, */
						/* BUT should use a semaphore */
		x = splbio() ;
		e = eaha->toperccb ;
		if (e == 0)
			splx(x) ;
	} while (!e) ;
	eaha->toperccb = e->next ;
	splx(x) ;
	dma_ptr = e->dma_ptr;
	/* bzero requires (char *) for its generic 1st argument */
	bzero((char *)e,sizeof(*e)) ;
	/* kvtophys requires type-casts to/from vm_offset_t */
	e->_eccb.status = (caddr_t) kvtophys((vm_offset_t)&e->status) ;
	e->dma_ptr = dma_ptr;
	e->eaha = eaha;
	return e ;
}

void
erccb_free(
	eaha_softc	*eaha,
	erccb		*e)
{
	spl_t	x;
	ASSERT ( e >= eaha->_erccbs && e < eaha->_erccbs+NECCBS) ;
	x = splbio() ;
	e->next = eaha->toperccb ;
	eaha->toperccb = e ;
	splx(x) ;
}

void
eaha_mboxout(
	int		port,
	unsigned int	phys)
{
        outb(MBOXOUT0(port),phys) ;
        outb(MBOXOUT1(port),phys>>8) ;
        outb(MBOXOUT2(port),phys>>16) ;
        outb(MBOXOUT3(port),phys>>24) ;
}

void
eaha_command(		/* start a command */
	int	port,
	erccb	*_erccb)
{
	spl_t s ;
	unsigned char g2stat;
	/* kvtophys requires type-casting to/from vm_offset_t */
	unsigned phys = (unsigned) kvtophys((vm_offset_t)&_erccb->_eccb) ;

	while ((g2stat = inb(G2STAT(port)) & 0x04)==0)/*While MBO busy.TRM6-1*/
	    if (g2stat & 0x2)
		eaha_intr(_erccb->eaha->unit);
	s = splbio() ;
	eaha_mboxout(port,phys) ;
	while (inb(G2STAT(port)) & 1) ; 	/* While adapter busy. TRM6-2 */
	outb(ATTN(port),0x40 | _erccb->active_target->target_id) ; /* TRM6-20 */
			/* (Should use target id for intitiator command) */
	splx(s) ;
}

void
eaha_reset(
	eaha_softc_t	eaha,
	boolean_t	quick)
{
	/*
	 * Reset board and wait till done
	 */
	unsigned st ;
	int target_id ;
	int port = eaha->port ;

	outb(ATTN(port), 0x10 | inb(SCSIDEF(port)) & 0x0f) ;
	outb(G2CNTRL(port),0x20) ;	/* TRM 4-22 */

	do {
		st = inb(G2INTST(port)) >> 4 ;
	} while (st == 0) ;
	/* TRM 4-22 implies that 1 should not be returned in G2INTST, but
	   in practise, it is. So this code takes 0 to mean non-completion. */

	for (target_id = 0 ; target_id < NTARGETS; target_id++)
		eaha->ti[target_id].has_sense_info = FALSE ;

}

void
eaha_init(
	eaha_softc_t	eaha,
	unsigned char	*my_scsi_id,
	unsigned char	*my_interrupt)
{
	int port = eaha->port;

	/* Issue RESET in case this is a reboot */

	outb(EBCTRL(port),0x04) ; /* Disable board. TRM4-12 */
	outb(PORTADDR(port),0x80) ; /* Disable standard mode ports. TRM4-13. */
	*my_interrupt = inb(INTDEF(port)) & 0x07 ;
	outb(INTDEF(port), *my_interrupt | 0x00) ;
					/* Disable interrupts. TRM4-15 */
	*my_scsi_id = inb(SCSIDEF(port)) & 0x0f ;
	outb(G2CNTRL(port),0xc0) ; /* Reset board, clear interrupt */
#if 0
				    /* and set 'host ready'. */
	delay(10*10) ;		/* HRST must remain set for 10us. TRM4-22 */
			/* (I don't believe the delay loop is slow enough.) */
	outb(G2CNTRL(port),0x60);/*Un-reset board, set 'host ready'. TRM4-22*/
#endif
}


int eaha_immediate_command_complete_expected = 0;

void
eaha_bus_reset(
	eaha_softc_t     eaha)

{
	unsigned char my_scsi_id, my_interrupt ;

	LOG(0x1d,"bus_reset");

	/*
	 * Clear bus descriptor
	 */
	eaha_immediate_command_complete_expected++;
#if	WATCHDOG
	eaha->wd.nactive = 0;
	eaha->wd.watchdog_state = SCSI_WD_INACTIVE;
#endif
	eaha_reset(eaha, TRUE);
	eaha_init(eaha, &my_scsi_id, &my_interrupt);

	printf("eaha: (%d) bus reset ", ++eaha->wd.reset_count);
	delay(scsi_delay_after_reset); /* some targets take long to reset */

	if (eaha->sc == 0)	/* sanity */
		return;

	scsi_bus_was_reset(eaha->sc);
}

boolean_t
eaha_host_adapter_inquiry(eaha_softc_t	eaha)
{
	int tid;
	target_info_t dummy_target ;	/* Keeps eaha_command() happy. HACK */
	erccb *_erccb = erccb_alloc(eaha) ;

	_erccb->active_target = &dummy_target ;
	dummy_target.target_id = tid ;
	
	_erccb->active_target = eaha->sc->target[eaha->sc->initiator_id];
	tid = _erccb->active_target->target_id;
	_erccb->_eccb.scather = (caddr_t) kvtophys((vm_offset_t)&eaha->host_inquiry_data) ;
	_erccb->_eccb.scathlen = sizeof(eaha->host_inquiry_data) ;
	_erccb->_eccb.lun = 0;
	_erccb->_eccb.ses = 1 ;
	_erccb->_eccb.command = EAHA_CMD_HOST_INQ ;
	eaha_command(eaha->port, _erccb);
	while ((inb(G2STAT(eaha->port)) & 0x02) == 0) ;
	outb(G2CNTRL(eaha->port),0x40);/* Clear int */
	if (_erccb->status.target_status.bits) {
		erccb_free(eaha, _erccb);
		return FALSE;
	} else {
		erccb_free(eaha, _erccb);
		return TRUE;
	}
}

void
eaha_print_host_adapter_inquiry(eaha_softc_t eaha)
{
	int s = splbio();
	if (!eaha_host_adapter_inquiry(eaha)){
		splx(s) ;
		printf("\nFailed to get host adapter data n");
	} else {
		splx(s) ;
		printf("\n%8s %8s %8s %4s %8s %8s\n",
		       eaha->host_inquiry_data.vendor_id,
		       eaha->host_inquiry_data.product_id,
		       eaha->host_inquiry_data.firmware_type,
		       eaha->host_inquiry_data.firmware_revision,
		       eaha->host_inquiry_data.release_date,
		       eaha->host_inquiry_data.release_time);
		printf("%s Asynch Events\n",
		       (eaha->host_inquiry_data.aen?
			"Accepting":"Rejecting"));
		printf("Target Mode %s and %s",
		       (eaha->host_inquiry_data.tms?
			"Unsupported":"Supported"),
		       (eaha->host_inquiry_data.tmd?
			"Disabled":"Enabled"));
		if (eaha->host_inquiry_data.tmd)
		    printf("\n");
		else
		    printf(" with %d LUNs\n",
			   eaha->host_inquiry_data.no_target_luns);
		printf("%d CBs can be buffered\n",
		       eaha->host_inquiry_data.no_cbs);
	}

}

#ifdef notdef
	/* functions added to complete 1742 support, but not used. Untested. */

	void eaha_download(port, data, len)
	int port ;
	char *data ;
	unsigned len ;
	{
		/* 1744 firmware download. Not implemented. TRM6-21 */
	}

	void eaha_initscsi(data, len)
	char *data ;
	unsigned len ;
	{
		/* initialize SCSI subsystem. Presume BIOS does it.
		   Not implemented. TRM6-23 */
	}

	void eaha_noop()
	{
		/* Not implemented. TRM6-27 */
	}

	erccb *eaha_read_sense_info(eaha, target, lun) /* TRM 6-33..35 */
	eaha_softc *eaha ;
	unsigned target, lun ;
	{	/* Don't think we need this because its done in scsi_alldevs.c */
	#ifdef notdef
		erccb *_erccb = erccb_alloc(eaha) ;
		_erccb->_eccb.command = EAHA_CMD_READ_SENS ;
		_erccb->_eccb.lun = lun ;
		eaha_command(eaha->port,_erccb->_eccb, target) ;/*Wrong # args*/
		return _erccb ;
	#else
		return 0 ;
	#endif
	}

	void eaha_diagnostic(eaha)
	eaha_softc *eaha ;
	{
		/* Not implemented. TRM6-36..37 */
	}

	erccb *eaha_target_cmd(eaha, target, lun, data, len) /* TRM6-38..39 */
	eaha_softc *eaha ;
	unsigned target, lun ;
	char *data ;
	unsigned len ;
	{
		erccb *_erccb = erccb_alloc(eaha) ;
		_erccb->_eccb.command = EAHA_CMD_TARG_CMD ;
		_erccb->_eccb.lun = lun ;
		eaha_command(eaha->port,_erccb->_eccb,target);/*Wrong # args*/
		return _erccb ;
	}

	erccb *eaha_init_cmd(port) /* SHOULD RETURN TOKEN. i.e. ptr to eccb */
				 /* Need list of free eccbs */
	{ /* to be continued,. possibly. */
	}

#endif /* notdef */

target_info_t *
eaha_tgt_alloc(
	eaha_softc_t	eaha,
	unsigned char	id,
				/* Took out sense length param dph */
	target_info_t	*tgt)
{
	erccb		*_erccb;

	if (tgt == 0)
		/* scsi_slave_alloc expects (char *) for generic 3rd arg */
		tgt = scsi_slave_alloc(eaha->sc->masterno, id, (char *)eaha);

	tgt->flags |= TGT_CHAINED_IO_SUPPORT;
	_erccb = erccb_alloc(eaha) ; /* This is very dodgy */
	tgt->cmd_ptr =  (char *)& _erccb->_eccb.cdb ;
	tgt->dma_ptr = _erccb->dma_ptr;
	return tgt;
}

void
eaha_tgt_setup(tgt)
	target_info_t	*tgt;
{
	erccb *_erccb;
	eaha_softc_t eaha = (eaha_softc_t)tgt->hw_state;
	_erccb = erccb_alloc(eaha) ; /* This is very dodgy */
	tgt->cmd_ptr =  (char *)& _erccb->_eccb.cdb;
	tgt->dma_ptr = _erccb->dma_ptr;
}


struct {
    scsi_sense_data_t sns ;
    unsigned char extra
	[254-sizeof(scsi_sense_data_t)] ;
} eaha_xsns [NTARGETS] ;/*must be bss to be contiguous*/


/* Enhanced adapter probe routine */

int
eaha_probe(
	register int		port,
	struct bus_ctlr		*ui)
{
	int unit = ui->unit;
	eaha_softc_t eaha = &eaha_softc_data[unit] ;
	int target_id ;
	scsi_softc_t *sc ;
	spl_t		s;
	boolean_t did_banner = FALSE ;
        struct aha_devs installed;
	erccb * _erccb;
	target_info_t *self;
	unsigned char my_scsi_id, my_interrupt ;
	int retry_count=1000;

	if (unit >= NEAHA)
		return(0);

	/* No interrupts yet */
	s = splbio();

	/*
	 * Detect prescence of 174x in enhanced mode. Ignore HID2 and HID3
	 * on the assumption that compatibility will be preserved. dph
	 */
	if (inb(HID0(port)) != 0x04 || inb(HID1(port)) != 0x90 ||
	    (inb(PORTADDR(port)) & 0x80) != 0x80) {
	  	splx(s);
		return 0 ;
	}

	eaha_softc_pool[unit] = eaha ;
	bzero((char *)eaha,sizeof(*eaha)) ;
	eaha->port = port ;
	eaha->unit = unit;

	eaha_init(eaha, &my_scsi_id, &my_interrupt);

	printf("Adaptec 1740A/1742A/1744 enhanced mode\n");
	
	/* Get host inquiry data */

	/* scsi_master_alloc expects (char *) for generic 2nd argument */
	sc = scsi_master_alloc(unit, (char *)eaha) ;
	eaha->sc = sc ;

	simple_lock_init(&eaha->aha_lock, ETAP_IO_EAHA);

	sc->go = eaha_go ;
#if	WATCHDOG
	sc->watchdog = scsi_watchdog ;
	nactive accounting is wrong with target mode
#else
	sc->watchdog = 0 ;
#endif
	sc->probe = eaha_probe_target ;
/*	sc->tgt_setup = eaha_tgt_setup ;*/
	sc->tgt_setup = (void (*)(target_info_t *))0;
	/*
	 * The controller-specific structure that is the argument to
	 * the reset call must start with struct watchdog_t.
	 * In assigning the wd.reset pointer, the controller-specific
	 * reset routine must be type-cast to the generic form.
	 */
	eaha->wd.reset = ((void (*)(
			struct watchdog_t	*wd))eaha_reset_scsibus);
	sc->max_dma_data = eaha_sg_max_size; 
	sc->max_dma_segs = SCATHER_ENTRIES; 
	sc->initiator_id = my_scsi_id ;
	eaha_reset(eaha,TRUE) ;
	eaha->I_hold_my_phys_address =
		/* kvtophys requires type-cast to vm_offset_t */
		kvtophys((vm_offset_t)&eaha->I_hold_my_phys_address) ;
	{
		erccb *e ;	
		eaha->toperccb = eaha->_erccbs ;
		for (e=eaha->_erccbs; e < eaha->_erccbs+NECCBS; e++) {
			e->active_target = (target_info_t *)0;
			e->eaha = eaha;
			e->next = e+1 ;
			e->_eccb.status =
			/* kvtophys requires type-casts to/from vm_offset_t */
				(caddr_t) kvtophys((vm_offset_t)&(e->status)) ;
			eaha_alloc_segment_list(&(e->dma_ptr));
		}
		eaha->_erccbs[NECCBS-1].next = 0 ;

	}

	ui->sysdep1 = my_interrupt + 9 ;
	take_ctlr_irq(ui) ;

	printf("%s%d: [port 0x%x intr ch %d] my SCSI id is %d",
		ui->name, unit, port, my_interrupt + 9, my_scsi_id) ;

	outb(INTDEF(port), my_interrupt | 0x10) ;
					/* Enable interrupts. TRM4-15 */
	outb(EBCTRL(port),0x01) ; /* Enable board. TRM4-12 */

	self = eaha_tgt_alloc(eaha, my_scsi_id, 0) ;
	sccpu_new_initiator(self, self);
	
	eaha_print_host_adapter_inquiry(eaha);

	/* Find targets, incl. ourselves. */

	for (target_id=0; target_id < SCSI_TARGETS; target_id++) {
		eaha->ti[target_id].has_sense_info = FALSE;
#if	DOUBLEI		
		eaha->ti[target_id].selected = FALSE;
		eaha->ti[target_id].initialized = FALSE;
		eaha->ti[target_id].p_tm_r = (erccb *)0;
#endif
		if (target_id != sc->initiator_id) {
		        scsi_cmd_test_unit_ready_t      *cmd;
			erccb *_erccb = erccb_alloc(eaha) ;
			unsigned attempts = 0 ;
#define MAX_ATTEMPTS	2
			target_info_t temp_targ ;

			temp_targ.ior = 0 ;
			temp_targ.hw_state = (char *) eaha ;
			temp_targ.cmd_ptr = (char *) &_erccb->_eccb.cdb ;
			temp_targ.target_id = target_id ;
			temp_targ.lun = 0 ;
			temp_targ.cur_cmd = SCSI_CMD_TEST_UNIT_READY;

			cmd = (scsi_cmd_test_unit_ready_t *) temp_targ.cmd_ptr;

			do {
				cmd->scsi_cmd_code = SCSI_CMD_TEST_UNIT_READY;
				cmd->scsi_cmd_lun_and_lba1 = 0; /*assume 1 lun?*/
				cmd->scsi_cmd_lba2 = 0;
				cmd->scsi_cmd_lba3 = 0;
				cmd->scsi_cmd_ss_flags = 0;
				cmd->scsi_cmd_ctrl_byte = 0;    /* not linked */

				eaha_go( &temp_targ,
					sizeof(scsi_cmd_test_unit_ready_t),0,0);
				/* ints disabled, so call isr yourself. */
				while (temp_targ.done == SCSI_RET_IN_PROGRESS)
					if (inb(G2STAT(eaha->port)) & 0x02) {
						eaha_quiet = 1 ;
						eaha_intr(unit) ;
						eaha_quiet = 0 ;
					}
                                if (temp_targ.done == SCSI_RET_NEED_SENSE) {
                                        /* MUST get sense info : TRM6-34 */
					if (eaha_retrieve_sense_info(
						eaha, temp_targ.target_id,
						temp_targ.lun) &&
						attempts == MAX_ATTEMPTS-1) {

						printf(
						"\nTarget %d Check Condition : "
							,temp_targ.target_id) ;
						scsi_print_sense_data(
							&eaha_xsns
							[temp_targ.target_id]
							.sns);
						printf("\n") ;
					}
                                }
			} while (temp_targ.done != SCSI_RET_SUCCESS &&
				!(temp_targ.done & SCSI_RET_ABORTED) &&
				++attempts < MAX_ATTEMPTS) ;

			/*
			 * Recognize target which is present, whether or not
			 * it is ready, e.g. drive with removable media.
			 */
			if (temp_targ.done == SCSI_RET_SUCCESS ||
				temp_targ.done == SCSI_RET_NEED_SENSE &&
				_erccb->status.target_status.bits != 0) {
			    /* Eureka */
			    installed.tgt_luns[target_id]=1;/*Assume 1 lun?*/
			    printf(", %s%d",
				did_banner++ ? "" : "target(s) at ",
				target_id);

			    erccb_free(eaha, _erccb) ;

			    /* Normally, only LUN 0 */
			    if (installed.tgt_luns[target_id] != 1)
			    	printf("(%x)", installed.tgt_luns[target_id]);
			    /*
			     * Found a target
			     */
			    (void) eaha_tgt_alloc(eaha, target_id, 0);
				/* Why discard ? */
			} else
			    installed.tgt_luns[target_id]=0;
		}
	}

	printf(".\n") ;
	splx(s);
	return 1 ;
}

int
eaha_retrieve_sense_info(
        eaha_softc_t	eaha,
	unsigned char	tid,
	unsigned char	lun)
{
	int result ;
	spl_t s ;
	caddr_t			mbi ;
	target_info_t dummy_target ;	/* Keeps eaha_command() happy. HACK */
	erccb *_erccb1 = erccb_alloc(eaha) ;

	_erccb1->active_target = &dummy_target ;
	dummy_target.target_id = tid ;
	_erccb1->_eccb.command =
		EAHA_CMD_READ_SENS ;
	_erccb1->_eccb.lun = lun ;
	_erccb1->_eccb.sense_p =
		/* kvtophys requires type-casts to/from vm_offset_t */
		(caddr_t) kvtophys((vm_offset_t)&eaha_xsns [tid]);
	_erccb1->_eccb.sense_len = sizeof(eaha_xsns [tid]);
	_erccb1->_eccb.ses = 1 ;
	s = splbio() ;
	eaha_command(eaha->port,_erccb1) ;
      repoll:
	while ((inb(G2STAT(eaha->port)) & 0x02) == 0) ;
	mbi = (caddr_t) (inb(MBOXIN0(eaha->port)) +
			 (inb(MBOXIN1(eaha->port))<<8) +
			 (inb(MBOXIN2(eaha->port))<<16) +
			 (inb(MBOXIN3(eaha->port))<<24)) ;
	if (_erccb1 != 
	    (erccb *)( ((vm_offset_t)&eaha->I_hold_my_phys_address) +
		      (mbi - eaha->I_hold_my_phys_address) -
		      (vm_offset_t)&(((erccb *)0)->_eccb))) {
		eaha_intr_common(eaha->unit, FALSE);
		goto repoll;
	}
	outb(G2CNTRL(eaha->port),0x40);/* Clear int */
	splx(s) ;
	result = _erccb1->status.target_status.bits != 0 ;
	_erccb1->active_target = 0;
	erccb_free(eaha,_erccb1) ;
	return result ;
}

/*
 * Start a SCSI command on a target (enhanced mode)
 */
void
eaha_go(
	target_info_t		*tgt,
	unsigned int		cmd_count,
	unsigned int		in_count,
	boolean_t		cmd_only)
{
	eaha_softc_t		eaha;
	spl_t			s;
	erccb			*_erccb;
	int			len;
	vm_offset_t		virt;
	int tid = tgt->target_id ;
	boolean_t	passive = (tgt->ior && (tgt->ior->io_op & IO_PASSIVE));
	boolean_t	sg = (tgt->ior && (tgt->ior->io_op & IO_SCATTER));

	at386_io_lock_state();

	LOG(1,"go");

	at386_io_lock(MP_DEV_WAIT);
	eaha = (eaha_softc_t)tgt->hw_state;

	if(eaha->ti[tid].has_sense_info) {
		(void) eaha_retrieve_sense_info
			(eaha, tid, eaha->ti[tid].sense_info_lun) ;
		eaha->ti[tid].has_sense_info = FALSE ;
		if (tgt->cur_cmd == SCSI_CMD_REQUEST_SENSE) {
			/* bcopy requires type-cast to (char *) */
			bcopy((char *)&eaha_xsns[tid],tgt->cmd_ptr,in_count) ;
			tgt->done = SCSI_RET_SUCCESS;
			tgt->transient_state.cmd_count = cmd_count;
			tgt->transient_state.out_count = 0;
			tgt->transient_state.in_count = in_count;
			/* Fake up interrupt */
			/* Highlights from eaha_initiator_intr(), */
			/* ignoring errors */
			if (tgt->ior)
				(*tgt->dev_ops->restart)( tgt, TRUE);
			at386_io_unlock();
			return ;
		}
	}

/* XXX delay the handling of the ccb till later */
	_erccb = (erccb *)
		((unsigned)tgt->cmd_ptr - (unsigned) &((erccb *) 0)->_eccb.cdb);
	/* Tell *rccb about target, eg. id ? */
	_erccb->active_target = tgt;

	/*
	 * We can do real DMA.
	 */
/*	tgt->transient_state.copy_count = 0;	unused */
/*	tgt->transient_state.dma_offset = 0;	unused */

	tgt->transient_state.cmd_count = cmd_count;

	if ((((tgt->cur_cmd == SCSI_CMD_WRITE) ||
	      (tgt->cur_cmd == SCSI_CMD_LONG_WRITE)) && !passive) ||
	    ((tgt->cur_cmd == SCSI_CMD_RECEIVE) && passive)) {
		io_req_t	ior = tgt->ior;
		register int	len = ior->io_count;

		tgt->transient_state.out_count = len;

	} else {
		tgt->transient_state.out_count = 0;
	}

	/* See above for in_count < block_size */
	tgt->transient_state.in_count = in_count;

	/*
	 * Setup CCB state
	 */
	tgt->done = SCSI_RET_IN_PROGRESS;

	switch (tgt->cur_cmd) {
	    case SCSI_CMD_READ:
	    case SCSI_CMD_LONG_READ:
		LOG(9,"readop");
		virt = (vm_offset_t)tgt->ior->io_data;
		if (passive)
		    len = tgt->transient_state.out_count;
		else
		    len = tgt->transient_state.in_count;
		break;
		break;
	    case SCSI_CMD_WRITE:
	    case SCSI_CMD_LONG_WRITE:
		LOG(0x1a,"writeop");
		virt = (vm_offset_t)tgt->ior->io_data;
		if (passive)
		    len = tgt->transient_state.in_count;
		else
		    len = tgt->transient_state.out_count;
		break;
		break;
	    case SCSI_CMD_INQUIRY:
	    case SCSI_CMD_REQUEST_SENSE:
	    case SCSI_CMD_MODE_SENSE:
	    case SCSI_CMD_RECEIVE_DIAG_RESULTS:
	    case SCSI_CMD_READ_CAPACITY:
	    case SCSI_CMD_READ_BLOCK_LIMITS:
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		virt = (vm_offset_t)tgt->cmd_ptr;
		len = tgt->transient_state.in_count;
		break;
	    case SCSI_CMD_MODE_SELECT:
	    case SCSI_CMD_REASSIGN_BLOCKS:
	    case SCSI_CMD_FORMAT_UNIT:
		tgt->transient_state.cmd_count = sizeof(scsi_command_group_0);
		len =
		tgt->transient_state.out_count = cmd_count
					- sizeof(scsi_command_group_0);
		virt = (vm_offset_t)tgt->cmd_ptr+sizeof(scsi_command_group_0);
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		break;
	    default:
		LOG(0x1c,"cmdop");
		LOG(0x80+tgt->cur_cmd,0);
		virt = 0;
		len = 0;
	}

	eaha_prepare_rccb(tgt, _erccb, virt, len, sg, passive);

	_erccb->_eccb.lun = tgt->lun;
	_erccb->_eccb.sense_p = (caddr_t) kvtophys((vm_offset_t)&eaha_xsns [tid]);
	_erccb->_eccb.sense_len = sizeof(eaha_xsns [tid]);

	if (passive) {
		_erccb->_eccb.command = EAHA_CMD_TARG_CMD;
		_erccb->_eccb.dir = (tgt->cur_cmd == SCSI_CMD_SEND) ? 1 : 0 ;
		_erccb->_eccb.dat = 1;
		s = splbio();
#if	DOUBLEI
		if (!eaha->ti[tid].selected && eaha->ti[tid].initialized) {
			assert(eaha->ti[tid].p_tm_r == (erccb *)0);
			eaha->ti[tid].p_tm_r =_erccb;
		} else {
			eaha->ti[tid].initialized = TRUE;
			eaha->ti[tid].selected = FALSE;
#else
		{
#endif
			LOG(0x1e,"enqueue-passive");
			eaha_command(eaha->port, _erccb) ;
		}
		splx(s);
	} else {
		_erccb->_eccb.ars = 1;

		/*
		 * XXX here and everywhere, locks!
		 */
		s = splbio();

#if	WATCHDOG
		simple_lock(&eaha->aha_lock);
		if (eaha->wd.nactive++ == 0)
		    eaha->wd.watchdog_state = SCSI_WD_ACTIVE;
		simple_unlock(&eaha->aha_lock);
#endif

		LOG(3,"enqueue");
		eaha_command(eaha->port, _erccb) ;

		splx(s);
	}
	at386_io_unlock();
}

int
eaha_sg(
	scather_entry		**sgl,
	vm_offset_t		virt,
	vm_size_t		len)
{
	vm_size_t	l1, off,l2;
	scather_entry	*seglist = *sgl;

	l1 = MACHINE_PGBYTES - (virt & (MACHINE_PGBYTES - 1));
	if (l1 > len)
	    l1 = len ;
	
	len -= l1;
	off = 1;
	while (1) {
		while(len > 0 && (kvtophys(virt) + l1 == kvtophys(virt + l1))) {
			l2 = (len > MACHINE_PGBYTES) ? MACHINE_PGBYTES : len;
			if ((l1+l2) > eaha_sg_max_size)
				break;
			l1 += l2;
			len -= l2;
		}
		seglist->ptr = (caddr_t) kvtophys(virt);
		seglist->len = l1;
		seglist++;
		
		if (len <= 0)
		    break;
		virt += l1;
		off++;
		
		l1 = (len > MACHINE_PGBYTES) ? MACHINE_PGBYTES : len;
		len -= l1;
	}
	*sgl = seglist;
	return off;
}

void
eaha_prepare_rccb(
	target_info_t		*tgt,
	erccb			*_erccb,
	vm_offset_t		virt,
	vm_size_t		len,
	boolean_t		sg,
	boolean_t		passive)
{

	io_req_t	ior;

	_erccb->_eccb.cdb_len = tgt->transient_state.cmd_count;

	_erccb->_eccb.command = EAHA_CMD_INIT_CMD;/* default common case */

	_erccb->_eccb.ses = 0;
	_erccb->_eccb.dat = 0;

	if (virt == 0) {
		/* no xfers */
		_erccb->_eccb.scather = 0 ;
		_erccb->_eccb.scathlen = 0 ;
                _erccb->_eccb.sg = 0 ;
	} else {
		/* messy xfer */
		scather_entry		*seglist;
		int			ne;

		_erccb->_eccb.sg = 1 ;

		if (tgt->dma_ptr == 0) /* Only for dummy targets */
		    eaha_alloc_segment_list(&(tgt->dma_ptr));
		seglist = (scather_entry *) tgt->dma_ptr;

		/* kvtophys requires type-casts to/from vm_offset_t */
		_erccb->_eccb.scather = (caddr_t)kvtophys((vm_offset_t)seglist);

		if (sg) {
			io_scatter_t ios = (io_scatter_t)virt;
			ne = 0;
			while(len) {
				assert(ne < SCATHER_ENTRIES);
				ne += eaha_sg(&seglist, ios->ios_address,
					      ios->ios_length);
				len -= ios->ios_length;
				ios++;
				assert(len >= 0);
			}
			assert(ne <= SCATHER_ENTRIES);
#if	CHAINED_IOS
		} else if ((ior = tgt->ior) &&
			   (tgt->flags & TGT_CHAINED_IO_SUPPORT) &&
			   (ior->io_op & IO_CHAINED)) {
			ne = 0;
			while(len) {
			  	int count = ior->io_count;

			  	assert(ior);
				assert(ne < SCATHER_ENTRIES);
				if (ior->io_link)
					count -= ior->io_link->io_count;
				assert(count > 0);
				ne +=  eaha_sg(&seglist,
					       (vm_offset_t)ior->io_data,
					       count);
				len -= count;
				ior = ior->io_link;
			}
#endif	/* CHAINED_IOS */
		} else {
			ne = eaha_sg(&seglist, virt, len);
		}
		_erccb->_eccb.scathlen = ne * sizeof(*seglist);
	        if (passive) {
			/*
			 * This is where we bend over backwards to deal
			 * with board brain-deadness wrt S/G in target
			 * mode.
			 */
			int i, size;
			seglist = (scather_entry *)tgt->dma_ptr;
#if 0
			for(i=0,size=0;i<ne;i++) {
				size += seglist[i].len;
				if (seglist[i].len % MACHINE_PGBYTES) {
					i++;
					break;
				}
			}
#else
			i=1;
			size=seglist[0].len;
#endif
			if (i<ne) {
				_erccb->_eccb.scathlen = i * sizeof(*seglist);
				if (tgt->transient_state.in_count)
				    tgt->transient_state.in_count = size;
				if (tgt->transient_state.out_count)
				    tgt->transient_state.out_count = size;
			}
			assert(size == tgt->transient_state.out_count +
			       tgt->transient_state.in_count);
			ne = i;
		}
#if	MAX_TRANSFER
	        if (passive || sg ) {
			int i, size;
			seglist = (scather_entry *)tgt->dma_ptr;
			for(i=0,size=0;i<ne;i++) {
				if (size + seglist[i].len > MAX_TRANSFER)
				    seglist[i].len = MAX_TRANSFER - size;
				size += seglist[i].len;
				if (size == MAX_TRANSFER) {
					i++;
					break;
				}
			}
			_erccb->_eccb.scathlen = i * sizeof(*seglist);
			if (tgt->transient_state.in_count)
			    tgt->transient_state.in_count = size;
			if (tgt->transient_state.out_count)
			    tgt->transient_state.out_count = size;
			assert(size == tgt->transient_state.out_count +
			       tgt->transient_state.in_count);
		}
#endif
	}
}


/*
 * Allocate dynamically segment lists to
 * targets (for scatter/gather)
 */
vm_offset_t	eaha_seglist_next = 0, eaha_seglist_end = 0 ;
#define	EALLOC_SIZE	(SCATHER_ENTRIES * sizeof(scather_entry))

void
eaha_alloc_segment_list(
	char	**ptr)
{

/* XXX locking */
/* ? Can't spl() for unknown duration */
	if ((eaha_seglist_next + EALLOC_SIZE) > eaha_seglist_end) {
		(void)kmem_alloc_wired(kernel_map,&eaha_seglist_next,4*PAGE_SIZE);
		eaha_seglist_end = eaha_seglist_next + 4*PAGE_SIZE;
	}
	*ptr = (char *)eaha_seglist_next;
	eaha_seglist_next += EALLOC_SIZE;
/* XXX locking */
}

void
eaha_print_active(eaha_softc_t eaha)
{
	register target_info_t	*tgt;
	register int		i;

	for (i = 0; i < NECCBS; i++) {
		tgt = eaha->_erccbs[i].active_target;
		if (tgt)
		    printf("(%x[%d],%x)Target %d active, cmd x%x in x%x out x%x\n", 
			   &eaha->_erccbs[i],
			   i,
			   tgt, tgt->target_id,
			   tgt->cur_cmd,
			   tgt->transient_state.in_count,
			   tgt->transient_state.out_count);
	}
}


void
eaha_scsibus_was_reset(eaha_softc_t	eaha)
{
	register target_info_t	*tgt;
	register int		i;

	eaha_print_active(eaha);
	/* We don't need to actually do anything.  It will all be
	   handled by the controller (We hope) TRM 4-47*/
	/* this should be a level higher in the scsi code, but what
	   it has is wrong, so for now... */
	if (!eaha->sc)
	    return;
	for(i=0; i<NTARGETS; i++) {
		target_info_t *tgt = eaha->sc->target[i];
		if (tgt) tgt->flags &= ~TGT_PROBED_LUNS;
	}
}
/*
 *
 * shameless copy from above
 */
void
eaha_reset_scsibus(
	register eaha_softc_t	eaha)
{
	register target_info_t	*tgt;
	register int		i;

	/* Check for space commands first */

	for (i = 0; i < NECCBS; i++) {
		tgt = eaha->_erccbs[i].active_target;
		if ( tgt && tgt->cur_cmd == SCSI_CMD_SPACE && tgt->done == SCSI_RET_IN_PROGRESS) {
			eaha->wd.watchdog_state = SCSI_WD_ACTIVE;
			return;
		}
	}
	eaha_print_active(eaha);
	eaha_bus_reset(eaha);
}

boolean_t
eaha_probe_target(
	target_info_t		*tgt,
	io_req_t		ior)
{
	eaha_softc_t    eaha = eaha_softc_pool[tgt->masterno];
	boolean_t	newlywed;

	newlywed = (tgt->cmd_ptr == 0);
	if (newlywed) {
		/* desc was allocated afresh */
		(void) eaha_tgt_alloc(eaha,tgt->target_id, tgt);
	}

	if (scsi_inquiry(tgt, SCSI_INQ_STD_DATA) == SCSI_RET_DEVICE_DOWN)
		return FALSE;

	tgt->flags = TGT_ALIVE;
	return TRUE;
}


/*
 * Interrupt routine (enhanced mode)
 *	Take interrupts from the board
 *
 * Implementation:
 *	TBD
 */
void
eaha_intr(
	  int			unit)
{
	eaha_intr_common(unit, TRUE);
}

void
eaha_intr_common(
		 int			unit,
		 boolean_t		check)
{
	register eaha_softc_t	eaha;
	register		port;
	unsigned 		g2intst, g2stat, g2stat2 ;
	caddr_t			mbi ;
	erccb			*_erccb ;
	status_block		*status ;

#if	MAPPABLE
	extern boolean_t	rz_use_mapped_interface;

	if (rz_use_mapped_interface) {
                EAHA_intr(unit);
                return ;
        }
#endif	/*MAPPABLE*/

	eaha = eaha_softc_pool[unit];
	port = eaha->port;

	LOG(5,"\n\tintr");
gotintr:
	/* collect ephemeral information */
	
	g2intst = inb(G2INTST(port)) ;		/* See TRM4-22..23 */
	mbi = (caddr_t) (inb(MBOXIN0(port)) + (inb(MBOXIN1(port))<<8) +
		(inb(MBOXIN2(port))<<16) + (inb(MBOXIN3(port))<<24)) ;

	/* we got an interrupt allright */
#if	WATCHDOG
	if (eaha->wd.nactive)
		eaha->wd.watchdog_state = SCSI_WD_ACTIVE;
#endif

	outb(G2CNTRL(port),0x40) ;	/* Clear EISA interrupt */

	switch(g2intst>>4) {
		case 0x0a :	/* immediate command complete - don't expect */
		case 0x0e :	/* ditto with failure */
		    if (eaha_immediate_command_complete_expected) {
			    eaha_immediate_command_complete_expected--;
		    /* This can happen with another host reseting the bus */
		    	break;
		    }
		case 0x07 :	/* hardware error ? */
		default :
			log(	LOG_KERN,
				"eaha%d: Bogus status (x%x) in MBI g2intst = $x\n",
				unit, mbi, g2intst);
			gimmeabreak() ; /* Any of above is disaster */
			break; 

		case 0x0d :	/* Asynchronous event TRM6-41 */
			if ((g2intst & 0x0f) == (inb(SCSIDEF(eaha->port)) & 0x0f)) {
				printf("MBI: %x \n",mbi);
				eaha_scsibus_was_reset(eaha) ;
			} else
				eaha_target_intr(eaha,
					/* Type cast for bit-mashing */
					(unsigned int)mbi,
					g2intst & 0x0f);
			break;

		case 0x0c : 	/* ccb complete with error */
		case 0x01 :	/* ccb completed with success */
		case 0x05 :	/* ccb complete with success after retry */

			_erccb = (erccb *)
				( ((vm_offset_t)&eaha->I_hold_my_phys_address) +
				(mbi - eaha->I_hold_my_phys_address) -
				(vm_offset_t)&(((erccb *)0)->_eccb) ) ;
				/* That ain't necessary. As kernel (must be) */
				/* contiguous, only need delta to translate */

			status = &_erccb->status ;

#ifdef NOTDEF
			if (!eaha_quiet && (!status->don || status->qf ||
				status->sc || status->dover ||
				status->ini || status->me)) {
				printf("\nccb complete error G2INTST=%02X\n",
					g2intst) ;
				DUMP(*_erccb) ;
				gimmeabreak() ;
			}
#endif /* NOTDEF */

			eaha_initiator_intr(eaha, _erccb);
			break;
	}

	if (check) 
	    /* See if more work ready */
	    if (inb(G2STAT(port)) & 0x02) {
		    LOG(7,"\n\tre-intr");
		    goto gotintr;
	    }
}

/*
 * The interrupt routine turns to one of these two
 * functions, depending on the incoming mbi's role
 */
void
eaha_target_intr(
	eaha_softc_t	eaha,
	unsigned int	mbi,
	unsigned char	peer)
{
	target_info_t		*initiator;	/* this is the caller */
	target_info_t		*self;		/* this is us */
	int			len;

	self = eaha->sc->target[eaha->sc->initiator_id];

	initiator = eaha->sc->target[peer];

	/* ..but initiators are not required to answer to our inquiry */
	if (initiator == 0) {
		/* allocate */
		initiator = eaha_tgt_alloc(eaha, peer, (target_info_t *) 0);

		/* We do not know here whether the host was down when
		   we inquired, or it refused the connection.  Leave
		   the decision on how we will talk to it to higher
		   level code */
		LOG(0xC, "new_initiator");
		sccpu_new_initiator(self, initiator);
		/* Bug fix: was (aha->sc, self, initiator); dph */
	}

#if	DOUBLEI
	if (eaha->ti[peer].p_tm_r) {
		erccb *_erccb = eaha->ti[peer].p_tm_r;
		eaha->ti[peer].p_tm_r = (erccb *)0;
		LOG(0x1e,"enqueue-passive");
		eaha_command(eaha->port, _erccb);
		return;
	} else
	    eaha->ti[peer].selected = TRUE;
#endif

	/* The right thing to do would be build an ior
	   and call the self->dev_ops->strategy routine,
	   but we cannot allocate it at interrupt level.
	   Also note that we are now disconnected from the
	   initiator, no way to do anything else with it
	   but reconnect and do what it wants us to do */

	/* obviously, this needs both spl and MP protection */
	self->dev_info.cpu.req_target = initiator;
	self->dev_info.cpu.req_pending = TRUE;
	self->dev_info.cpu.req_id = peer ;
	self->dev_info.cpu.req_lun = (mbi>>24) & 0x07 ;
	self->dev_info.cpu.req_cmd =
		(mbi & 0x80000000) ? SCSI_CMD_SEND: SCSI_CMD_RECEIVE;
	len = mbi & 0x00ffffff ;

	self->dev_info.cpu.req_len = len;

	LOG(0xB,"tgt-mode-restart");
	(*self->dev_ops->restart)( self, FALSE);

	/* The call above has either prepared the data,
	   placing an ior on self, or it handled it some
	   other way */
	if (self->ior == 0)
		return;	/* I guess we'll do it later */

	{
		erccb	*_erccb ;
		boolean_t sg = (self->ior && (self->ior->io_op & IO_SCATTER));

		assert(self->ior->io_op & IO_PASSIVE);
		_erccb = erccb_alloc(eaha) ;
		_erccb->active_target = initiator;

		eaha_prepare_rccb(initiator, _erccb,
			(vm_offset_t)self->ior->io_data,
				  self->ior->io_count,
				  sg, TRUE);
		_erccb->_eccb.dir = (self->cur_cmd == SCSI_CMD_SEND) ? 1 : 0 ;
		_erccb->_eccb.dat = 1;
		_erccb->_eccb.command = EAHA_CMD_TARG_CMD ;
		_erccb->_eccb.lun = initiator->lun;

#if	WATCHDOG
		simple_lock(&eaha->aha_lock);
		if (eaha->wd.nactive++ == 0)
			eaha->wd.watchdog_state = SCSI_WD_ACTIVE;
		simple_unlock(&eaha->aha_lock);
#endif	/*WATCHDOG*/
		
		eaha_command(eaha->port, _erccb);
	}
}

#define CHECK_IOR 1

void foo(int bar);
void foo(int bar)
{
}

void
eaha_initiator_intr(
	eaha_softc_t	eaha,
	erccb		*_erccb)
{
	scsi2_status_byte_t	status;
	target_info_t		*tgt;
	io_req_t		ior;

	tgt = _erccb->active_target;
	ior = tgt->ior;
#if CHECK_IOR
	if (ior)
	    foo(ior->io_op);
#endif
	_erccb->active_target = 0;

	/* shortcut (sic!) */
	if (_erccb->status.ha_status == HA_STATUS_SUCCESS)
		goto allok;

	switch (_erccb->status.ha_status) {	/* TRM6-17 */
	    case HA_STATUS_SUCCESS :
allok:
		status = _erccb->status.target_status ;
		if (status.st.scsi_status_code != SCSI_ST_GOOD) {
			scsi_error(tgt, SCSI_ERR_STATUS, status.bits, 0);
			tgt->done = (status.st.scsi_status_code == SCSI_ST_BUSY) ?
				SCSI_RET_RETRY : SCSI_RET_NEED_SENSE;
		} else
			tgt->done = SCSI_RET_SUCCESS;
		break;

	    case HA_STATUS_SEL_TIMEOUT :
		if (tgt->flags & TGT_FULLY_PROBED)
			tgt->flags = 0; /* went offline */
		tgt->done = SCSI_RET_DEVICE_DOWN;
		break;

	    case HA_STATUS_OVRUN : {
                /* 
		 * BUT we don't know if this is an underrun.
		 * It is ok if we get less data than we asked
		 * for, in a number of cases.  Most boards do not
		 * seem to generate this anyways, but some do.
		 */
		    register int cmd = tgt->cur_cmd;
		    switch (cmd) {
			  case SCSI_CMD_INQUIRY:
			  case SCSI_CMD_REQUEST_SENSE:
			  case SCSI_CMD_RECEIVE_DIAG_RESULTS:
			  case SCSI_CMD_MODE_SENSE:
			    if (_erccb->status.du) /*Ignore underrun only*/
				break;
			  case SCSI_CMD_SEND:
			  case SCSI_CMD_RECEIVE: {
				  scsi_sense_data_t *sensep = 
				      &eaha_xsns[tgt->target_id].sns;
				  ior->io_residual = _erccb->status.residue;
				  if (!ior->io_residual)
				      if (ior->io_op & IO_PASSIVE) {
					      /* We cannot get the residual
					       * out of the sense info since
					       * we don't have it yet.  We
					       * will instead grab it from
					       * the CCB
					       */
					      ior->io_residual = -1*
						  (_erccb->status.cdb[2] +
						   _erccb->status.cdb[3]<<8 +
						   _erccb->status.cdb[4]<<16);
					      printf("ior 0x%x\n",ior);
					      gimmeabreak();
				      } else {
					      ior->io_residual = 
						  sensep->u.xtended.info3 +
						  sensep->u.xtended.info2<<8 +
						  sensep->u.xtended.info1<<16 +
						  sensep->u.xtended.info0<<24;
				      }
				  if (ior->io_op & IO_PASSIVE)
				      ior->io_residual *= -1;
				  /*
				   * This means we got all we wanted
				   * and the other side needs to worry
				   */
				  if (ior->io_residual < 0)
				      ior->io_residual = 0;
				  break;
			  }
			  default:
			    status = _erccb->status.target_status ;
			    printf("eaha: U/OVRUN on scsi command 0x%x status 0x%x tgt 0x%x tgt_status %x\n",
				   cmd, (int)&_erccb->status, tgt,
				   status.st.scsi_status_code);
			    if (_erccb->status.sns)
				scsi_print_sense_data(&eaha_xsns[tgt->target_id].sns);
			    gimmeabreak();
		    }
		    goto allok;
	    }

	    case HA_STATUS_BUS_FREE :
                printf("eaha: bad disconnect\n");
		tgt->done = SCSI_RET_ABORTED;
		break;
	    case HA_STATUS_PHASE_ERROR :
	    case HA_STATUS_ADP_ABORTED :
		tgt->done = SCSI_RET_ABORTED|SCSI_RET_RETRY;
		break;
	    case HA_STATUS_BAD_OPCODE :
		printf("eaha: BADCCB\n");gimmeabreak();
		tgt->done = SCSI_RET_RETRY;
		break;

	    case HA_STATUS_HOST_ABORTED :
	    case HA_STATUS_NO_FIRM :
	    case HA_STATUS_NOT_TARGET :
	    case HA_STATUS_INVALID_LINK :	/* These aren't expected. */
	    case HA_STATUS_BAD_CBLOCK :
	    case HA_STATUS_DUP_CBLOCK :
	    case HA_STATUS_BAD_SCATHER :
	    case HA_STATUS_RSENSE_FAIL :
	    case HA_STATUS_TAG_REJECT :
	    case HA_STATUS_HARD_ERROR :
	    case HA_STATUS_TARGET_NOATTN :
	    case HA_STATUS_HOST_RESET :
	    case HA_STATUS_OTHER_RESET :
	    case HA_STATUS_PROG_BAD_SUM :
	    default :
		printf("eaha: bad ha_status (x%x)\n", _erccb->status.ha_status);
		tgt->done = SCSI_RET_ABORTED;
		gimmeabreak();
		break;
	}

	if (ior && ((ior->io_op & IO_INTERNAL) == 0)) {
		/* 
		 * This little bit is to handle more board brain deadness where
		 * we have to chop off the request and send less when we have
		 * a "bad" target mode S/G.
		 */

		int len;
		/* Since only one of these is non zero */
		len = tgt->transient_state.in_count +
		    tgt->transient_state.out_count;
		ior->io_residual += ior->io_count - len;
	}


	eaha->ti[tgt->target_id].has_sense_info =
		(tgt->done == SCSI_RET_NEED_SENSE) ;
	if (eaha->ti[tgt->target_id].has_sense_info)
		eaha->ti[tgt->target_id].sense_info_lun = tgt->lun ;

	LOG(8,"end");

#if	WATCHDOG
	simple_lock(&eaha->aha_lock);
	if (eaha->wd.nactive-- == 1)
		eaha->wd.watchdog_state = SCSI_WD_INACTIVE;
	simple_unlock(&eaha->aha_lock);
#endif	/*WATCHDOG*/

	if (ior) {
		LOG(0xA,"ops->restart");
		(*tgt->dev_ops->restart)( tgt, TRUE);
	}

	return;
}

#endif	/* NEAHA > 0 */
