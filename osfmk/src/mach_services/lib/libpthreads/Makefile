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
# POSIX Pthreads Library
#

PURE_MACH		= 1

VPATH			= ${${TARGET_MACHINE}_VPATH}

INCFLAGS		= -I. -I${${TARGET_MACHINE}_INCDIRS}

INCLUDES		=
PRIVATE_INCLUDES	= pthread_internals.h posix_sched.h

LIBRARIES		= libpthread.a

EXPINC_TARGETS		= export_pthread.h export_pthread_impl.h
EXPLIB_TARGETS		= export_libpthread.a

CFLAGS			= -D__POSIX_LIB__ -DMACH_IPC_COMPAT=0 ${INCFLAGS} \
			  ${${TARGET_MACHINE}CFLAGS} \
			  ${${OBJECT_FORMAT}_EXTRA_WARNINGS} -Werror
OFILES			= pthread.o pthread_mutex.o pthread_cond.o \
			  pthread_tsd.o fprintf.o \
			  ${${TARGET_MACHINE}_OFILES}

ILIST			= ${LIBRARIES} ${INCLUDES} ${${TARGET_MACHINE}_HFILES}
libpthread.a_IDIR	= ${MACH3_LIBRARY_IDIR}
IDIR			= ${MACH3_INCLUDE_IDIR}

IMODE			= 644

.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif

.include <${RULES_MK}>
