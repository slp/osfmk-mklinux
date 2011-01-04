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

PURE_MACH		= 1

VPATH			= ${MAKETOP}mach_services/servers/machid

LIBRARIES		= libmachid.a

USE_STATIC_LIBRARIES	=

EXPLIB_TARGETS		= export_libmachid.a

ILIST			= libmachid.a
IDIR			= ${MACH3_LIBRARY_IDIR}
IMODE			= 644

MIG_DEFS		= machid.defs machid_debug.defs
MIGFLAGS		= ${DEFINES} ${INCFLAGS} -nostdinc
machidUser.c_MIGFLAGS    = -DMIG_USER
machid_debugUser.c_MIGFLAGS    = -DMIG_USER
TRUNCOFILES		= machid_DUser.o
OFILES			= machid_lib.o \
			  machidUser.o \
			  ${TRUNCOFILES}

GARBAGE			= machidUser.c machid_debugUser.c

INCFLAGS                = -I.

.include <${RULES_MK}>

machid_DUser.o: machid_debugUser.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@
