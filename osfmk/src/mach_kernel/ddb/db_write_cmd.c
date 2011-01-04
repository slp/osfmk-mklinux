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
 * Revision 2.6  91/10/09  16:05:06  af
 * 	 Revision 2.5.3.1  91/10/05  13:09:25  jeffreyh
 * 		Added user space write support including inactive task.
 * 	 	[91/08/29            tak]
 * 
 * Revision 2.5.3.1  91/10/05  13:09:25  jeffreyh
 * 	Added user space write support including inactive task.
 * 	[91/08/29            tak]
 * 
 * Revision 2.5  91/05/14  15:38:04  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:07:35  mrt
 * 	Changed to new Mach copyright
 * 	[91/01/31  16:20:19  mrt]
 * 
 * Revision 2.3  90/10/25  14:44:26  rwd
 * 	Changed db_write_cmd to print unsigned.
 * 	[90/10/19            rpd]
 * 
 * Revision 2.2  90/08/27  21:53:54  dbg
 * 	Set db_prev and db_next instead of explicitly advancing dot.
 * 	[90/08/22            dbg]
 * 	Reflected changes in db_printsym()'s calling seq.
 * 	[90/08/20            af]
 * 	Warn user if nothing was written.
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
 *	Author: David B. Golub,  Carnegie Mellon University
 *	Date:	7/90
 */

#include <mach/boolean.h>
#include <kern/task.h>
#include <kern/thread.h>

#include <machine/db_machdep.h>

#include <ddb/db_lex.h>
#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_task_thread.h>
#include <ddb/db_expr.h>
#include <ddb/db_write_cmd.h>
#include <ddb/db_output.h>			/* For db_printf() */

/*
 * Write to file.
 */
void
db_write_cmd(
	db_expr_t	address,
	boolean_t	have_addr,
	db_expr_t	count,
	char *		modif)
{
	register db_addr_t	addr;
	register db_expr_t	old_value;
	db_expr_t	new_value;
	register int	size;
	boolean_t	wrote_one = FALSE;
	boolean_t	t_opt, u_opt;
	thread_act_t	thr_act;
	task_t		task;

	addr = (db_addr_t) address;

	size = db_size_option(modif, &u_opt, &t_opt);

	if (t_opt) 
	  {
	    if (!db_get_next_act(&thr_act, 0))
	      return;
	    task = thr_act->task;
	  }
	else
	  task = db_current_space();

	/* if user space is not explicitly specified, 
	   look in the kernel */
	if (!u_opt)
	  task = TASK_NULL;

	if (!DB_VALID_ADDRESS(addr, u_opt)) {
	  db_printf("Bad address 0x%x\n", addr);
	  return;
	}

	while (db_expression(&new_value)) {
	    old_value = db_get_task_value(addr, size, FALSE, task);
	    db_task_printsym(addr, DB_STGY_ANY, task);
	    db_printf("\t\t%#8n\t=\t%#8n\n", old_value, new_value);
	    db_put_task_value(addr, size, new_value, task);
	    addr += size;

	    wrote_one = TRUE;
	}

	if (!wrote_one)
	    db_error("Nothing written.\n");

	db_next = addr;
	db_prev = addr - size;
}
