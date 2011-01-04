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

${TARGET_MACHINE}_VPATH		= ${TARGET_MACHINE}

${TARGET_MACHINE}_DEFINES	= -DTHREAD_STATE=PPC_THREAD_STATE -DTHREAD_STATE_COUNT=PPC_THREAD_STATE_COUNT 
${TARGET_MACHINE}_CFLAGS	= 
${TARGET_MACHINE}_LDFLAGS	= -static
${TARGET_MACHINE}_LIBS		=

SFILES	    	                = setjmp.o hw_lock.o

${TARGET_MACHINE}_OFILES	= ${SFILES} fpu.o sqrt.o \
				  hw_regs.o lock.o hw_timer.o

MACH_KERNEL_CONFIG?=PRODUCTION
SYMBOL_FILE = ../../../../mach_kernel/${MACH_KERNEL_CONFIG}/mach_kernel.${MACH_KERNEL_CONFIG}

.if exists(${SYMBOL_FILE})
sym.c:	${SYMBOL_FILE}
	echo "#include <mach_perf.h>" > sym.c
	echo "struct symbol kernel_symbols[] = {" >> sym.c
	${${OBJECT_FORMAT}_CC_EXEC_PREFIX}nm ${SYMBOL_FILE} | awk '$$2 == "T" { print  "(unsigned)0x"$$1", \""$$3}' | sed -e 's/ *$$/\", 0,/' >> sym.c
	echo "(unsigned) 0, 0, 0};" >> sym.c
	echo "unsigned int kernel_symbols_count = (sizeof(kernel_symbols)/sizeof(struct symbol)) - 1;" >> sym.c
	echo char *kernel_symbols_version = \"`strings -a ${SYMBOL_FILE} | grep 'Mach 3.0 VERSION'`\\n\"\; >>sym.c
.else
sym.c:	
	echo "#include <mach_perf.h>" > sym.c
	echo "struct symbol kernel_symbols[] = {(unsigned) 0, 0, 0};" >> sym.c
	echo "unsigned int kernel_symbols_count = 0;" >> sym.c
	echo char *kernel_symbols_version = \"\"\; >>sym.c
.endif

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
