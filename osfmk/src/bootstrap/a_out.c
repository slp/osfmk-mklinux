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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
 */
/*
 * routines for loading a.out files.
 */

#include "bootstrap.h"

#include "exec.h"
#include <mach/machine/vm_param.h>

int a_out_recog(struct file *, objfmt_t, void *);
int a_out_load(struct file *, objfmt_t, void *);
void a_out_symload(struct file *, mach_port_t, task_port_t, const char *,
		   objfmt_t);

struct objfmt_switch a_out_switch = {
    "a_out",
    a_out_recog,
    a_out_load,
    a_out_symload
};

int
a_out_recog(struct file *fp, objfmt_t ofmt, void *hdr)
{
	struct exec *x = *(struct exec **)hdr;

	switch ((int)x->a_magic) {
	case OMAGIC:
	case NMAGIC:
	case ZMAGIC:
	    return 1;
	    break;
	default:
	    return 0;
	}
}

int
a_out_load(struct file *fp, objfmt_t ofmt, void *hdr)
{
    struct loader_info *lp = &ofmt->info;
    struct exec   *x = (struct exec *)hdr;
    kern_return_t result;

    switch ((int)x->a_magic) {
    case OMAGIC:
	lp->text_start  = 0;
	lp->text_size   = 0;
	lp->text_offset = 0;
	lp->data_start  = USRTEXT;
	lp->data_size   = x->a_text + x->a_data;
	lp->data_offset = sizeof(struct exec);
	lp->bss_size    = x->a_bss;
	break;

    case NMAGIC:
	if (x->a_text == 0) 
	    return(EX_NOT_EXECUTABLE);

	lp->text_start  = USRTEXT;
	lp->text_size   = x->a_text;
	lp->text_offset = sizeof(struct exec);
	lp->data_start  = lp->text_start + lp->text_size;
	lp->data_size   = x->a_data;
	lp->data_offset = lp->text_offset + lp->text_size;
	lp->bss_size    = x->a_bss;
	break;

    case ZMAGIC:
	if (x->a_text == 0)
	    return(EX_NOT_EXECUTABLE);
	
	lp->text_start  = USRTEXT;
	lp->text_size   = sizeof(struct exec) + x->a_text;
	lp->text_offset = 0;
	lp->data_start  = lp->text_start + lp->text_size;
	lp->data_size   = x->a_data;
	lp->data_offset = lp->text_offset + lp->text_size;
	lp->bss_size    = x->a_bss;
	break;
    default:
	return (EX_NOT_EXECUTABLE);
    }

    lp->entry_1 = x->a_entry;
    lp->entry_2 = 0;

    lp->sym_offset[0] = sizeof(struct exec) + x->a_text + x->a_data + x->a_trsize + x->a_drsize;
    lp->sym_size[0] = x->a_syms;
    lp->str_offset = lp->sym_offset[0] + lp->sym_size[0];

    result = read_file(fp, lp->str_offset, (vm_offset_t)&lp->str_size,
		       sizeof(int));
    if(result)
	return result;

    return(0);
}

/*
 * Load symbols from file into kernel debugger.
 */
void
a_out_symload(struct file *fp,
	      mach_port_t host_port,
	      task_port_t task,
	      const char *symtab_name,
	      objfmt_t ofmt)
{
    /*
     * The symbol table is read from the file.
     * Machine-dependent code can specify an
     * optional header to be prefixed to the
     * symbol table, containing information
     * that cannot directly be read from the file.
     */

    struct loader_info	*lp = &ofmt->info;
    kern_return_t result;
    vm_offset_t	  symtab;
    int           symsize;
    vm_size_t     table_size;

    /*
     * Allocate space for the symbol table, preceding it by the header.
     */

    symsize = lp->sym_size[0];

    table_size = symsize + lp->str_size + sizeof(int);

    result = vm_allocate(mach_task_self(), &symtab, table_size, TRUE);
    if (result)
    {
	BOOTSTRAP_IO_LOCK();
	printf("[ error %d allocating space for %s symbol table ]\n",
	       result, symtab_name);
	BOOTSTRAP_IO_UNLOCK();
	return;
    }

    *(int*)symtab = symsize;

    /*
     * Get the strings after the symbol table
     * Load the symbols into the kernel.
     */
    if (read_file(fp, lp->sym_offset[0],
		  symtab + sizeof(int), symsize + lp->str_size) ||
	host_load_symbol_table(host_port, task,
			       (char *)symtab_name, symtab, table_size))
    {
	BOOTSTRAP_IO_LOCK();
	printf("[ no valid symbol table present for %s ]\n", symtab_name);
	BOOTSTRAP_IO_UNLOCK();
    }

    (void) vm_deallocate(mach_task_self(), symtab, table_size);
}
