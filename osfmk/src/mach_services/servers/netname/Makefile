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

PURE_MACH		= 1

PROGRAMS		= name_server

ILIST			= ${PROGRAMS} ${INCLUDES}
IDIR			= ${MACH3_INCLUDE_IDIR}servers/
IMODE			= 644
name_server_IDIR	= ${MACH3_ROOT_SERVERS_IDIR}
name_server_IMODE	= 755

NO_STARTFILES		=
USE_STATIC_LIBRARIES	=

LIBS			= -lservice -lmach -lsa_mach -lmach
LDFLAGS			= ${SA_MACH_LDFLAGS}
GCC_WARNINGS		= -Werror
ICC_WARNINGS		=

CFLAGS 			= -D_ANSI_C_SOURCE \
			  ${${OBJECT_FORMAT}_EXTRA_WARNINGS} \
			  ${${CC_COMPILER}_WARNINGS}

netnameServer.o_CFLAGS	= -D_ANSI_C_SOURCE
notify_server.o_CFLAGS	= -D_ANSI_C_SOURCE
OFILES			= netname.o nprocs.o netnameServer.o

MIG_DEFS		= netname.defs

DATAFILES		= netname_defs.h netname.h netname_server.h ${MIG_DEFS}
INCLUDES		= ${DATAFILES}

EXPDIR			= ${MACH3_INCLUDE_IDIR}servers/

DEPENDENCIES		=

.include <${RULES_MK}>

netname_server.h: netname.defs
	${_MIG_} ${_MIGFLAGS_} \
		-server /dev/null \
		-user /dev/null \
		-header /dev/null \
		-sheader ${.TARGET} \
		${netname.defs:P}

.if exists(depend.mk)
.include "depend.mk"
.endif
