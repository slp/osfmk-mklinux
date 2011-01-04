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
 * A minimal test to prove FIFO total order with the sequencer
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/membership.h>
#include <xkern/include/prot/sequencer.h>
#include <xkern/protocols/proxy/xk_mig_sizes.h>

#if XK_DEBUG
int tracesequencertestp;
#endif /* XK_DEBUG */

#if MACH_KERNEL
#include <dipc_xkern.h>
#include <kern/clock.h>
#include <kern/ipc_host.h>
#include <mach/clock_server.h>
#include <kern/sched.h>

int tryPush( XObj self, XObj sessn, int times, int length );
int sequencertestControlProtl( XObj self, int opcode, char *buf, int len);
void sequencertesttrigger(void);

kern_return_t rtc_gettime(tvalspec_t *);
#else  /* MACH_KERNEL */
#define VERBOSE 1
#endif /* MACH_KERNEL */

XObj sequencertestprotocol;
int  sequencer_abort_count;

#define MIN(x,y)                ((x)<(y) ? (x) : (y))

#if DIPC_XKERN
int x_server;
int x_client;
#else /* DIPC_XKERN */
extern volatile int x_server;
extern volatile int x_client;
#endif /* DIPC_XKERN */

#define TEST_STACK_SIZE	200
#define FAILURE_LIMIT	3
#define PROT_STRING	"sequencertest"
#define DELAY 10
#define DUMP_AREA_SIZE  64 * 1024
#if MACH_KERNEL
#define SEQUENCERTEST_PRIORITY BASEPRI_KERNEL+1
#else  /* MACH_KERNEL */
#define SEQUENCERTEST_PRIORITY 9
#endif /* MACH_KERNEL */
#define LOOP_DEFAULT     1
#define ALL_TOGETHER     1

static group_id	defaultGroup = ALL_NODES_GROUP;
static int	xk_trips = 1000;
static int	lens[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

struct dump_area {
    unsigned long prod;
    unsigned long cons;
    char          *addr;
} d_area_s;

/* 
 * Client only
 */
typedef struct {
    char 	mark;
    char	loop;
    group_id	group;
    XObj	lls;
    int		delay;
    int         sequencertest_priority;
    xkSemaphore sema;
    Event       evsequencer;
#if MACH_KERNEL
    tvalspec_t  starttime;
    tvalspec_t  nettime;
#else
    XTime       starttime;
    XTime       nettime;
#endif /* MACH_KERNEL */
} PState;

xkern_return_t
serverDemux( 
	    XObj self, 
	    XObj lls, 
	    Msg dg);

xkern_return_t
clientDemux( 
	    XObj self, 
	    XObj lls, 
	    Msg dg);

void
server(
       Event ev, 
       void *arg );

void
sequencertest_record(
		 void *time);

xkern_return_t
sequencertest_init(
	       XObj self );

void
sequencer_abort(
	    Event	ev,
	    void 	*arg);

/* 
 * instName group group_id
 */
static xkern_return_t
readGroup( XObj self, char **str, int nFields, int line, void *arg )
{
    ((PState *)arg)->group = atoi(str[2]);
    return XK_SUCCESS;
}

/* 
 * instName delay int
 */
static xkern_return_t
readDelay( XObj self, char **str, int nFields, int line, void *arg )
{
    return sscanf(str[2], "%d", &((PState *)arg)->delay) < 1 ?
      XK_FAILURE : XK_SUCCESS;
}


#if 0
/* 
 * instName save_interval int
 */
static xkern_return_t
readInterval( XObj self, char **str, int nFields, int line, void *arg )
{
    return sscanf(str[2], "%d", (int *)arg) < 1 ? XK_FAILURE : XK_SUCCESS;
}
#endif

/* 
 * instName mark char
 */
static xkern_return_t
readMark( XObj self, char **str, int nFields, int line, void *arg )
{
    ((PState *)arg)->mark = *str[2];
    return XK_SUCCESS;
}


/* 
 * instName trips N
 */
static xkern_return_t
readTrips( XObj self, char **str, int nFields, int line, void *arg )
{
    return sscanf(str[2], "%d", &xk_trips) < 1 ? XK_FAILURE : XK_SUCCESS;
}


/* 
 * instName loop
 */
static xkern_return_t
readLoop( XObj self, char **str, int nFields, int line, void *arg )
{
    ((PState *)arg)->loop = 1;
    return XK_SUCCESS;
}

/* 
 * instName priority N
 */
static xkern_return_t
readPriority( XObj self, char **str, int nFields, int line, void *arg )
{
    return sscanf(str[2], "%d", &((PState *)arg)->sequencertest_priority) < 1 ? XK_FAILURE : XK_SUCCESS;
}

/* 
 * instName isserver
 */
static xkern_return_t
readIsServer( XObj self, char **str, int nFields, int line, void *arg )
{
    x_server = 1;
    return XK_SUCCESS;
}

/* 
 * instName isclient
 */
static xkern_return_t
readIsClient( XObj self, char **str, int nFields, int line, void *arg )
{
    x_client = 1;
    return XK_SUCCESS;
}

static XObjRomOpt	sequencerOpts[] = {
    { "group", 3, readGroup },
    { "mark", 3, readMark },
    { "trips", 3, readTrips },
    { "loop", 2, readLoop },
    { "delay", 3, readDelay },
    { "priority", 3, readPriority },
    { "isserver", 2, readIsServer },
    { "isclient", 2, readIsClient }
};

static xkern_return_t
saveServerSessn( XObj self, XObj lls, XObj llp, XObj hlpType, Path path )
{
    xTrace1(prottest, TR_MAJOR_EVENTS,
	    "%s test program duplicates lower server session",
	    PROT_STRING);
    xDuplicate(lls);
    return XK_SUCCESS;
}

xkern_return_t
serverDemux( XObj self, XObj lls, Msg dg )
{
    PState 	*ps = (PState *)self->state;
    sequencerXchangeHdr  *xhdr = msgGetAttr(dg, 0);

    xAssert(xhdr->xkr == XK_SUCCESS);

    xTrace2(prottest, TR_FULL_TRACE, "received for group %d message %d", ps->group, xhdr->seq);
    
    return XK_SUCCESS;
}

xkern_return_t
clientDemux( XObj self, XObj lls, Msg dg )
{

    PState 	*ps = (PState *)self->state;
    sequencerXchangeHdr  *xhdr = msgGetAttr(dg, 0);
    Msg_s       reply;
#if  MACH_KERNEL
    tvalspec_t  singletime;
#else 
    XTime       endtime;
#endif /* MACH_KERNEL */

    if (xhdr->xkr == XK_SUCCESS) {
	xTrace2(prottest, TR_FULL_TRACE, "received for group %d message %d", ps->group, xhdr->seq);
    } else {
  	xTrace2(prottest, TR_FULL_TRACE, "received for group %d message %d with XK_FAILURE", ps->group, xhdr->seq);
    }
    
    semSignal(&ps->sema);
#if MACH_KERNEL
    rtc_gettime(&ps->nettime);
    SUB_TVALSPEC(&ps->nettime, &ps->starttime);
    sequencertest_record((void *)&ps->nettime);
#else 
    xGetTime(&endtime);
    xSubTime(&ps->nettime, endtime, ps->starttime);
#endif /* MACH_KERNEL */
    return XK_SUCCESS;
}

void
sequencertest_record(
	       void *time)
{
    long *p = (long *)d_area_s.addr;
    long *sample = (long *)time;
    *p++ = *sample++;
    *p = *sample;
    d_area_s.addr += (2 * sizeof(long));
    d_area_s.prod += (2 * sizeof(long));
    if (d_area_s.prod >= DUMP_AREA_SIZE) {
	printf("Prod wraps\n");
	d_area_s.prod = 0;
	d_area_s.addr -= DUMP_AREA_SIZE;
    }
}

void
sequencer_abort(
	    Event	ev,
	    void 	*arg)
{
    XObj        self = (XObj)arg;
    PState 	*ps = (PState *)self->state;

    sequencer_abort_count++;
    semSignal(&ps->sema);
}

int
tryPush( XObj self, XObj sessn, int times, int length )
{
    xkern_return_t xkr;
    int i;
    Msg_s	savedMsg, request;
    PState 	*ps = (PState *)self->state;
#if MACH_KERNEL
    tvalspec_t  time_val_before, time_val_after;
#endif /* MACH_KERNEL */
    
    xkr = msgConstructContig(&savedMsg, self->path,
				 (u_int)length,
				 TEST_STACK_SIZE);
    if (xkr == XK_FAILURE) {
	sprintf(errBuf, "Could not construct a msg of length %d\n", 
		length + TEST_STACK_SIZE);
	Kabort(errBuf);
    }
    for (i = 0; i < times; i++) {
	msgConstructContig(&request, self->path, 0, 0);
	semInit(&ps->sema, 0);
#if  MACH_KERNEL
	rtc_gettime(&ps->starttime);
#else
	xGetTime(&ps->starttime);
#endif /* MACH_KERNEL */
	msgAssign(&request, &savedMsg);
	xkr = xPush(sessn, &request);
	if( xkr == XK_FAILURE ) {
	    printf( "Dgram Push error %d\n" , xkr ); 
	    goto abort;
	}
	evSchedule(ps->evsequencer, sequencer_abort, self, 30000000);
	semWait(&ps->sema);
	evCancel(ps->evsequencer);
	msgDestroy(&request);
    }
    
 abort:
    msgDestroy(&savedMsg);
    msgDestroy(&request);
    return i;
}

static void
client( Event ev, void *arg )
{
    Part_s	participants, *p;
    XObj	self = (XObj)arg;
    XObj	llp;
    PState	*ps = (PState *)self->state;
    int		testNumber = 0, len, count, lenindex, noResponseCount = 0;
    
    xAssert(xIsXObj(self));

    xTrace1(prottest, TR_ALWAYS, "sending to %d", ps->group);
    printf("SEQUENCER timing test\n");
    llp = xGetDown(self, 0);
    if ( ! xIsProtocol(llp) ) {
	xError("Test protocol has no lower protocol");
	return;
    }
    printf("I am the client\n");
    if ( ps->delay ) {
	xTrace1(prottest, TR_ALWAYS, "Delaying %d seconds", ps->delay);
	Delay( ps->delay * 1000 );
    }
    self->demux = clientDemux;

    p = &participants;
    partInit(p, 1);
    partPush(*p, &ps->group, sizeof(group_id));
    ps->lls = xOpen(self, self, llp, p, self->path);
    if ( ps->lls == ERR_XOBJ ) {
	printf("%s test: open failed!\n", PROT_STRING);
	xError("End of test");
	return;
    }
#ifdef notdef
    /*
     * HACK! client will steal an event thread that it will not return
     * for a while.
     */
    if ( xkPolicyFixedFifo(&ps->sequencertest_priority, sizeof(ps->sequencertest_priority)) != XK_SUCCESS ) {
        printf("WARNING: xk_service_thread is being TIMESHARED!\n");
    }
#endif /* notdef */
    /* 
     * Delay to allow incoming messages to finalize opens (if necessary)
     */
    Delay(1000);
    for (lenindex = 0; lenindex < sizeof(lens)/sizeof(long) || ps->loop;
	 lenindex++) {
	if (lenindex >= sizeof(lens)/sizeof(long)) {
	    lenindex = 0;
	}
	len = lens[lenindex];
#ifdef VERBOSE
	printf("Sending (%d, [%c]) ...\n", ++testNumber, ps->mark);
#endif /* VERBOSE */
	count = tryPush(self, ps->lls, xk_trips, len);
	if ( count == xk_trips ) {
#ifdef VERBOSE
	    printf("\n[%c] len = %4d, %d trips: %6d.%06d\n", 
		   ps->mark, len, xk_trips, ps->nettime.sec, ps->nettime.usec);
#endif /* VERBOSE */
	}
	if ( count == 0 ) {
	    if ( noResponseCount++ == FAILURE_LIMIT ) {
		printf("Server looks dead.  I'm outta here.\n");
		break;
	    }
	} else {
	    noResponseCount = 0;
	}
	Delay(DELAY * 1000);
    }
    printf("[%c] End of test\n", ps->mark);
    xClose(ps->lls);
    ps->lls = 0;
    xTrace0(prottest, TR_FULL_TRACE, "test client returns");

#ifdef notdef 
    {
	/*
	 * restore the event priority!
	 */
	int         new_priority = XK_EVENT_THREAD_PRIORITY;

	if ( xkPolicyFixedFifo(&new_priority, sizeof(new_priority)) != XK_SUCCESS ) {
	    printf("WARNING: xk_service_thread is being lost!\n");
	}
    }
#endif /* notdef */
}

void
server( Event ev, void *arg )
{
    XObj	self = (XObj)arg;
    PState	*ps = (PState *)self->state;
    XObj	llp;
    Part_s	participants, *p;

    llp = xGetDown(self, 0);
    if ( ! xIsProtocol(llp) ) {
	xError("Test protocol has no lower protocol");
	return;
    }
    self->opendone = saveServerSessn;
    self->demux = serverDemux;
    p = &participants;
    partInit(p, 1);
    partPush(*p, ANY_GROUP, 0);
    if ( xOpenEnable(self, self, llp, p) == XK_FAILURE ) {
	sprintf(errBuf, "%s server can't openenable lower protocol",
		PROT_STRING);
	xError(errBuf);
    } else {
	sprintf(errBuf, "%s server done with xopenenable", PROT_STRING);
	xError(errBuf);
    }
}

int
sequencertestControlProtl(
		      XObj self,
		      int opcode,
		      char *buf,
		      int len)
{
    switch (opcode){
    case  (10103<<16):
	{
	    int size;

	    if (d_area_s.prod > d_area_s.cons) {
		size = MIN(len, (d_area_s.prod - d_area_s.cons));
	    } else if (d_area_s.prod < d_area_s.cons) {
		size = MIN(len, (DUMP_AREA_SIZE - d_area_s.cons));
	    } else
		return 0;

	    size &= ~3;
	    memcpy(buf, d_area_s.addr - d_area_s.prod + d_area_s.cons, size);
	    d_area_s.cons += size;
	    d_area_s.cons %= DUMP_AREA_SIZE;
	    return size;
	}
    }
    return 0;
}

void
sequencertesttrigger(
		 void)
{
    PState	*ps = (PState *)sequencertestprotocol->state;

    if ( x_server ) {
	xError("Starting server");
	evDetach(evSchedule(evAlloc(sequencertestprotocol->path), server, sequencertestprotocol, 0));
    } else
	if ( x_client ) {
	    d_area_s.prod = 0;
	    d_area_s.cons = 0;
#if MACH_KERNEL
	    (void)kmem_alloc(kernel_map, (vm_offset_t *)&d_area_s.addr, DUMP_AREA_SIZE);
#else   /* MACH_KERNEL */
	    d_area_s.addr = (char *)pathAlloc(sequencertestprotocol->path, DUMP_AREA_SIZE);
#endif  /* MACH_KERNEL */
	    if (!d_area_s.addr) {
		xError("sequencertest: couldn't allocate dump area\n");
		pathFree(ps);
		return;
	    }
	    xError("Starting client");
	    evDetach(evSchedule(evAlloc(sequencertestprotocol->path), client, sequencertestprotocol, 0));
	}
    else 
	printf("Pls. assert either x_client or x_server\n");
}

xkern_return_t
sequencertest_init( XObj self )
{
    static XObj	firstServer;
    PState	*ps;

    sequencertestprotocol = self;
    ps = pathAlloc(self->path, sizeof(PState));
    if (!ps) {
	xError("sequencertest: couldn't allocate PState\n");
	return XK_FAILURE;
    }
    self->state = ps;
    if ((ps->evsequencer = evAlloc(self->path)) == 0) {
	pathFree(ps);
	return XK_FAILURE;
    }
    ps->mark = '.';
    ps->loop = LOOP_DEFAULT;
    ps->delay = 0;
    ps->sequencertest_priority = SEQUENCERTEST_PRIORITY;
    ps->group = defaultGroup;

    if ( findXObjRomOpts(self, sequencerOpts,
		sizeof(sequencerOpts)/sizeof(XObjRomOpt), ps) == XK_FAILURE ) {
	xError("sequencertest: romfile config errors, not running");
	return XK_FAILURE;
    }
#ifdef notdef
    {
	/*
	 * Overload standard path with custom path
	 */
	
	Path                p;
	XkThreadPolicy_s    policy;
	int                 pri = ps->sequencertest_priority;
	xkern_return_t      xkr;

	p = pathCreate(31, "test path");
	policy.func = xkPolicyFixedFifo;
	policy.arg = &pri;
	policy.argLen = sizeof(int);
	xkr = pathEstablishPool(p, 10, 1, &policy, 0);
	if ( xkr != XK_SUCCESS ) {
	    xError("pathEstablishPool fails");
	}
	self->path = p;
    }
#endif /* notdef */
#if !DIPC_XKERN
    sequencertesttrigger();
#endif /* DIPC_XKERN */
    self->control = sequencertestControlProtl;

    return XK_SUCCESS;
}










