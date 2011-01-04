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

.if !defined(_OSF_PROG_MK_)
_OSF_PROG_MK_=

#
#  Build rules
#
.if defined(PROGRAMS)

_START_FILES_=${_START_FILE_0_} ${_START_FILE_I_}
_END_FILES_=${_START_FILE_N_}

${PROGRAMS}: $${_OFILES_}
.if defined(VISTALIB)
	${_CC_} ${_LDFLAGS_} ${_START_FILES_} -o ${.TARGET}.X ${_OFILES_} ${VISTA:D${VISTALIB}} ${_LIBS_} ${_END_FILES_}
.else
	${_CC_} ${_LDFLAGS_} ${_START_FILES_} -o ${.TARGET}.X ${_OFILES_} ${_LIBS_} ${_END_FILES_}
.endif
.if defined(VISTA_LIST)
#	no .X file created
.else
	${MV} ${.TARGET}.X ${.TARGET}
.endif

.endif

.endif
