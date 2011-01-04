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
 * Revision 2.6  91/10/09  15:58:45  af
 * 	 Revision 2.5.2.1  91/10/05  13:05:30  jeffreyh
 * 	 	Added db_exec_conditional_cmd(), and db_option().
 * 	 	Deleted db_skip_to_eol().
 * 	 	[91/08/29            tak]
 * 
 * Revision 2.5.2.1  91/10/05  13:05:30  jeffreyh
 * 	Added db_exec_conditional_cmd(), and db_option().
 * 	Deleted db_skip_to_eol().
 * 	[91/08/29            tak]
 * 
 * Revision 2.5  91/07/09  23:15:46  danner
 * 	Grabbed up to date copyright.
 * 	[91/07/08            danner]
 * 
 * Revision 2.2  91/04/10  16:02:32  mbj
 * 	Grabbed 3.0 copyright/disclaimer since ddb comes from 3.0.
 * 	[91/04/09            rvb]
 * 
 * Revision 2.3  91/02/05  17:06:15  mrt
 * 	Changed to new Mach copyright
 * 	[91/01/31  16:17:28  mrt]
 * 
 * Revision 2.2  90/08/27  21:50:19  dbg
 * 	Replace db_last_address_examined with db_prev, db_next.
 * 	[90/08/22            dbg]
 * 	Created.
 * 	[90/08/07            dbg]
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
 * Command loop declarations.
 */

#ifndef	_DDB_DB_COMMAND_H_
#define	_DDB_DB_COMMAND_H_

#include <machine/db_machdep.h>
#include <db_machine_commands.h>

typedef void	(*db_func)(db_expr_t, int, db_expr_t, char *);

/*
 * Command table
 */
struct db_command {
	char *	name;		/* command name */
	db_func	fcn;		/* function to call */
	int	flag;		/* extra info: */
#define	CS_OWN		0x1	    /* non-standard syntax */
#define	CS_MORE		0x2	    /* standard syntax, but may have other
				       words at end */
#define	CS_SET_DOT	0x100	    /* set dot after command */
	struct db_command *more;   /* another level of command */
};


extern db_addr_t	db_dot;		/* current location */
extern db_addr_t	db_last_addr;	/* last explicit address typed */
extern db_addr_t	db_prev;	/* last address examined
					   or written */
extern db_addr_t	db_next;	/* next address to be examined
					   or written */


/* Prototypes for functions exported by this module.
 */

void db_command_loop(void);

#if DB_MACHINE_COMMANDS
void db_machine_commands_install(struct db_command *ptr);
#endif /* DB_MACHINE_COMMANDS */

boolean_t db_exec_cmd_nest(
	char	*cmd,
	int	size);

void db_error(char *s);

boolean_t db_option(
	char	*modif,
	int	option);

#endif	/* !_DDB_DB_COMMAND_H_ */
