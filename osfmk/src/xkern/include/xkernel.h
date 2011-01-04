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
 * xkernel.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * xkernel.h,v
 * Revision 1.27.1.3  1994/07/21  23:27:30  menze
 * Added xk_path.h
 *
 * Revision 1.27.1.2  1994/04/01  16:50:35  menze
 * xk_alloc.h instead of account.h
 *
 * Revision 1.27.1.1  1994/03/10  19:12:58  menze
 * Added account.h
 *
 * Revision 1.27  1994/02/05  00:05:56  menze
 *   [ 1994/01/28          menze ]
 *   Renamed assert.h to xk_assert.h
 *   Added romopt.h
 *
 */

#ifndef xkernel_h
#define xkernel_h

#include <xkern/include/domain.h>
#include <xkern/include/platform.h>
#include <xkern/include/xk_debug.h>
#include <xkern/include/upi.h>
#include <xkern/include/part.h>
#include <xkern/include/event.h>
#include <xkern/include/assert.h>
#include <xkern/include/xtime.h>
#include <xkern/include/prottbl.h>
#include <xkern/include/netmask.h>
#include <xkern/include/upi_inline.h>
#include <xkern/include/rwlock.h>
#include <xkern/include/gethost.h>
#include <xkern/include/romopt.h>
#include <xkern/include/xk_path.h>

#endif /* xkernel_h */
