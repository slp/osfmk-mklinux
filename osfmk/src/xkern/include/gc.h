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
 * gc.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * gc.h,v
 * Revision 1.6.1.2.1.2  1994/09/07  04:18:04  menze
 * OSF modifications
 *
 * Revision 1.6.1.2  1994/09/01  04:08:19  menze
 * initSessionCollector returns the event
 *
 * Revision 1.6  1993/12/10  02:38:56  menze
 * Fixed #endif
 *
 */


#ifndef gc_h
#define gc_h

#include <xkern/include/xkernel.h>
#include <xkern/include/idmap.h>

/*
 * initSessionCollector -- start a session garbage collector to run every
 * 'interval' microseconds, collecting  idle sessions on map 'm'
 * (an idle session is one whose ref count is zero.)  A session is collected
 * by calling the function 'destructor'.   Protocols mark a
 * session as 'non-idle' by clearing it's 'idle' field.
 *
 * 'msg' is a string used in trace statements to identify the collector.
 * (try to use a string unique to the map, such as a protocol name)
 * A null pointer is interpreted as an empty string.
 *
 * A session collector may be shut down by cancelling the returned event. 
 */

Event initSessionCollector(
	Map m,
	int interval,
    	void (*destructor)(XObj s), 
	Path p,
    	char *msg);

#endif	/* gc_h */
