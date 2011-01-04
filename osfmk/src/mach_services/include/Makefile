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

EXPINC_SUBDIRS		= sa_mach servers ${${TARGET_MACHINE}_SUBDIRS}
COMP_SUBDIRS		=
SUBDIRS			= sa_mach servers ${${TARGET_MACHINE}_SUBDIRS}

DATAFILES		= cthreads.h cthread_filter.h errorlib.h \
			  flipc.h flipc_config.h flipc_macros.h \
			  flipc_return.h flipc_ui.h \
			  mach.h mach_error.h mach_init.h mach_setjmp.h \
			  mig_errors.h msg_type.h port_obj.h  rthreads.h \
			  rthread_filter.h sa_mach.h \
			  ${${TARGET_MACHINE}_DATAFILES}
LINKS			= machine ${${TARGET_MACHINE}_LINKS}
OTHERS			= ${LINKS} ${${TARGET_MACHINE}_OTHERS}

INCLUDES		= ${DATAFILES} ${OTHERS}

ILIST			= ${INCLUDES}
IDIR			= ${MACH3_INCLUDE_IDIR}
IMODE			= 644

.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>

machine: ${ALWAYS}
	${RM} ${_RMFLAGS_} machine
	ln -s ${MACHINE} machine
