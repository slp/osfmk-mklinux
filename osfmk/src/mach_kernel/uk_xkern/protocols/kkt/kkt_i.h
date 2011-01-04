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

#ifndef _KKT_I_H_
#define _KKT_I_H_

#include <mach_kdb.h>
#include <cpus.h>
#include <mach/machine/kern_return.h>
#include <mach/kkt_types.h>
#include <mach/kkt_request.h>
#include <xkern/include/xkernel.h>
#include <kern/lock.h>
#include <xkern/include/prot/eth.h>

#define KKT_EXTRA_DEBUG 0
#define KKT_INTRUSIVE   0
#define KKT_STATS	MACH_ASSERT && MACH_KDB

#if KKT_STATS
#define kstat_decl(class, variable)	class unsigned int kkts_##variable;
#define kstat_inc(variable)	kkts_##variable++
#define kstat_add(variable, amount)	kkts_##variable += amount
#define kstat_print(variable)	iprintf("%s = %d\n",#variable,kkts_##variable)
#else
#define kstat_decl(x, y)
#define kstat_inc(x)
#define kstat_add(x, y)
#define kstat_print(x)
#endif

kstat_decl(extern, channel_init)
kstat_decl(extern, send_connect)
kstat_decl(extern, handle_alloc)
kstat_decl(extern, handle_inuse)
kstat_decl(extern, handle_free)
kstat_decl(extern, no_buffer)
kstat_decl(extern, send)
kstat_decl(extern, receive)
kstat_decl(extern, send_block)
kstat_decl(extern, receive_restart)
kstat_decl(extern, bytes_in)
kstat_decl(extern, bytes_out)

#define DONT_USE_IPHOST 1

/*
 * If must_request is FALSE, we associate the data to
 * the KKT_SEND_CONNECT and we try hard to do anything
 * in one shot. Data must be <= MAX_INLINE_SIZE.
 * MAX_INLINE_SIZE is determined as the MTU less the
 * headers which are known to be pushed along the way
 * (TBD: discovery oriented MTU determination)
 */
#define KKT_STACK_SIZE 100 /* ??? KKT_HDR + CHAN_STACK_SIZE + BLAST_STACK_SIZE */
#define MAX_INLINE_SIZE  (MAX_ETH_DATA_SZ - KKT_STACK_SIZE)

/*
 * Anytime KKT allocates a Msg, it preserves MAX_HOTPATH_SIZE
 * for the data section. Should the incoming data be less or
 * equal than MAX_HOTPATH_SIZE, we bcopy the data directly
 * rather than using other message constructors (such as
 * msgJoin or msgConstructInplace).
 */
#define MAX_HOTPATH_SIZE 64

#define MAX_RPC_BUFFER_SIZE MAX_INLINE_SIZE
#define MAX_CHANNEL  8
#define MAX_RESOLUTION_ENTRIES 16
#define KKT_ALLOC_DELAY        1000   /* in ms */

#define valid_channel(channel) ((channel) >= 0 || (channel) < MAX_CHANNEL)

#ifndef MAX
#define MAX(x,y)                ((x)>(y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y)                ((x)<(y) ? (x) : (y))
#endif

/*
 * Message cache stuff
 */
typedef enum {
    CACHED,
    ALLOCATED
} msg_type;

typedef struct {
    Msg_s		message;
    msg_type            msg_type;
    queue_chain_t	link;
} MsgK_s, *MsgK;

typedef struct retval {
        kern_return_t   status;
	kern_return_t   substatus;
} *retval_t;

typedef enum {
    IDLE,
    BUSY,
    ABORTED
} handle_state;

typedef enum {
    SENDCONN,
    SENDCONN_NOREQUEST,
    RPC,
    SENDDATA,
    SENDDATA_REPLY,
    RECVREQ,
    RECVREQ_REPLY,
    ABORT,
    ABORT_REPLY
} service_type;

#define REQUEST_IN_PROGRESS 0x100000

#define START_WINDOW 0x1200
#define MIN_WINDOW   0


typedef enum {
    GO,
    NOGO,
    ERROR
} action;

typedef struct {
	request_block_t		request_list;
	request_block_t		request_end;
} req_queue;
    
typedef struct kkt_handle {
	queue_chain_t	        link;	       
	handle_state            hstate;
	handle_t	        remote_kkth;
	struct kkt_channel	*kchan;
	kkt_error_t             error;
	endpoint_t		endpoint;
	node_name		node;
	kkt_return_t            abort_reason;
	IPhost			host;
	XObj                    lls;
#if KKT_STATS
	XObj                    last_lls;
#endif /* KKT_STATS */
	decl_simple_lock_data   (, handle_lock)
	vm_size_t               window;
	req_queue               send_req;
	req_queue               recv_req; 
	req_queue               callbacks;
	boolean_t
                                is_sender:1,
	                        request_ready:1,
	                        reply_ready:1,
	                        must_request:1,
                                status_set:1,
                                during_deliver:1,
	                        freehandle:1,
                                rpc_reply:1;
	
	boolean_t               data_transfer;
	/*
         * Save area for KKT_CONNECT_REPLY.
         */
        kern_return_t           status, substatus;
	
	/* 
	 * Rendezvous area with dispatch
	 */
        Msg                     request_message;
        Msg                     reply_message;
        char                    *request_data;  	
	/*
	 * Handle retries.
	 * One active per handle.
	 */
	Event			retry_event;
#if KKT_EXTRA_DEBUG
	int                     kkt_request_count;
	int                     kkt_callbacks_count;
#endif /* KKT_EXTRA_DEBUG */
} *kkt_handle_t;

#define kkt_handle_lock(handle)   simple_lock(&(handle)->handle_lock)
#define kkt_handle_unlock(handle) simple_unlock(&(handle)->handle_lock)

#define KKT_HANDLE_NULL (kkt_handle_t)0

typedef struct kkt_channel {
	channel_t	channel;
	service_type    chan_type;
	unsigned int	senders;
	unsigned int	receivers;
	union {
	    kkt_channel_deliver_t   deliver;
	    kkt_rpc_deliver_t       rpc_deliver;
	} input;
	kkt_malloc_t    malloc;
	kkt_free_t      free;
	decl_simple_lock_data   (, chan_lock)
	queue_head_t	handles_list;
	queue_head_t    busy_list;
	int		waiters;
	unsigned int	max_send_size;
	queue_head_t    message_cache;
	Path            chan_path;
} *kkt_channel_t;

#define kkt_channel_lock(chan)   simple_lock(&(chan)->chan_lock)
#define kkt_channel_unlock(chan) simple_unlock(&(chan)->chan_lock)

#define KKT_CHANNEL_NULL (kkt_channel_t)0
#define KKT_MESSAGE_CACHE_NUMBER_DEFAULT  10

/* 
 * KKT-to-KKT protocol headers 
 */

typedef struct {
    node_name           node;
    channel_t           hchan;
    service_type	htype;
    handle_t            hsrc, hdst;
    vm_size_t           window;
    union {
	/*
	 * A bunch of stuff that I need for the *deliver
	 */
	struct sendconn_h {
	    endpoint_t		endpoint; 	/* endpoint param */
	    vm_size_t		size;		/* size param */
	} sendconn;
        struct rpc_h {
	    /* 
	     * HACK! As I do not copy RPC data at the KKT boundary, I
	     * need to have the rpc header contiguous to the data.
	     * Thus, I pad it myself above the rpc_header.
	     */
	    unsigned int        pad1;
	    unsigned int	*ulink;	        /* user available field */
	    handle_t		handle;		/* RPC handle */
	} rpc;
	vm_size_t         bytes_copied;
	/*
	 * Used for reply messages.
	 */
	struct retval retcode;
	kkt_return_t  abort_reason;
    } u;
} KKT_HDR;

typedef struct {
    unsigned long max_length;
} PSTATE;

extern XObj thisprotocol;
extern kkt_channel_t kkt_channels[MAX_CHANNEL];
decl_simple_lock_data( extern, kkt_channel_lock_data )

#define kkt_channel_list_lock()   simple_lock(&kkt_channel_lock_data)
#define kkt_channel_list_unlock() simple_unlock(&kkt_channel_lock_data)

extern node_name _node_self;
extern struct kkt_handle handle_template;
#if XK_DEBUG
extern int tracekktp;
#endif /* XK_DEBUG */


kkt_return_t
kkt_channel_init_common(
			channel_t	channel,
			unsigned int	senders,
			unsigned int	receivers,
			kkt_channel_deliver_t   sendconn,
			kkt_rpc_deliver_t       rpc,
			kkt_malloc_t    kkt_alloc,
			kkt_free_t      kkt_free,
			unsigned int	max_send_size);

kkt_return_t
kkt_handle_alloc_common(
			channel_t	channel,
			handle_t 	*handle,
			kkt_error_t     error,
			boolean_t	wait,
			boolean_t	must_request,
			unsigned int	size,
			boolean_t       preallocate);

kkt_return_t
kkt_handle_free_common(
		       handle_t	handle);

xkern_return_t
xk_open(
	IPhost node,
	unsigned int attr,
	XObj *lls);

xkern_return_t
xk_rpc(
       XObj         lls,
       Msg          request,
       Msg          reply,
       boolean_t    is_async,
       boolean_t    *inprogress);

void
xk_abort(
	 XObj lls);

void
xk_close(
	 XObj lls,
	 boolean_t lls_is_active);

void
kkt_server_upcall(
		  XObj         lls,
		  Msg          request,
		  Msg          reply);

void
kkt_client_upcall(
		  Msg          reply);
	       
xkern_return_t
node_name_to_nap(
		 node_name    in,
		 IPhost       *out);

xkern_return_t
nap_to_node_name(
		 IPhost       in,
		 node_name    *out);

void
kkt_freefunc(
	     void *data, 
	     int   size,
	     void *arg);

void
kkt_fillinfunc( 
	       void *hdr, 
	       char *des, 
	       long len, 
	       void *arg);

long
kkt_pophdr(
	   void *hdr,
	   char *netHdr,
	   long int len,
	   void *arg);

kkt_return_t
kkt_request_send(
		 kkt_handle_t, 
		 request_block_t);

void
kkt_req_to_msg(
	       request_block_t, 
	       Msg, 
	       Path);

void
kkt_req_to_msg_fast(
		    request_block_t, 
		    Msg, 
		    Path);

action
kkt_operate_recv(
		 kkt_handle_t khandle,
		 Msg message,
		 request_block_t req);

action
kkt_operate_send(
		 kkt_handle_t khandle,
		 Msg message);

void
kkt_restart_send(
		 Event e,
		 void  *arg);

void
kkt_operate_callbacks(
		      Event e,
		      void *arg);

void
kkt_sender_unblock(
		   kkt_handle_t khandle);

xkern_return_t
kkt_push_data(
	      kkt_handle_t khandle,
	      Msg message, 
	      KKT_HDR *hdr,
	      service_type type);

void 
xk_kkt_enqueue_request(
		       req_queue *q,
		       request_block_t);


request_block_t 
xk_kkt_dequeue_request(
		       req_queue *q);

void
xk_kkt_abort(kkt_handle_t);

boolean_t
xk_kkt_copy_some(
		 char *ptr, 
		 long len, 
		 void *arg);

kkt_return_t
xk_kkt_copyout(
	       kkt_handle_t, 
	       request_block_t, 
	       Msg);

void
xk_kkt_callback(
		kkt_handle_t khandle,
		request_block_t req,
		kkt_return_t kr);

kkt_handle_t
xk_kkt_handle_check(
		    kkt_channel_t kchan,
		    kkt_handle_t khandle);

Msg
kkt_grab_message(
		 kkt_channel_t kchan);

void
kkt_return_message(
		   kkt_channel_t kchan,
		   Msg           msp);

#endif /* _KKT_I_H_ */
