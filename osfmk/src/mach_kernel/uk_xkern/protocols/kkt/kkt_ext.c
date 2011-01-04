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

#include <uk_xkern/protocols/kkt/kkt_i.h>
#include <uk_xkern/include/eth_support.h>
#include <xkern/include/prot/ip_host.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_funcs.h>
#include <kernel_test.h>
#include <kern/queue.h>
#include <kern/clock.h>
#include <i386/rtclock_entries.h>

#if  KKT_EXTRA_DEBUG
kkt_handle_t lastkhandle, lastremkhandle;
#if  KKT_INTRUSIVE
#define MARK(X)   putchar(X)
#else  /* KKT_INTRUSIVE */
char kkt_array[200];
unsigned short kkt_cur;
#define MARK(X)   {kkt_cur++; kkt_array[kkt_cur%200] = X; kkt_array[(kkt_cur+1)%200] = 0xff;}
#endif /* KKT_INTRUSIVE */
#else  /* KKT_EXTRA_DEBUG */
#define MARK(X)
#endif /* KKT_EXTRA_DEBUG */

kstat_decl(, channel_init)
kstat_decl(, send_connect)
kstat_decl(, handle_alloc)
kstat_decl(, handle_inuse)
kstat_decl(, handle_free)
kstat_decl(, no_buffer)
kstat_decl(, send)
kstat_decl(, receive)
kstat_decl(, send_block)
kstat_decl(, receive_restart)
kstat_decl(, bytes_in)
kstat_decl(, bytes_out)

struct kkt_handle handle_template; 

#define WINDOW_FUNCTION(X) (X)

#if  KERNEL_TEST

extern dmt_start(
		 node_name node);

extern kern_msg_test(
		 node_name node);

void
transport_test_setup(
		     void);

void
transport_send_test(
		    vm_size_t size,
		    vm_offset_t data,
		    node_name node);
void
transport_recv_test(
		    handle_t handle,
		    vm_size_t size,
		    unsigned int *ulink,
		    vm_offset_t data);
#endif  /*KERNEL_TEST*/

#if	MACH_KDB
void 
db_show_kkt(
	    void *arg);

void 
db_show_kkt_msg(
		void *arg);

void 
db_show_kkt_handle(
		   kkt_handle_t khandle);

void 
db_show_kkt_event(
		  void *arg);

void 
db_show_kkt_request(
		    request_block_t req);

void 
db_show_kkt_node(
		 void *arg);

void 
db_show_kkt_channel(
		    channel_t	channel);

void 
db_show_kkt_node_map(
		     node_map_t map);
#endif	/* MACH_KDB */

/*
 * Externals for KKT interface.
 */
node_name
KKT_NODE_SELF(
	      void)
{
    return _node_self;
}

void 
kkt_bootstrap_init(
		   void)
{
    extern int dipc_interrupt_delivery;

    dipc_interrupt_delivery = FALSE;

    simple_lock_init(&kkt_channel_lock_data, ETAP_KKT_CHANNEL_LIST);
    /* 
     * Initialize the x-kernel world 
     */
    xkInit();
    while (thisprotocol == ERR_XOBJ) {
	/* 
	 * Synchronize with the thread which builds up
	 * the x-kernel protocol graph.
	 */
	assert_wait((event_t) &thisprotocol, FALSE);
	thread_block((void (*)(void)) 0);
    }
    assert(KKT_NODE_SELF() != NODE_NAME_NULL);

    memset((char *)&handle_template, 0, sizeof(handle_template));

    handle_template.hstate = BUSY;
    handle_template.node = NODE_NAME_NULL;
    handle_template.window = START_WINDOW;
    handle_template.lls = ERR_XOBJ;
}

kkt_return_t
KKT_PROPERTIES(
	       kkt_properties_t *prop)
{
    prop->kkt_upcall_methods = KKT_THREAD_UPCALLS;
    prop->kkt_buffer_size = MAX_INLINE_SIZE;

    return KKT_SUCCESS;
}

kkt_return_t
KKT_CHANNEL_INIT(
		 channel_t	channel,
		 unsigned int	senders,
		 unsigned int	receivers,
		 kkt_channel_deliver_t  deliver,
		 kkt_malloc_t     	malloc,
		 kkt_free_t	       	free)
{
    return kkt_channel_init_common(channel, senders, receivers, deliver, 
			           (kkt_rpc_deliver_t) 0, malloc, free, 0);
}

kkt_return_t
KKT_RPC_INIT(
	     channel_t		channel,
	     unsigned int	senders,
	     unsigned int	receivers,
	     kkt_rpc_deliver_t  rpc_deliver,
	     kkt_malloc_t       malloc,
	     kkt_free_t	        free,
	     unsigned int	max_send_size)
{
    if (!max_send_size)
	return KKT_INVALID_ARGUMENT;

    if (max_send_size > MAX_RPC_BUFFER_SIZE)
	return KKT_RESOURCE_SHORTAGE;

    return kkt_channel_init_common(channel, senders, receivers, (kkt_channel_deliver_t) 0,
				   rpc_deliver, malloc, free, max_send_size);
}

kkt_return_t
KKT_HANDLE_ALLOC(
		 channel_t	channel,
		 handle_t 	*handle,
		 kkt_error_t 	error,
		 boolean_t	wait,
		 boolean_t	must_request)

{
    return kkt_handle_alloc_common(channel, handle, error, wait, must_request,
				   0, TRUE);
}

kkt_return_t
KKT_RPC_HANDLE_ALLOC(
		     channel_t	channel,
		     handle_t *handle,
		     vm_size_t size)
{
    return kkt_handle_alloc_common(channel, handle, (kkt_error_t)0, TRUE, FALSE, 
				   size, TRUE);
}
	
kkt_return_t
KKT_HANDLE_FREE(
		handle_t	handle)
{
    kkt_handle_t khandle = (kkt_handle_t)handle;

    assert(khandle);
    if (khandle->hstate != BUSY && khandle->hstate != ABORTED) 
	return KKT_INVALID_ARGUMENT;
    
    assert(khandle->freehandle == FALSE);
    if (khandle->during_deliver) {
	khandle->freehandle = TRUE;
	return KKT_SUCCESS;
    }
    return kkt_handle_free_common(handle);
}

kkt_return_t
KKT_RPC_HANDLE_FREE(
		    handle_t	handle)
{
    kkt_handle_t khandle = (kkt_handle_t)handle;

    assert(khandle);
    if (khandle->hstate != BUSY) 
	return KKT_INVALID_ARGUMENT;
    
    /*
     * At the receiver side, the KKT_RPC_HANDLE_FREE
     * is implicit. Thus, we two conditions below 
     * should always be verified.
     */
    assert(khandle->during_deliver == FALSE);
    assert(khandle->freehandle == FALSE);

    return kkt_handle_free_common(handle);
}

kkt_return_t
KKT_HANDLE_INFO(
		handle_t 	handle,
		handle_info_t	handle_info)
{
    kkt_handle_t khandle = (kkt_handle_t)handle;

    assert(khandle);

    if (khandle->hstate != BUSY && khandle->hstate != ABORTED) 
	return KKT_INVALID_ARGUMENT;

    handle_info->node = khandle->node;
    handle_info->abort_code = khandle->abort_reason;

    if (khandle->hstate == ABORTED) 
	return KKT_ABORT;

    return KKT_SUCCESS;
}

kkt_return_t
KKT_HANDLE_ABORT(
		 handle_t	handle,
		 kkt_return_t	reason)
{
    kkt_handle_t khandle = (kkt_handle_t)handle;
    kkt_channel_t kchan;
    Msg msp;
    xkern_return_t xkr;
    KKT_HDR hdr;

    assert(khandle);
    if (khandle->hstate != BUSY) 
	return KKT_INVALID_ARGUMENT;

    kchan = khandle->kchan;
    khandle->hstate = ABORTED;
    khandle->abort_reason = reason;
    /*
     * Abort any RPC that is still in progress.
     */
    if (khandle->data_transfer) {
	xk_abort(khandle->lls);
	khandle->data_transfer = FALSE;
    }

    if (khandle->remote_kkth != (handle_t)0) {

	Msg_s request, reply;
	xkern_return_t xkr1, xkr2 = XK_SUCCESS;
	/*
	 * Send the good news to the peer.
	 */
	if ((xkr1 = msgConstructContig(&request, kchan->chan_path, 0, KKT_STACK_SIZE)) == XK_SUCCESS &&
	    (xkr2 = msgConstructContig(&reply, kchan->chan_path, 0, KKT_STACK_SIZE)) == XK_SUCCESS) {

	    hdr.htype = ABORT;
	    hdr.node = _node_self;
	    hdr.hchan = kchan->channel;
	    hdr.hsrc = (handle_t)khandle;
	    hdr.hdst =  khandle->remote_kkth;
	    hdr.u.abort_reason = reason;
	    xkr = msgPush(&request, kkt_fillinfunc, &hdr, sizeof (hdr), (void *)0);
	    assert(xkr == XK_SUCCESS);
	    xk_thread_checkin();
	    xkr = xk_rpc(khandle->lls, &request, &reply, FALSE, &khandle->data_transfer); 
	    msgDestroy(&request);
	    msgDestroy(&reply);
	    xk_thread_checkout(FALSE);
	} else {
	    if (xkr2 != XK_SUCCESS)
		msgDestroy(&request);
	}
    }

    /*
     * Now we flush the queue and we make the upcalls.
     */
    xk_kkt_abort(khandle);
    
    return KKT_SUCCESS;
}

kkt_return_t
KKT_HANDLE_MIGRATE(
		   node_name		node,
		   int			old_handle,
		   handle_t		new_handle)
{
    /* 
     * Later ?.
     */
    panic("KKT_HANDLE_MIGRATE");
    return KKT_SUCCESS;
}

kkt_return_t
KKT_RPC_HANDLE_BUF(
		   handle_t		handle,
		   void			**userbuf)
{
    kkt_handle_t khandle = (kkt_handle_t)handle;

    assert(khandle);
    if (khandle->hstate != BUSY)
	return KKT_INVALID_ARGUMENT;

    assert(khandle->request_ready);

    *userbuf = khandle->request_data;
    assert(*userbuf);

    return KKT_SUCCESS;
}

#if KERNEL_TEST
unsigned int kkt_recurs;
unsigned int kkt_recurs_end;

void kkt_recursion(void);
void kkt_recursion_end(void);

void
kkt_recursion(
	      void)
{
    kkt_recurs++;
}
void
kkt_recursion_end(
	      void)
{
    kkt_recurs_end++;
}
#endif /* KERNEL_TEST */

kkt_return_t
KKT_SEND_CONNECT(
		 handle_t	handle,
		 node_name	node,
		 endpoint_t	*endpoint,
		 request_block_t request,
		 boolean_t	more,
		 kern_return_t	*ret)
{
    kkt_handle_t khandle = (kkt_handle_t)handle;
    kkt_channel_t kchan;
    char *addr;
    unsigned int size;
    Msg_s  message, *msp;
    xkern_return_t xkr;
    KKT_HDR hdr, rhdr;
    struct retval *rv;
    boolean_t inlinep, cachep;
    boolean_t node_change = FALSE;
    
    assert(request);
    assert(khandle);

#if    !KERNEL_TEST
    assert(!is_xk_input_thread(current_thread()));
#endif /* KERNEL_TEST */

    kchan = khandle->kchan;
    assert(kchan);

    if (khandle->hstate != BUSY) {
	return KKT_INVALID_HANDLE;
    }

    /* 
     * Loopback clause
     */
    if (node == NODE_NAME_LOCAL)
	node = _node_self;

    xk_thread_checkin();
    if (node != khandle->node) {
	/*
	 * Either the RPC handle was never used before, or it
	 * was used with a different target node.
	 */
	if (node_name_to_nap(node, &khandle->host) != XK_SUCCESS) {
	    xk_thread_checkout(FALSE);
	    return KKT_NODE_DOWN;
	}
	khandle->node = node;
	node_change = TRUE;
    }
    if (khandle->lls == ERR_XOBJ || node_change) {
	/*
	 * Either we don't have a lls, or we want a 
	 * different one.
	 */
	if (khandle->lls != ERR_XOBJ) {
	    xk_close(khandle->lls, khandle->data_transfer);
	    khandle->lls = ERR_XOBJ;
	}
	if (xk_open(khandle->host, kchan->channel, &khandle->lls) != XK_SUCCESS) {
	    xk_thread_checkout(FALSE);
	    return KKT_ERROR;  
	}
    }

    assert(khandle->lls != ERR_XOBJ);
    khandle->endpoint = *endpoint;
    khandle->is_sender = TRUE;

    xTrace1(kktp, TR_DETAILED, "send_connect for (%d) bytes",
	    request->data_length);

    assert(!khandle->request_ready);
    assert(khandle->reply_ready);
    assert(request->data_length);

    inlinep = ((ikm_plus_overhead(request->data_length) <= MAX_INLINE_SIZE) && 
	       !khandle->must_request);
    cachep = (!inlinep || request->data_length <= MAX_HOTPATH_SIZE);

    if (inlinep) {
	if (cachep) {
	    msp = kkt_grab_message(kchan);
	    if (!msp) {
		xk_thread_checkout(FALSE);
		return KKT_ERROR;
	    }
	    kkt_req_to_msg_fast(request, msp, kchan->chan_path);
	} else {
	    msp = &message;
	    kkt_req_to_msg(request, msp, kchan->chan_path);
	}
	hdr.htype = SENDCONN_NOREQUEST;
    } else {
	assert(cachep);
	msp = kkt_grab_message(kchan);
	if (!msp) { 
	    xk_thread_checkout(FALSE);
	    return KKT_ERROR;
	}
	hdr.htype = SENDCONN;
    }

    /*
     * Fill in the header
     */
    hdr.node = _node_self;
    hdr.hchan = kchan->channel;
    hdr.hsrc = (handle_t)khandle;
    hdr.hdst = (handle_t)0;
    hdr.u.sendconn.endpoint = *endpoint;
    hdr.u.sendconn.size = request->data_length;
    xkr = msgPush(msp, kkt_fillinfunc, &hdr, sizeof (hdr), (void *)0);
    assert(xkr == XK_SUCCESS);

    xTrace4(kktp, TR_DETAILED, "KKT_SEND_CONNECT --> nap <%d.%d.%d.%d>\n",
	    (unsigned) khandle->host.a, (unsigned) khandle->host.b, 
	    (unsigned) khandle->host.c, (unsigned) khandle->host.d);

    xTrace3(kktp, TR_DETAILED, "KKT_SEND_CONNECT: data is at %x, %d bytes + %d bytes (hdr)\n",
	   request->data.address, request->data_length, sizeof (hdr));

    xIfTrace(kktp, TR_DETAILED) 
	msgShow(msp);

    kstat_add(bytes_out, msgLen(msp));

    /*
     * Later: use the more argument to decide on rpc vs. dgram.
     * Also, if there is no second down protocol, always use RPC.
     */
    xkr = xk_rpc(khandle->lls, msp, khandle->reply_message, FALSE, &khandle->data_transfer);
    if (cachep)
	kkt_return_message(kchan, msp);
    else
	msgDestroy(msp);
    xk_thread_checkout(FALSE);

    kstat_add(bytes_in, msgLen(khandle->reply_message));
   
    if (xkr != XK_SUCCESS) {
	xTrace1(kktp, TR_ERRORS, "KKT_SEND_CONNECT: failure (%d)\n", xkr);
	printf("handle 0x%x: the rpc failed\n", khandle);  /* XXX */
	return KKT_ERROR;
    }
    xTrace1(kktp, TR_DETAILED, "KKT_SEND_CONNECT: reply has (%d) bytes\n",
	    msgLen(khandle->reply_message));
    /*
     * khandle->reply_message has been msgAssign-ed to contain the 
     * reply (KKT header + payload). 
     */
    if (msgPop(khandle->reply_message, kkt_pophdr, (void *)&rhdr, sizeof(rhdr), NULL) != XK_SUCCESS ) {
	xTrace0(kktp, TR_ERRORS, "KKT_SEND_CONNECT msgPop fails on reply");
	return KKT_ERROR;
    }
    /* 
     * Save what we need from the reply.
     */
    assert(rhdr.node == node);
    assert(rhdr.htype == SENDCONN || rhdr.htype == SENDCONN_NOREQUEST);
    assert(rhdr.hdst == (handle_t)khandle);
    assert(rhdr.hsrc);

    khandle->remote_kkth = rhdr.hsrc;
#if KKT_EXTRA_DEBUG
    lastkhandle = khandle;
    lastremkhandle = (kkt_handle_t)khandle->remote_kkth;
#endif /* KKT_EXTRA_DEBUG */

    rv = (struct retval *)ret;
    *rv = rhdr.u.retcode;

    /*
     * Our reply will tell DIPC whether a callback is coming.
     * (KKT_SUCCESS=callback yes; KKT_DATA_SENT=callback no).
     */
    if (inlinep)
	return KKT_DATA_SENT;
    else {
	/*
	 * a KKT_RECV from the peer will fire up the 
	 * KKT_SEND_CONNECT callback.
	 */
	KKT_REQUEST((handle_t)khandle, request);
	return KKT_SUCCESS;
    }
}


kkt_return_t
KKT_RPC(
	node_name		node,
	handle_t		handle)
{    
    kkt_handle_t khandle = (kkt_handle_t)handle;
    kkt_channel_t kchan;
    KKT_HDR rhdr;
    xkern_return_t xkr;
    boolean_t node_change = FALSE;
#if XK_DEBUG
    IPhost  host;
#endif /* XK_DEBUG */

    assert(khandle);
    
#if    !KERNEL_TEST
    assert(!is_xk_input_thread(current_thread()));
#endif /* KERNEL_TEST */

    kchan = khandle->kchan;
    assert(kchan);

    if (khandle->hstate != BUSY)
	return KKT_INVALID_HANDLE;

    if (node == NODE_NAME_LOCAL)
	node = _node_self;

    xk_thread_checkin();
    if (node != khandle->node) {
	/*
	 * Either the RPC handle was never used before, or it
	 * was used with a different target node.
	 */
	if (node_name_to_nap(node, &khandle->host) != XK_SUCCESS) {
	    xk_thread_checkout(FALSE);
	    return KKT_NODE_DOWN;
	}
	khandle->node = node;
	node_change = TRUE;
    }
    if (khandle->lls == ERR_XOBJ || node_change) {
	/*
	 * Either we don't have a lls, or we want a 
	 * different one.
	 */
	if (khandle->lls != ERR_XOBJ) {
	    xk_close(khandle->lls, khandle->data_transfer);
	    khandle->lls = ERR_XOBJ;
	}
	if (xk_open(khandle->host, kchan->channel, &khandle->lls) != XK_SUCCESS) {
	    xk_thread_checkout(FALSE);
	    return KKT_ERROR;  
	}
    }

    assert(khandle->lls != ERR_XOBJ);
    assert(khandle->request_ready);
    assert(khandle->reply_ready);

    xTrace1(kktp, TR_DETAILED, "KKT_RPC  for (%d) bytes",
	    msgLen(khandle->request_message));

    xTrace4(kktp, TR_DETAILED, "KKT_RPC --> nap <%d.%d.%d.%d>\n",
	    (unsigned) host.a, (unsigned) host.b, 
	    (unsigned) host.c, (unsigned) host.d);

    xTrace2(kktp, TR_DETAILED, "KKT_RPC: data is at %x, %d bytes\n",
	    khandle->request_data, msgLen(khandle->request_message));

    kstat_add(bytes_out, msgLen(khandle->request_message));

    xkr = xk_rpc(khandle->lls, khandle->request_message, khandle->reply_message, FALSE, &khandle->data_transfer);
    xk_thread_checkout(FALSE);

    kstat_add(bytes_in, msgLen(khandle->reply_message));

    /*
     * Both in and out messages will be destroied by the
     * KKT_RPC_HANDLE_FREE call
     */
    if (xkr != XK_SUCCESS) {
	xTrace1(kktp, TR_ERRORS, "KKT_RPC: failure (%d)\n", xkr);
	return KKT_ERROR;
    }
    xTrace1(kktp, TR_DETAILED, "KKT_RPC: reply has (%d) bytes\n",
	    msgLen(khandle->reply_message));

    if (msgPop(khandle->reply_message, kkt_pophdr, (void *)&rhdr, sizeof(rhdr), NULL) != XK_SUCCESS) {
	xTrace0(kktp, TR_ERRORS, "KKT_RPC msgPop fails on reply");
	return KKT_ERROR;
    }
    assert(rhdr.node == node);
    /* 
     * Caller does expect to see the reply bits where the
     * request bits where 
     */
    assert(msgLen(khandle->reply_message) <= msgLen(khandle->request_message));
    msg2buf(khandle->reply_message, khandle->request_data);

    return KKT_SUCCESS;
}

kkt_return_t
KKT_CONNECT_REPLY(
		  handle_t		handle,
		  kern_return_t		status,
		  kern_return_t		substatus)
{
    kkt_handle_t khandle = (kkt_handle_t)handle;

    assert(khandle);
    if (khandle->hstate != BUSY)
	return KKT_INVALID_HANDLE;

    assert(khandle->during_deliver);
    assert(!khandle->status_set);
    khandle->status = status;
    khandle->substatus = substatus;
    khandle->status_set = TRUE;
    return KKT_SUCCESS;
}

kkt_return_t
KKT_RPC_REPLY(
	      handle_t		handle)
{
    kkt_handle_t khandle = (kkt_handle_t)handle;

    assert(khandle);
    if (khandle->hstate != BUSY)
	return KKT_INVALID_HANDLE;

    assert(khandle->during_deliver);
    assert(!khandle->rpc_reply);
    khandle->rpc_reply = TRUE;
    
    return KKT_SUCCESS;
}


kkt_return_t
KKT_REQUEST(
	    handle_t			handle,
	    request_block_t		request)
{
    kkt_handle_t khandle = (kkt_handle_t)handle;

    assert(khandle);
    if (khandle->hstate != BUSY)
	return KKT_INVALID_HANDLE;
    
    /*
     * Cleanup the request
     */
    request->transport.save_address = (vm_offset_t)0;
    request->transport.save_size = 0;

    /*
     * For the moment, we just pass down.
     */
    switch(request->request_type & REQUEST_COMMANDS) {
    case REQUEST_SEND:
    case REQUEST_RECV:
#if KKT_EXTRA_DEBUG
	khandle->kkt_request_count++;
#endif /* KKT_EXTRA_DEBUG */
	return kkt_request_send(khandle, request);
    }
    xTrace1(kktp, TR_ERRORS, "KKT_REQUEST bad req_type %d\n",
	    request->request_type);
    return KKT_ERROR;
}

/*
 * Handle a request:
 *
 * Set up a Msg describing the request.
 * Send it to the other end, via async RPC client.
 */

kkt_return_t
kkt_request_send(
		 kkt_handle_t khandle,
		 request_block_t request)
{
    kkt_channel_t kchan;
    Msg_s topush;
    KKT_HDR hdr;
    xkern_return_t xkr;
    action act;
    service_type type;
    kkt_return_t   ret = KKT_SUCCESS;

    assert((request->request_type & REQUEST_COMMANDS) == REQUEST_SEND ||
	   (request->request_type & REQUEST_COMMANDS) == REQUEST_RECV);

    kchan = khandle->kchan;
    assert(kchan);

    /*
     * First, we enqueue our bit of work.
     * Second, we iterate both queues to see if 
     * something has already arrived.
     */
    if ((request->request_type & REQUEST_COMMANDS) == REQUEST_SEND) {
	Msg lmessage;

	lmessage = (Msg)pathAlloc(kchan->chan_path, sizeof(Msg_s));
	if (!lmessage) 
	    return KKT_RESOURCE_SHORTAGE;

	MARK('Q');

	kkt_req_to_msg(request, lmessage, kchan->chan_path);
	assert(request->data_length == msgLen(lmessage));
	request->transport.save_address = (vm_offset_t)lmessage;
	xk_kkt_enqueue_request(&khandle->send_req, request);
	act = kkt_operate_send(khandle, &topush);
	type = SENDDATA;
    } else { /* REQUEST_RECV */

	MARK('R');

	xk_kkt_enqueue_request(&khandle->recv_req, request);
	act = kkt_operate_recv(khandle, &topush, request);
	if (act == GO && khandle->lls == ERR_XOBJ) {
	    /*
	     * IFF we must send from the RX side and we must open a new channel
	     */
	    xk_thread_checkin();
	    if (xk_open(khandle->host, kchan->channel, &khandle->lls) != XK_SUCCESS) {
		xk_thread_checkout(FALSE);
		msgDestroy(&topush);
		return KKT_ERROR;
	    }
	    xk_thread_checkout(FALSE);
	}
	type = RECVREQ;
    }
    
    switch (act) {
    case GO:
	MARK('Y');
	xk_thread_checkin();
	if ((type == SENDDATA && msgLen(&topush)) || type == RECVREQ) {
	    xkr = kkt_push_data(khandle, &topush, &hdr, type);
	    ret = (xkr == XK_SUCCESS)? KKT_SUCCESS: KKT_ERROR;
	}
	msgDestroy(&topush);
	xk_thread_checkout(FALSE);
	break;
    case ERROR:
	ret = KKT_ERROR;
	break;
    case NOGO:
	break;
    default:
	panic("kkt internal error: op-code corrupted");
    }
    return ret;
}

xkern_return_t
kkt_push_data(
	      kkt_handle_t khandle,
	      Msg message, 
	      KKT_HDR *hdr,
	      service_type type)
{
    xkern_return_t xkr;

    /*
     * Build header.
     */
    hdr->node = _node_self;
    hdr->hchan = khandle->kchan->channel;
    hdr->hsrc = (handle_t)khandle;
    hdr->hdst =  khandle->remote_kkth;
    hdr->u.sendconn.endpoint = khandle->endpoint;
    hdr->u.sendconn.size = msgLen(message);
    hdr->htype = type;
    hdr->window = khandle->window;
    xkr = msgPush(message, kkt_fillinfunc, hdr, sizeof (KKT_HDR), (void *)0);
    assert(xkr == XK_SUCCESS);
    
    xTrace4(kktp, TR_DETAILED, "KKT_REQ(REQUEST_SEND) --> nap <%d.%d.%d.%d>\n",
	    (unsigned) khandle->host.a, (unsigned) khandle->host.b, 
	    (unsigned) khandle->host.c, (unsigned) khandle->host.d);
    
    xIfTrace(kktp, TR_DETAILED) 
	msgShow(message);

    assert(msgLen(message) <= (((PSTATE *)thisprotocol->state)->max_length + KKT_STACK_SIZE));
    kstat_add(bytes_out, msgLen(message));
   
    MARK('[');
    xkr = xk_rpc(khandle->lls, message, 0, TRUE, &khandle->data_transfer);
    MARK(']');
    /*
     * CHAN will set its own Timeout to resend the message, 
     * should a failure occur.
     */
    kstat_add(bytes_out, msgLen(message));
    return xkr;
}

/*
 * Message is uninitialized.
 */
action
kkt_operate_recv(
		 kkt_handle_t khandle,
		 Msg all_message,
		 request_block_t original_request)
{
    req_queue *send_q = &khandle->send_req;
    req_queue *recv_q = &khandle->recv_req;
    request_block_t sq, rq;
    kkt_return_t    kr;
    xkern_return_t  xkr;
    unsigned int    oldwindow;
    action ret = NOGO;
    
    oldwindow = khandle->window;
    for (;;) {
	if (!(rq = recv_q->request_list) ||  /* = rather than == */
	    !(sq = send_q->request_list))  { /* = rather than == */
	    MARK('A');
	    break;
	}
	assert(sq->transport.save_address);
	/*
	 * xk_kkt_copyout does trim the (Msg)sq->transport.save_address message
	 */
	kr = xk_kkt_copyout(khandle, rq, (Msg)sq->transport.save_address);
	if (!(msgLen((Msg)sq->transport.save_address))) {
	    MARK('S');
	    (void)xk_kkt_dequeue_request(&khandle->send_req);
	    msgDestroy((Msg)sq->transport.save_address);
	    pathFree((Msg)sq->transport.save_address);
	    pathFree(sq);
	}
	if (rq->data_length == rq->transport.save_size) {
	    (void)xk_kkt_dequeue_request(&khandle->recv_req);
	    if (original_request) {
		/*
		 * We're going to callback to the same request
		 * in the same stack frame! This is dangerous,
		 * because of 1. DIPC req's reuse 2. stack blowing.
		 * Ergo, we do it on a different stack!
		 */
		xk_kkt_enqueue_request(&khandle->callbacks, rq);
		evSchedule(khandle->retry_event, kkt_operate_callbacks, (void *)khandle, 0);
	    } else {
		/*
		 * We're an upcall. No recursion issue here.
		 * Go ahead and pop it.
		 */
		xk_kkt_callback(khandle, rq, KKT_SUCCESS);
	    }
	}
    }
    /*
     * Update window!
     */
    khandle->window = 0;
    for (rq = recv_q->request_list; rq; rq = rq->transport.next) {
	khandle->window += (rq->data_length - rq->transport.save_size);
    }
    khandle->window = MAX(MIN_WINDOW, khandle->window);
    khandle->window = MIN(khandle->window,
			  ((PSTATE *)thisprotocol->state)->max_length);
    if (original_request) {
	/*
	 * Only iff we are NOT an upcall!!
	 */
	if (!oldwindow && khandle->window && all_message) {
	    MARK('H');
	    kstat_inc(receive_restart);
	    xkr = msgConstructContig(all_message, khandle->kchan->chan_path, 0, KKT_STACK_SIZE);
	    if (xkr != XK_SUCCESS)
		ret = ERROR;
	    else 
		ret = GO;
	}
    }
    return ret;
}

/*
 * Message is uninitialized.
 */
action
kkt_operate_send(
		 kkt_handle_t khandle,
		 Msg all_message)
{
    request_block_t x_req;
    req_queue       *send_q, *recv_q;
    vm_size_t       swindow, mycount;
    xkern_return_t  xkr;
    action ret = NOGO;
    
    /*
     * Determine window
     */
    swindow = WINDOW_FUNCTION(khandle->window);
    send_q = &khandle->send_req;
    if (send_q->request_list && !khandle->data_transfer &&
	!(send_q->request_list->request_type & REQUEST_IN_PROGRESS) && swindow) {
	
	MARK('J');
	xkr = msgConstructContig(all_message, khandle->kchan->chan_path, 0, KKT_STACK_SIZE);
	if (xkr != XK_SUCCESS)
	    return ERROR;
	ret = GO;
	
	for (x_req = send_q->request_list, mycount = 0; 
	     x_req && (mycount < swindow);
	     mycount += (x_req->data_length - x_req->transport.save_size), x_req = x_req->transport.next) {
	    
	    MARK('K');
	    assert(!(x_req->request_type & REQUEST_IN_PROGRESS));
	    x_req->request_type |= REQUEST_IN_PROGRESS;
	    assert(x_req->transport.save_address);

	    if ((swindow - mycount) >= (x_req->data_length - x_req->transport.save_size)) {
		/*
		 * We swallow the entire message
		 */
		msgJoin(all_message, all_message, (Msg)x_req->transport.save_address);
	    } else {
		Msg_s lcopy;
		int len = swindow - mycount;

		msgConstructCopy(&lcopy, (Msg)x_req->transport.save_address);
		msgTruncate(&lcopy, len);
		msgJoin(all_message, all_message, &lcopy);
		msgDestroy(&lcopy);
		break;
	    }
	}
    }
    return ret;
}	    

void
kkt_restart_send(
		 Event e,
		 void *vhandle)
{
    kkt_handle_t khandle = (kkt_handle_t)vhandle;
    Msg_s lmessage;

    MARK(':');
    if (kkt_operate_send(khandle, &lmessage) == GO) {
	if (msgLen(&lmessage)) {
	    KKT_HDR hdr;

	    (void)kkt_push_data(khandle, &lmessage, &hdr, SENDDATA);
	}
	msgDestroy(&lmessage);
    }
}

void
kkt_operate_callbacks(
		      Event e,
		      void *vhandle)
{
    kkt_handle_t khandle = (kkt_handle_t)vhandle;
    request_block_t recv_req;

    MARK(';');
    xk_thread_checkout(FALSE);
    while (recv_req = xk_kkt_dequeue_request(&khandle->callbacks)) {
	xk_kkt_callback(khandle, recv_req, KKT_SUCCESS);
    }
    xk_thread_checkin();
}

void
kkt_req_to_msg(
	       request_block_t request,
	       Msg message,
	       Path path)
{
    if (request->request_type & REQUEST_SG) {
	Msg_s msg;
	vm_size_t length;
	io_scatter_t iop;
	xkern_return_t xkr;
	
	/*
	 * Build a message from the scatter/gather list.
	 *
	 * This is the simplest way I can think of to do this...
	 * We preallocate a stack whose size accounts for the sizes
	 * of the headers that we are about to prepend.
	 */
	while (msgConstructContig(message, path, 0, KKT_STACK_SIZE) != XK_SUCCESS) {
		xTrace1(kktp, TR_ERRORS, 
			"KKT Unable to msgConstructContig the request (%d) bytes\n",
			(u_int)iop->ios_length);
		Delay(KKT_ALLOC_DELAY); 
	}
	for (length = request->data_length, iop = &request->data.list[0];
	     length > 0;
	     length -= iop->ios_length, iop++) {
	    while (msgConstructInPlace(&msg, path, iop->ios_length,
				       0, (char *)iop->ios_address, kkt_freefunc, (void *)0) != XK_SUCCESS) {
		xTrace1(kktp, TR_ERRORS, 
			"KKT Unable to construct in place the request (%d) bytes\n",
			(u_int)iop->ios_length);
		Delay(KKT_ALLOC_DELAY); 
	    }
	    msgJoin(message, message, &msg);
	    msgDestroy(&msg);
	}
	assert(length == 0);
	assert(msgLen(message) == request->data_length);
    } else {
	/*
	 * Construct in place the x-kernel message.
	 * Note how persistent I am.
	 */
	while (msgConstructInPlace(message, path, request->data_length,
				   0, (char *)request->data.address, kkt_freefunc, (void *)0) != XK_SUCCESS) {
	    xTrace1(kktp, TR_ERRORS, 
		    "KKT Unable to construct in place the request (%d) bytes\n",
		    (u_int)request->data_length);
	    Delay(KKT_ALLOC_DELAY);
	}
    }
}

void
kkt_req_to_msg_fast(
	       request_block_t request,
	       Msg message,
	       Path path)
{
    xkern_return_t xkr;
    char *where;
    
    assert(request->data_length <= MAX_HOTPATH_SIZE);
    
    if (request->request_type & REQUEST_SG) {
	io_scatter_t iop = &request->data.list[0];
		
	where = (char *)iop->ios_address;
    } else {
	where = (char *)request->data.address;
	
    }
    xkr = msgPush(message, kkt_fillinfunc, where,
		  request->data_length, (void *)0);
    assert (xkr == XK_SUCCESS);
}

/*
 * Called from msgForEach().
 * Skip req->transport.save_size into user's buffer, then copy.
 */

boolean_t
xk_kkt_copy_some(
		 char *ptr, 
		 long len, 
		 void *arg) 
{
    request_block_t req = (request_block_t)arg;
    int offset = req->transport.save_size;
    kkt_sglist_t lp = req->data.list; /* ios_address, ios_length */
    char *to;
    int thislen;
    
    assert(offset < req->data_length);
    xTrace3(kktp, TR_EVENTS, "copy_some(%x, %x, %x)\n", ptr, len, arg);
    
    /*
     * Skip over data already copied.
     * There must be a better way.
     * Can I scribble over the sglist?  Probably not.
     */
    
    while (offset >= lp->ios_length) {
	offset -= lp->ios_length;
	lp++;
    }
    
    to = (char *)lp->ios_address;
    thislen = lp->ios_length;
    
    if (offset > 0) {
	to += offset;
	thislen -= offset;
    }
    /*
     * From here, offset counts the bytes copied in this call.
     */
    while (len > 0) {
	if (thislen >= len) {
	    /*
	     * This buffer extent consumes the Msg extent.
	     * One last bcopy is needed.
	     */
	    xTrace3(kktp, TR_DETAILED, "copy_some: bcopy(%x, %x, %x)\n",
		    ptr, to, len);
	    bcopy(ptr, to, len);
	    req->transport.save_size += len;
	    break;
	}
	assert(len > thislen);
	/*
	 * This buffer extent doesn't consume the Msg extent.
	 * Copy and advance to the next buffer.
	 */
	xTrace3(kktp, TR_DETAILED, "copy_some: bcopy(%x, %x, %x)\n",
		ptr, to, thislen);
	bcopy(ptr, to, thislen);
	req->transport.save_size += thislen;
	ptr += thislen;
	len -= thislen;
	if (req->transport.save_size < req->data_length) {
	    /*
	     * Next entry must be valid. Operate on it!
	     */
	    lp++;
	    assert(lp->ios_address);
	    assert(lp->ios_length);
	    thislen = lp->ios_length;
	    to = (char *)lp->ios_address;
	} else {
	    /*
	     * We've consumed the request but there is
	     * still some data in the x-kernel message.
	     * Stop the iteration.
	     */
	    return FALSE;
	}
    }
    assert(req->transport.save_size <= req->data_length);
    return TRUE;
}

/*
 * Given a request_block and a Msg,
 * copy the data from the Msg into the buffer(s)
 * described by the request_block.
 */
kkt_return_t
xk_kkt_copyout(
	       kkt_handle_t handle, 
	       request_block_t request, 
	       Msg message) 
{
    Msg_s lcopy, *lptr;
    int tocopy = msgLen(message);
    int avail = (request->data_length - request->transport.save_size);

    MARK('X');
    if (tocopy > avail) {
	msgConstructCopy(&lcopy, message);
	msgTruncate(&lcopy, avail);
	lptr = &lcopy;
	tocopy = avail;
    } else {
	/*
	 * The message will be consumed entirely
	 */
	lptr = message;
    }
    
    kstat_add(bytes_in, MIN(tocopy, avail));

    if (request->request_type & REQUEST_SG) {
	xTrace0(kktp, TR_EVENTS, "xk_kkt_copyout got REQUEST_SG\n");
	msgForEach(lptr, xk_kkt_copy_some, (void *)request);
    } else {
	msg2buf(lptr, (char *)(request->data.address + request->transport.save_size));
	request->transport.save_size += tocopy;
    }
    if (lptr == &lcopy)
	msgDestroy(lptr);
    msgPopDiscard(message, tocopy);

    return KKT_SUCCESS;
}

void
xk_kkt_enqueue_request(
		       req_queue *q,
		       request_block_t request) 
{
    assert(request);

    request->transport.next = NULL;

    if (q->request_end)
	q->request_end->transport.next = request;
    else
	q->request_list = request;
    q->request_end = request;
}

request_block_t
xk_kkt_dequeue_request(
		       req_queue *q)
{
    request_block_t req;

    if ((req = q->request_list) == NULL) {
	MARK('O');
	return NULL;
    }
    if ((q->request_list = req->transport.next) == NULL)
	q->request_end = NULL;
    MARK('D');
    req->transport.next = NULL;
    return req;
}

/*
 * Abort the entire queue.
 */
void
xk_kkt_abort(
	     kkt_handle_t khandle)
{
    request_block_t req;

    if (khandle->is_sender) {
	while (req = xk_kkt_dequeue_request(&khandle->send_req)) {
	    xk_kkt_callback(khandle, req, KKT_ABORT);
	} 
    } else {
	while (req = xk_kkt_dequeue_request(&khandle->send_req)) {
	    (void)xk_kkt_dequeue_request(&khandle->send_req);
	    msgDestroy((Msg)req->transport.save_address);
	    pathFree((Msg)req->transport.save_address);
	    pathFree(req);
	}	    
	while (req = xk_kkt_dequeue_request(&khandle->recv_req)) {
	    xk_kkt_callback(khandle, req, KKT_ABORT);
	} 
    }	
    while ((req = xk_kkt_dequeue_request(&khandle->callbacks))) {
	xk_kkt_callback(khandle, req, KKT_ABORT);
    }
}

/*
 * Terminate a single request.
 *
 * How does DIPC reclaim successful requests that don't have callbacks?
 * By the next request finishing?
 */

void
xk_kkt_callback(
		kkt_handle_t khandle,
		request_block_t req,
		kkt_return_t kr)
{
    MARK('F');
#if KKT_EXTRA_DEBUG
    khandle->kkt_callbacks_count++;
#endif /* KKT_EXTRA_DEBUG */
    /*
     * Clean out the req.
     */
    if (req->transport.save_address != (vm_offset_t)NULL) {
	msgDestroy((Msg)req->transport.save_address);
	pathFree((void *)req->transport.save_address);
	req->transport.save_address = (vm_offset_t)NULL;
	req->transport.save_size = 0;
    }
    req->request_type &= ~(REQUEST_IN_PROGRESS);
    
    if (req->callback == NULL) {
	if (kr != KKT_SUCCESS) {
	    /*
	     * No req callback, use handle's error callback instead.
	     */
	    assert(khandle->error);
	    xTrace3(kktp, TR_EVENTS, "khandle->error(%d, %x, %x)\n",
		    kr, khandle, req);
	    (*khandle->error)(kr, (handle_t)khandle, req);
	}
    } else {
	/*
	 * Normal callback.
	 */
	xTrace3(kktp, TR_EVENTS, "req->callback(%d, %x, %x)\n",
		kr, khandle, req);
	(*req->callback)(kr, (handle_t)khandle, req, TRUE);
    }
}

/*
 * Verify that the handle belongs to the channel.
 *
 * Run down the channel's handles_list.
 */
kkt_handle_t
xk_kkt_handle_check(
		kkt_channel_t kchan,
		kkt_handle_t khandle)
{
    xTrace0(kktp, TR_EVENTS, "xk_kkt_handle_check stubbed\n");
    return khandle;
}

/* 
 * Transport dependent data copy.
 */
kkt_return_t
KKT_COPYOUT_RECV_BUFFER(
			handle_t        handle,
			char            *dest)
{
    kkt_handle_t    khandle = (kkt_handle_t)handle;
    Msg message;

    assert(khandle);
    if (khandle->hstate != BUSY)
	return KKT_INVALID_HANDLE;

    assert(khandle->request_message && khandle->during_deliver);
    
    message = khandle->request_message;
    msgPopDiscard(message, sizeof (KKT_HDR));
    msg2buf(message, dest);

    return KKT_SUCCESS;
}

void
kkt_client_upcall(
		  Msg          reply)
{
    kkt_channel_t kchan;
    kkt_handle_t  khandle;
    kkt_return_t  kr;
    xkern_return_t xkr;
    KKT_HDR hdr;
    struct retval ret;
    thread_t thread = current_thread();
    
    assert(is_xk_input_thread(thread));

    xIfTrace(kktp, TR_DETAILED) 
	msgShow(reply);

    assert(msgLen(reply) >= sizeof(KKT_HDR));

    xkr = msgPop(reply, kkt_pophdr, (void *)&hdr, sizeof(hdr), NULL);
    assert(xkr == XK_SUCCESS);

    xTrace3(kktp, TR_DETAILED, "KKT_CLIENT__UPCALL chan %d type %d (handle_t)src x%x\n",
	    hdr.hchan, hdr.htype, hdr.hsrc);
    /*
     * A few consistency checks
     */
    assert(KKT_NODE_IS_VALID(hdr.node));
    assert(valid_channel(hdr.hchan));
    assert(kkt_channels[hdr.hchan] != KKT_CHANNEL_NULL);
    
    assert(hdr.htype == SENDDATA_REPLY ||
	   hdr.htype == RECVREQ_REPLY ||
	   hdr.htype == ABORT_REPLY);

    kchan = kkt_channels[hdr.hchan];
    khandle = (kkt_handle_t)hdr.hdst;
    assert(khandle->data_transfer == TRUE);
    khandle->data_transfer = FALSE;

    switch (hdr.htype) {
    case SENDDATA_REPLY:
	{
	    request_block_t sq;
	    int howmany;

	    MARK('Z');
	    howmany = hdr.u.bytes_copied;
	    /*
	     * Update the window information.
	     */
	    khandle->window = hdr.window;
	    if (!howmany) {
		/*
		 * The receiver cut us off!
		 * Nothing has been done with data we sent.
		 */
		MARK('C');
		while ((sq = khandle->send_req.request_list) && 
		       (sq->request_type & REQUEST_IN_PROGRESS)) {
		    sq->request_type &= ~REQUEST_IN_PROGRESS;
		}
		/*
		 * The send queue will be restarted by the receiver node
		 * or by an additional REQUEST_SEND.
		 */
		break;
	    }
	    while ((sq = khandle->send_req.request_list) && howmany > 0) { 
		unsigned int delta = MIN(howmany, sq->data_length - sq->transport.save_size);

		MARK('V');
		assert(sq->request_type & REQUEST_IN_PROGRESS);
		sq->transport.save_size += delta;
		if (sq->data_length == sq->transport.save_size) {
		    (void)xk_kkt_dequeue_request(&khandle->send_req);
		    xk_kkt_enqueue_request(&khandle->callbacks, sq);
		} else {
		    msgPopDiscard((Msg)sq->transport.save_address, howmany);
		    MARK('B');
		}
		/*
		 * If sq has still some data, we'll send it again!!
		 */
		howmany -= delta;
		sq->request_type &= ~REQUEST_IN_PROGRESS;
	    }
	    assert(khandle->send_req.request_list == (request_block_t)0 ||
		   !(khandle->send_req.request_list->request_type & REQUEST_IN_PROGRESS));

	    xk_thread_checkout(FALSE);
	    while (sq = xk_kkt_dequeue_request(&khandle->callbacks)) { /* = rather than == */ 
		/*
		 * The first KKT_REQUEST will send some more data
		 * (unless the window is 0).
		 */
		xk_kkt_callback(khandle, sq, KKT_SUCCESS);
	    }
	    assert(!khandle->callbacks.request_list); 
	    assert(!khandle->callbacks.request_end);
	    
	    xk_thread_checkin();
	    kkt_restart_send((Event)0, (void *)khandle);
	}
	break;
    case RECVREQ_REPLY:
	{
	    /*
	     * Nothing to do here
	     */
	    MARK('N');
	}
	break;
    case ABORT_REPLY:
	{
	    /*
	     * Nothing to do here
	     */
	    MARK('M');
	}
	break;
    default:
	panic("kkt internal error: type of message does not match");
    }
    /*
     * This is the end of the transaction with the other node.
     */
}

void
kkt_server_upcall(
		  XObj         lls,
		  Msg          request,
		  Msg          reply)
{
    kkt_channel_t kchan;
    kkt_handle_t  khandle;
    kkt_return_t  kr;
    xkern_return_t xkr;
    KKT_HDR *hdr;
    struct retval ret;
    thread_t thread = current_thread();

    xIfTrace(kktp, TR_DETAILED) 
	msgShow(request);

    assert(msgLen(request) >= sizeof(KKT_HDR));
    msgForEach(request, msgDataPtr, (void *)&hdr);


    xTrace3(kktp, TR_DETAILED, "KKT_SERVER_UPCALL chan %d type %d (handle_t)src x%x\n",
	    hdr->hchan, hdr->htype, hdr->hsrc);
    /*
     * A few consistency checks
     */
    assert(KKT_NODE_IS_VALID(hdr->node));
    assert(valid_channel(hdr->hchan));
    assert(kkt_channels[hdr->hchan] != KKT_CHANNEL_NULL);
    assert(hdr->htype == SENDCONN || 
	   hdr->htype == SENDCONN_NOREQUEST ||
	   hdr->htype == RPC ||
	   hdr->htype == SENDDATA || 
	   hdr->htype == RECVREQ ||
	   hdr->htype == ABORT);

    kchan = kkt_channels[hdr->hchan];

    assert(is_xk_input_thread(thread));

    switch (hdr->htype) {
    case SENDCONN:  
    case SENDCONN_NOREQUEST:
	{
	    KKT_HDR    reply_hdr;
	    IPhost     peer_host;
	    node_name  peer_name;

	    peer_name = (hdr->node != NODE_NAME_LOCAL)? hdr->node : _node_self;
	    /* 
	     * Do we know the <node name, nap> of the node we're talking to?!
	     */
	    xkr = node_name_to_nap(peer_name, &peer_host);
	    if (xkr != XK_SUCCESS) {
		xTrace1(kktp, TR_DETAILED, "Can't figure out who node %d is\n", peer_name);
		break; 
	    }

	    assert(kchan->chan_type == SENDCONN);
	    xk_thread_checkout(FALSE); 
	    /*
	     * OK, we passed. Let's get an handle, now.
	     */
	    while (kkt_handle_alloc_common(hdr->hchan, 
					   (handle_t *) &khandle, 
					   (kkt_error_t) 0, 
					   TRUE,                     /* wait */
					   (hdr->htype == SENDCONN), /* must_request */
					   0, 
					   TRUE) != KKT_SUCCESS) {
		xTrace0(kktp, TR_DETAILED, "KKT_DISPATCH can't get an handle. Retry\n");
		Delay(KKT_ALLOC_DELAY); 
	    }
	    assert(khandle);
	    khandle->node = peer_name;
	    khandle->host = peer_host;

	    assert(hdr->hsrc);
	    /* 
	     * Before sending, I save the contents of KKT_HDR for the reply.
	     * I must update hsrc and hdst, though.
	     */
	    reply_hdr = *hdr;
	    reply_hdr.node = _node_self;
	    reply_hdr.hsrc = (handle_t)khandle;
	    reply_hdr.hdst = hdr->hsrc;

	    khandle->remote_kkth = hdr->hsrc;
	    khandle->request_message = request;
	    assert(!khandle->during_deliver);
	    khandle->during_deliver = TRUE;
	    assert(!khandle->status_set);
	    (* kchan->input.deliver)(hdr->hchan,
				     (handle_t)khandle,
				     &hdr->u.sendconn.endpoint,
				     hdr->u.sendconn.size,
				     (hdr->htype == SENDCONN_NOREQUEST),
				     TRUE);
	    assert(khandle->status_set);
	    reply_hdr.u.retcode.status = khandle->status;
	    reply_hdr.u.retcode.substatus = khandle->substatus;
	    if (khandle->freehandle) {
		kkt_handle_free_common((handle_t)khandle);
	    } else {
		khandle->status_set = FALSE;
		khandle->during_deliver = FALSE;
		khandle->request_message = (Msg)0;
	    }
	    xk_thread_checkin();
	    xkr = msgPush(reply, kkt_fillinfunc, &reply_hdr, sizeof (reply_hdr), (void *)0);
	    assert(xkr == XK_SUCCESS);
	    break;
	}
    case ABORT: 
	{
	    KKT_HDR reply_hdr;
	    
	    xk_thread_checkout(FALSE); 
	    khandle = xk_kkt_handle_check(kchan, (kkt_handle_t)hdr->hdst);
	    /*
	     * Before aborting, make sure that the handle 
	     * is still belonging to the transaction we
             * want to abort. It has been observed that
             * a late ABORT message could unduly abort
             * a handle which was already recycled for 
	     * a new transaction (Our LIFO management of
	     * handle makes things worst!).
	     * 
	     * Thus, we check that the <local handle, 
             * remote handle, node> tuple match. Given that
	     * our peer node is now blocked in a RPC with
	     * remote handle busy, it is impossible for 
             * this tuple to describe a new transaction.
	     */
	    if (khandle && khandle->hstate == BUSY &&
		khandle->remote_kkth == hdr->hsrc  &&
		khandle->node == hdr->node) {
		    khandle->hstate = ABORTED;
		    khandle->abort_reason = hdr->u.abort_reason;
		    xk_kkt_abort(khandle);
	    }

	    reply_hdr = *hdr;
	    reply_hdr.node = _node_self;
	    reply_hdr.hsrc = (handle_t)khandle;
	    reply_hdr.hdst = hdr->hsrc;
	    reply_hdr.htype = ABORT_REPLY;
	    reply_hdr.u.retcode.status = KKT_SUCCESS;

	    xk_thread_checkin();
	    xkr = msgPush(reply, kkt_fillinfunc, &reply_hdr, sizeof (reply_hdr), (void *)0);
	    assert(xkr == XK_SUCCESS);
	    break;
	}
    case SENDDATA: 
	{
	    /*
	     *    1. There are REQUEST_RECVs whose size does match 
	     *       the size of the REQUEST_SEND. Cool.
	     *    2. There are REQUEST_RECVs whose size is smaller
	     *       than the size of the REQUEST_SEND.
	     */
	    request_block_t recv_req, lcopy = (request_block_t)0;
	    KKT_HDR hdr_s, reply_hdr;
	    Msg lmessage; 
	    int howmany, reswindow;
	    
	    xk_thread_checkout(FALSE); 
	    assert(kchan->chan_type == SENDCONN);
	    khandle = xk_kkt_handle_check(kchan, (kkt_handle_t)hdr->hdst);
	    /*
	     * We're connected ?!
	     */
	    assert(khandle->node != NODE_NAME_NULL);
	    assert(khandle->remote_kkth == hdr->hsrc);
	    assert(khandle);
	
	    xkr = msgPop(request, kkt_pophdr, (void *)&hdr_s, sizeof(hdr_s), NULL);
	    assert(xkr == XK_SUCCESS);

	    reply_hdr = hdr_s;
	    reply_hdr.node = _node_self;
	    reply_hdr.hsrc = (handle_t)khandle;
	    reply_hdr.hdst = hdr_s.hsrc;
	    reply_hdr.htype = SENDDATA_REPLY;

	    MARK('1');
	    lcopy = (request_block_t)pathAlloc(kchan->chan_path, sizeof(*lcopy));
	    lmessage = (Msg)pathAlloc(kchan->chan_path, sizeof(Msg_s));

	    if (lcopy && lmessage) {
		vm_size_t bytes_used = msgLen(request);

		MARK('2');
		msgConstructCopy(lmessage, request);
		lcopy->data_length = bytes_used;
		lcopy->request_type = REQUEST_SEND;
		lcopy->transport.save_address = (vm_offset_t)lmessage;
		lcopy->transport.save_size = 0;
		xk_kkt_enqueue_request(&khandle->send_req, lcopy);
		/*
		 * kkt_operate_recv will update the window.
		 * We don't care about its return value, as
		 * we piggyback the window information in
		 * any case.
		 */
		reply_hdr.u.bytes_copied = bytes_used;
		(void)kkt_operate_recv(khandle, (Msg)0, (request_block_t)0);
		reply_hdr.window = khandle->window;
	    } else {
		if (lcopy)
		    pathFree(lcopy);
		if (lmessage)
		    pathFree(lmessage);
		reply_hdr.u.bytes_copied = 0;
		MARK('3');
	    }

	    xk_thread_checkin();
	    xkr = msgPush(reply, kkt_fillinfunc, &reply_hdr, sizeof (reply_hdr), (void *)0);
	    assert(xkr == XK_SUCCESS);
	    break;
	}
    case RECVREQ:
	{
	    KKT_HDR hdr_s, reply_hdr;
	    
	    assert(kchan->chan_type == SENDCONN);
	    khandle = xk_kkt_handle_check(kchan, (kkt_handle_t)hdr->hdst);
	    /*
	     * We're connected ?!
	     */
	    assert(khandle->node != NODE_NAME_NULL);
	    assert(khandle->remote_kkth == hdr->hsrc);
	    assert(khandle);	
	
	    xkr = msgPop(request, kkt_pophdr, (void *)&hdr_s, sizeof(hdr_s), NULL);
	    assert(xkr == XK_SUCCESS);

	    /*
	     * We are notified that the window is not 0 any longer!
	     */
	    MARK('4');
	    khandle->window = hdr_s.window;
	    assert(hdr_s.window);
	    kstat_inc(receive_restart);

	    reply_hdr = hdr_s;
	    reply_hdr.node = _node_self;
	    reply_hdr.hsrc = (handle_t)khandle;
	    reply_hdr.hdst = hdr_s.hsrc;
	    reply_hdr.htype = RECVREQ_REPLY;

	    evSchedule(khandle->retry_event, kkt_restart_send, (void *)khandle, 0);	    
	    xkr = msgPush(reply, kkt_fillinfunc, &reply_hdr, sizeof (reply_hdr), (void *)0);
	    assert(xkr == XK_SUCCESS);
	    break;
	}
    case RPC:
	{
	    rpc_buf_t  rp;

	    xk_thread_checkout(FALSE); 
	    assert(kchan->chan_type == RPC);
	    /*
	     * OK, we passed. Let's get an handle, now.
	     */
	    while (kkt_handle_alloc_common(hdr->hchan, 
					   (handle_t *) &khandle, 
					   (kkt_error_t) 0, 
					   TRUE,                     /* wait */
					   (hdr->htype == SENDCONN), /* must_request */
					   0, 
					   FALSE) != KKT_SUCCESS) {
		xTrace0(kktp, TR_DETAILED, "KKT_DISPATCH can't get an handle. Retry\n");
		Delay(KKT_ALLOC_DELAY); 
	    }
	    assert(khandle);
	    khandle->node = hdr->node;
	    /* 
	     * Loopback clause
	     */
	    if (khandle->node == NODE_NAME_LOCAL)
		khandle->node = _node_self;

	    assert(hdr->hsrc);
	    khandle->remote_kkth = hdr->hsrc;

	    rp = (rpc_buf_t)&hdr->u.rpc.ulink;
	    rp->handle = (handle_t)khandle;
	    khandle->during_deliver = TRUE;
	    assert(!khandle->rpc_reply);
	    (* kchan->input.rpc_deliver)(hdr->hchan,
					 rp,
					 TRUE);
	    /* 
	     * We must deallocate the handle, now.
	     */
	    assert(khandle->rpc_reply);
	    kr = kkt_handle_free_common((handle_t)khandle);
	    assert(kr == KKT_SUCCESS);
	    /* 
	     * Prepare the reply. Its bits must be the
	     * same as the request's bits (they're opaque
	     * to the transport), with the sole exception
	     * of the KKT_HDR's bits.
	     */
	    xk_thread_checkin();
	    hdr->node = _node_self;
	    hdr->hdst = hdr->hsrc;
	    hdr->hsrc = (handle_t)khandle;
	    msgAssign(reply, request);
	    break;
	}
    }
    /* 
     * Returning means: send this reply for me, pls.
     */
}

#if DONT_USE_IPHOST

typedef unsigned int kkt_map_t;
#define MAP_SIZE sizeof(kkt_map_t)
/* TEMPORARY HACK !!! */
#define MAX_NODE_FOR_MAP 31

kkt_return_t
KKT_MAP_ALLOC(node_map_t	*map,
	      boolean_t		full)
{
    kkt_map_t *cmap;

    *map = (node_map_t)pathAlloc(thisprotocol->path, MAP_SIZE);
    if (!*map)
	return KKT_RESOURCE_SHORTAGE;
    cmap = (kkt_map_t *)(*map);
    *cmap = 0;
    return (KKT_SUCCESS);
}

kkt_return_t
KKT_MAP_FREE(node_map_t		map)
{
    pathFree((void *)map);
    return (KKT_SUCCESS);
}

kkt_return_t
KKT_ADD_NODE(node_map_t		map,
	     node_name		node)
{
    kkt_map_t *cmap = (kkt_map_t *)map;

    assert(KKT_NODE_IS_VALID(node));
    if (*cmap & 1<<node) {
	xTrace2(kktp, TR_DETAILED, "node_name %d is already in map x%x\n",
		node, map);
	return KKT_NODE_PRESENT;
    }
    *cmap |= 1<<node;
    xTrace2(kktp, TR_DETAILED, "node_name %d has been inserted in map x%x\n",
	    node, map);
    return (KKT_SUCCESS);
}

kkt_return_t
KKT_REMOVE_NODE(node_map_t		map,
		node_name		*node)
{
    kkt_map_t *cmap = (kkt_map_t *)map;

    if (*cmap == 0) {
	xTrace1(kktp, TR_DETAILED, "the map x%x is empty\n", map);
	return KKT_MAP_EMPTY;
    } 
    *node = ffs(*cmap)-1;
    *cmap &= ~(1<<*node);
    xTrace2(kktp, TR_DETAILED, "node_name %d has been removed from map x%x\n",
	    node, map);
    if (*cmap == 0) 
	return KKT_RELEASE;
    return KKT_SUCCESS;
}

boolean_t
KKT_NODE_IS_VALID(
		  node_name node)
{
	if (node >= 0 && node <= MAX_NODE_FOR_MAP)
	    return TRUE;
	else
	    return FALSE;
}

#else  /* DONT_USE_IPHOST */

#define MAX_NODE_FOR_MAP 32

typedef struct {
    short prod;
    short cons;
    IPhost nodes[MAX_NODE_FOR_MAP];
} kkt_map_data, *kkt_map;

kkt_return_t
KKT_MAP_ALLOC(node_map_t	*map,
	      boolean_t		full)
{
    kkt_map lmap;

    lmap = (kkt_map)pathAlloc(thisprotocol->path, sizeof(kkt_map_data));
    if (!lmap)
	return KKT_RESOURCE_SHORTAGE;
    lmap->prod = lmap->cons = 0;
    *map = (node_map_t)lmap;

    return KKT_SUCCESS;
}

kkt_return_t
KKT_MAP_FREE(node_map_t		map)
{
    pathFree((void *)map);
    return KKT_SUCCESS;
}

kkt_return_t
KKT_ADD_NODE(node_map_t		map,
	     node_name		node)
{
    kkt_map lmap = (kkt_map)map;
    IPhost  *host = (IPhost *)&node;
    int i;

    if ((lmap->prod - lmap->cons) == MAX_NODE_FOR_MAP) {
	xTrace1(kktp, TR_DETAILED, "map x%x is full\n", lmap);
	return KKT_RESOURCE_SHORTAGE;
    }
    /* shouldn't this be an assert ?! */
    for (i = lmap->cons; i < lmap->prod; i++)
	if (IP_EQUAL(lmap->nodes[i%MAX_NODE_FOR_MAP], *host)) {
	    xTrace2(kktp, TR_DETAILED, "node_name %x is already in map x%x\n",
		    *host, lmap);
	    return KKT_NODE_PRESENT;
	}
    lmap->nodes[lmap->prod++%MAX_NODE_FOR_MAP] = *host;

    xTrace2(kktp, TR_DETAILED, "node_name %x has been inserted in map x%x\n",
	    *host, lmap);
    return KKT_SUCCESS;
}

kkt_return_t
KKT_REMOVE_NODE(node_map_t		map,
		node_name		*node)
{
    kkt_map lmap = (kkt_map)map;

    if (lmap->prod == lmap->cons) {
	xTrace1(kktp, TR_DETAILED, "the map x%x is empty\n", lmap);
	return KKT_MAP_EMPTY;
    } 
    xTrace2(kktp, TR_DETAILED, "node_name %d has been removed from map x%x\n",
	    node, lmap);

    *(IPhost *)node = lmap->nodes[lmap->cons++%MAX_NODE_FOR_MAP];
    if (lmap->prod == lmap->cons) 
	return KKT_RELEASE;

    return KKT_SUCCESS;
}

boolean_t       
KKT_NODE_IS_VALID(
		  node_name node)
{
    return TRUE;
}


#endif /* DONT_USE_IPHOST */

#if     KERNEL_TEST

node_name test_node;
node_name msg_node;

/* 
 * I must hold transport_test_setup() 'til all the 
 * x-kernel stack is built and running.
 */
void
transport_test_setup(void)
{
    while (thisprotocol == ERR_XOBJ) {
	/* 
	 * Synchronize with the thread which builds up
	 * the x-kernel protocol graph.
	 */
	assert_wait((event_t) &thisprotocol, FALSE);
	thread_block((void (*)(void)) 0);
    }
    assert(KKT_NODE_SELF() != NODE_NAME_NULL);
}

void
transport_send_test(
		    vm_size_t size,
		    vm_offset_t data,
		    node_name node)
{
}

void
transport_recv_test(
		    handle_t handle,
		    vm_size_t size,
		    unsigned int *ulink,
		    vm_offset_t data)
{
}
#endif  /*KERNEL_TEST*/

#if	MACH_KDB

#include <ddb/db_output.h>

void 
db_show_kkt(
	    void *arg)
{
    int i;

    kstat_print(channel_init);
    kstat_print(send_connect);
    kstat_print(handle_alloc);
    kstat_print(handle_inuse);
    kstat_print(handle_free);
    kstat_print(no_buffer);
    kstat_print(send);
    kstat_print(receive);
    kstat_print(send_block);
    kstat_print(receive_restart);

    for(i=0;i<MAX_CHANNEL;i++)
	db_show_kkt_channel(i);
}

void 
db_show_kkt_msg(
		void *arg)
{
}

void 
db_show_kkt_handle(
		   kkt_handle_t khandle)
{
    request_block_t req;
    int i;

    indent += 2;
    iprintf("KKT: kkt_handle_t 0x%x\n", khandle);
    indent += 2;
    iprintf("hstate 0x%x, remote_kkth 0x%x, kchan 0x%x, abort_reason 0x%x, lls 0x%x\n",
	    khandle->hstate, khandle->remote_kkth, khandle->kchan, 
	    khandle->abort_reason, khandle->lls);
#if KKT_STATS
    iprintf("last_lls (rx only) 0x%x\n", khandle->last_lls);
#endif /* KKT_STATS */
    iprintf("window 0x%x, is_sender %d, request_ready %d, reply_ready %d, must_request %d\n",
	    khandle->window, khandle->is_sender, khandle->request_ready, 
	    khandle->reply_ready, khandle->must_request);
    iprintf("status_set %d, during deliver %d, freehandle %d, rpc_reply %d, data_transfer %d\n",
	    khandle->status_set, khandle->during_deliver, khandle->freehandle,
	    khandle->rpc_reply, khandle->data_transfer);
    iprintf("status 0x%x substatus 0x%x\n", khandle->status, khandle->substatus);
    iprintf("request_message 0x%x reply_message 0x%x request_data 0x%x\n", 
	    khandle->request_message, khandle->reply_message, khandle->request_data);
    if (khandle->send_req.request_list) {
	indent += 2;
	iprintf("Send Req list:\n");
	for (req = khandle->send_req.request_list; req; req = req->transport.next)
	    db_printf(" 0x%x", req);
	db_printf("\n");
	indent -= 2;
    }
    if (khandle->recv_req.request_list) {
	indent += 2;
	iprintf("Recv Req list:\n");
	for (req = khandle->recv_req.request_list; req; req = req->transport.next)
	    db_printf(" 0x%x", req);
	db_printf("\n");
	indent -= 2;
    }
    if (khandle->callbacks.request_list) {
	indent += 2;
	iprintf("Callbacks list:\n");
	for (req = khandle->callbacks.request_list; req; req = req->transport.next)
	    db_printf(" 0x%x", req);
	db_printf("\n");
	indent -= 2;
    }
    indent -= 4;
}

void 
db_show_kkt_event(
		  void *arg)
{
}

void 
db_show_kkt_request(
		    request_block_t req)
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
	indent -=2;
}

void 
db_show_kkt_node(
		 void *arg)
{
}

void 
db_show_kkt_channel(
		    channel_t	channel)
{
	kkt_channel_t kktc = (kkt_channel_t)channel;
	int i;
	kkt_handle_t kkth;
	MsgK mptr;

	if (valid_channel(channel))
	    kktc = kkt_channels[channel];
	if (kktc == KKT_CHANNEL_NULL)
	    return;
	for(i=0;i<MAX_CHANNEL;i++)
	    if (kktc == kkt_channels[i])
		break;
	if (i == MAX_CHANNEL)
	    return;
	iprintf("KKT: kkt_channel_t 0x%x\n", kktc);
	indent += 2;
	iprintf("channel 0x%x chan_type %d senders %d receivers %d\n",
		kktc->channel, kktc->chan_type, kktc->senders, kktc->receivers);
	iprintf("deliver 0x%x rpc_deliver 0x%x malloc 0x%x free 0x%x\n",
		kktc->input.deliver, kktc->input.rpc_deliver, kktc->malloc, kktc->free);
	iprintf("waiters %d max_send_size 0x%x\n",
		kktc->waiters, kktc->max_send_size);

	iprintf("Active Handles:\n");
	indent += 2;
	queue_iterate(&kktc->busy_list, kkth, kkt_handle_t, link) {
		db_show_kkt_handle(kkth);
	}
	indent -= 2;
	iprintf("Channel's message cache:\n");
	indent += 2;
	queue_iterate(&kktc->message_cache, mptr, MsgK, link) {
		iprintf("message entry 0x%x\n", mptr);
	}
	indent -=4;
}

void 
db_show_kkt_node_map(
		     node_map_t map)
{
}

#endif	/* MACH_KDB */
