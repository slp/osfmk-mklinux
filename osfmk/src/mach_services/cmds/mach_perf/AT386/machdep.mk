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

USE_STATIC_LIBRARIES		=

${TARGET_MACHINE}_CFLAGS	= -nostdinc -DTHREAD_STATE=${MACHINE}_THREAD_STATE -DTHREAD_STATE_COUNT=${MACHINE}_THREAD_STATE_COUNT
${TARGET_MACHINE}_LIBS          =

.if	PURE_MACH
${TARGET_MACHINE}_LDFLAGS	= -nostdlib
SYSTEM_LIBS			=
.else
${TARGET_MACHINE}_LDFLAGS	= 
SYSTEM_LIBS			= -lm -lc
.endif

#
# standalone flags to retrieve the correct crt0 for argc/argv
#
.if	PURE_MACH
SA_LDFLAGS			= -e __start_mach -u __start_mach
.else
SA_LDFLAGS			= 
.endif

#
# collocated option
#
INK_LDFLAGS 			= -Xlinker -Ttext -Xlinker 0xd0000000
#
# standalone mach libraries
#
SA_LIBMACH			= -lsa_mach -lmach -lsa_mach ${SYSTEM_LIBS}
#
# standalone mach libraries for collocated tasks
# NOTE: tests revealed that -maxonstack was not necessary for mach_perf
# to execute correctly. However, in case strange things happen such as
# spontaneous reboot under collocation, consider linking against a newly
# built libmach library with maxonstack set.
#
INK_SA_LIBMACH			= -lsa_mach -lmach -lsa_mach ${SYSTEM_LIBS}

.SUFFIXES: .S .s

${SFILES:.o=.S}: $${@:.S=.s}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${.ALLSRC} ${.TARGET}

${SFILES}: $${@:.o=.S}
	${_CC_} -E ${_CCFLAGS_} -DASSEMBLER -D__LANGUAGE_ASSEMBLY ${${.TARGET:.o=.S}:P} \
		> ${.TARGET:.o=.pp}
	${SED} '/^#/d' ${.TARGET:.o=.pp} > ${.TARGET:.o=.s}
	${_CC_} ${_CCFLAGS_} -c ${.TARGET:.o=.s}
	${RM} ${_RMFLAGS_} ${.TARGET:.o=.pp} ${.TARGET:.o=.s}
