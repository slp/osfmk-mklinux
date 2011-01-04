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
# lproxyServer.sed
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
#
# Revision: 1.3
# Date: 1993/04/08 17:27:39
#

#
# Sed hacks for the xk_lproxyServer MIG output. 
#

/bcopy/d

/	(\*routine) (InHeadP, OutHeadP);/a\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\#ifdef XK_PROXY_MSG_HACK \
\	if ( routine != _XxDemux && routine != _XxCallDemux ) {\
\	\    allocFree((char *)InHeadP);\
\	}\
\#endif\
\/* \
\ * End x-kernel Sed hack\
\ */\



/RetCode = do_lproxy_xDemux/i\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	In1P->msgStart = (int)InHeadP;\
\/* \
\ * End x-kernel Sed hack\
\ */\



/RetCode = do_lproxy_xCallDemux/i\
/* \
\ * Begin x-kernel Sed hack\
\ */\
\	In1P->reqMsgStart = (int)InHeadP;\
\/* \
\ * End x-kernel Sed hack\
\ */\


