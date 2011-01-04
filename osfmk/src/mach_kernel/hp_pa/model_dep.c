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
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: model_dep.c 1.34 94/12/14$
 */

#include <kgdb.h>
#include <dipc.h>
#include <kernel_test.h>
#include <fast_idle.h>
#include <dipc_xkern.h>

#include <kern/thread.h>
#include <machine/pmap.h>
#include <machine/mach_param.h>
#include <machine/regs.h>
#include <machine/psw.h>
#include <machine/break.h>
#include <device/param.h>
#include <mach/vm_param.h>
#include <mach/machine.h>
#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/spl.h>
#include <machine/rpb.h>
#include <hp_pa/boot.h>

#include <device/ds_routines.h>
#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/c_support.h>
#include <kern/clock.h>
#include <machine/trap.h>
#include <kern/spl.h>

#if	DIPC
#include <dipc/dipc_funcs.h>
#endif	/* DIPC */
#if KGDB
#include <kgdb_lan.h>
#include <kgdb/kgdb.h>
#endif

int boothowto = 0;
int pdc_boot = 0;
int halt_in_debugger = 0;


void parse_args(void);

char	boot_string[BOOT_LINE_LENGTH] = {0};
int	boot_string_sz = BOOT_LINE_LENGTH;

void
machine_conf(void)
{
	extern int cpusubtype;

	/* XXX - just hacks up the table, doesn't really look at anything...*/
	machine_slot[0].is_cpu = TRUE;
	machine_slot[0].running = TRUE;

	machine_slot[0].cpu_type = CPU_TYPE_HPPA;
	machine_slot[0].cpu_subtype = cpusubtype;

	machine_info.max_cpus = NCPUS;
	machine_info.avail_cpus = 1;
	machine_info.memory_size = mem_size;
}

#ifdef PDCDUMP
/*
 * Really bogus dumpconf! We do this using a name, but we should eventually
 * change it to use config and the device number.
 * NOTE: savecore needs dumpdev, dumplo, dumpsize, and dumpmag.
 */

dev_t dumpdev = 0;
int   dumplo = 0;	    /* DEV_BSIZE offset from start of dump partition.*/
int   dumpsize = 0;	    /* NBPG size of dump. */
int   dumpoffset = -1;	    /* DEV_BSIZE offset from start of disk */
int   dumpmag = 0x8fca0101; /* Magic number for savecore */


#include <device/conf.h>
#include <machine/machparam.h>
#include "sd.h"
dev_ops_t dumpdev_ops;

void
getdumpdev(void)
{
	int		dev_minor;
	extern char 	*dump_name;

	/*
	 * Get the device and unit number from the name.
	 */
	if (!dev_name_lookup(dump_name, &dumpdev_ops, &dev_minor)) {
		printf("Can't find dump device entry: %s\n", dump_name);
		return;
	}
	dumpdev = dev_minor;
	dumpsize = btoc(mem_size);

}
#endif

void
configure(void)
{
	parse_args();
	machine_conf();
#ifdef PDCDUMP
	getdumpdev();
#endif
	autoconf();
}

void
machine_init(void)
{
	/*
	 * still needs a bit of work here :-)
	 * wait til you see configure...
	 */
	procitab(SPLPOWER, powerfail, 0);

	configure();
	get_root_device();
	clock_config();
}

#include <sys/ioctl.h>
#include <device/device_types.h>
#include <device/disk_status.h>

#ifdef PDCDUMP
void
dumpconf(void)
{
	struct partinfo pi;
	
	/*
	 * The device should already be opened, and the dump info setup,
	 * but savecore needs a bit more.
	 */
	(*dumpdev_ops->d_setstat)(dumpdev, DIOCGPART, (int *)&pi, sizeof(pi));
	dumplo = pi.part->p_size - ctod(dumpsize);

	printf("dumpconf: size = %d blocks, *disk* offset = %d blocks\n",
	       ctod(dumpsize), dumpoffset);

	if (dumpsize != btoc(mem_size))
		printf("\tWARNING: Dump partition is smaller than mem size\n");
}
#endif

#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <mach/time_value.h>
#include <machine/machparam.h>	/* for btop */
#include <machine/pdc.h>

/*
 * Universal (Posix) time map. This should go away when the
 * kern/posixtime.c routine is removed from the kernel.
 */
vm_offset_t
utime_map(
	dev_t			dev,
	vm_offset_t		off,
	int			prot)
{
	extern time_value_t *mtime;

	if (prot & VM_PROT_WRITE)
		return (-1);

	return (btop(pmap_extract(pmap_kernel(), (vm_offset_t) mtime)));
}

/*
 * XXX - Put this here until we get it into movc.s
 */

void
ovbcopy(
	const char *src, 
	register char *dst,
	vm_size_t len)
{
	if (len < 0)
		return;

	/* 
	 * use the lightning fast bcopy() if at all possible 
	 */

	if (src > dst || dst > src+len)
		bcopy(src, dst, len);
	else {
		/* 
		 * use our tremendously slow backward byte copy 
		 */

		dst += (len - 1);
		src += (len - 1);
		while (len-- > 0)
			*dst-- = *src--;
	}

	return;
}

int dofastboot = 0;		/* if set, always do a fastboot() */
int dotocdump = 1;		/* if set, do a crash dump on TOC's */
struct rpb rpb;			/* Used by doadump and crashdump */

void
halt_all_cpus(boolean_t	reboot)
{
	if(reboot)
	{
		printf("Rebooting MACH...\n");
		(*(struct iomod *)LBCAST_ADDR).io_command = CMD_RESET;
	}
	else
	{
		printf("CPU halted\n");
		(void) spllo();
		(*(struct iomod *)LBCAST_ADDR).io_command = CMD_STOP;
	} 
	while(1);
}

extern char  *panicstr;
extern int   dumplo, dumpsize, dumpoffset;
extern dev_t dumpdev;

/*
 *  dopanic() will screw up the stack in such a way that adb will be
 *  unable to show us a stack back trace, so we do one here.  A trace
 *  of the current stack is stored in `stktrc', with the number of
 *  entries in `stkcnt'.
 */

#define	STKSIZ	64

struct {
	u_int	pc;
	u_int   fp;
	u_int	ssp;
} stktrc[STKSIZ] = { 0 };

u_int	stkcnt = 0;

#define	strippriv(pc)	(int *)((u_int)(pc) & ~3)	/* no priv bits */

void
savestk(void)
{
	register int *pc, *fp;				/* current pc & sp */
	struct hp700_saved_state *ssp;
#if 1
	int *high, *low;
#endif

	if (stkcnt >= STKSIZ)
		return;

	pc = strippriv(getpc());
	fp = strippriv(getfp());
#if 1
	high = fp;
	low = (int *)(trunc_page(fp) - 2 * PAGE_SIZE);
#endif

	stktrc[stkcnt].pc   = (u_int)pc;
	stktrc[stkcnt++].fp = (u_int)fp;
	while (stkcnt < STKSIZ) {
#if 1
		/*
		 * Don't switch stacks
		 */
		if (fp <= low || fp > high)
			break;
#else
		if (fp == 0) {
			/*
			 * Look at the saved state, and see if we can keep
			 * going by switching to another kernel stack.
			 */
			fp = (int *)stktrc[stkcnt-2].fp;
			ssp = (struct hp700_saved_state *) *(fp - 3);;
			if ((ssp->flags & SS_PSPKERNEL) == 0) {
				printf("savestk: no prev context (%x)\n", fp);
				break;
			}
			pc = strippriv(ssp->iioq_head);
			fp = (int *)ssp->r4;
			stktrc[stkcnt].ssp = (u_int)ssp;
		}
#endif
		else {
			/*
			 * Find the previous frame and return pointer.
			 */
			pc = strippriv(*(fp - 5));
			fp = (int *) *fp;
		}
		stktrc[stkcnt].pc   = (u_int)pc;
		stktrc[stkcnt++].fp = (u_int)fp;
	}
}

/*
 *  Display `stkcnt' elements of `stktrc' on the console.
 */

void
dispstk(void)
{
	static char hex[] = "0123456789abcdef";
	register char *cp1, *cp2;
	register int i, j;
	char stkstr[9];

	printf("\nStack Trace (depth=%d):", stkcnt);

	for (i = 0; i < stkcnt; i++) {

		/* format ethernet address into stkstr[] */
		cp1 = (char *)&stktrc[i].pc;
		cp2 = stkstr;
		for (j = 0; j < 4; j++) {
			*cp2++ = hex[*cp1 >> 4 & 0xf];
			*cp2++ = hex[*cp1++ & 0xf];
		}
		*cp2 = '\0';

		/* and splat it to the console */

#if 0
		say_symbol(stktrc[i].pc);
		printf("\n");
#else
		printf("%s\t0x%s", (i%4)? "": "\n", stkstr);
#endif
	}

	printf("\nEnd of Stack Trace\n");

	/* wait a couple seconds for messages to get out */
	delay(2000000);
}


void
halt_cpu(void)
{
        static int paniced = 0;
#if  KGDB
#if  KGDB_LAN
	kgdb_break();
#else
	{
		extern int kgdb_dev;

                if (kgdb_dev != -1) {
                        delay(1000000);
                        kgdb_break();
                }
	}
#endif
#endif	/* KGDB */
        if (panicstr && !paniced) {
                paniced = 1;
                savestk();
                dispstk();
        }
        halt_all_cpus(0);
}

void
dumpsys(void)
{
#ifdef PDCDUMP
	if (dumpoffset < 0) {
		printf("Sorry, not enough room on dumpdev for dump\n");
		return;
	}
	printf("\nPDC dumping %d blocks to device ", ctod(dumpsize));
	printf("0x%x at *disk* offset %d blocks\n", dumpdev, dumpoffset);
	doadump();
	/*NOTREACHED*/		
#endif
}

/*
 * Purge TLBs.
 * This is the architected algorithm.
 */
void
ptlball(struct pdc_cache *pdc_cache)
{
        register space_t sp;
        register caddr_t off;
	register int six, oix, lix;
	int sixend, oixend, lixend;

	/* instruction TLB */
        sixend = pdc_cache->it_sp_count;
        oixend = pdc_cache->it_off_count;
        lixend = pdc_cache->it_loop;
	sp = pdc_cache->it_sp_base;
	for (six = 0; six < sixend; six++) {
		off = (caddr_t) pdc_cache->it_off_base;
		for (oix = 0; oix < oixend; oix++) {
			for (lix = 0; lix < lixend; lix++)
				pitlbentry((vm_offset_t)sp, (vm_offset_t)off);
			off += pdc_cache->it_off_stride;
		}
		sp += pdc_cache->it_sp_stride;
	}

	/* data TLB */
        sixend = pdc_cache->dt_sp_count;
        oixend = pdc_cache->dt_off_count;
        lixend = pdc_cache->dt_loop;
	sp = pdc_cache->dt_sp_base;
	for (six = 0; six < sixend; six++) {
		off = (caddr_t) pdc_cache->dt_off_base;
		for (oix = 0; oix < oixend; oix++) {
			for (lix = 0; lix < lixend; lix++)
				pdtlbentry((vm_offset_t)sp, (vm_offset_t)off);
			off += pdc_cache->dt_off_stride;
		}
		sp += pdc_cache->dt_sp_stride;
	}
}

int itick_per_tick = 0;
u_int itick_per_n_usec = 0;
u_int n_usec = 0;

void
delay(int usec)
{
	u_long start, end;
	u_long max_n_usec, howlong;

	/* max # of `itimer_n_usec's before 32-bit register overflows */
	max_n_usec = (unsigned) 2000000000/itick_per_n_usec;	/* ~2^31 */

	do {
		/* determine how long to wait during this iteration */
		howlong = (usec > max_n_usec)? max_n_usec: usec;

		start = (u_long)mfctl(CR_ITMR);
		end = start + ((itick_per_n_usec * howlong) / n_usec);

		/* N.B. Interval Timer may wrap around */
		if (end < start)
			while (mfctl(CR_ITMR) > end)
				;
		while (mfctl(CR_ITMR) < end)
			;

		usec -= howlong;
	} while (usec);
}


/*
 * XXX: Stolen from HPUX.
 *
 * Pick good values of itick_per_n_usec and n_usec so that 
 * itick_per_n_usec/n_usec is a good approximation to the number
 * of iticks (determined by processor frequency) in a microsecond.
 * Previous use of 12 to approximate 12.5 caused gettimeofday() to
 * run backwards occasionally!
 * The numbers must be fairly small so that calculations with them
 * (e.g. converting iticks to usecs) won't cause integer overflow.
 */

void
delay_init(void)
{
    int min_err = 2000000;	/* a big initial value */
    int denom, num;
    int error;			/* current error */
    int max_denom;
    int	tick = 1000000 / HZ;

    itick_per_tick = (PAGE0->mem_10msec * 100) / HZ;

    /* Make sure that our denom won't cause overflow */
    max_denom = MIN(1000, 1000000000/itick_per_tick);

    /*
     * Compute good values of itick_per_n_usec and n_usec, such
     * that itick_per_n_usec/n_usec equals or is slightly greater than
     * itick_per_tick/10000 .  If we allowed it to be less than the
     * correct value, conversions from ticks to usecs would be slightly
     * high, and time might run backwards across 10ms ticks.  Catching
     * up slightly is preferable to that possibility.
     * 
     * The values produced by this code should produce conversions
     * accurate to about 1usec up to clock frequencies of 100MHz,
     * and smoothly degrade beyond that point.  One billion is used
     * below instead of maxint to allow for the possibility of a lost
     * clock tick.  In the worst case (denom = 1000), it would correctly
     * handle periods of up to 21.47 ms since the last tick.
     * 
     * Tick is defined to be 1000000/HZ, or the number of usecs per tick.
     * 
     */
    n_usec = 1;			/* must have an initial value */
    for (denom = 1; denom <= max_denom; denom++) {
	/* Given denom, compute corresponding value of num (Round up). */
	num = ((denom*itick_per_tick) + (tick -1)) / tick;
	error = num * tick - itick_per_tick * denom;
	if (error == 0) {	    /* Exact result; use it. */
	    n_usec = denom;
	    itick_per_n_usec = num;
	    break;
	}

	if (error < 0) {
	    panic("set_itick_per_n_usec: arithmetic doesn't work!");
	}

	/* Rounding up earlier guaranteed that error >= 0 . */
	/* Find smallest percent error, i.e. error/denom < min_err/n_usec */
	if (error * n_usec < min_err * denom) {
		min_err = error;
		n_usec = denom;
		itick_per_n_usec = num;
	}
    }
}

/*XXX*/
void fc_get(tvalspec_t *ts);
#include <kern/clock.h>
#include <hp_pa/rtclock_entries.h>
void fc_get(tvalspec_t *ts) {
	(void )rtc_gettime(ts);
}

#if	DIPC
boolean_t
no_bootstrap_task(void)
{
	return(FALSE);
}

ipc_port_t
get_root_master_device_port(void)
{
	return(IP_NULL); /* for now */
}

void
dipc_machine_init(void)
{
#if	FAST_IDLE
	extern fast_idle_enabled;
#endif	/* FAST_IDLE */
#if	KERNEL_TEST
	extern int kkt_test_iterations;
#endif	/* KERNEL_TEST */

#if	FAST_IDLE
	fast_idle_enabled = TRUE;
#endif	/* FAST_IDLE */
#if	KERNEL_TEST
#if	DIPC_XKERN
	kkt_test_iterations = 1;
#else	/* DIPC_XKERN */
	kkt_test_iterations = 10;
#endif	/* DIPC_XKERN */

#endif	/* KERNEL_TEST */
}

#endif	/* DIPC */

void
kern_print(
	int type,
	struct hp700_saved_state *ssp)
{
	char buffer[1024];

	copyin((const char*)ssp->arg0, buffer, ssp->arg1);
	printf("%s", buffer);

	/*
	 * move the pc past the break instruction
	 */
	
	ssp->iioq_head = ssp->iioq_tail;
	ssp->iioq_tail = ssp->iioq_head + 4;
}

#if	KGDB
int 	loop_in_Debugger = FALSE;
#endif	/* KGDB */

void
Debugger(
	const char	*message)
{
#if	KGDB
	while (1) {
		kgdb_break();
		if (!loop_in_Debugger)
			break;
		while (loop_in_Debugger);
	}
#else	/* KGDB */
	printf("Debugger(%s): KGDB not configured\n", message);
	while (1)
		continue;
#endif	/* KGDB */
}

void parse_args(void)
{
	char *btstring;

	for(btstring = boot_string; *btstring; btstring++)
	{
		switch(*btstring)
		{
		case 'a':
			boothowto |= RB_ASKNAME;
			break;
		case 's':
			boothowto |= RB_SINGLE;
			break;
		case 'd':
			halt_in_debugger = 1;
#if KGDB
			boothowto |= RB_KDB;
#else
			printf("This versions was not build with kgdb.\n");
#endif 
			break;
		case 'k':
			/* kernel loaded server, handled by bootstrap */
			break;
		case 'u':
			/* user loaded server, handled by bootstrap */
			break;			
		default:
			printf("Unknown kernel option %c\n", *btstring);
		}
	}
}


#if	MACH_ASSERT

/*
 * Machine-dependent routine to fill in an array with up to callstack_max
 * levels of return pc information.
 */
void machine_callstack(
	natural_t		*buf,
	vm_size_t		callstack_max)
{
}

#endif	/* MACH_ASSERT */
