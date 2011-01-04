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

extern void		dipc_alloc_init(void);
extern void		dipc_receive_init(void);
extern void		dipc_send_init(void);
extern void		dipc_kserver_init(void);
extern void		dipc_rpc_init(void);

extern vm_offset_t 	dipc_alloc(void *, vm_size_t);
extern void		dipc_free(void *, vm_offset_t, vm_size_t);

extern request_block_t	dipc_get_kkt_req(
				boolean_t	wait);

extern request_block_t	dipc_get_kkt_req_chain(
				int,
				unsigned int);

extern void		dipc_free_kkt_req(
				request_block_t	req);

extern void		dipc_free_kkt_req_chain(
				request_block_t	req);

extern vm_page_t	dipc_get_pin_page(
				msg_progress_t	mp);

extern void		dipc_kmsg_destroy_delayed(
				ipc_kmsg_t	kmsg);

extern ipc_kmsg_t	dipc_kmsg_alloc(
				vm_size_t	size,
				boolean_t	thread_context);

extern void		dipc_kmsg_free(
				ipc_kmsg_t	kmsg);

extern meta_kmsg_t	dipc_meta_kmsg_alloc(
				boolean_t	can_block);

extern int		dipc_meta_kmsg_free_count(void);

extern void		dipc_deliver(
				channel_t	channel,
				handle_t	handle,
				endpoint_t	*endpoint,
				vm_size_t	size,
				boolean_t	inlinep,
				boolean_t	thread_context);

extern void		dipc_rpc_server(void);

extern dipc_return_t	dipc_wakeup_blocked_senders(
			        ipc_port_t	port);

extern dipc_return_t	dipc_fill_chain(
				request_block_t	req,
				msg_progress_t	mp,
				boolean_t	can_block);

extern dipc_return_t	dipc_receive_ool(
				msg_progress_t	mp);

extern dipc_return_t	dipc_receive_ool(
				msg_progress_t  mp);

extern int		dipc_prep_pages(
				msg_progress_t	mp);

extern void		dipc_xmit_engine(
				kkt_return_t	kktr,
				handle_t	handle,
				request_block_t	req,
				boolean_t	thread_context);

extern dipc_return_t	dipc_ool_xfer(
				handle_t	handle,
				msg_progress_t	mp,
				request_block_t	chain1,
				boolean_t	steal_context);

extern request_block_t	dipc_get_msg_req(
				msg_progress_t	mp,
				unsigned int	req_type);

extern void		dipc_free_msg_req(
				request_block_t	req,
				boolean_t	thread_context);

kkt_return_t		dipc_send_handle_alloc(
				handle_t	*handle);


dipc_return_t		dipc_enqueue_message(
				meta_kmsg_t	mkmsg,
				boolean_t	*free_handle,
				boolean_t	*kern_port);


extern unsigned int dipc_fast_kmsg_size;

/*
 * The static node map is a fully allocated kkt node map, used to keep track
 * of nodes who wish to be notified when previously unavailable resources
 * are recovered.
 */
extern node_map_t	dipc_static_node_map;
