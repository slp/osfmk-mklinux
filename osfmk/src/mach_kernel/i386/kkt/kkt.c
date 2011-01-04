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
/*
 * Externals for KKT interface.
 */

#include <mach_kdb.h>

#include <mach/std_types.h>
#include <scsi/scsi_defs.h> /* For SCSI_NLUNS */

#include <mach/kkt_request.h>
#include <scsi/scsit.h>
#include <kern/zalloc.h>
#include <kern/misc_protos.h>
#include <ddb/tr.h>
#include <device/io_req.h>
#include <device/disk_status.h>
#include <i386/ipl.h>

#define KKT_DEBUG 0

#if	MACH_ASSERT
#define SPLassert assert
#else	/* MACH_ASSERT */
#define SPLassert(x) if (!(x)) panic("SPLassert")
#endif	/* MACH_ASSERT */

extern int curr_ipl[];

#define	BOOLP(x)	(x ? "" : "!")

#define KKT_STATS	MACH_ASSERT && MACH_KDB

#if KKT_STATS
#define kstat_decl(variable)	unsigned int kkts_##variable = 0;
#define kstat_inc(variable)	kkts_##variable++
#define kstat_add(variable, amount)	kkts_##variable += amount
#define kstat_print(variable)	iprintf("%s = %d\n",#variable,kkts_##variable)
#else
#define kstat_decl(x)
#define kstat_inc(x)
#define kstat_add(x,y)
#define kstat_print(x)
#endif

kstat_decl(channel_init)
kstat_decl(send_connect)
kstat_decl(handle_alloc)
kstat_decl(handle_inuse)
kstat_decl(handle_free)
kstat_decl(no_buffer)
kstat_decl(send)
kstat_decl(receive)

#define KKT_NLUNS SCSI_NLUNS-2
int kkt_luns[KKT_NLUNS];
#define CMD_LUN 0
#define NUMBER_NODES 8
#define WITHINAPAGE 1
#define LATENCY_FIRST 1

#define SendData 0
#define ReceiveData 1

typedef enum {
	Free,
	Allocated,
	Connecting,
	Sender,
	Receiver,
	Aborted,
	IsANode
    } kkt_state_t;

typedef struct kkt_msg {
	enum {
		SendConnect,
		SendConnectInline,
		ConnectReply,
		TransmitData,
		DataComing,
		RPC,
		RPCReply,
		Data,
		HandleAbort
	} service;
	channel_t	channel;
#if	KKT_DEBUG
	struct kkt_event	*kkte;
#endif
	union {
		struct send_connect {
			struct kkt_handle *remote_kkth;
			endpoint_t endpoint;
			vm_size_t size;
		} send_connect;
		struct rpc_buf rpc_buf;
		struct connect_reply {
			struct kkt_handle *kkth;
			struct kkt_handle *remote_kkth;
			kern_return_t status;
			kern_return_t substatus;
		} reply;
		struct transmit_data {
			struct kkt_handle *kkth;
			unsigned int lun;
		} trdata;
		struct handle_abort {
			struct kkt_handle *kkth;
			struct kkt_handle *remote_kkth;
			kern_return_t reason;
			kkt_state_t state;
		} abort;
	} export;
} *kkt_msg_t;

/*
 * This is a not the most elegant way to do this, but what the heck.
 */

#define MAX_RPC_BUFFER_SIZE 256
#define FIXED_BUFFER_SIZE MAX_RPC_BUFFER_SIZE + sizeof(struct kkt_msg)

typedef struct kkt_handle {
	struct kkt_handle 	*next;
	struct kkt_handle	*remote_kkth;
	struct kkt_channel	*kktc;
	kkt_error_t		error;
	kern_return_t		reason;
	node_name		node;
	decl_simple_lock_data(,lock)
	request_block_t		request_list;
	request_block_t		request_end;
	union {
		struct {
			unsigned int
			  must_request:1,
			  data_movement:1,
			  came_up_empty:1,
			  data_lun:3,
			  lun_allocated:1,
			  transmit_data_pending:1,
			  data_coming_pending:1,
			  in_use:1,
			  allocated_sender:1,
			  buffer_available:1;
		} b;
		unsigned int init;
	} bits;
	kkt_state_t state;
#if	MACH_KDB
	struct kkt_handle	*master_list;
#endif	MACH_KDB
	scsit_handle_t		shandle;
	scsit_handle_t		dhandle;
	char 			buffer[FIXED_BUFFER_SIZE];
} *kkt_handle_t;

#define HANDLE_TYPE_SENDER 0
#define HANDLE_TYPE_RECEIVER 1

#define kkt_handle_lock(kkth) simple_lock(&(kkth)->lock)
#define kkt_handle_unlock(kkth) simple_unlock(&(kkth)->lock)

#define KKT_HANDLE_NULL (kkt_handle_t)0

#define CHANNEL_LIM 9

typedef struct kkt_channel {
	channel_t		channel;
	unsigned int		senders;
	unsigned int		receivers;
	unsigned int		current_senders;
	kkt_channel_deliver_t	deliver;
	kkt_malloc_t		malloc;
	kkt_free_t		free;
	decl_simple_lock_data(,lock)
	kkt_handle_t		handles;
	int			waiters;
	unsigned int		max_send_size;
} *kkt_channel_t;

#define KKT_CHANNEL_NULL (kkt_channel_t)0

kkt_channel_t kkt_channels[CHANNEL_LIM];
decl_simple_lock_data(,kkt_channel_lock_data)
#define kkt_channel_list_lock() simple_lock(&kkt_channel_lock_data)
#define kkt_channel_list_unlock() simple_unlock(&kkt_channel_lock_data)

#define valid_channel(channel) ((channel) >= 0 || (channel) < CHANNEL_LIM)

#define kkt_channel_lock(kktc) simple_lock(&(kktc)->lock)
#define kkt_channel_unlock(kktc) simple_unlock(&(kktc)->lock)

typedef enum {
	Idle,
	Command,
	AcceptingData,
	NoChange,
	Down
} kkt_node_state_t;

char *kkt_node_state_name[5] = {
	"Idle",
	"Command",
	"AcceptingData",
	"NoChange",
	"Down"};

typedef struct kkt_event {
	struct kkt_event	*link;
	enum {
		Node,
		Handle,
		HandleSendData,
		HandleFree,
		HandleNoRelease,
		Request
	} 			source;
#if	KKT_DEBUG
	enum {
		EAllocated,
		EFree,
		Queued,
		Sent,
		Completed
	}			status;
#endif
	kkt_node_state_t	state;
	struct kkt_node		*kktn;
	void 			*source_ptr;
	char 			*buffer;
	vm_size_t 		size;
	scsit_handle_t		shandle;
} *kkt_event_t;

#define KKT_EVENT_NULL (kkt_event_t)0

decl_simple_lock_data(,kkt_event_lock_data)
#define kkt_event_lock() simple_lock(&kkt_event_lock_data)
#define kkt_event_unlock() simple_unlock(&kkt_event_lock_data)
kkt_event_t kkt_free_events = KKT_EVENT_NULL;
unsigned int kkt_free_event_elements = 0;
#define MAX_KKT_FREE_EVENTS 10

zone_t kkt_event_zone;
#define KKT_EVENT_ELEM 100

typedef struct kkt_node {
	node_name node;
	kkt_node_state_t state;
	kkt_event_t send_queue;
	kkt_event_t send_end;
	struct {
		kkt_handle_t inuse;
	} lun_info[KKT_NLUNS];
#if	MACH_ASSERT
	unsigned char preps;
	unsigned char recvs;
	unsigned short sending_to_us;
	unsigned length;
#endif	/*MACH_ASSERT*/
#if	KKT_DEBUG
	kkt_event_t kkte;
	kkt_event_t current_event;
#endif
	kkt_handle_t kkth;
	decl_simple_lock_data(,lock)
	struct kkt_handle handle;
} *kkt_node_t;

#define kkt_node_lock(kktn) simple_lock(&(kktn)->lock)
#define kkt_node_unlock(kktn) simple_unlock(&(kktn)->lock)

struct kkt_node kkt_nodes[NUMBER_NODES];

kkt_event_t kkt_event_get(void);
void kkt_event_free(kkt_event_t kkte);
void kkt_event_next(node_name node);
void kkt_event_post(kkt_event_t	kkte);
void kkt_node_prepare(kkt_node_t kktn);
void kkt_node_init(node_name node);
void kkt_recv_complete(
		       scsit_handle_t		handle,
		       void *			opaque,
		       char *			buffer,
		       unsigned int		count,
		       scsit_return_t		sr);
void kkt_target_select(
		       node_name		node,
		       int			lun,
		       unsigned int		size);
void kkt_send_complete(
		       scsit_handle_t		handle,
		       void *			opaque,
		       char *			buffer,
		       unsigned int		count,
		       scsit_return_t		sr);
kkt_return_t kkt_channel_init(
			      channel_t	channel,
			      unsigned int	senders,
			      unsigned int	receivers,
			      kkt_channel_deliver_t	deliver,
			      kkt_malloc_t	malloc,
			      kkt_free_t	free,
			      unsigned int	max_send_size);
void kkt_handle_setup(kkt_handle_t	kkth,
		      channel_t		channel,
		      kkt_error_t	error);
kkt_return_t kkt_handle_alloc(
			      channel_t	channel,
			      handle_t 	*handle,
			      kkt_error_t	error,
			      boolean_t	wait,
			      boolean_t	must_request,
			      unsigned int	size,
			      int type);
unsigned int kkt_get_lun(kkt_node_t kktn,
			 kkt_handle_t kkth);
void kkt_free_lun(
		  kkt_node_t		kktn,
		  unsigned int	lun);
kkt_return_t kkt_handle_free(handle_t handle);
void kkt_data_done(kkt_handle_t		kkth,
		   kkt_node_t		kktn);
void kkt_move_data(
		   kkt_handle_t		kkth,
		   unsigned int		movement,
		   kkt_node_state_t	state);
void kkt_send_data(kkt_handle_t	kkth);
void kkt_receive_data(kkt_handle_t	kkth);
kkt_return_t kkt_handle_abort(kkt_handle_t kkth,
			      kern_return_t reason);
#if	MACH_KDB
kkt_handle_t	kkt_all_handles = KKT_HANDLE_NULL;

void db_show_kkt(void);
void db_show_kkt_msg(kkt_msg_t kktm);
void db_show_kkt_handle(kkt_handle_t kkth);
void db_show_kkt_event(kkt_event_t kkte);
void db_show_kkt_request(request_block_t req);
void db_show_kkt_node(node_name node);
void db_show_kkt_channel(channel_t channel);
void db_show_kkt_node_map(node_map_t map);
#endif	/* MACH_KDB */

kkt_event_t
kkt_event_get(void)
{
	kkt_event_t kkte;
	TR_DECL("kkt_event_get");

	kkt_event_lock();
	if (kkt_free_event_elements) {
		kkt_free_event_elements--;
		kkte = kkt_free_events;
		assert(kkte != KKT_EVENT_NULL);
		kkt_free_events = kkte->link;
		kkt_event_unlock();
		tr2("exit: 0x%x",(unsigned int)kkte);
#if	KKT_DEBUG
		kkte->status = EAllocated;
#endif
		return (kkte);
	}
	kkt_event_unlock();

#if	WITHINAPAGE
	do {
#endif
		kkte = (kkt_event_t)zget(kkt_event_zone);
		assert(kkte != KKT_EVENT_NULL);
#if	WITHINAPAGE
	} while (round_page(kkte) != round_page((kkte+1)));
#endif
	tr2("exit: 0x%x",(unsigned int)kkte);
#if	KKT_DEBUG
	kkte->status = EAllocated;
#endif
	return (kkte);
}

void
kkt_event_free(kkt_event_t kkte)
{
	TR_DECL("kkt_event_free");
	tr2("args: 0x%x",(unsigned int)kkte);
#if	KKT_DEBUG
	kkte->status = EFree;
#endif
	kkt_event_lock();
	if (kkt_free_event_elements < MAX_KKT_FREE_EVENTS) {
		kkt_free_event_elements++;
		kkte->link = kkt_free_events;
		kkt_free_events = kkte;
		kkt_event_unlock();
		return;
	}
	kkt_event_unlock();
	zfree(kkt_event_zone, (vm_offset_t)kkte);
}

/*
 * Must be called with splkkt()
 */

void
kkt_event_next(node_name	node)
{
	kkt_node_t kktn = &kkt_nodes[node];
	kkt_event_t kkte;
	scsit_return_t sr;
	TR_DECL("kkt_event_next");

	tr2("enter: 0x%x", (unsigned int)node);
	SPLassert(curr_ipl[cpu_number()] >= SPL5);
	kkt_node_lock(kktn);
	if (kktn->send_queue != KKT_EVENT_NULL) {
		kkte = kktn->send_queue;
		tr2("dequeued: 0x%x",(unsigned int)kkte);
		kktn->send_queue = kkte->link;
		if (kktn->send_queue == KKT_EVENT_NULL)
		    kktn->send_end = KKT_EVENT_NULL;
		tr2("state transition %s Command",
		    (unsigned int)kkt_node_state_name[kktn->state]);
		kktn->state = Command;
#if	KKT_DEBUG
		assert(kkte->status == Queued);
		kkte->status = Sent;
		kktn->current_event = kkte;
#endif
		kkt_node_unlock(kktn);
		sr = scsit_send(kkte->shandle,
				node, 
				kkt_luns[CMD_LUN],
				(void *)kkte,
				kkte->buffer,
				kkte->size,
				0);
		assert(sr == SCSIT_SUCCESS);
	} else {
		tr2("state transition %s Idle",
		    (unsigned int)kkt_node_state_name[kktn->state]);
#if	KKT_DEBUG
		kktn->current_event = KKT_EVENT_NULL;
#endif	/* KKT_DEBUG */
		kktn->state = Idle;
		kkt_node_unlock(kktn);
	}
	tr1("exit");
}

/*
 * Must be called with splkkt()
 */

void
kkt_event_post(kkt_event_t	kkte)
{
	scsit_return_t sr;
	kkt_node_t kktn = kkte->kktn;
	TR_DECL("kkt_event_post");

	tr2("enter: 0x%x", (unsigned int)kkte);
	assert(kkte);
	SPLassert(curr_ipl[cpu_number()] >= SPL5);
	kkt_node_lock(kktn);
	if (kktn->send_queue != KKT_EVENT_NULL ||
	    kktn->state != Idle) {
		kkte->link = KKT_EVENT_NULL;
		if (kktn->send_end != KKT_EVENT_NULL)
		    kktn->send_end->link = kkte;
		else
		    kktn->send_queue = kkte;
		kktn->send_end = kkte;
#if	KKT_DEBUG
		kkte->status = Queued;
#endif
		kkt_node_unlock(kktn);
		tr1("exit queued");
		return;
	}
	tr2("state transition %s Command",
	    (unsigned int)kkt_node_state_name[kktn->state]);
	kktn->state = Command;
#if	KKT_DEBUG
	kktn->current_event = kkte;
	kkte->status = Sent;
#endif
	kkt_node_unlock(kktn);
	sr = scsit_send(kkte->shandle,
			kktn->node, 
			kkt_luns[CMD_LUN],
			(void *)kkte,
			kkte->buffer,
			kkte->size,
			0);
	assert(sr == SCSIT_SUCCESS);
	tr1("exit sent");
}

void
kkt_node_prepare(kkt_node_t kktn)
{
	scsit_return_t sr;
	kkt_event_t kkte = kkt_event_get();
	TR_DECL("kkt_node_prepare");

	tr2("enter: 0x%x", (unsigned int)kktn->node);
	assert(kktn->preps == kktn->recvs);
#if	KKT_DEBUG
	kktn->kkte = kkte;
#endif
#if	MACH_ASSERT
	assert(kktn->sending_to_us == FALSE);
	kktn->preps++;
	kktn->length = kkte->size;
#endif	/*MACH_ASSERT*/
	kkte->source = Node;
	kkte->source_ptr = (void *)kktn;
	kkte->buffer = kktn->kkth->buffer;
	kkte->size = FIXED_BUFFER_SIZE;
	kkte->kktn = kktn;

	kktn->kkth->state = IsANode;

	sr = scsit_receive(kktn->kkth->shandle,
			   kkte->kktn->node,
			   kkt_luns[CMD_LUN],
			   (void *)kkte,
			   kkte->buffer,
			   kkte->size,
			   0);
	assert(sr == SCSIT_SUCCESS);
	tr1("exit");
}

void
kkt_node_init(node_name		node)
{
	kkt_node_t kktn = &kkt_nodes[node];
	int i;
	spl_t s;
	scsit_return_t sr;
	TR_DECL("kkt_node_init");

	tr2("enter: 0x%x", (unsigned int)node);
#if	MACH_ASSERT
	kktn->preps = kktn->recvs = 0;
	kktn->sending_to_us = FALSE;
#endif	/*MACH_ASSERT*/
#if	KKT_DEBUG
	kktn->kkte = KKT_EVENT_NULL;
	kktn->current_event = KKT_EVENT_NULL;
#endif	/* KKT_DEBUG */
	kktn->node = node;
	kktn->send_queue = KKT_EVENT_NULL;
	kktn->send_end = KKT_EVENT_NULL;
	simple_lock_init(&kktn->lock, ETAP_KKT_NODE);
	for(i=0;i<KKT_NLUNS;i++) {
		kktn->lun_info[i].inuse=KKT_HANDLE_NULL;
	}
	kktn->lun_info[CMD_LUN].inuse=(kkt_handle_t)-1;
	kktn->kkth = &kktn->handle;
#if	MACH_KDB
	kktn->kkth->master_list = kkt_all_handles;
	kkt_all_handles = kktn->kkth;
#endif	/* MACH_KDB */
	sr = scsit_handle_alloc(&kktn->kkth->shandle);
	assert(sr == SCSIT_SUCCESS);
	if (scsit_node_setup(node) == SCSIT_SUCCESS) {
		kktn->state = Idle;
		s = splkkt();
		kkt_node_prepare(kktn);
		splx(s);
	} else {
		kktn->state = Down;
	}
	tr1("exit");
}

void
kkt_recv_complete(
		  scsit_handle_t		handle,
		  void *			opaque,
		  char *			buffer,
		  unsigned int			count,
		  scsit_return_t		sr)
{
	kkt_event_t kkte = (kkt_event_t)opaque;
	kkt_node_t kktn = kkte->kktn;
	kkt_msg_t kktm;
	kkt_return_t kktr;
	kkt_handle_t kkth;
	kkt_channel_t kktc;
	request_block_t request;
	TR_DECL("kkt_recv_complete");

	tr5("enter: 0x%x 0x%x 0x%x 0x%x",
	    (unsigned int)opaque, (unsigned int)buffer,
	    (unsigned int)count, (unsigned int)sr);
	assert(buffer == kkte->buffer);
	assert(count == kkte->size);

#if	KKT_DEBUG
	kkte->status = Completed;
#endif
	switch (kkte->source) {
	      case Node:
		assert(kktn == (kkt_node_t)kkte->source_ptr);
#if	MACH_ASSERT		
		kktn->recvs++;
#endif	/*MACH_ASSERT*/
		assert(kktn->preps == kktn->recvs);
		kktm = (kkt_msg_t)buffer;
		switch (kktm->service) {
		      case HandleAbort:
			tr1("HandleAbort");
			kkth = kktm->export.abort.kkth;
			kkt_handle_lock(kkth);
			/*
			 * There is no reason that this handle is still
			 * in use on this side.  To verify this, we check
			 * the state and the remote_kkth
			 */
			if (kkth->state != Free &&
			    (kktm->export.abort.remote_kkth ==
			     kkth->remote_kkth ||
			      kktm->export.abort.state == Connecting)) {
				assert(kkth->state == Connecting ||
				       !kktm->export.abort.state == Connecting);
				assert(!kkth->bits.b.data_movement);
				if (kkth->state != Aborted) {
					(void) kkt_handle_abort(kkth,
						   kktm->export.abort.reason);
				} else 
				    kkt_handle_unlock(kkth);
			} else
			    kkt_handle_unlock(kkth);
			kkt_node_prepare(kktn);
			break;
		      case SendConnect:
		      case SendConnectInline:
			tr1("SendConnect");
			kkth = kktn->kkth;
			kktr = kkt_handle_alloc(kktm->channel,
						(handle_t *)&kktn->kkth,
						(kkt_error_t)0,
						FALSE,
						FALSE,
						0,
						HANDLE_TYPE_RECEIVER);
			assert(kktr == KKT_SUCCESS);
#if	!LATENCY_FIRST
			kkt_node_prepare(kktn);
#endif
			kkt_handle_setup(kkth, kktm->channel, (kkt_error_t)0);
			kkth->remote_kkth = 
			   (kkt_handle_t)kktm->export.send_connect.remote_kkth;
			kkth->node = kktn->node;
			kkth->state = Connecting;
			kktc = kkt_channels[kktm->channel];
			tr5("*deliver: 0x%x 0x%x 0x%x 0x%x",
			    kktc->channel, kkth,
			    &kktm->export.send_connect.endpoint,
			    kktm->export.send_connect.size);
			tr3("args : 0x%x 0x%x",
			    (kktm->service== SendConnectInline),
			    FALSE);
			(*kktc->deliver)(kktc->channel,
					 (handle_t)kkth,
					 &kktm->export.send_connect.endpoint,
					 kktm->export.send_connect.size,
					 (kktm->service == SendConnectInline),
					 FALSE);
#if    	LATENCY_FIRST
			kkt_node_prepare(kktn);
#endif
			break;
		      case ConnectReply:
			tr1("ConnectReply");
			kkth = kktm->export.reply.kkth;
			bcopy((const char *)kktm, kkth->buffer,
			      FIXED_BUFFER_SIZE);
#if	!LATENCY_FIRST
			kkt_node_prepare(kktn);
#endif
			kkt_handle_lock(kkth);
			if (kkth->state == Connecting) {
				kkth->state = Sender;
				thread_wakeup((event_t)&kkth->state);
			} else
			    assert(kkth->state == Aborted);
			kkt_handle_unlock(kkth);
#if	LATENCY_FIRST
			kkt_node_prepare(kktn);
#endif
			break;
		      case TransmitData:
			tr1("TransmitData");
			kkth = kktm->export.trdata.kkth;
			kkt_handle_lock(kkth);
			kkth->bits.b.data_lun = kktm->export.trdata.lun;
			if (kkth->bits.b.data_lun != CMD_LUN) {
				kkt_handle_unlock(kkth);
				kkt_send_data(kkth);
				kkt_node_prepare(kktn);
			} else {
				if (kkth->bits.b.buffer_available) {
					kkth->bits.b.buffer_available = FALSE;
					kkt_handle_unlock(kkth);
#if	!LATENCY_FIRST
					kkt_node_prepare(kktn);
#endif
					kktm = (kkt_msg_t)kkth->buffer;
					kkte->source_ptr = (void *)kkth;
					kkte->source = HandleSendData;
					kkte->state = AcceptingData;
					kkte->buffer = (char *)kktm;
					kkte->size = FIXED_BUFFER_SIZE;
					kkte->kktn = kktn;
					kktm->service = DataComing;
					kktm->channel = kkth->kktc->channel;
					kktm->export.trdata.kkth =
					    kkth->remote_kkth;
					kkte->shandle = kkth->shandle;
					kkt_event_post(kkte);
					kkte = KKT_EVENT_NULL;
#if	LATENCY_FIRST
					kkt_node_prepare(kktn);
#endif
				} else {
					kkth->bits.b.data_coming_pending = TRUE;
					kkt_handle_unlock(kkth);
					kkt_node_prepare(kktn);
				}
			}
			break;
		      case RPC:
			tr1("RPC");
			kkth = kktn->kkth;
			kktr = kkt_handle_alloc(kktm->channel,
						(handle_t *)&kktn->kkth,
						(kkt_error_t) 0,
						FALSE,
						FALSE,
						0,
						HANDLE_TYPE_RECEIVER);
			assert(kktr == KKT_SUCCESS);
#if	!LATENCY_FIRST
			kkt_node_prepare(kktn);
#endif
			kkt_handle_setup(kkth, kktm->channel, (kkt_error_t)0);
			kkth->remote_kkth = 
			    (kkt_handle_t)kktm->export.rpc_buf.handle;
			kkth->node = kktn->node;
			kktm->export.rpc_buf.handle = (handle_t)kkth;
			kktc = kkt_channels[kktm->channel];
			tr4("*deliver: 0x%x 0x%x 0x%x",
			    kktc->channel, &kktm->export.rpc_buf, FALSE);
			(*(kkt_rpc_deliver_t)kktc->deliver)
			    (kktc->channel,
			     &kktm->export.rpc_buf,
			     FALSE);
#if	LATENCY_FIRST
			kkt_node_prepare(kktn);
#endif
			break;
		      case RPCReply:
			tr1("RPCReply");
			kkth = (kkt_handle_t)kktm->export.rpc_buf.handle;
			bcopy((const char *)kktm, kkth->buffer,
			      FIXED_BUFFER_SIZE);
#if	!LATENCY_FIRST
			kkt_node_prepare(kktn);
#endif
			kktm = (kkt_msg_t)kkth->buffer;
			kktm->export.rpc_buf.handle = (handle_t)kkth;
			thread_wakeup((event_t)kkth);
#if	LATENCY_FIRST
			kkt_node_prepare(kktn);
#endif
			break;
		      case DataComing:
			tr1("DataComing");
#if	MACH_ASSERT
			kktn->sending_to_us = TRUE;
#endif	/*MACH_ASSERT*/
			kkth = kktm->export.trdata.kkth;
			kkt_receive_data(kkth);
			break;
		      case Data:
		      default:
			assert(FALSE);
		}
		break;
	      case Request:
		tr1("Request");
		request = (request_block_t)kkte->source_ptr;
		kkth = (kkt_handle_t)request->transport.next;
		kkt_receive_data(kkth);
		if (request->callback) {
			tr5("*0x%x: 0x%x 0x%x 0x%x",
			    request->callback,
			    (unsigned int)KKT_SUCCESS,
			    (unsigned int)kkth,
			    (unsigned int)request);
			(*(request->callback))(KKT_SUCCESS,
					       (handle_t)kkth,
					       request,
					       FALSE);
		}
		break;
	      case Handle:
	      case HandleNoRelease:
	      default:
		assert(FALSE);
	}
	if (kkte)
	    kkt_event_free(kkte);
	tr1("exit");
}

void
kkt_target_select(
		  node_name			node,
		  int				lun,
		  unsigned int			size)
{
	kkt_node_t kktn = &kkt_nodes[node];
	TR_DECL("kkt_target_select");

	tr4("enter: 0x%x 0x%x 0x%x",
	    (unsigned int)node, (unsigned int)lun, (unsigned int)size);
	tr1("exit");
}

void
kkt_send_complete(
		  scsit_handle_t		handle,
		  void *			opaque,
		  char *			buffer,
		  unsigned int			count,
		  scsit_return_t		sr)
{
	kkt_event_t kkte = (kkt_event_t)opaque;
	kkt_node_t kktn = kkte->kktn;
	kkt_msg_t kktm;
	kkt_return_t kktr;
	kkt_handle_t kkth;
	kkt_channel_t kktc;
	request_block_t request;
	TR_DECL("kkt_send_complete");

	tr5("enter: 0x%x 0x%x 0x%x 0x%x",
	    (unsigned int)opaque, (unsigned int)buffer,
	    (unsigned int)count, (unsigned int)sr);
	assert(buffer == kkte->buffer);
	assert(count == kkte->size);

#if	KKT_DEBUG
	kkte->status = Completed;
#endif
	if (kkte->state == Idle) {
		kkt_event_next(kktn->node);
	} else if (kkte->state == NoChange) {
	} else if (kkte->state != kktn->state) {
		SPLassert(curr_ipl[cpu_number()] >= SPL5);
		kkt_node_lock(kktn);
		tr3("state transition %s %s",
		    (unsigned int)kkt_node_state_name[kktn->state],
		    (unsigned int)kkt_node_state_name[kkte->state]);
		kktn->state = kkte->state;
		kkt_node_unlock(kktn);
	}

	switch (kkte->source) {
	      case HandleFree:
		tr1("HandleFree");
		kkth = (kkt_handle_t)kkte->source_ptr;
		kkt_handle_free((handle_t)kkth);
	      case HandleSendData:
	      case Handle:
		kkth = (kkt_handle_t)kkte->source_ptr;
		kkt_handle_lock(kkth);
		if (kkth->bits.b.in_use) {
			kkth->bits.b.in_use = FALSE;
			if (kkth->state == Free) {
				kkth->state = Allocated;
				kkt_handle_unlock(kkth);
				kkt_handle_free((handle_t)kkth);
				break;
			}
		}
		if (kkth->bits.b.transmit_data_pending) {
			kkth->bits.b.transmit_data_pending = FALSE;
			kkt_handle_unlock(kkth);
			kktm = (kkt_msg_t)kkth->buffer;
			kktm->channel = kkth->kktc->channel;
			kktm->service = TransmitData;
			kktm->export.trdata.kkth = kkth->remote_kkth;
			kktm->export.trdata.lun = kkth->bits.b.data_lun;

			assert(kkte->source == Handle);
			tr1("Handle");
			assert(kkte->source_ptr == (void *)kkth);
			assert(kkte->buffer == (char *)kktm);
			assert(kkte->size == FIXED_BUFFER_SIZE);
			assert(kkte->state == Idle);

			kkt_event_post(kkte);
			kkte = KKT_EVENT_NULL;
		} else {
			kkth->bits.b.buffer_available = TRUE;
			kkt_handle_unlock(kkth);
			if (kkte->source == HandleSendData) {
				tr1("HandleSendData");
				kkt_send_data(kkth);
			} else {
				tr1("Handle");
			}
		}
		break;
	      case HandleNoRelease:
		tr1("HandleNoRelease");
		break;
	      case Request:
		tr1("Request");
		request = (request_block_t)kkte->source_ptr;
		kkth = (kkt_handle_t)request->transport.next;
		kkt_send_data(kkth);
		if (request->callback) {
			tr5("*0x%x: 0x%x 0x%x 0x%x",
			    request->callback,
			    (unsigned int)KKT_SUCCESS,
			    (unsigned int)kkth,
			    (unsigned int)request);
			(*(request->callback))(KKT_SUCCESS,
					       (handle_t)kkth,
					       request,
					       FALSE);
		}
		break;
	      case Node:
	      default:
		assert(FALSE);
	}
	if (kkte != KKT_EVENT_NULL)
	    kkt_event_free(kkte);
	tr1("exit");
}

void kkt_bootstrap_init(void)
{
	int i;
	static boolean_t initialized = FALSE;
	scsit_return_t sr;
	TR_DECL("kkt_bootstrap_init");

	if (initialized)
	    return

	tr1("enter");
	initialized = TRUE;
	scsit_init(64);
	for(i=CMD_LUN;i<KKT_NLUNS;i++) {
		sr = scsit_lun_allocate(kkt_recv_complete,
					kkt_target_select,
					kkt_send_complete,
					&kkt_luns[i]);
		assert(sr == SCSIT_SUCCESS);
	}

	for(i=0;i<CHANNEL_LIM;i++)
	    kkt_channels[i]=KKT_CHANNEL_NULL;

	simple_lock_init(&kkt_channel_lock_data, ETAP_KKT_CHANNEL_LIST);
	simple_lock_init(&kkt_event_lock_data, ETAP_KKT_RESOURCE);

	/*
	 * We should charge these to the channel somehow, but for now
	 * we will be lazy
	 */
	kkt_event_zone = zinit(sizeof(struct kkt_event), 
			       sizeof(struct kkt_event) * KKT_EVENT_ELEM,
			       sizeof(struct kkt_event) * KKT_EVENT_ELEM,
			       "kkt events");
	zone_change(kkt_event_zone, Z_EXHAUST, TRUE);
	zone_change(kkt_event_zone, Z_COLLECT, FALSE);
	zone_change(kkt_event_zone, Z_EXPAND, FALSE);
	zone_enable_spl(kkt_event_zone, splkkt);
	i = zfill(kkt_event_zone, KKT_EVENT_ELEM);
	assert( i >= KKT_EVENT_ELEM);

	for(i=0;i<NUMBER_NODES;i++) {
		kkt_node_init(i);
	}
	tr1("exit");
}

kkt_return_t
kkt_channel_init(
		 channel_t	channel,
		 unsigned int	senders,
		 unsigned int	receivers,
		 kkt_channel_deliver_t	deliver,
		 kkt_malloc_t	malloc,
		 kkt_free_t	free,
		 unsigned int	max_send_size)
{
	kkt_channel_t kktc;
	kkt_handle_t kkth;
	int handles, i;
	spl_t s;
	TR_DECL("kkt_channel_init");

	kstat_inc(channel_init);
	tr5("enter   : 0x%x 0x%x 0x%x 0x%x",
	    (unsigned int)channel, (unsigned int)senders,
	    (unsigned int)receivers, (unsigned int)deliver);
	tr4("enter(2): 0x%x 0x%x 0x%x",
	    (unsigned int)malloc, (unsigned int)free,
	    (unsigned int)max_send_size);
	if (senders <= 0 ||
	    receivers <= 0 ||
	    !deliver ||
	    !malloc ||
	    !free) {
		tr1("exit: KKT_INVALID ARGUMENT");
		return (KKT_INVALID_ARGUMENT);
	}

	if (!valid_channel(channel)) {
		tr1("exit: KKT_INVALID_CHANNEL");
		return (KKT_INVALID_CHANNEL);
	}

	if (kkt_channels[channel] != KKT_CHANNEL_NULL) {
		tr1("exit: KKT_CHANNEL_IN_USE");
		return (KKT_CHANNEL_IN_USE);
	}


	kktc = (kkt_channel_t)(*malloc)(channel, sizeof(struct kkt_channel));
	if (kktc == KKT_CHANNEL_NULL) {
		tr1("exit: KKT_RESOURCE_SHORTAGE");
		return (KKT_RESOURCE_SHORTAGE);
	}

	kktc->channel = channel;
	kktc->senders = senders;
	kktc->receivers = receivers;
	kktc->deliver = deliver;
	kktc->malloc = malloc;
	kktc->free = free;
	kktc->waiters = 0;
	kktc->max_send_size = max_send_size;
	kktc->current_senders = 0;

	simple_lock_init(&kktc->lock, ETAP_KKT_CHANNEL);

	handles = senders + receivers;
	kkth = (kkt_handle_t)(*malloc)(channel,
				       handles * sizeof(struct kkt_handle));
	if (kkth == KKT_HANDLE_NULL) {
		(*free)(channel, (vm_offset_t)kktc,
			sizeof(struct kkt_channel));
		tr1("exit: KKT_RESOURCE_SHORTAGE");
		return (KKT_RESOURCE_SHORTAGE);
	}

	kktc->handles = KKT_HANDLE_NULL;
	for(i=0;i<handles;i++) {
		kkth[i].next = kktc->handles;
		kkth[i].state = Free;
		kktc->handles = &kkth[i];
#if	MACH_KDB
		kkth[i].master_list = kkt_all_handles;
		kkt_all_handles = &kkth[i];
#endif
	}

	s = splkkt();
	kkt_channel_list_lock();
	if (kkt_channels[channel]) {
		kkt_channel_list_unlock();
		splx(s);
		(*free)(channel, (vm_offset_t)kktc,sizeof(struct kkt_channel));
		(*free)(channel, (vm_offset_t)kkth,
			handles * sizeof(struct kkt_handle));
		return (KKT_ERROR);
	}

	kkt_channels[channel] = kktc;
	kkt_channel_list_unlock();
	splx(s);

	tr1("exit");
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_CHANNEL_INIT(channel_t	channel,
		 unsigned int	senders,
		 unsigned int	receivers,
		 kkt_channel_deliver_t	deliver,
		 kkt_malloc_t	malloc,
		 kkt_free_t	free)
{
	TR_DECL("KKT_CHANNEL_INIT");
	tr1("enter");
	return kkt_channel_init(channel, senders, receivers, deliver, malloc,
				free, 0);

}

void kkt_handle_setup(kkt_handle_t	kkth,
		      channel_t		channel,
		      kkt_error_t	error)
{
	kkt_channel_t kktc = kkt_channels[channel];


	kkth->kktc = kktc;
	kkth->state = Allocated;
	kkth->error = error;

	kkth->bits.init = 0;
	kkth->bits.b.buffer_available = TRUE;

	kkth->request_list = NULL_REQUEST;
	kkth->request_end = NULL_REQUEST;
	kkth->reason = 0;
	kkth->remote_kkth = KKT_HANDLE_NULL;
	simple_lock_init(&kkth->lock, ETAP_KKT_HANDLE);
}

kkt_return_t
kkt_handle_alloc(channel_t	channel,
		 handle_t 	*handle,
		 kkt_error_t	error,
		 boolean_t	wait,
		 boolean_t	must_request,
		 unsigned int	size,
		 int		type)

{
	kkt_channel_t kktc;
	kkt_handle_t kkth;
	scsit_return_t sr;
	TR_DECL("kkt_handle_alloc");

	kstat_inc(handle_alloc);
	tr5("enter   : 0x%x 0x%x 0x%x 0x%x",
	    (unsigned int)channel, (unsigned int)handle,
	    (unsigned int)error, (unsigned int)wait);
	tr4("enter(2): 0x%x 0x%x 0x%x",
	    (unsigned int)must_request, (unsigned int)size, type);
	if (!valid_channel(channel) ||
	    kkt_channels[channel] == KKT_CHANNEL_NULL){
		tr1("exit: KKT_INVALID_CHANNEL");
		return (KKT_INVALID_CHANNEL);
	}

	/*
	 * We don't need to lock since all we do is allocate them and
	 * we only do that once
	 */
	kktc = kkt_channels[channel];
	
	if (kktc->max_send_size && size > kktc->max_send_size) {
		tr1("exit: KKT_INVALID_ARGUMENT");
		return (KKT_INVALID_ARGUMENT);
	}

      Retry:
	kkt_channel_lock(kktc);

	kkth = kktc->handles;
	if (kkth == KKT_HANDLE_NULL ||
	    (type == HANDLE_TYPE_SENDER &&
	     kktc->current_senders == kktc->senders)) {
		if (wait) {
			kktc->waiters++;
			assert_wait((event_t)&kktc->handles,FALSE);
			kkt_channel_unlock(kktc);
			thread_block(0);
			goto Retry;
		} else {
			kkt_channel_unlock(kktc);
			tr1("exit: KKT_RESOURCE_SHORTAGE");
			return (KKT_RESOURCE_SHORTAGE);
		}
	}
	if (type == HANDLE_TYPE_SENDER)
		kktc->current_senders++;
	kktc->handles = kkth->next;
	kkt_channel_unlock(kktc);

	assert(kkth->state == Free);
	kkt_handle_setup(kkth, channel, error);
	kkth->bits.b.must_request = must_request;
	if (type == HANDLE_TYPE_SENDER) {
		kkth->bits.b.allocated_sender = TRUE;
	} else {
		kkth->bits.b.allocated_sender = FALSE;
	}

	sr = scsit_handle_alloc(&kkth->shandle);
	assert(sr == SCSIT_SUCCESS);

	*handle = (handle_t) kkth;
	tr2("exit: 0x%x", (unsigned int)kkth);
	return (KKT_SUCCESS);
}
	
kkt_return_t
KKT_HANDLE_ALLOC(channel_t	channel,
		 handle_t 	*handle,
		 kkt_error_t	error,
		 boolean_t	wait,
		 boolean_t	must_request)

{
	kkt_return_t kktr;
	int s;
	TR_DECL("KKT_HANDLE_ALLOC");
	tr1("");
	s = splkkt();
	kktr =  kkt_handle_alloc(channel, handle, error, wait, must_request,
				 0, HANDLE_TYPE_SENDER);
	splx(s);
	return (kktr);
}
	
unsigned int
kkt_get_lun(kkt_node_t kktn,
	    kkt_handle_t kkth)
{
	int i;
	TR_DECL("kkt_get_lun");
	SPLassert(curr_ipl[cpu_number()] >= SPL5);
	kkt_node_lock(kktn);
	for(i=KKT_NLUNS-1;i>CMD_LUN;i--)
	    if (kktn->lun_info[i].inuse == KKT_HANDLE_NULL) {
		    kktn->lun_info[i].inuse = kkth;
		    kkt_node_unlock(kktn);
		    tr2("exit: %d", i);
		    return (i);
	    }
	kkt_node_unlock(kktn);
	tr2("exit: %d", CMD_LUN);
	return (CMD_LUN);
}

void
kkt_free_lun(kkt_node_t		kktn,
	     unsigned int	lun)
{
	TR_DECL("kkt_free_lun");
	SPLassert(curr_ipl[cpu_number()] >= SPL5);
	kkt_node_lock(kktn);
	assert(lun == CMD_LUN || kktn->lun_info[lun].inuse != KKT_HANDLE_NULL);
	kktn->lun_info[lun].inuse = KKT_HANDLE_NULL;
	kkt_node_unlock(kktn);
	tr3("node %d lun %d",kktn->node, lun);
}

/*
 * Must be called with splkkt()
 */

kkt_return_t
kkt_handle_free(handle_t	handle)
{
	kkt_handle_t kkth;
	kkt_channel_t kktc;
	kkt_node_t kktn;
	scsit_return_t sr;
	TR_DECL("kkt_handle_free");

	tr2("enter: 0x%x", (unsigned int)handle);
	assert(handle);
	if (handle == -1) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}
	kstat_inc(handle_free);
	kkth = (kkt_handle_t)handle;
	kktc = kkth->kktc;

	kkt_handle_lock(kkth);
	if (kkth->bits.b.in_use) {
		kstat_inc(handle_inuse);
		kkth->state = Free;
		kkt_handle_unlock(kkth);
		return (KKT_SUCCESS);
	}
	kktn = &kkt_nodes[kkth->node];

	if (kkth->bits.b.data_movement) {
		assert(kkth->state != Aborted);
		assert(kkth->request_list == NULL_REQUEST);
		assert(kkth->request_end == NULL_REQUEST);
		kkt_data_done(kkth, kktn);
	}

	kkt_handle_unlock(kkth);

	sr = scsit_handle_free(kkth->shandle);
	assert(sr == SCSIT_SUCCESS);
	kkth->shandle = (scsit_handle_t)0;

	kkt_channel_lock(kktc);

	kkth->next = kktc->handles;
	kktc->handles = kkth;
	assert(kkth->state != Free);
	kkth->state = Free;
	if (kkth->bits.b.allocated_sender)
	    kktc->current_senders--;

	if (kktc->waiters) {
		kktc->waiters--;
		thread_wakeup_one((event_t)&kktc->handles);
	}

	kkt_channel_unlock(kktc);

	tr1("exit");
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_HANDLE_FREE(handle_t	handle)
{
	int s;
	kkt_return_t kktr;
	TR_DECL("KKT_HANDLE_FREE");
	tr1("");
	s = splkkt();
	kktr = kkt_handle_free(handle);
	splx(s);
	return (kktr);
}

kkt_return_t
KKT_HANDLE_INFO(handle_t 	handle,
		handle_info_t	handle_info)
{
	kkt_handle_t kkth;
	spl_t s;
	TR_DECL("KKT_HANDLE_INFO");

	tr3("enter: 0x%x 0x%x",
	    (unsigned int)handle, (unsigned int)handle_info);
	if (handle == -1 || handle == 0) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}
	s = splkkt();
	kkth = (kkt_handle_t)handle;
	kkt_handle_lock(kkth);

	if (kkth->state != Free) {
		handle_info->node = kkth->node;
		handle_info->abort_code = kkth->reason;
		kkt_handle_unlock(kkth);
		splx(s);
		tr1("exit");
		if (kkth->state == Aborted)
		    return (KKT_ABORT);
		return (KKT_SUCCESS);
	}
	kkt_handle_unlock(kkth);
	splx(s);
	tr1("exit: KKT_INVALID_ARGUMENT");
	return (KKT_INVALID_ARGUMENT);
}

kkt_return_t
KKT_HANDLE_ABORT(handle_t	handle,
		 kern_return_t	reason)
{
	kkt_handle_t kkth;
	kkt_msg_t kktm;
	kkt_event_t kkte;
	kkt_return_t kktr;
	kkt_state_t state;
	spl_t s;
	TR_DECL("KKT_HANDLE_ABORT");

	tr3("enter: 0x%x 0x%x",
	    (unsigned int)handle, (unsigned int)reason);
	if (!handle) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	kkth = (kkt_handle_t)handle;
	if (kkth->state == Free) {
		tr1("exit: KKT_HANDLE_TYPE");
		return (KKT_HANDLE_TYPE);
	}

	assert(!kkth->bits.b.data_movement);	/* We cannot handle this yet */

	s = splkkt();
	kkt_handle_lock(kkth);
	state = kkth->state;
	kkth->state = Aborted;
	assert(kkth->bits.b.buffer_available);
	kkth->bits.b.buffer_available = FALSE;
	kkth->bits.b.in_use = TRUE;
	kkt_handle_unlock(kkth);

	kktm = (kkt_msg_t)kkth->buffer;

	kktm->service = HandleAbort;
	kktm->export.abort.remote_kkth = kkth;
	kktm->export.abort.kkth = kkth->remote_kkth;
	kktm->export.abort.reason = reason;
	kktm->export.abort.state = state;

	kkte = kkt_event_get();
#if	KKT_DEBUG
	kktm->kkte = kkte;
#endif
	kkte->source = Handle;
	kkte->source_ptr = (void *)kkth;
	kkte->buffer = (char *)kktm;
	kkte->size = FIXED_BUFFER_SIZE;
	kkte->state = Idle;
	kkte->kktn = &kkt_nodes[kkth->node];
	kkte->shandle = kkth->shandle;

	kkt_event_post(kkte);

	kkt_handle_lock(kkth);
	kktr = kkt_handle_abort(kkth, reason);
	splx(s);
	return (kktr);
}

kkt_return_t
kkt_handle_abort(kkt_handle_t kkth,
		 kern_return_t reason)
{
	request_block_t req;
	TR_DECL("kkt_handle_abort");

	tr3("enter: 0x%x 0x%x", kkth, reason);
	req = kkth->request_list;
	kkth->request_list = NULL_REQUEST;
	kkth->request_end = NULL_REQUEST;
	kkth->reason = reason;
	if (kkth->state == Connecting) {
		kkth->state = Aborted;
		thread_wakeup((event_t)&kkth->state);
		kkt_handle_unlock(kkth);
	} else {
		kkth->state = Aborted;
		thread_wakeup((event_t)&kkth->state);
		kkt_handle_unlock(kkth);
		while (req) {
			if (req->callback) {
				(*req->callback)(KKT_ABORT,
						 (handle_t)kkth, req, FALSE);
			} else if (kkth->error) {
				(*kkth->error)(KKT_ABORT,
					       (handle_t)kkth, req);
			}
			req = req->transport.next;
		}
	}
	tr1("exit");
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_HANDLE_MIGRATE(node_name		node,
		   int			old_handle,
		   handle_t		new_handle)
{
	kkt_handle_t kkth;
	TR_DECL("KKT_HANDLE_MIGRATE");

	tr4("enter: 0x%x 0x%x 0x%x",
	    (unsigned int)node, (unsigned int)old_handle,
	    (unsigned int)new_handle);

	if (!new_handle) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	kkth = (kkt_handle_t)new_handle;
	assert(kkth->state == Allocated);
	if (node == NODE_NAME_LOCAL)
	    node = scsit_node_self();
	if (!KKT_NODE_IS_VALID(node))
	    return (KKT_ERROR);
	if (kkt_nodes[node].state == Down)
	    return (KKT_NODE_DOWN);
	assert(FALSE);
	tr1("exit");
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_SEND_CONNECT(handle_t	handle,
		 node_name	node,
		 endpoint_t	*endpoint,
		 request_block_t request,
		 boolean_t	more,
		 kern_return_t	*ret)
{
	kkt_handle_t kkth;
	kkt_msg_t kktm;
	scsit_return_t sr;
	kkt_return_t kktr = KKT_SUCCESS;
	kkt_event_t kkte;
	spl_t s;
	TR_DECL("KKT_SEND_CONNECT");

	kstat_inc(send_connect);
	tr5("enter   : 0x%x 0x%x 0x%x 0x%x",
	    (unsigned int)handle, (unsigned int)node,
	    (unsigned int)endpoint, (unsigned int)request);
	tr3("enter(2): 0x%x 0x%x",
	    (unsigned int)more, (unsigned int)ret);
	assert(request);
	assert(handle);

	if ((request->request_type & REQUEST_COMMANDS) != REQUEST_SEND) {
		tr1("exit: KKT_INVALID_REQUEST");
		return (KKT_INVALID_REQUEST);
	}

	kkth = (kkt_handle_t)handle;
	if (kkth->state == Free) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}
	if (kkth->state != Allocated) {
		tr1("exit: KKT_HANDLE_TYPE");
		return (KKT_HANDLE_TYPE);
	}

	if (node == NODE_NAME_LOCAL)
		node = scsit_node_self();

	if (!KKT_NODE_IS_VALID(node))
	    return (KKT_ERROR);

	if (kkt_nodes[node].state == Down)
	    return (KKT_NODE_DOWN);

	s = splkkt();
	kkt_handle_lock(kkth);
	kkth->state = Connecting;
	kkth->node = node;
	assert(kkth->bits.b.buffer_available == TRUE);
	kkth->bits.b.buffer_available = FALSE;
	kkt_handle_unlock(kkth);
	splx(s);

	kktm = (kkt_msg_t)kkth->buffer;

	kktm->service = SendConnect;
	kktm->channel = kkth->kktc->channel;
	if (endpoint)
	    kktm->export.send_connect.endpoint = *endpoint;
	kktm->export.send_connect.size = request->data_length;
	kktm->export.send_connect.remote_kkth = kkth;
	if (request->data_length <= MAX_RPC_BUFFER_SIZE &&
	    !kkth->bits.b.must_request) {
		char *data = (char *)(kktm+1);
		if (request->request_type & REQUEST_SG) 
		    ios_copy_from((io_scatter_t)request->data.list, data,
				  request->data_length);
		else
		    bcopy((const char *)request->data.address, data,
			  request->data_length);
		kktm->service = SendConnectInline;
		kktr = KKT_DATA_SENT;
		kstat_add(send, request->data_length);
		tr1("sending data");
	}

	s = splkkt();
	kkte = kkt_event_get();
#if	KKT_DEBUG
	kktm->kkte = kkte;
#endif
	kkte->source = HandleNoRelease;
	kkte->source_ptr = (void *)kkth;
	kkte->buffer = (char *)kktm;
	kkte->size = FIXED_BUFFER_SIZE;
	kkte->state = Idle;
	kkte->kktn = &kkt_nodes[node];
	kkte->shandle = kkth->shandle;

	tr1("Connecting");
	kkt_event_post(kkte);

	kkt_handle_lock(kkth);
	while (kkth->state != Sender) {
		if (kkth->state == Aborted) {
			kkt_handle_unlock(kkth);
			splx(s);
			tr1("exit: KKT_ABORT");
			return (KKT_ABORT);
		}
		assert_wait((event_t)&kkth->state, FALSE);
		kkt_handle_unlock(kkth);
		splx(s);
		thread_block(0);
		s = splkkt();
		kkt_handle_lock(kkth);
	}

	kkth->remote_kkth = kktm->export.reply.remote_kkth;

	ret[0] = kktm->export.reply.status;
	ret[1] = kktm->export.reply.substatus;

	if (kkth->bits.b.data_coming_pending) {
		kkth->bits.b.data_coming_pending = FALSE;
		kkt_handle_unlock(kkth);
		kktm = (kkt_msg_t)kkth->buffer;
		kkte = kkt_event_get();
#if	KKT_DEBUG
		kktm->kkte = kkte;
#endif
		kkte->source_ptr = (void *)kkth;
		kkte->source = HandleSendData;
		kkte->state = AcceptingData;
		kkte->buffer = (char *)kktm;
		kkte->size = FIXED_BUFFER_SIZE;
		kkte->kktn = &kkt_nodes[node];
		kkte->shandle = kkth->shandle;
		kktm->service = DataComing;
		kktm->channel = kkth->kktc->channel;
		kktm->export.trdata.kkth = kkth->remote_kkth;
		kkt_event_post(kkte);
	} else {
		kkth->bits.b.buffer_available = TRUE; 
		kkt_handle_unlock(kkth);
	}

	splx(s);
	if (kktr == KKT_SUCCESS) {
		kktr = KKT_REQUEST(handle, request);
	}

	tr2("exit: 0x%x",kktr);
	return (kktr);
}

kkt_return_t
KKT_CONNECT_REPLY(handle_t		handle,
		  kern_return_t		status,
		  kern_return_t		substatus)
{
	kkt_handle_t kkth;
	kkt_msg_t kktm;
	kkt_event_t kkte;
	kkt_node_t kktn;
	spl_t s;
	TR_DECL("KKT_CONNECT_REPLY");

	tr4("enter: 0x%x 0x%x 0x%x",
	    (unsigned int)handle, (unsigned int)status, 
	    (unsigned int)substatus);

	if (!handle) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	kkth = (kkt_handle_t)handle;
	if (kkth->state != Connecting) {
		tr1("exit: KKT_HANDLE_TYPE");
		return (KKT_HANDLE_TYPE);
	}

	kktn = &kkt_nodes[kkth->node];

	s = splkkt();
	kkt_handle_lock(kkth);
	assert(kkth->bits.b.buffer_available == TRUE);
	kkth->bits.b.buffer_available = FALSE;
	kkth->state = Receiver;
	kkth->bits.b.in_use = TRUE;
	kkth->bits.b.data_lun = CMD_LUN;
	kkt_handle_unlock(kkth);

	kktm = (kkt_msg_t)kkth->buffer;
	kktm->service = ConnectReply;
	kktm->export.reply.kkth = kkth->remote_kkth;
	kktm->export.reply.remote_kkth = kkth;
	kktm->export.reply.status = status;
	kktm->export.reply.substatus = substatus;

	kkte = kkt_event_get();
#if	KKT_DEBUG
	kktm->kkte = kkte;
#endif
	kkte->source = Handle;
	kkte->source_ptr = (void *)kkth;
	kkte->buffer = (char *)kktm;
	kkte->size = FIXED_BUFFER_SIZE;
	kkte->state = Idle;
	kkte->kktn = kktn;
	kkte->shandle = kkth->shandle;

	kkt_event_post(kkte);
	splx(s);

	tr1("exit");
	return (KKT_SUCCESS);
}

/*
 * Must be called with splkkt()
 */

void kkt_data_done(kkt_handle_t		kkth,
		   kkt_node_t		kktn)
{
	scsit_return_t sr;
	TR_DECL("kkt_data_done");

	tr3("enter: 0x%x 0x%x",kkth, kktn);
	kkth->bits.b.data_movement = FALSE;
	sr = scsit_handle_free(kkth->dhandle);
	assert(sr == SCSIT_SUCCESS);
	kkth->dhandle = (scsit_handle_t)0;
	if (kkth->state == Receiver) {
		assert(kkth->bits.b.lun_allocated);
		kkth->bits.b.lun_allocated = FALSE;
		kkt_free_lun(kktn, kkth->bits.b.data_lun);
		if (kkth->bits.b.data_lun == CMD_LUN) {
#if	MACH_ASSERT
			assert(kktn->sending_to_us);
			kktn->sending_to_us = FALSE;
#endif	/*MACH_ASSERT*/
			kkt_node_prepare(kktn);
		}
	} else if (kkth->bits.b.data_lun == CMD_LUN)
	    kkt_event_next(kktn->node);
	tr1("exit");
}

void
kkt_move_data(kkt_handle_t	kkth,
	      unsigned int	movement,
	      kkt_node_state_t	state)
{
	request_block_t request;
	kkt_event_t kkte;
	scsit_return_t sr;
	kkt_node_t kktn = &kkt_nodes[kkth->node];
	TR_DECL("kkt_move_data");

	tr4("enter: 0x%x 0x%x 0x%x",
	    (unsigned int)kkth, movement, (unsigned int)state);
	assert(movement == SendData || movement == ReceiveData);
	kkt_handle_lock(kkth);
	if (!kkth->bits.b.data_movement) {
		scsit_return_t sr;
		assert(kkth->dhandle == (scsit_handle_t)0);
		sr = scsit_handle_alloc(&kkth->dhandle);
		assert(sr == SCSIT_SUCCESS);
		kkth->bits.b.data_movement = TRUE;
	}
	request = kkth->request_list;
	if (request) {
		boolean_t sg = (request->request_type & REQUEST_SG);
		kkth->request_list = request->transport.next;
		if (kkth->request_end == request)
		    kkth->request_end = NULL_REQUEST;
		kkt_handle_unlock(kkth);
		
		kkte = kkt_event_get();
#if	KKT_DEBUG
		request->transport.kkte = (void *)kkte;
#endif
		kkte->source = Request;
		kkte->source_ptr = (void *)request;
		kkte->buffer = (char *)request->data.address;
		kkte->size = request->data_length;
		kkte->kktn = kktn;
		kkte->state = state;
		request->transport.next = (request_block_t)kkth;

#if	MACH_ASSERT
		kkte->kktn->length = kkte->size;
#endif	/*MACH_ASSERT*/
			sr = ((movement == SendData)?
			      scsit_send:scsit_receive)
			    (kkth->dhandle,
			     kkte->kktn->node,
			     kkt_luns[kkth->bits.b.data_lun],
			     (void *)kkte,
			     kkte->buffer,
			     kkte->size,
			     sg);
		assert(sr == SCSIT_SUCCESS);
	} else {
		kkth->bits.b.came_up_empty = TRUE;
		kkt_handle_unlock(kkth);
	}
	tr1("exit");
}

void
kkt_send_data(kkt_handle_t	kkth)
{
	TR_DECL("kkt_send_data");
	tr1("");
	kkt_move_data(kkth, SendData,
		      (kkth->bits.b.data_lun==CMD_LUN)?AcceptingData:NoChange);
}

void
kkt_receive_data(kkt_handle_t	kkth)
{
#if	MACH_ASSERT
	kkt_node_t kktn = &kkt_nodes[kkth->node];
#endif	/*MACH_ASSERT*/
	TR_DECL("kkt_receive_data");
	tr1("");
	assert(kkth->bits.b.data_lun || kktn->sending_to_us == TRUE);
	assert(kkth->bits.b.data_lun || (kktn->preps == kktn->recvs));
	kkt_move_data(kkth, ReceiveData, NoChange);
}

kkt_return_t
KKT_REQUEST(handle_t			handle,
	    request_block_t		request)
{
	kkt_handle_t kkth;
	kkt_msg_t kktm;
	kkt_event_t kkte;
	kkt_node_t kktn;
	spl_t s;
	TR_DECL("KKT_REQUEST");

	tr3("enter: 0x%x 0x%x", 
	    (unsigned int)handle, (unsigned int)request);

	if (!handle) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	if (!request ||
	    (request->request_type & REQUEST_COMMANDS) != REQUEST_SEND &&
	    (request->request_type & REQUEST_COMMANDS) != REQUEST_RECV) {
		tr1("exit: KKT_INVALID_REQUEST");
		return (KKT_INVALID_REQUEST);
	}

	kkth = (kkt_handle_t)handle;

	s = splkkt();
	if ((request->request_type & REQUEST_COMMANDS) == REQUEST_SEND) {
		tr1("SEND");
		kstat_add(send, request->data_length);
		if (kkth->state != Sender) {
			splx(s);
			tr1("exit: KKT_HANDLE_TYPE");
			return (KKT_HANDLE_TYPE);
		}
		kkt_handle_lock(kkth);
		request->transport.next = NULL_REQUEST;
		if (kkth->request_end != NULL_REQUEST) {
			kkth->request_end->transport.next = request;
			kkth->request_end = request;
			kkt_handle_unlock(kkth);
		} else {
			kkth->request_list = request;
			kkth->request_end = request;
			if (kkth->bits.b.came_up_empty) {
				kkth->bits.b.came_up_empty = FALSE;
				kkt_handle_unlock(kkth);
				kkt_send_data(kkth);
			} else
			    kkt_handle_unlock(kkth);
		}
	} else {
		tr1("RECEIVE");
		kstat_add(receive, request->data_length);
		if (kkth->state != Receiver) {
			splx(s);
			tr1("exit: KKT_HANDLE_TYPE");
			return (KKT_HANDLE_TYPE);
		}
		kkt_handle_lock(kkth);
		request->transport.next = NULL_REQUEST;
		if (kkth->request_end != NULL_REQUEST) {
			kkth->request_end->transport.next = request;
			kkth->request_end = request;
			kkt_handle_unlock(kkth);
			splx(s);
			tr1("exit");
			return (KKT_SUCCESS);
		}
		kkth->request_list = request;
		kkth->request_end = request;
		if (kkth->bits.b.data_movement) {
			if (kkth->bits.b.came_up_empty) {
				kkth->bits.b.came_up_empty = FALSE;
				kkt_handle_unlock(kkth);
				kkt_receive_data(kkth);
			} else {
				kkt_handle_unlock(kkth);
			}
		} else {
			kktn = &kkt_nodes[kkth->node];
			if (!kkth->bits.b.lun_allocated) {
				kkth->bits.b.data_lun =
				    kkt_get_lun(kktn, kkth);
				kkth->bits.b.lun_allocated = TRUE;
				if (kkth->bits.b.buffer_available) {
					kkth->bits.b.buffer_available = FALSE;
					kkt_handle_unlock(kkth);
					kktm = (kkt_msg_t)kkth->buffer;
					kktm->channel = kkth->kktc->channel;
					kktm->service = TransmitData;
					kktm->export.trdata.kkth = 
					    kkth->remote_kkth;
					kktm->export.trdata.lun =
					    kkth->bits.b.data_lun;

					kkte = kkt_event_get();
#if	KKT_DEBUG
					kktm->kkte = kkte;
#endif
					kkte->source = Handle;
					kkte->source_ptr = (void *)kkth;
					kkte->buffer = (char *)kktm;
					kkte->size = FIXED_BUFFER_SIZE;
					kkte->state = Idle;
					kkte->kktn = kktn;
					kkte->shandle = kkth->shandle;
					
					kkt_event_post(kkte);
					if (kkth->bits.b.data_lun != CMD_LUN)
					    kkt_receive_data(kkth);
				} else {
					tr1("No Buffer");
					kstat_inc(no_buffer);
					assert(kkth->bits.b.
					       transmit_data_pending == FALSE);
					kkth->bits.b.transmit_data_pending=TRUE;
					kkt_handle_unlock(kkth);
					if (kkth->bits.b.data_lun != CMD_LUN)
					    kkt_receive_data(kkth);
				}
			} else {
				kkt_handle_unlock(kkth);
				kkt_receive_data(kkth);
			}
		}
	}
	splx(s);

	tr1("exit");
	return (KKT_SUCCESS);
}

kkt_return_t	KKT_COPYOUT_RECV_BUFFER(
			handle_t	handle,
			char		*dest)
{
	kkt_handle_t kkth;
	kkt_msg_t kktm;
	spl_t s;
	TR_DECL("KKT_COPYOUT_RECV_BUFFER");

	tr3("enter: 0x%x 0x%x", handle, dest);

	if (!handle) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	kkth = (kkt_handle_t)handle;

	if (kkth->state != Connecting) {
		tr1("exit: KKT_HANDLE_TYPE");
		return (KKT_HANDLE_TYPE);
	}

	s = splkkt();
	kkt_handle_lock(kkth);
	kktm = (kkt_msg_t)kkth->buffer;
	bcopy((const char *)(kktm + 1), dest, kktm->export.send_connect.size);
	kkt_handle_unlock(kkth);
	splx(s);
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_RPC_INIT(channel_t		channel,
	     unsigned int	senders,
	     unsigned int	receivers,
	     kkt_rpc_deliver_t	deliver,
	     kkt_malloc_t	malloc,
	     kkt_free_t		free,
	     unsigned int	max_send_size)
{
	TR_DECL("KKT_RPC_INIT");
	tr5("args   : 0x%x 0x%x 0x%x 0x%x", 
	    (unsigned int)channel, (unsigned int)senders,
	    (unsigned int)receivers, (unsigned int)deliver);
	tr4("args(2): 0x%x 0x%x 0x%x",
	    (unsigned int)malloc, (unsigned int)free,
	    (unsigned int)max_send_size);
	if (!max_send_size) {
		tr1("exit: KKT_INVALID_REQUEST");
		return (KKT_INVALID_ARGUMENT);
	}

	if (max_send_size > MAX_RPC_BUFFER_SIZE) {
		tr1("exit: KKT_RESOURCE_SHORTAGE");
		return (KKT_RESOURCE_SHORTAGE);
	}

	return kkt_channel_init(channel, senders, receivers,
				(kkt_channel_deliver_t)deliver, malloc,
				free, max_send_size);
}

kkt_return_t
KKT_RPC_HANDLE_ALLOC(channel_t	channel,
		     handle_t *handle,
		     vm_size_t size)
{
	kkt_return_t kktr;
	int s;
	TR_DECL("KKT_RPC_HANDLE_ALLOC");
	tr1("");
	s = splkkt();
	kktr =  kkt_handle_alloc(channel, handle, 0, 1, 0, size,
				 HANDLE_TYPE_SENDER);
	splx(s);
	return (kktr);
}

kkt_return_t
KKT_RPC_HANDLE_BUF(handle_t		handle,
		   void			**userbuf)
{
	kkt_handle_t kkth;
	kkt_msg_t kktm;
	TR_DECL("KKT_RPC_HANDLE_BUF");

	tr3("enter: 0x%x 0x%x", 
	    (unsigned int)handle, (unsigned int)userbuf);

	if (!handle) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	kkth = (kkt_handle_t)handle;

	if (kkth->state == Free) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	kktm = (kkt_msg_t)kkth->buffer;
	*userbuf = (void *)kktm->export.rpc_buf.buff;
	
	tr1("exit");
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_RPC_HANDLE_FREE(handle_t	handle)
{
	kkt_return_t kktr;
	int s;
	TR_DECL("KKT_RPC_HANDLE_FREE");
	tr1("");
	s = splkkt();
	kktr = kkt_handle_free(handle);
	splx(s);
	return (kktr);
}

kkt_return_t
KKT_RPC(node_name		node,
	handle_t		handle)
{
	kkt_event_t kkte;
	kkt_msg_t kktm;
	kkt_handle_t kkth;
	spl_t s;
	TR_DECL("KKT_RPC");

	tr3("enter : 0x%x 0x%x", 
	    (unsigned int)node, (unsigned int)handle);
	if (!handle) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	kkth = (kkt_handle_t)handle;

	if (kkth->state == Free) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	if (node == NODE_NAME_LOCAL)
		node = scsit_node_self();

	if (!KKT_NODE_IS_VALID(node))
	    return (KKT_ERROR);

	if (kkt_nodes[node].state == Down)
	    return (KKT_NODE_DOWN);

	s = splkkt();
	kkth->node = node;

	kktm = (kkt_msg_t)kkth->buffer;
	kktm->service = RPC;
	kktm->channel = kkth->kktc->channel;
	kktm->export.rpc_buf.handle = handle;

	kkte = kkt_event_get();
#if	KKT_DEBUG
	kktm->kkte = kkte;
#endif
	kkte->source = HandleNoRelease;
	kkte->source_ptr = (void *)kkth;
	kkte->buffer = (char *)kktm;
	kkte->size = FIXED_BUFFER_SIZE;
	kkte->state = Idle;
	kkte->kktn = &kkt_nodes[node];
	kkte->shandle = kkth->shandle;

	assert_wait((event_t)kkth, FALSE);
	tr1("sending");
	kkt_event_post(kkte);
	
	splx(s);

	thread_block(0);

	tr1("exit");
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_RPC_REPLY(handle_t		handle)
{
	kkt_event_t kkte;
	kkt_msg_t kktm;
	kkt_handle_t kkth;
	spl_t s;
	TR_DECL("KKT_RPC_REPLY");

	tr2("enter: 0x%x",(unsigned int)handle);
	if (!handle) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	kkth = (kkt_handle_t)handle;

	if (kkth->state == Free) {
		tr1("exit: KKT_INVALID_HANDLE");
		return (KKT_INVALID_HANDLE);
	}

	s = splkkt();

	/*
	 * No reason to lock this.  If the user
	 * does this on the same handle at the same
	 * time, they get what they deserve
	 */

	kktm = (kkt_msg_t)kkth->buffer;
	kktm->service = RPCReply;
	kktm->channel = kkth->kktc->channel;
	kktm->export.rpc_buf.handle = (handle_t)kkth->remote_kkth;

	kkte = kkt_event_get();
#if	KKT_DEBUG
	kktm->kkte = kkte;
#endif
	kkte->source = HandleFree;
	kkte->source_ptr = (void *)kkth;
	kkte->buffer = (char *)kktm;
	kkte->size = FIXED_BUFFER_SIZE;
	kkte->state = Idle;
	kkte->kktn = &kkt_nodes[kkth->node];
	kkte->shandle = kkth->shandle;

	kkt_event_post(kkte);

	splx(s);
	tr1("exit");
	return (KKT_SUCCESS);
}

typedef unsigned int kkt_map_t;
#define MAP_SIZE sizeof(kkt_map_t)

kkt_return_t
KKT_MAP_ALLOC(node_map_t	*map,
	      boolean_t		full)
{
	kkt_map_t *cmap;
	TR_DECL("KKT_MAP_ALLOC");
	tr3("enter: 0x%x 0x%x", 
	    (unsigned int)map, (unsigned int)full);
	*map = (node_map_t)kalloc(MAP_SIZE);
	cmap = (kkt_map_t *)(*map);
	*cmap = 0;
	tr2("exit: 0x%x",*map);
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_MAP_FREE(node_map_t		map)
{
	TR_DECL("KKT_MAP_FREE");
	tr2("enter: 0x%x", (unsigned int)map);
	kfree(map, MAP_SIZE);
	tr1("exit");
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_ADD_NODE(node_map_t		map,
	     node_name		node)
{
	kkt_map_t *cmap = (kkt_map_t *)map;
	TR_DECL("KKT_ADD_NODE");
	tr3("enter: 0x%x 0x%x", 
	    (unsigned int)map, (unsigned int)node);
	assert(KKT_NODE_IS_VALID(node));
	if (*cmap & 1<<node)
	    return KKT_NODE_PRESENT;
	*cmap |= 1<<node;
	tr1("exit");
	return (KKT_SUCCESS);
}

kkt_return_t
KKT_REMOVE_NODE(node_map_t		map,
		node_name		*node)
{
	kkt_map_t *cmap = (kkt_map_t *)map;
	TR_DECL("KKT_REMOVE_NODE");

	tr3("enter: 0x%x 0x%x", 
	    (unsigned int)map, (unsigned int)node);
	if (*cmap == 0) {
		tr1("exit: KKT_EMPTY");
		return (KKT_MAP_EMPTY);
	} else {
		*node = ffs(*cmap)-1;
		*cmap &= ~(1<<*node);
		if (*cmap == 0) {
		tr2("exit: KKT_RELEASE 0x%x", *node);
		    return (KKT_RELEASE);
		} else {
			tr2("exit: KKT_SUCCESS 0x%x", *node);
			return (KKT_SUCCESS);
		}
	}
}

kkt_return_t	KKT_PROPERTIES(
			kkt_properties_t *prop)
{
	prop->kkt_upcall_methods = KKT_INTERRUPT_UPCALLS;
	prop->kkt_buffer_size = MAX_RPC_BUFFER_SIZE;
	return (KKT_SUCCESS);
}


node_name KKT_NODE_SELF(void)
{
	return scsit_node_self();
}

boolean_t
KKT_NODE_IS_VALID(
		  node_name node)
{
	if (node >= 0 && node <= 7)
	    return TRUE;
	else
	    return FALSE;
}

#if	MACH_KDB
#include <ddb/db_output.h>
void
db_show_kkt_msg(
	     kkt_msg_t kktm)
{
	char *service[9] = {	"SendConnect",
				"SendConnectInline",
				"ConnectReply",
				"TransmitData",
				"DataComing",
				"RPC",
				"RPCReply",
				"Data",
			    	"HandleAbort"};
	iprintf("KKT: kkt_msg_t 0x%x\n",kktm);
	iprintf("  Service: %s  Channel 0x%x\n",
	       service[kktm->service],kktm->channel);
#if	KKT_DEBUG
	iprintf("  Event 0x%x\n", kktm->kkte);
#endif
}

void
db_show_kkt_handle(kkt_handle_t kkth)
{
	request_block_t req;
	char *state[7]={
		"Free",
		"Allocated",
		"Connecting",
		"Sender",
		"Receiver",
		"Aborted",
		"IsANode"};

	iprintf("KKT: kkt_handle_t 0x%x\n",kkth);
	indent +=2;
	iprintf("next 0x%x remote_kkth 0x%x kktc 0x%x error 0x%x\n",
	       kkth->next, kkth->remote_kkth, kkth->kktc, kkth->error);
	iprintf("buffer 0x%x node 0x%x reason 0x%x\n",
		kkth->buffer, kkth->node, kkth->reason);
	if (kkth->request_list) {
		iprintf("Request_list:");
		for(req = kkth->request_list;
		    req != NULL_REQUEST;
		    req = req->transport.next)
		    db_printf(" 0x%x",req);
		db_printf("\n");
	}
	iprintf("%smust_request %sdata_movement %scame_up_empty\n",
		BOOLP(kkth->bits.b.must_request),
		BOOLP(kkth->bits.b.data_movement),
		BOOLP(kkth->bits.b.came_up_empty));
	iprintf("%sbuffer_available %sin_use %slun_allocated %ssender\n",
		BOOLP(kkth->bits.b.buffer_available),
		BOOLP(kkth->bits.b.in_use),
		BOOLP(kkth->bits.b.lun_allocated),
		BOOLP(kkth->bits.b.allocated_sender));
	iprintf("Data Lun 0x%x State %s shandle 0x%x dhandle 0x%x\n",
		kkth->bits.b.data_lun,
		state[kkth->state], kkth->shandle, kkth->dhandle);
	indent -=2;
}

void
db_show_kkt_event(kkt_event_t kkte)
{
	char *source[6]={
		"Node",
		"Handle",
		"HandleSendData",
		"HandleFree",
		"HandleNoRelease",
		"Request"};

#if	KKT_DEBUG
	char *status[5]={
		"Allocated",
		"Free",
		"Queued",
		"Sent",
		"Completed"};
#endif

	iprintf("KKT: kkt_event_t 0x%x\n",kkte);
	indent +=2;
	iprintf("link 0x%x source %s node_state %s\n",
	       kkte->link, source[kkte->source],
	       kkt_node_state_name[kkte->state]);
	iprintf("kktn 0x%x src_ptr 0x%x buffer 0x%x size 0x%x\n",
	       kkte->kktn, kkte->source_ptr, kkte->buffer, kkte->size);
#if	KKT_DEBUG
	iprintf("Status: %s\n",status[kkte->status]);
#endif
	indent -=2;
}

void
db_show_kkt_request(request_block_t req)
{
	iprintf("KKT: request_block_t 0x%x\n",req);
	indent +=2;
	iprintf("next=0x%x\n", req->next);
	iprintf("request_type=%s%s, data.address=0x%x, data_length=0x%x\n",
	       (req->request_type & REQUEST_SG)?"SG/":"",
	       (req->request_type & REQUEST_SEND) ? "REQUEST_SEND" :
	       (req->request_type & REQUEST_RECV) ? "REQUEST_RECV" :
		"UNKNOWN",
	       req->data.address, req->data_length);
	iprintf("callback=0x%x, args[0]=0x%x, args[1]=0x%x\n", req->callback,
	       req->args[0], req->args[1]);
#if	KKT_DEBUG
	iprintf("  Event 0x%x\n", req->transport.kkte);
#endif
	indent -=2;
}

void
db_show_kkt_node(node_name node)
{
	kkt_node_t kktn = (kkt_node_t)node;
	kkt_event_t kkte;
	int i;
	if (KKT_NODE_IS_VALID(node))
	    kktn = &kkt_nodes[node];
	for(i=0;i<NUMBER_NODES;i++)
	    if (kktn == &kkt_nodes[i]) break;
	if (i == NUMBER_NODES)
	    return;

	iprintf("KKT: kkt_node_t 0x%x\n",kktn);
#if	MACH_ASSERT
	if (kktn->recvs == 0)
	    return;
#endif	MACH_ASSERT
	if (kktn->state == Down)
	    return;
	indent +=2;
	iprintf("node 0x%x state %s queue",
	       kktn->node, kkt_node_state_name[kktn->state]);
	for(kkte = kktn->send_queue;
	    kkte != KKT_EVENT_NULL;
	    kkte = kkte->link)
	    db_printf(" 0x%x", kkte);
	db_printf("\n");
	iprintf("Lun Usage:\n");
	indent +=2;
	for(i=CMD_LUN;i<KKT_NLUNS;i++) {
		iprintf("[%2d]0x%x", i, kktn->lun_info[i].inuse);
		db_printf("\n");
	}
	indent -= 2;
#if	MACH_ASSERT
	iprintf("prep 0x%x recv 0x%x %ssending_to_us length 0x%x\n",
	       kktn->preps, kktn->recvs, BOOLP(kktn->sending_to_us),
	       kktn->length);
#endif	/*MACH_ASSERT*/
#if	KKT_DEBUG
	iprintf("  Event 0x%x Current Event 0x%x\n",
		kktn->kkte, kktn->current_event);
#endif
	db_show_kkt_handle(kktn->kkth);
	indent -=2;
}
void
db_show_kkt_channel(channel_t	channel)
{
	kkt_channel_t kktc = (kkt_channel_t)channel;
	int i;
	kkt_handle_t kkth;

	if (valid_channel(channel))
	    kktc = kkt_channels[channel];
	if (kktc == KKT_CHANNEL_NULL)
	    return;
	for(i=0;i<CHANNEL_LIM;i++)
	    if (kktc == kkt_channels[i])
		break;
	if (i == CHANNEL_LIM)
	    return;

	iprintf("KKT: kkt_channel_t 0x%x\n", kktc);
	indent += 2;
	iprintf("channel 0x%x senders %d receivers %d current_senders %d\n",
		kktc->channel, kktc->senders, kktc->receivers,
		kktc->current_senders);
	iprintf("deliver 0x%x malloc 0x%x free 0x%x\n",
		kktc->deliver, kktc->malloc, kktc->free);
	iprintf("waiters %d max_send_size 0x%x\n",
		kktc->waiters, kktc->max_send_size);

	iprintf("Active Handles:\n");
	indent += 2;
	for(kkth = kkt_all_handles;
	    kkth != KKT_HANDLE_NULL;
	    kkth = kkth->master_list)
	    if (kkth->kktc == kktc && kkth->state != IsANode &&
		(kkth->state != Free || kkth->bits.b.in_use ||
		 kkth->dhandle || kkth->shandle))
		db_show_kkt_handle(kkth);
	indent -=4;
}

void
db_show_kkt(void)
{
	int i, th=0;
	kkt_handle_t kkth;
	iprintf("KKT Stats\n");
	indent += 2;
	kstat_print(channel_init);
	kstat_print(send_connect);
	kstat_print(handle_alloc);
	kstat_print(handle_inuse);
	kstat_print(handle_free);
	kstat_print(no_buffer);
	kstat_print(send);
	kstat_print(receive);
	for(kkth = kkt_all_handles;
	    kkth != KKT_HANDLE_NULL;
	    kkth = kkth->master_list)
	    if (kkth->state != IsANode &&
		(kkth->state != Free || kkth->bits.b.in_use ||
		 kkth->dhandle || kkth->shandle))
		th++;
	iprintf("Total inuse handles %d\n",th);
	for(i=0;i<NUMBER_NODES;i++)
	    db_show_kkt_node(i);
	for(i=0;i<CHANNEL_LIM;i++)
	    db_show_kkt_channel(i);
#if	SCSIT_STATS
	scsit_stats();
#endif
	indent -=2;
}

void
db_show_kkt_node_map(
	node_map_t	map)
{
	iprintf("Node map 0x%x:  no show map routine.\n", map);
}


#endif	/*MACH_KDB*/
