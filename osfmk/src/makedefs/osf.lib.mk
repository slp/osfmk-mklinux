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

.if !defined(_OSF_LIB_MK_)
_OSF_LIB_MK_=


#  Build rules
#
.if defined(LIBRARIES)

#${LIBRARIES}: $${_LIBRARY_OFILES_}

${LIBRARIES}: $${.TARGET}($${_OFILES_})
	${_AR_} ${DEF_ARFLAGS} ${.TARGET} ${.OODATE}
.if defined(ORDER_LIBRARIES)
	${RM} -rf tmp
	mkdir tmp
	cd tmp && { \
	    ${_AR_} x ../${.TARGET}; \
	    ${RM} -f __.SYMDEF __________ELELX; \
	    ${_AR_} cr ${.TARGET} ${${OBJECT_FORMAT}_LORDER}; \
	}
	${MV} -f tmp/${.TARGET} .
	${RM} -rf tmp
.endif
	${_RANLIB_} ${.TARGET}
	${RM} -f ${.OODATE}

.endif

.if defined(SHARED_LIBRARIES)

_START_FILES_=${_START_FILE_I_}
_END_FILES_=${_START_FILE_N_}

${SHARED_LIBRARIES}: $${_OFILES_}
.if defined(ORDER_LIBRARIES)
	${_LD_} ${_SHLDFLAGS_} -o ${.TARGET}.X ${_START_FILES_} ${${OBJECT_FORMAT}_LORDER} ${_END_FILES_} ${_LIBS_}
.else
	${_LD_} ${_SHLDFLAGS_} -o ${.TARGET}.X ${_START_FILES_} ${_OFILES_} ${_END_FILES_} ${_LIBS_}
.endif
	${MV} ${.TARGET}.X ${.TARGET}

.endif

.endif
