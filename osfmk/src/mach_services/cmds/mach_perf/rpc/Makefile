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
# MkLinux

PROGRAMS		= rpc_perf rpc_perf.INK

ILIST			= rpc_perf
IDIR			= ${COMMON_IDIR}

MIGFLAGS		+= -t
MIG_DEFS		= rpc.defs

OFILES			= rpc_perf.o \
			  rpc_server_main.o \
			  rpc_server.o \
			  rpc_user.o

rpc_perf.INK_LDFLAGS	+= ${COMMON_INK_LDFLAGS}
rpc_perf.INK_LIBS	= ${COMMON_INK_LIBS}

.include "../${TARGET_MACHINE}/machdep.mk"

.include "../monitor/common.mk"

.include <${RULES_MK}>

rpc_perf.o: rpc_user.h

rpc_server_main.o: rpc_server.h

rpc_server.c rpc_user.c rpc_server.h rpc_user.h: rpc.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} \
	-server rpc_server.c \
	-user rpc_user.c \
	-sheader rpc_server.h \
	-header rpc_user.h

${PROGRAMS}: ../lib/libperf.a

.if exists(depend.mk)
.include "depend.mk"
.endif
