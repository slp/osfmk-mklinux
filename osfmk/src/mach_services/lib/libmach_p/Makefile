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

USE_STATIC_LIBRARIES	=

LOCAL_LIBMACH		= libmach_p.a
LOCAL_CFLAGS		= -DGPROF ${MD_PGFLAGS}
LOCAL_MIGFLAGS		=

EXPLIB_TARGETS		= export_${LOCAL_LIBMACH}

MD_PGFLAGS	       ?= -pg

IDIR			= ${MACH3_LIBRARY_IDIR}
ILIST			= ${LOCAL_LIBMACH}
IMODE			= 644

.include "../libmach/common.mk"
