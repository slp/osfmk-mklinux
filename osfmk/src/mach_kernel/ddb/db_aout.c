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
/* CMU_HIST */
/*
 * Revision 2.9.3.1  92/03/03  16:13:15  jeffreyh
 * 	Pick up changes from MK68
 * 	[92/02/26  10:58:00  jeffreyh]
 * 
 * Revision 2.10  92/01/03  20:02:21  dbg
 * 	Don't print 'preserving symbols' if loading symbol table fails.
 * 	[91/11/29            dbg]
 * 
 * 	Use kern_sym_start, kern_sym_size to locate kernel symbol table.
 * 	Rename routine to ddb_init.
 * 	[91/10/30            dbg]
 * 
 * Revision 2.9  91/10/09  15:57:16  af
 * 	Supported address lookup with line number.
 * 	Changed X_db_line_at_pc to get file name without compiled
 * 	  with -g option.
 * 	Included "stab.h" for symbol type definitions.
 * 	[91/08/29            tak]
 * 
 * Revision 2.8  91/08/28  11:10:58  jsb
 * 	Added line number support, via X_db_line_at_pc.
 * 	[91/08/13  18:12:37  jsb]
 * 
 * Revision 2.7  91/07/31  17:29:43  dbg
 * 	Removed read_symtab_from_file.
 * 	Added task argument to X_db_sym_init.
 * 	[91/07/30  16:42:51  dbg]
 * 
 * Revision 2.6  91/07/09  23:15:35  danner
 * 	On luna, kdb_init needs to be called ddb_init. Add ifndef
 * 	 DB_SYMBOLS_PRELOADED for machines that use a.out format but
 * 	 whose prom loaders load generously load the symbol table.
 * 	[91/07/08            danner]
 * 
 * Revision 2.5  91/05/14  15:32:00  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/03/16  14:42:23  rpd
 * 	Updated for new kmem_alloc interface.
 * 	[91/03/03            rpd]
 * 
 * Revision 2.3  91/02/05  17:05:55  mrt
 * 	Changed to new Mach copyright
 * 	[91/01/31  16:16:44  mrt]
 * 
 * Revision 2.2  90/08/27  21:48:35  dbg
 * 	Created.
 * 	[90/08/17            dbg]
 * 
 */
/* CMU_ENDHIST */
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
 */
/*
 *	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */
/*
 * Symbol table routines for a.out format files.
 */
#include <mach/boolean.h>
#include <mach/std_types.h>
#include <machine/db_machdep.h>		/* data types */
#include <string.h>			/* For strcpy(), strcmp() */
#include <ddb/db_aout.h>
#include <ddb/db_output.h>		/* For db_printf() */
#include <ddb/db_sym.h>

#ifndef	DB_NO_AOUT

#include <ddb/nlist.h>			/* a.out symbol table */
#include <ddb/stab.h>

#define private static

private int aout_db_order_symbols(char *, char *);
private int aout_db_compare_symbols(char *, char *);
private boolean_t aout_db_is_filename(char *);
private boolean_t aout_db_eq_name(struct nlist *, char *, int);

/*
 * An a.out symbol table as loaded into the kernel debugger:
 *
 * symtab	-> size of symbol entries, in bytes
 * sp		-> first symbol entry
 *		   ...
 * ep		-> last symbol entry + 1
 * strtab	== start of string table
 *		   size of string table in bytes,
 *		   including this word
 *		-> strings
 */

/*
 * Find pointers to the start and end of the symbol entries,
 * given a pointer to the start of the symbol table.
 */
#define	db_get_aout_symtab(symtab, sp, ep) \
	(sp = (struct nlist *)(((vm_offset_t *)(symtab)) + 1), \
	 ep = (struct nlist *)((char *)sp + *((int *)(symtab))))

char *db_sorting_sym_end;

private int
aout_db_order_symbols(
	char	*s1,
	char	*s2)
{
	struct nlist	*sym1 = (struct nlist *) s1;
	struct nlist	*sym2 = (struct nlist *) s2;

	if (sym1->n_value != sym2->n_value) 
		return (sym1->n_value - sym2->n_value);
	else {
		return (sym1->n_un.n_name - sym2->n_un.n_name);
	}
}

private int
aout_db_compare_symbols(
	char	*sym1,
	char	*sym2)
{
	return (((struct nlist *) sym1)->n_value -
		((struct nlist *) sym2)->n_value);
}

int db_sorting_limit = 50000;

boolean_t
aout_db_sym_init(
	char *	symtab,		/* pointer to start of symbol table */
	char *	esymtab,	/* pointer to end of string table,
				   for checking - may be rounded up to
				   integer boundary */
	char *	name,
	char *	task_addr)	/* use for this task only */
{
	register struct nlist	*sym_start, *sym_end;
	register struct nlist	*sp;
	register char *	strtab;
	register int	strlen;
	char *		estrtab;
	unsigned long	minsym = ~0;
	unsigned long	maxsym = 0;
	boolean_t	sorted;
	boolean_t	sorting;

	db_get_aout_symtab(symtab, sym_start, sym_end);

	db_sorting_sym_end = (char *)sym_end;
	strtab = (char *)sym_end;
	strlen = *(int *)strtab;
	estrtab = strtab + strlen;

#define	round_to_size(x) \
	(((vm_offset_t)(x) + sizeof(vm_size_t) - 1) & ~(sizeof(vm_size_t) - 1))

	if (round_to_size(estrtab) != round_to_size(esymtab))
	    return (FALSE);

#undef	round_to_size

	sorting = ((sym_end - sym_start) < db_sorting_limit);

	for (sp = sym_start; sp < sym_end; sp++) {
	    register long strx;
	    strx = sp->n_un.n_strx;
	    if (strx != 0) {
		if (strx > strlen) {
		    sp->n_un.n_name = 0;
		    continue;
		}
		sp->n_un.n_name = strtab + strx;
	    }
	    if (sp->n_type != N_ABS) {
		if (sp->n_value > 0 && sp->n_value < minsym)
		    minsym = sp->n_value;
		if (sp->n_value > maxsym)
		    maxsym = sp->n_value;
	    }
	}
	if (maxsym < minsym)
		minsym = maxsym = 0;

	if (sorting) {
		db_qsort((char *) sym_start, sym_end - sym_start,
		         sizeof (struct nlist), aout_db_order_symbols);
		sorted = TRUE;
	} else
		sorted = FALSE;

	if (db_add_symbol_table(SYMTAB_AOUT,
				(char*)sym_start,
				(char*)sym_end,
				name,
				symtab,
				task_addr,
				minsym,
				maxsym,
				sorted))
	{
	    /* Successfully added symbol table */
	    pmap_protect(kernel_pmap,
			 (vm_offset_t) sym_start, (vm_offset_t) sym_end,
			 VM_PROT_READ|VM_PROT_WRITE);
	    pmap_protect(kernel_pmap,
			 (vm_offset_t) strtab, (vm_offset_t) estrtab,
			 VM_PROT_READ|VM_PROT_WRITE);
	    return TRUE;
	}
	return FALSE;
}

/*
 * This KLUDGE offsets the n_values of a copied symbol table
 */
void db_clone_offsetXXX(char *, long);
void
db_clone_offsetXXX(char * symtab, long offset)
{
	register struct nlist	*sym_start, *sym_end, *sp;

	db_get_aout_symtab((int *)symtab, sym_start, sym_end);

	for (sp = sym_start; sp < sym_end; sp++)
		if (sp->n_type != N_ABS)
			sp->n_value += offset;
}
/* end KLUDGE */

/*
 * check file name or not (check xxxx.x pattern)
 */
private boolean_t
aout_db_is_filename(char *name)
{
	while (*name) {
	    if (*name == '.') {
		if (name[1])
		    return(TRUE);
	    }
	    name++;
	}
	return(FALSE);
}

/*
 * special name comparison routine with a name in the symbol table entry
 */
private boolean_t
aout_db_eq_name(
	struct nlist *sp,
	char *name,
	int incomplete)
{
	register char *s1, *s2;

	s1 = sp->n_un.n_name;
	s2 = name;
#ifndef __NO_UNDERSCORES__
	if (*s1 == '_' && *s2 && *s2 != '_')
	    s1++;
#endif	/* __NO_UNDERSCORES__ */
	while (*s2) {
	    if (*s1++ != *s2++) {
		/*
		 * check .c .o file name comparison case
		 */
		if (*s2 == 0 && sp->n_un.n_name <= s1 - 2 
			&& s1[-2] == '.' && s1[-1] == 'o')
		    return(TRUE);
		return(FALSE);
	    }
	}
	if (incomplete)
	    return(TRUE);
	/*
	 * do special check for
	 *     xxx:yyy for N_FUN
	 *     xxx.ttt for N_DATA and N_BSS
	 */
	return(*s1 == 0 || (*s1 == ':' && sp->n_type == N_FUN) || 
		(*s1 == '.' && (sp->n_type == N_DATA || sp->n_type == N_BSS)));
}

/*
 * search a symbol table with name and type
 *	fp(in,out): last found text file name symbol entry
 */
private struct nlist *
aout_db_search_name(
	struct nlist	*sp,
	struct nlist	*ep,
	char		*name,
	int 		type,
	struct nlist	**fp,
        int 		incomplete)
{
	struct nlist	*file_sp = *fp;
	struct nlist	*found_sp = 0;

	for ( ; sp < ep; sp++) {
	    if (sp->n_other)
		sp->n_other = 0;
	    if (sp->n_type == N_TEXT && aout_db_is_filename(sp->n_un.n_name))
		*fp = sp;
	    if (type) {
		if (sp->n_type == type) {
		    /* dwm_debug: b26 name, mk6 added last param */
		    if (aout_db_eq_name(sp, name, 0))
	    		return(sp);
		}
		if (sp->n_type == N_SO)
		    *fp = sp;
		continue;
	    }
	    if (sp->n_type & N_STAB)
		continue;
	    if (sp->n_un.n_name && aout_db_eq_name(sp, name, incomplete)) {
		/*
		 * In case of qaulified search by a file,
		 * return it immediately with some check.
		 * Otherwise, search external one
		 */
		if (file_sp) {
		    if ((file_sp == *fp) || (sp->n_type & N_EXT))
			return(sp);
		} else if ((sp->n_type & N_EXT) ||
			   (incomplete && !aout_db_is_filename(sp->n_un.n_name)))
		    return(sp);
		else
		    found_sp = sp;
	    }
	}
	return(found_sp);
}

/*
 * Print sorted possible completions for a symbol.
 * Use n_other field to mark completion symbols in order
 *	to speed up sort.
 */
int
aout_db_qualified_print_completion(
	db_symtab_t	*stab,
	char		*sym)
{
	struct nlist	*sp;
	struct nlist	*sp1;
	struct nlist	*ep;
	struct nlist	*ep1;
	struct nlist	*fp = 0;
	int		symlen;
	int		nsym = 0;
	struct nlist	*cur;
	struct nlist	*new;
	char		*fname;
	int		func;
	int		line;

	sp = aout_db_search_name((struct nlist *)stab->start,
			      (struct nlist *)stab->end,
			      sym, 0, &fp, 1);
	if (sp == (struct nlist *)0)
	    return 0;

	symlen = strlen(sym);
	cur = sp;
	while (cur) {
	    if (strncmp(cur->n_un.n_name, sym, symlen) == 0)
		 cur->n_other = 1;
	    else
		 cur->n_other = 2;
	    ep = cur;
	    cur = aout_db_search_name(cur + 1, (struct nlist *)stab->end,
				   sym, 0, &fp, 1);
	}

	sp1 = sp;
	for (;;) {
	    new = cur = sp;
	    while (++cur <= ep)
		if (cur->n_other) {
		   if (sp1 == sp)
			sp1 = cur;
		   if (strcmp(&cur->n_un.n_name[cur->n_other - 1],
			      &new->n_un.n_name[new->n_other - 1]) < 0)
			new = cur;
		   else
			ep1 = cur;
		}

	    func = line = 0;
	    if ((new->n_type & N_EXT) == 0) {
		for (cur = new - 1; cur > (struct nlist *)stab->start; cur--) {
		    if (cur->n_type == N_SO ||
			(stab->sorted && cur->n_value < new->n_value))
			break;
		    if (line == 0 &&
			cur->n_type == N_SLINE &&
			cur->n_value == new->n_value)
			line = cur->n_desc;
		    if (func == 0 &&
			cur->n_type == N_FUN &&
			cur->n_value == new->n_value)
			func = 1;
		}

		if (cur->n_type == N_SO)
		    fname = cur->n_un.n_name;
		else
		    fname = (char *)0;

		if (line == 0 || func == 0)
		    for (cur = new + 1;
			 cur < (struct nlist *)stab->end; cur++) {
			if (cur->n_type == N_SO ||
			    (stab->sorted && cur->n_value > new->n_value))
			    break;
			if (line == 0 &&
			    cur->n_type == N_SLINE &&
			    cur->n_value == new->n_value) {
			    line = cur->n_desc;
			    if (func)
				break;
			}
			if (func == 0 &&
			    cur->n_type == N_FUN &&
			    cur->n_value == new->n_value) {
			    func = 1;
			    if (line)
				break;
			}
		}
	    } else {
		fname = (char *)0;
		for (cur = new - 1; cur > (struct nlist *)stab->start; cur--) {
		    if (cur->n_type == N_SO ||
			(stab->sorted && cur->n_value < new->n_value))
			break;
		    if (func == 0 &&
			cur->n_type == N_FUN &&
			cur->n_value == new->n_value)
			func = 1;
		}
		if (func == 0)
		    for (cur = new + 1;
			 cur < (struct nlist *)stab->end; cur++) {
			if (cur->n_type == N_SO ||
			    (stab->sorted && cur->n_value > new->n_value))
			    break;
			if (cur->n_type == N_FUN &&
			    cur->n_value == new->n_value) {
			    func = 1;
			    break;
			}
		}
	    }

	    db_sym_print_completion(stab, &new->n_un.n_name[new->n_other - 1],
				    func, fname, line);
	    nsym++;
	    new->n_other = 0;

	    if (new == sp) {
		if (sp1 == sp)
		    break;
		sp = sp1;
	    } else if (new == sp1)
		sp1 = sp;

	    if (new == ep)
		ep = ep1;
	}
	return nsym;
}

/*
 * search a (possibly incomplete) symbol with file, func and line qualification
 */
private int
aout_db_qualified_search(
	db_symtab_t	*stab,
	char		*file,
	char		*sym,
	int 		line,
	db_sym_t	*ret,
	char		**name,
	int		*len)
{
	register struct nlist *sp = (struct nlist *)stab->start;
	struct nlist	*ep = (struct nlist *)stab->end;
	struct nlist	*fp = 0;
	struct nlist	*found_sp;
	unsigned long	func_top;
	boolean_t	in_file;
	int		nsym = 0;
	int		i;
	char		*p;

	if (file == 0 && sym == 0)
	    return(0);
	if (file) {
	    if ((sp = aout_db_search_name(sp, ep, file, N_TEXT, &fp, 0)) == 0)
		return(0);
	}
	if (sym) {
	    for (;;) {
		sp = aout_db_search_name(sp, ep, sym, (line > 0)? N_FUN: 0, &fp,
				      (ret == (db_sym_t *)0));
		if (sp == 0)
		    return(nsym);
		if (ret)
		    break;

		if (strncmp(sp->n_un.n_name, sym, strlen(sym)) == 0)
		    p = sp->n_un.n_name;
		else
		    p = &sp->n_un.n_name[1];

		if (*name == (char *)0) {
		    *name = p;
		    *len = strlen(p);
		} else {
		    for (i = 0; i < *len; i++)
			if ((*name)[i] != p[i]) {
			    *len = i;
			    break;
			}
		}

		nsym++;
		sp++;
	    }
	}
	if (line > 0) {
	    if (file && !aout_db_eq_name(fp, file, 0))
		return(0);
	    found_sp = 0;
	    if (sp->n_type == N_FUN) {
		/*
		 * qualfied by function name
		 *     search backward because line number entries
		 *     for the function are above it in this case.
		 */
		func_top = sp->n_value;
		if (stab->sorted) {
		    /* symbols with the same value may have been mixed up */
		    do {
			sp++;
		    } while (sp->n_value == func_top);
	    	}
		for (sp--; sp >= (struct nlist *)stab->start; sp--) {
		    if (sp->n_type != N_SLINE)
			continue;
		    if (sp->n_value < func_top)
			break;
		    if (sp->n_desc <= line) {
			if (found_sp == 0 || found_sp->n_desc < sp->n_desc)
			    found_sp = sp;
			if (sp->n_desc == line)
			    break;
		    }
		}
		if (sp->n_type != N_SLINE || sp->n_value < func_top)
		    return(0);
	    } else {
		/*
		 * qualified by only file name
		 *    search forward in this case
		 */
		in_file = TRUE;
		if (stab->sorted) {
		    /* symbols with the same value may have been mixed up */
		    func_top = sp->n_value;
		    do {
			sp--;
		    } while (sp->n_value == func_top);
		}
		for (sp++; sp < ep; sp++) {
		    if (sp->n_type == N_TEXT 
			&& aout_db_is_filename(sp->n_un.n_name))
			break;		/* enter into another file */
		    if (sp->n_type == N_SOL) {
			in_file = aout_db_eq_name(sp, file, 0);
			continue;
		    }
		    if (!in_file || sp->n_type != N_SLINE)
			continue;
		    if (sp->n_desc <= line) {
			if (found_sp == 0 || found_sp->n_desc < sp->n_desc)
			    found_sp = sp;
			if (sp->n_desc == line)
			    break;
		    }
		}
	    }
	    sp = found_sp;
	}
	*ret = (db_sym_t) sp;
	return(1);
}

/*
 * lookup symbol by name
 */
db_sym_t
aout_db_lookup(
	db_symtab_t	*stab,
	char *		symstr)
{
	return(db_sym_parse_and_lookup(aout_db_qualified_search, stab, symstr));
}

/*
 * lookup (possibly incomplete) symbol by name
 */
int
aout_db_lookup_incomplete(
	db_symtab_t	*stab,
	char *		symstr,
	char **		name,
	int		*len,
	int		*toadd)
{
	return(db_sym_parse_and_lookup_incomplete(aout_db_qualified_search,
					stab, symstr, name, len, toadd));
}

/*
 * Display possible completion for the symbol
 */
int
aout_db_print_completion(stab, symstr)
	db_symtab_t	*stab;
	char *		symstr;
{

	return(db_sym_parse_and_print_completion(aout_db_qualified_print_completion,
						 stab, symstr));
}

db_sym_t
aout_db_search_symbol(
	db_symtab_t	*symtab,
	db_addr_t	off,
	db_strategy_t	strategy,
	db_expr_t	*diffp)		/* in/out */
{
	register unsigned long	diff = *diffp;
	register struct nlist	*symp = 0;
	struct nlist		*sp, *ep, *cp;
	boolean_t		first_pass = FALSE;

	sp = (struct nlist *)symtab->start;
	ep = (struct nlist *)symtab->end;

	if (symtab->sorted) {
	    struct nlist target;

	    target.n_value = off;
	    target.n_un.n_name = (char *) 0;
	    target.n_other = (char) 0;
	    db_qsort_limit_search((char *) &target, (char **) &sp, (char **) &ep,
			          sizeof (struct nlist), aout_db_compare_symbols);
	    first_pass = TRUE;
	}

    try_again:
	for (cp = ep-1; cp >= sp; cp--) {
	    if (cp->n_un.n_name == 0)
		continue;
	    if ((cp->n_type & N_STAB) != 0)
		continue;
	    if (strategy == DB_STGY_XTRN && (cp->n_type & N_EXT) == 0)
		continue;
	    if (off >= cp->n_value) {
		if (off - cp->n_value < diff) {
		    diff = off - cp->n_value;
		    symp = cp;
		    if (diff == 0 && (cp->n_type & N_EXT))
			    break;
		}
		else if (off - cp->n_value == diff) {
		    if (symp == 0)
			symp = cp;
		    else if ((symp->n_type & N_EXT) == 0 &&
				(cp->n_type & N_EXT) != 0)
			symp = cp;	/* pick the external symbol */
		}
	    }
	}
	if (symp == 0) {
	    if (first_pass) {
		first_pass = FALSE;
		sp = (struct nlist *) symtab->start;
		goto try_again;
	    }
	    *diffp = off;
	}
	else {
	    *diffp = diff;
	}
	return ((db_sym_t)symp);
}

/*
 * Return the name and value for a symbol.
 */
void
aout_db_symbol_values(
	db_sym_t	sym,
	char		**namep,
	db_expr_t	*valuep)
{
	register struct nlist *sp;

	sp = (struct nlist *)sym;
	if (namep)
	    *namep = sp->n_un.n_name;
	if (valuep)
	    *valuep = sp->n_value;
}

#define X_DB_MAX_DIFF	8	/* maximum allowable diff at the end of line */
extern int 	db_search_maxoff;	/* maximum acceptable offset */

/*
 * search symbol by value
 */
db_sym_t
aout_db_search_by_addr(
	db_symtab_t	*stab,
	db_addr_t	addr,
	char		**file,
	char		**func,
	int 	 	*line,
	db_expr_t	*diff,
        int             *args)
{
	struct nlist	*sp, *cp;
	register	struct nlist *line_sp, *func_sp, *file_sp, *line_func;
	unsigned long	func_diff, line_diff;
	boolean_t	found_line = FALSE;
	struct 	  	nlist *ep = (struct nlist *)stab->end;
	boolean_t	first_pass = FALSE;

	/*
	 * 92-May-16
	 * Added init of these two... not sure if it's correct, but
	 * can't be worse than random values....  -- jfriedl@omron.co.jp
	 */
	func_diff = line_diff = /*HUGE*/0x0fffffff;

	line_sp = func_sp = file_sp = line_func = 0;
	*file = *func = 0;
	*line = 0;
	*args = -1;

	sp = (struct nlist *)stab->start;
	if (stab->sorted) {
		struct nlist target;

		target.n_value = addr;
		target.n_un.n_name = (char *) 0;
		target.n_other = (char) 0;
		db_qsort_limit_search((char *) &target, (char **) &sp,
				      (char **) &ep, sizeof (struct nlist),
				      aout_db_compare_symbols);
		first_pass = TRUE;
	}

	for (cp = sp; cp < ep; cp++) {
	    switch(cp->n_type) {
	    case N_SLINE:
		if (cp->n_value <= addr) {
		    if (line_sp == 0 || line_diff >= addr - cp->n_value) {
			if (line_func)
			    line_func = 0;
			line_sp = cp;
			line_diff = addr - cp->n_value;
		    }
		}
		if (cp->n_value >= addr && line_sp)
		    found_line = TRUE;
		continue;
	    case N_FUN:
		if ((found_line || (line_sp && line_diff < X_DB_MAX_DIFF))
		    && line_func == 0)
		    line_func = cp;
		continue;
	    case N_SO:
		if (cp->n_value > addr)
		    continue;
		if (file_sp == 0 || file_sp->n_value <= cp->n_value)
		    file_sp = cp;
		continue;
	    case N_TEXT:
		if (aout_db_is_filename(cp->n_un.n_name)) {
		    if (cp->n_value > addr)
			continue;
		    if (file_sp == 0 || file_sp->n_value <= cp->n_value)
			file_sp = cp;
		} else if (cp->n_value <= addr &&
			 (func_sp == 0 || func_diff > addr - cp->n_value)) {
		    func_sp = cp;
		    func_diff = addr - cp->n_value;
		}
		continue;
	    case N_TEXT|N_EXT:
		if (cp->n_value <= addr &&
			 (func_sp == 0 || func_diff >= addr - cp->n_value)) {
		    func_sp = cp;
		    func_diff = addr - cp->n_value;
		    if (func_diff == 0 && file_sp && func_sp && line_sp == 0)
		        break;
		}
	    default:
		if (stab->sorted) {
			if ((cp->n_value > addr) &&
			    (cp->n_value - addr > db_search_maxoff))
				break;
		}
		continue;
	    }
	    break;
	}
	if (first_pass && (!file_sp || !line_sp || !func_sp)) {
	    first_pass = FALSE;
	    cp = sp;
	    sp = (struct nlist *)stab->start;
	    for (; cp >= sp; cp--) {
		switch(cp->n_type) {
		case N_SLINE:
		    if (line_sp)
			found_line = TRUE;
		    continue;
		case N_FUN:
		    if ((found_line || (line_sp && line_diff < X_DB_MAX_DIFF))
			&& line_func == 0)
			line_func = cp;
		    continue;
		case N_SO:
		    if (file_sp == 0)
			file_sp = cp;
		    continue;
		case N_TEXT:
		    if (aout_db_is_filename(cp->n_un.n_name)) {
			if (file_sp == 0)
			    file_sp = cp;
		    } else if (func_sp == 0) {
			func_sp = cp;
			func_diff = addr - cp->n_value;
		    }
		    continue;
		case N_TEXT|N_EXT:
		    if (func_sp == 0) {
			func_sp = cp;
			func_diff = addr - cp->n_value;
			if (func_diff == 0 && file_sp && func_sp
			    && line_sp == 0)
			    break;
		    }
		default:
		    if (line_sp && file_sp &&
			addr - cp->n_value > db_search_maxoff)
			break;
		    continue;
		}
		break;
	    }
	}
#if 0	
/*
 * XXX - barbou@gr.osf.org
 * I don't know if that code is useful to something, but it makes the -gline
 * option of gcc useless.
 */
	if (line_sp) {
	    if (line_func == 0 || func_sp == 0
		|| line_func->n_value != func_sp->n_value)
		line_sp = 0;
	}
#else
	if (line_sp && !found_line) {
		line_sp = 0;
	}
#endif
	*diff = 0;
	if (file_sp) {
	    *diff = addr - file_sp->n_value;
	    *file = file_sp->n_un.n_name;
	}
	if (line_sp) {
	    *diff = addr - line_sp->n_value;
	    *line = line_sp->n_desc;
	}
	if (func_sp) {
	    *diff = addr - func_sp->n_value;
	    *func = (func_sp->n_un.n_name[0] == '_')?
			func_sp->n_un.n_name + 1: func_sp->n_un.n_name;
	    if (line_func && (line_func->n_desc & 0x4000))
		*args = line_func->n_desc & 0x3ff;
	}
	return((db_sym_t) func_sp);
}

/*
 * Find filename and lineno within, given the current pc.
 */
boolean_t
aout_db_line_at_pc(
	db_symtab_t	*stab,
	db_sym_t	sym,
	char		**file,
	int		*line,
	db_expr_t	pc)
{
	char		*func;
	db_expr_t	diff;
	boolean_t	found;
	int		args;

	found = (aout_db_search_by_addr(stab, (unsigned)pc, file, &func, line,
				     &diff, &args)
		 != DB_SYM_NULL);
	return(found && func && *file);
}

/*
 * Initialization routine for a.out files.
 */
void
aout_db_init(void)
{
	extern vm_offset_t	kern_sym_start;
	extern vm_size_t	kern_sym_size;

	if (kern_sym_size != 0)
	    aout_db_sym_init((char *) kern_sym_start,
			  (char *)(kern_sym_start + kern_sym_size),
			  "mach",
			  (char *)0);
}

#endif	/* DB_NO_AOUT */
