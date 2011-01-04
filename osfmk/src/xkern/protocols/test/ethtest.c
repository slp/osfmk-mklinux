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
 * ethtest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:40:21 $
 */

/*
 * Ping-pong test of the ethernet protocol
 */
#include <xkern/include/xkernel.h>
#include <xkern/include/prot/eth.h>

#define HOST_TYPE ETHhost
#define INIT_FUNC ethtest_init
#define TRACE_VAR traceethtestp
#define PROT_STRING "eth"

/* 
 * If a host is booted without client/server parameters and matches
 * one of these addresses, it will come up in the appropriate role.
 */
static HOST_TYPE ServerAddr = { 0x0000, 0x0000, 0x0000 };
static HOST_TYPE ClientAddr = { 0x0000, 0x0000, 0x0000 };

#define TRIPS 100
#define TIMES 1
#define DELAY 3

/*
 * Define to do timing calculations
 */
#define TIME	 


static int lens[] = { 
  1, 200, 400, 600, 800, 1000, 1200
};


#define SAVE_SERVER_SESSN
#define STR2HOST	str2ethHost


#include <xkern/protocols/test/common_test.c>

static void
testInit( XObj self )
{
}
