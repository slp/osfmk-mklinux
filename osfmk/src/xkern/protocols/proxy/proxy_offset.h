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
 * proxy_offset.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * Revision: 1.2
 * Date: 1993/04/08 17:28:17
 */

#ifndef PROXY_OFFSET_H
#define PROXY_OFFSET_H


#include "xk_mig_sizes.h"

/* 
 * We determine the offsets of certain fields within certain MIG
 * request structures.  We do this because we need to know these
 * offsets before we get to the routines where the actual structure is
 * defined.  If the defs file is modified in such a way that the
 * structure offset changes, assertions will be tripped in the stub
 * code.  This isn't pretty ...
 */



typedef struct {
	mach_msg_header_t Head;
	/* start of the kernel processed data */
	mach_msg_body_t msgh_body;
	mach_msg_port_descriptor_t domPort;
	mach_msg_ool_descriptor_t msgOol;
	/* end of the kernel processed data */
	NDR_record_t NDR;
	xobj_ext_id_t lls;
	mach_msg_type_number_t reqCnt;
	char req[XK_INLINE_THRESHOLD];
	int msgStart;
	mach_msg_type_number_t msgOolCnt;
	mach_msg_type_number_t reqAttrCnt;
	char reqAttr[XK_MAX_MSG_ATTR_LEN];
} XkDemuxRequest;

#define XK_DEMUX_REQ_OFFSET	(offsetof(XkDemuxRequest, req))


typedef struct {
	mach_msg_header_t Head;
	/* start of the kernel processed data */
	mach_msg_body_t msgh_body;
	mach_msg_ool_descriptor_t xmsgool;
	/* end of the kernel processed data */
	NDR_record_t NDR;
	xobj_ext_id_t lls;
	mach_msg_type_number_t xmsgCnt;
	char xmsg[XK_INLINE_THRESHOLD];
	mach_msg_type_number_t xmsgoolCnt;
	mach_msg_type_number_t attrCnt;
	char attr[XK_MAX_MSG_ATTR_LEN];
	xk_path_t path;
} XkPushRequest;

#define XK_PUSH_REQ_OFFSET	(offsetof(XkPushRequest, xmsg))


typedef struct {
	mach_msg_header_t Head;
	/* start of the kernel processed data */
	mach_msg_body_t msgh_body;
	mach_msg_ool_descriptor_t repool;
	/* end of the kernel processed data */
	NDR_record_t NDR;
	xkern_return_t retVal;
	mach_msg_type_number_t repCnt;
	char rep[XK_INLINE_THRESHOLD];
	mach_msg_type_number_t repoolCnt;
	mach_msg_trailer_t trailer;
} XkCallReply;

#define XK_CALL_REP_OFFSET	(offsetof(XkCallReply, rep))


#endif /* ! PROXY_OFFSET_H */
