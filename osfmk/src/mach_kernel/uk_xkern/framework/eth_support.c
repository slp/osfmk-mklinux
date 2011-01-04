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
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 */

#include <mach_rt.h>
#include <xkern/include/xkernel.h>
#include <kern/sched_prim.h>

#include <mach/message.h>
#include <mach/mach_host_server.h>
#include <device/net_status.h>
/* xkernel.h includes xk_debug.h */
#include <xkern/include/assert.h>
/* platform includes process.h */
#include <xkern/include/platform.h>
#include <xkern/include/prot/eth.h>
#include <xkern/include/list.h>
#include <xkern/protocols/eth/eth_i.h>
#include <uk_xkern/include/eth_support.h>
#include <kern/lock.h>         

#ifdef TIME_XKINPUTPOOL
extern XTime push_begin, push_end, push_deltat; extern long push_end_ct;
#endif

InputPool       *defaultPool;
Map             poolMap;
queue_head_t ior_xkernel_qhead;

#if     XKSTATS
int     xk_ior_dropped;
int     xk_ior_queued;
int     xk_yields;
#endif /* XKSTATS */


#define POOL_MAP_SIZE  32
#define ETHER_INPUT_MAX (ROUND4(MAX_ETH_DATA_SZ) + ROUND4(NET_HDW_HDR_MAX) + 4)
#define BUFFER_ALLOC_SIZE (ETHER_INPUT_MAX)
#define	DEF_POOL_BUFFERS	32
#define DEF_POOL_THREADS	 4
#define MAX_BUFFERS_PER_VCI    128
#define MAX_THREADS_PER_VCI	32
#define INIT_BUFFERS_PER_VCI	32
#define INIT_THREADS_PER_VCI	 4
#define MIN_BUFFERS_PER_VCI      0
#define MIN_THREADS_PER_VCI	 0
#define INTER_ALLOC_DELAY	(5 * 1000)


/* forwards */
static xkern_return_t  readVciRom(
				  XObj self, 
				  char **str, 
				  int nFields, 
				  int line, 
				  void *arg);

static xkern_return_t bufInfoHandler(
				     XObj self, 
				     Path path,
				     u_int msgSize, 
				     int numMsgs);

static xkern_return_t poolInfoHandler(
				      XObj self,
				      Path path,
				      u_int numBuffs,
				      u_int numThreads,
				      XkThreadPolicy);


static void xkEthInputPool(void);

void xkEthInputLoop(InputPool *pool);


/* 
 * Default thread policy for input pools
 */
static int		defaultPriority = XK_INPUT_THREAD_PRIORITY;
static XkThreadPolicy_s defaultPolicy = {
    xkPolicyFixedFifo,
    &defaultPriority,
    sizeof(defaultPriority)
};


/*
 * Called from <anchor>_init()
 */
xkern_return_t
allocateResources(XObj self)
{
    VciType			vci = 0;
    Allocator		        defAlloc;

    poolMap = mapCreate(POOL_MAP_SIZE, sizeof(VciType), self->path);
    if ( ! poolMap ) {
	xTraceP0(self, TR_ERRORS, "allocateResources -- can't create map");
	return XK_FAILURE;
    }
    pathRegisterDriver(self, poolInfoHandler, bufInfoHandler);
    if ( mapResolve(poolMap, &vci, &defaultPool) == XK_FAILURE ) {
	xTraceP0(self, TR_ERRORS, "allocateResources -- no default pool");
	return XK_FAILURE;
    }
    /*
     * Initialize the (Msg_s+io_req_t) pool used for I/O
     * (we might block in io_req_alloc)
     */
    xk_thread_checkout(FALSE);
    {
	void	*ptr;
	Allocator   mda = pathGetAlloc(self->path, MD_ALLOC);
	io_req_t 	ior;
	int i;

	queue_init(&ior_xkernel_qhead);
	for (i = 0; i < MAX_IO_XK_BUFFERS; i++) {
	    ptr = allocGet(mda, sizeof( Msg_s ) + sizeof(ETHhdr));
	    if (!ptr) {
		xTraceP0(self, TR_ERRORS, "allocation error");
		continue;
	    }
	    io_req_alloc(ior);
	    memset((char *)ior, 0, sizeof(struct io_req));
	    ior->io_data = ptr;
	    enqueue_tail(&ior_xkernel_qhead, (queue_entry_t)ior);
#if     XKSTATS
	    xk_ior_queued++;
#endif /* XKSTATS */
	}
    }
    xk_thread_checkin();

    return XK_SUCCESS;
}

static void
poolFree(
	 InputPool *pool,
	 InputBuffer **b,
	 u_int resBuf,
	 u_int dataSize,
	 int allocBuf)
{
    int	i;

    if ( b ) {
	for ( i=0; i < allocBuf; i++ ) {
	    msgDestroy(&b[i]->upmsg);
	    pathFree(b[i]);
	}
	pathFree(b);
    }
    if ( pool ) {
	if ( pool->name ) {
	    pathFree(pool->name);
	}
	if ( pool->policy.arg ) {
	    pathFree(pool->policy.arg);
	}
	pathFree(pool);
    }
    if ( resBuf ) {
	xAssert(pool->path);
	msgReserveContig(pool->path, - resBuf, dataSize, 0);
    }
}


/*
 * Initialize an InputPool
 */
static InputPool *
InputPoolInit(
	      u_int numBuffers, 
	      u_int numThreads, 
	      u_int dataSize, 
	      Path path,
	      char *name,
	      void (*start)(void),
	      XkThreadPolicy policy)
{
    InputPool *pool;
    InputBuffer **buffers, *b;
    int i;
    thread_t th;
    xkern_return_t xkr;

    if (msgReserveContig(path, numBuffers, dataSize, 0) == XK_FAILURE) {
	xTrace3(ethp, TR_SOFT_ERRORS,
		"Couldn't allocate %d messages of %d bytes from path %d",
		numBuffers, dataSize, pathGetId(path));
	return IN_POOL_NULL;
    }

    if ( ! (pool = pathAlloc(path, sizeof(InputPool))) ) {
	msgReserveContig(path, - numBuffers, dataSize, 0);
	return IN_POOL_NULL;
    }
    bzero((char *)pool, sizeof(InputPool));
    queue_init(&pool->xkBufferPool);
    queue_init(&pool->xkIncomingData);
    simple_lock_init(&pool->iplock, ETAP_XKERNEL_ETHINPUT);
    pool->path = path;
    pool->allocSize = dataSize;
    pool->threads_out = 0;
    pool->threads_max = 0;
    if ( policy ) {
	pool->policy.func = policy->func;
	pool->policy.argLen = policy->argLen;
	if ( ! (pool->policy.arg = pathAlloc(path, policy->argLen)) ) {
	    poolFree(pool, 0, numBuffers, dataSize, 0);
	    return IN_POOL_NULL;
	}
	memcpy(pool->policy.arg, policy->arg, policy->argLen);
    } else {
	memcpy(&pool->policy, &defaultPolicy, sizeof(XkThreadPolicy_s));
    }
    if ( ! (pool->name = pathAlloc(path, strlen(name) + 1)) ) {
	poolFree(pool, 0, numBuffers, dataSize, 0);
	return IN_POOL_NULL;
    }
    strcpy(pool->name, name);
    buffers = pathAlloc(path, sizeof(InputBuffer *) * numBuffers);
    if ( ! buffers ) {
	poolFree(pool, 0, numBuffers, dataSize, 0);
	return IN_POOL_NULL;
    }
    for (i = 0; i < numBuffers; i++) {
	if ( ! (b = buffers[i] = pathAlloc(path, sizeof(InputBuffer))) ) {
	    poolFree(pool, buffers, numBuffers, dataSize, i-1);
	    return IN_POOL_NULL;
	}
	/* 
	 * The successful reservation above implies that these should
	 * succeed.  
	 */
        xkr = msgConstructContig(&b->upmsg, path, dataSize, 0);
	if ( xkr != XK_SUCCESS ) {
	    xTrace0(ethp, TR_ERRORS, "Error building pool buffers");
	    pathFree(b);
	    poolFree(pool, buffers, numBuffers, dataSize, i-1);
	    return IN_POOL_NULL;
	}
	msgForEach(&b->upmsg, msgDataPtr, &b->data);
	xAssert(b->data);
	b->driverProtl = 0;
        queue_enter_first(&pool->xkBufferPool, b, InputBuffer *, link);
    }
    
    xk_thread_checkout(FALSE);
    for (i = 0; i < numThreads; i++) {
	th = kernel_thread(kernel_task, start, (char *) 0);
        pool->threads_out++;
	pool->threads_max++;
        set_xk_input_thread(th);
	assert(th->top_act != THR_ACT_NULL);
	th->top_act->xk_resources = (void *)pool;
        thread_wakeup((event_t)&th->top_act->xk_resources);
    }
    xk_thread_checkin();
    return pool;
}


static xkern_return_t
poolInfoHandler(
    XObj		self,
    Path 		path,
    u_int 		numBuffs,
    u_int 		numThreads,
    XkThreadPolicy	policy)
{
    InputPool	*p;
    Bind	b;
    char	poolName[80];
    VciType	vci;

    vci = pathGetId(path);
    if ( mapResolve(poolMap, &vci, &p) == XK_SUCCESS ) return XK_SUCCESS;
    sprintf(poolName, "vciPool %d", vci);
    if ( ! numBuffs ) {
	numBuffs = (vci == 0 ? DEF_POOL_BUFFERS : INIT_BUFFERS_PER_VCI);
    }
    if ( ! numThreads ) {
	numThreads = (vci == 0 ? DEF_POOL_THREADS : INIT_THREADS_PER_VCI);
    }
    xTraceP3(self, TR_EVENTS,
	     "establish pool for path %d, %d threads, %d bufs",
	     pathGetId(path), numThreads, numBuffs);
    p = InputPoolInit(numBuffs, numThreads, BUFFER_ALLOC_SIZE, path,
		      poolName, xkEthInputPool, policy);
    if ( p == IN_POOL_NULL) {
	xTrace3(ethp, TR_ERRORS,
		"Could not create pool for vci %d, %d buffs, %d threads",
		vci, numBuffs, numThreads);
	return XK_FAILURE;
    }
    if ( mapBind(poolMap, &vci, p, 0) != XK_SUCCESS ) {
	xTrace1(ethp, TR_ERRORS, "could not bind pool for vci %d", vci);
	return XK_FAILURE;
    }
    return XK_SUCCESS;
}


static xkern_return_t
bufInfoHandler( XObj self, Path path, u_int msgSize, int numMsgs )
{
    int			numBuffs;
    xkern_return_t	xkr;

    xTraceP3(self, TR_EVENTS, "bufInfoHandler: %d messages of size %d, path %d",
	     numMsgs, msgSize, pathGetId(path));
    if ( msgSize > 0 ) {
	numBuffs = ((msgSize - 1) / ETHER_INPUT_MAX) + 1;
	xkr = msgReserveContig(path, numBuffs * numMsgs, BUFFER_ALLOC_SIZE, 0 );
	if ( xkr == XK_FAILURE ) {
	    return XK_FAILURE;
	}
    }
    return XK_SUCCESS;
}

/*
 * Entry point for a pool thread
 *
 *  A pool thread is activated by the driver; it processes the packet
 *   assigned by the driver and then dequeues from the incoming
 *   packet queue until it is empty.  
 *
 */
static void
xkEthInputPool(void)
{
    thread_t		thread = current_thread();
    InputPool           **pool = (InputPool **)&current_act()->xk_resources;
    xkern_return_t	xkr;

    thread->vm_privilege = TRUE;
    stack_privilege(thread);

    while (*pool == IN_POOL_NULL) {
	assert_wait((event_t) pool, FALSE);
	thread_block((void (*)(void)) 0);
    }

    xAssert((*pool)->policy.func);
    xkr = (*pool)->policy.func((*pool)->policy.arg, (*pool)->policy.argLen);
    if ( xkr != XK_SUCCESS ) {
	xTrace1(ethp, TR_ERRORS,
		"input thread policy function returns %d", xkr);
    } 
    xkEthInputLoop(*pool);
    /* NOTREACHED */
}

void
xkEthInputLoop(InputPool *pool)
{
    InputBuffer	             *buffer;
    Msg                      msg;
    spl_t                    ss;
    io_req_t 	             ior;
    int                      services = MAX_SERVICES;

    /* 
     * Beware: Uniprocessor non-preemptive code all over!!!
     */
    ss = splimp();
    simple_lock(&pool->iplock);
    pool->threads_out--;
    for (;;) {
        while (queue_empty(&pool->xkIncomingData)) {
            pool->threads_out++;
	    assert(pool->threads_out > 0 && pool->threads_out <= pool->threads_max);
	    assert_wait((event_t) pool, FALSE);		
	    simple_unlock(&pool->iplock);
	    thread_block((void (*)(void)) 0);
	    simple_lock(&pool->iplock);
	    pool->threads_out--;
	    assert(pool->threads_out >= 0 && pool->threads_out < pool->threads_max);
       	    services = MAX_SERVICES;
	}
        queue_remove_first(&pool->xkIncomingData, 
		           buffer, InputBuffer *, link);
	simple_unlock(&pool->iplock);
	splx(ss);

	msg = &buffer->upmsg;
	xAssert(xIsProtocol(buffer->driverProtl));
        /*
         * Buffer makeup, before it gets shepherded
         */
        msgSetAttr(&buffer->upmsg, 0, &buffer->hdr, sizeof(ETHhdr));
        msgTruncate(&buffer->upmsg, buffer->len);

#ifdef TIME_XKINPUTPOOL
	wait_for_tick(); xGetTime(&push_begin);
#endif
	xk_master_lock();
	xDemux(buffer->driverProtl, msg);

#ifdef TIME_XKINPUTPOOL
	push_end_ct = wait_for_tick(); xGetTime(&push_end);
#endif
	/*
	 * Cleanup the I/O request space
	 * (in most cases, msgDestroy is just a refCnt--)
	 */
	ss = splimp();
	for (ior = (io_req_t)queue_first(&ior_xkernel_qhead);
  	                     !queue_end(&ior_xkernel_qhead, (queue_entry_t)ior);
	                     ior = ior->io_next) {
	    if (!(ior->io_op & IO_IS_XK_DIRTY)) 
		break;
	    msgDestroy((Msg)ior->io_data);
	    ior->io_op &= ~IO_IS_XK_DIRTY;
	}
	splx(ss);
	/*
	 * refresh the buffer
	 */
	while (msgConstructContig(msg, pool->path, pool->allocSize, 0)
	       	== XK_FAILURE){
	    xTraceP1(buffer->driverProtl, TR_SOFT_ERRORS,
		     "Input buffer allocation error on path %d, sleeping",
		     pool->path->id);
	    /*
	     * ?? what to do here?? 
	     */
	    Delay( INTER_ALLOC_DELAY );
	}
	msgForEach(msg, msgDataPtr, &buffer->data);
	xk_master_unlock();

	xAssert(buffer->data);
	buffer->driverProtl = 0;

#if !MACH_RT
	if (--services < 0) { 
	    /* 
             * Give people a chance. AST_QUANTUM will push 
	     * my thread at the end of my runqueue.
	     */
#if XKSTATS
	    xk_yields++;
#endif /* XKSTATS */
            thread_block_reason((void (*)(void)) 0, AST_QUANTUM);
       	    services = MAX_SERVICES;
	}
#endif /* !MACH_RT */
	ss = splimp();
        simple_lock(&pool->iplock);
        queue_enter_first(&pool->xkBufferPool, buffer, InputBuffer *, link);
    }
}




