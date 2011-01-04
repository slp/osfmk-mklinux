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

PURE_MACH		= 1

PROGRAMS 		= default_pager

ILIST			= default_pager
IDIR			= ${MACH3_ROOT_SERVERS_IDIR}

VPATH			= mach

NO_STARTFILES		=
USE_STATIC_LIBRARIES	=

EXPINC_SUBDIRS		= mach
COMP_SUBDIRS		=

SUBDIRS			= mach

LIBS			= -lcthreads -lmach -lsa_mach -lmach
LDFLAGS			= ${SA_MACH_LDFLAGS}

CFLAGS			= -D_ANSI_C_SOURCE ${${OBJECT_FORMAT}_EXTRA_WARNINGS}
default_pager_objectServer.o_CFLAGS =

MIG_DEFS		= default_pager_object.defs

OFILES			= default_pager.o default_pager_objectServer.o \
			  wiring.o kalloc.o strfcns.o \
			  dp_memory_object.o dp_backing_store.o

DEPENDENCIES		=

.if defined(USER_PAGER)
default_pager.o_CFLAGS	= -DUSER_PAGER ${CFLAGS}
default_pager_LIBS	= -lnetname ${LIBS}
.endif

.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif

.include <${RULES_MK}>

.if exists(depend.mk)
.include "depend.mk"
.endif
