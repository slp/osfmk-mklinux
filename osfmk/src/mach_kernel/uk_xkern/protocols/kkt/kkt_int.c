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
#include <xkern/include/prot/chan.h>
#include <kern/ipc_sched.h>

kkt_channel_t kkt_channels[MAX_CHANNEL];
decl_simple_lock_data( , kkt_channel_lock_data )

kkt_return_t
kkt_channel_init_common(
		 channel_t	channel,
		 unsigned int	senders,
		 unsigned int	receivers,
		 kkt_channel_deliver_t   sendconn,
		 kkt_rpc_deliver_t       rpc,
		 kkt_malloc_t            kkt_alloc,
		 kkt_free_t       	 kkt_free,
		 unsigned int		 max_send_size)
{
    kkt_channel_t kchan;
    kkt_handle_t  khandle, kh;
    unsigned int max_handles;
    int i;

    if (!valid_channel(channel) ||
	senders <= 0 ||
	receivers <= 0 ||
	!(sendconn || rpc) ||
	!kkt_alloc ||
	!kkt_free)
	    return (KKT_INVALID_ARGUMENT);

    if (kkt_channels[channel] != KKT_CHANNEL_NULL)
	return (KKT_ERROR);

    /*
     * I don't use the x-kernel allocators for this kind
     * of memory
     */
    kchan = (kkt_channel_t)(*kkt_alloc)(channel, sizeof (struct kkt_channel));
    assert(kchan);

    kchan->channel = channel;
    kchan->senders = senders;
    kchan->receivers = receivers;
    kchan->malloc = kkt_alloc;        /* do I need them? */
    kchan->free = kkt_free;           /* do I need them? */
    kchan->waiters = 0;
    kchan->max_send_size = max_send_size;

    /*
     * Expect a lot of code, here.
     * For the time being, no sophysticated stuff.
     */
    kchan->chan_path = thisprotocol->path;

    if (sendconn) {
	kchan->chan_type = SENDCONN;
        kchan->input.deliver = sendconn;
    } else {
	kchan->chan_type = RPC;
        kchan->input.rpc_deliver = rpc;
    }

    /* in fact, I believe that should be a mutex */
    simple_lock_init(&kchan->chan_lock, ETAP_KKT_CHANNEL);

    queue_init(&kchan->handles_list);
    queue_init(&kchan->busy_list);
    queue_init(&kchan->message_cache);
    max_handles = senders + receivers;
    
    khandle = (kkt_handle_t)(*kkt_alloc)(channel, max_handles * sizeof(struct kkt_handle));
    assert(khandle);

    for (i = 0, kh = khandle; i < max_handles; i++, kh++) {
	kh->kchan = kchan;        /* backpointer */
	kh->hstate = IDLE;
	kh->request_ready = FALSE;
	kh->reply_ready = FALSE;
	queue_enter(&kchan->handles_list, kh, kkt_handle_t, link);
    }

    kkt_channel_list_lock();
    if (kkt_channels[channel]) {
	kkt_channel_list_unlock();
	(*kkt_free)(channel, (vm_offset_t)kchan, sizeof(struct kkt_channel));
	(*kkt_free)(channel, (vm_offset_t)khandle, max_handles * sizeof(struct kkt_handle));
	return (KKT_ERROR);
    } else {
	kkt_channels[channel] = kchan;
	kkt_channel_list_unlock();
    }

    for (i = 0; i < KKT_MESSAGE_CACHE_NUMBER_DEFAULT; i++) {
	MsgK qmess;

	qmess = (MsgK)pathAlloc(kchan->chan_path, sizeof(*qmess));
	if (qmess == (MsgK)0 || 
	    msgConstructContig(&qmess->message, kchan->chan_path, max_send_size,
			       MAX_HOTPATH_SIZE + KKT_STACK_SIZE) != XK_SUCCESS) 
	    continue;
	qmess->msg_type = CACHED;
	queue_enter(&kchan->message_cache, qmess, MsgK, link);
    }

    return (KKT_SUCCESS);
}


kkt_return_t
kkt_handle_alloc_common(
                        channel_t	channel,
                        handle_t 	*handle,
                        kkt_error_t  	error,
		        boolean_t	wait,
		        boolean_t	must_request,
		        unsigned int	size,
                        boolean_t       preallocate)
{
    kkt_channel_t kchan;
    xkern_return_t xkr;
    kkt_handle_t  khandle = KKT_HANDLE_NULL;
    Path pt;

    if (!valid_channel(channel) || kkt_channels[channel] == KKT_CHANNEL_NULL)
	return (KKT_ERROR);

    kchan = kkt_channels[channel];

    /*
     * Size is being used for __RPC__ only
     * (in the SEND_CONNECT case, size is being specified
     * as part of the request structure.)
     */
    if (kchan->max_send_size && size > kchan->max_send_size)
	return (KKT_INVALID_ARGUMENT);
    
    pt = kchan->chan_path;

    kkt_channel_lock(kchan);
    while (queue_empty(&kchan->handles_list)) {
	if (wait) {
	    kchan->waiters++;
	    assert_wait((event_t)&kchan->handles_list, FALSE);
	    kkt_channel_unlock(kchan);
	    thread_block(0);
	    kkt_channel_lock(kchan);
	} else {
	    kkt_channel_unlock(kchan);
	    return (KKT_RESOURCE_SHORTAGE);
	}
    }

    queue_remove_first(&kchan->handles_list, khandle, kkt_handle_t, link);
    kkt_channel_unlock(kchan);
    assert(khandle);
    assert(khandle->hstate == IDLE);
    assert(!khandle->request_ready);
    assert(!khandle->reply_ready);

    /* initialize the khandle */
    *khandle = handle_template;
    queue_enter(&kchan->busy_list, khandle, kkt_handle_t, link);
    khandle->error = error;
    khandle->must_request = must_request;
    khandle->kchan = kchan;
    simple_lock_init(&khandle->handle_lock, ETAP_KKT_HANDLE);

    while (!(khandle->retry_event = evAlloc(pt))) { /* = rather than == */
	if (wait) {
	    thread_will_wait_with_timeout(current_thread(), KKT_ALLOC_DELAY);
	    thread_block((void (*)(void)) 0);
	    reset_timeout_check(&current_thread()->timer);
	} else {
	    kkt_channel_lock(kchan);
	    queue_remove(&kchan->busy_list, khandle, kkt_handle_t, link);
	    queue_enter_first(&kchan->handles_list, khandle, kkt_handle_t, link);
	    kkt_channel_unlock(kchan);
	    return KKT_RESOURCE_SHORTAGE;
	}
    }

    if (preallocate) {
	/*
	 * In both the SEND_CONNECT and the RPC cases,
	 * it is convenient to preallocate a reply message.
	 * I might move this code to the xxx_init() routine.
	 * The reply message will be target of a msgAssign()
	 * (which is bcopy-free, btw).
	 * I'm not using Paths (yet!)
	 */
	for (;;) {
	    khandle->reply_message = pathAlloc(pt, sizeof(Msg_s));
	    if (khandle->reply_message != (Msg)0 &&
		msgConstructContig(khandle->reply_message, pt, 0, 0) == XK_SUCCESS) 
		    break;
	    xTrace0(kktp, TR_ERRORS, "KKT Unable to create reply message\n");
	    if (khandle->reply_message)
		pathFree(khandle->reply_message);
	    if (wait) {
		thread_will_wait_with_timeout(current_thread(), KKT_ALLOC_DELAY);
		thread_block((void (*)(void)) 0);
		reset_timeout_check(&current_thread()->timer);
	    } else {
		kkt_channel_lock(kchan);
		queue_remove(&kchan->busy_list, khandle, kkt_handle_t, link);
		queue_enter_first(&kchan->handles_list, khandle, kkt_handle_t, link);
		kkt_channel_unlock(kchan);
		return KKT_RESOURCE_SHORTAGE;
	    }
	}
	khandle->reply_ready = TRUE;
    
	/* 
	 * Only in the RPC case, we pre-allocate the
	 * message to be used as a request message.
	 * Try the cache first.
	 */
	if (size) {
	    KKT_HDR hdr;

	    for (;;) {
		khandle->request_message = kkt_grab_message(kchan);
		if (khandle->request_message != (Msg)0) 
		    break;
		xTrace2(kktp, TR_ERRORS, "KKT Unable to allocate request (%d+%d) bytes\n",
			(u_int)size, KKT_STACK_SIZE);
		if (wait) {
		    thread_will_wait_with_timeout(current_thread(), KKT_ALLOC_DELAY);
		    thread_block((void (*)(void)) 0);
		    reset_timeout_check(&current_thread()->timer);
		} else {
		    kkt_channel_lock(kchan);
		    khandle->reply_ready = FALSE;
		    queue_remove(&kchan->busy_list, khandle, kkt_handle_t, link);
		    queue_enter_first(&kchan->handles_list, khandle, kkt_handle_t, link);
		    kkt_channel_unlock(kchan);
	
		    msgDestroy(khandle->reply_message);
		    pathFree(khandle->reply_message);
		    return KKT_RESOURCE_SHORTAGE;
		}
	    }
	    msgForEach(khandle->request_message, msgDataPtr, (void *)&khandle->request_data);
	    /*
	     * Fill in the header
	     */
	    hdr.node = _node_self;
	    hdr.hchan = kchan->channel;
	    hdr.htype = RPC;
	    hdr.hsrc = (handle_t)khandle;
	    hdr.hdst = (handle_t)0;

	    xkr = msgPush(khandle->request_message, kkt_fillinfunc, &hdr, sizeof (hdr), (void *)0); 
	    assert(xkr == XK_SUCCESS);
	    khandle->request_ready = TRUE;
	}    
    }
    
    *handle = (handle_t) khandle;
    return (KKT_SUCCESS);
}

kkt_return_t
kkt_handle_free_common(
		       handle_t	handle)
{
    kkt_handle_t khandle;
    kkt_channel_t kchan;

    assert(handle);
    if (handle == -1)
	return (KKT_INVALID_HANDLE);

    khandle = (kkt_handle_t)handle;
    kchan = khandle->kchan;
    assert(khandle->hstate == BUSY || khandle->hstate == ABORTED);
    evDetach(khandle->retry_event);

    /*
     * It may be true in both SEND_CONNECT and RPC cases ...
     */
    if (khandle->reply_ready) {
	msgDestroy(khandle->reply_message);
	pathFree(khandle->reply_message);
	khandle->reply_ready = FALSE;
    }
	
    /*
     * It may be true in the RPC case only.
     */
    if (khandle->request_ready) {
	/* 
	 * Release the data associated to khandle->request_message.
	 */
	kkt_return_message(kchan, khandle->request_message);
	khandle->request_ready = FALSE;    
    }
 
    if (khandle->lls != ERR_XOBJ) {
	xk_close(khandle->lls, khandle->data_transfer);
    }

    khandle->hstate = IDLE;
    kkt_channel_lock(kchan);
    queue_remove(&kchan->busy_list, khandle, kkt_handle_t, link);
    queue_enter_first(&kchan->handles_list, khandle, kkt_handle_t, link);
    if (kchan->waiters) {
	kchan->waiters--;
	thread_wakeup_one((event_t)&kchan->handles_list);
    }
    kkt_channel_unlock(kchan);

    return (KKT_SUCCESS);
}

/*
 * Opening an x-kernel session and
 * binding it to a kkt handle.
 */
xkern_return_t
xk_open(
	IPhost node,
	unsigned int attr,
	XObj *lls)
{
    XObj	self, llp;
    Part_s	participants[2], *p;

    assert(*lls == ERR_XOBJ);
    /*
     * The KKT handle hasn't been connected yet to
     * an x-kernel session. This is the most typical
     * case for KKT_RPCs or for the first SEND_CONNECT
     * over an handle. The lls binding will survive
     * until the handle is alive.
     */
    self = thisprotocol;
    llp = xGetDown(self, 0);
    /* 
     * Set up the participants stack
     */
    p = &participants[0];
    partInit(p, 2);
    partPush(*p, &attr, sizeof(attr));
    partPush(*p, &node, sizeof(node));
    /*
     * The undelying VCACHE microprotocol is
     * supposed to handle a time-expiration
     * cache, addressed by the (node, attr) 
     * tuple. Thus, you may assume that xOpen 
     * and xClose are most of the time filtered 
     * by VCACHE.
     */
    *lls = xOpen(self, self, llp, p, thisprotocol->path);
    if (*lls == ERR_XOBJ) {
	xTrace0(kktp, TR_ERRORS,  "KKT can't open lower protocol");
	return XK_FAILURE;
    }

    return XK_SUCCESS;
}
/*
 * The most generic way to send bags of bits 
 * across the wire.
 */
xkern_return_t
xk_rpc(
       XObj lls,
       Msg request,
       Msg reply,
       boolean_t is_async,
       boolean_t *inprogress)
{
    xkern_return_t xkr;

    assert(lls != ERR_XOBJ);

    if (xControl(lls, CHAN_SET_ASYNC, (char *)&is_async,  sizeof(is_async)) < 0) {
	return XK_FAILURE;
    }
    *inprogress = TRUE;
    if (is_async) {
	assert(reply == (Msg)0);
	return xPush(lls, request) == XMSG_ERR_HANDLE?
	    XK_FAILURE : XK_SUCCESS;
    }

    xkr = xCall(lls, request, reply);
    *inprogress = FALSE;
    if (xkr != XK_SUCCESS) {
	xTrace1(kktp, TR_ERRORS,  "KKT xCall error %d", xkr);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}

/*
 * Abort a RPC in progress.
 */
void
xk_abort(
	 XObj lls)
{
    assert(lls != ERR_XOBJ);

    (void)xControl(lls, CHAN_ABORT_CALL, (char *)0, 0);
}

void
xk_close(
	 XObj lls,
	 boolean_t lls_is_active)
{
    assert(lls != ERR_XOBJ);

    if (lls_is_active)
	xk_abort(lls);
    xClose(lls);
}

void
kkt_freefunc(
	     void *data, 
	     int   size,
	     void *arg)
{
    /* 
     * Required by msgConstructInPlace. Nothing to do here.
     */
}

void
kkt_fillinfunc( 
	       void *hdr, 
	       char *des, 
	       long len, 
	       void *arg)
{
    /*
     * Used by the msgPush() utility.
     */
    bcopy(hdr, des, len);
}

long
kkt_pophdr(
	   void *hdr,
	   char *netHdr,
	   long int len,
	   void *arg)
{
    bcopy(netHdr, hdr, len);
    return len;
}

Msg
kkt_grab_message(
		 kkt_channel_t kchan)
{
    MsgK qmess;

    /*
     * 1st preference: message is cached
     */
    if (!queue_empty(&kchan->message_cache)) {    
	queue_remove_first(&kchan->message_cache, qmess, MsgK, link);
	assert(qmess);
	assert(qmess->msg_type == CACHED);
	assert(msgLen(&qmess->message) == kchan->max_send_size);
	return (Msg)qmess;
    }
    /*
     * 2nd preference: we must do anything from scratch :-(
     */
    qmess = (MsgK)pathAlloc(kchan->chan_path, sizeof(*qmess));
    if (qmess == (MsgK)0)
	return (Msg)qmess;
    
    if (msgConstructContig(&qmess->message, kchan->chan_path, kchan->max_send_size, 
			   MAX_HOTPATH_SIZE + KKT_STACK_SIZE) != XK_SUCCESS) {
	pathFree(qmess);
	return (Msg)0;
    }
    qmess->msg_type = ALLOCATED;
    return (Msg)qmess;
}

void
kkt_return_message(
		   kkt_channel_t kchan,
		   Msg           msp)
{
    MsgK qmess = (MsgK)msp;
    
    switch (qmess->msg_type) {
    case CACHED:
	assert(msgLen(&qmess->message) >= kchan->max_send_size);
	msgPopDiscard(&qmess->message, msgLen(&qmess->message) - kchan->max_send_size);
	assert(msgLen(&qmess->message) == kchan->max_send_size);
	queue_enter_first(&kchan->message_cache, qmess, MsgK, link);
	break;
    case ALLOCATED:
	msgDestroy(&qmess->message);
	pathFree(qmess);
	break;
    default:
	panic("kkt internal error: type of message does not match");
    }
}
