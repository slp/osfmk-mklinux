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


#ifndef bootp_internal_h
#define bootp_internal_h

#define MAXSTRING  10

typedef struct {
        xkSemaphore 	sem;
        Msg_s           request;
	Event           event;
	XObj            lls;
	int             max_attempts;
	int             wait;
	int             min_wait;
	int             max_wait;
	xkern_return_t	reply_value;
	ETHhost         ethaddr;
	IPhost          ipaddr;
	IPhost          gwipaddr;
	IPhost          tsipaddr;
	IPhost          mcipaddr;
	u_bit32_t       nodename;
	char            hostname[MAXSTRING+1];
	char            clustername[MAXSTRING+1];
	u_bit32_t       clusterid;
} PSTATE;

#define BOOTP_STACK_SIZE 700   /* ad abundantiam */

#define OUR_VENDOR_TAG VM_RFC1048

#define BOOTP_INVALID_ID  ((u_bit32_t)-1)

#endif /* bootp_internal_h */





