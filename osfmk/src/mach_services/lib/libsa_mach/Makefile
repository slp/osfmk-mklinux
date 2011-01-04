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

VPATH			= ${${TARGET_MACHINE}_VPATH}

.if exists(${TARGET_MACHINE}/Makefile)
EXPLIB_SUBDIRS		= ${TARGET_MACHINE}
SUBDIRS			= ${TARGET_MACHINE}
.endif


LIBRARIES		= libsa_mach.a

USE_STATIC_LIBRARIES	=

EXPLIB_TARGETS		= export_libsa_mach.a

ILIST			= libsa_mach.a

LOCAL_CFLAGS            =

.include "common.mk"

.if exists(depend.mk)
.include "depend.mk"
.endif
