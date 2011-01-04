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


EXPINC_SUBDIRS		= ${${TARGET_MACHINE}_SUBDIRS} sys
COMP_SUBDIRS		=
SUBDIRS			= ${${TARGET_MACHINE}_SUBDIRS}

DATAFILES		= string.h stdio.h stdlib.h errno.h \
			  ctype.h  stdarg.h va_list.h types.h \
			  math.h float.h ieeefloat.h \
			  ${${TARGET_MACHINE}_DATAFILES}
LINKS			= machine ${${TARGET_MACHINE}_LINKS}
OTHERS			= ${LINKS} ${${TARGET_MACHINE}_OTHERS}

INCLUDES		= ${DATAFILES} ${OTHERS}

ILIST			= ${INCLUDES}
IDIR			= ${MACH3_INCLUDE_IDIR}sa_mach/
IMODE			= 644

.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>

machine: ${ALWAYS}
	${RM} ${_RMFLAGS_} machine
	ln -s ${MACHINE} machine
