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
 * debug.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.9.2 $
 * $Date: 1995/03/10 22:36:48 $
 */

#include <xkern/include/domain.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/platform.h>

#ifndef	MACH_KERNEL
#include <stdio.h>
#endif /* MACH_KERNEL */

#if XK_DEBUG

int
  tracebuserror,
  tracecustom,
  traceether,
  traceevent,
  tracefixme,
  traceidle,
  traceie,
  tracememoryinit,
  traceprocesscreation,
  traceprocessswitch,
  traceprotocol,
  traceprottest,
  tracetick,
  tracetrap,
  traceuser;

#endif /* XK_DEBUG */   

char	errBuf[250];

void
xError( msg )
    char	*msg;
{
    xTraceLock();
#ifndef	MACH_KERNEL
    fprintf(stderr, "%s\n", msg);
#else
    printf("%s\n", msg);
#endif  /* MACH_KERNEL */
    xTraceUnlock();
}
