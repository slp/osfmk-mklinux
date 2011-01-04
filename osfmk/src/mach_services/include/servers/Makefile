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

MIG_DEFS		= service.defs
DATAFILES		= ${MIG_DEFS}
INCLUDES		= ${DATAFILES} ${_MIG_HDRS_} service_server.h

ILIST			= ${INCLUDES}
IDIR			= ${MACH3_INCLUDE_IDIR}servers/
IMODE			= 644

.include <${RULES_MK}>

service_server.h: service.defs
	${_MIG_} ${_MIGFLAGS_} \
		-server /dev/null \
		-user /dev/null \
		-header /dev/null \
		-sheader ${.TARGET} \
		${service.defs:P}
