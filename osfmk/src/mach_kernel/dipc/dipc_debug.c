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
 * 
 */
/*
 * MkLinux
 */

/*
 *	File:	dipc/dipc_debug.c
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Debugger support for analyzing the state of
 *	the distributed IPC subsystem.
 */

#include <mach_kdb.h>

#if	MACH_KDB

#include <mach/boolean.h>
#include <mach/kkt_request.h>
#include <dipc/dipc_counters.h>
#include <ddb/db_output.h>

#if	DIPC_DO_STATS
extern void	dipc_rpc_stats(void);
#endif	/* DIPC_DO_STATS */

extern void	db_dipc_special_ports(void);
extern void	db_dipc_kserver(void);
extern void	db_dipc_port_table_report(void);
extern void	db_dipc_port_facts(void);
extern void	db_show_kkt(void);
void		db_dipc_message_engine(void);
void		db_dipc_memory(void);
void		db_dipc_stats(void);

/*
 *	Display a detailed analysis of the memory
 *	consumed by DIPC.
 */
void
db_dipc_memory(void)
{
	iprintf("db_dipc_memory:  hook me up!\n");
}



/*
 *	Print interesting facts about the dipc subsystem.
 */
void
db_dipc_stats(void)
{
	extern int	indent;

	iprintf("** DIPC subsystem report **\n");
	indent += 2;

	/*
	 *	Special ports table.  This table
	 *	should be small.
	 */

	db_dipc_special_ports();

	/*
	 *	Information on general port operations,
	 *	like copyin/copyout.
	 */
	db_dipc_port_facts();

	/*
	 *	Message engine report.
	 */
	db_dipc_message_engine();

	/*
	 *	Kserver threads report.
	 */
	db_dipc_kserver();

	/*
	 *	Port name table report.
	 */
	db_dipc_port_table_report();

#if	DIPC_DO_STATS
	/*
	 *	RPC subsystem usage statistics.
	 */
	dipc_rpc_stats();
#endif	/* DIPC_DO_STATS */

	indent -= 2;
}


#if	DIPC_DO_STATS
dstat_decl(extern unsigned int c_dipc_fast_alloc_pages;)
dstat_decl(extern unsigned int c_dipc_mqueue_sends;)
dstat_decl(extern unsigned int c_dipc_mqueue_ool_sends;)

dstat_decl(extern unsigned int c_dmp_queue_length;)
dstat_decl(extern unsigned int c_dmp_queue_enqueued;)
dstat_decl(extern unsigned int c_dmp_queue_handled;)
dstat_decl(extern unsigned int c_dmp_queue_max_length;)

dstat_decl(extern unsigned int c_dmp_free_list_length;)
dstat_decl(extern unsigned int c_dmp_free_list_enqueued;)
dstat_decl(extern unsigned int c_dmp_free_list_handled;)
dstat_decl(extern unsigned int c_dmp_free_list_max_length;)

dstat_decl(extern unsigned int c_dmp_free_ast_list_length;)
dstat_decl(extern unsigned int c_dmp_free_ast_list_enqueued;)
dstat_decl(extern unsigned int c_dmp_free_ast_list_handled;)
dstat_decl(extern unsigned int c_dmp_free_ast_list_max_length;)

dstat_decl(extern unsigned int c_dmr_list_length;)
dstat_decl(extern unsigned int c_dmr_list_enqueued;)
dstat_decl(extern unsigned int c_dmr_list_handled;)
dstat_decl(extern unsigned int c_dmr_list_max_length;)

dstat_decl(extern unsigned int c_rp_list_length;)
dstat_decl(extern unsigned int c_rp_list_enqueued;)
dstat_decl(extern unsigned int c_rp_list_handled;)
dstat_decl(extern unsigned int c_rp_list_max_length;)

dstat_decl(extern unsigned int c_dipc_delivery_queue_length;)
dstat_decl(extern unsigned int c_dipc_delivery_queue_enqueued;)
dstat_decl(extern unsigned int c_dipc_delivery_queue_handled;)
dstat_decl(extern unsigned int c_dipc_delivery_queue_max_length;)

dstat_decl(extern unsigned int c_dkdq_length;)
dstat_decl(extern unsigned int c_dkdq_enqueued;)
dstat_decl(extern unsigned int c_dkdq_handled;)
dstat_decl(extern unsigned int c_dkdq_max_length;)

dstat_decl(extern unsigned int c_dkrq_length;)
dstat_decl(extern unsigned int c_dkrq_enqueued;)
dstat_decl(extern unsigned int c_dkrq_handled;)
dstat_decl(extern unsigned int c_dkrq_max_length;)

dstat_decl(extern unsigned int c_dkaq_length;)
dstat_decl(extern unsigned int c_dkaq_enqueued;)
dstat_decl(extern unsigned int c_dkaq_handled;)
dstat_decl(extern unsigned int c_dkaq_max_length;)

dstat_decl(extern unsigned int c_dipc_rpc_list_length;)
dstat_decl(extern unsigned int c_dipc_rpc_list_enqueued;)
dstat_decl(extern unsigned int c_dipc_rpc_list_handled;)
dstat_decl(extern unsigned int c_dipc_rpc_list_max_length;)

dstat_decl(extern unsigned int c_dml_length;)
dstat_decl(extern unsigned int c_dml_enqueued;)
dstat_decl(extern unsigned int c_dml_handled;)
dstat_decl(extern unsigned int c_dml_max_length;)

dstat_decl(extern unsigned int c_dkp_length;)
dstat_decl(extern unsigned int c_dkp_enqueued;)
dstat_decl(extern unsigned int c_dkp_handled;)
dstat_decl(extern unsigned int c_dkp_max_length;)

dstat_decl(extern unsigned int c_dipc_kmsg_released;)
dstat_decl(extern unsigned int c_dipc_kmsg_ool_released;)
dstat_decl(extern unsigned int c_dipc_kmsg_clean_send;)
dstat_decl(extern unsigned int c_dipc_kmsg_clean_recv_handle;)
dstat_decl(extern unsigned int c_dipc_kmsg_clean_recv_waiting;)
dstat_decl(extern unsigned int c_dipc_kmsg_clean_recv_inline;)

dstat_decl(extern unsigned int c_dim_total;)
dstat_decl(extern unsigned int c_dim_resource_shortage;)
dstat_decl(extern unsigned int c_dim_no_uid;)
dstat_decl(extern unsigned int c_dim_fast_no_inline;)
dstat_decl(extern unsigned int c_dim_fast_inline;)
dstat_decl(extern unsigned int c_dim_fast_data_dropped;)
dstat_decl(extern unsigned int c_dim_slow_delivery;)
dstat_decl(extern unsigned int c_dim_alloc_failed;)
dstat_decl(extern unsigned int c_dim_msg_too_large;)

dstat_decl(extern unsigned int c_dem_block;)
dstat_decl(extern unsigned int c_dem_migrated;)
dstat_decl(extern unsigned int c_dem_dead;)
dstat_decl(extern unsigned int c_dem_queue_full;)
dstat_decl(extern unsigned int c_dem_delivered;)
#endif	/* DIPC_DO_STATS */


#define	pct(a,b)	(((b) == 0) ? 0 : ((100 * (a)) / (b)))

void
db_dipc_message_engine(void)
{
	extern int	indent;
	unsigned int	total_deliveries;

	iprintf("Message Engine:\n");

#if	DIPC_DO_STATS
	indent += 2;
	iprintf("Fast Alloc Pages:  %d\n", c_dipc_fast_alloc_pages);
	iprintf("  Thread\t\tPending Enqueued  Handled   Max Length\n");
	iprintf("Delivery\t\t%7d %8d %8d      %7d\n",
		c_dipc_delivery_queue_length,
		c_dipc_delivery_queue_enqueued,
		c_dipc_delivery_queue_handled,
		c_dipc_delivery_queue_max_length);
	iprintf("Prep\t\t%7d %8d %8d      %7d\n",
		c_dmp_queue_length, c_dmp_queue_enqueued,
		c_dmp_queue_handled, c_dmp_queue_max_length);
	iprintf("Free\t\t%7d %8d %8d      %7d\n",
		c_dmp_free_list_length, c_dmp_free_list_enqueued,
		c_dmp_free_list_handled, c_dmp_free_list_max_length);
	iprintf("Free AST\t\t%7d %8d %8d      %7d\n",
		c_dmp_free_ast_list_length, c_dmp_free_ast_list_enqueued,
		c_dmp_free_ast_list_handled, c_dmp_free_ast_list_max_length);
	iprintf("Msg Req AST\t\t%7d %8d %8d      %7d\n",
		c_dmr_list_length, c_dmr_list_enqueued,
		c_dmr_list_handled, c_dmr_list_max_length);
	iprintf("Cleanup (Destroy)\t%7d %8d %8d      %7d\n",
		c_dkdq_length,c_dkdq_enqueued,c_dkdq_handled,c_dkdq_max_length);
	iprintf("Cleanup (Release)\t%7d %8d %8d      %7d\n",
		c_dkrq_length,c_dkrq_enqueued,c_dkrq_handled,c_dkrq_max_length);
	iprintf("Release AST\t\t%7d %8d %8d      %7d\n",
		c_dkaq_length,c_dkaq_enqueued,c_dkaq_handled,c_dkaq_max_length);
	iprintf("Kserver\t\t%7d %8d %8d      %7d\n",
		c_dkp_length, c_dkp_enqueued, c_dkp_handled, c_dkp_max_length);
	iprintf("RPC\t\t\t%7d %8d %8d      %7d\n",
		c_dipc_rpc_list_length, c_dipc_rpc_list_enqueued,
		c_dipc_rpc_list_handled, c_dipc_rpc_list_max_length);
	iprintf("Restart port\t%7d %8d %8d      %7d\n",
		c_rp_list_length, c_rp_list_enqueued,
		c_rp_list_handled, c_rp_list_max_length);
	iprintf("Migration\t\t%7d %8d %8d      %7d\n",
		c_dml_length, c_dml_enqueued, c_dml_handled, c_dml_max_length);
	iprintf("Outgoing messages:\n");
	indent += 2;
	iprintf("Total Sent\t%8d\tOOL Sent\t%8d\n", c_dipc_mqueue_sends,
		c_dipc_mqueue_ool_sends);
	iprintf("Total Released\t%8d\tOOL Released\t%8d\n",
		c_dipc_kmsg_released, c_dipc_kmsg_ool_released);
	indent -= 2;
	total_deliveries = c_dem_migrated + c_dem_dead +
		c_dem_queue_full + c_dem_delivered;
	iprintf("Incoming message delivery scenarios (%d total):\n",
		c_dim_total);
	indent += 2;
	iprintf("Interrupt level failures:\n");
	indent += 2;
	iprintf("Resource shortages:\t%8d (%3d%%)\n", c_dim_resource_shortage,
		pct(c_dim_resource_shortage, c_dim_total));
	iprintf("No UID:\t\t\t%8d (%3d%%)\n", c_dim_no_uid,
		pct(c_dim_no_uid, c_dim_total));
	indent -= 2;
	iprintf("Fast deliveries:\n");
	indent += 2;
	iprintf("No inline data:\t\t%8d (%3d%%)\n", c_dim_fast_no_inline,
		pct(c_dim_fast_no_inline, c_dim_total));
	iprintf("Inline data:\t\t%8d (%3d%%)\n", c_dim_fast_inline,
		pct(c_dim_fast_inline, c_dim_total));
	iprintf("Data dropped:\t\t%8d (%3d%%)\n", c_dim_fast_data_dropped,
		pct(c_dim_fast_data_dropped, c_dim_total));
	iprintf("[ SLOW:\t\t\t%8d (%3d%%) ]\n", c_dim_slow_delivery,
		pct(c_dim_slow_delivery, c_dim_total));
	iprintf("[ Large msg:\t\t%8d (%3d%%) ]\n", c_dim_msg_too_large,
		pct(c_dim_msg_too_large, c_dim_total));
	iprintf("[ Alloc failed:\t\t%8d (%3d%%) ]\n", c_dim_alloc_failed,
		pct(c_dim_alloc_failed, c_dim_total));
	indent -= 2;
	iprintf("Slow deliveries:\n");
	indent += 2;
	iprintf("Port migrated:\t\t%8d (%3d%%)\n", c_dem_migrated,
		pct(c_dem_migrated, c_dim_total));
	iprintf("Port dead:\t\t%8d (%3d%%)\n", c_dem_dead,
		pct(c_dem_dead, c_dim_total));
	iprintf("Port queue full:\t%8d (%3d%%)\n", c_dem_queue_full,
		pct(c_dem_queue_full, c_dim_total));
	iprintf("Port delivered:\t\t%8d (%3d%%)\n", c_dem_delivered,
		pct(c_dem_delivered, c_dim_total));
	iprintf("[ Port blocked:\t\t%8d (%3d%%) ]\n", c_dem_block,
		pct(c_dem_block, c_dim_total));
	indent -= 2;
	indent -= 2;
	iprintf("Messages destroyed:\n");
	indent += 2;
	iprintf("Send-side:  %8d\n", c_dipc_kmsg_clean_send);
	iprintf("Recv-side:  handle %8d waiting %8d inline %8d\n",
		c_dipc_kmsg_clean_recv_handle,
		c_dipc_kmsg_clean_recv_waiting,
		c_dipc_kmsg_clean_recv_inline);
	indent -= 2;
#else	/* DIPC_DO_STATS */
	iprintf("No statistics available\n");
#endif	/* DIPC_DO_STATS */
	indent -= 2;
}

#endif	/* MACH_KDB */
