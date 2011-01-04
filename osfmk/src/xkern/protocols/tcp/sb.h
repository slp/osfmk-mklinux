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
 * sb.h
 *
 * Derived from:
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * Modified for x-kernel v3.1	12/10/90
 * Modifications Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 * sb.h,v
 * Revision 1.5.1.2.1.2  1994/09/07  04:18:34  menze
 * OSF modifications
 *
 * Revision 1.5.1.2  1994/09/01  19:44:17  menze
 * Meta-data allocations now use Allocators and Paths
 * XObj initialization functions return xkern_return_t
 * Events are allocated and rescheduled
 *
 */

#ifndef sb_h
#define sb_h

#define TCP_BUFFER_SPACE (4 * 1024)
#define sbhiwat(sb) ((sb)->hiwat)

struct sb {
  struct sb_i *next, *prev;
  int hiwat;
  int len;
};

struct sb_i {
  struct sb_i *next, *prev;
  Msg_s m;
};

#define sblength(sb) ((sb)->len)
#define sbspace(sb)  (sbhiwat(sb)-sblength(sb))
#define sbinit(sb) { \
		       (sb)->next = (sb)->prev = (struct sb_i*)(sb); \
		       (sb)->len=0; \
		       (sb)->hiwat=TCP_BUFFER_SPACE; \
		   }

extern struct sb_i *sbifreelist;
#define sbinew(s, path) { if ((s) = sbifreelist) sbifreelist = (s)->next; else (s) = pathAlloc((path), sizeof(struct sb_i)); }
#define sbifree(s) { (s)->next = sbifreelist; sbifreelist = (s); }


extern xkern_return_t	sbappend( struct sb *, Msg, XObj );
extern xkern_return_t 	sbcollect( struct sb *, Msg, int off, int len,
				   int delete, XObj );
extern void 	sbflush( struct sb * );
extern void 	sbdrop( struct sb * , int len, XObj );
extern void 	sbdelete( struct sb * );

#endif
