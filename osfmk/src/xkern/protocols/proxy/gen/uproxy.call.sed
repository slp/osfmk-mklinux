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
# uproxy.call.sed
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
#
# Revision: 1.6
# Date: 1993/11/09 18:00:33
#

#
# Sed Hacks to modify the xCall routine of the 
# xk_uproxyUser MIG output 
#
# We allocate the Mach message in lproxy.c (around the request message
# if possible) and have to modify the MIG stubs to use this buffer.
# This lets us avoid copying the reply.
# 


/^	union {/,/^	register Reply/	c\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	typedef union {\
\		Request In;\
\		Reply Out;\
\	} union_mess;\
\
\	union_mess	*Mess = (union_mess *)repmsg;\
\	Msg		xReqMsg = (Msg)reqmsg;\
\	register Request *InP = &Mess->In;\
\	register Reply *Out0P = &Mess->Out;\
\
\/* \
\ * End x-kernel Sed hack\
\ */



/^	(void)memcpy((char \*) InP->reqmsg, (const char \*) reqmsg, reqmsgCnt);/	c\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	if ( xReqMsg ) {\
\    		msg2buf(xReqMsg, (char *)InP->reqmsg);\
\	}\
\/* \
\ * End x-kernel Sed hack\
\ */



/	(void)memcpy((char \*) repmsg, (const char \*) Out0P->repmsg, Out0P->repmsgCnt);/c\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\/*	(void)memcpy((char *) repmsg, (const char *) Out0P->repmsg, *repmsgCnt);*/\
\/* \
\ * End x-kernel Sed hack\
\ */


/		(void)memcpy((char \*) repmsg, (const char \*) Out0P->repmsg, \*repmsgCnt);/c\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\/*		(void)memcpy((char *) repmsg, (const char *) Out0P->repmsg, *repmsgCnt);*/\
\/* \
\ * End x-kernel Sed hack\
\ */


/	return KERN_SUCCESS;/i\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	xAssert( offsetof(Reply,repmsg) == XK_CALL_REP_OFFSET );\
\/* \
\ * End x-kernel Sed hack\
\ */



/	InP = &Mess.In;/c\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	InP = &Mess->In;\
\/* \
\ * End x-kernel Sed hack\
\ */


