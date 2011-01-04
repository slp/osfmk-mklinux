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
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: busconf.c 1.18 95/04/26$
 *	Author: Jeff Forys, University of Utah CSL
 */

/* HP700-hack-start */
#include <types.h>
#include <mach/vm_param.h>
#include <machine/iomod.h>

#include <grf.h>

#include <machine/viper.h>
#include <machine/asp.h>
#include <machine/cpu.h>
#include <machine/pdc.h>
#include <kern/spl.h>
#include <machine/psw.h>

#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/trap.h>
#include <hp_pa/c_support.h>
#include <hp_pa/HP700/bus_protos.h>

#include <eisaha.h>
#if      NEISAHA > 0
#include <hp_pa/HP700/eisa_common.h>
extern void scan_eisabus(void);
#endif

#define	BITSET(bit)	((unsigned int)0x80000000 >> bit)

/*
 * HP700 bus configuration.
 *	- The memory-mapped I/O SubSystem "Core" bus is also configured.
 *	  We configure the Memory modules as before (behind the Viper?).
 *
 * Warning: this routine is called in real mode, and before zeroing BSS.
 * Any variables that need to be accessed from other parts of the kernel
 * should be forced to data (with HP's C compiler, this is accomplished
 * by initializing the variables).
 */

struct iomod *prochpa = 0;	/* HPA for processor (CPU) */

int	viperix = -1;		/* modtab index of viper */
struct	iomod *viper = 0;	/* HPA of viper */
#define	VI_MPTR	(&modtab[viperix])
int	coreix = -1;		/* modtab index of ASP */
int	eisaix = -1;		/* modtab index of EISA */
int	waxix = -1;		/* modtab index of WAX */

int	aspibit;		/* EIRR bit for ASP */
int	waxibit;		/* EIRR bit for WAX */

struct asp_ctrl	*primary_ctrl = 0;
struct asp_ctrl *wax_ctrl = 0;	/* control regs for WAX */

extern int isgecko;		/* set if we are a 712 (LASI) */

unsigned char lan_addr[PDC_LAN_STATION_ID_SIZE] = {0}; /* Core LAN address */
unsigned int spa_size = 0;	/* amount of SPA space used by mem_size */
unsigned int mem_size = 0;	/* bottom of memory (alloc up) */
unsigned int io_size = FP_ADDR;	/* top of IO space (alloc down) */

/*static*/ struct modtab modtab[MODTABSIZ] = { &modtab[0] };
static	struct modtab *mptr = modtab;

static	unsigned int spaptr = 0;/* amt of SPA space used by mem_size */
static	int busconfig = 0;	/* set true when bus is configured */

void
busconf(void)
{
	struct iomod *main_bus;
	struct pdc_iodc_read ignore;

	if (prochpa == 0) {
		struct pdc_hpa pdcret;
		if ((*pdc)(PDC_HPA, PDC_HPA_DFLT, &pdcret) < 0)
			panic("can't read processor HPA");
		prochpa = pdcret.hpa;
	}

	/*
	 * Determine the address range for our main bus; we know it is
	 * the same bus that our processor is on, so just mask off the
	 * insignificant bits from its addr.
	 */
	main_bus = (struct iomod *)((unsigned int)prochpa & FLEX_MASK);

	/*
	 * If we've configured the bus once, there's no sense rebuilding
	 * our module table again.  Just reconfigure the bus and return.
	 */
	if (busconfig) {
		reconfig(main_bus, modtab[0].mt_next);
		return;
	}

	/*
	 * Broadcast the HPA to all cards on this bus; FPA_IOMOD will
	 * push us into Local Broadcast Address Space.  Then, initialize
	 * several global variables.
	 */
	(main_bus + FPA_IOMOD)->io_flex =
	 	(char *)((unsigned int)main_bus|DMA_ENABLE);

	mem_size = PAGE0->imm_max_mem;
	spaptr = PAGE0->imm_spa_size;

	/*
	 * Add our first memory module to the table.  This is a special
	 * case; we know it is here, otherwise we wouldnt be!  We also
	 * need the iodc_data for this Memory module; get this first.
	 */
	if ((*pdc)(PDC_IODC, PDC_IODC_READ, &ignore, PAGE0->imm_hpa,
	    PDC_IODC_INDEX_DATA, &mptr->mt_type, sizeof(mptr->mt_type)) < 0)
		panic("Initial Memory Module failed");

	mptr->m_hpa = PAGE0->imm_hpa;
	mptr->m_fixed = mptr->m_hpa - main_bus;
	mptr->m_spa = 0;
	mptr->m_spasiz = PAGE0->imm_spa_size;
	mptr->m_memsiz = PAGE0->imm_max_mem;
	mptr->mt_next = mptr + 1;
	mptr++;

	/*
	 * Go out on our main bus and make a list of what *else* we have.
	 * Record information about the ASP and VIPER for later use.
	 */
	scanbus(-1, MAXMODBUS);
	if (viperix < 0)
		viperix = 0;
	viper = (struct  iomod *)VI_MPTR->m_hpa;
	if (coreix < 0)
		panic("No ASP found");

	/*
	 * Besides the main bus (where CPU and memory hang out), there is
	 * also the "Core" I/O SubSystem bus.  For simplicity, we consider
	 * the Core bus as part of the main bus.
	 */
	(mptr - 1)->mt_next = mptr;
	scanbus(coreix, MAXMODCORE);

	if (waxix >= 0) {
		(mptr - 1)->mt_next = mptr;
		scanbus(waxix, MAXMODCORE);
	}

	/*
	 * Set up Viper control registers.
	 * Deny everyone access to bus, then enable graphics (if supported).
	 * Give "Core" I/O SubSystem bus priority.
	 */
	if (!isgecko) {
		spl_t s = splhigh();
		int psw = rsm(PSW_D);

		VI_CTRL |= VI_CTRL_ANYDEN;
#if NGRF > 0
		((struct vi_ctrl *)&VI_CTRL)->sgc0_den = 0;
		((struct vi_ctrl *)&VI_CTRL)->sgc1_den = 0;
#endif
		((struct vi_ctrl *)&VI_CTRL)->core_prf = 1;
		VI_TRS->vi_control = VI_CTRL;
		if (psw & PSW_D)
			ssm(PSW_D);
		splx(s);
	}
	/*
	 * Allocate SPA space for our new-found modules
	 * and reconfigure the bus.
	 */
	allocspa();
	reconfig(main_bus, modtab[0].mt_next);

#if      NEISAHA > 0
        if(eisaix != -1) {
                (mptr - 1)->mt_next = mptr;
                scan_eisabus();
        }
#endif
	busconfig = 1;			/* bus is configured (for getmod()) */

	showbus();
}

#define	SPA_SAFE	PAGE0->imm_spa_size

void
scanbus(
	int bus,
	int maxmod)
{
	struct pdc_iodc_read ignore;
	struct pdc_memmap memmap;
	struct device_path dp;
	struct iomod *io;
	int ix;
	struct modtab *m = mptr;	/* save mptr for later */

	/*
	 * For each module on the bus, ask the PDC what it is; if we
	 * support the module, add it to our module table.
	 */
	for (ix = 0; ix < maxmod; ix++) {
		dp.dp_bc[0] = dp.dp_bc[1] = dp.dp_bc[2] = dp.dp_bc[3] = -1;
		dp.dp_bc[4] = bus;
		dp.dp_bc[5] = bus < 0 ? -1 : 0;
		dp.dp_mod = ix;
		if ((*pdc)(PDC_MEM_MAP, PDC_MEM_MAP_HPA, &memmap, &dp) < 0)
			continue;
#if 0
printf("[%d,%d,%d]: ", dp.dp_bc[4], dp.dp_bc[5], dp.dp_mod);
printf("hpa = 0x%x, mpages = 0x%x\n", memmap.hpa, memmap.morepages);
#endif
		io = (struct iomod *) memmap.hpa;

		/*
		 * Skip processor and our initial memory module.
		 */
		if (io == prochpa || io == PAGE0->imm_hpa)
			continue;

		if ((*pdc)(PDC_IODC, PDC_IODC_READ, &ignore, io,
			   PDC_IODC_INDEX_DATA,
			   &mptr->mt_type, sizeof(mptr->mt_type)) < 0)
			continue;

		mptr->m_fixed = ix;
		mptr->m_hpa = io;
		mptr->m_spa = mptr->m_spasiz = 0;

		switch(mptr->mt_type.iodc_type) {
		    case IODC_TP_A_DMA:
			switch (mptr->mt_type.iodc_sv_model) {
			    case SVMOD_ADMA_FWSCSI:
				break;
			    case SVMOD_ADMA_MYRINET:
				break;
			    default:
				busthing("type-A DMA", mptr);
				break;
			}
			break;

		    case IODC_TP_MEMORY:	/* Memory: disable SPA */
			mptr->m_hpa->io_spa = 0;
			if (mptr->mt_type.iodc_sv_model == SVMOD_MEM_PDEP &&
			    /* XXX need proper way to id the viper */ 
			    mptr->m_hpa == (struct iomod*)0xfffbf000)
				viperix = ix;
			break;
		    /*
		     * N.B. The hp700 uses memory-mapped I/O, therefore
		     * we shouldnt need any SPA space for these modules.
		     */
		    case IODC_TP_BA:		/* hp700 bus adapter (ASP) */
			switch (mptr->mt_type.iodc_sv_model) {
			    case SVMOD_BA_LASI:
				primary_ctrl = (struct asp_ctrl*)mptr->m_hpa;
			        isgecko = 1;
				coreix = ix;
				break;
			    case SVMOD_BA_ASP:
				primary_ctrl = (struct asp_ctrl *)0xF0800000;
				coreix = ix;
				break;
			    case SVMOD_BA_WAX:
				waxix = ix;
				wax_ctrl = (struct asp_ctrl *) mptr->m_hpa;
				break;
			    case SVMOD_BA_WEISA:
			    case SVMOD_BA_EISA:
#if      NEISAHA > 0
                                eisaix = ix;
#endif
				break;
			    default:
				busthing("bus adaptor", mptr);
				break;
			}
			break;

		    case IODC_TP_FIO:		/* hp700 core bus thinger */
			switch (mptr->mt_type.iodc_sv_model) {
			    case SVMOD_FIO_GSCSI:
			    case SVMOD_FIO_SCSI:
				{
					struct foo { int field[32]; } *foop;

					foop = (struct foo *)&ignore;
					mptr->m_scsiclk = foop->field[16];
				}
				break;

			    case SVMOD_FIO_HIL:
			    case SVMOD_FIO_FWSCSI:
			    case SVMOD_FIO_FDDI:
			    case SVMOD_FIO_CENT:
			    case SVMOD_FIO_RS232:
			    case SVMOD_FIO_SGC:
			    case SVMOD_FIO_A1:
			    case SVMOD_FIO_A1NB:
			    case SVMOD_FIO_A2:
			    case SVMOD_FIO_A2NB:
			    case SVMOD_FIO_GPCIO:
			    case SVMOD_FIO_GPCFD:
			    case SVMOD_FIO_GRS232:
				break;
			    case SVMOD_FIO_LAN:
			    case SVMOD_FIO_GLAN:
				{
				    struct pdc_lan_station_id id;
				    unsigned char *addr;
				    unsigned int i;
				    if ((*pdc)(PDC_LAN_STATION_ID,
					       PDC_LAN_STATION_ID_READ,
					       &id, mptr->m_hpa) < 0)
					addr = (unsigned char *)ASP_PROM;
				    else
					addr = id.addr;
				    for (i = 0;
					 i < PDC_LAN_STATION_ID_SIZE; i++)
					lan_addr[i] = addr[i];
				}
				break;

			    case SVMOD_FIO_GGRF:
				/*
				 * XXX grab the STI ROM address which may
				 * not be part of the HPA space.
				 */
				mptr->m_stirom = PAGE0->pd_resv2[1];
				break;

			    default:
				busthing("foreign I/O module", mptr);
				break;
			}
			break;
		    default:			/* unsupported module  */
			busthing("module", mptr);
			break;
		}
		mptr->mt_next = mptr + 1;
		mptr++;
	}
	(mptr - 1)->mt_next = 0;

	/*
	 * Initialize any other memory boards.
	 *
	 * N.B. The Initial Memory Module was added to our module table
	 * before calling this routine.  As a result, `m' (i.e. `mptr'
	 * when scanbus() was called) already points past the IMM.
	 */
	for (; m; m = m->mt_next) {

		if (m->mt_type.iodc_type != IODC_TP_MEMORY)	/* nopers */
			continue;

		switch (m->mt_type.iodc_sv_model) {
		    case SVMOD_MEM_ARCH: {	/* architected */
			unsigned int spa, spaincr, spamax;
			volatile int junk;

			/*
			 * Send a reset command to the memory board.
			 * Give this memory module a fake SPA to enable it.
			 * Prepare for memtest by trapping certain errors.
			 */
			m->m_hpa->io_command = CMD_RESET;
			m->m_hpa->io_spa = IOSPA(SPA_SAFE,m->mt_type);
			m->m_hpa->io_control = IO_CTL_MEMINIT;

			m->m_spasiz = 1 << m->mt_type.iodc_spa_shift;

			/*
			 * We have "m->m_spasiz" bytes of memory; testing
			 * all of it will take too long!  Our memory boards
			 * are 16MB.  If we assume that the smallest memory
			 * board is 256K (given the boot size restriction),
			 * lets check every 128K or so (this will scale).
			 */
			spaincr = m->m_spasiz >> 7;
			spamax = SPA_SAFE + m->m_spasiz;
			for (spa = SPA_SAFE; spa < spamax; spa += spaincr) {
				junk = *(int *)spa;
				if (IO_ERR_VAL(m->m_hpa->io_status)==IO_ERR_SPA)
					break;
			}

			m->m_memsiz = spa - SPA_SAFE;	/* usable memory */
			m->m_hpa->io_command = CMD_CLEAR;

			break;
		    }

		    case SVMOD_MEM_PDEP: {	/* processor-dependent */
			struct pdc_iodc_minit minit;

			/*
			 * All we can do here is contact the PDC to find out
			 * how much memory this module has.
			 */
			(*pdc)(PDC_IODC, PDC_IODC_NINIT, &minit, m->m_hpa,
			       SPA_SAFE);
			m->m_memsiz = minit.max_mem;
			m->m_spasiz = minit.max_spa;
			break;
		    }

		    default:
			busthing("memory", m);
		}

		m->m_hpa->io_spa = 0;		/* disable */
	}
}
#undef	SPA_SAFE

/*ARGSUSED*/
void
allocspa(void)
{
#ifdef IO_HACK
	/*
	 * XXX all IO space has the same TLB attributes.
	 */
	io_size = 0;
#else
	/*
	 * XXX we divide the IO space into four quadrents:
	 *
	 *	0: 0xF0000000-0xF3FFFFFF:	ASP and unused
	 *	1: 0xF4000000-0xF7FFFFFF:	SGC1
	 *	2: 0xF8000000-0xFBFFFFFF:	SGC2
	 *	3: 0xFC000000-0xFFFFFFFF:	EISA/venom/CPU/viper/unused
	 *
	 * This is a cheap (time-wise) hack that allows us to map
	 * framebuffers without allocating a phys_entry for every
	 * page of IO space or using the block TLBs (not up to that
	 * yet).  Quads 0 and 3 are always assigned to the kernel.
	 * Quads 1 and 2 correspond to the SGC slots (i.e. graphics
	 * cards) which can be mapped by users.  These start off
	 * assigned to the kernel, but when mapped by a user receive
	 * their PID making that range accessible to only that task.
	 * This creates two big problems:
	 *
	 *	- an SGC slot can only be mapped by a single task at
	 *	  any given time, we live with this restriction.
	 *
	 *	- when mapped by a task the kernel cannot access the
	 *	  region, we hack around this right now by assuming
	 *	  that the region will always refer to a framebuffer
	 *	  and "fix" the ITE and GRF code to turn off PID
	 *	  checking prior to accessing the area.
	 *
	 * We implement this hack by making the IO space look like
	 * it has 4 (really big) pages.  The TLB miss handler knows
	 * about this "convention".
	 */
	io_size = -ptoa(4);
#endif
}

/*ARGSUSED*/
void
reconfig(
	 struct iomod *bus,
	 struct modtab *mhead)
{
	struct modtab *m;
	unsigned int irr;
	int ibit;

	/*
	 * Initialize the Viper's EIR and Interrupt Word mask.
	 */
	ibit = procitab(SPLVIPER, viperintr, 0);
	aspibit = procitab(SPLASP, aspintr, 0);
	VI_MPTR->m_vi_iwd = BITSET(aspibit);

	/*
	 * Get the ASP into a known state
	 */
	if (isgecko) {
		primary_ctrl->iar = (int)prochpa | aspibit;
		primary_ctrl->icr = 0;
		/*
		 * Setup the WAX, disable interrupts.
		 * XXX fixme: reasonable priority.
		 */
		if (waxix >= 0) {
			waxibit = procitab(SPLWAX, waxintr, 0);
			wax_ctrl->iar = (int)prochpa | waxibit;
			wax_ctrl->icr = 0;
			wax_ctrl->imr = -1;
			irr = wax_ctrl->irr;
			wax_ctrl->imr = 0;
		}
	} else {
		if (aspibit >= 0)
			VI_TRS->vi_intrwd = VI_MPTR->m_vi_iwd;
	}
	primary_ctrl->imr = -1;
	irr = primary_ctrl->irr;
	primary_ctrl->imr = 0;

	/*
	 * Disable all SPA space.
	 */
	for (m = mhead; m; m = m->mt_next)
		if (m->m_spa)
			m->m_hpa->io_spa = 0;

	/*
	 * Do module specific reconfiguration.
	 */
	for (m = mhead; m; m = m->mt_next) {
		switch(m->mt_type.iodc_type) {
		    case IODC_TP_MEMORY:

			if (m->m_spa == 0)	/* hole in memory: ignored */
				break;

			if (m->mt_type.iodc_sv_model == SVMOD_MEM_ARCH) {

				/* reset board, init SPA, enable err logging */
				m->m_hpa->io_command = CMD_RESET;
				m->m_hpa->io_spa = IOSPA(m->m_spa,m->mt_type);
				m->m_hpa->io_control = IO_CTL_MEMOKAY;
			} else if (m->mt_type.iodc_sv_model == SVMOD_MEM_PDEP) {
				struct pdc_iodc_minit minit;

				/* tell module IODC to initialize memory */
				(*pdc)(PDC_IODC, PDC_IODC_NINIT, &minit,
				       m->m_hpa, m->m_spa);
			}
			break;
		    case IODC_TP_BA:
			/* (m->mt_type.iodc_sv_model == SVMOD_BA_ASP) ... */
			break;
		}
	}
}

/*
 * We found something on a bus that we dont know how to deal with.
 * Print a message describing this thing and return.
 */
void
busthing(
    char *what,
    struct modtab *where)
{
	printf("Warning: unsupported %s at %x (type:%x svers:%x hvers:%x)\n",
	       what, where->m_hpa, where->mt_type.iodc_type,
	       where->mt_type.iodc_sv_model, where->mt_type.iodc_model);
}

/*
 * getmod() - Return HPA and SPA of a module given its type (IODC_TP_*).
 *
 *	type - type of module we are looking for.
 *	noinit - if 1, start search after module on which last search ended.
 *
 * Return modtab ptr on success, 0 if module of specific type not found.
 *
 * This is *the* interface to information stored in the module table.
 */
struct modtab *
getmod(
       int type, 
       int svmod,
       int noinit)
{
	static struct modtab *m = modtab;

	if (noinit == 0)		/* reinitialize module ptr */
		m = modtab;

	if (busconfig) {
		for (; m; m = m->mt_next)
			if (m->mt_type.iodc_type == type &&
			    (svmod == -1 || m->mt_type.iodc_sv_model == svmod)){
				struct modtab *retm = m;
				m = m->mt_next;	/* dont loop here! */
				return(retm);
			}
	}

	return(0);
}

/*
 * Displays our bus configuration.
 */
void
showbus(void)
{
	register struct modtab *m;
	unsigned int model;
	int memcnt = 0;
	register int tmp = primary_ctrl->iostat.spu_id;

	for (m = modtab; m; m = m->mt_next) {
		model = m->mt_type.iodc_sv_model;
		switch(m->mt_type.iodc_type) {
		    case IODC_TP_A_DMA:
			switch (model) {
			    case SVMOD_ADMA_FWSCSI:
				printf("fwscsi0 at core%d\n", m->m_fixed);
				break;
			    case SVMOD_ADMA_MYRINET:
				printf("myri0 at core%d\n", m->m_fixed);
				break;
			    default:
				printf("A_DMA model%d at core%d, hpa 0x%x\n",
				       model, m->m_fixed, m->m_hpa);
				break;
			}
			break;

		    case IODC_TP_MEMORY:
			if (m->m_hpa == viper) {
				printf("viper: vers %d, size %d.%dMB\n",
				       VI_TRS->vi_status.hw_rev,
				       m->m_spasiz / 0x100000,
				       m->m_spasiz % 0x100000);
				printf("viper at hpa 0x%x, eim 0x%x\n",
				       m->m_hpa, m->m_vi_iwd);
				memcnt++;
			} else {
				printf("mem%d at hpa 0x%x, size %d.%dMB\n",
				       memcnt++, m->m_hpa, m->m_spasiz/0x100000,
				       m->m_spasiz%0x100000);
			}
			break;
		    case IODC_TP_BA:
			switch (model) {
			    case SVMOD_BA_ASP:
				printf("asp0 at sc%d, vers %d\n",
				       m->m_fixed, CORE_ASP->version);
				break;
			    case SVMOD_BA_LASI:
				printf("lasi0 at sc%d, vers %d\n",
				       m->m_fixed, CORE_LASI->version & 0xff);
				break;
			    case SVMOD_BA_WAX:
				printf("wax0 at sc%d\n",
				       m->m_fixed);
				break;
			    case SVMOD_BA_EISA:
				printf("eisa0 at sc%d\n", m->m_fixed);
				break;
			    case SVMOD_BA_WEISA:
				printf("wax eisa at sc%d\n", m->m_fixed);
				break;
			    case SVMOD_BA_VME:
				printf("[unsupported] vme0 at sc%d\n",
				       m->m_fixed);
				break;
			    default:
				printf("BA model %d at core%d, hpa 0x%x\n",
				       model, m->m_fixed, m->m_hpa);
				break;
			}
			break;

		    case IODC_TP_FIO:
			switch (model) {
			    case SVMOD_FIO_SCSI:
			    case SVMOD_FIO_GSCSI:
				printf("scsi0 at core%d\n", m->m_fixed);
				break;
			    case SVMOD_FIO_FWSCSI:
				printf("fwscsi0 at core%d\n",
				       m->m_fixed);
				break;
			    case SVMOD_FIO_HPIB:
				printf("[unsupported] hpib0 at core%d\n",
				       m->m_fixed);
				break;
			    case SVMOD_FIO_LAN:
				tmp = primary_ctrl->iostat.lan_jmpr;
				printf("lan0 at core%d, type %s\n",
				       m->m_fixed, DISP_LANJMPR(tmp));
				break;
			    case SVMOD_FIO_GLAN:
				printf("lan0 at core%d\n", m->m_fixed);
				break;
			    case SVMOD_FIO_FDDI:
				printf("[unsupported] fddi0 at core%d\n",
				       m->m_fixed);
				break;
			    case SVMOD_FIO_HIL:
				printf("hil0 at core%d\n", m->m_fixed);
				break;
			    case SVMOD_FIO_CENT:
				printf("[unsupported] cent0 at core%d\n",
				       m->m_fixed);
				break;
			    case SVMOD_FIO_RS232:
				printf("dca%c at core%d\n",
				       m->m_fixed == 5 ? '1' : '0', m->m_fixed);
				break;
			    case SVMOD_FIO_GRS232:
				printf("dca0 at core%d\n", m->m_fixed);
				break;
			    case SVMOD_FIO_GGRF:
			    case SVMOD_FIO_SGC:
				printf("grf0 at sc%d", m->m_fixed);
				if (model == SVMOD_FIO_SGC)
					printf(" (sgc%d)",
					       ((int)m->m_hpa >> 26) & 3);
				printf("\n");
				break;
			    case SVMOD_FIO_A1:
			    case SVMOD_FIO_A1NB:
			    case SVMOD_FIO_A2:
			    case SVMOD_FIO_A2NB:
				printf("[unsupported] audio0 at core%d, type %d",
				       m->m_fixed,
				       model == SVMOD_FIO_A1 ||
				       model == SVMOD_FIO_A1NB ? 1 : 2);
				if (model == SVMOD_FIO_A1NB ||
				    model == SVMOD_FIO_A2NB)
					printf(" (no beeper)");
				printf("\n");
				break;
			    case SVMOD_FIO_GPCFD:
				printf("[unsupported] floppy0 at core%d\n",
				       m->m_fixed);
				break;
			    case SVMOD_FIO_GPCIO:
				if ((unsigned)m->m_hpa & 0x100)
					printf("%s at core%d, slot1\n",
					       "mouse0", m->m_fixed);
				else
					printf("%s at core%d, slot0\n",
					       "keyboard0", m->m_fixed);
				break;
			    default:
				printf("FIO model %d at core%d, hpa 0x%x\n",
				       model, m->m_fixed, m->m_hpa);
				break;
			}
			break;
		default:
			printf("type%d (model %d) at core%d, hpa 0x%x\n",
			       m->mt_type.iodc_type, model,
			       m->m_fixed, m->m_hpa);
			break;
		}
	}
}

extern struct intrtab itab_proc[];

/*
 * procitab(spllvl, handler, unit)
 *
 * Given the External Interrupt Enable Mask in `spllvl', validate it
 * and add the handler to the Processor Interrupt Table at the implied
 * interrupt priority.  If this routine is called a second time with
 * the same `spllvl', the previous table entry is quietly replaced with
 * the newer one.  If the handler is NULL, the interrupt table entry is
 * effectively reset.  If `ohandler' is non-NULL, a pointer to the old
 * handler will be stored there.
 *
 * N.B.  We are limited to one Processor EIR bit per interrupt (this
 * gives us a maximum of 32 interrupts).  This routine calculates an
 * interrupt's position in the Processor Interrupt Table (itab_proc)
 * using the SPL level.  The drawback here is that some interrupts
 * should be serviced at the same SPL level (e.g. 2 RS232 devices).
 * So, we add the `unit' number in calculating an itab_proc location
 * (assuming the unit number is really a unit number).  This code
 * should be reworked to handle multiple interrupts on a single bit,
 * at which point these assumptions can be removed.  This is really
 * just the second cut at handling interrupts on the hp700, and the
 * first attempt at combining hp700/hp800 interrupts, so...
 *
 * Return value: EIEM bit number on success, -1 on error (+message).
 */
int
procitab(
	 unsigned int spllvl,
	 void (*handler)(int),
	 int unit) 
{
	register int ibit, i;
	spl_t s;

	/*
	 * Make sure we have a valid EIEM (i.e. one of the INTPRI's
	 * #define'd in "spl.h") and ones complement it to locate
	 * the actual interrupt bit position.
	 */
	i = 0;
	switch (spllvl) {
	    case INTPRI_32:
		/*
		 * In the case of INTPRI_32 (SPL0), we have no clue where
		 * to place this in the processor interrupt table.  We do
		 * have the unit number, so place it at that offset.
		 */
		i = ibit = unit;
		break;

	    default:
		while (i < EIEM_BITCNT-1)
			if (EIEM_MASK(i++) == spllvl)	/* valid EIEM? */
				break;

	    case INTPRI_00:
		ibit = ffset(~spllvl);
		if ((unsigned int)unit < EIEM_BITCNT)		/* XXX */
			ibit += unit;
		break;
	}
	if (i < 0 || i > EIEM_BITCNT-1) {
		printf("procitab: invalid EIEM: %x (handler %x(%d))\n",
		       spllvl, handler, unit);
		return(-1);
	}

	/*
	 * Disable all interrupts and set up `itab_proc' appropriately.
	 */
	s = splhigh();
	itab_proc[ibit].handler = (handler == NULL)? int_illegal: handler;
	itab_proc[ibit].arg0 = unit;
	itab_proc[ibit].arg1 = 0;
	itab_proc[ibit].intpri = spllvl;
	splx(s);
	return(ibit);
}

/*
** The remainder of this file deals with ASP/LASI, WAX and Viper interrupts.
** It may move to it's own file someday, but not today.
**
** The ASP has it's own Interrupt Pending, Mask and Request registers.
** Upon receiving an ASP interrupt, we simply set its corresponding bit
** in the processor EIR register and let the external interrupt code
** deal with dispatching the appropriate handler.  This is not expensive
** because we have already taken an external interrupt (from the ASP).
**
** I do not know what to do with Viper interrupts yet...
*/

/*
 * Map ASP interrupt bits to Processor interrupt bits and back again.
 */
int asp2procibit[EIEM_BITCNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
int proc2aspibit[EIEM_BITCNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

void
aspinit(void)
{
	int i;

#ifdef USELEDS
	/*
	 * Initialize the LEDs
	 */
	ledinit();
#endif /* USELEDS */
#if 0
	/*
	 * XXX we don't reset the ASP anymore.
	 *
	 * Apparently HP-UX doesn't and the boot ROM seems to leave
	 * everything as we want it.
	 *
	 * XXX news flash!  The "fast" jefboot path doesn't work without
	 * the reset, so its back again!
	 *
	 * XXX EXTRA, EXTRA!  The jefboot problem has been fixed, we
	 * needed to make sure irr/imr are in a consistant state (see
	 * reconfig).  The reset is gone again.
	 */
 
	/*
	 * Reset the ASP.  After resetting the ASP, we must wait 400ms
	 * before accessing the 8042 after a reset.
	 */
	CORE_ASP->reset = 1;
#ifdef notdef
	/*
	 * Viper will set default for us.
	 */
	CORE_ASP->scsi_dsync = 0;
#endif
	CORE_8042->rstrel = 1;
	delay(400000);
	/*
	 * Configure the LAN output enable register
	 *
	 * ASP reset sets this to 0, disabling all LANs.  Note that we
	 * do nothing on an unexpected situation, this is because the
	 * documentation states that one of the possible consequences
	 * of mis-setting the register is "electrical shorts".
	 */
	switch (primary_ctrl->iostat.spu_id) {
	case SPUID_HARDBALL:
		i = primary_ctrl->iostat.net_id;
		if (i == NETID_ETHER)
			CORE_ASP->lan_oen = ASPLAN_EETHER;
		else if (i == NETID_FDDI)
			CORE_ASP->lan_oen = ASPLAN_EFDDI;
		else
			printf("WARNING: unrecognized network ID %x, %s\n",
			       i, "LAN disabled");
		break;
	case SPUID_CORALII:
		i = primary_ctrl->iostat.net_id;
		if (i == NETID_FDDI)
			CORE_ASP->lan_oen = ASPLAN_EETHER|ASPLAN_EFDDI;
		else if (i == NETID_NONE)
			CORE_ASP->lan_oen = ASPLAN_EETHER;
		else
			printf("WARNING: unrecognized network ID %x, %s\n",
			       i, "all LANs disabled");
		break;
	default:
		break;
	}
#endif

	/*
	 * Reset the ASP interrupt bit to processor EIEM mappings.
	 */
	for (i = 0; i < EIEM_BITCNT; i++)
		asp2procibit[i] = proc2aspibit[i] = -1;

	/*
	 * Enable bus grants for CORE IOSS (and possibly EISA).
	 */
	{
		spl_t s = splhigh();
		int psw = rsm(PSW_D);

		((struct vi_ctrl *)&VI_CTRL)->core_den = 0;
#if NEISAHA > 0
		((struct vi_ctrl *)&VI_CTRL)->eisa_den = 0;
#endif
		VI_TRS->vi_control = VI_CTRL;
		if (psw & PSW_D)
			ssm(PSW_D);
		splx(s);
	}
}

void
lasiinit(void)
{
	int i;
	extern int machtype;

#ifdef USELEDS
	switch(machtype) {
	case  CPU_M725_100:
	case  CPU_M725_75:
	case  CPU_M725_50:
		/*
		 * Initialize the LEDs
		 */
		ledinit();
		break;
	default:
		/*
		 * Don't know how to control LCD display on 770
		 */
		break;
	}
#endif /* USELEDS */

	/*
	 * Reset the ASP interrupt bit to processor EIEM mappings.
	 */
	for (i = 0; i < EIEM_BITCNT; i++)
		asp2procibit[i] = proc2aspibit[i] = -1;

	/*
	 * Enable bus grants for CORE IOSS (and possibly EISA).
	 */
	{
		spl_t s = splhigh();
		int psw = rsm(PSW_D);

		((struct vi_ctrl *)&VI_CTRL)->core_den = 0;
#if NEISAHA > 0
		((struct vi_ctrl *)&VI_CTRL)->eisa_den = 0;
#endif
		VI_TRS->vi_control = VI_CTRL;
		if (psw & PSW_D)
			ssm(PSW_D);
		splx(s);
	}
}


/*
 * aspitab(mask, spllvl, handler, unit)
 *
 * Given the bit in `mask', add the handler to the ASP Interrupt Table
 * with the specified interrupt priority.  If this routine is called
 * a second time with the same `mask', the previous table entry is
 * quietly replaced with the newer one.  If the handler is NULL, the
 * interrupt table entry is effectively reset.  If `ohandler' is
 * non-NULL, a pointer to the old handler will be stored there.
 *
 * Return value: ASP ibit number on success, -1 on error (+message).
 */
int
aspitab(unsigned int mask,
	unsigned int spllvl,
	void (*handler)(int),
	int unit) 
{
	register int ibit, procibit;
	spl_t s;

	/*
	 * Block ASP interrupts, determine which bit is set in `mask'
	 * and do some sanity checks, call procitab() to set up the
	 * processor interrupt table appropriately and update our
	 * ASP to Processor interrupt bit map.
	 */
	s = splasp();
	if ((ibit = ffset(mask)) >= 0) {
		if (ibit < ASPINT_LAST_L || ffset(mask & ~BITSET(ibit)) >= 0) {
			printf("aspitab: invalid mask: %x (ibit %x)\n", mask, ibit);
			goto bad;
		}
#if 0
/* FWSCSI and FDDI on the cutoff */	
		if (ibit == INT_IT1_L || ibit == INT_IT2_L) {
			printf("aspitab: ASP interval timer not supported\n");
			goto bad;
		}
#endif
		procibit = procitab(spllvl, handler, unit);
		if (procibit < 0)
			goto bad;

		if (handler) {
			/* XXX routine to call after processing interrupt */
			itab_proc[procibit].arg1 = (int) aspintsvc;
			asp2procibit[ibit] = procibit;
			proc2aspibit[procibit] = ibit;
		} else {
			asp2procibit[ibit] = proc2aspibit[procibit] = -1;
		}
	}

	/*
	 * We were successful; enable or disable this ASP interrupt.
	 */

	if (handler)
		primary_ctrl->imr |= mask;
	else
		primary_ctrl->imr &= ~mask;

	splx(s);

	return(ibit);

bad:
	splx(s);
	return(-1);
}

/*
 * aspintr()
 *
 * Service ASP interrupts.  For each pending ASP interrupt, set it's
 * corresponding bit in the Processor EIEM and let processor dispatch
 * appropriate handler.
 *
 * N.B. It is assumed that this routine will be called at SPLASP
 * (i.e. ASP interrupts are blocked from reaching the Processor).
 */
int aspicnt = 0;
void
aspintr(int i)
{
	extern struct iomod *prochpa;
	unsigned int irr, imr;
	int ibit, spurious = 0;

	aspicnt++;

	/*
	 * Prevent spurious interrupts.  A read of the IRR only clears
	 * the *unmasked* bits in the IPR.  When we set the IMR to zero
	 * (after reading the IRR), we must check to see that another
	 * interrupt did not occur.  If an interrupt did sneak in, we
	 * loop on the IRR, switching IMR masks until the IRR reads 0.
	 *
	 * The `spurious' variable can probably be eliminated (sanity).
	 */
	imr = primary_ctrl->imr;
	irr = primary_ctrl->irr;
	primary_ctrl->imr = 0;
	while (primary_ctrl->irr != 0 && ++spurious < 1000) {
		primary_ctrl->imr = imr;
		irr |= primary_ctrl->irr;
		primary_ctrl->imr = 0;
	}
	/*
	 * Block these interrupts (they will be reset by a call to
	 * aspintsvc() when actually serviced.
	 */
	primary_ctrl->imr = imr & ~irr;

	if (spurious == 1000)
		printf("Spurious ASP interrupt ignored (irr:%x)\n", irr);

	for (; (ibit=ffset(irr)) >= 0; irr &= ~BITSET(ibit)) {
		if (asp2procibit[ibit] == -1)
			printf("Unexpected ASP interrupt on EIR bit %d\n",ibit);
		else
			prochpa->io_eir = asp2procibit[ibit];
	}
}

/*
 * aspintsvc(procibit)
 *
 * Called by Processor External Interrupt dispatch code after servicing
 * an ASP interrupt to reset the temporarily masked ASP interrupt bits.
 * Of course, the processor doesnt know one interrupt from the next, so
 * this routine is called after servicing most external interrupts.  We
 * determine if it was one of ours by checking proc2aspibit[procibit].
 */
void
aspintsvc(int procibit)
{
	register int ibit;
	spl_t s;

	s = splasp();
	ibit = proc2aspibit[procibit];
	if (ibit >= 0)
		primary_ctrl->imr |= BITSET(ibit);
	splx(s);
}

void
viperintr(int i)
{
	/* XXX: What are these?!! ...havent seen one yet */
	printf("Unexpected Viper interrupt\n");
}


/*
 * WAX stuff
 *
 * XXX largely replicates the asp code, should be properly integrated.
 */

/*
 * Map WAX interrupt bits to Processor interrupt bits and back again.
 */
int wax2procibit[EIEM_BITCNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
int proc2waxibit[EIEM_BITCNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

void
waxinit(void)
{
	int i;

	/*
	 * Reset the WAX interrupt bit to processor EIEM mappings.
	 */
	for (i = 0; i < EIEM_BITCNT; i++)
		wax2procibit[i] = proc2waxibit[i] = -1;
}

/*
 * waxitab(mask, spllvl, handler, unit, *ohandler)
 *
 * Given the bit in `mask', add the handler to the WAX Interrupt Table
 * with the specified interrupt priority.  If this routine is called
 * a second time with the same `mask', the previous table entry is
 * quietly replaced with the newer one.  If the handler is NULL, the
 * interrupt table entry is effectively reset.  If `ohandler' is
 * non-NULL, a pointer to the old handler will be stored there.
 *
 * Return value: WAX ibit number on success, -1 on error (+message).
 */
int
waxitab(unsigned int mask, 
	unsigned int spllvl,
	void (*handler)(int),
	int unit)
{
	register int ibit, procibit;
	spl_t s;

	/*
	 * Block WAX interrupts, determine which bit is set in `mask'
	 * and do some sanity checks, call procitab() to set up the
	 * processor interrupt table appropriately and update our
	 * WAX to Processor interrupt bit map.
	 */
	s = splwax();
	if ((ibit = ffset(mask)) >= 0) {
		if (ffset(mask & ~BITSET(ibit)) >= 0) {
			printf("waxitab: invalid mask: %x (handler %x)\n",
			       mask, handler);
			goto bad;
		}
		procibit = procitab(spllvl, handler, unit);
		if (procibit < 0)
			goto bad;

		if (handler) {
			/* XXX routine to call after processing interrupt */
			itab_proc[procibit].arg1 = (int) waxintsvc;
			wax2procibit[ibit] = procibit;
			proc2waxibit[procibit] = ibit;
		} else {
			wax2procibit[ibit] = proc2waxibit[procibit] = -1;
		}
	}

	/*
	 * We were successful; enable or disable this WAX interrupt.
	 */
	if (handler)
		wax_ctrl->imr |= mask;
	else
		wax_ctrl->imr &= ~mask;

	splx(s);
	return(ibit);

bad:
	splx(s);
	return(-1);
}

/*
 * waxintr()
 *
 * Service WAX interrupts.  For each pending WAX interrupt, set it's
 * corresponding bit in the Processor EIEM and let processor dispatch
 * appropriate handler.
 *
 * N.B. It is assumed that this routine will be called at SPLWAX
 * (i.e. WAX interrupts are blocked from reaching the Processor).
 */
void
waxintr(int i)
{
	extern struct iomod *prochpa;
	unsigned int irr, imr;
	int ibit, spurious = 0;

	/*
	 * Prevent spurious interrupts.  A read of the IRR only clears
	 * the *unmasked* bits in the IPR.  When we set the IMR to zero
	 * (after reading the IRR), we must check to see that another
	 * interrupt did not occur.  If an interrupt did sneak in, we
	 * loop on the IRR, switching IMR masks until the IRR reads 0.
	 *
	 * The `spurious' variable can probably be eliminated (sanity).
	 */
	imr = wax_ctrl->imr;
	irr = wax_ctrl->irr;
	wax_ctrl->imr = 0;
	while (wax_ctrl->irr != 0 && ++spurious < 1000) {
		wax_ctrl->imr = imr;
		irr |= wax_ctrl->irr;
		wax_ctrl->imr = 0;
	}

	/*
	 * Block these interrupts (they will be reset by a call to
	 * waxintsvc() when actually serviced.
	 */
	wax_ctrl->imr = imr & ~irr;
	if (spurious == 1000)
		printf("Spurious WAX interrupt ignored (irr:%x)\n", irr);

	for (; (ibit=ffset(irr)) >= 0; irr &= ~BITSET(ibit)) {
		if (wax2procibit[ibit] == -1)
			printf("Unexpected WAX interrupt on EIR bit %d\n",ibit);
		else
			prochpa->io_eir = wax2procibit[ibit];
	}
}

/*
 * waxintsvc(procibit)
 *
 * Called by Processor External Interrupt dispatch code after servicing
 * an WAX interrupt to reset the temporarily masked WAX interrupt bits.
 * Of course, the processor doesnt know one interrupt from the next, so
 * this routine is called after servicing most external interrupts.  We
 * determine if it was one of ours by checking proc2waxibit[procibit].
 */
void
waxintsvc(int procibit)
{
	register int ibit;
	spl_t s;

	s = splwax();
	ibit = proc2waxibit[procibit];
	if (ibit >= 0)
		wax_ctrl->imr |= BITSET(ibit);
	splx(s);
}

#ifdef USELEDS

int inledcontrol = 0;	/* 1 if we are in ledcontrol already, cheap mutex */
int ledbytewide = 0;	/* 1 if LED register is a full byte */
volatile unsigned char *ledptr = NULL;

void
ledinit(void)
{
	if (isgecko) {
		ledptr = (volatile unsigned char*)0xF00E0000;
		ledbytewide = 1;
	}
	else {
		ledptr = &(primary_ctrl->ledctrl);
		ledbytewide = 0;
	}
}

/*
 * Do lights:
 *	`ons' is a mask of LEDs to turn on,
 *	`offs' is a mask of LEDs to turn off,
 *	`togs' is a mask of LEDs to toggle.
 * Note we don't use splclock/splx for mutual exclusion.
 * They are expensive and we really don't need to be that precise.
 * Besides we would like to be able to profile this routine.
 */
void
ledcontrol(int ons, int offs, int togs)
{
	static char currentleds;
	register int leds;
	register int i;

	if(ledptr == NULL)
	  return;

	inledcontrol = 1;
	leds = currentleds;
	if (ons)
		leds |= ons;
	if (offs)
		leds &= ~offs;
	if (togs)
		leds ^= togs;
	currentleds = leds;
	leds = ~leds;
	if (ledbytewide)
		*ledptr = leds;
	else
		for (i = 0; i < 8; i++) {
			if (leds & 0x80) {
				*ledptr = 1;
				*ledptr = 1 | LED_STROBE;
			} else {
				*ledptr = 0;
				*ledptr = 0 | LED_STROBE;
			}
			leds <<= 1;
		}
	inledcontrol = 0;
}
#endif /* USELEDS */

#if      NEISAHA > 0
void
scan_eisabus(void)
{
        register int slot;
        int board_id;
        struct pdc_memmap memmap;
        struct device_path dp;
        struct iomod *io;

        for(slot=0; slot < MAX_EISA_SLOTS;slot++) {
                dp.dp_bc[0] = dp.dp_bc[1] = dp.dp_bc[2] = dp.dp_bc[3] = -1;
                dp.dp_bc[4] = eisaix;
                dp.dp_bc[5] = 0;
                dp.dp_mod = slot;
                if ((*pdc)(PDC_MEM_MAP, PDC_MEM_MAP_HPA, &memmap, &dp) < 0)
                        continue;
		io = (struct iomod *) memmap.hpa;
                if(!(board_id = geteisa_boardid(io, 1)))
                        continue;
                mptr->mt_type.iodc_type=IODC_TP_EISA;
                /* first two bytes of eisa board id is manufacturer id.
                   the probe routine of driver needs to check the full
                   id to verify that we are talking to the right board.
                */
                mptr->mt_type.iodc_sv_model = (board_id & 0xFFFF0000) >> 16;
                mptr->m_fixed = slot;
                mptr->m_hpa = io;
                mptr->m_spa = mptr->m_spasiz = 0;
                mptr->mt_next = mptr + 1;
                mptr++;
        }
        (mptr - 1)->mt_next = 0;
}
#endif
