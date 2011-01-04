# 
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

EXPINC_SUBDIRS = prot

INCLUDES = assert.h compose.h debug.h dlist.h domain.h event.h event_i.h\
	   gc.h gethost.h idmap.h intern_alloc.h list.h msg.h msg_s.h\
	   netmask.h part.h platform.h prottbl.h romopt.h rwlock.h trace.h\
	   upi.h upi_inline.h xk_alloc.h xk_debug.h xk_malloc.h xk_path.h\
	   xk_thread.h xkernel.h xtime.h xtype.h

ILIST                   = ${INCLUDES}
IDIR                    = /usr/comm/xkern/include/
IMODE                   = 644

.include <${RULES_MK}>
