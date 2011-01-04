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
 * load_rose.c
 *
 * This file contains the functions required to load an OSF/1 object 
 * file into memory.
 *
 * Use of compiler conditionals in this file:
 *	STANDALONE  -- turned on if this function is being used in a
 *			standalone boot environment (e.g. as secondary
 *			bootstrap).
 *	LOADSYMBOLS -- turned on if symbol table information is to be
 *			loaded.
 *
 *	Test/Debug conditionals:
 *
 *	DEBUG       -- turned on to print out interesting debug information.
 *	TSTANDALONE -- turned on to actually read the object file sections
 *			into memory passed as a pointer to load_rose(), but
 *			does not jump to the entry point.  This conditional
 *			also turns OFF the code below in main(), and turns
 *			this into a library function called from another
 *			module.
 *	TSYMBOLS    -- turned on to do real reads on the symbol table
 *			related sections only.  There is a problem with
 *			simply reading into the memory described, since
 *			it is often a huge number in VA space.  Therefore,
 *			this option will unconditionally begin writing
 *			at the offset passed to load_rose(), not where the
 *			symbol table information would really be.
 *			This conditional must be used in conjunction 
 *			with TSTANDALONE.
 *
 * Expected use of conditionals:
 *	Production conditinals: -DSTANDALONE -DLOADSYMBOLS
 *	Debug conditionals:	-DDEBUG -DLOADSYMBOLS [-DTSTANDALONE]
 *	Debug conditionals:	-DDEBUG -DLOADSYMBOLS [-DTSTANDALONE -DTSYMBOLS]
 */


#define STANDALONE
#define LOADSYMBOLS

/*
 * Check for consistent #define's. Debug implies plenty
 * of space, hence !SMALLHD.
 */
#if defined(SMALLHD) && defined(DEBUG)
#undef  SMALLHD
#endif


#ifdef	STANDALONE

#ifdef PMAX
#include "../../../sys/param.h"
#include "../../cpu.h"
#include "../../entrypt.h"

#define VTOP(a) (a)
#define PTOV(a) (a)
#define printf _prom_printf
extern int prom_io;
extern int ub_argc;
extern char **ub_argv;
extern char **ub_envp;
#endif 	/* PMAX */

#ifdef AT386
#define VTOP(a)	 (((unsigned)(a)) & (~0xc0000000U))	
#define PTOV(a)  (((unsigned)(a)) |   0xc0000000 )
#define read(fd, addr, len) xread(fd, VTOP(addr), len)
#endif 	/* AT386 */

#else	/* STANDALONE */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#endif	/* STANDALONE */

#include <mach_o_header.h>
#include <mach_o_format.h>
#include "rose_syminfo.h"

#if	defined(STANDALONE) || defined(TSTANDALONE)
#define REALLY_DOIT 1
#else
#define REALLY_DOIT 0
#endif

/* local constants */
#define MAX_LOAD_CMD_BUF_SIZE 4*1024   /* 4KB space for load commands */

 /* inline functions */
#define LAST_LD_CMD LDC_GEN_INFO+1
#define GET_LOAD_CMD(buf,hdr,cmd_off) (buf - hdr->moh_first_cmd_off + cmd_off) 

char *ldCmdTable[LAST_LD_CMD]= {"LDC_UNDEFINED",
			 "LDC_CMD_MAP",
			 "LDC_INTERPRETER",
			 "LDC_STRINGS",
			 "LDC_REGION",
			 "LDC_RELOC",
			 "LDC_PACKAGE",
			 "LDC_SYMBOLS",
			 "LDC_ENTRY",
			 "LDC_FUNC_TABLE",
			 "LDC_GEN_INFO"};
#define GET_LD_CMD_TXT(cmd) (ldCmdTable[cmd])

char *ldSymTable[SYMC_STABS+1] =  {"SYMC_UNDEFINED", 
				   "SYMC_IMPORTS",
				   "SYMC_DEFINED_SYMBOLS",
				   "SYMC_STABS"};

/* 
 * XXX why isn't MAX_USAGE_TYPE right??
 */
#ifndef SMALLHD
char *ldUsageTable[2*MAX_USAGE_TYPE] = {"usage_undefined",
					"text",
					"data",
					"rdata",
					"sdata",
					"bss",
					"sbss",
					"glue"};
#endif	/* !SMALLHD */


/* 
 * external global variables 
 */
#ifdef	STANDALONE
#ifdef PMAX
#define printf _prom_printf
#endif

extern int ub_argc;			/* argc for bootee */
extern char **ub_argv;			/* argv for bootee */
extern char **ub_envp;			/* envp for bootee */
#endif /* STANDALONE */

/* 
 * module-local global variables 
 */
mo_vm_addr_t	programEntryPoint;	/* entry point */
mo_vm_addr_t	loadOffset;		/* addr to start load */

/* 
 * start and end addr of all regions 
 */
mo_vm_addr_t	regionLow  = (mo_vm_addr_t) 0x7fffffff;	
mo_vm_addr_t 	regionHigh = (mo_vm_addr_t) 0x0;	

/* 
 * symbol table information globals 
 */
struct ro_sym_info_hdr *ro_sym_hdr = NULL;


#ifdef LOADSYMBOLS

/* process_ldc_strings
 *
 * function:
 * 	This procedure processes a strings load command struct.
 * 	It loads a strings section into memory, starting at
 *	the global, regionHigh.
 *	It may be called more than once during the load process, so
 *	it must link multiple sections together.
 *
 * inputs:
 *	io - input file descriptor
 *	ldcsymbols - a pointer to the symbols load command struct
 *
 * outputs:
 *	success == 0
 *	failure != 0
 *
 */

process_ldc_strings (io, ldcid, ldcstrings)
	int io;
	mo_lcid_t ldcid;
	strings_command_t *ldcstrings;
{
	struct ro_sym_info *ro_info, *ro_info1;
	mo_long_t	   len = ldcstrings->ldc_section_len;

#ifdef	DEBUG
	printf("strings: flags\t\t: %x\n",ldcstrings->strc_flags);
	printf("strings: loading %d (0x%x) bytes of strings, starting at: %x; ldcid = %d\n", len, len, regionHigh, ldcid);
#endif	/* DEBUG */
	ro_info = (struct ro_sym_info *) VTOP(regionHigh);
  
	/*
	 * Read the section.
	 */
#if	REALLY_DOIT
	lseek(io, ldcstrings->ldc_section_off, 0);
	if (read(io, (caddr_t) &ro_info->ro_symi_strings, len) != len) {
		printf("short strings read\n");
		return(1);
	}
	ro_info->ro_symi_next = (struct ro_sym_info *) 0;
	ro_info->ro_symi_len = len;
	ro_info->ro_symi_strings_section = (struct ro_sym_info *) ldcid;
	/*
	 * This field has no meaning for this section.  It is used to 
	 * hold a cookie to match defined symbol section references.
	 */
	/*
	 * Fix up list of strings sections
	 */
	if (ro_sym_hdr->ro_symh_strings == (struct ro_sym_info *) 0)
		ro_sym_hdr->ro_symh_strings = ro_info;
#ifndef SMALLHD
	else {
		for (ro_info1 = ro_sym_hdr->ro_symh_strings;
			ro_info1->ro_symi_next; 
			ro_info1 = ro_info1->ro_symi_next);
		ro_info1->ro_symi_next = ro_info;
	}
#endif	/* SMALLHD */
#endif	/* REALLY_DOIT */
	regionHigh += (len + 2*sizeof(struct ro_sym_info *) +sizeof(mo_long_t));
	/* round up to long word */
	regionHigh = (mo_vm_addr_t) ((unsigned)regionHigh & (~3));
	return (0);
}

/* process_ldc_symbols
 *
 * function:
 * 	This procedure processes a symbols load command struct.
 * 	It loads the symbol table.
 *	It may be called more than once during the load process, so
 *	it must link multiple symbol sections together, keeping
 *	defined symbol sections separate from strings sections.
 *
 * inputs:
 *	io - input file descriptor
 *	ldcsymbols - a pointer to the symbols load command struct
 *
 * outputs:
 *	success == 0
 *	failure != 0
 *
 */

process_ldc_symbols (io, ldcsymbols)
	int io;
	symbols_command_t *ldcsymbols;
{
	struct ro_sym_info *ro_info, *ro_info1, **ro_infopp;
	mo_long_t	   len = ldcsymbols->ldc_section_len;

#ifdef	DEBUG
	printf("symbols: loading %d (0x%x) bytes of symbols, starting at: %x\n", len, len, regionHigh);
	printf("kind\t\t: %d (%s)\n",ldcsymbols->symc_kind,
					ldSymTable[ldcsymbols->symc_kind]);
	printf("nentries\t\t: %d (0x%x)\n",ldcsymbols->symc_nentries,
					ldcsymbols->symc_nentries);
	printf("pck list\t\t: %d (0x%x)\n",ldcsymbols->symc_pkg_list,
					ldcsymbols->symc_pkg_list);
	printf("strings section\t\t: %d (0x%x)\n",
					ldcsymbols->symc_strings_section,
					ldcsymbols->symc_strings_section);
	printf("relocation addr\t\t: %d (0x%x)\n",ldcsymbols->symc_reloc_addr,
					ldcsymbols->symc_reloc_addr);
#endif	/* DEBUG */
	ro_info = (struct ro_sym_info *) VTOP(regionHigh);
  
	/*
	 * Read the section.
	 */
#if	REALLY_DOIT
	lseek(io, ldcsymbols->ldc_section_off, 0);
	if (read(io, (caddr_t) &ro_info->ro_symi_symbols, len) != len) {
		printf("short symbols read\n");
		return(1);
	}
	ro_info->ro_symi_next = (struct ro_sym_info *) 0;
	ro_info->ro_symi_len = len;
	/*
	 * Find the associated strings section in memory and point to it.
	 */
	if (ldcsymbols->symc_kind != SYMC_STABS) {
		for (ro_info1 = ro_sym_hdr->ro_symh_strings; ro_info1 &&
			((mo_lcid_t) ro_info1->ro_symi_strings_section !=
			ldcsymbols->symc_strings_section);
			ro_info1 = ro_info1->ro_symi_next);
		ro_info->ro_symi_strings_section = ro_info1;
		ro_infopp = &ro_sym_hdr->ro_symh_defined;
	} else {
		ro_info->ro_symi_strings_section = 0;
		ro_infopp = &ro_sym_hdr->ro_symh_stabs;
	}
#ifdef	DEBUG
	if (!ro_info1)
		printf("No strings section found for symbols section\n");
	printf("Found strings section at 0x%x for symbol section at 0x%x; ldcid = %d\n",
		ro_info1, ro_info, ldcsymbols->symc_strings_section);
#endif
	/*
	 * Fix up list of symbols sections
	 */
	if (*ro_infopp == (struct ro_sym_info *) 0)
		*ro_infopp = ro_info;
#ifndef SMALLHD
	else {
		ro_info1 = *ro_infopp;
		while (ro_info1->ro_symi_next)
			ro_info1 = ro_info1->ro_symi_next;
		ro_info1->ro_symi_next = ro_info;
	}
#endif	/* !SMALLHD */
#endif	/* REALLY_DOIT */
	regionHigh += (len + 2*sizeof(struct ro_sym_info *) +sizeof(mo_long_t));
	return (0);
}

#endif	/* LOADSYMBOLS */

/* process_ldc_region
 *
 * function:
 *	This procedure processes the region load command
 *	It loads the executable code, data and bss.
 *
 *
 * inputs:
 *	io - input file descriptor
 *	ldcregion - a pointer to the region load command struct
 *	loadOffset - offset to do actual load
 *
 * outputs:
 *	success == 0
 *	failure != 0
 *     
 */
process_ldc_region (io, ldcregion, loadOffset)
	int io;
	region_command_t *ldcregion;
	mo_vm_addr_t loadOffset;
{
	unsigned base;

#ifdef	DEBUG
	printf("adr lcid\t\t: %d 0x%x\n",
     ldcregion->regc_region_name.adr_lcid,ldcregion->regc_region_name.adr_lcid);
	printf("adr section\t\t: %d (0x%x)\n",
		ldcregion->regc_region_name.adr_sctoff,
		ldcregion->regc_region_name.adr_sctoff);
	printf("rel/vm addr\t\t: %d (0x%x)\n",
		ldcregion->regc_rel_addr, ldcregion->regc_rel_addr);
	printf("vm size\t\t\t: %d (0x%x)\n",
		ldcregion->regc_vm_size, ldcregion->regc_vm_size);
	printf("flags\t\t\t: %d (0x%x)\n",
		ldcregion->regc_flags, ldcregion->regc_flags);
	printf("reloc addr\t\t: %d (0x%x)\n",
		ldcregion->regc_reloc_addr, ldcregion->regc_reloc_addr);
	printf("align\t\t\t: %d (0x%x)\n",
		ldcregion->regc_addralign, ldcregion->regc_addralign);
	printf("usage\t\t\t: %s (0x%x)\n",
		ldUsageTable[ldcregion->regc_usage_type], ldcregion->regc_usage_type);
	printf("init protection\t\t: %d (0x%x)\n",
		ldcregion->regc_initprot, ldcregion->regc_initprot);
#endif	/* DEBUG */

	/* 
	 * if we have a null region then just ignore it and return 
	 */
	if (ldcregion->regc_vm_size == 0)
		return (0);

#ifndef SMALLHD
	printf("region size (%s)\t: %d\t(0x%x)\n",
				ldUsageTable[ldcregion->regc_usage_type],
				ldcregion->regc_vm_size,
				ldcregion->regc_vm_size);
#else
	printf("+%d", ldcregion->regc_vm_size);
#endif 	/* !SMALLHD */

	/* 
	 * Calculate the highest and lowest addresses for the regions 
	 */
	if (regionHigh < (ldcregion->regc_vm_addr+ldcregion->regc_vm_size))
		regionHigh = ldcregion->regc_vm_addr + ldcregion->regc_vm_size;
	if (regionLow > ldcregion->regc_vm_addr)
		regionLow = ldcregion->regc_vm_addr;

	/* 
	 * load the region 
	 */
#if	REALLY_DOIT && !defined(TSYMBOLS)
	if (ldcregion->ldc_section_len > 0) {
		lseek(io, ldcregion->ldc_section_off, 0);
		base = (unsigned)ldcregion->regc_vm_addr +
		       (unsigned)loadOffset;
		if (read(io, base, ldcregion->ldc_section_len) !=
			ldcregion->ldc_section_len) {
			printf("error reading region \n");
			return(1);
		}
	} else {
		/* zero the bss section */
		base = (unsigned)ldcregion->regc_vm_addr +
		       (unsigned)loadOffset;
		bzero(base, ldcregion->regc_vm_size);
	}
#endif	/* REALLY_DOIT && !defined(TSYMBOLS) */
    
	return (0);
}

/* process_ldc_entry
 *
 * function:
 *	This procedure processes the entry load command.  There is
 *	nothing to do except grab the entry point.
 *
 * inputs:
 *	io - input file descriptor
 *	ldcentry - a pointer to the entry load command struct
 *
 * outputs:
 *	ENTRY POINT
 */

process_ldc_entry (io, ldcentry, cmdOff)
	int io;
	entry_command_t *ldcentry;
{
#ifdef	DEBUG
	printf("flags\t\t\t: %x\nabsaddr\t\t\t: %x\nentry lcid\t\t: %x\nsec off\t\t\t: %x\n",
	   ldcentry->entc_flags, ldcentry->entc_absaddr,
	   ldcentry->entc_entry_pt.adr_lcid,ldcentry->entc_entry_pt.adr_sctoff);
#endif

#ifndef SMALLHD
	if ((ldcentry->entc_flags & ENT_VALID_ABSADDR_F))
		printf("valid entry point\t: 0x%x\n",ldcentry->entc_absaddr);
	else
		printf("NOT VALID ABSADDR ENTRY: %x\n",ldcentry->entc_absaddr);
#endif	/* !SMALLHD */

	programEntryPoint = ldcentry->entc_absaddr;
	return (0);
}

/* process_load_command
 *
 * function:
 *	This procedure processes all valid load commands
 *
 * inputs:
 *	cmdOff -- (from BOF) command offset
 *	loadOffset -- bottom address for loading into memory
 *	dosymbols -- true if we're loading symbol table and string
 *			 information.  This is done last.
 *
 * outputs:
 *	success == 0
 *	failure != 0
 */

process_load_command (io, ldCmdBuf, mohdr, cmdOff, loadOffset, ldcid, dosymbols)
	int io;
	int cmdOff;
	struct mo_header_t *mohdr;
	char *ldCmdBuf;
	mo_vm_addr_t loadOffset;
	mo_lcid_t ldcid;
	int dosymbols;
{
	struct ldc_header_t *ldchdr;

	ldchdr = (ldc_header_t *)GET_LOAD_CMD(ldCmdBuf,mohdr,cmdOff);
	if (ldchdr->ldci_cmd_type > LAST_LD_CMD) {
		printf("unknown command type: %d\n", 
			ldchdr->ldci_cmd_type);
		return (1);
	}
#ifdef	DEBUG
    /*
     * Only print once.  This function may be called twice for each
     * load command (with, and without dosymbols).
     */
    if (!dosymbols) {
	printf("ld command type\t: %s\n",GET_LD_CMD_TXT(ldchdr->ldci_cmd_type));
	printf("ld command size\t: %d (0x%x)\n",
		ldchdr->ldci_cmd_size, ldchdr->ldci_cmd_size);
	printf("section offset\t: %d (0x%x)\n",
		ldchdr->ldci_section_off, ldchdr->ldci_section_off);
	printf("section length\t: %d (0x%x)\n",
		ldchdr->ldci_section_len, ldchdr->ldci_section_len);
    }
#endif
	switch(ldchdr->ldci_cmd_type) {
		case LDC_REGION:
			if (!dosymbols)
				return (process_ldc_region(io, 
						   (region_command_t *)ldchdr,
						   loadOffset));
			break;
#ifdef LOADSYMBOLS
		case LDC_SYMBOLS:
			if (dosymbols == LDC_SYMBOLS)
				return (process_ldc_symbols(io,
						  (symbols_command_t *)ldchdr));
			break;
		case LDC_STRINGS:
			if (dosymbols == LDC_STRINGS)
				return (process_ldc_strings(io, ldcid,
						  (strings_command_t *)ldchdr));
			break;
#endif
		case LDC_ENTRY:
			if (!dosymbols)
				return (process_ldc_entry(io,
						(entry_command_t *)ldchdr));
			break;
		default:
			break;
	}
	return (0);
}

/* load_rose
 *
 * function:
 *	This function loads a binary image in ROSE format
 *	It's analogous to the "exec" procedure getxfile.
 *
 * inputs:
 *	io - file descriptor for the image file to load
 *	loadOffset - starting address of memory to use for loading image
 *			(will be 0 for loading kernel in production system).
 *    
 * outputs:
 *	success == 0
 *	failure != 0
 *
 */

load_rose (io, loadOffset)
	int io;
	mo_vm_addr_t loadOffset;
{
	struct mo_header_t  *mohdr;
	char read_buffer[MO_SIZEOF_RAW_HDR],decode_buffer[MO_SIZEOF_RAW_HDR];
	struct ldc_header_t *ldchdr;
	struct load_cmd_map_command_t *ldcmc;
	char ldCmdBufHeap[MAX_LOAD_CMD_BUF_SIZE];
	char *ldCmdBuf;
	int ioStatus,ldcid;

	/* create some storage */
	ldCmdBuf = &ldCmdBufHeap[0];

	/* read in object header information */
	if ((ioStatus = read(io, &read_buffer, MO_SIZEOF_RAW_HDR)) < 1) {
		printf("Error reading object header\n");
		return (-1);
	}

	/* decode the canonical header */
	mohdr = (struct mo_header_t *) &decode_buffer;
	decode_mach_o_hdr(&read_buffer, MO_SIZEOF_RAW_HDR,
					MOH_HEADER_VERSION, mohdr);
	if (mohdr->moh_magic != MOH_MAGIC) {
		printf("bad magic: %x\n", mohdr->moh_magic);
		return(-1);
	}
#ifndef SMALLHD
	printf("Loading ROSE object file\n");
#endif

#ifdef	DEBUG
	printf(" magic number        %x\n cpu type            %x\n vendor type         %x\n load map cmd offset %x\n first cmd offset    %x\n size of cmd         %d 0x%x\n # of load cmds      %x\n page size           %x\n",
	mohdr->moh_magic, mohdr->moh_cpu_type, mohdr->moh_vendor_type,
	mohdr->moh_load_map_cmd_off, mohdr->moh_first_cmd_off,
	mohdr->moh_sizeofcmds, mohdr->moh_sizeofcmds,
	mohdr->moh_n_load_cmds, mohdr->moh_max_page_size);
#endif


	/* allocate some storage for all commands */

	if (mohdr->moh_sizeofcmds > MAX_LOAD_CMD_BUF_SIZE) {
		printf("command buffer overflow\n");
		return(-1);
	}

	/* now read in the commands  command map  */
	lseek(io,mohdr->moh_first_cmd_off,0);
	if ((ioStatus = read(io, ldCmdBuf, (long)mohdr->moh_sizeofcmds)) < 0) {
		printf("error reading load commands\n");
		return(-1);
	}

	/* get the load command map */
	ldcmc = (load_cmd_map_command_t *) GET_LOAD_CMD(ldCmdBuf, mohdr,
						mohdr->moh_load_map_cmd_off);

	if (ldcmc->ldc_cmd_type > LAST_LD_CMD) {
		printf("command botch; not load map cmd %i\n",
			ldcmc->ldc_cmd_type);
		return(-1);
	}

#ifdef	DEBUG
	printf(" cmd type %x %s\n cmd size %x\n section offset %x\n section length %x\n",
		ldcmc->ldc_cmd_type,GET_LD_CMD_TXT(ldcmc->ldc_cmd_type),
		ldcmc->ldc_cmd_size,ldcmc->ldc_section_off,
		ldcmc->ldc_section_len);
	printf(" cmd strings %x\n cmd entries %x\n",ldcmc->lcm_ld_cmd_strings,
		ldcmc->lcm_nentries);
	for (ldcid = 0; ldcid < ldcmc->lcm_nentries; ldcid++)
		printf("ldcid = %x %d\n",
			ldcmc->lcm_map[ldcid], ldcmc->lcm_map[ldcid]);
#endif

	/* 
	 * process each load command without symbols first
	 */
	for (ldcid = 0; ldcid < ldcmc->lcm_nentries; ldcid++)
		if (process_load_command (io, ldCmdBuf, mohdr,
			ldcmc->lcm_map[ldcid], loadOffset, ldcid, 0) != 0)
			return (-1);

#ifndef SMALLHD
	printf("Starting at 0x%x\n",programEntryPoint);
#endif	/* !SMALLHD */

#ifdef	LOADSYMBOLS

#ifndef SMALLHD
	printf("Loading symbol table starting at 0x%x\n", regionHigh);
#endif	/* !SMALLHD */

#ifdef	TSYMBOLS
	regionHigh = loadOffset;	/* start at the beginning */
#endif
	ro_sym_hdr = (struct ro_sym_info_hdr *) VTOP(regionHigh);
	regionHigh += sizeof(struct ro_sym_info_hdr);
#if	REALLY_DOIT
	bzero(ro_sym_hdr, sizeof(struct ro_sym_info_hdr));
#endif
#ifdef	DEBUG
	printf("First symbol table section at 0x%x\n", regionHigh);
#endif
	/*
	 * Make two more passes through the load commands, first for
	 * the strings sections, and second for the symbols and stabs.
	 * The reason for this is that the defined symbols sections
	 * must reference strings sections, and we want to have all the
	 * strings sections present so we can set the pointers correctly
	 * when loading the defined symbols sections.
	 */
	for (ldcid = 0; ldcid < ldcmc->lcm_nentries; ldcid++)
		if (process_load_command (io, ldCmdBuf, mohdr,
				ldcmc->lcm_map[ldcid], loadOffset, 
				ldcid, LDC_STRINGS) != 0)
			printf("Bad string table read\n");
	for (ldcid = 0; ldcid < ldcmc->lcm_nentries; ldcid++)
		if (process_load_command (io, ldCmdBuf, mohdr,
				ldcmc->lcm_map[ldcid], loadOffset, 
				ldcid, LDC_SYMBOLS) != 0)
			printf("Bad symbol table read\n");
	printf("\n");

#if	REALLY_DOIT
	/*
	 * Store the end of the symbol table information so the kernel
	 * can find it when it starts up. 
	 */
	ro_sym_hdr->ro_symh_end = (unsigned) regionHigh;

	/* 
	 * Relocate pointers from the physical addresses we use to the
	 * virtual addresses the kernel will use. Only bother to do this
	 * if we have symbol information, and the map is not the identity
	 * function.
	 */
#define CNV(a) (struct ro_sym_info *) PTOV(a)

	if (ro_sym_hdr != NULL && VTOP(-1) != -1)  {
		struct ro_sym_info *s;
        	if (s = ro_sym_hdr->ro_symh_defined) {
			s->ro_symi_next = CNV(s->ro_symi_next);
			s->ro_symi_strings_section = 
				CNV(s->ro_symi_strings_section);
			ro_sym_hdr->ro_symh_defined =
				CNV(ro_sym_hdr->ro_symh_defined);
		}
		if (s = ro_sym_hdr->ro_symh_strings) {
			s->ro_symi_next = CNV(s->ro_symi_next);
			s->ro_symi_strings_section = 
				CNV(s->ro_symi_strings_section);
			ro_sym_hdr->ro_symh_strings =
				CNV(ro_sym_hdr->ro_symh_strings);
		}		
		if (s = ro_sym_hdr->ro_symh_stabs) {
			s->ro_symi_next = CNV(s->ro_symi_next);
			s->ro_symi_strings_section = 
				CNV(s->ro_symi_strings_section);
			ro_sym_hdr->ro_symh_stabs =
				CNV(ro_sym_hdr->ro_symh_stabs);
		}		
			
	}
#undef CNV
		
#endif
#endif	/* LOADSYMBOLS */


	/*
	 * XXX mi load_rose should NOT call loaded image -- the
	 *     md code should (one might, e.g., have to switch to
	 *     i386 protected mode first).
	 */
#if	defined(STANDALONE) && defined(PMAX)
	/* 
	 * Start this puppy running.
	 * If this is not a multimax then call as if it were a procedure 
	 * entry 
	 */

	(*((int (*) ())programEntryPoint)) (ub_argc, ub_argv, ub_envp);

	return (-1);
#else
	return (0);
#endif	/* STANDALONE && PMAX */
}

#ifndef	STANDALONE
#include <malloc.h>
#define MALLOC_SIZE 0x4fffff
#define CHUNK_SIZE 8192
void fixup_ptrs();

main(argc, argv)
	int argc;
	char *argv[];
{

	int fd, error;
	char *space;

	if ((argc < 2) || (argc > 3)) {
		fprintf(stderr, "usage: %s file [outfile]\n", argv[0]);
		exit(1);
	}
	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror("open:");
		exit(1);
	}
	space = (char *) malloc(MALLOC_SIZE);
	if (space == 0) {
		perror("malloc:");
		exit(1);
	}
	error = load_rose(fd, space);
	fprintf(stdout, "Return from load_rose = %d, high = 0x%x\n", 
		error, regionHigh);
	if ((error == 0) && (argc == 3)) {
		int ret, fdout;

		if ((fdout = open(argv[2], O_TRUNC|O_CREAT|O_RDWR, 0640)) < 0) {
			perror("open1:");
			exit(1);
		}
		/* 
		 * fix up pointers first
		 */
#ifdef	TSYMBOLS
		fixup_ptrs(space);
#endif
		fprintf(stdout, "Writing %d bytes to %s\n", 
				(caddr_t) regionHigh - space, argv[2]);
		do {
			ret = write(fdout, space, CHUNK_SIZE);
			space += CHUNK_SIZE;
		} while ((ret == CHUNK_SIZE) && (space < regionHigh));
		close(fdout);
	}
	close(fd);
}

void
fixup_ptrs(ro_sym_hdrp)
	struct ro_sym_info **ro_sym_hdrp;
{
	int i;
	struct ro_sym_info *ro_sym_info;
	unsigned offset = (unsigned) ro_sym_hdrp;
	/*
	 * Subtract the offset from the header fields.
	 */
	for (i = 0; i < 5; i++)
		if (ro_sym_hdrp[i])
			(unsigned)ro_sym_hdrp[i] -= offset;
	/*
	 * Now fix up the ro_sym_info structures on each list.
	 */
	for (i = 0; i < 4; i++) {
		ro_sym_info = ro_sym_hdrp[i];
		while (ro_sym_info) {
		    (unsigned) ro_sym_info += offset;
		    if (ro_sym_info->ro_symi_next)
		    	(unsigned) ro_sym_info->ro_symi_next -= offset;
		    (unsigned) ro_sym_info->ro_symi_strings_section -= offset;
		    ro_sym_info = ro_sym_info->ro_symi_next;
		}
	}
}
#endif	/* !STANDALONE */
