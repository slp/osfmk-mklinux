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

VPATH         		:= .:${EXPORTBASE}/usr/comm

LIBRARIES       	= libdtime_mgmt.a

EXPLIB_TARGETS          = export_${LIBRARIES}

USE_STATIC_LIBRARIES    =

OFILES			= dtime_mgmt.o

ILIST                   = ${LIBRARIES}
IDIR                    = /usr/comm/s_xkern/lib/
IMODE                   = 644

INCFLAGS                = -I${EXPORTBASE}/usr/comm -I${MAKETOP}../src
MDINCFLAGS              = -I${EXPORTBASE}/usr/comm -I${MAKETOP}../src

DEPENDENCIES    	=

.include <${RULES_MK}>

.if exists(depend.mk)
.include "depend.mk"
.endif


