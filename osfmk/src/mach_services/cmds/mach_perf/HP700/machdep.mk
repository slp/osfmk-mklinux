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

#
#	Generic machine-specific make rules.
#

${TARGET_MACHINE}_CFLAGS	= -nostdinc -DTHREAD_STATE=${TARGET_MACHINE}_THREAD_STATE -DTHREAD_STATE_COUNT=${TARGET_MACHINE}_THREAD_STATE_COUNT
${TARGET_MACHINE}_LDFLAGS	= -static -nostdlib
${TARGET_MACHINE}_LIBS		= 

.if	PURE_MACH
SYSTEM_LIBS			=
.else
SYSTEM_LIBS			= -lm -lc
.endif

SA_LDFLAGS			= 
INK_LDFLAGS 			=  -Wl,-R50000000
SA_LIBMACH			= -lsa_mach -lmach -lsa_mach ${SYSTEM_LIBS}
INK_SA_LIBMACH			= ${SA_LIBMACH}
