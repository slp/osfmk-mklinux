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
 * flipc/flipc_message_engine.c
 *
 * The KKT flipc message engine implementation.
 */

#include <mach/flipc_cb.h>
#include <mach/flipc_locks.h>
#include <mach/kkt_request.h>
#include <flipc/flipc.h>
#include <machine/flipc_page.h>
#include <mach_kdb.h>
#include <mach_kgdb.h>
#if MACH_KDB
#include <ddb/db_output.h>
#endif
#include <flipc_trace.h>
#include <flipc_ucheck.h>
#include <flipc_kkt.h>

/*
 * This is exactly the file for inline functions, so if I can have them,
 * I will.
 */
#ifdef __GNUC__
#define INLINE __inline__
#else /* __GNUC__ */
#define INLINE
#endif /* __GNUC__ */

/*
 * Static functions defined in this file as helper functions for
 * the main entry points.  Most of these will be inline if that is
 * possible.
 */
static flipc_return_t flipcme_attempt_send(flipc_endpoint_t endpoint);
static int flipcme_deliver_wakeup(flipc_epgroup_t epgroup, int send_side_p);
static int flipcme_incr_corruptcb(void);
void	flipcme_receive_kkt_entry(channel_t chan, rpc_buf_t buf,
				  boolean_t thread_context_p);
void	flipcme_low_level_test(int node, int test_size, int *result);

#if MACH_KDB
void flipcme_print_dropvars(void);
#endif

/* Define a maximum for the data buffer size, mostly so that the tests
   work (ie. a given number of buffers takes up a given space in the
   communications buffer).  Initialization is actually taken from the
   variable, to allow the value to be modified (eg. by the debugger)
   at boot time. */
#define FLIPCME_MAX_DATA_BUFFER_SIZE 128
unsigned int flipcme_max_data_buffer_size = FLIPCME_MAX_DATA_BUFFER_SIZE;

/* Define the endpoint index that will be taken as a null index when
   it is seen on the receive side.  */
#define FLIPCME_NULL_ENDPOINT_IDX 0xffff

/*
 * If we are doing debug, reserve space large enough to hold a bitvector
 * with one bit for every buffer (for reference counting).  This is
 * initialized in flipcme_system_init, and stays around throughout system
 * boot.
 */
#if MACH_KDB || MACH_KGDB
u_long *flipc_debug_buffer_bitvec = (u_long *) 0;
#endif

/*
 * Message engine internal serialization locks.
 *
 * On a dedicated co-processor implementation, the ME would be singled
 * threaded and there would be no race conditions.  However, in the
 * current device driver implementation, the ME may have multiple threads
 * (depending on the specifics of the transport and how often the user
 * invokes the device driver).  We can divide the possible ME internal
 * sychronization issues into two: a) between send and receive threads,
 * and b) multiple threads within either the send or the receive side.
 * This requires multiple forms of locking:
 *
 * 	*) If we may have multiple threads in either context, before
 * 	   doing anything with the communication buffer we need to
 * 	   reduce this down to a single thread (^).  We do this in
 * 	   different manners on the two sides:
 *
 * 		*) On the send side we grab a gateway lock (see
 * 	   	   overview file).  This is always required, as the
 * 	   	   send side may block and yield the processor in a
 * 	   	   KKT_RPC.
 *
 * 		*) On the receive side, a gateway lock won't work, as
 * 	   	   it could lead to attempts to sleep from interrupt
 * 	   	   context (gateway lock taken on CPU 1 of a multiple
 * 	   	   CPU system, interrupt occurs on CPU 2, can't take
 * 	   	   lock, attempts to sleep).  Thus we need to have a
 * 	   	   simple, spin lock, coupled with blocking of
 * 	   	   interrupts if the receive side may execute in
 * 	   	   interrupt context.  Since the receive side doesn't
 * 	   	   block, and doesn't take very long delivering its
 * 	   	   message, this is acceptable.
 *
 * 	*) We need to prevent the (now single) send and receive side
 * 	   threads from accessing shared data structures at the same
 * 	   time.  This can be done through a simple lock, and will
 * 	   take the above path to deal with modifying the endpoint
 * 	   group structures.
 *
 * This leads to five controlling variables, set at system
 * initialization time:
 *
 * 	*) take_send_gateway_lock -- Take always, since the
 * 	   send side may block (KKT_RPC).
 * 	*) take_receive_simple_lock -- Always (will get
 * 	   optimized away if NCPUS == 1).
 * 	*) Interrupt block on receive -- Set if receive
 * 	   upcalls are mixed (you're ok if receives are
 * 	   always in interrupt context because it is already
 * 	   blocked, and you're ok if they're in thread context
 * 	   because there's nothing to block).
 * 	*) Interrupt block on shared access -- Set if receive
 * 	   upcalls might be in interrupt context.
 * 	*) simple lock on shared access -- Always set (will
 * 	   get optimized away if NCPUS == 1)
 *
 * and eight macros (which will take action based on the above
 * variables):
 * 	*) Send serialize lock/unlock (used for turning
 * 	   multiple send threads into a single one).
 * 	*) Receive serialize lock/unlock (ditto for receive).
 * 	*) send shared serialize lock/unlock (used to serialize
 * 	   access to the shared data structures from the
 * 	   send side)
 * 	*) Receive shared serialize lock/unlock (ditto for receive).
 *
 * The locks serializing access to the shared data structures
 * need to be different because the send side may have to
 * block interrupts.
 *
 * Because of the possibility of priority needing to be raised and
 * lowered as part of the locking processe, each of these macros will
 * take a single integer variable.  This variable must not be used for
 * any other purpose, and it must be passed to both of the lock/unlock
 * pair for which it is used.
 *
 * The other thing to consider is that this really should be written
 * targetted at the real-time kernel (where the kernel internals are
 * pre-emptable), since we want to move this implementation over onto
 * that base ASAP (to use user-level semaphores and locks).  However, I
 * believe that the entire above sketch also works under pre-emption, as
 * I'm told that you can't be pre-empted while holding a simple lock.
 *
 * (^) The decision not to actually try to make the implementation truely
 * multi-threaded is based on time constraints and the fact that this
 * will be a prototype from which we don't really expect to get close to
 * acceptable performance (for a short-message system, which this is, you
 * want very fast dispatch/short latency between dispatch and arrival
 * above all.  Kernel involvement makes that basically impossible).
 */

/*
 * Initialized in flipcme_system_init();
 */
/* For serializing send-side accesses and recv-side accesses.  */
static struct flipc_gateway_lock send_gateway_lock;
decl_simple_lock_data(static,recv_simple_lock)
decl_simple_lock_data(static,shared_simple_lock)

/*
 * Control variables for use of send and receive side serialization
 * macros.  We determine whether or not they should be used based on
 * a complicated series of conditions (partially the KKT implementation,
 * partially the system configuration).
 *
 * Initialized in flipcme_system_init();
 */
static int take_send_gateway_lock = 0;
static int take_receive_simple_lock = 0;
static int take_shared_simple_lock = 0;
static int interrupt_block_on_receive = 0;
static int interrupt_block_on_shared = 0;

/*
 * Macros for all of send/receive/shared serialize lock/unlock
 * What these include depends on the above variables.
 *
 * They each take an argument (sv) which is an integer variable.
 * This variable must be common to each lock/unlock pair called in
 * a function, and different between different lock/unlock pairs.
 *
 * These routines should work in a pre-emptible kernel as written.
 * Gatelocks will work under pre-emption (see flipc.h) and holding a
 * simple lock prevents pre-emption.
 */

#if	PARAGON860 && !FLIPC_KKT
#define SEND_SERIALIZE_LOCK(sv)			(void *)0
#define SEND_SERIALIZE_UNLOCK(sv)		(void *)0
#define RECV_SERIALIZE_LOCK(sv)			(void *)0
#define RECV_SERIALIZE_UNLOCK(sv)		(void *)0
#define SEND_SHARED_SERIALIZE_LOCK(sv)		(void *)0
#define SEND_SHARED_SERIALIZE_UNLOCK(sv)	(void *)0
#define RECV_SHARED_SERIALIZE_LOCK(sv)		(void *)0
#define RECV_SHARED_SERIALIZE_UNLOCK(sv)	(void *)0
#define	NODE_SELF()	node_self()

#else	/* PARAGON860 && !FLIPC_KKT */

#define SEND_SERIALIZE_LOCK(sv)				\
do {							\
    if (take_send_gateway_lock)				\
	FLIPC_ACQUIRE_GATELOCK(send_gateway_lock);	\
} while (0)

#define SEND_SERIALIZE_UNLOCK(sv)			\
do {							\
    if (take_send_gateway_lock)				\
	FLIPC_RELEASE_GATELOCK(send_gateway_lock);	\
} while (0);

#define RECV_SERIALIZE_LOCK(sv)				\
do {							\
    if (interrupt_block_on_receive)			\
	(sv) = splkkt();				\
							\
    if (take_receive_simple_lock)			\
	simple_lock(&recv_simple_lock);			\
} while (0)

#define RECV_SERIALIZE_UNLOCK(sv)			\
do {							\
    if (take_receive_simple_lock)			\
	simple_unlock(&recv_simple_lock);		\
							\
    if (interrupt_block_on_receive)			\
	splx(sv);					\
} while (0)

#define SEND_SHARED_SERIALIZE_LOCK(sv)			\
do {							\
    if (interrupt_block_on_shared)			\
	(sv) = splkkt();				\
							\
    if (take_shared_simple_lock)			\
	simple_lock(&shared_simple_lock);		\
} while (0)

#define SEND_SHARED_SERIALIZE_UNLOCK(sv)		\
do {							\
    if (take_shared_simple_lock)			\
	simple_unlock(&shared_simple_lock);		\
							\
    if (interrupt_block_on_shared)			\
	splx(sv);					\
} while (0)

#define RECV_SHARED_SERIALIZE_LOCK(sv)			\
do {							\
    if (take_shared_simple_lock)			\
	simple_lock(&shared_simple_lock);		\
} while (0)

#define RECV_SHARED_SERIALIZE_UNLOCK(sv)		\
do {							\
    if (take_shared_simple_lock)			\
	simple_unlock(&shared_simple_lock);		\
} while (0)

#define	NODE_SELF()	KKT_NODE_SELF()
#endif	/* PARAGON860 && !FLIPC_KKT */

/*
 * Is the buffer active?  Must be checked by both send
 * and receive (after it's done the appropriate serialization) to
 * make sure that it's allowed to do stuff.
 *
 * Set in flipcme_cb_init(); cleared in flipcme_cb_teardown();
 */
static int communications_buffer_active = 0;

/*
 * We cache some buffer control information here--a copy of the
 * buffer control structure, and a pointer to the beginning of
 * an array for each data object.
 *
 * Note that the freelist isn't valid in the ME's cached copy.
 */
static struct flipc_comm_buffer_ctl cb_ctl;
static flipc_endpoint_t endpoint_array;
static flipc_epgroup_t epgroup_array;
static flipc_data_buffer_t data_buffer_array; /* Can't use ptr arith.  */
static flipc_cb_ptr *bufferlist_array;

/*
 * Macros for paranoid error checking of values that we read from the
 * control buffer.  Uses the above values and
 * flipcme_data_buffer_size.  If FLIPC_UCHECK is not defined, these
 * always return True (ie. the value is ok) and code based on if's of
 * them will be eliminated by the compiler as dead code.
 */
#if FLIPC_UCHECK
/* CBPTR checks.  Each of these checks to see if a) the cbptr points
   into the range of locations in the CB valid for this data type, and b)
   if the pointer is aligned correctly based on the data type.
   Note that a null CBPTR is not valid as far as these macros
   are concerned; you can't use them at a place in the code where a null
   CBPTR may be valid.  */
#define CBPTR_VALID_ENDPOINT_P(cbptr)		\
((cbptr) >= cb_ctl.endpoint.start		\
 && ((cbptr)					\
     < (cb_ctl.endpoint.start			\
	+ (cb_ctl.endpoint.number		\
	   * sizeof(struct flipc_endpoint))))	\
 && !(((cbptr)					\
       - cb_ctl.endpoint.start)			\
      % sizeof(struct flipc_endpoint)))

#define CBPTR_VALID_EPGROUP_P(cbptr)		\
((cbptr) >= cb_ctl.epgroup.start		\
 && ((cbptr)					\
     < (cb_ctl.epgroup.start			\
	+ (cb_ctl.epgroup.number		\
	   *sizeof(struct flipc_epgroup))))	\
 && !(((cbptr)					\
       - cb_ctl.epgroup.start)			\
      % sizeof(struct flipc_epgroup)))

#define CBPTR_VALID_BUFFERLIST_P(cbptr)		\
((cbptr) >= cb_ctl.bufferlist.start		\
 && ((cbptr)					\
     < (cb_ctl.bufferlist.start			\
	+ (cb_ctl.bufferlist.number		\
	   * sizeof(struct flipc_epgroup))))	\
 && !(((cbptr)					\
       - cb_ctl.bufferlist.start)		\
      % sizeof(flipc_cb_ptr)))

#define CBPTR_VALID_DATA_BUFFER_P(cbptr)	\
((cbptr) >= cb_ctl.data_buffer.start		\
 && ((cbptr)					\
     < (cb_ctl.data_buffer.start		\
	+ (cb_ctl.data_buffer.number		\
	   * cb_ctl.data_buffer_size)))		\
 && !(((cbptr)					\
       - cb_ctl.data_buffer.start)		\
      % flipcme_data_buffer_size))

/* Index checks -- just making sure the index is greater than or
   equal to 0 and less than the number of the type.
   Slightly sleazy use of a macro.  */
#define INDEX_VALID_P(idx, type) ((idx) >= 0 && (idx) < cb_ctl.type.number)

/* Node check -- is this a valid node?    */
#define NODE_VALID_P(dest_node) KKT_NODE_IS_VALID(dest_node)

#else	/* FLIPC_UCHECK */

#define CBPTR_VALID_ENDPOINT_P(cbptr) 1
#define CBPTR_VALID_EPGROUP_P(cbptr) 1
#define CBPTR_VALID_BUFFERLIST_P(cbptr) 1
#define CBPTR_VALID_DATA_BUFFER_P(cbptr) 1
#define INDEX_VALID_P(idx, type) 1
#define NODE_VALID_P(dest_node) 1

#endif	/* FLIPC_UCHECK */

/*
 * Debugging variable which enables printf tracing of actions.
 */
#if FLIPC_TRACE
int flipc_me_trace = 0;
#endif

/*
 * What is the message data buffer size?  Based on KKT; initialized in
 * flipcme_system_init()
 */
static int flipcme_data_buffer_size = 0;

/*
 * Constants for resources requested from KKT.
 *
 * We only need one send handle, but several KKT implementations bomb
 * if we get too many incoming messages, then we're in trouble, so
 * lets ask for several receive handles.
 */
#define FLIPC_KKT_SEND_HANDLES 1
#define FLIPC_KKT_RECV_HANDLES 5

/*
 * Keep the send handle around.  Initialized in flipcme_system_init()
 */
handle_t flipcme_kkt_send_handle = (handle_t) 0;
void *flipcme_kkt_send_buffer = (void *) 0;

/*
 * Counters for various types of dropped messages and other bad things.
 */
/*
 * Almost all dropped message errors detected are recorded directly
 * in the comm buffer ctl struct.  The exceptions are
 * messages dropped because the comm buffer is inactive (don't have a
 * comm buffer to put them in) and times when the comm buffer data
 * structures are found corrupted (nastier things happen then).
 */
int flipcme_dropmsg_inactive = 0; /* Messages dropped because of an
				     inactive buffer.  */
int flipcme_corruptcb = 0;	/* Number of times the ME has noticed
				   that the communications buffer has been
				   corrupted.  This is BAD.  */
int flipcme_wakeupvars_off = 0;	/* Number of times the ME found
				   wakeups requested < wakeups delivered.
				   Not a big enough error to shut down
				   for.  */

/*
 * Function to increment the corruptcb counter.  This is done as a
 * function so that we can set breakpoints in it and put a more
 * drastic response in it.
 */
static int flipcme_incr_corruptcb(void)
{
    flipcme_corruptcb++;
#if MACH_KDB || MACH_KGDB
    Debugger("flipc: corruption detected in comm buffer.");
    return 0;
#else
    printf("flipc: corruption detected in comm buffer; shutting down.");
    communications_buffer_active = 0; /* Isn't checked for consistency on
					 teardown.  */
    return 1;
#endif
}

/*
 * KFR initialization access for buffer active variables (and
 * CB base pointers).
 */
int flipcme_buffer_active()
{
    return communications_buffer_active;
}

void flipcme_make_buffer_active(int endpoint_start, int endpoints,
				int epgroup_start, int epgroups,
				int bufferlist_start, int bufferlist_elements,
				int data_buffer_start, int data_buffers)
{
    cb_ctl.data_buffer_size = flipcme_data_buffer_size;

    /* Endpoints.  */
    cb_ctl.endpoint.start = endpoint_start;
    cb_ctl.endpoint.number = endpoints;
    endpoint_array = FLIPC_ENDPOINT_PTR(endpoint_start);

    /* Endpoint groups.  */
    cb_ctl.epgroup.start = epgroup_start;
    cb_ctl.epgroup.number = epgroups;
    epgroup_array = FLIPC_EPGROUP_PTR(epgroup_start);

    /* Bufferlists.  */
    cb_ctl.bufferlist.start = bufferlist_start;
    cb_ctl.bufferlist.number = bufferlist_elements;
    bufferlist_array = FLIPC_BUFFERLIST_PTR(bufferlist_start);

    /* Buffers themselves.  */
    cb_ctl.data_buffer.start = data_buffer_start;
    cb_ctl.data_buffer.number = data_buffers;
    data_buffer_array = FLIPC_DATA_BUFFER_PTR(data_buffers);

    /* Initialize synchronization variables (used to confirm when they change
       state in the CB).  All we need is the request_sync value, since we can
       trust the respond sync value (the AIL doesn't change it).  */
    cb_ctl.sail_request_sync =
	((flipc_comm_buffer_ctl_t)flipc_cb_base)->sail_request_sync;

    /* Set (both locally and in the CB) the local node address.  */
    cb_ctl.local_node_address =
	((flipc_comm_buffer_ctl_t) flipc_cb_base)->local_node_address =
	    NODE_SELF();

    /* Set the null destination in the CB.  */
    cb_ctl.null_destination =
	((flipc_comm_buffer_ctl_t) flipc_cb_base)->null_destination =
	    FLIPC_CREATE_ADDRESS(cb_ctl.local_node_address,
				 FLIPCME_NULL_ENDPOINT_IDX);

    /* Set msgdrop inactive in the CB.  */
    ((flipc_comm_buffer_ctl_t) flipc_cb_base)->sme_error_log.msgdrop_inactive
	= flipcme_dropmsg_inactive;

    /* Trace, if wanted.  */
#if FLIPC_TRACE
    if (flipc_me_trace)
	printf("Flipc ME: making buffer active.\n");
#endif

    /* About to do a synchronization variable access, even if it's only with
       other ME threads (KKT implementation specific).  */
    WRITE_FENCE();

    SYNCVAR_WRITE(communications_buffer_active = 1);
}

/*
 * Wrappers around kernel allocation routines to hand to KKT since
 * KKT really wants to tell us what channel it's allocating on.
 * Shamelessly swiped from dipc.
 */
static void *flipc_alloc(channel_t chan, vm_size_t size)
{
    return (void *) kalloc(size);
}

static void flipc_free(channel_t chan, vm_offset_t data, vm_size_t size)
{
    kfree(data, size);
}

/*
 * Called at system init by flipc_system_init()
 */
void flipcme_system_init(
	int *message_buffer_size,		/* OUT */
	int *transport_error_size)		/* OUT */
{
    kkt_properties_t kktprops;
    kkt_return_t kktr;

    /* Mark the buffer inactive.  */
    communications_buffer_active = 0;

    /* Initialize all locks used by the ME.  */
    FLIPC_INIT_GATELOCK(send_gateway_lock);
    simple_lock_init(&recv_simple_lock, ETAP_MISC_FLIPC);
    simple_lock_init(&shared_simple_lock, ETAP_MISC_FLIPC);

#if	PARAGON860 && !FLIPC_KKT
    flipcme_data_buffer_size = getbootint("FLIPC_BUFFER_SIZE",
					  flipcme_max_data_buffer_size);
    if (flipcme_data_buffer_size > 128) {
	    /* align on 32 (0x20) byte boundary */
	    flipcme_data_buffer_size =
		(flipcme_data_buffer_size + 0x1f) & ~(0x1f);
    } else if (flipcme_data_buffer_size > 64) {
	    flipcme_data_buffer_size = 128;
    }
    printf("FLIPC data buffer size: %d\n", flipcme_data_buffer_size);
    take_send_gateway_lock = 0;
    take_receive_simple_lock = 0;
    take_shared_simple_lock = 0;
    interrupt_block_on_receive = 0;
    interrupt_block_on_shared = 0;
    mcmsg_flipc_init();
#else	/* PARAGON860 && !FLIPC_KKT */
    /* Get KKT propertiesa and initialize variables based on them.  */
    kktr = KKT_PROPERTIES(&kktprops);
    if (kktr != KKT_SUCCESS)
	panic("flipcme_system_init: KKT properties failed");

    /* We want the biggest integer that is smaller than
       kktprops.kkt_buffer_size but evenly divides a page.
       We assume flipcme_max_data_buffer_size is never set higher
       than a page.  */

    for (flipcme_data_buffer_size = 1;
	 flipcme_data_buffer_size <= (kktprops.kkt_buffer_size >> 1);
	 flipcme_data_buffer_size <<= 1)
	;

    if (flipcme_data_buffer_size > flipcme_max_data_buffer_size)
	flipcme_data_buffer_size = flipcme_max_data_buffer_size;

    switch (kktprops.kkt_upcall_methods) {
      case KKT_THREAD_UPCALLS:
	take_send_gateway_lock = 1;
	take_receive_simple_lock = 1;
	take_shared_simple_lock = 1;
	interrupt_block_on_receive = 0;
	interrupt_block_on_shared = 0;
	break;
      case KKT_MIXED_UPCALLS:
	take_send_gateway_lock = 1;
	take_receive_simple_lock = 1;
	take_shared_simple_lock = 1;
	interrupt_block_on_receive = 1;
	interrupt_block_on_shared = 1;
	break;
      case KKT_INTERRUPT_UPCALLS:
	take_send_gateway_lock = 1;
	take_receive_simple_lock = 1;
	take_shared_simple_lock = 1;
	interrupt_block_on_receive = 0;
	interrupt_block_on_shared = 1;
	break;
    }

    /* Initialize KKT for FLIPC.  */
    kktr = KKT_RPC_INIT(CHAN_FLIPC,
			FLIPC_KKT_SEND_HANDLES,
			FLIPC_KKT_RECV_HANDLES,
			&flipcme_receive_kkt_entry,
			&flipc_alloc,
			&flipc_free,
			flipcme_data_buffer_size);
    if (kktr != KKT_SUCCESS)
	panic("flipcme_system_init: Could not initialize kkt.");

    kktr = KKT_RPC_HANDLE_ALLOC(CHAN_FLIPC, &flipcme_kkt_send_handle,
				flipcme_data_buffer_size);
    if (kktr != KKT_SUCCESS)
	panic("flipcme_system_init: Could not allocate kkt send channel.");

    /* Squirrel away location of buffer also.  */
    kktr = KKT_RPC_HANDLE_BUF(flipcme_kkt_send_handle,
			      &flipcme_kkt_send_buffer);
    if (kktr != KKT_SUCCESS)
	panic("flipcme_system_init: Could not get address of kkt send buffer.");
#endif	/* PARAGON860 && !FLIPC_KKT */

#if MACH_KDB || MACH_KGDB
    /* Initialize some space for the debug routines to use.
       A bit vector with one bit for each possible buffer.  */
    flipc_debug_buffer_bitvec =
	(u_long *) kalloc((flipc_cb_length
			   / (flipcme_data_buffer_size * sizeof(u_long) * 8))
			  + 1);
#endif

    *message_buffer_size = flipcme_data_buffer_size;
    *transport_error_size = 
#if	PARAGON860 && !FLIPC_KKT
	    /*
	     * Error processing is not well thought out.  This size
	     * is to establish an array to cover the range of possible
	     * error numbers returned by the transport: elements in 
	     * the array are incremented as error occur.  For now,
	     * stick some arbitrary number in there until we figure out
	     * real error processing.
	     */
	    0x10;
#else	/* PARAGON860 && !FLIPC_KKT */
	    (KKT_MAX_ERRNO - KKT_MIN_ERRNO + 1) * sizeof(int);
#endif	/* PARAGON860 && !FLIPC_KKT */
}

/*
 * Communications buffer teardown routine.  Called from KFR; it's the
 * KFR's responsibility to make sure that we aren't asked to teardown
 * the buffer when someone's uing it, or when it's already been
 * torndown or hasn't been initialized.
 */
void flipcme_cb_teardown()
{
    int sv;			/* Shared variable for syncs.  */

    SYNCVAR_WRITE(communications_buffer_active = 0);

    /* Synchronize with the send and receive sides to make sure that
       they see the change in buffer active status.  */
    SEND_SERIALIZE_LOCK(sv); SEND_SERIALIZE_UNLOCK(sv);
    RECV_SERIALIZE_LOCK(sv); RECV_SERIALIZE_UNLOCK(sv);

    /* Invoke the duty loop one more time (telling it to ignore
       buffer active status) to push out all waiting send messages.
       This may need to be different in different implementations;
       we need some way to guarantee that all messages queued before
       this function was invoked have been sent.  */
    flipcme_duty_loop(1);

    /* Trace, if wanted.  */
#if FLIPC_TRACE
    if (flipc_me_trace)
	printf("Flipc ME: buffer torn down.\n");
#endif
}

/*
 * Take care of delivering a wakeup.  Whichever side we're on (send or
 * receive) is assumed to be serialized, but serialization of the shared
 * lock is our responsibility.  The epgroup passed is assumed to be valid.
 * True (1) is returned if a wakeup is delivered, and False (0) is
 * returned if not.
 */
INLINE static int flipcme_deliver_wakeup(flipc_epgroup_t epgroup,
					 int send_side_p)
{
    int shared_sv;

    /* Is the structure active?  If not, I don't do anything.  */
    if (!epgroup->sail_enabled)
	return 0;

    /* Is there a specific wakeup request?  */
    if (epgroup->sail_wakeup_req <= epgroup->pme_wakeup_del) {
#if FLIPC_UCHECK
	if (epgroup->sail_wakeup_req < epgroup->pme_wakeup_del)
	    flipcme_wakeupvars_off++;
#endif
	return 0;
    }

    /* Serialize access to the shared structures.  */
    if (send_side_p)
	SEND_SHARED_SERIALIZE_LOCK(shared_sv);
    else
	RECV_SHARED_SERIALIZE_LOCK(shared_sv);

    /* Is it time for a wakeup?  If not, return.  */
    /* Use < in comparison because the AIL may have changed the
       msgs_per_wakeup on us.  */
    if (++(epgroup->pme_msgs_since_wakeup)
	< epgroup->sail_msgs_per_wakeup) {
	if (send_side_p)
	    SEND_SHARED_SERIALIZE_UNLOCK(shared_sv);
	else
	    RECV_SHARED_SERIALIZE_UNLOCK(shared_sv);
	return 0;
    }

    /* Clean up, do the wakeup, and return.  */
    epgroup->pme_msgs_since_wakeup = 0;
    epgroup->pme_wakeup_del++;
    if (send_side_p)
	SEND_SHARED_SERIALIZE_UNLOCK(shared_sv);
    else
	RECV_SHARED_SERIALIZE_UNLOCK(shared_sv);

#if	PARAGON860 && !FLIPC_KKT
    mcmsg_flipc_semaphore(epgroup - epgroup_array);
#else	/* PARAGON860 && !FLIPC_KKT */
    flipckfr_wakeup_epgroup(epgroup - epgroup_array);
#endif	/* PARAGON860 && !FLIPC_KKT */

    return 1;
}

#if	PARAGON860 && !FLIPC_KKT
/*
 * The entry point into the message engine from the NIC
 * takes an incoming message and does the right thing with it.
 */
void
flipcme_receive_send(
	unsigned int	hdr1,
	unsigned int	hdr2)
{
    unsigned int  endpoint_idx = hdr2 & 0xffff;
    flipc_endpoint_t endpoint;
    u_long dpb_or_enabled;
    flipc_cb_ptr acquire, release, process;
    flipc_cb_ptr bufferlist_entry;
    flipc_data_buffer_t dest_buffer;
    flipc_cb_ptr tmp_cbptr, process_cbptr;
    int epgroup_cbptr;
    flipc_epgroup_t epgroup;

    /* Set the AIL sync variable.  */
    SYNCVAR_WRITE(((flipc_comm_buffer_ctl_t) flipc_cb_base)->sme_receive_in_progress = 1);

    /* Define a cleanup macro for this routine, just so that I don't duplicate
       a lot of code all over the place, and so that future code modifications
       don't forget something.  */
#define ROUTINE_CLEANUP(kkt_reply)			      		   \
    do {								   \
	if (kkt_reply) {						   \
	   mcmsg_trace_debug("r_kkt_entry", 4, kkt_reply, endpoint_idx,0,0); \
	   mcmsg_trace_drop("bad berries", endpoint_idx);		   \
	   mcmsg_flush_invalid();					   \
	}								   \
	SYNCVAR_WRITE(((flipc_comm_buffer_ctl_t)			   \
		       flipc_cb_base)->sme_receive_in_progress = 0);	   \
	return;								   \
    } while (0)

    /* Check if the comm buffer's active.  */
    if (!communications_buffer_active) {
	flipcme_dropmsg_inactive++;
	ROUTINE_CLEANUP(1);
    }

    /* Always need to do range checking since FLIPC_NULL_ENDPOINT_IDX is
       now a possible value to have.  */
    if (endpoint_idx >= cb_ctl.endpoint.number) {
	/* Check for destination null.  */
	if (endpoint_idx != FLIPCME_NULL_ENDPOINT_IDX) {
		/* The user screwed up.  */
		flipcme_incr_corruptcb();
	}
	ROUTINE_CLEANUP(3);
    }

    endpoint = endpoint_array + endpoint_idx;

    dpb_or_enabled = endpoint->saildm_dpb_or_enabled;
#if FLIPC_UCHECK
    /* Is this endpoint active?  */
    if (!EXTRACT_ENABLED(dpb_or_enabled)) {
	((flipc_comm_buffer_ctl_t)flipc_cb_base)->sme_error_log.msgdrop_disabled++;
	ROUTINE_CLEANUP(4);
    }

    /* Is it a receive endpoint?  */
    if (endpoint->constdm_type != FLIPC_Receive) {
	((flipc_comm_buffer_ctl_t)flipc_cb_base)->sme_error_log.msgdrop_badtype++;
	ROUTINE_CLEANUP(5);
    }
#endif

    /* Does it have anything to send?  We put off the paranoid consistency
       checks for the moment to use macros; this is safe as the macros
       don't actually dereference anything.  See flipc_cb.h for the
       definitions of this macro.  */
    if (!ENDPOINT_PROCESS_OK(process_cbptr, tmp_cbptr, endpoint)) {
	/* No room.  */
	endpoint->sme_overruns++;
#if FLIPC_PERF
	/* Mark the message as having been received (overruns count).  */
	((flipc_comm_buffer_ctl_t)flipc_cb_base)->performance.messages_received++;
#endif
	ROUTINE_CLEANUP(7);
    }

    /* Now we make sure of the process pointer, since that's what we
       care about.  */
    if (!CBPTR_VALID_BUFFERLIST_P(process_cbptr)) {
	flipcme_incr_corruptcb();
	ROUTINE_CLEANUP(7);
    }

    /* Ok, we're set.  Find the destination buffer and copy in the
       data.  */
    bufferlist_entry = *FLIPC_BUFFERLIST_PTR(process_cbptr);

    /* Paranoid consistency checks.  */
    if (!CBPTR_VALID_DATA_BUFFER_P(bufferlist_entry)) {
	flipcme_incr_corruptcb();
	ROUTINE_CLEANUP(8);
    }

#if FLIPC_PERF
    /* Mark the message as having been received.  */
    ((flipc_comm_buffer_ctl_t)flipc_cb_base)->performance.messages_received++;
#endif	

    dest_buffer = FLIPC_DATA_BUFFER_PTR(bufferlist_entry);
    mcmsg_flipc_recv_buf(dest_buffer, flipcme_data_buffer_size);

    /* Increment the processing pointer.  */
    endpoint->sme_process_ptr =
    	NEXT_BUFFERLIST_PTR_ME(process_cbptr, endpoint);

    /* Set the state in the buffer.  */
    if (EXTRACT_DPB(dpb_or_enabled))
	SYNCVAR_WRITE(dest_buffer->shrd_state = flipc_Ready);
    else
	SYNCVAR_WRITE(dest_buffer->shrd_state = flipc_Completed);

    /* Start up endpoint group processing.  If there isn't an endpoint
       group, or if it isn't supposed to generate wakeups, or there isn't a
       wakeup requested, return.  */

    epgroup_cbptr = endpoint->sail_epgroup;
    if (epgroup_cbptr == FLIPC_CBPTR_NULL)
	ROUTINE_CLEANUP(0);

    /* Paranoid consistency checks.  */
    if (!CBPTR_VALID_EPGROUP_P(epgroup_cbptr)) {
	flipcme_incr_corruptcb();
	ROUTINE_CLEANUP(0);
    }

    /* Do all the wakeup processing.  */
    flipcme_deliver_wakeup(FLIPC_EPGROUP_PTR(epgroup_cbptr), 0);

    ROUTINE_CLEANUP(0);
}
#else	/* PARAGON860 && !FLIPC_KKT */

/*
 * The entry point into the message engine from KKT;
 * takes an incoming message and does the right thing with it.
 */
void
flipcme_receive_kkt_entry(channel_t chan, rpc_buf_t buf,
			  boolean_t thread_context_p)
{
    int recv_sv, shared_sv;	/* Shared vars for serialization.  */
    int endpoint_idx;
    flipc_endpoint_t endpoint;
    u_long dpb_or_enabled;
    flipc_cb_ptr tmp_cbptr, process_cbptr;
    flipc_cb_ptr bufferlist_entry;
    flipc_data_buffer_t dest_buffer;
    int epgroup_cbptr;
    flipc_epgroup_t epgroup;
    int wakeup_needed = 0;
    kkt_return_t kktr;

    /* What's our endpoint?  Done up top for the possible debugging;
       doesn't cost us anything.  */
    endpoint_idx = FLIPC_ADDRESS_ENDPOINT(((flipc_data_buffer_t)
					   buf->buff)->u.destination);

    /* Not sure if it's cool to do this in interrupt context or not.  */
#if FLIPC_TRACE
    if (flipc_me_trace)
	printf("Flipc ME receive for endpoint: %d\n",
	       endpoint_idx);
#endif

    /* Make sure we're the only thread receiving.  */
    RECV_SERIALIZE_LOCK(recv_sv);

    /* Set the AIL sync variable.  */
    SYNCVAR_WRITE(((flipc_comm_buffer_ctl_t) flipc_cb_base)->sme_receive_in_progress = 1);

    WRITE_FENCE();

    /* Define a cleanup macro for this routine, just so that I don't duplicate
       a lot of code all over the place, and so that future code modifications
       don't forget something.  */
#define ROUTINE_CLEANUP(kkt_reply)			      		   \
    do {								   \
	if (kkt_reply) {						   \
	    kktr = KKT_RPC_REPLY(buf->handle);				   \
	    if (kktr != KKT_SUCCESS)					   \
		panic("flipcme_receive_kkt_entry: KKT_RPC_REPLY failed."); \
	}								   \
	WRITE_FENCE();							   \
	SYNCVAR_WRITE(((flipc_comm_buffer_ctl_t)			   \
		       flipc_cb_base)->sme_receive_in_progress = 0);	   \
	RECV_SERIALIZE_UNLOCK(recv_sv);					   \
	return;								   \
    } while (0)

    /* Check if the comm buffer's active.  */
    if (!communications_buffer_active) {
	flipcme_dropmsg_inactive++;
	ROUTINE_CLEANUP(1);
    }

    /* Always need to do range checking since FLIPC_NULL_ENDPOINT_IDX is
       now a possible value to have.  */
    if (endpoint_idx < 0 || endpoint_idx >= cb_ctl.endpoint.number) {
	/* Check for destination null.  */
	if (endpoint_idx == FLIPCME_NULL_ENDPOINT_IDX) {
	    /* This is the null destination; we do nothing.  */
	    ROUTINE_CLEANUP(1);
	}
	/* The user screwed up.  */
	flipcme_incr_corruptcb();
	ROUTINE_CLEANUP(1);
    }

    endpoint = endpoint_array + endpoint_idx;

#if FLIPC_UCHECK
    /* Is this endpoint active?  */
    dpb_or_enabled = endpoint->saildm_dpb_or_enabled;
    if (!EXTRACT_ENABLED(dpb_or_enabled)) {
	((flipc_comm_buffer_ctl_t)flipc_cb_base)->sme_error_log.msgdrop_disabled++;
	ROUTINE_CLEANUP(1);
    }

    /* Is it a receive endpoint?  */
    if (endpoint->constdm_type != FLIPC_Receive) {
	((flipc_comm_buffer_ctl_t)flipc_cb_base)->sme_error_log.msgdrop_badtype++;
	ROUTINE_CLEANUP(1);
    }
#endif

    /* Does it have anything to send?  We put off the paranoid consistency
       checks for the moment to use macros; this is safe as the macros
       don't actually dereference anything.  See flipc_cb.h for the
       definitions of this macro.  */
    if (!ENDPOINT_PROCESS_OK(process_cbptr, tmp_cbptr, endpoint)) {
	/* No room.  */
	SYNCVAR_WRITE(endpoint->sme_overruns++);
#if FLIPC_PERF
	/* Mark the message as having been received (overruns counts).  */
	((flipc_comm_buffer_ctl_t)flipc_cb_base)->performance.messages_received++;
#endif	
	ROUTINE_CLEANUP(6);
    }

    /* Now we make sure of the process pointer, since that's what we
       care about.  */
    if (!CBPTR_VALID_BUFFERLIST_P(process_cbptr)) {
	flipcme_incr_corruptcb();
	ROUTINE_CLEANUP(7);
    }

    /* Ok, we're set.  Find the destination buffer and copy in the
       data.  */
    bufferlist_entry = *FLIPC_BUFFERLIST_PTR(process_cbptr);

    /* Paranoid consistency checks.  */
    if (!CBPTR_VALID_DATA_BUFFER_P(bufferlist_entry)) {
	flipcme_incr_corruptcb();
	ROUTINE_CLEANUP(1);
    }

#if FLIPC_PERF
    /* Mark the message as having been received.  */
    ((flipc_comm_buffer_ctl_t)flipc_cb_base)->performance.messages_received++;
#endif

    dest_buffer = FLIPC_DATA_BUFFER_PTR(bufferlist_entry);
    bcopy(((char *) buf->buff), ((char *) dest_buffer),
	  flipcme_data_buffer_size);

    /* RPC_REPLY now to holdup the remote node the minimum.  */
    kktr = KKT_RPC_REPLY(buf->handle);
    if (kktr != KKT_SUCCESS)
	panic("flipcme_receive_kkt_entry: KKT_RPC_REPLY failed.");

    /* Increment the processing pointer.  */
    endpoint->sme_process_ptr =
	NEXT_BUFFERLIST_PTR_ME(process_cbptr, endpoint);

    /* Need to get that done with before setting the state in the buffer.  */
    WRITE_FENCE();

    /* Set the state in the buffer.  */
    if (EXTRACT_DPB(dpb_or_enabled))
	SYNCVAR_WRITE(dest_buffer->shrd_state = flipc_Ready);
    else
	SYNCVAR_WRITE(dest_buffer->shrd_state = flipc_Completed);

    /* Start up endpoint group processing.  If there isn't an endpoint
       group, or if it isn't supposed to generate wakeups, or there isn't a
       wakeup requested, return.  */

    epgroup_cbptr = endpoint->sail_epgroup;
    if (epgroup_cbptr == FLIPC_CBPTR_NULL)
	ROUTINE_CLEANUP(0);

    /* Paranoid consistency checks.  */
    if (!CBPTR_VALID_EPGROUP_P(epgroup_cbptr)) {
	flipcme_incr_corruptcb();
	ROUTINE_CLEANUP(0);
    }

    /* Do all the wakeup processing.  */
    flipcme_deliver_wakeup(FLIPC_EPGROUP_PTR(epgroup_cbptr), 0);

    ROUTINE_CLEANUP(0);
}
#endif	/* PARAGON860 && !FLIPC_KKT */

#undef ROUTINE_CLEANUP

/*
 * Try to send a message from an endpoint.  Only called from the
 * duty loop, below.  Assumes the send serialization lock is held,
 * but deals with shared serialization lock itself.
 */
static INLINE
flipc_return_t flipcme_attempt_send(flipc_endpoint_t endpoint)
{
    int shared_sv;	/* Shared vars for serialization.  */
    u_long dpb_or_enabled;
    flipc_cb_ptr tmp_cbptr, process_cbptr;
    flipc_cb_ptr bufferlist_entry;
    flipc_data_buffer_t source_buffer;
    int epgroup_cbptr;
    flipc_epgroup_t epgroup;
    int wakeup_needed = 0;
    void *kkt_handle_buf;
    int dest_node;
    kkt_return_t kktr;

    /* Is this endpoint active?  */
    dpb_or_enabled = endpoint->saildm_dpb_or_enabled;
    if (!EXTRACT_ENABLED(dpb_or_enabled)) {
	return flipc_Send_Endpoint_Disabled;
    }

    /* Is it a send endpoint?  */
    if (endpoint->constdm_type != FLIPC_Send) {
	return flipc_Send_Endpoint_Badtype;
    }

    /* Does it have anything to send?  We put off the paranoid consistency
       checks for the moment to use macros; this is safe as the macros
       don't actually dereference anything.  See flipc_cb.h for the
       definitions of this macro.  */
    if (!ENDPOINT_PROCESS_OK(process_cbptr, tmp_cbptr, endpoint))
	return flipc_Send_Nobuffer;

    /* Now we make sure of the process pointer, since that's what we
       care about.  */
    if (!CBPTR_VALID_BUFFERLIST_P(process_cbptr)) {
	flipcme_incr_corruptcb();
	return flipc_Buffer_Corrupted;
    }

    /* Ok, we're set.  Find the destination buffer, copy the data out,
       and send it.  */
    bufferlist_entry = *FLIPC_BUFFERLIST_PTR(process_cbptr);

    /* Paranoid consistency checks.  */
    if (!CBPTR_VALID_DATA_BUFFER_P(bufferlist_entry)) {
	flipcme_incr_corruptcb();
	return flipc_Buffer_Corrupted;
    }

    source_buffer = FLIPC_DATA_BUFFER_PTR(bufferlist_entry);

#if FLIPC_TRACE
    if (flipc_me_trace)
	if (source_buffer->u.destination != cb_ctl.null_destination)
	    printf("Flipc ME: Sending from endpoint: %d to address: %d-%d\n",
		   endpoint - endpoint_array,
		   FLIPC_ADDRESS_NODE(source_buffer->u.destination),
		   FLIPC_ADDRESS_ENDPOINT(source_buffer->u.destination));
	else
	    printf("Flipc ME: Sending from endpoint: %d to null destination.\n",
		   endpoint - endpoint_array);
#endif

#if FLIPC_PERF
    /* Track performance info.  */
    ((flipc_comm_buffer_ctl_t)flipc_cb_base)->performance.messages_sent++;
#endif

    dest_node = FLIPC_ADDRESS_NODE(source_buffer->u.destination);

    /* Used to check for a bad destination here, but it's now
       strictly a transport check.  */

    /* Send it.  */
#if	PARAGON860 && !FLIPC_KKT
    /*
     * mcmsg_flipc_send will always succeed, so there is no error
     * code to evaluate.
     */
    mcmsg_flipc_send(dest_node, source_buffer, flipcme_data_buffer_size,
			FLIPC_ADDRESS_ENDPOINT(source_buffer->u.destination));
#else	/* PARAGON860 && !FLIPC_KKT */
    /* Need to copy the whole thing because the receive side
       flipc stuff needs to know what the destination handle is.  */
    bcopy((char *) source_buffer,
	  (char *) flipcme_kkt_send_buffer,
	  flipcme_data_buffer_size);

    kktr = KKT_RPC(dest_node, flipcme_kkt_send_handle);

    /* Note failure for user.  */
    if (kktr != KKT_SUCCESS)
	((flipc_comm_buffer_ctl_t)flipc_cb_base)->sme_error_log.transport_error_info[kktr-KKT_MIN_ERRNO]++;
#endif	/* PARAGON860 && !FLIPC_KKT */

    /* Increment the processing pointer.  */
    endpoint->sme_process_ptr =
	NEXT_BUFFERLIST_PTR_ME(process_cbptr, endpoint);

    /* Need to get that done with before setting the state in the buffer.  */
    WRITE_FENCE();

    /* Set the state in the buffer.  */
    if (EXTRACT_DPB(dpb_or_enabled))
	SYNCVAR_WRITE(source_buffer->shrd_state = flipc_Ready);
    else
	SYNCVAR_WRITE(source_buffer->shrd_state = flipc_Completed);

    /* Start up endpoint group processing.  */

    epgroup_cbptr = endpoint->sail_epgroup;
    if (epgroup_cbptr == FLIPC_CBPTR_NULL) {
	return flipc_Success;
    }

    /* Paranoid consistency checks.  */
    if (!CBPTR_VALID_EPGROUP_P(epgroup_cbptr)) {
	flipcme_incr_corruptcb();
	return flipc_Buffer_Corrupted;
    }

    /* Do the wakeup processing.  */
    flipcme_deliver_wakeup(FLIPC_EPGROUP_PTR(epgroup_cbptr), 1);

    return flipc_Success;
}

/*
 * Kicked off from device_set_status routine.  Scans for work to do
 * and does it.
 */

/* Duty loop control parameters.  */

#define DUTY_LOOP_SPINS 1
/* There should also be a parameter here determining how effort is
   spread among the different send endpoints.  For now, it's hard-coded
   to be a round-robin algorithm; later maybe we'll change that.  */

void flipcme_duty_loop(int ignore_buffer_active)
{
    flipc_return_t fr;
    int send_sv;
    int found_work;
    flipc_endpoint_t endpoint;
    flipc_comm_buffer_ctl_t shared_cb_ctl = 
	    (flipc_comm_buffer_ctl_t) flipc_cb_base;
    flipc_cb_ptr send_endpoint_list;
    flipc_cb_ptr endpoint_cbptr;
    int request_sync, respond_sync; /* non-volatile holders for values
				       in the shared_cb_ctl.  */
#define	FAST_MESSAGES	0
#if	FAST_MESSAGES
    if (shared_cb_ctl->sendpoint) {
	flipc_data_buffer_t source_buffer;

	shared_cb_ctl->sendpoint = 0;
	endpoint = FLIPC_ENDPOINT_PTR(shared_cb_ctl->sendpoint);
	source_buffer = FLIPC_DATA_BUFFER_PTR(endpoint->sendpoint_buffer);

	mcmsg_flipc_send(FLIPC_ADDRESS_NODE(source_buffer->u.destination),
			 source_buffer,
			 flipcme_data_buffer_size,
			 FLIPC_ADDRESS_ENDPOINT(source_buffer->u.destination));
	source_buffer->shrd_state = flipc_Completed;
    }
#endif	/* FAST_MESSAGES */

    if (shared_cb_ctl->send_ready == 0)
	    return;

    found_work = DUTY_LOOP_SPINS;
    send_endpoint_list = shared_cb_ctl->sail_send_endpoint_list;

    /* Serialize work on the send side.  */
    SEND_SERIALIZE_LOCK(send_sv);

    /* Make sure that we are supposed to be working.  If the comm buffer is
       inactive and we haven't been told to ignore that, return.  */
    if (!ignore_buffer_active
	&& !communications_buffer_active) {
	SEND_SERIALIZE_UNLOCK(send_sv);
	return;
    }

    /* While we still have things to do.  */
    while (found_work) {
	found_work--;

	/* Go through all the send endpoints and look for messages to
	   send.  */
	endpoint_cbptr = send_endpoint_list;
	while (endpoint_cbptr != FLIPC_CBPTR_NULL) {

	    /* Paranoid consistency check.  */
	    if (!CBPTR_VALID_ENDPOINT_P(endpoint_cbptr)) {
		flipcme_incr_corruptcb();
		SEND_SERIALIZE_UNLOCK(send_sv);
		return;
	    }

	    endpoint = FLIPC_ENDPOINT_PTR(endpoint_cbptr);

	    /* Try to send from this endpoint.  */
	    fr = flipcme_attempt_send(endpoint);

	    if (fr == flipc_Success) {
		found_work = DUTY_LOOP_SPINS;
		/*
		 * We can zero out the shared send counter as we will
		 * check again if there is something to send, which takes
		 * care of the obvious race.
		 */
		shared_cb_ctl->send_ready = 0;
	    } else if (fr == flipc_Buffer_Corrupted) {
		/* Shut down.  Setting buffer inactive has already
		   been taken care of for us.  */
		shared_cb_ctl->send_ready = 0;
		SEND_SERIALIZE_UNLOCK(send_sv);
		return;
	    }

	    endpoint_cbptr = endpoint->sail_next_send_endpoint;
	}

	/* Check to see if the AIL wants to sync with us.  If it does,
	   reload the send endpoint list start.  */

	/* I cast reads from my private copy of the cb_ctl to u_long
	   because those reads do not have to be considered volatile
	   by the compiler.  */
	request_sync = shared_cb_ctl->sail_request_sync;
	if (request_sync != *(u_long *) &cb_ctl.sail_request_sync) {
	    /* Note the change in value for next time.  */
	    (*(u_long *) &cb_ctl.sail_request_sync) = request_sync;

	    /* Reload the beginning of the send endpoint list.  */
	    send_endpoint_list = shared_cb_ctl->sail_send_endpoint_list;

	    /* Respond to the request for synchronization.  */
	    respond_sync = shared_cb_ctl->sme_respond_sync;
	    SYNCVAR_WRITE(shared_cb_ctl->sme_respond_sync =
			  respond_sync ^ 1);

	    found_work = DUTY_LOOP_SPINS;
	}
    }

    /* Let other sender stuff run.  */
    SEND_SERIALIZE_UNLOCK(send_sv);
}

#if FLIPC_KKT
/* Define a (currently null) low-level test for kkt flipc.  */
volatile int flipc_test_total = 1;

void flipcme_low_level_test(int node, int test_size, int *result)
{
    *result = 0;
}
     
#endif
    
