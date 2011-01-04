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
 * Revision 2.5  91/10/09  16:04:17  af
 * 	 Revision 2.4.3.1  91/10/05  13:08:42  jeffreyh
 * 	 	Added suffix related field to db_variable structure.
 * 	 	Added macro definitions of db_{read,write}_variable.
 * 	 	[91/08/29            tak]
 * 
 * Revision 2.4.3.1  91/10/05  13:08:42  jeffreyh
 * 	Added suffix related field to db_variable structure.
 * 	Added macro definitions of db_{read,write}_variable.
 * 	[91/08/29            tak]
 * 
 * Revision 2.4  91/05/14  15:37:12  mrt
 * 	Correcting copyright
 * 
 * Revision 2.3  91/02/05  17:07:23  mrt
 * 	Changed to new Mach copyright
 * 	[91/01/31  16:19:54  mrt]
 * 
 * Revision 2.2  90/08/27  21:53:40  dbg
 * 	Modularized typedef name.  Documented the calling sequence of
 * 	the (optional) access function of a variable.  Now the valuep
 * 	field can be made opaque, eg be an offset that fcn() resolves.
 * 	[90/08/20            af]
 * 
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
 * 	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

#ifndef	_DDB_DB_VARIABLES_H_
#define	_DDB_DB_VARIABLES_H_

#include <kern/thread.h>
#include <machine/db_machdep.h>		/* For db_expr_t */


#define DB_VAR_LEVEL	3	/* maximum number of suffix level */

/*
 * auxiliary parameters passed to a variable handler
 */
struct db_var_aux_param {
	char		*modif;			/* option strings */
	short		level;			/* number of levels */
	short		hidden_level;		/* hidden level */
	short		suffix[DB_VAR_LEVEL];	/* suffix */
	thread_act_t	thr_act;		/* target thr_act */
};

typedef struct db_var_aux_param	*db_var_aux_param_t;
	

/*
 * Debugger variables.
 */
struct db_variable {
	char	*name;		/* Name of variable */
	db_expr_t *valuep;	/* pointer to value of variable */
				/* function to call when reading/writing */
	int	(*fcn)(struct db_variable *,db_expr_t *,int,db_var_aux_param_t);
	short	min_level;	/* number of minimum suffix levels */
	short	max_level;	/* number of maximum suffix levels */
	short	low;		/* low value of level 1 suffix */
	short	high;		/* high value of level 1 suffix */
	boolean_t hidden_level;	/* is there a hidden suffix level ? */
	short	hidden_low;	/* low value of hidden level */
	short	hidden_high;	/* high value of hidden level */
	int	*hidden_levelp;	/* value of current hidden level */
	boolean_t precious;	/* print old value when affecting ? */
#define DB_VAR_GET	0
#define DB_VAR_SET	1
#define DB_VAR_SHOW	2
};

typedef struct db_variable	*db_variable_t;

#define	DB_VAR_NULL	(db_variable_t)0

#define	FCN_NULL	((int (*)(struct db_variable *,			\
				  db_expr_t *,				\
				  int,					\
				  db_var_aux_param_t)) 0)

#define DB_VAR_LEVEL	3	/* maximum number of suffix level */
#define DB_MACRO_LEVEL	5	/* max macro nesting */
#define DB_MACRO_NARGS	10	/* max args per macro */

#define db_read_variable(vp, valuep)	\
	db_read_write_variable(vp, valuep, DB_VAR_GET, 0)
#define db_write_variable(vp, valuep)	\
	db_read_write_variable(vp, valuep, DB_VAR_SET, 0)


extern struct db_variable	db_vars[];	/* debugger variables */
extern struct db_variable	*db_evars;
extern struct db_variable	db_regs[];	/* machine registers */
extern struct db_variable	*db_eregs;

#if defined(ALTERNATE_REGISTER_DEFS)

extern struct db_variable	db_altregs[];	/* alternate machine regs */
extern struct db_variable	*db_ealtregs;

#endif /* defined(ALTERNATE_REGISTER_DEFS) */

/* Prototypes for functions exported by this module.
 */

int db_get_variable(db_expr_t *valuep);

void db_read_write_variable(
	struct db_variable	*vp,
	db_expr_t		*valuep,
	int 			rw_flag,
	db_var_aux_param_t	ap);

void db_set_cmd(void);

void db_show_one_variable(void);

void db_show_variable(void);

db_variable_t db_find_reg_name(char	*s);

#endif	/* !_DDB_DB_VARIABLES_H_ */
