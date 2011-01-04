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
 *	File:   i860/PARAGON/kkt/kkt_driver.c
 *	Author: Steve Sears
 *	Date:   1994
 *
 *	Test driver for KKT interface
 */

#if	PARAGON860
#include <loopback_eng.h>
#else	/* PARAGON860 */
#define	LOOPBACK_ENG	0
#endif	/* PARAGON860 */

#include <mach_kdb.h>
#include <mach/kkt_request.h>
#include <kern/lock.h>
#include <dipc/dipc_timer.h>
#include <kern/misc_protos.h>
#include <norma_scsi.h>
#include <dipc_xkern.h>

boolean_t	kkt_driver_awake = FALSE;
boolean_t	kkt_run_driver = FALSE;
boolean_t	kkt_receiver = FALSE;
boolean_t	kkt_loopback = LOOPBACK_ENG;

typedef struct tran_request	*tran_request_t;

/*
 * The endpoint is used in these tests to transmit the test and the 
 * size of the following data.  The available tests are:
 */
#define TEST_BIGBUF	0xedde0001
#define TEST_SG		0xedde0002
#define TEST_STD	0xedde0003

/*
 *	This endpoint is reserved for use by higher-level clients
 *	for their own tests.  KKT will use its own test thread
 *	to deliver client RPCs.  No OOL data is expected.  The
 *	only client information is the address of the routine
 *	to execute, in endpoint.ep[1].
 */
#define	TEST_CLIENT	0x66666666
typedef void (*kkt_test_client)(void);

/*
 *	For the moment, testing KKT relies on the presence of
 *	a test buffer allocated by DIPC.  No, this is not pretty.
 *	XXX
 */
extern vm_offset_t	dipc_test_region;
extern vm_size_t	dipc_test_size;
extern int		dipc_test_verify_buffer(char *);

int test_type_set[] = {TEST_STD, TEST_SG, 0 };

#define MIN(a,b)	(a < b ? a : b)

struct tran_request {
	tran_request_t	next;
	enum {
		RPC_send,
		RPC_recv,
		Recv_Connect,
		Recv_Bigbuf,
		Recv_Engine,
		Send_Engine,
		Recv,
		Client			/* higher-level test */
	} type;
	request_block_t request;
	kkt_callback_t	callback;
	handle_t	handle;
	vm_size_t	size;
	unsigned int	msg_size;
	unsigned int	test_type;
};

tran_request_t	test_work_queue;
tran_request_t	test_recv_queue;
decl_simple_lock_data(,work_queue_lock)

typedef	struct garbage		*garbage_t;

#define	KMEM	1
#define	ZMEM	2

struct garbage {
	int		type;
	garbage_t	next;
	void		*poly;
};

garbage_t	garbage_list;
decl_simple_lock_data(,garbage_lock_data)
#define garbage_lock() simple_lock(&garbage_lock_data)
#define garbage_unlock() simple_unlock(&garbage_lock_data)

/*
 * Structure used to maintain current postion in data
 */
struct msg_progress {
	int		chain_count;
	unsigned int	timer;
	vm_offset_t	data_size;	/* total size of data */
	int		data_remaining; /* number of bytes remaining */
	int		sync;
	decl_simple_lock_data(,lock)
} msg_progress, rmsg_progress;

#define TESTMAGIC	0xdeadbabe

/*
 * Prototype declarations
 */
extern void transport_recv_test(
	handle_t	rpc_handle,
	vm_size_t	size,
	int		itoken,
	int		chunks,
	vm_offset_t	receive_data);


void	test_error(handle_t, int, request_block_t),
	test_deliver(unsigned int, handle_t, endpoint_t *, vm_size_t,
		     boolean_t, boolean_t),
	test_handle_alloc(int, int),
	test_recv_callback(int, handle_t, request_block_t, boolean_t),
	test_release(int, garbage_t, void *),
	test_send_callback(int, handle_t, request_block_t, boolean_t),
	test_enqueue_tran(tran_request_t),
    	test_release_request(request_block_t req),
	tran_free(tran_request_t),
	imsg_engine(handle_t, request_block_t),
        kkt_driver_thread(void),
	kkt_receiver_thread(void),
	kkt_test_support(void),
	show_tran_req(tran_request_t);

void	rpc_deliver(
		int		chan,
		int		*buf,
		boolean_t	thread_context);

void	test_send(vm_size_t, node_name, unsigned int, unsigned int);

int	test_allocate_chains(
		request_block_t	*chains,
		vm_size_t		size,
		unsigned int	rtype,
		struct msg_progress *mp,
		kkt_callback_t	callback);

kkt_return_t	test_recv_big_buf(tran_request_t);

tran_request_t
	test_tran_alloc(void);

int	test_data_send(
		handle_t	handle,
		vm_size_t	size,
		node_name	node,
		unsigned int	timer,
		unsigned int	test),

	test_recv_connect(
		tran_request_t tran);

void test_timer_overhead(void);
void test_rpc_test(node_name node);
void test_node_map(void);
void kkt_test_misc(void);
void kkt_test_driver(node_name	node);
request_block_t get_request_chain(
				  int		req_count,
				  unsigned int	req_type);
int get_req_cache(
		  request_block_t	*chains,
		  unsigned int		rtype,
		  struct msg_progress	*mp,
		  kkt_callback_t	callback);

void	test_tran_free(tran_request_t tran);
vm_offset_t kkt_test_alloc(void		*opaque,
			   vm_size_t	size);
void 	kkt_test_free(void		*opaque,
		      vm_offset_t	data,
		      vm_size_t		size);
void	kkt_test_bootstrap(void);
boolean_t kkt_start_test(int	new_target);

unsigned int	trbb_timer_start(
			unsigned int	size);




/*
 * Keep a static array of transaction records around that can be used
 * from interrupt, as opposed to dynamically allocating memory.
 */
#define	MAX_TRAN_RECORDS	20
struct tran_request tran_pool[MAX_TRAN_RECORDS];
tran_request_t	tran_free_list;

zone_t	kkt_test_request_zone;
zone_t	kkt_tran_request_zone;

#define	NULL_TRAN_REQ	(tran_request_t)0
#define	LARGE_KMSG_SIZE	32768
#define	BIG_KMSG_SIZE	(4*1024*1024)

int kmsg_inline_size = 52;
#define	RECV_HANDLE_COUNT	10
#define	SEND_HANDLE_COUNT	10

#define RPC_SENDERS		3
#define RPC_RECEIVERS		3
#define RPC_MAX_SIZE		32


/*
 * This is the target node.  Put here as a variable so we can
 * redirect things from the debugger to use different nodes.
 */
int kkt_test_target = 10;

int kktd_chain_len;	/* Initialized in boot init routine */

char *kktd_send_sm_timer = "kktd send small";
char *kktd_recv_sm_timer = "kktd recv small";
char *kktd_scon = "kktd send_con";
char *kktd_scon_sg = "kktd sg send_con";
char *kktd_rpc_timer = "KKT_RPC";

/*
 *	The following array needs to be restructured to make it easier
 *	to identify the entries that apply to each transport.
 */	

char *kktd_timers[] = {
#if	!DIPC_XKERN
	"kktd 4K send",
	"kktd 4K recv",
	"kktd sg 4K send",
	"kktd sg 4K recv",

	"kktd 8K send",			/* Algorithms using these timer */
	"kktd 8K recv",			/* names expect them to come in */
	"kktd sg 8K send",		/* blocks of 4 as they are here */
	"kktd sg 8K recv",

	"kktd 16K send",
	"kktd 16K recv",
	"kktd sg 16K send",
	"kktd sg 16K recv",

	"kktd 32K send",
	"kktd 32K recv",
	"kktd sg 32K send",
	"kktd sg 32K recv",

	"kktd 64K send",
	"kktd 64K recv",
	"kktd sg 64K send",
	"kktd sg 64K recv",

	"kktd 128K send",
	"kktd 128K recv",
	"kktd sg 128K send",
	"kktd sg 128K recv",

	"kktd 256K send",
	"kktd 256K recv",
	"kktd sg 256K send",
	"kktd sg 256K recv",

	"kktd 512K send",
	"kktd 512K recv",
	"kktd sg 512K send",
	"kktd sg 512K recv",

	"kktd 1M send",
	"kktd 1M recv",
	"kktd sg 1M send",
	"kktd sg 1M recv",

	"kktd 2M send",
	"kktd 2M recv",
	"kktd sg 2M send",
	"kktd sg 2M recv",
#if 0
#if	PARAGON860
	"kktd 3M send",
	"kktd 3M recv",
	"kktd sg 3M send",
	"kktd sg 3M recv",
#endif	/* PARAGON860 */

	"kktd 4M send",
	"kktd 4M recv",
	"kktd sg 4M send",
	"kktd sg 4M recv",

#if	PARAGON860
	"kktd 5M send",
	"kktd 5M recv",
	"kktd sg 5M send",
	"kktd sg 5M recv",

	"kktd 6M send",
	"kktd 6M recv",
	"kktd sg 6M send",
	"kktd sg 6M recv",

	"kktd 7M send",
	"kktd 7M recv",
	"kktd sg 7M send",
	"kktd sg 7M recv",
#endif	/* PARAGON860 */

	"kktd 8M send",
	"kktd 8M recv",
	"kktd sg 8M send",
	"kktd sg 8M recv",

#if	PARAGON860
	"kktd 9M send",
	"kktd 9M recv",
	"kktd sg 9M send",
	"kktd sg 9M recv",
#endif	/* PARAGON860 */

#if	NORMA_SCSI
	"kktd 16M send",
	"kktd 16M recv",
	"kktd sg 16M send",
	"kktd sg 16M recv",
#endif	/* NORMA_SCSI */
#endif /* 0 */
#else	/* !DIPC_XKERN */
	"kktd 4K send",
	"kktd 4K recv",
	"kktd sg 4K send",
	"kktd sg 4K recv",

	"kktd 16K send",
	"kktd 16K recv",
	"kktd sg 16K send",
	"kktd sg 16K recv",

	"kktd 64K send",
	"kktd 64K recv",
	"kktd sg 64K send",
	"kktd sg 64K recv",

	"kktd 1M send",
	"kktd 1M recv",
	"kktd sg 1M send",
	"kktd sg 1M recv",
#endif	/* !DIPC_XKERN */

	0
};

#define	K	* 1024
#define	M	* 1024 * 1024

struct script {
	int	iterations;
	int	size;
	char	*timer;
	void	(*test)(vm_size_t, node_name, unsigned int, unsigned int);
} script[] = {
	/*
	 *	timer values are initialized in the bootstrap
	 *	routine below.
	 */
	/* iterations	size		timer		test */
#if	PARAGON860
	/*
	 *	Paragon is faster, so we can do more iterations.
	 */
	{25,		4 K,		0,	test_send},
	{25,		8 K,		0,	test_send},
	{25,		16 K,		0,	test_send},
	{25,		32 K,		0,	test_send},
	{25,		64 K,		0,	test_send},
	{10,		128 K,		0,	test_send},
	{10,		256 K,		0,	test_send},
	{10,		512 K,		0,	test_send},
	{10,		1 M,		0,	test_send},
	{10,		2 M,		0,	test_send},
#if 0
	{10,		3 M,		0,	test_send},
	{10,		4 M,		0,	test_send},
	{10,		5 M,		0,	test_send},
	{10,		6 M,		0,	test_send},
	{10,		7 M,		0,	test_send},
	{10,		8 M,		0,	test_send},
	{10,		9 M,		0,	test_send},
#endif	/* 0 */
#elif	NORMA_SCSI
	{100,		4 K,		0,	test_send},
	{100,		8 K,		0,	test_send},
	{100,		16 K,		0,	test_send},
	{50,		32 K,		0,	test_send},
	{50,		64 K,		0,	test_send},
	{25,		128 K,		0,	test_send},
	{25,		256 K,		0,	test_send},
	{25,		512 K,		0,	test_send},
	{10,		1 M,		0,	test_send},
	{10,		2 M,		0,	test_send},
#if 0
	{10,		4 M,		0,	test_send},
	{10,		8 M,		0,	test_send},
	{10,		16 M,		0,	test_send},
#endif /* 0 */
#elif	DIPC_XKERN
	{25,		 4 K,		0,	test_send},
	{25,		16 K,		0,	test_send},
	{25,		64 K,		0,	test_send},
	{10,		1 M,		0,	test_send},
#endif	/* PARAGON860 */
	{0}
};


/*
 * Test misc interfaces for correctness.  Not currently invoked
 * from anywhere - typically called from kkt_test_driver, but
 * currently deleted for performance evaluation
 */
void
kkt_test_misc(void)
{
	test_handle_alloc(CHAN_TEST_KMSG, SEND_HANDLE_COUNT);
	test_node_map();
}

extern int	kkt_test_iterations;
int		kkt_rpc_test_iterations = 10000;

void
kkt_test_driver(
	node_name	node)
{
	int i;
	struct script *sp;
	int	*test_set;
	int	test_type;
	int	max_script_size;

	test_timer_overhead();

	timer_set((unsigned int) kktd_send_sm_timer, kktd_send_sm_timer);
	timer_set((unsigned int) kktd_scon, kktd_scon);
	timer_set((unsigned int) kktd_scon_sg, kktd_scon_sg);

	test_rpc_test(node);

	if (!dipc_test_verify_buffer("kkt_test_driver"))
		return;

	for (i = 0; i < kkt_test_iterations; i++)
		test_send(kmsg_inline_size,
			  node,
			  (unsigned int)kktd_send_sm_timer,
			  TEST_STD);

	printf("\tKKT OOL Tests:\n");
#if	0
	/*
	 *	Timings should really be sender-initiation through
	 *	receiver activation.
	 */
	printf("           KKT Low-Level OOL Timings\n");
	printf("Size            Iterations              Rate\n");
	printf("----            ----------              ----\n");
#endif	/* 0 */
	for (test_set = test_type_set; *test_set; test_set++) {
		sp = script;
		while (sp->size > 0) {
			if (*test_set == TEST_STD)
				sp->timer = kktd_timers[(sp - script) * 4];
			else
				sp->timer = kktd_timers[(sp - script) * 4 + 2];
			timer_set((unsigned int)sp->timer, sp->timer);
			for (i = sp->iterations; i > 0; i--) {
				(sp->test)(sp->size,
					   node,
					   (unsigned int)sp->timer,
					   *test_set);
			}
#if    MACH_KDB || KGDB
			printf("\t\t%s pass:  size=%d\n",
			       *test_set == TEST_STD ? "TEST_STD" : "TEST_SG",
			       sp->size);
/**			db_show_one_timer((unsigned int)sp->timer); **/
#endif /* MACH_KDB || KGDB */
			sp++;
		}
	}

	printf("\t\tKKT OOL Tests complete\n");
}

void
test_handle_alloc(int	channel,
		  int	handle_count)
{
	handle_t	hstack[SEND_HANDLE_COUNT+2];
	register int	sp;
	handle_t	handle;
	kern_return_t	kr;

	for (sp = 0; sp < handle_count; sp++) {
		kr = KKT_HANDLE_ALLOC(channel,
				      &hstack[sp],
				      (kkt_error_t)test_error,
				      FALSE,
				      FALSE);
		assert(kr == KKT_SUCCESS);
	}
#if 0	/* I changed the allocation mechanism to be a pool
	 * of handles, so the following is not a valid test
	 */
	/*
	 * We have allocated all of the valid handles, now
	 * get one more...
	 */
	kr = KKT_HANDLE_ALLOC(channel,
			      &handle,
			      (ktt_error_t)test_error,
			      FALSE);
	assert(kr == KKT_ERROR);
#endif
	for (sp = 0; sp < handle_count; sp++) {
		kr = KKT_HANDLE_FREE(hstack[sp]);
		assert(kr == KKT_SUCCESS);
	}
	/*
	 * Release the first one again
	 */
	kr = KKT_HANDLE_FREE(hstack[0]);
	assert(kr != KKT_SUCCESS);
}

/*
 * test_send
 *
 * Main useful routine for sending arbitary amounts of data.  This
 * routine is self contained so it can be invoked from any point
 * in the system.
 */
void
test_send(vm_size_t	size,
	  node_name	node,
	  unsigned int	timer,
	  unsigned int	test)
{
	handle_t	handle;
	kern_return_t	kr;

	if (kkt_loopback)
		node = KKT_NODE_SELF();

	kr = KKT_HANDLE_ALLOC(CHAN_TEST_KMSG,
			      &handle,
			      (kkt_error_t)test_error,
			      TRUE,
			      TRUE);
	assert(kr == KKT_SUCCESS);
	kr = test_data_send(handle,
			    size,
			    node,
			    timer,
			    test);

	/*
	 * Defer releasing the handle until the final callback; this
	 * relieves us of ugly synchronization in loopback mode and
	 * makes a single point for handle release for both sender
	 * and receivers.
	 */
}

/*
 * test_data_send:
 *
 * Send an arbitary amount of data to a receiver.  This works by
 * filling out a vector of request chains which are then passed 
 * the the transport.  Notice that the first element of each chain
 * has the callback associated with it and is actually sent last
 * to facilitate clean up (test_allocate_chains does the right thing
 * with the data pointers).
 */
kkt_return_t
test_data_send(handle_t		handle,
	       vm_size_t	size,
	       node_name	node,
	       unsigned int	timer,
	       unsigned int	test)
{
	request_block_t	chains[10], req, *cp;
	kern_return_t	kr, ret[2];
	boolean_t	more = TRUE;
	spl_t		s;
	endpoint_t	endpoint;
	unsigned int	rtype;
	unsigned int	stimer;

	msg_progress.timer = timer;
	msg_progress.data_size = size;
	stimer = (unsigned int)kktd_scon;
	rtype = REQUEST_SEND;
	if (test == TEST_SG) {
		rtype |= REQUEST_SG;
		stimer = (unsigned int)kktd_scon_sg;
	}
	msg_progress.data_remaining = size - test_allocate_chains(
							   chains,
							   size,
							   rtype,
							   &msg_progress,
							   test_send_callback);
	simple_lock_init(&msg_progress.lock, ETAP_KKT_TEST_MP);
	cp = (request_block_t *)chains;
	/*
	 * This is a little tricky: if there are multiple requests,
	 * we want to send the second one in the chain - the first
	 * one is the last data element and has the callback.  Below,
	 * we need to start with the 3rd in the chain and finally send
	 * the first.
	 */
	if (!(req = (*cp)->next)) {
		req = *cp;
		if (*(cp + 1) == NULL_REQUEST)
			more = FALSE;
		else {
			/*
			 * If there are no request chains involved,
			 * and the next "chain" entry is non-zero,
			 * we are dealing with an sglist so increment
			 * to the next chain.
			 */
			cp++;
		}
	}

	/*
	 * The endpoint data describes what is coming.
	 */
	endpoint.ep[0] = (void *)test;
	endpoint.ep[1] = (void *)size;

	if (size < req->data_length)
		req->data_length = size;

	/*
	 * Start the timer now to test how long the entire operation
	 * takes.  This timing will be stopped on the callback.
	 */
	timer_start(timer);
	timer_start(stimer);
	msg_progress.sync = FALSE;
	kr = KKT_SEND_CONNECT(handle,
			      node,
			      &endpoint,
			      req,
			      more,
			      &ret[0]);
	assert(kr == KKT_SUCCESS || kr == KKT_DATA_SENT);
	timer_stop(stimer, 0);

	/*
	 * send the rest of the chains
	 */
	req = req->next;
	s = splkkt();
	if (more) {
		while (*cp) {
			while (req) {
				kr = KKT_REQUEST(handle, req);
				assert(kr == KKT_SUCCESS);
				req = req->next;
			}
			kr = KKT_REQUEST(handle, *cp);
			assert(kr == KKT_SUCCESS);
			req = *++cp;
			if (req)
				req = req->next;
		}
	}

	/*
	 * Synchronize here before starting the next test
	 */
	simple_lock(&msg_progress.lock);
	while (msg_progress.sync == FALSE) {
		assert_wait((event_t)&msg_progress, TRUE);
		simple_unlock(&msg_progress.lock);
		splx(s);
		thread_block((void (*)) 0);
		s = splkkt();
		simple_lock(&msg_progress.lock);
	}
	simple_unlock(&msg_progress.lock);
	splx(s);

	return KERN_SUCCESS;
}

/*
 * get_request_chain
 *
 * Given a count, allocate a number of requests and chain them together.
 * A callback will be put in the front request.  This routine assumes
 * a data size of PAGE_SIZE.
 */
request_block_t
get_request_chain(
	int		req_count,	/* count of requests */
	unsigned int	req_type)	/* READ or WRITE */
{
	request_block_t	req, base;

	/*
	 * get the first request block and initialized it - use it
	 * as a template for the rest.
	 */
	req = (request_block_t)zalloc(kkt_test_request_zone);
	bzero((char *)req, sizeof(struct request_block));
	req->request_type = req_type;
	base = req;
	if (req_type & REQUEST_SG) {
		req->data.list = 
		   (kkt_sglist_t)kalloc(kktd_chain_len *
					sizeof(struct io_scatter));
		req->data_length = req_count * PAGE_SIZE;
	} else {
		req->data_length = PAGE_SIZE;
		for (req_count--; req_count > 0; req_count--) {
			req->next = 
				(request_block_t)zalloc(kkt_test_request_zone);
			req = req->next;
			*req = *base;
		}
	}
	req->next = NULL_REQUEST;

	return base;
}

/*
 * allocate chains for the request.  Notice that all requests are
 * a maximum size of 1K.  Also notice that the first element in each
 * chain will be sent last to facilitate cleanup: therefore, it
 * receives the final address in each chain.
 */
int
test_allocate_chains(request_block_t	*chains,
		     vm_size_t		size,
		     unsigned int	rtype,
		     struct msg_progress *mp,
		     kkt_callback_t	callback)
{
	request_block_t	req;
	int 		blocks, cnt = 0, i;
	int		req_count, send_size;
	vm_offset_t	data;
	int		data_size, data_send;

	if (size == 0)
		return 0;
	if (!dipc_test_verify_buffer("test_allocate_chains"))
		return 0;

	req_count = size / PAGE_SIZE;
	if (size & page_mask)		/* get final partial */
		req_count++;

	if (req_count > kktd_chain_len * 2)
		req_count = kktd_chain_len * 2;
 	send_size = req_count * PAGE_SIZE;
	if (send_size > size)
		send_size = size;

	if (send_size > dipc_test_size)
	        send_size = dipc_test_size;

	data_size = send_size;
	/*
	 * Use pages from the pre-allocated region.  If this is not
	 * available (i.e. you are trying to test KKT w/o DIPC) then
	 * kalloc the memory as you go.
	 */
	data = dipc_test_region;
	/*
	 * get a request chain and initialize the data.  Notice that
	 * for our tests, we only allocate a couple of chains and
	 * multiplex them repeatedly to get the data amounts we
	 * really want.  At least for this test, we don't worry
	 * about page alignment.
	 */
	while (req_count) {
		assert(data_size);
		cnt = MIN(req_count, kktd_chain_len);
		req_count -= cnt;
		req = get_request_chain(cnt, rtype);
		req->callback = callback;
		req->args[0] = (unsigned int)mp;
		mp->chain_count++;
		*chains++ = req;
		if (rtype & REQUEST_SG) {
			for (i = 0; i  < cnt; i++) {
				data_send = MIN(PAGE_SIZE, data_size);
				data_size -= data_send;
				req->data.list[i].ios_address = data;
				data += data_send;
				req->data.list[i].ios_length = data_send;
			}
		} else {
			while (req) {
				data_send = MIN(PAGE_SIZE, data_size);
				data_size -= data_send;
				
				req->data.address = data;
				data += data_send;
				req->data_length = data_send;
				req = req->next;
			}
		}
	}
	*chains = NULL_REQUEST;

	return send_size;
}


/*
 *	Measure timing package overhead.
 *		- Cost of timer_set
 *		- Cost of timer_start+timer_stop
 *
 *	N.B.  The timer_set cost is probably worst-case; we
 *	are inserting keys in ascending value, causing the
 *	tree to degenerate to a list.
 *
 *	The timer_start+timer_stop cost is best-case;
 *	all of the timer routines use timer_search to lookup
 *	a key in a tree; this time obviously can vary with the
 *	number of keys in the tree!
 */

#define	TIMER_OVERHEAD_KEY_START		1234
#define	TIMER_OVERHEAD_SET_COUNT		1000
#define	TIMER_OVERHEAD_TIME_COUNT		1000

char	*timer_set_overhead = "Tmr Set Ovrhd";
char	*timer_time_overhead = "Tmr Time Ovrhd";
char	*timer_ovrhd_name = "overhead timer";

void
test_timer_overhead(void)
{
	int	i;
	
	timer_clear_tree();

	timer_set((unsigned int)timer_set_overhead, timer_set_overhead);
	timer_set((unsigned int)timer_time_overhead, timer_time_overhead);

	timer_set((unsigned int)timer_time_overhead, timer_time_overhead);
	for (i = 0; i < TIMER_OVERHEAD_TIME_COUNT; i++) {
		timer_start((unsigned int)timer_time_overhead);
		timer_stop((unsigned int)timer_time_overhead, 0);
	}

	for (i = 0; i < TIMER_OVERHEAD_SET_COUNT; i++) {
		timer_start((unsigned int)timer_set_overhead);
		timer_set((unsigned int)TIMER_OVERHEAD_KEY_START + i,
			  timer_ovrhd_name);
		timer_stop((unsigned int)timer_set_overhead, 0);
	}
	printf("\tTimer Overhead tests completed\n");
}


void
test_rpc_test(node_name node)
{
	handle_t	handle;
	kkt_return_t	kktr;
	int		*buf;
	int		i;

	timer_set((unsigned int)kktd_rpc_timer, kktd_rpc_timer);
	kktr = KKT_RPC_HANDLE_ALLOC(CHAN_TEST_RPC, &handle, (vm_size_t)20);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_RPC_HANDLE_BUF(handle, (void **) &buf);
	assert(kktr == KKT_SUCCESS);
	buf[0] = 0, buf[1] = 1, buf[2] = 2, buf[3] = 3, buf[4] = 4;
	kktr = KKT_RPC(node, handle);
	assert(kktr == KKT_SUCCESS);
#if	MACH_ASSERT
	if (buf[0] == 0xc001babe &&
	    buf[1] != 0xc1000001 &&
	    buf[2] != 0xc2000002 &&
	    buf[3] != 0xc3000003 &&
	    buf[4] != 0xc4000004)
		printf("RPC TEST fails: 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
		       buf[0], buf[1], buf[2], buf[3], buf[4]);
#endif	/* MACH_ASSERT */
	printf("\tKKT_RPC functional test completed\n");

	for (i = 0; i < kkt_rpc_test_iterations; i++) {
		timer_start((unsigned int)kktd_rpc_timer);
		(void) KKT_RPC(node, handle);
		timer_stop((unsigned int)kktd_rpc_timer, 0);
	}
	KKT_RPC_HANDLE_FREE(handle);
	printf("\tKKT_RPC timing test completed\n");
}


/* test node map interfaces */
void
test_node_map(void)
{
	node_map_t	map = 0;
	kkt_return_t	kktr;
	int		i;
	node_name	node_list[4] = {4, 1000, 0, 500};
	node_name	node;

	kktr = KKT_MAP_ALLOC(&map, TRUE);
	assert(kktr == KKT_SUCCESS);
	assert(map != 0);

	/* put in some nodes */
	kktr = KKT_ADD_NODE(map, node_list[0]);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_ADD_NODE(map, node_list[1]);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_ADD_NODE(map, node_list[2]);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_ADD_NODE(map, node_list[3]);
	assert(kktr == KKT_SUCCESS);

	/* try a bogus node */
	kktr = KKT_ADD_NODE(map, 100000);
	assert(kktr == KKT_INVALID_ARGUMENT);

	/* put in one that's already there */
	kktr = KKT_ADD_NODE(map, node_list[1]);
	assert(kktr == KKT_NODE_PRESENT);

	/* take the nodes out of the map */
	kktr = KKT_REMOVE_NODE(map, &node);
	assert(kktr == KKT_SUCCESS);
	for (i=0; i<4; i++) {
		if (node_list[i] == node) {
			node_list[i] = -3;
			break;
		}
	}

	kktr = KKT_REMOVE_NODE(map, &node);
	assert(kktr == KKT_SUCCESS);
	for (i=0; i<4; i++) {
		if (node_list[i] == node) {
			node_list[i] = -3;
			break;
		}
	}

	kktr = KKT_REMOVE_NODE(map, &node);
	assert(kktr == KKT_SUCCESS);
	for (i=0; i<4; i++) {
		if (node_list[i] == node) {
			node_list[i] = -3;
			break;
		}
	}

	kktr = KKT_REMOVE_NODE(map, &node);
	assert(kktr == KKT_RELEASE);	/* should be the last node */
	for (i=0; i<4; i++) {
		if (node_list[i] == node) {
			node_list[i] = -3;
			break;
		}
	}

	/* make sure the right nodes came back */
	for (i=0; i<4; i++)
		assert(node_list[i] == -3);

	/* make sure map is empty */
	kktr = KKT_REMOVE_NODE(map, &node);
	assert(kktr == KKT_MAP_EMPTY);

	/* free the map */
	kktr = KKT_MAP_FREE(map);
	assert(kktr == KKT_SUCCESS);

	printf("KKT node map test completed\n");
}


/*
 * test_deliver
 *
 * Upcall entry point from KKT.  This will create a transaction
 * record and enqueue it, which will cause a receiver thread
 * to get kicked off to process receiving the message.
 */
#define	TEST_DELIVER_MAX	256
void
test_deliver(unsigned int	channel,
	     handle_t		handle,
	     endpoint_t		*endpoint,
	     vm_size_t		size,
	     boolean_t		inlinep,
	     boolean_t		thread_context)
{
	tran_request_t	tran;
	unsigned int	test;
	int		token;
	kkt_return_t	kktr;
	char		buf[TEST_DELIVER_MAX];

	if (channel == CHAN_RESERVE_TEST) {
		/*
		 * Run the raw transport test from interrupt mode
		 * as fast as possible.
		 */
		assert(size < TEST_DELIVER_MAX);
		kktr = KKT_COPYOUT_RECV_BUFFER(handle, buf);
		assert(kktr == KKT_SUCCESS);
		
		if (!dipc_test_verify_buffer("test_deliver (RESERVE_TEST)"))
			return;

		token = *((int *) buf);
		transport_recv_test(handle,
				    size,
				    token,
				    (int)endpoint->ep[0],
				    dipc_test_region);
		return;
	}

	/*
	 *  This request will have to be dealt with in thread context,
	 *  so allocate a transaction and throw it on the queue.
	 *  Notice that we allow various tests to use this framework,
	 *  using the endpoint as a tag.
	 */
	tran = test_tran_alloc();

	test = (unsigned int) endpoint->ep[0];
	switch(test) {
	    case TEST_BIGBUF:
		tran->type = Recv_Bigbuf;
		break;

	    case TEST_STD:
	    case TEST_SG:
		tran->type = Recv_Connect;
		break;

	    case TEST_CLIENT:
		tran->type = Client;
		break;

	    default:
		printf("Unknown test specified! 0x%x\n", endpoint->ep[0]);
		break;
	}

	tran->handle = handle;
	tran->size = size;
	tran->test_type = (unsigned int)endpoint->ep[0];
	tran->msg_size  = (unsigned int)endpoint->ep[1];

	if (tran->type != Client) {
		/*
		 * Release the sender
		 */
		kktr = KKT_CONNECT_REPLY(handle,
					 (kern_return_t) KKT_SUCCESS,
					 (kern_return_t) KKT_SUCCESS);
		assert(kktr == KKT_SUCCESS);
	}
	assert(!inlinep);	/* Temporary test */
	test_enqueue_tran(tran);
}

/*
 * rpc_deliver()
 *
 * receive an RPC from a remote node.  Just flip some bits and send
 * it back to establish point-to-point contact.
 */
void
rpc_deliver(int		chan,
	    int		*buf,
	    boolean_t	thread_context)
{
	handle_t	handle;
	kern_return_t	kr;

	handle = (handle_t)buf[1];
#if	MACH_ASSERT
	buf[2] = 0xc001babe;
	buf[3] |= 0xc1000000, buf[4] |= 0xc2000000, buf[5] |= 0xc3000000,
	buf[6] |= 0xc4000000;
#endif	/* MACH_ASSERT  */
	kr = KKT_RPC_REPLY(handle);
	if (kr != KKT_SUCCESS) {
		printf("KKT_RPC_REPLY returns 0x%x\n", kr);
		assert(0);
	}
}

/*
 * test_enqueue_tran
 *
 * Enqueue a transaction record on the work queue
 */
void
test_enqueue_tran(tran_request_t tran)
{
	tran_request_t	t;
	spl_t		s;

	tran->next = NULL_TRAN_REQ;
	s = splkkt();
	simple_lock(&work_queue_lock);
	t = test_work_queue;
	if (t == NULL_TRAN_REQ)
		test_work_queue = tran;
	else {
		while (t->next)
			t = t->next;
		t->next = tran;
	}
	simple_unlock(&work_queue_lock);
	splx(s);
	thread_wakeup_one((event_t) &kkt_receiver);
}


/*
 * test_tran_alloc
 *
 * Allocate a transaction record from the static pool
 */
tran_request_t
test_tran_alloc(void)
{
	tran_request_t	tran;
	spl_t		s;

	s = splkkt();
	simple_lock(&work_queue_lock);

	tran = tran_free_list;
	assert(tran);
	tran_free_list = tran->next;
	simple_unlock(&work_queue_lock);
	splx(s);

	return tran;
}

/*
 * test_tran_free
 *
 * Return a transaction record to the static free list.
 */
void
test_tran_free(tran_request_t tran)
{
	spl_t		s;

	s = splkkt();
	simple_lock(&work_queue_lock);
	tran->next = tran_free_list;
	tran_free_list = tran;
	simple_unlock(&work_queue_lock);
	splx(s);
}


/*
 * test_recv_callback
 *
 * Interrupt callback entry routine from the KKT layer.  It will
 * typically wakeup a receiver routine that will execute the
 * proper logic to suck the data from the remote node.
 */
void
test_recv_callback(int		error,
		   handle_t	handle,
		   request_block_t req,
		   boolean_t	thread_context)
{
        tran_request_t  tran;
	kern_return_t	kr;

#if LOOPBACK_ENG
	/*
	 * If we are in loopback mode, handoff to another
	 * thread or risk exhausting the stack
	 */
	tran = test_tran_alloc();
	tran->type = Recv_Engine;
	tran->request = req;
	tran->handle = handle;
	test_enqueue_tran(tran); /* will wakeup receiver */
#else	/* LOOPBACK_ENG */
	imsg_engine(handle, req);
#endif	/* LOOPBACK_ENG */
}

/*
 * Callback issued on sent data.  Called at interrupt level, so
 * queue resources to be released here.
 */
void
test_send_callback(int		error,
		   handle_t	handle,
		   request_block_t req,
		   boolean_t	thread_context)
{
#if LOOPBACK_ENG
        tran_request_t  tran;

	/*
	 * If we are in loopback mode, handoff to another
	 * thread or risk exhausting the stack
	 */
	tran = test_tran_alloc();
	tran->type = Send_Engine;
	tran->request = req;
	tran->handle = handle;
	test_enqueue_tran(tran); /* will wakeup receiver */
#else	/* LOOPBACK_ENG */
	imsg_engine(handle, req);
#endif	/* LOOPBACK_ENG */
}

/*
 * imsg_engine
 *
 * Interrupt level message engine.  Re-enqueue requests or dispose of
 * them as necessary.  This is symmetric for send and receive.
 */
void
imsg_engine(handle_t		handle,
	    request_block_t	req)
{
	struct msg_progress	*mp;
	request_block_t 	r;
	int			data_cnt;
	kern_return_t		kr;
	spl_t			s;

	mp = (struct msg_progress *)req->args[0];

	if (mp->data_remaining) {
		/*
		 * Send/Recv the chain again if there is more data
		 */
		r = req->next;
		s = splkkt();
		simple_lock(&mp->lock);

		/*
		 * As we get the the end of the data, we may possibly
		 * not send requests that are chained together: there
		 * is nothing wrong with this.
		 */
		while (mp->data_remaining && r) {
			if (r->data_length >= mp->data_remaining)
				break;
			mp->data_remaining -= r->data_length;
			kr = KKT_REQUEST(handle, r);
			assert(kr == KKT_SUCCESS);
			r = r->next;
		}

		/*
		 * Watch for partial (non page size) buffers
		 */
		if (req->data_length > mp->data_remaining) {
			req->data_length = mp->data_remaining;
			/*
			 * Notice that we don't process an SG list
			 * and fix up the lengths - the KKT SPEC
			 * says it will send entries while there is
			 * a data_length value.  This works because
			 * everything is page aligned unless the
			 * entire request is less than a page.
			 */
		}
		mp->data_remaining -= req->data_length;
		KKT_REQUEST(handle, req); /* send notification req */
		simple_unlock(&mp->lock);
		splx(s);
	} else {
		/*
		 * All done.  Release the request blocks, memory, and
		 * if we are truly done, update the timer.
		 */
		s = splkkt();
		simple_lock(&mp->lock);
		mp->chain_count--;

		if (mp->chain_count <= 0) {
			if (mp->sync == FALSE) {
				mp->sync = TRUE;
				thread_wakeup_one((event_t)mp);
			}
			timer_stop(mp->timer, mp->data_size);
			kr = KKT_HANDLE_FREE(handle);
			assert(kr == KKT_SUCCESS);
		}
		simple_unlock(&mp->lock);
		splx(s);

		test_release_request(req); /* cache the chain */
	}
}

/*
 * Put the request in the cache list.  This gets cleaned up by
 * new requests coming along and flushing out the old ones.
 */
void
test_release_request(
	   request_block_t req)
{
	int i;
	request_block_t r;

	while (req) {
		r = req->next;
		if (req->request_type & REQUEST_SG) {
			kfree((vm_offset_t)req->data.list,
			      kktd_chain_len * sizeof(struct io_scatter));
		}
		zfree(kkt_test_request_zone, (vm_offset_t)req);
		req = r;
	}
}

/*
 * Interrupt level memory release function.  Put the data onto the
 * garbage list to be collected later.  Note that this should only be
 * called from interrupt level routines, and that the garbage collection
 * agent masks interrupts as it removes an element so no further locking
 * is required.
 */
void
test_release(int	type,
	     garbage_t	addr,
	     void	*poly)
{
	spl_t s;

	s = splkkt();
	garbage_lock();
	addr->type = type;
	addr->next = garbage_list;
	addr->poly = poly;
	garbage_list = addr;
	garbage_unlock();
	splx(s);

	if (!kkt_driver_awake) {
		kkt_driver_awake = TRUE;
		thread_wakeup_one((event_t) &kkt_driver_awake);
	}
}

void
test_error(handle_t	handle,
	   int		error,
	   request_block_t req)
{
	printf("received error on handle 0x%x error %d req 0x%x\n", handle,
								    error,
								    req);
}

vm_offset_t
kkt_test_alloc(void		*opaque,
	       vm_size_t	size)
{
	return (kalloc(size));
}

void
kkt_test_free(void		*opaque,
	      vm_offset_t	data,
	      vm_size_t		size)
{
	kfree(data, size);
}



void		trbb_timers_init(void);


void
kkt_test_bootstrap(void)
{
	tran_request_t	tran;
	register	i;
	kern_return_t	kr;
	char		**tp;


	printf("Bootstrapping kkt test driver\n");
	kkt_test_request_zone = zinit(sizeof(struct request_block), 512*1024,
				  sizeof(struct request_block),
				  "kkt_test_request_zone");
	kkt_tran_request_zone = zinit(sizeof(struct tran_request), 512*1024,
				  sizeof(struct tran_request),
				  "kkt_tran_request_zone");
	simple_lock_init(&work_queue_lock, ETAP_KKT_TEST_WORK);
	simple_lock_init(&garbage_lock_data, ETAP_KKT_TEST_WORK);

	(void) kernel_thread(kernel_task,
			     kkt_driver_thread,
			     (char *) 0);
	(void) kernel_thread(kernel_task,
			     kkt_receiver_thread,
			     (char *) 0);

	tran = (tran_request_t)&tran_pool[0];
	for (i = 0; i < MAX_TRAN_RECORDS - 1; i++){
		tran->next = tran + 1;
		tran++;
	}
	tran->next = 0;
	tran_free_list = tran_pool;

	kr = KKT_CHANNEL_INIT(CHAN_TEST_KMSG,
			      SEND_HANDLE_COUNT,
			      RECV_HANDLE_COUNT,
			      test_deliver,
			      (kkt_malloc_t)kkt_test_alloc,
			      (kkt_free_t)kkt_test_free);
	kr = KKT_RPC_INIT(CHAN_TEST_RPC,
			  RPC_SENDERS,
			  RPC_RECEIVERS,
			  (kkt_rpc_deliver_t) rpc_deliver,
			  (kkt_malloc_t)kkt_test_alloc,
			  (kkt_free_t)kkt_test_free,
			  RPC_MAX_SIZE);
	assert(kr == KKT_SUCCESS);
	timer_set((unsigned int) kktd_recv_sm_timer, kktd_recv_sm_timer);

	for (tp = kktd_timers; *tp != 0; tp += 4) {
		timer_set((unsigned int)*(tp + 1), *(tp + 1));
		timer_set((unsigned int)*(tp + 3), *(tp + 3));
	}

	trbb_timers_init();

#if	PARAGON860
	kktd_chain_len = getbootint("DIPC_CHAIN_SIZE", 8);
#else	/*PARAGON860 */
	kktd_chain_len = 16;
#endif	/*PARAGON860 */
}

/*
 * kkt_start_test is invoked from the debugger (or whereever) and
 * will kick of the necessary tests to scrub KKT down.  In the full
 * implementation, there are three threads involved: the producer
 * (kkt_driver_thread) and a receiver (kkt_receiver_thread).
 */
boolean_t
kkt_start_test(
	int	new_target)
{
	kkt_test_target = new_target;
	thread_wakeup_one((event_t) &kkt_run_driver);

	return TRUE;
}

/*
 * Test producer thread
 */
void
kkt_driver_thread(void)
{
	extern int kkt_trace;
	int blk;


	while(1) {
		assert_wait((event_t) &kkt_run_driver, FALSE);
		thread_block((void (*)) 0);
		kkt_test_driver(kkt_test_target);
		kkt_run_driver = FALSE;
	}
}


/*
 * Test receiver thread
 *
 * This thread acts as the waiting receiver and will pull the
 * data from the remote *node*.
 */
void
kkt_receiver_thread(void)
{
	tran_request_t	tran;
	spl_t		s;
	kkt_return_t	kktr;
	kkt_test_client	func;

	for (;;) {
		/*
		 * Get the necessary info from the transaction work queue,
		 * which is used as a meta_kmsg here.
		 */
		s = splkkt();
		simple_lock(&work_queue_lock);
		tran = test_work_queue;
		if (tran == NULL_TRAN_REQ) {
			assert_wait((event_t) &kkt_receiver, FALSE);
			simple_unlock(&work_queue_lock);
			splx(s);
			thread_block((void (*)) 0);
			continue;
		}
		assert(tran != NULL_TRAN_REQ);
		test_work_queue = tran->next;
		simple_unlock(&work_queue_lock);
		splx(s);

		switch(tran->type) {

		    case Recv_Connect:
			kktr = test_recv_connect(tran);
			assert(kktr == KKT_SUCCESS);
			break;

		    case Recv_Engine:
			imsg_engine(tran->handle, tran->request);
			break;

		    case Recv_Bigbuf:
			(void) test_recv_big_buf(tran);
			break;

		    case Send_Engine:
			imsg_engine(tran->handle, tran->request);
			break;

		    case Client:
			func = (kkt_test_client) tran->msg_size;
			(*func)();
			kktr = KKT_CONNECT_REPLY(tran->handle,
						 (kern_return_t) KKT_SUCCESS,
						 (kern_return_t) KKT_SUCCESS);
			assert(kktr == KKT_SUCCESS);
			break;

		    default:
			printf("Invalid transaction element: 0x%x\n", tran);
			show_tran_req(tran);
			assert(0);
		}
		test_tran_free(tran);
	}
}

char *kkt_recv_max = "KKT Recv max";

typedef struct trbb_timer {
	unsigned int	size;
	char		*timer;
} trbb_timer_t;

trbb_timer_t	trbb_timers[] = {
	{4 K,	"KKT Recv 4K"},
	{8 K,	"KKT Recv 8K"},
	{16 K,	"KKT Recv 16K"},
	{32 K,	"KKT Recv 32K"},
	{64 K,	"KKT Recv 64K"},
	{128 K,	"KKT Recv 128K"},
	{256 K,	"KKT Recv 256K"},
	{512 K,	"KKT Recv 512K"},
	{1 M,	"KKT Recv 1M"},
	{2 M,	"KKT Recv 2M"},
#if	PARAGON860
	{3 M,	"KKT Recv 3M"},
#endif	/* PARAGON860 */
	{4 M,	"KKT Recv 4M"},
#if	PARAGON860
	{5 M,	"KKT Recv 5M"},
	{6 M,	"KKT Recv 6M"},
	{7 M,	"KKT Recv 7M"},
#endif	/* PARAGON860 */
	{8 M,	"KKT Recv 8M"},
#if	PARAGON860
	{9 M,	"KKT Recv 9M"},
#endif	/* PARAGON860 */
	{16 M,	"KKT Recv 16M"},
	-1
};


void
trbb_timers_init()
{
	trbb_timer_t	*tp;

	for (tp = trbb_timers; tp->size != -1; ++tp)
		timer_set((unsigned int) tp->timer, tp->timer);
}


unsigned int
trbb_timer_start(
	unsigned int	size)
{
	trbb_timer_t	*tp;

	for (tp = trbb_timers; tp->size != -1; ++tp)
		if (tp->size == size) {
			timer_start((unsigned int) tp->timer);
			return (unsigned int) tp->timer;
		}
	return 0;
}


unsigned int	trbb_cur_timer = 0;

kkt_return_t
test_recv_big_buf(
	tran_request_t	tran)
{
	struct request_block req;
	handle_t	handle;
	kkt_return_t	kktr;
	boolean_t	wait;
	extern void	dtest_kkt_test_notify(void);
	unsigned int	t;

	if (!dipc_test_verify_buffer("test_recv_big_buf"))
		return KKT_RESOURCE_SHORTAGE;

	trbb_cur_timer = trbb_timer_start(tran->size);
	handle = (handle_t)tran->handle;

	/*
	 * Pull the data in
	 */
	timer_set((unsigned int)&req, kkt_recv_max);
	bzero((char *)&req, sizeof(struct request_block));
	req.data.address = dipc_test_region;
	req.data_length = tran->size;
	req.request_type = REQUEST_RECV;
	req.args[0] = (unsigned int)&wait;
	req.callback = (kkt_callback_t)dtest_kkt_test_notify;
	wait = TRUE;

	timer_start((unsigned int)&req);

	kktr = KKT_REQUEST(handle, &req);
	while (wait) {
		assert_wait((event_t) &wait, TRUE);
		thread_block((void (*)) 0);
	}

	KKT_HANDLE_FREE(handle);

	return kktr;
}

/*
 * Set up an initial connection, request the data be sent over
 */
kkt_return_t
test_recv_connect(
	tran_request_t	tran)
{
	request_block_t	chains[10], *cp;
	handle_t	handle;
	kkt_return_t	kktr = KKT_SUCCESS;
	vm_size_t	msg_size;
	unsigned int	rtype;
	char		*timer;
	spl_t		s;
	struct script 	*sp;

	handle = (handle_t)tran->handle;

	/*
	 * Pull the data in
	 */
	msg_size = tran->msg_size;
	sp = script;
	for (sp = script; sp->size > 0; sp++) {
		if (sp->size == msg_size)
			break;
	}
	
	
	rtype = REQUEST_RECV;
	if (tran->test_type == TEST_SG) {
		rtype |= REQUEST_SG;
		rmsg_progress.timer =
			(unsigned int)kktd_timers[(sp - script) * 4 + 3];
	} else {
		if (msg_size < 100)
			rmsg_progress.timer = (unsigned int)kktd_recv_sm_timer;
		else
			rmsg_progress.timer = 
				(unsigned int)kktd_timers[(sp - script)*4 + 1];
	}
	rmsg_progress.data_size = msg_size;
	rmsg_progress.data_remaining = msg_size;

	(void)test_allocate_chains(chains,
				   msg_size, 
				   rtype,
				   &rmsg_progress,
				   test_recv_callback);

	simple_lock_init(&rmsg_progress.lock, ETAP_KKT_TEST_MP);

	timer_start(rmsg_progress.timer);
	/*
	 * Use the imsg engine to kick things off: raise spl to prevent
	 * some infinitely fast transport preventing us from ever 
	 * submitting all of the requests.
	 */
	s = splkkt();
	for (cp = chains; *cp; cp++) {
		imsg_engine(handle, *cp);
	}
	splx(s);

	return kktr;
}

/*
 * kkt_test_support
 *
 * Garbage collection thread - disposes of memory in thread context.
 * An interrupt level thread need only queue the resources on the 
 * global garbage_list and it will be free asynchronously by this
 * routine.
 */
void
kkt_test_support(void)
{
	spl_t s;
	garbage_t	garbage;

	for(;;) {
		s = splkkt();
		garbage_lock();
		garbage = garbage_list;
		if (garbage)
		    garbage_list = garbage->next;
		garbage_unlock();
		splx(s);
		
		while (garbage) {
			if (garbage->type == KMEM) {
				kfree((vm_offset_t)garbage,
				      (vm_size_t)garbage->poly);
			} else if (garbage->type == ZMEM) {
				zfree((zone_t)garbage->poly,
				      (vm_offset_t)garbage);
			}
			s = splkkt();
			garbage_lock();
			garbage = garbage_list;
			if (garbage)
			    garbage_list = garbage->next;
			garbage_unlock();
			splx(s);
		}
		kkt_driver_awake = FALSE;
		assert_wait((event_t) &kkt_driver_awake, FALSE);
		thread_block((void (*))0);
	}
}

void
show_tran_req(tran_request_t req)
{
	printf("trans_request: 0x%x   next 0x%x\n", req, req->next);
	printf("State %s, request_block=0x%x\n",
	       req->type == RPC_send ? "RPC_send":
	       req->type == RPC_recv ? "RPC_recv":
	       req->type == Recv_Connect ? "Recv_Connect":
	       req->type == Recv_Bigbuf ? "Recv_Bigbuf":
	       req->type == Recv_Engine ? "Recv_Engine":
	       req->type == Recv ? "Recv":
	       "unknown",
	       req->request);
	printf("callback=0x%x, handle=0x%x size=0x%x\n",
	       (int)req->callback, (int)req->handle, req->size);
	printf("test_type 0x%x msg_size 0x%x\n", 
	       req->test_type, req->msg_size);
}
