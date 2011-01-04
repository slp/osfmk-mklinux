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
 * Adapted from init.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 */

#include <dipc_xkern.h>
#include <xkern/include/domain.h>
#include <xkern/include/upi.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/platform.h>
#include <xkern/include/netmask.h>
#include <xkern/include/prottbl.h>
#include <xkern/include/xk_path.h>
#include <xkern/include/event.h>
#include <xkern/include/compose.h>
#include <xkern/include/xk_path.h>
#include <mach/mach_server.h>

/* the event scheduling interval granularity */
int  event_granularity = 100; 

#if XK_DEBUG
int traceinit;
#endif /* XK_DEBUG */

/* the inkernel version needs a pre-compiled initRom table */
#include <uk_xkern/initRom.c>

#if !DIPC_XKERN
/* 
 * If DIPC is not configured, we fall back to the test
 * configuration mode, whereby you just drop into the
 * kdb and set x_server or x_client.
 */
#define NMAXARGS	16

volatile int x_server;
volatile int x_client;

int globalArgc;
char **globalArgv;

/*
 *  initialization of testArgs determines default args: 
 *	 char *testArgsServer = "-testchan -s";
 *	char *testArgsClient = "-testchan -c130.105.2.102";
 */

char *testArgsServer = "";
char *testArgsClient = "";


static char *testArgv[NMAXARGS]	= {"xkernel"};
static int testArgc = 1;

/*
 * Split "testArgs" into individual arguments and store them in
 * testArgv[]; testArgc is the number of args in testArgv[];
 * testArgv[0] is always "xkernel".
 */
static void
split_args( void )
{
    char *cp = (x_server) ? testArgsServer : testArgsClient;
    extern char* strchr(const char *, int);

    /* split args and store them in testArgc/testArgv: */

    while (cp && *cp) {
	testArgv[testArgc++] = cp;
	if (testArgc >= NMAXARGS) {
	    Kabort("init.c: too many actual args (increase NMAXARGS)");
	} /* if */
	cp = strchr(cp, ' ');
	if (cp) {
	    *cp++ = '\0';
	    while (*cp == ' ') {
		++cp;
	    } /* while */
	} /* if */
    } /* while */
    testArgv[testArgc] = 0;
} /* split_args */
#endif /* !DIPC_XKERN */


#define try(funcCall, str) \
{ \
   if ( funcCall == XK_FAILURE ) { \
       sprintf(errBuf, "Fatal x-kernel initialization error in %s module", \
	       str); \
       xError(errBuf); \
       xError("x-kernel initialization halted"); \
       return; \
   } \
}


static void
build_pgraph_stub( Event ev, void *arg )
{
    build_pgraph();
}


void
xkInit()
{
#if !DIPC_XKERN
    printf("x-kernel ready to start ...\n");
    while (!x_server && !x_client) {
	assert_wait((event_t)0, TRUE);
	thread_set_timeout(10 * hz);
        thread_block((void (*)(void))0);
	reset_timeout_check(&current_thread()->timer);
    }
    printf("Starting x-kernel as a %s\n", 
	    (x_server)? "server" : "client");

    split_args();

    globalArgc = testArgc;
    globalArgv = testArgv;
#endif /* !DIPC_XKERN */

    /*
     * Locking is only needed to make the consistency checks happy!
     */
    simple_lock_init(&xkMaster_lock, ETAP_XKERNEL_MASTER);
    xk_master_lock();
    try( allocBootStrap(), "allocator bootstrap" );
    try( pathBootStrap(), "path bootstrap" );
    try( xTraceInit(), "trace" );
    try( initRom(), "rom" );
    try( addRomFlags(), "rom boot flags" );
    try( map_init(), "map" );
    try( threadInit(), "thread" );
    try( evInit(event_granularity), "event" );
    try( allocInit(), "allocator" );
    try( pathInit(), "path" );
    try( msgInit(), "msg" );
    try( netMaskInit(), "netmask" );
    try( prottbl_init(), "protocol table" );
    try( upiInit(), "UPI" );
    build_pgraph_dev();
    {
	Event ev;

	if ( ev = evAlloc(xkSystemPath) ) {
	    evDetach(evSchedule(ev, build_pgraph_stub, 0, 0 ));
	} else {
	    xError("x-kernel couldn't allocate graph initialization event");
	}
    }
    xk_master_unlock();

#if !DIPC_XKERN
    thread_terminate_self();
    panic( "The zombie xkInit strolls" );
#endif /* !DIPC_XKERN */
}
