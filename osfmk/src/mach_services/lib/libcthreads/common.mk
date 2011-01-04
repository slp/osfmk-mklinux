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
# 

PURE_MACH		= 1

USE_STATIC_LIBRARIES	=

CFLAGS			+= ${${TARGET_MACHINE}_CFLAGS} \
			   ${${OBJECT_FORMAT}_EXTRA_WARNINGS} ${WERROR} \
			   -DMACH_IPC_COMPAT=0

INCFLAGS		+= ${${TARGET_MACHINE}_INCFLAGS}

OFILES			+= cthreads.o malloc.o mig_support.o \
			  stack.o sync.o exp_malloc.o structs.o \
			  ${${TARGET_MACHINE}_OFILES}

ILIST			= ${LIBRARIES}
IDIR			= ${MACH3_LIBRARY_IDIR}
IMODE			= 644

DEPENDENCIES		=
