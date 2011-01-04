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
# Machine specific support
#

${TARGET_MACHINE}_VPATH         = ${TARGET_MACHINE}
${TARGET_MACHINE}_INCDIRS       = ${TARGET_MACHINE}

LOCAL_CFLAGS            = -msoft-float -fno-omit-frame-pointer

SFILES	= i386_lock.o i386_stack.o 

${TARGET_MACHINE}_HFILES = pthread_impl.h
pthread_impl.h_IDIR = ${MACH3_INCLUDE_IDIR}machine/

${TARGET_MACHINE}_OFILES= ${SFILES} i386_thread.o

${SFILES}: $${.TARGET:.o=.s}
	${RM} ${_RMFLAGS_} ${.TARGET:.o=.S}
	${CP} ${${.TARGET:.o=.s}:P} ${.TARGET:.o=.S}
	${_CC_} -DASSEMBLER ${_CCFLAGS_} -Wa,-Z -c ${.TARGET:.o=.S}

pthread_impl.h: mk_pthread_impl
	./mk_pthread_impl >pthread_impl.h

mk_pthread_impl_CCTYPE=host
mk_pthread_impl: mk_pthread_impl.o
	${_CC_} -o mk_pthread_impl mk_pthread_impl.o

mk_pthread_impl.o_CCTYPE=host
mk_pthread_impl.o: pthread.h pthread_internals.h
	${_CC_} ${_GENINC_} ${MACH3_INCDIRS} -D__POSIX_LIB__ -c \
		${mk_pthread_impl.c:P}
