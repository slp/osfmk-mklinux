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
 * xk_mig_sizes.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.6
 * Date: 1993/11/08 21:18:18
 */


#ifndef XK_MIG_SIZES_H
#define XK_MIG_SIZES_H

#define XOBJ_EXT_ID_LEN		4

/* 
 * Max number of 'int's needed to externalize a participant list.
 * 5 participants at 10 stack elements each pointing at 20 bytes (+ overhead)
 */
#define PART_EXT_BUF_LEN	( (4 + 5 * (4 + 10 * (4 + 20))) / 4 )


/* 
 * length in 'int's
 */
#define XOBJ_DUMP_LEN		( (( 8 + 3 * XOBJ_EXT_ID_LEN + 2 * 80 ) + 3) / 4 )

#define XK_INLINE_THRESHOLD	3800
#define XK_MAX_MSG_INLINE_LEN	3800
#define XK_MAX_MSG_ATTR_LEN	100

#define XK_MAX_MIG_MSG_LEN	(XK_MAX_MSG_INLINE_LEN + 	\
				 XK_MAX_MSG_ATTR_LEN + 		\
				 100 )

#define XK_MSG_INFO_LEN		2	/* size in int's */

#define XK_MAX_CTL_BUF_LEN	XK_MAX_MSG_INLINE_LEN

#endif /* ! XK_MIG_SIZES_H */
