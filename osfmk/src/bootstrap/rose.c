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

#include "bootstrap.h"

#include <mach_o_header.h>
#include <mach_o_header_md.h>
#include <mach_o_types.h>
#include <mach_o_format.h>

#include <ddb/nlist.h>

int rose_recog(struct file *, objfmt_t, void *);
int rose_load(struct file *, objfmt_t, void *);
void rose_symload(struct file *, mach_port_t, task_port_t, const char *,
		  objfmt_t);

/*
 *	Machine dependent code must provide the following routine.
 */

extern unsigned long ntohl(unsigned long arg);

struct objfmt_switch rose_switch = {
    "rose",
    rose_recog,
    rose_load,
    rose_symload
};

int
rose_recog(struct file *fp, objfmt_t ofmt, void *hdr)
{
    raw_mo_header_t *x = *(raw_mo_header_t **)hdr;

    return (x->rmoh_magic == 0xcefaefbeU);
}

int
rose_load(struct file *fp, objfmt_t ofmt, void *hdr)
{
    struct loader_info *lp = &ofmt->info;
    raw_mo_header_t *headerp = (raw_mo_header_t *)hdr;
    ldc_header_t           ldc_header;
    load_cmd_map_command_t *load_cmd_map; /* load command map */
    mo_long_t	ldc_nb;
    mo_offset_t	*ldc_off; 
    int nsymt = 0;
    int i;
    vm_size_t symsize;
    kern_return_t result;

    /*
     * get the load command map header
     */
    result = read_file(fp, ntohl(headerp->rmoh_load_map_cmd_off),
		       (vm_offset_t)&ldc_header, sizeof(ldc_header));
    if (result)
	return result;

    load_cmd_map = (load_cmd_map_command_t *)malloc(ldc_header.ldci_cmd_size);
    result = read_file(fp, ntohl(headerp->rmoh_load_map_cmd_off),
		       (vm_offset_t)load_cmd_map, ldc_header.ldci_cmd_size);
    if(result)
	return result;

    /*
     * get the regions infos
     */
    for (ldc_nb = load_cmd_map->lcm_nentries, ldc_off = load_cmd_map->lcm_map;
	 ldc_nb-- > 0; ldc_off++)
    {
	result = read_file(fp, *ldc_off, (vm_offset_t)&ldc_header, sizeof(ldc_header));
	if(result) 
	    return result;

	switch ((int)ldc_header.ldci_cmd_type)
	{
	case LDC_UNDEFINED:	/* undefined load command (logically deleted) */
	case LDC_CMD_MAP:	/* load command for the load command map */
	case LDC_INTERPRETER:	/* load command for the program interpreter (no section) */
	case LDC_RELOC:	        /* load command for a relocation section */
	case LDC_PACKAGE:	/* load command for an import or export package list (no section) */
	case LDC_FUNC_TABLE:	/* load command for a function table (no section) */
	case LDC_GEN_INFO:	/* load command for general information (no section) */
	    break;
	case LDC_STRINGS:	/* load command for a strings section */
	    lp->str_offset = (vm_offset_t)ldc_header.ldci_section_off;
	    lp->str_size = (vm_size_t)ldc_header.ldci_section_len;
	    break;
	case LDC_SYMBOLS:	/* load command for a symbols section */
	    lp->sym_offset[nsymt] = (vm_offset_t)ldc_header.ldci_section_off;
	    symsize = (vm_size_t)ldc_header.ldci_section_len;
	    lp->sym_size[nsymt] =
		(symsize / sizeof(symbol_info_t)) * sizeof(struct nlist);
	    for (i = 0; i < nsymt; i++)
		lp->sym_size[i] += lp->sym_size[nsymt];
	    nsymt++;
	    lp->sym_offset[nsymt] = 0;
	    lp->sym_size[nsymt] = 0;
	    break;
	case LDC_ENTRY:	        /* load command for the program main entry point (no section) */
	{
	    entry_command_t entry_reg_cmd;

	    result = read_file(fp, *ldc_off, (vm_offset_t)&entry_reg_cmd,
			       sizeof(entry_command_t));
	    if(result)
		return result;

	    lp->entry_1 = (vm_offset_t)entry_reg_cmd.entc_absaddr;
	    lp->entry_2 = 0;
	    break;
	}
	case LDC_REGION:	/* load command for a region section (part of the program)  */
	{
	    region_command_t load_reg_cmd;
	    
	    result = read_file(fp, *ldc_off, (vm_offset_t)&load_reg_cmd,
			       sizeof(region_command_t));
	    if(result)
		return result;

	    switch(load_reg_cmd.regc_usage_type)
	    {
	    case REG_TEXT_T:
		lp->text_size = (vm_size_t)load_reg_cmd.regc_vm_size;
		lp->text_offset = (vm_offset_t)ldc_header.ldci_section_off;
		lp->text_start = (vm_offset_t)load_reg_cmd.regc_addr.vm_addr;
		break;
	    case REG_DATA_T:
		lp->data_size = (vm_size_t)load_reg_cmd.regc_vm_size;
		lp->data_offset = (vm_offset_t)ldc_header.ldci_section_off;
		lp->data_start = (vm_offset_t)load_reg_cmd.regc_addr.vm_addr;
		break;
	    case REG_BSS_T:
		lp->bss_size = load_reg_cmd.regc_vm_size;
		break;
	    default:
		break;
	    }
	}
	default:
	    break;
	}
    }

    free((char*)load_cmd_map);
    return 0;
}

/*
 * Load symbols from file into kernel debugger.
 *
 * The symbol table is read from the file.
 * Machine-dependent code can specify an
 * optional header to be prefixed to the
 * symbol table, containing information
 * that cannot directly be read from the file.
 */
void
rose_symload(struct file *fp,
	     mach_port_t host_port,
	     task_port_t task,
	     const char *symtab_name,
	     objfmt_t ofmt)
{
    struct loader_info *lp = &ofmt->info;
    kern_return_t result;
    vm_offset_t	  symtab;
    vm_size_t     table_size;
    struct nlist  *nl;
    vm_offset_t	  buf;
    vm_offset_t	  strings;

    /*
     * Allocate space for the symbol table, preceding it by the header.
     */
    table_size = sizeof(int) + lp->sym_size[0] + lp->str_size + sizeof(int);
    result = vm_allocate(mach_task_self(), &symtab, table_size, TRUE);
    if (result)
    {
	BOOTSTRAP_IO_LOCK();
	printf("[ error %d allocating space for %s symbol table ]\n",
	       result, symtab_name);
	BOOTSTRAP_IO_UNLOCK();
	return;
    }

    *(int*)symtab = lp->sym_size[0];

    nl = (struct nlist *)(symtab + sizeof(int));

    if (lp->sym_size[0])
    {
	symbol_info_t *sstab, *estab;
	int nsyms;

	nsyms = lp->sym_size[0] / sizeof(struct nlist);
	buf = (vm_offset_t)malloc(nsyms * sizeof(symbol_info_t));
	result = read_file(fp, lp->sym_offset[0], buf,
			   nsyms * sizeof(symbol_info_t));
	if (result) {
	    BOOTSTRAP_IO_LOCK();
	    printf("[ error %d reading %s symbol table ]\n",
		   result, symtab_name);
	    BOOTSTRAP_IO_UNLOCK();
	    (void) vm_deallocate(mach_task_self(), symtab, table_size);
	    return;
	}

	sstab = (symbol_info_t*)buf;
	estab = (symbol_info_t*)(buf + nsyms * sizeof(symbol_info_t));

	while(sstab < estab)
	{
	    nl->n_un.n_strx = (long)sstab->si_name.symbol_name + sizeof(int);
	    nl->n_type = (unsigned char)sstab->si_sc_type;
	    nl->n_other = (char)0;
	    nl->n_desc = (short)sstab->si_type;
	    nl->n_value = (unsigned long)sstab->si_value.abs_val;
	    sstab++;
	    nl++;
	}
	free((void *)buf);
    }

    *(int*)nl = lp->str_size + sizeof(int);
    strings = (vm_offset_t)nl + sizeof(int);

    /* 
     * Get the strings after the symbol table
     * Load the symbols into the kernel
     */
    if (read_file(fp, lp->str_offset, strings, lp->str_size) ||
	host_load_symbol_table(host_port, task,
			       (char *)symtab_name, symtab, table_size))
    {
	BOOTSTRAP_IO_LOCK();
	printf("[ no valid symbol table present for %s ]\n", symtab_name);
	BOOTSTRAP_IO_UNLOCK();
    }

    (void) vm_deallocate(mach_task_self(), symtab, table_size);
}
