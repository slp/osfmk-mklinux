# Copyright 1991-1998 by Open Software Foundation, Inc. 
#              All Rights Reserved 
#  
# Permission to use, copy, modify, and distribute this software and 
# its documentation for any purpose and without fee is hereby granted, 
# provided that the above copyright notice appears in all copies and 
# that both the copyright notice and this permission notice appear in 
# supporting documentation. 
#  
# OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
# FOR A PARTICULAR PURPOSE. 
#  
# IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
# LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
# NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
# WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
#
#
# MkLinux
#     
# uproxy.push.sed
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
#
# Revision: 1.4
# Date: 1993/11/09 18:00:33
#

#
# Sed Hacks to modify the xPush routine of the 
# xk_uproxyUser MIG output 
#

#
# As an efficiency hack, we may just build the mach message around the
# x-kernel message buffer.  If we do this, we need to record the
# address of this buffer and catch all assignments to InP or OutP in
# the stub.

/	unsigned int msgh_size_delta;/a\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	char	*machMsgStart; \
\/* \
\ * End x-kernel Sed hack\
\ */


/	InP->msgh_body/i\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	if ( ((xk_and_mach_msg_t *)xmsg)->machMsg ) {	\
\	\	xAssert( offsetof(Request,xmsg) == XK_PUSH_REQ_OFFSET );  \
\	\	InP = (Request *)((xk_and_mach_msg_t *)xmsg)->machMsg;	\
\	\	Out0P = (Reply *)((xk_and_mach_msg_t *)xmsg)->machMsg;	\
\	\	xAssert( ! ((xk_and_mach_msg_t *)xmsg)->xkMsg );	\
\	} \
\	machMsgStart = (char *)InP; \
\/* \
\ * End x-kernel Sed hack\
\ */\



/	InP = &Mess.In;/c\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	InP = (Request *)machMsgStart;\
\/* \
\ * End x-kernel Sed hack\
\ */


/^	(void)memcpy((char \*) InP->xmsg, (const char \*) xmsg, xmsgCnt);/c\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	if ( ((xk_and_mach_msg_t *)xmsg)->xkMsg ) {\
\	\	msg2buf( ((xk_and_mach_msg_t *)xmsg)->xkMsg, (char *)InP->xmsg);\
\	}\
\/* \
\ * End x-kernel Sed hack\
\ */

