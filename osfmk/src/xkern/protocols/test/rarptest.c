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
 * rarpTest.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:40:41 $
 */

/*
 * This protocol sends out a single RARP request.
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/prot/arp.h>

xkern_return_t	rarptest_init(
			      XObj );

int tracerarptestp;
  
static ETHhost TARGET;



xkern_return_t
rarptest_init(
	      XObj self)
{
  int	buf[2];
  
  printf("rarpTest_init\n");
  *(ETHhost *)buf = TARGET;
  xTrace3(rarptestp, 2, "rarpTest resolving: %x:%x:%x",
	  TARGET.high, TARGET.mid, TARGET.low);
  if (xControl(xGetDown(self, 0), RRESOLVE, (char *)buf, sizeof(ETHhost))
      < 0) {
    xTrace0(rarptestp, 0, "RARP control op failed");
    return XK_FAILURE;
  }
  xTrace4(rarptestp, 2, "rarpTest: result <%d.%d.%d.%d>",
	  ((IPhost *)buf)->a, ((IPhost *)buf)->b,
	  ((IPhost *)buf)->c, ((IPhost *)buf)->d);
  return XK_SUCCESS;
}


