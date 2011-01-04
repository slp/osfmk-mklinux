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
 * Revision 2.5  91/10/09  15:59:46  af
 * 	 Revision 2.4.3.1  91/10/05  13:06:04  jeffreyh
 * 	 	Added relational expression etc. to support condition expression.
 * 	 	Supported modifier after indirect expression to specify size,
 * 	 	  sign extention and non current task space indirection.
 * 	 	Changed error messages to print more information.
 * 	 	[91/08/29            tak]
 * 
 * Revision 2.4.3.1  91/10/05  13:06:04  jeffreyh
 * 	Added relational expression etc. to support condition expression.
 * 	Supported modifier after indirect expression to specify size,
 * 	  sign extention and non current task space indirection.
 * 	Changed error messages to print more information.
 * 	[91/08/29            tak]
 * 
 * Revision 2.4  91/05/14  15:33:45  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:06:25  mrt
 * 	Changed to new Mach copyright
 * 	[91/01/31  16:17:46  mrt]
 * 
 * Revision 2.2  90/08/27  21:50:57  dbg
 * 	Use '..' instead of '$$' for db_prev.
 * 	Use '+' for db_next.
 * 	[90/08/22            dbg]
 * 
 * 	Allow repeated unary operators.
 * 	[90/08/20            dbg]
 * 
 * 	Reflected back rename of db_symbol_value->db_value_of_name
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

#include <mach/boolean.h>
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_expr.h>
#include <ddb/db_lex.h>
#include <ddb/db_output.h>		/* For db_printf() */
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <kern/task.h>



/* Prototypes for functions local to this file.  XXX -- should be static!
 */
boolean_t db_term(db_expr_t *valuep);
boolean_t db_unary(db_expr_t *valuep);
boolean_t db_mult_expr(db_expr_t *valuep);
boolean_t db_add_expr(db_expr_t *valuep);
boolean_t db_shift_expr(db_expr_t *valuep);
boolean_t db_logical_relation_expr(db_expr_t *valuep);
boolean_t db_logical_and_expr(db_expr_t *valuep);
boolean_t db_logical_or_expr(db_expr_t *valuep);


/* try to interpret unknown symbols as hexadecimal constants */
int db_allow_unprefixed_hexa = 1;

boolean_t
db_term(db_expr_t *valuep)
{
	int	t;
	boolean_t valid_symbol = FALSE;
	boolean_t valid_hexa = FALSE;

	switch(t = db_read_token()) {
	case tIDENT:
	    if (db_value_of_name(db_tok_string, valuep)) {
		valid_symbol = TRUE;
	    }
	    if (db_allow_unprefixed_hexa && db_radix == 16 &&
		db_tok_string) {
		char *cp;
		int value;
		    
		value = 0;
		valid_hexa = TRUE;
		for (cp = db_tok_string; *cp; cp++) {
		    if (*cp >= 'a' && *cp <= 'f') {
			value = value * 16 + 10 + (*cp - 'a');
		    } else if (*cp >= 'A' && *cp <= 'F') {
			value = value * 16 + 10 + (*cp - 'A');
		    } else if (*cp >= '0' && *cp <= '9') {
			value = value * 16 + (*cp - '0');
		    } else {
			valid_hexa = FALSE;
			break;
		    }
		}
		if (valid_hexa) {
		    if (valid_symbol) {
			db_printf("Ambiguous constant %x used as a symbol\n",
				  value);
		    } else {
			*valuep = (db_expr_t)value;
		    }	
		}
	    }
	    if (!valid_symbol && !valid_hexa) {
		db_printf("Symbol \"%s\" not found\n", db_tok_string);
		db_error(0);
		/*NOTREACHED*/
	    }
	    return (TRUE);
	case tNUMBER:
	    *valuep = /*(db_expr_t)*/db_tok_number;
	    return (TRUE);
	case tDOT:
	    *valuep = (db_expr_t)db_dot;
	    return (TRUE);
	case tDOTDOT:
	    *valuep = (db_expr_t)db_prev;
	    return (TRUE);
	case tPLUS:
	    *valuep = (db_expr_t) db_next;
	    return (TRUE);
	case tQUOTE:
	    *valuep = (db_expr_t)db_last_addr;
	    return (TRUE);
	case tDOLLAR:
	    if (!db_get_variable(valuep))
		return (FALSE);
	    return (TRUE);
	case tLPAREN:
	    if (!db_expression(valuep)) {
		db_error("Unmached ()s\n");
		/*NOTREACHED*/
	    }
	    t = db_read_token();
	    if (t != tRPAREN) {
		db_printf("')' expected at \"%s...\"\n", db_tok_string);
		db_error(0);
		/*NOTREACHED*/
	    }
	    return (TRUE);
	case tSTRING:
	    {
		    static int db_tok_offset = 0;
		    char *sp, *cp;

		    sp = (char *)db_tok_string + db_tok_offset;
		    *valuep = *(int *)sp;
		    for (cp = sp;
			 *cp && cp < sp + sizeof (int);
			 cp++);
		    if (cp == sp + sizeof (int) && *cp) {
			    db_tok_offset += sizeof (int);
			    db_unread_token(t);
		    } else {
			    db_tok_offset = 0;
		    }
		    return (TRUE);
	    }
	default:
	    db_unread_token(t);
	    return (FALSE);
	}
}

int
db_size_option(
	char		*modif,
	boolean_t	*u_option,
	boolean_t	*t_option)
{
	register  char *p;
	int	  size = sizeof(int);

	*u_option = FALSE;
	*t_option = FALSE;
	for (p = modif; *p; p++) {
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
	    case 'u':
		*u_option = TRUE;
		break;
	    case 't':
		*t_option = TRUE;
		break;
	    }
	}
	return(size);
}

boolean_t
db_unary(db_expr_t *valuep)
{
	int	  t;
	int	  size;
	boolean_t u_opt, t_opt;
	task_t	  task;
	extern	  task_t db_default_task;

	t = db_read_token();
	if (t == tMINUS) {
	    if (!db_unary(valuep)) {
		db_error("Expression syntax error after '-'\n");
		/*NOTREACHED*/
	    }
	    *valuep = -*valuep;
	    return (TRUE);
	}
	if (t == tSTAR) {
	    /* indirection */
	    if (!db_unary(valuep)) {
		db_error("Expression syntax error after '*'\n");
		/*NOTREACHED*/
	    }
	    task = TASK_NULL;
	    size = sizeof(db_addr_t);
	    u_opt = FALSE;
	    t = db_read_token();
	    if (t == tIDENT && db_tok_string[0] == ':') {
		size = db_size_option(&db_tok_string[1], &u_opt, &t_opt);
		if (t_opt)
		    task = db_default_task;
	    } else
		db_unread_token(t);
	    *valuep = db_get_task_value((db_addr_t)*valuep, size, !u_opt, task);
	    return (TRUE);
	}
	if (t == tEXCL) {
	    if (!db_unary(valuep)) {
		db_error("Expression syntax error after '!'\n");
		/*NOTREACHED*/
	    }
	    *valuep = (!(*valuep));
	    return (TRUE);
	}
	db_unread_token(t);
	return (db_term(valuep));
}

boolean_t
db_mult_expr(db_expr_t *valuep)
{
	db_expr_t	lhs, rhs;
	int		t;
	char		c;

	if (!db_unary(&lhs))
	    return (FALSE);

	t = db_read_token();
	while (t == tSTAR || t == tSLASH || t == tPCT || t == tHASH
		|| t == tBIT_AND) {
	    c = db_tok_string[0];
	    if (!db_term(&rhs)) {
		db_printf("Expression syntax error after '%c'\n", c);
		db_error(0);
		/*NOTREACHED*/
	    }
	    switch(t) {
	    case tSTAR:
		lhs *= rhs;
		break;
	    case tBIT_AND:
		lhs &= rhs;
		break;
	    default:
		if (rhs == 0) {
		    db_error("Divide by 0\n");
		    /*NOTREACHED*/
		}
		if (t == tSLASH)
		    lhs /= rhs;
		else if (t == tPCT)
		    lhs %= rhs;
		else
		    lhs = ((lhs+rhs-1)/rhs)*rhs;
	    }
	    t = db_read_token();
	}
	db_unread_token(t);
	*valuep = lhs;
	return (TRUE);
}

boolean_t
db_add_expr(db_expr_t *valuep)
{
	db_expr_t	lhs, rhs;
	int		t;
	char		c;

	if (!db_mult_expr(&lhs))
	    return (FALSE);

	t = db_read_token();
	while (t == tPLUS || t == tMINUS || t == tBIT_OR) {
	    c = db_tok_string[0];
	    if (!db_mult_expr(&rhs)) {
		db_printf("Expression syntax error after '%c'\n", c);
		db_error(0);
		/*NOTREACHED*/
	    }
	    if (t == tPLUS)
		lhs += rhs;
	    else if (t == tMINUS)
		lhs -= rhs;
	    else
		lhs |= rhs;
	    t = db_read_token();
	}
	db_unread_token(t);
	*valuep = lhs;
	return (TRUE);
}

boolean_t
db_shift_expr(db_expr_t *valuep)
{
	db_expr_t	lhs, rhs;
	int		t;

	if (!db_add_expr(&lhs))
	    return (FALSE);

	t = db_read_token();
	while (t == tSHIFT_L || t == tSHIFT_R) {
	    if (!db_add_expr(&rhs)) {
		db_printf("Expression syntax error after \"%s\"\n",
			(t == tSHIFT_L)? "<<": ">>");
		db_error(0);
		/*NOTREACHED*/
	    }
	    if (rhs < 0) {
		db_error("Negative shift amount\n");
		/*NOTREACHED*/
	    }
	    if (t == tSHIFT_L)
		lhs <<= rhs;
	    else {
		/* Shift right is unsigned */
		lhs = (natural_t) lhs >> rhs;
	    }
	    t = db_read_token();
	}
	db_unread_token(t);
	*valuep = lhs;
	return (TRUE);
}

boolean_t
db_logical_relation_expr(db_expr_t *valuep)
{
	db_expr_t	lhs, rhs;
	int		t;
	char		op[3];

	if (!db_shift_expr(&lhs))
	    return(FALSE);

	t = db_read_token();
	while (t == tLOG_EQ || t == tLOG_NOT_EQ
		|| t == tGREATER || t == tGREATER_EQ
		|| t == tLESS || t == tLESS_EQ) {
	    op[0] = db_tok_string[0];
	    op[1] = db_tok_string[1];
	    op[2] = 0;
	    if (!db_shift_expr(&rhs)) {
		db_printf("Expression syntax error after \"%s\"\n", op);
		db_error(0);
		/*NOTREACHED*/
	    }
	    switch(t) {
	    case tLOG_EQ:
		lhs = (lhs == rhs);
		break;
	    case tLOG_NOT_EQ:
		lhs = (lhs != rhs);
		break;
	    case tGREATER:
		lhs = (lhs > rhs);
		break;
	    case tGREATER_EQ:
		lhs = (lhs >= rhs);
		break;
	    case tLESS:
		lhs = (lhs < rhs);
		break;
	    case tLESS_EQ:
		lhs = (lhs <= rhs);
		break;
	    }
	    t = db_read_token();
	}
	db_unread_token(t);
	*valuep = lhs;
	return (TRUE);
}

boolean_t
db_logical_and_expr(db_expr_t *valuep)
{
	db_expr_t	lhs, rhs;
	int		t;

	if (!db_logical_relation_expr(&lhs))
	    return(FALSE);

	t = db_read_token();
	while (t == tLOG_AND) {
	    if (!db_logical_relation_expr(&rhs)) {
		db_error("Expression syntax error after \"&&\"\n");
		/*NOTREACHED*/
	    }
	    lhs = (lhs && rhs);
	    t = db_read_token();
	}
	db_unread_token(t);
	*valuep = lhs;
	return (TRUE);
}

boolean_t
db_logical_or_expr(db_expr_t *valuep)
{
	db_expr_t	lhs, rhs;
	int		t;

	if (!db_logical_and_expr(&lhs))
	    return(FALSE);

	t = db_read_token();
	while (t == tLOG_OR) {
	    if (!db_logical_and_expr(&rhs)) {
		db_error("Expression syntax error after \"||\"\n");
		/*NOTREACHED*/
	    }
	    lhs = (lhs || rhs);
	    t = db_read_token();
	}
	db_unread_token(t);
	*valuep = lhs;
	return (TRUE);
}

int
db_expression(db_expr_t *valuep)
{
	return (db_logical_or_expr(valuep));
}
