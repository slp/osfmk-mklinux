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
 * Copyright (c) 1991 Carnegie Mellon University
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
 * CMU_HISTORY
 * Revision 2.3  92/04/01  19:36:09  rpd
 * 	Fixed to handle kernels with non-contiguous text and data.
 * 	[92/03/13            rpd]
 * 
 * Revision 2.2  92/01/03  20:28:52  dbg
 * 	Created.
 * 	[91/09/12            dbg]
 * 
 */

/*
 * COFF-dependent (really, i860 COFF-dependent) routines for makeboot
 */

#include "bootstrap.h"

/*
 *	Following three includes come from kernel sources via export area.
 *	For i860, they are unrestricted.
 */
#include <machine/coff.h>
#include <machine/exec.h>
#include <machine/syms.h>


#define OSCNRND  8

int coff_recog(struct file *, objfmt_t, void *);
int coff_load(struct file *, objfmt_t, void *);
void coff_symload(struct file *, mach_port_t, task_port_t,
		  const char *, objfmt_t);

struct objfmt_switch coff_switch = {
    "coff",
    coff_recog,
    coff_load,
    coff_symload
};

int
coff_recog(struct file *fp, objfmt_t ofmt, void *hdr)
{
	struct filehdr *fhdr = *(struct filehdr **)hdr;

        switch ((int)fhdr->f_magic) {
#if	defined(i860)
	    case I860MAGIC:
		return(1);
#endif	/* defined(i860) */

#if	defined(__alpha)
	    case ALPHAMAGIC:
	    case ALPHAUMAGIC:
	        return 1;
	        break;
#endif	/* defined(__alpha) */
	    default:
	        return 0;
	}

}

/*
 * Convert a coff-style file header to a loader_info-style one.
 * On return, *sym_size contains the symbol table size (alone),
 * while the first sym_size member of lp contains the total
 * space needed for symbol table-related data.
 */
int
coff_load(
	struct file	*fp,
	objfmt_t 	ofmt,
	void		*hdr)
{
	struct loader_info	*lp = &ofmt->info;
        struct filehdr *fhdrp;
        struct aouthdr *aoutp;
        long text_offset;

#ifndef __alpha
	/* This exec_hdr is currently only needed to get sh0 and sh1
	 * structures.  It would be nice to remove it but further
	 * testing is needed on the i860 to determine if the
	 * code that calculates the .text offset for the APLHA
	 * would also work for the i860.  There are some rounding
	 * concerns.
	 */
	struct exec_hdrs	*x = (struct exec_hdrs *)hdr;
#endif

        fhdrp = (struct filehdr *)hdr;
        aoutp = (struct aouthdr *)(hdr + sizeof(struct filehdr));


	if ( 
#if	defined(i860)
	     (fhdrp->f_magic != I860MAGIC)
#endif	/* defined(i860) */
#if	defined(__alpha)
	     (fhdrp->f_magic != ALPHAMAGIC) && 
	     (fhdrp->f_magic != ALPHAUMAGIC)
#endif	/* defined(__alpha) */
	    )
	    return (EX_NOT_EXECUTABLE);

	switch ((int)aoutp->magic) {
	    case ZMAGIC:
		if (aoutp->tsize == 0) {
		    return (1);
		}
		lp->text_start	= aoutp->text_start;
		lp->text_size	= aoutp->tsize;
#ifdef __alpha
               /*
	        * determine the offset in the file of the text section
	        * basicly, just skip over the file, a.out, and section headers.
	        */
	        if (aoutp->magic == ZMAGIC)
	            text_offset = 0;
	        else {
	            text_offset =   sizeof(struct filehdr) 
		        + sizeof(struct aouthdr) 
			+ fhdrp->f_nscns * sizeof(struct scnhdr);

	            if (aoutp->vstamp < 23)
	                text_offset = (text_offset + OSCNRND-1) & ~(OSCNRND-1);
	            else 
		        text_offset = (text_offset + SCNROUND-1) & ~(SCNROUND-1);
	        }

	        lp->text_offset = text_offset;
		lp->data_offset = text_offset + lp->text_size;
#else
		lp->text_offset	= x->sh0.s_scnptr;  /* assume .text first */
		lp->data_offset	= x->sh1.s_scnptr;  /* assume .data second */
#endif
		lp->data_start	= aoutp->data_start;
		lp->data_size	= aoutp->dsize;

		lp->bss_size	= aoutp->bsize;
		break;
	    case OMAGIC:
	    case NMAGIC:
	    default:
		return (1);
	}

#ifdef DEBUG
	BOOTSTRAP_IO_LOCK();
        printf("text start:  0x%x size 0x%x offset 0x%x\n",
	    lp->text_start,
	    lp->text_size,
	    lp->text_offset );

        printf("data start:  0x%x size 0x%x offset 0x%x\n",
  	    lp->data_start,
  	    lp->data_size,
	    lp->data_offset );

        printf("bss size 0x%x\n", lp->bss_size);
	BOOTSTRAP_IO_UNLOCK();
#endif

	lp->entry_1 = aoutp->entry;
#ifdef	__alpha
        lp->entry_2 = aoutp->gp_value;
#else
        lp->entry_2 = 0;
#endif
	lp->sym_size[0] = fhdrp->f_nsyms;
	lp->sym_offset[0] = fhdrp->f_symptr;
	if (lp->sym_offset[0] == 0) {
		lp->sym_size[0] = 0;
	}
	else {
#ifdef __alpha

                lp->str_offset = lp->sym_offset[0] + lp->sym_size[0];

		/* Why are we setting str_size to 0?? */
                lp->str_size = 0;
#else
		if (SYMESZ != AUXESZ) {
			BOOTSTRAP_IO_LOCK();
			printf("can't determine symtab size\n");
			BOOTSTRAP_IO_UNLOCK();
			return (1);
		}
		lp->sym_size[0] *= SYMESZ;
		/*
		 * Compute length of string table.
		 */
		lp->str_offset = lp->sym_offset[0] + lp->sym_size[0];
		lp->str_size = file_size(fp) - lp->str_offset;
#endif
	}
	lp->format = COFF_F;
	return (0);
}

void
coff_symload(
	struct file	*fp,
	mach_port_t host_port,
	task_port_t task,
	const char *symtab_name,
	objfmt_t ofmt)
{
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
	if (result) {
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
				   (char *)symtab_name, symtab, table_size)) {
		BOOTSTRAP_IO_LOCK();
		printf("[ no valid symbol table present for %s ]\n",
		       symtab_name);
		BOOTSTRAP_IO_UNLOCK();
	}

	(void) vm_deallocate(mach_task_self(), symtab, table_size);
}
