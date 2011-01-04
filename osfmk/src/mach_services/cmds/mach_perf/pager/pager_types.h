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

#define PAGER_SERVER_NAME "pager perf server"

#define PAGER_MSG_BUF_SIZE (8192*2)

#define READ_PATTERN 0xdeadbeef
#define WRITE_PATTERN 0xfeedbeef

/*
 * Pager flags used at object creation time
 */

#define PRECIOUS 	0x01
#define CACHED	 	0x02
#define FILL		0x04	/* Pager fills in page */
#define NO_LISTEN	0x08	/* pager will not listen on the object port */
#define NO_INIT 	0x10	/* pager will ignore memory_object_init() */
#define NO_TERMINATE 	0x20	/* pager will ignore terminate request */

#define GET_ATTR	0
#define SET_ATTR	1
