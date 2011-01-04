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

USE_STATIC_LIBRARIES	= YES
DEPENDENCIES		= YES

CFLAGS			+= ${${TARGET_MACHINE}_CFLAGS}
INCFLAGS                = -I. -I../include ${INCDIRS} \
			  -I${EXPORTBASE}/mach_services/cmds/mach_perf
EXTRA_LIBDIRS		= -L../lib
LDFLAGS			+= ${${TARGET_MACHINE}_LDFLAGS} ${SA_LDFLAGS}
LIBS			= -lperf ${SA_LIBMACH} -lnetname \
			  ${${TARGET_MACHINE}_LIBS}

COMMON_INK_LDFLAGS	= ${${TARGET_MACHINE}_LDFLAGS} ${SA_LDFLAGS} \
			  ${INK_LDFLAGS}
COMMON_INK_LIBS		= -lperf ${INK_SA_LIBMACH} -lnetname \
			  ${${TARGET_MACHINE}_LIBS}
COMMON_IDIR		= ${MACH3_ROOT_SERVERS_IDIR}mach_perf.d/
