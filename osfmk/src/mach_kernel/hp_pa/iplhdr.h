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
 * Copyright (c) 1990 mt Xinu, Inc.  All rights reserved.
 *
 * This file may be freely distributed in any form as long as
 * this copyright notice is included.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	Utah $Hdr: iplhdr.h 1.3 91/09/25$
 */

/*
 * This structure goes at the absolute beginning of any boot media, and
 * points to the boot code, which is one module only, limited to 64kb.
 *
 * The code pointed to by the following structure is checksummed by the
 * "IPL code", summing each longword of the entire area and checking
 * against zero.
 */

struct	ipl_image {
	u_short	ipl_id;		/* gets IPL_ID (see below) if valid */
	char	ipl_unused[238];/* ignored */
	u_int	ipl_addr;	/* code address (bytes - must be at 2k bdry) */
	u_int	ipl_size;	/* size of actual code (in bytes - mpl of 2k) */
	u_int	ipl_entry;	/* byte offset into code to start execution */
};

#define IPL_ID		0x8000
#define IPL_BLOCK	2048
