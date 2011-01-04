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

#include <mach_kdb.h>
#include <kern/assert.h>

#include <mach/boolean.h>
#include <mach/kkt_request.h>
#include <mach/message.h>
#include <kern/lock.h>
#include <kern/sched_prim.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_space.h>
#include <dipc/dipc_kmsg.h>
#include <dipc/dipc_port.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_rpc.h>
#include <dipc/dipc_funcs.h>
#include <dipc/port_table.h>
#include <ddb/tr.h>

#define DIPC_INIT_NUM_KSERVER_THREADS	5
#define DIPC_INCR_NUM_KSERVER_THREADS	3
#define DIPC_MAX_NUM_KSERVER_THREADS   35

int dipc_init_num_kserver_threads = DIPC_INIT_NUM_KSERVER_THREADS;
int dipc_incr_num_kserver_threads = DIPC_INCR_NUM_KSERVER_THREADS;
int dipc_max_num_kserver_threads = DIPC_MAX_NUM_KSERVER_THREADS;
int dipc_current_num_kserver_threads;
int dipc_busy_num_kserver_threads;


ipc_port_t	dipc_kernel_port;
dstat_decl(unsigned int	c_dkp_length = 0;)
dstat_decl(unsigned int	c_dkp_enqueued = 0;)
dstat_decl(unsigned int	c_dkp_handled = 0;)
dstat_decl(unsigned int	c_dkp_max_length = 0;)
dstat_decl(unsigned int c_dkp_replies = 0;)
dstat_decl(unsigned int	c_dkp_sender_waiting = 0;)
dstat_decl(unsigned int	c_dkp_sender_wakeups = 0;)
void dipc_kserver_thread(void);
void dipc_kserver_init(void);

/*
 * Initialize the kserver thread(s) which are responsible for receiving
 * DIPC messages bound for kernel ports.  Allocate the dipc_kernel_port,
 * which is used to queue these messages.  Called from dipc_bootstrap().
 */
void
dipc_kserver_init(void)
{
	int i;

	dipc_kernel_port = ipc_port_alloc_special(ipc_space_kernel);
	assert(dipc_kernel_port != IP_NULL);
	ipc_port_reference(dipc_kernel_port);
	
	dipc_current_num_kserver_threads = dipc_init_num_kserver_threads;
	dipc_busy_num_kserver_threads = 0;
	for (i = 0; i < dipc_init_num_kserver_threads; i++)
	    (void) kernel_thread(kernel_task, dipc_kserver_thread, (char *) 0);
}

/*
 * Each kserver thread loops forever receiving and processing messages that
 * are queued on the dipc_kernel_port.  All incoming DIPC messages whose
 * destination port is in ipc_space_kernel get queued on the dipc_kernel_port.
 */
void
dipc_kserver_thread(void)
{
	boolean_t	sender_waiting;
	unsigned int	waitchan;
	node_name	node;
	ipc_kmsg_t	kmsg;
	dipc_return_t	dr;
	kkt_return_t	kr;
	ipc_port_t	reply;
	ipc_mqueue_t	mqueue;
	mach_port_seqno_t		seqno;
	mach_msg_return_t		mr;
	mach_msg_format_0_trailer_t	*trailer;
	TR_DECL("dipc_kserver_thread");

	ip_lock(dipc_kernel_port);
	for (;;) {

		assert(ip_active(dipc_kernel_port));
		ip_reference(dipc_kernel_port);

		mqueue = &dipc_kernel_port->ip_messages;
		imq_lock(mqueue);
		ip_unlock(dipc_kernel_port);

		mr = ipc_mqueue_receive(mqueue, MACH_MSG_OPTION_NONE,
			MACH_MSG_SIZE_MAX, MACH_MSG_TIMEOUT_NONE, FALSE,
			IMQ_NULL_CONTINUE, &kmsg, &seqno);
		assert(mr == MACH_MSG_SUCCESS);

		ip_lock(dipc_kernel_port);
		dstat_max(dipc_kernel_port->ip_msgcount, c_dkp_max_length);
		dstat(--c_dkp_length);
		dstat(++c_dkp_handled);
		dipc_kernel_port->ip_msgcount--;
		if(++dipc_busy_num_kserver_threads ==
		   dipc_current_num_kserver_threads &&
		   dipc_current_num_kserver_threads <
		   dipc_max_num_kserver_threads) {
			int i;
			dipc_current_num_kserver_threads +=
			    dipc_incr_num_kserver_threads;
			ip_unlock(dipc_kernel_port);
			for(i=0;i<dipc_incr_num_kserver_threads;i++)
			    (void) kernel_thread(kernel_task,
						 dipc_kserver_thread,
						 (char *) 0);
		} else {
			ip_unlock(dipc_kernel_port);
		}
		
		trailer = (mach_msg_format_0_trailer_t *)
			((vm_offset_t)&kmsg->ikm_header +
			round_msg(kmsg->ikm_header.msgh_size));
		trailer->msgh_seqno = seqno;
		trailer->msgh_trailer_size = MACH_MSG_TRAILER_FORMAT_0_SIZE;

		/*
		 *	Must save this bit before copyout turns it off.
		 */
		sender_waiting = KMSG_HAS_WAITING(kmsg);

		dr = dipc_kmsg_copyout(kmsg, current_task()->map,
			MACH_MSG_BODY_NULL);
		assert(dr == DIPC_SUCCESS);

		/*
		 *	Save these things for future reference; the
		 *	kmsg will be trashed by ipc_kobject_server.
		 */
		tr3("received kmsg 0x%x, id %d",kmsg,kmsg->ikm_header.msgh_id);
		if (sender_waiting) {
			dcntr(c_dkp_sender_waiting++);
			waitchan = trailer->dipc_sender_kmsg;
			node = (node_name)kmsg->ikm_handle;
			tr4("...kmsg 0x%x waitchan 0x%x node 0x%x",
			    kmsg, waitchan, node);
		}

		/* service the request */
		kmsg = ipc_kobject_server(kmsg);

		/*
		 * if there's a sending thread waiting, do the rpc to wake
		 * it up.
		 */
		if (sender_waiting) {
			kr = drpc_wakeup_sender(node, waitchan,
						DIPC_SUCCESS, &dr);
			assert(kr == KKT_SUCCESS);
			assert(dr == DIPC_SUCCESS);
			dcntr(c_dkp_sender_wakeups++);
		}

		/* send reply, if any */
		if (kmsg != IKM_NULL) {
			ipc_mqueue_send_always(kmsg);
			dstat(c_dkp_replies++);
		}
		ip_lock(dipc_kernel_port);
		dipc_busy_num_kserver_threads--;
	}
}


#if	MACH_KDB
#include <ddb/db_output.h>

void
db_dipc_kserver(void);
void
db_dipc_kserver(void)
{
	extern int	indent;

	iprintf("Additional Kserver Information:\n");
	indent += 2;
#if	DIPC_DO_STATS
	iprintf("Message Replies:\t%d\n", c_dkp_replies);
#endif	/* DIPC_DO_STATS */
#if	MACH_ASSERT
	iprintf("Sender Waiting:\t%d\n", c_dkp_sender_waiting);
	iprintf("Sender Wakeups:\t%d\n", c_dkp_sender_wakeups);
#endif	/* MACH_ASSERT */
	iprintf("Threads: init %d incr %d busy %d current %d max %d\n",
		dipc_init_num_kserver_threads,
		dipc_incr_num_kserver_threads,
		dipc_busy_num_kserver_threads,
		dipc_current_num_kserver_threads,
		dipc_max_num_kserver_threads);
	indent -= 2;
}
#endif	/* MACH_KDB */
