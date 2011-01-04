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

USE_STATIC_LIBRARIES		=

${TARGET_MACHINE}_CFLAGS        = -nostdinc -DTHREAD_STATE=PPC_THREAD_STATE -DTHREAD_STATE_COUNT=PPC_THREAD_STATE_COUNT
${TARGET_MACHINE}_LDFLAGS       = -static -nostdlib -L${${OBJECT_FORMAT}_GCC_EXEC_PREFIX}/../lib -Xlinker -Ttext -Xlinker 0x10000000 -Xlinker -Tdata -Xlinker 0x10200000 
${TARGET_MACHINE}_LIBS          =

.if     PURE_MACH
SYSTEM_LIBS                     =
.else
SYSTEM_LIBS                     = -lm -lc
.endif

SA_LDFLAGS                      = -e __start_mach -u __start_mach
INK_LDFLAGS 			=
SA_LIBMACH                      = -lsa_mach -lmach -lsa_mach ${SYSTEM_LIBS}
INK_SA_LIBMACH                  = ${SA_LIBMACH}

.SUFFIXES: .S .s

${SFILES:.o=.S}: $${@:.S=.s}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${.ALLSRC} ${.TARGET}

${SFILES}: $${.TARGET:.o=.S}
	${_CC_} -E ${_CCFLAGS_} -DASSEMBLER -D__LANGUAGE_ASSEMBLY ${${.TARGET:.o=.S}:P} \
		> ${.TARGET:.o=.pp}
	${SED} '/^#/d' ${.TARGET:.o=.pp} > ${.TARGET:.o=.s}
	${_CC_} ${_CCFLAGS_} -m601 -Wa,-Z -c ${.TARGET:.o=.s}
	${RM} ${_RMFLAGS_} ${.TARGET:.o=.pp} ${.TARGET:.o=.s}
