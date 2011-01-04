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

${TARGET_MACHINE}_VPATH		= HP700

${TARGET_MACHINE}_DEFINES	= -DTHREAD_STATE=${TARGET_MACHINE}_THREAD_STATE -DTHREAD_STATE_COUNT=${TARGET_MACHINE}_THREAD_STATE_COUNT 
${TARGET_MACHINE}_LDFLAGS	= -static
${TARGET_MACHINE}_LIBS		=

.if	PURE_MACH
${TARGET_MACHINE}_SYSTEM_SFILES	= sqrt.o fabs.o
${TARGET_MACHINE}_SYSTEM_FILES	= csqrt.o cfabs.o
.else
${TARGET_MACHINE}_SYSTEM_SFILES	= 
${TARGET_MACHINE}_SYSTEM_FILES	= 
.endif

SFILES	    	                = setjmp.o slock.o \
				  read_timer.o \
				  ${${TARGET_MACHINE}_SYSTEM_SFILES}

${TARGET_MACHINE}_OFILES	= ${SFILES} hp700_regs.o lock.o hp700_timer.o \
				  ${${TARGET_MACHINE}_SYSTEM_FILES}

.SUFFIXES: .S .s


${SFILES:.o=.S}: $${.TARGET:.S=.s}
	${RM} ${_RMFLAGS_} ${.TARGET} ${.TARGET:.S=.pp}
	${CP} ${${.TARGET:.S=.s}:P} ${.TARGET}
	${_CC_} -E ${_CCFLAGS_} -DASSEMBLER ${.TARGET} \
		> ${.TARGET:.S=.pp}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${MV} ${.TARGET:.S=.pp} ${.TARGET}

${SFILES}: $${.TARGET:.o=.S}
	${AS} ${.TARGET:.o=.S} -o ${.TARGET}

MACH_KERNEL_CONFIG?=PRODUCTION
SYMBOL_FILE = ../../../../mach_kernel/${MACH_KERNEL_CONFIG}/mach_kernel.${MACH_KERNEL_CONFIG}

.if exists(${SYMBOL_FILE})
sym.c:	${SYMBOL_FILE}
	echo "#include <mach_perf.h>" > sym.c
	echo "struct symbol kernel_symbols[] = {" >> sym.c
	${SOM_CC_EXEC_PREFIX}/nm ${SYMBOL_FILE}| grep -i " t " | grep -v ' L\$$' | grep -v ' \$$.*\$$$$' | awk ' { print  "(unsigned)0x"$$1", \""$$3}' | sed -e 's/ *$$/\", 0,/' >> sym.c
	echo "(unsigned) 0, 0, 0};" >> sym.c
	echo "unsigned int kernel_symbols_count = (sizeof(kernel_symbols)/sizeof(struct symbol)) - 1;" >> sym.c
	echo char *kernel_symbols_version = \"`strings -a ${SYMBOL_FILE} | grep ${MACH_KERNEL_CONFIG}`\\\\n\"\; >>sym.c
.else
sym.c:	
	echo "#include <mach_perf.h>" > sym.c
	echo "struct symbol kernel_symbols[] = {(unsigned) 0, 0, 0};" >> sym.c
	echo "unsigned int kernel_symbols_count = 0;" >> sym.c
	echo char *kernel_symbols_version = \"\"\; >>sym.c
.endif
