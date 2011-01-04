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

VPATH                   = ${${TARGET_MACHINE}_VPATH}:${MAKETOP}mach_kernel/mach:${MAKETOP}mach_kernel/mach_debug:${MAKETOP}mach_kernel/device:${MAKETOP}default_pager/mach

OPT_LEVEL		= -O2 -g

DEPENDENCIES		=

LIBRARIES		= libflipc.a

EXPLIB_TARGETS		= export_libflipc.a

ILIST			= libflipc.a
IDIR			= ${MACH3_LIBRARY_IDIR}
IMODE			= 644

INCFLAGS                = -I${TARGET_MACHINE}
DEFINES			= 
CFLAGS			= ${DEFINES}
#CFLAGS			= ${DEFINES} -Wall
MIGFLAGS		= ${DEFINES}
MIG_DEFS		= 
OFILES			= flipc_domain.o flipc_alloc.o flipc_endpoint.o \
			  flipc_epgroup.o flipc_buffer.o \
			  ${${TARGET_MACHINE}_OFILES}
			   
.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>



