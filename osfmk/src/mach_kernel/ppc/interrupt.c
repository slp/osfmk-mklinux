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

#include <lan.h>
#include <cpus.h>
#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <mach_mp_debug.h>
#include <debug.h>
#include <mach_debug.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/spl.h>
#include <kgdb/gdb_defs.h>
#include <kgdb/kgdb_defs.h>	/* For kgdb_printf */

#include <ppc/proc_reg.h>
#include <ppc/misc_protos.h>
#include <ppc/trap.h>
#include <ppc/exception.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/serial_io.h>
#include <ppc/POWERMAC/rtclock_entries.h>
#include <ppc/POWERMAC/powermac.h>
#if	NCPUS > 1
#include <ppc/POWERMAC/mp/MPPlugIn.h>
#endif /* NCPUS > 1 */
#if	MACH_KDB
#include <ppc/db_machdep.h>
#endif	/* MACH_KDB */
#include <ppc/POWERMAC/powermac_m2.h>

/* These dummy spl routines are used for initialization values before
 * interrupts are enabled. Interrupts should be at SPLHIGH at startup
 */
static spl_t dummy_spl(spl_t level);
static spl_t dummy_spl(spl_t level) { return SPLHIGH; }

spl_t	(*platform_spl)(spl_t level) = dummy_spl;
spl_t	(*platform_db_spl)(spl_t level) = dummy_spl;

void	(*platform_interrupt)(int type, struct ppc_saved_state *ssp,
			unsigned int dsisr, unsigned int dar);
void	(*platform_interrupt_initialize)(void);
void	(*pmac_register_int)(int device, void (*handler)(int, void *));
void	(*pmac_register_ofint)(int device, void (*handler)(int, void *));


/* 
 * set_priority_level implements spl()
 * The lower the number, the higher the priority. TODO NMGS change?? 
 */

#if MACH_DEBUG
#if DEBUG
struct {
	spl_t level;
	char* name;
} spl_names[] = {
	{ SPLHIGH,	"SPLHIGH" },
	{ SPLCLOCK,	"SPLCLOCK" },
	{ SPLPOWER,     "SPLPOWER" },
	{ SPLVM,	"SPLVM" },
	{ SPLBIO,	"SPLBIO" },
	{ SPLIMP,	"SPLIMP" },
	{ SPLTTY,	"SPLTTY" },
	{ SPLNET,	"SPLNET" },
	{ SPLSCLK,	"SPLSCLK" },
	{ SPLLO,	"SPLLO" },
};

#define SPL_NAMES_COUNT (sizeof (spl_names)/sizeof(spl_names[0]))

static char *spl_name(spl_t lvl);

/* A routine which, given the number of an spl, returns its name.
 * this uses a table set up in spl.h in order to make the association
 */
static char *spl_name(spl_t lvl)
{
	int i;
	for (i = 0; i < SPL_NAMES_COUNT; i++)
		if (spl_names[i].level == lvl)
			return spl_names[i].name;
	printf("UNKNOWN SPL %d",lvl);
	panic("");
	return ("UNKNOWN SPL");
}
#endif /* DEBUG */

#endif /* MACH_DEBUG */

void
interrupt_init(void)
{
	interrupt_disable();

	switch (powermac_info.class) {
	case POWERMAC_CLASS_POWERBOOK:
		platform_interrupt_initialize = m2_interrupt_initialize;
		break;

	case POWERMAC_CLASS_PDM:
		platform_interrupt_initialize = pdm_interrupt_initialize;
		break;
	case POWERMAC_CLASS_PCI:
		if (find_devices("mac-io"))
			platform_interrupt_initialize = heathrow_interrupt_initialize;
		else
			platform_interrupt_initialize = pci_interrupt_initialize;
		break;
	default:
		panic("Unsupported class for interrupt dispatcher\n");
	}		
	(*platform_interrupt_initialize)();
#if 0
	cpu_data[cpu_number()].interrupt_level = SPLLO;
#else
	/*
	 * We want interrupts to remain masked until we decide
	 * it's OK to receive them (probably via a splon() in 
	 * autoconf()...
	 */
	cpu_data[cpu_number()].interrupt_level = SPLHIGH;	/* XXX */
#endif
}


struct ppc_saved_state * interrupt(
        int type,
        struct ppc_saved_state *ssp,
	unsigned int dsisr,
	unsigned int dar)
{
        unsigned int sp, dec;
	int irq;
	spl_t		old_spl = get_interrupt_level();

	cpu_data[cpu_number()].interrupt_level = SPLHIGH;

#if 0 && DEBUG
	{
		/* make sure we're not near to overflowing intstack */
		unsigned int sp;
		static unsigned int spmin = 0xFFFFFFFF;
		__asm__ volatile("mr	%0, 1" : "=r" (sp));
		if (sp < (current_proc_info()->istackptr + PPC_PGBYTES)) {
			printf("INTERRUPT - LOW ON STACK!\n");
#if	MACH_KGDB
			call_kgdb_with_ctx(type, 0, ssp);
#else	/* MACH_KGDB */
#if	MACH_KDB
			kdb_trap(type, 0, ssp);
#endif	/* MACH_KDB */
#endif	/* MACH_KDB */
		}
#if 0
		if (sp < spmin) {
			printf("INTERRUPT - NEW LOW WATER 0x%08x\n",sp);
			spmin = sp;
		}
#endif /* 0 */
	}
#endif /* 0 && DEBUG */

#if	NCPUS > 1
	/* Other processor might have been updating a
	 * pinned or wired PTE, double check there really
	 * was a pte miss and that the pte exists and
	 * is valid when we have its lock
		 */
	if ((type == T_DATA_ACCESS) && 
	    (!USER_MODE(ssp->srr1)) &&
	    (dsisr & MASK(DSISR_HASH)) &&
	    (dar <= VM_MAX_KERNEL_ADDRESS) &&
	    (dar >= VM_MIN_KERNEL_ADDRESS) &&
	    (kvtophys(dar) != 0)) {
		/* return from exception silently */
		goto out;
	}
#endif	/* NCPUS > 1 */

	switch (type) {
	case T_DECREMENTER:
		rtclock_intr(0,ssp, old_spl);
		goto out;

	case T_INTERRUPT:
		break; /* Treat below */
#if	NCPUS > 1

	/*	NOTE: We better have not enabled interruptions yet, the signal parm
	 *	needed by mp_intr() can't be trusted after an enable */
		 
	case T_SIGP:
				/* Did the other processor signal us? */ 
		mp_intr();					/* Yeah, go handle it */
		goto out;					/* Ok, we're done here... */
			
#endif	/* NCPUS > 1 */
	default:
#if	MACH_KGDB
		kgdb_printf("Received illegal interrupt type 0x%x, dar=0x%x, dsisr=0x%x old_spl=%d\n",type,dar,dsisr,old_spl);
#if DEBUG
		regDump(ssp);
#endif /* DEBUG */
		call_kgdb_with_ctx(type, 0, ssp);
#else	/* MACH_KGDB */
#if	MACH_KDB
		kdb_trap(type, 0, ssp);
#endif	/* MACH_KDB */
#endif	/* MACH_KGDB */
		goto out;
	}

	/* Call the platform interrupt routine */
	platform_interrupt(type, ssp, dsisr, dar);
out:
	cpu_data[cpu_number()].interrupt_level = old_spl;
	return ssp;
}

/* Definitions of spl routines for function pointers... */

spl_t (sploff)(void)	{ return sploff(); }
spl_t (splhigh)(void)	{ return splhigh(); }
spl_t (splsched)(void)	{ return splsched(); }
spl_t (splclock)(void)	{ return splclock(); }
spl_t (splvm)(void)	{ return splvm(); }
spl_t (splbio)(void)	{ return splbio(); }
spl_t (splimp)(void)	{ return splimp(); }
spl_t (spltty)(void)	{ return spltty(); }
spl_t (splnet)(void)	{ return splnet(); }
void (spllo)(void)	{ spllo(); return; }
void (splx)(spl_t l)	{ splx(l); return; }
void (splon)(spl_t l)	{ splon(l); return; }
