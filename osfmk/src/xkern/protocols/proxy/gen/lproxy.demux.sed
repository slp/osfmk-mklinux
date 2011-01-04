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
# lproxy.demux.sed
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
# Sed hacks for the xDemux and xCallDemux stubs of the xk_lproxyUser
# MIG output. 
#
# The MIG stubs try to allocate the Mach message buffer on the stack, but
# that tends to blow the small in-kernel stack, so instead we allocate
# the Mach message buffer in uproxy.c and have to modify the MIG stubs
# to use this buffer.  (This also lets us avoid copying the reply data
# in callDemux.
# 
# We also try (in uproxy.c) to build the Mach message around a
# pre-existing x-kernel buffer if possible (this worked if
# ((xk_and_mach_msg_t *)req)->xkMsg is 0 in the stub.) 
#


#our mig version
/^	(void)memcpy((char \*) InP->req, (const char \*) req, reqCnt);/c\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	xAssert( offsetof(Request, req) == XK_DEMUX_REQ_OFFSET); \
\	if ( ((xk_and_mach_msg_t *)req)->xkMsg ) { \
\	    msg2buf(((xk_and_mach_msg_t *)req)->xkMsg, (char *) InP->req); \
\	} \
\/* \
\ * End x-kernel Sed hack\
\ */



/^	union {/c\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	typedef union {

/^	} Mess;/c\
\	} union_mess;\
\
\	union_mess	*Mess = (union_mess *)((xk_and_mach_msg_t *)req)->machMsg; \
\/* \
\ * End x-kernel Sed hack\
\ */



#alternate mig version
/^	(void)memcpy((char \*) rep, (const char \*) Out0P->rep, Out0P->repCnt);/c\
/* \
\ * x-kernel Sed hack deleted memcopy: rep (Msg) already has reply data \
\ *	(void)memcpy((char *) rep, (const char *) Out0P->rep, Out0P->repCnt);\
\ */\

#alternate mig version
/^		(void)memcpy((char \*) rep, (const char \*) Out0P->rep, \*repCnt);/c\
/* \
\ * x-kernel Sed hack deleted memcopy\
\ *		(void)memcpy((char *) rep, (const char *) Out0P->rep, *repCnt);\
\ */\



/&Mess\./s/Mess\./Mess->/


