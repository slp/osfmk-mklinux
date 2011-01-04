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
 * Revision 2.16.2.7  92/09/15  17:14:21  jeffreyh
 * 	Add call to netipc_packet_state printing code.  With Jeffreyh.
 * 	[92/08/20            alanl]
 * 
 * Revision 2.16.2.6  92/04/30  11:48:57  bernadat
 * 	Adaptations for Corollary and Systempro
 * 	[92/04/08            bernadat]
 * 
 * Revision 2.16.2.4  92/03/28  10:04:15  jeffreyh
 * 	Added  traphist and vaddrs for the i860
 * 	[92/03/20            andyp]
 * 
 * Revision 2.16.2.3  92/02/21  11:22:58  jsb
 * 	Added show pset, show all vuids.
 * 	[92/02/20  10:52:22  jsb]
 * 
 * Revision 2.16.2.2  92/02/18  18:38:34  jeffreyh
 * 	Print cpu number with the prompt
 * 	added a "cpu" command to switch between cpus on the Corollary
 * 	[91/06/25            bernadat]
 * 	Added new show callhere option to debugger for iPSC only.
 * 	[92/02/13  12:31:30  jeffreyh]
 * 
 * Revision 2.16.2.1  92/01/21  21:49:40  jsb
 * 	NORMA_VM: added show xmm_obj, show xmm_reply.
 * 	[92/01/21  18:13:32  jsb]
 * 
 * 	NORMA_IPC: added show all uids, proxies, principals.
 * 	[92/01/16  21:25:38  jsb]
 * 
 * 	Added show copy, show packet, show pcs.
 * 	[92/01/13  10:13:50  jsb]
 * 
 * Revision 2.16  91/10/09  15:58:27  af
 * 	Revision 2.15.1.1  91/10/05  13:05:19  jeffreyh
 * 	Added compound, macro, and conditional command support.
 * 	Added "xf" and "xb" command to examine forward and backward.
 * 	Added last command execution with "!!".
 * 	Added "show task" command.
 * 	Added "ipc_port" command to print all IPC ports in a task.
 * 	Added dot address and default thread indicator in prompt.
 * 	Changed error messages to output more information.
 * 	Moved "skip_to_eol()" to db_lex.c.
 * 	[91/08/29            tak]
 * 
 * Revision 2.15  91/08/24  11:55:24  af
 * 	Added optional funcall at ddb entry: similar to GDB's
 * 	"display"s in spirit.
 * 	[91/08/02  02:42:11  af]
 * 
 * Revision 2.14  91/08/03  18:17:16  jsb
 * 	Added `show kmsg' and `show msg'.
 * 	Replaced NORMA_BOOT conditionals with NORMA_IPC.
 * 	Use _node_self instead of node_self() for prompt,
 * 	since node_self() will panic if _node_self not intialized.
 * 	[91/07/24  22:48:48  jsb]
 * 
 * Revision 2.13  91/07/11  11:00:32  danner
 * 	Copyright Fixes
 * 
 * Revision 2.12  91/07/09  23:15:42  danner
 * 	Modified the command loop to include the cpu number in the
 * 	 command prompt on multicpu machines, and node number in norma
 * 	      systems.
 * 	[91/04/12            danner]
 *
 * Revision 2.11  91/07/01  08:24:04  jsb
 * 	Added support for 'show all slocks'.
 * 	[91/06/29  15:59:01  jsb]
 * 
 * Revision 2.10  91/06/17  15:43:46  jsb
 * 	Renamed NORMA conditionals.
 * 	[91/06/17  09:57:51  jsb]
 * 
 * 	Moved struct command definition to db_command.h, added include of
 * 	 db_command.h, renamed struct command to struct db_command. All
 * 	 in support of machine dependpent command extensions to db. 
 * 	[91/03/12            danner]
 * 
 * Revision 2.9  91/06/06  17:03:47  jsb
 * 	NORMA support: prompt includes node number, e.g., db4>.
 * 	[91/05/25  10:47:35  jsb]
 * 
 * Revision 2.8  91/05/14  15:32:51  mrt
 * 	Correcting copyright
 * 
 * Revision 2.7  91/03/16  14:42:30  rpd
 * 	Added db_recover.
 * 	[91/01/13            rpd]
 * 
 * Revision 2.6  91/02/05  17:06:10  mrt
 * 	Changed to new Mach copyright
 * 	[91/01/31  16:17:18  mrt]
 * 
 * Revision 2.5  91/01/08  17:31:54  rpd
 * 	Forward reference for db_fncall();
 * 	[91/01/04  12:35:17  rvb]
 * 
 * 	Add call as a synonym for ! and match for next
 * 	[91/01/04  12:14:48  rvb]
 * 
 * Revision 2.4  90/11/07  16:49:15  rpd
 * 	Added search.
 * 	[90/11/06            rpd]
 * 
 * Revision 2.3  90/10/25  14:43:45  rwd
 * 	Changed db_fncall to print the result unsigned.
 * 	[90/10/19            rpd]
 * 
 * 	Added CS_MORE to db_watchpoint_cmd.
 * 	[90/10/17            rpd]
 * 	Added watchpoint commands: watch, dwatch, show watches.
 * 	[90/10/16            rpd]
 * 
 * Revision 2.2  90/08/27  21:50:10  dbg
 * 	Remove 'listbreaks' - use 'show breaks' instead.  Change 'show
 * 	threads' to 'show all threads' to avoid clash with 'show thread'.
 * 	Set 'dot' here from db_prev or db_next, depending on 'db_ed_style'
 * 	flag and syntax table.
 * 	[90/08/22            dbg]
 * 	Reduce lint.
 * 	[90/08/07            dbg]
 * 	Created.
 * 	[90/07/25            dbg]
 * 
 */
/* CMU_ENDHIST */
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
 */
/*
 *	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

/*
 * Command dispatcher.
 */
#include <cpus.h>
#include <dipc.h>
#include <dipc_timer.h>
#include <xkmachkernel.h>
#include <norma_vm.h>
#ifdef	AT386
#include <norma_scsi.h>
#endif	/* AT386 */

#include <mach/boolean.h>
#include <string.h>
#include <machine/db_machdep.h>

#if defined(__alpha)
#  include <kdebug.h>
#  if KDEBUG
#    include <machine/kdebug.h>
#  endif
#endif /* defined(__alpha) */

#include <ddb/db_lex.h>
#include <ddb/db_output.h>
#include <ddb/db_break.h>
#include <ddb/db_command.h>
#include <ddb/db_cond.h>
#include <ddb/db_examine.h>
#include <ddb/db_expr.h>
#include <ddb/db_macro.h>
#include <ddb/db_print.h>
#include <ddb/db_run.h>
#include <ddb/db_task_thread.h>
#include <ddb/db_variables.h>
#include <ddb/db_watch.h>
#include <ddb/db_write_cmd.h>
#include <ddb/tr.h>

#include <machine/setjmp.h>
#include <kern/thread.h>

#include <kern/misc_protos.h>
#include <vm/vm_print.h>
#include <ipc/ipc_print.h>
#include <kern/kern_print.h>
#include <machine/db_machdep.h>		/* For db_stack_trace_cmd(). */
#include <kern/zalloc.h>	/* For db_show_one_zone, db_show_all_zones. */
#include <kern/lock.h>			/* For db_show_all_slocks(). */
#include <device/ds_routines.h>		/* For db_show_ior(). */

#if	DIPC_TIMER
#include <dipc/dipc_timer.h>
#endif	/* DIPC_TIMER */

#if	NORMA_VM
#include <xmm/xmm_obj.h>
#endif	/* NORMA_VM */

/*
 * Exported global variables
 */
boolean_t	db_cmd_loop_done;
jmp_buf_t	*db_recover = 0;
db_addr_t	db_dot;
db_addr_t	db_last_addr;
db_addr_t	db_prev;
db_addr_t	db_next;

/*
 * if 'ed' style: 'dot' is set at start of last item printed,
 * and '+' points to next line.
 * Otherwise: 'dot' points to next item, '..' points to last.
 */
boolean_t	db_ed_style = TRUE;

/*
 * Results of command search.
 */
#define	CMD_UNIQUE	0
#define	CMD_FOUND	1
#define	CMD_NONE	2
#define	CMD_AMBIGUOUS	3
#define	CMD_HELP	4

/* Prototypes for functions local to this file.  XXX -- should be static!
 */

void db_command(
	struct db_command	**last_cmdp,	/* IN_OUT */
	db_expr_t		*last_countp,	/* IN_OUT */
	char			*last_modifp,	/* IN_OUT */
	struct db_command	*cmd_table);

void db_help_cmd(void);

void db_output_prompt(void);

void db_fncall(void);

void db_cmd_list(struct db_command *table);

int db_cmd_search(
	char *			name,
	struct db_command *	table,
	struct db_command **	cmdp);	/* out */

void db_command_list(
	struct db_command	**last_cmdp,	/* IN_OUT */
	db_expr_t		*last_countp,	/* IN_OUT */
	char			*last_modifp,	/* IN_OUT */
	struct db_command	*cmd_table);



/*
 * Search for command prefix.
 */
int
db_cmd_search(
	char *			name,
	struct db_command *	table,
	struct db_command **	cmdp)	/* out */
{
	struct db_command	*cmd;
	int		result = CMD_NONE;

	for (cmd = table; cmd->name != 0; cmd++) {
	    register char *lp;
	    register char *rp;
	    register int  c;

	    lp = name;
	    rp = cmd->name;
	    while ((c = *lp) == *rp) {
		if (c == 0) {
		    /* complete match */
		    *cmdp = cmd;
		    return (CMD_UNIQUE);
		}
		lp++;
		rp++;
	    }
	    if (c == 0) {
		/* end of name, not end of command -
		   partial match */
		if (result == CMD_FOUND) {
		    result = CMD_AMBIGUOUS;
		    /* but keep looking for a full match -
		       this lets us match single letters */
		}
		else {
		    *cmdp = cmd;
		    result = CMD_FOUND;
		}
	    }
	}
	if (result == CMD_NONE) {
	    /* check for 'help' */
	    if (!strncmp(name, "help", strlen(name)))
		result = CMD_HELP;
	}
	return (result);
}

void
db_cmd_list(struct db_command *table)
{
	register struct db_command *new;
	register struct db_command *old;
	register struct db_command *cur;
	unsigned int l;
	unsigned int len;

	len = 1;
	for (cur = table; cur->name != 0; cur++)
	    if ((l = strlen(cur->name)) >= len)
		len = l + 1;

	old = (struct db_command *)0;
	for (;;) {
	    new = (struct db_command *)0;
	    for (cur = table; cur->name != 0; cur++)
		if ((new == (struct db_command *)0 ||
		     strcmp(cur->name, new->name) < 0) &&
		    (old == (struct db_command *)0 ||
		     strcmp(cur->name, old->name) > 0))
		    new = cur;
	    if (new == (struct db_command *)0)
		    return;
	    db_reserve_output_position(len);
	    db_printf("%-*s", len, new->name);
	    old = new;
	}
}

void
db_command(
	struct db_command	**last_cmdp,	/* IN_OUT */
	db_expr_t		*last_countp,	/* IN_OUT */
	char			*last_modifp,	/* IN_OUT */
	struct db_command	*cmd_table)
{
	struct db_command	*cmd;
	int		t;
	char		modif[TOK_STRING_SIZE];
	char		*modifp = &modif[0];
	db_expr_t	addr, count;
	boolean_t	have_addr;
	int		result;

	t = db_read_token();
	if (t == tEOL || t == tSEMI_COLON) {
	    /* empty line repeats last command, at 'next' */
	    cmd = *last_cmdp;
	    count = *last_countp;
	    modifp = last_modifp;
	    addr = (db_expr_t)db_next;
	    have_addr = FALSE;
	    if (t == tSEMI_COLON)
		db_unread_token(t);
	}
	else if (t == tEXCL) {
	    db_fncall();
	    return;
	}
	else if (t != tIDENT) {
	    db_printf("?\n");
	    db_flush_lex();
	    return;
	}
	else {
	    /*
	     * Search for command
	     */
	    while (cmd_table) {
		result = db_cmd_search(db_tok_string,
				       cmd_table,
				       &cmd);
		switch (result) {
		    case CMD_NONE:
			if (db_exec_macro(db_tok_string) == 0)
			    return;
			db_printf("No such command \"%s\"\n", db_tok_string);
			db_flush_lex();
			return;
		    case CMD_AMBIGUOUS:
			db_printf("Ambiguous\n");
			db_flush_lex();
			return;
		    case CMD_HELP:
			db_cmd_list(cmd_table);
			db_flush_lex();
			return;
		    default:
			break;
		}
		if ((cmd_table = cmd->more) != 0) {
		    t = db_read_token();
		    if (t != tIDENT) {
			db_cmd_list(cmd_table);
			db_flush_lex();
			return;
		    }
		}
	    }

	    if ((cmd->flag & CS_OWN) == 0) {
		/*
		 * Standard syntax:
		 * command [/modifier] [addr] [,count]
		 */
		t = db_read_token();
		if (t == tSLASH) {
		    t = db_read_token();
		    if (t != tIDENT) {
			db_printf("Bad modifier \"/%s\"\n", db_tok_string);
			db_flush_lex();
			return;
		    }
		    strcpy(modif, db_tok_string);
		}
		else {
		    db_unread_token(t);
		    modif[0] = '\0';
		}

		if (db_expression(&addr)) {
		    db_dot = (db_addr_t) addr;
		    db_last_addr = db_dot;
		    have_addr = TRUE;
		}
		else {
		    addr = (db_expr_t) db_dot;
		    have_addr = FALSE;
		}
		t = db_read_token();
		if (t == tCOMMA) {
		    if (!db_expression(&count)) {
			db_printf("Count missing after ','\n");
			db_flush_lex();
			return;
		    }
		}
		else {
		    db_unread_token(t);
		    count = -1;
		}
	    }
	}
	if (cmd != 0) {
	    /*
	     * Execute the command.
	     */
	    (*cmd->fcn)(addr, have_addr, count, modifp);

	    if (cmd->flag & CS_SET_DOT) {
		/*
		 * If command changes dot, set dot to
		 * previous address displayed (if 'ed' style).
		 */
		if (db_ed_style) {
		    db_dot = db_prev;
		}
		else {
		    db_dot = db_next;
		}
	    }
	    else {
		/*
		 * If command does not change dot,
		 * set 'next' location to be the same.
		 */
		db_next = db_dot;
	    }
	}
	*last_cmdp = cmd;
	*last_countp = count;
	strcpy(last_modifp, modifp);
}

void
db_command_list(
	struct db_command	**last_cmdp,	/* IN_OUT */
	db_expr_t		*last_countp,	/* IN_OUT */
	char			*last_modifp,	/* IN_OUT */
	struct db_command	*cmd_table)
{
	do {
	    db_command(last_cmdp, last_countp, last_modifp, cmd_table);
	    db_skip_to_eol();
	} while (db_read_token() == tSEMI_COLON && db_cmd_loop_done == 0);
}


extern void	db_system_stats(void);

#if     XKMACHKERNEL
extern void     db_show_net_memory(void);

struct db_command db_show_net_cmds[] = {
        { "memory",     (db_func)db_show_net_memory,            0,      0 },
};
#endif /* XKMACHKERNEL */

#if	DIPC
extern void	db_dipc_stats(void), db_dipc_memory(void),
		db_dipc_port_deads(void), db_dipc_port_table(void),
		db_dipc_msg_progress(void), db_dipc_special_ports(void),
		db_dipc_port_table_proxies(void),
		db_dipc_port_table_principals(void), db_dipc_port_get(void);

extern void	db_show_kkt_handle(void), db_show_kkt_request(void),
		db_show_kkt_channel(void), db_show_kkt(void),
		db_show_kkt_node_map(void);
#if	PARAGON860
extern void	db_show_kkt_transmit(void), db_show_kkt_transport(void);
#endif	/* PARAGON860 */
#if	NORMA_SCSI
extern void	db_show_kkt_node(void), db_show_kkt_msg(void),
		db_show_kkt_event(void);
#endif	/* NORMA_SCSI */

struct db_command db_dipc_port_cmds[] = {
	{ "dead",	(db_func)db_dipc_port_deads,		0,	0 },
	{ "principals",	(db_func)db_dipc_port_table_principals,	0,	0 },
	{ "proxies",	(db_func)db_dipc_port_table_proxies,	0,	0 },
	{ "special",	(db_func)db_dipc_special_ports,		0,	0 },
	{ "table",	(db_func)db_dipc_port_table,		0,	0 },
	{ "uid",	(db_func)db_dipc_port_get,		0,	0 },
	{ (char *)0 }
};

struct db_command db_dipc_cmds[] = {
	{ "port",	0,			0,	db_dipc_port_cmds },
	{ "memory",	(db_func)db_dipc_memory,		0,	0 },
	{ "message_progress",(db_func)db_dipc_msg_progress,	0,	0 },
	{ "mp",		(db_func)db_dipc_msg_progress,		0,	0 },
	{ "stats",	(db_func)db_dipc_stats,			0,	0 },
	{ (char *)0 }
};

struct db_command db_show_kkt_cmds[] = {
	{ "handle",	(db_func)db_show_kkt_handle,		0,	0 },
	{ "request",	(db_func)db_show_kkt_request,		0,	0 },
	{ "channel",	(db_func)db_show_kkt_channel,		0,	0 },
	{ "stats",	(db_func)db_show_kkt,			0,	0 },
	{ "map",	(db_func)db_show_kkt_node_map,		0,	0 },
#if	PARAGON860
	{ "transmit",	(db_func)db_show_kkt_transmit,		0,	0 },
	{ "transport",	(db_func)db_show_kkt_transport,		0,	0 },
#endif	/* PARAGON860 */
#if	NORMA_SCSI
        { "node",	(db_func)db_show_kkt_node,		0,	0 },
        { "msg",	(db_func)db_show_kkt_msg,		0,	0 },
        { "event",	(db_func)db_show_kkt_event,		0,	0 },
#endif	/* NORMA_SCSI */
	{ (char *)0 }
};
#endif	/*DIPC*/

struct db_command db_show_all_cmds[] = {
#if	USLOCK_DEBUG
	{ "slocks",	(db_func) db_show_all_slocks,		0,	0 },
#endif	/* USLOCK_DEBUG */
#if	DIPC_TIMER
	{ "timers",	(db_func)db_timer_print,		0,	0 },
#endif	/* DIPC_TIMER */
	{ "acts",	db_show_all_acts,			0,	0 },
	{ "spaces",	db_show_all_spaces, 			0,	0 },
	{ "tasks",	db_show_all_acts,			0,	0 },
	/* temporary alias for sanity preservation */
	{ "threads",	db_show_all_acts,			0,	0 },
	{ "zones",	db_show_all_zones,			0,	0 },
	{ "vmtask",	db_show_all_task_vm,			0,	0 },
	{ (char *)0 }
};

/* XXX */

extern void             db_show_thread_log(void);
extern void		db_show_one_lock(lock_t*);

struct db_command db_show_cmds[] = {
	{ "all",	0,			0,	db_show_all_cmds },
#if	DIPC
	{ "dipc",	0,			0,	db_dipc_cmds },
	{ "kkt",	0,			0,	db_show_kkt_cmds },
#endif	/* DIPC */
#if     XKMACHKERNEL
	{ "net",        0,                      0,      db_show_net_cmds },
#endif /* XKMACHKERNEL */
	{ "registers",	db_show_regs,		0,	0 },
	{ "variables",	(db_func) db_show_variable,	CS_OWN,	0 },
	{ "breaks",	(db_func) db_listbreak_cmd, 	0,	0 },
	{ "watches",	(db_func) db_listwatch_cmd, 	0,	0 },
	{ "task",	db_show_one_task,	0,	0 },
	{ "act",	db_show_one_act,	0,	0 },
	{ "shuttle",	db_show_shuttle,	0,	0 },
	{ "thread",	db_show_one_thread,	0,	0 },
	{ "vmtask",	db_show_one_task_vm,	0,	0 },
	{ "macro",	(db_func) db_show_macro,	CS_OWN, 0 },
	{ "runq",	(db_func) db_show_runq,	0,	0 },
	{ "map",	(db_func) vm_map_print,		0,	0 },
	{ "object",	(db_func) vm_object_print,	0,	0 },
	{ "page",	(db_func) vm_page_print,	0,	0 },
	{ "copy",	(db_func) vm_map_copy_print,	0,	0 },
	{ "port",	(db_func) ipc_port_print,	0,	0 },
	{ "pset",	(db_func) ipc_pset_print,	0,	0 },
	{ "kmsg",	(db_func) ipc_kmsg_print,	0,	0 },
	{ "msg",	(db_func) ipc_msg_print,	0,	0 },
        { "ior",	(db_func)db_show_ior,		0,	0 },
	{ "ipc_port",	db_show_port_id,	0,	0 },
	{ "lock",	(db_func)db_show_one_lock,	0,	0 },
#if	NORMA_VM
	{ "xmm_obj",	(db_func) xmm_obj_print,	0,	0 },
	{ "xmm_reply",	(db_func) xmm_reply_print,	0,	0 },
#endif	/* NORMA_VM */
#if	TRACE_BUFFER
	{ "tr",		db_show_tr,		0,	0 },
#endif	/* TRACE_BUFFER */
	{ "space",	db_show_one_space,	0,	0 },
	{ "system",	(db_func) db_system_stats,	0,	0 },
#if	DIPC_TIMER
	{ "timer",	(db_func)db_show_one_timer,	0,	0 },
#endif	/* DIPC_TIMER */
	{ "zone",	db_show_one_zone,	0,	0 },
        { "simple_lock", db_show_one_simple_lock,	0,      0 },
#if !NORMA_IPC
	{ "mutex",      db_show_one_mutex,      0,      0 },
#endif /* !NORMA_IPC */
        { "tlog",	(db_func)db_show_thread_log,    0,      0 },
	{ "subsystem",	db_show_subsystem,	0,	0 },
	{ (char *)0, }
};

#if	NCPUS > 1
#define	db_switch_cpu kdb_on
extern void	db_switch_cpu(int);
#endif	/* NCPUS > 1 */

struct db_command db_command_table[] = {
#if DB_MACHINE_COMMANDS

/* this must be the first entry, if it exists */
	{ "machine",    0,                      0,     		0},
#endif
	{ "print",	(db_func) db_print_cmd,	CS_OWN,		0 },
	{ "examine",	db_examine_cmd,		CS_MORE|CS_SET_DOT, 0 },
	{ "x",		db_examine_cmd,		CS_MORE|CS_SET_DOT, 0 },
	{ "xf",		db_examine_forward,	CS_SET_DOT,	0 },
	{ "xb",		db_examine_backward,	CS_SET_DOT,	0 },
	{ "search",	(db_func) db_search_cmd,CS_OWN|CS_SET_DOT, 0 },
	{ "set",	(db_func) db_set_cmd,	CS_OWN,		0 },
	{ "write",	db_write_cmd,		CS_MORE|CS_SET_DOT, 0 },
	{ "w",		db_write_cmd,		CS_MORE|CS_SET_DOT, 0 },
	{ "delete",	(db_func) db_delete_cmd,CS_OWN,		0 },
	{ "d",		(db_func) db_delete_cmd,CS_OWN,		0 },
	{ "break",	db_breakpoint_cmd,	CS_MORE,	0 },
	{ "dwatch",	db_deletewatch_cmd,	CS_MORE,	0 },
	{ "watch",	db_watchpoint_cmd,	CS_MORE,	0 },
	{ "step",	db_single_step_cmd,	0,		0 },
	{ "s",		db_single_step_cmd,	0,		0 },
	{ "continue",	db_continue_cmd,	0,		0 },
	{ "c",		db_continue_cmd,	0,		0 },
	{ "until",	db_trace_until_call_cmd,0,		0 },

	/* As per request of DNoveck, CR1550, leave this disabled	*/
#if 0	/* until CR1440 is fixed, to avoid toe-stubbing			*/
	{ "next",	db_trace_until_matching_cmd,0,		0 },
	{ "match",	db_trace_until_matching_cmd,0,		0 },
#endif
	{ "trace",	db_stack_trace_cmd,	0,		0 },
	{ "cond",	(db_func) db_cond_cmd,	CS_OWN,	 	0 },
	{ "call",	(db_func) db_fncall,	CS_OWN,		0 },
	{ "macro",	(db_func) db_def_macro_cmd,CS_OWN,	 	0 },
	{ "dmacro",	(db_func) db_del_macro_cmd,CS_OWN,		0 },
	{ "show",	0,			0,	db_show_cmds },
#if	NCPUS > 1
	{ "cpu",	(db_func) db_switch_cpu,0,		0 },
#endif	/* NCPUS > 1 */
	{ "reboot",	(db_func) db_reboot,	0,		0 },
	{ (char *)0, }
};

#if DB_MACHINE_COMMANDS

/* this function should be called to install the machine dependent
   commands. It should be called before the debugger is enabled  */
void db_machine_commands_install(struct db_command *ptr)
{
  db_command_table[0].more = ptr;
  return;
}

#endif /* DB_MACHINE_COMMANDS */


struct db_command	*db_last_command = 0;
db_expr_t		db_last_count = 0;
char			db_last_modifier[TOK_STRING_SIZE] = { '\0' };

void
db_help_cmd(void)
{
	struct db_command *cmd = db_command_table;

	while (cmd->name != 0) {
	    db_printf("%-12s", cmd->name);
	    db_end_line();
	    cmd++;
	}
}

int	(*ddb_display)(void);

void
db_command_loop(void)
{
	jmp_buf_t db_jmpbuf;
	jmp_buf_t *prev = db_recover;
	extern int db_output_line;
	extern int db_macro_level;
	extern int indent;

	/*
	 * Initialize 'prev' and 'next' to dot.
	 */
	db_prev = db_dot;
	db_next = db_dot;

	if (ddb_display)
		(*ddb_display)();

	db_cmd_loop_done = 0;
	while (!db_cmd_loop_done) {
	    (void) _setjmp(db_recover = &db_jmpbuf);
	    db_macro_level = 0;
	    if (db_print_position() != 0)
		db_printf("\n");
	    db_output_line = 0;
	    indent = 0;
	    db_reset_more();
	    db_output_prompt();

	    (void) db_read_line("!!");
	    db_command_list(&db_last_command, &db_last_count,
			    db_last_modifier, db_command_table);
	}

	db_recover = prev;
}

boolean_t
db_exec_cmd_nest(
	char	*cmd,
	int	size)
{
	struct db_lex_context lex_context;

	db_cmd_loop_done = 0;
	if (cmd) {
	    db_save_lex_context(&lex_context);
	    db_switch_input(cmd, size);
	}
	db_command_list(&db_last_command, &db_last_count,
			db_last_modifier, db_command_table);
	if (cmd)
	    db_restore_lex_context(&lex_context);
	return(db_cmd_loop_done == 0);
}

void
db_error(char *s)
{
	extern int db_macro_level;

#if defined(__alpha)
#  if KDEBUG
	extern boolean_t kdebug_mode;
	if (kdebug_mode) {
		if (s) kprintf(DBG_DEBUG, s);
		return;
	}
#  endif /* KDEBUG */
#endif /* defined(__alpha) */

	db_macro_level = 0;
	if (db_recover) {
	    if (s > (char *)1)
		db_printf(s);
	    db_flush_lex();
	    _longjmp(db_recover, (s == (char *)1) ? 2 : 1);
	}
	else
	{
	    if (s > (char *)1)
	        db_printf(s);
	    panic("db_error");
	}
}


/*
 * Call random function:
 * !expr(arg,arg,arg)
 */
void
db_fncall(void)
{
	db_expr_t	fn_addr;
#define	MAXARGS		11
	db_expr_t	args[MAXARGS];
	int		nargs = 0;
	db_expr_t	retval;
	db_expr_t	(*func)(db_expr_t, ...);
	int		t;

	if (!db_expression(&fn_addr)) {
	    db_printf("Bad function \"%s\"\n", db_tok_string);
	    db_flush_lex();
	    return;
	}
	func = (db_expr_t (*) (db_expr_t, ...)) fn_addr;

	t = db_read_token();
	if (t == tLPAREN) {
	    if (db_expression(&args[0])) {
		nargs++;
		while ((t = db_read_token()) == tCOMMA) {
		    if (nargs == MAXARGS) {
			db_printf("Too many arguments\n");
			db_flush_lex();
			return;
		    }
		    if (!db_expression(&args[nargs])) {
			db_printf("Argument missing\n");
			db_flush_lex();
			return;
		    }
		    nargs++;
		}
		db_unread_token(t);
	    }
	    if (db_read_token() != tRPAREN) {
		db_printf("?\n");
		db_flush_lex();
		return;
	    }
	}
	while (nargs < MAXARGS) {
	    args[nargs++] = 0;
	}

	retval = (*func)(args[0], args[1], args[2], args[3], args[4],
			 args[5], args[6], args[7], args[8], args[9] );
	db_printf(" %#n\n", retval);
}

boolean_t
db_option(
	char	*modif,
	int	option)
{
	register char *p;

	for (p = modif; *p; p++)
	    if (*p == option)
		return(TRUE);
	return(FALSE);
}
