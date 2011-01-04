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
 * Copyright (c) 1990,1991,1992,1993,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
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
 * 	Utah $Hdr: hp800_init.c 1.35 94/12/15$
 */

/*
 * Low machine initialization routine.
 *
 * Some boot mechanism brought the kernel into physical memory and transfered
 * control to a low-level routine in locore. This routine then set up a 
 * stack pointer and a global data pointer for this routine.
 * 
 * This routine is running in real mode, interrupts are off and the cache
 * is consistent. We need to set up the VM system and perform other tasks 
 * before going to virtual mode.
 * 
 * This routine returns the page number of the next available physical address.
 */

#include <kgdb.h>
#include <machine/pmap.h>

#include <mach/vm_param.h>
#include <machine/pdc.h>
#include <machine/regs.h>
#include <machine/thread.h>
#include <mach/vm_prot.h>
#include <mach/machine.h>
#include <machine/cpu.h>
#include <machine/iodc.h>
#include <kern/etap_macros.h>

#include <vm/vm_page.h>
#include <kern/misc_protos.h>
#include <mach/thread_status.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/c_support.h>
#include <hp_pa/boot.h>
#include <hp_pa/HP700/bus_protos.h>

/*
 * Forwards.
 */
void hp700_init(int argc, char *argv[], char *envp[]);
void set_bootstrap_args(void);

/* Interrupt stack pointer; zero while in interrupts */
vm_offset_t istackptr = 0;

/*
 * Random variables that have to go into the data segment of the kernel
 */
int machtype = 0;
int cputype = 0;
int cpusubtype = 0;
double fpu_zero = 0.0;
int copr_sfu_config = 0;
int fpcopr_version = 0;

struct {
	struct hp700_saved_state st;
	int extra[16];
} tmp_saved_state = { 0 };

/*
 * memory limits
 */
extern unsigned int mem_size;		/* from busconf */
extern unsigned int io_size;		/* from busconf */

int vm_on = 0;
vm_offset_t avail_start = 0;
vm_offset_t avail_end = 0;
vm_offset_t virtual_avail = 0;
vm_offset_t virtual_end = 0;
vm_offset_t first_avail = 0;

vm_offset_t text_start = 0;
vm_offset_t text_end = 0; 

extern long equiv_end;
extern long io_end;

extern vm_offset_t intstack_top;

/*
 * pointers in kernel load image
 */
extern char etext;
extern char edata;
extern char end;
extern void start_text(void);

vm_offset_t move_bootstrap(void);
void ovbcopy_ints(vm_offset_t, vm_offset_t, vm_size_t);

/*
 * location of hpmc trap vector code in locore.s. Must look like a function
 * so the linker knows that it's in $TEXT$
 */
extern void i_hpmach_chk(void);
int (*pdc)(int, ...) = (int (*)(int, ...))0;

/*
 * Size of instruction and data cache lines in this machine. 
 */
int dcache_line_size = 0;
int dcache_block_size = 0;
int dcache_line_mask = 0;
int dcache_size   = 0;
int dcache_base   = 0;
int dcache_stride = 0;
int dcache_count  = 0;
int dcache_loop   = 0;

int icache_line_size = 0;
int icache_block_size = 0;
int icache_line_mask = 0;
int icache_base   = 0;
int icache_stride = 0;
int icache_count  = 0;
int icache_loop   = 0;

struct	iodc_data cons_iodc = { 0 };
struct	device_path cons_dp = { 0 };

struct	iodc_data boot_iodc = { 0 };
struct	device_path boot_dp = { 0 };

extern char 	version[];
extern char 	boot_string[];

/* 
 * arguments are given from isl as:
 * argv[1] = boot_info
 * argv[2] = boot init string
 * argv[3] = btflags
 * argv[4] = 0
 */

void
hp700_init(int argc, char *argv[], char *envp[])
{
	int hpmc_br_instr;
	int *p = (int *) i_hpmach_chk;
	register struct mapping *mp;
	int i;
	vm_offset_t addr;
	int pdcerr;
	vm_offset_t first_page;

	struct pdc_coproc pdc_coproc;
	struct pdc_cache pdc_cache;
	struct pdc_model pdc_model;
	struct pdc_iodc_read pdc_iodc;
	extern int crashdump(void);
#ifdef	BTLB
	struct pdc_btlb pdc_btlb;
#endif
#ifdef	HPT
	struct pdc_hwtlb pdc_hwtlb;
	extern struct hpt_entry *hpt_table;
	extern int usehpt;
#endif	

	first_page = move_bootstrap();

	if (argc >= 1 && argc <= 4) {
		char *btstring = boot_string;
		char *src = (argc == 1 ? envp[5] : argv[2]);

		i = 0;
		while (*src != '\0' && i++ <= BOOT_LINE_LENGTH)
			*btstring++ = *src++;
		*btstring = '\0';
	}

	pdc = PAGE0->mem_pdc;

	delay_init();
	pdc_console_init();

	printf("%s", version);

	/*
	 * Determine what the boot program is using as its console
	 * so that we can use the same device.
	 */
	pdcerr = (*pdc)(PDC_IODC, PDC_IODC_READ, &pdc_iodc,
			PAGE0->mem_cons.pz_hpa, PDC_IODC_INDEX_DATA,
			&cons_iodc, sizeof(cons_iodc));
	if (pdcerr == 0)
		bcopy((char *)&PAGE0->mem_cons.pz_dp, (char *)&cons_dp,
		      sizeof(struct device_path));
	else
		printf("Warning: can't id console boot device (PDC Ret'd %d)\n",
		       pdcerr);

        /*
         * Read boot device from PROM
         */
	pdcerr = (*PAGE0->mem_pdc)(PDC_IODC, PDC_IODC_READ, &pdc_iodc,
	                           PAGE0->mem_boot.pz_hpa, PDC_IODC_INDEX_DATA,
	                           &boot_iodc, sizeof(boot_iodc));
	if (pdcerr == 0)
		bcopy((char *)&PAGE0->mem_boot.pz_dp, (char *)&boot_dp,
		      sizeof(struct device_path));
	else
		printf("Warning: can't id boot device (PDC Ret'd %d)\n",
		       pdcerr);
	
	/*
	 * Setup the transfer of control addr to point to the crash dump
	 * initialization code.
	 */
	PAGE0->ivec_toc = crashdump;

	/*
	 * get cache parameters from the PDC
	 */
	(*PAGE0->mem_pdc)(PDC_CACHE, PDC_CACHE_DFLT, &pdc_cache);

	dcache_line_size = pdc_cache.dc_conf.cc_line * 16;
	dcache_line_mask = dcache_line_size - 1;
	dcache_block_size = dcache_line_size * pdc_cache.dc_conf.cc_block;

	dcache_size = pdc_cache.dc_size;
	dcache_base = pdc_cache.dc_base;
	dcache_stride = pdc_cache.dc_stride;
	dcache_count = pdc_cache.dc_count;
	dcache_loop = pdc_cache.dc_loop;

	icache_line_size = pdc_cache.ic_conf.cc_line * 16;
	icache_line_mask = icache_line_size - 1;
	icache_block_size = icache_line_size * pdc_cache.ic_conf.cc_block;

	icache_base = pdc_cache.ic_base;
	icache_stride = pdc_cache.ic_stride;
	icache_count = pdc_cache.ic_count;
	icache_loop = pdc_cache.ic_loop;

	/*
	 * purge TLBs and flush caches
	 */
	ptlball(&pdc_cache);

#ifdef	BTLB
        /*
         * get block tlb information for clearing
         */
	pdcerr = (*pdc)(PDC_BLOCK_TLB, PDC_BTLB_DEFAULT, &pdc_btlb);
	
        if (pdcerr != 0)
                printf("Warning: PDC_BTLB call Ret'd %d\n", pdcerr);

	switch (pdc_btlb.finfo.num_c) {
	/* S-Chip specific */
	case 0: 
		cputype = CPU_PCXS;
		for (i = 0; i < pdc_btlb.finfo.num_i; i++)
			purge_block_itlb(i);
		for (i = 0; i < pdc_btlb.finfo.num_d; i++)
			purge_block_dtlb(i);
		break;
	/* L-Chip specific */
	case 8:
		cputype = CPU_PCXL;
		for (i = 0; i < pdc_btlb.finfo.num_c; i++)
			purge_L_block_ctlb(i);
		break;
	/* T-Chip specific */
	case 16:
		cputype = CPU_PCXT;
		for (i = 0; i < pdc_btlb.finfo.num_c; i++)
			purge_block_ctlb(i);
		break;
	default:
		panic("unrecognized block-TLB, cannot purge block TLB(s)");
		/* NOTREACHED */
	}
#endif

	fcacheall();

	/*
	 * get the cpu type
	 */
	(*PAGE0->mem_pdc)(PDC_MODEL, PDC_MODEL_INFO, &pdc_model);

	machtype = pdc_model.hvers >> 4;

	cpuinfo(&pdc_cache);

	if (dcache_line_size != CACHE_LINE_SIZE)
		printf("WARNING: data cache line size = %d bytes, %s\n",
		       dcache_line_size, "THIS IS *VERY* BAD!");

	/*
	 * Get the instruction to do branch to PDC_HPMC from PDC.  If
	 * successful, then insert the instruction at the beginning
	 * of the HPMC handler.
	 */
	if ((*PAGE0->mem_pdc)(PDC_INSTR, PDC_INSTR_DFLT, &hpmc_br_instr) == 0)
		p[0] = hpmc_br_instr;
	else
		p[0] = 0;

	/* 
	 * Now compute the checksum of the hpmc interrupt vector entry
	 */
	p[5] = -(p[0] + p[1] + p[2] + p[3] + p[4] + p[6] + p[7]);

	/*
	 * setup page size for Mach
	 */
	page_size = HP700_PGBYTES;
	vm_set_page_size();

	/*
	 * configure the devices including memory. Passes back size of 
	 * physical memory in mem_size.
	 */
	busconf();

	/* 
	 * Zero out BSS of kernel before doing anything else. The location
	 * pointed to by &edata is included in the data section.
	 */
	bzero((char*)((vm_offset_t) &edata + 4), (vm_offset_t) &end - 
	      (vm_offset_t) &edata - 4);

        /*
         * Locate any coprocessors and enable them by setting up the CCR.
         * SFU's are ignored (since we dont have any).  Also, initialize
         * the floating point registers here.
         */
        if ((pdcerr = (*pdc)(PDC_COPROC, PDC_COPROC_DFLT, &pdc_coproc)) < 0)
                printf("Warning: PDC_COPROC call Ret'd %d\n", pdcerr);
        copr_sfu_config = pdc_coproc.ccr_enable;
        mtctl(CR_CCR, copr_sfu_config & CCR_MASK);
        fprinit(&fpcopr_version);
	fpcopr_version = (fpcopr_version & 0x003ff800) >> 11;
        mtctl(CR_CCR, 0);

        /*
         * Clear the FAULT light (so we know when we get a real one)
         * PDC_COPROC apparently turns it on (for whatever reason).
         */
        pdcerr = PDC_OSTAT(PDC_OSTAT_RUN) | 0xCEC0;
        (void) (*pdc)(PDC_CHASSIS, PDC_CHASSIS_DISP, pdcerr);

#ifdef TIMEX
	/*
	 * Enable the quad-store instruction.
	 */
	pdcerr = (*pdc)(PDC_MODEL, PDC_MODEL_ENSPEC,
			&pdc_model, pdc_model.pot_key);
	if (pdcerr < 0)
		printf("Warning: PDC enable FP quad-store Ret'd %d\n", pdcerr);
#endif


	/*
	 * Intialize the Event Trace Analysis Package
	 * Static Phase: 1 of 2
	 */
	etap_init_phase1();

	/*
	 * on the hp700 the value in &etext is a pointer to the last word
	 * in the text section. Similarly &edata and &end are pointers to
	 * the last words in the section. We want to change this so that 
	 * these pointers point past the sections that they terminate.
	 */
	text_start = trunc_page((vm_offset_t) &start_text);
	text_end = round_page((vm_offset_t) &etext + 4);

	/*
	 * before we go to all the work to initialize the VM see if we really 
	 * linked the image past the end of the PDC/IODC area.
	 */
	if (text_start < 0x10800)
		panic("kernel text mapped over PDC and IODC memory");

	/*
	 * find ranges of physical memory that isn't allocated to the kernel
	 */

	avail_start = round_page(first_page);
	first_avail = avail_start;
	avail_end = trunc_page(mem_size);
	
	/*
	 * bootstrap the rest of the virtual memory system
	 */
#ifdef MAXMEMBYTES
	if ((avail_end - avail_start) > MAXMEMBYTES) {
		mem_size  = trunc_page(MAXMEMBYTES);
		avail_end = mem_size;
	}
#endif

#ifdef HPT
	/*
	 * If we want to use the HW TLB support, ensure that it exists.
	 */
	if (usehpt &&
	    !((*pdc)(PDC_TLB, PDC_TLB_INFO, &pdc_hwtlb) == 0 &&
	      (pdc_hwtlb.min_size || pdc_hwtlb.max_size)))
		usehpt = 0;
#endif

	pmap_bootstrap(&avail_start, &avail_end);

	/*
	 * set limits on virtual memory and kernel equivalenced memory
	 */
	virtual_avail = avail_end;
	virtual_end = trunc_page(VM_MAX_KERNEL_ADDRESS);

	/*
	 * pmap_bootstrap allocated memory for data structures that must
	 * be equivalently mapped.
	 */
	equiv_end = (long) round_page((vm_offset_t) &end);
	io_end = 0xF0000000;	/* XXX */

	/*
	 * Do block mapping. We are mapping from 0, up through the first
	 * power of 2 address above the end of the equiv region. This 
	 * means some memory gets block mapped that should not be, but
	 * so be it (we make the text writable also :-)). We do this to
	 * conserve block entries since we hope to use them for other
	 * purposes (someday).
	 */
	addr = avail_start;
	if (addr != 1 << log2(addr))
		addr = 1 << log2(addr);

#ifdef	BTLB
	if(pdc_btlb.finfo.num_c)
		printf("%d BTLB entries found.  Block mapping up to 0x%x (0x%x)\n",
		       pdc_btlb.finfo.num_c, addr, avail_start);

	/*
	 * XXX L-CHIP vs T-CHIP vs S-CHIP difference in Block TLB insertion.
	 */
	switch (pdc_btlb.finfo.num_c) {
	/* S-CHIP */
	case 0:
		pmap_block_map(0, addr, VM_PROT_ALL, 0, BLK_ICACHE);
		pmap_block_map(0, addr, VM_PROT_READ|VM_PROT_WRITE,
			       0, BLK_DCACHE);
		break;
	/* L-CHIP */
	case 8:
		pmap_block_map(0, addr, VM_PROT_ALL, 0, BLK_LCOMBINED);
		break;
	/* T-CHIP */
	case 16:
		pmap_block_map(0, addr, VM_PROT_ALL, 0, BLK_COMBINED);
		break;
	default:
		panic("unrecognized block-TLB, cannot map kernel");
		/* NOTREACHED */
	}
#endif

#ifdef	HPT
	/*
	 * Turn on the HW TLB assist.
	 */
	if (usehpt) {
		pdcerr = (*pdc)(PDC_TLB, PDC_TLB_CONFIG,
				&pdc_hwtlb, hpt_table,
				sizeof(struct hpt_entry) * HP700_HASHSIZE,
				PDC_TLB_WORD3);
		if (pdcerr) {
			printf("Warning: HW TLB init failed (%d), disabled\n",
			       pdcerr);
			usehpt = 0;
		} else
			printf("HW TLB initialized (%d entries at 0x%x)\n",
			       HP700_HASHSIZE, hpt_table);
	}
#endif

	/*
	 * map the PDC and IODC area for kernel read/write
	 * XXX - should this be read only?
	 */
	(void) pmap_map(0, 0, text_start, VM_PROT_READ | VM_PROT_WRITE);

	/*
	 * map the kernel text area.
	 */
#if KGDB
	(void) pmap_map(text_start, text_start, text_end, 
			VM_PROT_READ | VM_PROT_EXECUTE | VM_PROT_WRITE);
#else
	(void) pmap_map(text_start, text_start, text_end, 
			VM_PROT_READ | VM_PROT_EXECUTE);
#endif

	/*
	 * map the data section of the kernel
	 */
	(void) pmap_map(text_end, text_end, avail_start,
			VM_PROT_READ | VM_PROT_WRITE);

#ifndef IO_HACK
	/*
	 * map the I/O pages
	 */
	(void) pmap_map(trunc_page(io_size), trunc_page(io_size),
			0, VM_PROT_READ | VM_PROT_WRITE);
#endif

#if 0
	/*
	 * map the breakpoint page
	 */
	(void) pmap_map(break_page, break_page, break_page+HP700_PAGE_SIZE,
			VM_PROT_READ | VM_PROT_EXECUTE);
#endif

	/*
	 * map the interrupt stack red zone.
	 */
	addr = trunc_page((vm_offset_t) &intstack_top);
	(void) pmap_map(addr, addr, addr + PAGE_SIZE, VM_PROT_READ);

	vm_on = 1;
}

void
cpuinfo(struct pdc_cache *cp)
{
	extern unsigned int itick_per_n_usec, n_usec;
	register int mhzint;		/* integer part of clock speed */
	register char *mhzfra;		/* fractional part of clock speed */

	/*
	 * Determine our clock speed in a machine-independent way.
	 * We carry it out to 3 decimal places and round up to 2.
	 * At the end of this block, `mhzint' and `mhzfra' are set.
	 */
	{
		register char *nums, *cpfra;
		register int nindx;

		nums = "0123456789";
		mhzfra = cpfra = ".00";
		if ((mhzint = ((itick_per_n_usec * 1000) /
			       n_usec) % 1000 + 5) > 9) {
			cpfra++;
			*cpfra++ = nums[(mhzint / 100) % 10];
			if ((nindx = (mhzint / 10) % 10) != 0)
				*cpfra++ = nums[nindx];
		}
		*cpfra = '\0';
		mhzint = itick_per_n_usec / n_usec;
	}

	printf("HP9000/");
	switch (machtype) {
	case CPU_M705:
		cpusubtype = CPU_SUBTYPE_HPPA_705;
		printf("705 (%d%sMhz PA-RISC", mhzint, mhzfra);
		break;

	case CPU_M710:
		cpusubtype = CPU_SUBTYPE_HPPA_710;
		printf("710 (%d%sMhz PA-RISC", mhzint, mhzfra);
		break;

	case CPU_M712_60:
	case CPU_M712_80:
	case CPU_M712_3:
	case CPU_M712_4:
		cpusubtype = CPU_SUBTYPE_HPPA_712;
		printf("712 (%d%sMhz PA-RISC", mhzint, mhzfra);
		break;

        case CPU_M715_33:
        case CPU_M715_50:
        case CPU_M715T_50:
        case CPU_M715S_50:
        case CPU_M715_64:
        case CPU_M715_75:
	case CPU_M715_100:
		cpusubtype = CPU_SUBTYPE_HPPA_715;
		printf("715 (%d%sMhz PA-RISC", mhzint, mhzfra);
		break;

	case CPU_M720:
		cpusubtype = CPU_SUBTYPE_HPPA_720;
		printf("720 (%d%sMhz PA-RISC", mhzint, mhzfra);
		break;

	case CPU_M730:
		cpusubtype = CPU_SUBTYPE_HPPA_730;
		printf("730 (%d%sMhz PA-RISC", mhzint, mhzfra);
		break;

	case CPU_M750:
		cpusubtype = CPU_SUBTYPE_HPPA_750;
		printf("750 (%d%sMhz PA-RISC", mhzint, mhzfra);
		break;

	case CPU_M725_100:
	case CPU_M725_75:
	case CPU_M725_50:
		cpusubtype = CPU_SUBTYPE_HPPA_725;
		printf("725 (%d%sMhz PA-RISC", mhzint, mhzfra);
		break;

	case CPU_M770_100:
	case CPU_M770_120:
		cpusubtype = CPU_SUBTYPE_HPPA_770;
		printf("770 (%d%sMhz PA-RISC", mhzint, mhzfra);
		dcache_size = cp->ic_size; /* XXX */
		break;

	case CPU_M777_100:
	case CPU_M777_120:
		cpusubtype = CPU_SUBTYPE_HPPA_777;
		printf("777 (%d%sMhz PA-RISC", mhzint, mhzfra);
		dcache_size = cp->ic_size; /* XXX */
		break;

	    default:
		printf("\nunknown machine type 0x%x\n", machtype);
		printf("good luck :-)\n");
		machtype = 0;
		break;
	}

	if (cp->dc_conf.cc_sh == 0)
		printf(", %dK Icache, %dK Dcache", cp->ic_size / 1024,
		       dcache_size / 1024);
	else
		printf(", %dK shared I/D cache", cp->ic_size / 1024);
	if (cp->dt_conf.tc_sh == 0)
		printf(", %d entry ITLB, %d entry DTLB", cp->it_size,
		       cp->dt_size);
	else
		printf(", %d entry shared TLB", cp->it_size);
		
	printf(")\n");
}

void
set_bootstrap_args(void)
{
	char *arg_ptr;
	unsigned arg_len;
	extern struct thread_syscall_state thread_state;
	
	arg_len = strlen(boot_string);

	if (arg_len)
	{
		arg_ptr = (char*)thread_state.sp;
		(void) copyout(boot_string, arg_ptr, arg_len);
		thread_state.arg0 = 1;
		thread_state.arg1 = (unsigned)arg_ptr;
		thread_state.sp += arg_len;
	}
	else
	{
		thread_state.arg0 = 0;
		thread_state.arg1 = 0;
	}
}	
