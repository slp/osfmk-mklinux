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
 * chantest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:39:55 $
 */

/*
 * Ping-pong test of CHAN
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip_host.h>

/*
 * These definitions describe the lower protocol
 */
#ifndef NON_IP_ADDRESSING

#define HOST_TYPE IPhost
/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0, 0, 0, 0 };
static HOST_TYPE ClientAddr = { 0, 0, 0, 0 };

#else  /* NON_IP_ADDRESSING */

#define HOST_TYPE long
static HOST_TYPE ServerAddr;
static HOST_TYPE ClientAddr;

#define STR2HOST str2addr

void
str2addr(h, s)
long *h;
char *s;
{
    extern int atoi();

    *h = atoi(s);
}

#endif /* NON_IP_ADDRESSING */

#define INIT_FUNC chantest_init
#define TRACE_VAR tracechantestp
#define PROT_STRING "chan"


#define TRIPS 100
#define TIMES 1
#define DELAY 3
/*
 * Define to do timing calculations
 */
#define TIME
#define SAVE_SERVER_SESSN
#define RPCTEST
/* #define CUSTOM_ASSIGN */

/*
#define SIMPLE_ACKS
#define REPLY_SIZE 4
*/

static int lens[] = { 
  4, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144 
/*
  16384, 
  16384,   
  16384, 
  16384,   
  16384, 
  16384,   
  16384, 
  16384,   
  16384, 
  16384,   
  16384, 
  16384,   
  16384, 
  16384,   
  16384, 
  16384,   
  16384
*/
};

#include <xkern/protocols/test/common_test.c>

static void
testInit( XObj self )
{
}
