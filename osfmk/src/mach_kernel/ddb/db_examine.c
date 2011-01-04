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
 * Revision 2.7  91/10/09  15:59:28  af
 * 	 Revision 2.6.1.1  91/10/05  13:05:49  jeffreyh
 * 	 	Supported non current task space data examination and search.
 * 	 	Added 'm' format and db_xcdump to print with hex and characters.
 * 	 	Added db_examine_{forward, backward}.
 * 	 	Changed db_print_cmd to support variable number of parameters
 * 	 	including string constant.
 * 	 	Included "db_access.h".
 * 	 	[91/08/29            tak]
 * 
 * Revision 2.6.1.1  91/10/05  13:05:49  jeffreyh
 * 	Supported non current task space data examination and search.
 * 	Added 'm' format and db_xcdump to print with hex and characters.
 * 	Added db_examine_{forward, backward}.
 * 	Changed db_print_cmd to support variable number of parameters
 * 	including string constant.
 * 	Included "db_access.h".
 * 	[91/08/29            tak]
 * 
 * Revision 2.6  91/08/28  11:11:01  jsb
 * 	Added 'A' flag to examine: just like 'a' (address), but prints addr
 * 	as a procedure type, thus printing file/line info if available.
 * 	Useful when called as 'x/Ai'.
 * 	[91/08/13  18:14:55  jsb]
 * 
 * Revision 2.5  91/05/14  15:33:31  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:06:20  mrt
 * 	Changed to new Mach copyright
 * 	[91/01/31  16:17:37  mrt]
 * 
 * Revision 2.3  90/11/07  16:49:23  rpd
 * 	Added db_search_cmd, db_search.
 * 	[90/11/06            rpd]
 * 
 * Revision 2.2  90/08/27  21:50:38  dbg
 * 	Add 'r', 'z' to print and examine formats.
 * 	Change calling sequence of db_disasm.
 * 	db_examine sets db_prev and db_next instead of explicitly
 * 	advancing dot.
 * 	[90/08/20            dbg]
 * 	Reflected changes in db_printsym()'s calling seq.
 * 	[90/08/20            af]
 * 	Reduce lint.
 * 	[90/08/07            dbg]
 * 	Created.
 * 	[90/07/25            dbg]
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
#include <string.h>			/* For strcpy() */
#include <mach/boolean.h>
#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_lex.h>
#include <ddb/db_output.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_task_thread.h>
#include <ddb/db_command.h>		/* For db_option() */
#include <ddb/db_examine.h>
#include <ddb/db_expr.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <mach/vm_param.h>

#define db_act_to_task(thr_act)	((thr_act)? thr_act->task: TASK_NULL)

char		db_examine_format[TOK_STRING_SIZE] = "x";
int		db_examine_count = 1;
db_addr_t	db_examine_prev_addr = 0;
thread_act_t	db_examine_act = THR_ACT_NULL;

extern int	db_max_width;


/* Prototypes for functions local to this file.  XXX -- should be static!
 */
int db_xcdump(
	db_addr_t	addr,
	int		size,
	int		count,
	task_t		task);

int db_examine_width(
	int size,
	int *items,
	int *remainder);

/*
 * Examine (print) data.
 */
void
db_examine_cmd(
	db_expr_t	addr,
	int		have_addr,
	db_expr_t	count,
	char *		modif)
{
	thread_act_t	thr_act;
	extern char	db_last_modifier[];

	if (modif[0] != '\0')
	    strcpy(db_examine_format, modif);

	if (count == -1)
	    count = 1;
	db_examine_count = count;
	if (db_option(modif, 't')) {
	    if (modif == db_last_modifier)
		thr_act = db_examine_act;
	    else if (!db_get_next_act(&thr_act, 0))
		return;
	} else
	  if (db_option(modif,'u'))
	    thr_act = current_act();
	  else
	    thr_act = THR_ACT_NULL;

	db_examine_act = thr_act;
	db_examine((db_addr_t) addr, db_examine_format, count, 
					db_act_to_task(thr_act));
}

void
db_examine_forward(
	db_expr_t	addr,
	int		have_addr,
	db_expr_t	count,
	char *		modif)
{
	db_examine(db_next, db_examine_format, db_examine_count,
				db_act_to_task(db_examine_act));
}

void
db_examine_backward(
	db_expr_t	addr,
	int		have_addr,
	db_expr_t	count,
	char *		modif)
{
	db_examine(db_examine_prev_addr - (db_next - db_examine_prev_addr),
			 db_examine_format, db_examine_count,
				db_act_to_task(db_examine_act));
}

int
db_examine_width(
	int size,
	int *items,
	int *remainder)
{
	int sz;
	int entry;
	int width;

	width = size * 2 + 1;
	sz = (db_max_width - (sizeof (void *) * 2 + 4)) / width;
	for (entry = 1; (entry << 1) < sz; entry <<= 1)
		continue;

	sz = sizeof (void *) * 2 + 4 + entry * width;
	while (sz + entry < db_max_width) {
		width++;
		sz += entry;
	}
	*remainder = (db_max_width - sz + 1) / 2;
	*items = entry;
	return width;
}

void
db_examine(
	db_addr_t	addr,
	char *		fmt,	/* format string */
	int		count,	/* repeat count */
	task_t		task)
{
	int		c;
	db_expr_t	value;
	int		size;
	int		width;
	int		leader;
	int		items;
	int		nitems;
	char *		fp;
	db_addr_t	next_addr;
	int		sz;

	db_examine_prev_addr = addr;
	while (--count >= 0) {
	    fp = fmt;
	    size = sizeof(int);
	    width = db_examine_width(size, &items, &leader);
	    while ((c = *fp++) != 0) {
		switch (c) {
		    case 'b':
			size = sizeof(char);
			width = db_examine_width(size, &items, &leader);
			break;
		    case 'h':
			size = sizeof(short);
			width = db_examine_width(size, &items, &leader);
			break;
		    case 'l':
			size = sizeof(int);
			width = db_examine_width(size, &items, &leader);
			break;
		    case 'q':
			size = sizeof(long);
			width = db_examine_width(size, &items, &leader);
			break;
		    case 'a':	/* address */
		    case 'A':   /* function address */
			/* always forces a new line */
			if (db_print_position() != 0)
			    db_printf("\n");
			db_prev = addr;
			next_addr = addr + 4;
			db_task_printsym(addr, 
					(c == 'a')?DB_STGY_ANY:DB_STGY_PROC,
					task);
			db_printf(":\t");
			break;
		    case 'm':
			db_next = db_xcdump(addr, size, count+1, task);
			return;
		    case 't':
		    case 'u':
			break;
		    default:
		restart:
			/* Reset next_addr in case we are printing in
			   multiple formats.  */
			next_addr = addr;
			if (db_print_position() == 0) {
			    /* If we hit a new symbol, print it */
			    char *	name;
			    db_addr_t	off;

			    db_find_task_sym_and_offset(addr,&name,&off,task);
			    if (off == 0)
				db_printf("\r%s:\n", name);
			    db_printf("%#n: ", addr);
			    for (sz = 0; sz < leader; sz++)
				    db_putchar(' ');
			    db_prev = addr;
			    nitems = items;
			}

			switch (c) {
			    case 'p':	/* Addrs rendered symbolically. */
				if( size == sizeof(void *) )  {
				    char       *symName;
				    db_addr_t	offset;

				    items = 1;
				    value = db_get_task_value( next_addr,
					sizeof(db_expr_t), FALSE, task );
				    db_find_task_sym_and_offset( value,
					&symName, &offset, task);
				    db_printf("\n\t*%8x(%8X) = %s",
						next_addr, value, symName );
				    if( offset )  {
					db_printf("+%X", offset );
				    }
				    next_addr += size;
				}
				break;
			    case 'r':	/* signed, current radix */
				for (sz = size, next_addr = addr;
				     sz >= sizeof (db_expr_t);
				     sz -= sizeof (db_expr_t)) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr,
							      sizeof (db_expr_t),
							      TRUE,task);
				    db_printf("%-*r", width, value);
				    next_addr += sizeof (db_expr_t);
				}
				if (sz > 0) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr, sz,
							      TRUE, task);
				    db_printf("%-*R", width, value);
				    next_addr += sz;
				}
				break;
			    case 'x':	/* unsigned hex */
				for (sz = size, next_addr = addr;
				     sz >= sizeof (db_expr_t);
				     sz -= sizeof (db_expr_t)) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr,
							      sizeof (db_expr_t),
							      FALSE,task);
				    db_printf("%-*x", width, value);
				    next_addr += sizeof (db_expr_t);
			        }
				if (sz > 0) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr, sz,
							      FALSE, task);
				    db_printf("%-*X", width, value);
				    next_addr += sz;
				}
				break;
			    case 'z':	/* signed hex */
				for (sz = size, next_addr = addr;
				     sz >= sizeof (db_expr_t);
				     sz -= sizeof (db_expr_t)) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr,
							      sizeof (db_expr_t),
							      TRUE, task);
				    db_printf("%-*z", width, value);
				    next_addr += sizeof (db_expr_t);
				}
				if (sz > 0) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr,sz,
							      TRUE,task);
				    db_printf("%-*Z", width, value);
				    next_addr += sz;
				}
				break;
			    case 'd':	/* signed decimal */
				for (sz = size, next_addr = addr;
				     sz >= sizeof (db_expr_t);
				     sz -= sizeof (db_expr_t)) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr,
							      sizeof (db_expr_t),
							      TRUE,task);
				    db_printf("%-*d", width, value);
				    next_addr += sizeof (db_expr_t);
				}
				if (sz > 0) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr, sz,
							      TRUE, task);
				    db_printf("%-*D", width, value);
				    next_addr += sz;
				}
				break;
			    case 'U':	/* unsigned decimal */
			    case 'u':
				for (sz = size, next_addr = addr;
				     sz >= sizeof (db_expr_t);
				     sz -= sizeof (db_expr_t)) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr,
							      sizeof (db_expr_t),
							      FALSE,task);
				    db_printf("%-*u", width, value);
				    next_addr += sizeof (db_expr_t);
				}
				if (sz > 0) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr, sz,
							      FALSE, task);
				    db_printf("%-*U", width, value);
				    next_addr += sz;
				}
				break;
			    case 'o':	/* unsigned octal */
				for (sz = size, next_addr = addr;
				     sz >= sizeof (db_expr_t);
				     sz -= sizeof (db_expr_t)) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr,
							      sizeof (db_expr_t),
							      FALSE,task);
				    db_printf("%-*o", width, value);
				    next_addr += sizeof (db_expr_t);
				}
				if (sz > 0) {
				    if (nitems-- == 0) {
					db_putchar('\n');
					goto restart;
				    }
				    value = db_get_task_value(next_addr, sz,
							      FALSE, task);
				    db_printf("%-*o", width, value);
				    next_addr += sz;
				}
				break;
			    case 'c':	/* character */
				for (sz = 0, next_addr = addr;
				     sz < size;
				     sz++, next_addr++) {
				    value = db_get_task_value(next_addr,1,
							      FALSE,task);
				    if ((value >= ' ' && value <= '~') ||
					value == '\n' ||
					value == '\t')
					    db_printf("%c", value);
				    else
					    db_printf("\\%03o", value);
				}
				break;
			    case 's':	/* null-terminated string */
				size = 0;
				for (;;) {
				    value = db_get_task_value(next_addr,1,
							      FALSE,task);
				    next_addr += 1;
				    size++;
				    if (value == 0)
					break;
				    if (value >= ' ' && value <= '~')
					db_printf("%c", value);
				    else
					db_printf("\\%03o", value);
				}
				break;
			    case 'i':	/* instruction */
				next_addr = db_disasm(addr, FALSE, task);
				size = next_addr - addr;
				break;
			    case 'I':	/* instruction, alternate form */
				next_addr = db_disasm(addr, TRUE, task);
				size = next_addr - addr;
				break;
			    default:
				break;
			}
			if (db_print_position() != 0)
			    db_end_line();
			break;
		}
	    }
	    addr = next_addr;
	}
	db_next = addr;
}

/*
 * Print value.
 */
char	db_print_format = 'x';

void
db_print_cmd(void)
{
	db_expr_t	value;
	int		t;
	task_t		task = TASK_NULL;

	if ((t = db_read_token()) == tSLASH) {
	    if (db_read_token() != tIDENT) {
		db_printf("Bad modifier \"/%s\"\n", db_tok_string);
		db_error(0);
		/* NOTREACHED */
	    }
	    if (db_tok_string[0])
		db_print_format = db_tok_string[0];
	    if (db_option(db_tok_string, 't')) {
		if (db_default_act)
		    task = db_default_act->task;
		if (db_print_format == 't')
		   db_print_format = db_tok_string[1];
	    }
	} else
	    db_unread_token(t);
	
	for ( ; ; ) {
	    t = db_read_token();
	    if (t == tSTRING) {
		db_printf("%s", db_tok_string);
		continue;
	    }
	    db_unread_token(t);
	    if (!db_expression(&value))
		break;
	    switch (db_print_format) {
	    case 'a':
	    case 'A':
		db_task_printsym((db_addr_t)value,
				 (db_print_format == 'a') ? DB_STGY_ANY:
				 			    DB_STGY_PROC,
				 task);
		break;
	    case 'r':
		db_printf("%11r", value);
		break;
	    case 'x':
		db_printf("%08x", value);
		break;
	    case 'z':
		db_printf("%8z", value);
		break;
	    case 'd':
		db_printf("%11d", value);
		break;
	    case 'u':
		db_printf("%11u", value);
		break;
	    case 'o':
		db_printf("%16o", value);
		break;
	    case 'c':
		value = value & 0xFF;
		if (value >= ' ' && value <= '~')
		    db_printf("%c", value);
		else
		    db_printf("\\%03o", value);
		break;
	    default:
		db_printf("Unknown format %c\n", db_print_format);
		db_print_format = 'x';
		db_error(0);
	    }
	}
}

void
db_print_loc(
	db_addr_t       loc,
	task_t          task)
{
	db_task_printsym(loc, DB_STGY_PROC, task);
}

void
db_print_inst(
	db_addr_t       loc,
	task_t          task)
{
	(void) db_disasm(loc, TRUE, task);
}

void
db_print_loc_and_inst(
	db_addr_t	loc,
	task_t		task)
{
	db_task_printsym(loc, DB_STGY_PROC, task);
	db_printf(":\t");
	(void) db_disasm(loc, TRUE, task);
}

/*
 * Search for a value in memory.
 * Syntax: search [/bhl] addr value [mask] [,count] [thread]
 */
void
db_search_cmd(void)
{
	int		t;
	db_addr_t	addr;
	int		size = 0;
	db_expr_t	value;
	db_expr_t	mask;
	db_addr_t	count;
	thread_act_t	thr_act;
	boolean_t	thread_flag = FALSE;
	register char	*p;

	t = db_read_token();
	if (t == tSLASH) {
	    t = db_read_token();
	    if (t != tIDENT) {
	      bad_modifier:
		db_printf("Bad modifier \"/%s\"\n", db_tok_string);
		db_flush_lex();
		return;
	    }

	    for (p = db_tok_string; *p; p++) {
		switch(*p) {
		case 'b':
		    size = sizeof(char);
		    break;
		case 'h':
		    size = sizeof(short);
		    break;
		case 'l':
		    size = sizeof(long);
		    break;
		case 't':
		    thread_flag = TRUE;
		    break;
		default:
		    goto bad_modifier;
		}
	    }
	} else {
	    db_unread_token(t);
	    size = sizeof(int);
	}

	if (!db_expression((db_expr_t *) &addr)) {
	    db_printf("Address missing\n");
	    db_flush_lex();
	    return;
	}

	if (!db_expression(&value)) {
	    db_printf("Value missing\n");
	    db_flush_lex();
	    return;
	}

	if (!db_expression(&mask))
	    mask = ~0;

	t = db_read_token();
	if (t == tCOMMA) {
	    if (!db_expression((db_expr_t *) &count)) {
		db_printf("Count missing\n");
		db_flush_lex();
		return;
	    }
	} else {
	    db_unread_token(t);
	    count = -1;		/* effectively forever */
	}
	if (thread_flag) {
	    if (!db_get_next_act(&thr_act, 0))
		return;
	} else
	    thr_act = THR_ACT_NULL;

	db_search(addr, size, value, mask, count, db_act_to_task(thr_act));
}

void
db_search(
	db_addr_t	addr,
	int		size,
	db_expr_t	value,
	db_expr_t	mask,
	unsigned int	count,
	task_t		task)
{
	while (count-- != 0) {
		db_prev = addr;
		if ((db_get_task_value(addr,size,FALSE,task) & mask) == value)
			break;
		addr += size;
	}
	db_printf("0x%x: ", addr);
	db_next = addr;
}

#define DB_XCDUMP_NC	16

int
db_xcdump(
	db_addr_t	addr,
	int		size,
	int		count,
	task_t		task)
{
	register int 	i, n;
	db_expr_t	value;
	int		bcount;
	db_addr_t	off;
	char		*name;
	char		data[DB_XCDUMP_NC];

	db_find_task_sym_and_offset(addr, &name, &off, task);
	for (n = count*size; n > 0; n -= bcount) {
	    db_prev = addr;
	    if (off == 0) {
		db_printf("%s:\n", name);
		off = -1;
	    }
	    db_printf("%0*X:%s", 2*sizeof(db_addr_t), addr,
					(size != 1) ? " " : "" );
	    bcount = ((n > DB_XCDUMP_NC)? DB_XCDUMP_NC: n);
	    if (trunc_page(addr) != trunc_page(addr+bcount-1)) {
		db_addr_t next_page_addr = trunc_page(addr+bcount-1);
		if (!DB_CHECK_ACCESS(next_page_addr, sizeof(int), task))
		    bcount = next_page_addr - addr;
	    }
	    db_read_bytes((vm_offset_t)addr, bcount, data, task);
	    for (i = 0; i < bcount && off != 0; i += size) {
		if (i % 4 == 0)
			db_printf(" ");
		value = db_get_task_value(addr, size, FALSE, task);
		db_printf("%0*x ", size*2, value);
		addr += size;
		db_find_task_sym_and_offset(addr, &name, &off, task);
	    }
	    db_printf("%*s",
			((DB_XCDUMP_NC-i)/size)*(size*2+1)+(DB_XCDUMP_NC-i)/4,
			 "");
	    bcount = i;
	    db_printf("%s*", (size != 1)? " ": "");
	    for (i = 0; i < bcount; i++) {
		value = data[i];
		db_printf("%c", (value >= ' ' && value <= '~')? value: '.');
	    }
	    db_printf("*\n");
	}
	return(addr);
}
